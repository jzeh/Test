/*
  ******************************************************************************
  * @file    HalTimerWatchDoc.c
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
#include "HalHandler.h"
#include "HalTimerWatchDoc.h"
#include "HalRccDriver.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#if defined(AT32F435VMT7)
#include "at32f435_437_wdt.h"
#define IWDG_Enable         wdt_enable
#define IWDG_ReloadCounter  wdt_counter_reload
#define IWDG_SetReload      wdt_reload_value_set
#define IWDG_SetPrescaler   wdt_divider_set
#endif
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

#define Trace(...)  //GITDebug(DEBUG_MODULES_SW_TMR,__VA_ARGS__)

//------------------------------------------------------------------------------
//  IWDG_Enable() -> Starting watchdogtimer
//                  if watchdogtimer is enabled, can't disable
// IWDG_ReloadCounter(); -> reload watchdogtimer-counter
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  void Set_Wdt(u16 ms)
//      description : watchdog timer clock setting
//
//      param1(mscnt) : mscnt value ( max 4096) unit 8ms
//                      ex) if (mscnt==1)   8*1=8ms
//                          if (mscnt==4096) 8*4096=32768ms
//------------------------------------------------------------------------------
void Set_Wdt(u16 mscnt)
{
#if defined(STM32F427X)
	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(HAL_IWDG_WriteAccess_Enable);
#elif defined(AT32F435VMT7)/* disable register write protection */
    wdt_register_write_enable(TRUE);  
#endif

	IWDG_SetPrescaler(HAL_IWDG_Prescaler_256); 

	if (mscnt>4096) {
		IWDG_SetReload(4096-1);
	}
	else{
		IWDG_SetReload(mscnt-1);
	}

	IWDG_ReloadCounter();

	return;
}

//------------------------------------------------------------------------------
//  u8 Chk_WdtReset(void)
//      description : check watchdog-reset
//
//      reuturn 0 : no wdt reset
//              1 : wdt reset
//
//------------------------------------------------------------------------------
u8 Chk_WdtReset(void)
{
    if (HalDrvRccIOCtrl(eRCC_IO_GET_FLAG_STATUS, HAL_RCC_FLAG_IWDGRST, NULL, 0, 0) != HAL_RESET){
        HalDrvRccIOCtrl(eRCC_IO_Clear_FLAG_STATUS, 0, NULL, 0, 0);
        return 1;
    }else{

        return 0;
    }
}

void SetWDTReset(uint16_t msDelay)
{
	uint16_t delay;

	if(msDelay > 32768) {
		msDelay = 32768;
	}

	if(msDelay < 8) {
		delay = 1;
	}
	else {
		delay = msDelay / 8;
	}

	Set_Wdt(delay);
	IWDG_Enable();

	return;
}


