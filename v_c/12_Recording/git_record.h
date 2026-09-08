/*----------------------------------------------------------------------
 *   record control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_RECORD_H__
#define	__GIT_RECORD_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "typedef.h"

#include "git_PassthruDefines.h"
#include "git_vci.h"
#include "cmsis_os.h"

#define ENABLE_RECORED_MALLOC

#define ROOT_PATH "/"
#define RECORD_PATH "/03_Record"
#define MAX_FILE_PATH_LENGTH 100

#define SIGNAL_RECORD_CONFIG_PARSING	(int)(0x01)
#define SIGNAL_RECORD_RESUME			(int)(0x02)
#define SIGNAL_RECORD_WAKE_UP			(int)(0x03)
#define SIGNAL_RECORD_ENGINE_START		(int)(0x04)


/*----------------------------------------------------------------------*/


#define MAX_CAN_COMMUNICATION_ERROR_COUNT (5)

#define MAX_CAN_TX_BUFFER_LENGTH (50)
#define MAX_CAN_RX_BUFFER_LENGTH (2000)


// related ecu request code
#define MAX_REQCODE_SIZE (50) 
#define MAX_ITEM_COUNT (1000)
#define MAX_SLEEP_WAIT_TIME (60000)
#define MAX_VEHICLE_TIMEOUT_WAIT_TIME (6000)

#define MAX_BACKUP_BUFFER_LENGTH (4*1024)



#define MAX_ENG_STOP_REQ_LENGTH (20)
#define MAX_ENG_STOP_RES_LENGTH (255)
#define MAX_ENG_STOP_RPM_LENGTH (10)
#define MAX_TRIP_TIME_LENGTH (10)
#define MAX_TRIGGER_DTC_LENGTH (40)


typedef __packed struct __tagEngineStopTiggerInfo
{
	char strRequestCode[20];
	uint16_t usStartPos;
	uint16_t usRealPos;
	uint16_t usDataSize;
	uint16_t usDataType;
	uint16_t usConvType;
	uint16_t usConvRuleA;
	uint16_t usConvRuleB;
	uint16_t usReseved1;
	uint16_t usReseved2;
	uint16_t usReseved3;
}tagEngineStopTiggerInfo;


typedef enum _eEvtRecording{
	eEVT_REC_TRIGGER_CONNECTED = 0,
	eEVT_REC_TRIGGER_DISCONNECT,
	eEVT_REC_TRIGGER_START,
	eEVT_REC_TRIGGER_STOP,
	eEVT_REC_TRIGGER_INFO,
	eEVT_REC_DTC_TRIGGER_START,
	eEVT_REC_FREEZE,
	eEVT_REC_RESUME,
	eEVT_REC_ENGINE_START,
	eEVT_REC_ENGINE_STOP,
	eEVT_REC_CLEAR,
}eEvtRecording;


typedef enum _eWakeupStatus
{
	eFW_SWITCH_NONE,
	eFW_MODE_SLEEP,
	eFW_MODE_WAKEUP,
	eFW_SWITCH_MAX
}eWakeupStatus;


/////////// Flight Record
typedef enum __eRecordingState 
{
	eRecord_Init= 0,
	eRecord_ReadConfig,
	eRecord_HwSetting,
	eRecord_WaitTriggerModule,
	eRecord_OpenEcu,
	eRecord_Monitoring,
	eRecord_Idle,
	eRecord_Error,
	eRecord_Sleep,
} eRecordingState;


typedef enum __eRecordingStateResult{
	eRecordingStateResult_None = 0,
	eRecordingStateResult_Success,
	eRecordingStateResult_Fail,
	eRecordingStateResult_Retry,
}eRecordingStateResult;


typedef __packed struct __stVehicleInfo{
	uint8_t ucVin[20];
	uint8_t ucMaker[20];
	uint8_t ucRegion[20];
	uint8_t ucVehicle[50];
	uint8_t ucModelYear[10];
	uint8_t ucEngineType[20];
	uint8_t ucCommModule[30];
	uint8_t ucEcuId[10];
	uint32_t unSymptCount;
	uint8_t ucSystem1[20];
	uint8_t ucVersion[20];
	uint8_t ucVehicleType[3];
	uint8_t ucTemp[7];
	uint8_t ucSymptGrp[100][5];
	uint8_t ucSympItem[100][5];
	uint8_t ucComment[150];
}stVehicleInfo;

typedef __packed struct __stECUIDInfo{
	uint32_t unProtocolType;
	uint8_t ucInitialAddrCount;
	uint8_t ucInitialAddr;
	uint8_t ucCommCodeCount;
	uint8_t ucCommCodes[5][MAX_REQCODE_SIZE];
	tagHARDWARESET stHwSet;
	tagJ2534SETCONFIG stJ2534SetConfig;
}stECUIDInfo;

typedef __packed struct __stTriggerInfo{
	uint8_t ucTrigMode;
	uint8_t ucRecordTime;
	uint8_t ucDtcReqCodeCount;
	uint8_t ucDtcReqCodes[10][MAX_REQCODE_SIZE];
	uint8_t ucDtcStartPos;
	uint8_t ucDtcReadNo;
	uint8_t ucDtcSkipNo;
	uint8_t ucTrigDtcCount;
	uint8_t ucTrigDtcData[5][2];
	uint16_t usRpmDataIndex;
	uint16_t usRpmXmlDataIndex;
	uint8_t ucRpmStartPos;
	uint8_t ucRpmRealPos;
	uint8_t ucRpmDataSize;
	uint16_t usRpmDataMax;
	uint16_t usRpmDataMin;		
}stTriggerInfo;

typedef __packed struct __stRecordItems{
	uint16_t usRecordItemCount;
	uint16_t usRecordItems[MAX_ITEM_COUNT];
	
#ifdef ENABLE_RECORED_MALLOC	
	uint8_t* pucRecordItemReqCodes[MAX_ITEM_COUNT];
#else
	uint8_t ucRecordItemReqCodes[MAX_ITEM_COUNT][MAX_REQCODE_SIZE];
#endif

}stRecordItems;

typedef __packed struct __stFlightRecordConfigControl
{
	stECUIDInfo	stEcuIdInfo;
	stTriggerInfo stTrgInfo;
	stRecordItems stRecInfo;	
}stFlightRecordConfigControl;

typedef __packed struct __stConfigControl
{
	stFlightRecordConfigControl stCfgInfo;
	tagEngineStopTiggerInfo stEngStopTrgInfo;
}stConfigControl;


typedef struct __stRecordingCanControl
{	
	uint8_t unCanCommErrorCount;
	uint8_t unPreCanCommErrorCount;	
	uint8_t ucCurCommIndex;	
	uint8_t ucMaxCommIndex;
	uint8_t ucOpenCommCount;
	uint8_t ucCloseCommCount;
	
	uint8_t ucPacketIndex;
	PTmsgPkt_t 	stTxPacket;
	stCommPkt	stRxPacket;
}stRecordingCanControl;


typedef __packed struct __stRecordingDtcControl
{	
	uint8_t ucDtcCommCnt;
	uint8_t ucTrigDtcBlock[MAX_TRIGGER_DTC_LENGTH];	
	uint16_t usTrigMode;
}stRecordingDtcControl;

typedef __packed struct __stRecordingDataControl
{	
	uint16_t usBackupDataPosition;

	uint8_t strStartTime[MAX_TRIP_TIME_LENGTH];
	uint8_t strTrigTime[MAX_TRIP_TIME_LENGTH];
	uint8_t strEndTime[MAX_TRIP_TIME_LENGTH];

	uint8_t ucRecordItemCount;
	
	uint16_t usMsgCount;
	uint32_t unUsedRecTime;
	uint32_t unMaxRecordingTime;
	
	uint16_t usRecordTimeAfterTrigger;
	uint32_t unTriggerTime;
	uint8_t ucBackupData[MAX_BACKUP_BUFFER_LENGTH+512]; // 512 guard buffer
}stRecordingDataControl;

typedef struct __stRecordingControl
{
	eRecordingState eRcdState;
	eRecordingState ePreRcdState;

	uint32_t unPreProtocolType;

	uint8_t bConfigError;	
	uint8_t ucRecordingMode;
	uint8_t bIsMultiRecord;	
	uint8_t ucCurSysCnt;
	uint8_t ucTotalSysCnt;

	uint8_t bTrgModuleConnected;

	uint8_t bActiveTrigger;
	uint8_t bTriggerObdLedOn;
	uint8_t bActiveFlightRecording;

#ifdef ENABLE_RECORED_MALLOC
	stConfigControl* pstCfgCtrl;	
#else
	stConfigControl stCfgCtrl;
#endif
	stRecordingDtcControl stDtcCtrl;
	stRecordingCanControl stCanCtrl;
	stRecordingDataControl stDataCtrl;
}stRecordingControl;



#define MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER (24)


bool StartRecordThread( void );

void SaveHeader( void );

void UpdateFlightRecordingTailPacket();
void UpdateRecordMsg2Backup( stCommPkt* pstRxPacket );
void UpdateCurrentRtcInfo(uint8_t* pucTime, uint8_t ucLength);

osThreadId	GetRecordingHandle();

bool GetEcuConfig(stECUIDInfo*	pstEcuIdInfo);
bool GetEngineConfig(tagEngineStopTiggerInfo* pstEngStopTrgInfo);
bool GetTriggerConfig(stTriggerInfo* pstTrgInfo);
bool GetRecordingConfigStatus();


bool SendPassThrouMessage(PTmsgPkt_t* pPTpacket);
void SendRecordingMessage2DiagThread(PTmsgPkt_t	*pstTxPacket);


bool WaitDiagThreadResponsofRecordingSystem(stCommPkt*	pstRxPacket);

bool CovertBuffer2PassThruMessage(uint8_t* pucPayload, uint32_t unLength, PTmsgPkt_t** pPTpacket);


void ActFirstTriggerEnter();
void ActSecondTriggerStop();

void MakeRecordTailPacket(uint8_t* pucPacket, uint32_t* punLength);
void MakeEngineStopMessage(uint8_t ucIndex, tagEngineStopTiggerInfo* pstEngStopCtrl, PTmsgPkt_t* pPTpacket);




// util function
void SystemStandby();
void hexdump(uint8_t* pucBuffer, int32_t nLength);
uint8_t DecToHex(int32_t data);
void DecToAscii(uint32_t data, uint8_t* pcAscii);
bool isTriggerInterfaceMode(uint8_t ucMode);
void InitlalizeRecordingState();
void InitializeSystemModeHandler();




// related with backup ram
void ClearBkRam();
bool isAvailableBkRam();
uint32_t WrBkRam(uint8_t* pucSrc, uint32_t unDataSize);



#endif // __GIT_RECROD_H__
