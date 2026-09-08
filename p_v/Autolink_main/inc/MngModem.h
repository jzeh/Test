/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __MANAGER_MODEM_H__
#define __MANAGER_MODEM_H__

#include <time.h>
#include "GIT_OemInterface.h"
#include "Message_Manager.h"
#include "AutolinkMessage.h"
#include "SysHalFileSystem.h"
#include "AutolinkConfig.h"

#define USE_KMS_ENCRYPT
#define USE_KMS_BASE64_ENCRYPT
//#define ENABL_KMS_LOG
#define ENABLE_FOTA

#define MAX_MODEM_RESET_RETRY_COUNT 3
// when system request sleep to modem, modem must be response for sleep.
// if during the this time the device won't reponse, then forcely go to sleep.
#define TIMER_REQ_SLEEP_TIMEOUT_VALUE               (30*ONE_SECOND)
// if sending data is failed in 3 times continuously, 
// we shoul wait 30seconds for stable of modem.
#define TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE ((1*ONE_MINUTE)/4)
#define TIMER_REQ_WAIT_NETWORK_REGISTER_VALUE       ((2*ONE_MINUTE)/4)

#define MAX_MODEMSTATEBUFFER_SIZE 50

typedef struct{
    //변하면 저장
    uint8_t        ucMngMsgMdmState;
    uint8_t        ucMdmOldState;
    uint8_t        ucMdmNewState;
    uint8_t        ucMdmNewEvent;
    uint8_t        ucModemManagerState;
    //추가 저장
    boolean_t      bReqNetworkPostPone;
    boolean_t      bReqSleep;
    boolean_t      bPostponeReqSleep;
    boolean_t      bRecoveryMessageFlag;
    boolean_t      bRcvSms;
    boolean_t      bWaitforSendingData;
    boolean_t      bSentLastMessage;
    boolean_t      bModemRcvSysLoadingMessageFlag;
    boolean_t      bModemRcvSysStartMessageFlag;
    boolean_t      bModemRcvPBReadyMessageFlag;
    boolean_t      bRequestMessageCommFlag;
    boolean_t      bRequestAgpsCommFlag;
    boolean_t      bCheckNetworkStatusFlag;
    boolean_t      bNeedStartUpCheckSMSFlag;
    boolean_t      bAvailableModemCommFlag;
    uint8_t        ucFOTAStartState;
    boolean_t      bServiceOpen;
    boolean_t      bWriteReady;
    boolean_t      bConfirmWriteLen;
    boolean_t      bFinishWrite;
    boolean_t      bReadyReadData;
    boolean_t      bEndOfData; 
    boolean_t      bExpectEndOfData;
    boolean_t      bRcvBinDataZeroFlag;
    uint8_t        ucRecoveryRetryCount;
    boolean_t      bIsStoreData;
    uint8_t        ucMdmSubState;
    uint8_t        ucMdmSubNextState;
    uint8_t        ucOBDState;
    boolean_t      bOBDSleepIntoFlag;
    uint8_t        ucIGOffCnt;
    boolean_t      bSuspendStorage;
    boolean_t      bSendReport;
	boolean_t	   bServerConnected;
    uint8_t        arrSMONI[80];
    uint8_t        ucCGREG;
	uint32_t       OccurredUtcTime;
    uint8_t        MDResponse;
    uint8_t        cmdIndex;
    uint16_t       eCMEErrorCode;
    uint32_t       HTTPResponseCode;  
    uint16_t       checksum;
} MODEM_STATE_COLLECTION;

typedef struct{
    uint16_t rdindex;
    uint16_t wrindex;
    MODEM_STATE_COLLECTION *ModemStateCollection;

} MODEM_STATE_STRUCTURE;

enum{
    eModemControlStateIdle = 0,
    eModemControlStateInit,
    eModemControlStateConnecting,
    eModemControlStateConnected,
    eModemControlStateReConnected,
    eModemControlStateDisconnected,
    eModemControlStateSleep,
};

enum{
    eMngMdmNone                                     = 0,
    eMngMdmInitializeHardware                       = 0x01,
    eMngMdmInitializeSoftware                       = 0x02,
    eMngMdmRegistedNetwork                          = 0x04,
    eMngMdmConnected                                = 0x08,
    eMngMdmReConnected                              = 0x10, // 5
    eMngMdmSleep                                    = 0x20,
    eMngMdmDetachedBaseStation                      = 0x40,
    eMngMdmAttachedBaseStation                      = 0x80,
    eMngMdmGeneralError                             = 0x100,
    eMngMdmCriticalError                            = 0x200, // 10
    eMngMdmNotRegistered                            = 0x400,
    eMngMdmDoNothing                                = 0x800,
    eMngMdmWaitForModemStable                       = 0x1000, 
    eMngMdmWaitForModemStableShortTime              = 0x2000,
    eMngMdmRequestSendResult_Success                = 0x4000, // 15
    eMngMdmRequestSendResult_Fail                   = 0x8000,
    eMngMdmRequestSendResult_Fail_NotFoundServer    = 0x10000,
    eMngMdmRequestSendResult_Fail_NotStableServer   = 0x20000,
    eMngMdmRequestSendResult_Fail_SockError         = 0x40000,
    eMngMdmRequestSendResult_Fail_General           = 0x80000, // 20
};

enum{
    eModemProcessNone = 0,
    eModemProcessSystemHwReset,
    eModemProcessModemHwReset,
    eModemProcessAirplane,
    eModemProcessRssi,
    eModemProcessSms,
    eModemProcessInterval,
    eModemProcessFota,
};

enum{
    eModemLedStatus_Disconnect,
    eModemLedStatus_Connected,
    eModemLedStatus_Transfer,
};

enum {
    STAT_MNG_MDM_INIT = 0,
    STAT_MNG_MDM_SLEEP,
    STAT_MNG_MDM_SLEEP_WAIT,
    STAT_MNG_MDM_IDLE,
};

typedef enum _eSmsRcvStatus{
    eSmsRcvStatusNone=0,
    eSmsRcvStatusRcvIndex,
    eSmsRcvStatusRcvData,
    eSmsRcvStatusDelete,
}eSmsRcvStatus;

typedef enum _eModemStateSetType{
	MngMdmState = 0,
	ModemOldState,
	ModemNewState,
	ModemNewEvent,
	ModemOldEvent,
	ModemManagerState,
} eModemStateSetType;

typedef enum _eMODEM_ERROR_STATE
{
	MODEM_DENIED = 0,
	MODEM_NOT_COMMUNICATION,
	MODEM_CME_ERROR,
}eMODEM_ERROR_STATE;

void ModemInitalize();

void SetModemLedStatus(uint8_t cLedStatus);
void SetModemControlEvent(int32_t nStatus);

#define AUTOLINK_AGPS_DATA      "UbloxAgpsData"

void WriteModemConfiguration(boolean_t bRecoveryMessage, stMsgMdm stRecoveryMessage);
void ReadModemConfiguration(boolean_t* pbRecoveryMessage, stMsgMdm* pstRevoeryMessage);
eGitFresult DeleteAgpsData(char* pstrFileName);
boolean_t IsModemConnected();
void ModemReset_Install();
uint32_t GetModemControlOldEvent();
uint32_t GetModemControlEvent();
uint32_t GetModemControlLastEvent();
boolean_t GetRemoteControlStatus();
void Send2MngModemHandler(uint16_t iId, int32_t iEvent,int32_t iSubEvent,boolean_t bForce);
void SetWaitforSendingData(boolean_t bWaitforSendingData);
void SetReqWaitNetworkCheck(boolean_t bReqWaitNetworkCheck);
#if defined(PROTOCOL24)
int ReportModemStatus(unsigned char ucModemConnectStatus);
#endif

int GetModemStateBuffer(MODEM_STATE_COLLECTION *pGetModemState);
void PutModemStateBuffer(char* pPutModemState);
void ModemStateBufferClear();
void PushModemStateCollection(eModemStateSetType Status, char StatusValue);

#endif //__MANAGER_MODEM_H__
