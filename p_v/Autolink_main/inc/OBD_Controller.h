/* OBD Controller */
#ifndef __OBD_CONTROLLER_H__
#define __OBD_CONTROLLER_H__

#include "common.h"
#include "AutolinkConfig.h"
#include "AutolinkConfiguration.h"
#include "HalHandler.h"


#define FUEL_LIST_MAX_CNT	60
#define MAX_ODOMETER_SKIP_DATA (1677721)
#define BLEDISCONNECT_TIMEOUT (40*60*1000)



#pragma pack(push, 1)
/* Control State */
typedef enum _eTPMS_TIRE_STATE
{
	eTPMS_TIRE_STATE_FL = 0,
	eTPMS_TIRE_STATE_FR,
	eTPMS_TIRE_STATE_RL,
	eTPMS_TIRE_STATE_RR,
} eTPMS_TIRE_STATE;

typedef enum _eTPMS_TIRE
{
	eTPMS_TIRE_FL = 0,
	eTPMS_TIRE_FR,
	eTPMS_TIRE_RL,
	eTPMS_TIRE_RR,
}eTPMS_TIRE;

typedef enum _eVEHICLE_STATE
{
	eVEHICLE_STATE_OFF,
	eVEHICLE_STATE_ACC,
	eVEHICLE_STATE_IGON,
//	eVEHICLE_STATE_IG2,
	eVEHICLE_STATE_ENGRUN,
	eVEHICLE_STATE_EV_ON,
#if defined(FEATURE_EXTENSION_BOARD)
	eVEHICLE_STATE_NONE,
#endif
	eVEHICLE_STATE_MAX
}eVEHICLE_STATE;

typedef enum _eLAMP_STATE
{
	eLAMP_STATE_NONE,
	eLAMP_STATE_CHECKING,
	eLAMP_STATE_LAMPON,
}eLAMP_STATE;

typedef enum _eHORN_STATE
{
	eHORN_STATE_NONE,
	eHORN_STATE_CHECKING,
	eHORN_STATE_HORNON,
}eHORN_STATE;

typedef enum _eUSER
{
	eUSER_OUT,
	eUSER_IN,
}eUSER;

typedef enum _eISGRUNFLAG
{
	eISGRUN_OFF,
	eISGRUN_ON,
}eISGRUNFLAG;

typedef enum _eENGRUNFLAG
{
	eENGRUN_OFF,
	eENGRUN_ON,
}eENGRUNFLAG;
typedef enum __eForceAlertState{
	eForceAlert_Init = 0,
	eForceAlert_Check,
	eForceAlert_Stop,
}eForceAlertState;

typedef __packed struct {
  unsigned char BT_State;
}BT_Connection_State;

typedef __packed struct {
	bool m_bAlram;
	bool m_bFL;
	bool m_bFR;
	bool m_bRL;
	bool m_bRR;
	bool m_bTR;
	bool m_bHood;
}DOOROPENALRAMCHECK;

typedef __packed struct {
	bool m_bAlram;
	bool m_bFL;
	bool m_bFR;
	bool m_bRL;
	bool m_bRR;
	bool m_bTR;
}DOORUNLOCKALRAMCHECK;

typedef __packed struct {
	bool m_bAlram;
	bool m_bFL;
	bool m_bFR;
	bool m_bRL;
	bool m_bRR;
}TPMSALRAMCHECK;

typedef __packed struct _stLockTime{
	unsigned char m_ucDoorLock;
	unsigned char m_ucDoorOpen;
	unsigned int m_uiLastLockTime;
}stLockState;

typedef __packed struct _stElectricCarData{
 	unsigned char m_ucSOH;
 	unsigned char m_ucSOC;
 	unsigned short m_usHighBatteryTemperatureMin;
 	unsigned short m_usHighBatteryTemperatureMax;
 	unsigned short m_usRemainedDistance;
}stElectricCarData;

typedef __packed struct {
	unsigned char vin[18];
	long long drivingKey; 								/* use 6 byte, It's engine starting time. */
	long long drivingTime;								/* use 3 byte */
	//long long drivingEndTime;							/* use 3 byte */
	unsigned short periodMessage_counter;				/* use 2 byte */ /* move to message M */
	unsigned int m_uiStartOdometer;
	unsigned int m_uiOldOdometer;
	bool decide_Starting_Engine;
	stHalRTC_TimeTypeDef		DriveStartTime_Local;
	stHalRTC_DateTypeDef		DriveStartDate_Local;
	stHalRTC_TimeTypeDef		DriveStopTime_Local;
	stHalRTC_DateTypeDef		DriveStopDate_Local;
	unsigned int	DriveStartTime_LocalTime;
	unsigned int	DriveStartTime_UTC;
	int16_t snDriveStartTimeZone;
} BASIC_DATA;										/* unique Data  */

typedef __packed struct{
	//CAN???
	unsigned char	m_ucGearLever;
	unsigned char	m_ucGearSbw;
	unsigned short m_usCurrentSpeed;
	unsigned short m_usMaxSpeed;
	unsigned short m_usRPM;
	unsigned short m_usMaxRPM;

	float m_fBattery;							/* voltage * 10 */
	unsigned char m_ucLowBatterySOC;
	unsigned char m_ucACC;
	unsigned int m_uiOdometer;							/* use 3 byte */
	short m_sCoolantTemperature;
	unsigned char m_ucFuel_Level;						/* % */
	float m_fFuel_Level_Liter;							/* L */
	float m_fSlope_TCU;									/* % */  
	unsigned char m_ucGearPosition;						/* ASCII */
	unsigned short m_usTransmissionOil_Temperature;     /* 'c*10  */
	float m_fMassAirFlow;                                      /* ????? */
	float m_fMassAirPressure;                                  /* ??? */
	float m_fIntakeAirTemperature;                             /* ?? ??*/
	unsigned char FuelCut;                                  /* ??? ??*/
	float m_fFlashFuel_Consume;					/* mL */
	unsigned char m_ucAirconTemperature;
	unsigned char m_ucVentStatus;
	unsigned char m_ucHornStatus;
	unsigned char m_ucUserCtrlStatus;
	unsigned char m_ucAccelPos;
	bool m_bCrashSignal;							//????
	bool m_bIG1;
	bool m_bIG2;
	bool m_bIG3;
	bool m_bISGRun;
	bool m_bENGRun; //mod.pdh 2021.12.29 to devide ISG with B0C
	bool m_bEngStallState;
	bool m_bFootBrake;
	bool m_Celsius;
	bool m_Fahrenheit;
	float m_fSOCState;
	float m_fBatteryPackC;
	float m_fBatteryPackV;
	unsigned short m_usMotorRPM;
	float m_fSOHState;
	unsigned short m_usExtraDrivingDistance;
	char m_cOutsideTemperature;
	short m_sHighBatteryTemperature;
#if defined(PROTOCOL18)
	short m_sHighBatteryTemperatureMax; //dahae
	bool m_bChargingState;
	unsigned short m_usExtraChargeTime;
#endif
	float m_fElectronicMileageAvg;
	unsigned short m_usChargeCount;
	unsigned int m_usChargeTime;
	bool m_bChargeState;
	bool m_bFATCState;
	bool m_bRearSeatSet;
	bool m_bRearSeatOccured;

	//??
	bool m_bTailLamp;
	bool m_bMILLamp;
	bool m_bHazardLamp;
	bool m_bRearDefog;
	bool m_bHeadLamp;
	bool m_bHighBeam;
	bool m_bSecurity;
	bool m_bFuelWarningLamp;
	bool m_bABSIndicatorState;
	bool m_bISGIndicatorState;
	bool m_bEPBIndicatorState;
	unsigned int m_uiBrakeJudderValue;
	bool m_bBMSIndicatorState;
	bool m_bENGOilPressIndicatorState;
//	bool m_bBrakeLamp;
	
	//????
	double m_fPeriod_Fuel_Consume;											/* mL */
	double m_fTotal_Fuel_Consume;											/* for driving */ /*use 3 byte*/
	double m_fPeriod_Energy_Consume;											/* mL */
	double m_fTotal_Energy_Consume;											/* for driving */ /*use 3 byte*/
#if defined(PROTOCOL17)
	double m_fPeriod_Energy_Consume_Regen;											// 회생제동을 뺀 배터리 사용량
	double m_fTotal_Energy_Consume_Regen;											// 회생제동을 뺀 배터리 사용량
#endif
	eVEHICLE_STATE m_ucVehicleStatus;
	float m_fDriving_Distance;									/* use 4 byte */	
	
	unsigned short period_Rapid_acceleration_Counter;
	unsigned short period_Rapid_Deceleration_Counter;
	unsigned short total_Rapid_acceleration_Counter;
	unsigned short total_Rapid_Deceleration_Counter;
	unsigned int m_uiPeriod_Speed_Counter_0;								/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_1_under_10;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_11_under_20;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_21_under_30;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_31_under_40;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_41_under_50;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_51_under_60;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_61_under_70;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_71_under_80;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_81_under_90;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_91_under_100;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_101_under_110;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_111_under_120;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_121_under_130;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_from_131_under_140;			/* use 3 byte*/
	unsigned int m_uiPeriod_Speed_Counter_over_141;						/* include 160km */
	unsigned int m_uiTotal_Speed_Counter_0;									/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_1_under_10;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_11_under_20;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_21_under_30;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_31_under_40;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_41_under_50;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_51_under_60;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_61_under_70;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_71_under_80;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_81_under_90;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_91_under_100;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_101_under_110;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_111_under_120;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_121_under_130;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_from_131_under_140;			/* use 3 byte*/
	unsigned int m_uiTotal_Speed_Counter_over_141;						/* include 160km */
	unsigned int m_uiPeriodDrivePattern0;											//(?? : ?? ==0 && RPM > 0 && ????? == 0 ) : ?? ??? ?? 
	unsigned int m_uiPeriodDrivePattern1;											//(?? : ?? == 0 &&  RPM > 0 && ????? ==1 && Acc_Pos ==0) : ?? ?? ?? 
	unsigned int m_uiPeriodDrivePattern2;											//(?? : ?? > 0  && RPM > 0 && ????? ==1 && Acc_Pos ==0) : ?? ?? ??
	unsigned int m_uiPeriodDrivePattern3;											//(?? : ?? > 0 && RPM >0 && ? ???? == 0 && Acc_Pos ==0) : ?? ??
	unsigned int m_uiPeriodDrivePattern4;											//(?? : ??  > 0 && RPM > 0 &&? ???? ==0 && Acc_Pos >0) : ?? ??
#if defined(PROTOCOL17)
	unsigned int m_uiPeriodDrivePattern5;
#endif
	unsigned int m_uiTotalDrivePattern0;												//(?? : ?? ==0 && RPM > 0 && ????? == 0 ) : ?? ??? ?? 
	unsigned int m_uiTotalDrivePattern1;												//(?? : ?? == 0 &&  RPM > 0 && ????? ==1 && Acc_Pos ==0) : ?? ?? ?? 
	unsigned int m_uiTotalDrivePattern2;												//(?? : ?? > 0  && RPM > 0 && ????? ==1 && Acc_Pos ==0) : ?? ?? ??
	unsigned int m_uiTotalDrivePattern3;												//(?? : ?? > 0 && RPM >0 && ? ???? == 0 && Acc_Pos ==0) : ?? ??
	unsigned int m_uiTotalDrivePattern4;												//(?? : ??  > 0 && RPM > 0 &&? ???? ==0 && Acc_Pos >0) : ?? ??
#if defined(PROTOCOL17)
	unsigned int m_uiTotalDrivePattern5;
#endif
	float MileageAvg;											/*?? */
	unsigned int EngineIdleTime;						/* ??? ?? ?? */
	unsigned int WarmupTime;							/* ??? ?? */
	long long SpeedTotal;
	unsigned int SpeedCnt;
	float SpeedAvg;							/* ???? */
	long long uiRPMTotal;
	unsigned int uiRPMCnt;
	unsigned int uiRPMAvg;												/* ??RPM */
#if 1 // James Jean 2018/11/18
    unsigned int uiMotorRPMTotal;
    unsigned int uiMotorRPMCnt;
    unsigned int uiMotorRPMAvg;
	unsigned short m_usMaxMotorRPM;
#endif
	float fFuelcellVoltLow;
	float fFuelcellVoltHigh;
	unsigned int uiHydrogenChargeCnt;
	float fHydrogenTankPress;
	short sHydrogenCurrentTemperature;
	float m_fHydrogenFuel;
	float m_fAirPurification;
	float m_fCO2Reduction;
#if defined(PROTOCOL21)
	unsigned char m_ucEngOilLifeRatio;
	unsigned char m_ucEngOilLifeEna;
	unsigned char m_ucEngOilLifeWarn;
#endif
#if defined(PROTOCOL23)
    unsigned short m_usInsultationResistance;
#endif
	float m_fBatteryCellVolMax;
	float m_fBatteryCellVolMin;
	//?? GPS
	unsigned int GpsStartOnValidation;				/* ?? ?? GPS Validation*/
	double GpsStartOnLat;									/* ?? ?? GPS ??*/
	double GpsStartOnLon;									/* ?? ?? GPS ??*/	
	int iGpsDirection;
	//???? GPS
	unsigned int EngStopGpsValidation;				/* ?? ?? GPS Validation*/
	double EngStopGpsLat;                                  /* ?? ?? GPS ??*/
	double EngStopGpsLon;                                /* ?? ?? GPS ??*/	
	unsigned char GpsListCount;
    double GpsLatitudeList[MAX_GPS_LIST_COUNT+1];
    double GpsLongitudeList[MAX_GPS_LIST_COUNT+1];
} MONITERING_DATA;

typedef __packed struct{
	unsigned char m_uc4WD;
	unsigned char m_ucABS;
	unsigned char m_ucAirbag;
	unsigned char m_ucAutoHold;
	unsigned char m_ucBatteryCharge;
	unsigned char m_ucCheckEngine;
	unsigned char m_ucDBCWarnning;
	unsigned char m_ucDPF;
	unsigned char m_ucSCR;
	unsigned char m_ucEPB;
	unsigned char m_ucTCS;
	unsigned char m_ucISG;
	unsigned char m_ucMDPS;
	unsigned char m_ucOilLevel;
	unsigned char m_ucOilPress;
	unsigned char m_ucParkingBrake;
	unsigned char m_ucPowerDown;
	unsigned char m_ucAHB;
	unsigned char m_ucHEV;
	unsigned char m_ucEV;
	unsigned char m_ucFCEV;
	unsigned char m_ucTPMS;
	
	unsigned int m_uiAlramFlag;
	unsigned long long m_ullIndicatorFromXFlag;
}ALRAM_DATA;

typedef __packed struct {
	//bool starting_Engine;	/* use decide_Starting_Engine in BASIC_DATA */
	bool airconditioner;
	unsigned char doorLock;							/* include 'Trunk Unlock' */
	/*	<MSB><00000000><LSB>    : doorLock
		00000000 : Lock
		00000001 : Front-Left Door Unlock
		00000010 : Front-Right Door Unlock
		00000100 : Rear-Left Door Unlock
		00001000 : Rear-Right Door Unlock
		00010000 : Trunk Unlock              */
	unsigned char doorOpen;							/* include 'Trunk Open' */
	/*	<MSB><00000000><LSB>    : doorOpen
		00000000 : Lock
		00000001 : Front-Left Door Open
		00000010 : Front-Right Door Open
		00000100 : Rear-Left Door Open
		00001000 : Rear-Right Door Open
		00010000 : Trunk Open				*/

	float TPMS_FL;									/* TPMS Front-Left */
	float TPMS_FR;									/* TPMS Front-Right */
	float TPMS_RL;									/* TPMS Rear-Left */
	float TPMS_RR;									/* TPMS Rear-Right */
	unsigned char TPMS;								/* TPMS Alarm, High prioty is low psi.  */
	float fTPMS_Conv;
	/* 		Define TPMS
		0: Over 30 psi (include 30psi)
		1: Normal (25 ~30 psi)
		2: Under 25 psi (include 25psi)
	*/
} CURRENT_ACTION_STATE;  /* unique Data  */

typedef __packed struct {
	BASIC_DATA basicData;
	MONITERING_DATA monitering_Data;	/* diagnosis CAN */
	CURRENT_ACTION_STATE currentState; /* check control state */
	ALRAM_DATA AlramData;
	/* need to include DTC */
} OBD_CONTROLLER_DATA;

#if 0  /* DTC from Capital src */
typedef struct _stSlaveDTCData{
	char 	m_cEcuID[5];
	U16 	m_cFunctiontype;
	U8 		m_cStartPos;
	U8 		m_cReadno;
	U8 		m_cSkipno;
	U8 		m_cTotalmasking[8];
	char 	m_cOpenReqNode[15];
	char 	m_cOpenResNode[15];
	char 	m_cReqNode[20];
	char 	m_cResNode[20];
	char 	m_cCloseReqNode[15];
	char 	m_cCloseResNode[15];
	U8 		m_cRequesttime;
	U8 		m_cSearchtype;
	U8 		m_cIndex;
}stSlaveDTCData;

typedef struct _stDTCFunc{
	U8		m_ucECU_ID[4];					/* DTC ECU ID */
	U16		m_usFuncType;					/* DTC FUNCTION TYPE */
	U8		m_ucState;						/* RESULT */
	INT32U	m_nOdometer;					/* FCS ODOMETER */
	U8		m_ucFCSTime[7];					/* FCS START TIME */
	U8		m_ucDTCCode[100];				/* DTC CODE */
}stDTCFunc;
#endif

//void InitializeOBDController(void);
void UpdateOBDPeriodData(void);
void InitializeOBDPeriodData(void);

//void Send_DTC_From_VCAN(unsigned char *DTC);

/* diagnosis Data */
/* OBD Manager  ==> OBD Controller,  Only for OBD Manager */
void Send_Speed_From_CAN(unsigned int Speed);
void Send_MaxBatteryCellVoltage_From_CAN(float BatteryCell);
void Send_MinBatteryCellVoltage_From_CAN(float BatteryCell);
void Send_HydrogenChargeCnt_From_CAN(unsigned int ChargeCount);
void Send_HydrogenTankPress_From_CAN(float TankPress);
void Send_HydrogenCurrentTemperature_From_CAN(short CurrentTemp);
void Send_RPM_From_CAN(unsigned int RPM);
void Send_Battery_From_CAN(float Battery);
//void Send_DoorLock_From_CAN(unsigned int DoorLock);
void Send_ACC_From_CAN(unsigned int unACC);
void Send_Odometer_From_CAN(unsigned int Odometer);
void Send_Remain_Fuel_Percent_From_CAN(float RemainFuelPercent);
void Send_AccelPos_From_CAN(unsigned char AccelPos);
void Send_Fuel_Consume_From_CAN(double FuelConsume, unsigned char cType); //mL
void Send_Coolant_Temperature_From_CAN(short sCoolantTemperature);
void Send_GearPosition_From_CAN(unsigned int GearPosition);
void Send_TransmissionOil_Temperature_From_CAN(unsigned int TransmissionOilTemperature);
void Send_BrakeLamp_From_CAN(bool BrakeLamp);
void Send_TailLamp_From_CAN(unsigned int TailLampOn);
void Send_FootBrake_From_CAN(bool FootBrakeOn);
void Send_HeadLamp_From_CAN(unsigned int HeadLampOn);
void Send_MILLamp_From_CAN(unsigned int MILLampOn);
//void Send_Airbag_From_DCAN(unsigned int Airback);
//void Send_DTC_From_DCAN(unsigned char *DTC);


/* others */
void Send_Impact_From_CAN(unsigned short Impact);
void Send_DTC_From_CAN(unsigned char *DTC);
void Send_Driving_Distance_From_CAN(float Distance);
void Send_ExtraDrivingDistance_From_CAN(unsigned short Distance);
void Send_ExtraDistance_From_CAN(float Distance);
void Send_Airbag_From_CAN(bool Airback);
void Send_hazardLamp_From_CAN(bool hazardLamp);
void Send_RearDefog_From_CAN(bool RearDefog);
void Send_HighBeam_From_CAN(bool HighBeam);
//void Send_OutLamp_From_CAN(bool OutLamp);
//void Send_DoorOpen_From_CAN(unsigned int doorOpen);
//void Send_TrunkOpen_From_CAN(bool TrunkOpen);

void Send_DoorLock_FL_From_CAN(char doorLock_FL);
void Send_DoorLock_FR_From_CAN(char doorLock_FR);
void Send_DoorLock_RL_From_CAN(char doorLock_RL);
void Send_DoorLock_RR_From_CAN(char doorLock_RR);
void Send_TrunkLock_From_CAN(char TrunkLock);

void Send_DoorOpen_FL_From_CAN(char doorOpen_FL);
void Send_DoorOpen_FR_From_CAN(char doorOpen_FR);
void Send_DoorOpen_RL_From_CAN(char doorOpen_RL);
void Send_DoorOpen_RR_From_CAN(char doorOpen_RR);
void Send_TrunkOpen_From_CAN(char TrunkOpen);
void Send_HoodOpen_From_CAN(char HoodOpen);

//void Send_TPMS_From_CAN(unsigned char TPMS);
void Send_TPMS_FL_From_CAN(float TPMS_FL);  /* TPMS Front-Left */
void Send_TPMS_FR_From_CAN(float TPMS_FR);  /* TPMS Front-Right */
void Send_TPMS_RL_From_CAN(float TPMS_RL);  /* TPMS Rear-Left */
void Send_TPMS_RR_From_CAN(float TPMS_RR);  /* TPMS Rear-Right */
void Send_TPMS_Unit_From_CAN(unsigned short usTPMS_Conv);

void Send_MassAirFlow_From_CAN(float MassAirFlow);
void Send_MassAirPressure_From_CAN(float massAirPressure);
void Send_IntakeAirTemperature_From_CAN(float intakeAirTemperature);
void Send_OutsideTemperature_From_CAN(char Temperature);
void Send_HighBatteryTemperature_From_CAN(short Temperature);

#if defined(PROTOCOL18)
void Send_HighBatteryTemperatureMAX_From_CAN(short Temperature);
void Send_ChargingState_From_CAN(bool ChargingState);
void Send_ExtraChargeTime_From_CAN(unsigned short ExtraChargeTime);
#endif

#if defined(PROTOCOL21)
void Send_EngOilLifeRatio_From_CAN(unsigned char EngineOilLifeRatio);
void Send_EngOilLifeEna_From_CAN(unsigned char EngOilLifeEna);
void Send_EngOilLifeWarn_From_CAN(unsigned char EngOilLifeWarn);
#endif

void Send_InjectionQuantity_From_CAN(float fValue);
void Send_MI_From_CAN(float fValue);
void Send_PIL1_From_CAN(float fValue);
void Send_PIL2_From_CAN(float fValue);
void Send_PIL3_From_CAN(float fValue);
void Send_POL1_From_CAN(float fValue);
void Send_POL2_From_CAN(float fValue);
void Send_FuelCut_From_CAN(unsigned char fuelCut);
void Send_Total_Fuel_Consume(float TotalFuelConsume); //mL
void Send_Mileage_Avg(float mileAgeAvg);
void Send_ElectronicMileage_Avg(float mileAgeAvg);
void Send_Speed_Total(long long speedTotal);
void Send_Speed_Avg(float speedAvg);
void Send_Mileage_Avg_All(float mileAgeAvg);
void Send_Speed_Total_All(long long speedTotal);
void Send_Speed_Avg_All(float speedAvg);
void Send_RPM_Total(long long RPMTotal);
void Send_RPM_Avg(unsigned int RPMAvg);

#if 1 // James Jean 2018/11/18
void Send_MotorRPM_Total(unsigned int MotorRPMTotal);
void Send_MotorRPM_Avg(unsigned int MotorRPMAvg);
void increase_MotorRPM_Cnt(void);
#endif

void Send_GpsStatOnValidation(unsigned int gpsValidation);
void Send_GpsStartOnLat(double gpsLat);
void Send_GpsStartOnLon(double gpsLon);

void Send_EngStopGpsValidation(unsigned int gpsValidation);
void Send_EngStopGpsLat(double gpsLat);
void Send_EngStopGpsLon(double gpsLon);
void Send_GpsDirection(int direction);

void Send_AirconTemperature_From_CAN(unsigned char AirconTemperature);
void Send_VentStatus_From_CAN(unsigned char VentStatus);
void Send_HornStatus_From_CAN(unsigned char HornStatus);
void Send_IG1_From_CAN(bool IG1);
void Send_IG2_From_CAN(bool IG2);
void Send_IG3_From_CAN(bool IG3);
void Send_ISG_From_CAN(bool Running);
void Send_EngRun_From_CAN(bool EngRun);
void Send_EngStall_From_CAN(bool EngStall);
void Send_SOCState_From_CAN(float SOCState);
void Send_SOHState_From_CAN(float SOHState);
void Send_BatteryPackC_From_CAN(float );
void Send_BatteryPackV_From_CAN(float );
void Send_MotorRPM_From_CAN(unsigned int);
void Send_CrashSignal_From_CAN(bool Crash);
void Send_Vehicle_Status_From_CAN(eVEHICLE_STATE);
void Send_UserStatus_From_CAN(unsigned char User);
void Send_ChargeCount_From_CAN(unsigned short usChargeCount);
void Send_ChargeTime_From_CAN(unsigned int usChargeTime);
void Send_ChargeState_From_CAN(bool bChargeState);
void Send_ABSIndicator_From_CAN(bool bInput);
void Send_ISGIndicator_From_CAN(bool bInput);
void Send_EPBIndicator_From_CAN(bool bInput);
void Send_BrakeJudder_From_CAN(unsigned int uiInput);
void Send_BMSIndicator_From_CAN(bool bInput);
void Send_ENGOILPRESSIndicator_From_CAN(bool bInput);
void Send_FATCState_From_CAN(bool bFATCState);
void Send_RearSeatSet_From_CAN(bool bRearSeatSet);
void Send_RearSeatOccured_From_CAN(bool bRearSeatOccured);
void Send_Hydrogen_Fuel_From_CAN(float fHydrogenFuel);
void Send_AirPurification_From_CAN(float AirPurification);
void Send_CO2Reduction_From_CAN(float CO2);
void Send_4WD_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_ABS_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_Airbag_IND_From_CAN( unsigned char ucInput);
void Send_AutoHold_IND_From_CAN( unsigned char ucInput);
void Send_BatteryCharge_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_CheckEngine_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_DBCWarnning_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_DPF_IND_From_CAN( unsigned char ucInput );
void Send_SCR_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_EPB_IND_From_CAN( unsigned char ucInput );
void Send_TCS_IND_From_CAN( unsigned char ucInput );
void Send_ISG_IND_From_CAN( unsigned char ucInput );
void Send_MDPS_IND_From_CAN( unsigned char ucInput );
void Send_OilLevel_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_OilPress_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_ParkingBrake_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_PowerDown_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_AHB_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_HEV_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_EV_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_FCEV_IND_From_CAN( unsigned char ucInput, unsigned char ucMasking );
void Send_TPMS_IND_From_CAN( unsigned char ucInput );
void Send_InsulationResistance_From_CAN (unsigned short usInsulationResistance);
void Send_LowBatterySOC_From_CAN(unsigned char Battery);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Set_DriveStopTime(void);
void Set_DriveStartTime(void);
void increase_Speed_Cnt(void);
void increase_RPM_Cnt(void);
void Increase_period_Rapid_acceleration_Counter(void);
void Increase_period_Rapid_Deceleration_Counter(void);
void Set_Mass_Air_Flow(float massAirFlow);
void Increase_WarmupTime(void);
void Increase_EngineIdleTime(void);
void Increase_DrivePattern0(void);
void Increase_DrivePattern1(void);
void Increase_DrivePattern2(void);
void Increase_DrivePattern3(void);
void Increase_DrivePattern4(void);
void Increase_DrivePattern5(void);

void Decide_Crash_Alarm(void);
void Decide_MIL_Lamp_Alarm(void);
void Decide_Battery_High_Voltage_Alarm(void);
void Decide_Door_Lock_Alarm(void);
void Decide_Door_Open_Alarm(void);
void Decide_Coolant_Temperature_Alarm(void);
void Decide_Fuel_Run_Out_Alarm(void);
unsigned char Decide_Each_TPMS(float Each_TPMS);
void Decide_TPMS_Alarm();
void Decide_BrakeJudder_Alarm();
void Decide_OverSpeed_Alarm(void);
void Decide_OverRPM_Alarm(void);
void Decide_OverMotorRPM_Alarm(void);
void Decide_Tail_Lamp_Alarm(void);
void Decide_MIL_Lamp_Alarm(void);
void Decide_Indicator_Alarm(void);

#if defined(FEATURE_EXTENSION_BOARD) 
void Decide_Foot_Break_Alarm(void);
void RequestExtendFOBKeyControl(void);
#endif

float Kalmanfilter(float);
//#define DEBUG_CAN_PARSING

//void SendIndicatorAlramReport(int32_t nEvent, unsigned int InputVal_1, unsigned int InputValue_2, unsigned int InputVal_3);
void SendIndicatorAlramReport(int32_t nEvent, unsigned int InputVal_1, unsigned long long );
void Clear_Drivingkey(void);
void InitializeOBDTripData(void);

extern void Making_Drivingkey(void);
void CB_Indicator_3secCallback();
void CB_BLE_Disconnect_40minCallback();
void EngineStartFunction();
void EngineStopFunction(unsigned char);
void Send_Slope_TCU_From_CAN(float SlopeTcu);
BOOL Discrimination_Stable_Slope_TCU();
void SetGyroAngle(stSensorInfo* pstGyroAngle);
void Send_Celsius_From_CAN(bool Celsius);
void Send_Fahrenheit_From_CAN(bool Fahrenheit);
void Set_Fahrenheit_UpdatedFlag(bool bFlag);
void Decide_Security_Alarm(void);
void Send_Security_From_CAN(bool bFlag);
void Send_FuelWarningLamp_From_CAN(bool bFlag);
void Decide_FuelWarningLamp_Alarm(void);
void Decide_SecurityAlarm(void);
void ReportRsvEngCtrlStatus();
#if defined(QA_FIFA)
void RPM_1SecCheckCallback(void);
void ENGRUN_1SecCheckCallback(void); 
#endif //#if defined(QA_FIFA)

void SetRCVBatteryFromCAN(boolean_t bIsBatteryData);
boolean_t GetRCVBatteryFromCAN(void);


#endif /* __OBD_CONTROLLER_H__ */

