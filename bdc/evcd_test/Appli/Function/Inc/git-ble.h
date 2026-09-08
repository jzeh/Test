/**
  ******************************************************************************
  * @file    git-ble.h
  * @author  GIT Diagnosis Software Team by james jean
  * @version V1.1.0
  * @date    29-JAN-2016
  * @brief   Header for git-ble.c module (Bluetooth Low Energy)
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GIT_BLE_H__
#define __GIT_BLE_H__

#include <stdint.h>
#include <stdbool.h>
#include "typedef.h" 
#include <stdlib.h> 

typedef enum {HAL_DISABLE = 0, HAL_ENABLE = !HAL_DISABLE} eHalFunctionalState;
typedef enum {HAL_RESET = 0, HAL_SET = !HAL_RESET} eHalFlagStatus;
typedef enum {HAL_RETURN_SUCCESS = 0, HAL_RETURN_FAIL = (-1)} eHalReturnStatus;
// typedef enum {HAL_ERROR = 0,  HAL_SUCCESS = !HAL_ERROR} eHalErrorStatus;

#define BNCOM
#define BLE_PACKET_LENGTH_IDX		3
#ifdef BNCOM //mod.pdh 2021.11.29
#define BLE_PACKET_DATA_IDX			0
#else
#define BLE_PACKET_DATA_IDX			4
#endif

#define MAX_SPP_PACKET_SIZE 		25
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

typedef unsigned char	BYTE;

typedef enum __eAutolinkBluetoothBeaconStatus{
    eAutolinkBluetootBeaconStatus_Sleep = 0,
    eAutolinkBluetootBeaconStatus_Wakeup = 0x10,
}eAutolinkBluetoothBeaconStatus;

typedef struct _stQUEUE
{
	unsigned int 	uiQueueSize;
	unsigned int 	uiFront;
	unsigned int 	uiRear;
	BYTE*		 	pData;
}stQueue;
typedef enum _eBTCmdIndex
{
  	eBLECmd_Min = 0,
	eBLECmd_SetSWReset = eBLECmd_Min,	// BLE Devie Reset
	eBLECmd_SetConfigRestore, 			// 1: BLE ������ �ʱ�ȭ
	eBLECmd_GetVersion,					// 2: FW Version ����
	eBLECmd_GetBLE_Name,				// 3: BLE Device Name
	eBLECmd_SetBLE_Name,				// 4: ���ڿ� �̸� 1~12 ���� ���� ����
	eBLECmd_BD_ADDR,					// 5: BLE Mac address
	eBLECmd_UartConfig,					// Uart���� ����
	eBLECmd_Menufacturer,				// ������ ���� (3BYTE)
	eBLECmd_TXPWR,						// ���� �Ŀ� ����	10dBM,3dBM, 0dBM
	eBLECmd_ADVInterval,				// Advertising Interval����
	eBLECmd_ConnInterval,				// Connection Interval ����
	eBLECmd_ReMote_BD_ADDR,				// ����� ��ġ�� Address
	eBLECmd_GetAdvertiseState,			// Get Advertise State
	eBLECmd_SetAdvertiseState,	
	eBLECmd_EvtInitNoti,				// 14: BLE Devie Ready
	eBLECmd_EvtReponseOK,				// 15: "OK\r"
	eBLECmd_EvtReponseERROR,			// 16: "ERROR\r"
	eBLECmd_EvtConnected,
	eBLECmd_EvtDisConnected,
	eBLECmd_AT_TEST,					// 17: Test command : "AT\r" -> "OK\r"
	eBLECmd_GetSVCData,
	eBLECmd_SetSVCData,
	eBLECmd_Max,

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

/* 시리얼(광고 이름) 변경 pending — 연결 중 SetSerial 후, 연결 해제(명령 모드) 시 적용 */
void BTMarkDeviceNameDirty(void);
void BTApplyPendingDeviceName(void);

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
// int BTRecvGetLine(stQueue *pQueue);
void BTMake_n_SendPacket(eBTCmdIndex CmdIdx, char* CmdData, unsigned short nCmdDataSize);
void BTSend_VehicleStatus(uint8_t ucVehicle_state);
void BTOffRepeatModeReq(void);
// bool FindBtBnComPacket(unsigned char* pBuff,unsigned char* pBtPacketData, unsigned int* punRcvLength);

void BLE_UART2_PushRx(const uint8_t *data, uint16_t len);
void BLE_ProcessRxQueue(void);
uint16_t BLE_RxCount(void);

// Test Functions
void BT_TestSendCommand(eBTCmdIndex cmdIndex);

// Circular buffer ���� (git-comm.c���� ���ٿ�)
#define BLE_RX_QUEUE_SIZE 2048
extern uint8_t g_ble_rx_q[BLE_RX_QUEUE_SIZE];
extern uint16_t g_ble_rx_head;
extern uint16_t g_ble_rx_tail;


#endif /* __GIT_BLE_H__ */

/***************************** END OF FILE ****/
