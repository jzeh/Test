/* Includes ------------------------------------------------------------------*/
#include "Modem_comm.h"
#include "Modem_Manager.h"
#include "Power_Manager.h"
#include "OBD_Controller.h"
#include "AutolinkConfiguration.h"
#include "HdDebug.h"
#include "MngSystem.h"
#include "AutolinkConfig.h"
#include "Git_util.h"
#include "aes.h"
#include "base64_penta.h"
#include "MngSystemUtil.h"
#include "CanFD_RecvHandler.h"
#include "DebugHandler.h"
#include "SysHalFileSystem.h"

#include <time.h>

#include "HalHandler.h"
#if defined(USE_GIT_FAT_FS)
#include "ff.h"
#endif

#define Trace(...)  GITDebug(DEBUG_MODULES_APP,__VA_ARGS__)

extern BR_ModemInfo BkSram_ModemInfo;
extern BR_SystemInfo BkSram_SystemInfo;
#if defined(PROTOCOL17)
stURLInfo g_stTempURLInfo;	//서버에서 세팅시 내려온 URL을 임시로 담아놓는 곳
extern stServerUrl g_stServerUrl;
#endif
#define MAX_VIN_NUMBER 17

boolean_t m_bEnableKMSProcess = false;

//stAutolinkConfig m_stAutolinkConfig;
stAutolinkConfigData m_stAutolinkConfigData;

void WriteConfig(boolean_t bExportFuturekey, boolean_t bDisp);
void ReadConfig(boolean_t bDisp);
void WriteDefaultAutolinkConfigValue();
void TestFormat();
void DisplayConfigData(stAutolinkConfigData stConfig,boolean_t bDisp);
double Get_GPS_Lat_Origin();
double Get_GPS_Lon_Origin();
void Set_GPS_Lat_Origin(double lat);
void Set_GPS_Lon_Origin(double lon);

void DisplayAutolinkConfigData();
void LoadInitializeDamoKey(DukptFutureKeyInfo* pstFutureKey);

void DisplayBackupRamConfigData();


extern void printdump( char *str, unsigned char *data, size_t len );
extern bool GetModemActive();
extern void SetObdSetting(stAutolinkConfigData* pstAutolinkConfigData);

extern void SetAutolinkConfig2System(stAutolinkConfigData* pstAutolinkConfigData);
extern void GetAutolinkConfig2System(stAutolinkConfigData* pstAutolinkConfigData);
extern void DisplayGeofenceSetting();
extern void SendBlockingMessage(boolean_t bStop);
extern void WriteDefaultUserAction();
extern void SetModemActive(bool bActive);
extern void DisplayUserActionSetting(stUserActionSetting stUserActionSettingValue);

//extern unsigned char g_KMS_Publickey[];

#define STM32_UNIQUE_ID_ADDR_LENGTH				0x0C
const unsigned char m_carrIv[MAX_ENCRYPT_KEY_LENGTH]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

//void ConfirmPublicKey(void)
//{
//	unsigned int cnt=0;
//
//	Trace("\r\n======================== %s =======================\r\n",__FUNCTION__);
//	for(cnt=0; cnt<sizeof(KMS_Publickey);cnt++){
//		Trace("%c", KMS_Publickey[cnt]);
//	}
//	Trace("\r\n");
//}

void GetDefaultEncryptKey(char* pcarrKey,boolean_t bDefault)
{
//    uint8_t buff[16]={0};
#if defined(STM32F427X)
    uint32_t wAddress = 0x1FFF7A10;
#elif defined(AT32F435VMT7)
    uint32_t wAddress = 0x1FFFF7E8;
#endif

    // copy 12 bytes string to the key variables
    memcpy(pcarrKey,(char*)&(*(__IO uint32_t*)(wAddress)),STM32_UNIQUE_ID_ADDR_LENGTH);

    if( bDefault == true )
    {
        // ME change below code if ME added some property in configuration structure.
        // so ME power on at first time, then ME will be written default configuration value.
        // because decrypt key is changed with below code.
        pcarrKey[STM32_UNIQUE_ID_ADDR_LENGTH]='h';
        pcarrKey[STM32_UNIQUE_ID_ADDR_LENGTH+1]='m';
        pcarrKey[STM32_UNIQUE_ID_ADDR_LENGTH+2]='c';
        pcarrKey[STM32_UNIQUE_ID_ADDR_LENGTH+3]='c';

        return;
    }
    //for(int index=0; index<STM32_UNIQUE_ID_ADDR_LENGTH; index++)
    //    buff[index] = *(__IO uint32_t*)(wAddress+index);

    /************************************************************************************
    // do not modified below code because this code used that ME decrypt IPEK.
    ************************************************************************************/
    // add the string to remained 4 bytes.
    pcarrKey[STM32_UNIQUE_ID_ADDR_LENGTH]='h';
    pcarrKey[STM32_UNIQUE_ID_ADDR_LENGTH+1]='m';
    pcarrKey[STM32_UNIQUE_ID_ADDR_LENGTH+2]='c';
    pcarrKey[STM32_UNIQUE_ID_ADDR_LENGTH+3]='a';
    /***********************************************************************************/
}

// this fucntion write struct data to serail flash in // 007f f0000h - 007f ffffh
void WriteConfig(boolean_t bExportFuturekey, boolean_t bDisp)
{
    stAutolinkConfig stAutolink;
    //char carrBuf[CONFIG_READ_WRITE_SIZE] = {0,};
    unsigned char AutolinkEncryptKey[MAX_ENCRYPT_KEY_LENGTH] = {0};
    stHalRTCTypeDef stHalRtcDateTime;
    
    APP_TimeShow(&stHalRtcDateTime);    

    if( bExportFuturekey == true )
    {
        // add export future key
        if( DAMO_DUKPT_Export_Future_Key_Info(&m_stAutolinkConfigData.stFutureKey) != 0 )
        {
            //set up error for encryption
            // we should add recovery machanism
        }
    }

    // clear default write
    memset((char*)&stAutolink,0x00,sizeof(stAutolink));
    //git_disk_write(FAT_VOLUME_DRV, (BYTE*)carrBuf, SECTOR_CONFIG_STORAGE, 1);

    // getting system value
    GetAutolinkConfig2System(&m_stAutolinkConfigData);

    // get encrypt key
    GetDefaultEncryptKey((char *)AutolinkEncryptKey,true);

    // we will apply encryp process after we apply damo encryption process.
    int nEncryptResult = DAMO_CRYPT_AES_EncryptEx((unsigned char *)stAutolink.carrrEncryptData, (size_t*)&stAutolink.nEncryptSize,
        (const unsigned char*)&m_stAutolinkConfigData, sizeof(stAutolinkConfigData),
        AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, (unsigned char *)m_carrIv);

    if( nEncryptResult != 0 )
    {
        // erro encrypt
        Trace("Encrypt error : recovery\r\n");
    }

    stAutolink.nPreamble = 0xFE000617;

    if( (stAutolink.nEncryptSize + 8) > CONFIG_READ_WRITE_SIZE )
    {
        GIT_Assert(false,eErrorCodeApp|eMemoryLimit);
    }

    //sFLASH_WriteBuffer((BYTE*)&stAutolink, SECTOR_CONFIG_STORAGE*CONFIG_READ_WRITE_SIZE,CONFIG_READ_WRITE_SIZE);
    git_disk_write(FAT_VOLUME_DRV, (BYTE*)&stAutolink, SECTOR_CONFIG_STORAGE, 1);

    DisplayConfigData(m_stAutolinkConfigData,bDisp);
}

void LoadInitializeDamoKey(DukptFutureKeyInfo* pstFutureKey)
{
    int nResult;
    unsigned char ipek[16]={0,};
    int nSizeofIpek;
    unsigned char ksn[11]={0,};
    unsigned char unKeyofEncryptedIpek[16]={0,};
    unsigned char ucOpenSerial[SIZE_SERIAL_NUMBER+1];
    unsigned char ucEncryptedIpek[32];
    //unsigned char *pin="1234";
    //unsigned char *account_num="4012345678909";
    DukptFutureKeyInfo stFki;

    //int flags = DAMO_DUKPT_FLAG_USE_ALL_KEY;

    // get serial number for ksn
    memcpy(&ucOpenSerial[0],GetFWSerialNumber(), SIZE_SERIAL_NUMBER);
    Trace("Serial : %s\r\n",ucOpenSerial);

    // get default encrypt key of ipek from manufacturing

    // 2018-04-02 생산프로그램 Serial Write와 맞추기 위해 falase로 변경
	//GetDefaultEncryptKey((char*)unKeyofEncryptedIpek,true);
	GetDefaultEncryptKey((char*)unKeyofEncryptedIpek,false);
    Trace("key of encryptedIpek :\r\n");hexdump(unKeyofEncryptedIpek,16);

    // get an encrypted key with default key
    GetModuleKey((char*)ucEncryptedIpek);
    Trace("encrypted Ipek :\r\n");hexdump(ucEncryptedIpek,32);

    // decrypt the ipek
    nResult = DAMO_CRYPT_AES_DecryptEx(ipek,(size_t*)&nSizeofIpek,
                                ucEncryptedIpek, sizeof(ucEncryptedIpek),
                                unKeyofEncryptedIpek, sizeof(unKeyofEncryptedIpek), AES_128, CBC_MODE, (unsigned char *)m_carrIv);

    if( nResult != 0 )
    {
//#warning " ME need to request new master key because ipek is broken."
        // ME need to request new master key because ipek is broken.
        Trace("IPEK is broken, we must request new IPEK from server.\r\n");

        SetRequestIPEK();

        return;
    }

    if( nSizeofIpek <= sizeof( ipek ) )
	{
        Trace("ipek :\r\n");hexdump(ipek, nSizeofIpek);
    }

    // make ksn from serial number
    // ksn format [ XXXXXXXXYYY ]
    // X [0:7]  : serial number from 0 to 7
    // Y [8:11] : set 0 all bit
    // example
    // serial number : R0000001AUH0000
    // ksn : R0000001000
    memset(ksn,'0',sizeof(ksn));
    memcpy(ksn,ucOpenSerial,8);
    Trace("ksn :\r\n");hexdump(ksn,11);

    // initalize kms system library with ipek(master key)
    nResult = DAMO_DUKPT_Load_Initial_Key(ipek, nSizeofIpek, ksn, sizeof(ksn));

    if( nResult != 0 )
    {
        // critical error
        Trace("KMS System will not work because ipek couldn't apply to the library\r\n");
        Trace("Error code : %d\r\n",nResult);

        return;
    }

    // export future key to store in the autolinkconfiguration area.
    if( DAMO_DUKPT_Export_Future_Key_Info(&stFki) < 0 )
    {
        Trace("Error!!! Exporting Future Key Info!!!\r\n\r\n");
        return;
    }

#if true
    Trace("\n");
    Trace("Exporting Future Key Info!!!\r\n");
    Trace("Base64 Encoded KSN : %s\n", stFki.key_serial_number);
    for(int i=0; i<21; i++)
        Trace("Base64 Encoded %d-Future Key : %s\r\n", i, stFki.future_key_register[i]);
    Trace("\r\n");
#endif

    // set future key to the liblrary
    if(DAMO_DUKPT_Import_Future_Key_Info(&stFki)<0)
    {
        Trace("====================================================\r\n");
        Trace("error : import of penta security library.\r\n");
    }

    // save future key to use in the system whenever the device reboot.
    memcpy((char*)pstFutureKey,(char*)&stFki,sizeof(DukptFutureKeyInfo));

    Trace("Success to generate default future key with ipek\r\n");
}

boolean_t IsAvailableVin(char* pcarrVin)
{
    for( int i=0; i<strlen(pcarrVin); i++)
    //for( int i=0; i<MAX_REAL_VIN_SIZE; i++)
    {
        if( ((pcarrVin[i] < '0' )||(pcarrVin[i] > '9')) &&
            ((pcarrVin[i] < 'a') || (pcarrVin[i] > 'z')) &&
            ((pcarrVin[i] < 'A') || (pcarrVin[i] > 'Z')) )
        {
            // not available vin number
            return false;
        }
    }

    return true;
}

void WriteDefaultURL()
{
#if 0
    char * carrSeriaNumber = GetFWSerialNumber();
	DCSServiceType nServiceType;

    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);

	GetFirmwareInfo(&g_FirmwareInfo);
	memset(&g_FirmwareInfo.m_stURLInfo,0x00,sizeof(g_FirmwareInfo.m_stURLInfo));
	SetFirmwareInfo(&g_FirmwareInfo);

	memset((char*)&g_stServerUrl,0,sizeof(g_stServerUrl));

	if( strstr(carrSeriaNumber,"KRH") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_DEV_MESSAGE_URL, 	sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL));
		else											memcpy(g_stServerUrl.Url,	KR_HTTP_RETAIL_DEV_MESSAGE_URL,	sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL));
	}
	else if( strstr(carrSeriaNumber,"KRK") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_DEV_MESSAGE_URL_KIA, 	sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL_KIA));
		else											memcpy(g_stServerUrl.Url,	KR_HTTP_RETAIL_DEV_MESSAGE_URL_KIA,	sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL_KIA));
	}
	else if( strstr(carrSeriaNumber,"AUH") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	AU_HTTP_FLEET_MESSAGE_URL, 	sizeof(AU_HTTP_FLEET_MESSAGE_URL));
		else											memcpy(g_stServerUrl.Url,	AU_HTTP_RETAIL_MESSAGE_URL,	sizeof(AU_HTTP_RETAIL_MESSAGE_URL));
	}
	else if( strstr(carrSeriaNumber,"AUK") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	AU_HTTP_RETAIL_MESSAGE_URL_KIA,       		sizeof(AU_HTTP_RETAIL_MESSAGE_URL_KIA));
		else											memcpy(g_stServerUrl.Url,	AU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA,		sizeof(AU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA));
	}
	else if( strstr(carrSeriaNumber,"NZH") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	NZ_HTTP_FLEET_MESSAGE_URL, 						sizeof(NZ_HTTP_FLEET_MESSAGE_URL));
		else											memcpy(g_stServerUrl.Url,	NZ_HTTP_RETAIL_MESSAGE_URL,						sizeof(NZ_HTTP_RETAIL_MESSAGE_URL));
	}
	else if( strstr(carrSeriaNumber,"NZK") != NULL )
	{
		memcpy(g_stServerUrl.Url,	NZ_HTTP_RETAIL_MESSAGE_URL,						sizeof(NZ_HTTP_RETAIL_MESSAGE_URL));
	}
	else
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_DEV_MESSAGE_URL, 	sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL));
		else											memcpy(g_stServerUrl.Url,	KR_HTTP_RETAIL_DEV_MESSAGE_URL,	sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL));
	}

	memcpy(g_stServerUrl.Path,	HTTP__MESSAGE_PATH,	sizeof(HTTP__MESSAGE_PATH));
#else
	char * carrSeriaNumber = GetFWSerialNumber();
	DCSServiceType nServiceType;
	unsigned char AutolinkEncryptKey[MAX_ENCRYPT_KEY_LENGTH] = {0};
	stURLInfo stTemp;
	unsigned char strTemp[MAX_SERVER_URL_LENGTH+8+1]={0,};
	int nEncryptResult=0;

    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);

	GetFirmwareInfo(&g_FirmwareInfo);
	memset(&g_FirmwareInfo.m_stURLInfo,0x00,sizeof(g_FirmwareInfo.m_stURLInfo));

	memset((char*)&g_stServerUrl,0,sizeof(g_stServerUrl));

	if( strstr(carrSeriaNumber,"KRH") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url, KR_HTTP_FLEET_DEV_MESSAGE_URL, sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL));
		else							memcpy(g_stServerUrl.Url, KR_HTTP_RETAIL_DEV_MESSAGE_URL, sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL));
	}
	else if( strstr(carrSeriaNumber,"KRK") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url, KR_HTTP_FLEET_DEV_MESSAGE_URL_KIA, sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL_KIA));
		else							memcpy(g_stServerUrl.Url, KR_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL_KIA));
	}
	else if( strstr(carrSeriaNumber,"AUH") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	AU_HTTP_FLEET_MESSAGE_URL, 	sizeof(AU_HTTP_FLEET_MESSAGE_URL));
		else											memcpy(g_stServerUrl.Url, AU_HTTP_RETAIL_MESSAGE_URL, sizeof(AU_HTTP_RETAIL_MESSAGE_URL));
	}
	else if( strstr(carrSeriaNumber,"AUK") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url, AU_HTTP_RETAIL_MESSAGE_URL_KIA, sizeof(AU_HTTP_RETAIL_MESSAGE_URL_KIA));
		else							memcpy(g_stServerUrl.Url, AU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA,		sizeof(AU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA));
	}
    else if( strstr(carrSeriaNumber,"NZH_DEV") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	NZ_HTTP_FLEET_MESSAGE_URL,  sizeof(NZ_HTTP_FLEET_MESSAGE_URL));
		else							 memcpy(g_stServerUrl.Url,	NZ_HTTP_RETAIL_DEV_MESSAGE_URL, sizeof(NZ_HTTP_RETAIL_DEV_MESSAGE_URL));
	}
	else if( strstr(carrSeriaNumber,"NZH") != NULL )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	NZ_HTTP_FLEET_MESSAGE_URL,  sizeof(NZ_HTTP_FLEET_MESSAGE_URL));
		else							 memcpy(g_stServerUrl.Url,	NZ_HTTP_RETAIL_MESSAGE_URL, sizeof(NZ_HTTP_RETAIL_MESSAGE_URL));
	}
	else if( strstr(carrSeriaNumber,"NZK") != NULL )
	{
		memcpy(g_stServerUrl.Url,	NZ_HTTP_RETAIL_MESSAGE_URL, sizeof(NZ_HTTP_RETAIL_MESSAGE_URL));
	}
	else if( strstr(carrSeriaNumber,"VNH") != NULL )
	{
		memcpy(g_stServerUrl.Url, VN_HTTP_RETAIL_MESSAGE_URL_KIA,	sizeof(VN_HTTP_RETAIL_MESSAGE_URL_KIA));
	}
    else if( strstr(carrSeriaNumber,"VNK_DEV") != NULL )
	{
		memcpy(g_stServerUrl.Url, VN_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, sizeof(VN_HTTP_RETAIL_DEV_MESSAGE_URL_KIA));
	}
	else if( strstr(carrSeriaNumber,"VNK") != NULL )
	{
		memcpy(g_stServerUrl.Url, VN_HTTP_RETAIL_MESSAGE_URL_KIA, sizeof(VN_HTTP_RETAIL_MESSAGE_URL_KIA));
	}
	else if( strstr(carrSeriaNumber,"SGK") != NULL )
	{
		memcpy(g_stServerUrl.Url, SG_HTTP_RETAIL_MESSAGE_URL_KIA, sizeof(SG_HTTP_RETAIL_MESSAGE_URL_KIA));
	}
	else if( strstr(carrSeriaNumber,"RUK") != NULL )
	{
		memcpy(g_stServerUrl.Url, RU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, sizeof(RU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA));
	}
#if defined(GIT_FLEET)
	else if( strstr(carrSeriaNumber,"GFK") != NULL )
	{
	    if( nServiceType == DCS_Fleet ) memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_MESSAGE_URL_GIT,						sizeof(KR_HTTP_FLEET_MESSAGE_URL_GIT));
		else							memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_MESSAGE_URL_GIT,						sizeof(KR_HTTP_FLEET_MESSAGE_URL_GIT));
	}
	else if( strstr(carrSeriaNumber,"GFH") != NULL )
	{
		if( nServiceType == DCS_Fleet ) memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_MESSAGE_URL_GIT,						sizeof(KR_HTTP_FLEET_MESSAGE_URL_GIT));
		else							memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_MESSAGE_URL_GIT,						sizeof(KR_HTTP_FLEET_MESSAGE_URL_GIT));
	}
#endif
#if defined(QA_FIFA)
	else if( strstr(carrSeriaNumber,"QAH") != NULL)
	{
		memcpy(g_stServerUrl.Url, QA_HTTP_RETAIL_DEV_MESSAGE_URL, sizeof(QA_HTTP_RETAIL_DEV_MESSAGE_URL));
	}
#endif
	else
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_DEV_MESSAGE_URL, 	sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL));
		else											memcpy(g_stServerUrl.Url,	KR_HTTP_RETAIL_DEV_MESSAGE_URL,	sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL));
	}

	memcpy(g_stServerUrl.Path,	HTTP__MESSAGE_PATH,	sizeof(HTTP__MESSAGE_PATH));

	sprintf((char*)strTemp,"%s%s","https://",g_stServerUrl.Url);	//서버에서 내려주는 형식이 https 포함이라.... 수정하고싶지만 시간이..........

	memset((char*)&stTemp,0x00,sizeof(stTemp));
    GetDefaultEncryptKey((char *)AutolinkEncryptKey,true);

    // we will apply encryp process after we apply damo encryption process.
    nEncryptResult = DAMO_CRYPT_AES_EncryptEx((unsigned char *)stTemp.strUrl, (size_t*)&stTemp.nEncryptedLength,
        (const unsigned char*)&strTemp, MAX_SERVER_URL_LENGTH,
        AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, (unsigned char *)m_carrIv);

    if( nEncryptResult == 0 )	//if success
	{
		g_FirmwareInfo.m_stURLInfo.nEncryptedLength = stTemp.nEncryptedLength;
		g_FirmwareInfo.m_stURLInfo.nPreamble = 0xFE000728;
		memcpy(	g_FirmwareInfo.m_stURLInfo.strUrl,stTemp.strUrl,MAX_SERVER_URL_LENGTH+AES_PADDING_LENGTH);	//패딩포함
		SetFirmwareInfo(&g_FirmwareInfo);
	}
#endif
}

void WriteURL(char* code)
{
	DCSServiceType nServiceType;
	unsigned char AutolinkEncryptKey[MAX_ENCRYPT_KEY_LENGTH] = {0};
	stURLInfo stTemp;
	unsigned char strTemp[MAX_SERVER_URL_LENGTH+8+1]={0,};
	int nEncryptResult=0;

    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);

	GetFirmwareInfo(&g_FirmwareInfo);
	memset(&g_FirmwareInfo.m_stURLInfo,0x00,sizeof(g_FirmwareInfo.m_stURLInfo));

	memset((char*)&g_stServerUrl,0,sizeof(g_stServerUrl));

	if( strcmp(code,"KRH") == 0 )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_DEV_MESSAGE_URL, 	sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL));
		else							memcpy(g_stServerUrl.Url,	KR_HTTP_RETAIL_DEV_MESSAGE_URL,	sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL));
	}
	else if( strcmp(code,"KRK") == 0 )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_DEV_MESSAGE_URL_KIA, 	sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL_KIA));
		else							memcpy(g_stServerUrl.Url,	KR_HTTP_RETAIL_DEV_MESSAGE_URL_KIA,	sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL_KIA));
	}
	else if( strcmp(code,"AUH") == 0 )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	AU_HTTP_FLEET_MESSAGE_URL, 	sizeof(AU_HTTP_FLEET_MESSAGE_URL));
		else							memcpy(g_stServerUrl.Url,	AU_HTTP_RETAIL_MESSAGE_URL,	sizeof(AU_HTTP_RETAIL_MESSAGE_URL));
	}
	else if( strcmp(code,"AUK") == 0 )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	AU_HTTP_FLEET_MESSAGE_URL_KIA,      sizeof(AU_HTTP_FLEET_MESSAGE_URL_KIA));
		else							memcpy(g_stServerUrl.Url,	AU_HTTP_RETAIL_MESSAGE_URL_KIA,		sizeof(AU_HTTP_RETAIL_MESSAGE_URL_KIA));
	}
    else if( strcmp(code,"NZH_DEV") == 0 )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	NZ_HTTP_FLEET_MESSAGE_URL,          sizeof(NZ_HTTP_FLEET_MESSAGE_URL));
		else							memcpy(g_stServerUrl.Url,	NZ_HTTP_RETAIL_DEV_MESSAGE_URL,		sizeof(NZ_HTTP_RETAIL_DEV_MESSAGE_URL));
	}
	else if( strcmp(code,"NZH") == 0 )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	NZ_HTTP_FLEET_MESSAGE_URL,          sizeof(NZ_HTTP_FLEET_MESSAGE_URL));
		else							memcpy(g_stServerUrl.Url,	NZ_HTTP_RETAIL_MESSAGE_URL,	        sizeof(NZ_HTTP_RETAIL_MESSAGE_URL));
	}
	else if( strcmp(code,"NZK") == 0 )
	{
		memcpy(g_stServerUrl.Url, NZ_HTTP_RETAIL_MESSAGE_URL, sizeof(NZ_HTTP_RETAIL_MESSAGE_URL));
	}
	else if( strcmp(code,"VNH") == 0 )
	{
		memcpy(g_stServerUrl.Url, VN_HTTP_RETAIL_MESSAGE_URL_KIA, sizeof(VN_HTTP_RETAIL_MESSAGE_URL_KIA));
	}
	else if( strcmp(code,"VNKD") == 0 )
	{
		memcpy(g_stServerUrl.Url, VN_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, sizeof(VN_HTTP_RETAIL_DEV_MESSAGE_URL_KIA));
	}
	else if( strcmp(code,"VNK") == 0 )
	{
		memcpy(g_stServerUrl.Url, VN_HTTP_RETAIL_MESSAGE_URL_KIA, sizeof(VN_HTTP_RETAIL_MESSAGE_URL_KIA));
	}
    else if( strcmp(code, "SGK_DEV") == 0)
    {
        memcpy(g_stServerUrl.Url, SG_HTTP_DEV_MESSAGE_URL_KIA,	 sizeof(SG_HTTP_DEV_MESSAGE_URL_KIA));
    }
    else if(strcmp(code, "SGKD") == 0)
	{
        memcpy(g_stServerUrl.Url, SG_HTTP_RETAIL_DEV_MESSAGE_URL_KIA, sizeof(SG_HTTP_RETAIL_DEV_MESSAGE_URL_KIA));
    }
    else if( strcmp(code, "SGK") == 0)
    {
        memcpy(g_stServerUrl.Url, SG_HTTP_RETAIL_MESSAGE_URL_KIA,	 sizeof(SG_HTTP_RETAIL_MESSAGE_URL_KIA));
    }
	else if(strcmp(code, "SGKD2") == 0)
	{
		memcpy(g_stServerUrl.Url, SG_HTTP_RETAIL_DEV2_MESSAGE_URL_KIA, sizeof(SG_HTTP_RETAIL_DEV2_MESSAGE_URL_KIA));
	}
	else if( strcmp(code,"KR2") == 0 )
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_DEV_MESSAGE_URL, 	sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL));
		else											memcpy(g_stServerUrl.Url,	KR_HTTP_RETAIL_DEV_MESSAGE_URL2,	sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL2));
	}
	else
	{
		if( nServiceType == DCS_Fleet )	memcpy(g_stServerUrl.Url,	KR_HTTP_FLEET_DEV_MESSAGE_URL, 	sizeof(KR_HTTP_FLEET_DEV_MESSAGE_URL));
		else											memcpy(g_stServerUrl.Url,	KR_HTTP_RETAIL_DEV_MESSAGE_URL,	sizeof(KR_HTTP_RETAIL_DEV_MESSAGE_URL));
	}

	printf("SetURL : %s\r\n",g_stServerUrl.Url);

	memcpy(g_stServerUrl.Path,	HTTP__MESSAGE_PATH,	sizeof(HTTP__MESSAGE_PATH));

	sprintf((char*)strTemp,"%s%s","https://",g_stServerUrl.Url);	//서버에서 내려주는 형식이 https 포함이라.... 수정하고싶지만 시간이..........

	memset((char*)&stTemp,0x00,sizeof(stTemp));
    GetDefaultEncryptKey((char *)AutolinkEncryptKey,true);

    // we will apply encryp process after we apply damo encryption process.
    nEncryptResult = DAMO_CRYPT_AES_EncryptEx((unsigned char *)stTemp.strUrl, (size_t*)&stTemp.nEncryptedLength,
        (const unsigned char*)&strTemp, MAX_SERVER_URL_LENGTH,
        AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, (unsigned char *)m_carrIv);

    if( nEncryptResult == 0 )	//if success
	{
		g_FirmwareInfo.m_stURLInfo.nEncryptedLength = stTemp.nEncryptedLength;
		g_FirmwareInfo.m_stURLInfo.nPreamble = 0xFE000728;
		memcpy(	g_FirmwareInfo.m_stURLInfo.strUrl,stTemp.strUrl,MAX_SERVER_URL_LENGTH+AES_PADDING_LENGTH);	//패딩포함
		SetFirmwareInfo(&g_FirmwareInfo);
	}
}

void ConfigureVin()
{
    // 1. use default vin for manufacturing
    // 1-1. if default vin do nothing in this system.
    // 1-2. wait for an available vin
    // 2. at the first getting the available vin then set the vin all configuration.
    // 2-1. allow the system to send data to the server.
    // 3. if recevied another vin, not allows use this module in this car.
    // ( this is test scenario if decided a policy about vin. we must change the procedure. )

#ifndef ENABLE_DEV_VIN
    // check default vin or not
    if( memcmp(m_stAutolinkConfigData.stSystem.carrVin, STR_DEFAULT_VIN,strlen(STR_DEFAULT_VIN)) == 0 )
    {
        //notify to all manager do nothing because default vin
        Trace("IMPORTANT : default vin is used, do nothing\r\n");

        // send blocking an event to all manager
        SendBlockingMessage(true);
    }

    if( IsAvailableVin((char *)m_stAutolinkConfigData.stSystem.carrVin) == false )
    {
        Trace("IMPORTANT : vin number is broken, set the default vin\r\n");
        memcpy((char *)m_stAutolinkConfigData.stSystem.carrVin, (char *)g_FirmwareInfo.m_strVIN, sizeof(g_FirmwareInfo.m_strVIN));
        //memcpy(g_FirmwareInfo.m_strVIN, m_stAutolinkConfigData.carrVin, sizeof(g_FirmwareInfo.m_strVIN));
    }
#else //#ifndef ENABLE_DEV_VIN
	memcpy(m_stAutolinkConfigData.stSystem.carrVin,STR_DEFAULT_DEV_VIN, sizeof(STR_DEFAULT_DEV_VIN));
	memcpy(g_FirmwareInfo.m_strVIN,m_stAutolinkConfigData.stSystem.carrVin, sizeof(g_FirmwareInfo.m_strVIN));
#endif //#ifndef ENABLE_DEV_VIN
}

void ReadConfig(boolean_t bDisp)
{
    stAutolinkConfig stAutolink;
    //char carrBuf[CONFIG_READ_WRITE_SIZE] = {0,};
    unsigned char AutolinkEncryptKey[MAX_ENCRYPT_KEY_LENGTH] = {0};
    size_t nDecryptSize;
#if defined(PROTOCOL17)
	stURLInfo stTemp;
#endif

    // get encrypt key
    GetDefaultEncryptKey((char *)AutolinkEncryptKey,true);

    memset((char*)&m_stAutolinkConfigData,0x00,sizeof(m_stAutolinkConfigData));

    //git_disk_read(FAT_VOLUME_DRV, (BYTE*)carrBuf, SECTOR_CONFIG_STORAGE, 1);
    //git_disk_read(FAT_VOLUME_DRV, (BYTE*)&stAutolink, SECTOR_CONFIG_STORAGE, 1);
	sFLASH_ReadBuffer((uint8_t*)&stAutolink, SECTOR_CONFIG_STORAGE * SFLASH_SECTOR_SIZE, sizeof(stAutolink));
    //memcpy(&m_stAutolinkConfig,carrBuf,sizeof(stAutolinkConfig));

    if( stAutolink.nEncryptSize > SECTOR_CONFIG_STORAGE )
    {
        // error data is not correct
    }

    // we will apply encryp process after we apply damo encryption process.
    int nEncryptResult = DAMO_CRYPT_AES_DecryptEx((unsigned char *)&m_stAutolinkConfigData, &nDecryptSize,
    (const unsigned char *)stAutolink.carrrEncryptData, stAutolink.nEncryptSize,
    AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, (unsigned char *)m_carrIv);

    if( nEncryptResult != 0 )
    {
        // recovery configuration data
        Trace("IMPORTANT : Decrypt error : recovery\r\n");
        Trace("Write DefaultAutolinkConfiguration\r\n");
        WriteDefaultAutolinkConfigValue();
    }

    if( stAutolink.nPreamble != 0xFE000617 )
    {
        // data not correct
        Trace("read config : not match preamble\r\n");
        WriteDefaultAutolinkConfigValue();
    }

#if defined(PROTOCOL17)
	memset(&stTemp,0x00,sizeof(stTemp));

	if( g_FirmwareInfo.m_stURLInfo.nEncryptedLength   < (MAX_SERVER_URL_LENGTH+8+1) )
	{
		nEncryptResult = DAMO_CRYPT_AES_DecryptEx((unsigned char *)&stTemp.strUrl, (size_t*)&stTemp.nLength,
													(const unsigned char *)g_FirmwareInfo.m_stURLInfo.strUrl, g_FirmwareInfo.m_stURLInfo.nEncryptedLength,
													AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, (unsigned char *)m_carrIv);
		stTemp.nLength = strlen((char*)stTemp.strUrl);

		memset((char*)&g_stServerUrl,0,sizeof(g_stServerUrl));
	}
	else
	{
		printf("%s] Error Url Info\r\n",__FUNCTION__);
		// recovery configuration data
		WriteDefaultURL();
	}

	if( (nEncryptResult != 0) || (g_FirmwareInfo.m_stURLInfo.nPreamble != 0xFE000728) )
    {
        // recovery configuration data
		WriteDefaultURL();
    }
	else
	{
		memcpy(g_stServerUrl.Url,&stTemp.strUrl[URL_START_POSITION],stTemp.nLength-URL_START_POSITION);
		memcpy(g_stServerUrl.Path,	HTTP__MESSAGE_PATH,	sizeof(HTTP__MESSAGE_PATH));
	}
	Trace(">>>>> URL : %s\r\n",g_stServerUrl.Url);
    Trace(">>>>> Path : %s\r\n",g_stServerUrl.Path);
#endif

    // set future key to the liblrary
    if(DAMO_DUKPT_Import_Future_Key_Info(&m_stAutolinkConfigData.stFutureKey)<0)
    {
        Trace("====================================================\r\n");
        Trace("error : import of penta security library.\r\n");
        //set up error for encryption
        // we should add recovery machanism

        // try recovery future key of penta from ipek that stored in the internal flash
        LoadInitializeDamoKey(&m_stAutolinkConfigData.stFutureKey);
        if(DAMO_DUKPT_Import_Future_Key_Info(&m_stAutolinkConfigData.stFutureKey)<0)
        {
//#warning "// we should run a process to get ipek from server."
            // we should run a process to get ipek from server.
            Trace("====================================================\r\n");
            printf("IMPORTANT : error : import of penta security library.\r\n");
            Trace("we should run a process to get an ipek from server\r\n");
            SetRequestIPEK();
        }
    }

#if false
    Trace("\r\n");
    Trace("Exporting Future Key Info!!!\r\n");
    Trace("Base64 Encoded KSN : %s\r\n", m_stAutolinkConfigData.stFutureKey.key_serial_number);
    for(int i=0; i<21; i++)
        Trace("Base64 Encoded %d-Future Key : %s\r\n", i, m_stAutolinkConfigData.stFutureKey.future_key_register[i]);
    Trace("\r\n");
#endif

    // setting vin number according to the definition and scenario
    ConfigureVin();

    // initialize all setting to system
    SetAutolinkConfig2System(&m_stAutolinkConfigData);

    DisplayConfigData(m_stAutolinkConfigData,bDisp);

    // Display backup ram info
    DisplayBackupRamConfigData();

    // allow to get next pin entry for encrypt data.
    m_bEnableKMSProcess = true;
}

void WriteDefaultAutolinkConfigValue()
{
    //char carrBuf[CONFIG_READ_WRITE_SIZE] = {0,};
    memset((char*)&m_stAutolinkConfigData,0,sizeof(m_stAutolinkConfigData));

    // clear configuration write to serial flash
    //git_disk_write(FAT_VOLUME_DRV, (BYTE*)stAutolinkConfig, SECTOR_CONFIG_STORAGE, 1);

    // initialize future key with encrypted ipek that stored in the internal flash.
    LoadInitializeDamoKey(&m_stAutolinkConfigData.stFutureKey);

#ifndef ENABLE_DEV_VIN
    // there is no vin information then write default vin
    //sprintf((char *)m_stAutolinkConfigData.stSystem.carrVin,"%s\x00",g_FirmwareInfo.m_strVIN);
	memcpy((char *)m_stAutolinkConfigData.stSystem.carrVin,g_FirmwareInfo.m_strVIN,sizeof(g_FirmwareInfo.m_strVIN));
#else
    sprintf(m_stAutolinkConfigData.stSystem.carrVin,"%s\x00",STR_DEFAULT_DEV_VIN);
    memcpy(g_FirmwareInfo.m_strVIN,m_stAutolinkConfigData.stSystem.carrVin, sizeof(g_FirmwareInfo.m_strVIN));
#endif
    memset((char *)m_stAutolinkConfigData.stSystem.carrCellPhoneNum,0,MAX_PHONE_NUMBER_SIZE);
    m_stAutolinkConfigData.stSystem.bModemActive = GetModemActive();
    m_stAutolinkConfigData.stSystem.dlLastLatitude = Get_GPS_Lat_Origin();
    m_stAutolinkConfigData.stSystem.dlLastLongitude = Get_GPS_Lon_Origin();
    m_stAutolinkConfigData.stSystem.unOdometer = 0;
    m_stAutolinkConfigData.stSystem.bSystemReset = 0;
    m_stAutolinkConfigData.stSystem.usSystemResetCount = 0;
    // default 0 : not allow new vin / 1 : allow new vin
    m_stAutolinkConfigData.stSystem.cAllowNewVin = 0;
    m_stAutolinkConfigData.stSystem.nServiceType = GetServiceType();
    m_stAutolinkConfigData.stSystem.nEncryptType = GetEncryptType();

	memset((char*)&m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo,0,sizeof(stCANFDBoardUpdateInfo));

    // clear user action config
    memset((char*)&m_stAutolinkConfigData.stUserAction,0,sizeof(stUserActionSetting));

    WriteDefaultUserAction();

    // clear system properties related with autoconfiguration.
    SetObdSetting(&m_stAutolinkConfigData);

    WriteConfig(false, false);

    // clear ram properties
    ClearSectionBackupRAM();
}


void SetAutolinkConfigProperty(uint8_t cIndex,void* pvValue)
{
    switch(cIndex)
    {
        case eAutoLinkConfig_Vin:
            memset(m_stAutolinkConfigData.stSystem.carrVin,0,sizeof(m_stAutolinkConfigData.stSystem.carrVin));
            memcpy(m_stAutolinkConfigData.stSystem.carrVin,(char*)pvValue,MAX_AUTOCONFIG_VIN_SIZE);
            break;
        case eAutoLinkConfig_LastLatitude:
            m_stAutolinkConfigData.stSystem.dlLastLatitude = *((double*)pvValue);
            break;
        case eAutoLinkConfig_LastLongitude:
            m_stAutolinkConfigData.stSystem.dlLastLongitude = *((double*)pvValue);
            break;
        case eAutoLinkConfig_ModemActive:
            {
                boolean_t bActive = false;
                // when modem active is changed,
                // we use internal flash storage because it is safe better then serial flash
                m_stAutolinkConfigData.stSystem.bModemActive = *((uint8_t*)pvValue);
                if( *(uint8_t*)pvValue ==  0 )
                {
                    bActive = false;

                }
                else if( *(uint8_t*)pvValue ==  1 )
                {
                    bActive = true;
                }

                // save the data to internal flash
                SetModemActive(bActive);

                // send to the message / storage manager to control the data
                SendBlockingMessage(!bActive);
            }
            break;
        case eAutoLinkConfig_CellPhone:
            strcpy((char *)m_stAutolinkConfigData.stSystem.carrCellPhoneNum,(char*)pvValue);
            break;
        case eAutoLinkConfig_SystemResetFlag:
            m_stAutolinkConfigData.stSystem.bSystemReset = *((uint8_t*)pvValue);
            break;
        case eAutoLinkConfig_SystemResetCount:
            m_stAutolinkConfigData.stSystem.usSystemResetCount = *((uint16_t*)pvValue);
            break;
        case eAutoLinkConfig_AllowNewVin:
            m_stAutolinkConfigData.stSystem.cAllowNewVin = *((uint8_t*)pvValue);
            break;
        case eAutoLinkConfig_ServiceType:
            {
                DCSServiceType nServiceType = *((DCSServiceType*)pvValue);

                if( nServiceType < DCS_Retail || nServiceType >= DCS_MAX)
                {
                    Trace("Not defined service type\r\n");
                    SetServiceType(DCS_Retail);
                    m_stAutolinkConfigData.stSystem.nServiceType = DCS_Retail;//*((DCSServiceType*)pvValue);
                }
                else
                {
                    SetServiceType(nServiceType);
                    m_stAutolinkConfigData.stSystem.nServiceType = nServiceType;//*((DCSServiceType*)pvValue);
                }

#if defined(PROTOCOL15)
            	if( nServiceType == DCS_Retail )
                    Uart8_Baudrate_Set(115200);
                if( nServiceType == DCS_Fleet )
#if defined(FEATURE_EXTENSION_BOARD)
					Uart8_Baudrate_Set(115200);
#else
                    Uart8_Baudrate_Set(9600);
#endif
#endif
            }
            break;
        case eAutoLinkConfig_EncryptType:
            {
                DCSEncryptType nEncryptType = *((DCSEncryptType*)pvValue);

                if( nEncryptType < DCS_ENC_AES || nEncryptType >= DCS_ENC_MAX )
                {
                    Trace("Not defined encrypt type\r\n");
                    SetEncryptType(DCS_ENC_KMS);
                    m_stAutolinkConfigData.stSystem.nEncryptType = *((DCSEncryptType*)pvValue);
                }
                else
                {
                    SetEncryptType(nEncryptType);
                    m_stAutolinkConfigData.stSystem.nEncryptType = *((DCSEncryptType*)pvValue);
                }
            }
        case eAutoLinkConfig_NetworkInfo:
            memcpy((char*)&m_stAutolinkConfigData.stSystem.stNetworkInfo,(char*)pvValue,sizeof(stNetworkTime));
            break;
        case eAutoLinkConfig_SensorInfo:
            memcpy((char*)&m_stAutolinkConfigData.stSystem.stGyroAngle,(char*)pvValue,sizeof(stSensorInfo));
            break;
		case eAutoLinkConfig_FDBoardUpdateInfo:
			memcpy((char*)&m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo,(char*)pvValue,sizeof(stCANFDBoardUpdateInfo));
			break;
		case eAutoLinkConfig_Fahrenheit:
			m_stAutolinkConfigData.stSystem.bFahrenheit = *((uint8_t*)pvValue);
			break;
		case eAutoLinkConfig_PACVType:
			m_stAutolinkConfigData.stCANInfo.ucPACVType = *((uint8_t*)pvValue);
			break;
		case eAutoLinkConfig_AutovinCANLine:
			m_stAutolinkConfigData.stCANInfo.ucAutovinCANLine = *((uint8_t*)pvValue);
			break;
        default:
            Trace("Error Not Defined SetAutolinkConfigProperty\r\n");
            break;
    }

    WriteConfig(false, false);
}

void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue)
{
    switch(cIndex)
    {
        case eAutoLinkConfig_Vin:
#ifdef FIX_VIN
			memcpy((char*)pvValue,FIX_VIN_NUMBER,MAX_AUTOCONFIG_VIN_SIZE);
#else
            memcpy((char*)pvValue,m_stAutolinkConfigData.stSystem.carrVin,MAX_AUTOCONFIG_VIN_SIZE);
#endif
            break;
        case eAutoLinkConfig_LastLatitude:
             *((double*)pvValue) = m_stAutolinkConfigData.stSystem.dlLastLatitude;
            break;
        case eAutoLinkConfig_LastLongitude:
             *((double*)pvValue) = m_stAutolinkConfigData.stSystem.dlLastLongitude;
            break;
        case eAutoLinkConfig_ModemActive:
            {
                boolean_t bActive;
                bActive = GetModemActive();
                *((uint8_t*)pvValue) = bActive;
            }
            break;
        case eAutoLinkConfig_CellPhone:
            strcpy((char*)pvValue, (char*)m_stAutolinkConfigData.stSystem.carrCellPhoneNum);
            break;
        case eAutoLinkConfig_SystemResetFlag:
            *((uint8_t*)pvValue) = m_stAutolinkConfigData.stSystem.bSystemReset;
            break;
        case eAutoLinkConfig_SystemResetCount:
             *((uint16_t*)pvValue) = m_stAutolinkConfigData.stSystem.usSystemResetCount;
            break;
        case eAutoLinkConfig_AllowNewVin:
            *((uint8_t*)pvValue) = m_stAutolinkConfigData.stSystem.cAllowNewVin;
            break;
        case eAutoLinkConfig_ServiceType:
            {
                DCSServiceType nServiceType = GetServiceType();
                *((DCSServiceType*)pvValue) = nServiceType;
                //*((uint8_t*)pvValue) = m_stAutolinkConfigData.stSystem.nServiceType;
            }
            break;
        case eAutoLinkConfig_EncryptType:
            {
                DCSEncryptType nEncryptType = GetEncryptType();
                *((DCSEncryptType*)pvValue) = nEncryptType;
                //*((uint8_t*)pvValue) = m_stAutolinkConfigData.stSystem.nEncryptType;
            }
            break;
        case eAutoLinkConfig_NetworkInfo:
            memcpy((char*)pvValue,(char*)&m_stAutolinkConfigData.stSystem.stNetworkInfo,sizeof(stNetworkTime));
            break;
        case eAutoLinkConfig_SensorInfo:
            memcpy((char*)pvValue,(char*)&m_stAutolinkConfigData.stSystem.stGyroAngle,sizeof(stSensorInfo));
            break;
		case eAutoLinkConfig_FDBoardUpdateInfo:
		  	memcpy((char*)pvValue,(char*)&m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo,sizeof(stCANFDBoardUpdateInfo));
			break;
		case eAutoLinkConfig_Fahrenheit:
			*((uint8_t*)pvValue) = m_stAutolinkConfigData.stSystem.bFahrenheit;
			break;
		case eAutoLinkConfig_PACVType:
			*((uint8_t*)pvValue) = m_stAutolinkConfigData.stCANInfo.ucPACVType;
			break;
		case eAutoLinkConfig_AutovinCANLine:
			*((uint8_t*)pvValue) = m_stAutolinkConfigData.stCANInfo.ucAutovinCANLine;
			break;
        default:
            Trace("Error Not Defined GetAutolinkConfigProperty\r\n");
            break;
    }
}

void SetBackupRamConfigProperty(uint8_t cIndex,void* pvValue)
{
    switch(cIndex)
    {
        case eBackupRamConfig_Apn:
            memset(BkSram_ModemInfo.carrDefaultAPN,0,sizeof(BkSram_ModemInfo.carrDefaultAPN));
            memcpy(BkSram_ModemInfo.carrDefaultAPN,(char*)pvValue,MAX_AUTOCONFIG_APN_SIZE);
            break;
        case eBackupRamConfig_DrivingInterval:
            BkSram_SystemInfo.unDrivingInterval = *((uint32_t*)pvValue);
            break;
        case eBackupRamConfig_WakeupInterval:
            BkSram_SystemInfo.unWakeUpInterval = *((uint32_t*)pvValue);
            break;
        case eBackupRamConfig_SystemTimeout:
            BkSram_SystemInfo.unSystemTimeOut = *((uint32_t*)pvValue);
            break;
        case eBackupRamConfig_FotaInterval:
            BkSram_SystemInfo.unFotaInterval = *((uint32_t*)pvValue);
            break;
        case eBackupRamConfig_NetworkInfo:
            memcpy((char*)&BkSram_ModemInfo.stNetworkInfo,(char*)pvValue,sizeof(stNetworkTime));
            break;
        case eBackupRamConfig_WakeupAlramTime:
             BkSram_SystemInfo.unWakeupTime = *(uint32_t*)pvValue;
            break;
        case eBackupRamConfig_PowerOffTime:
            BkSram_SystemInfo.unPowerOffTime = *((uint32_t*)pvValue);
            break;
        default:
            Trace("Error Not Defined SetBackupRamConfigProperty\r\n");
            break;
    }
}

void GetBackupRamConfigProperty(uint8_t cIndex,void* pvValue)
{
    switch(cIndex)
    {
        case eBackupRamConfig_Apn:
            memcpy((char*)pvValue,BkSram_ModemInfo.carrDefaultAPN,MAX_AUTOCONFIG_APN_SIZE);
            break;
        case eBackupRamConfig_DrivingInterval:
            *((uint32_t*)pvValue) = BkSram_SystemInfo.unDrivingInterval;
            break;
        case eBackupRamConfig_WakeupInterval:
            *((uint32_t*)pvValue) = BkSram_SystemInfo.unWakeUpInterval;
            break;
        case eBackupRamConfig_SystemTimeout:
             *((uint32_t*)pvValue) = BkSram_SystemInfo.unSystemTimeOut;
            break;
        case eBackupRamConfig_FotaInterval:
             *((uint32_t*)pvValue) = BkSram_SystemInfo.unFotaInterval;
            break;
        case eBackupRamConfig_NetworkInfo:
            memcpy((char*)pvValue,(char*)&BkSram_ModemInfo.stNetworkInfo,sizeof(stNetworkTime));
            break;
        case eBackupRamConfig_WakeupAlramTime:
            *(uint32_t*)pvValue = BkSram_SystemInfo.unWakeupTime;
            break;
        case eBackupRamConfig_PowerOffTime:
            *((uint32_t*)pvValue) = BkSram_SystemInfo.unPowerOffTime;
            break;
        default:
            Trace("Error Not Defined GetBackupRamConfigProperty\r\n");
            break;
    }
}

void DisplayBackupRamConfigData()
{
    Trace("=============================================================\r\n");
    Trace("#############################################################\r\n");
    Trace("Display Backup Ram Properties\r\n");
    Trace("APN : [%s]\r\n",BkSram_ModemInfo.carrDefaultAPN);
    Trace("DrivingInterval : %d\r\n",BkSram_SystemInfo.unDrivingInterval);
    Trace("FotaInterval : %d\r\n",BkSram_SystemInfo.unFotaInterval);
    Trace("SystemTimeout : %d\r\n",BkSram_SystemInfo.unSystemTimeOut);
    Trace("WakeupInterval : %d\r\n",BkSram_SystemInfo.unWakeUpInterval);
    Trace("unPowerOffTime : %d\r\n",BkSram_SystemInfo.unPowerOffTime);

    DisplayTime("Set Wake up Time - ",BkSram_SystemInfo.unPowerOffTime);
    Trace("#############################################################\r\n");
}


void DisplayConfigData(stAutolinkConfigData stConfig, boolean_t bDisp)
{
    Trace("[%s]======================================================\r\n", __FUNCTION__);
    Trace("#############################################################\r\n");
    Trace("Display System Properties\r\n");
    Trace("VIN : [%s]\r\n",stConfig.stSystem.carrVin);
    Trace("Latitude : %f\r\n",stConfig.stSystem.dlLastLatitude);
    Trace("Longitutde : %f\r\n",stConfig.stSystem.dlLastLongitude);
    Trace("bModemActive : %d\r\n",stConfig.stSystem.bModemActive);
    Trace("unOdometer : %d\r\n",stConfig.stSystem.unOdometer);

    DisplayTime("Last Saved Network Time - ", stConfig.stSystem.stNetworkInfo.unNetWrokTime);
    Trace("Last Saved Network Time Zone : %d\r\n", stConfig.stSystem.stNetworkInfo.sTimeZone);

    Trace("carrCellPhoneNum : [%s]\r\n",stConfig.stSystem.carrCellPhoneNum);
    Trace("default write : %d\r\n",GetModemActive());
    Trace("System reset flag : %d\r\n",stConfig.stSystem.bSystemReset);
    Trace("System reset count : %d\r\n",stConfig.stSystem.usSystemResetCount);
    Trace("Allow New Vin : %d\r\n",stConfig.stSystem.cAllowNewVin);
    Trace("Service Type : %x\r\n",stConfig.stSystem.nServiceType);
    Trace("Encrypt Type : %x\r\n",stConfig.stSystem.nEncryptType);
    Trace("Cal X: %d, Y: %d, Z: %d\r\n",m_stAutolinkConfigData.stSystem.stGyroAngle.nX,
        m_stAutolinkConfigData.stSystem.stGyroAngle.nY,
        m_stAutolinkConfigData.stSystem.stGyroAngle.nZ);
    Trace("#############################################################\r\n");

    if( bDisp ) DisplayUserActionSetting(stConfig.stUserAction);
}

void DisplayAutolinkConfigData()
{
    //char carrBuff[64] = {0,};
    //struct tm* ptmTime;
    Trace("[%s]===================================================\r\n", __FUNCTION__);
    Trace("#############################################################\r\n");
    Trace("Display System Properties\r\n");
    Trace("VIN : [%s]\r\n",m_stAutolinkConfigData.stSystem.carrVin);
    Trace("Latitude : %f\r\n",m_stAutolinkConfigData.stSystem.dlLastLatitude);
    Trace("Longitutde : %f\r\n",m_stAutolinkConfigData.stSystem.dlLastLongitude);
    Trace("bModemActive : %d\r\n",m_stAutolinkConfigData.stSystem.bModemActive);
    Trace("unOdometer : %d\r\n",m_stAutolinkConfigData.stSystem.unOdometer);

    DisplayTime("Last Saved Network Time - ",m_stAutolinkConfigData.stSystem.stNetworkInfo.unNetWrokTime);
    Trace("Last Saved Network Time Zone : %d\r\n",m_stAutolinkConfigData.stSystem.stNetworkInfo.sTimeZone);

    Trace("carrCellPhoneNum : [%s]\r\n",m_stAutolinkConfigData.stSystem.carrCellPhoneNum);
    Trace("default write : %d\r\n",GetModemActive());
    Trace("System reset flag : %d\r\n",m_stAutolinkConfigData.stSystem.bSystemReset);
    Trace("System reset count : %d\r\n",m_stAutolinkConfigData.stSystem.usSystemResetCount);
    Trace("Allow New Vin : %d\r\n",m_stAutolinkConfigData.stSystem.cAllowNewVin);
    Trace("Service Type : %x\r\n",m_stAutolinkConfigData.stSystem.nServiceType);
    Trace("Encrypt Type : %x\r\n",m_stAutolinkConfigData.stSystem.nEncryptType);
    Trace("Cal X: %d, Y: %d, Z: %d\r\n",m_stAutolinkConfigData.stSystem.stGyroAngle.nX,
        m_stAutolinkConfigData.stSystem.stGyroAngle.nY,
        m_stAutolinkConfigData.stSystem.stGyroAngle.nZ);
    Trace("#############################################################\r\n");

    DisplayUserActionSetting(m_stAutolinkConfigData.stUserAction);
}

int GetDUKPTPinEntry(DukptPinEntry* pstEntry)
{
    unsigned char *pin="1234";
    unsigned char *account_num="4012345678909";
    int flags = DAMO_DUKPT_FLAG_USE_ALL_KEY;

    if( m_bEnableKMSProcess == false )
    {
        Trace("KMS isn't initialized\r\n");
        return -1;
    }

    int nResult = DAMO_DUKPT_Request_Pin_Entry(pin, strlen((char *)pin), account_num, strlen((char *)account_num), flags, pstEntry);

    return nResult;
}

typedef struct __stIpekList{
    char ipek[16];
}stIpekList;

/*

char* m_strTestSerialList[] =
{
"R9999990AUH0000",//0
"R9999991AUH0000",	//법인장
"R9999992AUH0000",	//팀장
"R9999993AUH0000",	//업체법인장
"R9999994AUH0000",	//연수모듈
"R9999995AUH0000",	//철훈
"R9999996KRH0000",	//문현걸
"R9999997AUH0000",	//품질
"R9999998AUH0000",  //종현모듈 호주향 8번
"R9999999AUH0000",	//호주 충돌테스트
"R9999700AUH0000",//10
"R9999701AUH0000",//11
"R9999702AUH0000",//12
"R9999703AUH0000",//13
"R9999704AUH0000",//14
"R9999705AUH0000",//15
"R9999706AUH0000",//16
"R9999707AUH0000",//17
"R9999710AUH0000",//18	호주 G80
"R9999711AUH0000",//19	호주 G80
"R9999712AUH0000",//20	호주 G80
"R9999713AUH0000",//21	호주 OS
"R9999714AUH0000",//22	호주 OS
"R9999715AUH0000",//23	호주 OS
"R9999716AUH0000",//24	호주 PD
//R9999717	1팀
//R9999718	1팀
//R9999719	문책임님호주
//R9999720	문책임님호주
//R9999721	모의해킹 5번시료
//R9999722	모의해킹 6번시료
//R9999723  //moon lab test
//R9999724	fleet
//R9999725  	fleet
};

stIpekList m_stTestIpekList[] =
{
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
{0x4D,0x4E,0xF1,0x30,0x15,0x6D,0x54,0x1E,0xA8,0xAB,0x8C,0xE9,0x2C,0x40,0x52,0xF0},
{0x0F,0x1B,0x2A,0xA4,0x83,0x84,0xE9,0x13,0xD7,0x0D,0x17,0x63,0xF5,0xE9,0xA5,0xAC},
{0xC1,0xEC,0x5F,0xBB,0x78,0xCF,0xFB,0xCB,0x4D,0xE0,0xB8,0xC9,0xC7,0x90,0xEE,0x4A},//R9999994
{0xF3,0xAF,0x5C,0xAF,0x7C,0x3B,0x75,0xDD,0xE4,0xC9,0xF7,0x89,0x47,0x90,0xD8,0xB1},//R9999995
{0x12,0x0A,0xAA,0xD2,0x38,0xE7,0x97,0xB1,0x01,0xA2,0x8E,0x23,0x8C,0xCE,0xF6,0xEE},//R9999996
{0x78,0x4D,0x0A,0x2B,0x48,0xAD,0xD7,0xFB,0x5F,0xB8,0x12,0xDE,0xF9,0x2B,0xB0,0x87},//R9999997
{0xC9,0xF3,0x43,0xB6,0x95,0xC8,0x2D,0x46,0x20,0xB8,0x01,0x3E,0x8C,0x9E,0x38,0x05},//R9999998
{0x90,0x1F,0xA7,0x4C,0xB2,0x53,0x06,0xA0,0xF5,0x3C,0xC9,0x24,0x7B,0x4F,0x44,0x8C},//R9999999
{0x14,0xE0,0xAF,0xF1,0x5E,0x3E,0x38,0x34,0x65,0xB3,0x02,0xA0,0xDA,0x39,0x50,0x53},//R9999700
{0x29,0x68,0xB5,0xCA,0xD5,0x3F,0x31,0x11,0x78,0xA6,0xE5,0x61,0x74,0x13,0xBB,0xF8},//R9999701
{0xFB,0x19,0xFC,0x8F,0x75,0x41,0x25,0xF4,0x8D,0x77,0xE7,0xA7,0x29,0x1E,0x7B,0xFF},//R9999702
{0x12,0x57,0x7B,0x78,0xBE,0x67,0xF8,0x1B,0x0F,0xC7,0x8E,0x42,0x38,0xED,0x27,0x88},//R9999703
{0x76,0x42,0xB6,0x7E,0x24,0xAC,0x28,0x8B,0x62,0xF5,0x2E,0x41,0x69,0x02,0x18,0xF9},//R9999704
{0x23,0x19,0x60,0x1D,0xE5,0x1F,0xC0,0x52,0x0E,0xAF,0x8A,0x6B,0x5A,0xAB,0xF7,0xB3},//R9999705
{0x03,0x21,0x7B,0x0D,0xA9,0xD9,0x6E,0x33,0x8F,0x77,0xA4,0xAB,0x1E,0x25,0x35,0xBA},//R9999706
{0xF4,0x9A,0x83,0x08,0x9E,0x1E,0xE4,0x7D,0xA3,0x5B,0xEE,0x0C,0xC6,0x2E,0x3D,0x6F},//R9999707
{0x3B,0x35,0x64,0x5E,0xEE,0x81,0x92,0xDD,0x7B,0xEA,0x73,0xFB,0x8E,0xB3,0x88,0xE9},//R9999710
{0x5C,0x8A,0x9F,0x38,0x78,0xC9,0xF8,0x0D,0x01,0x94,0xDB,0x2F,0x24,0xCA,0x62,0xF4},//R9999711
{0x78,0x65,0x2F,0xA4,0x69,0xBF,0x3F,0xE7,0xA2,0x6B,0xAD,0x0F,0x62,0x9E,0x99,0x39},//R9999712
{0x33,0x27,0x5C,0x37,0x36,0xC2,0xA5,0xFE,0x5F,0x42,0xC8,0xB2,0xE0,0x0F,0xDC,0x37},//R9999713
{0x07,0x9A,0x3E,0x03,0x00,0xAA,0x1F,0x94,0x95,0xA6,0xD6,0xCC,0x6B,0x34,0xBC,0x12},//R9999714
{0x53,0xA2,0x9A,0xEF,0xB0,0x63,0x9A,0x0D,0x0F,0xC2,0xF3,0x81,0xC1,0x33,0x7F,0xEB},//R9999715
{0xE4,0x53,0x53,0xD9,0x77,0x18,0x67,0x6D,0x6D,0x76,0x57,0x3F,0x4D,0xE1,0x61,0x0C},//R9999716
//R9999717	1팀
//R9999718	1팀
//R9999719	1팀
//R9999720	1팀
};

void TestSetSerialAndIpektoInternaFlash(uint8_t cNum)
{
	unsigned char key[16];												// STM32F4 unique id address 12byte + 0x00
	unsigned char in[16];												// 암호화할 데이터
	unsigned char iv[16]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char ucEncryptionKey[32];			// out
	//uint32_t wAddress = 0x1FFF7A10;
	size_t encrypt_len;

    uint8_t bModemActive = true;   // 생산시에는 Modem을 Deactive시켜 줘야 한다.

    memset((char *)in,0x00,sizeof(in));
	memset((char *)key,0x00,sizeof(key));
	memset((char *)ucEncryptionKey,0x00,sizeof(ucEncryptionKey));

	// Serial Write
	SetFWSerialNumber((char *)m_strTestSerialList[cNum]);

    Trace("Request Number : %d\n",cNum);
    Trace("Serial : %s\n",m_strTestSerialList[cNum]);

    GetDefaultEncryptKey((char*)key,false);

    memcpy(in,&m_stTestIpekList[cNum].ipek,16);
    Trace("Ipek : "); hexdump(&m_stTestIpekList[cNum].ipek, 16);

	Trace("============================ AES-CBC ============================\n");
	printdump( "aes-cbc encrypt(plaintext)", in, sizeof(in) );
	DAMO_CRYPT_AES_EncryptEx(ucEncryptionKey, &encrypt_len, in, sizeof(in), key, sizeof(key), AES_128, CBC_MODE, iv);
	printdump( "aes-cbc encrypt(ciphertext)", ucEncryptionKey, encrypt_len );
	Trace("\n");
	Trace("=================================================================\n");

    // set master key to internal flash
	SetModuleKey((char *)ucEncryptionKey);

	Trace("\r\n<DisplayFirmwareInfo>\r\n");
	DisplayFirmWareInfo();

    // set active mode default status
    SetModemActive(bModemActive);

    // initilalize ipek
    // added this code to initialize autolink configuration to serial flash
    WriteDefaultAutolinkConfigValue();
    //DisplayAutolinkConfigData();
}
*/

void TestSetSerialAndIpektoInternaFlash(uint8_t cNum)
{
}

void SetSerialAndIpektoInternaFlash(char* pcarrIpek, int nIpekSize)
{
	unsigned char key[16];												// STM32F4 unique id address 12byte + 0x00
	unsigned char in[16];												// 암호화할 데이터
	unsigned char iv[16]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	unsigned char ucEncryptionKey[32];			// out
	//uint32_t wAddress = 0x1FFF7A10;
	size_t encrypt_len;

    //uint8_t bModemActive = true;

    memset((char *)in,0x00,sizeof(in));
	memset((char *)key,0x00,sizeof(key));
	memset((char *)ucEncryptionKey,0x00,sizeof(ucEncryptionKey));

    GetDefaultEncryptKey((char*)key,false);

    memcpy(in,pcarrIpek,nIpekSize);
    Trace("Ipek : "); hexdump(pcarrIpek, nIpekSize);

	Trace("============================ AES-CBC ============================\r\n");
	printdump( "aes-cbc encrypt(plaintext)", in, sizeof(in) );
	DAMO_CRYPT_AES_EncryptEx(ucEncryptionKey, &encrypt_len, in, sizeof(in), key, sizeof(key), AES_128, CBC_MODE, iv);
	printdump( "aes-cbc encrypt(ciphertext)", ucEncryptionKey, encrypt_len );
	Trace("\r\n");
	Trace("=================================================================\r\n");

    // set master key to internal flash
	SetModuleKey((char *)ucEncryptionKey);

	DisplayFirmWareInfo();

    // initilalize ipek
    // added this code to initialize autolink configuration to serial flash
    WriteDefaultAutolinkConfigValue();
    DisplayAutolinkConfigData();
}


boolean_t m_bRequestIpek = false;

void SetRequestIPEK()
{
    Trace("Request IPEK to Server\r\n");
    // request ipek phase #1
    if( m_bRequestIpek == false )
    {
        Send2MngSysMsg(eMngSys,eReqIpek,eIpekPhase1,(stCarReport *)NULL,0);
        m_bRequestIpek = true;
    }
}


void ClearFutureKey()
{
    memset((char*)&m_stAutolinkConfigData.stFutureKey,0,sizeof(m_stAutolinkConfigData.stFutureKey));
    WriteConfig(false, false);
}

/*

extern unsigned char g_KMS_Publickey[];


void GetDecryptPublicKey(char* pcarrDecryptPublicKey,uint32_t* punSize)
{
    char carrBuf[1024] = {0,};
    char carrBuf2[1024] = {0,};
    unsigned char AutolinkEncryptKey[MAX_ENCRYPT_KEY_LENGTH] = {0};
    int32_t nPublicKeySize = strlen(g_KMS_Publickey);

    // get encrypt key
    GetDefaultEncryptKey((char *)AutolinkEncryptKey,false);

    // we will apply encryp process after we apply damo encryption process.
    int nEncryptResult = DAMO_CRYPT_AES_EncryptEx(pcarrDecryptPublicKey, (size_t*)punSize,
        g_KMS_Publickey, nPublicKeySize,
        AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, m_carrIv);

    if( nEncryptResult != 0 )
    {
        // erro encrypt
        Trace("Encrypt error : recovery\n");
    }

    printf("public key\n");
    hexdump(g_KMS_Publickey,strlen(g_KMS_Publickey));

    printf("encrypt public key\n");
    hexdump(pcarrDecryptPublicKey,*punSize);

    int decryptSize;
    // we will apply encryp process after we apply damo encryption process.
    int nEncryptReuslt = DAMO_CRYPT_AES_DecryptEx((unsigned char *)carrBuf2, &decryptSize,
    (const unsigned char *)pcarrDecryptPublicKey, *punSize,
    AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, m_carrIv);

    if( nEncryptReuslt != 0 )
    {
        // recovery configuration data
        Trace("IMPORTANT : Decrypt error : recovery\n");
        Trace("Write DefaultAutolinkConfiguration\n");
    }

    printf("decrypt public key\n");
    hexdump(carrBuf2,decryptSize);
}
*/

/*

-----BEGIN PUBLIC KEY-----
MIIBITANBgkqhkiG9w0BAQEFAAOCAQ4AMIIBCQKCAQBkrtV04FBW+eowKld0VmMm
yUvAl8S6pt/KQW0wwEfPibmE+4+7yhZ7B2AdE5I3wTrcd5xnM3VdmMg7a+CwB9Yu
O8d3cB7u2dMDmBd4GF+DHL2NO7RiRE15hYrilvDPiGv2hndl0B7ysI74QE+BUkst
QcVaDIhyrNaH0KXnRc1wWToeL81cQsN5OhSh/4oLaHrDZJ4fErgG45jrxi7dq2nU
te/x463t+8yJp+nOhvUdwK08nW7br9yanTMFaUTk+N6wlnuHtlqpGKcQT+Qgvx2N
+rhIoPnXvk0AbKb4Sm0SShLvxUWa2BNfU+7Q2kxJeu8xkKQZOpYfUjbEC9ilb25T
AgMBAAE=
-----END PUBLIC KEY-----

*/


