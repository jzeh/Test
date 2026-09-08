/*------------------------------------------------------------------
 * app_cli.c -- mock test file
 *
 * Copyright (c) 2009 B. Berry
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "cli.h"
#include "GIT_Util.h"
#include "GIT_OemInterface.h"
#include "Modem_Manager.h"
#include "Autolink_Manager.h"
#include "AutolinkConfiguration.h"
#include "GIT_VCI.h"
#include "GIT_OemInterface.h"
#include "modem_comm.h"
#include "FOTA_manager.h"
#include "Power_Manager.h"
#include "Message_Manager.h"
#include "GIT_AesEncrypt.h"
#include "Message_Make.h"
#include "GIT_CanParsingProc.h"
#include "GIT_Gps.h"
#include "OBD_Manager.h"
#include "OBD_Controller.h"
#include "OBD_Controller_Get.h"
#include "HdDebug.h"
#include "MngSystem.h"
#include "MngQueue.h"
#include "MngStorage.h"
#include "AutolinkConfiguration.h"
#include "MngSystemUtil.h"
#include "AutolinkConfig.h"
#include "MngModem.h"
#include "CanFD_Controller.h"
#include "Share_InterFunction.h"
#include "GIT_InterProtocol.h"
#include "SysHalFileSystem.h"
#include "GIT_BluetoothLowEnergy.h"
#include "AutolinkMessage.h"
#include "GIT_base64.h"
#include "HalHandler.h"
#include "SPIDriverMicron.h"
#include "HalPowerDriver.h"
#include "OBD_Controller_Get.h"


#if defined(USE_GIT_FAT_FS)
#include "ff.h"
#endif

#define Trace(...)  GITDebug(DEBUG_MODULES_CLI,__VA_ARGS__)

//#define AUTOLINKDATA_BASE_DIR   "AutoLinkData"

extern FIRMWARE_INFO	g_FirmwareInfo;
extern stAutolinkConfigData m_stAutolinkConfigData;
extern stServerUrl g_stServerUrl;

extern boolean_t DelInternalFlashofDB(int eCurrFotaFileType);

extern void Display_ReportDriving();
extern void Decide_EngingButton_State(void);

typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;

/*
 * directory record
 */
static cli_record_t cli_app_dir;
static cli_record_t cli_modem_dir;
static cli_record_t cli_operation_dir;

/*
 * allocate command records
 */
static cli_record_t fs_op_cmd_record;
static cli_record_t msg_op_cmd_record;
static cli_record_t modem_op_cmd_record2;

static cli_record_t ble_op_cmd_record;
static cli_record_t power_op_cmd_record;
static cli_record_t app_op_enable_cmd_record;
static cli_record_t app_op_disable_cmd_record;
static cli_record_t app_op_test_cmd_record;
static cli_record_t app_op_systest_cmd_record;
static cli_record_t app_op_can_cmd_record;
static cli_record_t debug_op_cmd_record;
static cli_record_t obd_op_cmd_record;
static cli_record_t start_op_cmd_record;
static cli_record_t stop_op_cmd_record;
static cli_record_t cfd_op_cmd_record;
static cli_record_t testmode_op_cmd_record;




static cli_record_t modem_op_run_cmd_record;
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
#include "usbd_cdc_vcp.h"
static cli_record_t usb_op_cmd_record;
uint8_t gbEnableUSBCDC = false;
#endif

uint8_t g_bEnableModemDirectCommunication = false;
uint8_t g_bEnableBLEDirectCommunication = false;
uint8_t g_bEnableGPSDirectCommunication = false;
uint8_t g_bEnableSELFTESTDirectCommunication = false;
//uint8_t g_bEnableSELFTESTDirectCommunication = true;
uint8_t g_bEnableBLEViewRes = false;
uint8_t g_bEnableGPSViewRes = false;
bool gb_EnableDispAlarmCount;
uint8_t g_bDisconnectTestFlag = false;

extern bool gbEnableShowRSSI;
extern OBD_CONTROLLER_DATA g_OBDControllerData;
extern stURLInfo g_stTempURLInfo;

// [TEST] modem reset / module sleep 를 테스트용으로 disable 하는 플래그
extern bool g_bTestDisableModemReset;   // Modem_Manager.c
extern boolean_t g_bTestDisableSleep;   // MngSystem.c

#if defined(FEATURE_CAN1_USE)
extern stHalCANTX_STRUCT   g_CAN1_TxBuffCtrl;
extern stHalCANRX_STRUCT   g_CAN1_RxBuffCtrl;
#endif

#if defined(FEATURE_CAN2_USE)
extern stHalCANTX_STRUCT   g_CAN2_TxBuffCtrl;
extern stHalCANRX_STRUCT   g_CAN2_RxBuffCtrl;
#endif

void StandbyRTCBKPSRAMMode_Measure(void);


extern eGitFresult MSG_DelAllErrorFile(void);
extern void SEED_TestFunc(void);
extern void SetupForInterruptforImpulse(bool bWomActive, bool bGyroActive, unsigned char ucValue);
extern void ModemReportHandler(int32_t nEventType, stMsgMdm* pstMsgMdm);
extern FRESULT WriteFile(int32_t nMode, int8_t *pcData, uint16_t nLen);
FRESULT ReadFile(int iMode, char *pData, uint16_t nLen);
extern FRESULT ReadFile2(int32_t nMode, int8_t *pcData, uint16_t nLen);
extern FRESULT DeleteFile(int iMode, char *pData, uint16_t nLen);
extern void EnterStandbyAndRTCAlarmWakeup(uint32_t wSec);
extern void SendGeoFenceAlramReport(int32_t nEvent, int32_t nResult, stGeofenceUnit* pstCurGeofence, stGeofenceUnit* pstSetGeofence, eGeoFenceType eGeofenceType,uint32_t unGeofenceID);
extern bool ReadInterruptStatus();
extern FRESULT Rename();
void ShowDir(char* carrPath);
extern FRESULT DelAllFile(void);
#if defined(REASON_8BYTE)
extern int ResponseSmartKeyResult(int iActuatorType , int iResult, uint64_t iReason);
#else
extern int ResponseSmartKeyResult(int iActuatorType , int iResult, int iReason);
#endif

extern void CheckInterrtupStatus();
extern void SetupInterruptLatch(boolean_t bPowerOff);
extern void GPS_DisableCommunication(void);
extern void SELFTESTUart_Init(unsigned int);
extern void InitSELFTESTDMA(void);
extern void DisableSELFTESTDMA(void);

extern FRESULT FindAutoLinkInfo(int iMode,char* pcPath, char* pcFileName);
extern FRESULT DeleteAutolinkFile(char *pcarrFullPath);
extern FRESULT ReadAutolinkAsset(int nMode, char* pcarrPath,char* pcarrFileName, char *pData, uint16_t nLen);
extern FRESULT DeleteAutolinkFolder(char *pcarrFolderPath);
extern FRESULT WriteAutoLinkData(char *pData, uint16_t nLen, char *pPath);
extern void HandlerNewVin(char* carrNewVin);

extern bool FOTA_MakeRequestVehicleInfo(void);
extern void FOTA_GetVehicleInfo(void);

extern void WriteConfig(boolean_t bExportFuturekey, boolean_t bDisp);
extern void ReadConfig(boolean_t bDisp);
extern void TestFormat();

extern void DisplayAutolinkConfigData();

extern void ClearRecoveryFile();
extern void SetNetworkPostPoneFlag(boolean_t bPostPonded);
extern void Send2MngSys2(stMsgSys* pstMsgSys);
extern uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
extern uint32_t GetUTCTime();
extern void SetVINCode(U8 * inputVIN);
extern void SetWDTReset(uint16_t msDelay);
extern stPASSTHRU_MSG		g_stWritePassThruMsg_L;
extern unsigned int		g_uiCanWriteMsgLength;
extern stCanPacket	g_OutLCanPacket;
extern stPASSTHRU_MSG		g_stWritePassThruMsg;
extern stCanPacket	g_OutCanPacket;
extern double g_dSavedGpsLat;
extern double g_dSavedGpsLon;
extern eLockState	g_eLockStatus;

extern void DisplayBackupRamConfigData();
extern void GetVINCode(uint8_t * pcarrVin);
extern void SetEmergencyStatus(boolean_t bOccurred);
extern void ClearEmergencyStatus();
extern void GetEmergencyStatus();
extern void PowerOnGemaltoModem();
extern void CheckExpiretAGPSData();
extern int MakeGpsList2String2(uint8_t ucCount, double* pdlLatitudeList,double* pdlLongitudeList,char* parrTemp);
extern int GetPublickey(char* parrPublicKey,int* pnPublicKeySize);
extern void SetActiveGuardMode(boolean_t bActive);
extern void Git_DeviceReset(void *pInterPtcl, unsigned int eInCommType);
extern bool MPU6515_CheckInterrupt();
extern float Get_Battery(void);
extern void ShowSmartkeyAction(stCarReport report);
extern unsigned char g_ucRandomKey[32];
extern void SendFotaAlramReport(int32_t nEvent, int32_t nResult);
//extern stActuatorReadyData		g_stActuatorReadyData[ACTUATOR_MAX_READY_CNT];
//extern unsigned char g_ucTotReadyCnt;
extern U16	g_usCurrentCnt;
extern stSlaveData	*g_pstCurrDataBase;
extern bool g_bIndicatorDBFlag;
extern void WaitCriticalErrorTimerCallBack();
extern boolean_t FindLastAddressofDB(int eCurrFotaFileType, char* dbName, int32_t nVer, uint16_t* pusCheckSum);
extern void MeasureGyroAngle();
extern void SendObdConfig2System(uint32_t unOdometer, double dlLatitude, double dlLongitude, uint8_t ucDoorLockStatus, uint8_t ucDoorOpenStatus, uint8_t ucHeadLight, float fReaminedFuel);
extern double Get_GPS_Lat_Origin();
extern double Get_GPS_Lon_Origin();
extern void Set_GPS_Lat_Origin(double lat);
extern void Set_GPS_Lon_Origin(double lon);


extern void MD_mDelay (const uint32_t msec);

extern void __iar_dlmalloc_stats(void);

static void fs_op_cmd (uint32_t argc, char *argv[])
{
//	Trace("%s \r\n", __FUNCTION__);

	char m_carrLatestFolderPath[256];
	char m_carrLatestFilePath[256];

	if(strcmp(argv[1], "dir") == 0) {
		Trace("\r\n");
	}
	else if(strcmp(argv[1], "type") == 0) {
		if(argc == 3) {
			if(strcmp(argv[2], "currerrmsg") == 0) {
			}
		}
	}
	if(strcmp(argv[2], "0") == 0)
	{
		char carrFullPath[64];

		sprintf(carrFullPath,"/AutoLinkData/%s",argv[3]);
		ShowDir(carrFullPath);
	}
#ifdef USE_FILE_BROKEN_TEST
	if(strcmp(argv[2], "1") == 0)
	{
		extern boolean_t m_bMessageBrokenTest;
		m_bMessageBrokenTest = true;
	}
#endif //#ifdef USE_FILE_BROKEN_TEST

	if(strcmp(argv[2], "8") == 0)
	{
		char carrFolderName[64];
		char carrFileName[64];
		char carrPath[256];
		char buffer[256];
		char carrTmpPath[256];
		memset(carrFolderName,0,sizeof(carrFolderName));
		sprintf(carrPath,"%s","AutoLinkData");
		FindAutoLinkInfo(FILE_MODE_FIND_FOLDER,carrPath,carrFolderName);

		printf("dir name : %s\n",carrFolderName);

		WriteAutoLinkData("abcdefg",7,carrTmpPath);

		sprintf(carrPath,"%s/%s","AutoLinkData",carrFolderName);
		memset(carrFileName,0,sizeof(carrFileName));
		FindAutoLinkInfo(FILE_MODE_FIND_FILE,carrPath,carrFileName);

		printf("file name : %s\r\n",carrFileName);

		ReadAutolinkAsset(FILE_MODE_READ_AND_DELETE, carrFolderName,carrFileName,buffer,7);
		printf("read data : %s\r\n",buffer);
	}
	if( strcmp(argv[2],"10") == 0)
	{
		if ( git_f_mkfs(FAT_VOLUME_DRV, 0, 4096) == GIT_FR_OK )
		{
			Trace("FS: format success\r\n");

			if ( git_f_chdir(DIR_ROOT) == GIT_FR_OK )
			{
				Trace("FS: Create default foler\r\n");
				git_f_mkdir(DIR_LOG);
				git_f_mkdir(DIR_FIRMWARE);
				git_f_mkdir(DIR_INFORMATION);
				git_f_mkdir(AUTOLINKDATA_BASE_DIR);
				git_directory_list();
			}
			else
			{
				Trace("FS: GIT_FR_NOK\r\n");
			}
		}
		else
		{
			Trace("FS: format fail\r\n");
		}
	}
	if( strcmp(argv[2],"11") == 0)
	{
		// delete folder
		DeleteAutolinkFolder(m_carrLatestFolderPath);
	}
	if( strcmp(argv[2],"12") == 0)
	{
		char carrFolderName[64];
		char carrFileName[64];
		char carrPath[256];
		char buffer[256];

		memset(carrFolderName,0,sizeof(carrFolderName));
		sprintf(carrPath,"%s","AutoLinkData");
		FindAutoLinkInfo(FILE_MODE_FIND_FOLDER,carrPath,carrFolderName);

		printf("dir name : %s\r\n",carrFolderName);

		sprintf(carrPath,"%s/%s","AutoLinkData",carrFolderName);
		memset(carrFileName,0,sizeof(carrFileName));
		FindAutoLinkInfo(FILE_MODE_FIND_FILE,carrPath,carrFileName);

		printf("file name : %s\r\n",carrFileName);

		sprintf(m_carrLatestFolderPath,"%s\x00",carrFolderName);
		sprintf(m_carrLatestFilePath,"/%s/%s/%s\x00","AutoLinkData",carrFolderName,carrFileName);

		if( ReadAutolinkAsset(FILE_MODE_READ_AND_DELETE,
			carrFolderName,
			carrFileName,
			(char*)buffer,
			7) == 0 )//GIT_FR_OK )
		{
			buffer[7]=0;
			Trace("Read data : %s\r\n",buffer);
		}
		else
		{
			Trace("ReadAutolinkAsset: error\r\n");
		}
	}
	if( strcmp(argv[2],"13") == 0)
	{
		DeleteAutolinkFile(m_carrLatestFilePath);
	}
	if( strcmp(argv[2],"14") == 0)
	{
		stFileSystemDescript fpFile;
		char path[256];

		sprintf(path,"/%s/%04d%02d\x00","AutoLinkData",
					18,1);
		git_f_mkdir(path);
		sprintf(path,"/%s/%04d%02d\x00","AutoLinkData",
					17,1);
		git_f_mkdir(path);

		sprintf(path,"/%s/%04d%02d\x00","AutoLinkData",
					17,5);
		git_f_mkdir(path);

		ShowDir("/AutoLinkData");

		char carrFolderName[64];
		char carrFileName[64];
		char carrPath[256];
        //eGitFresult ret = GIT_FR_OK;

		memset(carrFolderName,0,sizeof(carrFolderName));
		sprintf(carrPath,"/%s","AutoLinkData");
		FindAutoLinkInfo(FILE_MODE_FIND_FOLDER,carrPath,carrFolderName);

		sprintf(path,"/%s/%04d%02d/%02d%02d\x00","AutoLinkData", 17,1,10,2);
		if((git_f_open(&fpFile, (const char *)path, GIT_FA_CREATE_ALWAYS | GIT_FA_WRITE)) == GIT_FR_OK)
		{
			git_f_close(&fpFile);
		}

		sprintf(path,"/%s/%04d%02d/%02d%02d\x00","AutoLinkData", 17,1,10,5);
		if(( git_f_open(&fpFile, (const char *)path, GIT_FA_CREATE_ALWAYS | GIT_FA_WRITE)) == GIT_FR_OK)
		{
			git_f_close(&fpFile);
		}

		sprintf(path,"/%s/%04d%02d/%02d%02d\x00","AutoLinkData", 17,1,10,8);
		if(( git_f_open(&fpFile, (const char *)path, GIT_FA_CREATE_ALWAYS | GIT_FA_WRITE)) == GIT_FR_OK)
		{
			git_f_close(&fpFile);
		}

		sprintf(path,"/%s/%04d%02d/%02d%02d\x00","AutoLinkData", 17,1,10,1);
		if(( git_f_open(&fpFile, (const char *)path, GIT_FA_CREATE_ALWAYS | GIT_FA_WRITE)) == GIT_FR_OK)
		{
			git_f_close(&fpFile);
		}

		sprintf(carrPath,"%s/%s","AutoLinkData",carrFolderName);
		memset(carrFileName,0,sizeof(carrFileName));
		FindAutoLinkInfo(FILE_MODE_FIND_FILE,carrPath,carrFileName);


		sprintf(carrPath,"%s/%s","AutoLinkData",carrFolderName);

		ShowDir((char *)carrPath);

		printf("file name : %s\r\n",carrFileName);

	}
	return;
}

int dukpttest_cli_main( int argc, char *argv[] );
int aestest_main( int argc, char *argv[] );

#if defined (OLD_FOTA)
eFOTA_RESULT_CODE FOTA_ParseReceivedVehicleInfo(void);
#else
extern eFOTA_RESULT_CODE NEWFOTA_ParseReceivedVehicleInfo(void);
#endif

stGeofenceUnit m_stGeofenceUnitTest[] = {
//    {eREMOTE_CON_BOUNDTYPE_IN, 0,  37.519237, 127.076598, true, false,false,false},
//    {eREMOTE_CON_BOUNDTYPE_IN, 0, 37.513367, 127.088861, true, false,false,false},
//    {eREMOTE_CON_BOUNDTYPE_IN, 0, 37.510516, 127.098057, true, false,false,false},
//    {eREMOTE_CON_BOUNDTYPE_IN, 0, 37.507579, 127.109049, false, true,false,false},
//    {eREMOTE_CON_BOUNDTYPE_IN, 0, 37.506570, 127.118033, false, true,false,false},
//    {eREMOTE_CON_BOUNDTYPE_IN, 0, 37.506148, 127.127230, false, true,false,false},
//    {eREMOTE_CON_BOUNDTYPE_IN, 0, 37.502789, 127.138538, false, true,false,false},
//    {eREMOTE_CON_BOUNDTYPE_IN, 0, 37.495737, 127.153544, false, true,false,false},
//    {eREMOTE_CON_BOUNDTYPE_IN, 0, 37.482157, 127.177737, false, true,false,false},
    {eREMOTE_CON_BOUNDTYPE_IN, 0, 37.477472, 127.194334, false, true,false,false},
};
int m_nGetCount=0;

static void app_systest_cmd (uint32_t argc, char *argv[])
{
	if(strcmp(argv[1], "aes") == 0) {
#if defined (OLD_FOTA)
		FOTA_ParseReceivedVehicleInfo();
#else
		NEWFOTA_ParseReceivedVehicleInfo();
#endif
//		aestest_main(0, NULL);
	}
	else if(strcmp(argv[1], "d") == 0) {
		dukpttest_cli_main(0, NULL);
	}
	else if(strcmp(argv[1], "dukpt") == 0) {
		dukpttest_cli_main(0, NULL);
	}
	else if(strcmp(argv[1], "seed256") == 0) {
		// project ������ SEED256�� Ȱ��ȭ �� �Ŀ� ����� ��
//		Trace("SEED 256: reserved\r\n");
//		SEED_TestFunc();
	}
	else if(strcmp(argv[1], "aes256") == 0) {
		uint8_t arrTempBuffer[128];
		uint8_t arrEncryptionText[256];
		uint16_t nLen;

		Trace("Test AES256 - Make server SMS\r\n");
		strcpy((char *)arrTempBuffer, "10autolink1234567890123456789012345612345678901234567890123456");
		ApplyEncryption(arrTempBuffer, arrEncryptionText, &nLen,0);

		ApplyDecryption(arrEncryptionText, nLen);
	}
	else if(strcmp(argv[1], "base64") == 0) {
		mbedtls_base64_self_test(1);
	}
	else if(strcmp(argv[1], "default") == 0) {
		if(strcmp(argv[2], "fwinfo") == 0) {
			Trace("!!! ERASE FW Information !!!\r\n");
			DefaultFWInfo();
		}
	}
	else if(strcmp(argv[1], "save") == 0) {
//		if(strcmp(argv[2], "vehiclecode") == 0) {
//			uint16_t nLen;
//
//			nLen = strlen((char *)g_FirmwareInfo.m_strVehicleCode);
//			if(nLen > MAX_VEHICLECODE_SIZE) {
//				Trace("!!! error: over MAX_VEHICLECODE_SIZE\r\n");
//			}
//
//			strcpy((char *)g_FirmwareInfo.m_strVehicleCode, argv[3]);
//
//			SetFirmwareInfo(&g_FirmwareInfo);
//		}
	}
#ifdef USEBUZZER
	else if(strcmp(argv[1], "buzzer") == 0) {
		Buzzer_Control(eBUZZER_DOMISOL, MSEC(200),MSEC(200), 3);
	}
#endif
	else if(strcmp(argv[1], "latch") == 0) {
		if(strcmp(argv[2], "lowcanrxctl") == 0) {
			if(strcmp(argv[3], "low") == 0) {
				Trace("TEST: GPIO_LOW_CAN_RX_CTL - low\r\n");
				HalGPIOSetVaule(GPIO_LOW_CAN_RX_CTL, eBIT_RESET);
				HalGpioGenerateLatchClock();
			}
			else if(strcmp(argv[3], "high") == 0) {
				Trace("TEST: GPIO_LOW_CAN_RX_CTL - high\r\n");
				HalGPIOSetVaule(GPIO_LOW_CAN_RX_CTL, eBIT_SET);
				HalGpioGenerateLatchClock();
			}
		}
		else if(strcmp(argv[2], "canrxctl") == 0) {
			if(strcmp(argv[3], "low") == 0) {
				Trace("TEST: GPIO_CAN_RX_CTL - low\r\n");
				HalGPIOSetVaule(GPIO_CAN_RX_CTL, eBIT_RESET);
				HalGpioGenerateLatchClock();
			}
			else if(strcmp(argv[3], "high") == 0) {
				Trace("TEST: GPIO_CAN_RX_CTL - high\r\n");
				HalGPIOSetVaule(GPIO_CAN_RX_CTL, eBIT_SET);
				HalGpioGenerateLatchClock();
			}
		}
		else if(strcmp(argv[2], "mowakectl") == 0) {
			if(strcmp(argv[3], "low") == 0) {
				Trace("TEST: GPIO_MO_WAKE_CTL - low\r\n");
				HalGPIOSetVaule(GPIO_MO_WAKE_CTL, eBIT_RESET);
				HalGpioGenerateLatchClock();
			}
			else if(strcmp(argv[3], "high") == 0) {
				Trace("TEST: GPIO_MO_WAKE_CTL - high\r\n");
				HalGPIOSetVaule(GPIO_MO_WAKE_CTL, eBIT_SET);
				HalGpioGenerateLatchClock();
			}
		}
		else if(strcmp(argv[2], "btmonctl") == 0) {
			if(strcmp(argv[3], "low") == 0) {
				Trace("TEST: GPIO_BT_MON_CTL - low\r\n");
				HalGPIOSetVaule(GPIO_BT_MON_CTL, eBIT_RESET);
				HalGpioGenerateLatchClock();
			}
			else if(strcmp(argv[3], "high") == 0) {
				Trace("TEST: GPIO_BT_MON_CTL - high\r\n");
				HalGPIOSetVaule(GPIO_BT_MON_CTL, eBIT_SET);
				HalGpioGenerateLatchClock();
			}
		}
		else if(strcmp(argv[2], "igaccdetctl") == 0) {
			if(strcmp(argv[3], "low") == 0) {
				Trace("TEST: GPIO_IG_ACC_DET_CTL - low\r\n");
				HalGPIOSetVaule(GPIO_IG_ACC_DET_CTL, eBIT_RESET);
				HalGpioGenerateLatchClock();
			}
			else if(strcmp(argv[3], "high") == 0) {
				Trace("TEST: GPIO_IG_ACC_DET_CTL - high\r\n");
				HalGPIOSetVaule(GPIO_IG_ACC_DET_CTL, eBIT_SET);
				HalGpioGenerateLatchClock();
			}
		}
	}
	else if(strcmp(argv[1], "led") == 0) {
		if(strcmp(argv[2], "on") == 0) {
			if(strcmp(argv[3], "gps") == 0) {
				SetLedOnOffCtl(LED_ON, eLED_GPS);
			}
			else if(strcmp(argv[3], "server") == 0) {
				SetLedOnOffCtl(LED_ON, eLED_SERVER);
			}
			else if(strcmp(argv[3], "can") == 0) {
				SetLedOnOffCtl(LED_ON, eLED_CAN);
			}
		}
		else if(strcmp(argv[2], "off") == 0) {
			if(strcmp(argv[3], "gps") == 0) {
				SetLedOnOffCtl(LED_OFF, eLED_GPS);
			}
			else if(strcmp(argv[3], "server") == 0) {
				SetLedOnOffCtl(LED_OFF, eLED_SERVER);
			}
			else if(strcmp(argv[3], "can") == 0) {
				SetLedOnOffCtl(LED_OFF, eLED_CAN);
			}
		}
	}
	else if(strcmp(argv[1], "enable") == 0) {
		if(strcmp(argv[2], "disprtcalarm") == 0) {
			Trace("*display rtc alarm count\r\n");
			gb_EnableDispAlarmCount = true;
		}
		else if(strcmp(argv[2], "etcpow") == 0) {
			// OUTPUT - 3.3V �����ΰ� �뵵(HIGH)
			HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_SET);
		}
		else if(strcmp(argv[2], "mco1") == 0) {
            stHalGPIO_InitTypeDef GPIO_InitStructure;

            Trace("Output HSE clock on MCO1 pin(PA8)\r\n");
            /* Output HSE clock on MCO1 pin(PA8) ****************************************/
            /* Enable the GPIOA peripheral */
            HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOA_GROUP, NULL, 0, HAL_ENABLE);

            /* Configure MCO1 pin(PA8) in alternate function */
            GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
            GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
            GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_AF;
            GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
            GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
            GPIO_InitStructure.GPIO_Pin = GPIO_LOW_CAN_NERR;
            HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

            HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, GPIO_LOW_CAN_NERR, NULL, 0, HAL_GPIO_AF_MCO);


            //		  /* HSE clock selected to output on MCO1 pin(PA8), �ܺ� ũ����Ż ���ļ� ���, 25MHz ���*/
            //		  RCC_MCO1Config(RCC_MCO1Source_HSE, RCC_MCO1Div_1);
            /* PLLCLK clock selected to output on MCO1 pin(PA8)*/
            HalDrvRccIOCtrl(eRCC_IO_MCOX_Clock_config, HAL_RCC_MCO1Source_PLLCLK, NULL, 0, HAL_RCC_MCO1Div_4);
		}
		else if(strcmp(argv[2], "mco2") == 0) {
			stHalGPIO_InitTypeDef GPIO_InitStructure;

			Trace("Output HSE clock on MCO2 pin(PC9)\r\n");
            /* Output HSE clock on MCO2 pin(PC9) ****************************************/
            /* Enable the GPIOC peripheral */
            HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOC_GROUP, NULL, 0, HAL_ENABLE);

            /* Configure MCO1 pin(PC9) in alternate function */
            GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
            GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
            GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_AF;
            GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
            GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
            GPIO_InitStructure.GPIO_Pin = GPIO_LOW_CAN_EN;
            HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

            HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, GPIO_LOW_CAN_NERR, NULL, 0, HAL_GPIO_AF_MCO);

            /* SYSCLK clock selected to output on MCO2 pin(PC9), 180MHz �����϶�, 90MHz ���*/
            HalDrvRccIOCtrl(eRCC_IO_MCOX_Clock_config, HAL_RCC_MCO2Source_SYSCLK, NULL, 0, HAL_RCC_MCO2Div_2);
		}
	}
	else if(strcmp(argv[1], "disable") == 0) {
		if(strcmp(argv[2], "disprtcalarm") == 0) {
			Trace("*no display rtc alarm count\r\n");
			gb_EnableDispAlarmCount = false;
		}
		else if(strcmp(argv[2], "etcpow") == 0) {
			// OUTPUT - 3.3V �����ΰ� �뵵(LOW)
			HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_RESET);
		}
	}
	else if(strcmp(argv[1], "get") == 0) {
		if(strcmp(argv[2], "time") == 0) {
			Trace("\n\n\nCurrent Time & Date!!!\r\n");
            
            stHalRTCTypeDef stHalRtcDateTime;
            APP_TimeShow(&stHalRtcDateTime);
		}
		else if(strcmp(argv[2], "date") == 0) {
			Trace("\n\n\nCurrent Time & Date!!!\r\n");
            stHalRTCTypeDef stHalRtcDateTime;
            APP_TimeShow(&stHalRtcDateTime);
		}
		else if(strcmp(argv[2], "wakeuppin") == 0) {
			GetWakeupPinState();
		}
		else if(strcmp(argv[2], "wakemonpins") == 0) {
			GetWakeupDetectPinsState();
		}
        else if(strcmp(argv[2], "tinfo") == 0) {   //TERMINAL Infomation TEST
			Trace("Get Firmware Info\r\n");

            GetFirmwareInfo(&g_FirmwareInfo);

            printf("VIN: [%-18s]\r\n", g_FirmwareInfo.m_strVIN);
            printf("USIM: [%-24s]\r\n", g_FirmwareInfo.m_strUSIMNum);
            printf("PhoneNo: [%-16s]\r\n", g_FirmwareInfo.m_strPHONENum);
		}
	}
	else if(strcmp(argv[1], "set") == 0) {
/*
		if(strcmp(argv[2], "time1") == 0) {
			Trace("set time(1)...\r\n");
			APP_SetTime();
		}
		else*/
		if(strcmp(argv[2], "debug") == 0) {
			if(strcmp(argv[3], "uart8") == 0) {
				stSystemTestInfo.eDebugUartCh = eDEBUG_UART_CH_UART8;

				SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
				Trace("DBG: new debug port\r\n");
			}
			else if(strcmp(argv[3], "uart7") == 0) {
				stSystemTestInfo.eDebugUartCh = eDEBUG_UART_CH_UART7;

				SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
				Trace("DBG: new debug port\r\n");
			}
		}

		else if(strcmp(argv[2], "tinfo") == 0) {  //TERMINAL Infomation TEST
			Trace("set Firmware Info\r\n");

            sprintf((char *)g_FirmwareInfo.m_strVIN, "ABCDEFG0123456789\x00");
            sprintf((char *)g_FirmwareInfo.m_strUSIMNum, "%s\x00", BkSram_ModemInfo.aCCID);
            sprintf((char *)g_FirmwareInfo.m_strPHONENum, "%s\x00", BkSram_ModemInfo.aPhoneNo);

            SetFirmwareInfo(&g_FirmwareInfo);
		}
/*
		else if(strcmp(argv[2], "time2") == 0) {
			Trace("set time(2)...\r\n");
			APP_SetTime2();
		}
*/
		else if(strcmp(argv[2], "alarm") == 0) {

			if(strcmp(argv[3], "interrupt") == 0) {
				uint16_t sec;
				uint32_t wTime;
				uint32_t wTemp;

				Trace("\r\n\r\n\r\nCurrent Time & Date!!!\r\n");
                stHalRTCTypeDef stHalRtcDateTime;
                APP_TimeShow(&stHalRtcDateTime);

				wTime = stHalRtcDateTime.RtcTime.RTC_Hours * 60 * 60;
				wTime += (stHalRtcDateTime.RtcTime.RTC_Minutes * 60);
				wTime += stHalRtcDateTime.RtcTime.RTC_Seconds;
				Trace("wTime: %d(sec)\r\n", wTime);

				sec = atoi(argv[3]);
				Trace("period time: %d(sec)\r\n", sec);

				wTime += sec;

//				Trace("alarm time: %d(sec)\r\n", wTime);

				wTemp = wTime / 3600;
				stHalRtcDateTime.RtcTime.RTC_Hours = wTemp % 24;
				wTime = wTime % 3600;
				stHalRtcDateTime.RtcTime.RTC_Minutes = wTime / 60;
				stHalRtcDateTime.RtcTime.RTC_Seconds = wTime % 60;

				Trace("alarm Time: %02d:%02d:%02d\r\n", stHalRtcDateTime.RtcTime.RTC_Hours, stHalRtcDateTime.RtcTime.RTC_Minutes, stHalRtcDateTime.RtcTime.RTC_Seconds);
				HalDrvRtc_SetAlarmTime(&stHalRtcDateTime);
			}
		}
		else if(strcmp(argv[2], "default") == 0) {
			Trace("\r\nset default system test setting info\r\n");

			LoadSettingInfoFile(1);
		}
		else if(strcmp(argv[2], "wakectl") == 0) {
			if(strcmp(argv[3], "igacc") == 0) {
				if(strcmp(argv[4], "enable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC, eWAKE_CTL_PIN_ENABLE);
				}
				else if(strcmp(argv[4], "disable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC, eWAKE_CTL_PIN_DISABLE);
				}
			}
			else if(strcmp(argv[3], "modem") == 0) {
				if(strcmp(argv[4], "enable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE, eWAKE_CTL_PIN_ENABLE);
				}
				else if(strcmp(argv[4], "disable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE, eWAKE_CTL_PIN_DISABLE);
				}
			}
			else if(strcmp(argv[3], "lowcanrx") == 0) {
				if(strcmp(argv[4], "enable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX, eWAKE_CTL_PIN_ENABLE);
				}
				else if(strcmp(argv[4], "disable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX, eWAKE_CTL_PIN_DISABLE);
				}
			}
			else if(strcmp(argv[3], "btmon") == 0) {
				if(strcmp(argv[4], "enable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON, eWAKE_CTL_PIN_ENABLE);
				}
				else if(strcmp(argv[4], "disable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON, eWAKE_CTL_PIN_DISABLE);
				}
			}
			else if(strcmp(argv[3], "canrx") == 0) {
				if(strcmp(argv[4], "enable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX, eWAKE_CTL_PIN_ENABLE);
				}
				else if(strcmp(argv[4], "disable") == 0) {
					SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX, eWAKE_CTL_PIN_DISABLE);
				}
			}
		}
#if defined(PROTOCOL18)
		else if(strcmp(argv[2], "charging") == 0){
			if(strcmp(argv[3], "state") == 0){
				if(strcmp(argv[4], "0") == 0){
					Send_ChargingState_From_CAN(false);
				}
				else if(strcmp(argv[4], "1") == 0){
					Send_ChargingState_From_CAN(true);
				}
			}
		}
		else if(strcmp(argv[2], "extra") == 0){
			if(strcmp(argv[3], "chargetime") == 0){
				Send_ExtraChargeTime_From_CAN(50);
			}
		}
#endif
	}
	else if(strcmp(argv[1], "sflash") == 0) {
		if(strcmp(argv[2], "eraseall") == 0) {
			Trace("Serial Flash Bulk Erase\r\n");
			sFLASH_EraseBulk();
			Trace("Erase Done\r\n");
		}
		else if(strcmp(argv[2], "eraseapp") == 0) {
			Trace("Serial Flash application section Erase\r\n");
			Trace("FLASH: erase ADDR_APPLICATION_SAVE1 ~ ADDR_CONTROL_DB_SAVE - 1\r\n");
			Trace("FLASH: erase 0x08110000 ~ 0x081BFFFF\r\n");
            HalDrvFlashErase(ADDR_APPLICATION_SAVE1, ADDR_CONTROL_DB_SAVE - 1, NULL, 0, 0);

			Trace("Erase Done\r\n");
		}
	}
	else if(strcmp(argv[1], "reset") == 0) {
		uint16_t msDelay;

		Trace("\r\n");
		Trace("Reset System\r\n");
        
        HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);
        HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);

		if(argc == 2) {
			Trace("set warchdog reset\r\n");
			SystemForcelyReset();
		}
		else {
			msDelay = atoi(argv[2]);
			Trace("set warchdog reset: %d\r\n", msDelay);

			SetWDTReset(msDelay);
		}

		while(1);
	}
	else if(strcmp(argv[1], "sbreset") == 0) {
		EnterStandbyAndRTCAlarmWakeup(60);
	}
	else if(strcmp(argv[1], "rtcwakeup") == 0) {
		uint16_t nTime;
		float fTime;

		if(argc == 3) {
			nTime = atoi(argv[2]);
		}
		else {
			nTime = 0xa000 - 1;
		}

  /* RTC Wakeup Interrupt Generation: Clock Source: RTCCLK_Div16, Wakeup Time Base: ~20s
     RTC Clock Source LSE 32.768KHz or LSI ~32KHz

     Wakeup Time Base = (16 / (LSE or LSI)) * WakeUpCounter
  */

		fTime = 16.0 / 32768.0;
		fTime = fTime * (float)nTime;

		Trace("RTC Count wakeup...(%f)sec\r\n", fTime);

//		Rtc_Wakeup_Init(nTime);
//      HalDrvRtcIOCtrl(eRtc_IO_WakeupEnable, 0, NULL, 0, HAL_ENABLE);

		StandbyRTCBKPSRAMMode_Measure();
	}
	else if(strcmp(argv[1], "stop") == 0) {

#if 0
    exint_init_type exint_init_struct;

    /* config the exint line of the ertc alarm */
    exint_init_struct.line_select   = EXINT_LINE_17;
    exint_init_struct.line_enable   = TRUE;
    exint_init_struct.line_mode     = EXINT_LINE_INTERRUPUT;
    exint_init_struct.line_polarity = EXINT_TRIGGER_RISING_EDGE;
    exint_init(&exint_init_struct);

    /* set the alarm 05h:20min:10s */
    ertc_alarm_mask_set(ERTC_ALA, ERTC_ALARM_MASK_DATE_WEEK | ERTC_ALARM_MASK_HOUR | ERTC_ALARM_MASK_MIN);
    ertc_alarm_week_date_select(ERTC_ALA, ERTC_SLECT_DATE);
    ertc_alarm_set(ERTC_ALA, 31, 6, 20, 5, ERTC_AM);

    /* enable the ertc interrupt */
    nvic_irq_enable(ERTCAlarm_IRQn, 0, 0);

    /* enable ertc alarm a interrupt */
    ertc_interrupt_enable(ERTC_ALA_INT, TRUE);

    /* enable the alarm */
    ertc_alarm_enable(ERTC_ALA, TRUE);

    ertc_alarm_value_set(30);

    /* disable the alarm */
    ertc_alarm_enable(ERTC_ALA, TRUE);

    /* Enter Stop Mode */
    HalDrvPwr_WkupPin_Config(HAL_WKUPPIN_INT);
    HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStopMode, 0, NULL, 0, 0);

     /* congfig the system clock */
     HalDrvPower_SYSCLKConfig_STOP();

#else
		uint32_t wTime;
		uint32_t wTemp;
		uint16_t sec;
        
		sec = atoi(argv[2]);
        sec = (sec == 0) ? 30:sec;
		Trace("\n\n\nCurrent Time & Date!!!\r\n");
		
        stHalRTCTypeDef stHalRtcDateTime;
        APP_TimeShow(&stHalRtcDateTime);
		

		wTime = stHalRtcDateTime.RtcTime.RTC_Hours * 60 * 60;
		wTime += (stHalRtcDateTime.RtcTime.RTC_Minutes * 60);
		wTime += stHalRtcDateTime.RtcTime.RTC_Seconds;
		Trace("wTime: %d(sec)\r\n", wTime);

		Trace("period time: %d(sec)\r\n", sec);

		wTime += sec;

		wTemp = wTime / 3600;
		stHalRtcDateTime.RtcTime.RTC_Hours = wTemp % 24;
		wTime = wTime % 3600;
		stHalRtcDateTime.RtcTime.RTC_Minutes = wTime / 60;
		stHalRtcDateTime.RtcTime.RTC_Seconds = wTime % 60;

		Trace("alarm Time: %02d:%02d:%02d\r\n", stHalRtcDateTime.RtcTime.RTC_Hours, stHalRtcDateTime.RtcTime.RTC_Minutes, stHalRtcDateTime.RtcTime.RTC_Seconds);
		HalDrvRtc_SetAlarmTime(&stHalRtcDateTime);

		Trace("Enter Stop Mode\r\n\r\n");

		Trace("...\r\n\r\n\r\n");

        /* Enter Stop Mode */
        HalDrvPwr_WkupPin_Config(HAL_WKUPPIN_INT);
        HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStopMode, 0, NULL, 0, 0);
    
        /* Configures system clock after wake-up from STOP: enable HSE, PLL and select
        PLL as system clock source (HSE and PLL are disabled in STOP mode) */
        HalDrvPower_SYSCLKConfig_STOP();
        Trace("Exit Stop Mode\r\n");

        Trace("Enable the RTC alarmA\r\n");
        //   RTC_AlarmConfig();				// 1�� alarm interrupt�� �ʱ�ȭ �Ѵ�.
#endif
	}
	else if(strcmp(argv[1], "selftest") == 0) {
		// UART8 : DMA
		// UART7 : ST Link
		GITDebugPrintf("Go Selftest mode!! \r\n");

		ModemManagerData.eState = eMODEM_NONE;

		SetOBDState(eOBD_Selftest);

		GPS_DisableCommunication();

		GITDebugPrintf("SELFTEST: Init ()\r\n");
		SELFTESTUart_Init(115200);

		g_bYUJINSelftestFlag = true;

	}
	else if(strcmp(argv[1], "hypertec") == 0)
	{
		GITDebugPrintf("Go Hypertec mode!! \r\n");
		// UART8 (DMA) -> DEBUG (STLink)

#if defined(FEATURE_USE_UART_RX_DMA)
		printf("SELFTEST: Disble \r\n");
		DisableSELFTESTDMA();
#endif
		stSystemTestInfo.eDebugUartCh = eDEBUG_UART_CH_UART8;
		SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
		GITDebugPrintf("DBG: new debug port\r\n");

		LoadSettingInfoFile(0);
        SELFTESTUart_Init(BAUDRATE_115200);

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
		InitUSBDeviceClassType();
#endif
		GITDebugPrintf("END\r\n");

		ModemManagerData.eState = eMODEM_NONE;
		SetOBDState(eOBD_Selftest);
		g_bHYPERTECSelftestFlag = true;
	}
	else {
		Trace("known command!!!\r\n");
	}
	if( strcmp(argv[1],"geo") == 0 )
    {
        if( strcmp(argv[2],"circle") == 0 )
        {
            stGeofenceUnit stCurrentUnit;
            stGeofenceUnit stSettingUnit;

            stCurrentUnit.GpsLatitude = m_stGeofenceUnitTest[m_nGetCount].GpsLatitude;
            stCurrentUnit.GpsLongitude = m_stGeofenceUnitTest[m_nGetCount++].GpsLongitude;

            CheckGeoFence(stCurrentUnit,&stSettingUnit,false,true);

            int cnt = sizeof(m_stGeofenceUnitTest)/sizeof(stGeofenceUnit);
            m_nGetCount %= cnt;
        }
        if( strcmp(argv[2],"polygon") == 0 )
        {
            stGeofenceUnit stCurrentUnit;
            stGeofenceUnit stSettingUnit;

            stCurrentUnit.GpsLatitude = m_stGeofenceUnitTest[m_nGetCount].GpsLatitude;
            stCurrentUnit.GpsLongitude = m_stGeofenceUnitTest[m_nGetCount++].GpsLongitude;

            CheckPolygonGeoFence(stCurrentUnit,&stSettingUnit);

            int cnt = sizeof(m_stGeofenceUnitTest)/sizeof(stGeofenceUnit);
            m_nGetCount %= cnt;
        }
    }
	return;
}

static void app_can_cmd (uint32_t argc, char *argv[])
{
	uint32_t uiStartMask[HAL_CAN_MAX_MASK_CNT];
	uint32_t uiEndMask[HAL_CAN_MAX_MASK_CNT];
	int32_t nLoopCnt = 0; //mod.kks 21.11.05 todo remove not use value
	uint32_t uiRecvLen = 0; //mod.kks 21.11.05 todo remove not use value
	stCanPacket OutCanPacket, InCanPacket; //mod.kks 21.11.05 todo remove not use value
	unsigned char ucData[10]; //mod.kks 21.11.05 todo remove not use value
	uint32_t CANCH = 0;

    
    if(strcmp(argv[1], "send") == 0) {
      stHalCanTxMsg TxMessage;
      uint8_t n;

		//Oem_CAN_Initial_CH1(Highcan1, eCAN_500KBPS); //init
		Oem_CAN_Initial_CH1(Highcan1, eCAN_250KBPS);
		CANCH = CAN_CHANNEL_1;
		uiStartMask[0] = 0x00;
		uiEndMask[0] = 0x07FF;
		Oem_CAN_Channel_Masket_Set(CANCH, STANDARD_CAN,1, uiStartMask, uiEndMask);
	
        Trace("CAN1: data send(0x321)\r\n");
	    /* Transmit Structure preparation */
	      TxMessage.StdId = 0x321;
	    //TxMessage.ExtId = 0x01;
	      TxMessage.RTR = HAL_CAN_RTR_DATA;
	      TxMessage.IDE = HAL_CAN_ID_STD;
	      TxMessage.DLC = 2;
	      TxMessage.Data[0] = 0x12;
	      TxMessage.Data[1] = 0x34;

		while ( 1 )
		{
		//      n = HalDrvCanWrite((int)HAL_CAN1, 0, (char*)&TxMessage, sizeof(stHalCanTxMsg), 0);
				n = OemWriteCanBuff((unsigned char*)&TxMessage, 0, NULL, 1);

		        Trace("transmit: %d\r\n", n);
				Oem_GIT_mDelay(5000);
		}
    }

	if(strcmp(argv[1], "enable") == 0) {
		if(strcmp(argv[2], "h1") == 0) {
			Trace("CAN: Enable HIGH CAN1\r\n");

			Oem_CAN_Initial_CH1(Highcan1, eCAN_500KBPS); //init
			CANCH = CAN_CHANNEL_1;
		}
		else if(strcmp(argv[2], "h2") == 0) {
			Trace("CAN: Enable HIGH CAN2\r\n");

			Oem_CAN_Initial_CH1(Highcan2, eCAN_500KBPS); //init
			CANCH = CAN_CHANNEL_1;

			uiStartMask[0] = 0x0710;
			uiEndMask[0]  = 0x0710;
			uiStartMask[1] = 0x0711;
			uiEndMask[1]  = 0x0711;
			uiStartMask[2] = 0x0712;
			uiEndMask[2]  = 0x0712;

			Oem_CAN_Channel_Masket_Set(CANCH, STANDARD_CAN,3, uiStartMask, uiEndMask);
		}
		else if(strcmp(argv[2], "h3") == 0) {
			Trace("CAN: Enable HIGH CAN3\r\n");

			Oem_CAN_Initial_CH2(Highcan3, eCAN_500KBPS); //init
			CANCH = CAN_CHANNEL_2;


			uiStartMask[0] = 0x0000;
			uiEndMask[0]  = 0x00FF;
			uiStartMask[1] = 0x0100;
			uiEndMask[1]  = 0x01FF;
			uiStartMask[2] = 0x0200;
			uiEndMask[2]  = 0x02FF;
			uiStartMask[3] = 0x0300;
			uiEndMask[3]  = 0x03FF;
			uiStartMask[4] = 0x0400;
			uiEndMask[4]  = 0x04FF;
			uiStartMask[5] = 0x0500;
			uiEndMask[5]  = 0x05FF;
			uiStartMask[6] = 0x0600;
			uiEndMask[6]  = 0x06FF;
			uiStartMask[7] = 0x0700;
			uiEndMask[7]  = 0x07FF;

			Oem_CAN_Channel_Masket_Set(CANCH, STANDARD_CAN, 8, uiStartMask, uiEndMask);

		}
		else if(strcmp(argv[2], "l1") == 0) {
			Trace("CAN: Enable LOW CAN\r\n");

			Oem_CAN_Initial_CH2(Lowcan1, eCAN_100KBPS); //init
			CANCH = CAN_CHANNEL_2;
		}



	}
	if(strcmp(argv[1], "loopback") == 0) {
        CANCH = CAN_CHANNEL_1;
		uiStartMask[0] = 0x0700;
		uiEndMask[0]  = 0x07FF;

		OutCanPacket.stNormalPacket.ucSOF 		= CAN_FRAME_SOF;
		OutCanPacket.stNormalPacket.us11BitID	= 0x07E1;
		OutCanPacket.stNormalPacket.ucRTR		= 0;
		OutCanPacket.stNormalPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
		OutCanPacket.stNormalPacket.ucReserved	= 0;
		OutCanPacket.stNormalPacket.ucDLC		= 8;
		OutCanPacket.stNormalPacket.usCRC		= 0;
		OutCanPacket.stNormalPacket.ucCRCDelimiter = 1;
		OutCanPacket.stNormalPacket.ucACK		= 1;
		OutCanPacket.stNormalPacket.ucACKDelimiter = 1;
		OutCanPacket.stNormalPacket.ucEOF		= CAN_FRAME_EOF;

		ucData[0] = 0x01;
		ucData[1] = 0x02;
		ucData[2] = 0x03;
		ucData[3] = 0x04;
		ucData[4] = 0x05;
		ucData[5] = 0x06;
		ucData[6] = 0x07;
		ucData[7] = 0x08;
		memcpy(&OutCanPacket.stNormalPacket.arrDataFields[0],&ucData[0],8);

		Oem_CAN_Channel_Masket_Set(CANCH, STANDARD_CAN, 1, uiStartMask, uiEndMask);
		if ( FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, CANCH) > 0 )
		{
			nLoopCnt = 0;
			uiRecvLen = 0;
			do {
                memset(&InCanPacket, 0x00, sizeof(InCanPacket));
				uiRecvLen = FineReadCanBuff((unsigned char*)&InCanPacket, sizeof(InCanPacket), NULL, CANCH);

				if ( nLoopCnt++ > 3 )
				{
					GITDebugPrintf("Receive FAil ~~~~~~~~~~~~~~~~~~\r\n");
					break;
				}
				else
				{
					if( uiRecvLen > 0 )
					{
						GITDebugPrintf("Receive success ~~~~~~~~~~~~~~~~~~\r\n");
                        GITDebugPrintf("[0x%04X] %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                                       InCanPacket.stNormalPacket.us11BitID,
                                       InCanPacket.stNormalPacket.arrDataFields[0], InCanPacket.stNormalPacket.arrDataFields[1],
                                       InCanPacket.stNormalPacket.arrDataFields[2], InCanPacket.stNormalPacket.arrDataFields[3],
                                       InCanPacket.stNormalPacket.arrDataFields[4], InCanPacket.stNormalPacket.arrDataFields[5],
                                       InCanPacket.stNormalPacket.arrDataFields[6], InCanPacket.stNormalPacket.arrDataFields[7]);
                                      

					}
					else
						Oem_GIT_mDelay(30);
				}
			}while(uiRecvLen == 0);
		}
	}

	else if(strcmp(argv[1], "disable") == 0) {
		if(strcmp(argv[2], "CAN1") == 0) {
			Oem_CAN1_STANDBY_ACTIVE();
		}
		else if(strcmp(argv[2], "CAN2") == 0) {
			Oem_CAN2_STANDBY_ACTIVE();
		}
	}
	else if(strcmp(argv[1], "can1") == 0) {
		if(strcmp(argv[2], "enable") == 0) {
			Trace("CAN1: normal(enable)\r\n");
			Oem_CAN1_STANDBY_INACTIVE();
		}
		else if(strcmp(argv[2], "disable") == 0) {
			Trace("CAN1: standby\r\n");
			Oem_CAN1_STANDBY_ACTIVE();
		}
		else if(strcmp(argv[2], "set") == 0) {
			uint32_t CanID1 = 0x00FF;
			uint32_t CanID2 = 0x0F00;

			CanID1 = 0x00FF;
			CanID2 = 0x0F00;

			Trace("CAN1: set\r\n");

			Oem_CAN_Initial_CH1(Highcan1, eCAN_500KBPS); //init
			Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN, 1, &CanID1, &CanID2);
		}
	}
	else if(strcmp(argv[1], "can2") == 0) {
		if(strcmp(argv[2], "enable") == 0) {
			Trace("CAN2: normal(enable)\r\n");
//			Oem_CAN2_STANDBY_INACTIVE();
		}
		else if(strcmp(argv[2], "disable") == 0) {
			Trace("CAN1: standby\r\n");
			Oem_CAN2_STANDBY_ACTIVE();
		}
		else if(strcmp(argv[2], "set") == 0) {
			uint32_t CanID1 = 0x00FF;
			uint32_t CanID2 = 0x0F00;

			Trace("CAN2: set\r\n");

			CanID1 = 0x00FF;
			CanID2 = 0x0F00;

			Oem_CAN_Initial_CH2(Lowcan1, eCAN_100KBPS);
			Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_CAN, 1, &CanID1, &CanID2);

			HalGPIOSetVaule(GPIO_LOW_CAN_EN, eBIT_SET);
		}
		else if(strcmp(argv[2], "send") == 0) {
		  stHalCanTxMsg TxMessage;
		  uint8_t n;

			Trace("CAN2: data send(0x321)\r\n");
		  /* Transmit Structure preparation */
		  TxMessage.StdId = 0x321;
		//  TxMessage.ExtId = 0x01;
		  TxMessage.RTR = HAL_CAN_RTR_DATA;
		  TxMessage.IDE = HAL_CAN_ID_STD;
		  TxMessage.DLC = 2;
		  TxMessage.Data[0] = 0x12;
		  TxMessage.Data[1] = 0x34;

          n = HalDrvCanWrite((int)HAL_CAN2, 0, (char*)&TxMessage, sizeof(stHalCanTxMsg), 0);
			Trace("transmit: %d\r\n", n);
		}
	}
    if(strcmp(argv[1],"ms") == 0)
	{
#if defined(PROTOCOL24)
		ReportModemStatus(3);
#endif
	}
	else if(strcmp(argv[1], "set") == 0) {
		Oem_CAN1_STANDBY_INACTIVE();
		unsigned int uiStartMask[14];
		unsigned int uiEndMask[14];

		DefaultMaskSet(uiStartMask,uiEndMask,8);

		Trace("CAN1: set\r\n");

		Oem_CAN_Initial_CH1(Highcan1, eCAN_500KBPS); //init
		Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN, 8, uiStartMask, uiEndMask);
//		Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN, 1, &uiStartMask[atoi(argv[2])], &uiEndMask[atoi(argv[2])]);
	}
	else if(strcmp(argv[1], "s") == 0) {
		unsigned char ucTemp[10]={0x01, 0x78, 0x40 ,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
		unsigned int uiDataLen=10;
		unsigned int aa=0;
		BOOL bStandardCan;
		stPASSTHRU_MSG stData;
		int i=0;
		static unsigned int s_Timer=0;
		stData.ProtocolID = ISO15765;
		VCI_SetPassThruProtocolID((unsigned char*)stData.ProtocolID);
		g_uiCanWriteMsgLength = 0;
		uiDataLen = 10 + 24;	//24 : stPASSTHRU_MSG �� �պκ�
		stData.DataSize = uiDataLen-24;	//10

		memcpy(stData.pData,ucTemp ,stData.DataSize);

		Oem_CAN_Initial_CH2(Lowcan1, eCAN_100KBPS);
		memcpy(&g_stWritePassThruMsg_L, &stData, uiDataLen);
		memset(&g_OutLCanPacket, 0x00, sizeof(stCanPacket));
		CAN_TxParsing(&g_OutLCanPacket, &g_stWritePassThruMsg_L, &bStandardCan, eLCAN);
		CAN_MakeTxSingleFrame(&g_OutLCanPacket, bStandardCan, &g_stWritePassThruMsg_L, eLCAN);
//		CAN_MakeTxSingleFrame(&g_OutLCanPacket, bStandardCan, &g_stWritePassThruMsg_L, eLCAN);

		CAN_MakeSendFrame_Lcan(&g_OutLCanPacket, bStandardCan, CAN_SINGLE_FRAME, &g_stWritePassThruMsg_L);

		s_Timer = 	Get_Tmr();

		for(i=0; i<5000;)
		{
			aa = Get_TmrDelta(Get_Tmr(), s_Timer );
//			printf("%d\r\n",aa);
			if(  aa >= 2 )
			{
//				usSendLength = CAN_WriteBuff((unsigned char *)&g_OutLCanPacket, 0, 2/* CAN_CH*/, CAN_SINGLE_FRAME);
				OemWriteCanBuff((unsigned char*)&g_OutLCanPacket, 0, NULL, 2);
				s_Timer = 	Get_Tmr();
				i++ ;
			}
		}
	}
	else if(strcmp(argv[1], "s2") == 0) {
		unsigned char ucTemp[10]={0x01, 0x78, 0x40 ,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
		unsigned int uiDataLen=10;
		//unsigned int usSendLength;
		BOOL bStandardCan;
		stPASSTHRU_MSG stData;

		stData.ProtocolID = ISO15765;
		VCI_SetPassThruProtocolID((unsigned char*)stData.ProtocolID);
		g_uiCanWriteMsgLength = 0;
		uiDataLen = 10 + 24;	//24 : stPASSTHRU_MSG �� �պκ�
		stData.DataSize = uiDataLen-24;	//10

		memcpy(stData.pData,ucTemp ,stData.DataSize);

		Oem_CAN_Initial_CH1(Lowcan1, eCAN_500KBPS);
		memcpy(&g_stWritePassThruMsg, &stData, uiDataLen);
		memset(&g_OutCanPacket, 0x00, sizeof(stCanPacket));
		CAN_TxParsing(&g_OutCanPacket, &g_stWritePassThruMsg, &bStandardCan, eDCAN);
		CAN_MakeTxSingleFrame(&g_OutCanPacket, bStandardCan, &g_stWritePassThruMsg, eDCAN);
//		CAN_MakeTxSingleFrame(&g_OutLCanPacket, bStandardCan, &g_stWritePassThruMsg_L, eLCAN);

		CAN_MakeSendFrame(&g_OutCanPacket, bStandardCan, CAN_SINGLE_FRAME, &g_stWritePassThruMsg);

		for(int i=0; i<5000; i++ )
		{
			//usSendLength = CAN_WriteBuff((unsigned char *)&g_OutCanPacket, 0, 1/* CAN_CH*/, CAN_SINGLE_FRAME);
            CAN_WriteBuff((unsigned char *)&g_OutCanPacket, 0, 1/* CAN_CH*/, CAN_SINGLE_FRAME);
		}

    }
#ifdef CGW_SECURITY
    else if(strcmp(argv[1], "cgw") == 0) {
		SetOBDState(eOBD_CGWAlgorithm);
    }
#endif
	else {
		Trace("known command!!!\r\n");
	}
}

static void app_test_cmd (uint32_t argc, char *argv[])
{
	if(strcmp(argv[1], "1") == 0) {
		// modem vs MCU �ν� ���� Ȯ��
		TestData.eState = eTEST_STATE_1;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "2") == 0) {
		// buzzer ����
		TestData.eState = eTEST_STATE_2;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "3") == 0) {
		// LED On
		TestData.eState = eTEST_STATE_3;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "4") == 0) {
		// serial flash
		TestData.eState = eTEST_STATE_4;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "5") == 0) {
		// BLE
		TestData.eState = eTEST_STATE_5;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "6") == 0) {
		// USIM
		TestData.eState = eTEST_STATE_6;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "7") == 0) {
		// GPS
		TestData.eState = eTEST_STATE_7;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "8") == 0) {
		// RTC
		TestData.eState = eTEST_STATE_8;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "9") == 0) {
		// Micom USB  MSD
		TestData.eState = eTEST_STATE_9;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "Z") == 0 || strcmp(argv[1], "z") == 0) {
		// Modem USB
		TestData.eState = eTEST_STATE_10;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "X") == 0 || strcmp(argv[1], "x") == 0) {
		// LOW CAN
		TestData.eState = eTEST_STATE_11;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "C") == 0 || strcmp(argv[1], "c") == 0) {
		// HIGH CAN - channel 1
		TestData.eState = eTEST_STATE_12;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "D") == 0 || strcmp(argv[1], "d") == 0) {
		// HIGH CAN - channel 2
		TestData.eState = eTEST_STATE_14;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "V") == 0 || strcmp(argv[1], "v") == 0) {
		// MCU Sleep or Modem Sleep -> wakeup Ȯ��(CAN message�� wakeup)
		TestData.eState = eTEST_STATE_13;
		TestData.bEnableTestMode = true;
	}
	else if(strcmp(argv[1], "0") == 0) {
		// 1 ~ 14 ���� ���� ����
		TestData.eState = eTEST_STATE_1;
		TestData.bEnableTestMode = true;
		TestData.bEnableLoopTestMode = true;
		TestData.nTestCount = 0;

		HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

//		stSystemTestInfo.bRunTestMode = true;
//
//		SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
	}
	else if(strcmp(argv[1], "B") == 0 || strcmp(argv[1], "b") == 0) {
		// 1 ~ 13 ���� ���� ����
		TestData.bEnableTestMode = false;
		TestData.bEnableLoopTestMode = false;

//		stSystemTestInfo.bRunTestMode = false;
//
//		SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
	}
//	else if(strcmp(argv[1], "20") == 0) {
//		TestData.eState = eTEST_STATE_20;
//		TestData.bEnableTestMode = true;
//		TestData.bEnableLoopTestMode = false;
//		gnTestState_SFlash = 0;
//		gnTestCount_SFlash = 0;
//	}
	else {
		Trace("known command!!!\r\n");
	}

	return;
}

void StandbyRTCBKPSRAMMode_Measure(void)
{
    /* Display the RTC Time and Alarm */
    stHalRTCTypeDef stHalRtcDateTime;
    APP_TimeShow(&stHalRtcDateTime);
    Trace("1. Display the RTC Time\r\n");

    APP_Delay(1000);

    Trace("Reset RTC Domain\r\n");

    /* Reset RTC Domain */
    HalDrvRccIOCtrl(eRCC_IO_BKRam_Clock_Reset, HAL_ENABLE, NULL, 0, 0);
    HalDrvRccIOCtrl(eRCC_IO_BKRam_Clock_Reset, HAL_DISABLE, NULL, 0, 0);

    Trace("2. Display the RTC Time\r\n");
    APP_Delay(1000);

    /* Display the RTC Time and Alarm */
    APP_TimeShow(&stHalRtcDateTime);


    /* Allow access to RTC */
    HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);

    /* Enable the LSE OSC */
    HalDrvRccIOCtrl(eRCC_IO_SET_LSE_CONFIG, HAL_RCC_LSE_ON, NULL, 0, 0);

    /* Wait till LSE is ready */
    while(HalDrvRccIOCtrl(eRCC_IO_GET_FLAG_STATUS, HAL_RCC_FLAG_LSERDY, NULL, 0, 0) == RESET)
    {
    }

    /* Select the RTC Clock Source */
    HalDrvRccIOCtrl(eRCC_IO_SET_RTC_Clock_CONFIG, HAL_RCC_RTCCLKSource_LSE, NULL, 0, 0);

    /* Enable the RTC Clock */
    HalDrvRccIOCtrl(eRCC_IO_SET_RTC_Clock_ENABLE, 0, NULL, 0, HAL_ENABLE);

    /* Wait for RTC APB registers synchronisation */
    HalDrvRtcIOCtrl(eRtc_IO_WaitForSync, 0, NULL, 0, 0);

    /*  Backup SRAM ***************************************************************/
    /* Enable BKPRAM Clock */
    HalDrvRccIOCtrl(eRCC_IO_BKRam_Clock, 0, NULL, 0, HAL_ENABLE);
    

    /* Enable the Backup SRAM low power Regulator */
    HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);

    /* Wait until the Backup SRAM low power Regulator is ready */
    while( HalDrvPowerIOCtrl(ePWR_IO_GetFlagStatus, HAL_PWR_FLAG_BRR, NULL, 0, 0) == HAL_RESET )
    {
    }

    /* RTC Wakeup Interrupt Generation: Clock Source: RTCCLK_Div16, Wakeup Time Base: ~20s
     RTC Clock Source LSE 32.768KHz or LSI ~32KHz

     Wakeup Time Base = (16 / (LSE or LSI)) * WakeUpCounter
    */
    HalDrvRtcIOCtrl(eRtc_IO_WakeupClockConfig, HAL_RTC_WakeUpClock_RTCCLK_Div16, NULL, 0,0);
    HalDrvRtcIOCtrl(eRtc_IO_SetWakupCounter, (0xA000-1) / 2, NULL, 0,0);

    /* Disable the Wakeup Interrupt */
    HalDrvRtcIOCtrl(eRtc_IO_IntEnable, HAL_RTC_IT_WUT, NULL, 0, HAL_DISABLE);

    /* Clear Power WakeUp (CWUF) pending flag */
    /* Clear Wakeup flag */
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);

    /* Enable the Wakeup Interrupt */
    HalDrvRtcIOCtrl(eRtc_IO_IntEnable, HAL_RTC_IT_WUT, NULL, 0, HAL_ENABLE);

    /* Enable Wakeup Counter */
    HalDrvRtcIOCtrl(eRtc_IO_WakeupEnable, 0, NULL, 0, HAL_ENABLE);
    

    /* Clear WakeUp (WUTF) pending flag */
    HalDrvRtcIOCtrl(eRtc_IO_ClearFlagStatus, HAL_RTC_FLAG_WUTF, NULL, 0, 0);

    Trace("enter standby mode...\r\n");

    /* Request to enter STANDBY mode (Wake Up flag is cleared in ePWR_IO_PWR_EnterStandbyMode function) */
    HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStandbyMode, 0, NULL, 0, 0);
    Trace("...\r\n");

    /* Infinite loop */
    while (1)
    {
    }
}

static void msg_op_cmd (uint32_t argc, char *argv[])
{
	unsigned char ucGUID[MAX_GUID_LENGTH];
	unsigned char ucBTKEY[MAX_BT_CONTROLKEY_LENGTH];
	memset(ucGUID, 0x31, MAX_GUID_LENGTH);
	memset(ucBTKEY, 0x32, MAX_BT_CONTROLKEY_LENGTH);
	if(strcmp(argv[1], "set") == 0) 
	{
		if(strcmp(argv[2], "remotecmd") == 0) {
			stCarReport msg;
			if(argc == 6)
			{
				Trace("\r\n\r\n");
				Trace("****************************************\r\n");
				Trace("MSG: Remote control event!!!\r\n");
				Trace("****************************************\r\n\r\n");

                msg.rpSmartKey.Request.CommandType = (eREMOTE_CON_CMD_TYPE)atoi(argv[3]);
                msg.rpSmartKey.Request.ControlType = atoi(argv[4]);
                msg.rpSmartKey.Request.KeepPowerOnTime = 10;

				if(CFD_GetCanFDAdapter())
				{
                     //250,10,16,16,36
                    
                    msg.rpSmartKey.Request.Temperature = 0x1610;
                    msg.rpSmartKey.Request.CheckTemperature = 0x16;
                    msg.rpSmartKey.Request.Defrost = 0;
                    msg.rpSmartKey.Request.RearDefogger = atoi(argv[5]);
/*
				   msg.rpSmartKey.Request.Temperature = 0x0710;
				   msg.rpSmartKey.Request.CheckTemperature = 0x07;
				   msg.rpSmartKey.Request.Defrost = 0;
				   msg.rpSmartKey.Request.RearDefogger = atoi(argv[5]);
*/
				}
				else
				{
                    msg.rpSmartKey.Request.Temperature = 0x01D0;
                    msg.rpSmartKey.Request.CheckTemperature = 0x23;
                    msg.rpSmartKey.Request.Defrost = 0;
                    msg.rpSmartKey.Request.RearDefogger = atoi(argv[5]);
				}
				msg.rpSmartKey.Request.BoundType = eREMOTE_CON_BOUNDTYPE_NONE;
				msg.rpSmartKey.Request.ucSysSmartkeyReqType = eSysSmartkeyReqType_Modem;
				memcpy(msg.rpSmartKey.Request.Guid,ucGUID,MAX_GUID_LENGTH);
				memcpy(msg.rpSmartKey.Request.BTControlKey,ucBTKEY,MAX_BT_CONTROLKEY_LENGTH);

                Send2MngObd(eMngSysMsg,eReqReport,eR_ReqSmartKey,&msg,0);
			}
			else if(argc == 7)
			{
				Trace("\r\n\r\n");
				Trace("****************************************\r\n");
				Trace("MSG: Remote control event2!!!\r\n");
				Trace("****************************************\r\n\r\n");

                msg.rpSmartKey.Request.CommandType = (eREMOTE_CON_CMD_TYPE)atoi(argv[3]);
                msg.rpSmartKey.Request.ControlType = atoi(argv[4]);
                msg.rpSmartKey.Request.KeepPowerOnTime = 10;
                if(CFD_GetCanFDAdapter())
				{
				   msg.rpSmartKey.Request.Temperature = 0x0710;
				   msg.rpSmartKey.Request.CheckTemperature = 0x07;
				   msg.rpSmartKey.Request.Defrost = atoi(argv[5]);
				   msg.rpSmartKey.Request.RearDefogger = atoi(argv[6]);
				}
				else
				{
				   msg.rpSmartKey.Request.Temperature = 0x01D0;
				   msg.rpSmartKey.Request.CheckTemperature = 0x23;
				   msg.rpSmartKey.Request.Defrost = atoi(argv[5]);
				   msg.rpSmartKey.Request.RearDefogger = atoi(argv[6]);
				}
                msg.rpSmartKey.Request.BoundType = eREMOTE_CON_BOUNDTYPE_NONE;
				msg.rpSmartKey.Request.ucSysSmartkeyReqType = eSysSmartkeyReqType_BT;
				memcpy(msg.rpSmartKey.Request.Guid,ucGUID,MAX_GUID_LENGTH);
				memcpy(msg.rpSmartKey.Request.BTControlKey,ucBTKEY,MAX_BT_CONTROLKEY_LENGTH);

                Send2MngObd(eMngSysMsg,eReqReport,eR_ReqSmartKey,&msg,0);
			}
			ShowSmartkeyAction(msg);
		}
        else if (strcmp(argv[2], "di") == 0)
        {
            uint32_t unDi = atoi(argv[3])*ONE_SECOND;
            SetBackupRamConfigProperty(eBackupRamConfig_DrivingInterval,(void*)&unDi);
            Trace("Set driving interval : %d\r\n",unDi);
        }
        else if (strcmp(argv[2], "pi") == 0)
        {
            uint32_t unPi = atoi(argv[3]);
            SetBackupRamConfigProperty(eBackupRamConfig_WakeupInterval,(void*)&unPi);
            Trace("Set parking interval : %d\r\n",unPi);
        }
        else if (strcmp(argv[2], "fi") == 0)
        {
            uint32_t unFi = atoi(argv[3]);
            SetBackupRamConfigProperty(eBackupRamConfig_FotaInterval,(void*)&unFi);
            Trace("Set fota interval : %d\r\n",unFi);
        }
        else if (strcmp(argv[2], "odo") == 0)
        {
            uint32_t unOdo = atoi(argv[3]);
            SetAutolinkConfigProperty(eAutoLinkConfig_Odotmeter,(void*)&unOdo);
            Trace("Set fota interval : %d\r\n",unOdo);
        }
        else if (strcmp(argv[2], "clear") == 0)
        {
            WriteDefaultAutolinkConfigValue();
        }
        else if (strcmp(argv[2], "newVin") == 0)
        {
            uint8_t cNewVin = atoi(argv[3]);
            SetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,(void*)&cNewVin);
            Trace("Set New Vin Allow : %d\r\n",cNewVin);
        }
        else if (strcmp(argv[2], "factory") == 0)
        {
            if( strcmp(argv[3],"reset") == 0 )
            {
                Trace("##########################################\r\n");
                Trace("Factory Reset\r\n");
                Trace("##########################################\r\n");
                DefaultFWInfo();
                ClearRecoveryFile();
                WriteDefaultAutolinkConfigValue();
            }
        }
	}
	else if(strcmp(argv[1], "get") == 0) {
        if(strcmp(argv[2], "ver") == 0) {
            /***********************************************************
             * DO NOT BLOCK THIS printf for IOT.
             **********************************************************/
            printf("\r\n==========================================\r\n");
            printf("F/W Ver Rev : %04X\r\n",FW_VERSION());
        }
        else if(strcmp(argv[2], "hwver") == 0) {

            unsigned short  ulBattVolt=0;
            unsigned int    nAdc_RawBuf   = 0;

            HalADC_SetAdc_Chnnel((unsigned int)HAL_ADC2, 1);
            APP_Delay(1000);


            if (HalADC_Read2(&ulBattVolt))
			{
				nAdc_RawBuf += ulBattVolt;

                /***********************************************************
                * DO NOT BLOCK THIS printf for IOT.
                **********************************************************/
				//printf("\r\n adc2 adcnt %02d Batcnt %04X ", uAdCnt, ulBattVolt);
                printf("\r\n==========================================\r\n");
                if( ulBattVolt > 0x0F00 )
				    printf("HW Ver : 2.7\r\n");
                else if( ulBattVolt > 0x0C00 )
                    printf("HW Ver : 2.6\r\n");
                else if( ulBattVolt > 0x0900 )
                    printf("HW Ver : 2.5\r\n");
                else if( ulBattVolt > 0x0600 )
                    printf("HW Ver : 2.4\r\n");
                else if( ulBattVolt > 0x0300 )
                    printf("HW Ver : 2.3\r\n");
			}
        }
        else if (strcmp(argv[2], "di") == 0)
        {
            uint32_t unDi = 0;
            GetBackupRamConfigProperty(eBackupRamConfig_DrivingInterval,(void*)&unDi);
            Trace("Get driving interval : %d\r\n",unDi);
        }
        else if (strcmp(argv[2], "pi") == 0)
        {
            uint32_t unPi = 0;
            GetBackupRamConfigProperty(eBackupRamConfig_WakeupInterval,(void*)&unPi);
            Trace("Get parking interval : %d\r\n",unPi);
        }
        else if (strcmp(argv[2], "fi") == 0)
        {
            uint32_t unFi = 0;
            GetBackupRamConfigProperty(eBackupRamConfig_FotaInterval,(void*)&unFi);
            Trace("Get fota interval : %d\r\n",unFi);
        }
        else if (strcmp(argv[2], "odo") == 0)
        {
            uint32_t unOdo;
            GetAutolinkConfigProperty(eAutoLinkConfig_Odotmeter,(void*)&unOdo);
            Trace("Get fota interval : %d\r\n",unOdo);
        }
        else if ( strcmp(argv[2], "config" ) == 0 )
        {
            DisplayAutolinkConfigData();
        }
	}
	else if(strcmp(argv[1], "read") == 0) {
		if(strcmp(argv[2], "errfile") == 0) {
			Trace("[MSG] read failed message file\r\n");
			MSG_ReadErrorFile();
		}
	}
	else if(strcmp(argv[1], "del") == 0) {
		if(strcmp(argv[2], "currfile") == 0) {
			Trace("[MSG] delete current failed message file\r\n");
			MSG_DelErrorFile();
		}
		else if(strcmp(argv[2], "errfiles") == 0) {
			Trace("[MSG] delete all failed message file\r\n");
			MSG_DelAllErrorFile();
		}
	}
	else if(strcmp(argv[1], "remote") == 0) {
		if(strcmp(argv[2], "fail") == 0) {
			Trace("[MSG] Fail remote control\r\n");

			MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_FAIL;
		}
		else if(strcmp(argv[2], "success") == 0) {
			Trace("[MSG] success remot control\r\n");

			MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_SUCCESS;
		}
	}
	else if(strcmp(argv[1], "send") == 0) {
			Trace("\r\n\r\n");
			Trace("****************************************\r\n");
			Trace("MSG: Remote control event!!!\r\n");
			Trace("****************************************\r\n\r\n");

			stCarReport msg;
			msg.rpSmartKey.Request.CommandType = (eREMOTE_CON_CMD_TYPE)1;
			msg.rpSmartKey.Request.ControlType = 1;
			msg.rpSmartKey.Request.KeepPowerOnTime = 10;
			msg.rpSmartKey.Request.Temperature = 0xFFFF;
			msg.rpSmartKey.Request.Defrost = 0xFF;
			msg.rpSmartKey.Request.BoundType = eREMOTE_CON_BOUNDTYPE_NONE;

			Send2MngObd(eMngSysMsg,eReqReport,eR_ReqSmartKey,&msg,0);
	}
	else{
//		if(argc == 3) {
//			Trace("\n\n");
//			Trace("****************************************\n");
//			Trace("MSG: Remote control event!!!\n");
//			Trace("****************************************\n\n");
//
//			char temp1,temp2;
//
//			stCarReport msg;
//			msg.rpSmartKey.Request.CommandType = (eREMOTE_CON_CMD_TYPE)1;
//			msg.rpSmartKey.Request.ControlType = 1;
//			msg.rpSmartKey.Request.KeepPowerOnTime = 10;
//			temp1 = AsciiToHex(argv[1][0],argv[1][1]);
//			temp2 = AsciiToHex(argv[2][0],argv[2][1]);
//			msg.rpSmartKey.Request.Temperature = (uint16_t)(temp1*256 + temp2);
//			msg.rpSmartKey.Request.Defrost = 0;
//			msg.rpSmartKey.Request.BoundType = eREMOTE_CON_BOUNDTYPE_NONE;
//
//			Send2MngObd(eMngSysMsg,eReqReport,eR_ReqSmartKey,&msg,0);
//		}
	}
	return;
}

/*
 * NAME
 *    testmode_op_cmd()
 *
 * DESCRIPTION
 *    테스트용으로 modem H/W reset 과 module sleep(standby) 을 disable / enable 한다.
 *
 *    사용법:
 *      testmode on       : modem reset + sleep 둘 다 disable (bench test 진입)
 *      testmode off      : 정상 동작으로 복귀 (둘 다 다시 enable)
 *      testmode reset on|off  : modem reset 만 개별 제어 (on=정상, off=disable)
 *      testmode sleep on|off  : sleep 만 개별 제어        (on=정상, off=disable)
 *      testmode status   : 현재 상태 표시
 */
static void testmode_op_cmd (uint32_t argc, char *argv[])
{
	if( argc < 2 )
	{
		printf("usage: testmode <on|off|status|reset on/off|sleep on/off>\r\n");
		return;
	}

	if( strcmp(argv[1], "on") == 0 )
	{
		// 테스트 모드 진입: 둘 다 disable
		g_bTestDisableModemReset = true;
		g_bTestDisableSleep      = true;
		printf("[TESTMODE ON] modem reset DISABLED, sleep DISABLED\r\n");
	}
	else if( strcmp(argv[1], "off") == 0 )
	{
		// 정상 동작 복귀: 둘 다 enable
		g_bTestDisableModemReset = false;
		g_bTestDisableSleep      = false;
		printf("[TESTMODE OFF] modem reset ENABLED, sleep ENABLED (normal)\r\n");
	}
	else if( strcmp(argv[1], "reset") == 0 )
	{
		// modem reset 개별 제어 ( on = 정상 동작, off = disable )
		if( strcmp(argv[2], "off") == 0 )		g_bTestDisableModemReset = true;
		else if( strcmp(argv[2], "on") == 0 )	g_bTestDisableModemReset = false;
		else { printf("usage: testmode reset <on|off>\r\n"); return; }
		printf("modem reset : %s\r\n", (g_bTestDisableModemReset == true) ? "DISABLED" : "ENABLED");
	}
	else if( strcmp(argv[1], "sleep") == 0 )
	{
		// sleep 개별 제어 ( on = 정상 동작, off = disable )
		if( strcmp(argv[2], "off") == 0 )		g_bTestDisableSleep = true;
		else if( strcmp(argv[2], "on") == 0 )	g_bTestDisableSleep = false;
		else { printf("usage: testmode sleep <on|off>\r\n"); return; }
		printf("sleep : %s\r\n", (g_bTestDisableSleep == true) ? "DISABLED" : "ENABLED");
	}
	else if( strcmp(argv[1], "status") == 0 )
	{
		printf("[TESTMODE STATUS]\r\n");
		printf("  modem reset : %s\r\n", (g_bTestDisableModemReset == true) ? "DISABLED" : "ENABLED");
		printf("  sleep       : %s\r\n", (g_bTestDisableSleep == true) ? "DISABLED" : "ENABLED");
	}
	else
	{
		printf("usage: testmode <on|off|status|reset on/off|sleep on/off>\r\n");
	}
}

static void modem_op_cmd (uint32_t argc, char *argv[])
{
	if(strcmp(argv[1], "enable") == 0) {
		if(strcmp(argv[2], "rssi") == 0) {
			Trace(" enable rssi\r\n");
			gbEnableShowRSSI = true;
		}
	}
	else if(strcmp(argv[1], "disable") == 0) {
		if(strcmp(argv[2], "rssi") == 0) {
			Trace(" disable rssi\r\n");
			gbEnableShowRSSI = false;
		}
	}
	else if(strcmp(argv[1], "disconnecttest") == 0) {
		printf("disconnecttest on\r\n");
		g_bDisconnectTestFlag = true;
	}
	else if(strcmp(argv[1], "reset") == 0) {
		Trace("\r\n");
		Trace("modem hardware reset\r\n");

		if(strcmp(argv[2], "1") == 0) {
			Trace("Use reset state machine\r\n");
			ModemManagerData.bNeedModemHWResetFlag = true;
		}
		else {
			Trace("Immediately reset\r\n");
			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
			HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);
			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Recieved_TimeoutDly);

#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
            //HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
            //APP_Delay(200);
            HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
            APP_Delay(200);
            HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
            APP_Delay(200);
            HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
            //APP_Delay(200);
            //HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#else

			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
			APP_Delay(5);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
			APP_Delay(5);
			HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
#endif

			InitializeModemManager(true);
			SetModemState(eMODEM_Idle);
		}
	}
	else if(strcmp(argv[1], "enter") == 0) {
		if(strcmp(argv[2], "sleep") == 0) {
			SetModemState(eMODEM_ENTER_SLEEP_MODE);
		}
		else {
			HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

			Trace("\r\n");
			Trace("enter modem direct interface...\r\n");
			g_bEnableModemDirectCommunication = true;

			//SetModemState(eMODEM_NONE);
		}
	}
	else if(strcmp(argv[1], "checksms") == 0) {
		if(GetModemState() != eMODEM_READY) {
			Trace(" Modem busy now...(%d)\r\n", GetModemState());

			return;
		}
	}
	else if(strcmp(argv[1], "sendsms") == 0) {
		if(argc == 4) {
			/*uint16_t len;

			if(GetModemState() != eMODEM_READY) {
				Trace(" Modem busy now...(%d)\n", GetModemState());

				return;
			}
			Trace(" sending SMS\n");

			Trace("address: \"%s\"\n", argv[2]);
			Trace("   data: %s\n", argv[3]);

			sprintf((char *)ModemManagerData.aSMSDestinationAddress, "\"%s\"\x00", argv[2]);
			sprintf((char *)ModemManagerData.aSMSSendData, "%s\x00", argv[3]);

			len = strlen((char *)ModemManagerData.aSMSSendData);
			ModemManagerData.aSMSSendData[len] = 0x1A;
			ModemManagerData.nLengthSMSSendData = len + 1;

			Md_SendSMS(ModemManagerData.aSMSDestinationAddress);*/
		}
	}
	else if(strcmp(argv[1], "info") == 0) {
		Trace("\r\n");
//		Trace("Modem vendor name: %s\n", ModemManagerData.aVendorName);
		Trace("Modem product name: %s\r\n", ModemManagerData.aProductName);
		Trace("Modem revision: %s\r\n", ModemManagerData.aRevision);
		Trace("Modem a-revision: %s\r\n", ModemManagerData.aARevision);
		Trace("Modem IMEI: %s\r\n", ModemManagerData.aIMEI);
		Trace("Modem USIM CCID: %s\r\n", BkSram_ModemInfo.aCCID);
		Trace("Modem phone no: %s\r\n", BkSram_ModemInfo.aPhoneNo);
	}
	else if(strcmp(argv[1], "get") == 0) {
		if(strcmp(argv[2], "imei") == 0) {
			Trace("\r\n");
			Trace("Modem IMEI: %s\r\n", ModemManagerData.aIMEI);
		}
		else if(strcmp(argv[2], "rssi") == 0) {
			if(GetModemState() != eMODEM_READY) {
				Trace(" Modem busy now...\r\n");

				return;
			}

//			Md_CheckSignalQuality();
//			ModemManagerData.nRSSI = n1;
//			ModemManagerData.ndBm = 2 * n1 - 113;
			Trace(" RSSI: %d(%+d dBm)\r\n", ModemManagerData.nRSSI, ModemManagerData.ndBm);
		}
		else if(strcmp(argv[2], "cgreg") == 0) {
			if(GetModemState() != eMODEM_READY) {
				Trace(" Modem busy now...\r\n");

				return;
			}

			Md_CheckPacketDomainRegistration();
		}
		else if(strcmp(argv[2], "operator") == 0) {
			if(GetModemState() != eMODEM_READY) {
				Trace(" Modem busy now...\r\n");

				return;
			}

			Md_ReadOperatorSelection();
		}
		else if(strcmp(argv[2], "wakepin") == 0) {
			bool state;

			state = HalGPIOGetStatus(GPIO_MO_WAKE);
			Trace(" pin(MO_WAKE): %d\r\n", state);
		}
		else if(strcmp(argv[2], "state") == 0) {
			if(strcmp(argv[3], "fota") == 0) {
				Trace("g_FotaManagerData.eState: %d\r\n", g_FotaManagerData.eState);
			}
		}
        else if(strcmp(argv[2], "apn") == 0)
        {
            uint8_t carrApn[128]={0};

            GetBackupRamConfigProperty(eBackupRamConfig_Apn,(void*)carrApn);
            Trace("Get modem apn : %s\r\n",carrApn);
        }
        else if(strcmp(argv[2], "active") == 0)
        {
            uint8_t bModemActive;
            GetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);
            Trace("Get modem active : %d\r\n",bModemActive);
        }
	}
	else if(strcmp(argv[1], "set") == 0) {
		if(strcmp(argv[2], "powerkey") == 0) {
			if(strcmp(argv[3], "low") == 0) {
				Trace("\r\n");
				Trace("[MDM]: reset power key\r\n");
				HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
			}
			else if(strcmp(argv[3], "high") == 0) {
				Trace("\r\n");
				Trace("[MDM]: set power key\r\n");
				HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
			}
		}
		else if(strcmp(argv[2], "modembaud") == 0) {
			uint32_t wBaud;

			wBaud = atoi(argv[3]);
			if(wBaud == 3000000) {
				if(Md_Set_3M_Baudrate() == true) {
					Trace(" Set baud 3000000\r\n");
				}
			}
			else if(wBaud == 921600) {
				if(Md_Set_921600_Baudrate() == true) {
					Trace(" Set baud 921600\r\n");
				}
			}
			else if(wBaud == 115200) {
				if(Md_Set_115200_Baudrate() == true) {
					Trace(" Set baud 115200\r\n");
				}
			}
		}
		else if(strcmp(argv[2], "baud") == 0) {
//			stSystemTestInfo.wModemBaudrate = atoi(argv[3]);
//			SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
//			Trace(" Set baudrate: %d\n", stSystemTestInfo.wModemBaudrate);
		}
        else if(strcmp(argv[2], "apn") == 0)
        {
            Trace("Set modem apn : %s\r\n",argv[3]);

            SetBackupRamConfigProperty(eBackupRamConfig_Apn,(void*)argv[3]);

            stMsgMdm stMsg;
            memset((char*)&stMsg,0,sizeof(stMsg));

            stMsg.header.id = eMngModem;
            stMsg.header.event = eSetApn;

            memcpy(stMsg.carReport.rpSetting.ModemSetting.carrApn,argv[3],strlen(argv[3]));

            Send2MngModem2(&stMsg);
        }
        else if(strcmp(argv[2], "active") == 0)
        {
            uint8_t bModemActive = atoi(argv[3]);
            Trace("Set modem active : %d\r\n",bModemActive);
            SetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);
        }
        else if(strcmp(argv[2], "clear") == 0)
        {
            Trace("clear modem recovery file\r\n");
            ClearRecoveryFile();
        }
        else if(strcmp(argv[2], "idle") == 0)
        {
            if(strcmp(argv[2], "idle") == 0)
            {
                SetNetworkPostPoneFlag(true);
            }
        }
	}
	else if(strcmp(argv[1], "init") == 0) {
		Trace("\r\n");
		Trace(" Init UART2(3000000bps), enable flowcontrol\r\n");
		ModemUart_Init(3000000);

        HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart2.pUARTreg, NULL, 0, HAL_ENABLE);
	}
	else if(strcmp(argv[1], "flowcontrol") == 0) {
		if(strcmp(argv[2], "none") == 0) {
			stSystemTestInfo.eModemFlowControlType = eMODEM_UART_FLOWCONTROL_TYPE_NONE;

			SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
			Trace(" new setting flowcontrol - none\r\n");
		}
		else if(strcmp(argv[2], "gpio") == 0) {
			stSystemTestInfo.eModemFlowControlType = eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS_GPIO;

			SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
			Trace(" new setting flowcontrol - GPIO rts + cts\r\n");
		}
		else if(strcmp(argv[2], "rtscts") == 0) {
			stSystemTestInfo.eModemFlowControlType = eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS;

			SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
			Trace(" new setting flowcontrol - rts + cts\r\n");
		}
		else if(strcmp(argv[2], "cts") == 0) {
			stSystemTestInfo.eModemFlowControlType = eMODEM_UART_FLOWCONTROL_TYPE_CTS;

			SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
			Trace(" new setting flowcontrol - cts\r\n");
		}
	}
	else if(strcmp(argv[1], "fota") == 0) {
		if(strcmp(argv[2], "info") == 0) {
			Trace("FOTA: Start Fota - get info\r\n");
			ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_VERSION;

//			Send2MngModem(eMngSysMsg,eReqFota,eFwInfo,(stCarReport *)NULL,0);
		}
		else if(strcmp(argv[2], "bin") == 0) {
			Trace("!!! Default FW Information !!!\r\n");
//			DefaultFWInfo();

			Trace("FOTA: Start Fota - get bin\r\n");
			ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_BIN;
		}
		else if(strcmp(argv[2], "vinfo") == 0) {
			ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_VEHICLE_INFO;
		}
	}
	else {
		Trace("*unknow command...\r\n");
	}

	return;
}

static void ble_op_cmd (uint32_t argc, char *argv[])
{
  	LOCK_SET_STATE(eLOCK_STATE_UNLOCK);
    stBT_PTCL_PAYLOAD PayloadPtcl;
    memset(&PayloadPtcl, 0, sizeof(stBT_PTCL_PAYLOAD));
	if(strcmp(argv[1], "power") == 0) {
		Trace("\r\n");
		Trace("BLE power\r\n");

		if(strcmp(argv[2], "on") == 0) {
			Trace("BLE: Power On\r\n");
			HalGPIOSetVaule(GPIO_BT_PWEN, eBIT_SET);
		}
		else if(strcmp(argv[2], "off") == 0) {
			Trace("BLE: Power Off\r\n");
			HalGPIOSetVaule(GPIO_BT_PWEN, eBIT_RESET);
		}
	}
	else if(strcmp(argv[1], "apn") == 0) {
		PayloadPtcl.pPayload[0] = 0xc1;
		BTApnSettingRes(&PayloadPtcl, eCOMM_TYPE_UART_BT);
	}
#if defined(PROTOCOL18)
	else if(strcmp(argv[1], "aircon") == 0) {
		stBT_PTCL_PAYLOAD PayloadPtcl;
		unsigned short commandId = 0x2402;
		memset(&PayloadPtcl, 0, sizeof(stBT_PTCL_PAYLOAD));

		memcpy(&(PayloadPtcl.pPayload[1]), &commandId, sizeof(unsigned short));

		BTAirConStatusRes(&PayloadPtcl,eCOMM_TYPE_UART_BT);
	}
#endif
	else if(strcmp(argv[1], "remote") == 0) {
		if(strcmp(argv[2], "req") == 0){
			stBT_PTCL_PAYLOAD PayloadPtcl;
            memset(&PayloadPtcl, 0, sizeof(stBT_PTCL_PAYLOAD));
			unsigned short commandId = 0x4000;

			Trace("\r\n\r\n");
			Trace("****************************************\r\n");
			Trace("MSG: BT Remote control event!!!\r\n");
			Trace("****************************************\r\n\r\n");


	        PayloadPtcl.pPayload[0]=0xc1;
	        memcpy(&(PayloadPtcl.pPayload[1]), &commandId, sizeof(unsigned short));

	        PayloadPtcl.pPayload[3]=0;
	        for(int i=4;i<8;i++)
	        {
	        	PayloadPtcl.pPayload[i]=0;
	        }
	        for(int i=8;i<136;i++)
	        {
	        	PayloadPtcl.pPayload[i]=1;
	        }
	        for(int i=136;i<140;i++)
	        {
	        	PayloadPtcl.pPayload[i]=0;
	        }
#if defined(PROTOCOL18)
	       	BTOTCnRemoteCtrl(&PayloadPtcl, eCOMM_TYPE_UART_BT);
#endif
	   }
	   else if(strcmp(argv[2], "result") == 0)
	   {

			stBT_PTCL_PAYLOAD PayloadPtcl;
            memset(&PayloadPtcl, 0, sizeof(stBT_PTCL_PAYLOAD));
			unsigned short commandId = 0x8000;

			Trace("\r\n\r\n");
			Trace("****************************************\r\n");
			Trace("MSG: BT Remote control Result!!!\r\n");
			Trace("****************************************\r\n\r\n");


	        PayloadPtcl.pPayload[0]=0xc1;
	        memcpy(&(PayloadPtcl.pPayload[1]), &commandId, sizeof(unsigned short));

	        PayloadPtcl.pPayload[3]=0x04;

	        for(int i=4;i<8;i++)
	        {
	        	PayloadPtcl.pPayload[i]=0;
	        }
	        for(int i=8;i<136;i++)
	        {
	        	PayloadPtcl.pPayload[i]=1;
	        }
	        for(int i=136;i<140;i++)
	        {
	        	PayloadPtcl.pPayload[i]=0;
	        }
#if defined(PROTOCOL18)
	        BTOTCnRemoteCtrl(&PayloadPtcl, eCOMM_TYPE_UART_BT);
#endif
	   }

	}
	else if(strcmp(argv[1], "status") == 0) {
		BOOL b;

		Trace("\r\n");
		Trace("Check BLE monitor port\r\n");

		b = BTGetConnectStatus();

		Trace("BLE: Status %d\r\n", b);
		if(b == SET) {
			Trace("BLE: Connected\r\n");
		}
		else {
			Trace("BLE: Not Connected\r\n");
		}
	}
	else if(strcmp(argv[1], "wake") == 0) {
		Trace("\r\n");
		Trace("BLE wake port\r\n");

		if(strcmp(argv[2], "low") == 0) {
			Trace("BLE: GPIO_BT_WAKE Low\r\n");
			BTSetRepeatModeReq(5000,5000);
			APP_Delay(100);
			HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_RESET);
		}
		else if(strcmp(argv[2], "high") == 0) {
			Trace("BLE: GPIO_BT_WAKE High\r\n");
			HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_SET);
		}
	}
	else if(strcmp(argv[1], "txdw") == 0) {
		Trace("\r\n");
		Trace("BLE GPIO_BT_TX_DW port\r\n");

		if(strcmp(argv[2], "low") == 0) {
			Trace("BLE: GPIO_BT_TX_DW Low\r\n");
			HalGPIOSetVaule(GPIO_BT_TX_DW, eBIT_RESET);
		}
		else if(strcmp(argv[2], "high") == 0) {
			Trace("BLE: GPIO_BT_TX_DW High\r\n");
			HalGPIOSetVaule(GPIO_BT_TX_DW, eBIT_SET);
		}
	}
	else if(strcmp(argv[1], "reset") == 0) {
		if(argc == 2) {
			Trace("\r\n");
			Trace("BLE Reset\r\n");

			HalGPIOSetVaule(GPIO_BT_RESET, eBIT_SET);
			APP_Delay(100);
			HalGPIOSetVaule(GPIO_BT_RESET, eBIT_RESET);
		}
		else if(argc == 3) {
			Trace("\r\n");
			Trace("BLE reset port\r\n");

			if(strcmp(argv[2], "low") == 0) {
				Trace("BLE: BT_RSTB Low\r\n");
				HalGPIOSetVaule(GPIO_BT_RESET, eBIT_RESET);
			}
			else if(strcmp(argv[2], "high") == 0) {
				Trace("BLE: BT_RSTB High\r\n");
				HalGPIOSetVaule(GPIO_BT_RESET, eBIT_SET);
			}
		}
	}
	else if(strcmp(argv[1], "enable") == 0) {
		if(strcmp(argv[2], "res") == 0) {
			Trace("\r\n");
			Trace("enable BLE comm response\r\n");
			g_bEnableBLEViewRes = true;
		}
	}
	else if(strcmp(argv[1], "disable") == 0) {
		if(strcmp(argv[2], "res") == 0) {
			Trace("\r\n");
			Trace("disable BLE comm response\r\n");
			g_bEnableBLEViewRes = false;
		}
	}
	else if(strcmp(argv[1], "enter") == 0) {
		Trace("\r\n");
		Trace("enter BLE direct interface...\r\n");
		g_bEnableBLEDirectCommunication = true;
	}
	else if(strcmp(argv[1], "init") == 0) {
		Trace("BLE: Init\r\n");
		BTSetState(eBT_initialized);
		BTGetLocalAddressReq();
	}
	else if(strcmp(argv[1], "aes") == 0) {
		if(strcmp(argv[2], "en") == 0) {
			aes256_context ctx;
//			U8 ucKey[32] = "RunGitautoDCSVCIAuthoritR9999995";
			U8 ucKey[32] = {0x03, 0x4E, 0x73, 0xF8, 0x57, 0xA9, 0xDB, 0xA0, 0xB6, 0x15, 0x70, 0x29, 0x98, 0xF3, 0xEF, 0xEC, 0x4E, 0x73, 0xF8, 0x57, 0xA9, 0xDB, 0xA0, 0xB6, 0x15, 0x70, 0x29, 0x98, 0xF3, 0xEF, 0xEC, 0x03};
//			U8 ucTempBuff[16]="AUTOLINKR9999995";
//			U8 ucTempBuff[16]={0x41,0x4c,0x42,0x4f,0x4f,0x54,0x46,0x57,0x58,0x33,0x00,0x00,0x18,0x00,-100,37};
			signed char ucTempBuff[16]={65,76,66,79,79,84,70,87,88,51,0,0,24,0,-100,37};
//			char ucTempBuff[16]={65,76,66,79,79,84,70,87,88,51,0,0,0,0,0,0};
//			memcpy(ucKey,g_ucRandomKey,sizeof(g_ucRandomKey));
			aes256_init(&ctx, ucKey);
			aes256_encrypt_ecb(&ctx, (unsigned char*)ucTempBuff);	//Inputbuf�� decrypt�ؼ� ����
			aes256_done(&ctx);

			for(int i=0;i<16;i++)	printf("%02X",ucTempBuff[i]);
			printf("\r\n");

			aes256_init(&ctx, ucKey);
			aes256_decrypt_ecb(&ctx, (unsigned char*)ucTempBuff);	//Inputbuf�� decrypt�ؼ� ����
			aes256_done(&ctx);

			for(int i=0;i<16;i++)	printf("%02X",ucTempBuff[i]);
			printf("\r\n");

		}
		else if(strcmp(argv[2], "de") == 0) {

		}
	}
	else {
		Trace("*unknow command...\r\n");
	}

	return;
}

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
static void usb_op_cmd (uint32_t argc, char *argv[])
{
	if(strcmp(argv[1], "set") == 0) {
		Trace("\r\n");
		Trace("usb: set usb class\r\n");

		if(strcmp(argv[2], "cdc") == 0) {
			Trace("usb: set usb class - cdc device\r\n");
			stSystemTestInfo.eUSBDeviceClassType = eUSB_DEVICE_CLASS_CDC;

			SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
		}
		else if(strcmp(argv[2], "msd") == 0) {
			Trace("usb: set usb class - msd device\r\n");
			stSystemTestInfo.eUSBDeviceClassType = eUSB_DEVICE_CLASS_MSD;

			SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
		}
		else if(strcmp(argv[2], "bulk") == 0) {
			Trace("usb: set usb class - bulk device\r\n");
			stSystemTestInfo.eUSBDeviceClassType = eUSB_DEVICE_CLASS_BULK;

			SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
		}
	}
	else if(strcmp(argv[1], "get") == 0) {
		if(strcmp(argv[2], "usbtype") == 0) {
			switch(stSystemTestInfo.eUSBDeviceClassType) {
				case eUSB_DEVICE_CLASS_CDC:
					Trace("usb: current usb class - cdc device\r\n");
					break;
				case eUSB_DEVICE_CLASS_MSD:
					Trace("usb: current usb class - msd device\r\n");
					break;
				case eUSB_DEVICE_CLASS_BULK:
					Trace("usb: current usb class - bulk device\r\n");
					break;
			}
		}
	}
	else if(strcmp(argv[1], "cdc") == 0 && strcmp(argv[2], "enable") == 0) {
		Trace("\r\n");
		Trace("Enable CDC output\r\n");
		gbEnableUSBCDC = true;
	}
	else if(strcmp(argv[1], "cdc") == 0 && strcmp(argv[2], "disable") == 0) {
		Trace("\r\n");
		Trace("Disable CDC output\r\n");
		gbEnableUSBCDC = false;
	}
	else if(strcmp(argv[1], "cdctest") == 0) {
		uint16_t i, j;

		Trace("\r\n");

		for(i = 0, j = 0; i < 100; i++) {
			VCP_DataTxByte((uint8_t) '0' + j);
			if(j >= 9) {
				j = 0;
			}
			else {
				j++;
			}
		}

		VCP_DataTxByte((uint8_t) '\r');
		VCP_DataTxByte((uint8_t) '\n');

		gbEnableUSBCDC = true;
		Trace("1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n");
		Trace("1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n");
		Trace("1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n");
		Trace("1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n");
		Trace("1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n\r\n");
		gbEnableUSBCDC = false;

		Trace("CDC Test\r\n\r\n");
	}
}
#endif

char *uintToBinary(uint32_t i)
{
  static char s[16 + 1] = { '0', };
  int32_t count = 16;

  do {
  	s[--count] = '0' + (char) (i & 1);
		i = i >> 1;
  } while (count);

  return s;
}

static void power_op_cmd (uint32_t argc, char *argv[])
{
	if(strcmp(argv[1], "set") == 0) {
	}
	else if(strcmp(argv[1], "get") == 0) {
		if(strcmp(argv[2], "sleepflag") == 0) {
			Trace("g_PowerFlagBits.nSleepReady_AllManager: %s\r\n", uintToBinary(g_PowerFlagBits.nSleepReady_AllManager));
			Trace("g_PowerFlagBits.bSleepReady_OBDManager: %s\r\n", (g_PowerFlagBits.bSleepReady_OBDManager == true) ? "true" : "false");
			Trace("g_PowerFlagBits.bSleepReady_MessageManager: %s\r\n", (g_PowerFlagBits.bSleepReady_MessageManager == true) ? "true" : "false");
		}
	}
	else if(strcmp(argv[1], "enable") == 0) {
		if(strcmp(argv[2], "sleepflag") == 0) {
			if(strcmp(argv[3], "obd") == 0) {
				g_PowerFlagBits.bSleepReady_OBDManager = true;
			}
			else if(strcmp(argv[3], "message") == 0) {
				g_PowerFlagBits.bSleepReady_MessageManager = true;
			}
			else if(strcmp(argv[3], "fota") == 0) {
			}
			else if(strcmp(argv[3], "all") == 0) {
				g_PowerFlagBits.nSleepReady_AllManager = (SLEEP_READY_OBD_MANAGER | SLEEP_READY_MESSAGE_MANAGER | SLEEP_READY_MODEM_MANAGER);
			}

			Trace("g_PowerFlagBits.nSleepReady_AllManager: 0b%s\r\n", uintToBinary(g_PowerFlagBits.nSleepReady_AllManager));
			Trace("g_PowerFlagBits.bSleepReady_OBDManager: %s\r\n", (g_PowerFlagBits.bSleepReady_OBDManager == true) ? "true" : "false");
			Trace("g_PowerFlagBits.bSleepReady_MessageManager: %s\r\n", (g_PowerFlagBits.bSleepReady_MessageManager == true) ? "true" : "false");
		}
	}
	else if(strcmp(argv[1], "disable") == 0) {
		if(strcmp(argv[2], "sleepflag") == 0) {
			if(strcmp(argv[3], "obd") == 0) {
				g_PowerFlagBits.bSleepReady_OBDManager = false;
			}
			else if(strcmp(argv[3], "message") == 0) {
				g_PowerFlagBits.bSleepReady_MessageManager = false;
			}
			else if(strcmp(argv[3], "all") == 0) {
				g_PowerFlagBits.nSleepReady_AllManager = false;
			}

			Trace("g_PowerFlagBits.nSleepReady_AllManager: 0b%s\r\n", uintToBinary(g_PowerFlagBits.nSleepReady_AllManager));
			Trace("g_PowerFlagBits.bSleepReady_OBDManager: %s\r\n", (g_PowerFlagBits.bSleepReady_OBDManager == true) ? "true" : "false");
			Trace("g_PowerFlagBits.bSleepReady_MessageManager: %s\r\n", (g_PowerFlagBits.bSleepReady_MessageManager == true) ? "true" : "false");
		}
	}
}

static void app_op_enable_cmd (uint32_t argc, char *argv[])
{
	Trace("%s \r\n", __FUNCTION__);
	Trace("argc=%u  argv[0]=%s  argv[1]=%s  argv[2]=%s \r\n", argc, argv[0], argv[1], argv[2] );

	return;
}

static void app_op_disable_cmd (uint32_t argc, char *argv[])
{
  Trace("%s \r\n", __FUNCTION__);
  Trace("argc=%u  argv[0]=%s  argv[1]=%s  argv[2]=%s \r\n", argc, argv[0], argv[1], argv[2] );

  return;
}

stMsgSys m_tmpStMsgSys;

extern unsigned int GetTimefromDate(stHalRTCTypeDef stRtcInfo);
extern void GetDatefromTime(stHalRTCTypeDef* stDateTime,unsigned int binary);

extern stAutoVin g_stAutoVin;
extern uint16_t ConversionMultiByteToHexStr_n2(uint8_t nRealNum,uint8_t *pMultiByte, uint16_t n, uint16_t *pnPos, uint8_t *ptrBuffer);

static void debug_op_cmd (uint32_t argc, char *argv[])
{

	/* // jkc_0230712_BEGIN -- protocol 25
	if(strcmp(argv[1],"test") == 0)
	{
        if(strcmp(argv[2],"1") == 0)
    	{
            Send2MngSysMsg(eMngModem, eReqReport, eR_ReportInstallationNetworkCheck, (stCarReport*)NULL ,0);
    	}
        else if(strcmp(argv[2],"2") == 0)
    	{
            Send2MngModem(eMngModem,eRcvSms,eR_ReportInstallationSMSCheck,(stCarReport*)NULL,0);
    	}
        else if(strcmp(argv[2],"3") == 0)
    	{
            Send2MngModem(eMngModem,eRcvSms,eR_SmsTest,(stCarReport*)NULL,0);
    	}
	}
	*/ // jkc_0230712_END -- protocol 25
    
	if(strcmp(argv[1],"rtc") == 0)
	{
		extern void ConfigforAlram(boolean_t bWakeupSoon, boolean_t bReqLongSleep);
		ConfigforAlram(false, false);
	}

	if(strcmp(argv[1],"btserial") == 0)
	{
		uint8_t ucArrBuffer[64] = {0,};
		uint8_t ucArrBuffer2[64] = {0,};
		for(int i=0;i<16;i++)
		{
			ucArrBuffer[i] = 0x10+i;
		}

		int nLength = ConversionMultiByteToHexStr_n2(16,ucArrBuffer, 16, 0, ucArrBuffer2);

		printf("nLength : %d\r\n",nLength);
		hexdump(ucArrBuffer2,32 );

	}
	/* // jkc_0230620_BEGIN -- BT ��� ���� ����
	else if(strcmp(argv[1], "mdm") == 0)
	{
		if(strcmp(argv[2], "w")== 0)
		{	
			if(strcmp(argv[3], "d")== 0)
			{
				//printf("modem log File write MODEM_DENIED\r\n");
				WriteAutoLinkModemsStatusData(MODEM_DENIED);
			}
			else if(strcmp(argv[3], "n")== 0)
			{
				printf("modem log File write MODEM_NOT_COMMUNICATION\r\n");
				WriteAutoLinkModemsStatusData(MODEM_NOT_COMMUNICATION);
			}
			else if(strcmp(argv[3], "c")== 0)
			{
				printf("modem log File write MODEM_CME_ERROR\r\n");
				WriteAutoLinkModemsStatusData(MODEM_CME_ERROR);
			}
		}
		else if(strcmp(argv[2], "r")==0)
		{
			char carrPath[20]={0,};
			sprintf(carrPath,"/%s\x00",AUTOLINKDATA_BASE_DIR);

			ShowDir((char *)carrPath);
		}
 		else if(strcmp(argv[2], "d")==0)
		{
			DeleteAutolinkModemStatusData(MODEM_DENIED);
			DeleteAutolinkModemStatusData(MODEM_NOT_COMMUNICATION);
			DeleteAutolinkModemStatusData(MODEM_CME_ERROR);
		}
	}
	*/ // jkc_0230620_END -- BT ��� ���� ����
	if(strcmp(argv[1], "set") == 0) {
        if(strcmp(argv[2], "obd") == 0) {
			//ex) dbg set obd engon
			if(strcmp(argv[3], "engon") == 0) {
				g_OBDControllerData.monitering_Data.m_ucACC = 1;
				g_OBDControllerData.monitering_Data.m_bIG1 = 1;
				g_OBDControllerData.monitering_Data.m_bIG2 = 1;
				Send_RPM_From_CAN(9999);

                g_dSavedGpsLat = 9999.999999;
                g_dSavedGpsLon = 99999.999999;
                g_GPSInfo.lat = 9999.999999;
                g_GPSInfo.lon = 999999.999999;
            }
			if(strcmp(argv[3], "engoff") == 0) {
				g_OBDControllerData.monitering_Data.m_ucACC = 0;
				g_OBDControllerData.monitering_Data.m_bIG1 = 0;
				g_OBDControllerData.monitering_Data.m_bIG2 = 0;
				Send_RPM_From_CAN(0);
            }
            if(strcmp(argv[3], "on") == 0) {

                ResponseSmartKeyResult(601,1,0);

                Making_Drivingkey();

                ReportBeforeDriving();
                GITDebugPrintf("%s] send alram commdand\r\n",__FUNCTION__);
            }
			if(strcmp(argv[3], "igon") == 0) {
				Send_IG1_From_CAN(1);
				Send_IG2_From_CAN(1);

                g_dSavedGpsLat = 9999.999999;
                g_dSavedGpsLon = 99999.999999;
                g_GPSInfo.lat = 9999.999999;
                g_GPSInfo.lon = 999999.999999;
            }
			if(strcmp(argv[3], "igoff") == 0) {
				Send_IG1_From_CAN(0);
				Send_IG2_From_CAN(0);

                g_dSavedGpsLat = 9999.999999;
                g_dSavedGpsLon = 99999.999999;
                g_GPSInfo.lat = 9999.999999;
                g_GPSInfo.lon = 999999.999999;
            }
        }
        if(strcmp(argv[2], "mode") == 0) {
            if(strcmp(argv[3], "none") == 0) {
                SetSysDebugMode(DEBUG_MODE_NONE);
            }
            else if(strcmp(argv[3], "all") == 0) {
                SetSysDebugMode(DEBUG_MODE_ALL);
            }
            else if(strcmp(argv[3], "level") == 0) {
                SetSysDebugMode(DEBUG_MODE_LEVEL);
            }
            else if(strcmp(argv[3], "module") == 0) {
                SetSysDebugMode(DEBUG_MODE_MODULES);
            }
        }

        if(strcmp(argv[2], "module") == 0) {
			//ex) dbg set module XXX
			SetSysDebugMode(DEBUG_MODE_MODULES);
			SetSysDebugModule(DEBUG_MODULES_CLI,true);
			if(strcmp(argv[3], "none") == 0)			SetSysDebugModule(DEBUG_MODULES_NONE,true);
			else if(strcmp(argv[3], "obd") == 0)		SetSysDebugModule(DEBUG_MODULES_OBD,true);
			else if(strcmp(argv[3], "power") == 0)		SetSysDebugModule(DEBUG_MODULES_POWER,true);
			else if(strcmp(argv[3], "modem") == 0)		SetSysDebugModule(DEBUG_MODULES_MODEM,true);
			else if(strcmp(argv[3], "period") == 0)		SetSysDebugModule(DEBUG_MODULES_OBD_PERIOD_LOG,true);
			else if(strcmp(argv[3], "parsing") == 0)	SetSysDebugModule(DEBUG_MODULES_OBD_DBPARSING_LOG,true);
			else if(strcmp(argv[3], "act") == 0)		SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_LOG,true);
			else if(strcmp(argv[3], "actparsing") == 0)	SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_PARSING_LOG,true);
			else if(strcmp(argv[3], "sysmsg") == 0)		SetSysDebugModule(DEBUG_MODULES_OBD_SYSTEM_MSG,true);
			else if(strcmp(argv[3], "modemc") == 0)		SetSysDebugModule(DEBUG_MODULES_MODEM_COM,true);
			else if(strcmp(argv[3], "util") == 0)		SetSysDebugModule(DEBUG_MODULES_UTIL,true);
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
			else if(strcmp(argv[3], "usb") == 0)		SetSysDebugModule(DEBUG_MODULES_USB,true);
#endif
			else if(strcmp(argv[3], "handler") == 0)	SetSysDebugModule(DEBUG_MODULES_SYSTEM_HADNLER,true);
			else if(strcmp(argv[3], "system") == 0)		SetSysDebugModule(DEBUG_MODULES_SYSTEM,true);
			else if(strcmp(argv[3], "storage") == 0)	SetSysDebugModule(DEBUG_MODULES_STORAGE,true);
			else if(strcmp(argv[3], "assert") == 0)		SetSysDebugModule(DEBUG_MODULES_ASSERT,true);
			else if(strcmp(argv[3], "app") == 0)		SetSysDebugModule(DEBUG_MODULES_APP,true);
			else if(strcmp(argv[3], "file") == 0)		SetSysDebugModule(DEBUG_MODULES_FILE_SYSTEM,true);
			else if(strcmp(argv[3], "gps") == 0)		SetSysDebugModule(DEBUG_MODULES_GPS,true);
			else if(strcmp(argv[3], "ble") == 0)		SetSysDebugModule(DEBUG_MODULES_BLE,true);
			else if(strcmp(argv[3], "queue") == 0)		SetSysDebugModule(DEBUG_MODULES_QUEUE,true);
			else if(strcmp(argv[3], "basic") == 0)
			{
				SetSysDebugMode(DEBUG_MODE_MODULES);
				SetSysDebugModule(DEBUG_MODULES_OBD,true);
				SetSysDebugModule(DEBUG_MODULES_POWER,true);
				SetSysDebugModule(DEBUG_MODULES_MODEM,true);
				SetSysDebugModule(DEBUG_MODULES_OBD_PERIOD_LOG,false);
				SetSysDebugModule(DEBUG_MODULES_OBD_DBPARSING_LOG,false);
				SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_LOG,true);
				SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_PARSING_LOG,false);
				SetSysDebugModule(DEBUG_MODULES_OBD_SYSTEM_MSG,true);
				SetSysDebugModule(DEBUG_MODULES_MODEM_COM,true);
				SetSysDebugModule(DEBUG_MODULES_UTIL,true);
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
				SetSysDebugModule(DEBUG_MODULES_USB,false);
#endif
                SetSysDebugModule(DEBUG_MODULES_OBD_CANFD,true);

				SetSysDebugModule(DEBUG_MODULES_SYSTEM_HADNLER,true);
				SetSysDebugModule(DEBUG_MODULES_SYSTEM,true);
				SetSysDebugModule(DEBUG_MODULES_STORAGE,true);
				SetSysDebugModule(DEBUG_MODULES_ASSERT,true);
				SetSysDebugModule(DEBUG_MODULES_APP,true);
				SetSysDebugModule(DEBUG_MODULES_FILE_SYSTEM,false);
				SetSysDebugModule(DEBUG_MODULES_GPS,false);
				SetSysDebugModule(DEBUG_MODULES_CLI,true);
				SetSysDebugModule(DEBUG_MODULES_BLE,false);
				SetSysDebugModule(DEBUG_MODULES_QUEUE,true);
                SetSysDebugModule(DEBUG_MODULES_OBD_CANFD,true);
			}
			else if(strcmp(argv[3], "stop") == 0)
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
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
				SetSysDebugModule(DEBUG_MODULES_USB,false);
#endif
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
                SetSysDebugModule(DEBUG_MODULES_OBD_CANFD,false);
			}
			else if(strcmp(argv[3], "all") == 0)
			{
				SetSysDebugModule(DEBUG_MODULES_OBD,true);
				SetSysDebugModule(DEBUG_MODULES_POWER,true);
				SetSysDebugModule(DEBUG_MODULES_MODEM,true);
				SetSysDebugModule(DEBUG_MODULES_OBD_PERIOD_LOG,true);
				SetSysDebugModule(DEBUG_MODULES_OBD_DBPARSING_LOG,true);
				SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_LOG,true);
				SetSysDebugModule(DEBUG_MODULES_OBD_ACTUATOR_PARSING_LOG,true);
				SetSysDebugModule(DEBUG_MODULES_OBD_SYSTEM_MSG,true);
				SetSysDebugModule(DEBUG_MODULES_MODEM_COM,true);
				SetSysDebugModule(DEBUG_MODULES_UTIL,true);
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
				SetSysDebugModule(DEBUG_MODULES_USB,true);
#endif
				SetSysDebugModule(DEBUG_MODULES_SYSTEM_HADNLER,true);
				SetSysDebugModule(DEBUG_MODULES_SYSTEM,true);
				SetSysDebugModule(DEBUG_MODULES_STORAGE,true);
				SetSysDebugModule(DEBUG_MODULES_ASSERT,true);
				SetSysDebugModule(DEBUG_MODULES_APP,true);
				SetSysDebugModule(DEBUG_MODULES_FILE_SYSTEM,true);
				SetSysDebugModule(DEBUG_MODULES_GPS,true);
				SetSysDebugModule(DEBUG_MODULES_CLI,true);
				SetSysDebugModule(DEBUG_MODULES_BLE,true);
				SetSysDebugModule(DEBUG_MODULES_QUEUE,true);
                SetSysDebugModule(DEBUG_MODULES_OBD_CANFD,true);
			}
			printf("set\r\n");
        }
	}

    if(strcmp(argv[1], "mng") == 0)
    {
        if(strcmp(argv[2], "idle") == 0)
        {
            ModemManagerData.eState = eMODEM_NONE;
            ModemManagerData.bNeedModemHWResetFlag = false;
            ModemManagerData.bRunWorkaroundFlag = false;
            ModemManagerData.bCheckNetworkStatusFlag = false;
            ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;
            ModemManagerData.bRequestMessageCommFlag = false;
            ModemManagerData.bAvailableModemCommFlag = false;

            //m_nMngMsgMdmState = STAT_MNG_MDM_IDLE;

            Send2MngSys(eMngSys,eReqChangeMode,0,(stCarReport*)NULL,0);
        }
    }

    if(strcmp(argv[1], "gpio") == 0)
    {
        if(strcmp(argv[2], "0") == 0)
            HalGPIOSetVaule(GPIO_GPS_PWEN, eBIT_SET);
        if(strcmp(argv[2], "1") == 0)
            HalGPIOSetVaule(GPIO_GPS_PWEN, eBIT_RESET);
        if(strcmp(argv[2], "2") == 0)
            SetupInterruptLatch(false);
        if(strcmp(argv[2], "3") == 0)
            SetupInterruptLatch(true);
    }

    if(strcmp(argv[1], "autosave") == 0)
    {
        if(strcmp(argv[2], "start") == 0)
        {
            Send2MngSysSub(eMngSys, eReqSaveReport,0, (stCarReport *)NULL,0);
        }
        else if(strcmp(argv[2], "stop") == 0)
        {
            Send2MngSysSub(eMngSys, eRspSaveReport,0, (stCarReport *)NULL,0);
        }
    }

    if(strcmp(argv[1], "alram") == 0)
    {
        if(strcmp(argv[2], "0") == 0)
        {
            stGeofenceUnit stCurrentUnit;
            stCurrentUnit.GpsLatitude = Get_GPS_Lat();
            stCurrentUnit.GpsLongitude = Get_GPS_Lon();
            stGeofenceUnit stSettingUnit;

            SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM, 1, &stCurrentUnit, &stSettingUnit,eGFT_POLYGON,0x11223344);
//            ReportAlramStatus();
        }
        if(strcmp(argv[2], "1") == 0)
        {
            stGeofenceUnit stCurrentUnit;
            stCurrentUnit.GpsLatitude = Get_GPS_Lat();
            stCurrentUnit.GpsLongitude = Get_GPS_Lon();
            stGeofenceUnit stSettingUnit;

            SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_VALET_ALRAM, 1, &stCurrentUnit, &stSettingUnit,eGFT_VALET,0x11223344);
            //ReportAlramStatus(eMESSAGE_EVENT_KEY_PARKING_IMPACT , 99.5*10);
        }
        if(strcmp(argv[2], "2") == 0)
        {
            ReportAlramStatus(eMESSAGE_EVENT_KEY_DOOR_LOCK_ALARM , 1);
        }
        if(strcmp(argv[2], "3") == 0)
        {
            ReportAlramStatus(eMESSAGE_EVENT_KEY_OVER_ENGINE_TEMP_ALARM, 999);
        }
        if(strcmp(argv[2], "4") == 0)
        {
            ReportAlramStatus(eMESSAGE_EVENT_KEY_PARKING_IMPACT , 0x9);
        }
        if(strcmp(argv[2], "5") == 0)
        {
            ReportAlramStatus(eMESSAGE_EVENT_KEY_MIL_LAMP_ON , 0x9);
        }
		if(strcmp(argv[2], "55") == 0)
        {
            ReportAlramStatus(eMESSAGE_EVENT_KEY_SECURITY_ALRAM , 0x9);
        }
		if(strcmp(argv[2], "555") == 0)
		{
			Send_HornStatus_From_CAN(true);
		}
		if(strcmp(argv[2], "5555") == 0)
        {
        	for(int k=0;k<10;k++)
        	{
            	ReportAlramStatus(eMESSAGE_EVENT_KEY_MIL_LAMP_ON , k);
        	}
        }
		if((strcmp(argv[2], "666") == 0))
		{
			BTStartAdvertisingReq();
		}
        if(strcmp(argv[2], "6") == 0)
        {
            // send after driving
            Send2MngSysMsg(eMngObd, eReqReport, eR_AfterDriving, (stCarReport *)NULL,eMngObd);
        }
        if(strcmp(argv[2], "7") == 0)
        {
            ReportAlramStatus(eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM , 0x9);
        }
        if(strcmp(argv[2], "8") == 0)
        {
            stGeofenceUnit stCurrentUnit;
            stGeofenceUnit stSettingUnit;

            stCurrentUnit.GpsLatitude = -13.3834;
            stCurrentUnit.GpsLongitude = 150.20394;
            stSettingUnit.GpsLatitude = -13.3834;
            stSettingUnit.GpsLongitude = 150.19384;

            SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM, 1, &stCurrentUnit, &stSettingUnit, eGFT_CIRCLE, 0);
        }
        if(strcmp(argv[2], "9") == 0)
        {
            stGeofenceUnit stCurrentUnit;
            stGeofenceUnit stSettingUnit;

            stCurrentUnit.GpsLatitude = -13.3834;
            stCurrentUnit.GpsLongitude = 150.20394;
            stSettingUnit.GpsLatitude = -13.3834;
            stSettingUnit.GpsLongitude = 150.19384;

            SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_VALET_ALRAM, 1, &stCurrentUnit, &stSettingUnit,eGFT_CIRCLE,0);
        }
		if(strcmp(argv[2], "A") == 0)
        {
            ReportAlramStatus(eMESSAGE_EVENT_KEY_INDICATOR_ALRAM , 0x9);
        }
		if(strcmp(argv[2], "B") == 0)
        {
			SendFotaAlramReport(eMESSAGE_EVENT_KEY_FOTA_COMPLETE_ALRAM,eREMOTE_CON_FOTA_TYPE_TEST);
        }
#ifdef PROTOCOL18
		if(strcmp(argv[2], "23") == 0)
        {
			ReportAlramStatus(eMESSAGE_EVENT_KEY_REARSEAT_ALRAM,1);
        }
#endif
    }

    if( strcmp(argv[1],"hit") == 0 )
    {

        printf("#####################################################################\r\n");
        printf("Impluse Test Rcv Value : %x\r\n",atoi(argv[2]));
        printf("This register holds the threshold value\r\n");
        printf("for the Wake on Motion Interrupt for accel x/y/z axes.\r\n");
        printf("LSB = 4mg. Range is 0mg to 1020mg.\r\n");
        SetupForInterruptforImpulse(true,true, (uint8_t)atoi(argv[2]));
        ReadInterruptStatus();
    }

	if( strcmp(argv[1],"url") == 0 )
    {
		if( strcmp(argv[2],"1") == 0 )
		{
        	printf("URL SET 1\r\n");
			stMsgSysMsg stMessage;
//			unsigned char ucUrl[]="https://devautolinkapi-premium.hmca.com.au";
			//unsigned char ucUrl[]="https://devautolink.gitauto.com/AutoLink_TD/ServerAPI";
            const char ucUrl[]="https://devautolink.gitauto.com/AutoLink_TD/ServerAPI";
            stMessage.header.id = eMngModem;
            stMessage.header.event = eReqSysSetting;
            stMessage.header.subEvent = eR_ReqSetURLSave;
			stMessage.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
			stMessage.carReport.rpSetting.UserSetting.stUserActionSetting.SetUrl.unEventTime = GetLocalTimefromTime(GetUTCTime());
            strncpy(g_stTempURLInfo.strUrl,ucUrl,sizeof(ucUrl));
            Send2MngSysMsg3(&stMessage);
		}
		else if( strcmp(argv[2],"2") == 0 )
		{
        	printf("URL SET 2\r\n");
			stMsgSysMsg stMessage;
			unsigned char ucTmp[MAX_GUID_LENGTH];
			memset(ucTmp, 0x31, MAX_GUID_LENGTH);

            stMessage.header.id = eMngModem;
            stMessage.header.event = eReqSysSetting;
            stMessage.header.subEvent = eR_ReqSetURLInit;
			stMessage.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
			memcpy(stMessage.carReport.rpSetting.UserSetting.RequestGUID,ucTmp,sizeof(MAX_GUID_LENGTH));
            stMessage.carReport.rpSetting.UserSetting.stUserActionSetting.SetUrl.unEventTime = GetLocalTimefromTime(GetUTCTime());
            Send2MngSysMsg3(&stMessage);
		}
		else if( strcmp(argv[2],"get") == 0 )
		{
        	printf("URL SET get\r\n");
			unsigned char ucUrl[MAX_SERVER_URL_LENGTH+8+1]={0,};

			memcpy(ucUrl,g_stServerUrl.Url,sizeof(ucUrl));
			ucUrl[MAX_SERVER_URL_LENGTH+8]=0;
            printf("%s",ucUrl);
		}
		else
		{
			WriteURL(argv[2]);
			SystemForcelyReset();
			while(1);
		}
    }

    extern bool m_bRecoveryMessage;
    extern stMsgMdm m_stRecoveryMessage;

    extern void WriteModemConfiguration(boolean_t bRecoveryMessage, stMsgMdm stRecoveryMessage);
    extern void ReadModemConfiguration(boolean_t* pbRecoveryMessage, stMsgMdm* pstRevoeryMessage);
    extern void ConvertLocal2UtcTime(uint32_t unLocalTime, uint32_t* punUtcTime);


    if( strcmp(argv[1], "cg") == 0 )
    {
        if( strcmp(argv[2],"1") == 0)
        {
            TestFormat();
        }
        if( strcmp(argv[2],"2") == 0)
        {
            WriteConfig(true, true);
            ReadConfig(true);
        }
        if( strcmp(argv[2],"3") == 0)
        {
            ReadConfig(true);
        }
        if( strcmp(argv[2],"4") == 0)
        {
            ReadModemConfiguration((boolean_t *)&m_bRecoveryMessage,&m_stRecoveryMessage);
        }
        if( strcmp(argv[2],"5") == 0)
        {
            stMsgMdm stMedemMessage;
            stMedemMessage.header.id = eMngSysMsg;
            stMedemMessage.header.event = eReqReport;
            stMedemMessage.header.subEvent = eR_Alram;
            stMedemMessage.header.seq = 0;
            stMedemMessage.header.unTraceMng = eMngSysMsg;
            Send2MngModem2(&stMedemMessage);

            stMedemMessage.header.id = eMngSysMsg;
            stMedemMessage.header.event = eReqReport;
            stMedemMessage.header.subEvent = eR_DrivingInterval;
            stMedemMessage.header.seq = 1;
            Send2MngModem2(&stMedemMessage);

#ifndef GLOBAL_SHARE_QUEUE
            MngQueueSendMessage(ID_MNG_QUEUE_DATA, (int8_t*)&stMedemMessage,sizeof(stMsgMdm));
#else
	        SendSysHdShareQueueMessage(ID_MNG_QUEUE_DATA,(uint8_t*)&stMedemMessage.header,sizeof(stMsgHeader), (uint8_t*)&stMedemMessage.carReport, sizeof(stCarReport));
#endif

            stMedemMessage.header.id = eMngSysMsg;
            stMedemMessage.header.event = eReqReport;
            stMedemMessage.header.subEvent = eR_AfterDriving;
            stMedemMessage.header.seq = 2;
            Send2MngModem2(&stMedemMessage);

#ifndef GLOBAL_SHARE_QUEUE
            MngQueueSendMessage(ID_MNG_QUEUE_DATA, (int8_t*)&stMedemMessage,sizeof(stMsgMdm));
#else
	        SendSysHdShareQueueMessage(ID_MNG_QUEUE_DATA,(uint8_t*)&stMedemMessage.header,sizeof(stMsgHeader), (uint8_t*)&stMedemMessage.carReport, sizeof(stCarReport));
#endif

            WriteModemConfiguration(m_bRecoveryMessage,m_stRecoveryMessage);
        }
        if( strcmp(argv[2],"6") == 0)
        {
            WriteModemConfiguration(m_bRecoveryMessage,m_stRecoveryMessage);
        }
    }

    if( strcmp(argv[1], "ac") == 0 )
    {
        if( strcmp(argv[2], "1") == 0 )
        {
            uint8_t bModemActive=0;
            GetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);
            Trace("Get modem active : %d\r\n",bModemActive);
        }
        if( strcmp(argv[2], "2") == 0 )
        {
            uint8_t bModemActive = atoi(argv[3]);
            Trace("Set modem active : %d\r\n",bModemActive);
            SetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);
        }
        if( strcmp(argv[2], "3") == 0 )
        {
            WriteConfig(true, true);
        }
        if( strcmp(argv[2], "4") == 0 )
        {
            ReadConfig(true);
        }
        if( strcmp(argv[2], "5") == 0 )
        {
            WriteDefaultAutolinkConfigValue();
        }
        if( strcmp(argv[2], "6") == 0 )
        {
            DisplayAutolinkConfigData();
            DisplayBackupRamConfigData();
        }
        if( strcmp(argv[2], "7") == 0 )
        {
            char strVin[17]={0};

            sprintf(strVin,"%s\x00",STR_DEFAULT_DEV_VIN);
            //MONI 2018-02-21
            // obd recevied new vin number from car
            // notify to system to control of trasfer or scenario
            stMsgSys stMessage;
            memset((char*)&stMessage,0,sizeof(stMsgSys));

            stMessage.header.id = eMngObd;
            stMessage.header.event = eOBDStatus;
            stMessage.header.subEvent = eNewVin;
            memcpy(stMessage.rpSetting.ObdSetting.carrVin,strVin,sizeof(strVin));
            Send2MngSys2(&stMessage);
        }
        if( strcmp(argv[2], "8") == 0 )
        {
            uint8_t cAllowNewVin = atoi(argv[3]);

            SetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,(void*)&cAllowNewVin);
        }
        if( strcmp(argv[2], "9") == 0 )
        {
            // clear configuration all data
        }
        if( strcmp(argv[2],"10") == 0 )
        {
            if( strcmp(argv[3],"0") == 0 )
            {
                char carrVin[17]={0};
                memcpy(carrVin,STR_DEFAULT_VIN,sizeof(STR_DEFAULT_VIN));
                SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrVin);
            }
            else if( strcmp(argv[3],"1") == 0 )
            {
                char carrVin[17]={0};
                memcpy(carrVin,DEFAULT_TEST_VIN_7,sizeof(DEFAULT_TEST_VIN_7));
                SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrVin);
            }
            else if( strcmp(argv[3],"2") == 0 )
            {
                char* carrVin="KM12345679!";
                SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrVin);
            }
            else if( strcmp(argv[3],"3") == 0 )
            {
                char* carrVin="KMTG241ABJU005615";
                SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrVin);
            }
        }
        if(strcmp(argv[2],"time") == 0 )
        {
            // time stamp check
            stHalRTCTypeDef stHalRtcDateTime;
			stNetworkTime stNetworkDate;
			unsigned int uiLocalTime=0,uiUTCTime=0;
            
            HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRTCTypeDef), 0);

            printf(" Message Occurred Time_Local: %04d/%02d/%02d,%02d:%02d:%02d\r\n", 
                                                    stHalRtcDateTime.RtcDate.RTC_Year+2000,
                                                    stHalRtcDateTime.RtcDate.RTC_Month,
                                                    stHalRtcDateTime.RtcDate.RTC_Date,
                                                    stHalRtcDateTime.RtcTime.RTC_Hours,
                                                    stHalRtcDateTime.RtcTime.RTC_Minutes,
                                                    stHalRtcDateTime.RtcTime.RTC_Seconds);

            uiLocalTime = GetTimefromDate(stHalRtcDateTime);
			GetAutolinkConfigProperty(eAutoLinkConfig_NetworkInfo,(void*)&stNetworkDate);
			printf("uiLocalTime : %d,Timezone:%d\r\n",uiLocalTime,stNetworkDate.sTimeZone);

            memset((char*)&stHalRtcDateTime,0,sizeof(stHalRtcDateTime));

			ConvertLocal2UtcTime(uiLocalTime, &uiUTCTime);

            GetDatefromTime(&stHalRtcDateTime,uiUTCTime);
            printf(" Message Occurred Time_UTC: %04d/%02d/%02d,%02d:%02d:%02d\r\n", 
                                                    stHalRtcDateTime.RtcDate.RTC_Year+2000,
                                                    stHalRtcDateTime.RtcDate.RTC_Month,
                                                    stHalRtcDateTime.RtcDate.RTC_Date,
                                                    stHalRtcDateTime.RtcTime.RTC_Hours,
                                                    stHalRtcDateTime.RtcTime.RTC_Minutes,
                                                    stHalRtcDateTime.RtcTime.RTC_Seconds);
        }

        if( strcmp(argv[2],"fm") == 0 )
        {
            TestFormat();
        }
        if( strcmp(argv[2], "vin") == 0 )
        {
            //KMTG341ADJU001645
            //KMTG541EDJU000488
            //"KMTG241ABJU005612"
            //KONA "KMHK3815WJU009119"

            if( strcmp(argv[3], "test") == 0 )
            {
                //SetVINCode("KMHC051HFHU010637");// i got vin from sangjun for ionic
                //SetVINCode("KMHH551CVJU031520");// i got vin from sangjun for G80
                //SetVINCode("KMHJ281ABHU448177");// i got vin from sangjun for G80
                //SetVINCode("KMHH351EMJU059065");// i got vin from sangjun for G80
                //SetVINCode("KMHJ281ABHU448177");// 2241 // tl key
                //SetVINCode("KMHJ581ABHU450564");// 5106 // tl smart key
                SetVINCode("KMHS281CDKU007487");// tm

                //SetVINCode((U8 *)argv[3]);
                printf("set vin number : %s\r\n",argv[4]);

                SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)argv[4]);
            }
            else if( strcmp(argv[3], "fota") == 0 )
            {
                //SetVINCode("KMTG241ABJU005613");// i got vin from sangjun for G80
                //SetVINCode("KMHS381CSKU026040");// i got tm vin
                SetVINCode((U8*)argv[4]);// ik

                //SetVINCode((U8 *)argv[3]);
                printf("set vin number : %s\r\n",argv[4]);

                SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)argv[4]);
            }
            else if( strcmp(argv[3], "def") == 0 )
            {
		        SetVINCode(STR_DEFAULT_VIN);
            }
            else if( strcmp(argv[3], "clear") == 0 )
            {
                SetVINCode(STR_DEFAULT_VIN);// i got vin from sangjun for G80

                uint8_t carrVin[64]={0,};
                GetVINCode(carrVin);

                SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrVin);
            }
        }
        if( strcmp(argv[2], "rvin") == 0 )
        {
            uint8_t carrVin[64]={0,};
            GetVINCode(carrVin);

            SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrVin);
        }
        if( strcmp(argv[2], "serial") == 0 )
        {
            if( strcmp(argv[3], "default") == 0 )
            {
                SetFWSerialNumber(DEFAULT_SERIAL_NUMBER);
            }
            else
            {
                SetFWSerialNumber(argv[3]);
                printf("set serial number : %s\r\n",argv[3]);
            }
        }
/*        if( strcmp(argv[2], "test") == 0 )
        {
            char buf[256];
            sprintf(buf,"R,%04d,%04d,%04d,%02d.%02d,%02d.%02d\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF);

            printf(" f/w info : %s\n",buf);
        }*/
        if( strcmp(argv[2], "penta") == 0)
        {
            TestSetSerialAndIpektoInternaFlash(atoi(argv[3]));
        }
        if( strcmp(argv[2], "dtc") == 0)
        {
//            stCarReport stReport;
//            memset((char*)&stReport,0,sizeof(stCarReport));
//            if( GetRequestDtcAlram() == false )
//            {
//                // self check report
//                stReport.rpAlram.Dtc.ResponseType = 1;
//            }
//            else
//            {
//                // request from server
//                stReport.rpAlram.Dtc.ResponseType = 2;
//            }
//            stReport.rpAlram.Dtc.OccurredEventTime = GetLocalTime();
//            stReport.rpAlram.Dtc.OccurredEventUtcTime = GetUtcTimefromTime(GetLocalTime());
//            stReport.rpAlram.Dtc.OperationKey = Get_DrivingKey();
//
//            Send2MngSysMsg2(eMngObd,eReqReport, eR_AlramDTC,0,&stReport,0);
        }
		if( strcmp(argv[2], "utc") == 0 )
        {
            DisplayTime("Local", GetLocalTimefromTime(GetUTCTime()));
            DisplayTime("Utc", GetUTCTime());
        }
        if( strcmp(argv[2], "air") == 0 )
        {
            if( Md_SetFlightMode("4,0") == true) {
            }
        }
    }
    else if( strcmp(argv[1], "ipek") == 0 )
    {
        if( strcmp(argv[2], "0") == 0 )
        {
            // request ipek phase #1
            stCarReport report;
            memset((char*)&report,0,sizeof(report));
            Trace("request ipek serial : %s\r\n",argv[3]);
            //memcpy(report.buffer,argv[3],11);
            //memcpy(report.buffer,"R9999992000",11);
            SetFWSerialNumber(argv[3]);

            Send2MngSysMsg(eMngSys,eReqIpek,eIpekPhase1,(stCarReport *)&report,0);
        }
        else if( strcmp(argv[2], "1") == 0 )
        {
            // Clear ipek in internal flash
            unsigned char ucEncryptionKey[32];			// out
            memset((char *)ucEncryptionKey,0x00,sizeof(ucEncryptionKey));
            SetModuleKey((char *)ucEncryptionKey);

            ClearFutureKey();
        }
        else if( strcmp(argv[2], "2") == 0 )
        {
            //uint32_t size;
            //GetDecryptPublicKey(g_KMS_Publickey,&size);
        }
    }
    if( strcmp(argv[1], "mp") == 0 )
    {
        if( strcmp(argv[2], "0") == 0 )
        {
            //ModemManagerData.eState = eMODEM_ENTER_POWER_OFF;
            Send2MngSysMsg2(eMngSys,eReqModemPowerOff,0,eTrue,(stCarReport *)NULL,0);
        }
        else if( strcmp(argv[2], "1") == 0 )
        {
            PowerOnGemaltoModem();
        }
    }
    if( strcmp(argv[1],"data") == 0 )
    {
        if( strcmp(argv[2], "0" ) == 0 )
        {
            ReportDriving();
        }
        else if( strcmp(argv[2], "1" ) == 0 )
        {
            ReportNoDriving();
        }
    }
    if( strcmp(argv[1],"dtc") == 0 )
    {
        stCarReport stReport;
        //for(int i=0;i<3;i++)
        {
            stReport.rpAlram.Dtc.Index = 0;
            Send2MngSysMsg2(eMngObd,eReqReport, eR_AlramDTC,0,&stReport,0);
        }
    }
    if( strcmp(argv[1],"reset") == 0 )
    {
		SystemForcelyReset();
		while(1);
//        Git_DeviceReset(NULL,0);
    }
    if( strcmp(argv[1],"reset2") == 0 )
    {
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
        HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
        HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
        APP_Delay(100);

        HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
        APP_Delay(50);

		HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
        APP_Delay(100);

        HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
        APP_Delay(50);
        HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
        //APP_Delay(50);
        HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#endif
    }
	if( strcmp(argv[1],"reset3") == 0 )
    {
		ModemReset_Install();
	}
    if( strcmp(argv[1],"getinfo") == 0 )
    {
        Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
    }
    if( strcmp(argv[1],"svct") == 0 )
    {
        if( strcmp(argv[2],"retail") == 0 )
        {
            DCSServiceType svcType = DCS_Retail;
            SetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&svcType);
            //SetServiceType(DCS_Retail);
        }
        else if( strcmp(argv[2],"fleet") == 0 )
        {
            DCSServiceType svcType = DCS_Fleet;
            SetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&svcType);
            //SetServiceType(DCS_Fleet);
        }
		else if( strcmp(argv[2],"get") == 0 )
        {
            DCSServiceType svcType = (DCSServiceType)0x00;
            GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&svcType);
			printf("svcType : %02X\r\n",svcType);
        }
    }
    if( strcmp(argv[1],"enct") == 0 )
    {
        if( strcmp(argv[2],"aes") == 0 )
        {
            SetEncryptType(DCS_ENC_AES);
        }
        else if( strcmp(argv[2],"kms") == 0 )
        {
            SetEncryptType(DCS_ENC_KMS);
        }
		else if( strcmp(argv[2],"get") == 0 )
        {
            printf("GetEncryptType : %02X(0xB0:AES,0xB1:KMS)\r\n",GetEncryptType());
        }
		else
		{
		  	aes256_context ctx;
			uint8_t ucBluetoothNewKey[32] = {0x14,0x7b,0xfc,0x1a,0xde,0x48,0x1f,0x99,0x93,0xee,0x2c,0x30,0x68,0x73,0x34,0x8a,0xc0,0x86,0xee,0x7f,0x5f,0xd4,0x31,0x5b,0xb2,0xa2,0xeb,0xc7,0x6f,0x99,0xb0,0xc2};
			uint8_t ucEncDecData[128]={0,};
			uint8_t ucBase64EncDecData[128]={0x41,0x67,0x4b,0x41,0x41,0x41,0x49,0x50,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x44,0x74};
			//uint8_t ucBase64EncDecDataLength = 44; mod.kks todo remove
			//uint8_t ucDataIndex = 0; mod.kks todo remove
			uint8_t pucDst[128];
			uint8_t ucSaveSize = 0;

			// copy encrypted data from application
			memcpy(ucEncDecData,&ucBase64EncDecData[0],44);  // mod.kks todo remove  ucBase64EncDecDataLength);
			ucSaveSize = strlen((char const*)ucEncDecData);

			aes256_init(&ctx, ucBluetoothNewKey);
			//aes_encrypt_cbc
			aes256_encrypt_ecb(&ctx, ucEncDecData);

			// copy structure
			memcpy(pucDst,ucEncDecData,ucSaveSize);
			printf("aes:\r\n");
			hexdump((char*)pucDst,128);
			hexdump((char*)pucDst,strlen((char const*)ucEncDecData));

			aes256_decrypt_ecb(&ctx, pucDst);

			aes256_done(&ctx);
		}
    }
    if( strcmp(argv[1],"fota") == 0 )
    {
        if( strcmp(argv[2],"update") == 0 )
        {
            // bootloader
            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion = g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion-1;

		    // /APP �⺻ ���� ����
		    g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion = g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion-1;

            SetFirmwareInfo(&g_FirmwareInfo);

            stCarReport stData;
            memset(&stData,0,sizeof(stData));
            stData.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_FOTA;
            stData.rpSmartKey.Request.ControlType = eREMOTE_CON_FOTA_TYPE_UPDATE;
            Send2MngSysMsg2(eMngModem,eReqFota,eFwUpdate, eTrue, &stData,0);
        }
        else if( strcmp(argv[2],"test") == 0 )
        {
            stCarReport stData;
            memset(&stData,0,sizeof(stData));
            stData.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_FOTA;
            stData.rpSmartKey.Request.ControlType = eREMOTE_CON_FOTA_TYPE_TEST;
            Send2MngSysMsg2(eMngModem,eReqFota,eFwTestUpdate, eTrue, &stData,0);
        }
        else if( strcmp(argv[2],"clear") == 0 )
        {
            if( strcmp(argv[3],"all") == 0 )
            {
                // bootloader
                g_FirmwareInfo.AppProperty[eApp_Bootloader].nSignal = FIRMWARE_APP_SIGNAL;
                g_FirmwareInfo.AppProperty[eApp_Bootloader].wFirmwareSize = 100;//FIRMWARE_INFO_ADDRESS-BOOTLOADER_ADDRESS;
                g_FirmwareInfo.AppProperty[eApp_Bootloader].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
                memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Bootloader].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
                g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion = 1;

    		    // /APP �⺻ ���� ����
    		    g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion = 1;
            }

    		// Master DB �⺻ ���� ����
    		g_FirmwareInfo.AppProperty[eApp_MasterDB].nSignal = FIRMWARE_APP_SIGNAL;
    		g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
    		g_FirmwareInfo.AppProperty[eApp_MasterDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
    		memcpy((char*)g_FirmwareInfo.AppProperty[eApp_MasterDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
    		g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion = DEFAULT_FW_VERION;

    		// Slave DB �⺻ ���� ����
    		g_FirmwareInfo.AppProperty[eApp_SlaveDB].nSignal = FIRMWARE_APP_SIGNAL;
    		g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize = 100;//APPLICATION_ADDRESS-CAR_SLAVE_ADDRESS;
    		g_FirmwareInfo.AppProperty[eApp_SlaveDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
    		memcpy((char*)g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
    		g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion = DEFAULT_FW_VERION;

    		// Driving DB �⺻ ���� ����
    		g_FirmwareInfo.AppProperty[eApp_ControlDB].nSignal = FIRMWARE_APP_SIGNAL;
    		g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
    		g_FirmwareInfo.AppProperty[eApp_ControlDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
    		memcpy((char*)g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
    		g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion = DEFAULT_FW_VERION;


            SetFirmwareInfo(&g_FirmwareInfo);
            Send2MngSysMsg2(eMngModem,eReqFota,eFwUpdate, eTrue, (stCarReport *)NULL,0);
        }
    }
    if( strcmp(argv[1],"agps") == 0 )
    {
        if( strcmp(argv[2],"down") == 0 )
        {
            CheckExpiretAGPSData();
            //Send2MngSysMsg(eMngSys,eReqAgps,eAgpsDownload,(stCarReport *)NULL,0);
        }
        else if( strcmp(argv[2],"clear") == 0 )
        {
            extern BR_SystemInfo BkSram_SystemInfo;

            BkSram_SystemInfo.unAGPSExireDate = 0;
            DeleteAgpsData(AUTOLINK_AGPS_DATA);
        }
    }
/*    if( strcmp(argv[1],"ec") == 0 )
    {
        int nErrorCodeSample;
        if( strcmp(argv[2],"w") == 0 )
        {
            printf("Error Code Write\n");
            WriteErrorCode(nErrorCodeSample++);
        }
        else if( strcmp(argv[2],"r") == 0 )
        {
            printf("Error Code Read\n");
            ReadErrorCode();
        }
        else if( strcmp(argv[2],"c") == 0 )
        {
            printf("Error Code Clear\n");
            ClearErrorCode();
        }
    }
    if( strcmp(argv[1],"gpslist") == 0 )
    {
        printf("gps list test\b");
        char buf[1024] = {0,};
        double a[4] = {37.123456,37.123457,37.123458,37.123459};
        double b[4] = {127.013456,127.023456,127.033456,127.043456};
        MakeGpsList2String2(4,a,b,buf);
    }
    if( strcmp(argv[1],"eipek") == 0 )
    {
        char buffer[2048]={0,};
        uint32_t unSize;
        GetPublickey(buffer,(int*)&unSize);
    }
    if( strcmp(argv[1],"sw") == 0 )
    {
        char buf[256] = {0,};
        char buf2[256] = {0,};
        for(int i=0;i<256;i++)
            buf[i]=i;

        sFLASH_ReadID();


        sFLASH_EraseSubSector(SECTOR_CONFIG_STORAGE * 4096);
        sFLASH_ReadBuffer((unsigned char*)buf2, SECTOR_CONFIG_STORAGE*4096,256);
        hexdump(buf2,256);
        memset(buf2,0,256);
        sFLASH_WritePage((unsigned char*)buf,SECTOR_CONFIG_STORAGE*4096,256);
        sFLASH_ReadBuffer((unsigned char*)buf2, SECTOR_CONFIG_STORAGE*4096,256);
        hexdump(buf,256);
        hexdump(buf2,256);
    }
    if( strcmp(argv[1],"bcl") == 0 )
    {
        if( strcmp(argv[2],"0") == 0 )
        {

        }
        if( strcmp(argv[2],"1") == 0 )
        {
        }
    }
    if( strcmp(argv[1],"save") == 0 )
    {
        if( strcmp(argv[2],"0") == 0 )
        {
            extern stMsgHandlerData m_stMsgHdData;
            m_stMsgHdData.stObdSettingValue.ObdSetting.unOdometer = 0x999;
            m_stMsgHdData.stObdSettingValue.ObdSetting.dlLatitue = 32.999;
            m_stMsgHdData.stObdSettingValue.ObdSetting.dlLongitude = 127.999;
        }
    }
    if( strcmp(argv[1],"nou") == 0 )
    {
        SetModemState(eMODEM_NO_USIM);
    }
    if( strcmp(argv[1],"nmdm") == 0 )
    {
        SetModemState(eMODEM_NONE);
    }*/
    if( strcmp(argv[1],"adc") == 0)
    {
        if( strcmp(argv[2],"set") == 0)
        {
            HalADC_SetAdc_Chnnel((unsigned int)HAL_ADC2, 1);
        }
        else if( strcmp(argv[2],"get") == 0 )
        {
            unsigned short  ulBattVolt=0;
            unsigned char   uAdCnt = 0;
            HalADC_SetAdc_Chnnel((unsigned int)HAL_ADC2, 1);

            while(1)
            {
                if (HalADC_Read2(&ulBattVolt))
                {
                    printf("==========================================\r\n");
                    printf("\r\n adc2 adcnt %02d Batcnt %d \r\n", uAdCnt, ulBattVolt);
                }
                uAdCnt++;
                APP_Delay(1000);
            }
        }
    }
    if( strcmp(argv[1],"valet") == 0)
    {
        if( strcmp(argv[2],"set") == 0)
        {
            extern void SetActiveValetMode(boolean_t bActive);

            SetActiveValetMode(true);
        }
    }
    if( strcmp(argv[1],"ig") == 0 )
    {
        if( strcmp(argv[2],"on") == 0 )
        {
            Send2MngSysMsg2(eMngObd,eOBDStatus,eIGStatus,true, (stCarReport *)NULL,0);
        }
        else if( strcmp(argv[2],"off") == 0 )
        {
            Send2MngSysMsg2(eMngObd,eOBDStatus,eIGStatus,false, (stCarReport *)NULL,0);
        }
    }
    if( strcmp(argv[1],"guard") == 0)
    {
        if( strcmp(argv[2],"on") == 0)
        {
            SetActiveGuardMode(true);
        }
    }
	if( strcmp(argv[1],"time") == 0 )
    {
        uint32_t unCurTime = GetUTCTime();

        //2066684408; // pre-calculated time //2035 06 28 23 00 08
        // because posix is possiable to calculated until 2038 year.
        unCurTime += (17*365*24*3600);

        DisplayTime("2035 : ",unCurTime);
    }
	if( strcmp(argv[1],"battery") == 0 )
    {
        printf("battery : %f\r\n",Get_Battery());
    }
    if( strcmp(argv[1],"resetm") == 0 )
    {
        {
            Trace("=============================================================\r\n");
            Trace("Reset Test\r\n");

            WaitCriticalErrorTimerCallBack();

            for(int i=0;i<2;i++)
            {
                HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
                MD_mDelay(200);

        		HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
                MD_mDelay(200);

                HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
                MD_mDelay(50);
            }
            HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
        }
    }
/*    if( strcmp(argv[1],"polygeo") == 0 )
    {
        if( strcmp(argv[2],"go") == 0 )
        {
            static int m_nIndex=0;
            static stPolygonGeofencePoint stPolygonList[]=
            {
                {37.508054,127.126261},
                {37.507925,127.128325},
                {37.507577,127.129625},
                {37.507358,127.130974},
                {37.507254,127.132259},
                {37.507125,127.133884},
                {37.506912,127.130283}
            };

            stGeofenceUnit stCurrentUnit;
            stCurrentUnit.GpsLongitude = stPolygonList[m_nIndex].GpsLatitude;
            stCurrentUnit.GpsLatitude = stPolygonList[m_nIndex++].GpsLongitude;

            stGeofenceUnit stSettingUnit;
            memset((char*)&stSettingUnit,0,sizeof(stGeofenceUnit));

            CheckPolygonGeoFence(stCurrentUnit,&stSettingUnit);

            if( m_nIndex == 6 )
            {
                m_nIndex = 0;
            }
        }
        else if( strcmp(argv[2],"test") == 0 )
        {
            stMsgMdm stModem;
            stCarReport stGeofenceSetting;
            memset((char*)&stGeofenceSetting,0,sizeof(stCarReport));

            ParsePolygonGeoFenceMessage(&stGeofenceSetting,112+20);

            GetLastRemoteControlMessage(&stModem);
            // add check between occurred time and received sms time
            // if different time is over 40-50s, then remove this command
            // if we need to notify to server, then report reject report.
            if( stModem.carReport.rpSmartKey.Request.OccurredEventTime - stGeofenceSetting.rpSetting.UserSetting.stUserActionSetting.Geofence.unDateTime > 40 )
            {
                // notify to server if need
                Trace("Occurred Event Time - Sms Received Time : %d", stModem.carReport.rpSmartKey.Request.OccurredEventTime - stGeofenceSetting.rpSetting.UserSetting.stUserActionSetting.Geofence.unDateTime);
                //return
            }

            stGeofenceSetting.rpSetting.UserSetting.ucCommandType = eREMOTE_CON_CMD_TYPE_POLYGON_GEO_FENCE;

            Send2MngSysMsg(eMngModem, eReqReport, eR_ReqUserActionSetting, (stCarReport*)&stGeofenceSetting,0);
        }
    }*/
    if( strcmp(argv[1],"nvreset") == 0 )
    {
        NVIC_SystemReset();
        while(1);
    }
    if( strcmp(argv[1],"wtd") == 0 )
    {
        SetWDTReset(atoi(argv[2]));
    }
    if( strcmp(argv[1],"malloc") == 0 )
    {
        __iar_dlmalloc_stats();
    }

    if(strcmp(argv[1], "gpsdef") == 0) {
        printf("set gps defualt\r\n");
        g_GPSInfo.lat = 36.635148;
        g_GPSInfo.lon = 127.492172;

		SetAutolinkConfigProperty(eAutoLinkConfig_LastLatitude,&g_GPSInfo.lat);
		SetAutolinkConfigProperty(eAutoLinkConfig_LastLongitude,&g_GPSInfo.lon);
    }
    if( strcmp(argv[1], "rfid") == 0 )
    {
        extern BR_SystemInfo BkSram_SystemInfo;

        printf("\r\nRFID ID : ");
        hexdump(&BkSram_SystemInfo.ucarrRFIDUID,RFID_08C_DATA_LENGTH);
    }
    if(strcmp(argv[1], "db") == 0)
    {
        uint16_t usCheckSum;
        if(strcmp(argv[2], "master") == 0)
        {
            FindLastAddressofDB(eFOTA_FILE_TYPE_DIAGNOSIS_MASTER,argv[3],atoi(argv[4]),&usCheckSum);
            printf("DB Name : %s\r\n",argv[3]);
            printf("Find Check Sum : %x\r\n",usCheckSum);
        }
        else if(strcmp(argv[2], "slave") == 0)
        {
            FindLastAddressofDB(eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE,argv[3],atoi(argv[4]),&usCheckSum);
            printf("DB Name : %s\r\n",argv[3]);
            printf("Find Check Sum : %x\r\n",usCheckSum);
        }
        else if(strcmp(argv[2], "control") == 0)
        {
            FindLastAddressofDB(eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL,argv[3],atoi(argv[4]),&usCheckSum);
            printf("DB Name : %s\r\n",argv[3]);
            printf("Find Check Sum : %x\r\n",usCheckSum);
        }
		else if(strcmp(argv[2], "boot") == 0)
        {
            FindLastAddressofDB(eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT,"BOOTFW",atoi(argv[3]),&usCheckSum);
            printf("DB Name : %s\r\n",argv[3]);
            printf("Find Check Sum : %x\r\n",usCheckSum);
        }
		else if(strcmp(argv[2], "app") == 0)
        {
            FindLastAddressofDB(eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP,"APPLFW",atoi(argv[3]),&usCheckSum);
            printf("DB Name : %s\r\n",argv[3]);
            printf("Find Check Sum : %x\r\n",usCheckSum);
        }
        else if(strcmp(argv[2], "del") == 0)
        {
            if( strcmp(argv[3], "master") == 0)
            {
                printf("Del Master DB\r\n");
                DelInternalFlashofDB(eFOTA_FILE_TYPE_DIAGNOSIS_MASTER);

                g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_MASTER].nAppFWVersion = 0;
                memcpy((char *)g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_MASTER].arrFWName, DEFAULT_FILE_NAME, 6);
                g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_MASTER].wFirmwareSize = 0;
                g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_MASTER].nCheckSum = 0;

                SetFirmwareInfo(&g_FirmwareInfo);
            }
            else if( strcmp(argv[3], "slave") == 0)
            {
                printf("Del Slave DB\r\n");
                DelInternalFlashofDB(eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE);

                g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE].nAppFWVersion = 0;
                memcpy((char *)g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE].arrFWName, DEFAULT_FILE_NAME, 6);
                g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE].wFirmwareSize = 0;
                g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE].nCheckSum = 0;

                SetFirmwareInfo(&g_FirmwareInfo);
            }
            else if( strcmp(argv[3], "control") == 0)
            {
                printf("Del Control DB\r\n");
                DelInternalFlashofDB(eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL);

                g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL].nAppFWVersion = 0;
                memcpy((char *)g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL].arrFWName, DEFAULT_FILE_NAME, 6);
                g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL].wFirmwareSize = 0;
                g_FirmwareInfo.AppProperty[eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL].nCheckSum = 0;

                SetFirmwareInfo(&g_FirmwareInfo);
            }
			else if( strcmp(argv[3], "info") == 0)
            {
                printf("del info\r\n");
                g_FirmwareInfo.nSignal = FIRMWARE_SIGNAL;

				// Bootloader �⺻ ���� ����
				g_FirmwareInfo.AppProperty[eApp_Bootloader].nSignal = (int)FIRMWARE_APP_SIGNAL;
				g_FirmwareInfo.AppProperty[eApp_Bootloader].wFirmwareSize = 100;//FIRMWARE_INFO_ADDRESS-BOOTLOADER_ADDRESS;
				g_FirmwareInfo.AppProperty[eApp_Bootloader].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
				memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Bootloader].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
				g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion = 1;

				// /APP �⺻ ���� ����
				g_FirmwareInfo.AppProperty[eApp_Application].nSignal = FIRMWARE_APP_SIGNAL;
				g_FirmwareInfo.AppProperty[eApp_Application].wFirmwareSize = 100;//UPDATE_TMP_ADDRESS - APPLICATION_ADDRESS;
				g_FirmwareInfo.AppProperty[eApp_Application].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
				memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Application].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
				g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion = 1;

				// Master DB �⺻ ���� ����
				g_FirmwareInfo.AppProperty[eApp_MasterDB].nSignal = FIRMWARE_APP_SIGNAL;
				g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
				g_FirmwareInfo.AppProperty[eApp_MasterDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
				memcpy((char*)g_FirmwareInfo.AppProperty[eApp_MasterDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
				g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion = DEFAULT_FW_VERION;

				// Slave DB �⺻ ���� ����
				g_FirmwareInfo.AppProperty[eApp_SlaveDB].nSignal = FIRMWARE_APP_SIGNAL;
				g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize = 100;//APPLICATION_ADDRESS-CAR_SLAVE_ADDRESS;
				g_FirmwareInfo.AppProperty[eApp_SlaveDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
				memcpy((char*)g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
				g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion = DEFAULT_FW_VERION;

				// Driving DB �⺻ ���� ����
				g_FirmwareInfo.AppProperty[eApp_ControlDB].nSignal = FIRMWARE_APP_SIGNAL;
				g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
				g_FirmwareInfo.AppProperty[eApp_ControlDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
				memcpy((char*)g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
				g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion = DEFAULT_FW_VERION;

				g_FirmwareInfo.SwitchingInfo.iApplMode = eApp_Application;
				g_FirmwareInfo.SwitchingInfo.iStatus = eFW_SWITCH_COMPLETE;

                SetFirmwareInfo(&g_FirmwareInfo);
            }
        }
    }
    if(strcmp(argv[1],"sensor") == 0 )
    {
        #include "GIT_SensorProc.h"
        extern SENSOR_DATA SensorData;
        if(strcmp(argv[2],"init") == 0 )
        {
            InitializeSensorManager();
            SensorData.g_eSensorState = eSENSOR_STATE_RESET_MPU6515;
        }
		else if( strcmp(argv[2],"init2") == 0 )
	    {
	    	MPU6515_initMPU6515();
	    }
        else if( strcmp(argv[2],"get") == 0 )
        {
            stSensorInfo stGyroAngle;
            GetGyroAngle(&stGyroAngle);

            printf("XXX : %d, YYY : %d, ZZZ : %d\r\n",stGyroAngle.nX,stGyroAngle.nY,stGyroAngle.nZ);
        }
        else if( strcmp(argv[2],"cal") == 0)
        {
            stSensorInfo stGyroAngle;
            GetGyroNavieAngle(&stGyroAngle);
			stGyroAngle.bCalibration = true;
            SetGyroInitializeAngle(&stGyroAngle);
			UpdataGyroDefualtAngle();
        }
	    else if(strcmp(argv[2],"clear") == 0 )
	    {
			stSensorInfo stGyroAngle;
			memset(&stGyroAngle,0,sizeof(stGyroAngle));
	        SetGyroInitializeAngle(&stGyroAngle);
			UpdataGyroDefualtAngle();
		}
		else if(strcmp(argv[2],"check") == 0 )
		{
			MeasureGyroAngle();
		}
		else if(strcmp(argv[2],"obdset") == 0 )
		{
			SendObdConfig2System(999,Get_GPS_Lat_Origin(),Get_GPS_Lon_Origin(),Get_DoorLock(),Get_DoorOpen(),Get_HeadLamp_State(), 99);
		}
    }

    if(strcmp(argv[1],"start") == 0 )
    {
		Trace("\r\n\r\n");
		Trace("****************************************\r\n");
		Trace("MSG: Remote control start event!!!\r\n");
		Trace("****************************************\r\n\r\n");


        stCarReport msg;
        msg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_STARTING;
        msg.rpSmartKey.Request.ControlType = 1;;
        msg.rpSmartKey.Request.KeepPowerOnTime = 10;
        msg.rpSmartKey.Request.Temperature = 0x1004;
        msg.rpSmartKey.Request.Defrost = 0;
		msg.rpSmartKey.Request.RearDefogger = 0;
        msg.rpSmartKey.Request.BoundType = eREMOTE_CON_BOUNDTYPE_NONE;

        Send2MngObd(eMngSysMsg,eReqReport,eR_ReqSmartKey,&msg,0);
    }
    if(strcmp(argv[1],"stop") == 0 )
    {
		Trace("\r\n\r\n");
		Trace("****************************************\r\n");
		Trace("MSG: Remote control stop event!!!\r\n");
		Trace("****************************************\r\n\r\n");

        stCarReport msg;
        msg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_STARTING;
        msg.rpSmartKey.Request.ControlType = 0;
        msg.rpSmartKey.Request.KeepPowerOnTime = 10;
        msg.rpSmartKey.Request.Temperature = 0x1004;
        msg.rpSmartKey.Request.Defrost = 0;
		msg.rpSmartKey.Request.RearDefogger = 0;
        msg.rpSmartKey.Request.BoundType = eREMOTE_CON_BOUNDTYPE_NONE;

        Send2MngObd(eMngSysMsg,eReqReport,eR_ReqSmartKey,&msg,0);
    }
    if( strcmp(argv[1],"vinlock") == 0 )
    {
        uint8_t cNewVin = 0;
        SetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,(void*)&cNewVin);
    }
    if( strcmp(argv[1],"vinunlock") == 0 )
    {
        uint8_t cNewVin = 1;
        SetVINCode(STR_DEFAULT_VIN);
        SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)STR_DEFAULT_VIN);
        SetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,(void*)&cNewVin);
    }
    if( strcmp(argv[1],"qover") == 0 )
    {
        Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
        Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
        Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
        Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
        Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
        Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
        Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
        Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
    }
    if( strcmp(argv[1],"autovin") == 0 )
    {
        extern unsigned int g_uiAutoVinReqPacketIdx;
        extern eAUTOVIN_STATE g_eAutoVinReqFuelType;
        extern bool g_bAutoVinNegativeResponse;

        g_uiAutoVinReqPacketIdx = 0;
        g_bAutoVinNegativeResponse = FALSE;
        SetOBDState(eOBD_GetAutoVIN);
    }
    if( strcmp(argv[1],"power") == 0 )
    {
        extern void SetupForSleep();
        SetupForSleep();
    }
    if( strcmp(argv[1],"wakeup") == 0 )
    {
        EnterStandbyAndRTCAlarmWakeup(1);
    }
	if( strcmp(argv[1],"get") == 0 )
    {
		if( strcmp(argv[2],"modem") == 0 )
		{
			printf("ModemManagerData.bAvailableModemCommFlag : %d\r\nModemManagerData.bNeedStartUpCheckSMSFlag : %d\r\nModemManagerData.bRequestMessageCommFlag : %d\r\nModemManagerData.bRequestAgpsCommFlag : %d\r\nModemManagerData.eFOTAStartState : %d\r\nModemManagerData.bRequestNetworkTime : %d\r\n",
					ModemManagerData.bAvailableModemCommFlag,ModemManagerData.bNeedStartUpCheckSMSFlag,ModemManagerData.bRequestMessageCommFlag,ModemManagerData.bRequestAgpsCommFlag,ModemManagerData.eFOTAStartState,ModemManagerData.bRequestNetworkTime);
		}
	}
	if( strcmp(argv[1],"wdu") == 0 )
	{
		if(strcmp(argv[2],"def") == 0 )
		{
			WriteDefaultURL();
		}
		else if(strcmp(argv[2],"kr") == 0 )
		{
#include "aes.h"
#include "base64_penta.h"
			extern unsigned char m_carrIv[];

			char * carrSeriaNumber = GetFWSerialNumber();
			//DCSServiceType nServiceType; mod.kks todo remove
			unsigned char AutolinkEncryptKey[MAX_ENCRYPT_KEY_LENGTH] = {0};
			stURLInfo stTemp;
			//unsigned char strTemp[MAX_SERVER_URL_LENGTH+8+1]={0,}; mod.kks todo remove.
            char strTemp[MAX_SERVER_URL_LENGTH+8+1]={0,};
			int nEncryptResult=0;

			memcpy(g_stServerUrl.Url,	AU_HTTP_RETAIL_DEV_MESSAGE_URL,	sizeof(AU_HTTP_RETAIL_DEV_MESSAGE_URL));

			memcpy(g_stServerUrl.Path,	HTTP__MESSAGE_PATH, sizeof(HTTP__MESSAGE_PATH));

			sprintf(strTemp,"%s%s","https://",g_stServerUrl.Url);	//�������� �����ִ� ������ https �����̶�.... �����ϰ������� �ð���..........

			memset((char*)&stTemp,0x00,sizeof(stTemp));
			GetDefaultEncryptKey((char *)AutolinkEncryptKey,true);

			// we will apply encryp process after we apply damo encryption process.
			nEncryptResult = DAMO_CRYPT_AES_EncryptEx((unsigned char *)stTemp.strUrl, (size_t*)&stTemp.nEncryptedLength,
				(const unsigned char*)&strTemp, MAX_SERVER_URL_LENGTH,
				AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, m_carrIv);

			if( nEncryptResult == 0 )	//if success
			{
				g_FirmwareInfo.m_stURLInfo.nEncryptedLength = stTemp.nEncryptedLength;
				g_FirmwareInfo.m_stURLInfo.nPreamble = 0xFE000728;
				memcpy( g_FirmwareInfo.m_stURLInfo.strUrl,stTemp.strUrl,MAX_SERVER_URL_LENGTH+AES_PADDING_LENGTH);	//�е�����
				SetFirmwareInfo(&g_FirmwareInfo);
			}
		}
	}
	if( strcmp(argv[1],"pm") == 0)
	{
		if( strcmp(argv[2],"drv") == 0)
		{
			Display_ReportDriving();
		}
	}
}

#if 0
static void gpio_op_cmd (uint32_t argc, char *argv[])
{
	uint16_t pin;
	stHalGPIO_InitTypeDef GPIO_InitStructure;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOC_GROUP | GPIOC_GROUP | GPIOC_GROUP, NULL, 0, HAL_ENABLE);

	GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_NOPULL;				// No Pull-up���� ��������.

	if(argc != 4) {
		Trace("Not defined argument\n");
		Trace(" ex) gpio a 10 l --> porta, pin 10, output: low\n");
		Trace(" ex) gpio b 10 h --> portb, pin 10, output: high\n");
		Trace(" ex) gpio c 10 i --> portc, pin 10, input\n");

		return;
	}

	pin = atoi(argv[2]);
	if(pin < 16) {
		printf("PIN: %d, ", GPIO_Pin_0 + pin - 1);
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 << pin;
	}
	else {
		Trace("Not defined pin number\n");

		return;
	}

	if(strcmp(argv[3], "i") == 0) {
		printf("Dir: Input\n");
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;

        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
	}
	else {
		if(strcmp(argv[3], "l") == 0) {
			printf("Dir: Output, State: Low\n");

			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

			HalGPIOSetVaule(GPIO_InitStructure.GPIO_Pin, eBIT_RESET);
		}
		else if(strcmp(argv[3], "h") == 0) {
			printf("Dir: Output, State: High\n");

			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

			HalGPIOSetVaule(GPIO_InitStructure.GPIO_Pin, eBIT_SET);
		}
		else {
			Trace("Not defined state\n");
		}
	}
}
#endif

static void obd_op_cmd (uint32_t argc, char *argv[])
{
	//uint16_t pin;
	stHalGPIO_InitTypeDef  GPIO_InitStructure;
	//GPIO_TypeDef* GPIOx;
	char a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0,j=0;
	unsigned int uiStartId=0x0168,uiEndId=0x0168;

	a = HalGPIOGetStatus(GPIO_LOW_CAN_NSTB);	// LOW CAN Normal
	b = HalGPIOGetStatus(GPIO_LOW_CAN_EN);	// LOW CAN Enable
	c = HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	d = HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_SEL);
	e = HalGPIOGetStatus(GPIO_CAN_RX_CTL);
	f =	 HalGPIOGetStatus(GPIO_CAN_RX_MON);
	g = HalGPIOGetStatus(GPIO_CAN1_RX);
	h = HalGPIOGetStatus(GPIO_HI_CAN2_CHK_CTL);
	i =  HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	j =  HalGPIOGetStatus(GPIO_CAN1_ENABLE);

		printf("\r\n");
		printf("GPIO_LOW_CAN_NSTB : %d, GPIO_LOW_CAN_EN : %d, GPIO_HIGH_CAN3_EN:%d, GPIO_LOW_HIGH_CAN_SEL:%d, GPIO_CAN_RX_CTL:%d\r\n",a,b,c,d,e);
		printf("GPIO_CAN_RX_MON : %d, GPIO_CAN1_RX : %d, GPIO_HI_CAN2_CHK_CTL:%d, GPIO_HIGH_CAN3_EN:%d, GPIO_CAN1_ENABLE:%d\r\n",f,g,h,i,j);
		printf("\r\n");
	if(strcmp(argv[1], "a") == 0) {
		HalGPIOSetVaule(GPIO_LOW_CAN_RX_CTL,false);	//false ����ڴ�, true �ȱ���ڴ�
        HalGPIOSetVaule(GPIO_CAN_RX_CTL,true);
        HalGPIOSetVaule(GPIO_MO_WAKE_CTL,true);
        HalGPIOSetVaule(GPIO_BT_MON_CTL,true);
        HalGPIOSetVaule(GPIO_IG_ACC_DET_CTL,true);
        // select high can 1 : false
        // select high can 2 : true
        //HalGPIOSetVaule(GPIOE,GPIO_HI_CAN2_CHK_CTL,false);
        // select low can : true
        // select high can 3 : false
        HalGPIOSetVaule(GPIO_LOW_HIGH_CAN_SEL,true);
		HalGpioGenerateLatchClock();
		printf("Latch OK\r\n");
		a = HalGPIOGetStatus(GPIO_LOW_CAN_NSTB);	// LOW CAN Normal
	b = HalGPIOGetStatus(GPIO_LOW_CAN_EN);	// LOW CAN Enable
	c = HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	d = HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_SEL);
	e = HalGPIOGetStatus(GPIO_CAN_RX_CTL);
	f =	 HalGPIOGetStatus(GPIO_CAN_RX_MON);
	g = HalGPIOGetStatus(GPIO_CAN1_RX);
	h = HalGPIOGetStatus(GPIO_HI_CAN2_CHK_CTL);
	i =  HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	j =  HalGPIOGetStatus(GPIO_CAN1_ENABLE);


		printf("GPIO_LOW_CAN_NSTB : %d, GPIO_LOW_CAN_EN : %d, GPIO_HIGH_CAN3_EN:%d, GPIO_LOW_HIGH_CAN_SEL:%d, GPIO_CAN_RX_CTL:%d\r\n",a,b,c,d,e);
		printf("GPIO_CAN_RX_MON : %d, GPIO_CAN1_RX : %d, GPIO_HI_CAN2_CHK_CTL:%d, GPIO_HIGH_CAN3_EN:%d, GPIO_CAN1_ENABLE:%d\r\n",f,g,h,i,j);
	}
	else if(strcmp(argv[1], "state") == 0) {
		printf("GetOBDState:%d, VehicleStatus : %d,ACC:%d IG1:%d IG2:%d IG3:%d ISG:%d USER:%d RPM:%d g_bIndicatorDBFlag:%d g_OBDControllerData.basicData.decide_Starting_Engine%d\r\n",GetOBDState(),Get_VehicleStatus(),Get_ACC(),Get_IG1_Status(),Get_IG2_Status(),Get_IG3_Status(),Get_ISG_Status(),Get_UserStatus(),Get_RPM(),g_bIndicatorDBFlag,g_OBDControllerData.basicData.decide_Starting_Engine);
		Decide_EngingButton_State();
	}
	else if(strcmp(argv[1], "aaa") == 0) {
		float aaa=123.5678;
		unsigned char bbb=50;
		unsigned char ccc=0;
		ccc = abs((int)aaa);
		printf("%d \r\n",ccc);
		ccc = abs(bbb);
		printf("%d \r\n",ccc);
		Send_Remain_Fuel_Percent_From_CAN(aaa);
	}
	else if(strcmp(argv[1], "b") == 0) {	//H3 STANDBY , LOW STANDBY
		Oem_CAN2_STANDBY_ACTIVE();
		printf("H3,LOW STANDBY OK\r\n");
				a = HalGPIOGetStatus(GPIO_LOW_CAN_NSTB);	// LOW CAN Normal
	b = HalGPIOGetStatus(GPIO_LOW_CAN_EN);	// LOW CAN Enable
	c = HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	d = HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_SEL);
	e = HalGPIOGetStatus(GPIO_CAN_RX_CTL);
	f =	 HalGPIOGetStatus(GPIO_CAN_RX_MON);
	g = HalGPIOGetStatus(GPIO_CAN1_RX);
	h = HalGPIOGetStatus(GPIO_HI_CAN2_CHK_CTL);
	i =  HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	j =  HalGPIOGetStatus(GPIO_CAN1_ENABLE);


		printf("GPIO_LOW_CAN_NSTB : %d, GPIO_LOW_CAN_EN : %d, GPIO_HIGH_CAN3_EN:%d, GPIO_LOW_HIGH_CAN_SEL:%d, GPIO_CAN_RX_CTL:%d\r\n",a,b,c,d,e);
		printf("GPIO_CAN_RX_MON : %d, GPIO_CAN1_RX : %d, GPIO_HI_CAN2_CHK_CTL:%d, GPIO_HIGH_CAN3_EN:%d, GPIO_CAN1_ENABLE:%d\r\n",f,g,h,i,j);
	}
	else if(strcmp(argv[1], "c") == 0) {
		Oem_CAN2_SEL_LOW_CAN();
		printf("GPIO_LOW_HIGH_CAN_SEL Bit_SET OK\r\n");
				a = HalGPIOGetStatus(GPIO_LOW_CAN_NSTB);	// LOW CAN Normal
	b = HalGPIOGetStatus(GPIO_LOW_CAN_EN);	// LOW CAN Enable
	c = HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	d = HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_SEL);
	e = HalGPIOGetStatus(GPIO_CAN_RX_CTL);
	f =	 HalGPIOGetStatus(GPIO_CAN_RX_MON);
	g = HalGPIOGetStatus(GPIO_CAN1_RX);
	h = HalGPIOGetStatus(GPIO_HI_CAN2_CHK_CTL);
	i =  HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	j =  HalGPIOGetStatus(GPIO_CAN1_ENABLE);


		printf("GPIO_LOW_CAN_NSTB : %d, GPIO_LOW_CAN_EN : %d, GPIO_HIGH_CAN3_EN:%d, GPIO_LOW_HIGH_CAN_SEL:%d, GPIO_CAN_RX_CTL:%d\r\n",a,b,c,d,e);
		printf("GPIO_CAN_RX_MON : %d, GPIO_CAN1_RX : %d, GPIO_HI_CAN2_CHK_CTL:%d, GPIO_HIGH_CAN3_EN:%d, GPIO_CAN1_ENABLE:%d\r\n",f,g,h,i,j);
	}
	else if(strcmp(argv[1], "d") == 0) {
		HalGPIOSetVaule(GPIO_LOW_CAN_NSTB, eBIT_SET);	//  Normal
		printf("GPIO_LOW_CAN_NSTB Bit_SET OK\r\n");

        a = HalGPIOGetStatus(GPIO_LOW_CAN_NSTB);	// LOW CAN Normal
        b = HalGPIOGetStatus(GPIO_LOW_CAN_EN);	// LOW CAN Enable
        c = HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
        d = HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_SEL);
        e = HalGPIOGetStatus(GPIO_CAN_RX_CTL);
        f =	 HalGPIOGetStatus(GPIO_CAN_RX_MON);
        g = HalGPIOGetStatus(GPIO_CAN1_RX);
        h = HalGPIOGetStatus(GPIO_HI_CAN2_CHK_CTL);
        i =  HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
        j =  HalGPIOGetStatus(GPIO_CAN1_ENABLE);


		printf("GPIO_LOW_CAN_NSTB : %d, GPIO_LOW_CAN_EN : %d, GPIO_HIGH_CAN3_EN:%d, GPIO_LOW_HIGH_CAN_SEL:%d, GPIO_CAN_RX_CTL:%d\r\n",a,b,c,d,e);
		printf("GPIO_CAN_RX_MON : %d, GPIO_CAN1_RX : %d, GPIO_HI_CAN2_CHK_CTL:%d, GPIO_HIGH_CAN3_EN:%d, GPIO_CAN1_ENABLE:%d\r\n",f,g,h,i,j);
	}
	else if(strcmp(argv[1], "e") == 0) {

		HalGPIOSetVaule(GPIO_LOW_CAN_EN, eBIT_SET);	// eBIT_SETCAN Enable
		printf("GPIO_LOW_CAN_EN Bit_SET OK\r\n");
        a = HalGPIOGetStatus(GPIO_LOW_CAN_NSTB);	// LOW CAN Normal
        b = HalGPIOGetStatus(GPIO_LOW_CAN_EN);	// LOW CAN Enable
        c = HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
        d = HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_SEL);
        e = HalGPIOGetStatus(GPIO_CAN_RX_CTL);
        f =	 HalGPIOGetStatus(GPIO_CAN_RX_MON);
        g = HalGPIOGetStatus(GPIO_CAN1_RX);
        h = HalGPIOGetStatus(GPIO_HI_CAN2_CHK_CTL);
        i =  HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
        j =  HalGPIOGetStatus(GPIO_CAN1_ENABLE);


		printf("GPIO_LOW_CAN_NSTB : %d, GPIO_LOW_CAN_EN : %d, GPIO_HIGH_CAN3_EN:%d, GPIO_LOW_HIGH_CAN_SEL:%d, GPIO_CAN_RX_CTL:%d\r\n",a,b,c,d,e);
		printf("GPIO_CAN_RX_MON : %d, GPIO_CAN1_RX : %d, GPIO_HI_CAN2_CHK_CTL:%d, GPIO_HIGH_CAN3_EN:%d, GPIO_CAN1_ENABLE:%d\r\n",f,g,h,i,j);
	}
	else if(strcmp(argv[1], "f") == 0) {
		HalCanSetBaudrate((int)HAL_CAN2, eCAN_100KBPS);
		printf("CAN2 CAN_100KBPS OK\r\n");
	}
	else if(strcmp(argv[1], "g") == 0) {
        unsigned int Can1RXcnt=0,Can2RXcnt=0;
        Can1RXcnt = HalCan_GetRxQueueCount(&g_CAN1_RxBuffCtrl);
        Can2RXcnt = HalCan_GetRxQueueCount(&g_CAN2_RxBuffCtrl);
		printf("g_eCanCommState:%d g_eLCanCommState:%d\r\n",g_eCanCommState,g_eLCanCommState);
		printf("can1:%d Can2:%d\r\n",Can1RXcnt,Can2RXcnt);
	}
	else if(strcmp(argv[1], "h") == 0)
	{
		for(int i=0;i<100;i++)
		{
			HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_RX_MON);
			HalGPIOGetStatus(GPIO_CAN_RX_MON);
			printf("%d %d OK\r\n",a,b);
		}
	}
	else if(strcmp(argv[1], "i") == 0)
	{
		Oem_CAN_Initial_CH2(4,eCAN_100KBPS);
		Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_CAN, 1, &uiStartId, &uiEndId);
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
	}
	else if(strcmp(argv[1], "z") == 0)
	{
		Oem_CAN1_STANDBY_ACTIVE();		//HIGHCAN1 ������� 1,2���� ���� 1042Ĩ
		Oem_CAN2_HIGH_CAN3_DISABLE();	//HIGHCAN3 ������� 2��° 1042Ĩ
		Oem_CAN2_LOW_CAN_DISABLE();		//LOWCAN ������� 1055Ĩ

        HalDrvCanIOCtrl(eCAN_IO_DeInit, (int)HAL_CAN1, NULL, 0, 0);
        HalDrvCanIOCtrl(eCAN_IO_DeInit, (int)HAL_CAN2, NULL, 0, 0);

		APP_Delay(50);
		a = HalGPIOGetStatus(GPIO_LOW_CAN_NSTB);	// LOW CAN Normal
	b = HalGPIOGetStatus(GPIO_LOW_CAN_EN);	// LOW CAN Enable
	c = HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	d = HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_SEL);
	e = HalGPIOGetStatus(GPIO_CAN_RX_CTL);
	f =	 HalGPIOGetStatus(GPIO_CAN_RX_MON);
	g = HalGPIOGetStatus(GPIO_CAN1_RX);
	h = HalGPIOGetStatus(GPIO_HI_CAN2_CHK_CTL);
	i =  HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	j =  HalGPIOGetStatus(GPIO_CAN1_ENABLE);


		printf("GPIO_LOW_CAN_NSTB : %d, GPIO_LOW_CAN_EN : %d, GPIO_HIGH_CAN3_EN:%d, GPIO_LOW_HIGH_CAN_SEL:%d, GPIO_CAN_RX_CTL:%d\r\n",a,b,c,d,e);
		printf("GPIO_CAN_RX_MON : %d, GPIO_CAN1_RX : %d, GPIO_HI_CAN2_CHK_CTL:%d, GPIO_HIGH_CAN3_EN:%d, GPIO_CAN1_ENABLE:%d\r\n",f,g,h,i,j);
		printf("OK\r\n");
	}
	else if(strcmp(argv[1], "sb") == 0)
	{
		ModemManagerData.eState = eMODEM_NONE;

	  //MONI 2017-12-19 it was added for current test, we needs check reset sequence.
	  SetupForInterruptforImpulse(true, false, 0xAF);
	  //INOM

		HalGPIOSetVaule(UART2_RTS_PIN, eBIT_SET);
		HalGPIOSetVaule(GPIO_MCU_LNA_EN, eBIT_RESET);

        HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart2.pUARTreg, NULL, 0, HAL_DISABLE);
        HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart3.pUARTreg, NULL, 0, HAL_DISABLE);
        HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart4.pUARTreg, NULL, 0, HAL_DISABLE);

        HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);
        HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart3.pUARTreg, NULL, 0, 0);
        HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart4.pUARTreg, NULL, 0, 0);

        GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
		GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP;
		GPIO_InitStructure.GPIO_Pin =  UART2_TX_PIN;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
		GPIO_InitStructure.GPIO_Pin =  UART2_RX_PIN;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		HalGPIOSetVaule(UART2_TX_PIN, eBIT_RESET);
		HalGPIOSetVaule(UART2_RX_PIN, eBIT_RESET);

		APP_Delay(1);

		SetLedOnOffCtl(LED_OFF, eLED_GPS);
		SetLedOnOffCtl(LED_OFF, eLED_SERVER);
		SetLedOnOffCtl(LED_OFF, eLED_CAN);

		HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_RESET);

		ModemManagerData.eState = eMODEM_NONE;

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
	a = HalGPIOGetStatus(GPIO_LOW_CAN_NSTB);	// LOW CAN Normal
	b = HalGPIOGetStatus(GPIO_LOW_CAN_EN);	// LOW CAN Enable
	c = HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	d = HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_SEL);
	e = HalGPIOGetStatus(GPIO_CAN_RX_CTL);
	f =	 HalGPIOGetStatus(GPIO_CAN_RX_MON);
	g = HalGPIOGetStatus(GPIO_CAN1_RX);
	h = HalGPIOGetStatus(GPIO_HI_CAN2_CHK_CTL);
	i =  HalGPIOGetStatus(GPIO_HIGH_CAN3_EN);
	j =  HalGPIOGetStatus(GPIO_CAN1_ENABLE);


	printf("$GPIO_LOW_CAN_NSTB : %d, GPIO_LOW_CAN_EN : %d, GPIO_HIGH_CAN3_EN:%d, GPIO_LOW_HIGH_CAN_SEL:%d, GPIO_CAN_RX_CTL:%d\r\n",a,b,c,d,e);
	printf("$GPIO_CAN_RX_MON : %d, GPIO_CAN1_RX : %d, GPIO_HI_CAN2_CHK_CTL:%d, GPIO_HIGH_CAN3_EN:%d, GPIO_CAN1_ENABLE:%d\r\n",f,g,h,i,j);
GetWakeupDetectPinsState();
		// MONI 2018-02-24
		// before go to sleep,  we clear the interrupt flag
		MPU6515_CheckInterrupt();
        APP_Delay(3000);
        HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStandbyMode, 0, NULL, 0, 0);

	}
	else if(strcmp(argv[1], "fcs") == 0)
	{
		SetOBDState(eOBD_FCS_Start);
	}
	else if(strcmp(argv[1], "start") == 0)
	{
		g_OBDControllerData.monitering_Data.m_ucACC = 1;
		g_OBDControllerData.monitering_Data.m_bIG1 = 1;
		g_OBDControllerData.monitering_Data.m_bIG2 = 1;
		Send_RPM_From_CAN(500);
	}
	else if(strcmp(argv[1], "stop") == 0)
	{
		g_OBDControllerData.monitering_Data.m_ucACC = 0;
		g_OBDControllerData.monitering_Data.m_bIG1 = 0;
		g_OBDControllerData.monitering_Data.m_bIG2 = 0;
		Send_RPM_From_CAN(0);
	}
}

static void start_op_cmd (uint32_t argc, char *argv[])
{
	g_OBDControllerData.monitering_Data.m_ucACC = 1;
	g_OBDControllerData.monitering_Data.m_bIG1 = 1;
	g_OBDControllerData.monitering_Data.m_bIG2 = 1;
	g_OBDControllerData.monitering_Data.m_bISGRun = 1;
	g_OBDControllerData.monitering_Data.m_bENGRun = 1;
	Send_RPM_From_CAN(9999);

	g_dSavedGpsLat = 9999.999999;
	g_dSavedGpsLon = 99999.999999;
	g_GPSInfo.lat = 9999.999999;
	g_GPSInfo.lon = 999999.999999;

	Send_UserStatus_From_CAN(1);
}

static void stop_op_cmd (uint32_t argc, char *argv[])
{
	g_OBDControllerData.monitering_Data.m_ucACC = 0;
	g_OBDControllerData.monitering_Data.m_bIG1 = 0;
	g_OBDControllerData.monitering_Data.m_bIG2 = 0;
	g_OBDControllerData.monitering_Data.m_bISGRun = 0;
    g_OBDControllerData.monitering_Data.m_bENGRun = 0;
	Send_RPM_From_CAN(0);
}



static void cfd_op_cmd (uint32_t argc, char *argv[])
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));

    if(strcmp(argv[1], "doorlock") == 0)
        Set_Actuator( ACTUATOR_TYPE_DOORLOCK );
    else if(strcmp(argv[1], "doorunlock") == 0)
        Set_Actuator( ACTUATOR_TYPE_DOORUNLOCK );
    else if(strcmp(argv[1], "lamp") == 0)
        Set_Actuator( ACTUATOR_TYPE_LAMP );
    else if(strcmp(argv[1], "engon") == 0)
        Set_Actuator( ACTUATOR_TYPE_ENGINERUN );
    else if(strcmp(argv[1], "engoff") == 0)
	    Set_Actuator( ACTUATOR_TYPE_ENGINESTOP );
	else if(strcmp(argv[1], "horn") == 0)
        Set_Actuator( ACTUATOR_TYPE_LAMPHORN );
	//else if(strcmp(argv[1], "fatc") == 0)
        //Set_Actuator( ACTUATOR_TYPE_AIRCON );
	else if(strcmp(argv[1], "power") == 0)
	{
        CFD_Sleep();
        extern void SetupForSleep();
        SetupForSleep();
	}
	else if(strcmp(argv[1], "find") == 0)
	{
		char m_cFindIndex[4] = {0,};
		memcpy(&m_cFindIndex,argv[2],sizeof(m_cFindIndex));
		int ndata =  atoi(argv[3]);
		int nFindIndex = 0;
		int8_t ucResult = 0;
		for(nFindIndex = 0 ; nFindIndex < g_usCurrentCnt ;nFindIndex++)
		{
			if(strcmp(m_cFindIndex,g_pstCurrDataBase[nFindIndex].m_cIndexFine) == 0)
				break;
		}
		ucResult = Compute_Data(&g_pstCurrDataBase[nFindIndex], ndata);
		printf("----Result : %d , fData : %f -----\r\n",ucResult,g_pstCurrDataBase[nFindIndex].m_fData);
	}

	else if(strcmp(argv[1], "bt") == 0)
	{

	}

	else if(strcmp(argv[1], "log") == 0)
	{
		if(strcmp(argv[2], "on") == 0)
		{
			CFD_DisplayLog();
		}
		else if(strcmp(argv[2], "off") == 0)
		{
			CFD_DisplayLogOff();
		}
	}
	else if(strcmp(argv[1], "tr") == 0)
	{

		if(strcmp(argv[2], "fd") == 0)
		{
			ucCanData[0] = 0;
			ucCanData[1] = 0;
			ucCanData[2] = 0;
			ucCanData[3] = 0;

			CFD_SendTransceiverMode(ucCanData);
			CFD_DisplayConfig();
		}
		else if(strcmp(argv[2], "can") == 0)
		{
			ucCanData[0] = 1;
			ucCanData[1] = 1;
			ucCanData[2] = 1;
			ucCanData[3] = 1;

			CFD_SendTransceiverMode(ucCanData);
			CFD_DisplayConfig();
		}




    }
    else if(strcmp(argv[1], "disp") == 0)
        CFD_DisplayConfig();
    else if(strcmp(argv[1], "dummy") == 0)
        CFD_DummyController();
    else if(strcmp(argv[1], "by") == 0)
    {
    	if(strcmp(argv[2], "off") == 0)
    		CFD_SetByPassFlagAllOff();
    	else if(strcmp(argv[2], "on") == 0)
		    CFD_SetByPassFlagAllOn();
    }
	else if(strcmp(argv[1], "led") == 0)
	{
		if(strcmp(argv[2], "r") == 0)
		{
			if(strcmp(argv[3], "on") == 0) 			CFD_SetLedControl(eCanFD_Red,eCanFD_Control_On);
			else if(strcmp(argv[3], "off") == 0) 	CFD_SetLedControl(eCanFD_Red,eCanFD_Control_Off);
			else {}
		}
		else if(strcmp(argv[2], "g") == 0)
		{
			if(strcmp(argv[3], "on") == 0) 			CFD_SetLedControl(eCanFD_Green,eCanFD_Control_On);
			else if(strcmp(argv[3], "off") == 0) 	CFD_SetLedControl(eCanFD_Green,eCanFD_Control_Off);
			else {}
		}
		else if(strcmp(argv[2], "a") == 0)
		{
			if(strcmp(argv[3], "on") == 0) 			CFD_SetLedControl(eCanFD_ALL,eCanFD_Control_On);
			else if(strcmp(argv[3], "off") == 0) 	CFD_SetLedControl(eCanFD_ALL,eCanFD_Control_On);
			else {}
		}
	}
	else if(strcmp(argv[1], "serial") == 0)
	{
		if(strcmp(argv[2], "read") == 0)
			CFD_ReadSerial();
		else if(strcmp(argv[2], "write") == 0)
			CFD_ReadSerial();
	}
	else if(strcmp(argv[1], "setver") == 0)
	{
	  	stCANFDBoardUpdateInfo stFDBoardInfo;
		GetAutolinkConfigProperty(eAutoLinkConfig_FDBoardUpdateInfo,(void*)&stFDBoardInfo);
	  	if( strcmp(argv[2], "0") == 0 )
		{
		  	CFD_SetCanFDVersion(0x5000,0x5000,0x5000,0x5000);
		}
		else
		{
			CFD_SetCanFDVersion(0x1,0x1,0x1,0x1);
			stFDBoardInfo.usBootVersion = atoi(argv[2]);
			stFDBoardInfo.usAppVersion = atoi(argv[2]);
		}
		SetAutolinkConfigProperty(eAutoLinkConfig_FDBoardUpdateInfo,(void*)&stFDBoardInfo);
	}
	else if(strcmp(argv[1], "start") == 0)
	{
		m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.eNeedToUpdate = eCANFD_BOARD_NEED_UPDATE_BOTH;
		m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiBootSize = 26181;
		m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootCheckSum = 0xb256;
		m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiAppSize = 57984;
	  	m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppCheckSum = 0x3375;
		CFD_SetFWUpdateStart(m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion,m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiBootSize,CANFD_UPDATE_BOOT);
	}
	else if(strcmp(argv[1], "end") == 0)
	{
		CFD_SetFWUpdateEnd(m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootCheckSum);
	}
	else if(strcmp(argv[1], "version") == 0)
	{
for(int afd=0; afd<10; afd++)
{
		CFD_GetCanFDVersion();

        APP_Delay(1000);
}
	}
	else if(strcmp(argv[1], "reset") == 0)
	{
		CFD_Reset();
	}
	else if(strcmp(argv[1], "sleep") == 0)
	{
		CFD_Sleep();
	}
	else if(strcmp(argv[1], "baudrate") == 0)
	{
		if(strcmp(argv[2], "get") == 0)
			CFD_GetCanBaudRate();
		else if(strcmp(argv[2], "set") == 0)
		{
			ucCanData[0] = atoi(argv[3]);
			ucCanData[1] = atoi(argv[4]);
			ucCanData[2] = atoi(argv[5]);
			ucCanData[3] = atoi(argv[6]);

			hexdump(ucCanData,LENGTH_CANFRAME_DATA);

			CFD_SetCanBaudRate(ucCanData);
		}
	}
	else if(strcmp(argv[1], "batteryread") == 0)
	{
		CFD_ReadBatteryVoltage();
	}
	else if(strcmp(argv[1], "analogsw") == 0)
	{

		/*
#define CANFD_ANALOG_SWITCH_FD4		0
#define CANFD_ANALOG_SWITCH_FD5		1
		*/

		if(strcmp(argv[2], "4") == 0)
		{
			ucCanData[0] = CANFD_ANALOG_SWITCH_FD4;
			CFD_SetAnalogSwitch(ucCanData[0]);
		}
		else if(strcmp(argv[2], "5") == 0)
		{
			ucCanData[0] = CANFD_ANALOG_SWITCH_FD5;
			CFD_SetAnalogSwitch(ucCanData[0]);
		}
	}
	else if(strcmp(argv[1], "canline") == 0)
	{
		if(strcmp(argv[2], "get") == 0)
			CFD_GetCanLine();
		else if(strcmp(argv[2], "set") == 0)
		{
			ucCanData[0] = atoi(argv[3]);
			ucCanData[1] = atoi(argv[4]);
			ucCanData[2] = atoi(argv[5]);
			ucCanData[3] = atoi(argv[6]);

			hexdump(ucCanData,LENGTH_CANFRAME_DATA);

			CFD_SendCanLine(ucCanData);
		}
	}
	else if(strcmp(argv[1], "masking") == 0)
	{
		if(strcmp(argv[2], "clear") == 0)
		{
			ucCanData[0] = atoi(argv[3]);
			CFD_SetClearCanMasking(ucCanData[0]);
		}
		else if(strcmp(argv[2], "get") == 0)
		{
			ucCanData[0] = atoi(argv[3]);
			CFD_GetCanMaskingInfo(ucCanData[0]);
		}
		else if(strcmp(argv[2], "set1") == 0)
		{
			ucCanData[0] = 1;
			ucCanData[1] = 0x13;
			ucCanData[2] = 0x01;
			ucCanData[3] = 0x02;
			ucCanData[4] = 0x03;
			ucCanData[5] = 0x04;
			ucCanData[6] = 0x05;
			ucCanData[7] = 0x06;

			//CFD_SetCanMasking(ucCanData[0],ucCanData[1],&ucCanData[2]);
                        CFD_SetCanMasking(ucCanData[0],ucCanData[1],3,&ucCanData[2]);
		}
		else if(strcmp(argv[2], "set2") == 0)
		{
			ucCanData[0] = 2;
			ucCanData[1] = 0x13;
			ucCanData[2] = 0x03;
			ucCanData[3] = 0x04;
			ucCanData[4] = 0x05;
			ucCanData[5] = 0x06;
			ucCanData[6] = 0x07;
			ucCanData[7] = 0x08;

			CFD_SetCanMasking(ucCanData[0],ucCanData[1],3,&ucCanData[2]);
		}
		else if(strcmp(argv[2], "set3") == 0)
		{
			ucCanData[0] = 3;
			ucCanData[1] = 0x13;
			ucCanData[2] = 0x01;
			ucCanData[3] = 0x01;
			ucCanData[4] = 0x02;
			ucCanData[5] = 0x02;
			ucCanData[6] = 0x03;
			ucCanData[7] = 0x03;

			CFD_SetCanMasking(ucCanData[0],ucCanData[1],3,&ucCanData[2]);
		}
		else if(strcmp(argv[2], "set4") == 0)
		{
			ucCanData[0] = 4;
			ucCanData[1] = 0x13;
			ucCanData[2] = 0x03;
			ucCanData[3] = 0x03;
			ucCanData[4] = 0x04;
			ucCanData[5] = 0x04;
			ucCanData[6] = 0x05;
			ucCanData[7] = 0x05;

			CFD_SetCanMasking(ucCanData[0],ucCanData[1],3,&ucCanData[2]);
		}
	}
}


/*
 * NAME
 *    void
 *    app_cli_init()
 *
 * DESCRIPTION
 *    Registers the application commands with the cli.
 *    The [app] directory is added under [show].
 *
 * INPUT PARAMETERS
 *    None.
 *
 * RETURN VALUE
 *    None.
 *
 */
void app_cli_init (void)
{
	RC_CLI_t rc;

	/*
	* Create app directory and commands
	*/
//	rc = cli_mkdir("modem", NULL, &cli_modem_dir);
	rc = cli_mkdir("app", NULL, &cli_app_dir);
	rc = cli_mkdir("operation", &cli_app_dir, &cli_operation_dir);

	rc = cli_mkcmd("test", 	app_systest_cmd, NULL, &app_op_systest_cmd_record);
	rc = cli_mkcmd("t", 	app_test_cmd, NULL, &app_op_test_cmd_record);				// HW team���� ��û�� test �Լ�
	rc = cli_mkcmd("fs", 		fs_op_cmd, 		NULL, &fs_op_cmd_record);
	rc = cli_mkcmd("msg", 	msg_op_cmd, NULL, &msg_op_cmd_record);
	rc = cli_mkcmd("modem", modem_op_cmd, NULL, &modem_op_cmd_record2);
	rc = cli_mkcmd("ble", 	ble_op_cmd, 	NULL, &ble_op_cmd_record);
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	rc = cli_mkcmd("usb", 	usb_op_cmd, 	NULL, &usb_op_cmd_record);
#endif
	rc = cli_mkcmd("power", power_op_cmd, 	NULL, &power_op_cmd_record);
	rc = cli_mkcmd("can", 	app_can_cmd, NULL, &app_op_can_cmd_record);
    rc = cli_mkcmd("dbg", debug_op_cmd,   NULL, &debug_op_cmd_record);
	rc = cli_mkcmd("obd", obd_op_cmd,   NULL, &obd_op_cmd_record);
	rc = cli_mkcmd("start", start_op_cmd,   NULL, &start_op_cmd_record);
	rc = cli_mkcmd("stop", stop_op_cmd,   NULL, &stop_op_cmd_record);
	rc = cli_mkcmd("cfd", cfd_op_cmd,   NULL, &cfd_op_cmd_record);
	rc = cli_mkcmd("testmode", testmode_op_cmd, NULL, &testmode_op_cmd_record);

	rc = cli_mkcmd("enable", app_op_enable_cmd, &cli_app_dir, &app_op_enable_cmd_record);
	rc = cli_mkcmd("disable", app_op_disable_cmd, &cli_app_dir, &app_op_disable_cmd_record);
	rc = cli_mkcmd("run", app_op_disable_cmd, &cli_modem_dir, &modem_op_run_cmd_record);

	rc = rc;

	return;
}
