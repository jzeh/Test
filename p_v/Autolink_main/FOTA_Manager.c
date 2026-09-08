/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <ctype.h>
#include "Modem_Manager.h"
#include "FOTA_manager.h"
#include "GIT_Util.h"
#include "Share_InterFunction.h"
#include "modem_comm.h"
#include "UARTDMA_Manager.h"
#include "Power_Manager.h"
#include "Message_Make.h"
#include "CanFD_Defines.h"
#include "HalHandler.h"

/* Define ------------------------------------------------------------------*/
#define MAX_BYTE_BUFFER_LEN				1024
#define FOTA_LOG
/* Variable ------------------------------------------------------------------*/
FOTA_MANAGER_DATA           g_FotaManagerData;
stUpdateFileInfo            g_stUpdateFileInfoList;
stVehicleInfo               g_stVehicleInfoList;
eMODEM_RECEIVE_DATA_TYPE    geModemReceiveDataType;

extern BR_SystemInfo        BkSram_SystemInfo;
extern stCFDControl         m_stCFDCtrl;
extern uint16_t	            g_u16UpdateFileCheckSum;
extern uint32_t 	        g_u32UpdateFileSize;
extern stDownloadStartReq   g_DownloadInfo;
extern uint16_t	            g_u16UpdateFileCheckSum;


const stCountryMakerAreaInfo stTableCountryMakerAreaInfo[] =
{
 {"KR", "H", "HMC\x00"},
 {"KR", "K", "KMC\x00"},
 {"AU", "H", "HME\x00"},
 {"AU", "K", "KME\x00"},
 {"NZ", "H", "HME\x00"},
 {"NZ", "K", "KME\x00"},
#if defined(GIT_FLEET)
 {"GF", "H", "HMC\x00"},
 {"GF", "K", "KMC\x00"},
#endif
 {"VN", "H", "HME\x00"},
 {"VN", "K", "KME\x00"},
 {"RU", "H", "HME\x00"},
 {"RU", "K", "KME\x00"},
 {"SG", "H", "HME\x00"},
 {"SG", "K", "KME\x00"},
#if defined(QA_FIFA)
 {"QA", "H", "HME\x00"},
 {"QA", "K", "KME\x00"},
#endif
};


/* Function ------------------------------------------------------------------*/
#if defined (OLD_FOTA)
bool FOTA_MakeRequestBinDataContent(void);
#else
bool NEWFOTA_MakeRequestBinDataContent(void);
#endif

void FOTA_SetDownloadFileType(void);
bool FOTA_CheckFileCheckSum(void);
bool FOTA_EraseInternalFlashSection(eFOTA_FILE_TYEP eType);
bool FOTA_UpdateFileToInternalFlash(eFOTA_FILE_TYEP eFileNo);
bool FOTA_DownloadClose(void);

extern void SetRegModemProcessFunction(modem_process_fn processFunction, modem_process_fn resultFunction, MODEM_RESPONSE_TYPE eType);
extern eMODEM_PROCESS_FUNC_RET ProcessHTTPComm(void);
extern eMODEM_PROCESS_FUNC_RET ProcessHTTPCommResult_FOTAGetBin(void);

extern void DisplayFirmWareInfo(void);
extern void GPS_DisableCommunication(void);
/* ---------------------------------------------------------------------------*/


//*********************************************************************************
void InitializeFotaMager(void)
{
	g_FotaManagerData.eState = eFOTA_STATE_INIT;
	g_FotaManagerData.eFotaResultCode = eFOTA_RESULT_CODE_NONE;

	return;
}

void SetModeFotaResultCode(eFOTA_RESULT_CODE state)
{
	g_FotaManagerData.eFotaResultCode = state;
}

eFOTA_RESULT_CODE GetModeFotaResultCode(void)
{
	return g_FotaManagerData.eFotaResultCode;
}

void FOTA_GetVersion(void)
{
    // FOTA
    printf("\r\n\r\n*********************************\r\n");
    printf("FOTA: GET VERSION\r\n");

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

    stHalRTCTypeDef stHalRtcDateTime;
    APP_TimeShow(&stHalRtcDateTime);

	g_FotaManagerData.eFotaResultCode = eFOTA_RESULT_CODE_NONE;

	ModemManagerData.nInternetConnectionProfileId = MODEM_INTERNET_CONNECTION_ID_FOTA_REQUEST_FILE_INFO;
	ModemManagerData.nInternetServiceProfileId = MODEM_INTERNET_SERVICE_ID_FOTA_REQUEST_FILE_INFO;

	ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;

    ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_VERSION;
    SetModemState(eMODEM_READY);

	return;
}

void FOTA_GetBin(void)
{
    // FOTA
    printf("FOTA: GET BIN\r\n");

    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
    HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);
    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

    ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_BIN;

    stHalRTCTypeDef stHalRtcDateTime;
    APP_TimeShow(&stHalRtcDateTime);

    g_FotaManagerData.eFotaResultCode = eFOTA_RESULT_CODE_NONE;

    ModemManagerData.nInternetConnectionProfileId = MODEM_INTERNET_CONNECTION_ID_FOTA_REQUEST_FILE_INFO;
    ModemManagerData.nInternetServiceProfileId = MODEM_INTERNET_SERVICE_ID_FOTA_REQUEST_FILE_INFO;

    ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;

    g_FotaManagerData.eState = eFOTA_STATE_INIT;

    SetModemState(eMODEM_READY);

	return;
}

void FOTA_GetVehicleInfo(void)
{
    // FOTA
    printf("\r\n\r\n*********************************\r\n");
    printf("FOTA: GET VEHICLE INFO\r\n");

    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
    HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);
    HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

    stHalRTCTypeDef stHalRtcDateTime;
    APP_TimeShow(&stHalRtcDateTime);

    g_FotaManagerData.eFotaResultCode = eFOTA_RESULT_CODE_NONE;

    ModemManagerData.nInternetConnectionProfileId = MODEM_INTERNET_CONNECTION_ID_GET_VEHICLE_INFO;
    ModemManagerData.nInternetServiceProfileId = MODEM_INTERNET_SERVICE_GET_VEHICLE_INFO;

    ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;

    ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_VEHICLE_INFO;
    SetModemState(eMODEM_READY);

	return;
}

#if				0
	#define FOTA_PREFIX_FILE_NAME_BOOT				"AUTOLINKP_BOOT_V"
	#define FOTA_PREFIX_FILE_NAME_APP					"AUTOLINKP_MODULE_V"
	#define FOTA_PREFIX_FILE_NAME_MASTER_DB		"MASTER"
	#define FOTA_PREFIX_FILE_NAME_SLAVE_SB		"DCAG"
	#define FOTA_PREFIX_FILE_NAME_CONTROL_DB	"CONTROL"
#else
	#define FOTA_PREFIX_FILE_NAME_BOOT				"DCS_Bootloader"
	#define FOTA_PREFIX_FILE_NAME_APP					"DCS_Module"
	#define FOTA_PREFIX_FILE_NAME_MASTER_DB		"MASTER"
	#define FOTA_PREFIX_FILE_NAME_SLAVE_SB		"DC2C0B"
	#define FOTA_PREFIX_FILE_NAME_CONTROL_DB	"DCAG16"
#endif

eFOTA_FILE_TYEP FOTA_SelectDownloadFile(void)
{
	eFOTA_FILE_TYEP eFileType;
	stUpdateFileInfo *ptrFileInfo;

	ptrFileInfo = &g_stUpdateFileInfoList;

	printf("\n@FOTA_SelectDownloadFile()\r\n");

	printf("list no: %d\r\n", g_FotaManagerData.nCurrDownloadFileNo);
	printf("order: %d\r\n", ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder);
	printf("version: %d\r\n", ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion);
	printf("file name: [%s]\r\n", ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName);
	printf("size: %d\r\n", ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileSize);
	printf("checksum: 0x%x(%d)\r\n", ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileCheckSum, ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileCheckSum);

	eFileType = eFOTA_FILE_TYPE_NONE;

	DisplayFirmWareInfo();

	if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder == eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT) {       
		if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion > g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion 
            || g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion == 0
            || BkSram_SystemInfo.bTestFota == true ) {
			eFileType = eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT;
			g_FotaManagerData.wInternalFlashAddress = ADDR_BOOT_SAVE;
			printf("FOTA: update - eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT\r\n");
		}
		else {
			printf("FOTA: No update required - bootloader\r\n");
		}
	}
	else if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder == eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP) {
		if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion > g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion 
            || g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion == 0
            || BkSram_SystemInfo.bTestFota == true ) {
			eFileType = eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP;
			g_FotaManagerData.wInternalFlashAddress = ADDR_APPLICATION_SAVE1;
			printf("FOTA: update - eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP\r\n");
		}
		else {
			printf("FOTA: No update required - application\r\n");
		}
	}
	else if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder == eFOTA_FILE_TYPE_DIAGNOSIS_MASTER) {
		if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion > g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion 
            || g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion == 0
            || BkSram_SystemInfo.bTestFota == true ) {
			eFileType = eFOTA_FILE_TYPE_DIAGNOSIS_MASTER;
			g_FotaManagerData.wInternalFlashAddress = ADDR_MASTER_DB_SAVE;
			printf("FOTA: update - eFOTA_FILE_TYPE_DIAGNOSIS_MASTER\r\n");
		}
		else {
			printf("FOTA: No update required - master db\r\n");
		}
	}
	else if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder == eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE) {
		if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion > g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion 
            || g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion == 0
            || BkSram_SystemInfo.bTestFota == true ) {
			eFileType = eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE;
			g_FotaManagerData.wInternalFlashAddress = ADDR_SLAVE_DB_SAVE;
			printf("FOTA: update - eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE\r\n");
		}
		else {
			printf("FOTA: No update required - slave db\r\n");
		}
	}
	else if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder == eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL) {
		if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion > g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion 
            || g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion == 0
            || BkSram_SystemInfo.bTestFota == true ) {
			eFileType = eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL;
			g_FotaManagerData.wInternalFlashAddress = ADDR_CONTROL_DB_SAVE;
			printf("FOTA: update - eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL\r\n");
		}
		else {
			printf("FOTA: No update required - control db\r\n");
		}
	}
#if defined(EXTBOARD_FOTA)
	else if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder == eFOTA_FILE_TYPE_FDBOARD_BOOT || 
            ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder == eFOTA_FILE_TYPE_FDBOARD_ARTERY_BOOT ) 
    {
		if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion > m_stCFDCtrl.stVer.usBlVer
            || m_stCFDCtrl.stVer.usBlVer == 0
            || BkSram_SystemInfo.bTestFota == true ) {
			eFileType = eFOTA_FILE_TYPE_FDBOARD_BOOT;
			g_FotaManagerData.wInternalFlashAddress = ADDR_CONTROL_DB_SAVE;
			printf("FOTA: update - eFOTA_FILE_TYPE_FDBOARD_BOOT\r\n");
		}
		else {
			printf("FOTA: No update required - Ext Boot\r\n");
		}
	}
	else if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder == eFOTA_FILE_TYPE_FDBOARD_APP ||
            ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_cOrder == eFOTA_FILE_TYPE_FDBOARD_ARTERY_APP ) 
    {
		if(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion > m_stCFDCtrl.stVer.usAppVer
            || m_stCFDCtrl.stVer.usAppVer == 0
            || BkSram_SystemInfo.bTestFota == true ) {
			eFileType = eFOTA_FILE_TYPE_FDBOARD_APP;
			g_FotaManagerData.wInternalFlashAddress = ADDR_CONTROL_DB_SAVE;
			printf("FOTA: update - eFOTA_FILE_TYPE_FDBOARD_APP\r\n");
		}
		else {
			printf("FOTA: No update required - Ext App\r\n");
		}
	}
#endif
	else {
		printf("FOTA: No update required\r\n");
		return eFOTA_FILE_TYPE_NONE;
	}

	return eFileType;
}

void FOTA_SetModemCommProfileGetBin(void)
{
	g_FotaManagerData.eFotaResultCode = eFOTA_RESULT_CODE_NONE;

	ModemManagerData.nInternetConnectionProfileId = MODEM_INTERNET_CONNECTION_ID_FOTA_REQUEST_FILE_DOWNLOAD;
	ModemManagerData.nInternetServiceProfileId = MODEM_INTERNET_SERVICE_ID_FOTA_REQUEST_FILE_DOWNLOAD;

//	printf("Internet Connection Profile Id: %d\r\n", ModemManagerData.nInternetConnectionProfileId);
//	printf("Internet Service Profile Id: %d\r\n", ModemManagerData.nInternetServiceProfileId);
//	printf("Internet Service URL: %s\r\n", ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL);
//	printf("Write Data length: %d\r\n", ModemManagerData.nWriteDataLength);

	return;
}

eMODEM_PROCESS_FUNC_RET FOTA_RecvBinProcess(void)
{
	bool ret;

	switch(g_FotaManagerData.eState) {
		case eFOTA_STATE_INIT:
			printf("FOTA: run @FOTA_RecvBinProcess()\r\n");
			g_FotaManagerData.eState = eFOTA_STATE_CHECK_NETWORK_STATE;
			break;

		case eFOTA_STATE_CHECK_NETWORK_STATE:
//			if(ModemManagerData.nRSSI < MODEM_SIGNAL_QUILITY_STABLE_VALUE) {
//				// ?�테?��? ?�착?��? ?��? ?�태?�서 ModemManagerData.nRSSI 값�? 6 ~ 8 ?�도??값을 같는??
//				// RSSI 값이 10보다 ?�을 경우?�는 ?�테?��? ?�착?��? ?��? 경우�??�단?�고 FOTA�??�행?��? ?�도�??�다.
//				printf("FOTA: low signal quility.\r\n");
//				g_FotaManagerData.eFotaResultCode = eFOTA_RESULT_CODE_LOW_SIGNAL_QUILITY;
//				printf("\n\nFOTA: @FOTAStop() code: %d\r\n\r\n", g_FotaManagerData.eFotaResultCode);
//
//				ModemManagerData.bModemDataSaveToFlashFlag = false;
//				geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;
//
//				return eMODEM_PROCESS_FUNC_RET_FAIL;
//			}

			if(ModemManagerData.eNetworkRegStatus != eNETWORK_REG_STATUS_REGISTERED && ModemManagerData.eNetworkRegStatus != eNETWORK_REG_STATUS_REG_ROAMING) {
				printf("FOTA: network is not registered, stop FOTA.\r\n");
				g_FotaManagerData.eFotaResultCode = eFOTA_RESULT_CODE_NOT_NWETWORK_REGISTERED;
				printf("\n\nFOTA: @FOTAStop() code: %d\r\n\r\n", g_FotaManagerData.eFotaResultCode);

				ModemManagerData.bModemDataSaveToFlashFlag = false;
				geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;

				return eMODEM_PROCESS_FUNC_RET_FAIL;
			}

			g_FotaManagerData.eState = eFOTA_STATE_DISABLE_GPS;
			break;

		case eFOTA_STATE_DISABLE_GPS:
			SetLedOnOffCtl(LED_OFF, eLED_GPS);

			printf("FOTA: Disable GPS\r\n");

			// GPS??UART4�?channel??막아 ?�다.
			GPS_DisableCommunication();

			g_FotaManagerData.nMaxDownloadFileNo = g_stUpdateFileInfoList.m_nFileNo;
			g_FotaManagerData.nCurrDownloadFileNo = 0;
			g_FotaManagerData.eCurrFotaFileType = eFOTA_FILE_TYPE_NONE;

			printf("FOTA: state: *eFOTA_STATE_CONFIRM_FILE_VERSION\r\n");
			g_FotaManagerData.eState = eFOTA_STATE_SELECT_DOWNLOAD_FILE;
			break;

		case eFOTA_STATE_SELECT_DOWNLOAD_FILE:
			g_FotaManagerData.eCurrFotaFileType = FOTA_SelectDownloadFile();

			if(g_FotaManagerData.eCurrFotaFileType != eFOTA_FILE_TYPE_NONE) {
				g_FotaManagerData.eState = eFOTA_STATE_ERASE_INTERNAL_FLASH;
			}
			else {
				g_FotaManagerData.eState = eFOTA_STATE_CHECK_CONTINUE;
			}
			break;

		case eFOTA_STATE_ERASE_INTERNAL_FLASH:
			// ?�운로드 받을 internal flash ?�역????��?�자.
			ret = FOTA_EraseInternalFlashSection(g_FotaManagerData.eCurrFotaFileType);
			if(ret == true) {
				g_FotaManagerData.eState = eFOTA_STATE_MAKE_REQUEST_CONTENT;
			}
			else {
				FOTAStop(eFOTA_RESULT_CODE_ERROR_FLASH_ERASE_FAIL);
			}
			break;

		case eFOTA_STATE_MAKE_REQUEST_CONTENT:
			FOTA_SetModemCommProfileGetBin();

			//*********************************************************************
			// file ?�보�??�해??download??file data�??�청?�다.
			//*********************************************************************
			printf("FOTA: make statement of file download\r\n");
#if defined (OLD_FOTA)
			FOTA_MakeRequestBinDataContent();
#else
			NEWFOTA_MakeRequestBinDataContent();
#endif
			//*********************************************************************

			//
			printf(" Set & Run process fota function\r\n");
			SetRegModemProcessFunction(ProcessHTTPComm, ProcessHTTPCommResult_FOTAGetBin, MODEM_RESPONSE_TYPE_FOTA);

			g_FotaManagerData.eState = eFOTA_STATE_WAIT_FILE_DOWNLOAD_COMPLETE;
			break;

		case eFOTA_STATE_WAIT_FILE_DOWNLOAD_COMPLETE:
			break;

		case eFOTA_STATE_CONFIRM_CHECKSUM:
			ret = FOTA_CheckFileCheckSum();
			if(ret == true) {
				g_FotaManagerData.eState = eFOTA_STATE_UPDATE_BIN_DATA;
			}
			else {
				FOTAStop(eFOTA_RESULT_CODE_ERROR_MISSING_FILE_CHECKSUM);
				break;
			}
			break;

		case eFOTA_STATE_UPDATE_BIN_DATA:
			ret = FOTA_UpdateFileToInternalFlash(g_FotaManagerData.eCurrFotaFileType);
			if(ret == true) {
				g_FotaManagerData.eState = eFOTA_STATE_CHECK_CONTINUE;
			}
			else {
				FOTAStop(eFOTA_RESULT_CODE_ERROR_UPDATE_BINDATA_FAIL);
			}
			break;

		case eFOTA_STATE_CHECK_CONTINUE:
			// ???�운로드?�야 ?�는 ?�일???�는지 ?�인??보자
			g_FotaManagerData.nCurrDownloadFileNo++;
			if(g_FotaManagerData.nCurrDownloadFileNo >= g_FotaManagerData.nMaxDownloadFileNo) {
				printf("\r\n");
				printf("FOTA: *FOTA All done!!!!\r\n");
				FOTAStop(eFOTA_RESULT_CODE_SUCCESS);
				break;
			}
			else {
				printf("FOTA: *eFOTA_STATE_SELECT_DOWNLOAD_FILE\r\n");
				g_FotaManagerData.eState = eFOTA_STATE_SELECT_DOWNLOAD_FILE;
			}
			break;
			break;

		case eFOTA_STATE_ENABLE_GPS:
			printf("\n");
			printf("GPS: Enable Comm\r\n");
			GPS_EnableCommunication();

			SetLedOnOffCtl(LED_ON, eLED_GPS);

			g_FotaManagerData.eState = eFOTA_STATE_STOP;
			break;

		case eFOTA_STATE_STOP:
			if(g_FotaManagerData.eFotaResultCode == eFOTA_RESULT_CODE_SUCCESS) {
				printf("FOTA: eMODEM_PROCESS_FUNC_RET_OK\r\n");
				return eMODEM_PROCESS_FUNC_RET_OK;
			}
			else {
				printf("FOTA: eMODEM_PROCESS_FUNC_RET_FAIL\r\n");
				return eMODEM_PROCESS_FUNC_RET_FAIL;
			}
			break;
	}

	return eMODEM_PROCESS_FUNC_RET_CONTINUE;
}

bool FOTA_EraseInternalFlashSection(eFOTA_FILE_TYEP eType)
{
    bool bResult = true;
	printf("FLASH: erase internal flash...\r\n");

    __disable_irq();
    
	switch(eType) {
		case eFOTA_FILE_TYPE_DIAGNOSIS_MASTER:
            if (HalDrvFlashErase(ADDR_MASTER_DB_SAVE, ADDR_SLAVE_DB_SAVE - 1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
				bResult = false;
			}

			printf("FLASH: erase ADDR_MASTER_DB_SAVE ~ ADDR_SLAVE_DB_SAVE - 1\r\n");
			printf("FLASH: erase 0x08108000 ~ 0x0810BFFF\r\n");
			break;

		case eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE:
            if (HalDrvFlashErase(ADDR_SLAVE_DB_SAVE, ADDR_APPLICATION_SAVE1 - 1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
				bResult = false;
			}

			printf("FLASH: erase ADDR_SLAVE_DB_SAVE ~ ADDR_APPLICATION_SAVE1 - 1\r\n");
			printf("FLASH: erase 0x0810C000 ~ 0x0810FFFF\r\n");
			break;

		case eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL:
            if (HalDrvFlashErase(ADDR_CONTROL_DB_SAVE, ADDR_END_SAVE-1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
				bResult = false;
			}

			printf("FLASH: erase ADDR_CONTROL_DB_SAVE ~ ADDR_END_SAVE\r\n");
			printf("FLASH: erase 0x08180000 ~ 0x0818FFFF\r\n");
			break;

		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT:
            if (HalDrvFlashErase(ADDR_BOOT_SAVE, ADDR_FW_INFO_SAVE-1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
				bResult = false;
			}

			printf("FLASH: erase ADDR_BOOT_SAVE ~ ADDR_FW_INFO_SAVE - 1\r\n");
			printf("FLASH: erase 0x08100000 ~ 0x08103FFF\n");
			break;

		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP:
            if (HalDrvFlashErase(ADDR_APPLICATION_SAVE1, ADDR_CONTROL_DB_SAVE-1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
				bResult = false;
			}

			printf("FLASH: erase ADDR_APPLICATION_SAVE1 ~ ADDR_CONTROL_DB_SAVE - 1\r\n");
			printf("FLASH: erase 0x08110000 ~ 0x081BFFFF\r\n");
			break;
#if defined(EXTBOARD_FOTA)
		case eFOTA_FILE_TYPE_FDBOARD_BOOT:
            if (HalDrvFlashErase(ADDR_CONTROL_DB_SAVE, ADDR_END_SAVE-1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
				bResult = false;
			}
			printf("FLASH: erase ADDR_CONTROL_DB_SAVE ~ ADDR_END_SAVE - 1\r\n");
			printf("FLASH: erase 0x08120000 ~ 0x0813FFFF\r\n");
			
			//for(int i=0;i<SECTOR_EXBOARD_BOOT_FIRMWARE_SECTOR_SIZE;i++)
			//{
			//	sFLASH_EraseSubSector((SECTOR_EXBOARD_BOOT_FIRMWARE + i) * SFLASH_SECTOR_SIZE);
			//}
			//printf("SFLASH: 007f 5000h - 007f cfffh	//32kb");
			break;
		case eFOTA_FILE_TYPE_FDBOARD_APP:
            if (HalDrvFlashErase(ADDR_CONTROL_DB_SAVE, ADDR_END_SAVE-1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
				bResult = false;
			}
			printf("FLASH: erase ADDR_CONTROL_DB_SAVE ~ ADDR_END_SAVE - 1\r\n");
			printf("FLASH: erase 0x08120000 ~ 0x0813FFFF_\r\n");
			
			//for(int i=0;i<SECTOR_EXBOARD_APP_FIRMWARE_SECTOR_SIZE;i++)
			//{
			//	sFLASH_EraseSubSector((SECTOR_EXBOARD_APP_FIRMWARE + i) * SFLASH_SECTOR_SIZE);
			//}
			//printf("SFLASH: 007D 5000h - 007f 4fffh  //128kb\r\n");
			break;
#endif
	}

    __enable_irq();

	return bResult;
}

bool FOTA_SaveBinDataToInternalFlash(uint8_t* Data, uint16_t DataLength)
{
	static bool bToggle;
//	printf("FOTA: Write(0x%x)\r\n", g_FotaManagerData.wInternalFlashAddress);
	printf(".");

	__disable_irq();

	SetLedOnOffCtl(bToggle, eLED_SERVER);
	bToggle = ~bToggle;

	if(HalDrvFlashWriteByteCallByRef((uint32_t*)&g_FotaManagerData.wInternalFlashAddress, (uint8_t*)Data, DataLength) == HAL_RETURN_SUCCESS )
    {
		g_FotaManagerData.bFlashWriteSuccess = true;

		__enable_irq();

		SetLedOnOffCtl(LED_OFF, eLED_SERVER);
//		printf(".\n");
		return true;
	}

	__enable_irq();

	g_FotaManagerData.bFlashWriteSuccess = false;
	SetLedOnOffCtl(LED_ON, eLED_SERVER);

	return false;
}

void Convert_Ascii_To_HexString(char *pHexBuf, char *pAsciiBuf)
{
	int len, i;

	len = strlen((char *)pAsciiBuf);

//	printf("\n\n[");

	for(i = 0; i < len; i++) {
//		printf("%c", pAsciiBuf[i]);
		sprintf(&pHexBuf[i * 2], "%02x", pAsciiBuf[i]);
	}

//	printf("]\n\n");

	return;
}

void Convert_Ascii_To_HexString_len(char *pHexBuf, char *pAsciiBuf, int length)
{
	int len, i;

	len = length;
//	printf("\n\n[");
	for(i = 0; i < len; i++) {
//		printf("%c", pAsciiBuf[i]);
		sprintf(&pHexBuf[i * 2], "%02X", pAsciiBuf[i]);
	}
//	printf("]\n\n");	
	return;
}

u16 Convert_HexString_To_Bin_URL(char *pBinBuf, char *pHexBuf, int nLength)
{
	u16 i, j;
	u16 n;
	u8 temp;
	u8 data;

	n = nLength * 2;

	for(i = 0, j = 0; i < n; i += 2, j++) {
		temp = 0;

		data = pHexBuf[i];
//		printf("%c + ", data);
		if(data >= '0' && data <= '9') {
			data -= '0';
		}
		else if(data >= 'a' && data <= 'f') {
			data = data - 'a' + 10;
		}
		else if(data >= 'A' && data <= 'F') {
			data = data - 'A' + 10;
		}
		else {
//			printf("1. error!!!(%x)(%x)\r\n", data, pHexBuf[i]);

#if				0
		printf("\nRcv Data: %x\n", j);
		hexdump(pHexBuf, nLength);
#endif
			pBinBuf[j] = 0xFF;
			continue;
		}

		temp |= data;
		temp <<= 4;

		data = pHexBuf[i + 1];
//		printf("%c = ", data);
		if(data >= '0' && data <= '9') {
			data -= '0';
		}
		else if(data >= 'a' && data <= 'f') {
			data = data - 'a' + 10;
		}
		else if(data >= 'A' && data <= 'F') {
			data = data - 'A' + 10;
		}
		else {
			printf("2. error!!!(%x)(%x)(%x)\r\n", data, pHexBuf[i + 1], j);
			printf("2. error!!!(%x)(%x)(%x)\r\n", temp, pHexBuf[i + 1], j);

#if				0
		printf("\nRcv Data: %d\r\n", i);
		hexdump(pHexBuf, nLength);
#endif
		}
		temp |= data;
//		printf("0x%02x\r\n", temp);
		pBinBuf[j] = temp;
		if(pBinBuf[j] == 0x00 || pBinBuf[j] == 0x20)	break;	//NULL�̳� Space�� ������ URL����
	}

	pBinBuf[j] = 0x00;

	return j;
}

u16 Convert_HexString_To_Bin(char *pBinBuf, char *pHexBuf, int nLength)
{
	u16 i, j;
	u16 n;
	u8 temp;
	u8 data;

	n = nLength * 2;

	for(i = 0, j = 0; i < n; i += 2, j++) {
		temp = 0;

		data = pHexBuf[i];
//		printf("%c + ", data);
		if(data >= '0' && data <= '9') {
			data -= '0';
		}
		else if(data >= 'a' && data <= 'f') {
			data = data - 'a' + 10;
		}
		else if(data >= 'A' && data <= 'F') {
			data = data - 'A' + 10;
		}
		else {
//			printf("1. error!!!(%x)(%x)\r\n", data, pHexBuf[i]);

#if 0
		printf("\nRcv Data: %x\r\n", j);
		hexdump(pHexBuf, nLength);
#endif
			pBinBuf[j] = 0xFF;
			continue;
		}

		temp |= data;
		temp <<= 4;

		data = pHexBuf[i + 1];
//		printf("%c = ", data);
		if(data >= '0' && data <= '9') {
			data -= '0';
		}
		else if(data >= 'a' && data <= 'f') {
			data = data - 'a' + 10;
		}
		else if(data >= 'A' && data <= 'F') {
			data = data - 'A' + 10;
		}
		else {
			printf("2. error!!!(%x)(%x)(%x)\r\n", data, pHexBuf[i + 1], j);
			printf("2. error!!!(%x)(%x)(%x)\r\n", temp, pHexBuf[i + 1], j);

#if				0
		printf("\nRcv Data: %d\n", i);
		hexdump(pHexBuf, nLength);
#endif
//			while(1);
		}

		temp |= data;

//		printf("0x%02x\n", temp);

		pBinBuf[j] = temp;
	}

	pBinBuf[j + 1] = 0x00;

	return j;
}

void FOTAStop(eFOTA_RESULT_CODE eRetCode)
{
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

	g_FotaManagerData.eState = eFOTA_STATE_ENABLE_GPS;
	g_FotaManagerData.eFotaResultCode = eRetCode;

	ModemManagerData.bModemDataSaveToFlashFlag = false;
	geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;

	ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;

	printf("\n\nFOTA: @FOTAStop() code: %d\r\n\r\n", g_FotaManagerData.eFotaResultCode);

	return;
}

// 문자?�의 ?�에??부??비교?�서 ?�자가 ?�작?�는 ?�치�?찾아 보자
uint16_t SearchStringVersionToInt(u8 *ptrString)
{
	u8 *p;
	uint16_t len;
	uint16_t i,j;
	u8 temp[4];

	p = ptrString;
	len = strlen((char *)ptrString);
//	for(i = len - 1; i > 0; i--) {
//		if(p[i] < '0' || p[i] > '9') {
//			p = p + i + 1;
//			break;
//		}
//	}
//	i = atoi((const char *)p);
	memset(temp,0x00,sizeof(temp));
	if(len <10)
	{
		for(i=len,j=3;i>0;i--)
		{
			if(p[i-1]>='0' && p[i-1]<='9')
			{
				temp[j]=p[i-1]-0x30;
				j--;
			}
			else
			{
				temp[j]=p[i-1]-0x37;
				j--;
			}
		}
	}
	else
	{
		for(i=len-1;i>0;i--)
		{
			if(p[i]=='_')
			{
				//printf(" ~~%d ~~",i);
				break;
			}
		}

		for(i=i+2,j=0;i<len;i++)
		{
			if(p[i]>='0' && p[i]<='9')
			{
				temp[j]=p[i]-0x30;
				j++;
			}
			else
			{
				temp[j]=p[i]-0x37;
				j++;
			}
		}
	}
	//printf(" ~~%d %d %d %d ~~",temp[0],temp[1],temp[2],temp[3]);
	i =  temp[0]*4096 + temp[1]*256 + temp[2]*16 + temp[3];
	//printf("!!!!!!!!!!!!!!!%X",i );

	return i;
}

uint16_t strcpy_upper(uint8_t *pDestStr, uint8_t *pSrcStr)
{
	uint16_t i;
	char c;

	i = 0;
	while(1) {
		c = (char)*pSrcStr;
		if(c != NULL) {
			*pDestStr = (uint8_t)toupper((int)c);
		}
		else {
			break;
		}

		pSrcStr++;
		pDestStr++;
		i++;
	}

	return i;
}

// { "USERID" : "AUTOLINK", "TIMESTAMP" : "20171219142852", "COUNT" : "5", "RESPONSELIST" : [
// { "ORDER" : "0", "FILE_NAME" : "DCS_Bootloader.bin", "FILE_SIZE" : "14036", "FILE_VER" : "0003", "FILE_CHECKSUM" : "1405862", "FOLDER_NAME" : "D-E-H-01-0022" },
// { "ORDER" : "1", "FILE_NAME" : "DCS_Module.bin", "FILE_SIZE" : "111240", "FILE_VER" : "0018", "FILE_CHECKSUM" : "11716836", "FOLDER_NAME" : "D-E-H-01-0022" },
// { "ORDER" : "2", "FILE_NAME" : "MASTER.fin", "FILE_SIZE" : "752", "FILE_VER" : "0001", "FILE_CHECKSUM" : "98315" },
// { "ORDER" : "3", "FILE_NAME" : "DC2C0B.fin", "FILE_SIZE" : "4336", "FILE_VER" : "0001", "FILE_CHECKSUM" : "662328" },
// { "ORDER" : "4", "FILE_NAME" : "DCAG16.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }
// { "ORDER" : "5", "FILE_NAME" : "CanFD_Bootloader_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"// 6: " "ORDER" : "5", "FILE_NAME" : "CanFD_Bootloader_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"// 6: " "ORDER" : "5", "FILE_NAME" : "CanFD_Bootloader_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"
// { "ORDER" : "6", "FILE_NAME" : "CanFDAppication_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"
// { "ORDER" : "7", "FILE_NAME" : "CanFDArtery_Application_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"
// { "ORDER" : "8", "FILE_NAME" : "CanFDArtery_Application_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"   

//#define ARGC_MAX_VERSION_INFO_ORDER				10
#define ARGC_MAX_VERSION_INFO_ORDER				(MAX_FILE_INFO_NO+1)
unsigned int ConvertData(unsigned char *input)
{
	unsigned int uiRet=0,i=0,j=0,k=0,uiTemp=1;
	
	j=3;
	for(i=0; i<4; i++)
	{
		for(k=0;k<j;k++)	uiTemp = uiTemp*16;
		if(input[i]=='A' || input[i]=='a')		uiRet = uiRet + (10*uiTemp);
		else if(input[i]=='B' || input[i]=='b')	uiRet = uiRet + (11*uiTemp);
		else if(input[i]=='C' || input[i]=='c')	uiRet = uiRet + (12*uiTemp);
		else if(input[i]=='D' || input[i]=='d')	uiRet = uiRet + (13*uiTemp);
		else if(input[i]=='E' || input[i]=='e')	uiRet = uiRet + (14*uiTemp);
		else if(input[i]=='F' || input[i]=='f')	uiRet = uiRet + (15*uiTemp);
#if true         
		else if(input[i]>='0' && input[i]<='9') uiRet = uiRet + ((input[i]-0x30)*uiTemp);
        else return 0; 
#else 
        else uiRet = uiRet + ((input[i]-0x30)*uiTemp);
#endif
        
		uiTemp=1;
		j--;
	}
	return uiRet;
}

#if defined (OLD_FOTA)
eFOTA_RESULT_CODE FOTA_ParseReceivedVersionInfo(void)
{
	uint16_t i;
  uint16_t argc;
  uint8_t *p2str;
  uint8_t *argv[ARGC_MAX_VERSION_INFO_ORDER];
  uint16_t nNoList;

	uint8_t aTmpOrder[4]={0,};
	uint8_t aTmpName[64]={0,};
	uint8_t aTmpSize[12]={0,};
	uint8_t aTmpVersion[12]={0,};
	uint8_t aTmpChecksum[12]={0,};
	uint8_t aTmpFolderName[32]={0,};

	uint16_t nRes;
	uint16_t n;
	stUpdateFileInfo *ptrFileInfo;

	p2str = ModemManagerData.paModemCommDataRxBuffer;
	ptrFileInfo = &g_stUpdateFileInfoList;
	memset((char *)ptrFileInfo, 0x00, sizeof(stUpdateFileInfo));

//	printf("data: <%s>\n", p2str);

	argv[0] = (uint8_t *)strtok((char *)p2str, "{");
//	printf("%u: \"%s\"\n", 0, argv[0]);

	for (argc = 1, i = 1; i < ARGC_MAX_VERSION_INFO_ORDER; i++) {
		argv[i] = (uint8_t *)strtok((char *)NULL, "{");
		if (argv[i] == NULL) {
			break;
		}
		else {
			argc++;

//			printf("%u: \"%s\"\n", i, argv[i]);
		}
	}

	// 0: " "USERID" : "AUTOLINK", "TIMESTAMP" : "20171219142852", "COUNT" : "5", "RESPONSELIST" : ["
	// 1: " "ORDER" : "0", "FILE_NAME" : "DCS_Bootloader.bin", "FILE_SIZE" : "14036", "FILE_VER" : "0003", "FILE_CHECKSUM" : "1405862", "FOLDER_NAME" : "D-E-H-01-0022" }, "
	// 2: " "ORDER" : "1", "FILE_NAME" : "DCS_Module.bin", "FILE_SIZE" : "111240", "FILE_VER" : "0018", "FILE_CHECKSUM" : "11716836", "FOLDER_NAME" : "D-E-H-01-0022" }, "
	// 3: " "ORDER" : "2", "FILE_NAME" : "MASTER.fin", "FILE_SIZE" : "752", "FILE_VER" : "0001", "FILE_CHECKSUM" : "98315" }, "
	// 4: " "ORDER" : "3", "FILE_NAME" : "DC2C0B.fin", "FILE_SIZE" : "4336", "FILE_VER" : "0001", "FILE_CHECKSUM" : "662328" }, "
	// 5: " "ORDER" : "4", "FILE_NAME" : "DCAG16.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"
	// 6: " "ORDER" : "5", "FILE_NAME" : "CanFD_Bootloader_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"
	// 7: " "ORDER" : "6", "FILE_NAME" : "CanFDAppication_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"
	// 8: " "ORDER" : "7", "FILE_NAME" : "CanFDArtery_Application_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"
	// 9: " "ORDER" : "8", "FILE_NAME" : "CanFDArtery_Application_Vxxxx.fin", "FILE_SIZE" : "2160", "FILE_VER" : "0001", "FILE_CHECKSUM" : "397701" }] }"	

	nNoList = 0;
	for(i = 1; i < argc; i++) {
		memset(aTmpOrder,0x00,sizeof(aTmpOrder));
		memset(aTmpName,0x00,sizeof(aTmpName));
		memset(aTmpSize,0x00,sizeof(aTmpSize));
		memset(aTmpVersion,0x00,sizeof(aTmpVersion));
		memset(aTmpChecksum,0x00,sizeof(aTmpChecksum));
		memset(aTmpFolderName,0x00,sizeof(aTmpFolderName));

		n = strlen((char *)argv[i]);
		nRes = ssscanf((char *)argv[i], n, " \"ORDER\" : \"%s\", \"FILE_NAME\" : \"%s\", \"FILE_SIZE\" : \"%s\", \"FILE_VER\" : \"%s\", \"FILE_CHECKSUM\" : \"%s\", \"FOLDER_NAME\" : \"%s\" }",
																						aTmpOrder, aTmpName, aTmpSize, aTmpVersion, aTmpChecksum, aTmpFolderName);

		if(nRes == 5 || nRes == 6) {
//			printf("order no: [%s]\n", aTmpOrder);
//			printf("file name: [%s]\n", aTmpName);
//			printf("file size: [%s]\n", aTmpSize);
//			printf("file version: [%s]\n", aTmpVersion);
//			printf("file checksum: [%s]\n\n", aTmpChecksum);
//			printf("folder name: [%s]\n\n", aTmpFolderName);

			if(aTmpOrder[0] != NULL && aTmpName[0] != NULL && aTmpSize[0] != NULL && aTmpChecksum[0] != NULL) {
				ptrFileInfo->m_stFileInfo[nNoList].m_cOrder = atoi((char *)aTmpOrder);														// order
//				strcpy_upper(ptrFileInfo->m_stFileInfo[nNoList].m_strFileName, aTmpName);												// name
				strcpy((char *)ptrFileInfo->m_stFileInfo[nNoList].m_strFileName, (char *)aTmpName);								// name
				ptrFileInfo->m_stFileInfo[nNoList].m_wFileSize = atol((char *)aTmpSize);													// file size
//				ptrFileInfo->m_stFileInfo[nNoList].m_nVersion = atoi((char *)aTmpVersion);												// file version
				ptrFileInfo->m_stFileInfo[nNoList].m_nVersion = ConvertData(aTmpVersion);												// file version
				ptrFileInfo->m_stFileInfo[nNoList].m_wFileCheckSum = atol((char *)aTmpChecksum);									// file checksum
				strcpy((char *)ptrFileInfo->m_stFileInfo[nNoList].m_strFolderName, (char *)aTmpFolderName);				// folder name

				nNoList++;
			}
		}
	}

//	printf("Rcv File count: %d\n", nNoList);
	ptrFileInfo->m_nFileNo = nNoList;

	if(nNoList == 0) {
		return eFOTA_RCV_DATA_ERROR_INFO_FILE_COUNT;
	}

#if				1
	//*****************************************************************
	// ?�버�?부???�신 받�? file ?�보�?출력??보자
	//*****************************************************************
	printf("Rcv File Info:\r\n");
	for(i = 0; i < nNoList; i++) {
		printf("rcv info no: %d\r\n", i);
		printf("order: %d\r\n", ptrFileInfo->m_stFileInfo[i].m_cOrder);
		printf("version: %d\r\n", ptrFileInfo->m_stFileInfo[i].m_nVersion);
		printf("file name: [%s]\r\n", ptrFileInfo->m_stFileInfo[i].m_strFileName);
		printf("size: %d\r\n", ptrFileInfo->m_stFileInfo[i].m_wFileSize);
		printf("checksum: 0x%x(%d)\r\n", ptrFileInfo->m_stFileInfo[i].m_wFileCheckSum, ptrFileInfo->m_stFileInfo[i].m_wFileCheckSum);
		printf("folder name: [%s]\r\n\r\n", ptrFileInfo->m_stFileInfo[i].m_strFolderName);
	}
	//*****************************************************************
	// ?�기까�?
	//*****************************************************************
#endif

	return eFOTA_RESULT_CODE_SUCCESS;
}

#else
eFOTA_RESULT_CODE NEWFOTA_ParseReceivedVersionInfo(void)
{
    uint16_t i;
    uint16_t argc;
    uint8_t *p2str;
    uint8_t *argv[ARGC_MAX_VERSION_INFO_ORDER];
    uint16_t nNoList;

	uint8_t aTmpOrder[4]={0,};
    uint8_t aTmpCategory[12]={0,};
	uint8_t aTmpName[64]={0,};
	uint8_t aTmpSize[12]={0,};
	uint8_t aTmpVersion[12]={0,};
	uint8_t aTmpChecksum[12]={0,};

	uint16_t nRes;
	uint16_t n;

	stUpdateFileInfo *ptrFileInfo;

	p2str = ModemManagerData.paModemCommDataRxBuffer;
	ptrFileInfo = &g_stUpdateFileInfoList;
	memset((char *)ptrFileInfo, 0x00, sizeof(stUpdateFileInfo));

//	printf("data: <%s>\n", p2str);

	argv[0] = (uint8_t *)strtok((char *)p2str, "{");
//	printf("%u: \"%s\"\n", 0, argv[0]);

	for (argc = 1, i = 1; i < ARGC_MAX_VERSION_INFO_ORDER; i++) {
		argv[i] = (uint8_t *)strtok((char *)NULL, "{");
		if (argv[i] == NULL) {
			break;
		}
		else {
			argc++;

//			printf("%u: \"%s\"\n", i, argv[i]);
		}
	}

	nNoList = 0;
	for(i = 1; i < argc; i++) {
		memset(aTmpOrder,0x00,sizeof(aTmpOrder));
        memset(aTmpCategory,0x00,sizeof(aTmpCategory));
		memset(aTmpName,0x00,sizeof(aTmpName));
		memset(aTmpSize,0x00,sizeof(aTmpSize));
		memset(aTmpVersion,0x00,sizeof(aTmpVersion));
		memset(aTmpChecksum,0x00,sizeof(aTmpChecksum));

		n = strlen((char *)argv[i]);
		nRes = ssscanf((char *)argv[i], n, "\"ORDER\":\"%s\",\"CATEGORY\":\"%s\",\"FILE_NAME\":\"%s\",\"FILE_SIZE\":\"%s\",\"FILE_VER\":\"%s\",\"FILE_CEHCKSUM\":\"%s\"}",
																						aTmpOrder, aTmpCategory, aTmpName, aTmpSize, aTmpVersion, aTmpChecksum);


		if(nRes == 5 || nRes == 6) {
//			printf("order no: [%s]\n", aTmpOrder);
//			printf("file name: [%s]\n", aTmpName);
//			printf("file size: [%s]\n", aTmpSize);
//			printf("file version: [%s]\n", aTmpVersion);
//			printf("file checksum: [%s]\n\n", aTmpChecksum);
//			printf("folder name: [%s]\n\n", aTmpFolderName);

			if(aTmpOrder[0] != NULL && aTmpName[0] != NULL && aTmpSize[0] != NULL && aTmpChecksum[0] != NULL) {
				ptrFileInfo->m_stFileInfo[nNoList].m_cOrder = atoi((char *)aTmpOrder);								// order
				strcpy((char *)ptrFileInfo->m_stFileInfo[nNoList].m_strCategory, (char *)aTmpCategory);						// category
				strcpy((char *)ptrFileInfo->m_stFileInfo[nNoList].m_strFileName, (char *)aTmpName);					// name
				ptrFileInfo->m_stFileInfo[nNoList].m_wFileSize = atol((char *)aTmpSize);								// file size
				ptrFileInfo->m_stFileInfo[nNoList].m_nVersion = ConvertData(aTmpVersion);								// file version
				ptrFileInfo->m_stFileInfo[nNoList].m_wFileCheckSum = atol((char *)aTmpChecksum);						// file checksum


				nNoList++;
			}
		}
	}

//	printf("Rcv File count: %d\n", nNoList);
	ptrFileInfo->m_nFileNo = nNoList;

	if(nNoList == 0) {
		return eFOTA_RCV_DATA_ERROR_INFO_FILE_COUNT;
	}

#if				1
	//*****************************************************************
	// ?�버�?부???�신 받�? file ?�보�?출력??보자
	//*****************************************************************
	printf("Rcv File Info:\r\n");
	for(i = 0; i < nNoList; i++) {
		printf("rcv info no: %d\r\n", i);
		printf("order: %d\r\n", ptrFileInfo->m_stFileInfo[i].m_cOrder);
        printf("category: [%s]\r\n\r\n", ptrFileInfo->m_stFileInfo[i].m_strCategory);
		printf("version: %04X\r\n", ptrFileInfo->m_stFileInfo[i].m_nVersion);
		printf("file name: [%s]\r\n", ptrFileInfo->m_stFileInfo[i].m_strFileName);
		printf("size: %d\r\n", ptrFileInfo->m_stFileInfo[i].m_wFileSize);
		printf("checksum: 0x%x(%d)\r\n", ptrFileInfo->m_stFileInfo[i].m_wFileCheckSum, ptrFileInfo->m_stFileInfo[i].m_wFileCheckSum);
	}
	//*****************************************************************
	// ?�기까�?
	//*****************************************************************
#endif

	return eFOTA_RESULT_CODE_SUCCESS;
}
#endif

#define ARGC_MAX_VEHICLE_INFO_ORDER				20

uint16_t SearchInfoString(char *pSrcString, char *pRetString, uint16_t nMaxLen)
{
	char *ptr1;
	char *ptr2;
	uint16_t n;

	ptr1 = strchr(pSrcString, ':');
	ptr1 = strchr(ptr1, '"');
	ptr1++;
	ptr2 = strchr(ptr1, '"');

	n = ptr2 - ptr1;
	if(n > nMaxLen - 1) {
		n = nMaxLen - 1;
	}

	memcpy(pRetString, ptr1, n);

//	printf("[%s], [%s]\n", pSrcString, pRetString);

	return n;
}

#if defined (OLD_FOTA)
eFOTA_RESULT_CODE FOTA_ParseReceivedVehicleInfo(void)
{
    uint16_t i;
    uint16_t argc;
    char *p2str;
//    char *ptrRestList;
    char *argv[ARGC_MAX_VEHICLE_INFO_ORDER];
	stVehicleInfo *ptrVehicleInfo;
	uint16_t nRet;
//	char TestBuffer[] = "{\"RESULT\":\"S\",\"MSG\":\"\",\"USERID\":\"AUTOLINK\",\"TIMESTAMP\":\"20180213012131\",\"VIN\":\"K12GL4EE0H01234567\", \"AREA\":\"HME\",\"MODEL_CODE\":\"DH22\", \"MODEL_DESC\":\"G80(DH)\",\"YEAR\":\"2017\",\"ENGINE_CODE\":\"148\",\"ENGINE_DESC\":\"G 3.8 GDI\",\"SCAN_DB\":\"DC5C0J\",\"FCS_DB\":\"FCDP38\",\"DB_VER\":\"0001\",\"VC_CODE\":\"0001\",\"VC_APP\":\"genesis\",\"RESPONSELIST\":[{\"syssubitemdesc\":\"aaa\",\"systemcode\":\"7\",\"systemtype\":\"bcm\",\"imagedesc\":\"BCM\"},{\"syssubitemdesc\":\"bbb\",\"systemcode\":\"8\",\"systemtype\":\"bcm8\",\"imagedesc\":\"BCM8\"},{\"syssubitemdesc\":\"ccc\",\"systemcode\":\"9\",\"systemtype\":\"bcm9\",\"imagedesc\":\"BCM9\"}]}";

	p2str = /*(char *)TestBuffer*/(char *)ModemManagerData.paModemCommDataRxBuffer;
//	ptrRestList = strstr((char *)p2str, "RESPONSELIST");

	ptrVehicleInfo = &g_stVehicleInfoList;
	memset((char *)ptrVehicleInfo, 0x00, sizeof(stVehicleInfo));

//{"RESULT":"S","MSG":"","USERID":"AUTOLINK","TIMESTAMP":"20180213012131","VIN":"K12GL4EE0H01234567", "AREA":"HME","MODEL_CODE":"DH22", "MODEL_DESC":"G80(DH)","YEAR":"2017","ENGINE_CODE":"148","ENGINE_DESC":"G 3.8 GDI","SCAN_DB":"DC5C0J","FCS_DB":"FCDP38","DB_VER":"0001","VC_CODE":"0001","VC_APP":"genesis","RESPONSELIST":[]}
	//printf("data: <%s>\n\n", p2str);

	p2str = strchr((const char *)p2str, ']');
	*(p2str + 1) = 0x00;

	p2str = /*(char *)TestBuffer*/(char *)ModemManagerData.paModemCommDataRxBuffer;
	p2str = strchr((const char *)p2str, '"');

	argv[0] = strtok((char *)p2str, ",");
	printf("%u: %s\r\n", 0, argv[0]);

	for (argc = 1, i = 1; i < ARGC_MAX_VEHICLE_INFO_ORDER; i++) {
	    argv[i] = strtok((char *)NULL, ",");
		if (argv[i] == NULL) {
		    break;
	    }
		else {
		argc++;

	    	printf("%u: %s\r\n", i, argv[i]);
	    }
	}

	// "MSG"
	nRet = SearchInfoString((char *)argv[1], (char *)ptrVehicleInfo->m_aMsg, MAX_LEN_VINFO_MSG);

	// "USERID"
	nRet = SearchInfoString((char *)argv[2], (char *)ptrVehicleInfo->m_aUserID, MAX_LEN_VINFO_USERID);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "TIMESTAMP"
	nRet = SearchInfoString((char *)argv[3], (char *)ptrVehicleInfo->m_aTimeStamp, MAX_LEN_VINFO_TIMESTAMP);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "VIN"
	nRet = SearchInfoString((char *)argv[4], (char *)ptrVehicleInfo->m_aVIN, MAX_LEN_VINFO_VIN);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "AREA"
	nRet = SearchInfoString((char *)argv[5], (char *)ptrVehicleInfo->m_aArea, MAX_LEN_VINFO_AREA);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "MODEL_CODE"
	nRet = SearchInfoString((char *)argv[6], (char *)ptrVehicleInfo->m_aModelCode, MAX_LEN_VINFO_MODEL_CODE);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "MODEL_DESC"
	nRet = SearchInfoString((char *)argv[7], (char *)ptrVehicleInfo->m_aModelDesc, MAX_LEN_VINFO_MODEL_DESC);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "YEAR"
	nRet = SearchInfoString((char *)argv[8], (char *)ptrVehicleInfo->m_aYear, MAX_LEN_VINFO_YEAR);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "ENGINE_CODE"
	nRet = SearchInfoString((char *)argv[9], (char *)ptrVehicleInfo->m_aEngineCode, MAX_LEN_VINFO_ENGINE_CODE);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "ENGINE_DESC"
	nRet = SearchInfoString((char *)argv[10], (char *)ptrVehicleInfo->m_aEngineDesc, MAX_LEN_VINFO_ENGINE_DESC);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "SCAN_DB"
	nRet = SearchInfoString((char *)argv[11], (char *)ptrVehicleInfo->m_aScanDB, MAX_LEN_VINFO_SCAN_DB);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "FCB_DB"
	nRet = SearchInfoString((char *)argv[12], (char *)ptrVehicleInfo->m_aFcsDB, MAX_LEN_VINFO_FCS_DB);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "DV_VER"
	nRet = SearchInfoString((char *)argv[13], (char *)ptrVehicleInfo->m_aDBVer, MAX_LEN_VINFO_DB_VER);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "VC_CODE"
	nRet = SearchInfoString((char *)argv[14], (char *)ptrVehicleInfo->m_aVCcode, MAX_LEN_VINFO_VC_CODE);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "VC_APP"
	nRet = SearchInfoString((char *)argv[15], (char *)ptrVehicleInfo->m_aVCApp, MAX_LEN_VINFO_VC_APP);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}
	
	// "IS_SMK"
	nRet = SearchInfoString((char *)argv[16], (char *)ptrVehicleInfo->m_aIsSMK, MAX_LEN_VINFO_IS_SMK);
	if(nRet == 0) {
        ptrVehicleInfo->m_aIsSMK[0] = 'Y';
        ptrVehicleInfo->m_aIsSMK[1] = 0;
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}
    else
    {
        if( ptrVehicleInfo->m_aIsSMK[0] != 'N' && ptrVehicleInfo->m_aIsSMK[0] != 'n' )
        {            
            ptrVehicleInfo->m_aIsSMK[0] = 'Y';
            ptrVehicleInfo->m_aIsSMK[1] = 0;
        }
    }
    
    

	return eFOTA_RESULT_CODE_SUCCESS;
}

#else
eFOTA_RESULT_CODE NEWFOTA_ParseReceivedVehicleInfo(void)
{
    uint16_t i;
    uint16_t argc;
    char *p2str;
//    char *ptrRestList;
    char *argv[ARGC_MAX_VEHICLE_INFO_ORDER];
	stVehicleInfo *ptrVehicleInfo;
	uint16_t nRet;
//	char TestBuffer[] = "{\"RESULT\":\"S\",\"MSG\":\"\",\"USERID\":\"AUTOLINK\",\"TIMESTAMP\":\"20180213012131\",\"VIN\":\"K12GL4EE0H01234567\", \"AREA\":\"HME\",\"MODEL_CODE\":\"DH22\", \"MODEL_DESC\":\"G80(DH)\",\"YEAR\":\"2017\",\"ENGINE_CODE\":\"148\",\"ENGINE_DESC\":\"G 3.8 GDI\",\"SCAN_DB\":\"DC5C0J\",\"FCS_DB\":\"FCDP38\",\"DB_VER\":\"0001\",\"VC_CODE\":\"0001\",\"VC_APP\":\"genesis\",\"RESPONSELIST\":[{\"syssubitemdesc\":\"aaa\",\"systemcode\":\"7\",\"systemtype\":\"bcm\",\"imagedesc\":\"BCM\"},{\"syssubitemdesc\":\"bbb\",\"systemcode\":\"8\",\"systemtype\":\"bcm8\",\"imagedesc\":\"BCM8\"},{\"syssubitemdesc\":\"ccc\",\"systemcode\":\"9\",\"systemtype\":\"bcm9\",\"imagedesc\":\"BCM9\"}]}";

	p2str = /*(char *)TestBuffer*/(char *)ModemManagerData.paModemCommDataRxBuffer;
//	ptrRestList = strstr((char *)p2str, "RESPONSELIST");

	ptrVehicleInfo = &g_stVehicleInfoList;
	memset((char *)ptrVehicleInfo, 0x00, sizeof(ptrVehicleInfo));

//{"RESULT":"S","MSG":"","USERID":"AUTOLINK","TIMESTAMP":"20180213012131","VIN":"K12GL4EE0H01234567", "AREA":"HME","MODEL_CODE":"DH22", "MODEL_DESC":"G80(DH)","YEAR":"2017","ENGINE_CODE":"148","ENGINE_DESC":"G 3.8 GDI","SCAN_DB":"DC5C0J","FCS_DB":"FCDP38","DB_VER":"0001","VC_CODE":"0001","VC_APP":"genesis","RESPONSELIST":[]}
	//printf("data: <%s>\n\n", p2str);

	p2str = strchr((const char *)p2str, '}');
	*(p2str + 1) = 0x00;

	p2str = /*(char *)TestBuffer*/(char *)ModemManagerData.paModemCommDataRxBuffer;
	p2str = strchr((const char *)p2str, '"');

	argv[0] = strtok((char *)p2str, ",");
	printf("%u: %s\r\n", 0, argv[0]);

	for (argc = 1, i = 1; i < ARGC_MAX_VEHICLE_INFO_ORDER; i++) {
	    argv[i] = strtok((char *)NULL, ",");
		if (argv[i] == NULL) {
		    break;
	    }
		else {
		argc++;

	    	printf("%u: %s\r\n", i, argv[i]);
	    }
	}

    // "VIN"
	nRet = SearchInfoString((char *)argv[0], (char *)ptrVehicleInfo->m_aVIN, MAX_LEN_VINFO_VIN);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

    // "DEVOPR"
    nRet = SearchInfoString((char *)argv[1], (char *)ptrVehicleInfo->m_aDEVOPR, MAX_LEN_VINFO_DEVOPR);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}
    
    // "MODULE"
	nRet = SearchInfoString((char *)argv[2], (char *)ptrVehicleInfo->m_aModule, MAX_LEN_VINFO_MODULE);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}
	// "AREA"
	nRet = SearchInfoString((char *)argv[3], (char *)ptrVehicleInfo->m_aArea, MAX_LEN_VINFO_AREA);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "COUNTRY_CODE"
	nRet = SearchInfoString((char *)argv[4], (char *)ptrVehicleInfo->m_aCountryCode, MAX_LEN_VINFO_COUNTRY_CODE);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

    // "VIN_MAKER"
	nRet = SearchInfoString((char *)argv[5], (char *)ptrVehicleInfo->m_aVINMaker, MAX_LEN_VINFO_VIN_MAKER);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}
	
    // "MODEL_CODE"
	nRet = SearchInfoString((char *)argv[6], (char *)ptrVehicleInfo->m_aModelCode, MAX_LEN_VINFO_MODEL_CODE);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}
	// "MODEL_DESC"
	nRet = SearchInfoString((char *)argv[7], (char *)ptrVehicleInfo->m_aModelDesc, MAX_LEN_VINFO_MODEL_DESC);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "MODEL_YEAR"
	nRet = SearchInfoString((char *)argv[8], (char *)ptrVehicleInfo->m_aModelYear, MAX_LEN_VINFO_MODEL_YEAR);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "ENGINE_CODE"
	nRet = SearchInfoString((char *)argv[9], (char *)ptrVehicleInfo->m_aEngineCode, MAX_LEN_VINFO_ENGINE_CODE);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "ENGINE_DESC"
	nRet = SearchInfoString((char *)argv[10], (char *)ptrVehicleInfo->m_aEngineDesc, MAX_LEN_VINFO_ENGINE_DESC);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

    // "DIAGNOSISDB_VER"
	nRet = SearchInfoString((char *)argv[11], (char *)ptrVehicleInfo->m_aDiagnosisDB_Ver, MAX_LEN_VINFO_DIAGNOSISDB_VER);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

    // "MASTERDB_VER"
	nRet = SearchInfoString((char *)argv[12], (char *)ptrVehicleInfo->m_aMasterDB_Ver, MAX_LEN_VINFO_MASTERDB_VER);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "SCAN_DB"
	nRet = SearchInfoString((char *)argv[13], (char *)ptrVehicleInfo->m_aScanDB, MAX_LEN_VINFO_SCAN_DB);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "FCB_DB"
	nRet = SearchInfoString((char *)argv[14], (char *)ptrVehicleInfo->m_aFcsDB, MAX_LEN_VINFO_FCS_DB);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "CT_DB"
	nRet = SearchInfoString((char *)argv[15], (char *)ptrVehicleInfo->m_aCTDB, MAX_LEN_VINFO_CT_DB);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "VC_CODE"
	nRet = SearchInfoString((char *)argv[16], (char *)ptrVehicleInfo->m_aVCcode, MAX_LEN_VINFO_VC_CODE);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}

	// "VC_APP"
	nRet = SearchInfoString((char *)argv[17], (char *)ptrVehicleInfo->m_aVCApp, MAX_LEN_VINFO_VC_APP);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}
	
	// "IS_SMK"
	nRet = SearchInfoString((char *)argv[18], (char *)ptrVehicleInfo->m_aIsSMK, MAX_LEN_VINFO_IS_SMK);
	if(nRet == 0) {
        ptrVehicleInfo->m_aIsSMK[0] = 'Y';
        ptrVehicleInfo->m_aIsSMK[1] = 0;
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}
    else
    {
        if( ptrVehicleInfo->m_aIsSMK[0] != 'N' && ptrVehicleInfo->m_aIsSMK[0] != 'n' )
        {            
            ptrVehicleInfo->m_aIsSMK[0] = 'Y';
            ptrVehicleInfo->m_aIsSMK[1] = 0;
        }
    }

    // "GEAR"
	nRet = SearchInfoString((char *)argv[19], (char *)ptrVehicleInfo->m_aGear, MAX_LEN_VINFO_GEAR);
	if(nRet == 0) {
		return eFOTA_RESULT_CODE_ZERO_LENGTH;
	}
    
	return eFOTA_RESULT_CODE_SUCCESS;
}
#endif

bool FOTA_CheckFileCheckSum(void)
{
	u32 wSum;
	u32 wOrgSize;
	u32 wOrgChecksum;
	u16 i;

	stUpdateFileInfo *ptrFileInfo;
	uint32_t wAddr;
	uint8_t aBuffer[MAX_BYTE_BUFFER_LEN];
	uint16_t nReadSize;
	//uint16_t nLoopCount = 0;

	ptrFileInfo = &g_stUpdateFileInfoList;

	wOrgSize = ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileSize;
	wOrgChecksum = ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileCheckSum;

	printf("FOTA: Original File Info\r\n");
	printf(" - name: [%s]\r\n", ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName);
	printf(" - size(server)           : [%d]\r\n", wOrgSize);
	printf(" - Total Received Bin Data: [%d]\r\n", gwTotalReceiveBinDataLength);
	printf(" - checksum(server)  : [0x%x]\r\n", wOrgChecksum);

	if( (gwTotalReceiveBinDataLength != wOrgSize) || (wOrgSize==0) ) {
		printf("FOTA: File Size mismatch!!!%d,%d\r\n",gwTotalReceiveBinDataLength,wOrgSize);
		return false;
	}

	switch(g_FotaManagerData.eCurrFotaFileType) {
		case eFOTA_FILE_TYPE_DIAGNOSIS_MASTER:
			wAddr = ADDR_MASTER_DB_SAVE;
			break;

		case eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE:
			wAddr = ADDR_SLAVE_DB_SAVE;
			break;

		case eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL:
			wAddr = ADDR_CONTROL_DB_SAVE;
			break;

		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT:
			wAddr = ADDR_BOOT_SAVE;
			break;

		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP:
			wAddr = ADDR_APPLICATION_SAVE1;
			break;
#if defined(EXTBOARD_FOTA)
		case eFOTA_FILE_TYPE_FDBOARD_BOOT:
			wAddr = ADDR_CONTROL_DB_SAVE;
			break;
		case eFOTA_FILE_TYPE_FDBOARD_APP:
			wAddr = ADDR_CONTROL_DB_SAVE;
			break;
#endif
	}

	wSum = 0;
	while(1) {
		if(wOrgSize > MAX_BYTE_BUFFER_LEN) {
			nReadSize = MAX_BYTE_BUFFER_LEN;
		}
		else {
			nReadSize = wOrgSize;
		}

        if (HalDrvFlashReadByteCallByRef((uint32_t*)&wAddr, (BYTE*)aBuffer, nReadSize)== HAL_RETURN_SUCCESS){
			for(i = 0; i < nReadSize; i++) {
				wSum += aBuffer[i];
			}

			wOrgSize -= nReadSize;
			if(wOrgSize == 0) {
				printf(" - checksum(terminal): [0x%x]\r\n", wSum);

				g_u16UpdateFileCheckSum = (uint16_t)wSum;
				if(wSum == wOrgChecksum) {
					return true;
				}
				else {
					printf("FOTA: Fail checksum\r\n");
					return false;
				}
			}
		}
		else {
			return false;
		}
	}
}

boolean_t DelInternalFlashofDB(int eCurrFotaFileType)
{
    unsigned int StartAddr,StopAddr;

    
    switch(eCurrFotaFileType) {
        case eFOTA_FILE_TYPE_DIAGNOSIS_MASTER:
            StartAddr = CAR_MASTER_DB_ADDRESS;
            StopAddr = CAR_MASTER_DB_ADDRESS+0x3FFF;
            break;

        case eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE:
            StartAddr = CAR_SLAVE_ADDRESS;
            StopAddr = CAR_SLAVE_ADDRESS+0x3FFF;
            break;

        case eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL:
            StartAddr = CAR_CTRL_DB_ADDRESS;
            StopAddr = CAR_CTRL_DB_ADDRESS+0x3FFF;
            break;
    }
    
    if (HalDrvFlashErase(StartAddr, StopAddr, NULL, 0, 0) == HAL_RETURN_SUCCESS ) {}
    else {printf("%s flash erase fail\r\n", __FUNCTION__);}
    
    return 0;
}


bool FOTA_CheckFileCheckSum2(int eCurrFotaFileType, unsigned int FileSize, uint16_t* usCheckSum);

boolean_t FindLastAddressofDB(int eCurrFotaFileType, char* dbName, int32_t nVer, uint16_t* pusCheckSum)
{
    unsigned char ucarrEmpty[32]={0};
    unsigned char ucarrTemp[32]={0};

    unsigned int srcAddr;

    boolean_t bFind = false;
    unsigned int unLastAddress=0;
    volatile int* unpLastAddress=0;

    switch(eCurrFotaFileType) {
        case eFOTA_FILE_TYPE_DIAGNOSIS_MASTER:
            srcAddr = CAR_MASTER_DB_ADDRESS;
            break;

        case eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE:
            srcAddr = CAR_SLAVE_ADDRESS;
            break;

        case eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL:
            srcAddr = CAR_CTRL_DB_ADDRESS;
            break;
			
		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT:
            srcAddr = BOOTLOADER_ADDRESS;
            break;
			
		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP:
            srcAddr = APPLICATION_ADDRESS;
            break;
    }

    memset(ucarrEmpty, 0xff, 32);
    for(int i=0;i<srcAddr+0x10000;i++)
    {
        unpLastAddress = (volatile int*)(srcAddr+i);
        memcpy(ucarrTemp,(void const*)unpLastAddress,32);
        if(memcmp((char *)ucarrTemp,(char *)ucarrEmpty,32)== 0)
        {
            // find last address
            bFind = true;
            unLastAddress = srcAddr+i;
            
            break;
        }
    }

    if( bFind == true )
    {
        printf("find last address : 0x%x\r\n",unLastAddress);
        printf("find file size : %x\r\n",unLastAddress-srcAddr);

        FOTA_CheckFileCheckSum2(eCurrFotaFileType,unLastAddress-srcAddr,pusCheckSum);

        printf("find check sum : %x\r\n",*pusCheckSum);      
        
        g_FirmwareInfo.AppProperty[eCurrFotaFileType].nAppFWVersion = nVer;
        memcpy((char *)g_FirmwareInfo.AppProperty[eCurrFotaFileType].arrFWName, (char *)dbName, 6);
        g_FirmwareInfo.AppProperty[eCurrFotaFileType].wFirmwareSize = unLastAddress-srcAddr;
        g_FirmwareInfo.AppProperty[eCurrFotaFileType].nCheckSum = *pusCheckSum;
        
        printf("Set F/W info!!!\r\n");
        SetFirmwareInfo(&g_FirmwareInfo);
    }  
    
    return 0;
}

bool FOTA_CheckFileCheckSum2(int eCurrFotaFileType, unsigned int FileSize, uint16_t* usCheckSum)
{
	u32 wSum;
	u32 wOrgSize;
	//u32 wOrgChecksum;
	u16 i;

	//stUpdateFileInfo *ptrFileInfo;
	uint32_t wAddr;
	uint8_t aBuffer[MAX_BYTE_BUFFER_LEN];
	uint16_t nReadSize;

	//ptrFileInfo = &g_stUpdateFileInfoList;

	//wOrgSize = ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileSize;
	//wOrgChecksum = ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileCheckSum;

	//printf("FOTA: Original File Info\n");
	//printf(" - name: [%s]\n", ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName);

	switch(eCurrFotaFileType) {
        case eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT:
			wAddr = BOOTLOADER_ADDRESS;
			break;
        case eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP:
			wAddr = APPLICATION_ADDRESS;
			break;
		case eFOTA_FILE_TYPE_DIAGNOSIS_MASTER:
			wAddr = CAR_MASTER_DB_ADDRESS;
			break;

		case eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE:
			wAddr = CAR_SLAVE_ADDRESS;
			break;

		case eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL:
			wAddr = CAR_CTRL_DB_ADDRESS;
			break;
	}

    if( FileSize < MAX_BYTE_BUFFER_LEN )
        nReadSize = FileSize;
    else
        nReadSize = MAX_BYTE_BUFFER_LEN;
    
    wOrgSize = FileSize;
    
	wSum = 0;
	while(1) {
        if (HalDrvFlashReadByteCallByRef((uint32_t*)&wAddr, (BYTE*)aBuffer, nReadSize)== HAL_RETURN_SUCCESS){
			for(i = 0; i < nReadSize; i++) {
				wSum += aBuffer[i];
			}

			wOrgSize -= nReadSize;

            if( nReadSize >= wOrgSize )
                nReadSize = wOrgSize;
                            
			if(wOrgSize == 0) {
				printf(" - checksum(terminal): [0x%x]\r\n", wSum);

				*usCheckSum = (uint16_t)wSum;
                return true;
			}
		}
		else {
            *usCheckSum = 0;
			return false;
		}
	}
}


bool GetVehicleAreaInfo(uint8_t *pVehicleAreaInfo)
{
	uint16_t nMaxTableSize;
	uint16_t i;

	nMaxTableSize = sizeof(stTableCountryMakerAreaInfo) / sizeof(stCountryMakerAreaInfo);
//	printf("nMaxTableSize: %d\r\n", nMaxTableSize);

	for(i = 0; i < nMaxTableSize; i++) {
		if(memcmp((char *)stTableCountryMakerAreaInfo[i].arrArea, (char *)&g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], 2) == 0) {
			if(memcmp((char *)stTableCountryMakerAreaInfo[i].arrMaker, (char *)&g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO + 2], 1) == 0) {
				strcpy((char *)pVehicleAreaInfo, (char *)stTableCountryMakerAreaInfo[i].arrCountryCode);
				break;
			}
		}
	}

	if(i == nMaxTableSize) {
		printf("No Area Info!!!\r\n");

		return false;
	}

	return true;
}

#if defined (OLD_FOTA)
bool FOTA_MakeRequestVersionContent(void)
{
    stHalRTCTypeDef stHalRtcDateTime;
	uint16_t nLen;
	uint8_t *ptrHTTPBuffer;
	uint8_t arrHTTPTempBuffer[MAX_MODEM_COMM_BUFFER_LENGTH];
	uint8_t arrTemp[SIZE_SERIAL_NUMBER + 2];
	uint8_t arrTempAreaInfo[MAX_COUNTRY_CODE_SIZE];
	bool ret;

//	printf("FOTA: @FOTA_MakeRequestVersionContent()\r\n");

	ptrHTTPBuffer = arrHTTPTempBuffer;

	memset((char *)ptrHTTPBuffer, 0x00, MAX_MODEM_COMM_BUFFER_LENGTH);

#if defined(PROTOCOL17)
	stServerUrl strTempUrl;
	memset(&strTempUrl,0x00,sizeof(strTempUrl));
	ReadFOTA_URL(&strTempUrl,eHTTP_FOTA_REQ_TYPE_VERSION);
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, strTempUrl.Url);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);
   	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, strTempUrl.Path);
#else
	// url
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, HTTP_FOTA_URL);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);  
	if( BkSram_SystemInfo.bTestFota == false )
	{
	    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, HTTP_FOTA_PATH_REQUEST_VERSION);
	}
    else
    {
        strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, HTTP_TEST_FOTA_PATH_REQUEST_VERSION);
    }
#endif
	// user id, timestamp
    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRtcDateTime), 0);

	sprintf((char *)ptrHTTPBuffer, "{\"USERID\":\"%s\",\"TIMESTAMP\":\"%04d%02d%02d%0.2d%0.2d%0.2d\",\x00",
																					"AUTOLINK",
																					2000 + stHalRtcDateTime.RtcDate.RTC_Year,
																					stHalRtcDateTime.RtcDate.RTC_Month,
																					stHalRtcDateTime.RtcDate.RTC_Date,
																					stHalRtcDateTime.RtcTime.RTC_Hours,
																					stHalRtcDateTime.RtcTime.RTC_Minutes,
																					stHalRtcDateTime.RtcTime.RTC_Seconds);

	nLen = strlen((char *)ptrHTTPBuffer);

	// model code
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"MODEL_CODE\":\"%s\",\x00", g_stVehicleInfoList.m_aModelCode);
	nLen = strlen((char *)ptrHTTPBuffer);

	// year
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"YEAR\":\"%s\",\x00", g_stVehicleInfoList.m_aYear);
	nLen = strlen((char *)ptrHTTPBuffer);

	// area
	ret = GetVehicleAreaInfo(arrTempAreaInfo);
	if(ret == false) {
		return false;
	}

	sprintf((char *)&ptrHTTPBuffer[nLen], "\"AREA\":\"%s\",\x00", arrTempAreaInfo);
	nLen = strlen((char *)ptrHTTPBuffer);

	// Engine Code
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"ENGINE_CODE\":\"%s\",\x00", g_stVehicleInfoList.m_aEngineCode);
	nLen = strlen((char *)ptrHTTPBuffer);

	// country
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"CNTY_CODE\":\"%c%c\",\"COUNTRY_CODE\":\"%c%c\",\x00",
	                    g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO+1],
	                    g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO+1]);
	nLen = strlen((char *)ptrHTTPBuffer);

	// tel
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"TEL\":\"%s\",\x00", &ModemManagerData.aPhoneNo[3]);
	nLen = strlen((char *)ptrHTTPBuffer);
	
	// SMK
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"IS_SMK\":\"%s\",\x00", g_stVehicleInfoList.m_aIsSMK);
	nLen = strlen((char *)ptrHTTPBuffer);

	// vin
  char carrSystemVin[64]={0};

  GetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrSystemVin);
  sprintf((char *)&ptrHTTPBuffer[nLen], "\"VIN\":\"%s\",\x00", carrSystemVin);
	nLen = strlen((char *)ptrHTTPBuffer);

	// serial
	memset((char *)arrTemp, 0x00, SIZE_SERIAL_NUMBER + 2);
	memcpy((char *)arrTemp, (char *)g_FirmwareInfo.arrSerialNumber, SIZE_SERIAL_NUMBER);
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"SERIAL\":\"%s\",\x00", arrTemp);
	nLen = strlen((char *)ptrHTTPBuffer);

	// request list
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"REQUESTLIST\":[\x00");
	nLen = strlen((char *)ptrHTTPBuffer);

	// category, file name, file version - master db
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"CATEGORY\":\"diagnosis\",\"FILE_NAME\":\"MASTERDB\",\"FILE_VER\":\"%d\"},\x00", g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);

	// category, file name, file version - slave db
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"CNTY_CODE\":\"%c%c\",\"CATEGORY\":\"diagnosis\",\"FILE_NAME\":\"SLAVEDB\",\"FILE_VER\":\"%d\"},\x00", 
	                                        g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO+1],
	                                        g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);

	// category, file name, file version - control db
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"CNTY_CODE\":\"%c%c\",\"CATEGORY\":\"diagnosis\",\"FILE_NAME\":\"CTRLDB\",\"FILE_VER\":\"%d\"},\x00", 
	                                        g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO+1],
	                                        g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);

	// category, file name, file version - bootloader
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"CATEGORY\":\"firmware\",\"FILE_NAME\":\"Bootloader\",\"FILE_VER\":\"%d\"},\x00", g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);

	// category, file name, file version - application
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"CATEGORY\":\"firmware\",\"FILE_NAME\":\"Module\",\"FILE_VER\":\"%d\"},\x00", g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);

	sprintf((char *)&ptrHTTPBuffer[nLen], "]}\x00");
	nLen = strlen((char *)ptrHTTPBuffer);

	printf("FOTA: len: %d\r\n", nLen);
	printf("FOTA: <%s>\r\n", ptrHTTPBuffer);

	ApplyEncryption(arrHTTPTempBuffer, gaModemCommTxDataBuffer, &nLen, nLen);

	printf("FOTA: encrypted len: %d\r\n", nLen);
	//hexdump(gaModemCommTxDataBuffer, nLen);
    hexdump(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath,sizeof(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath));

	ModemManagerData.paModemCommDataTxBuffer = gaModemCommTxDataBuffer;
	ModemManagerData.nWriteDataLength = nLen;

	ModemManagerData.bModemDataSaveToFlashFlag = false;							// false?�면, modem?�서 ?�신 받�? ?�이?��? internal flash???�?�하지 ?�는??
	geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;				// ?�버�?부???�신 받는 ?�이???�?��? ascii 문자?�이??

	ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_NONE;				// event message ?�송???�니므�?
    	for(int n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
		BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
	}

	return true;
}

#else
bool NEWFOTA_MakeRequestVersionContent(void)
{
	uint16_t nLen = 0;
	uint8_t *ptrHTTPBuffer;
	uint8_t arrHTTPTempBuffer[MAX_MODEM_COMM_BUFFER_LENGTH];
	uint8_t arrTempAreaInfo[MAX_COUNTRY_CODE_SIZE];
    char arrTempVInfo[64] = {0,};
    char arrTempEncryptedVInfo[64] = {0,};
    uint16_t cTempEncryptedVInfoLen = 0;
    char* pCFDBootloaderstr, *pCFDAppstr;
    
	bool ret;

    char carrSystemVin[64]={0};

	printf("FOTA: @NEWFOTA_MakeRequestVersionContent()\r\n");

	ptrHTTPBuffer = arrHTTPTempBuffer;

	memset((char *)ptrHTTPBuffer, 0x00, MAX_MODEM_COMM_BUFFER_LENGTH);
	
	stServerUrl strTempUrl;
	memset(&strTempUrl,0x00,sizeof(strTempUrl));
	ReadFOTA_URL(&strTempUrl,eHTTP_FOTA_REQ_TYPE_VERSION);
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, strTempUrl.Url);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, NEWFOTA_HTTP_MESSAGE_HEADER_CONTENT);
   	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, strTempUrl.Path);

    if( BkSram_SystemInfo.bTestFota == false )
    {
        strncpy(g_stVehicleInfoList.m_aDEVOPR,DEVOPR_OPR,MAX_DEVOPR_LEN);
    }
    else
    {
        strncpy(g_stVehicleInfoList.m_aDEVOPR,DEVOPR_DEV,MAX_DEVOPR_LEN);
    }
    
    // DEVOPR
    cTempEncryptedVInfoLen = strlen(g_stVehicleInfoList.m_aDEVOPR);
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aDEVOPR, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"DEVOPR\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // MODULE
    cTempEncryptedVInfoLen = strlen(g_stVehicleInfoList.m_aModule);
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aModule, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"MODULE\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // AREA
    ret = GetVehicleAreaInfo(arrTempAreaInfo);
	if(ret == false) {
		return false;
	}
    cTempEncryptedVInfoLen = strlen((char *)arrTempAreaInfo);
    ApplyEncryption((uint8_t *)arrTempAreaInfo, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"AREA\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // COUNTRY_CODE
    cTempEncryptedVInfoLen = strlen(g_stVehicleInfoList.m_aCountryCode);
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aCountryCode, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"COUNTRY_CODE\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // VIN
    GetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrSystemVin);
    cTempEncryptedVInfoLen = strlen(carrSystemVin);
    ApplyEncryption((uint8_t *)carrSystemVin, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"VIN\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

	// MODEL_CODE
    cTempEncryptedVInfoLen = strlen(g_stVehicleInfoList.m_aModelCode);
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aModelCode, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"MODEL_CODE\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

	// ENGINE_CODE
    cTempEncryptedVInfoLen = strlen(g_stVehicleInfoList.m_aEngineCode);
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aEngineCode, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"ENGINE_CODE\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

	// MODEL_YEAR
	cTempEncryptedVInfoLen = strlen(g_stVehicleInfoList.m_aModelYear);
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aModelYear, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"MODEL_YEAR\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // IS_SMK 
    cTempEncryptedVInfoLen = strlen(g_stVehicleInfoList.m_aIsSMK);
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aIsSMK, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"IS_SMK\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

	// TEL
    memcpy(arrTempVInfo, (char *)&ModemManagerData.aPhoneNo[3], (sizeof(ModemManagerData.aPhoneNo)-3));
    cTempEncryptedVInfoLen = strlen(arrTempVInfo);
    ApplyEncryption((uint8_t *)arrTempVInfo, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"TEL\":\"%s\",\x00", arrTempEncryptedVInfo);
	nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));
    memset((char *)arrTempVInfo, 0x00, sizeof(arrTempVInfo));

	// SERIAL
	memcpy((char *)arrTempVInfo, (char *)g_FirmwareInfo.arrSerialNumber, SIZE_SERIAL_NUMBER);
    ApplyEncryption((uint8_t *)arrTempVInfo, (uint8_t *)arrTempEncryptedVInfo, &cTempEncryptedVInfoLen, 0);
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"SERIAL\":\"%s\",\x00", arrTempEncryptedVInfo);
	nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));
    memset((char *)arrTempVInfo, 0x00, sizeof(arrTempVInfo));

	// REQUEST LIST
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"REQUESTLIST\":[\x00");
	nLen = strlen((char *)ptrHTTPBuffer);

    // category, file name, file version - bootloader
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"ORDER\":\"0\",\"CATEGORY\":\"Bootloader\",\"FILE_VER\":\"%04X\"},\x00", g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);

	// category, file name, file version - application
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"ORDER\":\"1\",\"CATEGORY\":\"Application\",\"FILE_VER\":\"%04X\"},\x00", g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);

	// category, file name, file version - master db
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"ORDER\":\"2\",\"CATEGORY\":\"Master\",\"FILE_VER\":\"%04X\"},\x00", g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);

	// category, file name, file version - slave db
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"ORDER\":\"3\",\"CATEGORY\":\"SlaveDB\",\"FILE_VER\":\"%04X\"},\x00", g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);


#if defined(EXTBOARD_FOTA)
	// category, file name, file version - control db
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"ORDER\":\"4\",\"CATEGORY\":\"ControlDB\",\"FILE_VER\":\"%04X\"},\x00", g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);


    if ( (m_stCFDCtrl.stVer.usBlVer < CFD_ARTERY_MCU_VERSION) && (m_stCFDCtrl.stVer.usAppVer < CFD_ARTERY_MCU_VERSION) )
    {
        pCFDBootloaderstr = "{\"ORDER\":\"5\",\"CATEGORY\":\"CanFDBoot\",\"FILE_VER\":\"%04X\"},\x00";
        pCFDAppstr        = "{\"ORDER\":\"6\",\"CATEGORY\":\"CanFDApp\",\"FILE_VER\":\"%04X\"}\x00";
    }
    else
    {
        pCFDBootloaderstr = "{\"ORDER\":\"7\",\"CATEGORY\":\"CanFDArteryBoot\",\"FILE_VER\":\"%04X\"},\x00";
        pCFDAppstr        = "{\"ORDER\":\"8\",\"CATEGORY\":\"CanFDArteryApp\",\"FILE_VER\":\"%04X\"}\x00";
    }
    
	// category, file name, file version - ext boot
    sprintf((char *)&ptrHTTPBuffer[nLen], pCFDBootloaderstr, m_stCFDCtrl.stVer.usBlVer);
	nLen = strlen((char *)ptrHTTPBuffer);
    
	// category, file name, file version - ext app
    sprintf((char *)&ptrHTTPBuffer[nLen], pCFDAppstr, m_stCFDCtrl.stVer.usAppVer);
	nLen = strlen((char *)ptrHTTPBuffer);
#else
	// category, file name, file version - control db
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"ORDER\":\"4\",\"CATEGORY\":\"ControlDB\",\"FILE_VER\":\"%04X\"}\x00", g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion);
	nLen = strlen((char *)ptrHTTPBuffer);
#endif


	sprintf((char *)&ptrHTTPBuffer[nLen], "]}\x00");
	nLen = strlen((char *)ptrHTTPBuffer);

	printf("FOTA: len: %d\r\n", nLen);
	printf("FOTA: <%s>\r\n", ptrHTTPBuffer);
	
	strcpy((char *)gaModemCommTxDataBuffer, (char *)arrHTTPTempBuffer);
	//hexdump(gaModemCommTxDataBuffer, nLen);
    hexdump(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath,sizeof(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath));

	ModemManagerData.paModemCommDataTxBuffer = gaModemCommTxDataBuffer;
	ModemManagerData.nWriteDataLength = nLen;

	ModemManagerData.bModemDataSaveToFlashFlag = false;							// false?�면, modem?�서 ?�신 받�? ?�이?��? internal flash???�?�하지 ?�는??
	geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;				// ?�버�?부???�신 받는 ?�이???�?��? ascii 문자?�이??

	ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_NONE;				// event message ?�송???�니므�?
    	for(int n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
		BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
	}

	return true;
}
#endif

#if defined (OLD_FOTA)
bool FOTA_MakeRequestBinDataContent(void)
{
    stHalRTCTypeDef stHalRtcDateTime;
    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRtcDateTime), 0);
	uint16_t nLen;
	uint8_t *ptrHTTPBuffer;
	uint8_t arrHTTPTempBuffer[MAX_MODEM_COMM_BUFFER_LENGTH];
	stUpdateFileInfo *ptrFileInfo;
	char carrSystemVin[64]={0};

  	GetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrSystemVin);
	
	printf("FOTA: @FOTA_MakeRequestVersionContent()\r\n");

	ptrFileInfo = &g_stUpdateFileInfoList;
	ptrHTTPBuffer = arrHTTPTempBuffer;

	memset((char *)ptrHTTPBuffer, 0x00, MAX_MODEM_COMM_BUFFER_LENGTH);

#if defined(PROTOCOL17)
	stServerUrl strTempUrl;
	memset(&strTempUrl,0x00,sizeof(strTempUrl));
	ReadFOTA_URL(&strTempUrl,eHTTP_FOTA_REQ_TYPE_FILE_DOWNLOAD);
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, strTempUrl.Url);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);
   	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, strTempUrl.Path);	
#else
	// url
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, HTTP_FOTA_URL);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);
    if( BkSram_SystemInfo.bTestFota == false )
	{
	    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, HTTP_FOTA_PATH_REQUEST_FILE_DOWNLOAD);
    }
    else
    {
        strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, HTTP_TEST_FOTA_PATH_REQUEST_FILE_DOWNLOAD);
    }
#endif
	// user id, timestamp
    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRtcDateTime), 0);

	sprintf((char *)ptrHTTPBuffer, "{\"HTTP\":\"1.1\",\"TIMESTAMP\":\"%04d%02d%02d%0.2d%0.2d%0.2d\",\x00",
																					2000 + stHalRtcDateTime.RtcDate.RTC_Year,
																					stHalRtcDateTime.RtcDate.RTC_Month,
																					stHalRtcDateTime.RtcDate.RTC_Date,
																					stHalRtcDateTime.RtcTime.RTC_Hours,
																					stHalRtcDateTime.RtcTime.RTC_Minutes,
																					stHalRtcDateTime.RtcTime.RTC_Seconds);

	nLen = strlen((char *)ptrHTTPBuffer);

	switch(g_FotaManagerData.eCurrFotaFileType) {
		case eFOTA_FILE_TYPE_DIAGNOSIS_MASTER:
		case eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE:
		case eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL:
			sprintf((char *)&ptrHTTPBuffer[nLen], "\"CNTY_CODE\":\"%c%c\",\"CATEGORY\":\"diagnosis\",\"VERSION\":\"%s\",\"MODEL_CODE\":\"%s\",\"VIN\":\"%s\"}\x00",
                                                g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO+1],
                                                ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName, g_stVehicleInfoList.m_aModelCode,carrSystemVin);
			break;

		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT:
		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP:
			sprintf((char *)&ptrHTTPBuffer[nLen], "\"CNTY_CODE\":\"%c%c\",\"CATEGORY\":\"firmware\",\"VERSION\":\"%s\",\"FOLDER_NAME\":\"%s\",\"MODEL_CODE\":\"%s\",\"VIN\":\"%s\"}\x00", 
                                                g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO+1],
                                                ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName, ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFolderName, g_stVehicleInfoList.m_aModelCode,carrSystemVin);
			break;
	}

	nLen = strlen((char *)ptrHTTPBuffer);

	printf("FOTA: len: %d\r\n", nLen);
	printf("FOTA: <%s>\r\n", ptrHTTPBuffer);

	ApplyEncryption(arrHTTPTempBuffer, gaModemCommTxDataBuffer, &nLen, 0);

	//hexdump(gaModemCommTxDataBuffer, nLen);
    hexdump(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath,sizeof(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath));

	ModemManagerData.paModemCommDataTxBuffer = gaModemCommTxDataBuffer;
	ModemManagerData.nWriteDataLength = nLen;

	ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;

	ModemManagerData.bModemDataSaveToFlashFlag = true;
	geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_BIN;

	ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_NONE;				// event message |￠�???????
		for(int n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
		BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
	}

	return true;
}

#else
bool NEWFOTA_MakeRequestBinDataContent(void)
{
	uint16_t nLen = 0;
	uint8_t *ptrHTTPBuffer;
    uint8_t arrTempAreaInfo[MAX_COUNTRY_CODE_SIZE];
	uint8_t arrHTTPTempBuffer[MAX_MODEM_COMM_BUFFER_LENGTH];
	stUpdateFileInfo *ptrFileInfo;
    uint16_t cTempVInfoLen= 0;
    char arrVInfo[MAX_VIN_SIZE] = {0,};
    uint8_t arrTempEncryptedVInfo[64] = {0,};
	char carrSystemVin[64]={0,};
	
	bool ret;

  	GetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrSystemVin);
	
	printf("FOTA: @NEWFOTA_MakeRequestBinDataContent()\r\n");

	ptrFileInfo = &g_stUpdateFileInfoList;
	ptrHTTPBuffer = arrHTTPTempBuffer;

	memset((char *)ptrHTTPBuffer, 0x00, MAX_MODEM_COMM_BUFFER_LENGTH);
	
	stServerUrl strTempUrl;
	memset(&strTempUrl,0x00,sizeof(strTempUrl));
	ReadFOTA_URL(&strTempUrl,eHTTP_FOTA_REQ_TYPE_FILE_DOWNLOAD);
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, strTempUrl.Url);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, NEWFOTA_HTTP_MESSAGE_HEADER_CONTENT);
   	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, strTempUrl.Path);
	
	if( BkSram_SystemInfo.bTestFota == false )
    {
        strncpy(g_stVehicleInfoList.m_aDEVOPR,DEVOPR_OPR,MAX_DEVOPR_LEN);
    }
    else
    {
        strncpy(g_stVehicleInfoList.m_aDEVOPR,DEVOPR_DEV,MAX_DEVOPR_LEN);
    }

    // DEVOPR
    cTempVInfoLen = strlen(g_stVehicleInfoList.m_aDEVOPR);
#if defined(FOTA_LOG)
    hexdump(g_stVehicleInfoList.m_aDEVOPR, sizeof(g_stVehicleInfoList.m_aDEVOPR));
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aDEVOPR, arrTempEncryptedVInfo, &cTempVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "{\"DEVOPR\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // MODULE
    cTempVInfoLen = strlen(g_stVehicleInfoList.m_aModule);
#if defined(FOTA_LOG)
    hexdump(g_stVehicleInfoList.m_aModule, sizeof(g_stVehicleInfoList.m_aModule));
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aModule, arrTempEncryptedVInfo, &cTempVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"MODULE\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // AREA
    ret = GetVehicleAreaInfo(arrTempAreaInfo);
	if(ret == false) {
		return false;
	}
    cTempVInfoLen = strlen((char *)arrTempAreaInfo);
#if defined(FOTA_LOG)
    hexdump(arrTempAreaInfo, sizeof(arrTempAreaInfo));
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)arrTempAreaInfo, arrTempEncryptedVInfo, &cTempVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"AREA\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // COUNTRY_CODE
    cTempVInfoLen = strlen(g_stVehicleInfoList.m_aCountryCode);
#if defined(FOTA_LOG)
    hexdump(g_stVehicleInfoList.m_aCountryCode, sizeof(g_stVehicleInfoList.m_aCountryCode));
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aCountryCode, arrTempEncryptedVInfo, &cTempVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"COUNTRY_CODE\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // CATEGORY
    cTempVInfoLen = strlen((char*)ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strCategory);
#if defined(FOTA_LOG)
    hexdump(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strCategory, sizeof(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strCategory));
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strCategory, arrTempEncryptedVInfo, &cTempVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"CATEGORY\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // VIN
    cTempVInfoLen = strlen(carrSystemVin);
#if defined(FOTA_LOG)
    hexdump(carrSystemVin, sizeof(carrSystemVin));
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)carrSystemVin, arrTempEncryptedVInfo, &cTempVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"VIN\":\"%s\",\x00", arrTempEncryptedVInfo);
	nLen = strlen((char *)ptrHTTPBuffer);
    memset((char *)arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // NAME
    cTempVInfoLen = strlen((char *)ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName);
#if defined(FOTA_LOG)
    hexdump(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName, sizeof(ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName));
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName, arrTempEncryptedVInfo, &cTempVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"NAME\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    // VERSION
	sprintf(arrVInfo,"%04X", ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion);
#if defined(FOTA_LOG)
    hexdump(arrVInfo, sizeof(arrVInfo));
	printf("\r\n");
#endif
    cTempVInfoLen = strlen(arrVInfo);
    ApplyEncryption((uint8_t *)arrVInfo, arrTempEncryptedVInfo, &cTempVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"VERSION\":\"%s\",\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));
	
	// MODEL_CODE
    cTempVInfoLen = strlen(g_stVehicleInfoList.m_aModelCode);
#if defined(FOTA_LOG)
    hexdump(g_stVehicleInfoList.m_aModelCode, sizeof(g_stVehicleInfoList.m_aModelCode));
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)g_stVehicleInfoList.m_aModelCode, arrTempEncryptedVInfo, &cTempVInfoLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"MODEL_CODE\":\"%s\"}\x00", arrTempEncryptedVInfo);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset(arrTempEncryptedVInfo, 0x00, sizeof(arrTempEncryptedVInfo));

    

	nLen = strlen((char *)ptrHTTPBuffer);

	printf("FOTA: len: %d\r\n", nLen);
	printf("FOTA: <%s>\r\n", ptrHTTPBuffer);
	
	strcpy((char *)gaModemCommTxDataBuffer, (char *)arrHTTPTempBuffer);
	//ApplyEncryption(arrHTTPTempBuffer, gaModemCommTxDataBuffer, &nLen, 0);

	//hexdump(gaModemCommTxDataBuffer, nLen);
    hexdump(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath,sizeof(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath));
	printf("\r\n");

	ModemManagerData.paModemCommDataTxBuffer = gaModemCommTxDataBuffer;
	ModemManagerData.nWriteDataLength = nLen;

	ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;

	ModemManagerData.bModemDataSaveToFlashFlag = true;
	geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_BIN;

	ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_NONE;				// event message |￠�???????
		for(int n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
		BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
	}

	return true;
}
#endif

void ReadFOTA_URL(stServerUrl *prtUrl,eHTTP_FOTA_REQ_TYPE eType)
{
	char * carrSeriaNumber = GetFWSerialNumber();
	DCSServiceType nServiceType;
	
    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);
	
	//R0005063KRH1842
	if( carrSeriaNumber[MANUFACTURER_POS] == 'K' )	//���
	{
		memcpy(prtUrl->Url,	HTTP_FOTA_URL_KIA, 	sizeof(HTTP_FOTA_URL_KIA));
#if defined (OLD_FOTA)
		if( strncmp(&carrSeriaNumber[COUNTRY_POS],"KR",2)==0 )	//����
		{
			if( BkSram_SystemInfo.bTestFota == false )	memcpy(prtUrl->Path,	HTTP_FOTA_PATH_SELF_DEV,	sizeof(HTTP_FOTA_PATH_SELF_DEV));
			else										memcpy(prtUrl->Path,	HTTP_FOTA_PATH_SELF_DEV,	sizeof(HTTP_FOTA_PATH_SELF_DEV));
		}
		else	//AU,NZ
		{
			if( BkSram_SystemInfo.bTestFota == false )	memcpy(prtUrl->Path,	HTTP_FOTA_PATH_OPERATION,	sizeof(HTTP_FOTA_PATH_OPERATION));
			else										memcpy(prtUrl->Path,	HTTP_FOTA_PATH_DEV,			sizeof(HTTP_FOTA_PATH_DEV));
		}
#endif
	}
	else	//����
	{
		memcpy(prtUrl->Url,	HTTP_FOTA_URL, 	sizeof(HTTP_FOTA_URL));
#if defined (OLD_FOTA)
		if( strncmp(&carrSeriaNumber[COUNTRY_POS],"KR",2)==0 )	//����
		{
			if( BkSram_SystemInfo.bTestFota == false )	memcpy(prtUrl->Path,	HTTP_FOTA_PATH_SELF_DEV, 		sizeof(HTTP_FOTA_PATH_DEV));
			else										memcpy(prtUrl->Path,	HTTP_FOTA_PATH_SELF_DEV, 		sizeof(HTTP_FOTA_PATH_DEV));
		}
		else if( strncmp(&carrSeriaNumber[COUNTRY_POS],"NZ",2)==0 )
		{
//			if( BkSram_SystemInfo.bTestFota == false )	memcpy(prtUrl->Path,	HTTP_FOTA_PATH_SELF_DEV,	sizeof(HTTP_FOTA_PATH_SELF_DEV));
//			else										memcpy(prtUrl->Path,	HTTP_FOTA_PATH_SELF_DEV, 	sizeof(HTTP_FOTA_PATH_SELF_DEV));
			if( BkSram_SystemInfo.bTestFota == false )	memcpy(prtUrl->Path,	HTTP_FOTA_PATH_OPERATION,	sizeof(HTTP_FOTA_PATH_OPERATION));
			else										memcpy(prtUrl->Path,	HTTP_FOTA_PATH_DEV, 		sizeof(HTTP_FOTA_PATH_DEV));
		}
		else	//AU
		{
			if( BkSram_SystemInfo.bTestFota == false )	memcpy(prtUrl->Path,	HTTP_FOTA_PATH_OPERATION,	sizeof(HTTP_FOTA_PATH_OPERATION));
			else										memcpy(prtUrl->Path,	HTTP_FOTA_PATH_DEV, 		sizeof(HTTP_FOTA_PATH_DEV));
		}
#endif
	}

	
	if( eType == eHTTP_FOTA_REQ_TYPE_VERSION )				strcat(prtUrl->Path,HTTP_FOTA_VERSION);
	else if( eType == eHTTP_FOTA_REQ_TYPE_FILE_DOWNLOAD )	strcat(prtUrl->Path,HTTP_FOTA_FILE_DOWNLOAD);
	else if( eType == eHTTP_FOTA_REQ_TYPE_VEHICLE_INFO )	strcat(prtUrl->Path,HTTP_FOTA_VEHICLE_INFO);		
}

// http://gds.hyundai-motor.com/autolink_premium/autolink-premium/websvc_mws.asmx
// g_FirmwareInfo.m_strVIN

#if defined (OLD_FOTA)
bool FOTA_MakeRequestVehicleInfo(void)
{
    stHalRTCTypeDef stHalRtcDateTime;
	uint16_t nLen;
	uint8_t *ptrHTTPBuffer;
	uint8_t arrHTTPTempBuffer[MAX_MODEM_COMM_BUFFER_LENGTH];
	uint8_t arrTempAreaInfo[MAX_COUNTRY_CODE_SIZE];
	bool ret;

	printf("FOTA: @FOTA_MakeRequestVehicleInfo()\r\n");

	ptrHTTPBuffer = arrHTTPTempBuffer;

	memset((char *)ptrHTTPBuffer, 0x00, MAX_MODEM_COMM_BUFFER_LENGTH);

#if defined(PROTOCOL17)
	stServerUrl strTempUrl;
	memset(&strTempUrl,0x00,sizeof(strTempUrl));
	ReadFOTA_URL(&strTempUrl,eHTTP_FOTA_REQ_TYPE_VEHICLE_INFO);
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, strTempUrl.Url);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);
   	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, strTempUrl.Path);
#else
	// url
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, HTTP_FOTA_URL);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);

	if( BkSram_SystemInfo.bTestFota == false )
	{    	
    	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, HTTP_FOTA_PATH_REQUEST_VEHICLE_INFO);
	}
    else
    {
    	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, HTTP_TEST_FOTA_PATH_REQUEST_VEHICLE_INFO);
    }
#endif

	// VIN
  char carrSystemVin[64]={0};

  GetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrSystemVin);
  sprintf((char *)ptrHTTPBuffer, "{\"VIN\":\"%s\",\x00", carrSystemVin);
	nLen = strlen((char *)ptrHTTPBuffer);

	// area
	ret = GetVehicleAreaInfo(arrTempAreaInfo);
	if(ret == false) {
		return false;
	}

	sprintf((char *)&ptrHTTPBuffer[nLen], "\"AREA\":\"%s\",\x00", arrTempAreaInfo);
	nLen = strlen((char *)ptrHTTPBuffer);

	// country
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"CNTY_CODE\":\"%c%c\",\"COUNTRY_CODE\":\"%c%c\",\x00",
	                    g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO+1],
	                    g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO+1]);

	nLen = strlen((char *)ptrHTTPBuffer);

    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRtcDateTime), 0);

	sprintf((char *)&ptrHTTPBuffer[nLen], "\"TIMESTAMP\":\"%04d%02d%02d%0.2d%0.2d%0.2d\",\"USERID\":\"%s\",\"HTTP\":\"1.0\"}\x00",
																					2000 + stHalRtcDateTime.RtcDate.RTC_Year,
																					stHalRtcDateTime.RtcDate.RTC_Month,
																					stHalRtcDateTime.RtcDate.RTC_Date,
																					stHalRtcDateTime.RtcTime.RTC_Hours,
																					stHalRtcDateTime.RtcTime.RTC_Minutes,
																					stHalRtcDateTime.RtcTime.RTC_Seconds,
																					"AUTOLINK");
	nLen = strlen((char *)ptrHTTPBuffer);

	printf("FOTA: len: %d\r\n", nLen);
	printf("FOTA: <%s>\r\n", ptrHTTPBuffer);

#if				0
	ApplyEncryption(arrHTTPTempBuffer, gaModemCommTxDataBuffer, &nLen,0);
#else
	strcpy((char *)gaModemCommTxDataBuffer, (char *)arrHTTPTempBuffer);
#endif

	//hexdump(gaModemCommTxDataBuffer, nLen);
    hexdump(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath,sizeof(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath));

	ModemManagerData.paModemCommDataTxBuffer = gaModemCommTxDataBuffer;
	ModemManagerData.nWriteDataLength = nLen;

	ModemManagerData.bModemDataSaveToFlashFlag = false;							// false?�면, modem?�서 ?�신 받�? ?�이?��? internal flash???�?�하지 ?�는??
	geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;				// ?�버�?부???�신 받는 ?�이???�?��? ascii 문자?�이??

	ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_NONE;				// event message |￠�???????
	for(int n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
		BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
	}

	return true;
}

#else
bool NEWFOTA_MakeRequestVehicleInfo(void)
{
	uint16_t nLen = 0;
	uint8_t *ptrHTTPBuffer;
	uint8_t arrHTTPTempBuffer[MAX_MODEM_COMM_BUFFER_LENGTH];
	uint8_t arrTempAreaInfo[MAX_COUNTRY_CODE_SIZE];
    char aVehicleInfoData[64]={0,};
    char aEncryptedVehicleInforData[64]={0,};
    uint16_t cVehicleInfoDataLen = 0;
	bool ret;

	printf("FOTA: @NEWFOTA_MakeRequestVehicleInfo()\r\n");

	ptrHTTPBuffer = arrHTTPTempBuffer;

	memset((char *)ptrHTTPBuffer, 0x00, MAX_MODEM_COMM_BUFFER_LENGTH);
	
	stServerUrl strTempUrl;
	memset(&strTempUrl,0x00,sizeof(strTempUrl));
	ReadFOTA_URL(&strTempUrl,eHTTP_FOTA_REQ_TYPE_VEHICLE_INFO);
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, strTempUrl.Url);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, NEWFOTA_HTTP_MESSAGE_HEADER_CONTENT);
   	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath, strTempUrl.Path);
	
	if( BkSram_SystemInfo.bTestFota == false )
    {
        strncpy(g_stVehicleInfoList.m_aDEVOPR,DEVOPR_OPR,MAX_DEVOPR_LEN);
    }
    else
    {
        strncpy(g_stVehicleInfoList.m_aDEVOPR,DEVOPR_DEV,MAX_DEVOPR_LEN);
    }

// DEVOPR
    memcpy((void*)aVehicleInfoData, g_stVehicleInfoList.m_aDEVOPR, MAX_DEVOPR_LEN);
    cVehicleInfoDataLen = strlen(aVehicleInfoData);
#if defined(FOTA_LOG)
	hexdump(g_stVehicleInfoList.m_aDEVOPR, cVehicleInfoDataLen);
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)aVehicleInfoData, (uint8_t *)aEncryptedVehicleInforData, &cVehicleInfoDataLen, 0);
	sprintf((char *)&ptrHTTPBuffer[nLen], "{\"DEVOPR\":\"%s\",\x00", aEncryptedVehicleInforData);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset((char *)aVehicleInfoData, 0x00, sizeof(aVehicleInfoData));
    memset((char *)aEncryptedVehicleInforData, 0x00, sizeof(aEncryptedVehicleInforData));

// MODULE
    memcpy((void*)aVehicleInfoData, MODULE_INFO, MAX_MODULE_INFO_LEN); 
    cVehicleInfoDataLen = strlen(aVehicleInfoData);
#if defined(FOTA_LOG)
	hexdump(aVehicleInfoData, cVehicleInfoDataLen);
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)aVehicleInfoData, (uint8_t *)aEncryptedVehicleInforData, &cVehicleInfoDataLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"MODULE\":\"%s\",\x00", aEncryptedVehicleInforData);
    nLen = strlen((char *)ptrHTTPBuffer);
    memset((char *)aVehicleInfoData, 0x00, sizeof(aVehicleInfoData));
    memset((char *)aEncryptedVehicleInforData, 0x00, sizeof(aEncryptedVehicleInforData));

// AREA
	ret = GetVehicleAreaInfo(arrTempAreaInfo);
	if(ret == false) {
		return false;
	}
    cVehicleInfoDataLen = strlen((char *)arrTempAreaInfo);
#if defined(FOTA_LOG)
	hexdump(arrTempAreaInfo, cVehicleInfoDataLen);
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)arrTempAreaInfo, (uint8_t *)aEncryptedVehicleInforData, &cVehicleInfoDataLen, 0);
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"AREA\":\"%s\",\x00", aEncryptedVehicleInforData);
	nLen = strlen((char *)ptrHTTPBuffer);
    memset((char *)aVehicleInfoData, 0x00, sizeof(aVehicleInfoData));
    memset((char *)aEncryptedVehicleInforData, 0x00, sizeof(aEncryptedVehicleInforData));
    

// COUNTRY
    memcpy((void*)aVehicleInfoData, &g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], MAX_COUNTRY_CODE_LEN);
    cVehicleInfoDataLen = strlen(aVehicleInfoData);
#if defined(FOTA_LOG)
	hexdump(aVehicleInfoData, cVehicleInfoDataLen);
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)aVehicleInfoData, (uint8_t *)aEncryptedVehicleInforData, &cVehicleInfoDataLen, 0);
	sprintf((char *)&ptrHTTPBuffer[nLen], "\"COUNTRY_CODE\":\"%s\",\x00", aEncryptedVehicleInforData);
	nLen = strlen((char *)ptrHTTPBuffer);
    memset((char *)aVehicleInfoData, 0x00, sizeof(aVehicleInfoData));
    memset((char *)aEncryptedVehicleInforData, 0x00, sizeof(aEncryptedVehicleInforData));
	
// VIN
    GetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)aVehicleInfoData);
    cVehicleInfoDataLen = strlen(aVehicleInfoData);
#if defined(FOTA_LOG)
	hexdump(aVehicleInfoData, cVehicleInfoDataLen);
	printf("\r\n");
#endif
    ApplyEncryption((uint8_t *)aVehicleInfoData, (uint8_t *)aEncryptedVehicleInforData, &cVehicleInfoDataLen, 0);
    sprintf((char *)&ptrHTTPBuffer[nLen], "\"VIN\":\"%s\"}\x00", aEncryptedVehicleInforData);
	nLen = strlen((char *)ptrHTTPBuffer);
    memset((char *)aVehicleInfoData, 0x00, sizeof(aVehicleInfoData));
    memset((char *)aEncryptedVehicleInforData, 0x00, sizeof(aEncryptedVehicleInforData));
	
	printf("FOTA: len: %d\r\n", nLen);
	printf("FOTA: <%s>\r\n", ptrHTTPBuffer);

#if	0
	ApplyEncryption(arrHTTPTempBuffer, gaModemCommTxDataBuffer, &nLen,0);
#else
	strcpy((char *)gaModemCommTxDataBuffer, (char *)arrHTTPTempBuffer);
#endif

	//hexdump(gaModemCommTxDataBuffer, nLen);
    hexdump(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath,sizeof(ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath));
	printf("\r\n");

	ModemManagerData.paModemCommDataTxBuffer = gaModemCommTxDataBuffer;
	ModemManagerData.nWriteDataLength = nLen;

	ModemManagerData.bModemDataSaveToFlashFlag = false;							// false?�면, modem?�서 ?�신 받�? ?�이?��? internal flash???�?�하지 ?�는??
	geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;				// ?�버�?부???�신 받는 ?�이???�?��? ascii 문자?�이??

	ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_NONE;				// event message |￠�???????
	for(int n = 0; n < MAX_SERVICE_PROFILE_NO; n++) {
		BkSram_ModemInfo.bHTTPSuccessServiceConnection[n] = false;
	}

	return true;
}
#endif

bool FOTA_DownloadClose(void)
{
	uint32_t nDestAddress;
	uint32_t nSrcAddress;
	uint32_t nEraseEndAddress;
	uint32_t nReadSize;
	uint32_t nTotalReadSize;
	uint32_t nLoopCount = 0;
	uint16_t nCheckSum = 0;
	stCANFDBoardUpdateInfo stFDBoardInfo;
	int i=0;
	BYTE arrTempBuff[SIZE_TEMP_BUFF];

//	printf("@DownloadClose()\r\n");
	switch(g_FotaManagerData.eCurrFotaFileType) {
		case eFOTA_FILE_TYPE_DIAGNOSIS_MASTER:
			printf("\r\n <master db>\r\n");
			nDestAddress = CAR_MASTER_DB_ADDRESS;
			nSrcAddress = ADDR_MASTER_DB_SAVE;
			break;

		case eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE:
			printf("\r\n <slave db>\r\n");
			nDestAddress = CAR_SLAVE_ADDRESS;
			nSrcAddress = ADDR_SLAVE_DB_SAVE;
			break;

		case eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL:
			printf("\r\n <control db>\r\n");
			nDestAddress = CAR_CTRL_DB_ADDRESS;
			nSrcAddress = ADDR_CONTROL_DB_SAVE;
			break;

		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT:
			printf("\r\n <boot>\r\n");
			nDestAddress = BOOTLOADER_ADDRESS;
			nSrcAddress = ADDR_BOOT_SAVE;
			break;

		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP:
			printf("\r\n <app>\r\n");
			nDestAddress = APPLICATION_ADDRESS;
			nSrcAddress = ADDR_APPLICATION_SAVE1;
			g_FirmwareInfo.nApplicationUpdateSignal = FIRMWARE_APP_SIGNAL;
			break;
#if defined(EXTBOARD_FOTA)
		case eFOTA_FILE_TYPE_FDBOARD_BOOT:
			printf("\r\n <Ext Boot>\r\n");
			nDestAddress = SECTOR_EXBOARD_BOOT_FIRMWARE;
			nSrcAddress = ADDR_CONTROL_DB_SAVE;
			nLoopCount = SECTOR_EXBOARD_BOOT_FIRMWARE_SECTOR_SIZE;
			break;
		case eFOTA_FILE_TYPE_FDBOARD_APP:
			printf("\r\n <Ext App>\r\n");
			nDestAddress = SECTOR_EXBOARD_APP_FIRMWARE;
			nSrcAddress = ADDR_CONTROL_DB_SAVE;
			nLoopCount = SECTOR_EXBOARD_APP_FIRMWARE_SECTOR_SIZE;
			break;
#endif
	}
#if defined(EXTBOARD_FOTA)
	nTotalReadSize = g_u32UpdateFileSize /* included CheckSum Length*/;
	nReadSize = SIZE_TEMP_BUFF;
	nEraseEndAddress = nDestAddress + nTotalReadSize;

//	printf("checksum %d(0x%x), g_u32UpdateFileSize %d\r\n", g_u16UpdateFileCheckSum, g_u16UpdateFileCheckSum, g_u32UpdateFileSize);
	printf("nSrcAddress 0x%X, nDestAddress 0x%X\r\n", nSrcAddress, nDestAddress);

	if(g_FotaManagerData.eCurrFotaFileType != eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP ) {

        __disable_irq();

		if( g_FotaManagerData.eCurrFotaFileType == eFOTA_FILE_TYPE_FDBOARD_BOOT || g_FotaManagerData.eCurrFotaFileType == eFOTA_FILE_TYPE_FDBOARD_APP )
		{
			for( i=0; i<nLoopCount; i++ )	sFLASH_EraseSubSector((nDestAddress + i) * SFLASH_SECTOR_SIZE);

			if(nTotalReadSize < SIZE_TEMP_BUFF) {
				nReadSize = nTotalReadSize;
			}
			i=0;
			while ( nTotalReadSize > 0 ) {
                if (HalDrvFlashReadByteCallByRef((uint32_t*)&nSrcAddress, (uint8_t*)arrTempBuff, nReadSize)== HAL_RETURN_SUCCESS){
					//hexdump(arrTempBuff,nReadSize);
					sFLASH_WriteBuffer((uint8_t*)arrTempBuff,(nDestAddress*SFLASH_SECTOR_SIZE)+(i*SIZE_TEMP_BUFF), nReadSize);
					i++;
					printf("o");

					nTotalReadSize -= (nReadSize);
					if(nTotalReadSize < SIZE_TEMP_BUFF) {
						nReadSize = nTotalReadSize;
					}
				}
			}
			__enable_irq();
			nTotalReadSize = g_u32UpdateFileSize-sizeof(g_u16UpdateFileCheckSum);
			i=0;
			nReadSize = SIZE_TEMP_BUFF;
			if(nTotalReadSize < SIZE_TEMP_BUFF)	nReadSize = nTotalReadSize;
			while ( nTotalReadSize > 0 ) {
				memset(arrTempBuff,0x00,sizeof(arrTempBuff));
				sFLASH_ReadBuffer((uint8_t*)arrTempBuff, (nDestAddress*SFLASH_SECTOR_SIZE)+(i*SIZE_TEMP_BUFF), nReadSize );
				//hexdump(arrTempBuff,nReadSize);
				for( int j=0; j<nReadSize; j++)	nCheckSum+=arrTempBuff[j];
				nTotalReadSize -= (nReadSize);
				if(nTotalReadSize < SIZE_TEMP_BUFF) {
					nReadSize = nTotalReadSize;
				}
				//printf("i:%d,nTotalReadSize:%d,nReadSize:%d,nCheckSum:%d,g_u16UpdateFileCheckSum:%d\r\n",i,nTotalReadSize,nReadSize,nCheckSum,g_u16UpdateFileCheckSum);
				i++;
			}
			if( nCheckSum != g_u16UpdateFileCheckSum )
			{
			  printf("@@@@@@@@@@@@@@@@CheckSum Fail@@@@@@@@@@@@@@@@@@@@@@\r\n");
			  __enable_irq();
			  return false;
			}
			
			if( g_u32UpdateFileSize != 0 )	//if size 0 is download fail
			{
				GetAutolinkConfigProperty(eAutoLinkConfig_FDBoardUpdateInfo,(void*)&stFDBoardInfo);
				if( g_FotaManagerData.eCurrFotaFileType == eFOTA_FILE_TYPE_FDBOARD_BOOT )
				{
					stFDBoardInfo.usBootVersion = g_DownloadInfo.DB_Ver;
					stFDBoardInfo.uiBootSize = g_u32UpdateFileSize-sizeof(g_u16UpdateFileCheckSum);	//g_DownloadInfo.DB_Size; is same
					stFDBoardInfo.usBootCheckSum = g_u16UpdateFileCheckSum;	//g_DownloadInfo.nCheckSum; is same
				}
				else
				{
					stFDBoardInfo.usAppVersion = g_DownloadInfo.DB_Ver;
					stFDBoardInfo.uiAppSize = g_u32UpdateFileSize-sizeof(g_u16UpdateFileCheckSum);	//g_DownloadInfo.DB_Size; is same
					stFDBoardInfo.usAppCheckSum = g_u16UpdateFileCheckSum;	//g_DownloadInfo.nCheckSum; is same
				}
				SetAutolinkConfigProperty(eAutoLinkConfig_FDBoardUpdateInfo,(void*)&stFDBoardInfo);
			}
		}
		else
		{
		
            if (HalDrvFlashErase(nDestAddress, nEraseEndAddress, NULL, 0, 0) == HAL_RETURN_SUCCESS ) {
	//			printf("%s flash erase success\r\n", __FUNCTION__);
			}
			else {
				printf("%s flash erase fail\r\n", __FUNCTION__);
			}

			if(nTotalReadSize < SIZE_TEMP_BUFF) {
				nReadSize = nTotalReadSize;
			}
			
			while ( nTotalReadSize > 0 ) {
                if (HalDrvFlashReadByteCallByRef((uint32_t*)&nSrcAddress, (uint8_t*)arrTempBuff, nReadSize)== HAL_RETURN_SUCCESS){
					if(HalDrvFlashWriteByteCallByRef(&nDestAddress, (uint8_t*)arrTempBuff, nReadSize) == HAL_RETURN_SUCCESS ) {
						printf("o");
					}

					nTotalReadSize -= (nReadSize);
					if(nTotalReadSize < SIZE_TEMP_BUFF) {
						nReadSize = nTotalReadSize;
					}
				}
			}
		}

        __enable_irq();
        
		printf("\r\n\r\n");
	}

	g_FirmwareInfo.AppProperty[g_FotaManagerData.eCurrFotaFileType].nAppFWVersion = g_DownloadInfo.DB_Ver;
	memcpy((char *)g_FirmwareInfo.AppProperty[g_FotaManagerData.eCurrFotaFileType].arrFWName, (char *)g_DownloadInfo.DB_Name, 6);
	g_FirmwareInfo.AppProperty[g_FotaManagerData.eCurrFotaFileType].wFirmwareSize = g_u32UpdateFileSize;
	g_FirmwareInfo.AppProperty[g_FotaManagerData.eCurrFotaFileType].nCheckSum = g_u16UpdateFileCheckSum;

	printf("Set F/W info!!!\r\n");
	SetFirmwareInfo(&g_FirmwareInfo);

#else
	nTotalReadSize = g_u32UpdateFileSize /* included CheckSum Length*/;
	nReadSize = SIZE_TEMP_BUFF;
	nEraseEndAddress = nDestAddress + nTotalReadSize;

//	printf("checksum %d(0x%x), g_u32UpdateFileSize %d\r\n", g_u16UpdateFileCheckSum, g_u16UpdateFileCheckSum, g_u32UpdateFileSize);
	printf("nSrcAddress 0x%X, nDestAddress 0x%X\r\n", nSrcAddress, nDestAddress);

	if(g_FotaManagerData.eCurrFotaFileType != eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP ) {

        __disable_irq();
        
        if (HalDrvFlashErase(nDestAddress, nEraseEndAddress, NULL, 0, 0) == HAL_RETURN_SUCCESS ) {
//			printf("%s flash erase success\r\n", __FUNCTION__);
		}
		else {
			printf("%s flash erase fail\r\n", __FUNCTION__);
		}

//		printf("%s: flash write start\r\n", __FUNCTION__);

		if(nTotalReadSize < SIZE_TEMP_BUFF) {
			nReadSize = nTotalReadSize;
		}

		while ( nTotalReadSize > 0 ) {
            if (HalDrvFlashReadByteCallByRef((uint32_t*)&nSrcAddress, (uint8_t*)arrTempBuff, nReadSize)== HAL_RETURN_SUCCESS){
				if(HalDrvFlashWriteByteCallByRef(&nDestAddress, (uint8_t*)arrTempBuff, nReadSize) == HAL_RETURN_SUCCESS ) {
					printf("o");
				}

				nTotalReadSize -= (nReadSize);
				if(nTotalReadSize < SIZE_TEMP_BUFF) {
					nReadSize = nTotalReadSize;
				}
			}
		}

        __enable_irq();
        
		printf("\r\n\r\n");
	}

	g_FirmwareInfo.AppProperty[g_FotaManagerData.eCurrFotaFileType].nAppFWVersion = g_DownloadInfo.DB_Ver;
	memcpy((char *)g_FirmwareInfo.AppProperty[g_FotaManagerData.eCurrFotaFileType].arrFWName, (char *)g_DownloadInfo.DB_Name, 6);
	g_FirmwareInfo.AppProperty[g_FotaManagerData.eCurrFotaFileType].wFirmwareSize = g_u32UpdateFileSize;
	g_FirmwareInfo.AppProperty[g_FotaManagerData.eCurrFotaFileType].nCheckSum = g_u16UpdateFileCheckSum;

	printf("Set F/W info!!!\r\n");
	SetFirmwareInfo(&g_FirmwareInfo);
#endif
	return true;
}

bool FOTA_UpdateFileToInternalFlash(eFOTA_FILE_TYEP eFileType)
{
	stAMT_PTCL_PAYLOAD  stModemInPtcl;
	stDownloadStartReq *pDownloadStartReq;
	stUpdateFileInfo *ptrFileInfo;

	ptrFileInfo = &g_stUpdateFileInfoList;

	printf("@Md_UpdateFileToInternalFlash()\r\n");

	stModemInPtcl.DataLength = 0;
	pDownloadStartReq = (stDownloadStartReq *)stModemInPtcl.pPayload;

	switch(eFileType) {
		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT:
			pDownloadStartReq->DB_Name[0] = 'B';
			pDownloadStartReq->DB_Name[1] = 'O';
			pDownloadStartReq->DB_Name[2] = 'O';
			pDownloadStartReq->DB_Name[3] = 'T';
			pDownloadStartReq->DB_Name[4] = 'F';
			pDownloadStartReq->DB_Name[5] = 'W';
			break;
		case eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP:
			pDownloadStartReq->DB_Name[0] = 'A';
			pDownloadStartReq->DB_Name[1] = 'P';
			pDownloadStartReq->DB_Name[2] = 'P';
			pDownloadStartReq->DB_Name[3] = 'L';
			pDownloadStartReq->DB_Name[4] = 'F';
			pDownloadStartReq->DB_Name[5] = 'W';
			break;
		case eFOTA_FILE_TYPE_DIAGNOSIS_MASTER:
			pDownloadStartReq->DB_Name[0] = 'M';
			pDownloadStartReq->DB_Name[1] = 'A';
			pDownloadStartReq->DB_Name[2] = 'S';
			pDownloadStartReq->DB_Name[3] = 'T';
			pDownloadStartReq->DB_Name[4] = 'E';
			pDownloadStartReq->DB_Name[5] = 'R';
			break;
		case eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE:
		case eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL:
#if defined(EXTBOARD_FOTA)
		case eFOTA_FILE_TYPE_FDBOARD_BOOT:
		case eFOTA_FILE_TYPE_FDBOARD_APP:
#endif
			memcpy((char *)pDownloadStartReq->DB_Name, (char *)ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_strFileName, 6);
			break;
	}

	pDownloadStartReq->DB_Size = ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileSize;
	g_u32UpdateFileSize = pDownloadStartReq->DB_Size + sizeof(g_u16UpdateFileCheckSum);				// checksum ¡?�?¡???￠�??
	pDownloadStartReq->DB_Ver = ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_nVersion;
	pDownloadStartReq->nCheckSum = ptrFileInfo->m_stFileInfo[g_FotaManagerData.nCurrDownloadFileNo].m_wFileCheckSum;

	printf("file name: [%c%c%c%c%c%c]\r\n", pDownloadStartReq->DB_Name[0], pDownloadStartReq->DB_Name[1], pDownloadStartReq->DB_Name[2], pDownloadStartReq->DB_Name[3], pDownloadStartReq->DB_Name[4], pDownloadStartReq->DB_Name[5]);
	printf("file size: [%d]\r\n", pDownloadStartReq->DB_Size);
	printf("file version: [%d]\r\n", pDownloadStartReq->DB_Ver);
	printf("file checksum: [0x%x]\r\n", pDownloadStartReq->nCheckSum);

	memcpy(&g_DownloadInfo, pDownloadStartReq, sizeof(g_DownloadInfo));

	SetLedOnOffCtl(LED_OFF, eLED_GPS);			//GREEN
	SetLedOnOffCtl(LED_ON, eLED_SERVER);	//RED
	SetLedOnOffCtl(LED_OFF, eLED_CAN);			//BLUE

	FOTA_DownloadClose();

	return true;
}
