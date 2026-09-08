/*************************************************************
 * NOTE : git_rtc.c
 *      RTC
 * Author : Lee woohee
 * Since : 2019.09.26
**************************************************************/
#include "main.h"
#include "common.h"

#include "git_rtc.h"
#include "rtc.h"

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
typedef struct __stRtcDate
{
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
} stRtcDate;
/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void Get_RTCData( uint8_t * datetime )
{
	RTC_DateTypeDef	sDate;
	RTC_TimeTypeDef	sTime;

	HAL_RTC_GetTime( &hrtc, &sTime, RTC_FORMAT_BIN );
	HAL_RTC_GetDate( &hrtc, &sDate, RTC_FORMAT_BIN );

	datetime[0] = sDate.WeekDay;
	datetime[1] = sDate.Year;
	datetime[2] = sDate.Month;
	datetime[3] = sDate.Date;
	datetime[4] = sTime.Hours;
	datetime[5] = sTime.Minutes;
	datetime[6] = sTime.Seconds;

	GLogI( "Get RTC Date : " );
	GLogI( "%.2d:%.2d:%.2d ", sTime.Hours, sTime.Minutes, sTime.Seconds);
	GLogI( "%.2d-%.2d-%.2d\r\n", sDate.Month, sDate.Date, 2000 + sDate.Year);
}

void Set_RTCData( uint8_t * datetime, uint8_t convert )
{
	RTC_DateTypeDef	sDate = {0,};
	RTC_TimeTypeDef	sTime = {0,};

	if( convert )
	{
		sDate.Year		= HexToDec(datetime[0]);		//Year 20XX
		sDate.Month 	= HexToDec(datetime[1]);		//Month
		sDate.Date		= HexToDec(datetime[2]);		//Day
		sTime.Hours 	= HexToDec(datetime[3]-0x80);	//Hour
		sTime.Minutes	= HexToDec(datetime[4]);		//Minute
		sTime.Seconds	= HexToDec(datetime[5]);		//Sec
	}
	else
	{
		sDate.Year		= datetime[0];	//Year 20XX
		sDate.Month 	= datetime[1];	//Month
		sDate.Date		= datetime[2];	//Day
		sTime.Hours 	= datetime[3];	//Hour
		sTime.Minutes	= datetime[4];	//Minute
		sTime.Seconds	= datetime[5];	//Sec
	}
	sDate.WeekDay	= 0;

	HAL_RTC_SetTime( &hrtc, &sTime, RTC_FORMAT_BIN );
	HAL_RTC_SetDate( &hrtc, &sDate, RTC_FORMAT_BIN );

	GLogI( "Write RTC Date : " );
	GLogI( "%.2d:%.2d:%.2d ", sTime.Hours, sTime.Minutes, sTime.Seconds);
	GLogI( "%.2d-%.2d-%.2d\r\n", sDate.Month, sDate.Date, 2000 + sDate.Year);
}

uint32_t GetUnixTime( void )
{
	stRtcDate	stDate;

	uint32_t	year;
	uint32_t	month;
	uint32_t	day;
	uint32_t	time;

	HAL_RTC_GetTime( &hrtc, &stDate.time, RTC_FORMAT_BIN );
	HAL_RTC_GetDate( &hrtc, &stDate.date, RTC_FORMAT_BIN );

	//Year
	year	= stDate.date.Year + 2000;

	//Month of year
	month	= stDate.date.Month;

	//Day of month
	day		= stDate.date.Date;

	//January and February are counted as months 13 and 14 of the previous year
	if( month <= 2 )
	{
		month	+= 12;
		year	-= 1;
	}

	//Convert years to days
	time = (365 * year) + (year / 4) - (year / 100) + (year / 400);

	//Convert months to days
	time += (30 * month) + (3 * (month + 1) / 5) + day;

	//Unix time starts on January 1st, 1970
	time -= 719561;

	//Convert days to seconds
	time *= 86400;

	//Add hours, minutes and seconds
	time += (3600 * stDate.time.Hours) + (60 * stDate.time.Minutes) + stDate.time.Seconds;

	//Return Unix time
	return time;
}