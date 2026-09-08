/**
  ******************************************************************************
  * @file    GIT_CanParsingProc.c
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    20-MAR-2014
  * @brief   Manager GIT_CanParsingProc.c module
  ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "GIT_CanParsingProc.h"
#include "GIT_PassthruDefines.h"
#include "GIT_OemInterface.h"
#include "GIT_VCI.h"
#include "OBD_Manager.h"

#include "GIT_OemInterface.h"
#include "CanFD_RecvHandler.h"
#include "CanFD_Controller.h"
#include "CanFD_Defines.h"

#include "HalHandler.h"
#include "HalCanDriver.h"

//#define DEBUG_CAN_PACKET
/* Defines  ------------------------------------------------------------------*/
//#define DEBUG_CAN_LOG
//#define CAN_RX_STATE_LOG

//#if defined(DCS_SLEEP_TEST)
//#define OBD_RUNNING_TIME 			60000
//#define OBD_SLEEP_TIME 				60
//#define OBD_SLEEP_READY_TIME 	6000
//#else
//#define OBD_RUNNING_TIME 			120000		//60000(1분) * 2
//#define OBD_SLEEP_TIME 				60000		//60000(1분) * 1
//#define OBD_SLEEP_READY_TIME 	600000		//60000(1분) * 10
//#endif

/* Global Variables ----------------------------------------------------------*/
eCanComRxTxState 	g_eCanCommState = eCAN_NONE_STATE;
//eVCanComRxTxState 	g_eVCanCommState = eVCAN_NONE_STATE;
eLCanComRxTxState 	g_eLCanCommState = eLCAN_NONE_STATE;
unsigned int		g_uiCanReadMsgLength;
unsigned int		g_uiCanRxConsFrameNo;
unsigned int		g_uiCanWriteMsgLength;
unsigned int		g_uiCanTxConsFrameNo;
unsigned long 		g_ulCanWrittenTick;
unsigned long 		g_ulCanLineStopTimer;
//unsigned long 		g_ulSleepReadyTimer=0;
BOOL				g_bCanRxConcequtiveFrame = FALSE;
BOOL				g_bCanRxPendingFrame = FALSE;
BOOL				g_bCanRxCarbFrame = FALSE;
BOOL				g_bDCS_ENTER_SLEEP_MODE=TRUE;
//BOOL				g_bDCS_ENTER_SLEEP_MODE=FALSE;	//NONE SLEEP
extern unsigned long		g_ulProtocolID;
stPASSTHRU_MSG		g_stReadPassThruMsg,g_stReadPassThruMsg_V;
stPASSTHRU_MSG		g_stWritePassThruMsg,g_stWritePassThruMsg_L;
extern bool g_bCanDataFlag_Wakeup;
extern unsigned int g_uiCanStopTime;
extern eFUEL_TYPE g_eFuel_Type;

unsigned long g_usRcvCanID=0;
//unsigned long g_usTxCanID=0;
U16 g_ucCanCommFailComplete = 0;

unsigned long		g_ulFilterIndex = 0xFFFFFFFF;
stCanPacket 		g_OutCanPacket,g_OutLCanPacket; // J2534로 인해서 Globla로 변경함.
stCanPacket 		g_InRxCanPacket;
stGITSetConfig 		g_stECUSetConfig;

bool					g_bTripWaitingTime=0;
BOOL				g_bCARBReceiving = FALSE; // 0X07DF & 0x18DB33F1 CARB통신인 경우
int					g_iTimerCarbCallback = -1;

U8 g_ucCanExceptState = 0;
U8 g_ucCanCommSucces = 0;

/* extern Global Variables ---------------------------------------------------*/
extern eCommType			g_InputCommType;
extern U32 g_uiDiagnosisRxCanid[10];
extern U32 	g_uiPeriodicTime;
extern U8  	g_ucPeriodicMessage[20];

extern stHalCANTX_STRUCT   g_CAN1_TxBuffCtrl;
extern stHalCANRX_STRUCT   g_CAN1_RxBuffCtrl;
extern stHalCANTX_STRUCT   g_CAN2_TxBuffCtrl;
extern stHalCANRX_STRUCT   g_CAN2_RxBuffCtrl;


////////////////////////////////////////////////////////////////////////////////
// associated with CARB functions.
////////////////////////////////////////////////////////////////////////////////
#define MAX_CARB_RECV_ARRAY_CNT		30
typedef struct _CARB_REVC_INFO
{
	unsigned int 	uiOrder;
	unsigned int 	uiCANRecvedLength;
	unsigned int 	uiCANTotalPacketLength;
	unsigned long 	ulCANID;
	unsigned int 	uiCANIDLength;
	unsigned int 	uiCANDataSavePos;
}CARB_REVC_INFO;
CARB_REVC_INFO g_CarbRecvInfo[MAX_CARB_RECV_ARRAY_CNT];
void CAN_CarbRecvCallBack();
void CAN_PendingAckCallBack();

unsigned int CAN_GetCarbCanIDLength()
{
	unsigned int i, uiCanIDTotalLen = 0;

	for ( i=0; i<MAX_CARB_RECV_ARRAY_CNT; i++ )
	{
		uiCanIDTotalLen += g_CarbRecvInfo[i].uiCANIDLength;
	}

	return uiCanIDTotalLen;
}

unsigned int CAN_GetCarbRecvedLength(unsigned int uiOrder)
{
	return g_CarbRecvInfo[uiOrder-1].uiCANRecvedLength;
}

unsigned int CAN_GetCarbTotalPacketLength(unsigned int uiOrder)
{
	return g_CarbRecvInfo[uiOrder-1].uiCANTotalPacketLength;
}

void CAN_SaveCarbCanRecvLength(unsigned int uiOrder, unsigned int uiRecvedLen)
{
	g_CarbRecvInfo[uiOrder-1].uiCANRecvedLength += uiRecvedLen;
#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s] uiRecvedLen %d, g_CarbRecvInfo[%d].uiCANRecvedLength %d\r\n",
		__FUNCTION__, uiRecvedLen, uiOrder-1, g_CarbRecvInfo[uiOrder-1].uiCANRecvedLength);
#endif
}

void CAN_ClearCarbCanRecvInfo()
{
	memset(g_CarbRecvInfo, 0x00, sizeof(CARB_REVC_INFO)*MAX_CARB_RECV_ARRAY_CNT);
//	GITDebugPrintf("[%s] memset\r\n", __FUNCTION__);
}

unsigned int CAN_SaveCarbCanRecvInfo(stCanPacket *pInCanPacket, BOOL bIsStandardCan, unsigned int uiCanIDLen, unsigned int uiRecvedLen, unsigned int uiRecvPacketLen)
{
	int i;
	unsigned long ulCANID;
	unsigned int uiSavePos = 0;

	if ( bIsStandardCan )
		ulCANID = pInCanPacket->stNormalPacket.us11BitID;
	else
		ulCANID = (pInCanPacket->stExtendPacket.us11BitID <<18) | (pInCanPacket->stExtendPacket.us18BitID);

	for ( i=0; i<MAX_CARB_RECV_ARRAY_CNT; i++ )
	{
		if ( (g_CarbRecvInfo[i].ulCANID == 0) && (g_CarbRecvInfo[i].uiCANTotalPacketLength == 0) )
			break;
		else
		{
			uiSavePos += g_CarbRecvInfo[i].uiCANIDLength;
			uiSavePos += g_CarbRecvInfo[i].uiCANTotalPacketLength;
		}
	}

	if ( i >= MAX_CARB_RECV_ARRAY_CNT )
	{
		// 이상태가 나오면 안된다. Buffer를 키워줘라.
		GITDebugPrintf("\r\n\r\n\r\n\r\n\r\n[%s] !!!! Max Carb receive Buff.....so, return\r\n\r\n\r\n\r\n", __FUNCTION__);
//		CAN_ClearCarbCanRecvInfo();
//		g_bCARBReceiving = FALSE;		//150223 lwh 추가
//		g_ulCanWrittenTick = 0; 			//150223 lwh 추가
//		g_uiCanReadMsgLength = 0;		//150223 lwh 추가
//		g_stReadPassThruMsg.DataSize = 0;			//150302 lwh 추가
		return 0xFFFFFFFF;
	}

	g_CarbRecvInfo[i].uiOrder = i+1;
	g_CarbRecvInfo[i].ulCANID = ulCANID;
	g_CarbRecvInfo[i].uiCANIDLength = uiCanIDLen;
	g_CarbRecvInfo[i].uiCANTotalPacketLength = uiRecvPacketLen;
	g_CarbRecvInfo[i].uiCANRecvedLength = uiRecvedLen;
	g_CarbRecvInfo[i].uiCANDataSavePos = uiSavePos;

	//if(i==1)	  	CAN_CarbRecvCallBack();
#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s] Index %d, ulCANID 0x%04X, uiCANTotalPacketLength %d, uiRecvedLen %d, uiSavePos %d\r\n",
		__FUNCTION__, i, ulCANID, uiRecvPacketLen, uiRecvedLen, uiSavePos);
#endif

	return i;
}

unsigned int  CAN_AcquireArrayIndexFromCarbCanID(stCanPacket *pInCanPacket, BOOL bIsStandardCan, unsigned int* puiArrayIndex)
{
	unsigned int i, uiOrder = 0, uiSumCanIDLength = 0, uiLastPos = 0;
	unsigned long ulCANID;

	if ( bIsStandardCan )
		ulCANID = pInCanPacket->stNormalPacket.us11BitID;
	else
		ulCANID = (pInCanPacket->stExtendPacket.us11BitID <<18) | (pInCanPacket->stExtendPacket.us18BitID);

	for ( i=0; i<MAX_CARB_RECV_ARRAY_CNT; i++ )
	{
		if ( g_CarbRecvInfo[i].ulCANID == ulCANID )
		{
			uiOrder = g_CarbRecvInfo[i].uiOrder; // Order is equal to i+1
			break;
		}
		else
		{
			if ( g_CarbRecvInfo[i].ulCANID != 0 )
			{
				uiSumCanIDLength += g_CarbRecvInfo[i].uiCANIDLength;
				uiLastPos += g_CarbRecvInfo[i].uiCANTotalPacketLength;
			}
		}
	}

	if ( uiOrder > 0 )
	{
		*puiArrayIndex +=  g_CarbRecvInfo[uiOrder-1].uiCANDataSavePos;
		*puiArrayIndex += g_CarbRecvInfo[uiOrder-1].uiCANIDLength;
		*puiArrayIndex += g_CarbRecvInfo[uiOrder-1].uiCANRecvedLength;
#if defined(DEBUG_CAN_LOG)
		GITDebugPrintf("[%s] ulCANID 0x%04X, uiOrder %d, pos %d, *puiArrayIndex %d\r\n",
			__FUNCTION__, ulCANID, uiOrder, g_CarbRecvInfo[uiOrder-1].uiCANDataSavePos, *puiArrayIndex);
#endif
	}
	else
	{
		*puiArrayIndex += uiSumCanIDLength + uiLastPos;
	}

	return uiOrder;
}

void CAN_CarbRecvCallBack()	// 진단 통신에는 callback 안쓰는게 좋겠다 흐름이 안보여서 오류 찾기 힘들다
{
//	if ( (g_bCARBReceiving == TRUE) )
//	{
//		if ( g_uiCanReadMsgLength == g_stReadPassThruMsg.DataSize )
//		{
//#if defined(DEBUG_CAN_LOG)
//			GITDebugPrintf("\r\n[%s] g_iTimerCarbCallback %d, g_bCARBReceiving %d, pReadMsg->DataSize %d\r\n", __FUNCTION__, g_iTimerCarbCallback, g_bCARBReceiving, g_stReadPassThruMsg.DataSize);
//#endif
//			HalTimerStopSWTimer(g_iTimerCarbCallback);
//
//			CAN_ClearCarbCanRecvInfo();
//			g_bCARBReceiving = FALSE;
//
//			// PC로전달.
//			if (g_stReadPassThruMsg.DataSize != 0)
//				PassThruReadMsgs(&g_stReadPassThruMsg, g_uiCanReadMsgLength, g_InputCommType);
//			else	CANCOMM_SET_STATE(eCAN_COMM_RX_FAIL);
//
//			g_ulCanWrittenTick = 0; // consequtive frame의 마지막을 받고도 timeout 메시지가 전달이 되는 것을 막기 위해.
//			g_uiCanReadMsgLength = 0;
//			DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
//		}
//		else		// 길이가 다르면 어떻게 할껀지 에 대한 처리 추가
//		{
//			GITDebugPrintf("\r\n!~~~~~~~~~~~~~~~~~~~~~~~[%s] g_uiCanReadMsgLength %d, pReadMsg->DataSize %d diff ~~~~~~~~~~~~~~~~~~~~~~~~\r\n", __FUNCTION__, g_uiCanReadMsgLength, g_stReadPassThruMsg.DataSize);
//
//			HalTimerStopSWTimer(g_iTimerCarbCallback);
//
//			g_bCARBReceiving = FALSE;
//
//			g_ulCanWrittenTick = OemGetTmr()-g_stGITSetConfig.nP2Max;	//p2만큼 지났으니 p3min-p2max만큼만 더 기다려 보자 fine
//		}
//
//	}
}
//void CAN_TxAckMessage()
//{
//#if defined(DEBUG_CAN_LOG)
//	GITDebugPrintf("\r\n[%s]\r\n", __FUNCTION__);
//#endif
//
//	g_OutCanPacket.stNormalPacket.arrDataFields[0] = 0x02;
//	g_OutCanPacket.stNormalPacket.arrDataFields[1] = 0x3E;
//	g_OutCanPacket.stNormalPacket.arrDataFields[2] = 0x80;
//
//	CAN_WriteBuff((unsigned char*)&g_OutCanPacket, 0, 1, CAN_SINGLE_FRAME);
//}
////////////////////////////////////////////////////////////////////////////////
//Can't use this timer, if you want use must setting
//void CAN_uDelay(unsigned int uiDelay)
//{
//	if(uiDelay != 0)
//		Oem_GIT_uDelay(uiDelay);
//}

void CAN_mDelay(unsigned int uiDelay)
{
	if(uiDelay != 0)
		Oem_GIT_mDelay(uiDelay);
}

unsigned int CAN_WriteBuff(unsigned char* pBuff, unsigned int nCount, unsigned int uiCANChannel, int nCanFrameType)
{
	int nUDS_Data_Struct, uiWriteLen;
	//U8 uctempbuff[30];
	unsigned long uiProtocolID;
	uiProtocolID = VCI_GetPassThruProtocolID();
	//stCanPacket *pOutCanPacket = (stCanPacket*)pBuff;

	// compile 시, warning 제거를 위해서
	uiProtocolID = uiProtocolID;
/*	//미사용 로직
	if ( nCanFrameType == CAN_SINGLE_FRAME )
	{
		if( (uiProtocolID == ISO15765_SINGLE_PODS)||
			(uiProtocolID == ISO15765_EXCEPT)||
			(uiProtocolID == ISO14229_UDS)||			// KYC 20110926 HG IPM 프로토콜 관련 추가
			(uiProtocolID == ISO15765_SINGLE_SMK)||
			(uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE)||	//111013 LWH
			(uiProtocolID == ISO15765_ACU_SINGLE))      // 20150810 seo 0x2B 추가
		{
			for(U8 i=0;i<10;i++)
			{
				OemReadCanBuff(uctempbuff, NULL, NULL, 1);
				OemReadCanBuff(uctempbuff, NULL, NULL, 2);
			}
		}
		if( (uiProtocolID == ISO15765_CUBIS)&&
			(pOutCanPacket->stNormalPacket.arrDataFields[0]==0x02)&&
			(pOutCanPacket->stNormalPacket.arrDataFields[1]==0x01)&&
			(pOutCanPacket->stNormalPacket.arrDataFields[2]==0x00))
		{
			for(U8 i=0;i<10;i++)
				OemReadCanBuff(uctempbuff, NULL, NULL, 1);
		}
	}
*/

	if ( nCanFrameType == CAN_CONSECUTICE_FRAME )
	{
#ifndef CGW_SECURITY
		nUDS_Data_Struct = (g_stGITSetConfig.nEtc3&0x0100 == 0x0100 ? 1:0);
//		if ( (g_stECUSetConfig.nSTMinTx >= 0xF1) && (g_stECUSetConfig.nSTMinTx <= 0xF9) )
//			CAN_uDelay((g_stECUSetConfig.nSTMinTx & 0x0F)*100);	// uSec Delay : ISO15765-2
//		else
			CAN_mDelay(g_stECUSetConfig.nSTMinTx);

		if(g_stGITSetConfig.nEtc1 != 0)
		{
			if(nUDS_Data_Struct==1){}
		  	else 	CAN_mDelay(g_stGITSetConfig.nEtc1);
		}
#endif
	}
	else
	{
		nUDS_Data_Struct = (g_stGITSetConfig.nEtc3&0x0100 == 0x0100 ? 1:0);
		if(((g_stGITSetConfig.nEtc1>>4)==0x0F)&&(nUDS_Data_Struct == 1))	Oem_GIT_uDelay((g_stGITSetConfig.nEtc1&0x0F)*100);
		else if(g_stGITSetConfig.nEtc1 >=50)								CAN_mDelay(50);
		else if(g_stGITSetConfig.nEtc1 != 0)								CAN_mDelay(g_stGITSetConfig.nEtc1);
	}

	uiWriteLen = OemWriteCanBuff((unsigned char*)pBuff, 0, NULL, uiCANChannel);
	g_ulCanWrittenTick = OemGetTmr();

	return uiWriteLen;
}

void CAN_InitVariable(void)
{
	g_uiCanReadMsgLength 	= 0;
	g_uiCanWriteMsgLength 	= 0;
	g_ulCanWrittenTick 		= 0;
	//GITDebugPrintf("%s",__FUNCTION__);
	g_bCanRxConcequtiveFrame = FALSE;
	g_bCanRxPendingFrame = FALSE;
	g_bCanRxCarbFrame = FALSE;

	CAN_ClearCarbCanRecvInfo();
}


void CAN_SavePassThruWriteMsg(unsigned char *pData, unsigned int uiDataLen)
{
	uint8_t ucAutovinCANLine=0;
    GetAutolinkConfigProperty(eAutoLinkConfig_AutovinCANLine,(void*)&ucAutovinCANLine);

	if(ucAutovinCANLine == HIGHCAN3 && GetOBDState() == eOBD_GetAutoVIN)    memcpy(&g_stWritePassThruMsg_L, pData, uiDataLen);
	else                                                                    memcpy(&g_stWritePassThruMsg, pData, uiDataLen);

	g_uiCanWriteMsgLength = 0;
}

BOOL IsCanPassThruWriteMsgSaved(void)
{
	uint8_t ucAutovinCANLine=0;
    GetAutolinkConfigProperty(eAutoLinkConfig_AutovinCANLine,(void*)&ucAutovinCANLine);
	
#ifdef CGW_SECURITY
	if(ucAutovinCANLine == HIGHCAN3 && GetOBDState() == eOBD_GetAutoVIN)
	{
		if(g_stWritePassThruMsg_L.DataSize-3 > g_uiCanWriteMsgLength) return TRUE;
	}
	else
	{
		if(g_stWritePassThruMsg.DataSize-3 > g_uiCanWriteMsgLength)   return TRUE;
	}
#else
	if(ucAutovinCANLine == HIGHCAN3 && GetOBDState() == eOBD_GetAutoVIN)
	{
		if(g_stWritePassThruMsg_L.DataSize > g_uiCanWriteMsgLength)   return TRUE;
	}
	else 
	{
		if(g_stWritePassThruMsg.DataSize > g_uiCanWriteMsgLength)     return TRUE;
	}
#endif
	return FALSE;
}

eCanType CAN_FindCanPacket(unsigned char* pBuff, stCanPacket *pInCanPacket, BOOL *pbStandardCan)
{
	stQueue *pQueue;
	eCanType bReturn = eFAIL;
	stCanPacket CanPacketTmp;
	unsigned char ucTmp, i, *p;
	unsigned int uiExtendCanID;

	pQueue = (stQueue*)pBuff;
	if ( IsEmptyQueue(pQueue) ) return eFAIL;

	// SOF, EOF, Data Length Code를 이용해서 CRC체크 한다.
	// ST32F2XXX는 CRC가 수신되지 않는다.
	// 그래서 ST32F2XXX는 CAN 수신 패킷을 모두 저장한다.
	p = (unsigned char*)&CanPacketTmp;

	for ( i=0; i<sizeof(stCanPacket); i++ )
	{
		p[i] = pQueue->pData[(pQueue->uiFront+i)%pQueue->uiQueueSize];
	}

	if ( (CanPacketTmp.stNormalPacket.ucSOF == CAN_FRAME_SOF ) &&
		 (CanPacketTmp.stNormalPacket.ucIDE == CAN_FRAME_STANDARD_IDE ) &&
		 (CanPacketTmp.stNormalPacket.ucEOF == CAN_FRAME_EOF) )
	{
		// standard frame
		*pbStandardCan = TRUE;
		PopMultiDataQueue(pQueue, (unsigned char*)pInCanPacket, sizeof(stCanPacket));

		if(pInCanPacket->stNormalPacket.us11BitID < 0x0700)
		{
			bReturn = eVCAN;
			//VCAN_SET_COMM_STATE(eVCAN_RX_BLOCK);
			DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		}
		else
		{
			bReturn = eDCAN;
			DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		}

	}
	else if ( (CanPacketTmp.stExtendPacket.ucSOF == CAN_FRAME_SOF ) &&
		 	  (CanPacketTmp.stExtendPacket.ucIDE == CAN_FRAME_EXTEND_IDE ) &&
		 	  (CanPacketTmp.stExtendPacket.ucEOF == CAN_FRAME_EOF ) )
	{
		// Extend frame
		*pbStandardCan = FALSE;
		PopMultiDataQueue(pQueue, (unsigned char*)pInCanPacket, sizeof(stCanPacket));
		
		uiExtendCanID = ( CanPacketTmp.stExtendPacket.us11BitID << 18 ) | CanPacketTmp.stExtendPacket.us18BitID;
		
		if( ( uiExtendCanID >= 0x18DA0000 ) && ( uiExtendCanID <= 0x18DAFFFF ) )
			bReturn = eDCAN;
		else
			bReturn = eVCAN;
		
		DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
	}
	else
	{
		bReturn = eFAIL;
		// remove garbage data
		PopQueue(pQueue, &ucTmp);
		DCAN_SET_COMM_STATE(eCAN_RX_FAIL);
	}
//	if(bReturn == eVCAN)GITDebugPrintf("*");
//	if(bReturn == eDCAN)GITDebugPrintf("/");
#if defined(DEBUG_CAN_LOG)
		GITDebugPrintf("[FindCanPacket] %s CAN  %d\r\n", (*pbStandardCan) ? "Standard ":"Extended ",bReturn);
#endif

	return bReturn;
}

eCanType LCAN_FindCanPacket(unsigned char* pBuff, stCanPacket *pInCanPacket, BOOL *pbStandardCan)
{
	stQueue *pQueue;
	eCanType bReturn = eFAIL;
	stCanPacket CanPacketTmp;
	unsigned char ucTmp, i, *p;
	unsigned int uiExtendCanID;

	pQueue = (stQueue*)pBuff;
	if ( IsEmptyQueue(pQueue) ) return eFAIL;

	// SOF, EOF, Data Length Code를 이용해서 CRC체크 한다.
	// ST32F2XXX는 CRC가 수신되지 않는다.
	// 그래서 ST32F2XXX는 CAN 수신 패킷을 모두 저장한다.
	p = (unsigned char*)&CanPacketTmp;

	for ( i=0; i<sizeof(stCanPacket); i++ )
	{
		p[i] = pQueue->pData[(pQueue->uiFront+i)%pQueue->uiQueueSize];
	}

	if ( (CanPacketTmp.stNormalPacket.ucSOF == CAN_FRAME_SOF ) &&
		 (CanPacketTmp.stNormalPacket.ucIDE == CAN_FRAME_STANDARD_IDE ) &&
		 (CanPacketTmp.stNormalPacket.ucEOF == CAN_FRAME_EOF) )
	{
		// standard frame
		*pbStandardCan = TRUE;
		PopMultiDataQueue(pQueue, (unsigned char*)pInCanPacket, sizeof(stCanPacket));

		bReturn = eLCAN;
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
	}
	else if ( (CanPacketTmp.stExtendPacket.ucSOF == CAN_FRAME_SOF ) &&
		 	  (CanPacketTmp.stExtendPacket.ucIDE == CAN_FRAME_EXTEND_IDE ) &&
		 	  (CanPacketTmp.stExtendPacket.ucEOF == CAN_FRAME_EOF ) )
	{
		// Extend frame
		*pbStandardCan = FALSE;
		PopMultiDataQueue(pQueue, (unsigned char*)pInCanPacket, sizeof(stCanPacket));
		
		uiExtendCanID = ( CanPacketTmp.stExtendPacket.us11BitID << 18 ) | CanPacketTmp.stExtendPacket.us18BitID;
		
		if( ( uiExtendCanID >= 0x18DA0000 ) && ( uiExtendCanID <= 0x18DAFFFF ) )
			bReturn = eDCAN;
		else
		bReturn = eLCAN;
		
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
	}
	else
	{
		bReturn = eFAIL;
		// remove garbage data
		PopQueue(pQueue, &ucTmp);
//		LCAN_SET_COMM_STATE(eLCAN_RX_FAIL);
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!LCAN_FAIL!!!!!!!!!!!!!!!!!!!!!!!!\r\n");	//확인용
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);	//FAIL에 빠지면 다른곳으로 가기전까지 데이터 못받음
	}

//	  if(bReturn == eLCAN)GITDebugPrintf("-");
#if defined(DEBUG_CAN_LOG)
		GITDebugPrintf("[FindLCanPacket] %s CAN  %d\r\n", (*pbStandardCan) ? "Standard ":"Extended ",bReturn);
#endif

	return bReturn;
}

eCanType LCAN_FindCanPacket_FD(unsigned char* pBuff, stCanPacket *pInCanPacket, BOOL *pbStandardCan)
{
	stQueue *pQueue;
	eCanType bReturn = eFAIL;
	stCanPacket CanPacketTmp;
	unsigned char ucTmp, i, *p;

	pQueue = (stQueue*)pBuff;
	if ( IsEmptyQueue(pQueue) ) return eFAIL;

	// SOF, EOF, Data Length Code를 이용해서 CRC체크 한다.
	// ST32F2XXX는 CRC가 수신되지 않는다.
	// 그래서 ST32F2XXX는 CAN 수신 패킷을 모두 저장한다.
	p = (unsigned char*)&CanPacketTmp;

	for ( i=0; i<sizeof(stCanPacket); i++ )
	{
		p[i] = pQueue->pData[(pQueue->uiFront+i)%pQueue->uiQueueSize];
	}

	if ( (CanPacketTmp.stNormalPacket.ucSOF == CAN_FRAME_SOF ) &&
		 (CanPacketTmp.stNormalPacket.ucIDE == CAN_FRAME_STANDARD_IDE ) &&
		 (CanPacketTmp.stNormalPacket.ucEOF == CAN_FRAME_EOF) )
	{
		// standard frame
		*pbStandardCan = TRUE;
		PopMultiDataQueue(pQueue, (unsigned char*)pInCanPacket, sizeof(stCanPacket));


		if(pInCanPacket->stNormalPacket.us11BitID < 0x0700)
		{
			bReturn = eLCAN;
			//VCAN_SET_COMM_STATE(eVCAN_RX_BLOCK);
			//LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
		}
		else
		{
			bReturn = eDCAN;
			//DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		}
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
	}
	else if ( (CanPacketTmp.stExtendPacket.ucSOF == CAN_FRAME_SOF ) &&
			  (CanPacketTmp.stExtendPacket.ucIDE == CAN_FRAME_EXTEND_IDE ) &&
			  (CanPacketTmp.stExtendPacket.ucEOF == CAN_FRAME_EOF ) )
	{
		// Extend frame
		*pbStandardCan = FALSE;
		PopMultiDataQueue(pQueue, (unsigned char*)pInCanPacket, sizeof(stCanPacket));
		
		bReturn = eLCAN;
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
	}
	else
	{
		bReturn = eFAIL;
		// remove garbage data
		PopQueue(pQueue, &ucTmp);
//		LCAN_SET_COMM_STATE(eLCAN_RX_FAIL);
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!LCAN_FAIL 222222 !!!!!!!!!!!!!!!!!!!!!!!!\r\n");	//확인용
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);	//FAIL에 빠지면 다른곳으로 가기전까지 데이터 못받음
	}

//	  if(bReturn == eLCAN)GITDebugPrintf("-");
#if defined(DEBUG_CAN_LOG)
		GITDebugPrintf("[FindLCanPacket] %s CAN  %d\r\n", (*pbStandardCan) ? "Standard ":"Extended ",bReturn);
#endif

	return bReturn;
}


unsigned long CAN_GetRxP3MinTimeOutValue()
{
	unsigned long ulP3MinTimeout;

	if( g_bCanRxPendingFrame ) {
		ulP3MinTimeout = g_stGITSetConfig.nP3Max+500;
	}
	else if ( g_bCanRxCarbFrame ) {
		ulP3MinTimeout = g_stGITSetConfig.nP2Max;
	}
	else if ( g_bCanRxConcequtiveFrame ) {
		ulP3MinTimeout = 300;
	}
	else {
		ulP3MinTimeout = g_stGITSetConfig.nP3Min;
	}

//	if(GetOBDState() == eOBD_Sleep_Ready) {
//		if(g_bTripWaitingTime  == 1) {
//			ulP3MinTimeout = 30000;
//		}
//		else {
//			ulP3MinTimeout = 5000;
//		}
//
//		//GITDebugPrintf("\r\n[g_bTripWaitingTime : %d ] \r\n",g_bTripWaitingTime );
//	}

	return ulP3MinTimeout;
}

BOOL CAN_CheckP3MinTimeout(void)
{
	if ( g_ulCanWrittenTick != 0 )
	{
		unsigned long ulCurrentTmr, ulDiffTmr, ulTimeOut;
	 	ulCurrentTmr = OemGetTmr();
		ulDiffTmr 	= OemGetTmrDelta(ulCurrentTmr, g_ulCanWrittenTick);
		ulTimeOut = CAN_GetRxP3MinTimeOutValue();

		if ( ulDiffTmr > ulTimeOut )
		{
			//GITDebugPrintf("\r\n[%s] \r\n", __FUNCTION__);
			//GITDebugPrintf("CurTime %d, P3_MIN %d, P3_MAX %d TimeOut %d 0x%X[%04X %02X %02X %02X]\r\n", ulDiffTmr, ulTimeOut, g_stGITSetConfig.nP3Max,ulTimeOut,g_ulProtocolID,g_OutCanPacket.stNormalPacket.us11BitID,g_OutCanPacket.stNormalPacket.arrDataFields[0],g_OutCanPacket.stNormalPacket.arrDataFields[1],g_OutCanPacket.stNormalPacket.arrDataFields[2]);
			GITDebugPrintf("}");
			//GITDebugPrintf("TimeOut 0x%X[%04X %02X %02X %02X]\r\n", g_ulProtocolID, g_OutCanPacket.stNormalPacket.us11BitID,g_OutCanPacket.stNormalPacket.arrDataFields[0],g_OutCanPacket.stNormalPacket.arrDataFields[1],g_OutCanPacket.stNormalPacket.arrDataFields[2]);
			g_ulCanWrittenTick = 0;
			return TRUE;
		}
	}

	return FALSE;
}

void CAN_RxParsing(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	BOOL bStandardCan=FALSE;
	//BOOL bFindCan=FALSE;
	U8 ucCantype=0;
	//U8 uctempbuff[30];
	//static unsigned long ulSleepCntTmr=0, ulSleepDiffTmr=0;

	//unsigned long ulDiffRunningTmr=0;
	//진단이나 비클캔이 Tx중이면 Rx하지 않는다
	if ( (DCAN_GET_COMM_STATE() < eCAN_RX_SINGLE_FRAME) )
	{
		return;
	}

	ucCantype = CAN_FindCanPacket(pBuff, &g_InRxCanPacket, &bStandardCan);
	if ( ucCantype == eDCAN )
	{
		g_bCanDataFlag_Wakeup = true;
		switch( DCAN_GET_COMM_STATE() )
		{
			case eCAN_RX_BLOCK:
				CAN_RxBlockProc(&g_InRxCanPacket, bStandardCan, &g_stReadPassThruMsg);
				break;
			default:
				DCAN_SET_COMM_STATE(eCAN_NONE_STATE);
				//for(u8 i = 0; i < 10; i++)
				//	OemReadCanBuff(uctempbuff, NULL, NULL, 1);
				break;
		}
	}
	else if( ucCantype == eVCAN )	//비클캔
	{
		memset(&g_stReadPassThruMsg_V, 0x00, sizeof(g_stReadPassThruMsg_V));
		VCAN_RxBlockProc(&g_InRxCanPacket, bStandardCan, &g_stReadPassThruMsg_V);
	}
	else
	{
		if( (GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING) || (GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING) )	//체킹중에는 REQ없으므로 타임아웃도 없음(제어중에도 쏘기만하므로 타임아웃없음)
		{
			g_ulCanWrittenTick = 0;
		}
		else
		{
			if ( CAN_CheckP3MinTimeout() )
			{
				//CARB통신은 다받고난 후 타임아웃으로 처리되므로 여기서 데이터 처리해야함
				PassThruReadMsgs(&g_stReadPassThruMsg, g_uiCanReadMsgLength, g_InputCommType, bStandardCan);
				CAN_ClearCarbCanRecvInfo();
				g_bCARBReceiving = FALSE;
				g_uiCanReadMsgLength = 0;

				CAN_InitVariable();
				g_stReadPassThruMsg.DataSize = 0;
				DCAN_SET_COMM_STATE(eCAN_RX_FAIL);		//170412 LWH DB에 V CAN이 많은 경우 큐 OVERFLOW가 발생하는 경우가 있어서 RX_BLOCK으로 개선
			}
		}
	}
}

void CAN_RxParsing_Low(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	BOOL bStandardCan=FALSE;
	U8 ucCantype=0;
	//U8 uctempbuff[30];

	//Tx중이면 Rx하지 않는다
	if ( (LCAN_GET_COMM_STATE() < eLCAN_RX_SINGLE_FRAME) ) {
		return;
	}

	ucCantype = LCAN_FindCanPacket(pBuff, &g_InRxCanPacket, &bStandardCan);

	if( ucCantype == eDCAN)
	{
		g_bCanDataFlag_Wakeup = true;
		switch( LCAN_GET_COMM_STATE() )
		{
			case eLCAN_RX_BLOCK:
				LCAN_RxBlockProc(&g_InRxCanPacket, bStandardCan, &g_stReadPassThruMsg_V);
				break;
			default:
				LCAN_SET_COMM_STATE(eLCAN_NONE_STATE);
				//for(u8 i = 0; i < 10; i++)
				//	OemReadCanBuff(uctempbuff, NULL, NULL, 1);
				break;
		}
	}
	else if( ucCantype==eLCAN )
	{
		g_bCanDataFlag_Wakeup = true;
		g_uiCanStopTime = Get_Tmr();
		switch( LCAN_GET_COMM_STATE() )
		{
			case eLCAN_RX_BLOCK:
	  			memset(&g_stReadPassThruMsg_V, 0x00, sizeof(g_stReadPassThruMsg_V));
				LCAN_RxBlockProc(&g_InRxCanPacket, bStandardCan, &g_stReadPassThruMsg_V);
				break;
			default:
				LCAN_SET_COMM_STATE(eLCAN_NONE_STATE);
				//for(u8 i = 0; i < 10; i++)
				//	OemReadCanBuff(uctempbuff, NULL, NULL, 2);
				break;
		}
	}
	else
	{
		if( (GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING) || (GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING) )	//체킹중에는 REQ없으므로 타임아웃도 없음(제어중에도 쏘기만하므로 타임아웃없음)
		{
			g_ulCanWrittenTick = 0;
		}
		else
		{
			if ( CAN_CheckP3MinTimeout() )
			{
				//CARB통신은 다받고난 후 타임아웃으로 처리되므로 여기서 데이터 처리해야함
				PassThruReadMsgs(&g_stReadPassThruMsg_V, g_uiCanReadMsgLength, g_InputCommType, bStandardCan);
				CAN_ClearCarbCanRecvInfo();
				g_bCARBReceiving = FALSE;
				g_uiCanReadMsgLength = 0;

				CAN_InitVariable();
				g_stReadPassThruMsg_V.DataSize = 0;
				LCAN_SET_COMM_STATE(eLCAN_RX_FAIL);		//170412 LWH DB에 V CAN이 많은 경우 큐 OVERFLOW가 발생하는 경우가 있어서 RX_BLOCK으로 개선
			}
		}
	}
}

void CANFD_RxParsing(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	BOOL bStandardCan=FALSE;
	U8 ucCantype=0;
	//U8 uctempbuff[30];
	if ( (DCAN_GET_COMM_STATE() < eCAN_RX_SINGLE_FRAME) )
	{
		return;
	}

	ucCantype = CAN_FindCanPacket(pBuff, &g_InRxCanPacket, &bStandardCan);


	if ( ucCantype == eDCAN )// || ucCantype == eVCAN )	//진단캔
	{
		g_bCanDataFlag_Wakeup = true;

		//CAN_RxBlockProc(&g_InRxCanPackest, bStandardCan, &g_stReadPassThruMsg);

		//printf("[%s] id : %x, dlc : %d\n",__FUNCTION__,g_InRxCanPacket.stNormalPacket.us11BitID,g_InRxCanPacket.stNormalPacket.ucDLC);
		//hexdump(g_InRxCanPacket.stNormalPacket.arrDataFields, g_InRxCanPacket.stNormalPacket.ucDLC);


		FindCanFDResponse((eCanFDTxCmd)g_InRxCanPacket.stNormalPacket.us11BitID,g_InRxCanPacket.stNormalPacket.arrDataFields);
	}
	else if( ucCantype == eVCAN )	//비클캔
	{
		g_bCanDataFlag_Wakeup = true;

		//PassThruReadMsgs(&g_InRxCanPacket, bStandardCan, &g_stReadPassThruMsg_V);
	  	memset(&g_stReadPassThruMsg_V, 0x00, sizeof(g_stReadPassThruMsg_V));
		VCAN_RxBlockProc(&g_InRxCanPacket, bStandardCan, &g_stReadPassThruMsg_V);

		//printf("[%s] id : %x, dlc : %d\n",__FUNCTION__,g_InRxCanPacket.stNormalPacket.us11BitID,g_InRxCanPacket.stNormalPacket.ucDLC);
		//hexdump(g_InRxCanPacket.stNormalPacket.arrDataFields, g_InRxCanPacket.stNormalPacket.ucDLC);

	}
}

void CANFD_RxParsing_Low(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	BOOL bStandardCan=FALSE;
	U8 ucCantype=0;
	//U8 uctempbuff[30];
	if ( (LCAN_GET_COMM_STATE() < eLCAN_RX_SINGLE_FRAME) )
    //if ( (DCAN_GET_COMM_STATE() < eCAN_RX_SINGLE_FRAME) )
	{
		return;
	}

	//ucCantype = CAN_FindCanPacket(pBuff, &g_InRxCanPacket, &bStandardCan);
	ucCantype = LCAN_FindCanPacket_FD(pBuff, &g_InRxCanPacket, &bStandardCan);

	if ( ucCantype == eDCAN )// || ucCantype == eVCAN )	//진단캔
	{
		g_bCanDataFlag_Wakeup = true;

		//CAN_RxBlockProc(&g_InRxCanPackest, bStandardCan, &g_stReadPassThruMsg);

		//printf("[%s] id : %x, dlc : %d\n",__FUNCTION__,g_InRxCanPacket.stNormalPacket.us11BitID,g_InRxCanPacket.stNormalPacket.ucDLC);
		//hexdump(g_InRxCanPacket.stNormalPacket.arrDataFields, g_InRxCanPacket.stNormalPacket.ucDLC);

			FindCanFDResponse((eCanFDTxCmd)g_InRxCanPacket.stNormalPacket.us11BitID,g_InRxCanPacket.stNormalPacket.arrDataFields);
	}
	else if( ucCantype == eLCAN )	//비클캔
	{

		g_bCanDataFlag_Wakeup = true;
		g_uiCanStopTime = Get_Tmr();
		switch( LCAN_GET_COMM_STATE() )
		{
			case eLCAN_RX_BLOCK:
	  				memset(&g_stReadPassThruMsg_V, 0x00, sizeof(g_stReadPassThruMsg_V));
				LCAN_RxBlockProc(&g_InRxCanPacket, bStandardCan, &g_stReadPassThruMsg_V);
				break;
			default:
				LCAN_SET_COMM_STATE(eLCAN_NONE_STATE);
				//for(u8 i = 0; i < 10; i++)
				//	OemReadCanBuff(uctempbuff, NULL, NULL, 2);
				break;
		}
	}
}



BOOL CAN_RxBlockProc(stCanPacket *pInCanPacket, BOOL bIsStandardCan, stPASSTHRU_MSG *pReadMsg)
{
	eCanComRxTxState eCanRxState;
	bool bFineCanid = TRUE;
	//U8 uctempbuff[30];
	//unsigned long uiProtocolID;
	unsigned int uiExtendCanID;
	//uiProtocolID = VCI_GetPassThruProtocolID();
	if(bIsStandardCan == true)
	{
		if(pInCanPacket->stNormalPacket.us11BitID >= 0x0700 )
			eCanRxState = (eCanComRxTxState)((pInCanPacket->stNormalPacket.arrDataFields[0] >> 4 & 0x0F) + eCAN_RX_SINGLE_FRAME);
		else
			eCanRxState = eCAN_RX_SINGLE_FRAME;
	}
	else
	{
        uiExtendCanID = (pInCanPacket->stExtendPacket.us11BitID << 18) | (pInCanPacket->stExtendPacket.us18BitID);

		if( ( uiExtendCanID >= 0x18DA0000 ) && ( uiExtendCanID <= 0x18DAFFFF ) )
			eCanRxState = (eCanComRxTxState)((pInCanPacket->stExtendPacket.arrDataFields[0] >> 4 & 0x0F) + eCAN_RX_SINGLE_FRAME);
		else
			eCanRxState = eCAN_RX_SINGLE_FRAME;
	}

	bFineCanid=FALSE;

	//g_ulCanWrittenTick = 0; 			//150407 LWH vehicle CAN때문에 타임아웃이 꼬인다 여기로 옮김

	//GITDebugPrintf("|%d|",eCanRxState);
	switch ( eCanRxState )
	{
		case eCAN_RX_SINGLE_FRAME:
#if defined(DEBUG_CAN_LOG)
			printf("RX_SINGLE\r\n");
#endif
#if defined(CAN_RX_STATE_LOG)
			printf("RX_SINGLE\r\n");
#endif
			if((pInCanPacket->stNormalPacket.arrDataFields[1]==0x7F)&&(pInCanPacket->stNormalPacket.arrDataFields[3]==0x78))    //자동검사일때 동작성과 수동검사일때 동작성을 고려해야 한다
			{
				g_ulCanWrittenTick = OemGetTmr();		// pending 들어온 시점부터 타이머 재가동
				g_bCanRxPendingFrame = TRUE;
			}
            else if((pInCanPacket->stExtendPacket.arrDataFields[1]==0x7F)&&(pInCanPacket->stExtendPacket.arrDataFields[3]==0x78))    
			{
				g_ulCanWrittenTick = OemGetTmr();		// pending 들어온 시점부터 타이머 재가동
				g_bCanRxPendingFrame = TRUE;
			}
			else	//싱글프레임인데 펜딩이 아닌응답(정상 or 실패)
			{
				g_bCanRxPendingFrame = FALSE;
				g_bCanRxConcequtiveFrame = FALSE;
				CAN_MakeRxSingleFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
			}
			break;
		case eCAN_RX_FIRST_FRAME:
#if defined(DEBUG_CAN_LOG)
			printf("RX_FIRST\r\n");
#endif
#if defined(CAN_RX_STATE_LOG)
			printf("RX_FIRST\r\n");
#endif
			g_bCanRxPendingFrame = FALSE;
			g_bCanRxConcequtiveFrame = TRUE;
			CAN_MakeRxFirstFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
			break;
		case eCAN_RX_CONSECUTIVE_FRAME:
#if defined(DEBUG_CAN_LOG)
			printf("RX_CONSECUTIVE\r\n");
#endif
#if defined(CAN_RX_STATE_LOG)
			printf("RX_CONSECUTIVE\r\n");
#endif
			g_bCanRxConcequtiveFrame = TRUE;
			CAN_MakeRxConsecutiveFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
			break;
		case eCAN_RX_FLOWCONTROL_FRAME:
#if defined(DEBUG_CAN_LOG)
			printf("RX_FLOW\r\n");
#endif
#if defined(CAN_RX_STATE_LOG)
			printf("RX_FLOW\r\n");
#endif
			g_bCanRxPendingFrame = FALSE;
			g_bCanRxConcequtiveFrame = FALSE;
			CAN_MakeRxFlowControlFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
			break;

		default:
			DCAN_SET_COMM_STATE(eCAN_NONE_STATE);
			//OemReadCanBuff(uctempbuff, NULL, NULL, 1);
			break;
	}
	return bFineCanid;
}

BOOL VCAN_RxBlockProc(stCanPacket *pInCanPacket, BOOL bIsStandardCan, stPASSTHRU_MSG *pReadMsg)
{
	bool bFineCanid = TRUE;
	unsigned int uiCopyLen, uiCopyIndex, uiCanIDLen, uiArrayIndex = 0, uiVehicleCanReadMsgLength=0;
	unsigned char *pCanData;

	if(bIsStandardCan == true)
	{
		uiCanIDLen = 2;
		
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID >> 8;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID;

		uiCopyLen = pInCanPacket->stNormalPacket.ucDLC;
		uiCopyIndex = 0;

		pCanData = &pInCanPacket->stNormalPacket.arrDataFields[uiCopyIndex];

		pReadMsg->DataSize= 0;
		pReadMsg->DataSize += uiCanIDLen;
		memcpy(pReadMsg->pData+uiArrayIndex, pCanData, uiCopyLen);
		pReadMsg->DataSize += uiCopyLen;

		uiVehicleCanReadMsgLength += uiCanIDLen;
		uiVehicleCanReadMsgLength += uiCopyLen;

		if(CFD_GetCanFDAdapter())
		{
		  	pReadMsg->RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stNormalPacket.ucDummy2;
		}
		else
		{
		  	pReadMsg->RxStatus = g_InRxCanPacket.stNormalPacket.ucDummy2;
		}
	}
	else
	{
		uiCanIDLen = 4;
		
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stExtendPacket.us11BitID>>6;
		pReadMsg->pData[uiArrayIndex]   = (pInCanPacket->stExtendPacket.us11BitID & 0x3F)<<2;
		pReadMsg->pData[uiArrayIndex++] += (pInCanPacket->stExtendPacket.us18BitID& 0x30000)>>16;
		pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF00)>>8;
		pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF);

		uiCopyLen = pInCanPacket->stExtendPacket.ucDLC;
		uiCopyIndex = 0;

		pCanData = &pInCanPacket->stExtendPacket.arrDataFields[uiCopyIndex];

		pReadMsg->DataSize= 0;
		pReadMsg->DataSize += uiCanIDLen;
		memcpy(pReadMsg->pData+uiArrayIndex, pCanData, uiCopyLen);
		pReadMsg->DataSize += uiCopyLen;

		uiVehicleCanReadMsgLength += uiCanIDLen;
		uiVehicleCanReadMsgLength += uiCopyLen;

		if(CFD_GetCanFDAdapter())
		{
		  	pReadMsg->RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stExtendPacket.ucDummy1;
		}
		else
		{
			if( bIsStandardCan == true )	pReadMsg->RxStatus = g_InRxCanPacket.stNormalPacket.ucDummy2;
			else					  					pReadMsg->RxStatus = g_InRxCanPacket.stExtendPacket.ucDummy1;
		}
	}
	PassThruReadMsgs(pReadMsg, uiVehicleCanReadMsgLength, g_InputCommType, bIsStandardCan);
	memset(pReadMsg, 0x00, sizeof(stPASSTHRU_MSG));
	pReadMsg->DataSize = 0;
	return bFineCanid;
}

#if 1
BOOL LCAN_RxBlockProc(stCanPacket *pInCanPacket, BOOL bIsStandardCan, stPASSTHRU_MSG *pReadMsg)
{
	//eCanComRxTxState eCanRxState;
	bool bFineCanid = TRUE;
	//U8 uctempbuff[30];
	//unsigned long uiProtocolID;
	unsigned int uiCopyLen, uiCopyIndex, uiCanIDLen, uiArrayIndex = 0, uiVehicleCanReadMsgLength=0;
	unsigned char *pCanData;
	unsigned int uiExtendCanID;
	eCanComRxTxState eCanRxState;

	//uiProtocolID = VCI_GetPassThruProtocolID();

	//eCanRxState = (eCanComRxTxState)((pInCanPacket->stNormalPacket.arrDataFields[0] >> 4 & 0x0F) + eLCAN_RX_SINGLE_FRAME);

	stPASSTHRU_MSG pReadVehicleMsg;
	memset(&pReadVehicleMsg, 0x00, sizeof(pReadVehicleMsg));

	if( bIsStandardCan == true)
	{
		uiCanIDLen = 2;
		pReadVehicleMsg.pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID >> 8;
		pReadVehicleMsg.pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID;

		uiCopyIndex=0;
		uiCopyLen=8;

		pCanData = &pInCanPacket->stNormalPacket.arrDataFields[uiCopyIndex];


		pReadVehicleMsg.DataSize = 0;
		pReadVehicleMsg.DataSize += uiCanIDLen;
		memcpy(pReadVehicleMsg.pData+uiArrayIndex, pCanData, uiCopyLen);
		pReadVehicleMsg.DataSize += uiCopyLen;

		uiVehicleCanReadMsgLength += uiCanIDLen;
		uiVehicleCanReadMsgLength += uiCopyLen;

		//GitCANFDReadMsgs(pReadMsg, uiReadMsgLen, eInCommType);

		if(CFD_GetCanFDAdapter())
		{
		  	pReadVehicleMsg.RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stNormalPacket.ucDummy2;
		}
		else
		{
		  	pReadVehicleMsg.RxStatus = g_InRxCanPacket.stNormalPacket.ucDummy2;
		}
        
        Git_LCANReadMsgs((stPASSTHRU_MSG*)&pReadVehicleMsg, uiVehicleCanReadMsgLength, 0, bIsStandardCan);
        LCAN_SET_COMM_STATE(eLCAN_RX_DONE);
        memset((stPASSTHRU_MSG*)&pReadVehicleMsg, 0x00, sizeof(stPASSTHRU_MSG));
        pReadVehicleMsg.DataSize = 0;
	}
	else
	{

		uiExtendCanID = (pInCanPacket->stExtendPacket.us11BitID << 18) | (pInCanPacket->stExtendPacket.us18BitID);

		if( ( uiExtendCanID >= 0x18DA0000 ) && ( uiExtendCanID <= 0x18DAFFFF ) )
			eCanRxState = (eCanComRxTxState)((pInCanPacket->stExtendPacket.arrDataFields[0] >> 4 & 0x0F) + eLCAN_RX_SINGLE_FRAME);
		else
			eCanRxState = (eCanComRxTxState)eLCAN_RX_SINGLE_FRAME;
		
		switch ( eCanRxState )
		{
			case eLCAN_RX_SINGLE_FRAME:

				if((pInCanPacket->stNormalPacket.arrDataFields[1]==0x7F)&&(pInCanPacket->stNormalPacket.arrDataFields[3]==0x78))	//자동검사일때 동작성과 수동검사일때 동작성을 고려해야 한다
				{
					g_ulCanWrittenTick = OemGetTmr();		// pending 들어온 시점부터 타이머 재가동
					g_bCanRxPendingFrame = TRUE;
				}
				else if((pInCanPacket->stExtendPacket.arrDataFields[1]==0x7F)&&(pInCanPacket->stExtendPacket.arrDataFields[3]==0x78))	 
				{
					g_ulCanWrittenTick = OemGetTmr();		// pending 들어온 시점부터 타이머 재가동
					g_bCanRxPendingFrame = TRUE;
				}
				else	//싱글프레임인데 펜딩이 아닌응답(정상 or 실패)
				{
					g_bCanRxPendingFrame = FALSE;
					g_bCanRxConcequtiveFrame = FALSE;
					//CAN_MakeRxSingleFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
					
					uiCanIDLen = 4;
					
					pReadVehicleMsg.pData[uiArrayIndex++] = pInCanPacket->stExtendPacket.us11BitID>>6;
					pReadVehicleMsg.pData[uiArrayIndex]   = (pInCanPacket->stExtendPacket.us11BitID & 0x3F)<<2;
					pReadVehicleMsg.pData[uiArrayIndex++] += (pInCanPacket->stExtendPacket.us18BitID& 0x30000)>>16;
					pReadVehicleMsg.pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF00)>>8;
					pReadVehicleMsg.pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF);
					
					uiCopyLen = pInCanPacket->stExtendPacket.ucDLC;
					uiCopyIndex = 0;
					
					pCanData = &pInCanPacket->stExtendPacket.arrDataFields[uiCopyIndex];
					
					pReadVehicleMsg.DataSize= 0;
					pReadVehicleMsg.DataSize += uiCanIDLen;
					memcpy(pReadVehicleMsg.pData+uiArrayIndex, pCanData, uiCopyLen);
					pReadVehicleMsg.DataSize += uiCopyLen;
					
					uiVehicleCanReadMsgLength += uiCanIDLen;
					uiVehicleCanReadMsgLength += uiCopyLen;
					
					if(CFD_GetCanFDAdapter())
					{
						pReadVehicleMsg.RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stExtendPacket.ucDummy1;
					}
					else
					{
						if( bIsStandardCan == true )	pReadVehicleMsg.RxStatus = g_InRxCanPacket.stNormalPacket.ucDummy2;
						else							pReadVehicleMsg.RxStatus = g_InRxCanPacket.stExtendPacket.ucDummy1;
					}
				}
				
				Git_LCANReadMsgs((stPASSTHRU_MSG*)&pReadVehicleMsg, uiVehicleCanReadMsgLength, 0, bIsStandardCan);
				LCAN_SET_COMM_STATE(eLCAN_RX_DONE);
				memset((stPASSTHRU_MSG*)&pReadVehicleMsg, 0x00, sizeof(stPASSTHRU_MSG));
				pReadVehicleMsg.DataSize = 0;
				
				break;
			case eLCAN_RX_FIRST_FRAME:

				printf("RX_FIRST\r\n");
				
				g_bCanRxPendingFrame = FALSE;
				g_bCanRxConcequtiveFrame = TRUE;
				CAN_MakeRxFirstFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
				break;
			case eLCAN_RX_CONSECUTIVE_FRAME:

				printf("RX_CONSECUTIVE\r\n");

				g_bCanRxConcequtiveFrame = TRUE;
				CAN_MakeRxConsecutiveFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
				break;
			case eLCAN_RX_FLOWCONTROL_FRAME:

				printf("RX_FLOW\r\n");

				g_bCanRxPendingFrame = FALSE;
				g_bCanRxConcequtiveFrame = FALSE;
				CAN_MakeRxFlowControlFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
				break;
	
			default:
				LCAN_SET_COMM_STATE(eLCAN_NONE_STATE);
				//OemReadCanBuff(uctempbuff, NULL, NULL, 1);
				break;
		}
	
	}


	return bFineCanid;
}
#else
BOOL LCAN_RxBlockProc(stCanPacket *pInCanPacket, BOOL bIsStandardCan, stPASSTHRU_MSG *pReadMsg)
{
	//eCanComRxTxState eCanRxState;
	bool bFineCanid = TRUE;
	//U8 uctempbuff[30];
	//unsigned long uiProtocolID;
	unsigned int uiCopyLen, uiCopyIndex, uiCanIDLen, uiArrayIndex = 0, uiVehicleCanReadMsgLength=0;
	unsigned char *pCanData;

	//uiProtocolID = VCI_GetPassThruProtocolID();

	//eCanRxState = (eCanComRxTxState)((pInCanPacket->stNormalPacket.arrDataFields[0] >> 4 & 0x0F) + eLCAN_RX_SINGLE_FRAME);

	stPASSTHRU_MSG pReadVehicleMsg;
	memset(&pReadVehicleMsg, 0x00, sizeof(pReadVehicleMsg));

	if( bIsStandardCan == true)
	{
		uiCanIDLen = 2;
		pReadVehicleMsg.pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID >> 8;
		pReadVehicleMsg.pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID;

		uiCopyIndex=0;
		uiCopyLen=8;

		pCanData = &pInCanPacket->stNormalPacket.arrDataFields[uiCopyIndex];


		pReadVehicleMsg.DataSize = 0;
		pReadVehicleMsg.DataSize += uiCanIDLen;
		memcpy(pReadVehicleMsg.pData+uiArrayIndex, pCanData, uiCopyLen);
		pReadVehicleMsg.DataSize += uiCopyLen;

		uiVehicleCanReadMsgLength += uiCanIDLen;
		uiVehicleCanReadMsgLength += uiCopyLen;

		  	//GitCANFDReadMsgs(pReadMsg, uiReadMsgLen, eInCommType);

		if(CFD_GetCanFDAdapter())
		{
		  	pReadVehicleMsg.RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stNormalPacket.ucDummy2;
		}
		else
		{
		  	pReadVehicleMsg.RxStatus = g_InRxCanPacket.stNormalPacket.ucDummy2;
		}
	}
	else
	{
		uiCanIDLen = 4;
		
		pReadVehicleMsg.pData[uiArrayIndex++] = pInCanPacket->stExtendPacket.us11BitID>>6;
		pReadVehicleMsg.pData[uiArrayIndex]   = (pInCanPacket->stExtendPacket.us11BitID & 0x3F)<<2;
		pReadVehicleMsg.pData[uiArrayIndex++] += (pInCanPacket->stExtendPacket.us18BitID& 0x30000)>>16;
		pReadVehicleMsg.pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF00)>>8;
		pReadVehicleMsg.pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF);

		uiCopyLen = pInCanPacket->stExtendPacket.ucDLC;
		uiCopyIndex = 0;

		pCanData = &pInCanPacket->stExtendPacket.arrDataFields[uiCopyIndex];

		pReadVehicleMsg.DataSize= 0;
		pReadVehicleMsg.DataSize += uiCanIDLen;
		memcpy(pReadVehicleMsg.pData+uiArrayIndex, pCanData, uiCopyLen);
		pReadVehicleMsg.DataSize += uiCopyLen;

		uiVehicleCanReadMsgLength += uiCanIDLen;
		uiVehicleCanReadMsgLength += uiCopyLen;

		if(CFD_GetCanFDAdapter())
		{
		  	pReadVehicleMsg.RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stExtendPacket.ucDummy1;
		}
		else
		{
			if( bIsStandardCan == true )	pReadVehicleMsg.RxStatus = g_InRxCanPacket.stNormalPacket.ucDummy2;
			else					  		pReadVehicleMsg.RxStatus = g_InRxCanPacket.stExtendPacket.ucDummy1;
		}
	}

	Git_LCANReadMsgs((stPASSTHRU_MSG*)&pReadVehicleMsg, uiVehicleCanReadMsgLength, 0, bIsStandardCan);
	LCAN_SET_COMM_STATE(eLCAN_RX_DONE);
	memset((stPASSTHRU_MSG*)&pReadVehicleMsg, 0x00, sizeof(stPASSTHRU_MSG));
	pReadVehicleMsg.DataSize = 0;
	return bFineCanid;
}
#endif
void CAN_TxParsing(stCanPacket *pOutCanPacket, stPASSTHRU_MSG *pWriteMsg, BOOL *pbStandardCAN, eCanType eCurrCanType)
{
	unsigned long uiProtocolID;

	uiProtocolID = VCI_GetPassThruProtocolID();

	if ( (uiProtocolID == ISO15765_29BIT		) ||
		 (uiProtocolID == ISO15765_29BIT_EXCEPT ) || 
		 (uiProtocolID == ISO15765_CARB_29BIT	) )
	{
		/************************************************************************************/
		/* Extended Can											                        	*/
		/*		 0	|  1   |  2   |  3	 |	4	|  5  |   6   |  n-1  | n					*/
		/*		ID1 |  ID2 |  ID3 | ID4  |Data1 |Data2| Data2 |Datan-1|Datan				*/
		/************************************************************************************/
		pOutCanPacket->stExtendPacket.ucSOF 			= CAN_FRAME_SOF;
		pOutCanPacket->stExtendPacket.ucSRR 			= 1;
		pOutCanPacket->stExtendPacket.ucIDE 			= CAN_FRAME_EXTEND_IDE;
		pOutCanPacket->stExtendPacket.ucRTR 			= 0;
		pOutCanPacket->stExtendPacket.ucReserved		= 0;
		pOutCanPacket->stExtendPacket.usCRC 			= 0;
		pOutCanPacket->stExtendPacket.ucCRCDelimiter	= 1;
		pOutCanPacket->stExtendPacket.ucACK 			= 1;
		pOutCanPacket->stExtendPacket.ucACKDelimiter	= 1;
		pOutCanPacket->stExtendPacket.ucEOF 			= CAN_FRAME_EOF;
		pOutCanPacket->stExtendPacket.us11BitID 		= ((pWriteMsg->pData[0])<<6)  + (pWriteMsg->pData[1] >> 2);
		pOutCanPacket->stExtendPacket.us18BitID 		= ((pWriteMsg->pData[1]&0x03)<<16) + (pWriteMsg->pData[2] << 8) + (pWriteMsg->pData[3]);

		if ( (pWriteMsg->pData[4] > 7) )
		{
			pOutCanPacket->stExtendPacket.ucDLC = ((pWriteMsg->pData[4]&0x0F)<<4) + pWriteMsg->pData[5];
			//DCAN_SET_COMM_STATE(eCAN_TX_FIRST_FRAME_29BIT);
            if(eCurrCanType == eLCAN)  LCAN_SET_COMM_STATE(eLCAN_TX_FIRST_FRAME);
            else                       DCAN_SET_COMM_STATE(eCAN_TX_FIRST_FRAME);
		}
		else
		{
			pOutCanPacket->stExtendPacket.ucDLC = pWriteMsg->pData[4]+1;
			//DCAN_SET_COMM_STATE(eCAN_TX_SINGLE_FRAME_29BIT);
            if(eCurrCanType == eLCAN)  LCAN_SET_COMM_STATE(eLCAN_TX_SINGLE_FRAME);
			else                       DCAN_SET_COMM_STATE(eCAN_TX_SINGLE_FRAME);
		}

		memcpy(pOutCanPacket->stExtendPacket.arrDataFields, pWriteMsg->pData+4, pOutCanPacket->stExtendPacket.ucDLC);

		*pbStandardCAN = FALSE;
	}
	else
	{	unsigned short usDLCLength;
		/************************************************************************************/
		/* Standard Can                         											*/
		/*		 0	|  1   |  2   |  3	 |	4	|  5  |  6	|  n-1  | n						*/
		/*		DLC |  ID1 |  ID2 | Data0| Data1|Data2|Data3|Datan-1|Datan					*/
		/************************************************************************************/
		pOutCanPacket->stNormalPacket.ucSOF 			= CAN_FRAME_SOF;
		pOutCanPacket->stNormalPacket.ucIDE 			= CAN_FRAME_STANDARD_IDE;
		pOutCanPacket->stNormalPacket.ucRTR 			= 0;
		pOutCanPacket->stNormalPacket.ucReserved 		= 0;
		pOutCanPacket->stNormalPacket.usCRC 			= 0;
		pOutCanPacket->stNormalPacket.ucCRCDelimiter 	= 1;
		pOutCanPacket->stNormalPacket.ucACK 			= 1;
		pOutCanPacket->stNormalPacket.ucACKDelimiter 	= 1;
		pOutCanPacket->stNormalPacket.ucEOF 			= CAN_FRAME_EOF;
		pOutCanPacket->stNormalPacket.us11BitID 		= ((pWriteMsg->pData[0]&0x07)<<8)  + pWriteMsg->pData[1];

		if(eCurrCanType == eDCAN)
		{
			if( pOutCanPacket->stNormalPacket.us11BitID >= 0x0700 )
			{
				if ( CAN_CheckStandardTxFrameType(&usDLCLength, pWriteMsg) )
					DCAN_SET_COMM_STATE(eCAN_TX_FIRST_FRAME);
				else
					DCAN_SET_COMM_STATE(eCAN_TX_SINGLE_FRAME);
			}
			else
				DCAN_SET_COMM_STATE(eCAN_TX_SINGLE_FRAME);
		}
		else if(eCurrCanType == eLCAN)
		{
				LCAN_SET_COMM_STATE(eLCAN_TX_SINGLE_FRAME);
		}

		*pbStandardCAN = TRUE;
	}
}

BOOL CAN_CheckStandardTxFrameType(unsigned short *pusDLCLength, stPASSTHRU_MSG *pWriteMsg)
{
	BOOL bIsMultiFrame, bLengthBackWard = FALSE;
	unsigned char ucCanMultiCheckLen;
	unsigned long uiProtocolID;
	unsigned short usCalcDLCLength;

	uiProtocolID = VCI_GetPassThruProtocolID();

	// compile 시, warning 제거를 위해서
	uiProtocolID = uiProtocolID;

	ucCanMultiCheckLen = 7;

	bIsMultiFrame = (pWriteMsg->pData[2] > ucCanMultiCheckLen) ? TRUE:FALSE;

	if ( bIsMultiFrame )
	{
		usCalcDLCLength = CAN_FRAME_DATA_SIZE;

		if ( bLengthBackWard )
			usCalcDLCLength = ((pWriteMsg->pData[3]&0x0F)<<4) + pWriteMsg->pData[2];
		else
			usCalcDLCLength = ((pWriteMsg->pData[2]&0x0F)<<4) + pWriteMsg->pData[3];
	}
	else
	{
		usCalcDLCLength = pWriteMsg->pData[2];
	}

	*pusDLCLength = usCalcDLCLength;

	return bIsMultiFrame;
}

unsigned int CAN_TxBlockProc(void)
{
	static BOOL bStandardCan;
	unsigned short usSendLength = 0;

	if((DCAN_GET_COMM_STATE() > eCAN_TX_FLOWCONTROL_FRAME)) {
		return 0;
	}

	//g_bCanRxConcequtiveFrame = FALSE;
//	g_usTxCanID = g_OutCanPacket.stNormalPacket.us11BitID;

	switch ( DCAN_GET_COMM_STATE()  )
	{
		case eCAN_TX_NONE_PARSING:
			if ( IsCanPassThruWriteMsgSaved() )
			{
				g_bCanRxConcequtiveFrame = FALSE;
				g_bCanRxCarbFrame = FALSE;
				//printf("CAN: [%s] eCAN_TX_NONE_PARSING\r\n", __FUNCTION__);
#if defined(DEBUG_CAN_LOG)
				printf("TX\r\n");
#endif
				memset(&g_OutCanPacket, 0x00, sizeof(stCanPacket));
				// 저장된 상태에서 파싱은 되지 않은 상태, 파싱 후 can controller에 보내야함.
				CAN_TxParsing(&g_OutCanPacket, &g_stWritePassThruMsg, &bStandardCan, eDCAN);
			}
			break;

		case eCAN_TX_SINGLE_FRAME:
#if defined(DEBUG_CAN_LOG)
			GITDebugPrintf("[%s] eCAN_TX_SINGLE_FRAME\r\n", __FUNCTION__);
#endif
			usSendLength = CAN_MakeTxSingleFrame(&g_OutCanPacket, bStandardCan, &g_stWritePassThruMsg, eDCAN);
			break;

		case eCAN_TX_FIRST_FRAME:
#if defined(DEBUG_CAN_LOG)
			GITDebugPrintf("[%s] eCAN_TX_FIRST_FRAME\r\n", __FUNCTION__);
#endif
			g_ucCanExceptState = 0;
			usSendLength = CAN_MakeTxFirstFrame(&g_OutCanPacket, bStandardCan, g_eCanCommState, &g_stWritePassThruMsg, eDCAN);
			break;

		case eCAN_TX_FLOWCONTROL_FRAME:
#if defined(DEBUG_CAN_LOG)
			GITDebugPrintf("[%s] eCAN_TX_FLOWCONTROL_FRAME\r\n", __FUNCTION__);
#endif
			CAN_MakeTxFlowControlFrame(&g_OutCanPacket, bStandardCan, g_eCanCommState, &g_stWritePassThruMsg, eDCAN);
			break;

		case eCAN_TX_CONSECUTIVE_FRAME:
#if defined(DEBUG_CAN_LOG)
			GITDebugPrintf("[%s] eCAN_TX_CONSECUTIVE_FRAME\r\n", __FUNCTION__);
#endif
			usSendLength = CAN_MakeTxConsecutiveFrame(&g_OutCanPacket, bStandardCan, g_eCanCommState, &g_stWritePassThruMsg, eDCAN);
			break;

		default:
			break;
	}

	return usSendLength;
}

//unsigned int VCAN_TxBlockProc(void)
//{
//	static BOOL bStandardCan;
//	unsigned short usSendLength = 0;
//
//	if((VCAN_GET_COMM_STATE() > eVCAN_TX_FLOWCONTROL_FRAME)) {
//		return 0;
//	}
//
//	switch ( VCAN_GET_COMM_STATE()  )
//	{
//		case eVCAN_TX_NONE_PARSING:
//			printf("CAN: [%s] eVCAN_TX_NONE_PARSING\r\n", __FUNCTION__);
//			memset(&g_OutVCanPacket, 0x00, sizeof(stCanPacket));
//			// 저장된 상태에서 파싱은 되지 않은 상태, 파싱 후 can controller에 보내야함.
//			CAN_TxParsing(&g_OutVCanPacket, &g_stWritePassThruMsg_V, &bStandardCan, eVCAN);
//			break;
//
//		case eVCAN_TX_SINGLE_FRAME:
//#if defined(DEBUG_CAN_LOG)
//			GITDebugPrintf("[%s] eVCAN_TX_SINGLE_FRAME\r\n", __FUNCTION__);
//#endif
//			usSendLength = CAN_MakeTxSingleFrame(&g_OutVCanPacket, bStandardCan, &g_stWritePassThruMsg_V, eVCAN);
//			break;
//
//		default:
//			break;
//	}
//
//	return usSendLength;
//}

unsigned int LCAN_TxBlockProc(void)
{
	static BOOL bStandardCan;
	unsigned short usSendLength = 0;

	if((LCAN_GET_COMM_STATE() > eLCAN_TX_FLOWCONTROL_FRAME)) {
		return 0;
	}

	switch ( LCAN_GET_COMM_STATE()  )
	{
		case eLCAN_TX_NONE_PARSING:
			if ( IsCanPassThruWriteMsgSaved() )
			{
				g_bCanRxConcequtiveFrame = FALSE;
				g_bCanRxCarbFrame = FALSE;
#if defined(DEBUG_CAN_LOG)
				printf("TX\r\n");
#endif
				memset(&g_OutLCanPacket, 0x00, sizeof(stCanPacket));
			// 저장된 상태에서 파싱은 되지 않은 상태, 파싱 후 can controller에 보내야함.
				CAN_TxParsing(&g_OutLCanPacket, &g_stWritePassThruMsg_L, &bStandardCan, eLCAN);
			}
			break;

		case eLCAN_TX_SINGLE_FRAME:
#if defined(DEBUG_CAN_LOG)
			GITDebugPrintf("[%s] eLCAN_TX_SINGLE_FRAME\r\n", __FUNCTION__);
#endif
			usSendLength = CAN_MakeTxSingleFrame(&g_OutLCanPacket, bStandardCan, &g_stWritePassThruMsg_L, eLCAN);
			break;

		case eLCAN_TX_FIRST_FRAME:
#if defined(DEBUG_CAN_LOG)
			GITDebugPrintf("[%s] eLCAN_TX_FIRST_FRAME\r\n", __FUNCTION__);
#endif
			g_ucCanExceptState = 0;
			usSendLength = CAN_MakeTxFirstFrame(&g_OutLCanPacket, bStandardCan, (eCanComRxTxState)g_eLCanCommState, &g_stWritePassThruMsg_L, eLCAN);
			break;

		case eLCAN_TX_FLOWCONTROL_FRAME:
#if defined(DEBUG_CAN_LOG)
			GITDebugPrintf("[%s] eLCAN_TX_FLOWCONTROL_FRAME\r\n", __FUNCTION__);
#endif
			CAN_MakeTxFlowControlFrame(&g_OutLCanPacket, bStandardCan, (eCanComRxTxState)g_eLCanCommState, &g_stWritePassThruMsg_L, eLCAN);
			break;

		case eLCAN_TX_CONSECUTIVE_FRAME:
#if defined(DEBUG_CAN_LOG)
			GITDebugPrintf("[%s] eLCAN_TX_CONSECUTIVE_FRAME\r\n", __FUNCTION__);
#endif
			usSendLength = CAN_MakeTxConsecutiveFrame(&g_OutLCanPacket, bStandardCan, (eCanComRxTxState)g_eLCanCommState, &g_stWritePassThruMsg_L, eLCAN);
			break;

	default:
			break;
	}

	return usSendLength;
}

void CAN_MakeSendFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, int nCanFrameType, stPASSTHRU_MSG *pWriteMsg)	//tx single, first, consecutive
{
	unsigned char *pCanDataFields;
	unsigned short usCanDLC;
	unsigned long uiProtocolID, ulDataIndex, nUDS_Data_Struct;

	uiProtocolID = VCI_GetPassThruProtocolID();

	// compile 시, warning 제거를 위해서
	uiProtocolID = VCI_GetPassThruProtocolID();;

	nUDS_Data_Struct = (g_stGITSetConfig.nEtc3&0x0100 == 0x0100 ? 1:0);

	if ( bIsStandardCan )
	{
		ulDataIndex = 2/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;
		pCanDataFields = pOutCanPacket->stNormalPacket.arrDataFields;
		//usCanDLC = pOutCanPacket->stNormalPacket.ucDLC;	//150914 lwh 불필요 해보인다 삭제
		if ( nUDS_Data_Struct == 1)
			memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x55, SIZE_CAN_DATA_FIELD);
		else
			memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x00, SIZE_CAN_DATA_FIELD);
	}
	else
	{
		if( uiProtocolID == ISO15765_CARB_29BIT )
			ulDataIndex = 2/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;
		else
			ulDataIndex = 4/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;

		pCanDataFields = pOutCanPacket->stExtendPacket.arrDataFields;
		//usCanDLC = pOutCanPacket->stExtendPacket.ucDLC;
		memset(pCanDataFields, 0x00, SIZE_CAN_DATA_FIELD);
	}

	if ( nCanFrameType == CAN_SINGLE_FRAME )
	{
	 	if(uiProtocolID == ISO15765_SINGLE || uiProtocolID == ISO15765_SINGLE_PODS || uiProtocolID == ISO15765_ACU_SINGLE)
		{
			usCanDLC = CAN_FRAME_DATA_SIZE;
			memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, CAN_FRAME_DATA_SIZE);
		}
		if(uiProtocolID == ISO15765_SINGLE_SMK)
		{
			usCanDLC = pWriteMsg->pData[2];
			memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex, CAN_FRAME_DATA_SIZE);
		}
		else if(uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE)
		{
			usCanDLC = pWriteMsg->pData[2];
			memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, CAN_FRAME_DATA_SIZE);
		}
		else if(uiProtocolID == ISO15765_29BIT)
		{
			if( FUELTYPE_GET_STATE() == ELECTRONIC )
			{
				usCanDLC = CAN_FRAME_DATA_SIZE;
				memset(pCanDataFields,0x00,CAN_FRAME_DATA_SIZE);
				memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, CAN_FRAME_DATA_SIZE);
			}
			else
			{
			usCanDLC = pOutCanPacket->stExtendPacket.ucDLC;
				memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, CAN_FRAME_DATA_SIZE);
			}
		}
		else
		{
			//usCanDLC = pWriteMsg->pData[2];		//ori
			//pCanDataFields[0] = usCanDLC;			//ori
			//memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, usCanDLC);		//ori
			usCanDLC = CAN_FRAME_DATA_SIZE;
			if(pOutCanPacket->stNormalPacket.us11BitID >= 0x0700)
			{
				pCanDataFields[0] = pWriteMsg->pData[2];
				memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, pWriteMsg->pData[2]);	//비클인데 여기서 pData[2]만큼 복사하면 난리남
			}
			else
			{
				memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, usCanDLC);	//진단일때 Length만큼 복사 비클이면 렝스가 없으므로 다르게 사용
			}
		}
	}
	else if ( nCanFrameType == CAN_FIRST_FRAME )
	{
		unsigned short usSendLength;

		if( uiProtocolID == ISO15765_NEW || uiProtocolID == ISO15765_CAN_HWSET_DB_NEW )
		{
		  	g_uiCanWriteMsgLength++;		// 해당 프로토콜은 datasize가 2byte이다. 그래서 하나더 증가 시켜준다 150914 lwh
			usSendLength =  ((pWriteMsg->pData[2]&0x0F)<<4) + pWriteMsg->pData[3];
			usCanDLC = CAN_FRAME_DATA_SIZE;
			pCanDataFields[0] = CAN_FIRST_FRAME | ((usSendLength>>8) &0x0F);
			pCanDataFields[1] = (unsigned char)usSendLength;
			memcpy(pCanDataFields+2, pWriteMsg->pData+ulDataIndex+1, usCanDLC-2);
		}
		else
		{
#ifdef CGW_SECURITY
			usSendLength = pWriteMsg->DataSize-3;
#else
			usSendLength = pWriteMsg->pData[2];
#endif
			usCanDLC = CAN_FRAME_DATA_SIZE;
			pCanDataFields[0] = CAN_FIRST_FRAME | ((usSendLength>>8) &0x0F);
			pCanDataFields[1] = (unsigned char)usSendLength;
			memcpy(pCanDataFields+2, pWriteMsg->pData+ulDataIndex, usCanDLC-2);
		}
	}
	else if ( nCanFrameType == CAN_CONSECUTICE_FRAME )
	{
		//unsigned char ucSendLen;
		//ucSendLen = (pWriteMsg->DataSize - 2/*CanID*/ - 1/*Data Length*/) - g_uiCanWriteMsgLength;
		usCanDLC = CAN_FRAME_DATA_SIZE;
		/*if ( ucSendLen >= (CAN_FRAME_DATA_SIZE-1) )
			usCanDLC = CAN_FRAME_DATA_SIZE - 1;
		else
			usCanDLC = ucSendLen;*/

		pCanDataFields[0] = CAN_CONSECUTICE_FRAME + (++g_uiCanTxConsFrameNo % 0x10);

		if( uiProtocolID == ISO15765_NEW || uiProtocolID == ISO15765_CAN_HWSET_DB_NEW )
		{
			memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex+1, usCanDLC-1);
		}
		else
		{
			memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, usCanDLC-1);
		}
	}

	if ( bIsStandardCan )	pOutCanPacket->stNormalPacket.ucDLC = usCanDLC;
	else					pOutCanPacket->stExtendPacket.ucDLC = usCanDLC;

#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s]ulDataIndex %d, g_uiCanWriteMsgLength %d, ucDLC %d\r\n",
					__FUNCTION__, ulDataIndex, g_uiCanWriteMsgLength, usCanDLC);
#endif
}

void CAN_MakeSendFrame_Lcan(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, int nCanFrameType, stPASSTHRU_MSG *pWriteMsg)	//tx single, first, consecutive
{
	unsigned char *pCanDataFields;
	unsigned short usCanDLC;
	unsigned long  ulDataIndex;//, nUDS_Data_Struct;
	unsigned int uiProtocolID;

	uiProtocolID = VCI_GetPassThruProtocolID();

	// compile 시, warning 제거를 위해서
//	uiProtocolID = VCI_GetPassThruProtocolID();

	if ( bIsStandardCan )
	{
		ulDataIndex = 2/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;
		pCanDataFields = pOutCanPacket->stNormalPacket.arrDataFields;
		//usCanDLC = pOutCanPacket->stNormalPacket.ucDLC;	//150914 lwh 불필요 해보인다 삭제
//		if ( nUDS_Data_Struct == 1)
//			memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x55, SIZE_CAN_DATA_FIELD);
//		else
		memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x00, SIZE_CAN_DATA_FIELD);
	}
	else
	{
		if( uiProtocolID == ISO15765_CARB_29BIT )
			ulDataIndex = 2/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;
		else
			ulDataIndex = 4/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;

		pCanDataFields = pOutCanPacket->stExtendPacket.arrDataFields;
		//usCanDLC = pOutCanPacket->stExtendPacket.ucDLC;
		memset(pCanDataFields, 0x00, SIZE_CAN_DATA_FIELD);
	}

	usCanDLC = CAN_FRAME_DATA_SIZE;
	memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, usCanDLC);	//진단일때 Length만큼 복사 비클이면 렝스가 없으므로 다르게 사용

	if ( bIsStandardCan )	pOutCanPacket->stNormalPacket.ucDLC = usCanDLC;
	else					pOutCanPacket->stExtendPacket.ucDLC = usCanDLC;

#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s]ulDataIndex %d, g_uiCanWriteMsgLength %d, ucDLC %d\r\n",
					__FUNCTION__, ulDataIndex, g_uiCanWriteMsgLength, usCanDLC);
#endif
}

void CAN_MakeReadMsgFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
#if defined(DEBUG_CAN_LOG)
		GITDebugPrintf("[%s] run\r\n", __FUNCTION__);
#endif
}

void CAN_MakeRxSingleFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
	unsigned int uiProtocolID, uiCopyLen, uiCopyIndex, uiCanIDLen, uiArrayIndex = 0;
	unsigned char *pCanData;

#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s] run\r\n", __FUNCTION__);
#endif

	uiProtocolID = VCI_GetPassThruProtocolID();
	CAN_AcquireArrayIndexFromCarbCanID(pInCanPacket, bIsStandardCan, &uiArrayIndex);

	if(bIsStandardCan == true)
	{
		uiCanIDLen = 2;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID >> 8;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID;

		if( pInCanPacket->stNormalPacket.us11BitID >= 0x0700 )
		{
			uiCopyLen = pInCanPacket->stNormalPacket.arrDataFields[0]&0x0F;
			uiCopyIndex = 1;
		}
		else
		{
			uiCopyLen = pInCanPacket->stNormalPacket.ucDLC;
			uiCopyIndex = 0;
		}

		pCanData = &pInCanPacket->stNormalPacket.arrDataFields[uiCopyIndex];
	}
	else // Extend CAN
	{
		if ( uiProtocolID == ISO15765_CARB_29BIT )
		{
			uiCanIDLen = 2;
			pReadMsg->pData[uiArrayIndex++] = 0x07;
			pReadMsg->pData[uiArrayIndex++] = 0x00;
		}
		else
		{
			uiCanIDLen = 4;
	        
			pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stExtendPacket.us11BitID>>6;
			pReadMsg->pData[uiArrayIndex]   = (pInCanPacket->stExtendPacket.us11BitID & 0x3F)<<2;
			pReadMsg->pData[uiArrayIndex++] += (pInCanPacket->stExtendPacket.us18BitID& 0x30000)>>16;
			pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF00)>>8;
			pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF);
			
		}

		if( (uiProtocolID == ISO15765_SINGLE) || 
			(uiProtocolID == ISO15765_SINGLE_SMK) || 
			(uiProtocolID == ISO15765_SINGLE_PODS)||
			(uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE)/*||
			(GM_MODE==0xAA)*/ )
		{
			uiCopyLen = 8;
			uiCopyIndex = 0;
			CAN_mDelay(3);
		}
		else if ( uiProtocolID == ISO15765_CUBIS)
		{
			if ( pInCanPacket->stExtendPacket.arrDataFields[0] > 6 )
			{
				uiCopyLen = 8;
				uiCopyIndex = 1;
			}
			else
			{
				uiCopyLen = (pInCanPacket->stExtendPacket.arrDataFields[0]&0x0F);
				uiCopyIndex = 1;
			}
		}
		else
		{
			uiCopyLen = pInCanPacket->stExtendPacket.arrDataFields[0]&0x0F;
			uiCopyIndex = 1;
		}

		pCanData = &pInCanPacket->stExtendPacket.arrDataFields[uiCopyIndex];
	}
	// copy data
	pReadMsg->DataSize += uiCanIDLen;
	memcpy(pReadMsg->pData+uiArrayIndex, pCanData, uiCopyLen);
	pReadMsg->DataSize += uiCopyLen;

	g_uiCanReadMsgLength += uiCanIDLen;
	g_uiCanReadMsgLength += uiCopyLen;

	CAN_SaveCarbCanRecvInfo(pInCanPacket, bIsStandardCan, uiCanIDLen, uiCopyLen, uiCopyLen);

	if( VCI_GetPassThruProtocolID() == ISO15765_CARB )
	{
		g_bCanRxCarbFrame = TRUE;
		g_ulCanWrittenTick = OemGetTmr();		// carb 들어온 시점부터 타이머 재가동
	}
	else
	{
		if ( pReadMsg->DataSize > 0 )
		{
			CAN_ClearCarbCanRecvInfo();
			if(CFD_GetCanFDAdapter())
			{
				if(bIsStandardCan == true) pReadMsg->RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stNormalPacket.ucDummy2;
				else					   pReadMsg->RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stExtendPacket.ucDummy1;
			}
            else
            {
            	if(bIsStandardCan == true) pReadMsg->RxStatus = g_InRxCanPacket.stNormalPacket.ucDummy2;
               	else                       pReadMsg->RxStatus = g_InRxCanPacket.stExtendPacket.ucDummy1;
            }
			PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType, bIsStandardCan);
		}
		g_bCanRxCarbFrame = FALSE;
		g_uiCanReadMsgLength = 0;
	}
}

unsigned int CAN_MakeTxSingleFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, stPASSTHRU_MSG *pWriteMsg, eCanType eCurrCanType)
{
	unsigned short usSendLength;

	// 만들고,
//	CAN_MakeSendFrame(pOutCanPacket, bIsStandardCan, CAN_SINGLE_FRAME, pWriteMsg);

	// send 하고
//	if(eCurrCanType==eVCAN)
//	{
//		CAN_MakeSendFrame_Vcan(pOutCanPacket, bIsStandardCan, CAN_SINGLE_FRAME, pWriteMsg);
//		usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 1/* CAN_CH*/, CAN_SINGLE_FRAME);
//		//VCAN_SET_COMM_STATE(eVCAN_RX_BLOCK);
//	}
	if(eCurrCanType==eLCAN)
	{
		CAN_MakeSendFrame_Lcan(pOutCanPacket, bIsStandardCan, CAN_SINGLE_FRAME, pWriteMsg);
		if(bIsStandardCan == true)
		{
			usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 2/* CAN_CH*/, CAN_SINGLE_FRAME);
		}
		else 
		{
			usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 2/* CAN_CH*/, CAN_SINGLE_FRAME);
		}
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
	}
	else	//DCAN
	{
		CAN_MakeSendFrame(pOutCanPacket, bIsStandardCan, CAN_SINGLE_FRAME, pWriteMsg);
		if(bIsStandardCan == true)
		{
			usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 1/* CAN_CH*/, CAN_SINGLE_FRAME);
		}
		else
		{
			usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 1/* CAN_CH*/, CAN_SINGLE_FRAME);
		}
		DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		g_uiCanWriteMsgLength = pWriteMsg->DataSize;
	}

#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("\r\n[%s] pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d Type:%d\r\n", __FUNCTION__, pWriteMsg->DataSize, g_uiCanWriteMsgLength, eCurrCanType);
#endif

	return usSendLength;
}

void CAN_MakeRxFirstFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
	unsigned int uiProtocolID, uiCopyLen, uiCopyIndex, uiCanIDLen, uiArrayIndex=0, uiPacketTotalLen;
	unsigned char *pCanData;

 #if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s] run\r\n", __FUNCTION__);
#endif

	uiProtocolID = VCI_GetPassThruProtocolID();

	// compile 시, warning 제거를 위해서
	uiProtocolID = uiProtocolID;

	g_uiCanRxConsFrameNo = 0;

	CAN_AcquireArrayIndexFromCarbCanID(pInCanPacket, bIsStandardCan, &uiArrayIndex);

	if( bIsStandardCan )
	{
	 	g_usRcvCanID = pInCanPacket->stNormalPacket.us11BitID;

		uiCanIDLen = 2;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID >> 8;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID;

		uiPacketTotalLen = ((pInCanPacket->stNormalPacket.arrDataFields[0] & 0x0F)<<8) + pInCanPacket->stNormalPacket.arrDataFields[1];
		uiCopyLen = 6;
		uiCopyIndex = 2;
		if( (bIsStandardCan == false) && (g_InRxCanPacket.stExtendPacket.ucDummy1 == 2) ) LCAN_SET_COMM_STATE(eLCAN_TX_FLOWCONTROL_FRAME); 
		else                                                                              DCAN_SET_COMM_STATE(eCAN_TX_FLOWCONTROL_FRAME);

		pCanData = &pInCanPacket->stNormalPacket.arrDataFields[uiCopyIndex];
	}
	else
	{
		if ( uiProtocolID == ISO15765_CARB_29BIT )
		{
			uiCanIDLen = 2;
			uiCopyLen = 6;
			uiCopyIndex = 2;
			pReadMsg->pData[uiArrayIndex++] = 0x07;
			pReadMsg->pData[uiArrayIndex++] = 0x00;
		}
		else
		{
			uiCanIDLen = 4;
			uiCopyLen = 6;
			uiCopyIndex = 2;
			pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stExtendPacket.us11BitID>>6;
			pReadMsg->pData[uiArrayIndex]   = (pInCanPacket->stExtendPacket.us11BitID & 0x3F)<<2;
			pReadMsg->pData[uiArrayIndex++] += (pInCanPacket->stExtendPacket.us18BitID& 0x30000)>>16;
			pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF00)>>8;
			pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF);
		}
		
		uiPacketTotalLen = ((pInCanPacket->stExtendPacket.arrDataFields[0] & 0x0F)<<8) + pInCanPacket->stExtendPacket.arrDataFields[1];

		if( (bIsStandardCan == false) && (g_InRxCanPacket.stExtendPacket.ucDummy1 == 2) )    LCAN_SET_COMM_STATE(eLCAN_TX_FLOWCONTROL_FRAME);
		else                                                  DCAN_SET_COMM_STATE(eCAN_TX_FLOWCONTROL_FRAME);	
		
		pCanData = &pInCanPacket->stExtendPacket.arrDataFields[uiCopyIndex];
	}
	// copy data
	pReadMsg->DataSize += uiCanIDLen;
	memcpy(pReadMsg->pData+uiArrayIndex, pCanData, uiCopyLen);
	pReadMsg->DataSize += uiCopyLen;

	g_uiCanReadMsgLength += uiCanIDLen;
	g_uiCanReadMsgLength += uiPacketTotalLen;
    
	if(CFD_GetCanFDAdapter())
	{
	  	pReadMsg->RxStatus = CANFD_SPI_DEFAULT + pInCanPacket->stNormalPacket.ucDummy2;
	}
	else
    {
        pReadMsg->RxStatus = pInCanPacket->stNormalPacket.ucDummy2;
    }

	CAN_SaveCarbCanRecvInfo(pInCanPacket, bIsStandardCan, uiCanIDLen, uiCopyLen, uiPacketTotalLen);

#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s] g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", __FUNCTION__, g_uiCanReadMsgLength, pReadMsg->DataSize);
#endif
}

unsigned int CAN_MakeTxFirstFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pWriteMsg, eCanType eCurrCanType)
{
	unsigned short usSendLength;

	g_uiCanTxConsFrameNo = 0;
	// 만들고,
	CAN_MakeSendFrame(pOutCanPacket, bIsStandardCan, CAN_FIRST_FRAME, pWriteMsg);
	// send 하고
	if( eCurrCanType == eLCAN  )	
	{
		usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 2/* CAN_CH*/, CAN_FIRST_FRAME);
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
	}
	else
	{
	usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 1/* CAN_CH*/, CAN_FIRST_FRAME);
		DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
	}
	
	g_uiCanWriteMsgLength += CAN_FRAME_DATA_SIZE - 2/*FitstFrame*/; 	//datasize 가 2byte인 경우 대비 lwh
	//g_uiCanWriteMsgLength = CAN_FRAME_DATA_SIZE - 2/*FitstFrame*/;

#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("\r\n[%s] Can TX : pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d \r\n", __FUNCTION__, pWriteMsg->DataSize, g_uiCanWriteMsgLength);
#endif

	return usSendLength;
}

void CAN_MakeRxFlowControlFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
	unsigned char *pTmpCanPacket;
#ifndef CGW_SECURITY
    int nUDS_Data_Struct;
#endif
    
#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s] run\r\n", __FUNCTION__);
#endif

	if ( bIsStandardCan == TRUE )
	{
		pTmpCanPacket = pInCanPacket->stNormalPacket.arrDataFields;
	}
	else
	{
		pTmpCanPacket = pInCanPacket->stExtendPacket.arrDataFields;
	}

	if( (pTmpCanPacket[0]&0x0F) == 0 )
	{
		g_stECUSetConfig.nBSTx = pTmpCanPacket[1];
		g_stECUSetConfig.nSTMinTx = pTmpCanPacket[2];

#ifndef CGW_SECURITY
		nUDS_Data_Struct = (g_stGITSetConfig.nEtc3&0x0100 == 0x0100 ? 1:0);
	   // 각 FRAME마다 걸리는 시간지연을 사용자로부터 입력을 받아서 할 수 있도록 하기위한 코드 (현재는 지원 안함)
		if( !((nUDS_Data_Struct == 1) && (g_stGITSetConfig.nEtc1 != 0)) )
			g_stECUSetConfig.nSTMinTx = g_stGITSetConfig.nEtc1;
#endif
	   if( (bIsStandardCan == false) && (g_InRxCanPacket.stExtendPacket.ucDummy1 == 2) )    LCAN_SET_COMM_STATE(eLCAN_TX_CONSECUTIVE_FRAME);
	   else                                                        DCAN_SET_COMM_STATE(eCAN_TX_CONSECUTIVE_FRAME);
	}
	else
	{
		if( (bIsStandardCan == false) && (g_InRxCanPacket.stExtendPacket.ucDummy1 == 2) )   LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
		else	                                                   DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		
		g_bCanRxPendingFrame = TRUE;		//150915 lwh 추가
	}
}

void CAN_MakeTxFlowControlFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pWriteMsg, eCanType eCurrCanType)
{
	unsigned int uiProtocolID, nUDS_Data_Struct, nCanidOffset;

#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("\r\n[%s] pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d \r\n", __FUNCTION__, pWriteMsg->DataSize, g_uiCanWriteMsgLength);
#endif
	uint8_t ucAutovinCANLine=HIGHCAN1;
	GetAutolinkConfigProperty(eAutoLinkConfig_AutovinCANLine,(void*)&ucAutovinCANLine); 
  
	uiProtocolID = VCI_GetPassThruProtocolID();

	// compile 시, warning 제거를 위해서
	uiProtocolID = uiProtocolID;

	nUDS_Data_Struct = (g_stGITSetConfig.nEtc3&0x0100 == 0x0100 ? 1:0);
	nCanidOffset 	 = g_stGITSetConfig.nEtc2;

	if( uiProtocolID == ISO15765_CUBIS)
	{
		// cubis 관련 플로우 컨트롤 하지 않음.
	}
	else
	{
		if ( bIsStandardCan == TRUE )
		{
			pOutCanPacket->stNormalPacket.us11BitID = g_usRcvCanID - 8;

			if ( nUDS_Data_Struct == 1)
				memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x55, CAN_FRAME_DATA_SIZE);
			else
				memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x00, CAN_FRAME_DATA_SIZE);

			pOutCanPacket->stNormalPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
			pOutCanPacket->stNormalPacket.arrDataFields[1] = 0;// 2016/06/02 James Jean g_stGITSetConfig.nBSTx;
			pOutCanPacket->stNormalPacket.arrDataFields[2] = 0;// 2016/06/02 James Jean g_stGITSetConfig.nSTMinTx;

			if(g_stGITSetConfig.nBSTx == 0xFFFF)		g_stGITSetConfig.nBSTx = 0;
			if(g_stGITSetConfig.nSTMinTx == 0xFFFF)		g_stGITSetConfig.nSTMinTx = 0;

		  	if((nCanidOffset != 0)||
			  ( uiProtocolID == ISO15765_CAN_HWSET_DB )||
			  ( uiProtocolID == ISO15765_CAN_HWSET_DB_NEW )||
			  ( uiProtocolID == ISO14229_UDS_HWSET_DB ))
			{
			  	if(nCanidOffset != 0)
					pOutCanPacket->stNormalPacket.us11BitID = g_InRxCanPacket.stNormalPacket.us11BitID - nCanidOffset;

				pOutCanPacket->stNormalPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
				pOutCanPacket->stNormalPacket.arrDataFields[1] = g_stGITSetConfig.nBSTx;
				pOutCanPacket->stNormalPacket.arrDataFields[2] = g_stGITSetConfig.nSTMinTx;
			}
			else if ( uiProtocolID == ISO15765_CARB )// 0X07DF & 0x18DB33F1 CARB통신인 경우
			{
				pOutCanPacket->stNormalPacket.us11BitID = g_InRxCanPacket.stNormalPacket.us11BitID - 8/*Carb통신인 경우에 Can ID의 -8을 해서 전달*/;
			}
			else	//ISO15765
			{	//30 08 02
				g_stGITSetConfig.nBSTx = 0x08;
				g_stGITSetConfig.nSTMinTx = 0x02;
				pOutCanPacket->stNormalPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
				pOutCanPacket->stNormalPacket.arrDataFields[1] = g_stGITSetConfig.nBSTx;
				pOutCanPacket->stNormalPacket.arrDataFields[2] = g_stGITSetConfig.nSTMinTx;
			}
		}
		else
		{
			if ( uiProtocolID == ISO15765_CARB_29BIT )
			{
				// FlowControl을 전달할 때 0x18DA00F1으로 전달해야한다.
				// 따라서 11bit에는 0x636, 18bit에는 0x200F1가 입력되어야 한다.
				pOutCanPacket->stExtendPacket.us11BitID = 0x636;
				pOutCanPacket->stExtendPacket.us18BitID = 0x200F1;

				memset(pOutCanPacket->stExtendPacket.arrDataFields, 0xFF, CAN_FRAME_DATA_SIZE);

				pOutCanPacket->stExtendPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
				pOutCanPacket->stExtendPacket.arrDataFields[1] = 0; // 2016/06/02 James Jean g_stGITSetConfig.nBSTx;
				pOutCanPacket->stExtendPacket.arrDataFields[2] = 0x05;
				pOutCanPacket->stExtendPacket.arrDataFields[3] = 0x00;

			}
			else
			{
				if ( nUDS_Data_Struct == 1)
					memset(pOutCanPacket->stExtendPacket.arrDataFields, 0x55, CAN_FRAME_DATA_SIZE);
				else
					memset(pOutCanPacket->stExtendPacket.arrDataFields, 0x00, CAN_FRAME_DATA_SIZE);

				pOutCanPacket->stExtendPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
				pOutCanPacket->stExtendPacket.arrDataFields[1] = 0; // 2016/06/02 James Jean g_stGITSetConfig.nBSTx;
				pOutCanPacket->stExtendPacket.arrDataFields[2] = 0; // 2016/06/02 James Jean g_stGITSetConfig.nSTMinTx;
			}
		}
		
		if((ucAutovinCANLine == HIGHCAN3) && (GetOBDState()==eOBD_GetAutoVIN))
            CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 2/* CAN_CH*/, CAN_FLOWCTRL_FRAME);
		else
            CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 1/* CAN_CH*/, CAN_FLOWCTRL_FRAME);
	}

	if(eCurrCanType == eLCAN)    LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
	else                         DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
}

unsigned int CAN_MakeTxConsecutiveFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pWriteMsg, eCanType eCurrCanType)
{
	unsigned short usSendLength = 0;
	uint8_t ucAutovinCANLine=HIGHCAN1;
	GetAutolinkConfigProperty(eAutoLinkConfig_AutovinCANLine,(void*)&ucAutovinCANLine); 
	
#ifdef CGW_SECURITY
	if( Get_TmrDelta(Get_Tmr(),g_ulCanWrittenTick)>g_stECUSetConfig.nSTMinTx)
	{
		CAN_MakeSendFrame(pOutCanPacket, bIsStandardCan, CAN_CONSECUTICE_FRAME, pWriteMsg);
		if ( bIsStandardCan )	g_uiCanWriteMsgLength += (pOutCanPacket->stNormalPacket.ucDLC-1);
		else					g_uiCanWriteMsgLength += (pOutCanPacket->stExtendPacket.ucDLC-1);

		// send 하고
		usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 1/* CAN_CH*/, CAN_CONSECUTICE_FRAME);

		if ( (pWriteMsg->DataSize - 2/*CANID 사이즈 추가*/ - 1/*Data Length*/) > g_uiCanWriteMsgLength )
		{
			if ( g_stECUSetConfig.nBSTx != 0 )
			{
				if ( (g_uiCanTxConsFrameNo % g_stECUSetConfig.nBSTx) == 0 )
					DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
			}
			else
			{
				DCAN_SET_COMM_STATE(eCAN_TX_CONSECUTIVE_FRAME);
			}
		}
		else
		{
			DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		}
	}
	else
	{
		return 0;
	}
#else
	// 만들고,
	CAN_MakeSendFrame(pOutCanPacket, bIsStandardCan, CAN_CONSECUTICE_FRAME, pWriteMsg);
	if ( bIsStandardCan )	g_uiCanWriteMsgLength += (pOutCanPacket->stNormalPacket.ucDLC-1);
	else					g_uiCanWriteMsgLength += (pOutCanPacket->stExtendPacket.ucDLC-1);

	// send 하고
	
	if((ucAutovinCANLine == HIGHCAN3) && (GetOBDState()==eOBD_GetAutoVIN))
        usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 2/* CAN_CH*/, CAN_CONSECUTICE_FRAME);
	else
        usSendLength = CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, 1/* CAN_CH*/, CAN_CONSECUTICE_FRAME);

	if ( (pWriteMsg->DataSize - 2/*CANID 사이즈 추가*/ - 1/*Data Length*/) > g_uiCanWriteMsgLength )
	{
		if ( g_stECUSetConfig.nBSTx != 0 )
		{
			if ( (g_uiCanTxConsFrameNo % g_stECUSetConfig.nBSTx) == 0 )
				DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		}
		else
		{
			DCAN_SET_COMM_STATE(eCAN_TX_CONSECUTIVE_FRAME);
		}
	}
	else
	{
		DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
	}
#endif	//CGW_SECURITY

#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("\r\n[%s] Can TX : pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d , %d, %d\r\n",
						__FUNCTION__, pWriteMsg->DataSize, g_uiCanWriteMsgLength, g_uiCanTxConsFrameNo % 0x10, DCAN_GET_COMM_STATE());
#endif

	return usSendLength;
}

//#warning "optimization warning: CAN_MakeRxConsecutiveFrame() 함수만 code optimize를 수행하지 않도록 함"
#pragma optimize=z low no_code_motion
void CAN_MakeRxConsecutiveFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("\r\n[%s] run\r\n", __FUNCTION__);
#endif
	unsigned char *pTmp, ucDLC;
//	U8 uctempbuff[30];
	unsigned int uiArrayIndex = 0, uiOrder, uiCurCanRecvLength;
	unsigned int uiRemainLen,uiProtocolID;

	uiProtocolID = VCI_GetPassThruProtocolID();

	// compile 시, warning 제거를 위해서
	uiProtocolID = uiProtocolID;

	uiOrder = CAN_AcquireArrayIndexFromCarbCanID(pInCanPacket, bIsStandardCan, &uiArrayIndex);

	// pInCanPacket->stNormalPacket.arrDataFields[0]에는 Sequence Number정보가 존재
	if(bIsStandardCan == true)
	{
		ucDLC = pInCanPacket->stNormalPacket.ucDLC;
		pTmp = pInCanPacket->stNormalPacket.arrDataFields+1;
	}
	else
	{
		ucDLC = pInCanPacket->stExtendPacket.ucDLC;
		pTmp = pInCanPacket->stExtendPacket.arrDataFields+1;
	}

	if ( ucDLC > 0 )
	{
		uiRemainLen = CAN_GetCarbTotalPacketLength(uiOrder) - CAN_GetCarbRecvedLength(uiOrder);

		if ( uiRemainLen > (CAN_FRAME_DATA_SIZE-1) )	uiCurCanRecvLength = CAN_FRAME_DATA_SIZE-1;
		else											uiCurCanRecvLength  = uiRemainLen;

		memcpy(pReadMsg->pData+uiArrayIndex, pTmp, uiCurCanRecvLength);

		pReadMsg->DataSize += uiCurCanRecvLength;
		CAN_SaveCarbCanRecvLength(uiOrder, uiCurCanRecvLength);

#if defined(DEBUG_CAN_LOG)
		GITDebugPrintf("[%s] Standard CAN : DLC %d, g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", __FUNCTION__, pInCanPacket->stNormalPacket.ucDLC, g_uiCanReadMsgLength, pReadMsg->DataSize);
		GITDebugPrintf("[%s] Extended CAN :DLC %d, g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", __FUNCTION__, pInCanPacket->stExtendPacket.ucDLC, g_uiCanReadMsgLength, pReadMsg->DataSize);
#endif

		if( VCI_GetPassThruProtocolID() == ISO15765_CARB )
		{
			g_bCanRxCarbFrame = TRUE;
			g_ulCanWrittenTick = OemGetTmr();		// carb 들어온 시점부터 타이머 재가동
		}
		else
		{
#if 0 //기존로직
//			if ( g_uiCanReadMsgLength == pReadMsg->DataSize )
//			{
//				CAN_ClearCarbCanRecvInfo();
//				g_bCARBReceiving = FALSE;
//
//				PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType);
//				g_ulCanWrittenTick = 0; // consequtive frame의 마지막을 받고도 timeout 메시지가 전달이 되는 것을 막기 위해.
//				g_uiCanReadMsgLength = 0;
//				DCAN_SET_COMM_STATE(eCAN_RX_DONE);
//			}
//			else if ( g_uiCanReadMsgLength < pReadMsg->DataSize )
//			{
//				GITDebugPrintf("[%s] DLC %d, g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", __FUNCTION__, pInCanPacket->stNormalPacket.ucDLC, g_uiCanReadMsgLength, pReadMsg->DataSize);
//				for(u8 i=0; i<10; i++)	OemReadCanBuff(uctempbuff, NULL, NULL, 1);		//150915 lwh 추가
//				CANCOMM_SET_STATE(eCAN_COMM_RX_FAIL);
//				g_ulCanWrittenTick = OemGetTmr();		//160202 lwh 음... consecutive가 마지막까지 안들어오면 timeout필요
//			}
#else	//김진팀장님과 함께보며 수정
			if ( g_uiCanReadMsgLength == pReadMsg->DataSize )
			{
				CAN_ClearCarbCanRecvInfo();
				g_bCARBReceiving = FALSE;
				if(CFD_GetCanFDAdapter())
				{
					if(bIsStandardCan == true) pReadMsg->RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stNormalPacket.ucDummy2;
					else					   pReadMsg->RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stExtendPacket.ucDummy1;
				}
                else
                {
                	if(bIsStandardCan == true) pReadMsg->RxStatus = g_InRxCanPacket.stNormalPacket.ucDummy2;
					else					   pReadMsg->RxStatus = g_InRxCanPacket.stExtendPacket.ucDummy1;
                }
				PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType, bIsStandardCan);

				if( (bIsStandardCan == false) && (g_InRxCanPacket.stExtendPacket.ucDummy1 == 2) )  LCAN_SET_COMM_STATE(eLCAN_RX_DONE);
				else                                                      DCAN_SET_COMM_STATE(eCAN_RX_DONE);
				
				g_ulCanWrittenTick = 0; // consequtive frame의 마지막을 받고도 timeout 메시지가 전달이 되는 것을 막기 위해.
				g_uiCanReadMsgLength = 0;
			}
			else if ( g_uiCanReadMsgLength < pReadMsg->DataSize )
			{
				CAN_ClearCarbCanRecvInfo();
				g_bCARBReceiving = FALSE;
				g_uiCanReadMsgLength = 0;
				if(CFD_GetCanFDAdapter())
				{
				  	if(bIsStandardCan == true) pReadMsg->RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stNormalPacket.ucDummy2;
					else					   pReadMsg->RxStatus = CANFD_SPI_DEFAULT + g_InRxCanPacket.stExtendPacket.ucDummy1;
				}
                else
                {
                    if(bIsStandardCan == true) pReadMsg->RxStatus = g_InRxCanPacket.stNormalPacket.ucDummy2;
					else					   pReadMsg->RxStatus = g_InRxCanPacket.stExtendPacket.ucDummy1;
                }
				PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType, bIsStandardCan);
			}
#endif
			else
			{
				g_ulCanWrittenTick = OemGetTmr();		//160202 lwh 음... consecutive가 마지막까지 안들어오면 timeout필요
			}
		}
	}
	else
	{
#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s] DLC is zero ~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n", __FUNCTION__);
#endif
		return;
	}
	//g_ulCanWrittenTick = OemGetTmr();		//160202 lwh 음... consecutive가 마지막까지 안들어오면 timeout필요
	if ( (++g_uiCanRxConsFrameNo % g_stGITSetConfig.nBSTx) == 0 )
	{	
		if( (bIsStandardCan == false) && (g_InRxCanPacket.stExtendPacket.ucDummy1 == 2) )  LCAN_SET_COMM_STATE(eLCAN_TX_FLOWCONTROL_FRAME);
		else                                                      DCAN_SET_COMM_STATE(eCAN_TX_FLOWCONTROL_FRAME);
	}
}

void CAN_Reinit(stQueue *pQue)
{
#if defined(DEBUG_CAN_LOG)
	GITDebugPrintf("[%s] run\r\n", __FUNCTION__);
#endif
	g_stReadPassThruMsg.DataSize = 0;
	g_bCARBReceiving = FALSE;

	memset(g_CarbRecvInfo, 0x00, sizeof(CARB_REVC_INFO)*MAX_CARB_RECV_ARRAY_CNT);

	ClearQueue(pQue);
}

void VCI_PeriodicMessage(void)
{
//	static U16 s_uiOldtime=0;
//	stCanPacket *pOutCanPacket=NULL;
//
//	if( Get_TmrDelta( Get_Tmr(), s_uiOldtime) >= g_uiPeriodicTime)
//	{
//		s_uiOldtime = Get_Tmr();
//		if((g_ucPeriodicMessage[0]&0x80) != 0x80)
//		{
//			pOutCanPacket->stNormalPacket.ucSOF 	= CAN_FRAME_SOF;
//			pOutCanPacket->stNormalPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
//			pOutCanPacket->stNormalPacket.ucRTR		= 0;
//			pOutCanPacket->stNormalPacket.ucReserved	= 0;
//			pOutCanPacket->stNormalPacket.usCRC		= 0;
//			pOutCanPacket->stNormalPacket.ucCRCDelimiter = 0;
//			pOutCanPacket->stNormalPacket.ucACK		= 0;
//			pOutCanPacket->stNormalPacket.ucACKDelimiter = 0;
//			pOutCanPacket->stNormalPacket.ucEOF		= CAN_FRAME_EOF;
//
//			pOutCanPacket->stNormalPacket.ucDLC		= g_ucPeriodicMessage[0];
//			pOutCanPacket->stNormalPacket.us11BitID	= ((g_ucPeriodicMessage[1]&0x07)<<8)  + g_ucPeriodicMessage[2];
//			memcpy(pOutCanPacket->stNormalPacket.arrDataFields, &g_ucPeriodicMessage[3], g_ucPeriodicMessage[0]);
//
//			OemWriteCanBuff((unsigned char*)pOutCanPacket, 0, NULL, 1);
//		}
//		else
//		{
//			pOutCanPacket->stExtendPacket.ucSOF 			= CAN_FRAME_SOF;
//			pOutCanPacket->stExtendPacket.ucSRR 			= 1;
//			pOutCanPacket->stExtendPacket.ucIDE 				= CAN_FRAME_EXTEND_IDE;
//			pOutCanPacket->stExtendPacket.ucRTR 			= 0;
//			pOutCanPacket->stExtendPacket.ucReserved 		= 0;
//			pOutCanPacket->stExtendPacket.usCRC 			= 0;
//			pOutCanPacket->stExtendPacket.ucCRCDelimiter 	= 1;
//			pOutCanPacket->stExtendPacket.ucACK 			= 1;
//			pOutCanPacket->stExtendPacket.ucACKDelimiter 	= 1;
//			pOutCanPacket->stExtendPacket.ucEOF 			= CAN_FRAME_EOF;
//			pOutCanPacket->stExtendPacket.us11BitID 		= ((g_ucPeriodicMessage[1]&0x07)<<8)  + g_ucPeriodicMessage[2];
//			pOutCanPacket->stExtendPacket.us18BitID 		= ((g_ucPeriodicMessage[2]&0x03)<<16) + g_ucPeriodicMessage[3]<<8 + g_ucPeriodicMessage[4];
//
//			pOutCanPacket->stExtendPacket.ucDLC = g_ucPeriodicMessage[0]&0x08;
//			memcpy(pOutCanPacket->stExtendPacket.arrDataFields, &g_ucPeriodicMessage[5], pOutCanPacket->stExtendPacket.ucDLC);
//
//			OemWriteCanBuff((unsigned char*)pOutCanPacket, 0, NULL, 1);
//		}
//	}
}

unsigned int FineReadCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	unsigned int nRxLen = 0;
	stCanPacket RxCANPacket;
#if defined(DEBUG_CAN_PACKET)
	BOOL bStandardCan = TRUE;
#endif
	stHalCanRxMsg rxmsg;
	unsigned char nCanChannel;

	nCanChannel = wParam;

	if ( nCanChannel == eCOMM_TYPE_CAN1 )
	{
		if (!HalCan1_Read(&rxmsg))		return nRxLen;
	}
	else
	{
		if (!HalCan2_Read(&rxmsg))		return nRxLen;
	}

	if ( rxmsg.IDE == HAL_CAN_ID_STD )
	{
		RxCANPacket.stNormalPacket.ucSOF 		= 1;
		RxCANPacket.stNormalPacket.us11BitID	= rxmsg.StdId;
		RxCANPacket.stNormalPacket.ucRTR		= rxmsg.RTR;
		RxCANPacket.stNormalPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
		RxCANPacket.stNormalPacket.ucReserved	= 0;
		RxCANPacket.stNormalPacket.ucDLC		= rxmsg.DLC;
		memcpy(RxCANPacket.stNormalPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
		RxCANPacket.stNormalPacket.usCRC		= 0;
		RxCANPacket.stNormalPacket.ucCRCDelimiter = 0;
		RxCANPacket.stNormalPacket.ucACK		= 0;
		RxCANPacket.stNormalPacket.ucACKDelimiter = 0;
		RxCANPacket.stNormalPacket.ucEOF		= CAN_FRAME_EOF;

#if defined(DEBUG_CAN_PACKET)
		bStandardCan = TRUE;
#endif
	}
	else
	{
//		RxCANPacket.stExtendPacket.ucSOF 		= 1;
//		RxCANPacket.stExtendPacket.us11BitID	= rxmsg.ExtId >> 18;
//		RxCANPacket.stExtendPacket.ucSRR		= 1;
//		RxCANPacket.stExtendPacket.ucIDE		= CAN_FRAME_EXTEND_IDE;
//		RxCANPacket.stExtendPacket.us18BitID	= rxmsg.ExtId;
//		RxCANPacket.stExtendPacket.ucRTR		= rxmsg.RTR;
//		RxCANPacket.stExtendPacket.ucReserved	= 0;
//		RxCANPacket.stExtendPacket.ucDLC		= rxmsg.DLC;
//		memcpy(RxCANPacket.stExtendPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
//		RxCANPacket.stExtendPacket.usCRC		= 0;
//		RxCANPacket.stExtendPacket.ucCRCDelimiter = 0;
//		RxCANPacket.stExtendPacket.ucACK		= 0;
//		RxCANPacket.stExtendPacket.ucACKDelimiter = 0;
//		RxCANPacket.stExtendPacket.ucEOF		= CAN_FRAME_EOF;
//
//#if defined(DEBUG_CAN_PACKET)
//		bStandardCan = FALSE;
//#endif
	}

	nRxLen = sizeof(RxCANPacket);
	memcpy(pBuff, &RxCANPacket, nRxLen);


//	// LED blink
//	if ( g_nCANTransmitLedDelayCount == 0 )
//		g_nCANTransmitLedDelayCount = MAX_CAN_LED_DELAY_CNT;
//	SetCANTransmitLedOnOff();


#if defined(DEBUG_CAN_PACKET)
{
	unsigned char* pTmp;
	int i, nRecvCanLen;

	if ( bStandardCan == TRUE )
	{
		GITDebugPrintf("\r\n\r\n[%s] Receive Standard CanType : CAN Len %d\r\n %04X ",
			__FUNCTION__, RxCANPacket.stNormalPacket.ucDLC, RxCANPacket.stNormalPacket.us11BitID);
		pTmp = RxCANPacket.stNormalPacket.arrDataFields;
		nRecvCanLen = RxCANPacket.stNormalPacket.ucDLC;
	}
	else
	{
		GITDebugPrintf("\r\n\r\n[%s] Receive Extend CanType : CAN Len %d\r\n %04X ",
			__FUNCTION__, RxCANPacket.stExtendPacket.ucDLC, ((RxCANPacket.stExtendPacket.us11BitID<<18) | RxCANPacket.stExtendPacket.us18BitID));
		pTmp = RxCANPacket.stExtendPacket.arrDataFields;
		nRecvCanLen = RxCANPacket.stExtendPacket.ucDLC;
	}

	for ( i=0; i<nRecvCanLen; i++ )
		GITDebugPrintf("%02X ", pTmp[i]);

	GITDebugPrintf("\r\n");
}
#endif

	return nRxLen;
}

unsigned int FineWriteCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	stCanPacket *pOutCanPacket = (stCanPacket*)pBuff;
	unsigned char nCanChannel, ucCanSentLen = 0;
	unsigned int uiCanId;

	stHalCANTX_STRUCT *cantx;
	nCanChannel = wParam;

	if ( nCanChannel == eCOMM_TYPE_CAN1 )
		cantx = &g_CAN1_TxBuffCtrl;
	else
		cantx = &g_CAN2_TxBuffCtrl;

	if(!pOutCanPacket->stNormalPacket.ucIDE) {
		//standard
		uiCanId = pOutCanPacket->stNormalPacket.us11BitID;
		HalCanSet_Std(cantx, uiCanId);
		//ucCanSentLen += 2/*CAN ID*/;
		HalCan_Tx(cantx, pOutCanPacket->stNormalPacket.arrDataFields, pOutCanPacket->stNormalPacket.ucDLC);
		ucCanSentLen = pOutCanPacket->stNormalPacket.ucDLC;
	}
	else {
		//extended
		uiCanId = pOutCanPacket->stExtendPacket.us11BitID<<18 | pOutCanPacket->stExtendPacket.us18BitID;
		HalCanSet_Ext(cantx, uiCanId);
		ucCanSentLen += 4/*CAN ID*/;
		HalCan_Tx(cantx, pOutCanPacket->stExtendPacket.arrDataFields, CAN_FRAME_DATA_SIZE);
		ucCanSentLen = pOutCanPacket->stExtendPacket.ucDLC;
	}

#if defined(DEBUG_CAN_PACKET)
	{
		int i;
		unsigned char *pTmp;

		if ( !pOutCanPacket->stNormalPacket.ucIDE )
		{
			GITDebugPrintf("\r\n[%s] Can TX Standard CanType : Len %d\r\n %04X ", __FUNCTION__, ucCanSentLen, uiCanId);
			pTmp = pOutCanPacket->stNormalPacket.arrDataFields;
		}
		else
		{
			GITDebugPrintf("\r\n[%s] Can TX Extend CanType : Len %d\r\n %04X ", __FUNCTION__, ucCanSentLen, uiCanId);
			pTmp = pOutCanPacket->stExtendPacket.arrDataFields;
		}

		for ( i=0; i<ucCanSentLen+1/*DATA LEHGTN*/; i++ )
			GITDebugPrintf("%02X ", pTmp[i]);

		GITDebugPrintf("\r\n\r\n");
	}


#endif
	return ucCanSentLen;
}


//int TestCANMasking(unsigned int *puiStartCANID, unsigned int* puiEndCANID, unsigned int nMaskCount)
//{
//	stCanPacket OutCanPacket, InCanPacket;
//	unsigned int uiRecvLen = 0;
//	int nLoopCnt = 0;
//	int nTestMaskNum = 0,ret=0;
//
//	Oem_CAN_Channel_Initialize(CAN_CHANNEL_1, Highcan1, eCAN_500KBPS);
//
//	Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN, nMaskCount, puiStartCANID, puiEndCANID);
//
//	OutCanPacket.stNormalPacket.ucSOF 		= 1;
//	OutCanPacket.stNormalPacket.us11BitID	= puiStartCANID[nTestMaskNum];
//	OutCanPacket.stNormalPacket.ucRTR		= 0;
//	OutCanPacket.stNormalPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
//	OutCanPacket.stNormalPacket.ucReserved	= 0;
//	OutCanPacket.stNormalPacket.ucDLC		= 7;
//	OutCanPacket.stNormalPacket.usCRC		= 0;
//	OutCanPacket.stNormalPacket.ucCRCDelimiter = 0;
//	OutCanPacket.stNormalPacket.ucACK		= 0;
//	OutCanPacket.stNormalPacket.ucACKDelimiter = 0;
//	OutCanPacket.stNormalPacket.ucEOF		= CAN_FRAME_EOF;
//
//	memset(OutCanPacket.stNormalPacket.arrDataFields, 0x00, 8);
//	OutCanPacket.stNormalPacket.arrDataFields[0] = 0x02;
//	OutCanPacket.stNormalPacket.arrDataFields[1] = 0x10;
//	OutCanPacket.stNormalPacket.arrDataFields[2] = 0x81;
//
//	if ( FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, CAN_CHANNEL_1) > 0 )
//	{
//		nLoopCnt = 0;
//		uiRecvLen = 0;
//		do {
//			uiRecvLen = FineReadCanBuff((unsigned char*)&InCanPacket, sizeof(InCanPacket), NULL, CAN_CHANNEL_1);
//
//			if ( nLoopCnt++ > 3 )
//			{
//				GITDebugPrintf("Receive FAil ~~~~~~~~~~~~~~~~~~\r\n");
//				break;
//			}
//			else
//			{
//				if( uiRecvLen > 0 )
//				{
//					ret++;
//					GITDebugPrintf("Receive success ~~~~~~~~~~~~~~~~~~\r\n");
//				}
//				else
//					Oem_GIT_mDelay(30);
//			}
//		}while(uiRecvLen == 0);
//	}
//
////		Oem_CAN_Channel_Initialize(CAN_CHANNEL_2, Highcan2, eCAN_500KBPS);
////
////	Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_CAN, nMaskCount, puiStartCANID, puiEndCANID);
////
////	OutCanPacket.stNormalPacket.ucSOF 		= 1;
////	OutCanPacket.stNormalPacket.us11BitID	= puiStartCANID[nTestMaskNum];
////	OutCanPacket.stNormalPacket.ucRTR		= 0;
////	OutCanPacket.stNormalPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
////	OutCanPacket.stNormalPacket.ucReserved	= 0;
////	OutCanPacket.stNormalPacket.ucDLC		= 7;
////	OutCanPacket.stNormalPacket.usCRC		= 0;
////	OutCanPacket.stNormalPacket.ucCRCDelimiter = 0;
////	OutCanPacket.stNormalPacket.ucACK		= 0;
////	OutCanPacket.stNormalPacket.ucACKDelimiter = 0;
////	OutCanPacket.stNormalPacket.ucEOF		= CAN_FRAME_EOF;
////
////	memset(OutCanPacket.stNormalPacket.arrDataFields, 0x11, 8);
////	OutCanPacket.stNormalPacket.arrDataFields[0] = 0x07;
////
//////	while ( 1 )
//////	{
////		Oem_GIT_mDelay(500);
////		if ( FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, CAN_CHANNEL_2) > 0 )
////		{
////			nLoopCnt = 0;
////			uiRecvLen = 0;
////			do {
////				uiRecvLen = FineReadCanBuff((unsigned char*)&InCanPacket, sizeof(InCanPacket), NULL, CAN_CHANNEL_2);
////
////				if ( nLoopCnt++ > 3 )
////				{
////					GITDebugPrintf("Receive FAil ~~~~~~~~~~~~~~~~~~\r\n");
////					break;
////				}
////				else
////				{
////					if( uiRecvLen > 0 )
////					{
////						ret++;
////						GITDebugPrintf("Receive success ~~~~~~~~~~~~~~~~~~\r\n");
////					}
////					else
////						Oem_GIT_mDelay(30);
////				}
////			}while(uiRecvLen == 0);
////		}
////
////		OutCanPacket.stNormalPacket.us11BitID++;
////		if ( OutCanPacket.stNormalPacket.us11BitID == puiEndCANID[nTestMaskNum] )
////		{
////			nTestMaskNum = (++nTestMaskNum) % nMaskCount;
////			OutCanPacket.stNormalPacket.us11BitID = puiStartCANID[nTestMaskNum]-3;
////		}
////	}
//		return ret;
//}
/*****************************END OF FILE****/

void Can_DeInit_Sleep(void)	//슬립시 CAN2 Rx를 Low시켜주지않으면 웨이크업 신호가 계속 High라 엣지발생안함
{
	//GPIO_InitTypeDef  GPIO_InitStructure;
	Oem_CAN1_STANDBY_ACTIVE();		//HIGHCAN1 슬립모드 1,2번은 같은 1042칩
	Oem_CAN2_HIGH_CAN3_DISABLE();	//HIGHCAN3 슬립모드 2번째 1042칩
	Oem_CAN2_LOW_CAN_DISABLE();		//LOWCAN 슬립모드 1055칩


    HalDrvCanIOCtrl(eCAN_IO_DeInit, (int)HAL_CAN1, NULL, 0, 0);
    HalDrvCanIOCtrl(eCAN_IO_DeInit, (int)HAL_CAN2, NULL, 0, 0);

	APP_Delay(50);
}

