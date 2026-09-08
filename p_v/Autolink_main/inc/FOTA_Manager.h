/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FOTA_MANAGER_H__
#define __FOTA_MANAGER_H__

#include "GIT_OemInterface.h"
#include "Modem_Manager.h"
#include "common.h"



#define ADDR_BOOT_SAVE				(uint32_t)0x08100000		//
#define ADDR_FW_INFO_SAVE			(uint32_t)0x08104000		//
#define ADDR_MASTER_DB_SAVE			(uint32_t)0x08108000		//
#define ADDR_SLAVE_DB_SAVE			(uint32_t)0x0810C000		//
#define ADDR_CONTROL_DB_SAVE		(uint32_t)0x081C0000		//

#define ADDR_APPLICATION_SAVE1		(uint32_t)0x08110000		//
#define ADDR_APPLICATION_SAVE2		(uint32_t)0x08120000		//
#define ADDR_APPLICATION_SAVE3		(uint32_t)0x08140000		//
#define ADDR_APPLICATION_SAVE4		(uint32_t)0x08160000		//
#define ADDR_APPLICATION_SAVE5		(uint32_t)0x08180000		//
#define ADDR_APPLICATION_SAVE6		(uint32_t)0x081A0000		//


#define ADDR_END_SAVE				(uint32_t)(0x081E0000 - 1)		//

#define	REQUEST_MODE_FILE_INFO					0
#define REQUEST_MODE_FILE_DATA					1


#if defined(EXTBOARD_FOTA)
#define MAX_FILE_INFO_NO								(7)
#else
#define MAX_FILE_INFO_NO								(5)
#endif

#define MAX_JSON_REQUEST_LIST_INFO_NO		(10)
#define MAX_VEHICLE_CODE_LENGTH					(10 + 2)

//#define MAX_HTTP_DATA_BODY_LENGTH					(1500)


//************************************************************************************************
//#define	HTTP_FOTA_URL															"fileup.gitauto.com"
//#if				0
//	// 운영 서버
//	#define	HTTP_FOTA_PATH_REQUEST_VERSION							"/carsharing/api/update/updateif.aspx"
//	#define	HTTP_FOTA_PATH_REQUEST_FILE_DOWNLOAD				"/carsharing/api/update/filedownload.aspx"
//
////	#define	HTTP_FOTA_PATH_REQUEST_VERSION							"/carsharing/api/update/updateifrow.aspx"				// header, body start/end 가 필요없이, jsondata만 전송하면 되는 url
////	#define	HTTP_FOTA_PATH_REQUEST_FILE_DOWNLOAD				"/carsharing/api/update/filedownloadrow.aspx"
//#else
//	// 테스트 서버
//	#define	HTTP_FOTA_PATH_REQUEST_VERSION							"/carsharingdev/api/update/updateif.aspx"
//	#define	HTTP_FOTA_PATH_REQUEST_FILE_DOWNLOAD				"/carsharingdev/api/update/filedownload.aspx"
//#endif

#if	defined(PROTOCOL17)

	#define	HTTP_FOTA_URL							"gds.hyundai-motor.com"
	#define	HTTP_FOTA_URL_KIA						"gds.kia.co.kr"
	//운영
	#define	HTTP_FOTA_PATH_OPERATION		"autolink_premium/Autolink-Premium"
	//개발
	#define	HTTP_FOTA_PATH_DEV					"autolink_dev_premium/Autolink-Premium"
	//내부 개발
	#define	HTTP_FOTA_PATH_SELF_DEV		"AUTOLINK_Dev_KR/Autolink-Premium"


	#define MANUFACTURER_POS	10
	#define COUNTRY_POS	8

#if defined (OLD_FOTA)
	#define HTTP_FOTA_VERSION					"/FW_UPDATEIF.aspx"
	#define HTTP_FOTA_FILE_DOWNLOAD		"/FW_FileDownload.aspx"
	#define HTTP_FOTA_VEHICLE_INFO			"/FW_websvc_mws.aspx"
#else
	#define 	HTTP_FOTA_VERSION							"AUTOLINK_FOTA/api/Vehicle/FwUpdateInfo"
	#define	HTTP_FOTA_FILE_DOWNLOAD				"AUTOLINK_FOTA/api/Vehicle/Download"
	#define	HTTP_FOTA_VEHICLE_INFO					"AUTOLINK_FOTA/api/Vehicle/Info"

	#define DEVOPR_DEV "DEV"
	#define DEVOPR_OPR "OPR"
	#define MAX_DEVOPR_LEN      3
#ifdef RF_COMMON_MODEM
#ifdef AT32F435VMT7
	#define MODULE_INFO "ARTERY63"
	#define MAX_MODULE_INFO_LEN     8
#else	//AT32F435VMT7
	#define MODULE_INFO "PREMIUM63"
	#define MAX_MODULE_INFO_LEN     9
#endif	//AT32F435VMT7
#else
#ifdef AT32F435VMT7
	#define MODULE_INFO "ARTERY"
	#define MAX_MODULE_INFO_LEN     6
#else	//AT32F435VMT7
	#define MODULE_INFO "PREMIUM"
	#define MAX_MODULE_INFO_LEN     7
#endif	// AT32F435VMT7
#endif
	#define MAX_COUNTRY_CODE_LEN    2
#endif


#else
	#define	HTTP_FOTA_URL															"gds.hyundai-motor.com"
	#define	HTTP_FOTA_PATH_REQUEST_VERSION							"autolink_premium/Autolink-Premium/FW_UPDATEIF.aspx"
	#define	HTTP_FOTA_PATH_REQUEST_FILE_DOWNLOAD				"autolink_premium/Autolink-Premium/FW_FileDownload.aspx"
	#define	HTTP_FOTA_PATH_REQUEST_VEHICLE_INFO					"autolink_premium/Autolink-Premium/FW_websvc_mws.aspx"

    #define HTTP_TEST_FOTA_PATH_REQUEST_VERSION					"autolink_dev_premium/Autolink-Premium/FW_UPDATEIF.aspx"
    #define HTTP_TEST_FOTA_PATH_REQUEST_FILE_DOWNLOAD		"autolink_dev_premium/Autolink-Premium/FW_FileDownload.aspx"
    #define HTTP_TEST_FOTA_PATH_REQUEST_VEHICLE_INFO			"autolink_dev_premium/Autolink-Premium/FW_websvc_mws.aspx"
#endif




//************************************************************************************************

#define DEFAULT_FLAG_TEST											"test"
#define DEFAULT_FLAG_RELEASE									"release"

////#define DEFAULT_HTTP_HEADER_1									""
// 128 byte 넘는지 확인 할 것!!!(MAX_HCPROP_LENGTH = 128)
#define DEFAULT_HTTP_HEADER										"Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW"				// 사실 header line이 두개이지만, \r\n으로 구분을 하지는 않는다.
#define DEFAULT_HTTP_BODY_START								"------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name=\"jsondata\"\r\n\r\n"

#define DEFAULT_HTTP_BODY_END									"------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n"				// --> 마지막에 \r\n이 반드시 필요하다.

#define DEFAULT_HTTP_MESSAGE_HEADER						"Content-Type: application/json"

#define NEWFOTA_HTTP_MESSAGE_HEADER_CONTENT		"Content-Type: application/json"

#define NEWFOTA_HTTP_MESSAGE_HEADER_AUTH				"Authorization: autolink"



#define DEFAULT_HTTP_GET_VEHICLE_INFO_HEADER	"Content-Type: application/x-www-form-urlencoded"




#define JSON_DATA_ORDER_DIAGNOSIS_MASTER				0
#define JSON_DATA_ORDER_DIAGNOSIS_SLAVE					1

#define JSON_DATA_ORDER_FIRMWARE_MAIN_BOOT			0
#define JSON_DATA_ORDER_FIRMWARE_MAIN_APP				1

#define JSON_DATA_ORDER_FIRMWARE_EXT_BOOT				2
#define JSON_DATA_ORDER_FIRMWARE_EXT_APP				3

typedef enum _eFOTA_FILE_TYEP {
	eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT,
	eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP,
	eFOTA_FILE_TYPE_DIAGNOSIS_MASTER,
	eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE,
	eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL,
#if defined(EXTBOARD_FOTA)
	eFOTA_FILE_TYPE_FDBOARD_BOOT,
	eFOTA_FILE_TYPE_FDBOARD_APP,
	eFOTA_FILE_TYPE_FDBOARD_ARTERY_BOOT,
	eFOTA_FILE_TYPE_FDBOARD_ARTERY_APP,
#endif
	eFOTA_FILE_TYPE_NONE,
} eFOTA_FILE_TYEP;

typedef enum _eFOTA_RESULT_CODE
{
	eFOTA_RESULT_CODE_NONE = 0,
	eFOTA_RESULT_CODE_SUCCESS,
	eFOTA_RESULT_CODE_ERROR_HTTPSND_CMD,								// 2	AT@HTTPSND 명령에 대한 응답이 0(정상)이 아닌 경우, 해당 코드는 g_nHTTPSNDResponseCode 변수에 저장했다. 28번인 경우는 서버에서 응답이 없는 경우이다.
	eFOTA_RESULT_CODE_NOT_READY,												// 3	FOTA 동작 준비가 되지 않았다. packet application mode가 시작 되지 않은 경우
	eFOTA_RESULT_CODE_DO_NOT_FOTA,											// 4	다운로드 받을 파일이 없을 경우
	eFOTA_RESULT_CODE_ERROR_MISSING_FILE_INFO,					// 5	서버로 부터 응답 받은 file 정보가 올바르지 않은 경우
	eFOTA_RESULT_CODE_ERROR_MISSING_FILE_INFO_KIND,			// 6	서버로 부터 응답 받은 file 정보에서 kind 항목 에러
	eFOTA_RESULT_CODE_ERROR_MISSING_FILE_INFO_ORDER,		// 7	서버로 부터 응답 받은 file 정보에서 order 항목 에러
	eFOTA_RESULT_CODE_ERROR_MISSING_FILE_CHECKSUM,			// 8	서버로 부터 받은 file의 checksum이 틀린 경우
	eFOTA_RESULT_CODE_ERROR_EMMC_FILE_OPEN_READ,				// 9	EMMC에 저장된 file을 open/read 하지 못하는 경우
	eFOTA_RESULT_CODE_ERROR_SEVER_NO_RESPONSE,					// 10	서버에서 응답이 없는 경우
	eFOTA_RESULT_CODE_ERROR_HEAP_MEMORY,								// 11	다운로드 받을 file 정보를 저장하기 위한 data memory를 malloc으로 할당 받지 못할 경우
	eFOTA_RESULT_CODE_ERROR_OVERFLOW_FILE_INFO_DATA,		// 12	다운로드 받을 file 정보가 할당 받은 data memory보가 클 경우
	eFOTA_RESULT_CODE_ERROR_FILE_DATA_LENGTH,						// 13	다운로드 받은 file data의 packet 크기가 실제 수신한 data의 길이와 다를 경우
	eFOTA_RESULT_CODE_ERROR_SAVE_FILE_DATA,							// 14	다운로드 받은 file data를 eMMC memory에 저장하지 못할 경우(eMMC file system error)

	// file data 요청에 대한 응답 구문을 분석한 결과
	eFOTA_RCV_DATA_ERROR_NONE,
	eFOTA_RCV_DATA_ERROR_NO_RESULTCODE,							// "resultCode" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_RCV_INFO_FAIL,							// "success" 아니고 "fail" 문자를 수신 받았을때
	eFOTA_RCV_DATA_ERROR_NO_FLAG,										// "flag" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_NO_TEST_OR_RELEASE,				// "test" or "release" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_NO_RESPONSELIST,						// "responseList" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_NO_KIND,										// "kind" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_NO_DIAGNOSIS_OR_FIRMWARE,	// "diagnosis" or "firmware" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_NO_ORDER,									// "order" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_NO_ORDER_NO,								// order no가 비정상일 경우
	eFOTA_RCV_DATA_ERROR_NO_VERSION,								// "version" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_NO_FILENAME,								// "filename" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_NO_FILESIZE,								// "filesize" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_NO_FILECHECKSUM,						// "filechecksum" 문자열이 없을때
	eFOTA_RCV_DATA_ERROR_INFO_FILE_COUNT,						// 파일 정보가 MAX_FILE_INFO_NO 보다 클경우

	eFOTA_RESULT_CODE_ERROR_HTTP_COMM_FAIL,					// "HTTP POST Response" 응답으로 200이 와야하지만 그렇지 않은 경우
	eFOTA_RESULT_CODE_ERROR_FLASH_ERASE_FAIL,				// internal flash에 삭제 실패
	eFOTA_RESULT_CODE_ERROR_FLASH_WRITE_FAIL,				// internal flash에 저장 실패
	eFOTA_RESULT_CODE_ERROR_UPDATE_BINDATA_FAIL,		// 실제로 저장될 internal flash에 저장 실패한 경우 또는 file update 정보 저장이 실패한 경우
	eFOTA_RESULT_CODE_LOW_SIGNAL_QUILITY,						// RSSI 값이 낮아서 FOTA를 수행하지 않았다.
	eFOTA_RESULT_CODE_NOT_NWETWORK_REGISTERED,
	eFOTA_RESULT_CODE_HTTP_COMM_ERROR,							// ^SIS 응답으로 오는 error message 발생, ModemManagerData.nHTTPURCInfoId 값을 AT command manual의 10.14.(page 202) 챕터를 참조할 것
	eFOTA_RESULT_CODE_REMODE_HOST_HAS_RESET_THE_CONNECTION,							// ^SIS 응답으로 오는 error message 발생, ModemManagerData.nHTTPURCInfoId 값을 AT command manual의 10.14.(page 202) 챕터를 참조할 것
	eFOTA_RESULT_CODE_CME_ERROR,										//
	eFOTA_RESULT_CODE_RETRY_OVER_ERROR,							//
	eFOTA_RESULT_CODE_NO_RESP,											//
	eFOTA_RESULT_CODE_MODEM_WARNING,								//
	eFOTA_RESULT_CODE_ZERO_LENGTH,								//

} eFOTA_RESULT_CODE;

typedef enum _eFW_DOWNLOAD_STATE
{
	eFOTA_STATE_INIT = 0,
	eFOTA_STATE_WAIT_RESPONSE,
	eFOTA_STATE_CHECK_NETWORK_STATE,
	eFOTA_STATE_CHECK_SIGNAL_QUILITY,
	eFOTA_STATE_DISABLE_GPS,
	eFOTA_STATE_SELECT_DOWNLOAD_FILE,
	eFOTA_STATE_ERASE_INTERNAL_FLASH,
	eFOTA_STATE_MAKE_REQUEST_CONTENT,
	eFOTA_STATE_WAIT_FILE_DOWNLOAD_COMPLETE,
	eFOTA_STATE_CONFIRM_CHECKSUM,
	eFOTA_STATE_UPDATE_BIN_DATA,
	eFOTA_STATE_CHECK_CONTINUE,

	eFOTA_STATE_MAKE_FILE_INFO,				// 5
	eFOTA_STATE_SEND_FILE_INFO,
	eFOTA_STATE_FILE_INFO_ANALYSIS,
	eFOTA_STATE_CONFIRM_FILE_VERSION,
	eFOTA_STATE_MAKE_REQUEST_FILE_DATA,
	eFOTA_STATE_ENABLE_GPS,
	eFOTA_STATE_STOP,													// 14
}eFOTA_STATE;

typedef enum _eHTTP_CMD_PARAM_DATA_MODE
{
	eHTTP_CMD_PARAM_DATA_MODE_ASCII = 0,
	eHTTP_CMD_PARAM_DATA_MODE_HEX,
} eHTTP_CMD_PARAM_DATA_MODE;

typedef enum _eHTTP_FOTA_REQ_TYPE
{
	eHTTP_FOTA_REQ_TYPE_VERSION = 0,
	eHTTP_FOTA_REQ_TYPE_FILE_DOWNLOAD,
	eHTTP_FOTA_REQ_TYPE_VEHICLE_INFO,
} eHTTP_FOTA_REQ_TYPE;

typedef enum _eJSON_DATA_FLAG
{
	eJSON_DATA_FLAG_TEST = 0,
	eJSON_DATA_FLAG_RELEASE,
}eJSON_DATA_FLAG;

typedef enum _eMODEM_RECEIVE_DATA_TYPE
{
	eMODEM_RECEIVE_DATA_TYPE_NORMAL = 0,
	eMODEM_RECEIVE_DATA_TYPE_ASCII,
	eMODEM_RECEIVE_DATA_TYPE_BIN,
} eMODEM_RECEIVE_DATA_TYPE;

typedef __packed struct _stJSON_REQUEST_LIST_INFO
{
	uint8_t m_cOrder;
	uint16_t m_nVersion;
	uint8_t m_strFileName[32];
	uint32_t m_wFileSize;
	uint32_t m_wFileCheckSum;
} stJSON_FILE_INFO;

typedef __packed struct _stFileInfo
{
#if defined (OLD_FOTA)
	uint8_t m_cOrder;
	uint16_t m_nVersion;
	uint8_t m_strFileName[32];
	uint32_t m_wFileSize;
	uint32_t m_wFileCheckSum;
	uint8_t m_strFolderName[32];
#else
    uint8_t m_cOrder;
    uint8_t m_strCategory[32];
	uint8_t m_strFileName[40]; // 2022/12/27 Artery CANFD로인한 사이즈 증가 32 -> 40
	uint32_t m_wFileSize;
    uint16_t m_nVersion;
	uint32_t m_wFileCheckSum;
#endif
} stFileInfo;

typedef __packed struct _stUpdateFileInfo
{
	uint16_t									m_nFileNo;
	stFileInfo									m_stFileInfo[MAX_FILE_INFO_NO];
} stUpdateFileInfo;

#define MAX_AREA_SIZE								4
#define MAX_MAKER_SIZE							4
#define MAX_COUNTRY_CODE_SIZE				10

typedef struct _stCountryMakerAreaInfo {
	uint8_t arrArea[MAX_AREA_SIZE];
	uint8_t arrMaker[MAX_MAKER_SIZE];
	uint8_t arrCountryCode[MAX_COUNTRY_CODE_SIZE];
} stCountryMakerAreaInfo;

#define MAX_LEN_VINFO_MSG						30
#define MAX_LEN_VINFO_USERID					20
#define MAX_LEN_VINFO_TIMESTAMP				20
#define MAX_LEN_VINFO_VIN							30
#define MAX_LEN_VINFO_DEVOPR            		4
#ifdef RF_COMMON_MODEM
#define MAX_LEN_VINFO_MODULE            		10
#else
#define MAX_LEN_VINFO_MODULE            		8
#endif
#define MAX_LEN_VINFO_COUNTRY_CODE		10
#define MAX_LEN_VINFO_VIN_MAKER           	10
#define MAX_LEN_VINFO_AREA						20
#define MAX_LEN_VINFO_MODEL_CODE			20
#define MAX_LEN_VINFO_MODEL_DESC			20
#define MAX_LEN_VINFO_YEAR						20
#define MAX_LEN_VINFO_MODEL_YEAR       	5
#define MAX_LEN_VINFO_ENGINE_CODE			20
#define MAX_LEN_VINFO_ENGINE_DESC			20
#define MAX_LEN_VINFO_DIAGNOSISDB_VER  	5
#define MAX_LEN_VINFO_MASTERDB_VER      	5
#define MAX_LEN_VINFO_SCAN_DB				10
#define MAX_LEN_VINFO_FCS_DB					10
#define MAX_LEN_VINFO_CT_DB             		10
#define MAX_LEN_VINFO_DB_VER					10
#define MAX_LEN_VINFO_VC_CODE				10
#define MAX_LEN_VINFO_VC_APP					17
#define MAX_LEN_VINFO_IS_SMK					3
#define MAX_LEN_VINFO_GEAR                  	3

#define MAX_LEN_RESP_LIST_SYSSUBITEMDESC				20
#define MAX_LEN_RESP_LIST_SYSTEMCODE						10
#define MAX_LEN_RESP_LIST_SYSTEMTYPE						10
#define MAX_LEN_RESP_LIST_IMAGEDEC							20

#define MAX_VEHICLE_INFO_RESPONSE_LIST_CNT			10

typedef __packed struct _stResponseList
{
	char m_aSysSubItemDesc[MAX_LEN_RESP_LIST_SYSSUBITEMDESC];
	char m_aSystemCode[MAX_LEN_RESP_LIST_SYSTEMCODE];
	char m_aSystemType[MAX_LEN_RESP_LIST_SYSTEMTYPE];
	char m_aImageDesc[MAX_LEN_RESP_LIST_IMAGEDEC];
} stResponseList;

typedef __packed struct _stVehicleInfo
{
#if defined (OLD_FOTA)
	char m_aMsg[MAX_LEN_VINFO_MSG];
	char m_aUserID[MAX_LEN_VINFO_USERID];
	char m_aTimeStamp[MAX_LEN_VINFO_TIMESTAMP];
	char m_aVIN[MAX_LEN_VINFO_VIN];
	char m_aArea[MAX_LEN_VINFO_AREA];
	char m_aModelCode[MAX_LEN_VINFO_MODEL_CODE];
	char m_aModelDesc[MAX_LEN_VINFO_MODEL_DESC];
	char m_aYear[MAX_LEN_VINFO_YEAR];
	char m_aEngineCode[MAX_LEN_VINFO_ENGINE_CODE];
	char m_aEngineDesc[MAX_LEN_VINFO_ENGINE_DESC];
	char m_aScanDB[MAX_LEN_VINFO_SCAN_DB];
	char m_aFcsDB[MAX_LEN_VINFO_FCS_DB];
	char m_aDBVer[MAX_LEN_VINFO_DB_VER];
	char m_aVCcode[MAX_LEN_VINFO_VC_CODE];
	char m_aVCApp[MAX_LEN_VINFO_VC_APP];
	char m_aIsSMK[MAX_LEN_VINFO_IS_SMK];

	stResponseList m_stResponseList[MAX_VEHICLE_INFO_RESPONSE_LIST_CNT];
#else
    char m_aVIN[MAX_LEN_VINFO_VIN];
    char m_aDEVOPR[MAX_LEN_VINFO_DEVOPR];
    char m_aModule[MAX_LEN_VINFO_MODULE];
    char m_aArea[MAX_LEN_VINFO_AREA];
    char m_aCountryCode[MAX_LEN_VINFO_COUNTRY_CODE];
    char m_aVINMaker[MAX_LEN_VINFO_VIN_MAKER];
    char m_aModelCode[MAX_LEN_VINFO_MODEL_CODE];
	char m_aModelDesc[MAX_LEN_VINFO_MODEL_DESC];
	char m_aModelYear[MAX_LEN_VINFO_MODEL_YEAR];
    char m_aEngineCode[MAX_LEN_VINFO_ENGINE_CODE];
	char m_aEngineDesc[MAX_LEN_VINFO_ENGINE_DESC];
    char m_aDiagnosisDB_Ver[MAX_LEN_VINFO_DIAGNOSISDB_VER];
    char m_aMasterDB_Ver[MAX_LEN_VINFO_MASTERDB_VER];
    char m_aScanDB[MAX_LEN_VINFO_SCAN_DB];
	char m_aFcsDB[MAX_LEN_VINFO_FCS_DB];
    char m_aCTDB[MAX_LEN_VINFO_CT_DB];
    char m_aVCcode[MAX_LEN_VINFO_VC_CODE];
	char m_aVCApp[MAX_LEN_VINFO_VC_APP];
	char m_aIsSMK[MAX_LEN_VINFO_IS_SMK];
    char m_aGear[MAX_LEN_VINFO_GEAR];

	stResponseList m_stResponseList[MAX_VEHICLE_INFO_RESPONSE_LIST_CNT];
#endif
} stVehicleInfo;

typedef __packed struct {
	eFOTA_STATE eState;
	eFOTA_STATE eNextState;
	eFOTA_RESULT_CODE eFotaResultCode;
	uint16_t nCurrDownloadFileNo;
	uint16_t nMaxDownloadFileNo;
	eFOTA_FILE_TYEP eCurrFotaFileType;
	uint32_t wInternalFlashAddress;
	bool bFlashWriteSuccess;
} FOTA_MANAGER_DATA;


extern FOTA_MANAGER_DATA g_FotaManagerData;
extern u16 g_nCurrentDownloadFile;
extern eMODEM_RECEIVE_DATA_TYPE geModemReceiveDataType;
extern uint16_t g_nHTTPSNDResponseCode;
extern stUpdateFileInfo g_stUpdateFileInfoList;
extern stVehicleInfo g_stVehicleInfoList;

void Convert_Ascii_To_HexString(char *pHexBuf, char *pAsciiBuf);
void Convert_Ascii_To_HexString_len(char *pHexBuf, char *pAsciiBuf, int length);
u16 Convert_HexString_To_Bin(char *pBinBuf, char *pHexBuf, int nLength);
u16 Convert_HexString_To_Bin_URL(char *pBinBuf, char *pHexBuf, int nLength);
u8 Md_StartFOTA(u8 cOrder);
void FOTAStop(eFOTA_RESULT_CODE eRetCocd);
void Md_ForceFileSave_3SecCallback(void);
void MDRecvHttpRes(BYTE* pData, unsigned int nLength);
unsigned int ConvertData(unsigned char *input);

void FOTA_GetVersion(void);
void FOTA_GetBin(void);

eMODEM_PROCESS_FUNC_RET FOTA_RecvBinProcess(void);
void FOTA_SetResultMessageflag(void);

void MDResSECCA(BYTE* pData, unsigned int nLength);
void MDcheckRespAuthetication(uint8_t mode);
bool FOTA_SaveBinDataToInternalFlash(uint8_t* Data, uint16_t DataLength);

extern bool FOTA_MakeRequestVersionContent(void);
extern bool NEWFOTA_MakeRequestVersionContent(void);
extern eFOTA_RESULT_CODE FOTA_ParseReceivedVersionInfo(void);
extern eFOTA_RESULT_CODE NEWFOTA_ParseReceivedVersionInfo(void);
extern eFOTA_RESULT_CODE FOTA_ParseReceivedVehicleInfo(void);
extern eFOTA_RESULT_CODE NEWFOTA_ParseReceivedVehicleInfo(void);
void ReadFOTA_URL(stServerUrl *prtUrl,eHTTP_FOTA_REQ_TYPE eType);
boolean_t FindLastAddressofDB(int eCurrFotaFileType, char* dbName, int32_t nVer, uint16_t* pusCheckSum);


#endif /* __FOTA_MANAGER_H__ */

/***************************** END OF FILE ****/
