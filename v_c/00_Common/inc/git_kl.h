#ifndef __GIT_KL_H__
#define __GIT_KL_H__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------
	includes
----------------------------------------------------------------------*/
#include "stream_buffer.h"

/*----------------------------------------------------------------------
	defines
----------------------------------------------------------------------*/
typedef enum
{
	KL_LINE1	= 0,
	KL_LINE2	= 1
} eKL_LINE;

/*----------------------------------------------------------------------
	delay
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
	print
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
	common functions
----------------------------------------------------------------------*/
extern int32_t	InitKL( void );
extern void		transmitKL( uint8_t line, uint8_t *data, uint8_t len );
extern void		transmitKL_ByteTime( uint8_t line, uint8_t *data, uint8_t len, uint32_t P4time );
extern void		clearKLReceiveData( uint8_t line );
extern uint32_t	getKLReceiveDataSize( uint8_t line );
extern uint32_t	getKLReceiveData( uint8_t line, uint8_t *data, uint32_t len );
uint32_t getKLReceiveData_Timeout( uint8_t line, uint8_t *data, uint32_t len, uint32_t uiTimeout );
extern bool transmitKL_ByteTime_wabco_abs( uint8_t line, uint8_t *data, uint8_t len, uint32_t P4time );
extern bool transmitKL_ByteTime_bosch( uint8_t line, uint8_t *data, uint8_t len, uint32_t P4time );
extern uint32_t getWabcoAbsRxBlock( uint8_t ucLine, uint8_t *ucData );
extern uint32_t getBoschRxBlock( uint8_t ucLine, uint8_t *ucData );
extern int32_t	SelfTest_KLINE( void );
extern int32_t SelfTest_KLINE_OBD_Connect( void );
extern int32_t SelfTest_KLINE_pullup( uint8_t ucPullup );
extern uint32_t GetKlineDataTime( uint8_t line, uint8_t *data );

extern void		Enable_KL_Interrupt( uint8_t line );
extern void		Disable_KL_Interrupt( uint8_t line );
extern int32_t uartSetBaudRate(uint8_t KLineSelect, uint32_t Baudrate );

/*----------------------------------------------------------------------
	Variables
----------------------------------------------------------------------*/
extern StreamBufferHandle_t		hSBKLRx1;
extern StreamBufferHandle_t		hSBKLRx2;

extern uint8_t	gucKL1RxDummy;
extern uint8_t	gucKL2RxDummy;

#ifdef __cplusplus
}
#endif

#endif /* __GIT_KL_H__ */
