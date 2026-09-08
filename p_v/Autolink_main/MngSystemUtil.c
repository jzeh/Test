/* Includes ------------------------------------------------------------------*/
#include "AutolinkMessage.h"
#include "AutolinkConfig.h"

#include "HalHandler.h"
#include "AutolinkConfig.h"
#include "AutolinkMessage.h"
#include "AutolinkConfiguration.h"
#include "Autolink_Manager.h"
#include "Modem_Manager.h"
#include "GIT_Util.h"
#include "GIT_OemInterface.h"
#include "Modem_comm.h"
#include "Power_Manager.h"
#include "OBD_Controller.h"
#include "Message_Make.h"
#include "ff.h"

#include "MngSystem.h"
#include "MngSystemUtil.h"
#include "MngQueue.h"
#include "HdDebug.h"

#include <math.h>
#include <time.h>
#include "HandlerRsvEngCtrl.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)
#define MAX_RSVENGCTRL_CNT (5)

// user action setting variable
bool g_bAnyEventSentFlag = false;
extern stUserActionSetting m_stUserActionSetting;

extern stMsgHandlerData m_stMsgHdData;
extern BR_SystemInfo BkSram_SystemInfo;

extern void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern void SetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern void SetPostponeSleepFlag(boolean_t bPostpone);

//native
unsigned int GetTimefromDate(stHalRTCTypeDef stRtcInfo);
void GetDatefromTime(stHalRTCTypeDef* stDateTime,unsigned int binary);

//unix to system
uint32_t GetTimefromDate2(stHalRTCTypeDef stDate);
void GetDatefromTime2(stHalRTCTypeDef* stDate,uint32_t unTime);
void ConvertLocal2UtcTime(uint32_t unLocalTime, uint32_t* punUtcTime);
uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
void PowerOnGemaltoModem();
extern uint32_t GetUTCTime();

extern void SetObdSetting(stAutolinkConfigData* pstAutolinkConfigData);
extern boolean_t IsAvailableVin(char* pcarrVin);
extern bool GetModemActive();
extern void DisplayGeofenceSetting();
extern unsigned int Get_Odmeter(void);
extern double Get_GPS_Lat_Origin();
extern double Get_GPS_Lon_Origin();
extern long long Get_DrivingKey();
extern void GetUTCTimeforDate(stHalRTCTypeDef* pstDate);

void Send2MngSysSub(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng)
{
    stMsgSysSub stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgSysSub));
    stMsg.header.id = iId;
    stMsg.header.event = iEvent;
    stMsg.header.subEvent = iSubEvent;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));
#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_SYS_SUB, (int8_t*)&stMsg, sizeof(stMsgSysSub));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_SYS_SUB,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif

}

void Send2MngModem(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport, uint32_t unTraceMng)
{
    stMsgMdm stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgMdm));
    stMsg.header.id = iId;
    stMsg.header.event = iEvent;
    stMsg.header.subEvent = iSubEvent;
	stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();

    if( pstReport != NULL )
       memcpy((int8_t*)&stMsg.carReport,pstReport,sizeof(stCarReport));

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));

#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_MDM, (int8_t*)&stMsg, sizeof(stMsgMdm));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_MDM,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif
}

void Send2MngModem2(stMsgMdm* pstMsg)
{
#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_MDM, (int8_t*)pstMsg, sizeof(stMsgMdm));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_MDM,(uint8_t*)&pstMsg->header,sizeof(stMsgHeader), (uint8_t*)&pstMsg->carReport, sizeof(stCarReport));
#endif
}

void Send2MngModem3(uint16_t iId, int32_t iEvent,int32_t iSubEvent,int32_t iResult, stCarReport* pstReport, uint32_t unTraceMng)
{
    stMsgMdm stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgMdm));
    stMsg.header.id = iId;
    stMsg.header.event = iEvent;
    stMsg.header.subEvent = iSubEvent;
	stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());

	stMsg.header.drivingKey = Get_DrivingKey();
    stMsg.header.result = iResult;

    if( pstReport != NULL )
       memcpy((int8_t*)&stMsg.carReport,pstReport,sizeof(stCarReport));

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));

#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_MDM, (int8_t*)&stMsg, sizeof(stMsgMdm));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_MDM,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif
}


void Send2MngSensor(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng)
{
    stMsgSensor stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgSensor));
    stMsg.header.id = iId;
    stMsg.header.event = iEvent;
    stMsg.header.subEvent = iSubEvent;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));

#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_SENSOR, (int8_t*)&stMsg, sizeof(stMsgSensor));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_SENSOR,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif
}


void Send2MngObd(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng)
{
    stMsgObd stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgObd));
    stMsg.header.id = iId;
    stMsg.header.event = iEvent;
    stMsg.header.subEvent = iSubEvent;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();

    if( pstReport != NULL )
       memcpy((int8_t*)&stMsg.carReport,pstReport,sizeof(stCarReport));

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));

#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_OBD, (int8_t*)&stMsg, sizeof(stMsgObd));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_OBD,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif
}

void Send2MngObd2(stMsgObd* pstMsgObd)
{
#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_OBD, (int8_t*)pstMsgObd, sizeof(stMsgObd));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_OBD,(uint8_t*)&pstMsgObd->header,sizeof(stMsgHeader), (uint8_t*)&pstMsgObd->carReport, sizeof(stCarReport));
#endif
}

void Send2MngStorage(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng)
{
    stMsgStorage stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgStorage));
    stMsg.header.id = iId;
    stMsg.header.event = iEvent;
    stMsg.header.subEvent = iSubEvent;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();

    if( pstReport != NULL )
        memcpy((int8_t*)&stMsg.carReport,pstReport,sizeof(stCarReport));

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));

#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_STORAGE, (int8_t*)&stMsg, sizeof(stMsgStorage));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_STORAGE,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif
}

void Send2MngStorage2(stMsgStorage* pstMsgStorage)
{
#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_STORAGE, (int8_t*)pstMsgStorage, sizeof(stMsgStorage));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_STORAGE,(uint8_t*)&pstMsgStorage->header,sizeof(stMsgHeader), (uint8_t*)&pstMsgStorage->carReport, sizeof(stCarReport));
#endif
}

void Send2MngSysMsg(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng)
{
    stMsgSysMsg stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgSysMsg));
    stMsg.header.id = iId;
    stMsg.header.event=iEvent;
    stMsg.header.subEvent=iSubEvent;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();
    stMsg.header.result = 0;
    if( pstReport != NULL )
    {
        memcpy((int8_t*)&stMsg.carReport,(int8_t*)pstReport,sizeof(stCarReport));
    }

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));

#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_SYS_MSG, (int8_t*)&stMsg, sizeof(stMsgSysMsg));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_SYS_MSG,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif
}

void Send2MngSysMsg2(uint16_t iId, int32_t iEvent,int32_t iSubEvent,int32_t iResult, stCarReport* pstReport,uint32_t unTraceMng)
{
    stMsgSysMsg stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgSysMsg));
    stMsg.header.id = iId;
    stMsg.header.event=iEvent;
    stMsg.header.subEvent=iSubEvent;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();
    stMsg.header.result = iResult;
    if( pstReport != NULL )
    {
        memcpy((int8_t*)&stMsg.carReport,(int8_t*)pstReport,sizeof(stCarReport));
    }

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));

#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_SYS_MSG, (int8_t*)&stMsg, sizeof(stMsgSysMsg));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_SYS_MSG,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif
}

void Send2MngSysMsg3(stMsgSysMsg* pstMsgSysMsg)
{
#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_SYS_MSG, (int8_t*)pstMsgSysMsg, sizeof(stMsgSysMsg));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_SYS_MSG,(uint8_t*)&pstMsgSysMsg->header,sizeof(stMsgHeader), (uint8_t*)&pstMsgSysMsg->carReport, sizeof(stCarReport));
#endif
}

void Send2MngSysSensor(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng)
{
    stMsgSysSensor stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgSysSensor));
    stMsg.header.id = iId;
    stMsg.header.event=iEvent;
    stMsg.header.subEvent=iSubEvent;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));
#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_SYS_SENSOR, (int8_t*)&stMsg, sizeof(stMsgSysSensor));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_SYS_SENSOR,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif

}

void SendObdConfig2System(uint32_t unOdometer, double dlLatitude, double dlLongitude, uint8_t ucDoorLockStatus, uint8_t ucDoorOpenStatus, uint8_t ucHeadLight, float fReaminedFuel)
{
    stMsgSys stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgSys));

    stMsg.header.id = eMngObd;
    stMsg.header.event = eReqSetting;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();

    stMsg.rpSetting.ObdSetting.unOdometer = unOdometer;
    stMsg.rpSetting.ObdSetting.dlLatitue = dlLatitude;
    stMsg.rpSetting.ObdSetting.dlLongitude = dlLongitude;
	stMsg.rpSetting.ObdSetting.ucDoorLockStatus = ucDoorLockStatus;
    stMsg.rpSetting.ObdSetting.ucDoorOpenStatus = ucDoorOpenStatus;
    stMsg.rpSetting.ObdSetting.ucHeadLight = ucHeadLight;
	stMsg.rpSetting.ObdSetting.fRemainedFuel = fReaminedFuel;

#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_SYS, (int8_t*)&stMsg, sizeof(stMsgSys));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_SYS,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)&stMsg.rpSetting, sizeof(stReportSetting));
#endif
}


void Send2MngSys(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng)
{
    stMsgSys stMsg;
    memset((char*)&stMsg,0,sizeof(stMsgSys));
    stMsg.header.id = iId;
    stMsg.header.event=iEvent;
    stMsg.header.subEvent=iSubEvent;
    stMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stMsg.header.drivingKey = Get_DrivingKey();
    // this message have 2bytes buffer

    if( unTraceMng == 0x0 )
        stMsg.header.unTraceMng = iId;
    else
        stMsg.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));

#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_SYS, (int8_t*)&stMsg, sizeof(stMsgSys));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_SYS,(uint8_t*)&stMsg.header,sizeof(stMsgHeader), (uint8_t*)pstReport, sizeof(stCarReport));
#endif
}

#if defined(FEATURE_EXTENSION_BOARD)
void Send2SysExtend(uint16_t iId, int32_t iEvent,int32_t iSubEvent,int32_t iResult, stMsgExtend* pstExtend,uint32_t unTraceMng)
{
    stMsgExtend stExtend;
    memset((char*)&stExtend,0,sizeof(stExtend));
    stExtend.header.id = iId;
    stExtend.header.event=iEvent;
    stExtend.header.subEvent=iSubEvent;
    stExtend.header.result=iResult;
    stExtend.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	stExtend.header.drivingKey = Get_DrivingKey();
    // this message have 2bytes buffer
    if( stExtend.buffer != NULL )
    {
        memcpy((int8_t*)stExtend.buffer,(int8_t*)pstExtend->buffer, sizeof(stExtend.buffer));
    }

    if( unTraceMng == 0x0 )
        stExtend.header.unTraceMng = iId;
    else
        stExtend.header.unTraceMng = ((unTraceMng<<8)|(iId&0xFF));
#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_EXTEND, (int8_t*)&stExtend, sizeof(stExtend));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_EXTEND,(uint8_t*)&stExtend.header,sizeof(stMsgHeader), (uint8_t*)&pstExtend, sizeof(pstExtend));
#endif
}
#endif

void Send2MngSys2(stMsgSys* pstMsgSys)
{
#ifndef GLOBAL_SHARE_QUEUE
    MngQueueSendMessage(ID_MNG_QUEUE_SYS, (int8_t*)pstMsgSys, sizeof(stMsgSys));
#else
	SendSysHdShareQueueMessage(ID_MNG_QUEUE_SYS,(uint8_t*)&pstMsgSys->header,sizeof(stMsgHeader), (uint8_t*)&pstMsgSys->rpSetting, sizeof(stReportSetting));
#endif
}

void SendBlockingMessage(boolean_t bStop)
{
    int32_t nSubEvent;

    if( bStop == true )
    {
        nSubEvent = eBlockStart;
    }
    else
    {
        nSubEvent = eBlockStop;
    }
    // send an event to block to send data because of using default vin
    Send2MngSysMsg(eMngSys, eReqMsgBlockTrasfer, nSubEvent, (stCarReport *)NULL, eMngSys);
    Send2MngStorage(eMngSys, eReqStorageBlockTrasfer, nSubEvent, (stCarReport *)NULL, eMngSys);
}

char * GetDOWString(uint8_t ucDOW)
{
	static char * m_pst[] = {"NON","MON","TUE","WED","THU","FRI","SAT","SUN"};

	if(ucDOW > 0 && ucDOW<= HAL_RTC_Weekday_Sunday)
	{
		return m_pst[ucDOW];
	}

	return "NON";
}

void ConfigforAlram(boolean_t bWakeupSoon, boolean_t bReqLongSleep)
{
    stHalRTCTypeDef stDateTime;
    stRsvEngCntorl stRsvEngCtrl;
    uint32_t unTime;
    GetUTCTimeforDate(&stDateTime);
    unTime = GetTimefromDate2(stDateTime);
    printf("%s]Current Time(UTC): %04d/%02d/%02d,%02d:%02d:%02d (%s), wTime %d\r\n", __FUNCTION__,
                                            stDateTime.RtcDate.RTC_Year+2000,
                                            stDateTime.RtcDate.RTC_Month,
                                            stDateTime.RtcDate.RTC_Date,
                                            stDateTime.RtcTime.RTC_Hours,
                                            stDateTime.RtcTime.RTC_Minutes,
                                            stDateTime.RtcTime.RTC_Seconds,
                                            GetDOWString(stDateTime.RtcDate.RTC_WeekDay),
                                            unTime);
    //MONI 2018-02-09
    // change define to configuration variable
    //wTime += RTC_SET_WAKEUP_ALRAM_TIME;
    uint32_t unWakeupTime=0,unWakeupAlarmTime=0;
    GetBackupRamConfigProperty(eBackupRamConfig_WakeupInterval,(void*)&unWakeupTime);
	GetBackupRamConfigProperty(eBackupRamConfig_WakeupAlramTime,(void*)&unWakeupAlarmTime);

	//printf("unTime:%d unWakeupAlarmTime:%d\r\n",unTime,unWakeupAlarmTime);
#if 1
	if( bWakeupSoon == false && bReqLongSleep == false )
	{
		if( unTime < unWakeupAlarmTime )	//ÇöÀç ½Ã°£ÀÌ ÁÖÂ÷Áß ÁÖ±âº¸°í ½Ã°£ ÀüÀÏ¶§
		{
			if( (unWakeupAlarmTime - unTime) < 5 )	unTime=unWakeupAlarmTime+5;
			else									unTime=unWakeupAlarmTime;
		}
		else	//ÇöÀç ½Ã°£ÀÌ ÁÖÂ÷ÁßÁÖ±âº¸°í½Ã°£À» Áö³µÀ»¶§
		{
			if(g_bAnyEventSentFlag==false)	// ¾Ë¶÷1ºÐÀü¿¡ ±üÈÄ ÁÖÂ÷ÁßÁÖ±âº¸°í½Ã°£ Áö³ª¼­ Àáµé¶§ ¼­¹ö·Î¿Ã¶ó°£ µ¥ÀÌÅÍ ¾øÀ¸¸é 5ÃÊÈÄ±ú¼­ 60º¸°í
				unTime+=5;
			else
				unTime+=unWakeupTime;
		}
	}
	else
	{
		if( bWakeupSoon == true )
		{
			// wake up after 1 seconds
			unWakeupTime = 3;
		}

		if( bReqLongSleep == true )
		{
			// default is 1 hour
			// wake up after 4 hours
			unWakeupTime *= 4;
		}
    	unTime+=unWakeupTime;
	}
#else
	unTime+=10;
#endif

	memset(&stRsvEngCtrl, 0x00, sizeof(stRsvEngCntorl));

	GetRsvEngCtrlSetting(&stRsvEngCtrl);

	CheckRsvEngControlInfo(&stDateTime, &unTime, &stRsvEngCtrl); // ½Ã°£ º¯°æÀÌ ÇÊ¿äÇÑÁö check

	GetDatefromTime2(&stDateTime,unTime); // wakeup ½Ã°£ º¯°æ
    unTime = GetTimefromDate2(stDateTime);
    Trace("%s] period time: %d(sec)\r\n", __FUNCTION__, unWakeupTime);
    printf("%s]Set Alarm Time(UTC): %04d/%02d/%02d,%02d:%02d:%02d (%s), wTime %d\r\n", __FUNCTION__,
                                            stDateTime.RtcDate.RTC_Year+2000,
                                            stDateTime.RtcDate.RTC_Month,
                                            stDateTime.RtcDate.RTC_Date,
                                            stDateTime.RtcTime.RTC_Hours,
                                            stDateTime.RtcTime.RTC_Minutes,
                                            stDateTime.RtcTime.RTC_Seconds,
                                            GetDOWString(stDateTime.RtcDate.RTC_WeekDay),
                                            unTime);

	HalDrvRtc_SetAlarmTime(&stDateTime); // wakeup ½Ã°£ set

//	Trace("%s] wTime: %d(sec)\r\n", __FUNCTION__, unTime);
//	Trace("%s] period time: %d(sec)\r\n", __FUNCTION__,unWakeupTime);
//	printf("set alarm Time: %02d:%02d:%02d\r\n",stDateTime.RtcTime.RTC_Hours, stDateTime.RtcTime.RTC_Minutes, stDateTime.RtcTime.RTC_Seconds);

    if( bWakeupSoon == false && bReqLongSleep == false )
    {
        // setting alram time to autolink configuration properties
        SetBackupRamConfigProperty(eBackupRamConfig_WakeupAlramTime,(void*)&unTime);
    }

    HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_ENABLE);
}

boolean_t CheckAlramInterrupt()
{
    stHalRTCTypeDef stTimeStamp;
    uint32_t unSettingtime;
    // 1. read back ram alram info that saved last time
    // 2. read rtc setting value
    // 3. compare two time and check if a different is 15min.
    GetUTCTimeforDate(&stTimeStamp);
    stHalRTCTypeDef stHalRtcDateTime;
    APP_TimeShow(&stHalRtcDateTime);

    GetBackupRamConfigProperty(eBackupRamConfig_WakeupAlramTime,(void*)&unSettingtime);

    // change time to seconds
    uint32_t lCurTime = GetTimefromDate2(stTimeStamp);
    uint32_t lSetTime = unSettingtime;

    Trace("#############################################################\r\n");
    Trace("%s] Savetime : %d, SetTime : %d\r\n",__FUNCTION__,lCurTime,lSetTime);
    Trace("%s] different : %d\r\n",__FUNCTION__,lSetTime-lCurTime);
    Trace("#############################################################\r\n");

//#warning "this is test time we should change to target time"
    //long lDifferentTime = 15*1_MIN;
    long lDifferentTime = ( lSetTime - lCurTime ); // 30seconds

    if(  60 > lDifferentTime && -60 < lDifferentTime )
    {
		g_bAnyEventSentFlag = true;
        Trace("%s] Aram Interrupt Occurred\r\n",__FUNCTION__);
        Trace("#############################################################\r\n");
        // alram time
        return true;
    }

    Trace("%s] Aram Interrupt NOT Occurred\r\n",__FUNCTION__);
    Trace("#############################################################\r\n");

    return false;
}

void EnableSystemMessageTimer(int32_t* pnId,int32_t iValue, int32_t iMode, fnSWCallBack callback)
{
    if( *pnId != -1 )
    {
        Trace("change Sw timer : %d\r\n",*pnId);
        //(BYTE ucTimerIndex, uint32_t nTimerInterval_ms,
        //eSWTimerMode eTimerMode,
        //fnSWCallBack fnCallback,
        //BOOL bTimerStart)
        HalTimerChangeSWTimer(*pnId,iValue,(eSWTimerMode)iMode,callback,true);
    }

    if( *pnId == -1 )
    {
        *pnId = HalTimerSetSWTimer(iValue, (eSWTimerMode)iMode, callback, true);
        Trace("Enable Sw Timer : %d\r\n",*pnId);
    }
}

void DisableSystemMessageTimer(int32_t iId)
{
	if( iId != -1 )
	{
		Trace("Disable Sw Timer : %d\r\n",iId);
		HalTimerStopSWTimer(iId);
		//HalTimerClearSWTimer(iId);
	}
}

char* m_pstrManager[]=
{
"eMngNone",
"MngSys",
"MngSysSub",
"MngSysMsg",
"MngSysSensor",
"MngSysFota",
"MngModem",
"MngSensor",
"MngStorage",
"MngObd",
"MngBt",
"MngModemHandler",
};

char* m_pstrSysManager[]=
{
"eReqSleep",
"eRspSleep",
"eReqWakeupInterruptInfo",
"eRspWakeupInterruptInfo",
"eRspWakeupInterruptInfo",
"eReqChangeMode",
"eReqSetting",
"eReqSystemReset",
"eReqRecovery",
"eReqForcelySleep",
"eReqModemPowerOff",
};

char* m_pstrMdmManager[]=
{
"eRcvSms",
"eRcvSmartKey",
"eMdmStatus",
"eMdmRssi",
"eReqPostpond",
"eSetApn",
"eDummy",
};

char* m_pstrMdmManagerStatus[]=
{
"eMS_NoRspSysload",
"eMS_ServerConnected",
"eMS_ServerDisconnected",
"eMS_ChangeOpenSock",
"eMS_PhoneNum",
"eMS_NetworkTime",
};

char* m_pstrStorageManager[]=
{
"eReqSaveReport",
"eRspSaveReport",
"eReqGetReport",
"eRspGetReport",
"eReqDeleteReport",
"eRspDeleteReport",
"eReqDataExist",
"eRspDataExist",
"eReqStartAutoRead",
"eReqStopAutoRead",
"eReqStorageBlockTrasfer",
"eReqSetDrivingKey",
"eReqSuspend",
"eReqDeleteFile",
};

char * m_strMessageManager[]=
{
"eReqReport",
"eRspReport",
"eReqFota",
"eRspFota",
"eReqCarPowerOn",
"eReqCarPowerOff",
"eReqMsgBlockTrasfer",
"eReqForwardingSmartkey2Bt",
"eReqIpek",
"eRspIpek",
"eReqAgps",
"eRspAgps",
};

char * m_strMessageManagerReport[]=
{
"eR_None",
"eR_BeforeDriving",
"eR_DrivingInterval",
"eR_AfterDriving",
"eR_ParkingInterval",
"eR_Alram",
"eR_AlramMasking",
"eR_AlramDTC",
"eR_ImpulseAlram",
"eR_ReqSmartKey",
"eR_RspSmartKey",
"eR_CurrentVehicleStatus",
"eR_SettingGeofence",
"eR_SettingPolygonGeofence",
"eR_ReqUserActionSetting",
"eR_RspUserActionSetting",
"eR_SmsTest",
"eR_SettingInfo",
"eR_WriteTest",
"eR_ReqModemActivate",
"eR_RspModemActivate",
"eR_ReqSensorInitialize",
"eR_RspSensorInitialize",
#if defined(PROTOCOL17)
"eR_RepSetURL",
"eR_RspSetURL",
"eR_Max",
#endif
};

char * m_strMessageManagerFota[]=
{
"eFwVehicleInfo",
"eFwInfo",
"eFwBin",
"eFwUpdate",
"eFwTestUpdate",
};

char * m_strMessageManagerResult[]=
{
"eFalse",
"eTrue",
};

char * m_strObdManager[] =
{
"eOBDStatus",
"eOBDSetting",
"eOBDKeepAlive",
};

char* m_strSonsorManager[]=
{
"eReqWom",
"eRsqWom",
"eReqTowInt",
"eRspTowInt",
};

char* m_strFotaManager[]=
{
"eReqUpdate",
"eRspUpdate",
};

char* m_strAlramEvent[]=
{
"KEY_NONE",                         //0
"KEY_OVER_VOLTAGE_ALARM",
"KEY_LOW_VOLTAGE_ALARM",
"KEY_DOOR_LOCK_ALARM",
"KEY_DOOR_OPEN_ALARM",
"KEY_OVER_ENGINE_TEMP_ALARM",       //5
"KEY_FUEL_RUN_OUT",
"KEY_TIRE_PRESSURE",
"KEY_VEHICLE_OVER_SPEED_ALARM",
"KEY_VEHICLE_OVER_RPM_ALARM",
"KEY_GEO_FENCE_ALRAM",              //10
"KEY_VEHICLE_ENGINE_START_ALARM",
"KEY_TAIL_LAMP_ALARM",
"KEY_PARKING_IMPACT",
"KEY_MIL_LAMP_ON",
"KEY_FOTA_COMPLETE_ALRAM",          //15
"KEY_AIRBAG_ALRAM",
"KEY_TOWING_ALRAM",
"KEY_VALET_ALRAM",
"KEY_ENGKEEP_TIMEOVER_ALRAM",
"KEY_MODEM_POWER_OFF",              //20
"KEY_INDICATOR_ALRAM",
"KEY_CHARGE_ALRAM",
"KEY_SMSEXPIRE_ALRAM",
"KEY_STATE_ALRAM",
"KEY_KEY_REARSEAT_ALRAM",			//25
"KEY_EXTRA_CHARGE_TIME",
"KEY_CHARGING_STATE",
"KEY_MAX",
};

int8_t *GetStringFromId(int32_t nId)
{
    if( nId >= eMngNone && nId < eMngMax )
        return (int8_t *)m_pstrManager[nId];
    else
        return "NotDefined";
}

int8_t *GetStringFromEvent(int32_t nMode,int32_t nEvent,int32_t nSubEvent)
{
    if(nEvent >= eReqSleep && nEvent < eMaxSystemManager )
    {
        // system message
        nEvent^=eMASK_SYSTEM;
        return (int8_t *)m_pstrSysManager[nEvent];
    }
    else
    if(nEvent >= eRcvSms && nEvent < eMaxModemManager )
    {
        // modem message
        if( nMode == eGetStringEvent )
        {
            nEvent^=eMASK_MODEM;
            return (int8_t *)m_pstrMdmManager[nEvent];
        }
        else if( nMode == eGetStringSubEvent )
        {
            if( nEvent == eMdmStatus )
            {
                if(nSubEvent >= eMS_NoRspSysload &&
                   nSubEvent < eMS_MaxModemStatus )
                {
                    return (int8_t *)m_pstrMdmManagerStatus[nSubEvent];
                }
            }
        }
    }
    else
    if(nEvent >= eReqSaveReport &&
       nEvent < eMaxStorageManager )
    {
        // storage message
        nEvent^=eMASK_STORAGE;
        return (int8_t *)m_pstrStorageManager[nEvent];
    }
    else
    if(nEvent >= eReqReport &&
       nEvent < eMaxMessageManager )
    {
        // message message
        if( nMode == eGetStringEvent )
        {
            nEvent^=eMASK_MESSAGE;
            return (int8_t *)m_strMessageManager[nEvent];
        }
        else if( nMode == eGetStringSubEvent )
        {
            if( nEvent == eReqReport ||
                nEvent == eRspReport )
            {
                if( nSubEvent >= eR_None &&
                    nSubEvent < eR_Max )
                {
                    return (int8_t *)m_strMessageManagerReport[nSubEvent];
                }
            }
            else if( nEvent == eReqFota ||
                nEvent == eRspFota )
            {
                if( nSubEvent >= eFwInfo &&
                    nSubEvent < eMaxFotaMessage )
                {
                    return (int8_t *)m_strMessageManagerFota[nSubEvent];
                }
            }
            else if( nEvent == eReqPostpond )
            {
                if( nSubEvent >= eFalse &&
                    nSubEvent < eMaxResult1 )
                {
                    return (int8_t *)m_strMessageManagerResult[nSubEvent];
                }
            }
        }
    }
    else
    if(nEvent == eOBDStatus )
    {
        // obd message
        nEvent ^= eMASK_OBD;
        return (int8_t *)m_strObdManager[nEvent];
    }
    else
    if(nEvent >= eReqWom &&
       nEvent < eMaxSonsorMessage )
    {
        // sensor message
        nEvent ^= eMASK_SENSOR;
        return (int8_t *)m_strSonsorManager[nEvent];
    }
    else
    if(nEvent >= eReqUpdate &&
       nEvent <= eMaxFotaManager )
    {
        // fota message
        nEvent ^= eMASK_FOTA;
        return (int8_t *)m_strFotaManager[nEvent];
    }

    return "NotDefined";
}

int8_t *GetStringFromAlramEvent(int32_t nId)
{
    if( nId >= eMESSAGE_EVENT_KEY_NONE &&
        nId <= eMESSAGE_EVENT_KEY_MAX )
        return (int8_t *)m_strAlramEvent[nId];

    return "NotDefined";
}


void DisplayReport(int8_t* pcarrTitle, stMsgSysMsg* pstMessage)
{
    stHalRTCTypeDef stDate;
	unsigned long int lValueH;
	unsigned long int lValueL;

    long long llValue = pstMessage->header.drivingKey;

    lValueH = llValue>>32;
    lValueL = llValue;

    Trace("#############################################################\r\n");
    Trace(" %s\r\n",pcarrTitle);
//    Trace(" %s\n",GetStringFromId(pstMessage->header.id));
//    Trace(" event : %s\n",GetStringFromEvent(eGetStringEvent,
//                                    pstMessage->header.event,
//                                    pstMessage->header.subEvent));
//    Trace(" subEvent : %s\n",GetStringFromEvent(eGetStringSubEvent,
//                                    pstMessage->header.event,
//                                    pstMessage->header.subEvent));
    if( pstMessage->header.subEvent == eR_Alram )
    {
        //Trace(" alram event : %s\n", GetStringFromAlramEvent(pstMessage->carReport.rpAlram.CarStatus.EventKey));
        Trace(" alram event : %d\r\n", pstMessage->carReport.rpAlram.CarStatus.EventKey);
    }

    GetDatefromTime2(&stDate, pstMessage->header.unEventTime);
    Trace(" occurred time : %04d/%02d/%02d,%02d:%02d:%02d\r\n",stDate.RtcDate.RTC_Year+2000,
                                                        stDate.RtcDate.RTC_Month,
                                                        stDate.RtcDate.RTC_Date,
                                                        stDate.RtcTime.RTC_Hours,
                                                        stDate.RtcTime.RTC_Minutes,
                                                        stDate.RtcTime.RTC_Seconds);

    Trace(" drivingKey : %04x%08x\r\n",lValueH,lValueL);
    if( pstMessage->header.subEvent == eR_DrivingInterval )
	{
		Trace(" B1.SequnceNumber : %d\r\n",pstMessage->carReport.rpInterval.DrivingInfo.DrivingInfoB1.SequnceNumber);
		Trace(" B2.SequnceNumber : %d\r\n",pstMessage->carReport.rpInterval.DrivingInfo.DrivingInfoB2.SequnceNumber);
	}
//        GetStringFromId((pstMessage->header.unTraceMng>>16)&0xFF),
//        GetStringFromId((pstMessage->header.unTraceMng>>8)&0xFF),
//        GetStringFromId(pstMessage->header.unTraceMng&0xFF));

    Trace("#############################################################\r\n");
}

boolean_t CheckVersionInfo()
{
    //stUpdateFileInfo *ptrFileInfo;
    //ptrFileInfo = &stUpdateFileInfoList;

    return true;
}

void SetUpdateFlag(boolean_t bUpdate)
{
    m_stMsgHdData.bNeedUpdate = bUpdate;
}

void SetRealPowerOffTime(uint32_t unUTCTime)
{
    //uint32_t unPowerOffTime = ConvertRTC2Seconds(stMessageHeader.stDate.time,stMessageHeader.stDate.date);
    //SetBackupRamConfigProperty(eBackupRamConfig_PowerOffTime,(void*)&unPowerOffTime);
    uint32_t unPowerOffTime = unUTCTime;
    SetBackupRamConfigProperty(eBackupRamConfig_PowerOffTime,(void*)&unPowerOffTime);
}

#ifdef ENABLE_FOTA_TEST
void RequestTestFota()
{
    m_stMsgHdData.ucIGOffCount = 0;
    m_stMsgHdData.bIGOff = false;

    // clear f/w db version/
    ClearAppVersion();

    Send2MngSysMsg2(eMngSys,eReqFota,eFwUpdate, eTrue, (stCarReport *)NULL,0);
}
#endif //#ifdef ENABLE_FOTA_TEST

bool SetFotaSetting(uint32_t unFotaInterval ,uint32_t unPowerOffTime)
{
    uint32_t unCurrentTime = GetUTCTime();

    Trace("Fota interval time : %d, Power off time : %d, Current time : %d\r\n",unFotaInterval,unPowerOffTime, unCurrentTime);

#ifdef ENABLE_FOTA_TEST
    RequestTestFota();
    return false;
#endif //#ifdef ENABLE_FOTA_TEST

	// not allow the fota if modem is deactivated
    if( GetModemActive() == false )
    {
        Trace("Not active modem, fota is not allowed\r\n");
        unPowerOffTime = 0;
        Send2MngSysMsg2(eMngSys,eReqFota,eFwUpdate, eFalse, (stCarReport *)NULL,0);
        return false;
    }

    // if first boot return send default value.
    if( unPowerOffTime == 0 )
    {
        Send2MngSysMsg2(eMngSys,eReqFota,eFwUpdate, eFalse, (stCarReport *)NULL,0);
        return false;
    }

    if( (unPowerOffTime + unFotaInterval) <= unCurrentTime )
    {
        Trace("Request fota update when module go to sleep\r\n");
        // reuqest update firmware from fota.
        Send2MngSysMsg2(eMngSys,eReqFota,eFwUpdate, eTrue, (stCarReport *)NULL,0);

        return true;
    }
    else
    {
        Trace("No Request fota update when module go to sleep\r\n");
        Send2MngSysMsg2(eMngSys,eReqFota,eFwUpdate, eFalse, (stCarReport *)NULL,0);
    }

    return false;
}

void PowerOnGemaltoModem()
{
	printf("PowerOnGemaltoModem_Modem_Reset_Power3\r\n");
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
//    HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
//    APP_Delay(200);

	/*MODEM RST CONTROL */
	HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);
	APP_Delay(200);
	HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
	APP_Delay(200);
	HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);

//	APP_Delay(200);
//  HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);

#else
    HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
    APP_Delay(10);
    HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_SET);
    APP_Delay(100);
    HalGPIOSetVaule(GPIO_POWER_KEY, eBIT_RESET);
#endif
}

void SetAutolinkConfig2System(stAutolinkConfigData* pstAutolinkConfigData)
{
    //stGeofenceUnit stCurrentUnit;
    stGeofenceUnit stSettingUnit;

    // send obd setting value to obd manager.
    SetObdSetting(pstAutolinkConfigData);

    if( pstAutolinkConfigData->stSystem.bModemActive == false )
    {
        Trace("modem transfer is not activated\r\n");
        Trace("Block to transer\r\n");
        // send no blocking an event to all manager
        SendBlockingMessage(true);
    }

    if( SetFotaSetting(BkSram_SystemInfo.unFotaInterval,BkSram_SystemInfo.unPowerOffTime) == true )
    {
        //clear power off tiem to prevent fota update
        BkSram_SystemInfo.unPowerOffTime = 0;
    }

    // setting geofence data from serial flash to variable of system.
    memcpy((char*)&m_stUserActionSetting,(char*)&pstAutolinkConfigData->stUserAction,sizeof(stUserActionSetting));
    // initialize genfece alram

   // stCurrentUnit.GpsLatitude = pstAutolinkConfigData->stSystem.dlLastLatitude;
   // stCurrentUnit.GpsLongitude = pstAutolinkConfigData->stSystem.dlLastLongitude;
    memset((char*)&stSettingUnit,0,sizeof(stGeofenceUnit));

    // not laram just initialize all setting value.
    // CheckGeoFence(stCurrentUnit,&stSettingUnit,false,false);

#ifdef USE_96_HOURS_POWER_OFF
    if( BkSram_SystemInfo.bModemPowerOff == true )
    {
        // ME need to modem wake up because ME is wake up by can and impulse
        // so ME should report this message
        PowerOnGemaltoModem();

        BkSram_SystemInfo.bModemPowerOff = false;
        BkSram_SystemInfo.usSoftwareResetCount = 0;

        // request send alram to system manager for wake up
        Send2MngSysMsg2(eMngSys,eReqModemPowerOff,0,eFalse,(stCarReport *)NULL,0);

        Trace("===================================================\r\n");
        Trace("Disable Modem Power Off : %d, Reset Count : %d\r\n",BkSram_SystemInfo.bModemPowerOff,BkSram_SystemInfo.usSoftwareResetCount);
    }
    // check system reset count
    // if system is not used by user during 96hours,
    // then ME turn off the modem until there is an event from user.
    if( BkSram_SystemInfo.usSoftwareResetCount++ >= (96*2) )
    {
        // user not used this car
        // Step#1 Power Off modem
        // Step#2 Send alram for power off the modem.
        BkSram_SystemInfo.bModemPowerOff = true;

        // request send alram to system manager for power off modem.
        Send2MngSysMsg2(eMngSys,eReqModemPowerOff,0,eTrue,(stCarReport *)NULL,0);

        Trace("===================================================\r\n");
        Trace("Enabled Modem Power Off : %d, Reset Count : %d\r\n",BkSram_SystemInfo.bModemPowerOff,BkSram_SystemInfo.usSoftwareResetCount);
    }

    Trace("System Reset Count : %d\r\n",BkSram_SystemInfo.usSoftwareResetCount);
#endif //#ifdef USE_96_HOURS_POWER_OFF

    // save cold booting
    if( AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON )
        pstAutolinkConfigData->stSystem.usSystemResetCount++;
}

#ifdef USE_96_HOURS_POWER_OFF
void ClearSystemResetCount()
{
    Trace("===================================================\r\n");
    Trace("Clear usSoftwareResetCount\r\n");
    BkSram_SystemInfo.bModemPowerOff = false;
    BkSram_SystemInfo.usSoftwareResetCount = 0;
}
#endif //#ifdef USE_96_HOURS_POWER_OFF

void GetAutolinkConfig2System(stAutolinkConfigData* pstAutolinkConfigData)
{
    // set iniiatlize value for reset
    //pstAutolinkConfigData->stSystem.unOdometer = m_stMsgHdData.stObdSettingValue.ObdSetting.unOdometer;
    //pstAutolinkConfigData->stSystem.dlLastLatitude = m_stMsgHdData.stObdSettingValue.ObdSetting.dlLatitue;
    //pstAutolinkConfigData->stSystem.dlLastLongitude= m_stMsgHdData.stObdSettingValue.ObdSetting.dlLongitude;
    pstAutolinkConfigData->stSystem.unOdometer = Get_Odmeter();
    pstAutolinkConfigData->stSystem.dlLastLatitude = Get_GPS_Lat_Origin();
    pstAutolinkConfigData->stSystem.dlLastLongitude= Get_GPS_Lon_Origin();
    pstAutolinkConfigData->stSystem.fRemainedFuel= m_stMsgHdData.stObdSettingValue.ObdSetting.fRemainedFuel;

    BkSram_SystemInfo.ucDoorLockStatus = m_stMsgHdData.stObdSettingValue.ObdSetting.ucDoorLockStatus;
    BkSram_SystemInfo.ucDoorOpenStatus = m_stMsgHdData.stObdSettingValue.ObdSetting.ucDoorOpenStatus;
    BkSram_SystemInfo.ucHeadLight = m_stMsgHdData.stObdSettingValue.ObdSetting.ucHeadLight;

    Trace("sys:odometer:%d,lat:%f,lon:%f\r\n",pstAutolinkConfigData->stSystem.unOdometer,
        pstAutolinkConfigData->stSystem.dlLastLatitude,
        pstAutolinkConfigData->stSystem.dlLastLongitude);

    //pstAutolinkConfigData->bModemActive = GetModemActive();
    //pstAutolinkConfigData->dlLastLatitude = Get_GPS_Lat_Origin();
    //pstAutolinkConfigData->dlLastLongitude = Get_GPS_Lon_Origin();
    //pstAutolinkConfigData->unOdometer = Get_Odmeter();

    // coyp user action setting value to configuration vaule.
    memcpy((char*)&pstAutolinkConfigData->stUserAction,(char*)&m_stUserActionSetting,sizeof(stUserActionSetting));
}


void SetPhoneNubmer2AutolinkConfig(char* buffer, int32_t size)
{
    SetAutolinkConfigProperty(eAutoLinkConfig_CellPhone,(void*)buffer);
}

void HandlerNewVin(char* carrNewVin)
{
#ifndef ENABLE_DEV_VIN
    char carrSystemVin[64]={0};

    if( IsAvailableVin(carrNewVin) == false )
    {
        printf("IMPORTANT : new vin number is broken, do nothing\r\n");
        return;
    }

    GetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrSystemVin);

    // check default vin or not
    if( memcmp(carrSystemVin, STR_DEFAULT_VIN, strlen(STR_DEFAULT_VIN)) == 0 )
    {
        // received new vin / we can allow the vin
        printf("IMPORTANT : default vin (\"%s\") is changed with \"%s\"\r\n",carrSystemVin,carrNewVin);

        // send no blocking an event to all manager
        SendBlockingMessage(false);

        memset(carrSystemVin,0,sizeof(carrSystemVin));
        //save new vin to serail flash
        memcpy(carrSystemVin,carrNewVin,MAX_REAL_VIN_SIZE);

        SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrSystemVin);
    }
    else
    {
        if( IsAvailableVin(carrSystemVin) == false )
        {
            printf("IMPORTANT : vin number is broken, set the default vin\r/n");
            memset(carrSystemVin,0,sizeof(carrSystemVin));
            //memcpy((char *)carrSystemVin, (char *)g_FirmwareInfo.m_strVIN, strlen((char *)g_FirmwareInfo.m_strVIN));
            memcpy((char *)carrSystemVin, (char *)g_FirmwareInfo.m_strVIN, MAX_REAL_VIN_SIZE);
//            memcpy(g_FirmwareInfo.m_strVIN,carrSystemVin, sizeof(carrSystemVin));

            SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrSystemVin);
        }

        uint8_t cAllowNewVin;

        GetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,(void*)&cAllowNewVin);

        if( cAllowNewVin == 0 )
        {
            uint8_t bModemActive = false;

            if( memcmp(carrNewVin,carrSystemVin,MAX_REAL_VIN_SIZE) == 0 )
            {
                Trace("=================================================================\r\n");
                Trace("New Vin(%s) is same vin that stored into the system.\r\n",carrNewVin);
                Trace("System sends an event to allow to transfer the data.\r\n");

                // send no blocking an event to all manager
                SendBlockingMessage(false);

                //enable modem
                bModemActive = true;
            }
#ifdef FIX_VIN
			bModemActive = true;
#else
            else
            {
                Trace("=================================================================\r\n");
                Trace("New Vin(%s) is not allowed to system.\r\n",carrNewVin);
                Trace("This is policy during development.\r\n");
                Trace("Transfer is blocked.\r\n");
                Trace("If new policy about new vin, we should implement the scenario.\r\n");

                // send blocking an event to all manager
                SendBlockingMessage(true);

                //disable modem
                bModemActive = false;
            }
#endif	//FIX_VIN
            // save properties
            SetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);
        }
        else if( cAllowNewVin == 1 )
        {
            Trace("=================================================================\r\n");
            Trace("New Vin(%s) is allowed to system.\r\n",carrNewVin);
            Trace("This is policy during development.\r\n");
            Trace("System sends an event to allow to transfer the data.\r\n");

            SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrNewVin);
        }
    }
#endif //#ifndef ENABLE_DEV_VIN

}

void SetRTCTimewithNetworkTime(stHalRTC_DateTypeDef* newDate, stHalRTC_TimeTypeDef* newTime)
{
    stHalRTCTypeDef stHalRtcDateTime;
    
    memcpy((char*)&stHalRtcDateTime.RtcTime, newTime, sizeof(stHalRTC_TimeTypeDef));
    memcpy((char*)&stHalRtcDateTime.RtcDate, newDate, sizeof(stHalRTC_DateTypeDef));
    
    HalDrvRtcWrite(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRTCTypeDef), 0);

    APP_TimeShow(&stHalRtcDateTime);

}

void SetNetworkTime2AutolinkConfig(stNetworkTime stNetworkDate)
{
    // this network time is local time
    stHalRTCTypeDef stDate;
//    struct tm* tmLocalTime;

//    tmLocalTime = localtime((time_t*)&stNetworkDate.unNetWrokTime);

    GetDatefromTime2(&stDate, stNetworkDate.unNetWrokTime);

    // setting network time to RTC register
    SetRTCTimewithNetworkTime(&stDate.RtcDate,&stDate.RtcTime);

    // save network info for time zone
    //SetBackupRamConfigProperty(eBackupRamConfig_NetworkInfo,(void*)&stNetworkDate);
    SetAutolinkConfigProperty(eAutoLinkConfig_NetworkInfo,(void*)&stNetworkDate);

    Trace("New Network Time\r\n");
    Trace("%04d-%02d-%02d(%d)\r\n",2000 + stDate.RtcDate.RTC_Year,
        stDate.RtcDate.RTC_Month,
        stDate.RtcDate.RTC_Date,
        stDate.RtcDate.RTC_WeekDay);
    Trace("%0.2d:%0.2d:%0.2d\r\n",stDate.RtcTime.RTC_Hours,
        stDate.RtcTime.RTC_Minutes,
        stDate.RtcTime.RTC_Seconds);
}

void ShowSmartkeyAction(stCarReport report)
{
    // do something from server
    Trace("Action\r\n");
    Trace("CommandType : %d\r\n",report.rpSmartKey.Request.CommandType);
    Trace("ControlType : %d\r\n",report.rpSmartKey.Request.ControlType);
    Trace("Defrost : %d\n",report.rpSmartKey.Request.Defrost);
    Trace("Temperature : %04X\r\n",report.rpSmartKey.Request.Temperature);
	Trace("CheckTemperature : %02X\r\n",report.rpSmartKey.Request.CheckTemperature);
    Trace("Latitude : %f\r\n",report.rpSmartKey.Request.Latitude);
    Trace("Longitude : %f\r\n",report.rpSmartKey.Request.Longitude);
    Trace("BoundType : %d\r\n",report.rpSmartKey.Request.BoundType);
    Trace("Distance : %d\r\n",report.rpSmartKey.Request.Distance);
    Trace("KeepPowerOnTime : %d\r\n",report.rpSmartKey.Request.KeepPowerOnTime);
    //Trace("guid : %s\n",report.rpSmartKey.Request.Guid);
    Trace("Date Time : %d\r\n",report.rpSmartKey.Request.OccurredEventTime);
}

void DisplayUserActionSetting(stUserActionSetting stUserActionSettingValue)
{
    Trace("#####################################################\r\n");
    Trace("User Action Setting\r\n");
    Trace("Guard Active : %d\r\n",stUserActionSettingValue.Guard.bActive);
    Trace("Sensitivity High : %d\r\n",stUserActionSettingValue.Guard.stImpulseSetting.ucHigh);
    Trace("Sensitivity Middle : %d\r\n",stUserActionSettingValue.Guard.stImpulseSetting.ucHigh);
    Trace("Sensitivity Low : %d\r\n",stUserActionSettingValue.Guard.stImpulseSetting.ucHigh);
    Trace("Valet Active : %d, Distance : %d\r\n",stUserActionSettingValue.Valet.bActive,
        stUserActionSettingValue.Valet.nDistance);
    Trace("Valet Gps Latitude : %f, Logitude : %f\r\n",stUserActionSettingValue.Valet.CurGpsPosition.GpsLatitude,
        stUserActionSettingValue.Valet.CurGpsPosition.GpsLongitude);
    Trace("Towing Active : %d\r\n",stUserActionSettingValue.Towing.bActive);
    Trace("Data Active : %d\r\n",stUserActionSettingValue.ActiveModem.bActive);

    // DisplayGeofenceSetting(/*stUserActionSettingValue.Geofence*/);
}

void DisplayUserActionSetting2()
{
    Trace("#####################################################\r\n");
    Trace("User Action Setting\r\n");
    Trace("Guard Active : %d\r\n",m_stUserActionSetting.Guard.bActive);
    Trace("Sensitivity High : %d\r\n",m_stUserActionSetting.Guard.stImpulseSetting.ucHigh);
    Trace("Sensitivity Middle : %d\r\n",m_stUserActionSetting.Guard.stImpulseSetting.ucHigh);
    Trace("Sensitivity Low : %d\r\n",m_stUserActionSetting.Guard.stImpulseSetting.ucHigh);
    Trace("Valet Active : %d, Distance : %d\r\n",m_stUserActionSetting.Valet.bActive,
        m_stUserActionSetting.Valet.nDistance);
    Trace("Valet Gps Latitude : %f, Logitude : %f\r\n",m_stUserActionSetting.Valet.CurGpsPosition.GpsLatitude,
        m_stUserActionSetting.Valet.CurGpsPosition.GpsLongitude);
    Trace("Towing Active : %d\r\n",m_stUserActionSetting.Towing.bActive);
    Trace("Data Active : %d\r\n",m_stUserActionSetting.ActiveModem.bActive);

    // DisplayGeofenceSetting(/*m_stUserActionSetting.Geofence*/);
}

bool SystemDelayProcess(unsigned long* nBaseTime, int nDelay)
{
    // MONI modem rx Skip Process
    if( *nBaseTime == 0 )
    {
        *nBaseTime  = Get_Tmr();
    }

    if( Get_TmrDelta(Get_Tmr(), *nBaseTime) < nDelay )
        return false;

    *nBaseTime = 0;

    return true;
}

int ResponseSmartKey(int iResult, int iReason,stMsgSysMsg* pstMsgSysMsg)
{
    stMsgMdm msg;
    memset((char*)&msg,0,sizeof(stMsgMdm));

    memcpy((char*)&pstMsgSysMsg->header,(char*)&msg.header,sizeof(msg.header));

    msg.header.id = eMngSysMsg;
    msg.header.event = eRspReport;
    msg.header.subEvent = eR_RspSmartKey;

    memcpy((char*)msg.carReport.rpSmartKey.Response.Guid,
           (char*)pstMsgSysMsg->carReport.rpSmartKey.Request.Guid,MAX_GUID_LENGTH);

    msg.carReport.rpSmartKey.Response.Result = iResult;
    msg.carReport.rpSmartKey.Response.Reason = iReason;
    msg.carReport.rpSmartKey.Response.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.carReport.rpSmartKey.Response.OccurredEventUtcTime = GetUTCTime();

    Send2MngModem2(&msg);
    return 0;
}

int ResponseUserSetting(int iResult, int iReason, stMsgSysMsg* pstMsgSysMsg)
{
    stMsgMdm msg;
    memset((char*)&msg,0,sizeof(stMsgMdm));

    memcpy((char*)&pstMsgSysMsg->header,(char*)&msg.header,sizeof(msg.header));

    msg.header.id = eMngSysMsg;
    msg.header.event = eRspReport;

    if( pstMsgSysMsg->carReport.rpSetting.UserSetting.ucCommandType == eREMOTE_CON_CMD_TYPE_SETTING_RSV_ENG_CTRL )
    {
    	msg.header.subEvent = eR_RspSettingRsvEngCtrl;
    	memcpy(&msg.carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl,&pstMsgSysMsg->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl ,sizeof(msg.carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl));

		msg.carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.Result = iResult;
		msg.carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.Reason = iReason;
		msg.carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.unEventTime = GetLocalTimefromTime(GetUTCTime());
	}
    else
    {
    	msg.header.subEvent = eR_RspSmartKey;

		memcpy((char*)msg.carReport.rpSmartKey.Response.Guid,
		   (char*)pstMsgSysMsg->carReport.rpSetting.UserSetting.RequestGUID,MAX_GUID_LENGTH);

		msg.carReport.rpSmartKey.Response.Result = iResult;
		msg.carReport.rpSmartKey.Response.Reason = iReason;
		msg.carReport.rpSmartKey.Response.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
		msg.carReport.rpSmartKey.Response.OccurredEventUtcTime = GetUTCTime();
		msg.carReport.rpSmartKey.Response.ucSysSmartkeyReqType = eSysSmartkeyReqType_Modem;

		memset(msg.carReport.rpSmartKey.Response.BTControlKey,0x20,MAX_BT_CONTROLKEY_LENGTH);
	}

    Send2MngModem2(&msg);
    return 0;
}


void SetNewNetworkTime()
{
}

void ClearAppVersion()
{
    // Bootloader ï¿½ï¿½ï¿½ï¿½aï¿½ï¿½ï¿½ï¿½ï¿½ï¿½i Aï¿½Ë?¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ë¡ï¿½ ï¿½ï¿½uï¿½Ï©ï¿½Aï¿½Ë?¿½
    g_FirmwareInfo.AppProperty[eApp_Bootloader].nSignal = (int)FIRMWARE_APP_SIGNAL;
    g_FirmwareInfo.AppProperty[eApp_Bootloader].wFirmwareSize = 100;//FIRMWARE_INFO_ADDRESS-BOOTLOADER_ADDRESS;
    g_FirmwareInfo.AppProperty[eApp_Bootloader].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
    memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Bootloader].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
    g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion = DEFAULT_FW_VERION;

    // /APP ï¿½ï¿½ï¿½ï¿½aï¿½ï¿½ï¿½ï¿½ï¿½ï¿½i Aï¿½Ë?¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ë¡ï¿½ ï¿½ï¿½uï¿½Ï©ï¿½Aï¿½Ë?¿½
    g_FirmwareInfo.AppProperty[eApp_Application].nSignal = (int)FIRMWARE_APP_SIGNAL;
    g_FirmwareInfo.AppProperty[eApp_Application].wFirmwareSize = 100;//UPDATE_TMP_ADDRESS - APPLICATION_ADDRESS;
    g_FirmwareInfo.AppProperty[eApp_Application].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
    memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Application].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
    g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion = (int)DEFAULT_FW_VERION;

    // Master DB ï¿½ï¿½ï¿½ï¿½aï¿½ï¿½ï¿½ï¿½ï¿½ï¿½i Aï¿½Ë?¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ë¡ï¿½ ï¿½ï¿½uï¿½Ï©ï¿½Aï¿½Ë?¿½
    g_FirmwareInfo.AppProperty[eApp_MasterDB].nSignal = (int)FIRMWARE_APP_SIGNAL;
    g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
    g_FirmwareInfo.AppProperty[eApp_MasterDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
    memcpy((char*)g_FirmwareInfo.AppProperty[eApp_MasterDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
    g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion = (int)DEFAULT_FW_VERION;

    // Slave DB ï¿½ï¿½ï¿½ï¿½aï¿½ï¿½ï¿½ï¿½ï¿½ï¿½i Aï¿½Ë?¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ë¡ï¿½ ï¿½ï¿½uï¿½Ï©ï¿½Aï¿½Ë?¿½
    g_FirmwareInfo.AppProperty[eApp_SlaveDB].nSignal = (int)FIRMWARE_APP_SIGNAL;
    g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize = 100;//APPLICATION_ADDRESS-CAR_SLAVE_ADDRESS;
    g_FirmwareInfo.AppProperty[eApp_SlaveDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
    memcpy((char*)g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
    g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion = (int)DEFAULT_FW_VERION;

    // Driving DB ï¿½ï¿½ï¿½ï¿½aï¿½ï¿½ï¿½ï¿½ï¿½ï¿½i Aï¿½Ë?¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ë¡ï¿½ ï¿½ï¿½uï¿½Ï©ï¿½Aï¿½Ë?¿½
    g_FirmwareInfo.AppProperty[eApp_ControlDB].nSignal = (int)FIRMWARE_APP_SIGNAL;
    g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
    g_FirmwareInfo.AppProperty[eApp_ControlDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
    memcpy((char*)g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
    g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion = (int)DEFAULT_FW_VERION;

    SetFirmwareInfo(&g_FirmwareInfo);
}

#if defined (GLOBAL_SHARE_QUEUE)
void ConvertShareQueue2StaticQueueMessage(uint16_t usQueueId, uint8_t* pcDstHeader, uint32_t unHeaderLength, uint8_t* pcDstBody, uint32_t unBodyLength, stSysHdQueueData* pstMessageShareQueue)
{
#ifdef ENABLE_LOG_SHARE_QUEUE
	printf("\t\tSysHdQueue] deallocation id : %d, index : %d\n",usQueueId,pstMessageShareQueue->sBufferIndex);
#endif
	memcpy(pcDstHeader,&pstMessageShareQueue->header,unHeaderLength);
	if( pstMessageShareQueue->sBufferIndex != -1 )
	{
		uint8_t* pBody = (uint8_t*)GetMessageQueueBuffer(pstMessageShareQueue->sBufferIndex);
		if( pBody != NULL )
		{
			memcpy(pcDstBody,pBody,unBodyLength);
		}
		else
		{
			printf("\t\tSysHdQueue] error pBody is Null, deallocation id : %d, index : %d\n",usQueueId,pstMessageShareQueue->sBufferIndex);
		}
		
		ClearMessageShareQueueIndex(pstMessageShareQueue->sBufferIndex);
	}
}

void SendSysHdShareQueueMessage(uint16_t usId, uint8_t* pSrcHeader, uint32_t unSrcHeaderLength, uint8_t* pSrcBody, uint32_t unSrcBodyLength)
{
	stSysHdQueueData stMessageShareQueue;
	// copy header
	memcpy(&stMessageShareQueue.header, pSrcHeader, unSrcHeaderLength);
	// init buffer index
	stMessageShareQueue.sBufferIndex = -1;
	// copy body
	if( pSrcBody != NULL )
	{
		uint8_t* pBody;
		if( GetMessageQueueIndex((int16_t*)&stMessageShareQueue.sBufferIndex,(uint8_t**)&pBody) == true )
		{
        	memcpy((int8_t*)pBody, pSrcBody, unSrcBodyLength);
		}
		else
		{
			printf("\t\tSysHdQueue] error buffer is full, allocation id : %d, index : %d\n",usId,stMessageShareQueue.sBufferIndex);
		}
	}
#ifdef ENABLE_LOG_SHARE_QUEUE
	printf("\t\tSysHdQueue] allocation id : %d, index : %d\n",usId,stMessageShareQueue.sBufferIndex);
#endif
	if( SendSysHdQueueMessage(usId, (int8_t*)&stMessageShareQueue, sizeof(stSysHdQueueData)) == false )
	{
		ClearMessageShareQueueIndex(stMessageShareQueue.sBufferIndex);
	}
}


boolean_t GetSysHdShareQueueMessage(uint16_t usId, uint8_t* pDstHeader, uint32_t unDstHeaderLength, uint8_t* pDstBody, uint32_t unDstBodyLength)
{
	stSysHdQueueData stMessageShareQueue;
	// check external event
    if( GetSysHdQueueMessage(usId,(int8_t*)&stMessageShareQueue, sizeof(stSysHdQueueData)) == true )
    {
		ConvertShareQueue2StaticQueueMessage(usId, (uint8_t*)pDstHeader, unDstHeaderLength, (uint8_t*)pDstBody, unDstBodyLength, &stMessageShareQueue);

		return true;
    }

	return false;
}
#endif


