#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "typedef.h"
#include "ff.h"

/*---------------------------------------------------------------------------------------------
     For debuging
---------------------------------------------------------------------------------------------*/
//#define TEST_FIRMWARE_INFO														// Firmware Info를 테스트 할려면 주석 제거
//#define USE_APPLICATION_CHECKCS													// CheckSum을 검사할려면 주석 제거

/*  FW_TEST_VERSION_OVERRIDE: 정의하면 HSM/RS9116 버전 응답을 파일/CLI 로 강제 변경 가능.
 *  - FL_Git_HSM_GetVersion (0x121C) 가 Load_HSM_TestVersion() 사용
 *  - FL_RS9116VersionCheck  (0xE037) 가 Load_RS9116_TestVersion() 사용
 *  - hsm_firmware_upgrade 종료 시 HSM_TEST_VERSION_AFTER_UPG 자동 저장
 *  - RS9116FWUpdate       진입 시 RS9116_TEST_VERSION_AFTER_UPG 자동 저장
 *  - CLI 명령어 hsmtestver / wlantestver 활성화
 *  정의 안 하면(default) 기존 하드코딩 응답(1001 / "1610.2.10.0.0.5") 그대로 사용. */
//#define FW_TEST_VERSION_OVERRIDE

/*---------------------------------------------------------------------------------------------
     Defines
---------------------------------------------------------------------------------------------*/

#if (VCI_III_ASING_MODE)
	#define KKT_TEST  //주석 풀면 이더넷 VLAN DISABLE, 서버/클라이언트 IP 를 PC연결 가능 상태로 변경 - PING TEST 가능
#endif
#define FIRMWARE_INITIALIZED						0xAA
#define FIRMWARE_INITIALIZED_V2						0xBB
#define FIRMWARE_INITIALIZED_V3						0xCC
//#define APPLICATION_SECTOR_COUNT					2								// Application sector count : 1 sector = 128K
#define	FIRMWARE_RETRY_COUNT						3								// Firmware function retry count
#define	FIRMWARE_RETRY_DELAY						10								// Firmware function retry delay(ms)

#define FIRMWARE_BOOTLOADER_ADD						((uint32_t)0x08000000)
#define FIRMWARE_MAINAPP_ADD						((uint32_t)0x08020000)
#define FIRMWARE_RECOVERY_ADD                       ((uint32_t)0x08120000)
#define FIRMWARE_RECOVERY_INFO_OFFSET               ((uint32_t)0x00000200)
//#define FIRMWARE_TEMPORARY_ADD					((uint32_t)0x08180000)
//#define FIRMWARE_REPROGRAMAPP_ADD					((uint32_t)0x08100000)
#define FIRMWARE_INFO_ADD						    ((uint32_t)0x080E0000)
#define FIRMWARE_BK_INFO_ADD                        ((uint32_t)0x08100000)
//#define CONFIG_DATA_ADD								((uint32_t)0x08104000)

#define MAX_APP_CNT									40
#define	VERSION_SIZE								2
#define MODEL_NAME									"VCI3"
#define	MODEL_NAME_SIZE								sizeof( MODEL_NAME ) - 1
#define	SERIAL_NUMBER								"VCI12345"						// 8Byte
#define	SERIAL_NUMBER_SIZE							sizeof( SERIAL_NUMBER ) - 1

#define EMMC_ROOT_FOLDER							"/"
#define EMMC_APPLICATION_FOLDER						"/01_Application"
#define EMMC_BACKUP_FOLDER							"02_Backup"
#define EMMC_RECORD_FOLDER							"03_Record"

#define APPLICATION_INFO_FILE_NAME					"AppSwList.ini"

//#define FW_VERSION_FILE_NAME						"01_Application/AppSwList.ini"
#define MAIN_BACKUP_FILE_NAME						"Backup/mainApplication.bin"
#define RECOVERY_FW_NAME                            "C_vci3_recovery.bin"

#define	BLUETOOTH_DEVICE_MAX						10

// Upgrade File
#define READ_UPGRADE_FILE_OK						0
#define READ_UPGRADE_FILE_SIZE_ERROR				1
#define READ_UPGRADE_FILE_READ_ERROR				2
#define READ_UPGRADE_FILE_NOT_EXIST					3

#define TOKEN_NUMBER								5
#define TOKEN_SEPARATORS							" ,:;\t\r\n"

#define UPDATE_BOOTLOADER_NUMBER					0x01
#define UPDATE_APPLICATION_NUMBER					0x02
#define UPDATE_REPROGRAM_NUMBER						0x03
#define UPDATE_TOTALVERSION_NUMBER					0x00

#define ENCRYPT_PRJ									1
#define ENCRYPT_PRJ_LOG								0

#define BAKUPSRAM_ADDRESS							0x38800000
#define FAILCOUNT_ADDRESS							0x01
#define FAILFLAG_ADDRESS							0x400

/*---------------------------------------------------------------------------------------------
     Typedefs
---------------------------------------------------------------------------------------------*/
typedef enum
{	
	BOOT_DOWNLOADER		= 0,
	BOOT_DIAGNOSIS		= 1,
	BOOT_DLOGGER		= 2,
	BOOT_RECORDER		= 3,
	BOOT_PDI			= 4,
	BOOT_REPROGRAM		= 5
} BootMode;

typedef enum
{
	TARGET_BOOTLOADER	= 1,
	TARGET_DIAGNOSIS	= 2,
	TARGET_DLOGGER		= 3,
	TARGET_RECORDER		= 4,
	TARGET_PDI			= 5,
	TARGET_REPROGRAM	= 6
} UpdateTarget;

typedef enum
{
	eApp_Downloader			= 0,
	eApp_VCI_2				= 1,	
	eApp_ECUUpCAN			= 2,
	eApp_ECUUpKWP			= 3,
	eApp_ECUUpCV			= 4,
	eApp_Inside				= 5,
	eApp_TMB				= 6, 		
	eApp_TMD				= 7, 		
	eApp_TMTM				= 8, 		
	eApp_Inside2			= 9, 	
	eApp_bootloader			= 10,	
	eApp_Recovery			= 11, 	
	eApp_Selftest			= 12, 	
	eApp_ECUUpCCP			= 13, 	
	eApp_ECUUpFlexRay		= 14,
	eApp_ECUUpDownloader	= 16,
	eApp_ECUUpCVKWP			= 18,
	eApp_VCI_II_PDI			= 19,
	//eApp_HSM_UPDATE1		= 20,//for hsm update//HSM_Applet_V10.bin
	//eApp_HSM_UPDATE2		= 21,//for hsm update//HSM_Applet_V20.bin
	eApp_ECUUP_COMMON		= 22,//for stand alone reprogram (RDBI)
	eApp_ECUUP_STDA			= 23,//for stand alone reprogram
	//eApp_CFCI101_BOOT		= 26,//for CFCI-101 update
	//eApp_CFCI101_APP		= 27,//for CFCI-101 update
	//eApp_CFCI101_SELF		= 28,//for CFCI-101 update
	//eApp_HSM_UPDATE3		= 29,//for hsm update//ASK_Master_App.bin
	//eApp_HSM_UPDATE4		= 30,//for hsm update//HSM_mainApplet.bin
	eApp_MAX				= 37
} AppName;

typedef enum
{	
    eAppOK              = 0,
    eInvalidListSize    = 1,
    eListOpenFail       = 2,
    eRestoreAppFail     = 3,
    eListNumError       = 4,
    eBootRecovery       = 5,
    eMax                = 6,
} eResVerifiyApp;

typedef __packed struct _SAppInfo
{
	uint8_t		marrucVersion[VERSION_SIZE];				// Application Version
	uint32_t	mSize;										// Application Size
	uint32_t	mCheckSum;									// Application CheckSum
	uint8_t		mSectorCount;								// Application Sector Count
	uint8_t		mReserved[20];								// Reserved Area
} SAppInfo;													// Total 160Byte

typedef __packed struct _SBTbdInfo
{
	uint8_t		mdev_name[20];								// Remote devcie Name
	uint8_t		mdev_addr[6];								// Remote devcie Address
	uint8_t		mlinkKey[16];								// linked key
} SBTbdInfo;

typedef __packed struct _SBTInfo
{
    uint8_t     strMAC_Address[18];
	uint8_t		mHostOldIdx;								// QIdx
	SBTbdInfo	mTrigInfo;									// Trigger Module Info
	SBTbdInfo	mHostInfo[BLUETOOTH_DEVICE_MAX];			// Host PC Info
} SBTInfo;

typedef __packed struct _SWifiInfo		//last connected info
{
	uint8_t		initialized;								// AP infomation initialized
	uint8_t		mode;										// 0:OPEN, 1,WPA, 2:WPA2, 3:WEP
	uint8_t		ipSetting;									// 0 : DHCP 1 : Static
	uint8_t		PSK_length;									// PSK_length
	char		PSK_Key[100];								// PSK_Key
	uint8_t		SSIDlength;									// SSIDlength
	char		SSIDname[100];								// SSIDname
	uint32_t	SourceIpAddress;							// VCI III IpAddress
	uint32_t	SubnetMask;									// VCI III SubnetMask
	uint32_t	Gateway;									// VCI III Gateway
	uint32_t	TargetIpAddress;							// PC IpAddress
	uint32_t	port;										// port
} SWifiInfo;

typedef __packed struct _SServerInfo     					// 서버 정보 구조체
{
	uint8_t		protocolType;								// 0: WebSocket, 1: MQTT 등으로 구분
	uint8_t		websocket_domain[128];						// websocket 도메인 이름 (최대 127자 + 널 종료)
	uint8_t		websocket_resource[128];					// websocket 리소스 네임 (최대 127자 + 널 종료)
	uint16_t	websocket_port;								// websocket 포트번호 (2바이트)
	uint8_t		mqtt_domain[128];							// mqtt 도메인 이름 (최대 127자 + 널 종료)
	uint16_t	mqtt_port;									// mqtt 포트번호 (2바이트)
} SServerInfo;

#ifdef VCI_III_USB_HS
typedef __packed struct _SFDCanInfo
{
	uint8_t		mBypassFlag;								// Bypass on / off
	uint8_t		mBypassMode;								// Can / CanFD
	uint8_t		mBaudrateIndex;								// Baudrate Index
	uint8_t		mSWFilterNum;								// S/W Filter Number
	uint16_t	mSWFilter[200];								// S/W Filter
	uint8_t		mHWFilterNum;								// H/W Filter Number
	uint16_t	mHWFilter[32];								// H/W Filter - 0 : All
	uint8_t		mConnectLine;								// Connect Line
} SFDCanInfo;
#endif

typedef __packed struct _SFwInfo
{
  	uint32_t 	muiPreamble;
	// flag
	uint8_t		mucInitialized;								// Firmware Info initialied( AA:initialized )
	uint8_t		mucStandbyMode;								// Power Management : enter standby mode
	AppName		mucBootMode;								// Booting mode
	AppName		mucCurrentMode;								// 현재 App Flash Area에 저장되어 있는 App
	uint8_t		mucUpdated;									// update 성공 유무
	uint8_t		mucBackuped;								// backup 성공 유무
	uint8_t		mucChanged;									// Firmware Info 변경 유무
	uint8_t		mucBootUpdate;								// Bootloader Update 유무
	uint8_t		mucEmmcFormat;								// Emmc Format 유무
	uint8_t		mucModeChange;								// Mode Change 유무
	uint8_t		mucReserved1[22];							// For reserve

	// Model Info
	uint8_t		marrucModelName[MODEL_NAME_SIZE];			// Model Name
	uint8_t		marrucSerialNo[SERIAL_NUMBER_SIZE];			// Serial Number
	uint8_t		marrucTotalVersion[VERSION_SIZE];			// Total Version
	uint8_t		mucReserved2[47];							// For reserve : 32 - MODEL_NAME_SIZE(7) - SERIAL_NUMBER_SIZE(8) - VERSION_SIZE(2)

	// Application Info
	SAppInfo	msAppInfo[MAX_APP_CNT];						// Main Application Info
	uint8_t		mucReserved4[12];							// For reserve : SAppInfo size is 20Byte
//	SAppInfo	msReproAppInfo[MAX_APP_CNT];				// Reprogram Application Info
//	uint8_t		mucReserved5[12];							// For reserve : SAppInfo size is 20Byte
	uint8_t		mucReserved[78];							// For reserve : SAppInfo size is 20Byte

	SBTInfo		msBTDeviceInfo;								// Bluetooth Device Info
	SWifiInfo	msWifiConnectInfo;							// Wifi Last Connect Info
	//SServerInfo msServerConnectInfo;
#ifdef VCI_III_USB_HS
	uint8_t		mucCommandPort;								// Command Port 1: can1, 2: can2
	uint8_t		mucSwitchST;								// CanFD4 / CanFD5 select switch
	uint8_t		mucCan1Connect;								// Can1 -> Spi Connect line
	uint8_t		mucCan2Connect;								// Can2 -> Spi Connect line
	uint8_t		mucReserved8[4];							// For reserve
	SFDCanInfo	msFDCan1Info;
#endif
	uint32_t	muiCheckSum;
} SFwInfo;

#define VCI3_FWINFO_PREAMBLE (0xFE3131CA)

/*---------------------------------------------------------------------------------------------
     Functions
---------------------------------------------------------------------------------------------*/
extern void		printSerialNumber( void );
extern void		printFWVersion( void );
extern void		printFirmwareInfo( void );
extern int32_t	initFirmwareInfo( void );
extern int32_t  loadFirmwareInfo_Flash( SFwInfo* psFwInfo, uint32_t flashAdd );
extern int8_t 	Send_HSM_File_Flash( U8 *Data_Addr, int Count, int length, uint8_t firmware_type);
extern int32_t 	loadFirmwareInfo_EMMC( void );
extern int32_t 	load_S_ReproStart_EMMC( void );
extern bool 	UpdateBackupFirmwareInfo(bool bForceUpdate);
extern void     saveFirmwareInfo_Flash( SFwInfo* psFwInfo, uint32_t flashAdd );
extern void     saveFirmwareInfo_EMMC( bool bBkUpdate );
extern void     InitializeBackUpFwInfo_Flash( void );
extern void WriteBackupSRAM(uint16_t write_address,uint8_t write_data);
extern void ReadBackupSRAM(uint16_t read_address, uint8_t* read_data);
extern void BackupSRAM_Init(void);
extern void BackupSRAM_Deinit(void);

extern uint8_t	isDifferentVersion( uint8_t *compare1, uint8_t *compare2 );

extern eResVerifiyApp	checkApplication( void );
extern eResVerifiyApp   verifyApplication( void );

extern uint8_t 	RestoreApplication( TCHAR *path, uint32_t fsize, uint16_t fcs );
extern uint8_t 	UpdateFWVersion( void );

extern void 	InitializeRecoveryFirmwareInfo( void );
extern bool 	isFirmwareInfoBroken( SFwInfo	* psFwInfo );
extern uint32_t RecoveryFirmwareInfo(SFwInfo	* psFwInfo);
extern bool     checkRecoveryFirmware( void );
extern void     RecoverFirmware(void);
extern uint8_t  verifyFirmwareInfo( void );


extern uint32_t checkCS( uint32_t flashAdd, SAppInfo *appInfo );
extern void	dlcToVersion( uint8_t *version, int32_t dlc );
extern uint8_t VciChangeMode(uint8_t mode);
extern void RunModeSwitch(char cAppNubmer);
extern char GetCurFwServiceMode();

#if ( TEST_FIRMWARE_INFO )
extern void	testFirmwareInfo( void );
#endif	// TEST_FIRMWARE_INFO

/*---------------------------------------------------------------------------------------------
     Variables
---------------------------------------------------------------------------------------------*/
extern SFwInfo		gsFwInfo;
extern SServerInfo	msServerConnectInfo;
#ifdef __cplusplus
}
#endif

#endif /* __FIRMWARE_H__ */
