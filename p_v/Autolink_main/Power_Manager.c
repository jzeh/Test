/**
  ******************************************************************************
  * @file    Power_Manager.c
  * @author  GIT Firmware group by james jean
  * @version V1.1.0
  * @date    19-MAR-2014
  * @brief   Manager DLoggerManager.c module
  ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include "HalHandler.h"

#include "cli.h"
//#include "Autolink_Manager.h"
#include "Power_Manager.h"
#include "FOTA_Manager.h"
#include "GIT_CanParsingProc.h"
#include "OBD_Manager.h"
#include "HdDebug.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_POWER,__VA_ARGS__)

/* Define ------------------------------------------------------------------*/

/* Variable ------------------------------------------------------------------*/
ePOWER_STATE g_ePowerManageState;
PowerFlagBits_t g_PowerFlagBits;

#pragma section="BKSRAM"
RTC_TIME_AND_DATE g_stSavedWakeupAlarmTime @"BKSRAM";

unsigned int g_uiWakeupPinToggleCnt;
unsigned int g_uiSensorWakeupPinToggleCnt;
/* Function ------------------------------------------------------------------*/
void EnterStandbyAndRTCAlarmWakeup(uint32_t wSec);
bool MPU6515_CheckInterrupt();

/*----------------------------------------------------------------------------*/

void InitializePowerManager(void)
{
	g_ePowerManageState = ePOWER_NONE;

	g_PowerFlagBits.bSleepReady_OBDManager = false;
	g_PowerFlagBits.bSleepReady_MessageManager = false;
	g_PowerFlagBits.bSleepReady_ModemManager = false;
	g_PowerFlagBits.bFlag3 = false;
	g_PowerFlagBits.bFlag4 = false;
	g_PowerFlagBits.bFlag5 = false;
	g_PowerFlagBits.bFlag6 = false;
	g_PowerFlagBits.bFlag7 = false;
	g_PowerFlagBits.bFlag8 = false;
	g_PowerFlagBits.bFlag9 = false;
	g_PowerFlagBits.bFlagA = false;
	g_PowerFlagBits.bFlagB = false;
	g_PowerFlagBits.bFlagC = false;
	g_PowerFlagBits.bFlagD = false;
	g_PowerFlagBits.bFlagE = false;
	g_PowerFlagBits.bFlagF = false;

	g_PowerFlagBits.nSleepReady_AllManager = false;

	return;
}

ePOWER_STATE GetPOWERState(void)
{
	return g_ePowerManageState;
}

/* Only use test Mode and emergency. */
void SetPOWERState(ePOWER_STATE state)
{
	g_ePowerManageState = state;
}

void Power_Manager(void)
{
	switch(g_ePowerManageState) {
		case ePOWER_NONE:
			if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON) {
				Trace("ePOWER_Initialize\r\n");
				g_ePowerManageState = ePOWER_Initialize;
			}
			else {
				Trace("ePOWER_Wakeup\r\n");
				g_ePowerManageState = ePOWER_Wakeup;
			}
			break;

		case ePOWER_Initialize:
			Trace("clear BKRAM...\r\n");
			//ClearSectionBackupRAM();
			//SetOBDState(eOBD_Running_Info_Mode);
			//SetOBDState(eOBD_NONE);
			SetOBDState(eOBD_NONE);

			g_ePowerManageState = ePOWER_Running;
			break;

		case ePOWER_Wakeup:
			SetOBDState(eOBD_NONE);
			g_ePowerManageState = ePOWER_Running;
			break;

		case ePOWER_Running:
			if((g_PowerFlagBits.nSleepReady_AllManager && (SLEEP_READY_OBD_MANAGER | SLEEP_READY_MESSAGE_MANAGER | SLEEP_READY_MODEM_MANAGER)) == (SLEEP_READY_OBD_MANAGER | SLEEP_READY_MESSAGE_MANAGER | SLEEP_READY_MODEM_MANAGER)) {
				g_ePowerManageState = ePOWER_Sleep_Ready;
			}
			break;

		case ePOWER_Sleep_Ready:
			g_ePowerManageState = ePOWER_Sleep;
			break;

		case ePOWER_Wait_FOTA_Complete:
			break;

		case ePOWER_Sleep:
            //MONI 2018-02-16
            // block all sleep process
//#if 1
//			EnterStandbyMode();
//#else
//			/* Sleep and wakeup after 30 sec. */
//			EnterStandbyAndRTCAlarmWakeup();
//#endif
			break;

		case ePOWER_Error:
			Trace("\r\n Error Power_Manager()  \r\n");
			break;
	}
}

void EnterStandbyMode(void)
{
    HalGPIOSetVaule(GPIO_MCU_LNA_EN, eBIT_RESET);   

    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart2.pUARTreg, 0, NULL, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart3.pUARTreg, 0, NULL, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart4.pUARTreg, 0, NULL, HAL_DISABLE);

    HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, 0, NULL, 0);
    HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart3.pUARTreg, NULL, 0, 0);
    HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart4.pUARTreg, NULL, 0, 0);

#if 0//ndef RF_COMMON_MODEM
    stHalGPIO_InitTypeDef  GPIO_InitStructure;
    GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_Pin =  UART2_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
    
    GPIO_InitStructure.GPIO_Pin =  UART2_TX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    HalGPIOSetVaule(UART2_TX_PIN, eBIT_RESET);
    HalGPIOSetVaule(UART2_RX_PIN, eBIT_RESET);
#endif
    APP_Delay(1);

    SetLedOnOffCtl(LED_OFF, eLED_GPS);
    SetLedOnOffCtl(LED_OFF, eLED_SERVER);
    SetLedOnOffCtl(LED_OFF, eLED_CAN);
#define USE_DEVICE_ONLY
#ifdef USE_DEVICE_ONLY
    Can_DeInit_Sleep();	//슬립시 CAN2 Rx를 Low시켜주지않으면 웨이크업 신호가 계속 High라 엣지발생안함
	//CAN으로 웨이크업하고 싶으면 래치외에도 해당 라인 세팅하고 슬립들어갈것!!!!!!!!! ex)CAN1으로 깨고싶으면	DefaultAllCanMaskSet();
#endif

    // OUTPUT - 3.3V 전원인가 용도(HIGH)
    HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_RESET);

    SetModemState(eMODEM_NONE);

    //insem 20190114 request from insem for test
    HalDrvRtcIOCtrl(eRtc_IO_ClearFlagStatus, HAL_RTC_FLAG_ALRAF, NULL, 0, 0);

    /* Allow access to BKP Domain */
    Trace("Allow access to BKP Domain\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);

    /* Clear Wakeup flag */
    Trace("Clear Wakeup flag\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);

    /* Clear StandBy flag */
    Trace("Clear StandBy flag\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);

    /* Enable WKUP pin  */
    Trace("Enable WKUP pin\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_WakeupPinEnable, 0, NULL, 0, HAL_ENABLE);

    printf("ePWR_IO_PWR_EnterStandbyMode()\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStandbyMode, 0, NULL, 0, 0);
}

void EnterStandbyModeforSystemOnly(void)
{
    SetModemState(eMODEM_NONE);

    //insem 20190114 request from insem for test
    HalDrvRtcIOCtrl(eRtc_IO_ClearFlagStatus, HAL_RTC_FLAG_ALRAF, NULL, 0, 0);

    /* Allow access to BKP Domain */
    Trace("Allow access to BKP Domain\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);

    /* Clear Wakeup flag */
    Trace("Clear Wakeup flag\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);

    /* Clear StandBy flag */
    Trace("Clear StandBy flag\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);
}

void EnterStandbyAndRTCAlarmWakeup(uint32_t wSec)
{
	uint32_t wTime;
	uint32_t wTemp;
	bool bRet;
    stHalRTCTypeDef stRtcDateTime;

	Trace("\r\n\n\nCurrent Time & Date!!!\r\n");
    APP_TimeShow(&stRtcDateTime);

	GetWakeupDetectPinsState();
	bRet = GetWakeupPinState();
	if(bRet == true) {
		Trace("Not ready enter standby mode!!!\r\n\r\n");

		if(WakeupDetectPinState.bBeforeWakeupState[eWAKE_PIN_MODEM] == true) {
//#warning "APP_Delay() 함수 삭제 할 수 있는 방법 찾을 것!!"
			while(1) {
				APP_Delay(500);

				GetWakeupDetectPinsState();
				bRet = GetWakeupPinState();
				if(bRet == true) {
					Trace("H");
				}
				else {
					break;
				}
			}
		}
		else {
			return;
		}
	}

	wTime = stRtcDateTime.RtcTime.RTC_Hours * 60 * 60;
	wTime += (stRtcDateTime.RtcTime.RTC_Minutes * 60);
	wTime += stRtcDateTime.RtcTime.RTC_Seconds;

	Trace("period time: %d(sec)\r\n", wSec);

	wTime += wSec;

	wTemp = wTime / 3600;
	stRtcDateTime.RtcTime.RTC_Hours = wTemp % 24;
	wTime = wTime % 3600;
	stRtcDateTime.RtcTime.RTC_Minutes = wTime / 60;
	stRtcDateTime.RtcTime.RTC_Seconds = wTime % 60;

	Trace("alarm Time: %02d:%02d:%02d\r\n", stRtcDateTime.RtcTime.RTC_Hours, stRtcDateTime.RtcTime.RTC_Minutes, stRtcDateTime.RtcTime.RTC_Seconds);
	g_stSavedWakeupAlarmTime.RTCTime.RTC_Hours = stRtcDateTime.RtcTime.RTC_Hours;
	g_stSavedWakeupAlarmTime.RTCTime.RTC_Minutes = stRtcDateTime.RtcTime.RTC_Minutes;
	g_stSavedWakeupAlarmTime.RTCTime.RTC_Seconds = stRtcDateTime.RtcTime.RTC_Seconds;

	HalDrvRtc_SetAlarmTime(&stRtcDateTime);

	/* Allow access to BKP Domain */
	Trace("Allow access to BKP Domain\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);

	/* Clear Wakeup flag */
	Trace("Clear Wakeup flag\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);

	/* Clear StandBy flag */
	Trace("Clear StandBy flag\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);

    /* Enable WKUP pin  */
    Trace("Enable WKUP pin\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_WakeupPinEnable, 0, NULL, 0, HAL_ENABLE);

	Trace("ePWR_IO_PWR_EnterStandbyMode()\r\n\r\n\r\n");
    HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStandbyMode, 0, NULL, 0, 0);

	while(1);
}

bool SetCtlWakeDetectPinState(eCTL_WAKE_DET_PINS ePin, eWAKE_CTL_PIN eCtl)
{
	switch(ePin) {
		case eCTL_WAKE_DET_PIN_IG_ACC:
			if(eCtl == eWAKE_CTL_PIN_ENABLE) {
				Trace("GPIO_IG_ACC_DET_CTL: ENABLE\r\n");
				HalGPIOSetVaule(GPIO_IG_ACC_DET_CTL, eBIT_RESET);
			}
			else {
				Trace("GPIO_IG_ACC_DET_CTL: DISABLE\r\n");
				HalGPIOSetVaule(GPIO_IG_ACC_DET_CTL, eBIT_SET);
			}
			break;

		case eCTL_WAKE_DET_PIN_MO_WAKE:
			if(eCtl == eWAKE_CTL_PIN_ENABLE) {
				Trace("GPIO_MO_WAKE_CTL: ENABLE\r\n");
				HalGPIOSetVaule(GPIO_MO_WAKE_CTL, eBIT_RESET);
			}
			else {
				Trace("GPIO_MO_WAKE_CTL: DISABLE\r\n");
				HalGPIOSetVaule(GPIO_MO_WAKE_CTL, eBIT_SET);
			}
			break;

		case eCTL_WAKE_DET_PIN_SENSOR:
			if(eCtl == eWAKE_CTL_PIN_ENABLE) {
				Trace("GPIO_SENSOR_INT: ENABLE\r\n");
				HalGPIOSetVaule(GPIO_SENSOR_INT, eBIT_RESET);
			}
			else {
				Trace("GPIO_SENSOR_INT: DISABLE\r\n");
				HalGPIOSetVaule(GPIO_SENSOR_INT, eBIT_SET);
			}
			break;

		case eCTL_WAKE_DET_PIN_LOW_CAN_RX:
			if(eCtl == eWAKE_CTL_PIN_ENABLE) {
				Trace("GPIO_LOW_CAN_RX_CTL: ENABLE\r\n");
				HalGPIOSetVaule(GPIO_LOW_CAN_RX_CTL, eBIT_RESET);
			}
			else {
				Trace("GPIO_LOW_CAN_RX_CTL: DISABLE\r\n");
				HalGPIOSetVaule(GPIO_LOW_CAN_RX_CTL, eBIT_SET);
			}
			break;

		case eCTL_WAKE_DET_PIN_BT_MON:
			if(eCtl == eWAKE_CTL_PIN_ENABLE) {
				Trace("GPIO_BT_MON_CTL: ENABLE\r\n");
				HalGPIOSetVaule(GPIO_BT_MON_CTL, eBIT_RESET);
			}
			else {
				Trace("GPIO_BT_MON_CTL: DISABLE\r\n");
				HalGPIOSetVaule(GPIO_BT_MON_CTL, eBIT_SET);
			}
			break;

		case eCTL_WAKE_DET_PIN_CAN_RX:
			if(eCtl == eWAKE_CTL_PIN_ENABLE) {
				Trace("GPIO_CAN_RX_CTL: ENABLE\r\n");
				HalGPIOSetVaule(GPIO_CAN_RX_CTL, eBIT_RESET);
			}
			else {
				Trace("GPIO_CAN_RX_CTL: DISABLE\r\n");
				HalGPIOSetVaule(GPIO_CAN_RX_CTL, eBIT_SET);
			}
			break;

		default:
			return false;
	}

	HalGpioGenerateLatchClock();

	return true;
}

bool GetWakeupPinState(void)
{
	stHalGPIO_InitTypeDef  GPIO_InitStructure;

	 HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOA_GROUP, NULL, 0, HAL_ENABLE);
	//********************************************************
	// PA0: WAKE_UP
	//********************************************************
    GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Pin = GPIO_WAKEUP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
	//********************************************************

	if(HalGPIOGetStatus(GPIO_WAKEUP) == SET) {
		Trace("WAKEUP pin state     : HIGH\r\n\r\n");
		return true;
	}
	else {
		Trace("WAKEUP pin state     : LOW\r\n\r\n");
		return false;
	}
}
bool GetWakeupPA0PinState(void)
{
	if(HalGPIOGetStatus(GPIO_WAKEUP) == SET) {
		return true;
	}
	else {
		return false;
	}
}

bool GetWakeupDetectPinState(eWAKE_PINS ePin)
{
	switch(ePin) {
		case eWAKE_PIN_IG_ON:
			if(HalGPIOGetStatus(GPIO_IG_ON_DET) == SET) {
				return true;
			}
			else {
				return false;
			}
			break;

		case eWAKE_PIN_ACC:
			if(HalGPIOGetStatus(GPIO_ACC_DET) == SET) {
				return true;
			}
			else {
				return false;
			}
			break;

		case eWAKE_PIN_MODEM:
			if(HalGPIOGetStatus(GPIO_MO_WAKE) == SET) {
				return true;
			}
			else {
				return false;
			}
			break;

		case eWAKE_PIN_SENSOR:
			if(HalGPIOGetStatus(GPIO_SENSOR_INT) == SET) {
				return true;
			}
			else {
				return false;
			}
			break;

		case eWAKE_PIN_LOW_HIGH_CAN_RX:
			if(HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_RX_MON) == SET) {
				return true;
			}
			else {
				return false;
			}
			break;

		case eWAKE_PIN_BT_MON:
			if(HalGPIOGetStatus(GPIO_BT_STATUS) == SET) {
				return true;
			}
			else {
				return false;
			}
			break;

		case eWAKE_PIN_CAN_RX:
			if(HalGPIOGetStatus(GPIO_CAN_RX_MON) == SET) {
				return true;
			}
			else {
				return false;
			}
			break;

		default:
			return false;
	}
}

bool GetWakeupDetectPinsState(void)
{
	bool bRet;

	Trace("wakeup detect pin state:\r\n");

	bRet = GetWakeupDetectPinState(eWAKE_PIN_IG_ON);
	WakeupDetectPinState.bBeforeWakeupState[eWAKE_PIN_IG_ON] = bRet;
	Trace("IG_ON_DET       : %s\r\n", (bRet == true) ? "HIGH" : "LOW");

	bRet = GetWakeupDetectPinState(eWAKE_PIN_ACC);
	WakeupDetectPinState.bBeforeWakeupState[eWAKE_PIN_ACC] = bRet;
	Trace("ACC_DET        : %s\r\n", (bRet == true) ? "HIGH" : "LOW");

	bRet = GetWakeupDetectPinState(eWAKE_PIN_MODEM);
	WakeupDetectPinState.bBeforeWakeupState[eWAKE_PIN_MODEM] = bRet;
	Trace("MO_WAKE         : %s\r\n", (bRet == true) ? "HIGH" : "LOW");

	bRet = GetWakeupDetectPinState(eWAKE_PIN_SENSOR);
	WakeupDetectPinState.bBeforeWakeupState[eWAKE_PIN_SENSOR] = bRet;
	Trace("SENSOR_INT      : %s\r\n", (bRet == true) ? "HIGH" : "LOW");

	bRet = GetWakeupDetectPinState(eWAKE_PIN_LOW_HIGH_CAN_RX);
	WakeupDetectPinState.bBeforeWakeupState[eWAKE_PIN_LOW_HIGH_CAN_RX] = bRet;
	Trace("LOW_HIGH_CAN_RX: %s\r\n", (bRet == true) ? "HIGH" : "LOW");

	bRet = GetWakeupDetectPinState(eWAKE_PIN_BT_MON);
	WakeupDetectPinState.bBeforeWakeupState[eWAKE_PIN_BT_MON] = bRet;
	Trace("BT_MON         : %s\r\n", (bRet == true) ? "HIGH" : "LOW");

	bRet = GetWakeupDetectPinState(eWAKE_PIN_CAN_RX);
	WakeupDetectPinState.bBeforeWakeupState[eWAKE_PIN_CAN_RX] = bRet;
	Trace("CAN_RX         : %s\n\r\n", (bRet == true) ? "HIGH" : "LOW");

	return true;
}

/*****************************END OF FILE****/
