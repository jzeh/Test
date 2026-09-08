/**
  ******************************************************************************
  * @file    Share_InterFunction.h
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    19-FEB-2014
  * @brief   Header for Share_InterFunction.h module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SHARE_INTER_FUNCTION_H__
#define __SHARE_INTER_FUNCTION_H__

#include "common.h"
#include "HalHandler.h"
#include "GIT_Util.h"
#include "GIT_Interprotocol.h"
#include "GIT_OemInterface.h"
#include "OBD_Manager.h"
#include "Modem_Manager.h"
#include "CanFD_Defines.h"
//#include "MngModem.h"

///////////////////////////    CAPITAL PROTOCOL   ///////////////////////////////////////////////
/****************************************************************************************************/
/* 		 0  |  1 ~  4  |  5        |  6     | 7 ~ 20    |   21		|  22 23   |  24 ~ n | n+1|	n+2	*/
/* 		sof | ID        | source | dest | MSGdate | Opcode | Length |Payload | CS | Eof 			*/
/* 		sof : Start Of Frame (0x02)																	*/
/* 		ID : 012-2517-4275 -> 1225174275 를  hex 갑으로 저장										*/
/* 		Source, Dest : [통신단말]"T'  [관제서버]'W'   [모바일]'M'									*/
/*		MSGdate : 전송 시간 																		*/
/*		OPcode : 명령 구분 																			*/
/* 		CS : CheckSum field	(HEAD + BODY)															*/
/* 		Eof : End Of Frame(0x03)																	*/
/****************************************************************************************************/

#define MAX_SIZE_DOWNLOADING_BUFF	1024
#define USER_ID_LENGTH					16
#define RESERVED_INDEX_LENGTH	16
#define RESERVED_TIMEINFO_SIZE	12
#define RESERVED_INDEX_MAX			20
#define MAX_MASTER_ID	5


//******************************************************************************
//		Selftest Index List
//******************************************************************************
#define eSELFTEST_SUB_STATE_MODEM_READY				104

#define eSELFTEST_ADC_IDX						0x01
#define eSELFTEST_CURRENT_IDX					0x02
#define eSELFTEST_BUZZ_IDX						0x03
#define eSELFTEST_SENSOR_IDX					0x04
#define eSELFTEST_CAN_IDX						0x05
#define eSELFTEST_MODEM_UART_IDX				0x07
#define eSELFTEST_MODEM_GPS_IDX					0x09
#define eSELFTEST_FLASH_IDX						0x0A
#define eSELFTEST_SLEEP_BLE_CHIP_IDX			0x0E		// Micom Sleep X / BLE Sleep test
//#define eSELFTEST_SLEEP_MICOM_WAKE_BLE_IDX		0x0F		// Micom Sleep O / BLE Wake up
//#define eSELFTEST_SLEEP_MICOM_WAKE_HCAN_IDX		0x10		// Micom Sleep O / High CAN Wake up
//#define eSELFTEST_SLEEP_MICOM_WAKE_LCAN_IDX		0x20		// Micom Sleep O / Low CAN Wake up
//#define eSELFTEST_SLEEP_MICOM_WAKE_LTE_IDX		0x11		// Micom Sleep O / LTE Wake up
//#define eSELFTEST_SLEEP_LTE_MODEM_IDX			0x12		// Micom Sleep X / LTE Sleep test
#define eSELFTEST_LED_IDX						0x13
#define eSELFTEST_RTC_IDX						0x15
#define eSELFTEST_USIM_OPERATE_IDX				0x16

//#define eSELFTEST_LTE_PRX_DRX_IDX				0x18
//#define eSELFTEST_MODEM_GPS_WIRE_IDX			0x19
//#define eSELFTEST_MODEM_GPS_WIRELESS_IDX		0x1A

#define eSELFTEST_CREATE_PUBLIC_KEY_IDX			0x21
#define eSELFTEST_SERIAL_WRITE_READ_IDX			0x22
#define eSELFTEST_PERIODIC_DATA_STOP_IDX		0x33
#define eSELFTEST_BT_VERSION_IDX				0x35
#define eSELFTEST_MODEM_VERSION_IDX				0x36
//#define eSELFTEST_TCP_CMD_IP_URL_IDX			0x37
//#define eSELFTEST_TCP_TEST_IP_URL_IDX			0x38
//#define eSELFTEST_FOTA_URL_IDX				0x39
#define eSELFTEST_USIM_OPEN_IDX					0x3A
#define eSELFTEST_PHONE_NUM_IDX					0x3B
#define eSELFTEST_USB_IDX						0x3C
#define eSELFTEST_ACCBATT_WAKEUP				0x3D

#define eSELFTEST_HIGHCAN1_WAKEUP				0x40
#define eSELFTEST_HIGHCAN2_WAKEUP				0x41
#define eSELFTEST_HIGHCAN3_WAKEUP				0x42
#define eSELFTEST_LOWCAN_WAKEUP					0x43
#define eSELFTEST_BT_WAKEUP						0x44
#define eSELFTEST_SENSOR_WAKEUP					0x45
#define eSELFTEST_MODEM_WAKEUP					0x46
#define eSELFTEST_IG_ON_WAKEUP_IDX				0x47
#define eSELFTEST_MODEMBAUD_SET_IDX				0x48
#define eSELFTEST_SENSOR_WAKEUP_VARI			0x49
#define eSELFTEST_LED_N_BUZZ_IDX				0x4A
#define eSELFTEST_ABR_USIM_REGISTER_IDX			0x4B
#define eSELFTEST_DOM_USIM_REGISTER_IDX			0x4C

#define eSELFTEST_DEFAULT_SERIAL_WRITE_IDX		0x4F

#define eSELFTEST_FWVERSION_WRITE_IDX           0X50
#define eSELFTEST_FWVERSION_WRITE_DEFAULT_IDX   0X51



////////////////////////////   SELFTEST IDX   //////////////////////////////////
#define eSELFTEST_HYPERTEC_MODE_START			0x01
#define eSELFTEST_HYPERTEC_MODE_END				0x02

#define SELFTEST_PASS		0x00
#define SELFTEST_FAIL		0x01

#define SER_NOT_CORRECT_PLS  0x02
#define SER_NOT_CORRECT_ELS  0x03

#define MODEM_POWER_NORMAL	0x00
#define MODEM_POWER_MAX		0x01

#define SENSOR_ALL			0x00
#define SENSOR_MPU6515		0x01
#define SENSOR_ALPU			0x02

#define CAN_HIGH1			0x00
#define CAN_HIGH2			0x01
#define CAN_HIGH3			0x02
#define CAN_LOW1			0x03

#define BLE_SLEEP			0x00
#define BLE_WAKEUP			0x01

#define LED_TOGGLE_ON		0x00
#define LED_TOGGLE_OFF		0x01

#define LED_RED				0x00
#define LED_GREEN			0x01
#define LED_BLUE			0x02
#define LED_ALL				0x03

#define RTC_SETTING			0x01
#define RTC_READING			0x02
#define RTC_FAIL			0x03

#define PENTA_RANDOM_DATA	0x00
#define PENTA_HASH_DATA		0x01
#define PENTA_SIGN_DATA		0x02
#define PENTA_PUBLIC_DATA	0x03

#define SERIAL_WRITE		0x00
#define SERIAL_READ			0x01

#define FLASH_WRITE			0x00
#define FLASH_ERASE			0x01
#define FLASH_READ			0x02
#define FLASH_FORMAT		0x03

#define MODEM_SLEEP			0x00
#define MODEM_WAKEUP		0x01

#define MAIN_SLEEP			0x00

#define LATCH_SETTING		0x00
#define WAKEUP_PIN_READ		0x01

#define MODEM_READ_CCID			0x00
#define MODEM_WRITE_PHONENUM	0x01
#define MODEM_READ_PHONENUM		0x02

#define SELFTEST_CAN1		1
#define SELFTEST_CAN2		2
#define SELFTEST_CAN_MAX	3

#define SELFTEST_TOGGLE_CNT			15
#define SELFTEST_MODEM_CMD_TX_CNT	100
#define SELFTEST_BUZZER_DELAY		4000
#define SELFTEST_MODEM_RES_TIMEOUT		2500
#define SELFTEST_MODEM_POWER_TIMEOUT	30000

#define SELFTEST_MODEM_WAKE_PIN_LOW			1
#define SELFTEST_MODEM_WAKE_PIN_HIGH		0
////////////////////////////////////////////////////////////////////////////////


#define STM32_UNIQUE_ID_ADDR_LENGTH				0x0C

///////////////////////////////////////////////////////////////////////////////

#define FL_DOOR_ULOCK                      4
#define FR_DOOR_ULOCK                      5
#define RL_DOOR_ULOCK                     6
#define RR_DOOR_ULOCK                     7
#define HEAD_LIGHT                              8
#define HAZARD_LAMP                             9
#define HIGH_BEAM                               10
#define BRAKE                                   12

/////////////////////////////////////////////////////////////////////////////////

#define BT_OTC_REQ_INDEX         0
#define BT_COMMAND_ID            1
#define BT_OTC_REQ_CONTROL       3

#define BT_CTRL_COMMAND_TYPE_CHCK          0x8000

#define	BT_CTRL_REQ_INDEX                 0
#define	BT_CTRL_REQ_COMMAND               1
#define	BT_CTRL_REQ_CONTROL               3
#define	BT_CTRL_REQ_DURATION_TIME         4
#define	BT_CTRL_REQ_TEMP                  5
#define	BT_CTRL_REQ_DEFROST               7
#define BT_CTRL_REQ_BT_CTRL_KEY           8

#define BT_CTRL_ENGINE_START           0x0001
#define BT_CTRL_DOOR                   0x0002
#define BT_CTRL_HAZARD_LAMP            0x0004
#define BT_CTRL_HORN_HAZRARD_LAMP      0x0005

#define MAX_BT_SESSION_INDEX_ENGTH 4
#define BLUETOOTH_HEADER_EXCLUE_LENGTH 6
#define MAX_BASEKEY_SIZE                  32

#define SESSION_INDEX_1 3 // == 4 array index 3 because array start index 0
#define SESSION_INDEX_2 7 // == 8 array index 3 because array start index 0
#define SESSION_INDEX_3 15 // == 16 array index 3 because array start index 0
#define SESSION_INDEX_4 31 // == 32 array index 3 because array start index 0

///////////////////////////////////////////////////////////////////////////////////


#pragma pack(push, 1)
typedef struct _stDownloadStartReq
{
	INT8U DB_Name[MAX_FW_DB_FILE_NAME];	/* 모듈에 저장해야할 db name */
	INT32U DB_Size;											/* DB의 사이즈 정보 */
	INT16U DB_Ver;											// DB 버전 또는 F/W버전 정보를 전달
	INT16U nCheckSum; 									// 전달할 db  또는 f/w의 checksum (파일 사이즈에 대한 모든 byte의 합)
} stDownloadStartReq;

typedef struct _stDownloadFileReq
{
	INT8U DB_Name[MAX_FW_DB_FILE_NAME];					/* 모듈에 저당해야할 db name */
	INT32U DB_Size;						/* DB의 사이즈 정보 */
	INT16U DB_Ver;						/* DB 버전 또는 F/W버전 정보를 전달 */
	INT16U nCheckSum; 					/* 전달할 db  또는 f/w의 checksum (파일 사이즈에 대한 모든 byte의 합) */
}stDownloadFileReq;
#pragma pack(pop,1)

typedef struct _stDownloadingReq
{
	INT16U Frame_Size;				/* 전송하는 Frame Size */
	INT8U DB_Data[MAX_SIZE_DOWNLOADING_BUFF];	/* 최대 500byte */
}stDownloadingReq;

typedef enum _eLockState
{
	eLOCK_STATE_INIT,
	eLOCK_STATE_LOCK,
	eLOCK_STATE_UNLOCK,
	eLOCK_STATE_MAX
}eLockState;

typedef enum _eBTLockState
{
	eBT_LOCK_STATE_LOCK,
	eBT_LOCK_STATE_UNLOCK,
	eBT_LOCK_STATE_REBOOTUNLOCK,
	eBT_LOCK_STATE_MAX
}eBTLockState;

typedef enum _eSavePtclState
{
	ePTCL_BT_STATE,
	ePTCL_GIT_STATE,
	ePTCL_STATE_MAX
}eSavePtclState;

typedef enum _eOP_CODE
{
	eOPC_INIT,
	eOPC_RESERV_ADD = 0x10,
	eOPC_RESERV_CHG,
	eOPC_RESERV_DEL,
	eOPC_DELIVERY_YN,
	eOPC_SMART_LAMP,
	eOPC_SMART_DOOR,
	eOPC_SMART_HORN,
	eOPC_PERIOD_CYCLE,
	eOPC_SETUP_OVERSPEED,
	eOPC_SETUP_LOWBATT,
	eOPC_SETUP_HIGHBATT = 0x20,
	eOPC_MASTER_REG,
	eOPC_MASTER_CHG,
	eOPC_MASTER_DEL,
	eOPC_MASTER_SEARCH,
	eOPC_MASTER_ALLDEL,
	eOPC_SETUP_ALARM,
	eOPC_SETUP_RESET,
	eOPC_BT_MAC_INFO,
	eOPC_VEHICLE_INFO,
	eOPC_RESERV_RES = 0x50,
	eOPC_TRIP_DATA,
	eOPC_LOCAL_PERIODIC,
	eOPC_MASTER_RES,
	eOPC_EVENT_ALARM,
	eOPC_HIPASS_CHARGE,
	eOPC_ACK_MESSAGE,
	eOPC_UPDATE_REPORT,
	eOPC_BT_ADDRESS,
	eOPC_MAX,
}eOP_CODE;

typedef enum _eServerCommType
{
	eSERVER_HeartBeatSet,
	eSERVER_CONTROL,
	eSERVER_HeartBeat=3,
	eSERVER_PERIOD = 'D',
	eSERVER_SENDCC = 4,
	eSERVER_TYPE_MAX
}eServerCommType;

typedef enum _eAlarmMask
{
	eALARM_SystemReboot,
	eALARM_HipassComm,
	eALARM_EVCarSlot,
	eALARM_GPSComm,
	eALARM_HipassCard,
	eALARM_OverVoltage,
	eALARM_LowVoltage,
	eALARM_DoorOpen,
	eALARM_OverTemperature,
	eALARM_FuelRunout,
	eALARM_LowTirePress,
	eALARM_OverSpeed,
	eALARM_OverRpm,
	eALARM_3rdGear,
	eALARM_ACCCheck,
	eALARM_TailLamp,
	eALARM_TYPE_MAX
}eAlarmMask;

typedef enum _eControlVehicleType
{
	eNum_ControlLamp = 0,
	eNum_ControlDoor,
	eNum_ControlHorn,
	eNum_ControlMax
}eControlVehicleType;

#pragma pack(push, 1)
typedef struct _stGIT_INTER_FUNCTIONS
{
	unsigned short int uiFunctionID;
	pfnParsingPayLoadCB fnPayloadCB;

}stGIT_INTER_FUNCTIONS;

typedef struct _stBT_INTER_FUNCTIONS
{
	unsigned short int uiFunctionID;
	pfnParsingPayLoadCB fnPayloadCB;

}stBT_INTER_FUNCTIONS;

#if defined(PROTOCOL18)
typedef struct _stOTC_PTCL_PAYLOAD
{
	U8   ucIndex;
	U16  usCommand;
	U8   ucControl;
	U8   ucResult;
	U64  ulReason;
	U8   ucEngineStatus;
	U16  usVehicleStatus;
	U8 ucChecksum;
	U8  ucReserved[3];

}stOTC_PTCL_PAYLOAD; //dahae

typedef struct _stAIRCON_PTCL_PAYLOAD
{
	U8   ucDBCheck;
	U8   ucLowCheck;
	U8   ucHighCheck;
	U16  usMinTemp;
	U16  usMaxTemp;

}stAIRCON_PTCL_PAYLOAD; //dahae

typedef struct _stRemote_PTCL_PAYLOAD
{
	U8   ucIndex;
	U16  usCommand;
	U8   ucControl;
	U8   ucResult;
#if defined(REASON_8BYTE)
	U64  ullReason;
#else
	U64  ulReason;
#endif
	U8   ucEngineStatus;
	U16  usVehicleStatus;
	U8   ucBTControlKey[16];
	U8 ucChecksum;
	U8  ucReserved[3];

}stRemote_PTCL_PAYLOAD; //dahae

typedef struct _stREQ_CTRL_PAYLOAD
{
	U8   ucIndex;
	U16  usCommand;
	U8   ucControl;
	U8   ucDurationTime;
	U16  usTemperature;
	U8   ucDefrost;
	U8   ucBTControlKey[16];
	U8   ucCheckSum;
	U8  ucReserved[3];

}stREQ_CTRL_PAYLOAD;
#endif


typedef struct _stVEHICLESTATUS_PTCL_PAYLOAD
{
	U8   ucEngineStatus;
	U16  usVehicleStatus;
	U32  uiReserved;
}stVEHICLESTATUS_PTCL_PAYLOAD; //dahae


typedef struct _stCAP_PTCL_PAYLOAD		//big endian
{
	U8	ucSTX;
	U8	arrPhoneNumber[4];
	U8	ucSource;
	U8	ucDestination;
	U8	arrSendDate[14];
	eOP_CODE eOperationCode;
	U8	DataLength[2];
	U8	pPayload[MAX_MODEM_PROTO_DATA_LENGTH];
	//U8	*pPayload;
} stCAP_PTCL_PAYLOAD;

typedef void			(*pfnPayLoadCB)(stCAP_PTCL_PAYLOAD *pInterPtcl, unsigned short usDataLength);
typedef struct _stCAP_INTER_FUNCTIONS
{
	eOP_CODE eOperationCode;
	pfnPayLoadCB fnPayloadCB;

}stCAP_INTER_FUNCTIONS;

typedef struct _stHeartBeat
{
	U8	ucCode;
	U8	ucTryMax;
	U16	usTryTimeout;
}stHeartBeat;

typedef struct _stReservInfo
{
	U8	arrIndex[RESERVED_INDEX_LENGTH];
	U8	arrUserID[USER_ID_LENGTH];
	U8	ucStartTime[RESERVED_TIMEINFO_SIZE];
	U8	ucEndTime[RESERVED_TIMEINFO_SIZE];
	U8	ucWellcome;
	U8	ucUsed;
}stReservInfo;

//typedef struct _stMasterInfo
//{
//	U8	ucIndex;
//	U8	arrUserID[USER_ID_LENGTH];
//}stMasterInfo;

typedef struct _stBTBaseKeyInfo
{
	U8 ucBTSessionIndex[MAX_BT_SESSION_INDEX_ENGTH];
	U8 ucBTBaseKey[MAX_BASEKEY_SIZE];
}stBTBaseKeyInfo;

typedef struct _stDeviceInfo
{
	U8	ucCanState;
	U8	ucModemState[6];
	U8	ucGPSState;
	U8	ucHiPassState;
	U8	ucBTState;
}stDeviceInfo;

typedef enum _ePeriodInterval
{
	ePeriod_KeyOff,
	ePeriod_KeyOn,
	ePeriod_Trip,
	ePeriod_Delivery,
	ePeriod_Max
}ePeriodInterval;

typedef struct _stSetReqRes
{
  	INT16U Period_time;
	INT16U Period_Packet_t[ePeriod_Max];			/* 주기보고 시간 : sec 단위 */
}stSetReqRes;

typedef struct _stRefEventInfo
{
	U8  ucOverSpeedRef;
	U8  ucOverSpeedRef_LowLimit;
	U8  ucNormalVoltage;
	U8  ucLowVoltageRef;
	U8  ucHighVoltageRef;
	U8  ucHighVoltageRef_HighLimit;
	U8  arrAlarmMaskRef[eALARM_TYPE_MAX];
}stRefEventInfo;

typedef enum _eAlarmMaskList
{
	eAlarmList_SystemReboot = 0,
	eAlarmList_HiPass,
	eAlarmList_EVSlot,
	eAlarmList_Gps,
	eAlarmList_OilCard,
	eAlarmList_OverVoltage,
	eAlarmList_LowVoltage,
	eAlarmList_DoorOpen,
	eAlarmList_VehicleOverTemp,
	eAlarmList_FuelRunOut,
	eAlarmList_LowTire,
	eAlarmList_OverSpeed,
	eAlarmList_OverRpm,
	eAlarmList_ThirdTransmission,
	eAlarmList_EngineStart,
	eAlarmList_TailLamp
}eAlarmMaskList;

typedef enum _eDriverPerson
{
	eDriver_None = 0,
	eDriver_Delivery,
	eDriver_Custom,
	eDriver_PickUp
}eDriverPerson;

typedef enum _eWakeUpPinState
{
	eWAKEUP_PIN_STATE_PA0,
	eWAKEUP_PIN_STATE_CAN_RX,
	eWAKEUP_PIN_STATE_LOW_HIGH_CAN_RX,
	eWAKEUP_PIN_STATE_BT_MON,
	eWAKEUP_PIN_STATE_SENSOR_INT,
	eWAKEUP_PIN_STATE_MO_WAKE,
	eWAKEUP_PIN_STATE_IG_ON_DET,
	eWAKEUP_PIN_STATE_ACC_DET,
	eCTRL_TYPE_MAX
}eWakeUpPinState;

typedef enum _eSelftestModemManagerState
{
	eSELFTEST_MD_SET_CMD = 1,
	eSELFTEST_MD_GET_CMD_RES,
	eSELFTEST_MD_SEND_CMD,
	eSELFTEST_MD_RECEIVE_CMD_RES,
	eSELFTEST_MD_MAX
}eSelftestModemManagerState;

typedef struct _stGpsInfo
{
	INT8U ucValidData;
	float uflongitude;              //경도
	float ulatitude ;                //경도
	int  ucSatelNum;
	int  ucGpsYear;		//년
	int  ucGpsMonth;	//월
	int  ucGpsDay;		//일
  int  ucGpshour;       /**< Hours since midnight - [0,23] */
  int  ucGpsmin;        /**< Minutes after the hour - [0,59] */
  int  ucGpssec;        /**< Seconds after the minute - [0,59] */
	U8   ucGpsYYMMSS[14];
}stGpsInfo;

typedef struct _stCanFDInfoForBT
{
	eCFD_STATUS_BT eCFDStatus;
	unsigned short usBootVer;
	unsigned short usAppVer;
	unsigned char ucProgress;
}stCanFDInfoForBT;

typedef enum _eINSTALL_FILE_TYPE
{
    eINSTALL_FILE_TYPE_NONE = 0,
    eINSTALL_FILE_TYPE_BOOT,
    eINSTALL_FILE_TYPE_APP,
	eINSTALL_FILE_TYPE_MAX,
}eINSTALL_FILE_TYPE;

typedef __packed struct _stCANFD_BT_UPDATE_INFO
{
	eINSTALL_FILE_TYPE eInstallFileType;
	uint32_t nDestAddress;
	uint32_t nProgressAddress;
	stDownloadStartReq stFileInfo;
}stCANFD_BT_UPDATE_INFO;

//typedef struct _stSaveOBDInfo
//{
//	INT32U 	m_nOdometer_t;		/* 총이동 거리(TRIP) */
//	INT32U 	m_nOdometer_rev;		/* 예약자 이동거리 */
//	float 	m_fFuel_vehicle;		/* 총 연료소모량 */
//	float 	m_fFuel_rev;				/* 예약자 연료소모량 */
//	U8		arrUserID[USER_ID_LENGTH];
//	BYTE	m_strVehicleCode[MAX_VEHICLECODE_SIZE];
//}stSaveOBDInfo;
#pragma pack(pop,1)

void Git_FWUpdate_Start(void *pInterPtcl, unsigned int eInCommType);
void Git_FWUpdate_Recv(void *pInterPtcl, unsigned int eInCommType);
void Git_FWUpdate_End(void *pInterPtcl, unsigned int eInCommType);
void Git_DeviceReset(void *pInterPtcl, unsigned int eInCommType);
void DCS_DB_INFO_Res(void *pInterPtcl, unsigned int eInCommType);
void CANFD_Info_Res(void *pInterPtcl, unsigned int eInCommType);
void BTGetAutoVIN(void *pInterPtcl, unsigned int eInCommType);
void SendGITPtclResponse_DTC();
void SendGITPtclBTDisconnect();
void SendGITPtclResponse_fast();
void SendGITPtclResponse_slow1();
void SendGITPtclResponse_slow2();
void SendGITPtclResponse_data();

void ProcessGITInterProtocol(stGIT_PTCL_PAYLOAD *pInterPtcl, eCommType eWhatCommType);
void ProcessBTInterProtocol(stBT_PTCL_PAYLOAD *pInterPtcl, eCommType eWhatCommType);


//BLE MODULE
void BTGetInfoRes(void *pInterPtcl, unsigned int eInCommType);
void BTSetReserveIndexRes(void *pInterPtcl, unsigned int eInCommType);
void BTGetSecurityPassRes(void *pInterPtcl, unsigned int eInCommType);
void BTSetDoorControlRes(void *pInterPtcl, unsigned int eInCommType);
void BTSetRebootRes(void *pInterPtcl, unsigned int eInCommType);
void BTGetAccRes(void *pInterPtcl, unsigned int eInCommType);
void BTSetTestInfoRes(void *pInterPtcl, unsigned int eInCommType);
void BTGetDeviceInfoRes(void *pInterPtcl, unsigned int eInCommType);
void BTLockVinRes(void *pInterPtcl, unsigned int eInCommType);
void BTUnLockVinRes(void *pInterPtcl, unsigned int eInCommType);
void BTCompleteRes(void *pInterPtcl, unsigned int eInCommType);
void BTApnSettingRes(void *pInterPtcl, unsigned int eInCommType);
void BTSetCARType(void *pInterPtcl, unsigned int eInCommType);
void BTCheckModemWakeup(void *pInterPtcl, unsigned int eInCommType);
//void BTGetModuleLog(void *pInterPtcl, unsigned int eInCommType);
//void BTGetModuleLogDataSize(void *pInterPtcl, unsigned int eInCommType);
#if defined(PROTOCOL25)
void BTInstallationNetworkFailCode(void *pInterPtcl, unsigned int eInCommType);
void BTGetInstallationNetworkCheck(void *pInterPtcl, unsigned int eInCommType);
#endif

#if defined(PROTOCOL18)
//OTC, REMOTE CONTROL
void BTOTCnRemoteCtrl(void * pInterPtcl,unsigned int eInCommType); //dahae
void BTGetDeviceTime(void * pInterPtcl,unsigned int eInCommType); //dahae
void Send2BTOTCResPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stOTC_PTCL_PAYLOAD *pstOTCPayload, unsigned char ucControlID, unsigned int eInCommType);
void Send2BTRemoteCtrlResPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stRemote_PTCL_PAYLOAD *pstRemoteCtrlPayload, unsigned short usCommandID, unsigned int eInCommType);

unsigned short int Get_OTCVehicleStatus(void);

//공조 데이터
void BTAirConStatusRes(void *pInterPtcl, unsigned int eInCommType); //dahae

void BTCtrlFuncSettingInfo(void *pInterPtcl, unsigned int eInCommType);
#endif

//FW Download
void BTFWUpdateRes(void *pInterPtcl, unsigned int eInCommType);           //FW, DB 업데이트 시작
void BTFWUpdateRecv(void *pInterPtcl, unsigned int eInCommType);
void BTFWUpdateEnd(void *pInterPtcl, unsigned int eInCommType);
void BTFWDeviceReset(void *pInterPtcl, unsigned int eInCommType);
void BTTERMINALInfoRes(void *pInterPtcl, unsigned int eInCommType);
void BTSetGPSInfo(void *pInterPtcl, unsigned int eInCommType);
void GitFWModeChange(void *pInterPtcl, unsigned int eInCommType);
void BTSetRtcTime(void *pInterPtcl, unsigned int eInCommType);
void BTSetAutolinkPType(void *pInterPtcl, unsigned int eInCommType);
void BTGetCanFDAdaptorType(void *pInterPtcl, unsigned int eInCommType);

//FW Download End

//생산 프로그램
void BTDoor_UnlockRes(void *pInterPtcl, unsigned int eInCommType);
void BTDoor_LockRes(void *pInterPtcl, unsigned int eInCommType);
void BTSensor_InitRes(void *pInterPtcl, unsigned int eInCommType);
void BTDoor_UnlockStatusRes(void *pInterPtcl, unsigned int eInCommType);
void BTDoor_LockStatusRes(void *pInterPtcl, unsigned int eInCommType);
void BTEngine_StatusRes(void *pInterPtcl, unsigned int eInCommType);
//생산 프로그램 End

void Git_MainBoard_Test(void *pInterPtcl, unsigned int eInCommType);
void Git_ModemBoard_Test(void *pInterPtcl, unsigned int eInCommType);
void RTC_SetTimeRegulate(unsigned char ucYear,unsigned char ucMonth,unsigned char ucDate,unsigned char ucHour,unsigned char ucMinute,unsigned char ucSecnd);
void GitReadSerial(void *pInterPtcl, unsigned int eInCommType);
void GitReadHiddenSerial(void *pInterPtcl, unsigned int eInCommType);
void Git_FWModeChange(void *pInterPtcl, unsigned int eInCommType);
void GitPassThruDisconnect(void *pInterPtcl, unsigned int eInCommType);

void HandleSYSInfo(void *pInterPtcl, unsigned int eInCommType);
void HandleGPSInfo(void *pInterPtcl, unsigned int eInCommType);
void HandleSmokeInfo(void *pInterPtcl, unsigned int eInCommType);
void HandleSmartCardInfo(void *pInterPtcl, unsigned int eInCommType);
void HandleRFIDInfo(void *pInterPtcl, unsigned int eInCommType);
void HandleBLEState(void *pInterPtcl, unsigned int eInCommType);
void HandleBLERemodeAddr(void *pInterPtcl, unsigned int eInCommType);
void HandleBLERecvedData(void *pInterPtcl, unsigned int eInCommType);
void HandleBLECmdResBoardAddr(void *pInterPtcl, unsigned int eInCommType);
void HandleBLECmdResRemoteBoardAddr(void *pInterPtcl, unsigned int eInCommType);
void HandleBLECmdResRefuse(void *pInterPtcl, unsigned int eInCommType);
void HandleBLECmdResSendData(void *pInterPtcl, unsigned int eInCommType);
void HandleBLECmdResAdvControl(void *pInterPtcl, unsigned int eInCommType);
void HandleSYSCmdResSerialNo(void *pInterPtcl, unsigned int eInCommType);
void HandleSYSCmdResFirmwareVer(void *pInterPtcl, unsigned int eInCommType);
void HandleLEDCmdResSetModeCfg(void *pInterPtcl, unsigned int eInCommType);
void HandleLEDCmdResGetModeCfg(void *pInterPtcl, unsigned int eInCommType);
void HandleLEDCmdResOnOff(void *pInterPtcl, unsigned int eInCommType);
void HandleLEDCmdStatus(void *pInterPtcl, unsigned int eInCommType);
void HandleBEEPCmdResSetModeCfg(void *pInterPtcl, unsigned int eInCommType);
void HandleBEEPCmdResGetModeCfg(void *pInterPtcl, unsigned int eInCommType);
void HandleBEEPCmdResOnOff(void *pInterPtcl, unsigned int eInCommType);
void HandleBEEPCmdResStatus(void *pInterPtcl, unsigned int eInCommType);

void DeviceTimerCallBack();

int ReservedIDSearch(U8 *arrUserID,U8 *arrReservedNumber);

void KTServerParsing(U8* pData, U16 usDataLength);

void ManageReserveInfo();
void GetConvertFrontTime(U8 *pTimeBuffer, int nTime);
bool ExistSameReserveID(char *pTempPayLoad);
void CB_TimeOutCallback_BT();

void GitGetCertifyVCI(void *pInterPtcl, unsigned int eInCommType);
void Git_GetCertifyVCI(void *pInterPtcl, unsigned int eInCommType);
boolean_t IsPremiumCertificationCorrect(uint8_t* pucArrPayload, uint8_t ucPayloadLength);
boolean_t IsBasicCertificationCorrect(uint8_t* pucArrPayload, uint8_t ucPayloadLength);
void GetAESDecriptBluetoothData(uint8_t* pucArrSrc, uint8_t ucSrcLength, uint8_t* pucArrDst);
void GitGetCurrentVCIMode(void *pInterPtcl, unsigned int eInCommType);
void Git_FWUpdate_Recv(void *pInterPtcl, unsigned int eInCommType);
void Git_DeviceReset(void *pInterPtcl, unsigned int eInCommType);
void AutoVIN();

unsigned char HextoUpper(unsigned char Data);
bool makeEncryptionKey(unsigned char *arrSeedKey, unsigned char *arrModuleSerial);
unsigned short ModuleKeyDecryption(unsigned char *pMessage, unsigned char *arrModuleKey, unsigned short nLen);
bool SendModemCommand(modem_process_fn processFunction);
void MakeRandomKey(unsigned char*);
void MakeBTBaseKey();
void ClearBTBaseKey();
void GetBTBaseKey(uint8_t * pKey);
bool SecurityCheck(unsigned char *);
bool GetActiveRemoteControl(unsigned char* ucStrItem, unsigned char unLength);
void BT_EncryptRemoteControl(void *pInterPtcl, unsigned int eInCommType);
boolean_t DecryptBluetoothRemoteControlData(uint8_t* pucSrc, uint8_t ucSrcLength, stREQ_CTRL_PAYLOAD* pstREQ_CTRL_PAYLOAD);
void GetBluetoothNewKey(uint8_t* pucArrNewKey);
void SetBluetoothNewIndex(uint8_t* ucArrKeyIndex);
uint16_t CalCheckSum(uint8_t* pcArrBuffer,int32_t nLength);
void GetAESEncryptBluetoothData(uint8_t* pucArrSrc, uint8_t ucSrcLength, uint8_t* pucArrDst);
boolean_t EncryptBluetoothRemoteControlData(uint8_t* pucSrc, uint8_t ucSrcLength, uint8_t* pucDst, uint8_t* ucSize);
void Send2BTOTCResEncPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stOTC_PTCL_PAYLOAD *pstOTCPayload, unsigned char ucControlID, unsigned int eInCommType);
void Send2BTRemoteCtrlResEncPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stRemote_PTCL_PAYLOAD *pstRemoteCtrlPayload, unsigned short usCommandID, unsigned int eInCommType);
bool DownloadClose_CFD(void);
void Send_IGStatus_APP(unsigned char ucVehicleStatus);

#if defined(QI_DEVELOPE)
void BTSetServiceType(void *pInterPtcl, unsigned int eInCommType);
void BTCheckURL(void *pInterPtcl, unsigned int eInCommType);
void BTChangeURL(void *pInterPtcl, unsigned int eInCommType);
void BTChangeSerialNumber(void *pInterPtcl, unsigned int eInCommType);
void BTDeleteVin(void *pInterPtcl, unsigned int eInCommType);
void BTCheckVersion(void *pInterPtcl, unsigned int eInCommType);
void BTChageVersion(void *pInterPtcl, unsigned int eInCommType);
void BTModuleInit(void *pInterPtcl, unsigned int eInCommType);
void BTCheckSerialNumber(void *pInterPtcl, unsigned int eInCommType);
#endif

#if 0 
void SetSendingCount(unsigned char ucCnt);
unsigned char GetSendingCount(void);
void SetSendingBytes(unsigned short usSendingBytes);
unsigned short GetSendingBytes(void);

unsigned char SetFileSendingCount(unsigned short usSize, unsigned short usSendingBytes);
unsigned short SetDataFunction(eMODEM_ERROR_STATE eErrorState, unsigned char ucDataIndex, char *pModemFileData);
#endif
#endif /* __GIT_INTER_PROTOCOL_H__ */

/***************************** END OF FILE ****/

