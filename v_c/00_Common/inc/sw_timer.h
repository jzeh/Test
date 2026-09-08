/*----------------------------------------------------------------------
 *   SW Timer
 *--------------------------------------------------------------------*/
#ifndef __SW_TIMER_H__
#define __SW_TIMER_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "typedef.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define MAX_SW_TIMER									16
#define SEC(x)											(x * 1000)			// second unit
#define MSEC(x)											(x)					// mili second unit
#define TIMER_LOOP_INFINITE								0XFFFFFFFF

typedef void (*fnSWCallBack)(void);

typedef enum _eSWTimerMode
{
	eSWTimer_NONE,
	eSWTimer_ONESHOT,
	eSWTimer_INFINITE,
} eSWTimerMode;

typedef struct _stSWTimerInfo
{
	BOOL			bActive;
	eSWTimerMode	eTimerMode;
	U32				ui32SWtimerCnt;
	U32				ui32SaveTimerValue;
	fnSWCallBack	fp;
} stSWTimerInfo;

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
extern u32	healthcheckcnt;
extern u32	intDlccomCount;
/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
extern void	Internal_timer_Proc( void );
extern int	SetSWTimer( int nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, BOOL bTimerStart );
extern BOOL	StartSWTimer( u8 bTimerIndex, eSWTimerMode eTimerMode );
extern BOOL	StopSWTimer( u8 bTimerIndex );
extern void	ChangeSWTimer( u8 ucTimerIndex, int nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, BOOL bTimerStart );
extern void	ClearTimer( u8 ucTimerIndex );
extern u32	Timer_GetTimerCount(u8 ucTimerIndex);
extern bool	ContinueSWTimer(u8 ucTimerIndex);
extern u32	Get_TmrDelta( u32 ulNew, u32 ulOld );
extern u32	Get_Tmr( void );
extern void Timer_SetTimerCount(u8 ucTimerIndex, u32 nTimerCount_ms);

#endif	// __SW_TIMER_H__