/*************************************************************
* NOTE : git_protocol.c
*      protocol
* Author : Lee junho
* Since : 2019.06.11
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"

#include "common.h"
#include "cmsis_os.h"

#include "git_protocol.h"
#include "git_pool.h"
#include "git_function_list.h"
#include "git_rs9116.h"
#include "git_PassthruDefines.h"
#include "git_ListDiag.h"
#include "git_OBDcomm.h"
#include "git_vci.h"
#include "git_ioctl.h"
#include "git_rtc.h"
#include "git_global.h"
/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
extern void clearRXCanMessage( void );
extern void TransmitFunction( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID );
/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
extern stRECORD_HW_SET  g_stGITHWSetData;
extern U16              g_usES95486_RxCANID;
extern osMessageQId	    hDiagMsg;
extern osPoolId	        hDiagPool;
extern osPoolId	        hPTPKPool;

osMessageQId            hListDiagMsg;
osThreadId              hListDiagTh;

typedef __packed struct __stParseData{
	uint8_t ucGuid[36];
	uint8_t ucIndex;
	uint8_t ucType;
	uint8_t ucLength;
	uint8_t Reqcode[12];
	uint8_t ucstate;
}stParseData;
typedef __packed struct __stListDiagInfo{
	uint8_t ucListPos;
	stParseData stParseData[LISTDIAGSTRUCTMAX];
}stListDiagInfo;

#pragma location = ".dtcm_data"
__root stListDiagInfo g_stListDiagInfo;

bool StartlistdiagThread( void )
{
	osMessageQDef( listdiagqueue, MESSAGE_LISTDIAG_QUEUE_SIZE, int );
	osThreadDef( listdiag, ListDiagThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE*3 );
    hListDiagMsg = osMessageCreate( osMessageQ( listdiagqueue ), NULL );
    if( hListDiagMsg == NULL )
    {
        GLogE( "Error... fail create hListDiagMsg Thread!!!\r\n" );
        return false;
    }
    hListDiagTh = osThreadCreate( osThread( listdiag ), NULL );
    if( hListDiagTh == NULL )
    {
        GLogE( "Error... fail create hListDiagTh Thread!!!\r\n" );
        return false;
    }    
    return true;
}

/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/
void ListDiagThread( void const *argument )
{
	osEvent		evt;

	stMsgClst	*message;
	stCommPkt	*packet;

	uint8_t		ucRet = false;
	UUID_Struct ListUUid;
	  
	static uint8_t s_ucRetryCnt = 0;
	//static LISTDIAG_STATUS s_eListdiagstatus = LISTDIAG_NONE;

	for(;;)
	{
		evt = osMessageGet( hListDiagMsg, osWaitForever );
		if( evt.status == osEventMessage )
		{
			message 	= ( stMsgClst * )evt.value.p;
			packet		= ( stCommPkt * )message->pPacket;
			
            switch(message->mMod)
            {
                case LISTDIAG_INIT:
				case LISTSENSOR_INIT:
					GLogN("[LISTDIAG_INIT]\r\n");		
					g_OBD_Processing = true;
                    s_ucRetryCnt = 0;
					memset(&g_stListDiagInfo,0x00,sizeof(stListDiagInfo));
					memcpy(&ListUUid, &(packet->UUID), sizeof(UUID_Struct));
					/*InitGITSetConfig();
					InitGITHWSetData();
                    extern uint32_t g_ulProtocolID;
					g_ulProtocolID = ISO14229_ES95486_02_100;*/
					VCI_REINTI_COMM_STATE(VCI_GetPassThruProtocolID());
					VCI_HW_Setting(VCI_GetPassThruProtocolID());
                    ucRet = ListDiagParsing((uint8_t*)packet->mData,packet->mLen);
                    if( ucRet == true )
                    {
						if( message->mMod == LISTDIAG_INIT )	SendMSGToListDiag(LISTDIAG_RUNNING, message );
						else									SendMSGToListDiag(LISTSENSOR_RUNNING, message );
                    }
                    else
                    {
                    	packet->mData[PASSTHRUELENGTHPOS]=1+sizeof(LISTDIAGENDSTR);
                        packet->mData[SIZE_PASSTHRU_HEADER]=1;
						memcpy(&packet->mData[SIZE_PASSTHRU_HEADER+1],LISTDIAGENDSTR,sizeof(LISTDIAGENDSTR));
                        TransmitFunction(message->mPktType, &packet->mData[0], SIZE_PASSTHRU_HEADER+1+sizeof(LISTDIAGENDSTR), LISTDIAG_END_RES);
                    }
					osPoolFree( hCommPKPool, (void *)packet );
					osPoolFree( hMsgPool, (void *)message );
                    break;
                case LISTDIAG_RUNNING:
				case LISTSENSOR_RUNNING:
                    {
						//GLogN("[Run]%d  ",g_stListDiagInfo.ucListPos);
						if( (g_stListDiagInfo.ucListPos < LISTDIAGSTRUCTMAX) && (g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].ucIndex != 0) )
						{
							MsgDiag_t	*msgDiag;
							PTmsgPkt_t	*pktDiag;
							
							msgDiag	= ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
				            if( msgDiag == NULL )
				            {
				                return;
				            }
				            pktDiag = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
				            if( pktDiag == NULL )
				            {
				                osPoolFree( hDiagPool, (void *)msgDiag );
				                return;
				            }
							
							memcpy(pktDiag->pData,g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].Reqcode,sizeof(g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].Reqcode));
							g_usES95486_RxCANID = (((U16)g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].Reqcode[0]<< 8)+((U16)g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].Reqcode[1]))+8;
							
							memcpy(ListUUid.gitUUID, g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].ucGuid, 36);	// Sensor 출력 guid 추가
							ListUUid.gitUUIDLen = sizeof(g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].ucGuid);
							
							msgDiag->mMsgType 		= MSG_DIAG;
                            msgDiag->mPktType 		= message->mPktType;
							if(message->mMod == LISTDIAG_RUNNING)	msgDiag->event	= FAST_FCS;
							else									msgDiag->event	= LIST_SENSOR;
							msgDiag->subEvent 		= eDIAG_COMM_TX_START;
							msgDiag->unEventTime 	= GetUnixTime();
							msgDiag->pPacket 		= (void *)pktDiag;
                            memcpy(&(pktDiag->UUID), &ListUUid, sizeof(UUID_Struct));
							
							//GLogN("event:%d\r\n",msgDiag->event);
							//GLogN("subEvent:%d\r\n",msgDiag->subEvent);
							
							clearRXCanMessage();	//CAN
							
							if(osMessageAvailableSpace(hDiagMsg) == 0)
							{
								osPoolFree( hPTPKPool, (void *)pktDiag );
								osPoolFree( hDiagPool, (void *)msgDiag );
							}
							else
							{
								osMessagePut( hDiagMsg, (uint32_t)msgDiag, osWaitForever );
								//GLogN("[Send]%d  ",g_stListDiagInfo.ucListPos);
							}
						}
						else
						{
							if( message->mMod == LISTDIAG_RUNNING )	SendMSGToListDiag(LISTDIAG_END, message );
							else
							{
								g_stListDiagInfo.ucListPos=0;
								SendMSGToListDiag(LISTSENSOR_RUNNING, message );
							}
						}
						osPoolFree( hCommPKPool, (void *)packet );
						osPoolFree( hMsgPool, (void *)message );
                    }
                    break;
				case LISTDIAG_CHECK:
				case LISTSENSOR_CHECK:
                    {
						//GLogN("[Res]Pos:%d,Seq:%d\r\n",g_stListDiagInfo.ucListPos,message->mSeq);
						if( message->mSeq == eDIAG_COMM_RX_OK )
						{
						  	GLogN("htransmitMsg : %d\r\n", osMessageAvailableSpace(hTransmitMsg));
							if(osMessageAvailableSpace(hTransmitMsg) == 0)
							{
								GLogE("LISTDIAG_RUNNING eDIAG_COMM_RX_OK\r\n");
								osPoolFree( hCommPKPool, (void *)packet );
								osPoolFree( hMsgPool, (void *)message );
							}
							else
							{
								//osDelay(100);				//시연용(서버 느려서 기다리는 딜레이)
                                MSGChangeToListDiag(packet);
								osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
								osSemaphoreWait(hpairflagSemaphore, osWaitForever);
								g_stListDiagInfo.ucListPos++;
							}
						}
						else	//eDIAG_COMM_RX_FAIL
						{
							GLogN("[FAIL]Idx:%d ",g_stListDiagInfo.ucListPos);
							for(int i=0; i<12; i++)
							{
								GLogN("%02X ",g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].Reqcode[i]);
							}
							GLogN("\r\n");
							if(osMessageAvailableSpace(hTransmitMsg) == 0)
							{
								GLogE("LISTDIAG_RUNNING eDIAG_COMM_RX_FAIL\r\n");
								osPoolFree( hCommPKPool, (void *)packet );
								osPoolFree( hMsgPool, (void *)message );
							}
							else
							{
								if( (s_ucRetryCnt < RETRYCOUNT - 1) && (RETRYCOUNT != 0) )
								{
									s_ucRetryCnt++;
								}
								else
								{
	                                MSGChangeToListDiag(packet);
									osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
									osSemaphoreWait(hpairflagSemaphore, osWaitForever);
									g_stListDiagInfo.ucListPos++;
									if( message->mMod == LISTDIAG_CHECK )
									{
										for( int i=0; i<MAX_REQ_COUNT; i++ )
										{
											if( g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].ucType == 0x02 && g_stListDiagInfo.ucListPos < LISTDIAGSTRUCTMAX)
											{
												g_stListDiagInfo.ucListPos++;
											}
										}
									}
									else{}	//listsensor is not skip frame
									s_ucRetryCnt = 0;
								}
							}
						}

						if( g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].ucIndex != 0x00 && g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].ucType != 0x00 && g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].ucLength != 0x00 )
						{
							if( message->mMod == LISTDIAG_CHECK )	SendMSGToListDiag(LISTDIAG_RUNNING, message );
							else		//LISTSENSOR_CHECK
							{
								if(g_ListSensor_Endflag == false)	SendMSGToListDiag(LISTSENSOR_END, message );
								else								SendMSGToListDiag(LISTSENSOR_RUNNING, message );
							}
						}
						else
						{
							if( message->mMod == LISTDIAG_CHECK )	SendMSGToListDiag(LISTDIAG_END, message );
							else		//LISTSENSOR_CHECK
							{
								if(g_ListSensor_Endflag == false)	SendMSGToListDiag(LISTSENSOR_END, message );
								else
								{
									g_stListDiagInfo.ucListPos = 0;
									SendMSGToListDiag(LISTSENSOR_RUNNING, message );
								}
							}
						}
						osPoolFree( hCommPKPool, (void *)packet );
						osPoolFree( hMsgPool, (void *)message );
                    }
                    break;
				case LISTDIAG_END:
				case LISTSENSOR_END:
					if( message->mMod == LISTDIAG_END )
					{
						GLogN("\r\n[LISTDIAG_END]\r\n");
						packet->mData[PASSTHRUELENGTHPOS]=1+sizeof(LISTDIAGENDSTR);
						packet->mData[SIZE_PASSTHRU_HEADER]=0;
						memcpy(&packet->mData[SIZE_PASSTHRU_HEADER+1],LISTDIAGENDSTR,sizeof(LISTDIAGENDSTR));
	                    TransmitFunction(message->mPktType, &packet->mData[0], SIZE_PASSTHRU_HEADER+1+sizeof(LISTDIAGENDSTR), LISTDIAG_END_RES);
					}
					else
					{
						GLogN("\r\n[LISTSENSOR_END]\r\n");
						clearRXCanMessage();
						clearTXCanMessage();
						packet->mData[PASSTHRUELENGTHPOS]=1+sizeof(LISTSENSORENDSTR);
						packet->mData[SIZE_PASSTHRU_HEADER]=0;
						memcpy(&packet->mData[SIZE_PASSTHRU_HEADER+1],LISTSENSORENDSTR,sizeof(LISTSENSORENDSTR));
	                    TransmitFunction(message->mPktType, &packet->mData[0], SIZE_PASSTHRU_HEADER+1+sizeof(LISTSENSORENDSTR), LISTSENSOR_END_RES);
					}
                    osPoolFree( hCommPKPool, (void *)packet );
					osPoolFree( hMsgPool, (void *)message );
					g_ListSensor_Endflag	= true;
					g_OBD_Processing		= false;					// ListSensor Processing End
					osSignalSet(hParsingTh, OBD_ListDiag_END);
					break;
				case LISTDIAG_FAIL:
				case LISTSENSOR_FAIL:
					osPoolFree( hCommPKPool, (void *)packet );
					osPoolFree( hMsgPool, (void *)message );
					break;
                default:
					osPoolFree( hCommPKPool, (void *)packet );
					osPoolFree( hMsgPool, (void *)message );
					break;
            }
		}

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#else
        osDelay( 1 );
#endif
	}
}

uint8_t ListDiagParsing(uint8_t *ucInputBuff,uint16_t usLength)
{
	uint8_t ucBuff[LISTDIAGSIZE] = {0};
	uint8_t ucListIndex = 0;
    int		i			= 0;
	    
    if( usLength< LISTDIAGSIZE )
    {
        memcpy(ucBuff,ucInputBuff,usLength);
		//GLogN("~~~~~~~~ListDiagParsing~~~~~~~~~~\r\n");
    }
    else
    {
        GLogE("ListDiagParsing Oversize%d\r\n",usLength);
        return false;
    }
    
	while(i < usLength)
	{
		if( (ucBuff[i] != 0) && (ucBuff[i+1] != 0) )
		{
			memcpy(g_stListDiagInfo.stParseData[ucListIndex].ucGuid, &ucBuff[i], 36);
			i += 36;
			g_stListDiagInfo.stParseData[ucListIndex].ucIndex = ucBuff[i++];
			g_stListDiagInfo.stParseData[ucListIndex].ucType = ucBuff[i++];
            g_stListDiagInfo.stParseData[ucListIndex].ucLength = ucBuff[i++];
            memcpy(g_stListDiagInfo.stParseData[ucListIndex].Reqcode, &ucBuff[i], g_stListDiagInfo.stParseData[ucListIndex].ucLength);
            i += g_stListDiagInfo.stParseData[ucListIndex].ucLength;
		}
		else
		{
			GLogN("ListDiagParsing fail%d\r\n",i);
			return false;
		}
        ucListIndex++;
	}
    return true;
}

void InitGITSetConfig()
{
	memset((void*)&g_stGITSetConfig, 0x00, sizeof(stGITSetConfig));
	g_stGITSetConfig.nDataRate 		= 10417;
	g_stGITSetConfig.nLoopBack 		= OFF;
	g_stGITSetConfig.nNodeAddress 	= 0;
	g_stGITSetConfig.nNetworkLine 	= 0;
	g_stGITSetConfig.nP1Min 		= 0;
	g_stGITSetConfig.nP1Max 		= 20;
	g_stGITSetConfig.nP2Min 		= 25;
	g_stGITSetConfig.nP2Max 		= 50;
	g_stGITSetConfig.nP3Min 		= 55;
	g_stGITSetConfig.nP3Max 		= 5000;
	g_stGITSetConfig.nP4Min 		= 5;
	g_stGITSetConfig.nP4Max 		= 20;
	g_stGITSetConfig.nW1 			= 1000;
	g_stGITSetConfig.nW2 			= 1000;
	g_stGITSetConfig.nW3 			= 1000;
	g_stGITSetConfig.nW4 			= 30;
	g_stGITSetConfig.nW5 			= 300;
	g_stGITSetConfig.nTIdle 		= 300;
	g_stGITSetConfig.nTInil 		= 25;
	g_stGITSetConfig.nTWUp 			= 50;
	g_stGITSetConfig.nParity 		= 0;
	g_stGITSetConfig.nBitSamplePoint = 80;
	g_stGITSetConfig.nSyncJumpWidth = 15;
	g_stGITSetConfig.nT1Max 		= 20;
	g_stGITSetConfig.nT2Max 		= 100;
	g_stGITSetConfig.nT3Max 		= 50;
	g_stGITSetConfig.nT4Max 		= 20;
	g_stGITSetConfig.nT5Max 		= 100;
	g_stGITSetConfig.nIso15765BS 	= 0;
	g_stGITSetConfig.nIso15765STMin = 0;
	g_stGITSetConfig.nBSTx 			= 0xFFFF;
	g_stGITSetConfig.nSTMinTx 		= 0xFFFF;
	g_stGITSetConfig.nDataBits 		= 0;
	g_stGITSetConfig.nFiveBaudMod 	= 0;
	g_stGITSetConfig.nToolManufacturerSpec = 0;
	g_stGITSetConfig.nEtc1 			= 0;
	g_stGITSetConfig.nEtc2 			= 0;
	g_stGITSetConfig.nEtc3 			= 0;
	g_stGITSetConfig.nEtc4 			= 0;
	g_stGITSetConfig.nEtc5 			= 0;
}

void InitGITHWSetData()
{
	memset((void*)&g_stGITHWSetData, 0x00, sizeof(g_stGITHWSetData));
	g_stGITHWSetData.nCommRelay 	= Highcan1;
	g_stGITHWSetData.nKlineSelect 	= K_SERIAL;
	g_stGITHWSetData.nKlineStatus 	= K_NORMAL;
	g_stGITHWSetData.nLlineSelect 	= L_PULSE;
	g_stGITHWSetData.nLlineStatus 	= L_NORMAL;
	g_stGITHWSetData.nKlineSwitchStatus = K_PULSE_HIGH;
	g_stGITHWSetData.nLlineSwitchStatus = L_PULSE_HIGH;
	g_stGITHWSetData.nRxLineSelect 	= RXD_HIGHCAN;
	g_stGITHWSetData.nRxLineStatus 	= RXD_NORMAL;
	g_stGITHWSetData.nPullupRelay 	= Pullup;
	g_stGITHWSetData.nKlinePullup 	= K_Pullup_510;
	g_stGITHWSetData.nLlinePullup 	= L_Pullup_510;
	g_stGITHWSetData.nLlineGnd 		= 0;
	g_stGITHWSetData.nKlineCh 		= KL_LINE1_CONNECT_CH06;
	g_stGITHWSetData.nLlineCh 		= KL_LINE2_CONNECT_CH14;
	g_stGITHWSetData.nRProgramCh 	= R_ChOff;
//	g_stGITHWSetData.nEtc1 			=
//	g_stGITHWSetData.nEtc2 			=
//	g_stGITHWSetData.nEtc3 			=
//	g_stGITHWSetData.nEtc4 			=
//	g_stGITHWSetData.nEtc5 			=
//	g_stGITHWSetData.ackmessage[20] =
}

void SendMSGToListDiag(u16 Mode, stMsgClst *message )
{
	stMsgClst	*msg;
	stCommPkt	*pkt;
	
	ePKT_TD p_type = message->mPktType;
	eMSG_TD	m_type = message->mMsgType;
	
    msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
    if( msg == NULL )
    {
        return;
    }
    pkt = ( stCommPkt* )osPoolCAlloc( hCommPKPool );
    if( pkt == NULL )
    {
        osPoolFree( hMsgPool, (void *)msg );
        return;
    }

    msg->mPktType = p_type;
    msg->mMsgType = m_type;
	msg->mMod = Mode;
	memcpy(pkt,message->pPacket,sizeof(stCommPkt));
	msg->pPacket	= (void *)pkt;
	if(osMessageAvailableSpace(hListDiagMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)pkt );
		osPoolFree( hMsgPool, (void *)msg );
	}
	else
	{
		osMessagePut( hListDiagMsg, (uint32_t)msg, osWaitForever );
		//GLogN("SendMSGToListDiag\r\n");
	}
}

void MSGChangeToListDiag(stCommPkt *packet )
{
	stPASSTHRU_MSG PassThruMsg;
    memset(&PassThruMsg,0x00,sizeof(PassThruMsg));
	memcpy(&PassThruMsg,&packet->mData[0],packet->mLen);
    packet->mData[SIZE_PASSTHRU_HEADER]=g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].ucIndex;
    packet->mData[SIZE_PASSTHRU_HEADER+1]=g_stListDiagInfo.stParseData[g_stListDiagInfo.ucListPos].ucType;
	packet->mData[SIZE_PASSTHRU_HEADER+2]=PassThruMsg.DataSize; 
    PassThruMsg.DataSize = PassThruMsg.DataSize+RESDATA_ADDINFOSIZE;
    memcpy(&packet->mData[0],&PassThruMsg,SIZE_PASSTHRU_HEADER);
    memcpy(&packet->mData[SIZE_PASSTHRU_HEADER+3],&PassThruMsg.pData,PassThruMsg.DataSize-RESDATA_ADDINFOSIZE);
    packet->mLen = packet->mLen+RESDATA_ADDINFOSIZE;
}