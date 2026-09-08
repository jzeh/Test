/* Includes ------------------------------------------------------------------*/
#include "GIT_Util.h"

#include "MngSystem.h"
#include "MngQueue.h"
#include "HdDebug.h"
#include "OBD_Controller.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)

// system state
enum{
    STAT_SYS_MNG_HD_INIT = 0,
    STAT_SYS_MNG_HD_SLEEP,
    STAT_SYS_MNG_HD_SLEEP_WAIT,
    STAT_SYS_MNG_HD_IDLE,
};

// state variable
int m_iSysMngHdState = STAT_SYS_MNG_HD_INIT;

// internal variabls
int m_iHdMngSysEvtOccured = -1;

// function definitions
void SetMngSysHdState(int iState);
int GetExternalEvent();
void HandleSubMngSysExternalEvent(stMsgSysSub* pstMsgSysSub);
int SysEvtSubHandler(int lparam, int rparam);

stMsgSysSub m_stMsgSysSub;

bool m_bSubSysFirst = true;

bool m_bStartAutoSaveReport = false;
int m_nAutoSaveCount = 0;
int m_nReportCount = 0;

// system mananger
int HdMngSys(int lparam,int rparam)
{
    int ret = -1;

    //printf("%s]Enter\r\n",__FUNCTION__);
    SysEvtSubHandler(lparam,rparam);

    // check external event
#ifndef GLOBAL_SHARE_QUEUE //Get
    if( MngQueueGetMessage(ID_MNG_QUEUE_SYS_SUB, (int8_t*)&m_stMsgSysSub, sizeof(stMsgSysSub)) == true )
#else
	if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_SYS_SUB, (uint8_t*)&m_stMsgSysSub.header, sizeof(stMsgHeader), (uint8_t*)&m_stMsgSysSub.carReport, sizeof(stCarReport)) == true )
#endif
    {

#if defined(MNG_QUEUE_DEBUG)
        Trace("%s]Get] buffer[0]:%d,buffer[1]:%d\r\n",__FUNCTION__,buffer[0],buffer[1]);
#endif
        // handle external event
        HandleSubMngSysExternalEvent(&m_stMsgSysSub);
    }

    switch(m_iSysMngHdState)
    {
        case STAT_SYS_MNG_HD_INIT:
            SetMngSysHdState(STAT_SYS_MNG_HD_IDLE);
            break;
        case STAT_SYS_MNG_HD_SLEEP:
            // response to sys manager and wait reset
            if( m_bSubSysFirst == true )
            {
                ret = eRspSleep;
                m_bSubSysFirst = false;
            }
            break;
        case STAT_SYS_MNG_HD_SLEEP_WAIT:
            // backup routine process
            // finish the backup process and then change state to sleep.
            SetMngSysHdState(STAT_SYS_MNG_HD_SLEEP);
            break;
        case STAT_SYS_MNG_HD_IDLE:
            // wait for other event
            if( m_bStartAutoSaveReport == true )
            {
                stMsgSysMsg stReport;
                memset((char*)&stReport,0,sizeof(stMsgSysMsg));
                stReport.header.id = eMngObd;
                stReport.header.event = eRspReport;
                stReport.header.subEvent = eR_DrivingInterval;
                stReport.carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLatitude = 9.999;
                stReport.carReport.rpInterval.DrivingInfo.DrivingInfoB1.GpsLongitude = 99.999;
//                GetObdDrivingKey((long long *)&stReport.header.drivingKey);

                Send2MngSysMsg3(&stReport);

                if( m_nReportCount++ == 60 )
                {
                    m_bStartAutoSaveReport = false;
                    m_nReportCount=0;

                    Trace("############################\r\n");
                    Trace("AutoSave Stop\r\n");

                    Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerConnected, (stCarReport *)NULL,0);
                }
            }


            break;
        default:
            // not defined
            // error
            break;
    }

    if( m_iHdMngSysEvtOccured  != -1 )
    {
        ret = m_iHdMngSysEvtOccured ;
        m_iHdMngSysEvtOccured = -1;
    }

    return ret;
}

int SysEvtSubHandler(int lparam, int rparam)
{
    int ret=0;

    // process with lparam, rparam
    if( lparam == eMngSys )
    {
        if( rparam == eReqSleep )
        {
            // this event occured from system
            // this handler progress this schedule
            // 1. backup data
            m_iSysMngHdState = STAT_SYS_MNG_HD_SLEEP_WAIT;

            Trace("%s]Rcv]Sleep Evt from System Manager\r\n",__FUNCTION__);
        }
    }
    else
    {
        // not defined
    }

    return ret;
}

void SetMngSysHdState(int iState)
{
    if( iState >= STAT_SYS_MNG_HD_INIT &&
        iState <= STAT_SYS_MNG_HD_IDLE )
    {
        m_iSysMngHdState = iState;
        m_iHdMngSysEvtOccured  = 1;
    }
    else
    {
        // error state not exist

    }
}



void HandleSubMngSysExternalEvent(stMsgSysSub* pstMsgSysSub)
{
    if( pstMsgSysSub->header.id == eMngSys )
    {
        if( pstMsgSysSub->header.event == eReqSaveReport )
        {
            m_bStartAutoSaveReport = true;
            m_nReportCount = 0;
            Making_Drivingkey();

            Send2MngSysMsg(eMngModem,eMdmStatus,eMS_ServerDisconnected, (stCarReport *)NULL,0);
        }
        if( pstMsgSysSub->header.event == eRspSaveReport )
        {
            m_bStartAutoSaveReport = false;
        }
    }
    #if 0
    if( lparam == eSysSubSleep )
    {
        // if other manager wants to go sleep, this mananger request upper manager
        // and wait for event from upper manager.
        SetMngSysHdState(STAT_SYS_MNG_HD_SLEEP);
    }
    else
    {
        // not defined
        // error
    }
    #endif
}

