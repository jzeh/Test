/*************************************************************
 * NOTE : sw_timer.c
 *      software timer
 * Author : Lee junho
 * Since : 2019.07.30
**************************************************************/
#include "string.h"

#include "common.h"
#include "sw_timer.h"

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
stSWTimerInfo g_SWTimer[MAX_SW_TIMER];

u32	g_ul16timer_ms			= 0;
u32	intDlccomCount			= 0;
u32	intTxdRxdCount			= 0;
u32	healthcheckcnt			= 0;
/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void	Internal_timer_Proc( void );
void	SwTmr_initial( void );
int		SetSWTimer( int nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, BOOL bTimerStart );
BOOL	StartSWTimer( u8 bTimerIndex, eSWTimerMode eTimerMode );
BOOL	StopSWTimer( u8 bTimerIndex );
void	ChangeSWTimer( u8 ucTimerIndex, int nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, BOOL bTimerStart );
void	ClearTimer( u8 ucTimerIndex );
u32		Get_TmrDelta( u32 ulNew, u32 ulOld );
u32		Get_Tmr( void );

void	check1msCallback( void );

u32 Get_Tmr(void)
{
	return g_ul16timer_ms;
}

u32 Get_TmrDelta( u32 ulNew, u32 ulOld )
{
	if( ulNew > ulOld )				return ulNew - ulOld;
	else if( ulNew < ulOld )		return 0xFFFFFFFF + ulNew - ulOld;
	else							return 0;
}

void Internal_timer_Proc(void)
{
	u8 i;

	g_ul16timer_ms++;											// every 1msec : TICK COUNT
	if(intDlccomCount!=0)
		intDlccomCount--;
	if(intTxdRxdCount!=0)
		intTxdRxdCount--;
	if(healthcheckcnt!=0)
		healthcheckcnt--;

	for( i = 0; i < MAX_SW_TIMER; i++ )
	{
		if( g_SWTimer[i].bActive == TRUE )
		{
			if( g_SWTimer[i].eTimerMode == eSWTimer_NONE )
				continue;
			else												// eSWTimer_INFINITE or eSWTimer_ONESHOT
			{
				if( g_SWTimer[i].ui32SWtimerCnt != 0 )		g_SWTimer[i].ui32SWtimerCnt--;
				else										continue;
			}

			if( g_SWTimer[i].ui32SWtimerCnt == 0 && g_SWTimer[i].fp != NULL )
			{
				if( g_SWTimer[i].eTimerMode == eSWTimer_ONESHOT )			g_SWTimer[i].eTimerMode = eSWTimer_NONE;
				else if( g_SWTimer[i].eTimerMode == eSWTimer_INFINITE )		g_SWTimer[i].ui32SWtimerCnt = g_SWTimer[i].ui32SaveTimerValue;

				g_SWTimer[i].fp();
			}
		}
	}
}

/******************************************************************************************************/
/* timer base bit initial :                                                                           */
/******************************************************************************************************/
void SwTmr_initial(void)
{
	memset( g_SWTimer, 0x00, sizeof(stSWTimerInfo) * MAX_SW_TIMER );
}

// return success : SWTimer index, fail : -1
int SetSWTimer( int nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, BOOL bTimerStart )
{
	int i;

	for( i = 0; i < MAX_SW_TIMER; i++ )
	{
		if ( g_SWTimer[i].bActive == FALSE )		break;
	}

	if ( i == MAX_SW_TIMER )
		 return -1;

	g_SWTimer[i].bActive			= TRUE;
	g_SWTimer[i].ui32SWtimerCnt		= 0;
	g_SWTimer[i].ui32SaveTimerValue	= nTimerInterval_ms;
	g_SWTimer[i].eTimerMode			= eTimerMode;
	g_SWTimer[i].fp					= fnCallback;
	if( bTimerStart )
	{
		g_SWTimer[i].ui32SWtimerCnt = g_SWTimer[i].ui32SaveTimerValue;
		StartSWTimer( i, g_SWTimer[i].eTimerMode );
	}

	return i;
}

BOOL StartSWTimer( u8 ucTimerIndex, eSWTimerMode eTimerMode )
{
	if( g_SWTimer[ucTimerIndex].bActive )
	{
		g_SWTimer[ucTimerIndex].ui32SWtimerCnt	= g_SWTimer[ucTimerIndex].ui32SaveTimerValue;
		g_SWTimer[ucTimerIndex].eTimerMode		= eTimerMode;

		return TRUE;
	}

	return FALSE;
}

BOOL StopSWTimer( u8 ucTimerIndex )
{
	if( g_SWTimer[ucTimerIndex].bActive )
	{
		g_SWTimer[ucTimerIndex].eTimerMode = eSWTimer_NONE;
		return TRUE;
	}

	return FALSE;
}

void ClearTimer( u8 ucTimerIndex )
{
	if( g_SWTimer[ucTimerIndex].bActive == TRUE )
	{
		memset( &g_SWTimer[ucTimerIndex], 0x0, sizeof(stSWTimerInfo) );
	}
}

void ChangeSWTimer( u8 ucTimerIndex, int nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, BOOL bTimerStart )
{
	if( g_SWTimer[ucTimerIndex].bActive )
	{
		g_SWTimer[ucTimerIndex].bActive				= TRUE;
		g_SWTimer[ucTimerIndex].ui32SWtimerCnt		= 0;
		g_SWTimer[ucTimerIndex].ui32SaveTimerValue	= nTimerInterval_ms;
		g_SWTimer[ucTimerIndex].eTimerMode			= eTimerMode;
		g_SWTimer[ucTimerIndex].fp					= fnCallback;

		if ( bTimerStart )
		{
			g_SWTimer[ucTimerIndex].ui32SWtimerCnt = g_SWTimer[ucTimerIndex].ui32SaveTimerValue;
			StartSWTimer( ucTimerIndex, g_SWTimer[ucTimerIndex].eTimerMode );
		}
	}
}

u32 Timer_GetTimerCount(u8 ucTimerIndex)
{
	if ( g_SWTimer[ucTimerIndex].bActive )
	{
		return g_SWTimer[ucTimerIndex].ui32SWtimerCnt;
	}
	else
		return 0;
}

void Timer_SetTimerCount(u8 ucTimerIndex, u32 nTimerCount_ms)
{
	g_SWTimer[ucTimerIndex].ui32SWtimerCnt = nTimerCount_ms;
}

bool ContinueSWTimer(u8 ucTimerIndex)
{
	if ( g_SWTimer[ucTimerIndex].bActive )
	{
		g_SWTimer[ucTimerIndex].eTimerMode = eSWTimer_INFINITE;
		return true;
	}
	return false;
}