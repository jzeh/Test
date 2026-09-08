#ifndef __GIT_CLI_H__
#define __GIT_CLI_H__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------
	includes
----------------------------------------------------------------------*/
#include "usart.h"

#include "typedef.h"

/*----------------------------------------------------------------------
	defines
----------------------------------------------------------------------*/
#define DUMMY_SIZE	1024
/*----------------------------------------------------------------------
	delay
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
	print
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
	common functions
----------------------------------------------------------------------*/
extern int32_t	InitCli( void );
extern int32_t	StartCli( void );
extern void		DeinitCli( void );

extern void		sendCliData( void );

/*----------------------------------------------------------------------
	Variables
----------------------------------------------------------------------*/

extern bool g_bBTLogOnTxFlag;
extern bool g_bBTLogOnRxFlag;
extern bool g_bwebLogOnTxFlag;
extern bool g_bwebLogOnRxFlag;
extern bool g_bMqttLogOnTxFlag;
extern bool g_bMqttLogOnRxFlag;
extern bool g_bEncryptLogOnFlag;
extern uint32_t tt_start;
extern uint32_t tt_end;
#ifdef __cplusplus
}
#endif

#endif /* __GIT_CLI_H__ */