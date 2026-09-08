/*************************************************************
 * NOTE : git_iwdg.c
 *      IWDG
 * Author : Lee woohee
 * Since : 2019.09.26
**************************************************************/
#include "main.h"
#include "common.h"
#include "git_iwdg.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define IWDG_HANDLER									hiwdg1

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
void ResetUsingIWDG( uint32_t mscnt );

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void ResetUsingIWDG( uint32_t mscnt )
{
	if( __HAL_RCC_GET_FLAG( RCC_FLAG_IWDG1RST ) != RESET )
	{
		/* Clear reset flags */
		__HAL_RCC_CLEAR_RESET_FLAGS();
	}

	if( mscnt > 4095 )	mscnt = 4095;

	IWDG_HANDLER.Instance		= IWDG1;
	IWDG_HANDLER.Init.Prescaler	= IWDG_PRESCALER_16;
	hiwdg1.Init.Window			= 0;
	hiwdg1.Init.Reload			= ( 32000 * mscnt ) / ( 16 * 1000 );		// mscnt(ms)

	if( HAL_IWDG_Init( &IWDG_HANDLER ) != HAL_OK )
	{
		GLogE( "Error... fail HAL_IWDG_Init!!!\r\n" );
		Error_Handler();
	}
}