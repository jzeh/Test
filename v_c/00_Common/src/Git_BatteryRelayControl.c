/*************************************************************
* NOTE : Git_BatteryRelayControl.c
* Author : Kwon JooHyun
* Since : 2023.06.14
**************************************************************/
#ifdef USE_RELAY_MOSA

#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

#include "common.h"
#include "cmsis_os.h"
#include "Sw_timer.h"

#include "Git_BatteryRelayControl.h"
#include "git_PassthruDefines.h"
#include "git_pool.h"
#include "git_ListDiag.h"
#include "Git_vci.h"
#include "Git_record.h"
#include "git_OBDcomm.h"
#include "Git_rtc.h"
#include "Git_can.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
osMessageQId hBatRelayConMsg;
osThreadId hBatRelayConTh;
osThreadId hBatRelayMonitorTh;

extern osMessageQId	hDiagMsg;
extern osPoolId		hDiagPool;
extern osPoolId		hPTPKPool;
extern osPoolId 	hBatRelayConPool;
extern osPoolId 	hBatRelayConPKPool;

uint32_t m_unBatRelayCon_10ms_Timer = 0;
uint32_t m_unBatRelayCon_100ms_Timer = 0;
char g_cAliveCount_0035 = 0;
stCanPacket g_MosaRxCanPacket;

bool g_bBatRelayConFlag = false;
eBatRelayConType g_eBatRelayConType = BatRelayConType_None;

////////////////////////////////////////////////////////////////
// NE EV - Read CAN Data Variable
uint16_t g_usNEBattPackVolt = 0; // 0x0235, 14~15 Bytes
uint8_t g_ucNEPreChrgState = 0;
uint8_t g_ucNEMainRlyOnState = 0;
// OS EV - Read CAN Data Variable
uint16_t g_usOSBattPackVolt = 0;
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// NE EV - Write CAN List
// CAN ID : 0x0035
char g_carrPreNERelayOpenData_0035[32] = {
	0x00, 0x00, 0x00, 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00};
char g_carrNERelayOpenData_0035[32] = {0,};
// CAN ID : 0x010A
char g_carrPreNERelayOpenData_010A[32] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00};
char g_carrNERelayOpenData_010A[32] = {0,};
// CAN ID : 0x0120
char g_carrPreNERelayOpenData_0120[32] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00};
char g_carrNERelayOpenData_0120[32] = {0,};
// CAN ID : 0x02AA
char g_carrPreNERelayOpenData_02AA[32] = {0,};
char g_carrNERelayOpenData_02AA[32] = {0,};
////////////////////////////////////////////////////////////////
// OS EV - Write CAN List
char g_carrOSRelayOpenData_0200[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00};
char g_carrOSRelayOpenData_0291[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char g_carrOSRelayOpenData_0523[8] = {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char g_carrOSRelayOpenData_0524[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char g_carrOSRelayOpenData_0211[8] = {0xA1, 0x00, 0x00, 0x00, 0x00, 0x00 ,0x00, 0x00};

char g_carrOSRelayCloseData_0523[8] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// NE EV -> 0 : 0x0035, 1 : 0x010A, 2 : 0x0120, 3 : 0x02AA
// OS EV -> 0 : 0x0200, 1 : 0x0291, 2 : 0x0523, 3 : 0x0524, 4 : 0x0211
////////////////////////////////////////////////////////////////
stBatRelayConData g_stBatRelayConData_NE[4] =
{
	{g_carrPreNERelayOpenData_0035, g_carrNERelayOpenData_0035, 10, 0, 0x0035, 32},
	{g_carrPreNERelayOpenData_010A, g_carrNERelayOpenData_010A, 10, 0, 0x010A, 32},
	{g_carrPreNERelayOpenData_0120, g_carrNERelayOpenData_0120, 10, 0, 0x0120, 32},
	{g_carrPreNERelayOpenData_02AA, g_carrNERelayOpenData_02AA, 100, 0, 0x02AA, 32}
};

stBatRelayConData g_stBatRelayConData_OS[5] =
{
	{NULL, g_carrOSRelayOpenData_0200, 100, 0, 0x0200, 8},
	{NULL, g_carrOSRelayOpenData_0291, 100, 0, 0x0291, 8},
	{NULL, g_carrOSRelayOpenData_0523, 100, 0, 0x0523, 8},
	{NULL, g_carrOSRelayOpenData_0524, 100, 0, 0x0524, 8},
	{NULL, g_carrOSRelayOpenData_0211, 100, 0, 0x0211, 8}
};

stBatRelayConData g_stBatRelayConData_OS_OFF[1] = 
	{NULL, g_carrOSRelayCloseData_0523, 100, 0, 0x0523, 8};

/*--------------------------------------------------------------------*/


bool StartBatRelayConThread(void)
{
	osMessageQDef( BatRelayConQueue, MESSAGE_BATRELAYCON_QUEUE_SIZE, int );
	osThreadDef( BatRealyConTh, BatRelayConThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );
	osThreadDef( BatRealyMonitorTh, BatRelayMonitorThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );

    hBatRelayConMsg = osMessageCreate( osMessageQ( BatRelayConQueue ), NULL );
    if( hBatRelayConMsg == NULL )
    {
        GLogE( "Error... fail create hBatRelayConMsg Thread!!!\r\n" );
        return false;
    }
    hBatRelayConTh = osThreadCreate( osThread( BatRealyConTh ), NULL );
    if( hBatRelayConTh == NULL )
    {
        GLogE( "Error... fail create hBatRelayConTh Thread!!!\r\n" );
        return false;
    }

	hBatRelayMonitorTh = osThreadCreate( osThread( BatRealyMonitorTh ), NULL );
	if( hBatRelayMonitorTh == NULL )
	{
        GLogE( "Error... fail create hBatRelayMonitorTh Thread!!!\r\n" );
        return false;		
	}
	
    return true;
}


void BatRelayConThread(void const *argument)
{
	for(;;)
	{
		BatRelayConEventListener();

		if( g_bBatRelayConFlag == true )
		{
			if( Get_TmrDelta( Get_Tmr(), m_unBatRelayCon_10ms_Timer ) >= 10 )
			{
				m_unBatRelayCon_10ms_Timer = Get_Tmr();
								
				if( g_eBatRelayConType == BatRelayConType_NEEV )
				{
					BatRelayCon_WriteCanPacket(g_stBatRelayConData_NE, 10, BatRelayConType_NEEV, (sizeof(g_stBatRelayConData_NE)/sizeof(stBatRelayConData)));
				}
			}
			
			if( Get_TmrDelta( Get_Tmr(), m_unBatRelayCon_100ms_Timer ) >= 100 )
			{
				m_unBatRelayCon_100ms_Timer = Get_Tmr();
				
				if( g_eBatRelayConType == BatRelayConType_NEEV )
				{
					BatRelayCon_WriteCanPacket(g_stBatRelayConData_NE, 100, BatRelayConType_NEEV, (sizeof(g_stBatRelayConData_NE)/sizeof(stBatRelayConData)));
				}
				else if( g_eBatRelayConType == BatRelayConType_OSEV )
				{
					BatRelayCon_WriteCanPacket(g_stBatRelayConData_OS, 100, BatRelayConType_OSEV, (sizeof(g_stBatRelayConData_OS)/sizeof(stBatRelayConData)));
				}
			}
		}

		osDelay(1);
	}
}


void BatRelayMonitorThread(void const *argument)
{
	uint16_t usVolt = 0;
	float fVolt = 0.0;
	static uint16_t s_usNeBatPackVolt = 0;
	
	for(;;)
	{
		if( g_bBatRelayConFlag == true )
		{
			memset(&g_MosaRxCanPacket, 0, sizeof(stCanPacket));
			
			if( OemReadCanBuff((U8*)&g_MosaRxCanPacket, MOSA_RX_TIMEOUT) )
			{
				if( g_eBatRelayConType == BatRelayConType_NEEV )
				{
					if( g_MosaRxCanPacket.stFDStdPacket.us11BitID == NE_PRECHRGSTA_CANID )
					{
						//printf("[BatRelayMonitorThread] CAN ID : %x\r\n",g_MosaRxCanPacket.stFDStdPacket.us11BitID);
						memcpy(&g_ucNEPreChrgState, &g_MosaRxCanPacket.stFDStdPacket.arrDataFields[NE_CHRG_STATE_DATA_POSITION], NE_CHRG_STATE_DATA_SIZE);
						//hexdump(&g_ucNEPreChrgState, 1);

						g_ucNEPreChrgState = (g_ucNEPreChrgState>>4)&0x03;

						memcpy(&usVolt, &g_MosaRxCanPacket.stFDStdPacket.arrDataFields[NE_BATPACP_VOLT_DATA_POSITION], NE_BATPACP_VOLT_DATA_SIZE);
						//hexdump((uint8_t*)&g_usNEBattPackVolt, NE_BATPACP_VOLT_DATA_SIZE);

						fVolt = ((float)usVolt*NE_BATPACP_VOLT_FACTOR);
						s_usNeBatPackVolt = (uint16_t)round(fVolt);
					}
					else if( g_MosaRxCanPacket.stFDStdPacket.us11BitID == NE_MAINRLYONSTA_CNAID )
					{
						memcpy(&g_ucNEMainRlyOnState, &g_MosaRxCanPacket.stFDStdPacket.arrDataFields[NE_CHRG_STATE_DATA_POSITION], NE_CHRG_STATE_DATA_SIZE);

						g_ucNEMainRlyOnState = g_ucNEMainRlyOnState&0x0C;
					}

					// Set Baterry Pack Voltage Value
					if( g_ucNEPreChrgState > 0 || g_ucNEMainRlyOnState > 0 )
					{
						if( g_usNEBattPackVolt != s_usNeBatPackVolt )
						{
							g_usNEBattPackVolt = s_usNeBatPackVolt;
						}
					}
					else
					{
						if( g_usNEBattPackVolt != 0 )
						{
							g_usNEBattPackVolt = 0;
						}
					}
				}
				else if( g_eBatRelayConType == BatRelayConType_OSEV )
				{
					if( g_MosaRxCanPacket.stNormalPacket.us11BitID == OS_MONITORING_CANID )
					{
						memcpy(&usVolt, &g_MosaRxCanPacket.stNormalPacket.arrDataFields[OS_BATPACP_VOLT_DATA_POSITION], OS_BATPACP_VOLT_DATA_SIZE);

						if( !(usVolt < 0x000A || usVolt == 0xFFFF) )
						{
							fVolt = ((float)usVolt*OS_BATPACP_VOLT_FACTOR);
							g_usOSBattPackVolt = (uint16_t)round(fVolt);
						}
						else
						{
							g_usOSBattPackVolt = 0;
						}
					}
				}
			}
		}
		else
		{
#if 0
			memset(&g_MosaRxCanPacket, 0, sizeof(stCanPacket));
			g_usNEBattPackVolt = 0;
			g_ucNEPreChrgState = 0;
			g_usOSBattPackVolt = 0;
#endif
		}
		

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#else
        osDelay( 1 );
#endif
	}
}


void BatRelayConEventListener()
{
	osEvent		evt;
	
	stMsgClst	*message;
	stCommPkt	*packet;
	
	evt = osMessageGet(hBatRelayConMsg, 1);
	if( evt.status == osEventMessage )
	{
		message 	= (stMsgClst*)evt.value.p;
		packet		= (stCommPkt*)message->pPacket;

		switch(message->mMod)
		{
			case BatRelayConStatus_Init:
				InitGITSetConfig();
				InitGITHWSetData();

				if( message->mSeq == BatRelayConType_NEEV )
				{
					VCI_HW_Setting(BAT_FD_RELAY_CON);
					g_eBatRelayConType = BatRelayConType_NEEV;
				}
				else if( message->mSeq == BatRelayConType_OSEV )
				{
					VCI_HW_Setting(BAT_RELAY_CON);
					g_eBatRelayConType = BatRelayConType_OSEV;
				}
				
				m_unBatRelayCon_10ms_Timer = Get_Tmr();
				m_unBatRelayCon_100ms_Timer = Get_Tmr();

				SendMSGToBatRelayCon(BatRelayConStatus_Run, message);
				
				break;

			case BatRelayConStatus_Run:	
				g_bBatRelayConFlag = true;	
				SendMSGToBatRelayCon(BatRelayConStatus_Idle, message);

				break;

			case BatRelayConStatus_Idle:
				// (g_bBatRelayConFlag) 1 : Run - CAN Writting & Read Monitoring
				// (g_bBatRelayConFlag) 0 : Waitting
				break;

			case BatRelayConStatus_Stop:
				g_bBatRelayConFlag = false;
				SendMSGToBatRelayCon(BatRelayConStatus_Exit, message);

				break;
				
			case BatRelayConStatus_Exit:
				memset(&g_MosaRxCanPacket, 0, sizeof(stCanPacket));
				g_usNEBattPackVolt = 0;
				g_ucNEPreChrgState = 0;
				g_usOSBattPackVolt = 0;

				if( g_eBatRelayConType == BatRelayConType_OSEV )
				{
					OSEV_SendRelayStop();
				}
				
				SendMSGToBatRelayCon(BatRelayConStatus_Idle, message);

				break;

			default:
				break;
		}
		
		osPoolFree( hBatRelayConPKPool, (void *)packet );
		osPoolFree( hBatRelayConPool, (void *)message );
	}
}


void SendMSGToBatRelayCon(u16 Mode, stMsgClst *message)
{
	stMsgClst	*msg;
	stCommPkt	*pkt;
	
    msg	= ( stMsgClst* )osPoolCAlloc( hBatRelayConPool );
    if( msg == NULL )
    {
        return;
    }
    pkt = ( stCommPkt* )osPoolCAlloc( hBatRelayConPKPool );
    if( pkt == NULL )
    {
        osPoolFree( hBatRelayConPool, (void *)msg );
        return;
    }
    msg->mPktType = message->mPktType;
    msg->mMsgType = message->mMsgType;
	msg->mMod = Mode;
    memcpy(pkt,message->pPacket,sizeof(stCommPkt));
	msg->pPacket	= (void *)pkt;
	if( osMessageAvailableSpace(hBatRelayConMsg) == 0 )
	{
		osPoolFree( hBatRelayConPKPool, (void *)pkt );
		osPoolFree( hBatRelayConPool, (void *)msg );
	}
	else
	{
		osMessagePut( hBatRelayConMsg, (uint32_t)msg, osWaitForever );
		//GLogN("SendMSGToListDiag\r\n");
	}
}


unsigned short CalculateCRC16(char* pcData, unsigned int unLen)
{
    unsigned short usCrc = CRC16_INIT_VALUE;

    for(int i=0; i<unLen; i++)
    {   
        usCrc = usCrc ^ ((*pcData) << 8); 
        pcData++;

        for(int j=0; j<8; j++)
        {
            if(usCrc & 0x8000)
            {
                usCrc = (usCrc<<1) ^ CRC16_POLY;
            }
            else
            {
                usCrc = usCrc<<1;
            }
        }
    }   

    return usCrc;
}


void MakeNeEvTxPacket(char* pcData, char* pcPreData, unsigned short usCanId, char cCnt)
{
	char cCheckData[32] = {0,};
	char cFinalData[32] = {0,};
	unsigned short usCalData = 0;
	unsigned short usCRC16Data = 0;

	memcpy(cCheckData, &pcPreData[2], 30);
	memcpy(&cCheckData[0], &cCnt, 1);

	usCalData = usCanId + CRC16_ADD_VALUE;
	memcpy(&cCheckData[30], &usCalData, 2);

	usCRC16Data = CalculateCRC16(cCheckData, 32);

	memcpy(&cFinalData[0], &usCRC16Data, 2);
	memcpy(&cFinalData[2], cCheckData, 30);

	if( usCanId == 0x010A || usCanId == 0x0120 )
	{
		memcpy(&cFinalData[16], &g_usNEBattPackVolt, 2);
	}

	memcpy(pcData, cFinalData, sizeof(cCheckData));
}


void MakeOsEvTxPacket(char* pcData, unsigned short usCanId)
{
	if( usCanId == 0x0524 )
	{
		memcpy(&pcData[0], &g_usOSBattPackVolt, 2);
	}
}


void BatRelayCon_WriteCanPacket(stBatRelayConData* pstData, uint16_t unTime, eBatRelayConType eType, uint16_t unCount)
{
	unsigned short usCanId = 0, usSwapCanId = 0;

	MsgDiag_t	*msgDiag[5];
	PTmsgPkt_t	*pktDiag[5];

	for(int i=0; i<unCount; i++)
	{
		if( pstData[i].unCycle == unTime )
		{
			msgDiag[i] = ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
			if( msgDiag[i] == NULL )
			{
				return;
			}
			pktDiag[i] = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
			if( pktDiag[i] == NULL )
			{
				osPoolFree( hDiagPool, (void *)msgDiag[i] );
				return;
			}
			
			if( eType == BatRelayConType_NEEV )
			{
				memset(pstData[i].pcData, 0, sizeof(pstData[i].pcData));
				MakeNeEvTxPacket(pstData[i].pcData, pstData[i].pcPreData, pstData[i].usCanId, pstData[i].cAliveCnt);
			}
			else if( eType == BatRelayConType_OSEV )
			{
				MakeOsEvTxPacket(pstData[i].pcData, pstData[i].usCanId);
			}
			//printf("CAN TIME : %d, CAN ID : %X\n",unTime,pstData[i].usCanId);
			//hexdump(pstData[i].pcData, pstData[i].usCanLen);

			memcpy(&usCanId, &pstData[i].usCanId, MOSA_RX_CANID_SIZE);
			usSwapCanId = ByteSwap(usCanId);
			 
			memcpy(&pktDiag[i]->pData[0], &usSwapCanId, sizeof(unsigned short));
			memcpy(&pktDiag[i]->pData[2], &pstData[i].usCanLen, MOSA_RX_CANLENTH_SIZE);
			memcpy(&pktDiag[i]->pData[3], pstData[i].pcData, pstData[i].usCanLen);
			//hexdump(pktDiag[i]->pData, pstData[i].usCanLen+3);

			pktDiag[i]->DataSize = pstData[i].usCanLen;
			
			msgDiag[i]->mMsgType 		= MSG_DIAG;
            msgDiag[i]->mPktType 		= PACKET_CAN;
			msgDiag[i]->event 			= DIAG_PASSTHRU;
			msgDiag[i]->subEvent 		= eDIAG_COMM_TX_START;
			msgDiag[i]->unEventTime 	= GetUnixTime();
			msgDiag[i]->pPacket 		= (void *)pktDiag[i];

			if( osMessageAvailableSpace(hDiagMsg) == 0 )
			{
				osPoolFree( hPTPKPool, (void *)pktDiag[i] );
				osPoolFree( hDiagPool, (void *)msgDiag[i] );
			}
			else
			{
				osMessagePut( hDiagMsg, (uint32_t)msgDiag[i], osWaitForever );
			}

			if( eType == BatRelayConType_NEEV )
			{			
				pstData[i].cAliveCnt++;
				memcpy(pstData[i].pcPreData, pstData[i].pcData, pstData[i].usCanLen);
			}
		}

		osDelay(1);
	}

}


void OSEV_SendRelayStop()
{
	BatRelayCon_WriteCanPacket(g_stBatRelayConData_OS_OFF, 100, BatRelayConType_OSEV, (sizeof(g_stBatRelayConData_OS_OFF)/sizeof(stBatRelayConData)));
}
	

unsigned short ByteSwap(unsigned short usData)
{
	return ((usData >> 8) | (usData << 8));
}
 


#endif // #ifdef USE_RELAY_MOSA