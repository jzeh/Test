/**
  ******************************************************************************
  * @file    Autolink_Manager.h
  * @author  GIT Application Team by james jean
  * @version V1.1.0
  * @date    17-MAR-2014
  * @brief   Header for TestMainManager.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AUTOLINK_MANAGER_H__
#define __AUTOLINK_MANAGER_H__

#include "common.h"
#include "Message_Manager.h"
#include "HalHandler.h"


#define SYSTEM_BACKUP_RAM_LENGTH				0x1000

typedef enum _eAUTOLINK_STATE
{
	eAUTOLINK_STATE_INIT = 0,
	eAUTOLINK_STATE_START_CAN,
	eAUTOLINK_STATE_READY,
	eAUTOLINK_STATE_RUN,
	eAUTOLINK_STATE_IDLE,
} eAUTOLINK_STATE;

typedef enum _eUSB_DEVICE_CLASS
{
	eUSB_DEVICE_CLASS_BULK = 0,
	eUSB_DEVICE_CLASS_CDC,
	eUSB_DEVICE_CLASS_MSD,
} eUSB_DEVICE_CLASS;

typedef enum _eMODEM_UART_FLOWCONTROL_TYPE
{
	eMODEM_UART_FLOWCONTROL_TYPE_CTS = 0,
	eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS,
	eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS_GPIO,
	eMODEM_UART_FLOWCONTROL_TYPE_NONE,
} eMODEM_UART_FLOWCONTROL_TYPE;

typedef enum _eDEBUG_UART_CH
{
	eDEBUG_UART_CH_UART8 = 0,
	eDEBUG_UART_CH_UART7,
} eDEBUG_UART_CH;

typedef enum _eHIGH_CAN_SEL
{
	eHIGH_CAN_SEL_CH1 = 0,
	eHIGH_CAN_SEL_CH2,
} eHIGH_CAN_SEL;

typedef enum _eSYSTEM_RESET_MODE
{
	eSYSTEM_RESET_MODE_POWER_ON = 0,
	eSYSTEM_RESET_MODE_SOFTWARE,
} eSYSTEM_RESET_MODE;

typedef enum _eWAKE_PINS
{
	eWAKE_PIN_IG_ON = 0,
	eWAKE_PIN_ACC,
	eWAKE_PIN_MODEM,
	eWAKE_PIN_SENSOR,
	eWAKE_PIN_LOW_HIGH_CAN_RX,
	eWAKE_PIN_BT_MON,
	eWAKE_PIN_CAN_RX,

	eWAKE_PIN_MAX,
} eWAKE_PINS;

typedef enum _eCTL_WAKE_DET_PINS
{
	eCTL_WAKE_DET_PIN_IG_ACC = 0,
	eCTL_WAKE_DET_PIN_MO_WAKE,
	eCTL_WAKE_DET_PIN_SENSOR,
	eCTL_WAKE_DET_PIN_LOW_CAN_RX,
	eCTL_WAKE_DET_PIN_BT_MON,
	eCTL_WAKE_DET_PIN_CAN_RX,
} eCTL_WAKE_DET_PINS;

typedef enum _eWAKE_CTL_PIN
{
	eWAKE_CTL_PIN_ENABLE = 0,
	eWAKE_CTL_PIN_DISABLE,
} eWAKE_CTL_PIN;

//**********************************************************************************************
typedef enum _eTEST_STATES
{
	eTEST_STATE_1 = 0,
	eTEST_STATE_2,
	eTEST_STATE_3,
	eTEST_STATE_4,
	eTEST_STATE_5,
	eTEST_STATE_6,
	eTEST_STATE_7,
	eTEST_STATE_8,
	eTEST_STATE_9,
	eTEST_STATE_10,
	eTEST_STATE_11,
	eTEST_STATE_12,
	eTEST_STATE_13,
	eTEST_STATE_14,
	eTEST_STATE_START_LOOP,
	eTEST_STATE_STOP_LOOP,
	eTEST_STATE_WAIT_RESPONSE,
	eTEST_STATE_WAIT_RESPONSE2,
	eTEST_STATE_WAIT_RESPONSE3,
	eTEST_STATE_WAIT_RESPONSE4,
	eTEST_STATE_WAIT_RESPONSE5,
	eTEST_STATE_WAIT_RESPONSE6,
	eTEST_STATE_WAIT_RESPONSE7,
	eTEST_STATE_IDLE,

	eTEST_STATE_20,
} eTEST_STATES;

typedef enum _eCAN_PARSING_TYPE
{
	eCAN_PARSING_TYPE_CAN1 = 0,
	eCAN_PARSING_TYPE_CAN2,
	eCAN_PARSING_TYPE_ALL,
} eCAN_PARSING_TYPE;

typedef __packed struct {
	eTEST_STATES eState;
	eTEST_STATES eNextState;
	boolean_t bEnableTestMode;
	boolean_t bEnableLoopTestMode;
	int32_t iTimerTestDly;
	int32_t iTimerTest2Dly;
	uint16_t nTestCount;
} TEST_DATA;
//**********************************************************************************************

typedef __packed struct {
	boolean_t bBeforeWakeupState[eWAKE_PIN_MAX];
	boolean_t bAfterWakeupState[eWAKE_PIN_MAX];
} WAKEUP_DETECT_PIN_STATE;

#pragma pack(push, 1)
typedef __packed struct {
	uint16_t nOption1;
	uint16_t nOption2;
	uint16_t nOption3;
	uint16_t nOption4;
	uint16_t nOption5;
	uint16_t nOption6;
	uint16_t nOption7;
	uint16_t nOption8;
	uint16_t nOption9;
	uint16_t nOption10;
} SYSTEM_BACKUP_RAM_INFO;

typedef __packed struct {
	eMODEM_UART_FLOWCONTROL_TYPE eModemFlowControlType;
	eUSB_DEVICE_CLASS eUSBDeviceClassType;
	eDEBUG_UART_CH eDebugUartCh;
	boolean_t bRunTestMode;
	uint32_t wModemBaudrate;
	uint16_t nOption6;
	uint16_t nOption7;
	uint16_t nOption8;
	uint16_t nOption9;
	uint16_t nOption10;
} SYSTEM_TEST_INFO;
#pragma pack(pop, 1)

#pragma pack(push, 1)
typedef __packed struct {
	eAUTOLINK_STATE eState;
	int32_t iTimerAppDly;
	uint32_t wSFlashTotalSize;
	uint32_t wSFlashFreeSize;
	eHIGH_CAN_SEL bCurrCANSel;

	uint8_t bTestMode;
	boolean_t bTestResult1;
	boolean_t bTestResult2;
	eSYSTEM_RESET_MODE eSystemResetMode;
} AUTOLINK_MANAGER_DATA;
#pragma pack(pop, 1)

extern AUTOLINK_MANAGER_DATA AutoLinkManagerData;
extern bool g_bYUJINSelftestFlag;
extern bool g_bHYPERTECSelftestFlag;
extern bool g_bUSIMInsertFlag;
extern SYSTEM_TEST_INFO stSystemTestInfo;
extern volatile uint16_t gnTestState_SFlash;
extern TEST_DATA TestData;

extern WAKEUP_DETECT_PIN_STATE WakeupDetectPinState;

void AutoLink_MainProc(void);
void TestSetSerialAndIpektoInternaFlash(uint8_t);
void Can_Parsing_Manager(eCAN_PARSING_TYPE);
int ReportAlramState(eMESSAGE_EVENT_KEY eEVENT, char* iValue, int nSize);

void ClearSectionBackupRAM(void);

void SetServerUrl();
bool CheckDevServerUrl(); // 개발 서버 url이면 return true;

#endif /* __AUTOLINK_MANAGER_H__ */

/***************************** END OF FILE ****/

