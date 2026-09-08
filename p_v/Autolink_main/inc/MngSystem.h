/**
  ******************************************************************************
  * @file    ManagerSystem.h
  * @author  GIT Connectivity Development 2 Team
  * @version V1.0.0
  * @date    20-Dec-2017
  * @brief   Header for ManagerSystem.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MANAGER_SYSTEM_H__
#define __MANAGER_SYSTEM_H__

#include "common.h"
#include "AutolinkConfig.h"
#include "AutolinkMessage.h"
#include "GIT_InterProtocol.h"
#include "GIT_SensorProc.h"

/*
----------------------------------------------------------------
    System / OBD / Modem
----------------------------------------------------------------
    OBD - System
----------------------------------------------------------------
    [DTE -> Server]
----------------------------------------------------------------
    1. Interval report      1-1. Driving information
                            1-2. After driving information
                            1-3. Before driving information
                            1-4. No power on information

    2. Event report         2-1. Alram
----------------------------------------------------------------
    [DTE <- Server]
----------------------------------------------------------------
    1. Smart Key            1-1. Request from server
                            1-2. Send results to server
----------------------------------------------------------------
    System -> Modem
----------------------------------------------------------------
    1. Interval/Alram Report
----------------------------------------------------------------

----------------------------------------------------------------*/

// sleep event bit
#define SYS_MNG_HD_SYS_MODEM_SLEEP_OK           0x00000001
#define SYS_MNG_HD_SYS_STORAGE_SLEEP_OK         0x00000002
#define SYS_MNG_HD_SYS_OBD_SLEEP_OK             0x00000004
#if defined(FEATURE_EXTENSION_BOARD)
#define SYS_MNG_HD_SYS_EXTEND_BOARD_SLEEP_OK    0x00000008
#define SYS_MNG_HD_SLEEP_MASK (SYS_MNG_HD_SYS_MODEM_SLEEP_OK|SYS_MNG_HD_SYS_STORAGE_SLEEP_OK|SYS_MNG_HD_SYS_OBD_SLEEP_OK|SYS_MNG_HD_SYS_EXTEND_BOARD_SLEEP_OK)
#else
#define SYS_MNG_HD_SLEEP_MASK (SYS_MNG_HD_SYS_MODEM_SLEEP_OK|SYS_MNG_HD_SYS_STORAGE_SLEEP_OK|SYS_MNG_HD_SYS_OBD_SLEEP_OK)
#endif

#define SYS_MNG_HD_SYS_SLEEP_OK     0x00000010
#define SYS_MNG_HD_MSG_SLEEP_OK     0x00000020
#define SYS_MNG_HD_SENSOR_SLEEP_OK  0x00000040
#define SYS_MNG_HD_BT_SLEEP_OK      0x00000080
#define SYS_MNG_SLEEP_MASK (SYS_MNG_HD_SYS_SLEEP_OK|SYS_MNG_HD_MSG_SLEEP_OK|SYS_MNG_HD_SENSOR_SLEEP_OK|SYS_MNG_HD_BT_SLEEP_OK)

#define SYS_MNG_INTERRUPT_WAIT_MASK (SYS_WAKE_UP_ALRAM|SYS_WAKE_UP_OBD|SYS_WAKE_UP_MODEM)



enum{
    eSysMngCmdNone = 0,
    eSysMngCmdObd,
    eSysMngCmdMdm,
    eSysMngCmdBt,
#if defined(FEATURE_EXTENSION_BOARD)
    eSysMngCmdExtend,
#endif
    eSysMngMax,
};

///////////////////////////////////////////////////
// external events
enum{
    eMngNone = 0,   //0
    eMngSys,
    eMngSysSub,
    eMngSysMsg,
    eMngSysSensor,
    eMngSysFota,    //5
    eMngModem,
    eMngSensor,
    eMngStorage,
    eMngObd,
    eMngBt,         //10
#if defined(FEATURE_EXTENSION_BOARD)
    eMngExtend,
#endif
    eMngMax,        
};

//////////////////////////////////////////////////
enum{
    eMASK_SYSTEM = 0x100,
    eMASK_MODEM = 0x200,
    eMASK_STORAGE = 0x300,
    eMASK_MESSAGE = 0x400,
    eMASK_OBD = 0x500,
    eMASK_SENSOR = 0x600,
    eMASK_FOTA = 0x700,
#if defined(FEATURE_EXTENSION_BOARD)
    eMASK_EXTEND = 0xA00,
#endif
    eMASK_MAX = 0xB00,
};

// external message
// system message
enum{
    eReqSleep = 0x100,
    eRspSleep,
    eReqWakeupInterruptInfo,
    eRspWakeupInterruptInfo,
    eReqChangeMode,
    eReqSetting,
    eReqSystemReset,
    eReqRecovery,
    eReqForcelySleep,
    eReqModemPowerOff,
#if defined(PROTOCOL18)	
    eReqRsvEngCtrlOn,
#endif
    eMaxSystemManager,
#if defined(FEATURE_EXTENSION_BOARD)
    eSYS_ReqSleep = eReqSleep,
    eSYS_RspSleep = eRspSleep,
#endif
};

enum{
	eSysModeNone,
    eSysModeParking,
    eSysModeDriving,
};

// modem message to message
enum{
    eRcvSms = 0x200,
    eRcvSmartKey,
    eMdmStatus,
    eMdmRssi,
    eReqPostpond,
    eSetApn,
    eDummy,
    eMaxModemManager
};

// sotrage message from message handler
enum{
    eReqSaveReport = 0x300,
    eRspSaveReport,
    eReqGetReport,
    eRspGetReport,
    eReqDeleteReport,
    eRspDeleteReport,
    eReqDataExist,
    eRspDataExist,
    eReqStartAutoRead,
    eReqStopAutoRead,
    eReqStorageBlockTrasfer,
    eReqSetDrivingKey,
    eReqSuspend,
    eReqDeleteFile,
    eMaxStorageManager
};

enum{
    eSuspendStart = 0,
    eSuspendStop,
};

// message handler message
enum{
    eReqReport = 0x400,
    eRspReport,
    eReqFota,
    eRspFota,
    eReqCarPowerOn,
    eReqCarPowerOff,
    eReqMsgBlockTrasfer,
    eReqForwardingSmartkey2Bt,
    eReqIpek,
    eRspIpek,
    eReqAgps,
    eRspAgps,
#if defined(PROTOCOL17)
	eReqSysSetting,
#endif
    eMaxMessageManager,
#if defined(FEATURE_EXTENSION_BOARD)
    eAPP_ReqFota = eReqFota,
    eAPP_RspFota = eRspFota,
#endif

};

enum{
    eAgpsDownload,
    eAgpsDownloadDone,
    eAgpsDownloadFail,
};

enum{
    eIpekPhase1=0,
    eIpekPhase2,    
};

// related with eReqBlockTrasfer
enum{
    eBlockStop = 0,
    eBlockStart
};

enum{
    eFwVehicleInfo = 0,
    eFwInfo,
    eFwBin,
    eFwUpdate,
    eFwTestUpdate,
    eMaxFotaMessage
};

// obd message to message
enum{
    eOBDStatus = 0x500,
    eOBDSetting,
    eOBDKeepAlive,
#if defined(QA_FIFA)
    eOBDEngineStatus,
#endif
};

enum{
    eKeepAliveStart = 0,
    eKeepAliveStop,
};
#if defined(QA_FIFA)
enum{
	eEngineNone = 0,
	eEngineStop,
	eEngineStart,
};
#endif

enum{
    eIGStatus,
    eNewVin,
    eInitailize,
    eUpdateMode,
};

// sensor message
enum{
    eReqWom = 0x600,
    eRsqWom,
    eReqTowInt,
    eRspTowInt,
    eMaxSonsorMessage,
};

// fota message
enum{
    eReqUpdate = 0x700,
    eRspUpdate,
    eMaxFotaManager,
};

#if defined(FEATURE_EXTENSION_BOARD)
/****************************************************/
// Extend board Message
/****************************************************/
///////////////////////////////////////////////////
// system event id list

enum {
	eExtend_ReqReport = eMASK_EXTEND,
	eExtend_RspReport,
	eExtend_ReqControl,
	eExtend_RspControl,
	eExtend_ReqFota,
	eExtend_RspFota,
	eExtend_RspFirstGenControl,
	eExtend_OneSideMsg,
	eMaxExtendMessage
};
#endif

// sub common event list
// report related
enum{
    eR_None = 0,
    eR_BeforeDriving,
    eR_DrivingInterval,
    eR_AfterDriving,
    eR_ParkingInterval,	//5
    eR_Alram,
    eR_AlramMasking,
    eR_AlramDTC,
    eR_ImpulseAlram,
    eR_ReqSmartKey,
    eR_RspSmartKey,	//10
    eR_CurrentVehicleStatus,
    eR_SettingGeofence,
    eR_SettingPolygonGeofence,
    eR_ReqUserActionSetting,
    eR_RspUserActionSetting,	//15
    eR_SmsTest,
    eR_SettingInfo,
    eR_WriteTest,
    eR_ReqModemActivate,
    eR_RspModemActivate,	//20
    eR_ReqSensorInitialize,
    eR_RspSensorInitialize,    
#if defined(PROTOCOL17)
	eR_ReqSetURL,
	eR_ResSetURL,
	eR_ReqSetURLInit,
	eR_ReqSetURLSave,
#endif
#if defined(PROTOCOL18)
	eR_ReqSettingRsvEngCtrl,
	eR_RspSettingRsvEngCtrl,
	eR_Charging,
#endif
#if defined(FEATURE_EXTENSION_BOARD)
    eR_ReqFOBStatus,
    eR_ResFOBStatus,
#endif
	eR_SetTrackingMode,
	eR_SetTrackingModeRsp,
	eR_TrackingReport,
#if defined(PROTOCOL24)
	eR_ReportNetworkStatus,
#endif
#if defined(PROTOCOL25)
    eR_ReportInstallationNetworkCheck,
    eR_ReportInstallationSMSCheck,
#endif
    eR_Max
};

enum{
    eSuccess = 0,
    eFail,
};

enum{
    eFalse = 0,
    eTrue = 1,
    eMaxResult1
};

// sub modem status
enum{
    eMS_NoRspSysload = 0,
    eMS_ServerConnected,
    eMS_ServerDisconnected,
    eMS_ChangeOpenSock,
    eMS_PhoneNum,
    eMS_NetworkTime,
    eMS_MaxModemStatus
};

enum {
    SYSTEM_MODE_PARKING = 0,
    SYSTEM_MODE_DRIVING,
//    SYSTEM_MODE_IMPULSE,
//    SYSTEM_MODE_ALRAM,
};

// related with modem handler
enum{
    eReqCommand = 0,
};

enum {
    eMdmReqNone = 0,
    eMdmReqGemaltoReset,
    eMdmReqGemaltoWorkAround,
    eMdmReqNetworkTime,
    eMdmReqRssi,
    eMdmReqDonothing,    
    eMdmReqReInit,
    eMdmReqSleepAll,
    eMdmReqSleep,
};

#define MAX_PHONE_NUMBER_SIZE 20
#define SW_RESET_SIGNAL 0xABBACDDC
#define ODO_CLEAR_SIGNAL 0xADBCDACB
#define RTC_SET_ENABLE 0xADADADAD

int32_t HdMngSys(int32_t lparam,int32_t rparam);
int32_t HdMngSysObd(int32_t lparam, int32_t rparam);
int32_t HdMngSysMsg(int32_t lparam, int32_t rparam);
int32_t HdMngSysBt(int32_t lparam, int32_t rparam);
int32_t HdMngSysSensor(int32_t lparam, int32_t rparam);

#include "MngQueue.h"

typedef int32_t (*fnpMngSysWorkList)(int32_t,int32_t);

enum{
    SYS_WORK_NONE = 0,
    SYS_WORK_MNG_SYS,
    SYS_WORK_MNG_OBD,
    SYS_WORK_MNG_MDM,
    SYS_WORK_MNG_BT,
#if defined(FEATURE_EXTENSION_BOARD)
    SYS_WORK_MNG_EXTEND,
#endif
    SYS_WORK_MNG_SENSOR,
    MAX_SYS_WORK_LIST
};

typedef __packed struct _stMngSysSetting
{
    uint8_t test;
}stMngSysSetting;

typedef __packed struct _stMngSysData
{
    boolean_t bPowerOff;
    boolean_t m_bFirstBoot;
    int32_t m_iSystemMode;
    fnpMngSysWorkList fnMngSysWorkList[MAX_SYS_WORK_LIST];
    fnpMngSysWorkList fnMngSysWorkListResp[MAX_SYS_WORK_LIST];
}stMngSysData;

typedef __packed struct __stMsgHandlerData
{
    // state variable
    int32_t iHdMngMsgState;
    // sleep check register
    uint32_t uiSubSysSleepEvent;
    // no parking report time out timer id
    int32_t iTimeoutNoDrivingReportTimerID;
    // interval timer id
    int32_t iIntervalTimerID;
    // time out timer id
    int32_t iTimeoutTimerID;
    // rssi of gemalto
    int32_t iModemRSSI;
    // modem status
    boolean_t bServerConnected;
    // this boolean is notifing that this message from storage. so we need to delete
    boolean_t bIsStoreData;
    // phone number
    uint8_t chaPhoneNo[MAX_PHONE_NUMBER_SIZE];
    // network time
    stNetworkTime stNetworkDate;
    // set alram time
    stHalRTCTypeDef stSetAlramTime;
    // driving key
    long long m_llSytemDrivingKey;
    // this system allow just one smartkey message so we should check this flag
    boolean_t bAlreadyRcvSmartkey;
    // flag for update
    boolean_t bNeedUpdate;
    // flag for power off from obd
    boolean_t bIGOff;
    uint8_t ucIGOffCount;
    // request for postponding
    boolean_t bReqPostPondforSending;
    // request wait flag that network is not initialized
    boolean_t bReqWaitSendingforNetworkTime;
    // obd setting value
    stReportSetting stObdSettingValue;
    // suspend sending message
    boolean_t bSuspend;
    // skip sending message
    boolean_t bSkipMessage;
    //receive smartkey from BT
    boolean_t bRcvBTSmartkey; //dahae
}stMsgHandlerData;

typedef __packed struct __stUsimInfo
{
    uint8_t cellPhoneNum[MAX_PHONE_NUMBER_SIZE];
}stUsimInfo;

// this struct is related with backup ram
typedef __packed struct __BR_SystemInfo
{
    uint32_t unResetCount;
    uint32_t unDrivingInterval;
    uint32_t unWakeUpInterval;
    uint32_t unSystemTimeOut;
    uint32_t unFotaInterval;
    uint32_t unFotaUpdateTime;    
    uint8_t ucDoorLockStatus;
    uint8_t ucDoorOpenStatus;
    uint8_t ucHeadLight;
    uint32_t unWakeupTime;
    uint32_t unPowerOffTime;
    // this count used power on reset count
    uint16_t usSoftwareResetCount;
    // this is flag for modem power off.
    uint8_t bModemPowerOff;    
    uint32_t unAGPSExireDate;
    uint8_t bTestFota;
    stSensorInfo stGyroAngle;
    uint8_t ucarrRFIDUID[RFID_08C_DATA_LENGTH]; // woong bae 18/03/31 RFID UID AuAaCI¡¾a A¡×CN ©öe¢¯¡©
    uint8_t checksum;
#ifndef ENABLE_STANDBY_MODE
	uint8_t ucForceReset;
#endif
}BR_SystemInfo;

// system state
enum{
    STAT_SYS_MNG_HD_MSG_INIT = 0,
    STAT_SYS_MNG_HD_MSG_SLEEP,
    STAT_SYS_MNG_HD_MSG_SLEEP_WAIT,
    STAT_SYS_MNG_HD_MSG_RUN,
    STAT_SYS_MNG_HD_MSG_IDLE,
    STAT_SYS_MNG_HD_MSG_SETUP,
};

// definition for check system mode
#define SYS_WAKE_UP_ALRAM   0x00000001
#define SYS_WAKE_UP_SENSOR  0x00000002
#define SYS_WAKE_UP_OBD     0x00000004
#define SYS_WAKE_UP_MODEM   0x00000008


#define FILE_NAME_REPORT 0
#define FILE_NAME_CONTROL 1
#define DEVICE_RESET_COUNT_LIMIT 10

enum{
    STAT_PARKING_INIT=0,
    STAT_PARKING_REQ_OBD_REPORT,
    STAT_PARKING_RSP_OBD_REPORT,
    STAT_PARKING_REQ_MODEM_REPORT,
    STAT_PARKING_RSP_MODEM_REPORT,
    STAT_PARKING_REQ_SAVE_REPORT,
    STAT_PARKING_RSP_SAVE_REPORT,
    STAT_PARKING_REQ_GET_REPORT,
    STAT_PARKING_RSP_GET_REPORT,
    STAT_PARKING_REQ_DEL_REPORT,
    STAT_PARKING_RSP_DEL_REPORT,
    STAT_PARKING_REQ_LAST_REPORT,
    STAT_PARKING_RSP_LAST_REPORT,
    STAT_PARKING_IDLE,
    STAT_PARKING_SLEEP,
};

enum {
    eValetNone = 0,
    eValueDistance,
    eValuePoweron,
};

enum{
    eUbloxInit = 0,
    eUbloxUpload,
    eUbloxDownload,
    eUbloxIdle,
    eUbloxDisableNmea,
};

#if defined(PROTOCOL18)	
enum{
	eRsvEngOnNotNow = 0,	
	eRsvEngOnNow,
	eRsvEngOnInOneMin=10,
	eRsvEngOnPassTenSec,
};
#endif
typedef enum __eGeoFenceType
{
    eGFT_CIRCLE = 0, 
    eGFT_POLYGON,
    eGFT_VALET,
}eGeoFenceType;

int32_t EHL_MngNone(stMsgSysMsg* pstMsgSysMsg);
int32_t EHL_MngSys(stMsgSysMsg* pstMsgSysMsg);
int32_t EHL_MngSysSub(stMsgSysMsg* pstMsgSysMsg);
int32_t EHL_MngSysMsg(stMsgSysMsg* pstMsgSysMsg);
int32_t EHL_MngSysSensor(stMsgSysMsg* pstMsgSysMsg);
int32_t EHL_MngSysFota(stMsgSysMsg* pstMsgSysMsg);
int32_t EHL_MngModem(stMsgSysMsg* pstMsgSysMsg);
int32_t EHL_MngSensor(stMsgSysMsg* pstMsgSysMsg);
int32_t EHL_MngStorage(stMsgSysMsg* pstMsgSysMsg);
int32_t EHL_MngObd(stMsgSysMsg* pstMsgSysMsg);
#if defined(FEATURE_EXTENSION_BOARD)
int32_t EHL_MngExtend(stMsgSysMsg* pstMsgSysMsg);
#endif

void GetGyroInitializeAngle(stSensorInfo* pstGyroAngle);
void SetGyroInitializeAngle(stSensorInfo* pstGyroAngle);
void UpdataGyroDefualtAngle();
void ClearSystemMessageUserData();

uint32_t GetSystemOdometer();
void SetSystemOdometer(uint32_t unOdometer);
void SystemForcelyReset();
#if defined(PROTOCOL17)
void HandlerSysSetting(stMsgSysMsg* pstMsgSysMsg);
void ResponseSysSetUrl(stMsgSysMsg* pstMsgMdm,uint8_t ucResult, uint32_t ucReason);
void ChangeURL(stMsgSysMsg* pstMsgSysMsg);
void SendDefaultUrlComplete(stMsgSysMsg* pstMsgSysMsg);
#endif
#ifndef ENABLE_STANDBY_MODE
void UpdateBackupRam(boolean_t bForceReset);
void UpdateSystemBackupRam(boolean_t bForceReset);
boolean_t LoadSystemBackupRam();
#endif

#endif //__MANAGER_SYSTEM_H__

/***************************** END OF FILE ****/
