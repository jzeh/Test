/* Includes ------------------------------------------------------------------*/
#include "AutolinkMessage.h"

#include "HdDebug.h"
#include "Message_make.h"

#include <math.h>

// user action setting variable
extern stUserActionSetting m_stUserActionSetting;
extern boolean_t IsIntheBoundary(stGeofenceUnit stSettingUnit, stGeofenceUnit stCurrentUnit);
extern uint32_t GetUTCTime();

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)

//#define ENABLE_VALET_LOG

extern stMngSysData m_MngSysData;

boolean_t m_bAlreadyValetReport = false;

void SetActiveValetMode(boolean_t bActive)
{
    Trace("Valet Active : %d\r\n",bActive);
    m_stUserActionSetting.Valet.bActive = bActive;

    if( bActive == true )
    {
        uint32_t unTime = GetUTCTime();
        uint32_t unDistance = 2*1000; //2km
#ifdef ENABLE_VALET_LOG
        DisplayTime("Valet Active Time : ",unTime);
#endif //#ifdef ENABLE_VALET_LOG
        m_stUserActionSetting.Valet.nDistance = unDistance;
        m_stUserActionSetting.Valet.unActiveTime = unTime;
        m_stUserActionSetting.Valet.unKeepTime = 30*60; // 30 minutes

        // for test we use hyundai locate position in australia
        // latitude : -33.785514, longitude : 151.127215
        m_stUserActionSetting.Valet.CurGpsPosition.GpsLatitude = Get_GPS_Lat();
        m_stUserActionSetting.Valet.CurGpsPosition.GpsLongitude = Get_GPS_Lon();
        m_stUserActionSetting.Valet.CurGpsPosition.bActive = true;
        m_stUserActionSetting.Valet.CurGpsPosition.bAlreadyAlramed = false;
        m_stUserActionSetting.Valet.CurGpsPosition.bInAlreadyAlramed = false;
        m_stUserActionSetting.Valet.CurGpsPosition.bOutAlreadyAlramed = false;
        m_stUserActionSetting.Valet.CurGpsPosition.cFenceType = eREMOTE_CON_BOUNDTYPE_OUT;
        m_stUserActionSetting.Valet.CurGpsPosition.unDistance = unDistance;

//#warning "When valet is actived, do we need keep alive????"
        // request pospond flag to system
        //SetPostPoneSleepFlag(true);
    }

    // reinitialzie exceptional case flag
    m_bAlreadyValetReport = false;

    // save setting value
    WriteConfig(false,true);
}

int32_t CheckValet(stGeofenceUnit* pstCurrentUnit, stGeofenceUnit* pstSettingUnit)
{
    stGeofenceUnit stCurrentUnit;
    stGeofenceUnit stSettingUnit;
    stValetSetting* pValet = &m_stUserActionSetting.Valet;

    uint32_t unUTCTime = GetUTCTime();
    uint32_t unActiveTime = pValet->unActiveTime;

    stCurrentUnit.GpsLatitude = Get_GPS_Lat();
    stCurrentUnit.GpsLongitude = Get_GPS_Lon();

    memcpy((char*)&stSettingUnit,(char*)&pValet->CurGpsPosition,sizeof(stGeofenceUnit));

#ifdef ENABLE_VALET_LOG
    DisplayTime("Valet Active Time : ",unActiveTime);
    DisplayTime("Valet Check Time : ",unUTCTime);

    Trace("Valet] Active  Time : %d\r\n",unActiveTime);
    Trace("Valet] Current Time : %d\r\n",unUTCTime);
    Trace("Valet] Keep Time : %d\r\n", pValet->unKeepTime);
    Trace("Valet] Different Time : %d\r\n", unUTCTime - unActiveTime);
#endif //#ifdef ENABLE_VALET_LOG

    if( (unUTCTime - unActiveTime) > pValet->unKeepTime )
    {
        // exceptional case because valet check time 1 minutes it is long time
        // so before valet distance is checked, if time out is occurred, then valte report will not sent
        if( m_bAlreadyValetReport == false )
        {
            m_bAlreadyValetReport = true;

            // check out boundary
            if( IsIntheBoundary(stSettingUnit,stCurrentUnit) == false )
            {
                // behicle is not in the boundary
                // notify to user
                memcpy((char*)pstCurrentUnit,(char*)&stCurrentUnit,sizeof(stGeofenceUnit));
                memcpy((char*)pstSettingUnit,(char*)&stSettingUnit,sizeof(stGeofenceUnit));

                return eValueDistance;
            }
        }
        // keep time is expired
        Trace("Valet] time expired / initialize setting\r\n");
        memset((char*)pValet,0,sizeof(stValetSetting));
        m_bAlreadyValetReport = false;

        // save adjust setting
        WriteConfig(false, false);

        // ME report valet alram for car that didn't power off.
        if( m_MngSysData.bPowerOff == false )
        {
            memcpy((char*)pstCurrentUnit,(char*)&stCurrentUnit,sizeof(stGeofenceUnit));
            memcpy((char*)pstSettingUnit,(char*)&stSettingUnit,sizeof(stGeofenceUnit));

            return eValuePoweron;
        }

        return eValetNone;
    }

    // check out boundary
    if( IsIntheBoundary(stSettingUnit,stCurrentUnit) == false )
    {
        // behicle is not in the boundary
        // notify to user
        memcpy((char*)pstCurrentUnit,(char*)&stCurrentUnit,sizeof(stGeofenceUnit));
        memcpy((char*)pstSettingUnit,(char*)&stSettingUnit,sizeof(stGeofenceUnit));

        return eValueDistance;
    }

    return eValetNone;
}

boolean_t GetValetActive()
{
    return m_stUserActionSetting.Valet.bActive;
}

