/*----------------------------------------------------------------------
 *   EDR Adaptor Function List
 *--------------------------------------------------------------------*/
#ifndef __GIT_FUNCTION_LIST_H__
#define __GIT_FUNCTION_LIST_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
     Defines
----------------------------------------------------------------------*/
#define LOCK_GET_STATE() 	(g_eLockStatus)
#define LOCK_SET_STATE(X)	(g_eLockStatus = X)

#define	TEST_MODE_BATTERY_ADC								0x02
//#define	TEST_MODE_EMMC_INIT								0x03
#define	TEST_MODE_REPROGRAM_ADC								0x03
#define	TEST_MODE_RTC										0x04
#define	TEST_MODE_LED_GREEN									0x06
#define	TEST_MODE_LED_BLUE									0x07
#define	TEST_MODE_LED_RED									0x08
#define	TEST_MODE_LED_WHITE									0x09
#define	TEST_MODE_HCAN1										0x10
#define	TEST_MODE_HCAN2										0x11
#define	TEST_MODE_LCAN										0x12
#define	TEST_MODE_SLEEP_IG									0x13
#define	TEST_MODE_SLEEP_SENSOR								0x14
#define	TEST_MODE_SLEEP_HCAN1								0x16
#define	TEST_MODE_SLEEP_HCAN2								0x17
#define	TEST_MODE_SLEEP_LCAN								0x18
#define	TEST_MODE_SLEEP_TRG									0x19
#define	TEST_MODE_SLEEP_12V									0x20
#define	TEST_MODE_SLEEP_24V									0x21
#define	TEST_MODE_TRIGGER									0x23
#define	TEST_MODE_FW_VER									0x27
#define	TEST_MODE_SERIAL_WRITE								0x28
#define	TEST_MODE_WLAN_VER									0x29

#define TEST_MODE_BT_BTN									0x31
#define TEST_MODE_HSM										0x32
#define TEST_MODE_WIFI_RSSI									0x33
#define TEST_MODE_WIFI_BTN									0x34
#define TEST_MODE_BUZZER									0x35
#define TEST_MODE_EMMC_TEST									0x36
#define TEST_MODE_EMMC_FORMAT								0x37
#define TEST_MODE_EMMC_LOCK_TEST							0x38

#define	TEST_MODE_BT_CON_CHK								0x41
#define	TEST_MODE_WIFI_CONNECT_24							0x42
#define	TEST_MODE_WIFI_SEND_24								0x43
#define	TEST_MODE_WIFI_CONNECT_50							0x44
#define	TEST_MODE_WIFI_SEND_50								0x45
#define	TEST_MODE_WIFI_DISCONNECT							0x46
#define	TEST_MODE_WIFI_24_TEST								0x47
#define	TEST_MODE_WIFI_50_TEST								0x48
#define	TEST_MODE_BT_WIFI_MAC								0x49

#define	TEST_MODE_ETH_TX									0x51
#define	TEST_MODE_ETH_T1									0x52
#define	TEST_MODE_ETH_ENABLE								0x53
#define	TEST_MODE_ETH_DISABLE								0x54

#define	TEST_MODE_IG_OUTPUT									0x61
#define	TEST_MODE_KLINE_510									0x62
#define	TEST_MODE_KLINE_2K									0x63
#define	TEST_MODE_KLINE_47K									0x64
#define	TEST_MODE_OBD_CONNECT								0x65
#define	TEST_MODE_REPRO_VOLTAGE								0x66
#define	TEST_MODE_BT_SSID									0x67
#define	TEST_MODE_LATCH_RESET								0x68

#define TEST_MODE_HSM_VERSION                               0x69
#define TEST_MODE_HSM_CHECK                                 0x70
#define TEST_MODE_HSM_UPDATE                                0x71

#define TEST_MODE_KEK_IMPORT                                0x72
#define TEST_MODE_KM_IMPORT                                 0x73
#define TEST_MODE_ECUCODEKEY_IMPORT                         0x74
#define TEST_MODE_NEW_HSM_CHECK                    			0x75
#define TEST_MODE_NEW_HSM_VERSION                    		0x76

#define TEST_MODE_SSL_CERT_WRITE_START                  	0xA1
#define TEST_MODE_SSL_CERT_WRITE_UPDATE                 	0xA2
#define TEST_MODE_SSL_CERT_WRITE_END                    	0xA3
#define TEST_MODE_SSL_CERT_IMPORT                       	0xA4
#define TEST_MODE_PRODUCTION_MODE                       	0xA5

#define TEST_NRC_OLD_HSM                    				1
#define TEST_NRC_IMPORT_FAIL                    			2
#define TEST_NRC_VERIFY_IMPORT_FAIL                			3
#define TEST_NRC_NOT_MATCH_EXPECT                    		4
#define TEST_NRC_HMAC_SHA256_FAIL                  			5
#define TEST_NRC_AES_ENCRYPT_FAIL                  			6
#define TEST_NRC_AES_DECRYPT_FAIL                  			7
#define TEST_NRC_ASK_VERIFY_FAIL                    		8

#define	HSM_READ_CSN			                            0x0101
#define	HSM_INTERNAL_AUTH		                            0x0102
#define	HSM_EXTERNAL_AUTH		                            0x0103
#define	HSM_PUBLICKEY_HSM		                            0x0111
#define	HSM_STORE_AUTHKEY		                            0x0112
#define	HSM_STORE_AESKEY	                            	0x0113
#define	HSM_STORE_HMACKEY		                            0x0114
#define	HSM_STORE_ECU_CODE_KEY                            	0x0115
#define	HSM_READ_SDATA			                            0x0121
#define	HSM_STORE_CERTI			                            0x0122
#define	HSM_GET_SEEDKEY			                            0x0123

#ifdef ADL_CVCI_HSM_METHOD

#define	HSM_STORE_PRIVATEKEY	                            0x0124

#else

#define	HSM_READ_PUBKEY     	                            0x0124
#define	HSM_READ_RANDKEY     	                            0x0126
#define	HSM_STORE_PRIVATEKEY    	                        0x0127
#define	HSM_STORE_CRL           	                        0x0128
#define	HSM_CHECK_CODEKEY       	                        0x0129
#define	HSM_SELFTEST_PRIVATEKEY       	                    0x0130

#endif

#define HSM_GET_INVALID_DATE	                            0x0125

#define HSM_STORE_CERTI_CV_KD                               0x0131
#define HSM_STORE_PRIVATEKEY_CV_KD                          0x0132
#define HSM_STORE_CRL_CV_KD                                 0x0133

/* RSA Key Format (Old format: 528 bytes) */
#define RSA_KEY_DATA_OFFSET                                 3       /* SID(2) + KeyNum(1) */
#define RSA_MODULUS_SIZE                                    256
#define RSA_PUB_EXP_PADDED_SIZE                             16
#define RSA_PUB_EXP_SIZE                                    3       /* 0x010001 = 65537 */
#define RSA_PRIV_EXP_SIZE                                   256
#define ENC_UPPER_PRIV_EXP_SIZE                             256
#define ENC_LOWER_PRIV_EXP_SIZE                             256


/* HSM Error Codes */
#define HSM_NOT_SUPPORTED                                   0xFF
#define HSM_READ_FAIL                                       0xFE
#define HSM_WRITE_FAIL                                      0xFD
#define HSM_SIGN_FAIL                                       0xFC
#define HSM_HASH_FAIL                                       0xFB

#define READ_NUMBER_OF_FILES                                0x01
#define READ_PATH_INFO                                      0x02
#define CHECKSUM_CALCULATION                                0x03

/*----------------------------------------------------------------------
 *   Typedef
 *--------------------------------------------------------------------*/
typedef void	(*pfnCommandLoadCB)( stCommPkt *pkt, uint32_t eInCommType );

typedef enum _eLockState
{
	eLOCK_STATE_INIT,
	eLOCK_STATE_LOCK,
	eLOCK_STATE_UNLOCK,
	eLOCK_STATE_MAX
}eLockState;

typedef struct
{
	uint32_t			uiFunctionID;
	pfnCommandLoadCB	fnPayloadCB;
} stFunctionList;

typedef struct 
{
    uint8_t SSID_Length;
    uint8_t SSID[32];
    uint8_t PSK_Length;
    uint8_t PSK_Key[63];
    uint8_t Security_Mode;
    uint8_t ServerDomain[60];
} WifiSetInfo;

typedef struct 
{
    uint8_t SSID_Length;
    uint8_t SSID[32];
    uint8_t PSK_Length;
    uint8_t PSK_Key[63];
    uint8_t Security_Mode;
} WifiScanInfo;

typedef struct 
{
    uint8_t Serial_Number[8];
    uint8_t SSID[32];
    uint8_t Security_Mode;
    uint8_t bssid[12];
    uint8_t Autovin[17];
} WifiConnectInfo;
/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern stFunctionList	gsFunctions[];
extern u32				guiFuncCnt;
#if defined (USB_SPEED_TEST)
extern uint32_t	g_uiUSBRxInterval;
extern uint32_t	g_uiUSBTxInterval;
extern uint32_t g_uiUSBRx_Cnt;
#endif
extern uint8_t	g_ucRecvPassThruWriteMsg;
extern bool		g_bUsingKM;
/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern void FL_NotSupport( uint32_t iden );

extern uint32_t TestReprogramLine( void );
extern uint8_t TestWifiConnect (uint8_t* ssid);
void ClearDiagMessage();


#endif // __GIT_FUNCTION_LIST_H__
