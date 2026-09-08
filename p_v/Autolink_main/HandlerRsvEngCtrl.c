#include "AutolinkMessage.h"
#include "AutolinkConfig.h"

#include "HalHandler.h"

#define MAX_RSVENGCTRL_CNT (5)

#include "HandlerRsvEngCtrl.h"
extern stUserActionSetting m_stUserActionSetting;
extern void GetUTCTimeforDate(stHalRTCTypeDef* pstDate);
extern uint32_t GetUTCTime();

boolean_t IsValidRsvEngCtrlGUID(uint8_t* pBuff)
{
#if 0
	uint8_t ucTmp[MAX_GUID_LENGTH];

	memset(ucTmp, 0x00, MAX_GUID_LENGTH);

	if(memcmp(pBuff, ucTmp, MAX_GUID_LENGTH)==0)
		return false;
#else
	for(int i=0;i<MAX_GUID_LENGTH;i++)
	{
		if(pBuff[i]==0)	return false;
	}
#endif
	return true;
}

void ShowPosixToGeneralTime(long lPosix)
{
	stHalRTCTypeDef stDate;
	
	GetDatefromTime2(&stDate, lPosix);
	printf("20%d/%02d/%02d %02d:%02d:%02d\r\n",
								stDate.RtcDate.RTC_Year,
								stDate.RtcDate.RTC_Month,
								stDate.RtcDate.RTC_Date,
								stDate.RtcTime.RTC_Hours,
								stDate.RtcTime.RTC_Minutes,
								stDate.RtcTime.RTC_Seconds);
}

void GetRsvEngCtrlSetting(stRsvEngCntorl* pstRsvEngCtrlSetting)
{
    memcpy((char*)pstRsvEngCtrlSetting,(char*)&m_stUserActionSetting.RsvEngCtrl,sizeof(stRsvEngCntorl));
}

void SetRsvEngCtrlSetting(stRsvEngCntorl stRsvEngCtrlValue)
{
    memcpy((char*)&m_stUserActionSetting.RsvEngCtrl,(char*)&stRsvEngCtrlValue,sizeof(stRsvEngCntorl));

    // save setting value 
    WriteConfig(false,true);
}

uint32_t GetSecUntiltDay()
{
	stHalRTCTypeDef stDate;
	uint32_t nUntilDay;

	GetUTCTimeforDate(&stDate);

	nUntilDay = GetTimefromDate2(stDate);
	nUntilDay = nUntilDay - (stDate.RtcTime.RTC_Hours * 3600) - (stDate.RtcTime.RTC_Minutes * 60) - stDate.RtcTime.RTC_Seconds;

	return nUntilDay;
}

boolean_t CheckRsvEngCtrlDate(stRsvEngCntorlInfo stRsvEngCtrlInfo)
{
	int i;
	int32_t nUTCTime=0;
	uint8_t ucArrDOW[9] = {0,1,2,4,8,16,32,64,128};
	stHalRTCTypeDef stDate;

	memset(&stDate, 0x00, sizeof(stHalRTCTypeDef));

	nUTCTime = GetUTCTime();
//	GetUtcTimefromTime(nLocalTime);
// ?????ð????? ???? check ?????μ? UTC?? ?????? ??°??
// ???? ?ð? Set ?????? ???? ?ð????? local time set ??? ??? ?? ??? ???
	GetDatefromTime2(&stDate, nUTCTime);

	if(stRsvEngCtrlInfo.bRepeat)
	{
		for(i=0; i<7; i++)
		{
			uint8_t ucBitMask = HAL_RTC_Weekday_Monday<<i;

			uint8_t ucDOW = stRsvEngCtrlInfo.ucRsvDayoftheweek&ucBitMask;
			
			if(ucArrDOW[stDate.RtcDate.RTC_WeekDay] == ucDOW)
				return true;
		}
	}

	if(stRsvEngCtrlInfo.bRepeat == 0x00)
	{
		uint32_t nRsvTime = stRsvEngCtrlInfo.nRsvDate;
		uint32_t nUntiltDay = GetSecUntiltDay();
		
		if(nRsvTime == nUntiltDay)
			return true;
	}

	return false;
}

uint8_t HandlerRsvEngCtrl()
{
	int i;
	uint32_t nCurrtHM, nCurrTime;
	stRsvEngCntorl stRsvEngCtrl;
	int nRsvTime;
	stHalRTCTypeDef stDate;

	GetRsvEngCtrlSetting(&stRsvEngCtrl);
 
	nCurrTime = GetUTCTime();

	// 4. UTC로 변환된 Date 구조체 획득
	GetDatefromTime2(&stDate, nCurrTime);

	// 5. 현재 시간 + 분 UTC 기준으로 sec로 변환
	nCurrtHM = (stDate.RtcTime.RTC_Hours * 3600) + (stDate.RtcTime.RTC_Minutes * 60);

	for(i=0; i<MAX_RSVENGCTRL_CNT; i++)
	{
		if( stRsvEngCtrl.stRsvEngCtrlInfo[i].bRsvEngCtrl == true )
		{
			if(CheckRsvEngCtrlDate(stRsvEngCtrl.stRsvEngCtrlInfo[i])) // 예약 날짜 확인
			{
				nRsvTime = stRsvEngCtrl.stRsvEngCtrlInfo[i].nRsvTime;

				if(nRsvTime == nCurrtHM)
					return i+1; // wakeup 된 순간 engine on

				if(nRsvTime - nCurrtHM < 10)
					return i+1; // 예약시간 보다 10초 이내면 시동 ON
		
				if(nCurrtHM - nRsvTime < 60 * 2)
					return eRsvEngOnInOneMin; // 예약 시간 2분 남아있으면 sleep 금지 후 시동 ON
			}
		}
	}
	
	return eRsvEngOnNotNow;

}

void CheckRsvEngControlInfo(stHalRTCTypeDef * pstDateTime, uint32_t *punNextWakeupTime, stRsvEngCntorl * pstRsvEngCtrl)
{
	stRsvEngCntorl stRsvEngCtrl = *pstRsvEngCtrl;
	stHalRTCTypeDef stOriginWakeupDateConvUTC;
	
	boolean_t bResult = false;
	int i;
	
	uint32_t nLastSetWakeupTime=0;
	uint32_t nWakeupTime=0;
	uint32_t unNextWakeupTime;
	uint32_t nRsvUTCTime=0;
	uint32_t nNextWakeupUTCTime=0;
	uint32_t nCalcDiffUTCTime=0;
	uint32_t nNextWakeupUTCTimeHM=0;
	uint32_t nNextWakeupUTCTimeYMD=0;
	
	static boolean_t s_bIsFirstChange = false;

	printf("----------Origin Wakeup Time ---------- : ");
	ShowPosixToGeneralTime(*punNextWakeupTime);	// 기존 wakeup 날짜 + 시간

	// 기존 wakeup 날짜 + 시간 - 초
	unNextWakeupTime = *punNextWakeupTime - pstDateTime->RtcTime.RTC_Seconds;
//	printf("unNextWakeupTime-Sec : ");
//	ShowPosixToGeneralTime(unNextWakeupTime);
//	printf("unNextWakeupTime -Sec = %d\r\n",unNextWakeupTime);

	// 기존 wkaeup 날짜 + 시간 - 초 -> UTC 변환
	//printf("----------Origin Wakeup Time - Sec UTC ---------- : ");
	nNextWakeupUTCTime = unNextWakeupTime;
	//ShowPosixToGeneralTime(nNextWakeupUTCTime);
	//printf("nNextWakeupUTCTime = %d\r\n", nNextWakeupUTCTime);

	memset(&stOriginWakeupDateConvUTC, 0x00, sizeof(stOriginWakeupDateConvUTC));
	GetDatefromTime2(&stOriginWakeupDateConvUTC, nNextWakeupUTCTime);
	nNextWakeupUTCTimeHM = (stOriginWakeupDateConvUTC.RtcTime.RTC_Hours*3600)+(stOriginWakeupDateConvUTC.RtcTime.RTC_Minutes*60);
//	printf("Origin wakeup hour, min UTC (%02d:%02d) \r\n",stOriginWakeupDateConvUTC.RtcTime.RTC_Hours, stOriginWakeupDateConvUTC.RtcTime.RTC_Minutes);
//	printf("nNextWakeupUTCTimeHM : %d\r\n", nNextWakeupUTCTimeHM);
//	printf("nNextWakeupUTCTimeYMD Posix : ");
	nNextWakeupUTCTimeYMD = nNextWakeupUTCTime - nNextWakeupUTCTimeHM;	
//	ShowPosixToGeneralTime(nNextWakeupUTCTimeYMD);
//	printf("nNextWakeupUTCTimeYMD : %d\r\n", nNextWakeupUTCTimeYMD);
	
	for(i=0; i<MAX_RSVENGCTRL_CNT; i++)
	{
		if(!stRsvEngCtrl.stRsvEngCtrlInfo[i].bRsvEngCtrl)
			continue;
			
		if(CheckRsvEngCtrlDate(stRsvEngCtrl.stRsvEngCtrlInfo[i]))
		{	
			nRsvUTCTime = stRsvEngCtrl.stRsvEngCtrlInfo[i].nRsvTime; // 수신 받은 예약 UTC 시간
			nCalcDiffUTCTime = nNextWakeupUTCTimeHM - nRsvUTCTime;
			printf("Reserve Time value: %d, Calc Diff time : %d\r\n", nRsvUTCTime, nCalcDiffUTCTime);
			
			if( (GetUTCTime()<( nNextWakeupUTCTimeYMD + nRsvUTCTime))&&(nCalcDiffUTCTime <= RTC_SET_WAKEUP_ALRAM_TIME) && (nCalcDiffUTCTime > 0) ) //1h
			{
				printf("----------Change Wakeup Time----------\r\n");
				bResult = true;
				nWakeupTime = nNextWakeupUTCTimeYMD + nRsvUTCTime;
				ShowPosixToGeneralTime(nWakeupTime);
				printf("--------------------------------------\r\n");
			}

			if(bResult)
			{
				if(nLastSetWakeupTime > nWakeupTime || s_bIsFirstChange == false )
				{
					*punNextWakeupTime = nWakeupTime;
					nLastSetWakeupTime = nWakeupTime;
					bResult = false;
					s_bIsFirstChange = true;
				}
			}
		}
	}

	s_bIsFirstChange = false;

}
