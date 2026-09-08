/**
  ******************************************************************************
  * @file    GIT_BluetoothLowEnergy.h
  * @author  GIT Diagnosis Software Team by james jean
  * @version V1.1.0
  * @date    29-JAN-2016
  * @brief   Header for BluetoothLowEnery.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GIT_BLUETOOTH_LOW_ENERGYH__
#define __GIT_BLUETOOTH_LOW_ENERGYH__

#include "GIT_Util.h"
#include "common.h"

#define BLE_PACKET_LENGTH_IDX		3
#ifdef BNCOM //mod.pdh 2021.11.29
#define BLE_PACKET_DATA_IDX			0
#else
#define BLE_PACKET_DATA_IDX			4
#endif

#define MAX_SPP_PACKET_SIZE 		25
// dosodma
#define MAX_SPP_STREAM_DATA_BUFF 	1024
//#define MAX_SPP_STREAM_DATA_BUFF 	256

#define BT_ADDRESS_LEN				6
#define MAX_BT_SPP_INSTANCE			1
#define MAX_BT_DEVICE_NAME			32
#define BLUETOOTH_SEND_PACKET_SIZE	(32)


#define MAX_BT_MODULE_VERSION		12
#define DEFAULT_BT_MODULE_VER_STR 	"v_0.0.1.5"


#define BLUETOOTH_PACKET_SIZE 		(512)

//#define USE_BT_SEND_TEST
#ifdef USE_BT_SEND_TEST
#define BT_MONITORING_INTERVAL		(500) // 5msec
#else
#define BT_MONITORING_INTERVAL		(5000) // 5sec
#endif



#define BLE_COMMAND_SUCCESS			(0x01)
#define BLE_COMMAND_FAIL			(0x00)

typedef enum __eAutolinkBluetoothBeaconStatus{
    eAutolinkBluetootBeaconStatus_Sleep = 0,
    eAutolinkBluetootBeaconStatus_Wakeup = 0x10,
}eAutolinkBluetoothBeaconStatus;

typedef enum _eBTCmdIndex
{
#ifdef BNCOM //mod.kks
  	eBLECmd_Min = 0,
	eBLECmd_SetSWReset = eBLECmd_Min,					// BLE Devie Reset
	eBLECmd_SetConfigRestore, 			// BLE 설정값 초기화
	eBLECmd_GetVersion,
	eBLECmd_GetBLE_Name,
	eBLECmd_SetBLE_Name,				// 문자열 이름 1~12 길이 설정 가능
	eBLECmd_BD_ADDR,						// BLE Mac address
	eBLECmd_UartConfig,					// Uart설정 정보
	eBLECmd_Menufacturer,					// 제조사 정보 (3BYTE)
	eBLECmd_TXPWR,						// 송출 파워 정보	10dBM,3dBM, 0dBM
	eBLECmd_ADVInterval,					// Advertising Interval정보
	eBLECmd_ConnInterval,					// Connection Interval 정보
	eBLECmd_ReMote_BD_ADDR,				// 연결과 장치의 Address
	eBLECmd_GetAdvertiseState,				// Get Advertise State
	eBLECmd_SetAdvertiseState,
	eBLECmd_EvtInitNoti,				// BLE Devie Ready
	eBLECmd_EvtReponseOK,					// "OK\r"
	eBLECmd_EvtReponseERROR,				// "ERROR\r"
	eBLECmd_EvtConnected,
	eBLECmd_EvtDisConnected,
	eBLECmd_AT_TEST,						// Test command : "AT\r" -> "OK\r"
	eBLECmd_GetSVCData,
	eBLECmd_SetSVCData,
	eBLECmd_Max,
#else
	eBTCmd_Min,
	eBTCmd_GetBTInitNoti = eBTCmd_Min,
	eBTCmd_DataRestore,
	eBTCmd_ModuleReset,
	eBTCmd_AdvertisingStart,
	eBTCmd_AdvertisingStop,
	eBTCmd_GetModuleState,
	eBTCmd_GetLocalAddress,
	eBTCmd_GetDeviceName,
	eBTCmd_SetDeviceName,
	eBTCmd_GetFirmwareVer,
	eBTCmd_SetOtaActivate,
	eBTCmd_GetNameLength,
	eBTCmd_SetRepeatMode,
	eBTCmd_OffRepeatMode,
	eBTCmd_SetAdvertisingValue,
	eBTCmd_DUT_Mode,	//현재 사용안하는 모드
	eBTCmd_Max
#endif
}eBTCmdIndex;
#pragma pack(push, 1)
typedef __packed struct _BT_ADDR
{
	BYTE btAddr[BT_ADDRESS_LEN];
}BT_ADDR;

typedef enum _BleutoothState
{
	eBT_NONE,
	eBT_initializing,
	eBT_initialized,
	eBT_Ready,
	eBT_Connected,
	eBT_Disconnected,
	eBT_UPGRADE,
	eBT_Sleep_Ready,
	eBT_Sleep_Complete,
}eBleutoothState;

typedef __packed struct _stBluetoothInfo
{
	eBleutoothState 	eBTState;
	BT_ADDR				bt_addr;
	char				strDeviceName[MAX_BT_DEVICE_NAME];
	char				strVersion[MAX_BT_MODULE_VERSION];
}stBTInfo;


typedef __packed struct _stBT_SCAN_DEVICE
{
	BT_ADDR		bt_addr;
	char 	strRemoteDeviceName[MAX_BT_DEVICE_NAME];
}_stBT_SCAN_DEVICE;
#pragma pack(pop,1)

#ifdef BNCOM // mod.pdh 2021.10.22 
void BtEvtAck(uint8_t* pBuffer, uint32_t nLength);
void BtEvtNack(uint8_t* pBuffer, uint32_t nLength);
void BtEvtSleepAck(uint8_t* pBuffer, uint32_t nLength);
void BtEvtFwVersion(uint8_t* pBuffer, uint32_t nLength);
void BtEvtName(uint8_t* pBuffer , uint32_t nLength);
void BtEvtSppTx(uint8_t* pBuffer, uint32_t nLength);
void BtEvtAddress(uint8_t* pBuffer, uint32_t nLength);
void BtEvtRemoteBtAddr(uint8_t* pBuffer, uint32_t nLength);
void BtEvtBleAdvertising(uint8_t* pBuffer, uint32_t nLength);
void BtEvtDevInit(uint8_t* pBuffer, uint32_t nLength);
void BtEvtBleSVCData(uint8_t* pBuffer, uint32_t nLength);
void Make_n_SendToBnComUart(eBTCmdIndex CmdIdx, char *pArgument);

void BTReset(void);
#endif //#ifdef BNCOM // mod.pdh 2021.10.22 

void BT_Initialize(void);
void BT_Spp_Init(void);
BOOL BTGetConnectStatus(void);
void BTModuleReInit(void);
void BTGetFWVersionReq(void);
void BTSetOTAActivateReq(BOOL bActivate);

void BTGetLocalDeviceNameReq(void);
void BTSetLocalDeviceNameReq(void);
void BTGetLocalAddressReq(void);
void BTGetNameLengthReq(void);

void BTSetMonitoringPacketReq();
void BTSetDUTPacketReq(BYTE* pData);
void BTSetRepeatModeReq(U16 usAdveriseTime,U16 usSleepTime);
void BTStartAdvertisingReq();
void BTStopAdvertisingReq();


// parsing function
BOOL BTRecvParsing(BYTE* pData, unsigned int nLength);
void BTSetModuleRestoreRes(BYTE* pData, unsigned int nLength);
void BTSetModuleResetRes(BYTE* pData, unsigned int nLength);
void BTStartAdvertisingRes(BYTE* pData, unsigned int nLength);
void BTStopAdvertisingRes(BYTE* pData, unsigned int nLength);
void BTGetModuleStateRes(BYTE* pData, unsigned int nLength);
void BTGetLocalAddressRes(BYTE* pData, unsigned int nLength);
void BTGetLocalDeviceNameRes(BYTE* pData, unsigned int nLength);
void BTSetLocalDeviceNameRes(BYTE* pData, unsigned int nLength);
void BTGetFWVersionRes(BYTE* pData, unsigned int nLength);
void BTGetModuleInitNoti(BYTE* pData, unsigned int nLength);
void BTSetOTAActivateRes(BYTE* pData, unsigned int nLength);
void BTGetNameLengthRes(BYTE* pData, unsigned int nLength);
void BTSetRepeatModeRes(BYTE* pData, unsigned int nLength);
void BTOffRepeatModeRes(BYTE* pData, unsigned int nLength);
void BTSetAdvertisingVaule(BYTE* pData, unsigned int nLength);
void BTGetDUTPacketRes(BYTE* pData, unsigned int nLength);
void BTSetState(eBleutoothState eBTState);
eBleutoothState BTGetState();
int BTRecvGetLine(stQueue *pQueue);
void BTMake_n_SendPacket(eBTCmdIndex CmdIdx, char* CmdData, unsigned short nCmdDataSize);
void BTSend_VehicleStatus(uint8_t ucVehicle_state);
void BTOffRepeatModeReq(void);
boolean_t FindBtBnComPacket(unsigned char* pBuff,unsigned char* pBtPacketData, unsigned int* punRcvLength);

#endif /* __GIT_BLUETOOTH_LOW_ENERGYH__ */

/***************************** END OF FILE ****/
