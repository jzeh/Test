/**
  ******************************************************************************
  * @file    AutolinkMessage.h
  * @author  GIT Connectivity Development 2 Team
  * @version V1.0.0
  * @date    20-Dec-2017
  * @brief   Header for All Manager using Message type
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AUTOLINK_MESSAGE_H__
#define __AUTOLINK_MESSAGE_H__

#include "AutolinkConfiguration.h"

#include "common.h"
#include "HalHandler.h"


#define MAX_REAL_VIN_SIZE 17
#define MAX_VIN_SIZE 18
#define MAX_APN_SIZE 64

#define MAX_GUID_LENGTH 32

#if defined(PROTOCOL24)
#define MAX_MODEM_STATUS_LENGTH 200
#endif

#define MAX_BT_CONTROLKEY_LENGTH 16

#if defined(PROTOCOL17)
#define MAX_SERVER_URL_LENGTH 200
#define AES_PADDING_LENGTH	8
#define MAX_SERVER_PATH_LENGTH 128
#endif

enum{
    ID_MNG_QUEUE_SYS = 0,
    ID_MNG_QUEUE_SYS_SUB,
    ID_MNG_QUEUE_SYS_MSG,
    ID_MNG_QUEUE_SYS_SENSOR,
    ID_MNG_QUEUE_MDM,
    ID_MNG_QUEUE_STORAGE,
    ID_MNG_QUEUE_OBD,
    ID_MNG_QUEUE_SENSOR,
    ID_MNG_QUEUE_DATA,
    ID_MNG_QUEUE_BT,
    ID_MNG_QUEUE_MDMH,
#if defined(FEATURE_EXTENSION_BOARD)
    ID_MNG_QUEUE_EXTEND,
#endif
    ID_MNG_QUEUE_MAX,
};

// 6-2 : 10s sample
#define MAX_GPS_LIST_COUNT (((60/MAX_GPS_SAMPLE_TIME)-2)/2)

typedef __packed struct __stReportDrivingInfoBody{
    long long OperationKey;
    uint16_t Speed;
    uint16_t Rpm;
    uint16_t MaxSpeed;
    uint16_t MaxRpm;
    double ConsumedFuel;
    uint32_t DrivingDistance;
    uint16_t RapidAccelCount;
    uint16_t RapidDecelCount;
    uint8_t GpsValid;
    uint8_t GpsSateliteCount;
    double GpsLatitude;
    double GpsLongitude;
    uint8_t GpsListCount;
    double GpsLatitudeList[MAX_GPS_LIST_COUNT];
    double GpsLongitudeList[MAX_GPS_LIST_COUNT];
    uint16_t TpmsFL;
	uint16_t TpmsFR;
	uint16_t TpmsRL;
	uint16_t TpmsRR;
    uint16_t CarBattery;
    uint8_t ModemRssi;
    uint8_t DoorLockStatus;
    uint8_t DoorOpenStatus;
    uint8_t AccStatus;
    uint32_t Odometer;
    uint8_t BleStatus;
    uint8_t RemainedFuel;
    uint8_t RemainedECarBattery;
    uint8_t TailLanmp;
    uint8_t InteriorLamp;
    uint8_t HeadLight;
    uint8_t MILLamp;
    float FuelEffiency;
	uint16_t EngineIdleTime;
    uint16_t WarmUpTime;
	uint8_t SpdCnt0km;
	uint8_t SpdCntUpper1kmUnder10km;
    uint8_t SpdCntUpper11kmUnder20km;
	uint8_t SpdCntUpper21kmUnder30km;
	uint8_t SpdCntUpper31kmUnder40km;
	uint8_t SpdCntUpper41kmUnder50km;
	uint8_t SpdCntUpper51kmUnder60km;
	uint8_t SpdCntUpper61kmUnder70km;
	uint8_t SpdCntUpper71kmUnder80km;
	uint8_t SpdCntUpper81kmUnder90km;
	uint8_t SpdCntUpper91kmUnder100km;
	uint8_t SpdCntUpper101kmUnder110km;
	uint8_t SpdCntUpper111kmUnder120km;
	uint8_t SpdCntUpper121kmUnder130km;
	uint8_t SpdCntUpper131kmUnder140km;
    uint8_t SpdCntUpper140kmUnder150km;
    uint8_t SpdCntUpper150kmUnder160km;
    uint8_t SpdCntUpper141km;
    uint8_t IdlingCount;
    uint8_t StopCount;
    uint8_t BreakeCount;
    uint8_t InertiaDrivingCount;
    uint8_t NormalDrivingCount;
#if defined(PROTOCOL17)
	uint8_t ExtraDrivingCount;
	double ConsumedBattery_Regen;
#endif
    uint16_t SequnceNumber; /* added +2 because body1+body2 */
    uint32_t OccurredEventTime;
    uint32_t OccurredEventUtcTime;
	uint16_t MotorRpm;
	uint16_t MaxMotorRpm;
	float ElectronicEffiency;
	double ConsumedBattery;
	uint8_t SOH;
	uint16_t ExtraDrivingDistance;
	uint16_t HighBatteryTemperature;
	uint32_t EngRunTime;
#if defined(PROTOCOL18)
	uint16_t HighBatteryTemperatureMax; //dahae
#endif
#if defined(PROTOCOL19)
	uint32_t HydrogenChargeCnt;
	float HydrogenTankPress;
	float FuelcellVoltLow;
	float FuelcellVoltHigh;
	float HydrogenFuel;
	float AirPurification;
	float CO2Reduction;
	short HydrogenTemperature;
	float BMSBatteryVolt;
	float BMSBatteryCurr;
#endif
#if defined(PROTOCOL21)
	uint8_t EngOilLifeRatio;

	uint8_t EngOilLifeEna;
	uint8_t EngOilLifeWarn;
#endif
	uint8_t OutsideTemperature;
	uint32_t GpsDirection;
#if defined(PROTOCOL22)
	uint8_t LowBatterySOC;
#endif	
#if defined(PROTOCOL23)
 	unsigned short InsulationResistance;
#endif
}stReportDrivingInfoBody;

typedef __packed struct __stReportDrivingInfo{
    stReportDrivingInfoBody DrivingInfoB1;
    stReportDrivingInfoBody DrivingInfoB2;
}stReportDrivingInfo;

typedef __packed struct __stReportAfterDrivingInfo{
    long long OperationKey;
    uint32_t StartDrivingLocalTime;
    uint32_t StopDrivingLocalTime;
    uint32_t StartDrivingUTCTime;
    uint32_t StopDrivingUTCTime;
    uint16_t MaxSpeed;
	uint32_t AvgSpeed;
    uint16_t MaxRpm;
	uint32_t AvgRpm;
	uint16_t CarBattery;
    uint16_t RapidAccelCount;
    uint16_t RapidDecelCount;
    uint16_t FakeDrivingTime;
    uint16_t WarmUpTime;
    double ConsumedFuel;
    uint16_t FuelEffiency;
    uint32_t DrivingTime;
    uint32_t DrivingDistance;
//    uint32_t EverageSpeed;
    uint8_t GeerPosition;
    uint8_t RemainedFuel;
    uint8_t RemainedECarBattery;
    uint8_t DrivingConsumedFuel;
    uint8_t StartGpsValid;
    double PowerOnLatitude;
    double PowerOnLongitude;
    uint8_t EndGpsValid;
    double EndGpsLatitude;
    double EndGpsLongitude;
    uint32_t Odometer;
	uint32_t StartOdometer;
	uint32_t SpdCnt0km;
    uint32_t SpdCntUpper1kmUnder10km;
    uint32_t SpdCntUpper11kmUnder20km;
	uint32_t SpdCntUpper21kmUnder30km;
	uint32_t SpdCntUpper31kmUnder40km;
	uint32_t SpdCntUpper41kmUnder50km;
	uint32_t SpdCntUpper51kmUnder60km;
	uint32_t SpdCntUpper61kmUnder70km;
	uint32_t SpdCntUpper71kmUnder80km;
	uint32_t SpdCntUpper81kmUnder90km;
	uint32_t SpdCntUpper91kmUnder100km;
	uint32_t SpdCntUpper101kmUnder110km;
	uint32_t SpdCntUpper111kmUnder120km;
	uint32_t SpdCntUpper121kmUnder130km;
	uint32_t SpdCntUpper131kmUnder140km;
    uint32_t SpdCntUpper141km;
    uint32_t AccIdlingCount;
    uint32_t AccStopCount;
    uint32_t AccBreakeCount;
    uint32_t AccInertiaDrivingCount;
    uint32_t AccNormalDrivingCount;
#if defined(PROTOCOL17)
	uint32_t AccExtraDrivingCount;
	double ConsumedBattery_Regen;
#endif
    uint16_t TpmsFL;
	uint16_t TpmsFR;
	uint16_t TpmsRL;
	uint16_t TpmsRR;
    uint8_t TailLanmp;
    uint8_t InteriorLamp;
    uint8_t HeadLight;
    uint8_t MasterDBVersion;
    uint16_t SlaveDBVersion;
    uint16_t DrivingVersion;
    uint16_t BootloaderVersion;
    uint16_t AppVersion;
	double ConsumedBattery;
	uint8_t SOH;
	uint16_t ExtraDrivingDistance;
	uint16_t HighBatteryTemperature;
#if defined(PROTOCOL18)
	uint16_t HighBatteryTemperatureMax;
	char ChargingState;
#endif
	uint8_t OutsideTemperature;
	uint16_t MaxMotorRpm;
	uint32_t AvrMotorRpm;
	uint16_t m_usChargeCount;
	uint32_t m_usChargeTime;
	uint8_t m_bChargeState;
	uint32_t GpsDirection;
#if defined(PROTOCOL22)
	uint8_t LowBatterySOC;
#endif	
#if defined(PROTOCOL23)
	uint32_t HydrogenChargeCnt;
	float HydrogenTankPress;
	float FuelcellVoltLow;
	float FuelcellVoltHigh;
	float HydrogenFuel;
	float AirPurification;
	float CO2Reduction;
	short HydrogenTemperature;
	unsigned short InsulationResistance;
	uint8_t ModemRssi;
#endif
}stReportAfterDrivingInfo;

typedef __packed struct __stReportBeforeDrivingInfo{
    uint16_t AlramMaskSettingValue;
    uint32_t ParkingReportInterval;
    uint32_t DrivingReportInterval;
    uint32_t ValetAlramSetupThreshold;
    uint32_t ValetAlramSetupTime;
    uint32_t TowingAlramSetupthreshold;
    uint32_t TowingAlramSetupTime;
}stReportBeforeDrivingInfo;

typedef __packed  struct __stTrackingControl{
	boolean_t bActive;
	uint32_t unActiveTime;
	uint8_t ucArrGUID[MAX_GUID_LENGTH];
    uint8_t ucDurationMinTime;
	uint8_t ucIntervalSec;
    uint8_t ucDataIndex;
	double dLatitude;
	double dLongitude;
	uint8_t ucGpsValid;
	int16_t usCheckSum;
}stTrackingControl;

typedef __packed struct __stReportNoDrivingInfo{
    uint8_t GpsValid;
    uint8_t DoorLockStatus;
    uint8_t DoorOpenStatus;
    uint8_t HeadLight;
    double GpsLatitude;
    double GpsLongitude;
    uint16_t CarBattery;
    uint16_t ModemRssi;
    uint32_t Odometer;
    uint32_t OccurredEventTime;
    uint32_t OccurredEventUtcTime;
#if defined(PROTOCOL12)
	uint8_t VehicleStatus;
#endif
#if defined(PROTOCOL18)
	uint16_t ExtraDrivingDistance;
	uint8_t RemainedECarBattery;
	uint16_t HighBatteryTemperature;
	uint16_t HighBatteryTemperatureMax;
#endif

//    uint16_t MasterDBVersion;
//    uint16_t SlaveDBVersion;
//    uint16_t DrivingDBVersion;
//    uint16_t BootloaderVersion;
//    uint16_t AppVersion;
}stReportNoDrivingInfo;

#if defined(PROTOCOL18)
typedef __packed struct __stReportChargingInfo{
    uint8_t  HighBatterySOC;
    uint16_t ExtraDrivingDistance;
    uint16_t HighBatteryTemperatureMin;
    uint16_t HighBatteryTemperatureMax;
    uint8_t  HighBatterySOH;
    uint16_t ExtraCharingTime;
}stReportChargingInfo;
#endif

typedef __packed union __stReportInterval{
    stReportDrivingInfo         DrivingInfo;
    stReportAfterDrivingInfo    AfterDrivingInfo;
    stReportBeforeDrivingInfo   BrforeDrivingInfo;
    stReportNoDrivingInfo       NoDrivingInfo;
#if defined(PROTOCOL18)
	stReportChargingInfo        ChargingInfo;
#endif
}stReportInterval;

#if defined(PROTOCOL21)
#define MAX_EVENT_KEY_VALUE 64
#elif defined(PROTOCOL19)
#define MAX_EVENT_KEY_VALUE 52
#elif defined(PROTOCOL16)
#define MAX_EVENT_KEY_VALUE 42
#else
#define MAX_EVENT_KEY_VALUE 28
#endif

typedef __packed struct __stReportCarStatus{
    long long OperationKey;
    uint32_t OccurredEventTime;
    uint32_t OccurredEventUtcTime;
    uint16_t EventKey;
    uint8_t EventKeyValue[MAX_EVENT_KEY_VALUE];
    uint32_t Odometer;
    double GpsCurLatitude;
    double GpsCurLongitude;
    double GpsSetLatitude;
    double GpsSetLongitude;
    uint8_t Boundtype;
    uint32_t Distance;
#if defined(PROTOCOL23)
	uint8_t ModemRssi;
#endif
#if defined(FEATURE_EXTENSION_BOARD)
    uint8_t cFootBreakStatus;
#endif
}stReportCarStatus;

typedef __packed struct __stReportDtc
{
    long long OperationKey;
	uint8_t MILStatus;
    uint8_t ResponseType;
    uint8_t Index;
    uint32_t OccurredEventTime;
    uint32_t OccurredEventUtcTime;
    uint64_t DTCCode;
    uint64_t DescKey;
    uint64_t EcuId;
    uint8_t Status[10];
    uint32_t Odo;
    uint8_t System[10];
    uint8_t ProtocolId[10];
    uint8_t FunctionType[10];
    uint16_t Class;
    uint16_t FreezeFrame;
}stReportDtc;

typedef __packed union __stReportAlram
{
    stReportCarStatus   CarStatus;
    stReportDtc         Dtc;
}stReportAlram;

enum{
	eSysSmartkeyReqType_Modem = 0,
	eSysSmartkeyReqType_BT,
	eSysSmartkeyReqType_Sys,
};

typedef __packed struct __stReportSmartKeyReq{
    uint8_t CommandType;
    uint8_t Guid[MAX_GUID_LENGTH];
    uint8_t ControlType;
    uint8_t KeepPowerOnTime;
    uint16_t Temperature;
	uint8_t CheckTemperature;
    uint8_t RearDefogger;
    uint8_t HighBeam;
    uint8_t Defrost;
    double Latitude;
    double Longitude;
    uint8_t BoundType;
    uint32_t Distance;
    uint32_t OccurredEventTime;
    uint32_t OccurredEventUtcTime;
	uint8_t ucSysSmartkeyReqType;
#if defined(PROTOCOL18)
	uint8_t BTControlKey[MAX_BT_CONTROLKEY_LENGTH];
#endif
}stReportSmartReq;

typedef __packed struct __stReportSmartKeyRsp{
    uint8_t Guid[MAX_GUID_LENGTH];
   	uint8_t CommandType;
   	uint8_t ControlType;
    uint32_t OccurredEventTime;
    uint32_t OccurredEventUtcTime;
    uint8_t Result;
#if defined(REASON_8BYTE)
	uint64_t Reason;
#else
    uint32_t Reason;
#endif
    uint32_t ReceviedTime;
	uint16_t Temperature;
    uint8_t Defrost;
	uint8_t CheckTemperature;
	uint8_t KeepPowerOnTime;
	uint8_t ucSysSmartkeyReqType;
#if defined(PROTOCOL18)
	uint8_t BTControlKey[MAX_BT_CONTROLKEY_LENGTH];
#endif
}stReportSmartRsp;

typedef __packed struct __stReportSmartKeyStatus{
    uint8_t Guid[MAX_GUID_LENGTH];
    uint16_t CarBattery;
    uint8_t TailLanmp;
    uint8_t HeadLight;
    uint8_t DoorLockStatus;
    uint8_t DoorOpenStatus;
    uint8_t AccStatus;
    double Latitude;
    double Longitude;
    uint32_t OccurredEventTime;
    uint32_t OccurredEventUtcTime;
#if defined(PROTOCOL18)
    uint8_t BTControlKey[MAX_BT_CONTROLKEY_LENGTH];
#endif
}stReportSmartStatus;

typedef __packed union __stReportSmartKey{
    stReportSmartReq Request;
    stReportSmartRsp Response;
    stReportSmartStatus Status;
#if defined(PROTOCOL22)
	uint32_t unRcvSMSTime;
#endif
}stReportSmartKey;

#define MAX_POLYGON_LIST 2
#define MAX_POLYGON_LIST_COUNT 8

#define MAX_GEOFENCE_LIST 8

typedef __packed struct __stGeofenceUnit{
    uint8_t cFenceType;
    uint32_t unDistance;
//    uint8_t arrDateTime[16];
    double  GpsLatitude;
    double  GpsLongitude;
    boolean_t bActive;
    boolean_t bAlreadyAlramed;
    boolean_t bInAlreadyAlramed;
    boolean_t bOutAlreadyAlramed;
}stGeofenceUnit;

typedef __packed struct __stPolygonGeofencePoint
{
    double  GpsLatitude;
    double  GpsLongitude;
}stPolygonGeofencePoint;

typedef __packed struct __stPolygonGeofenceUnit{
    uint8_t cFenceType;
    uint32_t unGeofenceID;
    stPolygonGeofencePoint stPointList[MAX_POLYGON_LIST_COUNT];
    uint8_t ucPointListCount;
    boolean_t bActive;
    boolean_t bAlreadyAlramed;
    boolean_t bInAlreadyAlramed;
    boolean_t bOutAlreadyAlramed;
}stPolygonGeofenceUnit;

typedef __packed struct __stGeoFenceSetting{
    uint8_t usCommand;
//    uint8_t carrGuid[MAX_GUID_LENGTH];
    uint16_t usLengthSize;
    uint16_t usLength;
    uint32_t unDateTime;
    boolean_t bEnableAllGeofence;
    stGeofenceUnit stGeofencePoint[MAX_GEOFENCE_LIST];
}stGeoFenceSetting;

typedef __packed struct __stPolygonGeoFenceSetting{
    uint8_t usCommand;
    uint16_t usLengthSize;
    uint16_t usLength;
    uint32_t unDateTime;
    boolean_t bEnableAllGeofence;
    uint8_t ucPolygonGeofenceCount;
    stPolygonGeofenceUnit stPolygonGeofenceList[MAX_POLYGON_LIST];
}stPolygonGeoFenceSetting;

typedef __packed struct __stServiceTypeSetting{
    uint8_t unServiceType;
}stServiceTypeSetting;

typedef __packed struct __stEncryptTypeSetting{
    uint8_t unEncryptType;
}stEncryptTypeSetting;

typedef __packed struct __stImpulseSensitivity
{
    uint8_t ucLow;
    uint8_t ucMiddle;
    uint8_t ucHigh;
    uint8_t ucActiveLevel;
}stImpulseSensitivity;

typedef __packed struct __stValetSetting{
    boolean_t bActive;
    uint32_t nDistance;
    uint32_t unActiveTime;
    uint32_t unKeepTime;
    stGeofenceUnit CurGpsPosition;
}stValetSetting;

typedef __packed struct __stTowingSetting{
    boolean_t bActive;
}stTowingSetting;

typedef __packed struct __stGuardSetting{
    boolean_t bActive;
    uint32_t unActiveTime;
    uint32_t unKeepTime;
    stImpulseSensitivity stImpulseSetting;
}stGuardSetting;

typedef __packed struct __stActiveModemSetting{
    boolean_t bActive;
}stActiveModemSetting;

#if defined(PROTOCOL18)
typedef __packed struct __stRsvEngCntorlInfo{
	uint8_t ucGUID[MAX_GUID_LENGTH];
	boolean_t bRsvEngCtrl;
	boolean_t bRepeat;
	uint32_t nRsvTime;
	uint8_t ucRsvDayoftheweek;
	uint32_t nRsvDate;
	uint8_t ucDurationTime;
	uint16_t usTemperature;
	uint8_t ucCheckTemperature;
	boolean_t bRearDefog;
	boolean_t bHeadLamp;
	boolean_t bDeperost;
}stRsvEngCntorlInfo;

typedef __packed struct __stRsvEngCntorl{
	uint8_t ucGroupGUID[MAX_GUID_LENGTH];
	uint16_t usLengthSize;
	uint8_t ucDataCnt;
	uint16_t usLength;
	uint32_t unRcvSMSTime;
	stRsvEngCntorlInfo stRsvEngCtrlInfo[5]; // 5 max reserve number, need to define
	uint8_t ucWakeupIndexFlag;
    uint8_t Result;
#if defined(REASON_8BYTE)
	uint64_t Reason;
#else
    uint32_t Reason;
#endif
    uint32_t unEventTime;
}stRsvEngCntorl;
#endif

typedef __packed struct __stUserActionSetting{
    stValetSetting Valet;
    stTowingSetting Towing;
    stGuardSetting Guard;
    stActiveModemSetting ActiveModem;
    stGeoFenceSetting Geofence;
    stPolygonGeoFenceSetting PolygonGeofence;
#if defined(PROTOCOL18)
    stRsvEngCntorl RsvEngCtrl;
#endif
#if defined(FEATURE_EXTENSION_BOARD)
    boolean_t bAntithiefFlag;
#endif
	stTrackingControl Tracking;
}stUserActionSetting;

#if defined(PROTOCOL12)
typedef __packed  struct __stModemActivate{
    uint8_t carrRequestGUID[MAX_GUID_LENGTH];
    //uint8_t carrRequestVIN[MAX_VIN_SIZE];
    uint32_t unOdometer;
    uint32_t unEventTime;
    uint32_t unSmsRcvTime;
    uint8_t ucResult;
    uint16_t usReason;
}stModemActivateSetting;
#endif

#if defined(PROTOCOL15)
typedef __packed  struct __stSensorInitialize{
    //uint8_t carrRequestGUID[MAX_GUID_LENGTH];
    //uint8_t carrRequestVIN[MAX_VIN_SIZE];
    uint8_t ucResult;
    uint8_t ucReason;
    uint32_t unEventTime;
    uint32_t unSmsRcvTime;
    int16_t sX;
    int16_t sY;
    int16_t sZ;
}stSensorInitialize;
#endif

#if defined(PROTOCOL17)
typedef __packed  struct __stUrlSetting{
    uint8_t ucResult;
    uint8_t ucReason;
    uint32_t unEventTime;
    uint32_t unSmsRcvTime;
}stUrlSetting;
#endif

#if defined(PROTOCOL24)
typedef __packed struct __stReportModemStatus{
 	double   Latitude;
    double   Longitude;  
 	uint8_t  GpsValid;
	uint8_t  ModemConnectStatus;
	uint8_t  ModemStatus[MAX_MODEM_STATUS_LENGTH];
    uint32_t OccurredEventTime;
    uint32_t OccurredEventUtcTime;
}stReportModemStatus;
#endif

#if defined(FEATURE_EXTENSION_BOARD)
typedef __packed  struct __stAntiThiefSetting{
    uint8_t ucResult;
    uint8_t ucReason;
    boolean_t bAntithiefFlag;
}stAntiThiefSetting;
#endif

typedef __packed union __stReportUserActionSetting{
    stValetSetting Valet;
    stTowingSetting Towing;
    stGuardSetting Guard;
    stActiveModemSetting ActiveModem;
    stGeoFenceSetting Geofence;
    stPolygonGeoFenceSetting PolygonGeofence;
    stServiceTypeSetting ServiceType;
    stEncryptTypeSetting EncrryptType;
#if defined(PROTOCOL12)
    stModemActivateSetting ModemActivate;
#endif
#if defined(PROTOCOL15)
    stSensorInitialize SensorInitialize;
#endif
#if defined(PROTOCOL17)
    stUrlSetting SetUrl;
#endif
#if defined(PROTOCOL18)
	stRsvEngCntorl RsvEngCtrl;
#endif

#if defined(FEATURE_EXTENSION_BOARD)
    stAntiThiefSetting stAntiThiefCtl;
#endif
    stTrackingControl TrackingInfo;
}stReportUserActionSetting;

#if defined(PROTOCOL17)
typedef __packed struct __stServerUrl{
    char Url[MAX_SERVER_URL_LENGTH];
    char Path[MAX_SERVER_PATH_LENGTH];
}stServerUrl;
#else
typedef __packed struct __stServerUrl{
    char Url[128];
    char Path[128];
}stServerUrl;
#endif

typedef __packed struct __stUserSetting
{
    uint8_t ucCommandType;
    uint8_t RequestGUID[MAX_GUID_LENGTH];
    stReportUserActionSetting stUserActionSetting;
}stUserSetting;

typedef __packed struct __stPhase1
{
    uint16_t SrandSize;
    uint8_t Srand[64];
    uint16_t HashSize;
    uint8_t Hash[64];
    uint16_t SignSize;
    uint8_t Sign[512];
}stPhase1;

typedef __packed struct __stPhase2
{
    uint16_t KsnSize;
    uint8_t Ksn[11];
    uint16_t EncRandSize;
    uint8_t Enc[512];
}stPhase2;

typedef __packed struct __stIpek
{
    uint16_t IpekSize;
    uint8_t Ipek[145];
}stIpek;

typedef __packed struct __stReportIpek
{
    stPhase1 Phase1;
    stPhase2 Phase2;
    stIpek  Ipek;
}stReportIpek;

typedef __packed struct __stNetworkTime{
    uint32_t unNetWrokTime;
    int16_t sTimeZone;
	int     iTimeGap;
    uint8_t carrApn[MAX_APN_SIZE];
}stNetworkTime;

// this network tims is local time
typedef __packed struct __stModemSetting{
    stNetworkTime stNetworkDate;
    uint8_t carrApn[MAX_APN_SIZE];
}stModemSetting;

typedef __packed struct __stObdSetting{
    uint32_t unOdometer;
    double dlLatitue;
    double dlLongitude;
    uint8_t ucDoorLockStatus;
    uint8_t ucDoorOpenStatus;
    uint8_t ucHeadLight;
    uint8_t carrVin[MAX_VIN_SIZE];
	float fRemainedFuel;
}stObdSetting;

typedef __packed union __stReportSetting{
    stObdSetting ObdSetting;
    stModemSetting ModemSetting;
    stUserSetting UserSetting;
//    stReportIpek IpekSetting;
}stReportSetting;

typedef __packed union __stCarReport{
    stReportInterval    rpInterval;
    stReportAlram       rpAlram;
    stReportSmartKey    rpSmartKey;
    stReportSetting     rpSetting;
#if defined(PROTOCOL24)
	stReportModemStatus rpModemStatus;
#endif
    int8_t buffer[sizeof(stReportInterval)];
}stCarReport;

//////////////////////////////////////////////////////////////////

typedef __packed struct __stMsgHeader
{
    uint16_t id;
    uint16_t event;
    uint8_t subEvent;
    uint8_t result;
    uint16_t seq;
    long long drivingKey;
    uint32_t unTraceMng;
    uint32_t unEventTime;
}stMsgHeader;


/**
 * @brief this struct used with system manager
 */
typedef __packed struct __stMsgSys
{
    stMsgHeader header;
    stReportSetting rpSetting;
#ifndef GLOBAL_SHARE_QUEUE
    int8_t buffer[2];
#else
    int8_t buffer[sizeof(stReportSetting)];
#endif
}stMsgSys;

/**
 * @brief this struct used with sub handler of system manager
 */
typedef __packed struct __stMsgSysSub
{
    stMsgHeader header;
#ifndef GLOBAL_SHARE_QUEUE
    int8_t buffer[2];
#else
	stCarReport carReport;
#endif
}stMsgSysSub;

/**
 * @brief this struct used with sensor handler of system manager
 */
typedef __packed struct __stMsgSysSensor
{
    stMsgHeader header;
#ifndef GLOBAL_SHARE_QUEUE
    int8_t buffer[2];
#else
	stCarReport carReport;
#endif
}stMsgSysSensor;

/**
 * @brief this struct used with sensor manager
 */
typedef __packed struct __stMsgSensor
{
    stMsgHeader header;
#ifndef GLOBAL_SHARE_QUEUE
    int8_t buffer[2];
#else
    stCarReport carReport;
#endif
}stMsgSensor;

/**
 * @brief this struct used with message handler of system manager
 */
typedef __packed struct __stMsgSysMsg
{
    stMsgHeader header;
    stCarReport carReport;
}stMsgSysMsg;

/**
 * @brief this struct used with storage manager
 */
typedef __packed struct __stMsgStorage
{
    stMsgHeader header;
    stCarReport carReport;
}stMsgStorage;

/**
 * @brief this struct used in obd manager
 */
typedef __packed struct __stMsgObd
{
    stMsgHeader header;
    stCarReport carReport;
}stMsgObd;

/**
 * @brief this struct used in modem manager
 */
typedef __packed struct  __stMsgMdm
{
    stMsgHeader header;
    stCarReport carReport;
}stMsgMdm;

/**
 * @brief this struct used with sensor manager
 */
typedef __packed struct __stMsgData
{
    stMsgHeader header;
    stCarReport carReport;
}stMsgData;

typedef __packed struct __stMsgBt
{
    stMsgHeader header;
    stCarReport carReport;
}stMsgBt;

typedef __packed struct __stMsgMdmHandler
{
    stMsgHeader header;
#ifndef GLOBAL_SHARE_QUEUE
    int8_t buffer[32];
#else
	stCarReport carReport;
#endif
}stMsgMdmHandler;

#if defined(FEATURE_EXTENSION_BOARD)
typedef __packed struct __stMsgExtend
{
    stMsgHeader header;
#ifndef GLOBAL_SHARE_QUEUE
    int8_t buffer[512];
#else
	stCarReport carReport;
#endif
}stMsgExtend;
#endif

#endif //__AUTOLINK_MESSAGE_H__

/***************************** END OF FILE ****/
