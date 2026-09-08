/* Includes ------------------------------------------------------------------*/

#include <stdlib.h>
#include <time.h>

#include "AutolinkConfig.h"
#include "AutolinkConfiguration.h"
#include "Modem_Manager.h"
#include "FOTA_Manager.h"
#include "Autolink_Manager.h"
#include "GIT_Util.h"
#include "Modem_comm.h"
#include "UARTDMA_Manager.h"
#include "Message_Manager.h"
#include "GIT_base64.h"
#include "Power_Manager.h"
#include "GIT_AesEncrypt.h"
#include "OBD_Controller.h"
#include "Message_Make.h"
#include "MngModem.h"
#include "MngQueue.h"
#include "MngSystem.h"
#include "HdDebug.h"
#include "DebugHandler.h"
#include "GIT_OemInterface.h"
#include "MngSystemUtil.h"
#include "Share_InterFunction.h"

#include "HalHandler.h"


#define ENABLE_ROAMING_PROCESS
//#define USE_ONLY_3G
//#define USE_ONLY_4G
//#define USE_3G_4G
//#define SELECT_OPERATOR



// 007C : ||
// 0024 : $$
// 0021 : !!
const char m_carrPreableOfSmsCommander[2] = {0x21,0x21}; //!!
const char* m_pcarrPreableOfSmsCommander = "00210021"; //!!

//const char m_SmsSample[] = {
//    0x21,0x21,0x72,0x48,0x4f,0x44,0x61,0x59,0x6c,0x78,0x51,
//    0x51,0x74,0x4a,0x2f,0x2f,0x4f,0x36,0x4a,0x77,0x62,0x66,
//    0x51,0x64,0x4b,0x4b,0x62,0x49,0x61,0x5a,0x6d,0x53,0x34,
//    0x34,0x6d,0x78,0x65,0x4d,0x65,0x4f,0x77,0x51,0x76,0x72,
//    0x4c,0x55,0x59,0x52,0x6e,0x72,0x4c,0x2f,0x6d,0x64,0x65,
//    0x55,0x6f,0x31,0x49,0x6f,0x32,0x6c,0x31,0x59,0x2b,0x41,
//    0x21,0x21};


/* Define -------------------------------------------------------------------*/
#define MAX_SERCH_BAUDRATE_STEP_NO				3
//#define MAX_SERCH_BAUDRATE_STEP_NO				14
#define MAX_WRITE_DATA_AT_ONCE 1500
#ifdef RF_COMMON_MODEM //mod.pdh 22.01.26 SMS index start pls : 0, els : 1
#define NO_SMS_INDEX (-1)
#else
#define NO_SMS_INDEX (0)
#endif
/* Variable -----------------------------------------------------------------*/
//*****************************************************************************
MODEM_MANAGER_DATA ModemManagerData;

#pragma section="BKSRAM"
BR_ModemInfo BkSram_ModemInfo @"BKSRAM";

stSystemInfo g_stSystemInfo = {0,};

uint8_t gaModemCommTxDataBuffer[MAX_MODEM_COMM_BUFFER_LENGTH];				// modem 통신을 이용해서 전송되는 전문은 여기에 저장한다.
uint8_t gaModemCommRxDataBuffer[MAX_MODEM_COMM_RX_BUFFER_LENGTH];				// modem 통신을 이용해서 응답되는 전문은 여기에 저장한다.

uint32_t gwModemCommRxDataLength;
uint32_t gwTotalReceiveBinDataLength;

eMODEM_BAUDRATE_SEARCHING_STATE geChangeDefaultModemBaudFlag;

uint32_t gaModemSearchBaudrateStep[MAX_SERCH_BAUDRATE_STEP_NO] = {115200,921600,3000000};
//uint32_t gaModemSearchBaudrateStep[MAX_SERCH_BAUDRATE_STEP_NO] = {1200,2400,4800,9600,19200,38400,57600,115200,230400,460800,500000,750000,921600,3000000};
uint16_t gnModemSearchBaudrateStepCount;

uint8_t gaGITSMSBuffer[384];
uint16_t gaGITSMSBufferLength;
bool gbGITSMSEnablePatialFlag = false;
bool gbGITSMSReceivedSuccessFlag = false;
extern bool g_bAPNFlag;
extern int iTimer_AGPS_Send_TimeoutDly;

uint8_t g_arrXXXCompareString[64];
uint16_t g_nXXXCompareStringLength;
int g_iTimerProcessTimeoutCallback = -1;

unsigned int g_uiReadAGPSFilesize=0;


unsigned char g_ucRemodePhoneNumCnt = '1';

extern unsigned char g_ucUSIMServerPhoneNo[20];
extern unsigned int  g_uiUSIMCountryCode;
extern stAutolinkConfigData m_stAutolinkConfigData;
extern boolean_t g_bCompleteDataFlag;
extern boolean_t Md_GetPDPAddress(void);
extern uint8_t g_bEnableModemDirectCommunication;
extern MODEM_STATE_STRUCTURE g_ModemStateStructure;

//*****************************************************************************
extern bool gbStartInterceptModemRcvData;
extern uint8_t gb_RemoteMessageBuffer[MAX_CONTROL_REQUEST_FROM_SERVER_BUFFER_LENGTH];
extern uint16_t gn_RemoteMessageBufferLength;
extern boolean_t m_bPostponeReqSleep;

extern boolean_t IsModemConnected();
extern void SystemForcelyReset_PowerOn();
//*****************************************************************************

static unsigned long m_ulRxTimeStamp = 0;

void ClearNetworkDelayProcess();
bool NetworkDelayProcess(int nDelay);
void ResetForceModem();

// 180717 SPARROW
bool g_bModemBootingError_Flag = 0;

extern U8 g_arrYUJINTestIdx[2];
extern bool g_bAnyEventSentFlag;
extern unsigned int g_uiAGPSFilesize;
extern void PlusDeviceResetCount();
extern void ClearDeviceResetCount();
extern void LoadInitializeDamoKey(DukptFutureKeyInfo* pstFutureKey);
int ReadAgpsData(char* pstrFileName, char* pcarrBuff, int nSize, int nIndex);
//stTelNumberInfo g_stTelNumberInfo;
//*****************************************************************************

/* Function ------------------------------------------------------------------*/
eMODEM_PROCESS_FUNC_RET ProcessCMEErrorWorkaround(void);
eMODEM_PROCESS_FUNC_RET ProcessCMEErrorWorkaroundResult(void);
eMODEM_PROCESS_FUNC_RET ProcessModemHWReset(void);
eMODEM_PROCESS_FUNC_RET ProcessModemHWResetResult(void);
eMODEM_PROCESS_FUNC_RET ProcessModemStartUp(void);
eMODEM_PROCESS_FUNC_RET ProcessCheckModemBaudrate(void);
eMODEM_PROCESS_FUNC_RET ProcessCheckModemBaudrateResult(void);

eMODEM_PROCESS_FUNC_RET ProcessGetModemTime(void);
eMODEM_PROCESS_FUNC_RET ProcessGetModemTimeResult(void);

eMODEM_PROCESS_FUNC_RET ProcessHTTPComm(void);
eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_FOTAGetVer(void);
eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_FOTAGetBin(void);
eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_Message(void);
eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_GetVehicleInfo(void);
eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_AGPSGetData(void);

eMODEM_PROCESS_FUNC_RET ProcessCheckRssiResult(void);
eMODEM_PROCESS_FUNC_RET ProcessCheckRssi(void);

void ProcessMDResponse();
// 2018.09.04 SPARROW : 생산테스트용 modem response 함수
void SelftestProcessMDResponse();

int m_iMdmMainState=0;

boolean_t m_bFirstModemBoot = true;

#define Trace(...)  GITDebug(DEBUG_MODULES_MODEM,__VA_ARGS__)

extern void ClearModemResetRetryCount();
void SetModemControlEvent(int32_t nStatus);

void IncreaseModemResetRetryCount();

extern void SetNetworkPostPoneFlag(boolean_t bPostPonded);
extern void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern bool FOTA_MakeRequestVehicleInfo(void);
extern bool NEWFOTA_MakeRequestVehicleInfo(void);
extern eFOTA_RESULT_CODE NEWFOTA_ParseReceivedVersionInfo(void);
extern void FOTA_GetVehicleInfo(void);
extern void SetRequestSendResult(int16_t usResult);
extern uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
extern uint32_t GetUTCTime();
extern boolean_t Md_GetIMSI();	// 1 : Result Off   0 : Result On
extern boolean_t GetUserApn(char* pstrApn);
extern void SetReqWaitNetworkCheck(boolean_t bReqWaitNetworkCheck);
extern int GetLastSendMessageCount();
extern bool ApplyDecryption3(uint8_t *arrDecryptionText, uint16_t *nDecryptionTextLen, uint8_t *arrEncryptionText, uint16_t nEncryptionTextLen,DukptPinEntry* pstEncryptKeyEntry);
extern void GetLastUsedDukptPinEntry(DukptPinEntry* pstEncryptKeyEntry);
extern void SetReqWaitNetworkCheck(boolean_t bReqWaitNetworkCheck);
extern void ConvertUtc2LocalTimeTest();
extern void GetDatefromDateArray(char* parrDateTime,stHalRTCTypeDef* pstDate);
extern boolean_t IsModemSleepWaitState();
extern boolean_t Md_CheckAirplane(uint8_t * state);
extern boolean_t IsGemaltoAvailable();
extern uint32_t GetModemControlOldState();
extern boolean_t GetReqWaitNetworkCheck();
extern uint32_t ConvertUtc2LocalTime(uint32_t unUtcTime);
extern boolean_t Md_SetRegistPhoneNumMode(uint8_t * state);
extern eGitFresult ReadAgpsDataSize(char* pstrFileName, int* pnSize);
extern boolean_t Md_SetFactorymode(void);

/* ---------------------------------------------------------------------------*/
void InitializeModemManager(bool eOnlyModemReset)
{
	uint16_t n;

	Trace("@InitializeModemManager()\r\n");

	if(eOnlyModemReset == true) {
		Trace("*eMODEM_RESET_MODE_ONLY_MODEM\r\n");
		ModemManagerData.eModemResetMode = eMODEM_RESET_MODE_ONLY_MODEM;
		PlusDeviceResetCount();	//모뎀리셋때 증가시켜서 특정숫자 이상이면 모듈리셋 //20210412 확인필요
		//SetMngMdmState(STAT_MNG_MDM_IDLE);
		SetModemState(eMODEM_Idle);
        ModemManagerData.eModemResetMode = eMODEM_RESET_MODE_POWER_ON;
	}
	else {	
		SetModemState(eMODEM_Idle);

		if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_SOFTWARE) {
			ModemManagerData.eModemResetMode = eMODEM_RESET_MODE_SOFTWARE;
		}
		else {
			ModemManagerData.eModemResetMode = eMODEM_RESET_MODE_POWER_ON;
		}
		ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly = -1;
		ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly = -1;
		ModemManagerData.iTimer_MDM_Check_Network_Status_Dly = -1;
		ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly = -1;
		ModemManagerData.iTimer_MDM_Recieved_TimeoutDly = -1;
        ModemManagerData.iTimer_Rcv_AGPS_Data_TimeoutDly = -1;

		ClearDeviceResetCount();
	}

#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
	ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
#endif
	ModemManagerData.bModemRcvSysStartMessageFlag = false;
	ModemManagerData.bModemRcvPBReadyMessageFlag = false;

    ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_NONE;
    ModemManagerData.ePreviousMessageSendingType = eMESSAGE_TYPE_NONE;

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
	memset(ModemManagerData.aVendorName, 0x00, sizeof(ModemManagerData.aVendorName));
#endif
	memset(ModemManagerData.aProductName, 0x00, sizeof(ModemManagerData.aProductName));
	memset(ModemManagerData.aRevision, 0x00, sizeof(ModemManagerData.aRevision));
	memset(ModemManagerData.aARevision, 0x00, sizeof(ModemManagerData.aARevision));
	memset(ModemManagerData.aIMEI, 0x00, sizeof(ModemManagerData.aIMEI));
	memset(ModemManagerData.aPhoneNo, 0x00, sizeof(ModemManagerData.aPhoneNo));

	ModemManagerData.bRequestMessageCommFlag = false;

	ModemManagerData.eNetworkRegStatus = eNETWORK_REG_STATUS_IDLE;

	ModemManagerData.bRetryCommFlag = false;
	ModemManagerData.nRetryCommCount = 0;

	ModemManagerData.bNeedStartUpCheckSMSFlag = false;

	ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;

    ModemManagerData.bWriteReady = false;
	ModemManagerData.bConfirmWriteLen = false;
	ModemManagerData.bFinishWrite = false;
	ModemManagerData.bReadyReadData = false;
	ModemManagerData.bEndOfData = false;
	ModemManagerData.bServiceOpen = false;
	ModemManagerData.bExpectEndOfData = false;
	ModemManagerData.bRcvBinDataZeroFlag = false;

	ModemManagerData.bRunWorkaroundFlag = false;
	ModemManagerData.bRTCSettingCompleteFlag = false;

	ModemManagerData.bNeedModemHWResetFlag = false;

	gbStartInterceptModemRcvData = false;

	ModemManagerData.bAvailableModemCommFlag = false;

	gbGITSMSReceivedSuccessFlag = false;
	gbGITSMSEnablePatialFlag = false;
	gaGITSMSBufferLength = 0;

	if(ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_POWER_ON) {
		// Modem을 reset 하게 되면 HTTP setting 정보가 삭제 되므로 다시 접속을 해야 한다.
		Trace("reset: BkSram_ModemInfo.bHTTPSuccessServiceConnection\r\n");
		for(n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
			BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
		}
	}
	else if(ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_ONLY_MODEM) {
		// Modem을 reset 하게 되면 HTTP setting 정보가 삭제 되므로 다시 접속을 해야 한다.
		Trace("reset: BkSram_ModemInfo.bHTTPSuccessServiceConnection\r\n");
		for(n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
			BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
		}
	}
	else if(ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_SOFTWARE) {
	}

	Trace(" init modem timer\r\n");

	HalTimerClearSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
	HalTimerClearSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
	HalTimerClearSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);
	HalTimerClearSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);
	HalTimerClearSWTimer(ModemManagerData.iTimer_MDM_Recieved_TimeoutDly);
    HalTimerClearSWTimer(ModemManagerData.iTimer_Rcv_AGPS_Data_TimeoutDly);

	ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly = -1;
	ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly = -1;
	ModemManagerData.iTimer_MDM_Check_Network_Status_Dly = -1;
	ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly = -1;
	ModemManagerData.iTimer_MDM_Recieved_TimeoutDly = -1;
    ModemManagerData.iTimer_Rcv_AGPS_Data_TimeoutDly = -1;
/*
#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
	ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly = HalTimerSetSWTimer(MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, false);				// modem command를 전달하고 ok 응답받기까지 delay
#else
	ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly = HalTimerSetSWTimer(MDM_MSG_CIEV_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, true);				// modem command를 전달하고 ok 응답받기까지 delay
#endif
*/


#ifndef RF_COMMON_MODEM
    ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly = HalTimerSetSWTimer(MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, false);                // modem command를 전달하고 ok 응답받기까지 delay
#else
    ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly = HalTimerSetSWTimer(MDM_MSG_CIEV_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, true);             // modem command를 전달하고 ok 응답받기까지 delay
#endif



	ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly = HalTimerSetSWTimer(MDM_CMD_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_CMD_CallBack, false);				// modem command를 전달하고 ok 응답받기까지 delay
	ModemManagerData.iTimer_MDM_Check_Network_Status_Dly = HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, NULL, false);
	ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly = HalTimerSetSWTimer(10000, eSWTimer_ONESHOT, MDM_RcvTimeout_BinData_CallBack, false);

	ModemManagerData.iTimer_MDM_Recieved_TimeoutDly = HalTimerSetSWTimer(MDM_NORECIEVE_TIMEOUT, eSWTimer_ONESHOT,MDM_NoRecieve_Callback,false); //mod.pdh 220516 todo check modem state

    if( g_bYUJINSelftestFlag != true && g_bEnableModemDirectCommunication != true)
    {
        HalTimerStartSWTimer(ModemManagerData.iTimer_MDM_Recieved_TimeoutDly,eSWTimer_ONESHOT); //mod.pdh 220516 todo check modem state
    }

	ModemManagerData.bCheckNetworkStatusFlag = false;

	ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;

	ModemManagerData.bModemDataSaveToFlashFlag = false;				// false이면, modem에서 수신 받은 데이터를 internal flash에 저장하지 않는다.

	ModemManagerData.bNoUsimFlag = false; 
	return;
}

#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
//*******************************************************************************************************************
// Message Callback Function
//*******************************************************************************************************************
void MDM_NoResp_SysLoadingMessage_CallBack(void)
{
	Trace("\r\nMDM: No resp ^SYSLOADING\r\n");

	// Modem 구동 최초에 입력되는 "^SYSLOADING" message를 받지 못했다. Modem 연결이 불량 하거나 아니면, baudrate가 맞지 않을 수 있다.
	geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_REQ_SEARCHING;

	return;
}
#endif

void MDM_NoResp_SysStartMessage_CallBack(void)
{
	Trace("\r\nMDM: No resp ^SYSSTART\r\n");

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
	if( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_NOSERIAL_NUMBER) == 0 )
	{
		ModemManagerData.bModemRcvSysStartMessageFlag = true;
	}
	else
	{
		//geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_REQ_SEARCHING;	//RF Track module is not support Baudrate searching
		SetModemControlEvent(eMngMdmGeneralError);
	}
#endif

	g_stModem_Rep.eMDResponse = eMDResponseStateTimeOut;

	return;
}

void MDM_NoResp_SysStartMessage_AtWorkaround_CallBack(void)
{
	Trace("\r\nMDM: No resp ^SYSSTART, but error skip\r\n");

	ModemManagerData.bModemRcvSysStartMessageFlag = true;

	return;
}

void MDM_NoResp_PBREAD_CallBack(void)
{
	Trace("\r\nMDM: No +PBREAD\r\n");
    g_stModem_Rep.eMDResponse = eMDResponseStateTimeOut;

    SetModemControlEvent(eMngMdmGeneralError);

	return;
}

void MDM_NoResp_CMD_CallBack(void)
{
	Trace("\nMDM: @MDM_NoResp_CMD_CallBack()...\r\n");
    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);

	if(TestData.bEnableTestMode == true) {
		Trace("\nMDM: *Modem Response Time Out(test mode)...\r\n");
		g_stModem_Rep.start_rep = false;
		g_stModem_Rep.eMDResponse = eMDResponseStateTimeOut;
		return;
	}

    //  Trace("\nMDM: No CMD Response!!!\r\n");
    g_stModem_Rep.start_rep = false;
    //g_stModem_Rep.cmd_index = eCmd_None;
    g_stModem_Rep.eMDResponse = eMDResponseStateTimeOut;

	return;
}

void MDM_NoResp_URC_CallBack(void)
{
	Trace("\nMDM: @MDM_NoResp_URC_CallBack()...\r\n");
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);

	Trace("bConfirmWriteLen : %d\n, bFinishWrite : %d\r\n, bHTTPRcvPostURL : %d\n, bReadyReadData : %d\n, bHTTPRcvPostResponse : %d\n, nHTTPResponseCode : %d\r\n",
		   ModemManagerData.bConfirmWriteLen,
		   ModemManagerData.bFinishWrite,
		   ModemManagerData.bHTTPRcvPostURL,
		   ModemManagerData.bReadyReadData,
		   ModemManagerData.bHTTPRcvPostResponse,
		   ModemManagerData.nHTTPResponseCode);

	if(TestData.bEnableTestMode == true) {
//		Trace("\nMDM: *Modem Response Time Out(test mode)...\r\n");
		g_stModem_Rep.start_rep = false;
		g_stModem_Rep.eMDResponse = eMDResponseStateTimeOut;
		return;
	}

    g_stModem_Rep.start_rep = false;
    //g_stModem_Rep.cmd_index = eCmd_None;
    g_stModem_Rep.eMDResponse = eMDResponseStateTimeOut;

	return;
}

void MDM_RcvTimeout_BinData_CallBack(void)
{
	Trace("\nMDM: @MDM_RcvTimeout_BinData_CallBack()...\r\n");

	g_stModem_Rep.start_rep = false;
	g_stModem_Rep.cmd_index = eCmd_None;
	g_stModem_Rep.eMDResponse = eMDResponseStateTimeOut;

	return;
}

void MDM_AGPS_Send_Timeout_CallBack(void)
{
	Trace("\nMDM: @MDM_AGPS_Send_Timeout_CallBack()...\n");

	g_stModem_Rep.start_rep = false;
	g_stModem_Rep.cmd_index = eCmd_None;
	g_stModem_Rep.eMDResponse = eMDResponseStateTimeOut;
	ModemManagerData.nMdmSubState = 7;
	HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 2000, eSWTimer_ONESHOT, MDM_NoResp_CMD_CallBack, true);
	
	if( iTimer_AGPS_Send_TimeoutDly != -1 )
	{
		HalTimerClearSWTimer(iTimer_AGPS_Send_TimeoutDly);
		iTimer_AGPS_Send_TimeoutDly = -1;
	}
	return;
}


void MDM_SetNetworkRegistrationFlag_CallBack(void)
{
//	Trace("\nMDM: check network status\r\n");
	ModemManagerData.bCheckNetworkStatusFlag = true;

	return;
}

void MDM_ProcessTimeout_CallBack(void)
{
	if( ModemManagerData.eState == eMODEM_ENTER_SLEEP_MODE)
	{
		printf("MDM_ProcessTimeout_CallBack%d\r\n",ModemManagerData.nMdmSubState);
		SetRegModemProcessFunction(ProcessEnterModemPowerSaveMode, ProcessEnterModemPowerSaveModeResult, MODEM_RESPONSE_TYPE_COMMON);
		HalTimerClearSWTimer(g_iTimerProcessTimeoutCallback);
		g_iTimerProcessTimeoutCallback = -1;
	}
	return;
}

void MDM_SetFotaTestFlag_CallBack(void)
{
	Trace("\nMDM: Set Fota Test flag\r\n");

	return;
}

//mod.pdh 2022.05.16 to do check modem state
//2ºÐ°£ ¸ðµ© µ¿ÀÛ ¾øÀ» °æ¿ì, ¸ðµ© Reset 
void MDM_NoRecieve_Callback(void)
{
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Recieved_TimeoutDly); //mod.pdh 220516 todo check modem state
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);

	//WriteAutoLinkModemsStatusData(MODEM_NOT_COMMUNICATION);

	printf("********************************************\r\n");
	printf("********************************************\r\n");
	printf("********************************************\r\n");
	printf("**********MDM_NoRecieve_Callback************\r\n");
	printf("********************************************\r\n");
	printf("********************************************\r\n");
	printf("********************************************\r\n");

    SetModemState(eMODEM_READY);
	Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);

    SetWaitforSendingData(true);
    SetNetworkPostPoneFlag(false);
}

void MDM_AGPSTimeout_CallBack(void)
{
	Trace("MDM : Rcv AGPS Data Timeout\r\n");

	//need to close socket
	ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
	ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CLOSE;
	
	HalTimerClearSWTimer(ModemManagerData.iTimer_Rcv_AGPS_Data_TimeoutDly);
	ModemManagerData.iTimer_Rcv_AGPS_Data_TimeoutDly = -1;
}

//*******************************************************************************************************************

void MD_uDelay (const uint32_t usec)
{
  uint32_t count = 0;
  const uint32_t utime = (120 * usec / 7);

  do
  {
    if ( ++count > utime )
    {
      return ;
    }
  }
  while (1);
}

void MD_mDelay (const uint32_t msec)
{
  MD_uDelay(msec * 1000);
}

void SetModemState(eMODEM_STATE state)
{
	char *strState;

	switch (state) {
		case eMODEM_Idle:
			strState = "eMODEM_Idle";
			break;

#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
		case eMODEM_WAIT_RCV_SYSLOADING:
			strState = "eMODEM_WAIT_RCV_SYSLOADING";
			break;
#endif

		case eMODEM_WAIT_RCV_SYSSTART:
			strState = "eMODEM_WAIT_RCV_SYSSTART";
			break;

		case eMODEM_Wait_Message_PBREADY:
			strState = "eMODEM_Wait_Message_PBREADY";
			break;

		case eMODEM_READY:
			strState = "eMODEM_READY";
			break;

		case eMODEM_RUNNING_FOTA:
			strState = "eMODEM_RUNNING_FOTA";
			break;

		case eMODEM_CheckModemBaudrete:
			strState = "eMODEM_CheckModemBaudrete";
			break;

		case eMODEM_WAIT_MESSAGE_COMM_COMPLETE:
			strState = "eMODEM_WAIT_MESSAGE_COMM_COMPLETE";
			break;

		case eMODEM_CMD_ERROR:
			strState = "eMODEM_CMD_ERROR";
			break;

		case eMODEM_RunProcess:
			strState = "eMODEM_RunProcess";
			break;

		case eMODEM_ENTER_SLEEP_MODE:
			strState = "eMODEM_ENTER_SLEEP_MODE";
			break;

		case eMODEM_NOW_SLEEP_MODE:
			strState = "eMODEM_NOW_SLEEP_MODE";
			break;

        case eMODEM_PRODUCT_MODE:
			strState = "eMODEM_PRODUCT_MODE";
            break;

        case eMODEM_NO_USIM:
			strState = "eMODEM_NO_USIM";
            break;

		case eMODEM_NONE:
			strState = "eMODEM_NONE";
			break;

        case eMODEM_INIT_MODEM:
            strState = "eMODEM_INIT_MODEM";
            break;
        case eMODEM_CHECK_NETWORK_REGISTRATION:
            strState = "eMODEM_CHECK_NETWORK_REGISTRATION";
            break;
        case eMODEM_SETTING_TIME:
            strState = "eMODEM_SETTING_TIME";
            break;
        case eMODEM_EXT_INIT_MODEM:
            strState = "eMODEM_EXT_INIT_MODEM";
            break;
        case eMODEM_ENTER_SLEEP_MODE_INIT:
    	    strState = "eMODEM_ENTER_SLEEP_MODE_INIT";
    	    break;
		case eMODEM_SetupNetwork:
    	    strState = "eMODEM_SetupNetwork";
			break;
        default:
            strState = "Not defined";
            break;
	}

	if(ModemManagerData.eState != state)
	{
		PushModemStateCollection(ModemManagerState,ModemManagerData.eState);
	}
	
	ModemManagerData.eState = state;

	Trace(" ModemManager: %s(%d)\r\n", strState, state);

	return;
}

// [TEST] true 이면 자동 modem H/W reset 을 수행하지 않는다. (bench test 용)
bool g_bTestDisableModemReset = false;

void RequestModemReset()
{
    ModemManagerData.bNeedModemHWResetFlag = true;
    SetModemState(eMODEM_READY);
}

eMODEM_STATE GetModemState(void)
{
	return ModemManagerData.eState;
}

stModemProcess ModemProcess;

void SetRegModemProcessFunction(modem_process_fn processFunction, modem_process_fn resultFunction, MODEM_RESPONSE_TYPE eType)
{
	stModemProcess *p;

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

	p = &ModemProcess;

    p->modemProcessFunc = processFunction;
    p->modemProcessResultFunc = resultFunction;
    p->eResponseOption = eType;
//  p->pState = pState;
//  p->pNextState = pNextState;

//  *(p->pState) = 0;
//  *(p->pNextState) = 0;

	g_stModem_Rep.eMDResponse = eMDResponseStateWait;

	ModemManagerData.nMdmSubState = 0;
	ModemManagerData.nMdmSubNextState = 0;

	SetModemState(eMODEM_RunProcess);

  return;
}

boolean_t m_bRequstActionFromManager=false;
int32_t m_nRequestActionFromManager;

boolean_t SetRequestActionFromManager(int32_t nRequestActionFromManager, boolean_t bForce)
{
    if( m_bRequstActionFromManager == false || bForce == true )
    {
        m_nRequestActionFromManager = nRequestActionFromManager;
        m_bRequstActionFromManager = true;

        return true;
    }

//    if( m_nRequestActionFromManager == eMdmReqRssi ||
//        m_nRequestActionFromManager == eMdmReqNetworkTime )
//    {
//        m_nRequestActionFromManager = nRequestActionFromManager;
//        m_bRequstActionFromManager = true;
//    }


    return false;
}

boolean_t GetRequestActionFromManagerFlag()
{
    return m_bRequstActionFromManager;
}

int32_t GetRequestActionFromManager()
{
    return m_nRequestActionFromManager;
}

void ClearRequestActionFromManagerFlag()
{
    m_bRequstActionFromManager = false;
}


void ResetForceModem()
{
    int n;
	Trace(" *eMODEM_RESET(HW reset)\r\n");
	ModemManagerData.bAvailableModemCommFlag = false;

	ClearQueue(g_stGitCommInfo[eCOMM_TYPE_UART_MODEM].pstInQueue);
	ModemManagerData.eNetworkRegStatus = eNETWORK_REG_STATUS_IDLE;

	InitializeModemManager(true);

	g_stModem_Rep.start_rep = false;
	//g_stModem_Rep.cmd_index = eCmd_None;
	g_stModem_Rep.cmd_index = eCmd_SetEcho;

	for(n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
		BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
	}

	printf("PowerOnGemaltoModem_Modem_Reset_Power4\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
//	HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
//	APP_Delay(200);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
    APP_Delay(200);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
    APP_Delay(200);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
//  APP_Delay(200);
//	HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#else
    HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
    APP_Delay(20);


	HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
	APP_Delay(100);

    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
	APP_Delay(20);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
    APP_Delay(50);
    HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#endif

	ModemManagerData.bRunWorkaroundFlag = false;

    //MONI 2018-1-27
    // to disable postpond after reset we added this flag
    ModemManagerData.bNeedStartUpCheckSMSFlag = true;
}

unsigned long ulOldDeltaTime = 0;
unsigned long ulNewDeltaTime = 0;

void ModemManager(void)
{
	eMODEM_PROCESS_FUNC_RET eRet;

	switch(ModemManagerData.eState) {
		case eMODEM_Idle:
			geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_UNKNOWN;

			ProcessModemStartUp();
			break;

#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
		case eMODEM_WAIT_RCV_SYSLOADING:
			if(ModemManagerData.bModemRcvSysLoadingMessageFlag != true) {
				if(geChangeDefaultModemBaudFlag == eMODEM_BAUDRATE_SEARCHING_STATE_REQ_SEARCHING) {
					// Modem 구동 시, 최초 message 인 SYSLOADING을 입력 받지 못한 경우
					gnModemSearchBaudrateStepCount = 0;				
                  SetModemState(eMODEM_CheckModemBaudrete);
				}
				break;
			}

            m_iMdmMainState = ModemManagerData.eState;

			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, true);				// ^START, ^PBREADY message를 응답 받기까지의 delay
			SetModemState(eMODEM_WAIT_RCV_SYSSTART);
			break;
#endif

		case eMODEM_WAIT_RCV_SYSSTART:
			if(ModemManagerData.bModemRcvSysStartMessageFlag != true) {
				break;
			}

            m_iMdmMainState = ModemManagerData.eState;

			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
			//ModemManagerData.eState = eMODEM_INIT_MODEM;

            //if((ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_ONLY_MODEM || ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_POWER_ON) &&
            //    ModemManagerData.bModemRcvPBReadyMessageFlag == false )
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
            SetModemState(eMODEM_INIT_MODEM);
#else
            {
                ModemManagerData.bModemRcvPBReadyMessageFlag = false;
                HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_PBREADY_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_PBREAD_CallBack, true);
            }

            SetModemState(eMODEM_Wait_Message_PBREADY);
#endif
			break;

		case eMODEM_Wait_Message_PBREADY:

            // check sleep mode
            if( IsModemSleepWaitState() == true )
            {
                SetModemState(eMODEM_ENTER_SLEEP_MODE);
                return;
            }

			if(ModemManagerData.bModemRcvPBReadyMessageFlag == false) {
				break;
			}
			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
#if 0		// 2018.05.03 SPARROW : Modem manager init state 순서변경으로 PRODUCT_MODE로 설정하는 위치 이동
			// 2018.03.14 James Jean : 생산시에는 장비 유심을 사용하여서네트워크에 연결이 되지 않아
			// 모뎀 reset이 발생한다. 생산 후에 네트워크 상태를 체크 하도록 변경함.
			if( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_NOSERIAL_NUMBER) == 0 )
			{
				ModemManagerData.eState = eMODEM_PRODUCT_MODE;
			}
			else
#endif
			// 2018.09.04 SPARROW : 생산프로그램 AT^SCFG 명령 시 modem reset되는경우 있어 아래 조건문 추가
			if((g_bYUJINSelftestFlag == true) &&(g_arrYUJINTestIdx[0] == eSELFTEST_CURRENT_IDX))	SetModemState(eMODEM_PRODUCT_MODE);
			else																					SetModemState(eMODEM_INIT_MODEM);

            // initialized hardware
            SetModemControlEvent(eMngMdmInitializeHardware);

            m_iMdmMainState = ModemManagerData.eState;

			break;
        case eMODEM_INIT_MODEM:
#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);		// Bit_SET 후 11초 정도 뒤에 여기서 RESET
#endif
            m_iMdmMainState = ModemManagerData.eState;
            Trace(" Set & Run Modem initialize function\r\n");
            SetRegModemProcessFunction(ProcessInitializeModem, ProcessInitializeModemResult, MODEM_RESPONSE_TYPE_COMMON);
            break;

        case eMODEM_SetupNetwork:
            m_iMdmMainState = ModemManagerData.eState;
            Trace(" Set & Run network configure\r\n");
            SetRegModemProcessFunction(ProcessInitializeNetwork, ProcessInitializeNetworkResult, MODEM_RESPONSE_TYPE_COMMON);
            break;

		case eMODEM_CHECK_NETWORK_REGISTRATION:

            m_iMdmMainState = ModemManagerData.eState;
			Trace(" Set & Run check network registration function\r\n");
			SetRegModemProcessFunction(ProcessCheckNetwarkRegistration, ProcessCheckNetwarkRegistrationResult_First, MODEM_RESPONSE_TYPE_COMMON);
			break;

		case eMODEM_SETTING_TIME:

            m_iMdmMainState = ModemManagerData.eState;
	  	    Trace(" Set & Run setting time function\r\n");
    	    SetRegModemProcessFunction(ProcessSetModemTime, ProcessSetModemTimeResult, MODEM_RESPONSE_TYPE_COMMON);
			break;

		case eMODEM_EXT_INIT_MODEM:

            m_iMdmMainState = ModemManagerData.eState;
			Trace(" Set & Run modem extended initialize function\r\n");
			SetRegModemProcessFunction(ProcessExtInitModem, ProcessExtInitializeModemResult, MODEM_RESPONSE_TYPE_COMMON);
			break;

		case eMODEM_READY:
        {
            stMsgMdmHandler stReqMessage;

            // check sleep mode
            if( IsModemSleepWaitState() == true )
            {
                SetModemState(eMODEM_ENTER_SLEEP_MODE);
                return;
            }

            //if( ModemManagerData.bRequestMessageCommFlag == false )
            {
#ifndef GLOBAL_SHARE_QUEUE //Get, check
                if( MngQueueGetMessage(ID_MNG_QUEUE_MDMH, (int8_t*)&stReqMessage, sizeof(stMsgMdmHandler)) == true )
#else
				if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_MDMH, (uint8_t*)&stReqMessage.header, sizeof(stMsgHeader), (uint8_t*)&stReqMessage.carReport, sizeof(stCarReport)) == true )
#endif
                {
                    Trace("MainExternal event : %d, subEvent : %d\r\n",stReqMessage.header.event,stReqMessage.header.subEvent);

                    switch(stReqMessage.header.subEvent)
                    {
                        case eMdmReqGemaltoReset:

                            ResetForceModem();
                            Trace(" start modem manager_1\r\n");

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
							HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_CIEV_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, true);				// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^START, ^PBREADY message를 응답 받기까지의 delay
                            SetModemState(eMODEM_WAIT_RCV_SYSSTART);
#else
							HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, true);				// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^START, ^PBREADY message를 응답 받기까지의 delay
                            SetModemState(eMODEM_WAIT_RCV_SYSLOADING);
							ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
#endif

                            ModemManagerData.bModemRcvSysStartMessageFlag = false;
                            ModemManagerData.bModemRcvPBReadyMessageFlag = false;

                            extern int32_t m_nReqWaitNetworkStableTimeoutTimerID;
                            DisableSystemMessageTimer(m_nReqWaitNetworkStableTimeoutTimerID);

                            //ModemManagerData.bNeedModemHWResetFlag = true;
                            break;
                        case eMdmReqGemaltoWorkAround:
                            ModemManagerData.bRunWorkaroundFlag = true;
                            break;
                        case eMdmReqNetworkTime:
                            if( IsGemaltoAvailable() == false )
                            {
                                //if(Md_GetSIND("\"nitz\",2") == true)
                                {
                                    //Trace("reqeust network time to gemalto\r\n");
                                    ModemManagerData.bAvailableModemCommFlag = false;
                                    ModemManagerData.bRequestNetworkTime = true;
                                }
                            }
                            break;
                        case eMdmReqRssi:
                            if( IsGemaltoAvailable() == false )
                            {
                                //if(Md_CheckSignalQuality() == true){
                                    //Trace("reqeust rssi to gemalto\r\n");
                                    ModemManagerData.bCheckNetworkStatusFlag = true;
                                    ModemManagerData.bAvailableModemCommFlag = false;
                                //}
                            }
                            break;
                        case eMdmReqDonothing:
                            SetModemState(eMODEM_NONE);
                            break;
                        case eMdmReqReInit:
                            SetModemState(eMODEM_INIT_MODEM);
                            break;
                        default:
                            break;
                    }
                }
            }
            //
            if( ModemManagerData.bRequestMessageCommFlag == false &&
                GetRequestActionFromManagerFlag() == true )
            {
                // reset modem
                if( GetRequestActionFromManager() == eMdmReqGemaltoReset)
                {
                    ModemManagerData.bNeedModemHWResetFlag = true;
                }
                else if( GetRequestActionFromManager() == eMdmReqGemaltoWorkAround )
                {
                    ModemManagerData.bRunWorkaroundFlag = true;
                }
                else if( GetRequestActionFromManager() == eMdmReqNetworkTime )
                {
                    if( IsGemaltoAvailable() == false )
                    {
                        //if(Md_GetSIND("\"nitz\",2") == true)
                        {
                            //Trace("reqeust network time to gemalto\r\n");
                            ModemManagerData.bAvailableModemCommFlag = false;
                            ModemManagerData.bRequestNetworkTime = true;
                        }
                    }
                }
                else if( GetRequestActionFromManager() == eMdmReqRssi )
                {
                    if( IsGemaltoAvailable() == false )
                    {
                        //if(Md_CheckSignalQuality() == true){
                            //Trace("reqeust rssi to gemalto\r\n");
                            ModemManagerData.bCheckNetworkStatusFlag = true;
                            ModemManagerData.bAvailableModemCommFlag = false;
                        //}
                    }
                }
                else if( GetRequestActionFromManager() == eMdmReqDonothing )
                {
                    SetModemState(eMODEM_NONE);
                }
                else if( GetRequestActionFromManager() == eMdmReqReInit )
                {
                    SetModemState(eMODEM_INIT_MODEM);
                }

                ClearRequestActionFromManagerFlag();
            }

			//*****************************************************************************************
			// modem H/W reset 수행
			//*****************************************************************************************
			if(ModemManagerData.bNeedModemHWResetFlag == true) {
				ModemManagerData.bNeedModemHWResetFlag = false;

				// [TEST] modem reset 을 disable 한 경우, 요청을 무시한다.
				if(g_bTestDisableModemReset == true) {
					Trace("# Modem Hardware Reset SKIPPED (test mode)\r\n");
					break;
				}

				ModemManagerData.bAvailableModemCommFlag = false;

				Trace("_____________________________________\r\n");
                Trace("# Modem Hardware Reset\r\n");
				SetRegModemProcessFunction(ProcessModemHWReset, ProcessModemHWResetResult, MODEM_RESPONSE_TYPE_COMMON);

                // increase retry count;
                IncreaseModemResetRetryCount();
				break;
			}
			//*****************************************************************************************

			//*****************************************************************************************
			// modem error 발생 시, airplane mode 로 진입 한후 재 기동 한다.
			//*****************************************************************************************
            if(ModemManagerData.bRunWorkaroundFlag == true) {
				Trace(" Set & Run workaround\r\n");
                ModemManagerData.bRunWorkaroundFlag = false;
				ModemManagerData.bAvailableModemCommFlag = false;

				SetRegModemProcessFunction(ProcessCMEErrorWorkaround, ProcessCMEErrorWorkaroundResult, MODEM_RESPONSE_TYPE_COMMON);
                break;
            }
			//*****************************************************************************************

            if( ModemManagerData.bRequestMessageCommFlag == false &&
                ModemManagerData.bRequestAgpsCommFlag == false &&
                ModemManagerData.eFOTAStartState == eFOTA_START_STATE_STOP )
            {
    			//*****************************************************************************************
    			// network 접속 여부 및 RSSI 신호 세기를 check 한다.
    			//*****************************************************************************************
    			if(ModemManagerData.bCheckNetworkStatusFlag == true) {
    				ModemManagerData.bCheckNetworkStatusFlag = false;

                    if( m_bFirstModemBoot == true )
                    {
                        if( IsModemConnected() == true )
                        {
                            m_bFirstModemBoot = false;
                        }
                    }
                    else
                    {
        //				Trace(" Set & Run check modem status function\r\n");
        				//SetRegModemProcessFunction(ProcessCheckNetwarkStatus, ProcessCheckNetwarkStatusResult, MODEM_RESPONSE_TYPE_COMMON);
        				SetRegModemProcessFunction(ProcessCheckRssi, ProcessCheckRssiResult, MODEM_RESPONSE_TYPE_COMMON);
                    }
    				break;
    			}
                //*****************************************************************************************
    			// request network time
    			//*****************************************************************************************
    			else if(ModemManagerData.bRequestNetworkTime == true) {
                    ModemManagerData.bRequestNetworkTime = false;
//                    Trace(" Set & Run request network time\r\n");
                    SetRegModemProcessFunction(ProcessGetModemTime,ProcessGetModemTimeResult, MODEM_RESPONSE_TYPE_COMMON);
                    break;
    			}

				//*****************************************************************************************
				// SMS check
				//*****************************************************************************************
				else if(ModemManagerData.bAvailableModemCommFlag == true) {
					if(ModemManagerData.bNeedStartUpCheckSMSFlag == true /*&& GetReqWaitNetworkCheck() == false */) {				// SMS 검사
						ModemManagerData.bNeedStartUpCheckSMSFlag = false;

                        Send2MngModem(eMngModem,eDummy, 0,(stCarReport *)NULL, 0);

                        //disable network check
                        SetReqWaitNetworkCheck(true);
						Trace(" Set & Run check SMS function\r\n");
						SetRegModemProcessFunction(ProcessCheckSMS, ProcessCheckSMSResult, MODEM_RESPONSE_TYPE_COMMON);
						break;
					}
				}
            }
			//*****************************************************************************************

			if(ModemManagerData.eFOTAStartState == eFOTA_START_STATE_STOP) {				// FOTA 동작 중일때는 SMS check 와 주기 정보 전송을 막아주자.
				//*****************************************************************************************

				//*****************************************************************************************
				// 주기 정보를 서버에 전송한다.
				//*****************************************************************************************
	            if(ModemManagerData.bAvailableModemCommFlag == true &&
                    ModemManagerData.bRequestMessageCommFlag == true &&
                    GetReqWaitNetworkCheck() == false )
                {
	                Trace(" Send Message\r\n");
					ModemManagerData.bRequestMessageCommFlag = false;
                    SetReqWaitNetworkCheck(true);
					SetRegModemProcessFunction(ProcessHTTPComm, ProcessHTTPCommResult_Message, MODEM_RESPONSE_TYPE_MESSAGE);
					break;
				}

                //*****************************************************************************************
				// Request AGPS Data to Server
				//*****************************************************************************************
	            if(ModemManagerData.bAvailableModemCommFlag == true &&
                    ModemManagerData.bRequestAgpsCommFlag == true &&
                    GetReqWaitNetworkCheck() == false )
                {
	                Trace(" Send Message\r\n");
					ModemManagerData.bRequestAgpsCommFlag = false;
                    SetReqWaitNetworkCheck(true);

                    DeleteAgpsData(AUTOLINK_AGPS_DATA);

					ModemManagerData.iTimer_Rcv_AGPS_Data_TimeoutDly = HalTimerSetSWTimer(MDM_MSG_AGPS_RCVDATA_TIMEOUT, eSWTimer_ONESHOT, MDM_AGPSTimeout_CallBack, true);
				
					SetRegModemProcessFunction(ProcessHTTPComm, ProcessHTTPCommResult_AGPSGetData, MODEM_RESPONSE_TYPE_MESSAGE);
					break;
				}
			}
			if(ModemManagerData.bAvailableModemCommFlag == true)
            {
				//*****************************************************************************************
				// update file의 version 정보를 가져온다.
				//*****************************************************************************************
				if(ModemManagerData.eFOTAStartState == eFOTA_START_STATE_GET_VERSION) {
					FOTA_GetVersion();

					Trace(" Set & Run FOTA(version info)\r\n");
#if defined (OLD_FOTA)
					FOTA_MakeRequestVersionContent();
#else
					NEWFOTA_MakeRequestVersionContent();
#endif
					SetRegModemProcessFunction(ProcessHTTPComm, ProcessHTTPCommResult_FOTAGetVer, MODEM_RESPONSE_TYPE_FOTA);
				}

				if(ModemManagerData.eFOTAStartState == eFOTA_START_STATE_GET_BIN) {
					FOTA_GetBin();

					Trace(" Set & Run FOTA(bin)\r\n");
					g_FotaManagerData.eState = eFOTA_STATE_INIT;
					SetModemState(eMODEM_RUNNING_FOTA);
				}

				if(ModemManagerData.eFOTAStartState == eFOTA_START_STATE_GET_VEHICLE_INFO) {
					FOTA_GetVehicleInfo();

					Trace(" Set & Run FOTA(Vehicle info)\r\n");
#if defined (OLD_FOTA)
					FOTA_MakeRequestVehicleInfo();
#else
					NEWFOTA_MakeRequestVehicleInfo();
#endif
					SetRegModemProcessFunction(ProcessHTTPComm, ProcessHTTPCommResult_GetVehicleInfo, MODEM_RESPONSE_TYPE_FOTA);
				}
			}
			break;
		}
        break;
		case eMODEM_RUNNING_FOTA:
			eRet = FOTA_RecvBinProcess();
			if(eRet != eMODEM_PROCESS_FUNC_RET_CONTINUE) {
				switch(eRet) {
					case eMODEM_PROCESS_FUNC_RET_OK:
                        SetModemControlEvent(eMngMdmRequestSendResult_Success);
						break;

					case eMODEM_PROCESS_FUNC_RET_FAIL:
						m_bPostponeReqSleep = false;
                        SetModemControlEvent(eMngMdmRequestSendResult_Fail);
						break;
				}

				ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;
				HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);	SetModemState(eMODEM_READY);

				MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

				SetModemState(eMODEM_READY);
			}
			break;
        case eMODEM_ENTER_POWER_OFF:
            Trace(" Set & Run enter modem power off mode\r\n");
            SetRegModemProcessFunction(ProcessModemPowerOff, ProcessModemPowerOffResult, MODEM_RESPONSE_TYPE_COMMON);
            break;
        case eMODEM_ENTER_SLEEP_MODE_INIT:
            m_iMdmMainState = ModemManagerData.eState;
			Trace(" Set & Run modem sleep extended initialize function\r\n");
			SetRegModemProcessFunction(ProcessSleepExtInitModem, ProcessSleepExtInitializeModemResult, MODEM_RESPONSE_TYPE_COMMON);
            break;
		case eMODEM_ENTER_SLEEP_MODE:
			if( ModemManagerData.nMdmSubState == MDM_SUB_STATE_END_PROCESS || ModemManagerData.nMdmSubState == 0 )	//RSSI 체크 중 POWER SAVE 진입 시 CME ERROR 발생
			{
				Trace(" Set & Run enter modem sleep mode\r\n");
				SetRegModemProcessFunction(ProcessEnterModemPowerSaveMode, ProcessEnterModemPowerSaveModeResult, MODEM_RESPONSE_TYPE_COMMON);
			}
			else
			{
				if( g_iTimerProcessTimeoutCallback == -1 )
				{
					g_iTimerProcessTimeoutCallback = HalTimerSetSWTimer(MDM_CHECK_PROCESS_TIMEOUT_FOR_POWERSAVE, eSWTimer_ONESHOT, MDM_ProcessTimeout_CallBack, TRUE);
				}
			}
			break;
		case eMODEM_NOW_SLEEP_MODE:
            if(GetWakeupDetectPinState(eWAKE_PIN_BT_MON) == true) 
            {
                ModemManagerData.eModemResetMode = eMODEM_RESET_MODE_SOFTWARE;
                SetModemState(eMODEM_Idle);
                ModemInitalize();
            }
			break;
		case eMODEM_CheckModemBaudrete:
			// modem 의 baudrete를 검사한다.
			// 115200 -> 921600 순서로 검사한다.
			SetRegModemProcessFunction(ProcessCheckModemBaudrate, ProcessCheckModemBaudrateResult, MODEM_RESPONSE_TYPE_BAURDRATE);
			break;
		case eMODEM_CMD_ERROR:
			break;
		case eMODEM_RunProcess:
			eRet = ModemProcess.modemProcessFunc();
			if(eRet == eMODEM_PROCESS_FUNC_RET_OK) {
//				Trace(" process ending\r\n");

				if(ModemProcess.modemProcessResultFunc != NULL) {
					HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
					HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);

					ModemProcess.modemProcessResultFunc();

                    if( ModemManagerData.bNeedModemHWResetFlag == true )
                    {
                        printf("================================================\r\n");
                        printf("before sleep sms check is not correct\r\n");
                        SetModemState(eMODEM_READY);
                    }
				}

				break;
			}
			else if(eRet == eMODEM_PROCESS_FUNC_RET_FAIL) {
				if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE) {
                  ModemManagerData.bRequestMessageCommFlag = false;
				}
				else if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_FOTA) {
					ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;
				}
                else if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_BAURDRATE ) {
                    ModemManagerData.bNeedModemHWResetFlag = true;
                }
#ifdef RF_COMMON_MODEM
                else if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_AGPS)
                {
                    SetReqWaitNetworkCheck(false);
                }
#endif

				HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);

                // check who is failed
                SetModemControlEvent(eMngMdmRequestSendResult_Fail_General);
				SetModemState(eMODEM_READY);

				break;
			}

            // this process according to the recevice message from gemalto modem.
            ProcessMDResponse();

			break;

		case eMODEM_NONE:
            // check sleep mode
            if( IsModemSleepWaitState() == true )
            {
                SetModemState(eMODEM_ENTER_SLEEP_MODE);
                return;
            }

            if( GetRequestActionFromManagerFlag() == true )
            {
                // reset modem
                if( GetRequestActionFromManager() == eMdmReqGemaltoReset)
                {
                    ModemManagerData.bNeedModemHWResetFlag = true;
                    SetModemState(eMODEM_READY);
                }
                else if( GetRequestActionFromManager() == eMdmReqGemaltoWorkAround )
                {
                    ModemManagerData.bRunWorkaroundFlag = true;
                    SetModemState(eMODEM_READY);
                }
                else if( GetRequestActionFromManager() == eMdmReqNetworkTime )
                {
                    if(Md_GetSIND("\"nitz\",2") == true) {
                        //Trace("reqeust network time\r\n");
                    }
                }
                else if( GetRequestActionFromManager() == eMdmReqRssi )
                {
                    ModemManagerData.bCheckNetworkStatusFlag = true;
                    SetModemState(eMODEM_READY);
                }
                else if( GetRequestActionFromManager() == eMdmReqDonothing )
                {
                    SetModemState(eMODEM_NONE);
                }
                else if( GetRequestActionFromManager() == eMdmReqReInit )
                {
                    SetModemState(eMODEM_INIT_MODEM);
                }

                ClearRequestActionFromManagerFlag();
            }
			break;
		case eMODEM_PRODUCT_MODE:
    		{
                // check sleep mode
                if( IsModemSleepWaitState() == true )
                {
                    SetModemState(eMODEM_ENTER_SLEEP_MODE);
                    return;
                }

    			// 2018.09.04 SPARROW : 생산에서 Usim 등록하면서 Phone number 등록함 (그전까지 번호 없음)
    //			static int s_iProductStep = 0;
    //			switch( s_iProductStep )
    //			{
    //			case 0 :
    //				Md_GetPhoneNo();
    //				s_iProductStep = 2;
    //				break;
    //			case 1:
    //				Md_CheckNetworkRegistration();
    //				s_iProductStep = 3;
    //				break;
    //			case 2:
    //				APP_Delay(300);
    //				if(g_stModem_Rep.start_rep != true)
    //				{	s_iProductStep = 1;	}
    //				break;
    //			case 3:
    //				APP_Delay(300);
    //				s_iProductStep = 4;
    //				break;
    //			case 4:
    //
    //				s_iProductStep = 5;
    //				break;
    //			case 5:
    //				// to do nothing
    //                break;
    //			}

    		}
		    break;
        case eMODEM_NO_USIM:
        {
            static unsigned long s_ulBlinkTime = 0;
            static unsigned int s_uiCount = 0;
            
            ModemManagerData.bNoUsimFlag = true;
            // check sleep mode
            if( IsModemSleepWaitState() == true )
            {
                SetModemState(eMODEM_ENTER_SLEEP_MODE);
                return;
            }

            if( Get_TmrDelta(Get_Tmr(), s_ulBlinkTime) > 500 )
            {
                if ( s_uiCount % 3 == 0 )
                {
                    SetLedOnOffCtl(LED_ON, eLED_CAN);
                    SetLedOnOffCtl(LED_OFF, eLED_GPS);
                    SetLedOnOffCtl(LED_OFF, eLED_SERVER);
                }
                else if ( s_uiCount % 3 == 1 )
                {
                    SetLedOnOffCtl(LED_OFF, eLED_CAN);
                    SetLedOnOffCtl(LED_ON, eLED_GPS);
                    SetLedOnOffCtl(LED_OFF, eLED_SERVER);
                }
                else if ( s_uiCount % 3 == 2 )
                {
                    SetLedOnOffCtl(LED_OFF, eLED_CAN);
                    SetLedOnOffCtl(LED_OFF, eLED_GPS);
                    SetLedOnOffCtl(LED_ON, eLED_SERVER);
                }

                s_uiCount++;
                s_ulBlinkTime = Get_Tmr();

                if( s_uiCount == (1*60*1000/500)%500 )
                {
                    //reset modem
                    ResetForceModem();
                    Trace(" start modem manager\r\n");
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
                    SetModemState(eMODEM_WAIT_RCV_SYSSTART);
#else
                    SetModemState(eMODEM_WAIT_RCV_SYSLOADING);
#endif
                }
            }
        }
            break;
		default:
			break;
	}
}

void ProcessMDResponse()
{
    switch(g_stModem_Rep.eMDResponse)
    {
    	case eMDResponseStateOK:
    		ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
    		g_stModem_Rep.eMDResponse = eMDResponseStateWait;
    		break;

    	case eMDResponseStateFAIL:
            Trace("====================================================\r\n");
    		Trace(" command fail, cmd index: %d\r\n", g_stModem_Rep.cmd_index);

            if ( g_stModem_Rep.cmd_index == eCmd_GetIMSI ) // 2018/03/22 James Jean : USIM이 없는 상태는 모뎀 동작 하지 않도록 수정
            {
                SetModemState(eMODEM_NO_USIM);
                break;
            }

    		if( ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_FOTA || ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE)
            {
    			Trace(" Receive \"ERROR\"\r\n");
    			//ModemManagerData.bNeedModemHWResetFlag = true;
    			//SetModemState(eMODEM_READY);
                // MONI 2018-03-02
                // to control one handler we forwarding this message
                MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_MODEM_CMD_FAIL;

                // terminate state in the precedure
                ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    		}
#ifdef RF_COMMON_MODEM
            else if(ModemProcess.modemProcessFunc == ProcessSendAGPSData)
            {
                //SetReqWaitNetworkCheck(false);
    			//SetModemState(eMODEM_READY);				// modem 응답 timeout이 발생하면 바로 eMODEM_READY state로 전환하고 modem reset을 수행한다.
    			//SetModemControlEvent(eMngMdmGeneralError);
    			break;
    		}
#endif
    		else
            {
    			Trace(" Receive \"ERROR\"\r\n");
    			//ModemManagerData.bNeedModemHWResetFlag = true;
    			SetModemState(eMODEM_READY);


                // send modem error status
                SetModemControlEvent(eMngMdmGeneralError);
    		}

    		break;

    	case eMDResponseStateNoCarrier:
            Trace("====================================================\r\n");
    		Trace(" no carrier, cmd index: %d, [%s]\r\n", g_stModem_Rep.cmd_index, g_tbModemCmdList[g_stModem_Rep.cmd_index].strSendDataReq);

            if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_FOTA || ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE)
            {
                // terminate state in the precedure
    			if( ModemManagerData.bServiceOpen == true )
                {
                    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
    			}
    			else
                {
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    			}
            }
    		if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_FOTA || ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE) {
    			ModemManagerData.bRetryCommFlag = true;
    			if(ModemManagerData.nMdmSubState < eHTTP_COMM_STATE_SOCKET_OPEN) {
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    			}
    			else {
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
    			}
    		}
    		else {
    			Trace(" Receive \"ERROR\"\r\n");
    			//ModemManagerData.bNeedModemHWResetFlag = true;
    			SetModemState(eMODEM_READY);

                // send modem error status
                SetModemControlEvent(eMngMdmGeneralError);
    		}
    		break;

    	case eMDResponseStateTimeOut:
            Trace("====================================================\r\n");
    		Trace(" Time out, cmd index: %d\r\n", g_stModem_Rep.cmd_index);
    		if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_FOTA)
            {
                //ModemManagerData.bNeedModemHWResetFlag = true;
    			g_FotaManagerData.eFotaResultCode = eFOTA_RESULT_CODE_NO_RESP;

                //MONI 2018-1-19
                // if during the fota no response occurred,
                // this make modem no response about last message
                //ModemManagerData.bNeedModemHWResetFlag = true;
                SetModemControlEvent(eMngMdmRequestSendResult_Fail);

                // MONI 20180710 fota exception
                if( ModemManagerData.eFOTAStartState > eFOTA_START_STATE_STOP )
                {
                    Trace("Fota is stop by MDM_NoResp_URC_CallBack\r\n");
                    ResetForceModem();
                }

                SetModemState(eMODEM_READY);                // modem 응답 timeout이 발생하면 바로 eMODEM_READY state로 전환하고 modem reset을 수행한다.
    		}
    		else if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE)
            {
    			MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_NO_RESP;

                // terminate state in the precedure
    			//if( ModemManagerData.bServiceOpen == true )
                //{
                //    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
    			//}
    			//else
                //{
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    			//}
    		}
#ifdef RF_COMMON_MODEM
            else if(ModemProcess.modemProcessFunc == ProcessSendAGPSData)
            {
                //SetReqWaitNetworkCheck(false);
    			//SetModemState(eMODEM_READY);				// modem 응답 timeout이 발생하면 바로 eMODEM_READY state로 전환하고 modem reset을 수행한다.
    			//SetModemControlEvent(eMngMdmGeneralError);
    			break;
    		}
#endif
            else
            {
                // send modem error status
                SetModemControlEvent(eMngMdmGeneralError);

                //ModemManagerData.bNeedModemHWResetFlag = true;
                SetModemState(eMODEM_READY);                // modem 응답 timeout이 발생하면 바로 eMODEM_READY state로 전환하고 modem reset을 수행한다.
            }

    		break;

    	case eMDResponseStateCMEError:
            Trace("====================================================\r\n");

			//WriteAutoLinkModemsStatusData(MODEM_CME_ERROR);

			// 180723 SPARROW : 생산테스트 시 Error 응답 들어오는 경우 있어 추가
			if(g_bYUJINSelftestFlag == true) {
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
				break;
			}

			if(ModemProcess.modemProcessFunc == ProcessInitializeModem)
            {
    			if(ModemManagerData.eCMEErrorCode == eCME_ERROR_NO_SIM_NOT_INSERTED)
                {
                    Trace("eCME_ERROR_NO_SIM_NOT_INSERTED\r\n");
                    //ModemManagerData.bNeedModemHWResetFlag = true;
                    //SetModemState(eMODEM_NONE);                // modem 응답 timeout이 발생하면 바로 eMODEM_READY state로 전환하고 modem reset을 수행한다.
                    //SetModemControlEvent(eMngMdmDoNothing);
                    SetModemState(eMODEM_NO_USIM);
                    return;
    			}
    		}
    		else if(ModemProcess.modemProcessFunc == ProcessCMEErrorWorkaround)
            {
    			//ModemManagerData.bNeedModemHWResetFlag = true;
    			SetModemState(eMODEM_READY);				// modem 응답 timeout이 발생하면 바로 eMODEM_READY state로 전환하고 modem reset을 수행한다.
    			SetModemControlEvent(eMngMdmGeneralError);
    			break;
    		}
#ifdef RF_COMMON_MODEM
            else if(ModemProcess.modemProcessFunc == ProcessSendAGPSData)
            {
                //SetReqWaitNetworkCheck(false);
    			//SetModemState(eMODEM_READY);				// modem 응답 timeout이 발생하면 바로 eMODEM_READY state로 전환하고 modem reset을 수행한다.
    			//SetModemControlEvent(eMngMdmGeneralError);
    			break;
    		}
#endif

    		if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_FOTA)
            {
    			g_FotaManagerData.eFotaResultCode = eFOTA_RESULT_CODE_CME_ERROR;

    			// terminate state in the precedure
    			//if( ModemManagerData.bServiceOpen == true )
                //{
                //    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
    			//}
    			//else
                //{
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    			//}

                SetModemControlEvent(eMngMdmRequestSendResult_Fail);
    		}
    		else if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE)
            {
    			MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_CME_ERROR;

    			// terminate state in the precedure
    			//if( ModemManagerData.bServiceOpen == true )
                //{
                //    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
    			//}
    			//else
                //{
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    			//}

                break;
    		}

    		if(ModemManagerData.eCMEErrorCode == eCME_ERROR_NO_OPERATION_NOT_ALLOWED)
            {
    			//ModemManagerData.bNeedModemHWResetFlag = true;				// modem reset을 수행한다.
    			SetModemState(eMODEM_READY);
                SetModemControlEvent(eMngMdmCriticalError);
    		}
    		else if(ModemManagerData.eCMEErrorCode == eCME_ERROR_NO_OPERATION_TEMPORARY_NOT_ALLOWED)
            {
    			//ModemManagerData.bNeedModemHWResetFlag = true;				// modem reset을 수행한다.
    			SetModemState(eMODEM_READY);
                SetModemControlEvent(eMngMdmCriticalError);
    		}
    		else
            {
    			//ModemManagerData.bRunWorkaroundFlag = true;
                SetModemState(eMODEM_READY);
                SetModemControlEvent(eMngMdmCriticalError);
    		}

    		break;

    	case eMDResponseState_ModemRegDenied:				// 망 접속 거부 message가 수신 되면, system을 reset 한다.
        	Trace("====================================================\r\n");
    		Trace(" Modem H/W Reset\r\n");
            if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE)
            {
                MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_NOT_NWETWORK_REGISTERED;

    			// terminate state in the precedure
    			//if( ModemManagerData.bServiceOpen == true )
                //{
                //    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
    			//}
    			//else
                //{
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    			//}
            }
            else
            {
        		//ModemManagerData.bNeedModemHWResetFlag = true;
        		SetModemState(eMODEM_READY);
                SetModemControlEvent(eMngMdmCriticalError);
                Trace("======================================================\r\n");
                Trace("======================================================\r\n");
                Trace("======================================================\r\n");
                Trace("Network Denied Errorr \r\n");
                Trace("======================================================\r\n");
            }
    		break;

    	case eMDResponseState_ModemWorkaround:
            Trace("====================================================\r\n");
            if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE)
            {
                MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_NOT_NWETWORK_REGISTERED;

                // terminate state in the precedure
    			//if( ModemManagerData.bServiceOpen == true )
                //{
                //    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
    			//}
    			//else
                //{
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    			//}
            }
            else
            {
        		ModemManagerData.bRunWorkaroundFlag = true;
        		SetModemState(eMODEM_READY);
                SetModemControlEvent(eMngMdmGeneralError);
            }

    		break;
        case eMDResponseState_DoNothing:
            Trace("====================================================\r\n");
            // wait other event from modem manager
            if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE)
            {
                MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_WAIT;

                // terminate state in the precedure
    			//if( ModemManagerData.bServiceOpen == true )
                //{
                //    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
    			//}
    			//else
                //{
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    			//}
            }
            else
            {
                SetModemState(eMODEM_NONE);
            }
            break;
		case eMDResponseState_NetworkError:
			Trace("====================================================\r\n");
    		Trace(" network error, cmd index: %d\r\n", g_stModem_Rep.cmd_index);

    		if(ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_FOTA || ModemProcess.eResponseOption == MODEM_RESPONSE_TYPE_MESSAGE) {
    			ModemManagerData.bRetryCommFlag = true;
    			if(ModemManagerData.nMdmSubNextState < eHTTP_COMM_STATE_WAIT_OPEN_SUCCESS) {
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_IDLE;
    			}
    			else {
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
    			}
    		}
    		else {
    			Trace(" Receive \"ERROR\"\r\n");
    			SetModemState(eMODEM_READY);
                SetModemControlEvent(eMngMdmGeneralError);
    		}
			break;
    	default:
    		break;
    }
}

eMODEM_PROCESS_FUNC_RET ProcessModemStartUp(void)
{
	if(ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_POWER_ON) {				// system reset, modem reset 모두 수행된 경우
		// modem rst 단에 NPN TR이 적용되어있다. 따라서, High/Low 신호를 반대로 해야 한다.
		HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);

		Trace(" eMODEM_RESET_MODE_POWER_ON\r\n");
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
		if(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly == -1)
			ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly = HalTimerSetSWTimer(MDM_MSG_CIEV_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, true);
		else
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, true);

        ModemManagerData.bModemRcvSysStartMessageFlag = false;
        SetModemState(eMODEM_WAIT_RCV_SYSSTART);
#else
		HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, true);				// ^START, ^PBREADY message를 응답 받기까지의 delay
        ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
        SetModemState(eMODEM_WAIT_RCV_SYSLOADING);
#endif
			}
	else if(ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_ONLY_MODEM) {				// modem 만 reset 된 경우
		Trace(" eMODEM_RESET_MODE_ONLY_MODEM\r\n");
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
		HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, true);				// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^START, ^PBREADY message를 응답 받기까지의 delay
#else
		HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, true);				// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^START, ^PBREADY message를 응답 받기까지의 delay
#endif

//      MONI 2018-03-05
//      Not used code
//		// 전송해야할 event key가 있는지 확인한다.
//		if(GetVehicleEventKey() != eMESSAGE_EVENT_KEY_NONE) {
//			MessageManagerData.bRequestEventMsgFlag_Alarm = true;
//		}

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
		ModemManagerData.bModemRcvSysStartMessageFlag = false;
		SetModemState(eMODEM_WAIT_RCV_SYSSTART);
#else
		ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
		SetModemState(eMODEM_WAIT_RCV_SYSLOADING);
#endif

	}
	else if(ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_SOFTWARE) {				// system reset만 된 경우, modem은 reset을 하지 않았다.
		Trace(" eMODEM_RESET_MODE_SOFTWARE\r\n");
		// standby mode에서 wakeup 된 경우

#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
        ModemManagerData.bModemRcvSysLoadingMessageFlag = true;
#endif
        ModemManagerData.bModemRcvSysStartMessageFlag = true;
        ModemManagerData.bModemRcvPBReadyMessageFlag = true;

        HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_URC_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_URC_CallBack, false);              // modem command를 전달하고 ok 응답받기까지 delay

        // modem은 초기화를 거치지 않고 바로 망접속을 확인 한다.
        SetModemState(eMODEM_INIT_MODEM);

        //MONI 2018-03-07
        // we block this code because some time it makes modem stocked.
        // if we fixed this situation then, we will reuse this code
//        boolean_t bSystemReset;
//        GetAutolinkConfigProperty(eAutoLinkConfig_SystemResetFlag,(void*)&bSystemReset);
//
//        if( bSystemReset == false )
//        {
//    		ModemManagerData.bModemRcvSysLoadingMessageFlag = true;
//    		ModemManagerData.bModemRcvSysStartMessageFlag = true;
//    		ModemManagerData.bModemRcvPBReadyMessageFlag = true;
//
//            HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_URC_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_URC_CallBack, false);              // modem command를 전달하고 ok 응답받기까지 delay
//
//            // modem은 초기화를 거치지 않고 바로 망접속을 확인 한다.
//            SetModemState(eMODEM_INIT_MODEM);
//        }
//        else
//        {
//            //MONI 2018-02-18
//            // system is reseted because of modem is not response anything.
//            // after reset we should check sysloading message from modem.
//            HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, true);               // ^START, ^PBREADY message를 응답 받기까지의 delay
//
//            ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
//            SetModemState(eMODEM_WAIT_RCV_SYSLOADING);
//        }
	}

	ModemManagerData.bNeedStartUpCheckSMSFlag = true;

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessCMEErrorWorkaround(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			if(Md_SetFlightMode("4,0") == true) {
				ModemManagerData.bModemRcvSysStartMessageFlag = false;

				ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 1:
			// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^SYSSTART message를 응답 받기까지의 delay, 초기에 ^SYSSTART message를 받지 못할 경우도 있다.
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT_2, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_AtWorkaround_CallBack, true);
			ModemManagerData.nMdmSubState = 2;
			break;

		case 2:				// ^SYSSTART 응답을 기다린다.
			if(ModemManagerData.bModemRcvSysStartMessageFlag == false) {
				break;
			}

            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
			ModemManagerData.nMdmSubState = 3;
			break;

		case 3:
			if(Md_SetFlightMode("1,0") == true) {
				ModemManagerData.bModemRcvSysStartMessageFlag = false;

				ModemManagerData.nMdmSubNextState = 4;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 4:
			// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^START, ^PBREADY message를 응답 받기까지의 delay
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT_2, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_AtWorkaround_CallBack, true);
			ModemManagerData.bModemRcvSysStartMessageFlag = false;

			ModemManagerData.nMdmSubState = 5;
			break;

		case 5:				// ^SYSSTART 응답을 기다린다.
			if(ModemManagerData.bModemRcvSysStartMessageFlag == false) {
				break;
			}
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
			ModemManagerData.nMdmSubState = 6;
			break;

        case 6:
            ModemManagerData.bModemRcvPBReadyMessageFlag = false;
            HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_PBREADY_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_PBREAD_CallBack, true);

            ModemManagerData.nMdmSubState = 7;
            break;

        case 7:
            // check sleep mode
            if( IsModemSleepWaitState() == true )
            {
                SetModemState(eMODEM_ENTER_SLEEP_MODE);
                break;
            }

            if(ModemManagerData.bModemRcvPBReadyMessageFlag == false) {
                break;
            }
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);

            // initialized hardware
            SetModemControlEvent(eMngMdmInitializeHardware);

            ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
            break;
#if false
		case 6:
			if(Md_SetOperatorSelection("0") == true) {				// 네트워크 접속 시도
				//ModemManagerData.nMdmSubNextState = 7;
				//ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			}

            // check sleep mode
            if( IsModemSleepWaitState() == true )
            {
                SetModemState(eMODEM_ENTER_SLEEP_MODE);
            }

			break;

		case 7:
			ModemManagerData.eNetworkRegStatus = eNETWORK_REG_STATUS_NOT_REG;
			//ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
            ModemManagerData.nMdmSubState = 8;
			break;

		case 8:
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, 1000, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 9;
			break;

		case 9:
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 10;
			break;

		case 10:
//			if(Md_CheckNetworkRegistration() == true) {
			if(Md_CheckPacketDomainRegistration() == true) {
				ModemManagerData.nMdmSubNextState = 11;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 11:
			if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REGISTERED 
				|| ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REG_ROAMING) {
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
				break;
			}
			else if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REG_DENIED) {
				g_stModem_Rep.eMDResponse = eMDResponseState_ModemRegDenied;
			}
			else if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_NOT_REG) {
				ModemManagerData.nMdmSubState = 8;
			}
			break;
#endif
		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_END_PROCESS:
			Trace(" Complete workaround\r\n");
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessCheckModemBaudrate(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
            HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);
            HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

			// MCU의 UART2 baudrate를 변경한다.
			Trace(" Init UART2(%dbps)\r\n", gaModemSearchBaudrateStep[gnModemSearchBaudrateStepCount]);
			ModemUart_Init(gaModemSearchBaudrateStep[gnModemSearchBaudrateStepCount]);

			geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_UNKNOWN;
#ifdef RF_COMMON_MODEM
			ModemManagerData.bModemRcvSysStartMessageFlag = false;
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, true);
#else
			ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, true);
#endif
			ModemManagerData.nMdmSubNextState = 1;
		    ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_RESET;				// modem 만 reset
			break;

		case 1:
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
		if(ModemManagerData.bModemRcvSysStartMessageFlag != true) {
#else
		if(ModemManagerData.bModemRcvSysLoadingMessageFlag != true) {
#endif
				if(geChangeDefaultModemBaudFlag == eMODEM_BAUDRATE_SEARCHING_STATE_REQ_SEARCHING) {

                    // we must increse the count before retry because the array is accessed 1 more
					if(++gnModemSearchBaudrateStepCount < MAX_SERCH_BAUDRATE_STEP_NO) {
                        ModemManagerData.nMdmSubState = 0;
						Trace(" gnModemSearchBaudrateStepCount - %d\r\n", gnModemSearchBaudrateStepCount);
					}
					else {
						Trace(" Not install!!!\r\n");
						geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_FAIL;
						ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
                        return eMODEM_PROCESS_FUNC_RET_FAIL;
					}


				  break;
				}

				break;
			}

			ModemManagerData.bModemRcvSysStartMessageFlag = false;
			ModemManagerData.nMdmSubState = 2;
			break;

		case 2:
			if(ModemManagerData.bModemRcvSysStartMessageFlag == false) {
				break;
			}

			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, 1000, eSWTimer_ONESHOT, NULL, true);

			ModemManagerData.nMdmSubState = 3;
			break;

		case 3:
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly) != 0) {
				break;
			}
			// 2018.05.03 SPARROW : PBREADY확인하는 단계 추가  -> PBREADY가 늦게올경우 init못하는 문제 발생
			//ModemManagerData.nMdmSubState = 4;

			ModemManagerData.nMdmSubState = 9;
			ModemManagerData.bModemRcvPBReadyMessageFlag = false;
			break;

		case 4:
			Trace(" modem init\r\n");
			//	1 : echo enable   0 : echo disable
			if(Md_SetEcho("0") == true) {
				ModemManagerData.nMdmSubNextState = 5;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 5:
			if(Md_ResultOnOff("0") == true) {
				ModemManagerData.nMdmSubNextState = 6;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 6:
#if defined(INITIAL_SETTING_MODEMBAUD)
			if( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER) == 0 )
			{
				if(Md_Set_115200_Baudrate() == true) {
					ModemManagerData.nMdmSubNextState = 7;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;

					HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, 2000, eSWTimer_ONESHOT, NULL, true);
				}
			}
			else
			{
				if(Md_Set_921600_Baudrate() == true) {
					ModemManagerData.nMdmSubNextState = 7;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;

					HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, 2000, eSWTimer_ONESHOT, NULL, true);
				}
			}
#else
//			if(Md_Set_3M_Baudrate() == true) {
			if(Md_Set_921600_Baudrate() == true) {
//            if(Md_Set_115200_Baudrate() == true ) {
				ModemManagerData.nMdmSubNextState = 7;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;

				HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, 2000, eSWTimer_ONESHOT, NULL, true);
			}
#endif
			break;

		case 7:
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly) != 0) {
				break;
			}
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, false);
#else
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, false);
#endif
			ModemManagerData.nMdmSubState = 8;
			break;

		// modem baudrate 설정을 921600 속도로 사용하는 경우
		case 8:
#if defined(INITIAL_SETTING_MODEMBAUD)
			if( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER) == 0 )
			{
				Trace(" Set baud 115200\r\n");

                HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);
                HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

				// MCU의 UART2 baudrate를 변경한다.
				Trace(" Init UART2(115200dbps)\r\n");
				ModemUart_Init(115200);

				geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_SUCCESS;

				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
			  ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;

				// save new baudrate
				BkSram_ModemInfo.unBaurdRate = 115200;
			}
			else
			{
				Trace(" Set baud 921600\r\n");

                HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);
                HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

				// MCU의 UART2 baudrate를 변경한다.
				Trace(" Init UART2(921600dbps)\\rn");
				ModemUart_Init(921600);

				geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_SUCCESS;

				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
			    ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;

				// save new baudrate
				BkSram_ModemInfo.unBaurdRate = 921600;
			}
#else
			Trace(" Set baud 921600\r\n");

            HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);
            HalDrvDmaIOCtrl(eDMA_IO_DeInit, HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

			// MCU의 UART2 baudrate를 변경한다.
			Trace(" Init UART2(921600dbps)\r\n");
			ModemUart_Init(921600);

			geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_SUCCESS;

			ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
		    ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;

            // save new baudrate
            BkSram_ModemInfo.unBaurdRate = 921600;
#endif
			break;

		case 9:
			if(ModemManagerData.bModemRcvPBReadyMessageFlag == false) {
				break;
			}
			ModemManagerData.nMdmSubState = 4;
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_MODEM_RESET:
			Trace("Immediately reset\r\n");
			printf("PowerOnGemaltoModem_Modem_Reset_Power6\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
//			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
//			APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
//		    APP_Delay(200);
//			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#else
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			APP_Delay(20);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			APP_Delay(20);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
#endif
			ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
			break;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessCheckModemBaudrateResult(void)
{
	if(geChangeDefaultModemBaudFlag == eMODEM_BAUDRATE_SEARCHING_STATE_SUCCESS) {
		Trace("Changing baudrate success.\r\n");
        SetModemState(eMODEM_INIT_MODEM);
	}
	else if(geChangeDefaultModemBaudFlag == eMODEM_BAUDRATE_SEARCHING_STATE_FAIL) {
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
        SetModemState(eMODEM_WAIT_RCV_SYSSTART);
#else
		SetModemState(eMODEM_WAIT_RCV_SYSLOADING);
#endif
	}

	return eMODEM_PROCESS_FUNC_RET_OK;
}

boolean_t m_bRetryResetSeq = false;

eMODEM_PROCESS_FUNC_RET ProcessModemHWReset(void)
{
	uint16_t n;
	// 180414 SPARROW
	if ((g_bYUJINSelftestFlag == true)||(g_bHYPERTECSelftestFlag == true))			return eMODEM_PROCESS_FUNC_RET_OK;

	switch(ModemManagerData.nMdmSubState) {
		case 0:
			Trace(" *eMODEM_RESET(HW reset)\r\n");
			ModemManagerData.bAvailableModemCommFlag = false;

			ClearQueue(g_stGitCommInfo[eCOMM_TYPE_UART_MODEM].pstInQueue);

			ModemManagerData.eNetworkRegStatus = eNETWORK_REG_STATUS_IDLE;

            /* Disable the alarmA */
            //    HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_DISABLE);

			InitializeModemManager(true);

			g_stModem_Rep.start_rep = false;
			g_stModem_Rep.cmd_index = eCmd_None;

			for(n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
				BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
			}

            m_bRetryResetSeq = false;

			ModemManagerData.nMdmSubState = 1;

			break;

#if true
        case 1:
				printf("PowerOnGemaltoModem_Modem_Reset_Power7\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
//				HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
//				APP_Delay(200);
			    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			    APP_Delay(200);
			    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			    APP_Delay(200);
			    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
//			    APP_Delay(200);
//				HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#else
                HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
                APP_Delay(100);


    			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
                APP_Delay(100);

                HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
                APP_Delay(50);
                HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
                //APP_Delay(50);
#endif
            return eMODEM_PROCESS_FUNC_RET_OK;

            break;
#else

		case 1:
			printf("PowerOnGemaltoModem_Modem_Reset_Rst\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
//			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
//			APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
//		    APP_Delay(200);
//			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#else
            HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
#endif
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 2;
			return eMODEM_PROCESS_FUNC_RET_OK;


		case 2:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}
			printf("PowerOnGemaltoModem_Modem_Reset_Power8\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
//			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
//			APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
//		    APP_Delay(200);
//			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#else
            HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#endif
			ModemManagerData.nMdmSubState = 3;
			break;

		case 3:
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 200, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 4;
			break;

		case 4:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 5;
			break;

		case 5:
			printf("PowerOnGemaltoModem_Modem_Reset_Rst2\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
//			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
//			APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
//		    APP_Delay(200);
//			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#else
            HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
#endif
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);

			ModemManagerData.nMdmSubState = 6;
			break;


		case 6:
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 200, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 7;
			break;

		case 7:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 8;
			break;

		case 8:
			printf("PowerOnGemaltoModem_Modem_Reset_Rst3\r\n");
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);

			ModemManagerData.nMdmSubState = 9;
			break;

        case 9:
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

            ModemManagerData.nMdmSubState = 10;
            break;
        case 10:
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 200, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 11;
			break;

		case 11:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 12;
			break;

		case 12:
			printf("PowerOnGemaltoModem_Modem_Reset_Rst4\r\n");
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);

			ModemManagerData.nMdmSubState = 13;
			break;

        case 13:
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

            ModemManagerData.nMdmSubState = 14;
            break;
		case 14:
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_URC_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_URC_CallBack, false);
			return eMODEM_PROCESS_FUNC_RET_OK;
#endif
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessModemHWResetResult(void)
{
	ModemManagerData.bRunWorkaroundFlag = false;

    //MONI 2018-1-27
    // to disable postpond after reset we added this flag
    ModemManagerData.bNeedStartUpCheckSMSFlag = true;

#ifdef RF_COMMON_MODEM
	HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, true);				// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^START, ^PBREADY message를 응답 받기까지의 delay
	SetModemState(eMODEM_WAIT_RCV_SYSSTART);
#else
	HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, true);				// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^START, ^PBREADY message를 응답 받기까지의 delay
	SetModemState(eMODEM_WAIT_RCV_SYSLOADING);
#endif
    Trace(" start modem manager_2\r\n");

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessEnterModemPowerSaveMode(void)
{
	static unsigned int s_uiTimeoutTimer=0;

	switch(ModemManagerData.nMdmSubState) {
        case 0:
#ifdef RF_COMMON_MODEM 
#ifdef GPSVIEWNotOFF
		    ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
#else
            if(Md_SetGNSS("\"Engine\",\"0\"") == true) {    //mod.kks todo GPS setting "0:OFF / 1:ON"
                ModemManagerData.nMdmSubNextState = 1;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
                s_uiTimeoutTimer = Get_Tmr();
            }
#endif
#else
            ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
            ModemManagerData.nMdmSubState = 2;
#endif
            break;
#ifdef RF_COMMON_MODEM 
		case 1:
			if(Md_SetGNSS("\"NMEA/Output\",\"OFF\"") == true) {         //"OFF / ON / Last"
				ModemManagerData.nMdmSubNextState = 2;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;

//                HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_RESET); //mod.kks need to set the pin after ADVINTERVAL

                s_uiTimeoutTimer = Get_Tmr();
            }
			break;
#endif
        case 2:
#ifdef RF_COMMON_MODEM 
			if(Md_SetPowerSaveMode("2, 1000, 3") == true) 	//22.03.26 mod.kks to sleep mode for fast.
#else
			if(Md_SetPowerSaveMode("2, 3000, 3") == true) 	// modem�� power save mode�� �����Ѵ�.
#endif
			{  
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				s_uiTimeoutTimer = Get_Tmr();
			}

			//ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			if( Get_TmrDelta(Get_Tmr(),s_uiTimeoutTimer) >= 100 )
			{
				printf("ProcessEnterModemPowerSaveMode_Timeout\r\n");
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			}
			break;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessEnterModemPowerSaveModeResult(void)
{
	Trace(" modem is sleep mode, now\r\n");
	SetModemState(eMODEM_NOW_SLEEP_MODE);
    ModemManagerData.bNeedModemHWResetFlag = false;

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessCMEErrorWorkaroundResult(void)
{
    SetModemState(eMODEM_INIT_MODEM);

/* MONI Not used code
	Trace(" goto *ModemManager state: %d\r\n", ModemManagerData.eOldState);
	ModemManagerData.eState = ModemManagerData.eOldState;

	if(ModemManagerData.eOldState == eMODEM_CHECK_NETWORK_REGISTRATION) {
	}
	else {
		SetModemState(eMODEM_READY);

		HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, 500, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);
	}
*/
//  MONI 2018-03-05
//  Not used code
//	if(GetVehicleEventKey() != eMESSAGE_EVENT_KEY_NONE) {
//		MessageManagerData.bRequestEventMsgFlag_Alarm = true;
//	}

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessInitializeModem(void)
{
	char * carrSeriaNumber = GetFWSerialNumber();
	//static char s_cModemReset = 0;
	switch(ModemManagerData.nMdmSubState) {

        case 0:
            // check sleep mode
            if( IsModemSleepWaitState() == true )
            {
                SetModemState(eMODEM_ENTER_SLEEP_MODE);
            }

            //APP_Delay(10); // TOWDAM TEST
			if(Md_SetPowerSaveMode("1, 0, 0") == true) {				// modem의 ASC0/1을 항상 active 상태로 설정한다.
                ModemManagerData.nMdmSubNextState = 1;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;

		case 1:
			if(Md_ResultOnOff("0") == true) {
				ModemManagerData.nMdmSubNextState = 2;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
        case 2:
            if(Md_CheckAirplane("?") == true) {
                Trace("Request airplane status\r\n");
#ifdef RF_COMMON_MODEM //mod.kks 21.12.22
                ModemManagerData.nMdmSubNextState = 30; //to set the cfun=1 on the pls63W
#else
                ModemManagerData.nMdmSubNextState = 3;
#endif
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
        case 30: //mod.kks 21.12.22 only appiled in the PLS63W-model ....
            if( ModemManagerData.bAirplaneMode == true ){
                if(Md_SetFlightMode("1") == true) {
                    ModemManagerData.nMdmSubNextState = 3;
                    ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
                }
            }
            else{
                    ModemManagerData.nMdmSubNextState = 3;
                    ModemManagerData.nMdmSubState = 3;
            }
			break;
        case 3:
            if( ModemManagerData.bAirplaneMode == true )
            {
                Trace("actived airplane mode\r\n");
                if(Md_SetFlightMode("1,1") == true){
                    Trace("request normal mode\r\n");
                    ModemManagerData.bModemRcvSysStartMessageFlag = false;
                    ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
                    SetModemState(eMODEM_WAIT_RCV_SYSSTART);
#else
                    SetModemState(eMODEM_WAIT_RCV_SYSLOADING);
#endif

                    // MONI 2018-03-23
                    // if gemalto mode is changed from airplane to normal
                    // then, ME must clear all network parameters
                    InitializeModemManager(true);
                    g_stModem_Rep.start_rep = false;
                    g_stModem_Rep.cmd_index = eCmd_None;
                    for(int n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
                        BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
                    }
                    ModemManagerData.bNeedStartUpCheckSMSFlag = true;
					//s_cModemReset = 1;
                }
            }
            else
            {
                ModemManagerData.nMdmSubNextState = 4;
                ModemManagerData.nMdmSubState = 4;
				//s_cModemReset = 0;
            }
            break;
            
        case 4:
            //  1 : echo enable   0 : echo disable
            if(Md_SetEcho("0") == true) {
                ModemManagerData.nMdmSubNextState = 5;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;

        case 5:
			if(Md_SetFlowControl() == true) {
				ModemManagerData.nMdmSubNextState = 6;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
        case 6:
           if(Md_GetIMSI() == true) {
				ModemManagerData.nMdmSubNextState = 7;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
        case 7:
			if(Md_ReadSystemInfo() == true) {
				ModemManagerData.nMdmSubNextState = 8;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
        case 8:
			if(Md_GetIMEI() == true) {
				ModemManagerData.nMdmSubNextState = 9;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
        case 9:
			if(Md_GetDateTime() == true) {
				ModemManagerData.nMdmSubNextState = 10;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 10:
			if(Md_Set_ErrorMsgFormat("2") == true) {
				ModemManagerData.nMdmSubNextState = 11;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 11:
			if(Md_GetCCIDInfo() == true) {
				ModemManagerData.nMdmSubNextState = 12;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
        case 12:
            if(Md_GetSIND("\"message\",2") == true) {
#ifdef RF_COMMON_MODEM
				ModemManagerData.nMdmSubNextState = 14;
#else
				ModemManagerData.nMdmSubNextState = 13;
#endif
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
        case 13:
			// at+cnmi=2,1,0,0,0
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			if(Md_SetSMSReportConfig("0,1,0,0,1") == true) {
#else
			if(Md_SetSMSReportConfig("0,1,0,0,0") == true) {
#endif
				ModemManagerData.nMdmSubNextState = 14;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 14:
			if(Md_GetPhoneNo() == true) {
				ModemManagerData.nMdmSubNextState = 15;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
        case 15:
            // at+scfg="Gpio/mode/RING0","std"
//            if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"std\"") == true) {
//                ModemManagerData.nMdmSubNextState = 16;
//                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
//            }
// 180717 SPARROW
			 if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"std\"") == true) {

				//ModemManagerData.nMdmSubNextState = 16;
				// 1) AT^SCFG="gpio/mode/ring0","std"
				// 2) restart module with AT+CFUN=1,1
				// 3) AT^SCFG="urc/ringline","asc0"
				// noserialKRH0000 일 경우에만 CFUN=1,1 전송
				//				if((g_bModemBootingError_Flag == 0)&&(memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_NOSERIAL_NUMBER)==0)){
#if 0 // 2022/11/15 James Jean : 생산프로그램 연동이 늦어지는 이유 - 삭제함
				 if((g_bModemBootingError_Flag == 0)&&(memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER)==0)){

					ModemManagerData.nMdmSubNextState = 21;
					g_bModemBootingError_Flag = 1;
				}
				else
#endif
                {
					ModemManagerData.nMdmSubNextState = 16;
				}
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#ifdef SELECT_OPERATOR		//this postion is not good - sumtimes appear CME ERROR 210916
		case 16:
            // at+cops=1,2,"45204"    //Viettel
            if(Md_SetOperatorSelection("1,2,\"45204\"") == true) {
                ModemManagerData.nMdmSubNextState = 61;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
		case 61:
            // at+scfg="URC/Ringline","asc0"
            if(Md_SetModemConfigExt("\"URC/Ringline\",\"asc0\"") == true) {
                ModemManagerData.nMdmSubNextState = 17;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#else
		case 16:
            // at+scfg="URC/Ringline","asc0"
            if(Md_SetModemConfigExt("\"URC/Ringline\",\"asc0\"") == true) {
                ModemManagerData.nMdmSubNextState = 17;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#endif
#ifdef USE_ONLY_3G
        case 17:
            // at+scfg="URC/Ringline/ActiveTime","1"                // 100ms
            // at+scfg="URC/Ringline/ActiveTime","2"                // 1sec
            if(Md_SetModemConfigExt("\"URC/Ringline/ActiveTime\",\"2\"") == true) {
				if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON)
				{
                	ModemManagerData.nMdmSubNextState = 51;
				}
				else
				{
					ModemManagerData.nMdmSubNextState = 18;
				}
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#ifdef RF_COMMON_MODEM //mod.kks 21.11.29 to 3G only PLS63

            /* PLS63-W 3G Band Setting */
			/* Band1 -> 1   */
            /* Band2 -> 2   */
			/* Band3 -> 4   */
            /* Band4 -> 8   */
            /* Band5 -> 10  */
            /* Band6 -> 20  */
			/* Band8 -> 80   */
            /* Band19 -> 4000 */

		case 51: //2G Disable
			if(Md_SetModemConfigExt("\"Radio/Band/2G\",\"0\"") == true) {
                ModemManagerData.nMdmSubNextState = 52;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;

		case 52://4G Disable
			if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"0\",\"0\"") == true) {
                ModemManagerData.nMdmSubNextState = 53;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
		case 53:
			if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"1\"") == true) {
                ModemManagerData.nMdmSubNextState = 18;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#else   //RF_COMMON_MODEM
		case 51:
			//WCDMA : B1
			//0x00000001 WCDMA BAND I (BC1)
			//0x00000010 WCDMA BAND V (BC5)
			//0x00000080 WCDMA BAND VIII (BC8)
			if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"0x00000081\"") == true) {
                ModemManagerData.nMdmSubNextState = 52;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
		case 52:
			//LTE : B3
			//0x00000004 LTE BAND III (BC3)
			//0x00000010 LTE BAND V (BC5)
			//0x00000080 LTE BAND VIII (BC8)
			//0x08000000 LTE BAND XXVIII (BC28)
			if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"0x08000000\"") == true) {
                ModemManagerData.nMdmSubNextState = 18;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#endif //RF_COMMON_MODEM
#else	//USE_ONLY_3G
#ifdef USE_ONLY_4G
            /* PLS63-W 3G Band Setting */
			/* Band1 -> 1   */
            /* Band2 -> 2   */
			/* Band3 -> 4   */
            /* Band4 -> 8   */
            /* Band5 -> 10  */
            /* Band7 -> 40  */
			/* Band8 -> 80   */
            /* Band12 -> 800 */
            /* Band13 -> 1000 */
            /* Band18 -> 20000 */
            /* Band19 -> 40000 */
            /* Band20 -> 80000 */
            /* Band26 -> 2000000 */
            /* Band28 -> 8000000 */

		case 17:
            // at+scfg="URC/Ringline/ActiveTime","1"                // 100ms
            // at+scfg="URC/Ringline/ActiveTime","2"                // 1sec
            if(Md_SetModemConfigExt("\"URC/Ringline/ActiveTime\",\"2\"") == true) {
                if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON)
				{
                	ModemManagerData.nMdmSubNextState = 51;
				}
				else
				{
					ModemManagerData.nMdmSubNextState = 18;
				}
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#ifdef RF_COMMON_MODEM //mod.kks 21.11.29 to 3G only PLS63

            /* PLS63-W 3G Band Setting */
			/* Band1 -> 1   */
            /* Band2 -> 2   */
			/* Band3 -> 4   */
            /* Band4 -> 8   */
            /* Band5 -> 10  */
            /* Band6 -> 20  */
			/* Band8 -> 80   */
            /* Band19 -> 4000 */

		case 51: //2G Disable
			if(Md_SetModemConfigExt("\"Radio/Band/2G\",\"0\"") == true) {
                ModemManagerData.nMdmSubNextState = 52;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;

		case 52://4G Disable
			if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"0\"") == true) {
                ModemManagerData.nMdmSubNextState = 53;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
		case 53:
			if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"4\",\"0\"") == true) {
                ModemManagerData.nMdmSubNextState = 18;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#else   // RF_COMMON_MODEM
		case 51:
			//WCDMA : B1
			//0x00000001 WCDMA BAND I (BC1)
			//0x00000010 WCDMA BAND V (BC5)
			//0x00000080 WCDMA BAND VIII (BC8)
			if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"0x00000010\"") == true) {
                ModemManagerData.nMdmSubNextState = 52;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
		case 52:
			//LTE : B3
			//0x00000004 LTE BAND III (BC3)
			//0x00000010 LTE BAND V (BC5)
			//0x00000080 LTE BAND VIII (BC8)
			//0x08000000 LTE BAND XXVIII (BC28)
			if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"0x00000004\"") == true) {
                ModemManagerData.nMdmSubNextState = 18;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#endif //RF_COMMON_MODEM
#else	//USE_ONLY_4G
#ifdef USE_3G_4G
		case 17:
			// at+scfg="URC/Ringline/ActiveTime","1"				// 100ms
			// at+scfg="URC/Ringline/ActiveTime","2"				// 1sec
			if(Md_SetModemConfigExt("\"URC/Ringline/ActiveTime\",\"2\"") == true) {
				if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON)
				{
                	ModemManagerData.nMdmSubNextState = 51;
				}
				else
				{
					ModemManagerData.nMdmSubNextState = 18;
				}
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 51:
			//Korea KT :         B1,                    0x00000001
			//Korea SKT:     BC0,B1,                    0x00000001
			//Korea LG :         BC4                    -
			//Vietnam vinaphone:    B8,B9               0x00000080
			//Vietnam Viettel :  B1                     0x00000001
            //JAPAN : NTT -      B1                     0x00000001
            //JAPAN : softbank - B1,B8                  0x00000081
			//0x00000001 WCDMA BAND I (BC1)
			//0x00000010 WCDMA BAND V (BC5)
			//0x00000080 WCDMA BAND VIII (BC8)
			if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"0x00000081\"") == true) {
                ModemManagerData.nMdmSubNextState = 52;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
		case 52:
			//Korea KT :            B3,      B8                      0x00000084
			//Korea SKT:            B3,B5                            0x00000014
			//Korea LG :         B1,   B5,B7                         0x00000010
			//Vietnam vinaphone: B1,B3                               0x00000004
			//Vietnam Viettel :  B1,B3                               0x00000004
            //JAPAN : NTT -      B1,B3,             B19,B21,B28      0x08000004
            //JAPAN : softbank - B1,         B8,B11,            B38  0x00000080
			//0x00000004 LTE BAND III (BC3)
			//0x00000010 LTE BAND V (BC5)
			//0x00000080 LTE BAND VIII (BC8)
			//0x08000000 LTE BAND XXVIII (BC28)
			if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"0x00000004\"") == true) {
                ModemManagerData.nMdmSubNextState = 18;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#else	//USE_3G_4G
		case 17:
			// at+scfg="URC/Ringline/ActiveTime","1"				// 100ms
			// at+scfg="URC/Ringline/ActiveTime","2"				// 1sec
			if(Md_SetModemConfigExt("\"URC/Ringline/ActiveTime\",\"2\"") == true) {
#if defined(SINGTELUSIM) || defined(QA_FIFA) || defined(VIETNAMUSIM) //mod.kks 22.01.03 set the band to contact the supported band without the 38-band.
				ModemManagerData.nMdmSubNextState = 51 ;  // 4G supported Band in SINGTEL USIM : 3,7,8 band
#else
                ModemManagerData.nMdmSubNextState = 18 ;
#endif
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
            
#ifdef SINGTELUSIM
  	 	 case 51:
#ifdef RF_COMMON_MODEM
			if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"81\"") == true) {
#else
            if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"0x00000081\"") == true) {
#endif
                ModemManagerData.nMdmSubNextState = 52;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
         case 52:
#ifdef RF_COMMON_MODEM
			if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"C4\",\"0\",\"1\"") == true) {
#else
            if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"0x00000084\"") == true) {
#endif
                ModemManagerData.nMdmSubNextState = 18;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#endif  //SINGTELUSIM

#ifdef VIETNAMUSIM //3G BAND 1, 4G BAND 3
  	 	 case 51:
#ifdef RF_COMMON_MODEM
			if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"1\"") == true) {
#else
            if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"0x00000001\"") == true) {
#endif
                ModemManagerData.nMdmSubNextState = 52;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
         case 52:
#ifdef RF_COMMON_MODEM
			if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"45\",\"0\",\"1\"") == true) {
#else
            if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"0x00000004\"") == true) {
#endif
                ModemManagerData.nMdmSubNextState = 18;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#endif  //VIETNAMUSIM
#if defined(QA_FIFA) //3G BAND 1, 4G BAND 3,7
		case 51:
#ifdef RF_COMMON_MODEM
			if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"1\"") == true) {
#else
            if(Md_SetModemConfigExt("\"Radio/Band/3G\",\"0x00000001\"") == true) {
#endif
                ModemManagerData.nMdmSubNextState = 52;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
         case 52:
#ifdef RF_COMMON_MODEM
			if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"44\",\"0\",\"1\"") == true) {
#else
            if(Md_SetModemConfigExt("\"Radio/Band/4G\",\"0x00000004\"") == true) {
#endif
                ModemManagerData.nMdmSubNextState = 18;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#endif  //QA_FIFA
#endif	//USE_3G_4G
#endif	//USE_ONLY_4G
#endif	//USE_ONLY_3G
		case 18:
			// at+cnmi=2,1,0,0,0
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			//if(Md_SetSMSReportConfig("2,1,0,0,1") == true) { // mod.kks todo check "error"
			if(Md_SetSMSReportConfig("2,1") == true) {  //mod.kks TEMP
#else
			if(Md_SetSMSReportConfig("2,1,0,2,0") == true) {
#endif
				ModemManagerData.nMdmSubNextState = 19;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
        case 19:
            if(Md_SetSMSStorage("\"SM\",\"SM\",\"SM\"") == true) {
    //          if(Md_SetSMSStorage("\"ME\",\"ME\",\"ME\"") == true) {
                ModemManagerData.nMdmSubNextState = 20;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
        case 20:
            if(Md_SetSMSFormat("1") == true) {
#ifdef RF_COMMON_MODEM //mod.kks GPS 21.10.27
                ModemManagerData.nMdmSubNextState = 22;
#else
		        ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
#endif
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;

		 case 21:
			if(Md_SetFlightMode("1,1") == true) {								// Flighe mode off
#ifdef RF_COMMON_MODEM //mod.kks GPS 21.10.27
				ModemManagerData.nMdmSubNextState = 22;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
#else
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
#endif
			}

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
            SetModemState(eMODEM_WAIT_RCV_SYSSTART);
#else
			SetModemState(eMODEM_WAIT_RCV_SYSLOADING);
			ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
#endif
			ModemManagerData.bModemRcvSysStartMessageFlag = false;
			ModemManagerData.bModemRcvPBReadyMessageFlag = false;
			geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_UNKNOWN;
			break;
#ifdef RF_COMMON_MODEM //mod.kks 21.10.27
		/* GPS Initial by using the Modem AT command */
		case 22:
			if(Md_SetGNSS("\"Engine\",\"1\"") == true) {    //mod.kks todo GPS setting "0:OFF / 1:ON"
		   		ModemManagerData.nMdmSubNextState = 23;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 23:
			if(Md_SetGNSS("\"NMEA/Interface\",\"6\"") == true) {
				ModemManagerData.nMdmSubNextState = 24;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
			break;
		case 24:
			if(Md_SetGNSS("\"NMEA/Output\",\"ON\"") == true) {         //"OFF / ON / Last"
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
            }
			break;
#endif

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;
		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessInitializeModemResult(void)
{
	// AT+CCID 명령까지 OK이면 "+PBREADY" message가 와야 한다.
	// 테스트 해보니 넉넉잡고 40sec 정도 대기해야 "+PBREAD"가 수신된다.
	// timeout은 80초로 설정해보자.

	//if((ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_ONLY_MODEM || ModemManagerData.eModemResetMode == eMODEM_RESET_MODE_POWER_ON) &&
    //    ModemManagerData.bModemRcvPBReadyMessageFlag == false )
    //{
	//	ModemManagerData.bModemRcvPBReadyMessageFlag = false;
	//	HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_PBREADY_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_PBREAD_CallBack, true);
	//}

#if 1	// 2018.05.03 SPARROW : Modem manager init state 순서변경으로 PRODUCT_MODE로 설정하는 위치 이동

	g_bUSIMInsertFlag = true;
	if( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_NOSERIAL_NUMBER) == 0 ){
		SetModemState(eMODEM_PRODUCT_MODE);
        GIT_Assert(false,eErrorCodeMdm|eNoSerial);
	}
	else{
		SetModemState(eMODEM_SetupNetwork);
	}
#else	// Original code
	SetModemState(eMODEM_CHECK_NETWORK_REGISTRATION);
#endif
    // send
    SetModemControlEvent(eMngMdmInitializeSoftware);

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessInitializeNetwork(void)
{
    char carrBuff[128] = {0,};

	switch(ModemManagerData.nMdmSubState) {
		case 0:
            sprintf(carrBuff,"1,\"IPV4V6\",\"%s\"",BkSram_ModemInfo.carrDefaultAPN);
			if(Md_SetApn((uint8_t*)carrBuff) == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessInitializeNetworkResult(void)
{
	SetModemState(eMODEM_CHECK_NETWORK_REGISTRATION);

	return eMODEM_PROCESS_FUNC_RET_OK;
}


eMODEM_PROCESS_FUNC_RET ProcessExtInitModem(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			if(Md_GetPhoneNo() == true) {
				ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 1:
			// at+cnmi=2,1,0,0,0
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			if(Md_SetSMSReportConfig("2,1,0,0,1") == true) {
#else
			if(Md_SetSMSReportConfig("2,1,0,0,0") == true) {
#endif
				ModemManagerData.nMdmSubNextState = 2;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 2:
			// at+scfg="Gpio/mode/RING0","std"
			if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"std\"") == true) {
//				ModemManagerData.nMdmSubNextState = 3;
				ModemManagerData.nMdmSubNextState = 3;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 3:
			// at+scfg="URC/Ringline","asc0"
			if(Md_SetModemConfigExt("\"URC/Ringline\",\"asc0\"") == true) {
				ModemManagerData.nMdmSubNextState = 4;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 4:
			// at+scfg="URC/Ringline/ActiveTime","1"				// 100ms
			// at+scfg="URC/Ringline/ActiveTime","2"				// 1sec
			if(Md_SetModemConfigExt("\"URC/Ringline/ActiveTime\",\"2\"") == true) {
				ModemManagerData.nMdmSubNextState = 5;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 5:
			if(Md_SetSMSStorage("\"SM\",\"SM\",\"SM\"") == true) {
//			if(Md_SetSMSStorage("\"ME\",\"ME\",\"ME\"") == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessExtInitializeModemResult(void)
{
	// 이제 부터 message manager state를 정상적으로 구동한다.
	Trace(" run message manager\r\n");
	SetModemState(eMODEM_READY);

    // change mode state
    SetModemControlEvent(eMngMdmConnected);

    // clear modem reset retry count;
    ClearModemResetRetryCount();

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessModemPowerOff(void)
{
	switch(ModemManagerData.nMdmSubState) {
#if true
        case 0:
//			if(Md_SetPowerOff() == true) {
				ModemManagerData.nMdmSubState = 1;
//			}
			break;
		case 1:
#ifdef RF_COMMON_MODEM //mod.kks 21.01.05 todo test check
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
#if 0 //mod.kks todo remove only compare the sequence.
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
			APP_Delay(200);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			APP_Delay(200);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			APP_Delay(200);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			APP_Delay(200);
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#endif
#else
            HalGPIOSetVaule(GPIO_MA_PWEN, eBIT_RESET);
#endif
            ModemManagerData.nMdmSubState = 2;
            ClearNetworkDelayProcess();
            break;
//        case 7:
//            if(Md_SetPowerOff() == true) {
//                ModemManagerData.nMdmSubNextState = 8;
//                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
//                ClearNetworkDelayProcess();
//            }
//            break;
		case 2:
            if( NetworkDelayProcess(1000) == false )
            {
                //skip until setting delay value
                break;
            }
            ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			break;
#else
		case 0:
			if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"gpio\"") == true) {
#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
				ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
#endif
				ModemManagerData.bModemRcvSysStartMessageFlag = false;
				ModemManagerData.bModemRcvPBReadyMessageFlag = false;

				ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 1:
			// modem reset
			printf("MDM: Wait...\r\n");
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 2000, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 2;
			break;

		case 2:				// wait 2000ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 3;
			break;

		case 3:
			// modem reset
			printf("MDM: reset(only modem)\r\n");
			printf("PowerOnGemaltoModem_Modem_Reset_Rst5\r\n");
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 4;
			break;

		case 4:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 5;
			break;

		case 5:
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 6;
			break;

		case 6:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 7;
			break;

		case 7:
			printf("PowerOnGemaltoModem_Modem_Reset_Rst6\r\n");
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			ModemManagerData.nMdmSubState = 8;
			break;


		case 8:				// wait 2000ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 9;
			break;

		case 9:
			// modem reset
			printf("MDM: reset(only modem)\r\n");
			printf("PowerOnGemaltoModem_Modem_Reset_Rst7\r\n");
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 10;
			break;

		case 10:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 11;
			break;

		case 11:
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 12;
			break;

		case 12:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 13;
			break;

		case 13:
			printf("PowerOnGemaltoModem_Modem_Reset_Rst8\r\n");
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			ModemManagerData.nMdmSubState = 14;
			break;

		case 14:				// ^SYSLOADING 응답을 기다린다.
#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
			if(ModemManagerData.bModemRcvSysLoadingMessageFlag == false) {
				break;
			}
#endif

			ModemManagerData.nMdmSubState = 15;
			break;

		case 15:				// ^SYSSTART 응답을 기다린다.
			if(ModemManagerData.bModemRcvSysStartMessageFlag == false) {
				break;
			}

			ModemManagerData.nMdmSubState = 16;
			//ModemManagerData.nMdmSubState = 12;			//SIM 카드 없는 경우도 있어서  ^PBREADY 응답pass
			break;

		case 16:				// ^PBREADY 응답을 기다린다. SIM 카드 없으면 기다리지 말아야 한다. SIM 카드 없으면 PBREADY는 응답 하지 않는다.
#ifndef RF_COMMON_MODEM //mod.kks 21.11.25
			if(ModemManagerData.bModemRcvPBReadyMessageFlag == false) {
				break;
			}
#endif

			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 1000, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 17;
			break;

		case 17:				// wait 1000ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 18;
			break;

		case 18:
			//	1 : echo enable   0 : echo disable
			if(Md_SetEcho("0") == true) {
				ModemManagerData.nMdmSubNextState = 19;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 19:
			//  GPIO open and output is SET  -> MODEM에서 SET 을 해줘야 MO_MODEM 신호가 RESET이 된다.(반전)
			if(Md_OpenCloseGPIO("1,23,1,0") == true) {
				ModemManagerData.nMdmSubNextState = 20;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
       case 20:
            if(Md_SetPowerOff() == true) {
				ModemManagerData.nMdmSubNextState = 21;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
            break;
        case 21:
            ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
            break;
		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;
#endif
		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessModemPowerOffResult(void)
{
    Trace("ProcessModemPowerOffResult : true\r\n");
	SetModemState(eMODEM_NOW_SLEEP_MODE);
    ModemManagerData.bNeedModemHWResetFlag = false;

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessSleepExtInitModem(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			if(Md_SetFlowControl() == true) {
				ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 1:
			// at+cnmi=2,1,0,0,0
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			if(Md_SetSMSReportConfig("2,1,0,0,1") == true) {
#else
			if(Md_SetSMSReportConfig("2,1,0,0,0") == true) {
#endif
				ModemManagerData.nMdmSubNextState = 2;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 2:
			// at+scfg="Gpio/mode/RING0","std"
			if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"std\"") == true) {
				ModemManagerData.nMdmSubNextState = 3;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 3:
			// at+scfg="URC/Ringline","asc0"
			if(Md_SetModemConfigExt("\"URC/Ringline\",\"asc0\"") == true) {
				ModemManagerData.nMdmSubNextState = 4;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 4:
			// at+scfg="URC/Ringline/ActiveTime","1"				// 100ms
			// at+scfg="URC/Ringline/ActiveTime","2"				// 1sec
			if(Md_SetModemConfigExt("\"URC/Ringline/ActiveTime\",\"2\"") == true) {
				ModemManagerData.nMdmSubNextState = 5;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 5:
			if(Md_SetSMSStorage("\"SM\",\"SM\",\"SM\"") == true) {
//			if(Md_SetSMSStorage("\"ME\",\"ME\",\"ME\"") == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessSleepExtInitializeModemResult(void)
{
	HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);

	// 이제 부터 message manager state를 정상적으로 구동한다.
	Trace(" run message manager\r\n");
    SetModemState(eMODEM_ENTER_SLEEP_MODE);


	return eMODEM_PROCESS_FUNC_RET_OK;
}


eMODEM_PROCESS_FUNC_RET ProcessCheckNetwarkRegistration(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_URC_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_URC_CallBack, false);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, 500, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 10000, eSWTimer_ONESHOT, MDM_NoResp_CMD_CallBack, false);

			ModemManagerData.nModemNoRegCount = 0;
			ModemManagerData.eNetworkRegStatus = eNETWORK_REG_STATUS_NOT_REG;

			ModemManagerData.nMdmSubState = 1;
			break;

		case 1:
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 2;
			break;

		case 2:
//			if(Md_CheckNetworkRegistration() == true) {
			if(Md_CheckPacketDomainRegistration() == true) {
					ModemManagerData.nMdmSubNextState = 3;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 3:
			g_stModem_Rep.eMDResponse = eMDResponseStateWait;

			if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REGISTERED
#ifdef  ENABLE_ROAMING_PROCESS
                || ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REG_ROAMING
#endif //#ifdef  ENABLE_ROAMING_PROCESS
                || ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_UNKNOWN
               ) {
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
				return eMODEM_PROCESS_FUNC_RET_CONTINUE;
			}
			else if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REG_DENIED) {
				Trace(" eNETWORK_REG_STATUS_REG_DENIED\r\n");
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
				return eMODEM_PROCESS_FUNC_RET_CONTINUE;
			}
			else if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_NOT_REG) {
				ModemManagerData.nModemNoRegCount++;
				if(ModemManagerData.nModemNoRegCount > MAX_REGISTER_NETWORK_COUNT) {
					Trace(" ModemManagerData.nModemNoRegCount - %d\r\n", ModemManagerData.nModemNoRegCount);

//					ModemManagerData.eOldState = eMODEM_CHECK_NETWORK_REGISTRATION;
//					g_stModem_Rep.eMDResponse = eMDResponseState_ModemWorkaround;
//					ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;

                    g_stModem_Rep.eMDResponse = eMDResponseState_DoNothing;
                    ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;

                    //MONI 2018-02-15
                    // after 5minutes later, we should try to register network
                    SetModemControlEvent(eMngMdmNotRegistered);

					return eMODEM_PROCESS_FUNC_RET_OK;
				}
			}
#ifdef  ENABLE_ROAMING_PROCESS
            else if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REG_ROAMING) {
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
				return eMODEM_PROCESS_FUNC_RET_CONTINUE;
            }
#endif

			HalTimerStartSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, eSWTimer_ONESHOT);

			ModemManagerData.nMdmSubState = 1;
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_END_PROCESS:
			//HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, false);
			//HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 5000, eSWTimer_ONESHOT, MDM_NoResp_CMD_CallBack, false);
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);
			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

			ModemManagerData.nMdmSubState = 0;

			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessCheckNetwarkRegistrationResult_First(void)
{
	if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REGISTERED
#ifdef  ENABLE_ROAMING_PROCESS
        || ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REG_ROAMING
#endif
        ) {
        //MONI 2018-02-09
        // led control process is changed
		//SetLedOnOffCtl(LED_ON, eLED_SERVER);

	  if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON) {
			SetModemState(eMODEM_SETTING_TIME);

            // send modem status
            SetModemControlEvent(eMngMdmRegistedNetwork);
			ModemManagerData.bAvailableModemCommFlag = true;
	  }
	  else {
			// 이제 부터 modem을 이용한 통신이 가능하다.
			Trace(" modem is available(2)\r\n");
			ModemManagerData.bAvailableModemCommFlag = true;

	        SetModemState(eMODEM_READY);

            // send modem status
            SetModemControlEvent(eMngMdmReConnected);

            // clear modem reset retry count;
            ClearModemResetRetryCount();
			ClearDeviceResetCount();
	  }
	}
	else {
		if(g_stModem_Rep.eMDResponse == eMDResponseState_ModemWorkaround) {
			ModemManagerData.bRunWorkaroundFlag = true;
			SetModemState(eMODEM_READY);
            Trace(">>>>> Fail to register to network\r\n");
		}
	}

  return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessCheckRssi(void)
{
	switch(ModemManagerData.nMdmSubState) {
        case 0:
            if(Md_GPRSAttachDetach("?") == true) {
                ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
            break;
#if	1

		case 1:
			if(Md_CheckSignalQuality() == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
#else
		case 1:
			if(Md_ReadOperatorSelection() == true) {
				ModemManagerData.nMdmSubNextState = 2;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 2:
			if(Md_GetServiceMonitor() == true) {
				ModemManagerData.nMdmSubNextState = 3;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		//case 3:
		//	if(Md_GetPDPAddress() == true) {
		//		ModemManagerData.nMdmSubNextState = 4;
		//		ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
		//	}
		//	break;
		case 3:
			if(Md_CheckSignalQuality() == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
#endif

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_END_PROCESS:
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessCheckRssiResult(void)
{
    ModemManagerData.bAvailableModemCommFlag = true;
    SetModemState(eMODEM_READY);
    return eMODEM_PROCESS_FUNC_RET_OK;
}


eMODEM_PROCESS_FUNC_RET ProcessSetModemTime(void)
{
	char * carrSeriaNumber = GetFWSerialNumber();
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			ModemManagerData.nMdmSubState = 2;
			break;
        case 1:
            if(Md_SetCTZRTime() == true) {
				ModemManagerData.nMdmSubNextState = 2;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
            break;
		case 2:
			if(Md_GetSIND("\"nitz\",2") == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessSetModemTimeResult(void)
{
	Trace(" run message manager\r\n");
	SetModemState(eMODEM_READY);

    // change mode state
    SetModemControlEvent(eMngMdmConnected);

    // clear modem reset retry count;
    ClearModemResetRetryCount();

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessGetModemTime(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			ModemManagerData.nMdmSubState = 1;
			break;

		case 1:
			if(Md_GetSIND("\"nitz\",2") == true) {
					ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessGetModemTimeResult(void)
{
	SetModemState(eMODEM_READY);
    ModemManagerData.bAvailableModemCommFlag = true;

	return eMODEM_PROCESS_FUNC_RET_OK;
}



#define MAX_SMS_COUNT 20

extern eSmsRcvStatus m_eSmsRcvStatus;

boolean_t m_bSmsRequest = false;

void SetRequestSms2Gemalto(boolean_t bRequest)
{
    m_bSmsRequest = bRequest;
}

boolean_t GetRequestSms2Gemalto()
{
    return m_bSmsRequest;
}


eMODEM_PROCESS_FUNC_RET ProcessCheckSMS(void)
{
	static uint16_t nIndex;
	uint16_t n;
	uint16_t i;
	uint8_t aTemp[10];

	switch(ModemManagerData.nMdmSubState) {
		case 0:
			for(i = 0; i < MAX_RCV_SMS_COUNT; i++) {
				ModemManagerData.stRcvSMSInfos[i].nIndex = NO_SMS_INDEX;
			}

			ModemManagerData.nMdmSubState = 1;
            ClearNetworkDelayProcess();
			break;

		case 1:
#if false
            if( NetworkDelayProcess(2000) == false )
            {
                break;
            }
#endif
			if(Md_ReadSMSforStatus("\"ALL\"") == true) {
                Trace("Request Sms Count\r\n");
				g_nCurrRcvSMSIndex = 0xFF;
				ModemManagerData.nCurrentSMSCount = 0;
				ModemManagerData.nMdmSubNextState = 2;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
                m_eSmsRcvStatus = eSmsRcvStatusNone;

                SetRequestSms2Gemalto(true);

                ClearNetworkDelayProcess();
			}
			break;

		case 2:

            // wait 5 seconds time out
            // because some time gemalto will reponse different sequence.
            // skip process
            if( GetRequestSms2Gemalto() == true && NetworkDelayProcess(2000) == false )
            {
                break;
            }

#if false
            if( NetworkDelayProcess(2000) == false )
            {
                //if( m_eSmsRcvStatus == eSmsRcvStatusNone )
                {
                    //skip until setting delay value
                    break;
                }
            }
#endif

			if(g_nCurrRcvSMSIndex == 0xFF) {
                Trace("End read sms\r\n");
				// 수신된 메시지가 없다.
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;

                ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;
				break;
			}

            m_eSmsRcvStatus = eSmsRcvStatusNone;
            ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;

			Trace("\nMDM: SMS Infomation: message count: %d\r\n", ModemManagerData.nCurrentSMSCount);
			i = 0;
			n = ModemManagerData.nCurrentSMSCount;
			while(i < MAX_SMS_COUNT) {
				// index
				if(ModemManagerData.stRcvSMSInfos[i].nIndex == NO_SMS_INDEX)
                {
					i++;
					continue;
				}

				Trace("index: %d\r\n", ModemManagerData.stRcvSMSInfos[i].nIndex);

				//status
				if(ModemManagerData.stRcvSMSInfos[i].eStatus == eSMS_STATUS_REC_UNREAD) {
					Trace("status: REC UNREAD\r\n");
				}
				else if(ModemManagerData.stRcvSMSInfos[i].eStatus == eSMS_STATUS_REC_READ) {
					Trace("status: REC READ\r\n");
				}
				else {
					Trace("status: UNKNOWN\r\n");
				}

				// Origination Address
				Trace("Origination Address: %s\r\n", ModemManagerData.stRcvSMSInfos[i].aOriginatingAddr);

				// time
				Trace("Rcv Time: %04d/%02d/%02d,%02d:%02d:%02d\r\n", ModemManagerData.stRcvSMSInfos[i].strTime.tm_year,
																	ModemManagerData.stRcvSMSInfos[i].strTime.tm_mon,
																	ModemManagerData.stRcvSMSInfos[i].strTime.tm_mday,
																	ModemManagerData.stRcvSMSInfos[i].strTime.tm_hour,
																	ModemManagerData.stRcvSMSInfos[i].strTime.tm_min,
																	ModemManagerData.stRcvSMSInfos[i].strTime.tm_sec);

				Trace("[SMS Message]\r\n");
				//hexdump(ModemManagerData.stRcvSMSInfos[i].aData, ModemManagerData.stRcvSMSInfos[i].nDataLen);

                boolean_t bDecryptSuccess = false;
                uint8_t arrSMSTempBuffer[384]={0,};
                uint8_t arrSMSBuffer[384]={0,};

                // find 1 byte header
                if( memcmp(ModemManagerData.stRcvSMSInfos[i].aData,m_carrPreableOfSmsCommander,2) == 0 )
                {
					if(gbGITSMSEnablePatialFlag == false) {
						gbGITSMSReceivedSuccessFlag = false;
						gbGITSMSEnablePatialFlag = false;
						gaGITSMSBufferLength = 0;

						// find preable of sms commander
						if(memcmp((char*)&ModemManagerData.stRcvSMSInfos[i].aData[ModemManagerData.stRcvSMSInfos[i].nDataLen-2],m_carrPreableOfSmsCommander,2) == 0 ) {
							// preable of commander
                            if( ModemManagerData.stRcvSMSInfos[i].nDataLen >= 4 )
                            {
                                gaGITSMSBufferLength = ModemManagerData.stRcvSMSInfos[i].nDataLen-4;
                            }
                            else
                            {
                                gaGITSMSBufferLength = ModemManagerData.stRcvSMSInfos[i].nDataLen;
                            }
							memcpy((char *)arrSMSBuffer, &ModemManagerData.stRcvSMSInfos[i].aData[2],gaGITSMSBufferLength);
							arrSMSBuffer[gaGITSMSBufferLength] = 0;

							gbGITSMSEnablePatialFlag = false;				// 하나의 SMS에 GUID가 포함 되어있다.
							gbGITSMSReceivedSuccessFlag = true;
						}
					}


                    if(gbGITSMSReceivedSuccessFlag == true) {

                        uint16_t nLen = gaGITSMSBufferLength;

                        gbGITSMSReceivedSuccessFlag = false;

                        //hexdump(arrSMSBuffer, nLen);

                        nLen = ApplyDecryption(arrSMSBuffer, nLen);

                        //hexdump(gaGITSMSBuffer, nLen);

                        bDecryptSuccess = true;
                    }
				}
                else
                {
                    // find 2 byte header
					uint8_t *ptr1;
					uint8_t *ptr2;

//					ptr1 = (uint8_t *)strstr((char *)ModemManagerData.stRcvSMSInfos[i].aData, "005B005700650062BC1CC2E0005D000A007C007C");				// "[WEB발신] <0x0a> ||" 문자열 찾기
					ptr1 = (uint8_t *)strstr((char *)ModemManagerData.stRcvSMSInfos[i].aData, m_pcarrPreableOfSmsCommander); // find preable

					if(ptr1 != NULL) {
						uint16_t len;

						if(gbGITSMSEnablePatialFlag == false) {
							gbGITSMSReceivedSuccessFlag = false;
							gbGITSMSEnablePatialFlag = false;
							gaGITSMSBufferLength = 0;

//							len = strlen("005B005700650062BC1CC2E0005D000A007C007C");
							len = strlen(m_pcarrPreableOfSmsCommander);
							ptr1 += len;
							ptr2 = (uint8_t *)strstr((char *)ptr1, m_pcarrPreableOfSmsCommander);				// find preable of sms commander
							if(ptr2 == NULL) {
								uint8_t *ptr3;

								// no preable of commander
								ptr3 = ModemManagerData.stRcvSMSInfos[i].aData;
								gaGITSMSBufferLength = ModemManagerData.stRcvSMSInfos[i].nDataLen - (ptr1 - ptr3);
								memcpy((char *)gaGITSMSBuffer, ptr1, gaGITSMSBufferLength);
								gaGITSMSBuffer[gaGITSMSBufferLength] = 0;

								gbGITSMSEnablePatialFlag = true;				// 다음 SMS에 까지 GUID가 연결 되어있다.

								//hexdump(gaGITSMSBuffer, gaGITSMSBufferLength);
							}
							else {
                                // preable of commander
								gaGITSMSBufferLength = ptr2 - ptr1;
								memcpy((char *)gaGITSMSBuffer, ptr1, gaGITSMSBufferLength);
								gaGITSMSBuffer[gaGITSMSBufferLength] = 0;

								gbGITSMSEnablePatialFlag = false;				// 하나의 SMS에 GUID가 포함 되어있다.
								gbGITSMSReceivedSuccessFlag = true;
							}
						}
						else {
							gbGITSMSEnablePatialFlag = false;

							len = strlen(m_pcarrPreableOfSmsCommander);
							ptr2 = (uint8_t *)strstr((char *)ModemManagerData.stRcvSMSInfos[i].aData, m_pcarrPreableOfSmsCommander);				// "||" 문자열 찾기
							if(ptr2 == NULL) {
								// 이번 문자열에는 "||" 문자열이 없다. 그러면, 버리자.
								gbGITSMSReceivedSuccessFlag = false;
							}
							else {
								memcpy((char *)&gaGITSMSBuffer[gaGITSMSBufferLength], (char *)ModemManagerData.stRcvSMSInfos[i].aData, ModemManagerData.stRcvSMSInfos[i].nDataLen - len);
								gaGITSMSBufferLength += ModemManagerData.stRcvSMSInfos[i].nDataLen - len;
								gaGITSMSBuffer[gaGITSMSBufferLength] = 0;

								gbGITSMSReceivedSuccessFlag = true;

//								hexdump(gaGITSMSBuffer, gaGITSMSBufferLength);
							}
						}
					}
					else {

						if(memcmp((char *)ModemManagerData.stRcvSMSInfos[i].aData, "101", 3) == 0) {
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_VEHICLE_START_UP;
						}
						else if(memcmp((char *)ModemManagerData.stRcvSMSInfos[i].aData, "102", 3) == 0) {
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_VEHICLE_STOP;
						}
						else if(memcmp((char *)ModemManagerData.stRcvSMSInfos[i].aData, "103", 3) == 0) {
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_VEHICLE_DOORLOCK;
						}
						else if(memcmp((char *)ModemManagerData.stRcvSMSInfos[i].aData, "104", 3) == 0) {
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_VEHICLE_DOORUNLOCK;
						}
						else if(memcmp((char *)ModemManagerData.stRcvSMSInfos[i].aData, "888", 3) == 0) {
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_REMOTE;
						}
						else if(memcmp((char *)ModemManagerData.stRcvSMSInfos[i].aData, "999", 3) == 0) {
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_VEHICLE_RESET;
						}
					}
				}

				if(gbGITSMSReceivedSuccessFlag == true) {
					uint16_t nLen;

					gbGITSMSReceivedSuccessFlag = false;

					{
						uint16_t x, y;

						// 두바이트 HEX ASCII 문자 형태로 수신된 경우
						for(x = 2, y = 0; x < gaGITSMSBufferLength; x += 3, y++) {
							arrSMSTempBuffer[y] = gaGITSMSBuffer[x];
							x++;
							y++;
							arrSMSTempBuffer[y] = gaGITSMSBuffer[x];
						}

						nLen = y;
//						hexdump(arrSMSTempBuffer, nLen);

						Convert_HexString_To_Bin((char *)arrSMSBuffer, (char *)arrSMSTempBuffer, nLen / 2);

						nLen = nLen / 2;
					}

//					hexdump(arrSMSBuffer, nLen);

					nLen = ApplyDecryption(arrSMSBuffer, nLen);

//					hexdump(arrSMSBuffer, nLen);
                    bDecryptSuccess = true;
				}

				if( bDecryptSuccess == true )
				{
                    stHalRTCTypeDef stDate;
                    long long llTemp = 0;
                    char aTempBuffer[64]={0,};
                    // sms format : XXYYYYYYYYYYYYZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ
                    // X : COMMAND  [0:1]
                    // Y : TIME     [2:13]
                    // Z : GUID     [14:45]
                    // command
                    if(arrSMSBuffer[0] == '4' && arrSMSBuffer[1] == '0') {
                        // remote control
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_REMOTE;
                    }
					else if(arrSMSBuffer[0] == '4' && arrSMSBuffer[1] == '8') {
                        // sms test
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_REMOTE_TIME_IGNORE;
                    }
                    else if(arrSMSBuffer[0] == '3' && arrSMSBuffer[1] == '9') {
                        // sms test
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_SMS_TEST;
                    }
                    else if(arrSMSBuffer[0] == '6' && arrSMSBuffer[1] == '5') {
                        // geofence setting
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_SETTING_GEOFENCE;
                    }
                    else if(arrSMSBuffer[0] == '6' && arrSMSBuffer[1] == '6') {
                        // geofence setting
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_SETTING_POLYGON_GEOFENCE;
                    }
                    else if(arrSMSBuffer[0] == '7' && arrSMSBuffer[1] == '0') {
                        // get car status
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_VEHICLE_STATUS;
                    }
					else if(arrSMSBuffer[0] == '8' && arrSMSBuffer[1] == '5') {
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_TRACKINGMODE;
                    }
#if defined(PROTOCOL12)
                    else if(arrSMSBuffer[0] == '9' && arrSMSBuffer[1] == '1') {
                        // modemm activation request
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_MODEM_ACTIVATE;
                    }
#endif
#if defined(PROTOCOL15)
                    else if(arrSMSBuffer[0] == '9' && arrSMSBuffer[1] == '3') {
                        // modemm activation request
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_SENSOR_INITIALIZE;
                    }
#endif
#if defined(PROTOCOL17)
					else if(arrSMSBuffer[0] == '4' && arrSMSBuffer[1] == '5') {
                        // modemm activation request
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_SETURL;
                    }
					else if(arrSMSBuffer[0] == '8' && arrSMSBuffer[1] == '2') {
                        // modemm activation request
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_SETURL_INIT;
                    }
#endif
#if defined(PROTOCOL18)
					else if(arrSMSBuffer[0] == '9' && arrSMSBuffer[1] == '5') {
                        // modemm activation request
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_SETTING_RSVENGCTRL;
                    }
#endif
#if defined(PROTOCOL25)
                    else if(arrSMSBuffer[0] == '5' && arrSMSBuffer[1] == '9') {
                        // modemm activation request
                        ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_INSTOLATION_REQUEST_SMS_TEST;
                    }
#endif
					else if(arrSMSBuffer[0] == '9' && arrSMSBuffer[1] == '7') {

					}
					else if(arrSMSBuffer[0] == '9' && arrSMSBuffer[1] == '8') {

					}
					else if(arrSMSBuffer[0] == '9' && arrSMSBuffer[1] == '9') {
                        sprintf((char *)aTemp, "%d, 0\x00", ModemManagerData.stRcvSMSInfos[i].nIndex);				// 0: Delete the message specified index
						Md_DeleteSMSIndex(aTemp);
						APP_Delay(1000);
						Send2MngSysMsg(eMngSys,eReqIpek,eIpekPhase1,(stCarReport *)NULL,0);
					}
					else if(arrSMSBuffer[0] == 'r' && arrSMSBuffer[1] == 'e' && arrSMSBuffer[2] == 's' && arrSMSBuffer[3] == 'e' && arrSMSBuffer[4] == 't') {
                        //emergency code
						sprintf((char *)aTemp, "%d, 0\x00", ModemManagerData.stRcvSMSInfos[i].nIndex);				// 0: Delete the message specified index
						Md_DeleteSMSIndex(aTemp);
						APP_Delay(1000);
						TestFormat();
						SystemForcelyReset_PowerOn();
					}
					else
					{
						ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;
					}
                    ModemManagerData.unSmsRcvTime = GetLocalTimefromTime(GetUTCTime());

                    // time
                	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&arrSMSBuffer[2], 6);
                	llTemp = 0;
                	llTemp |= aTempBuffer[0];
                	llTemp <<= 8;
                	llTemp |= aTempBuffer[1];
                	llTemp <<= 8;
                	llTemp |= aTempBuffer[2];
                	llTemp <<= 8;
                	llTemp |= aTempBuffer[3];
                	llTemp <<= 8;
                	llTemp |= aTempBuffer[4];
                	llTemp <<= 8;
                	llTemp |= aTempBuffer[5];

                    //itoa_1(llTemp, (char *)stRemoteCommand.arrDateTime, 10);
                    sprintf((char *)aTempBuffer,"%lld\x00",llTemp);
                    printf("Occurred Event Time from text: %s\r\n",aTempBuffer);
                    GetDatefromDateArray((char *)aTempBuffer,&stDate);
                    ModemManagerData.unSmsTimeOutTime = GetTimefromDate2(stDate);

                    // guid
                    memcpy((char *)MessageManagerData.aRemoteControlGUID, (char *)&arrSMSBuffer[14], MAX_GUID_LENGTH);
					Trace("[GUID]\r\n");
					//hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
#ifdef USE_OLD_SMS_FORMAT
					if(memcmp((char *)&arrSMSBuffer[2], "autolink", 8) == 0) {
						memcpy((char *)MessageManagerData.aRemoteControlGUID, (char *)&arrSMSBuffer[10], MAX_GUID_LENGTH);
						Trace("[GUID]\r\n");
						hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
						if(arrSMSBuffer[0] == '4' && arrSMSBuffer[1] == '0') {
							// 문자 메시지로 차량 제어를 요청한 경우
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_REMOTE;
						}
						else if(arrSMSBuffer[0] == '6' && arrSMSBuffer[1] == '5') {
							// request setting geofence from server
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_SETTING_GEOFENCE;
						}
						else if(arrSMSBuffer[0] == '7' && arrSMSBuffer[1] == '0') {
							// 문자 메시지로 차량의 상태 정보를 요청한 경우
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_VEHICLE_STATUS;
						}
					}
                    else
                    {
                        memcpy((char *)MessageManagerData.aRemoteControlGUID, (char *)&arrSMSBuffer[14], MAX_GUID_LENGTH);
						Trace("[GUID]\r\n");
						hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
						if(arrSMSBuffer[0] == '3' && arrSMSBuffer[1] == '9') {
							// requst sms test for app install
							ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_REQUEST_SMS_TEST;
						}
#if defined(PROTOCOL25)
                        if(arrSMSBuffer[0] == '5' && arrSMSBuffer[1] == '9') {
                            // requst sms test for app install
                            ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_INSTOLATION_REQUEST_SMS_TEST;
                        }
#endif
                    }
#endif
				}
				i++;
				n--;
			}

			Trace("\r\n");

			nIndex = 0;
			ModemManagerData.nMdmSubState = 3;
            ModemManagerData.nMdmSubNextState = 3;
			break;

		case 3:
            //Trace("sms : step#3\r\n");
			if(ModemManagerData.nCurrentSMSCount > 0) {
                if(nIndex > 20) {
                    //Trace("sms : step#5\r\n");
                    ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
					break;
				}
                if(ModemManagerData.stRcvSMSInfos[nIndex].nIndex == NO_SMS_INDEX)
                {
                    //Trace("sms : step#4\r\n");
					nIndex++;
					break;
				}
#if true
				Trace("SMS index: %d\r\n", ModemManagerData.stRcvSMSInfos[nIndex].nIndex);
//				sprintf((char *)aTemp, "%d, 4\x00", ModemManagerData.stRcvSMSInfos[nIndex].nIndex);				// 4: Delete all messages from preferred message storage including unread messages.
				sprintf((char *)aTemp, "%d, 0\x00", ModemManagerData.stRcvSMSInfos[nIndex].nIndex);				// 0: Delete the message specified index
				if(Md_DeleteSMSIndex(aTemp) == true) {
					ModemManagerData.nMdmSubNextState = 4;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
#endif
			}
			else {
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			}
			break;

		case 4:
			if(ModemManagerData.nCurrentSMSCount > 0) {
				ModemManagerData.nCurrentSMSCount--;
                ModemManagerData.stRcvSMSInfos[nIndex].nIndex = NO_SMS_INDEX; // SMS가 없는 것을 의미
				nIndex++;

				ModemManagerData.nMdmSubState = 3;
			}
			else {
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
                ClearNetworkDelayProcess();
                APP_Delay(10);
			}
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_END_PROCESS:
            if( NetworkDelayProcess(100) == false )
            {
                break;
            }

			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

void GetNowUtcTimeString(char* buffer)
{
    long long llValue;
    stHalRTCTypeDef stHalRtcDateTime;
    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRtcDateTime), 0);

    GetDateTimeStamp((uint8_t *)buffer, stHalRtcDateTime.RtcDate, stHalRtcDateTime.RtcTime);

    llValue = atoll((char *)buffer);
    ConversionLongLongToHexDec_6(llValue, 0, (uint8_t *)buffer);
}

eMODEM_PROCESS_FUNC_RET ProcessCheckSMSResult(void)
{
    stCarReport stReport;
    Trace("request a message for sending a possible\r\n");

    if( ModemManagerData.eModemMessageMode != eMODEM_MESSAGE_MODE_NONE )
    {
        memset((char*)&stReport,0,sizeof(stReport));
        memcpy((char*)stReport.rpSmartKey.Request.Guid,(char *)MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);

        DisplayTime("sms utc Time",ModemManagerData.unSmsTimeOutTime);
        stReport.rpSmartKey.Request.OccurredEventTime = ConvertUtc2LocalTime(ModemManagerData.unSmsTimeOutTime);
		stReport.rpSmartKey.Request.OccurredEventUtcTime = ModemManagerData.unSmsTimeOutTime;
        DisplayTime("sms local Time",stReport.rpSmartKey.Request.OccurredEventTime);

    	switch(ModemManagerData.eModemMessageMode) {
    		case eMODEM_MESSAGE_MODE_REQUEST_REMOTE:
				stReport.rpSmartKey.Request.CommandType = 0x40;
                Send2MngModem(eMngModem,eRcvSms,eR_ReqSmartKey,&stReport,0);
    			break;
			case eMODEM_MESSAGE_MODE_REQUEST_REMOTE_TIME_IGNORE:
				stReport.rpSmartKey.Request.CommandType = 0x48;
                Send2MngModem(eMngModem,eRcvSms,eR_ReqSmartKey,&stReport,0);
				break;
            case eMODEM_MESSAGE_MODE_REQUEST_SETTING_GEOFENCE:
				stReport.rpSmartKey.Request.CommandType = 0x65;
                Send2MngModem(eMngModem,eRcvSms,eR_SettingGeofence,&stReport,0);
                break;
            case eMODEM_MESSAGE_MODE_REQUEST_SETTING_POLYGON_GEOFENCE:
				stReport.rpSmartKey.Request.CommandType = 0x66;
                Send2MngModem(eMngModem,eRcvSms,eR_SettingPolygonGeofence,&stReport,0);
                break;
    		case eMODEM_MESSAGE_MODE_REQUEST_VEHICLE_STATUS:
				stReport.rpSmartKey.Request.CommandType = 0x70;
                Send2MngModem(eMngModem,eRcvSms,eR_CurrentVehicleStatus,&stReport,0);
    			break;
            case eMODEM_MESSAGE_MODE_REQUEST_SMS_TEST:
				stReport.rpSmartKey.Request.CommandType = 0x39;
                Send2MngModem(eMngModem,eRcvSms,eR_SmsTest,&stReport,0);
                break;
#if defined(PROTOCOL12)
            case eMODEM_MESSAGE_MODE_REQUEST_MODEM_ACTIVATE:
				stReport.rpSmartKey.Request.CommandType = 0x91;
                Send2MngModem(eMngModem,eRcvSms,eR_ReqModemActivate,&stReport,0);
                break;
#endif
#if defined(PROTOCOL15)
            case eMODEM_MESSAGE_MODE_REQUEST_SENSOR_INITIALIZE:
				stReport.rpSmartKey.Request.CommandType = 0x93;
                Send2MngModem(eMngModem,eRcvSms,eR_ReqSensorInitialize,&stReport,0);
                break;
#endif
#if defined(PROTOCOL17)
			case eMODEM_MESSAGE_MODE_REQUEST_SETURL:
				stReport.rpSmartKey.Request.CommandType = 0x45;
                Send2MngModem(eMngModem,eRcvSms,eR_ReqSetURL,&stReport,0);
                break;
			case eMODEM_MESSAGE_MODE_REQUEST_SETURL_INIT:
				stReport.rpSmartKey.Request.CommandType = 0x82;
				Send2MngSysMsg(eMngModem,eReqSysSetting,eR_ReqSetURLInit,&stReport,0);
                break;
#endif
#if defined(PROTOCOL18)
			case eMODEM_MESSAGE_MODE_REQUEST_SETTING_RSVENGCTRL:
				stReport.rpSmartKey.Request.CommandType = 0x95;
				Send2MngModem(eMngModem,eRcvSms,eR_ReqSettingRsvEngCtrl,&stReport,0);
				break;
#endif
			case eMODEM_MESSAGE_MODE_REQUEST_TRACKINGMODE:
				stReport.rpSmartKey.Request.CommandType = 0x85;
                Send2MngModem(eMngModem,eRcvSms,eR_SetTrackingMode,&stReport,0);
    			break;
#if defined(PROTOCOL25)
            case eMODEM_MESSAGE_MODE_INSTOLATION_REQUEST_SMS_TEST:
                stReport.rpSmartKey.Request.CommandType = 0x59;
                Send2MngModem(eMngModem,eRcvSms,eR_ReportInstallationSMSCheck,&stReport,0);
                break;
#endif

    		//*************************************************************************************
    		// 아래는 문자 메시지로 차량을 바로 제어할 수 있는 테스트 코드임, 실제로 사용하지 않음
    		//*************************************************************************************
    //#ifdef USE_DIRECT_CONTROL_CAR
    //#warning "추후에 반드시 삭제 할 것"
    //		case eMODEM_MESSAGE_MODE_VEHICLE_START_UP:
    //			Set_Actuator(ACTUATOR_TYPE_ENGINERUN);
    //			ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;
    //			Trace("VEHICLE: Start-up...\r\n");
    //			break;
    //
    //		case eMODEM_MESSAGE_MODE_VEHICLE_STOP:
    //			Set_Actuator(ACTUATOR_TYPE_ENGINESTOP);
    //			ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;
    //			Trace("VEHICLE: Stop...\r\n");
    //			break;
    //
    //		case eMODEM_MESSAGE_MODE_VEHICLE_DOORLOCK:
    //			Set_Actuator(ACTUATOR_TYPE_DOORLOCK);
    //			ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;
    //			Trace("VEHICLE: Lock...\r\n");
    //			break;
    //
    //		case eMODEM_MESSAGE_MODE_VEHICLE_DOORUNLOCK:
    //			Set_Actuator(ACTUATOR_TYPE_DOORUNLOCK);
    //			ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;
    //			Trace("VEHICLE: Unlock...\r\n");
    //			break;
    //
    //		case eMODEM_MESSAGE_MODE_VEHICLE_RESET:
    //			ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;
    //			Trace("SYSTEM Reset\r\n");
    //			SystemSoftwareReset();
    //			break;
    //#endif //#ifdef USE_DIRECT_CONTROL_CAR
    		//***************************************************************
    	}
    }

    ModemManagerData.eModemMessageMode = eMODEM_MESSAGE_MODE_NONE;

	SetModemState(eMODEM_READY);

    // release network check
    SetReqWaitNetworkCheck(false);
    // send message for sending a possible to send
    SetNetworkPostPoneFlag(false);
    Send2MngSysMsg(eMngModem,eReqPostpond,eFalse,(stCarReport *)NULL,0);

	return eMODEM_PROCESS_FUNC_RET_OK;
}

int GetNetworkDelay()
{
    // datasheet에 명시된 RSSI 값과 dBm의 상관관계를 수식으로 표현하면
    // y = 2x - 113 이다. y: dBm, x: RSSI
//    ModemManagerData.nRSSI;
//    ModemManagerData.ndBm;

    if( ModemManagerData.ndBm >= -80 )
        return 0;
    else if ( ModemManagerData.ndBm >= -85 )
        return 10;
    else if ( ModemManagerData.ndBm >= -90 )
        return 20;
    else //if ( ModemManagerData.ndBm <= -95 )
        return 30;
}


/*  return  true : go to next steps
            false : skip next steps
*/
bool NetworkDelayProcess(int nDelay)
{
    //nDelay = 200;//GetNetworkDelay();

    // MONI modem rx Skip Process
    if( m_ulRxTimeStamp == 0 )
    {
        m_ulRxTimeStamp  = Get_Tmr();
    }

    if( Get_TmrDelta(Get_Tmr(), m_ulRxTimeStamp) < nDelay )
        return false;

    m_ulRxTimeStamp = 0;

    return true;
}

void ClearNetworkDelayProcess()
{
    m_ulRxTimeStamp = 0;
}

eMODEM_PROCESS_FUNC_RET ProcessHTTPComm(void)
{
  uint8_t aTemp[256]={0,};
    uint8_t carrApn[128]={0,};

	switch(ModemManagerData.nMdmSubState) {
		case eHTTP_COMM_STATE_INIT:
			if(ModemManagerData.eNetworkRegStatus != eNETWORK_REG_STATUS_REGISTERED
#ifdef  ENABLE_ROAMING_PROCESS
                && ModemManagerData.eNetworkRegStatus != eNETWORK_REG_STATUS_REG_ROAMING
#endif
                ) {
				Trace("\n\nMDM: not network registration.\n\r\n");
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_INIT;
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_INIT;
				return eMODEM_PROCESS_FUNC_RET_FAIL;
			}

//			if(ModemManagerData.nRSSI < MODEM_SIGNAL_QUILITY_STABLE_VALUE) {
//				// RSSI 값이 MODEM_SIGNAL_QUILITY_STABLE_VALUE 보다 작을 경우에는 Message 전송을 수행하지 않도록 한다.
//				Trace("\n\nMSG: low signal quility.(%d)\n\r\n", ModemManagerData.nRSSI);
//
//				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_INIT;
//
//				return eMODEM_PROCESS_FUNC_RET_FAIL;
//			}

			ModemManagerData.bWriteReady = false;
			ModemManagerData.bConfirmWriteLen = false;
			ModemManagerData.bFinishWrite = false;
			ModemManagerData.bReadyReadData = false;
			ModemManagerData.bEndOfData = false;
			ModemManagerData.bServiceOpen = false;
			ModemManagerData.bExpectEndOfData = false;

			gwModemCommRxDataLength = 0;
			gwTotalReceiveBinDataLength = 0;

			// 이전에 service가 open 된 이력이 있는지 검사한다.
			if(BkSram_ModemInfo.bHTTPSuccessServiceConnection[ModemManagerData.nInternetServiceProfileId] == false) {
				Trace(" clear connetion configuration\r\n");
#ifdef RF_COMMON_MODEM //mod.kks 21.12.30
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SET_SERVICE_CLEAR;
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_CLEAR;
#else
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SET_CONNECTION_CLEAR;
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_CONNECTION_CLEAR;
#endif
			}
			else {
                if(ModemManagerData.eCurrentMessageSendingType == ModemManagerData.ePreviousMessageSendingType) {
                	Trace(" open connetion\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.10.26
                 // ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SET_SECSNI;
                  ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CON_ACTIVE; //mod.kks to add the status....
                  ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CON_ACTIVE;
#else
                  ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SOCKET_OPEN;
                  ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SOCKET_OPEN;
#endif
                }
                else {
                	Trace(" modify connetion configuration\r\n");
#ifdef RF_COMMON_MODEM //mod.kks 21.12.23  fix ths pls63W spec....
		          ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CON_ACTIVE;
                  ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CON_ACTIVE;
#else
                  ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SET_SERVICE_URL;
                  ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_URL;
#endif
                }
			}
			break;

		case eHTTP_COMM_STATE_WAITING:
			// 각 명령에 대한 OK 응답을 대기한다. 실제 응답 체크는 ModemManager의 "eMODEM_RunProcess" state에서 검사한다.
			break;

		case eHTTP_COMM_STATE_SET_CONNECTION_CLEAR:
            sprintf((char *)aTemp, "%d,conType,none\x00", ModemManagerData.nInternetConnectionProfileId);

			if(Md_SetInternetConnectionService(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_CONNECTION_PROFILE;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_SET_CONNECTION_PROFILE:
			sprintf((char *)aTemp, "%d,conType,GPRS0\x00", ModemManagerData.nInternetConnectionProfileId);

			if(Md_SetInternetConnectionService(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_CONNECTION_APN;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_SET_CONNECTION_APN:
			//MONI 2018-02-09
			if( GetUserApn((char *)carrApn) == false )
			{
                GetBackupRamConfigProperty(eBackupRamConfig_Apn,(void*)carrApn);
			}

            sprintf((char *)aTemp, "%d,apn,\"%s\"\x00", ModemManagerData.nInternetConnectionProfileId, carrApn);

			if(Md_SetInternetConnectionService(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_CLEAR;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_SET_SERVICE_CLEAR:
#ifdef RF_COMMON_MODEM //mod.kks 21.12.30
			sprintf((char *)aTemp, "%d, \"srvType\",\"none\"", ModemManagerData.nInternetServiceProfileId);
#else
            sprintf((char *)aTemp, "%d,srvType,none\x00", ModemManagerData.nInternetServiceProfileId);
#endif

			if(Md_SetInternetServiceProfile(aTemp) == true) {
#ifdef RF_COMMON_MODEM // mod.kks 21.10.27
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CON_ACTIVE;
#else
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_PROFILE;
#endif
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
#ifdef RF_COMMON_MODEM
		/*Connection Activation */  //mod.kks 21.10.27 Only apply the RF Tracker PLSx3.
		case eHTTP_COMM_STATE_CON_ACTIVE:
			if(Md_ConActive("1,1") == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_PROFILE;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
#endif
		case eHTTP_COMM_STATE_SET_SERVICE_PROFILE:
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			sprintf((char *)aTemp, "%d,\"srvType\",\"Http\"", ModemManagerData.nInternetServiceProfileId);
#else
			sprintf((char *)aTemp, "%d,srvType,http\x00", ModemManagerData.nInternetServiceProfileId);
#endif

			if(Md_SetInternetServiceProfile(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_USED;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_SET_SERVICE_USED:
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			sprintf((char *)aTemp, "%d,\"conId\",\"%d\"", ModemManagerData.nInternetServiceProfileId, /*ModemManagerData.nInternetConnectionProfileId*/ 1);
			if(Md_SetInternetServiceProfile(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_ALPHABET;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
		case eHTTP_COMM_STATE_SET_SERVICE_ALPHABET:
			sprintf((char *)aTemp, "%d,\"alphabet\",\"%d\"", ModemManagerData.nInternetServiceProfileId, /*ModemManagerData.nInternetConnectionProfileId*/ 1);
			if(Md_SetInternetServiceProfile(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_URL;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
#else
			sprintf((char *)aTemp, "%d,conId,%d\x00", ModemManagerData.nInternetServiceProfileId, ModemManagerData.nInternetConnectionProfileId);
			if(Md_SetInternetServiceProfile(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_URL;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
#endif



		case eHTTP_COMM_STATE_SET_SERVICE_URL:
#ifdef  ENABLE_URL_HTTPS
            if( ModemManagerData.eCurrentMessageSendingType != eMESSAGE_APGS_DATA )
            {
#if defined(PROTOCOL17)
				// HTTPS
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
				sprintf((char *)aTemp, "%d,\"address\",\"https://%s/%s\"\x00",
#else
    			sprintf((char *)aTemp, "%d,address,https://%s/%s\x00",
#endif
									ModemManagerData.nInternetServiceProfileId,
									ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL,
									ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath);
#else
    			// HTTPS
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
				sprintf((char *)aTemp, "%d,\"address\",\"https://%s:%d/%s\"\x00",
#else
    			sprintf((char *)aTemp, "%d,address,https://%s:%d/%s\x00",
#endif
									ModemManagerData.nInternetServiceProfileId,
									ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL,
									MODEM_REMOTE_PORT_NO_HTTPS,
									ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath);
#endif
            }
            else
            {

    			// HTTPS
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
				sprintf((char *)aTemp, "%d,\"address\",\"http://%s/%s\"\x00",
#else
    			sprintf((char *)aTemp, "%d,address,\"http://%s/%s\"\x00",
#endif
									ModemManagerData.nInternetServiceProfileId,
									ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL,
									ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath);
            }
#else // ENABLE_URL_HTTP
			// HTTP
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			sprintf((char *)aTemp, "%d,\"address\",\"http://%s:%d/%s\"\x00",
#else
			sprintf((char *)aTemp, "%d,address,http://%s:%d/%s\x00",
#endif
															ModemManagerData.nInternetServiceProfileId,
															ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL,
															MODEM_REMOTE_PORT_NO_HTTP,
															ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath);
#endif

			if(Md_SetInternetServiceProfile(aTemp) == true) {
                if(BkSram_ModemInfo.bHTTPSuccessServiceConnection[ModemManagerData.nInternetServiceProfileId] == false) {
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
                  ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SECOPT;
#else
                  ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_CMD;
#endif

                }
                else {
                  if(ModemManagerData.eCurrentMessageSendingType == ModemManagerData.ePreviousMessageSendingType) {
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
                    ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SECOPT;
#else
                    ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_CMD;
#endif
                  }
                  else {
#ifdef RF_COMMON_MODEM  //mod.kks 21.10.26
                          ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SECOPT; //eHTTP_COMM_STATE_SET_SERVICE_HCCONLEN; 21.12.30 mod.kks to resolve the ^sis 2,0,57 issue.
#else
                          ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SOCKET_OPEN;
#endif
                  }
                }

                ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
#ifdef RF_COMMON_MODEM
	case eHTTP_COMM_STATE_SET_SECOPT:
            sprintf((char *)aTemp, "%d,\"secopt\",\"0\"", ModemManagerData.nInternetServiceProfileId);

			if(Md_SetInternetServiceProfile(aTemp) == true) {
				// set response handler  before send data ME must set response handler to check
				// wait tx and response
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_CMD;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
#endif
		case eHTTP_COMM_STATE_SET_SERVICE_CMD:
            if( ModemManagerData.eCurrentMessageSendingType != eMESSAGE_APGS_DATA )
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			    sprintf((char *)aTemp, "%d,\"cmd\",\"post\"", ModemManagerData.nInternetServiceProfileId);
#else
			    sprintf((char *)aTemp, "%d,cmd,post\x00", ModemManagerData.nInternetServiceProfileId);
#endif
            else
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			    sprintf((char *)aTemp, "%d,\"cmd\",\"get\"", ModemManagerData.nInternetServiceProfileId);
#else
                sprintf((char *)aTemp, "%d,cmd,get\x00", ModemManagerData.nInternetServiceProfileId);
#endif

			if(Md_SetInternetServiceProfile(aTemp) == true) {
#ifdef RF_COMMON_MODEM //mod.kks 21.11.16
				if(ModemManagerData.eCurrentMessageSendingType != eMESSAGE_APGS_DATA)
					ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_HCCONTENT;
				else // RF_COMMON_MODEM & AGPS DATA
					ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_HCPROP;
#else
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_HCCONTENT;
#endif
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_SET_SERVICE_HCCONTENT:
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
            sprintf((char *)aTemp, "%d,\"hcContent\",\"\"", ModemManagerData.nInternetServiceProfileId);
#else
			sprintf((char *)aTemp, "%d,hcContent,\"\"\x00", ModemManagerData.nInternetServiceProfileId);
#endif

			if(Md_SetInternetServiceProfile(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_HCCONLEN;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_SET_SERVICE_HCCONLEN:
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
			sprintf((char *)aTemp, "%d,\"hcContLen\",\"%d\"", ModemManagerData.nInternetServiceProfileId, ModemManagerData.nWriteDataLength);
#else
			sprintf((char *)aTemp, "%d,hcContLen,1\x00", ModemManagerData.nInternetServiceProfileId);
#endif

			if(Md_SetInternetServiceProfile(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SERVICE_HCPROP;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_SET_SERVICE_HCPROP:
#if false
            if( ModemManagerData.eCurrentMessageSendingType != eMESSAGE_APGS_DATA )
            {
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
				sprintf((char *)aTemp, "%d,\"hcProp\",\"%s\"\x00", ModemManagerData.nInternetServiceProfileId,
#else
    			sprintf((char *)aTemp, "%d,hcProp,\"%s\"\x00", ModemManagerData.nInternetServiceProfileId,
#endif
                                    ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp);

    			if(Md_SetInternetServiceProfile(aTemp) == true) {
#ifdef RF_COMMON_MODEM  //mod.kks 21.10.26
      	            ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SECSNI;
#else
    				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SOCKET_OPEN;
#endif //RF_COMMON_MODEM
    				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
    			}
            }
            else
            {
                ModemManagerData.bHTTPRcvPostURL = false;
#ifdef RF_COMMON_MODEM  //mod.kks 21.10.26
                ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SET_SECSNI;
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SECSNI;
#else
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SOCKET_OPEN;
    			ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SOCKET_OPEN;
#endif //RF_COMMON_MODEM
            }
#endif //false ..mod.kks please point mark.......
            // fota mode
            if((ModemManagerData.eFOTAStartState == eFOTA_START_STATE_GET_VEHICLE_INFO)
               ||(ModemManagerData.eFOTAStartState == eFOTA_START_STATE_GET_BIN)
               ||(ModemManagerData.eFOTAStartState == eFOTA_START_STATE_GET_VERSION))
            {
                strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, NEWFOTA_HTTP_MESSAGE_HEADER_CONTENT);
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
				sprintf((char *)aTemp, "%d,\"hcProp\", \"%s\\0d\\0a%s\"", ModemManagerData.nInternetServiceProfileId,
#else
				sprintf((char *)aTemp, "%d,hcProp, \"%s\\0d\\0a%s\"", ModemManagerData.nInternetServiceProfileId,
#endif
						ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, NEWFOTA_HTTP_MESSAGE_HEADER_AUTH);
				if(Md_SetInternetServiceProfile(aTemp) == true) {
#ifdef RF_COMMON_MODEM  //mod.kks 21.10.26
      	            ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SECSNI;
#else
                	ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SOCKET_OPEN;
#endif //RF_COMMON_MODEM
                	ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
            	}
            }
			else
			{
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
            	sprintf((char *)aTemp, "%d,\"hcProp\",\"%s\"\x00", ModemManagerData.nInternetServiceProfileId,
#else
            	sprintf((char *)aTemp, "%d,hcProp,\"%s\"\x00", ModemManagerData.nInternetServiceProfileId,
#endif
                                                ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp);

				if(Md_SetInternetServiceProfile(aTemp) == true) {
#ifdef RF_COMMON_MODEM  //mod.kks 21.10.26
                    ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SECSNI;
#else
                	ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SOCKET_OPEN;
#endif //RF_COMMON_MODEM
                	ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
				}
			}


			break;
#ifdef RF_COMMON_MODEM
		case eHTTP_COMM_STATE_SET_SECSNI:
			sprintf((char *)aTemp, "%d,\"secsni\",\"1\"", ModemManagerData.nInternetServiceProfileId);

			if(Md_SetInternetServiceProfile(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_SNINAME;
               	ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_SET_SNINAME:
			sprintf((char *)aTemp, "%d,\"sniname\",\"%s\"\x00",
									ModemManagerData.nInternetServiceProfileId,
									ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL);

			if(Md_SetInternetServiceProfile(aTemp) == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_GET_PDPADDRESS;
               	ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
		case eHTTP_COMM_STATE_GET_PDPADDRESS:
			if(Md_GetPDPAddress() == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_GET_CPOS;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
		/* GET the COPS */
		case eHTTP_COMM_STATE_GET_CPOS:
			if(Md_ReadOperatorSelection() == true) {
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_GET_SMONI;
               	ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
		/* GET the SMONI */
		case eHTTP_COMM_STATE_GET_SMONI:
			if(Md_GetServiceMonitor() == true) {
		   		ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SOCKET_OPEN;
               	ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;
#endif
		// service open
		case eHTTP_COMM_STATE_SOCKET_OPEN:
#ifdef RF_COMMON_MODEM //21.12.30 mod.kks to delay PLS63W
            sprintf((char *)aTemp, "%d", ModemManagerData.nInternetServiceProfileId);
#else
            sprintf((char *)aTemp, "%d\x00", ModemManagerData.nInternetServiceProfileId);
#endif
			if(Md_SetInternetSocketOpen(aTemp) == true) {
				// service를 open 하게 되면, ^SISS, ^SISW URC가 입력된다.
				// 이후에 OK 응답이 온다.
				ModemManagerData.bServiceOpen = false;				// ^SIS: id,0,2200,"Http fileup.gitauto.com:80" 정상 수신 여부 확인용
				ModemManagerData.bWriteReady = false;					// ^SISW: id,1 정상 수신 여부 확인용 -> write ready를 의미한다.
				g_bCompleteDataFlag = false;

				HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_URC_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_URC_CallBack, true);

				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_WAIT_OPEN_SUCCESS;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;				// OK 응답 대기
			}
			break;

		case eHTTP_COMM_STATE_WAIT_OPEN_SUCCESS:
			if(ModemManagerData.bServiceOpen == true) {				// service open 이후에, ^SIS: id,0,2200,"Http fileup.gitauto.com:80" 수신 여부 확인
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAIT_WRITE_READY;
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_WAIT_WRITE_READY;
			}
			break;

		case eHTTP_COMM_STATE_WAIT_WRITE_READY:

            if( ModemManagerData.eCurrentMessageSendingType != eMESSAGE_APGS_DATA )
            {
    			if(ModemManagerData.bWriteReady == true) {				// service open 이후에, ^SISW: id,1 수신 여부 확인 write ready를 의미한다.
    				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);

                    if( ModemManagerData.nWriteDataLength < MAX_WRITE_DATA_AT_ONCE )
    				{    
    				    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SET_WRITE_DATA_LEN;
                      ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_WRITE_DATA_LEN;
                    }
                    else
                    {
                        ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SET_MULTI_WRITE_DATA_LEN;
                        ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_MULTI_WRITE_DATA_LEN;
                        ModemManagerData.nSubWriteDataLength = ModemManagerData.nWriteDataLength;
                        ModemManagerData.nSubWriteIndex = 0;
                    }
    			}
            }
            else
            {
                // ^SIS: id,0,2200,"HTTP POST Response: 200"			--> 수신 되면 true
				ModemManagerData.bHTTPRcvPostResponse = true;
                ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_AGPS_READY_TO_READ;
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_AGPS_READY_TO_READ;
            }
			break;

		// write 할 data length 전송
		case eHTTP_COMM_STATE_SET_WRITE_DATA_LEN:

			sprintf((char *)aTemp, "%d,%d\x00", ModemManagerData.nInternetServiceProfileId, ModemManagerData.nWriteDataLength);

			// write 할 data length를 전송한다. 정상적으로 전송할 데이터 크기가 반환이 되면, 그때 실제 data를 write 하게 된다.
			if(Md_SetInternetSocketWrite(aTemp) == true) {
				ModemManagerData.bConfirmWriteLen = false;

				Trace(" *eHTTP_COMM_STATE_CONFIRM_WRITE_LEN\r\n");
				// OK 응답을 기다리면 안된다.
				// ^SISW: ?,?,? 응답이 온 이후에 바로 data를 전송해야한다.
				// data 전송이 완료된 이후에 write 명령에 대한 OK 응답이 온다.
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CONFIRM_WRITE_LEN;
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CONFIRM_WRITE_LEN;
			}
			break;

		case eHTTP_COMM_STATE_CONFIRM_WRITE_LEN:
			if(ModemManagerData.bConfirmWriteLen == true) {
				Trace(" Data Write...(1)\r\n");
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WRITE_DATA;				// 실제로 전문을 write 하러 가자.
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_WRITE_DATA;
				// clear skip process value
				ClearNetworkDelayProcess();
				break;
			}
			break;

		// 실제로 전문 data를 전송한다.
		case eHTTP_COMM_STATE_WRITE_DATA:
			OemWriteUartModemBuff((unsigned char *)ModemManagerData.paModemCommDataTxBuffer, ModemManagerData.nWriteDataLength, NULL, eCOMM_TYPE_UART_MODEM);

			//Trace(" Write Data:\r\n");
    	    //hexdump(ModemManagerData.paModemCommDataTxBuffer,ModemManagerData.nWriteDataLength);

			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_URC_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_URC_CallBack, true);

			ModemManagerData.bWriteReady = false;

			ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_WAIT_WRITE_COMPLETE;
			ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			break;

		case eHTTP_COMM_STATE_WAIT_WRITE_COMPLETE:
#ifndef RF_COMMON_MODEM //mod.kks 21.11.02
			if(ModemManagerData.bWriteReady == true) {					// 전문 write 이후에, ^SISW: id,1 수신 여부 확인.
#endif
				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);

				ModemManagerData.bReadyReadData = false;

				Trace(" Done data write!!!\n\r\n");
				Trace(" *eHTTP_COMM_STATE_WRITE_FINISH\r\n");
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WRITE_FINISH;
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_WRITE_FINISH;
#ifndef RF_COMMON_MODEM //mod.kks 21.11.02
			}
#endif
			break;

        case eHTTP_COMM_STATE_SET_MULTI_WRITE_DATA_LEN:

            if( ModemManagerData.nSubWriteDataLength >= MAX_WRITE_DATA_AT_ONCE )
            {
                sprintf((char *)aTemp, "%d,%d\x00", ModemManagerData.nInternetServiceProfileId, MAX_WRITE_DATA_AT_ONCE);
            }
            else
            {
                sprintf((char *)aTemp, "%d,%d\x00", ModemManagerData.nInternetServiceProfileId, ModemManagerData.nSubWriteDataLength);
            }

			// write 할 data length를 전송한다. 정상적으로 전송할 데이터 크기가 반환이 되면, 그때 실제 data를 write 하게 된다.
			if(Md_SetInternetSocketWrite(aTemp) == true) {
				ModemManagerData.bConfirmWriteLen = false;

				Trace(" *eHTTP_COMM_STATE_CONFIRM_WRITE_LEN\r\n");
				// OK 응답을 기다리면 안된다.
				// ^SISW: ?,?,? 응답이 온 이후에 바로 data를 전송해야한다.
				// data 전송이 완료된 이후에 write 명령에 대한 OK 응답이 온다.
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CONFIRM_MULTI_WRITE_LEN;
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CONFIRM_MULTI_WRITE_LEN;
			}
            break;
        case eHTTP_COMM_STATE_CONFIRM_MULTI_WRITE_LEN:
            if(ModemManagerData.bConfirmWriteLen == true) {
				Trace(" Data Write...(1)\r\n");
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_MULTI_WRITE_DATA;				// 실제로 전문을 write 하러 가자.
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_MULTI_WRITE_DATA;
				// clear skip process value
				ClearNetworkDelayProcess();
				break;
			}
            break;

        case eHTTP_COMM_STATE_MULTI_WRITE_DATA:
            //OemWriteUart2Buff((unsigned char *)ModemManagerData.paModemCommDataTxBuffer, ModemManagerData.nWriteDataLength, NULL, NULL);

            if( ModemManagerData.nSubWriteDataLength >= MAX_WRITE_DATA_AT_ONCE )
            {
                OemWriteUartModemBuff((unsigned char*)&ModemManagerData.paModemCommDataTxBuffer[ModemManagerData.nSubWriteIndex*MAX_WRITE_DATA_AT_ONCE], MAX_WRITE_DATA_AT_ONCE, NULL, eCOMM_TYPE_UART_MODEM);
                //hexdump(&ModemManagerData.paModemCommDataTxBuffer[ModemManagerData.nSubWriteIndex*1500], 1500);
            }
            else
            {
                OemWriteUartModemBuff((unsigned char*)&ModemManagerData.paModemCommDataTxBuffer[ModemManagerData.nSubWriteIndex*MAX_WRITE_DATA_AT_ONCE], ModemManagerData.nSubWriteDataLength, NULL, eCOMM_TYPE_UART_MODEM);
                //hexdump(&ModemManagerData.paModemCommDataTxBuffer[ModemManagerData.nSubWriteIndex*1500], ModemManagerData.nSubWriteDataLength);
            }

			//Trace(" Write Data:\r\n");
    	    //hexdump(ModemManagerData.paModemCommDataTxBuffer,ModemManagerData.nWriteDataLength);

			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_URC_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_URC_CallBack, true);

			ModemManagerData.bWriteReady = false;

			ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_WAIT_MULTI_WRITE_COMPLETE;
			ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
            break;

        case eHTTP_COMM_STATE_WAIT_MULTI_WRITE_COMPLETE:
            if(ModemManagerData.bWriteReady == true) {					// 전문 write 이후에, ^SISW: id,1 수신 여부 확인.
				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);

				ModemManagerData.bReadyReadData = false;

                if( ModemManagerData.nSubWriteDataLength >= MAX_WRITE_DATA_AT_ONCE )
                    ModemManagerData.nSubWriteDataLength -= MAX_WRITE_DATA_AT_ONCE;
                else
                    ModemManagerData.nSubWriteDataLength = 0;

                ModemManagerData.nSubWriteIndex++;

				Trace(" Done data write!!!\n\r\n");
				Trace(" *eHTTP_COMM_STATE_WRITE_FINISH\r\n");
                if( ModemManagerData.nSubWriteDataLength > 0 )
                {
                    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_SET_MULTI_WRITE_DATA_LEN;
                    ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_SET_MULTI_WRITE_DATA_LEN;
                }
                else
                {
                    Trace(" Write Total Data Length : %d\r\n",ModemManagerData.nWriteDataLength);
				    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WRITE_FINISH;
                    ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_WRITE_FINISH;
                }
			}
            break;

		case eHTTP_COMM_STATE_WRITE_FINISH:

#ifndef RF_COMMON_MODEM
            sprintf((char *)aTemp, "%d,0,1\x00", ModemManagerData.nInternetServiceProfileId);
            if(Md_SetInternetSocketWrite(aTemp) == true) {
#endif

			// write를 종료 한다.
#ifndef RF_COMMON_MODEM //mod.kks 21.11.02
				ModemManagerData.bConfirmWriteLen = false;				// ^SISW: 0,0,0 																			--> 수신 되면 true
                ModemManagerData.bHTTPRcvPostURL = false;					// ^SIS: id,0,2200,"HTTP POST: https://~~~ URL ~~~"		--> 수신 되면 true
                ModemManagerData.bHTTPRcvPostResponse = false;		// ^SIS: id,0,2200,"HTTP POST Response: 200"					--> 수신 되면 true
                ModemManagerData.bFinishWrite = false;						// ^SISW: 0,2																					--> 수신 되면 true
                ModemManagerData.bReadyReadData = false;					// ^SISR: 0,1 																				--> 수신 되면 true, 수신 받은 data가 있다. read 하면 된다.
#else
				ModemManagerData.bConfirmWriteLen = true;				// ^SISW: 0,0,0 PLS63W not define.																			--> 수신 되면 true
                ModemManagerData.bHTTPRcvPostURL = true; 					// ^SIS: id,0,2200,"HTTP POST: PLS63W not define"		--> 수신 되면 true
                ModemManagerData.bHTTPRcvPostResponse = false; //true;		// ^SIS: id,0,2200,"HTTP POST Response: 200"					--> 수신 되면 true
#endif

				ModemManagerData.bEndOfData = false;

				// write finish 명령에 대한 OK 응답이 위 다섯개의 응답 메시지가 무작위로 수신되는 과정에 온다.
				// OK 응답 대기 delay(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly)는 ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly delay로 대체한다.
				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

//				HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_URC_TIMEOUT_20SEC, eSWTimer_ONESHOT, MDM_NoResp_URC_CallBack, true);

				Trace(" *eHTTP_COMM_STATE_WAITING_READY_TO_READ\r\n");
				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_WAITING_READY_TO_READ;
#ifdef RF_COMMON_MODEM //mod.kks 21.11.02
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING_READY_TO_READ;
#else
                ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
#endif
                // MONI 2018-1-18. Bug Fixed.
                // very rarely gemalto didn't response with "OK".
                // it makes state hold so we should use time about "OK".
                HalTimerStartSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, eSWTimer_ONESHOT);
#ifndef RF_COMMON_MODEM //mod.kks 21.11.02
			}
#endif
			break;

		case eHTTP_COMM_STATE_WAITING_READY_TO_READ:
//			ModemManagerData.bConfirmWriteLen = false;				// ^SISW: 0,0,0 																			--> 수신 되면 true
//			ModemManagerData.bFinishWrite = false;						// ^SISW: 0,2																					--> 수신 되면 true
//			ModemManagerData.bHTTPRcvPostURL = false;					// ^SIS: id,0,2200,"HTTP POST: https://~~~ URL ~~~"		--> 수신 되면 true
//			ModemManagerData.bHTTPRcvPostResponse = false;		// ^SIS: id,0,2200,"HTTP POST Response: 200"					--> 수신 되면 true
//			ModemManagerData.bReadyReadData = false;					// ^SISR: 0,1 																				--> 수신 되면 true, 수신 받은 data가 있다. read 하면 된다.
//			ModemManagerData.bReadyReadData = false;					// ^SISR: 0,2 																				--> 수신 되면 true, 수신 받은 data가 없다. 서버에서 파일을 전송하지 못한 경우 발생함.
			// 위 URC가 modem program version에 따라 일정한 순서 없이 들어 올 수 있다.

            // MONI 2018-02-15
            // we need to handle this process because it is not perfect
			if(ModemManagerData.bConfirmWriteLen == true &&
                ModemManagerData.bFinishWrite == true &&
                ModemManagerData.bHTTPRcvPostURL == true &&
                ModemManagerData.bReadyReadData == true &&
                ModemManagerData.bHTTPRcvPostResponse == true) {
				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

                if(ModemManagerData.nHTTPResponseCode == 200)
                {
                    // success write and ready to read
                    if(ModemManagerData.bEndOfData == true) {
    					Trace(" File not received\r\n");
    					Trace(" *eHTTP_COMM_STATE_CLOSE\r\n");
    					ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
						ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CLOSE;
	    			}
    				else {
    					Trace(" success ready to read\r\n");
    					ModemManagerData.paModemCommDataRxBuffer = (uint8_t *)gaModemCommRxDataBuffer;
    					ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;

    					ModemManagerData.bRespSISR = false;
    					ModemManagerData.bEndOfData = false;							// 파일의 끝인 경우 true 로 전환
    					ModemManagerData.bExpectEndOfData = false;

    					{
    						// 응답 받는 data 내에 file 종료를 의미하는 "^SISR: <id>,2" 문자열이 포함되어 전송되는 문제를 해결하기 위해서, 미리 비교 문구를 만들어 두었다.
    						uint16_t nXXXLen;

    						g_arrXXXCompareString[0] = 0x0d;
    						g_arrXXXCompareString[1] = 0x0a;
    						sprintf((char *)&g_arrXXXCompareString[2], "^SISR: %d,2\x00", ModemManagerData.nInternetServiceProfileId);
    						nXXXLen = strlen((char *)&g_arrXXXCompareString[2]);
    						nXXXLen	+= 2;
    						g_arrXXXCompareString[nXXXLen++] = 0x0d;
    						g_arrXXXCompareString[nXXXLen++] = 0x0a;

    						g_nXXXCompareStringLength = nXXXLen;

    //						hexdump(g_arrXXXCompareString, g_nXXXCompareStringLength);
    					}

    					Trace(" goto eHTTP_COMM_STATE_READ_DATA\r\n");
    					ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_READ_DATA;
                        ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_READ_DATA;

                        // clear skip process value
                        ClearNetworkDelayProcess();
    				}
                }
			}


            if(ModemManagerData.bConfirmWriteLen == true &&
                ModemManagerData.bFinishWrite == true &&
                ModemManagerData.bHTTPRcvPostURL == true &&
                ModemManagerData.bHTTPRcvPostResponse == true &&
                ModemManagerData.bReadyReadData == false )
            {
                // urcInfoId : description
                // 1 - 2000 : Error // service aborted
                // 2001-4000 : Information related to progress of service
                // 4001-6000 : Warning, but no service abort,
                // 6001-8000 : Notes

                if( ModemManagerData.nHTTPResponseCode == 15 ||
					ModemManagerData.nHTTPResponseCode == -1 ||
                    ModemManagerData.nHTTPURCInfoId == 200 ||
                    ModemManagerData.nHTTPURCInfoId == 404 ||
                    ModemManagerData.nHTTPURCInfoId == 8002 )
                {
                    Trace("HTTP Comm Fail Response Code : %x\r\n",ModemManagerData.nHTTPResponseCode);

                    // modem received but something is wrong what to do after that
                    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
                    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
                    ModemManagerData.bRetryCommFlag = false;
                    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
                    ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CLOSE;
                }
				else if( ModemManagerData.nHTTPResponseCode == 200 )
				{
					//Already received SISR 2,1 but not proccing yet. so need continue
				}
				else	//
				{
	                Trace("HTTP Comm Fail Response Code2 : %x\r\n",ModemManagerData.nHTTPResponseCode);

                    // modem received but something is wrong what to do after that
                    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
                    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
                    ModemManagerData.bRetryCommFlag = false;
                    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
                    ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CLOSE;
				}
            }

			break;

		case eHTTP_COMM_STATE_READ_DATA:

            // skip process
            if( NetworkDelayProcess(200) == false )
            {
                //skip until setting delay value
                break;
            }

			// 수신 받을 data 크기를 전송한다. 그리고, data 크기 만큼 수신한다.
			sprintf((char *)aTemp, "%d,%d\x00", ModemManagerData.nInternetServiceProfileId, ModemManagerData.nMaxReadDataLength);


			if(Md_InternetSocketRead(aTemp) == true) {
//#warning "bin data를 받지 못하는 상황을 위한 타임 아웃 설정 할 것"
//				HalTimerStartSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly, eSWTimer_ONESHOT);
//				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_WAIT_RCV_END_OF_DATA_FLAG;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_WAIT_RCV_END_OF_DATA_FLAG:
			if(ModemManagerData.bEndOfData == false) {

//#ifdef USE_REREAD_AGAIN_PROCESS
				if(ModemManagerData.bExpectEndOfData == true) {
					// 다시한번 URC 입력을 기다린다.
					// ^SISR: id,2 메시지가 다시한번 수신된다. 이걸 받아야 정말로 end of data로 인식하도록 하자.
					break;
				}
				else {
					if(ModemManagerData.nAvailableReadDataLength == 0) {
//						Trace(" goto *eHTTP_COMM_STATE_READ_DATA\r\n");

						ModemManagerData.bRespSISR = false;
						ModemManagerData.bEndOfData = false;							// 파일의 끝인 경우 true 로 전환
						ModemManagerData.bExpectEndOfData = false;

						ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_READ_DATA;
                        ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_READ_DATA;
					}
					else {
						if( g_bCompleteDataFlag == false )
						{
							ModemManagerData.bRespSISR = false;
							ModemManagerData.bEndOfData = false;							// 파일의 끝인 경우 true 로 전환
							ModemManagerData.bExpectEndOfData = false;
							
							ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_READ_DATA;
	                        ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_READ_DATA;
						}
						else
						{
						// AT^SISR 명령에 대한 OK 응답을 받았는데,
						// ModemManagerData.nAvailableReadDataLength 값이 0이 아니라면,
						// 수신 받은 데이터가 없다는 뜻이다.
						Trace(" Data rcv End!!!\r\n");

						HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);

						ModemManagerData.bRetryCommFlag = true;
						ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
                        ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CLOSE;
					}
					}
					break;
				}
//#endif //#ifdef USE_REREAD_AGAIN_PROCESS

			}
			else {
				HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);

				Trace(" Recv Done\r\n");
				Trace(" *eHTTP_COMM_STATE_CLOSE\r\n");
				g_bCompleteDataFlag = false;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
                ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CLOSE;
				break;
			}
			break;

        case eHTTP_COMM_STATE_AGPS_READY_TO_READ:
            if( ModemManagerData.bHTTPRcvPostURL == true &&
                ModemManagerData.bReadyReadData == true &&
                ModemManagerData.bHTTPRcvPostResponse == true)
            {
				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
				HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

                {
                    // success write and ready to read
                    if(ModemManagerData.bEndOfData == true) {
    					Trace(" File not received\r\n");
    					Trace(" *eHTTP_COMM_STATE_CLOSE\r\n");
    					ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
						ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CLOSE;
	    			}
    				else {
    					Trace(" success ready to read\r\n");
    					ModemManagerData.paModemCommDataRxBuffer = (uint8_t *)gaModemCommRxDataBuffer;
    					ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;

    					ModemManagerData.bRespSISR = false;
    					ModemManagerData.bEndOfData = false;							// 파일의 끝인 경우 true 로 전환
    					ModemManagerData.bExpectEndOfData = false;

    					{
    						// 응답 받는 data 내에 file 종료를 의미하는 "^SISR: <id>,2" 문자열이 포함되어 전송되는 문제를 해결하기 위해서, 미리 비교 문구를 만들어 두었다.
    						uint16_t nXXXLen;

    						g_arrXXXCompareString[0] = 0x0d;
    						g_arrXXXCompareString[1] = 0x0a;
    						sprintf((char *)&g_arrXXXCompareString[2], "^SISR: %d,2\x00", ModemManagerData.nInternetServiceProfileId);
    						nXXXLen = strlen((char *)&g_arrXXXCompareString[2]);
    						nXXXLen	+= 2;
    						g_arrXXXCompareString[nXXXLen++] = 0x0d;
    						g_arrXXXCompareString[nXXXLen++] = 0x0a;

    						g_nXXXCompareStringLength = nXXXLen;

    //						hexdump(g_arrXXXCompareString, g_nXXXCompareStringLength);
    					}

                        // prepare write agps data to file


    					Trace(" goto eHTTP_COMM_STATE_READ_DATA\r\n");
    					ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_READ_DATA;
                        ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_READ_DATA;

                        // clear skip process value
                        ClearNetworkDelayProcess();
    				}
                }
			}


            if( ModemManagerData.bHTTPRcvPostURL == true &&
                ModemManagerData.bHTTPRcvPostResponse == true &&
                ModemManagerData.bReadyReadData == false )
            {
                // urcInfoId : description
                // 1 - 2000 : Error // service aborted
                // 2001-4000 : Information related to progress of service
                // 4001-6000 : Warning, but no service abort,
                // 6001-8000 : Notes

                if( ModemManagerData.nHTTPResponseCode == 15 ||
                    ModemManagerData.nHTTPURCInfoId == 200 ||
                    ModemManagerData.nHTTPURCInfoId == 210 ||		//210308 LWH 	^SIS: 2,0,210,"INT:error in sendRequest -313 SSL-Error: revcd alert fatal error"
                    ModemManagerData.nHTTPURCInfoId == 8002 )
                {
                    Trace("HTTP Comm Fail Response Code_2 : %x\r\n",ModemManagerData.nHTTPResponseCode);

                    // modem received but something is wrong what to do after that
                    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
                    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
                    ModemManagerData.bRetryCommFlag = false;
                    ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_CLOSE;
                    ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_CLOSE;
                }
            }
            break;

		case eHTTP_COMM_STATE_CLOSE:
			sprintf((char *)aTemp, "%d\x00", ModemManagerData.nInternetServiceProfileId);
			g_bCompleteDataFlag = false;

			if(Md_SetInternetSocketClose(aTemp) == true) {
				BkSram_ModemInfo.bHTTPSuccessServiceConnection[ModemManagerData.nInternetServiceProfileId] = true;

				ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_IDLE;
				ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_WAITING;
			}
			break;

		case eHTTP_COMM_STATE_IDLE:
			ModemManagerData.bWriteReady = false;
			ModemManagerData.bConfirmWriteLen = false;
			ModemManagerData.bFinishWrite = false;
			ModemManagerData.bReadyReadData = false;
			ModemManagerData.bEndOfData = false;
			ModemManagerData.bServiceOpen = false;
			ModemManagerData.bExpectEndOfData = false;
			ModemManagerData.bRcvBinDataZeroFlag = false;

			HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);
			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);

			ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_INIT;
            ModemManagerData.nMdmSubNextState = eHTTP_COMM_STATE_INIT;

			Trace(" close http communication\n\r\n");
            return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_FOTAGetVer(void)
{
	ModemManagerData.ePreviousMessageSendingType = ModemManagerData.eCurrentMessageSendingType;

	if(gwModemCommRxDataLength > 0) {
//		Trace("RCV MSG: encrypted\r\n");
//		hexdump(ModemManagerData.paModemCommDataRxBuffer, gwModemCommRxDataLength);
#if defined (OLD_FOTA)
	  	uint16_t nLen;
		nLen = ApplyDecryption(ModemManagerData.paModemCommDataRxBuffer, gwModemCommRxDataLength);
		nLen = nLen;
		Trace("RCV MSG: plain text\r\n");
        if( nLen < MAX_MODEM_COMM_RX_BUFFER_LENGTH )
		    hexdump(ModemManagerData.paModemCommDataRxBuffer, nLen);

		if(memcmp(ModemManagerData.paModemCommDataRxBuffer, "{\"RESULT\":\"F\"", strlen("{\"RESULT\":\"F\"")) == 0) {
			Trace("FOTA: result fail\r\n");
			g_FotaManagerData.eFotaResultCode = eFOTA_RCV_DATA_ERROR_RCV_INFO_FAIL;
		}
		else if(memcmp(ModemManagerData.paModemCommDataRxBuffer, "{ \"USERID\" : ", strlen("{ \"USERID\" : ")) == 0) {
			g_FotaManagerData.eFotaResultCode = FOTA_ParseReceivedVersionInfo();
		}
		else {
			g_FotaManagerData.eFotaResultCode = eFOTA_RCV_DATA_ERROR_RCV_INFO_FAIL;
		}
#else
		g_FotaManagerData.eFotaResultCode = NEWFOTA_ParseReceivedVersionInfo();
#endif
	}
	else {
		g_FotaManagerData.eFotaResultCode = eFOTA_RCV_DATA_ERROR_RCV_INFO_FAIL;
	}

	if(g_FotaManagerData.eFotaResultCode == eFOTA_RESULT_CODE_SUCCESS) {
		// file 정보를 정상적으로 수신 받았다.
		// 이제 실제 파일 본체를 수신 받자.
		Trace("FOTA: Success get version\r\n");

		SetModemControlEvent(eMngMdmRequestSendResult_Success);

        //MONI 2018-1-19
        // to test getting info from server, we just stop next step
        // ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_BIN;
        // send the message to system message to go next step.
        ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;

        //MONI Send2MngSysMsg2(eMngModem,eRspFota,eFwInfo,eSuccess, (stCarReport *)NULL);
	}
	else {
		HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);
    SetModemState(eMODEM_READY);

		SetModemControlEvent(eMngMdmRequestSendResult_Fail);

		Trace("FOTA: Fail get file info(%d)\r\n", g_FotaManagerData.eFotaResultCode);
		ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;

		m_bPostponeReqSleep = false;

		MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;
	}

	SetModemState(eMODEM_READY);

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_FOTAGetBin(void)
{
	ModemManagerData.ePreviousMessageSendingType = ModemManagerData.eCurrentMessageSendingType;

	Trace("gwTotalReceiveBinDataLength: %d\r\n", gwTotalReceiveBinDataLength);
	if(g_stUpdateFileInfoList.m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileSize == gwTotalReceiveBinDataLength) {
		g_FotaManagerData.eState = eFOTA_STATE_CONFIRM_CHECKSUM;
		SetModemState(eMODEM_RUNNING_FOTA);
	}
	else {
		Trace("FOTA: Mismatch File Size(org: %d - rcv: %d)\r\n", g_stUpdateFileInfoList.m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileSize, gwTotalReceiveBinDataLength);
		if(g_FotaManagerData.nCurrDownloadFileNo < g_FotaManagerData.nMaxDownloadFileNo) {
			g_FotaManagerData.eState = eFOTA_STATE_CHECK_CONTINUE;
			SetModemState(eMODEM_RUNNING_FOTA);
		}
		else {
			g_FotaManagerData.eState = eFOTA_STATE_INIT;

			//HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);
            //SetModemState(eMODEM_READY);

			MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

			SetModemState(eMODEM_READY);
		}
	}

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_AGPSGetData(void)
{
	ModemManagerData.ePreviousMessageSendingType = ModemManagerData.eCurrentMessageSendingType;

    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);
    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
    HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);

    ModemManagerData.bRetryCommFlag = false;
    ModemManagerData.nRetryCommCount = 0;

    // send success
    SetModemControlEvent(eMngMdmRequestSendResult_Success);
    // we must add a process about an error of server
    //
    if(ModemManagerData.eCurrentMessageSendingType == eMESSAGE_APGS_DATA )
    {
        Trace("AGPS Download Data\r\n");
        //hexdump(gaModemCommRxDataBuffer,gwModemCommRxDataLength);
    }

#if defined(USE_UBLOX_GPS)
    SetReqWaitNetworkCheck(false);
    SetModemState(eMODEM_READY);
#endif

    // clear network service
    BkSram_ModemInfo.bHTTPSuccessServiceConnection[ModemManagerData.nInternetServiceProfileId] = false;

#if false
    char buffer[1024];
    int nSize;
    int nReadSize;
    ReadAgpsDataSize(AUTOLINK_AGPS_DATA,&nSize);

    Trace("AGPS Read Data from File\r\n");

    for(int i=0;i<nSize/1024;i++)
    {
        ReadAgpsData(AUTOLINK_AGPS_DATA,buffer,1024,i*1024);
        hexdump(buffer,1024);
    }

    ReadAgpsData(AUTOLINK_AGPS_DATA,buffer,nSize%1024,(nSize/1024)*1024);
    hexdump(buffer,nSize%1024);
#endif

    // send an event that agps download down to system
    if(ModemManagerData.iTimer_Rcv_AGPS_Data_TimeoutDly == -1) 
    {
    	Send2MngSysMsg(eMngModem,eRspAgps,eAgpsDownloadFail,(stCarReport *)NULL,0);
    }
    else
	{
    Send2MngSysMsg(eMngModem,eRspAgps,eAgpsDownloadDone,(stCarReport *)NULL,0);
		HalTimerClearSWTimer(ModemManagerData.iTimer_Rcv_AGPS_Data_TimeoutDly);
		ModemManagerData.iTimer_Rcv_AGPS_Data_TimeoutDly = -1;
    }

	return eMODEM_PROCESS_FUNC_RET_OK;
}


eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_GetVehicleInfo(void)
{
	ModemManagerData.ePreviousMessageSendingType = ModemManagerData.eCurrentMessageSendingType;

	if(gwModemCommRxDataLength > 0) {
//#warning "차량정보는 아직 암호화 적용 전이다."
#if				0
		Trace("RCV MSG: encrypted\r\n");
		hexdump(ModemManagerData.paModemCommDataRxBuffer, gwModemCommRxDataLength);
		nLen = ApplyDecryption(ModemManagerData.paModemCommDataRxBuffer, gwModemCommRxDataLength);
#else
		printf("[%s]\r\n", ModemManagerData.paModemCommDataRxBuffer);
//		Trace("RCV MSG: plain text\r\n");
//		hexdump(ModemManagerData.paModemCommDataRxBuffer, gwModemCommRxDataLength);
#endif

#if defined (OLD_FOTA)
		if(memcmp(ModemManagerData.paModemCommDataRxBuffer, "{\"RESULT\":\"F\"", strlen("{\"RESULT\":\"F\"")) == 0) {
			Trace("FOTA: result fail\r\n");
			g_FotaManagerData.eFotaResultCode = eFOTA_RCV_DATA_ERROR_RCV_INFO_FAIL;
		}
		else if(memcmp(ModemManagerData.paModemCommDataRxBuffer, "{\"RESULT\":\"S\"", strlen("{\"RESULT\":\"S\"")) == 0) {
			g_FotaManagerData.eFotaResultCode = FOTA_ParseReceivedVehicleInfo();
		}
		else {
			g_FotaManagerData.eFotaResultCode = eFOTA_RCV_DATA_ERROR_RCV_INFO_FAIL;
		}
#else
		g_FotaManagerData.eFotaResultCode = NEWFOTA_ParseReceivedVehicleInfo();
#endif
	}
	else {
		g_FotaManagerData.eFotaResultCode = eFOTA_RCV_DATA_ERROR_RCV_INFO_FAIL;
	}

	if(g_FotaManagerData.eFotaResultCode == eFOTA_RESULT_CODE_SUCCESS) {
		// file 정보를 정상적으로 수신 받았다.
		// 이제 실제 파일 본체를 수신 받자.
		Trace("FOTA: Success get Vehicle Info\r\n");

		SetModemControlEvent(eMngMdmRequestSendResult_Success);

        //MONI 2018-1-19
        // to test getting info from server, we just stop next step
        // ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_BIN;
        // send the message to system message to go next step.
        ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;

        //MONI Send2MngSysMsg2(eMngModem,eRspFota,eFwInfo,eSuccess, (stCarReport *)NULL);
	}
	else {
		HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);
        SetModemState(eMODEM_READY);

		SetModemControlEvent(eMngMdmRequestSendResult_Fail);

		Trace("FOTA: Fail get file info(%d)\r\n", g_FotaManagerData.eFotaResultCode);
		ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;

		m_bPostponeReqSleep = false;

		MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;
	}

	SetModemState(eMODEM_READY);

	return eMODEM_PROCESS_FUNC_RET_OK;
}

extern unsigned long m_ulOldStartSendReportTime;

int m_nFailResponseCodeLst[] =
{
    1001,//incorrect password
    1012,//not allowed user
    1114,//first login, need to change password
    1115,//duplicated id
    1016,//fill out mendatory column
    1117,//not activated id
    1118,//not registered user
    1119,//invalid vin
    1024,//no information about vin
    1025,//error : parameter
    1026,//incorrect remote control
    1027,//no match between module and vin
    1028,//incorrect sms code
    1029,//incorrect remote password
    1030,//not allow the remote control during driving
    1031,//no authorization for control
    1032,//duplicated request remote control
    1033,//fail to save in server
    1034,//fail to sms
    1036,//there is no information
    1037,//over setting count
    311,//fail to save
    400,//bad request
    401,//unauthorized
    404,//not found
    500,//internal server error
    550,//partial error
};

char* m_nFailResponseTextForCodeLst[] =
{
    "incorrect password",
    "not allowed user",
    "first login, need to change password",
    "duplicated id",
    "fill out mendatory column",
    "not activated id",
    "not registered user",
    "invalid vin",
    "no information about vin",
    "error : parameter",
    "incorrect remote control ",
    "no match between module and vin",
    "incorrect sms code",
    "incorrect remote password",
    "not allow the remote control during driving",
    "no authorization for control",
    "duplicated request remote control",
    "fail to save in server",
    "fail to sms",
    "there is no information",
    "over setting count",
    "fail to save",
    "bad request",
    "unauthorized",
    "not found",
    "internal server error",
    "partial error",
};

enum{
    eServerErrorCodeHnaldeCode_None = 0,
    eServerErrorCodeHnaldeCode
};

int m_nFailResponseHandleCode[] =
{
    0,//incorrect password
    0,//not allowed user
    0,//first login, need to change password
    0,//duplicated id
    0,//fill out mendatory column
    1,//not activated id
    1,//not registered user
    5,//invalid vin
    5,//no information about vin
    5,//error : parameter
    5,//incorrect remote control
    6,//no match between module and vin
    6,//incorrect sms code
    6,//incorrect remote password
    7,//not allow the remote control during driving
    7,//no authorization for control
    7,//duplicated request remote control
    2,//fail to save in server
    2,//fail to sms
    3,//there is no information
    3,//over setting count
    3,//fail to save
    4,//bad request
    4,//unauthorized
    4,//not found
    8,//internal server error
    8,//partial error
};

/* Sample Data
{
 "ack": "error",
 "timestamp": "20180306081736",
 "error": {
  "errorcode": 500,
  "errormessage": "Internal server error",
  "details": [
   {
    "seqno": "16",
    "errortype": "sql",
    "errornumber": 2627,
    "errormessage": "Duplicate key"
   },
   {
    "seqno": "17",
    "errortype": "sql",
    "errornumber": 2627,
    "errormessage": "Duplicate key"
   }
  ]
 },
 "trace": {
  "runningtime": "00:00:00.1404018",
  "message": "api log url: devautolinkapi-premium.hmca.com.au:443/Toss/Detaillog/77068, error log url: devautolinkapi-premium.hmca.com.au:443/Toss/Detailerror/144412"
 }
}
*/

boolean_t GetNextToken(char* pcSrc, char* pcFindStr, char** pcNextIndex, char* pcValue)
{
    char* ptr;
    char* ptr2;
    char* ptr3;

    ptr = strstr((const char*)pcSrc,(const char*)pcFindStr);
    if( ptr != NULL )
    {
        ptr2 = (char *)strchr((const char *)ptr, ':');
        ptr3 = (char *)strchr((const char *)ptr2, ',');

        *pcNextIndex = ptr3;

        ptr2++;

        if( ptr3 - ptr2 > 0 )
            memcpy((char *)pcValue, (char *)ptr2, ptr3 - ptr2);

        return true;
    }
	*pcNextIndex = NULL;

    return false;
}

// this enum is defined by git server
// so if have a question, ask to git server
enum {
	eHttpServerInternalServerError = 500,
    eHttpServerInternalError = 600,
	eHttpServerInternalIPEKError = 1104,
};
enum {
    eHttpServerInternalErrorNumber_DataError = 547,
    eHttpServerInternalErrorNumber_Duplicate = 2627,
    eHttpServerInternalErrorNumber_VinError = 50000,
};
//220622 mod.pdh to do set server time to RTC
unsigned int m_unServerUtcTime;
boolean_t m_bServerUtcTime = false; 

void SetServerUtcTime(stHalRTCTypeDef utc)
{
    memset((char*)&m_unServerUtcTime,0,sizeof(m_unServerUtcTime));
    
    uint32_t unTime;
	
    unTime = GetTimefromDate2(utc);

    m_unServerUtcTime = unTime;
    m_bServerUtcTime = true;
}

bool GetServerUtcTime(uint32_t* punUtcTime)
{
    if( m_bServerUtcTime == true )
    {        
        *punUtcTime = m_unServerUtcTime;
        
        m_bServerUtcTime = false;
        return true;
    }

    *punUtcTime = 0;    
    return false;
}


stHalRTCTypeDef ConvertStringToRtcDate(unsigned char *ucString)
{
    stHalRTCTypeDef stDate;
    unsigned char ucYear[5] = {0,};
    unsigned char ucMonth[3] = {0,};
    unsigned char ucDate[3] = {0,};
    unsigned char ucHour[3] = {0,};
    unsigned char ucMin[3] = {0,};
    unsigned char ucSec[3] = {0,};

    memset(&stDate,0x00,sizeof(stDate));

    memcpy(ucYear,&ucString[0],4);
    memcpy(ucMonth,&ucString[4],2);
    memcpy(ucDate,&ucString[6],2);
    memcpy(ucHour,&ucString[8],2);
    memcpy(ucMin,&ucString[10],2);
    memcpy(ucSec,&ucString[12],2);

    ucYear[4] = 0x00;
    ucMonth[2] = 0x00;
    ucDate[2] = 0x00;
    ucHour[2] = 0x00;
    ucMin[2] = 0x00;
    ucSec[2] = 0x00;

    stDate.RtcDate.RTC_Year = atoi((char const*)ucYear)-2000;
    stDate.RtcDate.RTC_Month = atoi((char const*)ucMonth);
    stDate.RtcDate.RTC_Date = atoi((char const*)ucDate);
    stDate.RtcTime.RTC_Hours = atoi((char const*)ucHour);
    stDate.RtcTime.RTC_Minutes = atoi((char const*)ucMin);
    stDate.RtcTime.RTC_Seconds = atoi((char const*)ucSec);

    stDate.RtcDate.RTC_WeekDay = HAL_RTC_Weekday_Friday;
    stDate.RtcTime.RTC_H12 = HAL_RTC_HourFormat_24;

    return stDate;
}


void PorcessHTTPCommSubHandler()	//HTTP 결과가 성공인지 실패인지 에러인지 확인해서 넘겨줌
{
	//char *ptrOriginal;
	char *ptr = NULL;
	char *ptr1 = NULL;
	char *ptr2 = NULL;
	char *ptr3 = NULL;
	char carrKsn[64]={0,};
	char carrBuff[256]={0,};
	char carrDecrypt[1024+512] = {0,};
	uint16_t unLen = 0;
	DukptPinEntry stEncryptKeyEntry;
	uint32_t unKsnlength = 0;
	boolean_t bRet = false;
	//hexdump(ModemManagerData.paModemCommDataRxBuffer, gwModemCommRxDataLength);
	//hexdump(ModemManagerData.paModemCommDataRxBuffer, gwModemCommRxDataLength);

	MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_FAIL_SERVER_MESSAGE;
	MessageManagerData.nErrorCodeFromMessageServer = 0;

	GetNextToken((char*)ModemManagerData.paModemCommDataRxBuffer,"ack",&ptr,carrBuff);
	
    //220622 mod.pdh to do set server time to RTC
    char carrTimeStamp[14]={0,};
	uint32_t unTimeStamplength = 0;
	stHalRTCTypeDef stDate;

    if( strstr((const char *)carrBuff,(const char *)"success") )
    {
#ifndef QA_FIFA
	if(g_FirmwareInfo.arrSerialNumber[IDX_MANUFACTURER_CODE]=='H')
	{
		ptr = (char *)strstr((const char *)ptr, (const char *)"\"timestamp\":\"");
		if(ptr != NULL)
		{
		    ptr = ptr + strlen("\"timestamp\":\"");
		    ptr2 = (char *)strstr((const char *)ptr, (const char *)"\",");

		    unTimeStamplength = ptr2 - ptr;
		    memcpy((char*)carrTimeStamp,(char*)ptr,unTimeStamplength); 

			stDate = ConvertStringToRtcDate((unsigned char*)&carrTimeStamp);
			SetServerUtcTime(stDate);

		}	
	}

	if(g_FirmwareInfo.arrSerialNumber[IDX_MANUFACTURER_CODE]=='K')
	{
		ptr = (char *)strstr((const char *)ptr, (const char *)"\"utctimestamp\":\"");
		if(ptr != NULL)
		{
		    ptr = ptr + strlen("\"utctimestamp\":\"");
		    ptr2 = (char *)strstr((const char *)ptr, (const char *)"\",");

		    unTimeStamplength = ptr2 - ptr;
		    memcpy((char*)carrTimeStamp,(char*)ptr,unTimeStamplength); 

			stDate = ConvertStringToRtcDate((unsigned char*)&carrTimeStamp);
			SetServerUtcTime(stDate);

		}	
	}
#else
	ptr = (char *)strstr((const char *)ptr, (const char *)"\"utctimestamp\":\"");
	if(ptr != NULL)
	{
	    ptr = ptr + strlen("\"utctimestamp\":\"");
	    ptr2 = (char *)strstr((const char *)ptr, (const char *)"\",");

	    unTimeStamplength = ptr2 - ptr;
	    memcpy((char*)carrTimeStamp,(char*)ptr,unTimeStamplength); 

		stDate = ConvertStringToRtcDate((char*)&carrTimeStamp);
		SetServerUtcTime(stDate);

	}	
#endif

		switch(ModemManagerData.eCurrentMessageSendingType)
		{
			case eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST:
			case eMESSAGE_TYPE_SETTING_GEOFENCE:
			case eMESSAGE_TYPE_SETTING_POLYGON_GEOFENCE:
			case eMESSAGE_TYPE_ENGINE_START:
			case eMESSAGE_TYPE_REQUEST_MODEM_ACTIVATE:
#if defined(PROTOCOL15)
			case eMESSAGE_TYPE_REQUEST_SENSOR_INITIALIZE:
#endif
#if defined(PROTOCOL17)
			case eMESSAGE_TYPE_REQUEST_SETURL:
#endif
#if defined(PROTOCOL18)
			case eMESSAGE_TYPE_REQUEST_RESERVATION_ENGINE_CONTROL_SETTING:
#endif
			case eMESSAGE_TYPE_TRACKING_REQUEST:
			{
				DCSEncryptType nEncryptType;
				// get service type fleet / retail
				GetAutolinkConfigProperty(eAutoLinkConfig_EncryptType,(void*)&nEncryptType);

				if(MessageManagerData.bReceivedRequestRemoteMessageFlag == eMESSAGE_REMOTE_CONTROL_STATE_NONE)
				{
					// 현재 원격 제어가 진행 되지 않는 경우에만 수신 받는다.
					// 원격 제어 메시지가 존재 한다.
					if( nEncryptType == DCS_ENC_KMS )
//#ifdef USE_KMS_ENCRYPT
					{
						ptr = (char *)strstr((const char *)ptr, (const char *)"\"ksn\":\"");
						if(ptr != NULL)
						{
							ptr = ptr + strlen("\"ksn\":\"");
							ptr2 = (char *)strstr((const char *)ptr, (const char *)"\",");

							unKsnlength = ptr2 - ptr;
							memcpy((char*)carrKsn,(char*)ptr,unKsnlength);
							unKsnlength = ApplyDecryption((unsigned char*)carrKsn, unKsnlength);
							//hexdump(carrKsn,unKsnlength);
						}

						ptr = (char *)strstr((const char *)ptr, (const char *)"\"data\":\"");
						if(ptr != NULL)
						{
							ptr = ptr + strlen("\"data\":\"");
							ptr2 = (char *)strstr((const char *)ptr, (const char *)"\"}");
							gn_RemoteMessageBufferLength = ptr2 - ptr;

                            if( (gn_RemoteMessageBufferLength > 16) && (gn_RemoteMessageBufferLength < MAX_CONTROL_REQUEST_FROM_SERVER_BUFFER_LENGTH))
							{
								memcpy((char *)gb_RemoteMessageBuffer, (char *)ptr, gn_RemoteMessageBufferLength);

								//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);
								GetLastUsedDukptPinEntry(&stEncryptKeyEntry);

                                bRet=ApplyDecryption3((unsigned char*)carrDecrypt,&unLen,gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength,&stEncryptKeyEntry);
								if( bRet == false )
								{
								    MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;
								    printf(" ApplyDecryption3 Fail\r\n");
								    MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_FAIL_SERVER_MESSAGE;
																	
								    return;
								}
								memset(gb_RemoteMessageBuffer,0,sizeof(gb_RemoteMessageBuffer));
								memcpy(gb_RemoteMessageBuffer,carrDecrypt,unLen);
								gn_RemoteMessageBufferLength = unLen;

								//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);
								MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_OCCURE;

                                Trace(" eMESSAGE_REMOTE_CONTROL_STATE_OCCURE\r\n");
								MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
							}
							else
							{
								MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

                                Trace(" eMESSAGE_REMOTE_CONTROL_STATE_NONE\r\n");
								MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_FAIL_SERVER_MESSAGE;
							}
						}
						else
						{
                            Trace("MSG: mismatch_1 \"data\"\r\n");
							break;
						}
					}
					else if( nEncryptType == DCS_ENC_AES )
//#else //#ifdef USE_KMS_ENCRYPT
					{
						ptr = (char *)strstr((const char *)ptr, (const char *)"\"data\":\"");
						if(ptr != NULL)
						{
							ptr = ptr + strlen("\"data\":\"");
							ptr2 = (char *)strstr((const char *)ptr, (const char *)"\"}");
							gn_RemoteMessageBufferLength = ptr2 - ptr;

							if( gn_RemoteMessageBufferLength > 16 )
							{
								memcpy((char *)gb_RemoteMessageBuffer, (char *)ptr, gn_RemoteMessageBufferLength);

								//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

								gn_RemoteMessageBufferLength = ApplyDecryption(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

								//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);
								MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_OCCURE;

        						Trace(" eMESSAGE_REMOTE_CONTROL_STATE_OCCURE\r\n");
								MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
							}
							else
							{
								MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

                                Trace(" eMESSAGE_REMOTE_CONTROL_STATE_NONE\r\n");
								MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_FAIL_SERVER_MESSAGE;
							}
						}
						else
						{
    						Trace("MSG: mismatch_2 \"data\"\r\n");
							break;
						}
					}
//#endif //#ifdef USE_KMS_ENCRYPT
				}
				else {
      	            Trace(" I am busy!!!\r\n");
				}
			}
				break;
			case eMESSAGE_TYPE_IPEK_PHASE1:
			case eMESSAGE_TYPE_IPEK_PHASE2:
				ptr = (char *)strstr((const char *)ptr, (const char *)"\"data\":\"");
				if(ptr != NULL) {
					ptr = ptr + strlen("\"data\":\"");
					ptr2 = (char *)strstr((const char *)ptr, (const char *)"\"}");
					gn_RemoteMessageBufferLength = ptr2 - ptr;

					if( gn_RemoteMessageBufferLength > 16 )
					{
						memcpy((char *)gb_RemoteMessageBuffer, (char *)ptr, gn_RemoteMessageBufferLength);

						//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

						gn_RemoteMessageBufferLength = ApplyDecryption(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

						//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);
						MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_OCCURE;

                        Trace(" eMESSAGE_REMOTE_CONTROL_STATE_OCCURE\r\n");
						MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
					}
					else
					{
						MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

                        Trace(" eMESSAGE_REMOTE_CONTROL_STATE_NONE\r\n");
						MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_FAIL_SERVER_MESSAGE;
					}
				}
				else {
					Trace("MSG: mismatch_3 \"data\"\r\n");
					break;
				}
				break;
#if defined(PROTOCOL17)
			case eMESSAGE_TYPE_RESPONSE_SETURL:
				MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_OCCURE;
				Trace(" eMESSAGE_REMOTE_CONTROL_STATE_OCCURE\r\n");
				MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
				break;
			case eMESSAGE_TYPE_ALARM_EVENT:
				g_bAnyEventSentFlag = true;
				MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
				break;
#endif
			case eMESSAGE_TYPE_PERIOD_INFORMATION_SLEEP:
			case eMESSAGE_TYPE_REMOTE_CONTROL_REPORT:
			case eMESSAGE_TYPE_BT_REMOTE_CONTROL_REPORT:
#if defined(PROTOCOL24)
            case eMESSAGE_TYPE_MODEM_STATUS_REPORT:
#endif
			default:
				MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
				break;
		}
	}
	else if( strstr((const char *)carrBuff,(const char *)"fail") )
	{
		memset(carrBuff,0,sizeof(carrBuff));

		// if result is fail. it is duplicate with remote control. so we need to dump
		if( GetNextToken(ptr,"errorcode",&ptr1,carrBuff) == true )
		{
			// this is error code
			MessageManagerData.nErrorCodeFromMessageServer = atoi((char *)carrBuff);
			MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;

			Trace("MSG: MessageManagerData.nErrorCodeFromMessageServer: %d\r\n", MessageManagerData.nErrorCodeFromMessageServer);
		}
		else
		{
			MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
		}
	}
	else if( strstr((const char *)carrBuff,(const char *)"error") )
	{
		memset(carrBuff,0,sizeof(carrBuff));
		if( GetNextToken(ptr,"errorcode",&ptr1,carrBuff) == true )
		{
			int nErrorNum1;
			int nErrorNum2;
			int nMessageCount;

			// this is error code
			MessageManagerData.nErrorCodeFromMessageServer = atoi((char *)carrBuff);

			// find error number #1
			memset(carrBuff,0,sizeof(carrBuff));
			if( GetNextToken(ptr1,"errornumber",&ptr2,carrBuff) == true )
			{
				nErrorNum1 = atoi((char *)carrBuff);
			}
			else
			{
				nErrorNum1 = 0;
			}

			if( ptr2 != NULL )
			{
				// find error number #2
				memset(carrBuff,0,sizeof(carrBuff));
				if( GetNextToken(ptr2,"errornumber",&ptr3,carrBuff) == true )
				{
					nErrorNum2 = atoi((char *)carrBuff);
				}
				else
				{
					nErrorNum2 = 0;
				}
			}
			else
			{
				nErrorNum2 = 0;
			}

			nMessageCount = GetLastSendMessageCount();
            Trace("Error code : %d, number #1 : %d, #2 : %d,,body count : %d\r\n",MessageManagerData.nErrorCodeFromMessageServer,nErrorNum1,nErrorNum2,nMessageCount);

//#warning "we need to exception handle of modem according to the server and situation."
			// server internal error : 600
			if( MessageManagerData.nErrorCodeFromMessageServer == eHttpServerInternalError )
			{
				// data is duplicated
				if( nErrorNum1 == eHttpServerInternalErrorNumber_Duplicate )
				{
					if( nErrorNum2 == eHttpServerInternalErrorNumber_Duplicate )
					{
						MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
                        Trace("Delete data#1\r\n");
					}
					else
					{
						if( nMessageCount == 1 )
						{
							MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
                            Trace("Delete data#2\r\n");
						}
						else
						{
							MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_FAIL_SERVER_MESSAGE;
                            Trace("Resend data#3\r\n");

							// i think i might not use this code.
							// because one of body message is cracked. it might not be happened
							if( nErrorNum2 == eHttpServerInternalErrorNumber_DataError )
							{
                                Trace("###########################################\r\n");
                                Trace("ERROR : check this code...\r\n");
								// one of message is not correct we dump
								MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
							}
						}
					}
				}
				// data is not correct
				else if( nErrorNum1 == eHttpServerInternalErrorNumber_DataError )
				{
					MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
                    Trace("Delete data#4\r\n");
				}
				else if( nErrorNum1 == eHttpServerInternalErrorNumber_VinError )
				{
					MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
                    Trace("Delete data#5\r\n");
				}
				else
				{
					// one of body message is not sent so we need to send again.
					MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
                    Trace("Resend data#6\r\n");
				}
			}
			else if( MessageManagerData.nErrorCodeFromMessageServer == eHttpServerInternalIPEKError )
			{
				printf("~~~~~~~~~~~~~~~~~~~~~~~ReInitFutureKey\r\n");
				if(DAMO_DUKPT_Import_Future_Key_Info(&m_stAutolinkConfigData.stFutureKey)<0)
				{
					Trace("====================================================\r\n");
					Trace("error : import of penta security library.\r\n");
					//set up error for encryption
					// we should add recovery machanism

					// try recovery future key of penta from ipek that stored in the internal flash
					LoadInitializeDamoKey(&m_stAutolinkConfigData.stFutureKey);
					if(DAMO_DUKPT_Import_Future_Key_Info(&m_stAutolinkConfigData.stFutureKey)<0)
					{
						// we should run a process to get ipek from server.
						Trace("====================================================\r\n");
						printf("IMPORTANT : error : import of penta security library.\r\n");
						Trace("we should run a process to get an ipek from server\r\n");
						SetRequestIPEK();
					}
				}
				MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_DAMO_ERROR;
			}
			else if( MessageManagerData.nErrorCodeFromMessageServer == eHttpServerInternalServerError )
			{
				Trace("Delete data#8\r\n");
				MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
			}
			else
			{
				// other side error we need to retry
				MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_FAIL_SERVER_MESSAGE;
                Trace("Resend data#7\r\n");
			}
		}
	}
	else	// Rcv < ^SIS: 2,0,2200,"HTTP POST Response: 200 ,but there is no data ex)kia <!doctype html>
	{
		MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
        printf("Drop data. because server is not support\r\n");
	}
}

void CheckHTTPResponseResult()
{
    if( ModemManagerData.nHTTPResponseCode == 15 ||
        ModemManagerData.nHTTPURCInfoId == 200 )
    {
        // modem received but something is wrong what to do after that
        MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SERVER_ABORTED;

    }
    else if( ModemManagerData.nHTTPURCInfoId == 8002 )
    {
        if( ModemManagerData.nHTTPResponseCode == 24 )
        {
            //this is general socket error
            // case 1 // host name error we should reinialze the mode
            // case 2 // server is not response
            MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SERVER_NOT_FOUND;
        }
        else if( ModemManagerData.nHTTPResponseCode == 500 )
        {
//#warning "in this case we accept that it is success, because this code is related with data parsing error."
            //MONI 2018-02-23
            // in this case we accept that it is success, because this code is related with data parsing error.
            // So we can't recovery this message, we need to find out a solution about that.
            //MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SERVER_NOT_STABLE;
            MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
        }
		 else if( ModemManagerData.nHTTPResponseCode == 404 )
        {
            MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SUCCESS;
        }
        else if( ModemManagerData.nHTTPResponseCode == 0xFFF0)
        {
            // server ssl error
            // retry after few minutes
            MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SERVER_NOT_STABLE;
        }
        else if( ModemManagerData.nHTTPResponseCode == 0xFFF1)
        {
            // use wrong apn
            // check apn setting
            MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_NO_RESP;
        }
        else if( ModemManagerData.nHTTPResponseCode == 0xFFF2)
        {
            // if we use wrong address over 2times it was occurred
            MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SERVER_NOT_STABLE;
        }
		else if( ModemManagerData.nHTTPResponseCode == -1 )
		{
			MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SERVER_NOT_STABLE;
		}
        else //if( ModemManagerData.nHTTPResponseCode == 0xFFFF)
        {
            // unkown error
            // what to do???
            MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SERVER_NOT_STABLE;
        }
    }
	else if( ModemManagerData.nHTTPURCInfoId == 2200 )
	{
		if( ModemManagerData.nHTTPResponseCode == -1 )
		{
			MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_SERVER_NOT_STABLE;
		}
	}
    //printf("CheckHTTPResponseResult:%d,%d,%d \r\n",ModemManagerData.nHTTPResponseCode,ModemManagerData.nHTTPURCInfoId,MessageManagerData.eMessageCommResultCode);
}

eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_Message(void)
{
	//Trace("@ProcessHTTPCommResult_Message()\r\n");
	unsigned long ulDiffTmr   = OemGetTmrDelta(OemGetTmr(), m_ulOldStartSendReportTime);
    Trace("|               Transfer Time (Transfer Time:%d)             \r\n",ulDiffTmr);

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);

	if(gwModemCommRxDataLength > 0) {
		// this is for remote control message handler
		PorcessHTTPCommSubHandler();	//HTTP 결과가 성공인지 실패인지 에러인지 확인해서 넘겨줌
	}

	// check response of network
	CheckHTTPResponseResult();

    Trace("==============================================================\r\n");
    Trace("--------------------------------------------------------------\r\n");
	Trace("HTTP Result Handler\r\n");

//	Trace("MSG: result code: %d\n", MessageManagerData.eMessageCommResultCode);
	switch(MessageManagerData.eMessageCommResultCode) {
		case eMESSAGE_RESULT_CODE_SUCCESS:
            Trace("--------------------------------------------------------------\r\n");
			Trace("eMESSAGE_RESULT_CODE_SUCCESS\r\n");
			// sucess send data
			ModemManagerData.bRetryCommFlag = false;
			ModemManagerData.nRetryCommCount = 0;

			// send success
			SetModemControlEvent(eMngMdmRequestSendResult_Success);
			// we must add a process about an error of server
			//
			ModemManagerData.ePreviousMessageSendingType = ModemManagerData.eCurrentMessageSendingType;
			break;
		case eMESSAGE_RESULT_CODE_NO_RESP:
            Trace("--------------------------------------------------------------\r\n");
            Trace("eMESSAGE_RESULT_CODE_NO_RESP\r\n");
			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

			// need to modem reset
			SetModemControlEvent(eMngMdmCriticalError);
			ModemManagerData.ePreviousMessageSendingType = eMESSAGE_TYPE_NONE;
			break;
		case eMESSAGE_RESULT_CODE_SERVER_ABORTED:
            Trace("--------------------------------------------------------------\r\n");
            Trace("eMESSAGE_RESULT_CODE_SERVER_ABORTED\r\n");
			SetModemControlEvent(eMngMdmRequestSendResult_Fail_NotFoundServer);

			ModemManagerData.ePreviousMessageSendingType = eMESSAGE_TYPE_NONE;
			break;
		case eMESSAGE_RESULT_CODE_SERVER_NOT_STABLE:
            Trace("--------------------------------------------------------------\r\n");
            Trace("eMESSAGE_RESULT_CODE_SERVER_NOT_STABLE\r\n");
			SetModemControlEvent(eMngMdmRequestSendResult_Fail_NotStableServer);

			ModemManagerData.ePreviousMessageSendingType = eMESSAGE_TYPE_NONE;
			break;
		case eMESSAGE_RESULT_CODE_SERVER_NOT_FOUND:
            Trace("--------------------------------------------------------------\r\n");
            Trace("eMESSAGE_RESULT_CODE_SERVER_NOT_FOUND\r\n");
			SetModemControlEvent(eMngMdmRequestSendResult_Fail_SockError);

			ModemManagerData.ePreviousMessageSendingType = eMESSAGE_TYPE_NONE;
			break;
		case eMESSAGE_RESULT_CODE_ERROR_HTTP_COMM_FAIL:
            Trace("--------------------------------------------------------------\r\n");
            Trace("eMESSAGE_RESULT_CODE_ERROR_HTTP_COMM_FAIL\r\n");
			SetModemControlEvent(eMngMdmRequestSendResult_Fail_NotStableServer);

			ModemManagerData.ePreviousMessageSendingType = eMESSAGE_TYPE_NONE;
			break;
		case eMESSAGE_RESULT_CODE_DAMO_ERROR:
			Trace("--------------------------------------------------------------\n");
			Trace("eMESSAGE_RESULT_CODE_DAMO_ERROR\n");
			SetRequestIPEK();
			ModemManagerData.bRetryCommFlag = false;
			ModemManagerData.nRetryCommCount = 0;
			SetModemControlEvent(eMngMdmRequestSendResult_Success);

			ModemManagerData.ePreviousMessageSendingType = eMESSAGE_TYPE_NONE;
			break;
		default:
            Trace("--------------------------------------------------------------\r\n");
			Trace("default %d\n",MessageManagerData.eMessageCommResultCode);
			SetModemControlEvent(eMngMdmRequestSendResult_Fail_General);
			if(ModemManagerData.bRetryCommFlag == true) {
				ModemManagerData.bRetryCommFlag = false;
				// retry 를 하지 않도록 함.
				ModemManagerData.nRetryCommCount = 0;

				ModemManagerData.bRequestMessageCommFlag = false;

			ModemManagerData.ePreviousMessageSendingType = eMESSAGE_TYPE_NONE;
#if				0
//		  ModemManagerData.nRetryCommCount++;
//        Trace("MSG: ModemManagerData.nRetryCommCount = %d\r\n", ModemManagerData.nRetryCommCount);
//		  if(ModemManagerData.nRetryCommCount >= MAX_MODEM_RETRY_COUNT) {
//			ModemManagerData.nRetryCommCount = 0;
//
//          Trace(" fail message comm - 10\r\n");
//			ModemManagerData.bRequestMessageCommFlag = false;
//			ModemManagerData.eMessageResponseFromServer = eFAIL_MESSAGE_RESPONSE;
//
//			// 서버로 전송 실패한 경우, 모뎀 reset이 필요없을 지도 모르겠다.
//			// 그냥 전송하지 못한 전문을 저장한 후에 다음 메시지를 전송하면 어떨까...
//////					ModemManagerData.bNeedModemHWResetFlag = true;
//			break;
//		  }
//		  else {
//          Trace("MSG: retry sending message comm\r\n");
//			ModemManagerData.bRequestMessageCommFlag = true;
//			ModemManagerData.nMdmSubState = eHTTP_COMM_STATE_INIT;
//			break;
//		  }
#endif
			}
		break;
	}

	//HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);	SetModemState(eMODEM_READY);
	SetReqWaitNetworkCheck(false);

	SetModemState(eMODEM_READY);

	return eMODEM_PROCESS_FUNC_RET_OK;
}

eNETWORK_REG_STATUS isNetworkRegStatus(void)
{
	return ModemManagerData.eNetworkRegStatus;
}


uint8_t gbModemRingPinMode;
uint8_t gbModemRingPinState;

eMODEM_PROCESS_FUNC_RET SelftestProcessConfigModemGPIO(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			if(gbModemRingPinMode == MODEM_RING_PIN_MODE_GPIO) {
				if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"gpio\"") == true) {
#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
					ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
#endif
					ModemManagerData.bModemRcvSysStartMessageFlag = false;
					ModemManagerData.bModemRcvPBReadyMessageFlag = false;

					ModemManagerData.nMdmSubNextState = 1;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
			}
			else {
				if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"std\"") == true) {
#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
					ModemManagerData.bModemRcvSysLoadingMessageFlag = false;
#endif
					ModemManagerData.bModemRcvSysStartMessageFlag = false;
					ModemManagerData.bModemRcvPBReadyMessageFlag = false;

					ModemManagerData.nMdmSubNextState = 1;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
			}
			break;

		case 1:
			// modem reset
			printf("MDM: Wait...\r\n");
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 2000, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 2;
			break;

		case 2:				// wait 2000ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 3;
			break;

		case 3:
			// modem reset
			printf("MDM: reset(only modem)\r\n");
			printf("PowerOnGemaltoModem_Modem_Reset_Power9\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
			APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
#else
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
#endif
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 4;
			break;

		case 4:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 5;
			break;

		case 5:
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 100, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 6;
			break;

		case 6:				// wait 5ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 7;
			break;

		case 7:
			printf("PowerOnGemaltoModem_Modem_Reset_Rst9\r\n");
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			ModemManagerData.nMdmSubState = 8;
			break;

		case 8:				// ^SYSLOADING 응답을 기다린다.
#ifndef RF_COMMON_MODEM // mod.kks 21.10.25
			if(ModemManagerData.bModemRcvSysLoadingMessageFlag == false) {
				break;
			}
#endif

			ModemManagerData.nMdmSubState = 9;
			break;

		case 9:				// ^SYSSTART 응답을 기다린다.
			if(ModemManagerData.bModemRcvSysStartMessageFlag == false) {
				break;
			}

			ModemManagerData.nMdmSubState = 10;
			//ModemManagerData.nMdmSubState = 12;			//SIM 카드 없는 경우도 있어서  ^PBREADY 응답pass
			break;

		case 10:				// ^PBREADY 응답을 기다린다. SIM 카드 없으면 기다리지 말아야 한다. SIM 카드 없으면 PBREADY는 응답 하지 않는다.
			if(ModemManagerData.bModemRcvPBReadyMessageFlag == false) {
				break;
			}

			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 1000, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 11;
			break;

		case 11:				// wait 1000ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = 12;
			break;

		case 12:
			//	1 : echo enable   0 : echo disable
			if(Md_SetEcho("0") == true) {
				ModemManagerData.nMdmSubNextState = 13;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
#ifdef RF_COMMON_MODEM //mod.pdh 2021.12.13
		case 13:
			if(gbModemRingPinMode == MODEM_RING_PIN_MODE_GPIO) {
				if(Md_SetGPIODriver("1") == true) {
					ModemManagerData.nMdmSubNextState = 14;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
			}
            else
			{
				ModemManagerData.nMdmSubNextState = 14;
                ModemManagerData.nMdmSubState = 14;
			}
            break;
		case 14:
			if(gbModemRingPinMode == MODEM_RING_PIN_MODE_GPIO) {
				if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"gpio\"") == true) {
					ModemManagerData.nMdmSubNextState = 15;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
			}
			else
			{
				if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"std\"") == true) {
					ModemManagerData.nMdmSubNextState = 15;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
			}
			break;
		case 15:
			if(gbModemRingPinMode == MODEM_RING_PIN_MODE_GPIO) {
				if(Md_OpenCloseGPIO("1,23,1,1") == true) {
					ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
			}
			else {
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			}
			break;
#else //#ifdef RF_COMMON_MODEM //mod.pdh 2021.12.13
		case 13:
			if(gbModemRingPinMode == MODEM_RING_PIN_MODE_GPIO) {
				//  GPIO open and output is SET  -> MODEM에서 SET 을 해줘야 MO_MODEM 신호가 RESET이 된다.(반전)
				if(Md_OpenCloseGPIO("1,23,1,1") == true) {
					ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
			}
			else {
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			}
			break;
#endif //#ifdef RF_COMMON_MODEM //mod.pdh 2021.12.13
		case MDM_SUB_STATE_WAIT_RESPONSE:
			switch(g_stModem_Rep.eMDResponse) {
				case eMDResponseStateOK:
					ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
					g_stModem_Rep.eMDResponse = eMDResponseStateWait;
					printf("1 MDM: rcv ok\r\n");
					break;

				case eMDResponseStateFAIL:
					printf("MDM: eMDResponseStateFAIL\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateNoCarrier:
					printf("MDM: eMDResponseStateNoCarrier\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateTimeOut:
					printf("MDM: eMDResponseStateTimeOut\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateCMEError:
					printf("MDM: eMDResponseStateCMEError\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				default:
					break;
			}
			break;
            
        case MDM_SUB_STATE_MODEM_FAIL:
            ModemManagerData.nMdmSubState = 0;
            return eMODEM_PROCESS_FUNC_RET_FAIL;                  

		case MDM_SUB_STATE_END_PROCESS:
			printf("MDM: eMODEM_PROCESS_FUNC_RET_OK\r\n");
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET SelftestProcessSetResetModemGPIO(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			if(gbModemRingPinState == MODEM_RING_PIN_STATE_HIGH) {
				if(Md_SetResetModemGPIO("23,1") == true) {
					ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
			}
			else {
				if(Md_SetResetModemGPIO("23,0") == true) {
					ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
				}
			}
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			switch(g_stModem_Rep.eMDResponse) {
				case eMDResponseStateOK:
					ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
					g_stModem_Rep.eMDResponse = eMDResponseStateWait;
					printf("2 MDM: rcv ok\r\n");
					break;

				case eMDResponseStateFAIL:
					printf("MDM: eMDResponseStateFAIL\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateNoCarrier:
					printf("MDM: eMDResponseStateNoCarrier\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateTimeOut:
					printf("MDM: eMDResponseStateTimeOut\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateCMEError:
					printf("MDM: eMDResponseStateCMEError\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				default:
					break;
			}
			break;
            
        case MDM_SUB_STATE_MODEM_FAIL:
            ModemManagerData.nMdmSubState = 0;
            return eMODEM_PROCESS_FUNC_RET_FAIL;                   

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET SelftestProcessSetModemPower(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
#ifdef RF_COMMON_MODEM //mod.kks 21.12.22 change modem mode(Factory mode) to call the hidden function in PLS63 Qualcomm CPU.
			if(Md_SetFactorymode() == true) {
				ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
#else
				ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = 1;

#endif
			break;
		case 1:
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 10000, eSWTimer_ONESHOT, MDM_NoResp_CMD_CallBack, false);
#ifdef RF_COMMON_MODEM //mod.kks 21.12.22 change modem mode(Factory mode) to call the hidden function in PLS63 Qualcomm CPU.
            if(Md_SetModemConfigExt("\"MEopMode/CT\",\"1\",\"\",\"32\",\"9750\"") == true) {
#else
			if(Md_SetModemConfigExt("\"MEopMode/CT\",\"1\",\"\",\"127\",\"9750\"") == true) {
#endif
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 2:
			if(Md_SetOperatorSelection("2") == true) {				// 네트워크 접속 시도
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 3:
			if(Md_SetModemConfigExt("\"MEopMode/CT\",\"0\"") == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			switch(g_stModem_Rep.eMDResponse) {
				case eMDResponseStateOK:
					ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
					g_stModem_Rep.eMDResponse = eMDResponseStateWait;
					printf("MDM: rcv ok\r\n");
					break;

				case eMDResponseStateFAIL:
					printf("MDM: eMDResponseStateFAIL\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateNoCarrier:
					printf("MDM: eMDResponseStateNoCarrier\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateTimeOut:
					printf("MDM: eMDResponseStateTimeOut\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateCMEError:
					printf("MDM: eMDResponseStateCMEError\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				default:
					break;
			}
			break;

		case MDM_SUB_STATE_MODEM_FAIL:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_FAIL;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;

		default:
			break;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}
eMODEM_PROCESS_FUNC_RET SelftestProcessSetFlightMode(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			if(Md_SetFlightMode("4,0") == true) {								// Flighe mode on
				ModemManagerData.bModemRcvSysStartMessageFlag = false;

				ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 1:
			// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^SYSSTART message를 응답 받기까지의 delay, 초기에 ^SYSSTART message를 받지 못할 경우도 있다.
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT_2, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_AtWorkaround_CallBack, true);
			ModemManagerData.nMdmSubState = 2;
			break;

		case 2:				// ^SYSSTART 응답을 기다린다.
#ifndef RF_COMMON_MODEM //mod.kks 21.12.22
			if(ModemManagerData.bModemRcvSysStartMessageFlag == false) {
				break;
			}
#endif
			ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			break;

		case 3:
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT_2, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_AtWorkaround_CallBack, true);
			if(Md_SetFlightMode("1,1") == true) {								// Flighe mode off
				ModemManagerData.bModemRcvSysStartMessageFlag = false;

				ModemManagerData.nMdmSubNextState = 4;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 4:
			// modem command를 전달하고 ok 응답받기까지 delay, 또는 ^START, ^PBREADY message를 응답 받기까지의 delay
			//HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT_2, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_AtWorkaround_CallBack, true);
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_AtWorkaround_CallBack, true);
			ModemManagerData.bModemRcvSysStartMessageFlag = false;

			ModemManagerData.nMdmSubState = 5;
			break;

		case 5:				// ^SYSSTART 응답을 기다린다.
			if(ModemManagerData.bModemRcvSysStartMessageFlag == false) {
				break;
			}
			//ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			ModemManagerData.nMdmSubState = 6;
			break;
		case 6:				// ^PBREADY 응답을 기다린다. SIM 카드 없으면 기다리지 말아야 한다. SIM 카드 없으면 PBREADY는 응답 하지 않는다.
			if(ModemManagerData.bModemRcvPBReadyMessageFlag == false) {
				break;
			}

			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 3000, eSWTimer_ONESHOT, NULL, true);
			ModemManagerData.nMdmSubState = 7;
			break;

		case 7:				// wait 1000ms
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly) != 0) {
				break;
			}

			ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
			break;

		case 11:
			ResetForceModem();
			ModemManagerData.nMdmSubState = 5;

			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			switch(g_stModem_Rep.eMDResponse) {
				case eMDResponseStateOK:
					ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
					g_stModem_Rep.eMDResponse = eMDResponseStateWait;
					printf("MDM: rcv ok\r\n");
					break;

				case eMDResponseStateFAIL:
					printf("MDM: eMDResponseStateFAIL\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateNoCarrier:
					printf("MDM: eMDResponseStateNoCarrier\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateTimeOut:
					printf("MDM: eMDResponseStateTimeOut\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateCMEError:
					printf("MDM: eMDResponseStateCMEError\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				default:
					break;
			}
			break;

		case MDM_SUB_STATE_MODEM_FAIL:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_FAIL;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET SelftestProcessGetModemInfo(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			if(Md_ReadSystemInfo() == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
		case MDM_SUB_STATE_WAIT_RESPONSE:
			switch(g_stModem_Rep.eMDResponse) {
				case eMDResponseStateOK:
					ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
					g_stModem_Rep.eMDResponse = eMDResponseStateWait;
					printf("MDM: rcv ok\r\n");
					break;

				case eMDResponseStateFAIL:
					printf("MDM: eMDResponseStateFAIL\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateNoCarrier:
					printf("MDM: eMDResponseStateNoCarrier\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateTimeOut:
					printf("MDM: eMDResponseStateTimeOut\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateCMEError:
					printf("MDM: eMDResponseStateCMEError\r\n");
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				default:
					break;
			}
			break;
        case MDM_SUB_STATE_MODEM_FAIL:
            ModemManagerData.nMdmSubState = 0;
            return eMODEM_PROCESS_FUNC_RET_FAIL;
            
		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
            
		default:
			break;
	}
	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}


eMODEM_PROCESS_FUNC_RET SelftestProcessSetModemBaudrate(void)
{
	static uint16_t s_nMdmSubPrevState = 0;
	switch(ModemManagerData.nMdmSubState) {
		case 0:
				if(Md_Set_921600_Baudrate() == true) {
					s_nMdmSubPrevState = 0;
					ModemManagerData.nMdmSubNextState = 2;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;

					HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, 2000, eSWTimer_ONESHOT, NULL, true);
				}
				break;
		case 1:
				if(Md_Set_115200_Baudrate() == true) {
					s_nMdmSubPrevState = 1;
					ModemManagerData.nMdmSubNextState = 2;
					ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;

					HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, 2000, eSWTimer_ONESHOT, NULL, true);
				}
				break;
		case 2:
			if(HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly) != 0) {
				break;
			}
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSSTART_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysStartMessage_CallBack, false);
#else
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly, MDM_MSG_SYSLOADING_NORESPONSE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoResp_SysLoadingMessage_CallBack, false);
#endif
			if(s_nMdmSubPrevState == 0)	ModemManagerData.nMdmSubState = 3;
			else						ModemManagerData.nMdmSubState = 4;

			break;

		case 3:
			Trace(" Set baud 921600\r\n");

            HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);
            HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

			// MCU의 UART2 baudrate를 변경한다.
			Trace(" Init UART2(921600dbps)\r\n");
			ModemUart_Init(921600);

			geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_SUCCESS;

			ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
			ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;

			// save new baudrate
			BkSram_ModemInfo.unBaurdRate = 921600;

			break;

		case 4:
			Trace(" Set baud 115200\r\n");

            HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);
            HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

			// MCU의 UART2 baudrate를 변경한다.
			Trace(" Init UART2(115200dbps)\\rn");
			ModemUart_Init(115200);

			geChangeDefaultModemBaudFlag = eMODEM_BAUDRATE_SEARCHING_STATE_SUCCESS;

			ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
			ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;

			// save new baudrate
			BkSram_ModemInfo.unBaurdRate = 115200;

			break;
		case 5:

			break;
		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_MODEM_RESET:
			Trace("Immediately reset\r\n");
			printf("PowerOnGemaltoModem_Modem_Reset_Power10\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
			APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
#else
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			APP_Delay(20);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			APP_Delay(20);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
#endif
			ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
			break;
            
        case MDM_SUB_STATE_MODEM_FAIL:
            ModemManagerData.nMdmSubState = 0;
            return eMODEM_PROCESS_FUNC_RET_FAIL;               

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET SelftestProcessInitializeModem(void)
{
	switch(ModemManagerData.nMdmSubState) {
		case 0:
			//	1 : echo enable   0 : echo disable
			if(Md_SetEcho("0") == true) {
				ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 1:
			if(Md_ResultOnOff("0") == true) {
				//ModemManagerData.nMdmSubNextState = 2;
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
        case 2:
           if(Md_GetIMSI() == true) {
               ModemManagerData.nMdmSubNextState = 5;
               ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
           }
           break;
       case 3:
           if(Md_SetPowerSaveMode("1, 0, 0") == true) {                // modem의 ASC0/1을 항상 active 상태로 설정한다.
               ModemManagerData.nMdmSubNextState = 4;
               ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
           }
           break;
       case 4:
           if(Md_SetFlowControl() == true) {
               ModemManagerData.nMdmSubNextState = 5;
               ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
           }
           break;

       case 5:
           if(Md_ReadSystemInfo() == true) {
               ModemManagerData.nMdmSubNextState = 6;
               ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
           }
           break;
       case 6:
           if(Md_GetIMEI() == true) {
               ModemManagerData.nMdmSubNextState = 7;
               ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
           }
           break;
       case 7:
           if(Md_GetDateTime() == true) {
               ModemManagerData.nMdmSubNextState = 9;
               ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
           }
           break;

       case 8:
           if(Md_Set_ErrorMsgFormat("2") == true) {
               ModemManagerData.nMdmSubNextState = 9;
               ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
           }
           break;

       case 9:
           if(Md_GetCCIDInfo() == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
//               ModemManagerData.nMdmSubNextState = 12;
//               ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
           }
           break;

//       case 10:
//           if(Md_SetSMSFormat("1") == true) {
//               ModemManagerData.nMdmSubNextState = 11;
//               ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
//           }
//           break;
//        case 11:
//			// at+cnmi=2,1,0,0,0
//			if(Md_SetSMSReportConfig("2,1,0,0,0") == true) {
//				ModemManagerData.nMdmSubNextState = 12;
//				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
//			}
//			break;

		case 12:
			// at+scfg="Gpio/mode/RING0","std"
			if(Md_SetModemConfigExt("\"Gpio/mode/RING0\",\"gpio\"") == true) {
//				ModemManagerData.nMdmSubNextState = 3;
				ModemManagerData.nMdmSubNextState = 13;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 13:
			// at+scfg="URC/Ringline","asc0"
			if(Md_SetModemConfigExt("\"URC/Ringline\",\"off\"") == true) {
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;
            
        case MDM_SUB_STATE_MODEM_FAIL:
            ModemManagerData.nMdmSubState = 0;
            return eMODEM_PROCESS_FUNC_RET_FAIL;

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}
eMODEM_PROCESS_FUNC_RET SelftestProcessRegistUSIM(void)
{
	static uint16_t s_nMdmSubPrevState = 0;
	uint8_t aTemp[30];
	memset(aTemp,0x00,sizeof(aTemp));

	switch(ModemManagerData.nMdmSubState) {
		case 0:
			if(Md_GetCCIDInfo() == true) {
				s_nMdmSubPrevState = 0;
				ModemManagerData.nMdmSubNextState = 1;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 1:
			if(Md_SetRegistPhoneNumMode("\"on\"") == true) {
                printf("Request Phone Num Register Mode\r\n");
				//ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				s_nMdmSubPrevState = 1;
				ModemManagerData.nMdmSubNextState = 5;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
			break;
		case 2:
			sprintf((char *)aTemp, "=,\"%s\",%d",g_ucUSIMServerPhoneNo,g_uiUSIMCountryCode);
			if(Md_SetRegistPhoneNumber(aTemp) == true) {
                printf("RegistPhone Number\r\n");
				s_nMdmSubPrevState = 2;
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
			break;

		case 3:
			if(Md_GetPhoneNo() == true) {
				s_nMdmSubPrevState = 3;
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;

		case 4:
			if(Md_SetRegistPhoneNumber("?") == true) {
                printf("Check Registed PhoneNumber\r\n");
				s_nMdmSubPrevState = 4;
				//ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
			break;
		case 5:
			sprintf((char *)aTemp, "=%c",g_ucRemodePhoneNumCnt);
			if(Md_SetRegistPhoneNumber(aTemp) == true) {
				printf("Remove Phone Numbder :%c \r\n",g_ucRemodePhoneNumCnt);
				g_ucRemodePhoneNumCnt++;
				if(g_ucRemodePhoneNumCnt == '4'){
					ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
					g_ucRemodePhoneNumCnt = '1';
				}
				else{
					ModemManagerData.nMdmSubNextState = 5;
				}
				s_nMdmSubPrevState = 5;
				//ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case MDM_SUB_STATE_WAIT_RESPONSE:
			break;

		case MDM_SUB_STATE_MODEM_RESET:
			Trace("Immediately reset\r\n");
			printf("PowerOnGemaltoModem_Modem_Reset_Power11\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
			APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
		    APP_Delay(200);
		    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		    APP_Delay(200);
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#else
			HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			APP_Delay(20);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			APP_Delay(20);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
#endif
			ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
			break;
		case MDM_SUB_STATE_MODEM_FAIL:

			if(s_nMdmSubPrevState == 5){
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
				g_ucRemodePhoneNumCnt = '1';

				g_stModem_Rep.eMDResponse = eMDResponseStateWait;
				return eMODEM_PROCESS_FUNC_RET_OK;
			}
			else{
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
				return eMODEM_PROCESS_FUNC_RET_FAIL;
			}

		case MDM_SUB_STATE_END_PROCESS:
			ModemManagerData.nMdmSubState = 0;
			return eMODEM_PROCESS_FUNC_RET_OK;

	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}
void SelftestProcessMDResponse()
{
    switch(g_stModem_Rep.eMDResponse)
    {
    	case eMDResponseStateOK:
    		ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
    		g_stModem_Rep.eMDResponse = eMDResponseStateWait;
    		break;

    	case eMDResponseStateFAIL:
            Trace("====Selftest RES================================================\r\n");
    		Trace(" command fail, cmd index: %d\r\n", g_stModem_Rep.cmd_index);

			// 180723 SPARROW : 생산테스트 시 Fail 응답 들어오는 경우 있어 추가

			Trace(" Receive \"ERROR\"\r\n");
			ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
			break;

    	case eMDResponseStateTimeOut:
            Trace("====Selftest RES================================================\r\n");
    		Trace(" Time out, cmd index: %d\r\n", g_stModem_Rep.cmd_index);
			if((g_arrYUJINTestIdx[0] == eSELFTEST_CURRENT_IDX)&&(g_arrYUJINTestIdx[1] == MODEM_POWER_NORMAL)){
				ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
    			g_stModem_Rep.eMDResponse = eMDResponseStateWait;
			}
			else{
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
			}
			break;

    	case eMDResponseStateCMEError:
            Trace("====Selftest RES================================================\r\n");
			Trace(" CME Error, cmd index: %d\r\n", g_stModem_Rep.cmd_index);

#ifdef RF_COMMON_MODEM //mod.pdh 22.01.24
                if(g_stModem_Rep.cmd_index == eCmd_SetGPIODriver)
                {
                        if(Md_SetGPIODriver("0") == true) {
                                ;
                        }

                }
#endif
    		if(ModemManagerData.eCMEErrorCode == eCME_ERROR_NO_OPERATION_NOT_ALLOWED){
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
				break;
    		}
    		else if(ModemManagerData.eCMEErrorCode == eCME_ERROR_NO_OPERATION_TEMPORARY_NOT_ALLOWED){
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
				break;
    		}
    		else{
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
				break;
    		}
    		break;

    	case eMDResponseState_ModemRegDenied:				// ë§ ì ‘ì† ê±°ë¶€ messageê°€ ìˆ˜ì‹  ë˜ë©´, systemì„ reset í•œë‹¤.
															// ìƒì‚°í”„ë¡œê·¸ëž¨ Usim ë“¤ì€ ë‹¤ ë§ê±°ë¶€ ì¼ì–´ë‚¨
			Trace("====Selftest RES================================================\r\n");
			Trace(" Reg Denied, cmd index: %d\r\n", g_stModem_Rep.cmd_index);

    		break;

    	default:
    		break;
    }
}

#ifdef RF_COMMON_MODEM
#define SEND_SIZE   (1024)
eMODEM_PROCESS_FUNC_RET ProcessSendAGPSData(void)
{
    long long llSize=0;
    char strTempBuff[10]={0,};
    char strSize[20]={0,};
	char strbuffer[SEND_SIZE+1]={0,};
	static int s_nTotCount=0,s_nPos=0;
	uint8_t strTimeString[36];
	stHalRTCTypeDef stDate;
	static unsigned char s_ucRetryCnt=0;

	switch(ModemManagerData.nMdmSubState) {
		case 0:
			HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stDate, sizeof(stHalRTCTypeDef), 0);
			memset(strTimeString,0x00,sizeof(strTimeString));
			sprintf((char *)strTimeString, "\"%02d/%02d/%02d,%02d:%02d:%02d\"\x00", 
                            stDate.RtcDate.RTC_Year,
							stDate.RtcDate.RTC_Month,
							stDate.RtcDate.RTC_Date,
							stDate.RtcTime.RTC_Hours,
							stDate.RtcTime.RTC_Minutes,
							stDate.RtcTime.RTC_Seconds);
			if(	stDate.RtcDate.RTC_Year != 17 && stDate.RtcDate.RTC_Year>21)
			{
    			if(Md_SetDateTime(strTimeString) == true) 
                {
    				ModemManagerData.nMdmSubNextState = 1;
    				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
    			}
			}
            else
            {
                ModemManagerData.nMdmSubNextState = 10;
                ModemManagerData.nMdmSubState = 10;
            }
			break;
		case 1:
			if(Md_SetGNSS("\"Engine\",\"0\"") == true) {    //mod.kks todo GPS setting "0:OFF / 1:ON"
		   		ModemManagerData.nMdmSubNextState = 2;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 2:
			if(Md_SetGNSS("\"NMEA/Output\",\"OFF\"") == true) {         //"OFF / ON / Last"
				ModemManagerData.nMdmSubNextState = 3;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
			break;
		case 3:
			if(Md_SetAGPSFileDelete() == true) { 		//at^sbnw=agps,-1
				ModemManagerData.nMdmSubNextState = 4;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 4:
            llSize = g_uiAGPSFilesize;
			itoa_1(llSize,strTempBuff,10);

			sprintf(strSize,"%s,%s","agps",strTempBuff);
			if(Md_SetAGPSFileStartSend((uint8_t*)strSize) == true) { 		//at^sbnw=agps,size
				ModemManagerData.nMdmSubNextState = 5;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 5:
			ReadAgpsDataSize(AUTOLINK_AGPS_DATA,(int*)&g_uiReadAGPSFilesize);
			s_nTotCount = g_uiReadAGPSFilesize/SEND_SIZE;
			s_nPos = 0;
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 60000, eSWTimer_ONESHOT, NULL, true);
            printf("AGPS Read Data from File : %d,%d\r\n",g_uiReadAGPSFilesize,s_nTotCount);
			ModemManagerData.nMdmSubNextState = 6;
            ModemManagerData.nMdmSubState = 6;
			break;
		case 6:
            if( g_stModem_Rep.eMDResponse == eMDResponseStateWait )
            {
                memset(strbuffer,0x00,sizeof(strbuffer));
                if( s_nPos < s_nTotCount )
                {
                    //printf("s_nPos:%d,%d\r\n",s_nPos,s_nPos*SEND_SIZE);
                    printf(">");
                    ReadAgpsData(AUTOLINK_AGPS_DATA,strbuffer,SEND_SIZE,s_nPos*SEND_SIZE);
                    OemWriteUartModemBuff((unsigned char*)strbuffer, SEND_SIZE, NULL, eCOMM_TYPE_UART_MODEM);
                    //hexdump(strbuffer,SEND_SIZE);
                    s_nPos++;
                }
                else
                {
                    if( g_uiReadAGPSFilesize%SEND_SIZE != 0 )
                    {
                        //printf("rest:%d,%d\r\n",g_uiReadAGPSFilesize%SEND_SIZE,s_nTotCount*SEND_SIZE);
                        printf("%d Send ok\r\n",g_uiReadAGPSFilesize);
                        ReadAgpsData(AUTOLINK_AGPS_DATA,strbuffer,g_uiReadAGPSFilesize%SEND_SIZE,s_nTotCount*SEND_SIZE);
                        OemWriteUartModemBuff((unsigned char*)strbuffer, g_uiReadAGPSFilesize%SEND_SIZE, NULL, eCOMM_TYPE_UART_MODEM);
                        ModemManagerData.nMdmSubNextState = 7;
                        ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
						HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, 2000, eSWTimer_ONESHOT, MDM_NoResp_CMD_CallBack, true);
                        //hexdump(strbuffer,g_uiReadAGPSFilesize%SEND_SIZE);
                    }
                    else
                    {
                        ModemManagerData.nMdmSubState = MDM_SUB_STATE_END_PROCESS;
                    }
                }
            }
            else
            {
                ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
            }
			break;
		case 7:
			//printf("A-GPS Interface\r\n");
			if(Md_SetGNSS("\"NMEA/Interface\",\"6\"") == true) {
				ModemManagerData.nMdmSubNextState = 8;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 8:
			//printf("A-GPS Output\r\n");
			if(Md_SetGNSS("\"NMEA/Output\",\"ON\"") == true) {	  //mod.kks todo GPS setting "0:OFF / 1:ON"
				if( s_ucRetryCnt == 0 )	ModemManagerData.nMdmSubNextState = 9;
				else					ModemManagerData.nMdmSubNextState = 10;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 9:
			printf("A-GPS Engine On\r\n");
			if(Md_SetGNSS("\"Engine\",\"2\"") == true) {	//mod.kks todo GPS setting "0:OFF / 1:ON"
				ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case 10:
			printf("GPS ON Not A-GPS\r\n");
			if(Md_SetGNSS("\"Engine\",\"1\"") == true) {    //mod.kks todo GPS setting "0:OFF / 1:ON"
		   		ModemManagerData.nMdmSubNextState = MDM_SUB_STATE_END_PROCESS;
				ModemManagerData.nMdmSubState = MDM_SUB_STATE_WAIT_RESPONSE;
			}
			break;
		case MDM_SUB_STATE_WAIT_RESPONSE:
			switch(g_stModem_Rep.eMDResponse) {
				case eMDResponseStateOK:
					ModemManagerData.nMdmSubState = ModemManagerData.nMdmSubNextState;
					g_stModem_Rep.eMDResponse = eMDResponseStateWait;
					Trace("MDM: eMDResponseStateOK\r\n");
					break;

				case eMDResponseStateFAIL:
					Trace("MDM: eMDResponseStateFAIL\r\n");
					ModemManagerData.nMdmSubState = 7;
					s_ucRetryCnt++;
					if(s_ucRetryCnt>3)	ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateNoCarrier:
					Trace("MDM: eMDResponseStateNoCarrier\r\n");
					ModemManagerData.nMdmSubState = 7;
					s_ucRetryCnt++;
					if(s_ucRetryCnt>3)	ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateTimeOut:
					Trace("MDM: eMDResponseStateTimeOut\r\n");
					ModemManagerData.nMdmSubState = 7;
					s_ucRetryCnt++;
					if(s_ucRetryCnt>3)	ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				case eMDResponseStateCMEError:
					Trace("MDM: eMDResponseStateCMEError\r\n");
					ModemManagerData.nMdmSubState = 7;
					s_ucRetryCnt++;
					if(s_ucRetryCnt>3)	ModemManagerData.nMdmSubState = MDM_SUB_STATE_MODEM_FAIL;
					break;

				default:
					break;
			}
			break;
		case MDM_SUB_STATE_MODEM_FAIL:
            s_ucRetryCnt=0;
			if( iTimer_AGPS_Send_TimeoutDly != -1 )
			{
				HalTimerClearSWTimer(iTimer_AGPS_Send_TimeoutDly);
				iTimer_AGPS_Send_TimeoutDly=-1;
			}
			return eMODEM_PROCESS_FUNC_RET_FAIL;

		case MDM_SUB_STATE_END_PROCESS:
			s_ucRetryCnt=0;
			ModemManagerData.nMdmSubNextState = 0;
			ModemManagerData.nMdmSubState = 0;
			printf("MDM: AGPS PROCESS FINISH\r\n");
			return eMODEM_PROCESS_FUNC_RET_OK;

	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

eMODEM_PROCESS_FUNC_RET ProcessSendAGPSDataResult(void)
{
	if( iTimer_AGPS_Send_TimeoutDly != -1 )
	{
		HalTimerClearSWTimer(iTimer_AGPS_Send_TimeoutDly);
		iTimer_AGPS_Send_TimeoutDly = -1;
	}
	SetReqWaitNetworkCheck(false);
    SetModemState(eMODEM_READY);
	return eMODEM_PROCESS_FUNC_RET_OK;
}
#endif	//RF_COMMON_MODEM 

