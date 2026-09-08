/* Martin Thomas 4/2009 */

#include "fattime.h"
#include "HalHandler.h"

typedef union
{
	struct discreateTime
	{
		unsigned sec: 5;
		unsigned min: 6;
		unsigned hour: 5;
		unsigned day: 5;
		unsigned mon: 4;
		unsigned year: 7;
	} discreateTime;

	struct timeDate
	{
		uint16_t time;
		uint16_t date;
	} timeDate;
	uint32_t packedTime;
} SYS_FS_TIME;

DWORD get_fattime (void)
{
	SYS_FS_TIME time;
    stHalRTCTypeDef stHalRtcDateTime;

    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRtcDateTime), 0);

	time.discreateTime.year = 2000 + stHalRtcDateTime.RtcDate.RTC_Year - 1980;
	time.discreateTime.mon = stHalRtcDateTime.RtcDate.RTC_Month;
	time.discreateTime.day = stHalRtcDateTime.RtcDate.RTC_Date;
	time.discreateTime.hour = stHalRtcDateTime.RtcTime.RTC_Hours;
	time.discreateTime.min = stHalRtcDateTime.RtcTime.RTC_Minutes;
	time.discreateTime.sec = stHalRtcDateTime.RtcTime.RTC_Seconds;

	return time.packedTime;
}
