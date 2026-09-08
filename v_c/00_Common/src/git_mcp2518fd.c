/*************************************************************
 * NOTE : git_mcp2518fd.c
 *      SPI FDCAN controler
 * Author : Lee junho
 * Since : 2020.04.20
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "gpio.h"
#include "cmsis_os.h"
#include "main.h"
#include "tim.h"

#include "common.h"
#include "sw_timer.h"
#include "typedef.h"
#include "firmware.h"
#include "git_pool.h"
#ifdef VCI3_DIAG
#include "git_mcp2518fd.h"
#endif
#include "led.h"

#include "drv_canfdspi_api.h"
#include "drv_canfdspi_register.h"
#include "drv_spi.h"

#if defined ( SAVE_CAN_LOG )
#include "git_function_list.h"
#include "git_mmc.h"
#endif

#include "git_can.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define SPI_CAN_TX_MSG_Q_SIZE							50
#define SPI_CAN_RX_MSG_Q_SIZE							300
//#define SPI_CAN_RX_MSG_Q_SIZE							900//kkt.

#define SPI_TX_WAIT_TIME								10				// ms
#define SPI_RX_DATA_CHECK_TEST							0

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
static void SpiCanThread1( void const *argument );

static void RxMoniterThread( void const *argument );

int32_t	InitSPICanControler( void );
int32_t	DeinitMCP2518FD( void );
int32_t	InitMCP2518FD( void );
int32_t	StartSpiThread( void );
int32_t	StopSpiThread( void );
void	SetCanBaudrate( void );
void	SetSpiCanSTB( void );
void	StartSpiCan(void);
void	StopSpiCan(void);

static void checkSpiTxWaitCallback1( void );

void Mcp2518fd_RxInt1_handler( void );

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
// message queue
osMessageQId	hTxSpiCanMsg1;

osMessageQId	hRxSpiCanMsg;

osMessageQDef( spicantxmsg, SPI_CAN_TX_MSG_Q_SIZE, MsgClst_t );
osMessageQDef( spicanrxmsg, SPI_CAN_RX_MSG_Q_SIZE, MsgClst_t );

// thread
osThreadId		hSpiCanTh1;

osThreadId		hRxMonTh;

osThreadDef( spicanth1, SpiCanThread1, osPriorityHigh, 0, 3 * configMINIMAL_STACK_SIZE );

//osThreadDef( rxmonth, RxMoniterThread, osPriorityNormal, 0, 2 * configMINIMAL_STACK_SIZE );
osThreadDef( rxmonth, RxMoniterThread, osPriorityHigh, 0, 2 * configMINIMAL_STACK_SIZE );//kkt.


static osMutexId	hSpiMutex1;

osMutexDef( spi1_mutex );

static uint8_t	rxFilterIndex[4]	= { 0, };

static uint8_t	gWaitTimerIndex1;

static uint8_t	gWaitTimerFlag1;

uint32_t	gErrorCountRx1	= 0;

uint32_t	gSpiRxCount1	= 0;

// callback variable
UserIntCallBack_t	rx1_callback;

//SPICAN_HandleTypeDef hspican;

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Initialize
 *--------------------------------------------------------------------*/
int32_t InitSPICanControler( void )
{
#ifdef MCP2518_DEBUG_FUNC
	GLogN( "[MCP2518] +%s\r\n", __FUNCTION__ );
#endif

	// transmit message
	hTxSpiCanMsg1 = osMessageCreate( osMessageQ( spicantxmsg ), NULL );
	if( hTxSpiCanMsg1 == NULL )
	{
		GLogE( "error... osMessageCreate hTxSpiCanMsg1\r\n" );
		return INIT_FAIL;
	}

	// receive message
	hRxSpiCanMsg = osMessageCreate( osMessageQ( spicanrxmsg ), NULL );
	if( hRxSpiCanMsg == NULL )
	{
		GLogE( "error... osMessageCreate hRxSpiCanMsg\r\n" );
		return INIT_FAIL;
	}

	// Mutex
	hSpiMutex1	= osMutexCreate( osMutex( spi1_mutex ) );

	// register callback function
	rx1_callback	= Mcp2518fd_RxInt1_handler;

	// wait timer
	gWaitTimerIndex1 = SetSWTimer( SPI_TX_WAIT_TIME, eSWTimer_NONE, checkSpiTxWaitCallback1, FALSE );

	DRV_SPI_Initialize();

	return INIT_OK;
}

int32_t InitMCP2518FD( void )
{
	CAN_CONFIG			config;

	CAN_TX_FIFO_CONFIG	txConfig;
	CAN_RX_FIFO_CONFIG	rxConfig;

	REG_CiFLTOBJ		fObj;
	REG_CiMASK			mObj;

	CAN_BITTIME_SETUP	selectedBitTime = CAN_500K_2M;

#ifdef MCP2518_DEBUG_FUNC
	GLogN( "[MCP2518] +%s\r\n", __FUNCTION__ );
#endif

	EnableCan2OSC();
	osDelay( 10 );

	// All Transceiver standby mode
	StopSpiCan();

	DRV_CANFDSPI_OperationModeSelect( DRV_CANFDSPI_INDEX_1, CAN_CONFIGURATION_MODE );	//can tx error fix //kkt
	osDelay( 10 );

	// Wakeup
	DRV_CANFDSPI_WakeUp( DRV_CANFDSPI_INDEX_1 );

	//DRV_CANFDSPI_OperationModeSelect( DRV_CANFDSPI_INDEX_1, CAN_CONFIGURATION_MODE );
	//osDelay( 10 );

	// Reset device
	DRV_CANFDSPI_Reset( DRV_CANFDSPI_INDEX_1 );

	osDelay( 100 );

	// Enable ECC and initialize RAM
	DRV_CANFDSPI_EccEnable( DRV_CANFDSPI_INDEX_1 );

	DRV_CANFDSPI_RamInit( DRV_CANFDSPI_INDEX_1, 0xff );

	// Configure device
	DRV_CANFDSPI_ConfigureObjectReset( &config );
	config.IsoCrcEnable		= 1;
	config.StoreInTEF		= 0;

	DRV_CANFDSPI_Configure( DRV_CANFDSPI_INDEX_1, &config );

	// Setup TX FIFO
	DRV_CANFDSPI_TransmitChannelConfigureObjectReset( &txConfig );
	txConfig.FifoSize		= 7;
	txConfig.PayLoadSize	= CAN_PLSIZE_64;
	txConfig.TxPriority		= 1;
	DRV_CANFDSPI_TransmitChannelConfigure( DRV_CANFDSPI_INDEX_1, APP_TX_FIFO, &txConfig );

	// Setup RX FIFO
	DRV_CANFDSPI_ReceiveChannelConfigureObjectReset( &rxConfig );
	rxConfig.FifoSize		= 20;
	rxConfig.PayLoadSize	= CAN_PLSIZE_64;
	DRV_CANFDSPI_ReceiveChannelConfigure( DRV_CANFDSPI_INDEX_1, APP_RX_FIFO, &rxConfig );

	// All Receive
	// Setup RX Filter
	fObj.word		= 0;
	fObj.bF.SID		= 0xda;
	fObj.bF.EXIDE	= 0;
	fObj.bF.EID		= 0x00;
	DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, CAN_FILTER0, &fObj.bF );

	// Setup RX Mask
	mObj.word		= 0;
	mObj.bF.MSID	= 0x0;
	mObj.bF.MIDE	= 1; // Only allow standard IDs
	mObj.bF.MEID	= 0x0;
	DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, CAN_FILTER0, &mObj.bF );

	// Link FIFO and Filter
	DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, CAN_FILTER0, APP_RX_FIFO, true );

	// All Protect
	// Setup RX Filter
	fObj.word		= 0;
	fObj.bF.SID		= 0;
	fObj.bF.EXIDE	= 0;
	fObj.bF.EID		= 0;
	DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, CAN_FILTER31, &fObj.bF );

	// Setup RX Mask
	mObj.word		= 0;
	mObj.bF.MSID	= 0x7ff;
	mObj.bF.MIDE	= 1; // Only allow standard IDs
	mObj.bF.MEID	= 0x0;
	DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, CAN_FILTER31, &mObj.bF );

	// Link FIFO and Filter
	DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, CAN_FILTER31, APP_RX_FIFO, true );

	// Setup Bit Time
	DRV_CANFDSPI_BitTimeConfigure( DRV_CANFDSPI_INDEX_1, selectedBitTime, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M );

	// Setup Transmit and Receive Interrupts
	DRV_CANFDSPI_GpioModeConfigure( DRV_CANFDSPI_INDEX_1, GPIO_MODE_INT, GPIO_MODE_INT );

	DRV_CANFDSPI_TransmitChannelEventEnable( DRV_CANFDSPI_INDEX_1, APP_TX_FIFO, CAN_TX_FIFO_NOT_FULL_EVENT );

	DRV_CANFDSPI_ReceiveChannelEventEnable( DRV_CANFDSPI_INDEX_1, APP_RX_FIFO, CAN_RX_FIFO_NOT_EMPTY_EVENT );

	DRV_CANFDSPI_ModuleEventEnable( DRV_CANFDSPI_INDEX_1, (CAN_MODULE_EVENT)((uint16_t)CAN_TX_EVENT | (uint16_t)CAN_RX_EVENT ) );

	// Select Normal Mode
	DRV_CANFDSPI_OperationModeSelect( DRV_CANFDSPI_INDEX_1, CAN_NORMAL_MODE );

	return INIT_OK;
}

int32_t DeinitMCP2518FD( void )
{
	// All Transceiver Standby mode
	StopSpiCan();

	// Select Sleep Mode
//	DRV_CANFDSPI_OperationModeSelect( DRV_CANFDSPI_INDEX_1, CAN_SLEEP_MODE );
//	DRV_CANFDSPI_OperationModeSelect( DRV_CANFDSPI_INDEX_2, CAN_SLEEP_MODE );
//	DRV_CANFDSPI_OperationModeSelect( DRV_CANFDSPI_INDEX_3, CAN_SLEEP_MODE );
//	DRV_CANFDSPI_OperationModeSelect( DRV_CANFDSPI_INDEX_4, CAN_SLEEP_MODE );

//	StopSpiThread();

	return INIT_OK;
}

int32_t ReInitMCP2518FD( uint8_t index, CAN_BITTIME_SETUP bitTime)
{
	CAN_CONFIG			config;

	CAN_TX_FIFO_CONFIG	txConfig;
	CAN_RX_FIFO_CONFIG	rxConfig;

	REG_CiFLTOBJ		fObj;
	REG_CiMASK			mObj;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :			osMutexWait( hSpiMutex1, osWaitForever );			break;
	}

	CAN_BITTIME_SETUP	selectedBitTime = bitTime;

	// All Transceiver standby mode
	StopSpiCan();

	DRV_CANFDSPI_OperationModeSelect( DRV_CANFDSPI_INDEX_1, CAN_CONFIGURATION_MODE );	//can tx error fix //kkt
	osDelay( 10 );

	// Reset device
	DRV_CANFDSPI_Reset( index );

	// Enable ECC and initialize RAM
	DRV_CANFDSPI_EccEnable( index );
	DRV_CANFDSPI_RamInit( index, 0xff );

	// Configure device
	DRV_CANFDSPI_ConfigureObjectReset( &config );
	config.IsoCrcEnable		= 1;
	config.StoreInTEF		= 0;

	DRV_CANFDSPI_Configure( index, &config );

	// Setup TX FIFO
	DRV_CANFDSPI_TransmitChannelConfigureObjectReset( &txConfig );
	txConfig.FifoSize		= 7;
	txConfig.PayLoadSize	= CAN_PLSIZE_64;
	txConfig.TxPriority		= 1;
	DRV_CANFDSPI_TransmitChannelConfigure( index, APP_TX_FIFO, &txConfig );

	// Setup RX FIFO
	DRV_CANFDSPI_ReceiveChannelConfigureObjectReset( &rxConfig );
	rxConfig.FifoSize		= 20;
	rxConfig.PayLoadSize	= CAN_PLSIZE_64;
	DRV_CANFDSPI_ReceiveChannelConfigure( index, APP_RX_FIFO, &rxConfig );

	// All Receive
	// Setup RX Filter
	fObj.word		= 0;
	fObj.bF.SID		= 0xda;
	fObj.bF.EXIDE	= 0;
	fObj.bF.EID		= 0x00;
	DRV_CANFDSPI_FilterObjectConfigure( index, CAN_FILTER0, &fObj.bF );

	// Setup RX Mask
	mObj.word		= 0;
	mObj.bF.MSID	= 0x0;
	mObj.bF.MIDE	= 1; // Only allow standard IDs
	mObj.bF.MEID	= 0x0;
	DRV_CANFDSPI_FilterMaskConfigure( index, CAN_FILTER0, &mObj.bF );

	// Link FIFO and Filter
	DRV_CANFDSPI_FilterToFifoLink( index, CAN_FILTER0, APP_RX_FIFO, true );

	// All Protect
	// Setup RX Filter
	fObj.word		= 0;
	fObj.bF.SID		= 0;
	fObj.bF.EXIDE	= 0;
	fObj.bF.EID		= 0;
	DRV_CANFDSPI_FilterObjectConfigure( index, CAN_FILTER31, &fObj.bF );

	// Setup RX Mask
	mObj.word		= 0;
	mObj.bF.MSID	= 0x7ff;
	mObj.bF.MIDE	= 1; // Only allow standard IDs
	mObj.bF.MEID	= 0x0;
	DRV_CANFDSPI_FilterMaskConfigure( index, CAN_FILTER31, &mObj.bF );

	// Link FIFO and Filter
	DRV_CANFDSPI_FilterToFifoLink( index, CAN_FILTER31, APP_RX_FIFO, true );

	// Setup Bit Time
	DRV_CANFDSPI_BitTimeConfigure( index, selectedBitTime, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M );

	// Setup Transmit and Receive Interrupts
	DRV_CANFDSPI_GpioModeConfigure( index, GPIO_MODE_INT, GPIO_MODE_INT );

	DRV_CANFDSPI_TransmitChannelEventEnable( index, APP_TX_FIFO, CAN_TX_FIFO_NOT_FULL_EVENT );

	DRV_CANFDSPI_ReceiveChannelEventEnable( index, APP_RX_FIFO, CAN_RX_FIFO_NOT_EMPTY_EVENT );

	DRV_CANFDSPI_ModuleEventEnable( index, (CAN_MODULE_EVENT)((uint16_t)CAN_TX_EVENT | (uint16_t)CAN_RX_EVENT ) );

	// Select Normal Mode
	DRV_CANFDSPI_OperationModeSelect( index, CAN_NORMAL_MODE );

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :			rxFilterIndex[0] = 0;			osMutexRelease( hSpiMutex1 );			break;
	}

	// Select Normal Mode
	DRV_CANFDSPI_OperationModeSelect( DRV_CANFDSPI_INDEX_1, CAN_NORMAL_MODE );


	return INIT_OK;
}

void mcp2518_Rx_Error( void )
{
	uint32_t	i;

  	LED_ALL_OFF;
	__disable_irq();

	for( i = 0; i < 150; i++ )
	{
		LED_WHITE_TOGGLE;
		HAL_Delay( 200 );
	}
	jumpToBootloaderReset(3000);
}

int32_t StartSpiThread( void )
{
#ifdef MCP2518_DEBUG_FUNC
	GLogN( "[MCP2518] +%s\r\n", __FUNCTION__ );
#endif

	// Thread
 	hSpiCanTh1 = osThreadCreate( osThread(spicanth1), NULL );
	if( hSpiCanTh1 == NULL )
	{
		GLogE( "Error... fail create hSpiCanTh1 Thread!!!\r\n" );
		return 1;
	}

	hRxMonTh = osThreadCreate( osThread(rxmonth), NULL );
	if( hRxMonTh == NULL )
	{
		GLogE( "Error... fail create hRxMonTh Thread!!!\r\n" );
		return 1;
	}

	return 0;
}

int32_t StopSpiThread( void )
{
	StopSpiCan();								// all transceiver standby

	if( osThreadGetState( hSpiCanTh1 ) != osThreadDeleted)			osThreadTerminate( hSpiCanTh1 );

	while( osThreadGetState( hSpiCanTh1 ) != osThreadDeleted){}

	return 0;
}

// 220319 EnableHighCan2 하고 겹치는 부분
void StartSpiCan(void)
{
/*
        HAL_GPIO_WritePin( LAT_H_CAN_RX_EN2_GPIO_Port, LAT_H_CAN_RX_EN2_Pin, GPIO_PIN_RESET );
        HAL_GPIO_WritePin( LAT_L_CAN_RX_EN_GPIO_Port, LAT_L_CAN_RX_EN_Pin, GPIO_PIN_RESET );
*/
	EnableHighCan1();

}

// 220319 DisableHighCan2 하고 겹치는 부분
void StopSpiCan(void)
{
/*
        HAL_GPIO_WritePin( LAT_H_CAN_RX_EN2_GPIO_Port, LAT_H_CAN_RX_EN2_Pin, GPIO_PIN_SET );
*/
	DisableHighCan1();

}

void SetSpiCanSTB( void )
{
#ifdef MCP2518_DEBUG_FUNC
	GLogN( "[MCP2518] +%s\r\n", __FUNCTION__ );
#endif

	SetCanMasking();
	EnableReceive(DRV_CANFDSPI_INDEX_1);
	StartSpiCan();
#ifdef AddToTimer3 //mod.kks 22.04.29
	HAL_TIM_Base_Start_IT( &htim3 );
#endif
}

void SetCanBaudrate( void )
{
	CAN_BITTIME_SETUP	baudrate;

	// Setup Bit Time
	baudrate = (CAN_BITTIME_SETUP)gsFwInfo.msFDCan1Info.mBaudrateIndex;
	DRV_CANFDSPI_BitTimeConfigure( DRV_CANFDSPI_INDEX_1, baudrate, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M );

}


uint8_t mcp2518fd_set_baud( uint8_t nominalBaud, uint8_t dataBaud, uint8_t format )
{

	CAN_BITTIME_SETUP bitTime = CAN_500K_2M;
	int8_t re;

#ifdef MCP2518_DEBUG_FUNC
		GLogN( "[MCP2518] +%s\r\n", __FUNCTION__ );
#endif

	/* change mcp2518fd bittimme */
	if(nominalBaud == CAN_250KBPS )
	{
		switch( dataBaud )
		{
			case CAN_DAT_1MBPS: bitTime=CAN_250K_1M; break;
			case CAN_DAT_2MBPS: bitTime=CAN_250K_2M; break;
			case CAN_DAT_4MBPS: bitTime=CAN_250K_4M; break;
			//default: break
		}
	}
	else if(nominalBaud == CAN_500KBPS )
	{
		switch( dataBaud )
		{
			case CAN_DAT_1MBPS: bitTime=CAN_500K_1M; break;
			case CAN_DAT_2MBPS: bitTime=CAN_500K_2M; break;
			case CAN_DAT_4MBPS: bitTime=CAN_500K_4M; break;
			//default: break
		}
	}
	else if(nominalBaud == CAN_1MBPS )
	{
		switch( dataBaud )
		{
			case CAN_DAT_4MBPS: bitTime=CAN_1000K_4M; break;
			default: bitTime=CAN_1000K_4M; break;
		}
	}
	//else ... 기존can는 default 값을 넣음



	// 220319 format 처리는 ?

	//ReInitMCP2518FD( DRV_CANFDSPI_INDEX_1, bitTime );
	re = DRV_CANFDSPI_BitTimeConfigure( DRV_CANFDSPI_INDEX_1, bitTime, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M );


	return 1;

	// 220319 mcp2518fd 도 필요?
	// 220319 샘플 코드나 linux mcp2518fd 드라이버에서 이 레지스터만 초기화 하는 경우 없음.. 대부분 init에서 같이 호출됨
/*
	if( HAL_FDCAN_Init( hfdcan ) != HAL_OK )
	{
		GLogE( "Error... Fail init FDCAN!!!\r\n" );
		Error_Handler();
	}

	clearFilter( hfdcan );

	return 1;
*/
}


void SetCanMasking(void)
{
	CAN_FILTEROBJ_ID	fObj;
	CAN_MASKOBJ_ID		mObj;
	int8_t	addNum;
	uint8_t	i;

	addNum = gsFwInfo.msFDCan1Info.mHWFilterNum - rxFilterIndex[0];
//			if( gDBGFlag & DEBUG_LOG_INFORMATION )			GLogN( "SPI5 add Filter Num = %d\r\n", addNum );
	if( addNum )
	{

		//MONI 20230419 static analysis num : 25 / for checking array max index.	
		if( rxFilterIndex[0] >= 32 )
			return;
		
		for( i = 0; i < addNum; i++ )
		{
//				if( gDBGFlag & DEBUG_LOG_INFORMATION )			GLogN( "Filter Index : %d\r\n", rxFilterIndex[0] );
			// Disable Filter
			DRV_CANFDSPI_FilterDisable( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)rxFilterIndex[0] );

			// Configure Filter Object
			fObj.SID	= gsFwInfo.msFDCan1Info.mHWFilter[rxFilterIndex[0]];
			fObj.SID11	= 0;
			fObj.EID	= 0;
			fObj.EXIDE	= 0;

			DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)rxFilterIndex[0], &fObj );

			// Configure Mask Object
			mObj.MSID	= 0x7FF;
			mObj.MSID11	= 0;
			mObj.MEID	= 0;
			mObj.MIDE	= 1;

			DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)rxFilterIndex[0], &mObj );

			// Link Filter to RX FIFO and enable Filter
			DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)rxFilterIndex[0], APP_RX_FIFO, true );

			rxFilterIndex[0]++;
			if( rxFilterIndex[0] >= 32 )	return;
		}
	}

}
void SetCanMaskingMCP2518(uint32_t StartMask, uint32_t EndMask)
{
	CAN_FILTEROBJ_ID	fObj;
	CAN_MASKOBJ_ID		mObj;
	//int8_t	addNum;
	//uint8_t	i;
	uint32_t ulFilter, ulMask;

	GLogI( "\r\nMCP setCanMask : StartMask(0x%08X)EndMask(0x%08X)",StartMask,EndMask);
	if(StartMask==EndMask)
	{
		// Disable Filter
		DRV_CANFDSPI_FilterDisable( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0 );

		// Configure Filter Object
		fObj.SID	= StartMask;//gsFwInfo.msFDCan1Info.mHWFilter[rxFilterIndex[0]];
		fObj.SID11	= 0;
		fObj.EID	= 0;
		fObj.EXIDE	= 0;

		DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &fObj );

		// Configure Mask Object
		mObj.MSID	= 0x7FF;
		mObj.MSID11 = 0;
		mObj.MEID	= 0;
		mObj.MIDE	= 1;

		DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &mObj );

		// Link Filter to RX FIFO and enable Filter
		DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, APP_RX_FIFO, true );
	}
	else
	{
		// Disable Filter
		DRV_CANFDSPI_FilterDisable( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0 );

		ulFilter  = StartMask&EndMask;
		ulMask 	 = ~(StartMask^EndMask);

		// Configure Filter Object
		fObj.SID	= ulFilter;
		fObj.SID11	= 0;
		fObj.EID	= 0;
		fObj.EXIDE	= 0;

		DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &fObj );

		// Configure Mask Object
		mObj.MSID	= ulMask;
		mObj.MSID11	= 0;
		mObj.MEID	= 0;
		mObj.MIDE	= 1;

		DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &mObj );

		// Link Filter to RX FIFO and enable Filter
		DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, APP_RX_FIFO, true );
	}

#if 0//sample : accept only 0x07E8
	// Disable Filter
	DRV_CANFDSPI_FilterDisable( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0 );

	// Configure Filter Object
	fObj.SID	= 0x07E8;//gsFwInfo.msFDCan1Info.mHWFilter[rxFilterIndex[0]];
	fObj.SID11	= 0;
	fObj.EID	= 0;
	fObj.EXIDE	= 0;

	DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &fObj );

	// Configure Mask Object
	mObj.MSID	= 0x7FF;
	mObj.MSID11	= 0;
	mObj.MEID	= 0;
	mObj.MIDE	= 1;

	DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &mObj );

	// Link Filter to RX FIFO and enable Filter
	DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, APP_RX_FIFO, true );

#endif

}
#if 0 //working source
void SetCanMaskingMCP2518_EXT(uint32_t StartMask, uint32_t EndMask)
{
	CAN_FILTEROBJ_ID	fObj;
	CAN_MASKOBJ_ID		mObj;
	int8_t	addNum;
	uint8_t	i;

	GLogI( "\r\nMCP setCanMask : StartMask(0x%08X)EndMask(0x%08X)",StartMask,EndMask);
	//StartMask = 0x18DA00F9;
	//EndMask = 0x18DAF900;
	StartMask = 0x18DA0000;
	EndMask = 0x18DAFFFF;
	if(StartMask==EndMask)
	{
		// Disable Filter
		DRV_CANFDSPI_FilterDisable( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0 );

		// Configure Filter Object
		fObj.SID	= (StartMask&0x1FFC0000)>>18;//gsFwInfo.msFDCan1Info.mHWFilter[rxFilterIndex[0]];
		fObj.SID11	= 0;
		fObj.EID	= StartMask&0x3FFFF;
		fObj.EXIDE	= 1;
	GLogI( "\r\n Fobj:0x%08X)",(fObj.SID<<18)|fObj.EID);
		DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &fObj );

		// Configure Mask Object
		mObj.MSID	= (StartMask&0x1FFC0000)>>18;
		mObj.MSID11 = 0;
		mObj.MEID	= StartMask&0x3FFFF;
		mObj.MIDE	= 1;
	GLogI( "\r\n Mobj:0x%08X)",(mObj.MSID<<18)|mObj.MEID);
		DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &mObj );

		// Link Filter to RX FIFO and enable Filter
		DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, APP_RX_FIFO, true );
	}
	else
	{
		// Disable Filter
		DRV_CANFDSPI_FilterDisable( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0 );

		// Configure Filter Object
		fObj.SID	= (StartMask&0x1FFC0000)>>18;//gsFwInfo.msFDCan1Info.mHWFilter[rxFilterIndex[0]];
		fObj.SID11	= 0;
		fObj.EID	= StartMask&0x3FFFF;
		fObj.EXIDE	= 1;
GLogI( "\r\n Fobj:0x%08X\r\n",(fObj.SID<<18)|fObj.EID);
GLogI( "\r\n fObj.EXIDE:%d\r\n",fObj.EXIDE);
		DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &fObj );

		// Configure Mask Object
		mObj.MSID	= (StartMask&0x1FFC0000)>>18;
		mObj.MSID11 = 0;
		mObj.MEID	= StartMask&0x3FFFF;
		mObj.MIDE	= 1;

GLogI( "\r\n Mobj:0x%08X\r\n",(mObj.MSID<<18)|mObj.MEID);
GLogI( "\r\n mObj.MIDE:%d\r\n",mObj.MIDE);
		DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &mObj );

		// Link Filter to RX FIFO and enable Filter
		DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, APP_RX_FIFO, true );
	}
}
#endif
void SetCanMaskingMCP2518_EXT(uint32_t StartMask, uint32_t EndMask)
{
	CAN_FILTEROBJ_ID	fObj;
	CAN_MASKOBJ_ID		mObj;
	//int8_t	addNum;
	//uint8_t	i;
	uint32_t ulFilter, ulMask;

	GLogI( "\r\nMCP setCanMask : StartMask(0x%08X)EndMask(0x%08X)",StartMask,EndMask);
//	StartMask = 0x18DA00F9;
//	EndMask = 0x18DAF900;
	if(StartMask==EndMask)
	{
		// Disable Filter
		DRV_CANFDSPI_FilterDisable( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0 );

		// Configure Filter Object
		fObj.SID	= (EndMask&0x1FFC0000)>>18;//gsFwInfo.msFDCan1Info.mHWFilter[rxFilterIndex[0]];
		fObj.SID11	= 0;
		fObj.EID	= EndMask&0x3FFFF;
		fObj.EXIDE	= 1;
	GLogI( "\r\n Fobj:0x%08X)",(fObj.SID<<18)|fObj.EID);
		DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &fObj );

		// Configure Mask Object
		mObj.MSID	= (EndMask&0x1FFC0000)>>18;
		mObj.MSID11 = 0;
		mObj.MEID	= EndMask&0x3FFFF;
		mObj.MIDE	= 1;
	GLogI( "\r\n Mobj:0x%08X)",(mObj.MSID<<18)|mObj.MEID);
		DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &mObj );

		// Link Filter to RX FIFO and enable Filter
		DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, APP_RX_FIFO, true );
	}
	else
	{
		// Disable Filter
		DRV_CANFDSPI_FilterDisable( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0 );

		ulFilter  = StartMask&EndMask;
		ulMask 	 = ~(StartMask^EndMask);

		// Configure Filter Object
		fObj.SID	= (ulFilter&0x1FFC0000)>>18;//gsFwInfo.msFDCan1Info.mHWFilter[rxFilterIndex[0]];
		fObj.SID11	= 0;
		fObj.EID	= ulFilter&0x3FFFF;
		fObj.EXIDE	= 1;
GLogI( "\r\n Fobj:0x%08X\r\n",(fObj.SID<<18)|fObj.EID);
GLogI( "\r\n fObj.EXIDE:%d\r\n",fObj.EXIDE);
		DRV_CANFDSPI_FilterObjectConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &fObj );

		// Configure Mask Object
		mObj.MSID	= (ulMask&0x1FFC0000)>>18;
		mObj.MSID11 = 0;
		mObj.MEID	= ulMask&0x3FFFF;
		mObj.MIDE	= 1;

GLogI( "\r\n Mobj:0x%08X\r\n",(mObj.MSID<<18)|mObj.MEID);
GLogI( "\r\n mObj.MIDE:%d\r\n",mObj.MIDE);
		DRV_CANFDSPI_FilterMaskConfigure( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, &mObj );

		// Link Filter to RX FIFO and enable Filter
		DRV_CANFDSPI_FilterToFifoLink( DRV_CANFDSPI_INDEX_1, (CAN_FILTER)0, APP_RX_FIFO, true );
	}
}


#if 0
u8 CAN_Channel_Masket_Set(FDCAN_HandleTypeDef *hfdcan, /*u8 nRxFifo,*/ u8 nCANIDType, u8 nMaskNum, u32 *pStartMaskValue, u32 *pEndMaskValue)
{
	u16 index;
	FDCAN_FilterTypeDef sFilterConfig;

	CAN_FILTEROBJ_ID	fObj;
	CAN_MASKOBJ_ID		mObj;


    if(nCANIDType == 1)
    {
        sFilterConfig.IdType = FDCAN_STANDARD_ID;

		fObj.EXIDE = 0;
    }
    else
    {
        sFilterConfig.IdType = FDCAN_EXTENDED_ID;

		fObj.EXIDE = 1;
    }
/*
	if(nRxFifo == 1)
	{
        sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	}
	else
	{
        sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
	}
*/
    for(index=0; index<nMaskNum; index++)
    {

        if(pStartMaskValue[index] == pEndMaskValue[index])
        {
            // sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
			// Configure Mask Object
			mObj.MSID	= 0x7FF;
			mObj.MSID11	= 0;
			mObj.MEID	= 0;
			mObj.MIDE	= 1;

        }
        else
        {
#if 1
            sFilterConfig.FilterType = FDCAN_FILTER_RANGE;
#else       // 기존 sja1000, stm32f207에서 range가 없음. 하지만 PC APP 개발자들이 range로 알고 있어서 range로 설정 시도.
            // 추후 디로거 상태파악 후 FDCAN_FILTER_MASK 변경해도 좋음
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
#endif


void EnableReceive(uint8_t index)
{
        //MONI suresoft ecu 40] initalize filter num 
	uint8_t	filterNum = 0;
	uint8_t	i;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :		filterNum = gsFwInfo.msFDCan1Info.mHWFilterNum;			break;
	}
	for( i = 0; i < filterNum; i++ )			DRV_CANFDSPI_FilterEnable( index, (CAN_FILTER)i );
}

void DisableReceive( uint8_t index )
{
        //MONI suresoft ecu 40] initalize filter num 
	uint8_t	filterNum = 0;
	uint8_t	i;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :		filterNum = gsFwInfo.msFDCan1Info.mHWFilterNum;			break;
	}

	for( i = 0; i < filterNum; i++ )			DRV_CANFDSPI_FilterDisable( index, (CAN_FILTER)i );
}

void ClearCanMasking( uint8_t index )
{
	CAN_FILTEROBJ_ID	fObj;
	CAN_MASKOBJ_ID		mObj;

        //MONI suresoft ecu 40] initalize filter num 
	uint8_t	filterNum = 0;
	uint8_t	i;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :		filterNum = gsFwInfo.msFDCan1Info.mHWFilterNum;			break;
	}

	for( i = 0; i < filterNum; i++ )
	{
		// Disable Filter
		DRV_CANFDSPI_FilterDisable( index, (CAN_FILTER)i );
	}

	// Configure Filter Object
	fObj.SID	= 0;
	fObj.SID11	= 0;
	fObj.EID	= 0;
	fObj.EXIDE	= 0;
	DRV_CANFDSPI_FilterObjectConfigure( index, CAN_FILTER0, &fObj );

	// Configure Mask Object
	mObj.MSID	= 0;
	mObj.MSID11	= 0;
	mObj.MEID	= 0;
	mObj.MIDE	= 1;
	DRV_CANFDSPI_FilterMaskConfigure( index, CAN_FILTER0, &mObj );

	// Link Filter to RX FIFO and enable Filter
	DRV_CANFDSPI_FilterToFifoLink( index, CAN_FILTER0, APP_RX_FIFO, true );

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :		gsFwInfo.msFDCan1Info.mHWFilterNum = 0;			rxFilterIndex[0] = 0;			break;
	}
}

/*----------------------------------------------------------------------
 *   Threads
 *--------------------------------------------------------------------*/
static void SpiCanThread1( void const *argument )
{
	CAN_TX_FIFO_EVENT	txFlags;

	MsgClst_t	*message;
	SpiCanPkt_t	*packet;

	osEvent		evt;

#ifdef MCP2518_DEBUG_FUNC
	GLogN( "[MCP2518] +%s\r\n", __FUNCTION__ );
#endif

	for(;;)
	{
		// tx
		evt	= osMessageGet( hTxSpiCanMsg1, osWaitForever );
		if( evt.status == osEventMessage )
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hTxSpiCanMsg1));
#endif
			osMutexWait( hSpiMutex1, osWaitForever );

			message = ( MsgClst_t * )evt.value.p;
			packet	= ( SpiCanPkt_t * )message->pPacket;

#if 0
			if( gsFwInfo.msFDCan1Info.mBypassMode == 1 )		// can frame
#else
			// 220319 test
			{
				packet->mTxObj.bF.ctrl.FDF	= 0;
				packet->mTxObj.bF.ctrl.BRS	= 0;
			}
#endif

			if( gDBGFlag & DEBUG_LOG_BYPASS )
			{
#ifdef MCP2518_LOG
				GLogN( "SPI5:ID(0x%04x), DLC( %d ), IDE( %d), BRS( %d ), FDF( %d ), Data : ", packet->mTxObj.bF.id.SID, packet->mTxObj.bF.ctrl.DLC, packet->mTxObj.bF.ctrl.IDE, packet->mTxObj.bF.ctrl.BRS, packet->mTxObj.bF.ctrl.FDF );
				for( uint8_t i = 0; i < packet->mLen; i++ )
				{
					GLogN( "0x%02x ", packet->mData[i] );
				}
				GLogN( "\r\n" );
#endif
			}

			gWaitTimerFlag1 = 0;
			StartSWTimer( gWaitTimerIndex1, eSWTimer_ONESHOT );

			for(;;)
			{
				DRV_CANFDSPI_TransmitChannelEventGet( DRV_CANFDSPI_INDEX_1, APP_TX_FIFO, &txFlags );
				if( txFlags & CAN_TX_FIFO_NOT_FULL_EVENT )
				{
					// Load message and transmit
					DRV_CANFDSPI_TransmitChannelLoad( DRV_CANFDSPI_INDEX_1, APP_TX_FIFO, &packet->mTxObj, packet->mData, packet->mLen, true );
					break;
				}

				if( gWaitTimerFlag1 )
				{
					GLogEE( "txS1\t" );
					if( gDBGFlag & DEBUG_LOG_INFORMATION )
					{
						GLogEE( "Error... Attempts is zero!!!\r\n" );
						mcp2518_Rx_Error();//30sec blinking and sw reset
						//Error_Handler();
						// Reset device
						//DRV_CANFDSPI_Reset( DRV_CANFDSPI_INDEX_1 );
						//osDelay(500);
						//IO_CONTROL_LOW(OSC2_OE_EN);
						//osDelay(500);
						//IO_CONTROL_HIGH(OSC2_OE_EN);

						//원인 파악중
						//osDelay(10);
						//jumpToBootloaderReset(3000);
						//IO_CONTROL_LOW(OSC2_OE_EN);
						//osDelay(10);
						//IO_CONTROL_HIGH(OSC2_OE_EN);
						//osDelay(10);
					}
					break;
				}
			}

			osPoolFree( hSpiCanPktPool, (void *)packet );
			osPoolFree( hMsgPool, (void *)message );

			osMutexRelease( hSpiMutex1 );
		}
	}
}

static void RxMoniterThread( void const *argument )
{
	static uint8_t	count = 0;

#ifdef MCP2518_DEBUG_FUNC
	GLogN( "[MCP2518] +%s\r\n", __FUNCTION__ );
#endif

	for( ;; )
	{
		osSignalWait( 0x0001, osWaitForever );

#if 0
		if( HAL_GPIO_ReadPin( RX_INT1_GPIO_Port, RX_INT1_Pin ) == GPIO_PIN_RESET )				(*rx1_callback)();
		if( HAL_GPIO_ReadPin( RX_INT2_GPIO_Port, RX_INT2_Pin ) == GPIO_PIN_RESET )				(*rx2_callback)();
		if( HAL_GPIO_ReadPin( RX_INT3_GPIO_Port, RX_INT3_Pin ) == GPIO_PIN_RESET )				(*rx3_callback)();
		if( HAL_GPIO_ReadPin( RX_INT4_GPIO_Port, RX_INT4_Pin ) == GPIO_PIN_RESET )				(*rx4_callback)();
#else
		switch( count )
		{
			case	0 :
			{
				if( HAL_GPIO_ReadPin( RX_INT2_GPIO_Port, RX_INT2_Pin ) == GPIO_PIN_RESET )				(*rx1_callback)();

				count = 0;
				break;
			}
		}
#endif
	}
}

/**********************************************************************************************/
/*   Callback Function                                                                        */
/**********************************************************************************************/
inline void checkSpiTxWaitCallback1( void )
{
	gWaitTimerFlag1 = 1;
}

/*----------------------------------------------------------------------
 *   Can Interrupt Callback Function
 *--------------------------------------------------------------------*/
void Mcp2518fd_RxInt1_handler( void )
{
	MsgClst_t	*message;
#if 0
	SpiCanPkt_t	*packet;
#else
	stFdcanPkt	*packet;
	CAN_RX_MSGOBJ rxObj;
#endif

#if defined ( SAVE_CAN_LOG )
	uint32_t	can_id;
	uint16_t	size;
#endif

	int			ret = 0;

#ifdef MCP2518_LOG
	GLogN( "[MCP2518] +%s\r\n", __FUNCTION__ );
#endif

	if( osMutexWait( hSpiMutex1, 0 ) != osOK )		return;

	// memory pool alloc
	message	= ( MsgClst_t* )osPoolCAlloc( hMsgPool );
	if( message != NULL )
	{
#if 0
		packet	= ( SpiCanPkt_t* )osPoolCAlloc( hSpiCanPktPool );
#else
		packet	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
#endif
		if( packet != NULL )
		{
			// Get message
#if 0
			ret = DRV_CANFDSPI_ReceiveMessageGet( DRV_CANFDSPI_INDEX_1, APP_RX_FIFO, &packet->mRxObj, packet->mData, MAX_DATA_BYTES );
#else
			ret = DRV_CANFDSPI_ReceiveMessageGet( DRV_CANFDSPI_INDEX_1, APP_RX_FIFO, &rxObj, packet->mData, MAX_DATA_BYTES );
#endif
			if( ret == 0 )
			{
#ifdef MCP2518_LOG
				GLogE( "[MCP2518] Mcp2518fd_RxInt1_handler(0x%x)\r\n",rxObj.bF.id.SID );
#endif

#if 0
				packet->mLen	= DRV_CANFDSPI_DlcToDataBytes( (CAN_DLC)packet->mRxObj.bF.ctrl.DLC );
#else
				packet->mLen	= DRV_CANFDSPI_DlcToDataBytes( (CAN_DLC)rxObj.bF.ctrl.DLC );

				// packet header spi -> fdcan
				// 처리필요
				if(rxObj.bF.id.SID11 == 1) ;

				if(rxObj.bF.ctrl.IDE == MCP2518_IDE_STANDARD)
				{
					packet->mRxHeader.IdType = FDCAN_STANDARD_ID;
					packet->mRxHeader.Identifier = rxObj.bF.id.SID;
				}
				else
				{
					packet->mRxHeader.IdType = FDCAN_EXTENDED_ID;
					packet->mRxHeader.Identifier = rxObj.bF.id.SID << 18 | rxObj.bF.id.EID;
				}

				// FDCAN_ELEMENT_MASK_RTR 0x2000 0000, 29
				packet->mRxHeader.RxFrameType = rxObj.bF.ctrl.RTR << 29;

				// FDCAN_ELEMENT_MASK_DLC 0x000F 0000, 16
				packet->mRxHeader.DataLength = rxObj.bF.ctrl.DLC << 16;

				// STM32에서 동일값 의미인지 확인 필요
				// FDCAN_ELEMENT_MASK_ESI 0x8000 0000, 31
				packet->mRxHeader.ErrorStateIndicator = rxObj.bF.ctrl.ESI << 31;

				// FDCAN_ELEMENT_MASK_BRS 0x0010 0000, 20
				packet->mRxHeader.BitRateSwitch = rxObj.bF.ctrl.BRS << 20;


				// FDCAN_ELEMENT_MASK_FDF 0x0020 0000, 21
				packet->mRxHeader.FDFormat = rxObj.bF.ctrl.FDF << 21;

				// 확인 필요
				packet->mRxHeader.RxTimestamp = 0;
				packet->mRxHeader.FilterIndex = 0;
				packet->mRxHeader.IsFilterMatchingFrame = 0;

#endif
#if 0
				packet->pSource	= SPI5;
#else
				packet->pSource	= (FDCAN_HandleTypeDef*)SPI5;
				packet->pTarget	= (FDCAN_HandleTypeDef*)SPI5;
#endif

				message->mPktType	= PACKET_SPICAN;

				message->pPacket	= (void *)packet;
#if 0// test
//				if( packet->mRxObj.bF.id.SID == 0x03e0 )			GLogEE( "0x3e0\r\n" );
				if( packet->mRxObj.bF.id.SID == 0x03a5 )
				{
					GLogEE( "0x417 " );
//					GLogEE( "0x%02x ", packet->mData[10]);
				}
#endif

#if SPI_RX_DATA_CHECK_TEST
				if( packet->mRxObj.bF.id.SID == 0x0002 )
				{
					for( uint8_t i = 0; i < packet->mLen; i++ )
					{
						if( packet->mData[i] != 0x00 )
						{
							GLogEE( "1 : %d, 0x%02x\r\n", i, packet->mData[i] );
						}
					}
				}
#endif

#if 0
				gSpiRxCount1++;
				if( gsFwInfo.msFDCan1Info.mBypassFlag == 1 )
				{
					osMessagePut( hRxSpiCanMsg, (uint32_t)message, osWaitForever );
				}
				else
				{
					osPoolFree( hSpiCanPktPool, (void *)packet );
					osPoolFree( hMsgPool, (void *)message );
				}
#else
				gSpiRxCount1++;
				if( xQueueIsQueueFullFromISR( hFDRxMsg ) == TRUE )
 				{
					osPoolFree( hFdcanPktPool, (void *)packet );
					osPoolFree( hMsgPool, (void *)message );
				}
				else
				{
					osMessagePut( hFDRxMsg, (uint32_t)message, osWaitForever );
				}
#endif
			}
			else
			{
				if( gDBGFlag & DEBUG_LOG_INFORMATION )		GLogE( "error... DRV_CANFDSPI5_ReceiveMessageGet(%d)\r\n", ret );
#if 0
				osPoolFree( hSpiCanPktPool, (void *)packet );
#else
				osPoolFree( hFdcanPktPool, (void *)packet );
#endif
				osPoolFree( hMsgPool, (void *)message );
			}
		}
		else
		{
			if( gDBGFlag & DEBUG_LOG_QUEUE )			GLogN( "S1PQZ!!!\t" );
			osPoolFree( hMsgPool, (void *)message );
			gErrorCountRx1++;
		}
	}
	else
	{
		if( gDBGFlag & DEBUG_LOG_QUEUE )			GLogN( "S1MQZ!!!\t" );
		gErrorCountRx1++;
	}

	if( gErrorCountRx1 & 0x1000 )
	{
		GLogEE( "ESRX1!!!\t" );
		gErrorCountRx1 = 0;
	}

	osMutexRelease( hSpiMutex1 );

	return;
}

#if 0
void makeTxHeaderCANSPI( CAN_TX_MSGOBJ *header, uint32_t iden, uint8_t type, uint8_t len, uint8_t format )		// 11byte identifier
{
	header->bF.ctrl.DLC = DRV_CANFDSPI_DataBytesToDlc(len);

/*
	header->TxFrameType				= FDCAN_DATA_FRAME;
	header->ErrorStateIndicator		= FDCAN_ESI_ACTIVE;
	header->TxEventFifoControl		= FDCAN_NO_TX_EVENTS;
	header->MessageMarker			= 0;
*/
	if( type == CAN_IDTYPE_EXTENDED )
	{
		header->bF.ctrl.IDE = MCP2518_IDE_EXTENDED;
		header->bF.id.SID = iden >> 18;
		header->bF.id.EID = (iden & 0x3ffff);
	}
	else
	{
		header->bF.ctrl.IDE = MCP2518_IDE_STANDARD;
		header->bF.id.SID = iden;
		header->bF.id.EID = 0;
	}


	if( format == CAN_FRAMEFORMAT_FDCAN )					// FDCAN
	{
		header->bF.ctrl.BRS	= 1;
		header->bF.ctrl.FDF	= 1;
	}
	else													// ClassicCan
	{

		header->bF.ctrl.FDF	= 0;
		header->bF.ctrl.BRS	= 0;

	}
}
#endif

