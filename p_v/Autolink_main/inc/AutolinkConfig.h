/**
  ******************************************************************************
  * @file    AutolinkConfig.h
  * @author  GIT Connectivity Development 2 Team
  * @version V1.0.0
  * @date    06-Feb-2018
  * @brief   Header for AutolinkConfig.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __AUTOLINK_CONFIG_H__
#define __AUTOLINK_CONFIG_H__

#include "common.h"
#include "MngSystemUtil.h"
#include "MngQueue.h"
#include "GIT_SensorProc.h"
#include "dukpt_cli.h"
#include "Git_Util.h"
#include "SPIDriverMicron.h"
#include "GIT_Gps.h"


// file managerment
// we assigned double size for backup
#define MAX_CONFIG_STRUCTURE_SIZE       (4*1024)
#define MAX_CONFIG_STORAGE_SIZE         (MAX_CONFIG_STRUCTURE_SIZE*43)

#define SECTOR_EXBOARD_APP_FIRMWARE		(2005)  // 007D 5000h - 007f 4fffh  //128kb
#define SECTOR_EXBOARD_BOOT_FIRMWARE    (2037)  // 007f 5000h - 007f cfffh	//32kb

#define SECTOR_CONFIG_FIRMWAREINFO      (2045)  // 007f d000h - 007f dfffh
// for config storage we use 2046 and 2047 sector
#define SECTOR_CONFIG_STORAGE           (2046)  // 007f e000h - 007f efffh
#define SECTOR_DEBUG_STORAGE_BACKUP     (2047)  // 007f f000h - 007f ffffh

#define SECTOR_EXBOARD_BOOT_FIRMWARE_SECTOR_SIZE (SECTOR_CONFIG_FIRMWAREINFO-SECTOR_EXBOARD_BOOT_FIRMWARE)	//8
#define SECTOR_EXBOARD_APP_FIRMWARE_SECTOR_SIZE	(SECTOR_EXBOARD_BOOT_FIRMWARE-SECTOR_EXBOARD_APP_FIRMWARE)	//32

// this is minimum sector size
#define CONFIG_READ_WRITE_SIZE 4096
#define MAX_ENCRYPT_KEY_LENGTH 16
#define URL_START_POSITION	8

#define CANFD_UPDATE_BOOT 0x01
#define CANFD_UPDATE_APPLICATION 0x02

typedef __packed struct __stAutolinkConfig{
    uint32_t nPreamble;
    uint32_t nEncryptSize;
    uint8_t carrrEncryptData[CONFIG_READ_WRITE_SIZE-8];
}stAutolinkConfig;

typedef enum _eCANFD_BOARD_NEED_UPDATE
{
	eCANFD_BOARD_NEED_UPDATE_NONE = 0,
	eCANFD_BOARD_NEED_UPDATE_BOOT,
	eCANFD_BOARD_NEED_UPDATE_APP,
	eCANFD_BOARD_NEED_UPDATE_BOTH,
} eCANFD_BOARD_NEED_UPDATE;

typedef enum _eCANFD_UPDATE_SEQUENCE
{
	eCANFD_UPDATE_INSTALLATION_NONE = 0,
	eCANFD_UPDATE_INSTALLATION_BOOT,
	eCANFD_UPDATE_INSTALLATION_APP,
} eCANFD_UPDATE_SEQUENCE;

typedef __packed struct __stCANFDBoardUpdateInfo{
	uint16_t usBootVersion;
	uint32_t uiBootSize;
	uint16_t usBootCheckSum;
	uint16_t usAppVersion;
    uint32_t uiAppSize;
	uint16_t usAppCheckSum;
	eCANFD_BOARD_NEED_UPDATE eNeedToUpdate;
}stCANFDBoardUpdateInfo;


#define MAX_PHONE_NUMBER_SIZE 20
#define MAX_AUTOCONFIG_VIN_SIZE MAX_VIN_SIZE
#define MAX_AUTOCONFIG_APN_SIZE MAX_APN_SIZE

#define EXTBOARD_UPDATE_SIGNAL 0xEFFEABBA


typedef __packed struct __stSystemConfig{
    uint8_t carrVin[MAX_AUTOCONFIG_VIN_SIZE];
    uint8_t carrCellPhoneNum[MAX_PHONE_NUMBER_SIZE];
    uint32_t unOdometer;
    double dlLastLatitude;
    double dlLastLongitude;
    uint8_t bModemActive;
    uint8_t cAllowNewVin;
    uint8_t cReserved;
    // this flag used for checking modem
    uint8_t bSystemReset;
    // this count used power on reset count
    uint16_t usSystemResetCount;
    // network
    stNetworkTime stNetworkInfo;
    DCSServiceType nServiceType;
    DCSEncryptType nEncryptType;
    stSensorInfo stGyroAngle;
	float fRemainedFuel;
	stCANFDBoardUpdateInfo stFDBoardUpdateInfo;
	bool bFahrenheit;
}stSystemConfig;

typedef __packed struct __CANInfo{
    uint8_t ucAutovinCANLine;
	uint8_t ucPACVType;
}stCANInfo;

typedef __packed struct __stAutolinkConfigData{
    stSystemConfig stSystem;
    stUserActionSetting stUserAction;
    DukptFutureKeyInfo stFutureKey;
	stCANInfo stCANInfo;
}stAutolinkConfigData;

enum{
    eAutoLinkConfig_None = 0,
    eAutoLinkConfig_Vin,
    eAutoLinkConfig_CellPhone,
    eAutoLinkConfig_Odotmeter,
    eAutoLinkConfig_LastLatitude,
    eAutoLinkConfig_LastLongitude,
    eAutoLinkConfig_ModemActive,
    eAutoLinkConfig_NeedUpdate,
    eAutoLinkConfig_SystemResetFlag,
    eAutoLinkConfig_SystemResetCount,
    eAutoLinkConfig_AllowNewVin,
    eAutoLinkConfig_ServiceType,
    eAutoLinkConfig_EncryptType,
    eAutoLinkConfig_NetworkInfo,
    eAutoLinkConfig_SensorInfo,
    eAutoLinkConfig_FDBoardUpdateInfo,
    eAutoLinkConfig_Fahrenheit,
    eAutoLinkConfig_PACVType,
    eAutoLinkConfig_AutovinCANLine,
#if defined(FEATURE_EXTENSION_BOARD)
    eAutoLinkConfig_AntiThiefFlag,
#endif
};

enum{
    eBackupRamConfig_Apn = 0,
    eBackupRamConfig_DrivingInterval,
    eBackupRamConfig_WakeupInterval,
    eBackupRamConfig_SystemTimeout,
    eBackupRamConfig_FotaInterval,
    eBackupRamConfig_NetworkInfo,
    eBackupRamConfig_WakeupAlramTime,
    eBackupRamConfig_PowerOffTime,
};


void GetDefaultEncryptKey(char* pcarrKey,boolean_t bDefault);
void WriteDefaultAutolinkConfigValue();


void SetBackupRamConfigProperty(uint8_t cIndex,void* pvValue);
void GetBackupRamConfigProperty(uint8_t cIndex,void* pvValue);
void SetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);

void ClearFutureKey();
void WriteConfig(boolean_t bExportFuturekey, boolean_t bDisp);

void SetSerialAndIpektoInternaFlash(char* pcarrIpek, int nIpekSize);
void WriteDefaultURL();
void WriteURL(char*);
void SetRequestIPEK();


#endif //__AUTOLINK_CONFIG_H__

/***************************** END OF FILE ****/
