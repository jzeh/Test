/*
  ******************************************************************************
  * @file    gps.c
  * @author  KCH(App Service Team)
  * @version V1.0.0
  * @date    2013-08-13
  * @brief
  *
  *
  ******************************************************************************
*/
#include "GIT_Gps.h"
#include "GIT_OemInterface.h"
#include "GIT_Util.h"
#include "OBD_Controller.h"

#include "HdDebug.h"
#include "HalHandler.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_GPS,__VA_ARGS__)

#define UBLOX_CHECKSUM_LENGTH   2
#define UBLOX_HEADER_LENGTH 6

#if defined(USE_GPS_LGPL_PARSER)
#include <nmea/nmea.h>
#include "nmea/time.h"
#endif

#if defined(USE_GPS_LGPL_PARSER)
nmeaINFO        g_GPSInfo;
nmeaPARSER      g_GPSParser;
#endif

#include <time.h>
#include <math.h>

#define MAX_GPS_LED_DELAY_CNT 1
extern	int g_nGPSTransmitLedDelayCount;
extern void SetGPSTransmitLedOnOff();
extern unsigned short Get_RPM();
extern unsigned short Get_Speed();
extern double Get_GpsStartOnLat();
extern double Get_GpsStartOnLon();

typedef enum __UBLOX_RCV_TYPE{
    eUbloxNone = 0,
    eUbloxData,
    eUbloxProtocol,
}UBLOX_RCV_TYPE;

UBLOX_RCV_TYPE GPSRecvGetLine2(void *pQueue, unsigned char *pArrPopBuff,int* pnRcvLength);

void SetGpsUtcTime(nmeaTIME utc);
void GPSValidDataSave();
uint32_t GetGpsLocalTimefromUtcTime();

void ConvertGps2Rtc(nmeaTIME stGpsTime,stHalRTCTypeDef* pstDate);
//extern uint32_t GetTimefromDate2(stHalRtcDateTime stDate);
extern OBD_CONTROLLER_DATA g_OBDControllerData;

double g_dSavedGpsLat=0;
double g_dSavedGpsLon=0;

stPreGPSInfo g_st1SecPreGPSInfo;

void ConvertGps2Rtc(nmeaTIME stGpsTime, stHalRTCTypeDef* pstDate)
{
    pstDate->RtcDate.RTC_Year   = stGpsTime.year-2000;
    pstDate->RtcDate.RTC_Month  = stGpsTime.mon;
    pstDate->RtcDate.RTC_Date   = stGpsTime.day;
    pstDate->RtcTime.RTC_Hours  = stGpsTime.hour;
    pstDate->RtcTime.RTC_Minutes = stGpsTime.min;
    pstDate->RtcTime.RTC_Seconds = stGpsTime.sec;
}

void InitGpsParser(void)
{
#if defined(USE_GPS_LGPL_PARSER)
	nmea_zero_INFO(&g_GPSInfo);
	nmea_parser_init(&g_GPSParser);
#endif
}

void DestroyGpsParser(void)
{
#if defined(USE_GPS_LGPL_PARSER)
	nmea_parser_destroy(&g_GPSParser);
#endif
}

extern void HandlerUbloxProtocol(char* arrNmeaData, int uiReceivedPacket);
extern uint32_t GetUbloxStates();


void ParsingGPSNmeaProtocol(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType)
{
#if defined(USE_UBLOX_GPS)
    stQueue *pQueue = (stQueue*)pBuff;
    unsigned int uiReceivedPacket;
    //unsigned char arrNmeaData[NMEA_CONVSTR_BUF];
	// 180719 위성 21개 이상 잡으면 단말reset 걸려서 사이즈 늘림
	unsigned char arrNmeaData[MAX_GPS_BUFF_SIZE];
    UBLOX_RCV_TYPE eUbloxType;

	// NMEA데이터가 수신되는 상태를 확인하기 위한 플래그, NMEA 파서 내부로 넣으면 더 좋을 듯.
    //uiReceivedPacket = GPSRecvGetLine(pQueue, arrNmeaData);
    if( GetQueueDataLength(pQueue) > 0 )
    {
        // parse gps data from ublox module
        eUbloxType = GPSRecvGetLine2(pQueue, arrNmeaData,(int*)&uiReceivedPacket);
        //hexdump(arrNmeaData,uiReceivedPacket);

        if( eUbloxType == eUbloxProtocol )
        {
            //printf("ublox protocol:\n");
            //hexdump(arrNmeaData, uiReceivedPacket);
            HandlerUbloxProtocol((char*)arrNmeaData, uiReceivedPacket);
            //hexdump(arrNmeaData, uiReceivedPacket);
        }
        else if( eUbloxType == eUbloxData )
        {
#ifndef ENABLE_UBLOX_GPS_DATA
            if( GetUbloxStates() <= eUbloxInit )
            {
                // not initialize ublox wait for a seconds
                return;
            }

#if defined(USE_GPS_LGPL_PARSER)
    		// nmea start 찾기
    		if(uiReceivedPacket >= 1)
            {
    			nmea_parse(&g_GPSParser, (const char*)arrNmeaData, uiReceivedPacket, &g_GPSInfo);

#if false
                if( ((g_GPSInfo.smask & GPRMC ) == GPRMC))
                {
                    Trace("=================================\n");
                    Trace("NMEA1 lat : %f, lon : %f, valid : %d, count : %d\n",Get_GPS_Lat(),Get_GPS_Lon(),g_GPSInfo.sig,g_GPSInfo.satinfo.inview);
                }
#endif //#if true

    			GPSValidDataSave();

#if false
                if( ((g_GPSInfo.smask & GPRMC ) == GPRMC))
                {
                    Trace("=================================\n");
                    Trace("NMEA2 lat : %f, lon : %f, valid : %d, count : %d\n",Get_GPS_Lat(),Get_GPS_Lon(),g_GPSInfo.sig,g_GPSInfo.satinfo.inview);
                }
#endif //#if true


                if( ((g_GPSInfo.smask & GPRMC ) == GPRMC) && Get_GPS_Vailication() >= NMEA_SIG_FIXED_2D_3D )
                {
                    // setting utc time to system
                    SetGpsUtcTime(g_GPSInfo.utc);
                    g_GPSInfo.smask ^= GPRMC;
                }

#endif //#if defined(USE_GPS_LGPL_PARSER)
    		}
#endif //#ifndef ENABLE_UBLOX_GPS_DATA  
    	}
    }

#else	// //#if defined(USE_UBLOX_GPS)

#if defined(USE_GPS_LGPL_PARSER) && defined(RF_COMMON_MODEM)

    unsigned char arrNmeaData[MAX_GPS_BUFF_SIZE]={0,};
    stQueue *pQueue = (stQueue*)pBuff;
    int uiReceivedPacket;

    uiReceivedPacket = GPSRecvGetLine(pQueue, arrNmeaData);
	
	if(uiReceivedPacket >= 1)
	{
		nmea_parse(&g_GPSParser, (const char*)arrNmeaData, uiReceivedPacket, &g_GPSInfo);
		GPSValidDataSave();
	}
#endif //#if defined(USE_GPS_LGPL_PARSER)

#endif //#if defined(USE_UBLOX_GPS)
}

bool m_bGpsUtcTime = false;
uint32_t m_unGpsUtcTime = 0;

void SetGpsUtcTime(nmeaTIME utc)
{
    uint32_t unTime;
    stHalRTCTypeDef stDate;

    memset((char*)&m_unGpsUtcTime,0,sizeof(m_unGpsUtcTime));
    ConvertGps2Rtc(utc,&stDate);
    unTime = GetTimefromDate2(stDate);

    m_unGpsUtcTime = unTime;
    m_bGpsUtcTime = true;
}

bool GetGpsUtcTime(uint32_t* punUtcTime)
{
    if( m_bGpsUtcTime == true )
    {
        *punUtcTime = m_unGpsUtcTime;

        m_bGpsUtcTime = false;
        return true;
    }

    *punUtcTime = 0;
    return false;
}

void GPSValidDataSave()
{
	//bool bSpeedFilter = false;
  
	//bSpeedFilter = GPSSpeedFilter(g_GPSInfo);

    if( (g_GPSInfo.lat != 0) && (g_GPSInfo.lon != 0) && (g_GPSInfo.sig >= NMEA_SIG_FIXED_2D_3D))
	{
        if( Get_Speed() > 0 )
		{
			g_dSavedGpsLat = g_GPSInfo.lat;
			g_dSavedGpsLon = g_GPSInfo.lon;

		      // Start GPS position이 0,0이면 업데이트 해준다.
		      if ( Get_GpsStartOnLat() == 0 && Get_GpsStartOnLon() == 0 )
		      {
		              Send_GpsStatOnValidation(Get_GPS_Vailication());
		              Send_GpsStartOnLat(Get_GPS_Lat());
		              Send_GpsStartOnLon(Get_GPS_Lon());
		      }
		  }
		  else
		  {
			// Start GPS position이 0,0이면 업데이트 해준다.
			if ( Get_GpsStartOnLat() == 0 && Get_GpsStartOnLon() == 0 )
			{
			    Send_GpsStatOnValidation(Get_GPS_Vailication());
				  Send_GpsStartOnLat(Get_GPS_Lat());
				  Send_GpsStartOnLon(Get_GPS_Lon());			
			}
			// 저장된 GPS 값이 0,0이면 업데이트 
			if(g_dSavedGpsLat == 0 || g_dSavedGpsLon == 0)
			{
				g_dSavedGpsLat = g_GPSInfo.lat;
				g_dSavedGpsLon = g_GPSInfo.lon;

			}
			// 정차 시 저장된 값으로 갱신 
			else
			{
				 g_GPSInfo.lat = g_dSavedGpsLat;
				 g_GPSInfo.lon = g_dSavedGpsLon;
			}

		  }
		    Send_GpsDirection((int)g_GPSInfo.direction);
	}
    else
    {
            g_GPSInfo.lat = g_dSavedGpsLat;
            g_GPSInfo.lon = g_dSavedGpsLon;
    }

    //printf("lat : %f,lon : %f sig:%d view:%d use:%d\r\n",g_GPSInfo.lat,g_GPSInfo.lon,g_GPSInfo.sig,g_GPSInfo.satinfo.inview,g_GPSInfo.satinfo.inuse);
}

int GPSRecvGetLine(void *pQueue, unsigned char *pArrPopBuff)
{
	int i, bRet = 0; 	//-1:gabage packet, 0:recv packet, 1이상:recv packet & found \r\n
	unsigned int uiReceivedPacket;
	unsigned char ucTmp;
	stQueue *pQueueTemp = (stQueue*)pQueue;

	if ( IsEmptyQueue(pQueue) ) return 0;

	uiReceivedPacket = GetQueueDataLength((stQueue*)pQueueTemp);

	// find header of Response packet
	for ( i=0; i<uiReceivedPacket; i++ )
	{
		if ( pQueueTemp->pData[(pQueueTemp->uiFront)%pQueueTemp->uiQueueSize] == '$' )
		{
//			GITDebugPrintf("found packet : 0x%02X\r\n", pQueueTemp->pData[(pQueueTemp->uiFront+i)%pQueueTemp->uiQueueSize]);
			break;
		}
		else
		{
			PopQueue(pQueueTemp, &ucTmp);
//			GITDebugPrintf("Remove GPS packet : [%c, 0x%02X]\r\n", ucTmp, ucTmp);
		}
	}

	uiReceivedPacket = GetQueueDataLength(pQueueTemp);
	for ( i=0; i<uiReceivedPacket; i++ )
	{
		if ( (pQueueTemp->pData[((pQueueTemp->uiFront+i))%pQueueTemp->uiQueueSize] == 0x0D)/*'\n'*/ &&
			 (pQueueTemp->pData[((pQueueTemp->uiFront+i+1))%pQueueTemp->uiQueueSize] ==  0x0A)/*'\r'*/  )
		{
			bRet = i+2;
			PopMultiDataQueue(pQueue, pArrPopBuff, i+2);
//			GITDebugPrintf("found packet\r\n");
			break;
		}
	}

	return bRet;
}

UBLOX_RCV_TYPE GPSRecvGetLine2(void *pQueue, unsigned char *pArrPopBuff,int* pnRcvLength)
{
	int i; 	//-1:gabage packet, 0:recv packet, 1이상:recv packet & found \r\n
	unsigned int uiReceivedPacket;
	unsigned char ucTmp;
	stQueue *pQueueTemp = (stQueue*)pQueue;
    UBLOX_RCV_TYPE eUbloxType;
    int eUbloxLength = 0;

    *pnRcvLength = 0;

	if ( IsEmptyQueue(pQueue) ) return eUbloxNone;

	uiReceivedPacket = GetQueueDataLength((stQueue*)pQueueTemp);

    if( uiReceivedPacket < 6 )
        return eUbloxNone;

	// find header of Response packet
	for ( i=0; i<uiReceivedPacket; i++ )
	{
		if ( pQueueTemp->pData[(pQueueTemp->uiFront)%pQueueTemp->uiQueueSize] == '$' )
		{
            eUbloxType = eUbloxData;
			//GITDebugPrintf("found packet : 0x%02X\r\n", pQueueTemp->pData[(pQueueTemp->uiFront+i)%pQueueTemp->uiQueueSize]);
			break;
		}
        else if( pQueueTemp->pData[(pQueueTemp->uiFront)%pQueueTemp->uiQueueSize] == 0xb5 &&
                 pQueueTemp->pData[(pQueueTemp->uiFront+1)%pQueueTemp->uiQueueSize] == 0x62 )
        {
            eUbloxLength =
                pQueueTemp->pData[(pQueueTemp->uiFront+5)%pQueueTemp->uiQueueSize ]<<8|
                pQueueTemp->pData[(pQueueTemp->uiFront+4)%pQueueTemp->uiQueueSize ];

            eUbloxType = eUbloxProtocol;

            if( eUbloxLength > 1024 )
            {
                eUbloxType = eUbloxNone;
                PopQueue(pQueueTemp, &ucTmp);
            }
            break;
        }
		else
		{
			PopQueue(pQueueTemp, &ucTmp);
			//GITDebugPrintf("Remove GPS packet : 0x%02X\r\n", ucTmp);
		}
	}

	uiReceivedPacket = GetQueueDataLength(pQueueTemp);

    if( eUbloxType == eUbloxData )
    {
    	for ( i=0; i<uiReceivedPacket; i++ )
    	{
    		if ( (pQueueTemp->pData[((pQueueTemp->uiFront+i))%pQueueTemp->uiQueueSize] == 0x0D)/*'\n'*/ &&
    			 (pQueueTemp->pData[((pQueueTemp->uiFront+i+1))%pQueueTemp->uiQueueSize] ==  0x0A)/*'\r'*/  )
    		{
    			*pnRcvLength = i+2;
    			PopMultiDataQueue(pQueue, pArrPopBuff, i+2);
    //			GITDebugPrintf("found packet\r\n");
    			break;
    		}
    	}
    }
    else if( eUbloxType == eUbloxProtocol )
    {
        if( uiReceivedPacket >= eUbloxLength+UBLOX_HEADER_LENGTH+UBLOX_CHECKSUM_LENGTH ) // header + size + check
        {
            *pnRcvLength = eUbloxLength+UBLOX_HEADER_LENGTH+UBLOX_CHECKSUM_LENGTH ;
            PopMultiDataQueue(pQueue, pArrPopBuff, eUbloxLength+UBLOX_HEADER_LENGTH+UBLOX_CHECKSUM_LENGTH );
        }
        else
        {
            return eUbloxNone;
        }
    }

	return eUbloxType;
}


extern double GetDistance2(double lat1, double lon1, double lat2, double lon2);

// new gps info form ublox
void SetNewGpsInfo(int32_t nlat,int32_t nlon,uint8_t fixType,uint8_t numSV, nmeaTIME tm,int32_t direction)
{

    g_GPSInfo.lat = (double)nlat/10000000.0;
    g_GPSInfo.lon = (double)nlon/10000000.0;
    g_GPSInfo.sig = fixType;
    g_GPSInfo.satinfo.inuse = numSV;
    g_GPSInfo.satinfo.inview = numSV;
    g_GPSInfo.utc = tm;
    g_GPSInfo.direction = (double)direction/100000.0;

    // basic gps process every 1 second
    GPSValidDataSave();

    if( g_GPSInfo.sig >= NMEA_FIX_3D )
    {
        // setting utc time to system
        SetGpsUtcTime(g_GPSInfo.utc);
    }

    // LED Control
    if ( g_nGPSTransmitLedDelayCount == 0 ) {
        g_nGPSTransmitLedDelayCount = MAX_GPS_LED_DELAY_CNT;
    }

	// 2018.09.04 SPARROW : 생산프로그램 동작 시 아래 영향 안주도록 수정
   	if(g_bYUJINSelftestFlag == false){
		if(Get_GPS_Vailication() == NMEA_SIG_FIXED_2D_3D)  SetGPSTransmitLedOnOff();
		else SetLedOnOffCtl(LED_OFF, eLED_GPS);
	}
    Trace("GPS lat : %f, lon : %f, valid : %d, inuse : %d\r\n",g_GPSInfo.lat,g_GPSInfo.lon,g_GPSInfo.sig,g_GPSInfo.satinfo.inuse);

#if false
    Trace("===================================================\r\n");
    Trace("UBX: Y:%d,M:%d,D:%d,h:%d,m:%d,s:%d\n",
        tm.year,tm.mon,tm.day,
        tm.hour,tm.min,tm.sec);
    Trace("GPS : Y:%d,M:%d,D:%d,h:%d,m:%d,s:%d\n",
        g_GPSInfo.utc.year,g_GPSInfo.utc.mon,g_GPSInfo.utc.day,
        g_GPSInfo.utc.hour,g_GPSInfo.utc.min,g_GPSInfo.utc.sec);

    Trace("GPS : lat : %f, lon : %f, valid : %d, fixed : %d, count : %d\n", g_GPSInfo.lat,g_GPSInfo.lon,g_GPSInfo.sig,g_GPSInfo.fix,g_GPSInfo.satinfo.inuse);
#endif
}

nmeaPOS m_stGPS2SecondsPreviouse;
nmeaPOS m_stGPS1SecondsPreviouse;

extern double GetDistance2(double lat1, double lon1, double lat2, double lon2);

bool GPSReverseFilter(nmeaINFO stCurGps)
{
    //stCurGps.lat = m_stGpsSampleData[m_nIndeGPSSample].lat;
    //stCurGps.lon = m_stGpsSampleData[m_nIndeGPSSample].lon;
    //m_nIndeGPSSample++;

    //printf("Cur   lat : %f, lon : %f\n",stCurGps.lat,stCurGps.lon);
    //printf("1 sec lat : %f, lon : %f\n",m_stGPS1SecondsPreviouse.lat,m_stGPS1SecondsPreviouse.lon);
    //printf("2 sec lat : %f, lon : %f\n",m_stGPS2SecondsPreviouse.lat,m_stGPS2SecondsPreviouse.lon);

    double lat1,lat2,lon1,lon2;

    lat1 = ConvGPSData(m_stGPS2SecondsPreviouse.lat);
    lon1 = ConvGPSData(m_stGPS2SecondsPreviouse.lon);
    lat2 = ConvGPSData(m_stGPS1SecondsPreviouse.lat);
    lon2 = ConvGPSData(m_stGPS1SecondsPreviouse.lon);

    double dlDistA = GetDistance2(lat1,lon1,lat2,lon2);

    lat1 = ConvGPSData(m_stGPS1SecondsPreviouse.lat);
    lon1 = ConvGPSData(m_stGPS1SecondsPreviouse.lon);
    lat2 = ConvGPSData(stCurGps.lat);
    lon2 = ConvGPSData(stCurGps.lon);

    double dlDistB = GetDistance2(lat1,lon1,lat2,lon2);

    lat1 = ConvGPSData(m_stGPS2SecondsPreviouse.lat);
    lon1 = ConvGPSData(m_stGPS2SecondsPreviouse.lon);
    lat2 = ConvGPSData(stCurGps.lat);
    lon2 = ConvGPSData(stCurGps.lon);

    double dlDistC = GetDistance2(lat1,lon1,lat2,lon2);

    m_stGPS2SecondsPreviouse = m_stGPS1SecondsPreviouse;
    m_stGPS1SecondsPreviouse.lat = stCurGps.lat;
    m_stGPS1SecondsPreviouse.lon = stCurGps.lon;

    //printf("Distance A : %f, Distance B : %f, Distance C : %f\n",dlDistA,dlDistB,dlDistC);

    if( !(dlDistC>dlDistA && dlDistC>dlDistB) )
    {
//        printf("GPS Reverse Data // drop this gps data\n");
        return true;
    }

    return false;
}

bool GPSSpeedFilter(nmeaINFO stCurGps)
{
	double dDistance = 0.0;

//	printf("Cur   Lat : %f, Lon : %f\r\n",stCurGps.lat,stCurGps.lon);
//    printf("Prev  Lat : %f, Lon : %f\r\n",g_st1SecPreGPSInfo.dLat,g_st1SecPreGPSInfo.dLon);

	if( (g_st1SecPreGPSInfo.iSig >= NMEA_SIG_FIXED_2D_3D) && (stCurGps.sig >= NMEA_SIG_FIXED_2D_3D) )
	{
		dDistance = GetDistance2(g_st1SecPreGPSInfo.dLat,g_st1SecPreGPSInfo.dLon,stCurGps.lat,stCurGps.lon);	//1번전과 지금의 차이
//		printf("Distance : %f\n",dDistance);
	}
	else dDistance = 999999;

    g_st1SecPreGPSInfo.dLat = stCurGps.lat;
    g_st1SecPreGPSInfo.dLon = stCurGps.lon;
	g_st1SecPreGPSInfo.iSig = stCurGps.sig;

	//350km/h = 5833m/m = 97.2m/s
	if( dDistance < 100.0 )
	{
		return true;
	}
//	else    printf("GPS Reverse Data // drop this gps data\n");
    return false;
}

int Get_GPS_Vailication()
{
#ifndef ENABLE_UBLOX_GPS_DATA    
    return g_GPSInfo.sig;
#else
    if( g_GPSInfo.sig >= NMEA_SIG_FIXED_2D_3D )
        return NMEA_SIG_FIXED_2D_3D;
    else 
        return 0;
#endif
}

int Get_GPS_SatellitesNum()
{
	return g_GPSInfo.satinfo.inview;
}

int Get_GPS_SatellitesUsedNum()
{
	return g_GPSInfo.satinfo.inuse;
}

double Get_GPS_Lat()
{
    if ( Get_GPS_Vailication() < NMEA_SIG_FIXED_2D_3D ) 
        g_GPSInfo.lat = g_dSavedGpsLat;
    
#ifdef RF_COMMON_MODEM     
	return ConvGPSData(g_GPSInfo.lat);
#else
    return g_GPSInfo.lat;
#endif
}

double Get_GPS_Lon()
{
    if ( Get_GPS_Vailication() < NMEA_SIG_FIXED_2D_3D ) 
        g_GPSInfo.lon = g_dSavedGpsLon;
    
#ifdef RF_COMMON_MODEM    
	return ConvGPSData(g_GPSInfo.lon);
#else
    return g_GPSInfo.lon;
#endif
}

double ConvGPSData(double dGpsData)
{
	double dResultData;
	int iBigData ;
	double dSmallData;

	iBigData = (int)(dGpsData / 100.00);
	dSmallData = (dGpsData - iBigData*100.00)/60.0;
	dResultData = (double)iBigData + dSmallData;
	return dResultData;
}

double ConvReverseGPSData(double dGpsData)
{
	double dResultData;
	int iBigData ;
	double dSmallData;

	iBigData = (int)dGpsData;
	dSmallData = (dGpsData - iBigData)*60.000000;
	dResultData = (double)(iBigData*100 + dSmallData);
	return dResultData;
}

double Get_GPS_Lat_Origin()
{
#ifdef RF_COMMON_MODEM
	return g_dSavedGpsLat;
#else
	return g_GPSInfo.lat;
#endif
}

double Get_GPS_Lon_Origin()
{
#ifdef RF_COMMON_MODEM
	return g_dSavedGpsLon;
#else
	return g_GPSInfo.lon;
#endif
}

void Set_GPS_Lat_Origin(double lat)
{
	g_dSavedGpsLat = g_GPSInfo.lat = lat;
}

void Set_GPS_Lon_Origin(double lon)
{
	g_dSavedGpsLon = g_GPSInfo.lon = lon;
}
//float GetGPSData(float fGpsData)
//{
//	float fResultData;
//	int fBigData ;
//	float fSmallData;
//
//	fBigData = (int)(fGpsData / 100.00);
//	fSmallData = (fGpsData - fBigData*100.00)/ 60;
//	fResultData = fBigData + fSmallData;
//	//GITDebugPrintf(" [%d] %f %f\r\n", fBigData,(fGpsData - (fBigData)) ,fResultData);
//	return fResultData;
//}
//fGPSLongitude = GetGPSData((float)g_GPSInfo.lon);
