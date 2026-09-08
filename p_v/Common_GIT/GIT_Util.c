/**
  ******************************************************************************
  * @file    GIT_Util.c
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    05-SEP-2013
  * @brief   Manager GIT_Util.c module
  ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "GIT_Util.h"

#if !defined(FEATURE_BOOTLOADER)
#include "GIT_OemInterface.h"
#include "KISA_SEED_ECB.h"
#include "FOTA_Manager.h"
#include "Share_InterFunction.h"
#include "time.h"
#include "HdDebug.h"
#include "AutolinkConfig.h"
#include "UARTDMA_Manager.h"
#include "GIT_InterProtocol.h"
#include "Autolink_Manager.h"
#include "GIT_Gps.h"
#include <intrinsics.h> //mod.kks 21.12. 07 DI &EI
#include "SysHalFileSystem.h"
#endif
#include "GIT_SensorProc.h"
#include "HalHandler.h"

/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);

typedef struct __stEmergencyProperties
{
    uint8_t bEmergency;
}stEmergencyProperties;

/* Private define ------------------------------------------------------------*/
#if defined(FEATURE_BOOTLOADER)
#define Trace(...)
#else
#define Trace(...)  GITDebug(DEBUG_MODULES_UTIL,__VA_ARGS__)
#endif

#define FILE_NAME_SYS_TEST_INFO				"systestInfo.txt"

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
FIRMWARE_INFO g_FirmwareInfo;
FIRMWARE_INFO_BACKUP g_stFirmwareInfoBackup;
eFWApplIndex g_cDownloadFW_AppNumber = eApp_Bootloader;
uint32_t 	g_u32UpdateFileSize = 0;
uint16_t	g_u16UpdateFileCheckSum = 0;
unsigned char g_ucI2CFlag[2]={0,};

stEmergencyProperties m_stEmergencyInfo;

/* extern variables ----------------------------------------------------------*/
#if !defined(FEATURE_BOOTLOADER)
#ifdef USE_GIT_FAT_FS
extern void put_rc (FRESULT rc);
#endif
extern stDownloadStartReq   g_DownloadInfo;
#endif
extern uint32_t g_nSrcAddress;
extern void EnterStandbyAndRTCAlarmWakeup(uint32_t wSec);
extern void SELFTESTUart_Init(unsigned int);


/* Private functions ---------------------------------------------------------*/
void DisplayFirmWareInfo(void)
{
	int i;

    printf("SERIAL NO: [");
    for(i = 0; i < SIZE_SERIAL_NUMBER; i++ ) {
        printf("%c", ((g_FirmwareInfo.arrSerialNumber[i] >= ' ' && g_FirmwareInfo.arrSerialNumber[i] < 0x80) ? g_FirmwareInfo.arrSerialNumber[i] : '.'));
    }
    printf("]\r\n");

//    printf("VIN:[%s]\n", g_FirmwareInfo.m_strVIN);

    printf("VIN: [");
    for(i = 0; i < VIN_CODE_SIZE; i++ ) {
        printf("%c", g_FirmwareInfo.m_strVIN[i]);
    }
    printf("]\r\n");

//    printf("Module Key: [");
//    for(i = 0; i < ENCRYPT_KEY_SIZE; i++ ) {
//        printf("%02X", g_FirmwareInfo.m_strModuleKey[i]);
//    }
//    printf("]\r\n");

    printf("\r\nF/W Version Info\r\n");
//  printf("@%s()\n switch AppNum [%d]\n switch App status [%d]\r\n", __FUNCTION__, g_FirmwareInfo.SwitchingInfo.iApplMode, g_FirmwareInfo.SwitchingInfo.iStatus);
//  printf("g_FirmwareInfo.nApplicationUpdateSignal: 0x%x\r\n", g_FirmwareInfo.nApplicationUpdateSignal);

	for ( i = 0; i < eApp_MAX; i++ ) {
		printf("%d. Name[%c%c%c%c%c%c], signal[0x%X], Ver[%04X], Size[%d], CS[0x%X]\r\n",
										i + 1,
										g_FirmwareInfo.AppProperty[i].arrFWName[0],
										g_FirmwareInfo.AppProperty[i].arrFWName[1],
										g_FirmwareInfo.AppProperty[i].arrFWName[2],
										g_FirmwareInfo.AppProperty[i].arrFWName[3],
										g_FirmwareInfo.AppProperty[i].arrFWName[4],
										g_FirmwareInfo.AppProperty[i].arrFWName[5],
										g_FirmwareInfo.AppProperty[i].nSignal,
										g_FirmwareInfo.AppProperty[i].nAppFWVersion,
										g_FirmwareInfo.AppProperty[i].wFirmwareSize,
										g_FirmwareInfo.AppProperty[i].nCheckSum);
	}
}

void GetFirmwareInfo(FIRMWARE_INFO *pFirmWareInfo)
{
	uint32_t nFlashAddress = FIRMWARE_INFO_ADDRESS;				// (uint32_t)0x08004000		//16kbyte

    Trace("size: [%d]\n", sizeof(FIRMWARE_INFO));

	HalDrvFlashReadByteCallByRef((uint32_t*)&nFlashAddress, (uint8_t*)pFirmWareInfo, sizeof(FIRMWARE_INFO));
		
	DisplayFirmWareInfo();
}

unsigned char CalcChecksum(unsigned char* pBuff, unsigned int uiLength)
{
	unsigned char ucCalcCheckSum = 0;
	int i;

	for ( i=0; i<uiLength; i++ )
	{
		ucCalcCheckSum += pBuff[i];
	}

	return ucCalcCheckSum;
}

int16_t CalcChecksum_Short(unsigned char* pBuff, unsigned int uiLength)
{
	int16_t usCalcCheckSum = 0;
	int i;
	for ( i=0; i<uiLength; i++ )
	{
		usCalcCheckSum += pBuff[i];
	}
	return usCalcCheckSum;
}

void InitFirmwareInfo(void)
{
	unsigned char  ucCheckSum=0;
	memset(&g_FirmwareInfo, 0x00, sizeof(FIRMWARE_INFO));

	GetFirmwareInfo(&g_FirmwareInfo);


#if !defined(FEATURE_BOOTLOADER)
	sFLASH_ReadBuffer((uint8_t *)&g_stFirmwareInfoBackup, SECTOR_CONFIG_FIRMWAREINFO*SFLASH_SECTOR_SIZE, sizeof(FIRMWARE_INFO_BACKUP) );

	ucCheckSum = CalcChecksum((unsigned char*)&g_stFirmwareInfoBackup,sizeof(FIRMWARE_INFO)-1);
	if( (g_stFirmwareInfoBackup.nPreamble != FIRMWAREINFO_PREAMBLE) || (ucCheckSum != g_stFirmwareInfoBackup.stFirmwareInfo.m_ucCheckSum) )
	{
		g_FirmwareInfo.m_ucCheckSum = CalcChecksum((unsigned char*)&g_FirmwareInfo,sizeof(FIRMWARE_INFO)-1);
		memcpy(	&g_stFirmwareInfoBackup.stFirmwareInfo,&g_FirmwareInfo,sizeof(FIRMWARE_INFO));
		g_stFirmwareInfoBackup.nPreamble = FIRMWAREINFO_PREAMBLE;
		
		sFLASH_EraseSubSector(SECTOR_CONFIG_FIRMWAREINFO * SFLASH_SECTOR_SIZE);
		sFLASH_WriteBuffer((uint8_t *)&g_stFirmwareInfoBackup, SECTOR_CONFIG_FIRMWAREINFO*SFLASH_SECTOR_SIZE, sizeof(FIRMWARE_INFO_BACKUP));
	}
	
	ucCheckSum = CalcChecksum((unsigned char*)&g_stFirmwareInfoBackup,sizeof(FIRMWARE_INFO)-1);
	if( (g_stFirmwareInfoBackup.nPreamble == FIRMWAREINFO_PREAMBLE) && (ucCheckSum == g_stFirmwareInfoBackup.stFirmwareInfo.m_ucCheckSum) )
	{
		if( memcmp(&g_FirmwareInfo,&g_stFirmwareInfoBackup,sizeof(FIRMWARE_INFO)) != 0)
		{
			if( (g_FirmwareInfo.nApplicationUpdateSignal == 0xAFBECDAA) && (g_stFirmwareInfoBackup.stFirmwareInfo.nApplicationUpdateSignal == FIRMWARE_APP_SIGNAL) )
			{
				memcpy(&g_stFirmwareInfoBackup,&g_FirmwareInfo,sizeof(FIRMWARE_INFO));
				g_stFirmwareInfoBackup.nPreamble = FIRMWAREINFO_PREAMBLE;
				sFLASH_EraseSubSector(SECTOR_CONFIG_FIRMWAREINFO * SFLASH_SECTOR_SIZE);
				sFLASH_WriteBuffer((uint8_t *)&g_stFirmwareInfoBackup, SECTOR_CONFIG_FIRMWAREINFO*SFLASH_SECTOR_SIZE, sizeof(FIRMWARE_INFO_BACKUP));
			}
			else
			{
				memcpy(&g_FirmwareInfo,&g_stFirmwareInfoBackup.stFirmwareInfo,sizeof(FIRMWARE_INFO));
				SetFirmwareInfo(&g_FirmwareInfo);
			}
		}
	}

#if defined(QA_FIFA)
	if(memcmp(&g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], "QAH", 3) != 0)
	{
		memcpy(&g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO],"QAH",3);
		SetFWSerialNumber(g_FirmwareInfo.arrSerialNumber);
	}
#endif
#endif

}

// 2018.09.04 SPARROW : booting시 serial 유무에 따라 modem baudrate변경하는 함수
#if !defined(FEATURE_BOOTLOADER)
void SetSelftestModemUart(void)
{
	if( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER) == 0 )
	{
        HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);

        HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

		// MCU의 UART2 baudrate를 변경한다.
		Trace(" Init UART2(115200dbps)\n");
		ModemUart_Init(115200);
	}
}
#endif


void SetEmergencyStatus(boolean_t bOccurred)
{
    uint32_t nFlashAddress = EMERGENCY_INFO_ADDRESS;
	uint32_t uiWriteSize;

	Trace("\nEmergency : %d\n",bOccurred);

    m_stEmergencyInfo.bEmergency = bOccurred;


	uiWriteSize = sizeof(stEmergencyProperties);
    Trace("size: [%d]\n", uiWriteSize);


    if (HalDrvFlashWrite(nFlashAddress, 0, (char*)&m_stEmergencyInfo, uiWriteSize, 0) == HAL_RETURN_SUCCESS )
	{
		Trace("SetFirmwareInfo flash write success\n");
	}
	else {
		Trace("SetFirmwareInfo flash write fail\r\n");
	}
}

void GetEmergencyStatus()
{
    uint32_t nFlashAddress = EMERGENCY_INFO_ADDRESS;

    Trace("size: [%d]\n", sizeof(stEmergencyProperties));

   HalDrvFlashReadByteCallByRef((uint32_t*)&nFlashAddress, (uint8_t*)&m_stEmergencyInfo, sizeof(stEmergencyProperties));

	Trace("Emergency Occurred : %d\n",m_stEmergencyInfo.bEmergency);
}

void ClearEmergencyStatus()
{
    // Clear Emergency info
    SetFirmwareInfo(&g_FirmwareInfo);
}

void SetServiceType(DCSServiceType nServiceType)
{
    Trace("Set Service Type : %d\n",nServiceType);
    g_FirmwareInfo.nServiceType = nServiceType;

	g_FirmwareInfo.m_stURLInfo.nPreamble = 0;	//서비스타입변경시 URL도 재설정 필요하므로 세팅(Serial 기준으로 세팅됨)

    SetFirmwareInfo(&g_FirmwareInfo);
}

DCSServiceType GetServiceType()
{
//    Trace("Get Service Type : %d\n",g_FirmwareInfo.nServiceType);
    return g_FirmwareInfo.nServiceType;
}

void SetEncryptType(DCSEncryptType nEncryptType)
{
    Trace("Set Encrypt Type : %d\n",nEncryptType);
    g_FirmwareInfo.nEncryptType = nEncryptType;

    SetFirmwareInfo(&g_FirmwareInfo);
}

DCSEncryptType GetEncryptType()
{
    Trace("Get Encrypt Type : %d\n",g_FirmwareInfo.nEncryptType);
    return g_FirmwareInfo.nEncryptType;
}

void SetFirmwareInfo(FIRMWARE_INFO *pFirmWareInfo)
{
	uint32_t nFlashAddress = FIRMWARE_INFO_ADDRESS;
	uint32_t uiEraseEndAddress;
	uint32_t uiWriteSize;

	Trace("\r\nSet FW Information\r\n");

	pFirmWareInfo->m_ucCheckSum = CalcChecksum((unsigned char*)pFirmWareInfo,sizeof(FIRMWARE_INFO)-1);

	uiWriteSize = sizeof(FIRMWARE_INFO);
	uiEraseEndAddress = nFlashAddress  + uiWriteSize;

    Trace("size: [%d]\r\n", uiWriteSize);

    if (HalDrvFlashErase(nFlashAddress, uiEraseEndAddress, NULL, 0, 0) == HAL_RETURN_SUCCESS ) 
	{
        if (HalDrvFlashWrite(nFlashAddress, 0, (char*)pFirmWareInfo, uiWriteSize, 0) == HAL_RETURN_SUCCESS )
		{
			DisplayFirmWareInfo();
		}
		else {
			Trace("SetFirmwareInfo flash write fail\r\n");
		}
	}
	else {
		Trace("SetFirmwareInfo flash erase fail2\r\n");
        if (HalDrvFlashErase(nFlashAddress, uiEraseEndAddress, NULL, 0, 0) == HAL_RETURN_SUCCESS )
		{
            if (HalDrvFlashWrite(nFlashAddress, 0, (char*)pFirmWareInfo, uiWriteSize, 0) == HAL_RETURN_SUCCESS )
			{
				DisplayFirmWareInfo();
			}
			else {
				Trace("SetFirmwareInfo flash write fail3\r\n");
			}
		}
	}

#if !defined(FEATURE_BOOTLOADER)
	memcpy(	&g_stFirmwareInfoBackup,&pFirmWareInfo->nSignal,sizeof(FIRMWARE_INFO));
	g_stFirmwareInfoBackup.nPreamble = FIRMWAREINFO_PREAMBLE;
//	disk_write(FAT_VOLUME_DRV, (BYTE*)&g_stFirmwareInfoBackup, SECTOR_CONFIG_FIRMWAREINFO, 1);
	sFLASH_EraseSubSector(SECTOR_CONFIG_FIRMWAREINFO * SFLASH_SECTOR_SIZE);
	sFLASH_WriteBuffer((uint8_t *)&g_stFirmwareInfoBackup, SECTOR_CONFIG_FIRMWAREINFO*SFLASH_SECTOR_SIZE, sizeof(FIRMWARE_INFO_BACKUP));
#endif
}

void Uart8_Baudrate_Set( unsigned int uiInput )
{
	SELFTESTUart_Init(uiInput);
#if defined(FEATURE_USE_UART_RX_DMA)
	InitSELFTESTDMA();
#endif
}

#if !defined(FEATURE_BOOTLOADER)
void ClearFirmwareInfo(void)
{
	uint32_t nFlashAddress = FIRMWARE_INFO_ADDRESS;
	uint32_t uiEraseEndAddress;
	uint32_t uiWriteSize;

	Trace("SYS: clear FW information\n");
	uiWriteSize = sizeof(FIRMWARE_INFO);
	uiEraseEndAddress = nFlashAddress  + uiWriteSize;

    if (HalDrvFlashErase(nFlashAddress, uiEraseEndAddress, NULL, 0, 0) == HAL_RETURN_SUCCESS ) {
		Trace("SYS: flash erase OK\r\n");
	}
	else {
		Trace("SYS: flash erase fail\r\n");
	}

	return;
}

float GetFWAppVersion(eFWApplIndex eAppIdx)
{
	return atof((char*)g_FirmwareInfo.AppProperty[eAppIdx].nAppFWVersion);
}

char* GetFWSerialNumber(void)
{
	return (char*)g_FirmwareInfo.arrSerialNumber;
}

BOOL SetFWSerialNumber(char* strSerialNum)
{
	BYTE bLen;
	int i ;

	bLen = strlen(strSerialNum);
	if ( bLen > SIZE_SERIAL_NUMBER )
		return FALSE;

    memcpy(g_FirmwareInfo.arrSerialNumber, strSerialNum, SIZE_SERIAL_NUMBER);
	g_FirmwareInfo.arrSerialNumber[SIZE_SERIAL_NUMBER] = 0x00;

	Trace("\r\n%s [Set serial Number : ", __FUNCTION__);
	for ( i=0; i<SIZE_SERIAL_NUMBER; i++ )
	{
		Trace("%c", strSerialNum[i]);
	}
	Trace("]\r\n");

	SetFirmwareInfo(&g_FirmwareInfo);

	return TRUE;
}

//BOOL GetBTMacAddress(unsigned char* pBtMacAddr)
//{
//	DecyptData(pBtMacAddr, g_FirmwareInfo.Written_bt_addr, ENCRYPT_DECRYPT_DEFAULT_SIZE);
//
//	return TRUE;
//}

//BOOL SetBTMacAddress(unsigned char* pMacAddr)
//{
//	Trace("%s : MAC %02X %02X %02X %02X %02X %02X\r\n", __FUNCTION__,
//						pMacAddr[0], pMacAddr[1], pMacAddr[2], pMacAddr[3], pMacAddr[4], pMacAddr[5]);
//	memset(g_FirmwareInfo.Written_bt_addr, 0x00, ENCRYPT_DECRYPT_DEFAULT_SIZE +1 );
//	EncryptData(g_FirmwareInfo.Written_bt_addr, pMacAddr, ENCRYPT_DECRYPT_DEFAULT_SIZE);
//
//	SetFirmwareInfo(&g_FirmwareInfo);
//
//	return TRUE;
//}

//char* GetVehicleCode(void)
//{
////	sprintf((char*)g_FirmwareInfo.m_strVehicleCode,"FFFFFFFFFF\x00");				// test vehicle code
//
//	return (char*)g_FirmwareInfo.m_strVehicleCode;
//}

//BOOL SetVehicleCode(U8 * pucVehicleCode)
//{
////	Trace("Vehicle Code %s \r\n", pucVehicleCode);
////
////	memset(g_FirmwareInfo.m_strVehicleCode, 0x00, MAX_VEHICLECODE_SIZE);
////	memcpy(g_FirmwareInfo.m_strVehicleCode, pucVehicleCode, MAX_VEHICLECODE_SIZE-1);
////
////	SetFirmwareInfo(&g_FirmwareInfo);
////
//	return TRUE;
//}

void SetVINCode(U8 * inputVIN)
{
	Trace("VIN %s \r\n", inputVIN);

	memset( g_FirmwareInfo.m_strVIN, 0x00, VIN_CODE_SIZE+1);
	memcpy( g_FirmwareInfo.m_strVIN, inputVIN, VIN_CODE_SIZE );

	SetFirmwareInfo(&g_FirmwareInfo);
}

void GetVINCode(uint8_t * pcarrVin)
{
	memcpy( pcarrVin,g_FirmwareInfo.m_strVIN, VIN_CODE_SIZE );
}

void GetModuleKey(char* pstrModuleKey)
{
    memcpy(pstrModuleKey,g_FirmwareInfo.m_strModuleKey, ENCRYPT_KEY_SIZE);
//    pstrModuleKey[ENCRYPT_KEY_SIZE] = 0x00;
}

BOOL SetModuleKey(char* strModuleKey)
{
	Trace("strModuleKey : ");hexdump(strModuleKey,32);

//	memset(g_FirmwareInfo.m_strModuleKey, 0x00, ENCRYPT_KEY_SIZE);
	memcpy(g_FirmwareInfo.m_strModuleKey, strModuleKey, ENCRYPT_KEY_SIZE);
	g_FirmwareInfo.m_strModuleKey[ENCRYPT_KEY_SIZE] = 0x00;

	SetFirmwareInfo(&g_FirmwareInfo);

	return TRUE;
}

void SetModemActive(bool bActive)
{
	printf("Set Modem Active : %d \r\n", bActive);

	g_FirmwareInfo.bModemActive = (BYTE)bActive;

	SetFirmwareInfo(&g_FirmwareInfo);
}

bool GetModemActive()
{
//	printf("Get Modem Active : %d \r\n", g_FirmwareInfo.bModemActive);

	return (bool)g_FirmwareInfo.bModemActive;
}

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#define FILE_CNT_MAX 20			//최대 160개까지 검증완료
#define FILE_NUM_MAX 160		//최대 200개까지 검증완료

// Mode switch

#ifdef USE_FATFILE_SYSTEM
#ifdef USE_GIT_FAT_FS

FATFS g_Fatfs[_VOLUMES];
#endif
BOOL GetVCI2FileRead(char* strOpenFileName, BYTE* InBuff, UINT *iBuffLength)
{
	DWORD dwFileSize;
	stFileSystemDescript fpVer;
	eGitFresult ret = GIT_FR_OK;

	Trace("%s run\r\n", __FUNCTION__);

	if ( (ret = git_f_open(&fpVer, strOpenFileName, GIT_FA_EXIST | GIT_FA_READ)) == GIT_FR_OK )
	{
		//Trace("%s File Open Success\r\n", strOpenFileName);
		dwFileSize = git_f_size(&fpVer);

		if ((ret =  git_f_read(&fpVer, InBuff, dwFileSize, iBuffLength)) == GIT_FR_OK )
		{
			git_f_close(&fpVer);
			return TRUE;
		}
		else
		{
			git_f_close(&fpVer);
			Trace("%s File Read fail %d\r\n", strOpenFileName,ret);
		}
	}
	else
		Trace("%s File Open fail %d\r\n", strOpenFileName,ret);

	return FALSE;
}
BOOL DummyFileWrite(char* strOpenFileName,U8 *buff, UINT iBuffLength)
{
	stFileSystemDescript Filepnt;
	UINT uiWriteNum;
	U8 ret=0;

	if ( (ret = git_f_open(&Filepnt, strOpenFileName, GIT_FA_CREATE_NEW | GIT_FA_WRITE)) == GIT_FR_OK )
	{
		ret|=git_f_truncate(&Filepnt,0);
		ret|=git_f_write(&Filepnt, buff, iBuffLength, &uiWriteNum);
		ret|=git_f_close(&Filepnt);
	}

	if(ret != 0)
	{
		Trace("%s %s Fail!!! %d \r\n", __FUNCTION__, strOpenFileName,ret);
		return 1;
	}

	Trace("%s %s Success !!! %d \r\n", __FUNCTION__, strOpenFileName,ret);
	return 0;
}
BOOL GetVCI2FileWrite(char* strOpenFileName,U8 *buff, UINT iBuffLength)
{
	stFileSystemDescript Filepnt;
	UINT uiWriteNum;
	U8 ret=0;

	ret|=git_f_unlink(strOpenFileName);
	if ( (ret = git_f_open(&Filepnt, strOpenFileName, GIT_FA_CREATE_NEW | GIT_FA_WRITE)) == GIT_FR_OK )
	{
		ret|=git_f_truncate(&Filepnt,0);
		ret|=git_f_write(&Filepnt, buff, iBuffLength, &uiWriteNum);
		ret|=git_f_close(&Filepnt);
	}


	if(ret != 0)
	{
		Trace("%s %s Fail!!! %d \r\n", __FUNCTION__, strOpenFileName,ret);
		return 1;
	}

	Trace("%s %s Success !!! %d \r\n", __FUNCTION__, strOpenFileName,ret);
	return 0;
}

#ifdef USE_GIT_FAT_FS
void directory_list(void)
{
	DIR Dir;
	FRESULT res;
	static FILINFO Finfo;
	char Lfname[512];
	UINT s1 = 0, s2 = 0;
	long p1 = 0;
	FATFS *fs;
	char* strScanDirectory = DIR_ROOT;

	memset(&Finfo, NULL, sizeof(Finfo));

	res = f_opendir(&Dir, strScanDirectory);

	if (res) {
		return;
	}

	Trace("\r\n");
	Trace("fs: current directory - [%s]\r\n", DIR_ROOT);

	for(;;) {
#if _USE_LFN
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
		Finfo.fsize = 0;
#endif
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;

		if (Finfo.fattrib & AM_DIR) {
			s2++;
		}
		else {
			s1++;
			p1 += Finfo.fsize;
		}

		Trace("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s ",
										(Finfo.fattrib & AM_DIR) ? 'D' : '-',
										(Finfo.fattrib & AM_RDO) ? 'R' : '-',
										(Finfo.fattrib & AM_HID) ? 'H' : '-',
										(Finfo.fattrib & AM_SYS) ? 'S' : '-',
										(Finfo.fattrib & AM_ARC) ? 'A' : '-',
										(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
										(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
										Finfo.fsize, &(Finfo.fname[0]));
#if _USE_LFN
		if(strlen((const char *)Lfname) > 0) {
			Trace("[%s]\r\n", Lfname);
		}
		else {
			Trace("\r\n");
		}
#else
		Trace("\r\n");
#endif
	}

	Trace("\r\n%4u File(s),%10lu bytes total%4u Dir(s)\n", s1, p1, s2);

	if (f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == FR_OK) {
		Trace("%d bytes free\r\n\r\n", (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)4096)));
	}
	else {
		Trace("\r\n\r\n");
	}

	return;
}

void DirectoryList(char* strScanDirectory)
{
	DIR Dir;
	FRESULT res;
	FILINFO Finfo;
	char Lfname[512];
	UINT s1=0, s2=0;
	long p1=0;
	FATFS *fs;

	memset(&Finfo, NULL, sizeof(Finfo));

	res = f_opendir(&Dir, strScanDirectory);

	if (res) { /*put_rc((FRESULT)res);*/ return; }

	for(;;)
	{
#if _USE_LFN
		memset(Lfname, 0x00, sizeof(Lfname));
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
#endif
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;
		if (Finfo.fattrib & AM_DIR)
		{
			s2++;
		}
		else
		{
			s1++; p1 += Finfo.fsize;
		}

#if _USE_LFN
		GITDebugPrintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s [%s]\r\n",
		(Finfo.fattrib & AM_DIR) ? 'D' : '-',
		(Finfo.fattrib & AM_RDO) ? 'R' : '-',
		(Finfo.fattrib & AM_HID) ? 'H' : '-',
		(Finfo.fattrib & AM_SYS) ? 'S' : '-',
		(Finfo.fattrib & AM_ARC) ? 'A' : '-',
		(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
		(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,	Finfo.fsize, Lfname, &(Finfo.fname[0]));
#else
		GITDebugPrintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\r\n",
		(Finfo.fattrib & AM_DIR) ? 'D' : '-',
		(Finfo.fattrib & AM_RDO) ? 'R' : '-',
		(Finfo.fattrib & AM_HID) ? 'H' : '-',
		(Finfo.fattrib & AM_SYS) ? 'S' : '-',
		(Finfo.fattrib & AM_ARC) ? 'A' : '-',
		(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
		(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,	Finfo.fsize, &(Finfo.fname[0]));
		GITDebugPrintf("\r\n");
#endif
	}

//	GITDebugPrintf("\r\n%4u File(s),%10lu bytes total%4u Dir(s)", s1, p1, s2);
//
//	if (f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == FR_OK) {
//		GITDebugPrintf(", %d Mbytes free\r\n\r\n", (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)512)>>20));
//	}
//	else {
//		GITDebugPrintf("\r\n\r\n");
//	}

	Trace("\r\n%4u File(s),%10lu bytes total%4u Dir(s)", s1, p1, s2);
	AutoLinkManagerData.wSFlashTotalSize = p1;

	if (f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == FR_OK) {
		AutoLinkManagerData.wSFlashFreeSize = (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)4096));
		Trace(", %d bytes free\r\n\r\n", AutoLinkManagerData.wSFlashFreeSize);
	}
	else {
		Trace("\r\n\r\n");
	}

	Trace("%d bytes total\n", AutoLinkManagerData.wSFlashTotalSize);
	Trace("%d bytes free\n\n", AutoLinkManagerData.wSFlashFreeSize);

	return;
}
#endif

bool SystemTestInfoFileWrite(char *InBuff, uint16_t iBuffLength)
{
	UINT dwFileSize;
	stFileSystemDescript fpVer;
	eGitFresult ret = GIT_FR_OK;

	if ( (ret = git_f_open(&fpVer, FILE_NAME_SYS_TEST_INFO, GIT_FA_CREATE_ALWAYS | GIT_FA_WRITE)) == GIT_FR_OK ) {
		ret = git_f_write(&fpVer, (char *)InBuff, iBuffLength, &dwFileSize);
		git_f_close(&fpVer);
		if(ret != GIT_FR_OK) {

			Trace("@%s(), %s write fail!!! %d \r\n", __FUNCTION__, FILE_NAME_SYS_TEST_INFO, ret);

			return GIT_FR_DISK_ERR;
		}
	}
	else {
		Trace("@%s(), %s open fail!!! %d \r\n", __FUNCTION__, FILE_NAME_SYS_TEST_INFO, ret);

		return GIT_FR_DISK_ERR;
	}

	Trace("@%s(), %s Success !!! %d \r\n", __FUNCTION__, FILE_NAME_SYS_TEST_INFO, ret);

	return GIT_FR_OK;
}

bool SystemTestInfoFileRead(char *InBuff, uint16_t iBuffLength)
{
	UINT dwFileSize2;
	stFileSystemDescript fpVer;
	eGitFresult ret = GIT_FR_OK;

	Trace("SYS: @%s()\n", __FUNCTION__);

	if ( (ret = git_f_open(&fpVer, FILE_NAME_SYS_TEST_INFO, GIT_FA_EXIST | GIT_FA_READ)) == GIT_FR_OK ) { //20210412 확인필요
		if ((ret = git_f_read(&fpVer, InBuff, (UINT)iBuffLength, (UINT *)&dwFileSize2)) == GIT_FR_OK ) {
			git_f_close(&fpVer);

			Trace("SYS: read size: %d\r\n", dwFileSize2);
			if(iBuffLength == dwFileSize2) {
				return true;
			}
			else {
				return false;
			}
		}
		else {
			git_f_close(&fpVer);
			Trace("SYS: %s File Read fail %d\r\n", FILE_NAME_SYS_TEST_INFO, ret);

			return false;
		}
	}


	return false;
}

void LoadSettingInfoFile(bool bMode)
{
	bool ret;

	Trace("\n");
	Trace("SYS: @LoadSettingInfoFile()\n");
	ret = SystemTestInfoFileRead((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
	if(ret == false) {
		bMode = 1;				// set default
	}

	if(bMode == 0) {
	}
	else {
		Trace("SYS: <default setting>\n");
		Trace("SYS: File write(%s)\r\n", FILE_NAME_SYS_TEST_INFO);

		memset((char *)&stSystemTestInfo, 0x00, sizeof(SYSTEM_TEST_INFO));

        //MONI
		stSystemTestInfo.eModemFlowControlType = eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS_GPIO;
//		stSystemTestInfo.eModemFlowControlType = eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS;
		stSystemTestInfo.eUSBDeviceClassType = eUSB_DEVICE_CLASS_CDC;
		stSystemTestInfo.eDebugUartCh = eDEBUG_UART_CH_UART8;
		stSystemTestInfo.bRunTestMode = false;
//		stSystemTestInfo.wModemBaudrate = 3000000;

		ret = SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
		if(ret != GIT_FR_OK) {
			Trace("SYS: default setting fail!!! [%s](%d) \r\n", FILE_NAME_SYS_TEST_INFO, ret);

			return;
		}

		Trace("SYS: @%s() %s Success !!! %d \r\n", __FUNCTION__, FILE_NAME_SYS_TEST_INFO, ret);
	}

    stSystemTestInfo.eModemFlowControlType = eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS_GPIO;
//    stSystemTestInfo.eModemFlowControlType = eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS;
    stSystemTestInfo.eUSBDeviceClassType = eUSB_DEVICE_CLASS_BULK;
//	stSystemTestInfo.eUSBDeviceClassType = eUSB_DEVICE_CLASS_MSD;
//	stSystemTestInfo.eUSBDeviceClassType = eUSB_DEVICE_CLASS_CDC;
	stSystemTestInfo.eDebugUartCh = eDEBUG_UART_CH_UART8;
	stSystemTestInfo.bRunTestMode = false;

	Trace("\r\n");
	Trace("<System Test Setting Info>\r\n");
	Trace("stSystemTestInfo.eModemFlowControlType: %d\r\n", stSystemTestInfo.eModemFlowControlType);
	Trace("stSystemTestInfo.eUSBDeviceClassType: %d\r\n", stSystemTestInfo.eUSBDeviceClassType);
	Trace("stSystemTestInfo.eDebugUartCh: %d\r\n", stSystemTestInfo.eDebugUartCh);
	Trace("stSystemTestInfo.bRunTestMode: %d\r\n", stSystemTestInfo.bRunTestMode);
//	Trace("stSystemTestInfo.wModemBaudrate: %d\r\n", stSystemTestInfo.wModemBaudrate);

	if(stSystemTestInfo.eDebugUartCh == eDEBUG_UART_CH_UART8) {
		Trace(" - Serial debug ch: UART8\n");
	}
	else {
		Trace(" - Serial debug ch: UART7\n");
	}
	Trace("\n");

	return;
}

#if defined(USE_GIT_FAT_FS)
BOOL InitFatFileSystem(void)
{
	Trace("FS: Start File System\n");
	if (f_mount(FAT_VOLUME_DRV, &g_Fatfs[0]) == FR_OK)
	{
		Trace("FS: Fat File System mount Ok\r\n");

		if ( f_chdir(DIR_ROOT) == FR_OK )
		{
			Trace("FS: disk initial OK\r\n");
			Trace("FS: Change Directory root Ok\r\n");
			directory_list();
			return TRUE;
		}
		else
		{
			Trace("FS: format start\r\n");

			if ( f_mkfs(FAT_VOLUME_DRV, 0, 4096) == FR_OK )
			{
				Trace("FS: format success\r\n");

				if ( f_chdir(DIR_ROOT) == FR_OK )
				{
					Trace("FS: Create default foler\r\n");
					f_mkdir(DIR_LOG);
					f_mkdir(DIR_FIRMWARE);
					f_chmod(DIR_FIRMWARE, AM_HID, AM_HID);

					f_mkdir(DIR_INFORMATION);
					directory_list();
					return TRUE;
				}
				else
				{
					Trace("FS: FR_NOK\r\n");
				}
			}
			else
			{
				Trace("FS: format fail\r\n");
			}
		}
	}
	else
	{
		Trace("FS: Fat File System fail\r\n");
	}

	return FALSE;
}
#endif
#endif  //USE_FATFILE_SYSTEM

void DefaultFWInfo(void)
{
	// Firmware information 구조체에 초기값을 넣어준다.
	//memset(&g_FirmwareInfo, 0x00, sizeof(FIRMWARE_INFO));
	g_FirmwareInfo.nSignal = FIRMWARE_SIGNAL;

	// Bootloader 기본 정보 설정
	g_FirmwareInfo.AppProperty[eApp_Bootloader].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_Bootloader].wFirmwareSize = 100;//FIRMWARE_INFO_ADDRESS-BOOTLOADER_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_Bootloader].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Bootloader].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion = (uint16_t)DEFAULT_FW_VERION;

	// /APP 기본 정보 설정
	g_FirmwareInfo.AppProperty[eApp_Application].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_Application].wFirmwareSize = 100;//UPDATE_TMP_ADDRESS - APPLICATION_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_Application].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Application].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion = (uint16_t)DEFAULT_FW_VERION;

	// Master DB 기본 정보 설정
	g_FirmwareInfo.AppProperty[eApp_MasterDB].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_MasterDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_MasterDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion = (uint16_t)DEFAULT_FW_VERION;

	// Slave DB 기본 정보 설정
	g_FirmwareInfo.AppProperty[eApp_SlaveDB].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize = 100;//APPLICATION_ADDRESS-CAR_SLAVE_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_SlaveDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion = (uint16_t)DEFAULT_FW_VERION;

	// Driving DB 기본 정보 설정
	g_FirmwareInfo.AppProperty[eApp_ControlDB].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_ControlDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion = (uint16_t)DEFAULT_FW_VERION;


	g_FirmwareInfo.SwitchingInfo.iApplMode = eApp_Application;
	g_FirmwareInfo.SwitchingInfo.iStatus = eFW_SWITCH_COMPLETE;

	memset((char*)&g_FirmwareInfo.m_strVIN,NULL,VIN_CODE_SIZE + 1);
	memcpy((char*)&g_FirmwareInfo.m_strVIN, STR_DEFAULT_VIN, sizeof(STR_DEFAULT_VIN));

    //MONI 20180430
    // in default write mode, we must not delete seiral number.
    //memcpy(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER);

    // 2018/03/11 James Jean : 어차피 생산시점에 false이므로 초기화 값도 false로 변경
	g_FirmwareInfo.bModemActive = FALSE;

		//strncpy((char*)&g_FirmwareInfo.m_strVIN, STR_DEFAULT_VIN, strlen(STR_DEFAULT_VIN));
		memcpy(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER);
		memset(g_FirmwareInfo.m_strModuleKey, 0xFF, ENCRYPT_KEY_SIZE);

	SetFirmwareInfo(&g_FirmwareInfo);

	return;
}

void SelftestDefaultFWInfo(void)
{

	memset((char*)&g_FirmwareInfo.m_strVIN,NULL,VIN_CODE_SIZE + 1);
	memcpy((char*)&g_FirmwareInfo.m_strVIN, STR_DEFAULT_VIN, sizeof(STR_DEFAULT_VIN));

    //MONI 20180430
    // in default write mode, we must not delete seiral number.
    //memcpy(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER);

    // 2018/03/11 James Jean : 생산시점에 false이므로 초기화 값도 false로 변경
	g_FirmwareInfo.bModemActive = FALSE;

	//strncpy((char*)&g_FirmwareInfo.m_strVIN, STR_DEFAULT_VIN, strlen(STR_DEFAULT_VIN));
	memcpy(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER);
	memset(g_FirmwareInfo.m_strModuleKey, 0xFF, ENCRYPT_KEY_SIZE);

	SetFirmwareInfo(&g_FirmwareInfo);

	return;
}

U8 GitWriteUpdateDataTemp(U8 *buff, U16 Size)
{
	uint32_t nFlashAddress;
	uint32_t i;
	for (i=0; i<Size; i++ )	g_u16UpdateFileCheckSum += buff[i];	// calc checsum 16bit

/*
	int i;

	for ( i=0; i<Size; i++ )
		g_u16UpdateFileCheckSum += buff[i];	// calc checsum 16bit


	if ( g_cDownloadFW_AppNumber == eApp_Bootloader ) {
		nFlashAddress = ADDR_BOOT_SAVE+g_u32UpdateFileSize;
	}
	else if ( g_cDownloadFW_AppNumber == eApp_Application )	{
		nFlashAddress = ADDR_APPLICATION_SAVE1+g_u32UpdateFileSize;
	}
*/
    nFlashAddress = g_nSrcAddress + g_u32UpdateFileSize;

	//Trace("nFlashAddress: 0x%x Size:%d\n", nFlashAddress,Size);

    if (HalDrvFlashWrite(nFlashAddress, 0, (char*)buff, Size, 0) == HAL_RETURN_SUCCESS )
	{
		g_u32UpdateFileSize += Size;
	    printf(".");
	}
	else {
		Trace("%s flash write fail\r\n", __FUNCTION__);
	}
//	printf("g_u32UpdateFileSize : %d \r\n",g_u32UpdateFileSize);
	return TRUE;
}

BOOL DownloadClose(void)
{
	uint32_t nDestAddress;
	uint32_t nSrcAddress;
	uint32_t nEraseEndAddress;
	uint32_t nReadSize;
	uint32_t nTotalReadSize;

	BYTE arrTempBuff[SIZE_TEMP_BUFF];

	Trace("@DownloadClose()\n");
	Trace("g_u16UpdateFileCheckSum: 0x%x\n", g_u16UpdateFileCheckSum);
	Trace("g_DownloadFileInfo.nCheckSum: 0x%x\n", g_DownloadInfo.nCheckSum);

	if ( g_u16UpdateFileCheckSum == g_DownloadInfo.nCheckSum )
	{
		if ( g_cDownloadFW_AppNumber == eApp_Bootloader ) {
			Trace("<boot>\n");
			nDestAddress = BOOTLOADER_ADDRESS;
			nSrcAddress = ADDR_BOOT_SAVE;
		}
		else if ( g_cDownloadFW_AppNumber == eApp_Application )
		{
			Trace("<app>\n");
			nDestAddress = APPLICATION_ADDRESS;
			g_FirmwareInfo.nApplicationUpdateSignal = FIRMWARE_APP_SIGNAL;
			nSrcAddress = ADDR_APPLICATION_SAVE1;
		}
		else if ( g_cDownloadFW_AppNumber == eApp_MasterDB ) {
			Trace("<master db>\n");
			nDestAddress = CAR_MASTER_DB_ADDRESS;
			nSrcAddress = ADDR_MASTER_DB_SAVE;
		}
		else if ( g_cDownloadFW_AppNumber == eApp_SlaveDB ) {
			Trace("<slave db>\n");
			nDestAddress = CAR_SLAVE_ADDRESS;
			nSrcAddress = ADDR_SLAVE_DB_SAVE;
		}
		else if ( g_cDownloadFW_AppNumber == eApp_ControlDB ) {
			Trace("<control db>\n");
			nDestAddress = CAR_CTRL_DB_ADDRESS;
			nSrcAddress = ADDR_CONTROL_DB_SAVE;
		}
		else {
			Trace("\n\n<not support ext boot or ext app>\n\n");

			return TRUE;
		}

		//nSrcAddress = UPDATE_TMP_ADDRESS;
		nTotalReadSize = g_u32UpdateFileSize /* included CheckSum Length*/;

		nReadSize = SIZE_TEMP_BUFF;
		nEraseEndAddress = nDestAddress  + nTotalReadSize;
	//
		Trace("\r\n%s: g_cDownloadFW_AppNumber %d, checksum %d, g_u32UpdateFileSize %d\r\n", __FUNCTION__, g_cDownloadFW_AppNumber, g_u16UpdateFileCheckSum, g_u32UpdateFileSize);
		Trace("%s: nSrcAddress 0x%X, nDestAddress 0x%X \r\n", __FUNCTION__, nSrcAddress, nDestAddress);

		if ( g_cDownloadFW_AppNumber != eApp_Application ) {
            if (HalDrvFlashErase(nDestAddress, nEraseEndAddress, NULL, 0, 0) == HAL_RETURN_SUCCESS ) {
				printf("%s flash erase success\r\n", __FUNCTION__);
			}
			else {
				printf("%s flash erase fail\r\n", __FUNCTION__);
			}
	//
			Trace("%s: flash write start \r\n", __FUNCTION__);
	//
	//		// Decrypt 필요 없음.
			if ( nTotalReadSize < SIZE_TEMP_BUFF )		//150415 lwh 소스 위치 바꿈 db가 처음부터 512보다 작을 수 있다
				nReadSize = nTotalReadSize;
	//

			while ( nTotalReadSize > 0 ) {
                if(HalDrvFlashReadByteCallByRef((uint32_t*)&nSrcAddress, (uint8_t*)arrTempBuff, nReadSize)== HAL_RETURN_SUCCESS){
					if(HalDrvFlashWriteByteCallByRef(&nDestAddress, (uint8_t*)arrTempBuff, nReadSize) == HAL_RETURN_SUCCESS ) {
						printf(".");
					}
	//
					nTotalReadSize -= (nReadSize);
					if ( nTotalReadSize < SIZE_TEMP_BUFF ) {
						nReadSize = nTotalReadSize;
					}
				}
			}
			Trace("\r\n");
		}


		g_FirmwareInfo.AppProperty[g_cDownloadFW_AppNumber].nAppFWVersion = g_DownloadInfo.DB_Ver;
		memcpy(g_FirmwareInfo.AppProperty[g_cDownloadFW_AppNumber].arrFWName, g_DownloadInfo.DB_Name, MAX_FW_DB_FILE_NAME);
		g_FirmwareInfo.AppProperty[g_cDownloadFW_AppNumber].wFirmwareSize = g_u32UpdateFileSize;
		g_FirmwareInfo.AppProperty[g_cDownloadFW_AppNumber].nCheckSum = g_u16UpdateFileCheckSum;

		SetFirmwareInfo(&g_FirmwareInfo);

		return TRUE;
	}
	else
	{
		printf("[%s] Don't Match Recv Checksum %d, Calc Checksum %d\r\n", __FUNCTION__, g_DownloadInfo.nCheckSum, g_u16UpdateFileCheckSum);
		return FALSE;
	}
}


////////////////////////////////////////////////////////////////////////////////
unsigned char HexToDec(unsigned char ucHexData)
{
	unsigned char ucL_Nibble, ucH_Nibble, ucDecData;

	ucL_Nibble = ucHexData&0x0F;
	if( ucL_Nibble > 0x09 ) return 0xFF;

	ucH_Nibble = ucHexData>>4;
	if( ucH_Nibble > 0x09 ) return 0xFF;

	ucDecData = (ucH_Nibble*10)+ucL_Nibble;

	return ucDecData;
}

U8 AsciiToHex( U8 asciicode1, U8 asciicode2 )
{
 	U8 hexcode, hexcode1, hexcode2;

  	if( asciicode1 > 0x40 )
  	{
  		hexcode1 = asciicode1 - 0x37;
  		if( hexcode1 > 0x0F ) hexcode1 = 0x00;
  	}
  	else if( asciicode1 >= 0x30 )
  	{
  		hexcode1 = asciicode1 - 0x30;
  		if( hexcode1 > 0x09 ) hexcode1 = 0x00;
  	}
  	else
  		hexcode1 = 0x00;

  	if( asciicode2 > 0x40 )
  	{
  		hexcode2 = asciicode2 - 0x37;
  		if( hexcode2 > 0x0F ) hexcode2 = 0x00;
  	}
  	else if( asciicode2 >= 0x30 )
  	{
  		hexcode2 = asciicode2 - 0x30;
  		if( hexcode2 > 0x09 ) hexcode2 = 0x00;
  	}
  	else
  		hexcode2 = 0x00;

  	hexcode = (hexcode1<<4)+hexcode2;
  	return hexcode;
}

#ifdef USE_ALPU_CHIPSET
#include <stdlib.h>
#define ALPU_CHECK_COUNT        3
#define ALPU_FAIL_LIMIT         2

#define I2C_DELAY               Oem_GIT_mDelay(1)
#define I2C_DELAY_LONG          Oem_GIT_mDelay(100)

#define ERROR_CODE_FALSE        0
#define ERROR_CODE_TRUE         1

//ALPU-C
extern unsigned char _alpum_process(void);
extern unsigned char _alpu_rand(void);
extern void _alpu_delay_ms(unsigned int i);

extern unsigned char alpuc_process(unsigned char *, unsigned char *);
extern unsigned char I2C_Write(unsigned char, unsigned char, unsigned char*, unsigned char);
extern unsigned char I2C_Read(unsigned char, unsigned char, unsigned char* , unsigned char);


unsigned char _alpu_rand(void)
{
  static unsigned long seed; // 2byte, must be a static variable

  seed = seed + rand(); // rand(); <------------------ add time value
  seed =  seed * 1103515245 + 12345;

  return (seed/65536) % 32768;
}

void _alpu_delay_ms(unsigned int i)
{
	Oem_GIT_mDelay(i+1);
}

unsigned _i2c_write(unsigned char a, unsigned char b, unsigned char* c, unsigned char d)
{
    return I2C_Write(a, b, c, d);
}

unsigned _i2c_read(unsigned char a, unsigned char b, unsigned char* c, unsigned char d)
{
    return I2C_Read(a, b, c, d);
}


void AlpuCheck(void)
{
	int iAlpuCheckCnt = 0;
	int iAlpuCodeErrorFailCnt = 0;
	unsigned char ucError_code = ERROR_CODE_FALSE;
	BOOL bEncryptSuccess = FALSE;
	unsigned char tx_data[8];
	unsigned char dx_data[8];
	int i=0;

///////////////////////////////////////////////////////////////////
/*user_serial_read*/
/*
int j;
unsigned char sub_address;
for ( j=0; j<8; j++ )
{
    memset(dx_data, 0x00, 8);
    sub_address = 0x73 + j;
    ucError_code = _i2c_read(0x7A, sub_address, dx_data, 8);
    printf("subAddress 0x%02X, Error Code : %d\r\n", sub_address, ucError_code);
    printf("Data - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n\r\n", 
        dx_data[0], dx_data[1], dx_data[2], dx_data[3], 
        dx_data[4], dx_data[5], dx_data[6], dx_data[7]);
}
*/
///////////////////////////////////////////////////////////////////


#if 1   // alpuc_process占쏙옙 호占쏙옙占쌔억옙占쏙옙. 占쏙옙占쏙옙 占쌀쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙

	for( iAlpuCheckCnt=0; iAlpuCheckCnt < ALPU_CHECK_COUNT ; iAlpuCheckCnt++ ) 
    {
		for(i=0; i<8; i++) {
			tx_data[i] = _alpu_rand();
		}

		ucError_code = alpuc_process(tx_data, dx_data);

		if( ucError_code != ERROR_CODE_TRUE )//CODE FALSE
		{
			iAlpuCodeErrorFailCnt++;
#if 0   //DEBUG
			Trace("\r\nAlpu-M Encryption Test Fail!!!\r\n");
			Trace("nError_code : %d\r\n", ucError_code);
			Trace("========================ALPU-M IC Encryption========================\r\n");
			Trace(" Tx Data : "); for (i=0; i<8; i++) printf("0x%02x ", tx_data[i]); printf("\r\n");
			Trace(" Rx Data : "); for (i=0; i<8; i++) printf("0x%02x ", dx_data[i]); printf("\r\n");
			Trace("====================================================================\r\n");
			Trace("\r\n");

			//Trace("\r\n iAlpuCheckCnt:%d, iAlpuCodeErrorFailCnt:%d \r\n", iAlpuCheckCnt, iAlpuCodeErrorFailCnt);
#endif
		}
		else//CODE TRUE
		{
#if 0   //DEBUG
			Trace("\r\nAlpu-M Encryption Test Success!!!\r\n");
			Trace("Error_code : %d\r\n", ucError_code);
			Trace("========================ALPU-M IC Encryption========================\r\n");
			Trace(" Tx Data : "); for (i=0; i<8; i++) printf("0x%02x ", tx_data[i]); printf("\r\n");
			Trace(" Rx Data : "); for (i=0; i<8; i++) printf("0x%02x ", dx_data[i]); printf("\r\n");
			Trace("====================================================================\r\n");
			Trace("\r\n");
#endif

			bEncryptSuccess = TRUE;
			break;
		}
	}

    if ( bEncryptSuccess == FALSE ) g_ucI2CFlag[1] = 1;
#else
	uint16_t nRetry = 3;
	while(nRetry > 0) {
		for( iAlpuCheckCnt=0; iAlpuCheckCnt < ALPU_CHECK_COUNT ; iAlpuCheckCnt++ ) {
			for(i=0; i<8; i++) {
				tx_data[i] = _alpu_rand();
			}

			ucError_code = alpuc_process(tx_data, dx_data);

			//ucError_code=0;//test

			if( ucError_code != ERROR_CODE_TRUE )//CODE FALSE
			{
				iAlpuCodeErrorFailCnt++;
#if 0   //DEBUG
				Trace("\r\nAlpu-M Encryption Test Fail!!!\r\n");
				Trace("nError_code : %d\r\n", ucError_code);
				Trace("========================ALPU-M IC Encryption========================\r\n");
				Trace(" Tx Data : "); for (i=0; i<8; i++) printf("0x%02x ", tx_data[i]); printf("\r\n");
				Trace(" Rx Data : "); for (i=0; i<8; i++) printf("0x%02x ", dx_data[i]); printf("\r\n");
				Trace("====================================================================\r\n");
				Trace("\r\n");

				//Trace("\r\n iAlpuCheckCnt:%d, iAlpuCodeErrorFailCnt:%d \r\n", iAlpuCheckCnt, iAlpuCodeErrorFailCnt);
#endif
			}
			else//CODE TRUE
			{
#if 0   //DEBUG
				Trace("\r\nAlpu-M Encryption Test Success!!!\r\n");
				Trace("Error_code : %d\r\n", ucError_code);
				Trace("========================ALPU-M IC Encryption========================\r\n");
				Trace(" Tx Data : "); for (i=0; i<8; i++) printf("0x%02x ", tx_data[i]); printf("\r\n");
				Trace(" Rx Data : "); for (i=0; i<8; i++) printf("0x%02x ", dx_data[i]); printf("\r\n");
				Trace("====================================================================\r\n");
				Trace("\r\n");
#endif

				nRetry = 0;
				bEncryptSuccess = TRUE;
				break;
			}
		}

		if ( bEncryptSuccess == FALSE )
		{
			iAlpuCodeErrorFailCnt = 0;
			for( iAlpuCheckCnt=0; iAlpuCheckCnt < ALPU_CHECK_COUNT ; iAlpuCheckCnt++ )
			{
				ucError_code = _alpum_process(); //alpuc_process(tx_data, dx_data);//_alpum_process();

				//ucError_code=0;//test

				if( ucError_code != ERROR_CODE_TRUE )//CODE FALSE
				{
					iAlpuCodeErrorFailCnt++;
#if 0   //DEBUG
					Trace("\r\nAlpu-M Encryption Test Fail!!!\r\n");
					Trace("nError_code : %d\r\n", ucError_code);
					Trace("========================ALPU-M IC Encryption========================\r\n");
					Trace(" Tx Data : "); for (i=0; i<8; i++) printf("0x%02x ", tx_data[i]); printf("\r\n");
					Trace(" Rx Data : "); for (i=0; i<8; i++) printf("0x%02x ", dx_data[i]); printf("\r\n");
					Trace("====================================================================\r\n");
					Trace("\r\n");

					//Trace("\r\n iAlpuCheckCnt:%d, iAlpuCodeErrorFailCnt:%d \r\n", iAlpuCheckCnt, iAlpuCodeErrorFailCnt);
#endif
				}
				else//CODE TRUE
				{
#if 0   //DEBUG
					Trace("\r\nAlpu-M Encryption Test Success!!!\r\n");
					Trace("Error_code : %d\r\n", ucError_code);
					Trace("========================ALPU-M IC Encryption========================\r\n");
					Trace(" Tx Data : "); for (i=0; i<8; i++) printf("0x%02x ", tx_data[i]); printf("\r\n");
					Trace(" Rx Data : "); for (i=0; i<8; i++) printf("0x%02x ", dx_data[i]); printf("\r\n");
					Trace("====================================================================\r\n");
					Trace("\r\n");
#endif
					bEncryptSuccess = TRUE;
					break;
				}
			}
		}
		//Trace("\r\n iAlpuCodeErrorFailCnt: %d \r\n", iAlpuCodeErrorFailCnt);

		if( iAlpuCodeErrorFailCnt >= ALPU_FAIL_LIMIT )
		{
			//LedStaus = 200;

			printf("\r\n");
			printf("I2C Commmunication failure!!!\r\n");
			printf("\r\n");

			//	    printf("********************************************************\r\n");
			//	    printf("***@@@*************@@@@**********@@@@@*****@@@*****@@***\r\n");
			//	    printf("***@@@***********@@****@@******@@*****@@***@@@****@@****\r\n");
			//	    printf("***@@@**********@@******@@****@@***********@@@**@@@*****\r\n");
			//	    printf("***@@@**********@@******@@****@@***********@@@@@@*******\r\n");
			//	    printf("***@@@**********@@******@@****@@***********@@@@@@*******\r\n");
			//	    printf("***@@@**********@@******@@****@@***********@@@**@@@*****\r\n");
			//	    printf("***@@@***********@@****@@******@@*****@@***@@@****@@****\r\n");
			//	    printf("***@@@@@@@@@@******@@@@**********@@@@@*****@@@*****@@@**\r\n");
			//	    printf("********************************************************\r\n");
			//	    printf("\r\n");

			printf("\r\n");
			printf("Recovery I2C\r\n");
			printf("\r\n");
    
            HalDrvI2CIOCtrl(eI2C_IO_Enable, (int)HAL_I2C_1, NULL, 0, HAL_DISABLE);
            HalDrvI2CIOCtrl(eI2C_IO_DeInit, (int)HAL_I2C_1, NULL, 0, 0);

			APP_Delay(100);

			HalDrvI2C_SetInitialForGPIO();

			HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_RESET);			// ETC_3V3 power OFF

			HalGPIOSetVaule(GPIO_I2C1_SDA, eBIT_SET);			// I2C SDA: Low

			for(int xxx = 0; xxx < 20; xxx++) {
				HalGPIOSetVaule(GPIO_I2C1_SCL, eBIT_RESET);			// ETC_3V3 power OFF
				APP_Delay(1);
				HalGPIOSetVaule(GPIO_I2C1_SCL, eBIT_SET);			// ETC_3V3 power OFF
				APP_Delay(1);
			}


			APP_Delay(100);

			HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_SET);				// ETC_3V3 power ON
			APP_Delay(100);

            HalDrvI2COpen(0,0,NULL,0,0);

			APP_Delay(100);

			iAlpuCodeErrorFailCnt = 0;
			bEncryptSuccess = FALSE;

			printf("Check Sensor: MPU6515\n\r");

			MPU6515_GetName();

			nRetry--;
			if(nRetry == 0) {
				printf("\r\n");
				printf("\r\n********************************************************");
				printf("\r\n GPIO_ETC_PWEN RESET");
				printf("\r\n********************************************************");
				printf("\r\n");
				printf("\r\n");

				APP_Delay(500);
				g_ucI2CFlag[1] = 1;
//				SystemForcelyReset();
//				HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_RESET);			// ETC_3V3 power OFF
//				//MONI 20190217 ME i2c couldn't initialize hardware because i2c power be always supplied.
//                HalGPIOSetVaule(GPIO_MA_PWEN, eBIT_RESET);          // main powre off
//#warning "change wake up procedure"
				//EnterStandbyAndRTCAlarmWakeup(1);
				//MONI 20190201 sometime mcu stock in this code, so ME change reset fuction
//                while(TRUE);
			}
		}
	}
#endif
}

#endif		//USE_ALPU_CHIPSET


int LUT_Data_compute( int lut_no, int order, int int_data, char lut_buff[])	//DB LUT 읽어서처리
{
	char *word, *lastPos;//, iTokenCnt=0;//*p,
	char cLut[150]={0,};
	U8 cnt=0;
	U8 ucData=0;

	strncpy(cLut, lut_buff, strlen(lut_buff));

	word=strtok_r(cLut, "$", &lastPos);	//
	while(word!=NULL)	//LINE이 NULL
	{
		if(cnt==int_data)
		{
			if(strcmp(word, "-")==0)
				ucData = 0;
			else
			{
				if(word[0]>=0x41)
					ucData= *word;
				else
					ucData = atoi(word);
			}
			break;
		}

		word=strtok_r(NULL, "$", &lastPos);
		cnt++;
	}

	return ucData;
}

int LUT_Data_compute_20( int lut_no, int order, int int_data, char lut_buff[])	//DB LUT 읽어서처리
{
	char *p, *word, *lastPos[2], iTokenCnt=0;
	char cLut[150]={0,};
	//U8 cnt=0;
	U8 ucMask=0, ucCompare=0, ucValue=0, ucData=0;

	p=strtok_r(lut_buff, "$", &lastPos[0]);
	while(p!=NULL)
	{
		strncpy(cLut, p, sizeof(cLut)/sizeof(cLut[0]));
		word=strtok_r(cLut, ",",  &lastPos[1]);
		while( word != NULL )
		{
			int nFieldNum=iTokenCnt%3;
			switch(nFieldNum)
			{
			case 0:		//MASKING VALUE
				ucMask=AsciiToHex(word[2], word[3]);
				break;
			case 1:		//COMPARE VALUE
				ucCompare=AsciiToHex(word[2], word[3]);
				break;
			case 2:		//DATA SAVE
				if(word[0]>=0x41)
					ucValue= *word;
				else
					ucValue = atoi(word);
				break;
			default:
			  break;
			}

			iTokenCnt++;
			word=strtok_r(NULL, ",",  &lastPos[1]);
		}
		if(((U8)int_data & ucMask)==ucCompare)
		{
			if(strcmp(word, "-")==0)
				ucData = 0;
			else
				ucData = ucValue;
			break;
		}
		else
		{
			p=strtok_r(NULL, "$", &lastPos[0]);
		}
	}

	return ucData;
}

unsigned int bit_mask(unsigned char comm_data)
{
    unsigned char out, mask=0x01;
    short int    i;

    for( i=0; i<8; i++) {
        out = comm_data & mask;
        if( out != 0)   {
            comm_data >>= i;
            return( i+1);
        }
        mask <<= 1;
    }
    return( 0);
}

unsigned int bit_mask1( unsigned char comm_data, unsigned char mask_data)
{
    unsigned char out, mask=0x01, ret;
    short int    i;

    for( i=0; i<8; i++)
    {
        out = mask_data & mask;
        if( out != 0)
        {
            ret = (comm_data & mask_data) >> i;
            return( ( unsigned int)ret);
        }
        mask <<= 1;
    }
    return( 0);
}

unsigned int bit_mask2(unsigned short comm_data, unsigned short mask_data)
{
	unsigned short int out, mask=0x8000, ret=0, ret1, i, Sumbit=0;

	out = mask_data & comm_data;

	for( i=0; i<16; i++)
	{
		ret = mask_data & mask;
		if (ret != 0)
		{
			ret1 = out & mask;
			if (ret1!=0)
			{
				Sumbit = Sumbit << 1;
				Sumbit++;
			}
			else
				Sumbit = Sumbit << 1;
		}
		mask >>= 1;
	}
	return( ( unsigned int)Sumbit );
}

// 전달받은 mask 값과 masking 수행 후 처리 - 10번 Type
unsigned int bit_mask3(unsigned char comm_data, unsigned char mask_data)
{
	unsigned char out, mask=0x01;
	short int    i;

	// 전달받은 mask 값과  masking 수행 후 bit_mask() 함수와 같은 방식으로 수행
	comm_data = comm_data & mask_data;

	for( i=0; i<8; i++) {
		out = comm_data & mask;
		if( out != 0)   {
			comm_data >>= i;
			return( i+1);
		}
		mask <<= 1;
	}
	return( 0);

}

unsigned int bit_mask4(unsigned char ucComm_data, unsigned char ucMask_data)
{
	unsigned char ucMask=0x01;
	unsigned int uiRet=0;
	int i=0;

	for( i=0; i<8; i++ )
	{
		if( (ucMask_data & ucMask) != 0 )	break;
		ucMask <<= 1;
	}
	ucComm_data = ucComm_data & ucMask_data;
	uiRet = ucComm_data>>i;
	return uiRet;
}

/**
 * Join a Multicast Group
 * @param source source string
 * @param block_size size of one block
 * @param dest will take result string
 */
int pkcs7_pad_write(char *source, int block_size, char *dest)
{
	int source_len = strlen(source);
	int dest_len = (source_len / (block_size) + 1) * block_size;
	int pad = dest_len - source_len;

//	Trace("dest_len: %d, dest_len: %d\n", dest_len, dest_len);
//	Trace("source_len: %d, block_size: %d\n", source_len, block_size);
	if(source_len % block_size == 0) {
		strncpy(dest, source, source_len);
		dest = (char*)(dest+ source_len);
		for(int i=0; i<block_size; i++) {
			*dest = block_size;
			dest++;
		}

		return dest_len;
	}
	else {
		// 패딩
		// 채워질 문자열
		strncpy(dest, source, source_len);
		dest = (char*)(dest+ source_len);
		for(int i = 0; i < pad; i++) {
//			Trace("(%d)pad: %02x\n", i, pad);
			*dest = pad;
			dest++;
		}

		return dest_len;
	}
}

int pkcs7_pad_read(char *source, int sourceLength )
{
	int destLength = 0;

	destLength = source[sourceLength - 1];
	destLength = sourceLength - destLength;

	return destLength;
}

#define HEXDUMP_COLS 16

void hexdump(void *mem, unsigned int len)
{
	unsigned int i, j;

	if(GetSysDebugMode() == DEBUG_MODE_NONE || len==0) return;

	for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
	{
		/* print offset */
		if(i % HEXDUMP_COLS == 0)
		{
			printf("0x%06x: ", i);
		}

		/* print hex data */
		if(i < len)
		{
			printf("%02x ", 0xFF & ((char*)mem)[i]);
		}
		else /* end of block, just aligning for ASCII dump */
		{
			printf("   ");
		}

		/* print ASCII dump */
		if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
		{
			for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
			{
				if(j >= len) /* end of block, not really printing */
				{
					putchar(' ');
				}
				else if(isprint(((char*)mem)[j])) /* printable char */
				{
					putchar(0xFF & ((char*)mem)[j]);
				}
				else /* other char */
				{
					putchar('.');
				}
			}
			printf("\r\n");
		}
	}
	printf("\r\n");
}

#define SCANF_TOKS_COMPARE   (1)
#define SCANF_TOKS_PERCENT   (2)
#define SCANF_TOKS_WIDTH     (3)
#define SCANF_TOKS_TYPE      (4)

int satoi(const char *str, int str_sz, int radix)
{
    char *tmp_ptr;
    char buff[NMEA_CONVSTR_BUF];
    int res = 0;

    if(str_sz < NMEA_CONVSTR_BUF)
    {
        memcpy(&buff[0], str, str_sz);
        buff[str_sz] = '\0';
        res = strtol(&buff[0], &tmp_ptr, radix);
    }

    return res;
}

/**
 * \brief Convert string to fraction number
 */
double satof(const char *str, int str_sz)
{
    char *tmp_ptr;
    char buff[NMEA_CONVSTR_BUF];
    double res = 0;

    if(str_sz < NMEA_CONVSTR_BUF)
    {
        memcpy(&buff[0], str, str_sz);
        buff[str_sz] = '\0';
        res = strtod(&buff[0], &tmp_ptr);
    }

    return res;
}

/**
 * \brief Analyse string (specificate for NMEA sentences)
 */
int ssscanf(const char *buff, int buff_sz, const char *format, ...)
{
	const char *beg_tok;
	const char *end_buf = buff + buff_sz;

	va_list arg_ptr;
	int tok_type = SCANF_TOKS_COMPARE;
	int width = 0;
	const char *beg_fmt = 0;
	int snum = 0, unum = 0;

	int tok_count = 0;
	void *parg_target;

	va_start(arg_ptr, format);

	for(; *format && buff < end_buf; ++format) {
    switch(tok_type) {
	    case SCANF_TOKS_COMPARE:
	      if('%' == *format) {
	        tok_type = SCANF_TOKS_PERCENT;
	      }
	      else if(*buff++ != *format) {
					goto fail;
				}
				break;
			case SCANF_TOKS_PERCENT:
        width = 0;
        beg_fmt = format;
        tok_type = SCANF_TOKS_WIDTH;
      case SCANF_TOKS_WIDTH:
        if(isdigit(*format)) {
            break;
				}

        {
          tok_type = SCANF_TOKS_TYPE;
          if(format > beg_fmt) {
						width = satoi(beg_fmt, (int)(format - beg_fmt), 10);
					}
        }
			case SCANF_TOKS_TYPE:
        beg_tok = buff;

        if(!width && ('c' == *format || 'C' == *format) && *buff != format[1]) {
					width = 1;
				}

        if(width) {
					if(buff + width <= end_buf)
						buff += width;
					else
						goto fail;
        }
        else {
					if(!format[1] || (0 == (buff = (char *)memchr(buff, format[1], end_buf - buff))))
						buff = end_buf;
        }

				if(buff > end_buf)
					goto fail;

        tok_type = SCANF_TOKS_COMPARE;
        tok_count++;

        parg_target = 0;
        width = (int)(buff - beg_tok);

				switch(*format) {
          case 'c':
          case 'C':
            parg_target = (void *)va_arg(arg_ptr, char *);
            if(width && 0 != (parg_target)) {
							*((char *)parg_target) = *beg_tok;
						}
            break;

          case 's':
          case 'S':
              parg_target = (void *)va_arg(arg_ptr, char *);
              if(width && 0 != (parg_target)) {
                memcpy(parg_target, beg_tok, width);
                ((char *)parg_target)[width] = '\0';
              }
              break;

          case 'f':
          case 'g':
          case 'G':
          case 'e':
          case 'E':
            parg_target = (void *)va_arg(arg_ptr, double *);
            if(width && 0 != (parg_target)) {
							*((double *)parg_target) = satof(beg_tok, width);
						}
            break;
				};

        if(parg_target)
            break;

        if(0 == (parg_target = (void *)va_arg(arg_ptr, int *)))
            break;

        if(!width)
            break;

        switch(*format) {
          case 'd':
          case 'i':
            snum = satoi(beg_tok, width, 10);
            memcpy(parg_target, &snum, sizeof(int));
            break;

          case 'u':
            unum = satoi(beg_tok, width, 10);
            memcpy(parg_target, &unum, sizeof(unsigned int));
            break;

          case 'x':
          case 'X':
            unum = satoi(beg_tok, width, 16);
            memcpy(parg_target, &unum, sizeof(unsigned int));
            break;

          case 'o':
            unum = satoi(beg_tok, width, 8);
            memcpy(parg_target, &unum, sizeof(unsigned int));
            break;

          default:
              goto fail;
				};
				break;
		};
	}

fail:
	va_end(arg_ptr);

	return tok_count;
}

void SystemSoftwareReset(void)
{
	#define AIRCR_VECTKEY_MASK				(0x05FA0000)
	SCB->AIRCR = AIRCR_VECTKEY_MASK | 0x04;
	while(1);
}

void GetDateTimeStamp(uint8_t *ptr, stHalRTC_DateTypeDef DateStamp, stHalRTC_TimeTypeDef TimeStamp)
{
	// 주행 시작 시간(Local)
	sprintf((char *)ptr, "%04d%02d%02d%0.2d%0.2d%0.2d\x00",
				DateStamp.RTC_Year + 2000,
				DateStamp.RTC_Month,
				DateStamp.RTC_Date,
				TimeStamp.RTC_Hours,
				TimeStamp.RTC_Minutes,
				TimeStamp.RTC_Seconds);

	return;
}


void GetDatefromDateArray(char* parrDateTime,stHalRTCTypeDef* pstDate)
{
    char year[5]={0,};
    char month[3]={0,};
    char day[3]={0,};
    char hh[3]={0,};
    char mm[3]={0,};
    char ss[3]={0,};

    memcpy(year,&parrDateTime[0],4);
    memcpy(month,&parrDateTime[4],2);
    memcpy(day,&parrDateTime[6],2);
    memcpy(hh,&parrDateTime[8],2);
    memcpy(mm,&parrDateTime[10],2);
    memcpy(ss,&parrDateTime[12],2);

    pstDate->RtcDate.RTC_Year = atoi(year)-2000;
    pstDate->RtcDate.RTC_Month = atoi(month);
    pstDate->RtcDate.RTC_Date = atoi(day);
    pstDate->RtcTime.RTC_Hours = atoi(hh);
    pstDate->RtcTime.RTC_Minutes = atoi(mm);
    pstDate->RtcTime.RTC_Seconds = atoi(ss);

    printf("year : %d, mon : %d, day : %d, h : %d, m : %d, s : %d\r\n",
        pstDate->RtcDate.RTC_Year+2000,pstDate->RtcDate.RTC_Month,pstDate->RtcDate.RTC_Date,
        pstDate->RtcTime.RTC_Hours,pstDate->RtcTime.RTC_Minutes,pstDate->RtcTime.RTC_Seconds);
}

unsigned char PolygonGeoFenceCheck( GPSPOINT CurrentPoint, GPSPOINT* List, int PointCount )
{
    unsigned char ucCheckValue=0,ucRet=0;
	int i=0;

    for (i=0; i<PointCount-1; i++) {
        if (List[i].m_dLat <= CurrentPoint.m_dLat) {
            if (List[i+1].m_dLat  > CurrentPoint.m_dLat)
                 if (isLeft( List[i], List[i+1], CurrentPoint) > 0.0)
                    ++ucCheckValue;
        }
        else {
            if (List[i+1].m_dLat  <= CurrentPoint.m_dLat)
                 if (isLeft( List[i], List[i+1], CurrentPoint) < 0.0)
                    --ucCheckValue;
        }
    }

	if( ucCheckValue == 0 )	ucRet = GPS_OUT_RANGE;
	else								ucRet = GPS_IN_RANGE;

    return ucRet;
}

double isLeft( GPSPOINT P0, GPSPOINT P1, GPSPOINT P2 )
{
    return ( (P1.m_dLon - P0.m_dLon) * (P2.m_dLat - P0.m_dLat)
            - (P2.m_dLon -  P0.m_dLon) * (P1.m_dLat - P0.m_dLat) );
}
#endif				// #if defined(FEATURE_BOOTLOADER)

/*****************************END OF FILE****/
