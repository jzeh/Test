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
#include "HalHandler.h"
#include "HalFlashDriver.h"
#include "GIT_Util.h"

FIRMWARE_INFO g_FirmwareInfo;

typedef void (*pFunction)(void);

void DisplayFirmWareInfo(void)
{
	int i;
    char arrTemp[MAX_FW_DB_FILE_NAME+1] ={0,};

    printf("/********************************************************************************************\r\n");
	printf("SERIAL NO: [");
	for(i = 0; i < SIZE_SERIAL_NUMBER; i++ ) {
		printf("%c", ((g_FirmwareInfo.arrSerialNumber[i] >= ' ' && g_FirmwareInfo.arrSerialNumber[i] < 0x80) ? g_FirmwareInfo.arrSerialNumber[i] : '.'));
	}
	printf("]\r\n\r\n");

	printf("[%s]switch AppNum [%d], switch App status [%d]\r\n", __FUNCTION__, g_FirmwareInfo.SwitchingInfo.iApplMode, g_FirmwareInfo.SwitchingInfo.iStatus);
    for ( i = 0; i < eApp_MAX; i++ ) {
        memcpy(arrTemp, g_FirmwareInfo.AppProperty[i].arrFWName, MAX_FW_DB_FILE_NAME);
		printf("%d. App Name [%s], signal [0x%X], Version [%04X], FWSize [%d], CheckSum [0x%X]\r\n",
					i + 1,
					arrTemp,
					g_FirmwareInfo.AppProperty[i].nSignal, g_FirmwareInfo.AppProperty[i].nAppFWVersion,
					g_FirmwareInfo.AppProperty[i].wFirmwareSize, g_FirmwareInfo.AppProperty[i].nCheckSum);
	}
    printf("/********************************************************************************************\r\n");
}

void GetFirmwareInfo(FIRMWARE_INFO *pFirmWareInfo)
{
	uint32_t nFlashAddress = FIRMWARE_INFO_ADDRESS;				// (uint32_t)0x08004000		//16kbyte

    HalDrvFlashReadByteCallByRef((uint32_t*)&nFlashAddress, (uint8_t*)pFirmWareInfo, sizeof(FIRMWARE_INFO));

	DisplayFirmWareInfo();
}

void InitFirmwareInfo(void)
{
	memset(&g_FirmwareInfo, 0x00, sizeof(FIRMWARE_INFO));

	GetFirmwareInfo(&g_FirmwareInfo);
}

uint32_t Appl_Mode_Switch(void)
{
	stSwitchingInfo DummyMode;

	GetSwitchingModeInfo(&DummyMode);

	// ApplicationÏù¥ ÏóÖÎç∞Ïù¥Ìä∏ ÎêòÏóàÎäîÏßÄ Ï≤¥ÌÅ¨ÌïòÍ≥† flashÏóê writeÌï¥ Ï§ÄÎã§.
	Check_n_Update_Applicaton();

	return (RUNNING_ADDRESS);
}

void JumpToApp(uint32_t uiDestinationAddress)
{
	pFunction Jump_To_Application;
	uint32_t JumpAddress;

	JumpAddress = *(__IO uint32_t*) (uiDestinationAddress + 4);

	/* Jump to user application */
	Jump_To_Application = (pFunction) JumpAddress;

	/* Initialize user application's Stack Pointer */
	__set_MSP(*(__IO uint32_t*) uiDestinationAddress);

	Jump_To_Application();
}

void Check_n_Update_Applicaton(void)
{
	// nApplicationUpdateSignalÍ∞Ä FIRMWARE_APP_SIGNALÏù¥Î©¥ Îã§Ïö¥Î°úÎçîÍ∞Ä
	// UPDATE_TMP_ADDRESS Ïóê Ï†ÄÏû•ÎêòÏñ¥ ÏûàÎã§Îäî ÏÉÅÌÉúÎ°ú,
	// flashÏóê writeÌïòÍ≥† nApplicationUpdateSignalÏùÑ Î≥ÄÍ≤ΩÏãúÏºú Ï§òÏÑú
	// Ïù¥ÌõÑÏóêÎäî flash writeÎèôÏûëÏù¥ Ïù¥Î£®Ïñ¥ÏßÄÏßÄ ÏïäÎèÑÎ°ù ÌïúÎã§.
	// Í∑∏Î†áÏßÄ ÏïäÏúºÎ©¥ Îã§Ïö¥Î°úÎçî ÏóÖÎç∞Ïù¥Ìä∏Í∞Ä ÎêòÏñ¥ ÏûàÏßÄ ÏïäÏùÄ ÏÉÅÌÉú
	uint32_t nDestAddress, nSrcAddress, nEraseEndAddress;
	uint32_t nReadSize, nTotalReadSize;
	uint16_t nSavedChecksum=0;
	BYTE arrTempBuff[SIZE_TEMP_BUFF];
	uint8_t ucCheckSum[2];

	if ( (g_FirmwareInfo.nApplicationUpdateSignal == FIRMWARE_APP_SIGNAL) ||
		((g_FirmwareInfo.nApplicationUpdateSignal != 0xAFBECDAA)&&((g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion == 8) || (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion == 9)|| (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion == 10))))
	{
		nSrcAddress = UPDATE_TMP_ADDRESS;
		nDestAddress = APPLICATION_ADDRESS;
		nTotalReadSize = g_FirmwareInfo.AppProperty[eApp_Application].wFirmwareSize;
		nSavedChecksum = (uint16_t)g_FirmwareInfo.AppProperty[eApp_Application].nCheckSum;
		nReadSize = SIZE_TEMP_BUFF;
		nEraseEndAddress = nDestAddress  + nTotalReadSize;

        if (HalDrvFlashErase(nDestAddress, nEraseEndAddress, NULL, 0, 0) == HAL_RETURN_SUCCESS ) {
//			printf("%s flash erase success\r\n", __FUNCTION__);
		}
		else {
			printf("%s flash erase fail\r\n", __FUNCTION__);
		}

		while ( nTotalReadSize > 0 )
		{
            if( HalDrvFlashReadByteCallByRef((uint32_t*)&nSrcAddress, (uint8_t*)arrTempBuff, nReadSize) == HAL_RETURN_SUCCESS )
			{
				if(HalDrvFlashWriteByteCallByRef(&nDestAddress, (uint8_t*)arrTempBuff, nReadSize) == HAL_RETURN_SUCCESS )
				{
//					printf(".");
				}

				nTotalReadSize -= (nReadSize);
				if ( nTotalReadSize < SIZE_TEMP_BUFF )
					nReadSize = nTotalReadSize;
			}
		}

		ucCheckSum[0] = (uint8_t)(nSavedChecksum & 0x00FF);
		ucCheckSum[1] = (uint8_t)((nSavedChecksum >> 8) & 0x00FF);

		// checksum writeÌïòÍ≥†
		// nApplicationUpdateSignalÏùÑ Î≥ÄÍ≤ΩÏãúÏºú Ï§òÏÑú Ïù¥ÌõÑ Îã§Ïö¥Î°úÎçîÎ•º WRITEÌïòÏßÄ ÏïäÎèÑÎ°ù ÌïúÎã§.
		if(HalDrvFlashWriteByteCallByRef(&nDestAddress, (uint8_t*)ucCheckSum, 2) == HAL_RETURN_SUCCESS )
		{
			g_FirmwareInfo.nApplicationUpdateSignal = 0xAFBECDAA;
			SetFirmwareInfo(&g_FirmwareInfo);
		}
	}
}

void SetFirmwareInfo(FIRMWARE_INFO *pFirmWareInfo)
{
	uint32_t nFlashAddress = FIRMWARE_INFO_ADDRESS;
	uint32_t uiEraseEndAddress;
	uint32_t uiWriteSize;
	uint16_t i;

	printf("\r\n");
	printf("Set FW Information\r\n");

	printf("VEHICLE CODE: [");
	for(i = 0; i < MAX_VEHICLECODE_SIZE; i++ ) {
		printf("%c", ((g_FirmwareInfo.m_strVehicleCode[i] >= ' ' && g_FirmwareInfo.m_strVehicleCode[i] < 0x80) ? g_FirmwareInfo.m_strVehicleCode[i] : '.'));
	}
	printf("]\r\n");

	uiWriteSize = sizeof(FIRMWARE_INFO);
	uiEraseEndAddress = nFlashAddress  + uiWriteSize;

        if (HalDrvFlashErase(nFlashAddress, uiEraseEndAddress, NULL, 0, 0) == HAL_RETURN_SUCCESS ) {
			if(HalDrvFlashWriteByteCallByRef(&nFlashAddress, (uint8_t*)pFirmWareInfo, uiWriteSize) == HAL_RETURN_SUCCESS ) {
    			printf("SetFirmwareInfo flash write success\r\n");
    			DisplayFirmWareInfo();
		}
		else {
			printf("SetFirmwareInfo flash write fail\r\n");
		}
	}
	else {
		printf("SetFirmwareInfo flash erase fail\r\n");
	}
}

BOOL GetSwitchingModeInfo(stSwitchingInfo* pSwitchMode)
{
	BOOL bRet = FALSE;

	if(g_FirmwareInfo.nSignal == FIRMWARE_SIGNAL ) {
		pSwitchMode->iApplMode 	= eApp_Application;
		pSwitchMode->iStatus 	= eFW_SWITCH_COMPLETE;
		bRet = TRUE;
	}
	else {
		// √≥¿Ω ∫Œ∆√ Ω√ø°¥¬ ±‚∫ª Appl∑Œ ¡°«¡«“ ºˆ ¿÷µµ∑œ º≥¡§.

		// Firmware information ±∏¡∂√ºø° √ ±‚∞™¿ª ≥÷æÓ¡ÿ¥Ÿ.
		//memset(&g_FirmwareInfo, 0x00, sizeof(FIRMWARE_INFO));
		g_FirmwareInfo.nSignal = FIRMWARE_SIGNAL;

		// Bootloader ±‚∫ª ¡§∫∏ º≥¡§
		g_FirmwareInfo.AppProperty[eApp_Bootloader].nSignal = FIRMWARE_APP_SIGNAL;
		g_FirmwareInfo.AppProperty[eApp_Bootloader].wFirmwareSize = 100;//FIRMWARE_INFO_ADDRESS-BOOTLOADER_ADDRESS;
		g_FirmwareInfo.AppProperty[eApp_Bootloader].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
		memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Bootloader].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
		g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion = DEFAULT_FW_VERION;

		// /APP ±‚∫ª ¡§∫∏ º≥¡§
		g_FirmwareInfo.AppProperty[eApp_Application].nSignal = FIRMWARE_APP_SIGNAL;
		g_FirmwareInfo.AppProperty[eApp_Application].wFirmwareSize = 100;//UPDATE_TMP_ADDRESS - APPLICATION_ADDRESS;
		g_FirmwareInfo.AppProperty[eApp_Application].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
		memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Application].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
		g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion = DEFAULT_FW_VERION;

		// Master DB ±‚∫ª ¡§∫∏ º≥¡§
		g_FirmwareInfo.AppProperty[eApp_MasterDB].nSignal = FIRMWARE_APP_SIGNAL;
		g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
		g_FirmwareInfo.AppProperty[eApp_MasterDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
		memcpy((char*)g_FirmwareInfo.AppProperty[eApp_MasterDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
		g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion = DEFAULT_FW_VERION;

		// Slave DB ±‚∫ª ¡§∫∏ º≥¡§
		g_FirmwareInfo.AppProperty[eApp_SlaveDB].nSignal = FIRMWARE_APP_SIGNAL;
		g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize = 100;//APPLICATION_ADDRESS-CAR_SLAVE_ADDRESS;
		g_FirmwareInfo.AppProperty[eApp_SlaveDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
		memcpy((char*)g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
		g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion = DEFAULT_FW_VERION;

		// Driving DB ±‚∫ª ¡§∫∏ º≥¡§
		g_FirmwareInfo.AppProperty[eApp_ControlDB].nSignal = FIRMWARE_APP_SIGNAL;
		g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
		g_FirmwareInfo.AppProperty[eApp_ControlDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
		memcpy((char*)g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
		g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion = DEFAULT_FW_VERION;

		g_FirmwareInfo.SwitchingInfo.iApplMode = eApp_Application;
		g_FirmwareInfo.SwitchingInfo.iStatus = eFW_SWITCH_COMPLETE;

        // 2018/03/11 James Jean : æÓ¬˜«« ª˝ªÍΩ√¡°ø° false¿Ãπ«∑Œ √ ±‚»≠ ∞™µµ false∑Œ ∫Ø∞Ê
		g_FirmwareInfo.bModemActive = FALSE;

		memset((char*)&g_FirmwareInfo.m_strVIN,NULL,VIN_CODE_SIZE + 1);
		memcpy((char*)&g_FirmwareInfo.m_strVIN, STR_DEFAULT_VIN, sizeof(STR_DEFAULT_VIN));
		memcpy(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER);

        // added default service type with retail
        //printf("Dcs Service Type : %d\r\n",g_FirmwareInfo.nServiceType);
        g_FirmwareInfo.nServiceType = DCS_Retail;
        g_FirmwareInfo.nEncryptType = DCS_ENC_KMS;

		SetFirmwareInfo(&g_FirmwareInfo);

		bRet = TRUE;
	}

	//∫Œ∆√Ω√ ø©±‚ ∑Œ±◊¬Ô¥Ÿ ¡◊¥¬«ˆªÛ ∫ª¿˚¿÷æÓº≠ ¡˜¡¢ ¿‘∑¬¿∏∑Œ ∫Ø∞Ê
	printf("[GetSwitchingModeInfo] iApplMode %d, Change FW name APPLFW\r\n", g_FirmwareInfo.SwitchingInfo.iApplMode);
	if(g_FirmwareInfo.SwitchingInfo.iStatus == eFW_SWITCH_COMPLETE)
		printf("[GetSwitchingModeInfo] iStatus STR_FW_SWITCH_COMPLETE\r\n");
	else
		printf("[GetSwitchingModeInfo] iStatus STR_FW_SWITCH_READY\r\n");

	return bRet;
}



/*****************************END OF FILE****/
