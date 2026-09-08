/* Includes ------------------------------------------------------------------*/

#include <stdlib.h>
#include <math.h>

#include "GIT_Util.h"
#include "Message_Manager.h"
#include "Modem_Manager.h"
#include "OBD_Manager.h"

#include "AutolinkConfiguration.h"
#include "AutolinkMessage.h"

#include "MngSystem.h"
#include "MngQueue.h"
#include "MngModem.h"
#include "HdDebug.h"
#include "GIT_OemInterface.h"
#include "MngSystemUtil.h"
#include "Modem_comm.h"
#include "GIT_Gps.h"


#define Trace(...)  GITDebug(DEBUG_MODULES_MODEM,__VA_ARGS__)

extern void ModemRcvProcess();
extern void ModemReportHandler(int32_t nEventType, stMsgMdm* pstMsgMdm);
extern void WriteModemConfiguration(boolean_t bRecoveryMessage, stMsgMdm stRecoveryMessage);
extern void ReadModemConfiguration(boolean_t* pbRecoveryMessage, stMsgMdm* pstRevoeryMessage);
extern void ModemIpekHandler(int32_t iEventType, stMsgMdm* pstMsgMdm);
extern void ClearRequestActionFromManagerFlag();
extern void ModemAgpsHandler(int32_t iEventType, stMsgMdm* pstMsgMdm);
extern void SendAlramSMSExpire(stMsgMdm* pstMsgModem,int32_t nEvent, int32_t nResult);


extern void RequestNetworkTime();
extern void RequestRssi();

extern bool m_bPostponeReqSleep;
extern bool g_bOBDSleepIntoFlag;
extern boolean_t m_bSuspendStorage;
extern boolean_t m_bSendReport;
extern stMsgHandlerData m_stMsgHdData;

void EvtHdModemfromSystem(stMsgMdm* pstMsgModem);
void EvtHdModemfromObd(stMsgMdm* pstMsgModem);
void EvtHdModemfromStorage(stMsgMdm* pstMsgModem);

void SendResultsLastMessage(int32_t nValue, stMsgMdm* pstMessage);

void SetMngMdmState(int32_t nStatus);
void MngMdmExternalEvent(stMsgMdm* pstMsgModem);

boolean_t m_bRemoteControlStatus = false;

void SetRemoteControlStatus(boolean_t bRemoteControlStatus);

void RequestSleepTimeoutTimerCallBack();
void WakeupResponseTimerCallBack();

boolean_t GetLastSendMessage(stMsgMdm* pstMsgMdm);
boolean_t GetRcvSmsStatus();
boolean_t GetLastSendMessageFlag();
void SetLastSendMessageFlag(boolean_t bLastSendMessageFlag);

boolean_t IsModemConnected();

void SetModemControlEvent(int32_t nStatus);
void SetRcvSmsStatus(boolean_t bStatus);
void SetMngMdmState(int32_t nState);
void SetLastSendMessage(stMsgMdm* pstMsgMdm);

boolean_t GetNetworkPostPondeFlag();
void SetNetworkPostPoneFlag(boolean_t bPostPonded);

int32_t MngModem(int32_t nLparam, int32_t nRparam);
boolean_t GetModemNotWorkingFlag();
void SetModemNotWorkingFlag(boolean_t bModemNotWorkingFlag);
boolean_t IsModemAvailable();
boolean_t GetWaitforSendingData();
void SetWaitforSendingData(boolean_t bWaitforSendingData);
int8_t GetSeverErrorCount();
void ClearServerErrorCount();
void IncreaseServerErrorCount();

void ClearRetransmisionCount();
uint8_t GetReTransmisionCount();
void Retransmision(stMsgMdm stMessage);

void SetUserApn(char* apn);
boolean_t GetUserApn(char* pstrApn);

void SetReceivedSmsInfo(stMsgMdm* pstMsgModem);
boolean_t GetReceviedSms();
void SetReceivedSms(boolean_t bStatus);

boolean_t GetReqWaitNetworkCheck();

void ReqForcelyModemReset();

int16_t m_usRequestSendResult = -1;
extern unsigned long m_ulModemLedBlinkTime;
int8_t m_cServerErrorCount;
stMsgMdm m_stLastRcvSmartKey;

extern int8_t *GetStringFromEvent(int32_t nMode,int32_t nEvent,int32_t nSubEvent);
extern void ModemFotaHandler(int32_t iEventType, stMsgMdm* pstMsgMdm);
extern int8_t *GetStringFromAlramEvent(int32_t nId);
extern int8_t *GetStringFromId(int32_t nId);
extern void SetRequestActionFromManager(int32_t nRequestActionFromManager, boolean_t bForce);
extern uint32_t GetUTCTime();

void Send2MngModemHandler(uint16_t iId, int32_t iEvent,int32_t iSubEvent,boolean_t bForce);
void WaitGeneralErrorTimerCallBack();

uint32_t GetModemControlOldState();
boolean_t GetModemAvailableFlag();

int32_t m_nMngMsgMdmState = STAT_MNG_MDM_INIT;

int32_t m_nReqSleepTimeoutTimerID = -1;
int32_t m_nReqWaitNetworkStableTimeoutTimerID = -1;

int8_t m_cModemResetRetryCount;
boolean_t m_bRcvSms = false;
boolean_t m_bReqSleep = false;
boolean_t m_bReqDoNothingForSleep = false;

boolean_t m_bSentLastMessage;
boolean_t m_bRecoveryMessage;
uint8_t m_cRecoveryRetryCount;

stMsgMdm m_stLastMessageModem;
stMsgMdm m_stRecoveryMessage;

boolean_t m_bReqNetworkPostPone = false;
boolean_t m_bModemNotWorkingFlag = false;

int32_t m_nReqSleepCount=0;
boolean_t m_bForcelySleep = 0;

unsigned long m_ulOldStartSendReportTime = 0;

boolean_t m_bReqModemPowerOff = false;

MODEM_STATE_STRUCTURE g_ModemStateStructure;
MODEM_STATE_COLLECTION g_ModemStateMessage;

void ReadBkRamForModem()
{
    Trace("Import Recovery Data\r\n");
//    m_bRecoveryMessage;
//    m_bRecoveryRetryCount;
//    m_stRecoveryMessage;
    ReadModemConfiguration(&m_bRecoveryMessage,&m_stRecoveryMessage);
}
void WriteBkRamForModem()
{
//    m_bRecoveryMessage;
//    m_bRecoveryRetryCount;
//    m_stRecoveryMessage;
    if( m_bRecoveryMessage == true )
    {
        WriteModemConfiguration(m_bRecoveryMessage,m_stRecoveryMessage);
    }
    else if( m_bSentLastMessage == true )	// 저장된 데이터가 아닌 데이터를 모뎀에서 서버로 전달 하는중에 Critcal error 발생시 슬립진입시 저장하여 보내도록 수정
	{
		if( (m_stLastMessageModem.header.event!=eReqAgps) && (m_stLastMessageModem.header.event!=eReqIpek) && (m_stLastMessageModem.header.event!=eReqFota) )
		{
			m_bRecoveryMessage = true;
			//printf("m_stLastMessageModem.header.event:%x,m_stLastMessageModem.header.subEvent:%x,%x\r\n",m_stLastMessageModem.header.event,m_stLastMessageModem.header.subEvent,m_stLastMessageModem.header.id);
			//printf("ModemManagerData.eFOTAStartState:%d\r\n",ModemManagerData.eFOTAStartState);
			WriteModemConfiguration(m_bRecoveryMessage,m_stLastMessageModem);
		}
	}
}

void ModemInitalize()
{
    m_nReqSleepCount = 0;
    m_bReqSleep = false;
    m_bReqNetworkPostPone = false;
    m_cRecoveryRetryCount = 0;
    m_bRecoveryMessage = false;
    m_bRcvSms = false;
    m_cModemResetRetryCount = 0;

    m_nMngMsgMdmState = STAT_MNG_MDM_INIT;
    
    g_ModemStateStructure.ModemStateCollection = (MODEM_STATE_COLLECTION *)malloc(sizeof(MODEM_STATE_COLLECTION)*MAX_MODEMSTATEBUFFER_SIZE);
    
    if ( g_ModemStateStructure.ModemStateCollection != NULL)
    {
        memset(g_ModemStateStructure.ModemStateCollection,0x00,sizeof(MODEM_STATE_COLLECTION)*MAX_MODEMSTATEBUFFER_SIZE);
    }
    else
    {
        printf("Size = %d\r\n",sizeof(MODEM_STATE_COLLECTION)*MAX_MODEMSTATEBUFFER_SIZE);
        printf("%s : malloc fail so return false \r\n", __FUNCTION__);
    }
        
#ifdef CHECK_MALLOC
    printf("%s\r\n",__FUNCTION__);
    __iar_dlmalloc_stats();
#endif

    // added recovery modem status before sleep status
    // 1. retry status
    // 2. last message
    ReadBkRamForModem();
}

boolean_t IsModemAvailable()
{
    if( GetWaitforSendingData() == false && m_bRecoveryMessage == false && GetNetworkPostPondeFlag() == false &&
        IsModemConnected() == true &&  !GetLastSendMessageFlag() &&
        GetReqWaitNetworkCheck() == false/* && GetReceviedSms()== false */ )
        return true;

    return false;
}

#if defined(PROTOCOL24)
int ReportModemStatus(unsigned char ucModemConnectStatus)
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

	msg.rpModemStatus.GpsValid  = Get_GPS_Vailication();
	msg.rpModemStatus.Latitude  = Get_GPS_Lat();
    msg.rpModemStatus.Longitude = Get_GPS_Lon();
	
	msg.rpModemStatus.ModemConnectStatus = ucModemConnectStatus;

	memcpy(msg.rpModemStatus.ModemStatus, GetSMONIData(), MAX_MODEM_STATUS_LENGTH/2);
	
	msg.rpModemStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.rpModemStatus.OccurredEventUtcTime = GetUTCTime();

    Send2MngSysMsg(eMngModem, eReqReport, eR_ReportNetworkStatus, &msg,0);
	
    return 0;    
}
#endif

boolean_t m_bWaitforSendingData = false;

void SetWaitforSendingData(boolean_t bWaitforSendingData)
{
    m_bWaitforSendingData = bWaitforSendingData;
}

boolean_t GetWaitforSendingData()
{
    return m_bWaitforSendingData;
}

void DisplayModemStatus()
{
    Trace("GetWaitforSendingData() : %d, m_bRecoveryMessage : %d, GetNetworkPostPondeFlag() : %d\r\n",GetWaitforSendingData(),m_bRecoveryMessage,GetNetworkPostPondeFlag());
    Trace("IsModemConnected() : %d, !GetLastSendMessageFlag() : %d\r\n",IsModemConnected(),!GetLastSendMessageFlag());
}


boolean_t IsModemSleepWaitState()
{
    if( m_nMngMsgMdmState == STAT_MNG_MDM_SLEEP_WAIT )
    {
        Trace("modem is sleep wait\r\n");
        return true;
    }
    return false;
}

int32_t MngModem(int32_t nLparam, int32_t nRparam)
{
    stMsgMdm stTmpMessage;

    // check external event
#ifndef GLOBAL_SHARE_QUEUE //Get
    if( MngQueueGetMessage(ID_MNG_QUEUE_MDM, (int8_t*)&stTmpMessage, sizeof(stMsgMdm)) == true )
#else
	if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_MDM, (uint8_t*)&stTmpMessage.header, sizeof(stMsgHeader), (uint8_t*)&stTmpMessage.carReport, sizeof(stCarReport)) == true )
#endif
    {
#if defined(MNG_QUEUE_DEBUG)
        Trace("MainExternal event : %s, subEvent : %s, reuslt : %x\r\n",
            GetStringFromEvent(eGetStringEvent,stTmpMessage.header.event,0),
            GetStringFromEvent(eGetStringSubEvent,stTmpMessage.header.event,stTmpMessage.header.subEvent),
            stTmpMessage.header.result);
#endif
        //MONI 2018-02-08
        // we block this code because when modem initialize the modem, the modem will not go to sleep because of this code.
        // we should treat request sleep anytime it is write.
        //if( GetModemState() == eMODEM_READY )
        {
            if( stTmpMessage.header.id == eMngSysMsg &&
                stTmpMessage.header.event == eReqSleep )
            {
                Trace("==============================================\r\n");
                Trace("==============================================\r\n");
                Trace("Rcv : sleep request\r\n");
                // reinitialize request sleep flag.
                m_nReqSleepCount = 0;
                m_bReqSleep = false;
                SetMngMdmState(STAT_MNG_MDM_SLEEP_WAIT);

                SetModemState(eMODEM_ENTER_SLEEP_MODE);

                if( m_bReqDoNothingForSleep == false )
                {
                    Trace("Received request sleep from system. It makes modem go to sleep forcely.\r\n");
                    //m_bReqDoNothingForSleep = true;
                    EnableSystemMessageTimer(&m_nReqSleepTimeoutTimerID,TIMER_REQ_SLEEP_TIMEOUT_VALUE,eSWTimer_ONESHOT,RequestSleepTimeoutTimerCallBack);
                }
                return 0;
            }
            else if( stTmpMessage.header.id == eMngSysMsg &&
                stTmpMessage.header.event == eReqForcelySleep )
            {
                SetMngMdmState(STAT_MNG_MDM_SLEEP_WAIT);

                Trace("go to sleep forcely\r\n");
                //ReqForcelyModemReset();

                m_bForcelySleep = true;
                m_bReqSleep = false;

                Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerDisconnected, (stCarReport *)NULL,0);
                SetModemLedStatus(eModemLedStatus_Disconnect);

                WaitGeneralErrorTimerCallBack();
                return 0;
            }
            else if( stTmpMessage.header.id == eMngStorage )
            {
                if( stTmpMessage.header.event == eReqRecovery )	//스토리지가 보기에 모뎀은 정상인것 같은데 실패지속시 보내는 메시지
                {
                    SetWaitforSendingData(true);
                    SetNetworkPostPoneFlag(false);
                    SetLastSendMessageFlag(false);
                    DisplayModemStatus();

                    SetModemControlEvent(eMngMdmCriticalError);
                    return 0;
                }
            }
//            else if( stTmpMessage.header.id == eMngModem &&
//                stTmpMessage.header.event == eRcvSms )
//            {
//                SetReceivedSmsInfo(&stTmpMessage);
//                return 0;
//            }
        }

        // save all message to the queue
#ifndef GLOBAL_SHARE_QUEUE
        MngQueueSendMessage(ID_MNG_QUEUE_DATA, (int8_t*)&stTmpMessage,sizeof(stMsgMdm));
#else
	    SendSysHdShareQueueMessage(ID_MNG_QUEUE_DATA,(uint8_t*)&stTmpMessage.header,sizeof(stMsgHeader), (uint8_t*)&stTmpMessage.carReport, sizeof(stCarReport));
#endif
    }


    // check external event
    if(  IsModemAvailable() == true &&
#ifndef GLOBAL_SHARE_QUEUE
        ( MngQueueGetMessage(ID_MNG_QUEUE_DATA, (int8_t*)&stTmpMessage, sizeof(stMsgMdm)) == true )
#else
		( GetSysHdShareQueueMessage(ID_MNG_QUEUE_DATA, (uint8_t*)&stTmpMessage.header, sizeof(stMsgHeader), (uint8_t*)&stTmpMessage.carReport, sizeof(stCarReport)) == true )
#endif
	)   //modem에서 발생하는 외부 이벤트 check
    {
#if defined(MNG_QUEUE_DEBUG)
        Trace("%s]SubExternal event : %s, subEvent : %s, from : %s, reuslt : %x\r\n",__FUNCTION__,
            GetStringFromEvent(eGetStringEvent,stTmpMessage.header.event,0),
            GetStringFromEvent(eGetStringSubEvent,stTmpMessage.header.event,stTmpMessage.header.subEvent),
            GetStringFromId(stTmpMessage.header.unTraceMng&0xFF),
            stTmpMessage.header.result);
#endif

        if( stTmpMessage.header.event == eReqReport && stTmpMessage.header.subEvent == eR_Alram )
        {
//            Trace("alram event : %s\n",GetStringFromAlramEvent(stTmpMessage.carReport.rpAlram.CarStatus.EventKey));
        }

        stTmpMessage.header.unTraceMng=stTmpMessage.header.unTraceMng<<8|eMngModem;

        // external event process
        MngMdmExternalEvent(&stTmpMessage);
    }
    else
    {
        if( GetWaitforSendingData() == false && m_bRecoveryMessage == true &&
            GetNetworkPostPondeFlag() == false && IsModemConnected() == true &&
            GetReqWaitNetworkCheck() == false )
        {
            //if( GetReceviedSms() == false )
            //{
                Trace("############################################\r\n");
                Trace("Recovery Start : Count : %d\r\n", m_cRecoveryRetryCount);
                MngMdmExternalEvent(&m_stRecoveryMessage);

                m_bRecoveryMessage = false;
            //}
            //else
            //{
            //    Trace("Handler for sms\n");
            //    // handler received sms
            //    ModemReportHandler(m_stLastRcvSmartKey.header.subEvent, &m_stLastRcvSmartKey);
            //    SetReceivedSms(false);
            //}
        }
    }

    switch(m_nMngMsgMdmState)
    {
        case STAT_MNG_MDM_INIT:
            SetMngMdmState(STAT_MNG_MDM_IDLE);
            
            SetModemState(eMODEM_Idle);
            m_ulModemLedBlinkTime = OemGetTmr();

            SetModemLedStatus(eModemLedStatus_Disconnect);

            break;
        // idle
        case STAT_MNG_MDM_IDLE:
            // check sms status if exist sms or not

            // this function will treat some process after handling an event
            ModemRcvProcess();

        break;
        case STAT_MNG_MDM_SLEEP:
            // do nothing for sleep
            // response sleep request to system message handler to go down.
            //Send2MngSysMsg(eMngModem,eRspSleep,(stCarReport*)NULL);
            break;
        case STAT_MNG_MDM_SLEEP_WAIT:
            // process for sleep

            if( GetModemControlOldState() >= eModemControlStateInit )
            {
                // send event to MngSysMsgHandler
                if( GetModemState() == eMODEM_READY && m_bReqSleep == false )
                {
                    m_nReqSleepCount++;
                    m_bReqSleep = true;

                    // if sim card is not working, then modem couldn't go to sleep
                    // so if fail to go to sleep then change initialize process.
                    if( m_nReqSleepCount < 2 )
                    {
                        //ModemManagerData.eState = eMODEM_READY;
                        //Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqSleepAll,true);
                        //SetRequestActionFromManager(eMdmReqSleepAll,false);
                        //RequestCommandModem(eMdmReqSleepAll);

                        if( m_bReqModemPowerOff == false )
                        {
                            //MONI 2018-04-19 ME block reintialize modem setting
                            // we should test because we assuem that whether every exit setting make error or not
                            //ModemManagerData.eState = eMODEM_ENTER_SLEEP_MODE_INIT;
                            SetModemState(eMODEM_ENTER_SLEEP_MODE);
                        }
                        else
                        {
                            SetModemState(eMODEM_ENTER_POWER_OFF);
                        }
                    }
                    else
                    {
                        //ModemManagerData.eState = eMODEM_READY;
                        //Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqSleep,true);
                        //RequestCommandModem(eMdmReqSleep);
                        //SetRequestActionFromManager(eMdmReqSleep,false);

                        if( m_bReqModemPowerOff == false )
                        {
                            SetModemState(eMODEM_ENTER_SLEEP_MODE);
                        }
                        else
                        {
                            SetModemState(eMODEM_ENTER_POWER_OFF);
                        }
                    }
                }
            }

            if( GetModemState() == eMODEM_NOW_SLEEP_MODE )
            {
                Send2MngSysMsg(eMngModem,eRspSleep,0,(stCarReport*)NULL,0);
                SetMngMdmState(STAT_MNG_MDM_SLEEP);

                // save all status and message to recover after wake up
                WriteBkRamForModem();
            }
            /*
            else
            {
                if( GetModemNotWorkingFlag() == true )
                {
                    // forcely modem power off
                }

                DisableSystemMessageTimer(m_nReqSleepTimeoutTimerID);

                Send2MngSysMsg(eMngModem,eRspSleep,0,(stCarReport*)NULL,0);
                SetMngMdmState(STAT_MNG_MDM_SLEEP);

                // save all status and message to recover after wake up
                WriteBkRamForModem();
            }
            */
            break;
        default:
        break;
    }

    return 0;
}

void SetMngMdmState(int32_t nState)
{
    m_nMngMsgMdmState = nState;
	PushModemStateCollection(MngMdmState, m_nMngMsgMdmState);
}

void PushModemStateCollection(eModemStateSetType Status, char StatusValue)
{
	switch(Status)
	{
		case MngMdmState: 
			g_ModemStateMessage.ucMngMsgMdmState = StatusValue;
			break;
		case ModemOldState: 
			g_ModemStateMessage.ucMdmOldState = StatusValue;
			break;
		case ModemNewState:
			g_ModemStateMessage.ucMdmNewState = StatusValue;
			break;
		case ModemNewEvent: 
			g_ModemStateMessage.ucMdmNewEvent = StatusValue;
			break;
		case ModemManagerState: 
			g_ModemStateMessage.ucModemManagerState = StatusValue;
			break;
		default:
			break;

	}
														
	g_ModemStateMessage.bReqNetworkPostPone             	= m_bReqNetworkPostPone;
    g_ModemStateMessage.bReqSleep                       	= m_bReqSleep;
    g_ModemStateMessage.bPostponeReqSleep              	    = m_bPostponeReqSleep;
    g_ModemStateMessage.bRecoveryMessageFlag                = m_bRecoveryMessage;
    g_ModemStateMessage.bRcvSms							    = m_bRcvSms;
    g_ModemStateMessage.bWaitforSendingData				    = m_bWaitforSendingData;
    g_ModemStateMessage.bSentLastMessage					= m_bSentLastMessage;
#ifndef RF_COMMON_MODEM
    g_ModemStateMessage.bModemRcvSysLoadingMessageFlag	    = ModemManagerData.bModemRcvSysLoadingMessageFlag;
#endif
    g_ModemStateMessage.bModemRcvSysStartMessageFlag		= ModemManagerData.bModemRcvSysStartMessageFlag;
    g_ModemStateMessage.bModemRcvPBReadyMessageFlag		    = ModemManagerData.bModemRcvPBReadyMessageFlag;
    g_ModemStateMessage.bRequestMessageCommFlag			    = ModemManagerData.bRequestMessageCommFlag;
    g_ModemStateMessage.bRequestAgpsCommFlag				= ModemManagerData.bRequestAgpsCommFlag;
    g_ModemStateMessage.bCheckNetworkStatusFlag			    = ModemManagerData.bCheckNetworkStatusFlag;
    g_ModemStateMessage.bNeedStartUpCheckSMSFlag			= ModemManagerData.bNeedStartUpCheckSMSFlag;
    g_ModemStateMessage.bAvailableModemCommFlag			    = ModemManagerData.bAvailableModemCommFlag;	
    g_ModemStateMessage.ucFOTAStartState					= ModemManagerData.eFOTAStartState;
    g_ModemStateMessage.bServiceOpen						= ModemManagerData.bServiceOpen;
    g_ModemStateMessage.bWriteReady 						= ModemManagerData.bWriteReady;
    g_ModemStateMessage.bConfirmWriteLen					= ModemManagerData.bConfirmWriteLen;
    g_ModemStateMessage.bFinishWrite						= ModemManagerData.bFinishWrite;
    g_ModemStateMessage.bReadyReadData					    = ModemManagerData.bReadyReadData;
    g_ModemStateMessage.bEndOfData						    = ModemManagerData.bEndOfData; 
    g_ModemStateMessage.bExpectEndOfData					= ModemManagerData.bExpectEndOfData;
    g_ModemStateMessage.bRcvBinDataZeroFlag				    = ModemManagerData.bRcvBinDataZeroFlag;
    g_ModemStateMessage.ucRecoveryRetryCount				= m_cRecoveryRetryCount;
    g_ModemStateMessage.bIsStoreData						= m_stMsgHdData.bIsStoreData;
    g_ModemStateMessage.ucMdmSubState						= ModemManagerData.nMdmSubState;
    g_ModemStateMessage.ucMdmSubNextState					= ModemManagerData.nMdmSubNextState;
    g_ModemStateMessage.ucOBDState						    = GetOBDState();
    g_ModemStateMessage.bOBDSleepIntoFlag					= g_bOBDSleepIntoFlag;
    g_ModemStateMessage.ucIGOffCnt					     	= m_stMsgHdData.ucIGOffCount;
    g_ModemStateMessage.bSuspendStorage					    = m_bSuspendStorage;
    g_ModemStateMessage.bSendReport						    = m_bSendReport;
	g_ModemStateMessage.bServerConnected					= m_stMsgHdData.bServerConnected;
    strcpy(g_ModemStateMessage.arrSMONI,GetSMONIData());
    g_ModemStateMessage.ucCGREG							    = ModemManagerData.eNetworkRegStatus;
	g_ModemStateMessage.OccurredUtcTime                     = GetUTCTime();
    g_ModemStateMessage.MDResponse                          = g_stModem_Rep.eMDResponse;
    g_ModemStateMessage.cmdIndex                            = g_stModem_Rep.cmd_index;
    g_ModemStateMessage.eCMEErrorCode                       = ModemManagerData.eCMEErrorCode;
    g_ModemStateMessage.HTTPResponseCode                    = ModemManagerData.nHTTPResponseCode;
    g_ModemStateMessage.checksum							= CalcChecksum_Short((unsigned char*)&g_ModemStateMessage,sizeof(MODEM_STATE_COLLECTION)-2);

	PutModemStateBuffer((char*)&g_ModemStateMessage);
}

void SetRcvSmsStatus(boolean_t bStatus)
{
    if( bStatus == true )
        Send2MngSys(eMngModem,eRspWakeupInterruptInfo, eTrue, (stCarReport *)NULL,0);
    else
        Send2MngSys(eMngModem,eRspWakeupInterruptInfo, eFalse, (stCarReport *)NULL,0);

    m_bRcvSms = bStatus;
}

boolean_t GetRcvSmsStatus()
{
    return m_bRcvSms;
}

void Send2MngModemHandler(uint16_t iId, int32_t iEvent,int32_t iSubEvent,boolean_t bForce)
{
    int nCount;
    stMsgMdmHandler stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgMdmHandler));
    stMsg.header.id = iId;
    stMsg.header.event = iEvent;
    stMsg.header.subEvent = iSubEvent;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
    stMsg.header.unTraceMng = iId;

    nCount = MngQueueGetMessageCount(ID_MNG_QUEUE_MDMH);

    if( bForce == true )
    {
#ifndef GLOBAL_SHARE_QUEUE
        MngQueueSendMessage(ID_MNG_QUEUE_MDMH, (int8_t*)&stMsg, sizeof(stMsgMdmHandler));
#else
	    SendSysHdShareQueueMessage(ID_MNG_QUEUE_MDMH,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)NULL, 0);
#endif
    }
    else
    {
        if( nCount < 4 )
        {
#ifndef GLOBAL_SHARE_QUEUE
            MngQueueSendMessage(ID_MNG_QUEUE_MDMH, (int8_t*)&stMsg, sizeof(stMsgMdmHandler));
#else
	    	SendSysHdShareQueueMessage(ID_MNG_QUEUE_MDMH,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)NULL, 0);
#endif
        }
    }
}

void RequestSleepTimeoutTimerCallBack()
{
    // mode state weird // go to sleep force
//#warning "modem status is weird so initialize modem and go to sleep"
    SetModemState(eMODEM_READY);
    Trace("Request sleep time out\r\n");
    // ME is going to sleep forcely. because modem status weired, not expected status.
    // reset the gemalto
    SetModemControlEvent(eMngMdmNone);
    // request workaround to modem
    Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);
    //RequestCommandModem(eMdmReqGemaltoReset);
    //SetRequestActionFromManager(eMdmReqGemaltoReset,true);
    m_bReqDoNothingForSleep = true;
}

void WaitNetworkRegisterTimerCallBack()
{
    // restart modem initialize
    Trace("WaitNetworkRegisterTimerCallBack/ReInitialize modem\r\n");
    //ModemManagerData.eState = eMODEM_CHECK_NETWORK_REGISTRATION;
    //SetRequestActionFromManager(eMdmReqGemaltoReset,true);

    SetModemState(eMODEM_READY);
    Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);

    SetWaitforSendingData(true);
    SetNetworkPostPoneFlag(false);
    //SetLastSendMessageFlag(false);
}

void WaitNetworkAttachedTimerCallBack()
{
    // restart modem initialize
    Trace("WaitNetworkAttachedTimerCallBack/ReInitialize modem\r\n");
    SetModemState(eMODEM_READY);
    Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);
    //SetRequestActionFromManager(eMdmReqGemaltoReset,true);
    //RequestCommandModem(eMdmReqGemaltoReset);

    SetWaitforSendingData(true);
    SetNetworkPostPoneFlag(false);
    //SetLastSendMessageFlag(false);
}

void WaitNetworkDeAttachedTimerCallBack()
{
    // restart modem initialize
    Trace("WaitNetworkDeAttachedTimerCallBack/ReInitialize modem\r\n");
    SetModemState(eMODEM_READY);
    Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);
    //SetRequestActionFromManager(eMdmReqGemaltoReset,true);
    //RequestCommandModem(eMdmReqGemaltoReset);

    SetWaitforSendingData(true);
    SetNetworkPostPoneFlag(false);
    //SetLastSendMessageFlag(false);
}

void ModemReset_Install()
{
    Trace("ModemReset_Install\r\n");
    SetModemState(eMODEM_READY);
    Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);

    SetWaitforSendingData(true);
    SetNetworkPostPoneFlag(false);
}

void WaitNetworkStableTimerCallBack()
{
    // restart modem initialize
    Trace("WaitNetworkStableTimerCallBack/ReInitialize modem\r\n");
    //SetNetworkPostPoneFlag(false);
#if false
    SetModemState(eMODEM_INIT_MODEM);
    //SetModemControlEvent(eMngMdmReConnected);
    SetModemControlEvent(eMngMdmInitializeSoftware);
#endif
    SetModemState(eMODEM_READY);

    Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);
    //SetRequestActionFromManager(eMdmReqGemaltoReset,true);
    //RequestCommandModem(eMdmReqGemaltoReset);

    SetWaitforSendingData(true);
    SetNetworkPostPoneFlag(false);
    //SetLastSendMessageFlag(false);
}

void WaitGeneralErrorTimerCallBack()
{
    // restart modem initialize
    Trace("WaitGeneralErrorTimerCallBack/ReInitialize modem\r\n");
    //SetNetworkPostPoneFlag(false);
    SetModemState(eMODEM_READY);
    Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);
    //SetRequestActionFromManager(eMdmReqGemaltoReset,true);
    //RequestCommandModem(eMdmReqGemaltoReset);

    SetLastSendMessageFlag(false);  // 모뎀 데이터 실패시 Sleep에 진입하지 않는 현상 수정.
    SetWaitforSendingData(true);
    SetNetworkPostPoneFlag(false);
    //SetLastSendMessageFlag(false);
}

void WaitCriticalErrorTimerCallBack()
{
    // restart modem initialize
    Trace("WaitCriticalErrorTimerCallBack/ReInitialize modem\r\n");
    //SetNetworkPostPoneFlag(false);
    SetModemState(eMODEM_READY);
    //SetRequestActionFromManager(eMdmReqGemaltoWorkAround,true);
    Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);
    //Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoWorkAround,true);
    //SetRequestActionFromManager(eMdmReqGemaltoReset,true);
    //RequestCommandModem(eMdmReqGemaltoReset);

    SetWaitforSendingData(true);
    SetNetworkPostPoneFlag(false);
    //SetLastSendMessageFlag(false);
}

void WakeupResponseTimerCallBack()
{
    if( GetRcvSmsStatus() == true )
        Send2MngSys(eMngModem,eRspWakeupInterruptInfo, eTrue, (stCarReport *)NULL,0);
    else
        Send2MngSys(eMngModem,eRspWakeupInterruptInfo, eFalse, (stCarReport *)NULL,0);
}

boolean_t m_bRcvSmsFromServer  = false;

void SetReceivedSms(boolean_t bStatus)
{
    m_bRcvSmsFromServer = bStatus;
}

boolean_t GetReceviedSms()
{
    return m_bRcvSmsFromServer;
}

void SetReceivedSmsInfo(stMsgMdm* pstMsgModem)
{
    // check expire time of sms
    uint32_t unLocalTime = GetLocalTimefromTime(GetUTCTime());

    DisplayTime("Sms Rcv Time",pstMsgModem->carReport.rpSmartKey.Request.OccurredEventTime);
    DisplayTime("local Time",unLocalTime);
#if false
    if( unLocalTime >= pstMsgModem->carReport.rpSmartKey.Request.OccurredEventTime )
    {
        Trace("============================================\r\n");
        Trace("Sms time expired\r\n");
        Trace("============================================\r\n");
        return;
    }
#endif

    // if get vehicle status, then request to obd manager directly.
    // because we reduce report time and data rate.
    // and the device don't need to ask to server
    // because sms include expire time
    if( pstMsgModem->header.subEvent == eR_CurrentVehicleStatus )
    {
        Send2MngSysMsg(eMngModem,eReqReport,eR_CurrentVehicleStatus,&pstMsgModem->carReport,0);
        return;
    }

    SetReceivedSms(true);

    // save last smart key message
    memcpy((char*)&m_stLastRcvSmartKey,(char*)pstMsgModem,sizeof(stMsgMdm));
    //ModemReportHandler(pstMsgModem ->header.subEvent, pstMsgModem);
}

void MngMdmExternalEvent(stMsgMdm* pstMsgModem)
{
    if( pstMsgModem->header.id == eMngSysMsg)
    {
        EvtHdModemfromSystem(pstMsgModem);
    }
    else if( pstMsgModem->header.id == eMngObd )
    {
        EvtHdModemfromObd(pstMsgModem);
    }
    else if( pstMsgModem->header.id == eMngStorage)
    {
        EvtHdModemfromStorage(pstMsgModem);
    }
    else if( pstMsgModem->header.id == eMngModem )
    {
        if( pstMsgModem->header.event == eRcvSms ) //  Received SMS
        {
            // not used this code because it makes late response so ME changed.
//#if true
            // check expire time of sms
            uint32_t unUTCTime = GetUTCTime();

            DisplayTime("Sms Rcv Time",pstMsgModem->carReport.rpSmartKey.Request.OccurredEventUtcTime);
			DisplayTime("UTC Time",unUTCTime);

			if( pstMsgModem->carReport.rpSmartKey.Request.CommandType == 0x48 )
			{
				pstMsgModem->carReport.rpSmartKey.Request.CommandType = 0x40;
			}
			else
			{
	            if( unUTCTime >= pstMsgModem->carReport.rpSmartKey.Request.OccurredEventUtcTime )	//서버에서 1분더한값을 보내줌
	            {
	                if(m_cRecoveryRetryCount == 0)
	                {	
                    Trace("============================================\r\n");
                    Trace("Sms time expired\r\n");
                    Trace("============================================\r\n");

                    SendAlramSMSExpire(pstMsgModem,eMESSAGE_EVENT_KEY_SMSEXPIRE_ALRAM,1);
	                	return;
	            	}
	            }
			}

            // save last smart key message
            memcpy((char*)&m_stLastRcvSmartKey,(char*)pstMsgModem,sizeof(stMsgMdm));

            // save sms rcv time
            m_stLastRcvSmartKey.carReport.rpSmartKey.Request.OccurredEventTime = ModemManagerData.unSmsRcvTime;

#if defined(PROTOCOL12)
            if( pstMsgModem->header.subEvent == eR_ReqModemActivate )
            {
                // save sms rcv time
                m_stLastRcvSmartKey.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.unSmsRcvTime = ModemManagerData.unSmsRcvTime;
            }
#endif
#if defined(PROTOCOL15)
            if( pstMsgModem->header.subEvent == eR_ReqSensorInitialize )
            {
                // save sms rcv time
                m_stLastRcvSmartKey.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.unSmsRcvTime = ModemManagerData.unSmsRcvTime;
            }
#endif
#if defined(PROTOCOL17)
            if( pstMsgModem->header.subEvent == eR_ReqSetURL )
            {
                // save sms rcv time
                m_stLastRcvSmartKey.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.unSmsRcvTime = ModemManagerData.unSmsRcvTime;
            }
			if( pstMsgModem->header.subEvent == eR_ReqSetURLInit )
            {
                // save sms rcv time
                m_stLastRcvSmartKey.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.unSmsRcvTime = ModemManagerData.unSmsRcvTime;
            }
#endif
#if defined(PROTOCOL18)
			if( pstMsgModem->header.subEvent == eR_ReqSettingRsvEngCtrl )
            {
                // save sms rcv time
                m_stLastRcvSmartKey.carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.unRcvSMSTime = ModemManagerData.unSmsRcvTime;
            }
#endif
            // if get vehicle status, then request to obd manager directly.
            // because we reduce report time and data rate.
            // and the device don't need to ask to server
            // because sms include expire time
            if( pstMsgModem->header.subEvent == eR_CurrentVehicleStatus )
            {
                Send2MngSysMsg(eMngModem,eReqReport,eR_CurrentVehicleStatus,&pstMsgModem->carReport,0);
                return;
            }

            ModemReportHandler(pstMsgModem->header.subEvent, pstMsgModem);
//#endif
        }
        else if( pstMsgModem->header.event == eSetApn )
        {
            // set new apn
            SetUserApn((char *)pstMsgModem->carReport.rpSetting.ModemSetting.carrApn);
        }
#if defined(PROTOCOL17)	//개발중;;
		else if( pstMsgModem->header.event == eReqReport )
        {
            SetUserApn((char *)pstMsgModem->carReport.rpSetting.ModemSetting.carrApn);
        }
#endif
        else if( pstMsgModem->header.event == eDummy )
        {
            // request wait for sleep because ME received sms
            Send2MngSysMsg(eMngModem,eDummy,0,(stCarReport *) NULL,0);
        }
    }
}

void EvtHdModemfromObd(stMsgMdm* pstMsgModem)
{
    Trace("Get event3 : %x, subEvent : %x\r\n",pstMsgModem->header.event, pstMsgModem->header.subEvent);

    if( pstMsgModem->header.event== eReqSleep )
    {
        // if other manager wants to go sleep, this mananger request upper manager
        // and wait for event from upper manager.
        SetMngMdmState(STAT_MNG_MDM_SLEEP_WAIT);

        Trace("Get Sleep Event from other Manager\r\n");
    }
    else if( pstMsgModem->header.event == eReqReport )
    {
        // 예약 시동은 시동 off 시 또는 한시간 주기 보고시 sms을 수신하지 않고 서버로 데이터를 요청하여 예약 정보 싱크를 맞춘다.
        // 이때 96전문 SMS받은 시간을 0으로 보내지 않기 위해 SMS받은 시간을 전문 발생시간으로 Setting
    	if( pstMsgModem->header.subEvent == eR_ReqSettingRsvEngCtrl )
		{
			// save sms rcv time
			memcpy((char*)&m_stLastRcvSmartKey,(char*)pstMsgModem,sizeof(stMsgMdm));
			m_stLastRcvSmartKey.carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.unRcvSMSTime = GetLocalTimefromTime(GetUTCTime());
		}
        GIT_Assert((pstMsgModem->header.subEvent > eR_None) && (pstMsgModem->header.subEvent < eR_Max),eErrorCodeMdm|eSubEventUnknown);
        ModemReportHandler(pstMsgModem->header.subEvent, pstMsgModem);
    }
    else if( pstMsgModem->header.event == eRspReport )
    {
        GIT_Assert((pstMsgModem->header.subEvent > eR_None) && (pstMsgModem->header.subEvent < eR_Max),eErrorCodeMdm|eSubEventUnknown);
        ModemReportHandler(pstMsgModem->header.subEvent, pstMsgModem);
    }
    else
    {
        GIT_Assert(false,eErrorCodeMdm|eEventUnknown);
    }
}

//#define USE_FILE_BROKEN_TEST
#ifdef USE_FILE_BROKEN_TEST
boolean_t m_bMessageBrokenTest = false;
#endif //#ifdef USE_FILE_BROKEN_TEST

void EvtHdModemfromStorage(stMsgMdm* pstMsgModem)
{
    Trace("Get event2 : %x, subEvent : %x\r\n",pstMsgModem->header.event,pstMsgModem->header.subEvent);

#ifdef USE_FILE_BROKEN_TEST
    if( m_bMessageBrokenTest == true )
    {
        pstMsgModem->header.subEvent = eR_None;
    }
#endif

    if( pstMsgModem->header.event == eReqSleep )
    {
        // if other manager wants to go sleep, this mananger request upper manager
        // and wait for event from upper manager.
        SetMngMdmState(STAT_MNG_MDM_SLEEP_WAIT);

        Trace("Get Sleep Event from other Manager\r\n");
    }
    else if( pstMsgModem->header.event == eReqReport )
    {
        if( pstMsgModem->header.subEvent > eR_None &&
            pstMsgModem->header.subEvent < eR_Max)
        {
            ModemReportHandler(pstMsgModem->header.subEvent, pstMsgModem);

            //DisplayReport("mngModem]Rcv Data\n",(stMsgSysMsg*)pstMsgModem);
        }
        else
        {
            Trace("================================================\r\n");
            Trace("File Broken\r\n");
            //GIT_Assert(false,eErrorCodeMdm|eSubEventUnknown);
            pstMsgModem->header.id = eMngModem;
            pstMsgModem->header.event = eReqDeleteFile;
            pstMsgModem->header.result = eTrue;
            Send2MngStorage2((stMsgStorage*)pstMsgModem);
        }
    }
    else
    {
        Trace("================================================\r\n");
        Trace("File Broken\r\n");
        //GIT_Assert(false,eErrorCodeMdm|eEventUnknown);
        pstMsgModem->header.id = eMngModem;
        pstMsgModem->header.event = eReqDeleteFile;
        pstMsgModem->header.result = eTrue;
        Send2MngStorage2((stMsgStorage*)pstMsgModem);
    }
}

void EvtHdModemfromSystem(stMsgMdm* pstMsgModem)
{
    Trace("Get event1 : %x, subEvent : %x\r\n",pstMsgModem->header.event,pstMsgModem->header.subEvent);

    if( pstMsgModem->header.event == eReqSleep )
    {
        // if other manager wants to go sleep, this mananger request upper manager
        // and wait for event from upper manager.
        SetMngMdmState(STAT_MNG_MDM_SLEEP_WAIT);

        Trace("Get Sleep Event from other Manager\r\n");
    }
    else if( pstMsgModem->header.event == eReqReport )
    {
        if( pstMsgModem->header.subEvent > eR_None &&
            pstMsgModem->header.subEvent < eR_Max)
        {
            ModemReportHandler(pstMsgModem->header.subEvent, pstMsgModem);

            //DisplayReport("mngModem]Rcv Data\n",(stMsgSysMsg*)pstMsgModem);
        }
        else
        {
            GIT_Assert(false,eErrorCodeMdm|eSubEventUnknown);
        }
    }
    else if( pstMsgModem->header.event == eRspReport )
    {
        if( pstMsgModem->header.subEvent > eR_None &&
            pstMsgModem->header.subEvent < eR_Max)
        {
            ModemReportHandler(pstMsgModem->header.subEvent, pstMsgModem);
        }
        else
        {
            GIT_Assert(false,eErrorCodeMdm|eSubEventUnknown);
        }
    }
#ifdef ENABLE_FOTA
    else if( pstMsgModem->header.event == eReqFota )
    {
        if( pstMsgModem->header.subEvent == eFwVehicleInfo )
        {
            ModemFotaHandler(pstMsgModem->header.subEvent, pstMsgModem);
        }
        else if( pstMsgModem->header.subEvent == eFwInfo )
        {
            ModemFotaHandler(pstMsgModem->header.subEvent, pstMsgModem);
        }
        else if( pstMsgModem->header.subEvent == eFwBin )
        {
            ModemFotaHandler(pstMsgModem->header.subEvent, pstMsgModem);
        }
    }
#endif
    else if( pstMsgModem->header.event == eReqIpek )
    {
        if( pstMsgModem->header.subEvent == eIpekPhase1 )
        {
            ModemIpekHandler(pstMsgModem->header.subEvent, pstMsgModem);
        }
        else if( pstMsgModem->header.subEvent == eIpekPhase2 )
        {
            ModemIpekHandler(pstMsgModem->header.subEvent, pstMsgModem);
        }
        else
        {
            Trace("ipek not defined message\r\n");
        }
    }
    else if( pstMsgModem->header.event == eReqModemPowerOff )
    {
        if( pstMsgModem->header.result == eTrue )
        {
            //MONI It will not active because ME is going to setup shut down then ME will be reset.
            m_bReqModemPowerOff  = true;
        }
    }
    else if( pstMsgModem->header.event == eReqAgps )
    {
        if( pstMsgModem->header.subEvent == eAgpsDownload )
        {
            ModemAgpsHandler(pstMsgModem->header.subEvent, pstMsgModem);
        }
    }
    else
    {
        GIT_Assert(false,eErrorCodeMdm|eEventUnknown);
    }
}

boolean_t m_bRcvNewApn = false;
char m_strApn[MAX_APN_SIZE]={0};

void SetUserApn(char* pstrApn)
{
    memset((char*)m_strApn,0,strlen(m_strApn));
    memcpy((char*)m_strApn,pstrApn,strlen(pstrApn));
    m_bRcvNewApn = true;
    // request workaround to modem
    //SetRequestActionFromManager(eMdmReqGemaltoReset,true);
    SetModemState(eMODEM_READY);
    Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);
    //RequestCommandModem(eMdmReqGemaltoReset);
}

boolean_t GetUserApn(char* pstrApn)
{
    if( m_bRcvNewApn == true )
    {
        memcpy(pstrApn,(char*)m_strApn,strlen(m_strApn));
        return true;
    }

    return false;
}


boolean_t IsModemConnected()
{
    if( GetModemControlOldState() == eModemControlStateConnected ||
        GetModemControlOldState() == eModemControlStateReConnected )
        return true;


    return false;
}



int stModemEventIndex[]=
{
    0,
    0x01,
    0x02,
    0x04,
    0x08,
    0x10, // 5
    0x20,
    0x40,
    0x80,
    0x100,
    0x200, // 10
    0x400,
    0x800,
    0x1000,
    0x2000,
    0x4000,
    0x8000,
    0x10000,
    0x20000,
    0x40000,
    0x80000,
};

int GetModemEventStrIndex(uint32_t nStatus)
{
    for(int i=0;i<sizeof(stModemEventIndex)/4;i++)
    {
        if( stModemEventIndex[i] == nStatus )
        {
            return i;
        }
    }

    return -1;
}


int8_t* pcarrStrModemState[]=
{
    "eMngMdmNone",
    "eMngMdmInitializeHardware",
    "eMngMdmInitializeSoftware",
    "eMngMdmRegistedNetwork",
    "eMngMdmConnected",
    "eMngMdmReConnected",
    "eMngMdmSleep",
    "eMngMdmDetachedBaseStation",
    "eMngMdmAttachedBaseStation",
    "eMngMdmGeneralError",
    "eMngMdmCriticalError",
    "eMngMdmNotRegistered",
    "eMngMdmDoNothing",
    "eMngMdmWaitForModemStable",
    "eMngMdmWaitForModemStableShortTime",
    "eMngMdmRequestSendResult_Success",
    "eMngMdmRequestSendResult_Fail",
    "eMngMdmRequestSendResult_Fail_NotFoundServer",
    "eMngMdmRequestSendResult_Fail_NotStableServer",
    "eMngMdmRequestSendResult_Fail_SockError",
    "eMngMdmRequestSendResult_Fail_General",
};

boolean_t GetNetworkPostPondeFlag()
{
    return m_bReqNetworkPostPone;
}

void SetNetworkPostPoneFlag(boolean_t bPostPone)
{
    m_bReqNetworkPostPone = bPostPone;
}

//#define ENABLE_RETRY_TEST

// test code for retry
#ifdef ENABLE_RETRY_TEST
int m_nRetryTest = 0;
#endif // #ifdef ENABLE_RETRY_TEST

#define MAX_RETRY_COUNT 3

void SendResultsLastMessage(int32_t nValue, stMsgMdm* pstMessage)
{
    stMsgMdm stMessage;

    if( GetLastSendMessage(&stMessage) ==  true )
    {
        // set last message flag to clear.
        // SetLastSendMessageFlag(false);

        memcpy((char*)pstMessage,(char*)&stMessage,sizeof(stMsgMdm));

        if( stMessage.header.id == eMngSysMsg )
        {
            stMessage.header.id = eMngModem;
            stMessage.header.result = nValue;

            if(stMessage.header.event == eReqReport )
            {
                stMessage.header.event = eRspReport;
            }
            else if(stMessage.header.event == eReqFota )
            {
                stMessage.header.event = eRspFota;
            }
            else if(stMessage.header.event == eReqAgps )
            {
                stMessage.header.event = eRspAgps;
            }

            Send2MngSysMsg3((stMsgSysMsg*)&stMessage);
        }
        else if( stMessage.header.id == eMngStorage )
        {
            stMessage.header.id = eMngModem;
            stMessage.header.event = eRspReport;
            stMessage.header.result = nValue;
            Send2MngStorage2((stMsgStorage*)&stMessage);
        }
    }
    else
    {
        Trace("Error not match the last message\r\n");
        Trace("Error not match the last message\r\n");
        Trace("Error not match the last message\r\n");
    }
}

void SetLastSendMessage(stMsgMdm* pstMsgMdm)
{
    m_bSentLastMessage = true;
    memcpy((int8_t*)&m_stLastMessageModem,(int8_t*)pstMsgMdm,sizeof(stMsgMdm));

    SetModemLedStatus(eModemLedStatus_Transfer);
}

boolean_t GetLastSendMessageFlag()
{
    return m_bSentLastMessage;
}

void SetLastSendMessageFlag(boolean_t bLastSendMessageFlag)
{
    m_bSentLastMessage = bLastSendMessageFlag;
}

boolean_t GetLastSendMessage(stMsgMdm* pstMsgMdm)
{
    if( m_bSentLastMessage == true )
    {
        memcpy((int8_t*)pstMsgMdm,(int8_t*)&m_stLastMessageModem,sizeof(stMsgMdm));
        //DisplayReport("Result Message",(stMsgSysMsg*)pstMsgMdm);

        return true;
    }

    return false;
}

// this function will return report count included in that message.
int GetLastSendMessageCount()
{
    int nCount = 0;

    if( m_bSentLastMessage == true )
    {
        switch( m_stLastMessageModem.header.subEvent )
        {
            case eR_Alram:
            case eR_BeforeDriving:
            case eR_AfterDriving:
            case eR_ParkingInterval:
#if defined(PROTOCOL18)
			case eR_Charging:
#endif
            case eR_AlramMasking:
            case eR_AlramDTC:
            case eR_ImpulseAlram:
            case eR_ReqSmartKey:
            case eR_RspSmartKey:
            case eR_CurrentVehicleStatus:
            case eR_SettingGeofence:
            case eR_SettingPolygonGeofence:
            case eR_ReqUserActionSetting:
            case eR_RspUserActionSetting:
            case eR_ReqSettingRsvEngCtrl:
#if defined(PROTOCOL24)
			case eR_ReportNetworkStatus:
#endif
#if defined(PROTOCOL25)
            case eR_ReportInstallationNetworkCheck:
            //case eR_ReportInstallationSMSCheck:    
#endif
                nCount = 1;
                break;
            case eR_DrivingInterval:
                if( m_stLastMessageModem.carReport.rpInterval.DrivingInfo.DrivingInfoB1.OccurredEventTime != 0 )
                {
                    nCount++;
                }
                if( m_stLastMessageModem.carReport.rpInterval.DrivingInfo.DrivingInfoB2.OccurredEventTime != 0 )
                {
                    nCount++;
                }
                break;
            default:
                nCount=1;
                break;
        }

        return nCount;
    }

    return nCount;
}

void IncreaseModemResetRetryCount()
{
    m_cModemResetRetryCount++;
}

void ClearModemResetRetryCount()
{
    m_cModemResetRetryCount=0;
}

int8_t GetModemResetRetryCount()
{
    return m_cModemResetRetryCount;
}

void GetLastSmartMessageInfomation(stMsgMdm* pstModemMessage)
{
    memcpy((char*)pstModemMessage,(char*)&m_stLastRcvSmartKey,sizeof(stMsgMdm));
}

void SetModemNotWorkingFlag(boolean_t bModemNotWorkingFlag)
{
    m_bModemNotWorkingFlag = bModemNotWorkingFlag;
}

boolean_t GetModemNotWorkingFlag()
{
    return m_bModemNotWorkingFlag;
}

void Retransmision(stMsgMdm stMessage)
{
    //if( m_cRecoveryRetryCount++ < MAX_RETRY_COUNT )
    {
        // recovery
        m_bRecoveryMessage = true;
        memcpy(&m_stRecoveryMessage,&stMessage,sizeof(stMsgMdm));
    }
}

void ClearRetransmisionCount()
{
    m_cRecoveryRetryCount=0;
    m_bRecoveryMessage=false;
	memset(&m_stRecoveryMessage,0x00,sizeof(stMsgMdm));
}

uint8_t GetReTransmisionCount()
{
    return m_cRecoveryRetryCount;
}

void IncreaseRetransmisionCount()
{
    m_cRecoveryRetryCount++;
}

boolean_t m_bWeakRssiCount = false;
boolean_t m_bStrongRssiCount = false;

int32_t m_nDisconnectNetworkStatusCount = 0;
int32_t m_nRssiCount = 0;
int32_t m_nWeakRssiCount = 0;
int32_t m_nStrongRssiCount = 0;
boolean_t m_bDonothingRssi = false;

#define MAX_RSSI_LENGTH 60
int m_nAvgRssi[MAX_RSSI_LENGTH]={0,};

int MakeAverageRssi(int* pnAvrRssi,int nRssi)
{
    int nSumRssi=0;

    for(int i=(MAX_RSSI_LENGTH-1);i>0;i--)
    {
        pnAvrRssi[i]=pnAvrRssi[i-1];
    }

    pnAvrRssi[0]=nRssi;

    //hexdump(pnAvrRssi, MAX_RSSI_LENGTH*sizeof(int));

    for(int j=0;j<MAX_RSSI_LENGTH;j++)
        nSumRssi+=pnAvrRssi[j];

    return (nSumRssi/MAX_RSSI_LENGTH);
}

void ClearNetworkDisconnectCount()
{
    m_nDisconnectNetworkStatusCount = 0;
}

void SetModemNetworkStatus(eNETWORK_REG_STATUS eNetworkStatus,int nRssi)
{
    boolean_t bIdle = false;

    switch(eNetworkStatus)
    {
    	case eNETWORK_REG_STATUS_NOT_REG:
    	case eNETWORK_REG_STATUS_SEARCHING:
    	case eNETWORK_REG_STATUS_REG_DENIED:
    	case eNETWORK_REG_STATUS_UNKNOWN:
            m_nDisconnectNetworkStatusCount++;
            break;
    	case eNETWORK_REG_STATUS_REGISTERED:
    	case eNETWORK_REG_STATUS_REG_ROAMING:
            m_nDisconnectNetworkStatusCount=0;
            break;
        case eNETWORK_REG_STATUS_IDLE:
            m_nDisconnectNetworkStatusCount=0;
            bIdle = true;
            break;
    }

    // 30 means 30 seconds
    if( m_nDisconnectNetworkStatusCount > 360 )
    {
        // netwokr is not stable
        // action1 : forcely disconnect
        // action2 : set timer,5 mins, for reconnect to the base station
        // SetRequestActionFromManager(eMdmReqDonothing,true);
        // send 2 modem rssi signal is not statble event.

        //if( m_bDonothingRssi == false )
        {
            //Trace("Network detached\r\n");
            //SetRequestActionFromManager(eMdmReqDonothing,true);
            SetModemControlEvent(eMngMdmNotRegistered);
            m_nDisconnectNetworkStatusCount = 0;
        }
    }

    if( nRssi == 0 || bIdle == true )
        return;

    // not used below code to check network attahced or detached ME use AT+CGATT
    return;

#if false
 	int nSumRssi;
    nSumRssi = MakeAverageRssi(m_nAvgRssi,nRssi);
    m_nRssiCount++;

    //Trace("dBm : %d, Avg dBm : %d\r\n",nRssi,nSumRssi);

    if( m_nRssiCount > MAX_RSSI_LENGTH )
    {
        if( nSumRssi <= -110)
        {
            if( m_nWeakRssiCount++>30 && m_bWeakRssiCount == false )
            {

                Trace("===========================================\r\n");
                Trace("Do nothing Avg Rssi(eMngMdmDetachedBaseStation) : %d\r\n",nSumRssi);
                //SetRequestActionFromManager(eMdmReqDonothing,true);
                // halt all procedure of modem because modem disconnected with base station
                //HandlerRequestSendResult(eRequestSendResult_Fail_NotStableServer);

                SetModemControlEvent(eMngMdmDetachedBaseStation);

                m_nWeakRssiCount = 0;
                m_bWeakRssiCount = true;
            }

            m_nStrongRssiCount = 0;
            m_nStrongRssiCount = false;
        }
        else
        {
            //if( nSumRssi > -105 )
            {
                m_nDisconnectNetworkStatusCount=0;

                if( IsModemConnected() == true )
                {
                    // do nothing because states is connected
                    m_nStrongRssiCount = 0;
                    m_nStrongRssiCount = false;
                    return;
                }

                if( m_nStrongRssiCount++>30 && m_nStrongRssiCount == false )
                {
                    Trace("===========================================\r\n");
                    Trace("Do something Avg Rssi(eMngMdmAttachedBaseStation) : %d\r\n",nSumRssi);
                    //SetRequestActionFromManager(eMdmReqReInit,true);
                    SetModemControlEvent(eMngMdmAttachedBaseStation);

                    m_nStrongRssiCount = 0;
                    m_nStrongRssiCount = true;
                }

                m_nWeakRssiCount = 0;
                m_bWeakRssiCount = false;
            }
        }
    }
#endif
}

int m_nCriticalErrorCount = 0;

void HandlerRequestSendResult(uint32_t unResult)
{
    stMsgMdm stMessage;

    switch(unResult)
    {
        case eMngMdmRequestSendResult_Success:
            Trace("_____________________________________________________________\r\n");
            Trace("|                     Transfer Successful                    \r\n");
            Trace("-------------------------------------------------------------\r\n");
            ClearRetransmisionCount();

            SendResultsLastMessage(eSuccess,&stMessage);
            SetLastSendMessageFlag(false);

            SetWaitforSendingData(false);

            ClearNetworkDisconnectCount();

            m_nCriticalErrorCount=0;
            break;
        case eMngMdmRequestSendResult_Fail:
            // this error case
            // 1. no response
            // 2. server not response
            // 3. ack response with error
            // action : modem reset
            Trace("_____________________________________________________________\r\n");
            Trace("| / / / / / / / /  Fail to Transfer  1/ / / / / / / / / / / / \r\n");
            Trace("-------------------------------------------------------------\r\n");
            Trace("reason : eRequestSendResult_Fail\r\n");
            if( stMessage.header.event != eReqFota && stMessage.header.event != eRspFota )
            {
            	SendResultsLastMessage(eSuccess,&stMessage);
            }
			else	// 실패시 재전송 로직에 의해서 재전송 되므로 저장 데이터는 삭제
			{
				SendResultsLastMessage(eFail,&stMessage);
			}
            SetModemControlEvent(eMngMdmGeneralError);

            if( stMessage.header.event != eReqFota && stMessage.header.event != eRspFota )
            {
                Retransmision(stMessage);
            }
            else
            {
                // exception case
                Send2MngObd(eMngModem,eOBDSetting, eInitailize,(stCarReport *)NULL,0);
            }

            SetWaitforSendingData(true);
            break;
        case eMngMdmRequestSendResult_Fail_NotFoundServer:
        case eMngMdmRequestSendResult_Fail_NotStableServer:
        case eMngMdmRequestSendResult_Fail_SockError:
        case eMngMdmRequestSendResult_Fail_General:
        default:
            Trace("_____________________________________________________________\n");
            Trace("| / / / / / / / /  Fail to Transfer  %d / / / / / / / / / / / \n",unResult);
            Trace("-------------------------------------------------------------\n");
            if( stMessage.header.event != eReqFota && stMessage.header.event != eRspFota )
            {
            	SendResultsLastMessage(eSuccess,&stMessage);
            }
			else	// 실패시 재전송 로직에 의해서 재전송 되므로 저장 데이터는 삭제
			{
				SendResultsLastMessage(eFail,&stMessage);
			}
            SetModemControlEvent(eMngMdmWaitForModemStableShortTime);

            // prevent retransmit for fota
            if( stMessage.header.event != eReqFota && stMessage.header.event != eRspFota )
            {
                Retransmision(stMessage);
            }
            else
            {
                // exception case
                Send2MngObd(eMngModem,eOBDSetting, eInitailize,(stCarReport *)NULL,0);
            }

            SetWaitforSendingData(true);

            IncreaseRetransmisionCount();
            if( GetReTransmisionCount() >= MAX_RETRY_COUNT )
            {
                ClearRetransmisionCount();
				SetLastSendMessageFlag(false);
                SetModemControlEvent(eMngMdmGeneralError);
            }
            break;
    }
}


//#define MAX_NOT_REGISTERED_COUNT 10	//10 너무 짧다. 10초만에 stop되고 reset이 되어버리기 때문에 망 등록까지 시간이 부족하다
#define MAX_NOT_REGISTERED_COUNT 40
int m_nNotRegisteredConituesCount = 0;

void ActionForNewEvent(int32_t nStatus)
{
    uint32_t unDelay;

    switch(nStatus)
    {
        case eMngMdmNone:
        case eMngMdmInitializeHardware:
        case eMngMdmInitializeSoftware:
        case eMngMdmRegistedNetwork:
        case eMngMdmConnected:
        case eMngMdmSleep:
        case eMngMdmRequestSendResult_Success:
        case eMngMdmRequestSendResult_Fail:
        case eMngMdmRequestSendResult_Fail_NotFoundServer:
        case eMngMdmRequestSendResult_Fail_NotStableServer:
        case eMngMdmRequestSendResult_Fail_SockError:
        case eMngMdmRequestSendResult_Fail_General:
            break;
        case eMngMdmGeneralError:
            if( GetRemoteControlStatus() == true )
                unDelay = 1000;
            else
                unDelay = TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE;

            Trace("Error Occurred wait %d seconds\r\n",TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE/1000);
            EnableSystemMessageTimer(&m_nReqWaitNetworkStableTimeoutTimerID,
            unDelay,eSWTimer_ONESHOT,
            WaitGeneralErrorTimerCallBack);
            SetWaitforSendingData(true);
            break;
        case eMngMdmCriticalError:
            if( GetRemoteControlStatus() == true )
                unDelay = 1000;
            else
                unDelay = TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE;

            Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerDisconnected, (stCarReport *)NULL,0);
            Trace("Error Occurred wait %d seconds\r\n",TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE/1000);
#if true
            Trace("Modem has critical error\r\n");
            WaitCriticalErrorTimerCallBack();
#else
            EnableSystemMessageTimer(&m_nReqWaitNetworkStableTimeoutTimerID,
            unDelay,eSWTimer_ONESHOT,
            WaitCriticalErrorTimerCallBack);
#endif
            SetWaitforSendingData(true);

            Trace("=============================================================\r\n");
            Trace("Crirtical Error Count : %d\r\n",m_nCriticalErrorCount);
            Trace("=============================================================\r\n");
            // this is exception process
            if(m_nCriticalErrorCount++ > 10 )
            {
                Trace("=============================================================\r\n");
                Trace("=============================================================\r\n");
                Trace("=============================================================\r\n");
                Trace("Crirtical Error Count : %d\r\n",m_nCriticalErrorCount);
                Trace("=============================================================\r\n");
				printf("PowerOnGemaltoModem_Modem_Reset_Power\r\n");
#ifdef RF_COMMON_MODEM  //mod.kks 21.11.02
                HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
                APP_Delay(200);
                HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
                APP_Delay(200);
                HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
#else
				HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
                for(int i=0;i<2;i++)
                {
                    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
                    MD_mDelay(200);

            		HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
                    MD_mDelay(200);

                    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
                    MD_mDelay(50);
                }
                HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#endif
                m_nCriticalErrorCount = 0;
            }
            break;
        case eMngMdmReConnected:
            m_nNotRegisteredConituesCount=0;
            break;
        case eMngMdmNotRegistered:
            if( GetRemoteControlStatus() == true )
			{
                unDelay = 1000;
				Trace("Modem Not Registered wait %d ms %d cnt\n",unDelay, m_nNotRegisteredConituesCount);

				if( m_nNotRegisteredConituesCount++ < MAX_NOT_REGISTERED_COUNT )
				{
					EnableSystemMessageTimer(&m_nReqWaitNetworkStableTimeoutTimerID,
					unDelay,eSWTimer_ONESHOT,
					WaitNetworkRegisterTimerCallBack);
				}
				else
				{
					WaitNetworkRegisterTimerCallBack();
					m_nNotRegisteredConituesCount=0;
				}
				SetWaitforSendingData(true);
			}
            else
			{
                unDelay = TIMER_REQ_WAIT_NETWORK_REGISTER_VALUE;

            // set timer for 5minutes
            // and then try to intialize

				Trace("Modem Not Registered wait %d ms %d cnt\n",unDelay, m_nNotRegisteredConituesCount);

				if( m_nNotRegisteredConituesCount++ < 1 )
            {
                EnableSystemMessageTimer(&m_nReqWaitNetworkStableTimeoutTimerID,
					10000,eSWTimer_ONESHOT,
                WaitNetworkRegisterTimerCallBack);
            }
            else
            {
                WaitNetworkRegisterTimerCallBack();
                m_nNotRegisteredConituesCount=0;
            }
            SetWaitforSendingData(true);
			}
            break;
        case eMngMdmWaitForModemStable:
            if( GetRemoteControlStatus() == true )
                unDelay = 1000;
            else
                unDelay = TIMER_REQ_WAIT_NETWORK_REGISTER_VALUE;

            Trace("Modem Not Stable wait %d seconds\r\n",TIMER_REQ_WAIT_NETWORK_REGISTER_VALUE/1000);
            EnableSystemMessageTimer(&m_nReqWaitNetworkStableTimeoutTimerID,
            unDelay,eSWTimer_ONESHOT,
            WaitNetworkStableTimerCallBack);
            SetWaitforSendingData(true);
            break;
        case eMngMdmWaitForModemStableShortTime:
            if( GetRemoteControlStatus() == true )
                unDelay = 1000;
            else
                unDelay = TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE;

            Trace("Modem Not Stable wait %d seconds\r\n",TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE/1000);
            EnableSystemMessageTimer(&m_nReqWaitNetworkStableTimeoutTimerID,
            unDelay,eSWTimer_ONESHOT,
            WaitNetworkStableTimerCallBack);
            SetWaitforSendingData(true);
            break;
        case eMngMdmDetachedBaseStation:
            if( GetRemoteControlStatus() == true )
                unDelay = 1000;
            else
                unDelay = TIMER_REQ_WAIT_NETWORK_REGISTER_VALUE;

            // set timer for 5minutes
            // and then try to intialize
#if true
            Trace("Modem is detached and not correct state\r\n");
            WaitNetworkDeAttachedTimerCallBack();
#else
            Trace("Modem is detached wait %d seconds\r\n",TIMER_REQ_WAIT_NETWORK_REGISTER_VALUE/1000);

            EnableSystemMessageTimer(&m_nReqWaitNetworkStableTimeoutTimerID,
            unDelay,eSWTimer_ONESHOT,
            WaitNetworkDeAttachedTimerCallBack);
#endif
            SetWaitforSendingData(true);
            break;
        case eMngMdmAttachedBaseStation:
            if( GetRemoteControlStatus() == true )
                unDelay = 1000;
            else
                unDelay = TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE;

            // set timer for 5minutes
            // and then try to intialize
            Trace("Modem is attached wait %d seconds\r\n",TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE/1000);
            EnableSystemMessageTimer(&m_nReqWaitNetworkStableTimeoutTimerID,
            unDelay,eSWTimer_ONESHOT,
            WaitNetworkAttachedTimerCallBack);
            SetWaitforSendingData(true);
            break;
        default:
            Trace("received multiple error event at a one time\r\n");
            // this is multi error case it means that there are an event over 2EA at a short time
            // in this case ME start initialize
            if( GetRemoteControlStatus() == true )
                unDelay = 1000;
            else
                unDelay = TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE;

            // set timer for 5minutes
            // and then try to intialize
            Trace("Modem is weired wait %d seconds\r\n",TIMER_REQ_WAIT_NETWORK_STABLE_TIMEOUT_VALUE/1000);
            EnableSystemMessageTimer(&m_nReqWaitNetworkStableTimeoutTimerID,
            unDelay,eSWTimer_ONESHOT,
            WaitCriticalErrorTimerCallBack);
            SetWaitforSendingData(true);
            break;
    }

    Trace("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
	if(GetModemEventStrIndex(nStatus)!=-1)	Trace("Modem event : %s\r\n",pcarrStrModemState[GetModemEventStrIndex(nStatus)]);
	else									Trace("Modem event : NONE\r\n");
    Trace("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
}


uint32_t m_unModemControlState = eModemControlStateIdle;
uint32_t m_unModemControlOldState = eModemControlStateIdle;

uint32_t m_unModemControlOldEvent;
uint32_t m_unModemControlEvent;


// related with old state
uint32_t GetModemControlOldState()
{
    return m_unModemControlOldState;
}

void SetModemControlOldState(uint32_t unState)
{
    m_unModemControlOldState = unState;
}

// this function wil return new state of modem
uint32_t DecideNewModemState(uint32_t unModemNewEvent,uint32_t unModemOldEvent, uint32_t unModemOldState)
{
    uint32_t unDifEvent = unModemNewEvent^unModemOldEvent;
    uint32_t newState = unModemOldState;

    switch(unModemOldState)
    {
        case eModemControlStateIdle:
            if( unDifEvent & eMngMdmInitializeHardware ||
                unDifEvent & eMngMdmInitializeSoftware )
            {
                // wait for software initialize
                newState = eModemControlStateInit;
            }
            else if( unDifEvent == eMngMdmAttachedBaseStation ||
                unDifEvent == eMngMdmDetachedBaseStation)
            {
                Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerDisconnected, (stCarReport *)NULL,0);
            }
            else if( unDifEvent == eMngMdmConnected )
            {
                // MONI 2018 this is exception case
                SetModemState(eMODEM_READY);
                Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);
            }
            else if( unDifEvent == eMngMdmNone)
            {
                newState = eModemControlStateSleep;
            }
            else
            {
                // do nothing
                printf("eModemControlStateIdle Do nothing\r\n");
            }
            break;
        case eModemControlStateInit:
            if( unDifEvent & eMngMdmInitializeSoftware )
            {
                printf("rcv : eMngMdmInitializeSoftwarer\r\n");
            }
            else if( unDifEvent == eMngMdmAttachedBaseStation ||
                unDifEvent == eMngMdmDetachedBaseStation)
            {
                Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerDisconnected, (stCarReport *)NULL,0);
            }
            else if( unDifEvent == eMngMdmConnected )
            {
                // MONI 2018 this is exception case
                SetModemState(eMODEM_READY);
                Send2MngModemHandler(eMngModem,eReqCommand,eMdmReqGemaltoReset,true);
            }
            else if( unDifEvent == eMngMdmNone)
            {
                newState = eModemControlStateSleep;
            }
            else if( unDifEvent & eMngMdmRegistedNetwork )
            {
                // change for register to base station
                newState = eModemControlStateConnecting;
            }
            else if( unDifEvent & eMngMdmReConnected )
            {
                // for software booting ME needs this code
                newState = eModemControlStateReConnected;

                //Stop Timer
                DisableSystemMessageTimer(m_nReqWaitNetworkStableTimeoutTimerID);
                // clear request action to modem manager
                ClearRequestActionFromManagerFlag();
            }
            else if( unDifEvent == eMngMdmGeneralError ||
                unDifEvent == eMngMdmCriticalError ||
                unDifEvent == eMngMdmNotRegistered ||
                unDifEvent == eMngMdmDoNothing ||
                unDifEvent == eMngMdmWaitForModemStable ||
                unDifEvent == eMngMdmWaitForModemStableShortTime ||
                unDifEvent == eMngMdmRequestSendResult_Fail ||
                unDifEvent == eMngMdmRequestSendResult_Fail_NotFoundServer ||
                unDifEvent == eMngMdmRequestSendResult_Fail_NotStableServer ||
                unDifEvent == eMngMdmRequestSendResult_Fail_SockError ||
                unDifEvent == eMngMdmRequestSendResult_Fail_General )
            {
                // if error occurred, then is going to disconnection
                newState = eModemControlStateDisconnected;
            }
            else if( unDifEvent == eMngMdmNone)
            {
                newState = eModemControlStateSleep;
            }
            else
            {
                printf("eModemControlStateInit Do nothing\r\n");
            }
            break;
        case eModemControlStateConnecting:
            if( unDifEvent & eMngMdmConnected ||
                unDifEvent & eMngMdmReConnected )
            {
                // wait for connection
                newState = eModemControlStateConnected;

                //Stop Timer
                DisableSystemMessageTimer(m_nReqWaitNetworkStableTimeoutTimerID);
                // clear request action to modem manager
                ClearRequestActionFromManagerFlag();
            }
            else if( unDifEvent == eMngMdmGeneralError ||
                unDifEvent == eMngMdmCriticalError ||
                unDifEvent == eMngMdmDoNothing ||
                unDifEvent == eMngMdmWaitForModemStable ||
                unDifEvent == eMngMdmWaitForModemStableShortTime ||
                unDifEvent == eMngMdmRequestSendResult_Fail ||
                unDifEvent == eMngMdmRequestSendResult_Fail_NotFoundServer ||
                unDifEvent == eMngMdmRequestSendResult_Fail_NotStableServer ||
                unDifEvent == eMngMdmRequestSendResult_Fail_SockError ||
                unDifEvent == eMngMdmRequestSendResult_Fail_General )
            {
                // if error occurred, then is going to disconnection
                newState = eModemControlStateDisconnected;
            }
            else if( unDifEvent == eMngMdmNotRegistered )
            {
                printf("eMngMdmNotRegistered: wait network\r\n");
            }
            else if( unDifEvent == eMngMdmNone)
            {
                newState = eModemControlStateSleep;
            }
            else
            {
                printf("eModemControlStateConnecting Do nothing\r\n");
            }
            break;
        case eModemControlStateConnected:
        case eModemControlStateReConnected:
            if( unDifEvent == eMngMdmGeneralError ||
                unDifEvent == eMngMdmCriticalError ||
                unDifEvent == eMngMdmNotRegistered ||
                unDifEvent == eMngMdmDoNothing ||
                unDifEvent == eMngMdmWaitForModemStable ||
                unDifEvent == eMngMdmWaitForModemStableShortTime ||
                unDifEvent == eMngMdmRequestSendResult_Fail ||
                unDifEvent == eMngMdmRequestSendResult_Fail_NotFoundServer ||
                unDifEvent == eMngMdmRequestSendResult_Fail_NotStableServer ||
                unDifEvent == eMngMdmRequestSendResult_Fail_SockError ||
                unDifEvent == eMngMdmRequestSendResult_Fail_General ||
                unDifEvent == eMngMdmDetachedBaseStation )
            {
                // if error occurred, then is going to disconnection
                newState = eModemControlStateDisconnected;
            }
            else if( unDifEvent == eMngMdmConnected ||
                unDifEvent == eMngMdmReConnected )
            {
                newState = eModemControlStateReConnected;

                //Stop Timer
                DisableSystemMessageTimer(m_nReqWaitNetworkStableTimeoutTimerID);
                // clear request action to modem manager
                ClearRequestActionFromManagerFlag();
            }
            else if( unDifEvent == eMngMdmNone)
            {
                newState = eModemControlStateSleep;
            }
            else
            {
//                printf("eModemControlStateConnected Do nothing\r\n");
            }
            break;
        case eModemControlStateDisconnected:
            if( unDifEvent == eMngMdmNone)
            {
                newState = eModemControlStateSleep;
            }
            else
            {
                //newState = eModemControlStateIdle;
                newState = eModemControlStateInit;
            }
            break;
        case eModemControlStateSleep:
            Trace("System set modem status to none.\r\n");
            break;
        default:
            Trace("======================================================\r\n");
            Trace("not defined modem state so initialize modem\r\n");
            Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerDisconnected, (stCarReport *)NULL,0);
            SetModemControlEvent(eMngMdmCriticalError);
            break;
    }

    return newState;
}

void ActionForNewState(uint32_t unModemNewState,uint32_t unModemNewEvent)
{
    stMsgMdm stMessage;

    switch(unModemNewState)
    {
        case eModemControlStateIdle:
            Trace("new action : eModemControlStateIdle\r\n");
            SetModemLedStatus(eModemLedStatus_Disconnect);
            break;
        case eModemControlStateInit:
            Trace("new action : eModemControlStateInit\r\n");
            SetModemLedStatus(eModemLedStatus_Disconnect);
            SetNetworkPostPoneFlag(true);
            ModemManagerData.bNeedStartUpCheckSMSFlag = true;
            break;
        case eModemControlStateConnecting:
            Trace("new action : eModemControlStateConnecting\r\n");
            ModemManagerData.bNeedStartUpCheckSMSFlag = true;
            break;
        case eModemControlStateConnected:
            Trace("new action : eModemControlStateConnected\r\n");
            Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerConnected, (stCarReport *)NULL,0);
            SetModemLedStatus(eModemLedStatus_Connected);
            SetWaitforSendingData(false);

            if( GetLastSendMessage(&stMessage) ==  true )
            {
                if( stMessage.header.event != eReqFota && stMessage.header.event != eRspFota )
                {
                    Retransmision(stMessage);
                }
            }

            break;
        case eModemControlStateReConnected:
            Trace("new action : eModemControlStateReConnected\r\n");
            Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerConnected, (stCarReport *)NULL,0);
            SetModemLedStatus(eModemLedStatus_Connected);
            SetWaitforSendingData(false);

            if( GetLastSendMessage(&stMessage) ==  true )
            {
                if( stMessage.header.event != eReqFota && stMessage.header.event != eRspFota )
                {
                    printf("retransfer data to server#1\r\n");
                    Retransmision(stMessage);
                }
            }

            break;
        case eModemControlStateDisconnected:
            Trace("new action : eModemControlStateDisconnected\r\n");
            Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerDisconnected, (stCarReport *)NULL,0);
            SetModemLedStatus(eModemLedStatus_Disconnect);
            break;
        case eModemControlStateSleep:
            Trace("new action : eModemControlStateSleep\r\n");
            break;
        default:
            Trace("======================================================\r\n");
            Trace("not defined modem state so initialize modem\r\n");
            Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerDisconnected, (stCarReport *)NULL,0);
            SetModemLedStatus(eModemLedStatus_Disconnect);
            //SetModemControlEvent(eMngMdmCriticalError);
            break;
    }
}


// related with current event
void SetModemControlEvent(int32_t nStatus)
{
    m_unModemControlEvent |= nStatus;  
	PushModemStateCollection(ModemNewEvent, m_unModemControlEvent);
}

uint32_t GetModemControlEvent()
{
    uint32_t unEvent = m_unModemControlEvent;
    m_unModemControlEvent = eMngMdmNone;
    return unEvent;
}

// related with old event
void SetModemControlOldEvent(uint32_t unOldEvent)
{
    m_unModemControlOldEvent = unOldEvent;
}

uint32_t GetModemControlOldEvent()
{
    return m_unModemControlOldEvent;
}

boolean_t m_bReqWaitNetworkCheck = false;

void SetReqWaitNetworkCheck(boolean_t bReqWaitNetworkCheck)
{
    m_bReqWaitNetworkCheck = bReqWaitNetworkCheck;
}

boolean_t GetReqWaitNetworkCheck()
{
    return m_bReqWaitNetworkCheck;
}

uint32_t m_unModemLastEvent = 0;

void SetModemControlLastEvent(uint32_t unModemLastEvent)
{
    m_unModemLastEvent = unModemLastEvent;
}

uint32_t GetModemControlLastEvent()
{
    return m_unModemLastEvent;
}

void HandlerModemState()
{
    //boolean_t bRequestNetworkTime = false;
    uint32_t unModemNewState;

    uint32_t unModemOldState = GetModemControlOldState();
    // previous modem event
    uint32_t unModemOldEvent = GetModemControlOldEvent();
    // main event from modem
    uint32_t unModemNewEvent = GetModemControlEvent();

    if( unModemOldEvent != unModemNewEvent )
    {
        Trace("Old event : %x, New event : %x\r\n",unModemOldEvent, unModemNewEvent);
        Trace("Diff event : %x\r\n",unModemOldEvent^unModemNewEvent);

        // get a new statue from modem
        // in this routine check below list to change the modem states
        // 1. modem states
        // 2. reuslt of sending data
        // 3. rssi
        //SetModemOldState(cur state);
        //SetModemCurState(new state);
        unModemNewState = DecideNewModemState(unModemNewEvent, unModemOldEvent, unModemOldState);

        Trace("Old State : %x, New State : %x\r\n",unModemOldState,unModemNewState);
        
		PushModemStateCollection(ModemNewState, unModemNewState);
		PushModemStateCollection(ModemOldState, unModemOldState);

        if( unModemOldEvent^unModemNewEvent >= eMngMdmRequestSendResult_Success )
        {
            // this is handler for checking a result of sending the data to the server.
            HandlerRequestSendResult(unModemOldEvent^unModemNewEvent);
            SetModemLedStatus(eModemLedStatus_Connected);
        }

        // new state action
        ActionForNewState(unModemNewState,unModemNewEvent);
        // new event action
        ActionForNewEvent(unModemNewEvent);

        // clear old event
        SetModemControlOldEvent(0);
        // set previous state
        SetModemControlOldState(unModemNewState);
        // set last event
        SetModemControlLastEvent(unModemNewEvent);

        // this is for debugging message
        Trace("ModemManager States : %d\r\n",ModemManagerData.eState);
    }

    // ME reuest below information
    
    
    
    //Trace("m_bReqSleep : %d, WaitNetworkCheck : %d, GetLastSendMessageFlag : %d, GetModemControlLastEvent : %d, ModemManagerData.eFOTAStartState : %d\r\n", 
     //         m_bReqSleep,GetReqWaitNetworkCheck(), GetLastSendMessageFlag(),GetModemControlLastEvent(), ModemManagerData.eFOTAStartState) ;
    
    if( m_bReqSleep == false && GetReqWaitNetworkCheck() == false && GetLastSendMessageFlag()==false &&
        GetModemControlLastEvent() >= eMngMdmRegistedNetwork &&
        ModemManagerData.eFOTAStartState == eFOTA_START_STATE_STOP )
    {
        // if there isn't change anything then, get a rssi or network time.
        // every one second for rssi
        // every five seconds for entwork time
        // if gemalto is connected with base station then request
        // network time check Handler()
        // every 1 minute we should check network time for time zone
        RequestNetworkTime();
    }
    if( m_bReqSleep == false && GetReqWaitNetworkCheck() == false && GetLastSendMessageFlag()==false &&
        GetModemControlLastEvent() >= eMngMdmInitializeSoftware &&
        ModemManagerData.eFOTAStartState == eFOTA_START_STATE_STOP )
    {

        // rssi request handler()
        // every 5 seconds we should request rssi
        RequestRssi();
    }
}

/*
boolean_t m_bModemAvailable = true;

void IsModemAvailable()
{
    return m_bModemAvailable;
}


void SetGemaltoModemAvailable(boolean_t bModemAvailable)
{
    m_bModemAvailable = bModemAvailable;
}
*/

void ReqForcelyModemReset()
{
    InitializeModemManager(true);

    //SetRequestActionFromManager(eMdmReqGemaltoReset,true);
    printf("PowerOnGemaltoModem_Modem_Reset_Power2\r\n");
	HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);

#ifdef RF_COMMON_MODEM  //mod.kks 21.10.25
	APP_Delay(200);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
    APP_Delay(200);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
    APP_Delay(200);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
    //APP_Delay(200);
	//HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
#else
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
    APP_Delay(20);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
    APP_Delay(20);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
    APP_Delay(20);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
    APP_Delay(20);
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
	APP_Delay(10);
#endif



}

void SetRemoteControlStatus(boolean_t bRemoteControlStatus)
{
    m_bRemoteControlStatus = bRemoteControlStatus;
}

boolean_t GetRemoteControlStatus()
{
    return m_bRemoteControlStatus;
}

void ModemStateBufferClear()
{
	g_ModemStateStructure.rdindex = 0;
	g_ModemStateStructure.wrindex = 0;
}

void PutModemStateBuffer(char* PutModemState)
{
  // ring에 데이터 저장

  memcpy(g_ModemStateStructure.ModemStateCollection+g_ModemStateStructure.wrindex, PutModemState, sizeof(MODEM_STATE_COLLECTION));
  //memcpy(&ModemStateStructure.ModemStateCollection, pPutModemState, sizeof(MODEM_STATE_COLLECTION));

  // ring tag 조정
  g_ModemStateStructure.wrindex = ( g_ModemStateStructure.wrindex +1) % MAX_MODEMSTATEBUFFER_SIZE; // head 증가

  if(g_ModemStateStructure.wrindex == g_ModemStateStructure.rdindex)
  {
  	 g_ModemStateStructure.rdindex = ( g_ModemStateStructure.rdindex + 1) % MAX_MODEMSTATEBUFFER_SIZE;  // tail 증가
  
}
}

int GetModemStateBuffer(MODEM_STATE_COLLECTION *pGetModemState)
{
  // 큐에 데이터가 없다면 복귀
  if ( g_ModemStateStructure.wrindex == g_ModemStateStructure.rdindex ){
    return 0; // 테이터 없음
  }

  // 큐 데이터 구하기
  memcpy( pGetModemState, g_ModemStateStructure.ModemStateCollection+g_ModemStateStructure.rdindex, sizeof(MODEM_STATE_COLLECTION));

  g_ModemStateStructure.rdindex = ( g_ModemStateStructure.rdindex + 1) % MAX_MODEMSTATEBUFFER_SIZE;  // tail 증가

  return 1;
}

