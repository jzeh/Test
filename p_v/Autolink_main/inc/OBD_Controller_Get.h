#ifndef __OBD_CONTROLLER_GET_H__
#define __OBD_CONTROLLER_GET_H__


#include "common.h"
#include "AutolinkConfig.h"
#include "OBD_Controller.h"


unsigned int Get_GpsStatOnValidation(void);
double Get_GpsStartOnLat(void);
double Get_GpsStartOnLon(void);

uint32_t Get_DriveStartTime_LTC(void);
uint32_t Get_DriveStopTime_LTC(void);
uint32_t Get_DriveStartTime_UTC(void);
uint32_t Get_DriveStopTime_UTC(void);

unsigned int Get_EngAirconTemperature(void);
unsigned int Get_EngVentStatus(void);
unsigned int Get_EngHornStatus(void);
bool Get_EngHazardLampStatus(void);
bool Get_RearDefogStatus(void);
bool Get_HighBeamStatus(void);
bool Get_IG1_Status(void);
bool Get_IG2_Status(void);
bool Get_IG3_Status(void);
bool Get_ISG_Status(void);
bool Get_EngRun_Status(void);
bool Get_EngStall_Status(void);
char Get_ChargingState(void);
bool Get_Fahrenheit(void);
float Get_MinBatteryCellVoltage_Status(void);
float Get_MaxBatteryCellVoltage_Status(void);
unsigned int Get_HydrogenChargeCnt_Status(void);
float Get_HydrogenTankPress_Status(void);
short Get_HydrogenCurrentTemperature_Status(void);
float Get_Hydrogen_Fuel_Status(void);
float Get_AirPurification_Status(void);
float Get_CO2Reduction_Status(void);

#if defined(PROTOCOL21)
unsigned char Get_EngOilLifeRatio_Status(void);
unsigned char Get_EngOilLifeEna_Status(void);
unsigned char Get_EngOilLifeWarn_Status(void);
#endif

float Get_SOC_Status(void);
float Get_SOH_Status(void);
float Get_BatteryPackC_Status(void);
float Get_BatteryPackV_Status(void);
unsigned short Get_MotorRPM_Status(void);
unsigned char Get_UserStatus(void);

long long Get_Speed_Total(void);
unsigned int Get_Speed_Cnt(void);
float Get_Speed_Avg(void);
long long Get_RPM_Total(void);
unsigned int Get_RPM_Cnt(void);
unsigned int Get_RPM_Avg(void);

#if 1 // James Jean 2018/11/18
unsigned int Get_MotorRPM_Total(void);
unsigned int Get_MotorRPM_Cnt(void);
unsigned int Get_MotorRPM_Avg(void);
unsigned short Get_MotorMaxRPM(void);
#endif

unsigned int Get_WarmupTime(void);
unsigned int Get_EngineIdleTime(void);

float Get_Mass_Air_Flow(void);
float Get_MassAirPressure(void);
float Get_IntakeAirTemperature(void);
unsigned char Get_FuelCut(void);
float Get_InjectionQuantity(void);
float Get_MI(void);
float Get_PIL1(void);
float Get_PIL2(void);
float Get_PIL3(void);
float Get_POL1(void);
float Get_POL2(void);


/* OBD Controller  ==>  Message Manager Module */
void Get_VIN(unsigned char *vin);
unsigned short Get_periodMessage_counter(void);
void Up_periodMessage_counter(void);

long long Get_DrivingKey(void);
unsigned int Get_DrivingTime(void);
bool Get_Decide_Starting_Engine(void);
unsigned short Get_Speed(void);

unsigned int Get_Period_Speed_Counter_0(void);
unsigned int Get_Period_Speed_Counter_from_1_under_10(void);
unsigned int Get_Period_Speed_Counter_from_11_under_20(void);
unsigned int Get_Period_Speed_Counter_from_21_under_30(void);
unsigned int Get_Period_Speed_Counter_from_31_under_40(void);
unsigned int Get_Period_Speed_Counter_from_41_under_50(void);
unsigned int Get_Period_Speed_Counter_from_51_under_60(void);
unsigned int Get_Period_Speed_Counter_from_61_under_70(void);
unsigned int Get_Period_Speed_Counter_from_71_under_80(void);
unsigned int Get_Period_Speed_Counter_from_81_under_90(void);
unsigned int Get_Period_Speed_Counter_from_91_under_100(void);
unsigned int Get_Period_Speed_Counter_from_101_under_110(void);
unsigned int Get_Period_Speed_Counter_from_111_under_120(void);
unsigned int Get_Period_Speed_Counter_from_121_under_130(void);
unsigned int Get_Period_Speed_Counter_from_131_under_140(void);
unsigned int Get_Period_Speed_Counter_over_141(void);

unsigned int Get_Total_Speed_Counter_0(void);
unsigned int Get_Total_Speed_Counter_from_1_under_10(void);
unsigned int Get_Total_Speed_Counter_from_11_under_20(void);
unsigned int Get_Total_Speed_Counter_from_21_under_30(void);
unsigned int Get_Total_Speed_Counter_from_31_under_40(void);
unsigned int Get_Total_Speed_Counter_from_41_under_50(void);
unsigned int Get_Total_Speed_Counter_from_51_under_60(void);
unsigned int Get_Total_Speed_Counter_from_61_under_70(void);
unsigned int Get_Total_Speed_Counter_from_71_under_80(void);
unsigned int Get_Total_Speed_Counter_from_81_under_90(void);
unsigned int Get_Total_Speed_Counter_from_91_under_100(void);
unsigned int Get_Total_Speed_Counter_from_101_under_110(void);
unsigned int Get_Total_Speed_Counter_from_111_under_120(void);
unsigned int Get_Total_Speed_Counter_from_121_under_130(void);
unsigned int Get_Total_Speed_Counter_from_131_under_140(void);
unsigned int Get_Total_Speed_Counter_over_141(void);

unsigned int Get_PeriodDrivePattern0(void);
unsigned int Get_PeriodDrivePattern1(void);
unsigned int Get_PeriodDrivePattern2(void);
unsigned int Get_PeriodDrivePattern3(void);
unsigned int Get_PeriodDrivePattern4(void);
unsigned int Get_TotalDrivePattern0(void);
unsigned int Get_TotalDrivePattern1(void);
unsigned int Get_TotalDrivePattern2(void);
unsigned int Get_TotalDrivePattern3(void);
unsigned int Get_TotalDrivePattern4(void);
#if defined(PROTOCOL17)
unsigned int Get_PeriodDrivePattern5(void);
unsigned int Get_TotalDrivePattern5(void);
#endif
unsigned int Get_Total_Speed_Counter_over_141(void);
unsigned int Get_Total_Speed_Counter_over_141(void);
unsigned int Get_Total_Speed_Counter_over_141(void);
unsigned int Get_Total_Speed_Counter_over_141(void);

unsigned short Get_period_Rapid_acceleration_Counter(void);
unsigned short Get_period_Rapid_Deceleration_Counter(void);
unsigned short Get_total_Rapid_acceleration_Counter(void);
unsigned short Get_total_Rapid_Deceleration_Counter(void);
unsigned short Get_RPM(void);
unsigned short Get_MaxRPM(void);

uint16_t Get_TPMS1(eTPMS_TIRE_STATE eSelTire);
float Get_Battery(void);
unsigned char Get_DoorLock(void);
unsigned char Get_DoorOpen(void);
unsigned char Get_ACC(void);
eVEHICLE_STATE Get_VehicleStatus(void);
float Get_driving_distance(void);
unsigned short Get_ExtraDrivingDistance_Status();
unsigned int Get_driving_distance_int(void);	//float형으로 반환시 Internal Error: [AsmLine - OgAsm]: Error[406]: Bad data alignment 에러가 나옴 다른곳은 괜찮은데 sysMsg.carReport.rpInterval.AfterDrivingInfo.DrivingDistance 여기에만 대입하면그럼;;;
unsigned int Get_Odmeter(void);
unsigned int Get_StartOdmeter(void);
unsigned char Get_Remain_Fuel_Percent(void);
uint16_t Get_Remain_Fuel_Percent2(void);
double Get_Fuel_Consume(void); /* mL */
double Get_Energy_Consume(void);
double Get_Total_Fuel_Consume(void);
double Get_Total_Energy_Consume(void);
#if defined(PROTOCOL17)
double Get_Energy_Consume_Regen(void);
double Get_Total_Energy_Consume_Regen(void);
#endif
unsigned char Get_AccelPos(void);
short Get_CoolantTemperature(void);
char Get_OutsideTemperature(void);
short Get_HighBatteryTemperature(void);
#if defined(PROTOCOL18)
short Get_HighBatteryTemperatureMax(void); // dahae
#endif
unsigned char Get_GearPosition(void);
unsigned char Get_GearLever(void);
unsigned char Get_GearSbw(void);
unsigned short Get_TransmissionOil_Temperature(void);
bool Get_TailLamp_State(void);
bool Get_FootBrake_State(void);
bool Get_HeadLamp_State(void);
bool Get_MILLamp_State(void);
unsigned short Get_MaxSpeed(void);
unsigned int Get_AvgSpeed(void);
unsigned char Get_BT_Connection_State(void);
float Get_MileAge_avg(void);
float Get_ElectronicMileAge_avg(void);
float Get_TMPS_Conv(void);

float Get_Slope_TCU_From_CAN(void);
unsigned short Get_ChargeCount(void);
unsigned int Get_ChargeTime(void);
bool Get_RearSeatSet(void);
bool Get_RearSeatOccured(void);

bool Get_ChargeState(void);
bool Get_FATCState(void);
int Get_GpsDirection(void);

unsigned short Get_InsulationResistance(void);
unsigned char Get_LowBatterySOC_Status(void);

#endif // __OBD_CONTROLLER_GET_H__
