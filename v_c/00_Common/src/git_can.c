/*************************************************************
 * NOTE : git_can.c
 *      FDCAN control
 * Author : Lee junho
 * Since : 2019.09.03
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "fdcan.h"

#include "common.h"
#include "typedef.h"
#include "git_mmc.h"
#include "git_vci.h"
#include "git_ioctl.h"

#include "git_can.h"
#include "git_OBDcomm.h"
#include "typedef.h"
#include "led.h"
#include "git_function_list.h"
#ifdef VCI3_DIAG
#include "git_mcp2518fd.h"
#endif
#include "git_PassthruDefines.h"

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
 #if 1 //jkc
static void transmitFDCanThread( void const *argument );
 #endif
unsigned int OemWriteCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
void OemWriteCanBuff1(unsigned char* pBuff, unsigned char ch);
U8 OemReadCanBuff(U8 *pRxBuff, U32 timeout);
void SelCANFDLine(U8 BitrateNum, U8 *nominalBitrate, U8 *DataBitrate);

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
#ifdef PRINT_MESSAGE_ID
	stMESSAGE_ID_INFO stMessageIdInfo[20];
 	uint8_t ucMessageIdInfoCnt=0;
#endif
// message queue
osMessageQId	hFDRxMsg;
osMessageQId	hFDTxMsg;
osMessageQDef( fdcanmsg, FDCAN_MESSAGE_QUEUE_SIZE, stMsgClst );
#if 1 //jkc
// thread
osThreadId		hTransmitFDTh;
osThreadDef( transmitfdth, transmitFDCanThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );
#endif 
static uint8_t	g_filterNbr1	= 0;
#ifdef VCI_III_USB_HS
#else
static uint8_t	g_filterNbr2	= 0;
#endif

// external variables
extern FDCAN_HandleTypeDef hfdcan1;
extern stRECORD_HW_SET		g_stGITHWSetData;
#ifdef VCI_III_USB_HS

#else
//extern FDCAN_HandleTypeDef hfdcan2;
#endif
U32 g_CurLoc = 0;
extern osPoolId		hFdcanMsgPool;
extern uint32_t		g_ulProtocolID;
extern U16 g_usES95486_RxCANID;//TX CAN ID +8 만 수신처리 20220409 KKT
extern U32 g_us29bitES95486_RxCANID;//TX CAN ID TA SA 치환만 처리 20230912 Q_hyek
extern bool g_bCanLogOnTxFlag;
extern bool g_bCanLogOnRxFlag;
extern U32 g_ulPGN;
#ifdef CANFD_qhyek //Q_hyek CANFD
extern uint8_t g_ucCanformat;
#endif
extern u32	intTxdRxdCount;
extern bool g_bCanIdSwFilterEn;

/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/
int32_t initFDCan( void )
{
#ifdef GITCAN_DEBUG_FUNC
	GLogN( "[GITCAN] +%s\r\n", __FUNCTION__ );
#endif

	// receive message
	hFDRxMsg = osMessageCreate( osMessageQ( fdcanmsg ), NULL );
	if( hFDRxMsg == NULL )
	{
		GLogE( "error... osMessageCreate hFDRxMsg\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hFDRxMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hFDRxMsg",sizeof("hFDRxMsg"));
#endif

	// transmit message
	hFDTxMsg = osMessageCreate( osMessageQ( fdcanmsg ), NULL );
	if( hFDTxMsg == NULL )
	{
		GLogE( "error... osMessageCreate hFDTxMsg\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hFDTxMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hFDTxMsg",sizeof("hFDTxMsg"));
#endif


	return INIT_OK;
}

void deinitFDCan( void )
{
	if( hfdcan1.State == HAL_FDCAN_STATE_BUSY )
	{
		GLogN( "Using FDCAN1 goto Stop!!!\r\n" );
		stopFDCan( &hfdcan1 );							// stop FDCAN1
	}
#ifdef VCI_III_USB_HS

#else
	if( hfdcan2.State == HAL_FDCAN_STATE_BUSY )
	{
		GLogN( "Using FDCAN2 goto Stop!!!\r\n" );
		stopFDCan( &hfdcan2 );							// stop FDCAN2
	}

#endif
}

uint8_t startFDCanThread( void )
{
#if 1 //jkc
	// create thread
	hTransmitFDTh = osThreadCreate( osThread(transmitfdth), NULL );
	if( hTransmitFDTh == NULL )
	{
		GLogE( "Error... fail create hTransmitFDTh Thread!!!\r\n" );
		return 0;
	}
#endif
	return 1;
}

/*----------------------------------------------------------------------
 *   high1		FDCAN1 enable
 *   high2		FDCAN2 enable(for high)
 *   low		FDCAN2 enable(for low)
 *--------------------------------------------------------------------*/
uint8_t startFDCan( uint8_t high1, uint8_t high2, uint8_t low )
{
#ifdef GITCAN_DEBUG_FUNC
	GLogN( "[GITCAN] +%s\r\n", __FUNCTION__ );
#endif

	if( high2 == 1 && low == 1 )
	{
		GLogEE( "Low and high2 not support at the same time!!!\r\n" );
		return 0;
	}

	if( high1 )
	{
	  	DisableHighCan2();
		DisableLowCan2();
	  	EnableHighCan1();
	}
	if( high2 )
	{
	 	DisableHighCan1();
		DisableLowCan2();
	  	EnableHighCan2();
	}
	if( low )
	{
	 	DisableHighCan1(); 
		DisableHighCan2();
	  	EnableLowCan2();
	}

#ifdef VCI_III_USB_HS
#else
	if( high1 )
	{
		// Config Global Filter
		if( HAL_FDCAN_ConfigGlobalFilter( &hfdcan2, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE ) != HAL_OK )
		{
			GLogE( "error... FDCAN HAL_FDCAN_ConfigGlobalFilter!!!\r\n" );
			return HAL_ERROR;
		}

		// Start Fifo0, Fifo1 IT
		HAL_FDCAN_ActivateNotification( &hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0 );
		HAL_FDCAN_ActivateNotification( &hfdcan2, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0 );

		if( HAL_FDCAN_Start( &hfdcan2 ) != HAL_OK)
		{
			GLogE( "error...  FDCAN HAL_FDCAN_Start!!!\r\n" );
			return HAL_ERROR;
		}
	}
#endif
#ifdef USE_INTERNAL_CAN_ONLY
#else
	if( high2 || low )
#endif
	{
		// Config Global Filter
		if( HAL_FDCAN_ConfigGlobalFilter( &hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE ) != HAL_OK )
		{
			GLogE( "error... FDCAN HAL_FDCAN_ConfigGlobalFilter!!!\r\n" );
			return HAL_ERROR;
		}

		// Start Fifo0, Fifo1 IT
		HAL_FDCAN_ActivateNotification( &hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0 );
		HAL_FDCAN_ActivateNotification( &hfdcan1, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0 );

		if( HAL_FDCAN_Start( &hfdcan1 ) != HAL_OK)
		{
			GLogE( "error...  FDCAN HAL_FDCAN_Start!!!\r\n" );
			return HAL_ERROR;
		}
	}
	return HAL_OK;
}

/*----------------------------------------------------------------------
 *   hfdcan		FDCAN1 / FDCAN2 Handle
 *--------------------------------------------------------------------*/
uint8_t stopFDCan( FDCAN_HandleTypeDef* hfdcan )
{
	// HAL Stop
	if( HAL_FDCAN_Stop( hfdcan ) != HAL_OK)
	{
		GLogE( "error...  FDCAN HAL_FDCAN_Stop!!!\r\n" );
		return HAL_ERROR;
	}

	// transceiver disable
	if( hfdcan->Instance == FDCAN1 )
	{
		//DisableHighCan1();
		DisableHighCan2();
		DisableLowCan2();
	}
#ifdef VCI_III_USB_HS
#else
	else
	{
		//DisableHighCan2();
		//DisableLowCan2();
		DisableHighCan1();
	}
#endif
	return HAL_OK;
}

/*----------------------------------------------------------------------
 *   hfdcan		FDCAN1 / FDCAN2 Handle
 *   config		FDCAN_FILTER_TO_RXFIFO0 / FDCAN_FILTER_TO_RXFIFO1
 *   id1		start id
 *   id2		end id
 *--------------------------------------------------------------------*/
uint8_t setFilter1( uint32_t config, uint32_t id1, uint32_t id2 )
{
	FDCAN_FilterTypeDef sFilterConfig;

	sFilterConfig.IdType		= FDCAN_STANDARD_ID;							// FDCAN_STANDARD_ID, FDCAN_EXTENDED_ID
	sFilterConfig.FilterIndex	= g_filterNbr1++;
	sFilterConfig.FilterType	= FDCAN_FILTER_RANGE;							// FDCAN_FILTER_RANGE, FDCAN_FILTER_DUAL, FDCAN_FILTER_MASK, FDCAN_FILTER_RANGE_NO_EIDM
	sFilterConfig.FilterConfig	= config;
	sFilterConfig.FilterID1		= id1;
	sFilterConfig.FilterID2		= id2;

	if( HAL_FDCAN_ConfigFilter( &hfdcan1, &sFilterConfig ) != HAL_OK )
	{
		GLogE( "error... FDCAN HAL_FDCAN_ConfigFilter %d !!!\r\n", g_filterNbr1 - 1 );
		return HAL_ERROR;
	}

	// Config Global Filter
	if( HAL_FDCAN_ConfigGlobalFilter( &hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE ) != HAL_OK )
	{
		GLogE( "error... FDCAN1 HAL_FDCAN_ConfigGlobalFilter!!!\r\n" );
		return HAL_ERROR;
	}

	// Start Fifo0, Fifo1 IT
	HAL_FDCAN_ActivateNotification( &hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0 );
	HAL_FDCAN_ActivateNotification( &hfdcan1, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0 );
	
	return HAL_OK;
}
uint8_t setFilter2( uint32_t config, uint32_t id1, uint32_t id2 )
{
#ifndef VCI_III_USB_HS
	FDCAN_FilterTypeDef sFilterConfig;

	sFilterConfig.IdType		= FDCAN_STANDARD_ID;							// FDCAN_STANDARD_ID, FDCAN_EXTENDED_ID
	sFilterConfig.FilterIndex	= g_filterNbr2++;
	sFilterConfig.FilterType	= FDCAN_FILTER_RANGE;							// FDCAN_FILTER_RANGE, FDCAN_FILTER_DUAL, FDCAN_FILTER_MASK, FDCAN_FILTER_RANGE_NO_EIDM
	sFilterConfig.FilterConfig	= config;
	sFilterConfig.FilterID1		= id1;
	sFilterConfig.FilterID2		= id2;

	if( HAL_FDCAN_ConfigFilter( &hfdcan2, &sFilterConfig ) != HAL_OK )
	{
		GLogE( "error... FDCAN HAL_FDCAN_ConfigFilter %d !!!\r\n", g_filterNbr2 - 1 );
		return HAL_ERROR;
	}
#else
	return HAL_OK;
#endif
}


uint8_t	clearFilter( FDCAN_HandleTypeDef *hfdcan )
{
	FDCAN_FilterTypeDef sFilterConfig;

	uint8_t	i;
	uint8_t	count = 0;
	//MONI 20230419 static analysis num : 32 / intialize local variable 	


	if( hfdcan->Instance == FDCAN1 )
	{
		count			= g_filterNbr1;
		g_filterNbr1	= 0;
	}
#ifdef VCI_III_USB_HS
#else
	else
	{
		count			= g_filterNbr2;
		g_filterNbr2	= 0;
	}
#endif

	for( i = 0; i < count; i++ )
	{
		sFilterConfig.IdType		= FDCAN_STANDARD_ID;
		sFilterConfig.FilterConfig	= FDCAN_FILTER_DISABLE;
		sFilterConfig.FilterIndex	= i;

		if( HAL_FDCAN_ConfigFilter( hfdcan, &sFilterConfig ) != HAL_OK )
		{
			GLogE( "error... FDCAN Filter clear!!!" );
			break;
		}
	}

	if( i != count )					return 0;
	else								return 1;
}

void clearRXCanMessage( void )
{
	osEvent		evt;

	stMsgClst	*message;
	stFdcanPkt	*packet;

	uint32_t	i = 0;

	for( i = 0; i < FDCAN_MESSAGE_QUEUE_SIZE; i++ )
	{
		evt = osMessageGet( hFDRxMsg, 0 );//kkt.
		if( evt.status == osEventMessage )
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif

			message = (stMsgClst *)evt.value.p;
			packet	= (stFdcanPkt*)message->pPacket;
			//GLogN("!~%02X%02X%02X%02X%02X\r\n",packet[0],packet[1],packet[2],packet[3],packet[4]);

			osPoolFree( hFdcanPktPool, (void *)packet );
			osPoolFree( hMsgPool, (void *)message );
		}
		else
		{
			break;
		}
	}
}

void clearTXCanMessage( void )
{
	osEvent		evt;

	stMsgClst	*message;
	stFdcanPkt	*packet;

	uint32_t	i = 0;

	for( i = 0; i < FDCAN_MESSAGE_QUEUE_SIZE; i++ )
	{
		evt = osMessageGet( hFDTxMsg, 2 );
		if( evt.status == osEventMessage )
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDTxMsg));
#endif
			message = (stMsgClst *)evt.value.p;
			packet	= (stFdcanPkt*)message->pPacket;

			osPoolFree( hFdcanPktPool, (void *)packet );
			osPoolFree( hMsgPool, (void *)message );
		}
		else
		{
			break;
		}
	}
}

/*****************************************************************************
   header	: Can TxHeader Handler
   iden		: identifier
   type		: identifier type( eCanIDType )
   len		: DLC length
   format	: CAN frame format( eCanFrameFormat )
 *****************************************************************************/
void makeTxHeaderCAN( FDCAN_TxHeaderTypeDef *header, uint32_t iden, uint8_t type, uint8_t len, uint8_t format )		// 11byte identifier
{
	header->Identifier				= iden;
	header->TxFrameType				= FDCAN_DATA_FRAME;
	header->ErrorStateIndicator		= FDCAN_ESI_ACTIVE;
	header->TxEventFifoControl		= FDCAN_NO_TX_EVENTS;
	header->MessageMarker			= 0;

	/* Check CAN format */
	if( g_ucCanformat != format )
	{
		if(g_ucCanformat == CAN_FRAMEFORMAT_CLASSIC)
		{
			format = CAN_FRAMEFORMAT_CLASSIC;							
		}
		else
		{	
			format = CAN_FRAMEFORMAT_FDCAN;
		}
	}

	
	if( format == CAN_FRAMEFORMAT_CLASSIC )
	{
		if( type == CAN_IDTYPE_EXTENDED )			header->IdType	= CAN_IDTYPE_EXTENDED;					// 29byte identifier
		else										header->IdType	= CAN_IDTYPE_STANDARD;					// 11byte identifier
	}
	else
	{
		if( type == CAN_IDTYPE_EXTENDED )			header->IdType	= FDCAN_EXTENDED_ID;					// 29byte identifier
		else										header->IdType	= FDCAN_STANDARD_ID;					// 11byte identifier
	}

	switch( len )
	{
		case  0 :			header->DataLength = FDCAN_DLC_BYTES_0;				break;					/*!< 0 bytes data field  */
		case  1 :			header->DataLength = FDCAN_DLC_BYTES_1;				break;					/*!< 1 bytes data field  */
		case  2 :			header->DataLength = FDCAN_DLC_BYTES_2;				break;					/*!< 2 bytes data field  */
		case  3 :			header->DataLength = FDCAN_DLC_BYTES_3;				break;					/*!< 3 bytes data field  */
		case  4 :			header->DataLength = FDCAN_DLC_BYTES_4;				break;					/*!< 4 bytes data field  */
		case  5 :			header->DataLength = FDCAN_DLC_BYTES_5;				break;					/*!< 5 bytes data field  */
		case  6 :			header->DataLength = FDCAN_DLC_BYTES_6;				break;					/*!< 6 bytes data field  */
		case  7 :			header->DataLength = FDCAN_DLC_BYTES_7;				break;					/*!< 7 bytes data field  */
		case  8 :			header->DataLength = FDCAN_DLC_BYTES_8;				break;					/*!< 8 bytes data field  */
		case 12 :			header->DataLength = FDCAN_DLC_BYTES_12;			break;					/*!< 12 bytes data field */
		case 16 :			header->DataLength = FDCAN_DLC_BYTES_16;			break;					/*!< 16 bytes data field */
		case 20 :			header->DataLength = FDCAN_DLC_BYTES_20;			break;					/*!< 20 bytes data field */
		case 24 :			header->DataLength = FDCAN_DLC_BYTES_24;			break;					/*!< 24 bytes data field */
		case 32 :			header->DataLength = FDCAN_DLC_BYTES_32;			break;					/*!< 32 bytes data field */
		case 48 :			header->DataLength = FDCAN_DLC_BYTES_48;			break;					/*!< 48 bytes data field */
		case 64 :			header->DataLength = FDCAN_DLC_BYTES_64;			break;					/*!< 64 bytes data field */
		default	:			header->DataLength = FDCAN_DLC_BYTES_0;				break;					/*!< 0 bytes data field  */
	}

	if( format == CAN_FRAMEFORMAT_FDCAN )					// FDCAN
	{
		header->BitRateSwitch		= FDCAN_BRS_ON;
		header->FDFormat			= FDCAN_FD_CAN;
	}
	else													// ClassicCan
	{
		header->BitRateSwitch		= FDCAN_BRS_OFF;
		header->FDFormat			= FDCAN_CLASSIC_CAN;
	}
}

/*****************************************************************************
   BaudRate = fdcan_ck / ( Can prescaler * ( 1 + BS1 + BS2 ) )
   Sampling = ( 1 + BS1 ) / ( 1 + BS1 + BS2 )
 *****************************************************************************/
uint8_t CanSet_Baud( FDCAN_HandleTypeDef* hfdcan, uint8_t nominalBaud, uint8_t dataBaud, uint8_t format )
{
	uint8_t nprec	= 0;
	uint8_t nbs1	= 0;
	uint8_t nbs2	= 0;
	uint8_t dprec	= 0;
	uint8_t dbs1	= 0;
	uint8_t dbs2	= 0;

	/* Nominal Bit Time */
	switch( nominalBaud )
	{
		case CAN_1MBPS:				nprec	= 5;			break;
		case CAN_250KBPS:			nprec	= 20;			break;
		case CAN_125KBPS:			nprec	= 40;			break;
		case CAN_100KBPS:			nprec	= 50;			break;
		case CAN_500KBPS:
		default:					nprec	= 10;			break;
	}

	if( g_stGITSetConfig.nBitSamplePoint > 0x84 )					// 87.5%
	{
		nbs1	= 13;
		nbs2	= 2;
	}
	else if( g_stGITSetConfig.nBitSamplePoint > 0x80 )				// 80%
	{
		nbs1	= 12;
		nbs2	= 3;
	}
	else if( g_stGITSetConfig.nBitSamplePoint > 0x70 )				// 75%
	{
		nbs1	= 11;
		nbs2	= 4;
	}
	else															// 80%
	{
		nbs1	= 12;
		nbs2	= 3;
	}

	/* Data Bit Time */
	dprec	= 2;

	switch( dataBaud )
	{
        //case CAN_500KBPS:			dbs1 = 15;	dbs2 = 4;		break;
        //case CAN_250KBPS:			dbs1 = 31;	dbs2 = 8;		break;
        //case CAN_125KBPS:			dbs1 = 63;	dbs2 = 16;		break;
        //case CAN_100KBPS:			dbs1 = 80;	dbs2 = 24;		break;
		case CAN_DAT_1MBPS:			dbs1 = 31;	dbs2 = 8;		break;
		case CAN_DAT_4MBPS:			dbs1 = 7;	dbs2 = 2;		break;
		case CAN_DAT_2MBPS:
		default:					dbs1 = 15;	dbs2 = 4;		break;
	}

	if( format == CAN_FRAMEFORMAT_FDCAN )
	{
		hfdcan->Init.FrameFormat		= FDCAN_FRAME_FD_BRS;
	}
	else
	{
		hfdcan->Init.FrameFormat		= FDCAN_FRAME_CLASSIC;
	}

	hfdcan->Init.NominalPrescaler		= nprec;
	hfdcan->Init.NominalSyncJumpWidth	= 1;
	hfdcan->Init.NominalTimeSeg1		= nbs1;
	hfdcan->Init.NominalTimeSeg2		= nbs2;
	hfdcan->Init.DataPrescaler			= dprec;
	hfdcan->Init.DataSyncJumpWidth		= 1;
	hfdcan->Init.DataTimeSeg1			= dbs1;
	hfdcan->Init.DataTimeSeg2			= dbs2;

	if( HAL_FDCAN_Init( hfdcan ) != HAL_OK )
	{
		GLogE( "Error... Fail init FDCAN!!!\r\n" );
		Error_Handler();
	}

	clearFilter( hfdcan );

	return 1;
}

void HAL_FDCAN_RxFifo0Callback( FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs )
{
	HAL_StatusTypeDef		ret;

	stFdcanPkt	*packet;
	stMsgClst	*message;

	uint32_t	count = 0;

	if( ( RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE ) != RESET )
	{
		count = HAL_FDCAN_GetRxFifoFillLevel( hfdcan, FDCAN_RX_FIFO0 );
		while( count != 0 )
		{
			message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
			if( message != NULL )
			{
				packet	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
				if( packet != NULL )
				{
					ret = HAL_FDCAN_GetRxMessage( hfdcan, FDCAN_RX_FIFO0, &packet->mRxHeader, packet->mData );
					if( ret == HAL_ERROR )
					{
						GLogE( "error... HAL_FDCAN_GetRxMessage\r\n" );
						osPoolFree( hFdcanPktPool, (void *)packet );
						osPoolFree( hMsgPool, (void *)message );
						continue;
					}
					count--;

					switch( packet->mRxHeader.DataLength )
					{
						case FDCAN_DLC_BYTES_0  :		packet->mLen = 0;			break;					/*!< 0 bytes data field  */
						case FDCAN_DLC_BYTES_1  :		packet->mLen = 1;			break;					/*!< 1 bytes data field  */
						case FDCAN_DLC_BYTES_2  :		packet->mLen = 2;			break;					/*!< 2 bytes data field  */
						case FDCAN_DLC_BYTES_3  :		packet->mLen = 3;			break;					/*!< 3 bytes data field  */
						case FDCAN_DLC_BYTES_4  :		packet->mLen = 4;			break;					/*!< 4 bytes data field  */
						case FDCAN_DLC_BYTES_5  :		packet->mLen = 5;			break;					/*!< 5 bytes data field  */
						case FDCAN_DLC_BYTES_6  :		packet->mLen = 6;			break;					/*!< 6 bytes data field  */
						case FDCAN_DLC_BYTES_7  :		packet->mLen = 7;			break;					/*!< 7 bytes data field  */
						case FDCAN_DLC_BYTES_8  :		packet->mLen = 8;			break;					/*!< 8 bytes data field  */
						case FDCAN_DLC_BYTES_12 :		packet->mLen = 12;			break;					/*!< 12 bytes data field */
						case FDCAN_DLC_BYTES_16 :		packet->mLen = 16;			break;					/*!< 16 bytes data field */
						case FDCAN_DLC_BYTES_20 :		packet->mLen = 20;			break;					/*!< 20 bytes data field */
						case FDCAN_DLC_BYTES_24 :		packet->mLen = 24;			break;					/*!< 24 bytes data field */
						case FDCAN_DLC_BYTES_32 :		packet->mLen = 32;			break;					/*!< 32 bytes data field */
						case FDCAN_DLC_BYTES_48 :		packet->mLen = 48;			break;					/*!< 48 bytes data field */
						case FDCAN_DLC_BYTES_64 :		packet->mLen = 64;			break;					/*!< 64 bytes data field */
						default					:		packet->mLen = 0;			break;
					}

					packet->mTimeStamp = 0;

					if( hfdcan->Instance == FDCAN1 )
					{
						packet->pSource	= &hfdcan1;
						packet->pTarget	= &hfdcan1;
					}
#ifdef VCI_III_USB_HS

#else

					else if( hfdcan->Instance == FDCAN2 )
					{
						packet->pSource	= &hfdcan2;
						packet->pTarget	= &hfdcan2;
					}
#endif

					message->mPktType	= PACKET_FDCAN;
					message->pPacket	= (void *)packet;

					if( xQueueIsQueueFullFromISR( hFDRxMsg ) == TRUE )
					{
						osPoolFree( hFdcanPktPool, (void *)packet );
						osPoolFree( hMsgPool, (void *)message );
					}
					else
					{
						osMessagePut( hFDRxMsg, (uint32_t)message, osWaitForever );
					}
				}
				else
				{
					osPoolFree( hMsgPool, (void *)message );
					GLogN( "CP1\r\n" );
					osDelay( 10 );
				}
			}
			else
			{
				GLogN( "CM1\r\n" );
				osDelay( 10 );
			}
		}
	}

	HAL_FDCAN_ActivateNotification( hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0 );
}

void HAL_FDCAN_RxFifo1Callback( FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs )
{
	HAL_StatusTypeDef	ret;

	stFdcanPkt	*packet;
	stMsgClst	*message;

	uint32_t	count = 0;

	if( ( RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE ) != RESET )
	{
		count = HAL_FDCAN_GetRxFifoFillLevel( hfdcan, FDCAN_RX_FIFO1 );
		while( count != 0 )
		{
			message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
			if( message != NULL )
			{
				packet	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
				if( packet != NULL )
				{
					ret = HAL_FDCAN_GetRxMessage( hfdcan, FDCAN_RX_FIFO1, &packet->mRxHeader, packet->mData );
					if( ret == HAL_ERROR )
					{
						GLogE( "error... HAL_FDCAN_GetRxMessage\r\n" );
						osPoolFree( hFdcanPktPool, (void *)packet );
						osPoolFree( hMsgPool, (void *)message );
						continue;
					}
					count--;

					switch( packet->mRxHeader.DataLength )
					{
						case FDCAN_DLC_BYTES_0  :		packet->mLen = 0;			break;					/*!< 0 bytes data field  */
						case FDCAN_DLC_BYTES_1  :		packet->mLen = 1;			break;					/*!< 1 bytes data field  */
						case FDCAN_DLC_BYTES_2  :		packet->mLen = 2;			break;					/*!< 2 bytes data field  */
						case FDCAN_DLC_BYTES_3  :		packet->mLen = 3;			break;					/*!< 3 bytes data field  */
						case FDCAN_DLC_BYTES_4  :		packet->mLen = 4;			break;					/*!< 4 bytes data field  */
						case FDCAN_DLC_BYTES_5  :		packet->mLen = 5;			break;					/*!< 5 bytes data field  */
						case FDCAN_DLC_BYTES_6  :		packet->mLen = 6;			break;					/*!< 6 bytes data field  */
						case FDCAN_DLC_BYTES_7  :		packet->mLen = 7;			break;					/*!< 7 bytes data field  */
						case FDCAN_DLC_BYTES_8  :		packet->mLen = 8;			break;					/*!< 8 bytes data field  */
						case FDCAN_DLC_BYTES_12 :		packet->mLen = 12;			break;					/*!< 12 bytes data field */
						case FDCAN_DLC_BYTES_16 :		packet->mLen = 16;			break;					/*!< 16 bytes data field */
						case FDCAN_DLC_BYTES_20 :		packet->mLen = 20;			break;					/*!< 20 bytes data field */
						case FDCAN_DLC_BYTES_24 :		packet->mLen = 24;			break;					/*!< 24 bytes data field */
						case FDCAN_DLC_BYTES_32 :		packet->mLen = 32;			break;					/*!< 32 bytes data field */
						case FDCAN_DLC_BYTES_48 :		packet->mLen = 48;			break;					/*!< 48 bytes data field */
						case FDCAN_DLC_BYTES_64 :		packet->mLen = 64;			break;					/*!< 64 bytes data field */
						default					:		packet->mLen = 0;			break;
					}

					packet->mTimeStamp = 0;

					if( hfdcan->Instance == FDCAN1 )
					{
						packet->pSource	= &hfdcan1;
						packet->pTarget	= &hfdcan1;
					}
#ifdef VCI_III_USB_HS

#else
					else if( hfdcan->Instance == FDCAN2 )
					{
						packet->pSource	= &hfdcan2;
						packet->pTarget	= &hfdcan2;
					}
#endif

					message->mPktType	= PACKET_FDCAN;
					message->pPacket	= (void *)packet;

					if( xQueueIsQueueFullFromISR( hFDRxMsg ) == TRUE )
					{
						osPoolFree( hFdcanPktPool, (void *)packet );
						osPoolFree( hMsgPool, (void *)message );
					}
					else
					{
						osMessagePut( hFDRxMsg, (uint32_t)message, osWaitForever );
					}
				}
				else
				{
					osPoolFree( hMsgPool, (void *)message );
					GLogN( "CP1\r\n" );
					osDelay( 10 );
				}
			}
			else
			{
				GLogN( "CM1\r\n" );
				osDelay( 10 );
			}
		}
	}

	HAL_FDCAN_ActivateNotification( hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0 );
}

/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/
 #if 1 //jkc
void transmitFDCanThread( void const *argument )
{
	osEvent		evt;
	stMsgClst	*message;
	stFdcanPkt	*packet;
#if defined ( SAVE_CAN_LOG )
	uint32_t	can_id;
	uint16_t	size;
#endif
	

#ifdef GITCAN_DEBUG_FUNC
	GLogN( "[GITCAN] +%s\r\n", __FUNCTION__ );
#endif

	for(;;)
	{
        //jkc printf("----------------->test\n");
		evt	= osMessageGet( hFDTxMsg, osWaitForever );
		if( evt.status == osEventMessage )
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDTxMsg));
			GLogN( "[%s] get Meesage\r\n", __FUNCTION__ );
#endif

#ifdef GITCAN_DEBUG_FUNC
			GLogN( "[%s] get Meesage\r\n", __FUNCTION__ );
#endif

			message = ( stMsgClst * )evt.value.p;
			packet	= ( stFdcanPkt * )message->pPacket;

			if( g_bCanLogOnTxFlag == true )
			{
				int ii=0;
				if( packet->mTxHeader.IdType==FDCAN_STANDARD_ID)	GLogN("Tx: %04X ",packet->mTxHeader.Identifier);
				else												GLogN("Tx: %08X ",packet->mTxHeader.Identifier);
				for(ii=0;ii<packet->mLen;ii++)
				{
					GLogN("%02X ",packet->mData[ii]);
				}
				GLogN("\r\n");
			}

			if(packet->pTarget == &hfdcan1)
			{
#ifdef GITCAN_DEBUG_FUNC
				GLogN( "[%s] target fdcan1\r\n", __FUNCTION__ );
#endif
				while( HAL_FDCAN_GetTxFifoFreeLevel( packet->pTarget ) == 0 )
				{
					osDelay( 1 );
				}

				if( HAL_FDCAN_AddMessageToTxFifoQ( packet->pTarget, &packet->mTxHeader, packet->mData ) != HAL_OK )
				{
					GLogE( "error... fail HAL_FDCAN_AddMessageToTxFifoQ_%d\r\n",packet->pTarget->ErrorCode );
				}
#if defined ( SAVE_CAN_LOG )
				else
				{
				  	if ((g_ucCANLog_Enable == 1) && (GetCurFwServiceMode() == eApp_VCI_2))
					{				  
						can_id = packet->mTxHeader.Identifier;
						size = (uint16_t)(packet->mLen);
						WriteCANLog ( can_id, packet->mData, size );
					}
				}
#endif
//				if( message->mMsgType == MSG_PERIODIC )
//				{
//				 	if( (packet->mData[1] == 0x3E) && (packet->mData[2] == 0x80) )
//					{
//					  	//GLogN("50\r\n");
//                        GLogN("d\r\n");
//					  	osDelay(55);
//					}
//				}
			}
#ifdef VCI3_DIAG
			else if(packet->pTarget == (FDCAN_HandleTypeDef*)SPI5)
			{
				MsgClst_t	*spiMsg;
				SpiCanPkt_t	*spiPkt;
				uint8_t len=0;
				uint32_t uiProtocolID;
				uiProtocolID = VCI_GetPassThruProtocolID();
#ifdef GITCAN_DEBUG_FUNC
				GLogN( "[%s] target spican\r\n", __FUNCTION__ );
#endif
				// FDCAN -> SPICAN
				spiMsg	= ( MsgClst_t* )osPoolCAlloc( hMsgPool );
				if( spiMsg != NULL )
				{
					spiPkt	= ( SpiCanPkt_t* )osPoolCAlloc( hSpiCanPktPool );
					if( spiPkt != NULL )
					{
						// FDCAN_DLC_BYTES_64 0x000F 0000, 16
						//spiPkt->mTxObj.bF.ctrl.DLC = packet->mTxHeader.DataLength >> 16;

						switch( packet->mTxHeader.DataLength )
						{
							case FDCAN_DLC_BYTES_0 : len = 0;		break;
							case FDCAN_DLC_BYTES_1 : len = 1;		break;
							case FDCAN_DLC_BYTES_2 : len = 2;		break;
							case FDCAN_DLC_BYTES_3 : len = 3;		break;
							case FDCAN_DLC_BYTES_4 : len = 4;		break;
							case FDCAN_DLC_BYTES_5 : len = 5;		break;
							case FDCAN_DLC_BYTES_6 : len = 6;		break;
							case FDCAN_DLC_BYTES_7 : len = 7;		break;
							case FDCAN_DLC_BYTES_8 : len = 8;		break;
							case FDCAN_DLC_BYTES_12 : len = 12;		break;
							case FDCAN_DLC_BYTES_16 : len = 16;		break;
							case FDCAN_DLC_BYTES_20 : len = 20;		break;
							case FDCAN_DLC_BYTES_24 : len = 24;		break;
							case FDCAN_DLC_BYTES_32 : len = 32;		break;
							case FDCAN_DLC_BYTES_48 : len = 48;		break;
							case FDCAN_DLC_BYTES_64 : len = 64;		break;
							default	: len = 0;						break;
						}
						spiPkt->mTxObj.bF.ctrl.DLC = DRV_CANFDSPI_DataBytesToDlc(len);
						if (uiProtocolID == J1939)
						  spiPkt->mTxObj.bF.ctrl.DLC = len;
						spiPkt->mLen = len;
						/*
							header->TxFrameType				= FDCAN_DATA_FRAME;
							header->ErrorStateIndicator		= FDCAN_ESI_ACTIVE;
							header->TxEventFifoControl		= FDCAN_NO_TX_EVENTS;
							header->MessageMarker			= 0;
						*/

						if(packet->mTxHeader.IdType == CAN_IDTYPE_EXTENDED || packet->mTxHeader.IdType == FDCAN_EXTENDED_ID)
						{
							spiPkt->mTxObj.bF.ctrl.IDE = MCP2518_IDE_EXTENDED;
							spiPkt->mTxObj.bF.id.SID = packet->mTxHeader.Identifier >> 18;
							spiPkt->mTxObj.bF.id.EID = (packet->mTxHeader.Identifier & 0x3ffff);
						}
						else
						{
							spiPkt->mTxObj.bF.ctrl.IDE = MCP2518_IDE_STANDARD;
							spiPkt->mTxObj.bF.id.SID = packet->mTxHeader.Identifier;
							spiPkt->mTxObj.bF.id.EID = 0;
						}

						if(packet->mTxHeader.FDFormat == FDCAN_FD_CAN)
						{
							spiPkt->mTxObj.bF.ctrl.BRS	= 1;
							spiPkt->mTxObj.bF.ctrl.FDF	= 1;
						}
						else
						{
							spiPkt->mTxObj.bF.ctrl.BRS	= 0;
							spiPkt->mTxObj.bF.ctrl.FDF	= 0;
						}

						memcpy(spiPkt->mData, packet->mData,len);
						spiMsg->pPacket = (void *)spiPkt;

						osMessagePut( hTxSpiCanMsg1, (uint32_t)spiMsg, osWaitForever );

#if defined ( SAVE_CAN_LOG )
						if ((g_ucCANLog_Enable == 1) && (GetCurFwServiceMode() == eApp_VCI_2))
						{				  
							can_id = packet->mTxHeader.Identifier;
							size = (uint16_t)(packet->mLen);
							WriteCANLog ( can_id, packet->mData, size );
						}
#endif
					}
				}
			}
#endif
			
			osPoolFree( hFdcanPktPool, (void *)packet );
			osPoolFree( hMsgPool, (void *)message );
		}
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#endif
	}
}
 #endif
unsigned int OemWriteCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	stMsgClst	*message;
	stFdcanPkt	*packet;
	stCanPacket *pOutCanPacket = (stCanPacket*)pBuff;
	unsigned int  uiCanId;
	unsigned char ucCanSentLen = 0;
    uint32_t ulTempTime = 0;

    //message	= ( stMsgClst* )osPoolCAlloc( hFdcanMsgPool );
    message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return ucCanSentLen;
	}
    packet	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return ucCanSentLen;
	}

#ifdef CANFD_qhyek //Q_hyek CANFD
    if( !pOutCanPacket->stNormalPacket.ucIDE )
	{
		if( g_ucCanformat == CAN_FRAMEFORMAT_CLASSIC )
		{
#else //Q_hyek CANFD
	if( !pOutCanPacket->stNormalPacket.ucIDE )
	{
		if( !pOutCanPacket->stFDStdPacket.ucEDL )
		{
#endif
			uiCanId = pOutCanPacket->stNormalPacket.us11BitID;
			packet->mLen			= pOutCanPacket->stNormalPacket.ucDLC;
			memcpy(&packet->mData[0], pOutCanPacket->stNormalPacket.arrDataFields, pOutCanPacket->stNormalPacket.ucDLC);
			makeTxHeaderCAN( &packet->mTxHeader, uiCanId, CAN_IDTYPE_STANDARD, packet->mLen, CAN_FRAMEFORMAT_CLASSIC );
			ucCanSentLen = pOutCanPacket->stNormalPacket.ucDLC;
		}
		else
		{
			uiCanId = pOutCanPacket->stFDStdPacket.us11BitID;
			packet->mLen			= pOutCanPacket->stFDStdPacket.ucDLC;
			memcpy(&packet->mData[0], pOutCanPacket->stFDStdPacket.arrDataFields, pOutCanPacket->stFDStdPacket.ucDLC);
			makeTxHeaderCAN( &packet->mTxHeader, uiCanId, CAN_IDTYPE_STANDARD, packet->mLen, CAN_FRAMEFORMAT_FDCAN );
			ucCanSentLen = pOutCanPacket->stFDStdPacket.ucDLC;
		}
	}
	else
	{
		if( !pOutCanPacket->stFDExtPacket.ucEDL )
		{
			uiCanId = pOutCanPacket->stExtendPacket.us11BitID<<18 | pOutCanPacket->stExtendPacket.us18BitID;
			packet->mLen			= pOutCanPacket->stExtendPacket.ucDLC;
			memcpy(&packet->mData[0], pOutCanPacket->stExtendPacket.arrDataFields, pOutCanPacket->stExtendPacket.ucDLC);
			makeTxHeaderCAN( &packet->mTxHeader, uiCanId, CAN_IDTYPE_EXTENDED, packet->mLen, CAN_FRAMEFORMAT_CLASSIC );
			ucCanSentLen = pOutCanPacket->stExtendPacket.ucDLC;
		}
		else
		{
			uiCanId = pOutCanPacket->stFDExtPacket.us11BitID<<18 | pOutCanPacket->stFDExtPacket.us18BitID;
			packet->mLen			= pOutCanPacket->stFDExtPacket.ucDLC;
			memcpy(&packet->mData[0], pOutCanPacket->stFDExtPacket.arrDataFields, pOutCanPacket->stExtendPacket.ucDLC);
			makeTxHeaderCAN( &packet->mTxHeader, uiCanId, CAN_IDTYPE_EXTENDED, packet->mLen, CAN_FRAMEFORMAT_FDCAN );
			ucCanSentLen = pOutCanPacket->stFDExtPacket.ucDLC;
		}
	}

	if(wParam == 2)	//CAN_CHANNEL_2
	{
#ifdef VCI_III_USB_HS
		packet->pTarget		= &hfdcan1;
#else
    	packet->pTarget		= &hfdcan2;
#endif
	}
	else	//CAN_CHANNEL_1
	{
#ifdef USE_INTERNAL_CAN_ONLY
		packet->pTarget		= &hfdcan1;
#else
		packet->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
	}

    message->pPacket	= (void *)packet;
    
    if( g_bSendPeriodicFlag == true )
    {
        ulTempTime = Get_TmrDelta(Get_Tmr(), g_uiSendPeriodicOldTime);
        if( ulTempTime < 55 )
        {
            osDelay(55-ulTempTime);
            GLogN("%d\r\n", (55-ulTempTime));
        }
        g_bSendPeriodicFlag = false;
    }

	if(osMessageAvailableSpace(hFDTxMsg) == 0)
	{
		osPoolFree( hFdcanPktPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hFDTxMsg, (uint32_t)message, osWaitForever );
	}
	return ucCanSentLen;
}
 void OemWriteCanBuff1(unsigned char* pBuff, unsigned char ch)
 {
	 stFdcanPkt  *CANpacket;
	 stMsgClst	 *CANmessage;
	 
	 uint32_t CanID;
	 
	 CANmessage  = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	 if( CANmessage == NULL )
	 {
		 GLogE( "OemWriteCanBuff1 Error... CANmessage fail alloc message!!!\r\n" );
		 return;
	 }
 
	 CANpacket	 = ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
	 if( CANpacket == NULL )
	 {
		 osPoolFree( hMsgPool, (void *)CANmessage );
		 GLogE( "OemWriteCanBuff1 Error... CANpacket fail alloc message!!!\r\n" );
		 return;
	 }
	 
	 CANpacket->pTarget 	 = &hfdcan1;

	 CanID = (pBuff[0]<<8) + pBuff[1];
	 CANpacket->mLen = pBuff[2];
	 memcpy(&CANpacket->mData[0], &pBuff[3], pBuff[2]);
	 
	 makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_STANDARD, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );
 
	 CANmessage->pPacket = (void *)CANpacket;
 
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

U8 OemReadCanBuff(U8 *pRxBuff, U32 timeout)
{
	osEvent		evt;
	stCanPacket *pRxCANPacket = (stCanPacket*)pRxBuff;

	stMsgClst	*message;
	stFdcanPkt	*packet;
	
	int         nCurTime = 0;
	int         nMsgCount = 0;
    
    uint8_t     res = 0;
	
#if defined ( SAVE_CAN_LOG )
	uint32_t	can_id;
	uint16_t	size;
#endif

#ifdef GITCAN_DEBUG_FUNC
	GLogN( "[GITCAN] +%s\r\n", __FUNCTION__ );
#endif

#if true
	do
	{
		nMsgCount = FDCAN_MESSAGE_QUEUE_SIZE - osMessageAvailableSpace(hFDRxMsg);
		
		for(int i=0;i<nMsgCount;i++)
		{		
			evt = osMessageGet( hFDRxMsg, 0);
			if( evt.status == osEventMessage )
			{
#ifdef PRINT_MESSAGE_ID
				printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
		        //LED_GREEN_TOGGLE;//VHC_LED_TOGGLE;
			  	if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
				{}
				else
				{
				  	LED_SetState(eLED_DIAG_COMM, 1000, 100);
				}
				message = ( stMsgClst * )evt.value.p;
				packet	= (stFdcanPkt*)message->pPacket;

				if( g_bCanLogOnRxFlag == true )
				{
					int ii=0;
					if( packet->mRxHeader.IdType==FDCAN_STANDARD_ID)	GLogN("Rx: %04X ",packet->mRxHeader.Identifier);
					else												GLogN("Rx: %08X ",packet->mRxHeader.Identifier);
	
					for(ii=0;ii<packet->mLen;ii++)
					{
						GLogN("%02X ",packet->mData[ii]);
					}
					GLogN("\r\n");
				}

				if ( packet->mRxHeader.IdType == FDCAN_STANDARD_ID )
				{//packet->mData[0]
				//GLogN( "[%02X][%02X]\r\n", packet->mData[0],packet->mData[1] );
				
					if( packet->mRxHeader.FDFormat == FDCAN_CLASSIC_CAN )
					{
						pRxCANPacket->stNormalPacket.ucSOF 		= 1;
						pRxCANPacket->stNormalPacket.us11BitID	= packet->mRxHeader.Identifier;
						pRxCANPacket->stNormalPacket.ucRTR		= 0;
						pRxCANPacket->stNormalPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
						pRxCANPacket->stNormalPacket.ucReserved	= 0;
						pRxCANPacket->stNormalPacket.ucDLC		= packet->mLen;
						memcpy(pRxCANPacket->stNormalPacket.arrDataFields, packet->mData, packet->mLen);
						pRxCANPacket->stNormalPacket.usCRC		= 0;
						pRxCANPacket->stNormalPacket.ucCRCDelimiter = 0;
						pRxCANPacket->stNormalPacket.ucACK		= 0;
						pRxCANPacket->stNormalPacket.ucACKDelimiter = 0;
						pRxCANPacket->stNormalPacket.ucEOF		= CAN_FRAME_EOF;
					}
					else
					{
						pRxCANPacket->stFDStdPacket.ucSOF 		= 1;
						pRxCANPacket->stFDStdPacket.us11BitID	= packet->mRxHeader.Identifier;
						pRxCANPacket->stFDStdPacket.ucReserved1	= 0;
						pRxCANPacket->stFDStdPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
						pRxCANPacket->stFDStdPacket.ucEDL		= ON_FDCAN_FDFORMAT(packet->mRxHeader.FDFormat);
						pRxCANPacket->stFDStdPacket.ucReserved0	= 0;
						pRxCANPacket->stFDStdPacket.ucBRS		= ON_FDCAN_BRS(packet->mRxHeader.BitRateSwitch);
						pRxCANPacket->stFDStdPacket.ucESI		= ON_FDCAN_ESI(packet->mRxHeader.ErrorStateIndicator);
						pRxCANPacket->stFDStdPacket.ucDLC		= packet->mLen;
						memcpy(pRxCANPacket->stFDStdPacket.arrDataFields, packet->mData, packet->mLen);
						pRxCANPacket->stFDStdPacket.usCRC		= 0;
						pRxCANPacket->stFDStdPacket.ucCRCDelimiter = 0;
						pRxCANPacket->stFDStdPacket.ucACK		= 0;
						pRxCANPacket->stFDStdPacket.ucACKDelimiter = 0;
						pRxCANPacket->stFDStdPacket.ucEOF		= CAN_FRAME_EOF;
					}
				}
				else
				{
				//GLogN( "f[%02X][%02X]\r\n", packet->mData[0],packet->mData[1] );
					if( packet->mRxHeader.FDFormat == FDCAN_CLASSIC_CAN )
					{
						pRxCANPacket->stExtendPacket.ucSOF 		= 1;
						pRxCANPacket->stExtendPacket.us11BitID	= packet->mRxHeader.Identifier >> 18;
						pRxCANPacket->stExtendPacket.ucSRR		= 1;
						pRxCANPacket->stExtendPacket.ucIDE		= CAN_FRAME_EXTEND_IDE;
						pRxCANPacket->stExtendPacket.us18BitID	= packet->mRxHeader.Identifier;
						pRxCANPacket->stExtendPacket.ucRTR		= 0;
						pRxCANPacket->stExtendPacket.ucReserved	= 0;
						pRxCANPacket->stExtendPacket.ucDLC		= packet->mLen;
						memcpy(pRxCANPacket->stExtendPacket.arrDataFields, packet->mData, packet->mLen);
						pRxCANPacket->stExtendPacket.usCRC		= 0;
						pRxCANPacket->stExtendPacket.ucCRCDelimiter = 0;
						pRxCANPacket->stExtendPacket.ucACK		= 0;
						pRxCANPacket->stExtendPacket.ucACKDelimiter = 0;
						pRxCANPacket->stExtendPacket.ucEOF		= CAN_FRAME_EOF;
					}
					else
					{
						pRxCANPacket->stFDExtPacket.ucSOF 		= 1;
						pRxCANPacket->stFDExtPacket.us11BitID	= packet->mRxHeader.Identifier >> 18;
						pRxCANPacket->stFDExtPacket.ucSRR		= 1;
						pRxCANPacket->stFDExtPacket.ucIDE		= CAN_FRAME_EXTEND_IDE;
						pRxCANPacket->stFDExtPacket.us18BitID	= packet->mRxHeader.Identifier;
						pRxCANPacket->stFDExtPacket.ucReserved1 = 0;
						pRxCANPacket->stFDExtPacket.ucEDL		= ON_FDCAN_FDFORMAT(packet->mRxHeader.FDFormat);
						pRxCANPacket->stFDExtPacket.ucReserved0 = 0;
						pRxCANPacket->stFDExtPacket.ucBRS		= ON_FDCAN_BRS(packet->mRxHeader.BitRateSwitch);
						pRxCANPacket->stFDExtPacket.ucESI		= ON_FDCAN_ESI(packet->mRxHeader.ErrorStateIndicator);
						pRxCANPacket->stFDExtPacket.ucDLC		= packet->mLen;
						memcpy(pRxCANPacket->stFDExtPacket.arrDataFields, packet->mData, packet->mLen);
						pRxCANPacket->stFDExtPacket.usCRC		= 0;
						pRxCANPacket->stFDExtPacket.ucCRCDelimiter = 0;
						pRxCANPacket->stFDExtPacket.ucACK		= 0;
						pRxCANPacket->stFDExtPacket.ucACKDelimiter = 0;
						pRxCANPacket->stFDExtPacket.ucEOF		= CAN_FRAME_EOF;
					}
				}
				osPoolFree( hFdcanPktPool, (void *)packet );
				osPoolFree( hMsgPool, (void *)message );
		
				if((g_ulProtocolID == ISO14229_ES95486_02_100_NEW)||	  //20190918 Jay
#ifdef HOTA
                    (g_ulProtocolID == ISO14229_ES95486_02_HOTA)||
#endif
                      
#ifdef CANFD_PROTOCOL
                    (g_ulProtocolID == ISO14229_ES95486_02_130_CANFD)||
                    (g_ulProtocolID == ISO14229_ES95486_02_131_CANFD)||
#endif
					(g_ulProtocolID == ISO14229_ES95486_02_100)||
					(g_ulProtocolID == ISO14229_ES95486_02_102)||
					(g_ulProtocolID == ISO14229_ES95486_02_103)||
					(g_ulProtocolID == ISO14229_ES95486_02_104)||
					(g_ulProtocolID == ISO14229_ES95486_02_105)||
					(g_ulProtocolID == ISO14229_ES95486_02_106)||
					(g_ulProtocolID == ISO14229_ES95486_02_107)||
					(g_ulProtocolID == ISO14229_ES95486_02_108)||
					(g_ulProtocolID == ISO14229_ES95486_02_109)||					
					(g_ulProtocolID == ISO14230_ES95486_DOIP_120)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_121)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_122)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_123)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_124)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_125)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_126)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_127)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_128)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_129)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_12A)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_12B)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_12C)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_12D)||
					(g_ulProtocolID == ISO14230_ES95486_DOIP_12E)||
					(g_ulProtocolID == ISO14229_ES95486_170))		//TX CAN ID +8 만 수신처리 20220409 KKT
				{
					if( (packet->mRxHeader.Identifier != g_usES95486_RxCANID) && ( g_bCanIdSwFilterEn == true ))
					{
						res = 0;
					}
					else
					{
						//return 1;	// i got what i want message
                        res = 1;
					}
				}
#ifdef NEW_29BIT_CAN
                else if((g_ulProtocolID == ISO15765_ES95486_29bit)
						||(g_ulProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
                        ||(g_ulProtocolID == ISO15765_ES95486_135_29bit_CANFD)
                        ||(g_ulProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
						)
                {
                    if(packet->mRxHeader.Identifier != g_us29bitES95486_RxCANID)
                    {
                        if(g_bCanIdSwFilterEn == false) res = 1;
                    }
                    else
                    {
                        res = 1;
                    }
                } 
#endif
				else if(g_ulProtocolID == ISO14229_ES95486_02_10F || g_ulProtocolID == ISO14230_ES95486_DOIP_12F)
				{
					if (((packet->mRxHeader.IdType == FDCAN_STANDARD_ID && packet->mRxHeader.Identifier == g_usES95486_RxCANID)			||
						 (packet->mRxHeader.IdType == FDCAN_EXTENDED_ID && packet->mRxHeader.Identifier == g_us29bitES95486_RxCANID))	&&
						(g_bCanIdSwFilterEn == true))
					{
						res = 1;
					}
					else if (g_bCanIdSwFilterEn == false)
					{
						res = 1;
					}
					else
					{
						res = 0;
					}
				}
				else if((g_ulProtocolID == ISO15765_29BIT)||
						(g_ulProtocolID == ISO15765_29BIT_EXCEPT)||
						(g_ulProtocolID == ISO15765_29BIT_REPRO_PENNIMG_TIME)||
						(g_ulProtocolID == ISO15765_CARB_29BIT))
				{
					if( Check_PGN(packet,g_ulPGN) == TRUE )	{res = 1;}//return 1;}
				}
				else
				{
					//return 1;
                    res = 1;
				}
                
                if(res == 1) 
                {
#if defined ( SAVE_CAN_LOG )
                    if ((g_ucCANLog_Enable == 1) && (GetCurFwServiceMode() == eApp_VCI_2))
                    {						  
                        can_id = packet->mRxHeader.Identifier;
                        size = (uint16_t)(packet->mLen);
                        WriteCANLog ( can_id, packet->mData, size );
                    }
#endif
                    return res;
                }
			}
		}
		
		if( nCurTime >= timeout )
		{
			//GLogN( "EndnCurTime:%d\r\n", nCurTime );
#ifndef USE_RELAY_MOSA
			GLogN( "%s]Timeout:%d\r\n", __FUNCTION__, nCurTime );
            if(g_ulProtocolID==J1939_4PGN) intTxdRxdCount=0;
#endif
			return 0;	//RX FAIL
		}
		nCurTime++;
		osDelay(1);
	}while(1);
	return 0;

#else
	evt = osMessageGet( hFDRxMsg, timeout);
	if( evt.status == osEventMessage )
	{
#ifdef PRINT_MESSAGE_ID
		printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
        //LED_GREEN_TOGGLE;//VHC_LED_TOGGLE;
	  	if( gsFwInfo.mucCurrentMode == eApp_Inside || gsFwInfo.mucCurrentMode == eApp_VCI_II_PDI)
		{}
		else
		{
		  	LED_SetState(eLED_DIAG_COMM, 1000, 100);
		}
		message = ( stMsgClst * )evt.value.p;
		packet	= (stFdcanPkt*)message->pPacket;

#if defined ( SAVE_CAN_LOG )
		if ((g_ucCANLog_Enable == 1) && (GetCurFwServiceMode() == eApp_VCI_2))
		{						  
			can_id = packet->mRxHeader.Identifier;
			size = (uint16_t)(packet->mLen);
			WriteCANLog ( can_id, packet->mData, size );
		}
#endif

		if ( packet->mRxHeader.IdType == FDCAN_STANDARD_ID )
		{//packet->mData[0]
		//GLogN( "[%02X][%02X]\r\n", packet->mData[0],packet->mData[1] );
		
			if( packet->mRxHeader.FDFormat == FDCAN_CLASSIC_CAN )
			{
				pRxCANPacket->stNormalPacket.ucSOF 		= 1;
				pRxCANPacket->stNormalPacket.us11BitID	= packet->mRxHeader.Identifier;
				pRxCANPacket->stNormalPacket.ucRTR		= 0;
				pRxCANPacket->stNormalPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
				pRxCANPacket->stNormalPacket.ucReserved	= 0;
				pRxCANPacket->stNormalPacket.ucDLC		= packet->mLen;
				memcpy(pRxCANPacket->stNormalPacket.arrDataFields, packet->mData, packet->mLen);
				pRxCANPacket->stNormalPacket.usCRC		= 0;
				pRxCANPacket->stNormalPacket.ucCRCDelimiter = 0;
				pRxCANPacket->stNormalPacket.ucACK		= 0;
				pRxCANPacket->stNormalPacket.ucACKDelimiter = 0;
				pRxCANPacket->stNormalPacket.ucEOF		= CAN_FRAME_EOF;
			}
			else
			{
				pRxCANPacket->stFDStdPacket.ucSOF 		= 1;
				pRxCANPacket->stFDStdPacket.us11BitID	= packet->mRxHeader.Identifier;
				pRxCANPacket->stFDStdPacket.ucReserved1	= 0;
				pRxCANPacket->stFDStdPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
				pRxCANPacket->stFDStdPacket.ucEDL		= ON_FDCAN_FDFORMAT(packet->mRxHeader.FDFormat);
				pRxCANPacket->stFDStdPacket.ucReserved0	= 0;
				pRxCANPacket->stFDStdPacket.ucBRS		= ON_FDCAN_BRS(packet->mRxHeader.BitRateSwitch);
				pRxCANPacket->stFDStdPacket.ucESI		= ON_FDCAN_ESI(packet->mRxHeader.ErrorStateIndicator);
				pRxCANPacket->stFDStdPacket.ucDLC		= packet->mLen;
				memcpy(pRxCANPacket->stFDStdPacket.arrDataFields, packet->mData, packet->mLen);
				pRxCANPacket->stFDStdPacket.usCRC		= 0;
				pRxCANPacket->stFDStdPacket.ucCRCDelimiter = 0;
				pRxCANPacket->stFDStdPacket.ucACK		= 0;
				pRxCANPacket->stFDStdPacket.ucACKDelimiter = 0;
				pRxCANPacket->stFDStdPacket.ucEOF		= CAN_FRAME_EOF;
			}
		}
		else
		{
		//GLogN( "f[%02X][%02X]\r\n", packet->mData[0],packet->mData[1] );
			if( packet->mRxHeader.FDFormat == FDCAN_CLASSIC_CAN )
			{
				pRxCANPacket->stExtendPacket.ucSOF 		= 1;
				pRxCANPacket->stExtendPacket.us11BitID	= packet->mRxHeader.Identifier >> 18;
				pRxCANPacket->stExtendPacket.ucSRR		= 1;
				pRxCANPacket->stExtendPacket.ucIDE		= CAN_FRAME_EXTEND_IDE;
				pRxCANPacket->stExtendPacket.us18BitID	= packet->mRxHeader.Identifier;
				pRxCANPacket->stExtendPacket.ucRTR		= 0;
				pRxCANPacket->stExtendPacket.ucReserved	= 0;
				pRxCANPacket->stExtendPacket.ucDLC		= packet->mLen;
				memcpy(pRxCANPacket->stExtendPacket.arrDataFields, packet->mData, packet->mLen);
				pRxCANPacket->stExtendPacket.usCRC		= 0;
				pRxCANPacket->stExtendPacket.ucCRCDelimiter = 0;
				pRxCANPacket->stExtendPacket.ucACK		= 0;
				pRxCANPacket->stExtendPacket.ucACKDelimiter = 0;
				pRxCANPacket->stExtendPacket.ucEOF		= CAN_FRAME_EOF;
			}
			else
			{
				pRxCANPacket->stFDExtPacket.ucSOF 		= 1;
				pRxCANPacket->stFDExtPacket.us11BitID	= packet->mRxHeader.Identifier >> 18;
				pRxCANPacket->stFDExtPacket.ucSRR		= 1;
				pRxCANPacket->stFDExtPacket.ucIDE		= CAN_FRAME_EXTEND_IDE;
				pRxCANPacket->stFDExtPacket.us18BitID	= packet->mRxHeader.Identifier;
				pRxCANPacket->stFDExtPacket.ucReserved1 = 0;
				pRxCANPacket->stFDExtPacket.ucEDL		= ON_FDCAN_FDFORMAT(packet->mRxHeader.FDFormat);
				pRxCANPacket->stFDExtPacket.ucReserved0 = 0;
				pRxCANPacket->stFDExtPacket.ucBRS		= ON_FDCAN_BRS(packet->mRxHeader.BitRateSwitch);
				pRxCANPacket->stFDExtPacket.ucESI		= ON_FDCAN_ESI(packet->mRxHeader.ErrorStateIndicator);
				pRxCANPacket->stFDExtPacket.ucDLC		= packet->mLen;
				memcpy(pRxCANPacket->stFDExtPacket.arrDataFields, packet->mData, packet->mLen);
				pRxCANPacket->stFDExtPacket.usCRC		= 0;
				pRxCANPacket->stFDExtPacket.ucCRCDelimiter = 0;
				pRxCANPacket->stFDExtPacket.ucACK		= 0;
				pRxCANPacket->stFDExtPacket.ucACKDelimiter = 0;
				pRxCANPacket->stFDExtPacket.ucEOF		= CAN_FRAME_EOF;
			}
		}
		osPoolFree( hFdcanPktPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );

		if((g_ulProtocolID == ISO14229_ES95486_02_100_NEW)||	  //20190918 Jay
#ifdef HOTA
            (g_ulProtocolID == ISO14229_ES95486_02_HOTA)||
#endif
              
#ifdef CANFD_PROTOCOL
            (g_ulProtocolID == ISO14229_ES95486_02_130_CANFD)||
            (g_ulProtocolID == ISO14229_ES95486_02_131_CANFD)||
#endif
			(g_ulProtocolID == ISO14229_ES95486_02_100)||
			(g_ulProtocolID == ISO14229_ES95486_02_102)||
			(g_ulProtocolID == ISO14229_ES95486_02_103)||
			(g_ulProtocolID == ISO14229_ES95486_02_104)||
			(g_ulProtocolID == ISO14229_ES95486_02_105)||
			(g_ulProtocolID == ISO14229_ES95486_02_106)||
			(g_ulProtocolID == ISO14229_ES95486_02_107)||
			(g_ulProtocolID == ISO14229_ES95486_02_108)||
			(g_ulProtocolID == ISO14229_ES95486_02_109)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_120)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_121)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_122)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_123)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_124)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_125)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_126)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_127)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_128)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_129)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_12A)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_12B)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_12C)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_12D)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_12E)||
			(g_ulProtocolID == ISO14230_ES95486_DOIP_12F)||
			(g_ulProtocolID == ISO14229_ES95486_170))		//TX CAN ID +8 만 수신처리 20220409 KKT
		{
		//GLogI( "g_ulProtocolID:0x%X,packet->mRxHeader.Identifier:%X\r\n",g_ulProtocolID,packet->mRxHeader.Identifier );
			//if((MASK_CANID>0x00)&&(MASK_CANID < 0x0700))
		  	if( packet->mRxHeader.Identifier != g_usES95486_RxCANID  )
			{
				if(((g_stGITSetConfig.nEtc5>0x00)&&(g_stGITSetConfig.nEtc5 < 0x0700)) || ( g_bCanIdSwFilterEn == false ))
				{
					// if DB MASK VALUE exist 
					//For the same processing as VCI2
					return 1;
				}
				else
				{
					//GLogI( "ES95486 SW_Filtered, g_usES95486_RxCANID:0x%04X\r\n",g_usES95486_RxCANID );
					//GLogI( "?");
					return 2;
				}
			}
		}
		return 1;
	}
	return 0;
#endif
}

/*****************************************************************************
   CAN_CHANNEL	: 1 = FDCAN1, 2 = FDCAN2
   HighCan		: 1 = HighCan, 2 = LowCan
   Can_BPS		: Can nominal bit time
   format		: 0 = FDCAN, 1 = Classic Can

   < Sequence >
   1. Set Bit Timing
   2. Set Filter(X)
   3. Start FDCAN Hal( & GPIO Path )
 *****************************************************************************/
void CAN_Initial_CH( U8 CAN_CHANNEL, U8 HighCan, U8 Can_BPS, U8 format, U8 rxfifo, u8 CANIDType, u8 MaskNum, u32 *StartMaskValue, u32 *EndMaskValue)
{
    uint8_t ucDataBaud = 0;
    uint8_t ucNominalBaud = 0;
#ifdef GITCAN_DEBUG_FUNC
	GLogN( "[GITCAN] +%s\r\n", __FUNCTION__ );
#endif

	clearRXCanMessage();
	clearTXCanMessage();

	if( CAN_CHANNEL == 1 )
	{
		if( HighCan == 1 )												// High CAN1 Set
		{
#ifdef USE_INTERNAL_CAN_ONLY
            
#ifdef CANFD_PROTOCOL
			ucNominalBaud = Can_BPS;
            ucDataBaud = CAN_DAT_2MBPS;
            
            if((g_ulProtocolID == ISO14229_ES95486_02_130_CANFD ) ||
               (g_ulProtocolID == ISO14229_ES95486_02_131_CANFD ) ||
               (g_ulProtocolID == ISO15765_ES95486_135_29bit_CANFD ) ||
               (g_ulProtocolID == ISO15765_ES95486_136_29bit_CANFD ) )
            {
                SelCANFDLine(Can_BPS, &ucNominalBaud, &ucDataBaud);
                //Can_BPS = (Can_BPS-5)/3;
                //ucDataBaud = Can_BPS%3 + 5;
            }
            else if( (Can_BPS>=5) && (Can_BPS<=19) )
            {
                ucNominalBaud = CAN_500KBPS;
                ucDataBaud = CAN_DAT_2MBPS;
            }
            
            CanSet_Baud( &hfdcan1, ucNominalBaud, ucDataBaud, format );
#else
			CanSet_Baud( &hfdcan1, Can_BPS, CAN_DAT_2MBPS, format );
#endif
			CAN_Channel_Masket_Set(&hfdcan1, rxfifo, CANIDType, MaskNum, StartMaskValue,EndMaskValue);
            //DLC_CH_Set( g_stGITHWSetData.nKlineCh , g_stGITHWSetData.nLlineCh );//no need : [GDSN-11574]
			if( Can_BPS == CAN_100KBPS )
			{
				startFDCan( 0, 0, 1 );
			}
			else
			{
				if( g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH01 && g_stGITHWSetData.nLlineCh == KL_LINE2_CONNECT_CH09 )
					startFDCan( 0, 1, 0 );
				else
					startFDCan( 1, 0, 0 );
			}
#else
			mcp2518fd_set_baud(Can_BPS, CAN_DAT_2MBPS, format );
			// 220319 lee1008 test
			//CAN_Channel_Masket_Set(&hfdcan2, rxfifo, CANIDType, MaskNum, StartMaskValue,EndMaskValue);
			if( CANIDType == STANDARD_CAN )		SetCanMaskingMCP2518(*StartMaskValue,*EndMaskValue);
			else								SetCanMaskingMCP2518_EXT(*StartMaskValue,*EndMaskValue);
			DLC_CH_Set( g_stGITHWSetData.nKlineCh , g_stGITHWSetData.nLlineCh );
			//obd 6,14 고정
			startFDCan( 1, 0, 0 );
#endif
		}
	}
	else if( CAN_CHANNEL == 2 )
	{
		if( HighCan == 1 )												// High CAN2 Set
		{
#ifdef CANFD_PROTOCOL
			ucNominalBaud = Can_BPS;
            ucDataBaud = CAN_DAT_2MBPS;
            
            if((g_ulProtocolID == ISO14229_ES95486_02_130_CANFD ) ||
               (g_ulProtocolID == ISO14229_ES95486_02_131_CANFD ) ||
               (g_ulProtocolID == ISO15765_ES95486_135_29bit_CANFD ) ||
               (g_ulProtocolID == ISO15765_ES95486_136_29bit_CANFD ) )
            {
                SelCANFDLine(Can_BPS, &ucNominalBaud, &ucDataBaud);
                //Can_BPS = (Can_BPS-5)/3;
                //ucDataBaud = Can_BPS%3 + 5;
            }
            else if( (Can_BPS>=5) && (Can_BPS<=19) )
            {
                ucNominalBaud = CAN_500KBPS;
                ucDataBaud = CAN_DAT_2MBPS;
            }
            
            CanSet_Baud( &hfdcan1, ucNominalBaud, ucDataBaud, format );
#else
			CanSet_Baud( &hfdcan1, Can_BPS, CAN_DAT_2MBPS, format );
#endif
			CAN_Channel_Masket_Set(&hfdcan1, rxfifo, CANIDType, MaskNum, StartMaskValue,EndMaskValue);
			//DLC_CH_Set( g_stGITHWSetData.nKlineCh , g_stGITHWSetData.nLlineCh );//no need : [GDSN-11574]
			//startFDCan( &hfdcan2, 0 );
			//SetKL_Line( KL_LINE1_CONNECT_CH06, KL_LINE2_CONNECT_CH14 );
			//obd 1,9 고정
			startFDCan( 0, 1, 0 );
		}
		else															// Low CAN Set
		{
#ifdef CANFD_PROTOCOL
			ucNominalBaud = Can_BPS;
            ucDataBaud = CAN_DAT_2MBPS;
            
            if((g_ulProtocolID == ISO14229_ES95486_02_130_CANFD ) ||
               (g_ulProtocolID == ISO14229_ES95486_02_131_CANFD ) ||
               (g_ulProtocolID == ISO15765_ES95486_135_29bit_CANFD ) ||
               (g_ulProtocolID == ISO15765_ES95486_136_29bit_CANFD ) )
            {
                SelCANFDLine(Can_BPS, &ucNominalBaud, &ucDataBaud);
                //Can_BPS = (Can_BPS-5)/3;
                //ucDataBaud = Can_BPS%3 + 5;
            }
            else if( (Can_BPS>=5) && (Can_BPS<=19) )
            {
                ucNominalBaud = CAN_500KBPS;
                ucDataBaud = CAN_DAT_2MBPS;
            }
            
            CanSet_Baud( &hfdcan1, ucNominalBaud, ucDataBaud, format );
#else
			CanSet_Baud( &hfdcan1, Can_BPS, CAN_DAT_2MBPS, format );
#endif
			CAN_Channel_Masket_Set(&hfdcan1, rxfifo, CANIDType, MaskNum, StartMaskValue,EndMaskValue);
			//DLC_CH_Set( g_stGITHWSetData.nKlineCh , g_stGITHWSetData.nLlineCh );//no need : [GDSN-11574]
			//startFDCan( &hfdcan2, 1 );
			//SetKL_Line( KL_LINE1_CONNECT_CH06, KL_LINE2_CONNECT_CH14 );
			//obd 1,9 고정
			startFDCan( 0, 0, 1 );
		}
	}
}
unsigned char Oem_CAN_Channel_Masket_Set(unsigned char nChannel, unsigned char nCANIDType, unsigned char nMaskNum, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue)
{//need to test - kkt
	unsigned char ucReturn;
	clearRXCanMessage();
	clearTXCanMessage();
#ifdef USE_INTERNAL_CAN_ONLY
	HAL_FDCAN_Stop( &hfdcan1 );
	ucReturn = CAN_Channel_Masket_Set(&hfdcan1, nChannel, nCANIDType, nMaskNum, pStartMaskValue,pEndMaskValue);
	HAL_FDCAN_Start( &hfdcan1 );
#else
	if( nChannel == 1 )
	{
		StopSpiCan();
		//StopSpiThread();
		//mcp2518fd_set_baud(Can_BPS, CAN_DAT_2MBPS, format );
		SetCanMaskingMCP2518(*pStartMaskValue,*pEndMaskValue);
		ucReturn=HAL_OK;
		StartSpiCan();
		//StartSpiThread();
	}
	else
	{
		HAL_FDCAN_Stop( &hfdcan1 );
		ucReturn = CAN_Channel_Masket_Set(&hfdcan1, nChannel, nCANIDType, nMaskNum, pStartMaskValue,pEndMaskValue);
		HAL_FDCAN_Start( &hfdcan1 );
	}
#endif
	return ucReturn;
}

/*****************************************************************************
   nRxFifo	        : 1 = fifo0, 2 = fifo1
   nCANIDType	    : 1 = 11bit can, 2 = extended can
   nMaskNum		    : filter number
   pStartMaskValue	: filter ID
   pEndMaskValue	: filter ID
 *****************************************************************************/
#if 0
// LJS: Support both 11bit and 29bit CAN ID (ES95486 0x10F/0x12F)
u8 CAN_Channel_Masket_Set(FDCAN_HandleTypeDef *hfdcan, u8 nRxFifo, u8 nCANIDType, u8 nMaskNum, u32 *pStartMaskValue, u32 *pEndMaskValue)
{
	u16 index;
	FDCAN_FilterTypeDef sFilterConfig;

	if( ( g_ulProtocolID == ISO14229_ES95486_02_10F ) || ( g_ulProtocolID == ISO14230_ES95486_DOIP_12F ))
	{
		if(nRxFifo == 1)
		{
			sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
		}
		else
		{
			sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
		}

		// Clear STD filters
		sFilterConfig.IdType = FDCAN_STANDARD_ID;
		for(index=0; index < MAX_CANID_MASK_CNT; index++)
		{
			sFilterConfig.FilterIndex   = index;
			sFilterConfig.FilterID1		= 0;
			sFilterConfig.FilterID2		= 0;
			HAL_FDCAN_ConfigFilter( hfdcan, &sFilterConfig );
		}

		// Clear EXT filters
		sFilterConfig.IdType = FDCAN_EXTENDED_ID;
		for(index=0; index < MAX_CANID_MASK_CNT; index++)
		{
			sFilterConfig.FilterIndex   = index;
			sFilterConfig.FilterID1		= 0;
			sFilterConfig.FilterID2		= 0;
			HAL_FDCAN_ConfigFilter( hfdcan, &sFilterConfig );
		}

		for(index=0; index<nMaskNum; index++)
		{
			if(pStartMaskValue[index] <= 0x7ff)
			{
				sFilterConfig.IdType = FDCAN_STANDARD_ID;
			}
			else
			{
				sFilterConfig.IdType = FDCAN_EXTENDED_ID;
			}
			sFilterConfig.FilterType = FDCAN_FILTER_RANGE;

			sFilterConfig.FilterIndex   = index;
			sFilterConfig.FilterID1		= pStartMaskValue[index];
			sFilterConfig.FilterID2		= pEndMaskValue[index];

			if( HAL_FDCAN_ConfigFilter( hfdcan, &sFilterConfig ) != HAL_OK )
			{
				GLogE( "error... FDCAN HAL_FDCAN_ConfigFilter %d !!!\r\n", index );
				return HAL_ERROR;
			}
		}
	}
	else
	{
		if(nCANIDType == 1)
		{
			sFilterConfig.IdType = FDCAN_STANDARD_ID;
		}
		else
		{
			sFilterConfig.IdType = FDCAN_EXTENDED_ID;
		}

		if(nRxFifo == 1)
		{
			sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
		}
		else
		{
			sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
		}
		
		for(index=0; index < MAX_CANID_MASK_CNT; index++)
		{
			sFilterConfig.FilterIndex   = index;
			sFilterConfig.FilterID1		= 0;//pStartMaskValue[index];
			sFilterConfig.FilterID2		= 0;//pEndMaskValue[index];
			if( HAL_FDCAN_ConfigFilter( hfdcan, &sFilterConfig ) == HAL_OK )
			{
			  GLogI("CAN Filtering[%d] Clear \r\n", index);
			}
		}

		for(index=0; index<nMaskNum; index++)
		{
			if(pStartMaskValue[index] == pEndMaskValue[index])
			{
				sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
			}
			else
			{
#if 1
				sFilterConfig.FilterType = FDCAN_FILTER_RANGE;
#else			// 기존 sja1000, stm32f207에서 range가 없음. 하지만 PC APP 개발자들이 range로 알고 있어서 range로 설정 시도.
				// 추후 디로거 상태파악 후 FDCAN_FILTER_MASK 변경해도 좋
				sFilterConfig.FilterType = FDCAN_FILTER_MASK;
				sFilterConfig.FilterID1	= pStartMaskValue[index]&pEndMaskValue[index];
				sFilterConfig.FilterID2	= ~(pStartMaskValue[index]^pEndMaskValue[index]);
#endif
			}

			sFilterConfig.FilterIndex   = index;
			sFilterConfig.FilterID1		= pStartMaskValue[index];
			sFilterConfig.FilterID2		= pEndMaskValue[index];

			if( HAL_FDCAN_ConfigFilter( hfdcan, &sFilterConfig ) != HAL_OK )
			{
				GLogE( "error... FDCAN HAL_FDCAN_ConfigFilter %d !!!\r\n", index );
				return HAL_ERROR;
			}
		}
	}

	// Config Global Filter
	if( HAL_FDCAN_ConfigGlobalFilter( hfdcan, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE ) != HAL_OK )
	{
		GLogE( "error... FDCAN1 HAL_FDCAN_ConfigGlobalFilter!!!\r\n" );
		return HAL_ERROR;
	}

	// Start Fifo0, Fifo1 IT
	HAL_FDCAN_ActivateNotification( hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0 );
	HAL_FDCAN_ActivateNotification( hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0 );

	return HAL_OK;
}
#else
u8 CAN_Channel_Masket_Set(FDCAN_HandleTypeDef *hfdcan, u8 nRxFifo, u8 nCANIDType, u8 nMaskNum, u32 *pStartMaskValue, u32 *pEndMaskValue)
{
    FDCAN_FilterTypeDef sFilterConfig;
    uint32_t std_filter_idx = 0;
    uint32_t ext_filter_idx = 0;
    uint32_t i;

	memset(&sFilterConfig, 0, sizeof(sFilterConfig));
	
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterConfig = FDCAN_FILTER_DISABLE;
    for(i = 0; i < MAX_CANID_MASK_CNT; i++)
	{
        sFilterConfig.FilterIndex = i;
        HAL_FDCAN_ConfigFilter(hfdcan, &sFilterConfig);
    }

    sFilterConfig.IdType = FDCAN_EXTENDED_ID;
    sFilterConfig.FilterConfig = FDCAN_FILTER_DISABLE;
    for(i = 0; i < MAX_CANID_MASK_CNT; i++)
	{
        sFilterConfig.FilterIndex = i;
        HAL_FDCAN_ConfigFilter(hfdcan, &sFilterConfig);
    }
    
    if(nRxFifo == 1)
	{
        sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    }
	else
	{
        sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
    }

    for(i = 0; i < nMaskNum; i++)
    {
        uint32_t start_id = pStartMaskValue[i];
        uint32_t end_id = pEndMaskValue[i];

        if (start_id <= 0x7FF)
		{
            sFilterConfig.IdType = FDCAN_STANDARD_ID;
            sFilterConfig.FilterIndex = std_filter_idx;
            std_filter_idx++;

            if(std_filter_idx > 128)
			{
                GLogE("Error: Standard filter count exceeded!\r\n");
                return HAL_ERROR;
            }
        }
		else
		{
            sFilterConfig.IdType = FDCAN_EXTENDED_ID;
            sFilterConfig.FilterIndex = ext_filter_idx;
            ext_filter_idx++;
            
            if (ext_filter_idx > 64)
			{
                GLogE("Error: Extended filter count exceeded!\r\n");
                return HAL_ERROR;
            }
        }

        if (start_id == end_id) 
		{
			sFilterConfig.FilterType = FDCAN_FILTER_MASK;
			sFilterConfig.FilterID1 = start_id;
			
			if (sFilterConfig.IdType == FDCAN_STANDARD_ID)
			{
				sFilterConfig.FilterID2 = 0x7FF;
			}
			else
			{
				sFilterConfig.FilterID2 = 0x1FFFFFFF;
			}
		}
		else
		{
            sFilterConfig.FilterType = FDCAN_FILTER_RANGE;
			sFilterConfig.FilterID1 = start_id;
			sFilterConfig.FilterID2 = end_id;
        }

        if (HAL_FDCAN_ConfigFilter(hfdcan, &sFilterConfig) != HAL_OK)
		{
            GLogE("Error... FDCAN ConfigFilter failed at index %d\r\n", i);
            return HAL_ERROR;
        }
    }

    if (HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK)
	{
        GLogE("Error... FDCAN HAL_FDCAN_ConfigGlobalFilter!!!\r\n");
        return HAL_ERROR;
    }

    HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);

    return HAL_OK;
}
#endif
#ifdef PRINT_MESSAGE_ID
char* GetMessageIDName(uint32_t uiIndex)
{
	for(int i=0;i<20;i++)
	{
		if( stMessageIdInfo[i].uiID == uiIndex )
			return stMessageIdInfo[i].ucIdName;
	}
	return "NotFind";
}
#endif

U8 Check_PGN(stFdcanPkt *packet,U32 PGN)		//Can0ReadBuff_29bit
{
	U8 bRet = false;
	U32 CanRxID32=0, PGN_CARB=0;
	
    CanRxID32 = packet->mRxHeader.Identifier;
    
	if(PGN == 0x1234)
	{
		PGN_CARB = 0x18D0F100;
        if( PGN_CARB == (CanRxID32&0xFFF0FF00) )	bRet =  true;
        else										bRet =  false;
	}
	else
	{
		if( PGN == (CanRxID32&0x0000FFFF) )	bRet = true;
		else								bRet = false;
	}
	
	return bRet;
}

void CanTx(uint32_t CanID, unsigned char* pData, unsigned char length, unsigned char* pLog, unsigned char LogLoc)
{
	uint32_t i			= 0;

    stMsgClst	*CANmessage;
	stFdcanPkt	*CANpacket;
	
	CANmessage	= ( stMsgClst* )osPoolCAlloc( hFdcanMsgPool );
	if( CANmessage == NULL )
	{
		GLogE( "Error... fail alloc message!!!\r\n" );
	}

	CANpacket	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
	if( CANpacket == NULL )
	{
		GLogE( "Error... fail alloc Packet!!!\r\n" );
	}
    
    memset(CANpacket->mData, 0x00, 8);
	memcpy(CANpacket->mData, &pData[0], length);

	CANpacket->mLen 	= 8;
	CANpacket->pTarget		= &hfdcan1;

	GLogN( "CAN Tx : %04X ", CanID);
	for(i=0; i<8; i++)
	{
		GLogN( "%02X ",CANpacket->mData[0+i]);
	}
	GLogN( "\r\n" );
    
    /********************Save Log********************/
    pLog[LogLoc] = (CanID>>8)&0xFF;
    pLog[LogLoc+1] = CanID&0xFF;
    memcpy(&pLog[LogLoc+2], CANpacket->mData, 8);
    g_CurLoc+=10;
    /************************************************/

	makeTxHeaderCAN( &CANpacket->mTxHeader, CanID, CAN_IDTYPE_STANDARD, CANpacket->mLen, CAN_FRAMEFORMAT_CLASSIC );

	CANmessage->pPacket = (void *)CANpacket;

	osMessagePut( hFDTxMsg, (uint32_t)CANmessage, osWaitForever );
}

void CanRx(uint32_t *CanID, unsigned char* RxData, unsigned char *length, unsigned char* pLog, unsigned char LogLoc)
{
	uint32_t i			= 0,p3time;
    stMsgClst	*CANmessage;
	stFdcanPkt	*CANpacket;
	osEvent 	evt;
	static unsigned int uOldTimer	= 0;
	/////CAN RX//////////////////////////////////////////////////////
	p3time=1100;
	uOldTimer = Get_Tmr();
    U8 FlowControl[8] = {0x00, };
    U16 MulLen = 0;
    U16 MulCount = 0;
    U16 MulLeftVal = 0;
    
    U16 PacketNum = 0;
    
	for(;;)
	{
        if (Get_TmrDelta(Get_Tmr(),uOldTimer) >= p3time)break;
        
		evt = osMessageGet( hFDRxMsg, 1000 );
		if( evt.status == osEventMessage )
		{
			CANmessage = ( stMsgClst * )evt.value.p;
			CANpacket	= (stFdcanPkt*)CANmessage->pPacket;
            
            *CanID = CANpacket->mRxHeader.Identifier;

            /********************Save Log********************/
            PacketNum = (CANpacket->mLen)/8;
            if((CANpacket->mLen)%8 != 0) PacketNum+=1;
            for(int i=0; i<PacketNum; i++)
            {
                pLog[g_CurLoc] = (CANpacket->mRxHeader.Identifier>>8)&0xFF;
                pLog[g_CurLoc+1] = CANpacket->mRxHeader.Identifier&0xFF;
                
                if((i != PacketNum-1) || PacketNum==1) 
                {
                    memcpy(&pLog[g_CurLoc+2], CANpacket->mData, 8);
                    g_CurLoc+=10;
                }
                else
                {
                    memcpy(&pLog[g_CurLoc+2], CANpacket->mData, (CANpacket->mLen)%8);
                    g_CurLoc+=(CANpacket->mLen)%8;
                }
            }
            /************************************************/

            
			GLogN( "CAN Rx : %04X ", CANpacket->mRxHeader.Identifier);
			for(i=0; i<CANpacket->mLen; i++)
			{
				GLogN( "%02X ",RxData[0+i]);
			}				
			GLogN( "\r\n" );

			if((CANpacket->mData[1]==0x7F)&&(CANpacket->mData[3]==0x78)) 
			{
				GLogN( "rx pending \r\n" );
				uOldTimer = Get_Tmr();
				p3time=5100;
				continue;
			}
            
            if((CANpacket->mData[0]&0xF0)==0x00&&(MulCount==0))
            {
                *length = CANpacket->mLen;
                memcpy(&RxData[0], &CANpacket->mData[0], CANpacket->mLen);
            }
            else if((CANpacket->mData[0]&0xF0)==0x10 &&(MulCount==0))
            {
                memcpy(&RxData[0], &CANpacket->mData[2], 6);
                MulLen = ((CANpacket->mData[0]&0x0F)<<8)+CANpacket->mData[1];
                *length = MulLen;
                
                MulCount = MulLen;
                MulCount -= 6;
                
                FlowControl[0] = 0x30;
                
                CanTx((CANpacket->mRxHeader.Identifier)-0x08, FlowControl, 8, pLog, g_CurLoc);
                g_CurLoc+=10;
                continue;
            }
            else if((CANpacket->mData[0]&0xF0)==0x20 || MulCount!=0)
            {
                if((CANpacket->mData[0]&0xF0)!=0x20)
                {
                    memcpy(&RxData[MulLen-MulCount], &CANpacket->mData[1], 8-MulLeftVal);
                    MulCount -= (8-MulLeftVal);
                    CANpacket->mLen -= (8-MulLeftVal);
                }
                
                for(int i=0; i<(CANpacket->mLen/8)+1; i++)
                {
                    if(i == CANpacket->mLen/8) 
                    {
                      MulLeftVal=CANpacket->mLen%8;
                      memcpy(&RxData[MulLen-MulCount+(7*i)], &CANpacket->mData[1+(i*8)], MulLeftVal);
                      MulCount -= CANpacket->mLen%8;
                    }
                    else 
                    {
                      memcpy(&RxData[MulLen-MulCount+(7*i)], &CANpacket->mData[1+(i*8)], 7);
                      if(MulCount < 8) MulCount = 0;
                      else MulCount -= 7;
                    }
                }
                
                if(MulCount!=0) continue;
                else {}
            }
            
			osPoolFree( hFdcanPktPool, (void *)CANpacket );
			osPoolFree( hFdcanMsgPool, (void *)CANmessage );
			
			break;
		}
	}
}

void CanTx_Multi(uint32_t CanID, unsigned char* pData, uint16_t length, unsigned char* pLog, unsigned char LogLoc)
{
    U32 TxCanId, RxCanId = 0;
    U8 TxCanData[8], RxCanData[8] = {0x00, };
    U8 TxCanLen, RxCanLen = 0;
  
	if(length < 8) 
    {
      GLogE("This Function must be at least 8 data length");
      return;
    }
    
    TxCanId = CanID;
    TxCanData[0] = 0x10 | ((length&0xFF00)>>8);
    TxCanData[1] = length&0x00FF;
    memcpy(&TxCanData[2], &pData[0], 6);
    TxCanLen = 8;

    CanTx(TxCanId, TxCanData, TxCanLen, pLog, g_CurLoc); //First Frame
    CanRx((int*)&RxCanId, RxCanData, (char*)&RxCanLen, pLog, g_CurLoc); //Flow Control
    
    if(RxCanData[0] == 0x30)
    {
        TxCanData[0] = 0x20;
        
        for(int i=0; i<((length-6)/7)+1; i++)
        {
            TxCanData[0]+=1;
            if(TxCanData[0]==0x30) TxCanData[0] = 0x20;
            memcpy(&TxCanData[1], &pData[6+(7*i)], 7);
            
            if(i==((length-6)/7)) memset(&TxCanData[((length-6)%7)+1], 0x00, 7-((length-6)%7));
            
            CanTx(TxCanId, TxCanData, TxCanLen, pLog, g_CurLoc); //Consecutive Frame
        }
    }
}

void SelCANFDLine(U8 BitrateNum, U8 *nominalBitrate, U8 *DataBitrate)
{
    if( BitrateNum == 0 )
    {
        *nominalBitrate = CAN_1MBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 1 )
    {
        *nominalBitrate = CAN_500KBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 2 )
    {
        *nominalBitrate = CAN_250KBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 3 )
    {
        *nominalBitrate = CAN_125KBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 4 )
    {
        *nominalBitrate = CAN_100KBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 5 )
    {
        *nominalBitrate = CAN_500KBPS;
        *DataBitrate = CAN_DAT_1MBPS;
    }
    else if( BitrateNum == 6 )
    {
        *nominalBitrate = CAN_500KBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 7 )
    {
        *nominalBitrate = CAN_500KBPS;
        *DataBitrate = CAN_DAT_4MBPS;
    }
    else if( BitrateNum == 8 )
    {
        *nominalBitrate = CAN_1MBPS;
        *DataBitrate = CAN_DAT_1MBPS;
    }
    else if( BitrateNum == 9 )
    {
        *nominalBitrate = CAN_1MBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 10 )
    {
        *nominalBitrate = CAN_1MBPS;
        *DataBitrate = CAN_DAT_4MBPS;
    }
    else if( BitrateNum == 11 )
    {
        *nominalBitrate = CAN_250KBPS;
        *DataBitrate = CAN_DAT_1MBPS;
    }
    else if( BitrateNum == 12 )
    {
        *nominalBitrate = CAN_250KBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 13 )
    {
        *nominalBitrate = CAN_250KBPS;
        *DataBitrate = CAN_DAT_4MBPS;
    }
    else if( BitrateNum == 14 )
    {
        *nominalBitrate = CAN_125KBPS;
        *DataBitrate = CAN_DAT_1MBPS;
    }
    else if( BitrateNum == 15 )
    {
        *nominalBitrate = CAN_125KBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 16 )
    {
        *nominalBitrate = CAN_125KBPS;
        *DataBitrate = CAN_DAT_4MBPS;
    }
    else if( BitrateNum == 17 )
    {
        *nominalBitrate = CAN_100KBPS;
        *DataBitrate = CAN_DAT_1MBPS;
    }
    else if( BitrateNum == 18 )
    {
        *nominalBitrate = CAN_100KBPS;
        *DataBitrate = CAN_DAT_2MBPS;
    }
    else if( BitrateNum == 19 )
    {
        *nominalBitrate = CAN_100KBPS;
        *DataBitrate = CAN_DAT_4MBPS;
    }
}