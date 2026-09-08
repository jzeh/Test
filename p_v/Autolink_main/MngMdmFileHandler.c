/* Includes ------------------------------------------------------------------*/
#include "GIT_Util.h"
#include "AutolinkMessage.h"
#include "Message_Manager.h"
#if defined(USE_GIT_FAT_FS)
#include "ff.h"
#endif
#include "time.h"
#include "MngSystem.h"
#include "MngSystemUtil.h"
#include "MngQueue.h"
#include "HdDebug.h"
#include "MngStorage.h"
#include "DebugHandler.h"
#include "SysHalFileSystem.h"
#include "HalHandler.h"

#if 0
#include "MngModem.h"

#define AUTOLINK_MODEM_STATUS_DENIED   			  "Denied.log"
#define AUTOLINK_MODEM_STATUS_NOT_COMMUNICATION   "NoCommunication.log"
#define AUTOLINK_MODEM_STATUS_CME_ERROR           "CMEError.log"

extern MODEM_STATE_STRUCTURE g_ModemStateStructure;
#endif

#define Trace(...)  GITDebug(DEBUG_MODULES_FILE_SYSTEM,__VA_ARGS__)

#define AUTOLINKDATA_BASE_DIR   "AutoLinkData"
#define AUTOLINK_MODEM_CONFIG   "modem_data"
#define AUTOLINK_AGPS_DATA      "UbloxAgpsData"

extern uint8_t m_cRecoveryRetryCount;

extern void ShowDir(char* carrPath);
extern eGitFresult DeleteAutolinkFile(char *pcarrFullPath);

typedef __packed struct __stModemConfiguration
{
    uint8_t nMessageCount;
    uint8_t nDataCount;
    uint8_t bRecoveryMessage;
	uint8_t bRecoveryRetryCount;
    uint8_t reserved;
    stMsgMdm stRecoveryMessage;
}stModemConfiguration;

eGitFresult ReadAgpsDataSize(char* pstrFileName, int* pnSize);
boolean_t WriteBuffer(void* fpFile, char* pData, uint16_t nLen);
eGitFresult ReadBuffer(void* fpFile, char *pData, uint16_t nLen);
void WriteModemConfiguration(boolean_t bRecoveryMessage, stMsgMdm stRecoveryMessage);
void ReadModemConfiguration(boolean_t* pbRecoveryMessage, stMsgMdm* pstRevoeryMessage);


boolean_t WriteBuffer(void* fpFile, char* pData, uint16_t nLen)
{
    eGitFresult ret = GIT_FR_OK;
    UINT dwFileSize;
    int nTotalLength = nLen;
    int nTotalWriteLength = 0;
    int nCount = 0;

    while(1)
    {
        ret = git_f_write(fpFile, (char *)pData, nLen, &dwFileSize);
        if(ret != GIT_FR_OK) {
            git_f_close(fpFile);

            Trace("@%s(), %s write fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

            return GIT_FR_DISK_ERR;
        }

        nTotalWriteLength+=dwFileSize;
        nLen-=dwFileSize;

        if( nTotalLength == nTotalWriteLength )
        {
            break;
        }

        if( dwFileSize == 0 )
            break;

        nCount++;
    }

    return true;
}

eGitFresult WriteAgpsData(char* pstrFileName, char* pcarrBuff, int nSize)
{
	stFileSystemDescript fpFile;
    char path[256];

    sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,pstrFileName);

    if( git_f_open(&fpFile, (const char *)path, GIT_FA_OPEN_ALWAYS | GIT_FA_WRITE) != GIT_FR_OK )
    {
        // make directory
        //sprintf(path,"/%s\x00",AUTOLINKDATA_BASE_DIR);
        git_f_mkdir(path);
        //sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,pPath);
        //git_f_mkdir(path);
        sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,pstrFileName);

        if( git_f_open(&fpFile, (const char *)path, GIT_FA_CREATE_NEW | GIT_FA_WRITE) != GIT_FR_OK )
        {
            // error
            GIT_Assert(false,eErrorCodeMdm|eWriteOpenFail);
            return GIT_FR_DISK_ERR;
        }
    }

    if( git_f_lseek(&fpFile, git_f_size(&fpFile)) != GIT_FR_OK )
      return GIT_FR_DISK_ERR;

    //printf("lseek : %d\n",git_f_size(&fpFile));
    //ret = git_f_lseek(&fpFile, 0);

    if( WriteBuffer(&fpFile,pcarrBuff,nSize) == false )
    {
        GIT_Assert(false,eErrorCodeMdm|eWriteError);
        return GIT_FR_DISK_ERR;
    }

    if( git_f_close(&fpFile) != GIT_FR_OK )
    {
        GIT_Assert(false,eErrorCodeMdm|eCloseError);
        return GIT_FR_DISK_ERR;
    }

    return GIT_FR_OK;
}

eGitFresult ReadAgpsDataSize(char* pstrFileName, int* pnSize)
{
	stFileSystemDescript fpFile;
    char path[256];

    sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,pstrFileName);

    if( git_f_open(&fpFile, (const char *)path, GIT_FA_OPEN_ALWAYS | GIT_FA_READ) != GIT_FR_OK )
    {
        // error
        GIT_Assert(false,eErrorCodeMdm|eWriteOpenFail);
        return GIT_FR_DISK_ERR;
    }

    *pnSize = git_f_size(&fpFile);

    if( git_f_close(&fpFile) != GIT_FR_OK )
    {
        GIT_Assert(false,eErrorCodeMdm|eCloseError);
    }

    return GIT_FR_OK;
}


int ReadAgpsData(char* pstrFileName, char* pcarrBuff, int nSize, int nIndex)
{
	stFileSystemDescript fpFile;
    char path[256];

    sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,pstrFileName);

    if( git_f_open(&fpFile, (const char *)path, GIT_FA_OPEN_ALWAYS | GIT_FA_READ) != GIT_FR_OK )
    {
        // error
        GIT_Assert(false,eErrorCodeMdm|eWriteOpenFail);
        return GIT_FR_DISK_ERR;
    }

    if( git_f_lseek(&fpFile, nIndex) != GIT_FR_OK )
      return GIT_FR_DISK_ERR;
    //ret = git_f_lseek(&fpFile, 0);

    if( ReadBuffer(&fpFile,pcarrBuff,nSize) == GIT_FR_OK )
    {
        GIT_Assert(false,eErrorCodeMdm|eWriteError);
    }

    if( git_f_close(&fpFile) != GIT_FR_OK )
    {
        GIT_Assert(false,eErrorCodeMdm|eCloseError);
    }

    return GIT_FR_OK;
}

eGitFresult DeleteAgpsData(char* pstrFileName)
{
    char path[256];
    sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,pstrFileName);

    return DeleteAutolinkFile(path);
}

boolean_t IsAgpsFileExist()
{
	stFileSystemDescript fpFile;

    char path[256];
    sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_AGPS_DATA);

    eGitFresult ret = git_f_open(&fpFile, (const char *)path, GIT_FA_OPEN_ALWAYS | GIT_FA_READ);
    if( ret == GIT_FR_OK )
    {
        int nSize = git_f_size(&fpFile);
        git_f_close(&fpFile);

        if( nSize  > 0 )
        {
            return true;
        }
    }

    //git_f_close(&fpFile);

    return false;
}
#if 0
FRESULT WriteAutoLinkModemsStatusData(eMODEM_ERROR_STATE ModemState)	
{
	FIL fpFile;
	FRESULT ret = FR_OK;
    char path[256]={0,};

	if( ModemState == MODEM_DENIED )                  sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_DENIED);
	else if( ModemState == MODEM_NOT_COMMUNICATION )  sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_NOT_COMMUNICATION);
	else if( ModemState == MODEM_CME_ERROR )          sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_CME_ERROR);
	
    // make file
    ret = git_f_open(&fpFile, (const char *)path, FA_READ);
  
    if( ret != FR_OK)
    {
	    if( git_f_open(&fpFile, (const char *)path, FA_OPEN_ALWAYS | FA_WRITE) == FR_OK )
	    {
	        // make directory
	        sprintf(path,"/%s\x00",AUTOLINKDATA_BASE_DIR);
	        git_f_mkdir(path);

			if( ModemState == MODEM_DENIED )       			  sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_DENIED);
			else if( ModemState == MODEM_NOT_COMMUNICATION )  sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_NOT_COMMUNICATION);
			else if( ModemState == MODEM_CME_ERROR )  		  sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_CME_ERROR);

			ret = git_f_lseek(&fpFile, 0);

		    // write data
		    for(int i=0;i<MAX_MODEMSTATEBUFFER_SIZE;i++)
		    {
		        if( WriteBuffer(&fpFile, (char *)&(g_ModemStateStructure.ModemStateCollection[i]),sizeof(MODEM_STATE_COLLECTION)) == false )
		        {
		            GIT_Assert(false,eErrorCodeMdm|eWriteFail);
		        }
		    }

			Trace("%s] File Write Serveral time\n",__FUNCTION__);
			Trace("Success !!! %d \r\n", ret);

			git_f_close(&fpFile);

			return FR_OK;
	    }
    }
	else
	{
        git_f_close(&fpFile);

		return FR_DENIED;
	}

}

FRESULT ReadAutolinkModemStatusData(eMODEM_ERROR_STATE ModemState, MODEM_STATE_COLLECTION* pModemStateStructure)
{
	FIL fpFile;
	FRESULT ret = FR_OK;
    char path[256];
    stMsgMdm stModemMessage;
   
	if( ModemState == MODEM_DENIED )                 sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_DENIED);
	else if( ModemState == MODEM_NOT_COMMUNICATION ) sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_NOT_COMMUNICATION);
	else if( ModemState == MODEM_CME_ERROR )         sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_CME_ERROR);

	if ( (ret = git_f_open(&fpFile, (const char *)path, FA_OPEN_EXISTING | FA_READ)) == FR_OK )
    {
        ret = git_f_lseek(&fpFile, 0);

        // read data
        for(int i=0 ; i < MAX_MODEMSTATEBUFFER_SIZE ; i++)
        {
            ReadBuffer(&fpFile, (char*)&pModemStateStructure[i],sizeof(MODEM_STATE_COLLECTION));

            //DisplayReport("Modem Backup Read : Data",(stMsgSysMsg*)&stModemMessage);

            // enqueue the data
        }
			
		git_f_close(&fpFile);
	}
	else {
		Trace("@%s(), %s open fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

		return FR_DISK_ERR;
	}

	return FR_OK;
}

FRESULT ExistAutolinkModemStatusData(eMODEM_ERROR_STATE ModemState)
{
	FIL fpFile;
	FRESULT ret = FR_OK;
    char path[256];
    stMsgMdm stModemMessage;
   
	if( ModemState == MODEM_DENIED )                 sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_DENIED);
	else if( ModemState == MODEM_NOT_COMMUNICATION ) sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_NOT_COMMUNICATION);
	else if( ModemState == MODEM_CME_ERROR )         sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_CME_ERROR);

	if ( (ret = git_f_open(&fpFile, (const char *)path, FA_OPEN_EXISTING)) == FR_OK )
    {
		git_f_close(&fpFile);
	}
	else {
		
		Trace("@%s(), %s open fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

		return FR_DISK_ERR;
	}

	return FR_OK;
}



FRESULT DeleteAutolinkModemStatusData(eMODEM_ERROR_STATE ModemState)
{
	FIL fpFile;
    FRESULT ret = FR_OK;
	
	char path[256]={0,};
	
	if( ModemState == MODEM_DENIED )                 sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_DENIED);
	else if( ModemState == MODEM_NOT_COMMUNICATION ) sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_NOT_COMMUNICATION);
	else if( ModemState == MODEM_CME_ERROR )         sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_STATUS_CME_ERROR);

	if ( (git_f_unlink((const char *)path)) == FR_OK ) {
        Trace("[%s] delete success!!\r\n", path);
	}
	else {
		Trace("[%s] delete fail!!! : %d \r\n", path, ret);
		
		return FR_DISK_ERR;
	}

    return FR_OK;
}
#endif

eGitFresult WriteAutoLinkModemData(stModemConfiguration* pstModemConfigurationData)
{
	stFileSystemDescript fpFile;
	eGitFresult ret = GIT_FR_OK;
    char path[256];

    int nMdmMessageCount = MngQueueGetMessageCount(ID_MNG_QUEUE_MDM);
    int nMdmDataCount = MngQueueGetMessageCount(ID_MNG_QUEUE_DATA);

    //sprintf(path,"/%s\x00",AUTOLINKDATA_BASE_DIR);
    //ShowDir(path);

    sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_CONFIG);

    //DeleteAutolinkFile(path);

    // make file
    if( git_f_open(&fpFile, (const char *)path, GIT_FA_OPEN_ALWAYS | GIT_FA_WRITE) != GIT_FR_OK )
    {
        // make directory
        sprintf(path,"/%s\x00",AUTOLINKDATA_BASE_DIR);
        git_f_mkdir(path);
        //sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,pPath);
        //git_f_mkdir(path);
        sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_CONFIG);

        if( git_f_open(&fpFile, (const char *)path, /*GIT_FA_OPEN_ALWAYS*/GIT_FA_CREATE_NEW | GIT_FA_WRITE) != GIT_FR_OK ) // 20210412 확인필요
        {
            // error
            GIT_Assert(false,eErrorCodeMdm|eWriteOpenFail);
        }
    }

    //ret = git_f_lseek(&fpFile, git_f_size(&fpFile));
    ret = git_f_lseek(&fpFile, 0);

    // set data count for recovery
    pstModemConfigurationData->nMessageCount = nMdmMessageCount;
    pstModemConfigurationData->nDataCount = nMdmDataCount;
	pstModemConfigurationData->bRecoveryRetryCount = m_cRecoveryRetryCount;

    // write data header
    WriteBuffer(&fpFile,(char*)pstModemConfigurationData,sizeof(stModemConfiguration));

    stMsgMdm stTmpMessage;

    // write data
    for(int i=0;i<nMdmDataCount;i++)
    {
#ifndef GLOBAL_SHARE_QUEUE //Get
        if( MngQueueGetMessage(ID_MNG_QUEUE_DATA, (int8_t*)&stTmpMessage, sizeof(stMsgMdm)) == true )


#else
        if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_DATA, (uint8_t*)&stTmpMessage.header, sizeof(stMsgHeader), (uint8_t*)&stTmpMessage.carReport, sizeof(stCarReport)) == true )
#endif

        {
            if( WriteBuffer(&fpFile,(char*)&stTmpMessage,sizeof(stMsgMdm)) == false )
            {
                GIT_Assert(false,eErrorCodeMdm|eWriteFail);
            }
        }
        else
        {
            GIT_Assert(false,eErrorCodeMdm|eWriteDataQueueFail);
        }
    }

    // write message
    for(int i=0;i<nMdmMessageCount;i++)
    {
#ifndef GLOBAL_SHARE_QUEUE //Get
        if( MngQueueGetMessage(ID_MNG_QUEUE_MDM, (int8_t*)&stTmpMessage, sizeof(stMsgMdm)) == true )
#else
        if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_MDM, (uint8_t*)&stTmpMessage.header, sizeof(stMsgHeader), (uint8_t*)&stTmpMessage.carReport, sizeof(stCarReport)) == true )
#endif
        {
            if( WriteBuffer(&fpFile,(char*)&stTmpMessage,sizeof(stMsgMdm)) == false )
            {
                GIT_Assert(false,eErrorCodeMdm|eWriteFail);
            }
        }
        else
        {
            GIT_Assert(false,eErrorCodeMdm|eWriteMsgQueueFail);
        }
    }

    Trace("%s] File Write Serveral time\r\n",__FUNCTION__);
	Trace("Success !!! %d \r\n", ret);

	git_f_close(&fpFile);

    //sprintf(path,"/%s\x00",AUTOLINKDATA_BASE_DIR);

    //ShowDir(path);

	return GIT_FR_OK;
}

eGitFresult ReadBuffer(void* fpFile, char *pData, uint16_t nLen)
{
    UINT dwFileSize2=0;
    int totalSize = 0;
    int readSize = nLen;

    while(1)
    {
		if ((git_f_read(fpFile, &pData[totalSize], readSize, (UINT *)&dwFileSize2)) == GIT_FR_OK )
        {
            if(dwFileSize2 > 0)
            {
                if( readSize == dwFileSize2 )
                {
                    break;
                }

                readSize = readSize - dwFileSize2;
                totalSize = totalSize+dwFileSize2;
            }
            else
            {
                break;
            }

		}
		else
        {
			break;
		}
	}

	return GIT_FR_OK;
}

eGitFresult ReadAutolinkModemData(stModemConfiguration* pstModemConfigurationData)
{
	stFileSystemDescript fpFile;
	eGitFresult ret = GIT_FR_OK;
    char path[256];
    stMsgMdm stModemMessage;

    //sprintf(path,"/%s\x00",AUTOLINKDATA_BASE_DIR);
    //ShowDir(path);

    sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_CONFIG);

	if ( (ret = git_f_open(&fpFile, (const char *)path, GIT_FA_EXIST | GIT_FA_READ)) == GIT_FR_OK ) //20210412 확인필요
    {
        //if( git_f_size(&fpFile) >= sizeof(stModemConfiguration) )
        {
            ret = git_f_lseek(&fpFile, 0);

            // read header
            ReadBuffer(&fpFile,(char*)pstModemConfigurationData,sizeof(stModemConfiguration));

            Trace("nMessageCount : %d\r\n",pstModemConfigurationData->nMessageCount);
            Trace("nDataCount : %d\r\n",pstModemConfigurationData->nDataCount);

            // read data
            for(int i=0;i<pstModemConfigurationData->nDataCount;i++)
            {
                ReadBuffer(&fpFile, (char*)&stModemMessage,sizeof(stMsgMdm));

                DisplayReport("Modem Backup Read : Data",(stMsgSysMsg*)&stModemMessage);

                // enqueue the data
#ifndef GLOBAL_SHARE_QUEUE
                MngQueueSendMessage(ID_MNG_QUEUE_DATA,(int8_t*)&stModemMessage,sizeof(stMsgMdm));
#else
	            SendSysHdShareQueueMessage(ID_MNG_QUEUE_DATA,(uint8_t*)&stModemMessage.header,sizeof(stMsgHeader),(uint8_t*)&stModemMessage.carReport,sizeof(stCarReport));
#endif
            }

            // read message
            for(int i=0;i<pstModemConfigurationData->nMessageCount;i++)
            {
                ReadBuffer(&fpFile, (char*)&stModemMessage,sizeof(stMsgMdm));

                DisplayReport("Modem Backup Read : Message",(stMsgSysMsg*)&stModemMessage);

                // enqueue the message
#ifndef GLOBAL_SHARE_QUEUE
                MngQueueSendMessage(ID_MNG_QUEUE_MDM,(int8_t*)&stModemMessage,sizeof(stMsgMdm));
#else
	            SendSysHdShareQueueMessage(ID_MNG_QUEUE_MDM,(uint8_t*)&stModemMessage.header,sizeof(stMsgHeader),(uint8_t*)&stModemMessage.carReport,sizeof(stCarReport));
#endif
            }
        }

		git_f_close(&fpFile);
	}
	else {
		Trace("@%s(), %s open fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

		return GIT_FR_DISK_ERR;
	}

	return GIT_FR_OK;
}

void ClearRecoveryFile()
{
    char path[256];
    sprintf(path,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,AUTOLINK_MODEM_CONFIG);

    DeleteAutolinkFile(path);
}

void DisplayModemData(stModemConfiguration stModemConfigurationData )
{
    Trace("bRecoveryMessage: %d\r\n",stModemConfigurationData.bRecoveryMessage);
    Trace("nMessageCount : %d\r\n",stModemConfigurationData.nMessageCount);
    Trace("nDataCount : %d\r\n",stModemConfigurationData.nDataCount);
}

void ReadModemConfiguration(boolean_t* pbRecoveryMessage, stMsgMdm* pstRevoeryMessage)
{
    stModemConfiguration stModemConfigurationData;
    memset((char*)&stModemConfigurationData,0,sizeof(stModemConfiguration));

    ReadAutolinkModemData(&stModemConfigurationData);

    *pbRecoveryMessage = stModemConfigurationData.bRecoveryMessage;
	m_cRecoveryRetryCount = stModemConfigurationData.bRecoveryRetryCount;
    memcpy((char*)pstRevoeryMessage,(char*)&stModemConfigurationData.stRecoveryMessage,sizeof(stMsgMdm));

    ClearRecoveryFile();

    DisplayModemData(stModemConfigurationData);
}

void WriteModemConfiguration(boolean_t bRecoveryMessage, stMsgMdm stRecoveryMessage)
{
    stModemConfiguration stModemConfigurationData;
    memset((char*)&stModemConfigurationData,0,sizeof(stModemConfiguration));

    stModemConfigurationData.bRecoveryMessage = bRecoveryMessage;
    memcpy((char*)&stModemConfigurationData.stRecoveryMessage,(char*)&stRecoveryMessage,sizeof(stMsgMdm));

    WriteAutoLinkModemData(&stModemConfigurationData);

    DisplayModemData(stModemConfigurationData);
}


