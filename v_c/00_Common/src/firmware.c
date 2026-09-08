/*************************************************************
 * NOTE : firmware.c
 *      firmware interface
 *		firmware function retry count : FIRMWARE_RETRY_COUNT
 *		firmware function retry delay : FIRMWARE_RETRY_DELAY
 * Author : Lee junho
 * Since : 2019.04.12
**************************************************************/
#include <string.h>
#include <stdlib.h>

#include "stm32h7xx_hal.h"

#include "common.h"
#include "flash_if.h"
#include "firmware.h"
#include "git_mmc.h"
#include "buzzer.h"
#include "sw_timer.h"
#include "led.h"
#include "git_fsutil.h"
#if defined(VCI3_DIAG) && defined(VCI3_RECORD)
#include "git_rs9116.h"
#include "git_global.h"
#endif

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define	MAIN_BACKUP_TO_FLASH									0				// 0 : to Flash, 1 : to File									
/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
void		printSerialNumber( void );
void		printFWVersion( void );
void		printFirmwareInfo( void );
int32_t		initFirmwareInfo( void );
int32_t		loadFirmwareBackupInfo_Flash( void );
void		saveFirmwareInfo_Flash( SFwInfo* psFwInfo, uint32_t flashAdd );
void		saveFirmwareInfo_EMMC( bool bBkUpdate );
bool 		InitBackupFirmwareInfo( SFwInfo	* psFwBkInfo, SFwInfo * psFwInfo );
bool 		UpdateBackupFirmwareInfo(bool bForceUpdate);
FRESULT		WriteBackupFirmwareInfo( SFwInfo	* psFwBkInfo );
uint32_t 	ReadBackupFirmwareInfo( SFwInfo	* psFwBkInfo );
bool		RecoverSerial( void );
void		WriteBackupSRAM(uint16_t write_address,uint8_t write_data);
void		ReadBackupSRAM(uint16_t read_address, uint8_t* read_data);
void		BackupSRAM_Init(void);
void		BackupSRAM_Deinit(void);

uint8_t		isDifferentVersion( uint8_t *compare1, uint8_t *compare2 );

uint32_t	checkCS( uint32_t flashAdd, SAppInfo *appInfo );
uint32_t 	makeCS( uint32_t address, uint32_t size );

extern BOOL	g_bAckflag;

void	dlcToVersion( uint8_t *version, int32_t dlc );
#ifdef VCI3_RECORD
extern void ClearFwWakeupInfo();
extern void ClearFlightRecording();
extern void ClearTriggerHandler();
#endif
extern void	clearRXCanMessage( void );


/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
SFwInfo		gsFwInfo;
SServerInfo msServerConnectInfo;
/*----------------------------------------------------------------------
     Firmware Information
----------------------------------------------------------------------*/
void printSerialNumber( void )
{
	uint32_t	i;

	GLogI( " Serial Number  : " );
	for( i = 0; i < SERIAL_NUMBER_SIZE; i++ )
	{
		GLogI( "%c", gsFwInfo.marrucSerialNo[i] );
	}
	GLogI( "\r\n\n" );
}

void printFWVersion( void )
{
	GLogI( "==================================================\r\n" );
	GLogI( "===   Print Firmware Version   ===================\r\n" );
	GLogI( "==================================================\r\n" );
	GLogN( " bootloader   ver : %02d%02d\r\n", gsFwInfo.msAppInfo[eApp_bootloader].marrucVersion[0], gsFwInfo.msAppInfo[eApp_bootloader].marrucVersion[1]);
	for(uint8_t i = 0; i < MAX_APP_CNT; i++)
	{
		if((gsFwInfo.msAppInfo[i].marrucVersion[0]==0)&&(gsFwInfo.msAppInfo[i].marrucVersion[1]==1)){}
		else
			GLogN( " application[%02d]  ver : %02d%02d\r\n", i, gsFwInfo.msAppInfo[i].marrucVersion[0],	gsFwInfo.msAppInfo[i].marrucVersion[1]);
	}
	GLogN( "else app ver : 0001\r\n");
//	for(uint8_t i = 0; i < MAX_APP_CNT; i++)
//	  GLogN( " %s[%02d]    ver : %02d%02d\r\n", i, gsFwInfo.msReproAppInfo[i].marrucVersion[0],gsFwInfo.msReproAppInfo[i].marrucVersion[1]);
	GLogN( " total        ver : %02d%02d\r\n", gsFwInfo.marrucTotalVersion[0],			gsFwInfo.marrucTotalVersion[1]);
	GLogI( "==================================================\r\n\n" );
}

void printFirmwareInfo( void )
{
	uint32_t	i;

	GLogI( "==================================================\r\n" );
	GLogI( "===   Print Firmware Info   ======================\r\n" );
	GLogI( "==================================================\r\n" );
	GLogN( " mucStandbyMode : 0x%02x\r\n", gsFwInfo.mucStandbyMode );
	GLogN( " mucUpdated     : 0x%02x\r\n", gsFwInfo.mucUpdated );
	GLogN( " mucBootMode    : 0x%02x\r\n", gsFwInfo.mucBootMode );
	GLogN( " mucCurrentMode : 0x%02x\r\n", gsFwInfo.mucCurrentMode );

	// Model Name
	GLogN( " Model Name     : " );
	for( i = 0; i < MODEL_NAME_SIZE; i++ )
	{
		GLogN( "%c", gsFwInfo.marrucModelName[i] );
	}
	GLogN( "\r\n" );

	// Serial Number
	printSerialNumber();

//	// Bootloader Info
//	GLogN( "==================================================\r\n" );
//	GLogI( " Bootloader Version      : 0x" );
//	for( i = 0; i < VERSION_SIZE; i++ )
//	{
//		GLogI( "%02d", gsFwInfo.msBlInfo.marrucVersion[i] );
//	}
//	GLogN( "\r\n" );
//	
//	GLogN( " Bootloader Size         : 0x%08x\r\n", gsFwInfo.msBlInfo.mSize );
//	GLogN( " Bootloader CheckSum     : 0x%08x\r\n", gsFwInfo.msBlInfo.mCheckSum );

//	// Main Application Info
//	for(uint8_t i = 0; i < MAX_APP_CNT; i++)
//	{
//		GLogN( "==================================================\r\n" );
//		GLogI( " Application %02d\r\n", i );
//		GLogI( " Application Version      : 0x" );
//		GLogI( "%02d%02d", gsFwInfo.msAppInfo[i].marrucVersion[0], gsFwInfo.msAppInfo[i].marrucVersion[1] );
//		GLogN( "\r\n" );
//		
//		GLogN( " Application Size         : 0x%08x\r\n", gsFwInfo.msAppInfo[i].mSize );
//		GLogN( " Application CheckSum     : 0x%08x\r\n", gsFwInfo.msAppInfo[i].mCheckSum );
//	}
	
//	// Reprogram Application Info
//	GLogN( "==================================================\r\n" );
//	GLogI( " Reprogram Application \r\n" );
//	GLogI( " Application Version      : 0x" );
//	for( i = 0; i < VERSION_SIZE; i++ )
//	{
//		GLogI( "%02d", gsFwInfo.msReproAppInfo.marrucVersion[i] );
//	}
//	GLogN( "\r\n" );
//
//	GLogN( " Application Jump Address : 0x%08x\r\n", gsFwInfo.msReproAppInfo.mJumpAddress );
//	GLogN( " Application Size         : 0x%08x\r\n", gsFwInfo.msReproAppInfo.mSize );
//	GLogN( " Application CheckSum     : 0x%08x\r\n\n", gsFwInfo.msReproAppInfo.mCheckSum );
}

void print_FirmwareInfo( void )
{
	uint32_t i;

	GLogI( "======================   Print Firmware Info   ======================\r\n" );
	GLogN("muiPreamble : 0x%08X\r\n",gsFwInfo.muiPreamble);
	GLogN("mucInitialized : %d\r\n",gsFwInfo.mucInitialized);
	GLogN("mucStandbyMode : %d\r\n",gsFwInfo.mucStandbyMode);
	GLogN("mucBootMode : %d\r\n",gsFwInfo.mucBootMode);
	GLogN("mucUpdated : %d\r\n",gsFwInfo.mucUpdated);
	GLogN("mucBackuped : %d\r\n",gsFwInfo.mucBackuped);
	GLogN("mucChanged : %d\r\n",gsFwInfo.mucChanged);
	GLogN("mucBootUpdate : %d\r\n",gsFwInfo.mucBootUpdate);
	GLogN("mucEmmcFormat : %d\r\n",gsFwInfo.mucEmmcFormat);
	GLogN("mucModeChange : %d\r\n",gsFwInfo.mucModeChange);
	
	GLogN("mucReserved1 : ");
	for( i=0;i<sizeof(gsFwInfo.mucReserved1);i++)
	{
		GLogN("%02X ",gsFwInfo.mucReserved1[i]);
	}
	GLogN("\r\n");

	GLogN("marrucModelName : ");
	for( i=0;i<MODEL_NAME_SIZE;i++)
	{
		GLogN("%c",gsFwInfo.marrucModelName[i]);
	}
	GLogN("\r\n");

	GLogN("marrucSerialNo : ");
	for( i=0;i<SERIAL_NUMBER_SIZE;i++)
	{
		GLogN("%c",gsFwInfo.marrucSerialNo[i]);
	}
	GLogN("\r\n");

	GLogN("marrucTotalVersion : %02X.%02X\r\n",gsFwInfo.marrucTotalVersion[0],gsFwInfo.marrucTotalVersion[1]);

	GLogN("mucReserved2 : ");
	for( i=0;i<sizeof(gsFwInfo.mucReserved2);i++)
	{
		GLogN("%02X ",gsFwInfo.mucReserved2[i]);
	}
	GLogN("\r\n");

	for( i=0;i<20;i++)
	{
		GLogI("-%d-\r\n",i);
		GLogN("marrucVersion : %02X.%02X \r\n",gsFwInfo.msAppInfo[i].marrucVersion[0],gsFwInfo.msAppInfo[i].marrucVersion[1]);
		GLogN("mSize : %d\r\n",gsFwInfo.msAppInfo[i].mSize);
		GLogN("mCheckSum : 0x%08X \r\n",gsFwInfo.msAppInfo[i].mCheckSum);
		GLogN("mSectorCount : %d\r\n",gsFwInfo.msAppInfo[i].mSectorCount);
		GLogN("mReserved : \r\n");
		for( int j=0;j<sizeof(gsFwInfo.msAppInfo[i].mReserved);j++ )
		{
			GLogN("%02X ",gsFwInfo.msAppInfo[i].mReserved[j]);
		}
		GLogN("\r\n");
	}
	
	GLogN("mucReserved4 : ");
	for( i=0;i<sizeof(gsFwInfo.mucReserved2);i++)
	{
		GLogN("%02X ",gsFwInfo.mucReserved2[i]);
	}
	GLogN("\r\n");

	GLogN("mucReserved : ");
	for( i=0;i<sizeof(gsFwInfo.mucReserved2);i++)
	{
		GLogN("%02X ",gsFwInfo.mucReserved2[i]);
	}
	GLogN("\r\n");

	GLogI("mHostOldIdx : %d\r\n",gsFwInfo.msBTDeviceInfo.mHostOldIdx);
	
	GLogN("mTrigInfo.mdev_name : ");
	for( i=0;i<sizeof(gsFwInfo.msBTDeviceInfo.mTrigInfo.mdev_name);i++)
	{
		GLogN("%c",gsFwInfo.msBTDeviceInfo.mTrigInfo.mdev_name[i]);
	}
	GLogN("\r\n");

	GLogN("mTrigInfo.mdev_addr = ");
	for( i=0;i<sizeof(gsFwInfo.msBTDeviceInfo.mTrigInfo.mdev_addr);i++)
	{
		GLogN(":%02X",gsFwInfo.msBTDeviceInfo.mTrigInfo.mdev_addr[i]);
	}
	GLogN("\r\n");

	GLogN("mTrigInfo.mlinkKey = ");
	for( i=0;i<sizeof(gsFwInfo.msBTDeviceInfo.mTrigInfo.mlinkKey);i++)
	{
		GLogN("%02X",gsFwInfo.msBTDeviceInfo.mTrigInfo.mlinkKey[i]);
	}
	GLogN("\r\n");

	for( i=0;i<gsFwInfo.msBTDeviceInfo.mHostOldIdx;i++)
	{
		GLogI("mHostInfo[%d]:\r\n",i);
		GLogN("mdev_name = ");
		for( int j=0;j<sizeof(gsFwInfo.msBTDeviceInfo.mHostInfo[i].mdev_name);j++)
		{
			GLogN("%c",gsFwInfo.msBTDeviceInfo.mHostInfo[i].mdev_name[j]);
		}
		GLogN("\r\n");
		
		GLogN("mdev_addr = ");
		for( int j=0;j<sizeof(gsFwInfo.msBTDeviceInfo.mHostInfo[i].mdev_addr);j++)
		{
			GLogN(":%02X",gsFwInfo.msBTDeviceInfo.mHostInfo[i].mdev_addr[j]);
		}
		GLogN("\r\n");

		GLogN("mlinkKey = ");
		for( int j=0;j<sizeof(gsFwInfo.msBTDeviceInfo.mHostInfo[i].mlinkKey);j++)
		{
			GLogN("%02X",gsFwInfo.msBTDeviceInfo.mHostInfo[i].mlinkKey[j]);
		}
		GLogN("\r\n");
	}
}

uint32_t CalcFirmwareInfoCS( char* pcBuffer, uint32_t unSize )
{
	uint32_t unCheckSum = 0;
	for(int i=0;i<unSize;i++)
	{
		unCheckSum += pcBuffer[i];
	}
	
	return unCheckSum;
}

int32_t initFirmwareInfo( void )
{
	uint8_t		i;

	GLogN( "Start %s\r\n", __FUNCTION__ );

	// init flags
	gsFwInfo.muiPreamble		= VCI3_FWINFO_PREAMBLE;
	gsFwInfo.mucInitialized		= FIRMWARE_INITIALIZED_V3;
	gsFwInfo.mucStandbyMode		= 0;
	gsFwInfo.mucBootMode		= eApp_Downloader;
	gsFwInfo.mucCurrentMode		= eApp_Downloader;
	gsFwInfo.mucUpdated			= FALSE;
	gsFwInfo.mucBackuped		= FALSE;
	gsFwInfo.mucChanged			= TRUE;
	gsFwInfo.mucBootUpdate		= FALSE;
	gsFwInfo.mucEmmcFormat		= TRUE;
	gsFwInfo.mucModeChange		= FALSE;

	memset( gsFwInfo.mucReserved1, 0, sizeof( gsFwInfo.mucReserved1 ) );

	// init model info
	memcpy( gsFwInfo.marrucModelName,	MODEL_NAME,		MODEL_NAME_SIZE );
	memcpy( gsFwInfo.marrucSerialNo,	SERIAL_NUMBER,	SERIAL_NUMBER_SIZE );
    gsFwInfo.marrucTotalVersion[0] = 0x00;
    gsFwInfo.marrucTotalVersion[1] = 0x01;
//	memset( gsFwInfo.marrucTotalVersion,0,				sizeof( gsFwInfo.mucReserved1 ) );
//	memset( gsFwInfo.mucReserved2,		0,				sizeof( gsFwInfo.mucReserved2 ) );

	// init Application Info
	// Main Application Info
	for(i = 0; i < MAX_APP_CNT; i++)
	{
		gsFwInfo.msAppInfo[i].marrucVersion[0]	= 0;
		gsFwInfo.msAppInfo[i].marrucVersion[1]	= 1;

		gsFwInfo.msAppInfo[i].mSize		= 0xC0000;	//main app size : 666kbyte     512kbyte->768kbyte����    (0x08020000~0x080E0000 = 0xC0000 = 768K).
		gsFwInfo.msAppInfo[i].mCheckSum	= 0x00;
		gsFwInfo.msAppInfo[i].mSectorCount	= 0x06;		//512kbyte->768kbyte����   0x08020000~0x080E0000
		memset( gsFwInfo.msAppInfo[i].mReserved, 0, sizeof( gsFwInfo.msAppInfo[i].mReserved ) );
	}
	
	// Bootloader Info
	for( i = 0; i < VERSION_SIZE; i++ )
	{
		gsFwInfo.msAppInfo[eApp_bootloader].marrucVersion[i] = i;
	}
	gsFwInfo.msAppInfo[eApp_bootloader].mSize			= 0x20000;									// Max 0x20000( 128K )
	gsFwInfo.msAppInfo[eApp_bootloader].mCheckSum		= 0x00;
	gsFwInfo.msAppInfo[eApp_bootloader].mSectorCount	= 0x01;
	memset( gsFwInfo.msAppInfo[eApp_bootloader].mReserved, 0, sizeof( gsFwInfo.msAppInfo[eApp_bootloader].mReserved ) );

	// Reprogram Application Info
//	for( i = 0; i < VERSION_SIZE; i++ )
//	{
//		gsFwInfo.msReproAppInfo.marrucVersion[i]	= i;
//	}
//
//	gsFwInfo.msReproAppInfo.mJumpAddress	= FIRMWARE_REPROGRAMAPP_ADD;
//	gsFwInfo.msReproAppInfo.mSize			= 0x40000;								// Max 0x40000( 256K )
//	gsFwInfo.msReproAppInfo.mCheckSum		= 0x0a0a0a0a;
//	gsFwInfo.msReproAppInfo.mSectorCount	= 0x02;
	gsFwInfo.muiCheckSum = 0;
	gsFwInfo.muiCheckSum = CalcFirmwareInfoCS((char*)&gsFwInfo,sizeof(SFwInfo));

	//saveFirmwareInfo_EMMC();

	return 0;
}

int32_t loadFirmwareInfo_Flash( SFwInfo* psFwInfo, uint32_t flashAdd )
{
	uint32_t count		= FIRMWARE_RETRY_COUNT;
	int32_t ret			= 0;

//	GLogN( "Start %s\r\n", __FUNCTION__ );

	while( count-- )
	{
		ret = readByteFlash( flashAdd, (uint8_t*)psFwInfo, (uint32_t)sizeof( SFwInfo ) );
		if( ret )
		{
			GLogE( "fail read firmware info(%d)!!!\r\n", count );
			HAL_Delay( FIRMWARE_RETRY_DELAY );				// 10ms delay
		}
		else
		{
//			printFirmwareInfo();
			return 0;
		}
	}

	return -1;
}
int8_t Send_HSM_File_Flash( U8 *Data_Addr, int Count, int length, uint8_t firmware_type)//( SFwInfo* psFwInfo, uint32_t flashAdd )
{
	uint32_t count		= FIRMWARE_RETRY_COUNT;
	int32_t ret			= 0;

//	GLogN( "Start %s\r\n", __FUNCTION__ );

	while( count-- )
	{
		ret = readByteFlash( 0x081C0000+(Count*length), Data_Addr, (uint32_t)sizeof( SFwInfo ) );

		if( ret )
		{
			GLogE( "fail read firmware info(%d)!!!\r\n", count );
			HAL_Delay( FIRMWARE_RETRY_DELAY );				// 10ms delay
		}
		else
		{
//			printFirmwareInfo();
			return 0;
		}
	}

	return 1;
}

bool isFirmwareInfoBroken( SFwInfo	* psFwInfo )
{
	uint32_t uiCalcCheckSum;
	SFwInfo sTmpFwInfo;
	
	if( psFwInfo->muiPreamble != VCI3_FWINFO_PREAMBLE )
	{
		printf( "broken firmware preamble : %x, stored preamble: %x\r\n", VCI3_FWINFO_PREAMBLE, psFwInfo->muiPreamble );
		return true;
	}
	
	memcpy(&sTmpFwInfo, psFwInfo, sizeof(SFwInfo));
	sTmpFwInfo.muiCheckSum = 0;
	
	uiCalcCheckSum = CalcFirmwareInfoCS( (char*)&sTmpFwInfo, sizeof(SFwInfo));
	if(uiCalcCheckSum != psFwInfo->muiCheckSum)
	{//TARA, delete important info
		printf( "calc checksum : 0x%08x, stored checksum : 0x%08x\r\n", uiCalcCheckSum, psFwInfo->muiCheckSum );
	}

	if( psFwInfo->muiCheckSum == uiCalcCheckSum )
	{

		// firmware info wasn't broken
		return false;
	}

	// firmware was broken
	return true;
}

void InitializeRecoveryFirmwareInfo()
{
	SFwInfo sTmpFwInfo;
	
	printf("boot loader older version process\r\n");
	//update initialize byte for new process.
	gsFwInfo.mucInitialized = FIRMWARE_INITIALIZED_V3;
	
	//backup bluetooth / wifi info for current user
	memcpy(&sTmpFwInfo, &gsFwInfo, sizeof(SFwInfo));
	//clear bluetooth /wifi info for backup area
	memset(&gsFwInfo.msBTDeviceInfo, 0, sizeof(SBTInfo));
	memset(&gsFwInfo.msWifiConnectInfo, 0, sizeof(SWifiInfo));
	
	//update checksum firmware info
	gsFwInfo.muiCheckSum = 0;
	gsFwInfo.muiCheckSum = CalcFirmwareInfoCS((char*)&gsFwInfo,sizeof(SFwInfo));
	
	// update firmware for version 2 and backup the firmware info
	UpdateBackupFirmwareInfo(true);
	
	//restore bt / wifi info of user
	memcpy(&gsFwInfo, &sTmpFwInfo, sizeof(SFwInfo));
	
	//update firmware up
	gsFwInfo.mucChanged = true;
	saveFirmwareInfo_EMMC(true);
}

void InitializeBackUpFwInfo_Flash( void )
{
	printf("boot loader older version process\r\n");
	//update initialize byte for new process.
	gsFwInfo.mucInitialized = FIRMWARE_INITIALIZED_V3;
	
	//update checksum firmware info
	gsFwInfo.muiCheckSum = 0;
	gsFwInfo.muiCheckSum = CalcFirmwareInfoCS((char*)&gsFwInfo,sizeof(SFwInfo));
	
	//update firmware info
	gsFwInfo.mucChanged = true;
    saveFirmwareInfo_Flash(&gsFwInfo, FIRMWARE_BK_INFO_ADD);
}

bool UpdateBackupFirmwareInfo(bool bForceUpdate)
{
	SFwInfo stBkFwInfo;
	FRESULT ucFresult=FR_OK;

	printf("%s] start\r\n", __func__);
	
	f_chdir(DIR_ROOT);

	if( ReadBackupFirmwareInfo(&stBkFwInfo) == 0 )
	{
		// check backup firmware info is broken
		if( isFirmwareInfoBroken(&stBkFwInfo) == true || bForceUpdate == true )
		{
		
			InitBackupFirmwareInfo(&stBkFwInfo, &gsFwInfo);
			stBkFwInfo.mucChanged = true;
			ucFresult = WriteBackupFirmwareInfo(&stBkFwInfo);

			if( ucFresult != FR_OK)
			{
				ucFresult = WriteBackupFirmwareInfo(&stBkFwInfo);
				GLogN("WriteBackupFirmwareInfo fail:%d\r\n",ucFresult);
			}

			return true;
		}
		else
		{
			printf("%s] error isBackupFirmwareInfoBroken\r\n", __func__);		

			return false;
		}
	}
	else
	{
		GLogE("UpdateBackupFirmwareInfo : error - read backup firmware info\r\n");
	}

	return false;
}

bool InitBackupFirmwareInfo( SFwInfo	* psFwBkInfo, SFwInfo	* psFwInfo )
{
	// check backup firmware info is broken
	psFwBkInfo->muiPreamble = VCI3_FWINFO_PREAMBLE;
	psFwBkInfo->muiCheckSum = CalcFirmwareInfoCS((char*)psFwInfo,sizeof(SFwInfo));
	memcpy(psFwBkInfo, psFwInfo, sizeof(SFwInfo));

	printf("%s] backup checksum : %x\r\n", __func__, psFwBkInfo->muiCheckSum);

	return true;
}

FRESULT WriteBackupFirmwareInfo( SFwInfo	* psFwBkInfo )
{
	FIL 		Filepnt;
	uint32_t 	uiWriteNum;
	FRESULT		ret=FR_OK;
	
	f_chdir(DIR_ROOT);
	
	if( psFwBkInfo->mucChanged )
	{
	  	psFwBkInfo->mucChanged = false;
	  	
	  	psFwBkInfo->muiCheckSum = 0;
		psFwBkInfo->muiCheckSum = CalcFirmwareInfoCS((char*)psFwBkInfo,sizeof(SFwInfo));
		
		ret = f_open(&Filepnt, FILENAME_FW_INFO_BK_INI, FA_OPEN_EXISTING | FA_WRITE);
		GLogN("f_open %d ", ret);
		ret = f_truncate(&Filepnt);
		GLogN("f_truncate %d ", ret);
		ret = f_write(&Filepnt, psFwBkInfo, sizeof(SFwInfo), &uiWriteNum);
		GLogN("f_write %d ", ret);
		ret = f_close(&Filepnt);
		GLogN("f_close %d ", ret);
	}
	
	return ret;
}

uint32_t ReadBackupFirmwareInfo( SFwInfo	* psFwBkInfo )
{
  	FIL 				Filepnt;
	uint32_t 	uiReadNum;
	char 				ret;
	
	GLogN( "Start %s\r\n", __FUNCTION__ );
	
	f_chdir(DIR_ROOT);
  
  	ret = f_open(&Filepnt, FILENAME_FW_INFO_BK_INI, FA_OPEN_EXISTING | FA_READ);
	if(ret != FR_OK)
	{
	  	return -1;
	}
	GLogN("f_open %d ", ret);
	
	ret = f_read(&Filepnt, psFwBkInfo, sizeof(SFwInfo), &uiReadNum);
	if(ret != FR_OK)
	{
	  	return -1;
	}
	GLogN("f_read %d \r\n", ret);
	
	f_close(&Filepnt);
	return 0;
}


uint32_t RecoveryFirmwareInfo(SFwInfo	* psFwInfo)
{
	uint32_t count		= 5;
	uint32_t ret		= 0;

	SFwInfo stBkFwInfo;

	printf("%s] start\r\n", __func__);

	while( count-- )
	{
        if( count % 2 == 1 )
        {
            ret = loadFirmwareInfo_Flash(&stBkFwInfo, FIRMWARE_BK_INFO_ADD);
        }
        else
        {
            ret = ReadBackupFirmwareInfo(&stBkFwInfo);
        }
            
		if( ret  == 0 )
		{
			// add check routine for firmware info validation
			ret = !isFirmwareInfoBroken(&stBkFwInfo);
			if( ret == true)
			{
				// update firmware info from backup area
				memcpy((char*)psFwInfo, (char*)&stBkFwInfo, sizeof(SFwInfo));

				gsFwInfo.mucChanged = true;
				// update new firmware info to firmware info area
				saveFirmwareInfo_EMMC(false);

				GLogE("firmware info recovery success\r\n");
                break;
			}
			else
			{
				// do nothing
				// recovery fail
				GLogE("firmware info recovery fail\r\n");
			}
		}
		else
		{
			GLogE( "fail read backup firmware info(%d)!!!\r\n", count );
			HAL_Delay( FIRMWARE_RETRY_DELAY );				// 10ms delay
		}
	}

	return ret;
}

void saveFirmwareInfo_Flash( SFwInfo* psFwInfo, uint32_t flashAdd )
{
	uint32_t count		= FIRMWARE_RETRY_COUNT;
	uint32_t ret		= 0;

	GLogN( "++ %s\r\n", __FUNCTION__ );

	if( psFwInfo->mucChanged )
	{
		HAL_FLASH_Unlock();

		gsFwInfo.mucChanged = FALSE;

		while( count-- )
		{
			if( eraseFlash( flashAdd, 1 ) != FLASH_OK )
			{
				GLogE( "error... eraseFlash(%d)\r\n", count );
				continue;
			}

			ret = writeByteFlash( flashAdd, (uint8_t*)psFwInfo, (uint32_t)sizeof( SFwInfo ) );
			if( ret )
			{
				GLogE( "Fail save firmware information(%d)!!!\r\n", count );
				HAL_Delay( FIRMWARE_RETRY_DELAY );				// 10ms delay
			}
			else
			{
				break;
			}
		}

		HAL_FLASH_Lock();
	}
	else
	{
		GLogI( "%s : No Changed!!!\r\n", __FUNCTION__ );
	}

	GLogN( "-- %s\r\n", __FUNCTION__ );
}


void saveFirmwareInfo_EMMC( bool bBkUpdate )
{
  	FIL 	Filepnt;
	uint32_t uiWriteNum;
	char 	ret;
	
	f_chdir(DIR_ROOT);
	
	if( gsFwInfo.mucChanged )
	{
	  	gsFwInfo.mucChanged = false;
	  	
	  	gsFwInfo.muiCheckSum = 0;
		gsFwInfo.muiCheckSum = CalcFirmwareInfoCS((char*)&gsFwInfo,sizeof(SFwInfo));
		
		ret = f_open(&Filepnt, FILENAME_FW_INFO_INI, FA_OPEN_ALWAYS | FA_WRITE);
		GLogN("f_open %d ", ret);
		ret = f_truncate(&Filepnt);
		GLogN("f_truncate %d ", ret);
		ret = f_write(&Filepnt, (void*)&gsFwInfo, sizeof(SFwInfo), &uiWriteNum);
		GLogN("f_write %d ", ret);
		ret = f_close(&Filepnt);
		GLogN("f_close %d ", ret);
		
		//UpdateBackupFirmwareInfo(true);
        if( bBkUpdate == true )
        {
            gsFwInfo.mucChanged = true;
            saveFirmwareInfo_Flash(&gsFwInfo, FIRMWARE_BK_INFO_ADD);
        }
	}
}

int32_t loadFirmwareInfo_EMMC( void )
{
  	FIL 				Filepnt;
	volatile uint16_t 	uiReadNum;
	char 				ret;
	
	f_chdir(DIR_ROOT);
  
  	ret = f_open(&Filepnt, FILENAME_FW_INFO_INI, FA_OPEN_EXISTING | FA_READ);
    //ret = f_open(&Filepnt, FILENAME_FW_INFO_INI, FA_CREATE_ALWAYS);
	if(ret != FR_OK)
	{
		formatEmmc();
	  	f_close(&Filepnt);
	  	return -1;
	}
	else
	{
	  	ret = f_read(&Filepnt, (void *)&gsFwInfo, sizeof(SFwInfo), (void *)&uiReadNum);
		if(ret != FR_OK)
		{
			f_close(&Filepnt);
			return -1;
		}
	}
	f_close(&Filepnt);
	return 0;
}
int32_t load_S_ReproStart_EMMC( void )
{
  	FIL 				Filepnt;
	volatile uint16_t 	uiReadNum;
	char 				ret;
	
	f_chdir(DIR_ROOT);
  
  	ret = f_open(&Filepnt, S_REPRO_JUMP_FILE_NAME, FA_OPEN_EXISTING | FA_READ);
	if(ret != FR_OK)
	{
	  	f_close(&Filepnt);
	  	return FALSE;
	}
	f_close(&Filepnt);
	return TRUE;
}

/*----------------------------------------------------------------------
     Firmware Application
----------------------------------------------------------------------*/
uint32_t checkCS( uint32_t flashAdd, SAppInfo *appInfo )
{
#ifdef USE_APPLICATION_CHECKCS
	//uint32_t flashAdd	= FIRMWARE_MAINAPP_ADD;
	uint32_t calcCS		= 0;

	uint32_t count		= FIRMWARE_RETRY_COUNT;
	uint32_t i;

//	GLogN( "Start %s, target : 0x%08x\r\n", __FUNCTION__, flashAdd );

	while( count-- )
	{
		for( i = 0; ( i < appInfo->mSize ) && ( flashAdd <= ( USER_FLASH_LAST_PAGE_ADDRESS - 1 ) ); i++ )
		{
			calcCS += (uint64_t)( *(uint8_t*)(flashAdd + i) );						// read & calculate
		}

		if( appInfo->mCheckSum == (uint16_t)calcCS )
		{
			return 0;
		}
		else
		{
			GLogE( "calcCS : 0x%08x, savedCS : 0x%08x(%d) \r\n", calcCS, appInfo->mCheckSum, count );

			flashAdd 	= FIRMWARE_MAINAPP_ADD;
			calcCS		= 0;

			HAL_Delay( FIRMWARE_RETRY_DELAY );
		}
	}

	return 1;
#else	// USE_APPLICATION_CHECKCS
	//GLogN( "Disable Check CS( 0x%08x )\r\n", FIRMWARE_MAINAPP_ADD );//TARA, delete important info
	GLogN( "Disable Check CS\r\n");

	return 0;
#endif	// USE_APPLICATION_CHECKCS
}

uint32_t makeCS( uint32_t address, uint32_t size )
{
	uint32_t	i;
	uint32_t	cs	= 0;

	for( i = 0; ( i < size ) && ( address <= ( USER_FLASH_LAST_PAGE_ADDRESS - 1 ) ); i++ )
	{
		cs = cs + *(uint8_t*)address;

		address += 1;
		__DSB();
	}

	return cs;
}

/* Main Application Backup */
#if( MAIN_BACKUP_TO_FLASH )
uint32_t copyApplication( SAppInfo *source, SAppInfo *target )
{
	uint32_t count		= FIRMWARE_RETRY_COUNT;
	uint32_t ret		= 0;

//	GLogN( "Start %s\r\n", __FUNCTION__ );

	HAL_FLASH_Unlock();

	while( count-- )
	{
		// erase destination sector
		ret = eraseFlash( FIRMWARE_MAINAPP_ADD, source->mSectorCount );
		if( ret )
		{
			GLogE( "fail eraseFlash(%d)!!!\r\n", count );
			HAL_Delay( FIRMWARE_RETRY_DELAY );
			continue;
		}

		ret = writeFlash( FIRMWARE_MAINAPP_ADD, target->mJumpAddress, source->mSize );
		if( ret )
		{
			GLogE( "Fail copy flash(%d)!!!\r\n", count );
			HAL_Delay( FIRMWARE_RETRY_DELAY );
		}
		else
		{
			memcpy( target->marrucVersion, source->marrucVersion, VERSION_SIZE );		// Application Version
			target->mSize			= source->mSize;
			target->mCheckSum		= source->mCheckSum;
			target->mSectorCount	= source->mSectorCount;

			ret = checkCS( target );
			if( ret )
			{
				GLogE( "Fail CheckSum(%d)!!!\r\n", count );
				HAL_Delay( FIRMWARE_RETRY_DELAY );
			}
			else
			{
				GLogI( "Success copy application!!!\r\n" );
				HAL_FLASH_Lock();

				return 0;
			}
		}
	}

	HAL_FLASH_Lock();

	return 1;
}

void backupApplication( void )
{

	uint32_t ret = 0;

	GLogN( "Start Backup Application!!!\r\n" );
	ret = copyApplication( &gsFwInfo.msAppInfo, &gsFwInfo.msBackupAppInfo );
	if( ret )
	{
		GLogE( "Fail backup!!!\r\n" );
		gsFwInfo.mucBackuped = FALSE;
	}
	else
	{
		gsFwInfo.mucBackuped = TRUE;
	}

	gsFwInfo.mucChanged = TRUE;
}
#else
void backupApplication( void )
{
	TCHAR	backupFile[30] = { "\0", };

	GLogN( "Start Backup Application!!!\r\n" );

	sprintf( backupFile, "%s%s", USERPath, MAIN_BACKUP_FILE_NAME );
	if( DownloadFilefromFlash( backupFile, gsFwInfo.msAppInfo ) == 0 )
	{
		gsFwInfo.mucBackuped	= TRUE;
		gsFwInfo.mucChanged		= TRUE;
	}
	else
	{
		GLogEE( "Fail... Main Application Backup!!!\r\n" );
	}
}
#endif	// MAIN_BACKUP_TO_FLASH

uint8_t RestoreApplication( TCHAR *path, uint32_t fsize, uint16_t fcs )		// file size, file checksum
{
	uint16_t	cs		= 0;
	uint32_t	size	= 0;
	uint32_t	count	= 0;
	
	uint32_t	taget_Add = 0;

	// Copy Emmc => Temporary Flash
	for(count=0; count<FIRMWARE_RETRY_COUNT; count++)
	{
		if( gsFwInfo.mucBootMode == eApp_bootloader)
			taget_Add = FIRMWARE_BOOTLOADER_ADD;
		else
			taget_Add = FIRMWARE_MAINAPP_ADD;
		
		if( UploadFlashfromFile( path, taget_Add, &size, &cs, ( fsize / 131072 ) + 1, NULL ) == 0 )
		{
			if( fsize == size )
			{
				if( fcs == cs )
				{
				  	GLogI( "UploadFlashfromFile Success(%s)!!!\r\n", path );
					break;
				}
			}
		}
		else
		{
			GLogEE( "Fail... UploadFlashfromFile!!!\r\n" );
			HAL_Delay( FIRMWARE_RETRY_DELAY );
		}
	}


	if( count == FIRMWARE_RETRY_COUNT )			return 1;
	else										return 0;
}

uint8_t RestoreRecovery( TCHAR *path, uint32_t fsize, uint16_t fcs, SAppInfo* stFWAppInfo )		// file size, file checksum
{
	uint16_t	cs		    = 0;
	uint32_t	size	    = 0;
	uint32_t	count       = 0;
    uint32_t    flashAdd    = 0;
    
    flashAdd = FIRMWARE_RECOVERY_ADD;

	// Copy Emmc => Temporary Flash
	for(count=0; count<FIRMWARE_RETRY_COUNT; count++)
	{
		if( UploadFlashfromFile( path, flashAdd, &size, &cs, ( fsize / 131072 ) + 1, stFWAppInfo ) == 0 )
		{
			if( fsize == size )
			{
				if( fcs == cs )
				{
                    break;
				}
				else
				{
					GLogEE( "Fail... Checksum is diffrent!!!\r\n" );
					HAL_Delay( FIRMWARE_RETRY_DELAY );
				}
			}
			else
			{
				GLogEE( "Fail... Size is diffrent!!!\r\n" );
				HAL_Delay( FIRMWARE_RETRY_DELAY );
			}
		}
		else
		{
			GLogEE( "Fail... UploadFlashfromFile!!!\r\n" );
			HAL_Delay( FIRMWARE_RETRY_DELAY );
		}
	}


	if( count == FIRMWARE_RETRY_COUNT )			return 1;
	else										return 0;
}

void SaveFwVersionToArray( char* str, uint8_t* Arr )
{
  	char		*ver = 0;
	uint8_t		ver_cnt = 0;
	char		*ver_ptr;
	
	ver = strtok_r( str, ".\r\n", &ver_ptr );
	do
	{
		Arr[ver_cnt] = atoi(ver);
		GLogN( "%d ", Arr[ver_cnt]);
		ver_cnt++;
		ver = strtok_r( NULL, ".\r\n", &ver_ptr );
	}while(ver != NULL);
	GLogN( "\r\n");
	ver_cnt = 0;
}

uint8_t UpdateFWVersion( void )
{
	TCHAR		infoPath[30]	= { "\0", };
	uint8_t		AppNo			= 0;
	uint32_t	ret;

	char		*ptr;
	char		*token[TOKEN_NUMBER];
	uint8_t		ucTemp[80];
	
	// 1. Check Application Info
	sprintf( infoPath, "%s/%s", EMMC_APPLICATION_FOLDER, APPLICATION_INFO_FILE_NAME );
	ret = f_open( &USERFile, infoPath, FA_OPEN_EXISTING | FA_READ );
	if( ret == FR_OK )
	{
		// check file size
		if( f_size( &USERFile ) == 0 )
		{
			f_close( &USERFile );
			return 1;
		}

		while( f_gets( (char*)ucTemp, 80, &USERFile ) != NULL )
		{
			token[0] = strtok_r( (char*)ucTemp, TOKEN_SEPARATORS, &ptr );		// Application Number
			token[1] = strtok_r( NULL, TOKEN_SEPARATORS, &ptr);					// Application File name
			token[2] = strtok_r( NULL, TOKEN_SEPARATORS, &ptr);					// Application Version

			GLogN( "%2s %-20s %4s\r\n", token[0], token[1], token[2]);
			AppNo = (uint8_t)atoi( token[0] );
			
			//GLogN( "%d ", AppNo);
			//GLogN( "%-20s ", token[1]);
			
			if( !strncmp( token[0], "FF", strlen("FF") ))
			{
				SaveFwVersionToArray( token[2], gsFwInfo.marrucTotalVersion );
			}
			else
			{					
				SaveFwVersionToArray( token[2], gsFwInfo.msAppInfo[AppNo].marrucVersion );
			}
				
		}
		f_close( &USERFile );
	}
	else
	{
		GLogEE( "Fail... Read Application Info File(%s)\r\n", infoPath );
		f_close( &USERFile );
		return 2;
	}

	return 0;
}

eResVerifiyApp checkApplication( void )
{
	TCHAR		infoPath[30]	= { "\0", };
	TCHAR		FWPath[30]		= { "\0", };

	uint32_t	cs				= 0;
	uint32_t	size			= 0;
	uint8_t		flagUpload		= 0;
	uint8_t		version[2];

	uint32_t	ret;

	char		*ptr;
	char		*token[TOKEN_NUMBER];
	uint8_t		ucTemp[80];
	uint8_t		AppNo;

#if 0//defined(FEATURE_DETERMINE_ECU_TO_UPGRADE) 
	// Load Firmware Information
	ret = load_S_ReproStart_EMMC();
	if( ret==TRUE)
	{
		GLogI( "StandAloneReproStart.ini exist!!!\r\n" );
		AppNo = eApp_ECUUP_COMMON;
		gsFwInfo.mucBootMode = eApp_ECUUP_COMMON;
		//flagUpload = 1;
	
	}
	else
	{
		GLogI( "StandAloneReproStart.ini not exist!!!\r\n" );
	}
#endif
	GLogI( "mucBootMode: %d\r\n", gsFwInfo.mucBootMode);
	GLogI( "mucCurrentMode: %d\r\n", gsFwInfo.mucCurrentMode);
	
	if( gsFwInfo.mucBootMode != gsFwInfo.mucCurrentMode )			flagUpload = 1;

	////////////////////////////////////[GIMSRM-24869] abnormal mode exception///////////////////////////////////////////////
	if( //gsFwInfo.mucBootMode == eApp_Downloader					//eApp_Downloader			= 0,
    	gsFwInfo.mucBootMode == eApp_VCI_2       			//eApp_VCI_2				= 1,
		||gsFwInfo.mucBootMode == eApp_ECUUpCAN      		//eApp_ECUUpCAN			= 2,
		||gsFwInfo.mucBootMode == eApp_ECUUpKWP    			//eApp_ECUUpKWP			= 3,
  		//||gsFwInfo.mucBootMode == eApp_ECUUpCV    			//eApp_ECUUpCV			= 4,
		||gsFwInfo.mucBootMode == eApp_Inside      			//eApp_Inside				= 5,	//record
		//||gsFwInfo.mucBootMode == eApp_TMB      				//eApp_TMB				= 6,
		//||gsFwInfo.mucBootMode == eApp_TMD      				//eApp_TMD				= 7,
		//||gsFwInfo.mucBootMode == eApp_TMTM      				//eApp_TMTM				= 8,
		//||gsFwInfo.mucBootMode == eApp_Inside2      		//eApp_Inside2			= 9,
		//||gsFwInfo.mucBootMode == eApp_bootloader      	//eApp_bootloader			= 10,
		//||gsFwInfo.mucBootMode == eApp_Recovery      		//eApp_Recovery			= 11,
		//||gsFwInfo.mucBootMode == eApp_Selftest      		//eApp_Selftest			= 12,
		||gsFwInfo.mucBootMode == eApp_ECUUpCCP      		//eApp_ECUUpCCP			= 13,
		//||gsFwInfo.mucBootMode == eApp_ECUUpFlexRay     //eApp_ECUUpFlexRay		= 14,
		||gsFwInfo.mucBootMode == eApp_ECUUpDownloader  //eApp_ECUUpDownloader	= 16,
		//||gsFwInfo.mucBootMode == eApp_ECUUpCVKWP      	//eApp_ECUUpCVKWP			= 18,
		||gsFwInfo.mucBootMode == eApp_VCI_II_PDI      	//eApp_VCI_II_PDI			= 19,
		//||gsFwInfo.mucBootMode == eApp_HSM_UPDATE1      ////eApp_HSM_UPDATE1		= 20,//for hsm update//HSM_Applet_V10.bin
		//||gsFwInfo.mucBootMode == eApp_HSM_UPDATE2      ////eApp_HSM_UPDATE2		= 21,//for hsm update//HSM_Applet_V20.bin
		||gsFwInfo.mucBootMode == eApp_ECUUP_COMMON     //eApp_ECUUP_COMMON		= 22,//for stand alone reprogram (RDBI)
		||gsFwInfo.mucBootMode == eApp_ECUUP_STDA      	//eApp_ECUUP_STDA			= 23,//for stand alone reprogram
		//||gsFwInfo.mucBootMode == eApp_CFCI101_BOOT     ////eApp_CFCI101_BOOT		= 26,//for CFCI-101 update
		//||gsFwInfo.mucBootMode == eApp_CFCI101_APP      ////eApp_CFCI101_APP		= 27,//for CFCI-101 update
		//||gsFwInfo.mucBootMode == eApp_CFCI101_SELF     ////eApp_CFCI101_SELF		= 28,//for CFCI-101 update
		//||gsFwInfo.mucBootMode == eApp_HSM_UPDATE3      ////eApp_HSM_UPDATE3		= 29,//for hsm update//ASK_Master_App.bin
		//||gsFwInfo.mucBootMode == eApp_HSM_UPDATE4      ////eApp_HSM_UPDATE4		= 30,//for hsm update//HSM_mainApplet.bin
		//||gsFwInfo.mucBootMode == eApp_MAX      				////eApp_MAX				= 24
		)
	{
		AppNo = gsFwInfo.mucBootMode;
	}
    else if( gsFwInfo.mucBootMode == eApp_Recovery )
    {
        return eBootRecovery;
    }
	else	//abnormal mode
	{
		AppNo = eApp_VCI_2;
        if( gsFwInfo.mucBootMode != eApp_Downloader )
        {
            gsFwInfo.mucBootMode = eApp_VCI_2;
        }
	}

	//jkc directory_list();
	
	// 1. Check Application Info
	sprintf( infoPath, "%s/%s", EMMC_APPLICATION_FOLDER, APPLICATION_INFO_FILE_NAME );
	ret = f_open( &USERFile, infoPath, FA_OPEN_EXISTING | FA_READ );
	if( ret == FR_OK )
	{
		// check file size
		if( f_size( &USERFile ) == 0 )
		{
			f_close( &USERFile );
			return eInvalidListSize;
		}

		while( f_gets( (char*)ucTemp, 80, &USERFile ) != NULL )
		{
            if( (ucTemp[0] > 0x39 || ucTemp[0] < 0x30) && (ucTemp[0] != 0x49) )
            {
                f_close( &USERFile );
                return eListNumError;
            }
			token[0] = strtok_r( (char*)ucTemp, TOKEN_SEPARATORS, &ptr );		// Application Number
			token[1] = strtok_r( NULL, TOKEN_SEPARATORS, &ptr);					// Application File name
			token[2] = strtok_r( NULL, TOKEN_SEPARATORS, &ptr);					// Application Version
			//token[3] = strtok_r( NULL, TOKEN_SEPARATORS, &ptr);					// Application Size
			//token[4] = strtok_r( NULL, TOKEN_SEPARATORS, &ptr);					// Application CheckSum

			//GLogN( "%2s %-20s %4s\r\n", token[0], token[1], token[2]);//, token[3], token[4] );
			//GLogN( "%2s %4s\r\n", token[0], token[2]);//, token[3], token[4] );//TARA, delete important info

			if( AppNo == (uint32_t)atoi( token[0] ) )
			{
				size	= 	gsFwInfo.msAppInfo[AppNo].mSize; //(uint32_t)atoi( token[3] );
				cs		= 	gsFwInfo.msAppInfo[AppNo].mCheckSum;//(uint32_t)atoi( token[4] );
			
				SaveFwVersionToArray( token[2], version );
				f_close( &USERFile );
				//dlcToVersion( version, (uint32_t)atoi( token[2] ) );

				if( flagUpload )
				{
					sprintf( FWPath, "%s/%s", EMMC_APPLICATION_FOLDER, token[1] );
					if( RestoreApplication( FWPath, size, cs ) == 0 )
					{
						gsFwInfo.msAppInfo[AppNo].marrucVersion[0]	= version[0];
						gsFwInfo.msAppInfo[AppNo].marrucVersion[1]	= version[1];
					}
                    else
                    {
                        
                        return eRestoreAppFail;
                    }
				}
				else
				{
					if( isDifferentVersion( version, gsFwInfo.msAppInfo[AppNo].marrucVersion ) )
					{
						sprintf( FWPath, "%s/%s", EMMC_APPLICATION_FOLDER, token[1] );
						if( RestoreApplication( FWPath, size, cs ) == 0 )
						{
							gsFwInfo.msAppInfo[AppNo].marrucVersion[0]	= version[0];
							gsFwInfo.msAppInfo[AppNo].marrucVersion[1]	= version[1];
						}
                        else
                        {
                            return eRestoreAppFail;
                        }
					}
				}
			}
		}

		f_close( &USERFile );
	}
	else
	{
		GLogEE( "Fail... Read Application Info File(%s)\r\n", infoPath );
		f_close( &USERFile );
		return eListOpenFail;
	}

	return eAppOK;
}

eResVerifiyApp verifyApplication( void )
{
    eResVerifiyApp ret = eAppOK;
    uint8_t i = 0;
    
    for( i=0; i < FIRMWARE_RETRY_COUNT; i++ )
	{
        ret = checkApplication();
        
        if (ret == eAppOK || ret == eRestoreAppFail)
        {
            break;
        }
    }
    
    return ret;
}

uint8_t isDifferentVersion( uint8_t *compare1, uint8_t *compare2 )
{
	if( ( compare1[0] != compare2[0] ) || ( compare1[1] != compare2[1] ) )
	{
		return 1;
	}

	return 0;
}

void dlcToVersion( uint8_t *version, int32_t dlc )
{
	version[0] = dlc / 100;
	version[1] = dlc % 100;
}

#ifdef VCI3_RECORD
void RunModeSwitch(char cAppNubmer)
{
  	GLogI( ">>> Start %s : %d\r\n", __FUNCTION__, cAppNubmer);
	
	if(cAppNubmer == GetCurFwServiceMode())
	  return;
#if 0	
  	if ( cAppNubmer == eApp_Downloader 	|| 
//		 cAppNubmer == eApp_Inside  	||
		 cAppNubmer == eApp_Recovery 	)
	  
	{
//		if(eApp_Inside != GetCurFwServiceMode())
//		{
//			// clear recording wakeup info
//			ClearFwWakeupInfo();
//
//			// clear all data for restore configuration file
//			ClearFlightRecording();
//			ClearTriggerHandler();
//		}
	  	gsFwInfo.mucCurrentMode = (AppName)cAppNubmer;
		gsFwInfo.mucBootMode 	= (AppName)cAppNubmer;
		gsFwInfo.mucChanged 	= TRUE;
		saveFirmwareInfo_EMMC(false);

//		if( gsFwInfo.mucBootMode == eApp_Inside )
//		{
//			Buzzer_Control( eBUZZER_SIRESOL, MSEC(200), MSEC(200), 3 );
//		}
//		else if( gsFwInfo.mucBootMode == eApp_Downloader )
		{
		  	Buzzer_Control( eBUZZER_DOMISOL, MSEC(200), MSEC(200), 3 );
		}

		LED_ALL_OFF;
		LED_SetState(eLED_NORMAL, 0, 0);
		
		return;
	}
	else if( cAppNubmer == eApp_VCI_2 )
	{
	  	if( gsFwInfo.mucCurrentMode == eApp_bootloader	||
//		   	gsFwInfo.mucCurrentMode == eApp_Inside  	||
		 	gsFwInfo.mucCurrentMode == eApp_Recovery 	)
		{
//			if(eApp_Inside == GetCurFwServiceMode())
//			{
//				// clear recording wakeup info
//				ClearFwWakeupInfo();
//				
//				// clear all data for restore configuration file
//				ClearFlightRecording();
//				ClearTriggerHandler();
//			}
			gsFwInfo.mucCurrentMode = (AppName)cAppNubmer;
			gsFwInfo.mucBootMode 	= (AppName)cAppNubmer;
			gsFwInfo.mucChanged = TRUE;
			saveFirmwareInfo_EMMC(false);
			Buzzer_Control( eBUZZER_DOMISOL, MSEC(200),MSEC(200), 3 );

			LED_ALL_OFF;
			LED_SetState(eLED_NORMAL, 0, 0);
			
			return;
		}
	}
#endif
	gsFwInfo.mucModeChange = TRUE;
	gsFwInfo.mucChanged = TRUE;
	if( cAppNubmer < eApp_MAX )	gsFwInfo.mucBootMode = (AppName)cAppNubmer;
	else						gsFwInfo.mucBootMode = (AppName)eApp_VCI_2;
	
	saveFirmwareInfo_EMMC(false);
	
	HAL_NVIC_SystemReset();
}
#else
void RunModeSwitch(char cAppNubmer)
{
  	GLogI( ">>> Start %s : %d\r\n", __FUNCTION__, cAppNubmer);
	
	if(cAppNubmer == GetCurFwServiceMode())
	  return;
	
  	if ( cAppNubmer == eApp_Downloader 	|| 
		 cAppNubmer == eApp_Recovery)
	  
	{
	  	gsFwInfo.mucCurrentMode = (AppName)cAppNubmer;
		gsFwInfo.mucBootMode 	= (AppName)cAppNubmer;
		gsFwInfo.mucChanged 	= TRUE;
		saveFirmwareInfo_EMMC(false);
		
		Buzzer_Control( eBUZZER_DOMISOL, MSEC(200), MSEC(200), 3 );

		LED_ALL_OFF;
		LED_SetState(eLED_NORMAL, 0, 0);
		
		return;
	}
	else if( cAppNubmer == eApp_VCI_2 )
	{
	  	if( gsFwInfo.mucCurrentMode == eApp_bootloader	||
		 	gsFwInfo.mucCurrentMode == eApp_Recovery 	)
		{
			gsFwInfo.mucCurrentMode = (AppName)cAppNubmer;
			gsFwInfo.mucBootMode 	= (AppName)cAppNubmer;
			gsFwInfo.mucChanged		= TRUE;
			saveFirmwareInfo_EMMC(false);
			Buzzer_Control( eBUZZER_DOMISOL, MSEC(200),MSEC(200), 3 );

			LED_ALL_OFF;
			LED_SetState(eLED_NORMAL, 0, 0);
			
			return;
		}
	}
	gsFwInfo.mucModeChange = FALSE;
	gsFwInfo.mucChanged = TRUE;
	if( cAppNubmer < eApp_MAX )	gsFwInfo.mucBootMode = (AppName)cAppNubmer;
	else						gsFwInfo.mucBootMode = (AppName)eApp_VCI_2;
    
	if( gsFwInfo.mucUpdated == true )
    {
        if((gsFwInfo.marrucTotalVersion[0]==0) && (gsFwInfo.marrucTotalVersion[1] < 93))
            gsFwInfo.mucInitialized = FIRMWARE_INITIALIZED_V2;
        gsFwInfo.mucUpdated = false;
        saveFirmwareInfo_EMMC(true);
    }
    else
    {
        saveFirmwareInfo_EMMC(false);
    }
	
	HAL_NVIC_SystemReset();
}
#endif

char GetCurFwServiceMode()
{
	return gsFwInfo.mucCurrentMode;
}


uint8_t VciChangeMode(uint8_t mode)
{
	if (mode == 1)
	{
	  	RunModeSwitch(1);
	}
	else if (mode == 2)
	{
//		IsRecordRunning = 1; 
//		FRMode  = FR_ReadConfig; 
//		StartFRMode();
//		RES0xC051_4();
		RunModeSwitch(5);		//inside.bin ���ڵ����� ��� ����
		//VCI2 is reset when changing modes, but VCI3 is not.
		g_bAckflag=0;
		clearRXCanMessage();	//CAN		
	}
	else if (mode == 3)
	{
//		IsRecordRunning = 1; 
//		FRMode  = FR_ReadConfig; 
//		StartFRMode();
//		RES0xC051_4();
		RunModeSwitch(9);		//inside2.bin ���ڵ����� ��� ����
	}
	else if (mode == 4)
	{
//		IsRecordRunning = 1; 
//		FRMode  = FR_ReadConfig; 
//		StartFRMode();
		RunModeSwitch(19);		//VCI_II_PDI.bin ���ڵ����� ��� ����
	}
	else
	{
//		IsRecordRunning = 0; 
//		CurMode = NO_OPP;
//		StopFRMode();
	}
	return 0;
}

void WriteBackupSRAM(uint16_t write_address, uint8_t write_data)
{
    if(write_address > 0xFFF) return;

    *(__IO uint32_t*)(BAKUPSRAM_ADDRESS + write_address) = write_data;
    SCB_CleanDCache_by_Addr((uint32_t *)(BAKUPSRAM_ADDRESS + write_address), 8);
}

void ReadBackupSRAM(uint16_t read_address, uint8_t* read_data)
{
    if(read_address > 0xFFF) return;

    *read_data = *(__IO uint32_t*)(BAKUPSRAM_ADDRESS + read_address);
}

void BackupSRAM_Init(void)
{
    /*DBP : Enable access to Backup domain */
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BKPRAM_CLK_ENABLE();

    /*BRE : Enable backup regulator
      BRR : Wait for backup regulator to stabilize */
    HAL_PWREx_EnableBkUpReg();
}

void BackupSRAM_Deinit(void)
{
    /*BRE : Enable backup regulator
      BRR : Wait for backup regulator to stabilize */
    HAL_PWREx_DisableBkUpReg();

    /*DBP : Disable access to Backup domain */
    HAL_PWR_DisableBkUpAccess();
    __HAL_RCC_BKPRAM_CLK_DISABLE();
}

void RecoverFirmware(void)
{
    uint8_t count = 0;
    
    // EMMC Init.
//    GLogI( "EMMC Format...\r\n" );
//    formatEmmc();
    InitEmmcFolder();
    
    // FW List Init.
    InitFwListFile();
    
    // FW Info Init.
    memset( &gsFwInfo, 0x00, sizeof( SFwInfo ) );
//  gsFwInfo.mucEmmcFormat	= TRUE;
    initFirmwareInfo();
    gsFwInfo.mucBootMode = eApp_Recovery;

    // CDP recovery default version = 50.01 (5001): low baseline within the CDP version range
    // so the server always treats the recovery image as out-of-date and pushes the latest CDP firmware.
    {
        uint8_t v;
        gsFwInfo.marrucTotalVersion[0] = 50;
        gsFwInfo.marrucTotalVersion[1] = 1;
        for( v = 0; v < MAX_APP_CNT; v++ )     // covers all apps incl. eApp_bootloader(10)
        {
            gsFwInfo.msAppInfo[v].marrucVersion[0] = 50;
            gsFwInfo.msAppInfo[v].marrucVersion[1] = 1;
        }
    }

    // Recover Serial
    RecoverSerial();
    
    // Save Firmware Info.
	gsFwInfo.mucChanged = TRUE;
    saveFirmwareInfo_EMMC(true);
    
    HAL_FLASH_Unlock();
    
    
    // Copy Recovery FW
    for(count=0; count<FIRMWARE_RETRY_COUNT; count++)
	{
        // erase destination sector
        if( eraseFlash( FIRMWARE_MAINAPP_ADD, gsFwInfo.msAppInfo[eApp_VCI_2].mSectorCount ) != FLASH_OK )
        {
            GLogE( "fail eraseFlash(%d)!!!\r\n", count );
            HAL_FLASH_Lock();
            break;
        }

        // copy Recovery => Main Application Area
        if( copyFlash( FIRMWARE_RECOVERY_ADD+FIRMWARE_RECOVERY_INFO_OFFSET, FIRMWARE_MAINAPP_ADD, gsFwInfo.msAppInfo[eApp_Recovery].mSize ) == FLASH_OK )
        {
            HAL_FLASH_Lock();
            break;
        }
    }
    
    HAL_FLASH_Lock();
  
}

bool checkRecoveryFirmware( void )
{
    uint16_t    uiNewVer    = 0;
    uint16_t    uiOldVer    = 0;
    uint32_t    flashAdd	= FIRMWARE_RECOVERY_ADD;
	uint32_t    count		= FIRMWARE_RETRY_COUNT;
    int32_t     ret			= 0;
    TCHAR		FWPath[30]	= { "\0", };
    SAppInfo    stTempRecoveryFWInfo;
    uint32_t    ulRecoveryPreamble = 0;
    
    
    // Read information of recovery firmware
    memset(&stTempRecoveryFWInfo, 0x00, sizeof(SAppInfo));
                  
    while( count-- )
	{
		ret = readByteFlash( flashAdd, (uint8_t*)&stTempRecoveryFWInfo, (uint32_t)sizeof(SAppInfo) );
		if( ret )
		{
			GLogE( "fail read firmware info(%d)!!!\r\n", count );
			HAL_Delay( FIRMWARE_RETRY_DELAY );				// 10ms delay
		}
		else
		{
			break;
		}
	}
    
    // Compare Recovery FW Version
    uiNewVer = (gsFwInfo.msAppInfo[eApp_Recovery].marrucVersion[0] << 8) + gsFwInfo.msAppInfo[eApp_Recovery].marrucVersion[1];
    uiOldVer = (stTempRecoveryFWInfo.marrucVersion[0] << 8) + stTempRecoveryFWInfo.marrucVersion[1];
    
    memcpy(&ulRecoveryPreamble, &stTempRecoveryFWInfo.mReserved[0], sizeof(ulRecoveryPreamble));
    
    ret = checkCS( flashAdd + FIRMWARE_RECOVERY_INFO_OFFSET, &stTempRecoveryFWInfo );	
    
    // Recovery firmware info. is saved.
    if( (ulRecoveryPreamble == VCI3_FWINFO_PREAMBLE) && (ret == 0) )
    { 
        if( uiNewVer > uiOldVer )
        {
            /* Update stTempRecoveryFWInfo with new version info before writing to Flash.
             * Without this, the old version info would be written back to Flash header,
             * causing the version check to always trigger on next boot. */
            memcpy(&stTempRecoveryFWInfo, &gsFwInfo.msAppInfo[eApp_Recovery], sizeof(SAppInfo));
            memcpy(&stTempRecoveryFWInfo.mReserved[0], &ulRecoveryPreamble, sizeof(ulRecoveryPreamble));

            sprintf( FWPath, "%s/%s", EMMC_APPLICATION_FOLDER, RECOVERY_FW_NAME );
            if( RestoreRecovery( FWPath, stTempRecoveryFWInfo.mSize, stTempRecoveryFWInfo.mCheckSum, &stTempRecoveryFWInfo) != 0 )
            {
                return FALSE;
            }
        }
    }
    // Recovery firwmare info. is not saved.
    else
    {
        ulRecoveryPreamble = VCI3_FWINFO_PREAMBLE;
        memcpy(&stTempRecoveryFWInfo, &gsFwInfo.msAppInfo[eApp_Recovery], sizeof(SAppInfo));
        memcpy(&stTempRecoveryFWInfo.mReserved[0], &ulRecoveryPreamble, sizeof(ulRecoveryPreamble));
        sprintf( FWPath, "%s/%s", EMMC_APPLICATION_FOLDER, RECOVERY_FW_NAME );
        if( RestoreRecovery( FWPath, stTempRecoveryFWInfo.mSize, stTempRecoveryFWInfo.mCheckSum , &stTempRecoveryFWInfo) != 0 )
        {
            return FALSE;
        }
    }
    return TRUE;
}

bool RecoverSerial( void )
{
    uint32_t    flashAdd	= FIRMWARE_RECOVERY_ADD;
    uint8_t     count       = FIRMWARE_RETRY_COUNT;
    uint32_t    ret			= 0;
    bool        res         = false;
    uint32_t    ulRecoveryPreamble = 0;
    uint8_t     ucarrSerialTemp[SERIAL_NUMBER_SIZE] = {0,};
    SAppInfo    stTempRecoveryFWInfo;
    
    memset(&stTempRecoveryFWInfo, 0x00, sizeof(SAppInfo));
    
    // Read information of recovery firmware for checking preamble.
    while( count-- )
	{
		ret = readByteFlash( flashAdd, (uint8_t*)&stTempRecoveryFWInfo, (uint32_t)sizeof(SAppInfo) );
		if( ret )
		{
			GLogE( "fail read firmware info(%d)!!!\r\n", count );
			HAL_Delay( FIRMWARE_RETRY_DELAY );				// 10ms delay
		}
		else
		{
			break;
		}
	}
    
    // Read serial no.
    if( ret == FLASH_OK )
    {
        memcpy(&ulRecoveryPreamble, &stTempRecoveryFWInfo.mReserved[0], sizeof(ulRecoveryPreamble));
        
        if( ulRecoveryPreamble == VCI3_FWINFO_PREAMBLE )
        {
            count = FIRMWARE_RETRY_COUNT;
            while( count-- )
            {
                ret = readByteFlash( (flashAdd+(uint32_t)sizeof(SAppInfo)), &ucarrSerialTemp[0], SERIAL_NUMBER_SIZE );
                if( ret )
                {
                    GLogE( "fail read firmware info(%d)!!!\r\n", count );
                    HAL_Delay( FIRMWARE_RETRY_DELAY );				// 10ms delay
                }
                else
                {
                    memcpy( &gsFwInfo.marrucSerialNo[0], &ucarrSerialTemp[0], SERIAL_NUMBER_SIZE) ;
                    res = true;
                    break;
                }
            }
        }
        else
        {
            res = false;
        }
    }
    else
    {
        res = false;
    }
    
    
    return res;
}

FRESULT RemoveBkFirmwareInfo_EMMC( void )
{
    FIL 		Filepnt;
    char        arrTemp[30] = {0,};
    FRESULT		ret = FR_OK;
    
    f_getcwd(arrTemp, sizeof(arrTemp));
    sprintf(arrTemp, "%s", FILENAME_FW_INFO_BK_INI );
    
    ret = f_open(&Filepnt, FILENAME_FW_INFO_BK_INI, FA_OPEN_EXISTING | FA_WRITE);
    if ( ret == FR_OK )
    {
        f_close(&Filepnt);
        ret = f_unlink(arrTemp);
        if ( ret == FR_OK )
        {
            GLogN( "Remove OK : %s\r\n", arrTemp);
        }
        else
        {
            GLogN( "Remove Fail!!!\r\n");
        }
    }
    return ret;
}

uint8_t verifyFirmwareInfo( void )
{
    uint8_t     ret = 0;
    uint8_t     result = 0;
    SFwInfo		stBkFwInfo;
    
    
    // Firmware Information set 0
	memset( &gsFwInfo, 0x00, sizeof( SFwInfo ) );
	
    // Load Firmware Information

    ret = loadFirmwareInfo_EMMC();
    if( ret )
    {
        GLogE( "Error... Fail load firmware information from EMMC!!!\r\n" );
    }
	
	// check new bootloader process version1
	
    GLogN("boot loader new version process or error\r\n");
    // check check sum of firmware info
    if( isFirmwareInfoBroken(&gsFwInfo) == true )
    {
        GLogN("boot loader new version process, firmware info was broken\r\n");
        // recovery
        // add recovery firmware info
        // if cold booting at this point update firmware info
        if( RecoveryFirmwareInfo(&gsFwInfo) == false )
        {
            GLogN("boot loader new version process, recovery fail\r\n");
            GLogE( "Error.. Recovery Info Fail Fail\n");

            // fail to recovery firmware information -> Recovery firmware
            result = 1;
        }
        else
        {
            GLogN("boot loader new version process, recovery success\r\n");
        }
    }
    else
    {
        // normal process
        GLogN("boot loader new version process, firmware info is correct\r\n");
        // Change the location of BackUp information to flash.
        if( gsFwInfo.mucInitialized == FIRMWARE_INITIALIZED_V2  )
        {
            RemoveBkFirmwareInfo_EMMC();
            InitializeBackUpFwInfo_Flash();
        }
        else if( gsFwInfo.mucInitialized == FIRMWARE_INITIALIZED_V3 )
        {
            memset(&stBkFwInfo, 0x00, sizeof(SFwInfo));
            ret = loadFirmwareInfo_Flash(&stBkFwInfo, FIRMWARE_BK_INFO_ADD);
            if( ret  == 0 )
            {
                // add check routine for firmware info validation
                ret = isFirmwareInfoBroken(&stBkFwInfo);
                if( ret == true)
                {
                    RemoveBkFirmwareInfo_EMMC();
                    InitializeBackUpFwInfo_Flash();
                }
            }             
        }
    }
//    if( gsFwInfo.mucInitialized == FIRMWARE_INITIALIZED )
//	{
//		// exception case check backup frimeare winfo if didn't exist, update backup	
//		InitializeRecoveryFirmwareInfo();		
//	}
    return result;
}
