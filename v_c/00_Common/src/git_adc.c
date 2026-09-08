/*************************************************************
 * NOTE : git_adc.c
 *      ADC control
 * Author : Lee junho
 * Since : 2021.08.11
**************************************************************/
#include "cmsis_os.h"
#include "main.h"

#include "common.h"
#include "typedef.h"
#include "git_ioctl.h"
#include "git_adc.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
// Battery ADC
#define	BATTERY_ADC_PORT								hadc1
#define REPROGRAM_ADC_PORT								hadc3

#define	ADC_VREF_VOLTAGE								3300
#define	ADC_RESOLUTION									16

// H/W partition coefficient 							0.09836 + a
#define VOLTAGE_PARTITION_COEFFICIENT1					10500//9836
#define VOLTAGE_PARTITION_COEFFICIENT2					100000

// Battery ADC linear correction coefficients (260521 regression of 4 sample units)
// Vin = (meas * GAIN_NUM - OFFSET) / SCALE   (clamped to 0 if negative)
#define BATT_ADC_CAL_GAIN_NUM							1047		// gain x 1000 (a = 1.047)
#define BATT_ADC_CAL_OFFSET								137000		// -b x 1000 (b = -137 mV)
#define BATT_ADC_CAL_SCALE								1000

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
uint32_t	readBatteryValue( void );

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
uint32_t readBatteryValue( void )
{
	u32 value;
	u32 vout;
	int32_t corrected;

	HAL_ADC_Start( &BATTERY_ADC_PORT );
	HAL_ADC_PollForConversion( &BATTERY_ADC_PORT, HAL_MAX_DELAY );
	value = HAL_ADC_GetValue( &BATTERY_ADC_PORT );
	HAL_ADC_Stop( &BATTERY_ADC_PORT );

	vout = (uint32_t)( ( ( ADC_VREF_VOLTAGE * value ) >> ADC_RESOLUTION ) * VOLTAGE_PARTITION_COEFFICIENT2 / VOLTAGE_PARTITION_COEFFICIENT1 );

	// Apply linear correction (260521 regression of 4 sample units): Vin = (meas * 1047 - 137000) / 1000
	corrected = (int32_t)vout * BATT_ADC_CAL_GAIN_NUM - BATT_ADC_CAL_OFFSET;
	vout = (corrected < 0) ? 0U : (uint32_t)(corrected / BATT_ADC_CAL_SCALE);

//	GLogN( "v = %d\r\n", vout );

	return vout;
}

uint32_t readReprogramVoltageValue( void )
{
	u32 value;
	u32 vout;

	IO_CONTROL_HIGH( REPG_ON  );

	osDelay( 20 );

	HAL_ADC_Start( &REPROGRAM_ADC_PORT );
	HAL_ADC_PollForConversion( &REPROGRAM_ADC_PORT, HAL_MAX_DELAY );
	value = HAL_ADC_GetValue( &REPROGRAM_ADC_PORT );
	HAL_ADC_Stop( &REPROGRAM_ADC_PORT );

	vout = (uint32_t)( ( ( ADC_VREF_VOLTAGE * value ) >> ADC_RESOLUTION ) * VOLTAGE_PARTITION_COEFFICIENT2 / VOLTAGE_PARTITION_COEFFICIENT1 );

	GLogN( "v = %d\r\n", vout );

	IO_CONTROL_LOW( REPG_ON  );

	return vout;
}
