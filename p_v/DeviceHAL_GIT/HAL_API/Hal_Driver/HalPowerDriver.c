/*
  ******************************************************************************
  * @file    HalPowerDriver.c
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
#include "STM32F4xx.h"
#include "STM32F4xx_pwr.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_pwc.h"
#endif
#include "HalPowerDriver.h"
#include "HalHandler.h"

#include "Autolink_Manager.h"
#include "Mngsystem.h"
#include "Power_Manager.h"
#include "GIT_OemInterface.h"
#include "HdDebug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define Trace(...)  GITDebug(DEBUG_MODULES_POWER,__VA_ARGS__)

#define GET_GPIO_PIN_NUM(PIN)      ((unsigned int)(PIN&0x0000FFFF))
#if defined(AT32F435VMT7)
#define EXTI0_IRQHandler    EXINT0_IRQHandler    
#endif

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#pragma section="BKSRAM"
unsigned int g_unBkramSwResetSignal @"BKSRAM";				// backup ram에 저장되는 변수
unsigned int g_unBkramTmpOdometer @"BKSRAM";
unsigned int g_unBkramClearOdoFlag @"BKSRAM";
unsigned int g_unBkramRtcSetSignalFlag @"BKSRAM";
unsigned short int g_usBkramResetCount @"BKSRAM";


bool g_bFlagWakeup=false;
bool g_bFlagStandby=false;


extern AUTOLINK_MANAGER_DATA AutoLinkManagerData;
extern BR_SystemInfo BkSram_SystemInfo;

#if defined(STM32F427X)
extern void PWR_EnterSTANDBYModeSystemOnly();
#endif
/* Private function prototypes -----------------------------------------------*/
void CheckWakeUpSignal();

void HalDrvPower_BKRam_init();

/* Private functions ---------------------------------------------------------*/

void HalDrvPwr_WkupPin_Config(eHalPwr_WakeUpMode eState)
{
	stHalGPIO_InitTypeDef GPIO_InitStructure;
	stHalEXTI_InitTypeDef EXTI_InitStructure;
	stHalNVIC_InitTypeDef NVIC_InitStructure;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOA_GROUP, NULL, 0, HAL_ENABLE);
    HalDrvRccIOCtrl(eRCC_IO_SYSCFG_Clock, 0, NULL, 0, HAL_ENABLE);

	/* Configure Button pin as input */
    GPIO_InitStructure.GPIO_DS  = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;//eGPIO_OType_OD; // GPIO_OType_PP
    GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Low_Speed;
    GPIO_InitStructure.GPIO_Pin = HAL_WAKEUP_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    if ( eState == HAL_WKUPPIN_INT )
        HalDrvPowerIOCtrl(ePWR_IO_WakeupPinEnable, 0, NULL, 0, HAL_ENABLE);
    else
        HalDrvPowerIOCtrl(ePWR_IO_WakeupPinEnable, 0, NULL, 0, HAL_DISABLE);

    /* Connect EXTI Line0 to PC0 pin */
    HalDrvExIntIOCtrl(eEXTI_IO_REG, HAL_EXTI_PortSourceGPIOA, NULL, 0, HAL_EXTI_PinSource0);
    HalDrvExIntIOCtrl(eEXTI_IO_ClearItStatus, HAL_EXTI_Line0, NULL, 0, 0);

	/* Configure Button EXTI line */
	EXTI_InitStructure.EXTI_Line = HAL_EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = eEXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = eEXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = HAL_ENABLE;
    HalDrvExIntIOCtrl(eEXTI_IO_INIT, 0, (char*)&EXTI_InitStructure, sizeof(EXTI_InitStructure), 0);

	/* Enable and set Button EXTI Interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannel = HAL_EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_Group_0_NVIC_IRQChannelPreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_STOPMODE_WAKEUP_IRQChannelSubPriority;
	NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;

    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);
}


void EXTI0_IRQHandler(void)
{
    if(HalDrvExIntIOCtrl(eEXTI_IO_GetItStatus, HAL_EXTI_Line0, NULL, 0, 0) != HAL_RESET)
    {
        /* Clear the Key Button EXTI line pending bit */
        HalDrvExIntIOCtrl(eEXTI_IO_ClearItStatus, HAL_EXTI_Line0, NULL, 0, 0);
    }
}


//--------------------------------------------------------------------------------------//
int HalDrvPowerOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    HalDrvPower_GetStandbyFlag(); // Don't change line
    HalDrvPower_BKRam_init();

    CheckWakeUpSignal();

    return HAL_RETURN_SUCCESS;
}

int HalDrvPowerRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{

    return HAL_RETURN_SUCCESS;
}

int HalDrvPowerWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvPowerIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    int nIoMode = nLparam;

    switch ( nIoMode )
    {
        case ePWR_IO_GetFlagStatus:
        {
#if defined(STM32F427X)
/*            
#define PWR_FLAG_WU                     PWR_CSR_WUF
#define PWR_FLAG_SB                     PWR_CSR_SBF
#define PWR_FLAG_PVDO                   PWR_CSR_PVDO
#define PWR_FLAG_BRR                    PWR_CSR_BRR
#define PWR_FLAG_VOSRDY                 PWR_CSR_VOSRDY
#define PWR_FLAG_ODRDY                  PWR_CSR_ODRDY
#define PWR_FLAG_ODSWRDY                PWR_CSR_ODSWRDY
#define PWR_FLAG_UDRDY                  PWR_CSR_UDSWRDY
*/

            return PWR_GetFlagStatus(nRparam);
#elif defined(AT32F435VMT7)
            unsigned int unFlags = nRparam;
/*
- PWC_WAKEUP_FLAG                     
- PWC_STANDBY_FLAG                            
- PWC_PVM_OUTPUT_FLAG       
*/
            if      ( unFlags == HAL_PWR_FLAG_WU ) unFlags = PWC_WAKEUP_FLAG;
            else if ( unFlags == HAL_PWR_FLAG_SB ) unFlags = PWC_STANDBY_FLAG;
            else if ( unFlags == HAL_PWR_FLAG_PVDO) unFlags = PWC_PVM_OUTPUT_FLAG;
            else
                return HAL_SET;

            return pwc_flag_get(unFlags);
#endif
        }
            break;
        case ePWR_IO_SetFlagStatus:
        {
#if defined(STM32F427X)
/*
@arg PWR_FLAG_WU: Wake Up flag
@arg PWR_FLAG_SB: StandBy flag
@arg PWR_FLAG_UDRDY: Under-drive ready flag (STM32F42xxx/43xxx devices)
*/
            PWR_ClearFlag(nRparam);
#elif defined(AT32F435VMT7)
            unsigned int unFlags = nRparam;
/*
- PWC_WAKEUP_FLAG                     
- PWC_STANDBY_FLAG
- note:"PWC_PVM_OUTPUT_FLAG" cannot be choose!this bit is readonly bit,it means the voltage monitoring output state
*/
            if      ( unFlags == HAL_PWR_FLAG_WU ) unFlags = PWC_WAKEUP_FLAG;
            else if ( unFlags == HAL_PWR_FLAG_SB ) unFlags = PWC_STANDBY_FLAG;
            else if ( unFlags == HAL_PWR_FLAG_PVDO) unFlags = PWC_PVM_OUTPUT_FLAG;

            pwc_flag_clear(nRparam);
#endif
        }
            break;
        case ePWR_IO_ClearItStatus:
        {
            int nClearBit = nRparam;
#if defined(STM32F427X)
           PWR_ClearFlag(nClearBit);
#elif defined(AT32F435VMT7)
           pwc_flag_clear(nClearBit);
#endif
        }
            break;
        case ePWR_IO_BK_PwAccessEnable:
#if defined(STM32F427X)
            PWR_BackupAccessCmd((FunctionalState)nOverlap);
            PWR_BackupRegulatorCmd((FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            /* allow access to ertc */
            pwc_battery_powered_domain_access((confirm_state)nOverlap);
#endif
            break;
        case ePWR_IO_BK_PwDomain_Reset:
#if defined(STM32F427X)
#elif defined(AT32F435VMT7)
            /* reset ertc bpr domain */
            crm_battery_powered_domain_reset(TRUE);
            crm_battery_powered_domain_reset(FALSE);
#endif
            break;
        case ePWR_IO_WakeupPinEnable:
#if defined(STM32F427X)
            PWR_WakeUpPinCmd((FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            pwc_wakeup_pin_enable(PWC_WAKEUP_PIN_1, (confirm_state)nOverlap);
#endif
           APP_Delay(30); // Wakeup pin 설정후 Sleep 진입시 AT는 35us보장되어야함.
           break;
       case ePWR_IO_PWR_EnterStopMode:
       {
            unsigned char ucEntryMode = (unsigned char)nOverlap;
            unsigned int unPWR_Regulator = nRparam;
#if defined(STM32F427X)
            if      ( unPWR_Regulator == HAL_PWR_Regulator_ON      ) unPWR_Regulator = PWR_Regulator_ON;
            else if ( unPWR_Regulator == HAL_PWR_Regulator_LowPower) unPWR_Regulator = PWR_Regulator_LowPower;
            
			if      ( ucEntryMode == HAL_PWR_STOPEntry_WFI ) ucEntryMode = PWR_STOPEntry_WFI;
            else if ( ucEntryMode == HAL_PWR_STOPEntry_WFE ) ucEntryMode = PWR_STOPEntry_WFE;

            PWR_EnterSTOPMode(unPWR_Regulator, ucEntryMode);
#elif defined(AT32F435VMT7)
            uint32_t systick_index = 0;
            if      ( unPWR_Regulator == HAL_PWR_Regulator_ON      ) unPWR_Regulator = PWC_REGULATOR_ON;
            else if ( unPWR_Regulator == HAL_PWR_Regulator_LowPower) unPWR_Regulator = PWC_REGULATOR_LOW_POWER;
            
            if      ( ucEntryMode == HAL_PWR_STOPEntry_WFI ) ucEntryMode = PWC_DEEP_SLEEP_ENTER_WFI;
            else if ( ucEntryMode == HAL_PWR_STOPEntry_WFE ) ucEntryMode = PWC_DEEP_SLEEP_ENTER_WFE;

            systick_index = SysTick->CTRL;
            systick_index &= ~((uint32_t)0xFFFFFFFE);

            /* disable systick */
            SysTick->CTRL &= (uint32_t)0xFFFFFFFE;
            
            /* select system clock source as hick before ldo set */
            crm_sysclk_switch(CRM_SCLK_HICK);
            /* wait till hick is used as system clock source */
            while(crm_sysclk_switch_status_get() != CRM_SCLK_HICK)
            {
            }

            pwc_ldo_output_voltage_set(PWC_LDO_OUTPUT_1V0);

            /* congfig the voltage regulator mode */
            pwc_voltage_regulate_set((pwc_regulator_type)unPWR_Regulator);

            /* enter sleep mode */
            //pwc_sleep_mode_enter((pwc_sleep_enter_type)ucEntryMode);
            pwc_deep_sleep_mode_enter((pwc_deep_sleep_enter_type)ucEntryMode);

            /* restore systick register configuration */
            SysTick->CTRL |= systick_index;
            
            /* wait clock stable */ 
            for(int delay_index = 0; delay_index < 600; delay_index++)
            {
            __NOP();
            }
            /* resume ldo before system clock source enhance */
            pwc_ldo_output_voltage_set(PWC_LDO_OUTPUT_1V3);
#endif
       }
            break;
       case ePWR_IO_PWR_EnterStandbyMode:
#if defined(STM32F427X)
            PWR_EnterSTANDBYMode();
#elif defined(AT32F435VMT7)
            pwc_standby_mode_enter();
#endif
            break;
       case ePWR_IO_PWR_EnterStandbySystemOnlyMode:
#if defined(STM32F427X)       
            PWR_EnterSTANDBYModeSystemOnly();
#elif defined(AT32F435VMT7)
            pwc_standby_mode_enter();
#endif
       default:
            break;
    }
    

    return HAL_RETURN_SUCCESS;
}

int HalDrvPowerClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

//------------------------------------------------------------------------------
//  void HalDrvPower_SYSCLKConfig_STOP(void)
//  description : restore system clock src(HSE/PLL)
//
//------------------------------------------------------------------------------
void HalDrvPower_SYSCLKConfig_STOP(void)
{
    /* After wake-up from STOP reconfigure the system clock */
    /* Enable HSE */
    
    HalDrvRccIOCtrl(eRCC_IO_SET_HSE_CONFIG, HAL_RCC_HSE_ON, NULL, 0, HAL_ENABLE);

    /* Wait till HSE is ready */
    while (HalDrvRccIOCtrl(eRCC_IO_GET_FLAG_STATUS, HAL_RCC_FLAG_HSERDY, NULL, 0, 0) == HAL_RESET)
    {}

    /* Enable PLL */
    HalDrvRccIOCtrl(eRCC_IO_SET_PLL_ENABLE, 0, NULL, 0, HAL_ENABLE);

    /* Wait till PLL is ready */
    while (HalDrvRccIOCtrl(eRCC_IO_GET_FLAG_STATUS, HAL_RCC_FLAG_PLLRDY, NULL, 0, 0) == HAL_RESET)
    {}

    /* Select PLL as system clock source */
    HalDrvRccIOCtrl(eRCC_IO_SetSourceClock, HAL_RCC_SYSCLKSource_PLLCLK, NULL, 0, 0);
    
    /* Wait till PLL is used as system clock source */
    while (HalDrvRccIOCtrl(eRCC_IO_GetSourceClock, 0, NULL, 0, 0) != HAL_RCC_SYSCLKSource_PLLCLK)
    {}
}

void HalDrvPower_ClearStandbyFlag(void)
{
	eHalFlagStatus eFlagStat;

	/* Allow access to BKP Domain */
	Trace("Allow access to BKP Domain\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);

	/* Clear Wakeup flag */
	Trace("Clear Wakeup flag\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);
    
    eFlagStat = (eHalFlagStatus)HalDrvPowerIOCtrl(ePWR_IO_GetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);
	Trace("PWR_FLAG_WU: %d\r\n", eFlagStat);

	/* Clear StandBy flag */
	Trace("Clear StandBy flag\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);
    
    eFlagStat = (eHalFlagStatus)HalDrvPowerIOCtrl(ePWR_IO_GetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);
	Trace("PWR_FLAG_SB: %d\r\n", eFlagStat);
}

eHalPwr_WakeUpType HalDrvPower_GetStandbyFlag(void)
{
	eHalFlagStatus ePWRFlagWakeup;
	eHalFlagStatus ePWRFlagStandby;

	Trace(" *Check Wakeup, Standby Flag:\n");
    
    g_bFlagWakeup = ePWRFlagWakeup = (eHalFlagStatus)HalDrvPowerIOCtrl(ePWR_IO_GetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);
	Trace(" PWR_FLAG_WU: %d\r\n", ePWRFlagWakeup);

	g_bFlagStandby = ePWRFlagStandby = (eHalFlagStatus)HalDrvPowerIOCtrl(ePWR_IO_GetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);
	Trace(" PWR_FLAG_SB: %d\r\n", ePWRFlagStandby);

    HalDrvPowerIOCtrl(ePWR_IO_ClearItStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);
    HalDrvPowerIOCtrl(ePWR_IO_ClearItStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);

	if(ePWRFlagWakeup == HAL_SET || ePWRFlagStandby == HAL_SET) {
		Trace(" eSYSTEM_RESET_MODE_SOFTWARE\n\n");
		AutoLinkManagerData.eSystemResetMode = eSYSTEM_RESET_MODE_SOFTWARE;
        return HAL_WKUP_WARMBOOT;

	}
	else {
		Trace(" eSYSTEM_RESET_MODE_POWER_ON\n\n");
		AutoLinkManagerData.eSystemResetMode = eSYSTEM_RESET_MODE_POWER_ON;
        return HAL_WKUP_COLDBOOT;
	}
}

void CheckWakeUpSignal()
{
	bool bWakeupFlag;
	
	if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_SOFTWARE) {
		stHalGPIO_InitTypeDef  GPIO_InitStructure;

        HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIO_ALL_GROUP , NULL, 0, HAL_ENABLE);

        GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
		GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
		GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;

		//********************************************************
		// PB1: GPIO_IG_ON_DET
		//********************************************************
		GPIO_InitStructure.GPIO_Pin = GPIO_IG_ON_DET;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_IG_ON] = GetWakeupDetectPinState(eWAKE_PIN_IG_ON);
		//********************************************************

		//********************************************************
		// PB10: GPIO_ACC_DET
		//********************************************************
		GPIO_InitStructure.GPIO_Pin = GPIO_ACC_DET;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_ACC] = GetWakeupDetectPinState(eWAKE_PIN_ACC);
		//********************************************************

		//********************************************************
		// PC3: GPIO_MO_WAKE
		//********************************************************
		GPIO_InitStructure.GPIO_Pin = GPIO_MO_WAKE;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_MODEM] = GetWakeupDetectPinState(eWAKE_PIN_MODEM);
		//********************************************************

		//********************************************************
		// PC5: GPIO_SENSOR_INT
		//********************************************************
		GPIO_InitStructure.GPIO_Pin = GPIO_SENSOR_INT;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_SENSOR] = GetWakeupDetectPinState(eWAKE_PIN_SENSOR);
		//********************************************************

		//********************************************************
		// PB14: GPIO_LOW_HIGH_CAN_RX_MON
		//********************************************************
		GPIO_InitStructure.GPIO_Pin = GPIO_LOW_HIGH_CAN_RX_MON;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_LOW_HIGH_CAN_RX] = GetWakeupDetectPinState(eWAKE_PIN_LOW_HIGH_CAN_RX);
		//********************************************************

		//********************************************************
		// PD10: Bluetooth State : connect or Disconnect
		//********************************************************
		GPIO_InitStructure.GPIO_Pin = GPIO_BT_STATUS;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_BT_MON] = GetWakeupDetectPinState(eWAKE_PIN_BT_MON);
		//********************************************************

		//********************************************************
		// PB15: GPIO_CAN_RX_MON
		//********************************************************
		GPIO_InitStructure.GPIO_Pin = GPIO_CAN_RX_MON;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_CAN_RX] = GetWakeupDetectPinState(eWAKE_PIN_CAN_RX);
		//********************************************************

		Trace("IG_ON_DET       : %s\r\n", (WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_IG_ON] == true) ? "HIGH" : "LOW");
		Trace("ACC_DET        : %s\r\n", (WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_ACC] == true) ? "HIGH" : "LOW");
		Trace("MO_WAKE         : %s\r\n", (WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_MODEM] == true) ? "HIGH" : "LOW");
		Trace("SENSOR_INT      : %s\r\n", (WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_SENSOR] == true) ? "HIGH" : "LOW");
		Trace("LOW_HIGH_CAN_RX: %s\r\n", (WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_LOW_HIGH_CAN_RX] == true) ? "HIGH" : "LOW");
		Trace("BT_MON         : %s\r\n", (WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_BT_MON] == true) ? "HIGH" : "LOW");
		Trace("CAN_RX         : %s\r\n", (WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_CAN_RX] == true) ? "HIGH" : "LOW");

		bWakeupFlag = false;

		if(WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_IG_ON] == true) {
			Trace(" Wakeup Reason - eWAKE_PIN_IG_ON\r\n");
			bWakeupFlag = true;
		}

		if(WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_ACC] == true) {
			Trace(" Wakeup Reason - eWAKE_PIN_ACC\r\n");
			bWakeupFlag = true;
		}

		if(WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_MODEM] == true) {
			Trace(" Wakeup Reason - eWAKE_PIN_MODEM\n");
			bWakeupFlag = true;
		}

		if(WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_SENSOR] == true) {
			Trace(" Wakeup Reason - eWAKE_PIN_SENSOR\n");
			bWakeupFlag = true;
		}

//		if(WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_LOW_HIGH_CAN_RX] == true) {
//			Trace("PWR: Wakeup Reason - eWAKE_PIN_LOW_HIGH_CAN_RX\n");
//			bWakeupFlag = true;
//		}

		if(WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_BT_MON] == true) {
			Trace(" Wakeup Reason - eWAKE_PIN_BT_MON\n");
			bWakeupFlag = true;
		}

//		if(WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_CAN_RX] == true) {
//			Trace(" Wakeup Reason - eWAKE_PIN_CAN_RX\n");
//			bWakeupFlag = true;
//		}

		if(bWakeupFlag == false) {
            // if we need, added
		}

		Trace("\n\n");
	}
	else {
		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_IG_ON] = false;
		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_ACC] = false;
		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_MODEM] = false;
		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_SENSOR] = false;
		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_LOW_HIGH_CAN_RX] = false;
		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_BT_MON] = false;
		WakeupDetectPinState.bAfterWakeupState[eWAKE_PIN_CAN_RX] = false;
	}
}


void HalDrvPower_BKRam_init(void)
{
	HalDrvRccIOCtrl(eRCC_IO_Power_Clock, 0, NULL, 0, HAL_ENABLE);

    HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);
    
	/* Wait until the Backup SRAM low power Regulator is ready */
    while( HalDrvPowerIOCtrl(ePWR_IO_GetFlagStatus, HAL_PWR_FLAG_BRR, NULL, 0, 0) == HAL_RESET ) {
	}
    
	HalDrvRccIOCtrl(eRCC_IO_BKRam_Clock, 0, NULL, 0, HAL_ENABLE);

    HalDrvRtcIOCtrl(eRtc_IO_WaitForSync, 0, NULL, 0, 0);
	
#ifndef ENABLE_STANDBY_MODE	
		if( LoadSystemBackupRam() == false )
		{
			// clear backup memory
			ClearSectionBackupRAM();
		}
		//Standby -> Stop to wakeup state restore
		if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON )
		{
			if( BkSram_SystemInfo.ucForceReset == true )
			{
				AutoLinkManagerData.eSystemResetMode = eSYSTEM_RESET_MODE_SOFTWARE;
			}
		}
#endif

	if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON) {
		HalDrvRdBkRam((unsigned char*)&g_unBkramSwResetSignal, BKRAM_SW_RESET_SIGNAL_ADDR, BKRAM_SW_RESET_SIGNAL_SIZE);
		if( g_unBkramSwResetSignal == SW_RESET_SIGNAL )
		{
			Trace("SW_Reset\r\n");
			AutoLinkManagerData.eSystemResetMode = eSYSTEM_RESET_MODE_SOFTWARE;

			HalDrvRdBkRam((unsigned char*)&g_unBkramClearOdoFlag, BKRAM_CLEARODO_FLAG_ADDR, BKRAM_CLEARODO_FLAG_SIZE);
			if( g_unBkramClearOdoFlag != ODO_CLEAR_SIGNAL )
			{
				g_unBkramTmpOdometer = 0;
				HalDrvWrBkRam((unsigned char*)&g_unBkramTmpOdometer,BKRAM_ODOMETER_ADDR,BKRAM_ODOMETER_SIZE);	//백업램 ODO 초기화 - 파워온시에는 백업램 전체 초기화함
			}
		}
		else
		{
			ClearSectionBackupRAM();
		}
	}	
	g_unBkramSwResetSignal = 0;
	HalDrvWrBkRam((unsigned char*)&g_unBkramSwResetSignal, BKRAM_SW_RESET_SIGNAL_ADDR, BKRAM_SW_RESET_SIGNAL_SIZE);
}

unsigned short HalDrvRdBkRam(unsigned char *pucBuf, unsigned short usIndex, unsigned short usLen)
{
    // BKram영역을 @"BKSRAM"; 영역에서 동작으로 함수 구현 필요 하지 않음.
    // STM과 Artery MCU에서는 Stop모드여서 동일하게 동작하면 된다.
#if defined(ENABLE_STANDBY_MODE) && defined(STM32F427X)
	u16 i;

	if ((usIndex > BKRAM_MAX)||(usLen==0))
		return 0;

	for(i=0; i<usLen;i++){ 	   
		pucBuf[i]= *(__IO uint8_t *)(BKPSRAM_BASE+usIndex+i);
		if ((usIndex+i)==BKRAM_MAX)
		   break;
	}   

	return i+1;
#else
    return HAL_RETURN_SUCCESS;
#endif
}    

unsigned short HalDrvWrBkRam(unsigned char *pucBuf, unsigned short usIndex, unsigned short usLen)
{
    // BKram영역을 @"BKSRAM"; 영역에서 동작으로 함수 구현 필요 하지 않음.

    // STM 기준 stop mode, Artery 기준 Deepsleep모드에서 동일하게 동작함.


#if defined(ENABLE_STANDBY_MODE) && defined(STM32F427X)
    //------------------------------------------------------------------------------
    //  unsigned short HalDrvWrBkRam(u8 *buf,u16 index,u16 len)
    //      description : read backup-domain-ram( ram size-> 4Kbyte)  
    //                    
    //      param1(buf) : store buffer
    //      param2(index): backup-domain-ram index ( 0~4095 )
    //      param3(len): read buffer size
    //      return  : bytes num of reading
    //------------------------------------------------------------------------------
	u16 i;
    
    if ((usIndex > BKRAM_MAX)||(usLen==0))
        return 0;
    
    for(i=0; i<usLen;i++){        
        *(__IO uint8_t *)(BKPSRAM_BASE+usIndex+i)=pucBuf[i]; //BKPSRAM_BASE : 0x40000000+0x00020000+0x4000 = 0x40024000
        if ((usIndex+i)==BKRAM_MAX)
            break;
    }
    
    return i+1;
#else
	return HAL_RETURN_SUCCESS;
#endif
} 

