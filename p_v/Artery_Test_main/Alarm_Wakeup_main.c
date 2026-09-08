/**
  **************************************************************************
  * @file     main.c
  * @version  v2.0.4
  * @date     2021-12-31
  * @brief    main program
  **************************************************************************
  *                       Copyright notice & Disclaimer
  *
  * The software Board Support Package (BSP) that is made available to 
  * download from Artery official website is the copyrighted work of Artery. 
  * Artery authorizes customers to use, copy, and distribute the BSP 
  * software and its related documentation for the purpose of design and 
  * development in conjunction with Artery microcontrollers. Use of the 
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */

#include "at32f435_437_board.h"
#include "at32f435_437_clock.h"
#include "AT32F435_437.h"
#include "AT32F435_437_ertc.h"
#include "HalHandler.h"

#define RTC_Alarm_IRQHandler    ERTCAlarm_IRQHandler    

extern uint8_t computeDayOfWeek(uint16_t y, uint8_t m, uint8_t d);
extern void MD_mDelay (const uint32_t msec);


/** @addtogroup AT32F435_periph_examples
  * @{
  */
  
/** @addtogroup 435_PWC_deepsleep_ertc_alarm PWC_deepsleep_ertc_alarm
  * @{
  */
  
/**
  * @brief  ertc configuration.
  * @param  none
  * @retval none
  */
void ertc_config(void)
{
  /* enable the pwc clock interface */
  crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE);
  
  /* allow access to ertc */
  pwc_battery_powered_domain_access(TRUE);

  /* reset ertc domain */
  crm_battery_powered_domain_reset(TRUE);
  crm_battery_powered_domain_reset(FALSE);
  
  /* enable the lext osc */
  crm_clock_source_enable(CRM_CLOCK_SOURCE_LEXT, TRUE);

  /* wait till lext is ready */
  while(crm_flag_get(CRM_LEXT_STABLE_FLAG) == RESET);

  /* select the ertc clock source */
  crm_ertc_clock_select(CRM_ERTC_CLOCK_LEXT);

  /* enable the ertc clock */
  crm_ertc_clock_enable(TRUE);

  /* deinitializes the ertc registers */
  ertc_reset();

  /* wait for ertc apb registers synchronisation */
  ertc_wait_update();
  
  /* configure the ertc data register and ertc prescaler 
     ck_spre(1hz) = ertcclk(lext) /(ertc_clk_div_a + 1)*(ertc_clk_div_b + 1)*/
  ertc_divider_set(127, 255);

  /* configure the hour format is 24-hour format*/
  ertc_hour_mode_set(ERTC_HOUR_MODE_24);
  
  /* set the date: friday june 11th 2021 */
  ertc_date_set(21, 6, 11, 5);
  
  /* set the time to 06h 20mn 00s am */
  ertc_time_set(6, 20, 0, ERTC_AM);   
}

/**
  * @brief  ertc alarm configuration.
  * @param  none
  * @retval none
  */
void ertc_alarm_config(void)
{
  exint_init_type exint_init_struct;

  /* config the exint line of the ertc alarm */
  exint_init_struct.line_select   = EXINT_LINE_17;
  exint_init_struct.line_enable   = TRUE;
  exint_init_struct.line_mode     = EXINT_LINE_INTERRUPUT;
  exint_init_struct.line_polarity = EXINT_TRIGGER_RISING_EDGE;
  exint_init(&exint_init_struct);
  
  /* set the alarm 05h:20min:10s */
  ertc_alarm_mask_set(ERTC_ALA, ERTC_ALARM_MASK_DATE_WEEK | ERTC_ALARM_MASK_HOUR | ERTC_ALARM_MASK_MIN);
  ertc_alarm_week_date_select(ERTC_ALA, ERTC_SLECT_DATE);
  ertc_alarm_set(ERTC_ALA, 31, 6, 20, 5, ERTC_AM);
  
  /* enable the ertc interrupt */
  nvic_irq_enable(ERTCAlarm_IRQn, 0, 0);
  
  /* enable ertc alarm a interrupt */
  ertc_interrupt_enable(ERTC_ALA_INT, TRUE);
  
  /* enable the alarm */
  ertc_alarm_enable(ERTC_ALA, TRUE);
}

/**
  * @brief  ertc alarm value set.
  * @param  alam_index : ERTC Alarm Seconds value
  * @retval none
  */
void TestGetDatefromTime2(ertc_time_type* pstDate, uint32_t nTime)
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;
    
    if(nTime < 1)        nTime = 0;

    //Retrieve hours, minutes and seconds
    pstDate->sec = nTime % 60;
    nTime /= 60;
    pstDate->min = nTime % 60;
    nTime /= 60;
    pstDate->hour = nTime % 24;
    nTime /= 24;

    //Convert Unix time to date
    a = (uint32_t) ((4 * nTime + 102032) / 146097 + 15);
    b = (uint32_t) (nTime + 2442113 + a - (a / 4));
    c = (20 * b - 2442) / 7305;
    d = b - 365 * c - (c / 4);
    e = d * 1000 / 30601;
    f = d - e * 30 - e * 601 / 1000;

    //January and February are counted as months 13 and 14 of the previous year
    if(e <= 13)
    {
        c -= 4716;
        e -= 1;
    }
    else
    {
        c -= 4715;
        e -= 13;
    }

    //Retrieve year, month and day
    pstDate->year = c-2000;
    pstDate->month = e;
    pstDate->day = f;

    //Calculate day of week
    pstDate->week = computeDayOfWeek(c, e, f);
}

uint32_t TestGetTimefromDate2(ertc_time_type stDate)
{
    uint32_t y;
    uint32_t m;
    uint32_t d;
    uint32_t t;

    //Year
    y = stDate.year+2000;
    //Month of year
    m = stDate.month;
    //Day of month
    d = stDate.day;

    //January and February are counted as months 13 and 14 of the previous year
    if(m <= 2)
    {
        m += 12;
        y -= 1;
    }

    //Convert years to days
    t = (365 * y) + (y / 4) - (y / 100) + (y / 400);
    //Convert months to days
    t += (30 * m) + (3 * (m + 1) / 5) + d;
    //Unix time starts on January 1st, 1970
    t -= 719561;
    //Convert days to seconds
    t *= 86400;
    //Add hours, minutes and seconds
    t += (3600 * stDate.hour) + (60 * stDate.min) + stDate.sec;
    //Return Unix time
    return t;
}

void ertc_alarm_value_set(uint32_t alam_index)
{
  ertc_time_type ertc_time_struct,ertc_time_struct2;
   uint32_t unTime;
  /* disable the alarm */
  ertc_alarm_enable(ERTC_ALA, FALSE);
  ertc_calendar_get(&ertc_time_struct);
  unTime = TestGetTimefromDate2(ertc_time_struct);
  
    unTime += alam_index;  
    TestGetDatefromTime2(&ertc_time_struct2, unTime); // wakeup 시간 변경

    
  ertc_alarm_set(ERTC_ALA, ertc_time_struct2.day, ertc_time_struct2.hour, ertc_time_struct2.min, ertc_time_struct2.sec, ertc_time_struct2.ampm);
  
  /* disable the alarm */
  ertc_alarm_enable(ERTC_ALA, TRUE);
}

/**
  * @brief  system clock recover.
  * @param  none
  * @retval none
  */
void system_clock_recover(void)
{
  /* enable external high-speed crystal oscillator - hext */
  crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, TRUE);
  
  /* wait till hext is ready */
  while(crm_hext_stable_wait() == ERROR);
  
  /* enable pll */
  crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, TRUE);
  
  /* wait till pll is ready */
  while(crm_flag_get(CRM_PLL_STABLE_FLAG) == RESET);
  
  /* enable auto step mode */
  crm_auto_step_mode_enable(TRUE);
  
  /* select pll as system clock source */
  crm_sysclk_switch(CRM_SCLK_PLL);
  
  /* wait till pll is used as system clock source */
  while(crm_sysclk_switch_status_get() != CRM_SCLK_PLL);
}



void ertc_alarm_test(void)
{  
  exint_init_type exint_init_struct;
/*
  ertc_interrupt_enable(ERTC_ALA_INT, FALSE);
   crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, FALSE);
 // crm_ertc_clock_enable(FALSE);
   ertc_alarm_enable(ERTC_ALA, FALSE);
*/ 
  
     HalDrvRtcIOCtrl(eRtc_IO_IntEnable, HAL_RTC_IT_ALRA, NULL, 0, HAL_DISABLE);
  //    HalDrvRccIOCtrl(eRCC_IO_SET_RTC_Clock_ENABLE, 0, NULL, 0, HAL_DISABLE);
   HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_DISABLE);
  
   
  /* config the exint line of the ertc alarm */
  exint_init_struct.line_select   = EXINT_LINE_17;
  exint_init_struct.line_enable   = TRUE;
  exint_init_struct.line_mode     = EXINT_LINE_INTERRUPUT;
  exint_init_struct.line_polarity = EXINT_TRIGGER_RISING_EDGE;
  exint_init(&exint_init_struct);
  
  /* set the alarm 05h:20min:10s */
  //ertc_alarm_mask_set(ERTC_ALA, ERTC_ALARM_MASK_DATE_WEEK | ERTC_ALARM_MASK_HOUR | ERTC_ALARM_MASK_MIN);
  ertc_alarm_mask_set(ERTC_ALA, ERTC_ALARM_MASK_DATE_WEEK);
  ertc_alarm_week_date_select(ERTC_ALA, ERTC_SLECT_DATE);
  //ertc_alarm_set(ERTC_ALA, 31, 6, 20, 5, ERTC_AM);
  ertc_alarm_value_set(1800);
  /* enable the ertc interrupt */
  nvic_irq_enable(ERTCAlarm_IRQn, 0, 0);
  
  
 
    HalDrvRtcIOCtrl(eRtc_IO_IntEnable, HAL_RTC_IT_ALRA, NULL, 0, HAL_ENABLE);
    HalDrvRccIOCtrl(eRCC_IO_SET_RTC_Clock_ENABLE, 0, NULL, 0, HAL_ENABLE);
    HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_ENABLE);
 /* 
     ertc_interrupt_enable(ERTC_ALA_INT, TRUE);
   crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, TRUE);
 //    crm_ertc_clock_enable(TRUE);
   ertc_alarm_enable(ERTC_ALA, TRUE);
 */
}


/**
  * @brief  main function.
  * @param  none
  * @retval none
  */

#if 1
int main()
{
    HalHandlerInit();
    ertc_alarm_value_set(30);
    __IO unsigned int unSystick_index = 0;

        printf("ENTER STANDBY OR STOP(DeepSleep) MODE\r\n");
        while(HalUartGetFlagStatus((int)g_stUart7.pUARTreg, HAL_USART_FLAG_TC)== HAL_RESET);
        /********************************************************************/
        /* Don't print Debug Messages below									*/
        /********************************************************************/		
        HalHandlerDeInit();

#ifdef ENABLE_STANDBY_MODE
        HalDrvPowerIOCtrl(ePWR_IO_WakeupPinEnable, 0, NULL, 0, HAL_ENABLE);
        HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStandbyMode, 0, NULL, 0, 0);
#else
        HalDrvRtcIOCtrl(eRtc_IO_ClearFlagStatus, HAL_RTC_FLAG_ALRAF, NULL, 0, 0);
		HalDrvPwr_WkupPin_Config(HAL_WKUPPIN_INT);

        /* save systick register configuration */
        unSystick_index = SysTick->CTRL;
        unSystick_index &= ~((uint32_t)0xFFFFFFFE);
        /* disable systick */
        SysTick->CTRL &= (uint32_t)0xFFFFFFFE;

        HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStopMode, HAL_PWR_Regulator_LowPower, NULL, 0, HAL_PWR_STOPEntry_WFI);
        HalDrvPower_SYSCLKConfig_STOP();

        /* restore systick register configuration */
        SysTick->CTRL |= unSystick_index;

        NVIC_SystemReset();
        while(1)
        {
            MD_mDelay(10);
            NVIC_SystemReset();
        }
#endif
}
#else
int main(void)
{
  __IO uint32_t systick_index = 0;
  __IO uint32_t delay_index = 0;
  
  /* congfig the system clock */
  system_clock_config_GIT();  
HalGpioInit_Led();
  /* init at start board */
 // at32_board_init();  
  
  /* config priority group */  
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);

  /* enable pwc and bpr clock */
  crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE);  
  
  /* config ertc */
  ertc_config();
  
  /* set the wakeup time: 06h:20min:5s */
  ertc_alarm_config();
  
  //printf ("Start\r\n");
  while(1)
  {
    /* turn off the led light */  
    //at32_led_off(LED2);
HalGPIOSetVaule(GPIO_CAN_LED, true);
HalGPIOSetVaule(GPIO_LTE_LED, true);
HalGPIOSetVaule(GPIO_GPS_LED, true);
    
    /* save systick register configuration */
    systick_index = SysTick->CTRL;
    systick_index &= ~((uint32_t)0xFFFFFFFE);
    
    /* disable systick */
    SysTick->CTRL &= (uint32_t)0xFFFFFFFE;
    
    ertc_alarm_value_set(300);
    //ertc_alarm_test();
    
    /* select system clock source as hick before ldo set */
    crm_sysclk_switch(CRM_SCLK_HICK);
    
    /* wait till hick is used as system clock source */
    while(crm_sysclk_switch_status_get() != CRM_SCLK_HICK)
    {
    }
    
    pwc_ldo_output_voltage_set(PWC_LDO_OUTPUT_1V0);
    
    /* congfig the voltage regulator mode */
    pwc_voltage_regulate_set(PWC_REGULATOR_LOW_POWER);
    
    /* enter deep sleep mode */
    //printf ("Enter stopmode\r\n");
    pwc_deep_sleep_mode_enter(PWC_DEEP_SLEEP_ENTER_WFI);
    
    //printf ("exit stopmode\r\n");
    
      NVIC_SystemReset();
        while(1)
        {
            MD_mDelay(10);
            NVIC_SystemReset();
        }
    /* wake up from deep sleep mode, restore systick register configuration */
    SysTick->CTRL |= systick_index;
    
    /* turn on the led light */ 
    
HalGPIOSetVaule(GPIO_CAN_LED, true);
HalGPIOSetVaule(GPIO_LTE_LED, false);
HalGPIOSetVaule(GPIO_GPS_LED, true);
    
    
    /* wait clock stable */ 
    for(delay_index = 0; delay_index < 600; delay_index++)
    {
      __NOP();
    }
    
    /* resume ldo before system clock source enhance */
    pwc_ldo_output_voltage_set(PWC_LDO_OUTPUT_1V3);
    
    /* congfig the system clock */
    system_clock_recover();
    
    APP_Delay(500);

HalGPIOSetVaule(GPIO_CAN_LED, false);
HalGPIOSetVaule(GPIO_LTE_LED, false);
HalGPIOSetVaule(GPIO_GPS_LED, false);
   
  }
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 
