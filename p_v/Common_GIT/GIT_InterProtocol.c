/**
  ******************************************************************************
  * @file    GIT_InterProtocol.c
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    19-FEB-2014
  * @brief   Manager GIT_InterProtocol.c module
  ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include "stdlib.h"
#include "string.h"
#include "GIT_InterProtocol.h"
#include "GIT_OemInterface.h"
#include "GIT_CanParsingProc.h"
#include "GIT_Util.h"
#include "OBD_Manager.h"
#include "GIT_BluetoothLowEnergy.h"
#include "Share_InterFunction.h"
//#include "ExtendBoard_Manager.h"
#include "modem_comm.h"
#include "MngSystem.h"
#include "Autolink_Manager.h"
#include "AutolinkConfiguration.h"
#include "HalHandler.h"

/* Defines  ------------------------------------------------------------------*/
//#define DEBUG_GIT_PTCL_LOG
//#define DEBUG_GIT_PTCL_PACKET_PRINT
//#define DCS_DEMO

extern BR_SystemInfo BkSram_SystemInfo;

#define MAX_QUEUE_BUFF_SIZE_FOR_MODEM		(2048 + 64)
#define MAX_TEMP_BUFF_SIZE_FOR_MODEM		(2048 + 64)

#define MAX_TEMP_BUFF_SIZE						512
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
#define MAX_USB_BUFF_SIZE						512
#endif
#define MAX_CAN_QUEUE_BUFF_SIZE					512
#define MAX_SELFTEST_BUFF_SIZE					1024
#define BT_LOCK_GET_STATE() 					(g_eBTLockStatus)
/* Global Variables ----------------------------------------------------------*/

//stGIT_PTCL_PAYLOAD 	g_stPlusInPtcl;		//plus board
stGIT_PTCL_PAYLOAD 	g_stSelftestInPtcl;
stAMT_PTCL_PAYLOAD 	g_stModemInPtcl;	// modem
stBT_PTCL_PAYLOAD 	g_stBTInPtcl;			// Bluetooth
stBT_PTCL_PAYLOAD 	g_stGPSInPtcl;			// exGPS
#if defined(FEATURE_USE_USB_DRIVE)
stGIT_PTCL_PAYLOAD 	g_stUsbInPtcl;			// USB
#endif

//extern stSupportData g_ucSupportedData;
//extern stSupportData g_ucSupportedOldData[5];
extern stGIT_INTER_FUNCTIONS *g_tbGITInterPtclFunctions;
extern stBT_INTER_FUNCTIONS  *g_tbBTInterPtclFunctions;

unsigned char				g_ucUsbLastSeqNum = 0;
unsigned char* 				g_arrOutputGITPtclBuff;

//U8 g_ucCertificationFlag;	//사용자 인증여부 BTLOCK와는 다른게 마스터나 ADMIN배송기사등은 사용하지 않아야하는 Flag

//unsigned char				g_ucPlusLastSeqNum = 0;
unsigned char				g_ucSelftestLastSeqNum = 0;
unsigned int 				g_uiMutiFrameCopyIndex = 0;

eCommType					g_InputCommType = eCOMM_TYPE_UART_MODEM;

unsigned short 				g_usLastSentGITFrameLen;
eCommType 					g_eLastSentCommType;
int							g_iTimerVSS1secCallback = -1;
extern volatile boolean_t m_bUsedStorage;
extern eLockState	g_eLockStatus;



stGIT_COMM_INFO g_stGitCommInfo[eCOMM_TYPE_MAX] =
{
// COMM TYPE        			fnRecvData,       			fnSendData,       			fnParsing ,      		eCommState ,        	pstInQueue,  		uiInMaxQueueSize,    						fnCheckReadBuff
/* eCOMM_TYPE_UART_MODEM*/  	{OemReadUartModemBuff, 		OemWriteUartModemBuff, 		OemParsingModemUart, 	eCOMM_STATE_CONNECTED, 	NULL, 				MAX_QUEUE_BUFF_SIZE_FOR_MODEM,  			NULL},
/* eCOMM_TYPE_CAN1*/   			{OemReadCanBuff,			OemWriteCanBuff, 			OemParsingCan, 			eCOMM_STATE_CONNECTED, 	NULL, 				MAX_CAN_BUFF_SIZE,  						NULL},
/* eCOMM_TYPE_CAN2*/   			{OemReadCanBuff, 			OemWriteCanBuff, 			OemParsingCan, 			eCOMM_STATE_CONNECTED, 	NULL, 				MAX_CAN_BUFF_SIZE,  						NULL},
/* eCOMM_TYPE_UART_BT*/ 		{OemReadUartBTBuff, 		OemWriteUartBTBuff, 		OemParsingBTUart, 		eCOMM_STATE_CONNECTED,	NULL, 				MAX_TEMP_BUFF_SIZE,  						NULL},
//#if defined(USE_UBLOX_GPS)
/* eCOMM_TYPE_UART_GPS*/  		{OemReadUartGPSBuff, 		OemWriteUartGPSBuff, 		OemParsingGPSUart, 		eCOMM_STATE_CONNECTED, 	NULL, 				MAX_GPS_BUFF_SIZE,  						NULL},
//#endif
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
/* eCOMM_TYPE_USB*/  			{OemReadUsbBuff, 			OemWriteUsbBuff, 			OemParsingUsb, 			eCOMM_STATE_INIT, 		NULL, 				MAX_USB_BUFF_SIZE,							OemCheckUsbReadBuff},
#endif
/* eCOMM_TYPE_UART_SELFTEST*/	{OemReadUartSelftestBuff, 	OemWriteUartSelftestBuff, 	OemParsingSelftestUart, eCOMM_STATE_CONNECTED, 	NULL, 				MAX_SELFTEST_BUFF_SIZE,  					NULL},
};

void ProcessBTInterProtocol(stBT_PTCL_PAYLOAD *pInterPtcl, eCommType eInCommType);

//#define CHECK_MALLOC

/* Function Define  ----------------------------------------------------------*/
void InitDCSProtocol(void)
{
	int i, nQueueSize;

	for ( i=0; i<eCOMM_TYPE_MAX; i++ ) {
		if ( g_stGitCommInfo[i].uiInMaxQueueSize > 0 ) {
			nQueueSize = g_stGitCommInfo[i].uiInMaxQueueSize;
			g_stGitCommInfo[i].pstInQueue = CreateQueue(nQueueSize);

			if ( g_stGitCommInfo[i].pstInQueue == NULL )
				GITDebugPrintf("[%s] Create InQueue Fail !!!!!!!!\r\n", __FUNCTION__);
		}
	}
#ifdef CHECK_MALLOC
	printf("InitDCSProtocol");
	__iar_dlmalloc_stats();
#endif
	memset(g_stModemInPtcl.pPayload ,NULL, MAX_MODEM_PROTO_DATA_LENGTH);
	//g_stPlusInPtcl.pPayload 		= malloc(MAX_TEMP_BUFF_SIZE);
	g_stSelftestInPtcl.pPayload 		= malloc(MAX_TEMP_BUFF_SIZE);
	g_stBTInPtcl.pPayload 			= malloc(MAX_TEMP_BUFF_SIZE);
	g_stGPSInPtcl.pPayload  		= malloc(MAX_GPS_BUFF_SIZE);
#if defined(FEATURE_USE_USB_DRIVE)
	g_stUsbInPtcl.pPayload			= malloc(MAX_MODEM_PROTO_DATA_LENGTH);
#endif

	g_arrOutputGITPtclBuff 			= malloc(MAX_MODEM_PROTO_DATA_LENGTH);
#ifdef CHECK_MALLOC
	printf("InitDCSProtocol2222");
	__iar_dlmalloc_stats();
#endif
#if defined(FEATURE_USE_USB_DRIVE)
	if ( (g_stUsbInPtcl.pPayload == NULL)  || (g_stSelftestInPtcl.pPayload == NULL)/*|| (g_stPlusInPtcl.pPayload == NULL)*/ || (g_stModemInPtcl.pPayload == NULL)|| (g_stBTInPtcl.pPayload == NULL)|| (g_stGPSInPtcl.pPayload == NULL)|| (g_arrOutputGITPtclBuff == NULL) ) {
#else
	if ( (g_stSelftestInPtcl.pPayload == NULL)/*|| (g_stPlusInPtcl.pPayload == NULL)*/ || (g_stModemInPtcl.pPayload == NULL)|| (g_stBTInPtcl.pPayload == NULL)|| (g_stGPSInPtcl.pPayload == NULL)|| (g_arrOutputGITPtclBuff == NULL) ) {
#endif
		printf("\n\n");
		printf("[%s] InPtcl or OutPtcl malloc Fail !!!!!!!!\n\n", __FUNCTION__);

		while(1);
	}

	CAN_InitVariable();

//	memset(&g_ucSupportedData, 0xFF, sizeof(g_ucSupportedData));
//	memset(g_ucSupportedOldData, 0xFF, sizeof(g_ucSupportedOldData));
}

void DeInitDCSProtocol()
{
	int i;

	for ( i=0; i<eCOMM_TYPE_MAX; i++ ) {
		DestoryQueue(g_stGitCommInfo[i].pstInQueue);
	}
}

void CommuicationManager(void)
{
	eCommType i;
	unsigned int uiForLoop=0;
	static unsigned char ucTempBuff[MAX_CAN_BUFF_SIZE]={0,};
	unsigned int uiRecvLen;
    unsigned int unMaxBuffSize;

	// 각 통신에서 들어오는 데이터를 버퍼링한다.

	if(g_bHYPERTECSelftestFlag == false)	uiForLoop = eCOMM_TYPE_MAX;
	else 									uiForLoop = eCOMM_TYPE_MAX-1;

	//for(i = eCOMM_TYPE_UART_MODEM; i < eCOMM_TYPE_MAX; i++) {
	for(i = eCOMM_TYPE_UART_MODEM; i < uiForLoop; i++) {
		uiRecvLen = 0;
        unMaxBuffSize = g_stGitCommInfo[i].uiInMaxQueueSize;

		if ( (g_stGitCommInfo[i].eCommSate == eCOMM_STATE_CONNECTED) && (g_stGitCommInfo[i].fnRecvData != NULL )) {
        uiRecvLen = g_stGitCommInfo[i].fnRecvData(ucTempBuff, unMaxBuffSize, NULL, (eCommType)i);
            if ( uiRecvLen ) {
                PushMultiDataQueue((stQueue*)g_stGitCommInfo[i].pstInQueue, ucTempBuff, uiRecvLen, i);
            }
        }

		if ( g_stGitCommInfo[i].pstInQueue != NULL && g_stGitCommInfo[i].fnParsing != NULL ) {
			g_stGitCommInfo[i].fnParsing(g_stGitCommInfo[i].pstInQueue, GetQueueDataLength(((stQueue*)g_stGitCommInfo[i].pstInQueue)), NULL, (eCommType)i);
		}
	}

	if( (i == eCOMM_TYPE_CAN1) || (i == eCOMM_TYPE_CAN2) )
	{
		Can_Parsing_Manager(eCAN_PARSING_TYPE_ALL);
		Can_Parsing_Manager(eCAN_PARSING_TYPE_ALL);
	}
	return;
}

void ParsingCommandModeProtocol(unsigned char* pBuff, unsigned int nBuffCount, eCommType eWhatCommType)
{
	unsigned int unRecvReturn = 0;
	unsigned char arrTmp[BLUETOOTH_SEND_PACKET_SIZE];

	stQueue *pQueue = (stQueue*)pBuff;
	pQueue = (stQueue*)pBuff;

	if ( IsEmptyQueue(pQueue) )
		return;

	unRecvReturn = BTRecvGetLine(pQueue);

	if ( unRecvReturn == 1 )
	{
		if ( BTRecvParsing( (BYTE*)pQueue, BLUETOOTH_SEND_PACKET_SIZE) )
		{
			//PopMultiDataQueue(pQueue, arrTmp, BLUETOOTH_SEND_PACKET_SIZE);
		}
		else {
			GITDebugPrintf("\r\n Not Found BLE Command Response\r\n");
		}
	}
	else if( unRecvReturn == -1 )		// BLUETOOTH_SEND_PACKET_SIZE가 넘었으나 0x0D와 0x0A를 찾지 못함. 따라서 제거함.
	{
		if ( BTGetState() == eBT_NONE) {
			BTSetState(eBT_initialized);
		}

		GITDebugPrintf("\r\n\r\n Not %s !!! : %d\r\n\r\n", __FUNCTION__, unRecvReturn);

		//PopMultiDataQueue(pQueue, arrTmp, BLUETOOTH_SEND_PACKET_SIZE);
		PopQueue(pQueue, arrTmp);

//		if(GetOBDState() == eOBD_NONE) {
//			SetOBDState(eOBD_Initialize);
//		}
	}
	else if ( unRecvReturn == 0 ) {
	}
	else {
		//GITDebugPrintf( "No receive Packet\r\n");
	}

	return;
}

void ParsingGemaltoProtocol(unsigned char* pBuff, unsigned int nBuffCount, eCommType eWhatCommType)
{
    static unsigned long ulTimeStamp = 0;
	unsigned int unPackLength;
	unsigned int unRecvReturn;
	unsigned char arrTempBuff[MAX_TEMP_BUFF_SIZE_FOR_MODEM];

	unPackLength = 0;
	stQueue *pQueue = (stQueue*)pBuff;
	pQueue = (stQueue*)pBuff;

	if(IsEmptyQueue(pQueue)) {
		return ;
	}

    if( ulTimeStamp == 0 )
    {
        ulTimeStamp  = Get_Tmr();
    }

	unRecvReturn = ModemRecvGetLine(pQueue, &unPackLength);
    
	if(unRecvReturn == 1) {
		PopMultiDataQueue(pQueue, arrTempBuff, unPackLength);

		Modem_ResponsePrc( arrTempBuff, unPackLength);
	}
}

BOOL FindDCSPacket(unsigned char* pBuff)
{
	stQueue *pQueue;
	unsigned char ucTmp, ucPacketChecksum, ucCalcChecksum;
	unsigned int uiCheckSumIdx, uiLengthIdx1, uiLengthIdx2;
	unsigned short usPacketLen;
	unsigned int uiReceivedPacket;

	pQueue = (stQueue*)pBuff;
	uiReceivedPacket = GetQueueDataLength(pQueue);

	if ( IsEmptyQueue(pQueue) || (uiReceivedPacket < DCSPACKET_PAYLOAD_HEADER_SIZE)  )
	{
		//GITDebugPrintf(".");
		return FALSE;
	}

	if ( (pQueue->pData[pQueue->uiFront] == DCSPACKET_SERIAL_SOF) &&
		 (pQueue->pData[(pQueue->uiFront+DCSPACKET_RESERVE1_IDX)% pQueue->uiQueueSize] == DCSPACKET_RESERVE1) &&
		 (pQueue->pData[(pQueue->uiFront+DCSPACKET_RESERVE2_IDX)% pQueue->uiQueueSize] == DCSPACKET_RESERVE2) )
	{
		uiLengthIdx1 = (pQueue->uiFront+DCSPACKET_PAYLOAD_LEN0_IDX) % pQueue->uiQueueSize;
		uiLengthIdx2 = (pQueue->uiFront+DCSPACKET_PAYLOAD_LEN1_IDX) % pQueue->uiQueueSize;

		// usPacketLen ==> sof & eof & cs 제외
		usPacketLen = (unsigned short)((pQueue->pData[uiLengthIdx1] & 0x00FF) + (pQueue->pData[uiLengthIdx2]<<8 & 0xFF00));

        if ( usPacketLen >= pQueue->uiQueueSize  )
        {
            ClearQueue(pQueue);
            printf("[%s] packet length is over, usPacketLen %d !!!!!!!!!!~~~~~~~~~~~!!!!!!!!!!!!\r\n", __FUNCTION__, usPacketLen);
            return FALSE;
        }

		if ( usPacketLen+2/*SOF+Checksum*/ <= GetQueueDataLength(pQueue) )
		{
			//GITDebugPrintf("usPacketLen %d(%d,%d)\r\n", usPacketLen, uiLengthIdx1, uiLengthIdx2);

			uiCheckSumIdx = (pQueue->uiFront+usPacketLen+ 1) % pQueue->uiQueueSize;
			ucPacketChecksum = pQueue->pData[uiCheckSumIdx];			//cs

			// Calculate Checksum
			ucCalcChecksum = CalcChecksumBTPtclFromQueue((unsigned char*)pQueue, usPacketLen + DCSPACKET_SOF_SIZE);

			//GITDebugPrintf("(%02X,%02X)", ucPacketChecksum, ucCalcChecksum);

			if ( ucPacketChecksum == ucCalcChecksum )
			{
				return TRUE;
			}
			else
			{
				int i;
				if ( uiCheckSumIdx == ((pQueue->uiRear-1)% pQueue->uiQueueSize) )
				{
					GITDebugPrintf("Error Packet : Length %d, front %d, rear %d\r\n", usPacketLen+2, pQueue->uiFront, pQueue->uiRear);
					GITDebugPrintf("FindDCSPacket Packet Checksum 0x%02X, Calc Checksum 0x%X, SO Remove Queue.\r\n", ucPacketChecksum, ucCalcChecksum);
					// remove packet
					for ( i=0; i<usPacketLen+6/* SOF & R0 & R1 & L0 &L1 & CHECKSUM*/; i++ )
					{
						if ( PopQueue(pQueue, &ucTmp) == FALSE )
						{
	#if defined(DEBUG_GIT_PTCL_LOG)
							GITDebugPrintf("PopQueue -> break, Index %d\r\n", i);
	#endif
							break;
						}
					}
				}
				else
				{
	#if 0 //defined(DEBUG_GIT_PTCL_LOG)
					GITDebugPrintf("Packet : Length %d, front %d, rear %d, uiEOFIdx %d\r\n", usPacketLen+2, pQueue->uiFront, pQueue->uiRear, uiEOFIdx);
					GITDebugPrintf("isn't received Checksum Packet Calc Checksum 0x%X, SO RETURN FALSE.\r\n", ucCalcChecksum);

					for ( i=0; i< usPacketLen+6; i++ )
						GITDebugPrintf("%02X ", pQueue->pData[(pQueue->uiFront+i)%pQueue->uiQueueSize]);
					GITDebugPrintf("\r\n\r\n");
	#endif
					return FALSE;
				}
			}
		}
	}
	else
	{
		// remove garbage data : ex) 0x08 - USB ACK Data
		PopQueue(pQueue, &ucTmp);
//		GITDebugPrintf("<-- Remove 0x%02X, Front %d, Rear %d\r\n", ucTmp, pQueue->uiFront, pQueue->uiRear);
	}

	return FALSE;
}

void ParsingBypassModeProtocol(unsigned char* pBuff, unsigned int nBuffCount, eCommType eWhatCommType)
{
	static stBT_PTCL_PAYLOAD *pInterPtcl;

	// 1. queue에서 GIT 프로토콜을 찾는다.
	// 2. 찾아지면, 체크섬 검사하고
	// 3. SEQUENCE 체크 -> 예외처리
	// 4. 패킷 복사 후 function ID에 맞는 동작을 한다.

	if ( FindDCSPacket(pBuff) )
	{
		// copy to global InterProtocol structure variable
		g_InputCommType = eWhatCommType;
		{
			pInterPtcl = (stBT_PTCL_PAYLOAD*)&g_stBTInPtcl;
		}
		CopyFromQueueToDCSPayload(pBuff, (unsigned char*)pInterPtcl);

		ProcessBTInterProtocol(pInterPtcl, eWhatCommType);
	}
}

void ParsingDCSProtocol(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType)
{
	//printf("~! %d ",BTGetConnectStatus() );

	if ( BTGetConnectStatus() ) {
		ParsingBypassModeProtocol(pBuff, nCount, eWhatCommType);
	}
	else {	//BT끊긴상태
		ParsingCommandModeProtocol(pBuff, nCount, eWhatCommType);
		LOCK_SET_STATE(eLOCK_STATE_LOCK);
	}
}

#ifdef BNCOM //mod.pdh 2021.10.27
void ParsingBnComProtocol(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType)
{
	unsigned int unBleCmdLength;
    unsigned char arrTempBuff[1048]={0,};
    stQueue *pQueue = (stQueue*)pBuff;

	if ( IsEmptyQueue(pQueue) ) return;

    if ( eWhatCommType == eCOMM_TYPE_UART_BT )
    {
		if ( BTGetConnectStatus())
    	{
    		ParsingBypassModeProtocol(pBuff, nCount, eWhatCommType);
    	}
    	else
    	{
            //hexdump(arrTempBuff,unPackLength);
			if ( FindBtBnComPacket(pBuff, arrTempBuff, &unBleCmdLength) )
				BTRecvParsing(arrTempBuff, unBleCmdLength); //mod.pdh 2021.10.27
    	}
//		hexdump(arrTempBuff,uiReceivedPacket);
    }
}
#endif

BOOL FindGITPacket(unsigned char* pBuff)
{
	stQueue *pQueue;
	unsigned char ucTmp, ucPacketChecksum, ucCalcChecksum;
	unsigned int uiSOFIdx, uiEOFIdx, uiCheckSumIdx, uiLengthIdx1, uiLengthIdx2;
	unsigned short usPacketLen;
	unsigned int uiReceivedPacket;
	static unsigned int	s_uiOldPacketSize=0;
	static unsigned int uOldTimer 	= 0;
	int i=0;

	pQueue = (stQueue*)pBuff;
	uiReceivedPacket = GetQueueDataLength(pQueue);

	if ( IsEmptyQueue(pQueue) || (uiReceivedPacket < MIN_INTER_PROTO_DATA_LENGTH)  )
	{
		return FALSE;
	}
    uiSOFIdx = pQueue->uiFront;
    uiLengthIdx1 = (uiSOFIdx+1) % pQueue->uiQueueSize;
    uiLengthIdx2 = (uiSOFIdx+2) % pQueue->uiQueueSize;
    usPacketLen = (unsigned short)((pQueue->pData[uiLengthIdx1] & 0x00FF) + (pQueue->pData[uiLengthIdx2]<<8 & 0xFF00));
    uiEOFIdx = (uiSOFIdx+usPacketLen+1) % pQueue->uiQueueSize;
	uiCheckSumIdx = (uiEOFIdx+1)% pQueue->uiQueueSize;
    
    if( (pQueue->pData[uiSOFIdx] == GITPACKET_SERIAL_SOF)  ) 
    {
		if(usPacketLen+3/* SOF & EOF & CHECKSUM*/ <= uiReceivedPacket && (pQueue->pData[uiEOFIdx] == GITPACKET_SERIAL_EOF)) 
        {
			ucPacketChecksum = pQueue->pData[uiCheckSumIdx];
			// Calculate Checksum
			ucCalcChecksum = CalcChecksumGITPtclFromQueue((unsigned char*)pQueue, usPacketLen);

			if ( ucPacketChecksum == ucCalcChecksum ) 
            {
			  	s_uiOldPacketSize=0;
				return TRUE;
			}
			else 
            {
				GITDebugPrintf("Error Packet : Length %d, front %d, rear %d, uiEOFIdx %d\r\n", usPacketLen, pQueue->uiFront, pQueue->uiRear, uiEOFIdx);
				GITDebugPrintf("FindGITPacket Packet Checksum 0x%02X, Calc Checksum 0x%X, SO Remove Queue.\r\n", ucPacketChecksum, ucCalcChecksum);
				// remove packet
				for ( i=0; i<usPacketLen+3/* SOF & EOF & CHECKSUM*/; i++ )
				{
					if ( PopQueue(pQueue, &ucTmp) == FALSE )
					{
#if defined(DEBUG_GIT_PTCL_LOG)
						GITDebugPrintf("PopQueue -> break, Index %d\r\n", i);
#endif
						break;
					}
#if defined(DEBUG_GIT_PTCL_LOG)
                    GITDebugPrintf("[%02X]\r\n", ucTmp);
#endif
				}
				return FALSE;
			}
		}
		else		//SOF는 들어왔는데 EOF가 안들어오는 경우..... 어떻하나
		{
			// 정말로 EOF가 안들어 온다. 			2017-01-16 오후 8:42:30
			// 못받는 것인지, 안주는 것인지... 확인해 보니 안주는 것은 아닌 듯 하다.	2017-01-16 오후 8:47:58
			// DMA를 사용할 경우 데이터 전부를 다 받는다. 따라서 아래 queue를 읽어서 버리는 루틴은 필요가 없다.
			if(s_uiOldPacketSize != uiReceivedPacket)
			{
			  	s_uiOldPacketSize=uiReceivedPacket;
				uOldTimer = Get_Tmr();
			}
			else 
            {
				if (Get_TmrDelta(Get_Tmr(),uOldTimer) >= 2000)
				{
					printf("Error Packet : Length %d, front %d, rear %d, uiEOFIdx %d\r\n", usPacketLen+3, pQueue->uiFront, pQueue->uiRear, uiEOFIdx);
					printf("else Packet Checksum 0x%02X, Calc Checksum 0x%X, SO Remove Queue.\r\n", ucPacketChecksum, ucCalcChecksum);
					// remove packet
					s_uiOldPacketSize = 0;
					//for ( i=0; i<uiReceivedPacket; i++ )		//1byte씩 지우자 한꺼번에 지우지말고.. 160405
					{
						if ( PopQueue(pQueue, &ucTmp) == FALSE )
						{
							//break;
						}
//						printf(" %02X ",ucTmp);
					}
				}
			}
//			printf("<-- Not Found GIT protocol EOF - 0x%02X, Front %d, Rear %d\r\n", ucTmp, pQueue->uiFront, pQueue->uiRear);
		}
	}
	else
	{
	  	s_uiOldPacketSize = 0;

//        for (int j=0; j< uiReceivedPacket; j++ )
//            GITDebugPrintf("[%02X, %d]\r\n", pQueue->pData[(pQueue->uiFront+j)% pQueue->uiQueueSize], (pQueue->uiFront+j)% pQueue->uiQueueSize);
		PopQueue(pQueue, &ucTmp);
		if ( ucTmp == 0xF8 ) // retransmit last USB Data
		{
//			printf("<-- recv 0xF8 so retranmit last frame~~~~~~~~~\r\n");
			SendGITPtclFrame((char*)g_arrOutputGITPtclBuff, g_usLastSentGITFrameLen, g_eLastSentCommType, NULL, 0);
		}
		printf("<-- Remove 0x%02X, Front %d, Rear %d, receveDataSize %d\r\n", ucTmp, pQueue->uiFront, pQueue->uiRear, uiReceivedPacket);
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////
// Woong Bae 18/08/30
/*
    RFID Packet 구조
    02   30   42   30   30   35   46   41   32   44   42   03
   SOF                                                                  EOF
   FINDRFIDPacket 함수에서 큐의 시작 주소에
   RFID_08C_SECONDBYTE_OF_PACKET(1)을 더하는 이유는
   SOF가 들어오기 때문
*/
////////////////////////////////////////////////////////////////
bool FindRFIDPacket(unsigned char* pBuff)
{
	stQueue *pQueue;
	unsigned int uiDataSOFIndex=0;
	unsigned int uiDataCompareIndex=0,uiDataEOFIndex=0;
	unsigned int uiReceivedPacket=0;
	//unsigned char ucIdSize=0,ucId[RFID_08C_CID_LENGTH]={0,};
	int i=0,j=0;
	bool bRFIDUIDFindSuccess = false;

	pQueue = (stQueue*)pBuff;
	uiReceivedPacket = GetQueueDataLength(pQueue);
#if false
	ucIdSize = sizeof(RFID_IDENTIFIER)-1;
	memcpy(&ucId,RFID_IDENTIFIER,sizeof(RFID_IDENTIFIER)-1);
#endif

	if ( IsEmptyQueue(pQueue) )	return false;

	if( uiReceivedPacket >= RFID_08C_PACKET_LENGTH )
	{
		uiDataSOFIndex = (pQueue->uiFront) % (pQueue->uiQueueSize);
		uiDataEOFIndex = (pQueue->uiFront+RFID_08C_PACKET_LENGTH-1) % (pQueue->uiQueueSize);

		if( pQueue->pData[uiDataSOFIndex] == RFID_08C_SOF && pQueue->pData[uiDataEOFIndex] == RFID_08C_EOF )
		{
#if false
			for( j=0; j<ucIdSize; j++ )
			{
				uiDataCompareIndex = (pQueue->uiFront+RFID_08C_DATA_POSITION+j) % (pQueue->uiQueueSize);
				if( pQueue->pData[uiDataCompareIndex] == ucId[j] )
				{
					bRFIDUIDFindSuccess = true;
				}
				else
				{
					bRFIDUIDFindSuccess = false;
				}
			}
#endif

            printf("\nRFID ID : ");
            uiDataCompareIndex = (pQueue->uiFront) % (pQueue->uiQueueSize);
            hexdump((char *)pQueue->pData[uiDataCompareIndex],RFID_08C_DATA_LENGTH+2);
#if false
            for( j=0; j<RFID_08C_DATA_LENGTH+2; j++ ) //SOF,EOF 제외
            {
                uiDataCompareIndex = (pQueue->uiFront+j) % (pQueue->uiQueueSize);
                printf("%x(%c)",pQueue->pData[uiDataCompareIndex],pQueue->pData[uiDataCompareIndex]);
            }
            printf("\n");
#endif
            memset(&BkSram_SystemInfo.ucarrRFIDUID[0], NULL,RFID_08C_DATA_LENGTH);
			for( j=0; j<RFID_08C_DATA_LENGTH; j++ )	//SOF,EOF 제외
			{
				uiDataCompareIndex = (pQueue->uiFront+RFID_08C_DATA_POSITION+j) % (pQueue->uiQueueSize);
				BkSram_SystemInfo.ucarrRFIDUID[j] = pQueue->pData[uiDataCompareIndex];
			}
		}

#if false
		if( bRFIDUIDFindSuccess == true )
		{
			memset(&BkSram_SystemInfo.ucarrRFIDUID[0], NULL,RFID_08C_DATA_LENGTH);
			for( j=0; j<RFID_08C_DATA_LENGTH; j++ )	//SOF,EOF 제외
			{
				uiDataCompareIndex = (pQueue->uiFront+RFID_08C_DATA_POSITION+j) % (pQueue->uiQueueSize);
				BkSram_SystemInfo.ucarrRFIDUID[j] = pQueue->pData[uiDataCompareIndex];
			}
//			printf("%s\r\n",BkSram_SystemInfo.ucarrRFIDUID);
		}
#endif
		while(!IsEmptyQueue(pQueue))
		{
//			  printf("%c ",pQueue->pData[(uiDataSOFIndex+i)%uiReceivedPacket]);
			  PopQueue(pQueue, (BYTE *)pQueue->pData[(uiDataSOFIndex+i)%uiReceivedPacket]);
			  i++;
		}
	}
	return bRFIDUIDFindSuccess;
}

void ParsingGITProtocol(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType)
{
	static stGIT_PTCL_PAYLOAD *pInterPtcl;
	stQueue *pQueue = (stQueue*)pBuff;
	unsigned char 	ucFrameSeqNum;
	unsigned char 	*pucInterPtclSeqNum;
	unsigned int	uiTmpIndex;

	// 1. queue에서 GIT 프로토콜을 찾는다.
	// 2. 찾아지면, 체크섬 검사하고
	// 3. SEQUENCE 체크 -> 예외처리
	// 4. 패킷 복사 후 function ID에 맞는 동작을 한다.

	if(FindGITPacket(pBuff)) {
		/*
		#define GITPACKET_MODE0_IDX		3
		#define GITPACKET_MODE1_IDX		4
		#define GITPACKET_SEQ_IDX		5
		#define GITPACKET_PAYLOAD_IDX	6
		*/
		unsigned char ucMode0, ucMode1;

//		printf("pQueue->uiQueueSize: 0x%02x\n", pQueue->uiQueueSize);
//		printf("pQueue->uiFront: 0x%02x\n", pQueue->uiFront);
//		printf("pQueue->uiRear: 0x%02x\n", pQueue->uiRear);

		//Mod1은 0xF1=PAD(tablet), 0xF3=VCI_II 무선 보드, 0xF7=VCI2
		uiTmpIndex = (pQueue->uiFront + GITPACKET_MODE0_IDX) % pQueue->uiQueueSize;
		ucMode0 = pQueue->pData[uiTmpIndex];

		uiTmpIndex = (pQueue->uiFront + GITPACKET_MODE1_IDX) % pQueue->uiQueueSize;
		ucMode1 = pQueue->pData[uiTmpIndex];

		uiTmpIndex = (pQueue->uiFront + GITPACKET_SEQ_IDX) % pQueue->uiQueueSize;
		ucFrameSeqNum = pQueue->pData[uiTmpIndex];

//		printf("ucMode0: 0x%02x\n", ucMode0);
//		printf("ucMode1: 0x%02x\n", ucMode1);
//		printf("ucFrameSeqNum: 0x%02x\n", ucFrameSeqNum);

		// copy to global InterProtocol structure variable
		g_InputCommType = eWhatCommType;
//		printf("eWhatCommType: 0x%02x\n", eWhatCommType);
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
		if(eWhatCommType == eCOMM_TYPE_USB) {
//			printf("eCOMM_TYPE_USB\n");
			pInterPtcl = (stGIT_PTCL_PAYLOAD*)&g_stUsbInPtcl;
			g_ucUsbLastSeqNum = ucFrameSeqNum;
			pucInterPtclSeqNum = &g_ucUsbLastSeqNum;
		}
		else 
#endif
		{
			//pInterPtcl = (stGIT_PTCL_PAYLOAD*)&g_stPlusInPtcl;
			pInterPtcl = (stGIT_PTCL_PAYLOAD*)&g_stSelftestInPtcl;
			g_ucSelftestLastSeqNum = ucFrameSeqNum;
			pucInterPtclSeqNum = &g_ucSelftestLastSeqNum;
		}

		CopyFromQueueToGITPayload(pBuff, (unsigned char*)pInterPtcl, ucMode0);

		if(ucMode0 == 0x80) {	// Last frame
			if(ucMode1 == 0x00) {
				// git프로토콜 처리
				ProcessGITInterProtocol(pInterPtcl, eWhatCommType);
			}
			else if(ucMode1 == 0xF1) {		// 0xF1=PAD(tablet)
				if(((pInterPtcl->FunctionID&0xFFF0) == 0xC050)||((pInterPtcl->FunctionID&0xFF00) == 0xD000))
					ProcessGITInterProtocol(pInterPtcl, eWhatCommType);

				// bypass
			}
			else if(ucMode1 == 0xF3) {	// 0xF3=VCI_II 무선 보드
				// bypass
				// spi와 uart통신이 존재
				// uart 통신은 function ID가 0xC016
				//ProcessGITInterProtocol(pInterPtcl, eWhatCommType);
			}
			else if(ucMode1 == 0xF7) {	// 0xF7=VCI2
				// git프로토콜 처리
				ProcessGITInterProtocol(pInterPtcl, eWhatCommType);
			}
			else if(ucMode1 == 0xFF) {
				// j2534 프로시져로 고고.
				//ProcessJ2534Protocol(pInterPtcl, eWhatCommType);
			}
		}
		else if(ucMode0 == 0x40) {
			// 시퀀스 넘버가 같으면
			if(g_ucSelftestLastSeqNum == *pucInterPtclSeqNum) {
				unsigned char ucTmp;

				GITDebugPrintf("[%s] ucMode0 is %d, so send Serial ACK\r\n", __FUNCTION__, ucMode0);
				ucTmp = GITPACKET_SERIAL_ACK;
				SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, &ucTmp, sizeof(ucTmp), eWhatCommType);
			}
		}
	}
}
void ParsingGITProtocol_SELFTEST(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType)
{
	static stGIT_PTCL_PAYLOAD *pInterPtcl;
	stQueue *pQueue = (stQueue*)pBuff;
	unsigned char 	ucFrameSeqNum;
	unsigned char 	*pucInterPtclSeqNum;
	unsigned int	uiTmpIndex;


	unsigned char ucMode0, ucMode1;
	// 1. queue에서 GIT 프로토콜을 찾는다.
	// 2. 찾아지면, 체크섬 검사하고
	// 3. SEQUENCE 체크 -> 예외처리
	// 4. 패킷 복사 후 function ID에 맞는 동작을 한다.


#if defined(PROTOCOL14)
#if defined(FEATURE_EXTENSION_BOARD)
#else
	if( GetServiceType() == DCS_Fleet )
	{
		if ( FindRFIDPacket(pBuff) ){}
	}
	else
#endif
	{
		if ( FindGITPacket(pBuff) )
		{
			/*
			#define GITPACKET_MODE0_IDX		3
			#define GITPACKET_MODE1_IDX		4
			#define GITPACKET_SEQ_IDX		5
			#define GITPACKET_PAYLOAD_IDX	6
			*/
			//unsigned char ucMode0, ucMode1;

			//Mod1은 0xF1=PAD(tablet), 0xF3=VCI_II 무선 보드, 0xF7=VCI2
			uiTmpIndex = (pQueue->uiFront+GITPACKET_MODE0_IDX) % pQueue->uiQueueSize;
			ucMode0 = pQueue->pData[uiTmpIndex];

			uiTmpIndex = (pQueue->uiFront+GITPACKET_MODE1_IDX) % pQueue->uiQueueSize;
			ucMode1 = pQueue->pData[uiTmpIndex];

			uiTmpIndex = (pQueue->uiFront+GITPACKET_SEQ_IDX) % pQueue->uiQueueSize;
			ucFrameSeqNum = pQueue->pData[uiTmpIndex];

			// copy to global InterProtocol structure variable
			g_InputCommType = eWhatCommType;
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
			if ( eWhatCommType == eCOMM_TYPE_USB )
			{
				pInterPtcl = (stGIT_PTCL_PAYLOAD*)&g_stUsbInPtcl;
				g_ucUsbLastSeqNum = ucFrameSeqNum;
				pucInterPtclSeqNum = &g_ucUsbLastSeqNum;
			}
			else
#endif
			{
				pInterPtcl = (stGIT_PTCL_PAYLOAD*)&g_stSelftestInPtcl;
				g_ucSelftestLastSeqNum = ucFrameSeqNum;
				pucInterPtclSeqNum = &g_ucSelftestLastSeqNum;
			}


	//		printf("ucMode0: 0x%02x, ucMode1: 0x%02x\n", ucMode0, ucMode1);
			CopyFromQueueToGITPayload(pBuff, (unsigned char*)pInterPtcl, ucMode0);

			if ( ucMode0 == 0x80 )	// Last frame
			{
                if ( ucMode1 == 0x00 )
                {
 #if defined(FEATURE_EXTENSION_BOARD)
                   // git프로토콜 처리
                    if(pInterPtcl->FunctionID == 0x1301 || pInterPtcl->FunctionID == 0x1302 ||
                       pInterPtcl->FunctionID == 0x0175 || pInterPtcl->FunctionID == 0x0275 || pInterPtcl->FunctionID == 0x0375 || pInterPtcl->FunctionID == 0x0475         //FOTA 업데이트 처리
                       )
                    {
                        ProcessGITInterProtocol(pInterPtcl, eWhatCommType);     //ProductUnitTest
                    }
                    else
                    {
                        ProcessExtendInterProtocol(pInterPtcl, eWhatCommType);      //Extend Board
                    }
#else
					ProcessGITInterProtocol(pInterPtcl, eWhatCommType);
#endif

                }
				else if ( ucMode1 == 0xF1 )		// 0xF1=PAD(tablet)
				{
					if(((pInterPtcl->FunctionID&0xFFF0) == 0xC050)||((pInterPtcl->FunctionID&0xFF00) == 0xD000))
						ProcessGITInterProtocol(pInterPtcl, eWhatCommType);

					// bypass
				}
				else if ( ucMode1 == 0xF3 )	// 0xF3=VCI_II 무선 보드
				{
					// bypass
					// spi와 uart통신이 존재
					// uart 통신은 function ID가 0xC016
					//ProcessGITInterProtocol(pInterPtcl, eWhatCommType);
				}
				else if ( ucMode1 == 0xF7 )	// 0xF7=VCI2
				{
					// git프로토콜 처리
					ProcessGITInterProtocol(pInterPtcl, eWhatCommType);
				}
				else if ( ucMode1 == 0xFF )
				{
					// j2534 프로시져로 고고.
					//ProcessJ2534Protocol(pInterPtcl, eWhatCommType);
				}
			}
			else if ( ucMode0 == 0x40 )
			{
				// 시퀀스 넘버가 같으면
				if ( g_ucSelftestLastSeqNum == *pucInterPtclSeqNum  )
				{
					unsigned char ucTmp;
					GITDebugPrintf("[%s] ucMode0 is %d, so send Serial ACK\r\n", __FUNCTION__, ucMode0);
					ucTmp = GITPACKET_SERIAL_ACK;
					SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, &ucTmp, sizeof(ucTmp), eWhatCommType);
				}
			}
		}
	}
#else	//PROTOCOL14
	if ( FindGITPacket(pBuff) )
	{
		/*
		#define GITPACKET_MODE0_IDX		3
		#define GITPACKET_MODE1_IDX		4
		#define GITPACKET_SEQ_IDX		5
		#define GITPACKET_PAYLOAD_IDX	6
		*/
		unsigned char ucMode0, ucMode1;

		//Mod1은 0xF1=PAD(tablet), 0xF3=VCI_II 무선 보드, 0xF7=VCI2
		uiTmpIndex = (pQueue->uiFront+GITPACKET_MODE0_IDX) % pQueue->uiQueueSize;
		ucMode0 = pQueue->pData[uiTmpIndex];

		uiTmpIndex = (pQueue->uiFront+GITPACKET_MODE1_IDX) % pQueue->uiQueueSize;
		ucMode1 = pQueue->pData[uiTmpIndex];

		uiTmpIndex = (pQueue->uiFront+GITPACKET_SEQ_IDX) % pQueue->uiQueueSize;
		ucFrameSeqNum = pQueue->pData[uiTmpIndex];

		// copy to global InterProtocol structure variable
		g_InputCommType = eWhatCommType;
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
		if ( eWhatCommType == eCOMM_TYPE_USB )
		{
			pInterPtcl = (stGIT_PTCL_PAYLOAD*)&g_stUsbInPtcl;
			g_ucUsbLastSeqNum = ucFrameSeqNum;
			pucInterPtclSeqNum = &g_ucUsbLastSeqNum;
		}
		else
#endif
		{
			pInterPtcl = (stGIT_PTCL_PAYLOAD*)&g_stSelftestInPtcl;
			g_ucSelftestLastSeqNum = ucFrameSeqNum;
			pucInterPtclSeqNum = &g_ucSelftestLastSeqNum;
		}


//		printf("ucMode0: 0x%02x, ucMode1: 0x%02x\n", ucMode0, ucMode1);
		CopyFromQueueToGITPayload(pBuff, (unsigned char*)pInterPtcl, ucMode0);

		if ( ucMode0 == 0x80 )	// Last frame
		{
			if ( ucMode1 == 0x00 )
			{
				// git프로토콜 처리
				ProcessGITInterProtocol(pInterPtcl, eWhatCommType);
			}
			else if ( ucMode1 == 0xF1 )		// 0xF1=PAD(tablet)
			{
				if(((pInterPtcl->FunctionID&0xFFF0) == 0xC050)||((pInterPtcl->FunctionID&0xFF00) == 0xD000))
					ProcessGITInterProtocol(pInterPtcl, eWhatCommType);

				// bypass
			}
			else if ( ucMode1 == 0xF3 )	// 0xF3=VCI_II 무선 보드
			{
				// bypass
				// spi와 uart통신이 존재
				// uart 통신은 function ID가 0xC016
				//ProcessGITInterProtocol(pInterPtcl, eWhatCommType);
			}
			else if ( ucMode1 == 0xF7 )	// 0xF7=VCI2
			{
				// git프로토콜 처리
				ProcessGITInterProtocol(pInterPtcl, eWhatCommType);
			}
			else if ( ucMode1 == 0xFF )
			{
				// j2534 프로시져로 고고.
				//ProcessJ2534Protocol(pInterPtcl, eWhatCommType);
			}
		}
		else if ( ucMode0 == 0x40 )
		{
			// 시퀀스 넘버가 같으면
			if ( g_ucSelftestLastSeqNum == *pucInterPtclSeqNum  )
			{
				unsigned char ucTmp;
				GITDebugPrintf("[%s] ucMode0 is %d, so send Serial ACK\r\n", __FUNCTION__, ucMode0);
				ucTmp = GITPACKET_SERIAL_ACK;
				SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, &ucTmp, sizeof(ucTmp), eWhatCommType);
			}
		}
	}
#endif	//PROTOCOL14
}

void SendGITPtclFrame(char* bSendBuff, unsigned int uiSendLen, eCommType eWhatCommType, void* lParam, unsigned int wParam)
{
//	printf("@%s() eCommType %d\r\n", __FUNCTION__, eWhatCommType);
	switch( eWhatCommType ) {
		case eCOMM_TYPE_UART_BT:
			OemWriteUartBTBuff((unsigned char*)bSendBuff, uiSendLen, NULL, eWhatCommType);
			break;
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
		case eCOMM_TYPE_USB:
			OemWriteUsbBuff((unsigned char*)bSendBuff, uiSendLen, NULL, eWhatCommType);
			break;
#endif

#if defined(USE_UBLOX_GPS)
		case eCOMM_TYPE_UART_GPS:
			m_bUsedStorage = true;
#ifdef RF_COMMON_MODEM //mod.kks 21.10.27 to check the send packet in PLS63.
#else
			OemWriteUartGPSBuff((unsigned char*)bSendBuff, uiSendLen, NULL, eWhatCommType);
#endif
			m_bUsedStorage = false;
			break;
#endif

		case eCOMM_TYPE_UART_SELFTEST:/* eCOMM_TYPE_UART_PLUSBD */
			OemWriteUartSelftestBuff((unsigned char*)bSendBuff, uiSendLen, NULL, eWhatCommType);
			break;

//		case eCOMM_TYPE_UART_PLUSBD:
//			OemWriteUartPLUSBuff(bSendBuff, uiSendLen, NULL, eWhatCommType);
//			break;

		default:
			GITDebugPrintf("[%s] not defined, eCommType %d, %d\r\n", __FUNCTION__, uiSendLen, eWhatCommType);
			break;
	}

#ifdef DEBUG_GIT_PTCL_PACKET_PRINT
{
	int i;

	GITDebugPrintf("[%s] Send Len %d, eCommType %d\r\n", __FUNCTION__, uiSendLen, eWhatCommType);
	for (i=0; i<uiSendLen; i++ )
		GITDebugPrintf("%02X ", bSendBuff[i]);
	GITDebugPrintf("\r\n");
}
#endif
}

void MakeBTPtclFrame_n_Send(unsigned char* bSendBuff, stBT_PTCL_PAYLOAD *pSendInterPtcl, eCommType eWhatCommType)
{
	unsigned short usPacketLen;

	// copy SOF
	bSendBuff[DCSPACKET_SOF_IDX] = DCSPACKET_SERIAL_SOF;

	// RESERVE
	bSendBuff[DCSPACKET_RESERVE1_IDX]=0;
	bSendBuff[DCSPACKET_RESERVE2_IDX]=0;

	//length
	usPacketLen = pSendInterPtcl->DataLength + DCSPACKET_PAYLOAD_FUNC_SIZE + DCSPACKET_PAYLOAD_FUNC_SIZE + DCSPACKET_PAYLOAD_RESERVE_SIZE;
	memcpy(&bSendBuff[DCSPACKET_PAYLOAD_LEN0_IDX],
		   &usPacketLen,
		   sizeof(pSendInterPtcl->DataLength));

	//func ID
	memcpy(&bSendBuff[DCSPACKET_PAYLOAD_FUNC0_IDX],
		   &pSendInterPtcl->FunctionID,
		   sizeof(pSendInterPtcl->FunctionID));

	memcpy(&bSendBuff[DCSPACKET_PAYLOAD_DATA_IDX],
		   pSendInterPtcl->pPayload,
		   pSendInterPtcl->DataLength);

	// checksum
	bSendBuff[usPacketLen + 1] = CalcChecksumBTPtclFromArray(&bSendBuff[DCSPACKET_RESERVE1_IDX],
														  pSendInterPtcl->DataLength + DCSPACKET_PAYLOAD_LENGTH_SIZE + DCSPACKET_PAYLOAD_RESERVE_SIZE + DCSPACKET_PAYLOAD_FUNC_SIZE);

	g_usLastSentGITFrameLen = usPacketLen + 2; /* LENGTH + SOF + CS */
	g_eLastSentCommType = eWhatCommType;
	SendGITPtclFrame((char *)bSendBuff, g_usLastSentGITFrameLen, g_eLastSentCommType, NULL, 0);
}

void MakeAMThdlcPtclFrame_n_Send(unsigned char* bSendBuff, stAMT_PTCL_PAYLOAD *pSourceInterPtcl, eCommType eWhatCommType)
{
//	g_eLastSentCommType = eWhatCommType;
//
//	struct diag_send_desc_type stSourcePacket = { NULL, NULL, DIAG_STATE_START, 0 };
//	struct diag_hdlc_dest_type 	stDestinationPacket = { NULL, NULL, 0 };
//	unsigned int uiEncoded_Rsp_Length;
//
//	stSourcePacket.state = DIAG_STATE_START;
//	stSourcePacket.pkt = pSourceInterPtcl;
//	stSourcePacket.last = (void *)((U8*)pSourceInterPtcl +g_usLastSentGITFrameLen + 1);
//	stSourcePacket.terminate = 1;
//
//	stDestinationPacket.dest = bSendBuff;
//	stDestinationPacket.dest_last = (void *)(bSendBuff + MAX_MODEM_PROTO_DATA_LENGTH - 1);
//	diag_hdlc_encode(&stSourcePacket, &stDestinationPacket);
//
//	uiEncoded_Rsp_Length = (int)((char *)stDestinationPacket.dest - (char *)bSendBuff);
//
//	g_usLastSentGITFrameLen = uiEncoded_Rsp_Length;
//
//	SendGITPtclFrame(bSendBuff, g_usLastSentGITFrameLen, eWhatCommType, NULL, 0);

}

void MakeGITPtclFrame_n_Send(unsigned char* bSendBuff, stGIT_PTCL_PAYLOAD *pSendInterPtcl, unsigned char ucTarget, eCommType eWhatCommType)
{
	static unsigned char s_uiSendPlusSeqNum = 0;
	unsigned short usPacketLen;
	unsigned char *pucFrameSeqNum;

	// copy SOF
	bSendBuff[GITPACKET_SOF_IDX] = GITPACKET_SERIAL_SOF;

	// copy Length
	usPacketLen = pSendInterPtcl->DataLength + 5/*Len0 & Len1 & Mod0 & Mod1, Seq fields*/;

	memcpy(&bSendBuff[GITPACKET_LEN_IDX], &usPacketLen, sizeof(usPacketLen));

	// copy payload header
	memcpy(&bSendBuff[GITPACKET_PAYLOAD_IDX], pSendInterPtcl, GITPACKET_PAYLOAD_HEADER_SIZE);

	// copy payload data
	memcpy(&bSendBuff[GITPACKET_PAYLOAD_IDX+GITPACKET_PAYLOAD_HEADER_SIZE], pSendInterPtcl->pPayload, pSendInterPtcl->DataLength-GITPACKET_PAYLOAD_HEADER_SIZE);

	bSendBuff[GITPACKET_MODE0_IDX] = GITPACKET_SERIAL_MODE0_LAST_PACKET;
	bSendBuff[GITPACKET_MODE1_IDX] = ucTarget;

	pucFrameSeqNum = &s_uiSendPlusSeqNum;

	bSendBuff[GITPACKET_SEQ_IDX] = ++(*pucFrameSeqNum);
	bSendBuff[usPacketLen+1/*include SOF*/] = GITPACKET_SERIAL_EOF;

	// checksum
	bSendBuff[usPacketLen+2/*include  EOF*/] = CalcChecksumGITPtclFromArray(bSendBuff, usPacketLen + 2/*include  EOF*/);;

	g_usLastSentGITFrameLen = usPacketLen + 3;
	g_eLastSentCommType = eWhatCommType;

	SendGITPtclFrame((char *)bSendBuff, g_usLastSentGITFrameLen, g_eLastSentCommType, NULL, 0);
}

unsigned char CalcChecksumGITPtclFromArray(unsigned char* pBuff, unsigned int uiLength)
{
	unsigned char ucCalcCheckSum = 0;
	int i;

	for ( i=1; i<uiLength; i++ )
	{
		ucCalcCheckSum += pBuff[i];
	}

	return ucCalcCheckSum;
}

unsigned char CalcChecksumBTPtclFromArray(unsigned char* pBuff, unsigned int uiLength)
{
	unsigned char ucCalcCheckSum = 0;
	int i;

	for ( i=0; i<uiLength; i++ )
	{
		ucCalcCheckSum += pBuff[i];
	}

	return ucCalcCheckSum;
}

unsigned char CalcChecksumBTPtclFromQueue(unsigned char* pBuff, unsigned int uiLength)
{
	stQueue* pQueue;
	unsigned char ucCalcChecksum = 0;
	unsigned int uiCalcIdx, i;

	pQueue = (stQueue*)pBuff;
#ifdef DEBUG_GIT_PTCL_PACKET_PRINT
	GITDebugPrintf("\r\n\r\nInput Packet :\r\n%02X ", pQueue->pData[pQueue->uiFront]);
#endif
	for ( i=DCSPACKET_RESERVE1_IDX; i<uiLength/* SOF & EOF */; i++ )
	{
		uiCalcIdx = (pQueue->uiFront+i)%pQueue->uiQueueSize;
		ucCalcChecksum += pQueue->pData[uiCalcIdx];
#ifdef DEBUG_GIT_PTCL_PACKET_PRINT
		GITDebugPrintf("%02X ", pQueue->pData[uiCalcIdx]);
#endif
	}

#ifdef DEBUG_GIT_PTCL_PACKET_PRINT
	GITDebugPrintf("%02X ", pQueue->pData[(pQueue->uiFront+i)%pQueue->uiQueueSize]); //checksum print
	GITDebugPrintf("\r\n");
#endif

	return ucCalcChecksum;
}

unsigned char CalcChecksumGITPtclFromQueue(unsigned char* pBuff, unsigned int uiLength)
{
	stQueue* pQueue;
	unsigned char ucCalcChecksum = 0;
	unsigned int uiCalcIdx, i;

	pQueue = (stQueue*)pBuff;
#ifdef DEBUG_GIT_PTCL_PACKET_PRINT
	GITDebugPrintf("\r\n\r\nInput Packet :\r\n%02X ", pQueue->pData[pQueue->uiFront]);
#endif
	for ( i=GITPACKET_LEN_IDX; i<uiLength+2/* SOF & EOF */; i++ )
	{
		uiCalcIdx = (pQueue->uiFront+i)%pQueue->uiQueueSize;
		ucCalcChecksum += pQueue->pData[uiCalcIdx];
#ifdef DEBUG_GIT_PTCL_PACKET_PRINT
		GITDebugPrintf("%02X ", pQueue->pData[uiCalcIdx]);
#endif
	}

#ifdef DEBUG_GIT_PTCL_PACKET_PRINT
	GITDebugPrintf("%02X ", pQueue->pData[(pQueue->uiFront+i)%pQueue->uiQueueSize]); //checksum print
	GITDebugPrintf("\r\n");
#endif

	return ucCalcChecksum;
}

void MakeBTPtclPayloadFrame(stBT_PTCL_PAYLOAD *pInterPtcl, unsigned short int unFunctionID, unsigned char *pData, int nDataSize)
{
	int i;

	pInterPtcl->DataLength 		= nDataSize;
	pInterPtcl->FunctionID 		= unFunctionID;

	for ( i=0; (i<nDataSize) && (pData != NULL); i++ )
	{
		pInterPtcl->pPayload[i] = pData[i];
	}
}

void MakeGITPtclPayloadFrame(stGIT_PTCL_PAYLOAD *pInterPtcl, unsigned short int unFunctionID, unsigned char *pData, int nDataSize)
{
	int i;
	unsigned short int siCheckSum;

	pInterPtcl->DataLength 		= GITPACKET_PAYLOAD_HEADER_SIZE + nDataSize;
	pInterPtcl->FunctionID 		= unFunctionID;
	pInterPtcl->CurrentFrame 	= 0;
	pInterPtcl->CheckSum		= 0;

	for ( i=0; (i<nDataSize) && (pData != NULL); i++ )
	{
		pInterPtcl->pPayload[i] = pData[i];
	}
	siCheckSum = CalcChecksumGITPtclPayloadFrame(pInterPtcl);
	pInterPtcl->CheckSum = siCheckSum;

	siCheckSum = siCheckSum;
}

unsigned short int CalcChecksumGITPtclPayloadFrame(stGIT_PTCL_PAYLOAD *pInterPtcl)
{
	unsigned short usReturnVaule, i, usPayloadLen;
	unsigned char* p;

	p = (unsigned char*)pInterPtcl;
	usReturnVaule = 0;
	pInterPtcl->CheckSum = 0;
	usPayloadLen = pInterPtcl->DataLength-GITPACKET_PAYLOAD_HEADER_SIZE;

	// copy payload header
	for ( i=0; i<GITPACKET_PAYLOAD_HEADER_SIZE; i++ )
		usReturnVaule += *(p+i);
	// copy payload data
	for(i=0; i<usPayloadLen; i++)
		usReturnVaule += *(pInterPtcl->pPayload+i);

	return usReturnVaule;
}

unsigned short int CalcChecksumBTPtclPayloadFrame(stBT_PTCL_PAYLOAD *pInterPtcl)
{
	unsigned short usReturnVaule, i, usPayloadLen;
	unsigned char* p;

	p = (unsigned char*)pInterPtcl;
	usReturnVaule = 0;
	usPayloadLen = pInterPtcl->DataLength-DCSPACKET_PAYLOAD_HEADER_SIZE;

	// copy payload header
	for ( i=0; i<DCSPACKET_PAYLOAD_HEADER_SIZE; i++ )
		usReturnVaule += *(p+i);
	// copy payload data
	for(i=0; i<usPayloadLen; i++)
		usReturnVaule += *(pInterPtcl->pPayload+i);

	return usReturnVaule;
}

void CopyFromQueueToDCSPayload(unsigned char* pBuff, unsigned char* pInterPtcl)
{
	// 전체 GIT 프로토콜에서 Payload 프레임만 pInterPtcl로 복사를 해야한다.
	stBT_PTCL_PAYLOAD *pGitPtl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	stQueue* pQueue = (stQueue*)pBuff;
	unsigned char ucTmp[20];
	unsigned int uiCopyLen;

	uiCopyLen = (pQueue->pData[((pQueue->uiFront+DCSPACKET_PAYLOAD_LEN0_IDX)% pQueue->uiQueueSize)] & 0x00FF) +
				((pQueue->pData[((pQueue->uiFront+DCSPACKET_PAYLOAD_LEN1_IDX)% pQueue->uiQueueSize)] << 8) & 0xFF00);

	pGitPtl->DataLength = uiCopyLen;
	pGitPtl->FunctionID =  (pQueue->pData[((pQueue->uiFront+ DCSPACKET_PAYLOAD_FUNC0_IDX)% pQueue->uiQueueSize)] & 0x00FF) +
				((pQueue->pData[((pQueue->uiFront+ DCSPACKET_PAYLOAD_FUNC1_IDX)% pQueue->uiQueueSize)] << 8) & 0xFF00);

	// SOF LENGTH까지 Queue에서 제거
	PopMultiDataQueue(pQueue, ucTmp, DCSPACKET_PAYLOAD_HEADER_SIZE+DCSPACKET_PAYLOAD_FUNC_SIZE);

	// copy Payload Data
	PopMultiDataQueue(pQueue, (unsigned char*)pGitPtl->pPayload, pGitPtl->DataLength);

	// CHECKSUM을 Queue에서 제거
	PopQueue(pQueue, ucTmp);
}

void CopyFromQueueToGITPayload(unsigned char* pBuff, unsigned char* pInterPtcl, unsigned char ucMode0)
{
#if 0
////	// 전체 GIT 프로토콜에서 Payload 프레임만 pInterPtcl로 복사를 해야한다.
////	// sof | Len0 | Len1 | Mod0 | Mod1 | Seq |Payload | Eof | CS
////	stGIT_PTCL_PAYLOAD *pGitPtl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
////	stQueue* pQueue = (stQueue*)pBuff;
////	unsigned char ucTmp;
////	unsigned int i;
////	unsigned int uiCopyLen;
////
////GITDebugPrintf("000 [%s] s_bMultiFirstFrame %d, s_uiMutiFrameCopyIndex %d\r\n", __FUNCTION__, s_bMultiFirstFrame, g_uiMutiFrameCopyIndex);
////	if ( ucMode0 == 0x00 )
////	{
////		uiCopyLen = (pQueue->pData[((pQueue->uiFront+GITPACKET_LEN_IDX)% pQueue->uiQueueSize)] & 0x00FF) +
////					((pQueue->pData[((pQueue->uiFront+GITPACKET_LEN_IDX+1)% pQueue->uiQueueSize)] << 8) & 0xFF00);
////
////GITDebugPrintf("1111 [%s] s_bMultiFirstFrame %d, g_uiMutiFrameCopyIndex %d, uiCopyLen %d\r\n", __FUNCTION__, s_bMultiFirstFrame, g_uiMutiFrameCopyIndex, uiCopyLen);
////
////		// SOF에서 SEQ까지 Queue에서 제거
////		for ( i=0; i<GITPACKET_PAYLOAD_IDX; i++ ) PopQueue(pQueue, &ucTmp);
////
////		if ( s_bMultiFirstFrame == FALSE ) // multi frame의 처음에만 헤더를 붙임.
////		{
////			uiCopyLen -= (5/*Len0 & Len1 & Mod0 & Mod1 & Seq 제외*/ + GITPACKET_PAYLOAD_HEADER_SIZE);
////			// copy Payload header  : multi frame의 처음에만 헤더를 붙임.
////			PopMultiDataQueue(pQueue, (unsigned char*)pGitPtl, GITPACKET_PAYLOAD_HEADER_SIZE);
////		}
////		else
////			uiCopyLen -= (5/*Len0 & Len1 & Mod0 & Mod1 & Seq 제외*/);
////
////
////		// copy Payload Data
////		PopMultiDataQueue(pQueue, (unsigned char*)pGitPtl->pPayload+g_uiMutiFrameCopyIndex, uiCopyLen);
////		g_uiMutiFrameCopyIndex += uiCopyLen;
////		s_bMultiFirstFrame = TRUE;
//// 	}
////	else if ( ucMode0 == 0x80 )
////	{
////		uiCopyLen = (pQueue->pData[((pQueue->uiFront+GITPACKET_PAYLOAD_IDX)% pQueue->uiQueueSize)] & 0x00FF) +
////					((pQueue->pData[((pQueue->uiFront+1+GITPACKET_PAYLOAD_IDX)% pQueue->uiQueueSize)] << 8) & 0xFF00);
////		uiCopyLen -= GITPACKET_PAYLOAD_HEADER_SIZE;
////
////		// SOF에서 SEQ까지 Queue에서 제거
////		for ( i=0; i<GITPACKET_PAYLOAD_IDX; i++ ) PopQueue(pQueue, &ucTmp);
////
////		if ( s_bMultiFirstFrame == FALSE ) // multi frame의 처음에만 헤더를 붙임.
////		{
////			// copy Payload header
////			PopMultiDataQueue(pQueue, (unsigned char*)pGitPtl, GITPACKET_PAYLOAD_HEADER_SIZE);
////		}
////		else
////			GITDebugPrintf("2222 [%s] s_bMultiFirstFrame %d, g_uiMutiFrameCopyIndex %d\r\n", __FUNCTION__, s_bMultiFirstFrame, g_uiMutiFrameCopyIndex);
////
////		// copy Payload Data
////		PopMultiDataQueue(pQueue, (unsigned char*)pGitPtl->pPayload+g_uiMutiFrameCopyIndex, uiCopyLen);
////
////		g_uiMutiFrameCopyIndex = 0;
////		s_bMultiFirstFrame = FALSE;
//// 	}
////
////	// EOF와 CHECKSUM을 Queue에서 제거
////	for ( i=0; i<2; i++ ) PopQueue(pQueue, &ucTmp);
////
////GITDebugPrintf("333 [%s] s_bMultiFirstFrame %d, g_uiMutiFrameCopyIndex %d\r\n", __FUNCTION__, s_bMultiFirstFrame, g_uiMutiFrameCopyIndex);
////
#else
	// 전체 GIT 프로토콜에서 Payload 프레임만 pInterPtcl로 복사를 해야한다.
	// sof | Len0 | Len1 | Mod0 | Mod1 | Seq |Payload | Eof | CS
	stGIT_PTCL_PAYLOAD *pGitPtl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stQueue* pQueue = (stQueue*)pBuff;
	unsigned char ucTmp;
	unsigned int i;
	unsigned int uiCopyLen;

	if ( ucMode0 == 0x00 ) {
		uiCopyLen = (pQueue->pData[((pQueue->uiFront+GITPACKET_LEN_IDX)% pQueue->uiQueueSize)] & 0x00FF) + ((pQueue->pData[((pQueue->uiFront+GITPACKET_LEN_IDX+1)% pQueue->uiQueueSize)] << 8) & 0xFF00);
		// SOF에서 SEQ까지 Queue에서 제거
		for ( i=0; i<GITPACKET_PAYLOAD_IDX; i++ ) {
			PopQueue(pQueue, &ucTmp);
		}

		if ( g_uiMutiFrameCopyIndex == 0 ) { // multi frame의 처음에만 헤더를 붙임.
			uiCopyLen -= (5/*Len0 & Len1 & Mod0 & Mod1 & Seq 제외*/ + GITPACKET_PAYLOAD_HEADER_SIZE);
			// copy Payload header  : multi frame의 처음에만 헤더를 붙임.
			PopMultiDataQueue(pQueue, (unsigned char*)pGitPtl, GITPACKET_PAYLOAD_HEADER_SIZE);
		}
		else {
			uiCopyLen -= (5/*Len0 & Len1 & Mod0 & Mod1 & Seq 제외*/);
		}

		// copy Payload Data
		PopMultiDataQueue(pQueue, (unsigned char*)pGitPtl->pPayload + g_uiMutiFrameCopyIndex, uiCopyLen);
		g_uiMutiFrameCopyIndex += uiCopyLen;
 	}
	else if(ucMode0 == 0x80) {
		uiCopyLen = (pQueue->pData[((pQueue->uiFront + GITPACKET_PAYLOAD_IDX) % pQueue->uiQueueSize)] & 0x00FF) + ((pQueue->pData[((pQueue->uiFront + 1 + GITPACKET_PAYLOAD_IDX) % pQueue->uiQueueSize)] << 8) & 0xFF00);
		uiCopyLen -= GITPACKET_PAYLOAD_HEADER_SIZE;

		// SOF에서 SEQ까지 Queue에서 제거
		for(i = 0; i < GITPACKET_PAYLOAD_IDX; i++) {
			PopQueue(pQueue, &ucTmp);
		}

		if ( g_uiMutiFrameCopyIndex == 0 ) { // multi frame의 처음에만 헤더를 붙임.
			// copy Payload header
			PopMultiDataQueue(pQueue, (unsigned char*)pGitPtl, GITPACKET_PAYLOAD_HEADER_SIZE);
		}

		// copy Payload Data
		PopMultiDataQueue(pQueue, (unsigned char*)pGitPtl->pPayload + g_uiMutiFrameCopyIndex, uiCopyLen);

		g_uiMutiFrameCopyIndex = 0;
 	}

	// EOF와 CHECKSUM을 Queue에서 제거
	for(i = 0; i < 2; i++)
		PopQueue(pQueue, &ucTmp);

#endif
}

void SendGITPtclResponse(stGIT_PTCL_PAYLOAD *pInterPaylod, unsigned char* pResponseData, unsigned int uiResDataLen, eCommType eWhatCommType)
{
	unsigned char Target = GITPACKET_MODE1_DCSP;

	MakeGITPtclPayloadFrame(pInterPaylod, pInterPaylod->FunctionID, pResponseData, uiResDataLen);
	MakeGITPtclFrame_n_Send(g_arrOutputGITPtclBuff, pInterPaylod, Target, (eCommType)eWhatCommType);
}

void SendBTPtclResponse(stBT_PTCL_PAYLOAD *pInterPaylod, unsigned char* pResponseData, unsigned int uiResDataLen, eCommType eWhatCommType)
{
	MakeBTPtclPayloadFrame(pInterPaylod, pInterPaylod->FunctionID, pResponseData, uiResDataLen);
	MakeBTPtclFrame_n_Send(g_arrOutputGITPtclBuff, pInterPaylod, (eCommType)eWhatCommType);
}

////////////////////////////////////////////////////////////////////////////////
// QUEUE UTIL
stQueue* CreateQueue(unsigned uiQueueSize)
{
	stQueue *pQueue = NULL;

	pQueue = malloc(sizeof(stQueue));

	if ( pQueue != NULL)
	{
		pQueue->pData = malloc(uiQueueSize);

		if ( pQueue->pData )
		{
			pQueue->uiFront = 0;
			pQueue->uiRear = 0;
			pQueue->uiQueueSize = uiQueueSize;
		}
		else
		{
			printf("CreateQueue : pQueue->pData malloc fail\r\n");
			return NULL;
		}
	}
	else
	{
		printf("CreateQueue : pQueue malloc fail\r\n");
		return NULL;
	}

	return pQueue;
}

void DestoryQueue(stQueue* pQueue)
{
	if( (pQueue != NULL) && (pQueue->pData != NULL) )
	{
		free(pQueue->pData);
		free(pQueue);
	}
}

BOOL PushQueue(stQueue* pQueue, BYTE ucPushData, eCommType eCh)
{
	if ( (pQueue->uiFront == pQueue->uiRear+1) || (pQueue == NULL) )
	{
		printf("PushQueue : overflow queue(ch: %d)\r\n", eCh);

		//while(1);
		return FALSE;
	}
//	printf("%d : %d\r\n", pQueue->uiFront, pQueue->uiRear);

	pQueue->pData[pQueue->uiRear] = ucPushData;
	pQueue->uiRear = (pQueue->uiRear+1) % pQueue->uiQueueSize;

	return TRUE;
}

unsigned int PushMultiDataQueue(stQueue* pQueue, BYTE* pPushData, unsigned int uiPushDataLen, eCommType eCh)
{
	unsigned int i, uiPushCount = 0;

	//printf("[%s] push Data : uiPushDataLen %d, eCh %d\r\n<--", __FUNCTION__, uiPushDataLen, eCh);

	for ( i=0; i<uiPushDataLen; i++ ) {
		if ( PushQueue(pQueue, pPushData[i], eCh) ) {
			uiPushCount++;
		}
		else {
			printf("\r\n push failed.....\r\n");

			// clear gps queue because if queue is full, me couldn't recovery because of gps raw data.
			if( eCh == eCOMM_TYPE_UART_GPS )
			{
				ClearQueue(pQueue);
			}

			break;
		}
	}

	return uiPushCount;
}

BOOL PopQueue(stQueue* pQueue, BYTE *pPopData)
{
	if ( (pQueue == NULL) || IsEmptyQueue(pQueue) )
	{
//		printf("PopQueue : empty queue, front %d, rear %d\r\n", pQueue->uiFront, pQueue->uiRear);
		return FALSE;
	}

	*pPopData = pQueue->pData[pQueue->uiFront];
	pQueue->pData[pQueue->uiFront] = 0;
	pQueue->uiFront = (pQueue->uiFront+1) % pQueue->uiQueueSize;

	//printf("(%02X ", *pPopData);

	return TRUE;
}

unsigned int PopMultiDataQueue(stQueue* pQueue, BYTE* pPopData, unsigned int uiPopDataLen)
{
	unsigned int i, uiPopCount = 0;

	for(i = 0; i < uiPopDataLen; i++ ) {
		if(PopQueue(pQueue, &pPopData[i])) {
			uiPopCount++;
		}
		else {
//			printf("\r\npop fail...\r\n");
			break;
		}
	}

	return uiPopCount;
}

BOOL IsEmptyQueue(stQueue* pQueue)
{
	if ( pQueue == NULL ) return TRUE;

	if ( pQueue->uiFront == pQueue->uiRear )
		return TRUE;

	return FALSE;
}

unsigned int GetQueueDataLength(stQueue* pQueue)
{
	unsigned int uiQueDataLen = 0;

	if ( pQueue->uiRear == pQueue->uiFront )
	{
		uiQueDataLen = 0;
	}
	else if ( pQueue->uiRear > pQueue->uiFront )
	{
		uiQueDataLen = pQueue->uiRear - pQueue->uiFront;
	}
	else
	{
		uiQueDataLen = (pQueue->uiQueueSize - pQueue->uiFront) + pQueue->uiRear;
	}

	return uiQueDataLen;
}

BOOL ClearQueue(stQueue* pQueue)
{
	unsigned char ucTmp;
	unsigned int i;
	unsigned int uiQueLen;

	if ( !IsEmptyQueue(pQueue) )
	{
		uiQueLen = GetQueueDataLength(pQueue);
		for ( i=0; i<uiQueLen; i++ )
		{
			PopQueue(pQueue, &ucTmp);
		}
	}
	else
		return FALSE;

	return TRUE;
}

BOOL ReInitQueue(stQueue* pQueue)
{
	unsigned char ucTmp;
	unsigned int i;
	unsigned int uiQueLen;

	if ( !IsEmptyQueue(pQueue) )
	{
		uiQueLen = GetQueueDataLength(pQueue);
		for ( i=0; i<uiQueLen; i++ )
		{
			PopQueue(pQueue, &ucTmp);
		}
	}
	else
		return FALSE;

	return TRUE;
}
// QUEUE
////////////////////////////////////////////////////////////////////////////////

/*****************************END OF FILE****/

