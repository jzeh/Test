/*
  ******************************************************************************
  * @file    HalRtcDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#if defined(STM32F427X)
#include "STM32F4xx_rtc.h"
#include "Misc.h"
#elif defined(AT32F435VMT7)
#include "AT32F435_437.h"
#include "AT32F435_437_ertc.h"
#endif

#include "HalHandler.h"
#include "HalRtcDriver.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#if defined(AT32F435VMT7)
#define HAL_RTC_Alarm_IRQn      ERTCAlarm_IRQn           // same IRQ Number
#define RTC_Alarm_IRQHandler    ERTCAlarm_IRQHandler    
#define RTC_WKUP_IRQHandler     ERTC_WKUP_IRQHandler    
#endif
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
eHalErrorStatus HalRTC_ReadBackupRegister(unsigned int unRtcBKPReg, unsigned int unBKValue);

extern uint8_t computeDayOfWeek(uint16_t y, uint8_t m, uint8_t d);


/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Configure the RTC peripheral by selecting the clock source.
  * @param  None
  * @retval None
  */
void HalDrvRtc_Config(void)
{
    stHalRTC_InitTypeDef  RTC_InitStructure;
//    stHalRTC_TimeTypeDef  RTC_TimeStructure;
//    stHalRTC_DateTypeDef  RTC_DateStructure;
    stHalRTCTypeDef stRTC_DateTime;
    
    /* Enable the PWR clock */
    HalDrvRccIOCtrl(eRCC_IO_Power_Clock, 0, NULL, 0, HAL_ENABLE);
    /* Allow access to RTC */
    HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);

    HalDrvPowerIOCtrl(ePWR_IO_BK_PwDomain_Reset, 0, NULL, 0, HAL_ENABLE);

    /* Enable the LSE OSC */
    HalDrvRccIOCtrl(eRCC_IO_SET_LSE_CONFIG, HAL_RCC_LSE_ON, NULL, 0, HAL_ENABLE);

    
    /* Wait till LSE is ready */
    while( HalDrvRccIOCtrl(eRCC_IO_GET_FLAG_STATUS, HAL_RCC_FLAG_LSERDY, NULL, 0, 0) == HAL_RESET )
    {
    }

    /* Select the RTC Clock Source */
    HalDrvRccIOCtrl(eRCC_IO_SET_RTC_Clock_CONFIG, HAL_RCC_RTCCLKSource_LSE, NULL, 0, 0);
    /* ck_spre(1Hz) = RTCCLK(LSE) /(uiAsynchPrediv + 1)*(uiSynchPrediv + 1)*/

    /* Enable the RTC Clock */
    HalDrvRccIOCtrl(eRCC_IO_SET_RTC_Clock_ENABLE, 0, NULL, 0, HAL_ENABLE);

    HalDrvRtcIOCtrl(eRtc_IO_DeInit, 0, NULL, 0, 0);

    /* Wait for RTC APB registers synchronisation */
    HalDrvRtcIOCtrl(eRtc_IO_WaitForSync, 0, NULL, 0, 0);

    /* Configure the RTC data register and RTC prescaler */
    RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
    RTC_InitStructure.RTC_SynchPrediv = 0xFF;
    RTC_InitStructure.RTC_HourFormat = HAL_RTC_HourFormat_24;

    /* Check on RTC init */
    if ( HalDrvRtcIOCtrl(eRtc_IO_Init, 0, (char*)&RTC_InitStructure, sizeof(stHalRTC_InitTypeDef), 0) == HAL_ERROR )
    {
        printf("RTC: RTC Prescaler Config failed\r\n");
    }
    else {
    //		printf("RTC Prescaler Config success\r\n");
    }

    /* Enable RTC Alarm A Interrupt */
    HalDrvRtcIOCtrl(eRtc_IO_IntEnable, HAL_RTC_IT_ALRA, NULL, 0, HAL_DISABLE);

    /* Enable the alarm */
    HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_DISABLE);

    HalDrvRtcIOCtrl(eRtc_IO_ClearFlagStatus, HAL_RTC_FLAG_ALRAF, NULL, 0, 0);

    /* Set Date Week/Date/Month/Year */
    stRTC_DateTime.RtcDate.RTC_WeekDay   = HAL_RTC_Weekday_Sunday;
    stRTC_DateTime.RtcDate.RTC_Date      = 1;
    stRTC_DateTime.RtcDate.RTC_Month     = 1;
    stRTC_DateTime.RtcDate.RTC_Year      = 17; // 2017-2000

    /* Set Time hh:mm:ss */
    stRTC_DateTime.RtcTime.RTC_H12     = HAL_RTC_H24H;
    stRTC_DateTime.RtcTime.RTC_Hours   = 0x00;
    stRTC_DateTime.RtcTime.RTC_Minutes = 0x00;
    stRTC_DateTime.RtcTime.RTC_Seconds = 0x00;

    HalDrvRtcWrite(eRtcBin, eRtcAll, (char*)&stRTC_DateTime, sizeof(stHalRTCTypeDef), 0);

    /* Write BkUp DR0 */
    HalDrvRtcIOCtrl(eRtc_IO_SetBackupReg, HAL_RTC_BKP_DR0, NULL, 0, HAL_BKP_DR0_RTC_WAKEUP_VALUE);

    return;
}

void HalDrvRtc_Initial(void)
{
   if ( HalRTC_ReadBackupRegister(HAL_RTC_BKP_DR0, HAL_BKP_DR0_RTC_WAKEUP_VALUE) != HAL_SUCCESS )
    {
        /* RTC configuration  */
        HalDrvRtc_Config();

        HalDrvRtcIOCtrl(eRtc_IO_BypassEnable, 0, NULL, 0, HAL_ENABLE);
    }
    else
    {
        /* Check if the Power On Reset flag is set */
        if (HalDrvRccIOCtrl(eRCC_IO_GET_FLAG_STATUS, HAL_RCC_FLAG_PORRST, NULL, 0, 0) != HAL_RESET)
        {
            /* Power On Reset occurred     */
        }
            /* Check if the Pin Reset flag is set */
        else if (HalDrvRccIOCtrl(eRCC_IO_GET_FLAG_STATUS, HAL_RCC_FLAG_PINRST, NULL, 0, 0) != HAL_RESET)
        {
            /* External Reset occurred */
        }

        /* Enable the PWR clock */
        HalDrvRccIOCtrl(eRCC_IO_Power_Clock, 0, NULL, 0, HAL_ENABLE);
        /* Allow access to RTC */
        HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);
        HalDrvRtcIOCtrl(eRtc_IO_BypassEnable, 0, NULL, 0, HAL_ENABLE);

        /* Wait for RTC APB registers synchronisation */
        HalDrvRtcIOCtrl(eRtc_IO_WaitForSync, 0, NULL, 0, 0);
    }
}

void HalDrvRtc_SetAlarmTime(stHalRTCTypeDef *pRTCDateTime)
{
    stHalRTC_AlarmTypeDef RTC_AlarmStructure;
    stHalEXTI_InitTypeDef EXTI_InitStructure;
    stHalNVIC_InitTypeDef NVIC_InitStructure;

    /* Disable alarm A interrupt */
    HalDrvRtcIOCtrl(eRtc_IO_IntEnable, HAL_RTC_IT_ALRA, NULL, 0, HAL_DISABLE);

    /* Disable the RTC Clock */
#if defined(STM32F427X)
    HalDrvRccIOCtrl(eRCC_IO_SET_RTC_Clock_ENABLE, 0, NULL, 0, HAL_DISABLE);
#endif
    /* Disable the alarmA */
    HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_DISABLE);


    /* EXTI configuration */
    HalDrvExIntIOCtrl(eEXTI_IO_ClearItStatus, HAL_EXTI_Line17, NULL, 0, 0);
    EXTI_InitStructure.EXTI_Line    = HAL_EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode    = eEXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = eEXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = HAL_ENABLE;
    HalDrvExIntIOCtrl(eEXTI_IO_INIT, 0, (char*)&EXTI_InitStructure, sizeof(EXTI_InitStructure), 0);

    /* Enable the RTC Alarm Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = HAL_RTC_Alarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_Group_0_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_RTC_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);


    /* Set the alarm A Masks */
    RTC_AlarmStructure.RTC_AlarmTime.RTC_H12 = HAL_RTC_H24H;
    RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours = pRTCDateTime->RtcTime.RTC_Hours;
    RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes = pRTCDateTime->RtcTime.RTC_Minutes;
    RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds = pRTCDateTime->RtcTime.RTC_Seconds;
    RTC_AlarmStructure.RTC_AlarmDateWeekDay = computeDayOfWeek(pRTCDateTime->RtcDate.RTC_Year, pRTCDateTime->RtcDate.RTC_Month, pRTCDateTime->RtcDate.RTC_Date);
    RTC_AlarmStructure.RTC_AlarmDateWeekDaySel = HAL_RTC_AlarmDateWeekDaySel_WeekDay;
    RTC_AlarmStructure.RTC_AlarmMask = HAL_RTC_AlarmMask_DateWeekDay;

    HalDrvRtcIOCtrl(eRtc_IO_SetAlarmTime, HAL_RTC_Alarm_A, (char*)&RTC_AlarmStructure, sizeof(stHalRTC_AlarmTypeDef), HAL_RTC_Format_BIN);

    // confirm wakeup alram set
    memset((void*)&RTC_AlarmStructure, 0x00, sizeof(RTC_AlarmStructure));
    HalDrvRtcIOCtrl(eRtc_IO_GetAlarmTime, HAL_RTC_Alarm_A, (char*)&RTC_AlarmStructure, sizeof(stHalRTC_AlarmTypeDef), HAL_RTC_Format_BIN);




    /* Enable alarm A interrupt */
    HalDrvRtcIOCtrl(eRtc_IO_IntEnable, HAL_RTC_IT_ALRA, NULL, 0, HAL_ENABLE);

    /* Enable the RTC Clock */
    HalDrvRccIOCtrl(eRCC_IO_SET_RTC_Clock_ENABLE, 0, NULL, 0, HAL_ENABLE);

    /* Enable the alarmA */
    HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_ENABLE);
    

    /* Wait for RTC APB registers synchronisation */
    HalDrvRtcIOCtrl(eRtc_IO_WaitForSync, 0, NULL, 0, 0);
}


void RTC_Alarm_IRQHandler(void)
{
    /* Check on the Alarm A flag and on the number of interrupts per Second (60*8) */
    eHalFlagStatus eHalFlag;
    eHalFlag = (eHalFlagStatus)HalDrvRtcIOCtrl(eRtc_IO_GetITFlagStatus, HAL_RTC_IT_ALRA, NULL, 0, 0);
    if( eHalFlag != HAL_RESET) {
        /* Clear RTC AlarmA Flags */
        HalDrvRtcIOCtrl(eRtc_IO_ClearITFlagStatus, HAL_RTC_IT_ALRA, NULL, 0, 0);

        /* Disable the alarmA */
        HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_DISABLE);

        HalDrvRtcIOCtrl(eRtc_IO_ClearFlagStatus, HAL_RTC_FLAG_ALRAF, NULL, 0, 0);
    }

    /* Clear the EXTI line 17 */
    HalDrvExIntIOCtrl(eEXTI_IO_ClearItStatus, HAL_EXTI_Line17, NULL, 0, 0);

    return;
}



//------------------------------------------------------------------------------
//  void RTC_WKUP_IRQHandler(void)/void 5_10_IRQHandler(void)
//  description :
//
//------------------------------------------------------------------------------
void RTC_WKUP_IRQHandler(void)
{
//    Trace("R ");
    eHalFlagStatus eHalFlag;
    eHalFlag = (eHalFlagStatus)HalDrvRtcIOCtrl(eRtc_IO_GetITFlagStatus, HAL_RTC_IT_WUT, NULL, 0, 0);
    if( eHalFlag != HAL_RESET)
    {
        HalDrvRtcIOCtrl(eRtc_IO_ClearITFlagStatus, HAL_RTC_IT_WUT, NULL, 0, 0);

        HalDrvExIntIOCtrl(eEXTI_IO_ClearItStatus, HAL_EXTI_Line22, NULL, 0, 0);
    }
}

eHalErrorStatus HalRTC_ReadBackupRegister(unsigned int unRtcBKPReg, unsigned int unBKValue)
{
    eHalErrorStatus eHalRet     = HAL_SUCCESS;
    unsigned int unBKRegValue   = 0;

    unBKRegValue = HalDrvRtcIOCtrl(eRtc_IO_GetBackupReg, unRtcBKPReg, NULL, 0, 0);

    if ( unBKRegValue == unBKValue )    eHalRet = HAL_SUCCESS;  // warm reset
    else                                eHalRet = HAL_ERROR;    // cold reset

    return eHalRet;
}

////////////////////////////////////////////////////////////////////////////////////////
// base driver functions
int HalDrvRtcOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    HalDrvRtc_Initial();

    return HAL_RETURN_SUCCESS;
}

int HalDrvRtcRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    unsigned int nMode = nLparam;
    unsigned int nType = nRparam;
#if defined(STM32F427X)
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;
#elif defined(AT32F435VMT7)
    ertc_time_type RTC_TimeStructure;
#endif

    if( nMode == eRtcBin )
    {
        if( (nType & eRtcTime) )
        {
#if defined(STM32F427X)
            RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
#elif defined(AT32F435VMT7)
            ertc_calendar_get(&RTC_TimeStructure);
#endif
        }
        if( (nType & eRtcDate) )
        {
#if defined(STM32F427X)
            RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
#elif defined(AT32F435VMT7)
            ertc_calendar_get(&RTC_TimeStructure);
#endif
        }
    }
    if( nMode == eRtcHex )
    {
        // not supported
    }

    if(nLength == sizeof(stHalRTCTypeDef))
    {
        stHalRTCTypeDef* pstHalRtcTime = (stHalRTCTypeDef *)pBuffer;

#if defined(STM32F427X)
        /* Get Date Week/Date/Month/Year */
        pstHalRtcTime->RtcDate.RTC_Year     = RTC_DateStructure.RTC_Year;
        pstHalRtcTime->RtcDate.RTC_Month    = RTC_DateStructure.RTC_Month;
        pstHalRtcTime->RtcDate.RTC_Date     = RTC_DateStructure.RTC_Date;
        pstHalRtcTime->RtcDate.RTC_WeekDay  = RTC_DateStructure.RTC_WeekDay;
        /* Get Time hh:mm:ss */
        pstHalRtcTime->RtcTime.RTC_Hours    = RTC_TimeStructure.RTC_Hours;
        pstHalRtcTime->RtcTime.RTC_Minutes  = RTC_TimeStructure.RTC_Minutes;
        pstHalRtcTime->RtcTime.RTC_Seconds  = RTC_TimeStructure.RTC_Seconds;
        pstHalRtcTime->RtcTime.RTC_H12      = RTC_TimeStructure.RTC_H12; 
#elif defined(AT32F435VMT7)
        /* Get Date Week/Date/Month/Year */
        pstHalRtcTime->RtcDate.RTC_Year     = RTC_TimeStructure.year;
        pstHalRtcTime->RtcDate.RTC_Month    = RTC_TimeStructure.month;
        pstHalRtcTime->RtcDate.RTC_Date     = RTC_TimeStructure.day;
        pstHalRtcTime->RtcDate.RTC_WeekDay  = RTC_TimeStructure.week;
        /* Get Time hh:mm:ss */
        pstHalRtcTime->RtcTime.RTC_Hours    = RTC_TimeStructure.hour;
        pstHalRtcTime->RtcTime.RTC_Minutes  = RTC_TimeStructure.min;
        pstHalRtcTime->RtcTime.RTC_Seconds  = RTC_TimeStructure.sec;
        pstHalRtcTime->RtcTime.RTC_H12      = RTC_TimeStructure.ampm; 
 #endif
    }
    else
        return HAL_RETURN_FAIL;
    
    return HAL_RETURN_SUCCESS;
}

int HalDrvRtcWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    int nMode = nLparam;
    int nType = nRparam;
    stHalRTCTypeDef* pstHalRtcTypeDef = (stHalRTCTypeDef*)pBuffer;
    eHalErrorStatus eRetError = HAL_SUCCESS;
    
    if( nMode == eRtcBin )
    {
        if( (nType & eRtcTime) )
        {
#if defined(STM32F427X)
            RTC_TimeTypeDef stTimeType;

            stTimeType.RTC_Hours    = pstHalRtcTypeDef->RtcTime.RTC_Hours;
            stTimeType.RTC_Minutes  = pstHalRtcTypeDef->RtcTime.RTC_Minutes;
            stTimeType.RTC_Seconds  = pstHalRtcTypeDef->RtcTime.RTC_Seconds;
            stTimeType.RTC_H12      = pstHalRtcTypeDef->RtcTime.RTC_H12;

            RTC_SetTime(RTC_Format_BIN, &stTimeType);
#elif defined(AT32F435VMT7)
            ertc_am_pm_type eAmPm;
            if ( pstHalRtcTypeDef->RtcTime.RTC_H12 == HAL_RTC_H12_AM )  eAmPm = ERTC_AM;
            else                                                        eAmPm = ERTC_PM;

            ertc_time_set(pstHalRtcTypeDef->RtcTime.RTC_Hours, 
                            pstHalRtcTypeDef->RtcTime.RTC_Minutes, 
                            pstHalRtcTypeDef->RtcTime.RTC_Seconds, 
                            eAmPm);
#endif
        }
        if( (nType & eRtcDate) )
        {
#if defined(STM32F427X)
            RTC_DateTypeDef stRtcDateType;
            stRtcDateType.RTC_WeekDay   = pstHalRtcTypeDef->RtcDate.RTC_WeekDay;
            stRtcDateType.RTC_Month     = pstHalRtcTypeDef->RtcDate.RTC_Month;
            stRtcDateType.RTC_Date      = pstHalRtcTypeDef->RtcDate.RTC_Date;
            stRtcDateType.RTC_Year      = pstHalRtcTypeDef->RtcDate.RTC_Year;
               
            eRetError = (eHalErrorStatus)RTC_SetDate(RTC_Format_BIN, 
                                        (RTC_DateTypeDef*)&stRtcDateType);
#elif defined(AT32F435VMT7)
            eRetError = (eHalErrorStatus)ertc_date_set(pstHalRtcTypeDef->RtcDate.RTC_Year,
                            pstHalRtcTypeDef->RtcDate.RTC_Month,
                            pstHalRtcTypeDef->RtcDate.RTC_Date,
                            pstHalRtcTypeDef->RtcDate.RTC_WeekDay);
#endif
        }
    }
    if( nMode == eRtcHex )
    {
    }

    if ( eRetError == HAL_ERROR )
    {
        printf("[%s]Set time or Date Error \r\n", __FUNCTION__);
        return HAL_RETURN_FAIL;
    }

    return HAL_RETURN_SUCCESS;
}

int HalDrvRtcIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{

    int nIOMode = nLparam;
    eHalReturnStatus eRetStatus = HAL_RETURN_SUCCESS;

    switch ( nIOMode )
    {
        case eRtc_IO_Init:
        {
            stHalRTC_InitTypeDef* pRTC_InitStructure = (stHalRTC_InitTypeDef*)pBuffer;
#if defined(STM32F427X)
           return RTC_Init((RTC_InitTypeDef*)pRTC_InitStructure);
#elif defined(AT32F435VMT7)
            ertc_hour_mode_set_type HourModeType;
            unsigned short usDiv_A, usDiv_B;

            /* ertc second(1hz) = ertc_clk / (div_a + 1) * (div_b + 1) */
            usDiv_A = pRTC_InitStructure->RTC_AsynchPrediv; // 127 0x7F
            usDiv_B = pRTC_InitStructure->RTC_SynchPrediv;  // 255 0xFF

            ertc_divider_set(usDiv_A, usDiv_B); //127, 255
            /* configure the ertc hour mode */
            if ( pRTC_InitStructure->RTC_HourFormat == HAL_RTC_HourFormat_12 )
                HourModeType = ERTC_HOUR_MODE_12;
            else if ( pRTC_InitStructure->RTC_HourFormat == HAL_RTC_HourFormat_24 )
                HourModeType = ERTC_HOUR_MODE_24;

            return ertc_hour_mode_set(HourModeType);
#endif
        }
            break;
        case eRtc_IO_DeInit:
#if defined(STM32F427X)
            RTC_DeInit();
#elif defined(AT32F435VMT7)
            ertc_reset();
#endif
            break;  
        case eRtc_IO_ShowAppTime:
        {
            stHalRTCTypeDef stHalRtcDateTime;
            APP_TimeShow(&stHalRtcDateTime);
        }
            break;
        case eRtc_IO_SetTime:
        {
            stHalRTCTypeDef* pstHalRtcDateTime = (stHalRTCTypeDef*)pBuffer;
            eHalReturnStatus eRetStatus = (eHalReturnStatus)HalDrvRtcWrite(eRtcBin, eRtcAll, (char*)pstHalRtcDateTime, sizeof(stHalRTCTypeDef), 0);
            if ( eRetStatus == HAL_RETURN_SUCCESS )
            {
                printf("[eRtc_IO_SetTime]RTC_Date/Time Structure %04d-%02d-%02d(WeekDay %d) %02d:%02d:%02d(RTC AM/PM %d)\n",
                    pstHalRtcDateTime->RtcDate.RTC_Year+2000,
                    pstHalRtcDateTime->RtcDate.RTC_Month,
                    pstHalRtcDateTime->RtcDate.RTC_Date,
                    pstHalRtcDateTime->RtcDate.RTC_WeekDay,
                    pstHalRtcDateTime->RtcTime.RTC_Hours,
                    pstHalRtcDateTime->RtcTime.RTC_Minutes,
                    pstHalRtcDateTime->RtcTime.RTC_Seconds, 
                    pstHalRtcDateTime->RtcTime.RTC_H12);

            }
        }
        break;
        case eRtc_IO_GetAlarmTime:
        {
#if defined(STM32F427X)
            
#elif defined(AT32F435VMT7)
            unsigned int unAlarm = nRparam;
            stHalRTC_AlarmTypeDef *pAlarmStructure = (stHalRTC_AlarmTypeDef*)pBuffer;
            ertc_alarm_value_type AlarmValue;

            if ( unAlarm == HAL_RTC_Alarm_A )   unAlarm = ERTC_ALA;
            else                                unAlarm = ERTC_ALB;
            ertc_alarm_get((ertc_alarm_type)unAlarm, &AlarmValue);

            pAlarmStructure->RTC_AlarmTime.RTC_Hours    = AlarmValue.hour;
            pAlarmStructure->RTC_AlarmTime.RTC_Minutes  = AlarmValue.min;
            pAlarmStructure->RTC_AlarmTime.RTC_Seconds  = AlarmValue.sec;
            pAlarmStructure->RTC_AlarmTime.RTC_H12      = AlarmValue.ampm;
            pAlarmStructure->RTC_AlarmMask              = AlarmValue.mask;
            pAlarmStructure->RTC_AlarmDateWeekDaySel    = AlarmValue.week_date_sel;
            pAlarmStructure->RTC_AlarmDateWeekDay       = AlarmValue.week;

            printf("[eRtc_IO_GetAlarmTime](WeekDay %d, WeekDaySel %d, Mask 0x%X) %02d:%02d:%02d(RTC AM/PM %d)\r\n",
                            pAlarmStructure->RTC_AlarmDateWeekDay,
                            pAlarmStructure->RTC_AlarmDateWeekDaySel,
                            pAlarmStructure->RTC_AlarmMask,
                            pAlarmStructure->RTC_AlarmTime.RTC_Hours,
                            pAlarmStructure->RTC_AlarmTime.RTC_Minutes,
                            pAlarmStructure->RTC_AlarmTime.RTC_Seconds,
                            pAlarmStructure->RTC_AlarmTime.RTC_H12);
#endif
        }
            break;
        case eRtc_IO_SetAlarmTime:
        {   
            //***************************************************
            // not support ---> HAL_RTC_Format_BCD
            //***************************************************
            stHalRTC_AlarmTypeDef* pRTC_AlarmStructure = (stHalRTC_AlarmTypeDef*)pBuffer;
            unsigned int unAlarm = nRparam;
#if defined(STM32F427X)
            RTC_SetAlarm(RTC_Format_BIN, (unsigned int)unAlarm, (RTC_AlarmTypeDef*)pRTC_AlarmStructure);
#elif defined(AT32F435VMT7)
            unsigned char ucWeekDate, ucHour, ucMin, ucSec;
            unsigned int unAlarmMask = HAL_RTC_AlarmMask_None;
            ertc_am_pm_type eAmPm;
            ertc_week_date_select_type AlarmWeekDateSel;
         
            if ( unAlarm == HAL_RTC_Alarm_A )   unAlarm = ERTC_ALA;
            else                                unAlarm = ERTC_ALB;

            if ( pRTC_AlarmStructure->RTC_AlarmMask & HAL_RTC_AlarmMask_DateWeekDay )
                unAlarmMask  = ERTC_ALARM_MASK_DATE_WEEK;
            if ( pRTC_AlarmStructure->RTC_AlarmMask & HAL_RTC_AlarmMask_Hours )
                unAlarmMask |= ERTC_ALARM_MASK_HOUR;
            if ( pRTC_AlarmStructure->RTC_AlarmMask & HAL_RTC_AlarmMask_Minutes )
                unAlarmMask  |= ERTC_ALARM_MASK_MIN;
            if ( pRTC_AlarmStructure->RTC_AlarmMask & HAL_RTC_AlarmMask_Seconds )
                unAlarmMask |= ERTC_ALARM_MASK_SEC;
            ertc_alarm_mask_set((ertc_alarm_type)unAlarm, unAlarmMask);

            if ( pRTC_AlarmStructure->RTC_AlarmDateWeekDaySel == HAL_RTC_AlarmDateWeekDaySel_Date )
                AlarmWeekDateSel  = ERTC_SLECT_DATE;
            else //if ( pRTC_AlarmStructure->RTC_AlarmDateWeekDaySel == HAL_RTC_AlarmDateWeekDaySel_WeekDay )
                AlarmWeekDateSel = ERTC_SLECT_WEEK;
            ertc_alarm_week_date_select((ertc_alarm_type)unAlarm, AlarmWeekDateSel);

            ucWeekDate  = pRTC_AlarmStructure->RTC_AlarmDateWeekDay;
            ucHour      = pRTC_AlarmStructure->RTC_AlarmTime.RTC_Hours;
            ucMin       = pRTC_AlarmStructure->RTC_AlarmTime.RTC_Minutes;
            ucSec       = pRTC_AlarmStructure->RTC_AlarmTime.RTC_Seconds;
            
            if ( pRTC_AlarmStructure->RTC_AlarmTime.RTC_H12 == HAL_RTC_H12_AM ) eAmPm = ERTC_AM;
            else                                                                eAmPm = ERTC_PM;

            ertc_alarm_set((ertc_alarm_type)unAlarm, ucWeekDate, ucHour, ucMin, ucSec, eAmPm);
#endif
        }
            break;
        case eRtc_IO_AlarmEnable:
        {
            unsigned int unAlarm = nRparam;
#if defined(STM32F427X)
            RTC_AlarmCmd(unAlarm, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            if ( unAlarm == HAL_RTC_Alarm_A )   unAlarm = ERTC_ALA;
            else                                unAlarm = ERTC_ALB;

            ertc_alarm_enable((ertc_alarm_type)unAlarm, (confirm_state)nOverlap);
#endif
        }
            break;
        case eRtc_IO_GetBackupReg:
        {
            unsigned int unRtcBKPReg = nRparam;
            unsigned int unRtcBKReadValue = 0;
#if defined(STM32F427X)
            unRtcBKReadValue = RTC_ReadBackupRegister(unRtcBKPReg);
#elif defined(AT32F435VMT7)
            unRtcBKReadValue = ertc_bpr_data_read((ertc_dt_type)unRtcBKPReg);
#endif
            return unRtcBKReadValue;
        }
            break;
        case eRtc_IO_SetBackupReg:
#if defined(STM32F427X)
            RTC_WriteBackupRegister(nRparam, nOverlap);
#elif defined(AT32F435VMT7)
            ertc_bpr_data_write((ertc_dt_type)nRparam, nOverlap);
#endif
            break;

        case eRtc_IO_WaitForSync:
#if defined(STM32F427X)
            RTC_WaitForSynchro();
#elif defined(AT32F435VMT7)
            ertc_wait_update();
#endif
            break;
        case eRtc_IO_BypassEnable:
#if defined(STM32F427X)
            RTC_BypassShadowCmd((FunctionalState)nOverlap);
#elif defined(AT32F435VMT7) 
#endif
            break;

        case eRtc_IO_GetFlagStatus:
        {
            unsigned int unFlagStatus = nRparam;
#if defined(STM32F427X)
            return RTC_GetFlagStatus(unFlagStatus);
#elif defined(AT32F435VMT7)
            if      ( unFlagStatus == HAL_RTC_FLAG_RECALPF  ) unFlagStatus = ERTC_CALUPDF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_TAMP1F   ) unFlagStatus = ERTC_TP1F_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_TSOVF    ) unFlagStatus = ERTC_TSOF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_TSF      ) unFlagStatus = ERTC_TSF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_WUTF     ) unFlagStatus = ERTC_WATF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_ALRBF    ) unFlagStatus = ERTC_ALBF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_ALRAF    ) unFlagStatus = ERTC_ALAF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_INITF    ) unFlagStatus = ERTC_IMF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_RSF      ) unFlagStatus = ERTC_UPDF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_INITS    ) unFlagStatus = ERTC_INITF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_SHPF     ) unFlagStatus = ERTC_TADJF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_WUTWF    ) unFlagStatus = ERTC_WATWF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_ALRBWF   ) unFlagStatus = ERTC_ALBWF_FLAG;
            else if ( unFlagStatus == HAL_RTC_FLAG_ALRAWF   ) unFlagStatus = ERTC_ALAWF_FLAG;

            return ertc_flag_get(unFlagStatus);
/*
* @brief  get flag status.
* @param  flag: specifies the flag to check.
*         this parameter can be one of the following values:
*         - ERTC_ALAWF_FLAG: alarm a register allows write flag.
*         - ERTC_ALBWF_FLAG: alarm b register allows write flag.
*         - ERTC_WATWF_FLAG: wakeup timer register allows write flag.
*         - ERTC_TADJF_FLAG: time adjustment flag.
*         - ERTC_INITF_FLAG: calendar initialization flag.
*         - ERTC_UPDF_FLAG: calendar update flag.
*         - ERTC_IMF_FLAG: enter initialization mode flag.
*         - ERTC_ALAF_FLAG: alarm clock a flag.
*         - ERTC_ALBF_FLAG: alarm clock b flag.
*         - ERTC_WATF_FLAG: wakeup timer flag.
*         - ERTC_TSF_FLAG: timestamp flag.
*         - ERTC_TSOF_FLAG: timestamp overflow flag.
*         - ERTC_TP1F_FLAG: tamper detection 1 flag.
*         - ERTC_TP2F_FLAG: tamper detection 2 flag.
*         - ERTC_CALUPDF_FLAG: calibration value update completed flag.
* @retval the new state of flag (SET or RESET).
*/
#endif
        }
            break;

        case eRtc_IO_ClearFlagStatus:
#if defined(STM32F427X)
            RTC_ClearFlag(nRparam);
#elif defined(AT32F435VMT7)
            // same with Artery flag & STM flag
            ertc_flag_clear(nRparam);
#endif
            break;
        case eRtc_IO_GetITFlagStatus:
        {
           unsigned int unITFlag = nRparam;
#if defined(STM32F427X)
            return RTC_GetITStatus(unITFlag);
#elif defined(AT32F435VMT7)
            if      ( unITFlag == HAL_RTC_IT_TS     ) unITFlag = ERTC_TSF_FLAG;     // ERTC_TS_INT;
            else if ( unITFlag == HAL_RTC_IT_WUT    ) unITFlag = ERTC_WATF_FLAG;    // ERTC_WAT_INT;
            else if ( unITFlag == HAL_RTC_IT_ALRB   ) unITFlag = ERTC_ALBF_FLAG;    // ERTC_ALB_INT;
            else if ( unITFlag == HAL_RTC_IT_ALRA   ) unITFlag = ERTC_ALAF_FLAG;    // ERTC_ALA_INT;
            else if ( unITFlag == HAL_RTC_IT_TAMP   ) unITFlag = ERTC_TP1F_FLAG;     // ERTC_TP_INT;
            else if ( unITFlag == HAL_RTC_IT_TAMP1  ) unITFlag = ERTC_TP2F_FLAG;    // HAL_RTC_IT_TS;

            return ertc_flag_get(unITFlag);
            //return ertc_interrupt_get(unITFlag);
#endif
        }
            break;

        case eRtc_IO_ClearITFlagStatus:
        {
            unsigned int unITFlag = nRparam;
#if defined(STM32F427X)
            RTC_ClearITPendingBit(unITFlag);
#elif defined(AT32F435VMT7)
            if      ( unITFlag == HAL_RTC_IT_TS     ) unITFlag = ERTC_TSF_FLAG;     // ERTC_TS_INT;
            else if ( unITFlag == HAL_RTC_IT_WUT    ) unITFlag = ERTC_WATF_FLAG;    // ERTC_WAT_INT;
            else if ( unITFlag == HAL_RTC_IT_ALRB   ) unITFlag = ERTC_ALBF_FLAG;    // ERTC_ALB_INT;
            else if ( unITFlag == HAL_RTC_IT_ALRA   ) unITFlag = ERTC_ALAF_FLAG;    // ERTC_ALA_INT;
            else if ( unITFlag == HAL_RTC_IT_TAMP   ) unITFlag = ERTC_TP1F_FLAG;     // ERTC_TP_INT;
            else if ( unITFlag == HAL_RTC_IT_TAMP1  ) unITFlag = ERTC_TP2F_FLAG;    // HAL_RTC_IT_TS;

            ertc_flag_clear(unITFlag);
#endif
        }
            break;

        case eRtc_IO_IntEnable: 
        {
            unsigned int unITFlag = nRparam;
#if defined(STM32F427X)
            RTC_ITConfig(unITFlag, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            if      ( unITFlag == HAL_RTC_IT_TS     ) unITFlag = ERTC_TS_INT;
            else if ( unITFlag == HAL_RTC_IT_WUT    ) unITFlag = ERTC_WAT_INT;
            else if ( unITFlag == HAL_RTC_IT_ALRB   ) unITFlag = ERTC_ALB_INT;
            else if ( unITFlag == HAL_RTC_IT_ALRA   ) unITFlag = ERTC_ALA_INT;
            else if ( unITFlag == HAL_RTC_IT_TAMP   ) unITFlag = ERTC_TP_INT;
            //else if ( unITFlag == HAL_RTC_IT_TAMP1  ) unITFlag = ERTC_TP2F_FLAG;    // HAL_RTC_IT_TS;

            ertc_interrupt_enable(unITFlag, (confirm_state)nOverlap);
#endif  
        }
            break;
        case eRtc_IO_WakeupClockConfig:
        {
#if defined(STM32F427X)
            RTC_WakeUpClockConfig(nRparam);
#elif defined(AT32F435VMT7)
            ertc_wakeup_clock_set((ertc_wakeup_clock_type)nRparam);
#endif
        }
            break;
        case eRtc_IO_SetWakupCounter:
#if defined(STM32F427X)
            RTC_SetWakeUpCounter(nRparam);
#elif defined(AT32F435VMT7)
            ertc_wakeup_counter_set(nRparam);
#endif
            break;
        case eRtc_IO_WakeupEnable:
#if defined(STM32F427X)
            RTC_WakeUpCmd((FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            ertc_wakeup_enable((confirm_state)nOverlap);
#endif
            break;
        default:
            break;
    }

    return eRetStatus;
}

int HalDrvRtcClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

