/*************************************************************
 * NOTE : git_i2c.c
 *      I2C control
 * Author : Lee junho
 * Since : 2019.09.03
**************************************************************/
#include "main.h"

#include "common.h"
#include "typedef.h"
#include "git_i2c.h"

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void scanI2C( I2C_HandleTypeDef *hi2c )
{
	HAL_StatusTypeDef	result;
	uint8_t	i;
	uint8_t	j;

	GLogN( "Scanning I2C bus:\r\n" );
	for( i = 0; i < 128; i += 16 )
	{
		for( j = 0; j< 16; j++ )
		{
			result = HAL_I2C_IsDeviceReady( hi2c, (uint16_t)((i+j) << 1), 2, 2 );
			if( result != HAL_OK )
			{
				GLogN( "." );						// No ACK received at that address
			}
			else
			{
				GLogN( "0x%X", i+j );				// Received an ACK at that address
			}
		}

		GLogN( "\r\n" );
	}
}