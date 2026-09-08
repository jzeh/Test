/**
  ******************************************************************************
  * @file    TestMainManager.c
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
#include "GIT_BluetoothLowEnergy.h"

#include "cli.h"
//#include "UARTDMA_Manager.h"
#include "FOTA_Manager.h"
#include "GIT_SensorProc.h"
#include "Modem_comm.h"
#include "app_cli.h"
#include "Power_Manager.h"
#include "AutolinkConfiguration.h"
#include "GIT_VCI.h"
#include "GIT_Util.h"
#include "GIT_Gps.h"
#include "HdDebug.h"
#include "MngStorage.h"
#include "MngSystem.h"
#include "MngQueue.h"
#include "AutolinkConfig.h"
#include "GIT_OemInterface.h"
#include "MngSystemUtil.h"
#include "Modem_Manager.h"
#include "GIT_InterProtocol.h"
#include "OBD_Controller.h"
#include "OBD_Manager.h"
#include "SysHalFileSystem.h"
#if defined(FEATURE_EXTENSION_BOARD)
#include "SysPsExtend.h"
#endif
#include "Autolink_Manager.h"
#include "OBD_Manager.h"

#include "HalHandler.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_APP,__VA_ARGS__)

//#define ENABLE_MANAGER_USED_TIME

/* Define ------------------------------------------------------------------*/
#define TCUSLOPE_MAX_COUNT 10
/* Variable ------------------------------------------------------------------*/
AUTOLINK_MANAGER_DATA AutoLinkManagerData;
SYSTEM_TEST_INFO stSystemTestInfo;
volatile uint16_t gnTestState_SFlash;
bool g_bYUJINSelftestFlag = false;												// 유진 사용 생산프로그램
bool g_bHYPERTECSelftestFlag = false;											// 하이퍼텍 사용 생산프로그램
bool g_bUSIMInsertFlag = false;
bool g_bCanParsingRunFlag = false;
U8 g_arrYUJINTestIdx[2] = {0xFF,};

extern stAutolinkConfigData m_stAutolinkConfigData;
extern bool g_bIndicatorDBFlag;
extern unsigned char g_ucI2CFlag[2];
extern long long Get_DrivingKey(void);
extern stUserActionSetting m_stUserActionSetting;

extern uint8_t g_bEnableModemDirectCommunication;
extern uint8_t g_bEnableBLEDirectCommunication;
extern uint8_t g_bEnableGPSDirectCommunication;
extern uint8_t g_bEnableSELFTESTDirectCommunication;
extern uint8_t g_bEnableBLEViewRes;
extern uint8_t g_bEnableGPSViewRes;

extern unsigned int g_unBkramRtcSetSignalFlag;
extern unsigned int g_unBkramTmpOdometer;
extern unsigned int g_unBkramSwResetSignal;
extern unsigned short int g_usBkramResetCount;
extern unsigned int g_unBkramClearOdoFlag;

extern eFUEL_TYPE	g_eFuel_Type;
extern bool g_bDBParsingFailFlag;

#pragma section="BKSRAM"
WAKEUP_DETECT_PIN_STATE WakeupDetectPinState @"BKSRAM";				// backup ram에 저장되는 변수

/* Function ------------------------------------------------------------------*/
void InitializeTestManager(void);

void TestCommCAN1_CAN2(void);
void GITTestManager(void);
bool TestFileWrite(void);
bool TestFileRead(void);
void GITTest_SFlash(void);
void TestHW(void);
void SetDebugMode();
void CheckConfigurationDate();

extern void MngSystem();
extern int32_t MngModem(int32_t nLparam, int32_t nRparam);
extern void ModemInitalize();
extern int HdMngSysMsgInit();
extern void MngSystemInit();
extern void SetupInterruptLatch(boolean_t bPowerOff);
extern void SystemDelay(uint16_t delay);
extern void SetAutolinkConfig2System(stAutolinkConfigData* pstAutolinkConfigData);
extern void GPS_DisableCommunication(void);
extern void ReadConfig(boolean_t bDisp);
extern void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern void SetRestUBlox();
extern void SetDiableNmea();
extern uint32_t GetUTCTime();
extern eGitFresult MSG_DelAllErrorFile(void);
extern int16_t CalcChecksum_Short(unsigned char* pBuff, unsigned int uiLength);
//extern unsigned char g_ucDBParsingFlag;
/* ---------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*/
void InitializeAutoLinkManager(void)
{
	AutoLinkManagerData.eState = eAUTOLINK_STATE_INIT;

	return;
}

#include "MngSystem.h"
#include "MngStorage.h"

#define EXT_INT_CAN     0x00000001
#define EXT_INT_MDM     0x00000002
#define EXT_INT_SENSOR  0x00000004
#define EXT_INT_BT      0x00000008

enum {
    SYS_POWER_STATE_COLD_BOOT = 0x0,
    SYS_POWER_STATE_RESET = 0x10,
};

int32_t m_iAutolinkMode = SYSTEM_MODE_PARKING;

void WakeupGemaltoModem()
{
    HalGPIOSetVaule(UART2_RTS_PIN, eBIT_SET);
    SystemDelay(10);
    HalGPIOSetVaule(UART2_RTS_PIN, eBIT_RESET);
    SystemDelay(200);
    HalGPIOSetVaule(UART2_RTS_PIN, eBIT_SET);
    SystemDelay(10);
    HalGPIOSetVaule(UART2_RTS_PIN, eBIT_RESET);
}

#if defined(PROTOCOL17)
stServerUrl g_stServerUrl;
#endif

#define ATTSTRING(x,y) (x##y)

void SetServerUrl()
{
    char * carrSeriaNumber = GetFWSerialNumber();
#if defined(PROTOCOL17)
#else
	memset((char*)&g_stServerUrl,0,sizeof(g_stServerUrl));

    if( strstr(carrSeriaNumber,"KR") != NULL )
    {
//        memcpy(g_stServerUrl.Url,ATTSTRING(KR,_HTTP_RETAIL_DEV_MESSAGE_URL),sizeof(ATTSTRING(KR,_HTTP_RETAIL_DEV_MESSAGE_URL)));
//        memcpy(g_stServerUrl.Path,ATTSTRING(KR,_HTTP_RETAIL_DEV_MESSAGE_PATH),sizeof(ATTSTRING(KR,_HTTP_RETAIL_DEV_MESSAGE_PATH)));
		memcpy(g_stServerUrl.Url,ATTSTRING(KR,_HTTP_RETAIL_MESSAGE_URL),sizeof(ATTSTRING(KR,_HTTP_RETAIL_MESSAGE_URL)));
        memcpy(g_stServerUrl.Path,ATTSTRING(KR,_HTTP_RETAIL_MESSAGE_PATH),sizeof(ATTSTRING(KR,_HTTP_RETAIL_MESSAGE_PATH)));
    }
    else if( strstr(carrSeriaNumber,"AU") != NULL )
    {
//        memcpy(g_stServerUrl.Url,ATTSTRING(AU,_HTTP_RETAIL_DEV_MESSAGE_URL),sizeof(ATTSTRING(AU,_HTTP_RETAIL_DEV_MESSAGE_URL)));
//        memcpy(g_stServerUrl.Path,ATTSTRING(AU,_HTTP_RETAIL_DEV_MESSAGE_PATH),sizeof(ATTSTRING(AU,_HTTP_RETAIL_DEV_MESSAGE_PATH)));
		memcpy(g_stServerUrl.Url,ATTSTRING(AU,_HTTP_RETAIL_MESSAGE_URL),sizeof(ATTSTRING(AU,_HTTP_RETAIL_MESSAGE_URL)));
        memcpy(g_stServerUrl.Path,ATTSTRING(AU,_HTTP_RETAIL_MESSAGE_PATH),sizeof(ATTSTRING(AU,_HTTP_RETAIL_MESSAGE_PATH)));
    }
    else if( strstr(carrSeriaNumber,"NZ") != NULL )
    {
        //memcpy(g_stServerUrl.Url,ATTSTRING(NZ,_HTTP_RETAIL_DEV_MESSAGE_URL),sizeof(ATTSTRING(NZ,_HTTP_RETAIL_DEV_MESSAGE_URL)));
        //memcpy(g_stServerUrl.Path,ATTSTRING(NZ,_HTTP_RETAIL_DEV_MESSAGE_PATH),sizeof(ATTSTRING(NZ,_HTTP_RETAIL_DEV_MESSAGE_PATH)));
        memcpy(g_stServerUrl.Url,ATTSTRING(NZ,_HTTP_RETAIL_MESSAGE_URL),sizeof(ATTSTRING(NZ,_HTTP_RETAIL_MESSAGE_URL)));
        memcpy(g_stServerUrl.Path,ATTSTRING(NZ,_HTTP_RETAIL_MESSAGE_PATH),sizeof(ATTSTRING(NZ,_HTTP_RETAIL_MESSAGE_PATH)));
    }
	else if( strstr(carrSeriaNumber,"VNK") != NULL )
	{
		memcpy(g_stServerUrl.Url,ATTSTRING(VN,_HTTP_RETAIL_MESSAGE_URL),sizeof(ATTSTRING(VN,_HTTP_RETAIL_MESSAGE_URL_KIA)));
        memcpy(g_stServerUrl.Path,ATTSTRING(VN,_HTTP_RETAIL_MESSAGE_PATH),sizeof(ATTSTRING(VN,_HTTP_RETAIL_MESSAGE_URL_KIA)));
	}
    else
    {
        //memcpy(g_stServerUrl.Url,ATTSTRING(KR,_HTTP_RETAIL_DEV_MESSAGE_URL),sizeof(ATTSTRING(KR,_HTTP_RETAIL_DEV_MESSAGE_URL)));
        //memcpy(g_stServerUrl.Path,ATTSTRING(KR,_HTTP_RETAIL_DEV_MESSAGE_PATH),sizeof(ATTSTRING(KR,_HTTP_RETAIL_DEV_MESSAGE_PATH)));
        memcpy(g_stServerUrl.Url,ATTSTRING(KR,_HTTP_RETAIL_MESSAGE_URL),sizeof(ATTSTRING(KR,_HTTP_RETAIL_MESSAGE_URL)));
        memcpy(g_stServerUrl.Path,ATTSTRING(KR,_HTTP_RETAIL_MESSAGE_PATH),sizeof(ATTSTRING(KR,_HTTP_RETAIL_MESSAGE_PATH)));
    }

    printf(">>>>> URL : %s\n",g_stServerUrl.Url);
    printf(">>>>> Path : %s\n",g_stServerUrl.Path);
#endif
}

bool CheckDevServerUrl()
{
	bool bResult = false;
	// Message_Manager.h?? ???? ???? url ????? ??? u?????? ??? ???? ??.
	if 		( strncmp(g_stServerUrl.Url, KR_HTTP_RETAIL_DEV_MESSAGE_URL, strlen(KR_HTTP_RETAIL_DEV_MESSAGE_URL)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, KR_HTTP_RETAIL_DEV_MESSAGE_URL2, strlen(KR_HTTP_RETAIL_DEV_MESSAGE_URL2)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, AU_HTTP_RETAIL_DEV_MESSAGE_URL, strlen(AU_HTTP_RETAIL_DEV_MESSAGE_URL)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, AU_HTTP_FLEET_DEV_MESSAGE_URL, strlen(AU_HTTP_FLEET_DEV_MESSAGE_URL)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, KR_HTTP_FLEET_DEV_MESSAGE_URL, strlen(KR_HTTP_FLEET_DEV_MESSAGE_URL)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, NZ_HTTP_RETAIL_DEV_MESSAGE_URL, strlen(NZ_HTTP_RETAIL_DEV_MESSAGE_URL)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, KR_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, strlen(KR_HTTP_RETAIL_DEV_MESSAGE_URL_KIA)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, AU_HTTP_FLEET_DEV_MESSAGE_URL_KIA, strlen(AU_HTTP_FLEET_DEV_MESSAGE_URL_KIA)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, KR_HTTP_FLEET_DEV_MESSAGE_URL_KIA, strlen(KR_HTTP_FLEET_DEV_MESSAGE_URL_KIA)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, VN_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, strlen(VN_HTTP_RETAIL_DEV_MESSAGE_URL_KIA)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, SG_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, strlen(SG_HTTP_RETAIL_DEV_MESSAGE_URL_KIA)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, RU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, strlen(RU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA)) == 0 ) bResult = true;
	else if ( strncmp(g_stServerUrl.Url, AU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, strlen(AU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA)) == 0 ) bResult = true;
//	if ( strncmp(g_stServerUrl.Url, HTTP_FLEET_DEV_MESSAGE_URL, strlen(HTTP_FLEET_DEV_MESSAGE_URL)) == 0 ) bResult = true;

	if ( bResult == true )
	    printf(">>>>> Development server URL is Set : %s\n",g_stServerUrl.Url);

	return bResult;
}

void ConfigureAutolinkSystem()
{
    // wake up gemalto modem from sleep
    WakeupGemaltoModem();

    // disable interrupt latch
    SetupInterruptLatch(false);

    // test code after test we should delete this fucntion
    MSG_DelAllErrorFile();

    // read all configuration data from serial flash
	if ( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_NOSERIAL_NUMBER) != 0 )
	{
		ClearSystemMessageUserData();
		ReadConfig(true);
		// check backup ram information;
		CheckConfigurationDate();
	}

	if( AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON )
	{
		if( m_stUserActionSetting.Tracking.usCheckSum != CalcChecksum_Short(&m_stUserActionSetting.Tracking.bActive,sizeof(m_stUserActionSetting.Tracking)-2) )
		{
			memset(&m_stUserActionSetting.Tracking,0x00,sizeof(m_stUserActionSetting.Tracking));
		}
	}

    // set server url according to the nation of serial number
    SetServerUrl();
}

#ifdef ENABLE_MANAGER_USED_TIME
enum{
   MNG_CLI = 0,
   MNG_POWER,
   MNG_MODEM_OLD,
   MNG_MODEM_NEW,
   MNG_OBD,
   MNG_COMMUNICATION,
   MNG_SYSTEM,
   MNG_STORAGE,
   MNG_SENSOR,
   MNG_MAX,
};

char* pcarrManagerName[MNG_MAX]=
{"CLI_MNG",
"PWR_MNG",
"MDM_OLD",
"MDM_NEW",
"OBD_MNG",
"CMC_MNG",
"SYS_MNG",
"STR_MNG",
"SENSORM",
};


unsigned long m_ulManagerUsedtime = 0;
float m_ularrManagerUsedTime[MNG_MAX];

int32_t m_nUsedTimeCheckTimeoutTimerID = -1;

#define MNG_USED_TIME_CHECK_TIME 10000

void UsedTimeCheckTimeoutTimerCallBack()
{
    float ulDiffTmr = 0;

    Trace("==============================================================\r\n");
    for(int32_t i=0;i<MNG_MAX;i++)
    {
        Trace("MNG[ %s ]: %f\n",pcarrManagerName[i],(m_ularrManagerUsedTime[i]/MNG_USED_TIME_CHECK_TIME)*100);
        ulDiffTmr+=((m_ularrManagerUsedTime[i]/MNG_USED_TIME_CHECK_TIME)*100);
    }
    Trace("==============================================================\r\n");
    Trace("Manager Used Time for %d seconds : %f\n",MNG_USED_TIME_CHECK_TIME/1000,ulDiffTmr);
    Trace("==============================================================\r\n");
    memset(m_ularrManagerUsedTime,0,sizeof(m_ularrManagerUsedTime));
}
#endif //ENABLE_MANAGER_USED_TIME

void SetDebugMode()
{
	SetSysDebugMode(DEBUG_MODE_MODULES);
	SetSysDebugModule(DEBUG_MODULES_OBD,false);
	SetSysDebugModule(DEBUG_MODULES_POWER,false);
	SetSysDebugModule(DEBUG_MODULES_MODEM,false);
	SetSysDebugModule(DEBUG_MODULES_OBD_PERIOD_LOG,false);
	SetSysDebugModule(DEBUG_MODULES_OBD_DBPARSING_LOG,false);
	SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_LOG,false);
	SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_PARSING_LOG,false);
	SetSysDebugModule(DEBUG_MODULES_OBD_SYSTEM_MSG,false);
	SetSysDebugModule(DEBUG_MODULES_MODEM_COM,false);
	SetSysDebugModule(DEBUG_MODULES_UTIL,false);
	SetSysDebugModule(DEBUG_MODULES_USB,false);
	SetSysDebugModule(DEBUG_MODULES_SYSTEM_HADNLER,false);
	SetSysDebugModule(DEBUG_MODULES_SYSTEM,false);
	SetSysDebugModule(DEBUG_MODULES_STORAGE,false);
	SetSysDebugModule(DEBUG_MODULES_ASSERT,false);
	SetSysDebugModule(DEBUG_MODULES_APP,false);
	SetSysDebugModule(DEBUG_MODULES_FILE_SYSTEM,false);
	SetSysDebugModule(DEBUG_MODULES_GPS,false);
	SetSysDebugModule(DEBUG_MODULES_CLI,true);
	SetSysDebugModule(DEBUG_MODULES_BLE,false);
	SetSysDebugModule(DEBUG_MODULES_SENSOR,false);
	SetSysDebugModule(DEBUG_MODULES_QUEUE,false);
#if false
	SetSysDebugMode(DEBUG_MODE_MODULES);
	SetSysDebugModule(DEBUG_MODULES_OBD,false);
	SetSysDebugModule(DEBUG_MODULES_POWER,true);
	SetSysDebugModule(DEBUG_MODULES_MODEM,true);
	SetSysDebugModule(DEBUG_MODULES_OBD_PERIOD_LOG,false);
	SetSysDebugModule(DEBUG_MODULES_OBD_DBPARSING_LOG,false);
	SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_LOG,false);
	SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_PARSING_LOG,false);
	SetSysDebugModule(DEBUG_MODULES_OBD_SYSTEM_MSG,true);
	SetSysDebugModule(DEBUG_MODULES_MODEM_COM,true);
	SetSysDebugModule(DEBUG_MODULES_UTIL,false);
	SetSysDebugModule(DEBUG_MODULES_USB,false);
	SetSysDebugModule(DEBUG_MODULES_SYSTEM_HADNLER,true);
	SetSysDebugModule(DEBUG_MODULES_SYSTEM,true);
	SetSysDebugModule(DEBUG_MODULES_STORAGE,true);
	SetSysDebugModule(DEBUG_MODULES_ASSERT,true);
	SetSysDebugModule(DEBUG_MODULES_APP,true);
	SetSysDebugModule(DEBUG_MODULES_FILE_SYSTEM,true);
 	SetSysDebugModule(DEBUG_MODULES_GPS,true);
	SetSysDebugModule(DEBUG_MODULES_CLI,true);
	SetSysDebugModule(DEBUG_MODULES_BLE,true);
	SetSysDebugModule(DEBUG_MODULES_SENSOR,false);
	SetSysDebugModule(DEBUG_MODULES_QUEUE,false);
    
	//SetSysDebugModule(DEBUG_MODULES_OBD_CANFD,true);

#endif
}

void AGPSInit()
{
    // GPS LED should be invalid at booting time.
    SetLedOnOffCtl(LED_OFF, eLED_GPS);

	if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON)
    {
        // restart gps for agps
#if defined(USE_UBLOX_GPS)
        SetRestUBlox();
        APP_Delay(5);
        SetRestUBlox();
        APP_Delay(5);
#endif

#ifdef ENABLE_UBLOX_GPS_DATA
        SetDiableNmea();
#endif
	}
}

void Can_Parsing_Manager(eCAN_PARSING_TYPE eType)
{
	unsigned int uiRecvLen = 0;
	unsigned char ucTempBuff[3000];
	if( g_bIndicatorDBFlag == true )
	{

		if( eType == eCAN_PARSING_TYPE_CAN1 || eType == eCAN_PARSING_TYPE_ALL )
		{
			uiRecvLen = 0;
			if ( (g_stGitCommInfo[eCOMM_TYPE_CAN1].eCommSate == eCOMM_STATE_CONNECTED) && (g_stGitCommInfo[eCOMM_TYPE_CAN1].fnRecvData != NULL )) {
				uiRecvLen = g_stGitCommInfo[eCOMM_TYPE_CAN1].fnRecvData(ucTempBuff, MAX_CAN_BUFF_SIZE, NULL, (eCommType)eCOMM_TYPE_CAN1);

				if ( uiRecvLen ) {
					PushMultiDataQueue((stQueue*)g_stGitCommInfo[eCOMM_TYPE_CAN1].pstInQueue, ucTempBuff, uiRecvLen, eCOMM_TYPE_CAN1);
				}
			}

			if ( g_stGitCommInfo[eCOMM_TYPE_CAN1].pstInQueue != NULL && g_stGitCommInfo[eCOMM_TYPE_CAN1].fnParsing != NULL ) {
				g_stGitCommInfo[eCOMM_TYPE_CAN1].fnParsing(g_stGitCommInfo[eCOMM_TYPE_CAN1].pstInQueue, GetQueueDataLength(((stQueue*)g_stGitCommInfo[eCOMM_TYPE_CAN1].pstInQueue)), NULL, (eCommType)eCOMM_TYPE_CAN1);
			}
		}

		if( eType == eCAN_PARSING_TYPE_CAN2  || eType == eCAN_PARSING_TYPE_ALL )
		{
			uiRecvLen = 0;
			if ( (g_stGitCommInfo[eCOMM_TYPE_CAN2].eCommSate == eCOMM_STATE_CONNECTED) && (g_stGitCommInfo[eCOMM_TYPE_CAN2].fnRecvData != NULL )) {
				uiRecvLen = g_stGitCommInfo[eCOMM_TYPE_CAN2].fnRecvData(ucTempBuff, MAX_CAN_BUFF_SIZE, NULL, (eCommType)eCOMM_TYPE_CAN2);

				if ( uiRecvLen ) {
					PushMultiDataQueue((stQueue*)g_stGitCommInfo[eCOMM_TYPE_CAN2].pstInQueue, ucTempBuff, uiRecvLen, eCOMM_TYPE_CAN2);
				}
			}

			if ( g_stGitCommInfo[eCOMM_TYPE_CAN2].pstInQueue != NULL && g_stGitCommInfo[eCOMM_TYPE_CAN2].fnParsing != NULL ) {
				g_stGitCommInfo[eCOMM_TYPE_CAN2].fnParsing(g_stGitCommInfo[eCOMM_TYPE_CAN2].pstInQueue, GetQueueDataLength(((stQueue*)g_stGitCommInfo[eCOMM_TYPE_CAN2].pstInQueue)), NULL, (eCommType)eCOMM_TYPE_CAN2);
			}
		}
	}
}

int ReportAlramState(eMESSAGE_EVENT_KEY eEVENT, char* iValue, int nSize)
{
	stMsgSysMsg msg;
    memset((char*)&msg,0,sizeof(stMsgSysMsg));

	msg.header.id = eMngSysMsg;
    msg.header.event = eReqSaveReport;
    msg.header.subEvent = eR_Alram;

    msg.carReport.rpAlram.CarStatus.OperationKey = Get_DrivingKey();
    msg.carReport.rpAlram.CarStatus.OccurredEventTime=GetLocalTimefromTime(GetUTCTime());
    msg.carReport.rpAlram.CarStatus.OccurredEventUtcTime=GetUTCTime();
    msg.carReport.rpAlram.CarStatus.EventKey = eEVENT;
    memcpy(msg.carReport.rpAlram.CarStatus.EventKeyValue,iValue,nSize);
    msg.carReport.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    msg.carReport.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
    msg.carReport.rpAlram.CarStatus.GpsSetLatitude = 0;
    msg.carReport.rpAlram.CarStatus.GpsSetLongitude = 0;
    msg.carReport.rpAlram.CarStatus.Distance = 0;
    msg.carReport.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;

//    Send2MngSysMsg(eMngSysMsg,eReqReport,eR_Alram,&msg,0);
	Send2MngStorage2((stMsgStorage*)&msg);

    return 0;
}

// ver.67이상에서  1회성으로 UTC시간으로 설정 필요
void ChangeSystemRTCTimeBaseToUTCTime()
{
    stHalRTCTypeDef stHalRtcDateTime;

	HalDrvRdBkRam((unsigned char*)&g_unBkramRtcSetSignalFlag, BKRAM_RTC_SET_SIGNAL_FLAG_ADDR, BKRAM_RTC_SET_SIGNAL_FLAG_SIZE);
    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRTCTypeDef), 0);

	if ( (g_unBkramRtcSetSignalFlag != RTC_SET_ENABLE) && (stHalRtcDateTime.RtcDate.RTC_Year != 17/* cold boot이 아닌 경우만*/) )
	{
        g_unBkramRtcSetSignalFlag = RTC_SET_ENABLE;
		HalDrvWrBkRam((unsigned char*)&g_unBkramRtcSetSignalFlag, BKRAM_RTC_SET_SIGNAL_FLAG_ADDR, BKRAM_RTC_SET_SIGNAL_FLAG_SIZE); // RTC Setting UT 로 변경 : 변경 소스 포타 완료 후 최초 1번만 모뎀 Reset 진행

		printf(" Modem H/W Reset To Set RTC \r\n");

#ifdef RF_COMMON_MODEM  //mod.kks 21.10.25
		HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		APP_Delay(200);
		HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
		APP_Delay(200);
		HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);

#else
		HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
		APP_Delay(5);
		HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
		APP_Delay(5);
		HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
#endif
		printf("PowerOnGemaltoModem_Modem_Reset_Rst!\r\n");

		AutoLinkManagerData.eSystemResetMode = eSYSTEM_RESET_MODE_POWER_ON;
	}
}

void SetCANInformation(void)
{
	uint8_t ucPACVType=PA;
	uint8_t ucAutovinCANLine = HIGHCAN1;
	
	if(GetPACVType() == CV)
	{
		ucPACVType = CV;
		if(FUELTYPE_GET_STATE() == ELECTRONIC) ucAutovinCANLine = HIGHCAN3;
	
	}
	
	SetAutolinkConfigProperty(eAutoLinkConfig_PACVType,(void*)&ucPACVType);
	SetAutolinkConfigProperty(eAutoLinkConfig_AutovinCANLine,(void*)&ucAutovinCANLine);

}

void AutoLink_MainProc(void)
{
#ifdef TEST_GEO_FENCE
    int32_t i=0;
#endif //#ifdef TEST_GEO_FENCE
    // set up debug mode
    SetDebugMode();

    AGPSInit();

    // initialize queue.
    MngQueueInit();

    // initialize autolink manager
    InitializeAutoLinkManager();

	// Serial 정보를 가지고 와서 init 하는 부분이 추가되어 InitFWInfo함수를 더 빠르게 호출
	// 2018.09.04 SPARROW : Serial 정보로 modem 초기값 변경하도록 수정
	InitFirmwareInfo();
	OBDManagerInit();

#if defined(FEATURE_EXTENSION_BOARD)
        // initialize extension board
        SysPsExtensionInit();
	if( GetServiceType() == DCS_Fleet ) Uart8_Baudrate_Set(115200);
#elif defined(PROTOCOL14)
	if( GetServiceType() == DCS_Fleet ) Uart8_Baudrate_Set(9600);
#endif

	SetSelftestModemUart();

    // set up system with interrup and environment file
    ConfigureAutolinkSystem();

	// set PA/CV Type and AutoVIN CAN Line
	if(g_bDBParsingFailFlag != true) SetCANInformation(); 
	
    GetWakeupDetectPinsState();
#if defined TIME_CHECK
    stHalRTCTypeDef stHalRtcDateTime;
	int unTime=1483196413;
	GetDatefromTime2(&stHalRtcDateTime,unTime);
	uint8_t arrTmp[64];
	sprintf((char *)arrTmp, "%04d%02d%02d%0.2d%0.2d%0.2d\x00",
					2000 + stHalRtcDateTime.RtcDate.RTC_Year,
					stHalRtcDateTime.RtcDate.RTC_Month,
					stHalRtcDateTime.RtcDate.RTC_Date,
					stHalRtcDateTime.RtcTime.RTC_Hours,
					stHalRtcDateTime.RtcTime.RTC_Minutes,
					stHalRtcDateTime.RtcTime.RTC_Seconds);

	printf("%s\r\n",arrTmp);
#endif

	ChangeSystemRTCTimeBaseToUTCTime();

	while(true) {
        HalAPI_ShowGpsInfoEvery5Sec();
		switch(AutoLinkManagerData.eState) {
			case eAUTOLINK_STATE_INIT:
				VCI_Initialize();
				InitDCSProtocol();
				InitGpsParser();

                //MONI 2018-02-09
                // this initilize make the system reenter the "eAUTOLINK_STATE_INIT" state
                // so we don't need call this initialize.
				//InitializeAutoLinkManager();
				InitializePowerManager();
				InitializeModemManager(false);
				InitializeSensorManager();
				InitializeMessageManager();

				InitializeTestManager();

				InitializeCommandLineInterface();

                MngSystemInit();
                HdMngSysMsgInit();
                ModemInitalize();

                //MONI 2018-02-24
                // we don't neet to wait for 2 seconds herer.
                // so we set next step to run.
				//AutoLinkManagerData.iTimerAppDly = HalTimerSetSWTimer(2000, eSWTimer_INFINITE, NULL, true);
				//AutoLinkManagerData.eState = eAUTOLINK_STATE_START_CAN;
				AutoLinkManagerData.eState = eAUTOLINK_STATE_RUN;
#ifdef ENABLE_MANAGER_USED_TIME
                // check used time for manager
                EnableSystemMessageTimer(&m_nUsedTimeCheckTimeoutTimerID,MNG_USED_TIME_CHECK_TIME,eSWTimer_INFINITE,UsedTimeCheckTimeoutTimerCallBack);
#endif //#ifdef ENABLE_MANAGER_USED_TIME
				if( (g_ucI2CFlag[0] != 0) || (g_ucI2CFlag[1] != 0) )	ReportAlramState(eMESSAGE_EVENT_KEY_STATE_ALRAM,(char*)g_ucI2CFlag,2);
				break;
			case eAUTOLINK_STATE_START_CAN:
				if(HalTimerGetSwTimerCount(AutoLinkManagerData.iTimerAppDly) == 0) {
					HalTimerStartSWTimer(AutoLinkManagerData.iTimerAppDly, eSWTimer_ONESHOT);	//warning delete
//					AutoLinkManagerData.iTimerAppDly = HalTimerSetSWTimer(1000, eSWTimer_INFINITE, NULL, true);
					AutoLinkManagerData.eState = eAUTOLINK_STATE_RUN;
				}
				break;
			case eAUTOLINK_STATE_RUN:
#ifdef ENABLE_MANAGER_USED_TIME
                m_ulManagerUsedtime = OemGetTmr();
#endif //#ifdef ENABLE_MANAGER_USED_TIME

				CLI_Manager();				// command line interface - debug 환경

#ifdef ENABLE_MANAGER_USED_TIME
				m_ularrManagerUsedTime[MNG_CLI] += OemGetTmrDelta(OemGetTmr(), m_ulManagerUsedtime);
                m_ulManagerUsedtime = OemGetTmr();
#endif //#ifdef ENABLE_MANAGER_USED_TIME

                Power_Manager();

#ifdef ENABLE_MANAGER_USED_TIME
                m_ularrManagerUsedTime[MNG_POWER] += OemGetTmrDelta(OemGetTmr(), m_ulManagerUsedtime);
                m_ulManagerUsedtime = OemGetTmr();
#endif //#ifdef ENABLE_MANAGER_USED_TIME

#ifdef USE_GEMALTO_MODEM
				if(g_bYUJINSelftestFlag != true && g_bEnableModemDirectCommunication == false)	
				    ModemManager();
#endif

#ifdef ENABLE_MANAGER_USED_TIME
                m_ularrManagerUsedTime[MNG_MODEM_OLD] += OemGetTmrDelta(OemGetTmr(), m_ulManagerUsedtime);
                m_ulManagerUsedtime = OemGetTmr();
#endif //#ifdef ENABLE_MANAGER_USED_TIME

                if(g_bEnableModemDirectCommunication == false)	MngModem(0, 0);

#ifdef ENABLE_MANAGER_USED_TIME
                m_ularrManagerUsedTime[MNG_MODEM_NEW] += OemGetTmrDelta(OemGetTmr(), m_ulManagerUsedtime);
                m_ulManagerUsedtime = OemGetTmr();
#endif //#ifdef ENABLE_MANAGER_USED_TIME

				// 2017-07-28 오후 4:45:22 --> FOTA 진행 시에 OBDManager() 함수가 동작되면
				// flash write 시에 오동작이 발행한다.
				OBDManager();

#ifdef ENABLE_MANAGER_USED_TIME
                m_ularrManagerUsedTime[MNG_OBD] += OemGetTmrDelta(OemGetTmr(), m_ulManagerUsedtime);
                m_ulManagerUsedtime = OemGetTmr();
#endif //#ifdef ENABLE_MANAGER_USED_TIME
g_bCanParsingRunFlag = true;
				CommuicationManager();
g_bCanParsingRunFlag = false;
#ifdef ENABLE_MANAGER_USED_TIME
                m_ularrManagerUsedTime[MNG_COMMUNICATION] += OemGetTmrDelta(OemGetTmr(), m_ulManagerUsedtime);
                m_ulManagerUsedtime = OemGetTmr();
#endif //#ifdef ENABLE_MANAGER_USED_TIME

                MngSystem();

#ifdef ENABLE_MANAGER_USED_TIME
                m_ularrManagerUsedTime[MNG_SYSTEM] += OemGetTmrDelta(OemGetTmr(), m_ulManagerUsedtime);
                m_ulManagerUsedtime = OemGetTmr();
#endif //#ifdef ENABLE_MANAGER_USED_TIME
g_bCanParsingRunFlag = true;
Can_Parsing_Manager(eCAN_PARSING_TYPE_ALL);
g_bCanParsingRunFlag = false;
                MngStorage();
g_bCanParsingRunFlag = true;
Can_Parsing_Manager(eCAN_PARSING_TYPE_ALL);
g_bCanParsingRunFlag = false;
#ifdef ENABLE_MANAGER_USED_TIME
                m_ularrManagerUsedTime[MNG_STORAGE] += OemGetTmrDelta(OemGetTmr(), m_ulManagerUsedtime);
#endif //#ifdef ENABLE_MANAGER_USED_TIME

                GIT_SensorManager();

#ifdef ENABLE_MANAGER_USED_TIME
                m_ularrManagerUsedTime[MNG_SENSOR] += OemGetTmrDelta(OemGetTmr(), m_ulManagerUsedtime);
                m_ulManagerUsedtime = OemGetTmr();
#endif //#ifdef ENABLE_MANAGER_USED_TIME


#if defined(FEATURE_EXTENSION_BOARD)
                SysExtendBoardProcess();
#endif

				break;

			case eAUTOLINK_STATE_IDLE:
				break;

			default:
				break;
		}
	}
}

void TestHW(void)
{
	static uint8_t bTestStep;
	uint32_t wFlashID = 0;

	switch(bTestStep) {
		case 0:
			// gps
			g_bEnableGPSViewRes = true;

			bTestStep = 1;
			break;

		case 1:
			g_bEnableGPSViewRes = false;
			// modem
			bTestStep = 2;

			if(ModemManagerData.bModemRcvSysStartMessageFlag == true) {
				if(GetModemState() == eMODEM_READY) {
				}
			}
			break;

		case 2:
                    // ble
			printf("\n\nBLE: Init\r\n");
			BTSetState(eBT_initialized);
			BTGetLocalAddressReq();
			bTestStep = 3;
			break;

		case 3:
			// sflash
			wFlashID = sFLASH_ReadID();
			printf("\n\nSFALSH: Serial Flash ID: 0x%x\n", wFlashID);				// micron N25Q064A ID: 0x20BA17

			g_bEnableGPSViewRes = true;
			printf("\n\n\n");
			bTestStep = 0;
			break;

		default:
			bTestStep = 0;
			break;
	}
}


void TestCommCAN1_CAN2(void)
{
	AutoLinkManagerData.bTestMode++;
	if(AutoLinkManagerData.bTestMode > 2) {
		AutoLinkManagerData.bTestMode = 0;
	}

	if(AutoLinkManagerData.bTestMode == 0) {
		if(AutoLinkManagerData.bTestResult1 == false || AutoLinkManagerData.bTestResult2 == false) {
			printf("CAN: Comm Fail\n");
		}
		else {
			printf("CAN: Comm Success\n");
		}

		printf("\nCAN: Select CAN1(High) CH1\n");
//		Can_Line_Select(1);
	}
	else if(AutoLinkManagerData.bTestMode == 1) {
		if(AutoLinkManagerData.bTestResult1 == false || AutoLinkManagerData.bTestResult2 == false) {
			printf("CAN: CH1 - Comm Fail\n");
		}
		else {
			printf("CAN: CH1 - Comm Success\n");
		}

		printf("\nCAN: Select CAN1(High) CH2\n");
//		Can_Line_Select(2);
	}
	else {
		if(AutoLinkManagerData.bTestResult1 == false || AutoLinkManagerData.bTestResult2 == false) {
			printf("CAN: CH2 - Comm Fail\n");
		}
		else {
			printf("CAN: CH2 - Comm Success\n");
		}

		printf("\nCAN: Select CAN2(Low)\n");
	}

	AutoLinkManagerData.bTestResult1 = false;
	AutoLinkManagerData.bTestResult2 = true;

	return;
}

TEST_DATA TestData;
int32_t g_bCAN1OK = 0;
int32_t g_bCAN2OK = 0;

void InitializeTestManager(void)
{
	if(stSystemTestInfo.bRunTestMode == false) {
		TestData.eState = eTEST_STATE_IDLE;
		TestData.bEnableTestMode = false;
		TestData.bEnableLoopTestMode = false;

		TestData.iTimerTestDly = HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, NULL, false);				// modem command를 전달하고 ok 응답받기까지 delay
		TestData.iTimerTest2Dly = HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, NULL, false);				// modem command를 전달하고 ok 응답받기까지 delay
	}
	else {
		TestData.eState = eTEST_STATE_IDLE;
		TestData.bEnableTestMode = true;
		TestData.bEnableLoopTestMode = true;

		TestData.iTimerTestDly = HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, NULL, false);				// modem command를 전달하고 ok 응답받기까지 delay
		TestData.iTimerTest2Dly = HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, NULL, true);				// modem command를 전달하고 ok 응답받기까지 delay
	}

	return;
}



#define FILE_NAME_TEST_FILE				"TEST.txt"
#define FILE_TEST_STRING					"TEST: 4 - serial flash read/write"

bool TestFileWrite(void)
{
	UINT dwFileSize;
	stFileSystemDescript fpVer;
	eGitFresult ret = GIT_FR_OK;

	if ( (ret = git_f_open(&fpVer, FILE_NAME_TEST_FILE, GIT_FA_CREATE_ALWAYS | GIT_FA_OPEN_ALWAYS | GIT_FA_WRITE)) == GIT_FR_OK ) {
		ret = git_f_write(&fpVer, (char *)FILE_TEST_STRING, strlen(FILE_TEST_STRING), &dwFileSize);
		git_f_close(&fpVer);
		if(ret != GIT_FR_OK)
		{
			printf("@%s(), %s write fail!!! %d \r\n", __FUNCTION__, FILE_NAME_TEST_FILE, ret);

			return GIT_FR_DISK_ERR;
		}
	}
	else {
		printf("@%s(), %s open fail!!! %d \r\n", __FUNCTION__, FILE_NAME_TEST_FILE, ret);

		return GIT_FR_DISK_ERR;
	}

//	printf("@%s(), %s Success !!! %d \r\n", __FUNCTION__, FILE_NAME_TEST_FILE, ret);


	return GIT_FR_OK;
}

bool TestFileRead(void)
{
	UINT dwFileSize2;
	stFileSystemDescript fpVer;
	eGitFresult ret = GIT_FR_OK;
	uint8_t aTemp[100];

	if ( (ret = git_f_open(&fpVer, FILE_NAME_TEST_FILE, GIT_FA_EXIST|GIT_FA_READ)) == GIT_FR_OK )
	{
		if ((ret = git_f_read(&fpVer, aTemp, strlen(FILE_TEST_STRING), (UINT *)&dwFileSize2)) == GIT_FR_OK )
		{
			git_f_close(&fpVer);

//			printf("TEST: read size: %d\n", dwFileSize2);
			if(strlen(FILE_TEST_STRING) == dwFileSize2) {
				if(memcmp(aTemp, FILE_TEST_STRING, strlen(FILE_TEST_STRING)) == 0) {
					printf("RESULT: OK\n");
				}
				else {
										printf("RESULT: FAIL\n");
				}
				return true;
			}
			else {
				return false;
			}
		}
		else
		{
			git_f_close(&fpVer);
			printf("SYS: %s File Read fail %d\r\n", FILE_NAME_TEST_FILE, ret);

			return false;
		}
	}


	return true;
}

//**********************************************************************************************************************************
// BACKUP SRAM에 할당된 변수들은 아래와 같다.
//**********************************************************************************************************************************
extern WAKEUP_DETECT_PIN_STATE WakeupDetectPinState;				// backup ram에 저장되는 변수
extern BR_SystemInfo BkSram_SystemInfo;
extern BR_ModemInfo BkSram_ModemInfo;
extern stBTInfo g_LocalBTInfo;
extern stLockState g_stLockState;
extern unsigned char g_ucFuelLevel;
extern float g_fFuelLevel;
extern unsigned char g_ucButtonStatus;
extern unsigned long long g_ullIndicator;
extern unsigned char g_ucTCUSlopeAngleArrIndex;
extern int g_nTCUSlopeAngleArr[TCUSLOPE_MAX_COUNT][2];
extern unsigned int g_uiSetDoorSignal;

#if defined(PROTOCOL18)
extern stElectricCarData g_stElectricCarData;

#endif
void ClearSectionBackupRAM(void)
{
#if defined(STM32F427X)
	unsigned int* punBkRamAddr = (unsigned int*)BKPSRAM_BASE; // //BKPSRAM_BASE : 0x40000000+0x00020000+0x4000 = 0x40024000
#elif defined(AT32F435VMT7)
	unsigned int* punBkRamAddr = (unsigned int*)0x2005F000;  // define in Premium_Module_Artery.icf
#endif

    Trace("=============================================\r\n");
    Trace("ClearSectionBackupRAM Enter\n");
    memset(punBkRamAddr, 0x00, BKRAM_MAX);
/*    
	memset((char *)&WakeupDetectPinState, 0x00, sizeof(WAKEUP_DETECT_PIN_STATE));
	memset((char *)&BkSram_ModemInfo, 0x00, sizeof(BR_ModemInfo));
    memset((char *)&BkSram_SystemInfo, 0x00, sizeof(BR_SystemInfo));
	memset((char *)&g_stLockState, 0x00, sizeof(g_stLockState));
	memset((char *)&g_ucFuelLevel, 0x00, sizeof(g_ucFuelLevel));
	memset((char *)&g_fFuelLevel, 0x00, sizeof(g_fFuelLevel));
	memset((char *)&g_ucButtonStatus, 0x00, sizeof(g_ucButtonStatus));
	memset((char *)&g_ullIndicator, 0x00, sizeof(g_ullIndicator));
	memset((char *)&g_ucTCUSlopeAngleArrIndex, 0x00, sizeof(g_ucTCUSlopeAngleArrIndex)); // woong bae 18/11/26
	memset((char *)&g_nTCUSlopeAngleArr, 0x00, sizeof(g_nTCUSlopeAngleArr));
	memset((char *)&g_uiSetDoorSignal, 0x00, sizeof(g_uiSetDoorSignal));
#if defined(PROTOCOL18)
	memset((char *)&g_stElectricCarData, 0x00, sizeof(g_stElectricCarData));
#endif


g_unBkramTmpOdometer		= 0;
g_unBkramSwResetSignal		= 0;
g_unBkramRtcSetSignalFlag	= 0;
g_unBkramClearOdoFlag		= 0;
g_usBkramResetCount 		= 0;
//	memset((char *)&g_LocalBTInfo, 0x00, sizeof(stBTInfo));
*/
    // initialize baudrate
    BkSram_ModemInfo.unBaurdRate = 921600;
    BkSram_SystemInfo.ucDoorLockStatus = 0;
    BkSram_SystemInfo.ucDoorOpenStatus = 0;
    BkSram_SystemInfo.unDrivingInterval = TIMER_DRIVING_INTERVAL_VALUE;
    BkSram_SystemInfo.unFotaInterval = RTC_SET_UPDATE_FIRMWARE_ALRAM_TIME;
    BkSram_SystemInfo.unPowerOffTime = 0;
    BkSram_SystemInfo.unSystemTimeOut = TIMER_SYSTEM_TIMEOUT_INTERVAL_VALUE;
    BkSram_SystemInfo.unWakeUpInterval = RTC_SET_WAKEUP_ALRAM_TIME;
    memset(BkSram_SystemInfo.ucarrRFIDUID,0x20,RFID_08C_DATA_LENGTH);
}

void CheckConfigurationDate()
{
    if( BkSram_SystemInfo.unDrivingInterval != TIMER_DRIVING_INTERVAL_VALUE )
    {
        printf("IMPORTANT : unDrivingInterval is not correct\r\n");
        // data is changed unintentionally
        BkSram_SystemInfo.unDrivingInterval = TIMER_DRIVING_INTERVAL_VALUE;

    }
    if( BkSram_SystemInfo.unFotaInterval != RTC_SET_UPDATE_FIRMWARE_ALRAM_TIME )
    {
        printf("IMPORTANT : unFotaInterval is not correct\r\n");
        // data is changed unintentionally
        BkSram_SystemInfo.unFotaInterval = RTC_SET_UPDATE_FIRMWARE_ALRAM_TIME;
    }
    if( BkSram_SystemInfo.unSystemTimeOut != TIMER_SYSTEM_TIMEOUT_INTERVAL_VALUE )
    {
        printf("IMPORTANT : unSystemTimeOut is not correct\r\n");
        // data is changed unintentionally
        BkSram_SystemInfo.unSystemTimeOut = TIMER_SYSTEM_TIMEOUT_INTERVAL_VALUE;
    }
    if( BkSram_SystemInfo.unWakeUpInterval != RTC_SET_WAKEUP_ALRAM_TIME )
    {
        printf("IMPORTANT : unWakeUpInterval is not correct\r\n");
        // data is changed unintentionally
        BkSram_SystemInfo.unWakeUpInterval = RTC_SET_WAKEUP_ALRAM_TIME;
    }

    DCSEncryptType nEncryptType;
    DCSServiceType nServiceType;
    // get service type fleet / retail
    //GetAutolinkConfigProperty(eAutoLinkConfig_EncryptType,(void*)&nEncryptType);
    //GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);
	nServiceType = GetServiceType();
	nEncryptType = GetEncryptType();

    if( nServiceType < DCS_Retail || nServiceType >= DCS_MAX)
    {
        Trace("Not defined service type\r\n");
        SetServiceType(DCS_Retail);
    }

    if( nEncryptType < DCS_ENC_AES || nEncryptType >= DCS_ENC_MAX )
    {
        Trace("Not defined encrypt type\r\n");
        SetEncryptType(DCS_ENC_KMS);
    }

	if( g_uiSetDoorSignal != 0x6A8F )
	{
		memset((char *)&g_stLockState, 0x00, sizeof(g_stLockState));
		memset((char *)&g_ullIndicator, 0x00, sizeof(g_ullIndicator));
		memset((char *)&g_ucTCUSlopeAngleArrIndex, 0x00, sizeof(g_ucTCUSlopeAngleArrIndex));
		memset((char *)&g_nTCUSlopeAngleArr, 0x00, sizeof(g_nTCUSlopeAngleArr));
		g_uiSetDoorSignal = 0x6A8F;
	}
}

/*****************************END OF FILE****/

