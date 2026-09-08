/*************************************************************
* NOTE : common.c
*      bootloader, application ... common function define
* Author : Lee junho
* Since : 2020.03.16
**************************************************************/
#include "main.h"
#include "usart.h"
#include "adc.h"

#include "common.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#ifdef __GNUC__
	#define PUTCHAR_PROTOTYPE int __io_putchar( int ch )
#else
	#define PUTCHAR_PROTOTYPE int fputc( int ch, FILE *f )
#endif

#define USE_IWDG										0

#if( USE_IWDG )
#include "git_iwdg.h"
#endif

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
uint8_t	getBoardID( void );
void	jumpToBootloader( void );
void	jumpToBootloaderReset( uint32_t mscnt );
void	printMainCLKs( void );
void	printPeriCLKs( void );

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
uint8_t		gDBGFlag	= 0xFF;

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
#if 1
void HAL_Delay( uint32_t milliseconds )
{
	/* Initially clear flag */
	(void)SysTick->CTRL;

	while( milliseconds != 0 )
	{
		/* COUNTFLAG returns 1 if timer counted to 0 since the last flag read */
		milliseconds -= (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) >> SysTick_CTRL_COUNTFLAG_Pos;
	}
}
#endif

#if( USE_DWT_DELAY )
int32_t DWT_Delay_Init( void )
{
	/* Disable TRC */
	CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;	// ~0x01000000;

	/* Enable TRC */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;		// 0x01000000;

	/* Disable clock cycle counter */
	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;				// ~0x00000001;

	/* Enable clock cycle counter */
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;				// 0x00000001;

	/* Reset the clock cycle counter value */
	DWT->CYCCNT = 0;

	/* 3 NO OPERATION instructions */
	for( int i = 0; i < 100; i++ )
	{
		__ASM volatile ("NOP");
		__ASM volatile ("NOP");
		__ASM volatile ("NOP");
	}

	/* Check if clock cycle counter has started */
	if(DWT->CYCCNT)			return 0;					/*clock cycle counter started*/
	else					return -1;					/*clock cycle counter not started*/
}

/**
 * @brief This function provides a delay (in microseconds)
 * @param microseconds: delay in microseconds
 */
void DWT_Delay_us( volatile uint32_t microseconds )
{
	uint32_t clk_cycle_start = DWT->CYCCNT;
	uint32_t delay;

	/* Go to number of cycles for system */
	delay = microseconds * (HAL_RCC_GetSysClockFreq() / 1000000);

	/* Delay till end */
	while((DWT->CYCCNT - clk_cycle_start) < delay);
}

void DWT_Delay_ms( volatile uint32_t miliseconds )
{
	DWT_Delay_us( miliseconds * 1000 );
}
#endif	// USE_DWT_DELAY

/*---------------------------------------------------------------------------------------------
	Print
---------------------------------------------------------------------------------------------*/
PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit( &PRINTF_UART_PORT, (uint8_t *)&ch, 1, 0xFFFF );

	return ch;
}

/*---------------------------------------------------------------------------------------------
	Common Function
---------------------------------------------------------------------------------------------*/
uint8_t getBoardID( void )
{
	uint8_t	ucBoardID = 0;

#if( USE_BOARD_ID1 )
	//ucBoardID  = HAL_GPIO_ReadPin( BD_ID1_GPIO_Port, BD_ID1_Pin ) << 1;
#endif
	//ucBoardID += HAL_GPIO_ReadPin( BD_ID0_GPIO_Port, BD_ID0_Pin );

	switch( ucBoardID )
	{
		case 3 :			GLogI( "Board ID : 0x11\r\n" );			break;
		case 2 :			GLogI( "Board ID : 0x10\r\n" );			break;
		case 1 :			GLogI( "Board ID : 0x01\r\n" );			break;
		case 0 :
		default :			GLogI( "Board ID : 0x00\r\n" );			break;
	}

	return ucBoardID;
}

void jumpToBootloader( void )
{
	uint32_t i=0;
	void (*SysMemBootJump)(void);

	/* Set the address of the entry point to bootloader */
	volatile uint32_t BootAddr = 0x1FF09800;

	/* Disable all interrupts */
	__disable_irq();

	/* Disable Systick timer */
	SysTick->CTRL = 0;

	/* Set the clock to the default state */
	HAL_RCC_DeInit();

	/* Clear Interrupt Enable Register & Interrupt Pending Register */
	for( i = 0 ; i < 5 ; i++ )
	{
		NVIC->ICER[i] = 0xFFFFFFFF;
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

	/* Re-enable all interrupts */
	__enable_irq();

	/* Set up the jump to booloader address + 4 */
	SysMemBootJump = (void (*)(void)) (*((uint32_t *) ((BootAddr + 4))));

	/* Set the main stack pointer to the bootloader stack */
	__set_MSP(*(uint32_t *)BootAddr);

	/* Call the function to jump to bootloader location */
	SysMemBootJump();

	/* Jump is done successfully */
	while (1)
	{
		/* Code should never reach this loop */
	}
}


void jumpToBootloaderReset( uint32_t mscnt )
{
	uint32_t i=0;

	/* Disable all interrupts */
	__disable_irq();

	/* Disable Systick timer */
	SysTick->CTRL = 0;

	/* Set the clock to the default state */
	HAL_RCC_DeInit();

	/* Clear Interrupt Enable Register & Interrupt Pending Register */
	for (i=0;i<5;i++)
	{
		NVIC->ICER[i]=0xFFFFFFFF;
		NVIC->ICPR[i]=0xFFFFFFFF;
	}

	/* Re-enable all interrupts */
	__enable_irq();
#if( USE_IWDG )
	ResetUsingIWDG( mscnt );
#else
	HAL_NVIC_SystemReset();
#endif
	/* Jump is done successfully */
	while (1)
	{
		/* Code should never reach this loop */
	}
}


void printMainCLKs( void )
{
	GLogI( "==================================================\r\n" );
	GLogI( " CORE      = %9d, %6.2lf MHz\r\n", SystemCoreClock, (double)SystemCoreClock*1e-6 );
	GLogI( " HCLK      = %9d\r\n", HAL_RCC_GetHCLKFreq() );
	GLogI( " APB1      = %9d\r\n", HAL_RCC_GetPCLK1Freq() );
	GLogI( " APB2      = %9d\r\n", HAL_RCC_GetPCLK2Freq() );
	GLogI( "==================================================\r\n" );
}

void printPeriCLKs( void )
{
	PLL1_ClocksTypeDef PLL1_Clocks;
	PLL2_ClocksTypeDef PLL2_Clocks;
	int ck = SDMMC1->CLKCR & 0x3FF;

	HAL_RCCEx_GetPLL1ClockFreq(&PLL1_Clocks);
	HAL_RCCEx_GetPLL2ClockFreq(&PLL2_Clocks);

	GLogI( "==================================================\r\n" );
	GLogI( " PLL1_Q_CK = %9d, %6.2lf MHz\r\n", PLL1_Clocks.PLL1_Q_Frequency, (double)PLL1_Clocks.PLL1_Q_Frequency*1e-6 );
	GLogI( " PLL2_R_CK = %9d, %6.2lf MHz\r\n", PLL2_Clocks.PLL2_R_Frequency, (double)PLL2_Clocks.PLL2_R_Frequency*1e-6 );

#if 0		// sdmmc clock use PLL1Q
	GLogI( " SDMMC_CK  = %9d, %6.2lf MHz\r\n", ck, ((double)PLL1_Clocks.PLL1_Q_Frequency * 1e-6) / ( ck == 0 ? 1 : ( (double)(2.0 * ck) ) ) );
#else		// sdmmc clock use PLL2R
	GLogI( " SDMMC_CK  = %9d, %6.2lf MHz\r\n", ck, ((double)PLL2_Clocks.PLL2_R_Frequency * 1e-6) / ( ck == 0 ? 1 : ( (double)(2.0 * ck) ) ) );
#endif
	GLogI( "==================================================\r\n" );
}

uint8_t HexToDec( uint8_t HexData )
{
	uint8_t L_Nibble, H_Nibble, DecData;

	L_Nibble = HexData&0x0F;
	if( L_Nibble > 0x09 ) return 0xFF;

	H_Nibble = HexData>>4;
	if( H_Nibble > 0x09 ) return 0xFF;

	DecData = (H_Nibble*10)+L_Nibble;

	return DecData;
}

uint8_t AsciiToHex( uint8_t asciicode1, uint8_t asciicode2 )
{
	uint8_t hexcode, hexcode1, hexcode2;

	if( asciicode1 > 0x40 )
	{
		hexcode1 = asciicode1 - 0x37;
		if( hexcode1 > 0x0F ) hexcode1 = 0x00;
	}
	else if( asciicode1 >= 0x30 )
	{
		hexcode1 = asciicode1 - 0x30;
		if( hexcode1 > 0x09 ) hexcode1 = 0x00;
	}
	else
		hexcode1 = 0x00;

	if( asciicode2 > 0x40 )
	{
		hexcode2 = asciicode2 - 0x37;
		if( hexcode2 > 0x0F ) hexcode2 = 0x00;
	}
	else if( asciicode2 >= 0x30 )
	{
		hexcode2 = asciicode2 - 0x30;
		if( hexcode2 > 0x09 ) hexcode2 = 0x00;
	}
	else
		hexcode2 = 0x00;

	hexcode = ( hexcode1 << 4 ) + hexcode2;

	return hexcode;
}