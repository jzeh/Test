#ifndef __HAL_TIMER_DRIVER_H__
#define __HAL_TIMER_DRIVER_H__

#include "common.h"
////////////////////////////////////////////////////////////////////////////////
#define HAL_Delay   HalAPI_Delay    
/* Exported define -----------------------------------------------------------*/
//#define FEATURE_USE_TMR1
//#define FEATURE_USE_TMR2
//#define FEATURE_USE_TMR3
//#define FEATURE_USE_TMR4    
#define FEATURE_USE_TMR7
#define FEATURE_USE_TMR10    

#if defined(FEATURE_USE_TMR1)
#define HAL_TMR1                   TIM1
#endif

#if defined(FEATURE_USE_TMR2)
#define HAL_TMR2                   TIM2
#endif

#if defined(FEATURE_USE_TMR3)
#define HAL_TMR3                   TIM3
#endif

#if defined(FEATURE_USE_TMR7)
#define HAL_TMR7                   TIM7
#endif
#if defined(FEATURE_USE_TMR10)
#define HAL_TMR10                  TIM10
#endif


/** @defgroup TIM_Counter_Mode **/
#define HAL_TIM_CounterMode_Up                 ((uint16_t)0x0000)
#define HAL_TIM_CounterMode_Down               ((uint16_t)0x0010)
#define HAL_TIM_CounterMode_CenterAligned1     ((uint16_t)0x0020)
#define HAL_TIM_CounterMode_CenterAligned2     ((uint16_t)0x0040)
#define HAL_TIM_CounterMode_CenterAligned3     ((uint16_t)0x0060)
/** @defgroup TIM_Output_Compare_Polarity **/
#define HAL_TIM_OCPolarity_High                ((uint16_t)0x0000)
#define HAL_TIM_OCPolarity_Low                 ((uint16_t)0x0002)
/** @defgroup TIM_Output_Compare_N_Polarity **/
#define HAL_TIM_OCNPolarity_High               ((uint16_t)0x0000)
#define HAL_TIM_OCNPolarity_Low                ((uint16_t)0x0008)
/** @defgroup TIM_Output_Compare_State **/
#define HAL_TIM_OutputState_Disable            ((uint16_t)0x0000)
#define HAL_TIM_OutputState_Enable             ((uint16_t)0x0001)
/** @defgroup TIM_Output_Compare_N_State **/
#define HAL_TIM_OutputNState_Disable           ((uint16_t)0x0000)
#define HAL_TIM_OutputNState_Enable            ((uint16_t)0x0004)


/** @defgroup TIM_Prescaler_Reload_Mode **/
#define HAL_HAL_TIM_PSCReloadMode_Update           ((uint16_t)0x0000)
#define HAL_HAL_TIM_PSCReloadMode_Immediate        ((uint16_t)0x0001)

/** @defgroup TIM_Output_Compare_and_PWM_modes **/
#define HAL_TIM_OCMode_Timing                  ((uint16_t)0x0000)
#define HAL_TIM_OCMode_Active                  ((uint16_t)0x0010)
#define HAL_TIM_OCMode_Inactive                ((uint16_t)0x0020)
#define HAL_TIM_OCMode_Toggle                  ((uint16_t)0x0030)
#define HAL_TIM_OCMode_PWM1                    ((uint16_t)0x0060)
#define HAL_TIM_OCMode_PWM2                    ((uint16_t)0x0070)
/** @defgroup TIM_Output_Compare_State **/
#define HAL_TIM_OutputState_Disable            ((uint16_t)0x0000)
#define HAL_TIM_OutputState_Enable             ((uint16_t)0x0001)
/** @defgroup TIM_Output_Compare_N_State**/
#define HAL_TIM_OutputNState_Disable           ((uint16_t)0x0000)
#define HAL_TIM_OutputNState_Enable            ((uint16_t)0x0004)
/** @defgroup TIM_Output_Compare_Preload_State **/
#define HAL_TIM_OCPreload_Enable               ((uint16_t)0x0008)
#define HAL_TIM_OCPreload_Disable              ((uint16_t)0x0000)

/** @defgroup TIM_interrupt_sources **/
#define HAL_TIM_IT_Update                      ((uint16_t)0x0001)
#define HAL_TIM_IT_CC1                         ((uint16_t)0x0002)
#define HAL_TIM_IT_CC2                         ((uint16_t)0x0004)
#define HAL_TIM_IT_CC3                         ((uint16_t)0x0008)
#define HAL_TIM_IT_CC4                         ((uint16_t)0x0010)
#define HAL_TIM_IT_COM                         ((uint16_t)0x0020)
#define HAL_TIM_IT_Trigger                     ((uint16_t)0x0040)
#define HAL_TIM_IT_Break                       ((uint16_t)0x0080)

/** @defgroup TIM_Flags **/
#define HAL_TIM_FLAG_Update                    ((uint16_t)0x0001)
#define HAL_TIM_FLAG_CC1                       ((uint16_t)0x0002)
#define HAL_TIM_FLAG_CC2                       ((uint16_t)0x0004)
#define HAL_TIM_FLAG_CC3                       ((uint16_t)0x0008)
#define HAL_TIM_FLAG_CC4                       ((uint16_t)0x0010)
#define HAL_TIM_FLAG_COM                       ((uint16_t)0x0020)
#define HAL_TIM_FLAG_Trigger                   ((uint16_t)0x0040)
#define HAL_TIM_FLAG_Break                     ((uint16_t)0x0080)
#define HAL_TIM_FLAG_CC1OF                     ((uint16_t)0x0200)
#define HAL_TIM_FLAG_CC2OF                     ((uint16_t)0x0400)
#define HAL_TIM_FLAG_CC3OF                     ((uint16_t)0x0800)
#define HAL_TIM_FLAG_CC4OF                     ((uint16_t)0x1000)


typedef enum
{
  eHAL_TICK_FREQ_10HZ         = 100U,
  eHAL_TICK_FREQ_100HZ        = 10U,
  eHAL_TICK_FREQ_1KHZ         = 1U,
  eHAL_TICK_FREQ_DEFAULT      = eHAL_TICK_FREQ_1KHZ
} eHAL_TickFreqType;

//SW timer ---------------------------------------------------------------------
#define HAL_MAX_SW_TIMER	    32 /* 16 */
#define SEC(x)	            (x*1000)	// second unit
#define MSEC(x)	            (x)			// mili second unit
#define TIMER_LOOP_INFINITE	0XFFFF

/* Exported types - Structure, Enumeration -----------------------------------*/
typedef void (*fnSWCallBack)(void);

typedef enum _eSWTimerMode
{
	eSWTimer_NONE,
	eSWTimer_ONESHOT,
	eSWTimer_INFINITE,
}eSWTimerMode;

typedef __packed struct _stSWTimerInfo
{
  	bool    		bActive;
	eSWTimerMode	eTimerMode;
	unsigned int    ui32SWtimerCnt;
	unsigned int    ui32SaveTimerValue;
	fnSWCallBack 	fp;
}stSWTimerInfo;


typedef enum __eeHalTimer_IOCtlMode{
    eTimer_IO_TimebaseINIT,
    eTimer_IO_Timebase_prescaler_config,
    eTimer_IO_TimingMode_Config_OC1,
    eTimer_IO_TimingMode_Config_OC2,
    eTimer_IO_TimingMode_Config_OC3,
    eTimer_IO_TimingMode_Config_OC4,
    eTimer_IO_GetCapture_OC1,
    eTimer_IO_GetCapture_OC2,
    eTimer_IO_GetCapture_OC3,
    eTimer_IO_GetCapture_OC4,
    eTimer_IO_SetCompare_OC1,
    eTimer_IO_SetCompare_OC2,
    eTimer_IO_SetCompare_OC3,
    eTimer_IO_SetCompare_OC4,
    eTimer_IO_Timer_Enable,
    eTimer_IO_GetITFlags,
    eTimer_IO_ClearITFlags,
    eTimer_IO_IT_CONFIG,
//    eTimer_IO_SetClear,


}eHalTimer_IOCtlMode;


typedef __packed struct _stHalTIM_TimeBaseInitTypeDef

{
  uint16_t TIM_Prescaler;         /*!< Specifies the prescaler value used to divide the TIM clock.
                                       This parameter can be a number between 0x0000 and 0xFFFF */

  uint16_t TIM_CounterMode;       /*!< Specifies the counter mode.
                                       This parameter can be a value of @ref TIM_Counter_Mode */

  uint32_t TIM_Period;            /*!< Specifies the period value to be loaded into the active
                                       Auto-Reload Register at the next update event.
                                       This parameter must be a number between 0x0000 and 0xFFFF.  */ 

  uint16_t TIM_ClockDivision;     /*!< Specifies the clock division.
                                      This parameter can be a value of @ref TIM_Clock_Division_CKD */

  uint8_t TIM_RepetitionCounter;  /*!< Specifies the repetition counter value. Each time the RCR downcounter
                                       reaches zero, an update event is generated and counting restarts
                                       from the RCR value (N).
                                       This means in PWM mode that (N+1) corresponds to:
                                          - the number of PWM periods in edge-aligned mode
                                          - the number of half PWM period in center-aligned mode
                                       This parameter must be a number between 0x00 and 0xFF. 
                                       @note This parameter is valid only for TIM1 and TIM8. */
} stHalTIM_TimeBaseInitTypeDef; 

/** 
  * @brief  TIM Output Compare Init structure definition  
  */

typedef __packed struct _stHalTIM_OCInitTypeDef
{
  uint16_t TIM_OCMode;        /*!< Specifies the TIM mode.
                                   This parameter can be a value of @ref TIM_Output_Compare_and_PWM_modes */

  uint16_t TIM_OutputState;   /*!< Specifies the TIM Output Compare state.
                                   This parameter can be a value of @ref TIM_Output_Compare_State */

  uint16_t TIM_OutputNState;  /*!< Specifies the TIM complementary Output Compare state.
                                   This parameter can be a value of @ref TIM_Output_Compare_N_State
                                   @note This parameter is valid only for TIM1 and TIM8. */

  uint32_t TIM_Pulse;         /*!< Specifies the pulse value to be loaded into the Capture Compare Register. 
                                   This parameter can be a number between 0x0000 and 0xFFFF */

  uint16_t TIM_OCPolarity;    /*!< Specifies the output polarity.
                                   This parameter can be a value of @ref TIM_Output_Compare_Polarity */

  uint16_t TIM_OCNPolarity;   /*!< Specifies the complementary output polarity.
                                   This parameter can be a value of @ref TIM_Output_Compare_N_Polarity
                                   @note This parameter is valid only for TIM1 and TIM8. */

  uint16_t TIM_OCIdleState;   /*!< Specifies the TIM Output Compare pin state during Idle state.
                                   This parameter can be a value of @ref TIM_Output_Compare_Idle_State
                                   @note This parameter is valid only for TIM1 and TIM8. */

  uint16_t TIM_OCNIdleState;  /*!< Specifies the TIM Output Compare pin state during Idle state.
                                   This parameter can be a value of @ref TIM_Output_Compare_N_Idle_State
                                   @note This parameter is valid only for TIM1 and TIM8. */
} stHalTIM_OCInitTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/
int HalDrvTimerOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvTimerRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvTimerWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvTimerIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvTimerClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
void HalDrvTimer_Internal_Timer_Proc(void);


////////////////////////////////////////////////////////////////////////////////
//SW timer ---------------------------------------------------------------------
void HalTimerInitVariableSWTimer(void);
int HalTimerSetSWTimer(uint32_t nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, boolean_t bTimerStart);
boolean_t HalTimerStartSWTimer(BYTE ucTimerIndex, eSWTimerMode eTimerMode);
boolean_t HalTimerStopSWTimer(BYTE ucTimerIndex);
void HalTimerClearSWTimer(int ucTimerIndex);
void HalTimerClearAllSwTimer();
void HalTimerChangeSWTimer(BYTE ucTimerIndex, uint32_t nTimerInterval_ms, eSWTimerMode eTimerMode, fnSWCallBack fnCallback, boolean_t bTimerStart);
u32 HalTimerGetSwTimerCount(BYTE ucTimerIndex);
bool HalTimerContinueSWTimer(BYTE ucTimerIndex);
u32 HalTimerGetTimerActive(BYTE ucTimerIndex);
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//HW timer ---------------------------------------------------------------------
unsigned long Get_Tmr(void);
unsigned long Get_TmrDelta(unsigned long ulNew, unsigned long ulOld);
void udelay(unsigned int unCnt);
void HalAPI_Delay(unsigned int unDelay); // unit : milisecond


/* Exported define -----------------------------------------------------------*/
/* Exported types - Structure, Enumeration -----------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/


void HW_Timer3_Init(void);
void HW_udelay(unsigned int cnt);
void HW_Timer5_Init(void);



////////////////////////////////////////////////////////////////////////////////

#endif //__HAL_TIMER_DRIVER_H__
