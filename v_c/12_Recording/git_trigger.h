/*----------------------------------------------------------------------
 *   trigger control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_TRIGGER_H__
#define	__GIT_TRIGGER_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "typedef.h"
//MONI #include "PassThruStruct.h"

#include "git_record.h"
#include "cmsis_os.h"
/*----------------------------------------------------------------------*/

#define SIGNAL_TRIGGER_ALIVE_RESPONSE			(int)(0x01)
#define SIGNAL_TRIGGER_BTN_EVENT				(int)(0x02)
#define SIGNAL_TRIGGER_FREEZE_RESPONSE			(int)(0x03)




// related ecu request code
#define MAX_TRIP_TIME_LENGTH (10)
#define MAX_TRIGGER_DTC_LENGTH (40)

typedef enum _eTriggerModuleEvent
{
	eEVT_TM_CTRL_LED_OBD_CCP,					// 0xD001
	eEVT_TM_CTRL_REC_BTN_STATUS, 				// 0xD002
	eEVT_TM_CTRL_SLEEP,							// 0xD003
	eEVT_TM_CTRL_VER_INFO,						// 0xD004
	eEVT_TM_CTRL_FW_UPDATE_START,				// 0xD005
	eEVT_TM_CTRL_FW_UPDATE_WRITE,				// 0xD006
	eEVT_TM_CTRL_FW_CHECKSUM,					// 0xD007
	eEVT_TM_CTRL_FW_UPDATE_CLOSE,				// 0xD008
	eEVT_TM_CTRL_SW_APP_LIST_RCV,				// 0xD009
	eEVT_TM_CTRL_SW_APP_LIST_SEND,				// 0xD00A
	eEVT_TM_CTRL_LED_GREEN,						// 0xD00B
	eEVT_TM_CTRL_CUR_MODE_INFO,					// 0xD00C
	eEVT_TM_CTRL_FW_MODE_CHANGE,				// 0xD00D
	eEVT_TM_CTRL_TRIGGER,						// 0xD00E
	eEVT_TM_CTRL_SET_SERIAL_NUMBER,				// 0xD02F
	eEVT_TM_CTRL_GET_SERIAL_NUMBER,				// 0xD030
	eEVT_TM_CTRL_SELFTEST_LED_BLINK_DELAY_TIME,	// 0xD03E
	eEVT_TM_CTRL_SELFTEST_START,				// 0xD03F
}eTriggerModuleEvent;

typedef enum _eEVT_TM_CTRL_LED_OBD_VALUE{
	eTM_LED_OBD_OFF=0,
	eTM_LED_OBD_ON=1,	
}eEVT_TM_CTRL_LED_OBD_VALUE;

typedef enum _eEVT_TM_CTRL_LED_CCP_VALUE{
	eTM_LED_CCP_OFF=0,
	eTM_LED_CCP_ON=1,	
}eEVT_TM_CTRL_LED_CCP_VALUE;

typedef enum _eEVT_TM_CTRL_REC_BTN_STATUS_VALUE{
	eTM_ACK=0,
	eTM_NACK=1,	
}eEVT_TM_CTRL_REC_BTN_STATUS_VALUE;

typedef enum _eEVT_TM_CTRL_SLEEP_VALUE{
	eTM_SLEEE_INACTIVE = 0,
	eTM_SLEEE_ACTIVE = 1,
}eEVT_TM_CTRL_SLEEP_VALUE;

#if false
typedef enum _eEVT_TM_CTRL_VER_INFO_VALUE{
}eEVT_TM_CTRL_VER_INFO_VALUE;

typedef enum _eEVT_TM_CTRL_FW_UPDATE_START_VALUE{
}eEVT_TM_CTRL_FW_UPDATE_START_VALUE;

typedef enum _eEVT_TM_CTRL_FW_UPDATE_WRITE_VALUE{
}eEVT_TM_CTRL_FW_UPDATE_WRITE_VALUE;

typedef enum _eEVT_TM_CTRL_FW_CHECKSUM_VALUE{
}eEVT_TM_CTRL_FW_CHECKSUM_VALUE;

typedef enum _eEVT_TM_CTRL_FW_UPDATE_CLOSE_VALUE{
}eEVT_TM_CTRL_FW_UPDATE_CLOSE_VALUE;

typedef enum _eEVT_TM_CTRL_SW_APP_LIST_RCV_VALUE{
}eEVT_TM_CTRL_SW_APP_LIST_RCV_VALUE;

typedef enum _eEVT_TM_CTRL_SW_APP_LIST_SEND_VALUE{
}eEVT_TM_CTRL_SW_APP_LIST_SEND_VALUE;
#endif

typedef enum _eEVT_TM_CTRL_LED_GREEN_VALUE{
	eTM_LED_GREEN_OFF = 0,
	eTM_LED_GREEN_BLINK = 1,
	eTM_LED_GREEN_ON = 2,
}eEVT_TM_CTRL_LED_GREEN_VALUE;

#if false
typedef enum _eEVT_TM_CTRL_CUR_MODE_INFO_VALUE{
}eEVT_TM_CTRL_CUR_MODE_INFO_VALUE;

#endif

typedef enum _eEVT_TM_CTRL_FW_MODE_CHANGE_VALUE{
	eTM_FW_MODE_BOOTLOADER = 6,
	eTM_FW_MODE_DOWNLAOD = 7,
	eTM_FW_MODE_APP = 8,
}EVT_TM_CTRL_FW_MODE_CHANGE_VALUE;

typedef enum _eEVT_TM_CTRL_TRIGGER_ACTIVE_VALUE{
	eTM_TRIGGER_OFF = 0,
	eTM_TRIGGER_ON = 1,
}eEVT_TM_CTRL_TRIGGER_ACTIVE_VALUE;

#if false
typedef enum _eEVT_TM_CTRL_SET_SERIAL_NUMBER_VALUE{
}eEVT_TM_CTRL_SET_SERIAL_NUMBER_VALUE;

typedef enum _eEVT_TM_CTRL_GET_SERIAL_NUMBER_VALUE{
}eEVT_TM_CTRL_GET_SERIAL_NUMBER_VALUE;

typedef enum _eEVT_TM_CTRL_SELFTEST_LED_BLINK_DELAY_TIME_VALUE{
}eEVT_TM_CTRL_SELFTEST_LED_BLINK_DELAY_TIME_VALUE;

typedef enum _eEVT_TM_CTRL_SELFTEST_START_VALUE{
}eEVT_TM_CTRL_SELFTEST_START_VALUE;
#endif


typedef enum _eEvtTrigger{
	eEVT_TRG_BT_CONNECT = 0,
	eEVT_TRG_BT_DISCONNECT,
	eEVT_TRG_LED_OBD_ON,
	eEVT_TRG_LED_OBD_OFF,
	eEVT_TRG_BTN_ACTIVE,
	eEVT_TRG_CONNECTED,
	eEVT_TRG_CLEAR,
}eEvtTrigger;


typedef enum _eTrgMonitoringMode{
	eTrgMonitorMode_Menual=1,
	eTrgMonitorMode_Dtc=2,
	eTrgMonitorMode_EngineStart=4,
	eTrgMonitorMode_EngineStop=8,
}eTrgMonitoringMode;

typedef enum _eTrgMonitoringStates{
	eTrgMonitoring_Init,
	eTrgMonitoring_WaitTimeOut,
	eTrgMonitoring_Process,
	eTrgMonitoring_ResumeProcess,
	eTrgMonitoring_SleepProcess,
	eTrgMonitoring_Idle,
}eTrgMonitoringStates;


typedef enum _eTriggerControlState{
	eTCS_Init = 0,
	eTCS_Connecting,		
	eTCS_Connected,
	eTCS_Disconnect,
	eTCS_Idle,	
	eTCS_SetConfig,
}eTriggerControlState;



typedef enum _eTrgMonitorTime{
	eTrgMonitorTime_10m = 0,
	eTrgMonitorTime_30m,
	eTrgMonitorTime_60m,
}eTrgMonitoringTime;


typedef struct __stTriggerModuleInfo{
	uint8_t ucCurMode;
	uint8_t ucType;
	uint8_t ucVersion[12];
}stTriggerModuleInfo;

typedef struct __stEngineStopControl{
	tagEngineStopTiggerInfo stEngStopInfo;
	uint8_t ucCount;
}stEngineStopControl;

typedef struct __stDtcControl{
	uint8_t ucDtcCurIndex;// not used
	stTriggerInfo stTrgInfo;
}stDtcControl;

typedef struct __stTriggerMonitoringInfo{		
	eTrgMonitoringStates eState;
	eTrgMonitoringTime eTime;	

	// save dtc buffer
	uint8_t ucTrigDtcBlock[MAX_TRIGGER_DTC_LENGTH];
	uint8_t ucTrigDtcBlockIndex;
	
	uint8_t ucMode;
	uint8_t bDtcActive;
	uint8_t bEngineStopActive;

	uint8_t ucPreDtcCount;
	
	PTmsgPkt_t stTxPacket;	
	stCommPkt stRxPacket;
	
	stDtcControl stDtcCtrl;
	stEngineStopControl stEngineCtrl;
}stTriggerMonitoringInfo;


typedef struct __stTriggerControl
{
	uint8_t bBtConnected;
	uint8_t bUsbConnected;
	uint8_t bUsbBtnPressed;
	uint8_t bSendModeChanged;
	uint8_t bConnected;
	uint8_t bDlyReqObdLed;
	uint8_t ucDlyReqObdLedValue;
	uint8_t bActiveRecordButton;
	uint8_t ucNoBtRespAliveCount;

	uint8_t bActiveTrigger;


	uint8_t bEngineOn;
	
	eTriggerControlState eState;
	stTriggerModuleInfo stTrgCtrl;
	stTriggerMonitoringInfo stMonitoringCtrl;
	stECUIDInfo stEcuInfo;
}stTriggerControl;


bool StartTriggerThread( void );

osThreadId	GetTriggerHandle();


void SendEvent2TriggerThread(eEvtTrigger eEvt);
void SendEvent2RecordThread(eEvtRecording eEvt, uint8_t* pucPayload, uint32_t unLength);
void SendByteEvent2TriggerModule(eTriggerModuleEvent eEvent, uint8_t ucValue);
void SendEvent2TriggerThread(eEvtTrigger eEvt);
void SendEvent2TriggerModule(eTriggerModuleEvent eEvent, uint8_t* pucPayload, uint8_t ucPayloadLength);

void SetCurTriggerModuleLedObdStatus();
void SetCurTriggerModuleConnection();
void SetCurTriggerModuleButtonStatus(uint8_t* pucButtonStatus);
void SetCurTriggerModuleInfo(uint8_t* ucModuleInfo);
void SetCurTriggerModuleTriggerButton(uint8_t* ucTriggerButtonStatus);
void SendSignalBluetoothAlive();
void SystemStandby();

void ClearTriggerMonitoringHandler();


uint8_t GetTriggerDtcData(char* pcData);
void ClearTriggerDtcData();

void hdTriggerSwTimer();
void InitializeTriggerMonitoringState();


#endif // __GIT_TRIGGER_H__
