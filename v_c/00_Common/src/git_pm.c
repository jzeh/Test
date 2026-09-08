/*************************************************************
 * NOTE : git_pm.c
 *      Power Management
 * Author : Lee junho
 * Since : 2019.09.03
**************************************************************/
#include "main.h"
#include "gpio.h"

#include "common.h"
#include "typedef.h"
#include "git_ioctl.h"
#include "git_sensor.h"

#include "git_pm.h"

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
PWREx_WakeupPinTypeDef sPinParams;

uint8_t		gStanbyExitFlag	= 0;
uint32_t	gPMFlag			= 0;

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void setWakeUpSource( uint32_t flag )
{
	// CLK Low
	IO_CONTROL_LOW( LAT_CLK );
	HAL_Delay( 10 );
	
	// Wake source set
	if( flag & WAKEUP_SOURCE_IG )		IO_CONTROL_LOW( LAT_IG_DET_EN );
	else								IO_CONTROL_HIGH( LAT_IG_DET_EN );

#ifdef ADDTOLATCH_WIFI // mod.kks 22.05.01   Do not wake-up resource. only Latch connection
	// Wake source set
	if( flag & LAT_WIFI_BT_PWR_EN )		IO_CONTROL_HIGH( WIFI_BT_PWR_EN);
	else								IO_CONTROL_LOW( WIFI_BT_PWR_EN);
#endif

	if( flag & WAKEUP_SOURCE_SENSOR )
	{
		if 		(g_GyroSensorType == GYRO_SENSOR_MPU6515)  MPU6515_SetupForInterruptforImpulse( 1, 1, 0x0F, 0 );
        else if (g_GyroSensorType == GYRO_SENSOR_IIM42652) IIM42652_SetupForInterruptforImpulse( 1, 1, 300, 0 );
		else {} // check 
		IO_CONTROL_LOW( LAT_SENS_EN );
	}
	else	IO_CONTROL_HIGH( LAT_SENS_EN );
	
	//	if( flag & WAKEUP_SOURCE_HCAN1 )
//	{
//		IO_CONTROL_LOW( H_CAN2_SW_EN );
//		IO_CONTROL_LOW( LAT_H_CAN_RX_EN1 );
//	}
//	else
//	{
//		IO_CONTROL_HIGH( H_CAN2_SW_EN );
//		IO_CONTROL_HIGH( LAT_H_CAN_RX_EN1 );
//	}
//
//	if( flag & WAKEUP_SOURCE_HCAN2 )
//	{
//		IO_CONTROL_HIGH( H_CAN2_SW_EN );
//		IO_CONTROL_HIGH( LAT_H_CAN_RX_EN1 );
//	}
//	else
//	{
//		IO_CONTROL_LOW( H_CAN2_SW_EN );
//		IO_CONTROL_LOW( LAT_H_CAN_RX_EN1 );
//	}
	
	if( flag & WAKEUP_SOURCE_HCAN1 || flag & WAKEUP_SOURCE_HCAN2 )
	{
		if( flag & WAKEUP_SOURCE_HCAN1 )	// LJS : // Signal received only from CAN2, so LAT_H_CAN_RX_EN2 control is needed for both HCAN1 and HCAN2
		{
			
			IO_CONTROL_LOW( H_CAN2_SW_EN );
			IO_CONTROL_LOW( LAT_H_CAN_RX_EN1 );
			IO_CONTROL_LOW( LAT_H_CAN_RX_EN2 );
		}
		else
		{
			IO_CONTROL_HIGH( H_CAN2_SW_EN );
			IO_CONTROL_HIGH( LAT_H_CAN_RX_EN1 );
			IO_CONTROL_LOW( LAT_H_CAN_RX_EN2 );
		}
	}
	else
	{
		IO_CONTROL_HIGH( LAT_H_CAN_RX_EN1 );
	  	IO_CONTROL_HIGH( LAT_H_CAN_RX_EN2 );
		IO_CONTROL_HIGH( LAT_H_CAN_RX_EN2 );
	}
	
	if( flag & WAKEUP_SOURCE_LCAN )
	{
		EnableLowCan2();	 // for sleep & wake problem  
		DisableLowCan2();	 // for sleep & wake problem
		IO_CONTROL_LOW( LAT_L_CAN_RX_EN );
	}
	else								IO_CONTROL_HIGH( LAT_L_CAN_RX_EN );

	if( flag & WAKEUP_SOURCE_TRG )		IO_CONTROL_LOW( TRG_WAK_UP_EN );
	else								IO_CONTROL_HIGH( TRG_WAK_UP_EN );

	if( flag & WAKEUP_SOURCE_12V_DET )	IO_CONTROL_LOW( PWR_DET_EN_12V );
	else								IO_CONTROL_HIGH( PWR_DET_EN_12V );

	if( flag & WAKEUP_SOURCE_24V_DET )	IO_CONTROL_LOW( PWR_DET_EN_24V );
	else								IO_CONTROL_HIGH( PWR_DET_EN_24V );

	HAL_Delay( 10 );
	
	
	
	for( int i = 0; i < 3; i++ )
	{
		// CLK TOGGLE
		IO_CONTROL_HIGH( LAT_CLK );
		HAL_Delay( 10 );
		IO_CONTROL_LOW( LAT_CLK );
		HAL_Delay( 10 );
	}


}

void gotoStandbyMode( uint32_t flag )
{
	setWakeUpSource( flag );
	
	/* Disable used wakeup source: PWR_WAKEUP_PIN1 */
	HAL_PWR_DisableWakeUpPin( PWR_WAKEUP_PIN1 );

	/* Clear all related wakeup flags */
	HAL_PWREx_ClearWakeupFlag( PWR_WAKEUP_PIN_FLAGS );

	/* Enable WakeUp Pin PWR_WAKEUP_PIN1 connected to PA.00 */
	sPinParams.WakeUpPin	= PWR_WAKEUP_PIN1;
	sPinParams.PinPolarity	= PWR_PIN_POLARITY_HIGH;
	sPinParams.PinPull		= PWR_PIN_PULL_DOWN;
	HAL_PWREx_EnableWakeUpPin( &sPinParams );

	GLogI( "Enter Standby Mode1!!!\r\n" );

	HAL_PWR_EnterSTANDBYMode();

	GLogI( "Enter Standby Mode2!!!\r\n" );
}

void initPowerManagement( void )
{
	/* Check if the system was resumed from Standby mode */
	if( __HAL_PWR_GET_FLAG( PWR_FLAG_SB ) != RESET )
	{
		/* Clear Standby flag */
		__HAL_PWR_CLEAR_FLAG( PWR_FLAG_SB );
		GLogI( "Standby Mode exit\r\n" );

		gStanbyExitFlag = 1;
	}
	else
	{
		gStanbyExitFlag = 0;
	}

	//setWakeUpSource( 0xff ); mod.kks todo check remove //
}
