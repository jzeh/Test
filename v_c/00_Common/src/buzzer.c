/*************************************************************
 * NOTE : buzzer.c
 *      buzzer control
 * Author : Lee junho
 * Since : 2019.07.30
**************************************************************/
#include "tim.h"
#include "string.h"

#include "common.h"
#include "buzzer.h"
#include "sw_timer.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define BUZZER_TIMER								htim4

#define BUZZER_BASE_FR								12000000								// 10Mhz
#define BUZZER_FREQUENCY							2700
#define BUZZER_PERIOD(x)							((u32)BUZZER_BASE_FR / (u32)x)
#define BUZZER_DUTY									50



/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
int32_t	Buzzer_Init( void );
void	Buzzer_Control( eBuzzertype ucBuzzer,  uint32_t unOnTime_ms, uint32_t unOffTime_ms, uint32_t unloopCnt );

static void Buzzer_On(void);
static void Buzzer_Off(void);
static void Buzzer_TimerCallBack(void);
static void BzFrDuty_Set( uint32_t freq,uint32_t duty );

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
stBUZZER_INFO		stBuzzerInfo;

TIM_HandleTypeDef	TimHandle;
TIM_OC_InitTypeDef	sConfig;

extern TIM_HandleTypeDef htim4;


#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
const uint32_t sound[7][4] = {{ 512, 384, 320, 256 },	// DoMiSolDo
							  { 384, 320, 256, 0   },	// DoMiSol
							  { 427, 341, 256, 0   },	// DoPaLa
							  { 512, 384, 320, 0   },	// eBUZZER_DOMISOL
							  { 384, 288, 480, 0   },   // eBUZZER_SIRESOL
                              { 480, 341, 320, 0   }};	// eBUZZER_SOLPAMI
#elif 0
const uint32_t sound[5][8] = {{ 512, 384, 320, 256 },	// DoMiSolDo
							  { 384, 320, 256, 0   },	// DoMiSol
							  { 427, 341, 256, 0   },	// DoPaLa
							  { 384, 288, 480, 0   },	// SiReSol
							  { 262, 294, 330, 349, 392, 440, 494, 523}};	// DoReMiPaSolRaSiDo
#else
const uint32_t sound[6][27] = {{ 523, 392, 330, 262 },	// DoMiSolDo
							  { 392, 330, 262, 0   },	// DoMiSol
							  { 440, 349, 262, 0   },	// DoPaLa
							  { 392, 294, 494, 0   },	// SiReSol
							  { 262, 294, 330, 349, 392, 440, 494, 523},
							  {698, 932, 698, 587, 494, 349, 466, 349, 294, 466, 659, 831, 622, 523, 440, 330, 415, 311, 262, 208, 392, 587, 392, 330, 262, 196, 262}};	// DoReMiPaSolRaSiDo
#endif
							  
/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
int32_t Buzzer_Init( void )
{
	BUZZER_TIMER.Instance				= TIM4;
	BUZZER_TIMER.Init.Prescaler			= 20;
	BUZZER_TIMER.Init.Period			= BUZZER_PERIOD(BUZZER_FREQUENCY);
	BUZZER_TIMER.Init.ClockDivision		= 0;
	BUZZER_TIMER.Init.CounterMode		= TIM_COUNTERMODE_UP;
	BUZZER_TIMER.Init.RepetitionCounter	= 0;
	if( HAL_TIM_PWM_Init( &BUZZER_TIMER ) != HAL_OK )
	{
		return -1;
	}

	sConfig.OCMode			= TIM_OCMODE_PWM1;
	sConfig.OCPolarity		= TIM_OCPOLARITY_HIGH;
	sConfig.OCFastMode		= TIM_OCFAST_DISABLE;
	sConfig.OCNPolarity		= TIM_OCNPOLARITY_HIGH;
	sConfig.OCNIdleState	= TIM_OCNIDLESTATE_RESET;
	sConfig.OCIdleState		= TIM_OCIDLESTATE_RESET;

	/* Set the pulse value for channel 1 */
	sConfig.Pulse = BUZZER_PERIOD(BUZZER_FREQUENCY) / 2;
	if( HAL_TIM_PWM_ConfigChannel( &BUZZER_TIMER, &sConfig, TIM_CHANNEL_2 ) != HAL_OK )
	{
		return -2;
	}

	memset( &stBuzzerInfo, 0x0, sizeof( stBUZZER_INFO ) );
	stBuzzerInfo.ucTimerIndex = SetSWTimer( 0, eSWTimer_NONE, Buzzer_TimerCallBack, FALSE );

	return 0;
}

//------------------------------------------------------------------------------
//      freq : MAX 5MHz ,MIN 152Hz
//      duty : Max 100, MIN 0
//------------------------------------------------------------------------------
static void BzFrDuty_Set( uint32_t freq,uint32_t duty )
{
	TIM4->ARR	= BUZZER_PERIOD(freq);
	TIM4->CCR2	= ((uint32_t)TIM4->ARR*duty)/100;
}

static void Buzzer_On( void )
{
	HAL_TIM_PWM_Stop( &BUZZER_TIMER, TIM_CHANNEL_2 );
	if( HAL_TIM_PWM_Start( &BUZZER_TIMER, TIM_CHANNEL_2 ) != HAL_OK )
	{
		GLogE( "Error... Start Buzzer!!!\r\n" );
		Error_Handler();
	}
}

static void Buzzer_Off( void )
{
	if( HAL_TIM_PWM_Stop( &BUZZER_TIMER, TIM_CHANNEL_2 ) != HAL_OK )
	{
		GLogE( "Error... Stop Buzzer!!!\r\n" );
		Error_Handler();
	}
}

void Buzzer_Control( eBuzzertype ucBuzzer,  uint32_t unOnTime_ms, uint32_t unOffTime_ms, uint32_t unloopCnt )
{
	// jkc_0240424_BEGIN -- 1
	eSWTimerMode SWTimeMode;

	stBuzzerInfo.eBuzzerIndex		= ucBuzzer;
	stBuzzerInfo.unBuzzerOnTime		= unOnTime_ms;
	stBuzzerInfo.unBuzzerOffTime	= unOffTime_ms;
	stBuzzerInfo.unBuzzerLoopCnt	= unloopCnt;

	if ( unloopCnt == TIMER_LOOP_INFINITE )		SWTimeMode = eSWTimer_INFINITE;
	else										SWTimeMode = eSWTimer_ONESHOT;

	if( stBuzzerInfo.eBuzzerIndex != TRUE )
	{
		BzFrDuty_Set(sound[stBuzzerInfo.eBuzzerIndex-2][stBuzzerInfo.unBuzzerLoopCnt-1 ], BUZZER_DUTY);// sound freq array, 50%duty
	}

	Buzzer_On();

	ChangeSWTimer(stBuzzerInfo.ucTimerIndex, stBuzzerInfo.unBuzzerOnTime, SWTimeMode, Buzzer_TimerCallBack, TRUE);
	 // jkc_0240424_END -- 1
}

static void Buzzer_TimerCallBack( void )
{
	if ( stBuzzerInfo.unBuzzerLoopCnt == 0 )
	{
		BzFrDuty_Set( BUZZER_FREQUENCY, BUZZER_DUTY );
		Buzzer_Off();
		StopSWTimer(stBuzzerInfo.ucTimerIndex);
		return;
	}

	if ( stBuzzerInfo.eBuzzerIndex == TRUE )    //on 이면
	{
		Buzzer_Off();

		stBuzzerInfo.eBuzzerIndex = (eBuzzertype)FALSE;
		if( stBuzzerInfo.unBuzzerOffTime )
		{
			ChangeSWTimer(stBuzzerInfo.ucTimerIndex, stBuzzerInfo.unBuzzerOffTime, eSWTimer_ONESHOT, Buzzer_TimerCallBack, TRUE);
		}
	}
	else if ( stBuzzerInfo.eBuzzerIndex == FALSE )    //on 이면
	{
		stBuzzerInfo.unBuzzerLoopCnt--;
		if( stBuzzerInfo.unBuzzerLoopCnt == 0 )
		{
			Buzzer_Off();
			stBuzzerInfo.eBuzzerIndex = (eBuzzertype)FALSE;
		}
		else
		{
			Buzzer_On();
		}

		stBuzzerInfo.eBuzzerIndex = (eBuzzertype)TRUE;
		if( stBuzzerInfo.unBuzzerOnTime )
		{
			ChangeSWTimer(stBuzzerInfo.ucTimerIndex, stBuzzerInfo.unBuzzerOnTime, eSWTimer_ONESHOT, Buzzer_TimerCallBack, TRUE);
		}
	}
	else
	{
		stBuzzerInfo.unBuzzerLoopCnt--;
		if( stBuzzerInfo.unBuzzerLoopCnt == 0 )
		{
			Buzzer_Off();
			stBuzzerInfo.eBuzzerIndex = (eBuzzertype)FALSE;
		}
		else
		{
			BzFrDuty_Set(sound[stBuzzerInfo.eBuzzerIndex-2][stBuzzerInfo.unBuzzerLoopCnt-1], BUZZER_DUTY);// sound freq array, 50%duty
			Buzzer_On();
		}

		if( stBuzzerInfo.unBuzzerOnTime )
		{
			ChangeSWTimer(stBuzzerInfo.ucTimerIndex, stBuzzerInfo.unBuzzerOnTime, eSWTimer_ONESHOT, Buzzer_TimerCallBack, TRUE);
		}
	}
}