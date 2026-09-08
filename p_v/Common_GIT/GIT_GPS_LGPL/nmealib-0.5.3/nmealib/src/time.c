#include "nmea/time.h"
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: time.c 4 2007-08-27 13:11:03Z xtimor $
 *
 */

/*! \file time.h */
#include "main.h"

#ifdef NMEA_WIN
#   pragma warning(disable: 4201)
#   pragma warning(disable: 4214)
#   pragma warning(disable: 4115)
#   include <windows.h>
#   pragma warning(default: 4201)
#   pragma warning(default: 4214)
#   pragma warning(default: 4115)
#else
#   include <time.h>
#endif

#ifdef NMEA_WIN

void nmea_time_now(nmeaTIME *stm)
{
    SYSTEMTIME st;

    GetSystemTime(&st);

    stm->year = st.wYear - 1900;
    stm->mon = st.wMonth - 1;
    stm->day = st.wDay;
    stm->hour = st.wHour;
    stm->min = st.wMinute;
    stm->sec = st.wSecond;
    stm->hsec = st.wMilliseconds / 10;
}

#else /* NMEA_WIN */

void nmea_time_now(nmeaTIME *stm)
{
    struct tm tt;

#if defined(STM32F427X) || defined(AT32F435VMT7)
    stHalRTCTypeDef stTimeVar;
    
    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stTimeVar, sizeof(stHalRTCTypeDef), 0);

	tt.tm_year = stTimeVar.RtcDate.RTC_Year + 100;
	tt.tm_mon = stTimeVar.RtcDate.RTC_Month - 1;
	tt.tm_mday = stTimeVar.RtcDate.RTC_Date;
	tt.tm_hour = stTimeVar.RtcTime.RTC_Hours;
	tt.tm_min = stTimeVar.RtcTime.RTC_Minutes;
	tt.tm_sec = stTimeVar.RtcTime.RTC_Seconds;
#else
    time_t lt;

	time(&lt);
	tt = gmtime(&lt);
#endif
  

    stm->year = tt.tm_year;
    stm->mon = tt.tm_mon;
    stm->day = tt.tm_mday;
    stm->hour = tt.tm_hour;
    stm->min = tt.tm_min;
    stm->sec = tt.tm_sec;
    stm->hsec = 0;
}

#endif
