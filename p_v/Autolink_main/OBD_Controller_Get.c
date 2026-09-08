#include "OBD_Controller_Get.h"
#include "AutolinkConfiguration.h"
#include "OBD_Manager.h"
#include "MngSystemUtil.h"
#include "GIT_BluetoothLowEnergy.h"

extern uint32_t GetUTCTime();

extern OBD_CONTROLLER_DATA g_OBDControllerData;
extern unsigned long Oem_GetBattVoltage(char adcx);
extern stDieselFuelConsume g_stDieselFuelConsume;
extern bool g_bFuelLiterTypeFlag;
extern unsigned char g_ucFuelLevelMaxLiter;
extern double g_dChecked_Fuel_Consume;
extern float g_fFuelLevelPercent;
extern bool g_bFuelLevelSwitchedPercentFlag;
extern float g_fFuelFcevMax;
extern bool g_Fahrenheit_UpdatedFlag;

#if 1 // James Jean 2018/11/18
extern eFUEL_TYPE	g_eFuel_Type;
#endif

uint32_t Get_DriveStartTime_LTC(void)
{
	return g_OBDControllerData.basicData.DriveStartTime_LocalTime;
}

uint32_t Get_DriveStopTime_LTC(void)
{
	return GetLocalTimefromTime(GetUTCTime());
}

uint32_t Get_DriveStartTime_UTC(void)
{
	return g_OBDControllerData.basicData.DriveStartTime_UTC;
}

uint32_t Get_DriveStopTime_UTC(void)
{
	return GetUTCTime();
}

unsigned int Get_EngAirconTemperature(void)
{
	return g_OBDControllerData.monitering_Data.m_ucAirconTemperature;
}
unsigned int Get_EngVentStatus(void)
{
	return g_OBDControllerData.monitering_Data.m_ucVentStatus;
}
unsigned int Get_EngHornStatus(void)
{
	return g_OBDControllerData.monitering_Data.m_ucHornStatus;
}
bool Get_EngHazardLampStatus(void)
{
	return g_OBDControllerData.monitering_Data.m_bHazardLamp;
}
bool Get_RearDefogStatus(void)
{
	return g_OBDControllerData.monitering_Data.m_bRearDefog;
}
bool Get_HighBeamStatus(void)
{
	return g_OBDControllerData.monitering_Data.m_bHighBeam;
}
bool Get_IG1_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_bIG1;
}
bool Get_IG2_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_bIG2;
}
bool Get_IG3_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_bIG3;
}
bool Get_ISG_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_bISGRun;
}
bool Get_EngRun_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_bENGRun; //mod.pdh 2021.12.29 to devide ISG with B0C
}
bool Get_EngStall_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_bEngStallState;
}
float Get_SOC_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_fSOCState;
}
float Get_SOH_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_fSOHState;
}

float Get_MinBatteryCellVoltage_Status()
{
	return g_OBDControllerData.monitering_Data.fFuelcellVoltLow;
}
float Get_MaxBatteryCellVoltage_Status()
{
	return g_OBDControllerData.monitering_Data.fFuelcellVoltHigh;
}
unsigned int Get_HydrogenChargeCnt_Status()
{
	return g_OBDControllerData.monitering_Data.uiHydrogenChargeCnt;
}
float Get_HydrogenTankPress_Status()
{
	return g_OBDControllerData.monitering_Data.fHydrogenTankPress;
}
short Get_HydrogenCurrentTemperature_Status()
{
	return g_OBDControllerData.monitering_Data.sHydrogenCurrentTemperature;
}
float Get_Hydrogen_Fuel_Status()
{
   	return g_OBDControllerData.monitering_Data.m_fHydrogenFuel;
}
float Get_AirPurification_Status()
{
   	return g_OBDControllerData.monitering_Data.m_fAirPurification;
}
float Get_CO2Reduction_Status()
{
   	return g_OBDControllerData.monitering_Data.m_fCO2Reduction;
}

#ifdef PROTOCOL21
unsigned char Get_EngOilLifeRatio_Status()
{
	return g_OBDControllerData.monitering_Data.m_ucEngOilLifeRatio;
}

unsigned char Get_EngOilLifeEna_Status()
{
	return g_OBDControllerData.monitering_Data.m_ucEngOilLifeEna;
}

unsigned char Get_EngOilLifeWarn_Status()
{
	return g_OBDControllerData.monitering_Data.m_ucEngOilLifeWarn;
}
#endif

float Get_BatteryPackC_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_fBatteryPackC;
}
float Get_BatteryPackV_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_fBatteryPackV;
}
unsigned short Get_MotorRPM_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_usMotorRPM;
}
unsigned int Get_GpsStatOnValidation(void)
{
     	return g_OBDControllerData.monitering_Data.GpsStartOnValidation;
}

double Get_GpsStartOnLat(void)
{
      	return g_OBDControllerData.monitering_Data.GpsStartOnLat ;
}

double Get_GpsStartOnLon(void)
{
      	return g_OBDControllerData.monitering_Data.GpsStartOnLon ;
}


long long Get_Speed_Total(void)
{
       return g_OBDControllerData.monitering_Data.SpeedTotal;

}
unsigned int Get_Speed_Cnt(void)
{
       return g_OBDControllerData.monitering_Data.SpeedCnt;
}
float Get_Speed_Avg(void)
{
       return g_OBDControllerData.monitering_Data.SpeedAvg;
}

long long Get_RPM_Total(void)
{
       return g_OBDControllerData.monitering_Data.uiRPMTotal;

}
unsigned int Get_RPM_Cnt(void)
{
       return g_OBDControllerData.monitering_Data.uiRPMCnt;
}
unsigned int Get_RPM_Avg(void)
{
       return g_OBDControllerData.monitering_Data.uiRPMAvg;
}

#if 1 // James Jean 2018/11/18
unsigned int Get_MotorRPM_Total(void)
{
    return g_OBDControllerData.monitering_Data.uiMotorRPMTotal;
}

unsigned int Get_MotorRPM_Cnt(void)
{
    return g_OBDControllerData.monitering_Data.uiMotorRPMCnt;
}

unsigned int Get_MotorRPM_Avg(void)
{
    return g_OBDControllerData.monitering_Data.uiMotorRPMAvg;
}
unsigned short Get_MotorMaxRPM(void)
{
    return g_OBDControllerData.monitering_Data.m_usMaxMotorRPM;
}
#endif

unsigned int Get_WarmupTime(void)
{
       return g_OBDControllerData.monitering_Data.WarmupTime;

}
unsigned int Get_EngineIdleTime(void)
{
	return g_OBDControllerData.monitering_Data.EngineIdleTime;
}
unsigned int Get_PeriodDrivePattern0(void)
{
	return g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern0;
}
unsigned int Get_PeriodDrivePattern1(void)
{
	return g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern1;
}
unsigned int Get_PeriodDrivePattern2(void)
{
	return g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern2;
}
unsigned int Get_PeriodDrivePattern3(void)
{
	return g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern3;
}
unsigned int Get_PeriodDrivePattern4(void)
{
	return g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern4;
}
#if defined(PROTOCOL17)
unsigned int Get_PeriodDrivePattern5(void)
{
	return g_OBDControllerData.monitering_Data.m_uiPeriodDrivePattern5;
}
#endif
unsigned int Get_TotalDrivePattern0(void)
{
	return g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern0;
}
unsigned int Get_TotalDrivePattern1(void)
{
	return g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern1;
}
unsigned int Get_TotalDrivePattern2(void)
{
	return g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern2;
}
unsigned int Get_TotalDrivePattern3(void)
{
	return g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern3;
}
unsigned int Get_TotalDrivePattern4(void)
{
	return g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern4;
}
#if defined(PROTOCOL17)
unsigned int Get_TotalDrivePattern5(void)
{
	return g_OBDControllerData.monitering_Data.m_uiTotalDrivePattern5;
}
#endif
unsigned char Get_FuelCut(void)
{
        return g_OBDControllerData.monitering_Data.FuelCut;
}

float Get_IntakeAirTemperature(void)
{
  	return g_OBDControllerData.monitering_Data.m_fIntakeAirTemperature;
}
char Get_OutsideTemperature(void)
{
	return g_OBDControllerData.monitering_Data.m_cOutsideTemperature;
}
short Get_HighBatteryTemperature(void)
{
	return g_OBDControllerData.monitering_Data.m_sHighBatteryTemperature;
}

#if defined(PROTOCOL18)
short Get_HighBatteryTemperatureMax(void) //dahae
{
	return g_OBDControllerData.monitering_Data.m_sHighBatteryTemperatureMax;
}

char Get_ChargingState(void)
{
	return g_OBDControllerData.monitering_Data.m_bChargingState;
}

unsigned short Get_ExtraChargeTime(void)
{
	return g_OBDControllerData.monitering_Data.m_usExtraChargeTime;
}
#endif

float Get_MassAirPressure(void)
{
 	return g_OBDControllerData.monitering_Data.m_fMassAirPressure;
}
float Get_Mass_Air_Flow(void)
{
	return g_OBDControllerData.monitering_Data.m_fMassAirFlow;
}
float Get_InjectionQuantity(void)
{
	return g_stDieselFuelConsume.m_fInjectionQuantity;
}
float Get_MI(void)
{
	return g_stDieselFuelConsume.m_fMI;
}
float Get_PIL1(void)
{
	return g_stDieselFuelConsume.m_fPIL1;
}
float Get_PIL2(void)
{
	return g_stDieselFuelConsume.m_fPIL2;
}
float Get_PIL3(void)
{
	return g_stDieselFuelConsume.m_fPIL3;
}
float Get_POL1(void)
{
	return g_stDieselFuelConsume.m_fPOL1;
}
float Get_POL2(void)
{
	return g_stDieselFuelConsume.m_fPOL2;
}

/* OBD Controller  ==>  others Module */
void Get_VIN(unsigned char *vin)
{
	//return g_OBDControllerData.basicData.vin;
}

unsigned short Get_periodMessage_counter(void)
{
	return g_OBDControllerData.basicData.periodMessage_counter;
}
void Up_periodMessage_counter(void)
{
	g_OBDControllerData.basicData.periodMessage_counter++;
}

long long Get_DrivingKey(void)
{
	return g_OBDControllerData.basicData.drivingKey;
}
/* time is form engine starting to engine stop */
unsigned int Get_DrivingTime(void)
{

	return g_OBDControllerData.basicData.drivingTime;
}

bool Get_Decide_Starting_Engine(void)
{
	return g_OBDControllerData.basicData.decide_Starting_Engine;
}

unsigned short Get_Speed(void)
{
	return g_OBDControllerData.monitering_Data.m_usCurrentSpeed;
}
unsigned short Get_MaxSpeed(void)
{
	return g_OBDControllerData.monitering_Data.m_usMaxSpeed;
}

unsigned char Get_BT_Connection_State(void)	/* BT Connect State */
{
  if(BTGetConnectStatus()==TRUE)
  	return BT_STATE_CONNECTION_ON;
  else
  	return BT_STATE_CONNECTION_OFF;
}

unsigned int Get_Period_Speed_Counter_0(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_0;
}
unsigned int Get_Period_Speed_Counter_from_1_under_10(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_1_under_10;
}
unsigned int Get_Period_Speed_Counter_from_11_under_20(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_11_under_20;
}
unsigned int Get_Period_Speed_Counter_from_21_under_30(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_21_under_30;
}
unsigned int Get_Period_Speed_Counter_from_31_under_40(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_31_under_40;
}
unsigned int Get_Period_Speed_Counter_from_41_under_50(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_41_under_50;
}
unsigned int Get_Period_Speed_Counter_from_51_under_60(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_51_under_60;
}
unsigned int Get_Period_Speed_Counter_from_61_under_70(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_61_under_70;
}
unsigned int Get_Period_Speed_Counter_from_71_under_80(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_71_under_80;
}
unsigned int Get_Period_Speed_Counter_from_81_under_90(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_81_under_90;
}
unsigned int Get_Period_Speed_Counter_from_91_under_100(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_91_under_100;
}
unsigned int Get_Period_Speed_Counter_from_101_under_110(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_101_under_110;
}
unsigned int Get_Period_Speed_Counter_from_111_under_120(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_111_under_120;
}
unsigned int Get_Period_Speed_Counter_from_121_under_130(void)	/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_121_under_130;
}
unsigned int Get_Period_Speed_Counter_from_131_under_140(void)
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_131_under_140;
}
unsigned int Get_Period_Speed_Counter_over_141(void)  /* include 141km */
{
	return g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_over_141;
}

unsigned int Get_Total_Speed_Counter_0(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_0;
}
unsigned int Get_Total_Speed_Counter_from_1_under_10(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_1_under_10;
}
unsigned int Get_Total_Speed_Counter_from_11_under_20(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_11_under_20;
}
unsigned int Get_Total_Speed_Counter_from_21_under_30(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_21_under_30;
}
unsigned int Get_Total_Speed_Counter_from_31_under_40(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_31_under_40;
}
unsigned int Get_Total_Speed_Counter_from_41_under_50(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_41_under_50;
}
unsigned int Get_Total_Speed_Counter_from_51_under_60(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_51_under_60;
}
unsigned int Get_Total_Speed_Counter_from_61_under_70(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_61_under_70;
}
unsigned int Get_Total_Speed_Counter_from_71_under_80(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_71_under_80;
}
unsigned int Get_Total_Speed_Counter_from_81_under_90(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_81_under_90;
}
unsigned int Get_Total_Speed_Counter_from_91_under_100(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_91_under_100;
}
unsigned int Get_Total_Speed_Counter_from_101_under_110(void)		/* use 3 byte*/
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_101_under_110;
}
unsigned int Get_Total_Speed_Counter_from_111_under_120(void)
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_111_under_120;
}
unsigned int Get_Total_Speed_Counter_from_121_under_130(void)
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_121_under_130;
}
unsigned int Get_Total_Speed_Counter_from_131_under_140(void)
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_from_131_under_140;
}
unsigned int Get_Total_Speed_Counter_over_141(void)		/* include 141km */
{
	return g_OBDControllerData.monitering_Data.m_uiTotal_Speed_Counter_over_141;
}

unsigned short Get_period_Rapid_acceleration_Counter(void)
{
	return g_OBDControllerData.monitering_Data.period_Rapid_acceleration_Counter;

}
unsigned short Get_period_Rapid_Deceleration_Counter(void)
{
	return g_OBDControllerData.monitering_Data.period_Rapid_Deceleration_Counter;
}
unsigned short Get_total_Rapid_acceleration_Counter(void)
{
	return g_OBDControllerData.monitering_Data.total_Rapid_acceleration_Counter;

}
unsigned short Get_total_Rapid_Deceleration_Counter(void)
{
	return g_OBDControllerData.monitering_Data.total_Rapid_Deceleration_Counter;
}
unsigned short Get_RPM(void)
{
	return g_OBDControllerData.monitering_Data.m_usRPM;
}

unsigned short Get_MaxRPM(void)
{
	return g_OBDControllerData.monitering_Data.m_usMaxRPM;
}

uint16_t Get_TPMS1(eTPMS_TIRE_STATE eSelTire)
{
	uint16_t nTmp;

	// 타이어 공기압
	// 전좌/전우/후좌/후우, 소수점을 없애기 위해서 10을 곱해준다.
	// 23.5 일 경우, 235 --> 2byte
	switch(eSelTire) {
		case eTPMS_TIRE_STATE_FL:
			nTmp = (unsigned short int)g_OBDControllerData.currentState.TPMS_FL*10;
			break;
		case eTPMS_TIRE_STATE_FR:
			nTmp = (unsigned short int)g_OBDControllerData.currentState.TPMS_FR*10;
			break;
		case eTPMS_TIRE_STATE_RL:
			nTmp = (unsigned short int)g_OBDControllerData.currentState.TPMS_RL*10;
			break;
		case eTPMS_TIRE_STATE_RR:
			nTmp = (unsigned short int)g_OBDControllerData.currentState.TPMS_RR*10;
			break;
		default:
			nTmp = 0;
			break;
	}

	return nTmp;
}

eVEHICLE_STATE Get_VehicleStatus(void)
{
	return g_OBDControllerData.monitering_Data.m_ucVehicleStatus;
}

float Get_Battery(void)
{
	if( Get_VehicleStatus() == eVEHICLE_STATE_OFF || 
	    (GetRCVBatteryFromCAN() == true && g_OBDControllerData.monitering_Data.m_fBattery == 0.0) )
		return Oem_GetBattVoltage(2)/100.0;
	else
		return g_OBDControllerData.monitering_Data.m_fBattery;
}

unsigned char Get_DoorLock(void)
{
	return g_OBDControllerData.currentState.doorLock;
}

unsigned char Get_DoorOpen(void)
{
	return g_OBDControllerData.currentState.doorOpen;
}

unsigned char Get_ACC(void)
{
	return g_OBDControllerData.monitering_Data.m_ucACC;
}

unsigned char Get_UserStatus(void)
{
	return g_OBDControllerData.monitering_Data.m_ucUserCtrlStatus;
}


float Get_driving_distance(void)
{
	return g_OBDControllerData.monitering_Data.m_fDriving_Distance;
}
unsigned short Get_ExtraDrivingDistance_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_usExtraDrivingDistance;
}
unsigned int Get_driving_distance_int(void)
{
	return (unsigned int)g_OBDControllerData.monitering_Data.m_fDriving_Distance;
}

unsigned int Get_Odmeter(void)
{
	return g_OBDControllerData.monitering_Data.m_uiOdometer;	//단위 km
}

unsigned int Get_StartOdmeter(void)
{
	return g_OBDControllerData.basicData.m_uiStartOdometer;	//단위 km
}
//uint16_t Get_Remain_Fuel_Percent2(void)
//{
//	return g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter*10;
//}
unsigned char Get_Remain_Fuel_Percent(void)
{
	float fTemp = 0.0;
	if(g_bFuelLiterTypeFlag == true && g_bFuelLevelSwitchedPercentFlag == false )
	{	//단위 L
		fTemp = g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter - (g_dChecked_Fuel_Consume);
		fTemp = fTemp * ((float)(100.0/g_ucFuelLevelMaxLiter));	// %로 변환
//		printf("Get_Remain_Fuel_Percent m_fFuel_Level_Liter : %f g_dChecked_Fuel_Consume: %lf fTemp:%f%\r\n",g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter,g_dChecked_Fuel_Consume,fTemp);
		if( fTemp <= 0.0 ) fTemp = 0;
		g_fFuelLevelPercent = fTemp;	//현재 연료잔량 갱신
//		printf("Get_Remain_Fuel_Percent2 : %d %f\r\n",(unsigned char)fTemp,fTemp);
		return (unsigned char)fTemp;
//		return (unsigned char)g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter;
	}
	else
	{
		if( g_bFuelLevelSwitchedPercentFlag == true )
		{
			if( g_fFuelFcevMax == 0 )
				g_fFuelFcevMax = 6.33;

			// add driving distance fuel
			float ftotalkg = g_fFuelFcevMax;
			double dlpercent = (g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume/ftotalkg)*100.0;
			g_fFuelLevelPercent = g_OBDControllerData.monitering_Data.m_ucFuel_Level + dlpercent;

			//printf("FCEV Current Consume : %f\n",dlpercent);
			return (unsigned char)g_fFuelLevelPercent;
		}

		return g_OBDControllerData.monitering_Data.m_ucFuel_Level;
	}
}

float Get_Slope_TCU_From_CAN()
{
	return g_OBDControllerData.monitering_Data.m_fSlope_TCU;
}

double Get_Fuel_Consume() /* mL */
{
	return g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume;
}
double Get_Energy_Consume()
{
	return g_OBDControllerData.monitering_Data.m_fPeriod_Energy_Consume;
}
#if defined(PROTOCOL17)
double Get_Energy_Consume_Regen()
{
	return g_OBDControllerData.monitering_Data.m_fPeriod_Energy_Consume_Regen;
}
double Get_Total_Energy_Consume_Regen()
{
	return g_OBDControllerData.monitering_Data.m_fTotal_Energy_Consume_Regen;
}
#endif
unsigned char Get_AccelPos(void)
{
	return g_OBDControllerData.monitering_Data.m_ucAccelPos;
}

double Get_Total_Fuel_Consume() /* mL */
{
	return g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume;
}

double Get_Total_Energy_Consume()
{
	return g_OBDControllerData.monitering_Data.m_fTotal_Energy_Consume;
}

short Get_CoolantTemperature(void)
{
	return g_OBDControllerData.monitering_Data.m_sCoolantTemperature;
}

unsigned char Get_GearPosition(void)
{
	return g_OBDControllerData.monitering_Data.m_ucGearPosition;
}

unsigned char Get_GearLever(void)
{
	return g_OBDControllerData.monitering_Data.m_ucGearLever;
}

unsigned char Get_GearSbw(void)
{
	return g_OBDControllerData.monitering_Data.m_ucGearSbw;
}

unsigned short Get_TransmissionOil_Temperature(void)
{
	return g_OBDControllerData.monitering_Data.m_usTransmissionOil_Temperature;
}

bool Get_TailLamp_State(void)
{
	return g_OBDControllerData.monitering_Data.m_bTailLamp;
}

bool Get_FootBrake_State(void)
{
	return g_OBDControllerData.monitering_Data.m_bFootBrake;
}

bool Get_HeadLamp_State(void)
{
	return g_OBDControllerData.monitering_Data.m_bHeadLamp;
}

bool Get_MILLamp_State(void)
{
	return g_OBDControllerData.monitering_Data.m_bMILLamp;
}

bool Get_HighBeam_State(void)
{
	return g_OBDControllerData.monitering_Data.m_bHighBeam;
}

float Get_MileAge_avg(void)
{
 	return g_OBDControllerData.monitering_Data.MileageAvg;
}
float Get_ElectronicMileAge_avg(void)
{
 	return g_OBDControllerData.monitering_Data.m_fElectronicMileageAvg;
}

float Get_TMPS_Conv(void)
{
 	return g_OBDControllerData.currentState.fTPMS_Conv;
}

unsigned short Get_ChargeCount(void)
{
	return g_OBDControllerData.monitering_Data.m_usChargeCount;
}

unsigned int Get_ChargeTime(void)
{
	return g_OBDControllerData.monitering_Data.m_usChargeTime;
}

bool Get_RearSeatSet(void)
{
	return g_OBDControllerData.monitering_Data.m_bRearSeatSet;
}

bool Get_RearSeatOccured(void)
{
	return g_OBDControllerData.monitering_Data.m_bRearSeatOccured;
}
bool Get_ChargeState(void)
{
	return g_OBDControllerData.monitering_Data.m_bChargeState;
}
bool Get_FATCState(void)
{
	return g_OBDControllerData.monitering_Data.m_bFATCState;
}
int Get_GpsDirection(void)
{
	return g_OBDControllerData.monitering_Data.iGpsDirection;
}
bool Get_Fahrenheit(void)
{
	return g_OBDControllerData.monitering_Data.m_Fahrenheit;
}
bool Get_Fahrenheit_UpdatedFlag(void)
{
	return g_Fahrenheit_UpdatedFlag;
}
bool Get_Celsius(void)
{
	return g_OBDControllerData.monitering_Data.m_Celsius;
}

unsigned short Get_InsulationResistance(void)
{
	return g_OBDControllerData.monitering_Data.m_usInsultationResistance;
}

unsigned char Get_LowBatterySOC_Status(void)
{
	return g_OBDControllerData.monitering_Data.m_ucLowBatterySOC;
}