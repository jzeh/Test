/* Includes ------------------------------------------------------------------*/
#include "GIT_Util.h"
#include "ff.h"

#include "MngSystem.h"
#include "MngQueue.h"
#include "HdDebug.h"
#include "DebugHandler.h"
#include "MngStorage.h"
#include "GIT_OemInterface.h"
#include "AutolinkConfiguration.h"
#include "Message_manager.h"
#include "Modem_manager.h"
#include "MngSystemUtil.h"
#include "SysHalFileSystem.h"


#define Trace(...)  GITDebug(DEBUG_MODULES_STORAGE,__VA_ARGS__)

extern bool SystemDelayProcess(unsigned long* nBaseTime, int nDelay);
extern boolean_t IsFolderEmpty(char* pcarrFolderFullPath);
extern void Git_DeviceReset(void *pInterPtcl, unsigned int eInCommType);
extern void SendFotaAlramReport(int32_t nEvent, int32_t nResult);

enum {
    STAT_MNG_STORAGE_INIT = 0,
    STAT_MNG_STORAGE_SAVE,
    STAT_MNG_STORAGE_READ,
    STAT_MNG_STORAGE_DELETE,
    STAT_MNG_STORAGE_IDLE,
    STAT_MNG_STORAGE_SLEEP,
    STAT_MNG_STORAGE_SLEEP_WAIT,
};

int32_t m_nMngStorageState = STAT_MNG_STORAGE_INIT;

// external function
extern void ShowDir(char* carrPath);
extern eGitFresult WriteFile(int32_t nMode, int8_t *pcData, uint16_t nLen);
extern eGitFresult ReadFile(int32_t nMode, int8_t *pcData, uint16_t nLen);
extern eGitFresult ReadFile2(int32_t nMode, int8_t *pcData, uint16_t nLen);
extern eGitFresult CheckFile(int32_t nMode, int32_t* pnFileSize);
extern int32_t CheckFile2(int32_t nMode);
extern eGitFresult ReadAutolinkAsset(int nMode, char* pcarrPath,char* pcarrFileName, char *pData, uint16_t nLen);
extern eGitFresult WriteAutoLinkData(char *pData, uint16_t nLen, char *pPath);
extern int8_t *GetStringFromAlramEvent(int32_t nId);
extern int SearchLatestFile(stAutolinkFileInfo* pstLatestReadFileInfo);
extern void SetWaitforSendingData(boolean_t bFlag);
extern void SetNetworkPostPoneFlag(boolean_t bPostPonded);
extern void SetLastSendMessageFlag(boolean_t bFlag);

// internal function
void SetMngStorageState(int32_t nState);
void MngStorageExternalEvent(stMsgStorage* pstStorageMessage);
void ControlFile(int32_t nCmd, stMsgStorage* pstStorageMessage);
void ClearModemNoResponseCount();
boolean_t DeleteReport(stMsgStorage* pstStorageMessage);
boolean_t ReadReport(stMsgStorage* pstStorageMessage);
boolean_t SaveReport(stMsgStorage* pstStorageMessage);
boolean_t IsExistData();
boolean_t GetLastSendReportStatus();
boolean_t IsIsPossible2Transfer();

int32_t MngStorage(/*int32_t lparam, int32_t rparam*/);

stMsgStorage m_stStorageSendLastMessage;

boolean_t m_bSendReport = false;
boolean_t m_bNetworkConnected = false;
boolean_t m_bAutoReadFlag = false;
//uint32_t m_bAutoReadCount = 0;
boolean_t m_bSysBlockTransfer = false;

boolean_t m_bSuspendStorage = false;
volatile boolean_t m_bUsedStorage = false;
extern bool m_bRecoveryMessage;
extern bool m_bReqNetworkPostPone;
extern boolean_t m_bSentLastMessage;


stAutolinkFileInfo m_stLatestReadFileInfo;

extern eGitFresult DeleteAutolinkFile(char *pcarrFullPath);
extern eGitFresult DeleteAutolinkFolder(char *pcarrFolderPath);
extern int8_t *GetStringFromEvent(int32_t nMode,int32_t nEvent,int32_t nSubEvent);
extern eGitFresult IsFileEmpty(char * pcarrFullFilePath);
extern bool GetModemActive();

unsigned long m_ulStorageTimeStamp = 0;

boolean_t m_bBlockStorage = false;

int32_t MngStorage(/*int32_t lparam, int32_t rparam*/)
{
    char carrPath[128];
    stMsgStorage tmpMsgStorage;

    // check external event
//    if( MngQueueGetMessage(ID_MNG_QUEUE_STORAGE, (int8_t*)&tmpMsgStorage, sizeof(stMsgStorage)) == true )
	if( m_bBlockStorage == false )
	{
#ifndef GLOBAL_SHARE_QUEUE //Get
		if( MngQueueGetMessage(ID_MNG_QUEUE_STORAGE, (int8_t*)&tmpMsgStorage, sizeof(stMsgStorage)) == true )
#else
		if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_STORAGE, (uint8_t*)&tmpMsgStorage.header, sizeof(stMsgHeader), (uint8_t*)&tmpMsgStorage.carReport, sizeof(stCarReport)) == true )
#endif
		{

#if defined(MNG_QUEUE_DEBUG)
			Trace("event : %s, subEvent : %s, reuslt : %x\r\n",GetStringFromEvent(eGetStringEvent,tmpMsgStorage.header.event,0),
				GetStringFromEvent(eGetStringSubEvent,tmpMsgStorage.header.event,tmpMsgStorage.header.subEvent),
				tmpMsgStorage.header.result);
			if( tmpMsgStorage.header.event == eReqReport && tmpMsgStorage.header.subEvent == eR_Alram )
			{
				Trace("alram event : %s\r\n",GetStringFromAlramEvent(tmpMsgStorage.carReport.rpAlram.CarStatus.EventKey));
			}
#endif

			tmpMsgStorage.header.unTraceMng=tmpMsgStorage.header.unTraceMng<<8|eMngSysMsg;

			// external event process
			MngStorageExternalEvent(&tmpMsgStorage);
		}
	}

    switch(m_nMngStorageState)
    {
        case STAT_MNG_STORAGE_INIT:
            // check the first read file
            // check the last write file
            // check a different hour.
            // write process
            // make file a for an one hour.
            // write process.
            // 1. check is there a file?
            // 2. search how many file exist in  a /AutoLinkData
            // 3. find a file with prefix.
            // 4. save the data.
            {
                //if( SearchLatestFile(&m_stLatestReadFileInfo) == true )
                //{
                //    Trace("Success Search File Path : %s\r\n",m_stLatestReadFileInfo.m_carrLatestFullFilePath);
                //}
                boolean_t bExistData = IsExistData();

                SetMngStorageState(STAT_MNG_STORAGE_IDLE);

                Send2MngSysMsg(eMngStorage,eRspDataExist,bExistData,(stCarReport*)NULL,0);
            }
            break;
        case STAT_MNG_STORAGE_SAVE:
            SetMngStorageState(STAT_MNG_STORAGE_IDLE);
            break;
        case STAT_MNG_STORAGE_READ:
            // read data from serial flash and send event to system message handler
            // this code is just test
            SetMngStorageState(STAT_MNG_STORAGE_IDLE);

            break;
        case STAT_MNG_STORAGE_DELETE:
        {
            boolean_t bExistData;
            SetMngStorageState(STAT_MNG_STORAGE_IDLE);


			        //if( IsFileEmpty2(m_stLatestReadFileInfo.m_carrLatestFolderPath,m_stLatestReadFileInfo.m_carrLatestFilePath) == true )
        if( IsFileEmpty(m_stLatestReadFileInfo.m_carrLatestFullFilePath) == true )
        {
            Trace("already deleted\r\n");
            //delete file because read data size is zero
            //DeleteAutolinkFile(m_stLatestReadFileInfo.m_carrLatestFullFilePath);
            //DeleteAutolinkFolder(m_stLatestReadFileInfo.m_carrLatestFolderPath);

            //if( IsFolderEmpty(m_stLatestReadFileInfo.m_carrLatestFolderPath) == true )
       		sprintf((char *)carrPath,"%s/%s",AUTOLINKDATA_BASE_DIR, m_stLatestReadFileInfo.m_carrLatestFolderPath);
            if( IsFolderEmpty(carrPath) == true )
            {
                DeleteAutolinkFolder(m_stLatestReadFileInfo.m_carrLatestFolderPath);
            }


            //            if( SearchLatestFile(&m_stLatestReadFileInfo) == true )
            //            {
            //                Trace("Success Search File Path : %s\r\n",m_stLatestReadFileInfo.m_carrLatestFullFilePath);
            //            }
            //            else
            //            {
            //                Trace("Fail to search file\r\n");
            //            }
        }



            bExistData = IsExistData();
            Send2MngSysMsg(eMngStorage,eRspDataExist,bExistData,(stCarReport*)NULL,0);
        }
            break;
        case STAT_MNG_STORAGE_IDLE:
            // check smart key data or alram data
            // if exist send data event to message handler of system manager
            if( m_bSuspendStorage == false && m_bAutoReadFlag == true ) // for 2 seconds
            {
#ifdef ONLY_SAVE_MODE
				if( SystemDelayProcess(&m_ulStorageTimeStamp,500) == true )
#else
                if( SystemDelayProcess(&m_ulStorageTimeStamp,2*ONE_SECOND) == true )
#endif
                {
                    boolean_t bExistData = IsExistData();
                    //m_bAutoReadCount=0;

                    if( bExistData == false || m_bNetworkConnected == false || IsIsPossible2Transfer() == false )
                    {
                        Trace("Auto send process is stop because there is no data %d,%d,%d\r\n",bExistData,m_bNetworkConnected,IsIsPossible2Transfer());
                        m_bAutoReadFlag = false;
                    }
					else
					{
	                    if( bExistData == true && m_bNetworkConnected == true && m_bSuspendStorage == false && m_bSendReport == false &&
							m_bReqNetworkPostPone == false && m_bSentLastMessage == false && m_bRecoveryMessage == false ) // for 2 seconds
	                    {
	                        // request read data;
	                        Send2MngStorage(eMngSysMsg,eReqGetReport,0,(stCarReport *)NULL,0);
	                    }
					}
                }
            }

            break;
        case STAT_MNG_STORAGE_SLEEP:
            {
                Send2MngSysMsg(eMngStorage, eRspSleep, 0, (stCarReport *)NULL,0);

#if defined(MNG_QUEUE_DEBUG)
                Trace("Send] event[0]:%d, subEvent[1]:%d\r\n",tmp.event,tmp.subEvent);
#endif

                SetMngStorageState(STAT_MNG_STORAGE_IDLE);
            }
            break;
        case STAT_MNG_STORAGE_SLEEP_WAIT:
            // stop sending a data.
            m_bAutoReadFlag = false;

            SetMngStorageState(STAT_MNG_STORAGE_SLEEP);
            break;
        default:
            // not defined
            // error
        break;
    }

/*
    if( m_fnpMngStorageWork != NULL )
    {
        int32_t lparam, rparam;
        int32_t ret = m_fnpMngStorageWork(lparam,rparam);

        if( ret != 0 )
        {
            if( m_fnpMngStorageWorkResp != NULL )
                m_fnpMngStorageWorkResp(ret);
        }
    }
*/
    return 0;
}

void SetMngStorageState(int32_t nState)
{
    m_nMngStorageState = nState;
}

boolean_t SaveReport(stMsgStorage* pstStorageMessage)
{
    char carrPath[256];
#ifdef FILE_LOG
	printf("SaveReport\r\n");
#endif

    Trace("Save report\r\n");

#ifdef USE_OLD_FILE_PROCESS
    if( WriteFile(FILE_NAME_REPORT,(int8_t*)pstStorageMessage,sizeof(stMsgStorage)) == GIT_FR_OK )
#else
		m_bUsedStorage = true;
    if( WriteAutoLinkData((char*)pstStorageMessage,sizeof(stMsgStorage),carrPath) == GIT_FR_OK )
#endif
    {
        //ShowDir((char *)carrPath);
        //Trace("Write File Size : %d\r\n",nFileSize);
		m_bUsedStorage = false;
        return true;
    }
    else
    {
    	printf("========================================\r\n");
    	printf("ERROR / file write\r\n");
        GIT_Assert(false,eErrorCodeStg|eWriteError);
		TestFormat();
    }
m_bUsedStorage = false;
    return false;
}

void SetBlockStorageTransfer(boolean_t bStop)
{
    m_bSysBlockTransfer = bStop;
}

boolean_t GetBlockStorageTransfer()
{
    return m_bSysBlockTransfer;
}

boolean_t ReadReport(stMsgStorage* pstStorageMessage)
{
    Trace("Read report\r\n");
#ifdef FILE_LOG
	printf("ReadReport\r\n");
#endif

#ifdef USE_OLD_FILE_PROCESS
    if( ReadFile(FILE_NAME_REPORT,(int8_t*)pstStorageMessage,sizeof(stMsgStorage)) == GIT_FR_OK )
#else
		m_bUsedStorage = true;
    if( ReadAutolinkAsset(FILE_MODE_READ,
        m_stLatestReadFileInfo.m_carrLatestFolderPath,
        m_stLatestReadFileInfo.m_carrLatestFilePath,
        (char*)pstStorageMessage,
        sizeof(stMsgStorage)) == GIT_FR_OK )
#endif
    {
        char carrPath[256];
        sprintf((char *)carrPath,"/%s/%s\x00","AutoLinkData",m_stLatestReadFileInfo.m_carrLatestFolderPath);
        ShowDir((char *)carrPath);
/*
        if( IsFileEmpty(m_stLatestReadFileInfo.m_carrLatestFullFilePath) == true )
        {
            //delete file because read data size is zero
            DeleteAutolinkFile(m_stLatestReadFileInfo.m_carrLatestFullFilePath);

            DeleteAutolinkFolder(m_stLatestReadFileInfo.m_carrLatestFolderPath);

            if( SearchLatestFile(&m_stLatestReadFileInfo) == 0 )
            {
                Trace("Success Search File Path : %s\r\n",m_stLatestReadFileInfo.m_carrLatestFullFilePath);
            }
        }
*/
		m_bUsedStorage = false;
        return true;
    }
    else
    {
        GIT_Assert(false,eErrorCodeStg|eReadError);
    }
	m_bUsedStorage = false;
    return false;
}

boolean_t DeleteReport(stMsgStorage* pstStorageMessage)
{
#ifdef FILE_LOG
	printf("DeleteReport\r\n");
#endif
    Trace("Delete report\r\n");
#ifdef USE_OLD_FILE_PROCESS
    if( ReadFile2(FILE_NAME_REPORT,(int8_t*)pstStorageMessage,sizeof(stMsgStorage)) == GIT_FR_OK )
#else
	m_bUsedStorage = true;
    if( ReadAutolinkAsset(FILE_MODE_READ_AND_DELETE,
        m_stLatestReadFileInfo.m_carrLatestFolderPath,
        m_stLatestReadFileInfo.m_carrLatestFilePath,
        (char*)pstStorageMessage,
        sizeof(stMsgStorage)) == GIT_FR_OK )
#endif
    {
        char carrPath[128];
        Trace("Delete Success\r\n");

#if false
        //if( IsFileEmpty2(m_stLatestReadFileInfo.m_carrLatestFolderPath,m_stLatestReadFileInfo.m_carrLatestFilePath) == true )
        if( IsFileEmpty(m_stLatestReadFileInfo.m_carrLatestFullFilePath) == true )
        {
            Trace("already deleted\r\n");
            //delete file because read data size is zero
            //DeleteAutolinkFile(m_stLatestReadFileInfo.m_carrLatestFullFilePath);
            //DeleteAutolinkFolder(m_stLatestReadFileInfo.m_carrLatestFolderPath);

            if( IsFolderEmpty(m_stLatestReadFileInfo.m_carrLatestFolderPath) == true )
            {
                DeleteAutolinkFolder(m_stLatestReadFileInfo.m_carrLatestFolderPath);
            }


            //            if( SearchLatestFile(&m_stLatestReadFileInfo) == true )
            //            {
            //                Trace("Success Search File Path : %s\r\n",m_stLatestReadFileInfo.m_carrLatestFullFilePath);
            //            }
            //            else
            //            {
            //                Trace("Fail to search file\r\n");
            //            }
        }
#endif
        sprintf((char *)carrPath,"%s/%s","AutoLinkData",m_stLatestReadFileInfo.m_carrLatestFolderPath);
        ShowDir((char *)carrPath);
        m_bUsedStorage = false;
        return true;
    }
	m_bUsedStorage = false;
    return false;
}

boolean_t DeleteLastReportFile(stMsgStorage* pstStorageMessage)
{
#ifdef FILE_LOG
	printf("DeleteLastReportFile\r\n");
#endif

	m_bUsedStorage = true;
    if( ReadAutolinkAsset(FILE_MODE_DELETE,
        m_stLatestReadFileInfo.m_carrLatestFolderPath,
        m_stLatestReadFileInfo.m_carrLatestFilePath,
        (char*)pstStorageMessage,
        sizeof(stMsgStorage)) == GIT_FR_OK )
    {
        char carrPath[MAX_FILE_NAME_LENGTH];
        Trace("Delete Success\r\n");

        //if( IsFileEmpty2(m_stLatestReadFileInfo.m_carrLatestFolderPath,m_stLatestReadFileInfo.m_carrLatestFilePath) == true )
        if( IsFileEmpty(m_stLatestReadFileInfo.m_carrLatestFullFilePath) == true )
        {
            Trace("already deleted\r\n");
            //delete file because read data size is zero
            //DeleteAutolinkFile(m_stLatestReadFileInfo.m_carrLatestFullFilePath);
            //DeleteAutolinkFolder(m_stLatestReadFileInfo.m_carrLatestFolderPath);

            //if( IsFolderEmpty(m_stLatestReadFileInfo.m_carrLatestFolderPath) == true )
       		sprintf((char *)carrPath,"%s/%s",AUTOLINKDATA_BASE_DIR, m_stLatestReadFileInfo.m_carrLatestFolderPath);
            if( IsFolderEmpty(carrPath) == true )
            {
                DeleteAutolinkFolder(m_stLatestReadFileInfo.m_carrLatestFolderPath);
            }
        }

        //sprintf((char *)carrPath,"%s/%s",AUTOLINKDATA_BASE_DIR, m_stLatestReadFileInfo.m_carrLatestFolderPath);
//        ShowDir((char *)carrPath);
        m_bUsedStorage = false;
        return true;
    }
	m_bUsedStorage = false;
    return false;
}


boolean_t IsExistData()
{
#ifdef USE_OLD_FILE_PROCESS
    int32_t nFileSize;
    boolean_t bResult = CheckFile(FILE_NAME_REPORT,&nFileSize);
    //Trace("result : %x\r\n",bResult);
    return bResult;
#endif
m_bUsedStorage = true;
#ifdef FILE_LOG
	printf("IsExistData\r\n");
#endif

    if( SearchLatestFile(&m_stLatestReadFileInfo) == true )
    {
        Trace("Success Search File Path : %s\r\n",m_stLatestReadFileInfo.m_carrLatestFullFilePath);

        //if( IsFileEmpty2(m_stLatestReadFileInfo.m_carrLatestFolderPath,m_stLatestReadFileInfo.m_carrLatestFilePath) == true )
        if( IsFileEmpty(m_stLatestReadFileInfo.m_carrLatestFullFilePath) == true )
        {
            //delete file because read data size is zero
            DeleteAutolinkFile(m_stLatestReadFileInfo.m_carrLatestFullFilePath);

            DeleteAutolinkFolder(m_stLatestReadFileInfo.m_carrLatestFolderPath);

            //if( SearchLatestFile(&m_stLatestReadFileInfo) == true )
            //{
            //    Trace("Success Search File Path : %s\r\n",m_stLatestReadFileInfo.m_carrLatestFullFilePath);
            //}
m_bUsedStorage = false;
            return false;
        }
m_bUsedStorage = false;
        return true;
    }

m_bUsedStorage = false;
    return false;
}

void ClearLastSendReportStatus()
{
    m_bSendReport = false;
}

boolean_t GetLastSendReportStatus()
{
    return m_bSendReport;
}

void SetLastSendReport(stMsgStorage* pstMsgStorage)
{
    m_bSendReport = true;
    memcpy((int8_t*)&m_stStorageSendLastMessage,(int8_t*)pstMsgStorage,sizeof(stMsgStorage));
}

boolean_t CheckIsthisLastSendReport(stMsgStorage* pstCurrentStorage)
{
    if( memcmp((int8_t*)&m_stStorageSendLastMessage.carReport,(int8_t*)&pstCurrentStorage->carReport,sizeof(stCarReport)) == 0 )
    {
		// MONI20190102 this flag make retry very long term.
        //m_bSendReport = false;

        return true;
    }

    // stop auto send proces because mismatch occurred
    m_bAutoReadFlag = false;

    return false;
}

boolean_t IsIsPossible2Transfer()
{
    if( GetBlockStorageTransfer() == true )
    {
        Trace("#################################################\r\n");
        Trace("System block to transfer\r\n");
        return false;
    }

    return true;
}

unsigned long m_ulReadTime= 0;
unsigned long m_ulWriteTime= 0;
unsigned long m_ulDeleteTime= 0;

uint32_t m_unModemNoResonseCount = 0;
uint32_t m_unSaveCount = 0;
void IncreaseModemNoResponseCount()
{
    m_unModemNoResonseCount++;
}

void ClearModemNoResponseCount()
{
    m_unModemNoResonseCount = 0;
}

uint32_t GetModemNoResponseCount()
{
    return m_unModemNoResonseCount;
}

long long m_llStorageDrivingKey;

void SetStorageDrivingKey(long long llDrivingKey)
{
    m_llStorageDrivingKey = llDrivingKey;
}

void GetStorageDrivingKey(long long* pllDrivingKey)
{
    *pllDrivingKey = m_llStorageDrivingKey;
}

// external event handler
void MngStorageExternalEvent(stMsgStorage* pstMsgStorage)
{
    if( pstMsgStorage->header.id == eMngSys )
    {
        if( pstMsgStorage->header.event == eReqStorageBlockTrasfer )
        {
            if( pstMsgStorage->header.subEvent == eBlockStart )
            {
                Trace("System send an event to block to transfer\r\n");
                SetBlockStorageTransfer(true);
            }
            else
            {
                Trace("System send an event no to block to transfer\r\n");
                SetBlockStorageTransfer(false);
            }
        }
    }
    else if( pstMsgStorage->header.id == eMngSysMsg )
    {
        if( pstMsgStorage->header.event == eReqSleep )
        {
            // if other manager wants to go sleep, this mananger request upper manager
            // and wait for event from upper manager.
            SetMngStorageState(STAT_MNG_STORAGE_SLEEP_WAIT);
            Trace("Get Sleep Event from other Manager\r\n");
        }
        else if( pstMsgStorage->header.event == eReqForcelySleep )
        {
            // go to sleep forcely
        }
        else if( pstMsgStorage->header.event == eReqSaveReport )
        {
            Trace("Save Date\r\n");
            m_ulWriteTime = OemGetTmr();

//			printf("pstMsgStorage->header.subevent : %d\n",pstMsgStorage->header.subEvent);
            // if modem activation is false
            // we should block all message to send the modem manager.
            if( IsIsPossible2Transfer() == false )
            {
                Trace("#################################################\r\n");
                Trace("Transfer is not activated\r\n");
                Trace("#################################################\r\n");
                return;
            }

            // save data to serial flash
            SaveReport(pstMsgStorage);
			//DisplayReport("SaveReport",(stMsgSysMsg*)pstMsgStorage);
#if defined(PROTOCOL17)
			if( pstMsgStorage->header.subEvent == eR_ReqSetURLSave || pstMsgStorage->header.subEvent == eR_ReqSetURLInit )
			{
				Git_DeviceReset(NULL,0);
			}
#endif
            SetMngStorageState(STAT_MNG_STORAGE_SAVE);

            Send2MngSysMsg(eMngStorage,eRspDataExist,true,(stCarReport*)NULL,0);

			//210311 lwh ProcessFuelCellInfo에서 모뎀으로 보낼지 저장할지 판단하는 시점과 ModemProcess에서는 다음 데이터가 없다고 판단하는 시점이 맞지 않는 문제 수정
			//if( m_bSuspendStorage == false && m_bSendReport == false && m_bNetworkConnected == true && IsIsPossible2Transfer()==true && IsItPossibleSendingMessage() == true )
			//{
			Send2MngStorage(eMngSysMsg,eReqStartAutoRead,0,(stCarReport *)NULL,0);
			//}

            // log
            //DisplayReport("mngStorage]Save Data\n",(stMsgSysMsg*)pstMsgStorage);

			Trace("Write Time : %d\r\n",OemGetTmrDelta(OemGetTmr(), m_ulWriteTime));

            if( m_bNetworkConnected == true )
            {
                // this is excpetional case

                if( pstMsgStorage->header.subEvent != eR_Alram )
                    m_unSaveCount++;
#ifdef QA_FIFA
				if( m_unSaveCount > 18 )
#else
                if( m_unSaveCount > 5 )
#endif
                {
                    Trace("This is weired states go recovery\r\n");
                    ClearModemNoResponseCount();
                    ClearLastSendReportStatus();
                    Send2MngModem(eMngStorage,eReqRecovery,eTrue,(stCarReport*)NULL,0);

                    m_unSaveCount = 0;
                }
            }
            else
            {
                m_unSaveCount = 0;
            }
        }
        else if( pstMsgStorage->header.event == eReqGetReport)
        {
            boolean_t bExistData;

            if( m_bSuspendStorage == true )
            {
                //Trace("====================================================\r\n");
                //Trace("Data is suspended by system.\r\n");
                return;
            }

            m_ulReadTime = OemGetTmr();

//#warning "we need to add event according to the data type"
            // read data from serial flash
            SetMngStorageState(STAT_MNG_STORAGE_READ);
            bExistData = IsExistData();

            // we should block all message to send the modem manager.
            if( IsIsPossible2Transfer() == false )
            {
                Trace("#################################################\r\n");
                Trace("Transfer is not activated\r\n");
                Trace("#################################################\r\n");
                return;
            }

            Trace("Get Data\r\n");
            Trace("netwrok : %d, bExistData : %d\r\n",m_bNetworkConnected,bExistData);
            Trace("GetLastSendReportStatus : %d\r\n",GetLastSendReportStatus());
#ifdef ONLY_SAVE_MODE
            extern bool m_bRecoveryMessage;
			if( m_bNetworkConnected == true && bExistData == true &&
				GetLastSendReportStatus() == false && m_bRecoveryMessage == false )
			//if exist RecoveryMessage do not ReadReport
#else
            if( m_bNetworkConnected == true && bExistData == true &&
                GetLastSendReportStatus() == false )
#endif
            {
                boolean_t bResult = ReadReport(pstMsgStorage);
				static bool s_bFotaAlarm = false;
                if( bResult == true )
                {
#ifdef USE_WRONG_DRIVING_KEY_RESTORE
                    // MONI 20180429
                    // policy is changed
                    // if ME received wrong driving key, that data is discard
                    if( pstMsgStorage->header.unEventTime == 0 )
                    {
                        long long llTemp;
                        uint32_t unUtcTime = GetUTCTime();
						uint32_t unLocalTime = GetLocalTimefromTime(unUtcTime);

                        // intialize locatl/event time because this time was initialized time when it had generated
                        pstMsgStorage->header.unEventTime = unLocalTime;

                        GetStorageDrivingKey(&llTemp);
                        pstMsgStorage->header.drivingKey = llTemp;

                        switch(pstMsgStorage->header.subEvent)
                        {
                            case eR_Alram:
#if defined(PROTOCOL18)
                            case eR_Charging:
#endif
                            case eR_BeforeDriving:
                            case eR_AlramDTC:
                            case eR_ImpulseAlram:
                                pstMsgStorage->carReport.rpAlram.CarStatus.OperationKey = pstMsgStorage->header.drivingKey;
                                pstMsgStorage->carReport.rpAlram.CarStatus.OccurredEventTime = unLocalTime;
                                pstMsgStorage->carReport.rpAlram.CarStatus.OccurredEventUtcTime = unUtcTime;
                                break;
                            case eR_DrivingInterval:
                                pstMsgStorage->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OperationKey = pstMsgStorage->header.drivingKey;
                                pstMsgStorage->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OccurredEventTime = unLocalTime;
                                pstMsgStorage->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OccurredEventUtcTime = unUtcTime;
                                pstMsgStorage->carReport.rpInterval.DrivingInfo.DrivingInfoB2.OperationKey = pstMsgStorage->header.drivingKey;
                                pstMsgStorage->carReport.rpInterval.DrivingInfo.DrivingInfoB2.OccurredEventTime = unLocalTime;
                                pstMsgStorage->carReport.rpInterval.DrivingInfo.DrivingInfoB2.OccurredEventUtcTime = unUtcTime;
                                break;
                            case eR_AfterDriving:
                                pstMsgStorage->carReport.rpInterval.AfterDrivingInfo.OperationKey = pstMsgStorage->header.drivingKey;
                                pstMsgStorage->carReport.rpInterval.AfterDrivingInfo.StartDrivingLocalTime = unLocalTime;
                                pstMsgStorage->carReport.rpInterval.AfterDrivingInfo.StartDrivingUTCTime = unUtcTime;
                                break;
                            case eR_ParkingInterval:
                                pstMsgStorage->carReport.rpInterval.NoDrivingInfo.OccurredEventTime = unLocalTime;
                                pstMsgStorage->carReport.rpInterval.NoDrivingInfo.OccurredEventUtcTime = unUtcTime;
                                break;
                            case eR_AlramMasking:
                            case eR_ReqSmartKey:
                            case eR_RspSmartKey:
                            case eR_CurrentVehicleStatus:
                            case eR_SettingGeofence:
                            case eR_ReqUserActionSetting:
                            case eR_RspUserActionSetting:
                            case eR_SmsTest:
                            case eR_ReqSettingRsvEngCtrl:
#if defined(PROTOCOL24)
							case eR_ReportNetworkStatus:
#endif 								
#if defined(PROTOCOL25)
                            case eR_ReportInstallationNetworkCheck:
                            case eR_ReportInstallationSMSCheck:
#endif
                                break;
                        }
                    }
#endif //#ifdef USE_WRONG_DRIVING_KEY_RESTORE

#ifdef USE_WRONG_DRIVING_KEY_RESTORE
                    if( pstMsgStorage->header.drivingKey == 0 &&
                        (pstMsgStorage->header.subEvent == eR_BeforeDriving ||
                        pstMsgStorage->header.subEvent == eR_DrivingInterval) )
                    {
                        Trace("Driving Key is weired, discard\r\n");
                        return ;
                    }
#endif //#ifdef USE_WRONG_DRIVING_KEY_RESTORE


                    //FOTA 완료 알람 전송 시 이벤트 키 값 SIZE 증가로 인한 이상 데이터 해결
					if( pstMsgStorage->carReport.rpAlram.CarStatus.EventKey == eMESSAGE_EVENT_KEY_FOTA_COMPLETE_ALRAM )
					{
						if((pstMsgStorage->carReport.rpAlram.CarStatus.EventKeyValue[0] == 'T' || pstMsgStorage->carReport.rpAlram.CarStatus.EventKeyValue[0] == 'R') && (s_bFotaAlarm == false))
						{
							s_bFotaAlarm = true;

							DeleteReport(pstMsgStorage);
			                SetMngStorageState(STAT_MNG_STORAGE_DELETE);
			                ClearModemNoResponseCount();
							if(pstMsgStorage->carReport.rpAlram.CarStatus.EventKeyValue[0] == 'T')
							{
							 	SendFotaAlramReport(eMESSAGE_EVENT_KEY_FOTA_COMPLETE_ALRAM,eREMOTE_CON_FOTA_TYPE_TEST);
							}
							else if(pstMsgStorage->carReport.rpAlram.CarStatus.EventKeyValue[0] == 'R')
							{
								SendFotaAlramReport(eMESSAGE_EVENT_KEY_FOTA_COMPLETE_ALRAM,eREMOTE_CON_FOTA_TYPE_UPDATE);
							}
						}
						else
						{
							pstMsgStorage->header.id = eMngStorage;
                    		pstMsgStorage->header.event = eReqReport;
                    		Send2MngModem2((stMsgMdm*)pstMsgStorage);

                    		SetLastSendReport(pstMsgStorage);
							//DisplayReport("ReadReport",(stMsgSysMsg*)pstMsgStorage);
						}
					}
					else
					{
						pstMsgStorage->header.id = eMngStorage;
                    	pstMsgStorage->header.event = eReqReport;
                    	Send2MngModem2((stMsgMdm*)pstMsgStorage);

                    	SetLastSendReport(pstMsgStorage);
						//DisplayReport("ReadReport",(stMsgSysMsg*)pstMsgStorage);
					}

                    //2018-03-14
                    // if data exist, then ME start autosend.
                    m_bAutoReadFlag = true;
                    //m_bAutoReadCount = 0;
                    m_unSaveCount = 0;
                }
                else
                {
                    GIT_Assert(false,eErrorCodeStg|eReadError);
                }
            }


            if( GetLastSendReportStatus() == true )
            {
                IncreaseModemNoResponseCount();

                if( GetModemNoResponseCount() > 10 )
                {
                    Trace("####### Storage Recovery Process\r\n");

                    //DisplayModemStatus();
                    //MONI 2018-03-14
                    // this is special thing
                    // we must change this routine for recovery modem.
                    // request modem reinitialize
                    // SetRequestActionFromManager(eMdmReqGemaltoReset,true);


                    //ClearLastSendReportStatus();
                    //SetWaitforSendingData(false);
                    //SetNetworkPostPoneFlag(false);
                    //SetLastSendMessageFlag(false);
                    //DisplayModemStatus();

                    ClearModemNoResponseCount();
                    ClearLastSendReportStatus();
                    Send2MngModem(eMngStorage,eReqRecovery,eTrue,(stCarReport*)NULL,0);
                }
            }

            //DisplayReport("mngStorage]Read Data\n",(stMsgSysMsg*)pstMsgStorage);

            Send2MngSysMsg(eMngStorage,eRspDataExist,bExistData,(stCarReport*)NULL,0);

//            printf("Read Time : %d\r\n",OemGetTmrDelta(OemGetTmr(), m_ulReadTime));
        }
/*        else if( pstMsgStorage->header.event == eReqDeleteReport)
        {
            DeleteReport(&pstMsgStorage->carReport);
            // delete data of serial flash
            SetMngStorageState(STAT_MNG_STORAGE_DELETE);

            Send2MngSysMsg(eMngStorage,eRspDeleteReport,0,(stCarReport*)NULL);
            Send2MngSysMsg(eMngStorage,eRspDataExist,IsExistData(),(stCarReport*)NULL);
        }
*/
        else if( pstMsgStorage->header.event == eReqDataExist )
        {
            boolean_t bExistData = IsExistData();

            Send2MngSysMsg(eMngStorage,eRspDataExist,bExistData,(stCarReport*)NULL,0);
        }
        else if( pstMsgStorage->header.event == eMdmStatus )
        {
            if( pstMsgStorage->header.subEvent == eMS_ServerConnected )
            {
                m_bNetworkConnected = true;
            }
            else if( pstMsgStorage->header.subEvent == eMS_ServerDisconnected )
            {
                m_bNetworkConnected = false;
            }
        }
        else if( pstMsgStorage->header.event == eReqStartAutoRead )
        {
			m_bAutoReadFlag = true;
        }
        else if( pstMsgStorage->header.event == eReqStopAutoRead )
        {
            m_bAutoReadFlag = false;
        }
        else if( pstMsgStorage->header.event == eReqSetDrivingKey )
        {
            SetStorageDrivingKey(pstMsgStorage->header.drivingKey);
        }
        else if( pstMsgStorage->header.event == eReqSuspend )
        {
            if( pstMsgStorage->header.subEvent == eSuspendStart )
            {
                m_bSuspendStorage = true;
            }
            else if(pstMsgStorage->header.subEvent == eSuspendStop)
            {
                m_bSuspendStorage = false;

                if( IsExistData() == true )
                {
                    m_bAutoReadFlag = true;
                    //m_bAutoReadCount = 0;

                    Trace("Auto data sent process is started\r\n");
                }
            }
        }
        else
        {
            GIT_Assert(false,eErrorCodeStg|eEventUnknown);
        }
    }
    else if( pstMsgStorage->header.id == eMngModem )
    {
        if( pstMsgStorage->header.event == eRspReport )
        {
            Trace("Response from modem\r\n");

			//MONI 20190102 add this flag for preventing retry.
			m_bSendReport = false;

            if( pstMsgStorage->header.result == eSuccess &&
                CheckIsthisLastSendReport(pstMsgStorage) == true )
            {
                m_ulDeleteTime = OemGetTmr();

                // delete data of serial flash
                DeleteReport(pstMsgStorage);
                SetMngStorageState(STAT_MNG_STORAGE_DELETE);

                ClearModemNoResponseCount();

                Trace("Delete Time : %d\r\n",OemGetTmrDelta(OemGetTmr(), m_ulDeleteTime));
            }
            else
            {
                Trace("----------------------------------------\r\n");
                Trace("Data not mached in storage %d\r\n",pstMsgStorage->header.result);
                Trace("----------------------------------------\r\n");
                DisplayReport("mngStorage]Req Data\r\n",(stMsgSysMsg*)&m_stStorageSendLastMessage);
                DisplayReport("mngStorage]Rsp Data\r\n",(stMsgSysMsg*)pstMsgStorage);
            }
        }
        else if( pstMsgStorage->header.event == eReqDeleteFile )
        {
            if( pstMsgStorage->header.result == eTrue )
            {
                Trace("============================================\r\n");
                Trace(" File was broken / Delete last file  \r\n");
                Trace("============================================\r\n");
                ClearModemNoResponseCount();

                DeleteLastReportFile(pstMsgStorage);
                SetMngStorageState(STAT_MNG_STORAGE_DELETE);
#ifdef ONLY_SAVE_MODE
				Send2MngStorage(eMngSysMsg,eReqStartAutoRead,0,(stCarReport *)NULL, 0);
				m_bSuspendStorage = false;
				m_bAutoReadFlag = true;
				m_bSendReport = false;
#endif
            }
        }
    }
    else
    {
        // not defined
        // error
        GIT_Assert(false,eErrorCodeStg|eUnknownId);
    }
}
