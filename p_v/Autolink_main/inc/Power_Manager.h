/**
  ******************************************************************************
  * @file    Power_Manager.h
  * @author  GIT Application Team by james jean
  * @version V1.1.0
  * @date    17-MAR-2014
  * @brief   Header for TestMainManager.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __POWER_MANAGER_H__
#define __POWER_MANAGER_H__


#include "HalHandler.h"
#include "Autolink_Manager.h"


#define SLEEP_READY_OBD_MANAGER				0x0001
#define SLEEP_READY_MESSAGE_MANAGER		0x0002
#define SLEEP_READY_MODEM_MANAGER			0x0004


typedef union {
	struct {
		unsigned short bSleepReady_OBDManager         :1;
		unsigned short bSleepReady_MessageManager     :1;
		unsigned short bSleepReady_ModemManager       :1;
		unsigned short bFlag3                   :1;
		unsigned short bFlag4                   :1;
		unsigned short bFlag5                   :1;
		unsigned short bFlag6                   :1;
		unsigned short bFlag7                   :1;
		unsigned short bFlag8                   :1;
		unsigned short bFlag9                   :1;
		unsigned short bFlagA                   :1;
		unsigned short bFlagB                   :1;
		unsigned short bFlagC                   :1;
		unsigned short bFlagD                   :1;
		unsigned short bFlagE                   :1;
		unsigned short bFlagF                   :1;
		//unsigned short reserved :16;
	};

	unsigned short nSleepReady_AllManager;			/* 2 Byte */
} PowerFlagBits_t;

typedef enum _ePOWER_STATE
{
	ePOWER_NONE,
	ePOWER_Initialize,
	ePOWER_Wakeup,
	ePOWER_Running,
	ePOWER_Sleep_Ready,
	ePOWER_Sleep,
	ePOWER_Sleep_Complete,
	ePOWER_inform_FOTA,
	ePOWER_Wait_FOTA_Complete,

	ePOWER_Error,
	ePOWER_State_Max,
} ePOWER_STATE;

typedef struct {
	stHalRTC_TimeTypeDef RTCTime;
	stHalRTC_DateTypeDef RTCDate;
} RTC_TIME_AND_DATE;

extern PowerFlagBits_t g_PowerFlagBits;
extern ePOWER_STATE g_ePowerManageState;
extern RTC_TIME_AND_DATE g_stSavedWakeupAlarmTime;

extern unsigned int g_uiWakeupPinToggleCnt;
extern unsigned int g_uiSensorWakeupPinToggleCnt;

void Power_Manager(void);


/* Only use test Mode and emergency. */
void SetPOWERState(ePOWER_STATE state);

void InitializePowerManager(void);
ePOWER_STATE GetPOWERState(void);

bool GetWakeupDetectPinState(eWAKE_PINS ePin);
bool SetCtlWakeDetectPinState(eCTL_WAKE_DET_PINS ePin, eWAKE_CTL_PIN eCtl);
bool GetWakeupPinState(void);
bool GetWakeupPA0PinState(void);
bool GetWakeupDetectPinsState(void);

void EnterStandbyMode(void);

#endif /* __POWER_MANAGER_H__ */

/***************************** END OF FILE ****/
