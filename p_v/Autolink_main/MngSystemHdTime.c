/* Includes ------------------------------------------------------------------*/
#include "AutolinkMessage.h"
#include "AutolinkConfig.h"

#include "Modem_Manager.h"
#include "GIT_Util.h"
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

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)

//native
unsigned int GetTimefromDate(stHalRTCTypeDef stRtcInfo);
void GetDatefromTime(stHalRTCTypeDef* stDateTime,unsigned int binary);

//unix to system
uint8_t computeDayOfWeek(uint16_t y, uint8_t m, uint8_t d);
uint32_t GetTimefromDate2(stHalRTCTypeDef stDate);
void GetDatefromTime2(stHalRTCTypeDef* stDate,uint32_t unTime);
void ConvertLocal2UtcTime(uint32_t unLocalTime, uint32_t* punUtcTime);
uint32_t GetLocalTimefromTime(uint32_t unUTCTime);

void GetLocalTimeforDate(stHalRTCTypeDef* pstDate)
{
    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)pstDate, sizeof(stHalRTCTypeDef), 0);

#if false
    Trace(" Message Occurred Time: %04d/%02d/%02d,%02d:%02d:%02d\n", stDate->RtcDate.RTC_Year+2000,
                                            stDate->RtcDate.RTC_Month,
                                            stDate->RtcDate.RTC_Date,
                                            stDate->RtcTime.RTC_Hours,
                                            stDate->RtcTime.RTC_Minutes,
                                            stDate->RtcTime.RTC_Seconds);
#endif
}

void GetUTCTimeforDate(stHalRTCTypeDef* pstDate)
{
    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)pstDate, sizeof(stHalRTCTypeDef), 0);

#if false
    Trace(" Message Occurred Time: %04d/%02d/%02d,%02d:%02d:%02d\n", stDate.RtcDate.RTC_Year+2000,
                                            stDate->RtcDate.RTC_Month,
                                            stDate->RtcDate.RTC_Date,
                                            stDate->RtcTime.RTC_Hours,
                                            stDate->RtcTime.RTC_Minutes,
                                            stDate->RtcTime.RTC_Seconds);
#endif
}

uint32_t ConvertRTC2Seconds(stHalRTC_TimeTypeDef stTime,stHalRTC_DateTypeDef stDate)
{
    uint32_t wTime;
    uint32_t wTemp;

    wTime = stTime.RTC_Hours * 60 * 60;
    wTime += (stTime.RTC_Minutes * 60);
    wTime += stTime.RTC_Seconds;

    wTemp = wTime / 3600;
    stTime.RTC_Hours = wTemp % 24;
    wTime = wTime % 3600;
    stTime.RTC_Minutes = wTime / 60;
    stTime.RTC_Seconds = wTime % 60;

    Trace("wTime: %d(sec)\n",wTime);

    return wTime;
}

static unsigned short days[4][12] =
{
    {   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335},
    { 366, 397, 425, 456, 486, 517, 547, 578, 609, 639, 670, 700},
    { 731, 762, 790, 821, 851, 882, 912, 943, 974,1004,1035,1065},
    {1096,1127,1155,1186,1216,1247,1277,1308,1339,1369,1400,1430},
};


unsigned int GetTimefromDate(stHalRTCTypeDef stRtcInfo)
{
    unsigned int second = stRtcInfo.RtcTime.RTC_Seconds;  // 0-59
    unsigned int minute = stRtcInfo.RtcTime.RTC_Minutes;  // 0-59
    unsigned int hour   = stRtcInfo.RtcTime.RTC_Hours;    // 0-23
    unsigned int day    = stRtcInfo.RtcDate.RTC_Date-1;   // 0-30
    unsigned int month  = stRtcInfo.RtcDate.RTC_Month-1; // 0-11
    unsigned int year   = stRtcInfo.RtcDate.RTC_Year;    // 0-99
    return (((year/4*(365*4+1)+days[year%4][month]+day)*24+hour)*60+minute)*60+second;
}

void GetDatefromTime(stHalRTCTypeDef* stDateTime,unsigned int binary)
{
    stDateTime->RtcTime.RTC_Seconds = binary%60; binary /= 60;
    stDateTime->RtcTime.RTC_Minutes = binary%60; binary /= 60;
    stDateTime->RtcTime.RTC_Hours = binary%24; binary /= 24;

    unsigned int years = binary/(365*4+1)*4; binary %= 365*4+1;

    unsigned int year;
    for (year=3; year>0; year--)
    {
        if (binary >= days[year][0])
            break;
    }

    unsigned int month;
    for (month=11; month>0; month--)
    {
        if (binary >= days[year][month])
            break;
    }

    stDateTime->RtcDate.RTC_Year = years+year;
    stDateTime->RtcDate.RTC_Month = month+1;
    stDateTime->RtcDate.RTC_Date = binary-days[year][month]+1;
}

// check site : https://www.epochconverter.com/
// reference https://www.oryx-embedded.com/doc/date__time_8c_source.html
void GetDatefromTime2(stHalRTCTypeDef* pstDate, uint32_t nTime)
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;
    
    if(nTime < 1)        nTime = 0;

    //Retrieve hours, minutes and seconds
    pstDate->RtcTime.RTC_Seconds = nTime % 60;
    nTime /= 60;
    pstDate->RtcTime.RTC_Minutes = nTime % 60;
    nTime /= 60;
    pstDate->RtcTime.RTC_Hours = nTime % 24;
    nTime /= 24;

    //Convert Unix time to date
    a = (uint32_t) ((4 * nTime + 102032) / 146097 + 15);
    b = (uint32_t) (nTime + 2442113 + a - (a / 4));
    c = (20 * b - 2442) / 7305;
    d = b - 365 * c - (c / 4);
    e = d * 1000 / 30601;
    f = d - e * 30 - e * 601 / 1000;

    //January and February are counted as months 13 and 14 of the previous year
    if(e <= 13)
    {
        c -= 4716;
        e -= 1;
    }
    else
    {
        c -= 4715;
        e -= 13;
    }

    //Retrieve year, month and day
    pstDate->RtcDate.RTC_Year = c-2000;
    pstDate->RtcDate.RTC_Month = e;
    pstDate->RtcDate.RTC_Date = f;

    //Calculate day of week
    pstDate->RtcDate.RTC_WeekDay = computeDayOfWeek(c, e, f);
    pstDate->RtcTime.RTC_H12 = HAL_RTC_H24H;
}

uint32_t GetTimefromDate2(stHalRTCTypeDef stDate)
{
    uint32_t y;
    uint32_t m;
    uint32_t d;
    uint32_t t;

    //Year
    y = stDate.RtcDate.RTC_Year+2000;
    //Month of year
    m = stDate.RtcDate.RTC_Month;
    //Day of month
    d = stDate.RtcDate.RTC_Date;

    //January and February are counted as months 13 and 14 of the previous year
    if(m <= 2)
    {
        m += 12;
        y -= 1;
    }

    //Convert years to days
    t = (365 * y) + (y / 4) - (y / 100) + (y / 400);
    //Convert months to days
    t += (30 * m) + (3 * (m + 1) / 5) + d;
    //Unix time starts on January 1st, 1970
    t -= 719561;
    //Convert days to seconds
    t *= 86400;
    //Add hours, minutes and seconds
    t += (3600 * stDate.RtcTime.RTC_Hours) + (60 * stDate.RtcTime.RTC_Minutes) + stDate.RtcTime.RTC_Seconds;
    //Return Unix time
    return t;
}

uint8_t computeDayOfWeek(uint16_t y, uint8_t m, uint8_t d)
{
    uint32_t h;
    uint32_t j;
    uint32_t k;

    //January and February are counted as months 13 and 14 of the previous year
    if(m <= 2)
    {
        m += 12;
        y -= 1;
    }

    //J is the century
    j = y / 100;

    //K the year of the century
    k = y % 100;

    //Compute H using Zeller's congruence
    h = d + (26 * (m + 1) / 10) + k + (k / 4) + (5 * j) + (j / 4);

    //Return the day of the week

    return ((h + 5) % 7) + 1;
}

uint32_t GetTimeofDate(stHalRTC_TimeTypeDef stTime,stHalRTC_DateTypeDef stDate)
{
    stHalRTCTypeDef stDateTime;

    memcpy((char*)&stDateTime,(char*)&stTime,sizeof(stHalRTCTypeDef));


    Trace("%s]Time: %04d/%02d/%02d,%02d:%02d:%02d\n", __FUNCTION__,
                                            stDateTime.RtcDate.RTC_Year+2000,
                                            stDateTime.RtcDate.RTC_Month,
                                            stDateTime.RtcDate.RTC_Date,
                                            stDateTime.RtcTime.RTC_Hours,
                                            stDateTime.RtcTime.RTC_Minutes,
                                            stDateTime.RtcTime.RTC_Seconds);

    unsigned int nTime = GetTimefromDate2(stDateTime);

    return nTime;
}

uint32_t GetUtcTimeofDate(stHalRTC_TimeTypeDef stTime, stHalRTC_DateTypeDef stDate)
{
    stHalRTCTypeDef stDateTime;
    ConvertTime_LocalToUTC(stTime, stDate, &stDateTime.RtcTime, &stDateTime.RtcDate);
    Trace("%s]Time: %04d/%02d/%02d,%02d:%02d:%02d\n", __FUNCTION__,
                                            stDateTime.RtcDate.RTC_Year+2000,
                                            stDateTime.RtcDate.RTC_Month,
                                            stDateTime.RtcDate.RTC_Date,
                                            stDateTime.RtcTime.RTC_Hours,
                                            stDateTime.RtcTime.RTC_Minutes,
                                            stDateTime.RtcTime.RTC_Seconds);


    unsigned int nTime = GetTimefromDate2(stDateTime);

    return nTime;
}

uint32_t GetUtcTimefromTime(uint32_t unLocalTime)
{
    uint32_t unUtcTime;

    ConvertLocal2UtcTime(unLocalTime, &unUtcTime);

#if false
    DisplayTime("Local Time : ",unLocalTime);
    DisplayTime("Utc Time : ",unUtcTime);
#endif

    return unUtcTime;
}

void DisplayTime(char* pstr, uint32_t unTime)
{
    char carrBuf[64]={0,};
    struct tm* ptmTime;

    ptmTime = localtime((time_t*)&unTime);
    strftime(carrBuf,sizeof(carrBuf),"%Y %m %d %H %M %S %z", ptmTime);
    Trace("%s, Local Time : %s\r\n", pstr, carrBuf);
}

void ConvertUTC2LocalTime(uint32_t unUTCTime, uint32_t* punLocalTime)
{
    stNetworkTime stNetworkDate;

    // get time zone from network info field
    //GetBackupRamConfigProperty(eBackupRamConfig_NetworkInfo,(void*)&stNetworkDate);
    GetAutolinkConfigProperty(eAutoLinkConfig_NetworkInfo,(void*)&stNetworkDate);

    // 1 time zone unit is 15
    // multiply with 60 that is one hour.
    unUTCTime += ((stNetworkDate.sTimeZone*15)*60);

    *punLocalTime = unUTCTime;
}

uint32_t GetLocalTimefromTime(uint32_t unUTCTime)
{
    uint32_t unLocalTime;

    ConvertUTC2LocalTime(unUTCTime, &unLocalTime);

#if false
    DisplayTime("Local Time : ",unLocalTime);
    DisplayTime("Utc Time : ",unUTCTime);
#endif

    return unLocalTime;
}

uint32_t GetUTCTime()
{
    stHalRTCTypeDef stHalRtcDateTime;

    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRTCTypeDef), 0);
#if defined TIME_CHECK	
	uint8_t arrTmp[64];
	sprintf((char *)arrTmp, "%04d%02d%02d%0.2d%0.2d%0.2d\x00",
					2000 + stHalRtcDateTime.RtcDate.RTC_Year,
					stHalRtcDateTime.RtcDate.RTC_Month,
					stHalRtcDateTime.RtcDate.RTC_Date,
					stHalRtcDateTime.RtcTime.RTC_Hours,
					stHalRtcDateTime.RtcTime.RTC_Minutes,
					stHalRtcDateTime.RtcTime.RTC_Seconds);
	printf("MSG: GetLocal: [%s]\n", arrTmp);
#endif
    return GetTimefromDate2(stHalRtcDateTime);
}

//RTC수정
void ConvertTime_UTCToLocal(stHalRTC_TimeTypeDef stUTCTime, stHalRTC_DateTypeDef stUTCDate, stHalRTC_TimeTypeDef *pLocalTime, stHalRTC_DateTypeDef *pLocalDate)
{
	struct tm strCurrTime, *ptrTime;
	time_t currTime;
	int32_t nTimeZone;

    strCurrTime.tm_sec = stUTCTime.RTC_Seconds;
    strCurrTime.tm_min = stUTCTime.RTC_Minutes;
    strCurrTime.tm_hour = stUTCTime.RTC_Hours;

    strCurrTime.tm_mday = stUTCDate.RTC_Date;
    strCurrTime.tm_mon = stUTCDate.RTC_Month - 1;
    strCurrTime.tm_year = 2000 + stUTCDate.RTC_Year - 1900;
    strCurrTime.tm_wday = 0;				// 요일(0 ~ 6)				// 가능하면 정의해 주자. 모르면 0이라도...
    strCurrTime.tm_yday = 0;				// day in the year
    strCurrTime.tm_isdst = 0;				// 서머 타임 시간

	currTime = mktime(&strCurrTime);
	//Trace("currTime(GMT %-d): %d\n", BkSram_ModemInfo.snTimeZone, currTime);

	nTimeZone = (BkSram_ModemInfo.snTimeZone * 15) * 60;				// 위에서 받아온 time zone 은 15분 단위

	currTime = currTime + nTimeZone;


	//Trace("currTime(UTC %-d): %d\n", BkSram_ModemInfo.snTimeZone, currTime);

	ptrTime	= localtime(&currTime);

    //*********************************************************************
    pLocalDate->RTC_Date = ptrTime->tm_mday;
    pLocalDate->RTC_Month = ptrTime->tm_mon + 1;
    pLocalDate->RTC_Year= ptrTime->tm_year + 1900 - 2000;
    pLocalDate->RTC_WeekDay = 1;

    pLocalTime->RTC_H12 = HAL_RTC_H24H;
    pLocalTime->RTC_Seconds = ptrTime->tm_sec;
    pLocalTime->RTC_Minutes = ptrTime->tm_min;
    pLocalTime->RTC_Hours = ptrTime->tm_hour;

	Trace("UTC: %04d/%02d/%02d,%02d:%02d:%02d\n", pLocalDate->RTC_Year+2000,
												pLocalDate->RTC_Month,
												pLocalDate->RTC_Date,
												pLocalTime->RTC_Hours,
												pLocalTime->RTC_Minutes,
												pLocalTime->RTC_Seconds);
}

void ConvertTime_LocalToUTC(stHalRTC_TimeTypeDef stLocalTime, stHalRTC_DateTypeDef stLocalDate, stHalRTC_TimeTypeDef *pUTCTime, stHalRTC_DateTypeDef *pUTCDate)
{
	struct tm strCurrTime, *ptrTime;
	time_t currTime;
	int32_t nTimeZone;

    strCurrTime.tm_sec = stLocalTime.RTC_Seconds;
    strCurrTime.tm_min = stLocalTime.RTC_Minutes;
    strCurrTime.tm_hour = stLocalTime.RTC_Hours;

    strCurrTime.tm_mday = stLocalDate.RTC_Date;
    strCurrTime.tm_mon = stLocalDate.RTC_Month - 1;
    strCurrTime.tm_year = 2000 + stLocalDate.RTC_Year - 1900;
    strCurrTime.tm_wday = 0;				// 요일(0 ~ 6)				// 가능하면 정의해 주자. 모르면 0이라도...
    strCurrTime.tm_yday = 0;				// day in the year
    strCurrTime.tm_isdst = 0;				// 서머 타임 시간

	currTime = mktime(&strCurrTime);
	//Trace("currTime(GMT %-d): %d\n", BkSram_ModemInfo.snTimeZone, currTime);

	nTimeZone = (BkSram_ModemInfo.snTimeZone * 15) * 60;				// 위에서 받아온 time zone 은 15분 단위

	if(BkSram_ModemInfo.snTimeZone > 0) {
		currTime = currTime - nTimeZone;
	}
	else {
		currTime = currTime + (nTimeZone * -1);
	}

	//Trace("currTime(UTC %-d): %d\n", BkSram_ModemInfo.snTimeZone, currTime);

	ptrTime	= localtime(&currTime);

    //*********************************************************************
    pUTCDate->RTC_Date = ptrTime->tm_mday;
    pUTCDate->RTC_Month = ptrTime->tm_mon + 1;
    pUTCDate->RTC_Year= ptrTime->tm_year + 1900 - 2000;
    pUTCDate->RTC_WeekDay = 1;

    pUTCTime->RTC_H12 = HAL_RTC_H24H;
    pUTCTime->RTC_Seconds = ptrTime->tm_sec;
    pUTCTime->RTC_Minutes = ptrTime->tm_min;
    pUTCTime->RTC_Hours = ptrTime->tm_hour;

	Trace("UTC: %04d/%02d/%02d,%02d:%02d:%02d\n", pUTCDate->RTC_Year+2000,
												pUTCDate->RTC_Month,
												pUTCDate->RTC_Date,
												pUTCTime->RTC_Hours,
												pUTCTime->RTC_Minutes,
												pUTCTime->RTC_Seconds);
}

long long diff_tm(struct tm *a, struct tm *b)
{
  return a->tm_sec - b->tm_sec
          +60LL*(a->tm_min - b->tm_min)
          +3600LL*(a->tm_hour - b->tm_hour)
          +86400LL*(a->tm_yday - b->tm_yday)
          +(a->tm_year-70)*31536000LL
          -(a->tm_year-69)/4*86400LL
          +(a->tm_year-1)/100*86400LL
          -(a->tm_year+299)/400*86400LL
          -(b->tm_year-70)*31536000LL
          +(b->tm_year-69)/4*86400LL
          -(b->tm_year-1)/100*86400LL
          +(b->tm_year+299)/400*86400LL;
}

//           struct tm {
//             int tm_sec;         /* seconds */
//             int tm_min;         /* minutes */
//             int tm_hour;        /* hours */
//             int tm_mday;        /* day of the month */
//             int tm_mon;         /* month */
//             int tm_year;        /* year */
//             int tm_wday;        /* day of the week */
//             int tm_yday;        /* day in the year */
//             int tm_isdst;       /* daylight saving time */
//         };

void ConvertRtc2Tm(stHalRTCTypeDef stDate,struct tm* pstrCurrTime)
{
    pstrCurrTime->tm_year = stDate.RtcDate.RTC_Year + 2000 - 1900; // it is checked from 1900
    pstrCurrTime->tm_mon = stDate.RtcDate.RTC_Month - 1;           // month is start at 0 so we mius 1.
    pstrCurrTime->tm_mday = stDate.RtcDate.RTC_Date;
    pstrCurrTime->tm_hour = stDate.RtcTime.RTC_Hours;
    pstrCurrTime->tm_min = stDate.RtcTime.RTC_Minutes;
    pstrCurrTime->tm_sec = stDate.RtcTime.RTC_Seconds;
    pstrCurrTime->tm_isdst = -1;
}

//RTC수정
uint32_t ConvertUtc2LocalTime(uint32_t unUtcTime)
{
    uint32_t unLocalTime;
    stNetworkTime stNetworkDate;

    // get time zone from network info field
    //GetBackupRamConfigProperty(eBackupRamConfig_NetworkInfo,(void*)&stNetworkDate);
    GetAutolinkConfigProperty(eAutoLinkConfig_NetworkInfo,(void*)&stNetworkDate);

    // 1 time zone unit is 15
    // multiply with 60 that is one hour.
    unLocalTime = unUtcTime + ((stNetworkDate.sTimeZone*15)*60);

    return unLocalTime;
}

uint32_t ConvertLocal2UTCTime(uint32_t unLocalTime)
{
    uint32_t unUTCTime;
    stNetworkTime stNetworkDate;

    // get time zone from network info field
    //GetBackupRamConfigProperty(eBackupRamConfig_NetworkInfo,(void*)&stNetworkDate);
    GetAutolinkConfigProperty(eAutoLinkConfig_NetworkInfo,(void*)&stNetworkDate);

    // 1 time zone unit is 15
    // multiply with 60 that is one hour.
    unUTCTime = unLocalTime - ((stNetworkDate.sTimeZone*15)*60);

    return unUTCTime;
}

void ConvertLocal2UtcTime(uint32_t unLocalTime, uint32_t* punUtcTime)
{
    stNetworkTime stNetworkDate;

    // get time zone from network info field
    //GetBackupRamConfigProperty(eBackupRamConfig_NetworkInfo,(void*)&stNetworkDate);
    GetAutolinkConfigProperty(eAutoLinkConfig_NetworkInfo,(void*)&stNetworkDate);

    // 1 time zone unit is 15
    // multiply with 60 that is one hour.
    unLocalTime -= ((stNetworkDate.sTimeZone*15)*60);

    *punUtcTime = unLocalTime;
}

void ConvertUtc2LocalTimeTest()
{
    stHalRTCTypeDef stUtcDate;
    GetLocalTimeforDate(&stUtcDate);

    //ts = localtime(&now); // 지역표준시로 변환한다 (대한민국은 KST)
    //ts = gmtime(&now);  // 국제표준시 GMT로 변환한다
    //strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts)
    time_t utc, local;

    struct tm tp;
    struct tm *localt;

    memset((char*)&tp, 0, sizeof(struct tm));
    memset((char*)&localt, 0, sizeof(struct tm));

    // set up utc time
    tp.tm_year = stUtcDate.RtcDate.RTC_Year+2000;
    tp.tm_mon = stUtcDate.RtcDate.RTC_Month;
    tp.tm_mday = stUtcDate.RtcDate.RTC_Date;
    tp.tm_hour = stUtcDate.RtcTime.RTC_Hours;
    tp.tm_min = stUtcDate.RtcTime.RTC_Minutes;
    tp.tm_sec = stUtcDate.RtcTime.RTC_Seconds;

    /* put values of datetime into time structure *tp */
    //strptime(datetime, "%Y %m %d %H %M %S %z", tp);

    /* get seconds since EPOCH for this time */
    utc=mktime(&tp);
    Trace("UTC date and time in seconds since EPOCH1: %d\n", utc);
//    printf("UTC date and time in seconds since EPOCH2: %d\n", utcTime);


    /* lets convert this UTC date and time to local date and time */
    struct tm e0={ .tm_year = 70, .tm_mday = 1 }, e1/*, new*/;
    /* get time_t EPOCH value for e0 (Jan. 1, 1970) */
    time_t pseudo=mktime(&e0);

    /* get gmtime for this value */
    e1=*gmtime(&pseudo);

    /* calculate local time in seconds since EPOCH */
    e0.tm_sec += utc - diff_tm(&e1, &e0);

    /* assign to local, this can all can be coded shorter but I attempted to increase clarity */
    local=e0.tm_sec;
    Trace("local date and time in seconds since EPOCH: %d\n", local);

    /* convert seconds since EPOCH for local time into localt time structure */
    //localt=localtime(&local);
    DisplayTime("local date and time: ", local);
}

