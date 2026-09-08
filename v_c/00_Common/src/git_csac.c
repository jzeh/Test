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
#include "git_CsacDiag.h"
#include "git_OBDcomm.h"
#include "git_vci.h"
#include "git_ioctl.h"
#include "git_rtc.h"
#include "git_hsm.h"
#include "git_HSM_Operations.h"  // For hsm_read_certificate() - New HSM
#include "git_fsutil.h"         // For GetCrlEmmc() - CRL from EMMC
#include "gpio.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
extern void clearRXCanMessage( void );
extern void clearTXCanMessage( void );	
extern void TransmitFunction( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID );
/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
extern stRECORD_HW_SET g_stGITHWSetData;
extern U16 g_usES95486_RxCANID;
extern osMessageQId	hDiagMsg;
extern osPoolId	hDiagPool;
extern osPoolId	hPTPKPool;

extern bool g_bCF_TxComplete;// Ignore flow control out of sequence

//#define CSAC_TEST

osMessageQId hCsacDiagMsg;
osThreadId hCsacDiagTh;

typedef __packed struct __stParseData{
	uint32_t ucLength;
	uint8_t Reqcode[CSACDIAGSIZE];
}stParseData;

typedef __packed struct __stCsacDiagInfo{
    uint8_t ucCsacPos; //Pending Count
    uint8_t ucCsacStep; //Secure Access Step
    uint16_t CsacMode; // CSAC1.0 or CSAC2.0
    uint8_t CanId[4];
    stParseData stParseData[CSACDIAGSTRUCTMAX]; // [0]:CAN Data, [1]:App Data, [2]:Reserved
}stCsacDiagInfo;

stCsacDiagInfo g_stCsacDiagInfo;

bool StartCsacdiagThread( void )
{
	
	osMessageQDef( csacdiagqueue, MESSAGE_CSACDIAG_QUEUE_SIZE, int );
	osThreadDef( csacdiag, CsacDiagThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE*3 );
    hCsacDiagMsg = osMessageCreate( osMessageQ( csacdiagqueue ), NULL );
    if( hCsacDiagMsg == NULL )
    {
        GLogE( "Error... fail create hCsacDiagMsg Thread!!!\r\n" );
        return false;
    }
    hCsacDiagTh = osThreadCreate( osThread( csacdiag ), NULL );
    if( hCsacDiagTh == NULL )
    {
        GLogE( "Error... fail create hCsacDiagTh Thread!!!\r\n" );
        return false;
    }    
    return true;
}

/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/
void CsacDiagThread( void const *argument )
{
	osEvent		evt;

	stMsgClst	*message;
	stCommPkt	*packet;

    uint8_t		bPendingFlag = false;
    U32 uiOutDataLen = 0;
    U32 uiLogSize = 0;
    U8 ucCRT = 0;
    U8 ucCRL = 0;
    U8 ucTemp[1200] = {0x00, };
    bool b_sha2 = false; //0x29 Service
    bool b_exCAN = false; // Extended CAN Flag

	for(;;)
	{
		evt = osMessageGet( hCsacDiagMsg, osWaitForever );
		if( evt.status == osEventMessage )
		{
			message 	= ( stMsgClst * )evt.value.p;
			packet		= ( stCommPkt * )message->pPacket;
			
            switch(message->mMod)
            {
                case CSACDIAG_INIT:
                    {
                        GLogN("[CSACDIAG_INIT]\r\n");
                        memset(&g_stCsacDiagInfo,0x00,sizeof(stCsacDiagInfo));
#if defined(CSAC_TEST)
                        InitGITSetConfig2();
                        InitGITHWSetData2();
                        //VCI_HW_Setting(ISO14229_ES95486_02_100_NEW);
                        VCI_HW_Setting(ISO15765_29BIT);
#endif
                    
                        g_stCsacDiagInfo.CsacMode = packet->mFuncID;
                        
                        if(g_stCsacDiagInfo.CsacMode == CSAC10) //CSAC1.0
                        {
                            ucCRT = packet->mData[0];
                            ucCRL = 0; //Do not use
                        }
                        else //CSAC2.0 SHA1/SHA2
                        {
                            ucCRT = packet->mData[0];
                            ucCRL = packet->mData[1];
                        }
#if defined(CSAC_TEST)
                        ucCRT = 1;
                        ucCRL = 1;
#endif
                        
                        g_stCsacDiagInfo.stParseData[1].ucLength = 0;
                        
                        
                        if((packet->mData[2] != 0x00) && (packet->mData[3] != 0x00))
                        {
                            b_exCAN = true;
                        }
                        else
                        {
                            b_exCAN = false;
                        }
                        
                        if( b_exCAN )
                        {
                            memcpy(g_stCsacDiagInfo.CanId, &packet->mData[2], 4); // CANID
                        }
                        else
                        {
                            memcpy(g_stCsacDiagInfo.CanId, &packet->mData[4], 2); // CANID
                        }
                        
                        memcpy(&g_stCsacDiagInfo.stParseData[0].Reqcode[0], &packet->mData[6], 2); //27 42 or 29 01
                        
                        if((g_stCsacDiagInfo.stParseData[0].Reqcode[0] == 0x29)&&(g_stCsacDiagInfo.stParseData[0].Reqcode[1] == 0x01))
                        {
                            b_sha2 = true;
                        }
                        else
                        {
                            b_sha2 = false;
                        }
                            
                        if (g_HSM_Type == HSM_TYPE_OLD)
                        {
                            // ===== Old HSM (Coregate UART) =====
                            ActivationHSM();

                            if(ReadCertificateHSM(&g_stCsacDiagInfo.stParseData[0].Reqcode[2], (int*)&uiOutDataLen, ucCRT) == HSM_SUCCESS)
                            {
                                g_stCsacDiagInfo.stParseData[0].ucLength += uiOutDataLen;

                                if(g_stCsacDiagInfo.CsacMode == CSAC10)
                                {
                                    g_stCsacDiagInfo.stParseData[0].ucLength += 2; // +2741
                                    SendMSGToCsacDiag(CSACDIAG_RUNNING, message );
                                }
                                else if((g_stCsacDiagInfo.CsacMode == CSAC20)&&(ReadDataHSM(&g_stCsacDiagInfo.stParseData[0].Reqcode[2+uiOutDataLen], CRL_LENGTH, ucCRL)))
                                {
                                    g_stCsacDiagInfo.stParseData[0].ucLength += (CRL_LENGTH+2); //CRL+"2741"

                                    if( b_sha2 )
                                    {
                                        // make ccu2 secure access packet
                                        // packet : 29 01 00 [CRT+CRL Length] [CRT] [CRL] 00 00
                                        memset(&g_stCsacDiagInfo.stParseData[0].Reqcode[g_stCsacDiagInfo.stParseData[0].ucLength], 0x00, 2);
                                        memcpy(ucTemp, &g_stCsacDiagInfo.stParseData[0].Reqcode[2], g_stCsacDiagInfo.stParseData[0].ucLength);
                                        memcpy(&g_stCsacDiagInfo.stParseData[0].Reqcode[5], ucTemp, g_stCsacDiagInfo.stParseData[0].ucLength);
                                        g_stCsacDiagInfo.stParseData[0].Reqcode[2] = 0x00;
                                        g_stCsacDiagInfo.stParseData[0].Reqcode[3] = (g_stCsacDiagInfo.stParseData[0].ucLength-2)>>8;
                                        g_stCsacDiagInfo.stParseData[0].Reqcode[4] = (g_stCsacDiagInfo.stParseData[0].ucLength-2)&0x00FF;
                                        g_stCsacDiagInfo.stParseData[0].ucLength += 5;
                                    }

                                    SendMSGToCsacDiag(CSACDIAG_RUNNING, message );
                                }
                                else
                                {
                                    g_stCsacDiagInfo.stParseData[1].Reqcode[0] = FAIL;
                                    g_stCsacDiagInfo.stParseData[1].Reqcode[1] = 0xF1; //Fail to Read CRL or CsacMode Error
                                    SendMSGToCsacDiag(CSACDIAG_FAIL, message );
                                }
                            }
                            else
                            {
                                g_stCsacDiagInfo.stParseData[1].Reqcode[0] = FAIL;
                                g_stCsacDiagInfo.stParseData[1].Reqcode[1] = 0xF2; //Fail to Read CRT
                                SendMSGToCsacDiag(CSACDIAG_FAIL, message );
                            }
                        }
                        else
                        {
                            // ===== New HSM (Autocrypt SPI) =====
                            // No activation needed for New HSM
                            U16 cert_length = 0;
							U16 mapped_cert_id = 140 + ucCRT;

                            if(hsm_read_certificate(mapped_cert_id, HSM_ALG_RSA, &g_stCsacDiagInfo.stParseData[0].Reqcode[2], &cert_length) == HAL_OK)
                            {
                                // New HSM returns 616 bytes, but use only 600 bytes for ECU compatibility
                                uiOutDataLen = 600;
                                g_stCsacDiagInfo.stParseData[0].ucLength += uiOutDataLen;

                                if(g_stCsacDiagInfo.CsacMode == CSAC10)
                                {
                                    g_stCsacDiagInfo.stParseData[0].ucLength += 2; // +2741
                                    SendMSGToCsacDiag(CSACDIAG_RUNNING, message );
                                }
                                else if(g_stCsacDiagInfo.CsacMode == CSAC20)
                                {
                                    // New HSM uses EMMC for CRL storage
                                    U32 crl_length = 0;
                                    if(GetCrlEmmc(ucCRL, &g_stCsacDiagInfo.stParseData[0].Reqcode[2+uiOutDataLen], &crl_length) == TRUE)
                                    {
                                        g_stCsacDiagInfo.stParseData[0].ucLength += (crl_length + 2); //CRL+"2741"

                                        if( b_sha2 )
                                        {
                                            // make ccu2 secure access packet
                                            // packet : 29 01 00 [CRT+CRL Length] [CRT] [CRL] 00 00
                                            memset(&g_stCsacDiagInfo.stParseData[0].Reqcode[g_stCsacDiagInfo.stParseData[0].ucLength], 0x00, 2);
                                            memcpy(ucTemp, &g_stCsacDiagInfo.stParseData[0].Reqcode[2], g_stCsacDiagInfo.stParseData[0].ucLength);
                                            memcpy(&g_stCsacDiagInfo.stParseData[0].Reqcode[5], ucTemp, g_stCsacDiagInfo.stParseData[0].ucLength);
                                            g_stCsacDiagInfo.stParseData[0].Reqcode[2] = 0x00;
                                            g_stCsacDiagInfo.stParseData[0].Reqcode[3] = (g_stCsacDiagInfo.stParseData[0].ucLength-2)>>8;
                                            g_stCsacDiagInfo.stParseData[0].Reqcode[4] = (g_stCsacDiagInfo.stParseData[0].ucLength-2)&0x00FF;
                                            g_stCsacDiagInfo.stParseData[0].ucLength += 5;
                                        }

                                        SendMSGToCsacDiag(CSACDIAG_RUNNING, message );
                                    }
                                    else
                                    {
                                        g_stCsacDiagInfo.stParseData[1].Reqcode[0] = FAIL;
                                        g_stCsacDiagInfo.stParseData[1].Reqcode[1] = 0xF1; //Fail to Read CRL from EMMC
                                        SendMSGToCsacDiag(CSACDIAG_FAIL, message );
                                    }
                                }
                                else
                                {
                                    g_stCsacDiagInfo.stParseData[1].Reqcode[0] = FAIL;
                                    g_stCsacDiagInfo.stParseData[1].Reqcode[1] = 0xF1; //Invalid CsacMode
                                    SendMSGToCsacDiag(CSACDIAG_FAIL, message );
                                }
                            }
                            else
                            {
                                g_stCsacDiagInfo.stParseData[1].Reqcode[0] = FAIL;
                                g_stCsacDiagInfo.stParseData[1].Reqcode[1] = 0xF2; //Fail to Read CRT
                                SendMSGToCsacDiag(CSACDIAG_FAIL, message );
                            }
                        }
                        
                        osPoolFree( hCommPKPool, (void *)packet );
                        osPoolFree( hMsgPool, (void *)message );
                        
                        break;
                    }
                case CSACDIAG_RUNNING:
                    {
                        g_bCF_TxComplete=0;
                        g_usES95486_RxCANID = (g_stCsacDiagInfo.CanId[0]<<8) + g_stCsacDiagInfo.CanId[1] + 0x08;

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
                            osPoolFree( hMsgPool, (void *)msgDiag );
                            return;
                        }
                        
                        /********************Tx CAN Data********************/
                        memcpy(&pktDiag->pData[0],g_stCsacDiagInfo.CanId,sizeof(g_stCsacDiagInfo.CanId));
                        
                        if( g_stCsacDiagInfo.stParseData[0].ucLength <= 8 ) // Single Frame
                        {
                            if( b_exCAN )
                            {
                                memset(&pktDiag->pData[4], g_stCsacDiagInfo.stParseData[0].ucLength, 1);
                                memcpy(&pktDiag->pData[5], &g_stCsacDiagInfo.stParseData[0].Reqcode[0], g_stCsacDiagInfo.stParseData[0].ucLength);
                            }
                            else
                            {
                                memset(&pktDiag->pData[2], g_stCsacDiagInfo.stParseData[0].ucLength, 1);
                                memcpy(&pktDiag->pData[3], &g_stCsacDiagInfo.stParseData[0].Reqcode[0], g_stCsacDiagInfo.stParseData[0].ucLength);
                            }
                        }
                        else // Multi Frame
                        {
                            if( b_exCAN )
                            {
                                memset(&pktDiag->pData[4], 0x10|((g_stCsacDiagInfo.stParseData[0].ucLength&0xFF00)>>8), 1);
                                memset(&pktDiag->pData[5], g_stCsacDiagInfo.stParseData[0].ucLength&0x00FF, 1);
                                memcpy(&pktDiag->pData[6], &g_stCsacDiagInfo.stParseData[0].Reqcode[0], g_stCsacDiagInfo.stParseData[0].ucLength);
                            }
                            else
                            {
                                memset(&pktDiag->pData[2], 0x10|((g_stCsacDiagInfo.stParseData[0].ucLength&0xFF00)>>8), 1);
                                memset(&pktDiag->pData[3], g_stCsacDiagInfo.stParseData[0].ucLength&0x00FF, 1);
                                memcpy(&pktDiag->pData[4], &g_stCsacDiagInfo.stParseData[0].Reqcode[0], g_stCsacDiagInfo.stParseData[0].ucLength);
                            }
                        }
                        /****************************************************/
                        
                        /********************Save CAN Log********************/
                        //Only Multi Frame works fine... don't use Single Frame, so it's okay.....
                        if( b_exCAN )
                        {
                            uiLogSize = g_stCsacDiagInfo.stParseData[0].ucLength+sizeof(g_stCsacDiagInfo.CanId)+2; //DataLen + CANIDLen(4) + DLCLen
                            memcpy(&g_stCsacDiagInfo.stParseData[1].Reqcode[g_stCsacDiagInfo.stParseData[1].ucLength+4], &pktDiag->pData[0], uiLogSize);
                            g_stCsacDiagInfo.stParseData[1].ucLength += uiLogSize;
                        }
                        else
                        {
                            uiLogSize = g_stCsacDiagInfo.stParseData[0].ucLength+(sizeof(g_stCsacDiagInfo.CanId)/2)+2; //DataLen + CANIDLen(2) + DLCLen
                            memcpy(&g_stCsacDiagInfo.stParseData[1].Reqcode[g_stCsacDiagInfo.stParseData[1].ucLength+4], &pktDiag->pData[0], uiLogSize);
                            g_stCsacDiagInfo.stParseData[1].ucLength += uiLogSize;
                        }
                        /****************************************************/
                        
                        pktDiag->DataSize = uiLogSize;
                        
                        msgDiag->mMsgType 		= MSG_DIAG;
                        msgDiag->mPktType 		= message->mPktType;
                        msgDiag->event 			= CAN_CSAC;
                        msgDiag->subEvent 		= eDIAG_COMM_TX_START;
                        msgDiag->unEventTime 	= GetUnixTime();
                        msgDiag->pPacket 		= (void *)pktDiag;

                        clearTXCanMessage();
                        clearRXCanMessage();
                        
                        if(osMessageAvailableSpace(hDiagMsg) == 0)
                        {
                            osPoolFree( hDiagPool, (void *)pktDiag );
                            osPoolFree( hPTPKPool, (void *)msgDiag );
                        }
                        else
                        {
                            osMessagePut( hDiagMsg, (uint32_t)msgDiag, osWaitForever );
                        }
						
						osPoolFree( hCommPKPool, (void *)packet );
						osPoolFree( hMsgPool, (void *)message );
                    }
                    break;
				case CSACDIAG_CHECK:
                    {
                        bPendingFlag = false;
						
						if( message->mSeq == eDIAG_COMM_RX_OK )
						{
							if(osMessageAvailableSpace(hTransmitMsg) == 0)
							{
								GLogE("CSACDIAG_RUNNING COMM_RX_OK\r\n");
								osPoolFree( hCommPKPool, (void *)packet );
								osPoolFree( hMsgPool, (void *)message );
							}
							else
							{
                                //commercial car
                                if( b_exCAN )
                                {
                                    if( packet->mData[SIZE_PASSTHRU_HEADER+4] == 0x7F && packet->mData[SIZE_PASSTHRU_HEADER+6] == 0x78 ) //Pending
                                    {
                                        bPendingFlag=true;
                                    }
                                    else
                                    {    
                                        bPendingFlag=false;
                                    }
                                    
                                    //Secure Access Step 1
                                    if( (packet->mData[SIZE_PASSTHRU_HEADER+4] == 0x67 && packet->mData[SIZE_PASSTHRU_HEADER+5] == 0x41))
                                    {
                                        memset(&g_stCsacDiagInfo.stParseData[0], 0x00, CSACDIAGSIZE);
                                        
                                        
                                        g_stCsacDiagInfo.stParseData[0].Reqcode[0] = 0x27;
                                        g_stCsacDiagInfo.stParseData[0].Reqcode[1] = 0x42;
                                        
                                        //Sign SHA1, SHA2 Seed
                                        GLogN("\r\nSeed : ");
                                        for(int i=0; i<8; i++)
                                        {
                                            GLogN("0x%02X ", packet->mData[SIZE_PASSTHRU_HEADER+6+i]);
                                        }
                                        GLogN("\r\n");
                                              
                                        if(ucCRT%2 == 0) //SHA2
                                        {
                                            if (g_HSM_Type == HSM_TYPE_OLD) {
                                                SignPrivateHSMSHA256(&packet->mData[SIZE_PASSTHRU_HEADER+6], 8, &g_stCsacDiagInfo.stParseData[0].Reqcode[2], (int*)&uiOutDataLen, ucCRT);
                                            } else {
                                                hsm_sign_rsa_with_seed_new(&packet->mData[SIZE_PASSTHRU_HEADER+6], 8,
                                                                       &g_stCsacDiagInfo.stParseData[0].Reqcode[2], &uiOutDataLen,
                                                                       hsm_map_csac_key(ucCRT), HSM_SHA_256);
                                            }
                                        }
                                        else //SHA1
                                        {
                                            if (g_HSM_Type == HSM_TYPE_OLD) {
                                                SignPrivateHSM(&packet->mData[SIZE_PASSTHRU_HEADER+6], 8, &g_stCsacDiagInfo.stParseData[0].Reqcode[2], (int*)&uiOutDataLen, ucCRT);
                                            } else {
                                                hsm_sign_rsa_with_seed_new(&packet->mData[SIZE_PASSTHRU_HEADER+6], 8,
                                                                              &g_stCsacDiagInfo.stParseData[0].Reqcode[2], &uiOutDataLen,
                                                                              hsm_map_csac_key(ucCRT), HSM_SHA_160);
                                            }
                                        }

                                        g_stCsacDiagInfo.stParseData[0].ucLength = uiOutDataLen+2;

                                        g_stCsacDiagInfo.ucCsacStep = SECURE_ACCESS_STEP;
                                    }

                                    //Secure Access Step 2
                                    else if( (packet->mData[SIZE_PASSTHRU_HEADER+4] == 0x67 && packet->mData[SIZE_PASSTHRU_HEADER+5] == 0x42)) //CSAC SHA1
                                    {
                                        g_stCsacDiagInfo.stParseData[1].Reqcode[0] = SUCCESS;
                                        g_stCsacDiagInfo.ucCsacStep = SECURE_ACCESS_END;
                                    }
                                    
                                    else
                                    {
                                        g_stCsacDiagInfo.stParseData[1].Reqcode[0] = FAIL;
                                        g_stCsacDiagInfo.stParseData[1].Reqcode[1] = packet->mData[SIZE_PASSTHRU_HEADER+4]; //NRC
                                        g_stCsacDiagInfo.ucCsacStep = SECURE_ACCESS_ERROR;
                                    }
                                }
                                
                                //passenger car
                                else
                                {
                                    if( packet->mData[SIZE_PASSTHRU_HEADER+2] == 0x7F && packet->mData[SIZE_PASSTHRU_HEADER+4] == 0x78 ) //Pending
                                    {
                                        bPendingFlag=true;
                                    }
                                    else
                                    {    
                                        bPendingFlag=false;
                                    }
                                    
                                    //Secure Access Step 1
                                    if( (packet->mData[SIZE_PASSTHRU_HEADER+2] == 0x67 && packet->mData[SIZE_PASSTHRU_HEADER+3] == 0x41) ||
                                        (packet->mData[SIZE_PASSTHRU_HEADER+2] == 0x69 && packet->mData[SIZE_PASSTHRU_HEADER+3] == 0x01) )
                                    {
                                        memset(&g_stCsacDiagInfo.stParseData[0], 0x00, CSACDIAGSIZE);
                                        
                                        if( b_sha2 )
                                        {
                                            g_stCsacDiagInfo.stParseData[0].Reqcode[0] = 0x29;
                                            g_stCsacDiagInfo.stParseData[0].Reqcode[1] = 0x03;
                                            
                                            //Sign SHA2 Seed
                                            GLogN("\r\nSeed : ");
                                            for(int i=0; i<8; i++)
                                            {
                                                GLogN("0x%02X ", packet->mData[SIZE_PASSTHRU_HEADER+7+i]);
                                            }
                                            GLogN("\r\n");
                                            
                                            if (g_HSM_Type == HSM_TYPE_OLD) {
                                                SignPrivateHSMSHA256(&packet->mData[SIZE_PASSTHRU_HEADER+7], 8, &g_stCsacDiagInfo.stParseData[0].Reqcode[4], (int*)&uiOutDataLen, PA_SHA2);
                                            } else {
                                                hsm_sign_rsa_with_seed_new(&packet->mData[SIZE_PASSTHRU_HEADER+7], 8,
                                                                       &g_stCsacDiagInfo.stParseData[0].Reqcode[4], &uiOutDataLen,
                                                                       hsm_map_csac_key(PA_SHA2), HSM_SHA_256);
                                            }

                                            g_stCsacDiagInfo.stParseData[0].Reqcode[2] = uiOutDataLen>>8;
                                            g_stCsacDiagInfo.stParseData[0].Reqcode[3] = uiOutDataLen&0x00FF;
                                            memset(&g_stCsacDiagInfo.stParseData[0].Reqcode[4+uiOutDataLen], 0x00, 2);

                                            g_stCsacDiagInfo.stParseData[0].ucLength = uiOutDataLen+6;
                                        }
                                        else
                                        {
                                            g_stCsacDiagInfo.stParseData[0].Reqcode[0] = 0x27;
                                            g_stCsacDiagInfo.stParseData[0].Reqcode[1] = 0x42;

                                            //Sign SHA1, SHA2 Seed
                                            GLogN("\r\nSeed : ");
                                            for(int i=0; i<8; i++)
                                            {
                                                GLogN("0x%02X ", packet->mData[SIZE_PASSTHRU_HEADER+4+i]);
                                            }
                                            GLogN("\r\n");

                                            if(ucCRT%2 == 0) //SHA2
                                            {
                                                if (g_HSM_Type == HSM_TYPE_OLD) {
                                                    SignPrivateHSMSHA256(&packet->mData[SIZE_PASSTHRU_HEADER+4], 8, &g_stCsacDiagInfo.stParseData[0].Reqcode[2], (int*)&uiOutDataLen, ucCRT);
                                                } else {
                                                    hsm_sign_rsa_with_seed_new(&packet->mData[SIZE_PASSTHRU_HEADER+4], 8,
                                                                           &g_stCsacDiagInfo.stParseData[0].Reqcode[2], &uiOutDataLen,
                                                                           hsm_map_csac_key(ucCRT),HSM_SHA_256);
                                                }
                                            }
                                            else //SHA1
                                            {
                                                if (g_HSM_Type == HSM_TYPE_OLD) {
                                                    SignPrivateHSM(&packet->mData[SIZE_PASSTHRU_HEADER+4], 8, &g_stCsacDiagInfo.stParseData[0].Reqcode[2], (int*)&uiOutDataLen, ucCRT);
                                                } else {
                                                    hsm_sign_rsa_with_seed_new(&packet->mData[SIZE_PASSTHRU_HEADER+4], 8,
                                                                                  &g_stCsacDiagInfo.stParseData[0].Reqcode[2], &uiOutDataLen,
                                                                                  hsm_map_csac_key(ucCRT),HSM_SHA_160);
                                                }
                                            }

                                            g_stCsacDiagInfo.stParseData[0].ucLength = uiOutDataLen+2;
                                        }
                                        
                                        g_stCsacDiagInfo.ucCsacStep = SECURE_ACCESS_STEP;
                                    }
                                    
                                    //Secure Access Step 2
                                    else if( (packet->mData[SIZE_PASSTHRU_HEADER+2] == 0x67 && packet->mData[SIZE_PASSTHRU_HEADER+3] == 0x42) || //CSAC SHA1
                                             (packet->mData[SIZE_PASSTHRU_HEADER+2] == 0x69 && packet->mData[SIZE_PASSTHRU_HEADER+3] == 0x03) || //CSAC SHA2
                                             (packet->mData[SIZE_PASSTHRU_HEADER+2] == 0x67 && packet->mData[SIZE_PASSTHRU_HEADER+3] == 0x63)) //CSAC HI
                                    {
                                        g_stCsacDiagInfo.stParseData[1].Reqcode[0] = SUCCESS;
                                        g_stCsacDiagInfo.ucCsacStep = SECURE_ACCESS_END;
                                    }
                                    
                                    //CSAC HI
                                    else if( (packet->mData[SIZE_PASSTHRU_HEADER+2] == 0x67 && packet->mData[SIZE_PASSTHRU_HEADER+3] == 0x61) ||
                                             (packet->mData[SIZE_PASSTHRU_HEADER+2] == 0x67 && packet->mData[SIZE_PASSTHRU_HEADER+3] == 0x62))
                                    {
                                        g_stCsacDiagInfo.stParseData[0].Reqcode[0] = 0x27;
                                        g_stCsacDiagInfo.stParseData[0].Reqcode[1] =  packet->mData[SIZE_PASSTHRU_HEADER+3] + 1;
                                        
                                        if( packet->mData[SIZE_PASSTHRU_HEADER+3] == 0x61 )
                                        {
                                            g_stCsacDiagInfo.stParseData[0].ucLength = 2;
                                        }
                                        else
                                        {
                                            //Sign HI Seed
                                            GLogN("\r\nSeed : ");
                                            for(int i=0; i<8; i++)
                                            {
                                                GLogN("0x%02X ", packet->mData[SIZE_PASSTHRU_HEADER+4+i]);
                                            }
                                            GLogN("\r\n");
                                            
                                            if (g_HSM_Type == HSM_TYPE_OLD) {
                                                SignPrivateHSM(&packet->mData[SIZE_PASSTHRU_HEADER+4], 8, &g_stCsacDiagInfo.stParseData[0].Reqcode[2], (int*)&uiOutDataLen, PA_SHA1);
                                            } else {
                                                hsm_sign_rsa_with_seed_new(&packet->mData[SIZE_PASSTHRU_HEADER+4], 8,
                                                                              &g_stCsacDiagInfo.stParseData[0].Reqcode[2], &uiOutDataLen,
                                                                              hsm_map_csac_key(PA_SHA1), HSM_SHA_160);
                                            }

                                            g_stCsacDiagInfo.stParseData[0].ucLength = uiOutDataLen+2;
                                        }

                                        g_stCsacDiagInfo.ucCsacStep = SECURE_ACCESS_STEP;
                                    }

                                    else
                                    {
                                        g_stCsacDiagInfo.stParseData[1].Reqcode[0] = FAIL;
                                        g_stCsacDiagInfo.stParseData[1].Reqcode[1] = packet->mData[SIZE_PASSTHRU_HEADER+4]; //NRC
                                        g_stCsacDiagInfo.ucCsacStep = SECURE_ACCESS_ERROR;
                                    }
                                    
                                }
                                
                                /********************Save CAN Log********************/
                                uiLogSize = packet->mLen - SIZE_PASSTHRU_HEADER;
                                memcpy(&g_stCsacDiagInfo.stParseData[1].Reqcode[g_stCsacDiagInfo.stParseData[1].ucLength+4], &packet->mData[SIZE_PASSTHRU_HEADER], uiLogSize);
                                g_stCsacDiagInfo.stParseData[1].ucLength += uiLogSize;
                                /****************************************************/
                                
								if( bPendingFlag == true ) g_stCsacDiagInfo.ucCsacPos++;
								else g_stCsacDiagInfo.ucCsacPos=0;
							}
						}
						else	//eDIAG_COMM_RX_FAIL
						{
                            g_stCsacDiagInfo.stParseData[1].Reqcode[0] = FAIL;
                            g_stCsacDiagInfo.stParseData[1].Reqcode[1] = 0x00; //conditions not correct
                            g_stCsacDiagInfo.ucCsacStep = SECURE_ACCESS_ERROR;
						}

                        if( bPendingFlag == true )
                        {
                            MsgDiag_t	*pDiagmsg;
                            PTmsgPkt_t	*pPTpacket;
                            pDiagmsg = ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
                            if( pDiagmsg == NULL )
                            {
                                break;
                            }
                            pPTpacket = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
                            if( pPTpacket == NULL )
                            {
                                osPoolFree( hDiagPool, (void *)pDiagmsg );
                                break;
                            }

							pDiagmsg->mMsgType 		= MSG_DIAG;
                            pDiagmsg->mPktType 		= message->mPktType;
							pDiagmsg->event         = CAN_CSAC;
							pDiagmsg->subEvent 		= eCAN_RX_BLOCK;
							pDiagmsg->unEventTime 	= GetUnixTime();
							pDiagmsg->pPacket 		= (void *)pPTpacket;
							
                            if(osMessageAvailableSpace(hOBDRxMessage) == 0)
                            {
                                osPoolFree( hPTPKPool, (void *)pPTpacket );
                                osPoolFree( hDiagPool, (void *)pDiagmsg );
                            }
                            else
                            {
                                osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg, osWaitForever );
                            }
                        }
                        else
                        {
                            if( g_stCsacDiagInfo.ucCsacStep == SECURE_ACCESS_STEP )
                            {
                                SendMSGToCsacDiag(CSACDIAG_RUNNING, message );
                            }
                            else if( g_stCsacDiagInfo.ucCsacStep == SECURE_ACCESS_END )
                            {
                                SendMSGToCsacDiag(CSACDIAG_END, message );
                            }
                            else
                            {
                                SendMSGToCsacDiag(CSACDIAG_FAIL, message );
                            }
                        }
						osPoolFree( hCommPKPool, (void *)packet );
						osPoolFree( hMsgPool, (void *)message );
                    }
                    break;
                    
				case CSACDIAG_END:
                  {
					GLogN("\r\n[CSACDIAG_END]\r\n");
                    TransmitFunction(message->mPktType, &g_stCsacDiagInfo.stParseData[1].Reqcode[0], 1, g_stCsacDiagInfo.CsacMode);
                    
					osPoolFree( hCommPKPool, (void *)packet );
					osPoolFree( hMsgPool, (void *)message );
					break;
                  }
				case CSACDIAG_FAIL:
                  {
                    //memcpy(&g_stCsacDiagInfo.stParseData[1].Reqcode[4], &g_stCsacDiagInfo.stParseData[1].Reqcode[2], g_stCsacDiagInfo.stParseData[1].ucLength);
                    
                    //U8 temp = 0;
                    memcpy(&g_stCsacDiagInfo.stParseData[1].Reqcode[2], &g_stCsacDiagInfo.stParseData[1].ucLength, 2);
                    //temp = g_stCsacDiagInfo.stParseData[1].Reqcode[2];
                    //g_stCsacDiagInfo.stParseData[1].Reqcode[2] = g_stCsacDiagInfo.stParseData[1].Reqcode[3];
                    //g_stCsacDiagInfo.stParseData[1].Reqcode[3] = temp;
                    
                    TransmitFunction(message->mPktType, &g_stCsacDiagInfo.stParseData[1].Reqcode[0], g_stCsacDiagInfo.stParseData[1].ucLength+4, g_stCsacDiagInfo.CsacMode);
                    
					osPoolFree( hCommPKPool, (void *)packet );
					osPoolFree( hMsgPool, (void *)message );
                  }
					break;
                default:
                  {
					osPoolFree( hCommPKPool, (void *)packet );
					osPoolFree( hMsgPool, (void *)message );
					break;
                  }
            }
		}
        
		osDelay(1);
	}
}

#if defined(CSAC_TEST)
void InitGITSetConfig2()
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

void InitGITHWSetData2()
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
#endif

void SendMSGToCsacDiag(u16 Mode, stMsgClst *message )
{
	stMsgClst	*msg;
	stCommPkt	*pkt;
	
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
    msg->mPktType = message->mPktType;
    msg->mMsgType = message->mMsgType;
	msg->mMod = Mode;
    memcpy(pkt,message->pPacket,sizeof(stCommPkt));
	msg->pPacket	= (void *)pkt;
	if(osMessageAvailableSpace(hCsacDiagMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)pkt );
		osPoolFree( hMsgPool, (void *)msg );
	}
	else
	{
		osMessagePut( hCsacDiagMsg, (uint32_t)msg, osWaitForever );
	}
}
