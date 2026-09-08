/* Includes ------------------------------------------------------------------*/
#include "GIT_Util.h"

#include "MngSystem.h"
#include "MngQueue.h"
#include "HdDebug.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM_HADNLER,__VA_ARGS__)

typedef int (*fnpSysHander)(int,int);

void SensorEvtHandler(int lparam, int rparam);
void SetMngSensorState(int state);
void HandleMngSysSensorExternalEvent(stMsgSysSensor* pstReport);

enum{
    STAT_SYS_MNG_HD_SENSOR_INIT = 0,
    STAT_SYS_MNG_HD_SENSOR_SLEEP,
    STAT_SYS_MNG_HD_SENSOR_SLEEP_WAIT,
    STAT_SYS_MNG_HD_SENSOR_IDLE
};

int m_iMngSensorState = STAT_SYS_MNG_HD_SENSOR_INIT;

// internal variabls
int m_iMngSensorEvtOccured = -1;

stMsgSysSensor m_stMsgSysSensor;

bool m_bSysSensorFirst = true;

int HdMngSysSensor(int lparam,int rparam)
{
    int ret = -1;
    //printf("%s]Enter\n",__FUNCTION__);
    SensorEvtHandler(lparam,rparam);

    // check external event

#ifndef GLOBAL_SHARE_QUEUE //Get
    if( MngQueueGetMessage(ID_MNG_QUEUE_SYS_SENSOR, (int8_t*)&m_stMsgSysSensor, sizeof(m_stMsgSysSensor) ) == true )
#else
    if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_SYS_SENSOR, (uint8_t*)&m_stMsgSysSensor.header, sizeof(stMsgHeader), (uint8_t*)&m_stMsgSysSensor.carReport, sizeof(stCarReport)) == true )
#endif
    {

#if defined(MNG_QUEUE_DEBUG)
        Trace("%s]Get] buffer[0]:%d,buffer[1]:%d\r\n",__FUNCTION__,buffer[0],buffer[1]);
#endif
        // handle external event
        HandleMngSysSensorExternalEvent(&m_stMsgSysSensor);
        Trace("%s]Get] event[0]:%d,subEvent[1]:%d\r\n",__FUNCTION__,m_stMsgSysSensor.header.event,m_stMsgSysSensor.header.subEvent);

    }

    switch(m_iMngSensorState)
    {
        case STAT_SYS_MNG_HD_SENSOR_INIT:
            Trace("%s] state : STAT_SYS_MNG_HD_SENSOR_INIT\r\n",__FUNCTION__);
            SetMngSensorState(STAT_SYS_MNG_HD_SENSOR_IDLE);
            break;
        case STAT_SYS_MNG_HD_SENSOR_IDLE:
            //Trace("%s] state : STAT_SYS_MNG_HD_SENSOR_IDLE\r\n",__FUNCTION__);
            break;
        case STAT_SYS_MNG_HD_SENSOR_SLEEP:
            //Trace("%s] state : STAT_SYS_MNG_HD_SENSOR_SLEEP\r\n",__FUNCTION__);

            if( m_bSysSensorFirst == true )
            {
                ret = eRspSleep;
                m_bSysSensorFirst = false;
            }

            break;
        case STAT_SYS_MNG_HD_SENSOR_SLEEP_WAIT:
            //Trace("%s] state : STAT_SYS_MNG_HD_SENSOR_SLEEP_WAIT\r\n",__FUNCTION__);
            // 1. request sleep to sensor mananger and wait for response
            // 2. if recevied sleep from sensor mananger, change state form sleep wait to sleep


            //SetMngSensorState(STAT_SYS_MNG_HD_SENSOR_SLEEP);
            break;
        default:
            // not define
            // error
            Trace("%s] error : default\r\n",__FUNCTION__);
            break;
    }

    return ret;
}

void SensorEvtHandler(int lparam, int rparam)
{
    // process with lparam, rparam
    if( lparam == eMngSys )
    {
        if( rparam == eReqSleep )
        {
            Send2MngSensor(eMngSysSensor, eReqSleep, 0, (stCarReport *)NULL,0);

#if defined(MNG_QUEUE_DEBUG)
            Trace("%s]Send] buffer[0]:%d,buffer[1]:%d\r\n",__FUNCTION__,buffer[0],buffer[1]);
#endif
            // this event occured from system
            // this handler progress this schedule
            // 1. backup data
            // send event to sensor mananger
            SetMngSensorState(STAT_SYS_MNG_HD_SENSOR_SLEEP_WAIT);

            Trace("%s]Rcv]Sleep Evt from System Manager\r\n",__FUNCTION__);
        }
        else if( rparam == eReqChangeMode )
        {
            SetMngSensorState(STAT_SYS_MNG_HD_SENSOR_IDLE);
        }
    }
    else
    {
        // not defined
        //Trace("%s] if( lparam == eSysMngIntEvtSleep ) not defined\r\n",__FUNCTION__);
    }
}

void SetMngSensorState(int state)
{
    if( state >= STAT_SYS_MNG_HD_SENSOR_INIT &&
        state <= STAT_SYS_MNG_HD_SENSOR_IDLE )
    {
        m_iMngSensorState = state;
    }
    else
    {
        // error state not exist
        Trace("%s] error : not defined event)\r\n",__FUNCTION__);
    }
}

void HandleMngSysSensorExternalEvent(stMsgSysSensor* pstReport)
{
    if( pstReport->header.id == eMngSensor )
    {
        if( pstReport->header.event == eRspSleep )
        {
            // if other manager wants to go sleep, this mananger request upper manager
            // and wait for event from upper manager.
            SetMngSensorState(STAT_SYS_MNG_HD_SENSOR_SLEEP);

            Trace("%s] Get sleep event\r\n",__FUNCTION__);
        }
    }
    else
    {
        // not define
        // error
        Trace("%s] error : not defined\r\n",__FUNCTION__);
    }
}
