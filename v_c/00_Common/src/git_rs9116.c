/*************************************************************
 * NOTE : git_rs9116.c
 *      wifi/bt moudle contol
 * Author : Lee junho
 * Since : 2019.12.18
**************************************************************/
#include <string.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "common.h"
#include "firmware.h"
#include "git_protocol.h"
#include "sw_timer.h"
#include "buzzer.h"
#include "git_fsutil.h"
#include "git_rtc.h"
#include "git_ioctl.h"
#include "git_function_list.h"

#ifdef VCI3_RECORD
#include "git_trigger.h"
#endif

#include "git_global.h"
#include "git_rs9116.h"
#include "git_mqtt_Topic.h"
#include "git_cli.h"
#include "led.h"
#include "rsi_mqtt_client.h"
#include <rsi_socket.h>
#include "rsi_utils.h"
#include "time.h"
#include "rsi_wlan_config.h"
#include "rsi_bt_common_apis.h"
#include "Chain_RootCA_Bundle.h"
//#include "Wildcard.gitauto.com_pem_ca_chain.h"
//#include "Wildcard.gitauto.com_pem_server_cert.h"
//#define ENABLE_BT_LOG

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
/* CA 미로드 테스트: 0 → 포트 8883 접속 시 0xBBD2 나오면 RS9116이 CA 검증 수행 확인 / 1: 정상 운용 */
#define LOAD_CERTIFICATE								1
/* 훼손된 CA 인증서 로드 테스트: 1 → 잘못된 cert로 TLS 시도
 * 결과 해석: 0xBBD2=RS9116이 cert 서명 검증 후 실패 / 0=검증 안 함(암호화만 수행) */
#define TEST_CORRUPT_CA_CERT							0
#define WIFI_MAX_TX_LEN                                 1460
#define WIFI_MAX_RX_LEN                                 1460
#define LOG_START_ADDRESS								0x410		// Log start save address
#define LOG_END_ADDRESS									0xFFF		// Log end address
#define LAST_ADDRESS_OFFSET								0x40E		// Log address save location (0x40E ~ 0x40F)
//#define DEBUG_BT_PACKET
//#define CVCI_201_TEST

//! Memory Length for driver
//#define	BT_GLOBAL_BUFF_LEN                          10000

#define RSI_DRIVER_TASK_PRIORITY                        3
#define RSI_DRIVER_TASK_STACK_SIZE                      3000

#define RS9116_RETRY_COUNT						    	5

/*   Bluetooth                                          */
/////////////// application events list //////////////////
// number means priority.
#define RSI_APP_EVENT_CONNECTED							1

/*** PAIRING RELATED DEFINES********/
#define RSI_APP_EVENT_PINCODE_REQ						2
#define RSI_APP_EVENT_LINKKEY_SAVE						3
#define RSI_APP_EVENT_AUTH_COMPLT						4
#define RSI_APP_EVENT_LINKKEY_REQ						5

/** ssp related defines********/
#define RSI_APP_EVENT_PASSKEY_DISPLAY					8
#define RSI_APP_EVENT_PASSKEY_REQUEST					9
#define RSI_APP_EVENT_SSP_COMPLETE						10
#define RSI_APP_EVENT_CONFIRM_REQUEST					11

/*** SPP RELATED DEFINES********/
#define RSI_APP_EVENT_SPP_CONN							15
#define RSI_APP_EVENT_SPP_TX							16
#define RSI_APP_EVENT_SPP_RX							17

/*** SNIFF RELATED DEFINES********/
#define RSI_APP_EVENT_MODE_CHANGED						20
#define RSI_APP_EVENT_SNIFF_SUBRATING 					21

/*** SCAN RELATED DEFINES********/
#define RSI_APP_EVENT_SCAN_RESP							24
#define RSI_APP_EVENT_SCAN_REQ							25
#define RSI_APP_EVENT_SCAN_AGAIN						26
#define RSI_APP_EVENT_SCAN_TIMEOUT						27

#define RSI_APP_EVENT_NAME_REQ							28
#define RSI_APP_EVENT_NAME_RESP							29


/*** DISCONNECT RELATED DEFINES********/
#define RSI_APP_EVENT_SPP_DISCONN						30
#define RSI_APP_EVENT_DISCONNECTED						31

#define RSI_APP_EVENT_MAX								32
//////////////////////////////////////////////////////////

/*   Wifi                                                             */
//!Scan Channel number , 0 - to scan all channels
#define CHANNEL_NO										0

//!TCP Max retries
#define RSI_MAX_TCP_RETRIES                             10
#define RSI_MAX_TCP_TIMEOUT                             10000
//! Number of packet to send or receive
#define NUMBER_OF_PACKETS								100
#define NUMBER_OF_PACKETS_FACTORY						1

//! Inquiry Scan timeout set
#define SCAN_TIMEOUT									10000
#define SCAN_DEVICE										10

//! Power Save Profile Mode
#define PSP_TYPE										RSI_MAX_PSP

//! Sniff Parameters
#define SNIFF_MAX_INTERVAL								0xA0
#define SNIFF_MIN_INTERVAL								0XA0
#define SNIFF_ATTEMPT									0X04
#define SNIFF_TIME_OUT									0X02

#define RSI_BT_LOCAL_NAME								"VCI_III"
#define PIN_CODE										"0000"

#define SLAVE_NAME										"TRIG_"

#define BLUETOOTH_APP_SET_SIGNAL                        0x0001
#define SERVER_NAME                                     "ws://43.203.212.167" // WebSocket server name
#define RESOURCE_NAME                                   "/websocket"  

/*                  WEBSOCKET                    */
uint8_t ip_buff[20];
extern struct rsi_sockaddr_in	client_addr;


/*                  MQTT                    */
//!Memory to initialize MQTT client Info structure
#define MQTT_SERVER_IP_ADDRESS							"3.34.72.199"
#define MQTT_SERVER_PORT                                1883
#define MQTT_CLIENT_PORT                                1883
#define RSI_KEEP_ALIVE_PERIOD                           0
#define QOS                                             0
#define MQTT_Connect_Retry_count						5

uint8_t mqtt_url_name[]									= "mq.git-connect.com";
uint8_t websocket_url_name[]							= "ws.git-connect.com";
int8_t MQTT_USERNAME[]									= "cdpvci";
int8_t MQTT_PASSWORD[]									= "cdpvci!@";

connection_state connState = CONNECTION_INITIALIZE;


#define PRIMARY_DNS_SERVER		NULL  // NULL when using DHCP
#define SECONDARY_DNS_SERVER	NULL
//#define RSI_MQTT_TOPIC "cloud.diag123"
#define RSI_MQTT_TOPIC "/CDP/A001/Server"


//! Server port number
#if defined(ENABLE_WIFI_SSL_TLS_CERTIFICATE)
#define SERVER_PORT 1883
#else
//#define SERVER_PORT 1883
//#define SERVER_PORT 9092
#define SERVER_PORT 1883

#endif
//! Client port number
//#define CLIENT_PORT 5001
#define CLIENT_PORT 1883

uint8_t			g_wlanstatus	= 0;
uint32_t		t_len = 0;
uint16_t		g_socket_status;
BOOL			g_bWifiAutoConDisable	= 0;

extern uint8_t 	a1[5];
extern uint8_t 	aaa_len;
extern uint8_t 	g_ucDownloadFW_FileNo;
extern uint32_t g_u32UpdateFileSize;
extern uint32_t g_u32CheckSumTemp;
/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
static void BluetoothStatusTh( void const * argument );
static void WifiStatusTh( void const * argument );

//int compress_data(const uint8_t *data, uint32_t ndata, uint8_t *zdata, uint32_t *nzdata);
//int decompress_data(const uint8_t *zdata, uint32_t nzdata, uint8_t *data, uint32_t *ndata);

int git_deflate_data(uint8_t *in_data, uint32_t in_size, uint8_t *out_data, uint32_t *out_size);
int git_inflate_data(uint8_t *in_data, uint32_t in_size, uint8_t *out_data, uint32_t *out_size);
void* memstr(const void* haystack, size_t haystack_len, const void* needle, size_t needle_len);
//static void CreateWifiLogFile();

int32_t rsi_bt_app_init (void);
uint8_t GetPairedList(uint8_t *remote_dev_addr, uint8_t	**plinkKey, uint16_t *pIndex);
int32_t	WifiPacketSend(uint8_t *resp, uint32_t len);
int32_t WebsockPacketSend(uint8_t *resp, uint32_t len);
int32_t MQTTPacketSend(MqttMessageType mqtt_type, uint8_t *resp, uint32_t len );
uint32_t RequestDNS(uint8_t *domain_name, rsi_rsp_dns_query_t *dns_response);
uint8_t disconnectToMQTT(void);
uint8_t disconnectToWebsocket(void);
// Callback function
void InquiryTimeoutCallback(xTimerHandle  pxTimer);
void TotalScanTimoutCallback(xTimerHandle  pxTimer);
void GIT_sock_receive_callback(uint16_t sock_no, uint8_t *buffer, uint32_t length);
void GIT_MQTT_receive_callback(uint16_t status, uint8_t *buffer, const uint32_t length);
void mqtt_subscribe_callback(MessageData *md);
void socket_terminate_callback( uint16_t status, uint8_t *buffer, const uint32_t length);
void socket_notify_callback( uint16_t status, uint8_t *buffer, const uint32_t length );
void rsi_emb_mqtt_remote_socket_terminate_handler(uint16_t status, uint8_t *buffer, const uint32_t length);
void rsi_emb_mqtt_publish_receive_handler(uint16_t status, uint8_t *buffer, const uint32_t length);
void rsi_emb_mqtt_ka_timeout_handler(uint16_t status, uint8_t *buffer, const uint32_t length);

bool compare_connect_wifi(void);
void SaveConnectionLog(void);
/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
#ifdef PRINT_MESSAGE_ID
	extern stMESSAGE_ID_INFO stMessageIdInfo[20];
 	extern uint8_t ucMessageIdInfoCnt;
#endif
// Thread
static osThreadId	hBTAppTh;
osThreadDef( btappth,	BluetoothStatusTh,	osPriorityNormal, 0, 8 * configMINIMAL_STACK_SIZE );

static osThreadId	hWifiAppTh;
osThreadDef( wifiappth,	WifiStatusTh,	osPriorityNormal, 0, 2 * configMINIMAL_STACK_SIZE );

// Message
osMessageQId	hBTAppMsg;
osMessageQDef( btappqueue, MESSAGE_BTAPP_QUEUE_SIZE, uint32_t );

/**********************************************/
/**********   Variables for Common   **********/

/**********************************************/
/**********   Variables for Wifi   ************/
struct rsi_sockaddr_in	g_server_addr;
struct rsi_sockaddr_in	g_client_addr;

uint8_t     g_ucApConnected             = 0;
int32_t     g_iClient_socket            = 0;

uint8_t* 	g_uiSocketSeverIP = "43.203.212.167";
uint16_t 	g_uiSocketSeverPort = 8080;

uint32_t    hour_t                      = 0;
uint32_t    min_t                       = 0;
uint32_t    sec_t                       = 0;

bool m_ubMqttSocketAlreadyConnected = false;

uint32_t mqtt_recv_i = 0;

/**********************************************/
/**********   Variables for BT   **************/
static SBTInfo	*pBTRemoteInfo;

//! Application global parameters.
static rsi_bt_resp_get_local_name_t		local_name = {0};

static uint32_t	rsi_app_async_event_map				= 0;
static uint8_t	str_conn_bd_addr[18]				= {0,};
static uint8_t	local_dev_addr[RSI_DEV_ADDR_LEN]	= {0,};
static uint8_t	spp_data[RSI_BT_MAX_PAYLOAD_SIZE]	= {0,};

static osTimerId InquiryTimer;
static osTimerId TotalScanTimer;

//static uint8_t	s_ucBTTxFlag;
static uint8_t	TotalScanTimeOutFlag;

uint8_t	g_ucBtConnected = 0;
uint8_t g_ucBtConnRetryCnt = 0;
typedef enum __eBtScanHandler{
	eBtScanHandler_Init=0,
	eBtScanHandler_Wait,
	eBtScanHandler_Idle,
	eBtScanHandler_Occured,	
	eBtScanHandler_Scanning,
}eBtScanHandler;


eBtScanHandler m_eBtScanHandlerState = eBtScanHandler_Idle;
/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   RS9116 Common
 *--------------------------------------------------------------------*/
rsi_task_handle_t	driver_task_handle = NULL;
int32_t initRS9116( void )
{
	int32_t status;
	uint8_t macAddr[6];
	uint8_t str_bd_addr[18]	= {0};
	
    if( gsFwInfo.mucModeChange == FALSE )
    {
        //osDelay(1);
        //GLogEE("############### BT Reset ###############");
        //osDelay(1);
        EnableRS9116();
    }
#if 0
	//! Driver initialization
	status = rsi_driver_init(bt_global_buf, BT_GLOBAL_BUFF_LEN);
	if ((status < 0) || (status > BT_GLOBAL_BUFF_LEN))
	{
		GLogE( "error... rsi_driver_init(bt_global_buf)!!!\r\n" );
		return status;
	}
#endif

	status = rsi_driver_init(global_buf, GLOBAL_BUFF_LEN);
	if((status < 0) || (status > GLOBAL_BUFF_LEN))
	{
		GLogE( "error... rsi_driver_init(global_buf)!!!\r\n" );
		return status;
	}

	GLogN("RSI Driver Initalization %d\r\n", status);
	
	//rsi_wlan_power_save_profile( RSI_ACTIVE, RSI_FAST_PSP );//Disabled due to BT transmission issue in power save mode
	//rsi_bt_power_save_profile( RSI_ACTIVE, RSI_FAST_PSP );//Disabled due to BT transmission issue in power save mode

	rsi_hal_intr_config(rsi_interrupt_handler); //mod.kks to set the interrupt call back 22.07.05
if( gsFwInfo.mucModeChange == FALSE )
{
	//! Redpine module initialization
	status = rsi_device_init(LOAD_NWP_FW);
	if(status != RSI_SUCCESS)
	{
		GLogE( "error... rsi_device_init!!!\r\n" );
		return status;
	}

	GLogN("RSI Device Initialization %d\r\n", status);
}

	// Start BT-BLE Stack
//	initialize_bt_stack(STACK_BT_MODE);

	//! Task created for Driver task
	rsi_task_create((rsi_task_function_t)rsi_wireless_driver_task, (uint8_t *)"driver_task", RSI_DRIVER_TASK_STACK_SIZE, NULL, RSI_DRIVER_TASK_PRIORITY, &driver_task_handle);
if( gsFwInfo.mucModeChange == FALSE )
{
	//! WC initialization
	status = rsi_wireless_init( RSI_WLAN_CLIENT_MODE, RSI_OPERMODE_WLAN_BT_CLASSIC );
	if( status != RSI_SUCCESS )
	{
		GLogE( "Error... Fail RSI Wireless Initialization(%d)!!!\r\n", status );
		return status;
	}
	else
	{
		GLogN( "RSI Wireless Initialization... OK\r\n" );
	}
	
	status = rsi_wlan_radio_init();
    if (status != RSI_SUCCESS)
    {
        GLogE("Error... rsi_wlan_radio_init(%d)!!!\r\n", status);
        return status;
    }

    status = rsi_wlan_get(RSI_MAC_ADDRESS, macAddr, sizeof(macAddr));
    if (status != RSI_SUCCESS)
    {
        GLogE("Error... rsi_wlan_get MAC(%d)!!!\r\n", status);
        return status;
    }
	else
	{
		GLogN("local_wifi_mac_address : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
			macAddr[0], macAddr[1], macAddr[2],
			macAddr[3], macAddr[4], macAddr[5]);
		sprintf((char *)g_strWifiMacAddress, "%02X:%02X:%02X:%02X:%02X:%02X",
			macAddr[0], macAddr[1], macAddr[2],
			macAddr[3], macAddr[4], macAddr[5]);
	}
}
	// To bt_app Thread MessageQ
	hBTAppMsg = osMessageCreate( osMessageQ( btappqueue ), NULL );
	if( hBTAppMsg == NULL )
	{
		GLogE( "error... osMessageCreate hTriggergMsg\r\n" );
		return status;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hBTAppMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hBTAppMsg",sizeof("hBTAppMsg"));
#endif


	rsi_bt_app_init();

	// Regitser Callback Functions
	rsi_wlan_register_callbacks( RSI_REMOTE_SOCKET_TERMINATE_CB,			socket_terminate_callback );
	rsi_wlan_register_callbacks( RSI_WLAN_SOCKET_CONNECT_NOTIFY_CB,			socket_notify_callback );

	return INIT_OK;
}

void deinitRS9116( void )
{
	rsi_wlan_power_save_profile( RSI_SLEEP_MODE_8, RSI_MAX_PSP );
	rsi_bt_power_save_profile( RSI_SLEEP_MODE_8, RSI_MAX_PSP );
}

#if 0
int32_t GetWlanFWVersion( void )
{
	int32_t status;

	status = rsi_wlan_get( RSI_FW_VERSION, gWlanFWVersion, (uint16_t)sizeof( gWlanFWVersion ) );

	GLogN( "%s\r\n", gWlanFWVersion );

	return status;
}
#endif

void StartRS9116Thread( void )
{
	hBTAppTh = osThreadCreate( osThread(btappth), NULL );
	if( hBTAppTh == NULL )
	{
		GLogE( "Error... fail create hBTAppTh Thread!!!\r\n" );
	}

	hWifiAppTh = osThreadCreate( osThread(wifiappth), NULL );
	if( hWifiAppTh == NULL )
	{
		GLogE( "Error... fail create hWifiAppTh Thread!!!\r\n" );
	}
}

/*----------------------------------------------------------------------
 *   Wifi
 *--------------------------------------------------------------------*/
int32_t ConnectAP( uint8_t *ssid, uint8_t type, uint8_t *psk )
{
	int32_t		status			= RSI_SUCCESS;
	uint8_t cert_types[] = {1, 2, 3, 4, 5, 6, 7};
	uint8_t cert_indexes[] = {0, 1};
	
	
#if LOAD_CERTIFICATE
	for (int t = 0; t < sizeof(cert_types)/sizeof(cert_types[0]); t++)
	{
		for (int i = 0; i < sizeof(cert_indexes)/sizeof(cert_indexes[0]); i++)
		{
			status = rsi_wlan_set_certificate_index(cert_types[t], cert_indexes[i], NULL, 0);
			if(status != RSI_SUCCESS)
			{
				GLogN("erase fail!, %d, %d\r\n", t, i);
			}
		}
	}
/*
	// Load SSL Server Certificate
	status = rsi_wlan_set_certificate(RSI_SSL_SERVER_CERTIFICATE, ssl_server_certificate, (sizeof(ssl_server_certificate) - 1));
	if (status != RSI_SUCCESS)
	{
		GLogN("ssl_server_certificate Laod Fail!!\r\n");
	}
*/
#if TEST_CORRUPT_CA_CERT
	{
		// 원본 복사 후 서명 영역(base64 데이터 내부) 3곳 훼손
		static uint8_t corrupt_cert[sizeof(hyundai_ssl_ca_certificate)];
		memcpy(corrupt_cert, hyundai_ssl_ca_certificate, sizeof(corrupt_cert));
		corrupt_cert[100] ^= 0xFF;
		corrupt_cert[300] ^= 0xFF;
		corrupt_cert[600] ^= 0xFF;
		status = rsi_wlan_set_certificate(RSI_SSL_CA_CERTIFICATE, corrupt_cert, (sizeof(corrupt_cert) - 1));
		GLogN("[CA-TEST] Corrupted CA cert loaded, load status:0x%04X\r\n", status);
	}
#else
	// Load combined CA Certificate chain
	status = rsi_wlan_set_certificate(RSI_SSL_CA_CERTIFICATE, hyundai_ssl_ca_certificate, (sizeof(hyundai_ssl_ca_certificate) - 1));
	if (status != RSI_SUCCESS)
	{
		GLogN("CA cert load Fail!! status:0x%04X\r\n", status);
	}
	else
	{
		GLogN("CA cert loaded OK (%d bytes)\r\n", (sizeof(hyundai_ssl_ca_certificate) - 1));
	}
#endif
	
	status = rsi_wlan_get_status();
#endif
	
	// pbkdf2_hmac_sha1 mode
	if (type == RSI_WPA_PMK || type == RSI_WPA2_PMK  || type == RSI_WPA_WPA2_MIXED_PMK)
    {
        uint8_t pmk[32];
        size_t psk_len = strlen((const char *)psk);
        size_t ssid_len = strlen((const char *)ssid);
		
		pbkdf2_hmac_sha1(psk, psk_len, ssid, ssid_len, pmk, sizeof(pmk));
		
		status = rsi_wlan_connect((int8_t *)ssid, (rsi_security_mode_t)type, (void *)pmk);
    }
    else
    {
        status = rsi_wlan_connect((int8_t *)ssid, (rsi_security_mode_t)type, (void *)psk);
    }
	if( status != RSI_SUCCESS )
	{
		GLogE( "Error... Fail Connect AP(%s, 0x%04X)...\r\n", ssid, status );
		return status;
	}
	else
	{
		g_ucApConnected = 1;
		GLogN( "Connect AP(%s)... OK\r\n", ssid );
	}

	return 0;
}

int32_t SetIPAddressStatic( uint32_t ip_addr, uint32_t gateway , uint32_t network_mask)
{
	int32_t		status			= RSI_SUCCESS;

	uint8_t		*temp = (uint8_t *)&ip_addr;
	GLogN( "IP... %03d.%03d.%03d.%03d\r\n", temp[0], temp[1], temp[2], temp[3] );

	//! Configure IP
	status = rsi_config_ipaddress( RSI_IP_VERSION_4, RSI_STATIC, (uint8_t *)&ip_addr, (uint8_t *)&network_mask, (uint8_t *)&gateway, NULL, 0, 0 );
	if( status != RSI_SUCCESS )
	{
		GLogE( "Error... Fail Config IP Address(0x%04X)...\r\n", status );
	}
	else
	{
		GLogN( "Config IP Address...\r\n" );
	}

	return status;
}

int32_t SetIPAddressDHCP( void )
{ 
    uint8_t ip_buff[20] = {0}; 
    int32_t status = RSI_SUCCESS; 
     
    //! Configure IP 
    status = rsi_config_ipaddress( RSI_IP_VERSION_4, RSI_DHCP, NULL, NULL, NULL, (uint8_t*)ip_buff, sizeof(ip_buff), 0 ); 
     
    if( status != RSI_SUCCESS ) 
    { 
        GLogE( "Error... Fail Config IP AddressDHCP(%d)...\r\n", status ); 
    } 
    else 
    {
        // MAC address: ip_buff[0]~[5]
        // IP address: ip_buff[6]~[9]
        gsFwInfo.msWifiConnectInfo.SourceIpAddress = ip_buff[6] | ip_buff[7]<<8 | ip_buff[8]<<16 | ip_buff[9]<<24;
        // Subnet mask: ip_buff[10]~[13]
        gsFwInfo.msWifiConnectInfo.SubnetMask = ip_buff[10] | ip_buff[11]<<8 | ip_buff[12]<<16 | ip_buff[13]<<24;
        // Gateway: ip_buff[14]~[17]
        gsFwInfo.msWifiConnectInfo.Gateway = ip_buff[14] | ip_buff[15]<<8 | ip_buff[16]<<16 | ip_buff[17]<<24;
        
        GLogN("Config IP Address...\r\n");
        GLogN("MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\r\n", 
              ip_buff[0], ip_buff[1], ip_buff[2], ip_buff[3], ip_buff[4], ip_buff[5]);
        GLogN("IP Address  : %d.%d.%d.%d\r\n", 
              ip_buff[6], ip_buff[7], ip_buff[8], ip_buff[9]); 
        GLogN("Subnet Mask : %d.%d.%d.%d\r\n", 
              ip_buff[10], ip_buff[11], ip_buff[12], ip_buff[13]);
        GLogN("Gateway     : %d.%d.%d.%d\r\n", 
              ip_buff[14], ip_buff[15], ip_buff[16], ip_buff[17]);
    }  
     
    return status; 
}

int32_t DisconnectWLan( void )
{
	int32_t uiRet=0;
	
	if(g_mqtt_isconnected == 1)
	{
		if (disconnectToMQTT() == TRUE)
		{
			GLogN("Disconnected to MQTT successfully.\n");
		}
	}
	
	uiRet = rsi_wlan_disconnect();
	if(uiRet != RSI_SUCCESS)
	{
		GLogN( "WLAN Disconnect Fail!\r\n" );
	}
	else
	{
		g_ucApConnected = 0;
		GLogN( "WLAN Disconnect Success!\r\n" );
	}
	
	return uiRet;
}
void dns_query_callback(uint16_t status, const uint8_t *buffer, const uint16_t length) {
    if (status == RSI_SUCCESS) {
        GLogN("DNS query success\n");
		gsFwInfo.msWifiConnectInfo.TargetIpAddress = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
        
        uint32_t ip_addr = *(uint32_t *)buffer;
        GLogN("Resolved IP address: %d.%d.%d.%d\n",
               (ip_addr & 0xFF),
               (ip_addr >> 8) & 0xFF,
               (ip_addr >> 16) & 0xFF,
               (ip_addr >> 24) & 0xFF);
        } 
    else {
        GLogN("DNS query failed with status: %d\n", status);
    }
}

int32_t WifiPacketSend( uint8_t *resp, uint32_t len )
{
	int32_t		status	= RSI_SUCCESS;
	uint32_t	remain,loop;
	uint8_t		i;

	//! Send data on socket
	if( len > WIFI_MAX_TX_LEN )
	{
		loop	= len / WIFI_MAX_TX_LEN;
		remain	= len % WIFI_MAX_TX_LEN;

		for( i = 0; i < loop; i++ )
		{
			status = rsi_send( g_iClient_socket, (int8_t *)resp+(i*WIFI_MAX_TX_LEN), WIFI_MAX_TX_LEN, 0 );
		}

		if( remain > 0)
		{
			status = rsi_send( g_iClient_socket, (int8_t *)resp+(i*WIFI_MAX_TX_LEN), remain, 0 );
		}
	}
	else
	{
		status = rsi_send( g_iClient_socket, (int8_t *)resp, len, 0 );
	}

	if( status < 0 )
	{
		status = rsi_wlan_get_status();
		GLogE( "Error... Fail Send(0x%04X)\r\n", status );

		rsi_shutdown(g_iClient_socket, 0);
		GLogN( "rsi_shutdown(g_iClient_socket) \n\r" );
		return status;
	}

	return 0;
}

int32_t WebsockPacketSend(uint8_t *resp, uint32_t len)
{
    int32_t status = RSI_SUCCESS;
    uint32_t chunk_size = 1000;
    uint32_t remaining = len;
    uint32_t offset = 0;
    uint8_t is_first_packet = 1;

    while (remaining > 0)
    {
        uint32_t	send_len = (remaining > chunk_size) ? chunk_size : remaining;
        uint8_t		opcode;

        if (is_first_packet) // First Packet
        {
		  	if(remaining <= chunk_size)
			{
				opcode = 0x82; 
			}
			else
			{
				opcode = 0x02;
			}
            is_first_packet = 0;
        }
        else if (remaining > chunk_size)
        {
            opcode = 0x00; // Middle Packet
        }
        else
        {
            opcode = 0x80; // Termination packet
        }

        //! Send data on socket
        status = rsi_web_socket_send_async(g_iClient_socket, opcode, &resp[offset], send_len);
        if (status < 0)
        {
            GLogE("Error... Fail Send(0x%04X)\r\n", status);
            return status;
        }

        remaining -= send_len;
        offset += send_len;
    }
	GLogN("*\r\n");
    return 0;
}

int32_t MQTTPacketSend(MqttMessageType mqtt_type, uint8_t *resp, uint32_t len)
{
    static uint32_t		s_ulMqttFailStartTick	= 0;		// publish 실패 시작 시각(ms), 0이면 실패 추적 안함
    static BOOL			s_bMqttFail30secDone	= false;	// 30초 경과 처리 1회 실행 플래그
    int32_t 			status				= RSI_SUCCESS;
    rsi_mqtt_pubmsg_t 	publish_msg;
    int8_t				ucPubTopicName[TOPIC_LENGTH]	= {0,};

	memset(ucPubTopicName, 0, sizeof(ucPubTopicName));
	
    publish_msg.dup = 0;
    publish_msg.qos = QOS1;
    publish_msg.retained = 0;
    publish_msg.payload = resp;
    publish_msg.payloadlen = len;
    
    //! Send data on socket
    if(mqtt_type == MQTT_MESSAGE_TYPE_GENERAL)
    {
        GetTopicPublish(ucPubTopicName);
    }
    else if(mqtt_type == MQTT_MESSAGE_TYPE_HEALTH_CHECK)
    {
        GetHcTopicPublish(ucPubTopicName);
    }

    status = rsi_emb_mqtt_publish(ucPubTopicName, &publish_msg); 
    if (status != 0)
    {
	  	int32_t wlan_err = rsi_wlan_get_status();
        GLogE("Last WLAN err: 0x%04X\r\n", wlan_err);
		if (status != 0x0025)
		{
			int32_t wlan_err = rsi_wlan_get_status();
			GLogE("Last WLAN err: 0x%04X\r\n", wlan_err);
		}

		// publish Fail count
		if( s_ulMqttFailStartTick == 0 )
		{
			s_ulMqttFailStartTick	= rsi_hal_gettickcount();
			s_bMqttFail30secDone	= false;
		}
		// Fail 30sec -> ListSensor End
		else if( (s_bMqttFail30secDone == false) &&
		         ((rsi_hal_gettickcount() - s_ulMqttFailStartTick) >= 30000) )
		{
			if( g_OBD_Processing == true )
			{
				printf("MQTT Publish Failed 30sec\r\n");
				g_ListSensor_Endflag = false;
			}
			s_bMqttFail30secDone = true;
		}
    }
    else
    {
		// publish success
		s_ulMqttFailStartTick	= 0;
		s_bMqttFail30secDone	= false;
	}
    return status;
}


/*----------------------------------------------------------------------
 *   Bluetooth
 *--------------------------------------------------------------------*/
/*==============================================*/
/**
 * @fn         rsi_bt_app_init_events
 * @brief      initializes the event parameter.
 * @param[in]  none.
 * @return     none.
 * @section description
 * This function is used during BT initialization.
 */
static void rsi_bt_app_init_events()
{
	rsi_app_async_event_map = 0;
	return;
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_set_event
 * @brief      sets the specific event.
 * @param[in]  event_num, specific event number.
 * @return     none.
 * @section description
 * This function is used to set/raise the specific event.
 */
static void rsi_bt_app_set_event(uint32_t event_num)
{
	rsi_app_async_event_map |= BIT(event_num);
	//GLogN ("T%d ", event_num);

	osMessagePut( hBTAppMsg, event_num, osWaitForever );

	return;
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_clear_event
 * @brief      clears the specific event.
 * @param[in]  event_num, specific event number.
 * @return     none.
 * @section description
 * This function is used to clear the specific event.
 */
static void rsi_bt_app_clear_event(uint32_t event_num)
{
	rsi_app_async_event_map &= ~BIT(event_num);
	return;
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_get_event
 * @brief      returns the first set event based on priority
 * @param[in]  none.
 * @return     int32_t
 *             > 0  = event number
 *             -1   = not received any event
 * @section description
 * This function returns the highest priority event among all the set events
 */
#if 0
static int32_t rsi_bt_app_get_event(void)
{
	uint32_t  ix;

	for( ix = 0; ix < 32; ix++ )
	{
		if( rsi_app_async_event_map & (1 << ix) )		return ix;
	}

	return (RSI_FAILURE);
}
#endif

/*==============================================*/
/**
 * @fn         rsi_bt_app_on_conn
 * @brief      invoked when connection complete event is received
 * @param[out] resp_status, connection status of the connected device.
 * @param[out] conn_event, connected remote device information
 * @return     none.
 * @section description
 * This callback function indicates the status of the connection
 */
void rsi_bt_app_on_conn (uint16_t resp_status, rsi_bt_event_bond_t *conn_event)
{
	rsi_6byte_dev_address_to_ascii( (int8_t *)str_conn_bd_addr, conn_event->dev_addr );
	GLogN("on_conn: str_conn_bd_addr: %s\r\n", str_conn_bd_addr);
	rsi_bt_app_set_event (RSI_APP_EVENT_CONNECTED);
//	LED_ALL_OFF;
//	LED_SetState(eLED_NORMAL, 0, 0);
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_on_pincode_req
 * @brief      invoked when pincode request event is received
 * @param[out] user_pincode_request, pairing remote device information
 * @return     none.
 * @section description
 * This callback function indicates the pincode request from remote device
 */
void rsi_bt_app_on_pincode_req(uint16_t resp_status, rsi_bt_event_user_pincode_request_t *user_pincode_request)
{
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, user_pincode_request->dev_addr);
	GLogN("on_pin_coe_req: str_conn_bd_addr: %s\r\n", str_conn_bd_addr);
	rsi_bt_app_set_event (RSI_APP_EVENT_PINCODE_REQ);
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_on_linkkey_req
 * @brief      invoked when linkkey request event is received
 * @param[out] user_linkkey_req, pairing remote device information
 * @return     none.
 * @section description
 * This callback function indicates the linkkey request from remote device
 */
void rsi_bt_app_on_linkkey_req (uint16_t status, rsi_bt_event_user_linkkey_request_t  *user_linkkey_req)
{
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, user_linkkey_req->dev_addr);
	GLogN("linkkey_req: str_conn_bd_addr: %s\r\n", str_conn_bd_addr);
	rsi_bt_app_set_event (RSI_APP_EVENT_LINKKEY_REQ);
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_on_linkkey_save
 * @brief      invoked when linkkey save event is received
 * @param[out] user_linkkey_req, paired remote device information
 * @return     none.
 * @section description
 * This callback function indicates the linkkey save from local device
 */
void rsi_bt_app_on_linkkey_save (uint16_t status, rsi_bt_event_user_linkkey_save_t *user_linkkey_save)
{
	uint16_t usIndex = 0;

	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, user_linkkey_save->dev_addr);
	if(g_ucBtConnected == BT_FIND_TRIG)
	{
		memcpy(pBTRemoteInfo->mTrigInfo.mlinkKey,	user_linkkey_save->linkKey,		sizeof(user_linkkey_save->linkKey));
		memcpy(pBTRemoteInfo->mTrigInfo.mdev_addr,	user_linkkey_save->dev_addr,	sizeof(user_linkkey_save->dev_addr));
	}
	else
	{
		if(GetPairedList(str_conn_bd_addr, NULL, &usIndex) == true)
		{
			memcpy(pBTRemoteInfo->mHostInfo[usIndex].mlinkKey,	user_linkkey_save->linkKey,		sizeof(user_linkkey_save->linkKey));
			memcpy(pBTRemoteInfo->mHostInfo[usIndex].mdev_addr,	user_linkkey_save->dev_addr,	sizeof(user_linkkey_save->dev_addr));
		}
		else
		{
			if(pBTRemoteInfo->mHostOldIdx >= BLUETOOTH_DEVICE_MAX)			pBTRemoteInfo->mHostOldIdx = 0;

			memcpy(pBTRemoteInfo->mHostInfo[pBTRemoteInfo->mHostOldIdx].mlinkKey,	user_linkkey_save->linkKey,		sizeof(user_linkkey_save->linkKey));
			memcpy(pBTRemoteInfo->mHostInfo[pBTRemoteInfo->mHostOldIdx].mdev_addr,	user_linkkey_save->dev_addr,	sizeof(user_linkkey_save->dev_addr));

			pBTRemoteInfo->mHostOldIdx++;
			if(pBTRemoteInfo->mHostOldIdx >= BLUETOOTH_DEVICE_MAX) pBTRemoteInfo->mHostOldIdx = 0;
		}
	}

	gsFwInfo.mucChanged = TRUE;
	saveFirmwareInfo_EMMC(false);

	GLogN("linkkey_save: str_conn_bd_addr: %s : ", str_conn_bd_addr);
	for(int i = 0; i < RSI_LINK_KEY_LEN; i++)
	{
		GLogN("%02X", user_linkkey_save->linkKey[i]);
	}
	GLogN("\r\n");

	rsi_bt_app_set_event (RSI_APP_EVENT_LINKKEY_SAVE);
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_on_auth_complete
 * @brief      invoked when authentication event is received
 * @param[out] resp_status, authentication status
 * @param[out] auth_complete, paired remote device information
 * @return     none.
 * @section description
 * This callback function indicates the pairing status and remote device information
 */
void rsi_bt_app_on_auth_complete (uint16_t resp_status, rsi_bt_event_auth_complete_t *auth_complete)
{
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, auth_complete->dev_addr);
	GLogN("auth_complete: str_conn_bd_addr: %s\r\n", str_conn_bd_addr);
    rsi_bt_app_set_event (RSI_APP_EVENT_AUTH_COMPLT);
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_on_disconn
 * @brief      invoked when disconnect event is received
 * @param[out] resp_status, disconnect status/error
 * @param[out] bt_disconnected, disconnected remote device information
 * @return     none.
 * @section description
 * This callback function indicates the disconnected device information
 */
void rsi_bt_app_on_disconn (uint16_t resp_status, rsi_bt_event_disconnect_t *bt_disconnected)
{
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, bt_disconnected->dev_addr);
	GLogN("on_disconn: reason : %x str_conn_bd_addr: %s\r\n", resp_status, str_conn_bd_addr);
	rsi_bt_app_set_event (RSI_APP_EVENT_DISCONNECTED);
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_on_spp_connect
 * @brief      invoked when spp profile connected event is received
 * @param[out] spp_connect, spp connected remote device information
 * @return     none.
 * @section description
 * This callback function indicates the spp connected remote device information
 */
void rsi_bt_app_on_spp_connect (uint16_t resp_status, rsi_bt_event_spp_connect_t *spp_connect)
{
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, spp_connect->dev_addr);
	//GLogN("spp_conn: str_conn_bd_addr: %s\r\n", str_conn_bd_addr);
	rsi_bt_app_set_event (RSI_APP_EVENT_SPP_CONN);
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_on_spp_disconnect
 * @brief      invoked when spp profile disconnected event is received
 * @param[out] spp_disconn, spp disconnected remote device information
 * @return     none.
 * @section description
 * This callback function indicates the spp disconnected event
 */
void rsi_bt_app_on_spp_disconnect (uint16_t resp_status, rsi_bt_event_spp_disconnect_t *spp_disconn)
{
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, spp_disconn->dev_addr);
	GLogN("spp_disconn: reason : %x, str_conn_bd_addr: %s\r\n", resp_status, str_conn_bd_addr);
	rsi_bt_app_set_event (RSI_APP_EVENT_SPP_DISCONN);
}

/*==============================================*/
/**
 * @fn         rsi_bt_on_passkey_display
 * @brief      invoked when passkey diaplay event is received
 * @param[out] passkey display, remote device passkey information
 * @return     none.
 * @section description
 * This callback function indicates the passkey display event
 */
void rsi_bt_on_passkey_display (uint16_t resp_status, rsi_bt_event_user_passkey_display_t *bt_event_user_passkey_display)
{
	GLogN( "passkey: %d", *((uint32_t *)bt_event_user_passkey_display->passkey) );
	rsi_bt_app_set_event (RSI_APP_EVENT_PASSKEY_DISPLAY);
}

/*==============================================*/
/**
 * @fn         rsi_bt_on_passkey_request
 * @brief      invoked when passkey request event is received
 * @param[out] passkey request, passkey request to remote device
 * @return     none.
 * @section description
 * This callback function indicates the passkey request event
 */
void rsi_bt_on_passkey_request (uint16_t resp_status, rsi_bt_event_user_passkey_request_t *user_passkey_request)
{
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, user_passkey_request->dev_addr);
	GLogN ("passkey_request: str_conn_bd_addr: %s\r\n", str_conn_bd_addr);
	rsi_bt_app_set_event (RSI_APP_EVENT_PASSKEY_REQUEST);
}

/*==============================================*/
/**
 * @fn         rsi_bt_on_ssp_complete
 * @brief      invoked when ssp complete event is received
 * @param[out] ssp complete, ssp completed remote device information
 * @return     none.
 * @section description
 * This callback function indicates the ssp complete event
 */
void rsi_bt_on_ssp_complete (uint16_t resp_status, rsi_bt_event_ssp_complete_t *ssp_complete)
{
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, ssp_complete->dev_addr);
	GLogN ("ssp_complete: str_conn_bd_addr: %s\r\n",str_conn_bd_addr);
	rsi_bt_app_set_event (RSI_APP_EVENT_SSP_COMPLETE);
}

/*==============================================*/
/**
 * @fn         rsi_bt_on_confirm_request
 * @brief      invoked when confirmation request event is received
 * @param[out] confirmation request,confirmation request to remote device
 * @return     none.
 * @section description
 * This callback function indicates the confirmation request event
 */
void rsi_bt_on_confirm_request (uint16_t resp_status, rsi_bt_event_user_confirmation_request_t *user_confirmation_request)
{
	//GLogN ("data: %s\r\n",user_confirmation_request->confirmation_value );
	rsi_bt_app_set_event (RSI_APP_EVENT_CONFIRM_REQUEST);
}

/*==============================================*/
/**
 * @fn         rsi_bt_on_mode_change
 * @brief      invoked when mode chande event is received
 * @param[out] mode change,mode change request to remote device
 * @return     none.
 * @section description
 * This callback function indicates the mode change event
 */
void rsi_bt_on_mode_change (uint16_t resp_status, rsi_bt_event_mode_change_t  *mode_change)
{
	//sniff_mode = mode_change->current_mode;
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, mode_change->dev_addr);
	//GLogN ("mode_change_event: str_conn_bd_addr: %s, %d\r\n",str_conn_bd_addr, sniff_mode);
	rsi_bt_app_set_event (RSI_APP_EVENT_MODE_CHANGED);
}

/*==============================================*/
/**
 * @fn         rsi_bt_on_sniff_subrating
 * @brief      invoked when sniff subrating event is received
 * @param[out] sniff subrating,shiff subrating request to remote device
 * @return     none.
 * @section description
 * This callback function indicates the shiff subrating event
 */
void rsi_bt_on_sniff_subrating (uint16_t resp_status,rsi_bt_event_sniff_subrating_t  *mode_change)
{
	rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, mode_change->dev_addr);
	//GLogN ("mode_change_event: str_conn_bd_addr: %s\r\n",str_conn_bd_addr);
	rsi_bt_app_set_event (RSI_APP_EVENT_SNIFF_SUBRATING);
}

/*==============================================*/
/**
 * @fn         rsi_wlan_app_send_to_bt
 * @brief      this function is used to send data to ble app.
 * @param[in]   msg_type, it indicates write/notification event id.
 * @param[in]  data, raw data pointer.
 * @param[in]  data_len, raw data length.
 * @return     none.
 * @section description
 * This is a callback function
 */
void rsi_wlan_app_send_to_bt (uint16_t  msg_type, uint8_t *data, uint16_t data_len)
{
#if( PROTOCOL_UART_ENABLE )
	uint32_t	bytesSent = 0;
	//spp_data_len = RSI_MIN (RSI_BT_MAX_PAYLOAD_SIZE, data_len);
	bytesSent = xStreamBufferSpacesAvailable(hSBUartTx);
	if( bytesSent < data_len )
	{
		GLogE( "Error... fail send streambuffer1!!!\r\n" );
	}

	bytesSent =  xStreamBufferSend( hSBUartTx, (void *)data, data_len, 0);
	if( bytesSent != data_len )
	{
		GLogE( "Error... fail send streambuffer2!!!\r\n" );
	}
	//memcpy (spp_data, data, spp_data_len);		//BT driver runs twice while BT app runs only once, so memcpy cannot be used; changed to stream buffer usage. When using the BT trigger module, consecutive 2 or 3 TX transmissions are required.
#endif	// PROTOCOL_UART_ENABLE

	rsi_bt_app_set_event (RSI_APP_EVENT_SPP_TX);

//	WIFI_G_LED_TOGGLE;
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_on_spp_data_rx
 * @brief      invoked when spp data rx event is received
 * @param[out] spp_receive, spp data from remote device
 * @return     none.
 * @section description
 * This callback function indicates the spp data received event
 */
void rsi_bt_app_on_spp_data_rx (uint16_t resp_status, rsi_bt_event_spp_receive_t *spp_receive)//Data received
{
#if( PROTOCOL_UART_ENABLE )
	uint32_t	bytesSent = 0;

//	WIFI_G_LED_TOGGLE;
	bytesSent = xStreamBufferSpacesAvailable(hSBUartRx);
	if( bytesSent < spp_receive->data_len )
	{
		GLogN( "E]11\r\n" );
	}
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	bytesSent =  xStreamBufferSendFromISR( hSBUartRx, (void *)spp_receive->data, spp_receive->data_len, &xHigherPriorityTaskWoken);
    rsi_bt_app_set_event (RSI_APP_EVENT_SPP_RX);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	if( bytesSent != spp_receive->data_len )
	{
		GLogN( "E]23\r\n" );
	}
#endif	// PROTOCOL_UART_ENABLE
	if( g_bBTLogOnRxFlag == true )
	{
		uint16_t ii;
		GLogN("spp_rx:%d, ", spp_receive->data_len);

		for (ii = 0; ii < spp_receive->data_len; ii++)
		{
			GLogN("%02X ", spp_receive->data[ii]);
		}
		GLogN("\r\n");
	}
	else
	{
		for (uint16_t ii = 0; ii < spp_receive->data_len; ii++)
		{
			if( ii < RSI_BT_MAX_PAYLOAD_SIZE )	g_SaveLastRxPacket[ii] = spp_receive->data[ii];
			else{}
		}
	}

#if 0//defined(DEBUG_BT_PACKET)
	uint16_t ix;
#if 0
	GLogN("spp_rx: data_len: %d, ", spp_receive->data_len);

	for (ix = 0; ix < spp_receive->data_len; ix++)
	{
		GLogN("%02X ", spp_receive->data[ix]);
	}
	GLogN("\r\n");
#else
	printf("spp_rx: ");

	for (ix = 0; ix < spp_receive->data_len; ix++)
	{
		printf("%02X ", spp_receive->data[ix]);
	}
	printf("\r\n");
#endif
#endif
}

void rsi_bt_app_on_scan_resp (uint16_t rssp_status, rsi_bt_event_inquiry_response_t *single_scan_resp)
{

//    GLogN ("rsi_bt_app_on_scan_resp %02X:%02X:%02X:%02X:%02X:%02X \r\n",single_scan_resp->dev_addr[0],single_scan_resp->dev_addr[1],single_scan_resp->dev_addr[2],single_scan_resp->dev_addr[3],single_scan_resp->dev_addr[4],single_scan_resp->dev_addr[5] );
//    GLogN ("length %d,rssi %d,quiry_type %d\r\n",single_scan_resp->name_length ,single_scan_resp->rssi,single_scan_resp->inquiry_type );
//    GLogN ("cod %02X %02X %02X  \r\n",single_scan_resp->cod[0],single_scan_resp->cod[1],single_scan_resp->cod[2]);
//    GLogN ("~~~~~rsi_bt_app_on_scan_resp~~~~ %s \r\n",single_scan_resp->remote_device_name);
	rsi_6byte_dev_address_to_ascii(str_conn_bd_addr, single_scan_resp->dev_addr);
//	if ( strncmp((char const*)single_scan_resp->remote_device_name, SLAVE_NAME, 5) == 0 )
//	{
//		//rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, single_scan_resp->dev_addr);
//
//		g_ucBtConnected = BT_FIND_TRIG;
//		GLogN ("%d %s %d %s %s\r\n",
//			single_scan_resp->inquiry_type, single_scan_resp->dev_addr, single_scan_resp->name_length,
//			single_scan_resp->remote_device_name, str_conn_bd_addr);
//	}

	if( single_scan_resp->name_length > 0 )
		GLogN ("rsi_bt_app_on_scan_resp length : %d, ssid : %s\r\n",single_scan_resp->name_length, single_scan_resp->remote_device_name );
	else
		GLogN ("rsi_bt_app_on_scan_resp\r\n");
	
	rsi_bt_app_set_event(RSI_APP_EVENT_SCAN_RESP);
}

void rsi_bt_on_remote_name_resp (uint16_t rssp_status, rsi_bt_event_remote_device_name_t *single_scan_resp)
{

    GLogN ("rsi_bt_app_on_scan_resp %02X:%02X:%02X:%02X:%02X:%02X \r\n",single_scan_resp->dev_addr[0],single_scan_resp->dev_addr[1],single_scan_resp->dev_addr[2],single_scan_resp->dev_addr[3],single_scan_resp->dev_addr[4],single_scan_resp->dev_addr[5] );
  //  GLogN ("length %d,rssi %d,quiry_type %d\r\n",single_scan_resp->name_length ,single_scan_resp->rssi,single_scan_resp->inquiry_type );
//    GLogN ("cod %02X %02X %02X  \r\n",single_scan_resp->cod[0],single_scan_resp->cod[1],single_scan_resp->cod[2]);
    GLogN ("[~~~~~rsi_bt_app_on_scan_resp~~~~ ]%s \r\n",single_scan_resp->remote_device_name);
	if ( strncmp((char const*)single_scan_resp->remote_device_name, SLAVE_NAME, 5) == 0 )
	{
		//rsi_6byte_dev_address_to_ascii((int8_t *)str_conn_bd_addr, single_scan_resp->dev_addr);

		g_ucBtConnected = BT_FIND_TRIG;
//		GLogN ("%d %s %d %s %s\r\n",
//			single_scan_resp->inquiry_type, single_scan_resp->dev_addr, single_scan_resp->name_length,
//			single_scan_resp->remote_device_name, str_conn_bd_addr);
	}
	GLogN ("rsi_bt_app_on_scan_resp %s\r\n",single_scan_resp->remote_device_name );
	rsi_bt_app_set_event (RSI_APP_EVENT_NAME_RESP);
}

void rsi_bt_on_inquiry_complete_event (uint16_t resp_status)
{
	GLogN ("rsi_bt_on_inquiry_complete_event %d \r\n", resp_status );
	if(TotalScanTimeOutFlag == 1)
	{
		rsi_bt_app_set_event (RSI_APP_EVENT_SCAN_AGAIN);
		osTimerStart(InquiryTimer, 20000);
	}
	else
	{
		osTimerStop(InquiryTimer);
	}
/*
	if(g_ucBtConnected == BT_FIND_TRIG)
	{
		TotalScanTimeOutFlag = 0;
		rsi_bt_connect(str_conn_bd_addr);
		osTimerStop(InquiryTimer);
		osTimerStop(TotalScanTimer);
		//rsi_bt_cancel_inquiry();
		rsi_bt_start_discoverable();
		rsi_bt_set_connectable();
		GLogI("BT Trigger Found! (%s)\r\n", str_conn_bd_addr);
	}*/
}

void rsi_bt_app_on_scan_req()
{
	GLogN ("rsi_bt_app_on_scan_req\r\n" );
	rsi_bt_app_set_event( RSI_APP_EVENT_SCAN_REQ );
}

int32_t bt_spp_transfer(uint8_t *data, uint16_t length)
{
	int uiFilePos	= 0;
	int uiReadSize	= RSI_BT_MAX_PAYLOAD_SIZE;

//#if defined(DEBUG_BT_PACKET)
//	int i = 0;
//	GLogN ("spp_tx: Total_len: %d, ", length);
//#endif
	osEvent event;

	if( (uiFilePos + uiReadSize) >= length )			
		uiReadSize = length - uiFilePos;

	do
	{
		rsi_wlan_app_send_to_bt( 0,&data[uiFilePos], uiReadSize );

		//wait tx done
		event = osSignalWait(SIGNAL_RS9116_TX_DONE_WAIT, 10000);
		if( event.status != osEventSignal )
		{
			printf("%s] tx error \r\n", __func__);
			break;
		}
		if( g_bBTLogOnTxFlag == true )
		{
			int ii = 0;
			GLogN ("tx:%d, ", length);
			for (ii = 0; ii < uiReadSize; ii++)
			{
				GLogN ("%02X ", data[uiFilePos + ii]);
			}
			GLogN ("\r\n");
		}
		else
		{
			for (int ii = 0; ii < uiReadSize; ii++)
			{
				if( ii < RSI_BT_MAX_PAYLOAD_SIZE )	g_SaveLastTxPacket[ii] = data[uiFilePos + ii];
				else{}
			}
		}

#if defined(DEBUG_BT_PACKET)  //mod.kks 
	int i = 0;
#if 1
	GLogN ("tx:%d, ", length);
	//GLogI (" data_len: %d, data: ", uiReadSize);
	for (i = 0; i < uiReadSize; i++)
	{
		GLogN ("%02X ", data[uiFilePos + i]);
	}
	GLogN ("\r\n");
#else
	printf ("bt_tx fid: ");
	for (i = 8; i < 10; i++)
	//for (i = 8; i < 12; i++)
	{
		printf ("%02X ", data[uiFilePos + i]);
	}
	printf ("\r\n");
#endif
#endif

		uiFilePos += uiReadSize;
		if( (uiFilePos + uiReadSize) >= length )			uiReadSize = length - uiFilePos;

//		WIFI_G_LED_TOGGLE;
	} while( uiReadSize != 0 );

	return 0;
}

uint8_t GetPairedList(uint8_t *remote_dev_addr, uint8_t	**plinkKey, uint16_t *pIndex)
{
	uint8_t		dev_addr[RSI_DEV_ADDR_LEN];
	uint16_t	i = 0, k = 0;

	rsi_ascii_dev_address_to_6bytes_rev(dev_addr,  (int8_t *)remote_dev_addr);

	//MONI 20230419 static analysis num : 33 / for checking null pintor	
	if( pBTRemoteInfo == NULL )
	{
		printf("%s] error pBTRemoteInfo is null \r\n", __func__);
		return false;
	
}


	if( memcmp(pBTRemoteInfo->mTrigInfo.mdev_addr, dev_addr, 6) == 0)
	{
		*plinkKey = pBTRemoteInfo->mTrigInfo.mlinkKey;
	}
	else
	{
		for(i = 0; i <BLUETOOTH_DEVICE_MAX; i++)
		{
			if( memcmp(pBTRemoteInfo->mHostInfo[i].mdev_addr, dev_addr, 6) == 0)
			{
				*plinkKey = pBTRemoteInfo->mHostInfo[i].mlinkKey;
				*pIndex	  = i;

				for(k = 0; k < RSI_LINK_KEY_LEN; k++)
				{
					GLogN("%02X",pBTRemoteInfo->mHostInfo[i].mlinkKey[k]);
				}

				GLogN("\r\n");
				break;
			}
		}

		if( i == BLUETOOTH_DEVICE_MAX)			return false;
	}

	return true;
}

void InquiryTimeoutCallback(xTimerHandle  pxTimer)
{
	if( TotalScanTimeOutFlag == 1 )
	{
		rsi_bt_app_set_event (RSI_APP_EVENT_SCAN_AGAIN);
		osTimerStart(InquiryTimer, 20000);
	}
	else
	{
		osTimerStop(InquiryTimer);
	}
}

void TotalScanTimoutCallback(xTimerHandle  pxTimer)
{
	TotalScanTimeOutFlag = 0;
	rsi_bt_app_set_event (RSI_APP_EVENT_SCAN_TIMEOUT);
	osTimerStop(InquiryTimer);
	osTimerStop(TotalScanTimer);
}

/*==============================================*/
/**
 * @fn         rsi_bt_app_init
 * @brief      Tests the BT Classic SPP Slave role.
 * @param[in]  none
  * @return    none.
 * @section description
 * This function is used to test the SPP Slave role.
 */
int32_t rsi_bt_app_init (void)
{
	int32_t status			= 0;
	uint8_t str_bd_addr[18]	= {0};

	//! BT register GAP callbacks:
	rsi_bt_gap_register_callbacks(
		NULL,										//role_change
		rsi_bt_app_on_conn,
		NULL,
		rsi_bt_app_on_disconn,
		rsi_bt_app_on_scan_resp,
		rsi_bt_on_remote_name_resp,					//remote_name_req
		NULL,										//passkey_display
		NULL,										//remote_name_req+cancel
		rsi_bt_on_confirm_request,					//confirm req
		rsi_bt_app_on_pincode_req,
		NULL,										//passkey request
		rsi_bt_on_inquiry_complete_event,			//inquiry complete
		rsi_bt_app_on_auth_complete,
		rsi_bt_app_on_linkkey_req,					//linkkey request
		rsi_bt_on_ssp_complete,						//ssp complete
		rsi_bt_app_on_linkkey_save,
		NULL,										//get services
		NULL,										//search service
		rsi_bt_on_mode_change,
		rsi_bt_on_sniff_subrating,
        NULL);   //mod.kks to check init state. bt_on_connection_initiated : 0 SUCCESS

	//! initialize the event map
	rsi_bt_app_init_events ();

    pBTRemoteInfo = &gsFwInfo.msBTDeviceInfo;
	
if( gsFwInfo.mucModeChange == FALSE )
{
//	pBTRemoteInfo = &gsFwInfo.msBTDeviceInfo;

	//! get the local device address(MAC address).
	status = rsi_bt_get_local_device_address(local_dev_addr);
	if(status != RSI_SUCCESS)
	{
		return status;
	}
	
	//rsi_bt_set_antenna(1);	//mod.kch
	rsi_bt_set_antenna_tx_power_level(1,75);	//mod.kch
	
	rsi_6byte_dev_address_to_ascii ((int8_t *)str_bd_addr, local_dev_addr);
	(void)memcpy(gsFwInfo.msBTDeviceInfo.strMAC_Address, str_bd_addr, (sizeof(gsFwInfo.msBTDeviceInfo.strMAC_Address) - 1u));
	//GLogN("local_bd_address: %s\r\n", str_bd_addr);

	//! set the local device name
	char	device_name[20]={0,};
	sprintf( device_name, "%s_", RSI_BT_LOCAL_NAME);
	memcpy(&device_name[strlen(device_name)], gsFwInfo.marrucSerialNo, SERIAL_NUMBER_SIZE);
	
	status = rsi_bt_set_local_name((int8_t *)device_name);
	if(status != RSI_SUCCESS)
	{
		GLogE("rsi_bt_set_local_name:%d\r\n",status);
		return status;
	}

	//For fast search
	status = Set_Eir_Data(device_name);
	if(status != RSI_SUCCESS)
	{
		GLogE("Set_Eir_Data:%d\r\n",status);
		return status;
	}
	
	//! get the local device name
	status = rsi_bt_get_local_name(&local_name);
	if(status != RSI_SUCCESS)
	{
		GLogE("rsi_bt_get_local_name:%d\r\n",status);
		return status;
	}
	GLogN("local_name: %s\r\n", local_name.name);

	status = rsi_bt_set_local_class_of_device(0x100);
	if(status != RSI_SUCCESS)
	{
		GLogE("rsi_bt_set_local_class_of_device:%d\r\n",status);
		return status;
	}

	//! start the discover mode
	status = rsi_bt_start_discoverable();
	if(status != RSI_SUCCESS)
	{
		GLogE("rsi_bt_start_discoverable:%d\r\n",status);
		return status;
	}

	//! start the connectability mode
	status = rsi_bt_set_connectable();
	if(status != RSI_SUCCESS)
	{
		GLogE("rsi_bt_set_connectable:%d\r\n",status);
		return status;
	}

	//! start the ssp mode
	//status = rsi_bt_set_ssp_mode(0,1);// ask pin code
	// status = rsi_bt_set_ssp_mode(1,1);
	status = rsi_bt_set_ssp_mode(1,3);//ssp mode
	if(status != RSI_SUCCESS)
	{
		GLogE("rsi_bt_set_ssp_mode:%d\r\n",status);
		return status;
	}

	//! initilize the SPP profile
	status = rsi_bt_spp_init();
	if(status != RSI_SUCCESS)
	{
		GLogE("rsi_bt_spp_init:%d\r\n",status);
		return status;
	}
}
	//! register the SPP profile callback's
	rsi_bt_spp_register_callbacks(rsi_bt_app_on_spp_connect,
								  rsi_bt_app_on_spp_disconnect,
								  rsi_bt_app_on_spp_data_rx);

	InquiryTimer	= xTimerCreate("Xtimer1", pdMS_TO_TICKS(20000), pdFALSE , (void *)0, InquiryTimeoutCallback);
	TotalScanTimer	= xTimerCreate("Xtimer2", pdMS_TO_TICKS(60000), pdFALSE , (void *)1, TotalScanTimoutCallback);

	GLogN( "RSI Bluetooth Initialization.. OK\r\n" );

	return 0;
}

static int32_t rsi_bt_app_task( void )
{
	int32_t		status			= 0;
	uint32_t	temp_event_map	= 0;
	uint8_t		*linkKey		= NULL;
	rsi_bt_event_remote_device_name_t rsi_bt_event_remote_device_name;

#if( PROTOCOL_UART_ENABLE )
	int32_t	count		= 0;
#endif // PROTOCOL_UART_ENABLE

	static uint32_t s_unTxErrorCount = 0;

	//! Application main loop
	//! checking for received events
	osEvent	event;
	event = osMessageGet( hBTAppMsg, osWaitForever );

	if( event.status != osEventMessage )
	{
		//! if events are not received loop will be continued.
		GLogE ("error!! rsi_bt_app_get_event \r\n"); //mod.kks todo check !!!!
		return 0;
	}
#ifdef PRINT_MESSAGE_ID
	printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hBTAppMsg));
#endif

	temp_event_map = event.value.v;

	//! if any event is received, it will be served.
	switch(temp_event_map)
	{
		case RSI_APP_EVENT_CONNECTED:
		{
			//GLogN(" RSI_APP_EVENT_CONNECTED\r\n");

			//! remote device connected event
//			WIFI_G_LED_ON;

			if( g_ucBtConnected != BT_FIND_TRIG)
			{
				LED_ALL_OFF;
				LED_SetState(eLED_NORMAL, 0, 0);
			}


			//! clear the connected event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_CONNECTED);
			break;
		}

		case RSI_APP_EVENT_PINCODE_REQ:
		{
			//GLogN(" RSI_APP_EVENT_PINCODE_REQ\r\n");
			//! pincode request event
			uint8_t *pin_code = PIN_CODE;

			//! sending the pincode requet reply
			status = rsi_bt_pincode_request_reply((int8_t *)str_conn_bd_addr, pin_code, 1);
			if(status != RSI_SUCCESS)
			{
				GLogE ("Pincode error!!: %d \r\n", status);
			}

			//! clear the pincode request event.
			rsi_bt_app_clear_event(RSI_APP_EVENT_PINCODE_REQ);

			break;
		}

		case RSI_APP_EVENT_LINKKEY_SAVE:
		{
			//GLogN(" RSI_APP_EVENT_LINKKEY_SAVE\r\n");
			//! linkkey save event
			//! clear the likkey save event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_LINKKEY_SAVE);
			break;
		}

		case RSI_APP_EVENT_AUTH_COMPLT:
		{
			//GLogN(" RSI_APP_EVENT_AUTH_COMPLT\r\n");
			//! authentication complete event
			//! clear the authentication complete event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_AUTH_COMPLT);
			break;
		}

		case RSI_APP_EVENT_DISCONNECTED:
		{
			//GLogN(" RSI_APP_EVENT_DISCONNECTED\r\n");
#ifdef VCI3_RECORD
			if( g_ucBtConnected == BT_TRIG_CONNECT )
			{
				// send obd led on event to trigger module
				SendEvent2TriggerThread(eEVT_TRG_BT_DISCONNECT);
			}
#endif
			// changed led color from green to ??????
			//LED_ALL_OFF;
			//LED_SetState(eLED_NORMAL, 0, 0);			
			
			//! remote device connected event
			g_ucBtConnected = 0;
//			WIFI_G_LED_OFF;
			//! clear the disconnected event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_DISCONNECTED);
			break;
		}

		case RSI_APP_EVENT_LINKKEY_REQ:
		{
			//! linkkey request event
			GLogN("linkkey_req: %s\r\n", str_conn_bd_addr);
			if(g_ucBtConnRetryCnt>2)//If reconnection with the registered 'linkkey' repeatedly fails, it induces the tablet to re-register the 'linkkey'. kkt
			{
				g_ucBtConnRetryCnt=0;
				//! sending the linkkey request negative reply
				rsi_bt_linkkey_request_reply ((int8_t *)str_conn_bd_addr, NULL, 0);
			}
			else
			{
				if(GetPairedList(str_conn_bd_addr, &linkKey, NULL))
				{
					//! sending the linkkey request positive reply
					rsi_bt_linkkey_request_reply ((int8_t *)str_conn_bd_addr, linkKey, 1);
					g_ucBtConnRetryCnt++;
				}
				else
				{
					//! sending the linkkey request negative reply
					rsi_bt_linkkey_request_reply ((int8_t *)str_conn_bd_addr, NULL, 0);
				}
			}

			//! clear the linkkey request event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_LINKKEY_REQ);
			break;
		}

		case RSI_APP_EVENT_SPP_CONN:
		{
			GLogN(" RSI_APP_EVENT_SPP_CONN\r\n");
			//! spp connected event
			uint8_t		dev_addr[RSI_DEV_ADDR_LEN];

			rsi_ascii_dev_address_to_6bytes_rev(dev_addr,  (int8_t *)str_conn_bd_addr);

			if( memcmp(pBTRemoteInfo->mTrigInfo.mdev_addr, dev_addr, 6) == 0)
			{
				g_ucBtConnected = BT_TRIG_CONNECT;
				TotalScanTimeOutFlag = 0;
                
                uint8_t res = 99, ret = 99; 
                ret = rsi_bt_get_local_device_role(str_conn_bd_addr, &res);           // get local device role( 0 : master, 1 : slave )
                if ( ret == 0 && res == 0 )                                           // if local device role : master --> change role : slave         
                {        
                    ret = rsi_bt_set_local_device_role(str_conn_bd_addr, 1, &res);    // set local device role : slave
                    if ( ret == 0 ) GLogN("[SUCCESS]Set_local_Device_role : slave\r\n");
                    else            GLogE("[FAILURE]Set_local_Device_role\r\n");
                }
			}
			else
			{
				g_ucBtConnected = BT_SPP_CONNECT;
			}
			
			//rsi_bt_change_pkt_type(str_conn_bd_addr,PTYPE_1MBPS_MODE_ONLY);

			// send obd led on event to trigger module
#ifdef VCI3_RECORD
			SendEvent2TriggerThread(eEVT_TRG_BT_CONNECT);
#endif
			//! clear the spp connected event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SPP_CONN);
			g_ucBtConnRetryCnt=0;

			/* here we call the sniff_mode command*/
			break;
		}

		case RSI_APP_EVENT_SPP_DISCONN:
		{
			GLogN(" RSI_APP_EVENT_SPP_DISCONN\r\n");

			//if( g_ucBtConnected == BT_TRIG_CONNECT )
			//{
			// send obd led on event to trigger module
#ifdef VCI3_RECORD
			SendEvent2TriggerThread(eEVT_TRG_BT_DISCONNECT);
#endif
			//}
			
			//! spp disconnected event
			g_ucBtConnected = 0;
			//! clear the spp disconnected event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SPP_DISCONN);
			break;
		}

		case RSI_APP_EVENT_SPP_RX:
		{
			//GLogN(" RSI_APP_EVENT_SPP_RX\r\n");
			//! spp receive event
			//! clear the spp receive event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SPP_RX);
			break;
		}

		case RSI_APP_EVENT_SPP_TX:
		{
#if( PROTOCOL_UART_ENABLE )
			//GLogN(" RSI_APP_EVENT_SPP_TX\r\n");
			//! spp receive event
#if 0			
			count = xStreamBufferReceive( hSBUartTx, (void *)spp_data, RSI_BT_MAX_PAYLOAD_SIZE, pdMS_TO_TICKS( 20 ) );

			if(count != 0)	status = rsi_bt_spp_transfer (str_conn_bd_addr, spp_data, count);//Actual data transmission part
#else//moon mod
			memset(spp_data,0,sizeof(spp_data));
			count = xStreamBufferReceive( hSBUartTx, (void *)spp_data, RSI_BT_MAX_PAYLOAD_SIZE, pdMS_TO_TICKS( 20 ) );

			//if(count != 0)	status = rsi_bt_spp_transfer (str_conn_bd_addr, spp_data, count);//Actual data transmission part
			if(count != 0) status = rsi_bt_spp_transfer (str_conn_bd_addr, spp_data, RSI_BT_MAX_PAYLOAD_SIZE);

			//memset(spp_data,0,sizeof(spp_data));
   			//rsi_bt_spp_transfer (str_conn_bd_addr, spp_data, 200);//?�제 ?�이???�신?�는 붢��?
#endif
			if(status != RSI_SUCCESS)
			{
				GLogE ("TX error!!: %d \r\n", status);

				if( s_unTxErrorCount++ >= 3 )
				{
					GLogE("%s] error : RSI_APP_EVENT_SPP_TX\r\n", __func__);

					//initRS9116();
				}
			}
#endif	// PROTOCOL_UART_ENABLE

			//! clear the spp receive event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SPP_TX);

			// notify tx done 
#if(VCI_III_ASING_MODE)
			extern 	osThreadId		hTestCertifyTh;
			osSignalSet( hTestCertifyTh, SIGNAL_RS9116_TX_DONE_WAIT);	
#else
			osSignalSet( hTransmitTh, SIGNAL_RS9116_TX_DONE_WAIT);
#endif
			break;
		}

		case RSI_APP_EVENT_PASSKEY_DISPLAY:
		{
			//GLogN(" RSI_APP_EVENT_PASSKEY_DISPLAY\r\n");
			//! clear the ssp receive event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_PASSKEY_DISPLAY);
			break;
		}

		case RSI_APP_EVENT_PASSKEY_REQUEST:
		{
			//GLogN(" RSI_APP_EVENT_PASSKEY_REQUEST\r\n");
			//! clear the passkey request event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_PASSKEY_REQUEST);
			break;
		}

		case RSI_APP_EVENT_SSP_COMPLETE:
		{
			//GLogN(" RSI_APP_EVENT_SSP_COMPLETE\r\n");
			//! clear the ssp receive event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SSP_COMPLETE);
			break;
		}

		case RSI_APP_EVENT_CONFIRM_REQUEST:
		{
			GLogN(" RSI_APP_EVENT_CONFIRM_REQUEST\r\n");
			rsi_bt_accept_ssp_confirm((int8_t *)str_conn_bd_addr);

			//! clear the ssp receive event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_CONFIRM_REQUEST);
			break;
		}

		case RSI_APP_EVENT_MODE_CHANGED:
		{
			//! clear mode change event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_MODE_CHANGED);
//			GLogN("MODE_CHANGE IS COMPLETED : mode(%d)\r\n", sniff_mode);
			break;
		}

		case RSI_APP_EVENT_SNIFF_SUBRATING:
		{
			//! clear the sniff subrating event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SNIFF_SUBRATING);
			//GLogN("SNIFF SUBRATING IS COMPLETED\r\n");
			break;
		}

		case RSI_APP_EVENT_SCAN_RESP:
		{
			GLogN(" RSI_APP_EVENT_SCAN_RESP\r\n");
			//! scan response event
//			if(g_ucBtConnected == BT_FIND_TRIG)
//			{
//				TotalScanTimeOutFlag = 0;
//				rsi_bt_connect((int8_t*)str_conn_bd_addr);
//				osTimerStop(InquiryTimer);
//				osTimerStop(TotalScanTimer);
//				//rsi_bt_cancel_inquiry();
//				rsi_bt_start_discoverable();
//				rsi_bt_set_connectable();
//				GLogI("BT Trigger Found!\r\n");
//			}
//			else
//			{
//				//rsi_bt_app_on_scan_req();
//			}

			//! clear the spp receive event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SCAN_RESP);
			if(g_ucBtConnected >= BT_FIND_TRIG){}
			else							rsi_bt_app_set_event (RSI_APP_EVENT_NAME_REQ);
			break;
		}

		case RSI_APP_EVENT_NAME_REQ:
				rsi_bt_remote_name_request_async(str_conn_bd_addr,&rsi_bt_event_remote_device_name);
				rsi_bt_app_clear_event (RSI_APP_EVENT_NAME_REQ);
				break;
		case RSI_APP_EVENT_NAME_RESP:
				if(g_ucBtConnected == BT_FIND_TRIG)
				{
				  	//TotalScanTimeOutFlag = 0;
					//rsi_bt_connect(str_conn_bd_addr);
					osTimerStop(InquiryTimer);
					osTimerStop(TotalScanTimer);
					rsi_bt_cancel_inquiry();
					rsi_bt_start_discoverable();
					rsi_bt_set_connectable();
					GLogI("BT Trigger Found! (%s)\r\n", str_conn_bd_addr);


					osDelay(100);
					
					rsi_bt_connect(str_conn_bd_addr);
				}
				rsi_bt_app_clear_event (RSI_APP_EVENT_NAME_RESP);
				break;

		case RSI_APP_EVENT_SCAN_REQ:
		{
			GLogN(" RSI_APP_EVENT_SCAN_REQ\r\n");

			//! scan request event
//			SetBuzzerState(eBT_PAIRING);
			memset(&pBTRemoteInfo->mHostOldIdx, 0x00, sizeof(SBTInfo));
			if( (g_ucBtConnected == BT_SPP_CONNECT)||(g_ucBtConnected == BT_TRIG_CONNECT) )
			{
				//rsi_bt_disconnect((int8_t*)str_conn_bd_addr);     //on_disconn should be called, but only spp_disconn occurs, causing abnormal behavior instead.
				/*if( m_eBtScanHandlerState == eBtScanHandler_Scanning )
				{
					g_ucBtConnected = 0;
					rsi_bt_set_non_connectable();
					rsi_bt_stop_discoverable();
					osTimerStart(InquiryTimer, 20000);
					osTimerStart(TotalScanTimer, 60000);
					TotalScanTimeOutFlag = 1;

					rsi_bt_inquiry(2, SCAN_TIMEOUT, SCAN_DEVICE);
				}*/
			}
			else
			{
				g_ucBtConnected = 0;
				rsi_bt_set_non_connectable();
				rsi_bt_stop_discoverable();
				osTimerStart(InquiryTimer, 20000);
				osTimerStart(TotalScanTimer, 60000);
				TotalScanTimeOutFlag = 1;

				rsi_bt_inquiry(2, SCAN_TIMEOUT, SCAN_DEVICE);
			}

			//! clear the spp receive event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SCAN_REQ);
			break;
		}

		case RSI_APP_EVENT_SCAN_AGAIN:
		{
			GLogN(" RSI_APP_EVENT_SCAN_AGAIN\r\n");
			//! scan request event
			rsi_bt_cancel_inquiry( );
			rsi_bt_inquiry(2, SCAN_TIMEOUT, SCAN_DEVICE);

			//! clear the spp receive event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SCAN_AGAIN);
			break;
		}

		case RSI_APP_EVENT_SCAN_TIMEOUT:
		{
			GLogN(" RSI_APP_EVENT_SCAN_TIMEOUT\r\n");
			//! scan request event
			rsi_bt_cancel_inquiry();
			rsi_bt_start_discoverable();
			rsi_bt_set_connectable();

			gsFwInfo.mucChanged = TRUE;
			saveFirmwareInfo_EMMC(false);

			//! clear the spp receive event.
			rsi_bt_app_clear_event (RSI_APP_EVENT_SCAN_TIMEOUT);
			break;
		}
	}

	return 0;
}
void GetWifiMacAddress()
{
    GLogN("WIFI MAC : %s\r\n", g_strWifiMacAddress);
}

void DnsHandler()
{
	char* RSI_DNS_HOST_NAME = "ft.gitauto.com";
	char* RSI_DNS_ZONE_NAME = "git.mqtt.server";
	
	//			status = rsi_dns_update(RSI_IP_VERSION_4, (uint8_t *)RSI_DNS_ZONE_NAME, (uint8_t *)RSI_DNS_HOST_NAME,  
	//			(uint8_t *)&gsFwInfo.msWifiConnectInfo.TargetIpAddress, (uint16_t)RSI_DNS_TTL, rsi_dns_response_handler); 
}

void PrintWifiConnectInfo( void )
{
	uint8_t		i;
	uint8_t		mode;

	GLogI( "--------------Print AutoConnect Info--------------\r\n");

	mode = gsFwInfo.msWifiConnectInfo.mode;

	switch( mode )
	{
		case 0 :			GLogI( "mode : OPEN\r\n");			break;
		case 1 :			GLogI( "mode : WPA\r\n");			break;
		case 2 :			GLogI( "mode : WPA2\r\n");			break;
		case 3 :			GLogI( "mode : WEP\r\n");			break;
		default:			GLogI( "Unknown Wifi Mode\r\n" );	break;
	}

	GLogI( "PSK_length : %d \r\n", gsFwInfo.msWifiConnectInfo.PSK_length);
	GLogI( "PSK_Key : " );
	for( i = 0; i < gsFwInfo.msWifiConnectInfo.PSK_length; i++ )
	{
		GLogI( "%c", gsFwInfo.msWifiConnectInfo.PSK_Key[i]);
	}
	GLogI( "\r\n" );

	GLogI( "SSIDlength : %d \r\n", gsFwInfo.msWifiConnectInfo.SSIDlength );
	GLogI( "SSIDname : " );
	for( i = 0; i < gsFwInfo.msWifiConnectInfo.SSIDlength; i++ )
	{
		GLogI( "%c", gsFwInfo.msWifiConnectInfo.SSIDname[i] );
	}
	GLogI( "\r\n" );

	GLogI( "SourceIpAddress : %d.%d.%d.%d\r\n",(gsFwInfo.msWifiConnectInfo.SourceIpAddress&0x000000FF)
											  ,((gsFwInfo.msWifiConnectInfo.SourceIpAddress&0x0000FF00)>>8)
											  ,((gsFwInfo.msWifiConnectInfo.SourceIpAddress&0x00FF0000)>>16)
											  ,((gsFwInfo.msWifiConnectInfo.SourceIpAddress&0xFF000000)>>24));
	GLogI( "SourceSubnetMask: %d.%d.%d.%d\r\n",(gsFwInfo.msWifiConnectInfo.SubnetMask&0x000000FF)
											  ,((gsFwInfo.msWifiConnectInfo.SubnetMask&0x0000FF00)>>8)
											  ,((gsFwInfo.msWifiConnectInfo.SubnetMask&0x00FF0000)>>16)
											  ,((gsFwInfo.msWifiConnectInfo.SubnetMask&0xFF000000)>>24));
	GLogI( "SourceGateway   : %d.%d.%d.%d\r\n",(gsFwInfo.msWifiConnectInfo.Gateway&0x000000FF)
											  ,((gsFwInfo.msWifiConnectInfo.Gateway&0x0000FF00)>>8)
											  ,((gsFwInfo.msWifiConnectInfo.Gateway&0x00FF0000)>>16)
											  ,((gsFwInfo.msWifiConnectInfo.Gateway&0xFF000000)>>24));
	GLogI( "TargetIpAddress : %d.%d.%d.%d\r\n",(gsFwInfo.msWifiConnectInfo.TargetIpAddress&0x000000FF)
											  ,((gsFwInfo.msWifiConnectInfo.TargetIpAddress&0x0000FF00)>>8)
											  ,((gsFwInfo.msWifiConnectInfo.TargetIpAddress&0x00FF0000)>>16)
											  ,((gsFwInfo.msWifiConnectInfo.TargetIpAddress&0xFF000000)>>24));
	GLogI( "port : %d \n\r", gsFwInfo.msWifiConnectInfo.port);
	GLogI( "--------------------------------------------------\r\n");
}

uint8_t autoConnectAP( void )
{
	int32_t		status = 0;
	uint8_t		i;
	uint8_t		Wifi_Status[2];
	
	// Check Validate SSID
	if( ( gsFwInfo.msWifiConnectInfo.SSIDlength == 0)  || ( gsFwInfo.msWifiConnectInfo.SSIDlength > 100 ) )
	{
		GLogEE( "Connect AP Info Invalid!!!\r\n" );
		GLogEE( "SSIDlength : %d\r\n", gsFwInfo.msWifiConnectInfo.SSIDlength );

		return FALSE;
	}

    // Check AP Connected
    rsi_wlan_get( RSI_CONNECTION_STATUS, Wifi_Status, 2 );
    if( Wifi_Status[0] == WIFI_AP_CONNECTED ) 
    {
        GLogI("WIFI-AP Already Connected\r\n");
        g_ucApConnected = 1;
        return TRUE;
    }

    // Set AP settings
#ifdef TEST_MODE
	gsFwInfo.msWifiConnectInfo.PSK_length	= sizeof(GIT_PSK_KEY);				// PSK_length
	sprintf( gsFwInfo.msWifiConnectInfo.PSK_Key, GIT_PSK_KEY );					// PSK_Key
	gsFwInfo.msWifiConnectInfo.SSIDlength	= sizeof(GIT_SSID_NAME);			// SSIDlength
	sprintf( gsFwInfo.msWifiConnectInfo.SSIDname, GIT_SSID_NAME );				// SSIDname
#endif
    
    // Connect AP
    for (i = 0; i < RS9116_RETRY_COUNT; i++) {
        GLogN("AP SSID : %s, mode : %d\r\n", gsFwInfo.msWifiConnectInfo.SSIDname, gsFwInfo.msWifiConnectInfo.mode);
        GLogN("AP Connecting Count %d\r\n", i + 1);

        status = ConnectAP((uint8_t *)gsFwInfo.msWifiConnectInfo.SSIDname,
                           gsFwInfo.msWifiConnectInfo.mode,
                           (uint8_t *)gsFwInfo.msWifiConnectInfo.PSK_Key );

        if (status == 0)            
		{
		  	RSSI_SetLedIndicator();
			g_ucApConnected = 1;
			break;																// success
		}
        else                        osDelay(10);
    }

    if( i == RS9116_RETRY_COUNT )   return FALSE;

    gsFwInfo.msWifiConnectInfo.ipSetting = 0;

    // Get IP
    for (i = 0; i < RS9116_RETRY_COUNT; i++) 
	{
		if( i != 0 )			GLogN( "SetIPAddressStatic req %d \n\r", i );

        status = SetIPAddressDHCP();
        if (status == 0) break;  // success
        else osDelay(10);
    }
    DnsHandler();
    //GetWifiMacAddress();

    if (i == RS9116_RETRY_COUNT) return FALSE;

    return TRUE;
}

bool CompareConnectAP( WifiScanInfo* wifidata )
{
	int32_t		status = 0;
	uint8_t		i;
	uint8_t		Wifi_Status[2];

	// Check Validate SSID
	if( ( wifidata->SSID_Length == 0)  || ( wifidata->SSID_Length > 100 ) )
	{
		GLogEE( "Connect AP Info Invalid!!!\r\n" );
		GLogEE( "SSIDlength : %d\r\n", wifidata->SSID_Length );

		return FALSE;
	}

    // Check AP Connected
    rsi_wlan_get( RSI_CONNECTION_STATUS, Wifi_Status, 2 );
    if( Wifi_Status[0] == WIFI_AP_CONNECTED ) 
    {
        GLogI("WIFI-AP Already Connected\r\n");
        g_ucApConnected = 1;
        return TRUE;
    }

    // Set AP settings
	memset(gsFwInfo.msWifiConnectInfo.PSK_Key, 0x00, sizeof(gsFwInfo.msWifiConnectInfo.PSK_Key));
	memset(gsFwInfo.msWifiConnectInfo.SSIDname, 0x00, sizeof(gsFwInfo.msWifiConnectInfo.SSIDname));
	gsFwInfo.msWifiConnectInfo.PSK_length	= wifidata->PSK_Length;				// PSK_length
	sprintf( gsFwInfo.msWifiConnectInfo.PSK_Key, wifidata->PSK_Key );			// PSK_Key
	gsFwInfo.msWifiConnectInfo.SSIDlength	= wifidata->SSID_Length;			// SSIDlength
	sprintf( gsFwInfo.msWifiConnectInfo.SSIDname, wifidata->SSID );				// SSIDname
	gsFwInfo.msWifiConnectInfo.mode			= wifidata->Security_Mode;			// Security_Mode
	
	
    // Connect AP
    for (i = 0; i < RS9116_RETRY_COUNT; i++) {
        GLogN("AP SSID : %s, mode : %d\r\n", gsFwInfo.msWifiConnectInfo.SSIDname, gsFwInfo.msWifiConnectInfo.mode);
        GLogN("AP Connecting Count %d\r\n", i + 1);

        status = ConnectAP( (uint8_t *)gsFwInfo.msWifiConnectInfo.SSIDname,
                            gsFwInfo.msWifiConnectInfo.mode,
                            (uint8_t *)gsFwInfo.msWifiConnectInfo.PSK_Key );

        if (status == 0)            break;  // success
        else                        osDelay(10);
    }

    if( i == RS9116_RETRY_COUNT )   return FALSE;

    gsFwInfo.msWifiConnectInfo.ipSetting = 0;

    // Get IP
    for (i = 0; i < RS9116_RETRY_COUNT; i++) 
	{
		if( i != 0 )			GLogN( "SetIPAddressStatic req %d \n\r", i );

        status = SetIPAddressDHCP();
        if (status == 0)			break;  // success
        else						osDelay(10);
    }
	
	if( i == RS9116_RETRY_COUNT )   return FALSE;
	
    DnsHandler();
    GetWifiMacAddress();

    if (i == RS9116_RETRY_COUNT) return FALSE;

    return TRUE;
}

uint8_t disconnectToMQTT(void)
{
    int32_t status;
    status = rsi_emb_mqtt_disconnect();
    if (status != 0)
	{
        // Error handling
        GLogN("MQTT disconnect failed with status: %X\n", status);
    }
	else
	{
		g_mqtt_isconnected = false;
	}
	
	return status;
}
uint8_t disconnectToWebsocket(void)
{
    uint8_t status;
    status = rsi_web_socket_close(g_iClient_socket);
    if (status != 0)
	{
        // Error handling
        GLogN("Websocket disconnect failed with status: %d\n", status);
        return status;
    }
	else
	{
		g_websocket_isconnected = false;
		return status;
	}
}

uint8_t connectToMQTT(void)
{
    uint8_t				ucSubTopicName[35]	= {0};
	uint8_t				mqtt_ip_address[15]	= {0};
	uint32_t			server_address		= 0;
	uint32_t			client_port			= 0;
	uint8_t				datetime[7]			= {0};
	uint32_t			ret					= 0;
	uint8_t				status				= 0;
	int 				i					= 0;
	rsi_rsp_dns_query_t	dns_response;
	
	memset(&dns_response, 0, sizeof(dns_response));
	
	if(msServerConnectInfo.mqtt_domain[126] == 0)  // Domain mode
	{
		GLogN("Domain Mode\r\n");
		for(i = 0; i < 3; i++)
		{
			status = RequestDNS(msServerConnectInfo.mqtt_domain, &dns_response);
			if(status == RSI_SUCCESS)
			{
				sprintf(	(char *)mqtt_ip_address, "%d.%d.%d.%d",
							dns_response.ip_address[0].ipv4_address[0],
							dns_response.ip_address[0].ipv4_address[1],
							dns_response.ip_address[0].ipv4_address[2],
							dns_response.ip_address[0].ipv4_address[3] );

				server_address = ip_to_reverse_hex(mqtt_ip_address);
				break;
			}
		}
	}
	else if(msServerConnectInfo.mqtt_domain[126] == 1)  // IP mode
	{
		GLogN("IP Mode\r\n");
		sprintf( (char *)mqtt_ip_address, "%d.%d.%d.%d",
				 msServerConnectInfo.mqtt_domain[0],
				 msServerConnectInfo.mqtt_domain[1],
				 msServerConnectInfo.mqtt_domain[2],
				 msServerConnectInfo.mqtt_domain[3]
				);
		server_address = ip_to_reverse_hex(mqtt_ip_address);
		i = 0;  // Set as if DNS succeeded
	}
	
	GLogN("Connect MQTT IP : %d.%d.%d.%d\r\n", server_address & 0xFF, (server_address >> 8) & 0xFF, (server_address >> 16) & 0xFF, (server_address  >> 24) & 0xFF);
	
	if(i == 3)
	{
		ret = 1;
		return ret;
	}
	
	Get_RTCData(datetime);
	client_port = (((datetime[5] * 60) + datetime[6]) % 49151) + 2000;

	/* keep_alive_interval = 45s : NAT/방화벽 idle TCP termination 차단 +
	 * broker side keep-alive 안전망 활성화.
	 * 임계값 좁히기 결과:
	 *   - 30s: 일부 chip/SDK 조합에서 0xFF82 (EACCES) 일관 거부
	 *   - 45s: 정상 작동
	 *   - 60s: 정상 작동
	 * → 45s 채택 (안정 작동 최소값). broker keepalive timeout ≈ 1.5×45 = 67.5s. */
	GLogN("MQTT Port: %d, TLS: %s, client_port: %d\r\n",
		msServerConnectInfo.mqtt_port,
		(msServerConnectInfo.mqtt_port == 8883) ? "ON" : "OFF",
		client_port);

	if(msServerConnectInfo.mqtt_port == 8883) // TLS
	{
		ret = rsi_emb_mqtt_client_init( (int8_t *)&server_address,
							 		msServerConnectInfo.mqtt_port,
							 		client_port,
							 		RSI_EMB_MQTT_CLEAN_SESSION | RSI_EMB_MQTT_SSL_ENABLE,
							 		45,
							 		gsFwInfo.marrucSerialNo,
							 		(int8_t *)MQTT_USERNAME,
							 		(int8_t *)MQTT_PASSWORD);
	}
	else // NONE_TLS
	{
		ret = rsi_emb_mqtt_client_init( (int8_t *)&server_address,
							 		msServerConnectInfo.mqtt_port,
							 		client_port,
							 		RSI_EMB_MQTT_CLEAN_SESSION,
							 		45,
							 		gsFwInfo.marrucSerialNo,
							 		(int8_t *)MQTT_USERNAME,
							 		(int8_t *)MQTT_PASSWORD);
	}
	
	GLogN("rsi_emb_mqtt_client_init:%X\r\n",ret);
	
	if( ret == 0 )
	{
		ret = rsi_emb_mqtt_connect(RSI_EMB_MQTT_USER_FLAG | RSI_EMB_MQTT_PWD_FLAG, NULL, 0, NULL);	//rsi_emb_mqtt_connect(uint8_t mqtt_flags, int8_t *will_topic, uint16_t will_message_len, int8_t *will_message)
		GLogN("rsi_emb_mqtt_connect:%X\r\n",ret);
		if( ret == 0 )
		{
			GetTopicSubscribe(ucSubTopicName);
			ret = rsi_emb_mqtt_subscribe(QOS1, ucSubTopicName);			
			GLogN("rsi_emb_mqtt_subscribe:%X\r\n",ret);
			if( ret == 0 )
			{
				ret = rsi_emb_mqtt_register_call_back(RSI_WLAN_NWK_EMB_MQTT_PUB_MSG_CB, GIT_MQTT_receive_callback);	
				GLogN("GIT_MQTT_receive_callback error:%X\r\n",ret);
				ret = rsi_emb_mqtt_register_call_back(RSI_WLAN_NWK_EMB_MQTT_REMOTE_TERMINATE_CB,rsi_emb_mqtt_remote_socket_terminate_handler);
				GLogN("rsi_emb_mqtt_remote_socket_terminate_handler error:%X\r\n",ret);
				ret = rsi_emb_mqtt_register_call_back(RSI_WLAN_NWK_EMB_MQTT_KEEPALIVE_TIMEOUT_CB, rsi_emb_mqtt_ka_timeout_handler);
				GLogN("rsi_emb_mqtt_ka_timeout_handler error:%X\r\n",ret);
				g_mqtt_isconnected = true;

				FL_GitWifiConnectInfo(PACKET_MQTT);
				LED_SetState(eLED_SERVER_CONNECTED, 0, 0);
			}
		}
	}
	
	//SaveConnectionLog();
	return ret;
}

int32_t connectToWebsocket(void)
{
 	int32_t     status;
    int         flags 						= 0;								//RSI_SSL_ENABLE;
	uint8_t		websocket_ip_address[15]	= {0};
	uint16_t	server_port					= 80;								// Server Port
	uint8_t     resource_name[]				= "/websocket";						// Web resource name
	uint8_t     host_name[]					= "ws.git-connect.com";				// Web host name
	//uint8_t     host_name[]					= "172.20.10.12";				// Web host name
	uint32_t	server_address				= 0;
    uint16_t	client_port					= 0;
	uint8_t		datetime[7]					= {0};
	int			i							= 0;

	rsi_rsp_dns_query_t	dns_response;

	memset(&dns_response, 0, sizeof(dns_response));

	if(msServerConnectInfo.websocket_domain[126] == 0)  // Domain mode
	{
		GLogN("Domain Mode\r\n");
		for(i = 0; i < 3; i++)
		{
			status = RequestDNS(msServerConnectInfo.websocket_domain, &dns_response);
			if(status == RSI_SUCCESS)
			{
				sprintf(	(char *)websocket_ip_address, "%d.%d.%d.%d",
							dns_response.ip_address[0].ipv4_address[0],
							dns_response.ip_address[0].ipv4_address[1],
							dns_response.ip_address[0].ipv4_address[2],
							dns_response.ip_address[0].ipv4_address[3] );

				server_address = ip_to_reverse_hex(websocket_ip_address);
				break;
			}
		}
	}
	else if(msServerConnectInfo.websocket_domain[126] == 1)  // IP mode
	{
		GLogN("IP Mode\r\n");
		sprintf( (char *)websocket_ip_address, "%d.%d.%d.%d",
				 msServerConnectInfo.websocket_domain[0],
				 msServerConnectInfo.websocket_domain[1],
				 msServerConnectInfo.websocket_domain[2],
				 msServerConnectInfo.websocket_domain[3]
				);
		server_address = ip_to_reverse_hex(websocket_ip_address);
		i = 0;  // Set as if DNS succeeded
	}

	GLogN("Connect WebSocket IP : %d.%d.%d.%d\r\n", server_address & 0xFF, (server_address >> 8) & 0xFF, (server_address >> 16) & 0xFF, (server_address  >> 24) & 0xFF);

	if(i == 3)
	{
		status = 1;
		return status;
	}

	Get_RTCData(datetime);
	client_port = (((datetime[5] * 60) + datetime[6]) % 49151) + 2000;
	
    // Web socket creation
    status = rsi_web_socket_create( flags,										// flags
                                    (uint8_t*)&server_address,					// Server IP address
                                    msServerConnectInfo.websocket_port,			// Server port number
                                    client_port,								// Server port number
                                    msServerConnectInfo.websocket_resource,		// Web resource name
                                    host_name,									// Web host name
                                    &g_iClient_socket,							// Socket ID
                                    GIT_sock_receive_callback );				// �ݹ� �Լ�
    
    if (status != 0) {
        // Error handling0
		switch(status)
		{
			case 0xffffffff:
				GLogE( "Timeout Error(0x%04X)\r\n", status );
				break;
			case 0xfffffffe:
				GLogE( "Invalid parameter(0x%04X)\r\n", status );
				break;
			case 0xfffffffc:
				GLogE( "Packet allocation failure(0x%04X)\r\n", status );
				break;
			default:
				GLogE( "Error(0x%04X)\r\n", status );
		}
    }
    else
	{
		g_websocket_isconnected = true;
		//FL_GitWifiConnectInfo(PACKET_WEBSOCKET);
		GLogN("WebSocket created successfully with socket ID: %d\r\n", g_iClient_socket);
		LED_SetState(eLED_SERVER_CONNECTED, 0, 0);
	}
	//SaveConnectionLog();
    return status;
}

/*----------------------------------------------------------------------
 *   CallBack Functions
 *--------------------------------------------------------------------*/
void GIT_MQTT_receive_callback(uint16_t status, uint8_t *buffer, const uint32_t length)
{
  	rsi_mqtt_rcv_pub_async_pkt_t *rcv_data = (rsi_mqtt_rcv_pub_async_pkt_t *)buffer;
	uint8_t More_data_flag = rcv_data->mqtt_flags & BIT(4);					// More_data_flag( 0x01 : more data exists, 0x00 : no more data )
	uint8_t Qos_level = (rcv_data->mqtt_flags & (BIT(1) | BIT(2))) >> 1;; 	// QOS Level ( 0x00 : QOS0, 0x02 : QOS1, 0x04 : QOS2 )
	
    if (g_bMqttLogOnRxFlag == true) 
	{
        for (uint32_t j = 6 + rcv_data->topic_length; j < 6 + rcv_data->topic_length + rcv_data->current_chunk_length; j++)
		{
            GLogN("%02X ", buffer[j]);
        }
        GLogN("* %d", length);
        GLogN("\r\n");
    }
	
	xStreamBufferSend(hSBMqttRx, &buffer[6 + rcv_data->topic_length], rcv_data->current_chunk_length, pdMS_TO_TICKS(100));
}

void mqtt_subscribe_callback(MessageData *md){
    GLogN("Topic : %s\r\n", md->topicName);
    GLogN(" data : %s\r\n", md->message);
}


void GIT_sock_receive_callback(uint16_t sock_no, uint8_t *buffer, uint32_t length)
{
	if (g_bwebLogOnRxFlag == true) 
	{
        for (uint32_t j = 1; j < length - 1; j++) {
            GLogN("%02X ", buffer[j]);
        }
        GLogN("* %d", length - 1);
        GLogN("\r\n");
    }
	
	// Data buffering part - hSBWebSocketRx queue
	xStreamBufferSend(hSBWebSocketRx, &buffer[1], length-1, pdMS_TO_TICKS(100));
}


void socket_terminate_callback( uint16_t status, uint8_t *buffer, const uint32_t length )
{
	GLogI( "terminate status(0x%04X) : ",status );

	g_websocket_isconnected = false;
	if( g_OBD_Processing == true )
	{
		printf("Socket Terminated -> ListSensor STOP req\r\n");
		g_ListSensor_Endflag = false;
	}
	if( status == 0 )			GLogI( "Socket terminated!!!\r\n" );
}

void socket_notify_callback( uint16_t status, uint8_t *buffer, const uint32_t length )
{
	GLogI( "notify status(0x%04X) : ",status );
	g_socket_status = status;
	
	switch( status )
	{
		case	0x0000 :
		{
			GLogN( "Socket Connect OK!!!!!!!\r\n" );			
			break;
		}
		case	0xff87 :						GLogN( "non-existent server!!\r\n" );				break;
		case	0xff81 :						GLogN( "Socket already open!!\r\n" );				break;
        case	0xff88 :						GLogN( "WebSocket creation failed!!\r\n" );         break;
		case	0x0021 :
		{
			GLogN( "Command given in incorrect state!!\r\n" );			// Command given in wrong state//This error occurs when called in wrong state before DHCP IP allocation
			break;
		}

		default :								GLogN( "Not define state!!\r\n" );
	}
}

/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/
static void WifiStatusTh( void const * argument )
{
//	GLogN( "\r\n[Wifi thread Start]\n\r" );
	
	uint32_t	status		= 0;
	
	for(;;)
	{
#if 1
	  	rsi_wlan_get( RSI_CONNECTION_STATUS, &g_wlanstatus, sizeof(g_wlanstatus) );
		
		switch (connState)
		{
			case CONNECTION_INITIALIZE:
				//GLogI("CONNECTION_INITIALIZE\r\n");
				if (g_eMainState == eMain_Run)
				{
					connState = CONNECTION_WIFI_CONNECT;
				}
				break;

			case CONNECTION_WIFI_CONNECT:
				GLogI("CONNECTION_WIFI_CONNECT\r\n");
				status = autoConnectAP();
				if(status) 
				{
					connState = CONNECTION_MQTT_CONNECT;
					GLogN( "ConnectAP OK! \r\n" );
				}
				else
				{
					connState = CONNECTION_WIFI_SCANNING;
				}
				
				break;
				
			case CONNECTION_WIFI_SCANNING:
				GLogI("CONNECTION_WIFI_SCANNING\r\n");
				status = compare_connect_wifi();
				
				if(status)
				{
					connState = CONNECTION_MQTT_CONNECT;
				}
				else
				{
					GLogI("CONNECTION_NOT_CONNECT\r\n");
					connState = CONNECTION_NOT_CONNECT;
				}
				break;
				
			case CONNECTION_MQTT_CONNECT:
				GLogI("CONNECTION_MQTT_CONNECT\r\n");
				LED_ALL_OFF;
				LED_SetState(eLED_SERVER_SCAN, 60*1000, 500);
				Buzzer_Control( eBUZZER_DOMISOL, MSEC(200), MSEC(0), 3 );	// 서버 페어링 멜로디
				if(g_websocket_isconnected == 1)
				{
					disconnectToWebsocket();
				}
				for(int i = 0; i < 3; i++)
				{
					GLogN("MQTT Connect Count %d/3\r\n", i + 1);
					status = connectToMQTT();
					if(status == 0)
					{
						GLogN("Connected to MQTT successfully.\r\n");
						connState = CONNECTION_ACTIVE;
						break;
					}
				}

				if(status != 0)
				{
					switch(status)
					{
						case 0x0004:
							GLogN("Broker ID, PW incorrect.\r\n");
							break;

						case 0xFF87:
							GLogN("Socket IP, PORT incorrect.\r\n");
							break;
						default:
							GLogN("MQTT connect failed after 3 retries: 0x%04X\r\n", status);
							break;
					}
					connState = CONNECTION_NOT_CONNECT;
				}
				break;
				
			case CONNECTION_WEBSOCKET_CONNECT:
				GLogI("CONNECTION_WEBSOCKET_CONNECT\r\n");
				if(g_mqtt_isconnected == 1)
				{
					if (disconnectToMQTT() == TRUE)
					{
						GLogN("Disconnected to MQTT successfully.\n");
					}
				}
				
				status = connectToWebsocket();

				if(status == 0)
				{
					connState = CONNECTION_ACTIVE;
				}
				else
				{
					switch(g_socket_status)
					{
						case 0xFF87:
							GLogN("Socket IP, PORT incorrect. %x\r\n", g_socket_status);
							break;

						case 0xFF88:
							GLogN("Socket IP, PORT incorrect. %x\r\n", g_socket_status);
							break;
						case 0x00D2:
							GLogN("SSL/TLS Handshake Failed. Socket will be closed. %x\r\n", g_socket_status);
						default:
							GLogN("Invalid connection. %x\r\n", g_socket_status);
							break;
					}
					disconnectToWebsocket();
					connState = CONNECTION_NOT_CONNECT;
				}
				break;

			case CONNECTION_ACTIVE:
				if(g_wlanstatus == 0)
				{
					/* Wi-Fi 자체가 끊긴 경우: 처음부터 (Wi-Fi 재스캔) 재시도 */
					g_ucApConnected = 0;

					if( g_OBD_Processing == true )
					{
						printf("Wi-Fi/AP disconnected -> ListSensor STOP req\r\n");
						g_ListSensor_Endflag = false;
					}

					if(g_mqtt_isconnected == true || g_websocket_isconnected == true)
					{
						status = rsi_emb_mqtt_destroy();
						g_mqtt_isconnected = false;
						g_websocket_isconnected = false;
					}
					connState = CONNECTION_WIFI_SCANNING;
				}
				else if(g_ucApConnected == 1
				        && g_mqtt_isconnected == false
				        && g_websocket_isconnected == false)
				{
					/* Wi-Fi는 살아있는데 MQTT/WebSocket만 끊긴 경우:
					 * broker가 보낸 TCP close(REMOTE_TERMINATE) 또는 PINGRESP 손실
					 * 등으로 MQTT만 죽었을 때 자동 재연결한다. 이 분기가 없으면
					 * CONNECTION_ACTIVE 에 영원히 머물러 수동 power cycle 필요. */
					GLogN("Server disconnected while Wi-Fi alive, reconnect MQTT\r\n");
					(void)rsi_emb_mqtt_destroy();    /* SDK 내부 세션 정리 */
					connState = CONNECTION_MQTT_CONNECT;
				}
				break;

			case CONNECTION_NOT_CONNECT:
				// 서버 연결 타임아웃 처리는 freertos.c의 Connection_Check()에서 통합 관리
				// BT, USB, MQTT 모두 미연결 시에만 알람 발생 (서버 우선 - Yellow LED)
				break;

			case CONNECTION_ERROR:
				GLogI("CONNECTION_ERROR\r\n");
				connState = CONNECTION_INITIALIZE; // Return to initialization state when error occurs
				break;

			default:
				GLogI("default\r\n");
				connState = CONNECTION_ERROR; // Handle disconnection or error
				break;
    	}
#endif
		osDelay(10);
	}
}


bool m_bBtScanPressed = false;

#define MAX_BT_SCAN_OCCURED_TIME_OUT (1*1000)

void rsi_bt_scan_handler()
{
	static bool s_bEvtScanPressed = false;
	static bool s_bScanPressed = false;
	static uint32_t s_unTimeout = 0;
	
	if( m_bBtScanPressed == true )
	{

		m_bBtScanPressed = false;

		s_bEvtScanPressed = true;
		
		// low active
		if( HAL_GPIO_ReadPin(GPIOF, PAIR_SW_Pin) == GPIO_PIN_RESET )
		{
			m_eBtScanHandlerState = eBtScanHandler_Init;
		}
		else
		{
			if( m_eBtScanHandlerState < eBtScanHandler_Occured )
			{
				m_eBtScanHandlerState = eBtScanHandler_Idle;
			}
		}
	}

	switch(m_eBtScanHandlerState)
	{
		case eBtScanHandler_Init:

			GLogN("%s] eBtScanHandler_Init\r\n", __func__);

		
			m_eBtScanHandlerState = eBtScanHandler_Wait;
			s_unTimeout = Get_Tmr();
			break;
		case eBtScanHandler_Wait:

			if( (Get_Tmr() - s_unTimeout) > MAX_BT_SCAN_OCCURED_TIME_OUT )
			{
				GLogN("%s] eBtScanHandler_Wait : Time out\r\n", __func__);
				m_eBtScanHandlerState = eBtScanHandler_Occured;	
			}			
			
			break;
		case eBtScanHandler_Idle:
			break;			
		case eBtScanHandler_Occured:

			GLogN("%s] eBtScanHandler_Occured\r\n", __func__);
			
			m_eBtScanHandlerState = eBtScanHandler_Scanning;	

			rsi_bt_app_on_scan_req();

			// clear connection check to prevent led disturbing in scanning mode.
			ClearConnctionLedCheck();

			LED_ALL_OFF;
			LED_SetState(eLED_BT_SCAN, 60*1000, 500);
			Buzzer_Control( eBUZZER_DOMISOL, MSEC(200), MSEC(0), 3 );

			osDelay(500);
			
			break;

		case eBtScanHandler_Scanning:

			if( TotalScanTimeOutFlag == 0 )
			{
				m_eBtScanHandlerState = eBtScanHandler_Idle;

				LED_ALL_OFF;

				if( g_ucBtConnected == BT_INITAILIZE )
				{
					LED_SetState(eLED_NOTI, 30*1000, 300);
				}
				else
				{
					LED_SetState(eLED_NORMAL, 0, 0);
				}

				//rsi_bt_cancel_inquiry();
				
				osTimerStop(InquiryTimer);
				osTimerStop(TotalScanTimer);
			}
			break;
	}
	
}

static void BluetoothStatusTh( void const * argument )
{
	while( 1 )
	{
		rsi_bt_app_task();
	}
}

uint8_t	GetBluetoothConnectionStatus()
{
	return g_ucBtConnected;
}

void RequestBtDisconnect()
{
	GLogI("%s] bluetooth disconnect#1\r\n",__func__);
	rsi_bt_disconnect((int8_t *)str_conn_bd_addr);
	GLogI("%s] bluetooth disconnect#2\r\n",__func__);
}

char* GetBtConnectedAddr()
{
	return str_conn_bd_addr;
}

int32_t Set_Eir_Data(char * device_name)
{
	int32_t uRet = 0; 
	uint8_t eir_data[200] = {2,1,0}; //! prepare Extended Response Data 
	eir_data[3] = strlen(device_name) + 1; 
	eir_data[4] = 9;
	strncpy(&eir_data[5], device_name,eir_data[3]-1); //! set eir data 
	uRet = rsi_bt_set_eir_data (eir_data, strlen (device_name) + 5);
	return uRet;
}
volatile uint8_t ping_rsp_received;
volatile uint32_t g_pingStartTick;
volatile uint32_t g_pingEndTick;
volatile int16_t  g_pingRspStatus;

//! ping response notify call back handler
void rsi_ping_response_handler(uint16_t status, const uint8_t *buffer, const uint16_t length)
{
	UNUSED_CONST_PARAMETER(buffer);
	UNUSED_CONST_PARAMETER(length);

	g_pingEndTick = HAL_GetTick();
	g_pingRspStatus = (int16_t)status;
	ping_rsp_received = 1;
}

void* memstr(const void* haystack, size_t haystack_len, const void* needle, size_t needle_len) {
    // If needle length is 0, return start address of haystack
    if (needle_len == 0) {
        return (void*)haystack;
    }

    // If haystack_len is less than needle_len, return NULL
    if (haystack_len < needle_len) {
        return NULL;
    }

    const unsigned char* h = (const unsigned char*)haystack;
    const unsigned char* n = (const unsigned char*)needle;

    // Search for needle in haystack
    for (size_t i = 0; i <= haystack_len - needle_len; ++i) {
        if (h[i] == n[0]) {
            // If first byte matches, check remaining bytes
            if (memcmp(&h[i], n, needle_len) == 0) {
                return (void*)&h[i];
            }
        }
    }

    // Return NULL if needle not found
    return NULL;
}

bool compare_connect_wifi(void)
{
    FIL				wifi_info;
    char			Filename[] = "Wifi_info.dat";
    uint8_t			wifi_file[490]	= {0};		// File buffer initialization
    WifiScanInfo	wifidata[5]		= {0};		// Structure array initialization
    int32_t			status			= 0;
    rsi_rsp_scan_t	scan_result		= {0};		// Scan result structure initialization
    uint8_t			len				= 0;
    uint32_t		wifiCount		= 0;
    UINT			nReadLen		= 0;		// Read byte count initialization
	bool			result			= false;

    // Directory change and creation
    f_mkdir("/wifi_info");
    f_chdir("/wifi_info");

    // File open (read mode)
    if (f_open(&wifi_info, Filename, FA_READ) == FR_OK)
    {
        f_read(&wifi_info, wifi_file, sizeof(wifi_file), &nReadLen);
        f_close(&wifi_info);
        f_chdir(DIR_ROOT);		// restore cwd to root (don't leave it at /wifi_info)
    }
    else
    {
        GLogN("File open error!!!\r\n");
        f_chdir(DIR_ROOT);		// restore cwd to root (don't leave it at /wifi_info)
        return false;
    }

    wifiCount = nReadLen / 98;

    if(wifiCount > 5)
    {
        wifiCount = 5;
    }

	if(wifiCount == 0)
	{
		return false;
	}
    // Parse file data into wifidata array
    for (uint8_t i = 0; i < wifiCount; i++)
    {
        uint16_t offset = i * 98;

        wifidata[i].SSID_Length   = wifi_file[offset];
        memcpy(wifidata[i].SSID, &wifi_file[offset + 1], 32);
        wifidata[i].PSK_Length    = wifi_file[offset + 33];
        memcpy(wifidata[i].PSK_Key, &wifi_file[offset + 34], 63);
        wifidata[i].Security_Mode = wifi_file[offset + 97];
    }

    // 저장된 WiFi 목록 출력
//    GLogN("=== Saved WiFi List (eMMC) ===\r\n");
//    for(uint8_t i = 0; i < wifiCount; i++)
//    {
//        GLogN("[%d] SSID: %s\r\n", i, wifidata[i].SSID);
//    }

    // WiFi scan start
	for(uint8_t k = 0; k < 3; k++)
	{
		status = rsi_wlan_scan(NULL, 0, &scan_result, sizeof(scan_result));
		if (status == 0)
		{
			// 스캔된 WiFi 목록 출력
			//GLogN("=== Scanned WiFi List ===\r\n");
			for (uint8_t i = 0; i < scan_result.scan_count[0]; i++)
			{
				GLogN("[%d] SSID: %s\r\n", i, scan_result.scan_info[i].ssid);
			}

			//GLogN("=== Comparing... ===\r\n");
			for (uint8_t i = 0; i < scan_result.scan_count[0]; i++)
			{
				size_t scanSsidLen = strlen(scan_result.scan_info[i].ssid);
				for (uint8_t j = 0; j < wifiCount; j++)
				{
					size_t wifiSsidLen = strlen(wifidata[j].SSID);
					/* Skip empty/corrupt saved records: with wifiSsidLen == 0,
					 * len becomes 0 and memcmp(...,0) returns 0, falsely matching
					 * every scanned SSID. Guard before comparing. */
					if (wifiSsidLen == 0) continue;
					len = (scanSsidLen < wifiSsidLen) ? scanSsidLen : wifiSsidLen;
					if (memcmp(scan_result.scan_info[i].ssid, wifidata[j].SSID, len) == 0)
					{
					  	//GLogN("MATCH! Scan[%d]:%s == Saved[%d]:%s\r\n", i, scan_result.scan_info[i].ssid, j, wifidata[j].SSID);
						result = CompareConnectAP(&wifidata[j]);
						
						if(result == true)
						{
						  	gsFwInfo.mucChanged = TRUE;
							saveFirmwareInfo_EMMC(true);
							return result;
						}
					}
				}
			}
		}
		
		osDelay(1000);
	}
	
	return result;
}


uint32_t RequestDNS(uint8_t *domain_name, rsi_rsp_dns_query_t *dns_response)
{
  	uint32_t status = RSI_SUCCESS;
	status = rsi_dns_req(RSI_IP_VERSION_4, domain_name, NULL, NULL, dns_response, sizeof(rsi_rsp_dns_query_t));

    if (status != RSI_SUCCESS)
	{
        GLogN("DNS Fail: %X\r\n", status);
    }
}

void rsi_emb_mqtt_remote_socket_terminate_handler(uint16_t status, uint8_t *buffer, const uint32_t length)
{
  int32_t wlan_err = rsi_wlan_get_status();

  UNUSED_PARAMETER(buffer);       //This statement is added only to resolve compilation warning, value is unchanged
  GLogN("emb_mqtt_terminate!!! status=0x%04X len=%lu wlan_err=0x%08lX\r\n",
        status,
        (unsigned long)length,
        (unsigned long)wlan_err);
  g_mqtt_isconnected = false;
  if( g_OBD_Processing == true )
  {
    printf("MQTT Socket disconnected by broker/server -> ListSensor STOP\r\n");
    g_ListSensor_Endflag = false;
  }
}

void rsi_emb_mqtt_publish_receive_handler(uint16_t status, uint8_t *buffer, const uint32_t length)
{
  UNUSED_PARAMETER(status);       //This statement is added only to resolve compilation warning, value is unchanged
  UNUSED_PARAMETER(buffer);       //This statement is added only to resolve compilation warning, value is unchanged
  UNUSED_CONST_PARAMETER(length); //This statement is added only to resolve compilation warning, value is unchanged

  char topic_name[RSI_EMB_MQTT_TOPIC_MAX_LEN];
  rsi_mqtt_rcv_pub_async_pkt_t *rcv_data = (rsi_mqtt_rcv_pub_async_pkt_t *)buffer;
  LOG_PRINT("\r\nMQTT Flags: %d\r\n", rcv_data->mqtt_flags);
  LOG_PRINT("\r\nMQTT Message length: %d\r\n", rcv_data->current_chunk_length);
  LOG_PRINT("\r\nMQTT Topic length: %d\r\n", rcv_data->topic_length);
  strncpy(topic_name, (char *)rcv_data->topic, rcv_data->topic_length);
  topic_name[rcv_data->topic_length] = '\0';
  LOG_PRINT("\r\nMQTT Topic: %s\r\n", topic_name);
  LOG_PRINT("\r\nMQTT Message: %s\r\n", rcv_data->topic + rcv_data->topic_length);
  memset(buffer, 0, length);
  UNUSED_PARAMETER(status);
}

void rsi_emb_mqtt_ka_timeout_handler(uint16_t status, uint8_t *buffer, const uint32_t length)
{
  UNUSED_PARAMETER(status);       //This statement is added only to resolve compilation warning, value is unchanged
  UNUSED_PARAMETER(buffer);       //This statement is added only to resolve compilation warning, value is unchanged
  UNUSED_CONST_PARAMETER(length); //This statement is added only to resolve compilation warning, value is unchanged
  GLogN("KeepAlive Handler Callback!!!\r\n");
}

#define HTTP_SERVER_IP_ADDRESS "cdp.git-connect.com"
//#define HTTP_SERVER_IP_ADDRESS "172.20.10.12"
#define HTTPS_PORT              443
#define HTTP_RESOURCE          "/fw/HDO146401.bin"
//#define HTTP_RESOURCE          "/firmware.bin"
#define HTTP_HOSTNAME          "172.20.10.12"
#define HTTP_EXTENDED_HEADER   NULL
#define USERNAME               NULL
#define PASSWORD               NULL
char extended_header[] = 
  "Content-Type: application/json\r\n"
  "Accept: */*\r\n"
  "User-Agent: RS9116_Example\r\n"
  "Connection: keep-alive\r\n\r\n";

uint8_t	g_http_fwupdate_url[100] = {0}; 

extern FIL g_TempFilepnt;
// HTTP GET flag setting
// Default GET request option flag set to 0 for no HTTPS, IPv4 usage and other options
#define HTTPS_FLAGS					BIT(1) | BIT(3) | BIT(6)		// BIT(1) | BIT(3) | BIT(6)

// Application buffer variable (stores received data)
uint32_t app_buf_len = 0;
uint8_t test_d[5000] = {0};
// HTTP GET request completion flag variable (updated in asynchronous callback)
volatile int32_t http_get_status = 0;
#define RSI_HTTP_CLIENT_SUCCESS 1

/**
 * @brief Callback function that handles HTTP GET response
 *
 * @param status	- Request result status (RSI_SUCCESS, etc)
 * @param buffer	- Received response buffer
 * @param length	- Received response length
 * @param moredata	- 1 if more data exists, 0 if no additional data to come
 */
void rsi_http_client_get_response_handler(uint16_t status, const uint8_t *buffer, const uint16_t length, const uint32_t moredata)
{
    static FRESULT res;
    static UINT written;
	uint8_t cResult = 0;
	
	memcpy(test_d, buffer, length);

    // Open temp file on first chunk received
    if (status == RSI_SUCCESS && app_buf_len == 0)
    {
		tt_start = Get_Tmr();
        cResult = WriteUpdateDataTemp(test_d, length);
        if (cResult != FR_OK)
        {
            GLogE("file write fail : %d\r\n", res);
            http_get_status = (U8)res;
            return;
        }
    }
	else if(status != RSI_SUCCESS)
	{
		GLogE("ERROR : %X\r\n", status);
		return;
	}
	
	
	
    if (status == RSI_SUCCESS)
    {
        // Write received data to temp file
        cResult = WriteUpdateDataTemp(test_d, length);
        if (cResult != FR_OK || written != length)
        {
            GLogE("file write fail [res:%d, written:%u]\r\n", res, written);
        }

        app_buf_len += length;

        // Close file on last chunk received
        if (moredata == 1)
        {
			U32 u32Temp = CalCRC32();
			if(g_fwupdate_crc32 == u32Temp)
			{
				gsFwInfo.msAppInfo[g_ucDownloadFW_FileNo].mCheckSum = CheckVCIFWCheckSum();
				gsFwInfo.msAppInfo[g_ucDownloadFW_FileNo].mSize = g_u32UpdateFileSize;
				gsFwInfo.msAppInfo[g_ucDownloadFW_FileNo].mSectorCount = (g_u32UpdateFileSize/131072 + 1);
				
				cResult = DownloadClose();
				http_get_status = RSI_HTTP_CLIENT_SUCCESS;
				GLogN("Receive Finish!!\r\n");
			}
			else
			{
				GLogE("FWUpdate CRC incorrect!!");
				cResult = 1;
			}
            TransmitFunction(PACKET_MQTT, &cResult, 1, 0x0257);
        }
    }
    else
    {
        // Print error on error occurrence
        http_get_status = status;
        switch (status)
        {
            case 0xBB38:
                GLogN("Trying to connect non-existing TCP server socket : %X\r\n", status);
                break;
            default:
                GLogN("Fail status : %X\r\n", status);
        }
    }
}

/**
 * @brief Main function that performs HTTP GET request asynchronously
 *
 * @return int32_t  - Request result and error handling status
 */
int32_t http_get_example(void)
{
    int32_t				status = 0;
    uint32_t			server_address = 0;
    rsi_rsp_dns_query_t	dns_response;
	uint8_t				http_ip_address[15]	= {0};
	http_get_status = 0;
	app_buf_len = 0;
	
	status = RequestDNS(HTTP_SERVER_IP_ADDRESS, &dns_response);
	if(status == RSI_SUCCESS)
	{
		 sprintf(	(char *)http_ip_address, "%d.%d.%d.%d",
					dns_response.ip_address[0].ipv4_address[0],
					dns_response.ip_address[0].ipv4_address[1],
					dns_response.ip_address[0].ipv4_address[2],
					dns_response.ip_address[0].ipv4_address[3] );
//		 sprintf(	(char *)http_ip_address,"%d.%d.%d.%d",
//					172,
//					20,
//					10,
//					12 );
		 GLogN("ip : %d.%d.%d.%d\r\n", dns_response.ip_address[0].ipv4_address[0], dns_response.ip_address[0].ipv4_address[1], dns_response.ip_address[0].ipv4_address[2], dns_response.ip_address[0].ipv4_address[3]);
		 GLogN("port : %d\r\n", HTTPS_PORT);
	}
	
    // rsi_http_client_get_async API call
    status = rsi_http_client_get_async(HTTPS_FLAGS,
                                       (uint8_t *)http_ip_address,
                                       HTTPS_PORT,
                                       (uint8_t *)HTTP_RESOURCE,
                                       (uint8_t *)HTTP_HOSTNAME,
                                       (uint8_t *)extended_header,
                                       (uint8_t *)USERNAME,
                                       (uint8_t *)PASSWORD,
                                       rsi_http_client_get_response_handler);
    if(status != RSI_SUCCESS)
    {
        return status;
    }
    
//    // Wait for request completion (busy-wait, avoid in RTOS environment)
//    while(http_get_status == 0)
//    {
//        rsi_wireless_driver_task();
//    }
    
    // After completion, received data is stored in app_buf
    return RSI_SUCCESS;
}

int32_t http_get_FWupdate(void)
{
    int32_t				status = 0;
    uint32_t			server_address = 0;
    rsi_rsp_dns_query_t	dns_response;
	uint8_t				http_ip_address[15]	= {0};
	http_get_status = 0;
	app_buf_len = 0;
	
	status = RequestDNS(g_http_fwupdate_url, &dns_response);
	if(status == RSI_SUCCESS)
	{
		 sprintf(	(char *)http_ip_address, "%d.%d.%d.%d",
					dns_response.ip_address[0].ipv4_address[0],
					dns_response.ip_address[0].ipv4_address[1],
					dns_response.ip_address[0].ipv4_address[2],
					dns_response.ip_address[0].ipv4_address[3] );
//		 sprintf(	(char *)http_ip_address,"%d.%d.%d.%d",
//					172,
//					20,
//					10,
//					12 );
		 GLogN("ip : %d.%d.%d.%d\r\n", dns_response.ip_address[0].ipv4_address[0], dns_response.ip_address[0].ipv4_address[1], dns_response.ip_address[0].ipv4_address[2], dns_response.ip_address[0].ipv4_address[3]);
		 GLogN("port : %d\r\n", HTTPS_PORT);
	}
	
    // rsi_http_client_get_async API Call
    status = rsi_http_client_get_async(HTTPS_FLAGS,
                                       (uint8_t *)http_ip_address,
                                       HTTPS_PORT,
                                       (uint8_t *)HTTP_RESOURCE,
                                       (uint8_t *)HTTP_HOSTNAME,
                                       (uint8_t *)extended_header,
                                       (uint8_t *)USERNAME,
                                       (uint8_t *)PASSWORD,
                                       rsi_http_client_get_response_handler);
    if(status != RSI_SUCCESS)
    {
        return status;
    }
    
//    // Wait for request completion (busy-wait, avoid in RTOS environment)
//    while(http_get_status == 0)
//    {
//        rsi_wireless_driver_task();
//    }
    
    // After completion, received data is stored in app_buf
    return RSI_SUCCESS;
}

void SaveLastAddress(uint16_t last_address)
{
    WriteBackupSRAM(LAST_ADDRESS_OFFSET, (uint8_t)(last_address & 0xFF));
    WriteBackupSRAM(LAST_ADDRESS_OFFSET + 1, (uint8_t)(last_address >> 8));
}

uint16_t LoadLastAddress(void)
{
    uint8_t low, high;
	uint16_t last_address;
		
    ReadBackupSRAM(LAST_ADDRESS_OFFSET, &low);
    ReadBackupSRAM(LAST_ADDRESS_OFFSET + 1, &high);
    last_address = (uint16_t)((high << 8) | low);
    
    if (last_address < LOG_START_ADDRESS || last_address > LOG_END_ADDRESS)
	{
			return LOG_START_ADDRESS - sizeof(ConnectionLog);
    }
	else	return last_address;
}

/*
Recent updates: ssid, ap password (on/off), connection type (mqtt/websocket), connection time setting, server address setting
*/


void SaveConnectionLog(void)
{
    ConnectionLog log;
	
    memset(&log, 0, sizeof(ConnectionLog));
    strncpy((char*)log.ap_ssid, gsFwInfo.msWifiConnectInfo.SSIDname, sizeof(log.ap_ssid) - 1);
    log.ap_status = g_wlanstatus;			// 0 : not conncet, 1 : connect
	if(g_mqtt_isconnected == 1)				//mqtt connect
	{
		log.comm_type = COMM_MQTT;
	}
	else if(g_websocket_isconnected == 1)	//websocket connect�
	{
		log.comm_type = COMM_WEBSOCKET;
	}
	else									//not connect
	{
		log.comm_type = COMM_DISCONNECTED;
	}
    
    log.current_time = GetUnixTime();
    
    uint16_t last_address = LoadLastAddress();
    
    uint16_t next_address = last_address + sizeof(ConnectionLog);
    if (last_address < LOG_START_ADDRESS || next_address + sizeof(ConnectionLog) - 1 > LOG_END_ADDRESS) {
        next_address = LOG_START_ADDRESS;
    }
    
    uint8_t* ptr = (uint8_t*)&log;
	
    for (size_t i = 0; i < sizeof(ConnectionLog); i++)
	{
        WriteBackupSRAM(next_address + i, ptr[i]);
    }
    
    SaveLastAddress(next_address);
}

void EMMC_GenerateLogFileName(void)
{
    uint32_t unixTime = GetUnixTime();

    // e.g. mqtt_log_1713850200.dat
    snGLogN(g_emmcLogFileName, EMMC_FILE_NAME_MAX, "%lu_Data_log.dat", (unsigned long)unixTime);
}


uint8_t EMMC_OpenLogFile(void)
{
	FRESULT res;

	EMMC_GenerateLogFileName();  // Filename generation
	f_chdir(DIR_ROOT);

	res = f_open(&g_logEmmcFile, g_emmcLogFileName, FA_WRITE | FA_OPEN_ALWAYS);
	if (res != FR_OK)
	{
		GLogN("f_open failed in %s, res=%d\r\n", DIR_ROOT, res);
		return 0;
	}

	res = f_lseek(&g_logEmmcFile, f_size(&g_logEmmcFile));
	if (res != FR_OK)
	{
		GLogN("f_lseek failed in %s, res=%d\r\n", DIR_ROOT, res);
		f_close(&g_logEmmcFile);
		return 0;
	}

	return 1;
}

uint8_t EMMC_WriteLog(const void* pData, uint32_t size)
{
	FRESULT res;
	UINT bw;

	if (pData == NULL || size == 0)
	{
		GLogN("Invalid write request. size=%lu\r\n", size);
		return 0;
	}

	res = f_write(&g_logEmmcFile, pData, size, &bw);
	if (res != FR_OK || bw != size)
	{
		GLogN("f_write failed in %s, res=%d, bw=%u\r\n", DIR_ROOT, res, bw);
		return 0;
	}

	return 1;
}

uint8_t EMMC_CloseLogFile(void)
{
	FRESULT res;

	res = f_close(&g_logEmmcFile);
	if (res != FR_OK)
	{
		GLogN("f_close failed in %s, res=%d\r\n", DIR_ROOT, res);
		return 0;
	}

	return 1;
}
/*----------------------------------------------------------------------
 *   RSSI Signal Strength LED Indicator
 *   Blue   : Strong signal (>= -60 dBm)
 *   Yellow : Medium signal (-61 ~ -75 dBm)
 *   Red    : Weak signal   (< -75 dBm)
 *--------------------------------------------------------------------*/
int8_t RSSI_SetLedIndicator(void)
{
	int8_t rssi = 0;

	if(rsi_wlan_get(RSI_RSSI, &rssi, sizeof(rssi)) != 0)
	{
		GLogE("RSSI read failed\r\n");
		LED_ALL_OFF;
		LED_BLUE_ON;
		LED_SetState(eLED_SERVER_CONNECTED, 0, 0);
		return 0;
	}

	LED_ALL_OFF;

	if(rssi <= RSSI_THRESHOLD_STRONG)
	{
		GLogI("RSSI: %d [STRONG] -> BLUE\r\n", rssi);
		LED_SetState(eLED_RSSI_STRONG, 800, 200);
	}
	else if(rssi <= RSSI_THRESHOLD_MEDIUM)
	{
		GLogI("RSSI: %d [MEDIUM] -> YELLOW\r\n", rssi);
		LED_SetState(eLED_RSSI_MEDIUM, 800, 200);
	}
	else
	{
		GLogI("RSSI: %d [WEAK] -> RED\r\n", rssi);
		LED_SetState(eLED_RSSI_WEAK, 800, 200);
	}

	return rssi;
}
