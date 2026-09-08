/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __MODEM_MANAGER_H__
#define __MODEM_MANAGER_H__

#include <time.h>
#include "GIT_OemInterface.h"
#include "Message_Manager.h"
#include "AutolinkConfig.h"

/* Define ------------------------------------------------------------------*/
#define ENABLE_MESSAGE_SENDING				// 주석 처리하면, event message를 전송하지 않는다.


#if defined(PROTOCOL17)
#define MAX_URL_LENGTH						100
#else
#define MAX_URL_LENGTH						48
#endif
#define MAX_PATH_LENGTH						96
#define MAX_HEADER_CONTENT_LENGTH			8
#define MAX_HCPROP_LENGTH					100

#define MAX_MODEM_COMM_BUFFER_LENGTH		(3000 + 4)
#define MAX_MODEM_COMM_RX_BUFFER_LENGTH		(4096 + 4)

#ifndef USE_KSA_URL_PORT
//#define MODEM_REMOTE_PORT_NO_HTTP			80				// HTTP port no
#define MODEM_REMOTE_PORT_NO_HTTP			8443				// HTTP port no
#else
#define MODEM_REMOTE_PORT_NO_HTTP			8090				// HTTP port no
#endif

#define MODEM_REMOTE_PORT_NO_HTTPS			443				// HTTPS port no

#define	MAX_RCV_SMS_COUNT					20
#define	MAX_SMS_DATA_LENGTH					(256 + 16)

#define MAX_CONNECTION_PROFILE_NO			6
#define MAX_SERVICE_PROFILE_NO				7	// 원래는 9개를 지원한다. 메모리 절양 차원에서...

#define MAX_REGISTER_NETWORK_COUNT          100

#define MODEM_SIGNAL_QUILITY_STABLE_VALUE	1

#ifdef RF_COMMON_MODEM //mod.kks 21.11.02
#define MODEM_INTERNET_CONNECTION_ID_FOTA_REQUEST_FILE_INFO			1
#define MODEM_INTERNET_SERVICE_ID_FOTA_REQUEST_FILE_INFO			1
#else
#define MODEM_INTERNET_CONNECTION_ID_FOTA_REQUEST_FILE_INFO			0
#define MODEM_INTERNET_SERVICE_ID_FOTA_REQUEST_FILE_INFO			0
#endif

#ifdef RF_COMMON_MODEM //mod.kks 21.11.02
#define MODEM_INTERNET_CONNECTION_ID_FOTA_REQUEST_FILE_DOWNLOAD		1
#define MODEM_INTERNET_SERVICE_ID_FOTA_REQUEST_FILE_DOWNLOAD		2
#else
#define MODEM_INTERNET_CONNECTION_ID_FOTA_REQUEST_FILE_DOWNLOAD		0
#define MODEM_INTERNET_SERVICE_ID_FOTA_REQUEST_FILE_DOWNLOAD		1
#endif

#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
#define MODEM_INTERNET_CONNECTION_ID_HTTP_COMM						1
#else
#define MODEM_INTERNET_CONNECTION_ID_HTTP_COMM						0
#endif
#define MODEM_INTERNET_SERVICE_ID_HTTP_COMM							2

#ifdef RF_COMMON_MODEM //mod.kks 21.11.02
#define MODEM_INTERNET_CONNECTION_ID_GET_VEHICLE_INFO				1
#else
#define MODEM_INTERNET_CONNECTION_ID_GET_VEHICLE_INFO				0
#endif
#define MODEM_INTERNET_SERVICE_GET_VEHICLE_INFO						3

#define MAX_RCV_ZERO_BIN_DATA				10
#define MAX_CME_ERROR_STRING				64

#define MAX_MODEM_RETRY_COUNT				5

#define MDM_SYSLOADING_NO_RESPONSE_TIMEOUT			10000
#define MDM_START_NO_RESPONSE_TIMEOUT				20000

#define MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT		10000
#define MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT			20000
#define MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT_2		5000
#define MDM_MSG_PBREADY_NORESPONSE_TIMEOUT			80000
#define MDM_MSG_CIEV_NORESPONSE_TIMEOUT				30000
#define MDM_MSG_URC_TIMEOUT							50000
#define MDM_MSG_URC_TIMEOUT_10SEC					10000
#define MDM_MSG_URC_TIMEOUT_20SEC					20000
#define MDM_CMD_NORESPONSE_TIMEOUT					10000
#define MDM_MSG_AGPS_RCVDATA_TIMEOUT                60000
#define MDM_AGPS_SEND_TIMEOUT						30000

#define MDM_NORECIEVE_TIMEOUT                       (2*60*1000) //2분 체크// mod.pdh 220516 to do check modem state
//#define MDM_NORECIEVE_TIMEOUT                       (2*60) //2분 체크// mod.pdh 220516 to do check modem state

#define MDM_CHECK_NETWORK_STATUS_DLY				1000
#define MDM_CHECK_PROCESS_TIMEOUT_FOR_POWERSAVE		2000


#define MDM_SUB_STATE_WAIT_RESPONSE				100
#define MDM_SUB_STATE_END_PROCESS				101
#define MDM_SUB_STATE_MODEM_RESET				102
#define MDM_SUB_STATE_MODEM_FAIL				103

#define MODEM_RING_PIN_MODE_STD				0
#define MODEM_RING_PIN_MODE_GPIO			1

#define MODEM_RING_PIN_STATE_LOW			0
#define MODEM_RING_PIN_STATE_HIGH			1

#define MAX_MODEM_READ_DATA_LEHGTH			1500

typedef enum _eMODEM_PROCESS_FUNC_RET
{
	eMODEM_PROCESS_FUNC_RET_CONTINUE = 0,
	eMODEM_PROCESS_FUNC_RET_FAIL,
	eMODEM_PROCESS_FUNC_RET_OK,
} eMODEM_PROCESS_FUNC_RET;

typedef eMODEM_PROCESS_FUNC_RET (*modem_process_fn)(void);

typedef __packed struct _stTelNumberInfo
{
	U8 ucTelNumber[11];
}stTelNumberInfo;

typedef __packed struct _stSystemInfo
{
	U8 ucNSI;
	U8 ucServiceState;
	U8 ucNetworkName;
	U8 ucRoamingState;
	U8 ucRat;
}stSystemInfo;

typedef enum _eMODEM_STATE
{
	eMODEM_Idle = 0,
	eMODEM_NONE,
#ifndef RF_COMMON_MODEM	 //mod.kks 21.10.25
	eMODEM_WAIT_RCV_SYSLOADING,
#endif
	eMODEM_WAIT_RCV_SYSSTART,
	eMODEM_INIT_MODEM,                  
	eMODEM_Wait_Message_PBREADY, 
	eMODEM_SetupNetwork,                // RF_COMMON_MODEM 5
	eMODEM_CHECK_NETWORK_REGISTRATION,
	eMODEM_SETTING_TIME,
	eMODEM_EXT_INIT_MODEM,
	eMODEM_CheckModemBaudrete,
	eMODEM_CMD_ERROR,                   // RF_COMMON_MODEM 10
	eMODEM_RUNNING_FOTA,
	eMODEM_WAIT_MESSAGE_COMM_COMPLETE,

	eMODEM_READY,
	eMODEM_RunProcess,
	eMODEM_ENTER_SLEEP_MODE_INIT,       // RF_COMMON_MODEM 15
	eMODEM_ENTER_SLEEP_MODE,
	eMODEM_NOW_SLEEP_MODE,
	eMODEM_PRODUCT_MODE,
	eMODEM_NO_USIM,
	eMODEM_ENTER_POWER_OFF,             // RF_COMMON_MODEM 20
}eMODEM_STATE;

typedef enum _eSOCKET_COMM_STATE
{
	eSOCKET_COMM_STATE_INIT = 0,
	eSOCKET_COMM_STATE_WAITING,
	eSOCKET_COMM_STATE_SET_CONNECTION_CLEAR,
	eSOCKET_COMM_STATE_SET_CONNECTION_PROFILE,
	eSOCKET_COMM_STATE_SET_CONNECTION_APN,
	eSOCKET_COMM_STATE_SET_SERVICE_CLEAR,
	eSOCKET_COMM_STATE_SET_SERVICE_PROFILE,
	eSOCKET_COMM_STATE_SET_SERVICE_USED,
	eSOCKET_COMM_STATE_SET_SERVICE_URL,

	eSOCKET_COMM_STATE_SOCKET_OPEN,
	eSOCKET_COMM_STATE_WAIT_WRITE_READY,
	eSOCKET_COMM_STATE_SET_WRITE_DATA,
	eSOCKET_COMM_STATE_CONFIRM_WRITE_LEN,
	eSOCKET_COMM_STATE_WRITE_DATA,

	eSOCKET_COMM_STATE_READ_DATA,
	eSOCKET_COMM_STATE_CLOSE,

	eSOCKET_COMM_STATE_IDLE,
} eSOCKET_COMM_STATE;

typedef enum _eHTTP_COMM_STATE
{
	eHTTP_COMM_STATE_INIT = 0,
	eHTTP_COMM_STATE_WAITING,
	eHTTP_COMM_STATE_SET_CONNECTION_CLEAR,
#ifdef RF_COMMON_MODEM
	eHTTP_COMM_STATE_CON_ACTIVE,
#endif
	eHTTP_COMM_STATE_SET_CONNECTION_PROFILE,
	eHTTP_COMM_STATE_SET_CONNECTION_APN,
	eHTTP_COMM_STATE_SET_SERVICE_CLEAR,
	eHTTP_COMM_STATE_SET_SERVICE_PROFILE,
	eHTTP_COMM_STATE_SET_SERVICE_USED,
	eHTTP_COMM_STATE_SET_SERVICE_URL,
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
	eHTTP_COMM_STATE_SET_SERVICE_ALPHABET,
	eHTTP_COMM_STATE_SET_SECOPT,
#endif
	eHTTP_COMM_STATE_SET_SERVICE_CMD,
	eHTTP_COMM_STATE_SET_SERVICE_HCCONTENT,				// 10
	eHTTP_COMM_STATE_SET_SERVICE_HCCONLEN,
	eHTTP_COMM_STATE_SET_SERVICE_HCPROP,
#ifdef RF_COMMON_MODEM
	eHTTP_COMM_STATE_SET_SECSNI,
	eHTTP_COMM_STATE_SET_SNINAME,
	eHTTP_COMM_STATE_GET_PDPADDRESS,
	eHTTP_COMM_STATE_GET_CPOS,
	eHTTP_COMM_STATE_GET_SMONI,
#endif
	eHTTP_COMM_STATE_SOCKET_OPEN,

	eHTTP_COMM_STATE_WAIT_OPEN_SUCCESS,
	eHTTP_COMM_STATE_WAIT_WRITE_READY,

	eHTTP_COMM_STATE_SET_WRITE_DATA_LEN,
	eHTTP_COMM_STATE_WAIT_WRITE_COMPLETE,

	eHTTP_COMM_STATE_CONFIRM_WRITE_LEN,
	eHTTP_COMM_STATE_WRITE_DATA,

    eHTTP_COMM_STATE_SET_MULTI_WRITE_DATA_LEN,	//20
    eHTTP_COMM_STATE_WAIT_MULTI_WRITE_COMPLETE,
    eHTTP_COMM_STATE_CONFIRM_MULTI_WRITE_LEN,
    eHTTP_COMM_STATE_MULTI_WRITE_DATA,

	eHTTP_COMM_STATE_WRITE_FINISH,
	eHTTP_COMM_STATE_WAITING_READY_TO_READ,	//25
	eHTTP_COMM_STATE_CONFIRM_WRITE_END_LEN,

	eHTTP_COMM_STATE_WAIT_HTTP_RESPONSE,
	eHTTP_COMM_STATE_WAIT_RCV_END_OF_DATA_FLAG,
	eHTTP_COMM_STATE_READ_DATA,
	eHTTP_COMM_STATE_WAIT_RESP_READ_DATA,	//30

    eHTTP_COMM_STATE_AGPS_READY_TO_READ,

	eHTTP_COMM_STATE_CLOSE,

	eHTTP_COMM_STATE_IDLE,							//33
} eHTTP_COMM_STATE;

//typedef enum _eCHECK_SMS_STATE
//{
//	eCHECK_SMS_STATE_INIT = 0,
//	eCHECK_SMS_STATE_WAITING,
//
//	eCHECK_SMS_STATE_READ_LIST,
//	eCHECK_SMS_STATE_VIEW,
//	eCHECK_SMS_STATE_DELETE,
//
//	eCHECK_SMS_STATE_IDLE,
//} eCHECK_SMS_STATE;

typedef enum _eINTERNET_CONNECTION_PARAM
{
	eINTERNET_CONNECTION_INTERNET_CONN = 0,
	eINTERNET_CONNECTION_USERNAME,
} eINTERNET_CONNECTION_PARAM;

typedef enum _eHTTP_TYPE
{
	eHTTP_TYPE_HTTP = 0,
	eHTTP_TYPE_HTTPS,
} eHTTP_TYPE;

typedef enum _ePACKET_CONNECTION_PARAM
{
	ePACKET_CONNECTION_IPV4 = 0,
	ePACKET_CONNECTION_IPV6,
} ePACKET_CONNECTION_PARAM;

typedef enum _eNETWORK_REG_STATUS
{
	eNETWORK_REG_STATUS_NOT_REG = 0,
	eNETWORK_REG_STATUS_REGISTERED,
	eNETWORK_REG_STATUS_SEARCHING,
	eNETWORK_REG_STATUS_REG_DENIED,
	eNETWORK_REG_STATUS_UNKNOWN,
	eNETWORK_REG_STATUS_REG_ROAMING,
	eNETWORK_REG_STATUS_IDLE = 100,
} eNETWORK_REG_STATUS;

typedef enum _eSMS_STATUS
{
	eSMS_STATUS_REC_UNREAD = 0,
	eSMS_STATUS_REC_READ,
	eSMS_STATUS_UNKNOWN,
} eSMS_STATUS;

typedef enum _eINTERNET_COMM_TYPE
{
	eINTERNET_COMM_TYPE_SOCKET = 0,
	eINTERNET_COMM_TYPE_HTTP,
} eINTERNET_COMM_TYPE;

typedef enum _eRESPONSE_FROM_SERVER
{
	eWAITING_MESSAGE_RESPONSE = 0,
	eSUCCESS_MESSAGE_RESPONSE,
	eFAIL_MESSAGE_RESPONSE,
} eRESPONSE_FROM_SERVER;

typedef enum _eHTTP_HEADER_TYPE
{
	eHTTP_HEADER_TYPE_ASCII = 0,
	eHTTP_HEADER_TYPE_HEX_STRING,
} eHTTP_HEADER_TYPE;

typedef enum _eMODEM_MESSAGE_MODE
{
	eMODEM_MESSAGE_MODE_NONE = 0,
	eMODEM_MESSAGE_MODE_REQUEST_REMOTE,
	eMODEM_MESSAGE_MODE_REQUEST_VEHICLE_STATUS,
	eMODEM_MESSAGE_MODE_VEHICLE_START_UP,
	eMODEM_MESSAGE_MODE_VEHICLE_STOP,
	eMODEM_MESSAGE_MODE_VEHICLE_DOORLOCK,
	eMODEM_MESSAGE_MODE_VEHICLE_DOORUNLOCK,
	eMODEM_MESSAGE_MODE_VEHICLE_RESET,
	eMODEM_MESSAGE_MODE_REQUEST_SETTING_GEOFENCE,
	eMODEM_MESSAGE_MODE_REQUEST_SETTING_POLYGON_GEOFENCE,
	eMODEM_MESSAGE_MODE_REQUEST_SMS_TEST,
	eMODEM_MESSAGE_MODE_REQUEST_MODEM_ACTIVATE,
	eMODEM_MESSAGE_MODE_REQUEST_SENSOR_INITIALIZE,
#if defined(PROTOCOL17)
	eMODEM_MESSAGE_MODE_REQUEST_SETURL,
	eMODEM_MESSAGE_MODE_REQUEST_SETURL_INIT,
#endif
#if defined(PROTOCOL18)
	eMODEM_MESSAGE_MODE_REQUEST_SETTING_RSVENGCTRL,
#endif
	eMODEM_MESSAGE_MODE_REQUEST_REMOTE_TIME_IGNORE,
	eMODEM_MESSAGE_MODE_REQUEST_TRACKINGMODE,
#if defined(PROTOCOL25)
    eMODEM_MESSAGE_MODE_INSTOLATION_REQUEST_SMS_TEST,
#endif
} eMODEM_MESSAGE_MODE;

typedef enum _eMODEM_RESET_MODE
{
	eMODEM_RESET_MODE_POWER_ON,
	eMODEM_RESET_MODE_SOFTWARE,
	eMODEM_RESET_MODE_ONLY_MODEM,
} eMODEM_RESET_MODE;

typedef enum _eFOTA_START_STATE
{
	eFOTA_START_STATE_STOP = 0,
	eFOTA_START_STATE_START_GET_VERSION,
	eFOTA_START_STATE_GET_VERSION,
	eFOTA_START_STATE_START_GET_BIN,
	eFOTA_START_STATE_GET_BIN,
	eFOTA_START_STATE_GET_VEHICLE_INFO,
} eFOTA_START_STATE;

typedef enum _eMODEM_BAUDRATE_SEARCHING_STATE
{
	eMODEM_BAUDRATE_SEARCHING_STATE_UNKNOWN = 0,
	eMODEM_BAUDRATE_SEARCHING_STATE_REQ_SEARCHING,
	eMODEM_BAUDRATE_SEARCHING_STATE_SUCCESS,
	eMODEM_BAUDRATE_SEARCHING_STATE_FAIL,
} eMODEM_BAUDRATE_SEARCHING_STATE;

typedef enum _MODEM_RESPONSE_TYPE
{
	MODEM_RESPONSE_TYPE_COMMON = 0,
	MODEM_RESPONSE_TYPE_MESSAGE,
	MODEM_RESPONSE_TYPE_FOTA,
	MODEM_RESPONSE_TYPE_BAURDRATE,
#ifdef RF_COMMON_MODEM //THALES_AGPS
	MODEM_RESPONSE_TYPE_AGPS,
#endif
	MODEM_RESPONSE_TYPE_NONE,
} MODEM_RESPONSE_TYPE;

typedef enum _eCME_ERROR_NO
{
	eCME_ERROR_NO_OPERATION_NOT_ALLOWED = 3,
	eCME_ERROR_NO_OPERATION_TEMPORARY_NOT_ALLOWED = 256,
	eCME_ERROR_NO_INVALID_INDEX = 21,
	eCME_ERROR_NO_SIM_FAILURE = 13,
	eCME_ERROR_NO_SIM_NOT_INSERTED = 10,
	eCME_ERROR_TEXT_STRING_TOO_LONG = 24,
	eCME_ERROR_UNKNOWN = 100,
	eCME_ERROR_NO_FFFF = 0xFFFF,
} eCME_ERROR_NO;

#pragma pack(push, 1)

typedef __packed struct __BR_ModemInfo{
	bool bHTTPSuccessServiceConnection[MAX_SERVICE_PROFILE_NO];
	uint8_t aCCID[30];
	uint8_t aPhoneNo[20];
	int16_t snTimeZone;
    uint32_t unBaurdRate;
    stNetworkTime stNetworkInfo;
    uint8_t carrDefaultAPN[MAX_AUTOCONFIG_APN_SIZE];
}BR_ModemInfo;

typedef __packed struct _stModemProcess {
	modem_process_fn modemProcessFunc;
	modem_process_fn modemProcessResultFunc;
	uint16_t *pState;
	uint16_t *pNextState;
	MODEM_RESPONSE_TYPE eResponseOption;
} stModemProcess;

typedef __packed struct {
	uint8_t aURL[MAX_URL_LENGTH];
	uint8_t aPath[MAX_PATH_LENGTH];
	uint8_t ahcProp[MAX_HCPROP_LENGTH];
} stInternetServiceProfiles;

typedef __packed struct {
	struct tm strTime;
#ifdef RF_COMMON_MODEM
	int16_t nIndex; //mod.pdh 22.01.26
#else
	uint16_t nIndex;
#endif
	eSMS_STATUS eStatus;
	uint8_t aOriginatingAddr[16];
	uint16_t nDataLen;
	uint8_t aData[MAX_SMS_DATA_LENGTH];
} stRcvSMSInfo;

typedef __packed struct {
	eMODEM_STATE eState;
	eMODEM_STATE eOldState;

	eMODEM_RESET_MODE eModemResetMode;

#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
	bool bModemRcvSysLoadingMessageFlag;
#endif
	bool bModemRcvSysStartMessageFlag;
	bool bModemRcvPBReadyMessageFlag;

	eFOTA_START_STATE eFOTAStartState;

	int iTimer_MDM_URC_Resp_TimeoutDly;				// modem에 URC response를 기다린다. 5초가 응답 없으면 reset 한다.
	int iTimer_MDM_Cmd_Resp_TimeoutDly;			// modem에 명령을 전속하고 response를 기다린다. 5초가 응답 없으면 reset 한다.
	int iTimer_Rcv_Bin_Data_TimeoutDly;			// FOTA를 이용해서 bin file을 수신할때 일정 시간동안 응답이 없는 경우 reset 한다.
	int iTimer_MDM_Check_Network_Status_Dly;		// 초기 구동 시, network 접속 여부를 1초 마다 체크한다. 또는, 구동중에 signal quility 와 network 접속 여부를 5초마다 체크한다.
    int iTimer_MDM_Recieved_TimeoutDly;            // 2분동안 모뎀 동작이 없을 경우 Reset한다. //mod.pdh 220516 to do check modem state
	
	int iTimer_Rcv_AGPS_Data_TimeoutDly;

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
	uint8_t aVendorName[12];
#endif
	uint8_t aProductName[12];
	uint8_t aRevision[18];
	uint8_t aARevision[28];
	uint8_t aIMEI[20];
	uint8_t aPhoneNo[20];
	uint8_t aOperator[20];
    uint8_t aSIMI[17];

    bool bRequestNetworkTime;
	bool bRequestMessageCommFlag;
    bool bRequestAgpsCommFlag;
	bool bCheckNetworkStatusFlag;

  eMESSAGE_TYPE eCurrentMessageSendingType;
  eMESSAGE_TYPE ePreviousMessageSendingType;

	uint16_t nMdmSubState;
	uint16_t nMdmSubNextState;

	stInternetServiceProfiles stInternetServiceProfile[MAX_SERVICE_PROFILE_NO];
	uint16_t nInternetConnectionProfileId;
	uint16_t nInternetServiceProfileId;

	bool bWriteReady;
	bool bConfirmWriteLen;
	bool bFinishWrite;
	bool bReadyReadData;
	bool bEndOfData;
	bool bServiceOpen;
	bool bExpectEndOfData;
	bool bHTTPRcvPostResponse;
	bool bHTTPRcvPostURL;
	bool bRcvBinDataZeroFlag;
	bool bRetryCommFlag;
	bool bRespSISR;
    bool bAirplaneMode;

	bool bModemDataSaveToFlashFlag;

	uint16_t nRetryCommCount;

	bool bNeedStartUpCheckSMSFlag;
	bool bNeedModemHWResetFlag;
	bool bAvailableModemCommFlag;

	eCME_ERROR_NO eCMEErrorCode;
	int nHTTPResponseCode;
	uint16_t nHTTPURCInfoId;
	uint16_t nWriteDataLength;
    uint16_t nSubWriteDataLength;
    uint16_t nSubWriteIndex;
	uint16_t nMaxReadDataLength;
	int nAvailableReadDataLength;
	uint32_t wInternetDataBufferLength;

	uint8_t *paModemCommDataTxBuffer;				// modem 통신에 사용되는 송신 버퍼의 포인터
	uint8_t *paModemCommDataRxBuffer;				// modem 통신에 사용되는 수신 버퍼의 포인터

	eNETWORK_REG_STATUS eNetworkRegStatus;

	stRcvSMSInfo stRcvSMSInfos[MAX_RCV_SMS_COUNT + 1];				// 반드시 MAX_RCV_SMS_COUNT 보다 1크게 할것, 0번을 제외하고 MAX_RCV_SMS_COUNT 만큼 저장한다.

	uint8_t aSMSDestinationAddress[16];
	uint8_t aSMSSendData[MAX_SMS_DATA_LENGTH];
	uint16_t nLengthSMSSendData;

	uint16_t nCurrentSMSCount;
	int nRSSI;
	int ndBm;

	//eRESPONSE_FROM_SERVER eMessageResponseFromServer;/* for Messange M */

	bool bRunWorkaroundFlag;
	bool bRTCSettingCompleteFlag;

	eMODEM_MESSAGE_MODE eModemMessageMode;

    uint32_t unSmsRcvTime;
    uint32_t unSmsTimeOutTime;
	uint16_t nModemNoRegCount;
    bool bNoUsimFlag;
} MODEM_MANAGER_DATA;
#pragma pack(pop, 1)

extern MODEM_MANAGER_DATA ModemManagerData;
//extern stTelNumberInfo g_stTelNumberInfo;
extern stSystemInfo g_stSystemInfo;

extern uint8_t gaModemCommTxDataBuffer[MAX_MODEM_COMM_BUFFER_LENGTH];
extern uint8_t gaModemCommRxDataBuffer[MAX_MODEM_COMM_RX_BUFFER_LENGTH];

extern uint32_t gwModemCommRxDataLength;
extern uint32_t gwTotalReceiveBinDataLength;

extern BR_ModemInfo BkSram_ModemInfo;


extern uint8_t gbModemRingPinMode;
extern uint8_t gbModemRingPinState;

void InitializeModemManager(bool eOnlyModemReset);
void SetModemState(eMODEM_STATE state);
eMODEM_STATE GetModemState(void);
void ModemManager(void);
void MDM_NoResp_CMD_CallBack(void);
void MDM_NoResp_URC_CallBack(void);
void MDM_NoResp_PBREAD_CallBack(void);
void MDM_RcvTimeout_BinData_CallBack(void);
void MDM_AGPS_Send_Timeout_CallBack(void);


void MDM_NoResp_SysLoadingMessage_CallBack(void);
void MDM_NoResp_SysStartMessage_AtWorkaround_CallBack(void);
void MDM_NoResp_SysStartMessage_CallBack(void);
void MDM_SetNetworkRegistrationFlag_CallBack(void);
void MDM_ProcessTimeout_CallBack(void);
void MDM_SetFotaTestFlag_CallBack(void);

void MDM_NoRecieve_Callback(void);//mod.pdh 220516 todo check modem state

void AddSpecialChar(uint8_t nLetter ,char *Buffer,int nStartPos );
void ConvertIntToHex(int nSize ,uint8_t nLength ,char *Buffer ,int nStartPos);
void ConvertIntArrToHexPayload(int nSize ,uint32_t nLength ,char *Buffer ,int nStartPos);
void ConvertCharArrayToHex(int nLength ,char *nLetter ,char *Buffer ,int nStartPos );
void ConvertCharToHex(uint8_t nLetter ,char *Buffer,int nStartPos );

bool ProcessHTTPComm_Message(void);
bool ProcessHTTPComm_FOTA(void);
void ModemSkipStartAndPBRead(void);
void MD_uDelay (const uint32_t usec);
bool CheckProductMode();

void SetServerUtcTime(stHalRTCTypeDef utc);
bool GetServerUtcTime(uint32_t* punUtcTime);
stHalRTCTypeDef ConvertStringToRtcDate(unsigned char *ucString);

////void DescriptionModemError(void);

eMODEM_PROCESS_FUNC_RET ProcessInitializeModem(void);
eMODEM_PROCESS_FUNC_RET ProcessInitializeModemResult(void);

eMODEM_PROCESS_FUNC_RET ProcessSleepExtInitModem(void);

eMODEM_PROCESS_FUNC_RET ProcessModemPowerOff(void);
eMODEM_PROCESS_FUNC_RET ProcessModemPowerOffResult(void);

eMODEM_PROCESS_FUNC_RET ProcessExtInitModem(void);
eMODEM_PROCESS_FUNC_RET ProcessExtInitializeModemResult(void);

eMODEM_PROCESS_FUNC_RET ProcessInitializeNetwork(void);
eMODEM_PROCESS_FUNC_RET ProcessInitializeNetworkResult(void);

eMODEM_PROCESS_FUNC_RET ProcessCheckNetwarkRegistration(void);
eMODEM_PROCESS_FUNC_RET ProcessCheckNetwarkRegistrationResult_First(void);
//bool ProcessCheckNetwarkRegistrationResult(void);

eMODEM_PROCESS_FUNC_RET ProcessSetModemTime(void);
eMODEM_PROCESS_FUNC_RET ProcessSetModemTimeResult(void);

eMODEM_PROCESS_FUNC_RET ProcessCheckNetwarkStatus(void);
eMODEM_PROCESS_FUNC_RET ProcessCheckNetwarkStatusResult(void);

eMODEM_PROCESS_FUNC_RET ProcessSocketComm(void);

eMODEM_PROCESS_FUNC_RET ProcessCheckSMS(void);
eMODEM_PROCESS_FUNC_RET ProcessCheckSMSResult(void);

eMODEM_PROCESS_FUNC_RET ProcessEnterModemPowerSaveMode(void);
eMODEM_PROCESS_FUNC_RET ProcessEnterModemPowerSaveModeResult(void);
eMODEM_PROCESS_FUNC_RET ProcessSleepExtInitializeModemResult(void);

void fnHex2Str(char* out, char* in);



eNETWORK_REG_STATUS IsNetworkRegStatus(void);

/*Inform to message manager*/
bool NeedToCheckSMS(void);

eMODEM_PROCESS_FUNC_RET SelftestProcessConfigModemGPIO(void);
eMODEM_PROCESS_FUNC_RET SelftestProcessSetResetModemGPIO(void);
eMODEM_PROCESS_FUNC_RET SelftestProcessSetModemPower(void);
eMODEM_PROCESS_FUNC_RET SelftestProcessSetFlightMode(void);
eMODEM_PROCESS_FUNC_RET SelftestProcessGetModemInfo(void);
eMODEM_PROCESS_FUNC_RET SelftestProcessSetModemBaudrate(void);
eMODEM_PROCESS_FUNC_RET SelftestProcessInitializeModem(void);
eMODEM_PROCESS_FUNC_RET SelftestProcessRegistUSIM(void);

eMODEM_PROCESS_FUNC_RET ProcessSendAGPSData(void);
eMODEM_PROCESS_FUNC_RET ProcessSendAGPSDataResult(void);

void SetRegModemProcessFunction(modem_process_fn processFunction, modem_process_fn resultFunction, MODEM_RESPONSE_TYPE eType);

void MDM_AGPSTimeout_CallBack(void);

#endif
