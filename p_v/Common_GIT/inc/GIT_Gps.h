#ifndef __GIT_GPS_H__
#define __GIT_GPS_H__

#include "GIT_Interprotocol.h"
#include <nmea/nmea.h>


extern nmeaINFO g_GPSInfo;
extern nmeaPARSER g_GPSParser;

void InitGpsParser(void);
void DestroyGpsParser(void);
void SetNewGpsInfo(int32_t nlat,int32_t nlon,uint8_t fixType,uint8_t numSV, nmeaTIME tm,int32_t direction);

#if defined(USE_UBLOX_GPS)
void SetUBX_NAVX5(void);
void SetUBX_NAV5(void);
#endif
int GPSRecvGetLine(void *pQueue, unsigned char *pArrPopBuff);

typedef __packed struct _stPreGPSInfo
{
	int iSig;
	double dLat;
	double dLon;
}stPreGPSInfo;

int Get_GPS_Vailication();
int Get_GPS_SatellitesNum();
double Get_GPS_Lat();
double Get_GPS_Lon();
double ConvGPSData(double fGpsData);
double ConvReverseGPSData(double dGpsData);
void GPSValidDataSave();
bool GPSSpeedFilter(nmeaINFO stCurGps);
#endif
