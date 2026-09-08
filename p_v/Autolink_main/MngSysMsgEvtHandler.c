/* Includes ------------------------------------------------------------------*/
#include "AutolinkConfig.h"
#include "Message_Manager.h"
#include "OBD_Manager.h"
#include "GIT_Util.h"

#include "OBD_Controller_Get.h"
#include "Share_InterFunction.h"

#include "MngSystem.h"
#include "MngQueue.h"
#include "HdDebug.h"
#include "MngSystemUtil.h"
#include "dukpt_algo.h"
#include "aes.h"
#include "CanFD_Defines.h"
#include "HandlerTracking.h"

//#define ENABLE_DROP_GPS_ZERO_MESSAGE

#define ENABLE_FOTA

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM_HADNLER,__VA_ARGS__)

extern BR_SystemInfo BkSram_SystemInfo;

// external function definitions
extern void HdMsgEvtFota(stMsgSysMsg* pstReport);
extern void HdMsgEvtMessage(stMsgSysMsg* pstReport);
extern void HdMsgEvtModem(stMsgSysMsg* pstReport);
extern void HdMsgEvtStorage(stMsgSysMsg* pstReport);

extern boolean_t SetSystemDrivingKey(long long value);
extern void GetSystemDrivingKey(long long* value);

extern void SetMngSysMsgHdState(int32_t iState);
extern uint16_t key_display(long long llValue);

extern void HandlerAgps(stMsgSysMsg* pstMsgSysMsg);
extern void CheckExpiretAGPSData();
extern uint32_t GetUTCTime(); //mod.kks 21.11.05 to warnning check

// main control data of message handler
extern void SetPhoneNubmer2AutolinkConfig(int8_t* buffer,int32_t size);
extern void SetNetworkTime2AutolinkConfig(stNetworkTime stNetworkDate);

extern stMsgHandlerData m_stMsgHdData;
extern stMngSysData m_MngSysData;
extern boolean_t m_bPostponeReqSleep;
extern boolean_t m_bNeedUpdate;
extern unsigned int g_uiSetDoorSignal;
extern stLockState g_stLockState;
extern boolean_t m_bRecoveryMessage;


extern unsigned char g_ucBTControlKey[]; //dahae
extern void SetForwardingSmartKey2BtFlag(boolean_t bForwardingSmartKey2Bt);

boolean_t GetGuardActive();

extern void IntervalTimerCallBack();
extern void TimeoutTimerCallBack();
extern uint8_t VerifyTrackingControl(stTrackingControl* pstTrackingValue);

boolean_t isRcvResultforLastMessage();
void SetResultforLastMessage(boolean_t iStatus);

void ProcessModemStatus(stMsgSysMsg* pstMsgSysMsg);
void ProcessReqSmartkey(stMsgSysMsg* pstMsgSysMsg);
void ProcessRspSmartKey(stMsgSysMsg* pstMsgSysMsg);
void ProcessDriving(stMsgSysMsg* pstMsgSysMsg);
void ProcessAfterDriving(stMsgSysMsg* pstMsgSysMsg);
void ProcessBeforeDriving(stMsgSysMsg* pstMsgSysMsg);

void SendGeoFenceAlramReport(int32_t nEvent, int32_t nResult, stGeofenceUnit* pstCurGeofence, stGeofenceUnit* pstSetGeofence,eGeoFenceType eGeofenceType,uint32_t unGeofenceID);

void ClearReportSequenceNumber();
uint16_t GetReportSequenceNumber();

void SendAlramReport(int32_t nEvent, int32_t nResult);
extern void SetGeoFenceUnit(stReportSmartReq stGeofenceValue);
extern void SetRealPowerOffTime(uint32_t unUTCTime);
extern void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern void SetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern void SetUserActionSettingfromServer(stReportSetting rpSetting);
extern uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
extern int ResponseSmartKey(int iResult, int iReason,stMsgSysMsg* pstMsgSysMsg);
extern void SetPostponeSleepFlag(boolean_t bPostpone);
extern void SetupForInterruptforImpulse(bool bWomActive,bool bGyroActive, unsigned char ncValue,bool bHelpInterrupSignal);
extern void ResponseSmartKey2Bt(stMsgSysMsg* pstMsgSysMsg);
extern void SetRequestSystemReset(boolean_t bSystemReset);
extern void ClearSystemResetCount();
extern void CheckColdBooting();

stMsgSysMsg m_stRspSmartkey;
boolean_t m_bSaveRspSmartKey = false;

uint16_t m_usReportSequenceNumber = 0;
stMsgMdm m_stLastDrivingReport;

boolean_t m_bReqSmartkey = false;
boolean_t m_bBlockMsgTransfer = false;

extern boolean_t m_bForwardingSmartKey2Bt;
extern boolean_t GetForwardingSmartKey2BtFlag();
extern void SetForwardingSmartKey2BtFlag(boolean_t bForwardingSmartKey2Bt);
extern void HandlerIpek(stMsgSysMsg* pstMsgSysMsg);
extern void SetReceivedNetworkTime(boolean_t bNetworkTime);
extern unsigned int Get_Odmeter(void);
extern bool g_bAnyEventSentFlag;
extern int ResponseUserSetting(int iResult, int iReason, stMsgSysMsg* pstMsgSysMsg);

#if defined(REASON_8BYTE)
void ResponseSysSmartkey(stMsgSysMsg* pstMsgMdm,uint8_t ucResult, uint64_t ullReason);
#else
void ResponseSysSmartkey(stMsgSysMsg* pstMsgMdm,uint8_t ucResult, uint32_t ucReason);
#endif

void SetBlockMsgTrasfer(boolean_t bStop);
boolean_t GetBlockMsgTrasfer();
#if defined(PROTOCOL17)
extern stURLInfo g_stTempURLInfo;
extern unsigned char m_carrIv[];
#endif


#if defined(PROTOCOL18)
extern stRemote_PTCL_PAYLOAD g_stResRemotePayload;
extern unsigned char g_ucDoneBTControlKey[];
extern unsigned char g_ucDoneBTControlKeyResult;
#if defined(REASON_8BYTE)
extern uint64_t g_ullDoneBTControlKeyReason;
#else
extern unsigned long g_ulDoneBTControlKeyReason;
#endif
#endif

#if defined(PROTOCOL19)
extern stCFDControl m_stCFDCtrl;
#endif	//defined(PROTOCOL19)

extern boolean_t m_bAutoReadFlag;

//#define TEST_MODE_ONLY_MODEDM

uint16_t GetReportSequenceNumber()
{
    return m_usReportSequenceNumber++;
}

void ClearReportSequenceNumber()
{
    m_usReportSequenceNumber=0;
}

boolean_t m_bWrongDrivingKey = false;

boolean_t IsItPossibleSendingMessage()
{
    Trace("==========================================================\r\n");
    // if modem activation is false
    // we should block all message to send the modem manager.
    if( GetBlockMsgTrasfer() == true )
    {
        Trace("#################################################\r\n");
        Trace("System block to transfer\r\n");
        Trace("#################################################\r\n");

        return false;
    }

	g_bAnyEventSentFlag = true;	//서버에 올린데이터가있으면 체크

    // if sett up the flag, skip all data except dtc/emergency
    if( m_stMsgHdData.bSkipMessage == true )
    {
        Trace("================================================================\r\n");
        Trace("Skip All data for smartkey message\r\n");
        Trace("Skip All data for smartkey message\r\n");

        return true;
    }

    Trace("==========================================================\r\n");
    Trace("syspend flag : %d, pending flag : %d, server flag : %d, storage flag : %d, wait for network time : %d\r\n",
        m_stMsgHdData.bSuspend,
        m_stMsgHdData.bReqPostPondforSending,
        m_stMsgHdData.bServerConnected,
        m_stMsgHdData.bIsStoreData,
        m_stMsgHdData.bReqWaitSendingforNetworkTime);

    if( (m_stMsgHdData.bSuspend == true) ||
        (m_stMsgHdData.bReqPostPondforSending == true) ||
    (m_stMsgHdData.bServerConnected == false) ||
    (m_stMsgHdData.bIsStoreData == true) ||
    (m_stMsgHdData.bReqWaitSendingforNetworkTime == true) )
    {
        Trace("==========================================================\r\n");
        Trace("%s] : false\r\n",__FUNCTION__);
        return false;
    }

	if( m_bRecoveryMessage == true )
	{
		return false;
	}

    Trace("==========================================================\r\n");
    Trace("%s] : true\r\n",__FUNCTION__);

    // wait for respoinse for the request sending data
    m_stMsgHdData.bReqPostPondforSending = true;
    // request status of stroage.
    //MONI 2018-03-13
    // it is duplicated so block
    //Send2MngStorage(eMngSysMsg,eReqDataExist,0,(stCarReport *)NULL,0);
    return true;
}

void ProcessBeforeDriving(stMsgSysMsg* pstMsgSysMsg)
{
    // if obd send before driving message to server,
    // system should be set about driving
    // driving key and etc
    if( SetSystemDrivingKey(pstMsgSysMsg->header.drivingKey) == false )
    {
        Trace("Transfer Wrong Driving Key\r\n");
        m_bWrongDrivingKey = true;
    }

    ClearReportSequenceNumber();

    // start driving interval timer to report
    //if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_PARKING)
    {
        //MONI 2018-02-09
        // change define to configuration variable
        //EnableSystemMessageTimer(&m_stMsgHdData.iIntervalTimerID,TIMER_DRIVING_INTERVAL_VALUE,eSWTimer_INFINITE,IntervalTimerCallBack);
        uint32_t unDrivingIntervalTime;
        GetBackupRamConfigProperty(eBackupRamConfig_DrivingInterval,(void*)&unDrivingIntervalTime);

        EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iIntervalTimerID,unDrivingIntervalTime,eSWTimer_INFINITE,IntervalTimerCallBack);

        // if power on is received, after ig off recevied.
        // then we reinitialize ig off flag.
        m_stMsgHdData.bIGOff = false;

        m_MngSysData.bPowerOff = false;
        //m_stMsgHdData.bCarPowerOn = true;

        m_MngSysData.m_iSystemMode = SYSTEM_MODE_DRIVING;

        SetMngSysMsgHdState(STAT_SYS_MNG_HD_MSG_SETUP);
        //Send2MngSysMsg(eMngSysMsg,eReqChangeMode,eSysModeDriving,(stCarReport *)NULL);

        pstMsgSysMsg->header.id = eMngSysMsg;
        pstMsgSysMsg->header.unEventTime = GetLocalTimefromTime(GetUTCTime());
        //GetLocalTimeforDate(&pstMsgSysMsg->header.stDate);

#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
		if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            pstMsgSysMsg->header.unEventTime = 0;
        }

        Trace("send data to storage\r\n");
        pstMsgSysMsg->header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
#else
        if( IsItPossibleSendingMessage() == true )
        {
            Trace("send data to modem\r\n");
            pstMsgSysMsg->header.event = eReqReport;
            Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
        }
        else
        {
            if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
            {
                pstMsgSysMsg->header.unEventTime = 0;
            }

            Trace("send data to storage\r\n");
            pstMsgSysMsg->header.event = eReqSaveReport;
            Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
        }
#endif	//ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
        Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
#endif // #ifndef TEST_MODE_ONLY_MODEDM
    }
    //else
    //{
    //    Trace("\n=======================================================\r\n");
    //    Trace("%s] before driving message occurred in driving mode\r\n",__FUNCTION__);
    //    Trace("=======================================================\r\n");
    //}
}

void SendLastOddMessage()
{
    // current sequence number
    uint16_t usNextSequence = m_usReportSequenceNumber;

    // this sequence is increased with one in last sequence of saved message.
    if( (usNextSequence%2) == 1 )
    {
        Trace("=====================================================\r\n");
        Trace("Odd message sent to server\r\n");
        // clear the body2 because we sent last odd message.
        memset((char*)&m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB2,
        0,
        sizeof(stReportDrivingInfoBody));

        m_stLastDrivingReport.header.id = eMngSysMsg;
        m_stLastDrivingReport.header.subEvent = eR_DrivingInterval;

#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE
        if( m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLatitude == 0.0 &&
            m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLongitude == 0.0 )
        {
            Trace("Message Drop Because of GPS DATA NULL\r\n");
            return;
        }
#endif //#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE


#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
		if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            m_stLastDrivingReport.header.unEventTime = 0;
        }

        Trace("send data to storage\r\n");
        m_stLastDrivingReport.header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)&m_stLastDrivingReport);
#else
        if( IsItPossibleSendingMessage() == true )
        {
            Trace("send data to modem\r\n");
            m_stLastDrivingReport.header.event = eReqReport;
            Send2MngModem2((stMsgMdm*)&m_stLastDrivingReport);
        }
        else
        {
            if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
            {
                m_stLastDrivingReport.header.unEventTime = 0;
            }

            Trace("send data to storage\r\n");
            m_stLastDrivingReport.header.event = eReqSaveReport;
            Send2MngStorage2((stMsgStorage*)&m_stLastDrivingReport);
        }
#endif	// ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
        m_stLastDrivingReport.header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)&m_stLastDrivingReport);
#endif // #ifndef TEST_MODE_ONLY_MODEDM
        // log
        //DisplayReport("mngSysMsg]Send Data\n",(stMsgSysMsg*)&m_stLastDrivingReport);
    }
}

void ProcessAfterDriving(stMsgSysMsg* pstMsgSysMsg)
{
    // inititalize flag for checking drivingkey
    m_bWrongDrivingKey = false;
	uint32_t unUTCTime;

    GetQueueState();
    // reset time out timer
    //MONI 2018-02-09
    // change define to configuration variable
    //EnableSystemMessageTimer(&m_stMsgHdData.iTimeoutTimerID, TIMER_SYSTEM_TIMEOUT_INTERVAL_VALUE, eSWTimer_INFINITE, TimeoutTimerCallBack);
    uint32_t unSystemTimeout;
    GetBackupRamConfigProperty(eBackupRamConfig_SystemTimeout,(void*)&unSystemTimeout);
    EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout, eSWTimer_INFINITE, TimeoutTimerCallBack);


    //if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_DRIVING)
    {
        DisableSystemMessageTimer(m_stMsgHdData.iIntervalTimerID);
        m_MngSysData.bPowerOff = true;
        m_MngSysData.m_iSystemMode = SYSTEM_MODE_PARKING;

        SetMngSysMsgHdState(STAT_SYS_MNG_HD_MSG_SETUP);
        //Send2MngSysMsg(eMngSysMsg,eReqChangeMode,eSysModeParking,(stCarReport *)NULL);

        pstMsgSysMsg->header.id = eMngSysMsg;
		unUTCTime = GetUTCTime();
        pstMsgSysMsg->header.unEventTime = GetLocalTimefromTime(unUTCTime);

        //GetLocalTimeforDate(&pstMsgSysMsg->header.stDate);
        // set power off time for fota update
		SetRealPowerOffTime(unUTCTime);

        // save odometer to system.
        m_stMsgHdData.stObdSettingValue.ObdSetting.unOdometer = pstMsgSysMsg->carReport.rpInterval.AfterDrivingInfo.Odometer;

#ifndef QA_FIFA //Origin driving info : interval 1 min, 2packet | QA FIFA driving info : interval 15 sec, 1 packet
        // MONI 2018-03-06
        // check process that if sequence number is odd, then the device send last message
        // because basically the device send one message that include 2 interval message.
        SendLastOddMessage();
#endif

#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE
        if( pstMsgSysMsg->carReport.rpInterval.AfterDrivingInfo.PowerOnLatitude == 0.0 &&
            pstMsgSysMsg->carReport.rpInterval.AfterDrivingInfo.PowerOnLongitude == 0.0 &&
            pstMsgSysMsg->carReport.rpInterval.AfterDrivingInfo.EndGpsLatitude == 0.0 &&
            pstMsgSysMsg->carReport.rpInterval.AfterDrivingInfo.EndGpsLongitude == 0.0 )
        {
            Trace("Message Drop Because of GPS DATA NULL\r\n");
            return;
        }
#endif //#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE

        // after driving always send to storage
		if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
		{
			pstMsgSysMsg->header.unEventTime = 0;
		}

		Trace("send data to storage\r\n");
		pstMsgSysMsg->header.event = eReqSaveReport;
		Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);

/*#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
		if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
		{
			pstMsgSysMsg->header.unEventTime = 0;
		}

		Trace("send data to storage\r\n");
		pstMsgSysMsg->header.event = eReqSaveReport;
		Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
#else
        if( IsItPossibleSendingMessage() == true )
        {
            Trace("send data to modem\r\n");
            pstMsgSysMsg->header.event = eReqReport;
            Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
        }
        else
        {
            if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
            {
                pstMsgSysMsg->header.unEventTime = 0;
            }

            Trace("send data to storage\r\n");
            pstMsgSysMsg->header.event = eReqSaveReport;
            Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
        }
#endif	//ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
        Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
#endif // #ifndef TEST_MODE_ONLY_MODEDM*/
    }
    //else
    //{
    //    Trace("\n=======================================================\r\n");
    //    Trace("%s] after driving message occurred in nodriving mode\r\n",__FUNCTION__);
    //    Trace("=======================================================\r\n");
    //}
}

#if defined(PROTOCOL18)
void ProcessCharging(stMsgSysMsg* pstMsgSysMsg)
{
	pstMsgSysMsg->header.id = eMngSysMsg;
	pstMsgSysMsg->header.event = eReqReport;
	pstMsgSysMsg->header.subEvent = eR_Charging;
	Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
}
#endif

void SetNewOdometer(stMsgSysMsg* pstMsgSysMsg)
{
    SetSystemOdometer(pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.Odometer);
}

void ProcessDriving(stMsgSysMsg* pstMsgSysMsg)
{
    uint16_t seq = GetReportSequenceNumber();
    GetSystemDrivingKey((long long *)&pstMsgSysMsg->header.drivingKey);

    //Trace("%s]drivekey\r\n",__FUNCTION__);
    //key_display(pstMsgSysMsg->header.drivingKey);

    pstMsgSysMsg->header.id = eMngSysMsg;
    pstMsgSysMsg->header.event = eReqReport;
    pstMsgSysMsg->header.subEvent = eR_DrivingInterval;

#ifndef QA_FIFA //Origin driving info : interval 1 min, 2packet | QA FIFA driving info : interval 15 sec, 1 packet
    if( (seq % 2) == 1 )
    {
        //GetLocalTimeforDate(&pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB2.stDate);

        memcpy((int8_t*)&m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB2,(int8_t*)&pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1,sizeof(stReportDrivingInfoBody));

        // seconds body time
        //GetLocalTimeforDate(&m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB2.stDate);
        m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB2.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
        m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB2.OccurredEventUtcTime = GetUTCTime();

        // set header time to send
        // memcpy((int8_t*)&pstMsgSysMsg->header.stDate,(int8_t*)&m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB2.stDate,sizeof(stHalRTCTypeDef));

        m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB2.SequnceNumber = seq;
        m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB2.OperationKey = pstMsgSysMsg->header.drivingKey;

        m_stLastDrivingReport.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
		if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
		{
			m_stLastDrivingReport.header.unEventTime = 0;
		}

		Trace("send data to storage\r\n");
		m_stLastDrivingReport.header.event = eReqSaveReport;
		Send2MngStorage2((stMsgStorage*)&m_stLastDrivingReport);
#else
        if( IsItPossibleSendingMessage() == true )
        {
            Trace("send data to modem\r\n");
            m_stLastDrivingReport.header.event = eReqReport;
            Send2MngModem2((stMsgMdm*)&m_stLastDrivingReport);
        }
        else
        {
            if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
            {
                m_stLastDrivingReport.header.unEventTime = 0;
            }

            Trace("send data to storage\r\n");
            m_stLastDrivingReport.header.event = eReqSaveReport;
            Send2MngStorage2((stMsgStorage*)&m_stLastDrivingReport);
        }
#endif	//ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
        m_stLastDrivingReport.header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)&m_stLastDrivingReport);
#endif // #ifndef TEST_MODE_ONLY_MODEDM
        // log
        //DisplayReport("mngSysMsg]Send Data\n",(stMsgSysMsg*)&m_stLastDrivingReport);

    }
    else
    {
        // first body time
        //GetLocalTimeforDate(&pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.stDate);
        pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
        pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OccurredEventUtcTime = GetUTCTime();
        pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.SequnceNumber = seq;
        pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OperationKey = pstMsgSysMsg->header.drivingKey;
        memcpy((int8_t*)&m_stLastDrivingReport,(int8_t*)pstMsgSysMsg,sizeof(stMsgSysMsg));
    }

#else //#ifndef QA_FIFA

	pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OccurredEventUtcTime = GetUTCTime();
    pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.SequnceNumber = seq;
    pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OperationKey = pstMsgSysMsg->header.drivingKey;
    memcpy((int8_t*)&m_stLastDrivingReport,(int8_t*)pstMsgSysMsg,sizeof(stMsgSysMsg));
	memset((char*)&m_stLastDrivingReport.carReport.rpInterval.DrivingInfo.DrivingInfoB2,0,sizeof(stReportDrivingInfoBody));
	if( IsItPossibleSendingMessage() == true )
    {
        Trace("send data to modem\r\n");
        m_stLastDrivingReport.header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)&m_stLastDrivingReport);
    }
    else
    {
        if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            m_stLastDrivingReport.header.unEventTime = 0;
        }
        Trace("send data to storage\r\n");
        m_stLastDrivingReport.header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)&m_stLastDrivingReport);
    }
#endif//#ifndef QA_FIFA
}


void ProcessDrivingOddMessageTest(stMsgSysMsg* pstMsgSysMsg)
{
    uint16_t seq = GetReportSequenceNumber();
    Trace("Enter ProcessDriving\r\n");

    GetSystemDrivingKey((long long *)&pstMsgSysMsg->header.drivingKey);

    //Trace("%s]drivekey\r\n",__FUNCTION__);
    //key_display(pstMsgSysMsg->header.drivingKey);

    pstMsgSysMsg->header.id = eMngSysMsg;
    pstMsgSysMsg->header.event = eReqReport;
    pstMsgSysMsg->header.subEvent = eR_DrivingInterval;

    pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OccurredEventUtcTime = GetUTCTime();
    pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.SequnceNumber = seq;
    pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.OperationKey = pstMsgSysMsg->header.drivingKey;


#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
	if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
	{
		pstMsgSysMsg->header.unEventTime = 0;
	}

	Trace("send data to storage\r\n");
	pstMsgSysMsg->header.event = eReqSaveReport;
	Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
#else
    if( IsItPossibleSendingMessage() == true )
    {
        Trace("send data to modem\r\n");
        pstMsgSysMsg->header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
    }
    else
    {
        if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            pstMsgSysMsg->header.unEventTime = 0;
        }

        Trace("send data to storage\r\n");
        pstMsgSysMsg->header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
    }
#endif	//ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
    pstMsgSysMsg->header.event = eReqReport;
    Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
#endif // #ifndef TEST_MODE_ONLY_MODEDM
    // log
    //DisplayReport("mngSysMsg]Send Data\n",(stMsgSysMsg*)&m_stLastDrivingReport);
}


void ProcessNoDriving(stMsgSysMsg* pstMsgSysMsg)
{
    GetSystemDrivingKey((long long *)&pstMsgSysMsg->header.drivingKey);

    //Trace("%s]drivekey\r\n",__FUNCTION__);
    //key_display(pstMsgSysMsg->header.drivingKey);

    pstMsgSysMsg->header.id = eMngSysMsg;
    pstMsgSysMsg->header.subEvent = eR_ParkingInterval;

    //GetLocalTimeforDate(&pstMsgSysMsg->header.stDate);
    pstMsgSysMsg->header.unEventTime = GetLocalTimefromTime(GetUTCTime());

    //GetLocalTimeforDate(&pstMsgSysMsg->carReport.rpInterval.NoDrivingInfo.stDate);

#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
	if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
	{
		pstMsgSysMsg->header.unEventTime = 0;
	}

	Trace("send data to storage\r\n");
	pstMsgSysMsg->header.event = eReqSaveReport;
	Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
#else
    if( IsItPossibleSendingMessage() == true )
    {
        Trace("send data to modem\r\n");
        pstMsgSysMsg->header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
    }
    else
    {
        if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            pstMsgSysMsg->header.unEventTime = 0;
        }

        Trace("send data to storage\r\n");
        pstMsgSysMsg->header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
    }
#endif	// ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
    Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
#endif // #ifndef TEST_MODE_ONLY_MODEDM
}

void ProcessRspSmartKey(stMsgSysMsg* pstMsgSysMsg)
{
    pstMsgSysMsg->header.id = eMngSysMsg;
    pstMsgSysMsg->header.event = eRspReport;
    pstMsgSysMsg->header.unTraceMng=eMngSysMsg;
    pstMsgSysMsg->header.subEvent = eR_RspSmartKey;

    //GetLocalTimeforDate(&pstMsgSysMsg->header.stDate);
    //pstMsgSysMsg->header.unEventTime = GetLocalTime();
    pstMsgSysMsg->header.unEventTime = GetLocalTimefromTime(GetUTCTime());
    pstMsgSysMsg->carReport.rpSmartKey.Response.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    pstMsgSysMsg->carReport.rpSmartKey.Response.OccurredEventUtcTime = GetUTCTime();

	Send2MngModem2((stMsgMdm*)pstMsgSysMsg);

	// enable recevie smartkey from modem.
	m_stMsgHdData.bAlreadyRcvSmartkey = false;

    // suspend sending message
    m_stMsgHdData.bSuspend = false;

    // stop skip message
    m_stMsgHdData.bSkipMessage = false;

#if defined(PROTOCOL18)
    // check message from BT
	if(GetForwardingSmartKey2BtFlag() == true) //dahae
	{
		memcpy(g_ucDoneBTControlKey, pstMsgSysMsg->carReport.rpSmartKey.Response.BTControlKey, 16); //같은 BT Key로 들어오는지 확인하기 위해
		g_stResRemotePayload.ucResult = pstMsgSysMsg->carReport.rpSmartKey.Response.Result; //BT로 응답
#if defined(REASON_8BYTE)
		g_stResRemotePayload.ullReason = pstMsgSysMsg->carReport.rpSmartKey.Response.Reason;	//reason type change 210824
#else
		g_stResRemotePayload.ulReason = pstMsgSysMsg->carReport.rpSmartKey.Response.Reason;
#endif

		g_ucDoneBTControlKeyResult = pstMsgSysMsg->carReport.rpSmartKey.Response.Result;//BT 연결 끊킨 후 SIM 제어에 대한 응답
#if defined(REASON_8BYTE)
		g_ullDoneBTControlKeyReason = pstMsgSysMsg->carReport.rpSmartKey.Response.Reason;
#else
		g_ulDoneBTControlKeyReason = pstMsgSysMsg->carReport.rpSmartKey.Response.Reason;
#endif

	    SetForwardingSmartKey2BtFlag(false);
	}
#endif
}

uint8_t m_cRcvSystemResetCmd = 0;

void ProcessHandlerRemoteControl(stMsgSysMsg* pstMsgSysMsg)
{
    switch( pstMsgSysMsg->carReport.rpSmartKey.Request.CommandType)
    {
        case eREMOTE_CON_CMD_TYPE_NONE:
            break;
        case eREMOTE_CON_CMD_TYPE_STARTING:
            // set flag for a exception handler
            if( pstMsgSysMsg->carReport.rpSmartKey.Request.ControlType == 1 )
            {
                m_bReqSmartkey = true;
                m_stMsgHdData.bSkipMessage = true;
            }
        case eREMOTE_CON_CMD_TYPE_DOOR:
        case eREMOTE_CON_CMD_TYPE_EMERGENCY_LIGHT:
        case eREMOTE_CON_CMD_TYPE_HORN_EMERGENCY_LIGHT:
        case eREMOTE_CON_CMD_TYPE_DTC_STATUS:
            Trace("Rcv Remote Control\r\n");

            // MONI 2018-03-15
            // if modem activation is false
            // we should block all message to send the modem manager.
            if( GetBlockMsgTrasfer() == true )
            {
                Trace("#################################################\r\n");
                Trace("System block to transfer\r\n");
                Trace("#################################################\r\n");

                return;
            }

            //MONI 2018-02-18
            // why do we cast "stCarReport*" check.
            Send2MngObd(eMngSysMsg, eReqReport, eR_ReqSmartKey ,(stCarReport*)&pstMsgSysMsg->carReport, pstMsgSysMsg->header.unTraceMng);

            // prevent to reenter.
            m_stMsgHdData.bAlreadyRcvSmartkey = true;

            // suspend sending message
            m_stMsgHdData.bSuspend = true;

            // send suspend an event to storage to stop a report
            Send2MngStorage(eMngSysMsg,eReqSuspend,eSuspendStart,(stCarReport *)NULL,0);

            break;

        case eREMOTE_CON_CMD_TYPE_FOTA:
            // MONI 2018-03-06
            // we block these code because the fota procedure is changed to make one fota procedure
            //Trace("Rcv Force Fota\r\n");
            //// this is force fota test
            //m_bPostponeReqSleep = true;
            //// send request fota info
            //Send2MngModem(eMngSysMsg,eReqFota,eFwInfo,(stCarReport *)NULL,pstMsgSysMsg->header.unTraceMng);
            Trace("Not defined\r\n");
            break;

        case eREMOTE_CON_CMD_TYPE_RESET:
            if( m_cRcvSystemResetCmd++ < 2 )
            {
                Trace("Rcv Request System Reset : %d\r\n",m_cRcvSystemResetCmd);
                Send2MngSys(eMngSysMsg,eReqSleep,0, (stCarReport *)NULL,0);
                SetRequestSystemReset(true);

                break;
            }

            SystemForcelyReset();

            break;
        case eREMOTE_CON_CMD_TYPE_GEO_FENCE:
        case eREMOTE_CON_CMD_TYPE_SENSOR_SENSITIVITY:
        case eREMOTE_CON_CMD_TYPE_VALET:
        case eREMOTE_CON_CMD_TYPE_TOWING:
        case eREMOTE_CON_CMD_TYPE_GUARD:
            Trace("Not defined\r\n");
            break;
        case eREMOTE_CON_CMD_TYPE_DATA:
            Trace("ME should add to handler for control of data transfer\r\n");
            break;
    }
}

#if defined(REASON_8BYTE)
void ResponseSysSmartkey(stMsgSysMsg* pstMsgMdm,uint8_t ucResult, uint64_t ullReason)
#else
void ResponseSysSmartkey(stMsgSysMsg* pstMsgMdm,uint8_t ucResult, uint32_t ucReason)
#endif
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    memcpy(msg.rpSmartKey.Response.Guid,pstMsgMdm->carReport.rpSmartKey.Request.Guid,MAX_GUID_LENGTH);

    msg.rpSmartKey.Response.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.rpSmartKey.Response.OccurredEventUtcTime = GetUTCTime();
    msg.rpSmartKey.Response.Result = ucResult;
#if defined(REASON_8BYTE)
    msg.rpSmartKey.Response.Reason = ullReason;
#else
	msg.rpSmartKey.Response.Reason = ucReason;
#endif
    msg.rpSmartKey.Response.ReceviedTime = pstMsgMdm->carReport.rpSmartKey.Request.OccurredEventTime;
#if defined(PROTOCOL18)
	memcpy(msg.rpSmartKey.Response.BTControlKey, pstMsgMdm->carReport.rpSmartKey.Request.BTControlKey, 16);
#endif

	// send response with protocl /40 : eSysSmartkeyReqType_Modem
	// send response with protocl /44 : eSysSmartkeyReqType_Bt
	// send response with protocl /97 : eSysSmartkeyReqType_Sys
	msg.rpSmartKey.Response.ucSysSmartkeyReqType = pstMsgMdm->carReport.rpSmartKey.Request.ucSysSmartkeyReqType;

    Trace("Response Smartkey in System Message Manager\r\n");

    Send2MngModem(eMngSysMsg,eRspReport,eR_RspSmartKey,&msg,0);
}

void ProcessReqSmartkey(stMsgSysMsg* pstMsgSysMsg)
{
    // geofence setting is included request smart key message
    if ( pstMsgSysMsg->header.subEvent == eR_ReqSmartKey ||
         pstMsgSysMsg->header.subEvent == eR_SettingGeofence )
    {
        // request from server to control the car
        // 1. stop all timer for parking and driving
        // 2. and then save all parking and driving data
        // 3. and change the main status for smartkey

        if( (m_stMsgHdData.bAlreadyRcvSmartkey == false) && (GetForwardingSmartKey2BtFlag() == false) )
        {
#if defined(PROTOCOL18)
        	// if session key isn't same with sim session key, then new remote control process
			if( memcmp(pstMsgSysMsg->carReport.rpSmartKey.Request.BTControlKey, g_ucDoneBTControlKey, 16) != 0 )
#endif
			{
	            //handler for remote control
	            ProcessHandlerRemoteControl(pstMsgSysMsg);

				// refresh ig off flag to prevent to go sleep.
				m_stMsgHdData.bIGOff = false;
			}
			else
			{
#if defined(PROTOCOL18)
				//else if(memcmp(pstMsgSysMsg->carReport.rpSmartKey.Request.BTControlKey, g_ucDoneBTControlKey, 16)==0) // 같은 BTKEY로 들어온 경우
				{
					// bt session and sim session is same, reject sim remote control
#if defined(REASON_8BYTE)
					uint64_t ullDoneBTKeyReason = 0x20000000|g_ullDoneBTControlKeyReason; // BT로 이미 제어완료 + 제어 실패 사유
					ResponseSysSmartkey(pstMsgSysMsg,g_ucDoneBTControlKeyResult,ullDoneBTKeyReason);
#else
					unsigned long ulDoneBTKeyReason = 0x20000000|g_ulDoneBTControlKeyReason; // BT로 이미 제어완료 + 제어 실패 사유
					ResponseSysSmartkey(pstMsgSysMsg,g_ucDoneBTControlKeyResult,ulDoneBTKeyReason);
#endif
				}
#endif
			}
        }
        else
        {
            // send reject event to server
            // because this system handle only one event at a one time
            Trace("==================================================\r\n");
            Trace("==================================================\r\n");
            Trace("Reject a smartkey request, already received\r\n");
            Trace("==================================================\r\n");
            ResponseSysSmartkey(pstMsgSysMsg,eREMOTE_CON_RESULT_FAIL,0x2000);
        }
    }
    else
    {
        GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
    }
}

#if defined(PROTOCOL17)

//void ProcessReqSmartkey(stMsgSysMsg* pstMsgSysMsg)
//{
//    // geofence setting is included request smart key message
//    if ( pstMsgSysMsg->header.subEvent == eR_ReqSmartKey ||
//        pstMsgSysMsg->header.subEvent == eR_SettingGeofence )
//    {
//        // request from server to control the car
//        // 1. stop all timer for parking and driving
//        // 2. and then save all parking and driving data
//        // 3. and change the main status for smartkey
//
//        if( m_stMsgHdData.bAlreadyRcvSmartkey == false )
//        {
//            //handler for remote control
//            ProcessHandlerRemoteControl(pstMsgSysMsg);
//
//			// refresh ig off flag to prevent to go sleep.
//			m_stMsgHdData.bIGOff = false;
//        }
//        else
//        {
//            // send reject event to server
//            // because this system handle only one evetn at a one time
//            Trace("==================================================\r\n");
//            Trace("==================================================\r\n");
//            Trace("Reject a smartkey request, already received\r\n");
//            Trace("==================================================\r\n");
//            ResponseSysSmartkey(pstMsgSysMsg,eREMOTE_CON_RESULT_FAIL,0x2000);
//        }
//    }
//    else
//    {
//        GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
//    }
//}

bool CheckValidURL(stMsgSysMsg* pstMsgSysMsg)
{
	bool bRet=false;
	if(strncmp((char*)g_stTempURLInfo.strUrl,"HTTP",4)==true)	bRet=true;
	return bRet;
}


void ResponseSysSetTrackingMode(stMsgSysMsg* pstMsgMdm,uint8_t ucResult, uint32_t ucReason)
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    memcpy(msg.rpSmartKey.Response.Guid,pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucArrGUID,MAX_GUID_LENGTH);
    msg.rpSmartKey.Response.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.rpSmartKey.Response.OccurredEventUtcTime = GetUTCTime();
    msg.rpSmartKey.Response.Result = ucResult;
    msg.rpSmartKey.Response.Reason = ucReason;
    msg.rpSmartKey.Response.ReceviedTime = pstMsgMdm->carReport.rpSmartKey.Request.OccurredEventTime;

    Trace("Response SysSetUrl\r\n");

    Send2MngModem(eMngSysMsg,eRspReport,eR_SetTrackingModeRsp,&msg,0);
}

void HandlerSysSetting(stMsgSysMsg* pstMsgSysMsg)
{
    if ( pstMsgSysMsg->header.subEvent == eR_ReqSetURL )
    {
		if(CheckValidURL(pstMsgSysMsg)==true)
		{
			ResponseSysSetUrl(pstMsgSysMsg,eREMOTE_CON_RESULT_SUCCESS,0);
		}
        else
        {
//#warning "add reason for fail"
			ResponseSysSetUrl(pstMsgSysMsg,eREMOTE_CON_RESULT_FAIL,1);
        }
    }
	else if( pstMsgSysMsg->header.subEvent == eR_ReqSetURLSave)
	{
		if(CheckValidURL(pstMsgSysMsg)==true)
		{
			ChangeURL(pstMsgSysMsg);
		}
		else{}
	}
	else if( pstMsgSysMsg->header.subEvent == eR_ReqSetURLInit)
	{
		GetFirmwareInfo(&g_FirmwareInfo);
		memset(&g_FirmwareInfo.m_stURLInfo,0x00,sizeof(g_FirmwareInfo.m_stURLInfo));
		SetFirmwareInfo(&g_FirmwareInfo);
		Send2MngStorage(eMngSysMsg,eReqSaveReport,eR_ReqSetURLInit,&pstMsgSysMsg->carReport,0);
	}
	else if( pstMsgSysMsg->header.subEvent == eR_SetTrackingMode )
	{
		uint8_t ucErrorCode = VerifyTrackingControl(&pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo);
		uint8_t ucResult=0,ucReason=0;
		if(ucErrorCode == eTM_REASON_NONE)
		{
			SetTrackingSetting(&pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo);

			ucResult = 1;
			ucReason = 0;
			SetPostponeSleepFlag(true);
		}
		else
		{
			ucResult = 0;
			ucReason = ucErrorCode;
		}
		ResponseSysSetTrackingMode(pstMsgSysMsg,ucResult,ucReason);
	}
    else
    {
        GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
    }
}

void ChangeURL(stMsgSysMsg* pstMsgSysMsg)
{
    unsigned char AutolinkEncryptKey[MAX_ENCRYPT_KEY_LENGTH] = {0};
	stURLInfo stTemp;
	int nEncryptResult=0;

	GetFirmwareInfo(&g_FirmwareInfo);

    memset((char*)&stTemp,0x00,sizeof(stTemp));
    GetDefaultEncryptKey((char *)AutolinkEncryptKey,true);

    // we will apply encryp process after we apply damo encryption process.
    nEncryptResult = DAMO_CRYPT_AES_EncryptEx((unsigned char *)stTemp.strUrl, (size_t*)&stTemp.nEncryptedLength,
        (const unsigned char*)&g_stTempURLInfo.strUrl, MAX_SERVER_URL_LENGTH,
        AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, m_carrIv);

	g_FirmwareInfo.m_stURLInfo.nEncryptedLength = stTemp.nEncryptedLength;
	memcpy(	g_FirmwareInfo.m_stURLInfo.strUrl,stTemp.strUrl,MAX_SERVER_URL_LENGTH+AES_PADDING_LENGTH);	//패딩포함

    if( nEncryptResult != 0 )
    {
        Trace("Encrypt error - Do Not Work\r\n");
    }
	else
	{
		g_FirmwareInfo.m_stURLInfo.nPreamble = 0xFE000728;
		SetFirmwareInfo(&g_FirmwareInfo);
	}

	Send2MngStorage(eMngSysMsg,eReqSaveReport,eR_ReqSetURLSave,&pstMsgSysMsg->carReport,0);
}

void ResponseSysSetUrl(stMsgSysMsg* pstMsgMdm,uint8_t ucResult, uint32_t ucReason)
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    memcpy(msg.rpSmartKey.Response.Guid,pstMsgMdm->carReport.rpSetting.UserSetting.RequestGUID,MAX_GUID_LENGTH);
    msg.rpSmartKey.Response.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.rpSmartKey.Response.OccurredEventUtcTime = GetUTCTime();
    msg.rpSmartKey.Response.Result = ucResult;
    msg.rpSmartKey.Response.Reason = ucReason;
    msg.rpSmartKey.Response.ReceviedTime = pstMsgMdm->carReport.rpSmartKey.Request.OccurredEventTime;

    Trace("Response SysSetUrl\r\n");

    Send2MngModem(eMngSysMsg,eRspReport,eR_ResSetURL,&msg,0);
}
#endif
void ProcessHandlerUserActionSetting(stMsgSysMsg* pstMsgSysMsg)
{
    switch( pstMsgSysMsg->carReport.rpSetting.UserSetting.ucCommandType )
    {
        case eREMOTE_CON_CMD_TYPE_NONE:
        case eREMOTE_CON_CMD_TYPE_STARTING:
        case eREMOTE_CON_CMD_TYPE_DOOR:
        case eREMOTE_CON_CMD_TYPE_EMERGENCY_LIGHT:
        case eREMOTE_CON_CMD_TYPE_HORN_EMERGENCY_LIGHT:
        case eREMOTE_CON_CMD_TYPE_FOTA:
        case eREMOTE_CON_CMD_TYPE_RESET:
        case eREMOTE_CON_CMD_TYPE_DTC_STATUS:
        default:
            Trace("Not Defined\r\n");
            break;
        case eREMOTE_CON_CMD_TYPE_GEO_FENCE:
        case eREMOTE_CON_CMD_TYPE_POLYGON_GEO_FENCE:
        case eREMOTE_CON_CMD_TYPE_SENSOR_SENSITIVITY:
        case eREMOTE_CON_CMD_TYPE_VALET:
        case eREMOTE_CON_CMD_TYPE_TOWING:
        case eREMOTE_CON_CMD_TYPE_GUARD:
        case eREMOTE_CON_CMD_TYPE_DATA:
#if defined(PROTOCOL12)
        case eREMOTE_CON_CMD_TYPE_MODEM_ACTIVE:
#endif
#if defined(PROTOCOL15)
        case eREMOTE_CON_CMD_TYPE_SENSOR_INITIALIZE:
#endif
#if defined(PROTOCOL18)
		case eREMOTE_CON_CMD_TYPE_SETTING_RSV_ENG_CTRL:
#endif
#if defined(FEATURE_EXTENSION_BOARD)
        case eREMOTE_CON_CMD_TYPE_SETTING_ANTI_THIEF:
#endif
            SetUserActionSettingfromServer(pstMsgSysMsg->carReport.rpSetting);
            break;
    }
}

void ProcessReqUserActionSetting(stMsgSysMsg* pstMsgSysMsg)
{
    // MONI 2018-03-15
    // if modem activation is false
    // we should block all message to send the modem manager.
    if( GetBlockMsgTrasfer() == true )
    {
        if( pstMsgSysMsg->carReport.rpSetting.UserSetting.ucCommandType != eREMOTE_CON_CMD_TYPE_DATA &&
            pstMsgSysMsg->carReport.rpSetting.UserSetting.ucCommandType != eREMOTE_CON_CMD_TYPE_MODEM_ACTIVE &&
            pstMsgSysMsg->carReport.rpSetting.UserSetting.ucCommandType != eREMOTE_CON_CMD_TYPE_RESET )
        {
            Trace("#################################################\r\n");
            Trace("System block to transfer\r\n");
            Trace("#################################################\r\n");

            return ;
        }
    }

    //handler for remote control
    ProcessHandlerUserActionSetting(pstMsgSysMsg);

    // response for success about remote control
    if( pstMsgSysMsg->carReport.rpSetting.UserSetting.ucCommandType != eREMOTE_CON_CMD_TYPE_MODEM_ACTIVE
#if defined(PROTOCOL15)
		&& pstMsgSysMsg->carReport.rpSetting.UserSetting.ucCommandType != eREMOTE_CON_CMD_TYPE_SENSOR_INITIALIZE
#endif
      )
        ResponseUserSetting(1,0,pstMsgSysMsg);
}
void GetNewDrivingKey(long long* pllDrivingKey)
{
    uint8_t arrTmp[64];
    stHalRTCTypeDef stDate;
	uint32_t unLocalTime;

	unLocalTime = GetLocalTimefromTime(GetUTCTime());
	GetDatefromTime2(&stDate, unLocalTime);

    sprintf((char *)arrTmp, "%04d%02d%02d%0.2d%0.2d%0.2d\x00",
                2000 + stDate.RtcDate.RTC_Year,
                stDate.RtcDate.RTC_Month,
                stDate.RtcDate.RTC_Date,
                stDate.RtcTime.RTC_Hours,
                stDate.RtcTime.RTC_Minutes,
                stDate.RtcTime.RTC_Seconds);

    printf("Generate New Driving Key: [%s]\r\n", arrTmp);
    *pllDrivingKey = (long long)atoll((char *)arrTmp);
}

void ProcessModemStatus(stMsgSysMsg* pstMsgSysMsg)
{
    if( pstMsgSysMsg->header.subEvent == eMS_ServerConnected )
    {
        m_stMsgHdData.bServerConnected = true;

        Send2MngStorage(eMngSysMsg,eMdmStatus,eMS_ServerConnected,(stCarReport *)NULL, pstMsgSysMsg->header.unTraceMng);

        // send request read data
        if( m_stMsgHdData.bIsStoreData == true )
        {
            Send2MngStorage(eMngSysMsg,eReqStartAutoRead,0,(stCarReport *)NULL, pstMsgSysMsg->header.unTraceMng);
        }
    }
    else if( pstMsgSysMsg->header.subEvent == eMS_ServerDisconnected )
    {
        m_stMsgHdData.bServerConnected = false;

        Send2MngStorage(eMngSysMsg,eMdmStatus,eMS_ServerDisconnected,(stCarReport *)NULL, pstMsgSysMsg->header.unTraceMng);

        Send2MngStorage(eMngSysMsg,eReqStopAutoRead,0,(stCarReport *)NULL, pstMsgSysMsg->header.unTraceMng);
    }
//#warning "this event is not needed for system. this is treated by modem manager"
/*
    else if( pstMsgSysMsg->header.subEvent == eMS_NoRspSysload )
    {
        m_stMsgHdData.bServerConnected = false;
        Send2MngModem(eMngSysMsg,eMdmStatus,eMS_ChangeOpenSock,(stCarReport *)NULL);
    }
*/
    else if( pstMsgSysMsg->header.subEvent == eMS_PhoneNum )
    {
        stUsimInfo info;
        memcpy((char *)&info, (char *)pstMsgSysMsg->carReport.buffer, sizeof(stUsimInfo));
        memset((char *)m_stMsgHdData.chaPhoneNo, 0, sizeof(m_stMsgHdData.chaPhoneNo));
        memcpy((char *)m_stMsgHdData.chaPhoneNo, (char *)info.cellPhoneNum, strlen((char *)info.cellPhoneNum));

        Trace("Phone NUM:%s\r\n",m_stMsgHdData.chaPhoneNo);

        SetPhoneNubmer2AutolinkConfig((int8_t*)m_stMsgHdData.chaPhoneNo,strlen((char *)m_stMsgHdData.chaPhoneNo));
    }
    else if( pstMsgSysMsg->header.subEvent == eMS_NetworkTime )
    {
        stNetworkTime stNetworkDate;
		uint32_t unOldUTCTime, unNewUTCTime;
        stNetworkDate.unNetWrokTime = pstMsgSysMsg->carReport.rpSetting.ModemSetting.stNetworkDate.unNetWrokTime;
        stNetworkDate.sTimeZone = pstMsgSysMsg->carReport.rpSetting.ModemSetting.stNetworkDate.sTimeZone;

        // change save variables
        // memcpy((int8_t*)&info,pstMsgSysMsg->carReport.buffer,sizeof(stBackupInfo));
        memcpy((int8_t*)&m_stMsgHdData.stNetworkDate,(int8_t*)&stNetworkDate,sizeof(stNetworkTime));

        // send network time to obd
        pstMsgSysMsg->header.id = eMngSysMsg;
        pstMsgSysMsg->header.event = eOBDSetting;
        pstMsgSysMsg->header.subEvent = eMS_NetworkTime;

#if 1
		unOldUTCTime = GetUTCTime(); 


		SetNetworkTime2AutolinkConfig(m_stMsgHdData.stNetworkDate);

		unNewUTCTime = GetUTCTime();  
		pstMsgSysMsg->header.unEventTime = GetLocalTimefromTime(unNewUTCTime);
		pstMsgSysMsg->carReport.rpSetting.ModemSetting.stNetworkDate.iTimeGap = CheckInitializeTime(unNewUTCTime,unOldUTCTime,stNetworkDate.sTimeZone);

#ifndef GLOBAL_SHARE_QUEUE
        MngQueueSendMessage(ID_MNG_QUEUE_OBD, (int8_t*)pstMsgSysMsg, sizeof(stMsgObd));
#else
		SendSysHdShareQueueMessage(ID_MNG_QUEUE_OBD,(uint8_t*)&pstMsgSysMsg->header,sizeof(stMsgHeader), (uint8_t*)&pstMsgSysMsg->carReport, sizeof(stCarReport));
#endif
        // send network time to obd
        //pstMsgSysMsg->header.id = eMngSysMsg;
        //pstMsgSysMsg->header.event = eOBDSetting;
        //pstMsgSysMsg->header.subEvent = eMS_NetworkTime;
        //Send2MngObd2((stMsgObd*)pstMsgSysMsg);
#else
        pstMsgSysMsg->header.unEventTime = GetLocalTimefromTime(unUTCTime);               
#ifndef GLOBAL_SHARE_QUEUE
        MngQueueSendMessage(ID_MNG_QUEUE_OBD, (int8_t*)pstMsgSysMsg, sizeof(stMsgObd));
#else
		SendSysHdShareQueueMessage(ID_MNG_QUEUE_OBD,(uint8_t*)&pstMsgSysMsg.header,sizeof(stMsgHeader), (uint8_t*)NULL, 0);
#endif

        SetNetworkTime2AutolinkConfig(m_stMsgHdData.stNetworkDate);
        
        // send network time to obd
        pstMsgSysMsg->header.id = eMngSysMsg;
        pstMsgSysMsg->header.event = eOBDSetting;
        pstMsgSysMsg->header.subEvent = eMS_NetworkTime;
        Send2MngObd2((stMsgObd*)pstMsgSysMsg);
#endif

        // 2018-03-15
        // release wait flag for initialized local time
        m_stMsgHdData.bReqWaitSendingforNetworkTime = false;

        SetReceivedNetworkTime(true);
        // cold booting check
        // if cold booting, ME'll report firmware info alram
        CheckColdBooting();

        if( m_bWrongDrivingKey == true )
        {
#ifdef USE_WRONG_DRIVING_KEY_RESTORE
            stMsgStorage stMessage;
#endif //#ifdef USE_WRONG_DRIVING_KEY_RESTORE
            long long llTemp;
            Trace("Wrong driving key, Regenerate driving key\r\n");
            m_bWrongDrivingKey = false;

            GetNewDrivingKey(&llTemp);

            pstMsgSysMsg->header.drivingKey = llTemp;
//            ProcessBeforeDriving(pstMsgSysMsg);

            // stop sensor interrupt
            SetupForInterruptforImpulse(false, false, 0xAF,false);

#ifdef USE_WRONG_DRIVING_KEY_RESTORE
            // MONI 20180429
            // policy is changed
            // if ME received wrong driving key, that data is discard

            SetSystemDrivingKey(llTemp);

            memset((char*)&stMessage,0,sizeof(stMsgStorage));
            stMessage.header.id = eMngSysMsg;
            stMessage.header.event = eReqSetDrivingKey;
            stMessage.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
            stMessage.header.drivingKey = llTemp;
            Send2MngStorage2(&stMessage);
#endif //#ifdef USE_WRONG_DRIVING_KEY_RESTORE
        }
    }
    else
    {
        GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
    }
}

void SendAlramReport(int32_t nEvent, int32_t nResult)
{
    stMsgSysMsg msg;
    long long llOperationKey;
    memset((int8_t*)&msg,0,sizeof(stMsgSysMsg));

    msg.header.id = eMngSysMsg;
    msg.header.subEvent = eR_Alram;
    msg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());

    GetSystemDrivingKey(&llOperationKey);
    msg.carReport.rpAlram.CarStatus.OperationKey = llOperationKey;
    msg.carReport.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.carReport.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    msg.carReport.rpAlram.CarStatus.EventKey = nEvent;
    memcpy(msg.carReport.rpAlram.CarStatus.EventKeyValue,(char*)&nResult,4);

#ifdef PROTOCOL15
    msg.carReport.rpAlram.CarStatus.Odometer = GetSystemOdometer();
#endif

    msg.carReport.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    msg.carReport.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
    msg.carReport.rpAlram.CarStatus.GpsSetLatitude = 0;
    msg.carReport.rpAlram.CarStatus.GpsSetLongitude = 0;
    msg.carReport.rpAlram.CarStatus.Distance = 0;
#if defined(PROTOCOL15)
	msg.carReport.rpAlram.CarStatus.Odometer = Get_Odmeter();
#endif
    msg.carReport.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;

    if( Get_GPS_Lat() == 0 && Get_GPS_Lon() == 0 )
    {
        Trace("Gps lat & lon is 0 so ME didn't report impulse alram\r\n");
        return;
    }

#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
	if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
	{
		msg.header.unEventTime = 0;
	}

	Trace("send data to storage\r\n");
	msg.header.event = eReqSaveReport;
	Send2MngStorage2((stMsgStorage*)&msg);
#else
    if( IsItPossibleSendingMessage() == true )
    {
        Trace("send data to modem\r\n");
        msg.header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)&msg);
    }
    else
    {
        if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            msg.header.unEventTime = 0;
        }

        Trace("send data to storage\r\n");
        msg.header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)&msg);
    }
#endif	//ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
    Send2MngModem(eMngSysMsg,eReqReport,eR_Alram,&msg,0);
#endif // #ifndef TEST_MODE_ONLY_MODEDM
}

void SendAlramSMSExpire(stMsgMdm* pstMsgModem,int32_t nEvent, int32_t nResult)
{
    stMsgSysMsg msg;
    long long llOperationKey;
    memset((int8_t*)&msg,0,sizeof(stMsgSysMsg));

    msg.header.id = eMngSysMsg;
    msg.header.subEvent = eR_Alram;
    msg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
    GetSystemDrivingKey(&llOperationKey);
    msg.carReport.rpAlram.CarStatus.OperationKey = llOperationKey;
    msg.carReport.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.carReport.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    msg.carReport.rpAlram.CarStatus.EventKey = nEvent;

#if defined(PROTOCOL19)
	sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"%X,%s",pstMsgModem->carReport.rpSmartKey.Request.CommandType,pstMsgModem->carReport.rpSmartKey.Request.Guid);
#elif defined(PROTOCOL17)
    memcpy(msg.carReport.rpAlram.CarStatus.EventKeyValue,(char*)&nResult,4);
#endif
    msg.carReport.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    msg.carReport.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
    msg.carReport.rpAlram.CarStatus.GpsSetLatitude = 0;
    msg.carReport.rpAlram.CarStatus.GpsSetLongitude = 0;
    msg.carReport.rpAlram.CarStatus.Distance = 0;
	msg.carReport.rpAlram.CarStatus.Odometer = Get_Odmeter();
    msg.carReport.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;
#ifdef ONLY_SAVE_MODE
	if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
	{
		msg.header.unEventTime = 0;
	}

	Trace("send data to storage\r\n");
	msg.header.event = eReqSaveReport;
	Send2MngStorage2((stMsgStorage*)&msg);
#else
    if( IsItPossibleSendingMessage() == true )
    {
        Trace("send data to modem\r\n");
        msg.header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)&msg);
    }
    else
    {
        if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            msg.header.unEventTime = 0;
        }

        Trace("send data to storage\r\n");
        msg.header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)&msg);
    }
#endif
}

void SendGeoFenceAlramReport(int32_t nEvent, int32_t nResult, stGeofenceUnit* pstCurGeofence, stGeofenceUnit* pstSetGeofence,eGeoFenceType eGeofenceType,uint32_t unGeofenceID)
{
    stMsgSysMsg msg;
    long long llOperationKey;
    memset((int8_t*)&msg,0,sizeof(stMsgSysMsg));

    GetSystemDrivingKey(&llOperationKey);

    msg.header.id = eMngSysMsg;
    msg.header.event = eReqReport;
    msg.header.subEvent = eR_Alram;
    msg.header.drivingKey = llOperationKey;

    msg.carReport.rpAlram.CarStatus.OperationKey = llOperationKey;
    msg.carReport.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.carReport.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    msg.carReport.rpAlram.CarStatus.EventKey = nEvent;

    memset(msg.carReport.rpAlram.CarStatus.EventKeyValue,0, sizeof(msg.carReport.rpAlram.CarStatus.EventKeyValue));

    if( eGeofenceType == eGFT_POLYGON )
    {
        sprintf((char*)(msg.carReport.rpAlram.CarStatus.EventKeyValue),"%d,%04X",nResult,unGeofenceID);
    }
    else
    {
        sprintf((char*)(msg.carReport.rpAlram.CarStatus.EventKeyValue),"%d",nResult);
    }

#ifdef PROTOCOL15
    msg.carReport.rpAlram.CarStatus.Odometer = GetSystemOdometer();
#endif

    msg.carReport.rpAlram.CarStatus.GpsCurLatitude = pstCurGeofence->GpsLatitude;
    msg.carReport.rpAlram.CarStatus.GpsCurLongitude = pstCurGeofence->GpsLongitude;
    msg.carReport.rpAlram.CarStatus.GpsSetLatitude = pstSetGeofence->GpsLatitude;
    msg.carReport.rpAlram.CarStatus.GpsSetLongitude = pstSetGeofence->GpsLongitude;
    msg.carReport.rpAlram.CarStatus.Distance = pstSetGeofence->unDistance;
    msg.carReport.rpAlram.CarStatus.Boundtype = pstCurGeofence->cFenceType;

#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
	if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
	{
		msg.header.unEventTime = 0;
	}

	Trace("send data to storage\r\n");
	msg.header.event = eReqSaveReport;
	Send2MngStorage2((stMsgStorage*)&msg);
#else
    if( IsItPossibleSendingMessage() == true )
    {
        Trace("send data to modem\r\n");
        msg.header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)&msg);
    }
    else
    {
        if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            msg.header.unEventTime = 0;
        }

        Trace("send data to storage\r\n");
        msg.header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)&msg);
    }
#endif	// ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
    Send2MngModem(eMngSysMsg,eReqReport,eR_Alram,&msg,0);
#endif // #ifndef TEST_MODE_ONLY_MODEDM

}


void SendFotaAlramReport(int32_t nEvent, int32_t nResult)
{
    stMsgSysMsg msg;
    long long llOperationKey;
    memset((int8_t*)&msg,0,sizeof(stMsgSysMsg));

    msg.header.id = eMngSysMsg;
    msg.header.event = eReqSaveReport;
    msg.header.subEvent = eR_Alram;
    msg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());

    msg.carReport.rpAlram.CarStatus.EventKey = nEvent;
#if defined(PROTOCOL19)
	unsigned char ucSlaveDBName[7],ucControlDBName[7];

	memset(ucSlaveDBName,0x00,sizeof(ucSlaveDBName));
	memset(ucControlDBName,0x00,sizeof(ucControlDBName));
	memcpy(ucSlaveDBName,g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName,MAX_FW_DB_FILE_NAME);
	memcpy(ucControlDBName,g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName,MAX_FW_DB_FILE_NAME);
	ucSlaveDBName[MAX_FW_DB_FILE_NAME]=0x00;
	ucControlDBName[MAX_FW_DB_FILE_NAME]=0x00;

	if( nResult == eREMOTE_CON_FOTA_TYPE_UPDATE )
    {
        sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"R,%04X,%04X,%04X,%02X.%02X,%02X.%02X,%s,%s,%04X,%04X\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF,
							ucSlaveDBName,ucControlDBName,
							m_stCFDCtrl.stVer.usBlVer,
							m_stCFDCtrl.stVer.usAppVer);
    }
    else if( nResult == eREMOTE_CON_FOTA_TYPE_TEST )
    {
        sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"T,%04X,%04X,%04X,%02X.%02X,%02X.%02X,%s,%s,%04X,%04X\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF,
							ucSlaveDBName,ucControlDBName,
							m_stCFDCtrl.stVer.usBlVer,
							m_stCFDCtrl.stVer.usAppVer);
    }
    else if( nResult == eREMOTE_CON_FOTA_TYPE_BOOT )
    {
        sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"B,%04X,%04X,%04X,%02X.%02X,%02X.%02X,%s,%s,%04X,%04X\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF,
							ucSlaveDBName,ucControlDBName,
							m_stCFDCtrl.stVer.usBlVer,
							m_stCFDCtrl.stVer.usAppVer);
    }
#else	//defined(PROTOCOL19)
#if defined(PROTOCOL16)
	unsigned char ucSlaveDBName[7],ucControlDBName[7];

	memset(ucSlaveDBName,0x00,sizeof(ucSlaveDBName));
	memset(ucControlDBName,0x00,sizeof(ucControlDBName));
	memcpy(ucSlaveDBName,g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName,MAX_FW_DB_FILE_NAME);
	memcpy(ucControlDBName,g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName,MAX_FW_DB_FILE_NAME);
	ucSlaveDBName[MAX_FW_DB_FILE_NAME]=0x00;
	ucControlDBName[MAX_FW_DB_FILE_NAME]=0x00;

	if( nResult == eREMOTE_CON_FOTA_TYPE_UPDATE )
    {
        sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"R,%04X,%04X,%04X,%02X.%02X,%02X.%02X,%s,%s\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF,
							ucSlaveDBName,ucControlDBName);
    }
    else if( nResult == eREMOTE_CON_FOTA_TYPE_TEST )
    {
        sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"T,%04X,%04X,%04X,%02X.%02X,%02X.%02X,%s,%s\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF,
							ucSlaveDBName,ucControlDBName);
    }
    else if( nResult == eREMOTE_CON_FOTA_TYPE_BOOT )
    {
        sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"B,%04X,%04X,%04X,%02X.%02X,%02X.%02X,%s,%s\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF,
							ucSlaveDBName,ucControlDBName);
    }
#else	//defined(PROTOCOL16)
    if( nResult == eREMOTE_CON_FOTA_TYPE_UPDATE )
    {
        sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"R,%04X,%04X,%04X,%02X.%02X,%02X.%02X\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF);
    }
    else if( nResult == eREMOTE_CON_FOTA_TYPE_TEST )
    {
        sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"T,%04X,%04X,%04X,%02X.%02X,%02X.%02X\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF);
    }
    else if( nResult == eREMOTE_CON_FOTA_TYPE_BOOT )
    {
        sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"B,%04X,%04X,%04X,%02X.%02X,%02X.%02X\x00",
                            g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion,
                            g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion,
                            (g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion&0xFF,
                            (g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion>>8)&0xFF,
                            g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion&0xFF);
    }
#endif	//defined(PROTOCOL16)
#endif	//defined(PROTOCOL19)

#ifdef PROTOCOL15
    msg.carReport.rpAlram.CarStatus.Odometer = GetSystemOdometer();
#endif

    GetSystemDrivingKey(&llOperationKey);
    msg.carReport.rpAlram.CarStatus.OperationKey = llOperationKey;
    msg.carReport.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.carReport.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    msg.carReport.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    msg.carReport.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
    msg.carReport.rpAlram.CarStatus.GpsSetLatitude = 0;
    msg.carReport.rpAlram.CarStatus.GpsSetLongitude = 0 ;
    msg.carReport.rpAlram.CarStatus.Distance = 0;
    msg.carReport.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;

    Trace("send data to storage\r\n");
    msg.header.event = eReqSaveReport;

    if( GetBlockMsgTrasfer() == true )
    {
        Trace("#################################################\r\n");
        Trace("System block to transfer\r\n");
        Trace("#################################################\r\n");

        return;
    }

    Send2MngStorage2((stMsgStorage*)&msg);
}

void ProcessReport(stMsgSysMsg* pstMsgSysMsg)
{
    pstMsgSysMsg->header.id = eMngSysMsg;
    pstMsgSysMsg->header.unTraceMng = eMngSysMsg;
    pstMsgSysMsg->header.unEventTime = GetLocalTimefromTime(GetUTCTime());

#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
	if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
	{
		pstMsgSysMsg->header.unEventTime = 0;
	}

	Trace("send data to storage_2\r\n");
	pstMsgSysMsg->header.event = eReqSaveReport;
	Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
#else
    if( IsItPossibleSendingMessage() == true )
    {
        if( m_stMsgHdData.bSkipMessage == true )
        {
            Trace("Skip Process All event is skipped.\r\n");
            return ;
        }

        Trace("send data to modem_2\r\n");
        pstMsgSysMsg->header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
    }
    else
    {
        if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            pstMsgSysMsg->header.unEventTime = 0;
        }

        Trace("send data to storage_2\r\n");
        pstMsgSysMsg->header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
    }
#endif	//ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
    Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
#endif // #ifndef TEST_MODE_ONLY_MODEDM

}

void ProcessAlram(stMsgSysMsg* pstMsgSysMsg)
{
    pstMsgSysMsg->header.id = eMngSysMsg;
    pstMsgSysMsg->header.unTraceMng = eMngSysMsg;
    // time will be set in obd manager
    pstMsgSysMsg->header.unEventTime = GetLocalTimefromTime(GetUTCTime());
    //GetLocalTimeforDate(&pstMsgSysMsg->header.stDate);

    GetSystemDrivingKey((long long *)&pstMsgSysMsg->header.drivingKey);
    pstMsgSysMsg->carReport.rpAlram.CarStatus.OperationKey = pstMsgSysMsg->header.drivingKey;

#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
	if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
	{
		pstMsgSysMsg->header.unEventTime = 0;
	}

	Trace("send data to storage\r\n");
	pstMsgSysMsg->header.event = eReqSaveReport;
	Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
#else
    if( IsItPossibleSendingMessage() == true )
    {
        if( m_stMsgHdData.bSkipMessage == true )
        {
            Trace("Skip Process All event is skipped.\r\n");
            return ;
        }

        Trace("send data to modem\r\n");
        pstMsgSysMsg->header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
    }
    else
    {
        if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            pstMsgSysMsg->header.unEventTime = 0;
        }

        Trace("send data to storage\r\n");
        pstMsgSysMsg->header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
    }
#endif	//ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
    Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
#endif // #ifndef TEST_MODE_ONLY_MODEDM

}

#if defined(PROTOCOL24)
void ProcessReportModemStatus(stMsgSysMsg* pstMsgSysMsg)
{
	pstMsgSysMsg->header.id = eMngSysMsg;
	pstMsgSysMsg->header.event = eReqReport;
	pstMsgSysMsg->header.subEvent = eR_ReportNetworkStatus;
	Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
}
#endif

#if defined(PROTOCOL25)
void ProcessReportInstallationStatus(stMsgSysMsg* pstMsgSysMsg)
{
	pstMsgSysMsg->header.id = eMngSysMsg;
	pstMsgSysMsg->header.event = eReqReport;
	pstMsgSysMsg->header.subEvent = eR_ReportInstallationNetworkCheck;
	Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
}
#endif

void ProcessAlramDtc(stMsgSysMsg* pstMsgSysMsg)
{
    pstMsgSysMsg->header.id = eMngSysMsg;
    pstMsgSysMsg->header.unTraceMng = eMngSysMsg;
    // time will be set in obd manager
    //pstMsgSysMsg->header.unEventTime = GetLocalTime();
    //GetLocalTimeforDate(&pstMsgSysMsg->header.stDate);

    GetSystemDrivingKey((long long *)&pstMsgSysMsg->header.drivingKey);
    pstMsgSysMsg->carReport.rpAlram.Dtc.OperationKey = pstMsgSysMsg->header.drivingKey;

#ifndef TEST_MODE_ONLY_MODEDM
#ifdef ONLY_SAVE_MODE
	if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
	{
		pstMsgSysMsg->header.unEventTime = 0;
	}

	Trace("send data to storage\r\n");
	pstMsgSysMsg->header.event = eReqSaveReport;
	Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
#else
    if( IsItPossibleSendingMessage() == true )
    {
        Trace("send data to modem\r\n");
        pstMsgSysMsg->header.event = eReqReport;
        Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
    }
    else
    {
        if( m_stMsgHdData.bReqWaitSendingforNetworkTime == true )
        {
            pstMsgSysMsg->header.unEventTime = 0;
        }

        Trace("send data to storage\r\n");
        pstMsgSysMsg->header.event = eReqSaveReport;
        Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
    }
#endif	// ONLY_SAVE_MODE
#else  // #ifndef TEST_MODE_ONLY_MODEDM
    Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
#endif // #ifndef TEST_MODE_ONLY_MODEDM
}

// we save this message for guid and command
stMsgSysMsg m_stLastFotaRequestMessage;

void SetLastFotaRequestMessage(stMsgSysMsg* pstMsgSysMsg)
{
    memcpy((char*)&m_stLastFotaRequestMessage,(char*)pstMsgSysMsg,sizeof(stMsgSysMsg));
}

void GetLastFotaReqeustMessage(stMsgSysMsg* pstMsgSysMsg)
{
    memcpy((char*)pstMsgSysMsg,(char*)&m_stLastFotaRequestMessage,sizeof(stMsgSysMsg));
}

void ProcessReqFota(stMsgSysMsg* pstMsgSysMsg)
{
    uint8_t carrGuid[MAX_GUID_LENGTH]={0,};
    uint32_t unSystemTimeout;
    GetBackupRamConfigProperty(eBackupRamConfig_SystemTimeout,(void*)&unSystemTimeout);

    if( pstMsgSysMsg->header.subEvent == eFwUpdate )
    {
    // compare guid because when fota scenario event is occurred, then guid is 0,
    // and then we don't need to respose for smartkey
    if( memcmp(pstMsgSysMsg->carReport.rpSmartKey.Request.Guid,carrGuid,MAX_GUID_LENGTH) != 0 )
    {
        // response for the reqeust and then request vehicle information
        ResponseSysSmartkey(pstMsgSysMsg,eREMOTE_CON_RESULT_SUCCESS,0);
    }

    // request to get fw inforamtion
    Send2MngModem(eMngSysMsg,eReqFota,eFwVehicleInfo,(stCarReport *)NULL,0);
    m_bPostponeReqSleep = true;

    // save last remote request message for guid and control type.
    // beause if server request fota there is two option, test, update
    SetLastFotaRequestMessage(pstMsgSysMsg);

    EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout, eSWTimer_INFINITE, TimeoutTimerCallBack);

    // Set obd status to firmware update

    // Send suspended event
    Send2MngStorage(eMngSysMsg,eReqSuspend, eSuspendStart, (stCarReport *)NULL , 0);
    //Send2MngObd(eMngSysMsg,eReqFota, eTrue, (stCarReport *)NULL , 0);
    Send2MngSensor(eMngSysMsg,eReqSuspend, eSuspendStart, (stCarReport *)NULL , 0);
    // change obd status for update
    Send2MngObd(eMngModem,eOBDSetting, eUpdateMode,(stCarReport *)NULL,0);
    //SetOBDState(eOBD_Selftest_FW_Update_Mode);

        BkSram_SystemInfo.bTestFota = false;
    }
    else if( pstMsgSysMsg->header.subEvent == eFwTestUpdate )
    {
        // compare guid because when fota scenario event is occurred, then guid is 0,
        // and then we don't need to respose for smartkey
        if( memcmp(pstMsgSysMsg->carReport.rpSmartKey.Request.Guid,carrGuid,MAX_GUID_LENGTH) != 0 )
        {
            // response for the reqeust and then request vehicle information
            ResponseSysSmartkey(pstMsgSysMsg,eREMOTE_CON_RESULT_SUCCESS,0);
        }

        // request to get fw inforamtion
        Send2MngModem(eMngSysMsg,eReqFota,eFwVehicleInfo,(stCarReport *)NULL,0);
        m_bPostponeReqSleep = true;

        // save last remote request message for guid and control type.
        // beause if server request fota there is two option, test, update
        SetLastFotaRequestMessage(pstMsgSysMsg);

        EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout, eSWTimer_INFINITE, TimeoutTimerCallBack);

        // Set obd status to firmware update

        // Send suspended event
        Send2MngStorage(eMngSysMsg,eReqSuspend, eSuspendStart, (stCarReport *)NULL , 0);
        //Send2MngObd(eMngSysMsg,eReqFota, eTrue, (stCarReport *)NULL , 0);
        Send2MngSensor(eMngSysMsg,eReqSuspend, eSuspendStart, (stCarReport *)NULL , 0);
        // change obd status for update
        Send2MngObd(eMngModem,eOBDSetting, eUpdateMode,(stCarReport *)NULL,0);
        //SetOBDState(eOBD_Selftest_FW_Update_Mode);

        BkSram_SystemInfo.bTestFota = true;

        // clear f/w db version/
        // MONI 2018.10.01
        // block this code, if recevied test fota,
        // fota handler didn't compare version, just donwload new files.
        // ClearAppVersion();
    }
}

void SendFotaResult()
{
    stMsgSysMsg stLastFotaRequestMessage;
    GetLastFotaReqeustMessage(&stLastFotaRequestMessage);

    // response normal update within alram
    if( stLastFotaRequestMessage.carReport.rpSmartKey.Request.ControlType == eREMOTE_CON_FOTA_TYPE_UPDATE )
    {
        // success report for fota
        SendFotaAlramReport(eMESSAGE_EVENT_KEY_FOTA_COMPLETE_ALRAM,eREMOTE_CON_FOTA_TYPE_UPDATE);// complete
    }
    // response force update within smartkey
    else if( stLastFotaRequestMessage.carReport.rpSmartKey.Request.ControlType == eREMOTE_CON_FOTA_TYPE_TEST )
    {
        // success report for force fota
        SendFotaAlramReport(eMESSAGE_EVENT_KEY_FOTA_COMPLETE_ALRAM,eREMOTE_CON_FOTA_TYPE_TEST);// complete
    }
}

void ProcessRspFota(stMsgSysMsg* pstMsgSysMsg)
{
    uint32_t unSystemTimeout;
    GetBackupRamConfigProperty(eBackupRamConfig_SystemTimeout,(void*)&unSystemTimeout);

    if( pstMsgSysMsg->header.subEvent == eFwVehicleInfo )
    {
        if( pstMsgSysMsg->header.result == eSuccess )
        {
            // request to get fw inforamtion
            Send2MngModem(eMngSysMsg,eReqFota,eFwInfo,(stCarReport *)NULL,0);
            m_bPostponeReqSleep = true;
        }
        else
        {
            // release this flag to send the data to server
            m_bPostponeReqSleep = false;
            // change obd status to initialize to go sleep
            Send2MngObd(eMngModem,eOBDSetting, eInitailize,(stCarReport *)NULL,0);
            //SetOBDState(eOBD_Initialize);

            //MONI 20190108 added fail result of FOTA
            SendFotaResult();

            // set device reset event to system.
            Send2MngSys(eMngSysMsg,eReqSystemReset,0,(stCarReport *)NULL,0);
        }

        EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout, eSWTimer_INFINITE, TimeoutTimerCallBack);
    }
    else if( pstMsgSysMsg->header.subEvent == eFwInfo )
    {
        if( pstMsgSysMsg->header.result == eSuccess )
        {
            // success report for force fota
            // we don't need to send this message
            // SendAlramReport(eMESSAGE_EVENT_FOTA_COMPLETE,1);// in progress

            // request to download fw binary
            Send2MngModem(eMngSysMsg,eReqFota,eFwBin,(stCarReport *)NULL,0);
            m_bPostponeReqSleep = true;
        }
        else
        {
            // release this flag to send the data to server
            m_bPostponeReqSleep = false;
            //SetOBDState(eOBD_Initialize);
            // change obd status to initialize to go sleep
            Send2MngObd(eMngModem,eOBDSetting, eInitailize,(stCarReport *)NULL,0);

            //MONI 20190108 added fail result of FOTA
            SendFotaResult();

            // set device reset event to system.
            Send2MngSys(eMngSysMsg,eReqSystemReset,0,(stCarReport *)NULL,0);
        }

        EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout, eSWTimer_INFINITE, TimeoutTimerCallBack);
    }
    else if(pstMsgSysMsg->header.subEvent == eFwBin )
    {
        if( pstMsgSysMsg->header.result == eSuccess )
        {
        }
        else
        {
            // not define event in the server
            //if( m_bForceTota == true )
            //{
            //    // fail report for force fota
            //    // success report for force fota
            //    SendAlramReport(eMESSAGE_EVENT_FOTA_COMPLETE,1);// complete
            //}
        }

		// if me do fota process then format the serial flash
		TestFormat();

        //MONI 20190108 added fail result of FOTA
        SendFotaResult();

        EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout*2, eSWTimer_INFINITE, TimeoutTimerCallBack);

        m_bPostponeReqSleep = false;

        // Disable Powerofftime for stopping fota update
        pstMsgSysMsg->header.id = eMngSysMsg;
        //memset((char*)&pstMsgSysMsg->header.stDate,0,sizeof(stHalRTCTypeDef));
        pstMsgSysMsg->header.unEventTime = 0;
        // set power off time for fota update
        SetRealPowerOffTime(pstMsgSysMsg->header.unEventTime);

        // change obd status to initialize to go sleep
        Send2MngObd(eMngModem,eOBDSetting, eInitailize,(stCarReport *)NULL,0);

#ifdef ENABLE_FOTA_DOWNLOAD_TEST
        m_stMsgHdData.ucIGOffCount = 0;
        m_stMsgHdData.bIGOff = false;

        // clear f/w db version/
        ClearAppVersion();

        Send2MngSysMsg2(eMngModem,eReqFota,eFwUpdate, eTrue, (stCarReport *)NULL,0);
#else
        // set device reset event to system.
        Send2MngSys(eMngSysMsg,eReqSystemReset,0,(stCarReport *)NULL,0);
#endif
    }
    else
    {
        GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
    }
}

int32_t EHL_MngModem(stMsgSysMsg* pstMsgSysMsg)
{
    // below events for modem
    if( pstMsgSysMsg->header.event == eRspSleep )
    {
        // set for each sleep bit
        m_stMsgHdData.uiSubSysSleepEvent |= SYS_MNG_HD_SYS_MODEM_SLEEP_OK;
    }
    else if( pstMsgSysMsg->header.event == eMdmStatus )
    {
        ProcessModemStatus(pstMsgSysMsg);
    }
    else if( pstMsgSysMsg->header.event == eMdmRssi )
    {
        //m_iModemRSSI =
    }
    else if( pstMsgSysMsg->header.event == eReqPostpond )
    {
        if( pstMsgSysMsg->header.subEvent == true )
            m_stMsgHdData.bReqPostPondforSending = true;
        else
            m_stMsgHdData.bReqPostPondforSending = false;
    }
    // below events for message
    else if( pstMsgSysMsg->header.event == eReqReport )
    {
        if( pstMsgSysMsg->header.subEvent == eR_ReqSmartKey )
        {
#ifdef USE_96_HOURS_POWER_OFF
            // clear all reset count because user use the car
            ClearSystemResetCount();
#endif //#ifdef USE_96_HOURS_POWER_OFF

            // MONI 2018-03-14
            // in driving mode, ME block a request of remote control
            if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_DRIVING)
            {
                // send reject response for remote control in driving mode.
                Trace("==================================================\r\n");
                Trace("Reject a smartkey request because of driving received\r\n");
                // ResponseSysSmartkey(pstMsgSysMsg,eREMOTE_CON_RESULT_FAIL,0x4000);
                // return 0;
            }

            ProcessReqSmartkey(pstMsgSysMsg);
        }
        else if( pstMsgSysMsg->header.subEvent == eR_ReqUserActionSetting )
        {
            ProcessReqUserActionSetting(pstMsgSysMsg);
        }
        else if( pstMsgSysMsg->header.subEvent == eR_CurrentVehicleStatus )
        {
            // MONI 2018-03-15
            // if modem activation is false
            // we should block all message to send the modem manager.
            if( GetBlockMsgTrasfer() == true )
            {
                Trace("#################################################\r\n");
                Trace("System block to transfer\r\n");
                Trace("#################################################\r\n");

                return 0;
            }

            Send2MngObd(eMngSysMsg, eReqReport, eR_CurrentVehicleStatus ,&pstMsgSysMsg->carReport, pstMsgSysMsg->header.unTraceMng);
        }
#if defined(PROTOCOL24)
		else if( pstMsgSysMsg->header.subEvent == eR_ReportNetworkStatus )
        {
        	ProcessReportModemStatus(pstMsgSysMsg);
        }
#endif
#if defined(PROTOCOL25)
        else if( pstMsgSysMsg->header.subEvent == eR_ReportInstallationNetworkCheck )
        {
            ProcessReportInstallationStatus(pstMsgSysMsg);
        }
#endif

    }
    else if( pstMsgSysMsg->header.event == eRspReport )
    {
        if( pstMsgSysMsg->header.result == eSuccess)
        {
            m_stMsgHdData.bReqPostPondforSending = false;
            Trace("#############################################################\r\n");
            Trace("netwokr is stable / temporary we set the connect flag to connect.\r\n");

            // MONI 2018-03-02
            // we set the flag for the postpone until we get the storage status.
            // if we get the status of storage, then set the flag of postpone with false.
            if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_DRIVING)
            {
                SetPostponeSleepFlag(true);
            }

            Send2MngStorage(eMngSysMsg,eReqDataExist,0,(stCarReport *)NULL,0);
        }
        else //( pstMsgSysMsg->header.result == eFail )
        {
            // retransfer is failed 3times,
            // disconnect network status and wait until network is connected
            m_stMsgHdData.bReqPostPondforSending = true;
            Trace("#############################################################\r\n");
            Trace("netwokr is unstable / temporary we set the connect flag to disconnect.\r\n");
        }

        // MONI 2018-03-13
        // this code is duplicated so blocked
        // request status of stroage.
        //Send2MngStorage(eMngSysMsg,eReqDataExist,0,(stCarReport *)NULL,0);
    }
#ifdef ENABLE_FOTA
    else if( pstMsgSysMsg->header.event == eReqFota )
    {
        if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_PARKING)
        {
            uint8_t bActive = 0;
            GetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bActive);

            if( bActive == true )
            {
                ProcessReqFota(pstMsgSysMsg);
            }
            else
            {
                Trace("##########################################################\r\n");
                Trace("Modem is not activated, so fota won't start.\r\n");
                ResponseSysSmartkey(pstMsgSysMsg,eREMOTE_CON_RESULT_FAIL,0x00400000);

                // MONI 20180716
                // if one time is reject then release this flag for following command.
                bActive = false;
            }
        }
        else if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_DRIVING)
        {
            //reject request fota

            Trace("##########################################################\r\n");
            Trace("##########################################################\r\n");
            Trace("reject fota request because ME is driving mode.\r\n");
            ResponseSysSmartkey(pstMsgSysMsg,eREMOTE_CON_RESULT_FAIL,0x00004000);
        }
    }
    else if( pstMsgSysMsg->header.event == eRspFota )
    {
        ProcessRspFota(pstMsgSysMsg);
    }
#endif
    else if( pstMsgSysMsg->header.event == eReqSleep )
    {

    }
    else if( pstMsgSysMsg->header.event == eReqIpek )
    {
        // HandlerIpek(pstMsgSysMsg);
    }
    else if( pstMsgSysMsg->header.event == eRspIpek )
    {
        HandlerIpek(pstMsgSysMsg);
    }
    else if( pstMsgSysMsg->header.event == eDummy )
    {
        Trace("Rcv System Dummy Event\r\n");
        // reset igoff to make delay
        m_stMsgHdData.bIGOff = false;
        m_stMsgHdData.ucIGOffCount = 0;
    }
    else if( pstMsgSysMsg->header.event == eReqAgps )
    {
        Trace("Rcv AGPS Data eReqAgps\r\n");
        HandlerAgps(pstMsgSysMsg);
    }
    else if( pstMsgSysMsg->header.event == eRspAgps )
    {
        Trace("Rcv AGPS Data eRspAgps\r\n");
        HandlerAgps(pstMsgSysMsg);
    }
#if defined(PROTOCOL17)
	else if( pstMsgSysMsg->header.event == eReqSysSetting )
    {
        Trace("Rcv eReqSysSetting\r\n");
        HandlerSysSetting(pstMsgSysMsg);
    }
#endif
    else
    {
        GIT_Assert(false,eErrorCodeSysHd|eEventUnknown);
    }

    return 0;
}

int32_t EHL_MngStorage(stMsgSysMsg* pstMsgSysMsg)
{
    if( pstMsgSysMsg->header.event == eRspSleep )
    {
        // set for each sleep bit
        m_stMsgHdData.uiSubSysSleepEvent |= SYS_MNG_HD_SYS_STORAGE_SLEEP_OK;
    }
    else if( pstMsgSysMsg->header.event == eRspDataExist )
    {
        // MONI 2018-03-02
        // we set the flag of postpone, we get the status of storage
        if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_DRIVING)
        {
            SetPostponeSleepFlag(false);
        }

        // data
        m_stMsgHdData.bIsStoreData = pstMsgSysMsg->header.subEvent;
#ifdef ONLY_SAVE_MODE
		if( m_stMsgHdData.bIsStoreData == true && IsItPossibleSendingMessage() == true )
        {
            m_bAutoReadFlag = true;
        }
#endif
        Trace("Data Exist : %d\r\n",m_stMsgHdData.bIsStoreData);
    }
    else
    {
        GIT_Assert(false,eErrorCodeSysHd|eEventUnknown);
    }

    return 0;
}

int32_t EHL_MngSensor(stMsgSysMsg* pstMsgSysMsg)
{
    if( pstMsgSysMsg->header.event == eReqReport )
    {
        if( pstMsgSysMsg->header.subEvent == eR_ImpulseAlram )
        {
            int nResult = 1;

            if( GetGuardActive() == true )
                SendAlramReport(eMESSAGE_EVENT_KEY_PARKING_IMPACT,nResult);
        }
        else
        {
            GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
        }
    }
    else
    {
        GIT_Assert(false,eErrorCodeSysHd|eEventUnknown);
    }
    return 0;
}

int32_t EHL_MngObd(stMsgSysMsg* pstMsgSysMsg)
{
    if( pstMsgSysMsg->header.event == eRspSleep )
    {
        // set for each sleep bit
        m_stMsgHdData.uiSubSysSleepEvent |= SYS_MNG_HD_SYS_OBD_SLEEP_OK;
		if( pstMsgSysMsg->header.result == eFail)
		{
			SetRequestSystemReset(true);
		}
    }
    else if( pstMsgSysMsg->header.event == eReqReport)
    {
        if( pstMsgSysMsg->header.subEvent == eR_BeforeDriving )
        {
#ifdef USE_96_HOURS_POWER_OFF
            // clear all reset count because user use the car
            ClearSystemResetCount();
#endif //#ifdef USE_96_HOURS_POWER_OFF

            if( m_bReqSmartkey == false )
            {
                // to get a setting info before driving send this message
                if( GetBlockMsgTrasfer() == false )
                {
                    Send2MngModem3(eMngSysMsg,eReqReport,eR_SettingInfo,0,(stCarReport *)NULL,0);
                }

                ProcessBeforeDriving(pstMsgSysMsg);

//#warning "Stop sensor interrupt during the driving"
                // stop sensor interrupt
                SetupForInterruptforImpulse(false, false, 0xAF, false);
            }
            else
            {
                m_bSaveRspSmartKey = true;
                memcpy((int8_t*)&m_stRspSmartkey,(int8_t*)pstMsgSysMsg,sizeof(stMsgSysMsg));
            }

            // notify to sensor mode change
            Send2MngSensor(eMngSysMsg,eReqChangeMode,eSysModeDriving,(stCarReport *)NULL,0);
        }
        else if( pstMsgSysMsg->header.subEvent == eR_AfterDriving )
        {
            ProcessAfterDriving(pstMsgSysMsg);

            // MONI 2018-03-02
            // we set the flag of postpone, we get the status of storage
            if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_DRIVING)
            {
                SetPostponeSleepFlag(false);
            }

			// clear driving key
			SetSystemDrivingKey((long long)0);

            // check expire apgs data
            if( GetBlockMsgTrasfer() == false )
                CheckExpiretAGPSData();

            // notify to sensor mode change
            Send2MngSensor(eMngSysMsg,eReqChangeMode,eSysModeParking,(stCarReport *)NULL,0);
        }
        else if( pstMsgSysMsg->header.subEvent == eR_Alram )
        {
            ProcessAlram(pstMsgSysMsg);
        }
        else if( pstMsgSysMsg->header.subEvent == eR_AlramDTC )
        {
            ProcessAlramDtc(pstMsgSysMsg);
        }
#if defined(PROTOCOL18)
		else if( pstMsgSysMsg->header.subEvent == eR_Charging)
        {
            ProcessCharging(pstMsgSysMsg);
        }
#endif
        else
        {
            GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
        }
    }
    else if( pstMsgSysMsg->header.event == eRspReport )
    {
        if( pstMsgSysMsg->header.subEvent == eR_RspSmartKey )
        {
            // MONI 2018-03-06
            // this is forwarding process from obd to bt
//            if( GetForwardingSmartKey2BtFlag() == true )
//            {
//               ResponseSmartKey2Bt(pstMsgSysMsg);
//               return 0;
//            }

            // MONI 2018-03-15
            // if modem activation is false
            // we should block all message to send the modem manager.
            if( GetBlockMsgTrasfer() == true )
            {
                Trace("#################################################\r\n");
                Trace("System block to transfer\r\n");
                Trace("#################################################\r\n");

                return 0;
            }

            ProcessRspSmartKey(pstMsgSysMsg);

            // send suspend an event to storage to stop a report
            Send2MngStorage(eMngSysMsg,eReqSuspend,eSuspendStop,(stCarReport *)NULL,0);

            if( m_bReqSmartkey == true )
            {
                if( m_bSaveRspSmartKey == true )
                {
                    Send2MngSysMsg3(&m_stRspSmartkey);
                    m_bSaveRspSmartKey = false;
                }

                m_bReqSmartkey = false;
            }
        }
        else if( pstMsgSysMsg->header.subEvent == eR_DrivingInterval )
        {
#ifndef USE_SEND_ODD_MESSAGE
            Trace("[%s:%d]lat : %f, lon : %f\r\n", __FUNCTION__, __LINE__,
                pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLatitude,
                pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLongitude);

#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE
            if( pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLatitude != 0.0 &&
                pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLongitude != 0.0 )
#endif //#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE
            {

                ProcessDriving(pstMsgSysMsg);

                // save odometer
                SetNewOdometer(pstMsgSysMsg);
            }
#else //#ifndef USE_SEND_ODD_MESSAGE
            Trace("[%s:%d]lat : %f, lon : %f\r\n", __FUNCTION__, __LINE__,
                pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLatitude,
                pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLongitude);

#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE
            if( pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLatitude != 0.0 &&
                pstMsgSysMsg->carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLongitude != 0.0 )
#endif //#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE
            {
                ProcessDrivingOddMessageTest(pstMsgSysMsg);

                // save odometer
                SetNewOdometer(pstMsgSysMsg);
            }
#endif //#ifndef USE_SEND_ODD_MESSAGE
        }
        else if( pstMsgSysMsg->header.subEvent == eR_ParkingInterval )
        {
            Trace("[%s:%d]lat : %f, lon : %f\r\n", __FUNCTION__, __LINE__,
                pstMsgSysMsg->carReport.rpInterval.NoDrivingInfo.GpsLatitude,
                pstMsgSysMsg->carReport.rpInterval.NoDrivingInfo.GpsLongitude);

#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE
            if( pstMsgSysMsg->carReport.rpInterval.NoDrivingInfo.GpsLatitude!= 0.0 &&
                pstMsgSysMsg->carReport.rpInterval.NoDrivingInfo.GpsLongitude != 0.0 )
#endif //#ifdef ENABLE_DROP_GPS_ZERO_MESSAGE
            {
                ProcessNoDriving(pstMsgSysMsg);
            }
        }
        else if( pstMsgSysMsg->header.subEvent == eR_CurrentVehicleStatus )
        {
            // MONI 2018-03-15
            // if modem activation is false
            // we should block all message to send the modem manager.
            if( GetBlockMsgTrasfer() == true )
            {
                Trace("#################################################\r\n");
                Trace("System block to transfer\r\n");
                Trace("#################################################\r\n");

                return 0;
            }

            Send2MngModem(eMngSysMsg,eRspReport,eR_CurrentVehicleStatus,&pstMsgSysMsg->carReport,0);
        }
		else if( pstMsgSysMsg->header.subEvent == eR_RspSettingRsvEngCtrl )
        {
            if( IsItPossibleSendingMessage() == true )
            {
                Trace("send data to modem\r\n");
                pstMsgSysMsg->header.event = eRspReport;
                Send2MngModem2((stMsgMdm*)pstMsgSysMsg);
            }
            else
            {
                Trace("send data to storage\r\n");
				pstMsgSysMsg->header.id = eMngSysMsg;
                pstMsgSysMsg->header.event = eReqSaveReport;
                Send2MngStorage2((stMsgStorage*)pstMsgSysMsg);
            }
        }
        else
        {
            GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
        }
    }
    else if( pstMsgSysMsg->header.event == eOBDStatus )
    {
        if( pstMsgSysMsg->header.subEvent == eIGStatus )
        {
            Trace(" Received IG Off from obd count : %d\r\n",m_stMsgHdData.ucIGOffCount);
            GetQueueState();
            uint32_t unSystemTimeout;
            GetBackupRamConfigProperty(eBackupRamConfig_SystemTimeout,(void*)&unSystemTimeout);
            EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout, eSWTimer_INFINITE, TimeoutTimerCallBack);

            // after report parking info set the this flag for request fota info.
            if( pstMsgSysMsg->header.result == false )
            {
                m_stMsgHdData.bIGOff = true;
                m_stMsgHdData.ucIGOffCount++;

                // clear suspend flag
                m_stMsgHdData.bSkipMessage = false;

                // suspend sending message
                m_stMsgHdData.bSuspend = false;

            }
            else
            {
                m_stMsgHdData.bIGOff = false;
                m_stMsgHdData.ucIGOffCount = 0;
            }
        }
        else
        {
            GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
        }
    }
    else if( pstMsgSysMsg->header.event == eOBDKeepAlive )
    {
        if( pstMsgSysMsg->header.subEvent == eKeepAliveStart )
        {
            Trace("Request OBD KeepAlive Start\r\n");
            m_stMsgHdData.bSkipMessage = true;
            // send suspend an event to storage to stop a report
            Send2MngStorage(eMngSysMsg,eReqSuspend,eSuspendStart,(stCarReport *)NULL,0);
        }
        else if( pstMsgSysMsg->header.subEvent == eKeepAliveStop )
        {
            Trace("Request OBD KeepAlive Stop\r\n");
            m_stMsgHdData.bSkipMessage = false;
            // send suspend an event to storage to stop a report
            Send2MngStorage(eMngSysMsg,eReqSuspend,eSuspendStop,(stCarReport *)NULL,0);
        }
    }
    else
    {
        // not defined
        GIT_Assert(false,eErrorCodeSysHd|eEventUnknown);
    }

    return 0;
}

int32_t EHL_MngSysFota(stMsgSysMsg* pstMsgSysMsg)
{
    if( pstMsgSysMsg->header.event == eRspSleep )
    {
        // start driving interval
    }
    return 0;
}

void SetBlockMsgTrasfer(boolean_t bStop)
{
    m_bBlockMsgTransfer = bStop;
}

boolean_t GetBlockMsgTrasfer()
{
    return m_bBlockMsgTransfer;
}

int32_t EHL_MngSys(stMsgSysMsg* pstMsgSysMsg)
{
    if( pstMsgSysMsg->header.event == eReqFota )
    {
        if( pstMsgSysMsg->header.subEvent == eFwUpdate )
        {
            if( pstMsgSysMsg->header.result == eTrue )
            {
                Trace("#############################################################\r\n");
                Trace("#############################################################\r\n");
                Trace("Fota will be start before going to sleep\r\n");
                Trace("#############################################################\r\n");
                // set flag for fota update
                m_bNeedUpdate = true;
            }
            else
            {
                m_bNeedUpdate = false;
            }

            Trace(" received update request from sysmet manager status : %d\r\n",m_bNeedUpdate);
        }
        else
        {
            GIT_Assert(false,eErrorCodeSysHd|eSubEventUnknown);
        }

    }
    else if( pstMsgSysMsg->header.event == eReqMsgBlockTrasfer )
    {
        Trace("Message Manager get an block event\r\n");

        if( pstMsgSysMsg->header.subEvent == eBlockStart )
        {
            Trace("System send an event to block to transfer\r\n");
            SetBlockMsgTrasfer(true);
        }
        else
        {
            Trace("System send an event to transfer\r\n");
            SetBlockMsgTrasfer(false);
        }
    }
    else if( pstMsgSysMsg->header.event == eReqIpek )
    {
        Trace("System request Ipek to server\r\n");
        HandlerIpek(pstMsgSysMsg);
    }
    else if( pstMsgSysMsg->header.event == eReqModemPowerOff )
    {
        if( pstMsgSysMsg->header.result == eTrue )
        {
//#warning "why make two event?"
            Trace("Notify modem power off\r\n");
            SendAlramReport(eMESSAGE_EVENT_KEY_MODEM_POWER_OFF,1);

            Send2MngModem3(eMngSysMsg,eReqModemPowerOff,0,eTrue,(stCarReport *)NULL,0);
        }
        else if ( pstMsgSysMsg->header.result == eFalse )
        {
            Trace("Notify modem power on\r\n");
            SendAlramReport(eMESSAGE_EVENT_KEY_MODEM_POWER_OFF,0);
        }
    }
    else if( pstMsgSysMsg->header.event == eReqAgps )
    {
        Trace("Rcv AGPS Data\r\n");
        HandlerAgps(pstMsgSysMsg);
    }
    else
    {
        GIT_Assert(false,eEventUnknown);
    }

    return 0;
}

int32_t EHL_MngSysMsg(stMsgSysMsg* pstMsgSysMsg)
{
	uint8_t ucArrGuid[MAX_GUID_LENGTH] = {0,};

	if( pstMsgSysMsg->header.event == eReqRsvEngCtrlOn)
	{
		// check system received remote control alraedy
		if( (m_stMsgHdData.bAlreadyRcvSmartkey == false) && (GetForwardingSmartKey2BtFlag() == false) )
		{
			stMsgObd stMsgObd;
			memset(&stMsgObd, 0x00, sizeof(stMsgObd));

			uint8_t ucCtrlIdx = pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.ucWakeupIndexFlag;

			stMsgObd.header.id = eMngSysMsg;
			stMsgObd.header.event = eReqReport;
			stMsgObd.header.subEvent = eR_ReqSmartKey;

			stMsgObd.carReport.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_STARTING;
			stMsgObd.carReport.rpSmartKey.Request.ControlType = 1;
			stMsgObd.carReport.rpSmartKey.Request.ucSysSmartkeyReqType = eSysSmartkeyReqType_Sys;

			stMsgObd.carReport.rpSmartKey.Request.KeepPowerOnTime =
			pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.stRsvEngCtrlInfo[ucCtrlIdx].ucDurationTime;

			stMsgObd.carReport.rpSmartKey.Request.Temperature =
			pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.stRsvEngCtrlInfo[ucCtrlIdx].usTemperature;

			stMsgObd.carReport.rpSmartKey.Request.CheckTemperature =
			pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.stRsvEngCtrlInfo[ucCtrlIdx].ucCheckTemperature;

			stMsgObd.carReport.rpSmartKey.Request.RearDefogger =
			pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.stRsvEngCtrlInfo[ucCtrlIdx].bRearDefog;

			stMsgObd.carReport.rpSmartKey.Request.HighBeam =
			pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.stRsvEngCtrlInfo[ucCtrlIdx].bHeadLamp;

			stMsgObd.carReport.rpSmartKey.Request.Defrost =
			pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.stRsvEngCtrlInfo[ucCtrlIdx].bDeperost;

			memcpy(stMsgObd.carReport.rpSmartKey.Request.Guid,
			pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.stRsvEngCtrlInfo[ucCtrlIdx].ucGUID,
			MAX_GUID_LENGTH);

			Send2MngObd2(&stMsgObd);

			// set remote control
			m_stMsgHdData.bAlreadyRcvSmartkey = true;
		}
		else
		{
			Trace("==================================================\r\n");
            Trace("==================================================\r\n");
            Trace("Reject a smartkey request, already received\r\n");
            Trace("==================================================\r\n");
			stMsgObd stMsgObd;
			memset(&stMsgObd, 0x00, sizeof(stMsgObd));

			uint8_t ucCtrlIdx = pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.ucWakeupIndexFlag;

			// copy guid temprary
			memcpy(ucArrGuid,
			pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.stRsvEngCtrlInfo[ucCtrlIdx].ucGUID,
			MAX_GUID_LENGTH);

			// set guid in response report message.
			memcpy(pstMsgSysMsg->carReport.rpSmartKey.Request.Guid,ucArrGuid,MAX_GUID_LENGTH);

            pstMsgSysMsg->carReport.rpSmartKey.Request.ucSysSmartkeyReqType = eSysSmartkeyReqType_Sys;
            ResponseSysSmartkey(pstMsgSysMsg,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_ALREADY_RUNNING);
		}
	}
	else if( pstMsgSysMsg->header.event == eReqReport )
	{
		if( pstMsgSysMsg->header.subEvent == eR_TrackingReport )
		{
		    ProcessReport(pstMsgSysMsg);
		}
	}

	return 0;
}

int32_t EHL_MngNone(stMsgSysMsg* pstMsgSysMsg)
{
    GIT_Assert(false,eErrorCodeSysHd|eUnknownId);
    return 0;
}

int32_t EHL_MngSysSub(stMsgSysMsg* pstMsgSysMsg)
{
    GIT_Assert(false,eErrorCodeSysHd|eUnknownId);
    return 0;
}

int32_t EHL_MngSysSensor(stMsgSysMsg* pstMsgSysMsg)
{
    GIT_Assert(false,eErrorCodeSysHd|eUnknownId);
    return 0;
}

#if defined(FEATURE_EXTENSION_BOARD)
int32_t EHL_MngExtend(stMsgSysMsg* pstMsgSysMsg)
{
    stMsgSys* pMsg = (stMsgSys*)pstMsgSysMsg;
    Trace("EHL_MngExtend\r\n");

printf("EHL_MngExtend header->id %d, pMsg->header.event 0x06%X ~~~~~~~~~~~~!!!!!!!!!!!!!!!!!!~~~~~~~~~~~~~~~\r\n",
            pMsg->header.id,pMsg->header.event);

    if( pMsg->header.event == eRspSleep )
    {
        // set for each sleep bit
        m_stMsgHdData.uiSubSysSleepEvent |= SYS_MNG_HD_SYS_EXTEND_BOARD_SLEEP_OK;
    }
    else if( pMsg->header.event == eRspReport )
    {
        if ( pMsg->header.subEvent == eR_ReqFOBStatus )
        {
            Send2MngModem2((stMsgMdm*)pMsg);
        }
    }

/*    if( pMsg->header.event ==  eSYS_ReqSleep )
    {

        Send2SysExtend(eEvtSysPsSystem, eSYS_RspSleep,0, true, NULL, eEvtSysPsSystem);
    }
    else

    if( pMsg->header.event ==  eSYS_RspSleep )
    {
		Trace("System Extension Response Time : %d\n",Get_Tmr());
        printf("pMsg->header.event ==  eSYS_RspSleep  ~~~~~~~~~~~~~~~\r\n");
        //SetSleepBit(SLEEP_BIT_ExtBD);
    }
    */

    return 0;
}

#endif
