/* OBD Controller */

//#include "stdlib.h"
#include "GIT_BluetoothLowEnergy.h"
#include "HalHandler.h"

#include "AutolinkConfiguration.h"
#include "OBD_Controller.h"
#include "OBD_Controller_Get.h"
#include "OBD_Manager.h"

#include "time.h"
#include "math.h"
#include "GIT_Util.h"
#include "GIT_Gps.h"

#include "MngSystemUtil.h"
#include "MngSystem.h"
#include "Share_InterFunction.h"
#include "Modem_Manager.h"
#include "CanFD_Controller.h"
#if defined(FEATURE_EXTENSION_BOARD)
#include "SysPsExtendHdEvent.h"
#endif


#include "Share_InterFunction.h"
#include "HandlerRsvEngCtrl.h"
#include "MngModem.h"

//#define   DEBUG_ALARM_ALL


//#define DISTANCE
#define X_AXIS 0
#define Y_AXIS 1
#define TCUSLOPE_MAX_COUNT 10
#define SLOPE_TCU_LOG_FLAG 0

extern eOBD_STATE	g_eOBDState;
OBD_CONTROLLER_DATA g_OBDControllerData;
DOOROPENALRAMCHECK g_stDoorOpenAlramCheckState;
DOORUNLOCKALRAMCHECK g_stDoorUnlockAlramCheckState;
TPMSALRAMCHECK	g_stTPMSAlramCheckState;
bool g_bCrashAlramFlag=true;
bool g_bMILLampAlramFlag=true;
bool g_Fahrenheit_UpdatedFlag=false;


#if defined(PROTOCOL18)
#define EXTRA_CHARGE_TIME_REPORT_PERIOD       1*60
#endif
extern uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
extern uint32_t GetUTCTime();
extern void GetDatefromTime2(stHalRTCTypeDef* stDate,uint32_t unTime);
extern boolean_t GetBlockMsgTrasfer();
extern void GetSystemDrivingKey(long long* value);
extern eSET_OFFSET_STATE	g_eSET_OFFSET_STATE;
extern eFUEL_TYPE	g_eFuel_Type;
extern stIndicatorCheck g_stIndicatorCheck[ePOS_INDICATOR_TIMER_MAX];
extern bool g_bIndicatorDBFlag;
extern stBrakeJudder g_stBrakeJudder;
/* Local func */
extern void ConvertTime_LocalToUTC(stHalRTC_TimeTypeDef stLocalTime, stHalRTC_DateTypeDef stLocalDate, stHalRTC_TimeTypeDef *pUTCTime, stHalRTC_DateTypeDef *pUTCDate);
extern   bool g_bEngineIdleFlag;
extern   bool g_bWarmupFlag;
extern   bool g_bFCSRunFlag;
extern bool g_bFATCRunActFlag;
extern   bool g_bIndicatorFirstCheckFlag;
extern   bool g_bFreezeFrameRunFlag;
extern unsigned int g_uiEngRunStartTime;
extern unsigned int g_uiEnergyOldTime;
extern unsigned int g_uiFuelOldTime;
extern unsigned int g_uiCalFuelConsumptionOldTime;
extern bool g_bEngRunActFlag;
extern void GetStartingDateTimeStamp(uint8_t *ptr, stHalRTC_DateTypeDef DateStamp, stHalRTC_TimeTypeDef TimeStamp);
extern int ReportBeforeDriving();
extern int ReportAfterDriving();
extern void SetObdDrivingKey(long long value);
extern double Get_GPS_Lat_Origin();
extern double Get_GPS_Lon_Origin();
extern uint32_t GetTimeofDate(stHalRTC_TimeTypeDef stTime, stHalRTC_DateTypeDef stDate);
extern uint32_t GetUtcTimeofDate(stHalRTC_TimeTypeDef stTime,stHalRTC_DateTypeDef stDate);
extern unsigned long Oem_GetBattVoltage(char adcx);

extern eLAMP_STATE g_eLampCheckState;
extern eHORN_STATE g_eHornCheckState;
extern double g_dSavedGpsLat;
extern double g_dSavedGpsLon;
extern stDistanceInfo g_stDistanceInfo;
bool g_OdometerFirstTimeFlag = TRUE;
extern stControlConfig g_stControlConfig;
extern unsigned int g_uiTpmsAlramValue;
extern bool g_bEngRunContinueSuppFlag;
extern bool g_bEngRunKeepFlag;
extern unsigned char g_ucDTCTotalCnt;	//최초 IG ON 시 DTC가 있었으면 MIL LAMP가 ON상태여도 DTC알람 띄울필요없으므로 체크용
extern int	g_iTimerVSS1secCallback;
extern void SetEmergencyStatus(boolean_t bOccurred);
extern unsigned int g_ui1secTimer;
extern stDieselFuelConsume g_stDieselFuelConsume;
extern unsigned char g_ucGearPos;
extern bool g_bFuelLevelCheckFlag;
extern bool g_bFuelLiterTypeFlag;
extern unsigned char g_ucFuelLevelMaxLiter;
extern double g_dChecked_Fuel_Consume;
extern bool g_bFuelLevelSwitchedPercentFlag;

#pragma section="BKSRAM"
stElectricCarData g_stElectricCarData @"BKSRAM";

#pragma section="BKSRAM"
stLockState g_stLockState @"BKSRAM";
#pragma section="BKSRAM"
unsigned char g_ucButtonStatus @"BKSRAM";
#pragma section="BKSRAM"
unsigned char g_ucFuelLevel @"BKSRAM";
#pragma section="BKSRAM"
float g_fFuelLevel @"BKSRAM";
#pragma section="BKSRAM"
unsigned long long g_ullIndicator @"BKSRAM";
#pragma section="BKSRAM"
unsigned char g_ucTCUSlopeAngleArrIndex @"BKSRAM";
#pragma section="BKSRAM"
int g_nTCUSlopeAngleArr[TCUSLOPE_MAX_COUNT][2] @"BKSRAM";
#pragma section="BKSRAM"
unsigned int g_uiSetDoorSignal @"BKSRAM";

float g_fFuelLevelPercent=0;
unsigned char g_ucFuelListCount = 0;
unsigned char g_ucFuelListPosition = 0;
unsigned char g_ucFuelList[FUEL_LIST_MAX_CNT];
int	g_iTimerIndicator3secCallback = -1;
int	g_iTimerBleNoDisconnectCallback = -1;
#if defined(QA_FIFA)
int g_iTimerRPMCheckCallback = -1;
int g_iTimerENGRUNCheckCallback = -1;
#endif //#if defined(QA_FIFA)
bool g_bIndicator3secCallbackRunningFlag = false;
bool g_bTPMSConvCheckFlag = true;
//unsigned int g_uiTemp=0,g_uiTemp2=0,g_uiTemp3=0,g_uiTemp4=0,g_uiTemp5=0;

void Making_Drivingkey(void)
{
    uint8_t arrTmp[64]={0,};
	uint32_t unUTCTime;
	uint32_t unLocalTime;
	stHalRTCTypeDef stdate;

	unUTCTime = GetUTCTime();
	unLocalTime = GetLocalTimefromTime(unUTCTime);
	GetDatefromTime2(&stdate, unLocalTime);

	//RTC 수정
    g_OBDControllerData.basicData.DriveStartTime_Local = stdate.RtcTime;
	g_OBDControllerData.basicData.DriveStartDate_Local = stdate.RtcDate;
	sprintf((char *)arrTmp, "%04d%02d%02d%0.2d%0.2d%0.2d\x00",
				2000 + g_OBDControllerData.basicData.DriveStartDate_Local.RTC_Year,
				g_OBDControllerData.basicData.DriveStartDate_Local.RTC_Month,
				g_OBDControllerData.basicData.DriveStartDate_Local.RTC_Date,
				g_OBDControllerData.basicData.DriveStartTime_Local.RTC_Hours,
				g_OBDControllerData.basicData.DriveStartTime_Local.RTC_Minutes,
				g_OBDControllerData.basicData.DriveStartTime_Local.RTC_Seconds);

	// 운행 키를 저장해 둔다.
//	GetCurTimefromDate(g_OBDControllerData.basicData.DriveStartTime_Local,g_OBDControllerData.basicData.DriveStartDate_Local);
	printf("MSG: Generate Driving Key: [%s]\n", arrTmp);
	g_OBDControllerData.basicData.drivingKey = (long long)atoll((char *)arrTmp);
	return;
}

void Save_drivingTime(void)
{
	g_OBDControllerData.basicData.drivingTime = Get_DriveStopTime_UTC() - Get_DriveStartTime_UTC();
}

#define DECIDE_ENGINE_STOP_CHECK_TIME  600

void Decide_EngingButton_State(void)
{
#if !defined(QA_FIFA)
	static unsigned int s_uiEngineOffCheckTime = 0;
#endif
	if( (FUELTYPE_GET_STATE() == FCEV) || (FUELTYPE_GET_STATE() == ELECTRONIC) || (FUELTYPE_GET_STATE() == GASOLINE_HEV) || (FUELTYPE_GET_STATE() == PLUGIN_HEV) || (FUELTYPE_GET_STATE() == EV_NONE_READY) )
	{
		if( Get_EngRun_Status() == eENGRUN_OFF )
		{
			Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
			if( Get_Decide_Starting_Engine() == true )		EngineStopFunction(g_stControlConfig.m_ucEngStopType);
		}
		else	EngineStartFunction();
	}
	else
	{
		if( g_bIndicatorDBFlag == true)
		{
			if(GetPACVType() == CV)
			{
				if( FUELTYPE_GET_STATE() == ELECTRONIC )
				{
					if( Get_EngRun_Status() == eENGRUN_OFF )
					{
						Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
						if( Get_Decide_Starting_Engine() == true )		EngineStopFunction(g_stControlConfig.m_ucEngStopType);
					}
					else	EngineStartFunction();
				}
				else
				{
					if( Get_RPM() > DECIDE_ENGINE_STARTING_RPM) EngineStartFunction();
					else if(g_OBDControllerData.basicData.decide_Starting_Engine == true)  
					{
						Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
						EngineStopFunction(g_stControlConfig.m_ucEngStopType);
					}
				}
			}
			else
			{
#if defined(QA_FIFA)
				if( Get_EngRun_Status() == eENGRUN_OFF )
				{
					Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
					if( Get_Decide_Starting_Engine() == true )		EngineStopFunction(g_stControlConfig.m_ucEngStopType);
				}
				else	EngineStartFunction();
#else
				if( Get_RPM() > DECIDE_ENGINE_STARTING_RPM && ( Get_IG2_Status() == 1) )		EngineStartFunction();
				else if( (Get_IG2_Status() == 0) )
				{
					if( g_OBDControllerData.monitering_Data.m_bIG1 == true )			Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_IGON);
					else if( g_OBDControllerData.monitering_Data.m_ucACC == true )		Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_ACC);
					else																Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
					
					if( g_OBDControllerData.basicData.decide_Starting_Engine == TRUE )	EngineStopFunction(g_stControlConfig.m_ucEngStopType);
				}
				else if( ( (Get_IG1_Status() == 1) ) && (Get_EngRun_Status() == eENGRUN_OFF) )
				{
					if((s_uiEngineOffCheckTime == 0))
					{
					 	if(Get_ISG_Status() == 0 )
					 	{
						s_uiEngineOffCheckTime = Get_Tmr();
					 	}
					}
					else
					{
						if ( (Get_TmrDelta(Get_Tmr(), s_uiEngineOffCheckTime) > DECIDE_ENGINE_STOP_CHECK_TIME) )
						{
							s_uiEngineOffCheckTime = 0;
							Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_IGON);
							if( g_OBDControllerData.basicData.decide_Starting_Engine == TRUE )	EngineStopFunction(g_stControlConfig.m_ucEngStopType);				
						}
						else{}
					}
				}
				else if(  ( (Get_IG2_Status() == 0)) || (Get_EngRun_Status() == eENGRUN_OFF) || (Get_EngStall_Status() == 1) )
				{
					if( g_OBDControllerData.monitering_Data.m_bIG1 == true || g_OBDControllerData.monitering_Data.m_bIG2 == true )			Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_IGON);
					else if( g_OBDControllerData.monitering_Data.m_ucACC == true )		Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_ACC);
					else																								Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
					
					if( g_OBDControllerData.basicData.decide_Starting_Engine == TRUE )	EngineStopFunction(g_stControlConfig.m_ucEngStopType);
				}
				else if((Get_EngRun_Status() == eENGRUN_ON) && (Get_IG1_Status() == 1) && (Get_IG2_Status() == 1)) 
				{
					s_uiEngineOffCheckTime = 0;
				}
#endif
			}
		}
		else
		{
			if( g_stControlConfig.m_ucEngStopType == 0 )
			{	//이체크가 RPM체크보다 먼저 이루어 져야함 시동 ON OFF시 RPM이 0으로 떨어지기 전에 CAN데이터가 끊기는데 ACC IG IG2는 다 0으로 떨어지는거 확인가능 함
		//		if( (g_OBDControllerData.monitering_Data.m_bIG1==0) && (g_OBDControllerData.monitering_Data.m_bIG2==0) && (g_OBDControllerData.monitering_Data.m_ucACC==0)
		//		   ||((Get_RPM()==0) && (g_OBDControllerData.monitering_Data.m_bISGRun==eISGRUN_OFF)) )
				if( (g_OBDControllerData.monitering_Data.m_bIG1==0) && (g_OBDControllerData.monitering_Data.m_bIG2==0) && (g_OBDControllerData.monitering_Data.m_ucACC==0)
				   ||((Get_RPM()==0) && (g_OBDControllerData.monitering_Data.m_bENGRun==eENGRUN_OFF)) )
				{
		//			Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
					if( g_OBDControllerData.monitering_Data.m_bIG1 == true || g_OBDControllerData.monitering_Data.m_bIG2 == true )			Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_IGON);
					else if( g_OBDControllerData.monitering_Data.m_ucACC == true )		Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_ACC);
					else																								Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
					
					if( g_OBDControllerData.basicData.decide_Starting_Engine == TRUE )	EngineStopFunction(g_stControlConfig.m_ucEngStopType);
				}
				else if( Get_RPM() > DECIDE_ENGINE_STARTING_RPM )		EngineStartFunction();
			}
			else if( g_stControlConfig.m_ucEngStopType == 1 )
			{	//DH PE(G80)은 모듈에서 시동걸었을때 IG1,IG2,ACC상태가 변하지 않음
		//		if( Get_RPM()==0 )
				if( (Get_RPM()==0) && (g_OBDControllerData.monitering_Data.m_bENGRun==eENGRUN_OFF) )
				{
					if( g_OBDControllerData.monitering_Data.m_bIG1 == true || g_OBDControllerData.monitering_Data.m_bIG2 == true )			Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_IGON);
					else if( g_OBDControllerData.monitering_Data.m_ucACC == true )		Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_ACC);
					else																								Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
					
					if( g_OBDControllerData.basicData.decide_Starting_Engine == TRUE )	EngineStopFunction(g_stControlConfig.m_ucEngStopType);
				}
				else if( Get_RPM() > DECIDE_ENGINE_STARTING_RPM )		EngineStartFunction();
			}
			else if( g_stControlConfig.m_ucEngStopType == 2 )
			{	//DH PE(G80)은 모듈에서 시동걸었을때 IG1,IG2,ACC상태가 변하지 않음
		//		if( Get_RPM()==0 )
				if( (Get_RPM()==0) && (g_OBDControllerData.monitering_Data.m_bENGRun==eENGRUN_OFF) )
				{
					if( g_OBDControllerData.monitering_Data.m_bIG1 == true || g_OBDControllerData.monitering_Data.m_bIG2 == true )			Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_IGON);
					else if( g_OBDControllerData.monitering_Data.m_ucACC == true )		Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_ACC);
					else																								Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
					
					if( g_OBDControllerData.basicData.decide_Starting_Engine == TRUE )	EngineStopFunction(g_stControlConfig.m_ucEngStopType);
				}
				else if( g_OBDControllerData.monitering_Data.m_bENGRun > eENGRUN_OFF )	EngineStartFunction();
			}
			else if( g_stControlConfig.m_ucEngStopType == 3 )
			{
				if( Get_RPM() > DECIDE_ENGINE_STARTING_RPM && ( Get_IG2_Status() == 1) )		EngineStartFunction();
				else if( (Get_IG2_Status() == 0) )
				{
					if( g_OBDControllerData.monitering_Data.m_bIG1 == true )					Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_IGON);
					else if( g_OBDControllerData.monitering_Data.m_ucACC == true )		Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_ACC);
					else																								Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
					if( g_OBDControllerData.basicData.decide_Starting_Engine == TRUE )	EngineStopFunction(g_stControlConfig.m_ucEngStopType);
				}
				else if( ( (Get_IG1_Status() == 1) ) && Get_EngRun_Status() == eENGRUN_OFF )
				{
					Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_IGON);
					if( g_OBDControllerData.basicData.decide_Starting_Engine == TRUE )	EngineStopFunction(g_stControlConfig.m_ucEngStopType);
				}
				else	if(  ( (Get_IG2_Status() == 0)) || (Get_EngRun_Status() == eENGRUN_OFF) || (Get_EngStall_Status() == 1) )
				{
					if( g_OBDControllerData.monitering_Data.m_bIG1 == true || g_OBDControllerData.monitering_Data.m_bIG2 == true )			Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_IGON);
					else if( g_OBDControllerData.monitering_Data.m_ucACC == true )		Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_ACC);
					else																								Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_OFF);
					
					if( g_OBDControllerData.basicData.decide_Starting_Engine == TRUE )	EngineStopFunction(g_stControlConfig.m_ucEngStopType);
				}
			}
			else
			{}//	if( (Get_RPM()==0) && (g_OBDControllerData.monitering_Data.m_bISGRun==eISGRUN_OFF) )	//코나는 RPM 0 떨어지기전에 데이터멈춤;;
		}
	}
}

void EngineStartFunction()
{
#ifndef BNCOM
    if ( BTGetState() < eBT_Ready )
    {
        printf("BLE is not ready~~~~~~\r\n");
        return;
    }
#endif
    
	g_OBDControllerData.basicData.decide_Starting_Engine = TRUE;
	
	if( FUELTYPE_GET_STATE() == ELECTRONIC || FUELTYPE_GET_STATE() == EV_NONE_READY )	Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_EV_ON);
	else																			Send_Vehicle_Status_From_CAN(eVEHICLE_STATE_ENGRUN);
	if(g_OBDControllerData.basicData.drivingKey == 0)
	{
		InitializeOBDTripData();
		InitializeOBDPeriodData();
		g_bWarmupFlag = TRUE;

		if( g_bFuelLiterTypeFlag == true )
		{
			g_bFuelLevelCheckFlag = true;
		}

		if( FUELTYPE_GET_STATE() == EV_NONE_READY )
		{
		  	if( g_bEngRunKeepFlag == true )	//엔진 유지코드 멈췄어요 이벤트
			{
				Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStop, (stCarReport *)NULL,0);
				g_bEngRunKeepFlag = false;
				g_bFATCRunActFlag = false;
			}
		}

		g_iTimerVSS1secCallback=HalTimerSetSWTimer(1000, eSWTimer_INFINITE, VSS_1SecCallback, TRUE);
#if defined(QA_FIFA)
		if((GetPACVType() == CV) && (FUELTYPE_GET_STATE() == DIESEL)) 
 		{
 			g_iTimerRPMCheckCallback = HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, RPM_1SecCheckCallback, TRUE);
		}

		if((GetPACVType() == CV) && (FUELTYPE_GET_STATE() == ELECTRONIC))
		{
			g_iTimerENGRUNCheckCallback = HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, ENGRUN_1SecCheckCallback, TRUE);
		
		}
#endif
		Making_Drivingkey();
		Set_DriveStartTime();

		Send_GpsStatOnValidation(Get_GPS_Vailication());
		Send_GpsStartOnLat(Get_GPS_Lat());
		Send_GpsStartOnLon(Get_GPS_Lon());
		printf("~~~~~~~~~~starting engine~~~~~~~~~\r\n");

		ReportBeforeDriving();
		g_uiEngRunStartTime = Get_Tmr();
		if( g_iTimerBleNoDisconnectCallback != -1)
		{
			printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~g_iTimerBleNoDisconnectCallback~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
			HalTimerClearSWTimer(g_iTimerBleNoDisconnectCallback);
			g_iTimerBleNoDisconnectCallback = -1;
		}
	}
}

#if defined(FEATURE_EXTENSION_BOARD) 
void RequestExtendFOBKeyControl(void)
{
	boolean_t bAntithiefFlag;
	
	stMsgObd stTempMsg;
	memset((char*)&stTempMsg,0,sizeof(stTempMsg));
	
	//GetAutolinkConfigProperty(eAutoLinkConfig_AntiThiefFlag,(void*)&bAntithiefFlag);
	bAntithiefFlag = GetAntiThiefFlagSetting();
	
	stTempMsg.carReport.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_SETTING_ANTI_THIEF;

	if(bAntithiefFlag == true)
		stTempMsg.carReport.rpSmartKey.Request.ControlType = 0; //Fob OFF
	else
	   stTempMsg.carReport.rpSmartKey.Request.ControlType = 1; //Fob On
	
    Send2SysExtend(eMngObd, eExtend_ReqControl, eExtBD_FobKeyControlReq, 
                   false, (stMsgExtend*)&stTempMsg, eMngExtend);
}
#endif

void EngineStopFunction(unsigned char ucInput)
{
	DCSServiceType nServiceType;
    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);
	
	if( g_bEngRunKeepFlag == true )	//엔진 유지코드 멈췄어요 이벤트
	{
		Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStop, (stCarReport *)NULL,0);
		g_bEngRunKeepFlag = false;
	}
	g_bEngRunActFlag = false;
	g_bFATCRunActFlag = false;
	printf("~~~~~~~~~~~~~~engine stop~~~~~~~~~~~~ %d\r\n",ucInput);
	g_OBDControllerData.basicData.decide_Starting_Engine = FALSE;
	g_bEngineIdleFlag = FALSE;
	g_bFCSRunFlag=true;	//주행완료시 세팅해서 다음주행시작때 다시 체크하도록 함
	g_bIndicatorFirstCheckFlag=true;
	g_bFreezeFrameRunFlag=true;	//주행완료시 세팅해서 다음주행시작때 다시 체크하도록 함
	g_bMILLampAlramFlag=true;

	Set_DriveStopTime();
	Save_drivingTime();
	Send_EngStopGpsValidation(Get_GPS_Vailication());
	Send_EngStopGpsLat(Get_GPS_Lat()) ;
	Send_EngStopGpsLon(Get_GPS_Lon());
	printf("~~~~~~~~~~~~~eACC_OFF ~~~~~~~~~~~~, fuel Level Percent %f\r\n",g_fFuelLevelPercent);

	// engine off alram : key : 0
	//MONI 20180430
	//we must send this message to system
	ReportAlramStatus(eMESSAGE_EVENT_KEY_VEHICLE_ENGINE_START_ALARM, 0);

#if defined(FEATURE_EXTENSION_BOARD) 
    RequestExtendFOBKeyControl();
#endif

	if( g_iTimerIndicator3secCallback != -1 )
	{
		CB_Indicator_3secCallback();
	}
	if( (g_iTimerBleNoDisconnectCallback == -1) && (BTGetConnectStatus()==true) )
	{
		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~g_iTimerBleNoDisconnectCallback start~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
		g_iTimerBleNoDisconnectCallback = HalTimerSetSWTimer(BLEDISCONNECT_TIMEOUT, eSWTimer_ONESHOT, CB_BLE_Disconnect_40minCallback, TRUE);
	}

#if defined(QA_FIFA)
	if(g_iTimerRPMCheckCallback != -1) 	 
	{
		HalTimerClearSWTimer(g_iTimerRPMCheckCallback);
		g_iTimerRPMCheckCallback=-1;		
	}

	if(g_iTimerENGRUNCheckCallback != -1) 
	{
		HalTimerClearSWTimer(g_iTimerENGRUNCheckCallback);
		g_iTimerENGRUNCheckCallback = -1;
    }
#endif

	UpdateOBDPeriodData();
	ReportAfterDriving();

#if defined(PROTOCOL18)
	//시동 종료 후 예약시동 정보 얻어오기 위한 처리
	ReportRsvEngCtrlStatus();
#endif

	Clear_Drivingkey();
	HalTimerClearSWTimer(g_iTimerVSS1secCallback);
	g_iTimerVSS1secCallback=-1;

	g_bCrashAlramFlag = true;
}

void ReportRsvEngCtrlStatus()
{
	stCarReport stMsg;

	memset(&stMsg,0x00,sizeof(stMsg));
	GetRsvEngCtrlSetting((stRsvEngCntorl*)&stMsg.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl);
	//printf("ReportRsvEngCtrlStatus\r\n");
	//hexdump(&stMsg,sizeof(stMsg));
	Send2MngSysMsg2(eMngObd, eRspReport, eR_RspSettingRsvEngCtrl,true, &stMsg, eMngObd);
}


/* For reporting event alarm */
void Decide_Battery_High_Voltage_Alarm(void)
{
	static int high_Battery_Alarm_Counter = 0;
	unsigned short battery = 0;

	battery = (unsigned short)g_OBDControllerData.monitering_Data.m_fBattery * 10;


	if(battery >= DECIDE_BATTERY_HIGH_VOLTAGE_ON)
	{
		if(high_Battery_Alarm_Counter < 0)
			high_Battery_Alarm_Counter = 1;
		else if(high_Battery_Alarm_Counter < 1000)		/* block overflow  */
			high_Battery_Alarm_Counter++;
	}
	else if (battery < DECIDE_BATTERY_HIGH_VOLTAGE_ON)
	{
		if(high_Battery_Alarm_Counter > 0)
			high_Battery_Alarm_Counter = -1;
		else if(high_Battery_Alarm_Counter > -1000)		/* block overflow  */
			high_Battery_Alarm_Counter--;
	}

	if(high_Battery_Alarm_Counter == 3)  /* report only once. */
	{
		printf("~~~~~~~~~~~~~~~BATT_ALARM : %d \r\n", battery);
		ReportAlramStatus(eMESSAGE_EVENT_KEY_OVER_VOLTAGE_ALARM, battery);
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_BATTERY_HIGH_VOLTAGE_ON ALARM : %d \r\n", battery);
#endif
	}
	else if(high_Battery_Alarm_Counter == -3)
	{
//		SetVehicleEventKeyValueAndSendFlag(eMESSAGE_EVENT_KEY_OVER_VOLTAGE_ALARM, 0, true);
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_BATTERY_HIGH_VOLTAGE_OFF ALARM : %d \r\n", battery);
#endif
	}
	/* else not changed */
}

void Decide_Battery_Low_Voltage_Alarm(void)
{
	static int low_Battery_Alarm_Counter = 0;
	unsigned short battery = 0;


	battery = (unsigned short)g_OBDControllerData.monitering_Data.m_fBattery * 10;


	if(battery <= DECIDE_BATTERY_LOW_VOLTAGE_ON)
	{
		if(low_Battery_Alarm_Counter < 0)
			low_Battery_Alarm_Counter = 1;
		else if(low_Battery_Alarm_Counter < 1000)		/* block overflow  */
			low_Battery_Alarm_Counter++;
	}
	else if (battery > DECIDE_BATTERY_LOW_VOLTAGE_ON)
	{
		if(low_Battery_Alarm_Counter > 0)
			low_Battery_Alarm_Counter = -1;
		else if(low_Battery_Alarm_Counter > -1000)		/* block overflow  */
			low_Battery_Alarm_Counter--;
	}

	if(low_Battery_Alarm_Counter == 3)  /* report only once. */
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_LOW_VOLTAGE_ALARM, battery);
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_BATTERY_LOW_VOLTAGE_ON ALARM  : %d \r\n", battery);
#endif
	}
	else if(low_Battery_Alarm_Counter == -3)
	{
//		SetVehicleEventKeyValueAndSendFlag(eMESSAGE_EVENT_KEY_LOW_VOLTAGE_ALARM, 0, true);
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_BATTERY_LOW_VOLTAGE_OFF ALARM  : %d \r\n", battery);
#endif
	}
	/* else not changed */
}

void Decide_Door_Lock_Alarm(void)
{
	static unsigned char s_ucDoorLock=0;
	stHalRTCTypeDef stDate;
	
	if( s_ucDoorLock != g_OBDControllerData.currentState.doorLock )
	{
		s_ucDoorLock = g_OBDControllerData.currentState.doorLock;
		if( (g_OBDControllerData.currentState.doorOpen == 0) && (g_OBDControllerData.currentState.doorLock == 0) )
		{
			g_stLockState.m_uiLastLockTime = GetUTCTime();
			GetDatefromTime2( &stDate, g_stLockState.m_uiLastLockTime );
			printf(" Lock Save time : %04d/%02d/%02d,%02d:%02d:%02d\n",stDate.RtcDate.RTC_Year+2000,
                                                        stDate.RtcDate.RTC_Month,
                                                        stDate.RtcDate.RTC_Date,
                                                        stDate.RtcTime.RTC_Hours,
                                                        stDate.RtcTime.RTC_Minutes,
                                                        stDate.RtcTime.RTC_Seconds);
		}
	}
	
	if( g_stDoorUnlockAlramCheckState.m_bAlram == true &&
	   g_stDoorUnlockAlramCheckState.m_bFL == true && g_stDoorUnlockAlramCheckState.m_bFR == true &&
	   g_stDoorUnlockAlramCheckState.m_bRL == true && g_stDoorUnlockAlramCheckState.m_bRR == true )
	{
		g_stDoorUnlockAlramCheckState.m_bAlram	= false;
		g_stDoorUnlockAlramCheckState.m_bFL		= false;
		g_stDoorUnlockAlramCheckState.m_bFR		= false;
		g_stDoorUnlockAlramCheckState.m_bRL		= false;
		g_stDoorUnlockAlramCheckState.m_bRR		= false;
		printf("\r\n~Lock : %d\r\n",g_OBDControllerData.currentState.doorLock);
		ReportAlramStatus(eMESSAGE_EVENT_KEY_DOOR_LOCK_ALARM , g_OBDControllerData.currentState.doorLock);
	}
}

void Decide_Door_Open_Alarm(void)
{
	static unsigned char s_ucDoorOpen=0;
	stHalRTCTypeDef stDate;
	
	if( s_ucDoorOpen != g_OBDControllerData.currentState.doorOpen )
	{
		//이전상태가 트렁크가 열려있었다면은 시간 체크, 나머지 체크안하는 이유는 모든문에서 시간갱신하면 문열렸다고올라가는 실패응답이 45초대기 때문에 늦게 올라가기 때문
		if( (g_OBDControllerData.currentState.doorOpen == 0) && (g_OBDControllerData.currentState.doorLock == 0) && ((s_ucDoorOpen&0x30) != 0x00))
		{
			g_stLockState.m_uiLastLockTime = GetUTCTime();
			GetDatefromTime2( &stDate, g_stLockState.m_uiLastLockTime );
			printf(" Lock Save time_open : %04d/%02d/%02d,%02d:%02d:%02d\n",stDate.RtcDate.RTC_Year+2000,
                                                        stDate.RtcDate.RTC_Month,
                                                        stDate.RtcDate.RTC_Date,
                                                        stDate.RtcTime.RTC_Hours,
                                                        stDate.RtcTime.RTC_Minutes,
                                                        stDate.RtcTime.RTC_Seconds);
		}
		s_ucDoorOpen = g_OBDControllerData.currentState.doorOpen;
	}
	
	if( g_stDoorOpenAlramCheckState.m_bAlram == true &&
	   g_stDoorOpenAlramCheckState.m_bFL == true && g_stDoorOpenAlramCheckState.m_bFR == true &&
	   g_stDoorOpenAlramCheckState.m_bRL == true && g_stDoorOpenAlramCheckState.m_bRR == true && 
	   g_stDoorOpenAlramCheckState.m_bTR == true && g_stDoorOpenAlramCheckState.m_bHood == true)
	{
		g_stDoorOpenAlramCheckState.m_bAlram	= false;
		g_stDoorOpenAlramCheckState.m_bFL		= false;
		g_stDoorOpenAlramCheckState.m_bFR		= false;
		g_stDoorOpenAlramCheckState.m_bRL		= false;
		g_stDoorOpenAlramCheckState.m_bRR		= false;
		g_stDoorOpenAlramCheckState.m_bTR		= false;
		g_stDoorOpenAlramCheckState.m_bHood	= false;
		printf("~OPEN : %d\r\n",g_OBDControllerData.currentState.doorOpen);
		ReportAlramStatus(eMESSAGE_EVENT_KEY_DOOR_OPEN_ALARM , g_OBDControllerData.currentState.doorOpen);

		if(FUELTYPE_GET_STATE() == EV_NONE_READY )
        {
        	//printf("~~~~ ACTUATOR_TYPE_AIRCON_STOP ~~~~~\r\n");
			
        	if(IsNeedAirConOff())
        	{
				Set_Actuator(ACTUATOR_TYPE_AIRCON_STOP);
				SetClearAirControl();
        	}
		}		
	}
}



void Decide_Coolant_Temperature_Alarm(void)
{
	static int engine_Temperature_Alarm_Counter = 0;
	unsigned short engineTemperature = 0;

	engineTemperature = g_OBDControllerData.monitering_Data.m_sCoolantTemperature;

	if(engineTemperature >= DECIDE_ENGINE_OVER_TEMPERATURE_ON)
	{
		if(engine_Temperature_Alarm_Counter < 0)
			engine_Temperature_Alarm_Counter = 1;
		else if(engine_Temperature_Alarm_Counter < 1000)		/* block overflow  */
			engine_Temperature_Alarm_Counter++;
	}
	else if (engineTemperature < DECIDE_ENGINE_OVER_TEMPERATURE_ON)
	{
		if(engine_Temperature_Alarm_Counter > 0)
			engine_Temperature_Alarm_Counter = -1;
		else if(engine_Temperature_Alarm_Counter > -1000)		/* block overflow  */
			engine_Temperature_Alarm_Counter--;
	}

	if(engine_Temperature_Alarm_Counter == 3)	/* report only once. */
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_OVER_ENGINE_TEMP_ALARM, g_OBDControllerData.monitering_Data.m_sCoolantTemperature);
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_ENGINE_OVER_TEMPERATURE_ON ALARM : %d \r\n", engineTemperature);
#endif
	}
	else if(engine_Temperature_Alarm_Counter == -3)
	{
//		SetVehicleEventKeyValueAndSendFlag(eMESSAGE_EVENT_KEY_OVER_ENGINE_TEMP_ALARM, 0, true);
#if defined(DEBUG_ALARM_ALL)
                printf("DECIDE_ENGINE_OVER_TEMPERATURE_OFF ALARM : %d \r\n", engineTemperature);
#endif
	}
	/* else not changed */
}

void Decide_Fuel_Run_Out_Alarm(void)
{
	static int fuel_Run_Out_Alarm_Counter = 0;
	unsigned char remain_Fuel = 0;

	if( g_bFuelLiterTypeFlag == true && g_bFuelLevelSwitchedPercentFlag == false )
	{
		
	}
	else
	{
		remain_Fuel = g_OBDControllerData.monitering_Data.m_ucFuel_Level;

		if(remain_Fuel <= DECIDE_FUEL_RUN_OUT_ON) {
			if(fuel_Run_Out_Alarm_Counter < 0) {
				fuel_Run_Out_Alarm_Counter = 1;
			}
			else if(fuel_Run_Out_Alarm_Counter < 1000) {		/* block overflow  */
				fuel_Run_Out_Alarm_Counter++;
			}
		}
		else if (remain_Fuel > DECIDE_FUEL_RUN_OUT_ON) {
			if(fuel_Run_Out_Alarm_Counter > 0) {
				fuel_Run_Out_Alarm_Counter = -1;
			}
			else if(fuel_Run_Out_Alarm_Counter > -1000) {		/* block overflow  */
				fuel_Run_Out_Alarm_Counter--;
			}
		}

		if(fuel_Run_Out_Alarm_Counter == 3)	{/* report only once. */
			ReportAlramStatus(eMESSAGE_EVENT_KEY_FUEL_RUN_OUT, g_OBDControllerData.monitering_Data.m_ucFuel_Level);
	#if defined(DEBUG_ALARM_ALL)
			printf("DECIDE_FUEL_RUN_OUT_ON ALARM : %d \r\n", remain_Fuel);
	#endif

		}
		else if(fuel_Run_Out_Alarm_Counter == -3) {
	//		SetVehicleEventKeyValueAndSendFlag(eMESSAGE_EVENT_KEY_FUEL_RUN_OUT, 0, true);
	#if defined(DEBUG_ALARM_ALL)
			printf("DECIDE_FUEL_RUN_OUT_OFF ALARM : %d \r\n", remain_Fuel);
	#endif
		}
		/* else not changed */
	}
}

unsigned char Decide_Each_TPMS(float Each_TPMS)
{
	if( (Each_TPMS < g_uiTpmsAlramValue) && (Each_TPMS != 0) )	return 1;	//0이면 초기값이므로 정상
	else																											return 0; /* normal */
}

void Decide_BrakeJudder_Alarm(unsigned int uiInput)
{
	bool bRet=false;
	switch(g_stBrakeJudder.ucCheckType)
	{
		case eBelow:	//이하
			if( g_stBrakeJudder.uiValue >= uiInput ) bRet=true;
			break;
		case eExcess:	//초과
			if( g_stBrakeJudder.uiValue < uiInput ) bRet=true;
			break;
		case eUnder:	//미만
			if( g_stBrakeJudder.uiValue > uiInput ) bRet=true;
			break;
		case eEqual:	//같음
			if( g_stBrakeJudder.uiValue == uiInput ) bRet=true;
			break;
		default:		//이상
			if( g_stBrakeJudder.uiValue <= uiInput ) bRet=true;
			break;
	}
	if( bRet == true )	ReportAlramStatus(eMESSAGE_EVENT_KEY_BRAKEJUDDERINDICATOR_ALRAM,uiInput);
}

void Decide_TPMS_Alarm()
{
	char cData[20];

	memset(&cData,0x00,sizeof(cData));
    
	// 타이어 공기압: 0 									--> 000000000000: 타이어 공기압
	// 전좌/전우/후좌/후우(12자리), 소수점을 없애기 위해서 10을 곱해준다.
	// 23.5/23.5/23.5/23.5 일 경우, 235/235/235/235 --> 2byte/2byte/2byte/2byte --> 을 최종 적으로 사용
	if(Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN)
	{
		sprintf(cData, "%04x,%04x,%04x,%04x", (unsigned short int)g_OBDControllerData.currentState.TPMS_FL * 10,
																(unsigned short int)g_OBDControllerData.currentState.TPMS_FR * 10,
																(unsigned short int)g_OBDControllerData.currentState.TPMS_RL * 10,
																(unsigned short int)g_OBDControllerData.currentState.TPMS_RR * 10);
		printf("~TPMS : %s\r\n",cData);
		ReportAlramTPMS(eMESSAGE_EVENT_KEY_TIRE_PRESSURE , cData);
	}
}

void Decide_OverSpeed_Alarm(void)
{
	static int overSpeed_Counter = 0;
	unsigned short currentSpeed = 0;

	currentSpeed = g_OBDControllerData.monitering_Data.m_usCurrentSpeed;

	if(currentSpeed >= DECIDE_OVER_SPEED_ON)
	{
		if(overSpeed_Counter < 0)
			overSpeed_Counter = 1;
		else if(overSpeed_Counter < 1000) 	/* block overflow  */
			overSpeed_Counter++;
	}
	else if (currentSpeed < DECIDE_OVER_SPEED_ON)
	{
		if(overSpeed_Counter > 0)
			overSpeed_Counter = -1;
		else if(overSpeed_Counter > -1000)		/* block overflow  */
			overSpeed_Counter--;
	}

	if(overSpeed_Counter == 3)	/* report only once. */
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_VEHICLE_OVER_SPEED_ALARM, g_OBDControllerData.monitering_Data.m_usCurrentSpeed);
		printf("DECIDE_OVER_SPEED_ON ALARM  : %d \r\n", currentSpeed);
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_OVER_SPEED_ON ALARM  : %d \r\n", currentSpeed);
#endif
        }
	else if(overSpeed_Counter == -3)
	{
//		SetVehicleEventKeyValueAndSendFlag(eMESSAGE_EVENT_KEY_VEHICLE_OVER_SPEED_ALARM, 0, true);
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_OVER_SPEED_OFF ALARM  : %d \r\n", currentSpeed);
#endif
	}
	/* else not changed */
}

void Decide_OverRPM_Alarm(void)
{
	static int overRPM_Counter = 0;
	unsigned short RPM = 0;

	RPM = Get_RPM();

	if(RPM >= DECIDE_OVER_RPM_ON)
	{
		if(overRPM_Counter < 0)
			overRPM_Counter = 1;
		else
			if(overRPM_Counter < 1000) 	/* block overflow  */
				overRPM_Counter++;
	}
	else if (RPM < DECIDE_OVER_RPM_ON)
	{
		if(overRPM_Counter > 0)
			overRPM_Counter = -1;
		else if(overRPM_Counter > -1000)		/* block overflow  */
			overRPM_Counter--;
	}

	if(overRPM_Counter == 3)	/* report only once. */
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_VEHICLE_OVER_RPM_ALARM, Get_RPM());
		printf("DECIDE_OVER_RPM_ON ALARM  : %d \r\n", RPM);
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_OVER_RPM_ON ALARM  : %d \r\n", RPM);
#endif
	}
	else if(overRPM_Counter == -3)
	{
//		SetVehicleEventKeyValueAndSendFlag(eMESSAGE_EVENT_KEY_VEHICLE_OVER_RPM_ALARM, 0, true);
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_OVER_RPM_OFF ALARM  : %d \r\n", RPM);
#endif
	}
	/* else not changed */
}

void Decide_OverMotorRPM_Alarm(void)
{
	static int overMotorRPM_Counter = 0;
	unsigned short usMotorRPM = 0;

	usMotorRPM  = Get_MotorRPM_Status();

	if(usMotorRPM  >= DECIDE_OVER_MOTOR_RPM_ON)
	{
		if(overMotorRPM_Counter < 0)
			overMotorRPM_Counter = 1;
		else
			if(overMotorRPM_Counter < 1000) 	/* block overflow  */
				overMotorRPM_Counter++;
	}
	else if (usMotorRPM  < DECIDE_OVER_MOTOR_RPM_ON)
	{
		if(overMotorRPM_Counter > 0)
			overMotorRPM_Counter = -1;
		else if(overMotorRPM_Counter > -1000)		/* block overflow  */
			overMotorRPM_Counter--;
	}

	if(overMotorRPM_Counter == 3)	/* report only once. */
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_VEHICLE_OVER_RPM_ALARM, Get_MotorRPM_Status());
		printf("DECIDE_OVER_MOTOR_RPM_ON ALARM  : %d \r\n", usMotorRPM );
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_OVER_MOTOR_RPM_ON ALARM  : %d \r\n", usMotorRPM);
#endif
	}
	else if(overMotorRPM_Counter == -3)
	{
#if defined(DEBUG_ALARM_ALL)
		printf("DECIDE_OVER_RPM_MOTOR_OFF ALARM  : %d \r\n", usMotorRPM);
#endif
	}
}


void Decide_Tail_Lamp_Alarm(void)
{
	static int tail_Lamp_Alarm_Counter = 0;
	unsigned char tailLamp = 0;

	tailLamp = g_OBDControllerData.monitering_Data.m_bTailLamp;

	if(tailLamp == 0 )
	{
		if(tail_Lamp_Alarm_Counter < 0)
			tail_Lamp_Alarm_Counter = 1;
		else if(tail_Lamp_Alarm_Counter < 1000)		/* block overflow  */
			tail_Lamp_Alarm_Counter++;
	}
	else
	{
		if(tail_Lamp_Alarm_Counter > 0)
			tail_Lamp_Alarm_Counter = -1;
		else if(tail_Lamp_Alarm_Counter > -1000) 	/* block overflow  */
			tail_Lamp_Alarm_Counter--;
	}

	if(tail_Lamp_Alarm_Counter == 3)	/* report only once. */
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_TAIL_LAMP_ALARM , g_OBDControllerData.monitering_Data.m_bTailLamp);
		printf("Tail Lamp 0 ALARM  : %d \r\n", tailLamp);
#if defined(DEBUG_ALARM_ALL)
		printf("Tail Lamp 0 ALARM  : %d \r\n", tailLamp);
#endif
	}
	else if(tail_Lamp_Alarm_Counter == -3)
	{
//		SetVehicleEventKeyValueAndSendFlag(eMESSAGE_EVENT_KEY_TAIL_LAMP_ALARM , 0, true);
#if defined(DEBUG_ALARM_ALL)
		printf("Tail Lamp 1 ALARM  : %d \r\n", tailLamp);
#endif
	}
	/* else not changed */
}

void Decide_Crash_Alarm(void)
{
//	static int Crash_Alarm_Counter = 0;
//
//	if(g_OBDControllerData.monitering_Data.m_bCrashSignal == 1 )
//	{
//		if(Crash_Alarm_Counter < 0)
//			Crash_Alarm_Counter = 1;
//		else if(Crash_Alarm_Counter < 1000)		/* block overflow  */
//			Crash_Alarm_Counter++;
//	}
//	else
//	{
//		if(Crash_Alarm_Counter > 0)
//			Crash_Alarm_Counter = -1;
//		else
//			if(Crash_Alarm_Counter > -1000) 	/* block overflow  */
//				Crash_Alarm_Counter--;
//	}
//
//	if(Crash_Alarm_Counter == 1)	/* report only once. */
//	{
//		ReportAlramStatus(eMESSAGE_EVENT_KEY_AIRBAG_ALRAM , 1);
//#if defined(DEBUG_ALARM_ALL)
//		printf("Crash 1 ALARM  : %d \r\n" ,g_OBDControllerData.monitering_Data.m_bCrashSignal );
//#endif
//	}
//	else if(Crash_Alarm_Counter == -1)
//	{
////		SetVehicleEventKeyValueAndSendFlag(eMESSAGE_EVENT_KEY_PARKING_IMPACT , 0, true);
//#if defined(DEBUG_ALARM_ALL)
//		printf("Crash 0 ALARM : %d \r\n" ,g_OBDControllerData.monitering_Data.m_bCrashSignal );
//#endif
//	}
	/* else not changed */
}

void Decide_MIL_Lamp_Alarm(void)
{
	unsigned char MIL_Lamp = 0;

	if( (Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME) && (g_OBDControllerData.monitering_Data.m_bMILLamp==1) && (g_bMILLampAlramFlag==true) && (Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN) )
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_MIL_LAMP_ON , g_OBDControllerData.monitering_Data.m_bMILLamp);
		if( (g_ucDTCTotalCnt==0) && GetOBDState()==eOBD_Running_Info_Mode )	//부팅시 DTC가 없었으면 MIL알람시 DTC도 체크
		{
			SetOBDState(eOBD_FCS_Start);
			g_bFCSRunFlag = true;
			g_bFreezeFrameRunFlag = true;
		}
		printf("Mil Lamp 1 ALARM  %d \r\n" ,MIL_Lamp );
#if defined(DEBUG_ALARM_ALL)
		printf("Mil Lamp 1 ALARM  %d \r\n" ,MIL_Lamp );
#endif
		g_bMILLampAlramFlag = false;
	}
}

#if defined(FEATURE_EXTENSION_BOARD)
void Decide_Foot_Break_Alarm(void)
{
	static unsigned char s_ucFootBreak=0;
	
	if( s_ucFootBreak != g_OBDControllerData.monitering_Data.m_bFootBrake )
	{
		s_ucFootBreak = g_OBDControllerData.monitering_Data.m_bFootBrake;
		
        // 브레이크가 밟혀진다면&& 문이 닫힌 상태이면
        if ( (s_ucFootBreak == true) &&(g_OBDControllerData.currentState.doorOpen == 0) ) 
        {
            RequestExtendFOBKeyControl();
        }
	}
}
#endif

void Decide_Indicator_Alarm()
{
	//unsigned int uiIndcatorFromX1=0, uiIndcatorFromX2=0; 
    unsigned int uiNewAlramFlag=0;
	unsigned long long ullIndicatorFromX = 0;
		
	//어디서 온 데이터 인지 변경
//	if( (g_OBDControllerData.AlramData.m_uc4WD&0xFF) != 0 )						uiIndcatorFromX1 |= 0x00000001;	//4WD11
//	if( (g_OBDControllerData.AlramData.m_ucABS&0x01) == 0x01 	)				uiIndcatorFromX1 |= 0x00000002;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucABS&0x02) == 0x02 )				uiIndcatorFromX1 |= 0x00000004;	//ABS11
//	if( (g_OBDControllerData.AlramData.m_ucAirbag) != 0x00 )						uiIndcatorFromX1 |= 0x00000008;	//ACU11
////	if( (g_OBDControllerData.AlramData.m_ucAirbag&0x1C) != 0x00 )				uiIndcatorFromX1 |= 0x00000010;	//CGW_CP5
//	if( (g_OBDControllerData.AlramData.m_ucAutoHold) != 0x00 )					uiIndcatorFromX1 |= 0x00000020;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucBatteryCharge&0x01) == 0x01 )	uiIndcatorFromX1 |= 0x00000040;	//EMS19
//	if( (g_OBDControllerData.AlramData.m_ucBatteryCharge&0x02) == 0x02 )	uiIndcatorFromX1 |= 0x00000080;	//LDC1
//	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x01) == 0x01 )	uiIndcatorFromX1 |= 0x00000100;	//EMS4
//	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x02) == 0x02 )	uiIndcatorFromX1 |= 0x00000200;	//BMS38_1
//	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x04) == 0x04 )	uiIndcatorFromX1 |= 0x00000400;	//HEV_CP2
//	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x08) == 0x08 )	uiIndcatorFromX1 |= 0x00000800;	//HEV_CP9
//	if( (g_OBDControllerData.AlramData.m_ucDBCWarnning&0x01) == 0x01 )	uiIndcatorFromX1 |= 0x00001000;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucDPF) != 0x00 )							uiIndcatorFromX1 |= 0x00002000;	//EMS19
//	if( (g_OBDControllerData.AlramData.m_ucSCR&0x01) == 0x01 )				uiIndcatorFromX1 |= 0x00004000;	//EMS21
//	if( (g_OBDControllerData.AlramData.m_ucEPB) != 0x00 )							uiIndcatorFromX1 |= 0x00008000;	//EPB11
//	if( (g_OBDControllerData.AlramData.m_ucTCS) != 0x00 )						uiIndcatorFromX1 |= 0x00010000;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucISG) != 0x00 )							uiIndcatorFromX1 |= 0x00020000;	//EMS_H12
////	if( (g_OBDControllerData.AlramData.m_ucISG&0x04) == 0x04 )					uiIndcatorFromX1 |= 0x00040000;	//CGW_PCS
//	if( (g_OBDControllerData.AlramData.m_ucMDPS) != 0x00 )						uiIndcatorFromX1 |= 0x00080000;	//CGW_EP2
//	if( (g_OBDControllerData.AlramData.m_ucOilLevel&0x01) == 0x01 )			uiIndcatorFromX1 |= 0x00100000;	//EMS_H12
//	if( (g_OBDControllerData.AlramData.m_ucOilPress&0x01) == 0x01 )			uiIndcatorFromX1 |= 0x00200000;	//EMS19
//	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x01) == 0x01 )	uiIndcatorFromX1 |= 0x00400000;	//ABS11
//	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x02) == 0x02 )	uiIndcatorFromX1 |= 0x00800000;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x04) == 0x04 )	uiIndcatorFromX1 |= 0x01000000;	//EBP11
//	if( (g_OBDControllerData.AlramData.m_ucPowerDown&0x03) != 0x00 )		uiIndcatorFromX1 |= 0x02000000;	//VCU5
//	if( (g_OBDControllerData.AlramData.m_ucAHB&0x01) == 0x01 )				uiIndcatorFromX1 |= 0x04000000;	//EBS1
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x01) == 0x01 )				uiIndcatorFromX1 |= 0x08000000;	//HCU5
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x06) != 0x00 )				uiIndcatorFromX1 |= 0x10000000;	//MCU2
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x08) == 0x08 )				uiIndcatorFromX1 |= 0x20000000;	//BMS1
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x10) == 0x10 )				uiIndcatorFromX1 |= 0x40000000;	//LDC1
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x20) == 0x20 )				uiIndcatorFromX1 |= 0x80000000;	//TCU15
//	
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x40) == 0x40 )				uiIndcatorFromX2 |= 0x00000001;	//DATC1
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x80) == 0x80 )				uiIndcatorFromX2 |= 0x00000002;	//OBC1
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x01) == 0x01 )					uiIndcatorFromX2 |= 0x00000004;	//VCU5
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x02) == 0x02 )					uiIndcatorFromX2 |= 0x00000008;	//BMS1
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x04) == 0x04 )					uiIndcatorFromX2 |= 0x00000010;	//LDC1
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x08) == 0x08 )					uiIndcatorFromX2 |= 0x00000020;	//MCU2
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x10) == 0x10 )					uiIndcatorFromX2 |= 0x00000040;	//DATC3
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x20) == 0x20 )					uiIndcatorFromX2 |= 0x00000080;	//OBC1
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x01) == 0x01 )				uiIndcatorFromX2 |= 0x00000100;	//MCU2
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x02) == 0x02 )				uiIndcatorFromX2 |= 0x00000200;	//BMS1
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x04) == 0x04 )				uiIndcatorFromX2 |= 0x00000400;	//HDC2
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x08) == 0x08 )				uiIndcatorFromX2 |= 0x00000800;	//BPCU4
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x10) == 0x10 )				uiIndcatorFromX2 |= 0x00001000;	//LDC1
//	if( (g_OBDControllerData.AlramData.m_ucTPMS) != 0x00 )						uiIndcatorFromX2 |= 0x00002000;	//TPMS11
	
	if( (g_OBDControllerData.AlramData.m_uc4WD&0xFF) != 0 )						ullIndicatorFromX |= 0x0000000000000001;	//4WD11
	if( (g_OBDControllerData.AlramData.m_ucABS&0x01) == 0x01 	)				ullIndicatorFromX |= 0x0000000000000002;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucABS&0x02) == 0x02 )				ullIndicatorFromX |= 0x0000000000000004;	//ABS11
	if( (g_OBDControllerData.AlramData.m_ucAirbag) != 0x00 )						ullIndicatorFromX |= 0x0000000000000008;	//ACU11
//	if( (g_OBDControllerData.AlramData.m_ucAirbag&0x1C) != 0x00 )				ullIndicatorFromX |= 0x0000000000000010;	//CGW_CP5
	if( (g_OBDControllerData.AlramData.m_ucAutoHold) != 0x00 )					ullIndicatorFromX |= 0x0000000000000020;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucBatteryCharge&0x01) == 0x01 )	ullIndicatorFromX |= 0x0000000000000040;	//EMS19
	if( (g_OBDControllerData.AlramData.m_ucBatteryCharge&0x02) == 0x02 )	ullIndicatorFromX |= 0x0000000000000080;	//LDC1
	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x01) == 0x01 )	ullIndicatorFromX |= 0x0000000000000100;	//EMS4
	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x02) == 0x02 )	ullIndicatorFromX |= 0x0000000000000200;	//BMS38_1
	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x04) == 0x04 )	ullIndicatorFromX |= 0x0000000000000400;	//HEV_CP2
	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x08) == 0x08 )	ullIndicatorFromX |= 0x0000000000000800;	//HEV_CP9
	if( (g_OBDControllerData.AlramData.m_ucDBCWarnning&0x01) == 0x01 )	ullIndicatorFromX |= 0x0000000000001000;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucDPF) != 0x00 )							ullIndicatorFromX |= 0x0000000000002000;	//EMS19
	if( (g_OBDControllerData.AlramData.m_ucSCR&0x01) == 0x01 )				ullIndicatorFromX |= 0x0000000000004000;	//EMS21
	if( (g_OBDControllerData.AlramData.m_ucEPB) != 0x00 )							ullIndicatorFromX |= 0x0000000000008000;	//EPB11
	if( (g_OBDControllerData.AlramData.m_ucTCS) != 0x00 )						ullIndicatorFromX |= 0x0000000000010000;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucISG) != 0x00 )							ullIndicatorFromX |= 0x0000000000020000;	//EMS_H12
//	if( (g_OBDControllerData.AlramData.m_ucISG&0x04) == 0x04 )					ullIndicatorFromX |= 0x0000000000040000;	//CGW_PCS
	if( (g_OBDControllerData.AlramData.m_ucMDPS) != 0x00 )						ullIndicatorFromX |= 0x0000000000080000;	//CGW_EP2
	if( (g_OBDControllerData.AlramData.m_ucOilLevel&0x01) == 0x01 )			ullIndicatorFromX |= 0x0000000000100000;	//EMS_H12
	if( (g_OBDControllerData.AlramData.m_ucOilPress&0x01) == 0x01 )			ullIndicatorFromX |= 0x0000000000200000;	//EMS19
	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x01) == 0x01 )	ullIndicatorFromX |= 0x0000000000400000;	//ABS11
	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x02) == 0x02 )	ullIndicatorFromX |= 0x0000000000800000;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x04) == 0x04 )	ullIndicatorFromX |= 0x0000000001000000;	//EBP11
	if( (g_OBDControllerData.AlramData.m_ucPowerDown&0x03) != 0x00 )		ullIndicatorFromX |= 0x0000000002000000;	//VCU5
	if( (g_OBDControllerData.AlramData.m_ucAHB&0x01) == 0x01 )				ullIndicatorFromX |= 0x0000000004000000;	//EBS1
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x01) == 0x01 )				ullIndicatorFromX |= 0x0000000008000000;	//HCU5
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x06) != 0x00 )				ullIndicatorFromX |= 0x0000000010000000;	//MCU2
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x08) == 0x08 )				ullIndicatorFromX |= 0x0000000020000000;	//BMS1
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x10) == 0x10 )				ullIndicatorFromX |= 0x0000000040000000;	//LDC1
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x20) == 0x20 )				ullIndicatorFromX |= 0x0000000080000000;	//TCU15
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x40) == 0x40 )				ullIndicatorFromX |= 0x0000000100000000;	//DATC1
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x80) == 0x80 )				ullIndicatorFromX |= 0x0000000200000000;	//OBC1
	if( (g_OBDControllerData.AlramData.m_ucEV&0x01) == 0x01 )					ullIndicatorFromX |= 0x0000000400000000;	//VCU5
	if( (g_OBDControllerData.AlramData.m_ucEV&0x02) == 0x02 )					ullIndicatorFromX |= 0x0000000800000000;	//BMS1
	if( (g_OBDControllerData.AlramData.m_ucEV&0x04) == 0x04 )					ullIndicatorFromX |= 0x0000001000000000;	//LDC1
	if( (g_OBDControllerData.AlramData.m_ucEV&0x08) == 0x08 )					ullIndicatorFromX |= 0x0000002000000000;	//MCU2
	if( (g_OBDControllerData.AlramData.m_ucEV&0x10) == 0x10 )					ullIndicatorFromX |= 0x0000004000000000;	//DATC3
	if( (g_OBDControllerData.AlramData.m_ucEV&0x20) == 0x20 )					ullIndicatorFromX |= 0x0000008000000000;	//OBC1
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x01) == 0x01 )				ullIndicatorFromX |= 0x0000010000000000;	//MCU2
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x02) == 0x02 )				ullIndicatorFromX |= 0x0000020000000000;	//BMS1
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x04) == 0x04 )				ullIndicatorFromX |= 0x0000040000000000;	//HDC2
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x08) == 0x08 )				ullIndicatorFromX |= 0x0000080000000000;	//BPCU4
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x10) == 0x10 )				ullIndicatorFromX |= 0x0000100000000000;	//LDC1
	if( (g_OBDControllerData.AlramData.m_ucTPMS) != 0x00 )						ullIndicatorFromX |= 0x0000200000000000;	//TPMS11
	
	if( g_OBDControllerData.AlramData. m_uc4WD != 0 )			 	uiNewAlramFlag |= 0x00000001;
	if( g_OBDControllerData.AlramData. m_ucABS != 0 )			 	uiNewAlramFlag |= 0x00000002;
	if( g_OBDControllerData.AlramData. m_ucAirbag != 0 ) 			uiNewAlramFlag |= 0x00000004;
	if( g_OBDControllerData.AlramData. m_ucAutoHold != 0 )		uiNewAlramFlag |= 0x00000008;
	if( g_OBDControllerData.AlramData. m_ucBatteryCharge != 0 )	uiNewAlramFlag |= 0x00000010;
	if( g_OBDControllerData.AlramData. m_ucCheckEngine != 0 ) 	uiNewAlramFlag |= 0x00000020;
	if( g_OBDControllerData.AlramData. m_ucDBCWarnning != 0 )	uiNewAlramFlag |= 0x00000040;
	if( g_OBDControllerData.AlramData. m_ucDPF != 0 ) 				uiNewAlramFlag |= 0x00000080;
	if( g_OBDControllerData.AlramData. m_ucSCR != 0 )				uiNewAlramFlag |= 0x00000100;
	if( g_OBDControllerData.AlramData. m_ucEPB != 0 )				uiNewAlramFlag |= 0x00000200;
	if( g_OBDControllerData.AlramData. m_ucTCS != 0 )				uiNewAlramFlag |= 0x00000400;
	if( g_OBDControllerData.AlramData. m_ucISG != 0 )				uiNewAlramFlag |= 0x00000800;
	if( g_OBDControllerData.AlramData. m_ucMDPS != 0 )			uiNewAlramFlag |= 0x00001000;
	if( g_OBDControllerData.AlramData. m_ucOilLevel != 0 )			uiNewAlramFlag |= 0x00002000;
	if( g_OBDControllerData.AlramData. m_ucOilPress != 0 )			uiNewAlramFlag |= 0x00004000;
	if( g_OBDControllerData.AlramData. m_ucParkingBrake != 0 )	uiNewAlramFlag |= 0x00008000;
	if( g_OBDControllerData.AlramData. m_ucPowerDown != 0 )	uiNewAlramFlag |= 0x00010000;
	if( g_OBDControllerData.AlramData. m_ucAHB != 0 )				uiNewAlramFlag |= 0x00020000;
	if( g_OBDControllerData.AlramData. m_ucHEV != 0 )				uiNewAlramFlag |= 0x00040000;
	if( g_OBDControllerData.AlramData. m_ucEV != 0 )					uiNewAlramFlag |= 0x00080000;
	if( g_OBDControllerData.AlramData. m_ucFCEV != 0 )				uiNewAlramFlag |= 0x00100000;
	if( g_OBDControllerData.AlramData. m_ucTPMS != 0 )			uiNewAlramFlag |= 0x00200000;
#if 0
	if( uiNewAlramFlag != 0 )
	{
		//변경된 알람 상태가 있으면 알람
		if( (uiNewAlramFlag&(~g_OBDControllerData.AlramData.m_uiAlramFlag)) != 0 )
		{
			printf("~Alram : %08X, %08X, %08X, %08X\r\n",(uiNewAlramFlag&(~g_OBDControllerData.AlramData.m_uiAlramFlag)),uiIndcatorFromX1,uiIndcatorFromX2,g_OBDControllerData.AlramData.m_uiAlramFlag);
			SendIndicatorAlramReport(eMESSAGE_EVENT_KEY_INDICATOR_ALRAM ,(uiNewAlramFlag&(~g_OBDControllerData.AlramData.m_uiAlramFlag)), uiIndcatorFromX2, uiIndcatorFromX1 );
		}
		g_OBDControllerData.AlramData.m_uiAlramFlag |= uiNewAlramFlag;
//		if( uiNewAlramFlag != 0 )
//		{
//			printf("~Alram : %08X, %08X, %08X, %08X\r\n",uiNewAlramFlag,uiIndcatorFromX1,uiIndcatorFromX2,g_OBDControllerData.AlramData.m_uiAlramFlag);
//			SendIndicatorAlramReport(eMESSAGE_EVENT_KEY_INDICATOR_ALRAM ,uiNewAlramFlag, uiIndcatorFromX2, uiIndcatorFromX1 );
//		}
//		g_OBDControllerData.AlramData.m_uiAlramFlag = uiNewAlramFlag;
	}
#else	
//	if( ullIndicatorFromX != 0 )
//	if( (ullIndicatorFromX&(~g_OBDControllerData.AlramData.m_ullIndicatorFromXFlag)) != 0 )
//	if( (ullIndicatorFromX&(~g_ullIndicator)) != 0 )
	if( (ullIndicatorFromX != g_ullIndicator) && (g_OBDControllerData.monitering_Data.m_bIG1==1) )
	{
		if( g_bIndicator3secCallbackRunningFlag == false )
		{
			printf("@@@@@@@@@@3secCallbakOn\r\n");
			g_iTimerIndicator3secCallback = HalTimerSetSWTimer(3000, eSWTimer_ONESHOT, CB_Indicator_3secCallback, TRUE);
			if( g_iTimerIndicator3secCallback != -1) g_bIndicator3secCallbackRunningFlag = true;
		}
	}
//	if( uiNewAlramFlag != 0 )
//	{
//		if( g_bIndicator3secCallbackRunningFlag == false )
//		{
//			printf("@@@@@@@@@@3secCallbakOn\r\n");
////			printf("############################################################################%X!!!!!!!!!!!!!!\r\n",uiNewAlramFlag);
//			g_iTimerIndicator3secCallback = HalTimerSetSWTimer(3000, eSWTimer_ONESHOT, CB_Indicator_3secCallback, TRUE);
//			if( g_iTimerIndicator3secCallback != -1) g_bIndicator3secCallbackRunningFlag = true;
//		}
//	}
#endif
}

void CB_Indicator_3secCallback()
{
	//unsigned int uiIndcatorFromX1=0, uiIndcatorFromX2=0; 
    unsigned int uiNewAlramFlag=0;
	unsigned long long ullIndicatorFromX = 0;
	
//	if( (g_OBDControllerData.AlramData.m_uc4WD&0xFF) != 0 )						uiIndcatorFromX1 |= 0x00000001;	//4WD11
//	if( (g_OBDControllerData.AlramData.m_ucABS&0x01) == 0x01 	)				uiIndcatorFromX1 |= 0x00000002;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucABS&0x02) == 0x02 )				uiIndcatorFromX1 |= 0x00000004;	//ABS11
//	if( (g_OBDControllerData.AlramData.m_ucAirbag) != 0x00 )						uiIndcatorFromX1 |= 0x00000008;	//ACU11
////	if( (g_OBDControllerData.AlramData.m_ucAirbag&0x1C) != 0x00 )				uiIndcatorFromX1 |= 0x00000010;	//CGW_CP5
//	if( (g_OBDControllerData.AlramData.m_ucAutoHold) != 0x00 )					uiIndcatorFromX1 |= 0x00000020;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucBatteryCharge&0x01) == 0x01 )	uiIndcatorFromX1 |= 0x00000040;	//EMS19
//	if( (g_OBDControllerData.AlramData.m_ucBatteryCharge&0x02) == 0x02 )	uiIndcatorFromX1 |= 0x00000080;	//LDC1
//	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x01) == 0x01 )	uiIndcatorFromX1 |= 0x00000100;	//EMS4
//	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x02) == 0x02 )	uiIndcatorFromX1 |= 0x00000200;	//BMS38_1
//	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x04) == 0x04 )	uiIndcatorFromX1 |= 0x00000400;	//HEV_CP2
//	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x08) == 0x08 )	uiIndcatorFromX1 |= 0x00000800;	//HEV_CP9
//	if( (g_OBDControllerData.AlramData.m_ucDBCWarnning&0x01) == 0x01 )	uiIndcatorFromX1 |= 0x00001000;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucDPF) != 0x00 )							uiIndcatorFromX1 |= 0x00002000;	//EMS19
//	if( (g_OBDControllerData.AlramData.m_ucSCR&0x01) == 0x01 )				uiIndcatorFromX1 |= 0x00004000;	//EMS21
//	if( (g_OBDControllerData.AlramData.m_ucEPB) != 0x00 )							uiIndcatorFromX1 |= 0x00008000;	//EPB11
//	if( (g_OBDControllerData.AlramData.m_ucTCS) != 0x00 )						uiIndcatorFromX1 |= 0x00010000;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucISG) != 0x00 )							uiIndcatorFromX1 |= 0x00020000;	//EMS_H12
////	if( (g_OBDControllerData.AlramData.m_ucISG&0x04) == 0x04 )					uiIndcatorFromX1 |= 0x00040000;	//CGW_PCS
//	if( (g_OBDControllerData.AlramData.m_ucMDPS) != 0x00 )						uiIndcatorFromX1 |= 0x00080000;	//CGW_EP2
//	if( (g_OBDControllerData.AlramData.m_ucOilLevel&0x01) == 0x01 )			uiIndcatorFromX1 |= 0x00100000;	//EMS_H12
//	if( (g_OBDControllerData.AlramData.m_ucOilPress&0x01) == 0x01 )			uiIndcatorFromX1 |= 0x00200000;	//EMS19
//	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x01) == 0x01 )	uiIndcatorFromX1 |= 0x00400000;	//ABS11
//	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x02) == 0x02 )	uiIndcatorFromX1 |= 0x00800000;	//TCS15
//	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x04) == 0x04 )	uiIndcatorFromX1 |= 0x01000000;	//EBP11
//	if( (g_OBDControllerData.AlramData.m_ucPowerDown&0x03) != 0x00 )		uiIndcatorFromX1 |= 0x02000000;	//VCU5
//	if( (g_OBDControllerData.AlramData.m_ucAHB&0x01) == 0x01 )				uiIndcatorFromX1 |= 0x04000000;	//EBS1
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x01) == 0x01 )				uiIndcatorFromX1 |= 0x08000000;	//HCU5
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x06) != 0x00 )				uiIndcatorFromX1 |= 0x10000000;	//MCU2
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x08) == 0x08 )				uiIndcatorFromX1 |= 0x20000000;	//BMS1
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x10) == 0x10 )				uiIndcatorFromX1 |= 0x40000000;	//LDC1
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x20) == 0x20 )				uiIndcatorFromX1 |= 0x80000000;	//TCU15
//	
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x40) == 0x40 )				uiIndcatorFromX2 |= 0x00000001;	//DATC1
//	if( (g_OBDControllerData.AlramData.m_ucHEV&0x80) == 0x80 )				uiIndcatorFromX2 |= 0x00000002;	//OBC1
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x01) == 0x01 )					uiIndcatorFromX2 |= 0x00000004;	//VCU5
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x02) == 0x02 )					uiIndcatorFromX2 |= 0x00000008;	//BMS1
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x04) == 0x04 )					uiIndcatorFromX2 |= 0x00000010;	//LDC1
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x08) == 0x08 )					uiIndcatorFromX2 |= 0x00000020;	//MCU2
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x10) == 0x10 )					uiIndcatorFromX2 |= 0x00000040;	//DATC3
//	if( (g_OBDControllerData.AlramData.m_ucEV&0x20) == 0x20 )					uiIndcatorFromX2 |= 0x00000080;	//OBC1
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x01) == 0x01 )				uiIndcatorFromX2 |= 0x00000100;	//MCU2
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x02) == 0x02 )				uiIndcatorFromX2 |= 0x00000200;	//BMS1
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x04) == 0x04 )				uiIndcatorFromX2 |= 0x00000400;	//HDC2
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x08) == 0x08 )				uiIndcatorFromX2 |= 0x00000800;	//BPCU4
//	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x10) == 0x10 )				uiIndcatorFromX2 |= 0x00001000;	//LDC1
//	if( (g_OBDControllerData.AlramData.m_ucTPMS) != 0x00 )						uiIndcatorFromX2 |= 0x00002000;	//TPMS11
	
	if( (g_OBDControllerData.AlramData.m_uc4WD&0xFF) != 0 )						ullIndicatorFromX |= 0x0000000000000001;	//4WD11
	if( (g_OBDControllerData.AlramData.m_ucABS&0x01) == 0x01 	)				ullIndicatorFromX |= 0x0000000000000002;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucABS&0x02) == 0x02 )				ullIndicatorFromX |= 0x0000000000000004;	//ABS11
	if( (g_OBDControllerData.AlramData.m_ucAirbag) != 0x00 )						ullIndicatorFromX |= 0x0000000000000008;	//ACU11
//	if( (g_OBDControllerData.AlramData.m_ucAirbag&0x1C) != 0x00 )				ullIndicatorFromX |= 0x0000000000000010;	//CGW_CP5
	if( (g_OBDControllerData.AlramData.m_ucAutoHold) != 0x00 )					ullIndicatorFromX |= 0x0000000000000020;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucBatteryCharge&0x01) == 0x01 )	ullIndicatorFromX |= 0x0000000000000040;	//EMS19
	if( (g_OBDControllerData.AlramData.m_ucBatteryCharge&0x02) == 0x02 )	ullIndicatorFromX |= 0x0000000000000080;	//LDC1
	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x01) == 0x01 )	ullIndicatorFromX |= 0x0000000000000100;	//EMS4
	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x02) == 0x02 )	ullIndicatorFromX |= 0x0000000000000200;	//BMS38_1
	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x04) == 0x04 )	ullIndicatorFromX |= 0x0000000000000400;	//HEV_CP2
	if( (g_OBDControllerData.AlramData.m_ucCheckEngine&0x08) == 0x08 )	ullIndicatorFromX |= 0x0000000000000800;	//HEV_CP9
	if( (g_OBDControllerData.AlramData.m_ucDBCWarnning&0x01) == 0x01 )	ullIndicatorFromX |= 0x0000000000001000;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucDPF) != 0x00 )							ullIndicatorFromX |= 0x0000000000002000;	//EMS19
	if( (g_OBDControllerData.AlramData.m_ucSCR&0x01) == 0x01 )				ullIndicatorFromX |= 0x0000000000004000;	//EMS21
	if( (g_OBDControllerData.AlramData.m_ucEPB) != 0x00 )							ullIndicatorFromX |= 0x0000000000008000;	//EPB11
	if( (g_OBDControllerData.AlramData.m_ucTCS) != 0x00 )						ullIndicatorFromX |= 0x0000000000010000;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucISG) != 0x00 )							ullIndicatorFromX |= 0x0000000000020000;	//EMS_H12
//	if( (g_OBDControllerData.AlramData.m_ucISG&0x04) == 0x04 )					ullIndicatorFromX |= 0x0000000000040000;	//CGW_PCS
	if( (g_OBDControllerData.AlramData.m_ucMDPS) != 0x00 )						ullIndicatorFromX |= 0x0000000000080000;	//CGW_EP2
	if( (g_OBDControllerData.AlramData.m_ucOilLevel&0x01) == 0x01 )			ullIndicatorFromX |= 0x0000000000100000;	//EMS_H12
	if( (g_OBDControllerData.AlramData.m_ucOilPress&0x01) == 0x01 )			ullIndicatorFromX |= 0x0000000000200000;	//EMS19
	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x01) == 0x01 )	ullIndicatorFromX |= 0x0000000000400000;	//ABS11
	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x02) == 0x02 )	ullIndicatorFromX |= 0x0000000000800000;	//TCS15
	if( (g_OBDControllerData.AlramData.m_ucParkingBrake&0x04) == 0x04 )	ullIndicatorFromX |= 0x0000000001000000;	//EBP11
	if( (g_OBDControllerData.AlramData.m_ucPowerDown&0x03) != 0x00 )		ullIndicatorFromX |= 0x0000000002000000;	//VCU5
	if( (g_OBDControllerData.AlramData.m_ucAHB&0x01) == 0x01 )				ullIndicatorFromX |= 0x0000000004000000;	//EBS1
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x01) == 0x01 )				ullIndicatorFromX |= 0x0000000008000000;	//HCU5
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x06) != 0x00 )				ullIndicatorFromX |= 0x0000000010000000;	//MCU2
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x08) == 0x08 )				ullIndicatorFromX |= 0x0000000020000000;	//BMS1
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x10) == 0x10 )				ullIndicatorFromX |= 0x0000000040000000;	//LDC1
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x20) == 0x20 )				ullIndicatorFromX |= 0x0000000080000000;	//TCU15
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x40) == 0x40 )				ullIndicatorFromX |= 0x0000000100000000;	//DATC1
	if( (g_OBDControllerData.AlramData.m_ucHEV&0x80) == 0x80 )				ullIndicatorFromX |= 0x0000000200000000;	//OBC1
	if( (g_OBDControllerData.AlramData.m_ucEV&0x01) == 0x01 )					ullIndicatorFromX |= 0x0000000400000000;	//VCU5
	if( (g_OBDControllerData.AlramData.m_ucEV&0x02) == 0x02 )					ullIndicatorFromX |= 0x0000000800000000;	//BMS1
	if( (g_OBDControllerData.AlramData.m_ucEV&0x04) == 0x04 )					ullIndicatorFromX |= 0x0000001000000000;	//LDC1
	if( (g_OBDControllerData.AlramData.m_ucEV&0x08) == 0x08 )					ullIndicatorFromX |= 0x0000002000000000;	//MCU2
	if( (g_OBDControllerData.AlramData.m_ucEV&0x10) == 0x10 )					ullIndicatorFromX |= 0x0000004000000000;	//DATC3
	if( (g_OBDControllerData.AlramData.m_ucEV&0x20) == 0x20 )					ullIndicatorFromX |= 0x0000008000000000;	//OBC1
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x01) == 0x01 )				ullIndicatorFromX |= 0x0000010000000000;	//MCU2
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x02) == 0x02 )				ullIndicatorFromX |= 0x0000020000000000;	//BMS1
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x04) == 0x04 )				ullIndicatorFromX |= 0x0000040000000000;	//HDC2
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x08) == 0x08 )				ullIndicatorFromX |= 0x0000080000000000;	//BPCU4
	if( (g_OBDControllerData.AlramData.m_ucFCEV&0x10) == 0x10 )				ullIndicatorFromX |= 0x0000100000000000;	//LDC1
	if( (g_OBDControllerData.AlramData.m_ucTPMS) != 0x00 )						ullIndicatorFromX |= 0x0000200000000000;	//TPMS11

	if( g_OBDControllerData.AlramData. m_uc4WD != 0 )			 	uiNewAlramFlag |= 0x00000001;
	if( g_OBDControllerData.AlramData. m_ucABS != 0 )			 	uiNewAlramFlag |= 0x00000002;
	if( g_OBDControllerData.AlramData. m_ucAirbag != 0 ) 			uiNewAlramFlag |= 0x00000004;
	if( g_OBDControllerData.AlramData. m_ucAutoHold != 0 )		uiNewAlramFlag |= 0x00000008;
	if( g_OBDControllerData.AlramData. m_ucBatteryCharge != 0 )	uiNewAlramFlag |= 0x00000010;
	if( g_OBDControllerData.AlramData. m_ucCheckEngine != 0 ) 	uiNewAlramFlag |= 0x00000020;
	if( g_OBDControllerData.AlramData. m_ucDBCWarnning != 0 )	uiNewAlramFlag |= 0x00000040;
	if( g_OBDControllerData.AlramData. m_ucDPF != 0 ) 				uiNewAlramFlag |= 0x00000080;
	if( g_OBDControllerData.AlramData. m_ucSCR != 0 )				uiNewAlramFlag |= 0x00000100;
	if( g_OBDControllerData.AlramData. m_ucEPB != 0 )				uiNewAlramFlag |= 0x00000200;
	if( g_OBDControllerData.AlramData. m_ucTCS != 0 )				uiNewAlramFlag |= 0x00000400;
	if( g_OBDControllerData.AlramData. m_ucISG != 0 )				uiNewAlramFlag |= 0x00000800;
	if( g_OBDControllerData.AlramData. m_ucMDPS != 0 )			uiNewAlramFlag |= 0x00001000;
	if( g_OBDControllerData.AlramData. m_ucOilLevel != 0 )			uiNewAlramFlag |= 0x00002000;
	if( g_OBDControllerData.AlramData. m_ucOilPress != 0 )			uiNewAlramFlag |= 0x00004000;
	if( g_OBDControllerData.AlramData. m_ucParkingBrake != 0 )	uiNewAlramFlag |= 0x00008000;
	if( g_OBDControllerData.AlramData. m_ucPowerDown != 0 )	uiNewAlramFlag |= 0x00010000;
	if( g_OBDControllerData.AlramData. m_ucAHB != 0 )				uiNewAlramFlag |= 0x00020000;
	if( g_OBDControllerData.AlramData. m_ucHEV != 0 )				uiNewAlramFlag |= 0x00040000;
	if( g_OBDControllerData.AlramData. m_ucEV != 0 )					uiNewAlramFlag |= 0x00080000;
	if( g_OBDControllerData.AlramData. m_ucFCEV != 0 )				uiNewAlramFlag |= 0x00100000;
	if( g_OBDControllerData.AlramData. m_ucTPMS != 0 )			uiNewAlramFlag |= 0x00200000;

	if(Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN && (g_OBDControllerData.monitering_Data.m_bIG1==1))
	{
		printf("~~~~~~~~~~~~~~~~~~~~~~!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!~~~22222222222Alram : %08X, %016llX, %016llX, %d\r\n",uiNewAlramFlag,ullIndicatorFromX,g_ullIndicator,g_bIndicatorFirstCheckFlag);
		SendIndicatorAlramReport(eMESSAGE_EVENT_KEY_INDICATOR_ALRAM ,uiNewAlramFlag, ullIndicatorFromX );
		g_ullIndicator = ullIndicatorFromX;
	}
////	if( (ullIndicatorFromX&(~g_OBDControllerData.AlramData.m_ullIndicatorFromXFlag)) != 0 )
//	if( (ullIndicatorFromX&(~g_ullIndicator)) != 0 )
//	{
//		printf("~~~~~~~~~~~~~~~~~~~~~~!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!~~~22222222222Alram : %016llX, %016llX, %016llX, %d\r\n",(ullIndicatorFromX&(~g_ullIndicator)),ullIndicatorFromX,g_ullIndicator,g_bIndicatorFirstCheckFlag);
//		SendIndicatorAlramReport(eMESSAGE_EVENT_KEY_INDICATOR_ALRAM ,(ullIndicatorFromX&(~g_ullIndicator)), ullIndicatorFromX );
//	
//		g_ullIndicator |= ullIndicatorFromX;
//	}
//	if( (uiNewAlramFlag&(~g_OBDControllerData.AlramData.m_uiAlramFlag)) != 0 )
//	{
//		printf("~~~~~~~~~~~~~~~~~~~~~~!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!~~~22222222222Alram : %08X, %08X, %08X, %08X\r\n",(uiNewAlramFlag&(~g_OBDControllerData.AlramData.m_uiAlramFlag)),uiIndcatorFromX1,uiIndcatorFromX2,g_OBDControllerData.AlramData.m_uiAlramFlag);
//		SendIndicatorAlramReport(eMESSAGE_EVENT_KEY_INDICATOR_ALRAM ,(uiNewAlramFlag&(~g_OBDControllerData.AlramData.m_uiAlramFlag)), uiIndcatorFromX2, uiIndcatorFromX1 );
//		
//		g_uiTemp = uiNewAlramFlag&(~g_OBDControllerData.AlramData.m_uiAlramFlag) ;
//		g_uiTemp2 = uiIndcatorFromX2;
//		g_uiTemp3 = uiIndcatorFromX1;
//		g_uiTemp4 = g_OBDControllerData.AlramData.m_uiAlramFlag;
//		g_uiTemp5 = g_OBDControllerData.AlramData.m_uiAlramFlag |= uiNewAlramFlag;
//		
//		g_OBDControllerData.AlramData.m_uiAlramFlag |= uiNewAlramFlag;
//	}
	
	g_bIndicator3secCallbackRunningFlag = false;
	HalTimerClearSWTimer(g_iTimerIndicator3secCallback);
	g_iTimerIndicator3secCallback = -1;
	
//	if( g_bIndicatorFirstCheckFlag==true)	g_bIndicatorFirstCheckFlag = false;
}

void CB_BLE_Disconnect_40minCallback()
{
	printf("~~~~~~~~~~~~~~~~~~~~~~CB_BLE_Disconnect_40minCallback ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
	HalGPIOSetVaule(GPIO_BT_RESET, eBIT_RESET);
	//SystemDelay(10);
	for(int i=0;i<220000;i++){}
	HalGPIOSetVaule(GPIO_BT_RESET, eBIT_SET);
	//SystemDelay(100);
	for(int i=0;i<2200000;i++){}
	HalGPIOSetVaule(GPIO_BT_RESET, eBIT_RESET);
	HalTimerClearSWTimer(g_iTimerBleNoDisconnectCallback);
	g_iTimerBleNoDisconnectCallback = -1;
}

//void SendIndicatorAlramReport(int32_t nEvent, unsigned int InputVal_1, unsigned int InputValue_2, unsigned int InputVal_3)
//{
//    stMsgSysMsg msg;
//    long long llOperationKey;
//    memset((int8_t*)&msg,0,sizeof(stMsgSysMsg));
//
//    msg.header.id = eMngObd;
//    msg.header.event = eReqReport;
//    msg.header.subEvent = eR_Alram;
//
//    msg.carReport.rpAlram.CarStatus.EventKey = nEvent;
//
//	sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"%08X,%08X%08X",InputVal_1,InputValue_2,InputVal_3);
//					  
//
//    GetSystemDrivingKey(&llOperationKey);
//    msg.carReport.rpAlram.CarStatus.OperationKey = llOperationKey;
//    msg.carReport.rpAlram.CarStatus.OccurredEventTime = GetLocalTime();
//    msg.carReport.rpAlram.CarStatus.OccurredEventUtcTime = GetUtcTimefromTime(GetLocalTime());
//    msg.carReport.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
//    msg.carReport.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
//    msg.carReport.rpAlram.CarStatus.GpsSetLatitude = 0;
//    msg.carReport.rpAlram.CarStatus.GpsSetLongitude = 0 ;
//    msg.carReport.rpAlram.CarStatus.Distance = 0;
//    msg.carReport.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;
//
//    Send2MngSysMsg3(&msg);
//}

void SendIndicatorAlramReport(int32_t nEvent, unsigned int InputVal_1, unsigned long long ullInput)
{
    stMsgSysMsg msg;
    long long llOperationKey;
    memset((int8_t*)&msg,0,sizeof(stMsgSysMsg));

    msg.header.id = eMngObd;
    msg.header.event = eReqReport;
    msg.header.subEvent = eR_Alram;

    msg.carReport.rpAlram.CarStatus.EventKey = nEvent;

	sprintf((char*)msg.carReport.rpAlram.CarStatus.EventKeyValue,"%08X,%016llX",InputVal_1,ullInput);

    GetSystemDrivingKey(&llOperationKey);
    msg.carReport.rpAlram.CarStatus.OperationKey = llOperationKey;
    msg.carReport.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.carReport.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    msg.carReport.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    msg.carReport.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
    msg.carReport.rpAlram.CarStatus.GpsSetLatitude = 0;
    msg.carReport.rpAlram.CarStatus.GpsSetLongitude = 0 ;
    msg.carReport.rpAlram.CarStatus.Distance = 0;
#if defined(PROTOCOL15)
	msg.carReport.rpAlram.CarStatus.Odometer = Get_Odmeter();
#endif
    msg.carReport.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;

    Send2MngSysMsg3(&msg);
}

/* update Total Data */
void UpdateOBDPeriodData(void)
{
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern0	+=	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern0;
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern1	+=	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern1;
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern2	+=	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern2;
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern3	+=	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern3;
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern4	+=	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern4;
#if defined(PROTOCOL17)
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern5	+=	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern5;
#endif
	/* speed */
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_0							+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_1_under_10		+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_1_under_10;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_11_under_20	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_11_under_20;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_21_under_30	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_21_under_30;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_31_under_40	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_31_under_40;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_41_under_50	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_41_under_50;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_51_under_60	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_51_under_60;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_61_under_70	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_61_under_70;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_71_under_80	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_71_under_80;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_81_under_90	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_81_under_90;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_91_under_100	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_91_under_100;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_101_under_110	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_101_under_110;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_111_under_120	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_111_under_120;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_121_under_130	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_121_under_130;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_131_under_140	+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_131_under_140;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_over_141					+= g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_over_141;

	/* rapid acceleration counter */
	g_OBDControllerData.monitering_Data.total_Rapid_acceleration_Counter
		+= g_OBDControllerData.monitering_Data.period_Rapid_acceleration_Counter;

	/* rapid deceleration counter */
	g_OBDControllerData.monitering_Data.total_Rapid_Deceleration_Counter
		+= g_OBDControllerData.monitering_Data.period_Rapid_Deceleration_Counter;

	/* Fuel */
	g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume
		+= g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume;

#if 1 // James Jean 2018/11/18
    /* 전기차 */
	g_OBDControllerData.monitering_Data.m_fTotal_Energy_Consume
		+= g_OBDControllerData.monitering_Data.m_fPeriod_Energy_Consume;
#endif
#if defined(PROTOCOL17)
	g_OBDControllerData.monitering_Data.m_fTotal_Energy_Consume_Regen
		+= g_OBDControllerData.monitering_Data.m_fPeriod_Energy_Consume_Regen;
#endif
}

/* init period Data for next period reporting */
void InitializeOBDPeriodData(void)
{
	g_ui1secTimer = 0;
	
	g_OBDControllerData.monitering_Data.GpsListCount=0;
	memset(&g_OBDControllerData.monitering_Data.GpsLatitudeList[0],0x00,sizeof(g_OBDControllerData.monitering_Data.GpsLatitudeList));
	memset(&g_OBDControllerData.monitering_Data.GpsLongitudeList[0],0x00,sizeof(g_OBDControllerData.monitering_Data.GpsLongitudeList));
	
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern0 =0;
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern1 =0;
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern2 =0;
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern3 =0;
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern4 =0;
#if defined(PROTOCOL17)
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern5 =0;
#endif
		/* init speed */
    g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_0 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_1_under_10 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_11_under_20 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_21_under_30 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_31_under_40 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_41_under_50 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_51_under_60 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_61_under_70 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_71_under_80 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_81_under_90 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_91_under_100 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_101_under_110 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_111_under_120 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_121_under_130 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_131_under_140 = 0;
	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_over_141 = 0;

	/* init rapid acceleration/deceleration counter */
	g_OBDControllerData.monitering_Data.period_Rapid_acceleration_Counter = 0;
	g_OBDControllerData.monitering_Data.period_Rapid_Deceleration_Counter = 0;

	/* init Fuel */
//	g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume = 0;

}


void InitializeOBDTripData(void)
{
    // before initiazlie obd, send odometer / last gps to autolinkconfig to save.
    // MONI 2018-02-19
	g_OBDControllerData.monitering_Data.EngineIdleTime =0;
    g_OBDControllerData.monitering_Data.WarmupTime =0;
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern0 =0;
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern1 =0;
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern2 =0;
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern3 =0;
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern4 =0;
#if defined(PROTOCOL17)
	g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern5 =0;
#endif

	g_bEngineIdleFlag = FALSE;
	g_bWarmupFlag = FALSE;
	g_bFuelLevelCheckFlag = false;

	g_OBDControllerData.monitering_Data.m_fDriving_Distance =0;
	
	if(g_bFuelLiterTypeFlag == true && g_bFuelLevelSwitchedPercentFlag == false )	g_dChecked_Fuel_Consume = 0;

	g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume =0;
#if 1 // James Jean 2018/11/18
   	g_OBDControllerData.monitering_Data.m_fTotal_Energy_Consume =0;
#endif
#if defined(PROTOCOL17)
	g_OBDControllerData.monitering_Data.m_fTotal_Energy_Consume_Regen =0;
#endif
	g_OBDControllerData.monitering_Data.m_usMaxSpeed =0;
	g_OBDControllerData.monitering_Data.m_usMaxRPM =0;

	g_OBDControllerData.monitering_Data.total_Rapid_acceleration_Counter =0;
	g_OBDControllerData.monitering_Data.total_Rapid_Deceleration_Counter =0;

//	g_OBDControllerData.monitering_Data.total_Speed_Counter_from_0_under_40 = 0;
//	g_OBDControllerData.monitering_Data.total_Speed_Counter_from_40_under_80 = 0;
//	g_OBDControllerData.monitering_Data.total_Speed_Counter_from_80_under_120 = 0;
//	g_OBDControllerData.monitering_Data.total_Speed_Counter_from_120_under_160 = 0;
//	g_OBDControllerData.monitering_Data.total_Speed_Counter_over_160 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_0 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_1_under_10 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_11_under_20 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_21_under_30 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_31_under_40 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_41_under_50 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_51_under_60 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_61_under_70 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_71_under_80 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_81_under_90 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_91_under_100 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_101_under_110 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_111_under_120 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_121_under_130 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_131_under_140 = 0;
	g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_over_141 = 0;
	
	g_OBDControllerData.monitering_Data.MileageAvg = 0;
	g_OBDControllerData.monitering_Data.SpeedTotal = 0;
	g_OBDControllerData.monitering_Data.SpeedCnt = 0;
	g_OBDControllerData.monitering_Data.SpeedAvg = 0;
	g_OBDControllerData.monitering_Data.uiRPMTotal = 0;
	g_OBDControllerData.monitering_Data.uiRPMCnt = 0;
	g_OBDControllerData.monitering_Data.uiRPMAvg = 0;
	g_OBDControllerData.monitering_Data.uiMotorRPMTotal = 0;
	g_OBDControllerData.monitering_Data.uiMotorRPMCnt = 0;
	g_OBDControllerData.monitering_Data.uiMotorRPMAvg = 0;
	g_OBDControllerData.monitering_Data.m_usMaxMotorRPM = 0;
	
	g_ucFuelListCount = 0;
	g_ucFuelListPosition = 0;
	memset(&g_ucFuelList,0x00,sizeof(g_ucFuelList));					
	memset(&g_stDistanceInfo,0x00,sizeof(g_stDistanceInfo));
	OFFSET_SET_STATE(GET_DISTANCE);	//g_stDistanceInfo 구조체를 초기화 하지않던가 이상태를 바꿔줘야 옵셋이 정상적으로 계산됨(처리후에는 주행때마다 옵셋 계산)
	g_OdometerFirstTimeFlag = TRUE;
	g_uiEnergyOldTime = Get_Tmr();
	g_uiFuelOldTime=Get_Tmr();
	g_uiCalFuelConsumptionOldTime=Get_Tmr();
    
	g_stTPMSAlramCheckState.m_bRL = false;
	g_stTPMSAlramCheckState.m_bRR = false;
	g_stTPMSAlramCheckState.m_bFR = false;
	g_stTPMSAlramCheckState.m_bFL = false;
}


/* D-CAN Data */
/* OBD Manager  ==> OBD Controller,  Only for OBD Manager */
void Send_Speed_From_CAN(unsigned int speed)
{
        if(Get_Decide_Starting_Engine() == FALSE)	return ;
	/* Converter speed type */
	g_OBDControllerData.monitering_Data.m_usCurrentSpeed = speed;

	/* change Max Speed */
	if (g_OBDControllerData.monitering_Data.m_usMaxSpeed < g_OBDControllerData.monitering_Data.m_usCurrentSpeed)
		g_OBDControllerData.monitering_Data.m_usMaxSpeed = g_OBDControllerData.monitering_Data.m_usCurrentSpeed;

	Decide_OverSpeed_Alarm();
}

void Send_RPM_From_CAN(unsigned int RPM)
{
	/* Converter speed type */
	g_OBDControllerData.monitering_Data.m_usRPM = RPM;
	//printf("***RPM : %d\r\n",g_OBDControllerData.monitering_Data.m_usRPM);

#if defined(QA_FIFA) 
	if(GetPACVType() == CV)
	{
		if(g_iTimerRPMCheckCallback != -1) 
		{
            //상용 차량 시동 OFF 시 CAN 데이터 수집 불가, CAN 데이터가 1초동안 들어오지 않을 경우 Engine OFF 체크 
			HalTimerChangeSWTimer(g_iTimerRPMCheckCallback, 1000, eSWTimer_ONESHOT, RPM_1SecCheckCallback, true);
		}
	}
#endif

	/* change Max RPM */
	if(Get_MaxRPM() < Get_RPM())
		g_OBDControllerData.monitering_Data.m_usMaxRPM = Get_RPM();

	Decide_OverRPM_Alarm();
	Decide_EngingButton_State();
}

boolean_t g_bIsBatteryData = false;

void Send_Battery_From_CAN(float battery)
{
	g_OBDControllerData.monitering_Data.m_fBattery = battery;	/* voltage * 10 */
	SetRCVBatteryFromCAN(true);
}

void SetRCVBatteryFromCAN(boolean_t bIsBatteryData)
{
	g_bIsBatteryData = bIsBatteryData;
}

boolean_t GetRCVBatteryFromCAN(void)
{
	return g_bIsBatteryData;	
}

void Send_Vehicle_Status_From_CAN(eVEHICLE_STATE Vehicle_state)
{
	static eVEHICLE_STATE s_eSaveState = eVEHICLE_STATE_OFF;
	if( s_eSaveState != Vehicle_state )
	{
		printf("ButtonStatus : %d->%d\r\n",s_eSaveState,Vehicle_state);
		s_eSaveState = Vehicle_state;
		
		if(BTGetConnectStatus())
		{
			Send_IGStatus_APP(Vehicle_state);
		}
		else
		{	
			if( Vehicle_state == eVEHICLE_STATE_OFF || Vehicle_state > eVEHICLE_STATE_IGON )
			{
			 	BTSend_VehicleStatus((unsigned char)Vehicle_state);
			}
		}
	}
	
	g_OBDControllerData.monitering_Data.m_ucVehicleStatus = Vehicle_state;
}

void Send_Odometer_From_CAN(unsigned int Odometer)
{
	g_OBDControllerData.monitering_Data.m_uiOdometer = Odometer;

	if (g_OdometerFirstTimeFlag == TRUE )
	{
		g_OdometerFirstTimeFlag = FALSE;
#ifdef DISTANCE
		printf("[Send_Odometer_From_CAN] %d \r\n",g_OBDControllerData.monitering_Data.m_uiOdometer);
#endif
		g_OBDControllerData.basicData.m_uiStartOdometer = g_OBDControllerData.monitering_Data.m_uiOdometer;
		g_OBDControllerData.basicData.m_uiOldOdometer = g_OBDControllerData.monitering_Data.m_uiOdometer;
	}
}

void Send_Odometer_From_File(unsigned int Odometer)
{
	g_OBDControllerData.monitering_Data.m_uiOdometer = Odometer;
}

float Kalmanfilter(float fData)
{
	// Kalman filter setup
	static float Pk = 1.0;
	static float Kk = 1.0;
	static float Xk = 50.0;
	float varP = 0.001;
	float R = 0.25;
	float kalmanMax = 100.0, kalmanMin=0.0;
	
	// Kalman Gain(K) 숫자가 클수록 측정치가 정확하고 숫자가 작을 수록 예측치가 정확
	Pk = Pk + varP;							// P = 현재P+예측노이즈공분산
    Kk = Pk / (Pk + R);						// Kalman Gain(K) = 예측치에러 / (예측치에러 + 측정치에러(측정노이즈공분산)))
    Xk = (Kk * fData) + (1 - Kk) * Xk;	// 
    Pk = (1 - Kk) * Pk;						//예측치에러(예측공분산)) = (1-K) * 예측치에러(예측공분산)
	
    if ( Xk > kalmanMax) Xk = kalmanMax;
    else if(Xk < kalmanMin) Xk = kalmanMin;
	
	return Xk;
}

void Send_Remain_Fuel_Percent_From_CAN(float fFuelLevel)
{
	int iSum = 0;
	int i=0;
	float fTemp=0;
	
	if( fFuelLevel != 0 )	//G80의 경우 시동을끄면 연료잔량이 0xFF가 온다
	{
		if(g_bFuelLiterTypeFlag == true )
		{	// 데이터가 L값으로 들어옴
	//		printf("FUEL_LEVEL : %fL consume: %lf\r\n",fFuelLevel,g_dChecked_Fuel_Consume);

			if( g_bFuelLevelSwitchedPercentFlag == false )
			{
				if( fFuelLevel >= g_ucFuelLevelMaxLiter )
				{
					g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter = g_ucFuelLevelMaxLiter;
					g_dChecked_Fuel_Consume = 0;	//연료 소모량 초기화
				}
				else
				{
					fTemp = fFuelLevel-(g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter-(float)g_dChecked_Fuel_Consume);
					if( ((abs((int)fTemp))>10.0) || (g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter==0.0) )
					{
						g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter = fFuelLevel;
						g_dChecked_Fuel_Consume = 0;
					}
					else{}
				}
			}
			else
			{
				g_OBDControllerData.monitering_Data.m_ucFuel_Level = (unsigned char)fFuelLevel; //mod.kks
				g_fFuelLevelPercent = g_OBDControllerData.monitering_Data.m_ucFuel_Level;
			}
		}
		else	// 데이터가 %값으로 옴
		{
//			printf("Fuel %d ->",fFuelLevel);
			if( fFuelLevel >= 100 )	g_OBDControllerData.monitering_Data.m_ucFuel_Level=100;
			else
			{
				if(fFuelLevel != g_OBDControllerData.monitering_Data.m_ucFuel_Level)
				{
					g_OBDControllerData.monitering_Data.m_ucFuel_Level = (unsigned char)Kalmanfilter(fFuelLevel);
//					printf(" Kal %d \r\n",g_OBDControllerData.monitering_Data.m_ucFuel_Level);
				}
				else
				{
	//				printf("same ");
					g_OBDControllerData.monitering_Data.m_ucFuel_Level = (unsigned char)fFuelLevel;
				}
			}
			
			g_ucFuelList[g_ucFuelListPosition] = g_OBDControllerData.monitering_Data.m_ucFuel_Level;
			g_ucFuelListCount++;
			g_ucFuelListPosition++;
			if( g_ucFuelListPosition == FUEL_LIST_MAX_CNT ) g_ucFuelListPosition = 0;
			if( g_ucFuelListCount >= FUEL_LIST_MAX_CNT ) g_ucFuelListCount = FUEL_LIST_MAX_CNT;		
			
			//평균구하기
			for(i=0; i<g_ucFuelListCount; i++)
			{
				iSum = iSum + g_ucFuelList[i];
			}
			g_OBDControllerData.monitering_Data.m_ucFuel_Level = (unsigned char)(iSum/g_ucFuelListCount);
//			printf(" avr %d %d\r\n",g_OBDControllerData.monitering_Data.m_ucFuel_Level,g_ucFuelListCount );
			
			g_fFuelLevelPercent = g_OBDControllerData.monitering_Data.m_ucFuel_Level;
			
			Decide_Fuel_Run_Out_Alarm();
		}
	}

#if SLOPE_TCU_LOG_FLAG

	stSensorInfo stGyroAngle;	 
	GetGyroAngle(&stGyroAngle);

	printf("@@@@@@@@@@@@@@@@@@@@@@@\r\nSend_Remain_Fuel_Percent_From_CAN\r\n@@@@@@@@@@@@@@@@@@@@@@@\r\n");
	if(g_bFuelLiterTypeFlag == true && g_bFuelLevelSwitchedPercentFlag == false)
		printf("Fuel Level] %lf, x:%d, y:%d \r\n", g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter, stGyroAngle.nX, stGyroAngle.nY);
	else 
		printf("Fuel Level] %d, x:%d, y:%d \r\n", g_OBDControllerData.monitering_Data.m_ucFuel_Level, stGyroAngle.nX, stGyroAngle.nY);
#endif
}

void Send_Slope_TCU_From_CAN(float SlopeTcu)
{
	static unsigned char s_ucTCUCnt = 0;
	stSensorInfo stGyroAngle;	
	GetGyroNavieAngle(&stGyroAngle);
	
	g_OBDControllerData.monitering_Data.m_fSlope_TCU = SlopeTcu;				

	if(stGyroAngle.bCalibration == false) // Flash에서 저장 값 없으면 타고 있으면 무시
	{				
		if(Discrimination_Stable_Slope_TCU() == true && SlopeTcu == 0) 
		{
			s_ucTCUCnt++;
			
			if(s_ucTCUCnt >= 150 )
			{
#if SLOPE_TCU_LOG_FLAG
				printf("Set Angle int Send_Slope_TCU_From_CAN] x:%d, y:%d, z:%d\r\n", stGyroAngle.nX, stGyroAngle.nY, stGyroAngle.nZ);
				printf("BS:%d, GP:%d, GL:%c, CNT:%d, TCU:%f\r\n",Get_FootBrake_State(), g_ucGearPos, Get_GearPosition(), s_ucTCUCnt,SlopeTcu);
#endif
				SetGyroAngle(&stGyroAngle);
				SetGyroInitializeAngle(&stGyroAngle);
				UpdataGyroDefualtAngle();
					
				if(g_ucTCUSlopeAngleArrIndex == TCUSLOPE_MAX_COUNT)
				{
#if SLOPE_TCU_LOG_FLAG
					printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
					printf("!!!!!!Set Angle!!!!!!\r\nTCU : %lf, Spd : %d\r\n", SlopeTcu, g_OBDControllerData.monitering_Data.m_usCurrentSpeed); // tcu 각도 / 속도 / 우리 자이로 각도	
					printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
#endif
					stGyroAngle.bCalibration = true;
					SetGyroInitializeAngle(&stGyroAngle);
					UpdataGyroDefualtAngle();
				}
				else
				{	
#if SLOPE_TCU_LOG_FLAG
					printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
					printf("!!!!!!Set %d] Angle!!!!!!\r\nTCU : %lf, Spd : %d\r\n",g_ucTCUSlopeAngleArrIndex, SlopeTcu, g_OBDControllerData.monitering_Data.m_usCurrentSpeed); // tcu 각도 / 속도 / 우리 자이로 각도	
					printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
#endif
					s_ucTCUCnt = 0;
				}
				
			}
		}
		else
		{
			s_ucTCUCnt = 0;
		}
	}
}

void Send_AccelPos_From_CAN(unsigned char AccelPos)
{
	g_OBDControllerData.monitering_Data.m_ucAccelPos = AccelPos;
}

void Send_Fuel_Consume_From_CAN(double fuelConsume, unsigned char ucType) //mL
{
        CalFuelConsumption( fuelConsume, ucType );
        Mileage(ucType);	//평균연비
}

void Send_Coolant_Temperature_From_CAN(short sCoolantTemperature)
{
	g_OBDControllerData.monitering_Data.m_sCoolantTemperature = sCoolantTemperature;
//	printf("DCS_ENGINEOIL_TEMP_IN : %d\r\n",g_OBDControllerData.monitering_Data.m_sEngineTemperature);

	if(Get_VehicleStatus()>=eVEHICLE_STATE_ENGRUN)  Decide_Coolant_Temperature_Alarm();
}

void Send_GearPosition_From_CAN(unsigned int gearPosition)
{
	/* change ASCII */
	g_OBDControllerData.monitering_Data.m_ucGearPosition = gearPosition;
}

void Send_TransmissionOil_Temperature_From_CAN(unsigned int transmissionOilTemperature)
{
	/* change 'c*10  */
	g_OBDControllerData.monitering_Data.m_usTransmissionOil_Temperature = transmissionOilTemperature;
}

void Send_BrakeLamp_From_CAN(bool BrakeLamp)
{
//	g_OBDControllerData.monitering_Data.m_bBrakeLamp = BrakeLamp;
}

void Send_TailLamp_From_CAN(unsigned int tailLampOn)
{
	g_OBDControllerData.monitering_Data.m_bTailLamp = tailLampOn;
//	Decide_Tail_Lamp_Alarm();
}

void Send_HeadLamp_From_CAN(unsigned int HeadLampOn)
{
	g_OBDControllerData.monitering_Data.m_bHeadLamp = HeadLampOn;
}

void Send_FootBrake_From_CAN(bool FootBrakeOn)
{
	g_OBDControllerData.monitering_Data.m_bFootBrake = FootBrakeOn;
#if defined(FEATURE_EXTENSION_BOARD)
    Decide_Foot_Break_Alarm();
#endif
}

void Send_MILLamp_From_CAN(unsigned int MILLampOn)
{
//	printf("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM %d",MILLampOn);
	g_OBDControllerData.monitering_Data.m_bMILLamp = MILLampOn;
	Decide_MIL_Lamp_Alarm();
}

#if 0
void Send_DTC_From_CAN(unsigned char *DTC)
{
}
#endif

/* others */
void Send_Impact_From_CAN(unsigned short Impact)
{
//	g_OBDControllerData.currentState.m_ucImpact = Impact;
//	Decide_Impact_Alarm();
}

void Send_DTC_From_CAN(unsigned char *DTC)
{

}

void Send_Driving_Distance_From_CAN(float Distance)
{
	g_OBDControllerData.monitering_Data.m_fDriving_Distance = Distance;
}
void Send_ExtraDrivingDistance_From_CAN(unsigned short Distance)
{
	g_OBDControllerData.monitering_Data.m_usExtraDrivingDistance = Distance;
	g_stElectricCarData.m_usRemainedDistance = Distance;
}

void Send_Airbag_From_CAN(bool airback)
{
//	g_OBDControllerData.currentState.airBag = airback;
}

void Send_hazardLamp_From_CAN(bool HazardLamp)
{
	if( (g_eLampCheckState == eLAMP_STATE_CHECKING) && (HazardLamp == 1) )
	{
		g_eLampCheckState = eLAMP_STATE_LAMPON;
	}
	g_OBDControllerData.monitering_Data.m_bHazardLamp = HazardLamp;
}

void Send_RearDefog_From_CAN(bool RearDefog)
{
	g_OBDControllerData.monitering_Data.m_bRearDefog = RearDefog;
}

void Send_HighBeam_From_CAN(bool HighBeam)
{
	g_OBDControllerData.monitering_Data.m_bHighBeam = HighBeam;
}

//void Send_OutLamp_From_CAN(bool OutLamp)
//{
//	g_OBDControllerData.monitering_Data.m_bOutLamp = OutLamp;
//}

void Send_DoorLock_FL_From_CAN(char doorLock_FL)
{
	if( doorLock_FL != (g_OBDControllerData.currentState.doorLock&0x01))
	{
		if (doorLock_FL)
			g_OBDControllerData.currentState.doorLock |= (1);
		else
			g_OBDControllerData.currentState.doorLock &= ~(1);
		
		g_stLockState.m_ucDoorLock = g_OBDControllerData.currentState.doorLock;
		g_stDoorUnlockAlramCheckState.m_bAlram=true;
	}
	
	if( g_stDoorUnlockAlramCheckState.m_bAlram == true )	g_stDoorUnlockAlramCheckState.m_bFL=true;
	Decide_Door_Lock_Alarm();
}
void Send_DoorLock_FR_From_CAN(char doorLock_FR)
{
	if( doorLock_FR != ((g_OBDControllerData.currentState.doorLock>>1)&0x01))
	{
		if (doorLock_FR)
			g_OBDControllerData.currentState.doorLock |= (1<<1);
		else
			g_OBDControllerData.currentState.doorLock &= ~(1<<1);
		
		g_stLockState.m_ucDoorLock = g_OBDControllerData.currentState.doorLock;
		g_stDoorUnlockAlramCheckState.m_bAlram=true;
	}

	if( g_stDoorUnlockAlramCheckState.m_bAlram == true )	g_stDoorUnlockAlramCheckState.m_bFR=true;

	Decide_Door_Lock_Alarm();
}
void Send_DoorLock_RL_From_CAN(char doorLock_RL)
{
	if( doorLock_RL != ((g_OBDControllerData.currentState.doorLock>>2)&0x01))
	{
		if (doorLock_RL)
			g_OBDControllerData.currentState.doorLock |= (1<<2);
		else
			g_OBDControllerData.currentState.doorLock &= ~(1<<2);
		
		g_stLockState.m_ucDoorLock = g_OBDControllerData.currentState.doorLock;
		g_stDoorUnlockAlramCheckState.m_bAlram=true;
	}
	
	if( g_stDoorUnlockAlramCheckState.m_bAlram == true )	g_stDoorUnlockAlramCheckState.m_bRL=true;

	Decide_Door_Lock_Alarm();
}
void Send_DoorLock_RR_From_CAN(char doorlock_RR)
{
	if( doorlock_RR != ((g_OBDControllerData.currentState.doorLock>>3)&0x01))
	{
		if (doorlock_RR)
			g_OBDControllerData.currentState.doorLock |= (1<<3);
		else
			g_OBDControllerData.currentState.doorLock &= ~(1<<3);
		
		g_stLockState.m_ucDoorLock = g_OBDControllerData.currentState.doorLock;
		g_stDoorUnlockAlramCheckState.m_bAlram=true;
	}

	if( g_stDoorUnlockAlramCheckState.m_bAlram == true )	g_stDoorUnlockAlramCheckState.m_bRR=true;

	Decide_Door_Lock_Alarm();
}
void Send_TrunkLock_From_CAN(char TrunkLock)
{
//	static bool s_bOldState=false;
//
//	if( TrunkLock != s_bOldState)
//	{
//		s_bOldState = TrunkLock;
//		if (TrunkLock)
//			g_OBDControllerData.currentState.doorLock |= (1<<4);
//		else
//			g_OBDControllerData.currentState.doorLock &= ~(1<<4);
//		
//		g_stLockState.m_ucDoorLock = g_OBDControllerData.currentState.doorLock;
//		g_stDoorUnlockAlramCheckState.m_bAlram=true;
//	}
//	if( g_stDoorUnlockAlramCheckState.m_bAlram == true )	g_stDoorUnlockAlramCheckState.m_bTR=true;
//
//	Decide_Door_Lock_Alarm();
}


void Send_DoorOpen_FL_From_CAN(char doorOpen_FL)
{
	if( doorOpen_FL != (g_OBDControllerData.currentState.doorOpen&0x01))
	{
		if (doorOpen_FL)
			g_OBDControllerData.currentState.doorOpen |= (1);
		else
			g_OBDControllerData.currentState.doorOpen &= ~(1);
		
		g_stLockState.m_ucDoorOpen = g_OBDControllerData.currentState.doorOpen;
		g_stDoorOpenAlramCheckState.m_bAlram = true;
	}
	if( g_stDoorOpenAlramCheckState.m_bAlram == true )	g_stDoorOpenAlramCheckState.m_bFL=true;

	Decide_Door_Open_Alarm();
}
void Send_DoorOpen_FR_From_CAN(char doorOpen_FR)
{
	if( doorOpen_FR != ((g_OBDControllerData.currentState.doorOpen>>1)&0x01))
	{
		if (doorOpen_FR)
			g_OBDControllerData.currentState.doorOpen |= (1<<1);
		else
			g_OBDControllerData.currentState.doorOpen &= ~(1<<1);
		
		g_stLockState.m_ucDoorOpen = g_OBDControllerData.currentState.doorOpen;
		g_stDoorOpenAlramCheckState.m_bAlram = true;
	}
	if( g_stDoorOpenAlramCheckState.m_bAlram == true )	g_stDoorOpenAlramCheckState.m_bFR=true;

	Decide_Door_Open_Alarm();
}
void Send_DoorOpen_RL_From_CAN(char doorOpen_RL)
{
	if( doorOpen_RL != ((g_OBDControllerData.currentState.doorOpen>>2)&0x01))
	{
		if (doorOpen_RL)
			g_OBDControllerData.currentState.doorOpen |= (1<<2);
		else
			g_OBDControllerData.currentState.doorOpen &= ~(1<<2);
		
		g_stLockState.m_ucDoorOpen = g_OBDControllerData.currentState.doorOpen;
		g_stDoorOpenAlramCheckState.m_bAlram = true;
	}
	if( g_stDoorOpenAlramCheckState.m_bAlram == true )	g_stDoorOpenAlramCheckState.m_bRL=true;

	Decide_Door_Open_Alarm();
}
void Send_DoorOpen_RR_From_CAN(char doorOpen_RR)
{
	if( doorOpen_RR != ((g_OBDControllerData.currentState.doorOpen>>3)&0x01))
	{
		if (doorOpen_RR)
			g_OBDControllerData.currentState.doorOpen |= (1<<3);
		else
			g_OBDControllerData.currentState.doorOpen &= ~(1<<3);
		
		g_stLockState.m_ucDoorOpen = g_OBDControllerData.currentState.doorOpen;
		g_stDoorOpenAlramCheckState.m_bAlram = true;
	}
	if( g_stDoorOpenAlramCheckState.m_bAlram == true )	g_stDoorOpenAlramCheckState.m_bRR=true;

	Decide_Door_Open_Alarm();
}
void Send_TrunkOpen_From_CAN(char TrunkOpen)
{
	if( TrunkOpen != ((g_OBDControllerData.currentState.doorOpen>>4)&0x01))
	{
		if (TrunkOpen)
			g_OBDControllerData.currentState.doorOpen |= (1<<4);
		else
			g_OBDControllerData.currentState.doorOpen &= ~(1<<4);
		
		g_stLockState.m_ucDoorOpen = g_OBDControllerData.currentState.doorOpen;
		g_stDoorOpenAlramCheckState.m_bAlram = true;
	}
	if( g_stDoorOpenAlramCheckState.m_bAlram == true )	g_stDoorOpenAlramCheckState.m_bTR=true;

	Decide_Door_Open_Alarm();
}

void Send_HoodOpen_From_CAN(char HoodOpen)
{
	if( HoodOpen != ((g_OBDControllerData.currentState.doorOpen>>5)&0x01))
	{
		if (HoodOpen)
			g_OBDControllerData.currentState.doorOpen |= (1<<5);
		else
			g_OBDControllerData.currentState.doorOpen &= ~(1<<5);
		
		g_stLockState.m_ucDoorOpen = g_OBDControllerData.currentState.doorOpen;
		g_stDoorOpenAlramCheckState.m_bAlram = true;
	}
	if( g_stDoorOpenAlramCheckState.m_bAlram == true )	g_stDoorOpenAlramCheckState.m_bHood=true;

	Decide_Door_Open_Alarm();
}

void Send_TPMS_FL_From_CAN(float TPMS_FL)
{
	bool bNewState = false;

	g_OBDControllerData.currentState.TPMS_FL = TPMS_FL;
	
	if( (g_OBDControllerData.currentState.TPMS_FL != 0) && (g_OBDControllerData.currentState.TPMS_FR != 0) &&
	    (g_OBDControllerData.currentState.TPMS_RL != 0) && (g_OBDControllerData.currentState.TPMS_RR != 0) )
	{
		bNewState = Decide_Each_TPMS( TPMS_FL );
		
		if( bNewState == true )
		{
			if( g_stTPMSAlramCheckState.m_bFL == false ) 
			{
				g_stTPMSAlramCheckState.m_bFL = true;
				Decide_TPMS_Alarm();
			}
		}
	}
}

void Send_TPMS_FR_From_CAN(float TPMS_FR)
{
	bool bNewState = false;

	g_OBDControllerData.currentState.TPMS_FR = TPMS_FR;
	
	if( (g_OBDControllerData.currentState.TPMS_FL != 0) && (g_OBDControllerData.currentState.TPMS_FR != 0) &&
	    (g_OBDControllerData.currentState.TPMS_RL != 0) && (g_OBDControllerData.currentState.TPMS_RR != 0) )
	{
		bNewState = Decide_Each_TPMS( TPMS_FR );

		if( bNewState == true )
		{
			if( g_stTPMSAlramCheckState.m_bFR == false ) 
			{
				g_stTPMSAlramCheckState.m_bFR = true;
				Decide_TPMS_Alarm();
			}
		}
	}
}

void Send_TPMS_RL_From_CAN(float TPMS_RL)
{
	bool bNewState = false;

	g_OBDControllerData.currentState.TPMS_RL = TPMS_RL;

	if( (g_OBDControllerData.currentState.TPMS_FL != 0) && (g_OBDControllerData.currentState.TPMS_FR != 0) &&
	    (g_OBDControllerData.currentState.TPMS_RL != 0) && (g_OBDControllerData.currentState.TPMS_RR != 0) )
	{
		bNewState = Decide_Each_TPMS( TPMS_RL );

		if( bNewState == true )
		{
			if( g_stTPMSAlramCheckState.m_bRL == false ) 
			{
				g_stTPMSAlramCheckState.m_bRL = true;
				Decide_TPMS_Alarm();
			}
		}
	}
}

void Send_TPMS_RR_From_CAN(float TPMS_RR)
{
	bool bNewState = false;

	g_OBDControllerData.currentState.TPMS_RR = TPMS_RR;

	if( (g_OBDControllerData.currentState.TPMS_FL != 0) && (g_OBDControllerData.currentState.TPMS_FR != 0) &&
	    (g_OBDControllerData.currentState.TPMS_RL != 0) && (g_OBDControllerData.currentState.TPMS_RR != 0) )
	{
		bNewState = Decide_Each_TPMS( TPMS_RR );

		if( bNewState == true )
		{
			if( g_stTPMSAlramCheckState.m_bRR == false ) 
			{
				g_stTPMSAlramCheckState.m_bRR = true;
				Decide_TPMS_Alarm();
			}
		}
	}
}

void Send_TPMS_Unit_From_CAN(unsigned short usTPMS_Conv)
{
	if( (CFD_GetCanFDAdapter()) && (FUELTYPE_GET_STATE() != FCEV)) // 호주 Fleet NEXO == 일반 CAN 차종에 CANFD 모듈 연결로 인하여 예외처리
	{
		if(usTPMS_Conv == 0)
		{
			g_OBDControllerData.currentState.fTPMS_Conv = 6.894733;
			if( g_bTPMSConvCheckFlag == true )
			{
				g_uiTpmsAlramValue *= 0.145038;
				g_bTPMSConvCheckFlag = false;
			}
		}
		else if(usTPMS_Conv == 1)
		{
			g_OBDControllerData.currentState.fTPMS_Conv = 5;
		}
		else
		{
			g_OBDControllerData.currentState.fTPMS_Conv = 10;
			if( g_bTPMSConvCheckFlag == true )
			{
				g_uiTpmsAlramValue *= 0x01;
				g_bTPMSConvCheckFlag = false;
			}
		}

	}
	else
	{
		if(usTPMS_Conv >= 0x00 && usTPMS_Conv <= 0x07 )	//PSI
		{
			g_OBDControllerData.currentState.fTPMS_Conv = 6.894733;
			if( g_bTPMSConvCheckFlag == true )
			{
				g_uiTpmsAlramValue *= 0.145038;
				g_bTPMSConvCheckFlag = false;
			}
		}
		else if( usTPMS_Conv >= 0x08 && usTPMS_Conv <= 0x0F)	//KPA
		{
			g_OBDControllerData.currentState.fTPMS_Conv = 5;
		}
		else //( Get_TMPS_Unit() >= 0x10)	//BAR
		{
			g_OBDControllerData.currentState.fTPMS_Conv = 10;
			if( g_bTPMSConvCheckFlag == true )
			{
				g_uiTpmsAlramValue *= 0x01;
				g_bTPMSConvCheckFlag = false;
			}
		}	
	}
//		printf("usTPMS_Conv : %d g_OBDControllerData.currentState.fTPMS_Conv: %f g_uiTpmsAlramValue:%d ",usTPMS_Conv,g_OBDControllerData.currentState.fTPMS_Conv,g_uiTpmsAlramValue);
}

void Increase_period_Rapid_acceleration_Counter(void)
{
        g_OBDControllerData.monitering_Data.period_Rapid_acceleration_Counter++;

}
void Increase_period_Rapid_Deceleration_Counter(void)
{
        g_OBDControllerData.monitering_Data.period_Rapid_Deceleration_Counter++;
}


void Increase_WarmupTime(void)
{
	g_OBDControllerData.monitering_Data.WarmupTime++;

}
void Increase_EngineIdleTime(void)
{
	g_OBDControllerData.monitering_Data.EngineIdleTime++;
}
void Increase_DrivePattern0(void)
{
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern0++;
}
void Increase_DrivePattern1(void)
{
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern1++;
}
void Increase_DrivePattern2(void)
{
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern2++;
}
void Increase_DrivePattern3(void)
{
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern3++;
}
void Increase_DrivePattern4(void)
{
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern4++;
}
#if defined(PROTOCOL17)
void Increase_DrivePattern5(void)
{
	g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern5++;
}
#endif

void Send_MassAirFlow_From_CAN(float MassAirFlow)
{
	g_OBDControllerData.monitering_Data.m_fMassAirFlow = MassAirFlow;
}
void Send_MassAirPressure_From_CAN(float MassAirPressure)
{
	g_OBDControllerData.monitering_Data.m_fMassAirPressure= MassAirPressure;
}
void Send_IntakeAirTemperature_From_CAN(float intakeAirTemperature)
{
 	g_OBDControllerData.monitering_Data.m_fIntakeAirTemperature = intakeAirTemperature;
}
void Send_OutsideTemperature_From_CAN(char Temperature)
{
	if((Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN) && (Get_VehicleStatus() <= eVEHICLE_STATE_EV_ON))
	{
		g_OBDControllerData.monitering_Data.m_cOutsideTemperature = Temperature;
	}
	else{}
}
void Send_HighBatteryTemperature_From_CAN(short Temperature)
{
	g_OBDControllerData.monitering_Data.m_sHighBatteryTemperature = Temperature;
	g_stElectricCarData.m_usHighBatteryTemperatureMin = Temperature;
}

#if defined(PROTOCOL18)
void Send_HighBatteryTemperatureMAX_From_CAN(short Temperature) //dahae
{
	g_OBDControllerData.monitering_Data.m_sHighBatteryTemperatureMax = Temperature;
	g_stElectricCarData.m_usHighBatteryTemperatureMax = Temperature;
}

void Send_ChargingState_From_CAN(bool ChargingState)
{
	if( g_OBDControllerData.monitering_Data.m_bChargingState != ChargingState)
	{	
		if(ChargingState == false)
		{
			ReportChargingStatus();
		}
		
		g_OBDControllerData.monitering_Data.m_bChargingState = ChargingState;
		ReportAlramStatus(eMESSAGE_EVENT_KEY_CHARGING_STATE,g_OBDControllerData.monitering_Data.m_bChargingState);
	}
}

void Send_ExtraChargeTime_From_CAN(unsigned short ExtraChargeTime)
{

	static unsigned int s_uiExtraChargeTimeReportTime = 0;
	static boolean_t s_bExtraChargeTimeReportStartFlag = false;
	
	g_OBDControllerData.monitering_Data.m_usExtraChargeTime = ExtraChargeTime;
	
	if( Get_ChargingState()==true )
	{
		if( s_bExtraChargeTimeReportStartFlag == false )
		{
			if(g_OBDControllerData.monitering_Data.m_usExtraChargeTime != 0)
			{
				s_bExtraChargeTimeReportStartFlag = true;
				s_uiExtraChargeTimeReportTime = GetUTCTime();
				//ReportAlramStatus(eMESSAGE_EVENT_KEY_EXTRA_CHARGE_TIME,g_OBDControllerData.monitering_Data.m_ucExtraChargeTime);
				ReportChargingStatus();
			}
		}
		else 
		{
			if( (s_uiExtraChargeTimeReportTime + EXTRA_CHARGE_TIME_REPORT_PERIOD) <= GetUTCTime() )
			{
				s_uiExtraChargeTimeReportTime = GetUTCTime();
				//ReportAlramStatus(eMESSAGE_EVENT_KEY_EXTRA_CHARGE_TIME,g_OBDControllerData.monitering_Data.m_ucExtraChargeTime);
				ReportChargingStatus();
			}
		}
	}
	else
	{
		s_bExtraChargeTimeReportStartFlag = false;
	}
}
#endif

void Send_InjectionQuantity_From_CAN(float fValue)
{
	g_stDieselFuelConsume.m_fInjectionQuantity = fValue;
}
void Send_MI_From_CAN(float fValue)
{
	g_stDieselFuelConsume.m_fMI = fValue;
}
void Send_PIL1_From_CAN(float fValue)
{
	g_stDieselFuelConsume.m_fPIL1 = fValue;
}
void Send_PIL2_From_CAN(float fValue)
{
	g_stDieselFuelConsume.m_fPIL2 = fValue;
}
void Send_PIL3_From_CAN(float fValue)
{
	g_stDieselFuelConsume.m_fPIL3 = fValue;
}
void Send_POL1_From_CAN(float fValue)
{
	g_stDieselFuelConsume.m_fPOL1 = fValue;
}
void Send_POL2_From_CAN(float fValue)
{
	g_stDieselFuelConsume.m_fPOL2 = fValue;
}
void Send_FuelCut_From_CAN(unsigned char fuelCut)
{
	g_OBDControllerData.monitering_Data.FuelCut = fuelCut;
}
void Send_Total_Fuel_Consume(float totalFuelConsume) //mL
{
  	g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume = totalFuelConsume;
}

void Send_Mileage_Avg(float mileAgeAvg)
{
  	g_OBDControllerData.monitering_Data.MileageAvg = mileAgeAvg;
}
void Send_ElectronicMileage_Avg(float mileAgeAvg)
{
  	g_OBDControllerData.monitering_Data.m_fElectronicMileageAvg = mileAgeAvg;
}

void Send_Speed_Total(long long speedTotal)
{
   	g_OBDControllerData.monitering_Data.SpeedTotal = speedTotal;
}
void increase_Speed_Cnt(void)
{
   	g_OBDControllerData.monitering_Data.SpeedCnt++;

}
void Send_Speed_Avg(float speedAvg)
{
   	g_OBDControllerData.monitering_Data.SpeedAvg = speedAvg;
}

void Send_RPM_Total(long long RPMTotal)
{
   	g_OBDControllerData.monitering_Data.uiRPMTotal = RPMTotal;
}
void increase_RPM_Cnt(void)
{
   	g_OBDControllerData.monitering_Data.uiRPMCnt++;

}
void Send_RPM_Avg(unsigned int RPMAvg)
{
   	g_OBDControllerData.monitering_Data.uiRPMAvg = RPMAvg;
}

#if 1 // James Jean 2018/11/18
void Send_MotorRPM_Total(unsigned int MotorRPMTotal)
{
   	g_OBDControllerData.monitering_Data.uiMotorRPMTotal = MotorRPMTotal;

	/* change Max RPM */
	if(Get_MotorMaxRPM() < Get_MotorRPM_Status())
		g_OBDControllerData.monitering_Data.m_usMaxMotorRPM = Get_MotorRPM_Status();

	Decide_OverMotorRPM_Alarm();
	Decide_EngingButton_State();
}

void Send_MotorRPM_Avg(unsigned int MotorRPMAvg)
{
   	g_OBDControllerData.monitering_Data.uiMotorRPMAvg = MotorRPMAvg;
}

void increase_MotorRPM_Cnt(void)
{
    g_OBDControllerData.monitering_Data.uiMotorRPMCnt++;
}
#endif

void Send_GpsStatOnValidation(unsigned int gpsValidation)
{
	g_OBDControllerData.monitering_Data.GpsStartOnValidation = gpsValidation;
}
void Send_GpsStartOnLat(double gpsLat)
{
	g_OBDControllerData.monitering_Data.GpsStartOnLat = gpsLat;
}
void Send_GpsStartOnLon(double gpsLon)
{
   	g_OBDControllerData.monitering_Data.GpsStartOnLon = gpsLon;
}
void Send_GpsDirection(int direction)
{
   	g_OBDControllerData.monitering_Data.iGpsDirection = direction;
}
void Send_EngStopGpsValidation(unsigned int gpsValidation)
{
	g_OBDControllerData.monitering_Data.EngStopGpsValidation = gpsValidation;
}
void Send_EngStopGpsLat(double gpsLat)
{
   	g_OBDControllerData.monitering_Data.EngStopGpsLat = gpsLat;
}
void Send_EngStopGpsLon(double gpsLon)
{
   	g_OBDControllerData.monitering_Data.EngStopGpsLon = gpsLon;
}

void Send_AirconTemperature_From_CAN(unsigned char AirconTemperature)
{
   	g_OBDControllerData.monitering_Data.m_ucAirconTemperature = AirconTemperature;
}
void Send_VentStatus_From_CAN(unsigned char VentStatus)
{
   	g_OBDControllerData.monitering_Data.m_ucVentStatus = VentStatus;
}
void Send_HornStatus_From_CAN(unsigned char HornStatus)
{
	if( (g_eHornCheckState == eHORN_STATE_CHECKING ) && (HornStatus == 1) )
	{
		g_eHornCheckState = eHORN_STATE_HORNON;
	}
   	g_OBDControllerData.monitering_Data.m_ucHornStatus = HornStatus;
}
void Send_UserStatus_From_CAN(unsigned char User)
{
   	g_OBDControllerData.monitering_Data.m_ucUserCtrlStatus = User;
}

void Send_ACC_From_CAN(unsigned int unACC)
{
//	printf("ACC : %d\r\n",ACC);
	g_OBDControllerData.monitering_Data.m_ucACC = unACC;

	Decide_EngingButton_State();
}
void Send_IG1_From_CAN(bool IG1)
{
   	g_OBDControllerData.monitering_Data.m_bIG1 = IG1;
	Decide_EngingButton_State();
}
void Send_IG2_From_CAN(bool IG2)
{
   	g_OBDControllerData.monitering_Data.m_bIG2 = IG2;
	Decide_EngingButton_State();
}
void Send_IG3_From_CAN(bool IG3)
{
   	g_OBDControllerData.monitering_Data.m_bIG3 = IG3;
}
void Send_ISG_From_CAN(bool ISGRun)
{
   	g_OBDControllerData.monitering_Data.m_bISGRun = ISGRun;
	Decide_EngingButton_State();
}
void Send_EngRun_From_CAN(bool EngRun)
{
	g_OBDControllerData.monitering_Data.m_bENGRun = EngRun;
#if defined(QA_FIFA) 
		if(GetPACVType() == CV)
		{
			if(g_iTimerENGRUNCheckCallback != -1) 
			{
				//상용 차량 시동 OFF 시 CAN 데이터 수집 불가, CAN 데이터가 1초동안 들어오지 않을 경우 Engine OFF 체크 
				HalTimerChangeSWTimer(g_iTimerENGRUNCheckCallback, 1000, eSWTimer_ONESHOT, ENGRUN_1SecCheckCallback, true);
			}
		}
#endif
	Decide_EngingButton_State();//mod.pdh 2021.12.29 to devide ISG with B0C
}

void Send_EngStall_From_CAN(bool EngStall)
{
   	g_OBDControllerData.monitering_Data.m_bEngStallState = EngStall;
	Decide_EngingButton_State();
}
void Send_SOCState_From_CAN(float SOCState)
{
   	g_OBDControllerData.monitering_Data.m_fSOCState = SOCState;
   	g_stElectricCarData.m_ucSOC = (unsigned char)SOCState;
}
void Send_SOHState_From_CAN(float SOHState)
{
   	g_OBDControllerData.monitering_Data.m_fSOHState = SOHState;
   	g_stElectricCarData.m_ucSOH = (unsigned char)SOHState;
}

#if defined(PROTOCOL21)
void Send_EngOilLifeRatio_From_CAN(unsigned char EngOilLifeRatio)
{
	g_OBDControllerData.monitering_Data.m_ucEngOilLifeRatio = EngOilLifeRatio;
}

void Send_EngOilLifeEna_From_CAN(unsigned char EngOilLifeEna)
{
	g_OBDControllerData.monitering_Data.m_ucEngOilLifeEna = EngOilLifeEna;
}

void Send_EngOilLifeWarn_From_CAN(unsigned char EngOilLifeWarn)
{
	g_OBDControllerData.monitering_Data.m_ucEngOilLifeWarn = EngOilLifeWarn;
}
#endif

#ifdef PROTOCOL19
void Send_MinBatteryCellVoltage_From_CAN(float BatteryCell)
{
   	g_OBDControllerData.monitering_Data.fFuelcellVoltLow = BatteryCell;
}
void Send_MaxBatteryCellVoltage_From_CAN(float BatteryCell)
{
   	g_OBDControllerData.monitering_Data.fFuelcellVoltHigh = BatteryCell;
}
void Send_HydrogenChargeCnt_From_CAN(unsigned int ChargeCount)
{
	g_OBDControllerData.monitering_Data.uiHydrogenChargeCnt = ChargeCount ;
}
void Send_HydrogenTankPress_From_CAN(float TankPress)
{
	g_OBDControllerData.monitering_Data.fHydrogenTankPress = TankPress;
}
void Send_HydrogenCurrentTemperature_From_CAN(short CurrentTemp)
{
	g_OBDControllerData.monitering_Data.sHydrogenCurrentTemperature = CurrentTemp;
}
void Send_Hydrogen_Fuel_From_CAN(float fHydrogenFuel)
{
   	g_OBDControllerData.monitering_Data.m_fHydrogenFuel = fHydrogenFuel;
}
void Send_AirPurification_From_CAN(float AirPurification)
{
   	g_OBDControllerData.monitering_Data.m_fAirPurification = AirPurification;
}
void Send_CO2Reduction_From_CAN(float CO2)
{
   	g_OBDControllerData.monitering_Data.m_fCO2Reduction = CO2;
}
#endif

void Send_BatteryPackC_From_CAN(float BatteryPack)
{
   	g_OBDControllerData.monitering_Data.m_fBatteryPackC = BatteryPack;
}
void Send_BatteryPackV_From_CAN(float BatteryPack)
{
   	g_OBDControllerData.monitering_Data.m_fBatteryPackV = BatteryPack;
}
void Send_MotorRPM_From_CAN(unsigned int MotorRPM)
{
   	g_OBDControllerData.monitering_Data.m_usMotorRPM = MotorRPM;
}
void Send_ChargeCount_From_CAN(unsigned short usChargeCount)
{
   	g_OBDControllerData.monitering_Data.m_usChargeCount = usChargeCount;
}
void Send_ChargeTime_From_CAN(unsigned int usChargeTime)
{
   	g_OBDControllerData.monitering_Data.m_usChargeTime = usChargeTime;
}

void Send_ABSIndicator_From_CAN(bool bInput)
{
	if(Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN)
	{
		if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME))
		{
	if((g_OBDControllerData.monitering_Data.m_bABSIndicatorState != bInput) && (bInput==true))
	{
				ReportAlramStatus(eMESSAGE_EVENT_KEY_ABSINDICATOR_ALRAM,bInput);
	}
   	g_OBDControllerData.monitering_Data.m_bABSIndicatorState = bInput;
		}
	}
}

void Send_ISGIndicator_From_CAN(bool bInput)
{
	if(Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN)
	{
		if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME))
		{
	if((g_OBDControllerData.monitering_Data.m_bISGIndicatorState != bInput) && (bInput==true))
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_ISGINDICATOR_ALRAM,(uint8_t)Get_SOC_Status());
	}
   	g_OBDControllerData.monitering_Data.m_bISGIndicatorState = bInput;
		}
	}
}

void Send_EPBIndicator_From_CAN(bool bInput)
{
	if(Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN)
	{
		if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME))
		{
	if((g_OBDControllerData.monitering_Data.m_bEPBIndicatorState != bInput) && (bInput==true))
	{
				ReportAlramStatus(eMESSAGE_EVENT_KEY_EPBINDICATOR_ALRAM,bInput);
	}
   	g_OBDControllerData.monitering_Data.m_bEPBIndicatorState = bInput;
		}
	}
}

void Send_BrakeJudder_From_CAN(unsigned int uiInput)
{
	if(Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN)
	{
		if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME))
		{
   	g_OBDControllerData.monitering_Data.m_uiBrakeJudderValue = uiInput;
		
	if( g_stBrakeJudder.bValid == true )
	{
		Decide_BrakeJudder_Alarm(g_OBDControllerData.monitering_Data.m_uiBrakeJudderValue);
	}
		}
	}
}

void Send_BMSIndicator_From_CAN(bool bInput)
{
	if(Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN)
	{
		if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME))
		{	
			if((g_OBDControllerData.monitering_Data.m_bBMSIndicatorState != bInput) && (bInput==true))
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_BMSINDICATOR_ALRAM,bInput);
	}
   	g_OBDControllerData.monitering_Data.m_bBMSIndicatorState = bInput;
		}
	}
}

void Send_ENGOILPRESSIndicator_From_CAN(bool bInput)
{
	if(Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN)
	{
		if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME))
		{
	if((g_OBDControllerData.monitering_Data.m_bENGOilPressIndicatorState != bInput) && (bInput==true))
	{
				ReportAlramStatus(eMESSAGE_EVENT_KEY_ENGOILINDICATOR_ALRAM,bInput);
			}
	}
   	g_OBDControllerData.monitering_Data.m_bENGOilPressIndicatorState = bInput;
	}
}


void Send_ChargeState_From_CAN(bool bChargeState)
{
	if(g_OBDControllerData.monitering_Data.m_bChargeState != bChargeState)
	{
		g_OBDControllerData.monitering_Data.m_bChargeState = bChargeState;
		ReportAlramStatus(eMESSAGE_EVENT_KEY_CHARGE_ALRAM,g_OBDControllerData.monitering_Data.m_bChargeState);
	}
}
void Send_RearSeatSet_From_CAN(bool bRearSeatSet)
{
	g_OBDControllerData.monitering_Data.m_bRearSeatSet = bRearSeatSet;
}
void Send_RearSeatOccured_From_CAN(bool bRearSeatOccured)
{
#ifdef PROTOCOL18  
	if( (Get_RearSeatSet()) && (g_OBDControllerData.monitering_Data.m_bRearSeatOccured != bRearSeatOccured))
	{
		g_OBDControllerData.monitering_Data.m_bRearSeatOccured = bRearSeatOccured;
		ReportAlramStatus(eMESSAGE_EVENT_KEY_REARSEAT_ALRAM,g_OBDControllerData.monitering_Data.m_bRearSeatOccured);
	}
#endif    
}
void Send_FATCState_From_CAN(bool bFATCState)
{
   	g_OBDControllerData.monitering_Data.m_bFATCState = bFATCState;
}
void Send_CrashSignal_From_CAN(bool Crash)
{
	g_OBDControllerData.monitering_Data.m_bCrashSignal = Crash;
	if( (g_bCrashAlramFlag == true) && (Crash == 1) )
	{
		SetEmergencyStatus(true);
		if( g_bEngRunKeepFlag == true )	ReportDirectAlramStatus( eR_Alram, eMESSAGE_EVENT_KEY_AIRBAG_ALRAM, 1 );
		else											ReportAlramStatus( eMESSAGE_EVENT_KEY_AIRBAG_ALRAM , 1);
		g_bCrashAlramFlag = false;
	}
	//	Decide_Crash_Alarm();
}

void Send_4WD_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_uc4WD |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_uc4WD = g_OBDControllerData.AlramData.m_uc4WD & (~ucMasking);
		}
		
		Decide_Indicator_Alarm();
	}
}

void Send_ABS_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucABS |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucABS = g_OBDControllerData.AlramData.m_ucABS & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_Airbag_IND_From_CAN( unsigned char ucInput )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		g_OBDControllerData.AlramData.m_ucAirbag = ucInput;
		Decide_Indicator_Alarm();
	}
}

//void Send_Airbag_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
//{
//	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
//	{
//		if( ucInput != 0 )
//		{
//			g_OBDControllerData.AlramData.m_ucAirbag |= ucMasking;
//		}
//		else
//		{
//			g_OBDControllerData.AlramData.m_ucAirbag = g_OBDControllerData.AlramData.m_ucAirbag & (~ucMasking);
//		}
//		Decide_Indicator_Alarm();
//	}
//}

void Send_AutoHold_IND_From_CAN( unsigned char ucInput )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		g_OBDControllerData.AlramData.m_ucAutoHold = ucInput;
		Decide_Indicator_Alarm();
	}
}

//void Send_AutoHold_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
//{
//	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
//	{
//		if( ucInput != 0 )
//		{
//			g_OBDControllerData.AlramData.m_ucAutoHold |= ucMasking;
//		}
//		else
//		{
//			g_OBDControllerData.AlramData.m_ucAutoHold = g_OBDControllerData.AlramData.m_ucAutoHold & (~ucMasking);
//		}
//		Decide_Indicator_Alarm();
//	}
//}

void Send_BatteryCharge_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucBatteryCharge |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucBatteryCharge = g_OBDControllerData.AlramData.m_ucBatteryCharge & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_CheckEngine_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucCheckEngine |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucCheckEngine = g_OBDControllerData.AlramData.m_ucCheckEngine & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_DBCWarnning_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucDBCWarnning |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucDBCWarnning = g_OBDControllerData.AlramData.m_ucDBCWarnning & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_DPF_IND_From_CAN( unsigned char ucInput )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		g_OBDControllerData.AlramData.m_ucDPF = ucInput;
		Decide_Indicator_Alarm();
	}
}

//void Send_DPF_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
//{
//	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
//	{
//		if( ucInput != 0 )
//		{
//			g_OBDControllerData.AlramData.m_ucDPF |= ucMasking;
//		}
//		else
//		{
//			g_OBDControllerData.AlramData.m_ucDPF = g_OBDControllerData.AlramData.m_ucDPF & (~ucMasking);
//		}
//		Decide_Indicator_Alarm();
//	}
//}

void Send_SCR_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucSCR |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucSCR = g_OBDControllerData.AlramData.m_ucSCR & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_EPB_IND_From_CAN( unsigned char ucInput )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		g_OBDControllerData.AlramData.m_ucEPB = ucInput;
		Decide_Indicator_Alarm();
	}
}

//void Send_EPB_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
//{
//	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
//	{
//		if( ucInput != 0 )
//		{
//			g_OBDControllerData.AlramData.m_ucEPB |= ucMasking;
//		}
//		else
//		{
//			g_OBDControllerData.AlramData.m_ucEPB = g_OBDControllerData.AlramData.m_ucEPB & (~ucMasking);
//		}
//		Decide_Indicator_Alarm();
//	}
//}

void Send_TCS_IND_From_CAN( unsigned char ucInput )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		g_OBDControllerData.AlramData.m_ucTCS = ucInput;
		Decide_Indicator_Alarm();
	}
}

//void Send_TCS_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
//{
//	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
//	{
//		if( ucInput != 0 )
//		{
//			g_OBDControllerData.AlramData.m_ucTCS |= ucMasking;
//		}
//		else
//		{
//			g_OBDControllerData.AlramData.m_ucTCS = g_OBDControllerData.AlramData.m_ucTCS & (~ucMasking);
//		}
//		Decide_Indicator_Alarm();
//	}
//}


void Send_ISG_IND_From_CAN( unsigned char ucInput )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		g_OBDControllerData.AlramData.m_ucISG = ucInput;
		Decide_Indicator_Alarm();
	}
}

//void Send_ISG_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
//{
//	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
//	{
//		if( ucInput != 0 )
//		{
//			g_OBDControllerData.AlramData.m_ucISG |= ucMasking;
//		}
//		else
//		{
//			g_OBDControllerData.AlramData.m_ucISG = g_OBDControllerData.AlramData.m_ucISG & (~ucMasking);
//		}
//		Decide_Indicator_Alarm();
//	}
//}

void Send_MDPS_IND_From_CAN( unsigned char ucInput )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		g_OBDControllerData.AlramData.m_ucMDPS = ucInput;
		Decide_Indicator_Alarm();
	}
}

//void Send_MDPS_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
//{
//	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
//	{
//		if( ucInput != 0 )
//		{
//			g_OBDControllerData.AlramData.m_ucMDPS |= ucMasking;
//		}
//		else
//		{
//			g_OBDControllerData.AlramData.m_ucMDPS = g_OBDControllerData.AlramData.m_ucMDPS & (~ucMasking);
//		}
//		Decide_Indicator_Alarm();
//	}
//}

void Send_OilLevel_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucOilLevel |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucOilLevel = g_OBDControllerData.AlramData.m_ucOilLevel & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_OilPress_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucOilPress |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucOilPress = g_OBDControllerData.AlramData.m_ucOilPress & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_ParkingBrake_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucParkingBrake |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucParkingBrake = g_OBDControllerData.AlramData.m_ucParkingBrake & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_PowerDown_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucPowerDown |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucPowerDown = g_OBDControllerData.AlramData.m_ucPowerDown & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_AHB_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucAHB |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucAHB = g_OBDControllerData.AlramData.m_ucAHB & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_HEV_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucHEV |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucHEV = g_OBDControllerData.AlramData.m_ucHEV & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_EV_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucEV |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucEV = g_OBDControllerData.AlramData.m_ucEV & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_FCEV_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		if( ucInput != 0 )
		{
			g_OBDControllerData.AlramData.m_ucFCEV |= ucMasking;
		}
		else
		{
			g_OBDControllerData.AlramData.m_ucFCEV = g_OBDControllerData.AlramData.m_ucFCEV & (~ucMasking);
		}
		Decide_Indicator_Alarm();
	}
}

void Send_TPMS_IND_From_CAN( unsigned char ucInput )
{
	if(g_bIndicatorFirstCheckFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
	{
		g_OBDControllerData.AlramData.m_ucTPMS = ucInput;
		Decide_Indicator_Alarm();
	}
}

//void Send_TPMS_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking )
//{
//	if(g_bFCSRunFlag == false)	//첫시동시 경고등이 모두 들어왔다가 꺼지므로 FCS가 끝난후부터 체크
//	{
//		if( ucInput != 0 )
//		{
//			g_OBDControllerData.AlramData.m_ucTPMS |= ucMasking;
//		}
//		else
//		{
//			g_OBDControllerData.AlramData.m_ucTPMS = g_OBDControllerData.AlramData.m_ucTPMS & (~ucMasking);
//		}
//		Decide_Indicator_Alarm();
//	}
//}

void Send_LowBatterySOC_From_CAN(unsigned char BatterySOC)
{
	g_OBDControllerData.monitering_Data.m_ucLowBatterySOC = BatterySOC;
}
void Send_InsulationResistance_From_CAN(unsigned short usInsulationResistance)
{
 	g_OBDControllerData.monitering_Data.m_usInsultationResistance = usInsulationResistance;
}
void Set_DriveStopTime(void)
{	
    stHalRTCTypeDef stTimeVar;

    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stTimeVar, sizeof(stHalRTCTypeDef), 0);
#if defined TIME_CHECK	
    uint8_t arrTmp[64];
	sprintf((char *)arrTmp, "%04d%02d%02d%0.2d%0.2d%0.2d\x00",
				2000 + g_OBDControllerData.basicData.DriveStopDate_Local.RTC_Year,
				g_OBDControllerData.basicData.DriveStopDate_Local.RTC_Month,
				g_OBDControllerData.basicData.DriveStopDate_Local.RTC_Date,
				g_OBDControllerData.basicData.DriveStopTime_Local.RTC_Hours,
				g_OBDControllerData.basicData.DriveStopTime_Local.RTC_Minutes,
				g_OBDControllerData.basicData.DriveStopTime_Local.RTC_Seconds);

	// 운행 키를 저장해 둔다.
//	GetCurTimefromDate(g_OBDControllerData.basicData.DriveStartTime_Local,g_OBDControllerData.basicData.DriveStartDate_Local);
	printf("MSG: EndTime: [%s]\n", arrTmp);
#endif
}

void Set_DriveStartTime(void)
{
	g_OBDControllerData.basicData.DriveStartTime_LocalTime = GetLocalTimefromTime(GetUTCTime());
	g_OBDControllerData.basicData.DriveStartTime_UTC = GetUTCTime();
	g_OBDControllerData.basicData.snDriveStartTimeZone = BkSram_ModemInfo.snTimeZone;
	printf("Set_DriveStartTime UTC %d, Local%d\r\n",g_OBDControllerData.basicData.DriveStartTime_UTC,	g_OBDControllerData.basicData.DriveStartTime_LocalTime);
}

void Clear_Drivingkey(void)
{
	g_OBDControllerData.basicData.drivingKey = 0;
}

BOOL Discrimination_Stable_Slope_TCU()
{
	static unsigned char s_ucOldGearPos = -1;	
	BOOL bResult=false;
					
	if(Get_FootBrake_State() == 0 && g_ucGearPos == s_ucOldGearPos && g_ucGearPos != 1 && Get_GearPosition() == 'D' && Get_Speed() >= 40)
		bResult = true;

	s_ucOldGearPos = g_ucGearPos;

	return bResult;
}

void SetGyroAngle(stSensorInfo* pstGyroAngle)
{
	int i;
	
	int nSumAngleX = 0, nSumAngleY = 0;
	int nMaxIndexX = 0, nMaxIndexY = 0, nMinIndexX = 0, nMinIndexY = 0;
	
	g_nTCUSlopeAngleArr[g_ucTCUSlopeAngleArrIndex][X_AXIS] = pstGyroAngle->nX;
	g_nTCUSlopeAngleArr[g_ucTCUSlopeAngleArrIndex][Y_AXIS] = pstGyroAngle->nY;
	g_ucTCUSlopeAngleArrIndex++;

	for(i=0; i<g_ucTCUSlopeAngleArrIndex; i++)
	{
		nSumAngleX += g_nTCUSlopeAngleArr[i][X_AXIS];
		nSumAngleY += g_nTCUSlopeAngleArr[i][Y_AXIS];

		if(g_nTCUSlopeAngleArr[i][X_AXIS] >= g_nTCUSlopeAngleArr[nMaxIndexX][X_AXIS])	nMaxIndexX = i;
		if(g_nTCUSlopeAngleArr[i][X_AXIS] <= g_nTCUSlopeAngleArr[nMinIndexX][X_AXIS])	nMinIndexX = i;
		
		if(g_nTCUSlopeAngleArr[i][Y_AXIS] >= g_nTCUSlopeAngleArr[nMaxIndexY][Y_AXIS])	nMaxIndexY = i;
		if(g_nTCUSlopeAngleArr[i][Y_AXIS] <= g_nTCUSlopeAngleArr[nMinIndexY][Y_AXIS])	nMinIndexY = i;
	}

	if( g_ucTCUSlopeAngleArrIndex < 3)
	{
		pstGyroAngle->nX = nSumAngleX/(g_ucTCUSlopeAngleArrIndex);
		pstGyroAngle->nY = nSumAngleY/(g_ucTCUSlopeAngleArrIndex);
	}
	else 
	{
		nSumAngleX = nSumAngleX - g_nTCUSlopeAngleArr[nMaxIndexX][X_AXIS] - g_nTCUSlopeAngleArr[nMinIndexX][X_AXIS];
		nSumAngleY = nSumAngleY - g_nTCUSlopeAngleArr[nMaxIndexY][Y_AXIS] - g_nTCUSlopeAngleArr[nMinIndexY][Y_AXIS];
		
		pstGyroAngle->nX = nSumAngleX/(g_ucTCUSlopeAngleArrIndex-2);
		pstGyroAngle->nY = nSumAngleY/(g_ucTCUSlopeAngleArrIndex-2);
	}
}

void Send_Celsius_From_CAN(bool Celsius)
{
	g_OBDControllerData.monitering_Data.m_Celsius= Celsius;
}

void Send_Fahrenheit_From_CAN(bool Fahrenheit)
{
	g_OBDControllerData.monitering_Data.m_Fahrenheit= Fahrenheit;
	CheckFahrenheit();
}

void Set_Fahrenheit_UpdatedFlag(bool bFlag)
{
	g_Fahrenheit_UpdatedFlag = bFlag;
}

//210608 미사용
/*
void Send_FuelWarningLamp_From_CAN(bool bFlag)
{
	g_OBDControllerData.monitering_Data.m_bFuelWarningLamp = bFlag;
	Decide_FuelWarningLamp_Alarm();
}

void Decide_FuelWarningLamp_Alarm(void)
{
	static bool s_bAlramOccurredFlag=false;
	if( (g_OBDControllerData.monitering_Data.m_bFuelWarningLamp==true) && (s_bAlramOccurredFlag==false) && (Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN) && (Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME) )
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_FUEL_RUN_OUT , g_OBDControllerData.monitering_Data.m_bFuelWarningLamp);
		printf("FuelWarningLamp Lamp ALARM  %d \r\n" ,g_OBDControllerData.monitering_Data.m_bFuelWarningLamp );
		s_bAlramOccurredFlag = true;
	}
	else
	{
		s_bAlramOccurredFlag = false;
	}
}
*/

void Decide_SecurityAlarm()
{
	static boolean_t s_bPreHornStatusActive = false;
	static uint32_t s_unActiveCount = 0;
	static eForceAlertState  s_eForceAlertState = eForceAlert_Init;
	static unsigned long s_ulWaitTimeout = 0;
	
	if( (g_OBDControllerData.monitering_Data.m_ucHornStatus == true) && (Get_VehicleStatus() == eVEHICLE_STATE_OFF) && (GetRemoteControlStatus() == false) )
	{
		if(s_bPreHornStatusActive == false)
		{
			s_bPreHornStatusActive = true;
			s_eForceAlertState = eForceAlert_Init;	
		}
	}

	if( s_bPreHornStatusActive == true )
	{
		switch(s_eForceAlertState)
		{
			case eForceAlert_Init:
				ReportAlramStatus(eMESSAGE_EVENT_KEY_SECURITY_ALRAM,true);
				s_eForceAlertState = eForceAlert_Check;

				s_ulWaitTimeout = Get_Tmr();

				break;
			case eForceAlert_Check:
				if(g_OBDControllerData.monitering_Data.m_ucHornStatus == true )
					s_unActiveCount++;

				if( ( Get_Tmr() - s_ulWaitTimeout ) > 60 * ONE_SECOND )
				{
					if( s_unActiveCount == 0 )
					{
						s_eForceAlertState = eForceAlert_Stop;
					}

					s_ulWaitTimeout = Get_Tmr();
					s_unActiveCount = 0;
				}

				break;
			case eForceAlert_Stop:
				s_bPreHornStatusActive = false;
				break;

		}
	}
}

#if defined(QA_FIFA)
void RPM_1SecCheckCallback() //상용 차량 시동 OFF 시 CAN 데이터 수집 불가, CAN 데이터가 1초동안 들어오지 않을 경우 Engine OFF 체크 
{
	if(GetOBDState() != eOBD_Running_Info_Mode)
	{
		if(g_iTimerRPMCheckCallback != -1)
		{
			HalTimerChangeSWTimer(g_iTimerRPMCheckCallback, 1000, eSWTimer_ONESHOT, RPM_1SecCheckCallback, true);
		}
	}
	else
	{
		if(g_iTimerRPMCheckCallback != -1) 	 
		{
			HalTimerClearSWTimer(g_iTimerRPMCheckCallback);
			g_iTimerRPMCheckCallback=-1;		
		}
		
		Send2MngObd(eMngObd, eOBDEngineStatus, eEngineStop,(stCarReport *)NULL,0);
	}
}

void ENGRUN_1SecCheckCallback() //상용 차량 시동 OFF 시 CAN 데이터 수집 불가, CAN 데이터가 1초동안 들어오지 않을 경우 Engine OFF 체크 
{
	if(GetOBDState() != eOBD_Running_Info_Mode)
	{
		if(g_iTimerENGRUNCheckCallback != -1)
		{
			HalTimerChangeSWTimer(g_iTimerENGRUNCheckCallback, 1000, eSWTimer_ONESHOT, ENGRUN_1SecCheckCallback, true);
		}
	}
	else
	{
		if(g_iTimerENGRUNCheckCallback != -1) 	 
		{
			HalTimerClearSWTimer(g_iTimerENGRUNCheckCallback);
			g_iTimerENGRUNCheckCallback=-1;		
		}
		
    Send2MngObd(eMngObd, eOBDEngineStatus, eEngineStop,(stCarReport *)NULL,0);
	}
}

#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
