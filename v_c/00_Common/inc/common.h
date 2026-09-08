#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------
	includes
----------------------------------------------------------------------*/
#include <stdio.h>
#include "stm32h7xx_hal.h"
#include "typedef.h"
/*----------------------------------------------------------------------
	defines
----------------------------------------------------------------------*/
#define	GIT_LOG_DEBUG															// Print를 사용하지 않을 경우 주석처리
#ifdef GIT_LOG_DEBUG
	#define DEBUG_COLOR										1
	#define DEBUG_ERROR										1
	#define DEBUG_INFO										1
	#define DEBUG_NORMAL									1
#endif

#define USE_DWT_DELAY										1
#define USE_BOARD_ID1										0
  
#define CANFD_qhyek  
#define K_LINE_BUFF_SIZE		2000
  
#define NEW_29BIT_CAN
/*----------------------------------------------------------------------
	Error Code
----------------------------------------------------------------------*/
#define ERROR_REPROGRAM_CH03								0x01
#define ERROR_REPROGRAM_CH06								0x02
#define ERROR_REPROGRAM_CH09								0x04
#define ERROR_REPROGRAM_CH11								0x08
#define ERROR_REPROGRAM_CH12								0x10
#define ERROR_REPROGRAM_CH13								0x20
#define ERROR_REPROGRAM_CH14								0x40

/*----------------------------------------------------------------------
	delay
----------------------------------------------------------------------*/
#if ( USE_DWT_DELAY )
	extern int32_t	DWT_Delay_Init( void );
	extern void		DWT_Delay_us( volatile uint32_t microseconds );
	extern void		DWT_Delay_ms( volatile uint32_t miliseconds );
#endif	// USE_DWT_DELAY

/*----------------------------------------------------------------------
	print
----------------------------------------------------------------------*/
#define PRINTF_UART_PORT							huart2

#ifdef GIT_LOG_DEBUG
	#if( DEBUG_NORMAL )
		#define GLogN( fmt, args... )			printf( fmt, ##args )															// Normal Print
	#endif
	#if( DEBUG_INFO )
		#if( DEBUG_COLOR )
			#define	GLogI( fmt, args... )		{ printf( "\033[33m" ); printf( fmt, ##args ); printf( "\033[0m" );}		// Info Print, Yellow
		#else
			#define	GLogI( fmt, args... )		printf( fmt, ##args )														// Info Print, Yellow
		#endif
	#endif
	#if( DEBUG_ERROR )
		#if( DEBUG_COLOR )
			#define	GLogE( fmt, args... )		{ printf( "\r\n\033[31m" ); printf( "%s : " fmt, __FUNCTION__ , ##args ); printf( "\033[0m" ); }	// Error Print, Red
			#define	GLogEE( fmt, args... )		{ printf( "\033[31m" ); printf( fmt, ##args ); printf( "\033[0m" ); }	// Error Print, Red
		#else
			#define	GLogE( fmt, args... )		printf( fmt, ##args )																			// Error Print, Red
			#define	GLogEE( fmt, args... )		printf( fmt, ##args )																			// Error Print, Red
		#endif
	#endif
#endif

#ifndef GLogN
	#define GLogN(x...)					do { } while(0);
#endif

#ifndef GLogI
	#define GLogI(x...)					do { } while(0);
#endif

#ifndef GLogE
	#define GLogE(x...)					do { } while(0);
	#define GLogEE(x...)				do { } while(0);
#endif

// Debug
#define	DEBUG_LOG_QUEUE											0x01
#define	DEBUG_LOG_FUNCTION_LIST									0x02
#define	DEBUG_LOG_INFORMATION									0x04
#define	DEBUG_LOG_COMMAND										0x08
#define	DEBUG_LOG_BYPASS										0x10
#ifdef USE_INTERNAL_CAN_ONLY
#else
//#define FEATURE_MCP2518FD										1
#endif

/*----------------------------------------------------------------------
	common functions
----------------------------------------------------------------------*/
extern uint8_t	getBoardID( void );

extern void		jumpToBootloader( void );
extern void 	jumpToBootloaderReset( uint32_t mscnt );

extern void		printMainCLKs( void );
extern void		printPeriCLKs( void );

extern uint8_t	HexToDec( uint8_t HexData );
extern uint8_t	AsciiToHex( uint8_t asciicode1, uint8_t asciicode2 );

/*----------------------------------------------------------------------
     Variables
----------------------------------------------------------------------*/
extern uint8_t	gDBGFlag;

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_H__ */
