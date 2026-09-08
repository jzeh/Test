/*
  ******************************************************************************
  * @file    HalTimerDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @
  *
  *
  *
  ******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#if defined(STM32F427X)
#include "STM32F4xx.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#endif
#include "HalHandler.h"
#include "HalTimerDriver.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define Trace(...)  //GITDebug(DEBUG_MODULES_SW_TMR,__VA_ARGS__)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
stSWTimerInfo g_SWTimer[HAL_MAX_SW_TIMER];

#if USE_TIMER_CAPTURE_MICROSEC_DELAY  
volatile unsigned long g_ul16timer_us = 0;
#else
volatile unsigned long g_ul16timer_ms = 0;
#endif

/* Private function prototypes -----------------------------------------------*/
void HW_Delay_us(unsigned int unMicrosec);
void HW_Delay_ms(unsigned int unMiliSec);
unsigned long HalAPI_GetMicroTick(void);
void HalAPI_IncMicroTick(void);

/* Private functions ---------------------------------------------------------*/


unsigned long Get_Tmr(void)
{
#if USE_TIMER_CAPTURE_MICROSEC_DELAY  
    return g_ul16timer_us;
#else
  return g_ul16timer_ms;
#endif
}

unsigned long Get_TmrDelta(unsigned long ulNew, unsigned long ulOld)
{
  if (ulNew>ulOld){
     return (ulNew-ulOld);
  }else if (ulNew<ulOld) {
    return(0xFFFFFFFF+ulNew-ulOld);
  }else{
     return 0;
  }
}

void HalDrvTimer_Internal_Timer_Proc(void)
{
	uint8_t i;

#if USE_TIMER_CAPTURE_MICROSEC_DELAY  
	g_ul16timer_us++;
#else
	g_ul16timer_ms++;	//every 1msec : TICK COUNT
//    g_ul16timer_ms = HalAPI_GetMicroTick() / 1000;
#endif

	for ( i = 0; i < HAL_MAX_SW_TIMER; i++ ) {
		if ( g_SWTimer[i].bActive == TRUE ) {		// TRUE일때만 동작하게 추가 150513 LWH
			if ( g_SWTimer[i].eTimerMode == eSWTimer_NONE ) {
				continue;
			}
			else { // eSWTimer_INFINITE or eSWTimer_ONESHOT
				if ( g_SWTimer[i].ui32SWtimerCnt != 0 ) {
					g_SWTimer[i].ui32SWtimerCnt--;
				}
				else {
					continue;			// 0이면 RETURN 추가 LWH
				}
			}

			if ( g_SWTimer[i].ui32SWtimerCnt == 0 && g_SWTimer[i].fp != NULL ) {
				if ( g_SWTimer[i].eTimerMode == eSWTimer_ONESHOT ) {
					g_SWTimer[i].eTimerMode = eSWTimer_NONE;
				}
				else if ( g_SWTimer[i].eTimerMode == eSWTimer_INFINITE ) {
					g_SWTimer[i].ui32SWtimerCnt = g_SWTimer[i].ui32SaveTimerValue;
				}

				g_SWTimer[i].fp();
			}
		}
	}
}

/******************************************************************************************************/
/* timer base bit initial :		                                                              */
/******************************************************************************************************/
void HalTimerInitVariableSWTimer(void)
{
	g_ul16timer_ms = 0;
	memset(g_SWTimer, 0x00, sizeof(stSWTimerInfo)*HAL_MAX_SW_TIMER);
}

int HalTimerSetSWTimer(uint32_t nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, boolean_t bTimerStart)
{
	// return success : SWTimer index, fail : -1
	int i;

	for ( i=0; i<HAL_MAX_SW_TIMER; i++ )
	{
		if ( g_SWTimer[i].bActive == FALSE )
			break;
	}

	if ( i == HAL_MAX_SW_TIMER )
	{
		Trace("\r\n@@@@@@@@@@@ [error] Timer is MAX @@@@@@@@@@@@\r\n\r\n");
	 	return -1;
	}
	else
	{
		Trace(" @HalTimerStartSWTimer(), id: %d, %d\n", i, nTimerInterval_ms);

		g_SWTimer[i].bActive = TRUE;
		g_SWTimer[i].ui32SWtimerCnt = 0;
		g_SWTimer[i].ui32SaveTimerValue = nTimerInterval_ms;
		g_SWTimer[i].eTimerMode = eTimerMode;
		g_SWTimer[i].fp = fnCallback;
		if ( bTimerStart ) {
			g_SWTimer[i].ui32SWtimerCnt = g_SWTimer[i].ui32SaveTimerValue;
			HalTimerStartSWTimer(i, g_SWTimer[i].eTimerMode);
		}
	}

	return i;
}

boolean_t HalTimerStartSWTimer(BYTE ucTimerIndex, eSWTimerMode eTimerMode)
{
	if ( g_SWTimer[ucTimerIndex].bActive )
	{
		g_SWTimer[ucTimerIndex].ui32SWtimerCnt = g_SWTimer[ucTimerIndex].ui32SaveTimerValue;
		g_SWTimer[ucTimerIndex].eTimerMode = eTimerMode;

		return TRUE;
	}

	return FALSE;
}

boolean_t HalTimerStopSWTimer(BYTE ucTimerIndex)
{
	if(ucTimerIndex >= 0) 
	{
	if ( g_SWTimer[ucTimerIndex].bActive )
	{
		g_SWTimer[ucTimerIndex].eTimerMode = eSWTimer_NONE;
		return TRUE;
	}
	}
	return FALSE;
}

void HalTimerClearSWTimer(int ucTimerIndex)
{
	if(ucTimerIndex >= 0) {
		memset(&g_SWTimer[ucTimerIndex], 0x0, sizeof(stSWTimerInfo));
	}
}

void HalTimerClearAllSwTimer()
{
    for ( int i=0; i<HAL_MAX_SW_TIMER; i++ )
	{
        memset(&g_SWTimer[i], 0x0, sizeof(stSWTimerInfo));
    }

}

void HalTimerChangeSWTimer(BYTE ucTimerIndex, uint32_t nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, boolean_t bTimerStart)
{
//	Trace(" @HalTimerChangeSWTimer(), id: %d, ms: %d\n", ucTimerIndex, nTimerInterval_ms);
	if ( g_SWTimer[ucTimerIndex].bActive )
	{
		g_SWTimer[ucTimerIndex].bActive = TRUE;
		g_SWTimer[ucTimerIndex].ui32SWtimerCnt = 0;
		g_SWTimer[ucTimerIndex].ui32SaveTimerValue = nTimerInterval_ms;
		g_SWTimer[ucTimerIndex].eTimerMode = eTimerMode;
		g_SWTimer[ucTimerIndex].fp = fnCallback;

		if ( bTimerStart )
		{
			g_SWTimer[ucTimerIndex].ui32SWtimerCnt = g_SWTimer[ucTimerIndex].ui32SaveTimerValue;
			HalTimerStartSWTimer(ucTimerIndex, g_SWTimer[ucTimerIndex].eTimerMode);
		}
	}
}

//**************************************************
// 특정 타이머의 타이머 카운트 값을 읽어 온다.
//**************************************************
u32 HalTimerGetSwTimerCount(BYTE ucTimerIndex)
{
	if ( g_SWTimer[ucTimerIndex].bActive )
	{
		return g_SWTimer[ucTimerIndex].ui32SWtimerCnt;
	}
	else
		return 0;
}

bool HalTimerContinueSWTimer(BYTE ucTimerIndex)
{
	if ( g_SWTimer[ucTimerIndex].bActive )
	{
		g_SWTimer[ucTimerIndex].eTimerMode = eSWTimer_INFINITE;
		return true;
	}
	return false;
}

u32 HalTimerGetTimerActive(BYTE ucTimerIndex)
{
	if ( g_SWTimer[ucTimerIndex].bActive )
	{
		return true;
	}
	return false;
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
#if !defined(FEATRUE_USE_FREERTOS)
void SysTick_Handler(void)
{
#if !defined(FEATURE_BOOTLOADER)
    HalDrvTimer_Internal_Timer_Proc();
#endif
}
#endif

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------
// HW timer
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* delay macros */
#define STEP_DELAY_MS                    50

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* delay variable */
static __IO uint32_t g_unFac_us;
static __IO uint32_t g_unFac_ms;


void HW_Delay_init()
{
#if defined(STM32F427X)

    //SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

#elif defined(AT32F435VMT7)
    /* configure systick */
    systick_clock_source_config(SYSTICK_CLOCK_SOURCE_AHBCLK_NODIV);
    g_unFac_us = system_core_clock / (1000000U);
    g_unFac_ms = g_unFac_us * (1000U);
#endif
}

void HW_Delay_us(unsigned int unMicrosec)
{
#if defined(STM32F427X)
#if 0
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);


    /* TIM IT enable */
    TIM_ITConfig(TIM3, TIM_IT_Update , ENABLE);
    
    // Set SysTick Reload(1us) register and Enable
    // usec * (RCC_Clocks.HCLK_Frequency / 1000000) < 0xFFFFFFUL -- because of 24bit timer
    // RCC_Clocks.HCLK_Frequency = 72000000
    // Systick Reload Value Register = 72
    // 72 / 72000000 = 1us
    SysTick_Config(unMicrosec * (RCC_Clocks.HCLK_Frequency / 1000000));
    // Until Tick count is 0
    while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));

    TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET

    TIM_ITConfig(TIM3, TIM_IT_Update , DISABLE);
#endif

#elif defined(AT32F435VMT7)
    uint32_t temp = 0;
    SysTick->LOAD = (uint32_t)(unMicrosec * g_unFac_us);
    SysTick->VAL = 0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk ;
    do
    {
        temp = SysTick->CTRL;
    }while((temp & 0x01) && !(temp & (1 << 16)));

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL = 0x00;
#endif
}

void HW_Delay_ms(unsigned int unMiliSec)
{
#if defined(STM32F427X)
#elif defined(AT32F435VMT7)
    uint32_t temp = 0;
    while(unMiliSec)
    {
        if(unMiliSec > STEP_DELAY_MS)
        {
            SysTick->LOAD = (uint32_t)(STEP_DELAY_MS * g_unFac_ms);
            unMiliSec -= STEP_DELAY_MS;
        }
        else
        {
            SysTick->LOAD = (uint32_t)(unMiliSec * g_unFac_ms);
            unMiliSec = 0;
        }
        SysTick->VAL = 0x00;
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        do
        {
            temp = SysTick->CTRL;
        }while((temp & 0x01) && !(temp & (1 << 16)));

        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
        SysTick->VAL = 0x00;
    }
#endif
}

void HW_Delay_sec(uint16_t unSec)
{
     uint16_t index;
    for(index = 0; index < unSec; index++)
    {
        HW_Delay_ms(500);
        HW_Delay_ms(500);
    }
}

void udelay(unsigned int unCnt)
{
#if defined(STM32F427X)
    /*
	TIM3->SR = (uint16_t)~TIM_FLAG_CC1;//TIM_ClearFlag(TIM3, TIM_FLAG_CC1);
	TIM_GetCapture1(TIM3);

	TIM3->CCR1 = TIM3->CNT + unCnt * 10 - 6;        // clock이 0.1usec이기 때문에 *10를 해 준다.

	while ((TIM3->SR & TIM_FLAG_CC1) == RESET);
    */
#elif defined(AT32F435VMT7)
#endif
}

void HW_Timer3_Init(void)
{
#if defined(STM32F427X)
#if 0
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = 9-1;//90MHz/9=10Mhz=0.1usec
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* Prescaler configuration */
    TIM_PrescalerConfig(TIM3, TIM_TimeBaseStructure.TIM_Prescaler, TIM_PSCReloadMode_Immediate);
    /* Output Compare Timing Mode configuration: Channel1 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
    TIM_OCInitStructure.TIM_Pulse = 10000;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);

    
	TIM_Cmd(TIM3, ENABLE);
#endif
    
#elif defined(AT32F435VMT7)
#endif
	return;
}

/*
void TIM3_IRQHandler(void)
{
#if defined(STM32F427X)
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET )
    {
        // Also cleared the wrong interrupt flag in the ISR
        TIM_ClearFlag(TIM3, TIM_FLAG_Update);
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update); // Clear the interrupt flag
    }
#elif defined(AT32F435VMT7)
#endif
}
*/


int HalDrvTimerOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
#if defined(STM32F427X)
   // HW_Timer3_Init();
#elif defined(AT32F435VMT7)
    HW_Delay_init();
#endif
    
    HalTimerInitVariableSWTimer();

    return HAL_RETURN_SUCCESS;
}

//--------------------------------------------------------------//
int HalDrvTimerRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvTimerWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvTimerIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    eHalTimer_IOCtlMode eTimerIOMode = (eHalTimer_IOCtlMode)nLparam;

    switch (eTimerIOMode)
    {
        case eTimer_IO_TimebaseINIT:
        {
           stHalTIM_TimeBaseInitTypeDef *pstTimeBase = (stHalTIM_TimeBaseInitTypeDef*)pBuffer;
#if defined(STM32F427X)
            TIM_TimeBaseInit((TIM_TypeDef*)nRparam, (TIM_TimeBaseInitTypeDef*)pstTimeBase);
#elif defined(AT32F435VMT7)
            tmr_base_init((tmr_type*)nRparam, pstTimeBase->TIM_Prescaler, pstTimeBase->TIM_ClockDivision);
#endif
        }
            break;
        case eTimer_IO_Timebase_prescaler_config:
        {
 #if defined(STM32F427X)
            int nPrescaler = nLength;
            int nOption = nOverlap;
            /* Prescaler configuration */           
TIM_PrescalerConfig((TIM_TypeDef*)nRparam, nPrescaler, nOption);
#elif defined(AT32F435VMT7)
//            uint32_t tmr_cnt_value = ;
//            tmr_period_value_set((tmr_type*)nRparam, uint32_t tmr_cnt_value);
#endif
        }
            break;
        case eTimer_IO_Timer_Enable:
#if defined(STM32F427X)
            TIM_Cmd((TIM_TypeDef*)nRparam, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            tmr_counter_enable((tmr_type*)nRparam, (confirm_state)nOverlap);
#endif
            break;
        case eTimer_IO_TimingMode_Config_OC1:
#if defined(STM32F427X)
            TIM_OC1Init((TIM_TypeDef*)nRparam, (TIM_OCInitTypeDef*)pBuffer);
            TIM_OC1PreloadConfig((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            tmr_channel_value_set((tmr_type*)nRparam, TMR_SELECT_CHANNEL_1, nOverlap);
#endif
            break;
        case eTimer_IO_TimingMode_Config_OC2:
#if defined(STM32F427X)
            TIM_OC2Init((TIM_TypeDef*)nRparam, (TIM_OCInitTypeDef*)pBuffer);
            TIM_OC2PreloadConfig((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            tmr_channel_value_set((tmr_type*)nRparam, TMR_SELECT_CHANNEL_2, nOverlap);
#endif
            break;
        case eTimer_IO_TimingMode_Config_OC3:
#if defined(STM32F427X)
            TIM_OC3Init((TIM_TypeDef*)nRparam, (TIM_OCInitTypeDef*)pBuffer);
            TIM_OC3PreloadConfig((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            tmr_channel_value_set((tmr_type*)nRparam, TMR_SELECT_CHANNEL_3, nOverlap);
#endif
            break;
        case eTimer_IO_TimingMode_Config_OC4:
#if defined(STM32F427X)
            TIM_OC4Init((TIM_TypeDef*)nRparam, (TIM_OCInitTypeDef*)pBuffer);
            TIM_OC4PreloadConfig((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            tmr_channel_value_set((tmr_type*)nRparam, TMR_SELECT_CHANNEL_4, nOverlap);
#endif
            break;
        case eTimer_IO_GetCapture_OC1:
#if defined(STM32F427X)
            return TIM_GetCapture1((TIM_TypeDef*)nRparam);
#elif defined(AT32F435VMT7)
            return tmr_channel_value_get((tmr_type*)nRparam, TMR_SELECT_CHANNEL_1);
#endif
            break;
        case eTimer_IO_GetCapture_OC2:
#if defined(STM32F427X)
            return TIM_GetCapture2((TIM_TypeDef*)nRparam);
#elif defined(AT32F435VMT7)
            return tmr_channel_value_get((tmr_type*)nRparam, TMR_SELECT_CHANNEL_2);
#endif
            break;
        case eTimer_IO_GetCapture_OC3:
#if defined(STM32F427X)
            return TIM_GetCapture3((TIM_TypeDef*)nRparam);
#elif defined(AT32F435VMT7)
            return tmr_channel_value_get((tmr_type*)nRparam, TMR_SELECT_CHANNEL_3);
#endif
            break;
        case eTimer_IO_GetCapture_OC4:
#if defined(STM32F427X)
            return TIM_GetCapture4((TIM_TypeDef*)nRparam);
#elif defined(AT32F435VMT7)
            return tmr_channel_value_get((tmr_type*)nRparam, TMR_SELECT_CHANNEL_4);
#endif
            break;
        case eTimer_IO_SetCompare_OC1:
#if defined(STM32F427X)
            TIM_SetCompare1((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            tmr_channel_value_set((tmr_type*)nRparam, TMR_SELECT_CHANNEL_1, nOverlap);
#endif
            break;
        case eTimer_IO_SetCompare_OC2:
#if defined(STM32F427X)
            TIM_SetCompare2((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            tmr_channel_value_set((tmr_type*)nRparam, TMR_SELECT_CHANNEL_2, nOverlap);
#endif
            break;
        case eTimer_IO_SetCompare_OC3:
#if defined(STM32F427X)
            TIM_SetCompare3((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            tmr_channel_value_set((tmr_type*)nRparam, TMR_SELECT_CHANNEL_3, nOverlap);
#endif
            break;
        case eTimer_IO_SetCompare_OC4:
#if defined(STM32F427X)
            TIM_SetCompare4((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            tmr_channel_value_set((tmr_type*)nRparam, TMR_SELECT_CHANNEL_4, nOverlap);
#endif
            break;
        case eTimer_IO_GetITFlags:
#if defined(STM32F427X)
            return TIM_GetITStatus((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            return tmr_flag_get((tmr_type*)nRparam, nOverlap);
#endif
            break;
        case eTimer_IO_ClearITFlags:
#if defined(STM32F427X)
            TIM_ClearFlag((TIM_TypeDef*)nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            tmr_flag_clear((tmr_type*)nRparam, nOverlap);
#endif
            break;
        case eTimer_IO_IT_CONFIG:
#if defined(STM32F427X)
            TIM_ITConfig((TIM_TypeDef*)nRparam, nLength, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            tmr_interrupt_enable((tmr_type*)nRparam, nLength, (confirm_state)nOverlap);
#endif
            break;
        default:
            break;
    }
    return HAL_RETURN_SUCCESS;
}

int HalDrvTimerClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    HalTimerInitVariableSWTimer();
    return HAL_RETURN_SUCCESS;
}

