/*************************************************************
 * NOTE : git_can.c
 *      FDCAN control
 * Author : Lee junho
 * Since : 2019.09.03
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "gpio.h"
#include "cmsis_os.h"
#include "fdcan.h"

#include "common.h"  
#include "typedef.h"
//#include "data.h"
#include "git_can.h"
#include "git_vci.h"
#include "git_OBDcomm.h"
#include "git_PassthruDefines.h"
#include "sw_timer.h"
#include "git_kl.h"
#include "git_ioctl.h"
#include "git_function_list.h"
#include "git_mmc.h"
#include "git_rtc.h"
#include "git_protocol.h"
#include "git_global.h"

//#define DEBUG_CAN_LOG

#define CARB_LENGTH_LEN		2

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
static void  OBDCanTxThread( void const *argument );
static void  OBDCanRxThread( void const *argument );
static void  OBDKlineTxThread( void const *argument );
static void  OBDKlineRxThread( void const *argument );
unsigned int CAN_WriteBuff(unsigned char* pBuff, unsigned int nCount, unsigned int uiCANChannel, int nCanFrameType);
eDiagCanState CAN_TxParsing(stCanPacket *pOutCanPacket, stPASSTHRU_MSG *pWriteMsg, U8 *pbStandardCAN);
eDiagCanState CAN_MakeTxConsecutiveFrame(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg);
eDiagCanState CAN_MakeTxFlowControlFrame(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg);
eDiagCanState CAN_MakeTxFirstFrame(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg);
eDiagCanState CAN_MakeTxSingleFrame(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg);
unsigned long CAN_GetRxP3MinTimeOutValue();
BOOL 		  CAN_FindCanPacket(stCanPacket *pInCanPacket, eCanType *pbStandardCan);
eDiagCanState CAN_RxBlockProc(stCanPacket *pInCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pReadMsg);
eDiagCanState CAN_RxBlockProc_EX(stCanPacket *pInCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pReadMsg);
eDiagCanState CAN_MakeRxSingleFrame(stCanPacket *pInCanPacket, U8 bIsStandardCan, eDiagCanState eCanRxState, stPASSTHRU_MSG *pReadMsg);
eDiagCanState CAN_MakeRxConsecutiveFrame(stCanPacket *pInCanPacket, U8 bIsStandardCan, eDiagCanState eCanRxState, stPASSTHRU_MSG *pReadMsg);
eDiagCanState CAN_MakeRxFlowControlFrame(stCanPacket *pInCanPacket, U8 bIsStandardCan, eDiagCanState eCanRxState, stPASSTHRU_MSG *pReadMsg);
eDiagCanState CAN_MakeRxFirstFrame(stCanPacket *pInCanPacket, U8 bIsStandardCan, eDiagCanState eCanRxState, stPASSTHRU_MSG *pReadMsg);
eDiagCanState CAN_MakeTxSingleFrame_EX(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg);
eDiagCanState CAN_J1939_FuncProc (stCanPacket *pInCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pReadMsg, bool bRecvResult);
void 		  PassThruReadMsgs(stPASSTHRU_MSG *pReadMsg, unsigned int uiReadMsgLen, unsigned int eInCommType);
void		  CAN_ClearCarbCanRecvInfo();
void 		  CAN_uDelay(unsigned int uiDelay);
void 		  CAN_SendNotiRcvMultiFrame( ePKT_TD eInCommType );

extern unsigned int OemWriteCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
extern U8 OemReadCanBuff(U8 *pRxBuff, U32 timeout);
extern U8 GetKlineSelect(void);
extern void OemWriteCanBuff1(unsigned char* pBuff, unsigned char ch);
bool CheckNewUDS( uint32_t unProtocolID );
/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
unsigned int	g_uiCanReadMsgLength;
unsigned int	g_uiCanRxConsFrameNo;
unsigned int	g_uiCanWriteMsgLength;
unsigned int	g_uiCanTxConsFrameNo;
unsigned int	g_uiCanTxSequenceNo;
BOOL			g_bCanRxConcequtiveFrame = FALSE;
BOOL			g_bCanRxPendingFrame = FALSE;
BOOL			g_bCanRxCarbFrame = FALSE;

BOOL			g_bRcvMultiFrame = false;
uint32_t		g_uiRecvMultiFrameOldtime = 0;

stGITSetConfig 	g_stECUSetConfig;
CARB_REVC_INFO  g_CarbRecvInfo[MAX_CARB_RECV_ARRAY_CNT];

U8 				g_ucCanExceptState = 0;
U32 			g_uiCANTxID;
BOOL			g_bAckflag=0;
BOOL			g_bJ2534AckModeStatus=1;
U32 			g_uiAckTiming = 0;

uint32_t 		g_ulPGN = 0;
uint8_t 		g_ucCanEx_Func = 0;
bool			g_bCanEx_First = 0;
uint8_t 		g_ucCanEx_State = 0;
uint32_t 		g_ulRetryCnt_EX = 0;
uint32_t		g_ulTxdRxdCount = 0;
U16				g_u16CanDataTxlen_SMK;					// ISO15765_SMK
U16				g_u16CanDataTxlen_SMK_endframe;			// ISO15765_SMK
U8				g_ucCan_DLC;							//ISO15765_SMK
U8				g_ucCan_FC; 							//ISO15765_SMK
extern bool 	g_bKLogOnTxFlag;
extern bool 	g_bKLogOnRxFlag;

// message queue
osMessageQId	hOBDTxMessage;
osMessageQId	hOBDRxMessage;
osMessageQId	hWriteMsg;
osMessageQId	hDiagMsg;
osMessageQId	hOBDKlineTxMessage;
osMessageQId	hOBDKlineRxMessage;

// thread
osThreadId		hOBDcanTxTh;
osThreadDef( OBDcanTxTh, OBDCanTxThread, osPriorityNormal, 0, 1100 );
osThreadId		hOBDcanRxTh;
osThreadDef( OBDcanRxTh, OBDCanRxThread, osPriorityNormal, 0, 1100 );
osThreadId		hOBDKlineTxTh;
osThreadDef( OBDKlineTxTh, OBDKlineTxThread, osPriorityNormal, 0, 1100 );
osThreadId		hOBDKlineRxTh;
osThreadDef( OBDKlineRxTh, OBDKlineRxThread, osPriorityNormal, 0, 2100 );

bool                g_bSendPeriodicFlag = false;
uint32_t            g_uiSendPeriodicOldTime = 0;
stPERIODIC_MSG_INFO stPeriodicMsgInfo[MAX_PERIODICMSG_CNT];
stACK_MSG_INFO stAckMsgInfo;
bool g_bCF_TxComplete=0;// Ignore flow control out of sequence

// extern Global Variables ---------------------------------------------------//
extern stGITSetConfig 		g_stGITSetConfig;
extern U8		 			g_ucCAN_CH;
extern osPoolId		hDiagPool;
extern osPoolId		hPTPKPool;
extern stRECORD_HW_SET		g_stGITHWSetData;
extern U8					g_ucUDS_Data_Struct;
extern BOOL	g_bIsFastInit;
extern u32	intDlccomCount;
extern uint32_t g_ulProtocolID;
extern BOOL	g_bFastInit_Success;
extern U8 g_ucVehicle_Current_Read;
extern uint8_t g_b1003Lock;
extern uint8_t g_b3002Lock;
#ifdef PRINT_MESSAGE_ID
	extern stMESSAGE_ID_INFO stMessageIdInfo[20];
 	extern uint8_t ucMessageIdInfoCnt;
#endif
#ifdef CANFD_qhyek //Q_hyek CANFD
extern uint8_t g_ucCanformat;
extern U8 g_ucCanfd_dlc;
#endif
extern u32	intTxdRxdCount;

/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/
char initOBDComm( void )
{
	// To Write Thread MessageQ
	osMessageQDef( writequeue, MESSAGE_WRITE_QUEUE_SIZE, int );
	hWriteMsg = osMessageCreate( osMessageQ( writequeue ), NULL );
	if( hWriteMsg == NULL ) 
	{
		GLogE( "error... osMessageCreate hWriteMsg\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hWriteMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hWriteMsg",sizeof("hWriteMsg"));
#endif


	// To Diagnositc Thread MessageQ
	osMessageQDef( diagqueue, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, int );
	hDiagMsg = osMessageCreate( osMessageQ( diagqueue ), NULL );
	if( hDiagMsg == NULL )
	{
		GLogE( "error... osMessageCreate hDiagMsg\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hDiagMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hDiagMsg",sizeof("hDiagMsg"));
#endif


	// transmit message
	osMessageQDef( obdtxqueue, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, int );
	hOBDTxMessage = osMessageCreate( osMessageQ( obdtxqueue ), NULL );
	if( hOBDTxMessage == NULL )
	{
		GLogE( "error... osMessageCreate hOBDTxMessage\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hOBDTxMessage;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hOBDTxMessage",sizeof("hOBDTxMessage"));
#endif

	osMessageQDef( obdklinetxqueue, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, int );
	hOBDKlineTxMessage = osMessageCreate( osMessageQ( obdklinetxqueue ), NULL );
	if( hOBDKlineTxMessage == NULL )
	{
		GLogE( "error... osMessageCreate hOBDKlineTxMessage\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hOBDKlineTxMessage;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hOBDKlineTxMessage",sizeof("hOBDKlineTxMessage"));
#endif


	// receive message
	osMessageQDef( obdrxqueue, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, int );
	hOBDRxMessage = osMessageCreate( osMessageQ( obdrxqueue ), NULL );
	if( hOBDRxMessage == NULL )
	{
		GLogE( "error... osMessageCreate hOBDRxMessage\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hOBDRxMessage;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hOBDRxMessage",sizeof("hOBDRxMessage"));
#endif

	osMessageQDef( obdklinerxqueue, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, int );
	hOBDKlineRxMessage = osMessageCreate( osMessageQ( obdklinerxqueue ), NULL );
	if( hOBDKlineRxMessage == NULL )
	{
		GLogE( "error... osMessageCreate hOBDKlineRxMessage\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hOBDKlineRxMessage;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hOBDKlineRxMessage",sizeof("hOBDKlineRxMessage"));
#endif

	// create thread
	hOBDcanTxTh = osThreadCreate( osThread(OBDcanTxTh), NULL );
	if( hOBDcanTxTh == NULL )
	{
		GLogE( "Error... fail create hOBDcanTxTh Thread!!!\r\n" );
	}
	hOBDKlineTxTh = osThreadCreate( osThread(OBDKlineTxTh), NULL );
	if( hOBDKlineTxTh == NULL )
	{
		GLogE( "Error... fail create hOBDKlineTxTh Thread!!!\r\n" );
	}
	hOBDcanRxTh = osThreadCreate( osThread(OBDcanRxTh), NULL );
	if( hOBDcanRxTh == NULL )
	{
		GLogE( "Error... fail create hOBDcanRxTh Thread!!!\r\n" );
	}
	hOBDKlineRxTh = osThreadCreate( osThread(OBDKlineRxTh), NULL );
	if( hOBDKlineRxTh == NULL )
	{
		GLogE( "Error... fail create hOBDKlineRxTh Thread!!!\r\n" );
	}

	stPeriodicMsgInfo[0].ucTimerIndex = SetSWTimer( 0, eSWTimer_NONE, VCI_PeriodicMessage0, false );
	stPeriodicMsgInfo[1].ucTimerIndex = SetSWTimer( 0, eSWTimer_NONE, VCI_PeriodicMessage1, false );
	stPeriodicMsgInfo[2].ucTimerIndex = SetSWTimer( 0, eSWTimer_NONE, VCI_PeriodicMessage2, false );
	stPeriodicMsgInfo[3].ucTimerIndex = SetSWTimer( 0, eSWTimer_NONE, VCI_PeriodicMessage3, false );
	stPeriodicMsgInfo[4].ucTimerIndex = SetSWTimer( 0, eSWTimer_NONE, VCI_PeriodicMessage4, false );
	
	return INIT_OK;
}
void deinitOBDComm( void )
{
	// Terminate mmc all thread
	if( osThreadGetState(  hOBDcanRxTh ) != osThreadDeleted)
		osThreadTerminate( hOBDcanRxTh );
	
	if( osThreadGetState(  hOBDcanTxTh ) != osThreadDeleted)
		osThreadTerminate( hOBDcanTxTh );
}
void CAN_InitVariable(void)
{
	g_bCanRxConcequtiveFrame = FALSE;
	g_bCanRxPendingFrame = FALSE;
	g_bCanRxCarbFrame = FALSE;
	g_uiCanReadMsgLength 	= 0;
	g_uiCanWriteMsgLength 	= 0;
	CAN_ClearCarbCanRecvInfo();
	g_ucCanExceptState		= 0;
	g_bRcvMultiFrame		= false;
}

void OBDCanTxThread( void const *argument )
{
	osEvent		evt;
	MsgDiag_t	*message;
	PTmsgPkt_t	*packet;
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;
	stCanPacket stOutCanPacket;
	U8			bStandardCan;
	int iCanTxID32=0;

	for(;;)
	{
		evt	= osMessageGet( hOBDTxMessage, osWaitForever );
		if( evt.status == osEventMessage )
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hOBDTxMessage));
#endif
			message = ( MsgDiag_t * )evt.value.p;
			packet	= ( PTmsgPkt_t* )message->pPacket;

			pDiagmsg = ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
			if( pDiagmsg == NULL )
			{
			  	message = NULL;
				break;
			}
			pPTpacket = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
			if( pPTpacket == NULL )
			{
				osPoolFree( hDiagPool, (void *)pDiagmsg );
				break;
			}
			memcpy(pPTpacket, packet, sizeof(PTmsgPkt_t));
			memcpy(pDiagmsg, message, sizeof(MsgDiag_t));
			pDiagmsg->pPacket = pPTpacket;

			osPoolFree( hPTPKPool, (void *)packet );
			osPoolFree( hDiagPool, (void *)message );

			switch ( pDiagmsg->subEvent  )
			{
				case eCAN_TX_NONE_PARSING:
				{
					//pPTpacket->DataSize = 0;		//rx 초기화
#if defined(DEBUG_CAN_LOG)
					GLogN("[%s] eCAN_TX_NONE_PARSING\r\n", __FUNCTION__);
#endif
					pDiagmsg->subEvent = CAN_TxParsing(&stOutCanPacket, pPTpacket, &bStandardCan);

					if(osMessageAvailableSpace(hOBDTxMessage) == 0)
					{
						osPoolFree( hPTPKPool, (void *)pPTpacket );
						osPoolFree( hDiagPool, (void *)pDiagmsg );
					}
					else
					{
						osMessagePut( hOBDTxMessage, (uint32_t)pDiagmsg, osWaitForever );
					}
					break;
				}
				case eCAN_TX_SINGLE_FRAME:
				case eCAN_TX_SINGLE_FRAME_29BIT:
				{
#if defined(DEBUG_CAN_LOG)
					GLogN("[%s] eCAN_TX_SINGLE_FRAME\r\n", __FUNCTION__);
#endif
                    StopFunctionalPeriodicMsg();
					g_ucCanExceptState = 0;
					pDiagmsg->subEvent = CAN_MakeTxSingleFrame(&stOutCanPacket, bStandardCan, pPTpacket);

					if(GetCurFwServiceMode()!=eApp_Inside)	g_bAckflag = 1;
					intDlccomCount = g_uiAckTiming; // Ack Timming
					iCanTxID32 = (pPTpacket->pData[0]<<24)+
								 (pPTpacket->pData[1]<<16)+
								 (pPTpacket->pData[2]<<8)+
								 (pPTpacket->pData[3]);

					if( g_ulProtocolID == ISO15765_CARB_29BIT ) //20090219 kyc obd29bit
					{
						g_ulPGN = 0x1234;
					}
					else if( iCanTxID32 == 0x18DBFFF9){}	//140219 LWH 추가 - 해당 CAN ID는 Functional Data로만 쓴다. pc에서는 무조건 p3_min=0 이어야 한다.
					else
					{
						//if(PassThruRecvMsg[DlcTxBlock].Data[2]==0x33) PassThruRecvMsg[DlcTxBlock].Data[2]=0x00;	// 상용 tpms id 33 사용으로 이 예외처리가 들어가서 통신오류, 들어간 이유를 모르겠음.
						g_ulPGN = pPTpacket->pData[3]*0x100+pPTpacket->pData[2];
					}

					pPTpacket->DataSize = 0;		//rx 초기화
					if(osMessageAvailableSpace(hOBDRxMessage) == 0)
					{
						osPoolFree( hPTPKPool, (void *)pPTpacket );
						osPoolFree( hDiagPool, (void *)pDiagmsg );
					}
					else
					{
#ifdef USE_RELAY_MOSA
						if( g_ulProtocolID == BAT_RELAY_CON || g_ulProtocolID == BAT_FD_RELAY_CON )
						{
							osPoolFree( hPTPKPool, (void *)pPTpacket );
							osPoolFree( hDiagPool, (void *)pDiagmsg );
						}
						else
#endif
						{
							osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg, osWaitForever );
						}
					}
					break;
				}
				case eCAN_TX_SINGLE_FRAME_EX:
				{
#if defined(DEBUG_CAN_LOG)
					GLogN("[%s] eCAN_TX_SINGLE_FRAME_EX\r\n", __FUNCTION__);
#endif					
					g_ucCanExceptState = 0;
					pDiagmsg->subEvent = CAN_MakeTxSingleFrame_EX(&stOutCanPacket, bStandardCan, pPTpacket);

					pPTpacket->DataSize = 0;		//rx 초기화
					if ( pPTpacket->RxStatus != 1 )
					{
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
					  	osPoolFree( hPTPKPool, (void *)pPTpacket );
						osPoolFree( hDiagPool, (void *)pDiagmsg );
						pPTpacket->RxStatus = 0;
					}
					break;
				}

				case eCAN_TX_FIRST_FRAME:
				case eCAN_TX_FIRST_FRAME_29BIT:
				{
#if defined(DEBUG_CAN_LOG)
					GLogN("[%s] eCAN_TX_FIRST_FRAME\r\n", __FUNCTION__);
#endif
                    StopFunctionalPeriodicMsg();
					g_ucCanExceptState = 0;
					pDiagmsg->subEvent = CAN_MakeTxFirstFrame(&stOutCanPacket, bStandardCan, pPTpacket);

					if(GetCurFwServiceMode()!=eApp_Inside)	g_bAckflag = 1;
					intDlccomCount = g_uiAckTiming; // Ack Timming

					if(g_ulProtocolID==ISO15765_SMK)
					{
						GLogE("ISO15765_SMK FIRST_FRAME\r\n");

						pDiagmsg->subEvent = eCAN_TX_CONSECUTIVE_FRAME;

						if(osMessageAvailableSpace(hOBDTxMessage) == 0)
						{
							osPoolFree( hPTPKPool, (void *)pPTpacket );
							osPoolFree( hDiagPool, (void *)pDiagmsg );
						}
						else
						{
							osMessagePut( hOBDTxMessage, (uint32_t)pDiagmsg, osWaitForever );
						}
						break;
					}

					if( g_ulProtocolID == ISO15765_CARB_29BIT ) //20090219 kyc obd29bit		//130111 LWH SINGLE에 있던 PGN 설정 가지고 옴
					{
						g_ulPGN = 0x1234;
					}
					else
					{
						//if(PassThruRecvMsg[DlcTxBlock].Data[2]==0x33) PassThruRecvMsg[DlcTxBlock].Data[2]=0x00;	// 상용 tpms id 33 사용으로 이 예외처리가 들어가서 통신오류, 들어간 이유를 모르겠음.
						g_ulPGN = pPTpacket->pData[3]*0x100+pPTpacket->pData[2];
					}

					if(osMessageAvailableSpace(hOBDRxMessage) == 0)
					{
						osPoolFree( hPTPKPool, (void *)pPTpacket );
						osPoolFree( hDiagPool, (void *)pDiagmsg );
					}
					else
					{
						osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg, osWaitForever );
					}
					break;
				}
				case eCAN_TX_CONSECUTIVE_FRAME:
				case eCAN_TX_CONSECUTIVE_FRAME_29BIT:
				{
#if defined(DEBUG_CAN_LOG)
					GLogN("[%s] eCAN_TX_CONSECUTIVE_FRAME\r\n", __FUNCTION__);
#endif
					pDiagmsg->subEvent = CAN_MakeTxConsecutiveFrame(&stOutCanPacket, bStandardCan, pPTpacket);	

					if(GetCurFwServiceMode()!=eApp_Inside)	g_bAckflag = 1;
					intDlccomCount = g_uiAckTiming; // Ack Timming

					if( pDiagmsg->subEvent == eCAN_TX_CONSECUTIVE_FRAME )
					{
						if(osMessageAvailableSpace(hOBDTxMessage) == 0)
						{
							osPoolFree( hPTPKPool, (void *)pPTpacket );
							osPoolFree( hDiagPool, (void *)pDiagmsg );
						}
						else
						{
							osMessagePut( hOBDTxMessage, (uint32_t)pDiagmsg, osWaitForever );
						}
					}
					else
					{
						pPTpacket->DataSize = 0;		//rx 초기화
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
					break;
				}
				default:
					break;
			}
		}
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
    osThreadYield();
#endif
	}
}

void OBDCanRxThread( void const *argument )
{
	osEvent		evt;
	MsgDiag_t	*message;
	PTmsgPkt_t	*packet;
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;
	stCanPacket g_InRxCanPacket;
	eCanType	bStandardCan;
	eDiagCanState eCanRxState;

	for(;;)
	{
		evt	= osMessageGet( hOBDRxMessage, osWaitForever );
		if( evt.status == osEventMessage )
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hOBDRxMessage));
#endif
			message = ( MsgDiag_t * )evt.value.p;
			packet	= ( PTmsgPkt_t* )message->pPacket;

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
			memcpy(pPTpacket, packet, sizeof(PTmsgPkt_t));
			memcpy(pDiagmsg, message, sizeof(MsgDiag_t));
			pDiagmsg->pPacket = pPTpacket;
//GLogN("---psize:%d---%X,%d\r\n",pPTpacket->DataSize,pPTpacket,pDiagmsg->subEvent);
			osPoolFree( hPTPKPool, (void *)packet );
			osPoolFree( hDiagPool, (void *)message );

			if ( pDiagmsg->subEvent == eCAN_RX_BLOCK )
			{//GLogN("R");
				if ( CAN_FindCanPacket(&g_InRxCanPacket, &bStandardCan) )
				{
					intDlccomCount = g_uiAckTiming;//Disable ACK while receiving 'consecutive frames'
					eCanRxState = CAN_RxBlockProc(&g_InRxCanPacket, bStandardCan, pPTpacket);
					if( g_bRcvMultiFrame == true )
					{
						if( Get_TmrDelta( Get_Tmr(), g_uiRecvMultiFrameOldtime ) > 2000 )
						{
					  		CAN_SendNotiRcvMultiFrame( (ePKT_TD)pDiagmsg->mPktType );
							g_uiRecvMultiFrameOldtime = Get_Tmr();
						}
					}
					
				//GLogN("---size:%d---%d\r\n",pPTpacket->DataSize,eCanRxState);
					switch(eCanRxState)
					{
						case eCAN_RX_BLOCK :
						{
							if(osMessageAvailableSpace(hOBDRxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)pPTpacket );
								osPoolFree( hDiagPool, (void *)pDiagmsg );
							}
							else
							{
								osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg, osWaitForever );
							}
							break;
						}
						case eCAN_RX_COMPLETE :
						{
							//PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType);
                            if( g_bCanRxPendingFrame == FALSE )
                            {
                                ContinueFunctionalPeriodicMsg();
                            }
							pDiagmsg->subEvent =  eDIAG_COMM_RX_OK;
							if(osMessageAvailableSpace(hDiagMsg) == 0)
							{
								osPoolFree( hPTPKPool, (void *)pPTpacket );
								osPoolFree( hDiagPool, (void *)pDiagmsg );
							}
							else
							{
								osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
							}
							g_b3002Lock = false;
							break;
						}
						case eCAN_TX_FLOWCONTROL_FRAME :
						{
							eCanRxState = CAN_MakeTxFlowControlFrame(&g_InRxCanPacket, bStandardCan, pPTpacket);	
							if(osMessageAvailableSpace(hOBDRxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)pPTpacket );
								osPoolFree( hDiagPool, (void *)pDiagmsg );
							}
							else
							{
								osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg, osWaitForever );
							}
							break;
						}
						case eCAN_TX_CONSECUTIVE_FRAME :
						{
							pDiagmsg->subEvent = eCanRxState;
							if(osMessageAvailableSpace(hOBDTxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)pPTpacket );
								osPoolFree( hDiagPool, (void *)pDiagmsg );
							}
							else
							{
								osMessagePut( hOBDTxMessage, (uint32_t)pDiagmsg, osWaitForever );
							}
							break;
						}
						case eCAN_RX_PENDING :
						{
						  	MsgDiag_t	*pDiagmsg_pending;
                            PTmsgPkt_t	*pPTpacket_pending;
                            pDiagmsg_pending = ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
                            if( pDiagmsg_pending == NULL )
                            {
                                break;
                            }
                            pPTpacket_pending = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
                            if( pPTpacket_pending == NULL )
                            {
                                osPoolFree( hDiagPool, (void *)pDiagmsg_pending );
                                break;
                            }
							
							pDiagmsg_pending->mMsgType 		= MSG_DIAG;
                            pDiagmsg_pending->mPktType 		= message->mPktType;
							pDiagmsg_pending->event         = pDiagmsg->event;
							pDiagmsg_pending->subEvent 		= eCAN_RX_BLOCK;
							pDiagmsg_pending->unEventTime 	= GetUnixTime();
							pDiagmsg_pending->pPacket 		= (void *)pPTpacket_pending;
						  	memcpy(&(pPTpacket_pending->UUID), &(packet->UUID), sizeof(packet->UUID));
							
						  	pDiagmsg->subEvent	= eDIAG_COMM_RX_PENDING;
							if(osMessageAvailableSpace(hDiagMsg) == 0)
							{
								osPoolFree( hPTPKPool, (void *)pPTpacket );
								osPoolFree( hDiagPool, (void *)pDiagmsg );
							}
							else
							{
								osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
							}
							
                            if(osMessageAvailableSpace(hOBDRxMessage) == 0)
                            {
                                osPoolFree( hPTPKPool, (void *)pPTpacket_pending );
                                osPoolFree( hDiagPool, (void *)pDiagmsg_pending );
                            }
                            else
                            {
                                osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg_pending, osWaitForever );
                            }
							break;
						}
						default:					//eCAN_NONE_STATE
						{
							osPoolFree( hPTPKPool, (void *)pPTpacket );
							osPoolFree( hDiagPool, (void *)pDiagmsg );
							
							CAN_ClearCarbCanRecvInfo();
							g_uiCanReadMsgLength = 0;
							eCanRxState = eCAN_NONE_STATE;
							break;
						}
					}
				}
				else//CAN No Response
				{
				  	unsigned long uiProtocolID;
					uiProtocolID = VCI_GetPassThruProtocolID();
					//PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType);
					if((uiProtocolID == ISO15765_CARB)				||
					   	(uiProtocolID == ISO15765_CARB_NEW)			||
					   	(uiProtocolID == ISO15765_CARB_NEW_LENGTH)	||
						(uiProtocolID == J1939_23_CARB_NEW_LENGTH)	||
						(uiProtocolID == ISO15765_CARB_29BIT_NEW)	||
						(uiProtocolID == J1939_23_CARB_29BIT_NEW)	||
						(uiProtocolID == ISO15765_ACU_SINGLE)	||
						(uiProtocolID == ISO15765_CARB_29BIT))
					{
						if(uiProtocolID == ISO15765_ACU_SINGLE)
						{
							GLogN("g_uiCanReadMsgLength:%d\r\n",g_uiCanReadMsgLength);
							if(g_uiCanReadMsgLength>MAX_PASSTHRUMSG_DATA_SIZE)
							{
								GLogE("overflow MAX_PASSTHRUMSG_DATA_SIZE!!!\r\n");
								g_uiCanReadMsgLength=MAX_PASSTHRUMSG_DATA_SIZE;
							}
						}
					  	pDiagmsg->subEvent =  eDIAG_COMM_RX_OK;
						pPTpacket->DataSize = g_uiCanReadMsgLength;
						g_uiCanReadMsgLength = 0;
						if(osMessageAvailableSpace(hDiagMsg) == 0)
						{
							osPoolFree( hPTPKPool, (void *)pPTpacket );
							osPoolFree( hDiagPool, (void *)pDiagmsg );
						}
						else
						{
							osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
						}
					}
					else//CAN No Response
					{
					  	pPTpacket->DataSize = g_uiCanReadMsgLength = 0;//buff clear
					  	if(((uiProtocolID == ISO14229_ES95486_02_100)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_102)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_103)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_104)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_105)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_106)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_107)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_108)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_109)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_10A)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_10B)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_10C)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_10D)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_10E)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_10F)||				//20190918 Jay
							(uiProtocolID == ISO14229_ES95486_02_100_NEW)||		//20190918 Jay
#ifdef HOTA
                            (uiProtocolID == ISO14229_ES95486_02_HOTA)||
#endif
                              
#ifdef CANFD_PROTOCOL
                            (uiProtocolID == ISO14229_ES95486_02_130_CANFD)||
                            (uiProtocolID == ISO14229_ES95486_02_131_CANFD)||
#endif
							(uiProtocolID == ISO14230_ES95486_DOIP_120)||
							(uiProtocolID == ISO14230_ES95486_DOIP_121)||
							(uiProtocolID == ISO14230_ES95486_DOIP_122)||
							(uiProtocolID == ISO14230_ES95486_DOIP_123)||
							(uiProtocolID == ISO14230_ES95486_DOIP_124)||
							(uiProtocolID == ISO14230_ES95486_DOIP_125)||
							(uiProtocolID == ISO14230_ES95486_DOIP_126)||
							(uiProtocolID == ISO14230_ES95486_DOIP_127)||
							(uiProtocolID == ISO14230_ES95486_DOIP_128)||
							(uiProtocolID == ISO14230_ES95486_DOIP_129)||
							(uiProtocolID == ISO14230_ES95486_DOIP_12A)||
							(uiProtocolID == ISO14230_ES95486_DOIP_12B)||
							(uiProtocolID == ISO14230_ES95486_DOIP_12C)||
							(uiProtocolID == ISO14230_ES95486_DOIP_12D)||
							(uiProtocolID == ISO14230_ES95486_DOIP_12E)||
							(uiProtocolID == ISO14230_ES95486_DOIP_12F)||
		                    (uiProtocolID == ISO14229_ES95486_170)||
		                    (uiProtocolID == ISO14229_ES95486_170_MMCAN)||
		                    (uiProtocolID == ISO14230_ES95486_170)||
		                    (uiProtocolID == ISO14230_ES95486_170_MMCAN)
#ifdef NEW_29BIT_CAN
                            || (uiProtocolID == ISO15765_ES95486_29bit)
                            || (uiProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
                            || (uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
                            || (uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
                              ))
					  	{
							pDiagmsg->subEvent =  eDIAG_COMM_RX_FAIL;	//Respond to tablet with '0'
							if( g_bRcvMultiFrame == true )
								g_bRcvMultiFrame = false;
						}
						else
						{
						  	GLogI("@");
						  	g_diagpairflag = false;
							pDiagmsg->subEvent =  eDIAG_COMM_RX_FAIL;		//No response sent to tablet
							g_b1003Lock = false;
						}
						if(osMessageAvailableSpace(hDiagMsg) == 0)
						{
							osPoolFree( hPTPKPool, (void *)pPTpacket );
							osPoolFree( hDiagPool, (void *)pDiagmsg );
						}
						else
						{
							osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
						}
						g_b3002Lock = false;
					}
					CAN_ClearCarbCanRecvInfo();
                    ContinueFunctionalPeriodicMsg();
				}
			}
			else if( pDiagmsg->subEvent == eCAN_RX_BLOCK_EX )
			{
			  	eCanRxState = CAN_RxBlockProc_EX(&g_InRxCanPacket, bStandardCan, pPTpacket);
				switch( eCanRxState )
				{
					case eCAN_RX_BLOCK_EX:
					{
					  	pPTpacket->DataSize = 0;		//rx 초기화
						if(osMessageAvailableSpace(hOBDRxMessage) == 0)
						{
							osPoolFree( hPTPKPool, (void *)pPTpacket );
							osPoolFree( hDiagPool, (void *)pDiagmsg );
						}
						else
						{
							osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg, osWaitForever );
						}
						break;
					}
					case eCAN_TX_SINGLE_FRAME_EX:
					{
					  	pDiagmsg->subEvent = eCanRxState;
						if(osMessageAvailableSpace(hOBDTxMessage) == 0)
						{
							osPoolFree( hPTPKPool, (void *)pPTpacket );
							osPoolFree( hDiagPool, (void *)pDiagmsg );
						}
						else
						{
							osMessagePut( hOBDTxMessage, (uint32_t)pDiagmsg, osWaitForever );
						}
						
//						pDiagmsg->subEvent = eCAN_RX_BLOCK_EX;
//						if(osMessageAvailableSpace(hOBDRxMessage) == 0)
//						{
//							osPoolFree( hPTPKPool, (void *)pPTpacket );
//							osPoolFree( hDiagPool, (void *)pDiagmsg );
//						}
//						else
//						{
//							osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg, osWaitForever );
//						}
						break;
					}
					case eCAN_RX_COMPLETE :
					{
						//PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType);
					  	if( g_ucRecvPassThruWriteMsg > 0)	g_ucRecvPassThruWriteMsg--;
						pDiagmsg->subEvent =  eDIAG_COMM_RX_OK;
						if(osMessageAvailableSpace(hDiagMsg) == 0)
						{
							osPoolFree( hPTPKPool, (void *)pPTpacket );
							osPoolFree( hDiagPool, (void *)pDiagmsg );
						}
						else
						{
							osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
						}
						g_b3002Lock = false;
						break;
					}
					default :
					{
						osPoolFree( hPTPKPool, (void *)pPTpacket );
						osPoolFree( hDiagPool, (void *)pDiagmsg );
						if( g_ucRecvPassThruWriteMsg > 0)	g_ucRecvPassThruWriteMsg--;
						CAN_ClearCarbCanRecvInfo();
						g_uiCanReadMsgLength = 0;
						eCanRxState = eCAN_NONE_STATE;
						break;
					}
				}
			}
		}
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
    osThreadYield();
#endif
	}
}

void ClearRxCanState()
{
	osEvent		evt;
	MsgDiag_t	*message;
	PTmsgPkt_t	*packet;

	//printf("%s] enter\r\n", __func__);
	for(int ii=0;ii<MESSAGE_DIAGNOSTIC_QUEUE_SIZE ;ii++)
	{
		evt = osMessageGet( hOBDRxMessage, 0 );
		if( evt.status == osEventMessage )
		{
			message = ( MsgDiag_t * )evt.value.p;
			packet	= ( PTmsgPkt_t* )message->pPacket;

			printf("%s] del rx packet old addr : %x\r\n", __func__, packet);

			osPoolFree( hPTPKPool, (void *)packet );
			osPoolFree( hDiagPool, (void *)message );
		}				
	}

}

void ClearTxCanState()
{
	osEvent		evt;
	MsgDiag_t	*message;
	PTmsgPkt_t	*packet;

	//printf("%s] enter\r\n", __func__);
	for(int ii=0;ii<MESSAGE_DIAGNOSTIC_QUEUE_SIZE ;ii++)
	{
		evt = osMessageGet( hOBDTxMessage, 0 );
		if( evt.status == osEventMessage )
		{
			message = ( MsgDiag_t * )evt.value.p;
			packet	= ( PTmsgPkt_t* )message->pPacket;

			printf("%s] del tx packet old addr : %x\r\n", __func__, packet);

			osPoolFree( hPTPKPool, (void *)packet );
			osPoolFree( hDiagPool, (void *)message );
		}				
	}
}

U32 CAN_SavePassThruWriteMsg(void)
{	
	eDiagCanState eCanRxState;

	g_uiCanWriteMsgLength = 0;
	eCanRxState = eCAN_TX_NONE_PARSING;
	
	return eCanRxState;
}
U32 KLINE_SavePassThruWriteMsg(void)
{	
	eDiagKLINEState eKLINERxState;

	//g_uiCanWriteMsgLength = 0;
	eKLINERxState = eKLINE_TX_BLOCK;
	
	return eKLINERxState;
}


BOOL CAN_CheckStandardTxFrameType(unsigned short *pusDLCLength, stPASSTHRU_MSG *pWriteMsg)
{
	BOOL bIsMultiFrame;//, bLengthBackWard = FALSE;
	unsigned char ucCanMultiCheckLen;
	unsigned long uiProtocolID;
	unsigned short usCalcDLCLength;

	uiProtocolID = VCI_GetPassThruProtocolID();

#ifdef USE_RELAY_MOSA
	if( g_ucCanformat == CAN_FRAMEFORMAT_FDCAN )
		ucCanMultiCheckLen = 63;
	else
#endif
		ucCanMultiCheckLen = 7;
		
	//bIsMultiFrame = (pWriteMsg->pData[2] > ucCanMultiCheckLen) ? TRUE:FALSE;
	if(pWriteMsg->pData[2] > ucCanMultiCheckLen) bIsMultiFrame=TRUE;
	else bIsMultiFrame=FALSE;

	if((uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE)||
	  ( uiProtocolID == ISO15765_SINGLE )||
	  ( uiProtocolID == ISO15765_SINGLE_PODS )||
	  ( uiProtocolID == ISO15765_ACU_SINGLE )||
	  ( uiProtocolID == ISO15765_SMK)||//for reprogram rom id read 20221211 kkt
	  ( uiProtocolID == ISO15765_SINGLE_SMK ))
	{
		bIsMultiFrame = FALSE;	//pWriteMsg->pData[2] 와 무관하게 single이다
	}
	

#ifdef USE_RELAY_MOSA
	if( uiProtocolID == BAT_RELAY_CON || uiProtocolID == BAT_FD_RELAY_CON )
	{
		bIsMultiFrame = false;
	}
#endif

	if ( bIsMultiFrame == TRUE)
	{
		usCalcDLCLength = CAN_FRAME_DATA_SIZE;
		
		// This logic is not use now, in my opinion, if you need to use this logic you must fix App
		//if ( bLengthBackWard )
		//	usCalcDLCLength = ((pWriteMsg->pData[3]&0x0F)<<8) + pWriteMsg->pData[2];
		//else
		{
		  	usCalcDLCLength = pWriteMsg->pData[2];
		 	if( (uiProtocolID == ISO14229_ES95486_02_10A)||
				(uiProtocolID == ISO15765_NEW)||
				(uiProtocolID == ISO14229_ES95486_02_100_NEW)||
#ifdef HOTA
                (uiProtocolID == ISO14229_ES95486_02_HOTA)||
#endif
                  
#ifdef CANFD_PROTOCOL
                (uiProtocolID == ISO14229_ES95486_02_131_CANFD)||
#endif
				(uiProtocolID == ISO14230_ES95486_DOIP_121)||
				(uiProtocolID == ISO15765_CAN_HWSET_DB_NEW))
			{
			  	usCalcDLCLength = ((pWriteMsg->pData[2]&0x0F)<<8) + pWriteMsg->pData[3];
			}
		}
	}
	else
	{
		if ( uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE ) 
		{
			 usCalcDLCLength = pWriteMsg->DataSize - 0x02;
		}
		else if( uiProtocolID == ISO15765_SINGLE || uiProtocolID == ISO15765_SINGLE_PODS || uiProtocolID == ISO15765_ACU_SINGLE ) 
 		{
			usCalcDLCLength = CAN_FRAME_DATA_SIZE;
 		}
		else if( uiProtocolID == ISO15765_SMK ) 
 		{
			usCalcDLCLength = ((pWriteMsg->pData[3]&0x0F)<<8) + pWriteMsg->pData[2];
			if( ( (pWriteMsg->pData[3]&0xf0) == 0xf0 )|| 
					( (pWriteMsg->pData[3]&0xe0) == 0xe0 ))	// 첫번째 4094개 데이터 송신후 남은 데이터 송신인지 데이터 구분은 fx xx 의 'f' 로 구분하며
				{															// x xx 데이터 길이정보 2바이트는 빼고 이후 데이터만 보낸다.
					usCalcDLCLength = ((pWriteMsg->pData[3]&0x0f))*0x100+pWriteMsg->pData[2];
					g_u16CanDataTxlen_SMK=0;										// 0 일경우 x xx 길이정보 보내지 않기 위한 구분
					if((pWriteMsg->pData[3]&0x10) == 0x00 ) // e0와 f0의 차이는 0x10임 추후 4번째 프레임을 보낼 경우가 생긴다면 이부분 유의할것
							g_u16CanDataTxlen_SMK_endframe=1;					// 마지막 rx time 적용할지 안할지 구분 플래그
					else 
							g_u16CanDataTxlen_SMK_endframe=0;
				}
				else if( usCalcDLCLength > 4094)								// 1번째 많은거 보내는거.
				{
					g_u16CanDataTxlen_SMK=4094;
				}
				else
				{
					g_u16CanDataTxlen_SMK = usCalcDLCLength;
				}
				//UartPrintf("\n CanDataTxlen CanDataTxlen_SMK -> %x %x  \n ",CanDataTxlen,CanDataTxlen_SMK);	
				if( usCalcDLCLength > 6 )		bIsMultiFrame=TRUE;//CurMode = CAN_TX_FIRST_FRAME;
				else						bIsMultiFrame=FALSE;//CurMode = CAN_TX_SINGLE_FRAME;
				g_ucCan_FC = 0;
 		}
#ifdef HOTA
        else if ( uiProtocolID == ISO14229_ES95486_02_HOTA )
        {
            usCalcDLCLength = ((pWriteMsg->pData[2]&0x0F)<<8) + pWriteMsg->pData[3];
        }
#endif
		else
		{
			usCalcDLCLength = pWriteMsg->pData[2];
		}
	}
	
	*pusDLCLength = usCalcDLCLength;

	return bIsMultiFrame;
}

void CAN_MakeSendFrame(stCanPacket *pOutCanPacket, U8 bIsStandardCan, int nCanFrameType, stPASSTHRU_MSG *pWriteMsg)	//tx single, first, consecutive
{
	unsigned char *pCanDataFields;
	unsigned short usCanDLC=CAN_FRAME_DATA_SIZE;
	unsigned long uiProtocolID, ulDataIndex;
	U8 i;

	uiProtocolID = VCI_GetPassThruProtocolID();

	if ( bIsStandardCan == eCAN_CLASSIC_STANDARD) 
	{
		ulDataIndex = 2/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;
		pCanDataFields = pOutCanPacket->stNormalPacket.arrDataFields;
		//usCanDLC = pOutCanPacket->stNormalPacket.ucDLC;	//150914 lwh 불필요 해보인다 삭제
		if ( CheckNewUDS(uiProtocolID) )
			memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x55, SIZE_CAN_DATA_FIELD);
		else
			memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x00, SIZE_CAN_DATA_FIELD);
	}
#ifdef CANFD_qhyek //Q_hyek CANFD
	else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD) 
	{
		ulDataIndex = 2/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;
		pCanDataFields = pOutCanPacket->stFDStdPacket.arrDataFields;
		//usCanDLC = pOutCanPacket->stNormalPacket.ucDLC;
		if ( CheckNewUDS(uiProtocolID) )
			memset(pOutCanPacket->stFDStdPacket.arrDataFields, 0x55, SIZE_FDCAN_DATA_FIELD);
		else
			memset(pOutCanPacket->stFDStdPacket.arrDataFields, 0x00, SIZE_FDCAN_DATA_FIELD);
	}
#endif
	else
	{
		if( uiProtocolID == ISO15765_CARB_29BIT )
			ulDataIndex = 2/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;
		else
			ulDataIndex = 4/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;

		pCanDataFields = pOutCanPacket->stExtendPacket.arrDataFields;
		usCanDLC = pOutCanPacket->stExtendPacket.ucDLC;
        
        if ( ( CheckNewUDS(uiProtocolID) ) )
			memset(pOutCanPacket->stExtendPacket.arrDataFields, 0x55, SIZE_CAN_DATA_FIELD);
		else
			memset(pOutCanPacket->stExtendPacket.arrDataFields, 0x00, SIZE_CAN_DATA_FIELD);
	}
	
	if ( nCanFrameType == CAN_SINGLE_FRAME )
	{
	 	if(uiProtocolID == ISO15765_SINGLE || uiProtocolID == ISO15765_SINGLE_PODS || uiProtocolID == ISO15765_ACU_SINGLE)
		{
			usCanDLC = CAN_FRAME_DATA_SIZE;
			memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, CAN_FRAME_DATA_SIZE);
		}
		else if(uiProtocolID == ISO15765_SINGLE_SMK)
		{
			usCanDLC = pWriteMsg->pData[2];
			memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex, CAN_FRAME_DATA_SIZE);
		}
		else if(uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE || uiProtocolID == ISO15765_SMK)
		{
			usCanDLC = pWriteMsg->DataSize-2;
			memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, CAN_FRAME_DATA_SIZE);
        }
        else if(uiProtocolID == ISO15765_29BIT || uiProtocolID == ISO15765_CARB_29BIT || uiProtocolID == ISO15765_CARB_29BIT_NEW || uiProtocolID == J1939_23_CARB_29BIT_NEW ||uiProtocolID == ISO15765_29BIT_EXCEPT 
#ifdef NEW_29BIT_CAN
                || uiProtocolID == ISO15765_ES95486_29bit
                || uiProtocolID == ISO15765_ES95486_DOIP_29BIT
#ifdef CANFD_PROTOCOL
                || (uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
                || (uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
                  )//2018.08.06 LJH 솔라티 통신 오류 수정
        {
            usCanDLC = CAN_FRAME_DATA_SIZE;
			pCanDataFields[0] = pWriteMsg->pData[4];
			memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, pWriteMsg->pData[4]);
        }
		else if( uiProtocolID == ISO14229_ES95486_02_10F || uiProtocolID == ISO14230_ES95486_DOIP_12F ) // 25.09.29 0x10F, 0x12F 프로토콜 추가
        {
			if(bIsStandardCan == eCAN_CLASSIC_STANDARD)
			{
				usCanDLC = CAN_FRAME_DATA_SIZE;
				memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, CAN_FRAME_DATA_SIZE);
			}
			else
			{
				usCanDLC = CAN_FRAME_DATA_SIZE;
				pCanDataFields[0] = pWriteMsg->pData[4];
				memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, pWriteMsg->pData[4]);
			}
        }
		
#ifdef USE_RELAY_MOSA
		else if(uiProtocolID == BAT_FD_RELAY_CON || uiProtocolID == BAT_RELAY_CON )
		{
			usCanDLC = pWriteMsg->pData[2];
			memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex, pWriteMsg->pData[2]);
		}
#endif
#ifdef HOTA
        else if(uiProtocolID == ISO14229_ES95486_02_HOTA)
        {
            g_uiCanWriteMsgLength++;
            usCanDLC = CAN_FRAME_DATA_SIZE;
            pCanDataFields[0] = pWriteMsg->pData[3];
            memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex+1, pWriteMsg->pData[3]);
		}
#endif
		else
		{
			//usCanDLC = pWriteMsg->pData[2];		//ori
			//pCanDataFields[0] = usCanDLC;			//ori
			//memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, usCanDLC);		//ori
			usCanDLC = CAN_FRAME_DATA_SIZE;
			pCanDataFields[0] = pWriteMsg->pData[2];
			memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, pWriteMsg->pData[2]);
		}
	}
	else if ( nCanFrameType == CAN_FIRST_FRAME )
	{
		unsigned short usSendLength;
		
		if( uiProtocolID == ISO15765_NEW || 
		   	uiProtocolID == ISO15765_CAN_HWSET_DB_NEW ||
#ifdef HOTA
            uiProtocolID == ISO14229_ES95486_02_HOTA||
#endif
#ifdef CANFD_PROTOCOL
            uiProtocolID == ISO14229_ES95486_02_131_CANFD ||
#endif
		   	uiProtocolID == ISO14229_ES95486_02_10A ||
		   	uiProtocolID == ISO14230_ES95486_DOIP_121 ||
			uiProtocolID == ISO14229_ES95486_02_100_NEW)
		{
		  	g_uiCanWriteMsgLength++;		// 해당 프로토콜은 datasize가 2byte이다. 그래서 하나더 증가 시켜준다 150914 lwh
			usSendLength =  ((pWriteMsg->pData[2]&0x0F)<<8) + pWriteMsg->pData[3];
			usCanDLC = CAN_FRAME_DATA_SIZE;
			pCanDataFields[0] = CAN_FIRST_FRAME | ((usSendLength>>8) &0x0F);
			pCanDataFields[1] = (unsigned char)usSendLength;
			memcpy(pCanDataFields+2, pWriteMsg->pData+ulDataIndex+1, usCanDLC-2);
		}
        else if(uiProtocolID == ISO15765_29BIT || uiProtocolID == ISO15765_CARB_29BIT || uiProtocolID == ISO15765_CARB_29BIT_NEW || uiProtocolID == J1939_23_CARB_29BIT_NEW ||uiProtocolID == ISO15765_29BIT_EXCEPT 
#ifdef NEW_29BIT_CAN
                || uiProtocolID == ISO15765_ES95486_29bit
                || uiProtocolID == ISO15765_ES95486_DOIP_29BIT
#ifdef CANFD_PROTOCOL
                || (uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
                || (uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
                  )
        {
            g_uiCanWriteMsgLength++;
            usCanDLC = CAN_FRAME_DATA_SIZE;
            pCanDataFields[0] = pWriteMsg->pData[4];
            pCanDataFields[1] = pWriteMsg->pData[5];
            memcpy(pCanDataFields+2, pWriteMsg->pData+ulDataIndex+1, usCanDLC-2);
        }
		else if( uiProtocolID == ISO14229_ES95486_02_10F || uiProtocolID == ISO14230_ES95486_DOIP_12F )
		{
			// LJS 250105: 0x10F uses 1-byte Length, different from 0x100
			// 0x101: 2-byte Length
			// 0x10F: 1-byte Length
			// remove: g_uiCanWriteMsgLength++, pWriteMsg->pData+ulDataIndex + 1 -> pWriteMsg->pData+ulDataIndex
			if ( bIsStandardCan == eCAN_CLASSIC_STANDARD)
			{
				usSendLength = pWriteMsg->pData[2];	
			}
			else
			{
				usSendLength = pWriteMsg->pData[4];
			}
			usCanDLC = CAN_FRAME_DATA_SIZE;
			pCanDataFields[0] = CAN_FIRST_FRAME | ((usSendLength>>8) &0x0F);
			pCanDataFields[1] = (unsigned char)usSendLength;
			memcpy(pCanDataFields+2, pWriteMsg->pData+ulDataIndex, usCanDLC-2);
		}
		else if(uiProtocolID == ISO15765_SMK)
		{
			
			usCanDLC = CAN_FRAME_DATA_SIZE;
			memcpy(pCanDataFields, pWriteMsg->pData+ulDataIndex-1, CAN_FRAME_DATA_SIZE);
			if(g_u16CanDataTxlen_SMK==0)
			{
				for( i = 0; i < 8; i++ )
				{
					pCanDataFields[i] = pWriteMsg->pData[4+g_uiCanWriteMsgLength++];
				}							
			}
			else
			{
				for( i = 0; i < 6; i++ )
				{
					pCanDataFields[i+2] = pWriteMsg->pData[4+g_uiCanWriteMsgLength++];
				}
				pWriteMsg->DataSize = g_u16CanDataTxlen_SMK+2;	// pc가 최대 4096 바이트밖에 송신못하여 구분하여 보내도록 렝쓰 두바이트 제외하고 4094까지 끊어서 1회 보내기 위한 길이 재설정
			}
        }
		else
		{
			usSendLength = pWriteMsg->pData[2];
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
        g_uiCanTxConsFrameNo++;
        g_uiCanTxSequenceNo++;
		pCanDataFields[0] = CAN_CONSECUTICE_FRAME + (g_uiCanTxSequenceNo % 0x10);

#ifdef NEW_UDS_FILLER
        U32 unRestData = 0;

        if( (bIsStandardCan == eCAN_CLASSIC_STANDARD) || (bIsStandardCan == eCAN_FDFORMAT_STANDARD) )
        {
            unRestData = pWriteMsg->DataSize - CAN_STANDARD_ID_SIZE - CAN_DLC_BYTE_SIZE - g_uiCanWriteMsgLength;
            if( unRestData < CAN_FRAME_DATA_SIZE )
            {
                memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, unRestData);
            }
            else
            {
                memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, usCanDLC-1);
            }
        }
        else
        {
            unRestData = pWriteMsg->DataSize - CAN_EXTENDED_ID_SIZE - CAN_DLC_BYTE_SIZE - g_uiCanWriteMsgLength;
            if( unRestData < CAN_FRAME_DATA_SIZE )
            {
                memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, unRestData);
            }
            else
            {
                memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, usCanDLC-1);
            }
        }
#endif
        
		if(uiProtocolID == ISO15765_SMK)
		{
			for( i = 0; i < 8; i++ )
			{
				pCanDataFields[i] = pWriteMsg->pData[4+g_uiCanWriteMsgLength++];
				if( g_uiCanWriteMsgLength == (pWriteMsg->DataSize-2) ) break;
			}
			
			if(i<7)	
			{
				//usCanDLC=(7-i);					// Data Length 만큼만 보내기 위함.
				usCanDLC=(1+i);
			}
			else
			{
				//usCanDLC=0;					// Data Length 만큼만 보내기 위함.
				usCanDLC=8;							
			}
        }
	}
    
#ifdef CANFD_qhyek //Q_hyek CANFD
    if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD) 
    {
      //GLogN("\r\n g_ucCanfd_dlc : %d \r\n", g_ucCanfd_dlc);
      if(uiProtocolID==ISO14229_ES95486_02_130_CANFD) usCanDLC = 8;
	  else
	  {
	      if(g_ucCanfd_dlc <= 8) usCanDLC = 8;
	      else if(g_ucCanfd_dlc <= 12) usCanDLC = 12;
	      else if(g_ucCanfd_dlc <= 16) usCanDLC = 16;
	      else if(g_ucCanfd_dlc <= 20) usCanDLC = 20;
	      else if(g_ucCanfd_dlc <= 24) usCanDLC = 24;
	      else if(g_ucCanfd_dlc <= 32) usCanDLC = 32;
	      else if(g_ucCanfd_dlc <= 48) usCanDLC = 48;
	      else usCanDLC = 64;
	  }
    }
#endif

#ifdef USE_RELAY_MOSA
	if( uiProtocolID == BAT_FD_RELAY_CON )
	{
		usCanDLC = pWriteMsg->pData[2];
	}
#endif

	if ( bIsStandardCan == eCAN_CLASSIC_STANDARD )	pOutCanPacket->stNormalPacket.ucDLC = usCanDLC;
#ifdef CANFD_qhyek //Q_hyek CANFD
	else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD )	pOutCanPacket->stFDStdPacket.ucDLC = usCanDLC;
#endif
	else											pOutCanPacket->stExtendPacket.ucDLC = usCanDLC;

#if defined(DEBUG_CAN_LOG)
	GLogI("[%s]ulDataIndex %d, g_uiCanWriteMsgLength %d, ucDLC %d\r\n", 
					__FUNCTION__, ulDataIndex, g_uiCanWriteMsgLength, usCanDLC);
#endif
}

void CAN_MakeSendFrame_EX(stCanPacket *pOutCanPacket, U8 bIsStandardCan, int nCanFrameType, stPASSTHRU_MSG *pWriteMsg)	//tx single, first, consecutive
{
	unsigned char *pCanDataFields;
	unsigned short usCanDLC;
	unsigned long uiProtocolID, ulDataIndex;
	uint32_t nStartMask;
	uint32_t nEndMask;
	uint32_t nCanType;
	uint32_t nCANBaudrate;

	uiProtocolID = VCI_GetPassThruProtocolID();
	
	nCanType = EXTENDED_CAN;
	nCANBaudrate = g_stGITSetConfig.nDataRate;

	if( uiProtocolID == ISO15765_CARB_29BIT )
		ulDataIndex = 2/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;
	else
		ulDataIndex = 4/*CanID*/ + 1/*Data Length*/ + g_uiCanWriteMsgLength;
		
	pCanDataFields = pOutCanPacket->stExtendPacket.arrDataFields;
	usCanDLC = pOutCanPacket->stExtendPacket.ucDLC;
	memset(pCanDataFields, 0x00, SIZE_CAN_DATA_FIELD);
	
	if ( nCanFrameType == CAN_SINGLE_FRAME )
	{
	  	usCanDLC = pWriteMsg->pData[4];
		memcpy(pCanDataFields, &pWriteMsg->pData[ulDataIndex], pWriteMsg->pData[4]);
		
		if ( pWriteMsg->RxStatus != 1 )
		{
			g_ulPGN = ((pWriteMsg->pData[6] * 0x10000) + (pWriteMsg->pData[5] * 0x100));
			
			if( uiProtocolID == J1939_4PGN )
			{
				g_ulPGN = ((pWriteMsg->pData[7] * 0x1000000) + (pWriteMsg->pData[6] * 0x10000) + (pWriteMsg->pData[5] * 0x100) + (pWriteMsg->pData[8]));
			}
			if( pWriteMsg->pData[2] == 0xFD )
			{
				g_ulPGN = 0x00F00400;
			}
			
			if( uiProtocolID == J1939 )
			{
				if( g_stGITSetConfig.nEtc3 == 0x04 )
				{
					if( (pWriteMsg->pData[0] == 0x0C) && (pWriteMsg->pData[1] == 0xFF) && (pWriteMsg->pData[2] == 0xC6) && (pWriteMsg->pData[3] == 0x00) )
					{
						g_ulPGN = 0x00FFC500;
					}
					else if( (pWriteMsg->pData[0] == 0x18) && (pWriteMsg->pData[1] == 0xFF) && (pWriteMsg->pData[2] == 0x0E) && (pWriteMsg->pData[3] == 0x63) )
					{
						g_ulPGN = 0x00FF0D00;
					}
					else
					{
						if( (pWriteMsg->pData[5] == 0xC9) && 
							((pWriteMsg->pData[6] == 0x05) || 
							 (pWriteMsg->pData[6] == 0x06) || 
							 (pWriteMsg->pData[6] == 0x07) || 
							 (pWriteMsg->pData[6] == 0x08)) )
						{
							g_ulPGN = 0x18FF1002;
							
							nStartMask = 0x18ff1002;
							nEndMask = 0x18ff1002;
							
							CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
						}
						
						if( ((pWriteMsg->pData[7] == 0x01) && (pWriteMsg->pData[8] == 0x00) && (pWriteMsg->pData[9] == 0x00)) || 
								((pWriteMsg->pData[7] == 0x04) && (pWriteMsg->pData[8] == 0xF4) && (pWriteMsg->pData[9] == 0x01)) )
						{
							g_ulPGN = 0x1CFF10FE;
							
							nStartMask = 0x1CFF10FE;
							nEndMask = 0x1CFF10FE;
							
							CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
						}
					}
					clearRXCanMessage();
				}
				if( g_ulPGN == 0x00C10000)	g_ulPGN = 0x00C1F900;
			}
			
			if( uiProtocolID == J1939_4PGN )
			{
				g_ucCanEx_Func = 6;
				clearRXCanMessage();

				// 1sec rx timer start
				intTxdRxdCount=1000;
				
			}
			//DTC
			else if( (pWriteMsg->pData[5] == 0xCB) && (pWriteMsg->pData[6] == 0xFE) )
			{
				g_ucCanEx_Func = 7;
				nStartMask = 0x18E8C800;
				nEndMask = 0x1CFFFF00;
				CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
				clearRXCanMessage();
			}
			//FUEL TABLE
			else if( (pWriteMsg->pData[5] == 0xB4) && (pWriteMsg->pData[6] == 0xFF) )
			{
				g_ucCanEx_Func = 9;
				nStartMask = 0x1CE8F900;
				nEndMask = 0x1CEFFF00;
				CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
				clearRXCanMessage();
			}
			//EURO-6 FREEZE or EURO-6 F/G, H/L ENG IUPR Monitoring
			else if( ((pWriteMsg->pData[5] == 0xB7) && (pWriteMsg->pData[6] == 0xFD)) ||
					 ((pWriteMsg->pData[5] == 0x00) && (pWriteMsg->pData[6] == 0xC2)) )
			{
				g_ucCanEx_Func = 9;
				nStartMask = 0x18E8F900;
				nEndMask = 0x18EFF900;
				CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
				clearRXCanMessage();
			}
			//ECU INFO
			else if( (pWriteMsg->pData[5] == 0xDA) && (pWriteMsg->pData[6] == 0xFE) )
			{
				g_ucCanEx_Func = 10;
				nStartMask = 0x1CE8F900;
				nEndMask = 0x1CEFFF00;
				CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
				clearRXCanMessage();
			}
			//DTC  ACTIVE
			else if( (pWriteMsg->pData[5] == 0xCA) && (pWriteMsg->pData[6] == 0xFE) )
			{
				g_ucCanEx_Func = 1;
				intDlccomCount = 3000;
			}
			//ERASE
			else if( ((pWriteMsg->pData[5] == 0xCC) || (pWriteMsg->pData[5] == 0xD3)) && (pWriteMsg->pData[6] == 0xFE) )
			{
				g_ucCanEx_Func = 2;
			}
			//ACT 1 or ACT 2
			else if( (pWriteMsg->pData[1] == 0xFF) && ((pWriteMsg->pData[2] == 0xAA) || (pWriteMsg->pData[2] == 0xAB)) )
			{
				g_ucCanEx_Func = 4;
			}
			//CURR
			else
			{
				g_ucCanEx_Func = 3;
			}
			g_uiCanReadMsgLength = 0;
			GLogI("g_ucCanEx_Func : %d\r\n", g_ucCanEx_Func);
		}
	}
	else if ( nCanFrameType == CAN_FIRST_FRAME )
	{
		unsigned short usSendLength;
		
		if( uiProtocolID == ISO15765_NEW || 
		   	uiProtocolID == ISO15765_CAN_HWSET_DB_NEW ||
#ifdef HOTA
            uiProtocolID == ISO14229_ES95486_02_HOTA||
#endif
              
#ifdef CANFD_PROTOCOL
            uiProtocolID == ISO14229_ES95486_02_131_CANFD ||
#endif
		   	uiProtocolID == ISO14229_ES95486_02_10A ||
		   	uiProtocolID == ISO14230_ES95486_DOIP_121 ||
			uiProtocolID == ISO14229_ES95486_02_100_NEW)
		{
		  	g_uiCanWriteMsgLength++;		// 해당 프로토콜은 datasize가 2byte이다. 그래서 하나더 증가 시켜준다 150914 lwh
			usSendLength =  ((pWriteMsg->pData[2]&0x0F)<<8) + pWriteMsg->pData[3];
			usCanDLC = CAN_FRAME_DATA_SIZE;
			pCanDataFields[0] = CAN_FIRST_FRAME | ((usSendLength>>8) &0x0F);
			pCanDataFields[1] = (unsigned char)usSendLength;
			memcpy(pCanDataFields+2, pWriteMsg->pData+ulDataIndex+1, usCanDLC-2);
		}
		else if(uiProtocolID == ISO15765_29BIT)//2018.08.06 LJH 솔라티 통신 오류 수정
		{
		 	usSendLength = pWriteMsg->pData[4];
			usCanDLC = CAN_FRAME_DATA_SIZE;
			pCanDataFields[0] = CAN_FIRST_FRAME | ((usSendLength>>8) &0x0F);
			pCanDataFields[1] = (unsigned char)usSendLength;
			memcpy(pCanDataFields+2, pWriteMsg->pData+ulDataIndex, usCanDLC-2);
		}
		else if( uiProtocolID == ISO14229_ES95486_02_10F || uiProtocolID == ISO14230_ES95486_DOIP_12F )
		{
			// LJS 250105: 0x10F uses 1-byte Length, different from 0x100
			// 0x101: 2-byte Length
			// 0x10F: 1-byte Length
			// remove: g_uiCanWriteMsgLength++, pWriteMsg->pData+ulDataIndex + 1 -> pWriteMsg->pData+ulDataIndex
			if ( bIsStandardCan == eCAN_CLASSIC_STANDARD)
			{
				usSendLength = pWriteMsg->pData[2];		// 11bit: 1-byte Length
			}
			else
			{
				usSendLength = pWriteMsg->pData[4];		// 29bit: 1-byte Length
			}
			usCanDLC = CAN_FRAME_DATA_SIZE;
			pCanDataFields[0] = CAN_FIRST_FRAME | ((usSendLength>>8) &0x0F);
			pCanDataFields[1] = (unsigned char)usSendLength;
			memcpy(pCanDataFields+2, pWriteMsg->pData+ulDataIndex, usCanDLC-2);
		}
		else
		{
			usSendLength = pWriteMsg->pData[2];
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
			
		g_uiCanTxConsFrameNo++;
		g_uiCanTxSequenceNo++;
		pCanDataFields[0] = CAN_CONSECUTICE_FRAME + (g_uiCanTxSequenceNo % 0x10);
		memcpy(pCanDataFields+1, pWriteMsg->pData+ulDataIndex, usCanDLC-1);
	}

	if ( bIsStandardCan == eCAN_CLASSIC_STANDARD )	pOutCanPacket->stNormalPacket.ucDLC = usCanDLC;
	else											pOutCanPacket->stExtendPacket.ucDLC = usCanDLC;

#if defined(DEBUG_CAN_LOG)
	GLogI("[%s]ulDataIndex %d, g_uiCanWriteMsgLength %d, ucDLC %d\r\n", 
					__FUNCTION__, ulDataIndex, g_uiCanWriteMsgLength, usCanDLC);
#endif
}

void CAN_TxAckMessage(stCanPacket stOutCanPacket, U8 bIsStandardCan)
{
#if defined(DEBUG_CAN_LOG)
	GLogN("\r\n[%s]\r\n", __FUNCTION__);
#endif

#ifdef NEW_UDS_FILLER
    unsigned long uiProtocolID = 0;
    uiProtocolID = VCI_GetPassThruProtocolID();
#endif

    if ( bIsStandardCan == eCAN_CLASSIC_STANDARD) 
	{
		if ( CheckNewUDS(uiProtocolID) )
			memset(stOutCanPacket.stNormalPacket.arrDataFields, 0x55, SIZE_CAN_DATA_FIELD);
		else
			memset(stOutCanPacket.stNormalPacket.arrDataFields, 0x00, SIZE_CAN_DATA_FIELD);
	}
#ifdef CANFD_qhyek //Q_hyek CANFD
	else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD) 
	{
		if ( CheckNewUDS(uiProtocolID) )
			memset(stOutCanPacket.stFDStdPacket.arrDataFields, 0x55, SIZE_FDCAN_DATA_FIELD);
		else
			memset(stOutCanPacket.stFDStdPacket.arrDataFields, 0x00, SIZE_FDCAN_DATA_FIELD);
	}
#endif
	else
	{
        if ( CheckNewUDS(uiProtocolID) )
			memset(stOutCanPacket.stExtendPacket.arrDataFields, 0x55, SIZE_CAN_DATA_FIELD);
		else
			memset(stOutCanPacket.stExtendPacket.arrDataFields, 0x00, SIZE_CAN_DATA_FIELD);
	}

	if ( bIsStandardCan == eCAN_CLASSIC_STANDARD )
	{
		stOutCanPacket.stNormalPacket.ucSOF 			= CAN_FRAME_SOF;
		stOutCanPacket.stNormalPacket.ucIDE 			= CAN_FRAME_STANDARD_IDE;
		stOutCanPacket.stNormalPacket.ucRTR 			= 0;
		stOutCanPacket.stNormalPacket.ucReserved 		= 0;
		stOutCanPacket.stNormalPacket.usCRC 			= 0;
		stOutCanPacket.stNormalPacket.ucCRCDelimiter 	= 1;
		stOutCanPacket.stNormalPacket.ucACK 			= 1;
		stOutCanPacket.stNormalPacket.ucACKDelimiter 	= 1;
		stOutCanPacket.stNormalPacket.ucEOF 			= CAN_FRAME_EOF;
		stOutCanPacket.stNormalPacket.us11BitID 		= g_uiCANTxID;
		stOutCanPacket.stNormalPacket.ucDLC				= 0x08;

		stOutCanPacket.stNormalPacket.arrDataFields[0]  = 0x02;
		stOutCanPacket.stNormalPacket.arrDataFields[1]  = 0x3E;
		stOutCanPacket.stNormalPacket.arrDataFields[2]  = 0x80;
	}
	else
	{
		stOutCanPacket.stExtendPacket.ucSOF 			= CAN_FRAME_SOF;
		stOutCanPacket.stExtendPacket.ucSRR 			= 1;
		stOutCanPacket.stExtendPacket.ucIDE 			= CAN_FRAME_EXTEND_IDE;
		stOutCanPacket.stExtendPacket.ucRTR 			= 0;
		stOutCanPacket.stExtendPacket.ucReserved 		= 0;
		stOutCanPacket.stExtendPacket.usCRC 			= 0;
		stOutCanPacket.stExtendPacket.ucCRCDelimiter 	= 1;
		stOutCanPacket.stExtendPacket.ucACK 			= 1;
		stOutCanPacket.stExtendPacket.ucACKDelimiter 	= 1;
		stOutCanPacket.stExtendPacket.ucEOF 			= CAN_FRAME_EOF;
        stOutCanPacket.stExtendPacket.us11BitID 		= (g_uiCANTxID&0xfffc0000)>>18;
		stOutCanPacket.stExtendPacket.us18BitID 		= g_uiCANTxID&0x0003ffff;
		stOutCanPacket.stExtendPacket.ucDLC 			= 0x08;

		stOutCanPacket.stExtendPacket.arrDataFields[0]  = 0x02;
		stOutCanPacket.stExtendPacket.arrDataFields[1]  = 0x3E;
		stOutCanPacket.stExtendPacket.arrDataFields[2]  = 0x80;
	}
	
	CAN_WriteBuff((unsigned char*)&stOutCanPacket, 0, g_ucCAN_CH, CAN_SINGLE_FRAME);
}
void CAN_uDelay(unsigned int uiDelay)
{
	if(uiDelay != 0) 
		DWT_Delay_us(uiDelay);
}

void CAN_mDelay(unsigned int uiDelay)
{
	if(uiDelay != 0) 
		osDelay(uiDelay);
}

BOOL CAN_FindCanPacket(stCanPacket *pInCanPacket, eCanType *pbStandardCan)
{
	if( OemReadCanBuff((U8*)pInCanPacket, CAN_GetRxP3MinTimeOutValue()) )
	{
#ifdef CANFD_qhyek //Q_hyek CANFD
		if ( (pInCanPacket->stNormalPacket.ucSOF == CAN_FRAME_SOF ) &&
			 (pInCanPacket->stNormalPacket.ucIDE == CAN_FRAME_STANDARD_IDE ) &&
			 (pInCanPacket->stNormalPacket.ucReserved == 1 ) /*&&           //Q_hyek CANFD
			 (pInCanPacket->stNormalPacket.ucEOF == CAN_FRAME_EOF)*/ )
		{
			*pbStandardCan = eCAN_FDFORMAT_STANDARD;
		}
#else
        if ( (pInCanPacket->stNormalPacket.ucSOF == CAN_FRAME_SOF ) &&
			 (pInCanPacket->stNormalPacket.ucIDE == CAN_FRAME_STANDARD_IDE ) &&
			 (pInCanPacket->stNormalPacket.ucReserved == 1 ) &&
			 (pInCanPacket->stNormalPacket.ucEOF == CAN_FRAME_EOF) )
		{
			*pbStandardCan = eCAN_FDFORMAT_STANDARD;
		}
#endif
		else if ( (pInCanPacket->stNormalPacket.ucSOF == CAN_FRAME_SOF ) &&
			 (pInCanPacket->stNormalPacket.ucIDE == CAN_FRAME_STANDARD_IDE ) &&
			 (pInCanPacket->stNormalPacket.ucReserved == 0 ) &&
			 (pInCanPacket->stNormalPacket.ucEOF == CAN_FRAME_EOF) )
		{
			*pbStandardCan = eCAN_CLASSIC_STANDARD;
		}
		else if ( (pInCanPacket->stExtendPacket.ucSOF == CAN_FRAME_SOF ) &&
			 (pInCanPacket->stExtendPacket.ucIDE == CAN_FRAME_EXTEND_IDE ) &&
			 (pInCanPacket->stExtendPacket.ucReserved == 1 ) &&
			 (pInCanPacket->stExtendPacket.ucEOF == CAN_FRAME_EOF) )
		{
			*pbStandardCan = eCAN_FDFORMAT_EXTENDED;
		}
		else if ( (pInCanPacket->stExtendPacket.ucSOF == CAN_FRAME_SOF ) &&
			 (pInCanPacket->stExtendPacket.ucIDE == CAN_FRAME_EXTEND_IDE ) &&
			 (pInCanPacket->stExtendPacket.ucReserved == 0 ) &&
			 (pInCanPacket->stExtendPacket.ucEOF == CAN_FRAME_EOF) )
		{
			*pbStandardCan = eCAN_CLASSIC_EXTENDED;
		}
		
		//StopSWTimer(g_ulCanWrittenTick);
#if defined(DEBUG_CAN_LOG)
		GLogI("[%s] CAN version %d \r\n", __FUNCTION__, *pbStandardCan);

        GLogI("[%s] %02X ", __FUNCTION__, pInCanPacket->stNormalPacket.us11BitID);
        for ( int jj=0; jj<pInCanPacket->stNormalPacket.ucDLC; jj++ )
            GLogI("%02X ", pInCanPacket->stNormalPacket.arrDataFields[jj]);
        GLogI("\r\n");
#endif
		return 1;
	}
	else return 0;
}

BOOL CAN_FindCanPacket_EX( stCanPacket *pInCanPacket, eCanType *pbStandardCan )
{
	BOOL result = FALSE;
	
	if( OemReadCanBuff((U8*)pInCanPacket, CAN_GetRxP3MinTimeOutValue()) )
	{
		if ( (pInCanPacket->stExtendPacket.ucSOF == CAN_FRAME_SOF ) &&
			 (pInCanPacket->stExtendPacket.ucIDE == CAN_FRAME_EXTEND_IDE ) &&
			 (pInCanPacket->stExtendPacket.ucReserved == 1 ) &&
			 (pInCanPacket->stExtendPacket.ucEOF == CAN_FRAME_EOF) )
		{
			*pbStandardCan = eCAN_FDFORMAT_EXTENDED;
		}
		else if ( (pInCanPacket->stExtendPacket.ucSOF == CAN_FRAME_SOF ) &&
			 (pInCanPacket->stExtendPacket.ucIDE == CAN_FRAME_EXTEND_IDE ) &&
			 (pInCanPacket->stExtendPacket.ucReserved == 0 ) &&
			 (pInCanPacket->stExtendPacket.ucEOF == CAN_FRAME_EOF) )
		{
			*pbStandardCan = eCAN_CLASSIC_EXTENDED;
		}
		else
		{
			result = FALSE;
		}
		
		//StopSWTimer(g_ulCanWrittenTick);
#if defined(DEBUG_CAN_LOG)
			GLogI("[%s] CAN version %d \r\n", __FUNCTION__, *pbStandardCan);
#endif
			result = TRUE;
	}
	else	result = FALSE;
	
	return result;
}

unsigned long CAN_GetRxP3MinTimeOutValue(void)
{
	unsigned long ulP3MinTimeout;

	if( g_bCanRxPendingFrame )				    ulP3MinTimeout = g_stGITSetConfig.nP3Max+500;
	else if ( g_bCanRxCarbFrame )			    ulP3MinTimeout = g_stGITSetConfig.nP2Max;
	else if ( g_bRcvMultiFrame )				ulP3MinTimeout = 1000;
	else if ( g_bCanRxConcequtiveFrame )		ulP3MinTimeout = 300;
	else										ulP3MinTimeout = g_stGITSetConfig.nP3Min;

	if(g_ulProtocolID == ISO15765_ACU_SINGLE)
	{
		ulP3MinTimeout = g_stGITSetConfig.nP3Min;
		//GLogN("ulP3MinTimeout: %d\r\n",ulP3MinTimeout);
	}
	else if( g_ulProtocolID == ISO14230_ES95486_DOIP_12F && g_bCanRxPendingFrame != true)
	{
	  	ulP3MinTimeout = 2000;
	}
	else if( g_ulProtocolID == J1939 )
	{
	  	ulP3MinTimeout = g_ulTxdRxdCount;
	}
	else if( g_ulProtocolID == J1939_4PGN )
	{
	  	ulP3MinTimeout = 1000;//g_ulTxdRxdCount;
	}
    else if( (g_ulProtocolID == ISO15765_29BIT)
             ||(g_ulProtocolID == ISO15765_29BIT_EXCEPT)
             ||(g_ulProtocolID == ISO15765_CARB_29BIT)
             ||(g_ulProtocolID == ISO15765_CARB_29BIT_NEW)
			 ||(g_ulProtocolID == J1939_23_CARB_29BIT_NEW)
#ifdef NEW_29BIT_CAN
             ||(g_ulProtocolID == ISO15765_ES95486_29bit)
             ||(g_ulProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
             || (g_ulProtocolID == ISO15765_ES95486_135_29bit_CANFD)
             || (g_ulProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
            )
    {
        if( g_bCF_TxComplete == 1 )
        {
            ulP3MinTimeout = 5000;
        }
    }
	return ulP3MinTimeout;
	
}

unsigned int CAN_WriteBuff(unsigned char* pBuff, unsigned int nCount, unsigned int uiCANChannel, int nCanFrameType)
{
	int uiWriteLen;
	unsigned long uiProtocolID;
	uiProtocolID = VCI_GetPassThruProtocolID();
	stCanPacket *pOutCanPacket = (stCanPacket*)pBuff;

	if ( nCanFrameType == CAN_SINGLE_FRAME )
	{
		if( (uiProtocolID == ISO15765_SINGLE_PODS)			||
			(uiProtocolID == ISO15765_EXCEPT)				||
			(uiProtocolID == ISO15765_29BIT_EXCEPT)			||
			(uiProtocolID == ISO14229_UDS)					||		// KYC 20110926 HG IPM 프로토콜 관련 추가
			(uiProtocolID == ISO15765_SINGLE_SMK)			||
			(uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE)	||		//111013 LWH
			(uiProtocolID == ISO15765_ACU_SINGLE)			||
			(uiProtocolID == J1939)							)      	// 20150810 seo 0x2B 추가
		{
			clearRXCanMessage();
		}
		if( (uiProtocolID == ISO15765_CUBIS)&&
			(pOutCanPacket->stNormalPacket.arrDataFields[0]==0x02)&&
			(pOutCanPacket->stNormalPacket.arrDataFields[1]==0x01)&&	
			(pOutCanPacket->stNormalPacket.arrDataFields[2]==0x00))		
		{
			clearRXCanMessage();
		}
	}

	if ( nCanFrameType == CAN_CONSECUTICE_FRAME )
	{
#ifdef HOTA
        if( uiProtocolID == ISO14229_ES95486_02_HOTA )
        {
            CAN_uDelay(g_stECUSetConfig.nSTMinTx);
        }
        else
#endif
        {
            if ( (g_stECUSetConfig.nSTMinTx >= 0xF1) && (g_stECUSetConfig.nSTMinTx <= 0xF9) )
                CAN_uDelay((g_stECUSetConfig.nSTMinTx & 0x0F)*100);	// uSec Delay : ISO15765-2
            else
                CAN_mDelay(g_stECUSetConfig.nSTMinTx);
            
            if(g_stGITSetConfig.nEtc1 != 0)
            {
                CAN_mDelay(g_stGITSetConfig.nEtc1);
            }
        }
	}
	else
	{		
		if(g_stGITSetConfig.nEtc1 >=50)								        CAN_mDelay(50); 
		else if(g_stGITSetConfig.nEtc1 != 0)								CAN_mDelay(g_stGITSetConfig.nEtc1);
	}
	
	uiWriteLen = OemWriteCanBuff((unsigned char*)pBuff, 0, NULL, uiCANChannel);
	
	return uiWriteLen;
}

eDiagCanState CAN_TxParsing(stCanPacket *pOutCanPacket, stPASSTHRU_MSG *pWriteMsg, U8 *pbStandardCAN)
{
	unsigned long uiProtocolID;
	eDiagCanState  eCanTxState = eCAN_NONE_STATE;
	unsigned short usDLCLength;
	
	uiProtocolID = VCI_GetPassThruProtocolID();

	if (uiProtocolID == ISO14229_ES95486_02_10F || uiProtocolID == ISO14230_ES95486_DOIP_12F)
	{
		if ((pWriteMsg->pData[0] == 0x17 && pWriteMsg->pData[1] == 0xFF) ||
			(pWriteMsg->pData[0] == 0x18 && pWriteMsg->pData[1] == 0xDA))
		{
			*pbStandardCAN = FALSE;

			pOutCanPacket->stExtendPacket.ucSOF 			= CAN_FRAME_SOF;
			pOutCanPacket->stExtendPacket.ucSRR 			= 1;
			pOutCanPacket->stExtendPacket.ucIDE 			= CAN_FRAME_EXTEND_IDE;
			pOutCanPacket->stExtendPacket.ucRTR 			= 0;
			pOutCanPacket->stExtendPacket.ucReserved 		= 0;
			pOutCanPacket->stExtendPacket.usCRC 			= 0;
			pOutCanPacket->stExtendPacket.ucCRCDelimiter 	= 1;
			pOutCanPacket->stExtendPacket.ucACK 			= 1;
			pOutCanPacket->stExtendPacket.ucACKDelimiter	= 1;
			pOutCanPacket->stExtendPacket.ucEOF 			= CAN_FRAME_EOF;
			
			pOutCanPacket->stExtendPacket.us11BitID 		= (((pWriteMsg->pData[0]&0x1F)<<8)  + pWriteMsg->pData[1])>>2;
			pOutCanPacket->stExtendPacket.us18BitID 		= ((pWriteMsg->pData[1]&0x03)<<16) + (pWriteMsg->pData[2]<<8) + pWriteMsg->pData[3];
			g_uiCANTxID = ((U32)pWriteMsg->pData[0]<<24) + ((U32)pWriteMsg->pData[1]<<16) + ((U32)pWriteMsg->pData[2]<< 8) + ((U32)pWriteMsg->pData[3]);
			
			pOutCanPacket->stExtendPacket.ucDLC = CAN_FRAME_DATA_SIZE;
			
			if (pWriteMsg->pData[4] > 7)
			{
				eCanTxState = eCAN_TX_FIRST_FRAME_29BIT;
			}
			else
			{
				eCanTxState = eCAN_TX_SINGLE_FRAME_29BIT;
			}
		}
		else
		{
			*pbStandardCAN = TRUE;

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
			g_uiCANTxID = pOutCanPacket->stNormalPacket.us11BitID;

			if (CAN_CheckStandardTxFrameType(&usDLCLength, pWriteMsg))
			{
				if(pWriteMsg->DataSize < usDLCLength)
				{
					pWriteMsg->DataSize = usDLCLength + 2;// CAN ID Length
				}
				eCanTxState = eCAN_TX_FIRST_FRAME;
			}
			else
			{
				eCanTxState = eCAN_TX_SINGLE_FRAME;
			}
		}
	}
	else if ((uiProtocolID == J1939 				)	||
			 (uiProtocolID == J1939_4PGN 			)	||
			 (uiProtocolID == ISO15765_29BIT 		)	||
			 (uiProtocolID == ISO15765_29BIT_EXCEPT	)	|| 
			 (uiProtocolID == ISO15765_CARB_29BIT_NEW)	|| 
			 (uiProtocolID == J1939_23_CARB_29BIT_NEW)  ||
			 (uiProtocolID == ISO15765_CARB_29BIT 	)
#ifdef NEW_29BIT_CAN
        ||(uiProtocolID == ISO15765_ES95486_29bit)
        ||(uiProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
         || (uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
         || (uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
          )
	{
		/************************************************************************************/
		/* Standard Can : pWriteMsg->pData 포맷												*/
		/*		 0	|  1   |  2   |  3	 |	4	|  5  |   6	  |  n-1  | n					*/
		/*		ID1 |  ID2 |  ID3 | ID4  |Data1 |Data2| Data2 |Datan-1|Datan				*/
		/************************************************************************************/
		pOutCanPacket->stExtendPacket.ucSOF 			= CAN_FRAME_SOF;
		pOutCanPacket->stExtendPacket.ucSRR 			= 1;
		pOutCanPacket->stExtendPacket.ucIDE 			= CAN_FRAME_EXTEND_IDE;
		pOutCanPacket->stExtendPacket.ucRTR 			= 0;
		pOutCanPacket->stExtendPacket.ucReserved 		= 0;
		pOutCanPacket->stExtendPacket.usCRC 			= 0;
		pOutCanPacket->stExtendPacket.ucCRCDelimiter 	= 1;
		pOutCanPacket->stExtendPacket.ucACK 			= 1;
		pOutCanPacket->stExtendPacket.ucACKDelimiter 	= 1;
		pOutCanPacket->stExtendPacket.ucEOF 			= CAN_FRAME_EOF;
        pOutCanPacket->stExtendPacket.us11BitID 		= (((pWriteMsg->pData[0]&0x1F)<<8)  + pWriteMsg->pData[1])>>2;//2018.08.06 LJH 29bit CAN ID 오류 수정
		pOutCanPacket->stExtendPacket.us18BitID 		= ((pWriteMsg->pData[1]&0x03)<<16) + (pWriteMsg->pData[2]<<8) + pWriteMsg->pData[3];

		g_uiCANTxID = ((U32)pWriteMsg->pData[0]<<24) + ((U32)pWriteMsg->pData[1]<<16) + ((U32)pWriteMsg->pData[2]<< 8) + ((U32)pWriteMsg->pData[3]);

		if( ( uiProtocolID == ISO15765_29BIT )||
				 ( uiProtocolID == ISO15765_29BIT_EXCEPT)|| 
				 ( uiProtocolID == ISO15765_29BIT_REPRO_PENNIMG_TIME)||
				 ( uiProtocolID == ISO15765_CARB_29BIT ) ||
                 ( uiProtocolID == ISO15765_CARB_29BIT_NEW )||
                 ( uiProtocolID == J1939_23_CARB_29BIT_NEW )
#ifdef NEW_29BIT_CAN
                  ||(uiProtocolID == ISO15765_ES95486_29bit)
                  ||(uiProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
                || (uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
                || (uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
                 )
        {
			pOutCanPacket->stExtendPacket.ucDLC = CAN_FRAME_DATA_SIZE;
        	if ( (pWriteMsg->pData[4] > 7) )
			{
				eCanTxState = eCAN_TX_FIRST_FRAME_29BIT;
				//memcpy(pOutCanPacket->stExtendPacket.arrDataFields, pWriteMsg->pData+5, pOutCanPacket->stExtendPacket.ucDLC);	// when i go to first frame, not use this arrDataFields, they use pData direct
			}
			else
			{
				eCanTxState = eCAN_TX_SINGLE_FRAME_29BIT;
				//memcpy(pOutCanPacket->stExtendPacket.arrDataFields, pWriteMsg->pData+4, pOutCanPacket->stExtendPacket.ucDLC);
			}
		}
		else
		{
            pOutCanPacket->stExtendPacket.ucDLC = pWriteMsg->pData[4];
            eCanTxState = eCAN_TX_SINGLE_FRAME_29BIT;
			memcpy(pOutCanPacket->stExtendPacket.arrDataFields, pWriteMsg->pData+4, pOutCanPacket->stExtendPacket.ucDLC);
		}

		// except 처리
		if( uiProtocolID == ISO15765_CARB_29BIT )
		{
			if( pWriteMsg->pData[0] == 0x07 ) // CARB 29bit에 대해서는 11bit처럼 내려옴으로 인해서 exception처리 함. 
			{
				pOutCanPacket->stExtendPacket.ucDLC	 = pWriteMsg->pData[2];

				// 29bit CARB에 대해서는 CanID를 0x18DB33F1으로 변환해야 한다.
				pOutCanPacket->stExtendPacket.us11BitID = 0x0636;
				pOutCanPacket->stExtendPacket.us18BitID = 0x333F1;
				g_uiCANTxID = 0x18DB33F1;
			}															
		}
		else if((uiProtocolID == J1939)||(uiProtocolID == J1939_4PGN))
		{
		  	eCanTxState = eCAN_TX_SINGLE_FRAME_EX;
		}

		*pbStandardCAN = FALSE;
	}
	else
	{
#ifdef CANFD_qhyek //Q_hyek CANFD
		if(g_ucCanformat == CAN_FRAMEFORMAT_FDCAN)
		{
			/************************************************************************************/
			/* Standard Can : pWriteMsg->pData 포맷												*/
			/*		 0	|  1   |  2   |  3	 |	4	|  5  |  6	|  n-1  | n						*/
			/*		DLC |  ID1 |  ID2 | Data0| Data1|Data2|Data3|Datan-1|Datan					*/
			/************************************************************************************/
			pOutCanPacket->stFDStdPacket.ucSOF 			= CAN_FRAME_SOF;
			pOutCanPacket->stFDStdPacket.ucIDE 			= CAN_FRAME_STANDARD_IDE;
			//pOutCanPacket->stFDStdPacket.ucRTR 			= 0;
			//pOutCanPacket->stFDStdPacket.ucReserved 		= 0;
			pOutCanPacket->stFDStdPacket.usCRC 			= 0;
			pOutCanPacket->stFDStdPacket.ucCRCDelimiter 	= 1;
			pOutCanPacket->stFDStdPacket.ucACK 			= 1;
			pOutCanPacket->stFDStdPacket.ucACKDelimiter 	= 1;
			pOutCanPacket->stFDStdPacket.ucEOF 			= CAN_FRAME_EOF;
			pOutCanPacket->stFDStdPacket.us11BitID 		= ((pWriteMsg->pData[0]&0x07)<<8)  + pWriteMsg->pData[1];

			g_uiCANTxID = ((pWriteMsg->pData[0]&0x07)<<8) + pWriteMsg->pData[1];

			if ( CAN_CheckStandardTxFrameType(&usDLCLength, pWriteMsg) )
				eCanTxState = eCAN_TX_FIRST_FRAME;
			else
				eCanTxState = eCAN_TX_SINGLE_FRAME;

			*pbStandardCAN = eCAN_FDFORMAT_STANDARD;
		}
		else
#endif
		{
		
		/************************************************************************************/
		/* Standard Can : pWriteMsg->pData 포맷												*/
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

		g_uiCANTxID = ((pWriteMsg->pData[0]&0x07)<<8) + pWriteMsg->pData[1];

		if ( CAN_CheckStandardTxFrameType(&usDLCLength, pWriteMsg) )
		{
			if(pWriteMsg->DataSize < usDLCLength)
			{
			  	pWriteMsg->DataSize = usDLCLength + 2;// CAN ID Length
			}
		  	eCanTxState = eCAN_TX_FIRST_FRAME;
		}
		else
			eCanTxState = eCAN_TX_SINGLE_FRAME;

		*pbStandardCAN = TRUE;
		}
	}
	return eCanTxState;
}

eDiagCanState CAN_MakeTxSingleFrame(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg)
{
	eDiagCanState eCanTxState = eCAN_NONE_STATE;

	// 만들고,
	CAN_MakeSendFrame(pOutCanPacket, bIsStandardCan, CAN_SINGLE_FRAME, pWriteMsg);

	// send 하고 
	CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, g_ucCAN_CH, CAN_SINGLE_FRAME);
	eCanTxState = eCAN_RX_BLOCK;
	
	g_uiCanWriteMsgLength = pWriteMsg->DataSize;
#if defined(DEBUG_CAN_LOG)
	GLogI("\r\n[%s] pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d \r\n", __FUNCTION__, pWriteMsg->DataSize, g_uiCanWriteMsgLength);
#endif
	if(GetCurFwServiceMode()!=eApp_Inside)	g_bAckflag = 1;
	intDlccomCount = g_uiAckTiming; // Ack Timming

	return eCanTxState;
}

eDiagCanState CAN_MakeTxFirstFrame(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg)
{
	eDiagCanState eCanTxState = eCAN_NONE_STATE;

	g_uiCanTxConsFrameNo = 0;
	g_uiCanTxSequenceNo = 0;
	// 만들고,
	CAN_MakeSendFrame(pOutCanPacket, bIsStandardCan, CAN_FIRST_FRAME, pWriteMsg);
	// send 하고 
	CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, g_ucCAN_CH, CAN_FIRST_FRAME);
	eCanTxState = eCAN_RX_BLOCK;

	if(g_ulProtocolID==ISO15765_SMK)
	{
		//g_uiCanWriteMsgLength += CAN_FRAME_DATA_SIZE	/*FitstFrame*/;
	}
	else 
	{
		g_uiCanWriteMsgLength += CAN_FRAME_DATA_SIZE - 2/*FitstFrame*/;
	}
	
	if(GetCurFwServiceMode()!=eApp_Inside)	g_bAckflag = 1;
	intDlccomCount = g_uiAckTiming; // Ack Timming

#if defined(DEBUG_CAN_LOG)
	GLogI("\r\n[%s] Can TX : pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d \r\n", __FUNCTION__, pWriteMsg->DataSize, g_uiCanWriteMsgLength);
#endif
	
	return eCanTxState;
}

eDiagCanState CAN_MakeTxFlowControlFrame(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg)
{
	unsigned int uiProtocolID, nCanidOffset;
	eDiagCanState eCanTxState = eCAN_NONE_STATE;
		
#if defined(DEBUG_CAN_LOG)
	GLogI("\r\n[%s] pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d \r\n", __FUNCTION__, pWriteMsg->DataSize, g_uiCanWriteMsgLength);
#endif

	uiProtocolID = VCI_GetPassThruProtocolID();

	nCanidOffset 	 = g_stGITSetConfig.nEtc2;

	if( uiProtocolID == ISO15765_CUBIS)
	{
		// cubis 관련 플로우 컨트롤 하지 않음.
	}
	else
	{
		if ( bIsStandardCan == eCAN_CLASSIC_STANDARD )
		{
			if ( CheckNewUDS(uiProtocolID) )
				memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x55, CAN_FRAME_DATA_SIZE);
			else
				memset(pOutCanPacket->stNormalPacket.arrDataFields, 0x00, CAN_FRAME_DATA_SIZE);
            
            // For LX3 CCU2 241129
            if( CheckNewUDS(uiProtocolID) )
            {
                osDelay(2);
            }
			
			pOutCanPacket->stNormalPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
			pOutCanPacket->stNormalPacket.arrDataFields[1] = 0;// 2016/06/02 James Jean g_stGITSetConfig.nBSTx;
			pOutCanPacket->stNormalPacket.arrDataFields[2] = 0;// 2016/06/02 James Jean g_stGITSetConfig.nSTMinTx;		
			
			// Set BS, STmin
			if ( uiProtocolID == ISO15765_CARB ||
					  uiProtocolID == ISO15765_CARB_NEW ||
                      uiProtocolID == J1939_23_CARB_NEW_LENGTH	||
					  uiProtocolID == ISO15765_CARB_NEW_LENGTH )// 0X07DF & 0x18DB33F1 CARB통신인 경우
			{
				if(g_stGITSetConfig.nBSTx == 0xFFFF)		g_stGITSetConfig.nBSTx = 0x00;
				if(g_stGITSetConfig.nSTMinTx == 0xFFFF)		g_stGITSetConfig.nSTMinTx = 0x00;
			}
			else
			{
			  	if(g_stGITSetConfig.nBSTx == 0xFFFF)		g_stGITSetConfig.nBSTx = 0x08;
				if(g_stGITSetConfig.nSTMinTx == 0xFFFF)		g_stGITSetConfig.nSTMinTx = 0x02;
			}
			
			pOutCanPacket->stNormalPacket.arrDataFields[1] = g_stGITSetConfig.nBSTx;
			pOutCanPacket->stNormalPacket.arrDataFields[2] = g_stGITSetConfig.nSTMinTx;
			
			// Set CAN ID
		  	if((nCanidOffset != 0) ||
				( uiProtocolID == ISO15765_CAN_HWSET_DB )||
				( uiProtocolID == ISO15765_CAN_HWSET_DB_NEW )||
				( uiProtocolID == ISO14229_UDS_HWSET_DB ))
			{
			  	if(nCanidOffset != 0)
					pOutCanPacket->stNormalPacket.us11BitID = pOutCanPacket->stNormalPacket.us11BitID - nCanidOffset;
				else
				  	pOutCanPacket->stNormalPacket.us11BitID = g_uiCANTxID;

//				pOutCanPacket->stNormalPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
//				pOutCanPacket->stNormalPacket.arrDataFields[1] = g_stGITSetConfig.nBSTx;
//				pOutCanPacket->stNormalPacket.arrDataFields[2] = g_stGITSetConfig.nSTMinTx;
			}
			else if ( uiProtocolID == ISO15765_CARB ||
					  uiProtocolID == ISO15765_CARB_NEW ||
                      uiProtocolID == J1939_23_CARB_NEW_LENGTH	||
					  uiProtocolID == ISO15765_CARB_NEW_LENGTH )// 0X07DF & 0x18DB33F1 CARB통신인 경우
			{
				pOutCanPacket->stNormalPacket.us11BitID = pOutCanPacket->stNormalPacket.us11BitID - 8/*Carb통신인 경우에 Can ID의 -8을 해서 전달*/;
//				pOutCanPacket->stNormalPacket.arrDataFields[1] = g_stGITSetConfig.nBSTx;
//				pOutCanPacket->stNormalPacket.arrDataFields[2] = g_stGITSetConfig.nSTMinTx;
			}
			else
			{
//				pOutCanPacket->stNormalPacket.arrDataFields[1] = g_stGITSetConfig.nBSTx;
//				pOutCanPacket->stNormalPacket.arrDataFields[2] = g_stGITSetConfig.nSTMinTx;
				pOutCanPacket->stNormalPacket.us11BitID = g_uiCANTxID;
			}
		}
        
#ifdef CANFD_qhyek //Q_hyek CANFD
		else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD )
		{
			if ( CheckNewUDS(uiProtocolID) )
				memset(pOutCanPacket->stFDStdPacket.arrDataFields, 0x55, CAN_FRAME_DATA_SIZE);
			else
				memset(pOutCanPacket->stFDStdPacket.arrDataFields, 0x00, CAN_FRAME_DATA_SIZE);
			
			pOutCanPacket->stFDStdPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
			pOutCanPacket->stFDStdPacket.arrDataFields[1] = 0; // 2016/06/02 James Jean g_stGITSetConfig.nBSTx;
			pOutCanPacket->stFDStdPacket.arrDataFields[2] = 0; // 2016/06/02 James Jean g_stGITSetConfig.nSTMinTx;		
			
            // Set BS, STmin
			if ( uiProtocolID == ISO15765_CARB ||
					  uiProtocolID == ISO15765_CARB_NEW ||
                      uiProtocolID == J1939_23_CARB_NEW_LENGTH	||
					  uiProtocolID == ISO15765_CARB_NEW_LENGTH )// 0X07DF & 0x18DB33F1 CARB통신인 경우
			{
				if(g_stGITSetConfig.nBSTx == 0xFFFF)		g_stGITSetConfig.nBSTx = 0x00;
				if(g_stGITSetConfig.nSTMinTx == 0xFFFF)		g_stGITSetConfig.nSTMinTx = 0x00;
			}
			else
			{
			  	if(g_stGITSetConfig.nBSTx == 0xFFFF)		g_stGITSetConfig.nBSTx = 0x08;
				if(g_stGITSetConfig.nSTMinTx == 0xFFFF)		g_stGITSetConfig.nSTMinTx = 0x02;
			}
			
			pOutCanPacket->stFDStdPacket.arrDataFields[1] = g_stGITSetConfig.nBSTx;
			pOutCanPacket->stFDStdPacket.arrDataFields[2] = g_stGITSetConfig.nSTMinTx;
			
		  	if((nCanidOffset != 0)||
			  ( uiProtocolID == ISO15765_CAN_HWSET_DB )||
			  ( uiProtocolID == ISO15765_CAN_HWSET_DB_NEW )||
			  ( uiProtocolID == ISO14229_UDS_HWSET_DB ))
			{
			  	if(nCanidOffset != 0)
					pOutCanPacket->stFDStdPacket.us11BitID = pOutCanPacket->stFDStdPacket.us11BitID - nCanidOffset;

				pOutCanPacket->stFDStdPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
				pOutCanPacket->stFDStdPacket.arrDataFields[1] = g_stGITSetConfig.nBSTx;
				pOutCanPacket->stFDStdPacket.arrDataFields[2] = g_stGITSetConfig.nSTMinTx;
			}
			else if ( uiProtocolID == ISO15765_CARB )// 0X07DF & 0x18DB33F1 CARB통신인 경우
			{
				pOutCanPacket->stFDStdPacket.us11BitID = pOutCanPacket->stFDStdPacket.us11BitID - 8/*Carb통신인 경우에 Can ID의 -8을 해서 전달*/;
			}
			else
			{
				pOutCanPacket->stFDStdPacket.us11BitID = g_uiCANTxID;
			}
		}	
#endif
        
		else
		{
			if ( uiProtocolID == ISO15765_CARB_29BIT ||
				 uiProtocolID == ISO15765_CARB_29BIT_NEW ||
                 uiProtocolID == J1939_23_CARB_29BIT_NEW )
			{
				// ASSAGAORY : FlowControl을 전달할 때 0x18DA00F1으로 전달해야한다.
				// 따라서 11bit에는 0x636, 18bit에는 0x200F1가 입력되어야 한다.
				pOutCanPacket->stExtendPacket.us11BitID = 0x636;
                if( uiProtocolID == J1939_23_CARB_29BIT_NEW )
                {
                    pOutCanPacket->stExtendPacket.us18BitID = 0x20000;
                    
                    int iIndex=0;
                    for(iIndex=0;iIndex<MAX_CARB_RECV_ARRAY_CNT;iIndex++)
                    {
                        if(g_CarbRecvInfo[iIndex].uiOrder == 0 ) break;
                    }
                    char cTempID[4]={0,};
                    if( iIndex > 0) memcpy(cTempID,&g_CarbRecvInfo[iIndex-1].ulCANID,sizeof(cTempID));
                    pOutCanPacket->stExtendPacket.us18BitID = pOutCanPacket->stExtendPacket.us18BitID | (cTempID[0]<<8) | (cTempID[1]);
                    
                    memset(pOutCanPacket->stExtendPacket.arrDataFields, 0x00, CAN_FRAME_DATA_SIZE);
                }
				else
                {
                    pOutCanPacket->stExtendPacket.us18BitID = 0x200F1;
                    memset(pOutCanPacket->stExtendPacket.arrDataFields, 0xFF, CAN_FRAME_DATA_SIZE);
                }
				
				pOutCanPacket->stExtendPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
				pOutCanPacket->stExtendPacket.arrDataFields[1] = 8; // 2016/06/02 James Jean g_stGITSetConfig.nBSTx;
				pOutCanPacket->stExtendPacket.arrDataFields[2] = 0x05; 	
				pOutCanPacket->stExtendPacket.arrDataFields[3] = 0x00; 	
			}
			else
			{
				if ( CheckNewUDS(uiProtocolID) )
					memset(pOutCanPacket->stExtendPacket.arrDataFields, 0x55, CAN_FRAME_DATA_SIZE);
				else
					memset(pOutCanPacket->stExtendPacket.arrDataFields, 0x00, CAN_FRAME_DATA_SIZE);
				if( g_stGITSetConfig.nBSTx == 0xFFFF && g_stGITSetConfig.nSTMinTx == 0xFFFF )
				{
					pOutCanPacket->stExtendPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
					pOutCanPacket->stExtendPacket.arrDataFields[1] = 0x08;
					pOutCanPacket->stExtendPacket.arrDataFields[2] = 0x05;
					pOutCanPacket->stExtendPacket.arrDataFields[3] = 0x00;
					pOutCanPacket->stExtendPacket.arrDataFields[4] = 0xFF;
					pOutCanPacket->stExtendPacket.arrDataFields[5] = 0xFF;
					pOutCanPacket->stExtendPacket.arrDataFields[6] = 0xFF;
					pOutCanPacket->stExtendPacket.arrDataFields[7] = 0xFF;
				}
				else
				{
					pOutCanPacket->stExtendPacket.arrDataFields[0] = CAN_FLOWCTRL_FRAME | CAN_FS_CTS;
					pOutCanPacket->stExtendPacket.arrDataFields[1] = g_stGITSetConfig.nBSTx; // 2016/06/02 James Jean g_stGITSetConfig.nBSTx;
					pOutCanPacket->stExtendPacket.arrDataFields[2] = g_stGITSetConfig.nSTMinTx; // 2016/06/02 James Jean g_stGITSetConfig.nSTMinTx;	
					pOutCanPacket->stExtendPacket.arrDataFields[3] = 0x00;
					pOutCanPacket->stExtendPacket.arrDataFields[4] = 0xFF;
					pOutCanPacket->stExtendPacket.arrDataFields[5] = 0xFF;
					pOutCanPacket->stExtendPacket.arrDataFields[6] = 0xFF;
					pOutCanPacket->stExtendPacket.arrDataFields[7] = 0xFF;
				}

				pOutCanPacket->stExtendPacket.us11BitID 	= (g_uiCANTxID&0xfffc0000)>>18;
				pOutCanPacket->stExtendPacket.us18BitID 	= g_uiCANTxID&0x0003ffff;
			}
		}

		CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, g_ucCAN_CH, CAN_FLOWCTRL_FRAME);
	}
	
	eCanTxState = eCAN_RX_BLOCK;
	return eCanTxState;
}

eDiagCanState CAN_MakeTxConsecutiveFrame(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg)
{
	eDiagCanState eCanTxState = eCAN_TX_CONSECUTIVE_FRAME;

	// 만들고,
	CAN_MakeSendFrame(pOutCanPacket, bIsStandardCan, CAN_CONSECUTICE_FRAME, pWriteMsg);
	if(g_ulProtocolID==ISO15765_SMK)
	{		
	}
	else 
	{
		if ( bIsStandardCan == eCAN_CLASSIC_STANDARD)	g_uiCanWriteMsgLength += (pOutCanPacket->stNormalPacket.ucDLC-1);
#ifdef CANFD_qhyek //Q_hyek CANFD
		else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD)	g_uiCanWriteMsgLength += (pOutCanPacket->stFDStdPacket.ucDLC-1);
#endif
		else											g_uiCanWriteMsgLength += (pOutCanPacket->stExtendPacket.ucDLC-1);
	}

	// send 하고 
	CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, g_ucCAN_CH, CAN_CONSECUTICE_FRAME);

	if(g_ulProtocolID==ISO15765_SMK)
	{
	  	if(pWriteMsg->DataSize > 300)
			osDelay(1);
		if ( (pWriteMsg->DataSize - 2/*CANID 사이즈 추가*/) > g_uiCanWriteMsgLength )
		{
			eCanTxState = eCAN_TX_CONSECUTIVE_FRAME;
		}
		else
		{
			g_bCF_TxComplete=1;
			eCanTxState = eCAN_RX_BLOCK;
		}
	}
	else
	{
        uint8_t ucCanIdLen = 0;
        if ( bIsStandardCan == 1 )              ucCanIdLen = 2;               // Standard Can Id Length
        else                                    ucCanIdLen = 4;               // Extended Can Id Length   
        if ( (pWriteMsg->DataSize - ucCanIdLen - 1) > g_uiCanWriteMsgLength ) // CanId, DataLength 
		{
			if ( g_stECUSetConfig.nBSTx != 0 )
			{
				if ( (g_uiCanTxConsFrameNo % g_stECUSetConfig.nBSTx) == 0 )
				{
					eCanTxState = eCAN_RX_BLOCK;
				}
				else
				{
					eCanTxState = eCAN_TX_CONSECUTIVE_FRAME;
				}
			}
			else
			{
				eCanTxState = eCAN_TX_CONSECUTIVE_FRAME;
			}
		}
		else
		{
			g_bCF_TxComplete=1;
			eCanTxState = eCAN_RX_BLOCK;
		}
	}
	//GLogI("\r\nCan TX : pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d, eCanTxState %d\r\n", 
	//					pWriteMsg->DataSize, g_uiCanWriteMsgLength, eCanTxState);

#if defined(DEBUG_CAN_LOG)
	GLogI("\r\n[%s] Can TX : pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d , %d, %d\r\n", 
						__FUNCTION__, pWriteMsg->DataSize, g_uiCanWriteMsgLength, g_uiCanTxConsFrameNo % 0x10, eCanTxState);
#endif
	if(GetCurFwServiceMode()!=eApp_Inside)	g_bAckflag = 1;
	intDlccomCount = g_uiAckTiming; // Ack Timming
	
	return eCanTxState;
}

eDiagCanState CAN_MakeRxSingleFrame_EX(stCanPacket *pInCanPacket, U8 bIsStandardCan, eDiagCanState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
	unsigned int /*uiProtocolID,*/ uiCopyLen, /*uiCanIDLen,*/ uiArrayIndex = 0;
	unsigned char *pCanData;
	eCanRxState = eCAN_NONE_STATE;

#if defined(DEBUG_CAN_LOG)
	GLogI("[%s] run\r\n", __FUNCTION__);
#endif

	//uiProtocolID = VCI_GetPassThruProtocolID();
	 
	//CAN_AcquireArrayIndexFromCarbCanID(pInCanPacket, bIsStandardCan, &uiArrayIndex);
	
	//uiCanIDLen = 4;
	pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stExtendPacket.us11BitID>>6;
	pReadMsg->pData[uiArrayIndex]   = (pInCanPacket->stExtendPacket.us11BitID & 0x3F)<<2;
	pReadMsg->pData[uiArrayIndex++] += (pInCanPacket->stExtendPacket.us18BitID& 0x30000)>>16;
	pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF00)>>8;
	pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.us18BitID & 0xFF);

	uiCopyLen = pInCanPacket->stExtendPacket.ucDLC;
	
	pCanData = &pInCanPacket->stExtendPacket.arrDataFields[0];

//	if( uiProtocolID == ISO15765_CARB_NEW_LENGTH ||
//	   	uiProtocolID == ISO15765_CARB_29BIT_NEW	)
//	{
//	  	pReadMsg->pData[uiArrayIndex++] = 0;
//	  	pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.arrDataFields[0]&0x0F;
//		g_uiCanReadMsgLength += CARB_LENGTH_LEN;
//		pReadMsg->DataSize += CARB_LENGTH_LEN;
//	}

	memcpy(&pReadMsg->pData[uiArrayIndex], pCanData, uiCopyLen);
	pReadMsg->DataSize += uiCopyLen;
	
//	g_uiCanReadMsgLength += uiCanIDLen;
//	g_uiCanReadMsgLength += uiCopyLen;
	
	//CAN_SaveCarbCanRecvInfo(pInCanPacket, bIsStandardCan, uiCanIDLen, uiCopyLen, uiCopyLen);
//	if( uiProtocolID == ISO15765_CARB 				|| 
//	   	uiProtocolID == ISO15765_CARB_NEW 			||
//	   	uiProtocolID == ISO15765_CARB_NEW_LENGTH 	||
//	   	uiProtocolID == ISO15765_CUBIS 				|| 
//	   	uiProtocolID == ISO15765_ACU_SINGLE) 
//	{
//		g_bCanRxCarbFrame = TRUE;
//		eCanRxState = eCAN_RX_BLOCK; //g_ulCanWrittenTick = OemGetTmr();
//	}
//	else
	{
		if ( pReadMsg->DataSize > 0 )
		{
			CAN_ClearCarbCanRecvInfo();
			eCanRxState = eCAN_RX_BLOCK_EX;	//PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType);
//			GLogN("CAN RX[%02d] ", pReadMsg->DataSize);
//			for(uint8_t i=0; i < (uiCanIDLen+uiCopyLen); i++)
//			{
//			  GLogN("%02X", pReadMsg->pData[i]);
//			}
//			GLogN("\r\n");
		}
		g_bCanRxCarbFrame = FALSE;
		//g_uiCanReadMsgLength = 0;
	}
	return eCanRxState;
}

eDiagCanState CAN_MakeTxSingleFrame_EX(stCanPacket *pOutCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pWriteMsg)
{
  	unsigned long uiProtocolID;

	uiProtocolID = VCI_GetPassThruProtocolID();
	
	eDiagCanState eCanTxState = eCAN_NONE_STATE;

	// 만들고,
	CAN_MakeSendFrame_EX(pOutCanPacket, bIsStandardCan, CAN_SINGLE_FRAME, pWriteMsg);

	// send 하고 
	if ( uiProtocolID != J1939_4PGN )
		CAN_WriteBuff((unsigned char*)pOutCanPacket, 0, g_ucCAN_CH, CAN_SINGLE_FRAME);
	
	if(g_ucCanEx_Func != 1)
	{
		intDlccomCount = g_uiAckTiming;
		if( g_ucCanEx_Func == 9 || g_ucCanEx_Func == 10 )
			g_ulTxdRxdCount = 4000;
		else
			g_ulTxdRxdCount = g_stGITSetConfig.nP3Min;
	}
	else
	{
		g_ulTxdRxdCount = 3000;
	}

	eCanTxState = eCAN_RX_BLOCK_EX;
	
	g_uiCanWriteMsgLength = pWriteMsg->DataSize;
#if defined(DEBUG_CAN_LOG)
	GLogI("\r\n[%s] pWriteMsg->DataSize %d, g_uiCanWriteMsgLength %d \r\n", __FUNCTION__, pWriteMsg->DataSize, g_uiCanWriteMsgLength);
#endif

	return eCanTxState;
}

bool SendTxMsgToTxThread(stPASSTHRU_MSG *pReadMsg)
{
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;

	pDiagmsg = ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
	if( pDiagmsg == NULL )
	{
	  	return false;
	}
	pPTpacket = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
	if( pPTpacket == NULL )
	{
		osPoolFree( hDiagPool, (void *)pDiagmsg );
		return false;
	}
	
	memcpy(pPTpacket, pReadMsg, sizeof(PTmsgPkt_t));
	pPTpacket->RxStatus = 1;
	pDiagmsg->pPacket = pPTpacket;
	pDiagmsg->subEvent = eCAN_TX_NONE_PARSING;

	if(osMessageAvailableSpace(hOBDTxMessage) == 0)
	{
		osPoolFree( hPTPKPool, (void *)pPTpacket );
		osPoolFree( hDiagPool, (void *)pDiagmsg );
		return false;
	}
	else
	{
		osMessagePut( hOBDTxMessage, (uint32_t)pDiagmsg, osWaitForever );
	}
	
	return true;
}

eDiagCanState CAN_J1939_FuncProc (stCanPacket *pInCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pReadMsg, bool bRecvResult)
{
	eDiagCanState eCanRxState = eCAN_NONE_STATE;
	uint32_t ulCanIdEx = 0;
	uint32_t ulCheckPGN = 0;
	uint8_t i = 0;
	static uint8_t ucTpEnd = 0;
	static uint32_t ulRetryRecvCnt = 0, ulRetrySendCnt = 0;
  	static uint32_t ulJ1939_FrameLength = 0, ulReceiveFramelength = 0;
	static uint8_t ucRxCanExBuff[500] = {0,};					// 위치 조정 필요
	static uint8_t ucTempBuff[10] = {0,};
	uint32_t nStartMask;
	uint32_t nEndMask;
	uint32_t nCanType;
	uint32_t nCANBaudrate;
	uint8_t ucCanIdEx_Len = 4;
	
	nCanType = EXTENDED_CAN;
	nCANBaudrate = g_stGITSetConfig.nDataRate;
	
	for (i = 0; i < sizeof(uint32_t); i++)
		ulCanIdEx |= (((uint32_t)pReadMsg->pData[i]) << (24 - (i*8)));
		
	ulCheckPGN = ulCanIdEx & 0x00FFFF00;
	
	switch( g_ucCanEx_Func )
	{
		case 1:
		{
			if( g_bCanEx_First == 0 )						// TX Thread로 옮기자
			{
			  	g_uiCanReadMsgLength = 0;
				g_ulTxdRxdCount = 2100;
				g_bCanEx_First = TRUE;
				memset(ucRxCanExBuff, 0x00, sizeof(ucRxCanExBuff));
			}
			if ( bRecvResult == TRUE)
			{
				if( ulCheckPGN == 0x00FECA00 )
				{
					if( g_ucCanEx_State == 0 )
					{
						// If DTC is single frame, should to offset 8bytes for sincronizing like to multi frame.
						memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[0], (pReadMsg->DataSize+ucCanIdEx_Len) );
						memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength+(pReadMsg->DataSize+ucCanIdEx_Len)], &pReadMsg->pData[4], pReadMsg->DataSize );
						g_uiCanReadMsgLength += ((pReadMsg->DataSize*2) + ucCanIdEx_Len - 4);
					}
					g_ulTxdRxdCount = 1100;
					g_ucCanEx_State = 2;
				}
				else if( (ulCheckPGN == 0x00ECFF00) && (pReadMsg->pData[9] == 0xCA) )
				{
					if( g_ucCanEx_State == 0 )
					{
						memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[0], (pReadMsg->DataSize+ucCanIdEx_Len) );
						g_uiCanReadMsgLength += (pReadMsg->DataSize+ucCanIdEx_Len);
					}
					g_ulTxdRxdCount = 1100;
					g_ucCanEx_State++;
				}
				else if( (ulCheckPGN == 0x00EBFF00) && ( g_ucCanEx_State >= 1) )
				{
					memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[5], (pReadMsg->DataSize + ucCanIdEx_Len - 5) );
					g_uiCanReadMsgLength += (pReadMsg->DataSize + ucCanIdEx_Len - 5);
					g_ulTxdRxdCount = 1100;
				}
			}
			
			eCanRxState = eCAN_RX_BLOCK_EX;
			
			if( g_ulTxdRxdCount == 0 || g_ucCanEx_State == 2 )
			{
				g_bCanEx_First = FALSE;
				
				if( g_ulTxdRxdCount == 0 || g_ucCanEx_State == 0 )
				{
					eCanRxState = eCAN_RX_FAIL;
				}
				else if( g_ucCanEx_State == 2 )
				{
					memcpy( &pReadMsg->pData[0], &ucRxCanExBuff[0], g_uiCanReadMsgLength );
					eCanRxState = eCAN_RX_COMPLETE;
				}
				
				g_ucCanEx_State = 0;
				pReadMsg->DataSize = g_uiCanReadMsgLength;
				if( pReadMsg->pData[4] == 0x20 ) 
				{
					pReadMsg->DataSize = 12 + pReadMsg->pData[5];
				}
				g_ulTxdRxdCount = 1000;
				nStartMask = 0x00000000;
				nEndMask = 0x1FFFFFFF;
				CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
			}
			break;
		}
		case 2:
		case 4:
		{
			if( g_ucCanEx_Func == 4 )
			{
				pReadMsg->DataSize = 8;
				eCanRxState = eCAN_RX_COMPLETE;
			}
			else
			{
				if( g_bCanEx_First == TRUE )
				{
					g_ulTxdRxdCount = 2100;
					g_bCanEx_First = TRUE;
					memset(ucRxCanExBuff, 0x00, sizeof(ucRxCanExBuff));
				}
				if ( bRecvResult == TRUE)
				{
					if( ulCheckPGN == 0x00E8FF00 )
					{
						if( g_ucCanEx_State == 0 )
						{
							memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[0], (pReadMsg->DataSize+ucCanIdEx_Len) );
							g_uiCanReadMsgLength += (pReadMsg->DataSize+ucCanIdEx_Len);
							g_ucCanEx_State = 1;
						}
					}
					
					if ( g_ucCanEx_State == 1 )
					{
						memcpy( &pReadMsg->pData[0], &ucRxCanExBuff[0], g_uiCanReadMsgLength );
						//g_uiCanReadMsgLength = 0;
						g_bCanEx_First = FALSE;
						g_ucCanEx_State = 0;
						pReadMsg->DataSize = 8;
						g_ulTxdRxdCount = 1000;
						eCanRxState = eCAN_RX_COMPLETE;
					}
					else
					{
						eCanRxState = eCAN_RX_BLOCK_EX;
					}
					pReadMsg->DataSize = 0;
				}
				else
				{
					osDelay(100);
					eCanRxState = eCAN_RX_BLOCK_EX;
				}
				
				if( g_ulTxdRxdCount == 0 )
				{
					pReadMsg->DataSize = 8;
					g_ulTxdRxdCount = 1000;
					eCanRxState = eCAN_RX_FAIL;
				}
			}
			break;
		}
		case 3:
		{
			if ( bRecvResult == TRUE)
			{
				if( ulCheckPGN == g_ulPGN )
				{
					g_bCanEx_First = FALSE;
					eCanRxState = eCAN_RX_COMPLETE;
					pReadMsg->DataSize += ucCanIdEx_Len;
					ulRetryRecvCnt = 0;
					ulRetrySendCnt = 0;
				}
				else
				{
					if( ulRetryRecvCnt >= 100 )
					{
						ulRetryRecvCnt = 0;
						ulRetrySendCnt++;
						uint8_t ucTP[13] = {0x18, 0xea, 0x00, 0xf9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
						pReadMsg->DataSize = 3;
						ucTP[4] = pReadMsg->DataSize;
						ucTP[5] = (uint8_t)(g_ulPGN>>8)&0xFF;
						ucTP[6] = (uint8_t)(g_ulPGN>>16)&0xFF;
						memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
						SendTxMsgToTxThread(pReadMsg);
						g_uiCanWriteMsgLength = 0;
						eCanRxState = eCAN_RX_BLOCK_EX;
						if( ulRetrySendCnt >= 10 )	
						{
							ulRetrySendCnt = 0;
							ulRetryRecvCnt = 0;
							pReadMsg->DataSize = 0;		//rx 초기화
							eCanRxState = eCAN_RX_FAIL;
						}
					}
					else
					{
						ulRetryRecvCnt++;
						pReadMsg->DataSize = 0;		//rx 초기화
						eCanRxState = eCAN_RX_BLOCK_EX;
					}
				}
			}
			else
			{
				ulRetryRecvCnt++;
				osDelay(100);
				pReadMsg->DataSize = 0;		//rx 초기화
				eCanRxState = eCAN_RX_BLOCK_EX;
			}
			break;
		}
		case 5:
		{
			if ( bRecvResult == TRUE)
			{
				if( ulCheckPGN == g_ulPGN )
				{
					memset( ucRxCanExBuff, 0x00, sizeof(ucRxCanExBuff));
					memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[0], (pReadMsg->DataSize+ucCanIdEx_Len) );
					pReadMsg->pData[5]  = ucRxCanExBuff[7];
					pReadMsg->pData[6]  = ucRxCanExBuff[6];
					pReadMsg->pData[7]  = ucRxCanExBuff[5];
					pReadMsg->pData[8]  = ucRxCanExBuff[10];
					pReadMsg->pData[9]  = ucRxCanExBuff[9];
					pReadMsg->pData[10] = ucRxCanExBuff[8];
					eCanRxState = eCAN_RX_COMPLETE;
				}
				else
				{
					int8_t ucTP[13] = {0x18, 0xea, 0x00, 0xf9, 0x00, 0xb1, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
					pReadMsg->DataSize = 3;
					ucTP[4] = pReadMsg->DataSize;
					memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
					SendTxMsgToTxThread(pReadMsg);
					g_uiCanWriteMsgLength = 0;
					eCanRxState = eCAN_RX_BLOCK_EX;
				}
			}
			else
			{
				osDelay(100);
				eCanRxState = eCAN_RX_BLOCK_EX;
				pReadMsg->DataSize = 0;
			}
			break;
		}
		case 6:
		{
			if ( bRecvResult == TRUE)	// data received
			{
				if( ulCanIdEx == g_ulPGN || intTxdRxdCount==0 )
				{
					intTxdRxdCount=0;
					g_bCanEx_First = FALSE;
					pReadMsg->DataSize += ucCanIdEx_Len;
					eCanRxState = eCAN_RX_COMPLETE;
				}
				else
				{
					pReadMsg->DataSize = 0;
					eCanRxState = eCAN_RX_BLOCK_EX;
				}
			}
			else
			{
				intTxdRxdCount=0;
				eCanRxState = eCAN_RX_FAIL;
				pReadMsg->DataSize = 0;
			}
			break;
		}
		case 7:
		{
			if( g_bCanEx_First == TRUE )
			{
				g_ulTxdRxdCount = 2100;
				g_bCanEx_First = TRUE;
				memset(ucRxCanExBuff, 0x00, sizeof(ucRxCanExBuff));
			}
			if ( bRecvResult == TRUE)
			{
				if( ulCheckPGN == 0x00FECB00 )
				{
					if( g_ucCanEx_State == 0 )
					{
						// If DTC is single frame, should to offset 8bytes for sincronizing like to multi frame.
						memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[0], (pReadMsg->DataSize+ucCanIdEx_Len) );
						memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength+(pReadMsg->DataSize+ucCanIdEx_Len)], &pReadMsg->pData[4], pReadMsg->DataSize );
						g_uiCanReadMsgLength += ((pReadMsg->DataSize*2) + ucCanIdEx_Len);
					}
					g_ulTxdRxdCount = 1100;
					g_ucCanEx_State = 2;
					pReadMsg->DataSize = 0;		//rx 초기화
					eCanRxState = eCAN_RX_BLOCK_EX;
				}
				else if( ((ulCheckPGN == 0x00ECFF00) || (ulCheckPGN == 0x00ECF900)) && (pReadMsg->pData[9] == 0xCB) )
				{
					ucTempBuff[1] = pReadMsg->pData[5];
					ucTempBuff[2] = pReadMsg->pData[6];
					ucTempBuff[3] = pReadMsg->pData[7];
					if( g_ucCanEx_State == 0 )
					{
						memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[0], (pReadMsg->DataSize+ucCanIdEx_Len) );
						g_uiCanReadMsgLength += (pReadMsg->DataSize+ucCanIdEx_Len);
					}
					g_ulTxdRxdCount = 1100;
					g_ucCanEx_State++;

					if( (ulCheckPGN == 0x00ECFF00) && (ucTpEnd == 0) )
					{
					  	uint8_t ucTP[13] = {0x1c, 0xec, 0x00, 0xf9, 0x00, 0x11, 0x00, 0x01, 0xff, 0xff, 0xcb, 0xfe, 0x00};
						pReadMsg->DataSize = 8;
						ucTP[4] = pReadMsg->DataSize;
						ucTP[6] = ucTempBuff[3];
						memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
						SendTxMsgToTxThread(pReadMsg);
						g_uiCanWriteMsgLength = 0;
						ucTpEnd = 1;
						eCanRxState = eCAN_RX_BLOCK_EX;
					}
				}
				else if( ((ulCheckPGN == 0x00EBFF00) || (ulCheckPGN == 0x00EBF900)) && ( g_ucCanEx_State >= 1) )
				{
					memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[5], (pReadMsg->DataSize+ucCanIdEx_Len-5) );
					g_uiCanReadMsgLength += pReadMsg->DataSize+ucCanIdEx_Len-5;
					g_ulTxdRxdCount = 500;
					eCanRxState = eCAN_RX_BLOCK_EX;
					pReadMsg->DataSize = 0;		//rx 초기화
				}
				else
				{
				  	eCanRxState = eCAN_RX_BLOCK_EX;
					pReadMsg->DataSize = 0;		//rx 초기화
				}
			}
			else
			{
				osDelay(100);
				pReadMsg->DataSize = 0;
				eCanRxState = eCAN_RX_BLOCK_EX;
			}
			
			if( ucTpEnd == 1 )
			{
			  	uint8_t ucTP[13] = {0x1c, 0xec, 0x00, 0xf9, 0x00, 0x13, 0x00, 0x00, 0x00, 0xff, 0xcb, 0xfe, 0x00};
				pReadMsg->DataSize = 8;
				ucTP[4] = pReadMsg->DataSize;
				ucTP[5] = ucTempBuff[1];
				ucTP[6] = ucTempBuff[2];
				ucTP[7] = ucTempBuff[3];
				memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
				SendTxMsgToTxThread(pReadMsg);
				g_uiCanWriteMsgLength = 0;
				eCanRxState = eCAN_RX_BLOCK_EX;
				break;
			}
			
			if( (g_ulTxdRxdCount == 0) || (g_ucCanEx_State == 2) )
			{	
				if( (g_ulTxdRxdCount == 0) || (g_ucCanEx_State == 0) )
				{
					eCanRxState = eCAN_RX_FAIL;
				}
				else if(g_ucCanEx_State == 2)
				{
					memcpy( &pReadMsg->pData[0], &ucRxCanExBuff[0], g_uiCanReadMsgLength );
					eCanRxState = eCAN_RX_COMPLETE;
				}
				
				g_bCanEx_First = FALSE;
				g_ucCanEx_State = 0;
				
				pReadMsg->DataSize = g_uiCanReadMsgLength;
				if( pReadMsg->pData[4] == 0x20 ) 
				{
					pReadMsg->DataSize = 12 + pReadMsg->pData[5];
				}
				g_ulTxdRxdCount = 1000;
				nStartMask = 0x00000000;
				nEndMask = 0x1FFFFFFF;
				CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
			}
			break;
		}
		case 9:
		{
			if( g_bCanEx_First == TRUE )
			{
				g_ulTxdRxdCount = 4100;
				g_bCanEx_First = TRUE;
				memset(ucRxCanExBuff, 0x00, sizeof(ucRxCanExBuff));
			}
			if( bRecvResult == TRUE)
			{
				if( ((ulCheckPGN == 0x00ECFF00) || (ulCheckPGN == 0x00ECF900)) 
							&& ((pReadMsg->pData[9] == 0xB4) || (pReadMsg->pData[9] == 0xB7) || (pReadMsg->pData[9] == 0x00) || (pReadMsg->pData[10] == 0xC2)) )
				{
					if( g_ucCanEx_State == 0 )
					{
						memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[0], (pReadMsg->DataSize+ucCanIdEx_Len) );
						g_uiCanReadMsgLength += (pReadMsg->DataSize+ucCanIdEx_Len);
					}
					g_ulTxdRxdCount = 1100;
					g_ucCanEx_State++;
					if( (ulCheckPGN == 0x00ECF900) && (pReadMsg->pData[9] == 0xB4) && (ucTpEnd == 0) )
					{
						uint8_t ucTP[13] = {0x1c, 0xec, 0x00, 0xf9, 0x00, 0x11, 0x12, 0x01, 0xff, 0xff, 0xb4, 0xff, 0x00};
						pReadMsg->DataSize = 8;
						ucTP[4] = pReadMsg->DataSize;
						memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
						SendTxMsgToTxThread(pReadMsg);
						g_uiCanWriteMsgLength = 0;
						ucTpEnd = 1;
					}
					else if( (ulCheckPGN == 0x00ECF900) && (pReadMsg->pData[9] == 0xB7) && (ucTpEnd == 0) )
					{
						uint8_t ucTP[13] = {0x18, 0xec, 0x00, 0xf9, 0x00, 0x11, 0x00, 0x01, 0xff, 0xff, 0xb7, 0xfd, 0x00};
						pReadMsg->DataSize = 8;
						ucTP[4] = pReadMsg->DataSize;
						ucTP[6] = pReadMsg->pData[7];
						memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
						SendTxMsgToTxThread(pReadMsg);
						g_uiCanWriteMsgLength = 0;
						ucTpEnd = 1;
					}
					else if( (ulCheckPGN == 0x00ECF900) && (pReadMsg->pData[9] == 0x00) && (pReadMsg->pData[10] == 0xC2) && (ucTpEnd == 0) )
					{
						uint8_t ucTP[13] = {0x18, 0xec, 0x00, 0xf9, 0x00, 0x11, 0x00, 0x01, 0xff, 0xff, 0x00, 0xc2, 0x00};
						pReadMsg->DataSize = 8;
						ucTP[4] = pReadMsg->DataSize;
						ucTP[6] = pReadMsg->pData[7];
						ulJ1939_FrameLength = pReadMsg->pData[7];
						memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
						SendTxMsgToTxThread(pReadMsg);
						g_uiCanWriteMsgLength = 0;
						ucTpEnd = 2;
					}
					eCanRxState = eCAN_RX_BLOCK_EX;
				}
				//else if( ((ulCheckPGN == 0x00EBFF00) || (ulCheckPGN == 0x00EBF900)) && ( g_ucCanEx_State >= 1) )
				else if((ulCheckPGN == 0x00EBF900) && ( g_ucCanEx_State >= 1) )
				{
					memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[5], (pReadMsg->DataSize+ucCanIdEx_Len-5) );
					g_uiCanReadMsgLength += (pReadMsg->DataSize+ucCanIdEx_Len-5);
					g_ulTxdRxdCount = 500;
					eCanRxState = eCAN_RX_BLOCK_EX;
					if( ucTpEnd == 2 )
					{
						ulReceiveFramelength++;
						if( ulJ1939_FrameLength == ulReceiveFramelength )
						{
							ulJ1939_FrameLength = 0;
							ulReceiveFramelength = 0;
							ucTpEnd = 3;
							g_ucCanEx_State = 2;
							eCanRxState = eCAN_RX_COMPLETE;
						}
					}
				}
				else
				{
				  	eCanRxState = eCAN_RX_BLOCK_EX;
					pReadMsg->DataSize = 0;		//rx 초기화
				}
			}
			else
			{
				osDelay(100);
				pReadMsg->DataSize = 0;
				eCanRxState = eCAN_RX_BLOCK_EX;
			}
			
			if( (g_ulTxdRxdCount == 0) || (g_ucCanEx_State == 2) )
			{
				if( (ucTpEnd == 1) && (pReadMsg->pData[9] == 0xB4))
				{
					uint8_t ucTP[13] = {0x1c, 0xec, 0x00, 0xf9, 0x00, 0x13, 0x7d, 0x00, 0x12, 0xff, 0xb4, 0xff, 0x00};
					pReadMsg->DataSize = 8;
					ucTP[4] = pReadMsg->DataSize;
					memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
					SendTxMsgToTxThread(pReadMsg);
					g_uiCanWriteMsgLength = 0;
				}
				else if( (ucTpEnd == 1) && (pReadMsg->pData[9] == 0xB7))
				{
					uint8_t ucTP[13] = {0x18, 0xec, 0x00, 0xf9, 0x00, 0x13, 0x00, 0x00, 0x00, 0xff, 0xb7, 0xfd, 0x00};
					pReadMsg->DataSize = 8;
					ucTP[4] = pReadMsg->DataSize;
					ucTP[6] = pReadMsg->pData[5];
					ucTP[7] = pReadMsg->pData[6];
					ucTP[8] = pReadMsg->pData[7];
					memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
					SendTxMsgToTxThread(pReadMsg);
					g_uiCanWriteMsgLength = 0;
				}
				else if( (ucTpEnd == 3) )
				{
					uint8_t ucTP[13] = {0x18, 0xec, 0x00, 0xf9, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc2, 0x00};
					pReadMsg->DataSize = 8;
					ucTP[4] = pReadMsg->DataSize;
					ucTP[6] = pReadMsg->pData[5];
					ucTP[7] = pReadMsg->pData[6];
					ucTP[8] = pReadMsg->pData[7];
					memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
					SendTxMsgToTxThread(pReadMsg);
					g_uiCanWriteMsgLength = 0;
					ucTpEnd = 0;
				}
				
				if( (g_ulTxdRxdCount == 0) || (g_ucCanEx_State == 0) )
				{
					eCanRxState = eCAN_RX_FAIL;
					ucTpEnd = 0;
				}
				else if(g_ucCanEx_State == 2)
				{
					memcpy( &pReadMsg->pData[0], &ucRxCanExBuff[0], g_uiCanReadMsgLength );
					eCanRxState = eCAN_RX_COMPLETE;
					ucTpEnd = 0;
				}
				
				pReadMsg->DataSize = g_uiCanReadMsgLength;
				
				if( (pReadMsg->pData[4] == 0x20) || (pReadMsg->pData[4] == 0x10) ) 
				{
					pReadMsg->DataSize = 12 + pReadMsg->pData[5] +  (pReadMsg->pData[6]* 0x100);
				}
				
				g_bCanEx_First = FALSE;
				g_ucCanEx_State = 0;
				
				g_ulTxdRxdCount = 1000;
				nStartMask = 0x00000000;
				nEndMask = 0x1FFFFFFF;
				CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
			}
			break;
		}
		case 10:
		{
			if( g_bCanEx_First == TRUE )
			{
				g_ulTxdRxdCount = 4100;
				g_bCanEx_First = TRUE;
				memset(ucRxCanExBuff, 0x00, sizeof(ucRxCanExBuff));
			}
			
			if( bRecvResult == TRUE)
			{
				if( ((ulCheckPGN == 0x00ECFF00) || (ulCheckPGN == 0x00ECF900)) && (pReadMsg->pData[9] == 0xDA) )
				{
					if( g_ucCanEx_State == 0 )
					{
						memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[0], (pReadMsg->DataSize+ucCanIdEx_Len) );
						g_uiCanReadMsgLength += (pReadMsg->DataSize+ucCanIdEx_Len);
					}
					g_ulTxdRxdCount = 1100;
					g_ucCanEx_State++;
					eCanRxState = eCAN_RX_BLOCK_EX;
					
					if( (ulCheckPGN == 0x00ECF900) && (ucTpEnd == 0) )
					{
						uint8_t ucTP[13] = {0x1c, 0xec, 0x00, 0xf9, 0x00, 0x11, 0x04, 0x01, 0xff, 0xff, 0xda, 0xfe, 0x00};
						pReadMsg->DataSize = 8;
						ucTP[4] = pReadMsg->DataSize;
						memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
						SendTxMsgToTxThread(pReadMsg);
						g_uiCanWriteMsgLength = 0;
						ucTpEnd = 1;
						eCanRxState = eCAN_RX_BLOCK_EX;
					}
				}
				else if( ((ulCheckPGN == 0x00EBFF00) || (ulCheckPGN == 0x00EBF900)) && ( g_ucCanEx_State >= 1) )
				{
					memcpy( &ucRxCanExBuff[g_uiCanReadMsgLength], &pReadMsg->pData[5], (pReadMsg->DataSize+ucCanIdEx_Len-5) );
					g_uiCanReadMsgLength += (pReadMsg->DataSize+ucCanIdEx_Len-5);
					g_ulTxdRxdCount = 500;
					eCanRxState = eCAN_RX_BLOCK_EX;
				}
				else
				{
				  	eCanRxState = eCAN_RX_BLOCK_EX;
					pReadMsg->DataSize = 0;		//rx 초기화
				}
			}
			else
			{
				osDelay(100);
				pReadMsg->DataSize = 0;
				eCanRxState = eCAN_RX_BLOCK_EX;
			}
			
			if( (g_ulTxdRxdCount == 0) || (g_ucCanEx_State == 2) )
			{	
				if( ucTpEnd == 1 )
				{
					uint8_t ucTP[13] = {0x1c, 0xec, 0x00, 0xf9, 0x00, 0x13, 0x16, 0x00, 0x04, 0xff, 0xda, 0xfe, 0x00};
					pReadMsg->DataSize = 8;
					ucTP[4] = pReadMsg->DataSize;
					ucTpEnd = 0;
					memcpy( &pReadMsg->pData[0], &ucTP[0], sizeof(ucTP) );
					SendTxMsgToTxThread(pReadMsg);
					g_uiCanWriteMsgLength = 0;
				}
				
				if( (g_ulTxdRxdCount == 0) || (g_ucCanEx_State == 0) )
				{
					eCanRxState = eCAN_RX_FAIL;
				}
				else if(g_ucCanEx_State == 2)
				{
					memcpy( &pReadMsg->pData[0], &ucRxCanExBuff[0], g_uiCanReadMsgLength );
					eCanRxState = eCAN_RX_COMPLETE;
				}
				
				pReadMsg->DataSize = g_uiCanReadMsgLength;
				if( (pReadMsg->pData[4] == 0x20) || (pReadMsg->pData[4] == 0x10) ) 
				{
					pReadMsg->DataSize = 12 + pReadMsg->pData[5];
				}
				if( pReadMsg->pData[12] == 0x00 )
				{
					pReadMsg->DataSize -= 7;
				}
				
				g_bCanEx_First = FALSE;
				g_ucCanEx_State = 0;
				
				g_ulTxdRxdCount = 1000;
				nStartMask = 0x00000000;
				nEndMask = 0x1FFFFFFF;
				CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, 1, &nStartMask, &nEndMask);
			}
			break;
		}
		default:
		{
			break;
		}
	}
	return eCanRxState;
}

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
	if(uiOrder>0)	return g_CarbRecvInfo[uiOrder-1].uiCANRecvedLength;
	else return 0;
}

unsigned int CAN_GetCarbTotalPacketLength(unsigned int uiOrder)
{
	if(uiOrder>0)	return g_CarbRecvInfo[uiOrder-1].uiCANTotalPacketLength;
	else return 0;
}

void CAN_SaveCarbCanRecvLength(unsigned int uiOrder, unsigned int uiRecvedLen)
{
	if(uiOrder>0)	g_CarbRecvInfo[uiOrder-1].uiCANRecvedLength += uiRecvedLen;
#if defined(DEBUG_CAN_LOG)
	GLogI("[%s] uiRecvedLen %d, g_CarbRecvInfo[%d].uiCANRecvedLength %d\r\n", 
		__FUNCTION__, uiRecvedLen, uiOrder-1, g_CarbRecvInfo[uiOrder-1].uiCANRecvedLength);
#endif
}

void CAN_ClearCarbCanRecvInfo()
{
	memset(g_CarbRecvInfo, 0x00, sizeof(CARB_REVC_INFO)*MAX_CARB_RECV_ARRAY_CNT);
}

unsigned int CAN_SaveCarbCanRecvInfo(stCanPacket *pInCanPacket, U8 bIsStandardCan, unsigned int uiCanIDLen, unsigned int uiRecvedLen, unsigned int uiRecvPacketLen)
{
	int i;
	unsigned long ulCANID;
	unsigned int uiSavePos = 0;
	unsigned long uiProtocolID;

	uiProtocolID = VCI_GetPassThruProtocolID();

	if ( bIsStandardCan == eCAN_CLASSIC_STANDARD)
		ulCANID = pInCanPacket->stNormalPacket.us11BitID;
#ifdef CANFD_qhyek //Q_hyek CANFD
	else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD )
		ulCANID = pInCanPacket->stFDStdPacket.us11BitID;
#endif
	else
		ulCANID = ((pInCanPacket->stExtendPacket.us11BitID <<18) & 0x1FFC0000) | (pInCanPacket->stExtendPacket.us18BitID&0x3FFFF);

	for ( i=0; i<MAX_CARB_RECV_ARRAY_CNT; i++ )
	{
		if ( (g_CarbRecvInfo[i].ulCANID == 0) && (g_CarbRecvInfo[i].uiCANTotalPacketLength == 0) )
			break;
		else
		{
			uiSavePos += g_CarbRecvInfo[i].uiCANIDLength;
			uiSavePos += g_CarbRecvInfo[i].uiCANTotalPacketLength;
			
			if( uiProtocolID == ISO15765_CARB_NEW_LENGTH ||
                uiProtocolID == J1939_23_CARB_NEW_LENGTH ||
                uiProtocolID == J1939_23_CARB_29BIT_NEW  ||
				uiProtocolID == ISO15765_CARB_29BIT_NEW	)
			{
			  	uiSavePos += CARB_LENGTH_LEN;
			}
		}
	}

	if ( i >= MAX_CARB_RECV_ARRAY_CNT )
	{
		// 이상태가 나오면 안된다. Buffer를 키워줘라.
		GLogE("\r\n\r\n\r\n\r\n\r\n[%s] !!!! Max Carb receive Buff.....so, return\r\n\r\n\r\n\r\n", __FUNCTION__);
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
	GLogI("[%s] Index %d, ulCANID 0x%04X, uiCANTotalPacketLength %d, uiRecvedLen %d, uiSavePos %d\r\n", 
		__FUNCTION__, i, ulCANID, uiRecvPacketLen, uiRecvedLen, uiSavePos);
#endif

	return i;
}

unsigned int  CAN_AcquireArrayIndexFromCarbCanID(stCanPacket *pInCanPacket, U8 bIsStandardCan, unsigned int* puiArrayIndex)
{
	unsigned int i, uiOrder = 0, uiSumCanIDLength = 0, uiLastPos = 0;
	unsigned long ulCANID;
	unsigned long uiProtocolID;

	uiProtocolID = VCI_GetPassThruProtocolID();

	if ( bIsStandardCan == eCAN_CLASSIC_STANDARD)
		ulCANID = pInCanPacket->stNormalPacket.us11BitID;
	else  if ( bIsStandardCan == eCAN_CLASSIC_EXTENDED)
		ulCANID = ((pInCanPacket->stExtendPacket.us11BitID <<18) & 0x1FFC0000) | (pInCanPacket->stExtendPacket.us18BitID&0x3FFFF);
	else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD)
		ulCANID = pInCanPacket->stFDStdPacket.us11BitID;
	else
		ulCANID = ((pInCanPacket->stFDExtPacket.us11BitID <<18) & 0x1FFC0000) | (pInCanPacket->stFDExtPacket.us18BitID&0x3FFFF);

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
				if( uiProtocolID == ISO15765_CARB_NEW_LENGTH ||
                    uiProtocolID == J1939_23_CARB_NEW_LENGTH ||
                    uiProtocolID == J1939_23_CARB_29BIT_NEW  ||
					uiProtocolID == ISO15765_CARB_29BIT_NEW	)
				{
					uiLastPos += CARB_LENGTH_LEN;
				}
			}
		}
	}
	
	if ( uiOrder > 0 )
	{
		*puiArrayIndex +=  g_CarbRecvInfo[uiOrder-1].uiCANDataSavePos;
		*puiArrayIndex += g_CarbRecvInfo[uiOrder-1].uiCANIDLength;
		*puiArrayIndex += g_CarbRecvInfo[uiOrder-1].uiCANRecvedLength;
		if( uiProtocolID == ISO15765_CARB_NEW_LENGTH ||
            uiProtocolID == J1939_23_CARB_NEW_LENGTH ||
            uiProtocolID == J1939_23_CARB_29BIT_NEW  ||
			uiProtocolID == ISO15765_CARB_29BIT_NEW	)
		{
			*puiArrayIndex += CARB_LENGTH_LEN;
		}
#if defined(DEBUG_CAN_LOG)
		GLogI("[%s] ulCANID 0x%04X, uiOrder %d, pos %d, *puiArrayIndex %d\r\n", 
			__FUNCTION__, ulCANID, uiOrder, g_CarbRecvInfo[uiOrder-1].uiCANDataSavePos, *puiArrayIndex);
#endif
	}
	else
	{
		*puiArrayIndex += uiSumCanIDLength + uiLastPos;
	}

	return uiOrder;
}

eDiagCanState CAN_RxBlockProc(stCanPacket *pInCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pReadMsg)
{
	eDiagCanState	eCanRxState = eCAN_NONE_STATE;
	unsigned long	uiProtocolID;
	static int		uiOldTimer_Timeout=0;
	bool			pendingStatus = false;
	uiProtocolID = VCI_GetPassThruProtocolID();
	
	if ( uiProtocolID == ISO15765_CUBIS || 
		 uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE || 
		 uiProtocolID == ISO15765_SINGLE || 
		 uiProtocolID == ISO15765_SINGLE_SMK || 
		 uiProtocolID == ISO15765_SINGLE_PODS || 
		 uiProtocolID == ISO15765_ACU_SINGLE )
	{
		eCanRxState = eCAN_RX_SINGLE_FRAME;
		//GLogN("[%04X]", pInCanPacket->stNormalPacket.us11BitID);
		//GLogN(" [%02X] [%02X]\r\n", pInCanPacket->stNormalPacket.arrDataFields[0],pInCanPacket->stNormalPacket.arrDataFields[1]);		
	}
	else if ( uiProtocolID == J1939 ||
			  uiProtocolID == J1939_4PGN )
	{
	  	eCanRxState = eCAN_RX_SINGLE_FRAME_EX; 
	}
	else if(uiProtocolID == ISO15765_SMK)			//110908 LWH										
 	{
 		U16 CanDataRxlen=0;
 		if(g_ucCan_FC!=1)											// CF일경우(Can_FC==1) candatarxlen 바뀌지 않도록
 		{
 			CanDataRxlen = (pInCanPacket->stNormalPacket.arrDataFields[1])*0x100+pInCanPacket->stNormalPacket.arrDataFields[0];
 		}
		
 		if(g_ucCan_FC==1)											// 퍼스트프레임 형식데이터 수신이후 CF 응답으로 예외처리 하기 위한 플래그
 		{
 			//CurMode = CAN_RX_CONSECUTIVE_FRAME;
 			eCanRxState = eCAN_RX_CONSECUTIVE_FRAME;
 		}
 		else if(CanDataRxlen>6)
 		{
			//CurMode = CAN_RX_FIRST_FRAME;
			eCanRxState = eCAN_RX_FIRST_FRAME;
			g_ucCan_FC = 0;
 		}
 		else
 		{
 			//CurMode = CAN_RX_SINGLE_FRAME;
 			eCanRxState = eCAN_RX_SINGLE_FRAME;
 		}
 	}
	else
	{
		if 		( bIsStandardCan == eCAN_CLASSIC_STANDARD )
			eCanRxState = (eDiagCanState)((pInCanPacket->stNormalPacket.arrDataFields[0] >> 4 & 0x0F) + eCAN_RX_SINGLE_FRAME);
#ifdef CANFD_qhyek //Q_hyek CANFD
		else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD )
			eCanRxState = (eDiagCanState)((pInCanPacket->stFDStdPacket.arrDataFields[0] >> 4 & 0x0F) + eCAN_RX_SINGLE_FRAME);
#endif
		else if ( bIsStandardCan == eCAN_CLASSIC_EXTENDED )
			eCanRxState = (eDiagCanState)( (pInCanPacket->stExtendPacket.arrDataFields[0] >> 4 & 0x0F) + eCAN_RX_SINGLE_FRAME);
		else
			eCanRxState = (eDiagCanState)( (pInCanPacket->stFDExtPacket.arrDataFields[0] >> 4 & 0x0F) + eCAN_RX_SINGLE_FRAME);
		
	  	if ( uiProtocolID == ISO15765_EXCEPT )
		{
			if( g_ucCanExceptState == 0x00)	g_ucCanExceptState = pInCanPacket->stNormalPacket.arrDataFields[0];
		  	else if((g_ucCanExceptState & 0xF0) == CAN_FIRST_FRAME)
			{
				if((pInCanPacket->stNormalPacket.arrDataFields[0] & 0xF0) != CAN_CONSECUTICE_FRAME)
				{
				  	eCanRxState = eCAN_MAX_STATE;
				}
				else g_ucCanExceptState = pInCanPacket->stNormalPacket.arrDataFields[0];
			}
		  	else if((g_ucCanExceptState & 0xF0) == CAN_CONSECUTICE_FRAME)
			{
				if(g_ucCanExceptState == 0x2F)	g_ucCanExceptState = 0x20;
				else 							g_ucCanExceptState++;
			  	
				if(pInCanPacket->stNormalPacket.arrDataFields[0] != g_ucCanExceptState)  eCanRxState = eCAN_MAX_STATE;
			}
		}
	}
	//pReadMsg->DataSize = 0;		//rx 초기화
	switch ( eCanRxState )
	{
		case eCAN_RX_SINGLE_FRAME:
#if defined(DEBUG_CAN_LOG)
			GLogI("[%s] eCAN_RX_SINGLE_FRAME\r\n", __FUNCTION__);
#endif
			pReadMsg->DataSize = 0;		//rx 초기화
			switch (bIsStandardCan) 
			{
				case eCAN_CLASSIC_STANDARD:
					pendingStatus = ((pInCanPacket->stNormalPacket.arrDataFields[1] == 0x7F) &&
								   (pInCanPacket->stNormalPacket.arrDataFields[3] == 0x78));
					break;

				case eCAN_CLASSIC_EXTENDED:
					pendingStatus = ((pInCanPacket->stExtendPacket.arrDataFields[1] == 0x7F) &&
								   (pInCanPacket->stExtendPacket.arrDataFields[3] == 0x78));
					break;

				case eCAN_FDFORMAT_STANDARD:
					pendingStatus = ((pInCanPacket->stFDStdPacket.arrDataFields[1] == 0x7F) &&
								   (pInCanPacket->stFDStdPacket.arrDataFields[3] == 0x78));
					break;

				case eCAN_FDFORMAT_EXTENDED:
					pendingStatus = ((pInCanPacket->stFDExtPacket.arrDataFields[1] == 0x7F) &&
								   (pInCanPacket->stFDExtPacket.arrDataFields[3] == 0x78));
					break;

				default:
					pendingStatus = false;
					break;
			}
			if(pendingStatus == true)
			{
				if( (uiProtocolID == ISO15765_NORMAL)||
					(uiProtocolID == ISO15765_CAN_HWSET_DB)||
					(uiProtocolID == ISO14229_ES95486_02_100)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_102)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_103)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_104)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_105)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_106)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_107)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_108)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_109)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_10A)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_10B)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_10C)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_10D)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_10E)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_10F)||				//20190918 Jay
					(uiProtocolID == ISO14229_ES95486_02_100_NEW)||		//20190918 Jay
#ifdef HOTA
                    (uiProtocolID == ISO14229_ES95486_02_HOTA)||
#endif
                      
#ifdef CANFD_PROTOCOL
                    (uiProtocolID == ISO14229_ES95486_02_130_CANFD)||
                    (uiProtocolID == ISO14229_ES95486_02_131_CANFD)||
#endif
					(uiProtocolID == ISO14230_ES95486_DOIP_120)||
					(uiProtocolID == ISO14230_ES95486_DOIP_121)||
					(uiProtocolID == ISO14230_ES95486_DOIP_122)||
					(uiProtocolID == ISO14230_ES95486_DOIP_123)||
					(uiProtocolID == ISO14230_ES95486_DOIP_124)||
					(uiProtocolID == ISO14230_ES95486_DOIP_125)||
					(uiProtocolID == ISO14230_ES95486_DOIP_126)||
					(uiProtocolID == ISO14230_ES95486_DOIP_127)||
					(uiProtocolID == ISO14230_ES95486_DOIP_128)||
					(uiProtocolID == ISO14230_ES95486_DOIP_129)||
					(uiProtocolID == ISO14230_ES95486_DOIP_12A)||
					(uiProtocolID == ISO14230_ES95486_DOIP_12B)||
					(uiProtocolID == ISO14230_ES95486_DOIP_12C)||
					(uiProtocolID == ISO14230_ES95486_DOIP_12D)||
					(uiProtocolID == ISO14230_ES95486_DOIP_12E)||
					(uiProtocolID == ISO14230_ES95486_DOIP_12F)||
                    (uiProtocolID == ISO14229_ES95486_170)||
                    (uiProtocolID == ISO14229_ES95486_170_MMCAN)||
                    (uiProtocolID == ISO14230_ES95486_170)||
                    (uiProtocolID == ISO14230_ES95486_170_MMCAN)||
					(uiProtocolID == ISO15765_CAN_HWSET_DB_NEW)
#ifdef NEW_29BIT_CAN
                    || (uiProtocolID == ISO15765_ES95486_29bit)
                    || (uiProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
                    || (uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
                    || (uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
                      )
				{
					g_bAckflag=0;//No ACK tx while pending
					g_bCanRxPendingFrame = TRUE;
					g_bCanRxConcequtiveFrame = FALSE;
					if( g_bRcvMultiFrame == true ) 
					  	g_bRcvMultiFrame = false;
					eCanRxState = CAN_MakeRxSingleFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
				}
				else 
				{
					g_bAckflag=0;//No ACK tx while pending
					g_bCanRxPendingFrame = TRUE;
					eCanRxState = eCAN_RX_BLOCK;	//g_ulCanWrittenTick = OemGetTmr();
					
					U8 TxBuff[10], ch=1;
					if(uiProtocolID == ISO15765_REPRO_PENNIMG)
					{
						TxBuff[0]=0x07;
						TxBuff[1]=0xDF;
						TxBuff[2]=0x08;//dlc
						TxBuff[3]=0x02;
						TxBuff[4]=0x3E;
						TxBuff[5]=0x80;
						TxBuff[6]=0x00;
						TxBuff[7]=0x00;
						TxBuff[8]=0x00;
						TxBuff[9]=0x00;
						OemWriteCanBuff1(TxBuff, ch);
					}
				}
				if( g_stGITSetConfig.nEtc3 == 1 || g_stGITSetConfig.nEtc3 == 0x05 )
				{
				  	if(Get_TmrDelta(Get_Tmr(), uiOldTimer_Timeout) > 3000)
					{
					  	uiOldTimer_Timeout = Get_Tmr();
				  		CAN_TxAckMessage(*pInCanPacket, bIsStandardCan);
					}
				}
			}
			else
			{
				g_bCanRxPendingFrame = FALSE;
				g_bCanRxConcequtiveFrame = FALSE;
				g_bRcvMultiFrame = false;
				eCanRxState = CAN_MakeRxSingleFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
			}
			break;
		case eCAN_RX_FIRST_FRAME:
#if defined(DEBUG_CAN_LOG)
			GLogI("[%s] eCAN_RX_FIRST_FRAME\r\n", __FUNCTION__);
#endif
			pReadMsg->DataSize = 0;		//rx 초기화
			g_bCanRxPendingFrame = FALSE;
			g_bCanRxConcequtiveFrame = TRUE;
			if(((uiProtocolID == ISO15765_NORMAL)||
				(uiProtocolID == ISO15765_CAN_HWSET_DB)||
				(uiProtocolID == ISO14229_ES95486_02_100)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_102)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_103)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_104)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_105)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_106)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_107)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_108)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_109)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_10A)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_10B)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_10C)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_10D)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_10E)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_10F)||				//20190918 Jay
				(uiProtocolID == ISO14229_ES95486_02_100_NEW)||		//20190918 Jay
				(uiProtocolID == ISO14230_ES95486_DOIP_120)||
				(uiProtocolID == ISO14230_ES95486_DOIP_121)||
				(uiProtocolID == ISO14230_ES95486_DOIP_122)||
				(uiProtocolID == ISO14230_ES95486_DOIP_123)||
				(uiProtocolID == ISO14230_ES95486_DOIP_124)||
				(uiProtocolID == ISO14230_ES95486_DOIP_125)||
				(uiProtocolID == ISO14230_ES95486_DOIP_126)||
				(uiProtocolID == ISO14230_ES95486_DOIP_127)||
				(uiProtocolID == ISO14230_ES95486_DOIP_128)||
				(uiProtocolID == ISO14230_ES95486_DOIP_129)||
				(uiProtocolID == ISO14230_ES95486_DOIP_12A)||
				(uiProtocolID == ISO14230_ES95486_DOIP_12B)||
				(uiProtocolID == ISO14230_ES95486_DOIP_12C)||
				(uiProtocolID == ISO14230_ES95486_DOIP_12D)||
				(uiProtocolID == ISO14230_ES95486_DOIP_12E)||
				(uiProtocolID == ISO14230_ES95486_DOIP_12F)||
				(uiProtocolID == ISO14229_ES95486_170)||
				(uiProtocolID == ISO14229_ES95486_170_MMCAN)||
				(uiProtocolID == ISO14230_ES95486_170)||
				(uiProtocolID == ISO14230_ES95486_170_MMCAN)||
				(uiProtocolID == ISO15765_CAN_HWSET_DB_NEW)	||
				(uiProtocolID == ISO15765_CARB_29BIT)||
				(uiProtocolID == ISO15765_CARB_29BIT_NEW)||
                (uiProtocolID == J1939_23_CARB_29BIT_NEW)  ||
				(uiProtocolID == ISO15765_29BIT)||
				(uiProtocolID == ISO15765_29BIT_EXCEPT)||
				(uiProtocolID == ISO15765_29BIT_REPRO_PENNIMG_TIME)
#ifdef NEW_29BIT_CAN
				|| (uiProtocolID == ISO15765_ES95486_29bit)
				|| (uiProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
                || (uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
                || (uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
				))
			{
			 	g_bRcvMultiFrame = true;
			 	g_uiRecvMultiFrameOldtime = Get_Tmr();
			}
			eCanRxState = CAN_MakeRxFirstFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
			break;
		case eCAN_RX_CONSECUTIVE_FRAME:
#if defined(DEBUG_CAN_LOG)
			GLogI("[%s] eCAN_RX_CONSECUTIVE_FRAME\r\n", __FUNCTION__);
#endif
			g_bCanRxConcequtiveFrame = TRUE;
			eCanRxState = CAN_MakeRxConsecutiveFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
			break;
		case eCAN_RX_FLOWCONTROL_FRAME:
#if defined(DEBUG_CAN_LOG)
			GLogI("[%s] eCAN_RX_FLOWCONTROL_FRAME\r\n", __FUNCTION__);
#endif
			if(g_bCF_TxComplete==1)
			{
				//intTxdRxdCount = 5000;
				g_bCF_TxComplete =0;
				eCanRxState = eCAN_RX_BLOCK;
				break;
			}
			g_bCanRxPendingFrame = FALSE;
			g_bCanRxConcequtiveFrame = FALSE;
			g_bRcvMultiFrame = false;
			eCanRxState = CAN_MakeRxFlowControlFrame(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
			break;
		case eCAN_RX_SINGLE_FRAME_EX:
		  	g_bCanRxPendingFrame = FALSE;
			g_bCanRxConcequtiveFrame = FALSE;
			g_bRcvMultiFrame = false;
			eCanRxState = CAN_MakeRxSingleFrame_EX(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
			break;
		case eCAN_RX_FIRST_FRAME_EX:
			break;
		case eCAN_RX_CONSECUTIVE_FRAME_EX:
			break;
		case eCAN_RX_FLOWCONTROL_FRAME_EX:
			break;
			
		default:
			eCanRxState = eCAN_NONE_STATE;
			clearRXCanMessage();
			break;
	}
	return eCanRxState;
}

eDiagCanState CAN_RxBlockProc_EX(stCanPacket *pInCanPacket, U8 bIsStandardCan, stPASSTHRU_MSG *pReadMsg)
{
	eDiagCanState eCanRxState = eCAN_NONE_STATE;
	unsigned long uiProtocolID;
	//static int uiOldTimer_Timeout=0;
	bool bResult = FALSE;
	
	bResult = CAN_FindCanPacket_EX(pInCanPacket, (eCanType*)&bIsStandardCan);
	
	if( bResult == TRUE )
	{
		uiProtocolID = VCI_GetPassThruProtocolID();

		if(  uiProtocolID == J1939 || uiProtocolID == J1939_4PGN )
		{
		  	eCanRxState = eCAN_RX_SINGLE_FRAME_EX; 
		}
		else
		{
			eCanRxState = (eDiagCanState)( (pInCanPacket->stExtendPacket.arrDataFields[0] >> 4 & 0x0F) + eCAN_RX_SINGLE_FRAME);
		}
		//pReadMsg->DataSize = 0;		//rx 초기화
		switch ( eCanRxState )
		{
			case eCAN_RX_SINGLE_FRAME_EX:
			  	g_bCanRxPendingFrame = FALSE;
				g_bCanRxConcequtiveFrame = FALSE;
				CAN_MakeRxSingleFrame_EX(pInCanPacket, bIsStandardCan, eCanRxState, pReadMsg);
				break;
			case eCAN_RX_FIRST_FRAME_EX:
				break;
			case eCAN_RX_CONSECUTIVE_FRAME_EX:
				break;
			case eCAN_RX_FLOWCONTROL_FRAME_EX:
				break;
				
			default:
				eCanRxState = eCAN_NONE_STATE;
				clearRXCanMessage();
				break;
		}
	}
	
	eCanRxState = CAN_J1939_FuncProc( pInCanPacket, bIsStandardCan, pReadMsg, bResult);
	
	return eCanRxState;
}

eDiagCanState CAN_MakeRxSingleFrame(stCanPacket *pInCanPacket, U8 bIsStandardCan, eDiagCanState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
	unsigned int uiProtocolID, uiCopyLen, uiCopyIndex, uiCanIDLen, uiArrayIndex = 0;
	unsigned char *pCanData;
	eCanRxState = eCAN_NONE_STATE;

#if defined(DEBUG_CAN_LOG)
	GLogI("[%s] run\r\n", __FUNCTION__);
#endif

	uiProtocolID = VCI_GetPassThruProtocolID();

	if(uiProtocolID==ISO15765_ACU_SINGLE
		||uiProtocolID==ISO15765_SMK) {}
	else CAN_AcquireArrayIndexFromCarbCanID(pInCanPacket, bIsStandardCan, &uiArrayIndex);
	
	if ( bIsStandardCan == eCAN_CLASSIC_STANDARD )
	{
		if(uiProtocolID == ISO15765_ACU_SINGLE)
		{
			if(g_ucVehicle_Current_Read==2)
			{
				g_uiCanReadMsgLength=0;
			}
			else
			{
				uiArrayIndex=(uiArrayIndex+g_uiCanReadMsgLength);
			}
		}
		uiCanIDLen = 2;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID >> 8;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID;

		if( (uiProtocolID == ISO15765_SINGLE) || 
			(uiProtocolID == ISO15765_SINGLE_SMK) || 
			(uiProtocolID == ISO15765_SINGLE_PODS)||
			(uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE))
		{
			uiCopyLen = 8;
			uiCopyIndex = 0;
			CAN_mDelay(3);
		}
        else if(uiProtocolID == ISO15765_ACU_SINGLE)    // 20150810 seo 0x2B 추가
        {
			uiCopyLen = 8;
			uiCopyIndex = 0;
		}
		else if ( uiProtocolID == ISO15765_CUBIS)
		{
			uiCopyLen = 7;
			uiCopyIndex = 1;
		}
		else if ( uiProtocolID == ISO15765_SMK)
		{
			uiCopyLen = (pInCanPacket->stNormalPacket.arrDataFields[1])*0x100+pInCanPacket->stNormalPacket.arrDataFields[0];
			uiCopyLen += 2;//include length size. kkt
			uiCopyIndex = 0;
		}
		else
		{
			uiCopyLen = pInCanPacket->stNormalPacket.arrDataFields[0]&0x0F;
			uiCopyIndex = 1;
		}
		pCanData = &pInCanPacket->stNormalPacket.arrDataFields[uiCopyIndex];
	}
#ifdef CANFD_qhyek //Q_hyek CANFD
	else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD )
	{
		uiCanIDLen = 2;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stFDStdPacket.us11BitID >> 8;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stFDStdPacket.us11BitID;

		if( (uiProtocolID == ISO15765_SINGLE) || 
			(uiProtocolID == ISO15765_SINGLE_SMK) || 
			(uiProtocolID == ISO15765_SINGLE_PODS)||
			(uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE))
		{
			uiCopyLen = 8;
			uiCopyIndex = 0;
			CAN_mDelay(3);
		}
        else if(uiProtocolID == ISO15765_ACU_SINGLE)    // 20150810 seo 0x2B 추가
        {
			uiCopyLen = 8;
			uiCopyIndex = 0;
		}
		else if ( uiProtocolID == ISO15765_CUBIS)
		{
			uiCopyLen = 7;
			uiCopyIndex = 1;
		}
		else
		{
			uiCopyLen = pInCanPacket->stFDStdPacket.arrDataFields[0]&0x0F;
			uiCopyIndex = 1;
		}
		pCanData = &pInCanPacket->stFDStdPacket.arrDataFields[uiCopyIndex];
				
	}
#endif
	else
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
			(uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE))
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
	
	if( uiProtocolID == ISO15765_CARB_NEW_LENGTH ||
        uiProtocolID == J1939_23_CARB_NEW_LENGTH ||
        uiProtocolID == J1939_23_CARB_29BIT_NEW  ||
	   	uiProtocolID == ISO15765_CARB_29BIT_NEW	)
	{
	  	pReadMsg->pData[uiArrayIndex++] = 0;
		if( uiProtocolID == ISO15765_CARB_NEW_LENGTH || uiProtocolID == J1939_23_CARB_NEW_LENGTH)
	  		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.arrDataFields[0]&0x0F;
		else
		 	pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stExtendPacket.arrDataFields[0]&0x0F;
		g_uiCanReadMsgLength += CARB_LENGTH_LEN;
		pReadMsg->DataSize += CARB_LENGTH_LEN;
	}
	
	memcpy(pReadMsg->pData+uiArrayIndex, pCanData, uiCopyLen);
	pReadMsg->DataSize += uiCopyLen;
	
	g_uiCanReadMsgLength += uiCanIDLen;
	g_uiCanReadMsgLength += uiCopyLen;
	
	CAN_SaveCarbCanRecvInfo(pInCanPacket, bIsStandardCan, uiCanIDLen, uiCopyLen, uiCopyLen);

	if( uiProtocolID == ISO15765_CARB 				|| 
	   	uiProtocolID == ISO15765_CARB_NEW 			||
	   	uiProtocolID == ISO15765_CARB_NEW_LENGTH 	||
        uiProtocolID == J1939_23_CARB_NEW_LENGTH    ||
	   	uiProtocolID == ISO15765_CUBIS 				||
        uiProtocolID == ISO15765_CARB_29BIT_NEW     ||
        uiProtocolID == J1939_23_CARB_29BIT_NEW     ||
	   	uiProtocolID == ISO15765_ACU_SINGLE) 
	{
		g_bCanRxCarbFrame = TRUE;
		if(g_ucVehicle_Current_Read==2)
		  	if(g_bCanRxPendingFrame == true)	eCanRxState = eCAN_RX_PENDING;
			else								eCanRxState = eCAN_RX_COMPLETE;
			
		else
			eCanRxState = eCAN_RX_BLOCK; //g_ulCanWrittenTick = OemGetTmr();
		
	}
	else
	{
		if ( pReadMsg->DataSize > 0 )
		{
			CAN_ClearCarbCanRecvInfo();
			if(g_bCanRxPendingFrame == true)	eCanRxState = eCAN_RX_PENDING;
			else								eCanRxState = eCAN_RX_COMPLETE;	//PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType);
		}
		g_bCanRxCarbFrame = FALSE;
		g_uiCanReadMsgLength = 0;
	}

#if defined(DEBUG_CAN_LOG)
	GLogI("[%s]ulDataIndex %d, g_uiCanReadMsgLength %d, ucDLC %d\r\n", 
					__FUNCTION__, uiCopyIndex, uiCanIDLen);
#endif

	return eCanRxState;
}

eDiagCanState CAN_MakeRxFirstFrame(stCanPacket *pInCanPacket, U8 bIsStandardCan, eDiagCanState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
	unsigned int uiProtocolID, uiCopyLen, uiCopyIndex, uiCanIDLen, uiArrayIndex=0, uiPacketTotalLen;
	unsigned char *pCanData;
	eCanRxState = eCAN_NONE_STATE;
	
#if defined(DEBUG_CAN_LOG)
	GLogN("[%s] run\r\n", __FUNCTION__);
#endif
	
	uiProtocolID = VCI_GetPassThruProtocolID();

	g_uiCanRxConsFrameNo = 0;

	CAN_AcquireArrayIndexFromCarbCanID(pInCanPacket, bIsStandardCan, &uiArrayIndex);

	if ( bIsStandardCan == eCAN_CLASSIC_STANDARD )
	{
		uiCanIDLen = 2;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID >> 8;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.us11BitID;

		uiPacketTotalLen = ((pInCanPacket->stNormalPacket.arrDataFields[0] & 0x0F)<<8) + pInCanPacket->stNormalPacket.arrDataFields[1];
		uiCopyLen = 6;
		uiCopyIndex = 2;
		eCanRxState = eCAN_TX_FLOWCONTROL_FRAME;

		if(uiProtocolID==ISO15765_SMK)
		{
			uiPacketTotalLen = (pInCanPacket->stNormalPacket.arrDataFields[1])*0x100+pInCanPacket->stNormalPacket.arrDataFields[0];
			uiCopyIndex = 0;
			uiCopyLen = 8;
		}
		pCanData = &pInCanPacket->stNormalPacket.arrDataFields[uiCopyIndex];
	}
#ifdef CANFD_qhyek //Q_hyek CANFD
	else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD)
	{
		uiCanIDLen = 2;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stFDStdPacket.us11BitID >> 8;
		pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stFDStdPacket.us11BitID;

		uiPacketTotalLen = ((pInCanPacket->stFDStdPacket.arrDataFields[0] & 0x0F)<<8) + pInCanPacket->stFDStdPacket.arrDataFields[1];
		uiCopyLen = 6;
		uiCopyIndex = 2;
		eCanRxState = eCAN_TX_FLOWCONTROL_FRAME;
		
		pCanData = &pInCanPacket->stFDStdPacket.arrDataFields[uiCopyIndex];
	}
#endif
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
			// ASSAGAORY : 확인해 봐야 한다.
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

		eCanRxState = eCAN_TX_FLOWCONTROL_FRAME;
		
		pCanData = &pInCanPacket->stExtendPacket.arrDataFields[uiCopyIndex];
	}

	// copy data
	pReadMsg->DataSize += uiCanIDLen;
	
	if( uiProtocolID == ISO15765_CARB_NEW_LENGTH ||
        uiProtocolID == J1939_23_CARB_NEW_LENGTH ||
        uiProtocolID == J1939_23_CARB_29BIT_NEW  ||
	   	uiProtocolID == ISO15765_CARB_29BIT_NEW	)
	{
	  	if( uiProtocolID == ISO15765_CARB_NEW_LENGTH || uiProtocolID == J1939_23_CARB_NEW_LENGTH )
		{
	  		pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stNormalPacket.arrDataFields[0] & 0x0F)<<8;
			pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stNormalPacket.arrDataFields[1];
		}
		else
		{
		 	pReadMsg->pData[uiArrayIndex++] = (pInCanPacket->stExtendPacket.arrDataFields[0] & 0x0F)<<8;
			pReadMsg->pData[uiArrayIndex++] = pInCanPacket->stExtendPacket.arrDataFields[1];
		}
		g_uiCanReadMsgLength += CARB_LENGTH_LEN;
		pReadMsg->DataSize += CARB_LENGTH_LEN;
	}
	
	memcpy(pReadMsg->pData+uiArrayIndex, pCanData, uiCopyLen);
	pReadMsg->DataSize += uiCopyLen;

	g_uiCanReadMsgLength += uiCanIDLen;
	g_uiCanReadMsgLength += uiPacketTotalLen;

	CAN_SaveCarbCanRecvInfo(pInCanPacket, bIsStandardCan, uiCanIDLen, uiCopyLen, uiPacketTotalLen);

	if(g_ulProtocolID == ISO15765_SMK  )
	{
		g_uiCanReadMsgLength += 2;//include length size
		//CurMode = CAN_RX_BLOCK;
		eCanRxState = eCAN_RX_BLOCK;
		g_ucCan_FC = 1;
	}
	
#if defined(DEBUG_CAN_LOG)
	GLogI("[%s] g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", __FUNCTION__, g_uiCanReadMsgLength, pReadMsg->DataSize);
    GLogI("%02X ", pInCanPacket->stNormalPacket.us11BitID);
    for ( int jj=0; jj<pInCanPacket->stNormalPacket.ucDLC; jj++ )
        GLogI("%02X ", pInCanPacket->stNormalPacket.arrDataFields[jj]);
    GLogI("\r\n");
#endif
	return eCanRxState;
}

eDiagCanState CAN_MakeRxFlowControlFrame(stCanPacket *pInCanPacket, U8 bIsStandardCan, eDiagCanState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
	unsigned char *pTmpCanPacket;
	eCanRxState = eCAN_NONE_STATE;
    unsigned int uiProtocolID;

    uiProtocolID = VCI_GetPassThruProtocolID();

#if defined(DEBUG_CAN_LOG)
	GLogN("[%s] run\r\n", __FUNCTION__);
#endif
    g_uiCanTxConsFrameNo = 0;
    
	if ( bIsStandardCan == eCAN_CLASSIC_STANDARD )
	{
		pTmpCanPacket = pInCanPacket->stNormalPacket.arrDataFields;

        if( (pTmpCanPacket[0]&0x0F) == 0 )
        {
            g_stECUSetConfig.nBSTx = pTmpCanPacket[1];
            g_stECUSetConfig.nSTMinTx = pTmpCanPacket[2];
                
            eCanRxState = eCAN_TX_CONSECUTIVE_FRAME;
        }
        else
        {
            eCanRxState = eCAN_RX_BLOCK;
            g_bCanRxPendingFrame = TRUE;
        }

        return eCanRxState;
	}
#ifdef CANFD_qhyek //Q_hyek CANFD
	else if( bIsStandardCan == eCAN_FDFORMAT_STANDARD )
	{
		pTmpCanPacket = pInCanPacket->stFDStdPacket.arrDataFields;
        
        if( (pTmpCanPacket[0]&0x0F) == 0 )
        {
            g_stECUSetConfig.nBSTx = pTmpCanPacket[1];
            g_stECUSetConfig.nSTMinTx = pTmpCanPacket[2];
                
            eCanRxState = eCAN_TX_CONSECUTIVE_FRAME;
        }
        else
        {
            eCanRxState = eCAN_RX_BLOCK;
            g_bCanRxPendingFrame = TRUE;
        }

        return eCanRxState;
	}
#endif
	else /* eCAN_CLASSIC_EXTENDED */
	{
		pTmpCanPacket = pInCanPacket->stExtendPacket.arrDataFields;
        
        if( (pTmpCanPacket[0]&0x0F) == 0 )
    	{
            if( uiProtocolID == ISO15765_ES95486_29bit || uiProtocolID == ISO15765_ES95486_DOIP_29BIT 
#ifdef CANFD_PROTOCOL
                || (uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
                || (uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
                || (uiProtocolID == ISO15765_29BIT)
              )
            {
                g_stECUSetConfig.nBSTx = pTmpCanPacket[1];
    		    g_stECUSetConfig.nSTMinTx = pTmpCanPacket[2];
            }
            else if( ( g_stGITSetConfig.nEtc3 & 0x0400 ) == 0x0400 ) /* CV_Except */
            {
                g_stECUSetConfig.nBSTx = pTmpCanPacket[1];
                g_stECUSetConfig.nSTMinTx = g_stGITSetConfig.nSTMinTx;
            }
            else
            {
                g_stECUSetConfig.nBSTx = 0x00;
                g_stECUSetConfig.nSTMinTx = pTmpCanPacket[2];
            }

            eCanRxState = eCAN_TX_CONSECUTIVE_FRAME;
    	}
    	else
    	{
    		eCanRxState = eCAN_RX_BLOCK;
    		g_bCanRxPendingFrame = TRUE;
    	}

        return eCanRxState;
	}

    GLogE("Error %s\r\n", __FUNCTION__);
    return eCanRxState;
}

eDiagCanState CAN_MakeRxConsecutiveFrame(stCanPacket *pInCanPacket, U8 bIsStandardCan, eDiagCanState eCanRxState, stPASSTHRU_MSG *pReadMsg)
{
	eCanRxState = eCAN_NONE_STATE;
#if defined(DEBUG_CAN_LOG)
	GLogN("\r\n[%s] run\r\n", __FUNCTION__);
#endif
	unsigned char *pTmp, ucDLC;
	unsigned int uiArrayIndex = 0, uiOrder, uiCurCanRecvLength;
	unsigned int uiRemainLen,uiProtocolID;
	
	uiProtocolID = VCI_GetPassThruProtocolID();

	uiOrder = CAN_AcquireArrayIndexFromCarbCanID(pInCanPacket, bIsStandardCan, &uiArrayIndex);

	if ( bIsStandardCan )	
	{
		ucDLC = pInCanPacket->stNormalPacket.ucDLC;
		pTmp = pInCanPacket->stNormalPacket.arrDataFields+1;
		if(g_ulProtocolID == ISO15765_SMK  )
		{
			pTmp = pInCanPacket->stNormalPacket.arrDataFields;
		}
	}
	else
	{
		ucDLC = pInCanPacket->stExtendPacket.ucDLC;
		pTmp = pInCanPacket->stExtendPacket.arrDataFields+1;
	}
    
#ifdef CANFD_qhyek //Q_hyek CANFD
	if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD )
	{
		ucDLC = pInCanPacket->stFDStdPacket.ucDLC;
		pTmp = pInCanPacket->stFDStdPacket.arrDataFields+1;
	}
#endif
	
	if ( ucDLC > 0 )
	{
		uiRemainLen = CAN_GetCarbTotalPacketLength(uiOrder) - CAN_GetCarbRecvedLength(uiOrder);
		if(g_ulProtocolID == ISO15765_SMK  )
		{
			if ( uiRemainLen > (CAN_FRAME_DATA_SIZE) )	uiCurCanRecvLength = CAN_FRAME_DATA_SIZE;
			else										uiCurCanRecvLength  = ucDLC;
		}
		else
		{
			if ( uiRemainLen > (CAN_FRAME_DATA_SIZE-1) )	uiCurCanRecvLength = CAN_FRAME_DATA_SIZE-1;
			else											uiCurCanRecvLength  = uiRemainLen;
		}
		memcpy(pReadMsg->pData+uiArrayIndex, pTmp, uiCurCanRecvLength);
	
		pReadMsg->DataSize += uiCurCanRecvLength;
		CAN_SaveCarbCanRecvLength(uiOrder, uiCurCanRecvLength);

#if defined(DEBUG_CAN_LOG)
        if ( bIsStandardCan == eCAN_CLASSIC_STANDARD )
        {GLogI("[%s] DLC %d, g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", __FUNCTION__, pInCanPacket->stNormalPacket.ucDLC, g_uiCanReadMsgLength, pReadMsg->DataSize);}
#ifdef CANFD_qhyek//Q_hyek CANFD
		else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD )
			{GLogI("[%s] DLC %d, g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", __FUNCTION__, pInCanPacket->stFDStdPacket.ucDLC, g_uiCanReadMsgLength, pReadMsg->DataSize);}
#endif
#endif

		if( uiProtocolID == ISO15765_CARB 				|| 
			uiProtocolID == ISO15765_CARB_NEW 			||
			uiProtocolID == ISO15765_CARB_NEW_LENGTH 	||
            uiProtocolID == J1939_23_CARB_NEW_LENGTH    ||
			uiProtocolID == ISO15765_CARB_29BIT 		||
            uiProtocolID == J1939_23_CARB_29BIT_NEW		||
			uiProtocolID == ISO15765_CARB_29BIT_NEW 	)
		{
			g_bCanRxCarbFrame = TRUE;
			eCanRxState = eCAN_RX_BLOCK;		//g_ulCanWrittenTick = OemGetTmr();		// carb 들어온 시점부터 타이머 재가동
		}
		else
		{
			if ( g_uiCanReadMsgLength == pReadMsg->DataSize )
			{
				CAN_ClearCarbCanRecvInfo();

				// PC로전달.
				eCanRxState  = eCAN_RX_COMPLETE;		//PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType);
				//g_ulCanWrittenTick = 0;
				g_uiCanReadMsgLength = 0;
				g_bRcvMultiFrame = false;
			}
			else if ( g_uiCanReadMsgLength < pReadMsg->DataSize )
			{
				GLogE(" DLC %d, g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", pInCanPacket->stNormalPacket.ucDLC, g_uiCanReadMsgLength, pReadMsg->DataSize);
                
#ifdef CANFD_qhyek //Q_hyek CANFD
				if ( bIsStandardCan == eCAN_CLASSIC_STANDARD )
					{GLogE(" DLC %d, g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", pInCanPacket->stNormalPacket.ucDLC, g_uiCanReadMsgLength, pReadMsg->DataSize);}
				else if ( bIsStandardCan == eCAN_FDFORMAT_STANDARD )
					{GLogE(" DLC %d, g_uiCanReadMsgLength %d, pReadMsg->DataSize %d\r\n", pInCanPacket->stFDStdPacket.ucDLC, g_uiCanReadMsgLength, pReadMsg->DataSize);}
#endif

				clearRXCanMessage();
				eCanRxState = eCAN_RX_BLOCK;		//g_ulCanWrittenTick = OemGetTmr();
			}
			else
			{
				eCanRxState = eCAN_RX_BLOCK;		//g_ulCanWrittenTick = OemGetTmr();
			}
		}
	}
	else
	{
#if defined(DEBUG_CAN_LOG)
	GLogE("DLC is zero ~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
#endif
		return eCanRxState;
	}
	if(eCanRxState  != eCAN_RX_COMPLETE)//When data reception is complete, 'flow control' is not transmitted
	{
        if( g_stGITSetConfig.nBSTx == 0xFFFF )
        {
            if ( (++g_uiCanRxConsFrameNo % 8) == 0 )
            {
                eCanRxState = eCAN_TX_FLOWCONTROL_FRAME;
            }
        }
		/* If Block size zero(30 00), don't need to send FlowControl. because zero division in below logic */
		else if (g_stGITSetConfig.nBSTx == 0) {}
        else
        {
            if ( (++g_uiCanRxConsFrameNo % g_stGITSetConfig.nBSTx) == 0 )
            {
                eCanRxState = eCAN_TX_FLOWCONTROL_FRAME;
            }
        }
	}
	return eCanRxState;
}

void VCI_PeriodicMessage0(void)
{
	stFdcanPkt	*CANpacket;
	stMsgClst	*CANmessage;
	
	uint32_t CanID;
	
	CANmessage	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( CANmessage == NULL )
	{
		GLogE( "Error... fail alloc message!!!\r\n" );
		return;
	}

	CANpacket	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
	if( CANpacket == NULL )
	{
		osPoolFree( hMsgPool, (void *)CANmessage );
		return;
	}
	
#ifdef USE_INTERNAL_CAN_ONLY
	CANpacket->pTarget		= &hfdcan1;
#else
	CANpacket->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
	if( (stPeriodicMsgInfo[0].ucMsg[0]&0x80) != 0x80 )
	{
	  	CanID = (stPeriodicMsgInfo[0].ucMsg[1]<<8) + stPeriodicMsgInfo[0].ucMsg[2];
		CANpacket->mLen = stPeriodicMsgInfo[0].ucMsg[0];
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[0].ucMsg[3], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_STANDARD, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	else
	{
	  	CanID = (stPeriodicMsgInfo[0].ucMsg[1]<<24) + (stPeriodicMsgInfo[0].ucMsg[2]<<16) + (stPeriodicMsgInfo[0].ucMsg[3]<<8) + (stPeriodicMsgInfo[0].ucMsg[4]);
		CANpacket->mLen = (stPeriodicMsgInfo[0].ucMsg[0] - 0x80);
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[0].ucMsg[5], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_EXTENDED, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	
	GLogI( "Periodic0 : %04X\r\n", CanID);
	
	CANmessage->mMsgType = MSG_PERIODIC;
	CANmessage->pPacket = (void *)CANpacket;
    
    g_bSendPeriodicFlag = true;
	g_uiSendPeriodicOldTime = Get_Tmr();

	if(xQueueIsQueueFullFromISR(hFDTxMsg) == TRUE)
	{
		osPoolFree( hFdcanPktPool, (void *)CANpacket );
		osPoolFree( hMsgPool, (void *)CANmessage );
	}
	else
	{
		osMessagePut( hFDTxMsg, (uint32_t)CANmessage, osWaitForever );
	}
}

void VCI_PeriodicMessage1(void)
{
	stFdcanPkt	*CANpacket;
	stMsgClst	*CANmessage;
	
	uint32_t CanID;
	
	CANmessage	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( CANmessage == NULL )
	{
		GLogE( "Error... fail alloc message!!!\r\n" );
		return;
	}

	CANpacket	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
	if( CANpacket == NULL )
	{
		osPoolFree( hMsgPool, (void *)CANmessage );
		return;
	}
	
#ifdef USE_INTERNAL_CAN_ONLY
	CANpacket->pTarget		= &hfdcan1;
#else
	CANpacket->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
	
	if( (stPeriodicMsgInfo[1].ucMsg[0]&0x80) != 0x80 )
	{
	  	CanID = (stPeriodicMsgInfo[1].ucMsg[1]<<8) + stPeriodicMsgInfo[1].ucMsg[2];
		CANpacket->mLen = stPeriodicMsgInfo[1].ucMsg[0];
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[1].ucMsg[3], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_STANDARD, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	else
	{
	  	CanID = (stPeriodicMsgInfo[1].ucMsg[1]<<24) + (stPeriodicMsgInfo[1].ucMsg[2]<<16) + (stPeriodicMsgInfo[1].ucMsg[3]<<8) + (stPeriodicMsgInfo[1].ucMsg[4]);
		CANpacket->mLen = (stPeriodicMsgInfo[1].ucMsg[0] - 0x80);
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[1].ucMsg[5], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_EXTENDED, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	
	GLogI( "Periodic1 : %04X\r\n", CanID);

	CANmessage->mMsgType = MSG_PERIODIC;
	CANmessage->pPacket = (void *)CANpacket;
    
    g_bSendPeriodicFlag = true;
	g_uiSendPeriodicOldTime = Get_Tmr();

	if(xQueueIsQueueFullFromISR(hFDTxMsg) == TRUE)
	{
		osPoolFree( hFdcanPktPool, (void *)CANpacket );
		osPoolFree( hMsgPool, (void *)CANmessage );
	}
	else
	{
		osMessagePut( hFDTxMsg, (uint32_t)CANmessage, osWaitForever );
	}
}
void VCI_PeriodicMessage2(void)
{
	stFdcanPkt	*CANpacket;
	stMsgClst	*CANmessage;
	
	uint32_t CanID;
	
	CANmessage	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( CANmessage == NULL )
	{
		GLogE( "Error... fail alloc message!!!\r\n" );
		return;
	}

	CANpacket	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
	if( CANpacket == NULL )
	{
		osPoolFree( hMsgPool, (void *)CANmessage );
		return;
	}
	
#ifdef USE_INTERNAL_CAN_ONLY
	CANpacket->pTarget		= &hfdcan1;
#else
	CANpacket->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
	
	if( (stPeriodicMsgInfo[2].ucMsg[0]&0x80) != 0x80 )
	{
	  	CanID = (stPeriodicMsgInfo[2].ucMsg[1]<<8) + stPeriodicMsgInfo[2].ucMsg[2];
		CANpacket->mLen = stPeriodicMsgInfo[2].ucMsg[0];
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[2].ucMsg[3], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_STANDARD, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	else
	{
	  	CanID = (stPeriodicMsgInfo[2].ucMsg[1]<<24) + (stPeriodicMsgInfo[2].ucMsg[2]<<16) + (stPeriodicMsgInfo[2].ucMsg[3]<<8) + (stPeriodicMsgInfo[2].ucMsg[4]);
		CANpacket->mLen = (stPeriodicMsgInfo[2].ucMsg[0] - 0x80);
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[2].ucMsg[5], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_EXTENDED, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	
	GLogI( "Periodic2 : %04X\r\n", CanID);

	CANmessage->mMsgType = MSG_PERIODIC;
	CANmessage->pPacket = (void *)CANpacket;
    
    g_bSendPeriodicFlag = true;
	g_uiSendPeriodicOldTime = Get_Tmr();

	if(xQueueIsQueueFullFromISR(hFDTxMsg) == TRUE)
	{
		osPoolFree( hFdcanPktPool, (void *)CANpacket );
		osPoolFree( hMsgPool, (void *)CANmessage );
	}
	else
	{
		osMessagePut( hFDTxMsg, (uint32_t)CANmessage, osWaitForever );
	}
}
void VCI_PeriodicMessage3(void)
{
	stFdcanPkt	*CANpacket;
	stMsgClst	*CANmessage;
	
	uint32_t CanID;
	
	CANmessage	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( CANmessage == NULL )
	{
		GLogE( "Error... fail alloc message!!!\r\n" );
		return;
	}

	CANpacket	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
	if( CANpacket == NULL )
	{
		osPoolFree( hMsgPool, (void *)CANmessage );
		return;
	}
	
#ifdef USE_INTERNAL_CAN_ONLY
	CANpacket->pTarget		= &hfdcan1;
#else
	CANpacket->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
	
	if( (stPeriodicMsgInfo[3].ucMsg[0]&0x80) != 0x80 )
	{
	  	CanID = (stPeriodicMsgInfo[3].ucMsg[1]<<8) + stPeriodicMsgInfo[3].ucMsg[2];
		CANpacket->mLen = stPeriodicMsgInfo[3].ucMsg[0];
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[3].ucMsg[3], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_STANDARD, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	else
	{
	  	CanID = (stPeriodicMsgInfo[3].ucMsg[1]<<24) + (stPeriodicMsgInfo[3].ucMsg[2]<<16) + (stPeriodicMsgInfo[3].ucMsg[3]<<8) + (stPeriodicMsgInfo[3].ucMsg[4]);
		CANpacket->mLen = (stPeriodicMsgInfo[3].ucMsg[0] - 0x80);
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[3].ucMsg[5], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_EXTENDED, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	
	GLogI( "Periodic3 : %04X\r\n", CanID);

	CANmessage->mMsgType = MSG_PERIODIC;
	CANmessage->pPacket = (void *)CANpacket;
    
    g_bSendPeriodicFlag = true;
	g_uiSendPeriodicOldTime = Get_Tmr();

	if(xQueueIsQueueFullFromISR(hFDTxMsg) == TRUE)
	{
		osPoolFree( hFdcanPktPool, (void *)CANpacket );
		osPoolFree( hMsgPool, (void *)CANmessage );
	}
	else
	{
		osMessagePut( hFDTxMsg, (uint32_t)CANmessage, osWaitForever );
	}
}
void VCI_PeriodicMessage4(void)
{
	stFdcanPkt	*CANpacket;
	stMsgClst	*CANmessage;
	
	uint32_t CanID;
	
	CANmessage	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( CANmessage == NULL )
	{
		GLogE( "Error... fail alloc message!!!\r\n" );
		return;
	}

	CANpacket	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
	if( CANpacket == NULL )
	{
		osPoolFree( hMsgPool, (void *)CANmessage );
		return;
	}
	
#ifdef USE_INTERNAL_CAN_ONLY
	CANpacket->pTarget		= &hfdcan1;
#else
	CANpacket->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
	
	if( (stPeriodicMsgInfo[4].ucMsg[0]&0x80) != 0x80 )
	{
	  	CanID = (stPeriodicMsgInfo[4].ucMsg[1]<<8) + stPeriodicMsgInfo[4].ucMsg[2];
		CANpacket->mLen = stPeriodicMsgInfo[4].ucMsg[0];
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[4].ucMsg[3], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_STANDARD, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	else
	{
	  	CanID = (stPeriodicMsgInfo[4].ucMsg[1]<<24) + (stPeriodicMsgInfo[4].ucMsg[2]<<16) + (stPeriodicMsgInfo[4].ucMsg[3]<<8) + (stPeriodicMsgInfo[4].ucMsg[4]);
		CANpacket->mLen = (stPeriodicMsgInfo[4].ucMsg[0] - 0x80);
		memcpy(&CANpacket->mData[0], &stPeriodicMsgInfo[4].ucMsg[5], CANpacket->mLen);
		makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_EXTENDED, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
	}
	
	GLogI( "Periodic4 : %04X\r\n", CanID);

	CANmessage->mMsgType = MSG_PERIODIC;
	CANmessage->pPacket = (void *)CANpacket;
    
    g_bSendPeriodicFlag = true;
	g_uiSendPeriodicOldTime = Get_Tmr();

	if(xQueueIsQueueFullFromISR(hFDTxMsg) == TRUE)
	{
		osPoolFree( hFdcanPktPool, (void *)CANpacket );
		osPoolFree( hMsgPool, (void *)CANmessage );
	}
	else
	{
		osMessagePut( hFDTxMsg, (uint32_t)CANmessage, osWaitForever );
	}
}
void OBDKlineTxThread( void const *argument )
{
	osEvent		evt;
	MsgDiag_t	*message;
	PTmsgPkt_t	*packet;
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;

	for(;;)
	{
		evt	= osMessageGet( hOBDKlineTxMessage, osWaitForever );
		if( evt.status == osEventMessage )
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hOBDKlineTxMessage));
#endif
			message = ( MsgDiag_t * )evt.value.p;
			packet	= ( PTmsgPkt_t* )message->pPacket;

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
			memcpy(pPTpacket, packet, sizeof(PTmsgPkt_t));
			memcpy(pDiagmsg, message, sizeof(MsgDiag_t));
			pDiagmsg->pPacket = pPTpacket;

			osPoolFree( hPTPKPool, (void *)packet );
			osPoolFree( hDiagPool, (void *)message );

			switch ( pDiagmsg->subEvent  )
			{
				//case eCAN_TX_NONE_PARSING:
            case eKLINE_TX_BLOCK :
				{
#if defined(DEBUG_CAN_LOG)
					GLogN("[%s] eKLINE_TX_BLOCK\r\n", __FUNCTION__);
#endif
					unsigned int uiProtocolID = 0;
	
					uiProtocolID = VCI_GetPassThruProtocolID();
					intDlccomCount = g_uiAckTiming; // 20050526 데이터를 ECU로 TX하자마자 ACK타임이 걸려 ACK 소수신 모드로 변환데어 송신한데이터의
													//수신 데이터가 ACK에 포함되어 PC에서 인식 불가
					// [group 1] : 1 2 3 6 7 8_1 13 15_1
					// [group 2] : 8 9 10 11 12 14 15
					
					switch(uiProtocolID)
					{
						case WABCO_ABS:
						{
						  	transmitKL_ByteTime_wabco_abs( GetKlineSelect(), pPTpacket->pData, pPTpacket->DataSize, g_stGITSetConfig.nP4Min);
						  	break;
						}
						case ISO9141_BOSCH:
						{
						  	transmitKL_ByteTime_bosch( GetKlineSelect(), pPTpacket->pData, pPTpacket->DataSize, g_stGITSetConfig.nP4Min);
						  	break;
						}
						default:
						{
						  	transmitKL_ByteTime( GetKlineSelect(), pPTpacket->pData, pPTpacket->DataSize, g_stGITSetConfig.nP4Min);
						  	break;
						}
					}

					if( g_bKLogOnTxFlag == true )
					{
						int ii=0;
						GLogN("Ktx Ch:%d,%d, ",g_stGITHWSetData.nKlineCh,pPTpacket->DataSize);
						for(ii=0; ii<pPTpacket->DataSize; ii++) { GLogN("%02X ",pPTpacket->pData[ii]); }
						GLogN("\r\n");
					}
#if 0
                    uint8_t i;
					GLogN("\r\ntx :");
					for(i=0; i<pPTpacket->DataSize; i++) GLogN(" %02X",pPTpacket->pData[i]);
#endif
#if defined ( SAVE_CAN_LOG )
                    //if( g_bIsFastInit == false )
                    {
                        if ((g_ucCANLog_Enable == 1) && (GetCurFwServiceMode() == eApp_VCI_2))
                        {				  
                            WriteKlLog( pPTpacket->pData, pPTpacket->DataSize );
                        }
                    }
#endif
					//osDelay(1);
                    
                    if(uiProtocolID == ISO14230_POWERTEC)
                    {
                        CAN_uDelay(500); //need to check
                        
                        clearKLReceiveData(KL_LINE1);//rx buffer clear
                        clearKLReceiveData(KL_LINE2);//rx buffer clear
                    }
                    else
                    {
                        osDelay(1);
                        
                        clearKLReceiveData(KL_LINE1);//rx buffer clear
                        clearKLReceiveData(KL_LINE2);//rx buffer clear
                    }

					pDiagmsg->subEvent = eKLINE_RX_BLOCK;

					if(osMessageAvailableSpace(hOBDKlineRxMessage) == 0)
					{
						osPoolFree( hPTPKPool, (void *)pPTpacket );
						osPoolFree( hDiagPool, (void *)pDiagmsg );
					}
					else
					{
						osMessagePut( hOBDKlineRxMessage, (uint32_t)pDiagmsg, osWaitForever );
					}
					break;
				}
				
				default:
					break;
			}
			intDlccomCount = g_uiAckTiming;
		}
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#endif
	}
}
void OBDKlineRxThread( void const *argument )
{
	osEvent		evt;
	MsgDiag_t	*message;
	PTmsgPkt_t	*packet;
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;
	//stCanPacket g_InRxCanPacket;
	//eCanType	bStandardCan;
	//eDiagKLINEState eKLINERxState;
	uint8_t	rxBuff[2000]	= { 0, },i;//kkt 
	uint32_t ret;
	
	for(;;)
	{
		evt	= osMessageGet( hOBDKlineRxMessage, osWaitForever );
		if( evt.status == osEventMessage )
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hOBDKlineRxMessage));
#endif
			message = ( MsgDiag_t * )evt.value.p;
			packet	= ( PTmsgPkt_t* )message->pPacket;

			//osPoolFree( hPTPKPool, (void *)packet );
			//osPoolFree( hDiagPool, (void *)message );

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
			memcpy(pPTpacket, packet, sizeof(PTmsgPkt_t));
			memcpy(pDiagmsg, message, sizeof(MsgDiag_t));
			pDiagmsg->pPacket = pPTpacket;

			osPoolFree( hPTPKPool, (void *)packet );
			osPoolFree( hDiagPool, (void *)message );

			if ( pDiagmsg->subEvent == eKLINE_RX_BLOCK )
			{
				unsigned int uiProtocolID = VCI_GetPassThruProtocolID();
				
				g_bAckflag=0;
			
				switch(uiProtocolID)
				{
					case WABCO_ABS :
					{
					  	ret = getWabcoAbsRxBlock(GetKlineSelect(),rxBuff);
					  	break;
					}
					case ISO9141_BOSCH : 
					{
					  	ret = getBoschRxBlock(GetKlineSelect(),rxBuff);
					  	break;
					}
					default :
					{
					  	ret = GetKlineDataTime(GetKlineSelect(),rxBuff);
					  	break;
					}
					
				}
				if( g_bKLogOnRxFlag == true )
				{
					int ii=0;
					GLogN("Krx: %d, ",ret);
					for(ii=0; ii<ret; ii++) { GLogN("%02X ",rxBuff[ii]); }
					GLogN("\r\n");
				}
				
				if(GetCurFwServiceMode()!=eApp_Inside)	g_bAckflag = 1;
				intDlccomCount = g_uiAckTiming; // Ack Timming
				
				if(g_bIsFastInit==TRUE)
				{
					if(ret!=0) g_bFastInit_Success=1;
				}
				
				pPTpacket->DataSize = ret;
				for(i=0; i<ret; i++)
				{
					pPTpacket->pData[i]=rxBuff[i];
				}

				if(ret>0)
				{
					//eKLINERxState = eKLINE_RX_COMPLETE;
#if defined ( SAVE_CAN_LOG )
					if ((g_ucCANLog_Enable == 1) && (GetCurFwServiceMode() == eApp_VCI_2))
					{				  
						WriteKlLog( pPTpacket->pData, pPTpacket->DataSize );
					}
#endif
					//PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType);
					pDiagmsg->subEvent =  eDIAG_COMM_RX_OK;
					if(osMessageAvailableSpace(hDiagMsg) == 0)
					{
						osPoolFree( hPTPKPool, (void *)pPTpacket );
						osPoolFree( hDiagPool, (void *)pDiagmsg );
					}
					else
					{
						osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
					}
				}
				else
				{
					//PassThruReadMsgs(pReadMsg, g_uiCanReadMsgLength, g_InputCommType);
					g_uiCanReadMsgLength = 0;
					pDiagmsg->subEvent =  eDIAG_COMM_RX_FAIL;
					if(osMessageAvailableSpace(hDiagMsg) == 0)
					{
						osPoolFree( hPTPKPool, (void *)pPTpacket );
						osPoolFree( hDiagPool, (void *)pDiagmsg );
					}
					else
					{
						osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
					}
				}
				//g_b3002Lock = false; //Is need Kline communication?
			}
		}
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#endif
	}
}

void CAN_SendNotiRcvMultiFrame( ePKT_TD eInCommType )
{
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;

	pDiagmsg = ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
	if( pDiagmsg == NULL )
	{
		return;
	}
	pPTpacket = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
	if( pPTpacket == NULL )
	{
		osPoolFree( hDiagPool, (void *)pDiagmsg );
		return;
	}
	
	pPTpacket->DataSize = 3;
	
	pPTpacket->pData[0] = 0;
	pPTpacket->pData[1] = 0;
	pPTpacket->pData[2] = 0;
	
	pDiagmsg->mMsgType 		= MSG_DIAG;
	pDiagmsg->event			= DIAG_PASSTHRU;
	pDiagmsg->subEvent 		= eDIAG_COMM_RX_ING;
	pDiagmsg->mPktType 		= (ePKT_TD)eInCommType;
	pDiagmsg->unEventTime 	= GetUnixTime();
	pDiagmsg->pPacket 		= (void *)pPTpacket;
	
	if(osMessageAvailableSpace(hDiagMsg) == 0)
	{
		osPoolFree( hPTPKPool, (void *)pPTpacket );
		osPoolFree( hDiagPool, (void *)pDiagmsg );
	}
	else
	{
		osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
	}
}


/**
 * @brief : To Check New UDS Filler.
 * @param : unProtocolID is CAN protocolID
 * @return : New UDS Filler True, Not New UDS Filler False
 *
 */
bool CheckNewUDS( uint32_t unProtocolID )
{
#ifndef NEW_UDS_FILLER
    return false;
#endif
    switch( unProtocolID )
    {
        case ISO14229_UDS :
        case ISO14229_ES95486_02_100 : 
        case ISO14229_ES95486_02_100_NEW : 
#ifdef CANFD_PROTOCOL
        case ISO14229_ES95486_02_130_CANFD : 
        case ISO14229_ES95486_02_131_CANFD : 
#endif
        case ISO14229_ES95486_02_102 : 
        case ISO14229_ES95486_02_103 : 
        case ISO14229_ES95486_02_104 : 
        case ISO14229_ES95486_02_105 : 
        case ISO14229_ES95486_02_106 : 
        case ISO14229_ES95486_02_107 : 
        case ISO14229_ES95486_02_108 : 
        case ISO14229_ES95486_02_109 : 
        case ISO14229_ES95486_02_10A : 
        case ISO14229_ES95486_02_10B : 
        case ISO14229_ES95486_02_10C : 
        case ISO14229_ES95486_02_10D : 
        case ISO14229_ES95486_02_10E : 
        case ISO14229_ES95486_02_10F : 
        case ISO14229_ES95486_170 : 
        case ISO14229_ES95486_170_MMCAN : 
        case ISO14230_ES95486_170 : 
        case ISO14230_ES95486_170_MMCAN : 
        case ISO14230_ES95486_DOIP_120 : 
        case ISO14230_ES95486_DOIP_121 : 
        case ISO14230_ES95486_DOIP_123 : 
        case ISO14230_ES95486_DOIP_124 : 
        case ISO14230_ES95486_DOIP_125 : 
        case ISO14230_ES95486_DOIP_126 : 
        case ISO14230_ES95486_DOIP_127 : 
        case ISO14230_ES95486_DOIP_128 : 
        case ISO14230_ES95486_DOIP_129 : 
        case ISO14230_ES95486_DOIP_12A : 
        case ISO14230_ES95486_DOIP_12B : 
        case ISO14230_ES95486_DOIP_12C : 
        case ISO14230_ES95486_DOIP_12D : 
        case ISO14230_ES95486_DOIP_12E : 
        case ISO14230_ES95486_DOIP_12F : 
#ifdef NEW_29BIT_CAN
        case ISO15765_ES95486_29bit : 
        case ISO15765_ES95486_DOIP_29BIT : 
#ifdef CANFD_PROTOCOL
        case ISO15765_ES95486_135_29bit_CANFD : 
        case ISO15765_ES95486_136_29bit_CANFD :
#endif
#endif
            return true;
        default : 
            return false;
    }
}


