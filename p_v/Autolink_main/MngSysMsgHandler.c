/* Includes ------------------------------------------------------------------*/
#include "AutolinkConfiguration.h"
#include "GIT_Util.h"
#include "Message_Manager.h"
#include "MngSystem.h"
#include "MngQueue.h"
#include "HdDebug.h"
#include "OBD_Manager.h"
#include "GIT_BluetoothLowEnergy.h"
#include "Power_Manager.h"
#include <time.h>
#include "HandlerRsvEngCtrl.h"
#include "Modem_Manager.h"

#if defined(FEATURE_EXTENSION_BOARD)
#include "SysPsExtendHdEvent.h"
#endif

#ifdef RF_COMMON_MODEM
#include "Modem_Manager.h"
#endif

#define ENABLE_FOTA
// this is guard mode but we don't need to check active time
// because if Driver set this mode, guard mode wil not terminate before driver turn off
#define ENABLE_GUARD_MODE

// this define block because when ME use ublox,
// ME enalbed PTV message and ME use utc tiem that is in the PVT"
// BUT some times this utc time is not correct so ME block this define.
#define ENABLE_ADJUST_TIME

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM_HADNLER,__VA_ARGS__)

typedef int (*fnpSysMsgEventHanderList)(stMsgSysMsg* pstMsgSysMsg);

#define NETUTCTIMESET    1
#define GPSUTCTIMESET    2
#define SERVERUTCTIMESET 3

// external function definitions
extern int EHL_MngNone(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngSys(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngSysSub(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngSysMsg(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngSysSensor(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngSysFota(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngModem(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngSensor(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngStorage(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngObd(stMsgSysMsg* pstMsgSysMsg);
extern int EHL_MngBt(stMsgSysMsg* pstMsgSysMsg);


extern void HdMsgEvtFota(stMsgSysMsg* pstReport);
extern void HdMsgEvtMessage(stMsgSysMsg* pstReport);
extern void HdMsgEvtModem(stMsgSysMsg* pstReport);
extern void HdMsgEvtStorage(stMsgSysMsg* pstReport);
extern void ClearReportSequenceNumber();
extern uint16_t GetReportSequenceNumber();
extern uint16_t key_display(long long llValue);
extern int8_t *GetStringFromAlramEvent(int32_t nId);
extern int8_t *GetStringFromId(int32_t nId);
extern boolean_t GetGeofenceActive();
extern void SendGeoFenceAlramReport(int32_t nEvent, int32_t nResult, stGeofenceUnit* stCurGeofence, stGeofenceUnit* stSetGeofence,eGeoFenceType eGeofenceType,uint32_t unGeofenceID);
extern boolean_t GetValetActive();
extern int32_t CheckValet(stGeofenceUnit* pstCurrentUnit, stGeofenceUnit* pstSettingUnit);
extern unsigned int GetTimefromDate(stHalRTCTypeDef stRtcInfo);
extern bool GetGpsUtcTime(uint32_t* punUtcTime);
extern uint32_t ConvertUtc2LocalTime(uint32_t unUtcTime);
extern stUserActionSetting m_stUserActionSetting;

extern boolean_t GetTrackingActive();//mod.kks to 21.11.05 modify the warnning.
extern void CheckTracking();//mod.kks to 21.11.05 modify the warnning.
extern eVEHICLE_STATE Get_VehicleStatus(void);//mod.kks to 21.11.05 modify the warnning.

extern uint32_t HandlerUblox();
extern void HandlerUbloxProtocol(char* arrNmeaData, int uiReceivedPacket);
extern boolean_t GetReceivedNetworkTime();
extern boolean_t GetBlockMsgTrasfer();
extern void CheckExpiretAGPSData();
extern bool MPU6515_CheckInterrupt();
extern void GetUTCTimeforDate(stHalRTCTypeDef* pstDate);
// internal function defninitions
void HandleMngMsgExternalEvent(stMsgSysMsg* pstMsgSysMsg);
void SysMsgEvtHandler(int iLparam, int iRparam);
void SetMngSysMsgHdState(int iState);
void SetModeParkingState(int state);
void SetPostponeSleepFlag(boolean_t bPostpone);
int HdMngSysMsg(int iLparam,int iRparam);
int HdMngSysMsgInit();

void DisableSystemMessageTimer(int iId);
void EnableSystemMessageTimer(int32_t* iId,int iValue, int iMode, fnSWCallBack callback);

void InitizlieSystemMessage();

void GetSystemDrivingKey(long long * value);
boolean_t SetSystemDrivingKey(long long value);
bool CheckIdleQueue();
void ProcessGeoFence();

// timer call back function list
void TimeoutTimerCallBack(void);
void TimeoutNoDrivingTimerCallBack();
void IntervalTimerCallBack(void);
void HandlerSystemScenario();

extern int8_t *GetStringFromEvent(int32_t nMode,int32_t nEvent,int32_t nSubEvent);
extern void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern uint32_t GetUTCTime();
extern bool SystemDelayProcess(unsigned long* nBaseTime, int nDelay);

boolean_t CheckGuard();
boolean_t GetGuardActive();

uint32_t m_unNetworkUtcTime = 0;
boolean_t m_bNetworkUtcTime = false;

void SetNetworkUtcTime(stHalRTCTypeDef stDate);
boolean_t GetNetworkUtcTime(uint32_t* punNetworkUtcTime);

boolean_t IsItNeededAdjustUTCTime(uint32_t unUtcTime, uint8_t ucTimeFrom);
void AdjustNewTime(uint32_t unUtcTime, uint8_t ucTimeFrom);
void ProcessAdjustTime();

extern stMngSysData m_MngSysData;
extern stAutolinkConfigData m_stAutolinkConfigData;
extern uint8_t g_bEnableModemDirectCommunication;
extern boolean_t m_bSentLastMessage;
extern boolean_t m_bSendReport;
extern boolean_t m_bSuspendStorage;
extern boolean_t m_bSendReport;
extern bool m_bReqNetworkPostPone;
extern boolean_t m_bSentLastMessage;


// main control data of message handler
stMsgHandlerData m_stMsgHdData;

// just used first boot.
bool m_bSysMsgFirst;

#ifdef ENABLE_FOTA
bool m_bPostponeReqSleep;
bool m_bNeedUpdate;
#endif

#if defined(FEATURE_EXTENSION_BOARD)
fnpSysMsgEventHanderList m_pfnMngSysMsgEventList[eMngMax]
    = {EHL_MngNone,EHL_MngSys,EHL_MngSysSub,EHL_MngSysMsg,EHL_MngSysSensor,
    EHL_MngSysFota,EHL_MngModem,EHL_MngSensor,EHL_MngStorage,EHL_MngObd,EHL_MngBt, EHL_MngExtend};
#else
fnpSysMsgEventHanderList m_pfnMngSysMsgEventList[eMngMax]
    = {EHL_MngNone,EHL_MngSys,EHL_MngSysSub,EHL_MngSysMsg,EHL_MngSysSensor,
    EHL_MngSysFota,EHL_MngModem,EHL_MngSensor,EHL_MngStorage,EHL_MngObd,EHL_MngBt};
#endif
void ClearSystemMessageUserData()
{
	memset((char*)&m_stMsgHdData.stObdSettingValue,0,sizeof(m_stMsgHdData.stObdSettingValue));
}

int HdMngSysMsgInit()
{
#ifdef ENABLE_FOTA
    m_bPostponeReqSleep = false;
#endif

    m_bSysMsgFirst = true;
    m_bNeedUpdate = false;

    m_stMsgHdData.bIGOff = false;
    m_stMsgHdData.iHdMngMsgState = STAT_SYS_MNG_HD_MSG_INIT;
    m_stMsgHdData.bIsStoreData = false;
    m_stMsgHdData.bServerConnected = false;
    m_stMsgHdData.bReqWaitSendingforNetworkTime = false;
    m_stMsgHdData.uiSubSysSleepEvent = 0;
    m_stMsgHdData.iTimeoutNoDrivingReportTimerID = -1;
    m_stMsgHdData.iIntervalTimerID = -1;
    m_stMsgHdData.iTimeoutTimerID = -1;
    //MONI 2018-02-12
    //memset((char*)&m_stMsgHdData.stNetworkTime,0,sizeof(stRTCInfo));
    //memset((char*)&m_stMsgHdData.stSetAlramTime,0,sizeof(stRTCInfo));
    // we don't need to clear because we're setting value from serial flash
//    memset((char*)&m_stMsgHdData.stObdSettingValue,0,sizeof(m_stMsgHdData.stObdSettingValue));
    //memset(m_stMsgHdData.chaPhoneNo,0,MAX_PHONE_NUMBER_SIZE);
    m_stMsgHdData.uiSubSysSleepEvent = 0;
    m_stMsgHdData.bAlreadyRcvSmartkey = false;
    m_stMsgHdData.bReqPostPondforSending = false;
    // suspend sending message
    m_stMsgHdData.bSuspend = false;
	m_stMsgHdData.bSkipMessage = false;

    return 0;
}

// main control function
int HdMngSysMsg(int iLparam,int iRparam)
{
    int ret = -1;
    stMsgSysMsg stTempMsg;

    SysMsgEvtHandler(iLparam,iRparam);

    // check external event
#ifndef GLOBAL_SHARE_QUEUE //Get
    if( MngQueueGetMessage(ID_MNG_QUEUE_SYS_MSG,(int8_t*)&stTempMsg, sizeof(stMsgSysMsg) ) == true )
#else
	if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_SYS_MSG, (uint8_t*)&stTempMsg.header, sizeof(stMsgHeader), (uint8_t*)&stTempMsg.carReport, sizeof(stCarReport)) == true )
#endif
    {
#if defined(MNG_QUEUE_DEBUG)
        Trace("event : %s, subEvent : %s, from : %s, reuslt : %x\r\n",
            GetStringFromEvent(eGetStringEvent,stTempMsg.header.event,0),
            GetStringFromEvent(eGetStringSubEvent,stTempMsg.header.event,stTempMsg.header.subEvent),
            GetStringFromId(stTempMsg.header.unTraceMng&0xFF),
            stTempMsg.header.result);

        if( stTempMsg.header.event == eReqReport && stTempMsg.header.subEvent == eR_Alram )
        {
            Trace("alram event : %s\r\n",GetStringFromAlramEvent(stTempMsg.carReport.rpAlram.CarStatus.EventKey));
        }
#endif

        stTempMsg.header.unTraceMng=stTempMsg.header.unTraceMng<<8|eMngSysMsg;

        // handle external event
        HandleMngMsgExternalEvent(&stTempMsg);

        if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_PARKING )
        {
            GetQueueState();
            // reset time out timer
            //MONI 2018-02-09
            // change define to configuration variable
            //EnableSystemMessageTimer(&m_stMsgHdData.iTimeoutTimerID, TIMER_SYSTEM_TIMEOUT_INTERVAL_VALUE, eSWTimer_INFINITE, TimeoutTimerCallBack);
            //uint32_t unSystemTimeout;
            //GetBackupRamConfigProperty(eBackupRamConfig_SystemTimeout,(void*)&unSystemTimeout);
            //EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout, eSWTimer_INFINITE, TimeoutTimerCallBack);
        }
    }

    switch(m_stMsgHdData.iHdMngMsgState)
    {
        case STAT_SYS_MNG_HD_MSG_INIT:
            {
                stHalRTCTypeDef stDate;
                SetMngSysMsgHdState(STAT_SYS_MNG_HD_MSG_IDLE);

                // start checking for sleep
                //MONI 2018-02-09
                // change define to configuration variable
                //EnableSystemMessageTimer(&m_stMsgHdData.iTimeoutTimerID, TIMER_SYSTEM_TIMEOUT_INTERVAL_VALUE, eSWTimer_INFINITE, TimeoutTimerCallBack);
                uint32_t unSystemTimeout;
                GetBackupRamConfigProperty(eBackupRamConfig_SystemTimeout,(void*)&unSystemTimeout);
                EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout, eSWTimer_INFINITE, TimeoutTimerCallBack);

                // MONI 2018-03-15
                // check local time
                // if local time is inialized because of cold booting
                // then wait for network time.
                GetUTCTimeforDate(&stDate);
                if( stDate.RtcDate.RTC_Year == 17 )
                {
                    m_stMsgHdData.bReqWaitSendingforNetworkTime = true;
                }
            }
            break;
        case STAT_SYS_MNG_HD_MSG_SLEEP:
            // response to system to go sleep.

            if( m_bSysMsgFirst == true )
            {
                ret = eRspSleep;
                m_bSysMsgFirst = false;
            }

            break;
        case STAT_SYS_MNG_HD_MSG_SLEEP_WAIT:
            // wait for other manager sleep response

            // this is for test because obd manager is not impolemented
            if( m_stMsgHdData.uiSubSysSleepEvent == SYS_MNG_HD_SLEEP_MASK )
            //if( m_stMsgHdData.uiSubSysSleepEvent ^= SYS_MNG_HD_SLEEP_MASK == 0x00000004 )
            {
                //All peripheral was response for sleep
                //response to system to go down.
                Trace("%s] All peripheral was response for sleep.\r\n",__FUNCTION__);

                SetMngSysMsgHdState(STAT_SYS_MNG_HD_MSG_SLEEP);

                DisableSystemMessageTimer(m_stMsgHdData.iTimeoutTimerID);
            }
            break;
        case STAT_SYS_MNG_HD_MSG_IDLE:
            // do something
            // check control flag between storage, obd, modem, sensor

            // this fuction is scenario handle for valet, guard and geo fence.
            HandlerSystemScenario();

            break;
        case STAT_SYS_MNG_HD_MSG_SETUP:
            SetMngSysMsgHdState(STAT_SYS_MNG_HD_MSG_IDLE);

            // chech mode change
            //m_MngSysData.m_iSystemMode = stTempMsg.header.subEvent;

            if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_PARKING )
            {
                InitizlieSystemMessage();
                // clear busi flag in queue
                GetQueueState();
                //MONI 2018-02-09
                // change define to configuration variable
                //EnableSystemMessageTimer(&m_stMsgHdData.iTimeoutTimerID, TIMER_SYSTEM_TIMEOUT_INTERVAL_VALUE, eSWTimer_INFINITE, TimeoutTimerCallBack);
                uint32_t unSystemTimeout;
                GetBackupRamConfigProperty(eBackupRamConfig_SystemTimeout,(void*)&unSystemTimeout);
                EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutTimerID, unSystemTimeout, eSWTimer_INFINITE, TimeoutTimerCallBack);
            }
            else if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_DRIVING )
            {
                DisableSystemMessageTimer(m_stMsgHdData.iTimeoutTimerID);
            }

            break;
        default:
            break;
    }

    return ret;
}

unsigned long m_ulGeofenceTimeStamp = 0;
unsigned long m_ulValetTimeStamp = 0;
unsigned long m_ulGuardTimeStamp = 0;
unsigned long m_ulAgpsTimeStamp = 0;
unsigned long m_ulRsvEngCtrlTimeStamp = 0;
unsigned long m_ulTrackingTimeStamp = 0;



void ProcessGeoFence()
{
#if false
    // set the 5 seconds for checking of gps
    if( GetGeofenceActive() == true )
    {
        if( SystemDelayProcess(&m_ulGeofenceTimeStamp,5*ONE_SECOND) == true )
        {
            //Trace("Geofence Check Time\r\n");
            stGeofenceUnit stCurrentUnit;
            stGeofenceUnit stSettingUnit;

            stCurrentUnit.GpsLatitude = Get_GPS_Lat();
            stCurrentUnit.GpsLongitude = Get_GPS_Lon();

            memset((char*)&stSettingUnit,0,sizeof(stGeofenceUnit));

#ifdef USE_REPORT_ONLY_ONE_GEOFENCE
            if( CheckGeoFence(stCurrentUnit,&stSettingUnit,false,true) == true )
            {
                // send geo fence alram
                SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM, 1, stCurrentUnit, stSettingUnit,eGFT_CIRCLE,0);
            }
#else
            // check all check point
            CheckGeoFence(stCurrentUnit,&stSettingUnit,false,true);
#endif
        }
    }
#else //#if true
    // set the 5 seconds for checking of gps
    if( SystemDelayProcess(&m_ulGeofenceTimeStamp,5*ONE_SECOND) == true )
    {
        stGeofenceUnit stCurrentUnit;
        stCurrentUnit.GpsLatitude = Get_GPS_Lat();
        stCurrentUnit.GpsLongitude = Get_GPS_Lon();
        stGeofenceUnit stSettingUnit;
        memset((char*)&stSettingUnit,0,sizeof(stGeofenceUnit));

        // check circle geo fence.
        if( GetGeofenceActive() == true )
        {
            //Trace("Geofence Check Time\r\n");


#ifdef USE_REPORT_ONLY_ONE_GEOFENCE
            if( CheckGeoFence(stCurrentUnit,&stSettingUnit,false,true) == true )
            {
                // send geo fence alram
                SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM, 1, stCurrentUnit, stSettingUnit,eGFT_CIRCLE,0);
            }
#else
            // check all check point
            CheckGeoFence(stCurrentUnit,&stSettingUnit,false,true);
#endif
        }

        // check polygon geo fence.
        if( GetPolygonGeofenceActive() == true )
        {
            CheckPolygonGeoFence(stCurrentUnit,&stSettingUnit);
        }
    }
#endif //#if true
}

void ProcessTracking()
{
	if(GetTrackingActive())
	{
		CheckTracking();
	}
}

void ProcessValet()
{
    if( GetValetActive() == true )
    {
        if( SystemDelayProcess(&m_ulValetTimeStamp,1*ONE_MINUTE) == true )
        {
            int32_t nResult;
            stGeofenceUnit stCurrentUnit;
            stGeofenceUnit stSettingUnit;

            Trace("Valet Check Time\r\n");

            nResult = CheckValet(&stCurrentUnit,&stSettingUnit);

            if( nResult > eValetNone )
            {
                // send valet alram
                Trace("###############################################\r\n");
                Trace("Valet Alram Occured :%d\r\n",nResult);
                SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_VALET_ALRAM, nResult, &stCurrentUnit, &stSettingUnit,eGFT_VALET,0);
            }
        }
    }
}

void ProcessGuard()
{
    if( GetGuardActive() == true )
    {
        if( SystemDelayProcess(&m_ulGuardTimeStamp,10*ONE_SECOND) == true )
        {
            Trace("Guard Check Time\r\n");
            if( CheckGuard() == true )
            {
                // send guard alram
                Trace("Guard] Report Time out\r\n");
//#warning "Do we need to report a alram that guard mode time out"
            }
        }
    }
}

boolean_t IsItExceptionalTime(uint32_t unUtcTime)
{
    uint32_t un2035Time = 2066684408; // pre-calculated time //2035 06 28 23 00 08

    //DisplayTime("Utc Time", unUtcTime);
    //DisplayTime("Limit Utc Time",un2035Time);

    if( unUtcTime > un2035Time )
    {
        Trace("Time is not correct\r\n");
        return true;
    }
    return false;
}

boolean_t IsItNeededAdjustUTCTime(uint32_t unUtcTime, uint8_t ucTimeFrom)
{
    uint32_t unCurrentUtcTime = GetUTCTime();
    int nDifTime = (int)unUtcTime - (int)unCurrentUtcTime;
	boolean_t result = false;

    // this check is for 2090 year.
    if( IsItExceptionalTime(unUtcTime) == true )
        return false;

    // if different time is higher or lower than 5 seconds.

	switch(ucTimeFrom)
	{
	   case SERVERUTCTIMESET: 
	   		if((nDifTime >= ONE_MINUTE)) result=true;
			break;
	   case NETUTCTIMESET:
	   		if(nDifTime > 3) result=true;
			break;
	   case GPSUTCTIMESET:
			if( nDifTime > 5 || nDifTime < -5 ) 
			{
				result=true;
			}
			break;
	   default:
	   		break;
	}
    return result;
}

void AdjustNewTime(uint32_t unUtcTime, uint8_t ucTimeFrom)
{
    if(IsItNeededAdjustUTCTime(unUtcTime, ucTimeFrom) == true)
    {
        stHalRTCTypeDef stDate;
        uint32_t unCurrentUTCTime;
        //int32_t nTemp;

        // check gps time / network time / local time
        // convert utc to local
        unCurrentUTCTime = GetUTCTime();

        // check time different between gps and local time
        Trace("=============================================================\r\n");
        Trace(" New Time Adjust to System\r\n");
        Trace("=============================================================\r\n");
        DisplayTime("New Utc Time :",unUtcTime);
        DisplayTime("Local Utc Time :",unCurrentUTCTime);
        Trace("Different Time : %d\r\n", unUtcTime - unCurrentUTCTime);
        
#ifdef SPARKUSIM
        if(BkSram_ModemInfo.snTimeZone == 0)
            return;
#endif
        // if time different is over 2 then sett new alram with gps time
        // set network time to rtc time
        GetDatefromTime2(&stDate, unUtcTime);
        HalDrvRtcWrite(eRtcBin, eRtcAll, (char*)&stDate, sizeof(stHalRTCTypeDef), 0);


		if(BkSram_ModemInfo.snTimeZone != 36) // Korea....
		{
#ifdef QA_FIFA
		//modem timezone unit : 1=15m, 2=30m, 3=45m / QA Timezone = 3 h, so timezone is 12 
			if( BkSram_ModemInfo.snTimeZone != 12) BkSram_ModemInfo.snTimeZone = 12; 
#elif SINGTELUSIM
			if( BkSram_ModemInfo.snTimeZone != 32) BkSram_ModemInfo.snTimeZone = 32; 
#elif VIETNAMUSIM
			if( BkSram_ModemInfo.snTimeZone != 4) BkSram_ModemInfo.snTimeZone = 4; 
#endif
		}
		unCurrentUTCTime = GetUTCTime();
		DisplayTime("New Local Time : ", unCurrentUTCTime);
        Trace("=============================================================\r\n");
    }
}

void ProcessAdjustTime()
{
    uint32_t unGpsUtcTime = 0;
    uint32_t unNetUtcTime = 0;
	uint32_t unServerUtcTime = 0;
    boolean_t bNetUtcTime = GetNetworkUtcTime(&unNetUtcTime);
    boolean_t bGpsUtcTime = GetGpsUtcTime(&unGpsUtcTime);
	boolean_t bServerUtcTime = GetServerUtcTime(&unServerUtcTime);

	
#if false        
	DisplayTime("Net Utc Time : %s\n",unNetUtcTime);
	DisplayTime("Gps Utc Time : %s\n",unGpsUtcTime);
	DisplayTime("Server Utc Time : %s\n",unServerUtcTime);
#endif
	//Priority 1.GPS 2.Server 3.Modem
	if((bGpsUtcTime == true) && (Get_GPS_Vailication() == 1))
        {
		AdjustNewTime(unGpsUtcTime, GPSUTCTIMESET);
            }
	else if( bServerUtcTime == true)
            {
		AdjustNewTime(unServerUtcTime, SERVERUTCTIMESET);
        }
	else if( bNetUtcTime == true )
        {
		AdjustNewTime(unNetUtcTime, NETUTCTIMESET);
    }
	
}

void ProcessAgps()
{
#ifdef RF_COMMON_MODEM
    static bool s_bFirstTimeFlag=true;
#endif
    
    if( SystemDelayProcess(&m_ulAgpsTimeStamp,10) == true )
    {
        //Trace("Agps Check Time\r\n");
#ifdef USE_UBLOX_GPS
        if( HandlerUblox() == eUbloxIdle )
#endif
        {
            //if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON)
            BOOL bModemActive;
            GetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);
            if ( bModemActive == true && (CheckProductMode()!=true)) // 2022/11/22 모뎀 액티브상태에서만 AGPS 활성화하여야함. 생산 이슈로 변경함.
            {
                if( GetReceivedNetworkTime() == true )
                {
                    //if( GetBlockMsgTrasfer() == false )
                        CheckExpiretAGPSData();
                }
#ifdef RF_COMMON_MODEM
                else if( (CheckProductMode()==true) && (s_bFirstTimeFlag == true) )
                {
                    Send2MngSysMsg(eMngSys,eReqAgps,eAgpsDownload,(stCarReport *)NULL,0);
                    s_bFirstTimeFlag=false;
                }
#endif
            }
        }
    }
}

bool CheckProductMode()
{
	if( (ModemManagerData.eState==eMODEM_NO_USIM || ModemManagerData.eState==eMODEM_PRODUCT_MODE) 
        && (memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_NOSERIAL_NUMBER) == 0 ) )
    	return true;
	else
		return false;
}

void ProcessRsvStatus()
{
	if(GetOBDState() != eOBD_Running_Info_Mode)
		return;

    if( SystemDelayProcess(&m_ulRsvEngCtrlTimeStamp,5000) == true )
    {
        //Trace("Agps Check Time\r\n");
        uint8_t ucHdRsvEngCtrlResult = HandlerRsvEngCtrl();
        static unsigned long s_ulEngOnCtrlTimer = 0;
        boolean_t bCanCtrl=false;

        if(s_ulEngOnCtrlTimer == 0)
			bCanCtrl=true;
		else if(Get_Tmr()-s_ulEngOnCtrlTimer>60000 && s_ulEngOnCtrlTimer != 0)
			bCanCtrl=true;

		if( ucHdRsvEngCtrlResult<eRsvEngOnInOneMin && ucHdRsvEngCtrlResult>eRsvEngOnNotNow && bCanCtrl)
        {
        	//printf("#####%s#########################\n", __FUNCTION__);
			stCarReport stReport;

			memcpy(&stReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl,&m_stAutolinkConfigData.stUserAction.RsvEngCtrl,
			sizeof(stRsvEngCntorl));

			stReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.ucWakeupIndexFlag = ucHdRsvEngCtrlResult-1;

			Send2MngSysMsg2(eMngSysMsg,eReqRsvEngCtrlOn,0,eTrue,&stReport,0);
			s_ulEngOnCtrlTimer = Get_Tmr();
			bCanCtrl = false;
        }
        else if(ucHdRsvEngCtrlResult == eRsvEngOnInOneMin)
        {
        	if(Get_VehicleStatus() < eVEHICLE_STATE_ENGRUN)
				SetPostponeSleepFlag(true);
        }
    }
}

void HandlerSystemScenario()
{
    // it will be actived over initialzie state
    if( m_stMsgHdData.iHdMngMsgState <= STAT_SYS_MNG_HD_MSG_INIT )
        return;

    // GeoFence process
    ProcessGeoFence();

	// Tracking process
    ProcessTracking();

    // Valet process
    ProcessValet();

#ifdef ENABLE_GUARD_MODE
    // Gaurd process
    ProcessGuard();
#endif

#ifdef ENABLE_ADJUST_TIME
    // Time Adjust process
    ProcessAdjustTime();
#endif

    // agps handler
    ProcessAgps();

    // user reservate status
    ProcessRsvStatus();

	// Horn Security Alarm 
	ProcessSecurityAlarm();
}

void SetMngSysMsgHdState(int iState)
{
    GIT_Assert(iState>=STAT_SYS_MNG_HD_MSG_INIT&&iState<=STAT_SYS_MNG_HD_MSG_SETUP,eErrorCodeSysHd|eUnknownStateError);
    m_stMsgHdData.iHdMngMsgState = iState;
}

void SysMsgEvtHandler(int iLparam, int iRparam)
{
    if( iLparam == eMngSys )
    {
        if( iRparam == eReqSleep )
        {
            uint8_t bModemActive;
            GetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);
            Trace("Get modem active : %d\r\n",bModemActive);

#ifdef USE_GEMALTO_MODEM
            // send request sleep event to all handlers.
            // if modem is active then send request sleep
            // if( bModemActive == true )
            Send2MngModem(eMngSysMsg, eReqSleep, 0, (stCarReport *)NULL,0);
#else
            bModemActive = false;
#endif

            Send2MngStorage(eMngSysMsg, eReqSleep, 0, (stCarReport *)NULL,0);
            Send2MngObd(eMngSysMsg, eReqSleep, 0, (stCarReport *)NULL,0);
#if defined(FEATURE_EXTENSION_BOARD)
            Send2SysExtend(eMngSysMsg, eReqSleep, 0, false, NULL, 0);
#endif
            SetMngSysMsgHdState(STAT_SYS_MNG_HD_MSG_SLEEP_WAIT);

            // check bit variable for sleep
            // all peripheral will response to msg handler.
            m_stMsgHdData.uiSubSysSleepEvent = 0;
        }
        else if( iRparam == eReqWakeupInterruptInfo )
        {
            // check rtc alram
            // if true, report no driving information to server
            if( CheckAlramInterrupt() == true )
            {
                // set flag after time out, send request no parking report to obd manager.
                EnableSystemMessageTimer((int32_t*)&m_stMsgHdData.iTimeoutNoDrivingReportTimerID, TIMER_SYSTEM_TIMEOUT_INTERVAL_VALUE, eSWTimer_ONESHOT, TimeoutNoDrivingTimerCallBack);
            }
        }
        else if( iRparam == eReqChangeMode )
        {
            SetMngSysMsgHdState(STAT_SYS_MNG_HD_MSG_IDLE);
        }
    }
}

void HandleMngMsgExternalEvent(stMsgSysMsg* pstMsgSysMsg)
{
    // check handler id
    GIT_Assert((pstMsgSysMsg->header.id>eMngNone)&&(pstMsgSysMsg->header.id < eMngMax),eErrorCodeSysHd|eUnknownId);
    m_pfnMngSysMsgEventList[pstMsgSysMsg->header.id](pstMsgSysMsg);
}

void InitizlieSystemMessage()
{
}

boolean_t SetSystemDrivingKey(long long llValue)
{
    long long llTemp = 20171200000000;
	unsigned long int lValueH;
	unsigned long int lValueL;

    lValueH = llValue>>32;
    lValueL = llValue;

    m_stMsgHdData.m_llSytemDrivingKey=llValue;

    Trace("=============================================================\r\n");
    Trace("%s] Set System Driving Key : %04x%08x\r\n",__FUNCTION__,lValueH,lValueL);

    if( (llValue - llTemp) < 0 )
        return false;

    return true;
}

void GetSystemDrivingKey(long long* value)
{
    *value = m_stMsgHdData.m_llSytemDrivingKey;
}

// interval : 30sec from Timer
void IntervalTimerCallBack(void)
{
    // MONI 2018-03-01
    // thisi is temporary position after that we need to move this code to the right position
    //if( m_stMsgHdData.bServerConnected == true && m_stMsgHdData.bIsStoreData == true )	//210308 lwh storage 프로세스에서 처리되고 있는 부분 중복 제거
    //{
    //    Send2MngStorage(eMngSysMsg,eReqGetReport,0,(stCarReport *)NULL,0);
    //}
    // INOM

    // 1. Get Interval Data from OBD
    // 2. Send the data to server.
    // MONI 2020.07.07 this code changed for fuel cell process.
    Send2MngObd(eMngSysMsg, eReqReport, eR_DrivingInterval,(stCarReport *)NULL,0);
    // INOM
}

void TimeoutNoDrivingTimerCallBack()
{
    // request to obd for no dirive report message
    Send2MngObd(eMngSysMsg,eReqReport,eR_ParkingInterval, (stCarReport *)NULL,0);
}

bool CheckIdleQueue()
{
    return GetQueueState();
}

// time out interval
void TimeoutTimerCallBack(void)
{
    Trace("TimeoutTimerCallBack Enter\r\n");
    // 1. Get Interval Data from OBD
    // 2. Send the data to server.
    if( m_MngSysData.m_iSystemMode == SYSTEM_MODE_DRIVING)
    {
        // system can not use this timer in driving mode
        GIT_Assert(false,eErrorCodeSysHd|eModeError);

        if( m_stMsgHdData.ucIGOffCount >= ((3*ONE_MINUTE)/OBD_SLEEP_TIME) )
        {
            // send message to go to sleep forcely
            Send2MngStorage(eMngSysMsg,eReqForcelySleep,0, (stCarReport *)NULL,0);
            Send2MngModem(eMngSysMsg,eReqForcelySleep,0, (stCarReport *)NULL,0);

            m_stMsgHdData.ucIGOffCount = 0;
        }

        m_MngSysData.m_iSystemMode = SYSTEM_MODE_PARKING;
    }
    else if(m_MngSysData.m_iSystemMode == SYSTEM_MODE_PARKING )
    {
        boolean_t bSentMessage = false;
        Trace("%s] Server Status : %d, Exist Data : %d,m_bSuspendStorage:%d,m_bSendReport:%d,m_bReqNetworkPostPone:%d,m_bSentLastMessage:%d\r\n",
            __FUNCTION__,m_stMsgHdData.bServerConnected,m_stMsgHdData.bIsStoreData,m_bSuspendStorage,m_bSendReport,m_bReqNetworkPostPone,m_bSentLastMessage);

        // check to store message
        // 1. if going on fota
        // 2. server is not connected
        // 3. data is exist
        if( m_stMsgHdData.bServerConnected == true )
        {
            //if( m_stMsgHdData.bIsStoreData == true )
            if( m_stMsgHdData.ucIGOffCount < ((10*ONE_MINUTE)/OBD_SLEEP_TIME) )
            {
    			if( m_stMsgHdData.bIsStoreData == true && m_bSuspendStorage == false && m_bSendReport == false && m_bReqNetworkPostPone == false && m_bSentLastMessage == false) // for 2 seconds
                {
    	                // wait for 10 minutes after that ME is going to sleep.	                
                        bSentMessage = true;
                        Send2MngStorage(eMngSysMsg,eReqGetReport,0,(stCarReport *)NULL,0);
    					return; 
    	            }
    				else if( m_bReqNetworkPostPone == false && (m_bSendReport == true || m_bSentLastMessage == true) )
    				{
    					return;
                    }
                }

#ifdef ENABLE_FOTA
            //printf("bSentMessage : %d m_stMsgHdData.bIGOff : %d m_bPostponeReqSleep : %d m_bNeedUpdate : %d \r\n",bSentMessage,m_stMsgHdData.bIGOff,m_bPostponeReqSleep,m_bNeedUpdate);

            // this is fota process
            // 1. before we go to sleep, we should check version info
            // 2. send the message to modem to get version info
            // 3. wait for response from modem.
            if( bSentMessage == false &&
                m_stMsgHdData.bIGOff == true &&
                m_bPostponeReqSleep == false &&
                m_bNeedUpdate == true )
            {
                m_bPostponeReqSleep = true;
                m_bNeedUpdate = false;

                // if already check fota info
                // and check we must update firmware or not
                // send request fota info
                // here is fota prodedure
                // 1. get vehicle information
                // 2. get version information
                // 3. download binary
                //Send2MngModem(eMngSysMsg,eReqFota,eFwVehicleInfo,(stCarReport *)NULL,0);

                stCarReport stReport;
                stReport.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_FOTA;
                memset((char*)&stReport.rpSmartKey.Request.Guid,0,MAX_GUID_LENGTH);

#if 1
extern stServerUrl g_stServerUrl;


    stReport.rpSmartKey.Request.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    memset(stReport.rpSmartKey.Request.Guid,0,MAX_GUID_LENGTH);
    char * cSeriaNumberCheck = GetFWSerialNumber();
    
    //if ( strcmp(g_stServerUrl.Url, VN_HTTP_RETAIL_DEV_MESSAGE_URL_KIA) == 0 )
    if( strstr(cSeriaNumberCheck,"_DEV") != NULL )
    {
        stReport.rpSmartKey.Request.ControlType = eREMOTE_CON_FOTA_TYPE_TEST;
        Send2MngSysMsg(eMngModem,eReqFota,eFwTestUpdate,(stCarReport *)&stReport,0);
    }
    else
    {
        stReport.rpSmartKey.Request.ControlType = eREMOTE_CON_FOTA_TYPE_UPDATE;
        Send2MngSysMsg(eMngModem,eReqFota,eFwUpdate,(stCarReport *)&stReport,0);
    }
#else
                stReport.rpSmartKey.Request.ControlType = eREMOTE_CON_FOTA_TYPE_UPDATE;
                stReport.rpSmartKey.Request.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
                memset(stReport.rpSmartKey.Request.Guid,0,MAX_GUID_LENGTH);

                // we set a fake the manager id with manager modem because we would like to make one fota procedure.
                // because when server request force fota to the device then we used same path.
                Send2MngSysMsg(eMngModem,eReqFota,eFwUpdate,(stCarReport *)&stReport,0);
#endif
            }
        }
#endif

        // wait for 10 minutes after that ME is going to sleep.
        if( m_stMsgHdData.ucIGOffCount >= ((15*ONE_MINUTE)/OBD_SLEEP_TIME) && BTGetConnectStatus()==false )
//            && m_bSentLastMessage == false && m_bSendReport == false)
        {
            // send message to go to sleep forcely
            Send2MngStorage(eMngSysMsg,eReqForcelySleep,0, (stCarReport *)NULL,0);
            Send2MngModem(eMngSysMsg, eReqForcelySleep, 0, (stCarReport *)NULL,0);

            m_stMsgHdData.ucIGOffCount = 0;
            SetPostponeSleepFlag(false);
        }

        //Trace("%s] mode : SYSTEM_MODE_PARKING\r\n",__FUNCTION__);
        // Check Idle or not
        // if system is idle go to sleep
        // add check routine queue status.
        // every 5 seconds, it will be changed just test
        Trace("ig off : %d, m_bPostponeReqSleep :%d\r\n",m_stMsgHdData.bIGOff,m_bPostponeReqSleep);
        Trace("bServerConnected : %d, m_stMsgHdData.ucIGOffCount :%d\r\n",m_stMsgHdData.bServerConnected,m_stMsgHdData.ucIGOffCount);

        if( m_stMsgHdData.bIGOff == true
#ifdef ENABLE_FOTA
            && m_bPostponeReqSleep == false
#endif
            )
        {
            // check data transfer or not signal area

            if( m_stMsgHdData.bServerConnected == true &&
                CheckIdleQueue() == true &&
                m_stMsgHdData.ucIGOffCount<((10*ONE_MINUTE)/OBD_SLEEP_TIME))
            {
                return;
            }

            // to prevent go to sleep before wake up interrupt
            if( CheckInterruptSignal() == false )
            {
                boolean_t bIntModem = GetWakeupDetectPinState(eWAKE_PIN_MODEM);
                if( bIntModem == false && BTGetConnectStatus()==false )
                {
#ifdef ENABLE_SYSTEM_SLEEP
                    // reuqest to sleep
                    if(g_bEnableModemDirectCommunication == false)	Send2MngSys(eMngSysMsg,eReqSleep,0, (stCarReport *)NULL,0);
#endif
                }
                else
                {
                    Trace("Received SMS\r\n");
                    Send2MngSysMsg(eMngModem,eDummy,0,(stCarReport *) NULL,0);
                }
            }
            else
            {
                // clear sensor
                if( GetWakeupDetectPinState(eWAKE_PIN_SENSOR) == true )
                {
                    MPU6515_CheckInterrupt();
                }
            }
        }
    }
    else
    {
        m_MngSysData.m_iSystemMode = SYSTEM_MODE_PARKING;
        GIT_Assert(false,eErrorCodeSysHd|eUnknownModeError);
    }
}

void SetPostponeSleepFlag(boolean_t bPostpone)
{
	if( bPostpone == false )
	{
		if( m_stUserActionSetting.Tracking.bActive == false )	m_bPostponeReqSleep = bPostpone;
	}
	else
	{
		m_bPostponeReqSleep = bPostpone;
	}
}

void GetPostponeSleepFlag()
{
}

void SetNetworkUtcTime(stHalRTCTypeDef stDate)
{
    m_unNetworkUtcTime = GetTimefromDate2(stDate);
    m_bNetworkUtcTime = true;
}

boolean_t GetNetworkUtcTime(uint32_t* punNetworkUtcTime)
{
    if( m_bNetworkUtcTime == true )
    {
        *punNetworkUtcTime = m_unNetworkUtcTime;

        m_bNetworkUtcTime = false;
        return true;
    }

    *punNetworkUtcTime = 0;
    return false;
}

void SetSystemOdometer(uint32_t unOdometer)
{
    m_stMsgHdData.stObdSettingValue.ObdSetting.unOdometer = unOdometer;
}

uint32_t GetSystemOdometer()
{
    return m_stMsgHdData.stObdSettingValue.ObdSetting.unOdometer;
}

