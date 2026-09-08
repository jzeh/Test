/**
  ******************************************************************************
  * @file    GIT_Util.h
  * @author  GIT Application Team by james jean
  * @version V1.1.0
  * @date    05-SEP-2013
  * @brief   Header for GIT_Util.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GIT_UTIL_H__
#define __GIT_UTIL_H__

#include "main.h"
#include "common.h"
#include "stdlib.h"
#include "GIT_InterProtocol.h"
#include "AutolinkMessage.h"
#include "AutolinkConfiguration.h"
#include "KISA_SEED_ECB.h"


#include "HalHandler.h"
#include "SPIDriverMicron.h"


#if defined(USE_GIT_FAT_FS)
#include "ff.h"
#include "ffconf.h"
#include "diskio.h"
#endif
#if defined(USE_GIT_LITTLE_FS)
#include "lfs.h"
#endif

#define SIZE_TEMP_BUFF				1024
#define SFLASH_SECTOR_SIZE 			(4*1024)
#define AUTOLINKDATA_BASE_DIR   "AutoLinkData"

//////////////////////////////////////////////////////////////////////////////////
//// QUEUE
//typedef struct _stQUEUE
//{
//	unsigned int 	uiQueueSize;
//	unsigned int 	uiFront;
//	unsigned int 	uiRear;
//	BYTE*		 	pData;
//}stQueue;
//
//stQueue* CreateQueue(unsigned uiQueueSize);
//void DestoryQueue(stQueue* pQueue);
//BOOL PushQueue(stQueue* pQueue, BYTE ucPushData, eCommType eCh);
//unsigned int PushMultiDataQueue(stQueue* pQueue, BYTE* pPushData, unsigned int uiPushDataLen, eCommType eCh);
//BOOL PopQueue(stQueue* pQueue,  BYTE *pPopData);
//unsigned int PopMultiDataQueue(stQueue* pQueue, BYTE* pPopData, unsigned int uiPopDataLen);
//BOOL IsEmptyQueue(stQueue* pQueue);
//unsigned int GetQueueDataLength(stQueue* pQueue);
//BOOL ClearQueue(stQueue* pQueue);
//////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// MODE SWITCH
//#define FILENAME_MODE_SWITCH_INI		"ModeSwitch.ini"
//#define FILENAME_APP_LIST_INI				"AppSwList.ini"
//#define STR_FW_SWITCH_COMPLETE			"FW_SWITCH_COMPLETE"
//#define STR_FW_SWITCH_READY  				"FW_SWITCH_READY"
#define DIR_ROOT										"/"
#define DIR_LOG											"LOG"
//#define DIR_RECORD									"RECORD"
#define DIR_FIRMWARE								"FIRMWARE"
#define DIR_INFORMATION							"INFO"
#define FAT_VOLUME_DRV							0
//
//#define FILENAME_VCI2_SERIAL_DAT		"VCI2Serial.dat"
//#define FILENAME_RESERV_INFO				"Reservation.inf"
//#define FILENAME_MASTERKEY_INFO 		"MasterKey.inf"
//#define FILENAME_FOTA_EXIST					"FOTAHistory.inf"
//#define FILENAME_VERSION_DAT				"VERSION.inf"
//#define PERIOD_TIME_INFO						"TIMESTAMP.inf"
//#define TIME_STAMP_INFO							"PTIME.inf"
//#define PERIOD_ENGINE_OFF_INFO			"ENGOFF.inf"
//#define DRIVER_STATE_INFO						"DRIVER.inf"
//#define GPS_STATE_INFO							"GPS.inf"
//#define SAVE_OBDATA_INFO						"SAVEOBD.inf"
//#define EVENT_STATE_INFO						"EVENT.inf"
//#define DenialSave_INFO							"Denial.inf"
//#define SAVE_SLEEP_INFO							"SLEEP.inf"
//#define Emer_INFO										"EMER.inf"


// MCU Flash address
#define BOOTLOADER_ADDRESS			(uint32_t)0x08000000		//16kbyte
#define FIRMWARE_INFO_ADDRESS		(uint32_t)0x08004000		//16kbyte

#define EMERGENCY_INFO_ADDRESS		(uint32_t)0x08007000		//16kbyte


#define CAR_MASTER_DB_ADDRESS		(uint32_t)0x08008000		//16kbyte
#define CAR_SLAVE_ADDRESS			(uint32_t)0x0800C000		//16kbyte
#define APPLICATION_ADDRESS			(uint32_t)0x08010000		//438kbyte
#define CAR_CTRL_DB_ADDRESS			(uint32_t)0x080C0000		//128kbyte, 실제로 사용하는 address

#define UPDATE_TMP_ADDRESS			(uint32_t)0x08110000		// updata해야 하는 application이 저장되어있는 address

#define RUNNING_ADDRESS					(uint32_t)APPLICATION_ADDRESS

typedef enum _eFWSwitchingModeStatus
{
	eFW_SWITCH_NONE,
	eFW_SWITCH_COMPLETE, 	// appl로 jump가 완료되면 complete를 bootloader에서 write해 준다.
	eFW_SWITCH_READY,		// Appl이 jump해야되는 appl list를 write하고 ready를 write해 준다.
	eFW_SWITCH_MAX
}eFWSwitchingModeStatus;

typedef enum eFWApplIndex
{
	eApp_Bootloader,
	eApp_Application,
	eApp_MasterDB,
	eApp_SlaveDB,
	eApp_ControlDB,
	eApp_MAX,
}eFWApplIndex;

typedef __packed struct _stSwitchingMode
{

	UINT iApplMode;	/*	0,Downloader,12.57
						1,VCI_2.bin,10.03
						2,ECUUpCAN.bin,10.04
						3,ECUUpKWP.bin,10.04
						4,ECUUpCV.bin,10.04
						5,Inside.bin,10.02
						6,TM_Bootloader.bin,1.0		// TRIGGER_MODULE_BOOTLOADER
						7,TM_Downloader.bin,1.0
						8,Trigger_Module.bin,1.0
						9,TBA_Bootloader.bin,1.0 	// TPMS_BT_ADAPTER_BOOTLOADER
						10,TBA_Downloader.bin,1.0
						11,TBA_Module.bin,1.0*/
	UINT iStatus;	/*	FW_UPDATE_COMPLETE
              			FW_UPDATE_READY */
}stSwitchingInfo;

BOOL GetSwitchingModeInfo(stSwitchingInfo* pSwitchMode);
uint32_t Appl_Mode_Switch(void);
void DefaultFWInfo(void);
void SelftestDefaultFWInfo(void);
void JumpToApp(uint32_t uiDestinationAddress);
BOOL DownloadAppToFlash(stSwitchingInfo *pSwitchModeInfo);
void Check_n_Update_Applicaton(void);
BOOL GetVCI2FileRead(char* strOpenFileName, BYTE* InBuff, UINT *iBuffLength);
BOOL GetVCI2FileWrite(char* strOpenFileName,U8 *buff, UINT iBuffLength);
BOOL DummyFileWrite(char* strOpenFileName,U8 *buff, UINT iBuffLength);
BOOL GetVCI2FileWrite2(char* strOpenFileName,U8 *buff, UINT iBuffLength);
BOOL GetVCI2FWAppList(BYTE* InBuff, UINT *iBuffLength);
BOOL DownloadClose(void);
BOOL UpdateAppListFromRemoteDevice(U8 *buff, U16 Size);
U8 GitWriteUpdateDataTemp(U8 *buff, U16 Size);
U16 GitCheckVCIFWCheckSum(void);
bool getMemoryfreeRecordCheck(char *STORE_REC_TEMP_FILE_NAME, U32 dwEmmcFrAddCnt);

#if defined(USE_FATFILE_SYSTEM)
BOOL InitFatFileSystem(void);

void LoadSettingInfoFile(bool bMode);
bool SystemTestInfoFileWrite(char *InBuff, uint16_t iBuffLength);
bool SystemTestInfoFileRead(char *InBuff, uint16_t iBuffLength);

//void directory_list(char * str);
void directory_list(void);
void DirectoryList(char* strScanDirectory);
void GetRecordFileList();
#endif

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// FIRMWARE INFOMATION
#define FIRMWARE_SIGNAL								(0xAABBCCDD)
#define FIRMWARE_APP_SIGNAL						(0xBACADAFA)
#define DEFAULT_FW_VERION							(0x00)
																		  // 012345678901234567890123456789
#define DEFAULT_FILE_NAME							"NOFILE"
#define DEFAULT_SERIAL_NUMBER                       "noSerialKRH0000"
#define SIZE_SERIAL_NUMBER							(15)
#define SIZE_NOSERIAL_NUMBER						(11)
#define IDX_CONTRY_CODE_SERIAL_NO                  (8)
#define IDX_MANUFACTURER_CODE                    (10)
#define SIZE_SERIAL_NUMBER_INT                      (7)

//호주향 단말 시리얼 번호 범위 : R0000001 ~ R1000000

//호주향 외 해외향 시리얼 번호 범위 
//ELS 모뎀 단말시리얼 번호 범위 : R1000001 ~ R1200000
//PLS 모뎀 단말시리얼 번호 범위 : R1200001 ~ R2000000
#define ELS_RANGE_START                           1000001
#define ELS_RANGE_END                             1200000

#define PLS_RANGE_START                           1200001
#define PLS_RANGE_END                             2000000   

#define SIZE_FIRMWARE_VER								2
#define MAX_FW_DB_FILE_NAME	 						6
#define SIZE_BLOCKBOX_STR								16
#define MAX_VEHICLECODE_SIZE						24
#define VIN_CODE_SIZE										18
#define MAX_USIM_NUM                    30
#define MAX_PHONE_NUM                   20
#define MAX_VEHICLE_AREA                6
#define MAX_VEHICLE_YEAR                               6
#define MAX_VEHICLE_ENGINE_CODE                    6

#define ENCRYPT_DECRYPT_DEFAULT_SIZE		(16)
#define ENCRYPT_KEY_SIZE					(32)	//(48)

#define FIRMWAREINFO_PREAMBLE		0x8791AEFE

#define GPS_IN_RANGE							0
#define GPS_OUT_RANGE						1

typedef __packed struct _stCANMaskReq
{
	INT32U StartMask[HAL_CAN_MAX_MASK_CNT];
	INT32U EndMask[HAL_CAN_MAX_MASK_CNT];
	INT8U MaskCnt;
}stCANMaskReq;

typedef __packed struct _FIRMWARE_APP_INFO
{
	int 	nSignal;
	BYTE 	arrFWName[MAX_FW_DB_FILE_NAME];
	uint16_t 	nAppFWVersion;
	uint32_t 	wFirmwareSize;
	uint16_t 	nCheckSum;
}FIRMWARE_APP_INFO;

typedef enum __DCSServiceType
{
    DCS_Retail = 0xa0,
    DCS_Fleet,
    DCS_MAX,
}DCSServiceType;

typedef enum __DCSApnType
{
    DCS_DEF_APN = 0xc0,
    DCS_GIT_APN,
    DCS_ApnMAX,
}DCSApnType;

typedef enum __DCSEncryptType
{
    DCS_ENC_AES = 0xb0,
    DCS_ENC_KMS,
    DCS_ENC_MAX,
}DCSEncryptType;

typedef __packed struct _GPSPOINT
{
	double m_dLon;
	double m_dLat;
}GPSPOINT;

#if defined(PROTOCOL17)
typedef __packed struct __stURLInfo{
	uint32_t nPreamble;
	uint32_t nEncryptedLength;
	uint32_t nLength;
	//uint8_t strUrl[MAX_SERVER_URL_LENGTH+8+1];	//패딩포함된 사이즈
	char strUrl[MAX_SERVER_URL_LENGTH+8+1];	//패딩포함된 사이즈
}stURLInfo;
#endif

typedef __packed struct _FIRMWARE_INFO
{
	int 				nSignal;
	stSwitchingInfo 	SwitchingInfo;
	BYTE				arrSerialNumber[SIZE_SERIAL_NUMBER +1];
	FIRMWARE_APP_INFO	AppProperty[eApp_MAX];
	int					nApplicationUpdateSignal; 	// Application을 업데이트 할 수 있도록 플래그 추가.
	BYTE				Written_bt_addr[ENCRYPT_DECRYPT_DEFAULT_SIZE + 1];  // Decrpyt시에 6byte에만 address가 저장되어 진다.
	stCANMaskReq 		g_stSavedCANMask;			// Nand에 저장되어 부팅시에 CAN 마스킹이 이루어져야 한다.
	BYTE				m_strVehicleCode[MAX_VEHICLECODE_SIZE + 2];		//FOTA를 사용하는 제품은 필요한 정보 //차종정보 모델코드
	BYTE				m_strVIN[VIN_CODE_SIZE + 1];
	BYTE                m_strUSIMNum[MAX_USIM_NUM + 1];
	BYTE                m_strPHONENum[MAX_PHONE_NUM + 1];
	BYTE				m_strVehicleArea[MAX_VEHICLE_AREA + 1];
	BYTE				m_strVehicleYear[MAX_VEHICLE_YEAR + 1];
	BYTE				m_strVehicleEngineCode[MAX_VEHICLE_ENGINE_CODE + 1];
	BYTE				m_strModuleKey[ENCRYPT_KEY_SIZE + 1];
	BYTE                bModemActive;
    DCSServiceType      nServiceType;
    DCSEncryptType      nEncryptType;
#if defined(PROTOCOL17)
	stURLInfo			m_stURLInfo;
#endif
	BYTE 				m_ucApnFlag;
	BYTE                m_ucCheckSum;
}FIRMWARE_INFO;

#pragma pack(push, 1)
typedef __packed struct _FIRMWARE_INFO_BACKUP
{
	FIRMWARE_INFO stFirmwareInfo;
	uint32_t nPreamble;
}FIRMWARE_INFO_BACKUP;
#pragma pack(pop, 1)

extern FIRMWARE_INFO 		g_FirmwareInfo;
extern eFWApplIndex g_cDownloadFW_AppNumber;

void SetSelftestModemUart(void);
void InitFirmwareInfo(void);
void GetFirmwareInfo(FIRMWARE_INFO *pFirmWareInfo);
void SetFirmwareInfo(FIRMWARE_INFO *pFirmWareInfo);
void ClearFirmwareInfo(void);
float GetFWAppVersion(eFWApplIndex eAppIdx);
char* GetFWSerialNumber(void);
BOOL SetFWSerialNumber(char* strSerialNum);
BOOL SetBTMacAddress(unsigned char* pMacAddr);
BOOL GetBTMacAddress(unsigned char* pBtMacAddr);
char* GetVehicleCode(void);
BOOL SetVehicleCode(U8 * pucVehicleCode);
DCSEncryptType GetEncryptType();
void SetEncryptType(DCSEncryptType nEncryptType);
DCSServiceType GetServiceType();
void SetServiceType(DCSServiceType nServiceType);

////////////////////////////////////////////////////////////////////////////////
unsigned char HexToDec(unsigned char ucHexData);
U8 AsciiToHex( U8 asciicode1, U8 asciicode2 );
U8 IsConfigFile(void);
U8 VciDirFile(U8 *DirBuff, U32 *DirBuffSize);
U8 VciRecvFileOpen(char *FileName, U32 FileSize);
U8 VciRecvFileReceive(U8 *buff, U16 Size);
U8 VciRecvFileClose();
U8 VciSendFileOpen(char *FileName, U32 *FileSize);
U8 VciReadFile(U32 ReadSize, U8 *FileBuff, U32 *FileSize);
U8 VciReadFile_Slave(U32 ReadSize, U8 *FileBuff, U32 *FileSize, U32 uiSequence);
U8 VciSendFileClose(void);
U8 VciEraseFile(void);
U8 VciChangeMode(U8 Mode);
void StartFRMode(void);
void StopFRMode(void);
void AlpuCheck(void);
unsigned int bit_mask(unsigned char comm_data);
unsigned int bit_mask1( unsigned char comm_data, unsigned char mask_data);
unsigned int bit_mask2(unsigned short comm_data, unsigned short mask_data);
unsigned int bit_mask3(unsigned char comm_data, unsigned char mask_data);
unsigned int bit_mask4(unsigned char comm_data, unsigned char mask_data);

void hexdump(void *mem, unsigned int len);
int ssscanf(const char *buff, int buff_sz, const char *format, ...);

int pkcs7_pad_write(char *source, int block_size, char *dest);
int pkcs7_pad_read(char *source, int sourceLength );

int GetCurrentTime_UnixTimeFormat();
void SystemSoftwareReset(void);
void GetDateTimeStamp(uint8_t *ptr, stHalRTC_DateTypeDef DateStamp, stHalRTC_TimeTypeDef TimeStamp);
int16_t CalcChecksum_Short(unsigned char* pBuff, unsigned int uiLength);

void GetModuleKey(char* pstrModuleKey);
BOOL SetModuleKey(char* strModuleKey);
void DisplayFirmWareInfo(void);

unsigned char PolygonGeoFenceCheck( GPSPOINT P, GPSPOINT* V, int n );
double isLeft( GPSPOINT P0, GPSPOINT P1, GPSPOINT P2 );
void Uart8_Baudrate_Set( unsigned int uiInput );
#endif /* __GIT_UTIL_H__ */

/***************************** END OF FILE ****/
