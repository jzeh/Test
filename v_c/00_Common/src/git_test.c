/*************************************************************
 * NOTE : git_test.c
 *      Test Firmware
 *      VCI_III_ASING_MODE == 1 : Wifi 2.4G
 *      VCI_III_ASING_MODE == 2 : Wifi 5G
 * Author : Lee junho
 * Since : 2021.11.29
**************************************************************/
#if( VCI_III_ASING_MODE )
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include "common.h"
#include "typedef.h"
#include "git_can.h"
#ifdef VCI3_DIAG
#include "git_eth.h"
#endif
#include "git_vci.h"
#include "git_pool.h"
#include "led.h"
#include "git_rs9116.h"

#include "git_test.h"
#ifdef VCI3_DIAG
#include "git_mcp2518fd.h"
#endif

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define DHCP_IP_SETTING											0

#define TEST_CANFD													0
#define TEST_ETHERNET_TX											1
#define TEST_ETHERNET_T1											0
#define TEST_WIFI													1
#define TEST_BT													1

#if( VCI_III_ASING_MODE == 1 )
	#define WIFI_AP_NAME											"VCI3_test2.4G"
	#define WIFI_AP_PSK												"#git1234"
#elif( VCI_III_ASING_MODE == 2 )
	#define WIFI_AP_NAME											"VCI3_test5G"
	#define WIFI_AP_PSK												"#git1234"
#endif


/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
static void testCertifyThread( void const *argument );

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
// message queue

// thread
osThreadId		hTestCertifyTh;
osThreadDef( testcertifyth, testCertifyThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 10 );

uint8_t		txBuff[9]	= {0x48, 0x45, 0x4C, 0x4C, 0x4F, 0x20, 0x47, 0x49, 0x54};			//"HELLO GIT"

uint8_t		g_TriggerKey	= 0;

// external variables

/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/
#if( VCI_III_ASING_MODE == 1 )
int32_t initTestModule( void )
{  
	GLogI( "==================================================\r\n" );
	if( g_TriggerKey == 1 )
	{
		GLogI( " Verification Case 1-2( Wifi 2.4G : %s )\r\n", WIFI_AP_NAME );
	}
	else
	{
		GLogI( " Verification Case 1-1( Wifi 2.4G : %s )\r\n", WIFI_AP_NAME );
	}

	GLogI( "==================================================\r\n" );
#if( TEST_WIFI )
	GLogI( "   >> Wifi Connection \r\n" );

	while( ConnectAP( WIFI_AP_NAME, RSI_WPA2 , WIFI_AP_PSK ) != 0 )
	{
		osDelay(10);
	}
	
	SetIPAddressStatic( RS9116_IPV4_ADDR( 192, 168,   0,   2 ), RS9116_IPV4_ADDR( 192, 168,   0,   1 ), RS9116_IPV4_ADDR( 255, 255, 255,   0 ) );
#endif	// TEST_WIFI

	if( g_TriggerKey == 1 )											// Ethernet T1
	{
		GLogI( "   >> Ethernet Connect T1\r\n" );

		g_stGITHWSetDataEth.nSourceIP_T1		= RS9116_IPV4_ADDR( 10, 0, 5, 0 );
		g_stGITHWSetDataEth.nSourcePort_T1		= 52081;
		g_stGITHWSetDataEth.nDestinationIp_T1	= RS9116_IPV4_ADDR( 10, 32, 0, 0 );
		g_stGITHWSetDataEth.nDestinationPort_T1	= 13402;
		g_ethSocket.state						= eSOCK_STATE_INIT;

		setEthSocketState(&g_ethSocket, eSOCK_STATE_INIT);

		//eth_SetIPAddressStatic( RS9116_IPV4_ADDR( 10, 0, 5, 0 ), RS9116_IPV4_ADDR( 255, 0, 0, 0 ), RS9116_IPV4_ADDR( 10, 0, 128, 1 ) );

		GLogI( "   >> CAN Loopback Test\r\n" );
		// Can Loopback setting
		clearRXCanMessage();
		clearTXCanMessage();

		HIGHCAN2_120OHM_ENABLE;

		
		setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x700, 0x700 );
		setFilter2( FDCAN_FILTER_TO_RXFIFO0, 0x700, 0x700 );

		IO_CONTROL_HIGH( DLCA_EN1 );
		IO_CONTROL_LOW( DLCA_EN2 );
		IO_CONTROL_LOW( DLCA_EN3 );
		IO_CONTROL_HIGH( DLCA_EN6 );
		IO_CONTROL_LOW( DLCA_EN7 );
		IO_CONTROL_LOW( DLCA_EN8_1 );
		IO_CONTROL_LOW( DLCA_EN13 );
		IO_CONTROL_LOW( DLCA_EN15_1 );

		IO_CONTROL_LOW( DLCB_EN8 );
		IO_CONTROL_HIGH( DLCB_EN9 );
		IO_CONTROL_LOW( DLCB_EN10 );
		IO_CONTROL_LOW( DLCB_EN11 );
		IO_CONTROL_LOW( DLCB_EN12 );
		IO_CONTROL_HIGH( DLCB_EN14 );
		IO_CONTROL_LOW( DLCB_EN15 );

		//startFDCan( 1, 1, 0 );
	}
	else																				// Ethernet Tx
	{
		GLogI( "   >> Ethernet Connect TX \r\n" );

		//g_stGITHWSetDataEth.nSourceIP_T1		= RS9116_IPV4_ADDR( 192, 168, 0, 3 );
		//g_stGITHWSetDataEth.nSourcePort_T1		= 52081;
		//g_stGITHWSetDataEth.nDestinationIp_T1	= RS9116_IPV4_ADDR( 192, 168, 0, 16 );
		//g_stGITHWSetDataEth.nDestinationPort_T1	= 13402;
		//g_ethSocket.state						= eSOCK_STATE_INIT;
		
		//eth_SetIPAddressStatic( RS9116_IPV4_ADDR( 192, 168, 0, 3 ), RS9116_IPV4_ADDR( 255, 255, 255,   0 ), RS9116_IPV4_ADDR( 192, 168,   0,   1 ) );

		GLogI( "   >> CAN1 Tx Rx Test\r\n" );
		HIGHCAN2_120OHM_ENABLE;

		CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_FDCAN );

//		clearFilter( &hfdcan1 );

		setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x700, 0x700 );

		DisableUSB3300(); 

		startFDCan( 1, 0, 0 );
	}

	LED_ALL_OFF;

	HAL_GPIO_WritePin(HSM_PWR_EN_GPIO_Port,HSM_PWR_EN_Pin,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(HSM_RST_GPIO_Port, HSM_RST_Pin, GPIO_PIN_RESET);
	
	GLogI( "   >> KLLine Loopback Test\r\n" );
	GLogI( "   >> BT SPP Test\r\n" );
	GLogI( "==================================================\r\n" );

	return INIT_OK;
}
#elif( VCI_III_ASING_MODE == 2 )
int32_t initTestModule( void )
{
	GLogI( "==================================================\r\n" );
	if( g_TriggerKey == 1 )
	{
		GLogI( " Verification Case 2-2( Wifi 5G : %s )\r\n", WIFI_AP_NAME );
	}
	else
	{
		GLogI( " Verification Case 2-1( Wifi 5G : %s )\r\n", WIFI_AP_NAME );
	}

	GLogI( "==================================================\r\n" );
#if( TEST_WIFI )
	GLogI( "   >> Wifi Connection \r\n" );

	while( ConnectAP( WIFI_AP_NAME, RSI_WPA2 , WIFI_AP_PSK ) != 0 )
	{
		osDelay(10);
	}

	SetIPAddressStatic( RS9116_IPV4_ADDR( 192, 168,   0,   2 ), RS9116_IPV4_ADDR( 192, 168,   0,   1 ), RS9116_IPV4_ADDR( 255, 255, 255,   0 ) );
#endif	// TEST_WIFI

	if( g_TriggerKey == 1 )											// Ethernet T1
	{
#if 0 //mod.kks todo test
		GLogI( "   >> Ethernet Connect T1\r\n" );

		g_stGITHWSetDataEth.nSourceIP_T1		= RS9116_IPV4_ADDR( 10, 0, 5, 0 );
		g_stGITHWSetDataEth.nSourcePort_T1		= 52081;
		g_stGITHWSetDataEth.nDestinationIp_T1	= RS9116_IPV4_ADDR( 10, 32, 0, 0 );
		g_stGITHWSetDataEth.nDestinationPort_T1	= 13402;
		g_ethSocket.state						= eSOCK_STATE_INIT;

		setEthSocketState(&g_ethSocket, eSOCK_STATE_INIT);

		eth_SetIPAddressStatic( RS9116_IPV4_ADDR( 10, 0, 5, 0 ), RS9116_IPV4_ADDR( 255, 0, 0, 0 ), RS9116_IPV4_ADDR( 10, 0, 128, 1 ) );
#endif

		GLogI( "   >> CAN Loopback Test\r\n" );
		// Can Loopback setting
		clearRXCanMessage();
		clearTXCanMessage();

		HIGHCAN2_120OHM_ENABLE; 

		setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x700, 0x700 );
		setFilter2( FDCAN_FILTER_TO_RXFIFO0, 0x700, 0x700 );

		IO_CONTROL_HIGH( DLCA_EN1 );
		IO_CONTROL_LOW( DLCA_EN2 );
		IO_CONTROL_LOW( DLCA_EN3 );
		IO_CONTROL_HIGH( DLCA_EN6 );
		IO_CONTROL_LOW( DLCA_EN7 );
		IO_CONTROL_LOW( DLCA_EN8_1 );
		IO_CONTROL_LOW( DLCA_EN13 );
		IO_CONTROL_LOW( DLCA_EN15_1 );

		IO_CONTROL_LOW( DLCB_EN8 );
		IO_CONTROL_HIGH( DLCB_EN9 );
		IO_CONTROL_LOW( DLCB_EN10 );
		IO_CONTROL_LOW( DLCB_EN11 );
		IO_CONTROL_LOW( DLCB_EN12 );
		IO_CONTROL_HIGH( DLCB_EN14 );
		IO_CONTROL_LOW( DLCB_EN15 );

		startFDCan( 1, 1, 0 );
	}
	else																				// Ethernet Tx
	{
		GLogI( "   >> Ethernet Connect TX \r\n" );

		//g_stGITHWSetDataEth.nSourceIP_T1		= RS9116_IPV4_ADDR( 192, 168, 0, 2 );
		//g_stGITHWSetDataEth.nSourcePort_T1		= 52081;
		//g_stGITHWSetDataEth.nDestinationIp_T1	= RS9116_IPV4_ADDR( 192, 168, 0, 16 );
		//g_stGITHWSetDataEth.nDestinationPort_T1	= 13402;
		//g_ethSocket.state						= eSOCK_STATE_INIT;

		//eth_SetIPAddressStatic( RS9116_IPV4_ADDR( 192, 168, 0, 3 ), RS9116_IPV4_ADDR( 255, 255, 255,   0 ), RS9116_IPV4_ADDR( 192, 168,   0,   1 ) );

		GLogI( "   >> CAN1 Tx Rx Test\r\n" );
		HIGHCAN2_120OHM_ENABLE;

		CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_FDCAN );

//		clearFilter( &hfdcan1 );

		setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x700, 0x700 );

		DisableUSB3300(); 

		startFDCan( 1, 0, 0 );
	}

	LED_ALL_OFF;

	GLogI( "   >> KLLine Loopback Test\r\n" );
	GLogI( "   >> BT SPP Test\r\n" );
	GLogI( "==================================================\r\n" );

	return INIT_OK;
}
#endif	// VCI_III_ASING_MODE

uint8_t startTestThread( void )
{
	hTestCertifyTh = osThreadCreate( osThread(testcertifyth), NULL );
	if( hTestCertifyTh == NULL )
	{
		GLogE( "Error... fail create hTestCertifyTh Thread!!!\r\n" );
		return 0;
	}

	return 1;
}

void termDelay( void )
{
	osDelay( 200 );
}

uint8_t testCanTxRxTest( void )
{
	uint8_t 	ret = 0;

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	SpiCanPkt_t *rxPktSPI;

	uint8_t		txBuff[8],i;
	uint32_t	canID;

	osEvent		evt;


	clearRXCanMessage();
	clearTXCanMessage();

	HIGHCAN2_120OHM_ENABLE;

	IO_CONTROL_LOW( DLCA_EN1 );
	IO_CONTROL_LOW( DLCA_EN2 );
	IO_CONTROL_LOW( DLCA_EN3 );
	IO_CONTROL_HIGH( DLCA_EN6 );
	IO_CONTROL_LOW( DLCA_EN7 );
	IO_CONTROL_LOW( DLCA_EN8_1 );
	IO_CONTROL_LOW( DLCA_EN13 );
	IO_CONTROL_LOW( DLCA_EN15_1 );

	IO_CONTROL_LOW( DLCB_EN8 );
	IO_CONTROL_LOW( DLCB_EN9 );
	IO_CONTROL_LOW( DLCB_EN10 );
	IO_CONTROL_LOW( DLCB_EN11 );
	IO_CONTROL_LOW( DLCB_EN12 );
	IO_CONTROL_HIGH( DLCB_EN14 );
	IO_CONTROL_LOW( DLCB_EN15 );

	startFDCan( 1, 0, 0 );

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			//canID = ( pPayloadPtcl[1] << 8 ) + pPayloadPtcl[2];
			canID = 0x7FD;

			txPkt->mLen			= 8;
#if 0
			txPkt->pTarget		= &hfdcan2;
#else
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else			
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			
#endif
			for( i = 0; i < 8; i++ )			txBuff[i] = txPkt->mData[i] = i+1;

#ifdef CANTEST_LOG
			GLogN( "HighCAN1 Tx : %04X ", canID );
			for( i = 0; i < 8; i++ )
			{
				GLogN( "%02X ", txBuff[i] );
			}
			GLogN( "\r\n" );
#endif

#if 0
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_FDCAN );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			// Rx
			for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			{
				evt = osMessageGet( hFDRxMsg, 1000 );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					if( ( rxPkt->pSource == &hfdcan1 ) &&
#else
					if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5) &&
#endif
						( rxPkt->mRxHeader.Identifier == canID ) &&
						( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					{
						//packet->mData[1] = TEST_OK;
#ifdef CANTEST_LOG
						GLogN( "TEST_OK\r\n", canID );
#endif
						ret=1;
					}
#ifdef CANTEST_LOG
					GLogN( "HighCAN1 Rx : %04X ", canID );
					for( i = 0; i < 8; i++ )
					{
						GLogN( "%02X ", rxPkt->mData[i] );
					}
					GLogN( "\r\n" );
#endif
					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
			}
		}
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );

	return ret;

}

/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/
static void testCertifyThread( void const *argument )
{
	osEvent		evt;
	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint32_t	i;

	error_t error;
	uint8_t recevieBuf[9] = {0,};

	extern int8_t gModule_step;

	//if( g_TriggerKey == 1 )											// Ethernet T1
	//{
	//	g_ethSocket.state = ethTCPConnect(&g_ethSocket);
	//}

	for( ;; )
	{
		#if 0
		if(gModule_step == 3 )
		{
			DisableEthDiag();

			stMsgClst	*txMsg, *rxMsg;
			stFdcanPkt	*txPkt, *rxPkt;
			SpiCanPkt_t *rxPktSPI;

			uint8_t		txBuff[8];
			uint32_t	canID;
			
			//GLogI( "[MODE_HCAN1]\r\n");

			clearRXCanMessage();
			clearTXCanMessage();

			HIGHCAN2_120OHM_ENABLE;
			
			CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );//for HCAN2
			setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x0, 0x7ff );//for HCAN2
#if 0
			IOCanSetLoopback();
#else
			//HCAN1(6/14), HCAN2(1/9) short
			IO_CONTROL_HIGH( DLCA_EN1 );
			IO_CONTROL_LOW( DLCA_EN2 );
			IO_CONTROL_LOW( DLCA_EN3 );
			IO_CONTROL_HIGH( DLCA_EN6 );
			IO_CONTROL_LOW( DLCA_EN7 );
			IO_CONTROL_LOW( DLCA_EN8_1 );
			IO_CONTROL_LOW( DLCA_EN13 );
			IO_CONTROL_LOW( DLCA_EN15_1 );

			IO_CONTROL_LOW( DLCB_EN8 );
			IO_CONTROL_HIGH( DLCB_EN9 );
			IO_CONTROL_LOW( DLCB_EN10 );
			IO_CONTROL_LOW( DLCB_EN11 );
			IO_CONTROL_LOW( DLCB_EN12 );
			IO_CONTROL_HIGH( DLCB_EN14 );
			IO_CONTROL_LOW( DLCB_EN15 );
#endif

			startFDCan( 1, 1, 0 );

			// Tx
			txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
			if( txMsg != NULL )
			{
				txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
				if( txPkt != NULL )
				{
					canID = 0x0700;

					txPkt->mLen			= 8;

					txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;

					for( i = 0; i < 8; i++ )			txBuff[i] = txPkt->mData[i] = i+1;
#ifdef CANTEST_LOG
					GLogN( "HCAN1 Tx : %04X ", canID );
					for( i = 0; i < 8; i++ )
					{
						GLogN( "%02X ", txBuff[i] );
					}
					GLogN( "\r\n" );
#endif
					makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );

					txMsg->pPacket	= (void *)txPkt;

					osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

					// 2. FDCan2 Rx
					for( i = 0; i < 3; i++ )				// Max wait 3 seconds
					{
						evt = osMessageGet( hFDRxMsg, 1000 );
						if( evt.status == osEventMessage )
						{
							rxMsg	= ( stMsgClst * )evt.value.p;
							rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

							if( ( rxPkt->pSource == &hfdcan1 ) &&
								( rxPkt->mRxHeader.Identifier == canID ) &&
								( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
							{
								GLogN( "C" );
								termDelay();
							}
#ifdef CANTEST_LOG
							GLogN( "HCAN2 Rx : %04X ", canID );
							for( i = 0; i < 8; i++ )
							{
								GLogN( "%02X ", rxPkt->mData[i] );
							}
							GLogN( "\r\n" );
#endif
							osPoolFree( hFdcanPktPool, (void *)rxPkt );
							osPoolFree( hMsgPool, (void *)rxMsg );

							break;
						}
					}
				}
			}

			HIGHCAN2_120OHM_DISABLE;

			SetKL_Line( 0, 0 );
			stopFDCan( &hfdcan1 );
		}
#endif
		/*

		if(gModule_step == 1) // Test Step WIFI Ethernet T1 send
		{

			// Eth(T1)
            g_stGITHWSetDataEth.nSourceIP_T1 = 0x0005000A;		//10.0.5.0
            g_stGITHWSetDataEth.nSourcePort_T1 = 52081;
            g_stGITHWSetDataEth.nDestinationIp_T1 = 0x0000200A;	//10.32.0.0
            g_stGITHWSetDataEth.nDestinationPort_T1 = 13402;
            g_ethSocket.state = eSOCK_STATE_INIT;
            eth_SetIPAddressStatic( RS9116_IPV4_ADDR( 10, 0, 5, 0 ), RS9116_IPV4_ADDR( 255, 0, 0, 0 ), RS9116_IPV4_ADDR( 10, 0, 128, 1 ) );

            setEthSocketState(&g_ethSocket, eSOCK_STATE_INIT);

            g_ethSocket.state = ethTCPConnect(&g_ethSocket);

            if(g_ethSocket.state == eSOCK_STATE_CONNECTED)
            {
               // GLogI( "Socket Connect!!! \r\n" );
                error = tcpSend(g_ethSocket.socket, txBuff, sizeof(txBuff), NULL, FALSE);
               // GLogI( "T1 : HELLO GIT \r\n" );


                error = socketReceive(g_ethSocket.socket, recevieBuf, sizeof(recevieBuf), NULL, FALSE);
                if( !error )
                {
                    GLogN( "E" );
					termDelay();
                }
            }
            else
            {
                GLogE( "Socket Connect Fail!!! \r\n" );
            }

            osDelay(1000);	//JIG socket close time

            socketShutdown(g_ethSocket.socket,2);//GLogI( "socketShutdown!!! \r\n" );
            socketClose(g_ethSocket.socket);//GLogI( "socketClose!!! \r\n" );
		}

		}
		*/
		//else //TX
		//{
#if 0 //mod.kks to test TX Packet
        if(gModule_step == 1)
		{
            EnableEthDiag();
            osDelay(10);
            EnableLAN9371();
            osDelay(5000);

            g_stGITHWSetDataEth.nSourceIP_T1 = 0x0005000A;		//10.0.5.0
            g_stGITHWSetDataEth.nSourcePort_T1 = 52081;
            g_stGITHWSetDataEth.nDestinationIp_T1 = 0x0000200A;	//10.32.0.0
            g_stGITHWSetDataEth.nDestinationPort_T1 = 13402;
            g_ethSocket.state = eSOCK_STATE_INIT;
            eth_SetIPAddressStatic( RS9116_IPV4_ADDR( 10, 0, 5, 0 ), RS9116_IPV4_ADDR( 255, 0, 0, 0 ), RS9116_IPV4_ADDR( 10, 0, 128, 1 ) );

            GLogI( "Source IP : %s \r\n",APP_IF1_IPV4_HOST_ADDR );
            GLogI( "Source PORT : %d \r\n",TCP_LOCL_PORT );
            GLogI( "Source SUBNET_MASK : %s \r\n",APP_IF1_IPV4_SUBNET_MASK );
            GLogI( "Source GATEWAY : %s \r\n",APP_IF1_IPV4_DEFAULT_GATEWAY );
            GLogI( "\r\n");
            GLogI( "Target IP : %s \r\n",TCP_TARGET_NAME );
            GLogI( "Target PORT : %d \r\n",TCP_TARGET_PORT );

            setEthSocketState(&g_ethSocket, eSOCK_STATE_INIT);

            g_ethSocket.state = ethTCPConnect(&g_ethSocket);

            if(g_ethSocket.state == eSOCK_STATE_CONNECTED)
            {
                GLogI( "Socket Connect!!! \r\n" );
                error = tcpSend(g_ethSocket.socket, txBuff, sizeof(txBuff), NULL, FALSE);
                GLogI( "TX : HELLO GIT \r\n" );


                error = socketReceive(g_ethSocket.socket, recevieBuf, sizeof(recevieBuf), NULL, FALSE);
                if( !error )
                {
                    GLogN( "E \r\n" );
                }
            }
            else
            {
                GLogE( "Socket Connect Fail!!! \r\n" );
            }

            osDelay(1000);	//JIG socket close time

            socketShutdown(g_ethSocket.socket,2);GLogI( "socketShutdown!!! \r\n" );
            socketClose(g_ethSocket.socket);GLogI( "socketClose!!! \r\n" );
			DisableEthDiag();
        }

#else

/*
			if(gModule_step == 1)// TX Ping Test
			{ 
				EnableEthDiag();

				for( i = 0; i < 25; i++ )
				{
					GLogN( "E" );
					termDelay();
				}
				DisableEthDiag();
			}
		
*/
#endif
			EnableEthDiag();

#if 1	
			if(gModule_step == 1)// Test Step CAN
			{
				if( testCanTxRxTest() == 1 )
				{
					GLogN( "C" );
					termDelay();
				}
			}
#endif

		//}

		if(gModule_step == 3)// Test Step KLINE
		{ 
			if( SelfTest_KLINE() == 0 )
			{
				GLogN( "K" );
			}
			termDelay();
		}

		if(gModule_step == 5)// Test Step BT
		{ 
			// BT Spp Send
			if( g_ucBtConnected == BT_SPP_CONNECT )
			{
				bt_spp_transfer( txBuff, 9 );
				GLogN( "B" );
			}
			termDelay();
		}
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#endif
	}
}
#endif	// VCI_III_ASING_MODE
