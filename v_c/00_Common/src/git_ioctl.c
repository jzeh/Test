/*************************************************************
 * NOTE : git_gpio.c
 *      IO control
 * Author : Lee junho
 * Since : 2021.08.13
**************************************************************/
#include "common.h"
#include "typedef.h"
#include "git_ioctl.h"

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void InitIOCTL( void )
{
	SetKL_Line( 0, 0 );				// set KL line All off
	SetReprogramVol( 0 );			// set Reprogram All off

	DisableHighCan1();
	DisableHighCan2();
	//DisableEthDiag();				// Eth Diag All off

	KL_TXD1_INV_DISABLE;
	KL_TXD2_INV_DISABLE;
	KL_RXD1_INV_DISABLE;
	KL_RXD2_INV_DISABLE;

	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;

	HIGHCAN2_120OHM_DISABLE;
	IG_ON_DETECT_ENABLE;
	TRIGGER_LED_OFF;
}

/**********************************************************************/
/***   EMMC Control   ************************************************/
/**********************************************************************/
void EnableEmmc( void )
{
	IO_CONTROL_HIGH( EMMC_RST );
	IO_CONTROL_LOW( EMMC_PWR_EN );
	HAL_Delay( 10 );
	IO_CONTROL_HIGH( EMMC_PWR_EN );
	HAL_Delay( 10 );
	IO_CONTROL_LOW( EMMC_RST );
	HAL_Delay( 10 );
	IO_CONTROL_HIGH( EMMC_RST );
	HAL_Delay( 10 );
}

void DisableEmmc( void )
{
	IO_CONTROL_HIGH( EMMC_RST );
	IO_CONTROL_LOW( EMMC_PWR_EN );
}
/**********************************************************************/
/***   Wireless(Wifi/BT) Control   ************************************/
/**********************************************************************/
#if 0
void EnableRS9116( void )
{
	// CLK Low
#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_LOW( LAT_CLK );
#endif

	IO_CONTROL_LOW( WIFI_BT_PWR_EN );  //Latch PIN Control

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
#endif

	HAL_Delay( 10 );
	IO_CONTROL_HIGH( WIFI_REST );
	HAL_Delay( 10 );

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_LOW( LAT_CLK );
#endif

	IO_CONTROL_HIGH( WIFI_BT_PWR_EN ); //Latch PIN Control

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
#endif

	HAL_Delay( 10 );
	IO_CONTROL_LOW( WIFI_REST );
	HAL_Delay( 100 );
	IO_CONTROL_HIGH( WIFI_REST );
	HAL_Delay( 10 );
}
#else
void EnableRS9116( void )
{
	// CLK Low
	IO_CONTROL_LOW( LAT_CLK );

	IO_CONTROL_LOW( WIFI_BT_PWR_EN );  //Latch PIN Control	
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
	
	IO_CONTROL_LOW( WIFI_REST );
	HAL_Delay( 100 );
	
	IO_CONTROL_HIGH( WIFI_BT_PWR_EN ); //Latch PIN Control
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
	

	IO_CONTROL_HIGH( LAT_CLK );
	HAL_Delay( 10 );
	IO_CONTROL_LOW( LAT_CLK );
	HAL_Delay( 10 );
}
#endif
void DisableRS9116( void )
{
#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_LOW( LAT_CLK );
#endif

	IO_CONTROL_LOW( WIFI_BT_PWR_EN ); // Latch PIN Control

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
#endif

	IO_CONTROL_HIGH( WIFI_REST );
}

/**********************************************************************/
/***   HSM Control   **************************************************/
/**********************************************************************/
void EnableHSM( void )
{
	IO_CONTROL_LOW( HSM_PWR_EN );
	IO_CONTROL_HIGH( HSM_RST );
	HAL_Delay( 10 );
	IO_CONTROL_HIGH( HSM_PWR_EN );
	HAL_Delay( 10 );
	IO_CONTROL_LOW( HSM_RST );
	HAL_Delay( 100 );
	IO_CONTROL_HIGH( HSM_RST );
	HAL_Delay( 10 );
}

void DisableHSM( void )
{
	IO_CONTROL_LOW( HSM_PWR_EN );
	IO_CONTROL_HIGH( HSM_RST );
}
#ifdef LAN_9514
void EnableUSB3300(void)
{
	IO_CONTROL_LOW(USB_PHY_PWR_EN);
	USB_PHY_RST_ENABLE;
	HAL_Delay( 10 );
	IO_CONTROL_HIGH(USB_PHY_PWR_EN);
	USB_PHY_RST_DISABLE;
	HAL_Delay( 10 );
	USB_PHY_RST_ENABLE;
	HAL_Delay( 10 );
	USB_PHY_RST_DISABLE;
	HAL_Delay( 10 );
}

void DisableUSB3300(void)
{
	IO_CONTROL_LOW(USB_PHY_PWR_EN);
	USB_PHY_RST_ENABLE;
}

void EnableLAN9514(void)
{
	ETH_PWR_EN_DISENABLE;
	USB_ETH_NRST_ENABLE;
	HAL_Delay( 10 );
	ETH_PWR_EN_ENABLE;
	USB_ETH_NRST_DISABLE;
	HAL_Delay( 10 );
	USB_ETH_NRST_ENABLE;
	HAL_Delay( 10 );
	USB_ETH_NRST_DISABLE;
	HAL_Delay( 10 );
}

void DisableLAN9514(void)
{
	ETH_PWR_EN_DISENABLE;
	USB_ETH_NRST_ENABLE;
}

/*mod.kks 2023.03.22 for test. */
void EnableGL850G(void)
{
	USB_HUB_PWR_DISABLE;
	USB_HUB_RST_ENABLE;
	HAL_Delay( 10 );
	USB_HUB_PWR_ENABLE;
	USB_HUB_RST_DISABLE;
	HAL_Delay( 10 );
	USB_HUB_RST_ENABLE;
	HAL_Delay( 10 );
	USB_HUB_RST_DISABLE;
	HAL_Delay( 10 );
}

void DisableGL850G(void)
{
	USB_HUB_PWR_DISABLE;
	USB_HUB_RST_ENABLE;
}
/*end*/

#else
/**********************************************************************/
/***   Ethernet Dignosis Control   ************************************/
/**********************************************************************/
void EnableLAN9371( void )
{
	ETH_PWR_EN_DISENABLE;
	RMII_RSTN_ENABLE;
	HAL_Delay( 10 );
	ETH_PWR_EN_ENABLE;
	HAL_Delay( 10 );
	RMII_RSTN_DISABLE;
	HAL_Delay( 100 );
	RMII_RSTN_ENABLE;
	HAL_Delay( 10 );
}

void DisableLAN9371( void )
{
	ETH_PWR_EN_DISENABLE;
	RMII_RSTN_ENABLE;
}
#endif

void EnableEthDiag( void )
{
	//InitIOCTL();// Delete to keep ethernet activation

	IO_CONTROL_HIGH( ETH_SW_EN );					// Diag Line(ch1, ch9, ch12, ch13) Connect
}

void DisableEthDiag( void )
{
	IO_CONTROL_LOW( ETH_SW_EN );					// Diag Line(ch1, ch9, ch12, ch13) Disconnect
}

/**********************************************************************/
/***   Reprogram Channel Control   ************************************/
/**********************************************************************/
void SetReprogramVol( uint8_t line )
{
//	GLogN( "[%s] line(%d)\r\n", __FUNCTION__, line );

	IO_CONTROL_HIGH( REPG_ON );
	//REPG_LINE_DISABLE;

	switch( line )
	{
		case REPG_LINE_CH03 :			IO_CONTROL_HIGH( REPG_CH3 );				break;
		case REPG_LINE_CH06 :			IO_CONTROL_HIGH( REPG_CH6 );				break;
		case REPG_LINE_CH09 :			IO_CONTROL_HIGH( REPG_CH9 );				break;
		case REPG_LINE_CH11 :			IO_CONTROL_HIGH( REPG_CH11 );			break;
		case REPG_LINE_CH12 :			IO_CONTROL_HIGH( REPG_CH12 );			break;
		case REPG_LINE_CH13 :			IO_CONTROL_HIGH( REPG_CH13 );			break;
		case REPG_LINE_CH14 :			IO_CONTROL_HIGH( REPG_CH14 );			break;
		default :
		{
			IO_CONTROL_LOW( REPG_ON  );
			REPG_LINE_DISABLE;
			if( line != 0 )				GLogEE( "Unknown Line\r\n" );
		}
	}
}

/**********************************************************************/
/***   KL_Line Control   **********************************************/
/**********************************************************************/
void SetKL_Line( uint8_t line1, uint8_t line2 )
{
//	GLogN( "[%s] line1(0x%02x), line2(0x%02x) \r\n", __FUNCTION__, line1, line2 );

	// All Line Clear
	IO_CONTROL_LOW( DLCA_EN1 );
	IO_CONTROL_LOW( DLCA_EN2 );
	IO_CONTROL_LOW( DLCA_EN3 );
	IO_CONTROL_LOW( DLCA_EN6 );
	IO_CONTROL_LOW( DLCA_EN7 );
	IO_CONTROL_LOW( DLCA_EN8_1 );
	IO_CONTROL_LOW( DLCA_EN13 );
	IO_CONTROL_LOW( DLCA_EN15_1 );

	IO_CONTROL_LOW( DLCB_EN8 );
	IO_CONTROL_LOW( DLCB_EN9 );
	IO_CONTROL_LOW( DLCB_EN10 );
	IO_CONTROL_LOW( DLCB_EN11 );
	IO_CONTROL_LOW( DLCB_EN12 );
	IO_CONTROL_LOW( DLCB_EN14 );
	IO_CONTROL_LOW( DLCB_EN15 );

	// KL_Line 1
	switch( line1 )
	{
		case KL_LINE1_CONNECT_CH01 :			IO_CONTROL_HIGH( DLCA_EN1 );									break;
		case KL_LINE1_CONNECT_CH02 :			IO_CONTROL_HIGH( DLCA_EN2 );									break;
		case KL_LINE1_CONNECT_CH03 :			IO_CONTROL_HIGH( DLCA_EN3 );									break;
		case KL_LINE1_CONNECT_CH06 :			IO_CONTROL_HIGH( DLCA_EN6 );									break;
		case KL_LINE1_CONNECT_CH07 :			IO_CONTROL_HIGH( DLCA_EN7 );									break;
		case KL_LINE1_CONNECT_CH08 :			IO_CONTROL_HIGH( DLCA_EN8_1 );									break;
		case KL_LINE1_CONNECT_CH13 :			IO_CONTROL_HIGH( DLCA_EN13 );									break;
		case KL_LINE1_CONNECT_CH15 :			IO_CONTROL_HIGH( DLCA_EN15_1 );									break;
		case KL_LINE1_CONNECT_CH01_CH08 :		IO_CONTROL_HIGH( DLCA_EN1 );	IO_CONTROL_HIGH( DLCA_EN8_1 );	break;
		case KL_LINE1_CONNECT_CH02_CH08 :		IO_CONTROL_HIGH( DLCA_EN2 );	IO_CONTROL_HIGH( DLCA_EN8_1 );	break;
		case KL_LINE1_CONNECT_CH03_CH08 :		IO_CONTROL_HIGH( DLCA_EN3 );	IO_CONTROL_HIGH( DLCA_EN8_1 );	break;
		case KL_LINE1_CONNECT_CH06_CH08 :		IO_CONTROL_HIGH( DLCA_EN6 );	IO_CONTROL_HIGH( DLCA_EN8_1 );	break;
		case KL_LINE1_CONNECT_CH07_CH08 :		IO_CONTROL_HIGH( DLCA_EN7 );	IO_CONTROL_HIGH( DLCA_EN8_1 );	break;
		case KL_LINE1_CONNECT_CH13_CH08 :		IO_CONTROL_HIGH( DLCA_EN13 );	IO_CONTROL_HIGH( DLCA_EN8_1 );	break;
		case KL_LINE1_CONNECT_CH15_CH08 :		IO_CONTROL_HIGH( DLCA_EN15_1 );	IO_CONTROL_HIGH( DLCA_EN8_1 );	break;
		default :								if( line1 != 0 )			GLogEE( "Unknown Line 1\r\n" );
	}

	// KL_Line 2
	switch( line2 )
	{
		case KL_LINE2_CONNECT_CH08 :			IO_CONTROL_HIGH( DLCB_EN8 );									break;
		case KL_LINE2_CONNECT_CH09 :			IO_CONTROL_HIGH( DLCB_EN9 );									break;
		case KL_LINE2_CONNECT_CH10 :			IO_CONTROL_HIGH( DLCB_EN10 );									break;
		case KL_LINE2_CONNECT_CH11 :			IO_CONTROL_HIGH( DLCB_EN11 );									break;
		case KL_LINE2_CONNECT_CH12 :			IO_CONTROL_HIGH( DLCB_EN12 );									break;
		case KL_LINE2_CONNECT_CH14 :			IO_CONTROL_HIGH( DLCB_EN14 );									break;
		case KL_LINE2_CONNECT_CH15 :			IO_CONTROL_HIGH( DLCB_EN15 );									break;
		case KL_LINE2_CONNECT_CH08_CH15:			IO_CONTROL_HIGH( DLCB_EN8  );	IO_CONTROL_HIGH( DLCB_EN15 );	break;
		case KL_LINE2_CONNECT_CH09_CH15 :		IO_CONTROL_HIGH( DLCB_EN9  );	IO_CONTROL_HIGH( DLCB_EN15 );	break;
		case KL_LINE2_CONNECT_CH10_CH15 :		IO_CONTROL_HIGH( DLCB_EN10 );	IO_CONTROL_HIGH( DLCB_EN15 );	break;
		case KL_LINE2_CONNECT_CH11_CH15 :		IO_CONTROL_HIGH( DLCB_EN11 );	IO_CONTROL_HIGH( DLCB_EN15 );	break;
		case KL_LINE2_CONNECT_CH12_CH15 :		IO_CONTROL_HIGH( DLCB_EN12 );	IO_CONTROL_HIGH( DLCB_EN15 );	break;
		case KL_LINE2_CONNECT_CH14_CH15 :		IO_CONTROL_HIGH( DLCB_EN14 );	IO_CONTROL_HIGH( DLCB_EN15 );	break;
		default :								if( line2 != 0 )			GLogEE( "Unknown LineDLCB_EN8 2\r\n" );
	}
}

/**********************************************************************/
/***   CAN_Line Control   *********************************************/
/**********************************************************************/

void EnableHighCan1( void )
{
#ifdef CAN_LOG
	GLogN( "[GIT_IOCTL] +%s\r\n", __FUNCTION__ );
#endif

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_LOW( LAT_CLK );
#endif

	DisableEthDiag();
#ifdef USE_INTERNAL_CAN_ONLY
	IO_CONTROL_LOW( H_STB_EN2 );
	IO_CONTROL_LOW( HL_CAN_SW_EN );
	IO_CONTROL_LOW( H_CAN2_SW_EN);
	IO_CONTROL_LOW( LAT_H_CAN_RX_EN1);
#else
	IO_CONTROL_LOW( H_STB_EN1 );
	IO_CONTROL_LOW( H_CAN2_SW_EN);
	IO_CONTROL_LOW( LAT_H_CAN_RX_EN1);
#endif
	
#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
#endif
}

void DisableHighCan1( void )
{
#ifdef CAN_LOG
	GLogN( "[GIT_IOCTL] +%s\r\n", __FUNCTION__ );
#endif

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_LOW( LAT_CLK );
#endif

#ifdef USE_INTERNAL_CAN_ONLY
	IO_CONTROL_HIGH( H_STB_EN2 );
	IO_CONTROL_LOW( H_CAN2_SW_EN);
	IO_CONTROL_HIGH( LAT_H_CAN_RX_EN1);
#else
	IO_CONTROL_HIGH( H_STB_EN1 );
	IO_CONTROL_HIGH( H_CAN2_SW_EN);
	IO_CONTROL_HIGH( LAT_H_CAN_RX_EN1);
#endif	
	
#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
#endif
}

void EnableHighCan2( void )
{
#ifdef CAN_LOG
	GLogN( "[GIT_IOCTL] +%s\r\n", __FUNCTION__ );
#endif

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_LOW( LAT_CLK );
#endif

	DisableEthDiag();
	IO_CONTROL_LOW( H_STB_EN2 );
	IO_CONTROL_LOW( HL_CAN_SW_EN );
	IO_CONTROL_HIGH( H_CAN2_SW_EN);
	IO_CONTROL_LOW( LAT_H_CAN_RX_EN2);

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
#endif

}

void DisableHighCan2( void )
{
#ifdef CAN_LOG 
	GLogN( "[GIT_IOCTL] +%s\r\n", __FUNCTION__ );
#endif

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_LOW( LAT_CLK );
#endif

#ifdef USE_INTERNAL_CAN_ONLY
	IO_CONTROL_LOW( H_CAN2_SW_EN);
	IO_CONTROL_LOW(HL_CAN_SW_EN);
	IO_CONTROL_HIGH( LAT_H_CAN_RX_EN2);
#else
	IO_CONTROL_HIGH( H_STB_EN2 );
	IO_CONTROL_LOW( H_CAN2_SW_EN);
	IO_CONTROL_HIGH( LAT_H_CAN_RX_EN2);
#endif	

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
#endif
}

void EnableLowCan2( void )
{
#ifdef CAN_LOG
	GLogN( "[GIT_IOCTL] +%s\r\n", __FUNCTION__ );
#endif
	// 220319 회로가 latch 뒤는low, gpio 는 high 가 되여야지 enable 이 됨
	
#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_LOW( LAT_CLK );
#endif

	DisableEthDiag();
	IO_CONTROL_HIGH( HL_CAN_SW_EN );
	LOW_CAN_NSTB_ENABLE;
	IO_CONTROL_HIGH(LOW_CAN_EN);
	IO_CONTROL_LOW(LAT_L_CAN_RX_EN);

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
#endif
}

void DisableLowCan2( void )
{
#ifdef CAN_LOG
	GLogN( "[GIT_IOCTL] +%s\r\n", __FUNCTION__ );
#endif

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
		IO_CONTROL_LOW( LAT_CLK );
#endif
	
	LOW_CAN_NSTB_DISABLE;
	IO_CONTROL_LOW(LOW_CAN_EN);
	IO_CONTROL_HIGH(LAT_L_CAN_RX_EN);

#ifdef ADDTOLATCH_WIFI //mod.kks 22.05.01
	IO_CONTROL_HIGH( LAT_CLK );
	IO_CONTROL_LOW( LAT_CLK );
#endif
}

void IOCanSetLoopback( void )
{
#ifdef CAN_LOG
	GLogN( "[GIT_IOCTL] +%s\r\n", __FUNCTION__ );
#endif
	SetKL_Line( 0, 0 );
	SetReprogramVol( 0 );

	HIGHCAN2_120OHM_ENABLE;

	IO_CONTROL_HIGH( DLCA_EN1 );
	IO_CONTROL_HIGH( DLCA_EN6 );

	IO_CONTROL_HIGH( DLCB_EN9 );
	IO_CONTROL_HIGH( DLCB_EN14 );

	EnableHighCan1();  
	EnableHighCan2();  
	DisableLowCan2();

	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;

}
void SetWakeUpPin_Input( void )
{
	//WAKE PIN 의HIGH LOW를 읽을경우는 GPIO_MODE_INPUT 으로 설정해야 함
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = WAK_UP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
void SetWakeUpPin_AF( void )// Alternate Function Input
{
	//WAKE UP PIN 으로 사용 시 AF로 설정 필요
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = WAK_UP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void EnableCan2OSC( void )
{
#ifdef CAN_LOG
	GLogN( "[GIT_IOCTL] +%s\r\n", __FUNCTION__ );
#endif
	// oscillator를 disable -> enable 시 controller도 같이 초기화 필요
	IO_CONTROL_HIGH(OSC_EN);
}
void DisableCan2OSC( void )
{
#ifdef CAN_LOG
	GLogN( "[GIT_IOCTL] +%s\r\n", __FUNCTION__ );
#endif
	IO_CONTROL_LOW(OSC_EN);
}
void EnableHSM_AT( void )
{
	HAL_GPIO_WritePin(HSM_PWR_EN_GPIO_Port, HSM_PWR_EN_Pin, GPIO_PIN_SET);
}
void DisableHSM_AT( void )
{
	HAL_GPIO_WritePin(HSM_PWR_EN_GPIO_Port, HSM_PWR_EN_Pin, GPIO_PIN_RESET);
}

