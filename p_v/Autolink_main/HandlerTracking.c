#include "HandlerTracking.h"
#include "MngSystem.h"

extern stUserActionSetting m_stUserActionSetting;
extern uint32_t GetUTCTime();
extern void SetPostponeSleepFlag(boolean_t bPostpone);
extern int16_t CalcChecksum_Short(unsigned char* pBuff, unsigned int uiLength);

void GetTrackingSetting(stTrackingControl* pstTrackingValue)
{
    memcpy((char*)pstTrackingValue, (char*)&m_stUserActionSetting.Tracking,sizeof(stTrackingControl));
}

void SetTrackingSetting(stTrackingControl* pstTrackingValue)
{
	if( pstTrackingValue != NULL )
	{
    	memcpy((char*)&m_stUserActionSetting.Tracking,(char*)pstTrackingValue,sizeof(stTrackingControl));

		m_stUserActionSetting.Tracking.unActiveTime = GetUTCTime();
		if(m_stUserActionSetting.Tracking.bActive)
		{
			m_stUserActionSetting.Tracking.ucDataIndex = 0;
		}
		m_stUserActionSetting.Tracking.usCheckSum = CalcChecksum_Short(&m_stUserActionSetting.Tracking.bActive,sizeof(m_stUserActionSetting.Tracking)-2);

		printf("%s] Tracking Active : %d\n", __FUNCTION__,m_stUserActionSetting.Tracking.bActive);
		printf("%s] Tracking Start Time : %d\n", __FUNCTION__,m_stUserActionSetting.Tracking.unActiveTime);

	    // save setting value
    	WriteConfig(false,true);

		//ClearSleepProcess();
	}
}

boolean_t GetTrackingActive()
{
	return m_stUserActionSetting.Tracking.bActive;
}

void DispTrackingSetting()
{
	stTrackingControl stDispTrackingControl;
	GetTrackingSetting(&stDispTrackingControl);

	printf(" [Disp Tracking] Active : %d \n",stDispTrackingControl.bActive);
	printf(" [Disp Tracking] bDurationMinTime : %d \n",stDispTrackingControl.ucDurationMinTime);
	printf(" [Disp Tracking] bIntervalSec : %d \n",stDispTrackingControl.ucIntervalSec);
}


void CheckTracking()
{
    static unsigned long s_ulTrackingTimer = 0;
	stCarReport report;
	
	if( (Get_Tmr() - s_ulTrackingTimer) > (m_stUserActionSetting.Tracking.ucIntervalSec * ONE_SECOND) )		
	{
		uint32_t unCurTime = GetUTCTime();

		printf("Report Tracking\n");
		// report tracking event to server
		memcpy(&report.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucArrGUID,&m_stUserActionSetting.Tracking.ucArrGUID,MAX_GUID_LENGTH);
        report.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucDataIndex = m_stUserActionSetting.Tracking.ucDataIndex;
		report.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.dLatitude = Get_GPS_Lat();
		report.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.dLongitude = Get_GPS_Lon();
		report.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucGpsValid = Get_GPS_Vailication();
		
		if(m_stUserActionSetting.Tracking.ucDataIndex >= 255)
			m_stUserActionSetting.Tracking.ucDataIndex = 0;
		else
			m_stUserActionSetting.Tracking.ucDataIndex++;
		SetPostponeSleepFlag(true);
		Send2MngSysMsg(eMngSysMsg,eReqReport,eR_TrackingReport,&report,0);

		// check tracking mode time out
		if( (( unCurTime - m_stUserActionSetting.Tracking.unActiveTime ) > (m_stUserActionSetting.Tracking.ucDurationMinTime * ONE_MINUTE_SEC)))
		{
			m_stUserActionSetting.Tracking.bActive = false;
			m_stUserActionSetting.Tracking.unActiveTime = 0;
			m_stUserActionSetting.Tracking.ucDataIndex = 0;
			SetPostponeSleepFlag(false);
		}
		s_ulTrackingTimer = Get_Tmr();
	}
}

void GetTrackingActiveGuid(uint8_t* pcArrGuid)
{
	if( pcArrGuid != NULL )
	{
		memcpy(pcArrGuid,m_stUserActionSetting.Tracking.ucArrGUID, MAX_GUID_LENGTH);
	}
}

uint8_t VerifyTrackingControl(stTrackingControl* pstTrackingValue)
{
	uint8_t ucErrorCode = eTM_REASON_NONE;
	
	stTrackingControl stCheckTracking;
	memcpy((char*)&stCheckTracking,(char*)pstTrackingValue,sizeof(stTrackingControl));

	if(GetTrackingActive() == eTM_ACTIVE_ON)
	{
		if(stCheckTracking.bActive)
		{
			ucErrorCode |= eTM_REASON_ACTIVE_OPERATING;
		}
	}

	if(!(stCheckTracking.bActive == eTM_ACTIVE_OFF || stCheckTracking.bActive == eTM_ACTIVE_ON))
	{
		ucErrorCode |= eTM_REASON_ACTIVE_NOTRANGE;
	}


	if(!(stCheckTracking.ucDurationMinTime >= 1 || stCheckTracking.ucDurationMinTime <= 10))
	{
		ucErrorCode |= eTM_REASON_DURATION_NOTRANGE;
	}

	if(!(stCheckTracking.ucIntervalSec >= 10 || stCheckTracking.ucIntervalSec <= 50))
	{
		ucErrorCode |= eTM_REASON_INTERVAL_NOTRANGE;
	}

	return ucErrorCode;
	
}

