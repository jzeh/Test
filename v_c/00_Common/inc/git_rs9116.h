/*----------------------------------------------------------------------
 *   RS9116 Control( wifi, bt )
 *--------------------------------------------------------------------*/
#ifndef	__GIT_RS9116_H__
#define	__GIT_RS9116_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "typedef.h"

#include "rsi_data_types.h"							//! include file to refer data types
#include "rsi_common_apis.h"						//! COMMON include file to refer wlan APIs
#include "rsi_wlan_apis.h"							//! WLAN include file to refer wlan APIs
#include "rsi_socket.h"								//! socket include file to refer socket APIs
#include "rsi_bootup_config.h"
#include "rsi_error.h"								//! Error include files

#include "rsi_driver.h"
#include "rsi_os.h"

//! BT include file to refer BT APIs
#include "rsi_bt_apis.h"
#include "rsi_bt_common_apis.h"
#include "rsi_bt_common.h"
#include "rsi_bt_config.h"
#include "rsi_bt_apis.h"
#include "rsi_bt.h"
#include "rsi_mqtt_client.h"
/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define USE_SSID_SCAN										0
#define MESSAGE_BTAPP_QUEUE_SIZE							10						// To BT_App Thread MessageQ
#define WIFI_AP_NOT_CONNECTED								0
#define WIFI_AP_CONNECTED									1
#define WIFI_AP_NOT_DEFINE									2
#define DEBUG_WIFI_PACKET									0
#define WIFI_RECEIVING_SIGNAL								0x0001
#define WIFI_SENDING_SIGNAL									0x0002
#define OFF													0
#define ON													1
#define RS9116_IPV4_ADDR(a, b, c, d)						((a) | ((b) << 8) | ((c) << 16) | ((uint32_t) (d) << 24))
#define SIGNAL_RS9116_TX_DONE_WAIT							(int)(0x07)
#define GLOBAL_BUFF_LEN                                 	15000
/*---------------------------MQTT---------------------------*/
#define RSI_KEEP_ALIVE_PERIOD                           	0

//#define TEST_MODE
/////////////////////////////////////TEST_MODE//////////////////////////
#ifdef TEST_MODE
//	#define GIT_SSID_NAME		"CHOAP5"
//	#define GIT_PSK_KEY			"homehome"

//	#define GIT_SSID_NAME		"Galaxy Z Fold46284"
//	#define GIT_PSK_KEY			"123456789"

//	#define GIT_SSID_NAME		"GIT_HQ_PUBLIC"
//	#define GIT_PSK_KEY			"Git1auto1@"

//	#define GIT_SSID_NAME		"kks1234"
//	#define GIT_PSK_KEY			"ko2580ko2580"

	#define GIT_SSID_NAME		"LJS"
	#define GIT_PSK_KEY			"123456789a"

//	#define GIT_SSID_NAME		"Galaxy Z Fold46284"
//	#define GIT_PSK_KEY			"123456789"

//	#define GIT_SSID_NAME		"CDP_TEST"
//	#define GIT_PSK_KEY			"12345678"

	//#define DUMMY_AESKEY_USE
	//#define DUMMY_VIN_USE
	#ifdef DUMMY_VIN_USE
		//#define DUMMY_VIN			"KMHJ281ABHU448177"
		//#define DUMMY_VIN			"KMHHB817FPU000258"
   		//#define DUMMY_VIN			"KNAC381BFNA009074"		//EV6
   		//#define DUMMY_VIN			"KMHM541AFPA007489"
		//#define DUMMY_VIN			"KNAM6411BSA070771"		//k8 hybrid
		//#define DUMMY_VIN			"KMHM241AFPA010383"		//Ioniq 6
		//#define DUMMY_VIN			"KMHFB41BBAA513230"		//�׷��� tg
		//#define DUMMY_VIN			"KMHFG41EBCA155228"		//�׷��� tg
		#define DUMMY_VIN			"KMHDT41BBAU937718"		//�׷��� tg
	#endif
#endif
//#define DUMMY_AESKEY_USE
/////////////////////////////////////TEST_MODE//////////////////////////
/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/
typedef enum _bluetooth_connect
{
	BT_INITAILIZE					= 0x00,
	BT_SPP_CONNECT					= 0x01,
	BT_FIND_TRIG					= 0x02,
	BT_TRIG_CONNECT					= 0x03
} bluetooth_connect;

typedef enum
{
	MQTT_MESSAGE_TYPE_HEALTH_CHECK	= 0,
	MQTT_MESSAGE_TYPE_GENERAL		= 1
} MqttMessageType;

typedef enum _connection_state
{
    CONNECTION_INITIALIZE = 0x00,
    CONNECTION_WIFI_CONNECT,
	CONNECTION_WIFI_SCANNING,
    CONNECTION_MQTT_CONNECT,
    CONNECTION_WEBSOCKET_CONNECT,
    CONNECTION_ACTIVE,
	CONNECTION_NOT_CONNECT,
    CONNECTION_ERROR
} connection_state;

typedef struct _SWifiLog
{
	uint8_t		mucWifiButtonStatus;
	uint8_t		mucSSIDInvalid;
	uint8_t		mucIncorrectCntFull;
	uint8_t		mucAPConnectStatus;
	int32_t		muiWifiAutoConnectCnt;
	uint8_t		mucAPConnectInfo;
	int32_t		muiConnectApRetValue;
	uint8_t		mucAPRetryCnt;
	int32_t		muiSetIpRetValue;
	uint8_t		mucSetIpRetryCnt;
	int32_t		muiConnectSocketRetValue;
	uint8_t		mucSocketRetryCnt;
} SWifiLog;

typedef __packed struct
{
    uint8_t		ap_ssid[32];	// �ֱ� ������ AP SSID (�ִ� 31�� + NULL)
    uint8_t		ap_status;		// AP ���� ���� (0: off, 1: on)
    uint8_t		comm_type;		// ����� ��� ���� (0: mqtt, 1: websocket)
    uint32_t	current_time;	// ���� �ð� (Unix timestamp)
} ConnectionLog;

typedef enum {
    COMM_MQTT			= 0,
    COMM_WEBSOCKET		= 1,
    COMM_DISCONNECTED	= 2
} CommType;

/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern uint8_t				g_ucBtConnected;
extern uint8_t				g_ucApConnected;
extern UserIntCallBack_t	call_back;
extern connection_state		connState;
extern uint32_t 			t_len; 
extern uint8_t* 			g_uiSocketSeverIP;
extern uint16_t 			g_uiSocketSeverPort;
extern BOOL					g_bWifiAutoConDisable;
/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern int32_t	initRS9116( void );
extern void		deinitRS9116( void );
extern void		StartRS9116Thread( void );

extern int32_t	ConnectAP( uint8_t *ssid, uint8_t type, uint8_t *psk );
extern int32_t	WifiPacketSend(uint8_t *resp, uint32_t len );
extern int32_t  WebsockPacketSend(uint8_t *resp, uint32_t len);
extern int32_t  DisconnectWLan( void );

extern int32_t	SetIPAddressDHCP( void );
extern int32_t	SetIPAddressStatic( uint32_t ip_addr, uint32_t network_mask, uint32_t gateway );
extern int32_t	connectToWebsocket(void);

extern void 	rsi_bt_app_on_scan_req();
void            data_transfer_complete_callback(int32_t sockID, uint16_t length);
void            rsi_sock_data_tx_done_cb(int32_t sockID, int16_t status, uint16_t total_data_sent);
extern void		ping_response_callback(uint16_t status, const uint8_t *buffer, const uint16_t length);
extern volatile uint32_t g_pingStartTick;
extern volatile uint32_t g_pingEndTick;
extern volatile int16_t  g_pingRspStatus;
void            rsi_bt_scan_handler();
int32_t         Set_Eir_Data(char* name);
void*           memstr(const void* haystack, size_t haystack_len, const void* needle, size_t needle_len);
/*----------------------------------------------------------------------
 *   RSSI Signal Strength Threshold
 *--------------------------------------------------------------------*/
#define RSSI_THRESHOLD_STRONG		(60)
#define RSSI_THRESHOLD_MEDIUM		(75)

extern int8_t RSSI_SetLedIndicator(void);

/* ============================================================
 * RS9116 Test Version persistence (FW_TEST_VERSION_OVERRIDE 전용)
 * firmware.h 의 FW_TEST_VERSION_OVERRIDE 가 정의돼야 활성화됨.
 * ============================================================ */
#ifdef FW_TEST_VERSION_OVERRIDE
#define RS9116_TEST_VERSION_FILE       "/01_Application/WLAN_TEST_VER.txt"
#define RS9116_TEST_VERSION_DEFAULT    "1610.2.10.0.0.5"
#define RS9116_TEST_VERSION_AFTER_UPG  "1610.2.10.0.0.6"
#define RS9116_TEST_VERSION_MAXLEN     20U

void Load_RS9116_TestVersion(uint8_t *buf, uint32_t buflen);
int  Save_RS9116_TestVersion(const char *version);
#endif /* FW_TEST_VERSION_OVERRIDE */

#endif // __GIT_RS9116_H__