/* Includes ------------------------------------------------------------------*/
#include "AutolinkMessage.h"
#include "AutolinkConfig.h"

#include "Modem_Manager.h"
#include "GIT_Util.h"
#include "Modem_comm.h"
#include "Power_Manager.h"
#include "OBD_Controller.h"
#include "Message_Make.h"
#if defined(USE_GIT_FAT_FS)
#include "ff.h"
#endif

#include "MngSystem.h"
#include "MngSystemUtil.h"
#include "MngQueue.h"
#include "HdDebug.h"



#include <math.h>
#include <time.h>

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)

//#define ENABLE_GEOFENSE_LOG

extern void SendGeoFenceAlramReport(int32_t nEvent, int32_t nResult, stGeofenceUnit* pstCurGeofence, stGeofenceUnit* pstSetGeofence,eGeoFenceType eGeofenceType,uint32_t unGeofenceID);

void SetGeofenceSetting(stGeoFenceSetting stGeofenceValue);
void GetGeofenceSetting(stGeoFenceSetting* pstGeofenceSetting);
void DisplayGeofenceSetting();


// user action setting variable
extern stUserActionSetting m_stUserActionSetting;

extern stMsgHandlerData m_stMsgHdData;
void GetGeofenceSetting(stGeoFenceSetting* pstGeofenceSetting)
{
    memcpy((char*)pstGeofenceSetting,(char*)&m_stUserActionSetting.Geofence,sizeof(stGeoFenceSetting));
}

void SetGeofenceSetting(stGeoFenceSetting stGeofenceValue)
{
    stGeofenceUnit stCurrentUnit;
    stGeofenceUnit stSettingUnit;

    memcpy((char*)&m_stUserActionSetting.Geofence,(char*)&stGeofenceValue,sizeof(stGeoFenceSetting));

    stCurrentUnit.GpsLatitude = Get_GPS_Lat();
    stCurrentUnit.GpsLongitude = Get_GPS_Lon();

    memset((char*)&stSettingUnit,0,sizeof(stGeofenceUnit));

    if( CheckGeoFence(stCurrentUnit,&stSettingUnit,false,false) == true )
    {
        // initialize check geofence setting
        // send geo fence alram
        SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM, 1, &stCurrentUnit, &stSettingUnit,eGFT_CIRCLE,0);
    }

    // save setting value 
    WriteConfig(false,true);
}

void DisplayPolygonGeofence()
{
    Trace("============================================================\r\n");
    Trace(" All Enable : %d\r\n",m_stUserActionSetting.PolygonGeofence.bEnableAllGeofence);

    for(int i=0;i<m_stUserActionSetting.PolygonGeofence.ucPolygonGeofenceCount;i++)
    {
        Trace("#%d] Enable : %d\r\n",i,m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].bActive);
        Trace("#%d] Type : %d\r\n",i,m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].cFenceType);
        Trace("#%d] Point List Count : %d\r\n",i,m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].ucPointListCount);
        for(int j=0;j<m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].ucPointListCount;j++)
        {
            Trace("#%d] Lat : %f, Log : %f\r\n",i,m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].stPointList[j].GpsLatitude,
                m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].stPointList[j].GpsLongitude);
        }
        
    }
}

void SetPolygonGeofenceSetting(stPolygonGeoFenceSetting stPolygonGeofenceValue)
{
    stGeofenceUnit stCurrentUnit;
    stGeofenceUnit stSettingUnit;
    
    memcpy((char*)&m_stUserActionSetting.PolygonGeofence,(char*)&stPolygonGeofenceValue,sizeof(stPolygonGeoFenceSetting));

    stCurrentUnit.GpsLatitude = Get_GPS_Lat();
    stCurrentUnit.GpsLongitude = Get_GPS_Lon();

    memset((char*)&stSettingUnit,0,sizeof(stGeofenceUnit));

    CheckPolygonGeoFence(stCurrentUnit,&stSettingUnit);

    // display polygon geofence info
    DisplayPolygonGeofence();

    // save setting value 
    WriteConfig(false,true);
}


void DisplayGeofenceSetting()
{
    Trace("=============================================================\r\n");
    Trace("=============================================================\r\n");
    Trace("Command : %d\r\n",m_stUserActionSetting.Geofence.usCommand);
    Trace("EnableAllGeofence : %d\r\n",m_stUserActionSetting.Geofence.bEnableAllGeofence);
    //Trace("Guid : %s\r\n",m_stUserActionSetting.Geofence.carrGuid);
    Trace("LengthSize : %d\r\n",m_stUserActionSetting.Geofence.usLengthSize);
    Trace("Length : %d\r\n",m_stUserActionSetting.Geofence.usLength);
    Trace("UtcTime : %d\r\n",m_stUserActionSetting.Geofence.unDateTime);

    for(int i=0;i<MAX_GEOFENCE_LIST;i++)
    {
        Trace("=============================================================\r\n");
        Trace("Index : %d\r\n",i);
        Trace("FenceType : %d\r\n",m_stUserActionSetting.Geofence.stGeofencePoint[i].cFenceType);
        Trace("Distance : %d\r\n",m_stUserActionSetting.Geofence.stGeofencePoint[i].unDistance);
        //Trace("UtcTiem : %s\r\n",m_stUserActionSetting.Geofence.stGeofencePoint[i].arrDateTime);
        Trace("Latitude : %f\r\n",m_stUserActionSetting.Geofence.stGeofencePoint[i].GpsLatitude);
        Trace("Longitude : %f\r\n",m_stUserActionSetting.Geofence.stGeofencePoint[i].GpsLongitude);
        Trace("Active : %d\r\n",m_stUserActionSetting.Geofence.stGeofencePoint[i].bActive);
        Trace("AlreadyAlram : %d\r\n",m_stUserActionSetting.Geofence.stGeofencePoint[i].bAlreadyAlramed);
    }
    Trace("=============================================================\r\n");
    Trace("=============================================================\r\n");
}

boolean_t CheckInOutBoundary(stGeofenceUnit stSettingUnit, stGeofenceUnit stCurrentUnit);

void SetGeoFenceUnit(stReportSmartReq stGeofenceValue)
{
    m_stUserActionSetting.Geofence.stGeofencePoint[0].GpsLatitude = stGeofenceValue.Latitude;
    m_stUserActionSetting.Geofence.stGeofencePoint[0].GpsLongitude = stGeofenceValue.Longitude;

    if( m_stUserActionSetting.Geofence.stGeofencePoint[0].GpsLatitude != 0 && m_stUserActionSetting.Geofence.stGeofencePoint[0].GpsLongitude != 0 &&
        m_stUserActionSetting.Geofence.stGeofencePoint[0].cFenceType != 0 && m_stUserActionSetting.Geofence.stGeofencePoint[0].unDistance != 0)
        m_stUserActionSetting.Geofence.stGeofencePoint[0].bActive = true;

    m_stUserActionSetting.Geofence.stGeofencePoint[0].bAlreadyAlramed = false;
    m_stUserActionSetting.Geofence.stGeofencePoint[0].bInAlreadyAlramed = false;
    m_stUserActionSetting.Geofence.stGeofencePoint[0].bOutAlreadyAlramed = false;
    m_stUserActionSetting.Geofence.stGeofencePoint[0].cFenceType = stGeofenceValue.BoundType;
    m_stUserActionSetting.Geofence.stGeofencePoint[0].unDistance = stGeofenceValue.Distance;

    Trace("lat : %f, lon : %f, distance : %d, FenceType : %c, Active : %x\r\n",
                                                m_stUserActionSetting.Geofence.stGeofencePoint[0].GpsLatitude,
                                                m_stUserActionSetting.Geofence.stGeofencePoint[0].GpsLongitude,
                                                m_stUserActionSetting.Geofence.stGeofencePoint[0].unDistance,
                                                m_stUserActionSetting.Geofence.stGeofencePoint[0].cFenceType,
                                                m_stUserActionSetting.Geofence.stGeofencePoint[0].bActive);
}

boolean_t CheckGeoFence(stGeofenceUnit stCurrentUnit,stGeofenceUnit* pstSettingUnit,boolean_t bActive,boolean_t bReport)
{
    for(int32_t i=0;i<MAX_GEOFENCE_LIST;i++)
    {
        if( m_stUserActionSetting.Geofence.stGeofencePoint[i].bActive == true )
        {
            // check current point is in or not
            if( CheckInOutBoundary(m_stUserActionSetting.Geofence.stGeofencePoint[i], stCurrentUnit) == true )
            {
                // clear out event flag
                m_stUserActionSetting.Geofence.stGeofencePoint[i].bOutAlreadyAlramed = false;

                // check this geofence is inboundary
                if( m_stUserActionSetting.Geofence.stGeofencePoint[i].cFenceType == eREMOTE_CON_BOUNDTYPE_IN ||
                    m_stUserActionSetting.Geofence.stGeofencePoint[i].cFenceType == eREMOTE_CON_BOUNDTYPE_BOTH )
                {
                    // in boundary process                
                    // check already report this alram
                    if( m_stUserActionSetting.Geofence.stGeofencePoint[i].bInAlreadyAlramed == false )
                    {
                        m_stUserActionSetting.Geofence.stGeofencePoint[i].bInAlreadyAlramed = true;

                        // send alram
                        Trace("=============================================================\r\n");
                        Trace("GeoFence In Alram Occurred\r\n");
                        Trace("=============================================================\r\n");
                        // copy current setting info to send to the server
                        memcpy((char*)pstSettingUnit,(char*)&m_stUserActionSetting.Geofence.stGeofencePoint[i],sizeof(stGeofenceUnit));
                        if( bActive == true )
                            return true;

                        if( bReport == true )
                        {
							stCurrentUnit.cFenceType = eREMOTE_CON_BOUNDTYPE_IN;
                            SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM, 1, &stCurrentUnit, &m_stUserActionSetting.Geofence.stGeofencePoint[i],eGFT_CIRCLE,0);
                        }
                    }
                }
            }
            else
            {
                // clear in event flag
                m_stUserActionSetting.Geofence.stGeofencePoint[i].bInAlreadyAlramed = false;
                
                // check this geofence is inboundary
                if( m_stUserActionSetting.Geofence.stGeofencePoint[i].cFenceType == eREMOTE_CON_BOUNDTYPE_OUT ||
                    m_stUserActionSetting.Geofence.stGeofencePoint[i].cFenceType == eREMOTE_CON_BOUNDTYPE_BOTH )
                {
                    // out boundary process
                    if( m_stUserActionSetting.Geofence.stGeofencePoint[i].bOutAlreadyAlramed == false )
                    {
                        m_stUserActionSetting.Geofence.stGeofencePoint[i].bOutAlreadyAlramed = true;

                        // send alram
                        Trace("=============================================================\r\n");
                        Trace("GeoFence Out Alram Occurred\r\n");
                        Trace("=============================================================\r\n");
                        // copy current setting info to send to the server
                        memcpy((char*)pstSettingUnit,(char*)&m_stUserActionSetting.Geofence.stGeofencePoint[i],sizeof(stGeofenceUnit));
                        if( bActive == true )
                            return true;

                        if( bReport == true )
                        {
							stCurrentUnit.cFenceType = eREMOTE_CON_BOUNDTYPE_OUT;
                            SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM, 1, &stCurrentUnit, &m_stUserActionSetting.Geofence.stGeofencePoint[i],eGFT_CIRCLE,0);
                        }
                    }
                }
            }
        }
    }

    return false;
}

boolean_t GetGeofenceActive()
{
    return m_stUserActionSetting.Geofence.bEnableAllGeofence;
}

#define PHI (3.141592)
//#define PHI (3.14159265358979323846)

// This function converts decimal degrees to radians
double deg2rad(double deg) {
    return (double)((deg * (double)PHI) / (double)180.0);
}

// This function converts radians to decimal degrees
double rad2deg(double rad) {
    return (rad * (double)180 / (double)PHI);
}

//#define deg2rad(angleDegrees) (double)((double)(angleDegrees * PHI) / 180.0)
//#define rad2deg(angleRadians) (double)((double)(angleRadians * 180.0) / PHI)

double GetDistance(stGeofenceUnit stSettingUnit, stGeofenceUnit stCurrentUnit)
{
    if( stSettingUnit.GpsLatitude == stCurrentUnit.GpsLatitude &&
        stSettingUnit.GpsLongitude == stCurrentUnit.GpsLongitude )
        return 0;

    // convert from degree to radian
    double e10 = stSettingUnit.GpsLatitude * PHI / 180;
    double e11 = stSettingUnit.GpsLongitude * PHI / 180;
    double e12 = stCurrentUnit.GpsLatitude * PHI / 180;
    double e13 = stCurrentUnit.GpsLongitude * PHI / 180;

    // get circle grs80
    double c16 = 6356752.314140910;
    double c15 = 6378137.000000000;
    double c17 = 0.0033528107;
//    double f15 = c17+c17*c17;
//    double f16 = f15/2;
//    double f17 = c17*c17/2;
//    double f18 = c17*c17/8;
//    double f19 = c17*c17/16;
    double c18 = e13-e11;
    double c20 = (1-c17)*tan(e10);
    double c21 = atan(c20);
    double c22 = sin(c21);
    double c23 = cos(c21);
    double c24 = (1-c17)*tan(e12);
    double c25 = atan(c24);
    double c26 = sin(c25);
    double c27 = cos(c25);
    double c29 = c18;
    double c31 = (c27*sin(c29)*c27*sin(c29))+(c23*c26-c22*c27*cos(c29))*(c23*c26-c22*c27*cos(c29));
    double c33 = (c22*c26)+(c23*c27*cos(c29));
    double c35 = sqrt(c31)/c33;
    double c36 = atan(c35);
    double c38 = 0;

    if(c31 == 0 )
    {
        c38 = 0;
    }
    else
    {
        c38 = c23*c27*sin(c29)/sqrt(c31);
    }

    double c40 = 0;

    if( (cos(asin(c38))*cos(asin(c38))) == 0 )
    {
        c40 = 0;
    }
    else
    {
        c40 = c33-2*c22*c26/(cos(asin(c38))*cos(asin(c38)));
    }

    double c41 = cos(asin(c38))*cos(asin(c38))*(c15*c15-c16*c16)/(c16*c16);
    double c43 = 1+c41/16384*(4096+c41*(-768+c41*(320-175*c41)));
    double c45 = c41/1024*(256+c41*(-128+c41*(74-47*c41)));
    double c47 = c45*sqrt(c31)*(c40+c45/4*(c33*(-1+2*c40*c40)-c45/6*c40*(-3+4*c31)*(-3+4*c40*c40)));
    double c50 = c17/16*cos(asin(c38))*cos(asin(c38))*(4+c17*(4-3*cos(asin(c38))*cos(asin(c38))));
    double c52 = c18+(1-c50)*c17*c38*(acos(c33)+c50*sin(acos(c33))*(c40+c50*c33*(-1+2*c40*c40)));
    double c54 = c16*c43*(atan(c35)-c47);

    return c54;

}

// this function will return mailes value
double GetDistance1(stGeofenceUnit stSettingUnit, stGeofenceUnit stCurrentUnit)
{
    double lat1 = stSettingUnit.GpsLatitude;
    double lon1 = stSettingUnit.GpsLongitude;
    double lat2 = stCurrentUnit.GpsLatitude;
    double lon2 = stCurrentUnit.GpsLongitude;

//    double earth_radius = 3960.00; //# in miles
    //double lat_1 = lat1;
    //double lon_1 = lon1;
    //double lat_2 = lat2;
    //double lon_2 = lon2;
    //double delta_lat = lat2 - lat1;
    //double delta_lon = lon2 - lon1;

    double delta_lon = (double)((lon2 - lon1));


/*
    double s1 = (float)sin(deg2rad(lat1*100));
    double s2 = (float)sin(deg2rad(lat2));
    double s3 = (float)cos(deg2rad(lat1));
    double s4 = (float)cos(deg2rad(lat2));
    double s5 = (float)cos(deg2rad(delta_lon));
*/

    double s1 = (double )(sin(deg2rad(lat1)));
    double s2 = (double )(sin(deg2rad(lat2)));
    double s3 = (double )(cos(deg2rad(lat1)));
    double s4 = (double )(cos(deg2rad(lat2)));
    double s5 = (double )(cos(deg2rad(delta_lon)));

    printf("lat1:%f,lat2:%f\n",sin(deg2rad(lat1)),sin(deg2rad(lat2)));
    printf("s1:%f,s2:%f,s3:%f,s4:%f\n",s1,s2,s3,s4);

    double distance =  (double)((s1 * s2 + s3 * s4 * s5));


/*
    volatile double distance = (double)sin(deg2rad(lat1)) *
        (double)sin(deg2rad(lat2)) +
        (double)cos(deg2rad(lat1)) *
        (double)cos(deg2rad(lat2)) *
        (double)cos(deg2rad(delta_lon));
*/
    distance = acos(distance);
    distance = rad2deg(distance);
    distance = distance * 60 * 1.1515;
    //distance = round(distance, 4);
    distance = round(distance);

    return distance;
}

// this function will return meter value
double GetDistance2(double lat1, double lon1, double lat2, double lon2)
{
    double theta = lon1 - lon2;

    double dist = sin(deg2rad(lat1)) *
        sin(deg2rad(lat2)) +
        cos(deg2rad(lat1)) *
        cos(deg2rad(lat2)) *
        cos(deg2rad(theta));

    dist = acos(dist);
    dist = rad2deg(dist);
    dist = dist * 60 * 1.1515;
    dist = dist * 1609.344;
/*
    if (unit == "kilometer") {
        dist = dist * 1.609344;
    } else if(unit == "meter"){
        dist = dist * 1609.344;
    }
*/
    return (dist);
}

boolean_t IsIntheBoundary(stGeofenceUnit stSettingUnit, stGeofenceUnit stCurrentUnit)
{
#ifdef ENABLE_GEOFENSE_LOG    
    Trace("stSettingUnit.GpsLatitude : %f, stSettingUnit.GpsLatitude : %f\r\n",
        stSettingUnit.GpsLatitude, stSettingUnit.GpsLongitude);
    Trace("stCurrentUnit.GpsLatitude : %f, stCurrentUnit.GpsLatitude : %f\r\n",
        stCurrentUnit.GpsLatitude, stCurrentUnit.GpsLongitude);
#endif //#ifdef ENABLE_GEOFENSE_LOG

    double fDistance1 = GetDistance2(stSettingUnit.GpsLatitude,stSettingUnit.GpsLongitude,
        stCurrentUnit.GpsLatitude,stCurrentUnit.GpsLongitude);

#ifdef ENABLE_GEOFENSE_LOG
    Trace("Set Distance : %d, Calc Distance1 : %f, \r\n",stSettingUnit.unDistance,fDistance1);
#endif //#ifdef ENABLE_GEOFENSE_LOG

    // we need to change decesion value with user setting value
    // compare killometer values
    if( fDistance1 < stSettingUnit.unDistance )
        return true;

    return false;
}

// 322killometer // 200miles
// seoul 37.577140, 126.964034
// busan 35.197562, 129.028814
boolean_t CheckInOutBoundary(stGeofenceUnit stSettingUnit, stGeofenceUnit stCurrentUnit)
{
    // in boundary
    if( IsIntheBoundary(stSettingUnit, stCurrentUnit) == true )
    {
        return true;
    }

    return false;
}

double isLeft2( stPolygonGeofencePoint P0, stPolygonGeofencePoint P1, stGeofenceUnit P2 )
{
    return ( (P1.GpsLongitude - P0.GpsLongitude) * (P2.GpsLatitude - P0.GpsLatitude)
            - (P2.GpsLongitude -  P0.GpsLongitude) * (P1.GpsLatitude - P0.GpsLatitude) );
}

unsigned char PolygonGeoFenceCheck2( stGeofenceUnit CurrentPoint, stPolygonGeofenceUnit* pstPolyGeofenceListInfo,stPolygonGeofencePoint* pstPolygonPointList)
{
    unsigned char ucCheckValue=0,ucRet=0;
	int i=0;

    for (i=0; i<pstPolyGeofenceListInfo->ucPointListCount; i++) {
        if (pstPolygonPointList[i].GpsLatitude <= CurrentPoint.GpsLatitude) {
            if (pstPolygonPointList[i+1].GpsLatitude  > CurrentPoint.GpsLatitude)
                 if (isLeft2( pstPolygonPointList[i], pstPolygonPointList[i+1], CurrentPoint) > 0.0)
                    ++ucCheckValue;
        }
        else {
            if (pstPolygonPointList[i+1].GpsLatitude  <= CurrentPoint.GpsLatitude)
                 if (isLeft2( pstPolygonPointList[i], pstPolygonPointList[i+1], CurrentPoint) < 0.0)
                    --ucCheckValue;
        }
    }
	
	if( ucCheckValue == 0 )	ucRet = eREMOTE_CON_BOUNDTYPE_OUT;
	else					ucRet = eREMOTE_CON_BOUNDTYPE_IN;
	
    return ucRet;
}

boolean_t GetPolygonGeofenceActive()
{
    return m_stUserActionSetting.PolygonGeofence.bEnableAllGeofence;
}

void DisplayReportPolygonGeofence(stGeofenceUnit* pstCurrentUnit, stPolygonGeofenceUnit* pstPolygonGeofenceUnit)
{
    Trace("========================================================\r\n");
    Trace("New Polygon Geofence Alram Occurred\r\n");
    Trace("Poly GeoFence Type : %C\r\n", pstPolygonGeofenceUnit->cFenceType);
    Trace("Current GPS lat : %f, lon : %f\r\n",pstCurrentUnit->GpsLatitude,pstCurrentUnit->GpsLongitude);
    Trace("Set First GPS lat : %f, lon : %f\r\n",pstPolygonGeofenceUnit->stPointList[0].GpsLatitude,
        pstPolygonGeofenceUnit->stPointList[0].GpsLongitude);
    Trace("========================================================\r\n");    
}

void Send2AppPolygonGeofence(stGeofenceUnit* pstCurrentUnit, stPolygonGeofenceUnit* pstPolygonGeofenceUnit)
{

    stGeofenceUnit stSettingUnit;
    memset(&stSettingUnit,0,sizeof(stGeofenceUnit));

    DisplayReportPolygonGeofence(pstCurrentUnit,pstPolygonGeofenceUnit);
    
    // new alram occurred
    stSettingUnit.bActive = pstPolygonGeofenceUnit->bActive;
    stSettingUnit.bAlreadyAlramed = pstPolygonGeofenceUnit->bAlreadyAlramed;
    stSettingUnit.cFenceType = pstCurrentUnit->cFenceType;
    stSettingUnit.unDistance = 0;
    stSettingUnit.GpsLatitude = pstPolygonGeofenceUnit->stPointList[0].GpsLatitude;
    stSettingUnit.GpsLongitude = pstPolygonGeofenceUnit->stPointList[0].GpsLongitude;
    
    SendGeoFenceAlramReport(eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM, 1, pstCurrentUnit, &stSettingUnit, eGFT_POLYGON, pstPolygonGeofenceUnit->unGeofenceID);
}

void CheckPolygonGeoFence(stGeofenceUnit stCurrentUnit,stGeofenceUnit* pstSettingUnit)
{
    unsigned char ucPolygonCheckType;
    stPolygonGeofencePoint stPointList[MAX_POLYGON_LIST_COUNT+1];
    memset((char*)&stPointList,0,sizeof(stPointList));

    // display polygon geofence info
    //DisplayPolygonGeofence();

//    Trace("Polygon Geofence Current GPS lat : %f, lon : %f\r\n",stCurrentUnit.GpsLatitude,stCurrentUnit.GpsLongitude);
    
    for(int i=0;i<m_stUserActionSetting.PolygonGeofence.ucPolygonGeofenceCount;i++)
    {
        // copy save data 
        memcpy((char*)&stPointList,
        (char*)&m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].stPointList,
        sizeof(stPolygonGeofencePoint)*m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].ucPointListCount);
        
        // and then added last point to the list
        memcpy((char*)&stPointList[m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].ucPointListCount],(char*)&stPointList[0],sizeof(stPolygonGeofencePoint));

        // check this list is active or not
        if( m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].bActive == true )
        {            
            ucPolygonCheckType=PolygonGeoFenceCheck2(stCurrentUnit,&m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i],(stPolygonGeofencePoint*)&stPointList);

            if ( ucPolygonCheckType == eREMOTE_CON_BOUNDTYPE_IN )        
            {
                // clear out event flag
                m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].bOutAlreadyAlramed = false;
                
                if( m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].cFenceType == eREMOTE_CON_BOUNDTYPE_IN || 
                    m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].cFenceType == eREMOTE_CON_BOUNDTYPE_BOTH )
                {
                    
                    if( m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].bInAlreadyAlramed == false )
                    {
                        m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].bInAlreadyAlramed = true;

                        // report in event flag
						stCurrentUnit.cFenceType = eREMOTE_CON_BOUNDTYPE_IN;
                        Send2AppPolygonGeofence(&stCurrentUnit,&m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i]);
                    }
                }
            }
            else
            {
                // clear out event flag
                m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].bInAlreadyAlramed = false;
                
                if( m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].cFenceType == eREMOTE_CON_BOUNDTYPE_OUT ||
                    m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].cFenceType == eREMOTE_CON_BOUNDTYPE_BOTH )
                {                    
                    if( m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].bOutAlreadyAlramed == false )
                    {
                        // new alram occurred
                        m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i].bOutAlreadyAlramed = true;

                        // report in event flag
						stCurrentUnit.cFenceType = eREMOTE_CON_BOUNDTYPE_OUT;
                        Send2AppPolygonGeofence(&stCurrentUnit,&m_stUserActionSetting.PolygonGeofence.stPolygonGeofenceList[i]);                        
                    }
                }

            }

        }

        memset((char*)&stPointList,0,sizeof(stPointList));
    }
}
