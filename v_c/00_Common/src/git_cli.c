/*************************************************************
 * NOTE : git_cli.c
 *      Command Line Interface for Uart
 * Author : Lee junho
 * Since : 2020.10.21
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "common.h"
#include "firmware.h"
#include "git_adc.h"
#include "git_mmc.h"
#include "git_ioctl.h"
#include "git_kl.h"
#include "git_i2c.h"
#include "git_function_list.h"
#include "git_pm.h"
#include "git_can.h"
#include "git_rs9116.h"
#ifdef VCI3_DIAG
#include "git_eth.h"
#endif
#include "git_rtc.h"
#include "git_vci.h"
#include "git_cli.h"
#include "git_fsutil.h"
#ifdef VCI3_DIAG
#include "git_mcp2518fd.h"
#endif
#include "Buzzer.h"
#include "Sw_timer.h"
#include "rsi_firmware_upgradation.h"
#include "git_hsm.h"
#include "git_base64.h" 
#include "git_websocket.h"
#include "git_bkSRAM.h"
#include "rsi_mqtt_client.h"
#include "git_global.h"
#include "git_HSM_SPICommand.h"
#include "git_HSM_Operations.h"
#include "gpio.h"
#include "cmox_init.h"
#include "cmox_low_level.h"
#include "cmox_crypto.h"
/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define CLI_PARSING_START_SIGNAL		(uint32_t)0x01
#define CLI_DELIM_CHARS					", "
#define TEST_FILE_NAME					"eMMC_Test.dat"
#define TEST_FILE_DATA					"VCI3 eMMC Test!!!"
#define TEST_FILE_DATA_SIZE				sizeof( TEST_FILE_DATA ) + 1
#define NUM_OF_HSM						6
#define RS9116_RETRY_COUNT				5
#define MAX_CLI_ARGS					16
#define MAX_FAILCOUNT					50
/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
int32_t InitCli( void );
int32_t StartCli( void );
void DeinitCli( void );

void sendCliData( void );
uint8_t	parsingCliString( void );
uint8_t runCliCommand( uint8_t count );
void CheckPassword( void );

void uartCliThread( void const *argument );
extern void ETHERNET_LINE_CONTROL();
extern FRESULT scan_files(char* path);
extern u32	Get_TmrDelta( u32 ulNew, u32 ulOld );
void cli_make_test_files(void);
void cli_test_fileparsing_root(uint16_t seq);
void cli_delete_test_files(void);

/*----------------------------------------------------------------------
 *   HMAC-SHA256 Implementation using CMOX Library
 *   Uses ST's optimized cryptographic library
 *--------------------------------------------------------------------*/
static int manual_hmac_sha256(const uint8_t *key, size_t key_len,
                              const uint8_t *data, size_t data_len,
                              uint8_t *mac_out)
{
    size_t out_len = 0;
    cmox_mac_retval_t ret;

    if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
    {
        return -1;
    }

    ret = cmox_mac_compute(
        CMOX_HMAC_SHA256_ALGO,
        data, data_len,
        key, key_len,
        NULL, 0,
        mac_out, 32,
        &out_len
    );

    cmox_finalize(NULL);

    return (ret == CMOX_MAC_SUCCESS && out_len == 32) ? 0 : -1;
}

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
bool g_bBTLogOnTxFlag		= false;
bool g_bBTLogOnRxFlag   	= false;
bool g_bCanLogOnTxFlag  	= false;
bool g_bCanLogOnRxFlag  	= false;
bool g_bKLogOnTxFlag    	= false;
bool g_bKLogOnRxFlag    	= false;
bool g_bwebLogOnTxFlag  	= false;
bool g_bwebLogOnRxFlag  	= false;
bool g_bMqttLogOnTxFlag		= false;
bool g_bMqttLogOnRxFlag		= false;
bool g_bEncryptLogOnFlag	= false;
bool g_bHSMCheckFlag    	= true;
uint32_t g_u32firmware_size = 0;

bool m_bIsCorrectPassword = true;
static U8 m_ucFailCount = 0;
static U8 m_unFailFlag = 0;
// Thread
osThreadId uartCliTh;
osThreadDef( uartcli,	uartCliThread,	osPriorityNormal, 0, 3*1024 );

static char		gCliString[128];
static char		gCliArgs[4][128];

extern uint8_t	g_SaveLastTxPacket[RSI_BT_MAX_PAYLOAD_SIZE];
extern uint8_t	g_SaveLastRxPacket[RSI_BT_MAX_PAYLOAD_SIZE];
extern uint32_t	app_buf_len;
#ifdef USE_RELAY_MOSA
#include "Git_BatteryRelayControl.h"

extern uint8_t g_ucCanformat;
extern osPoolId hBatRelayConPool;
extern osMessageQId hBatRelayConMsg;
#endif

static uint8_t	gIndex	= 0;

uint8_t	gucCLIRxDummy;
uint8_t g_ucWifiTest = 0;
uint8_t g_ucBTTest = 0;
uint32_t send_idx = 0;

uint32_t tt_start;
uint32_t tt_end;
uint8_t a1[100] = "500KB";
uint8_t aaa_len;
extern bool g_bEncryptFlag;
extern uint8_t g_ucAES256_Key[32];

uint32_t mem_i = 0;

static rsi_bt_resp_get_local_name_t		local_name = {0};

extern SFwInfo gsFwInfo;
/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/
int32_t InitCli( void )
{
	HAL_UART_Receive_IT( &huart2, &gucCLIRxDummy, 1 );

	return INIT_OK;
}

int32_t StartCli( void )
{
	uartCliTh = osThreadCreate( osThread( uartcli ), NULL );
	if( uartCliTh == NULL )
	{
		return -1;
	}

	return 0;
}

void DeinitCli( void )
{
}

void sendCliData( void )
{
	if( uartCliTh != NULL )
	{
		if( gucCLIRxDummy == '\r' )
		{
			osSignalSet( uartCliTh, CLI_PARSING_START_SIGNAL );
		}
		else if( gucCLIRxDummy == '\b' )
		{
			if( gIndex != 0 )
			{
				if(m_bIsCorrectPassword == true) GLogN( "\b \b" );
				gIndex--;
				gCliString[ gIndex ] = '\0';
			}
		}
		else
		{
			if(m_bIsCorrectPassword == true) GLogN( "%c", gucCLIRxDummy );
			gCliString[ gIndex++ ] = gucCLIRxDummy;
		}
	}

	HAL_UART_Receive_IT( &huart2, &gucCLIRxDummy, 1 );
}

uint8_t	parsingCliString( void )
{
	char	*ret_ptr;
	char	*next_ptr;
	uint8_t	count	= 0;
	
	if(m_bIsCorrectPassword == true) GLogI( "\r\nCommand : %s\r\n", gCliString );
	ret_ptr = strtok_r( gCliString, CLI_DELIM_CHARS, &next_ptr );
	while( ret_ptr )
	{
		memset( gCliArgs[count], 0, sizeof( gCliArgs[count] ) );
		strcpy( gCliArgs[count], ret_ptr );
//		GLogN( "args[%d] = [%s]\r\n", count, gCliArgs[count] );
		ret_ptr = strtok_r( NULL, CLI_DELIM_CHARS, &next_ptr );

		count++;
	}

	return count;
}

extern uint8_t		g_ucClientConnected;
extern uint8_t 		g_ucPingRegistered;
extern volatile uint8_t ping_rsp_received;
extern void rsi_ping_response_handler(uint16_t status, const uint8_t *buffer, const uint16_t length);

extern int32_t					g_iClient_socket;
extern struct rsi_sockaddr_in	g_server_addr;
extern FIL			g_TempFilepnt;

#define BUF_SIZE	(1460 * 3)
#define KEY_LENGTH 	4

//#include <zlib.h>

static void UpdateAppSwListVer(const char *target, uint8_t ucMajor, uint8_t ucMinor)
{
	FIL         fp;
	static char inBuf[2048];
	static char outBuf[2048];
	UINT        br;
	int         outLen = 0;
	UINT        bw;

	memset(inBuf,  0, sizeof(inBuf));
	memset(outBuf, 0, sizeof(outBuf));

	int targetAppNo = -1;
	int isTotal = (!strcmp(target, "total"));
	if (!isTotal) {
		if (!strcmp(target, "boot"))
			targetAppNo = (int)eApp_bootloader;
		else
			targetAppNo = atoi(target);
	}

	f_chdir(DIR_ROOT);
	if (f_chdir(DIR_APP) != FR_OK) {
		GLogE("AppSwList: chdir failed\r\n");
		f_chdir(DIR_ROOT);
		return;
	}

	if (f_open(&fp, APPLICATION_INFO_FILE_NAME, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
		GLogE("AppSwList: open failed\r\n");
		f_chdir(DIR_ROOT);
		return;
	}

	/* f_read: \r\n 그대로 읽음 (f_gets는 \r 제거함) */
	f_read(&fp, inBuf, sizeof(inBuf) - 1, &br);
	f_close(&fp);
	inBuf[br] = '\0';

	char *p         = inBuf;
	int   remaining = (int)br;

	while (remaining > 0) {
		char       *nl      = memchr(p, '\n', remaining);
		int         lineLen = nl ? (int)(nl - p + 1) : remaining;
		const char *eol;
		int         contentLen;

		if (nl && nl > p && *(nl - 1) == '\r') {
			eol        = "\r\n";
			contentLen = lineLen - 2;
		} else if (nl) {
			eol        = "\n";
			contentLen = lineLen - 1;
		} else {
			eol        = "";       /* 마지막 줄, 줄바꿈 없음 */
			contentLen = lineLen;
		}

		char tmp[80];
		int  cpLen = (contentLen < (int)sizeof(tmp) - 1) ? contentLen : (int)sizeof(tmp) - 1;
		memcpy(tmp, p, cpLen);
		tmp[cpLen] = '\0';

		char *ptr, *tok0, *tok1, *tok2;
		tok0 = strtok_r(tmp, TOKEN_SEPARATORS, &ptr);
		tok1 = strtok_r(NULL, TOKEN_SEPARATORS, &ptr);
		tok2 = strtok_r(NULL, TOKEN_SEPARATORS, &ptr);

		if (tok0 == NULL || tok1 == NULL || tok2 == NULL) {
			/* 빈 줄 또는 파싱 불가 → 원본 그대로 */
			if (outLen + lineLen < (int)sizeof(outBuf)) {
				memcpy(&outBuf[outLen], p, lineLen);
				outLen += lineLen;
			}
		} else {
			int lineIsTotal = (!strncmp(tok0, "FF", 2));
			int lineAppNo   = lineIsTotal ? -1 : atoi(tok0);
			int match       = (isTotal && lineIsTotal) || (!isTotal && lineAppNo == targetAppNo);

			if (match) {
				outLen += snprintf(&outBuf[outLen], sizeof(outBuf) - outLen,
				                   "%s,%s,%d.%02d%s", tok0, tok1, ucMajor, ucMinor, eol);
			} else {
				/* 구분자를 쉼표로 정규화하여 재조립 */
				outLen += snprintf(&outBuf[outLen], sizeof(outBuf) - outLen,
				                   "%s,%s,%s%s", tok0, tok1, tok2, eol);
			}
		}

		p         += lineLen;
		remaining -= lineLen;
	}

	f_unlink(APPLICATION_INFO_FILE_NAME);
	if (f_open(&fp, APPLICATION_INFO_FILE_NAME, FA_CREATE_NEW | FA_WRITE) == FR_OK) {
		f_write(&fp, outBuf, outLen, &bw);
		f_close(&fp);
		GLogN("AppSwList.ini updated (%d bytes)\r\n", outLen);
	} else {
		GLogE("AppSwList: write failed\r\n");
	}

	f_chdir(DIR_ROOT);
}

uint8_t runCliCommand( uint8_t count )
{
	FRESULT	res;
	FIL		testFile;
	uint8_t	wtext[TEST_FILE_DATA_SIZE] = TEST_FILE_DATA;
	uint8_t	rtext[TEST_FILE_DATA_SIZE];
	U8 i;
	uint32_t byteswrite	= 0;
	uint32_t bytesread	= 0;
	int32_t status 		= 0;
	char Path[100]= {'\0', };

	char *frame = NULL;
	int32_t recv_len = 0;
	int32_t addr_size;
	int32_t send_len = 0;
	addr_size = sizeof(g_server_addr);
		
	size_t payload_len = 0;
	size_t frame_len = 0;
	

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)

	//rdbi.ini
    U8 Data[165] = {    
    0x01, 0x00, 0x39, 0x32, 0x30, 0x32, 0x32, 0x30, 0x34, 0x31, 0x38, 0x5f, 0x5f, 0x31, 0x31, 0x33,
    0x31, 0x32, 0x36, 0x5f, 0x5f, 0x43, 0x45, 0x45, 0x56, 0x5f, 0x5f, 0x56, 0x43, 0x55, 0x5f, 0x5f,
    0x33, 0x39, 0x37, 0x35, 0x31, 0x31, 0x58, 0x45, 0x44, 0x31, 0x5f, 0x5f, 0x30, 0x31, 0x5f, 0x5f,
    0x44, 0x51, 0x30, 0x34, 0x5f, 0x5f, 0x46, 0x44, 0x2e, 0x78, 0x6d, 0x6c, 0x39, 0x32, 0x30, 0x32,
    0x32, 0x30, 0x34, 0x31, 0x38, 0x5f, 0x5f, 0x31, 0x31, 0x33, 0x31, 0x32, 0x36, 0x5f, 0x5f, 0x43,
    0x45, 0x45, 0x56, 0x5f, 0x5f, 0x56, 0x43, 0x55, 0x5f, 0x5f, 0x33, 0x39, 0x37, 0x35, 0x31, 0x31,
    0x58, 0x45, 0x44, 0x31, 0x5f, 0x5f, 0x30, 0x31, 0x5f, 0x5f, 0x44, 0x51, 0x30, 0x34, 0x5f, 0x5f,
    0x46, 0x44, 0x2e, 0x62, 0x69, 0x6e, 0x02, 0xf1, 0x87, 0x00, 0x0a, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x01, 0x33, 0x39, 0x37, 0x35, 0x31, 0x31, 0x58, 0x45, 0x44, 0x31,
    0x02, 0xf1, 0xb1, 0x00, 0x04, 0x04, 0x05, 0x06, 0x07, 0x44, 0x51, 0x30, 0x34, 0x00, 0x00, 0x07,
    0xe2, 0x00, 0x00, 0x07, 0xea};

#endif

	// Check Args[0] : Command
	if( ( !strcmp( gCliArgs[0], "help" ) )  || ( !strcmp( gCliArgs[0], "h" ) ) )
	{
		GLogI( "\r\n\n=================================================================\r\n" );
		GLogI( "   Command List\r\n" );
		GLogI( "=================================================================\r\n" );
		GLogN( " battery [count]      - Battery Voltage Read, default count = 1\r\n" );
		GLogN( " batteryrepro [count] - Repro Voltage Read, default count = 1\r\n" );
		GLogN( " build                - Source Build Date\r\n" );
		GLogN( " fwinfo               - print Current F/W Information\r\n" );
		GLogN( " fwinit               - initialize F/W Information\r\n" );
		GLogN( " reset                - reset Module \r\n" );
		GLogN( " serial               - print Current Serial Number\r\n" );
		GLogN( " version [boot] [app] - print or save Current F/W version\r\n" );
		GLogN( " setver                          - read F/W version from EMMC\r\n" );
		GLogN( " setver <app|boot|total> <M> <m> - set F/W version on EMMC\r\n" );
		GLogN( "   app_no: 0~%d, boot=bootloader(%d), total=total version\r\n", eApp_MAX - 1, eApp_bootloader );
		GLogN( "   ex) setver boot 1 23  -> bootloader 01.23\r\n" );
		GLogN( "   ex) setver total 2 10 -> total ver 02.10\r\n" );
		GLogN( "   ex) setver 0 1 23     -> app[0] 01.23\r\n" );
#ifdef FW_TEST_VERSION_OVERRIDE
		GLogN( " hsmtestver                      - read HSM test version from EMMC\r\n" );
		GLogN( " hsmtestver <ver>                - set HSM test version (e.g. 1001)\r\n" );
		GLogN( " wlantestver                     - read WLAN test version from EMMC\r\n" );
		GLogN( " wlantestver 1                   - set to default (1610.2.10.0.0.5)\r\n" );
		GLogN( " wlantestver <ver_str>           - set WLAN test version (e.g. 1610.2.10.0.0.5)\r\n" );
#endif
		GLogI( "=================================================================\r\n" );
		GLogN( " format               - Format Emmc & make Folder\r\n" );
		GLogN( " kline [1] [2]        - Set KLine1, 2\r\n" );
		GLogN( " repg  [line]         - Set Reprogram\r\n" );
		GLogN( " can [number]         - can tx number: 1(FDCan1), 2(FDCan2), 3(LowCan)\r\n" );
		GLogN( " sensor               - sensor sleep\r\n" );
		GLogN( " rtc                  - rtc read\r\n" );
		GLogN( " bt                   - Enable bt loop back mode\r\n" );
		GLogN( " wifi24 [Server IP]   - Connect wifi24 and eable loop back mode\r\n" );
		GLogN( " wifi50 [Server IP]   - Connect wifi50 and eable loop back mode\r\n" );
		GLogN( " igon                 - IG_ON_EN : LOW  &  CH3 high detect\r\n" );
		GLogN( " emmc                 - eMMC write read erase test\r\n" );
		GLogN( " makelog              - Make log file\r\n" );
		GLogN( " readfile [File Name] - Show file data\r\n" );
		//GLogN( " HSM                  - Show HSM Version,CRT Date,HolderRef\r\n" );
		//GLogN( " CRL                  - Show CRL Date\r\n" );
		GLogN( " mkdir [path]           - make directory\r\n" );
		GLogN( " mkfile [path/filename] - make file\r\n" );
		GLogN( " rm [path]	    	    - remove directory or file\r\n" );
		GLogN( " dir [path] 	        - scan directory\r\n" );
		GLogN( " cd [path] 	            - change directory, if no argument is current directory\r\n" );
		GLogN( " mklst		 	        - creat RDBI.BIN\r\n" );
		GLogN( " cat [filename]         - read file in current folder \r\n" );
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
		GLogN( " rdbi                   - creat RDBI.BIN\r\n" );
		GLogN( " stdr	 	            - creat StandAloneReproStart.ini\r\n" );
#endif
		//GLogN( " ethon                - DLC 8pin 12V high \r\n" );
		//GLogN( " ethoff               - DLC 8pin 12V low \r\n" );
		GLogI( "=================================================================\r\n" );
	}
	else if( !strcmp( gCliArgs[0], "test" ) )
	{
		if( !strcmp( gCliArgs[1], "ap" ) )
        {
            if(autoConnectAP() == true)
			{
				GLogN( "ConnectAP OK! \r\n" );
			}
			else
			{
				GLogI( "ConnectAP FAIL! \r\n" );
			}
            
            PrintWifiConnectInfo();
        }
		else if( !strcmp( gCliArgs[1], "ecu_test" ) )
        {
			FRESULT res;
			DIR dir;
			
			res = f_opendir(&dir, "/myfolder");
			if (res == FR_NO_PATH) {
				res = f_mkdir("/myfolder");
				if (res != FR_OK) {
					GLogE("dir open fail!!\r\n");
				}
			}
			f_closedir(&dir);
			
			char filename[64];
			FIL fil;

			for (int i = 0; i < 10; i++)
			{
				sprintf(filename, "/myfolder/file_%03d.bin", i);

				res = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
				if (res != FR_OK)
				{
					GLogE("file open fail!!\r\n");
					break;
				}
				else
				{
					GLogN("file open ok!!\r\n");
				}

				for (int j = 0; j < (10 * 1024); j++) {
					uint8_t buffer[1024] = {0};  // 1KB ����
					UINT bw;
					
					memset(buffer, j % 256, sizeof(buffer));
					res = f_write(&fil, buffer, sizeof(buffer), &bw);
					if (res != FR_OK || bw != sizeof(buffer)) {
						GLogE("write fail!! b : %d, w : %d", sizeof(buffer), bw);
						break;
					}
					
				if(0 == j % 100)
					GLogN(".");
				}
				
				if(res != FR_OK)
				{
					GLogE("\r\nYour life is a failure!!\r\n");
				}
				else
				{
					GLogN("\r\nfile write ok!(%d)\r\n", i);
				}
				f_close(&fil);
			}
        }
		else if( !strcmp( gCliArgs[1], "http" ) )
        {
			app_buf_len = 0;
			tt_start = rsi_hal_gettickcount();
			
			status = http_get_example();
			if(status == 0)
			{
				GLogN("http OK!!");
			}
        }
		else if( !strcmp( gCliArgs[1], "a9" ) )
        {
			uint8_t result;
			
			result = f_chdir(DIR_ROOT);
			if ( result == FR_OK )
			{
				GLogN( "Change Directory DIR_ROOT Ok\r\n");
			}
			else
			{
				GLogN("result:%d\r\n",result);
				return result;
			}

			result = f_chdir(DIR_APP);
			if ( result == FR_OK )
			{
				GLogN( "Change Directory Application Ok\r\n");
			}
			else
			{
				GLogN("result2:%d\r\n",result);
				return result;
			}
			
			result = f_open(&g_TempFilepnt, DOWNLOAD_FW_TEMP_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE | FA_READ );
			if ( result == FR_OK )
			{
				GLogN( "%s File Open Success\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
				result = f_truncate(&g_TempFilepnt);
				if ( result == FR_OK )
					GLogN( "%s File Truncate Success\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
			}
        }
		else if( !strcmp( gCliArgs[1], "a0" ) )
        {
			memcpy(a1, "1234567890", 10);
			aaa_len = 10;
			tt_start = rsi_hal_gettickcount();
			MQTTPacketSend(1, a1, aaa_len);
        }
		else if( !strcmp( gCliArgs[1], "a1" ) )
        {
			memcpy(a1, "0987654321", 10);
			aaa_len = 10;
			tt_start = rsi_hal_gettickcount();
			MQTTPacketSend(1, a1, aaa_len);
        }
		else if( !strcmp( gCliArgs[1], "a2" ) )
        {
			memcpy(a1, "1MB", 3);
			aaa_len = 3;
			tt_start = rsi_hal_gettickcount();
			MQTTPacketSend(1, a1, aaa_len);
        }
		else if( !strcmp( gCliArgs[1], "a3" ) )
        {
			memcpy(a1, "1234567890", 3);
			aaa_len = 3;
			tt_start = rsi_hal_gettickcount();
			MQTTPacketSend(1, a1, aaa_len);
        }
		else if( !strcmp( gCliArgs[1], "s_set" ) )
        {
            // ���� ���� �õ�
            t_len = 0;
        }
        else if( !strcmp( gCliArgs[1], "scan" ) )
        {         
            rsi_rsp_scan_t scan_result;

            status = rsi_wlan_scan(NULL, 0, &scan_result, sizeof(scan_result));
            if (status != RSI_SUCCESS) {
                printf("WiFi scan failed with status: %04X\n", status);
            }
			else
			{
				for (uint32_t i = 0; i < scan_result.scan_count[0]; i++) {
                printf("Network: %d\r\n", i + 1);
                printf("SSID: %s\r\n", scan_result.scan_info[i].ssid);
                printf("BSSID: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                       scan_result.scan_info[i].bssid[0], scan_result.scan_info[i].bssid[1], scan_result.scan_info[i].bssid[2],
                       scan_result.scan_info[i].bssid[3], scan_result.scan_info[i].bssid[4], scan_result.scan_info[i].bssid[5]);
                printf("Channel: %d\r\n", scan_result.scan_info[i].rf_channel);
                printf("Security Mode: %d\r\n", scan_result.scan_info[i].security_mode);
                printf("RSSI: %d\r\n", scan_result.scan_info[i].rssi_val);
                printf("Network Type: %d\r\n", scan_result.scan_info[i].network_type);
                printf("---------------------------\r\n");
            	}
			}
            
            printf("%d\r\n", sizeof(scan_result.scan_info));
			printf("%d\r\n", sizeof(rsi_scan_info_t) * 11);
            printf("%d\r\n", sizeof(rsi_mqtt_client_info_t));
        }
        else if( !strcmp( gCliArgs[1], "rssi" ) )
        {         
            int8_t rssi_value;
            int32_t iii;
            
            while(iii < 100){
                tt_start = rsi_hal_gettickcount();
                status = rsi_wlan_get(RSI_RSSI, &rssi_value, sizeof(rssi_value));
                if (status != RSI_SUCCESS) {
                    printf("RSSI fail : %2X\r\n", status);
                }
                tt_end = rsi_hal_gettickcount();
                GLogI("tt_start : %d tt_end : %d   %d \r\n", tt_start, tt_end, tt_end- tt_start);
                iii++;
                printf("RSSI value: %d\r\n", rssi_value);
                osDelay(500);
            }
            
            printf("RSSI Value: %d dBm\n", rssi_value);
        }
        else if( !strcmp( gCliArgs[1], "wifi_info" ) )
        {         
            rsi_rsp_wireless_info_t wifi_info;
            
            status = rsi_wlan_get(RSI_WLAN_INFO, (uint8_t *)&wifi_info, sizeof(wifi_info));
            
            printf("WLAN State: %d\r\n", wifi_info.wlan_state);
            printf("Channel Number: %d\r\n", wifi_info.channel_number);
            printf("SSID: %s\r\n", wifi_info.ssid);
            printf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    wifi_info.mac_address[0], wifi_info.mac_address[1], wifi_info.mac_address[2],
                    wifi_info.mac_address[3], wifi_info.mac_address[4], wifi_info.mac_address[5]);
            printf("Security Type: %d\r\n", wifi_info.sec_type);
            printf("PMK: ");
            for (int i = 0; i < 64; i++) {
                printf("%02x", wifi_info.pmk[i]);
            }
            printf("\r\n");
            printf("IPv4 Address: %d.%d.%d.%d\r\n",
                    wifi_info.ipv4_address[0], wifi_info.ipv4_address[1], wifi_info.ipv4_address[2], wifi_info.ipv4_address[3]);
        }
        else if( !strcmp( gCliArgs[1], "mqtt" ) )
        {
            if (connectToMQTT() == TRUE)
            {
                GLogN("Connected to MQTT successfully.\r\n");
            }
        }
        else if( !strcmp( gCliArgs[1], "mqtt_discon" ) )
        {
            if (disconnectToMQTT() == TRUE)
            {
                GLogN("Disconnected to MQTT successfully.\r\n");
            }
        }
		else if( !strcmp( gCliArgs[1], "ap_discon" ) )
        {
			DisconnectWLan( );							// restart from AP connect
        }
		else if( !strcmp( gCliArgs[1], "websocket" ) )
        {
            // ���� ���� �õ�
            if (connectToWebsocket() == 0) {
                GLogN("Connection Socket OK!!!! \r\n");
            } else {
                GLogI("connectToWebsocket FAIL \r\n");
            }
        }
		else if( !strcmp( gCliArgs[1], "websocket_discon" ) )
        {			
			uint8_t how = 0;
			uint8_t status = 0;
			status = rsi_shutdown(g_iClient_socket, how);
			
			GLogN("socket id : %d\r\n", g_iClient_socket);
			if(status == 0)
			{
				GLogN("Disconnected to Websokcet successfully .\r\n");
			}
			else
			{
				GLogN("Disconnected to Websocket Fail.\r\n");
			}
			
		}
        else if( !strcmp( gCliArgs[1], "mqtt_send" ) )
        {
		  	uint8_t *data = "helloworld";
			//FL_GitWifiConnectInfo(PACKET_MQTT);
			MQTTPacketSend(1, data, strlen(data));
        }
		else if( !strcmp( gCliArgs[1], "web_send" ) )
        {
		  	while(1)
			{
				uint8_t* data_temp = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
            	WebsockPacketSend(data_temp, strlen(data_temp));
				osDelay(1000);
			}
        }
		else if( !strcmp( gCliArgs[1], "ping" ) )
		{
			uint8_t  ping_ip[4];
			uint32_t pingCount   = 5;
			uint32_t successCnt  = 0;
			uint32_t totalLatency = 0;
			uint32_t minLatency  = 0xFFFFFFFF;
			uint32_t maxLatency  = 0;
			uint32_t pi;
			rsi_rsp_dns_query_t dns_response;

			/* Resolve ping target IP (same as connectToMQTT) */
			if(msServerConnectInfo.mqtt_domain[126] == 0)
			{
				uint8_t dnsOk = 0;
				for(pi = 0; pi < 3; pi++)
				{
					status = RequestDNS(msServerConnectInfo.mqtt_domain, &dns_response);
					if(status == RSI_SUCCESS)
					{
						ping_ip[0] = dns_response.ip_address[0].ipv4_address[0];
						ping_ip[1] = dns_response.ip_address[0].ipv4_address[1];
						ping_ip[2] = dns_response.ip_address[0].ipv4_address[2];
						ping_ip[3] = dns_response.ip_address[0].ipv4_address[3];
						dnsOk = 1;
						break;
					}
				}
				if(!dnsOk)
				{
					GLogE("DNS Failed\r\n");
					return -1;
				}
			}
			else if(msServerConnectInfo.mqtt_domain[126] == 1)
			{
				ping_ip[0] = msServerConnectInfo.mqtt_domain[0];
				ping_ip[1] = msServerConnectInfo.mqtt_domain[1];
				ping_ip[2] = msServerConnectInfo.mqtt_domain[2];
				ping_ip[3] = msServerConnectInfo.mqtt_domain[3];
			}

			if(msServerConnectInfo.mqtt_domain[126] == 0)
			{
				GLogN("Ping %s (%d.%d.%d.%d)\r\n",
						msServerConnectInfo.mqtt_domain,
						ping_ip[0], ping_ip[1], ping_ip[2], ping_ip[3]);
			}
			else
			{
				GLogN("Ping %d.%d.%d.%d\r\n",
						ping_ip[0], ping_ip[1], ping_ip[2], ping_ip[3]);
			}

			for(pi = 0; pi < pingCount; pi++)
			{
				ping_rsp_received = 0;
				g_pingStartTick = HAL_GetTick();

				status = rsi_wlan_ping_async(0, ping_ip, 64,
											rsi_ping_response_handler);
				if(status != RSI_SUCCESS)
				{
					osDelay(1000);
					continue;
				}

				while(!ping_rsp_received
					  && (HAL_GetTick() - g_pingStartTick < 3000))
				{
					osDelay(10);
				}

				if(ping_rsp_received && g_pingRspStatus == RSI_SUCCESS)
				{
					uint32_t latency = g_pingEndTick - g_pingStartTick;
					GLogN("[%d] %d ms\r\n", pi + 1, latency);
					successCnt++;
					totalLatency += latency;
					if(latency < minLatency) minLatency = latency;
					if(latency > maxLatency) maxLatency = latency;
				}
				else
				{
					GLogN("[%d] timeout\r\n", pi + 1);
				}

				if(pi < pingCount - 1) osDelay(1000);
			}

			GLogN("Result: %d/%d, Min=%dms Max=%dms Avg=%dms\r\n",
					successCnt, pingCount,
					(successCnt > 0) ? minLatency : 0,
					maxLatency,
					(successCnt > 0) ? (totalLatency / successCnt) : 0);
		}
	}


	/********************************************
	    Common Command
	********************************************/
#if 0
    else if( !strcmp( gCliArgs[0], "filetest" ) )
    {
        char arrTemp[10];
        arrTemp[0] = 3;
        f_chdir("/");
        sprintf(&arrTemp[1], "log", strlen("log"));
	    FL_EMMC_Directory( arrTemp, PACKET_UART, 4  );
    }
    else if( !strcmp( gCliArgs[0], "makefile" ) )
    {
        FIL  fp;
        char arrFilename[100];
        int nWriteLen;
        
        f_chdir("/");
        f_chdir("log");
        for (int i=0; i< 500; i++ )
        {
            sprintf(arrFilename, "%s_%d", "abcdefghijklmnopqrstuvwxyz", i);
            f_open(&fp, arrFilename, FA_CREATE_ALWAYS|FA_WRITE);
            for (int j=0; j<i; j++ )
            {
                sprintf(arrFilename, "%d", j);
                f_write(&fp, arrFilename, 1, &nWriteLen);
            }
            f_close(&fp);
        }
        
        f_chdir("/");
    }


	else if( !strcmp( gCliArgs[0], "ethon" ) )
	{
		ETHERNET_LINE_CONTROL();
	}
	else if( !strcmp( gCliArgs[0], "ethoff" ) )
	{
		VCI_Clear_DLC_HW();
	}
#endif
	else if( !strcmp( gCliArgs[0], "encryptoff" ) )
	{
		g_bEncryptFlag=OFF;
		GLogN( "encrypt off\r\n" );		
	}
	else if( !strcmp( gCliArgs[0], "encrypton" ) )
	{
		g_bEncryptFlag=ON;
		GLogN( "encrypt on\r\n" );		
	}
	else if( !strcmp( gCliArgs[0], "encryptkey" ) )
	{
		GLogN( "\r\n" );
		GLogN( "AES256_Key: " );
		for(i=0; i<32; i++)
		{
			GLogI( "%02X ",g_ucAES256_Key[i]);
		}
	}
#if 1
	else if( !strcmp( gCliArgs[0], "hsmupdate" ) )
	{
		extern U32 g_HSM_Timer;
		extern void HSM_Update_Ack( bool state, int progress );
		U32 uiReturnCode = 0;
		U32 ok = 0;
		U32 fail = 0;
		U8 authkey[] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
						0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
						0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};

		//while(1)
		{
		  GLogN("\r\nHSM Update Count : %d (ok : %d, fail : %d)", i, ok, fail);
			//g_HSM_Timer = SetSWTimer( 3000, eSWTimer_INFINITE, HSM_Update_Ack, true );
			uiReturnCode = VCI3_HSM_UPDATE(authkey);
			
			if(uiReturnCode == HSM_SUCCESS)
			{
			  ok++;
			}
			else
			{
			  fail++;
			}
		}
	}
#endif
	else if( !strcmp( gCliArgs[0], "hsmack" ) )
	{
		extern void HSM_Update_Ack( bool state, int progress );
		extern SHSMUpdateAck *pHSMAck;
		if( count == 3 )
		{
			bool   bResult   = (bool)atoi( gCliArgs[1] );
			int    iProgress = atoi( gCliArgs[2] );
			pHSMAck->mResult   = bResult;
			pHSMAck->mProgress = iProgress;
			GLogN( "hsmack test: result=%d, progress=%d\r\n", bResult, iProgress );
			HSM_Update_Ack( bResult, iProgress );
		}
		else
		{
			GLogN( "Usage: hsmack <result> <progress>\r\n" );
			GLogN( "  ex) hsmack 0 100  -> success, 100%%\r\n" );
			GLogN( "  ex) hsmack 1 0    -> fail, 0%%\r\n" );
		}
	}
	else if( !strcmp( gCliArgs[0], "mkdir" ) )
	{
		switch (count)
		{
		case 1:
		  GLogN( "please input 'directory name'\r\n" );
		  break;
		case 2:
		  memcpy(Path, &gCliArgs[1], sizeof(Path));
		  VciMakeDirWithPath(Path);
		  break;
		}
		
	}
	else if( !strcmp( gCliArgs[0], "rm" ) )
	{
        char arrTemp[128];
        f_getcwd(arrTemp, sizeof(arrTemp));
        sprintf(arrTemp, "%s", gCliArgs[1]);

        if ( f_unlink(arrTemp) == FR_OK )
            GLogN( "Remove OK : %s\r\n", arrTemp);
	}
    else if ( !strcmp( gCliArgs[0], "dir" ) )
		{
        char arrTemp[128];
        f_getcwd(arrTemp, sizeof(arrTemp));
        GLogN("Current Directory : %s\r\n", arrTemp);
        directory_listWithPath(arrTemp);
		}
    else if ( !strcmp( gCliArgs[0], "cd" ) )
    {
        char arrTemp[128];
        f_getcwd(arrTemp, sizeof(arrTemp));
        if ( count == 1 )
        {
            f_getcwd(arrTemp, sizeof(arrTemp));
            GLogN("Get Current Direcory : %s\r\n", arrTemp);    
	}
        else
        {
            if ( strcmp(gCliArgs[1], "/") == 0 )
            {
                f_chdir(gCliArgs[1]);
            }
            else
            {
                if ( strcmp(arrTemp, DIR_ROOT) == 0 )
                {
                    sprintf(arrTemp, "/%s", gCliArgs[1]);
                }
                else
                {
                    sprintf(arrTemp, "%s", gCliArgs[1]);
                }
                f_chdir(arrTemp);

                f_getcwd(arrTemp, sizeof(arrTemp));
                GLogN("Change Direcory : %s\r\n", arrTemp);    
            }
        }
    }
    else if( !strcmp( gCliArgs[0], "mklst" ) )
	{
		//StoreLstEmmc(Data,sizeof(Data));
	}
#if 0
    else if( !strcmp( gCliArgs[0], "readcert" ) )
	{
        U8 data[600];
        U32 len = 0;
        U32 		uiReturnCode = 200;
        memset(data, 0x00, sizeof(data));
        
        uiReturnCode = ActivationHSM();
        for(int k=1; k<5; k++)
        {
            uiReturnCode = ReadCertificateHSM(&data[0], (int*)&len, k);
            
            GLogN("\r\nCert(%d) :", k);
            for(int i=0; i<len; i++)
            {
                GLogN(" %02X", data[i]);
            }
            GLogN("\r\n\n");
            
            uiReturnCode = ReadDataHSM(&data[0], len, k);
            GLogN("\r\nCRL(%d) :", k);
            for(int i=0; i<len; i++)
            {
                GLogN(" %02X", data[i]);
            }
            GLogN("\r\n\n");
        }
	}
    else if( !strcmp( gCliArgs[0], "hsmtest" ) )
    {
        U8 data[501] ={0x01, 0x00, 0x01, 0x00, 0x02, 0x08, 0x40, 0x00, 0x11, 0x35, 0x49, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x4D, 0x43, 0x03, 0x43, 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x48, 0x4D, 0x43, 0x08, 0x47, 0x4B, 0x49, 0x41, 0x55, 0x50, 0x53, 0x30, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x10, 0x00, 0x37, 0x78, 0xE8, 0x25, 0xD3, 0x65, 0x37, 0x0D, 0xEE, 0x58, 0xEA, 0x74, 0x9A, 0xF4, 0x5D, 0x43, 0xBE, 0x27, 0xC2, 0x9C, 0x84, 0x78, 0x66, 0x4B, 0x58, 0xF6, 0x93, 0x85, 0xDF, 0xC3, 0xBF, 0xB2, 0x23, 0x11, 0x28, 0x24, 0x01, 0x02, 0x00, 0x01, 0x90, 0x01, 0x00, 0x00, 0x01, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x24, 0x70, 0x02, 0x8C, 0xC4, 0x5A, 0xA9, 0x47, 0x80, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9C, 0x92, 0x41, 0x4A, 0x01, 0x31, 0x22, 0x09, 0x01, 0x68, 0x51, 0x41, 0x28, 0x00, 0x10, 0x02, 0x00, 0x10, 0x00, 0x04, 0x08, 0x00, 0xA8, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x8C, 0xB4, 0x18, 0x4D, 0xDE, 0x30, 0xF0, 0xF5, 0xA4, 0x68, 0x32, 0xE5, 0x48, 0xA9, 0xC5, 0xF7, 0x5D, 0xEA, 0xB1, 0xCB, 0x91, 0x14, 0x12, 0x5F, 0xF9, 0x81, 0xA3, 0x31, 0x5F, 0x89, 0xC6, 0x06, 0xC1, 0x71, 0xC3, 0xEC, 0x2C, 0x5C, 0xBF, 0x23, 0x20, 0x22, 0xE1, 0x93, 0x0C, 0x2E, 0xDA, 0xB5, 0xB0, 0x9E, 0x7C, 0x4C, 0xB4, 0x74, 0x05, 0x10, 0x17, 0x4A, 0xF4, 0xC3, 0x8A, 0x6E, 0x4C, 0xAD, 0x03, 0x88, 0x63, 0x0A, 0xD7, 0xA5, 0xD9, 0xEB, 0x50, 0x2C, 0x0B, 0xB4, 0xAB, 0xA3, 0xE6, 0x86, 0x78, 0x1A, 0x96, 0x64, 0x35, 0x1B, 0x69, 0x3A, 0x01, 0xA2, 0xD7, 0xA6, 0x5D, 0xA2, 0x67, 0xC7, 0xAF, 0x34, 0x90, 0xB1, 0xCD, 0x8B, 0xDA, 0x2B, 0x77, 0xFF, 0xC3, 0x65, 0xB8, 0x4F, 0xE5, 0xC2, 0x33, 0x9F, 0x4A, 0xBD, 0xC8, 0x8C, 0x64, 0x05, 0x0F, 0x8B, 0xDA, 0x18, 0xBC, 0xD0, 0xE4, 0x09, 0xDE, 0xF3, 0xD9, 0x42, 0xD7, 0xAC, 0xED, 0xE8, 0x14, 0x8A, 0xAF, 0x9B, 0x9D, 0xFF, 0xBE, 0x45, 0x5E, 0xFD, 0xE4, 0xE4, 0x91, 0x5B, 0x46, 0xF8, 0xE9, 0x4B, 0x03, 0x77, 0x70, 0x68, 0x84, 0xCE, 0x2D, 0x25, 0xFD, 0xEF, 0x1A, 0x09, 0xE6, 0xCF, 0xED, 0xB1, 0xB5, 0xA5, 0x8E, 0x8F, 0xCF, 0x35, 0x42, 0x5D, 0x4B, 0x5A, 0xB7, 0x96, 0xF2, 0xEB, 0x37, 0x7C, 0x26, 0x3B, 0x82, 0xC2, 0x5B, 0xD5, 0x93, 0x42, 0xF5, 0xF2, 0x8B, 0xF7, 0x7A, 0x43, 0xFD, 0x8F, 0xE9, 0x26, 0xB7, 0x39, 0xCC, 0xD5, 0x38, 0x9C, 0x71, 0x63, 0x8B, 0xBA, 0xB3, 0x49, 0x40, 0xA0, 0x02, 0x56, 0x9A, 0x63, 0xEB, 0x47, 0x6F, 0x29, 0x5D, 0xA8, 0x71, 0xE6, 0xA6, 0x93, 0x19, 0xF7, 0x75, 0x39, 0x56, 0x77, 0xB7, 0x2F, 0x48, 0x56, 0x52, 0x37, 0x7B, 0xDB, 0x90, 0x1F, 0xDF, 0x85, 0xA3, 0xD9, 0x3E, 0xCE, 0x13};
        memset(data, 0xaa, sizeof(data));
        WriteDataHSM(&data[0], 501, 1);
        WriteDataHSM(&data[0], 501, 3);
    }
    else if( !strcmp( gCliArgs[0], "putcert" ) )
    {
        U8 data[600] = {0x01, 0x48, 0x4D, 0x43, 0x03, 0x43, 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23, 0x01, 0x13, 0x24, 0x01, 0x21, 0x00, 0x01, 0x00, 0x02, 0x04, 0x10, 0x00, 0x20, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x4D, 0x43, 0x08, 0x47, 0x4B, 0x49, 0x41, 0x55, 0x50, 0x53, 0x30, 0x00, 0x01, 0x99, 0x30, 0x00, 0x60, 0x00, 0x07, 0x00, 0x01, 0x00, 0x02, 0x08, 0x40, 0x00, 0x11, 0x35, 0x49, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xCE, 0xEC, 0xD9, 0xF1, 0x58, 0x14, 0xC5, 0x97, 0xD3, 0xD5, 0x7F, 0xC4, 0xED, 0x64, 0x16, 0x23, 0xFC, 0x54, 0x2E, 0xB1, 0x8E, 0xCE, 0xFE, 0x52, 0x45, 0x75, 0xD8, 0xE3, 0x59, 0xD6, 0x90, 0x3B, 0x35, 0xE3, 0x59, 0x11, 0xFB, 0x4C, 0xC4, 0x04, 0x4C, 0x61, 0xB6, 0x98, 0x02, 0xFC, 0xC6, 0xB5, 0x55, 0xF7, 0x49, 0xD9, 0xC6, 0x8F, 0x7E, 0x2B, 0x13, 0x7F, 0x6E, 0xDA, 0x43, 0x5A, 0x53, 0x76, 0xD5, 0x37, 0x40, 0x53, 0xF6, 0x85, 0x6B, 0x12, 0xF4, 0x9C, 0x0E, 0xDA, 0x77, 0x30, 0x0B, 0x46, 0x87, 0x41, 0x4F, 0x87, 0x22, 0x58, 0x5F, 0x3E, 0xAC, 0x07, 0x64, 0xDD, 0xE0, 0xA9, 0xD2, 0x31, 0x25, 0x96, 0x60, 0x4A, 0xC6, 0xE8, 0xD9, 0x54, 0xB5, 0xAB, 0xA2, 0xF4, 0x4A, 0xDB, 0x00, 0x64, 0x6A, 0x8C, 0xB8, 0xC5, 0xFA, 0xD8, 0xDA, 0x32, 0x16, 0xD1, 0x05, 0xFA, 0x9C, 0x71, 0x1D, 0xB8, 0x7C, 0x7C, 0x80, 0x5D, 0x31, 0xD8, 0x91, 0x8E, 0x8F, 0xB8, 0xDE, 0x9D, 0x55, 0xFF, 0xD5, 0x95, 0x68, 0x5D, 0x6F, 0x27, 0xE5, 0x57, 0x51, 0xC7, 0xC9, 0xAF, 0xA3, 0x7A, 0x93, 0x9E, 0xC2, 0xA1, 0xB3, 0x29, 0xA6, 0x73, 0xC9, 0x8C, 0xD8, 0xC0, 0x5C, 0x3E, 0xF6, 0xB7, 0x47, 0xBD, 0x31, 0xAE, 0x8F, 0xBE, 0x8B, 0x6F, 0xAD, 0x61, 0x56, 0x9D, 0xFC, 0x9D, 0xF5, 0xEF, 0xC4, 0xA5, 0xF6, 0x70, 0xA2, 0x55, 0x2C, 0x50, 0x20, 0x2C, 0xA0, 0x38, 0x12, 0xDE, 0x65, 0xEF, 0xA8, 0xFA, 0xA3, 0x33, 0x79, 0x67, 0x0C, 0x3A, 0x64, 0x51, 0xC1, 0xEA, 0xC2, 0xDE, 0xCF, 0xDF, 0x1A, 0x68, 0xF9, 0x33, 0xB5, 0x8B, 0x04, 0xBC, 0x26, 0x07, 0x2D, 0x43, 0xA3, 0xF1, 0x6D, 0xA6, 0x47, 0x78, 0x66, 0xAE, 0xC4, 0x1B, 0xA7, 0xED, 0x59, 0x67, 0x56, 0x56, 0x07, 0x98, 0x7F, 0x77, 0x70, 0x49, 0x5C, 0x79, 0xB1, 0x21, 0x99, 0xB1, 0x27, 0x76, 0x9B, 0x9D, 0x1B, 0xFC, 0x88, 0x70, 0xEC, 0x6D, 0x35, 0x4B, 0x20, 0x72, 0xE0, 0x73, 0xD0, 0x1D, 0x1E, 0xF1, 0x07, 0xD0, 0x43, 0x86, 0xE4, 0x89, 0x69, 0xD0, 0xB0, 0xFC, 0x71, 0x63, 0x4C, 0x18, 0x6B, 0xCF, 0x4D, 0xE0, 0x91, 0x01, 0x9D, 0xF3, 0x06, 0xC7, 0xF1, 0xEE, 0x8B, 0x47, 0x9A, 0x3F, 0x26, 0x77, 0x3D, 0x9F, 0x8C, 0x30, 0x84, 0x9E, 0xFF, 0x14, 0x29, 0x66, 0x2D, 0xCF, 0xAB, 0x61, 0xFC, 0xA4, 0xB0, 0xE0, 0xA9, 0xFB, 0xA8, 0xC1, 0xC5, 0x0C, 0x76, 0x60, 0x92, 0xD6, 0x7D, 0xCD, 0x3E, 0x69, 0x75, 0x23, 0x7B, 0xBF, 0x7A, 0xC1, 0xCB, 0xCC, 0x6E, 0x66, 0xA2, 0xE4, 0x40, 0x94, 0x4A, 0x64, 0xF5, 0x38, 0xA2, 0x95, 0x38, 0xA4, 0x85, 0xD9, 0xD1, 0x5E, 0x85, 0xE3, 0x60, 0x30, 0xC0, 0x08, 0x70, 0xB1, 0xFC, 0xEE, 0xEE, 0x07, 0x41, 0x25, 0x0D, 0xDA, 0x94, 0x52, 0xA0, 0xA8, 0xEA, 0x2A, 0x91, 0xF0, 0x0F, 0xF8, 0x85, 0x9F, 0xDF, 0x96, 0x88, 0xBD, 0x84, 0x2C, 0xD3, 0xE4, 0x75, 0xB9, 0x04, 0xE3, 0xEA, 0x7F, 0x48, 0x6C, 0x18, 0x96, 0xED, 0xB2, 0x33, 0x7B, 0x7E, 0x5F, 0x91, 0x8D, 0x58, 0xB8, 0x1B, 0x6A, 0x53, 0xA9, 0xA3, 0xB3, 0x54, 0xC7, 0xCA, 0xC3, 0x64, 0x05, 0x04, 0xCE, 0x02, 0x27, 0x7F, 0x89, 0x0C, 0x8C, 0xF0, 0x1F, 0x64, 0x31, 0x76, 0x61, 0x6F, 0x7E, 0x5A, 0x9E, 0x20, 0x4E, 0xF2, 0x9D, 0x39, 0xED, 0xE6, 0xBB, 0x33, 0x5C, 0x5C, 0x86, 0x57, 0x72, 0x77, 0x21, 0x20, 0xB4, 0xC0, 0xEF, 0xBF, 0x85, 0xC1, 0x0C, 0x74, 0x84, 0x71, 0xDD, 0x9E, 0x1F, 0x42, 0xE1, 0xB7, 0x44, 0x41, 0xD4, 0x7B, 0x91, 0x80, 0xFD, 0x40, 0x67, 0xB3, 0xF4, 0x77, 0x3B, 0x1A, 0x67, 0xAF, 0xBF, 0x84, 0xBF, 0xC4, 0x33, 0xC3, 0xC9};
        U8 output[1000];
        U8 output2[1000];
        u32 len = 0;
        memset(data, 0xbb, sizeof(data));
        memset(output, 0x00, sizeof(output));
        memset(output2, 0x00, sizeof(output));
        
        StoreCertificateHSM(&data[0], 600, 1);
        ReadCertificateHSM(output2, (int*)&len, 1);
        ReadDataHSM(&output[0], 600, 1);
        
        StoreCertificateHSM(&data[0], 600, 3);
        ReadCertificateHSM(output2, (int*)&len, 3);
        ReadDataHSM(&output[0], 600, 3);
    }
    else if( !strcmp( gCliArgs[0], "hsmupdate" ) )
    {
        extern U32 g_HSM_Timer;
        extern void HSM_Update_Ack( bool state, int progress );
        U32 uiReturnCode = 0;
        U32 ok = 0;
        U32 fail = 0;
        U8 authkey[] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
                        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
                        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47};

        //while(1)
        {
          GLogN("\r\nHSM Update Count : %d (ok : %d, fail : %d)", i, ok, fail);
            //g_HSM_Timer = SetSWTimer( 3000, eSWTimer_INFINITE, HSM_Update_Ack, true );
            uiReturnCode = VCI3_HSM_UPDATE(authkey);
            
            if(uiReturnCode == HSM_SUCCESS)
            {
              ok++;
            }
            else
            {
              fail++;
            }
        }
    }
    else if( !strcmp( gCliArgs[0], "codekey" ) )
    {
        U32 uiReturnCode = 300;
        U8 ucChecksum = 0;
        
        uiReturnCode = ActivationHSM();
        uiReturnCode = ReadAES128KeyChecksumHSM(AES_INDEX_6, &ucChecksum);
        
        if((ucChecksum==0x7B) && (uiReturnCode==HSM_SUCCESS)) GLogN("\r\ncodekey ok!!!");
        else GLogN("\r\nthere is no codekey");
    }
    else if( !strcmp( gCliArgs[0], "sign" ) )
    {
        U32 uiReturnCode = 300;
        U8 seed[8] = {0x00, };
        U8 out[200] = {0x00, };
        U32 uiOutDataLen = 0;
        
        memset(seed, 0x34, sizeof(seed));
        
        uiReturnCode = ActivationHSM();
        uiReturnCode = SignPrivateHSM(seed, 8, out, (int*)&uiOutDataLen, 1);
        
        GLogN("\r\n Result : %d", uiReturnCode);
    }
    else if( !strcmp( gCliArgs[0], "hsmreset" ) )
    {
        U8 cert[600];
        U32 ret = 0;
        
        memset(cert, 0x00, sizeof(cert));
        
        ret = DeleteDataHSM(SIZE_GIT_FILES, 1);
        if(ret != HSM_SUCCESS) GLogE("\r\n Error1 !!!!!!!!!!");
        ret = DeleteDataHSM(SIZE_GIT_FILES, 2);
        if(ret != HSM_SUCCESS) GLogE("\r\n Error2 !!!!!!!!!!");
        ret = DeleteDataHSM(SIZE_GIT_FILES, 3);
        if(ret != HSM_SUCCESS) GLogE("\r\n Error3 !!!!!!!!!!");
        ret = DeleteDataHSM(SIZE_GIT_FILES, 4);
        if(ret != HSM_SUCCESS) GLogE("\r\n Error4 !!!!!!!!!!");
        
        ret = StoreCertificateHSM(&cert[0], 600, 1);
        if(ret != HSM_SUCCESS) GLogE("\r\n Error5 !!!!!!!!!!");
        ret = StoreCertificateHSM(&cert[0], 600, 2);
        if(ret != HSM_SUCCESS) GLogE("\r\n Error6 !!!!!!!!!!");
        ret = StoreCertificateHSM(&cert[0], 600, 3);
        if(ret != HSM_SUCCESS) GLogE("\r\n Error7 !!!!!!!!!!"); 
        ret = StoreCertificateHSM(&cert[0], 600, 4);
        if(ret != HSM_SUCCESS) GLogE("\r\n Error8 !!!!!!!!!!");
    }

    else if( !strcmp( gCliArgs[0], "ask" ) )
    {
        uint8_t seed[10][8] = {
		{ 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 },
		{ 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 },
		{ 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 },
		{ 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA },
		{ 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB },
		{ 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC },
		{ 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD },
		{ 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE },
		{ 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF },
		{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
		};
        //uint8_t a[16] = {0x87, 0x87, 0xb6, 0xb1, 0xf3, 0xf9, 0x46, 0xb7, 0xb0, 0x2c, 0x39, 0xee, 0xd3, 0xc2, 0x99, 0xc0};
        uint8_t test_ecu_code[16] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48 };
        uint8_t test_key[16] = {0x56, 0x43, 0x49, 0x33, 0x21, 0x48, 0x73, 0x6D, 0x40, 0x41, 0x65, 0x73, 0x23, 0x4B, 0x65, 0x79};
        uint8_t test_iv[16] = {0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48, 0x53, 0x4D, 0x41, 0x45, 0x53, 0x5F, 0x49, 0x56};
        
        uint8_t test_out[200];
        uint8_t test_sign[8];
        uint32_t outlen = 0;
        uint32_t ret = 0;
        uint32_t oldtime = 0;
        
        memset(test_out, 0x00, sizeof(test_out));
        
        ActivationHSM();
        
        for(U8 i=0; i<10; i++)
        {
            getAESEncoding_CTR(test_ecu_code, 16, test_key, test_iv, test_out, &outlen);
            //memcpy(test_out, a, 16);
            oldtime = Get_Tmr();
            ret = ASKSignHSM(&seed[i][0], test_out, test_iv, test_sign);
            GLogN("Time : %d =====> ", Get_TmrDelta( Get_Tmr(), oldtime ));
            
            if(ret == HSM_SUCCESS)
            {
                GLogN("Seed : ");
                for(int y=0; y<8; y++)
                {
                    GLogN(" %02x", seed[i][y]);
                }
                
                GLogN("   ---------> Code : ");
                for(int k=0; k<outlen; k++)
                {
                    GLogN(" %02x", test_out[k]);
                }
                
                GLogN("   ---------> Sign : ");
                for(int g=0; g<sizeof(test_sign); g++)
                {
                    GLogN(" %02x", test_sign[g]);
                }
                GLogN("\n\n\r");
            }
            else
            {
                GLogN("\n\n\rASK Fail (%d)", ret);
            }
        }
    }
    else if( !strcmp( gCliArgs[0], "rsa" ) )
    {
        U32 ret = 0;
        U8 PubKey[200];
        U32 PubKeyLen = 0;
        U8 EncData[128] = {0x78, 0x44, 0xd2, 0xd0, 0x96, 0xac, 0x21, 0x33, 0x85, 0x4b, 0x5, 0x8d, 0x70, 0xe9, 0x14, 0x8a, 0xfa, 0x3f, 0x6c, 0x80, 0x71, 0xcf, 0x43, 0xe8, 0xe3, 0x4a, 0x8e, 0xf3, 0xf1, 0xf, 0xf0, 0x49, 0x94, 0x6c, 0x9a, 0x70, 0x9f, 0xa7, 0xdc, 0x5b, 0x8f, 0x1b, 0x13, 0xcf, 0xcb, 0x12, 0xeb, 0xd3, 0xf, 0x56, 0x86, 0x24, 0x93, 0x41, 0x78, 0x84, 0xc2, 0x6f, 0x2c, 0x62, 0x56, 0x2a, 0x1b, 0xb0, 0x56, 0x1c, 0x62, 0xbe, 0xee, 0xbc, 0x95, 0xe5, 0xd7, 0xb4, 0x54, 0x63, 0x11, 0xd3, 0x92, 0x91, 0x70, 0xbe, 0x7f, 0x8d, 0x46, 0xaf, 0x29, 0x7, 0x64, 0x17, 0xe7, 0x22, 0x5d, 0x47, 0x34, 0x69, 0xec, 0xb7, 0xd6, 0x9b, 0x11, 0x3d, 0x6d, 0x12, 0x9, 0xfc, 0xf, 0x86, 0xea, 0x54, 0x59, 0x29, 0xd9, 0x64, 0xbe, 0xd8, 0x22, 0x21, 0xf3, 0xb6, 0xc0, 0xcb, 0xc7, 0x7c, 0xe8, 0xaa, 0x75, 0x10};
        U8 Data[] = "HSM_RSA_TEST";
        U8 fdata[200];// = {0x78, 0x44, 0xd2, 0xd0, 0x96, 0xac, 0x21, 0x33, 0x85, 0x4b, 0x5, 0x8d, 0x70, 0xe9, 0x14, 0x8a, 0xfa, 0x3f, 0x6c, 0x80, 0x71, 0xcf, 0x43, 0xe8, 0xe3, 0x4a, 0x8e, 0xf3, 0xf1, 0xf, 0xf0, 0x49, 0x94, 0x6c, 0x9a, 0x70, 0x9f, 0xa7, 0xdc, 0x5b, 0x8f, 0x1b, 0x13, 0xcf, 0xcb, 0x12, 0xeb, 0xd3, 0xf, 0x56, 0x86, 0x24, 0x93, 0x41, 0x78, 0x84, 0xc2, 0x6f, 0x2c, 0x62, 0x56, 0x2a, 0x1b, 0xb0, 0x56, 0x1c, 0x62, 0xbe, 0xee, 0xbc, 0x95, 0xe5, 0xd7, 0xb4, 0x54, 0x63, 0x11, 0xd3, 0x92, 0x91, 0x70, 0xbe, 0x7f, 0x8d, 0x46, 0xaf, 0x29, 0x7, 0x64, 0x17, 0xe7, 0x22, 0x5d, 0x47, 0x34, 0x69, 0xec, 0xb7, 0xd6, 0x9b, 0x11, 0x3d, 0x6d, 0x12, 0x9, 0xfc, 0xf, 0x86, 0xea, 0x54, 0x59, 0x29, 0xd9, 0x64, 0xbe, 0xd8, 0x22, 0x21, 0xf3, 0xb6, 0xc0, 0xcb, 0xc7, 0x7c, 0xe8, 0xaa, 0x75, 0x10};
        
        memset(PubKey, 0x00, sizeof(PubKey));
        //memset(EncData, 0x00, sizeof(EncData));
        memset(fdata, 0x00, sizeof(fdata));
        
        ret = ActivationHSM();
        
        if(ret == HSM_SUCCESS)
        {
            //ret = GenerateRSAKeypairHSM();
            
            if(ret == HSM_SUCCESS)
            {
                ret = ReadPublicKeyHSM2(PubKey, &PubKeyLen);
                //setRSA_Encrypt(Data, 128, PubKey, PubKeyLen , EncData);
                
                if(ret == HSM_SUCCESS)
                {
                    DecryptPrivateKeyHSM(EncData, fdata);
                    GLogN("%02X", fdata[0]);
                }
            }
        }
    }
#endif
	else if( !strcmp( gCliArgs[0], "cat" ) )
	{
        FIL fpVer;
        FILINFO File_Info;
        FRESULT fResult;
        char arrTemp[128];
        unsigned int unReadLen, unFileSize;
		switch (count)
		{
		case 1:
		  GLogN( "please input 'file name' or 'directory name'\r\n" );
		  break;
		case 2:
                f_getcwd(arrTemp, sizeof(arrTemp));

                if ( strcmp(arrTemp, DIR_ROOT) == 0 )
                    sprintf(arrTemp, "%s%s", arrTemp, &gCliArgs[1]);
                else
                    sprintf(arrTemp, "%s/%s", arrTemp, &gCliArgs[1]);
                fResult = f_stat(arrTemp, &File_Info);

                if ( fResult == FR_OK )
                {
                    GLogN( "\r\n" );
                    GLogN( "Read File : %s\r\n", arrTemp);
                    GLogN( "\r\n" );
                    unFileSize = File_Info.fsize;
                    f_open(&fpVer, arrTemp, FA_OPEN_EXISTING|FA_READ);

                    for( int i=0; i<unFileSize; i+=unReadLen )
                    {
                        unReadLen = 0;
                        fResult = f_read(&fpVer, &arrTemp[0], 16, &unReadLen);
		
                        if ( fResult == FR_OK )
                        {
                            for ( int j=0; j<unReadLen; j++ )
                                GLogN( "%02X ", arrTemp[j]);

                            for(int l=0; l<(16 - unReadLen)*3; l++ )
                                GLogN( " ");

                            GLogN( "\t");
                            
                            for ( int k=0; k<unReadLen; k++ )
                            {
                                if ( arrTemp[k] >= 0x20 && arrTemp[k] <=0x7E )
                                    GLogN( "%c", arrTemp[k]);
                                else
                                    GLogN( ".");
                            }
                            GLogN( "\r\n" );
                        }
                        else
                            break;
                    }
                    f_close(&fpVer);
                    GLogN( "\r\nFile size : %d bytes\r\n", unFileSize);
                }
                else
                    GLogN( "check input 'file name' \r\n" );
		  break;
		}
		
	}
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
    else if( !strcmp( gCliArgs[0], "rdbi" ) )
	{
		StoreRdbiEmmc(Data,sizeof(Data));
		
	}
	else if( !strcmp( gCliArgs[0], "stdr" ) )
	{
		StoreSTDA_ReproEmmc(Data,sizeof(Data));
		
	}
	else if( !strcmp( gCliArgs[0], "mklst" ) )
	{
		StoreLstEmmc(Data,sizeof(Data));
	}
	/*
	else if( !strcmp( gCliArgs[0], "cat" ) )
	{
		switch (count)
		{
		case 1:
		  GLogN( "please input 'file name' or 'directory name'\r\n" );
		  break;
		case 2:
		  memcpy(Path, &gCliArgs[1], sizeof(Path));
		  if ( GetVCI2FWAppList(&ucData[0], &iLength) == TRUE )
		
			ucData[iLength]='\0';
			UartPrintf(&U1, "\n\r%s\n\r", ucData);
		  break;
		}
	}
	*/
#endif
#if 0
	else if( !strcmp( gCliArgs[0], "CRL" ) )
	{
		U8 Data[510];
		GetCrlFileRead(FILENAME_CRL_GIT,&Data[1], 506);

	GLogN( "CRL Effective Date : %02X %02X %02X\r\n", Data[90],Data[91],Data[92] );
	GLogN( "CRL Expiration Date : %02X %02X %02X\r\n", Data[93],Data[94],Data[95] );
		
	}
	else if( !strcmp( gCliArgs[0], "HSM" ) )
	{
		U32 		uiOutDataLen = 0;
		U8			arrOutData[1000];
		U8 mHSM_Ver[2] = {0x00, 0x00};
		U16 version=0;
		U32 Version_state = 300;
		
		ActivationHSM();

		Version_state = ReadVersionHSM(mHSM_Ver); 
		if(Version_state == 200)
		{
			version = ((mHSM_Ver[0]-0x30)*10) + (mHSM_Ver[1]-0x30);
		}
		GLogN("HSM_Current_Version:%d\r\n",version);
		ReadCertificateHSM(arrOutData, (int*)&uiOutDataLen);
		GLogN("CRT DATE:");
		for(i=0; i<6; i++) GLogN(" %02X",arrOutData[15+i]);
		GLogN("\r\n");
		GLogN("CRT HOLDER_REF:");
		for(i=0; i<16; i++) GLogN(" %02X",arrOutData[44+i]);
		GLogN("\r\n");
		
	}
#endif
	else if( !strcmp( gCliArgs[0], "getmode" ) )
	{
		GLogN("mode:%d\r\n",GetCurFwServiceMode());
	}
	/*else if( !strcmp( gCliArgs[0], "3002" ) )
	{
		stMsgClst	*msg;
		stCommPkt	*pkt;
		unsigned char ucReq[]={0x00, 0x00, 0x02,
			                   0x48, 0x00, 0x80, 0x00, 0x04, 0x43, 0x00, 0x02, 0x30, 0x00, 0x00, 0x03, 0x0B,
			                   0x01, 0x01, 0x05, 0x07, 0xE0, 0x02, 0x10, 0x90,
			                   0x03, 0x1E};
		InitGITSetConfig();
		InitGITHWSetData();
		VCI_HW_Setting(0x0100);
		msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
		if( msg == NULL )
		{
			GLogN("alloc fail hMsgPool\r\n");
		}

		pkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
		if( pkt == NULL )
		{
			osPoolFree( hMsgPool, (void *)msg );
			GLogN("alloc fail hCommPKPool\r\n");
		}
		msg->mMsgType	= MSG_COMM;
		msg->mPktType	= PACKET_UART;
		msg->pPacket	= (void*)pkt;
        memcpy(pkt,ucReq,sizeof(ucReq));

		if( osMessageAvailableSpace(hParsingMsg) == 0 )
		{
			osPoolFree( hMsgPool, (void *)msg );
			osPoolFree( hCommPKPool, (void *)pkt );
		}
		else
		{
			osMessagePut( hParsingMsg, (uint32_t)msg, osWaitForever );
		}
	}*/
	else if( !strcmp( gCliArgs[0], "sendcan" ) )
	{
		stMsgClst	*msg;
		stCommPkt	*pkt;
		int nLen=0;
		unsigned char ucReq[]={0x00, 0x00, 0x02,
			                   0x48, 0x00, 0x80, 0x00, 0x04, 0x43, 0x00, 0x48, 0x12, 0x00, 0x00, 0x03, 0x0B,
			                   0x07, 0xE0, 0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
							   0x03, 0x68};
		
		gCliArgs[1][MAX_CLI_ARGS-1] = 0x00;
		nLen = strlen( gCliArgs[1] );
		if( nLen != 0 )
		{
			for(int k=0; k<nLen/2; k++)
			{
				ucReq[k+16] = AsciiToHex(gCliArgs[1][k*2],gCliArgs[1][k*2+1]);
			}
		}
		else
		{
		}
		
		msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
		if( msg == NULL )
		{
			GLogN("alloc fail hMsgPool\r\n");
		}

		pkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
		if( pkt == NULL )
		{
			osPoolFree( hMsgPool, (void *)msg );
			GLogN("alloc fail hCommPKPool\r\n");
		}
		msg->mMsgType	= MSG_COMM;
		msg->mPktType	= PACKET_UART;
		msg->pPacket	= (void*)pkt;
        memcpy(pkt,ucReq,sizeof(ucReq));

		if( osMessageAvailableSpace(hParsingMsg) == 0 )
		{
			osPoolFree( hMsgPool, (void *)msg );
			osPoolFree( hCommPKPool, (void *)pkt );
		}
		else
		{
			osMessagePut( hParsingMsg, (uint32_t)msg, osWaitForever );
		}
		memset(&gCliArgs,0x00,sizeof(gCliArgs));
	}
	else if( !strcmp( gCliArgs[0], "readcan" ) )
	{
		stMsgClst	*msg;
		stCommPkt	*pkt;
		unsigned char ucReq[]={0x00, 0x00, 0x02,
			                   0x48, 0x00, 0x80, 0x00, 0x04, 0x43, 0x00, 0x49, 0x12, 0x00, 0x00, 0x03, 0x01,
							   0x03, 0x68};
	
		msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
		if( msg == NULL )
		{
			GLogN("alloc fail hMsgPool\r\n");
		}

		pkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
		if( pkt == NULL )
		{
			osPoolFree( hMsgPool, (void *)msg );
			GLogN("alloc fail hCommPKPool\r\n");
		}
		msg->mMsgType	= MSG_COMM;
		msg->mPktType	= PACKET_UART;
		msg->pPacket	= (void*)pkt;
        memcpy(pkt,ucReq,sizeof(ucReq));

		if( osMessageAvailableSpace(hParsingMsg) == 0 )
		{
			osPoolFree( hMsgPool, (void *)msg );
			osPoolFree( hCommPKPool, (void *)pkt );
		}
		else
		{
			osMessagePut( hParsingMsg, (uint32_t)msg, osWaitForever );
		}
	}
#ifdef LISTDIAG
	else if( !strcmp( gCliArgs[0], "listdiag" ) )
	{
		stMsgClst	*msg;
		stCommPkt	*pkt;
		/*unsigned char ucReq[]={0x00, 0x00, 0x02,
			                   0x48, 0x00, 0x80, 0x00, 0x04, 0x43, 0x00, 0x00, 0x60, 0x00, 0x00, 0x03, 0x0B,
			                   0x01, 0x01, 0x05, 0x07, 0xE0, 0x02, 0x10, 0x90,
			                   0x02, 0x02, 0x06, 0x07, 0xE0, 0x03, 0x19, 0x02, 0x08,
			                   0x03, 0x02, 0x06, 0x07, 0xE0, 0x03, 0x19, 0x02, 0x08,
			                   0x04, 0x01, 0x05, 0x07, 0xE1, 0x02, 0x10, 0x90,
			                   0x05, 0x02, 0x06, 0x07, 0xE1, 0x03, 0x19, 0x02, 0x08,
			                   0x06, 0x01, 0x05, 0x07, 0xE2, 0x02, 0x10, 0x90,
			                   0x07, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
			                   0x03, 0x1E};*/
		unsigned char ucReq[]={0x00, 0x00, 0x02,
	                   0x58, 0x03, 0x80, 0x00, 0x04, 0x53, 0x03, 0x00, 0x60, 0x00, 0x00, 0x03, 0x0B,
	                   0x01, 0x01, 0x05, 0x07, 0xE0, 0x02, 0x10, 0x90,
	                   0x02, 0x02, 0x06, 0x07, 0xE0, 0x03, 0x19, 0x02, 0x08,
	                   0x03, 0x02, 0x06, 0x07, 0xE0, 0x03, 0x19, 0x02, 0x08,
	                   0x04, 0x01, 0x05, 0x07, 0xE1, 0x02, 0x10, 0x90,
	                   0x05, 0x02, 0x06, 0x07, 0xE1, 0x03, 0x19, 0x02, 0x08,
	                   0x06, 0x01, 0x05, 0x07, 0xE2, 0x02, 0x10, 0x90,
	                   0x07, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
	                   0x08, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
						0x09, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
						0x0A, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
						0x0B, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
						0x0C, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
						0x0D, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
						0x0E, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
						0x0F, 0x01, 0x05, 0x07, 0xE3, 0x02, 0x10, 0x90,
						0x10, 0x01, 0x05, 0x05, 0xA1, 0x02, 0x10, 0x90,
						0x11, 0x01, 0x05, 0x05, 0xA2, 0x02, 0x10, 0x90,
						0x12, 0x01, 0x05, 0x05, 0xA3, 0x02, 0x10, 0x90,
						0x13, 0x01, 0x05, 0x05, 0xA4, 0x02, 0x10, 0x90,
						0x14, 0x01, 0x05, 0x05, 0xA5, 0x02, 0x10, 0x90,
						0x15, 0x01, 0x05, 0x05, 0xA6, 0x02, 0x10, 0x90,
						0x16, 0x01, 0x05, 0x05, 0xA7, 0x02, 0x10, 0x90,
						0x17, 0x01, 0x05, 0x05, 0xA8, 0x02, 0x10, 0x90,
						0x18, 0x01, 0x05, 0x05, 0xA9, 0x02, 0x10, 0x90,
						0x19, 0x01, 0x05, 0x05, 0xAA, 0x02, 0x10, 0x90,
						0x20, 0x01, 0x05, 0x05, 0xB1, 0x02, 0x10, 0x90,
						0x21, 0x01, 0x05, 0x05, 0xB2, 0x02, 0x10, 0x90,
						0x22, 0x01, 0x05, 0x05, 0xB3, 0x02, 0x10, 0x90,
						0x23, 0x01, 0x05, 0x05, 0xB4, 0x02, 0x10, 0x90,
						0x24, 0x01, 0x05, 0x05, 0xB5, 0x02, 0x10, 0x90,
						0x25, 0x01, 0x05, 0x05, 0xB6, 0x02, 0x10, 0x90,
						0x26, 0x01, 0x05, 0x05, 0xB7, 0x02, 0x10, 0x90,
						0x27, 0x01, 0x05, 0x05, 0xB8, 0x02, 0x10, 0x90,
						0x28, 0x01, 0x05, 0x05, 0xB9, 0x02, 0x10, 0x90,
						0x29, 0x01, 0x05, 0x05, 0xBA, 0x02, 0x10, 0x90,
						0x30, 0x01, 0x05, 0x05, 0xC1, 0x02, 0x10, 0x90,
						0x31, 0x01, 0x05, 0x05, 0xC2, 0x02, 0x10, 0x90,
						0x32, 0x01, 0x05, 0x05, 0xC3, 0x02, 0x10, 0x90,
						0x33, 0x01, 0x05, 0x05, 0xC4, 0x02, 0x10, 0x90,
						0x34, 0x01, 0x05, 0x05, 0xC5, 0x02, 0x10, 0x90,
						0x35, 0x01, 0x05, 0x06, 0xC6, 0x02, 0x10, 0x90,
						0x36, 0x01, 0x05, 0x05, 0xC7, 0x02, 0x10, 0x90,
						0x37, 0x01, 0x05, 0x05, 0xC8, 0x02, 0x10, 0x90,
						0x38, 0x01, 0x05, 0x05, 0xC9, 0x02, 0x10, 0x90,
						0x39, 0x01, 0x05, 0x05, 0xCA, 0x02, 0x10, 0x90,
						0x40, 0x01, 0x05, 0x05, 0xD1, 0x02, 0x10, 0x90,
						0x41, 0x01, 0x05, 0x05, 0xD2, 0x02, 0x10, 0x90,
						0x42, 0x01, 0x05, 0x05, 0xD3, 0x02, 0x10, 0x90,
						0x43, 0x01, 0x05, 0x05, 0xD4, 0x02, 0x10, 0x90,
						0x44, 0x01, 0x05, 0x05, 0xD5, 0x02, 0x10, 0x90,
						0x45, 0x01, 0x05, 0x05, 0xD6, 0x02, 0x10, 0x90,
						0x46, 0x01, 0x05, 0x05, 0xD7, 0x02, 0x10, 0x90,
						0x47, 0x01, 0x05, 0x05, 0xD8, 0x02, 0x10, 0x90,
						0x48, 0x01, 0x05, 0x05, 0xD9, 0x02, 0x10, 0x90,
						0x49, 0x01, 0x05, 0x06, 0xDA, 0x02, 0x10, 0x90,
						0x50, 0x01, 0x05, 0x06, 0xE1, 0x02, 0x10, 0x90,
						0x51, 0x01, 0x05, 0x05, 0xE2, 0x02, 0x10, 0x90,
						0x52, 0x01, 0x05, 0x05, 0xE3, 0x02, 0x10, 0x90,
						0x53, 0x01, 0x05, 0x05, 0xE4, 0x02, 0x10, 0x90,
						0x54, 0x01, 0x05, 0x05, 0xE5, 0x02, 0x10, 0x90,
						0x55, 0x01, 0x05, 0x06, 0xE6, 0x02, 0x10, 0x90,
						0x56, 0x01, 0x05, 0x05, 0xE7, 0x02, 0x10, 0x90,
						0x57, 0x01, 0x05, 0x05, 0xE8, 0x02, 0x10, 0x90,
						0x58, 0x01, 0x05, 0x05, 0xE9, 0x02, 0x10, 0x90,
						0x59, 0x01, 0x05, 0x05, 0xEA, 0x02, 0x10, 0x90,
						0x60, 0x01, 0x05, 0x07, 0xA1, 0x02, 0x10, 0x90,
						0x61, 0x01, 0x05, 0x07, 0xA2, 0x02, 0x10, 0x90,
						0x62, 0x01, 0x05, 0x07, 0xA3, 0x02, 0x10, 0x90,
						0x63, 0x01, 0x05, 0x07, 0xA4, 0x02, 0x10, 0x90,
						0x64, 0x01, 0x05, 0x07, 0xA5, 0x02, 0x10, 0x90,
						0x65, 0x01, 0x05, 0x07, 0xA6, 0x02, 0x10, 0x90,
						0x66, 0x01, 0x05, 0x07, 0xA7, 0x02, 0x10, 0x90,
						0x67, 0x01, 0x05, 0x07, 0xA8, 0x02, 0x10, 0x90,
						0x68, 0x01, 0x05, 0x07, 0xA9, 0x02, 0x10, 0x90,
						0x69, 0x01, 0x05, 0x07, 0xAA, 0x02, 0x10, 0x90,
						0x70, 0x01, 0x05, 0x07, 0xB1, 0x02, 0x10, 0x90,
						0x71, 0x01, 0x05, 0x07, 0xB2, 0x02, 0x10, 0x90,
						0x72, 0x01, 0x05, 0x07, 0xB3, 0x02, 0x10, 0x90,
						0x73, 0x01, 0x05, 0x07, 0xB4, 0x02, 0x10, 0x90,
						0x74, 0x01, 0x05, 0x07, 0xB5, 0x02, 0x10, 0x90,
						0x75, 0x01, 0x05, 0x07, 0xB6, 0x02, 0x10, 0x90,
						0x76, 0x01, 0x05, 0x07, 0xB7, 0x02, 0x10, 0x90,
						0x77, 0x01, 0x05, 0x07, 0xB8, 0x02, 0x10, 0x90,
						0x78, 0x01, 0x05, 0x07, 0xB9, 0x02, 0x10, 0x90,
						0x79, 0x01, 0x05, 0x07, 0xBA, 0x02, 0x10, 0x90,
						0x80, 0x01, 0x05, 0x07, 0xC1, 0x02, 0x10, 0x90,
						0x81, 0x01, 0x05, 0x07, 0xC2, 0x02, 0x10, 0x90,
						0x82, 0x01, 0x05, 0x07, 0xC3, 0x02, 0x10, 0x90,
						0x83, 0x01, 0x05, 0x07, 0xC4, 0x02, 0x10, 0x90,
						0x84, 0x01, 0x05, 0x07, 0xC5, 0x02, 0x10, 0x90,
						0x85, 0x01, 0x05, 0x07, 0xC6, 0x02, 0x10, 0x90,
						0x86, 0x01, 0x05, 0x07, 0xC7, 0x02, 0x10, 0x90,
						0x87, 0x01, 0x05, 0x07, 0xC8, 0x02, 0x10, 0x90,
						0x88, 0x01, 0x05, 0x07, 0xC9, 0x02, 0x10, 0x90,
						0x89, 0x01, 0x05, 0x07, 0xCA, 0x02, 0x10, 0x90,
						0x90, 0x01, 0x05, 0x07, 0xD1, 0x02, 0x10, 0x90,
						0x91, 0x01, 0x05, 0x07, 0xD2, 0x02, 0x10, 0x90,
						0x92, 0x01, 0x05, 0x07, 0xD3, 0x02, 0x10, 0x90,
						0x93, 0x01, 0x05, 0x07, 0xD4, 0x02, 0x10, 0x90,
						0x94, 0x01, 0x05, 0x07, 0xD5, 0x02, 0x10, 0x90,
						0x95, 0x01, 0x05, 0x07, 0xD6, 0x02, 0x10, 0x90,
						0x96, 0x01, 0x05, 0x07, 0xD7, 0x02, 0x10, 0x90,
						0x97, 0x01, 0x05, 0x07, 0xD8, 0x02, 0x10, 0x90,
						0x98, 0x01, 0x05, 0x07, 0xD9, 0x02, 0x10, 0x90,
						0x99, 0x01, 0x05, 0x07, 0xDA, 0x02, 0x10, 0x90,
	                   0x03, 0x36};
		msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
		if( msg == NULL )
		{
			GLogN("alloc fail hMsgPool\r\n");
		}

		pkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
		if( pkt == NULL )
		{
			osPoolFree( hMsgPool, (void *)msg );
			GLogN("alloc fail hCommPKPool\r\n");
		}
		msg->mMsgType	= MSG_COMM;
		msg->mPktType	= PACKET_UART;
		msg->pPacket	= (void*)pkt;
        memcpy(pkt,ucReq,sizeof(ucReq));

		if( osMessageAvailableSpace(hParsingMsg) == 0 )
		{
			osPoolFree( hMsgPool, (void *)msg );
			osPoolFree( hCommPKPool, (void *)pkt );
		}
		else
		{
			osMessagePut( hParsingMsg, (uint32_t)msg, osWaitForever );
		}
	}
#endif
	else if( !strcmp( gCliArgs[0], "igon" ) )
	{
		bool	IgState;
		IO_CONTROL_HIGH( IG_ON_EN);
		IO_CONTROL_LOW( KL_RXD1_SEL );
		IO_CONTROL_LOW( DLCA_EN3 );
		IO_CONTROL_LOW(  KL_TXD1_INV );
        osDelay( 100 );
		IgState=HAL_GPIO_ReadPin( KL_RXD1_GPIO_Port, KL_RXD1_Pin );
		GLogN( "before KL_RXD1_Pin : %d\r\n", IgState );

		IO_CONTROL_LOW( IG_ON_EN);
		IO_CONTROL_LOW( KL_RXD1_SEL );
		IO_CONTROL_HIGH( DLCA_EN3 );
		IO_CONTROL_HIGH( KL_TXD1_INV );
		GLogN( "SET IG_ON_EN LOW,KL_RXD1_SEL HIGH,DLCA_EN3 HIGH\r\n");

		osDelay( 100 );
		IgState=HAL_GPIO_ReadPin( KL_RXD1_GPIO_Port, KL_RXD1_Pin );
		GLogN( "KL_RXD1_Pin : %d\r\n", IgState );
	}
	else if( !strcmp( gCliArgs[0], "igoff" ) )
	{
		bool	IgState;
		IO_CONTROL_HIGH( IG_ON_EN);
		IO_CONTROL_LOW( KL_RXD1_SEL );
		IO_CONTROL_LOW( DLCA_EN3 );
		IO_CONTROL_LOW(  KL_TXD1_INV );
		GLogN( "SET IG_ON_EN HIGH,KL_RXD1_SEL LOW,DLCA_EN3 LOW\r\n");

		osDelay( 100 );
		IgState=HAL_GPIO_ReadPin( KL_RXD1_GPIO_Port, KL_RXD1_Pin );
		GLogN( "KL_RXD1_Pin : %d\r\n", IgState );
	}
	else if( !strcmp( gCliArgs[0], "filecheck" ) )
	{
/*		FRESULT fsResult;

		U16 returnValue = 0;
		U32 i, iReadByte;
		U8 cBuff = 0;

        extern FIL g_TempFilepnt;
        extern uint32_t g_u32UpdateFileSize;

		if ( f_lseek(&g_TempFilepnt, 0) == FR_OK )
		{
			for(i=0; i<g_u32UpdateFileSize; i++)
			{
				fsResult = f_read(&g_TempFilepnt, (void*)&cBuff, 1, &iReadByte);
				if (  fsResult == FR_OK )
				{
					GLogN("%02X",cBuff);
					returnValue += cBuff;
				}
				else
					GLogE( "CRC Read fail\r\n");
			}
		}*/
	}
	else if( !strcmp( gCliArgs[0], "bz" ) )
	{
		if( !strcmp( gCliArgs[1], "1" ) )
		{
			Buzzer_Control( eBUZZER_DOMISOL, MSEC(200), MSEC(200), 3 );
		}
		else if( !strcmp( gCliArgs[1], "2" ) )
		{
			Buzzer_Control( eBUZZER_DOPARA, MSEC(200), MSEC(200), 3 );
		}
		else if( !strcmp( gCliArgs[1], "3" ) )
		{
			Buzzer_Control( eBUZZER_SIRESOL, MSEC(200), MSEC(200), 3 );
		}
		else if( !strcmp( gCliArgs[1], "4" ) )
		{
			Buzzer_Control( eBUZZER_ON, MSEC(100),MSEC(100), 3 );
		}
	}
	else if( !strcmp( gCliArgs[0], "logon" ) )
	{
		if( !strcmp( gCliArgs[1], "bt" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bBTLogOnTxFlag=true; GLogN("------bt tx On------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bBTLogOnRxFlag=true; GLogN("------bt rx On------\r\n"); }
		}
		else if( !strcmp( gCliArgs[1], "can" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bCanLogOnTxFlag=true; GLogN("------can tx On------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bCanLogOnRxFlag=true; GLogN("------can rx On------\r\n"); }
		}
		else if( !strcmp( gCliArgs[1], "k" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bKLogOnTxFlag=true; GLogN("------k tx On------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bKLogOnRxFlag=true; GLogN("------k rx On------\r\n"); }
		}
        else if( !strcmp( gCliArgs[1], "web" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bwebLogOnTxFlag=true; GLogN("------web tx On------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bwebLogOnRxFlag=true; GLogN("------web rx On------\r\n"); }
		}
		else if( !strcmp( gCliArgs[1], "mqtt" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bMqttLogOnTxFlag=true; GLogN("------mqtt tx On------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bMqttLogOnRxFlag=true; GLogN("------mqtt rx On------\r\n"); }
		}
		else if( !strcmp( gCliArgs[1], "enc" ) )
		{
			g_bEncryptLogOnFlag=true; GLogN("------encrypt on------\r\n");
		}
	}
#ifdef VCI3_DIAG
	else if( !strcmp( gCliArgs[0], "tcpupdate" ) )
	{
		application_tcp();
	}
#endif
	else if( !strcmp( gCliArgs[0], "pop" ) )
	{
		int32_t iStatus    = RSI_SUCCESS;
		uint8_t recv_buffer[1024]={0,};
		uint32_t uiBuffLength=0;
		uint32_t uilseek=0;
		uint32_t uiChunkCount=0;
		uint32_t uiFilesize=0;
		FIL fpVer;

		char strOpenFileName[100]={0,};

		if( !strcmp( gCliArgs[1], "1" ) )		strcpy(strOpenFileName,"/01_Application/vci3_main.bin");
		else if( !strcmp( gCliArgs[1], "2" ) )	strcpy(strOpenFileName,"/01_Application/vci3_bootloader.bin");
		else if( !strcmp( gCliArgs[1], "3" ) )	strcpy(strOpenFileName,"/01_Application/vci3_pdi.bin");
		else if( !strcmp( gCliArgs[1], "4" ) )	strcpy(strOpenFileName,"/01_Application/Repro_CAN.bin");
		else if( !strcmp( gCliArgs[1], "5" ) )	strcpy(strOpenFileName,"/01_Application/Repro_CCP.bin");
		else if( !strcmp( gCliArgs[1], "6" ) )	strcpy(strOpenFileName,"/01_Application/Repro_KWP.bin");
		else if( !strcmp( gCliArgs[1], "7" ) )	strcpy(strOpenFileName,"/01_Application/Repro_DOWN.bin");
		
		if ( f_open(&fpVer, strOpenFileName, FA_OPEN_EXISTING | FA_READ) == FR_OK )
		{
			uiFilesize = (f_size(&fpVer)/1024)+1;
			
			for( int i=0; i<uiFilesize; i++)
			{	
				if(f_lseek(&fpVer,uilseek) == FR_OK)
	            {
	            	memset(&recv_buffer[0],0x00,sizeof(uiBuffLength));
	                if ( f_read(&fpVer, recv_buffer, 1024, &uiBuffLength) == FR_OK )
	                {
	                	if( uiBuffLength != 0 )
	            		{
		            		for(int j=0;j<uiBuffLength;j++)
		            		{
	                        	GLogN("%02X",recv_buffer[j]);
		            		}
	            		}
						else
						{
							GLogN("!!!!!!!!!!!!!!!!\r\n");
						}
					}
					else
					{
						GLogN("~~~~~~~~~~~~~~\r\n");
					}
					uilseek = uilseek+1024;
				}
			}
		}
		f_close(&fpVer);
	}
	else if( !strcmp( gCliArgs[0], "update2" ) )
	{
		char cRet=0;
		cRet = ReinitRS9116();
		GLogN("cRet=%d\r\n",cRet);;
	}
	else if( !strcmp( gCliArgs[0], "update" ) )
	{
#if 0       
		char cRet=0;
		uint32_t uiOldtime=0;
		uint32_t uiTimeout=40000;
		uint32_t uiRetryCnt=0,uiRetryCntMax=5,uiRet=0;
		char strOpenFileName[]="/01_Application/TM_Bootloader.bin"; 	//2.7.0
		char strOpenFileName2[]="/01_Application/TM_Downloader.bin";	//2.8.0

		cRet = Rs9116Update(strOpenFileName);
		GLogN("Rs9116Update result=%d\r\n",cRet);

		Updatestart();

		uiOldtime = Get_Tmr();
		if( cRet == TRUE )
		{
			while(1)
			{
				if( Get_TmrDelta( Get_Tmr(), uiOldtime ) >= uiTimeout )
				{
					uiRet = ReinitRS9116();
					if( uiRet == INIT_OK )
					{
						GLogN("uiRet=%d\r\n",uiRet);
						break;
					}
					else
					{
						uiRetryCnt++;
                        uiOldtime = Get_Tmr();
                        uiTimeout+=10000;
                        GLogN("uiTimeout=%d\r\n",uiTimeout);
						if( uiRetryCnt >= uiRetryCntMax )
						{
							GLogN("uiRetryCntMax\r\n");
							break;
						}
					}
				}
				else
				{
					//GLogN("uiTimeout=%d\r\n",uiTimeout);
				}
			}
		}
#else
#ifdef VCI3_DIAG
        FL_RS9116UpdateStart(0, PACKET_UART, 0  );
#endif
#endif        
	}
	else if( !strcmp( gCliArgs[0], "fc" ))
	{
		stCommPkt	*packet;
		stMsgClst	*message;
        int ret=0;
        int testMode;

        if( !strcmp( gCliArgs[1], "1" ))    testMode = TEST_MODE_KLINE_510;
        else if( !strcmp( gCliArgs[1], "2" ))    testMode = TEST_MODE_KLINE_2K;
        else if( !strcmp( gCliArgs[1], "3" ))    testMode = TEST_MODE_KLINE_47K;
        else testMode = TEST_MODE_KLINE_47K;

		if(testMode==TEST_MODE_KLINE_510) {GLogI( "[TEST_MODE_KLINE_510]\r\n");}
		else if(testMode==TEST_MODE_KLINE_2K) {GLogI( "[TEST_MODE_KLINE_2K]\r\n");}
		else if(testMode==TEST_MODE_KLINE_47K) {GLogI( "[TEST_MODE_KLINE_47K]\r\n");}

		packet->mLen	= 3;

		ret=SelfTest_KLINE_pullup(testMode-TEST_MODE_KLINE_510);
#define	TEST_OK												0
#define	TEST_NG												1
		if( ret == 20 )
		{
			packet->mData[1] = TEST_OK;
			packet->mData[2] = 0;
            printf("success\r\n");
		}
		else
		{
			packet->mData[1] = TEST_NG;
            printf("fail %d\r\n",ret);
			switch( ret )
			{
				case 0 :		packet->mData[2] = 1;		break;	//1pin error SetKL_Line( KL_LINE1_CONNECT_CH01_CH08, KL_LINE2_CONNECT_CH08 );			break;
				case 1 :		packet->mData[2] = 2; 		break;	//2pin error SetKL_Line( KL_LINE1_CONNECT_CH02_CH08, KL_LINE2_CONNECT_CH08 );			break;
				case 2 :		packet->mData[2] = 3; 		break;	//3pin error SetKL_Line( KL_LINE1_CONNECT_CH03_CH08, KL_LINE2_CONNECT_CH08 );			break;
				case 3 :		packet->mData[2] = 6; 		break;	//6pin error SetKL_Line( KL_LINE1_CONNECT_CH06_CH08, KL_LINE2_CONNECT_CH08 );			break;
				case 4 :		packet->mData[2] = 7; 		break;	//7pin error SetKL_Line( KL_LINE1_CONNECT_CH07_CH08, KL_LINE2_CONNECT_CH08 );			break;
				case 5 :		packet->mData[2] = 13; 		break;	//13pin error SetKL_Line( KL_LINE1_CONNECT_CH13_CH08, KL_LINE2_CONNECT_CH08 );			break;
				case 6 :		packet->mData[2] = 15; 		break;	//15pin error SetKL_Line( KL_LINE1_CONNECT_CH15_CH08, KL_LINE2_CONNECT_CH08 );			break;
				case 7 :		packet->mData[2] = 9; 		break;	//9pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH09_CH08 );			break;
				case 8 :		packet->mData[2] = 10; 		break;	//10pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH10_CH08 );			break;
				case 9 :		packet->mData[2] = 11; 		break;	//11pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH11_CH08 );			break;
				case 10 :		packet->mData[2] = 12; 		break;	//12pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH12_CH08 );			break;
				case 11 :		packet->mData[2] = 14; 		break;	//14pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH14_CH08 );			break;
				case 12 :		packet->mData[2] = 15; 		break;	//15pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH15_CH08 );			break;
				default : 		packet->mData[2] = 16; 		break;	//unknown pin error
			}
		}
	}
	else if( !strcmp( gCliArgs[0], "logoff" ) )
	{
		if( !strcmp( gCliArgs[1], "bt" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bBTLogOnTxFlag=false; GLogN("------bt tx Off------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bBTLogOnRxFlag=false; GLogN("------bt rx Off------\r\n"); }
		}
		else if( !strcmp( gCliArgs[1], "can" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bCanLogOnTxFlag=false; GLogN("------can tx Off------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bCanLogOnRxFlag=false; GLogN("------can rx Off------\r\n"); }
		}
		else if( !strcmp( gCliArgs[1], "k" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bKLogOnTxFlag=false; GLogN("------k tx Off------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bKLogOnRxFlag=false; GLogN("------k rx Off------\r\n"); }
		}
        else if( !strcmp( gCliArgs[1], "web" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bwebLogOnTxFlag=false; GLogN("------web tx Off------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bwebLogOnRxFlag=false; GLogN("------web rx Off------\r\n"); }
		}
		else if( !strcmp( gCliArgs[1], "mqtt" ) )
		{
			if( !strcmp( gCliArgs[2], "tx" ) )		{ g_bMqttLogOnTxFlag=false; GLogN("------mqtt tx Off------\r\n"); }
			else if( !strcmp( gCliArgs[2], "rx" ) )	{ g_bMqttLogOnRxFlag=false; GLogN("------mqtt rx Off------\r\n"); }
		}
		else if( !strcmp( gCliArgs[1], "encrypt" ) )
		{
			g_bEncryptLogOnFlag=false; GLogN("------encrypt off------\r\n");
		}
	}
	else if( !strcmp( gCliArgs[0], "send" ) )
	{
		ePKT_TD eCommLine = PACKET_UART;
		unsigned char ucData[1000]={0,};
		unsigned short usLength=0;
		for(int i=0;i<1000;i++)
		{
			ucData[i]=i;
		}
		TransmitFunction(eCommLine, ucData, 1000, 0x1234);
	}
	else if( !strcmp( gCliArgs[0], "btssid" ) )
	{
		//! get the local device name
		status = rsi_bt_get_local_name(&local_name);
		if(status != RSI_SUCCESS)
		{
			//return status;
		}
		GLogN("local_name: %s\r\n", local_name.name);
	}
	else if( !strcmp( gCliArgs[0], "battery" ) )
	{
		if( count > 1 )
		{
			for( int i = 0; i < atoi(gCliArgs[1]); i++ )
			{
				GLogN( "Battery Voltage : %dmV\r\n", readBatteryValue() );
				osDelay( 100 );
			}
		}
		else
		{
			GLogN( "Battery Voltage : %dmV\r\n", readBatteryValue() );
		}
	}
	else if( !strcmp( gCliArgs[0], "btver" ) )
	{
		uint8_t	wlanFWVer[20] = { 0, };

		rsi_wlan_get( RSI_FW_VERSION, wlanFWVer, sizeof(wlanFWVer) );
		wlanFWVer[19]=0;
		GLogN( "RS9116FW:%s\r\n",wlanFWVer);
		//strcpy((char*)wlanFWVer, "1610.2.10.0.0.5");
		//GLogN( "RS9116FW(forced):%s\r\n",wlanFWVer);
		memset(wlanFWVer,0x00,sizeof(wlanFWVer));
		rsi_driver_version(wlanFWVer);
		GLogN( "RS9116SDK:%s\r\n",wlanFWVer);
	}
	else if( !strcmp( gCliArgs[0], "wstatus" ) )
	{
		uint8_t wlanstatus = 0;
		tt_start = rsi_hal_gettickcount();
		rsi_wlan_get( RSI_CONNECTION_STATUS, &wlanstatus, sizeof(wlanstatus) );
		tt_end = rsi_hal_gettickcount();
		GLogI("tt_start : %d tt_end : %d   %d \r\n", tt_start, tt_end, tt_end- tt_start);
		GLogN("WLAN_status : %d\r\n", wlanstatus);
		
	}
	else if( !strcmp( gCliArgs[0], "dns" ) )
	{
		rsi_rsp_dns_query_t dns_response;
		uint8_t *url_name = (uint8_t *)"echo.websocket.org";
		
		status = RequestDNS(url_name, &dns_response);
		if(status == RSI_SUCCESS)
		{
			GLogN("IP : %d.%d.%d.%d", dns_response.ip_address[0].ipv4_address[0], dns_response.ip_address[0].ipv4_address[1], dns_response.ip_address[0].ipv4_address[2], dns_response.ip_address[0].ipv4_address[3]);
		}
	}
	else if( !strcmp( gCliArgs[0], "info_size" ) )
	{
		GLogN("Firmware Info Size : %dByte", sizeof(SFwInfo));
	}
	else if( !strcmp( gCliArgs[0], "emmc" ) )
	{
		/***********************************************/
		/*   Write                                     */
		/***********************************************/
		// 1. Open
	 	res = f_open( &testFile, TEST_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE );
	 	if( res != FR_OK )
	 	{
	 		GLogE( "Error... fail open file!!!\r\n" );
			//packet->mData[1]=TEST_NG;//0:OK,  1:NG
			//break;
		}

		osDelay( 100 );

		// 2. Write
		res = f_write( &testFile, wtext, sizeof( wtext ), (void *)&byteswrite );
		if( byteswrite == 0 || res != FR_OK )
	 	{
	 		GLogE( "Error... fail write file!!!\r\n" );
			//packet->mData[1]=TEST_NG;//0:OK,  1:NG
			//break;
		}

		osDelay( 100 );

		// 3. Close
		res = f_close( &testFile );
		if( res != FR_OK )
		{
			GLogE( "Error... fail close file!!!\r\n" );
			//packet->mData[1]=TEST_NG;//0:OK,  1:NG
			//break;
		}

		osDelay( 100 );

		/***********************************************/
		/*   Read                                      */
		/***********************************************/
		// 1. Open
		res = f_open( &testFile, TEST_FILE_NAME, FA_READ );
		if( res != FR_OK )
		{
			GLogE( "Error... fail open file!!!\r\n" );
			//packet->mData[1]=TEST_NG;//0:OK,  1:NG
			//break;
		}

		osDelay( 100 );

		// 2. Read
		res = f_read( &testFile, rtext, sizeof( rtext ), (void *)&bytesread );
		GLogI( "eMMC WRITE DATA : VCI3 eMMC Test!!!\r\n" );
		GLogI( "eMMC READ  DATA : " );
		for( i = 0; i < bytesread; i++ )
		{
			GLogI( "%c",rtext[i] );
		}
		GLogI( "\r\n" );
		if( bytesread == 0 || res != FR_OK )
		{
			GLogE( "Error... fail read file!!!\r\n" );
			//packet->mData[1]=TEST_NG;//0:OK,  1:NG
			//break;
		}

		osDelay( 100 );

		// 3. Close
		res = f_close( &testFile );
		if( res != FR_OK )
		{
			GLogE( "Error... fail close file!!!\r\n" );
			//packet->mData[1]=TEST_NG;//0:OK,  1:NG
			//break;
		}

		osDelay( 100 );

		/***********************************************/
		/*   Erase                                     */
		/***********************************************/
		res = f_unlink( TEST_FILE_NAME );
		if( res != FR_OK )
		{
			GLogE( "Error... fail remove file!!!\r\n" );
			//packet->mData[1]=TEST_NG;//0:OK,  1:NG
			//break;
		}

		osDelay( 100 );

		/***********************************************/
		/*   Check                                     */
		/***********************************************/
		if( strcmp( (const char*)wtext, (const char*)rtext ) == 0 )
		{
			GLogI( "Test OK!!!\r\n" );
		}
		else
		{
			GLogE( "Test NG!!!\r\n" );
			//packet->mData[1]=TEST_NG;//0:OK,  1:NG
		}

		osDelay( 3000 );
	}
	else if( !strcmp( gCliArgs[0], "igon" ) )
	{
		InitIOCTL();

		setWakeUpSource( WAKEUP_SOURCE_IG );

		SetWakeUpPin_Input();//for wake pin state read

		IO_CONTROL_HIGH( IG_ON_EN);
		GLogN( "IG_ON_EN_PIN HIGH\r\n");
		osDelay( 100 );
		GLogN( "WAK_UP_Pin & CH3 state : %d\r\n", HAL_GPIO_ReadPin( WAK_UP_GPIO_Port, WAK_UP_Pin ) );

		IO_CONTROL_LOW( IG_ON_EN);
		GLogN( "IG_ON_EN_PIN LOW\r\n");
		osDelay( 100 );
		GLogN( "WAK_UP_Pin & CH3 state : %d\r\n", HAL_GPIO_ReadPin( WAK_UP_GPIO_Port, WAK_UP_Pin ) );

		IO_CONTROL_HIGH( IG_ON_EN);
		GLogN( "IG_ON_EN_PIN HIGH\r\n");
		osDelay( 100 );
		GLogN( "WAK_UP_Pin & CH3 state : %d\r\n", HAL_GPIO_ReadPin( WAK_UP_GPIO_Port, WAK_UP_Pin ) );

		IO_CONTROL_LOW( IG_ON_EN);
		GLogN( "IG_ON_EN_PIN LOW\r\n");
		osDelay( 100 );
		GLogN( "WAK_UP_Pin & CH3 state : %d\r\n", HAL_GPIO_ReadPin( WAK_UP_GPIO_Port, WAK_UP_Pin ) );

		setWakeUpSource( 0 );
		SetWakeUpPin_AF();//wake pin ������ ����ũ�� �뵵�� ����ϱ� ���� ���� ����

  		//gotoStandbyMode( WAKEUP_SOURCE_SENSOR );

	}
	else if( !strcmp( gCliArgs[0], "batteryrepro" ) )
	{
		if( count > 1 )
		{
			for( int i = 0; i < atoi(gCliArgs[1]); i++ )
			{
				GLogN( "Repro Voltage : %dmV\r\n", readReprogramVoltageValue() );
				osDelay( 100 );
			}
		}
		else
		{
			GLogN( "Repro Voltage : %dmV\r\n", readReprogramVoltageValue() );
		}
	}
	else if( !strcmp( gCliArgs[0], "build" ) )
	{
		GLogN( "Build Date is %s\r\n", __DATE__ );
	}
	else if( !strcmp( gCliArgs[0], "clk" ) )
	{
		printMainCLKs();
		printPeriCLKs();
	}
	else if( !strcmp( gCliArgs[0], "Event1" ) )
	{
		xEventGroupSetBits(hEventGroup, EVENT_BIT_WS);
	}
	else if( !strcmp( gCliArgs[0], "Event2" ) )
	{
		xEventGroupSetBits(hEventGroup, EVENT_BIT_MQTT);
	}
	else if( !strcmp( gCliArgs[0], "dir" ) )
	{
		
		switch (count)
		{
		case 1:
		  directory_list();
		  break;
		case 2:
		  
		  //memset(FilePath, '\0', sizeof(FilePath));
		  memcpy(Path, &gCliArgs[1], sizeof(Path));
		  directory_listWithPath(Path);
		  break;
		}
	}
//	else if( !strcmp( gCliArgs[0], "read" ) )
//	{
//        char strPath[100]={0,};
//		if( !strcmp( gCliArgs[1], "A" ) )
//        {
//            strcpy(strPath,DIR_ROOT);
//            strcat(strPath,DIR_APP);
//        }
//        else if( !strcmp( gCliArgs[1], "B" ) )
//        {
//            strcpy(strPath,DIR_ROOT);
//            strcat(strPath,EMMC_BACKUP_FOLDER);
//        }
//        else if( !strcmp( gCliArgs[1], "R" ) )
//        {
//            strcpy(strPath,DIR_ROOT);
//            strcat(strPath,DIR_RECORD);
//        }
//		else
//		{
//            strcpy(strPath,DIR_ROOT);
//			f_chdir( gCliArgs[1] );
//            strcat(strPath,gCliArgs[1]);
//		}
//		scan_files(strPath);
//	}
	else if( !strcmp( gCliArgs[0], "lastpacket" ) )
	{
		int jj=0;
		GLogN("g_SaveLastRxPacket : \r\n");
		for(jj=0; jj<RSI_BT_MAX_PAYLOAD_SIZE;jj++)
		{
			GLogN("%02X ",g_SaveLastRxPacket[jj]);
		}
		GLogN("\r\n");

		GLogN("g_SaveLastTxPacket : \r\n");
		for(jj=0; jj<RSI_BT_MAX_PAYLOAD_SIZE;jj++)
		{
			GLogN("%02X ",g_SaveLastTxPacket[jj]);
		}
		GLogN("\r\n");
	}
	else if( !strcmp( gCliArgs[0], "del" ) )
	{
		f_unlink( gCliArgs[1] );
	}
	else if( !strcmp( gCliArgs[0], "version" ) )
	{
		switch( count )
		{
			case 1 :
			{
				printFWVersion();
			}
			break;

			case 2 :
			{
				dlcToVersion( gsFwInfo.msAppInfo[eApp_bootloader].marrucVersion, atoi( gCliArgs[1] ) );

				saveFirmwareInfo_EMMC(true);

				printFWVersion();
			}
			break;

			case 3 :
			{
//				dlcToVersion( gsFwInfo.msBlInfo.marrucVersion, atoi( gCliArgs[1] ) );
//				dlcToVersion( gsFwInfo.msMainAppInfo.marrucVersion, atoi( gCliArgs[2] ) );
				gsFwInfo.mucChanged = TRUE;
				saveFirmwareInfo_EMMC(true);

				printFWVersion();
			}
			break;

			default :
			{
			}
			break;
		}
	}
	else if( !strcmp( gCliArgs[0], "setver" ) )
	{
		if( count == 1 )
		{
			if( loadFirmwareInfo_EMMC() == 0 )
			{
				GLogN( "[EMMC] F/W version read OK\r\n" );
				printFWVersion();
			}
			else
			{
				GLogN( "[EMMC] Fail to load firmware info\r\n" );
			}
		}
		else if( count == 4 )
		{
			uint8_t ucMajor = (uint8_t)atoi( gCliArgs[2] );
			uint8_t ucMinor = (uint8_t)atoi( gCliArgs[3] );
			bool bValid = true;

			if( !strcmp( gCliArgs[1], "total" ) )
			{
				gsFwInfo.marrucTotalVersion[0] = ucMajor;
				gsFwInfo.marrucTotalVersion[1] = ucMinor;
				GLogN( "Set total version -> %02d.%02d\r\n", ucMajor, ucMinor );
			}
			else if( !strcmp( gCliArgs[1], "boot" ) )
			{
				gsFwInfo.msAppInfo[eApp_bootloader].marrucVersion[0] = ucMajor;
				gsFwInfo.msAppInfo[eApp_bootloader].marrucVersion[1] = ucMinor;
				GLogN( "Set bootloader version -> %02d.%02d\r\n", ucMajor, ucMinor );
			}
			else
			{
				int32_t iAppNo = atoi( gCliArgs[1] );
				if( iAppNo < 0 || iAppNo >= (int32_t)eApp_MAX )
				{
					GLogN( "Invalid app index: %d (valid: 0~%d)\r\n", iAppNo, eApp_MAX - 1 );
					bValid = false;
				}
				else
				{
					gsFwInfo.msAppInfo[iAppNo].marrucVersion[0] = ucMajor;
					gsFwInfo.msAppInfo[iAppNo].marrucVersion[1] = ucMinor;
					GLogN( "Set app[%d] version -> %02d.%02d\r\n", iAppNo, ucMajor, ucMinor );
				}
			}

			if( bValid )
			{
				gsFwInfo.mucChanged = TRUE;
				saveFirmwareInfo_EMMC( true );
				UpdateAppSwListVer( gCliArgs[1], ucMajor, ucMinor );
				printFWVersion();
			}
		}
		else
		{
			GLogN( "Usage:\r\n" );
			GLogN( "  setver                           - read version from EMMC\r\n" );
			GLogN( "  setver <app_no|boot|total> <major> <minor> - set version\r\n" );
		}
	}
#ifdef FW_TEST_VERSION_OVERRIDE
	else if( !strcmp( gCliArgs[0], "hsmtestver" ) )
	{
		if( count == 1 )
		{
			uint16_t uiVer = Load_HSM_TestVersion();
			GLogN( "HSM Test Version: %u\r\n", uiVer );
		}
		else if( count == 2 )
		{
			uint16_t uiVer = (uint16_t)atoi( gCliArgs[1] );
			if( Save_HSM_TestVersion( uiVer ) == HAL_OK )
			{
				GLogN( "HSM Test Version set to: %u\r\n", uiVer );
			}
			else
			{
				GLogN( "HSM Test Version save FAIL\r\n" );
			}
		}
		else
		{
			GLogN( "Usage:\r\n" );
			GLogN( "  hsmtestver         - read HSM test version\r\n" );
			GLogN( "  hsmtestver <ver>   - set HSM test version (e.g. 1001)\r\n" );
		}
	}
	else if( !strcmp( gCliArgs[0], "wlantestver" ) )
	{
		if( count == 1 )
		{
			uint8_t verBuf[RS9116_TEST_VERSION_MAXLEN] = { 0, };
			Load_RS9116_TestVersion( verBuf, sizeof(verBuf) );
			GLogN( "WLAN Test Version: %s\r\n", verBuf );
		}
		else if( count == 2 )
		{
			const char *pVer = gCliArgs[1];

			/* Shortcut: "1" -> default version (1610.2.10.0.0.5) */
			if( !strcmp( pVer, "1" ) )
			{
				pVer = RS9116_TEST_VERSION_DEFAULT;
			}

			if( Save_RS9116_TestVersion( pVer ) == 0 )
			{
				GLogN( "WLAN Test Version set to: %s\r\n", pVer );
			}
			else
			{
				GLogN( "WLAN Test Version save FAIL\r\n" );
			}
		}
		else
		{
			GLogN( "Usage:\r\n" );
			GLogN( "  wlantestver           - read WLAN test version\r\n" );
			GLogN( "  wlantestver 1         - set to default (%s)\r\n", RS9116_TEST_VERSION_DEFAULT );
			GLogN( "  wlantestver <ver_str> - set WLAN test version\r\n" );
			GLogN( "  ex) wlantestver 1610.2.10.0.0.5\r\n" );
		}
	}
#endif /* FW_TEST_VERSION_OVERRIDE */
	else if( !strcmp( gCliArgs[0], "serial" ) )
	{
		switch (count)
		{
		case 1:
		  printSerialNumber();
		  break;
		case 2:
		  gsFwInfo.mucChanged = TRUE;
		  memset(gsFwInfo.marrucSerialNo, 0x00, sizeof(gsFwInfo.marrucSerialNo));
		  memcpy(gsFwInfo.marrucSerialNo, &gCliArgs[1], sizeof(gsFwInfo.marrucSerialNo));
		  saveFirmwareInfo_EMMC(true);
		  printSerialNumber();
		  break;
		}
	}
	else if( !strcmp( gCliArgs[0], "mq_info" ) )
	{
		gsFwInfo.mucChanged = TRUE;
		memset(msServerConnectInfo.mqtt_domain, 0x00, sizeof(msServerConnectInfo.mqtt_domain));

		uint8_t type = strtoul(gCliArgs[1], NULL, 10);

		if(type == 1)  // IP mode
		{
			msServerConnectInfo.mqtt_domain[126] = 1;
			// Parse IP address format: 192.168.0.1
			uint8_t ip_octets[4] = {0};
			char *ip_str = gCliArgs[2];
			char *token = strtok(ip_str, ".");
			int i = 0;

			while(token != NULL && i < 4)
			{
				ip_octets[i] = (uint8_t)strtoul(token, NULL, 10);
				token = strtok(NULL, ".");
				i++;
			}

			msServerConnectInfo.mqtt_domain[0] = ip_octets[0];
			msServerConnectInfo.mqtt_domain[1] = ip_octets[1];
			msServerConnectInfo.mqtt_domain[2] = ip_octets[2];
			msServerConnectInfo.mqtt_domain[3] = ip_octets[3];

			msServerConnectInfo.mqtt_port = strtoul(gCliArgs[3], NULL, 10);

			GLogN("IP Mode : %d.%d.%d.%d\r\n",
				msServerConnectInfo.mqtt_domain[0],
				msServerConnectInfo.mqtt_domain[1],
				msServerConnectInfo.mqtt_domain[2],
				msServerConnectInfo.mqtt_domain[3]);
			GLogN("port : %d\r\n", msServerConnectInfo.mqtt_port);
		}
		else if(type == 0)  // Domain mode
		{
			msServerConnectInfo.mqtt_domain[126] = 0;
			memcpy(msServerConnectInfo.mqtt_domain, gCliArgs[2], strlen(gCliArgs[2]));
			msServerConnectInfo.mqtt_port = strtoul(gCliArgs[3], NULL, 10);
			GLogN("Domain : %s\r\n", msServerConnectInfo.mqtt_domain);
			GLogN("port : %d\r\n", msServerConnectInfo.mqtt_port);
		}

		Save_ServerInfo_EMMC();
	}
	else if( !strcmp( gCliArgs[0], "del_serverinfo" ) )
	{
		Delete_ServerInfo_EMMC();
	}
	else if( !strcmp( gCliArgs[0], "hsmserial" ) )
    {
        uint8_t serial_number[8];
        HAL_StatusTypeDef status;

        GLogN("\r\n========== HSM Serial Number ==========\r\n");

        status = hsm_get_serial_number(serial_number);

        if (status == HAL_OK)
        {
            GLogN("HSM Serial Number: ");
            for (int i = 0; i < 8; i++)
            {
                GLogN("%02X ", serial_number[i]);
            }
            GLogN("\r\n");
            GLogN("========================================\r\n");
        }
        else
        {
            GLogE("Failed to get HSM serial number! Status: %d\r\n", status);
        }
    }
#if 1//for test = 1
	else if( !strcmp( gCliArgs[0], "hsmtest" ) )
    {
        if (count < 2)
        {
            GLogN("Usage: hsmtest <opcode_hex> [params...]\r\n");
            GLogN("\r\n");
            GLogN("=== System Info (OP 0x00, 0x01, 0x02) ===\r\n");
            GLogN("  0x00          HSM Info (Version, Lot, Serial)\r\n");
            GLogN("  0x01          Get HSN (8-byte UID)\r\n");
            GLogN("  0x02          Cancel Job\r\n");
            GLogN("  0xCC          W_CC Clear (ERROR→READY 상태 초기화)\r\n");
            GLogN("  0x0B          Get Last Error Code\r\n");
            GLogN("\r\n");
            GLogN("=== Key Generate (OP 0x05) ===\r\n");
            GLogN("  0x05          RSA-2048 KeyPair Gen + Export (#150)\r\n");
            GLogN("  0x52 [id][sz] AES Key Gen (sz:1=128,2=192,3=256)\r\n");
            GLogN("  0x53 [id]     ECC P-256 Key Generate\r\n");
            GLogN("  0x54 [id]     Ed25519 Key Generate\r\n");
            GLogN("\r\n");
            GLogN("=== Key Import (OP 0x04, 0x0115) ===\r\n");
            GLogN("  0x51 [id]     Import Encrypted Key (default=#101)\r\n");
            GLogN("  0x55 [id][sz] AES Plaintext Import (test key)\r\n");
            GLogN("  0x62          KEK RSA PubKey Import (#149)\r\n");
            GLogN("  0x63          KM Master Key Import (#102) - HMAC\r\n");
            GLogN("  0x64          KM Master Key Import (#102) - AES-128 (NIST test)\r\n");
            GLogN("  0x65          KM Master Key Import (#102) - Production Key\r\n");
            GLogN("  0x66          KM Master Key Import (#102) - AES-256 (NIST test)\r\n");
            GLogN("  0x73          Kauth Key Import (#103)\r\n");
            GLogN("\r\n");
            GLogN("=== Key Export (OP 0x0124) ===\r\n");
            GLogN("  0x56 [id]     ECC Public Key Export\r\n");
            GLogN("  0x57 [id]     ED Public Key Export\r\n");
            GLogN("  0xA8 [id]     RSA Public Key Export (#149)\r\n");
            GLogN("\r\n");
            GLogN("=== Key Exchange/Derivation (OP 0x08, 0x17) ===\r\n");
            GLogN("  0x07 [id]     ECDH Key Exchange\r\n");
            GLogN("  0x16 [src]    HKDF Key Derivation\r\n");
            GLogN("  0xA9          RSA Decrypt + AES Import (2-step)\r\n");
            GLogN("  0xAA          1-Step Encrypted AES Import (production)\r\n");
            GLogN("\r\n");
            GLogN("=== Hash/Random (OP 0x10, 0x11) ===\r\n");
            GLogN("  0x10          Random (TRNG 32 bytes)\r\n");
            GLogN("  0x11          SHA-256 Hash Test\r\n");
            GLogN("\r\n");
            GLogN("=== AES Encrypt/Decrypt (OP 0x12) ===\r\n");
            GLogN("  0x20 [id][sz] AES-ECB Encrypt\r\n");
            GLogN("  0x21 [id][sz] AES-ECB Decrypt\r\n");
            GLogN("  0x22 [id][sz] AES-CBC Encrypt\r\n");
            GLogN("  0x23 [id][sz] AES-CBC Decrypt\r\n");
            GLogN("\r\n");
            GLogN("=== RSA Encrypt/Decrypt (OP 0x15) ===\r\n");
            GLogN("  0x36 [id]     RSA Encrypt (PKCS1-v1_5)\r\n");
            GLogN("  0x37 [id]     RSA Decrypt (PKCS1-v1_5)\r\n");
            GLogN("\r\n");
            GLogN("=== MAC (OP 0x13) ===\r\n");
            GLogN("  0x13 [id]     HMAC-SHA256\r\n");
            GLogN("  0x14 [id][sz] CMAC-AES\r\n");
            GLogN("\r\n");
            GLogN("=== Sign/Verify (OP 0x14) ===\r\n");
            GLogN("  0x30 [id]     ECDSA Sign (P-256)\r\n");
            GLogN("  0x31 [id]     ECDSA Verify\r\n");
            GLogN("  0x32 [id]     EdDSA Sign (Ed25519)\r\n");
            GLogN("  0x33 [id]     EdDSA Verify\r\n");
            GLogN("  0x34 [id]     RSA Sign (SHA256)\r\n");
            GLogN("  0x35 [id]     RSA Verify\r\n");
            GLogN("\r\n");
            GLogN("=== Certificate/CRL (OP 0x06) ===\r\n");
            GLogN("  0x41 [cert]   Store Certificate\r\n");
            GLogN("  0x42 [cert]   Read Certificate\r\n");
            GLogN("\r\n");
            GLogN("=== Authentication (OP 0x20, 0x21, 0x22) ===\r\n");
            GLogN("  0x60 [master] ASK Generate Key (0~2)\r\n");
            GLogN("  0x70 [mode]   Mutual Auth (0=Km, 1=Kauth) - AES-128\r\n");
            GLogN("  0x71 [mode]   Mutual Auth (0=Km, 1=Kauth) - AES-256\r\n");
            GLogN("  0x74          One-way Authentication\r\n");
            GLogN("\r\n");
            GLogN("=== System (OP 0x90, 0x91) ===\r\n");
            GLogN("  0x90 [type]   DFU (1=HSE_FW, 2=APP_FW)\r\n");
            GLogN("  0x91 CONFIRM  Set Complete (Lifecycle)\r\n");
            GLogN("\r\n");
            GLogN("=== ECU-Code Key Verification ===\r\n");
            GLogN("  0xA3 [id]     ECU Code Key Plaintext Import (#101)\r\n");
            GLogN("  0xA4 [id]     Import Real ECU-Code (VCI3!Hsm@Aes#Key)\r\n");
            GLogN("  0xA5 [id]     AES-CTR Decrypt Test\r\n");
            GLogN("  0xA6          Full ECU-Code Verification (A4+A5)\r\n");
            GLogN("  0xA7 [id]     AES-ECB Round-trip Test\r\n");
            GLogN("\r\n");
            GLogN("=== Hardcoded Key Tests ===\r\n");
            GLogN("  0xA0 [id]     RSA-2048 PubKey Import (#149)\r\n");
            GLogN("  0xA1 [id]     ED25519 Key Placeholder (#261)\r\n");
            GLogN("  0xA2 [id]     HMAC Key Generate (#121)\r\n");
            GLogN("\r\n");
            GLogN("=== RSA Decrypt Flow (Step-by-Step) ===\r\n");
            GLogN("  0xE1          [1/3] RSA KeyPair Gen + Export PubKey\r\n");
            GLogN("  0xE2 <hex>    [2/3] RSA Decrypt (user ciphertext)\r\n");
            GLogN("  0xE3          [3/3] RSA Decrypt (hardcoded test)\r\n");
            GLogN("\r\n");
            GLogN("=== FL_Git Function Tests (Old/New HSM) ===\r\n");
            GLogN("  0xF0          HSM GetVersion\r\n");
            GLogN("  0xF1          HSM State Check\r\n");
            GLogN("  0xF2 [crl]    CRL GetDate (0~7)\r\n");
            GLogN("  0xF3 [cert]   CRT GetHolderRef (0~6)\r\n");
            GLogN("  0xF4          RSA KeyPair Gen (FL_Git)\r\n");
            GLogN("  0xF5          ECU Code Key Check\r\n");
            GLogN("  0xF6          Store Encrypt Key\r\n");
            GLogN("  0xF7 [id][seed] ASK v2 Gen (id:0~2, seed:1~10)\r\n");
            GLogN("  0xF8 [crl]    CRL Store/Restore (0~7)\r\n");
            GLogN("  0xF9 [cert]   Certificate Full Read (0~6)\r\n");
            GLogN("\r\n");
            GLogN("=== Real Data Store (CSAC) ===\r\n");
            GLogN("  0xFA [cert][key] Store Certificate (1~7, key:141~148)\r\n");
            GLogN("  0xFB [key]    Store Private Key (Old HSM only)\r\n");
            GLogN("  0xFC [crl]    Store CRL (1~8)\r\n");
            GLogN("\r\n");
            GLogN("=== CSAC RSA Sign Tests (New HSM only) ===\r\n");
            GLogN("  0xFD [key]    RSA Sign SHA1 (#141~148, default=141)\r\n");
            GLogN("  0xFE [key]    RSA Sign SHA256 (#141~148, default=142)\r\n");
            GLogN("  0xD0 [mode][crt][crl] CSAC Full Flow\r\n");
            GLogN("                mode:0=CSAC1.0, 1=CSAC2.0\r\n");
            GLogN("                crt:1~7, crl:0~7\r\n");
            GLogN("\r\n");
            GLogN("=== RSA Compatibility Tests (New HSM only) ===\r\n");
            GLogN("  0x100 [slot]  RSA Sign SHA1+SEED_PAD (#141, Old HSM compat)\r\n");
            GLogN("  0x101 [slot]  RSA Sign SHA256+PKCS1 (#142)\r\n");
            GLogN("  0x102 [slot]  RSA Private Key Import (#141, SHA1)\r\n");
            GLogN("  0x103 [slot]  RSA Private Key Import (#142, SHA256)\r\n");
            GLogN("  0x104 [slot]  RSA Private Key Import (hsm_import_rsa_key)\r\n");
            GLogN("\r\n");
            GLogN("=== ED25519 / HMAC Tests (New HSM only) ===\r\n");
            GLogN("  0x105 [slot][mode] ED25519 Key Import (#261, mode:0=pair,1=pub)\r\n");
            GLogN("  0x106 [slot]  ED25519 Sign Test (Seed->Signature)\r\n");
            GLogN("  0x107 [slot]  HMAC Key Import (#121, 32 bytes)\r\n");
            GLogN("  0x108 [slot]  HMAC_SHA256 Test (Seed+Secret->MAC)\r\n");
            GLogN("\r\n");
            GLogN("=== Additional Tests ===\r\n");
            GLogN("  0x111 [id]    RSA Public Key Export (hsm_export_public_key)\r\n");
            GLogN("  0x112 [mode]  Kauth Import via RSA Encrypt (OP 0x22)\r\n");
            return 0;
        }

        // Help command support
        if (!strcmp(gCliArgs[1], "help") || !strcmp(gCliArgs[1], "?"))
        {
            GLogN("Usage: hsmtest <opcode_hex> [params...]\r\n");
            GLogN("\r\n");
            GLogN("=== System Info (OP 0x00, 0x01, 0x02) ===\r\n");
            GLogN("  0x00          HSM Info (Version, Lot, Serial)\r\n");
            GLogN("  0x01          Get HSN (8-byte UID)\r\n");
            GLogN("  0x02          Cancel Job\r\n");
            GLogN("  0xCC          W_CC Clear (ERROR→READY 상태 초기화)\r\n");
            GLogN("  0x0B          Get Last Error Code\r\n");
            GLogN("\r\n");
            GLogN("=== Key Generate (OP 0x05) ===\r\n");
            GLogN("  0x05          RSA-2048 KeyPair Gen + Export (#150)\r\n");
            GLogN("  0x52 [id][sz] AES Key Gen (sz:1=128,2=192,3=256)\r\n");
            GLogN("  0x53 [id]     ECC P-256 Key Generate\r\n");
            GLogN("  0x54 [id]     Ed25519 Key Generate\r\n");
            GLogN("\r\n");
            GLogN("=== Key Import (OP 0x04, 0x0115) ===\r\n");
            GLogN("  0x51 [id]     Import Encrypted Key (default=#101)\r\n");
            GLogN("  0x55 [id][sz] AES Plaintext Import (test key)\r\n");
            GLogN("  0x62          KEK RSA PubKey Import (#149)\r\n");
            GLogN("  0x63          KM Master Key Import (#102) - HMAC\r\n");
            GLogN("  0x64          KM Master Key Import (#102) - AES-128 (NIST test)\r\n");
            GLogN("  0x65          KM Master Key Import (#102) - Production Key\r\n");
            GLogN("  0x66          KM Master Key Import (#102) - AES-256 (NIST test)\r\n");
            GLogN("  0x73          Kauth Key Import (#103)\r\n");
            GLogN("\r\n");
            GLogN("=== Key Export (OP 0x0124) ===\r\n");
            GLogN("  0x56 [id]     ECC Public Key Export\r\n");
            GLogN("  0x57 [id]     ED Public Key Export\r\n");
            GLogN("  0xA8 [id]     RSA Public Key Export (#149)\r\n");
            GLogN("\r\n");
            GLogN("=== Key Exchange/Derivation (OP 0x08, 0x17) ===\r\n");
            GLogN("  0x07 [id]     ECDH Key Exchange\r\n");
            GLogN("  0x16 [src]    HKDF Key Derivation\r\n");
            GLogN("  0xA9          RSA Decrypt + AES Import (2-step)\r\n");
            GLogN("  0xAA          1-Step Encrypted AES Import (production)\r\n");
            GLogN("\r\n");
            GLogN("=== Hash/Random (OP 0x10, 0x11) ===\r\n");
            GLogN("  0x10          Random (TRNG 32 bytes)\r\n");
            GLogN("  0x11          SHA-256 Hash Test\r\n");
            GLogN("\r\n");
            GLogN("=== AES Encrypt/Decrypt (OP 0x12) ===\r\n");
            GLogN("  0x20 [id][sz] AES-ECB Encrypt\r\n");
            GLogN("  0x21 [id][sz] AES-ECB Decrypt\r\n");
            GLogN("  0x22 [id][sz] AES-CBC Encrypt\r\n");
            GLogN("  0x23 [id][sz] AES-CBC Decrypt\r\n");
            GLogN("\r\n");
            GLogN("=== RSA Encrypt/Decrypt (OP 0x15) ===\r\n");
            GLogN("  0x36 [id]     RSA Encrypt (PKCS1-v1_5)\r\n");
            GLogN("  0x37 [id]     RSA Decrypt (PKCS1-v1_5)\r\n");
            GLogN("\r\n");
            GLogN("=== MAC (OP 0x13) ===\r\n");
            GLogN("  0x13 [id]     HMAC-SHA256\r\n");
            GLogN("  0x14 [id][sz] CMAC-AES\r\n");
            GLogN("\r\n");
            GLogN("=== Sign/Verify (OP 0x14) ===\r\n");
            GLogN("  0x30 [id]     ECDSA Sign (P-256)\r\n");
            GLogN("  0x31 [id]     ECDSA Verify\r\n");
            GLogN("  0x32 [id]     EdDSA Sign (Ed25519)\r\n");
            GLogN("  0x33 [id]     EdDSA Verify\r\n");
            GLogN("  0x34 [id]     RSA Sign (SHA256)\r\n");
            GLogN("  0x35 [id]     RSA Verify\r\n");
            GLogN("\r\n");
            GLogN("=== Certificate/CRL (OP 0x06) ===\r\n");
            GLogN("  0x41 [cert]   Store Certificate\r\n");
            GLogN("  0x42 [cert]   Read Certificate\r\n");
            GLogN("\r\n");
            GLogN("=== Authentication (OP 0x20, 0x21, 0x22) ===\r\n");
            GLogN("  0x60 [master] ASK Generate Key (0~2)\r\n");
            GLogN("  0x70 [mode]   Mutual Auth (0=Km, 1=Kauth) - AES-128\r\n");
            GLogN("  0x71 [mode]   Mutual Auth (0=Km, 1=Kauth) - AES-256\r\n");
            GLogN("  0x74          One-way Authentication\r\n");
            GLogN("\r\n");
            GLogN("=== System (OP 0x90, 0x91) ===\r\n");
            GLogN("  0x90 [type]   DFU (1=HSE_FW, 2=APP_FW)\r\n");
            GLogN("  0x91 CONFIRM  Set Complete (Lifecycle)\r\n");
            GLogN("\r\n");
            GLogN("=== ECU-Code Key Verification ===\r\n");
            GLogN("  0xA3 [id]     ECU Code Key Plaintext Import (#101)\r\n");
            GLogN("  0xA4 [id]     Import Real ECU-Code (VCI3!Hsm@Aes#Key)\r\n");
            GLogN("  0xA5 [id]     AES-CTR Decrypt Test\r\n");
            GLogN("  0xA6          Full ECU-Code Verification (A4+A5)\r\n");
            GLogN("  0xA7 [id]     AES-ECB Round-trip Test\r\n");
            GLogN("\r\n");
            GLogN("=== Hardcoded Key Tests ===\r\n");
            GLogN("  0xA0 [id]     RSA-2048 PubKey Import (#149)\r\n");
            GLogN("  0xA1 [id]     ED25519 Key Placeholder (#261)\r\n");
            GLogN("  0xA2 [id]     HMAC Key Generate (#121)\r\n");
            GLogN("\r\n");
            GLogN("=== RSA Decrypt Flow (Step-by-Step) ===\r\n");
            GLogN("  0xE1          [1/3] RSA KeyPair Gen + Export PubKey\r\n");
            GLogN("  0xE2 <hex>    [2/3] RSA Decrypt (user ciphertext)\r\n");
            GLogN("  0xE3          [3/3] RSA Decrypt (hardcoded test)\r\n");
            GLogN("\r\n");
            GLogN("=== FL_Git Function Tests (Old/New HSM) ===\r\n");
            GLogN("  0xF0          HSM GetVersion\r\n");
            GLogN("  0xF1          HSM State Check\r\n");
            GLogN("  0xF2 [crl]    CRL GetDate (0~7)\r\n");
            GLogN("  0xF3 [cert]   CRT GetHolderRef (0~6)\r\n");
            GLogN("  0xF4          RSA KeyPair Gen (FL_Git)\r\n");
            GLogN("  0xF5          ECU Code Key Check\r\n");
            GLogN("  0xF6          Store Encrypt Key\r\n");
            GLogN("  0xF7 [id][seed] ASK v2 Gen (id:0~2, seed:1~10)\r\n");
            GLogN("  0xF8 [crl]    CRL Store/Restore (0~7)\r\n");
            GLogN("  0xF9 [cert]   Certificate Full Read (0~6)\r\n");
            GLogN("\r\n");
            GLogN("=== Real Data Store (CSAC) ===\r\n");
            GLogN("  0xFA [cert][key] Store Certificate (1~7, key:141~148)\r\n");
            GLogN("  0xFB [key]    Store Private Key (Old HSM only)\r\n");
            GLogN("  0xFC [crl]    Store CRL (1~8)\r\n");
            GLogN("\r\n");
            GLogN("=== CSAC RSA Sign Tests (New HSM only) ===\r\n");
            GLogN("  0xFD [key]    RSA Sign SHA1 (#141~148, default=141)\r\n");
            GLogN("  0xFE [key]    RSA Sign SHA256 (#141~148, default=142)\r\n");
            GLogN("  0xD0 [mode][crt][crl] CSAC Full Flow\r\n");
            GLogN("                mode:0=CSAC1.0, 1=CSAC2.0\r\n");
            GLogN("                crt:1~7, crl:0~7\r\n");
            GLogN("\r\n");
            GLogN("=== RSA Compatibility Tests (New HSM only) ===\r\n");
            GLogN("  0x100 [slot]  RSA Sign SHA1+SEED_PAD (#141, Old HSM compat)\r\n");
            GLogN("  0x101 [slot]  RSA Sign SHA256+PKCS1 (#142)\r\n");
            GLogN("  0x102 [slot]  RSA Private Key Import (#141, SHA1)\r\n");
            GLogN("  0x103 [slot]  RSA Private Key Import (#142, SHA256)\r\n");
            GLogN("  0x104 [slot]  RSA Private Key Import (hsm_import_rsa_key)\r\n");
            GLogN("\r\n");
            GLogN("=== ED25519 / HMAC Tests (New HSM only) ===\r\n");
            GLogN("  0x105 [slot][mode] ED25519 Key Import (#261, mode:0=pair,1=pub)\r\n");
            GLogN("  0x106 [slot]  ED25519 Sign Test (Seed->Signature)\r\n");
            GLogN("  0x107 [slot]  HMAC Key Import (#121, 32 bytes)\r\n");
            GLogN("  0x108 [slot]  HMAC_SHA256 Test (Seed+Secret->MAC)\r\n");
            GLogN("\r\n");
            GLogN("=== Additional Tests ===\r\n");
            GLogN("  0x111 [id]    RSA Public Key Export (hsm_export_public_key)\r\n");
            GLogN("  0x112 [mode]  Kauth Import via RSA Encrypt (OP 0x22)\r\n");
            return 0;
        }

        uint16_t opcode = (uint16_t)strtol(gCliArgs[1], NULL, 16);

        switch(opcode)
        {
            case 0x00:  // Info (HSM Version)
            {
                GLogN("\r\n========== HSM Info Test ==========\r\n");
				uint16_t HsmTotalVer = 0;
                HSM_VersionInfo_t version_info;
                HAL_StatusTypeDef status = hsm_get_version_info(&version_info);
                if (status == HAL_OK)
                {
                    GLogN("[OK] Info Success\r\n");
                    GLogN("Host F/W: %d.%d.%d\r\n",
                          version_info.host_major,
                          version_info.host_minor,
                          version_info.host_patch);
                    GLogN("HSE F/W:  %d.%d.%d\r\n",
                          version_info.hse_major,
                          version_info.hse_minor,
                          version_info.hse_patch);
                    GLogN("ASK:      %d.%d.%d (Vendor:0x%04X, Module:0x%04X)\r\n",
                          version_info.ask_major,
                          version_info.ask_minor,
                          version_info.ask_patch,
                          version_info.ask_vendor_id,
                          version_info.ask_module_id);
                    GLogN("Lot:      0x%08X, Serial: 0x%08X\r\n",
                          version_info.lot_number,
                          version_info.serial_number);
                    GLogN("Used Cert Slots: %d\r\n", version_info.used_cert_slots);
					GLogN("Release : %d\r\n",version_info.release);

					HsmTotalVer = version_info.release;

					GLogN("HsmTotalVer: %d\r\n", HsmTotalVer);
                }
                else
                {
                    GLogE("[FAIL] Info failed, status: %d\r\n", status);
                }
                GLogN("====================================\r\n");
                break;
            }

            case 0x10:  // Random test
            {
                // Test TRNG with 32 bytes
                uint8_t random_output[32];

                HAL_StatusTypeDef status = hsm_generate_random(random_output, 32, true);  // true = TRNG

                if (status == HAL_OK)
                {
                    // Basic sanity check: not all zeros
                    bool all_zero = true;
                    for (int i = 0; i < 32; i++)
                    {
                        if (random_output[i] != 0)
                        {
                            all_zero = false;
                            break;
                        }
                    }

                    if (all_zero)
                    {
                        GLogE("[VERIFY FAIL] Random output is all zeros!\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] Random command failed with status: %d\r\n", status);
                }
                break;
            }

            case 0x11:  // SHA test
            {
                // Test SHA-256 with "abc" (NIST test vector)
                const uint8_t test_data[] = "abc";
                uint8_t hash_output[32];

                // Expected SHA-256("abc") from NIST
                const uint8_t expected_hash[32] = {
                    0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
                    0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
                    0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
                    0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
                };

                HAL_StatusTypeDef status = hsm_calculate_sha(
                    HSM_SHA_256,  // 0x01
                    false,        // use_kd = false (standard SHA)
                    0,            // key_id (not used for standard SHA-256)
                    test_data,
                    3,  // strlen("abc")
                    hash_output
                );

                if (status == HAL_OK)
                {
                    // Verify against NIST test vector
                    if (memcmp(hash_output, expected_hash, 32) != 0)
                    {
                        GLogN("[VERIFY FAIL] Hash does NOT match expected value!\r\n");
                        GLogN("Expected: ");
                        for (int i = 0; i < 32; i++)
                        {
                            GLogN("%02X ", expected_hash[i]);
                            if ((i + 1) % 16 == 0) GLogN("\r\n");
                        }
                        GLogN("\r\n");
                        GLogN("Got:      ");
                        for (int i = 0; i < 32; i++)
                        {
                            GLogN("%02X ", hash_output[i]);
                            if ((i + 1) % 16 == 0) GLogN("\r\n");
                        }
                        GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] SHA-256 command failed with status: %d\r\n", status);
                }
                break;
            }

            case 0x05:  // Key Management Generate + Export test
            {
                GLogN("\r\n========================================\r\n");
                GLogN("[TEST] RSA Key Generate + Export (Key ID: %d)\r\n", HSM_KEK_RSA_KEY_ID);
                GLogN("========================================\r\n");

                // Step 1: Generate RSA key pair
                GLogN("\r\n[STEP 1] Generate RSA Key Pair\r\n");
                HAL_StatusTypeDef gen_status = hsm_generate_key_pair(
                    150,	// Key ID 149
                    HSM_ALG_RSA,		// Algorithm RSA
                    0,					// key_size (0 for RSA, used for AES)
                    false				// lock_key = false (unlocked for testing)
                );

                if (gen_status != HAL_OK)
                {
                    GLogE("[FAIL] Generate Key Pair failed, status: %d\r\n", gen_status);
                    GLogE("Cannot proceed to Export step\r\n");
                    GLogE("\r\n[NOTE] Detailed error code was already read by wait_for_host_status() function\r\n");
                    GLogE("[NOTE] Please check above logs for '[WAIT_STATUS ERROR]' or '[WAIT_STATUS] Error code from HSM:' messages\r\n");
                    break;
                }
                GLogN("[OK] Generate Key Pair Success\r\n");

                // Step 2: Export public key
                GLogN("\r\n[STEP 2] Export Public Key\r\n");
                uint8_t pub_key[512];
                uint16_t pub_key_len = 0;

                HAL_StatusTypeDef export_status = hsm_export_public_key(
                    150,	// Key ID 149
                    HSM_ALG_RSA,		// Algorithm RSA
                    pub_key,
                    &pub_key_len
                );

                if (export_status == HAL_OK)
                {
                    GLogN("[OK] Export Public Key Success\r\n");
                    GLogN("Public Key Length: %d bytes\r\n", pub_key_len);
                    GLogN("Public Key Data:\r\n");
                    for (int i = 0; i < pub_key_len; i++)
                    {
                        GLogN("%02X ", pub_key[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    if (pub_key_len % 16 != 0) GLogN("\r\n");

                    GLogN("\r\n========================================\r\n");
                    GLogN("[TEST COMPLETE] All steps passed\r\n");
                    GLogN("========================================\r\n");
                }
                else
                {
                    GLogE("[FAIL] Export Public Key failed, status: %d\r\n", export_status);
                    GLogE("========================================\r\n");
                }
                break;
            }

            // ========== Key Import (0x51) ==========
            case 0x51:  // Import Encrypted Key to Key Slot
            {
                GLogN("\r\n========================================\r\n");
                GLogN("[TEST] Import Encrypted Key\r\n");
                GLogN("========================================\r\n");

                // Hardcoded ECU-CODE KEK (RSA-2048 encrypted, 256 bytes)
                static const uint8_t ecucodeKEK_encrypted[256] = {
                    0x13, 0x98, 0xe2, 0x67, 0x9b, 0x6e, 0xf7, 0x5c, 0x9d, 0x06, 0xde, 0x48, 0xe2, 0x14, 0xb5, 0xdf,
                    0xab, 0xb5, 0x6c, 0x5b, 0x37, 0x81, 0x23, 0x0f, 0x6e, 0x12, 0x76, 0x03, 0x7c, 0x7d, 0xd4, 0x40,
                    0x0c, 0x49, 0xef, 0x9c, 0x6a, 0x31, 0x98, 0xad, 0xc8, 0xfc, 0x94, 0x5b, 0x01, 0x2e, 0x7d, 0x58,
                    0xd1, 0xab, 0xc5, 0xab, 0xf1, 0xf0, 0x89, 0xc1, 0x9f, 0xa4, 0x03, 0x3c, 0x65, 0x74, 0x7e, 0xc2,
                    0x13, 0x3b, 0xd7, 0x03, 0x85, 0x5e, 0x14, 0x31, 0xb7, 0x9c, 0x97, 0xaa, 0x17, 0x0e, 0x3a, 0x27,
                    0x88, 0xbf, 0xa0, 0xdf, 0xdc, 0x25, 0xd0, 0x6c, 0xca, 0x5a, 0x33, 0x15, 0xb8, 0x75, 0xfd, 0x15,
                    0x51, 0x53, 0x00, 0xb7, 0xe5, 0x9c, 0x3d, 0xfc, 0x25, 0xec, 0xc9, 0xbd, 0xcf, 0x70, 0x20, 0x94,
                    0xcf, 0xd4, 0x1a, 0x1c, 0x7d, 0x61, 0x45, 0xf9, 0x7b, 0xab, 0x01, 0xb7, 0xbd, 0x18, 0x91, 0xf2,
                    0x96, 0x7b, 0x1d, 0x61, 0x0a, 0x99, 0x3c, 0x48, 0x8a, 0x78, 0xc3, 0xfe, 0x7e, 0x39, 0x56, 0xa7,
                    0xbd, 0x0a, 0xd7, 0x1e, 0xb6, 0xb9, 0x6f, 0x35, 0x87, 0xce, 0xd8, 0xe1, 0xc2, 0xba, 0x81, 0x0d,
                    0x5d, 0x3e, 0xc4, 0xd0, 0x59, 0x92, 0xd8, 0xc0, 0x6a, 0xa6, 0xee, 0x5f, 0x02, 0x11, 0x78, 0x7e,
                    0xe4, 0xfd, 0x8e, 0x71, 0xfb, 0xbc, 0x71, 0x95, 0xe2, 0x20, 0xe8, 0xa2, 0x0b, 0xf3, 0x69, 0x40,
                    0x01, 0x5d, 0xaa, 0x79, 0x1c, 0xf8, 0xc9, 0x10, 0x06, 0x3b, 0xd4, 0x9e, 0x5e, 0xaf, 0x04, 0x80,
                    0x29, 0x43, 0x41, 0x28, 0x90, 0xcb, 0x15, 0x1f, 0xc4, 0x8d, 0xce, 0xd4, 0x2a, 0x34, 0x10, 0x1e,
                    0x79, 0xcb, 0x97, 0x8b, 0x2d, 0xef, 0xe2, 0xdd, 0x0a, 0x31, 0x94, 0xad, 0x49, 0xb1, 0x11, 0x9c,
                    0x0c, 0x74, 0xb3, 0x82, 0xce, 0xf4, 0xda, 0xfa, 0x2d, 0xbb, 0xca, 0x86, 0x45, 0xce, 0x79, 0xdb
                };

                uint16_t key_id = 101;  // Default: ECU-CODE KEK slot
                const uint8_t* key_data = ecucodeKEK_encrypted;
                size_t data_len = 256;

                // Allow override key_id if provided
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 10);
                }

                GLogN("Key ID: %d (ECU-CODE KEK slot)\r\n", key_id);
                GLogN("Using hardcoded encrypted key data (256 bytes)\r\n");

                GLogN("Parsed Data Length: %d bytes\r\n", (int)data_len);
                GLogN("First 16 bytes: ");
                for (size_t i = 0; i < 16 && i < data_len; i++)
                {
                    GLogN("%02X ", key_data[i]);
                }
                GLogN("\r\n");

                // Call HSM Import function
                // Mode1: Import(0x01) + Encrypted(0x04) = 0x05
                // Mode2: AES256 = 0x03
                GLogN("\r\n[STEP] Calling hsm_import_aes_key()...\r\n");
                GLogN("  encrypted=true, lock_key=false\r\n");

                HAL_StatusTypeDef status = hsm_import_aes_key(
                    key_id,           // Key slot ID
                    key_data,         // Encrypted key data (RSA ciphertext)
                    HSM_AES_256,      // AES-256 key
                    true,             // encrypted = true (RSA-wrapped)
                    false             // lock_key = false (for testing)
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] Import Encrypted Key Success!\r\n");
                    GLogN("Key imported to slot #%d\r\n", key_id);
                }
                else
                {
                    GLogE("[FAIL] Import failed, status: %d\r\n", status);
                    GLogE("[NOTE] Check above logs for detailed error\r\n");
                }
                GLogN("========================================\r\n");
                break;
            }

            // ========== Level 1: Simple Commands (5-byte W_CMD) ==========
            case 0x01:  // Get HSN (HSM Serial Number)
            {
                GLogN("\r\n========== Get HSN Test ==========\r\n");
                uint8_t serial_number[8];
                HAL_StatusTypeDef status = hsm_get_serial_number(serial_number);
                if (status == HAL_OK)
                {
                    GLogN("[OK] Get HSN Success\r\n");
                    GLogN("HSM Serial Number: ");
                    for (int i = 0; i < 8; i++)
                    {
                        GLogN("%02X ", serial_number[i]);
                    }
                    GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] Get HSN failed, status: %d\r\n", status);
                }
                GLogN("===================================\r\n");
                break;
            }

            case 0x02:  // Cancel Job
            {
                GLogN("\r\n========== Cancel Job Test ==========\r\n");
                HAL_StatusTypeDef status = hsm_cancel_job();
                if (status == HAL_OK)
                {
                    GLogN("[OK] Cancel Job Success\r\n");
                }
                else
                {
                    GLogE("[FAIL] Cancel Job failed, status: %d\r\n", status);
                }
                GLogN("======================================\r\n");
                break;
            }

            case 0xCC:  // W_CC - ERROR 상태에서 READY로 초기화
            {
                GLogN("\r\n========== W_CC (Clear Command) ==========\r\n");
                HAL_StatusTypeDef status = spi_cmd_w_cc();
                if (status == HAL_OK)
                {
                    GLogN("[OK] Cancel Command sent\r\n");
                }
                else
                {
                    GLogE("[FAIL] Cancel Command failed, status: %d\r\n", status);
                }
                GLogN("============================================\r\n");
                break;
            }

            case 0x0B:  // Get Last Error
            {
                GLogN("\r\n========== Get Last Error Test ==========\r\n");
                uint8_t error_code = 0U;
                HAL_StatusTypeDef status = hsm_get_last_error(&error_code);
                if (status == HAL_OK)
                {
                    GLogN("[OK] Get Last Error Success\r\n");
                    GLogN("Last Error Code: 0x%02X\r\n", error_code);

                    // Error code interpretation (per HSM SPI Command Manual v3)
                    if (error_code == 0x00U)
                    {
                        GLogN("  -> No Error\r\n");
                    }
                    else if ((error_code >= 0x10U) && (error_code <= 0x13U))
                    {
                        GLogN("  -> HSM Operation Error (0x%02X)\r\n", error_code);
                    }
                    else if ((error_code >= 0x20U) && (error_code <= 0x23U))
                    {
                        GLogN("  -> Type Check Error (0x%02X)\r\n", error_code);
                    }
                    else if ((error_code >= 0x30U) && (error_code <= 0x34U))
                    {
                        GLogN("  -> SPI Dispatcher Error (0x%02X)\r\n", error_code);
                    }
                    else if ((error_code >= 0x40U) && (error_code <= 0x43U))
                    {
                        GLogN("  -> OP-Code Dispatcher Error (0x%02X)\r\n", error_code);
                    }
                    else if ((error_code >= 0x50U) && (error_code <= 0x56U))
                    {
                        GLogN("  -> Crypto Dispatcher Error (0x%02X)\r\n", error_code);
                    }
                    else if ((error_code >= 0x60U) && (error_code <= 0x64U))
                    {
                        GLogN("  -> Flash R/W Error (0x%02X)\r\n", error_code);
                    }
                    else if ((error_code >= 0x70U) && (error_code <= 0x72U))
                    {
                        GLogN("  -> DFU Dispatcher Error (0x%02X)\r\n", error_code);
                    }
                    else
                    {
                        GLogN("  -> Unknown Error (0x%02X)\r\n", error_code);
                    }
                }
                else
                {
                    GLogE("[FAIL] Get Last Error failed, status: %d\r\n", status);
                }
                GLogN("==========================================\r\n");
                break;
            }

            // ========== MAC Generation (OP 0x13) ==========
            case 0x13:  // HMAC-SHA256 test
            {
                GLogN("\r\n========== HMAC-SHA256 Test ==========\r\n");

                // Test data (HMAC requires minimum 64 bytes per HSM spec)
                const uint8_t test_data[] = "Hello HSM HMAC Test Data for Verification - Padded to 64 bytes!!";  // 64 bytes
                uint8_t hmac_output[32];
                uint16_t key_id = HSM_KEY_HMAC_121;  // Default HMAC key slot (#121 UDK)

                // Allow user to specify key_id
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Input Data: \"%s\" (%d bytes)\r\n", test_data, (int)sizeof(test_data) - 1);

                HAL_StatusTypeDef status = hsm_generate_hmac_sha256(
                    key_id,
                    test_data,
                    (uint16_t)(sizeof(test_data) - 1U),
                    hmac_output
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] HMAC-SHA256 Success\r\n");
                    GLogN("HMAC Output (32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", hmac_output[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] HMAC-SHA256 failed, status: %d\r\n", status);
                }
                GLogN("=======================================\r\n");
                break;
            }

            case 0x14:  // CMAC-AES test (actual OP is 0x13, using 0x14 for CLI distinction)
            {
                GLogN("\r\n========== CMAC-AES Test ==========\r\n");
                GLogN("[NOTE] Valid AES Key IDs: UDK #101-108, TEMP #201-204\r\n\r\n");

                // Test data (16-byte aligned for AES)
                const uint8_t test_data[] = "CMAC Test Data!!";  // 16 bytes
                uint8_t cmac_output[16];
                uint16_t key_id = HSM_KEY_AES_104;  // Default: UDK AES slot #104
                uint8_t aes_key_size = 16U;  // AES-128

                // Allow user to specify key_id
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // Allow user to specify key_size (16, 24, 32)
                if (count >= 4)
                {
                    aes_key_size = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("AES Key Size: %d bytes\r\n", aes_key_size);
                GLogN("Input Data: \"%s\" (%d bytes)\r\n", test_data, (int)sizeof(test_data) - 1);

                HAL_StatusTypeDef status = hsm_generate_cmac_aes(
                    key_id,
                    aes_key_size,
                    test_data,
                    (uint16_t)(sizeof(test_data) - 1U),
                    cmac_output
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] CMAC-AES Success\r\n");
                    GLogN("CMAC Output (16 bytes):\r\n");
                    for (int i = 0; i < 16; i++)
                    {
                        GLogN("%02X ", cmac_output[i]);
                    }
                    GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] CMAC-AES failed, status: %d\r\n", status);
                }
                GLogN("=====================================\r\n");
                break;
            }

            case 0x41:  // Store Certificate test
            {
                GLogN("\r\n========== Store Certificate Test ==========\r\n");

                //uint16_t cert_id = 141U;  // Default: RSA cert slot
                uint16_t cert_id = 361U;  // ED 361~380
                if (count >= 3)
                {
                    cert_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                //uint8_t alg_type = HSM_ALG_RSA;  // Default: RSA
                uint8_t alg_type = HSM_ALG_ED;  // ED
                if (count >= 4)
                {
                    alg_type = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                // Test certificate data (dummy data for testing)
                //uint8_t test_cert[512];
				uint8_t test_cert[512]= {
					0x30, 0x82, 0x01, 0xfa, 0x30, 0x82, 0x01, 0xac, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x14, 0x44,
					0x12, 0x6e, 0x03, 0xbb, 0x22, 0x16, 0x34, 0xa0, 0x3e, 0x59, 0x2d, 0xd4, 0x21, 0x6f, 0x12, 0x10,
					0x9d, 0xa4, 0x20, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70, 0x30, 0x17, 0x31, 0x15, 0x30, 0x13,
					0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x0c, 0x5a, 0x46, 0x2d, 0x43, 0x56, 0x2d, 0x64, 0x69, 0x61,
					0x67, 0x63, 0x61, 0x30, 0x1e, 0x17, 0x0d, 0x32, 0x34, 0x31, 0x30, 0x31, 0x34, 0x31, 0x31, 0x31,
					0x32, 0x31, 0x30, 0x5a, 0x17, 0x0d, 0x32, 0x35, 0x30, 0x34, 0x31, 0x34, 0x31, 0x31, 0x32, 0x32,
					0x31, 0x30, 0x5a, 0x30, 0x2f, 0x31, 0x2d, 0x30, 0x2b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x24,
					0x31, 0x61, 0x39, 0x62, 0x64, 0x33, 0x65, 0x34, 0x2d, 0x35, 0x62, 0x63, 0x36, 0x2d, 0x34, 0x33,
					0x65, 0x36, 0x2d, 0x61, 0x34, 0x37, 0x32, 0x2d, 0x35, 0x65, 0x61, 0x64, 0x30, 0x38, 0x64, 0x30,
					0x30, 0x35, 0x33, 0x33, 0x30, 0x2a, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70, 0x03, 0x21, 0x00,
					0xa8, 0x2b, 0xf3, 0xd6, 0x8d, 0x2d, 0xa6, 0xff, 0xe7, 0x4e, 0x04, 0xb7, 0x70, 0xc2, 0x0d, 0xf2,
					0x86, 0x71, 0xde, 0x16, 0x3e, 0xb7, 0x81, 0xba, 0xad, 0xb3, 0xe5, 0x6e, 0x6a, 0x53, 0x00, 0x24,
					0xa3, 0x81, 0xf1, 0x30, 0x81, 0xee, 0x30, 0x0c, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
					0x04, 0x02, 0x30, 0x00, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff, 0x04, 0x04,
					0x03, 0x02, 0x06, 0x40, 0x30, 0x16, 0x06, 0x03, 0x55, 0x1d, 0x25, 0x01, 0x01, 0xff, 0x04, 0x0c,
					0x30, 0x0a, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x08, 0x30, 0x1f, 0x06, 0x0e,
					0x2b, 0x06, 0x01, 0x04, 0x01, 0xe6, 0x67, 0x01, 0x02, 0x83, 0x7d, 0x03, 0x01, 0x01, 0x01, 0x01,
					0xff, 0x04, 0x0a, 0x04, 0x08, 0x64, 0x69, 0x61, 0x67, 0x55, 0x73, 0x65, 0x72, 0x30, 0x1a, 0x06,
					0x0e, 0x2b, 0x06, 0x01, 0x04, 0x01, 0xe6, 0x67, 0x01, 0x02, 0x83, 0x7d, 0x03, 0x02, 0x01, 0x01,
					0x01, 0xff, 0x04, 0x05, 0x04, 0x03, 0x64, 0x65, 0x76, 0x30, 0x18, 0x06, 0x0e, 0x2b, 0x06, 0x01,
					0x04, 0x01, 0xe6, 0x67, 0x01, 0x02, 0x83, 0x7d, 0x03, 0x03, 0x01, 0x01, 0x01, 0xff, 0x04, 0x03,
					0x02, 0x01, 0x20, 0x30, 0x1f, 0x06, 0x0e, 0x2b, 0x06, 0x01, 0x04, 0x01, 0xe6, 0x67, 0x01, 0x02,
					0x83, 0x7d, 0x03, 0x04, 0x01, 0x01, 0x01, 0xff, 0x04, 0x0a, 0x04, 0x08, 0x00, 0x09, 0x00, 0x0f,
					0x00, 0x01, 0x00, 0x01, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x76,
					0xe6, 0x71, 0xc7, 0x10, 0x4a, 0xfd, 0x90, 0x06, 0x0b, 0xd0, 0x6f, 0xf9, 0xb4, 0x20, 0x57, 0x51,
					0xc4, 0x77, 0x3c, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14,
					0xe7, 0xbe, 0x18, 0xd4, 0xc4, 0x39, 0xce, 0x2e, 0x1b, 0x32, 0xe2, 0x95, 0x69, 0x20, 0xbb, 0xcb,
					0xbd, 0xd9, 0x41, 0x5a, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70, 0x03, 0x41, 0x00, 0x5f, 0xec,
					0x3a, 0x89, 0xa7, 0xdd, 0xf4, 0x21, 0x40, 0xee, 0xed, 0xb9, 0xf2, 0x88, 0x1d, 0x5d, 0xbe, 0x9e,
					0xf4, 0x92, 0xe4, 0x78, 0x77, 0x44, 0x63, 0x83, 0x70, 0x2a, 0x0c, 0x7d, 0xd9, 0x0e, 0xde, 0x1b,
					0x9d, 0x1f, 0x8c, 0xd4, 0xec, 0xe1, 0x32, 0x59, 0xc6, 0xcb, 0x43, 0xec, 0xb2, 0x83, 0x5c, 0xb4,
					0x34, 0xb8, 0x41, 0x5c, 0x98, 0x8e, 0xb8, 0xdd, 0x01, 0x62, 0x4b, 0x86, 0x48, 0x05
					
				};
                //uint16_t cert_len = 512U;
                uint16_t cert_len = 510U;

                // Fill with test pattern
                //for (int i = 0; i < 512; i++)
                //{
                //    test_cert[i] = (uint8_t)(i & 0xFF);
                //}

                GLogN("Cert ID: %d (RSA:141-148, ECC/ED:361-380)\r\n", cert_id);
                GLogN("Alg Type: 0x%02X (2=ECC, 3=RSA, 5=ED)\r\n", alg_type);
                GLogN("Cert Length: %d bytes\r\n", cert_len);
                //GLogN("Test Pattern: 0x00, 0x01, 0x02, ..., 0xFF, 0x00, ...\r\n");

                HAL_StatusTypeDef status = hsm_store_certificate(
                    cert_id,
                    alg_type,
                    test_cert,
                    cert_len
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] Store Certificate Success\r\n");
                }
                else
                {
                    GLogE("[FAIL] Store Certificate failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            case 0x42:  // Read Certificate test
            {
                GLogN("\r\n========== Read Certificate Test ==========\r\n");
#if 1
                uint16_t cert_id = 141U;  // Default: RSA cert slot
#else
                uint16_t cert_id = 361U;  // ED cert slot
#endif
                if (count >= 3)
                {
                    cert_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }
#if 1
                uint8_t alg_type = HSM_ALG_RSA;  // Default: RSA
#else
                uint8_t alg_type = HSM_ALG_ED;  // ED
#endif
                if (count >= 4)
                {
                    alg_type = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                uint8_t cert_buffer[2048];
                uint16_t cert_len = 0U;

                GLogN("Cert ID: %d (RSA:141-148, ECC/ED:361-380)\r\n", cert_id);
                GLogN("Alg Type: 0x%02X (2=ECC, 3=RSA, 5=ED)\r\n", alg_type);

                HAL_StatusTypeDef status = hsm_read_certificate(
                    cert_id,
                    alg_type,
                    cert_buffer,
                    &cert_len
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] Read Certificate Success\r\n");
                    GLogN("Cert Length: %d bytes\r\n", cert_len);
                    GLogN("Cert Data (first 64 bytes):\r\n");
                    uint16_t print_len = (cert_len > 64U) ? 64U : cert_len;
                    for (int i = 0; i < print_len; i++)
                    {
                        GLogN("%02X ", cert_buffer[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    if (print_len % 16 != 0) GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] Read Certificate failed, status: %d\r\n", status);
                }
                GLogN("============================================\r\n");
                break;
            }

            // ========== AES Key Generate (0x52) ==========
            case 0x52:  // AES Key Generate
            {
                GLogN("\r\n========== AES Key Generate Test ==========\r\n");
                GLogN("[NOTE] Valid AES Key IDs: UDK #101-108, TEMP #201-204\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_AES_201;   // Default: TEMP AES slot #201
                uint8_t key_size = 3U;   // Default AES-256

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    key_size = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                const char* size_str = (key_size == 1U) ? "AES-128" :
                                       (key_size == 2U) ? "AES-192" : "AES-256";

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Key Size: %s (mode2=0x%02X)\r\n", size_str, key_size);

                HAL_StatusTypeDef status = hsm_generate_key_pair(
                    key_id,
                    HSM_ALG_AES,
                    key_size,
                    false  // lock_key = false
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] AES Key Generate Success\r\n");
                }
                else
                {
                    GLogE("[FAIL] AES Key Generate failed, status: %d\r\n", status);
                }
                GLogN("============================================\r\n");
                break;
            }

            // ========== ECC Key Generate (0x53) ==========
            case 0x53:  // ECC Key Generate (P-256)
            {
                GLogN("\r\n========== ECC Key Generate Test ==========\r\n");
                GLogN("[NOTE] Valid ECC Key IDs: UDK #161-166, TEMP #261-263\r\n");
                GLogN("[NOTE] ECC and ED share same key slots (per HSM manual)\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_ECC_261;   // Default: TEMP ECC slot #261

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: ECC P-256\r\n");

                HAL_StatusTypeDef status = hsm_generate_key_pair(
                    key_id,
                    HSM_ALG_ECC,
                    0U,     // key_size not used for ECC
                    false   // lock_key = false
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] ECC Key Generate Success\r\n");
                }
                else
                {
                    GLogE("[FAIL] ECC Key Generate failed, status: %d\r\n", status);
                }
                GLogN("============================================\r\n");
                break;
            }

            // ========== ED Key Import (0x54) ==========
            // NOTE: HSM does NOT support ED Key Generate - only Import is supported
            // NOTE: Per HSM Manual 5.6.2, ED key requires ENCRYPTED import (not plaintext)
            // Reference: RFC 8032 Ed25519 Test Vector 2
            case 0x54:  // ED Key Import (Ed25519)
            {
                GLogN("\r\n========== ED Key Import Test ==========\r\n");
                GLogN("[NOTE] HSM supports ED Key Import only (not Generate)\r\n");
                GLogN("[REF] RFC 8032 Ed25519 Test Vector 2\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_ED_261;   // Default: TEMP ED key slot (#261)
                bool encrypted = true;  // Default: encrypted import (required for Sign)

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    // Allow override for testing: 0=plaintext, 1=encrypted
                    encrypted = (strtol(gCliArgs[3], NULL, 0) != 0);
                }

                // RFC 8032 Ed25519 Test Vector 2
                // SECRET KEY: 4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb
                // PUBLIC KEY: 3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c
                // MESSAGE: 72 (1 byte)
                // SIGNATURE: 92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da
                //            085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00
                HSM_EDKey_t ed_key = {
                    .private_key = {
                        0x4c, 0xcd, 0x08, 0x9b, 0x28, 0xff, 0x96, 0xda,
                        0x9d, 0xb6, 0xc3, 0x46, 0xec, 0x11, 0x4e, 0x0f,
                        0x5b, 0x8a, 0x31, 0x9f, 0x35, 0xab, 0xa6, 0x24,
                        0xda, 0x8c, 0xf6, 0xed, 0x4f, 0xb8, 0xa6, 0xfb
                    },
                    .public_key = {
                        0x3d, 0x40, 0x17, 0xc3, 0xe8, 0x43, 0x89, 0x5a,
                        0x92, 0xb7, 0x0a, 0xa7, 0x4d, 0x1b, 0x7e, 0xbc,
                        0x9c, 0x98, 0x2c, 0xcf, 0x2e, 0xc4, 0x96, 0x8c,
                        0xc0, 0xcd, 0x55, 0xf1, 0x2a, 0xf4, 0x66, 0x0c
                    }
                };

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: Ed25519\r\n");
                GLogN("Key Type: Full key pair (private + public)\r\n");
                GLogN("Import Mode: %s\r\n", encrypted ? "ENCRYPTED (auto RSA encrypt)" : "PLAINTEXT (test only)");
                GLogN("Private Key: 4ccd089b28ff96da9db6c346ec114e0f...\r\n");
                GLogN("Public Key:  3d4017c3e843895a92b70aa74d1b7ebc...\r\n");

                if (encrypted)
                {
                    GLogN("\r\n[FLOW] Encrypted ED Key Import:\r\n");
                    GLogN("  1. Export RSA KEK Public Key (#149)\r\n");
                    GLogN("  2. Encrypt ED Key (64B) with RSA -> 256B ciphertext\r\n");
                    GLogN("  3. Send encrypted data to HSM\r\n");
                    GLogN("[PRE] Run 'hsmtest 0x05' first to generate RSA KEK\r\n\r\n");
                }

                HAL_StatusTypeDef status = hsm_import_ed_key(
                    key_id,
                    &ed_key,
                    false,      // public_only = false (import full key pair)
                    encrypted,  // encrypted = true (required for Sign)
                    false       // lock_key = false
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] ED Key Import Success\r\n");
                    GLogN("Key imported to slot #%d\r\n", key_id);
                    GLogN("\r\n[NEXT] Run 'hsmtest 0x32' for EdDSA Sign test\r\n");
                }
                else
                {
                    GLogE("[FAIL] ED Key Import failed, status: %d\r\n", status);
                    if (encrypted)
                    {
                        GLogE("[HINT] Check if RSA KEK (#149) is generated first\r\n");
                        GLogE("[HINT] Run 'hsmtest 0x05' to generate RSA KEK\r\n");
                    }
                }
                GLogN("============================================\r\n");
                break;
            }

            // ========== AES Plaintext Key Import (0x55) ==========
            case 0x55:  // AES Plaintext Key Import
            {
                GLogN("\r\n========== AES Plaintext Key Import Test ==========\r\n");

                uint16_t key_id = 50U;   // Default key slot
                uint8_t key_size = 3U;   // Default AES-256

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    key_size = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                // Test key data (for testing only)
                static const uint8_t test_key_128[16] = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
                };
                static const uint8_t test_key_192[24] = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
                };
                static const uint8_t test_key_256[32] = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
                };

                const uint8_t* key_data;
                const char* size_str;

                if (key_size == 1U) {
                    key_data = test_key_128;
                    size_str = "AES-128";
                } else if (key_size == 2U) {
                    key_data = test_key_192;
                    size_str = "AES-192";
                } else {
                    key_data = test_key_256;
                    size_str = "AES-256";
                    key_size = 3U;
                }

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Key Size: %s\r\n", size_str);
                GLogN("Key Type: Plaintext (test key)\r\n");

                HAL_StatusTypeDef status = hsm_import_aes_key(
                    key_id,
                    key_data,
                    key_size,
                    false,  // encrypted = false (plaintext)
                    false   // lock_key = false
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] AES Plaintext Key Import Success\r\n");
                }
                else
                {
                    GLogE("[FAIL] AES Key Import failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== ECC Public Key Export (0x56) ==========
            case 0x56:  // ECC Public Key Export
            {
                GLogN("\r\n========== ECC Public Key Export Test ==========\r\n");

                uint16_t key_id = 20U;   // Default key slot

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                uint8_t pub_key[128];
                uint16_t pub_key_len = 0U;

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: ECC P-256\r\n");

                HAL_StatusTypeDef status = hsm_export_public_key(
                    key_id,
                    HSM_ALG_ECC,
                    pub_key,
                    &pub_key_len
                );

                if (status == HAL_OK)
                {
                    // Manual 5.6.5.1.2: Response Code excluded by spi_cmd_r_cmd
                    // [0-31]=X, [32-63]=Y
                    GLogN("Total Length: %d bytes\r\n", pub_key_len);
                    GLogN("[OK] ECC Public Key Export Success\r\n");
                    GLogN("Public Key X (32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", pub_key[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("Public Key Y (32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", pub_key[32 + i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] ECC Public Key Export failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== ED Public Key Export (0x57) ==========
            case 0x57:  // ED Public Key Export
            {
                GLogN("\r\n========== ED Public Key Export Test ==========\r\n");

                uint16_t key_id = 30U;   // Default key slot

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                uint8_t pub_key[64];
                uint16_t pub_key_len = 0U;

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: Ed25519\r\n");

                HAL_StatusTypeDef status = hsm_export_public_key(
                    key_id,
                    HSM_ALG_ED,
                    pub_key,
                    &pub_key_len
                );

                if (status == HAL_OK)
                {
                    // Manual 5.6.5.1.3: Response Code excluded by spi_cmd_r_cmd
                    // [0-31]=public key
                    GLogN("Total Length: %d bytes\r\n", pub_key_len);
                    GLogN("[OK] ED Public Key Export Success\r\n");
                    GLogN("Public Key (32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", pub_key[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] ED Public Key Export failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== AES Encrypt ECB (0x20) ==========
            case 0x20:  // AES Encrypt ECB mode
            {
                GLogN("\r\n========== AES Encrypt (ECB) Test ==========\r\n");
                GLogN("[NOTE] Valid AES Key IDs: UDK #101-108, TEMP #201-204\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x52' first to generate AES key\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_AES_201;   // Default: TEMP AES slot #201 (same as 0x52)
                uint8_t key_size = HSM_AES_256;

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    key_size = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                // Test plaintext (32 bytes = 2 blocks)
                static const uint8_t plaintext[32] = {
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
                };
                uint8_t ciphertext[32];

                const char* size_str = (key_size == 1U) ? "AES-128" :
                                       (key_size == 2U) ? "AES-192" : "AES-256";

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Key Size: %s\r\n", size_str);
                GLogN("Mode: ECB\r\n");
                GLogN("Plaintext (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", plaintext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_aes_encrypt(
                    key_id,
                    key_size,
                    HSM_AES_ECB,
                    NULL,       // IV not used for ECB
                    plaintext,
                    32,
                    ciphertext
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] AES Encrypt Success\r\n");
                    GLogN("Ciphertext:\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", ciphertext[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] AES Encrypt failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== AES Decrypt ECB (0x21) ==========
            case 0x21:  // AES Decrypt ECB mode
            {
                GLogN("\r\n========== AES Decrypt (ECB) Test ==========\r\n");
                GLogN("[NOTE] Valid AES Key IDs: UDK #101-108, TEMP #201-204\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x52' first to generate AES key\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_AES_201;   // Default: TEMP AES slot #201
                uint8_t key_size = HSM_AES_256;

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    key_size = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                // Test ciphertext (should be result of previous encrypt)
#if 1
                static const uint8_t ciphertext[32] = {
                    0x8E, 0xA2, 0xB7, 0xCA, 0x51, 0x67, 0x45, 0xBF,
                    0xEA, 0xFC, 0x49, 0x90, 0x4B, 0x49, 0x60, 0x89,
                    0x8E, 0xA2, 0xB7, 0xCA, 0x51, 0x67, 0x45, 0xBF,
                    0xEA, 0xFC, 0x49, 0x90, 0x4B, 0x49, 0x60, 0x89
                };
#else
				static const uint8_t ciphertext[32] = {
				    0xE6, 0xC5, 0xD6, 0xC5, 0x23, 0x0E, 0x8A, 0xAA, 0xE5, 0x6E, 0x15, 0xA9, 0xCE, 0x75, 0x12, 0x31,
					0xE6, 0xC5, 0xD6, 0xC5, 0x23, 0x0E, 0x8A, 0xAA, 0xE5, 0x6E, 0x15, 0xA9, 0xCE, 0x75, 0x12, 0x31
                };
#endif
                uint8_t plaintext[32];

                const char* size_str = (key_size == 1U) ? "AES-128" :
                                       (key_size == 2U) ? "AES-192" : "AES-256";

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Key Size: %s\r\n", size_str);
                GLogN("Mode: ECB\r\n");
                GLogN("Ciphertext (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", ciphertext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_aes_decrypt(
                    key_id,
                    key_size,
                    HSM_AES_ECB,
                    NULL,
                    ciphertext,
                    32,
                    plaintext
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] AES Decrypt Success\r\n");
                    GLogN("Plaintext:\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", plaintext[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] AES Decrypt failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== AES Encrypt CBC (0x22) ==========
            case 0x22:  // AES Encrypt CBC mode
            {
                GLogN("\r\n========== AES Encrypt (CBC) Test ==========\r\n");
                GLogN("[NOTE] Valid AES Key IDs: UDK #101-108, TEMP #201-204\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x52' first to generate AES key\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_AES_201;   // Default: TEMP AES slot #201
                uint8_t key_size = HSM_AES_256;

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    key_size = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                static const uint8_t plaintext[32] = {
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
                };
                static const uint8_t iv[16] = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
                };
                uint8_t ciphertext[32];

                const char* size_str = (key_size == 1U) ? "AES-128" :
                                       (key_size == 2U) ? "AES-192" : "AES-256";

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Key Size: %s\r\n", size_str);
                GLogN("Mode: CBC\r\n");
                GLogN("IV: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", iv[i]);
                GLogN("\r\n");
                GLogN("Plaintext (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", plaintext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_aes_encrypt(
                    key_id,
                    key_size,
                    HSM_AES_CBC,
                    iv,
                    plaintext,
                    32,
                    ciphertext
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] AES CBC Encrypt Success\r\n");
                    GLogN("Ciphertext:\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", ciphertext[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] AES CBC Encrypt failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== AES Decrypt CBC (0x23) ==========
            case 0x23:  // AES Decrypt CBC mode
            {
                GLogN("\r\n========== AES Decrypt (CBC) Test ==========\r\n");
                GLogN("[NOTE] Valid AES Key IDs: UDK #101-108, TEMP #201-204\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x52' first to generate AES key\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_AES_201;   // Default: TEMP AES slot #201
                uint8_t key_size = HSM_AES_256;

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    key_size = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                static const uint8_t ciphertext[32] = {
                    0x8E, 0xA2, 0xB7, 0xCA, 0x51, 0x67, 0x45, 0xBF,
                    0xEA, 0xFC, 0x49, 0x90, 0x4B, 0x49, 0x60, 0x89,
                    0x8E, 0xA2, 0xB7, 0xCA, 0x51, 0x67, 0x45, 0xBF,
                    0xEA, 0xFC, 0x49, 0x90, 0x4B, 0x49, 0x60, 0x89
                };
                static const uint8_t iv[16] = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
                };
                uint8_t plaintext[32];

                const char* size_str = (key_size == 1U) ? "AES-128" :
                                       (key_size == 2U) ? "AES-192" : "AES-256";

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Key Size: %s\r\n", size_str);
                GLogN("Mode: CBC\r\n");
                GLogN("IV: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", iv[i]);
                GLogN("\r\n");
                GLogN("Ciphertext (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", ciphertext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_aes_decrypt(
                    key_id,
                    key_size,
                    HSM_AES_CBC,
                    iv,
                    ciphertext,
                    32,
                    plaintext
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] AES CBC Decrypt Success\r\n");
                    GLogN("Plaintext:\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", plaintext[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] AES CBC Decrypt failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== ECDSA Sign (0x30) ==========
            case 0x30:  // ECDSA Sign
            {
                GLogN("\r\n========== ECDSA Sign Test ==========\r\n");
                GLogN("[NOTE] Valid ECC Key IDs: UDK #161-166, TEMP #261-263\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x53' first to generate ECC key\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_ECC_261;   // Default: TEMP ECC slot #261 (same as 0x53)

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // Test digest (SHA-256 of "test")
                static const uint8_t digest[32] = {
                    0x9F, 0x86, 0xD0, 0x81, 0x88, 0x4C, 0x7D, 0x65,
                    0x9A, 0x2F, 0xEA, 0xA0, 0xC5, 0x5A, 0xD0, 0x15,
                    0xA3, 0xBF, 0x4F, 0x1B, 0x2B, 0x0B, 0x82, 0x2C,
                    0xD1, 0x5D, 0x6C, 0x15, 0xB0, 0xF0, 0x0A, 0x08
                };
                uint8_t sig_r[32];
                uint8_t sig_s[32];

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: ECDSA P-256\r\n");
                GLogN("Digest (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", digest[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_sign_ecdsa(
                    key_id,
                    digest,
                    sig_r,
                    sig_s
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] ECDSA Sign Success\r\n");
                    GLogN("Signature R:\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", sig_r[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("Signature S:\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", sig_s[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] ECDSA Sign failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== ECDSA Verify (0x31) ==========
            case 0x31:  // ECDSA Verify
            {
                GLogN("\r\n========== ECDSA Verify Test ==========\r\n");
                GLogN("[NOTE] Valid ECC Key IDs: UDK #161-166, TEMP #261-263\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x53' then 'hsmtest 0x30' first\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_ECC_261;   // Default: TEMP ECC slot #261

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // Test digest and signature (placeholder - needs real values)
                static const uint8_t digest[32] = {
                    0x9F, 0x86, 0xD0, 0x81, 0x88, 0x4C, 0x7D, 0x65,
                    0x9A, 0x2F, 0xEA, 0xA0, 0xC5, 0x5A, 0xD0, 0x15,
                    0xA3, 0xBF, 0x4F, 0x1B, 0x2B, 0x0B, 0x82, 0x2C,
                    0xD1, 0x5D, 0x6C, 0x15, 0xB0, 0xF0, 0x0A, 0x08
                };
                static const uint8_t sig_r[32] = {0}; // Placeholder
                static const uint8_t sig_s[32] = {0}; // Placeholder
                bool is_valid = false;

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: ECDSA P-256\r\n");
                GLogN("[NOTE] Using placeholder signature - sign first with 0x30\r\n");

                HAL_StatusTypeDef status = hsm_verify_ecdsa_signature(
                    key_id,
                    digest,
                    sig_r,
                    sig_s,
                    &is_valid
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] ECDSA Verify completed\r\n");
                    GLogN("Signature Valid: %s\r\n", is_valid ? "YES" : "NO");
                }
                else
                {
                    GLogE("[FAIL] ECDSA Verify failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== EdDSA Sign (0x32) ==========
            // Reference: RFC 8032 Ed25519 Test Vector 2
            case 0x32:  // EdDSA Sign
            {
                GLogN("\r\n========== EdDSA Sign Test ==========\r\n");
                GLogN("[REF] RFC 8032 Ed25519 Test Vector 2\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_ED_261;   // Default: TEMP ED key slot (#261, private key)

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // RFC 8032 Test Vector 2: MESSAGE = 0x72 (1 byte)
                // Expected Signature:
                // 92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da
                // 085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00
                static const uint8_t message[] = { 0x72 };
                uint16_t msg_len = 1;
                uint8_t signature[64];

                // Expected signature for verification
                static const uint8_t expected_sig[64] = {
                    0x92, 0xa0, 0x09, 0xa9, 0xf0, 0xd4, 0xca, 0xb8,
                    0x72, 0x0e, 0x82, 0x0b, 0x5f, 0x64, 0x25, 0x40,
                    0xa2, 0xb2, 0x7b, 0x54, 0x16, 0x50, 0x3f, 0x8f,
                    0xb3, 0x76, 0x22, 0x23, 0xeb, 0xdb, 0x69, 0xda,
                    0x08, 0x5a, 0xc1, 0xe4, 0x3e, 0x15, 0x99, 0x6e,
                    0x45, 0x8f, 0x36, 0x13, 0xd0, 0xf1, 0x1d, 0x8c,
                    0x38, 0x7b, 0x2e, 0xae, 0xb4, 0x30, 0x2a, 0xee,
                    0xb0, 0x0d, 0x29, 0x16, 0x12, 0xbb, 0x0c, 0x00
                };

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: Ed25519\r\n");
                GLogN("Message: 0x72 (1 byte)\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x54' first to import ED key\r\n\r\n");

                HAL_StatusTypeDef status = hsm_sign_eddsa(
                    key_id,
                    message,
                    msg_len,
                    signature
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] EdDSA Sign Success\r\n");
                    GLogN("Signature (64 bytes):\r\n");
                    for (int i = 0; i < 64; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }

                    // Compare with expected signature
                    bool match = true;
                    for (int i = 0; i < 64; i++)
                    {
                        if (signature[i] != expected_sig[i])
                        {
                            match = false;
                            break;
                        }
                    }
                    GLogN("\r\nExpected Signature Match: %s\r\n", match ? "YES" : "NO");
                    if (!match)
                    {
                        GLogN("Expected:\r\n");
                        for (int i = 0; i < 64; i++)
                        {
                            GLogN("%02X ", expected_sig[i]);
                            if ((i + 1) % 16 == 0) GLogN("\r\n");
                        }
                    }
                }
                else
                {
                    GLogE("[FAIL] EdDSA Sign failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== EdDSA Verify (0x33) ==========
            // Reference: RFC 8032 Ed25519 Test Vector 2
            case 0x33:  // EdDSA Verify
            {
                GLogN("\r\n========== EdDSA Verify Test ==========\r\n");
                GLogN("[REF] RFC 8032 Ed25519 Test Vector 2\r\n\r\n");

                // Note: For Ed25519 verify, same slot as signing can be used
                // HSM internally uses public key portion from the key pair
                uint16_t key_id = HSM_KEY_TEMP_ED_261;   // Default: TEMP ED key slot (#261)

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // RFC 8032 Test Vector 2: MESSAGE = 0x72 (1 byte)
                static const uint8_t message[] = { 0x72 };
                uint16_t msg_len = 1;

                // RFC 8032 Test Vector 2: Known valid signature
                static const uint8_t signature[64] = {
                    0x92, 0xa0, 0x09, 0xa9, 0xf0, 0xd4, 0xca, 0xb8,
                    0x72, 0x0e, 0x82, 0x0b, 0x5f, 0x64, 0x25, 0x40,
                    0xa2, 0xb2, 0x7b, 0x54, 0x16, 0x50, 0x3f, 0x8f,
                    0xb3, 0x76, 0x22, 0x23, 0xeb, 0xdb, 0x69, 0xda,
                    0x08, 0x5a, 0xc1, 0xe4, 0x3e, 0x15, 0x99, 0x6e,
                    0x45, 0x8f, 0x36, 0x13, 0xd0, 0xf1, 0x1d, 0x8c,
                    0x38, 0x7b, 0x2e, 0xae, 0xb4, 0x30, 0x2a, 0xee,
                    0xb0, 0x0d, 0x29, 0x16, 0x12, 0xbb, 0x0c, 0x00
                };
                bool is_valid = false;

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: Ed25519\r\n");
                GLogN("Message: 0x72 (1 byte)\r\n");
                GLogN("Signature: RFC 8032 Test Vector 2 (known valid)\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x54' first to import ED key\r\n\r\n");

                HAL_StatusTypeDef status = hsm_verify_eddsa_signature(
                    key_id,
                    message,
                    msg_len,
                    signature,
                    &is_valid
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] EdDSA Verify completed\r\n");
                    GLogN("Signature Valid: %s\r\n", is_valid ? "YES (Expected)" : "NO (Unexpected!)");
                }
                else
                {
                    GLogE("[FAIL] EdDSA Verify failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== RSA Sign (0x34) ==========
            case 0x34:  // RSA Sign
            {
                GLogN("\r\n========== RSA Sign Test ==========\r\n");

                uint16_t key_id = 150U;   // Default RSA key slot

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // SHA-256 digest
                static const uint8_t digest[32] = {
                    0x9F, 0x86, 0xD0, 0x81, 0x88, 0x4C, 0x7D, 0x65,
                    0x9A, 0x2F, 0xEA, 0xA0, 0xC5, 0x5A, 0xD0, 0x15,
                    0xA3, 0xBF, 0x4F, 0x1B, 0x2B, 0x0B, 0x82, 0x2C,
                    0xD1, 0x5D, 0x6C, 0x15, 0xB0, 0xF0, 0x0A, 0x08
                };
                uint8_t signature[256];

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: RSA-2048 + SHA-256\r\n");
                GLogN("Digest (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", digest[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_sign_rsa(
                    key_id,
                    2,          // SHA-256
                    digest,
                    signature
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Sign Success\r\n");
                    GLogN("Signature (256 bytes, first 64):\r\n");
                    for (int i = 0; i < 64; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("...\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Sign failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== RSA Verify (0x35) ==========
            case 0x35:  // RSA Verify
            {
                GLogN("\r\n========== RSA Verify Test ==========\r\n");

                uint16_t key_id = 150U;

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                static const uint8_t digest[32] = {
                    0x9F, 0x86, 0xD0, 0x81, 0x88, 0x4C, 0x7D, 0x65,
                    0x9A, 0x2F, 0xEA, 0xA0, 0xC5, 0x5A, 0xD0, 0x15,
                    0xA3, 0xBF, 0x4F, 0x1B, 0x2B, 0x0B, 0x82, 0x2C,
                    0xD1, 0x5D, 0x6C, 0x15, 0xB0, 0xF0, 0x0A, 0x08
                };
                static const uint8_t signature[256] = {0}; // Placeholder
                bool is_valid = false;

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: RSA-2048 + SHA-256\r\n");
                GLogN("[NOTE] Using placeholder signature - sign first with 0x34\r\n");

                HAL_StatusTypeDef status = hsm_verify_rsa_signature(
                    key_id,
                    2,          // SHA-256
                    digest,
                    signature,
                    &is_valid
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Verify completed\r\n");
                    GLogN("Signature Valid: %s\r\n", is_valid ? "YES" : "NO");
                }
                else
                {
                    GLogE("[FAIL] RSA Verify failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== RSA Encrypt (0x36) ==========
            case 0x36:  // RSA Encrypt
            {
                GLogN("\r\n========== RSA Encrypt Test ==========\r\n");

                uint16_t key_id = 150U;

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // Test plaintext (32 bytes)
                static const uint8_t plaintext[32] = {
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
                };
                uint8_t ciphertext[256];

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: RSAES-PKCS1-v1_5\r\n");
                GLogN("Plaintext (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", plaintext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_rsa_encrypt(
                    key_id,
                    plaintext,
                    32,
                    ciphertext
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Encrypt Success\r\n");
                    GLogN("Ciphertext (256 bytes):\r\n");
                    for (int i = 0; i < 256; i++)
                    {
                        GLogN("%02X ", ciphertext[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] RSA Encrypt failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== RSA Decrypt (0x37) ==========
            case 0x37:  // RSA Decrypt
            {
                GLogN("\r\n========== RSA Decrypt Test ==========\r\n");

                uint16_t key_id = 150U;

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // Placeholder ciphertext (should use result from 0x36)
                static const uint8_t ciphertext[256] = {0};
                uint8_t plaintext[256];
                uint16_t plaintext_length = 0U;

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: RSAES-PKCS1-v1_5\r\n");
                GLogN("[NOTE] Using placeholder ciphertext - encrypt first with 0x36\r\n");

                HAL_StatusTypeDef status = hsm_rsa_decrypt(
                    key_id,
                    ciphertext,
                    plaintext,
                    &plaintext_length
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Decrypt Success\r\n");
                    GLogN("Plaintext Length: %d bytes\r\n", plaintext_length);
                    GLogN("Plaintext:\r\n");
                    for (int i = 0; i < plaintext_length; i++)
                    {
                        GLogN("%02X ", plaintext[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    if (plaintext_length % 16 != 0) GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Decrypt failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== Key Exchange ECDH (0x07) ==========
            case 0x07:  // Key Exchange ECDH
            {
                GLogN("\r\n========== Key Exchange ECDH Test ==========\r\n");
                GLogN("[NOTE] Valid ECC Key IDs: UDK #161-166, TEMP #261-263\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x53' first to generate ECC key\r\n\r\n");

                uint16_t key_id = HSM_KEY_TEMP_ECC_261;   // Default: TEMP ECC slot #261 (same as 0x53)

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // Test peer public key (P-256)
                static const uint8_t peer_public_x[32] = {
                    0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
                    0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
                    0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
                    0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96
                };
                static const uint8_t peer_public_y[32] = {
                    0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B,
                    0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
                    0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE,
                    0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5
                };
                uint8_t shared_secret[32];

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: ECDH SECP256r1\r\n");
                GLogN("Peer Public Key X:\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", peer_public_x[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("Peer Public Key Y:\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", peer_public_y[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_key_exchange_ecdh(
                    key_id,
                    peer_public_x,
                    peer_public_y,
                    shared_secret
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] ECDH Key Exchange Success\r\n");
                    GLogN("Shared Secret (32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", shared_secret[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] ECDH Key Exchange failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== HKDF (0x16) ==========
            case 0x16:  // HKDF Key Derivation
            {
                GLogN("\r\n========== HKDF Test ==========\r\n");

                uint16_t source_key_id = 0U;

                if (count >= 3)
                {
                    source_key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                // Test password (if source_key_id == 0)
                static const uint8_t password[16] = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
                };
                // Test salt
                static const uint8_t salt[16] = {
                    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
                };
                // Test info
                static const uint8_t info[8] = {
                    0x48, 0x4B, 0x44, 0x46, 0x54, 0x45, 0x53, 0x54  // "HKDFTEST"
                };
                uint8_t derived_key[32];

                GLogN("Source Key ID: %d (0=use password)\r\n", source_key_id);
                GLogN("Algorithm: HKDF-SHA256\r\n");
                GLogN("Output Length: 32 bytes\r\n");

                HAL_StatusTypeDef status = hsm_derive_key_hkdf(
                    source_key_id,
                    password,
                    16,
                    salt,
                    16,
                    info,
                    8,
                    32,
                    derived_key
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] HKDF Key Derivation Success\r\n");
                    GLogN("Derived Key (32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", derived_key[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] HKDF failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== ASK Generate (0x60) ==========
            case 0x60:  // ASK Generate Key
            {
                GLogN("\r\n========== ASK Generate Test ==========\r\n");

                uint8_t master_id = 0U;
                uint8_t use_iv = 0U;  // 0=reserved IV, 1=user IV

                if (count >= 3)
                {
                    master_id = (uint8_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    use_iv = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                // Test seed (8 bytes) - Any 8-byte value for testing
                static const uint8_t seed[8] = {
                    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
                };
                // Document 5.16.4 Example - Encrypted ECU code (16 bytes)
                // Original: 11 22 33 44 55 66 77 88 (+ IV first 8 bytes for padding)
                // Encrypted with ECU-Code Key (AES128-CTR)
                static const uint8_t encrypted_ecu_code[16] = {
                    0x3C, 0x0F, 0x2F, 0x5F, 0x0C, 0x35, 0x9B, 0x95,
                    0xB0, 0x2C, 0x39, 0xEE, 0xD3, 0xC2, 0x99, 0xC0
                };
                // Document 5.16.4 Example - IV for AES-128-CTR (16 bytes)
                // "GitVCI3HSMAES_IV" in ASCII
                static const uint8_t iv[16] = {
                    0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48,
                    0x53, 0x4D, 0x41, 0x45, 0x53, 0x5F, 0x49, 0x56
                };
                uint8_t ask_key[8];

                GLogN("Master ID: %d\r\n", master_id);
                GLogN("IV Mode: %s\r\n", use_iv ? "User IV (IV Length=16)" : "Reserved IV (IV Length=0)");
                GLogN("Seed: ");
                for (int i = 0; i < 8; i++) GLogN("%02X ", seed[i]);
                GLogN("\r\n");
                GLogN("Encrypted ECU-Code (Doc 5.16.4): ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", encrypted_ecu_code[i]);
                GLogN("\r\n");
                if (use_iv)
                {
                    GLogN("IV (GitVCI3HSMAES_IV): ");
                    for (int i = 0; i < 16; i++) GLogN("%02X ", iv[i]);
                    GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_generate_ask_key(
                    seed,
                    encrypted_ecu_code,
                    master_id,
                    use_iv ? iv : NULL,  // NULL = reserved IV, non-NULL = user IV
                    ask_key
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] ASK Generate Success\r\n");
                    GLogN("ASK Key (8 bytes): ");
                    for (int i = 0; i < 8; i++) GLogN("%02X ", ask_key[i]);
                    GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] ASK Generate failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== KEK Import (0x62) - RSA Public Key to #149 ==========
            case 0x62:  // KEK (Key Encryption Key) Import
            {
                GLogN("\r\n========== KEK Import Test (RSA#241) ==========\r\n");

                // KEK RSA-2048 Public Key (provided by user)
                // Modulus: 256 bytes
                static const uint8_t kek_modulus[256] = {
                    0xba, 0xb0, 0x07, 0x17, 0x31, 0xe0, 0x5b, 0x2e, 0x09, 0x55, 0x36, 0x3c, 0x29, 0x25, 0x49, 0x2b,
                    0x50, 0xf3, 0x31, 0x15, 0x18, 0x20, 0x05, 0x3f, 0xb3, 0xe5, 0x14, 0x93, 0x57, 0x6a, 0x56, 0x0c,
                    0x65, 0xcb, 0x10, 0x3a, 0xb3, 0x72, 0xad, 0xc7, 0xc9, 0x3f, 0xe2, 0x41, 0xba, 0xc2, 0x50, 0x5e,
                    0xec, 0x7b, 0x10, 0xf0, 0xbc, 0xe7, 0x81, 0x94, 0x13, 0x52, 0x7d, 0x72, 0x04, 0x38, 0xb7, 0x0f,
                    0xf0, 0xb9, 0xd8, 0xe3, 0x8d, 0xf1, 0x97, 0xd5, 0xc8, 0x99, 0x87, 0xad, 0xdd, 0xaf, 0x25, 0x72,
                    0x49, 0x35, 0x9a, 0xb6, 0xe6, 0x23, 0x1e, 0x99, 0xf0, 0xf6, 0x04, 0xad, 0x8a, 0xa6, 0x63, 0x5e,
                    0xac, 0x0f, 0x3d, 0xd7, 0xee, 0xc2, 0xc2, 0xf8, 0x22, 0x67, 0x8c, 0xc8, 0xbd, 0xc7, 0x61, 0x10,
                    0x7c, 0x6e, 0x44, 0xf7, 0x04, 0xb9, 0xdf, 0xd6, 0xc8, 0xcb, 0x66, 0xfc, 0xca, 0xbf, 0x68, 0x8f,
                    0x3c, 0x3e, 0xba, 0x36, 0x11, 0xe5, 0x6a, 0x17, 0xe2, 0x95, 0xb8, 0xa9, 0xeb, 0xd5, 0x8f, 0xad,
                    0x5e, 0xd6, 0x08, 0x71, 0x3f, 0x7e, 0x49, 0x93, 0x5d, 0xb0, 0xb9, 0xaf, 0x4e, 0xf0, 0x51, 0x6a,
                    0x3e, 0xf8, 0xc7, 0x64, 0x98, 0x85, 0x68, 0x26, 0xfc, 0x5f, 0x14, 0x38, 0x51, 0xe8, 0x44, 0xb4,
                    0x65, 0x37, 0xca, 0xc2, 0x8e, 0x1f, 0x92, 0x9e, 0xaf, 0xba, 0xe6, 0xaa, 0x8f, 0x0f, 0xf1, 0xeb,
                    0x99, 0xb3, 0x56, 0xb7, 0x4d, 0x03, 0xba, 0xbb, 0xec, 0xa3, 0xd2, 0x5e, 0x5d, 0x82, 0xb3, 0xdd,
                    0x03, 0x7f, 0xa7, 0x9e, 0xf1, 0x3e, 0xf1, 0x25, 0x70, 0xcd, 0x00, 0x22, 0xcb, 0x00, 0x46, 0x0c,
                    0xb2, 0x96, 0xee, 0x21, 0x1a, 0xf5, 0xea, 0xf2, 0x03, 0xcb, 0x1b, 0x59, 0x0c, 0x87, 0x35, 0x2a,
                    0xb6, 0x14, 0x44, 0x74, 0xfc, 0xdd, 0x3c, 0x82, 0xd4, 0x6b, 0x72, 0x4d, 0x72, 0xf8, 0xec, 0xb1
                };
                // Public exponent: 0x00010001 (65537) - 4 bytes
                static const uint8_t kek_exponent[4] = { 0x00, 0x01, 0x00, 0x01 };

                HSM_RSAKey_t rsa_key;
                memset(&rsa_key, 0, sizeof(rsa_key));
                rsa_key.modulus_length = 256;
                memcpy(rsa_key.modulus, kek_modulus, 256);
                rsa_key.public_exp_size = 4;
                memcpy(rsa_key.public_exp, kek_exponent, 4);

                GLogN("Key ID: 241 (TEMP RSA Pub slot)\r\n");
                GLogN("Algorithm: RSA-2048\r\n");
                GLogN("Modulus (first 32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", kek_modulus[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("Public Exponent: %02X%02X%02X%02X (65537)\r\n",
                      kek_exponent[0], kek_exponent[1], kek_exponent[2], kek_exponent[3]);

                HAL_StatusTypeDef status = hsm_import_rsa_key(
                    241,            // TEMP RSA Pub slot #241 (public key only)
                    &rsa_key,
                    true,           // public_only = true
                    false,          // encrypted = false
                    false,          // lock_key = false (for testing)
                    HSM_RSA_2048    // key_size_bits
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] KEK Import Success!\r\n");
                    GLogN("RSA Public Key imported to slot #241\r\n");
                }
                else
                {
                    GLogE("[FAIL] KEK Import failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== KM Import (0x63) - AES Key to #102 ==========
            case 0x63:  // KM (Master Key) Import
            {
                GLogN("\r\n========== KM Import Test (AES#102) ==========\r\n");

                // KM AES-128 Key (16 bytes) - provided by user
                // Original: e446276d52a38d6a21c0f39e9c58480 (31 chars - missing 1 char)
                // Assumed: 0e446276d52a38d6a21c0f39e9c58480 (leading 0 added)
                static const uint8_t km_plaintext[32] = {
                    // RFC 4868 AUTH256-1 test key (32 x 0x0b)
                    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
                };

                uint8_t import_mode = 0U;  // 0=plaintext, 1=encrypted with KEK

                if (count >= 3)
                {
                    import_mode = (uint8_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: 102 (KM slot)\r\n");
                GLogN("Algorithm: AES-128\r\n");
                GLogN("Import Mode: %s\r\n", import_mode ? "Encrypted (with KEK#149)" : "Plaintext");
                GLogN("KM Key Value:\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", km_plaintext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                if (import_mode == 0)
                {
                    // Plaintext import
                    HAL_StatusTypeDef status = hsm_import_hmac_key(
                        102,			// KM key slot
                        km_plaintext,	// 32 bytes HMAC key
                        false,			// encrypted = false
                        false			// lock_key = false
                    );

                    if (status == HAL_OK)
                    {
                        GLogN("[OK] KM Import (Plaintext) Success!\r\n");
                        GLogN("AES-128 Key imported to slot #102\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] KM Import failed, status: %d\r\n", status);
                    }
                }
                else
                {
                    // Encrypted import (need to encrypt km_plaintext with KEK#149 first)
                    // For now, show placeholder - user needs to provide RSA-encrypted KM
                    GLogN("[NOTE] Encrypted import requires RSA-encrypted KM data\r\n");
                    GLogN("       1. Encrypt km_plaintext with KEK#149 public key\r\n");
                    GLogN("       2. Provide 256-byte RSA ciphertext\r\n");
                    GLogN("[SKIP] Using placeholder - implement with real encrypted data\r\n");

                    // Placeholder for encrypted KM (256 bytes RSA ciphertext)
                    static const uint8_t km_encrypted[256] = {0};

                    HAL_StatusTypeDef status = hsm_import_aes_key(
                        102,
                        km_encrypted,
                        HSM_AES_128,
                        true,           // encrypted = true
                        false
                    );

                    if (status == HAL_OK)
                    {
                        GLogN("[OK] KM Import (Encrypted) Success!\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] KM Import (Encrypted) failed, status: %d\r\n", status);
                    }
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== KM Import AES-128 (0x64) ==========
            case 0x64:  // KM (Master Key) Import as AES-128
            {
                GLogN("\r\n========== KM Import Test (AES-128 #102) ==========\r\n");

                // NIST SP 800-38A F.1.1 - AES-128 ECB Test Key
                // Reference: https://csrc.nist.gov/publications/detail/sp/800-38a/final
                // Test Vector for ECB-AES128.Encrypt
                //   Key:        2b7e151628aed2a6abf7158809cf4f3c
                //   Plaintext:  6bc1bee22e409f96e93d7e117393172a
                //   Ciphertext: 3ad77bb40d7a3660a89ecaf32466ef97
                static const uint8_t km_aes128[16] = {
                    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
                };

                GLogN("Key ID: 102 (KM slot)\r\n");
                GLogN("Algorithm: AES-128 (16 bytes)\r\n");
                GLogN("Standard: NIST SP 800-38A F.1.1\r\n");
                GLogN("KM Key Value: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", km_aes128[i]);
                GLogN("\r\n\r\n");

                GLogN("Calling hsm_import_aes_key()...\r\n");
                HAL_StatusTypeDef status = hsm_import_aes_key(
                    102,            // KM slot #102
                    km_aes128,      // 16 bytes AES-128 key
                    HSM_AES_128,    // 0x01
                    false,          // plaintext (not encrypted)
                    false           // don't lock
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] KM Import (AES-128) Success!\r\n");
                    GLogN("AES-128 Key imported to slot #102\r\n");
                }
                else
                {
                    GLogE("[FAIL] KM Import (AES-128) failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== KM Import - Production Key (0x65) ==========
            case 0x65:  // KM (Master Key) Import - Actual Production Key
            {
                GLogN("\r\n========== KM Import (Production Key #102) ==========\r\n");

                // Production KM key value (from HSM provisioning document)
                // KM: 0e446276d52a38d6a21c0f39e9c58480 (16 bytes, AES-128)
                static const uint8_t km_production[16] = {
                    0x0e, 0x44, 0x62, 0x76, 0xd5, 0x2a, 0x38, 0xd6,
                    0xa2, 0x1c, 0x0f, 0x39, 0xe9, 0xc5, 0x84, 0x80
                };

                GLogN("Key ID: 102 (KM slot)\r\n");
                GLogN("Algorithm: AES-128 (16 bytes)\r\n");
                GLogN("Source: Production Provisioning Document\r\n");
                GLogN("KM Key Value: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", km_production[i]);
                GLogN("\r\n\r\n");

                GLogN("Calling hsm_import_aes_key()...\r\n");
                HAL_StatusTypeDef status = hsm_import_aes_key(
                    102,              // KM slot #102
                    km_production,    // 16 bytes AES-128 key
                    HSM_AES_128,      // 0x01
                    false,            // plaintext (not encrypted)
                    false             // don't lock
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] KM Import (Production) Success!\r\n");
                    GLogN("Production KM Key imported to slot #102\r\n");
                }
                else
                {
                    GLogE("[FAIL] KM Import (Production) failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== KM Import - AES-256 (0x66) ==========
            case 0x66:  // KM (Master Key) Import as AES-256
            {
                GLogN("\r\n========== KM Import Test (AES-256 #102) ==========\r\n");

                // NIST SP 800-38A F.1.5 - AES-256 ECB Test Key
                // Reference: https://csrc.nist.gov/publications/detail/sp/800-38a/final
                // Test Vector for ECB-AES256.Encrypt
                //   Key: 603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4
                static const uint8_t km_aes256[32] = {
                    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
                    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
                    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
                    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
                };

                GLogN("Key ID: 102 (KM slot)\r\n");
                GLogN("Algorithm: AES-256 (32 bytes)\r\n");
                GLogN("Standard: NIST SP 800-38A F.1.5\r\n");
                GLogN("KM Key Value: ");
                for (int i = 0; i < 32; i++) GLogN("%02X ", km_aes256[i]);
                GLogN("\r\n\r\n");

                GLogN("Calling hsm_import_aes_key()...\r\n");
                HAL_StatusTypeDef status = hsm_import_aes_key(
                    102,            // KM slot #102
                    km_aes256,      // 32 bytes AES-256 key
                    HSM_AES_256,    // 0x03
                    false,          // plaintext (not encrypted)
                    false           // don't lock
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] KM Import (AES-256) Success!\r\n");
                    GLogN("AES-256 Key imported to slot #102\r\n");
                }
                else
                {
                    GLogE("[FAIL] KM Import (AES-256) failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== Mutual Auth (0x70) ==========
            case 0x70:  // Mutual Authentication (Step-by-step with SW HMAC)
            {
                GLogN("\r\n========== Mutual Authentication Test (Step-by-Step) ==========\r\n");

                bool use_kauth = false;

                if (count >= 3)
                {
                    use_kauth = (strtol(gCliArgs[2], NULL, 0) != 0);
                }

                // Test random SR from VCI (16 bytes) - RFC 4231 test vector compatible
                static const uint8_t random_sr[16] = {
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
                };

                // Test Km key (must match what's provisioned in HSM slot #102)
                // Same as km_production in case 0x65 (KM Import Production)
                // Only first 16 bytes are used for HMAC (AES-128 key)
                static const uint8_t test_km[16] = {
                    // Production KM Key: 0e446276d52a38d6a21c0f39e9c58480
                    0x0e, 0x44, 0x62, 0x76, 0xd5, 0x2a, 0x38, 0xd6,
                    0xa2, 0x1c, 0x0f, 0x39, 0xe9, 0xc5, 0x84, 0x80
                };

                uint8_t random_cr[16] = {0};
                uint8_t signature_hsm[32] = {0};  // Sign1 from HSM
                uint8_t key_id = 0U;
                bool auth_success = false;

                GLogN("Use Kauth: %s\r\n", use_kauth ? "YES" : "NO (use Km)");
                GLogN("Random SR from VCI: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", random_sr[i]);
                GLogN("\r\n");

                // ===== STEP 1: Internal Auth (Get CR + Sign1) =====
                GLogN("\r\n[STEP 1] Internal Auth - Get CR + Sign1\r\n");
                HAL_StatusTypeDef status = hsm_internal_auth(use_kauth, random_sr, random_cr, signature_hsm, &key_id);

                if (status != HAL_OK)
                {
                    GLogE("[FAIL] Internal Auth failed, status: %d\r\n", status);
                    GLogN("=============================================\r\n");
                    break;
                }

                GLogN("[OK] Internal Auth Success\r\n");
                GLogN("Random CR from HSM: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", random_cr[i]);
                GLogN("\r\n");
                GLogN("Sign1 from HSM (HMAC(SR||CR, Km)):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", signature_hsm[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("Key ID: %d\r\n", key_id);
                // ===== STEP 1.5: Verify Sign1 (SW vs HSM) =====
                GLogN("\r\n[STEP 1.5] Verify Sign1 - SW HMAC(SR||CR, Km) vs HSM Sign1\r\n");

                // Prepare SR||CR (32 bytes) for Sign1 verification
                uint8_t sr_cr[32];
                memcpy(sr_cr, random_sr, 16);       // SR first
                memcpy(sr_cr + 16, random_cr, 16);  // CR second

                uint8_t sw_sign1[32] = {0};

                // Use manual HMAC-SHA256 (CMOX cmox_mac_compute has issues)
                int hmac_ret = manual_hmac_sha256(test_km, 16, sr_cr, 32, sw_sign1);

                if (hmac_ret != 0)
                {
                    GLogE("[FAIL] SW HMAC Sign1 computation failed (ret=%d)\r\n", hmac_ret);
                    break;
                }

                GLogN("SW  Sign1: ");
                for (int i = 0; i < 32; i++) GLogN("%02X ", sw_sign1[i]);
                GLogN("\r\n");
                GLogN("HSM Sign1: ");
                for (int i = 0; i < 32; i++) GLogN("%02X ", signature_hsm[i]);
                GLogN("\r\n");

                if (memcmp(sw_sign1, signature_hsm, 32) == 0)
                {
                    GLogN("[OK] Sign1 MATCH! Km and HMAC algorithm are identical.\r\n");
                }
                else
                {
                    GLogE("[FAIL] Sign1 MISMATCH! Check Km key or HMAC algorithm.\r\n");
                    GLogE("  - SW uses HMAC-SHA256 with 16-byte Km (AES-128)\r\n");
                    GLogE("  - HSM may use different key or algorithm\r\n");
                }

                // ===== STEP 2: Compute Sign2 using Software HMAC =====
                GLogN("\r\n[STEP 2] Compute Sign2 = HMAC(CR||SR, Km)\r\n");

                // Prepare CR||SR (32 bytes)
                uint8_t cr_sr[32];
                memcpy(cr_sr, random_cr, 16);       // CR first
                memcpy(cr_sr + 16, random_sr, 16);  // SR second

                // Compute Sign2 using manual HMAC-SHA256
                uint8_t sign2[32] = {0};

                hmac_ret = manual_hmac_sha256(test_km, 16, cr_sr, 32, sign2);

                if (hmac_ret != 0)
                {
                    GLogE("[FAIL] SW HMAC Sign2 computation failed (ret=%d)\r\n", hmac_ret);
                    GLogN("[NOTE] Ensure Km in HSM matches test_km for loopback test\r\n");
                    GLogN("=============================================\r\n");
                    break;
                }

                GLogN("[OK] Sign2 computed (SW HMAC-SHA256)\r\n");
                GLogN("Sign2 (HMAC(CR||SR, Km)):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", sign2[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                // ===== STEP 3: External Auth (Send Sign2) =====
                GLogN("\r\n[STEP 3] External Auth - Send Sign2\r\n");

                status = hsm_external_auth(use_kauth, sign2, &auth_success);

                if (status != HAL_OK)
                {
                    GLogE("[FAIL] External Auth failed, status: %d\r\n", status);
                    GLogN("[NOTE] Check if Km in HSM slot #102 matches test_km\r\n");
                    GLogN("[RECOVER] Sending W_CC to reset HSM state...\r\n");
                    spi_cmd_w_cc();
                }
                else
                {
                    if (auth_success)
                    {
                        GLogN("[OK] Mutual Authentication SUCCESS!\r\n");
                        GLogN("Session Key Ks generated at slot #209\r\n");
                    }
                    else
                    {
                        GLogN("[WARN] External Auth completed but verification FAILED\r\n");
                        GLogN("[NOTE] Sign2 mismatch - Km key in HSM differs from test_km\r\n");
                    }
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== Mutual Auth AES-256 (0x71) ==========
            case 0x71:  // Mutual Authentication with AES-256 KM (matches 0x66)
            {
                GLogN("\r\n========== Mutual Authentication Test (AES-256 KM) ==========\r\n");

                bool use_kauth = false;

                if (count >= 3)
                {
                    use_kauth = (strtol(gCliArgs[2], NULL, 0) != 0);
                }

                // Test random SR from VCI (16 bytes)
                static const uint8_t random_sr[16] = {
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
                };

                // AES-256 KM key (must match what's provisioned via 0x66)
                // NIST SP 800-38A F.1.5 - AES-256 ECB Test Key
                static const uint8_t test_km[32] = {
                    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
                    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
                    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
                    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
                };

                uint8_t random_cr[16] = {0};
                uint8_t signature_hsm[32] = {0};
                uint8_t key_id = 0U;
                bool auth_success = false;

                GLogN("Use Kauth: %s\r\n", use_kauth ? "YES" : "NO (use Km)");
                GLogN("KM Key (NIST AES-256): ");
                for (int i = 0; i < 32; i++) GLogN("%02X ", test_km[i]);
                GLogN("\r\n");
                GLogN("Random SR from VCI: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", random_sr[i]);
                GLogN("\r\n");

                // ===== STEP 1: Internal Auth (Get CR + Sign1) =====
                GLogN("\r\n[STEP 1] Internal Auth - Get CR + Sign1\r\n");
                HAL_StatusTypeDef status = hsm_internal_auth(use_kauth, random_sr, random_cr, signature_hsm, &key_id);

                if (status != HAL_OK)
                {
                    GLogE("[FAIL] Internal Auth failed, status: %d\r\n", status);
                    GLogN("=============================================\r\n");
                    break;
                }

                GLogN("[OK] Internal Auth Success\r\n");
                GLogN("Random CR from HSM: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", random_cr[i]);
                GLogN("\r\n");
                GLogN("Sign1 from HSM:\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", signature_hsm[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("Key ID: %d\r\n", key_id);

                // Initialize CMOX library
                if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
                {
                    GLogE("[FAIL] CMOX library initialization failed\r\n");
                    GLogN("=============================================\r\n");
                    break;
                }

                // ===== STEP 1.5: Verify Sign1 (SW vs HSM) =====
                GLogN("\r\n[STEP 1.5] Verify Sign1 - SW vs HSM\r\n");

                uint8_t sr_cr[32];
                memcpy(sr_cr, random_sr, 16);
                memcpy(sr_cr + 16, random_cr, 16);

                uint8_t sw_sign1[32] = {0};
                size_t sw_sign1_len = 0;

                cmox_mac_retval_t mac_ret = cmox_mac_compute(
                    CMOX_HMAC_SHA256_ALGO,
                    sr_cr, 32,
                    test_km, 32,    // Full 32-byte AES-256 key
                    NULL, 0,
                    sw_sign1, 32,
                    &sw_sign1_len
                );

                if (mac_ret != CMOX_MAC_SUCCESS)
                {
                    GLogE("[FAIL] SW HMAC Sign1 computation failed\r\n");
                    break;
                }

                GLogN("SW  Sign1: ");
                for (int i = 0; i < 32; i++) GLogN("%02X ", sw_sign1[i]);
                GLogN("\r\n");
                GLogN("HSM Sign1: ");
                for (int i = 0; i < 32; i++) GLogN("%02X ", signature_hsm[i]);
                GLogN("\r\n");

                if (memcmp(sw_sign1, signature_hsm, 32) == 0)
                {
                    GLogN("[OK] Sign1 MATCH!\r\n");
                }
                else
                {
                    GLogE("[FAIL] Sign1 MISMATCH! Check if 0x66 was used to import KM.\r\n");
                }

                // ===== STEP 2: Compute Sign2 =====
                GLogN("\r\n[STEP 2] Compute Sign2 = HMAC(CR||SR, Km)\r\n");

                uint8_t cr_sr[32];
                memcpy(cr_sr, random_cr, 16);
                memcpy(cr_sr + 16, random_sr, 16);

                uint8_t sign2[32] = {0};
                size_t sign2_len = 0;

                mac_ret = cmox_mac_compute(
                    CMOX_HMAC_SHA256_ALGO,
                    cr_sr, 32,
                    test_km, 32,    // Full 32-byte AES-256 key
                    NULL, 0,
                    sign2, 32,
                    &sign2_len
                );

                if (mac_ret != CMOX_MAC_SUCCESS)
                {
                    GLogE("[FAIL] SW HMAC Sign2 computation failed\r\n");
                    GLogN("=============================================\r\n");
                    break;
                }

                GLogN("[OK] Sign2 computed\r\n");
                GLogN("Sign2: ");
                for (int i = 0; i < 32; i++) GLogN("%02X ", sign2[i]);
                GLogN("\r\n");

                // ===== STEP 3: External Auth =====
                GLogN("\r\n[STEP 3] External Auth - Send Sign2\r\n");

                status = hsm_external_auth(use_kauth, sign2, &auth_success);

                if (status != HAL_OK)
                {
                    GLogE("[FAIL] External Auth failed, status: %d\r\n", status);
                    GLogN("[RECOVER] Sending W_CC to reset HSM state...\r\n");
                    spi_cmd_w_cc();
                }
                else
                {
                    if (auth_success)
                    {
                        GLogN("[OK] Mutual Authentication SUCCESS! (AES-256 KM)\r\n");
                        GLogN("Session Key Ks generated at slot #209\r\n");
                    }
                    else
                    {
                        GLogN("[WARN] External Auth completed but verification FAILED\r\n");
                    }
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== Kauth Import (0x73) ==========
            // Reference: HSM SPI Manual v4.1 - Section 5.18 Kauth Import[0x22]
            // - OP Code: 0x22
            // - Mode_1, Mode_2: reserved (0x00)
            // - Key ID: reserved (0x00), internally uses slot #103
            // - Message: 256 bytes RSA-encrypted Kauth key (EKauth)
            // - Kauth is decrypted using HSM's private key (Hkpr) and stored
            case 0x73:  // Kauth Key Import
            {
                GLogN("\r\n========== Kauth Import Test (OP 0x22) ==========\r\n");
                GLogN("[REF] HSM SPI Manual v4.1 - Section 5.18\r\n\r\n");

                // Original Kauth key (32 bytes) - This is what we want to import
                // In real scenario, this key is generated externally and RSA-encrypted
                static const uint8_t original_kauth[32] = {
                    // Test Kauth key: "KauthTestKey2024" + padding
                    0x4B, 0x61, 0x75, 0x74, 0x68, 0x54, 0x65, 0x73,  // "KauthTes"
                    0x74, 0x4B, 0x65, 0x79, 0x32, 0x30, 0x32, 0x34,  // "tKey2024"
                    0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18,  // Random padding
                    0x29, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x90   // Random padding
                };

                // Encrypted Kauth (256 bytes RSA-2048 encrypted)
                // In real usage: EKauth = RSA_ENCRYPT(Kauth, Hkpu)
                // This is dummy data simulating RSA-encrypted output
                static const uint8_t encrypted_kauth[256] = {
                    // Block 0: RSA PKCS#1 v1.5 padding pattern simulation
                    0x00, 0x02, 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56,
                    0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22, 0x33,
                    // Block 1-14: Simulated encrypted data
                    0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,
                    0xCC, 0xDD, 0xEE, 0xFF, 0x01, 0x23, 0x45, 0x67,
                    0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98,
                    0x76, 0x54, 0x32, 0x10, 0x0F, 0x1E, 0x2D, 0x3C,
                    0x4B, 0x5A, 0x69, 0x78, 0x87, 0x96, 0xA5, 0xB4,
                    0xC3, 0xD2, 0xE1, 0xF0, 0x00, 0x11, 0x22, 0x33,
                    0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,
                    0xCC, 0xDD, 0xEE, 0xFF, 0x12, 0x34, 0x56, 0x78,
                    // Block 15-30: More simulated encrypted data
                    0x9A, 0xBC, 0xDE, 0xF0, 0x13, 0x57, 0x9B, 0xDF,
                    0x02, 0x46, 0x8A, 0xCE, 0x15, 0x37, 0x59, 0x7B,
                    0x9D, 0xBF, 0xE1, 0x03, 0x25, 0x47, 0x69, 0x8B,
                    0xAD, 0xCF, 0xF1, 0x13, 0x35, 0x57, 0x79, 0x9B,
                    0xBD, 0xDF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
                    0xCD, 0xEF, 0x11, 0x33, 0x55, 0x77, 0x99, 0xBB,
                    0xDD, 0xFF, 0x21, 0x43, 0x65, 0x87, 0xA9, 0xCB,
                    0xED, 0x0F, 0x31, 0x53, 0x75, 0x97, 0xB9, 0xDB,
                    0xFD, 0x1F, 0x41, 0x63, 0x85, 0xA7, 0xC9, 0xEB,
                    0x0D, 0x2F, 0x51, 0x73, 0x95, 0xB7, 0xD9, 0xFB,
                    0x1D, 0x3F, 0x61, 0x83, 0xA5, 0xC7, 0xE9, 0x0B,
                    0x2D, 0x4F, 0x71, 0x93, 0xB5, 0xD7, 0xF9, 0x1B,
                    0x3D, 0x5F, 0x81, 0xA3, 0xC5, 0xE7, 0x09, 0x2B,
                    0x4D, 0x6F, 0x91, 0xB3, 0xD5, 0xF7, 0x19, 0x3B,
                    0x5D, 0x7F, 0xA1, 0xC3, 0xE5, 0x07, 0x29, 0x4B,
                    0x6D, 0x8F, 0xB1, 0xD3, 0xF5, 0x17, 0x39, 0x5B,
                    0x7D, 0x9F, 0xC1, 0xE3, 0x05, 0x27, 0x49, 0x6B,
                    0x8D, 0xAF, 0xD1, 0xF3, 0x15, 0x37, 0x59, 0x7B,
                    0x9D, 0xBF, 0xE1, 0x03, 0x25, 0x47, 0x69, 0x8B,
                    0xAD, 0xCF, 0xF1, 0x13, 0x35, 0x57, 0x79, 0x9B,
                    0xBD, 0xDF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
                    0xCD, 0xEF, 0x4B, 0x41, 0x55, 0x54, 0x48, 0x21   // "KAUTH!" marker
                };

                GLogN("[INFO] Kauth Import Packet Structure:\r\n");
                GLogN("  - OP Code: 0x22\r\n");
                GLogN("  - Mode_1: 0x00 (reserved)\r\n");
                GLogN("  - Mode_2: 0x00 (reserved)\r\n");
                GLogN("  - Key ID: 0x0000 (internal: slot #103)\r\n");
                GLogN("  - Context ID: 0x0000 (reserved)\r\n");
                GLogN("  - Message Length: 256 bytes\r\n");
                GLogN("  - Message: RSA-encrypted Kauth (EKauth)\r\n\r\n");

                GLogN("[NOTE] This test uses DUMMY encrypted data!\r\n");
                GLogN("       Real usage requires RSA encryption with HSM public key (Hkpu)\r\n");
                GLogN("       Formula: EKauth = RSA_ENCRYPT(Kauth, Hkpu)\r\n\r\n");

                GLogN("Original Kauth (32 bytes, plaintext reference):\r\n  ");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", original_kauth[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                }
                GLogN("\r\n");

                GLogN("Encrypted Kauth (256 bytes, first 32 shown):\r\n  ");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", encrypted_kauth[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                }
                GLogN("  ... (224 more bytes)\r\n\r\n");

                GLogN("[EXEC] Calling hsm_import_kauth_key()...\r\n");

                HAL_StatusTypeDef status = hsm_import_kauth_key(
                    encrypted_kauth,
                    256
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] Kauth Import Success\r\n");
                    GLogN("Kauth decrypted and stored at key slot #103\r\n");
                    GLogN("\r\n[NEXT] Use 'hsmtest 0x70 1' for Mutual Auth with Kauth\r\n");
                }
                else
                {
                    GLogE("[FAIL] Kauth Import failed, status: %d\r\n", status);
                    GLogE("[HINT] Ensure HSM has valid RSA private key (Hkpr) provisioned\r\n");
                    GLogE("[HINT] Use 'hsmtest 0x112 0' for RSA-encrypted import flow\r\n");
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== Oneway Auth (0x74) ==========
            case 0x74:  // One-way Authentication
            {
                GLogN("\r\n========== One-way Authentication Test ==========\r\n");

                // Test MAC value from VCI (16 bytes)
                static const uint8_t mac_value[16] = {
                    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
                    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99
                };
                uint8_t random_cr[16];
                bool auth_success = false;

                GLogN("MAC Value from VCI: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", mac_value[i]);
                GLogN("\r\n");
                GLogN("[NOTE] Need correct MAC = CMAC(Ks, SR)\r\n");

                HAL_StatusTypeDef status = hsm_oneway_authentication(
                    mac_value,
                    random_cr,
                    &auth_success
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] One-way Auth completed\r\n");
                    GLogN("Random CR from HSM: ");
                    for (int i = 0; i < 16; i++) GLogN("%02X ", random_cr[i]);
                    GLogN("\r\n");
                    GLogN("Auth Result: %s\r\n", auth_success ? "SUCCESS" : "FAILED");
                }
                else
                {
                    GLogE("[FAIL] One-way Auth failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== DFU (0x90) ==========
            case 0x90:  // Device Firmware Upgrade
            {
                GLogN("\r\n========== DFU Test ==========\r\n");

                uint8_t fw_type = 1U;  // HSE_FW

                if (count >= 3)
                {
                    fw_type = (uint8_t)strtol(gCliArgs[2], NULL, 0);
                }
				if (count >= 4)
                {
                    g_u32firmware_size = (U32)strtol(gCliArgs[3], NULL, 0);
                }

                GLogN("Firmware Type: %s\r\n", (fw_type == 1U) ? "HSE_FW" : "APP_FW");
                //GLogN("\r\n");
                //GLogN("[WARNING] DFU requires actual firmware binary file!\r\n");
                //GLogN("          This test only validates DFU protocol flow.\r\n");
                //GLogN("\r\n");
                //GLogN("Usage: Load firmware binary and call hsm_firmware_upgrade()\r\n");
                //GLogN("       fw_type: 1=HSE_FW, 2=APP_FW\r\n");
                //GLogN("\r\n");
                //GLogN("[SKIP] No firmware data provided - skipping actual DFU\r\n");
                GLogN("=============================================\r\n");
				hsm_firmware_upgrade(fw_type);
				
                break;
            }

            // ========== Set Complete (0x91) ==========
            case 0x91:  // Set Complete (Lifecycle Transition)
            {
                GLogN("\r\n========== Set Complete Test ==========\r\n");
                GLogN("\r\n");
                GLogN("[WARNING] This command triggers lifecycle transition!\r\n");
                GLogN("          IN_FIELD -> ACTIVE\r\n");
                GLogN("          HSM will REBOOT after this command.\r\n");
                GLogN("\r\n");

                if (count >= 3 && strcmp(gCliArgs[2], "CONFIRM") == 0)
                {
                    GLogN("Executing Set Complete...\r\n");

                    HAL_StatusTypeDef status = hsm_set_complete();

                    if (status == HAL_OK)
                    {
                        GLogN("[OK] Set Complete Success\r\n");
                        GLogN("HSM is rebooting...\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] Set Complete failed, status: %d\r\n", status);
                    }
                }
                else
                {
                    GLogN("To execute, run: hsmtest 0x91 CONFIRM\r\n");
                    GLogN("[SKIP] Confirmation required\r\n");
                }
                GLogN("=============================================\r\n");
                break;
            }
			// ========== DFU (0x90) ==========
            case 0x92:  // Device Firmware Upgrade
            {
                GLogN("\r\n========== DFU Test ==========\r\n");



                GLogN("Firmware Type: HSE_FW & APP_FW\r\n");
                HSM_VersionInfo_t version_info;
				U16 HSM_Current_Version = 0;
		        if (hsm_get_version_info(&version_info) == HAL_OK)
		        {
		            GLogN("HSM Version - Host: %d.%d.%d, HSE: %d.%d.%d\r\n",
		                  version_info.host_major, version_info.host_minor, version_info.host_patch,
		                  version_info.hse_major, version_info.hse_minor, version_info.hse_patch);

					if(0 < ((version_info.host_major + version_info.host_minor + version_info.host_patch
															+ version_info.hse_major + version_info.hse_minor + version_info.hse_patch)-260))
					{
						HSM_Current_Version = ((version_info.host_major + version_info.host_minor + version_info.host_patch
													+ version_info.hse_major + version_info.hse_minor + version_info.hse_patch)-260);
					}
					else
					{
						HSM_Current_Version = 0;
					}
					
		        }
		        GLogN("HSM_Current_Version:%d\r\n",HSM_Current_Version);

				if(HAL_OK != hsm_firmware_upgrade(HSM_DFU_TYPE_APP_FW))
				{
					//Save_HSM_UpdateFailCount();
				}
				else
	            {
	            	if((version_info.hse_major<2)//newest version 2.55.0
	            		||((version_info.hse_major<=2)&&(version_info.hse_minor<55))
	            		//||((version_info.hse_major<=2)&&(version_info.hse_minor<=55)&&(version_info.hse_minor<0))
	            		)
	            	{
		            	if(HAL_OK != hsm_firmware_upgrade(HSM_DFU_TYPE_HSE_FW))
						{
							//Save_HSM_UpdateFailCount();
						}
						else
			            {
			            	
			                //f_unlink(HSM_UPDATE_FAIL_INFO);
			            }
	            	}
					else
					{
	                	//f_unlink(HSM_UPDATE_FAIL_INFO);
					}
	            }
                GLogN("=============================================\r\n");
				
                break;
            }

            // ========== RSA-2048 Public Key Import Test (0xA0) ==========
            case 0xA0:
            {
                GLogN("\r\n========== RSA-2048 Public Key Import Test ==========\r\n");

                // Hardcoded RSA-2048 test key
                static const uint8_t test_modulus[256] = {
                    0x5F, 0x7C, 0x8E, 0x1B, 0x2C, 0x3B, 0x0B, 0x27, 0x6C, 0x9B, 0xC4, 0xB8, 0x0C, 0xF2, 0x16, 0x0C,
                    0x9C, 0x05, 0x5D, 0xA0, 0x7F, 0xDE, 0x6C, 0x28, 0xE9, 0x15, 0xAC, 0x39, 0x08, 0x7D, 0xF6, 0xD4,
                    0x01, 0x61, 0xB4, 0x34, 0xA6, 0x66, 0x3D, 0xC8, 0x10, 0xE7, 0x48, 0x68, 0x6A, 0xB4, 0xB8, 0x21,
                    0xAA, 0xC3, 0x41, 0x10, 0x48, 0xD8, 0xF0, 0x06, 0xF2, 0xE1, 0xD0, 0xD7, 0x7D, 0xC0, 0xE6, 0x74,
                    0x27, 0x8D, 0x9B, 0x60, 0xD1, 0x37, 0x84, 0xA2, 0xF5, 0xB3, 0xBF, 0x7B, 0xE8, 0x65, 0xD8, 0x71,
                    0x65, 0x57, 0x56, 0xD9, 0xCC, 0x11, 0xC9, 0xD4, 0x08, 0x74, 0x1A, 0x27, 0x9A, 0xBB, 0x6B, 0xC7,
                    0x9D, 0xA9, 0x46, 0xB1, 0x82, 0x89, 0xBA, 0xA1, 0x40, 0xA0, 0x4E, 0x5C, 0xBF, 0xAF, 0x2D, 0x2C,
                    0x3D, 0xE8, 0x8C, 0x96, 0xE6, 0xEC, 0x61, 0xFA, 0x4D, 0x1C, 0x1A, 0x50, 0xCA, 0xAA, 0x00, 0xB8,
                    0x40, 0x84, 0xC8, 0x8A, 0x6F, 0x18, 0x4D, 0x28, 0x8D, 0x28, 0xDA, 0x8C, 0x31, 0xB8, 0xE5, 0x4C,
                    0x84, 0x50, 0x10, 0x9C, 0xC8, 0x8F, 0x25, 0xB2, 0x4A, 0xB1, 0x62, 0x05, 0xF4, 0xCB, 0xFF, 0x7A,
                    0x70, 0xC4, 0xBB, 0xBE, 0x8C, 0x19, 0x66, 0xF2, 0x65, 0xBE, 0x1F, 0xBE, 0xDB, 0x7E, 0x8E, 0x66,
                    0x40, 0xA4, 0xCF, 0xF9, 0x01, 0xA5, 0xDF, 0x40, 0x8E, 0xAA, 0xF7, 0xAE, 0x82, 0x75, 0x54, 0x8E,
                    0x2B, 0xEC, 0xA8, 0x44, 0x67, 0x7B, 0x48, 0x61, 0x3A, 0x06, 0x12, 0x31, 0x62, 0xEF, 0xBF, 0xC0,
                    0x0A, 0x79, 0x4F, 0x55, 0xFC, 0x76, 0x22, 0xFC, 0xF2, 0xD1, 0x17, 0xAB, 0xFE, 0xA1, 0xE1, 0xD3,
                    0xA2, 0x1B, 0xE2, 0xB7, 0x00, 0x21, 0x75, 0x39, 0xD1, 0x1D, 0x6E, 0x53, 0x7E, 0x28, 0xEE, 0xF2,
                    0x7E, 0xFC, 0xAE, 0xEA, 0xF1, 0x17, 0xFA, 0xFD, 0x51, 0xB1, 0x7C, 0x5B, 0xB8, 0x18, 0xC2, 0x77
                };

                static const uint8_t test_private_exp[256] = {
                    0x51, 0x80, 0x06, 0xDA, 0x93, 0xFA, 0xC2, 0x89, 0xF2, 0xC3, 0xA1, 0xBE, 0xCF, 0x1A, 0x19, 0x79,
                    0x70, 0xD0, 0xEC, 0x5F, 0xC2, 0xF0, 0x31, 0x6E, 0x69, 0xA6, 0x46, 0x87, 0xED, 0xF2, 0x3C, 0x93,
                    0xEC, 0xC7, 0xE3, 0x8A, 0xC5, 0x26, 0x30, 0x28, 0xC3, 0x95, 0x40, 0xC1, 0x9C, 0x27, 0x18, 0x46,
                    0xDD, 0xFC, 0x01, 0x7F, 0x3B, 0x7F, 0xD5, 0x3D, 0xDB, 0x27, 0xE9, 0x8C, 0x48, 0x8F, 0x04, 0x6B,
                    0x07, 0x07, 0xD1, 0xCA, 0xFF, 0x9E, 0x45, 0x48, 0xE5, 0xDB, 0x97, 0x8D, 0x6C, 0x96, 0x22, 0xCB,
                    0x8F, 0xF5, 0xE4, 0x89, 0x2B, 0x83, 0xEF, 0x32, 0xC9, 0x1A, 0x05, 0xB2, 0xEA, 0x56, 0x40, 0xCC,
                    0xB1, 0x7B, 0xAF, 0x93, 0x9F, 0x06, 0xEE, 0x14, 0x9F, 0xEA, 0x86, 0x23, 0x95, 0xBB, 0xBE, 0x9D,
                    0x21, 0x23, 0x01, 0x47, 0xA6, 0x5C, 0xAD, 0x87, 0xD3, 0x5B, 0xBC, 0xC3, 0x49, 0x89, 0x49, 0xCE,
                    0x40, 0x4F, 0x1C, 0x2F, 0x87, 0x83, 0x14, 0x38, 0x31, 0x7C, 0xA9, 0x67, 0xCB, 0x54, 0xB4, 0x52,
                    0xB9, 0xC8, 0xF1, 0xBA, 0xB2, 0x77, 0x71, 0xBC, 0x2C, 0xC1, 0x95, 0xD9, 0xB2, 0x9F, 0x8F, 0x24,
                    0xF7, 0x0B, 0x9B, 0xB6, 0x47, 0x9F, 0x2E, 0x17, 0x04, 0x0E, 0xEC, 0xC7, 0x62, 0xEA, 0x84, 0x9F,
                    0x8A, 0x6D, 0x74, 0x15, 0xF8, 0x40, 0x7D, 0x7D, 0xC7, 0xC9, 0xB0, 0x45, 0x65, 0xFA, 0x70, 0x94,
                    0x5A, 0x52, 0xAA, 0x3F, 0xD8, 0xE2, 0xEA, 0x08, 0x4A, 0x77, 0x21, 0x3F, 0x94, 0x73, 0xC3, 0x20,
                    0xF4, 0xC9, 0x1F, 0x4E, 0x42, 0xD5, 0x3D, 0xF3, 0x22, 0x13, 0x94, 0x91, 0x09, 0xDF, 0xE4, 0x36,
                    0xA7, 0x0E, 0xD3, 0xF4, 0x7A, 0x52, 0xBE, 0x86, 0xAB, 0xC7, 0x96, 0xD2, 0x7C, 0xA5, 0xC4, 0x3B,
                    0xF3, 0x15, 0x78, 0x02, 0x10, 0x53, 0x00, 0xE6, 0x83, 0xFF, 0xA9, 0x6B, 0xA7, 0x41, 0x31, 0x67
                };

                // Public exponent = 65537 (0x010001) - 3 bytes
                static const uint8_t test_public_exp[3] = {0x01, 0x00, 0x01};

                // NOTE: RSA Public-only import requires TEMP RSA Public slots (#241-242)
                // Slot #149 is for RSA key pairs (public+private), NOT public-only!
                uint16_t key_id = HSM_KEY_TEMP_RSA_PUB_241;  // Default: 241 (TEMP RSA Public)
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: %d (TEMP RSA Public slots: 241-242)\r\n", key_id);

                HSM_RSAKey_t rsa_key;
                memset(&rsa_key, 0, sizeof(rsa_key));

                rsa_key.modulus_length = 256;
                memcpy(rsa_key.modulus, test_modulus, 256);
                rsa_key.public_exp_size = 3;
                memcpy(rsa_key.public_exp, test_public_exp, 3);
                memcpy(rsa_key.private_exp, test_private_exp, 256);

                // NOTE: Manual 5.6.4.1.1 - Import RSA Data structure:
                // modulus_len(2) + modulus(256) + pubSize(2) + pubExp(0~256) = MAX 516 bytes
                // NO private exponent in plaintext import! Use public_only=true
                HAL_StatusTypeDef status = hsm_import_rsa_key(key_id, &rsa_key, true, false, false, HSM_RSA_2048);

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA-2048 Public Key Import Success\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA-2048 Public Key Import failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== ED25519 Key Import Test (0xA1) ==========
            case 0xA1:
            {
                GLogN("\r\n========== ED25519 Key Import Test ==========\r\n");

                // ED25519 test key - Real test vector
                // PrivateKey (Seed): f8b56168c770658e9d7328e40e80b50fab180a5fc3a1560073aeccc1ae22d9f0
                static const uint8_t test_ed_private[32] = {
                    0xf8, 0xb5, 0x61, 0x68, 0xc7, 0x70, 0x65, 0x8e,
                    0x9d, 0x73, 0x28, 0xe4, 0x0e, 0x80, 0xb5, 0x0f,
                    0xab, 0x18, 0x0a, 0x5f, 0xc3, 0xa1, 0x56, 0x00,
                    0x73, 0xae, 0xcc, 0xc1, 0xae, 0x22, 0xd9, 0xf0
                };

                // PublicKey (from Output Key last 32 bytes): 564007A4603B8AE0795D5061EAD4177A8B1CE17957438F4F406110B290E37C09
                static const uint8_t test_ed_public[32] = {
                    0x56, 0x40, 0x07, 0xA4, 0x60, 0x3B, 0x8A, 0xE0,
                    0x79, 0x5D, 0x50, 0x61, 0xEA, 0xD4, 0x17, 0x7A,
                    0x8B, 0x1C, 0xE1, 0x79, 0x57, 0x43, 0x8F, 0x4F,
                    0x40, 0x61, 0x10, 0xB2, 0x90, 0xE3, 0x7C, 0x09
                };

                uint16_t key_id = 261;  // TEMP ECC slot
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Private Key (32B): ");
                for (int i = 0; i < 32; i++) GLogN("%02X", test_ed_private[i]);
                GLogN("\r\n");
                GLogN("Public Key (32B): ");
                for (int i = 0; i < 32; i++) GLogN("%02X", test_ed_public[i]);
                GLogN("\r\n");

                // TODO: Call hsm_import_ed_key() when implemented
                GLogN("[INFO] ED25519 Import not yet implemented\r\n");
                GLogN("=============================================\r\n");
                break;
            }

            // ========== HMAC-SHA256 Key Import Test (0xA2) ==========
            case 0xA2:
            {
                GLogN("\r\n========== HMAC-SHA256 Key Test ==========\r\n");

                // Hardcoded test: HMAC-SHA256(seed, secret)
                // Seed: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF (16 bytes)
                // Secret: 0x00000000000000000000000000000000 (16 bytes)
                // Expected Output: 293531BBB4268FD5145915D35FCEEE6E4182241CC5EE9A533944F8BC8C30AAF2
                static const uint8_t test_seed[16] = {
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
                };

                static const uint8_t test_secret[16] = {
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                };

                // Expected HMAC output for verification
                static const uint8_t expected_hmac[32] = {
                    0x29, 0x35, 0x31, 0xBB, 0xB4, 0x26, 0x8F, 0xD5,
                    0x14, 0x59, 0x15, 0xD3, 0x5F, 0xCE, 0xEE, 0x6E,
                    0x41, 0x82, 0x24, 0x1C, 0xC5, 0xEE, 0x9A, 0x53,
                    0x39, 0x44, 0xF8, 0xBC, 0x8C, 0x30, 0xAA, 0xF2
                };

                uint16_t key_id = HSM_KEY_HMAC_121;  // Default: 121
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Seed (16B): ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", test_seed[i]);
                GLogN("\r\n");
                GLogN("Secret (16B): ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", test_secret[i]);
                GLogN("\r\n");

                // Generate HMAC using HSM
                uint8_t hmac_output[32];
                HAL_StatusTypeDef status = hsm_generate_hmac_sha256(key_id, test_seed, 16, hmac_output);

                if (status == HAL_OK)
                {
                    GLogN("[OK] HMAC-SHA256 Success\r\n");
                    GLogN("Output Key (32B): ");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", hmac_output[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n                  ");
                    }
                    GLogN("\r\n");

                    // Verify against expected value
                    if (memcmp(hmac_output, expected_hmac, 32) == 0)
                    {
                        GLogN("[VERIFY] Output matches expected value!\r\n");
                    }
                    else
                    {
                        GLogN("[VERIFY] Output differs from expected:\r\n");
                        GLogN("Expected (32B): ");
                        for (int i = 0; i < 32; i++)
                        {
                            GLogN("%02X ", expected_hmac[i]);
                            if ((i + 1) % 16 == 0) GLogN("\r\n                ");
                        }
                        GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] HMAC-SHA256 failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== ECU Code Key Plaintext Import Test (0xA3) ==========
            case 0xA3:
            {
                GLogN("\r\n========== ECU Code Key Plaintext Import Test (0x0115) ==========\r\n");

                // Test AES-128 key for ECU Code (plaintext injection)
                // This bypasses CODE_Key decryption and directly imports to HSM
                static const uint8_t test_ecu_code_key[16] = {
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
                };

                uint16_t key_id = HSM_KEY_ECU_CODE;  // Default: 101
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: %d (HSM_KEY_ECU_CODE)\r\n", key_id);
                GLogN("Key Size: 16 bytes (AES-128)\r\n");
                GLogN("Key Type: Plaintext (bypassing CODE_Key decryption)\r\n");
                GLogN("Test Key Data: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", test_ecu_code_key[i]);
                GLogN("\r\n");

                // Import plaintext AES key to HSM
                HAL_StatusTypeDef status = hsm_import_aes_key(
                    key_id,
                    test_ecu_code_key,
                    HSM_AES_128,  // AES-128 (0x01, not 16!)
                    false,        // encrypted = false (plaintext)
                    false         // lock_key = false
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] ECU Code Key import success!\r\n");
                    GLogN("  -> Key stored in slot #%d\r\n", key_id);
                }
                else
                {
                    GLogE("[FAIL] ECU Code Key import failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== ECU-Code Key Verification Suite (0xA4 ~ 0xA6) ==========
            // Based on provided test vectors:
            // Key: 56 43 49 33 21 48 73 6D 40 41 65 73 23 4B 65 79
            // IV:  47 69 74 56 43 49 33 48 53 4D 41 45 53 5F 49 56
            // Original: 11 22 33 44 55 66 77 88
            // Encrypted: 3c 0f 2f 5f 0c 35 9b 95 b0 2c 39 ee d3 c2 99 c0

            // ========== Step 1: Import Real ECU-Code Key (0xA4) ==========
            case 0xA4:
            {
                GLogN("\r\n========== [Step 1] Import Real ECU-Code Key ==========\r\n");

                // Real ECU-Code Key from test vector
                // "VCI3!Hsm@Aes#Key" in ASCII
                static const uint8_t real_ecu_code_key[16] = {
                    0x56, 0x43, 0x49, 0x33, 0x21, 0x48, 0x73, 0x6D,
                    0x40, 0x41, 0x65, 0x73, 0x23, 0x4B, 0x65, 0x79
                };

                uint16_t key_id = HSM_KEY_ECU_CODE;  // 101
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: %d (HSM_KEY_ECU_CODE)\r\n", key_id);
                GLogN("Algorithm: AES-128\r\n");
                GLogN("Key (hex): ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", real_ecu_code_key[i]);
                GLogN("\r\n");
                GLogN("Key (ASCII): VCI3!Hsm@Aes#Key\r\n");

                HAL_StatusTypeDef status = hsm_import_aes_key(
                    key_id,
                    real_ecu_code_key,
                    HSM_AES_128,
                    false,  // plaintext
                    false   // no lock
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] Real ECU-Code Key imported to slot #%d\r\n", key_id);
                    GLogN("  -> Next: Run 'hsmtest 0xA5' to test decryption\r\n");
                }
                else
                {
                    GLogE("[FAIL] ECU-Code Key import failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== Step 2: AES-CTR Decrypt Test (0xA5) ==========
            case 0xA5:
            {
                GLogN("\r\n========== [Step 2] AES-CTR Decrypt Test ==========\r\n");

                // IV for AES-CTR: "GitVCI3HSMAES_IV" in ASCII
                static const uint8_t test_iv[16] = {
                    0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48,
                    0x53, 0x4D, 0x41, 0x45, 0x53, 0x5F, 0x49, 0x56
                };

                // Encrypted data (16 bytes)
                static const uint8_t encrypted_data[16] = {
                    0x3c, 0x0f, 0x2f, 0x5f, 0x0c, 0x35, 0x9b, 0x95,
                    0xb0, 0x2c, 0x39, 0xee, 0xd3, 0xc2, 0x99, 0xc0
                };

                // Expected plaintext after decryption:
                // Original(8) + IV_first_8(8) = 16 bytes
                // 11 22 33 44 55 66 77 88 + 47 69 74 56 43 49 33 48
                static const uint8_t expected_plaintext[16] = {
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                    0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48
                };

                uint16_t key_id = HSM_KEY_ECU_CODE;  // 101
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Mode: AES-CTR (Counter Mode)\r\n");
                GLogN("IV (hex): ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", test_iv[i]);
                GLogN("\r\n");
                GLogN("IV (ASCII): GitVCI3HSMAES_IV\r\n");
                GLogN("Encrypted (16B): ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", encrypted_data[i]);
                GLogN("\r\n");

                // Perform AES-CTR decryption using hsm_aes_decrypt
                uint8_t decrypted[16];
                HAL_StatusTypeDef status = hsm_aes_decrypt(
                    key_id,
                    HSM_AES_128,        // key size
                    HSM_AES_CTR,        // mode = CTR (0x02)
                    test_iv,            // IV
                    encrypted_data,     // ciphertext
                    16,                 // length
                    decrypted           // plaintext output
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] AES-CTR Decrypt Success\r\n");
                    GLogN("Decrypted (16B): ");
                    for (int i = 0; i < 16; i++) GLogN("%02X ", decrypted[i]);
                    GLogN("\r\n");

                    // Verify against expected
                    if (memcmp(decrypted, expected_plaintext, 16) == 0)
                    {
                        GLogN("[VERIFY] Decryption matches expected!\r\n");
                        GLogN("  Original data (8B): ");
                        for (int i = 0; i < 8; i++) GLogN("%02X ", decrypted[i]);
                        GLogN("\r\n");
                        GLogN("  Padding (IV first 8B): ");
                        for (int i = 8; i < 16; i++) GLogN("%02X ", decrypted[i]);
                        GLogN("\r\n");
                    }
                    else
                    {
                        GLogN("[VERIFY FAIL] Decryption differs from expected!\r\n");
                        GLogN("Expected (16B): ");
                        for (int i = 0; i < 16; i++) GLogN("%02X ", expected_plaintext[i]);
                        GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] AES-CTR Decrypt failed, status: %d\r\n", status);
                    GLogN("  -> Make sure ECU-Code Key is imported first (hsmtest 0xA4)\r\n");
                }
                GLogN("=============================================\r\n");
                break;
            }

            // ========== Step 3: Full Verification Flow (0xA6) ==========
            case 0xA6:
            {
                GLogN("\r\n========== [Step 3] Full ECU-Code Verification ==========\r\n");
                GLogN("Running complete verification sequence...\r\n\r\n");

                // ---- Part A: Import Key ----
                static const uint8_t real_ecu_code_key[16] = {
                    0x56, 0x43, 0x49, 0x33, 0x21, 0x48, 0x73, 0x6D,
                    0x40, 0x41, 0x65, 0x73, 0x23, 0x4B, 0x65, 0x79
                };

                GLogN("[A] Importing ECU-Code Key to slot #101...\r\n");
                HAL_StatusTypeDef status = hsm_import_aes_key(
                    HSM_KEY_ECU_CODE,
                    real_ecu_code_key,
                    HSM_AES_128,
                    false,
                    false
                );

                if (status != HAL_OK)
                {
                    GLogE("[A] FAIL - Key import failed: %d\r\n", status);
                    break;
                }
                GLogN("[A] OK - Key imported\r\n\r\n");

                // ---- Part B: Decrypt ----
                static const uint8_t test_iv[16] = {
                    0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48,
                    0x53, 0x4D, 0x41, 0x45, 0x53, 0x5F, 0x49, 0x56
                };

                static const uint8_t encrypted_data[16] = {
                    0x3c, 0x0f, 0x2f, 0x5f, 0x0c, 0x35, 0x9b, 0x95,
                    0xb0, 0x2c, 0x39, 0xee, 0xd3, 0xc2, 0x99, 0xc0
                };

                static const uint8_t expected_original[8] = {
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
                };

                GLogN("[B] Decrypting with AES-CTR...\r\n");
                uint8_t decrypted[16];
                status = hsm_aes_decrypt(
                    HSM_KEY_ECU_CODE,
                    HSM_AES_128,
                    HSM_AES_CTR,
                    test_iv,
                    encrypted_data,
                    16,
                    decrypted
                );

                if (status != HAL_OK)
                {
                    GLogE("[B] FAIL - Decryption failed: %d\r\n", status);
                    break;
                }
                GLogN("[B] OK - Decrypted\r\n\r\n");

                // ---- Part C: Verify Original Data ----
                GLogN("[C] Verifying decrypted data...\r\n");
                GLogN("    Decrypted first 8 bytes: ");
                for (int i = 0; i < 8; i++) GLogN("%02X ", decrypted[i]);
                GLogN("\r\n");
                GLogN("    Expected original:       ");
                for (int i = 0; i < 8; i++) GLogN("%02X ", expected_original[i]);
                GLogN("\r\n");

                if (memcmp(decrypted, expected_original, 8) == 0)
                {
                    GLogN("[C] OK - Original data matches!\r\n\r\n");

                    // ---- Part D: Verify Padding ----
                    GLogN("[D] Verifying padding (IV first 8 bytes)...\r\n");
                    GLogN("    Decrypted padding: ");
                    for (int i = 8; i < 16; i++) GLogN("%02X ", decrypted[i]);
                    GLogN("\r\n");
                    GLogN("    Expected (IV[0:7]): ");
                    for (int i = 0; i < 8; i++) GLogN("%02X ", test_iv[i]);
                    GLogN("\r\n");

                    if (memcmp(&decrypted[8], test_iv, 8) == 0)
                    {
                        GLogN("[D] OK - Padding matches!\r\n\r\n");
                        GLogN("========================================\r\n");
                        GLogN("  ALL VERIFICATIONS PASSED!\r\n");
                        GLogN("========================================\r\n");
                    }
                    else
                    {
                        GLogE("[D] FAIL - Padding mismatch!\r\n");
                    }
                }
                else
                {
                    GLogE("[C] FAIL - Original data mismatch!\r\n");
                }
                GLogN("=============================================\r\n");
                break;
            }

            case 0xA7:
            {
                // AES-ECB Test (Debug) - No IV needed
                GLogN("\r\n========== [DEBUG] AES-ECB Encrypt/Decrypt Test ==========\r\n");

                // Test key: simple 16-byte key
                static const uint8_t test_key[16] = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
                };

                // Test plaintext: 16 bytes
                static const uint8_t test_plaintext[16] = {
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
                };

                uint16_t key_id = 241;  // TEMP AES slot
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key Slot: %d (TEMP)\r\n", key_id);
                GLogN("Mode: AES-ECB (no IV)\r\n\r\n");

                // Step 1: Import test key
                GLogN("[1] Importing test key...\r\n");
                HAL_StatusTypeDef status = hsm_import_aes_key(key_id, test_key, HSM_AES_128, false, false);
                if (status != HAL_OK)
                {
                    GLogE("[1] FAIL - Key import failed: %d\r\n", status);
                    break;
                }
                GLogN("[1] OK - Key imported\r\n\r\n");

                // Step 2: Encrypt
                GLogN("[2] Encrypting with AES-ECB...\r\n");
                GLogN("    Plaintext: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", test_plaintext[i]);
                GLogN("\r\n");

                uint8_t ciphertext[16];
                status = hsm_aes_encrypt(key_id, HSM_AES_128, HSM_AES_ECB, NULL, test_plaintext, 16, ciphertext);
                if (status != HAL_OK)
                {
                    GLogE("[2] FAIL - Encrypt failed: %d\r\n", status);
                    break;
                }
                GLogN("[2] OK - Encrypted\r\n");
                GLogN("    Ciphertext: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", ciphertext[i]);
                GLogN("\r\n\r\n");

                // Step 3: Decrypt
                GLogN("[3] Decrypting with AES-ECB...\r\n");
                uint8_t decrypted[16];
                status = hsm_aes_decrypt(key_id, HSM_AES_128, HSM_AES_ECB, NULL, ciphertext, 16, decrypted);
                if (status != HAL_OK)
                {
                    GLogE("[3] FAIL - Decrypt failed: %d\r\n", status);
                    break;
                }
                GLogN("[3] OK - Decrypted\r\n");
                GLogN("    Decrypted: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", decrypted[i]);
                GLogN("\r\n\r\n");

                // Step 4: Verify
                if (memcmp(test_plaintext, decrypted, 16) == 0)
                {
                    GLogN("========================================\r\n");
                    GLogN("  AES-ECB ROUND-TRIP SUCCESS!\r\n");
                    GLogN("========================================\r\n");
                }
                else
                {
                    GLogE("[VERIFY FAIL] Decrypted != Original!\r\n");
                }
                break;
            }

            case 0xA8:
            {
                GLogN("\r\n========== HSM_READ_PUBKEY (0x0124) Test ==========\r\n");
                GLogN("Testing hsm_export_public_key() for NEW HSM...\r\n\r\n");

                uint8_t pub_key_buffer[270];  // RSA-2048 public key
                U16 pub_key_len = 0;

                // Test export from HSM_PUBKEY (0x00)
                uint16_t key_id = 150;
                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("[1] Exporting RSA public key from slot %d...\r\n", key_id);
                HAL_StatusTypeDef status = hsm_export_public_key(key_id, HSM_ALG_RSA, pub_key_buffer, &pub_key_len);

                if (status != HAL_OK)
                {
                    GLogE("[1] FAIL - Export failed: %d\r\n", status);
                    GLogE("    Make sure RSA key pair exists at slot %d\r\n", key_id);
                    break;
                }

                GLogN("[1] OK - Public key exported (%d bytes)\r\n\r\n", pub_key_len);

                // Display key info (Manual 5.6.5.1.1 Export RSA Key structure)
                // Response Code excluded by spi_cmd_r_cmd
                // [0-1] modulus_len, [2-257] modulus, [258-259] pubSize, [260+] pubExp
                GLogN("[2] Public Key Structure:\r\n");

                if (pub_key_len >= 260)
                {
                    uint16_t mod_len = (uint16_t)pub_key_buffer[0] | ((uint16_t)pub_key_buffer[1] << 8);
                    uint16_t pub_size = (uint16_t)pub_key_buffer[258] | ((uint16_t)pub_key_buffer[259] << 8);
                    GLogN("    Modulus Length: %d bytes\r\n", mod_len);
                    GLogN("    Modulus (first 16 bytes): ");
                    for (int i = 2; i < 18 && i < pub_key_len; i++)
                        GLogN("%02X ", pub_key_buffer[i]);
                    GLogN("...\r\n");
                    GLogN("    Modulus (last 16 bytes):  ");
                    for (int i = 258 - 16; i < 258; i++)
                        GLogN("%02X ", pub_key_buffer[i]);
                    GLogN("\r\n");
                    GLogN("    PubExp Size: %d bytes\r\n", pub_size);
                    GLogN("    PubExp: ");
                    for (int i = 0; i < pub_size && i < 8; i++)
                        GLogN("%02X ", pub_key_buffer[260 + i]);
                    GLogN("\r\n");
                }
                else
                {
                    GLogN("    Raw (first 32 bytes): ");
                    for (int i = 0; i < 32 && i < pub_key_len; i++)
                        GLogN("%02X ", pub_key_buffer[i]);
                    GLogN("...\r\n");
                }

                GLogN("\r\n========================================\r\n");
                GLogN("  HSM_READ_PUBKEY TEST PASSED!\r\n");
                GLogN("========================================\r\n");
                break;
            }

            case 0xA9:
            {
                GLogN("\r\n========== HSM_READ_RANDKEY (0x0126) Test ==========\r\n");
                GLogN("Testing RSA decrypt + AES key import for NEW HSM...\r\n\r\n");

                // Test Session Key: 16 bytes AES-128
                static const uint8_t test_session_key[16] = {
                    0x54, 0x45, 0x53, 0x54,  // "TEST"
                    0x53, 0x45, 0x53, 0x53,  // "SESS"
                    0x49, 0x4F, 0x4E, 0x4B,  // "IONK"
                    0x45, 0x59, 0x21, 0x21   // "EY!!"
                };

                // Step 1: Check if RSA key exists
                GLogN("[1] Checking RSA key at slot %d...\r\n", HSM_PUBKEY);
                uint8_t pub_key[270];
                U16 pub_key_len = 0;

                HAL_StatusTypeDef status = hsm_export_public_key(HSM_PUBKEY, HSM_ALG_RSA, pub_key, &pub_key_len);
                if (status != HAL_OK)
                {
                    GLogE("[1] FAIL - No RSA key at slot %d\r\n", HSM_PUBKEY);
                    GLogE("    Run 'hsmtest 0x05' first to generate RSA key pair\r\n");
                    break;
                }
                GLogN("[1] OK - RSA key exists (%d bytes)\r\n\r\n", pub_key_len);

                // Step 2: Encrypt test key with RSA public key
                GLogN("[2] Encrypting test session key with RSA...\r\n");
                GLogN("    Session Key: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", test_session_key[i]);
                GLogN("\r\n");

                uint8_t encrypted_key[256];
                status = hsm_rsa_encrypt(HSM_PUBKEY, test_session_key, 16, encrypted_key);
                if (status != HAL_OK)
                {
                    GLogE("[2] FAIL - RSA encrypt failed: %d\r\n", status);
                    break;
                }
                GLogN("[2] OK - Key encrypted (256 bytes)\r\n");
                GLogN("    Encrypted (first 16): ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", encrypted_key[i]);
                GLogN("...\r\n\r\n");

                // Step 3: Decrypt with RSA private key
                GLogN("[3] Decrypting with RSA private key...\r\n");
                uint8_t decrypted_key[256];
                U16 decrypted_len = 0;

                status = hsm_rsa_decrypt(HSM_PUBKEY, encrypted_key, decrypted_key, &decrypted_len);
                if (status != HAL_OK)
                {
                    GLogE("[3] FAIL - RSA decrypt failed: %d\r\n", status);
                    break;
                }
                GLogN("[3] OK - Key decrypted (%d bytes)\r\n", decrypted_len);
                GLogN("    Decrypted Key: ");
                for (int i = 0; i < decrypted_len && i < 16; i++) GLogN("%02X ", decrypted_key[i]);
                GLogN("\r\n\r\n");

                // Step 4: Verify decrypted matches original
                GLogN("[4] Verifying decrypted key matches original...\r\n");
                if (decrypted_len >= 16 && memcmp(test_session_key, decrypted_key, 16) == 0)
                {
                    GLogN("[4] OK - Keys match!\r\n\r\n");
                }
                else
                {
                    GLogE("[4] FAIL - Keys don't match!\r\n");
                    break;
                }

                // Step 5: Import as AES session key
                GLogN("[5] Importing decrypted key as AES Session Key (slot %d)...\r\n", HSM_KEY_SESSION);
                status = hsm_import_aes_key(HSM_KEY_SESSION, decrypted_key, HSM_AES_128, false, false);
                if (status != HAL_OK)
                {
                    GLogE("[5] FAIL - AES key import failed: %d\r\n", status);
                    break;
                }
                GLogN("[5] OK - Session key imported to slot %d\r\n\r\n", HSM_KEY_SESSION);

                // Step 6: Verify by encrypting test data
                GLogN("[6] Verifying Session Key by AES encrypt/decrypt...\r\n");
                static const uint8_t test_data[16] = {
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
                };
                uint8_t encrypted[16], decrypted[16];

                status = hsm_aes_encrypt(HSM_KEY_SESSION, HSM_AES_128, HSM_AES_ECB, NULL, test_data, 16, encrypted);
                if (status != HAL_OK)
                {
                    GLogE("[6] FAIL - AES encrypt failed: %d\r\n", status);
                    break;
                }

                status = hsm_aes_decrypt(HSM_KEY_SESSION, HSM_AES_128, HSM_AES_ECB, NULL, encrypted, 16, decrypted);
                if (status != HAL_OK)
                {
                    GLogE("[6] FAIL - AES decrypt failed: %d\r\n", status);
                    break;
                }

                if (memcmp(test_data, decrypted, 16) == 0)
                {
                    GLogN("[6] OK - AES round-trip successful!\r\n\r\n");
                    GLogN("========================================\r\n");
                    GLogN("  HSM_READ_RANDKEY TEST PASSED!\r\n");
                    GLogN("  Session Key stored at slot %d\r\n", HSM_KEY_SESSION);
                    GLogN("========================================\r\n");
                }
                else
                {
                    GLogE("[6] FAIL - AES round-trip failed!\r\n");
                }
                break;
            }

            case 0xAA:  // 1-step Encrypted AES Import Test (HSM_READ_RANDKEY production method)
            {
                GLogN("\r\n========== 1-Step Encrypted AES Import Test ==========\r\n");
                GLogN("Testing hsm_import_aes_key(encrypted=true) for HSM_READ_RANDKEY...\r\n\r\n");

                // NIST SP 800-38A AES-128 Test Vector Key
                static const uint8_t test_aes_key[16] = {
                    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
                    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
                };

                // Use HSM_KEY_KEK_RSA (#149) for key encryption/decryption per manual
                uint16_t rsa_key_id = HSM_KEY_KEK_RSA;

                // Step 1: Check RSA key exists
                GLogN("[1] Checking RSA key at slot %d (HSM_KEY_KEK_RSA)...\r\n", rsa_key_id);
                uint8_t pub_key[270];
                U16 pub_key_len = 0;

                status = hsm_export_public_key(rsa_key_id, HSM_ALG_RSA, pub_key, &pub_key_len);
                if (status != HAL_OK)
                {
                    GLogE("[1] FAIL - No RSA key at slot %d\r\n", rsa_key_id);
                    GLogE("    Generate RSA key: hsmtest 0x05 149\r\n");
                    break;
                }
                GLogN("[1] OK - RSA key exists (%d bytes)\r\n\r\n", pub_key_len);

                // Step 2: Encrypt test key with RSA (simulating SAM encryption)
                GLogN("[2] Encrypting AES key with RSA-2048...\r\n");
                GLogN("    AES Key (NIST): ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", test_aes_key[i]);
                GLogN("\r\n");

                uint8_t encrypted_key[256];
                status = hsm_rsa_encrypt(rsa_key_id, test_aes_key, 16, encrypted_key);
                if (status != HAL_OK)
                {
                    GLogE("[2] FAIL - RSA encrypt failed: %d\r\n", status);
                    break;
                }
                GLogN("[2] OK - RSA encrypted (256 bytes)\r\n");
                GLogN("    Ciphertext[0:15]: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", encrypted_key[i]);
                GLogN("...\r\n\r\n");

                // Step 3: 1-step import (encrypted=true) - HSM decrypts internally
                GLogN("[3] 1-Step Import: hsm_import_aes_key(encrypted=true)...\r\n");
                GLogN("    This is the production method for HSM_READ_RANDKEY\r\n");
                GLogN("    HSM internally: RSA decrypt -> Store AES key\r\n");

                status = hsm_import_aes_key(HSM_KEY_SESSION, encrypted_key, HSM_AES_128, true, false);
                if (status != HAL_OK)
                {
                    GLogE("[3] FAIL - 1-step import failed: %d\r\n", status);
                    GLogE("    Check if HSM supports encrypted import mode\r\n");
                    break;
                }
                GLogN("[3] OK - Key imported to slot %d (no plaintext exposure!)\r\n\r\n", HSM_KEY_SESSION);

                // Step 4: Verify by AES encrypt/decrypt
                GLogN("[4] Verifying imported key with AES-ECB...\r\n");

                // NIST SP 800-38A ECB Test Vector
                static const uint8_t nist_plaintext[16] = {
                    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
                    0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A
                };
                // Expected ciphertext for NIST key + plaintext
                static const uint8_t nist_expected[16] = {
                    0x3A, 0xD7, 0x7B, 0xB4, 0x0D, 0x7A, 0x36, 0x60,
                    0xA8, 0x9E, 0xCA, 0xF3, 0x24, 0x66, 0xEF, 0x97
                };

                uint8_t ciphertext[16], decrypted[16];

                status = hsm_aes_encrypt(HSM_KEY_SESSION, HSM_AES_128, HSM_AES_ECB, NULL,
                                         nist_plaintext, 16, ciphertext);
                if (status != HAL_OK)
                {
                    GLogE("[4] FAIL - AES encrypt failed: %d\r\n", status);
                    break;
                }

                GLogN("    Plaintext:  ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", nist_plaintext[i]);
                GLogN("\r\n    Ciphertext: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", ciphertext[i]);
                GLogN("\r\n    Expected:   ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", nist_expected[i]);
                GLogN("\r\n");

                // Compare with NIST expected value
                if (memcmp(ciphertext, nist_expected, 16) == 0)
                {
                    GLogN("[4] OK - Ciphertext matches NIST test vector!\r\n\r\n");
                }
                else
                {
                    GLogN("[4] WARN - Ciphertext differs from NIST (padding difference?)\r\n\r\n");
                }

                // Step 5: Round-trip verification
                GLogN("[5] AES decrypt round-trip test...\r\n");
                status = hsm_aes_decrypt(HSM_KEY_SESSION, HSM_AES_128, HSM_AES_ECB, NULL,
                                         ciphertext, 16, decrypted);
                if (status != HAL_OK)
                {
                    GLogE("[5] FAIL - AES decrypt failed: %d\r\n", status);
                    break;
                }

                if (memcmp(nist_plaintext, decrypted, 16) == 0)
                {
                    GLogN("[5] OK - Round-trip successful!\r\n\r\n");
                    GLogN("============================================\r\n");
                    GLogN("  1-STEP ENCRYPTED IMPORT TEST PASSED!\r\n");
                    GLogN("  Session Key at slot #%d\r\n", HSM_KEY_SESSION);
                    GLogN("  Method: hsm_import_aes_key(encrypted=true)\r\n");
                    GLogN("  Security: No plaintext key in Host memory\r\n");
                    GLogN("============================================\r\n");
                }
                else
                {
                    GLogE("[5] FAIL - Decrypted data mismatch!\r\n");
                }
                break;
            }

            // ========== FL_Git Function Tests (0xF0~0xFF) ==========
            case 0xF0:  // FL_Git_HSM_GetVersion (0x121C)
            {
                GLogN("\r\n========== FL_Git_HSM_GetVersion Test ==========\r\n");
                GLogN("Testing both Old HSM and New HSM version APIs...\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using Read_HSM_Status + HSM_Version_Check\r\n");
                    U16 version = 0;
                    if (Read_HSM_Status() == 0)
                    {
                        if (HSM_Version_Check((int*)&version) == HSM_SUCCESS)
                        {
                            GLogN("[OK] HSM Version: %d\r\n", version);
                        }
                        else
                        {
                            GLogE("[FAIL] HSM_Version_Check failed\r\n");
                        }
                    }
                    else
                    {
                        GLogE("[FAIL] Read_HSM_Status failed\r\n");
                    }
                }
                else
                {
                    GLogN("[NEW HSM] Using hsm_get_version_info\r\n");
                    HSM_VersionInfo_t version_info;
                    if (hsm_get_version_info(&version_info) == HAL_OK)
                    {
                        GLogN("[OK] HSM Version Info:\r\n");
                        GLogN("  Host F/W: %d.%d.%d\r\n",
                              version_info.host_major, version_info.host_minor, version_info.host_patch);
                        GLogN("  HSE F/W:  %d.%d.%d\r\n",
                              version_info.hse_major, version_info.hse_minor, version_info.hse_patch);
                        GLogN("  ASK:      %d.%d.%d\r\n",
                              version_info.ask_major, version_info.ask_minor, version_info.ask_patch);
                    }
                    else
                    {
                        GLogE("[FAIL] hsm_get_version_info failed\r\n");
                    }
                }
                GLogN("================================================\r\n");
                break;
            }

            case 0xF1:  // FL_Git_HSM_State_Check (0x1220)
            {
                GLogN("\r\n========== FL_Git_HSM_State_Check Test ==========\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using Read_HSM_Status\r\n");
                    U8 status = Read_HSM_Status();
                    if (status == 0)
                    {
                        GLogN("[OK] HSM State: READY (status=0)\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] HSM State: ERROR (status=%d)\r\n", status);
                    }
                }
                else
                {
                    GLogN("[NEW HSM] Using hsm_get_serial_number\r\n");
                    uint8_t serial[8];
                    if (hsm_get_serial_number(serial) == HAL_OK)
                    {
                        GLogN("[OK] HSM State: READY\r\n");
                        GLogN("  Serial: %02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                              serial[0], serial[1], serial[2], serial[3],
                              serial[4], serial[5], serial[6], serial[7]);
                    }
                    else
                    {
                        GLogE("[FAIL] HSM State: ERROR\r\n");
                    }
                }
                GLogN("=================================================\r\n");
                break;
            }

            case 0xF2:  // FL_Git_CRL_GetDate (0x1218)
            {
                GLogN("\r\n========== FL_Git_CRL_GetDate Test ==========\r\n");

                U8 crl_no = 1;  // Default CRL #1
                if (count >= 3)
                {
                    crl_no = (U8)strtol(gCliArgs[2], NULL, 0);
                }
                GLogN("CRL Number: %d\r\n\r\n", crl_no);

                U8 crl_data[1000] = {0};
                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using ActivationHSM + ReadDataHSM\r\n");
                    ActivationHSM();
                    ret = ReadDataHSM(crl_data, 506, crl_no);
                }
                else
                {
                    GLogN("[NEW HSM] Using GetCrlEmmc\r\n");
                    U32 read_len = 0;
                    if (GetCrlEmmc(crl_no, crl_data, &read_len) == TRUE)
                    {
                        ret = HSM_SUCCESS;
                        GLogN("  Read Length: %d bytes\r\n", read_len);
                    }
                }

                if (ret == HSM_SUCCESS)
                {
                    GLogN("[OK] CRL Date Read Success\r\n");
                    // Parse date from offset 89 (6 bytes: Effective 3 + Expiration 3)
                    GLogN("  Effective Date : 20%02X-%02X-%02X (YYMMDD)\r\n",
                          crl_data[89], crl_data[90], crl_data[91]);
                    GLogN("  Expiration Date: 20%02X-%02X-%02X (YYMMDD)\r\n",
                          crl_data[92], crl_data[93], crl_data[94]);
                    GLogN("  Raw (offset 89): %02X %02X %02X %02X %02X %02X\r\n",
                          crl_data[89], crl_data[90], crl_data[91],
                          crl_data[92], crl_data[93], crl_data[94]);
                }
                else
                {
                    GLogE("[FAIL] CRL Read Failed (ret=%d)\r\n", ret);
                }
                GLogN("=============================================\r\n");
                break;
            }

            case 0xF3:  // FL_Git_CRT_GetHolderRef (0x121A)
            {
                GLogN("\r\n========== FL_Git_CRT_GetHolderRef Test ==========\r\n");

                U8 cert_no = 1;  // Default Cert #1
                if (count >= 3)
                {
                    cert_no = (U8)strtol(gCliArgs[2], NULL, 0);
                }
                GLogN("Certificate Number: %d\r\n\r\n", cert_no);

                U8 cert_data[700] = {0};
                U32 cert_len = 0;
                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using ActivationHSM + ReadCertificateHSM\r\n");
                    ret = ActivationHSM();
                    if (ret == HSM_SUCCESS)
                    {
                        ret = ReadCertificateHSM(cert_data, (int*)&cert_len, cert_no);
                    }
                }
                else
                {
                    GLogN("[NEW HSM] Using hsm_read_certificate\r\n");
                    U16 length = 0;
                    if (hsm_read_certificate(cert_no, HSM_ALG_RSA, cert_data, &length) == HAL_OK)
                    {
                        ret = HSM_SUCCESS;
                        cert_len = length;
                    }
                }

                if (ret == HSM_SUCCESS)
                {
                    GLogN("[OK] Certificate Read Success (len=%d)\r\n", cert_len);
                    // HolderRef is at offset 75-93 (19 bytes)
                    GLogN("  HolderRef (19 bytes): ");
                    for (int i = 75; i < 94 && i < cert_len; i++)
                    {
                        GLogN("%02X ", cert_data[i]);
                    }
                    GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] Certificate Read Failed (ret=%d)\r\n", ret);
                }
                GLogN("=================================================\r\n");
                break;
            }

            case 0xF4:  // FL_Git_GenRsaKeyPair (0x123B)
            {
                GLogN("\r\n========== FL_Git_GenRsaKeyPair Test ==========\r\n");

                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using ActivationHSM + GenerateRSAKeypairHSM\r\n");
                    if (Read_HSM_Status() == 0)
                    {
                        if (ActivationHSM() == HSM_SUCCESS)
                        {
                            DeleteRSAKeypairHSM();
                            ret = GenerateRSAKeypairHSM();
                        }
                    }
                }
                else
                {
                    GLogN("[NEW HSM] Using hsm_generate_key_pair (Key ID: %d)\r\n", HSM_KEK_RSA_KEY_ID);
                    if (hsm_generate_key_pair(150, HSM_ALG_RSA, 0, false) == HAL_OK)
                    {
                        ret = HSM_SUCCESS;
                    }
                }

                if (ret == HSM_SUCCESS)
                {
                    GLogN("[OK] RSA Key Pair Generation Success\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Key Pair Generation Failed\r\n");
                }
                GLogN("===============================================\r\n");
                break;
            }

            case 0xF5:  // FL_Git_ECUCODE_KEY_Req (0x121B)
            {
                GLogN("\r\n========== FL_Git_ECUCODE_KEY_Req Test ==========\r\n");

                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using ActivationHSM + ReadAES128KeyChecksumHSM\r\n");
                    U8 checksum = 0;
                    ret = ActivationHSM();
                    ret = ReadAES128KeyChecksumHSM(AES_INDEX_6, &checksum);
                    if (ret == HSM_SUCCESS && checksum == 0x7B)
                    {
                        GLogN("[OK] ECU Code Key Valid (checksum=0x%02X)\r\n", checksum);
                    }
                    else
                    {
                        GLogE("[FAIL] ECU Code Key Invalid (ret=%d, checksum=0x%02X)\r\n", ret, checksum);
                    }
                }
                else
                {
                    GLogN("[NEW HSM] ECU Code Key check always returns OK\r\n");
                    GLogN("[OK] ECU Code Key Valid (New HSM always OK)\r\n");
                    ret = HSM_SUCCESS;
                }
                GLogN("=================================================\r\n");
                break;
            }

            case 0xF6:  // FL_Git_StoreEncryptKey (0x123A)
            {
                GLogN("\r\n========== FL_Git_StoreEncryptKey Test ==========\r\n");
                GLogN("Testing RSA Decrypt for Encrypted Key Storage...\r\n\r\n");

                // Real encrypted data (AES key encrypted with HSM's RSA public key)
                // Plaintext: 1122334455667788990011223344556677889900112233445566778899001122
                
				
				static const uint8_t test_encrypted[256] = {
					0x5A, 0x42, 0x97, 0x1A, 0xDA, 0x1F, 0x34, 0x83, 0x2F, 0x96, 0x62, 0x1F, 0x05, 0xE2, 0x27, 0x23,
					0x1D, 0xEF, 0xCE, 0x70, 0x03, 0xE0, 0x78, 0x94, 0x1A, 0x54, 0xE7, 0x96, 0x39, 0x38, 0x29, 0xA2,
					0x8C, 0xD4, 0xC1, 0xA8, 0xED, 0x46, 0xA5, 0xB2, 0x88, 0x19, 0x94, 0x8E, 0xE0, 0x17, 0xDD, 0x69,
					0xD3, 0x2A, 0xFC, 0xB8, 0xBE, 0x9D, 0x03, 0xB8, 0x46, 0xC8, 0x66, 0x84, 0xB2, 0x02, 0x83, 0x1E,
					0x38, 0x90, 0x24, 0xDD, 0xAE, 0xE2, 0xAE, 0xF2, 0x53, 0x74, 0x83, 0x51, 0x6D, 0x5A, 0xAF, 0x75,
					0x16, 0x9F, 0x49, 0x24, 0x05, 0xF7, 0x0B, 0xFB, 0x67, 0xB1, 0x01, 0x8F, 0x23, 0x1B, 0xB3, 0xD1,
					0xD4, 0x0C, 0x56, 0xBE, 0xCF, 0x1E, 0xC5, 0xAB, 0x6E, 0x44, 0x31, 0xA5, 0x7E, 0xF2, 0xC5, 0xA5,
					0xB6, 0x47, 0x33, 0x27, 0x18, 0x24, 0x84, 0x24, 0xBA, 0xA6, 0xC6, 0x47, 0x13, 0x47, 0x48, 0xF4,
					0x02, 0xC4, 0x7D, 0x0E, 0x4B, 0x2A, 0x36, 0x98, 0x38, 0x29, 0xAB, 0x68, 0x4C, 0xFE, 0x80, 0x2F,
					0x57, 0xC6, 0x04, 0xDC, 0x77, 0xB1, 0x44, 0xDE, 0x44, 0xDA, 0xD7, 0x16, 0xD7, 0xA9, 0x24, 0x26,
					0xBA, 0x08, 0xEE, 0xA7, 0x2F, 0xA1, 0x6B, 0x1E, 0x00, 0x80, 0xEF, 0x8E, 0x56, 0x43, 0x18, 0x7A,
					0x70, 0x62, 0x4B, 0x1C, 0x87, 0x95, 0xD0, 0x70, 0xF6, 0xA1, 0xEF, 0xD8, 0x6A, 0x57, 0x4E, 0x23,
					0x78, 0x76, 0x04, 0xBA, 0x6B, 0xEA, 0x0F, 0xAD, 0xB5, 0x18, 0x4D, 0x52, 0x59, 0x55, 0x24, 0x11,
					0x7B, 0x95, 0xA6, 0x4E, 0xA2, 0x74, 0x73, 0x99, 0x09, 0x9D, 0xE7, 0x31, 0xE7, 0xD7, 0x14, 0xFE,
					0xAE, 0x31, 0xFE, 0xED, 0x78, 0xA8, 0x45, 0xE1, 0x3C, 0xD4, 0x6F, 0xE4, 0x32, 0x7C, 0x6C, 0xF7,
					0xED, 0xC4, 0xC8, 0xCA, 0xE1, 0x1A, 0xF6, 0x5A, 0x7B, 0x63, 0x25, 0x94, 0xD4, 0xF5, 0xFC, 0x22
				};
                
                // Expected decrypted: 11 22 33 44 55 66 77 88 99 00 11 22 33 44 55 66 77 88 99 00 11 22 33 44 55 66 77 88 99 00 11 22

                U8 decrypted[256] = {0};
                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using DecryptPrivateKeyHSM\r\n");
                    ret = DecryptPrivateKeyHSM((U8*)test_encrypted, decrypted);
                }
                else
                {
                    GLogN("[NEW HSM] Using hsm_rsa_decrypt (Key ID: %d)\r\n", HSM_KEK_RSA_KEY_ID);
                    U16 plain_len = 0;
                    if (hsm_rsa_decrypt(149, test_encrypted, decrypted, &plain_len) == HAL_OK)
                    {
                        ret = HSM_SUCCESS;
                        GLogN("  Decrypted Length: %d bytes\r\n", plain_len);
                    }
                }

                if (ret == HSM_SUCCESS)
                {
                    GLogN("[OK] RSA Decrypt Success\r\n");
                    GLogN("  Decrypted data (32 bytes):\r\n  ");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", decrypted[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                    }
                    GLogN("\r\n  Expected:\r\n  11 22 33 44 55 66 77 88 99 00 11 22 33 44 55 66\r\n  77 88 99 00 11 22 33 44 55 66 77 88 99 00 11 22\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Decrypt Failed (need valid RSA key pair first)\r\n");
                    GLogE("  Run 'hsmtest 0xF4' or 'hsmtest 0x05' to generate RSA key\r\n");
                }
                GLogN("=================================================\r\n");
                break;
            }

            // ========== RSA Decrypt Test Flow (0xE1 ~ 0xE3) ==========
            // Usage:
            //   1. hsmtest 0xE1  - Generate RSA key pair + Export public key
            //   2. Copy public key and encrypt data on PC
            //   3. Update test_encrypted array in 0xE3 case, rebuild
            //   4. hsmtest 0xE3  - Decrypt test (DO NOT power off between steps!)

            case 0xE1:  // Step 1: Generate RSA Key Pair + Export Public Key
            {
                GLogN("\r\n");
                GLogN("╔══════════════════════════════════════════════════════════════╗\r\n");
                GLogN("║  [STEP 1/3] RSA Key Generate + Export Public Key             ║\r\n");
                GLogN("╚══════════════════════════════════════════════════════════════╝\r\n");
                GLogN("\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using ActivationHSM + GenerateRSAKeypairHSM\r\n");
                    if (Read_HSM_Status() == 0)
                    {
                        if (ActivationHSM() == HSM_SUCCESS)
                        {
                            DeleteRSAKeypairHSM();
                            if (GenerateRSAKeypairHSM() == HSM_SUCCESS)
                            {
                                GLogN("[OK] RSA Key Pair Generated (OLD HSM)\r\n");
                                // OLD HSM doesn't have easy export, user needs to use GetPublicKeyHSM
                                GLogN("\r\n[NOTE] Use GetPublicKeyHSM to retrieve public key\r\n");
                            }
                            else
                            {
                                GLogE("[FAIL] RSA Key Pair Generation Failed\r\n");
                            }
                        }
                    }
                    break;
                }

                // NEW HSM
                GLogN("[NEW HSM] Key ID: %d (KEK RSA Slot)\r\n", HSM_KEK_RSA_KEY_ID);
                GLogN("\r\n");

                // Step 1: Generate RSA key pair
                GLogN(">> Generating RSA-2048 Key Pair...\r\n");
                HAL_StatusTypeDef gen_status = hsm_generate_key_pair(
                    HSM_KEK_RSA_KEY_ID,
                    HSM_ALG_RSA,
                    0,
                    false
                );

                if (gen_status != HAL_OK)
                {
                    GLogE("[FAIL] Generate Key Pair failed, status: %d\r\n", gen_status);
                    break;
                }
                GLogN("[OK] RSA Key Pair Generated Successfully!\r\n");
                GLogN("\r\n");

                // Step 2: Export public key
                GLogN(">> Exporting Public Key...\r\n");
                uint8_t pub_key[512];
                uint16_t pub_key_len = 0;

                HAL_StatusTypeDef export_status = hsm_export_public_key(
                    HSM_KEK_RSA_KEY_ID,
                    HSM_ALG_RSA,
                    pub_key,
                    &pub_key_len
                );

                if (export_status != HAL_OK)
                {
                    GLogE("[FAIL] Export Public Key failed, status: %d\r\n", export_status);
                    break;
                }

                GLogN("[OK] Public Key Exported (%d bytes)\r\n", pub_key_len);
                GLogN("\r\n");

                // Parse according to document structure (5.6.5.1.1):
                // Response Code excluded by spi_cmd_r_cmd
                // [0-1] modulus byte length (2)
                // [2-257] modulus (256)
                // [258-259] pubSize (2)
                // [260+] pubExp (pubSize bytes)

                uint16_t mod_len = (uint16_t)pub_key[0] | ((uint16_t)pub_key[1] << 8);
                uint16_t pub_size = (uint16_t)pub_key[258] | ((uint16_t)pub_key[259] << 8);

                GLogN("════════════════════════════════════════════════════════════════\r\n");
                GLogN("  PUBLIC KEY (Copy this for PC encryption)\r\n");
                GLogN("════════════════════════════════════════════════════════════════\r\n");
                GLogN("\r\n");

                // Modulus (256 bytes starting at offset 2, after mod_len)
                GLogN("Modulus (N) - %d bytes:\r\n", mod_len);
                for (int i = 0; i < 256; i++)
                {
                    GLogN("%02X", pub_key[2 + i]);
                    if ((i + 1) % 32 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                // Public Exponent (pubSize bytes starting at offset 260)
                GLogN("Public Exponent (E) - %d bytes: ", pub_size);
                for (int i = 0; i < pub_size; i++)
                {
                    GLogN("%02X", pub_key[260 + i]);
                }
                GLogN("\r\n");
                GLogN("\r\n");
                GLogN("════════════════════════════════════════════════════════════════\r\n");
                GLogN("\r\n");
                GLogN("┌──────────────────────────────────────────────────────────────┐\r\n");
                GLogN("│  NEXT STEPS                                                  │\r\n");
                GLogN("├──────────────────────────────────────────────────────────────┤\r\n");
                GLogN("│  1. Copy the Modulus (N) above                               │\r\n");
                GLogN("│  2. Encrypt data on PC with this public key                  │\r\n");
                GLogN("│     - Algorithm: RSA-2048 PKCS#1 v1.5                         │\r\n");
                GLogN("│     - Public Exponent: 65537 (0x010001)                       │\r\n");
                GLogN("│  3. Run: hsmtest 0xE2 <512 hex chars>                        │\r\n");
                GLogN("│                                                              │\r\n");
                GLogN("│  *** DO NOT POWER OFF - Key is in RAM! ***                   │\r\n");
                GLogN("└──────────────────────────────────────────────────────────────┘\r\n");
                break;
            }

            case 0xE2:  // Step 2: Decrypt with user-provided ciphertext
            {
                GLogN("\r\n");
                GLogN("╔══════════════════════════════════════════════════════════════╗\r\n");
                GLogN("║  [STEP 2/3] RSA Decrypt Test                                 ║\r\n");
                GLogN("╚══════════════════════════════════════════════════════════════╝\r\n");
                GLogN("\r\n");

                // Check if ciphertext argument provided
                if (count < 3)
                {
                    GLogN("Usage: hsmtest 0xE2 <512 hex chars>\r\n");
                    GLogN("\r\n");
                    GLogN("Example:\r\n");
                    GLogN("  hsmtest 0xE2 3CABA660...(512 hex chars)...0DEB\r\n");
                    GLogN("\r\n");
                    GLogN("Or use 0xE3 with hardcoded test data.\r\n");
                    break;
                }

                // Parse hex string to bytes
                const char* hex_str = gCliArgs[2];
                size_t hex_len = strlen(hex_str);

                if (hex_len != 512)
                {
                    GLogE("[ERROR] Ciphertext must be 512 hex chars (256 bytes)\r\n");
                    GLogE("  Provided: %d chars\r\n", (int)hex_len);
                    break;
                }

                uint8_t ciphertext[256];
                for (int i = 0; i < 256; i++)
                {
                    char byte_str[3] = { hex_str[i*2], hex_str[i*2+1], 0 };
                    ciphertext[i] = (uint8_t)strtol(byte_str, NULL, 16);
                }

                GLogN("Ciphertext (first 32 bytes):\r\n  ");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", ciphertext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                }
                GLogN("\r\n");

                // Decrypt
                GLogN(">> Decrypting with Key ID %d...\r\n", HSM_KEK_RSA_KEY_ID);
                uint8_t plaintext[256] = {0};
                uint16_t plain_len = 0;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    if (DecryptPrivateKeyHSM(ciphertext, plaintext) == HSM_SUCCESS)
                    {
                        plain_len = 32;
                        GLogN("[OK] Decrypt Success (OLD HSM)\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] Decrypt Failed (OLD HSM)\r\n");
                        break;
                    }
                }
                else
                {
                    if (hsm_rsa_decrypt(HSM_KEK_RSA_KEY_ID, ciphertext, plaintext, &plain_len) != HAL_OK)
                    {
                        GLogE("[FAIL] RSA Decrypt Failed!\r\n");
                        GLogE("  - Run 0xE1 first to generate key pair\r\n");
                        GLogE("  - Do not power off between 0xE1 and 0xE2\r\n");
                        break;
                    }
                }

                GLogN("\r\n");
                GLogN("════════════════════════════════════════════════════════════════\r\n");
                GLogN("  DECRYPTED DATA (%d bytes)\r\n", plain_len);
                GLogN("════════════════════════════════════════════════════════════════\r\n");
                for (int i = 0; i < plain_len; i++)
                {
                    GLogN("%02X ", plaintext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                if (plain_len % 16 != 0) GLogN("\r\n");
                GLogN("════════════════════════════════════════════════════════════════\r\n");
                GLogN("\r\n[OK] RSA Decrypt Test Complete!\r\n");
                break;
            }

            case 0xE3:  // Step 3: Decrypt with hardcoded test data (for easy re-test)
            {
                GLogN("\r\n");
                GLogN("╔══════════════════════════════════════════════════════════════╗\r\n");
                GLogN("║  [STEP 3/3] RSA Decrypt Test (Hardcoded Ciphertext)          ║\r\n");
                GLogN("╚══════════════════════════════════════════════════════════════╝\r\n");
                GLogN("\r\n");

                // ================================================================
                // UPDATE THIS ARRAY with your encrypted data from PC!
                // Plaintext was: 1122334455667788990011223344556677889900112233445566778899001122
                // ================================================================
#if 0
					static const uint8_t test_ciphertext[256] = {
                    0x3C, 0xAB, 0xA6, 0x60, 0x48, 0x22, 0xBF, 0x1B, 0x44, 0x21, 0xCA, 0x6A, 0xD7, 0x0C, 0x7B, 0xF4,
                    0xB9, 0xC7, 0xEB, 0x34, 0x02, 0x6F, 0x6A, 0x5A, 0x4E, 0x42, 0xB2, 0xDE, 0x8C, 0x0C, 0xB8, 0x27,
                    0xFC, 0xC6, 0xD1, 0x9C, 0x7C, 0xB1, 0x62, 0x1D, 0x54, 0x7C, 0x35, 0x78, 0x94, 0x15, 0x89, 0xC8,
                    0xA9, 0x75, 0x9D, 0xF0, 0x49, 0x7C, 0x8D, 0xE4, 0xA2, 0xBA, 0x78, 0x3B, 0x51, 0xDB, 0xDA, 0x2A,
                    0x41, 0xA6, 0x8B, 0xA7, 0x13, 0xC8, 0xA8, 0x24, 0xC6, 0xA6, 0xFB, 0xB0, 0xB4, 0x6E, 0x63, 0xE0,
                    0xEE, 0x04, 0x5B, 0xB8, 0x73, 0x19, 0x5C, 0x4E, 0x9E, 0x9B, 0xA4, 0x14, 0x92, 0xFA, 0x2F, 0x9D,
                    0x05, 0x2A, 0x3F, 0x3A, 0x98, 0xE0, 0x67, 0x8A, 0x18, 0xA1, 0x7E, 0xF3, 0x57, 0x8C, 0x97, 0xCC,
                    0xEB, 0xBF, 0xF1, 0x24, 0x20, 0x29, 0x12, 0xEB, 0x9C, 0x1A, 0xF0, 0xB3, 0x4B, 0xD3, 0x8C, 0x45,
                    0xDB, 0x6E, 0x54, 0x4D, 0xE6, 0x83, 0x22, 0x92, 0xE1, 0x56, 0xC9, 0xD3, 0x2D, 0x9E, 0x98, 0x45,
                    0x2B, 0xC5, 0x07, 0x1C, 0x42, 0xFB, 0x5F, 0xC0, 0xD7, 0x70, 0xDE, 0xB0, 0x7F, 0x3A, 0x67, 0xED,
                    0x67, 0xF4, 0x4C, 0x55, 0xAF, 0x87, 0x53, 0xA3, 0x57, 0x2D, 0xC2, 0x19, 0x41, 0x6E, 0xCF, 0x4C,
                    0xBF, 0xD4, 0x2F, 0xBD, 0xC7, 0x1B, 0xCD, 0x40, 0x13, 0x5B, 0x11, 0xAF, 0x51, 0xA1, 0x0A, 0xA5,
                    0xE2, 0x7E, 0x07, 0xA1, 0x74, 0xEB, 0x71, 0xF9, 0x38, 0x26, 0x35, 0x78, 0x1B, 0xEC, 0x0D, 0xDD,
                    0xB3, 0x03, 0xA3, 0x12, 0x89, 0x4B, 0xDE, 0x9E, 0xB1, 0x7E, 0x61, 0xCF, 0x20, 0x37, 0xE8, 0xEA,
                    0x11, 0x1E, 0x70, 0x4E, 0x82, 0xEA, 0x35, 0xC2, 0xF3, 0xC9, 0x0D, 0x70, 0xF8, 0xDA, 0x47, 0xE2,
                    0x89, 0x4C, 0x30, 0xBE, 0x54, 0x02, 0xE6, 0x4A, 0x5E, 0x21, 0xCD, 0x48, 0xBA, 0xDD, 0x0D, 0xEB
                };
#endif
#if 0	
				static const uint8_t test_ciphertext[256] = {
					0xA4, 0x5F, 0x64, 0xA1, 0x07, 0xAE, 0xCE, 0x49, 0x72, 0x67, 0x39, 0x92, 0xAB, 0x86, 0x18, 0xAC,
					0x31, 0x0A, 0xB0, 0x8E, 0x5F, 0x0B, 0x67, 0x91, 0x77, 0x54, 0xF8, 0x81, 0x3D, 0x2D, 0x4D, 0x93,
					0x9A, 0xAC, 0xFD, 0xE8, 0xCE, 0xBD, 0x9C, 0x2B, 0x6A, 0xF4, 0xD2, 0x3E, 0x7C, 0xB0, 0x37, 0x36,
					0x7C, 0x1A, 0x1F, 0xE5, 0x42, 0xF8, 0x80, 0x1E, 0xFF, 0x0A, 0xC2, 0xBB, 0x49, 0xC9, 0xCC, 0x22,
					0xC3, 0x79, 0x17, 0x88, 0x9A, 0xC9, 0x04, 0xD4, 0x66, 0x7E, 0x90, 0xD7, 0xA4, 0xCA, 0x45, 0x8D,
					0xC4, 0xF4, 0xFA, 0x85, 0xB0, 0x20, 0x78, 0x0D, 0x94, 0x2E, 0x26, 0xD2, 0xEA, 0xC9, 0x1F, 0x5E,
					0xE6, 0xC3, 0x4F, 0x41, 0xFE, 0xB8, 0x87, 0x1C, 0x67, 0xAD, 0x6D, 0x16, 0x56, 0x6A, 0x73, 0x10,
					0x12, 0x5A, 0x9B, 0x25, 0xB4, 0x28, 0xFB, 0xC3, 0xE1, 0x63, 0x9C, 0xE4, 0x9D, 0x50, 0x8C, 0x82,
					0x0B, 0xD9, 0x74, 0x6C, 0x89, 0xB2, 0x8F, 0xA8, 0x8F, 0xD0, 0xC9, 0xED, 0x1B, 0x56, 0xB3, 0xA1,
					0xF8, 0x59, 0x9D, 0x62, 0x2F, 0xFC, 0x5F, 0x15, 0x99, 0xCC, 0x54, 0x12, 0x21, 0xE9, 0x96, 0x02,
					0x64, 0x16, 0x54, 0xC7, 0x14, 0xF5, 0x24, 0x43, 0x1F, 0x5A, 0xDE, 0x04, 0x4F, 0xC7, 0x86, 0xEA,
					0xE0, 0x99, 0x14, 0x66, 0xAE, 0x55, 0xAE, 0x14, 0xF5, 0x5D, 0x6F, 0x04, 0xE1, 0x83, 0xCD, 0x20,
					0xBC, 0xFB, 0x97, 0xBE, 0x4E, 0x9E, 0x21, 0xBF, 0xF1, 0x01, 0x3B, 0x5A, 0x7E, 0x80, 0x16, 0x0D,
					0x21, 0x31, 0xFD, 0x43, 0xA6, 0x8B, 0xBD, 0xBD, 0xB2, 0xDB, 0x89, 0x42, 0x56, 0x5D, 0xEE, 0x87,
					0xED, 0x38, 0x28, 0x07, 0x7A, 0x88, 0x79, 0x97, 0xF0, 0x25, 0x9A, 0xF3, 0x65, 0xA8, 0xF7, 0x55,
					0x77, 0xB3, 0x3F, 0x21, 0x2E, 0xA5, 0x4F, 0x21, 0xAF, 0x19, 0x89, 0x51, 0x28, 0x4A, 0xA5, 0x48
				};
#endif
				
#if 1
				static const uint8_t test_ciphertext[256] = {
					0xA8, 0x71, 0x38, 0x67, 0x90, 0x85, 0xBB, 0x57, 0x02, 0x7B, 0x9C, 0x05, 0x92, 0x08, 0x51, 0x23,
					0x97, 0xB8, 0xB8, 0x29, 0x6A, 0xD0, 0x01, 0x1D, 0xC5, 0x61, 0xB6, 0xEB, 0x4C, 0x78, 0x84, 0x7F,
					0x91, 0x3D, 0xD1, 0x2B, 0x46, 0x25, 0xF0, 0x50, 0x02, 0xDC, 0xF2, 0x02, 0xAA, 0x7E, 0x73, 0x52,
					0x58, 0x73, 0x42, 0x6B, 0x4C, 0x98, 0xD5, 0x4B, 0xB4, 0xB1, 0x06, 0x6C, 0xD8, 0xE4, 0x8D, 0x79,
					0x80, 0x34, 0x30, 0xEA, 0x7C, 0x11, 0x6F, 0xB2, 0xED, 0x75, 0xCD, 0x48, 0xDD, 0x46, 0x9D, 0xB4,
					0xE7, 0x0D, 0x24, 0x44, 0x21, 0x10, 0x66, 0x59, 0x0B, 0x9B, 0xA8, 0x0B, 0xCE, 0x7D, 0xCF, 0x5F,
					0x39, 0x78, 0x67, 0xCA, 0x20, 0xD3, 0x98, 0x6E, 0x25, 0xC8, 0x17, 0x85, 0x94, 0xC8, 0xF6, 0xCC,
					0x36, 0x70, 0xF4, 0xA1, 0xA5, 0x0E, 0xFB, 0x89, 0x3F, 0x38, 0xBA, 0x24, 0xF5, 0xB4, 0x88, 0xD1,
					0xBF, 0x5C, 0x71, 0xCB, 0x53, 0x1D, 0xB0, 0x97, 0xAF, 0xE3, 0xB0, 0x66, 0xE2, 0x49, 0x47, 0xE2,
					0x59, 0x2F, 0xDC, 0x03, 0x15, 0xDD, 0xE0, 0x40, 0xF3, 0xC7, 0x91, 0xD0, 0x74, 0x54, 0x93, 0x69,
					0xF2, 0xF0, 0xC6, 0x7B, 0xCC, 0x62, 0x59, 0xA5, 0xA6, 0x32, 0x1E, 0x12, 0x24, 0xCF, 0xEE, 0xA2,
					0xCE, 0x22, 0x93, 0x2F, 0xB3, 0xA5, 0x50, 0xEE, 0x09, 0xE7, 0xE6, 0x8C, 0x9A, 0xDF, 0x78, 0xD5,
					0x24, 0xFD, 0xC7, 0xCD, 0x59, 0x84, 0x3D, 0x33, 0x84, 0x19, 0x09, 0xED, 0x17, 0xDC, 0xCA, 0xF8,
					0x41, 0x00, 0xAD, 0xC0, 0xC7, 0x32, 0xA0, 0x29, 0x82, 0xEB, 0xE6, 0x4B, 0xF0, 0x54, 0xFA, 0x05,
					0xCA, 0x22, 0xC2, 0xDD, 0xF1, 0x65, 0x40, 0x81, 0xA3, 0x74, 0xD4, 0x03, 0xE3, 0xF3, 0x7A, 0x1B,
					0xAD, 0x38, 0x5C, 0x38, 0xA1, 0x86, 0x94, 0xB8, 0x5F, 0xD5, 0x14, 0x43, 0x05, 0xCB, 0x69, 0xFB
				};
#endif
				
#if 0
				static const uint8_t test_ciphertext[256] = {
					0x6F, 0x89, 0x99, 0x15, 0x16, 0x85, 0x98, 0xA8, 0x65, 0xE3, 0xE9, 0xF5, 0xBB, 0x08, 0x02, 0xFD,
					0x37, 0x59, 0xD9, 0x69, 0x50, 0x7B, 0x6B, 0x36, 0x3B, 0x7A, 0xA4, 0xBF, 0x67, 0x9C, 0xF9, 0x87,
					0x8B, 0x9A, 0x92, 0x53, 0x3E, 0x7D, 0xD4, 0x5E, 0x59, 0x1F, 0xF2, 0x8A, 0x91, 0xBC, 0x97, 0xD1,
					0x0E, 0x98, 0xF2, 0xC3, 0xF5, 0x2A, 0xBD, 0x81, 0x6E, 0xA2, 0xF9, 0x50, 0xA5, 0x85, 0xFD, 0x56,
					0x56, 0x54, 0xEA, 0x5B, 0x55, 0xA3, 0x11, 0xCA, 0x5D, 0x1E, 0xB1, 0xD0, 0x56, 0x8A, 0x49, 0x4E,
					0xB7, 0xED, 0x73, 0x10, 0xB0, 0x6A, 0x58, 0x3B, 0xF0, 0x74, 0x06, 0x63, 0x46, 0x10, 0x2F, 0xFC,
					0xDD, 0xF5, 0x0B, 0x8C, 0x7C, 0x93, 0x31, 0x8B, 0x75, 0xF5, 0x79, 0x90, 0x95, 0xCD, 0x03, 0x6D,
					0x25, 0xE0, 0x51, 0x96, 0xAE, 0x4A, 0x5B, 0x92, 0xD2, 0x53, 0x35, 0x62, 0xE1, 0x41, 0xA0, 0x77,
					0x67, 0x3B, 0x0B, 0x48, 0x25, 0x8B, 0x79, 0xFA, 0x5C, 0x37, 0xCE, 0xFE, 0x82, 0x8D, 0xAA, 0x54,
					0x62, 0x2B, 0xBB, 0x93, 0x12, 0x04, 0xE3, 0xAF, 0xFE, 0x14, 0x95, 0x53, 0x44, 0xA1, 0x05, 0x20,
					0x23, 0xC2, 0xBE, 0x33, 0xD9, 0xAA, 0x3B, 0x70, 0x60, 0x09, 0x8B, 0x40, 0x9D, 0xE9, 0x6B, 0x67,
					0x68, 0x20, 0x2D, 0x7F, 0xDD, 0xCC, 0x96, 0x07, 0xE9, 0x2D, 0x7E, 0xC8, 0x3E, 0x86, 0xEA, 0xB9,
					0x19, 0xC2, 0xDF, 0xA0, 0x60, 0x09, 0xA9, 0xBB, 0x8B, 0x46, 0x9B, 0xEA, 0xC9, 0xC8, 0x8D, 0xD7,
					0x3C, 0xA2, 0x01, 0xAC, 0x07, 0x0B, 0xC6, 0xFB, 0xF9, 0xA6, 0x78, 0x16, 0x73, 0x86, 0x6E, 0x4E,
					0x2D, 0xB3, 0x32, 0xB5, 0x79, 0x20, 0x09, 0x68, 0x0D, 0x4C, 0x56, 0xE8, 0xF2, 0x15, 0x75, 0xB2,
					0x22, 0x1F, 0xA1, 0xE6, 0x90, 0x8C, 0xDF, 0xE3, 0xFF, 0x61, 0x58, 0x3A, 0x95, 0xD8, 0xB5, 0x1B
				};
#endif

                static const uint8_t expected_plaintext[32] = {
                    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                    0x99, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                    0x77, 0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44,
                    0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22
                };

                GLogN("[NOTE] Using hardcoded ciphertext from code\r\n");
                GLogN("  To use new ciphertext: update test_ciphertext[] and rebuild\r\n");
                GLogN("  Or use: hsmtest 0xE2 <hex_ciphertext>\r\n");
                GLogN("\r\n");

                GLogN("Ciphertext (first 32 bytes):\r\n  ");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_ciphertext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                }
                GLogN("\r\n");

                // Decrypt
                GLogN(">> Decrypting with Key ID %d...\r\n", HSM_KEK_RSA_KEY_ID);
                uint8_t plaintext[256] = {0};
                uint16_t plain_len = 0;
                int success = 0;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    if (DecryptPrivateKeyHSM((uint8_t*)test_ciphertext, plaintext) == HSM_SUCCESS)
                    {
                        plain_len = 32;
                        success = 1;
                    }
                }
                else
                {
                    if (hsm_rsa_decrypt(HSM_KEK_RSA_KEY_ID, test_ciphertext, plaintext, &plain_len) == HAL_OK)
                    {
                        success = 1;
                    }
                }

                if (!success)
                {
                    GLogE("\r\n[FAIL] RSA Decrypt Failed!\r\n");
                    GLogE("════════════════════════════════════════════════════════════════\r\n");
                    GLogE("  Troubleshooting:\r\n");
                    GLogE("  1. Did you run 'hsmtest 0xE1' to generate key pair?\r\n");
                    GLogE("  2. Was power cycled? (Key is lost from RAM)\r\n");
                    GLogE("  3. Is ciphertext encrypted with the correct public key?\r\n");
                    GLogE("════════════════════════════════════════════════════════════════\r\n");
                    break;
                }

                GLogN("\r\n");
                GLogN("════════════════════════════════════════════════════════════════\r\n");
                GLogN("  DECRYPTED DATA (%d bytes)\r\n", plain_len);
                GLogN("════════════════════════════════════════════════════════════════\r\n");
                for (int i = 0; i < plain_len; i++)
                {
                    GLogN("%02X ", plaintext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                if (plain_len % 16 != 0) GLogN("\r\n");

                // Verify against expected
                GLogN("\r\nExpected:\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", expected_plaintext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                int match = (plain_len >= 32) && (memcmp(plaintext, expected_plaintext, 32) == 0);
                GLogN("\r\n════════════════════════════════════════════════════════════════\r\n");
                if (match)
                {
                    GLogN("[OK] Decrypted data matches expected value!\r\n");
                }
                else
                {
                    GLogN("[INFO] Decrypted data differs from expected.\r\n");
                    GLogN("  (This is normal if you used different plaintext)\r\n");
                }
                GLogN("════════════════════════════════════════════════════════════════\r\n");
                break;
            }

            case 0xF7:  // FL_Git_ASK_v2_Req
            {
                GLogN("\r\n========== FL_Git_ASK_v2_Req Test ==========\r\n");

                // ASK Test Vectors
                static const U8 ask_ecu_code[16] = {
                    0x87, 0x87, 0xB6, 0xB1, 0xF3, 0xF9, 0x46, 0xB7,
                    0xB0, 0x2C, 0x39, 0xEE, 0xD3, 0xC2, 0x99, 0xC0
                };
                static const U8 ask_seeds[10][8] = {
                    {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77},  // Seed 01
                    {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},  // Seed 02
                    {0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99},  // Seed 03
                    {0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA},  // Seed 04
                    {0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB},  // Seed 05
                    {0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC},  // Seed 06
                    {0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD},  // Seed 07
                    {0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE},  // Seed 08
                    {0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},  // Seed 09
                    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}   // Seed 10
                };
                // Expected keys for index=0 (VCI3 PV)
                static const U8 ask_expected_pv[10][8] = {
                    {0xDE, 0x93, 0xC3, 0xD1, 0x60, 0x53, 0x0E, 0x20},
                    {0x36, 0xBB, 0x4B, 0xB7, 0xC4, 0x71, 0xA9, 0x56},
                    {0x3A, 0xCB, 0x95, 0x95, 0xC1, 0x2E, 0x58, 0x42},
                    {0xF5, 0x55, 0x15, 0xDF, 0xF7, 0x0B, 0xE9, 0xA7},
                    {0x52, 0x66, 0xA6, 0x40, 0xB9, 0x84, 0x74, 0x37},
                    {0x2E, 0xC7, 0x82, 0x20, 0x7D, 0x9A, 0x8C, 0xF8},
                    {0x42, 0x0D, 0x30, 0x97, 0x06, 0x09, 0xA9, 0xDB},
                    {0xB2, 0x5D, 0x8A, 0x78, 0x4E, 0xFC, 0xDB, 0xB9},
                    {0x57, 0xBA, 0x42, 0xAE, 0x53, 0x24, 0xE3, 0x17},
                    {0xB7, 0xFF, 0xE6, 0xD8, 0x31, 0xC5, 0x3A, 0x1B}
                };
                // Expected keys for index=1 (VCI3 CV)
                static const U8 ask_expected_cv[10][8] = {
                    {0xB3, 0x22, 0x48, 0xE5, 0x07, 0x07, 0x69, 0x36},
                    {0xCD, 0x81, 0x8A, 0x62, 0x05, 0x1D, 0x58, 0xE4},
                    {0x3A, 0xBF, 0x5A, 0x1F, 0xE8, 0xB4, 0xA1, 0xDF},
                    {0x5A, 0x08, 0xB0, 0x09, 0xB1, 0x59, 0x31, 0x2C},
                    {0x30, 0xA9, 0x7A, 0x99, 0x9A, 0xB6, 0x73, 0x4B},
                    {0x44, 0xAD, 0xDF, 0xB4, 0x04, 0x4D, 0xC6, 0x2B},
                    {0x9F, 0xA8, 0xE4, 0x7A, 0x3E, 0xC9, 0x9A, 0xAF},
                    {0x42, 0xE3, 0x23, 0x77, 0x5F, 0x21, 0x7A, 0x2C},
                    {0x6A, 0xFE, 0x25, 0xEA, 0xE5, 0x06, 0xA1, 0x5A},
                    {0x82, 0x1D, 0x28, 0x91, 0xD3, 0x8C, 0x55, 0x85}
                };
                // Expected keys for index=2 (rework)
                static const U8 ask_expected_rework[10][8] = {
                    {0x7B, 0x51, 0x7C, 0x4E, 0x08, 0x3B, 0xD5, 0x4D},
                    {0xAD, 0x1F, 0x4C, 0x42, 0xC2, 0x99, 0xB0, 0xBE},
                    {0x1D, 0xFC, 0xB6, 0x9E, 0x63, 0x10, 0xEC, 0xBF},
                    {0x10, 0xE5, 0xBB, 0xCA, 0xD7, 0xF9, 0xE7, 0xB0},
                    {0x69, 0x8E, 0x2C, 0xB2, 0x3F, 0xFF, 0x0C, 0x01},
                    {0xC3, 0x5F, 0x84, 0x28, 0xC5, 0x2A, 0x98, 0xAA},
                    {0x4C, 0xFB, 0x97, 0xBB, 0xF4, 0x6A, 0x23, 0x77},
                    {0x57, 0xAD, 0x3D, 0x1B, 0x60, 0xF5, 0x2F, 0x8F},
                    {0xBB, 0x70, 0xC7, 0x32, 0xEC, 0x63, 0x90, 0xC9},
                    {0xE3, 0x12, 0xB2, 0x60, 0x41, 0xC0, 0xB7, 0xEF}
                };
                static const U8 test_iv[16] = {
                    0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48,
                    0x53, 0x4D, 0x41, 0x45, 0x53, 0x5F, 0x49, 0x56
                };

                // Parse arguments: hsmtest 0xf7 [master_id]
                U8 master_id = 0;  // 0=PV, 1=CV, 2=rework
                if (count >= 3)
                {
                    master_id = (U8)strtol(gCliArgs[2], NULL, 0);
                }
                if (master_id > 2) master_id = 0;

                const char *type_name = (master_id == 0) ? "PV" : (master_id == 1) ? "CV" : "Rework";
                GLogN("Master ID: %d (%s)\r\n", master_id, type_name);
                GLogN("ECU Code: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", ask_ecu_code[i]);
                GLogN("\r\n\r\n");

                // Select expected key table
                const U8 (*expected_table)[8] = NULL;
                if (master_id == 0) expected_table = ask_expected_pv;
                else if (master_id == 1) expected_table = ask_expected_cv;
                else expected_table = ask_expected_rework;

                int pass_count = 0;
                int fail_count = 0;

                // Old HSM activation (once)
                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using ActivationHSM + ASKSignHSM_AIDxx\r\n\r\n");
                    ActivationHSM();
                }
                else
                {
                    GLogN("[NEW HSM] Using hsm_generate_ask_key\r\n\r\n");
                }

                // Test all 10 seeds
                for (int seed_idx = 0; seed_idx < 10; seed_idx++)
                {
                    const U8 *selected_seed = ask_seeds[seed_idx];
                    const U8 *expected_key = expected_table[seed_idx];
                    U8 result[8] = {0};
                    U32 ret = HSM_UNKNOWN_ERROR;

                    if (g_HSM_Type == HSM_TYPE_OLD)
                    {
                        if (master_id == 0)
                            ret = ASKSignHSM_AID90((U8*)selected_seed, (U8*)ask_ecu_code, (U8*)test_iv, result);
                        else if (master_id == 1)
                            ret = ASKSignHSM_AID93((U8*)selected_seed, (U8*)ask_ecu_code, (U8*)test_iv, result);
                        else
                            ret = ASKSignHSM_AID92((U8*)selected_seed, (U8*)ask_ecu_code, (U8*)test_iv, result);
                    }
                    else
                    {
                        //if (hsm_generate_ask_key((U8*)selected_seed, (U8*)ask_ecu_code, master_id, (U8*)test_iv, result) == HAL_OK)
                        if (hsm_generate_ask_key((U8*)selected_seed, (U8*)ask_ecu_code, master_id, NULL, result) == HAL_OK)
                        {
                            ret = HSM_SUCCESS;
                        }
                    }

                    GLogN("Seed[%02d]: ", seed_idx + 1);
                    for (int i = 0; i < 8; i++) GLogN("%02X ", selected_seed[i]);

                    if (ret == HSM_SUCCESS)
                    {
                        GLogN("\r\n  Result  : ");
                        for (int i = 0; i < 8; i++) GLogN("%02X ", result[i]);
                        GLogN("\r\n  Expected: ");
                        for (int i = 0; i < 8; i++) GLogN("%02X ", expected_key[i]);

                        if (memcmp(result, expected_key, 8) == 0)
                        {
                            GLogN(" [PASS]\r\n");
                            pass_count++;
                        }
                        else
                        {
                            GLogE(" [FAIL]\r\n");
                            fail_count++;
                        }
                    }
                    else
                    {
                        GLogE(" -> HSM Error (ret=%d) [FAIL]\r\n", ret);
                        fail_count++;
                    }
                }

                // Summary
                GLogN("\r\n--------------------------------------------\r\n");
                GLogN("Result: %d PASSED, %d FAILED (Total: 10)\r\n", pass_count, fail_count);
                if (fail_count == 0)
                {
                    GLogN(">> ALL TESTS PASSED!\r\n");
                }
                else
                {
                    GLogE(">> SOME TESTS FAILED!\r\n");
                }
                GLogN("============================================\r\n");
                break;
            }

            case 0xF8:  // FL_Git_CRL_Restore (0x1219) - Store CRL Test
            {
                GLogN("\r\n========== FL_Git_CRL_Restore Test ==========\r\n");

                U8 crl_no = 1;
                if (count >= 3)
                {
                    crl_no = (U8)strtol(gCliArgs[2], NULL, 0);
                }
                GLogN("CRL Number: %d\r\n\r\n", crl_no);

                // Test CRL data (501 bytes dummy)
                static U8 test_crl[501];
                memset(test_crl, 0xAA, 501);
                test_crl[0] = 0x30;  // CRL header
                test_crl[1] = 0x82;

                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using WriteDataHSM\r\n");
                    ret = WriteDataHSM(test_crl, 501, crl_no);
                }
                else
                {
                    GLogN("[NEW HSM] Using StoreCrlEmmc\r\n");
                    if (StoreCrlEmmc(crl_no, test_crl, 501) == FR_OK)
                    {
                        ret = HSM_SUCCESS;
                    }
                }

                if (ret == HSM_SUCCESS)
                {
                    GLogN("[OK] CRL Store Success\r\n");
                }
                else
                {
                    GLogE("[FAIL] CRL Store Failed (ret=%d)\r\n", ret);
                }
                GLogN("=============================================\r\n");
                break;
            }

            case 0xF9:  // Certificate Read Test (Full)
            {
                GLogN("\r\n========== Certificate Full Read Test ==========\r\n");

                U8 cert_no = 1;
                if (count >= 3)
                {
                    cert_no = (U8)strtol(gCliArgs[2], NULL, 0);
                }
                GLogN("Certificate Number: %d\r\n\r\n", cert_no);

                U8 cert_data[700] = {0};
                U32 cert_len = 0;
                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using ActivationHSM + ReadCertificateHSM\r\n");
                    ret = ActivationHSM();
                    if (ret == HSM_SUCCESS)
                    {
                        ret = ReadCertificateHSM(cert_data, (int*)&cert_len, cert_no);
                    }
                }
                else
                {
                    GLogN("[NEW HSM] Using hsm_read_certificate\r\n");
                    U16 length = 0;
                    if (hsm_read_certificate(cert_no, HSM_ALG_RSA, cert_data, &length) == HAL_OK)
                    {
                        ret = HSM_SUCCESS;
                        cert_len = length;
                    }
                }

                if (ret == HSM_SUCCESS)
                {
                    GLogN("[OK] Certificate Read Success\r\n");
                    GLogN("  Total Length: %d bytes\r\n", cert_len);
                    GLogN("  For ECU: 600 bytes (fixed)\r\n\r\n");
                    GLogN("  First 32 bytes:\r\n    ");
                    for (int i = 0; i < 32 && i < cert_len; i++)
                    {
                        GLogN("%02X ", cert_data[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n    ");
                    }
                    GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] Certificate Read Failed (ret=%d)\r\n", ret);
                }
                GLogN("=================================================\r\n");
                break;
            }

            case 0xFA:  // Store Real Certificate (600 bytes)
            {
                GLogN("\r\n========== Store Real Certificate Test ==========\r\n");

                U8 cert_no = 141;  // RSA cert slot: 141~148
                if (count >= 3)
                {
                    cert_no = (U8)strtol(gCliArgs[2], NULL, 0);
                }
                GLogN("Certificate Number: %d\r\n\r\n", cert_no);

                // Real certificate data (600 bytes)
                static const U8 real_cert[600] = {
                    0x01, 0x48, 0x4D, 0x43, 0x03, 0x43, 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x23,
                    0x01, 0x13, 0x24, 0x01, 0x21, 0x00, 0x01, 0x00, 0x02, 0x04, 0x10, 0x00, 0x20, 0x01, 0x00, 0x00,
                    0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x4D, 0x43, 0x08,
                    0x47, 0x4B, 0x49, 0x41, 0x55, 0x50, 0x53, 0x33, 0x00, 0x00, 0x19, 0x93, 0x00, 0x60, 0x00, 0x07,
                    0x00, 0x01, 0x00, 0x02, 0x08, 0x40, 0x00, 0x11, 0x35, 0x49, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xCE, 0xEC, 0xD9, 0xF1, 0x58, 0x14, 0xC5, 0x97,
                    0xD3, 0xD5, 0x7F, 0xC4, 0xED, 0x64, 0x16, 0x23, 0xFC, 0x54, 0x2E, 0xB1, 0x8E, 0xCE, 0xFE, 0x52,
                    0x45, 0x75, 0xD8, 0xE3, 0x59, 0xD6, 0x90, 0x3B, 0x35, 0xE3, 0x59, 0x11, 0xFB, 0x4C, 0xC4, 0x04,
                    0x4C, 0x61, 0xB6, 0x98, 0x02, 0xFC, 0xC6, 0xB5, 0x55, 0xF7, 0x49, 0xD9, 0xC6, 0x8F, 0x7E, 0x2B,
                    0x13, 0x7F, 0x6E, 0xDA, 0x43, 0x5A, 0x53, 0x76, 0xD5, 0x37, 0x40, 0x53, 0xF6, 0x85, 0x6B, 0x12,
                    0xF4, 0x9C, 0x0E, 0xDA, 0x77, 0x30, 0x0B, 0x46, 0x87, 0x41, 0x4F, 0x87, 0x22, 0x58, 0x5F, 0x3E,
                    0xAC, 0x07, 0x64, 0xDD, 0xE0, 0xA9, 0xD2, 0x31, 0x25, 0x96, 0x60, 0x4A, 0xC6, 0xE8, 0xD9, 0x54,
                    0xB5, 0xAB, 0xA2, 0xF4, 0x4A, 0xDB, 0x00, 0x64, 0x6A, 0x8C, 0xB8, 0xC5, 0xFA, 0xD8, 0xDA, 0x32,
                    0x16, 0xD1, 0x05, 0xFA, 0x9C, 0x71, 0x1D, 0xB8, 0x7C, 0x7C, 0x80, 0x5D, 0x31, 0xD8, 0x91, 0x8E,
                    0x8F, 0xB8, 0xDE, 0x9D, 0x55, 0xFF, 0xD5, 0x95, 0x68, 0x5D, 0x6F, 0x27, 0xE5, 0x57, 0x51, 0xC7,
                    0xC9, 0xAF, 0xA3, 0x7A, 0x93, 0x9E, 0xC2, 0xA1, 0xB3, 0x29, 0xA6, 0x73, 0xC9, 0x8C, 0xD8, 0xC0,
                    0x5C, 0x3E, 0xF6, 0xB7, 0x47, 0xBD, 0x31, 0xAE, 0x8F, 0xBE, 0x8B, 0x6F, 0xAD, 0x61, 0x56, 0x9D,
                    0xFC, 0x9D, 0xF5, 0xEF, 0xC4, 0xA5, 0xF6, 0x70, 0xA2, 0x55, 0x2C, 0x50, 0x20, 0x2C, 0xA0, 0x38,
                    0x12, 0xDE, 0x65, 0xEF, 0xA8, 0xFA, 0xA3, 0x33, 0x79, 0x67, 0x0C, 0x3A, 0x64, 0x51, 0xC1, 0xEA,
                    0xC2, 0xDE, 0xCF, 0xDF, 0x1A, 0x68, 0xF9, 0x33, 0xB5, 0x8B, 0x04, 0xBC, 0x26, 0x07, 0x2D, 0x43,
                    0xA3, 0xF1, 0x6D, 0xA6, 0x47, 0x78, 0x66, 0xAE, 0xC4, 0x1B, 0xA7, 0xED, 0x59, 0x67, 0x56, 0x56,
                    0x07, 0x98, 0x7F, 0x77, 0x70, 0x49, 0x5C, 0x79, 0xB1, 0x21, 0x99, 0xB1, 0x27, 0x76, 0x9B, 0x9D,
                    0x1B, 0xFC, 0x88, 0x70, 0xEC, 0x6D, 0x35, 0x4B, 0x20, 0x72, 0xE0, 0x73, 0xD0, 0x1D, 0x1E, 0xF1,
                    0x07, 0xD0, 0x43, 0x86, 0xE4, 0x89, 0x69, 0xD0, 0xB0, 0xFC, 0x71, 0x63, 0x4C, 0x18, 0x6B, 0xCF,
                    0x4D, 0xE0, 0x91, 0x01, 0x9D, 0xF3, 0x06, 0xC7, 0xF1, 0xEE, 0x8B, 0x47, 0x9A, 0x3F, 0x26, 0x77,
                    0x3D, 0x9F, 0x8C, 0x30, 0x84, 0x9E, 0xFF, 0x14, 0x29, 0x66, 0x2D, 0xCF, 0xAB, 0x61, 0xFC, 0xA4,
                    0xB0, 0xE0, 0xA9, 0xFB, 0xA8, 0xC1, 0xC5, 0x0C, 0x76, 0x60, 0x92, 0xD6, 0x7D, 0xCD, 0x3E, 0x69,
                    0x75, 0x23, 0x7B, 0xBF, 0x7A, 0xC1, 0xCB, 0xCC, 0x6E, 0x66, 0xA2, 0xE4, 0x40, 0x94, 0x4A, 0x64,
                    0xF5, 0x38, 0xA2, 0x95, 0x38, 0xA4, 0x85, 0xD9, 0xD1, 0x5E, 0x85, 0xE3, 0x60, 0x30, 0xC0, 0x08,
                    0x70, 0xB1, 0xFC, 0xEE, 0xEE, 0x07, 0x41, 0x25, 0x0D, 0xDA, 0x94, 0x52, 0xA0, 0xA8, 0xEA, 0x2A,
                    0x91, 0xF0, 0x0F, 0xF8, 0x85, 0x9F, 0xDF, 0x96, 0x88, 0xBD, 0x84, 0x2C, 0xD3, 0xE4, 0x75, 0xB9,
                    0x04, 0xE3, 0xEA, 0x7F, 0x48, 0x6C, 0x18, 0x96, 0xED, 0xB2, 0x33, 0x7B, 0x7E, 0x5F, 0x91, 0x8D,
                    0x58, 0xB8, 0x1B, 0x6A, 0x53, 0xA9, 0xA3, 0xB3, 0x54, 0xC7, 0xCA, 0xC3, 0x64, 0x05, 0x04, 0xCE,
                    0x02, 0x27, 0x7F, 0x89, 0x0C, 0x8C, 0xF0, 0x1F, 0x64, 0x31, 0x76, 0x61, 0x6F, 0x7E, 0x5A, 0x9E,
                    0x20, 0x4E, 0xF2, 0x9D, 0x39, 0xED, 0xE6, 0xBB, 0x33, 0x5C, 0x5C, 0x86, 0x57, 0x72, 0x77, 0x21,
                    0x20, 0xB4, 0xC0, 0xEF, 0xBF, 0x85, 0xC1, 0x0C, 0x74, 0x84, 0x71, 0xDD, 0x9E, 0x1F, 0x42, 0xE1,
                    0xB7, 0x44, 0x41, 0xD4, 0x7B, 0x91, 0x80, 0xFD, 0x40, 0x67, 0xB3, 0xF4, 0x77, 0x3B, 0x1A, 0x67,
                    0xAF, 0xBF, 0x84, 0xBF, 0xC4, 0x33, 0xC3, 0xC9
                };

                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using StoreCertificateHSM\r\n");
                    ret = ActivationHSM();
                    if (ret == HSM_SUCCESS)
                    {
                        ret = StoreCertificateHSM((U8*)real_cert, 600, cert_no);
                    }
                }
                else
                {
                    GLogN("[NEW HSM] Using hsm_store_certificate\r\n");
                    if (hsm_store_certificate(cert_no, HSM_ALG_RSA, real_cert, 600) == HAL_OK)
                    {
                        ret = HSM_SUCCESS;
                    }
                }

                if (ret == HSM_SUCCESS)
                {
                    GLogN("[OK] Certificate Store Success\r\n");
                    GLogN("  First 16 bytes: ");
                    for (int i = 0; i < 16; i++) GLogN("%02X ", real_cert[i]);
                    GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] Certificate Store Failed (ret=%d)\r\n", ret);
                }
                GLogN("==================================================\r\n");
                break;
            }

            case 0xFB:  // Store Real Private Key (528 bytes)
            {
                GLogN("\r\n========== Store Real Private Key Test ==========\r\n");

                U8 key_no = 1;
                if (count >= 3)
                {
                    key_no = (U8)strtol(gCliArgs[2], NULL, 0);
                }
                GLogN("Key Number: %d\r\n\r\n", key_no);

                // Real private key data (528 bytes)
                static const U8 real_prk[528] = {
                    0xAE, 0x8C, 0x6D, 0x7C, 0x8D, 0x29, 0x56, 0x91, 0x84, 0x38, 0x02, 0xB8, 0xC4, 0xDF, 0x9C, 0x08,
                    0x0D, 0xC7, 0xDE, 0x6C, 0xD2, 0x73, 0xF4, 0x0C, 0xFD, 0x2C, 0x99, 0xAB, 0xA3, 0x81, 0xB6, 0xA8,
                    0xF9, 0x6E, 0x8B, 0xB1, 0xEA, 0x52, 0x87, 0x68, 0x29, 0x07, 0x29, 0x3F, 0xFB, 0xB8, 0x72, 0x90,
                    0xCA, 0x6D, 0x1C, 0xDB, 0xE1, 0x0D, 0x96, 0x6E, 0xDD, 0xD1, 0xB3, 0x44, 0x20, 0x77, 0xA9, 0x04,
                    0xF7, 0xB8, 0x08, 0x31, 0xDA, 0x65, 0x75, 0x29, 0x45, 0x69, 0x79, 0x51, 0x4A, 0x2B, 0xA3, 0xA8,
                    0xF1, 0x77, 0x38, 0xE7, 0x86, 0x81, 0xF6, 0xEA, 0x2B, 0x39, 0x96, 0x8D, 0xEF, 0x77, 0x8F, 0x97,
                    0xDC, 0x38, 0x5D, 0x55, 0x94, 0x38, 0xE1, 0x94, 0xDE, 0xB4, 0x21, 0x8D, 0xF8, 0x04, 0x96, 0xA3,
                    0x9E, 0xBE, 0x9E, 0xD1, 0xE0, 0x4B, 0xBF, 0x19, 0xA4, 0x57, 0x7A, 0x1A, 0xB4, 0x60, 0xF8, 0xEB,
                    0x15, 0xE4, 0x9D, 0x3A, 0x5B, 0x64, 0x37, 0xEB, 0xA7, 0x5D, 0x1E, 0x06, 0x2D, 0x5C, 0x2B, 0x00,
                    0xB1, 0x48, 0xAB, 0x68, 0x07, 0xBF, 0x4A, 0x92, 0x8E, 0xC6, 0x99, 0x34, 0xA7, 0x7C, 0x86, 0xBB,
                    0xBE, 0xA6, 0x01, 0xEC, 0xDD, 0x6C, 0xBE, 0x8E, 0x3E, 0xFA, 0xDA, 0xF7, 0x02, 0x1A, 0x62, 0x86,
                    0x4D, 0xBF, 0x09, 0xB6, 0xD5, 0x64, 0x18, 0x40, 0xFD, 0x15, 0xB6, 0x64, 0x2C, 0x7B, 0xCB, 0x3E,
                    0x26, 0xB8, 0x4B, 0xCC, 0x70, 0x2F, 0xD2, 0x3F, 0x9B, 0xEA, 0x58, 0x2D, 0x5C, 0x43, 0x16, 0x83,
                    0x59, 0xD9, 0xD3, 0x62, 0x50, 0xAB, 0x0E, 0x7E, 0x2F, 0xE7, 0xC7, 0x26, 0xE7, 0x85, 0xAE, 0x29,
                    0x2B, 0x1B, 0x58, 0xCB, 0x19, 0x57, 0xD3, 0xED, 0x66, 0xAB, 0xB2, 0xC8, 0x8D, 0x39, 0x78, 0xDE,
                    0x7A, 0xEC, 0x62, 0x2B, 0xED, 0xED, 0x40, 0x6F, 0xE3, 0x03, 0x88, 0xA9, 0xB7, 0x7D, 0x60, 0xF9,
                    0xA2, 0xDA, 0x22, 0x44, 0xD4, 0x8A, 0xA3, 0x52, 0xCD, 0xBC, 0x5A, 0xA6, 0x51, 0xF1, 0x17, 0x4A,
                    0x60, 0xE7, 0xF8, 0xDA, 0x44, 0x74, 0x66, 0xA6, 0x83, 0x09, 0x4B, 0xCC, 0x34, 0xF9, 0x47, 0x1D,
                    0x00, 0xD4, 0xB0, 0x67, 0xDA, 0x37, 0x64, 0x18, 0xF3, 0x6B, 0xFB, 0x5A, 0xEA, 0x6B, 0x1B, 0xF7,
                    0x97, 0xAB, 0x88, 0x25, 0xE5, 0xE8, 0x10, 0x02, 0x86, 0x9A, 0xD6, 0xF1, 0xB6, 0xF7, 0x87, 0x4B,
                    0x47, 0xF7, 0xF6, 0xCB, 0x5A, 0xA3, 0xCB, 0x8D, 0x20, 0x95, 0x23, 0x77, 0x9B, 0xA6, 0xFA, 0x7C,
                    0x7F, 0xAE, 0x03, 0xD8, 0x86, 0x9D, 0xD2, 0xB7, 0x55, 0x9F, 0x7D, 0x24, 0x62, 0xBA, 0x26, 0x32,
                    0xF0, 0x2A, 0x23, 0x42, 0x75, 0x45, 0x23, 0xF0, 0x10, 0xFC, 0x6F, 0x2C, 0xB3, 0x1E, 0xDA, 0xB5,
                    0x5F, 0x7F, 0x67, 0x4F, 0xF6, 0x3B, 0xC7, 0x7A, 0x62, 0x4F, 0xAE, 0xF3, 0xEE, 0x71, 0x1F, 0xC4,
                    0x09, 0xE5, 0xCF, 0x24, 0xD8, 0xAC, 0x21, 0xF0, 0xBA, 0x3B, 0x60, 0x86, 0x5E, 0x5D, 0x4D, 0x0B,
                    0x7A, 0x53, 0xE1, 0x1A, 0xFE, 0x5F, 0x3B, 0x1F, 0xB6, 0x42, 0x9E, 0xB3, 0x65, 0x17, 0xA2, 0xC7,
                    0x09, 0xA0, 0x92, 0xDA, 0xB1, 0x94, 0x9F, 0x05, 0x17, 0xCC, 0x3F, 0xF2, 0x60, 0x54, 0x02, 0x11,
                    0x22, 0x6A, 0x78, 0x8A, 0x28, 0x66, 0x26, 0xF4, 0x42, 0x22, 0x93, 0x3E, 0xE3, 0xD1, 0x6D, 0x1F,
                    0x5F, 0x84, 0xE1, 0x21, 0x37, 0xC7, 0x4C, 0xD0, 0x07, 0x64, 0xC3, 0xDB, 0x98, 0x17, 0xDE, 0xF5,
                    0xA8, 0x9A, 0x27, 0xFE, 0xE5, 0x57, 0xD9, 0x97, 0x12, 0xC4, 0x5F, 0x79, 0xEF, 0xA4, 0xC4, 0x40,
                    0x73, 0x56, 0x1B, 0xE0, 0x2A, 0x3D, 0x8C, 0x17, 0xBC, 0xE0, 0xA1, 0xF4, 0xEB, 0xAE, 0xC7, 0xCE,
                    0x60, 0x7A, 0x39, 0xE1, 0xB7, 0x93, 0x6F, 0xC9, 0xDA, 0x11, 0x10, 0x4B, 0x15, 0x0E, 0xC2, 0xE6
                };

                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using StorePrivateKeyHSM\r\n");
                    ret = ActivationHSM();
                    if (ret == HSM_SUCCESS)
                    {
                        ret = StorePrivateKeyHSM((U8*)real_prk, 528, key_no);
                    }
                }
                else
                {
                    GLogN("[NEW HSM] Private Key storage not supported via SPI\r\n");
                    GLogN("  New HSM generates keys internally\r\n");
                    ret = HSM_SUCCESS;  // Skip for New HSM
                }

                if (ret == HSM_SUCCESS)
                {
                    GLogN("[OK] Private Key Store Success\r\n");
                    GLogN("  First 16 bytes: ");
                    for (int i = 0; i < 16; i++) GLogN("%02X ", real_prk[i]);
                    GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] Private Key Store Failed (ret=%d)\r\n", ret);
                }
                GLogN("==================================================\r\n");
                break;
            }

            case 0xFC:  // Store Real CRL (501 bytes)
            {
                GLogN("\r\n========== Store Real CRL Test ==========\r\n");

                U8 crl_no = 1;
                if (count >= 3)
                {
                    crl_no = (U8)strtol(gCliArgs[2], NULL, 0);
                }
                GLogN("CRL Number: %d\r\n\r\n", crl_no);

                // Real CRL data (501 bytes)
                static const U8 real_crl[501] = {
                    0x01, 0x00, 0x01, 0x00, 0x02, 0x08, 0x40, 0x00, 0x11, 0x35, 0x49, 0x00, 0x01, 0x00, 0x01, 0x00,
                    0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x4D, 0x43, 0x03, 0x43, 0x41, 0x01, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x48, 0x4D, 0x43, 0x08, 0x47, 0x4B, 0x49, 0x41, 0x55, 0x50,
                    0x53, 0x33, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x10, 0x00, 0x37, 0x78, 0xE8, 0x25, 0xD3, 0x65,
                    0x37, 0x0D, 0xEE, 0x58, 0xEA, 0x74, 0x9A, 0xF4, 0x5D, 0x43, 0xBE, 0x27, 0xC2, 0x9C, 0x84, 0x78,
                    0x66, 0x4B, 0x58, 0xF6, 0x93, 0x85, 0xDF, 0xC3, 0xBF, 0xB2, 0x23, 0x10, 0x24, 0x23, 0x11, 0x28,
                    0x00, 0x01, 0x90, 0x01, 0x00, 0x00, 0x01, 0x26, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // +9 bytes to make 501 total
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0xDD, 0x73, 0x15,
                    0x96, 0x10, 0x25, 0xCA, 0xDF, 0xC9, 0xA5, 0xA0, 0x23, 0xC5, 0xD1, 0xF6, 0xD0, 0x16, 0x90, 0xA9,
                    0x0F, 0x2A, 0xFD, 0x04, 0x98, 0x31, 0x2F, 0xA4, 0x86, 0xEA, 0xA4, 0xCF, 0x39, 0x6B, 0x39, 0x9E,
                    0xD3, 0xE7, 0xB5, 0xC5, 0x74, 0x77, 0x23, 0x22, 0x6C, 0x60, 0x28, 0x5F, 0x63, 0x66, 0xAB, 0x9D,
                    0xA3, 0xA3, 0xEA, 0xBC, 0xDF, 0xBC, 0x19, 0x67, 0x7A, 0x8E, 0xAD, 0xCB, 0x1F, 0xAB, 0x01, 0xB8,
                    0x22, 0x1B, 0x41, 0xF2, 0x65, 0xAD, 0x85, 0xA1, 0xC4, 0xF2, 0x80, 0x80, 0x4F, 0xF5, 0x14, 0x17,
                    0x53, 0x42, 0xF4, 0x8A, 0x8B, 0x3E, 0xA8, 0x1B, 0xA1, 0x95, 0xE3, 0xA0, 0x81, 0x5F, 0x4A, 0xFA,
                    0xB9, 0x9B, 0x2D, 0xDB, 0xDC, 0x00, 0xA9, 0xC1, 0x39, 0x0E, 0x00, 0x79, 0xDC, 0x11, 0xC7, 0xCB,
                    0x36, 0xA3, 0xC2, 0xAD, 0x01, 0x48, 0x67, 0x6D, 0xAC, 0x57, 0xAD, 0x29, 0x20, 0xE8, 0x3D, 0x13,
                    0x65, 0x0D, 0xE1, 0x46, 0xAC, 0xB0, 0x0C, 0x9F, 0xAD, 0x9D, 0xA1, 0xEF, 0x31, 0x86, 0x62, 0x21,
                    0x54, 0x5B, 0x00, 0xC2, 0xCF, 0x88, 0x8A, 0x77, 0x0E, 0x26, 0x36, 0xFC, 0x1A, 0x15, 0xAF, 0x0D,
                    0xDB, 0x05, 0x19, 0xE8, 0xF9, 0x26, 0xFE, 0x68, 0x58, 0x01, 0x28, 0x34, 0x85, 0x36, 0xCC, 0x14,
                    0x82, 0x4D, 0x34, 0xA1, 0x6D, 0x2D, 0x13, 0xD3, 0x09, 0xB1, 0x97, 0xCF, 0x22, 0xB8, 0x9A, 0x95,
                    0xF4, 0x36, 0x5C, 0x74, 0xEE, 0x84, 0x94, 0xD9, 0x67, 0xBF, 0x1F, 0xA4, 0x09, 0xAD, 0x9C, 0x41,
                    0xCD, 0x6F, 0x88, 0x64, 0x87, 0x10, 0x61, 0x4D, 0xDC, 0x73, 0x71, 0x6D, 0x42, 0x73, 0x62, 0x88,
                    0xE5, 0x2F, 0x26, 0x45, 0x61, 0xAA, 0x02, 0xFF, 0x8D, 0x5E, 0x61, 0xCD, 0x81, 0x43, 0x3C, 0x5F,
                    0x73, 0x28, 0xCC, 0x50, 0xF6, 0xFF, 0xD9, 0x5B, 0x36, 0x5B, 0xDE, 0xE1
                };

                U32 ret = HSM_UNKNOWN_ERROR;

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogN("[OLD HSM] Using WriteDataHSM\r\n");
                    ret = WriteDataHSM((U8*)real_crl, 501, crl_no);
                }
                else
                {
                    GLogN("[NEW HSM] Using StoreCrlEmmc\r\n");
                    if (StoreCrlEmmc(crl_no, (U8*)real_crl, 501) == FR_OK)
                    {
                        ret = HSM_SUCCESS;
                    }
                }

                if (ret == HSM_SUCCESS)
                {
                    GLogN("[OK] CRL Store Success\r\n");
                    GLogN("  First 16 bytes: ");
                    for (int i = 0; i < 16; i++) GLogN("%02X ", real_crl[i]);
                    GLogN("\r\n");
                }
                else
                {
                    GLogE("[FAIL] CRL Store Failed (ret=%d)\r\n", ret);
                }
                GLogN("==========================================\r\n");
                break;
            }

            case 0xFD:  // RSA Sign SHA1 Test (New HSM only)
            {
                GLogN("\r\n========== RSA Sign SHA1 Test ==========\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    GLogN("  For Old HSM, use original CSAC flow\r\n");
                    break;
                }

                // Parse key_slot argument: hsmtest 0xFD [key_slot]
                U16 key_slot = HSM_KEY_CSAC_PA_SHA1;  // Default: 141
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }
                GLogN("Key Slot: %d\r\n", key_slot);

                // Test seed (8 bytes)
                //static const U8 test_seed[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
				static const U8 test_seed[8] = {0x04, 0xC6, 0x44, 0x4D, 0x8A, 0x84, 0x07, 0x7D};
                U8 signature[256] = {0};
                U32 sign_len = 0;

                GLogN("Test Seed: ");
                for (int i = 0; i < 8; i++) GLogN("%02X ", test_seed[i]);
                GLogN("\r\n\r\n");

                //HAL_StatusTypeDef status = hsm_sign_rsa_with_seed_direct(test_seed, 8, signature, &sign_len,
                //                                                   HSM_SHA_160, key_slot);
				HAL_StatusTypeDef status = hsm_sign_rsa_with_seed_new(test_seed, 8, signature, &sign_len,
                                                                   key_slot, HSM_SHA_160 );

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Sign SHA1 Success (len=%d)\r\n\r\n", sign_len);
                    GLogN("Signature (first 32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("...\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Sign SHA1 Failed (status=%d)\r\n", status);
                    GLogE("  Check if RSA private key exists at slot %d\r\n", key_slot);
                }
                GLogN("==========================================\r\n");
                break;
            }

            case 0xFE:  // RSA Sign SHA256 Test (New HSM only)
            {
                GLogN("\r\n========== RSA Sign SHA256 Test ==========\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    GLogN("  For Old HSM, use original CSAC flow\r\n");
                    break;
                }

                // Parse key_slot argument: hsmtest 0xFE [key_slot]
                U16 key_slot = HSM_KEY_CSAC_PA_SHA2;  // Default: 142
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }
                GLogN("Key Slot: %d\r\n", key_slot);

                // Test seed (8 bytes)
                static const U8 test_seed[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
                U8 signature[256] = {0};
                U32 sign_len = 0;

                GLogN("Test Seed: ");
                for (int i = 0; i < 8; i++) GLogN("%02X ", test_seed[i]);
                GLogN("\r\n\r\n");

                HAL_StatusTypeDef status = hsm_sign_rsa_with_seed(test_seed, 8, signature, &sign_len,
                                                                   HSM_SHA_256, key_slot);

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Sign SHA256 Success (len=%d)\r\n\r\n", sign_len);
                    GLogN("Signature (first 32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("...\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Sign SHA256 Failed (status=%d)\r\n", status);
                    GLogE("  Check if RSA private key exists at slot %d\r\n", key_slot);
                }
                GLogN("==========================================\r\n");
                break;
            }

            case 0x100:  // WBC Compatibility Test - SignPrivateHSM (Raw RSA, No Hash)
            {
                GLogN("\r\n");
                GLogN("╔══════════════════════════════════════════════════════════════╗\r\n");
                GLogN("║  WBC Compatibility Test - SignPrivateHSM                     ║\r\n");
                GLogN("║  (Raw RSA with SEED_PADDING, NO hash calculation)            ║\r\n");
                GLogN("╚══════════════════════════════════════════════════════════════╝\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    GLogN("  Testing New HSM WBC compatibility\r\n");
                    break;
                }

                // Parse key_slot argument: hsmtest 0x100 [key_slot]
                U16 key_slot = HSM_KEY_CSAC_PA_SHA1;  // Default: 141
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                /* WBC Test Seed: [126, 56, -10, 60, 37, 100, 71, -74] (signed)
                 * Converted to unsigned: 0x7E 0x38 0xF6 0x3C 0x25 0x64 0x47 0xB6
                 */
                static const U8 test_seed[8] = {0x7E, 0x38, 0xF6, 0x3C, 0x25, 0x64, 0x47, 0xB6};

                /* Expected WBC Result (verified from sha1_sign_tool.html)
                 * Algorithm: SEED_PADDING + seed @ offset 235 -> Raw RSA (EM^d mod n)
                 * NO hash calculation - seed placed directly in hash position!
                 */
                static const U8 expected_wbc_sig[256] = {
                    0xC9, 0x77, 0x64, 0xE9, 0x77, 0x3F, 0xB4, 0x80, 0x2E, 0x21, 0xE3, 0x26, 0xA6, 0x61, 0x28, 0x2F,
                    0xCD, 0x6A, 0x81, 0xEA, 0x2F, 0xD4, 0xC0, 0xA1, 0x35, 0x2F, 0x18, 0xA6, 0xF1, 0x81, 0xF5, 0x2F,
                    0x4B, 0x30, 0x28, 0x62, 0x91, 0xB2, 0xC3, 0xE5, 0x4B, 0xA0, 0x08, 0xAA, 0x9D, 0x09, 0xD7, 0xB6,
                    0xCF, 0xF3, 0x41, 0x06, 0xA6, 0x53, 0xA1, 0x90, 0x27, 0xCE, 0x0D, 0x7D, 0x7D, 0xCC, 0xED, 0x15,
                    0xC8, 0xB8, 0xC9, 0x77, 0xE9, 0x92, 0x11, 0x72, 0xBF, 0x12, 0x06, 0x51, 0xCC, 0xC5, 0x1D, 0xAA,
                    0x1F, 0xE2, 0x93, 0xCF, 0xE1, 0xB6, 0x09, 0x48, 0x61, 0x64, 0x6E, 0x8F, 0xCD, 0x0A, 0xA8, 0xD1,
                    0xBD, 0x14, 0xD7, 0x32, 0x7D, 0x1F, 0xCF, 0xC0, 0xDC, 0xB7, 0x92, 0xA6, 0x1A, 0xA5, 0x7A, 0x89,
                    0x0C, 0xCA, 0xBF, 0x9D, 0xF3, 0xEB, 0xB6, 0xB7, 0xFA, 0x39, 0x25, 0x0B, 0x7C, 0x45, 0x79, 0x1A,
                    0x48, 0x3A, 0xD4, 0xE7, 0xB2, 0x52, 0x35, 0x50, 0x5E, 0x2C, 0x28, 0x1B, 0x78, 0x9E, 0x65, 0x3A,
                    0x0C, 0x0F, 0x74, 0x57, 0xF4, 0x29, 0xC6, 0x2F, 0x21, 0x90, 0x30, 0xBE, 0xEE, 0xA4, 0x87, 0xB0,
                    0x6D, 0xD1, 0x12, 0xCD, 0x29, 0x71, 0x8E, 0xEE, 0x16, 0x43, 0x9F, 0x87, 0xB8, 0xA7, 0x34, 0xE5,
                    0x80, 0x03, 0x20, 0xC3, 0x17, 0x52, 0x16, 0x7B, 0x72, 0xE5, 0xC9, 0xB4, 0x11, 0xE7, 0x95, 0x2E,
                    0x5B, 0x7B, 0xCB, 0xF9, 0x80, 0xAB, 0x75, 0xFF, 0xB4, 0xB9, 0x41, 0x41, 0x46, 0xAB, 0x6E, 0xBD,
                    0xF2, 0x57, 0xA1, 0xE0, 0x99, 0x15, 0x45, 0xEB, 0xC2, 0x71, 0x26, 0x2E, 0xDA, 0x14, 0xEB, 0x57,
                    0x4E, 0x44, 0xC0, 0x17, 0xFD, 0x9B, 0x90, 0x81, 0x2B, 0xBE, 0xF1, 0xA2, 0xE1, 0x0C, 0xA0, 0x79,
                    0xD2, 0xD4, 0xA4, 0x15, 0x55, 0x2F, 0xFA, 0x76, 0x91, 0x22, 0xBE, 0x24, 0x8A, 0x56, 0xD8, 0x64
                };

                U8 signature[256] = {0};
                U32 sign_len = 0;

                GLogN("WBC Algorithm:\r\n");
                GLogN("  1. SEED_PADDING: 01 + FF*218 + 00 + SHA1_DigestInfo(15B) + 00*20\r\n");
                GLogN("  2. Place seed(8B) directly at offset 235 (NO HASH!)\r\n");
                GLogN("  3. Prepend 0x00 -> 256 bytes\r\n");
                GLogN("  4. Raw RSA: signature = padded^d mod n\r\n\r\n");

                GLogN("Test Parameters:\r\n");
                GLogN("  Key Slot: %d\r\n", key_slot);
                GLogN("  Seed (8 bytes): ");
                for (int i = 0; i < 8; i++) GLogN("%02X ", test_seed[i]);
                GLogN("\r\n\r\n");

                // Call WBC compatible function with SHA-1 mode (NO hash, using SIGN internally)
                HAL_StatusTypeDef status = hsm_sign_rsa_with_seed_new(test_seed, 8, signature, &sign_len, key_slot, HSM_SHA_160);

                if (status == HAL_OK)
                {
                    GLogN("[OK] hsm_sign_rsa_with_seed_new: Success (len=%lu)\r\n\r\n", sign_len);

                    // Display signature (first 64 bytes)
                    GLogN("New HSM Signature (first 64 bytes):\r\n");
                    for (int i = 0; i < 64; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("\r\n");

                    // Compare with expected (full 256 bytes)
                    int match = 1;
                    int first_mismatch = -1;
                    for (int i = 0; i < 256; i++)
                    {
                        if (signature[i] != expected_wbc_sig[i])
                        {
                            if (first_mismatch < 0) first_mismatch = i;
                            match = 0;
                        }
                    }

                    GLogN("Expected WBC Signature (first 64 bytes):\r\n");
                    for (int i = 0; i < 64; i++)
                    {
                        GLogN("%02X ", expected_wbc_sig[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("\r\n");

                    // Full signature display
                    GLogN("Full New HSM Signature:\r\n");
                    for (int i = 0; i < 256; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("\r\n");

                    if (match)
                    {
                        GLogN("╔══════════════════════════════════════════════════════════════╗\r\n");
                        GLogN("║  [PASS] New HSM output matches WBC expected value!           ║\r\n");
                        GLogN("║         WBC compatibility verified!                          ║\r\n");
                        GLogN("╚══════════════════════════════════════════════════════════════╝\r\n");
                    }
                    else
                    {
                        GLogE("╔══════════════════════════════════════════════════════════════╗\r\n");
                        GLogE("║  [FAIL] New HSM output does NOT match WBC expected!          ║\r\n");
                        GLogE("║         First mismatch at byte %3d                           ║\r\n", first_mismatch);
                        GLogE("║         WBC implementation needs adjustment                  ║\r\n");
                        GLogE("╚══════════════════════════════════════════════════════════════╝\r\n");

                        // Show difference details
                        GLogE("\r\nMismatch details (first 5):\r\n");
                        int shown = 0;
                        for (int i = 0; i < 256 && shown < 5; i++)
                        {
                            if (signature[i] != expected_wbc_sig[i])
                            {
                                GLogE("  [%3d] Got: 0x%02X, Expected: 0x%02X\r\n", i, signature[i], expected_wbc_sig[i]);
                                shown++;
                            }
                        }
                    }
                }
                else
                {
                    GLogE("[FAIL] hsm_sign_rsa_with_seed_new failed (status=%d)\r\n", status);
                    GLogE("  Check if RSA private key exists at slot %d\r\n", key_slot);
                    GLogE("  Run 'hsmtest 0x102' first to import the WBC key\r\n");
                }
                break;
            }

            case 0x101:  // SHA1 Sign with SEED_PADDING (Old HSM SignPrivateHSM compatible)
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  SHA1 Sign with SEED_PADDING                                    \r\n");
                GLogN("  (8-byte Seed -> SEED_PADDING -> SHA1 Hash -> RSA Sign)         \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    GLogN("  Testing New HSM compatibility with Old HSM result\r\n");
                    break;
                }

                // Parse key_slot argument: hsmtest 0x101 [key_slot]
                U16 key_slot = HSM_KEY_CSAC_PA_SHA1;  // Default: 141 (SHA1 key)
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                // Test seed: 8 bytes
                static const U8 test_seed[8] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

                // SEED_PADDING from SignPrivateHSM (255 bytes)
                // Structure: 0x01 + 0xFF*218 + 0x00 + DigestInfo(15) + Hash(20)
                static const U8 SEED_PADDING[255] = {
                    0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
                    // SHA1 DigestInfo (15 bytes): 30 21 30 09 06 05 2b 0e 03 02 1a 05 00 04 14
                    0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14,
                    // Hash position (20 bytes) - will be filled with seed + zero padding
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                };

                // Step 1: Create padded data (SEED_PADDING + seed at offset 235)
                U8 padded_data[255];
                memcpy(padded_data, SEED_PADDING, 255);
                memcpy(&padded_data[235], test_seed, 8);  // Place 8-byte seed at hash position

                GLogN("Test Parameters:\r\n");
                GLogN("  Key Slot: %d\r\n", key_slot);
                GLogN("  Seed (8 bytes): ");
                for (int i = 0; i < 8; i++) GLogN("%02X ", test_seed[i]);
                GLogN("\r\n");
                GLogN("  Method: SEED_PADDING + SHA1 Hash + RSA Sign\r\n\r\n");

                GLogN("Padded Data (last 32 bytes showing DigestInfo + Hash position):\r\n");
                for (int i = 223; i < 255; i++)
                {
                    GLogN("%02X ", padded_data[i]);
                    if ((i - 223 + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                // Step 2: Calculate SHA1 hash of padded_data (255 bytes)
                U8 sha1_hash[20] = {0};
                HAL_StatusTypeDef status = hsm_calculate_sha(HSM_SHA_160, false, 0, padded_data, 255, sha1_hash);

                if (status != HAL_OK)
                {
                    GLogE("[FAIL] SHA1 hash calculation failed (status=%d)\r\n", status);
                    break;
                }

                GLogN("SHA1 Hash of padded data (20 bytes):\r\n");
                for (int i = 0; i < 20; i++) GLogN("%02X ", sha1_hash[i]);
                GLogN("\r\n\r\n");

                // Step 3: RSA Sign with SHA1 hash
                U8 signature[256] = {0};
                status = hsm_sign_rsa(key_slot, HSM_SHA_160, sha1_hash, signature);

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Sign Success\r\n\r\n");

                    // Display signature (first 64 bytes)
                    GLogN("RSA Signature (first 64 bytes):\r\n");
                    for (int i = 0; i < 64; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("...\r\n\r\n");

                    // Display full signature
                    GLogN("Full Signature (256 bytes):\r\n");
                    for (int i = 0; i < 256; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("\r\n");

                    GLogN("==================================================================\r\n");
                    GLogN("  [DONE] SEED_PADDING + SHA1 -> RSA Sign completed               \r\n");
                    GLogN("==================================================================\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Sign failed (status=%d)\r\n", status);
                    GLogE("  Check if RSA private key exists at slot %d\r\n", key_slot);
                }
                break;
            }
            case 0x102:  // RSA Private Key Import - WBC Compatible Key
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  RSA Private Key Import - WBC Compatible Key                   \r\n");
                GLogN("  Import user-provided RSA key for WBC compatibility test        \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse key_slot argument: hsmtest 0x102 [key_slot]
                U16 key_slot = HSM_KEY_CSAC_PA_SHA1;  // Default: 141
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                /* WBC Compatible RSA 2048-bit Key
                 * Source: User-provided key from sha1_sign_tool.html
                 * Format: [Modulus(256)][Skip(16)][PrivateExp(256)] = 528 bytes
                 */
                static const U8 test_modulus[256] = {
                    0xEB, 0x64, 0x16, 0xC7, 0xF6, 0x6C, 0x21, 0x0A, 0xDE, 0xA6, 0x8D, 0x7F, 0xFB, 0x2A, 0xAA, 0xAE,
                    0x9E, 0xE0, 0x2A, 0x7D, 0xA4, 0x9F, 0xD4, 0xD0, 0x29, 0x45, 0x3A, 0x7F, 0xB1, 0x9E, 0xD5, 0x69,
                    0xBF, 0x44, 0x20, 0x95, 0x1D, 0xA5, 0x05, 0xA9, 0x7F, 0x18, 0x92, 0xD4, 0xCF, 0xED, 0x63, 0xC2,
                    0xF9, 0xEA, 0xE6, 0x05, 0x1F, 0x59, 0xE3, 0x58, 0x1A, 0x43, 0x8A, 0x22, 0xF0, 0x10, 0xFF, 0xE3,
                    0x34, 0x0D, 0x11, 0xC4, 0x59, 0x0C, 0x35, 0xB4, 0x63, 0x78, 0x3A, 0x2E, 0x27, 0x88, 0xC7, 0x65,
                    0xEF, 0x23, 0xE0, 0x8C, 0xA0, 0x27, 0x1B, 0x6E, 0x6D, 0x88, 0xE2, 0x39, 0x1E, 0x3A, 0x86, 0x27,
                    0xA6, 0xEC, 0x3E, 0x93, 0x0D, 0x59, 0x34, 0xDF, 0xB3, 0x1B, 0x9F, 0xBF, 0x3C, 0x8D, 0x1F, 0x25,
                    0x08, 0xBF, 0x59, 0x24, 0x6C, 0xAB, 0x3C, 0x31, 0x89, 0xDF, 0x24, 0xDF, 0x32, 0x8C, 0x9D, 0xD5,
                    0x48, 0x1E, 0x0A, 0xD0, 0x62, 0x00, 0xFA, 0x5F, 0x4B, 0x26, 0xB7, 0xFC, 0xC7, 0x1A, 0x76, 0x1A,
                    0xA0, 0xAB, 0xD4, 0x4B, 0x4A, 0x11, 0x6C, 0x6B, 0x8B, 0xA6, 0x17, 0x5A, 0xF9, 0x42, 0xD2, 0x84,
                    0x4D, 0x17, 0x76, 0x09, 0xD1, 0x3D, 0xB0, 0x99, 0x4D, 0x73, 0x1C, 0xE4, 0x87, 0x9F, 0x06, 0x14,
                    0xE2, 0x8A, 0x05, 0x6C, 0xAD, 0x57, 0x24, 0x3E, 0x11, 0x44, 0x3E, 0x0B, 0x17, 0xE8, 0xC6, 0x0B,
                    0x8F, 0xD3, 0xBE, 0xAB, 0xB1, 0x79, 0xC4, 0x0A, 0x63, 0x23, 0xDD, 0x8F, 0x30, 0x4E, 0xD9, 0x1C,
                    0xC3, 0x8B, 0x39, 0xC0, 0xB4, 0x0A, 0x95, 0xEB, 0x18, 0xC9, 0xF2, 0x16, 0x37, 0xD2, 0x73, 0x02,
                    0x37, 0xA4, 0xC5, 0x64, 0xDD, 0x2E, 0x46, 0xAF, 0x16, 0x92, 0x17, 0x41, 0x19, 0x01, 0x63, 0x73,
                    0x0C, 0xE7, 0xA9, 0xB4, 0xDA, 0xCB, 0xAB, 0x7D, 0x24, 0xF7, 0xF1, 0xE4, 0x3D, 0x76, 0xC0, 0xA7
                };

                static const U8 test_private_exp[256] = {
                    0xDF, 0x7C, 0x01, 0x9D, 0x0C, 0x3B, 0x11, 0x0C, 0x0E, 0xE4, 0x36, 0x88, 0x01, 0xE1, 0x3A, 0x77,
                    0xDB, 0xE3, 0x9C, 0xB3, 0xF9, 0x6E, 0xBE, 0x50, 0x7C, 0x3E, 0x7C, 0x11, 0xEC, 0x83, 0x8E, 0xBF,
                    0x7D, 0x96, 0xA3, 0x10, 0xB0, 0x3A, 0x93, 0x2B, 0x9F, 0xBD, 0xA6, 0xFA, 0x62, 0x07, 0x52, 0xA6,
                    0x35, 0x3D, 0x6D, 0xF2, 0x03, 0x18, 0x06, 0x9F, 0x09, 0x20, 0x82, 0xB0, 0x35, 0x19, 0x0E, 0xB9,
                    0x9A, 0x1B, 0xA6, 0x78, 0xB2, 0xCA, 0xC2, 0xDA, 0x67, 0x6E, 0x28, 0x5E, 0xD5, 0xAE, 0x29, 0x12,
                    0x80, 0x7F, 0xA1, 0x9C, 0x22, 0x19, 0xBF, 0x13, 0x92, 0xEF, 0x59, 0x49, 0x3D, 0x87, 0xF7, 0x03,
                    0x02, 0x9A, 0x5D, 0xB6, 0xA3, 0xFF, 0xC4, 0x11, 0x77, 0x2D, 0x8C, 0xF7, 0xB5, 0xF7, 0x8E, 0x50,
                    0x4D, 0x03, 0xCF, 0x10, 0xA4, 0x13, 0xD7, 0x6C, 0x79, 0x27, 0xE1, 0x0C, 0x00, 0x24, 0x3D, 0x5C,
                    0x05, 0x96, 0x41, 0xBC, 0xC3, 0x65, 0x7B, 0x62, 0xBC, 0xDD, 0xE2, 0x08, 0x7E, 0x55, 0x20, 0x2A,
                    0x1F, 0x42, 0x7E, 0x2E, 0x20, 0xB3, 0x41, 0xE4, 0x16, 0x93, 0x7F, 0x3F, 0x95, 0x2A, 0x10, 0xA4,
                    0x03, 0x32, 0xAB, 0x05, 0xB5, 0xD2, 0x61, 0xA8, 0x9C, 0x44, 0xA0, 0x50, 0xBA, 0x49, 0x8F, 0xDF,
                    0xA6, 0xA7, 0x59, 0x05, 0xBB, 0xC1, 0xBD, 0x25, 0x42, 0x6D, 0x35, 0xEC, 0x9C, 0x08, 0x1E, 0xCD,
                    0xA5, 0xC0, 0xFA, 0xE0, 0xFA, 0x90, 0xFF, 0x96, 0x40, 0x12, 0x5B, 0x1B, 0x41, 0x35, 0x18, 0xDF,
                    0xBB, 0xA6, 0x8F, 0x0F, 0x8F, 0xEC, 0x9A, 0xBD, 0x07, 0xF3, 0x5B, 0x21, 0x18, 0x61, 0xC5, 0x23,
                    0x65, 0x60, 0x4C, 0x51, 0xFC, 0x71, 0xB5, 0xFD, 0x92, 0xD8, 0x78, 0x80, 0x8E, 0x19, 0x2A, 0x17,
                    0x0E, 0x68, 0x09, 0xD8, 0x49, 0x93, 0x45, 0xD1, 0xE0, 0x76, 0x24, 0xC0, 0xB8, 0xF5, 0xD8, 0xD1
                };

                // Public exponent: 65537 (0x010001)
                static const U8 test_public_exp[3] = { 0x01, 0x00, 0x01 };

                GLogN("WBC Compatible Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", key_slot);
                GLogN("  Algorithm: RSA-2048\r\n");
                GLogN("  Public Exponent: 65537 (0x010001)\r\n\r\n");

                GLogN("Modulus (first 32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_modulus[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                // Build RSA key structure
                HSM_RSAKey_t rsa_key;
                memset(&rsa_key, 0, sizeof(rsa_key));
                rsa_key.modulus_length = 256;
                memcpy(rsa_key.modulus, test_modulus, 256);
                rsa_key.public_exp_size = 3;
                memcpy(rsa_key.public_exp, test_public_exp, 3);
                memcpy(rsa_key.private_exp, test_private_exp, 256);

                GLogN("Importing WBC RSA Private Key to slot %d...\r\n", key_slot);

                // Import with private key (public_only=false)
                HAL_StatusTypeDef status = hsm_import_rsa_key(key_slot, &rsa_key, false, false, false, HSM_RSA_2048);

                if (status == HAL_OK)
                {
                    GLogN("[OK] WBC RSA Private Key imported to slot %d\r\n", key_slot);
                    GLogN("     Ready for WBC compatibility test (hsmtest 0x100)\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Key Import failed, status: %d\r\n", status);
                }
                break;
            }

            case 0x103:  // RSA Private Key Import to slot 142 (SHA256 compatibility test)
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  RSA Private Key Import - Slot 142 (CSAC_PA_SHA2)              \r\n");
                GLogN("  Import test RSA key for SHA256 compatibility verification     \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse key_slot argument: hsmtest 0x103 [key_slot]
                U16 key_slot = HSM_KEY_CSAC_PA_SHA2;  // Default: 142
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                // Test RSA 2048-bit Key (same key as 0x102)
                static const U8 test_modulus[256] = {
                    0x5F, 0x7C, 0x8E, 0x1B, 0x2C, 0x3B, 0x0B, 0x27, 0x6C, 0x9B, 0xC4, 0xB8, 0x0C, 0xF2, 0x16, 0x0C,
                    0x9C, 0x05, 0x5D, 0xA0, 0x7F, 0xDE, 0x6C, 0x28, 0xE9, 0x15, 0xAC, 0x39, 0x08, 0x7D, 0xF6, 0xD4,
                    0x01, 0x61, 0xB4, 0x34, 0xA6, 0x66, 0x3D, 0xC8, 0x10, 0xE7, 0x48, 0x68, 0x6A, 0xB4, 0xB8, 0x21,
                    0xAA, 0xC3, 0x41, 0x10, 0x48, 0xD8, 0xF0, 0x06, 0xF2, 0xE1, 0xD0, 0xD7, 0x7D, 0xC0, 0xE6, 0x74,
                    0x27, 0x8D, 0x9B, 0x60, 0xD1, 0x37, 0x84, 0xA2, 0xF5, 0xB3, 0xBF, 0x7B, 0xE8, 0x65, 0xD8, 0x71,
                    0x65, 0x57, 0x56, 0xD9, 0xCC, 0x11, 0xC9, 0xD4, 0x08, 0x74, 0x1A, 0x27, 0x9A, 0xBB, 0x6B, 0xC7,
                    0x9D, 0xA9, 0x46, 0xB1, 0x82, 0x89, 0xBA, 0xA1, 0x40, 0xA0, 0x4E, 0x5C, 0xBF, 0xAF, 0x2D, 0x2C,
                    0x3D, 0xE8, 0x8C, 0x96, 0xE6, 0xEC, 0x61, 0xFA, 0x4D, 0x1C, 0x1A, 0x50, 0xCA, 0xAA, 0x00, 0xB8,
                    0x40, 0x84, 0xC8, 0x8A, 0x6F, 0x18, 0x4D, 0x28, 0x8D, 0x28, 0xDA, 0x8C, 0x31, 0xB8, 0xE5, 0x4C,
                    0x84, 0x50, 0x10, 0x9C, 0xC8, 0x8F, 0x25, 0xB2, 0x4A, 0xB1, 0x62, 0x05, 0xF4, 0xCB, 0xFF, 0x7A,
                    0x70, 0xC4, 0xBB, 0xBE, 0x8C, 0x19, 0x66, 0xF2, 0x65, 0xBE, 0x1F, 0xBE, 0xDB, 0x7E, 0x8E, 0x66,
                    0x40, 0xA4, 0xCF, 0xF9, 0x01, 0xA5, 0xDF, 0x40, 0x8E, 0xAA, 0xF7, 0xAE, 0x82, 0x75, 0x54, 0x8E,
                    0x2B, 0xEC, 0xA8, 0x44, 0x67, 0x7B, 0x48, 0x61, 0x3A, 0x06, 0x12, 0x31, 0x62, 0xEF, 0xBF, 0xC0,
                    0x0A, 0x79, 0x4F, 0x55, 0xFC, 0x76, 0x22, 0xFC, 0xF2, 0xD1, 0x17, 0xAB, 0xFE, 0xA1, 0xE1, 0xD3,
                    0xA2, 0x1B, 0xE2, 0xB7, 0x00, 0x21, 0x75, 0x39, 0xD1, 0x1D, 0x6E, 0x53, 0x7E, 0x28, 0xEE, 0xF2,
                    0x7E, 0xFC, 0xAE, 0xEA, 0xF1, 0x17, 0xFA, 0xFD, 0x51, 0xB1, 0x7C, 0x5B, 0xB8, 0x18, 0xC2, 0x77
                };

                static const U8 test_private_exp[256] = {
                    0x51, 0x80, 0x06, 0xDA, 0x93, 0xFA, 0xC2, 0x89, 0xF2, 0xC3, 0xA1, 0xBE, 0xCF, 0x1A, 0x19, 0x79,
                    0x70, 0xD0, 0xEC, 0x5F, 0xC2, 0xF0, 0x31, 0x6E, 0x69, 0xA6, 0x46, 0x87, 0xED, 0xF2, 0x3C, 0x93,
                    0xEC, 0xC7, 0xE3, 0x8A, 0xC5, 0x26, 0x30, 0x28, 0xC3, 0x95, 0x40, 0xC1, 0x9C, 0x27, 0x18, 0x46,
                    0xDD, 0xFC, 0x01, 0x7F, 0x3B, 0x7F, 0xD5, 0x3D, 0xDB, 0x27, 0xE9, 0x8C, 0x48, 0x8F, 0x04, 0x6B,
                    0x07, 0x07, 0xD1, 0xCA, 0xFF, 0x9E, 0x45, 0x48, 0xE5, 0xDB, 0x97, 0x8D, 0x6C, 0x96, 0x22, 0xCB,
                    0x8F, 0xF5, 0xE4, 0x89, 0x2B, 0x83, 0xEF, 0x32, 0xC9, 0x1A, 0x05, 0xB2, 0xEA, 0x56, 0x40, 0xCC,
                    0xB1, 0x7B, 0xAF, 0x93, 0x9F, 0x06, 0xEE, 0x14, 0x9F, 0xEA, 0x86, 0x23, 0x95, 0xBB, 0xBE, 0x9D,
                    0x21, 0x23, 0x01, 0x47, 0xA6, 0x5C, 0xAD, 0x87, 0xD3, 0x5B, 0xBC, 0xC3, 0x49, 0x89, 0x49, 0xCE,
                    0x40, 0x4F, 0x1C, 0x2F, 0x87, 0x83, 0x14, 0x38, 0x31, 0x7C, 0xA9, 0x67, 0xCB, 0x54, 0xB4, 0x52,
                    0xB9, 0xC8, 0xF1, 0xBA, 0xB2, 0x77, 0x71, 0xBC, 0x2C, 0xC1, 0x95, 0xD9, 0xB2, 0x9F, 0x8F, 0x24,
                    0xF7, 0x0B, 0x9B, 0xB6, 0x47, 0x9F, 0x2E, 0x17, 0x04, 0x0E, 0xEC, 0xC7, 0x62, 0xEA, 0x84, 0x9F,
                    0x8A, 0x6D, 0x74, 0x15, 0xF8, 0x40, 0x7D, 0x7D, 0xC7, 0xC9, 0xB0, 0x45, 0x65, 0xFA, 0x70, 0x94,
                    0x5A, 0x52, 0xAA, 0x3F, 0xD8, 0xE2, 0xEA, 0x08, 0x4A, 0x77, 0x21, 0x3F, 0x94, 0x73, 0xC3, 0x20,
                    0xF4, 0xC9, 0x1F, 0x4E, 0x42, 0xD5, 0x3D, 0xF3, 0x22, 0x13, 0x94, 0x91, 0x09, 0xDF, 0xE4, 0x36,
                    0xA7, 0x0E, 0xD3, 0xF4, 0x7A, 0x52, 0xBE, 0x86, 0xAB, 0xC7, 0x96, 0xD2, 0x7C, 0xA5, 0xC4, 0x3B,
                    0xF3, 0x15, 0x78, 0x02, 0x10, 0x53, 0x00, 0xE6, 0x83, 0xFF, 0xA9, 0x6B, 0xA7, 0x41, 0x31, 0x67
                };

                // Public exponent: 65537 (0x010001)
                static const U8 test_public_exp[3] = { 0x01, 0x00, 0x01 };

                GLogN("Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", key_slot);
                GLogN("  Algorithm: RSA-2048\r\n");
                GLogN("  Public Exponent: 65537 (0x010001)\r\n\r\n");

                GLogN("Modulus (first 32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_modulus[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                // Build RSA key structure
                HSM_RSAKey_t rsa_key;
                memset(&rsa_key, 0, sizeof(rsa_key));
                rsa_key.modulus_length = 256;
                memcpy(rsa_key.modulus, test_modulus, 256);
                rsa_key.public_exp_size = 3;
                memcpy(rsa_key.public_exp, test_public_exp, 3);
                memcpy(rsa_key.private_exp, test_private_exp, 256);

                GLogN("Importing RSA Private Key to slot %d...\r\n", key_slot);

                // Import with private key (public_only=false)
                HAL_StatusTypeDef status = hsm_import_rsa_key(key_slot, &rsa_key, false, false, false, HSM_RSA_2048);

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Private Key imported to slot %d\r\n", key_slot);
                    GLogN("     Ready for SHA256 compatibility test (hsmtest 0x101)\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Key Import failed, status: %d\r\n", status);
                }
                break;
            }

            case 0x104:  // RSA Private Key Import (using hsm_import_rsa_key wrapper)
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  RSA Private Key Import (using hsm_import_rsa_key)               \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0x104 [key_slot]
                U16 key_slot = 141;  // Default slot
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                // Test RSA 2048-bit Key - Modulus
                static const U8 test_modulus[256] = {
                    0x5F, 0x7C, 0x8E, 0x1B, 0x2C, 0x3B, 0x0B, 0x27, 0x6C, 0x9B, 0xC4, 0xB8, 0x0C, 0xF2, 0x16, 0x0C,
                    0x9C, 0x05, 0x5D, 0xA0, 0x7F, 0xDE, 0x6C, 0x28, 0xE9, 0x15, 0xAC, 0x39, 0x08, 0x7D, 0xF6, 0xD4,
                    0x01, 0x61, 0xB4, 0x34, 0xA6, 0x66, 0x3D, 0xC8, 0x10, 0xE7, 0x48, 0x68, 0x6A, 0xB4, 0xB8, 0x21,
                    0xAA, 0xC3, 0x41, 0x10, 0x48, 0xD8, 0xF0, 0x06, 0xF2, 0xE1, 0xD0, 0xD7, 0x7D, 0xC0, 0xE6, 0x74,
                    0x27, 0x8D, 0x9B, 0x60, 0xD1, 0x37, 0x84, 0xA2, 0xF5, 0xB3, 0xBF, 0x7B, 0xE8, 0x65, 0xD8, 0x71,
                    0x65, 0x57, 0x56, 0xD9, 0xCC, 0x11, 0xC9, 0xD4, 0x08, 0x74, 0x1A, 0x27, 0x9A, 0xBB, 0x6B, 0xC7,
                    0x9D, 0xA9, 0x46, 0xB1, 0x82, 0x89, 0xBA, 0xA1, 0x40, 0xA0, 0x4E, 0x5C, 0xBF, 0xAF, 0x2D, 0x2C,
                    0x3D, 0xE8, 0x8C, 0x96, 0xE6, 0xEC, 0x61, 0xFA, 0x4D, 0x1C, 0x1A, 0x50, 0xCA, 0xAA, 0x00, 0xB8,
                    0x40, 0x84, 0xC8, 0x8A, 0x6F, 0x18, 0x4D, 0x28, 0x8D, 0x28, 0xDA, 0x8C, 0x31, 0xB8, 0xE5, 0x4C,
                    0x84, 0x50, 0x10, 0x9C, 0xC8, 0x8F, 0x25, 0xB2, 0x4A, 0xB1, 0x62, 0x05, 0xF4, 0xCB, 0xFF, 0x7A,
                    0x70, 0xC4, 0xBB, 0xBE, 0x8C, 0x19, 0x66, 0xF2, 0x65, 0xBE, 0x1F, 0xBE, 0xDB, 0x7E, 0x8E, 0x66,
                    0x40, 0xA4, 0xCF, 0xF9, 0x01, 0xA5, 0xDF, 0x40, 0x8E, 0xAA, 0xF7, 0xAE, 0x82, 0x75, 0x54, 0x8E,
                    0x2B, 0xEC, 0xA8, 0x44, 0x67, 0x7B, 0x48, 0x61, 0x3A, 0x06, 0x12, 0x31, 0x62, 0xEF, 0xBF, 0xC0,
                    0x0A, 0x79, 0x4F, 0x55, 0xFC, 0x76, 0x22, 0xFC, 0xF2, 0xD1, 0x17, 0xAB, 0xFE, 0xA1, 0xE1, 0xD3,
                    0xA2, 0x1B, 0xE2, 0xB7, 0x00, 0x21, 0x75, 0x39, 0xD1, 0x1D, 0x6E, 0x53, 0x7E, 0x28, 0xEE, 0xF2,
                    0x7E, 0xFC, 0xAE, 0xEA, 0xF1, 0x17, 0xFA, 0xFD, 0x51, 0xB1, 0x7C, 0x5B, 0xB8, 0x18, 0xC2, 0x77
                };

                // Public Exponent: 65537 (0x010001)
                static const U8 test_public_exp[3] = { 0x01, 0x00, 0x01 };

                // Test RSA 2048-bit Key - Private Exponent
                static const U8 test_private_exp[256] = {
                    0x51, 0x80, 0x06, 0xDA, 0x93, 0xFA, 0xC2, 0x89, 0xF2, 0xC3, 0xA1, 0xBE, 0xCF, 0x1A, 0x19, 0x79,
                    0x70, 0xD0, 0xEC, 0x5F, 0xC2, 0xF0, 0x31, 0x6E, 0x69, 0xA6, 0x46, 0x87, 0xED, 0xF2, 0x3C, 0x93,
                    0xEC, 0xC7, 0xE3, 0x8A, 0xC5, 0x26, 0x30, 0x28, 0xC3, 0x95, 0x40, 0xC1, 0x9C, 0x27, 0x18, 0x46,
                    0xDD, 0xFC, 0x01, 0x7F, 0x3B, 0x7F, 0xD5, 0x3D, 0xDB, 0x27, 0xE9, 0x8C, 0x48, 0x8F, 0x04, 0x6B,
                    0x07, 0x07, 0xD1, 0xCA, 0xFF, 0x9E, 0x45, 0x48, 0xE5, 0xDB, 0x97, 0x8D, 0x6C, 0x96, 0x22, 0xCB,
                    0x8F, 0xF5, 0xE4, 0x89, 0x2B, 0x83, 0xEF, 0x32, 0xC9, 0x1A, 0x05, 0xB2, 0xEA, 0x56, 0x40, 0xCC,
                    0xB1, 0x7B, 0xAF, 0x93, 0x9F, 0x06, 0xEE, 0x14, 0x9F, 0xEA, 0x86, 0x23, 0x95, 0xBB, 0xBE, 0x9D,
                    0x21, 0x23, 0x01, 0x47, 0xA6, 0x5C, 0xAD, 0x87, 0xD3, 0x5B, 0xBC, 0xC3, 0x49, 0x89, 0x49, 0xCE,
                    0x40, 0x4F, 0x1C, 0x2F, 0x87, 0x83, 0x14, 0x38, 0x31, 0x7C, 0xA9, 0x67, 0xCB, 0x54, 0xB4, 0x52,
                    0xB9, 0xC8, 0xF1, 0xBA, 0xB2, 0x77, 0x71, 0xBC, 0x2C, 0xC1, 0x95, 0xD9, 0xB2, 0x9F, 0x8F, 0x24,
                    0xF7, 0x0B, 0x9B, 0xB6, 0x47, 0x9F, 0x2E, 0x17, 0x04, 0x0E, 0xEC, 0xC7, 0x62, 0xEA, 0x84, 0x9F,
                    0x8A, 0x6D, 0x74, 0x15, 0xF8, 0x40, 0x7D, 0x7D, 0xC7, 0xC9, 0xB0, 0x45, 0x65, 0xFA, 0x70, 0x94,
                    0x5A, 0x52, 0xAA, 0x3F, 0xD8, 0xE2, 0xEA, 0x08, 0x4A, 0x77, 0x21, 0x3F, 0x94, 0x73, 0xC3, 0x20,
                    0xF4, 0xC9, 0x1F, 0x4E, 0x42, 0xD5, 0x3D, 0xF3, 0x22, 0x13, 0x94, 0x91, 0x09, 0xDF, 0xE4, 0x36,
                    0xA7, 0x0E, 0xD3, 0xF4, 0x7A, 0x52, 0xBE, 0x86, 0xAB, 0xC7, 0x96, 0xD2, 0x7C, 0xA5, 0xC4, 0x3B,
                    0xF3, 0x15, 0x78, 0x02, 0x10, 0x53, 0x00, 0xE6, 0x83, 0xFF, 0xA9, 0x6B, 0xA7, 0x41, 0x31, 0x67
                };

                // Build RSA key structure (same pattern as 0x102, 0x103)
                HSM_RSAKey_t rsa_key;
                memset(&rsa_key, 0, sizeof(rsa_key));
                rsa_key.modulus_length = 256;
                memcpy(rsa_key.modulus, test_modulus, 256);
                rsa_key.public_exp_size = 3;
                memcpy(rsa_key.public_exp, test_public_exp, 3);
                memcpy(rsa_key.private_exp, test_private_exp, 256);

                GLogN("Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", key_slot);
                GLogN("  Algorithm: RSA-2048\r\n");
                GLogN("  Public Exponent: 0x010001 (65537)\r\n\r\n");

                GLogN("Modulus (first 32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_modulus[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nPrivate Exponent (first 32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_private_exp[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nCalling hsm_import_rsa_key()...\r\n");

                // Import with private key (public_only=false)
                HAL_StatusTypeDef status = hsm_import_rsa_key(
                    key_slot,
                    &rsa_key,
                    false,  // public_only = false (import full key pair)
                    false,  // encrypted
                    false,  // lock_key
                    HSM_RSA_2048  // key_size_bits
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Private Key imported to slot %d\r\n", key_slot);
                    GLogN("     Ready for signing operations\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Key Import failed, status: %d\r\n", status);
                }
                break;
            }

			case 0x117:  // special key import : KEK -> KM -> ECUCODE KEY
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  special Key Import (KEK -> KM -> ECUCODE KEY)               \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0x104 [key_slot]
                //U16 key_slot = 141;  // Default slot
                U16 key_len = 0;
                
                // Test RSA 2048-bit Key - Modulus
                static const U8 test_modulus[256] = {
                    0xba, 0xb0, 0x07, 0x17, 0x31, 0xe0, 0x5b, 0x2e, 0x09, 0x55, 0x36, 0x3c, 0x29, 0x25, 0x49, 0x2b,
                    0x50, 0xf3, 0x31, 0x15, 0x18, 0x20, 0x05, 0x3f, 0xb3, 0xe5, 0x14, 0x93, 0x57, 0x6a, 0x56, 0x0c,
                    0x65, 0xcb, 0x10, 0x3a, 0xb3, 0x72, 0xad, 0xc7, 0xc9, 0x3f, 0xe2, 0x41, 0xba, 0xc2, 0x50, 0x5e,
                    0xec, 0x7b, 0x10, 0xf0, 0xbc, 0xe7, 0x81, 0x94, 0x13, 0x52, 0x7d, 0x72, 0x04, 0x38, 0xb7, 0x0f,
                    0xf0, 0xb9, 0xd8, 0xe3, 0x8d, 0xf1, 0x97, 0xd5, 0xc8, 0x99, 0x87, 0xad, 0xdd, 0xaf, 0x25, 0x72,
                    0x49, 0x35, 0x9a, 0xb6, 0xe6, 0x23, 0x1e, 0x99, 0xf0, 0xf6, 0x04, 0xad, 0x8a, 0xa6, 0x63, 0x5e,
                    0xac, 0x0f, 0x3d, 0xd7, 0xee, 0xc2, 0xc2, 0xf8, 0x22, 0x67, 0x8c, 0xc8, 0xbd, 0xc7, 0x61, 0x10,
                    0x7c, 0x6e, 0x44, 0xf7, 0x04, 0xb9, 0xdf, 0xd6, 0xc8, 0xcb, 0x66, 0xfc, 0xca, 0xbf, 0x68, 0x8f,
                    0x3c, 0x3e, 0xba, 0x36, 0x11, 0xe5, 0x6a, 0x17, 0xe2, 0x95, 0xb8, 0xa9, 0xeb, 0xd5, 0x8f, 0xad,
                    0x5e, 0xd6, 0x08, 0x71, 0x3f, 0x7e, 0x49, 0x93, 0x5d, 0xb0, 0xb9, 0xaf, 0x4e, 0xf0, 0x51, 0x6a,
                    0x3e, 0xf8, 0xc7, 0x64, 0x98, 0x85, 0x68, 0x26, 0xfc, 0x5f, 0x14, 0x38, 0x51, 0xe8, 0x44, 0xb4,
                    0x65, 0x37, 0xca, 0xc2, 0x8e, 0x1f, 0x92, 0x9e, 0xaf, 0xba, 0xe6, 0xaa, 0x8f, 0x0f, 0xf1, 0xeb,
                    0x99, 0xb3, 0x56, 0xb7, 0x4d, 0x03, 0xba, 0xbb, 0xec, 0xa3, 0xd2, 0x5e, 0x5d, 0x82, 0xb3, 0xdd,
                    0x03, 0x7f, 0xa7, 0x9e, 0xf1, 0x3e, 0xf1, 0x25, 0x70, 0xcd, 0x00, 0x22, 0xcb, 0x00, 0x46, 0x0c,
                    0xb2, 0x96, 0xee, 0x21, 0x1a, 0xf5, 0xea, 0xf2, 0x03, 0xcb, 0x1b, 0x59, 0x0c, 0x87, 0x35, 0x2a,
                    0xb6, 0x14, 0x44, 0x74, 0xfc, 0xdd, 0x3c, 0x82, 0xd4, 0x6b, 0x72, 0x4d, 0x72, 0xf8, 0xec, 0xb1
                };

                // Public Exponent: 65537 (0x010001)
                static const U8 test_public_exp[3] = { 0x01, 0x00, 0x01 };

                // Test RSA 2048-bit Key - Private Exponent
                static const U8 test_private_exp[256] = {
                    0x2a, 0x3c, 0x09, 0x96, 0x9f, 0x43, 0x85, 0x0d, 0x40, 0xb2, 0x44, 0xef, 0x4e, 0x55, 0xaf, 0x9c,
					0x7b, 0x97, 0x51, 0x1a, 0xd7, 0x16, 0xe3, 0x69, 0x1a, 0x7f, 0x30, 0x6c, 0xf7, 0x01, 0x49, 0x0f,
					0x4b, 0xf6, 0x29, 0x29, 0x46, 0x90, 0xa2, 0xad, 0x08, 0xa4, 0x09, 0xc1, 0x5a, 0x09, 0x7c, 0xda,
					0x44, 0xc0, 0xc0, 0xbf, 0xdd, 0xd4, 0xb8, 0x15, 0x72, 0x5b, 0x9e, 0xa2, 0xad, 0x3e, 0xd1, 0x77,
					0x6b, 0x3b, 0xf8, 0x4b, 0xde, 0xc0, 0x71, 0xdb, 0xbd, 0x22, 0xb2, 0xb5, 0xcc, 0x69, 0xc3, 0xdc,
					0x7b, 0xa0, 0x83, 0x6c, 0x6e, 0x5b, 0x32, 0xa0, 0x9c, 0x09, 0x64, 0x5c, 0x88, 0x7a, 0x05, 0x10,
					0x15, 0x77, 0xeb, 0x43, 0x4f, 0x32, 0x43, 0x9e, 0x94, 0x55, 0xfd, 0xdd, 0x2a, 0x46, 0x11, 0x79,
					0x03, 0xd5, 0x84, 0x87, 0xa7, 0xcb, 0x8f, 0x4e, 0xcd, 0xa5, 0x6a, 0x9a, 0xb5, 0x19, 0xa1, 0x83,
					0x27, 0x0c, 0x3b, 0x56, 0x7c, 0x92, 0x11, 0x56, 0x25, 0xe7, 0x33, 0x86, 0xc5, 0x7b, 0x63, 0xf9,
					0x42, 0xdc, 0xe4, 0x74, 0x1c, 0x86, 0x1f, 0xec, 0xc7, 0xf7, 0xbf, 0x3e, 0x2d, 0xf7, 0x8a, 0xef,
					0xee, 0x76, 0x47, 0x0f, 0x8d, 0x81, 0xae, 0xdc, 0x93, 0xc6, 0x47, 0xeb, 0xa7, 0x72, 0xda, 0x45,
					0x09, 0xf4, 0x30, 0x6a, 0x95, 0xe0, 0x4c, 0xff, 0x56, 0x59, 0x1b, 0x01, 0x1a, 0xdb, 0x99, 0x76,
					0xe5, 0x83, 0x59, 0xb6, 0xc0, 0x10, 0x9d, 0xd9, 0xa7, 0xb9, 0x04, 0x51, 0x39, 0x41, 0x5b, 0x1c,
					0x79, 0xc4, 0x3f, 0x79, 0x38, 0x42, 0xf7, 0x49, 0xbf, 0x52, 0xeb, 0x00, 0x80, 0x6b, 0x7c, 0x55,
					0x31, 0xeb, 0x2e, 0xc6, 0xb0, 0x89, 0x6e, 0xe5, 0xf8, 0x37, 0xf0, 0xa1, 0x08, 0xa9, 0x2f, 0x2c,
					0x27, 0xd8, 0x04, 0xf3, 0x44, 0xb1, 0x43, 0x0d, 0x09, 0xb5, 0xdb, 0x83, 0xfb, 0xc9, 0x87, 0xd9                    
                };

                // Build RSA key structure (same pattern as 0x102, 0x103)
#if 0
                HSM_RSAKey_t rsa_key;
                memset(&rsa_key, 0, sizeof(rsa_key));
                rsa_key.modulus_length = 256;
                memcpy(rsa_key.modulus, test_modulus, 256);
                rsa_key.public_exp_size = 3;
                memcpy(rsa_key.public_exp, test_public_exp, 3);
                memcpy(rsa_key.private_exp, test_private_exp, 256);
#else
				uint8_t uckey_data[520] = {0,};
				memset(uckey_data, 0, sizeof(uckey_data));
				uckey_data[0] = (256 & 0x00FF);
				uckey_data[1] = (256 & 0xFF00) >> 8;
				memcpy(&uckey_data[2], test_modulus, 256);
				uckey_data[258] = (3 & 0x00FF);
				uckey_data[259] = (3 & 0xFF00) >> 8;
				memcpy(&uckey_data[260], test_public_exp, 3);
				memcpy(&uckey_data[263], test_private_exp, 256);
#endif

                GLogN("Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", 149);
                GLogN("  Algorithm: RSA-2048\r\n");
                GLogN("  Public Exponent: 0x010001 (65537)\r\n\r\n");

                GLogN("Modulus (first 32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_modulus[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nPrivate Exponent (first 32 bytes):\r\n");
				
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_private_exp[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nCalling hsm_import_special_key()...\r\n");

				key_len = 519;//modulus byte length 2, modulus 256, pubSize 2, pubExp 3, priExp 256
				
                // Import with private key (public_only=false)
                HAL_StatusTypeDef status = hsm_import_special_key(
                    HSM_SPECIAL_KEY_RSA_KEK,
                    uckey_data,
                    key_len,
                    false,  // encrypted
                    false   // lock_key
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Private Key imported to slot %d\r\n", 149);
                    GLogN("     Ready for signing operations\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Key Import failed, status: %d\r\n", status);
                }
				
				
				static const uint8_t KM_encrypted[256] = {
					0x23, 0x48, 0x55, 0x8d, 0x4e, 0x8b, 0xad, 0x14, 0xe2, 0xcf, 0x00, 0xab, 0x00, 0xea, 0x6b, 0x9b,
					0xc4, 0x66, 0x63, 0x47, 0xb9, 0x95, 0x8f, 0x23, 0x7e, 0x8a, 0x8d, 0xe5, 0xfe, 0x4b, 0x41, 0xb4,
					0xfc, 0x95, 0x8e, 0x09, 0x72, 0x58, 0x0c, 0x18, 0xfa, 0xde, 0x57, 0x3a, 0xe3, 0x6b, 0x0d, 0x1c,
					0xca, 0x8a, 0x55, 0x02, 0x3f, 0xde, 0xf8, 0x5b, 0x02, 0x6f, 0x9d, 0x88, 0xa4, 0xa1, 0x10, 0x34,
					0x72, 0x59, 0x69, 0x6f, 0x87, 0x9e, 0x13, 0xc2, 0x8d, 0xb4, 0x4c, 0x06, 0x20, 0x43, 0x89, 0x23,
					0x63, 0x3b, 0xad, 0xcf, 0xfc, 0x26, 0x1e, 0x27, 0x16, 0x1f, 0x45, 0xe0, 0x7d, 0x01, 0xf5, 0x42,
					0x0e, 0xbb, 0xd2, 0x76, 0xde, 0xb3, 0xca, 0x75, 0xb3, 0x50, 0xd2, 0x0c, 0xfb, 0xd8, 0x66, 0x8e,
					0xb2, 0x73, 0x8c, 0xb2, 0x31, 0x7e, 0xd7, 0xb3, 0x61, 0x2a, 0x13, 0x4a, 0xa1, 0x61, 0x2b, 0x03,
					0xe0, 0x03, 0xf1, 0x6b, 0x1f, 0x7a, 0x16, 0x38, 0x8c, 0x34, 0x59, 0x1e, 0x33, 0x5e, 0x28, 0x80,
					0xa8, 0xed, 0xf4, 0xa4, 0x43, 0x79, 0x1e, 0x4c, 0xe0, 0xc8, 0xe3, 0xd1, 0x99, 0x21, 0x41, 0x11,
					0x33, 0xc6, 0x3b, 0xb8, 0x24, 0xe0, 0xdd, 0x9c, 0x9f, 0x31, 0xbb, 0xac, 0xce, 0x5a, 0xdf, 0x91,
					0x25, 0xb5, 0x32, 0x9a, 0x5d, 0xf3, 0x1f, 0x5f, 0x41, 0x6d, 0x34, 0x5d, 0xb1, 0x76, 0x92, 0xf0,
					0x9f, 0x85, 0x7f, 0xca, 0x5d, 0xd7, 0xbb, 0xa1, 0x6f, 0xc6, 0x46, 0x98, 0xaf, 0x26, 0x25, 0x79,
					0xc9, 0x6c, 0xf0, 0x29, 0xda, 0x4b, 0xd3, 0xb1, 0xec, 0xb2, 0xd8, 0x8e, 0x97, 0xc1, 0xec, 0x1e,
					0x98, 0xaa, 0x5d, 0xb4, 0x4d, 0x41, 0x6c, 0x69, 0xed, 0x0d, 0x34, 0xcc, 0x7d, 0xbd, 0x84, 0xfe,
					0x01, 0xbc, 0x96, 0x8c, 0xfb, 0x13, 0x02, 0x35, 0xdf, 0x36, 0x67, 0x91, 0xdc, 0xf6, 0x1c, 0x43
					
				};
				GLogN("\r\nCalling hsm_import_special_key()...\r\n");
				key_len = 256;
				status = hsm_import_special_key(
					HSM_SPECIAL_KEY_KM,
					KM_encrypted,
					key_len,
					true,  // encrypted
					false	// lock_key
				);
				if (status == HAL_OK)
				{
					GLogN("[OK] KM Key imported to slot %d\r\n", 102);
				}
				else
				{
					GLogE("[FAIL] KM Key Import failed, status: %d\r\n", status);
				}
							
				static const uint8_t ecucodeKEK_encrypted[256] = {
					0x13, 0x98, 0xe2, 0x67, 0x9b, 0x6e, 0xf7, 0x5c, 0x9d, 0x06, 0xde, 0x48, 0xe2, 0x14, 0xb5, 0xdf,
					0xab, 0xb5, 0x6c, 0x5b, 0x37, 0x81, 0x23, 0x0f, 0x6e, 0x12, 0x76, 0x03, 0x7c, 0x7d, 0xd4, 0x40,
					0x0c, 0x49, 0xef, 0x9c, 0x6a, 0x31, 0x98, 0xad, 0xc8, 0xfc, 0x94, 0x5b, 0x01, 0x2e, 0x7d, 0x58,
					0xd1, 0xab, 0xc5, 0xab, 0xf1, 0xf0, 0x89, 0xc1, 0x9f, 0xa4, 0x03, 0x3c, 0x65, 0x74, 0x7e, 0xc2,
					0x13, 0x3b, 0xd7, 0x03, 0x85, 0x5e, 0x14, 0x31, 0xb7, 0x9c, 0x97, 0xaa, 0x17, 0x0e, 0x3a, 0x27,
					0x88, 0xbf, 0xa0, 0xdf, 0xdc, 0x25, 0xd0, 0x6c, 0xca, 0x5a, 0x33, 0x15, 0xb8, 0x75, 0xfd, 0x15,
					0x51, 0x53, 0x00, 0xb7, 0xe5, 0x9c, 0x3d, 0xfc, 0x25, 0xec, 0xc9, 0xbd, 0xcf, 0x70, 0x20, 0x94,
					0xcf, 0xd4, 0x1a, 0x1c, 0x7d, 0x61, 0x45, 0xf9, 0x7b, 0xab, 0x01, 0xb7, 0xbd, 0x18, 0x91, 0xf2,
					0x96, 0x7b, 0x1d, 0x61, 0x0a, 0x99, 0x3c, 0x48, 0x8a, 0x78, 0xc3, 0xfe, 0x7e, 0x39, 0x56, 0xa7,
					0xbd, 0x0a, 0xd7, 0x1e, 0xb6, 0xb9, 0x6f, 0x35, 0x87, 0xce, 0xd8, 0xe1, 0xc2, 0xba, 0x81, 0x0d,
					0x5d, 0x3e, 0xc4, 0xd0, 0x59, 0x92, 0xd8, 0xc0, 0x6a, 0xa6, 0xee, 0x5f, 0x02, 0x11, 0x78, 0x7e,
					0xe4, 0xfd, 0x8e, 0x71, 0xfb, 0xbc, 0x71, 0x95, 0xe2, 0x20, 0xe8, 0xa2, 0x0b, 0xf3, 0x69, 0x40,
					0x01, 0x5d, 0xaa, 0x79, 0x1c, 0xf8, 0xc9, 0x10, 0x06, 0x3b, 0xd4, 0x9e, 0x5e, 0xaf, 0x04, 0x80,
					0x29, 0x43, 0x41, 0x28, 0x90, 0xcb, 0x15, 0x1f, 0xc4, 0x8d, 0xce, 0xd4, 0x2a, 0x34, 0x10, 0x1e,
					0x79, 0xcb, 0x97, 0x8b, 0x2d, 0xef, 0xe2, 0xdd, 0x0a, 0x31, 0x94, 0xad, 0x49, 0xb1, 0x11, 0x9c,
					0x0c, 0x74, 0xb3, 0x82, 0xce, 0xf4, 0xda, 0xfa, 0x2d, 0xbb, 0xca, 0x86, 0x45, 0xce, 0x79, 0xdb
				};

				GLogN("\r\nCalling hsm_import_special_key()...\r\n");				
				key_len = 256;
				status = hsm_import_special_key(
					HSM_SPECIAL_KEY_ECU_CODE,
					ecucodeKEK_encrypted,
					key_len,
					true,  // encrypted
					false	// lock_key
				);
				if (status == HAL_OK)
				{
					GLogN("[OK] ECU_CODE Key imported to slot %d\r\n", 101);
				}
				else
				{
					GLogE("[FAIL] ECU_CODE Key Import failed, status: %d\r\n", status);
				}

				
                break;
            }
			case 0x119:  // special key import : KEK -> KM -> ECUCODE KEY
            {
				GLogN("\r\n========== AES Encrypt (ECB) Test ==========\r\n");
                GLogN("[NOTE] Valid AES Key IDs: UDK #101-108, TEMP #201-204\r\n");
                GLogN("[NOTE] Run 'hsmtest 0x52' first to generate AES key\r\n\r\n");

                uint16_t key_id = HSM_KEY_KM;   // Default: TEMP AES slot #201 (same as 0x52)
                uint8_t key_size = HSM_AES_128;

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    key_size = (uint8_t)strtol(gCliArgs[3], NULL, 0);
                }

                // Test plaintext (32 bytes = 2 blocks)
                /*static const*/ uint8_t plaintext[32] = {
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
                };
                uint8_t ciphertext[32];

                const char* size_str = (key_size == 1U) ? "AES-128" :
                                       (key_size == 2U) ? "AES-192" : "AES-256";

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Key Size: %s\r\n", size_str);
                GLogN("Mode: ECB\r\n");
                GLogN("Plaintext (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", plaintext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                HAL_StatusTypeDef status = hsm_aes_encrypt(
                    key_id,
                    key_size,
                    HSM_AES_ECB,
                    NULL,       // IV not used for ECB
                    plaintext,
                    32,
                    ciphertext
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] AES Encrypt Success\r\n");
                    GLogN("Ciphertext:\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", ciphertext[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] AES Encrypt failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");

				uint8_t expecttext[32]={0,};
				status = hsm_aes_decrypt(
                    key_id,
                    key_size,
                    HSM_AES_ECB,
                    NULL,
                    ciphertext,
                    32,
                    expecttext
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] AES Decrypt Success\r\n");
                    GLogN("Plaintext:\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", expecttext[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
					if (memcmp(plaintext, expecttext, 32) == 0)
	                {
	                	GLogN("[PASS] Output matches expected value!\r\n");
					}
                }
                else
                {
                    GLogE("[FAIL] AES Decrypt failed, status: %d\r\n", status);
                }
                GLogN("=============================================\r\n");
				
				break;
			}

            // ========== ED25519 / HMAC Test Suite (0x105 ~ 0x108) ==========

            case 0x105:  // ED25519 Key Import (using hsm_import_ed_key wrapper)
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  ED25519 Key Import (using hsm_import_ed_key)                   \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0x105 [key_slot] [mode]
                // mode: 0=key pair (64 bytes), 1=public only (32 bytes)
                U16 key_slot = 381;  // Default: ED TEMP slot
                U8 public_only = 0;  // 0=key pair, 1=public only

                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    public_only = (U8)strtol(gCliArgs[3], NULL, 0);
                }

                // ED25519 Test Key (same as 0xA1)
                // PrivateKey: f8b56168c770658e9d7328e40e80b50fab180a5fc3a1560073aeccc1ae22d9f0
                // PublicKey:  564007A4603B8AE0795D5061EAD4177A8B1CE17957438F4F406110B290E37C09
                HSM_EDKey_t ed_key = {
                    .private_key = {
                        0xf8, 0xb5, 0x61, 0x68, 0xc7, 0x70, 0x65, 0x8e,
                        0x9d, 0x73, 0x28, 0xe4, 0x0e, 0x80, 0xb5, 0x0f,
                        0xab, 0x18, 0x0a, 0x5f, 0xc3, 0xa1, 0x56, 0x00,
                        0x73, 0xae, 0xcc, 0xc1, 0xae, 0x22, 0xd9, 0xf0
                    },
                    .public_key = {
                        0x56, 0x40, 0x07, 0xA4, 0x60, 0x3B, 0x8A, 0xE0,
                        0x79, 0x5D, 0x50, 0x61, 0xEA, 0xD4, 0x17, 0x7A,
                        0x8B, 0x1C, 0xE1, 0x79, 0x57, 0x43, 0x8F, 0x4F,
                        0x40, 0x61, 0x10, 0xB2, 0x90, 0xE3, 0x7C, 0x09
                    }
                };

                GLogN("Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", key_slot);
                GLogN("  Import Mode: %s\r\n", public_only ? "Public Only (32 bytes)" : "Key Pair (64 bytes)");
                GLogN("  Algorithm: ED25519\r\n\r\n");

                GLogN("Private Key (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", ed_key.private_key[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nPublic Key (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", ed_key.public_key[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
				public_only=1;
                GLogN("\r\nCalling hsm_import_ed_key()...\r\n");

                HAL_StatusTypeDef status = hsm_import_ed_key(
                    key_slot,
                    &ed_key,
                    (bool)public_only,  // public_only
                    false,              // encrypted
                    false               // lock_key
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] ED25519 Key imported to slot %d\r\n", key_slot);
                }
                else
                {
                    GLogE("[FAIL] ED25519 Key Import failed, status: %d\r\n", status);
                }
                break;
            }
			case 0x114:  // ED25519 Key Encrypt Import (using hsm_import_ed_key wrapper)
			{
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  ED25519 Key Import (using hsm_import_ed_key)                   \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0x105 [key_slot] [mode]
                // mode: 0=key pair (64 bytes), 1=public only (32 bytes)
                //U16 key_slot = 261;  // Default: ED TEMP slot
                U16 key_slot = 381;  // Default: ED TEMP slot
                U8 public_only = 0;  // 0=key pair, 1=private only, 2=public only

                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    public_only = (U8)strtol(gCliArgs[3], NULL, 0);
                }

                // ED25519 Test Key (same as 0xA1)
                // PrivateKey: f8b56168c770658e9d7328e40e80b50fab180a5fc3a1560073aeccc1ae22d9f0
                // PublicKey:  564007A4603B8AE0795D5061EAD4177A8B1CE17957438F4F406110B290E37C09

                HSM_EDKey_t ed_key = {
                    .private_key = {
                        0xf8, 0xb5, 0x61, 0x68, 0xc7, 0x70, 0x65, 0x8e,
                        0x9d, 0x73, 0x28, 0xe4, 0x0e, 0x80, 0xb5, 0x0f,
                        0xab, 0x18, 0x0a, 0x5f, 0xc3, 0xa1, 0x56, 0x00,
                        0x73, 0xae, 0xcc, 0xc1, 0xae, 0x22, 0xd9, 0xf0
                    },
                    .public_key = {
#if 1
                        0x56, 0x40, 0x07, 0xA4, 0x60, 0x3B, 0x8A, 0xE0,
                        0x79, 0x5D, 0x50, 0x61, 0xEA, 0xD4, 0x17, 0x7A,
                        0x8B, 0x1C, 0xE1, 0x79, 0x57, 0x43, 0x8F, 0x4F,
                        0x40, 0x61, 0x10, 0xB2, 0x90, 0xE3, 0x7C, 0x09
#else
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#endif
                    }
                };
#if 0	//sampl1 key slot 381~530 (other)
				HSM_EDKey_enc_t ed_key_enc = {
                    .key = {
						0x1d,0x83,0x0d,0x63,0x5f,0xb2,0xc7,0x4d,0x12,0x65,0x50,0xfc,0x75,0xfd,0x1c,0x84,
						0xf0,0x74,0xf7,0x97,0x52,0x90,0x5f,0xe2,0x6d,0x6e,0xee,0xf1,0x77,0xc4,0xe0,0xe3,
						0xa6,0x3a,0x43,0x1b,0x20,0x74,0xe8,0x59,0xef,0xfd,0x85,0x89,0x62,0x34,0xe1,0xff,
						0xd7,0xb8,0x3c,0x7c,0x28,0x1b,0x92,0xfa,0x26,0xac,0xbc,0x02,0xc6,0x81,0xe7,0x2a,
						0x71,0x15,0x66,0xe8,0xe9,0x09,0x9b,0xb8,0xc1,0xd0,0xe5,0x84,0x40,0xba,0x0f,0xfd,
						0xce,0x63,0x35,0xa7,0xf1,0xce,0x2a,0x3b,0xa2,0x13,0x2f,0x20,0x34,0x26,0x2e,0x0e,
						0xf3,0xa1,0x74,0x6f,0x8a,0x2f,0x3e,0xf4,0xf5,0xe9,0xb1,0xe1,0xfd,0x4e,0xa8,0x92,
						0xa3,0xa8,0x91,0xd1,0x18,0xd4,0x3e,0x3a,0x72,0x94,0x0c,0x2a,0x8b,0x1b,0x9b,0x43,
						0x9e,0x92,0xff,0xe7,0x20,0x80,0x3d,0x7b,0x66,0xbd,0x49,0x28,0x39,0x43,0x1e,0xbf,
						0x37,0x75,0x38,0xbe,0xaf,0xa7,0x35,0x2b,0x6d,0xbf,0x94,0x8e,0x6a,0xf8,0x97,0x3f,
						0xa9,0xf2,0x4c,0xa6,0xcb,0x10,0x5f,0x14,0x13,0x27,0xc2,0xe1,0xb7,0x07,0x3b,0x98,
						0x0a,0x70,0x45,0xd2,0xdf,0x5f,0x3c,0x8f,0xaf,0x95,0x3b,0x3f,0xa7,0x5d,0x78,0x69,
						0x1a,0x50,0x4e,0xa2,0x35,0xaa,0xe5,0xe8,0x96,0x7d,0x3f,0x69,0x18,0x79,0x8b,0x58,
						0x2f,0x3a,0x7d,0x27,0x97,0x2f,0x63,0xe5,0x5f,0x30,0x04,0xdf,0x3f,0xef,0x76,0x1b,
						0x69,0xe9,0x91,0xe3,0x4a,0x8f,0xfe,0x79,0x8f,0x0b,0xf9,0x08,0xa1,0x69,0xea,0x1b,
						0xc5,0xe9,0x61,0x71,0x97,0xbb,0x5a,0x55,0xc5,0x56,0x04,0x0e,0x47,0xaa,0xf1,0xc1
                    }
                };
#else	//sample2 key slot 361~380 (only cert)
				key_slot = 361;
				HSM_EDKey_enc_t ed_key_enc = {
                    .key = {
						0x38, 0x29, 0x53, 0x41, 0x9a, 0xa1, 0x03, 0xe4, 0xd2, 0x8d, 0x02, 0x6a, 0x2a, 0x78, 0x32, 0x15,
						0x4b, 0xa3, 0x6c, 0x60, 0x5d, 0xc6, 0xb0, 0x04, 0xb2, 0x3b, 0xa1, 0xef, 0xf8, 0xf0, 0x0d, 0xf2,
						0x1a, 0x86, 0x85, 0x88, 0x55, 0xb4, 0x9a, 0xda, 0x7d, 0x5f, 0x44, 0x1c, 0x93, 0x0c, 0xc8, 0xc6,
						0x27, 0xd8, 0x76, 0xad, 0x90, 0xd0, 0xdf, 0xc5, 0x9f, 0x8b, 0xc6, 0x5f, 0x9b, 0x6c, 0x00, 0x02,
						0x7e, 0x5c, 0xe1, 0xc8, 0xe3, 0xc9, 0x54, 0x2f, 0x8a, 0x1b, 0x14, 0xf5, 0xfd, 0x2a, 0x05, 0x8b,
						0x45, 0xe3, 0x25, 0xe7, 0x64, 0x02, 0x6a, 0x91, 0x41, 0xfa, 0x11, 0x7c, 0x9a, 0xae, 0x4c, 0x31,
						0xab, 0x42, 0xb2, 0xc0, 0xd3, 0xad, 0x87, 0xe0, 0x3d, 0xa5, 0xa4, 0xaf, 0x2f, 0x20, 0x98, 0xa2,
						0x6a, 0xd2, 0xfb, 0x53, 0x57, 0x65, 0x7b, 0x42, 0x4e, 0x3a, 0x12, 0x20, 0x9d, 0x7c, 0x66, 0x8a,
						0x6b, 0xaf, 0x78, 0x50, 0xac, 0xb3, 0x49, 0x78, 0x90, 0x08, 0xd5, 0xc9, 0xc4, 0x2c, 0x8d, 0x17,
						0x03, 0x52, 0x9c, 0xb8, 0xdf, 0x30, 0x03, 0x27, 0x64, 0x33, 0x8b, 0xd4, 0x59, 0x82, 0xde, 0xbc,
						0x85, 0x35, 0x54, 0x5a, 0x9a, 0x94, 0x14, 0x57, 0x4f, 0x20, 0xa7, 0x01, 0x49, 0xaf, 0x75, 0xc9,
						0xac, 0x36, 0x64, 0x9f, 0x85, 0xb4, 0xb2, 0xf3, 0x37, 0x78, 0x31, 0x3f, 0x6c, 0x4d, 0x1f, 0xc8,
						0x6e, 0x92, 0xa1, 0x64, 0x40, 0xd7, 0x19, 0xea, 0x05, 0xed, 0x54, 0x58, 0x82, 0x63, 0x62, 0x5a,
						0x9c, 0x19, 0xf7, 0x09, 0xbc, 0x42, 0xd9, 0x8d, 0x12, 0x54, 0x21, 0xf7, 0x5c, 0x29, 0xf8, 0xe6,
						0x43, 0x94, 0x96, 0x41, 0x36, 0x7a, 0xf2, 0xcd, 0x33, 0x62, 0x82, 0xf5, 0xc8, 0x81, 0xf2, 0xd6,
						0x45, 0x90, 0x6d, 0x65, 0xfd, 0xab, 0xd3, 0xf7, 0x90, 0x8a, 0x96, 0x6f, 0x0f, 0x2e, 0xe7, 0x90
                    }
                };
#endif

                GLogN("Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", key_slot);
                GLogN("  Import Mode: %s\r\n", public_only ? "Public Only (32 bytes)" : "Key Pair (64 bytes)");
                GLogN("  Algorithm: ED25519\r\n\r\n");

                GLogN("Private Key (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", ed_key.private_key[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nPublic Key (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", ed_key.public_key[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nCalling hsm_import_ed_key()...\r\n");
#if 0
                HAL_StatusTypeDef status = hsm_import_ed_key(
                    key_slot,
                    &ed_key,
                    (bool)public_only,  // public_only
                    false,              // encrypted
                    false               // lock_key
                );
#else
				public_only=1;//private only
				HAL_StatusTypeDef status = hsm_import_ed_key_en(
                    key_slot,
                    &ed_key_enc,
                    (bool)public_only,  // public_only
                    true,	            // encrypted
                    false               // lock_key
                );
#endif

                if (status == HAL_OK)
                {
                    GLogN("[OK] ED25519 Key imported to slot %d\r\n", key_slot);
                }
                else
                {
                    GLogE("[FAIL] ED25519 Key Import failed, status: %d\r\n", status);
                }
                break;
            }

            case 0x106:  // ED25519 Sign Test (Seed -> Key)
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  ED25519 Sign Test (Seed -> Key)                                \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0x106 [key_slot]
                //U16 key_slot = 261;
                U16 key_slot = 381;
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }
#if 0	//key_slot = 381
                // Test Seed (64 bytes)
                // 009E4B160000000000000000000000000000001E6DAA5D7864671BC007A1755B66A4655BA6E802986DE54A33B575AF44AAC4877D1F7BA668EABC3EB099BFCDF0
                static const U8 test_seed[64] = {
                    0x00, 0x9E, 0x4B, 0x16, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x1E, 0x6D, 0xAA, 0x5D, 0x78,
                    0x64, 0x67, 0x1B, 0xC0, 0x07, 0xA1, 0x75, 0x5B,
                    0x66, 0xA4, 0x65, 0x5B, 0xA6, 0xE8, 0x02, 0x98,
                    0x6D, 0xE5, 0x4A, 0x33, 0xB5, 0x75, 0xAF, 0x44,
                    0xAA, 0xC4, 0x87, 0x7D, 0x1F, 0x7B, 0xA6, 0x68,
                    0xEA, 0xBC, 0x3E, 0xB0, 0x99, 0xBF, 0xCD, 0xF0
                };

                // Expected output (64 bytes)
                // 31BC48DE8F7544885C64C41318FF1235655AC47C9CFC93B61D0FE0D877D5AFFD564007A4603B8AE0795D5061EAD4177A8B1CE17957438F4F406110B290E37C09
                static const U8 expected_key[64] = {
                    0x31, 0xBC, 0x48, 0xDE, 0x8F, 0x75, 0x44, 0x88,
                    0x5C, 0x64, 0xC4, 0x13, 0x18, 0xFF, 0x12, 0x35,
                    0x65, 0x5A, 0xC4, 0x7C, 0x9C, 0xFC, 0x93, 0xB6,
                    0x1D, 0x0F, 0xE0, 0xD8, 0x77, 0xD5, 0xAF, 0xFD,
                    0x56, 0x40, 0x07, 0xA4, 0x60, 0x3B, 0x8A, 0xE0,
                    0x79, 0x5D, 0x50, 0x61, 0xEA, 0xD4, 0x17, 0x7A,
                    0x8B, 0x1C, 0xE1, 0x79, 0x57, 0x43, 0x8F, 0x4F,
                    0x40, 0x61, 0x10, 0xB2, 0x90, 0xE3, 0x7C, 0x09
                };

                GLogN("Test Parameters:\r\n");
                GLogN("  Key Slot: %d\r\n", key_slot);
                GLogN("  Seed (64 bytes):\r\n");
                for (int i = 0; i < 64; i++)
                {
                    GLogN("%02X ", test_seed[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                GLogN("Expected Output (64 bytes):\r\n");
                for (int i = 0; i < 64; i++)
                {
                    GLogN("%02X ", expected_key[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                // ED25519 Sign: OP 0x15
                U8 signature[64] = {0};
                HAL_StatusTypeDef status = hsm_sign_eddsa(key_slot, test_seed, 64, signature);

                if (status == HAL_OK)
                {
                    GLogN("[OK] ED25519 Sign Success\r\n\r\n");
                    GLogN("Actual Output (64 bytes):\r\n");
                    for (int i = 0; i < 64; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("\r\n");

                    // Compare with expected
                    if (memcmp(signature, expected_key, 64) == 0)
                    {
                        GLogN("[PASS] Output matches expected value!\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] Output does NOT match expected value\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] ED25519 Sign failed, status: %d\r\n", status);
                }
#else	//key_slot = 361
				key_slot = 361;

				// Test Seed (32 bytes)
                // B1411384A7111C679D5A4416CB7632C8EBCDF40E45CECAB95840119B9F29D914
                static const U8 test_seed[32] = {
                    0xB1, 0x41, 0x13, 0x84, 0xA7, 0x11, 0x1C, 0x67, 0x9D, 0x5A, 0x44, 0x16, 0xCB, 0x76, 0x32, 0xC8,
					0xEB, 0xCD, 0xF4, 0x0E, 0x45, 0xCE, 0xCA, 0xB9, 0x58, 0x40, 0x11, 0x9B, 0x9F, 0x29, 0xD9, 0x14
                    
                };

                // Expected output (64 bytes)
                // 616f730569a12fb870974594b3bd5c60d7c65d7efe777485ac168a1580b97c7740f0e3ac8b47bd4b827fb41af93576367c834056478ea354eeb579539ac2240e
                static const U8 expected_key[64] = {
                    0x61, 0x6f, 0x73, 0x05, 0x69, 0xa1, 0x2f, 0xb8, 0x70, 0x97, 0x45, 0x94, 0xb3, 0xbd, 0x5c, 0x60,
					0xd7, 0xc6, 0x5d, 0x7e, 0xfe, 0x77, 0x74, 0x85, 0xac, 0x16, 0x8a, 0x15, 0x80, 0xb9, 0x7c, 0x77,
					0x40, 0xf0, 0xe3, 0xac, 0x8b, 0x47, 0xbd, 0x4b, 0x82, 0x7f, 0xb4, 0x1a, 0xf9, 0x35, 0x76, 0x36,
					0x7c, 0x83, 0x40, 0x56, 0x47, 0x8e, 0xa3, 0x54, 0xee, 0xb5, 0x79, 0x53, 0x9a, 0xc2, 0x24, 0x0e
                    
                };

                GLogN("Test Parameters:\r\n");
                GLogN("  Key Slot: %d\r\n", key_slot);
                GLogN("  Seed (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_seed[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                GLogN("Expected Output (64 bytes):\r\n");
                for (int i = 0; i < 64; i++)
                {
                    GLogN("%02X ", expected_key[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                // ED25519 Sign: OP 0x15
                U8 signature[64] = {0};
                HAL_StatusTypeDef status = hsm_sign_eddsa(key_slot, test_seed, 32, signature);

                if (status == HAL_OK)
                {
                    GLogN("[OK] ED25519 Sign Success\r\n\r\n");
                    GLogN("Actual Output (64 bytes):\r\n");
                    for (int i = 0; i < 64; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("\r\n");

                    // Compare with expected
                    if (memcmp(signature, expected_key, 64) == 0)
                    {
                        GLogN("[PASS] Output matches expected value!\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] Output does NOT match expected value\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] ED25519 Sign failed, status: %d\r\n", status);
                }
#endif

                break;
            }
			case 0x115:  // KD SHA256 Key Import
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  KD SHA256 Key Import (using hsm_import_KD_SHA256_key)           \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0x15 [key_slot] [mode]
                // mode: 0=key pair (64 bytes), 1=public only (32 bytes)
                U16 key_slot = 601;  // slot #601 ~ #650
                U8 public_only = 0;  // 0=key pair, 1=private only, 2=public only

                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    public_only = (U8)strtol(gCliArgs[3], NULL, 0);
                }

                // ED25519 Test Key (same as 0xA1)
                // (Secret)PrivateKey: 11223344556677889900AABBCCDDEEFF11223344556677889900AABBCCDDEE00
#if 0 //plain key
				uint8_t Secret[32] = {
					0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
					0x99, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
					0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
					0x99, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00
				};
#else
				uint8_t Secret[256] = {
				    0xAD, 0xC5, 0x64, 0x0E, 0xB2, 0xF6, 0x97, 0x11, 0x41, 0xF2, 0x82, 0x8A, 0x8C, 0xBA, 0x35, 0x0C,
					0x3C, 0x4B, 0x9F, 0x02, 0xD5, 0x8E, 0xAD, 0x49, 0xB6, 0xDF, 0x39, 0xBE, 0xBC, 0xEA, 0xC5, 0xFE,
					0x6C, 0x30, 0x7E, 0x12, 0x36, 0x8B, 0xCD, 0x40, 0x53, 0xAB, 0x3D, 0x32, 0x98, 0xC1, 0xAE, 0xB3,
					0x59, 0x23, 0xDB, 0xE4, 0x57, 0x7E, 0x7A, 0x53, 0x30, 0x69, 0x25, 0x1D, 0x74, 0xBF, 0xC0, 0x1E,
					0x61, 0x08, 0x88, 0x89, 0x9B, 0x3B, 0x24, 0x74, 0x98, 0xFE, 0x26, 0x61, 0x0B, 0x97, 0xDD, 0x08,
					0xEC, 0x91, 0x4A, 0xE8, 0xF3, 0x5E, 0xD8, 0xF8, 0x65, 0x81, 0x42, 0xE0, 0x9A, 0xA1, 0xB5, 0xE7,
					0x4E, 0xDA, 0x47, 0x42, 0x0E, 0x4A, 0xEF, 0x12, 0xB7, 0x89, 0x81, 0xB9, 0x91, 0xB3, 0x89, 0xFE,
					0x87, 0xD7, 0x4F, 0xE7, 0x8F, 0xDF, 0x51, 0xE1, 0x68, 0x68, 0xA1, 0xF4, 0xF7, 0x7B, 0xBB, 0xBB,
					0xB2, 0x7F, 0x11, 0xC3, 0x72, 0x8E, 0x65, 0x6B, 0x4C, 0xF4, 0x62, 0xA9, 0x13, 0xFF, 0x8C, 0x6C,
					0x22, 0x17, 0x52, 0xB8, 0xA8, 0x57, 0x9E, 0x52, 0x1C, 0xB0, 0x8B, 0x29, 0x24, 0x63, 0x53, 0xF4,
					0xE0, 0xF9, 0xD0, 0x48, 0xE0, 0xAD, 0xF4, 0xD0, 0x16, 0x25, 0x89, 0x24, 0xF0, 0xB7, 0xB6, 0x6F,
					0xE6, 0xB1, 0xF1, 0x3A, 0x97, 0xD0, 0xA5, 0xC2, 0xD3, 0xB0, 0xAE, 0x49, 0x67, 0x07, 0xC5, 0x69,
					0x4B, 0xA1, 0xB4, 0x41, 0x5D, 0xC0, 0x8F, 0xC1, 0x73, 0x5C, 0x89, 0x2C, 0x45, 0x81, 0x85, 0xBA,
					0xD5, 0x3F, 0x25, 0xCC, 0x06, 0x25, 0x1C, 0xB3, 0xF7, 0xFD, 0x87, 0x7E, 0x8F, 0x2E, 0x61, 0xA1,
					0x94, 0xD5, 0x21, 0xF3, 0x2D, 0xD9, 0x70, 0xF6, 0xC8, 0x80, 0x39, 0x9F, 0xD6, 0xA1, 0x72, 0xF9,
					0x51, 0xBE, 0x80, 0xEA, 0xE8, 0xB3, 0xD9, 0xB9, 0x27, 0xFA, 0xC2, 0xE6, 0x91, 0xAD, 0x26, 0xB1
				    
				};
#endif

                GLogN("Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", key_slot);
                GLogN("  Import Mode: %s\r\n", public_only ? "Public Only (32 bytes)" : "Key Pair (64 bytes)");
                GLogN("  Algorithm: ED25519\r\n\r\n");

                GLogN("Secret Key (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", Secret[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nCalling hsm_import_ed_key()...\r\n");
				
                HAL_StatusTypeDef status = hsm_import_KD_SHA256_key(
                    key_slot,
                    Secret,
                    true,              // encrypted
                    false               // lock_key
                );

                if (status == HAL_OK)
                {
                    GLogN("[OK] KD SHA256 Secret imported to slot %d\r\n", key_slot);
                }
                else
                {
                    GLogE("[FAIL] KD SHA256 Secret Import failed, status: %d\r\n", status);
                }
                break;
            }
			case 0x116:  // KD SHA256 sign
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  KD SHA256 sign (using hsm_calculate_sha)                   \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0x15 [key_slot] [mode]
                // mode: 0=key pair (64 bytes), 1=public only (32 bytes)
                U16 key_slot = 601;  // slot #601 ~ #650

                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                // ED25519 Test Key (same as 0xA1)
                // (Secret)PrivateKey: 11223344556677889900AABBCCDDEEFF11223344556677889900AABBCCDDEE00

				uint8_t Seed[32] = 	{
					0xDD, 0xF5, 0x18, 0x5C, 0x05, 0x9C, 0x2E, 0xDA,
					0x19, 0x89, 0x2D, 0x53, 0xF4, 0xD6, 0xF6, 0x38,
					0xAA, 0xF8, 0x64, 0x8D, 0xB8, 0xF4, 0x90, 0x89,
					0x03, 0x67, 0xBE, 0x16, 0x49, 0x7A, 0xC8, 0x4E
				};
				// Expected Output
                // 919937F99B26BEAD725377C0BB2CAECAA4967571254139857988DEED4C2762E0 (32 bytes)
                static const U8 expected_key[32] = {
                    0x91, 0x99, 0x37, 0xF9, 0x9B, 0x26, 0xBE, 0xAD, 
					0x72, 0x53, 0x77, 0xC0, 0xBB, 0x2C, 0xAE, 0xCA,
					0xA4, 0x96, 0x75, 0x71, 0x25, 0x41, 0x39, 0x85, 
					0x79, 0x88, 0xDE, 0xED, 0x4C, 0x27, 0x62, 0xE0
                };
                

                GLogN("Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", key_slot);

                GLogN("  Algorithm: KD SHA256\r\n\r\n");


                GLogN("\r\nSeed (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", Seed[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nCalling hsm_calculate_sha()...\r\n");
				
                U8 sha1_hash[20] = {0};
                HAL_StatusTypeDef status = hsm_calculate_sha(HSM_SHA_256, true, key_slot, Seed, 32, sha1_hash);
				if (status == HAL_OK)
                {
                    GLogN("[OK] kd SHA256 sign Success\r\n\r\n");
                    GLogN("Actual Output (32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", sha1_hash[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("\r\n");

                    if (memcmp(sha1_hash, expected_key, 32) == 0)
                    {
                        GLogN("[PASS] Output matches expected value!\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] Output does NOT match expected value\r\n");
                    }
                }

                if (status == HAL_OK)
                {
                    GLogN("[OK] kd sign using key_slot %d\r\n", key_slot);
                }
                else
                {
                    GLogE("[FAIL] kd sign failed, status: %d\r\n", status);
                }
                break;
            }

            case 0x107:  // HMAC Key Import (using hsm_import_hmac_key wrapper)
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  HMAC Key Import (using hsm_import_hmac_key)                     \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0x107 [key_slot]
                //U16 key_slot = 121;  // Default: HMAC UDK slot #121
                U16 key_slot = 321;  // Default: HMAC UDK slot #321 ~ #350
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                // HMAC Secret Key (32 bytes = 256 bits per manual)
                // Secret : 32 bytes of zeros for test
                static const U8 hmac_secret[32] = {
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                };
				
				
				static const U8 hmac_secret_encrypt[256] = {
					0x4e, 0xf0, 0xb3, 0xaf, 0x9a, 0xed, 0xb2, 0x32,
					0xe4, 0xd8, 0x7b, 0xee, 0x17, 0x28, 0x0f, 0xd0,
					0x1e, 0x08, 0x03, 0x40, 0x1b, 0xa3, 0xc4, 0x82,
					0x60, 0x04, 0x19, 0x4e, 0x03, 0x35, 0x77, 0xbc,
					0xf9, 0x37, 0x13, 0x14, 0xdb, 0x3f, 0x04, 0xe5,
					0xf7, 0x9b, 0x42, 0x5a, 0x5f, 0x86, 0x0b, 0xed,
					0x29, 0x39, 0x6e, 0xd8, 0xdd, 0x7b, 0xbf, 0xdb,
					0x7b, 0xd0, 0xcf, 0x22, 0x14, 0x01, 0x69, 0x8b,
					0x59, 0xec, 0x63, 0x3e, 0xea, 0xb2, 0xf6, 0x3d,
					0x69, 0x9b, 0xd7, 0xd3, 0x11, 0x40, 0xb5, 0x88,
					0x2a, 0x0d, 0x9d, 0xfd, 0xe0, 0x68, 0x70, 0xac,
					0x70, 0x6b, 0xcb, 0x5a, 0xf9, 0x45, 0x67, 0x4a,
					0x81, 0x9f, 0xf5, 0x51, 0x4b, 0xdf, 0xcb, 0x50,
					0x2c, 0x0b, 0x32, 0x96, 0x1a, 0xf4, 0xe2, 0x90,
					0x52, 0xb3, 0x93, 0x93, 0xdb, 0x7b, 0x0e, 0x97,
					0xc8, 0xe0, 0x2d, 0xf2, 0x69, 0x68, 0x60, 0x02,
					0xb1, 0xd6, 0x29, 0xbf, 0x36, 0xb8, 0xdd, 0xe3,
					0xda, 0x00, 0xb9, 0x9c, 0x76, 0x28, 0x7b, 0x42,
					0xef, 0x76, 0x89, 0xea, 0x4e, 0x38, 0x7e, 0x3d,
					0x28, 0x53, 0x7b, 0x9b, 0x6b, 0x71, 0xa7, 0xee,
					0x02, 0xd6, 0x86, 0x08, 0xd2, 0xc2, 0xa3, 0x32,
					0x72, 0x94, 0xfa, 0x70, 0x4a, 0xa5, 0x0f, 0x6f,
					0x7d, 0xae, 0xa3, 0x45, 0x38, 0x52, 0x90, 0xb8,
					0xff, 0x96, 0x29, 0x88, 0xe2, 0x61, 0xd6, 0x56,
					0xdf, 0x3c, 0xa0, 0x9f, 0x47, 0xc8, 0xa4, 0x97,
					0x63, 0xea, 0x14, 0x8e, 0xa0, 0xbe, 0x3d, 0x50,
					0x24, 0xda, 0x12, 0x0b, 0xaf, 0x95, 0x27, 0x1e,
					0x4e, 0x38, 0xbb, 0x9a, 0x6c, 0xb0, 0xe5, 0xab,
					0x7a, 0x9d, 0xf9, 0x90, 0xab, 0x3d, 0xa1, 0x3a,
					0xb0, 0x52, 0x33, 0xe9, 0x57, 0x26, 0x05, 0xf3,
					0xc1, 0x29, 0x06, 0xb4, 0x4f, 0x8e, 0xbe, 0xa7,
					0x0a, 0x45, 0xf6, 0x46, 0x7c, 0x8e, 0xb6, 0x46
				};

                GLogN("Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", key_slot);
                GLogN("  Algorithm: HMAC (HSM_ALG_HMAC = 0x04)\r\n");
                GLogN("  Key Size: 32 bytes (256 bits)\r\n\r\n");

                GLogN("HMAC Secret (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", hmac_secret[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }

                GLogN("\r\nCalling hsm_import_hmac_key()...\r\n");
#if 0//plain key
                HAL_StatusTypeDef status = hsm_import_hmac_key(
                    key_slot,
                    hmac_secret,
                    false,  // encrypted
                    false   // lock_key
                );
#else//encrypt key
				HAL_StatusTypeDef status = hsm_import_hmac_key(
                    key_slot,
                    hmac_secret_encrypt,
                    true,  // encrypted
                    false   // lock_key
                );
#endif

                if (status == HAL_OK)
                {
                    GLogN("[OK] HMAC Key imported to slot %d\r\n", key_slot);
                }
                else
                {
                    GLogE("[FAIL] HMAC Key Import failed, status: %d\r\n", status);
                }
                break;
            }

            case 0x108:  // HMAC_SHA256 Test
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  HMAC_SHA256 Test (Seed + Secret -> Key)                        \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0x108 [key_slot]
                U16 key_slot = 321;
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                // Test data
                // Seed: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF (16 bytes)
                static const U8 test_seed[16] = {
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
                };

                // Expected Output
                // Key: 0x293531BBB4268FD5145915D35FCEEE6E4182241CC5EE9A533944F8BC8C30AAF2 (32 bytes)
                static const U8 expected_key[32] = {
                    0x29, 0x35, 0x31, 0xBB, 0xB4, 0x26, 0x8F, 0xD5,
                    0x14, 0x59, 0x15, 0xD3, 0x5F, 0xCE, 0xEE, 0x6E,
                    0x41, 0x82, 0x24, 0x1C, 0xC5, 0xEE, 0x9A, 0x53,
                    0x39, 0x44, 0xF8, 0xBC, 0x8C, 0x30, 0xAA, 0xF2
                };

                GLogN("Test Parameters:\r\n");
                GLogN("  Key Slot: %d (HMAC Secret should be pre-loaded)\r\n", key_slot);
                GLogN("  Seed (16 bytes): ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", test_seed[i]);
                GLogN("\r\n\r\n");

                GLogN("Expected Output (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", expected_key[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                // HMAC-SHA256 operation
                U8 output[32] = {0};
                // Note: hsm_generate_hmac_sha256 requires minimum 64 bytes input
                // Pad test_seed to 64 bytes
                U8 padded_seed[64] = {0};
                //memcpy(padded_seed, test_seed, 16);

                //HAL_StatusTypeDef status = hsm_generate_hmac_sha256(key_slot, padded_seed, 64, output);
                HAL_StatusTypeDef status = hsm_generate_hmac_sha256(key_slot, test_seed, 16, output);

                if (status == HAL_OK)
                {
                    GLogN("[OK] HMAC_SHA256 Success\r\n\r\n");
                    GLogN("Actual Output (32 bytes):\r\n");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", output[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("\r\n");

                    if (memcmp(output, expected_key, 32) == 0)
                    {
                        GLogN("[PASS] Output matches expected value!\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] Output does NOT match expected value\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] HMAC_SHA256 failed, status: %d\r\n", status);
                }
                break;
            }

            case 0x110:  // WBC Compatibility Test - SHA-256 mode
            {
                GLogN("\r\n");
                GLogN("╔══════════════════════════════════════════════════════════════╗\r\n");
                GLogN("║  WBC Compatibility Test - SignPrivateHSM (SHA-256)           ║\r\n");
                GLogN("║  (Using SIGN command with SHA-256 DigestInfo)                ║\r\n");
                GLogN("╚══════════════════════════════════════════════════════════════╝\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse key_slot argument: hsmtest 0x110 [key_slot]
                U16 key_slot = HSM_KEY_CSAC_PA_SHA1;  // Default: 141 (same key can be used)
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                /* Same test seed as SHA-1 test */
                static const U8 test_seed[8] = {0x7E, 0x38, 0xF6, 0x3C, 0x25, 0x64, 0x47, 0xB6};

                U8 signature[256] = {0};
                U32 sign_len = 0;

                GLogN("SHA-256 WBC Algorithm:\r\n");
                GLogN("  1. SIGN command internally creates:\r\n");
                GLogN("     [0x00][0x01][FF*202][0x00][SHA256_DigestInfo:19][hash:32]\r\n");
                GLogN("  2. We pass: fake_hash[32] = [seed:8][0x00:24]\r\n");
                GLogN("  3. Result: seed placed at hash position, RSA sign executed\r\n\r\n");

                GLogN("Test Parameters:\r\n");
                GLogN("  Key Slot: %d\r\n", key_slot);
                GLogN("  SHA Type: SHA-256 (32-byte hash)\r\n");
                GLogN("  Seed (8 bytes): ");
                for (int i = 0; i < 8; i++) GLogN("%02X ", test_seed[i]);
                GLogN("\r\n\r\n");

                // Call WBC compatible function with SHA-256 mode
                HAL_StatusTypeDef status = hsm_sign_rsa_with_seed_new(test_seed, 8, signature, &sign_len, key_slot, HSM_SHA_256);

                if (status == HAL_OK)
                {
                    GLogN("[OK] hsm_sign_rsa_with_seed_new (SHA-256): Success (len=%lu)\r\n\r\n", sign_len);

                    // Display full signature
                    GLogN("SHA-256 WBC Signature (256 bytes):\r\n");
                    for (int i = 0; i < 256; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n");
                    }
                    GLogN("\r\n");

                    GLogN("╔══════════════════════════════════════════════════════════════╗\r\n");
                    GLogN("║  SHA-256 WBC signature generated successfully!               ║\r\n");
                    GLogN("║  Compare with WBC tool output for verification               ║\r\n");
                    GLogN("╚══════════════════════════════════════════════════════════════╝\r\n");
                }
                else
                {
                    GLogE("[FAIL] hsm_sign_rsa_with_seed_new (SHA-256) failed (status=%d)\r\n", status);
                    GLogE("  Check if RSA private key exists at slot %d\r\n", key_slot);
                    GLogE("  Run 'hsmtest 0x102' first to import the WBC key\r\n");
                }
                break;
            }

            case 0xD0:  // CSAC Full Flow Test (New HSM)
            {
                GLogN("\r\n========== CSAC Full Flow Test (New HSM) ==========\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse arguments: hsmtest 0xD0 [mode] [crt] [crl]
                // mode: 0=CSAC10, 1=CSAC20
                // crt: 1~8 (certificate number)
                // crl: 0~7 (CRL number, CSAC20 only)
                U8 mode = 0;
                U8 crt_no = 1;
                U8 crl_no = 0;

                if (count >= 3) mode = (U8)strtol(gCliArgs[2], NULL, 0);
                if (count >= 4) crt_no = (U8)strtol(gCliArgs[3], NULL, 0);
                if (count >= 5) crl_no = (U8)strtol(gCliArgs[4], NULL, 0);

                GLogN("Mode: %s\r\n", mode == 0 ? "CSAC10" : "CSAC20");
                GLogN("Certificate: %d\r\n", crt_no);
                if (mode == 1) GLogN("CRL: %d\r\n", crl_no);
                GLogN("\r\n");

                // Step 1: Read Certificate
                GLogN("--- Step 1: Read Certificate ---\r\n");
                U8 cert_buf[650] = {0};
                U16 cert_len = 0;
                HAL_StatusTypeDef status = hsm_read_certificate(crt_no, HSM_ALG_RSA, cert_buf, &cert_len);

                if (status != HAL_OK)
                {
                    GLogE("[FAIL] Certificate read failed (status=%d)\r\n", status);
                    break;
                }
                GLogN("[OK] Certificate read success (len=%d)\r\n", cert_len);
                GLogN("  First 16 bytes: ");
                for (int i = 0; i < 16; i++) GLogN("%02X ", cert_buf[i]);
                GLogN("\r\n\r\n");

                // Step 2: Read CRL (CSAC20 only)
                if (mode == 1)
                {
                    GLogN("--- Step 2: Read CRL ---\r\n");
                    U8 crl_buf[512] = {0};
                    U32 crl_len = 0;
                    if (GetCrlEmmc(crl_no, crl_buf, &crl_len) == TRUE)
                    {
                        GLogN("[OK] CRL read success (len=%d)\r\n", crl_len);
                        GLogN("  First 16 bytes: ");
                        for (int i = 0; i < 16; i++) GLogN("%02X ", crl_buf[i]);
                        GLogN("\r\n\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] CRL read failed\r\n\r\n");
                    }
                }

                // Step 3: RSA Sign Test
                GLogN("--- Step 3: RSA Sign Test ---\r\n");
                static const U8 test_seed[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11};
                U8 signature[256] = {0};
                U32 sign_len = 0;

                // Determine key slot and SHA type based on crt_no
                U16 key_slot;
                U8 sha_type;
                if (crt_no % 2 == 0)
                {
                    sha_type = HSM_SHA_256;
                    key_slot = hsm_map_csac_key(crt_no);
                    GLogN("SHA Type: SHA256\r\n");
                }
                else
                {
                    sha_type = HSM_SHA_160;
                    key_slot = hsm_map_csac_key(crt_no);
                    GLogN("SHA Type: SHA1\r\n");
                }
                GLogN("Key Slot: %d\r\n", key_slot);

                status = hsm_sign_rsa_with_seed_direct(test_seed, 8, signature, &sign_len, sha_type, key_slot);

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Sign success (len=%d)\r\n", sign_len);
                    GLogN("  Signature (first 32 bytes):\r\n  ");
                    for (int i = 0; i < 32; i++)
                    {
                        GLogN("%02X ", signature[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                    }
                    GLogN("...\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Sign failed (status=%d)\r\n", status);
                }

                GLogN("\r\n========== CSAC Test Complete ==========\r\n");
                break;
            }

            // ========== HSM Public Key Export (0x111) ==========
            case 0x111:  // HSM_PUBLICKEY_HSM - Export RSA Public Key from HSM
            {
                GLogN("\r\n========== HSM Public Key Export Test (0x111) ==========\r\n");

                uint16_t key_id = HSM_KEY_KEK_RSA;  // Default: KEK RSA key (#149)

                if (count >= 3)
                {
                    key_id = (uint16_t)strtol(gCliArgs[2], NULL, 0);
                }

                GLogN("Key ID: %d\r\n", key_id);
                GLogN("Algorithm: RSA\r\n");

                uint8_t pub_key_buffer[300];
                uint16_t pub_key_length = 0;

                HAL_StatusTypeDef status = hsm_export_public_key(key_id, HSM_ALG_RSA, pub_key_buffer, &pub_key_length);

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Public Key Export Success!\r\n");
                    GLogN("Total Data Length: %d bytes\r\n", pub_key_length);

                    // Parse RSA public key structure:
                    // [Modulus Length: 2 bytes LE] + [Modulus: N bytes] + [Exp Length: 2 bytes LE] + [Exponent: M bytes]
                    if (pub_key_length >= 6)  // Minimum: 2 + 0 + 2 + 2
                    {
                        // Parse Modulus Length (Little Endian)
                        uint16_t modulus_len = (uint16_t)pub_key_buffer[0] | ((uint16_t)pub_key_buffer[1] << 8);
                        uint16_t modulus_offset = 2;

                        // Parse Exponent Length (Little Endian)
                        uint16_t exp_len_offset = modulus_offset + modulus_len;
                        uint16_t exp_len = 0;
                        uint16_t exp_offset = 0;

                        if (exp_len_offset + 2 <= pub_key_length)
                        {
                            exp_len = (uint16_t)pub_key_buffer[exp_len_offset] | ((uint16_t)pub_key_buffer[exp_len_offset + 1] << 8);
                            exp_offset = exp_len_offset + 2;
                        }

                        // Display parsed structure
                        GLogN("\r\n[Parsed Structure]\r\n");
                        GLogN("Modulus Length: %d bytes (0x%04X)\r\n", modulus_len, modulus_len);

                        if (modulus_len == 256)
                            GLogN("Key Type: RSA-2048\r\n");
                        else if (modulus_len == 128)
                            GLogN("Key Type: RSA-1024\r\n");

                        GLogN("Modulus (first 32 bytes):\r\n  ");
                        for (int i = 0; i < 32 && i < modulus_len; i++)
                        {
                            GLogN("%02X ", pub_key_buffer[modulus_offset + i]);
                            if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                        }
                        GLogN("...\r\n");

                        GLogN("Exponent Length: %d bytes (0x%04X)\r\n", exp_len, exp_len);
                        GLogN("Exponent: ");
                        for (int i = 0; i < exp_len && (exp_offset + i) < pub_key_length; i++)
                        {
                            GLogN("%02X ", pub_key_buffer[exp_offset + i]);
                        }

                        // Check if exponent is 65537 (0x00010001)
                        if (exp_len == 4 && exp_offset + 4 <= pub_key_length)
                        {
                            uint32_t exp_value = (uint32_t)pub_key_buffer[exp_offset] |
                                                ((uint32_t)pub_key_buffer[exp_offset + 1] << 8) |
                                                ((uint32_t)pub_key_buffer[exp_offset + 2] << 16) |
                                                ((uint32_t)pub_key_buffer[exp_offset + 3] << 24);
                            if (exp_value == 65537)
                                GLogN("(65537 = 0x10001)\r\n");
                            else
                                GLogN("(%u)\r\n", exp_value);
                        }
                        else if (exp_len == 3)
                        {
                            uint32_t exp_value = (uint32_t)pub_key_buffer[exp_offset] |
                                                ((uint32_t)pub_key_buffer[exp_offset + 1] << 8) |
                                                ((uint32_t)pub_key_buffer[exp_offset + 2] << 16);
                            GLogN("(%u)\r\n", exp_value);
                        }
                        else
                        {
                            GLogN("\r\n");
                        }
                    }
                    else
                    {
                        GLogN("Raw Public Key:\r\n  ");
                        for (int i = 0; i < pub_key_length && i < 64; i++)
                        {
                            GLogN("%02X ", pub_key_buffer[i]);
                            if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                        }
                        if (pub_key_length > 64) GLogN("...\r\n");
                    }
                }
                else
                {
                    GLogE("[FAIL] RSA Public Key Export failed (status=%d)\r\n", status);
                    GLogN("[HINT] Key may not exist. Generate RSA key first:\r\n");
                    GLogN("       hsmtest 0x05    - Generate RSA key pair (#149)\r\n");
                }

                GLogN("=============================================\r\n");
                break;
            }

            // ========== HSM Kauth Import (0x112) - OP Code 0x22 ==========
            case 0x112:  // Kauth Import - RSA encrypted key only (per SPI Manual v4)
            {
                GLogN("\r\n========== Kauth Import Test (0x112 -> OP 0x22) ==========\r\n");
                GLogN("[USAGE] hsmtest 0x112 <mode> <rsa_key_id>\r\n");
                GLogN("        mode: 0=Runtime RSA Encrypt, 1=Placeholder\r\n");
                GLogN("        rsa_key_id: 149=Key Pair slot, 241=Public Only slot (default)\r\n");
                GLogN("[SCENARIO 1] hsmtest 0x05 -> hsmtest 0x112 0 149 (Key Pair)\r\n");
                GLogN("[SCENARIO 2] hsmtest 0x62 -> hsmtest 0x112 0 241 (Public Only)\r\n\r\n");

                // Test Kauth key (32 bytes) - will be RSA encrypted
                static uint8_t test_kauth[32] = {
                    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,
                    0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00,
                    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
                    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
                };

                uint8_t test_mode = 0;  // 0=runtime RSA encrypt, 1=use placeholder
                uint16_t rsa_key_id = 241;  // Default: TEMP RSA Pub (#241)

                if (count >= 3)
                {
                    test_mode = (uint8_t)strtol(gCliArgs[2], NULL, 0);
                }
                if (count >= 4)
                {
                    rsa_key_id = (uint16_t)strtol(gCliArgs[3], NULL, 0);
                }

                GLogN("Test Mode: %s\r\n", test_mode ? "Placeholder (will fail)" : "Runtime RSA Encrypt");
                GLogN("RSA Key: #%d (%s)\r\n", rsa_key_id,
                      (rsa_key_id == 149) ? "Key Pair slot" :
                      (rsa_key_id == 241) ? "TEMP RSA Pub" : "Custom");
                GLogN("Target Slot: #103 (Kauth, HSM internal)\r\n");
                GLogN("Test Kauth Key (32 bytes, plaintext):\r\n  ");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_kauth[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                }

                if (test_mode == 0)
                {
                    // Use HSM internal RSA engine for encryption (not software CMOX)
                    // This is more reliable as HSM knows its own key format

                    // Step 1: RSA encrypt Kauth using HSM internal engine
                    GLogN("\r\n[STEP 1] RSA Encrypt Kauth using HSM (Key #%d)...\r\n", rsa_key_id);
                    GLogN("         Using hsm_rsa_encrypt() - HSM internal RSA engine\r\n");
                    if (rsa_key_id == 149)
                        GLogN("         [NOTE] Run 'hsmtest 0x05' first to generate key pair at #149\r\n");
                    else
                        GLogN("         [NOTE] Run 'hsmtest 0x62' first to import RSA key at #241\r\n");

                    uint8_t encrypted_kauth[256];
                    memset(encrypted_kauth, 0, sizeof(encrypted_kauth));

                    HAL_StatusTypeDef status = hsm_rsa_encrypt(
                        rsa_key_id,         // Key ID: #149 or #241
                        test_kauth,         // Input: 32-byte Kauth
                        32,                 // Input length
                        encrypted_kauth     // Output: 256-byte RSA ciphertext
                    );

                    if (status != HAL_OK)
                    {
                        GLogE("[FAIL] HSM RSA encryption failed (status=%d)\r\n", status);
                        if (rsa_key_id == 149)
                            GLogN("[HINT] Generate RSA key pair first: hsmtest 0x05\r\n");
                        else
                            GLogN("[HINT] Import RSA key first: hsmtest 0x62\r\n");
                        GLogN("=============================================\r\n");
                        break;
                    }

                    GLogN("[OK] HSM RSA encryption success\r\n");
                    for (int i = 0; i < 256; i++)
                    {
                        GLogN("%02X ", encrypted_kauth[i]);
                        if ((i + 1) % 16 == 0) GLogN("\r\n  ");
                    }
                    GLogN("...\r\n");

                    // Step 2: Import encrypted Kauth to HSM
                    GLogN("\r\n[STEP 2] Import Encrypted Kauth to HSM (OP 0x22)...\r\n");

                    status = hsm_import_kauth_key(encrypted_kauth, 256);

                    if (status == HAL_OK)
                    {
                        GLogN("[OK] Kauth Import Success!\r\n");
                        GLogN("Key stored at slot #103 (HSM internal)\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] Kauth Import failed (status=%d)\r\n", status);

                        uint8_t err_code = 0;
                        hsm_get_last_error(&err_code);
                        GLogN("[ERROR] HSM Error Code: 0x%02X\r\n", err_code);

                        if (err_code == 0x13)
                            GLogN("  -> Invalid data length\r\n");
                        else if (err_code == 0x23)
                            GLogN("  -> Wrong key type\r\n");
                        else if (err_code == 0x40)
                            GLogN("  -> Internal error\r\n");
                    }
                }
                else
                {
                    // Placeholder mode - just send zeros (will fail)
                    GLogN("\r\n[NOTE] Sending placeholder zeros (256 bytes)...\r\n");

                    static const uint8_t placeholder[256] = {0};
                    HAL_StatusTypeDef status = hsm_import_kauth_key(placeholder, 256);

                    if (status == HAL_OK)
                    {
                        GLogN("[OK] Kauth Import Success (unexpected!)\r\n");
                    }
                    else
                    {
                        GLogE("[FAIL] Expected failure with placeholder data\r\n");
                        GLogN("[HINT] Use 'hsmtest 0x112 0' for runtime RSA encrypt\r\n");
                    }
                }

                GLogN("=============================================\r\n");
                break;
            }
			case 0x113:  // RSA Private Key(encrypt) Import to slot 141 (SHA256 compatibility test)
            {
                GLogN("\r\n");
                GLogN("==================================================================\r\n");
                GLogN("  RSA Private Key(encrypt) Import - Slot 141 (CSAC_PA_SHA1)              \r\n");
                GLogN("  Import test RSA key for SHA256 compatibility verification     \r\n");
                GLogN("==================================================================\r\n\r\n");

                if (g_HSM_Type == HSM_TYPE_OLD)
                {
                    GLogE("[ERROR] This command is for New HSM only\r\n");
                    break;
                }

                // Parse key_slot argument: hsmtest 0x103 [key_slot]
                U16 key_slot = HSM_KEY_CSAC_PA_SHA1;  // Default: 141
                if (count >= 3)
                {
                    key_slot = (U16)strtol(gCliArgs[2], NULL, 0);
                }

                // Test RSA 2048-bit Key (same key as 0x102)
                				
				static const U8 test_modulus[256] = {
					0xEB, 0x64, 0x16, 0xC7, 0xF6, 0x6C, 0x21, 0x0A, 0xDE, 0xA6, 0x8D, 0x7F, 0xFB, 0x2A, 0xAA, 0xAE,
					0x9E, 0xE0, 0x2A, 0x7D, 0xA4, 0x9F, 0xD4, 0xD0, 0x29, 0x45, 0x3A, 0x7F, 0xB1, 0x9E, 0xD5, 0x69,
					0xBF, 0x44, 0x20, 0x95, 0x1D, 0xA5, 0x05, 0xA9, 0x7F, 0x18, 0x92, 0xD4, 0xCF, 0xED, 0x63, 0xC2,
					0xF9, 0xEA, 0xE6, 0x05, 0x1F, 0x59, 0xE3, 0x58, 0x1A, 0x43, 0x8A, 0x22, 0xF0, 0x10, 0xFF, 0xE3,
					0x34, 0x0D, 0x11, 0xC4, 0x59, 0x0C, 0x35, 0xB4, 0x63, 0x78, 0x3A, 0x2E, 0x27, 0x88, 0xC7, 0x65,
					0xEF, 0x23, 0xE0, 0x8C, 0xA0, 0x27, 0x1B, 0x6E, 0x6D, 0x88, 0xE2, 0x39, 0x1E, 0x3A, 0x86, 0x27,
					0xA6, 0xEC, 0x3E, 0x93, 0x0D, 0x59, 0x34, 0xDF, 0xB3, 0x1B, 0x9F, 0xBF, 0x3C, 0x8D, 0x1F, 0x25,
					0x08, 0xBF, 0x59, 0x24, 0x6C, 0xAB, 0x3C, 0x31, 0x89, 0xDF, 0x24, 0xDF, 0x32, 0x8C, 0x9D, 0xD5,
					0x48, 0x1E, 0x0A, 0xD0, 0x62, 0x00, 0xFA, 0x5F, 0x4B, 0x26, 0xB7, 0xFC, 0xC7, 0x1A, 0x76, 0x1A,
					0xA0, 0xAB, 0xD4, 0x4B, 0x4A, 0x11, 0x6C, 0x6B, 0x8B, 0xA6, 0x17, 0x5A, 0xF9, 0x42, 0xD2, 0x84,
					0x4D, 0x17, 0x76, 0x09, 0xD1, 0x3D, 0xB0, 0x99, 0x4D, 0x73, 0x1C, 0xE4, 0x87, 0x9F, 0x06, 0x14,
					0xE2, 0x8A, 0x05, 0x6C, 0xAD, 0x57, 0x24, 0x3E, 0x11, 0x44, 0x3E, 0x0B, 0x17, 0xE8, 0xC6, 0x0B,
					0x8F, 0xD3, 0xBE, 0xAB, 0xB1, 0x79, 0xC4, 0x0A, 0x63, 0x23, 0xDD, 0x8F, 0x30, 0x4E, 0xD9, 0x1C,
					0xC3, 0x8B, 0x39, 0xC0, 0xB4, 0x0A, 0x95, 0xEB, 0x18, 0xC9, 0xF2, 0x16, 0x37, 0xD2, 0x73, 0x02,
					0x37, 0xA4, 0xC5, 0x64, 0xDD, 0x2E, 0x46, 0xAF, 0x16, 0x92, 0x17, 0x41, 0x19, 0x01, 0x63, 0x73,
					0x0C, 0xE7, 0xA9, 0xB4, 0xDA, 0xCB, 0xAB, 0x7D, 0x24, 0xF7, 0xF1, 0xE4, 0x3D, 0x76, 0xC0, 0xA7
				};
                

				
				static const U8 test_upper_private_exp[256] = {
					0x4C, 0x6F, 0x99, 0x37, 0x6D, 0x6F, 0x07, 0x18, 0x52, 0x48, 0x55, 0x4F, 0xF9, 0x91, 0x18, 0xEC,
					0x6C, 0x91, 0x38, 0x3C, 0xD6, 0xCE, 0xA7, 0xA5, 0xBD, 0x51, 0x55, 0xC9, 0x28, 0x5D, 0xEB, 0x85,
					0x7F, 0x33, 0x11, 0x4D, 0x7F, 0x07, 0x4F, 0xA9, 0x5E, 0x52, 0x74, 0x91, 0x73, 0x3D, 0xEA, 0xCC,
					0x7F, 0x83, 0x36, 0x6C, 0xE0, 0xBE, 0xA0, 0x5B, 0x32, 0x5C, 0x4D, 0x3B, 0xDD, 0xC0, 0x51, 0x38,
					0x1A, 0x29, 0xC0, 0xA3, 0xA9, 0xBA, 0x2D, 0xD4, 0x19, 0x39, 0xDB, 0x04, 0xD1, 0xD7, 0x5F, 0xBE,
					0x24, 0xFA, 0xCA, 0x32, 0x7D, 0x2C, 0x7B, 0xCE, 0x6C, 0x37, 0x09, 0x4B, 0xEA, 0xDC, 0x29, 0x92,
					0x32, 0xD9, 0x6F, 0xDD, 0x95, 0xD4, 0x4B, 0xFD, 0xC4, 0x81, 0xB3, 0xF8, 0xCF, 0x88, 0xC8, 0xAE,
					0x30, 0xB2, 0xD6, 0xE1, 0xA9, 0xAC, 0x44, 0xF2, 0xC4, 0xA2, 0x8F, 0x1A, 0x83, 0xAD, 0xD1, 0x12,
					0x6B, 0x2A, 0x41, 0x06, 0xF6, 0xE7, 0x34, 0x86, 0x86, 0xEE, 0x15, 0x66, 0x82, 0x71, 0x9B, 0x1C,
					0xAC, 0x12, 0xE1, 0xDA, 0x2D, 0x95, 0x2B, 0x56, 0x3B, 0x50, 0xD7, 0xF2, 0x63, 0xDC, 0x48, 0xA5,
					0xDE, 0xDE, 0x99, 0x9E, 0x9C, 0xCE, 0x45, 0xC0, 0x0F, 0xC3, 0x1B, 0x12, 0xF9, 0xFB, 0x63, 0x8B,
					0x80, 0xBA, 0xE7, 0x44, 0x74, 0xB2, 0xF5, 0x3E, 0x62, 0xD2, 0x9E, 0x13, 0x1B, 0xF0, 0xF1, 0x12,
					0x45, 0x5F, 0xBC, 0x61, 0x33, 0x3D, 0x68, 0xAD, 0x72, 0xA6, 0x1A, 0x22, 0xB8, 0x13, 0xE2, 0xB3,
					0x2E, 0x7E, 0x16, 0xE2, 0x35, 0xE6, 0x31, 0x68, 0xB6, 0x0E, 0xE4, 0xEA, 0xDF, 0x06, 0x49, 0x09,
					0x55, 0x69, 0x26, 0xFF, 0x72, 0x7E, 0x55, 0xFB, 0x3B, 0xC7, 0xC0, 0xA0, 0x39, 0xFE, 0x2B, 0x89,
					0x8F, 0x72, 0x4D, 0x5F, 0x1A, 0x10, 0x52, 0x31, 0xF2, 0xE6, 0xD6, 0xD1, 0x0F, 0x76, 0x01, 0x79
				};
				
				
                
				
				static const U8 test_lower_private_exp[256] = {
					0x76, 0xBC, 0x95, 0xB8, 0xF5, 0xDE, 0x4D, 0x40, 0x61, 0xA2, 0xAF, 0x1D, 0x39, 0xD8, 0xBC, 0x47,
					0x1B, 0x54, 0x18, 0xEA, 0xC2, 0xC4, 0xA4, 0x7F, 0xC5, 0x1A, 0x2F, 0x64, 0x67, 0xA8, 0x99, 0xD1,
					0x7B, 0xCD, 0x39, 0x0C, 0xF1, 0xDF, 0x13, 0x5A, 0x4C, 0x29, 0x87, 0x0B, 0x75, 0x6D, 0xFB, 0x4D,
					0x2F, 0x19, 0xC4, 0x35, 0x1B, 0xDA, 0x04, 0x54, 0x0B, 0x0B, 0x18, 0xA7, 0x80, 0xB3, 0xAD, 0xD5,
					0xCB, 0x9C, 0xF0, 0xBE, 0x66, 0x19, 0x77, 0xFC, 0xBF, 0x63, 0x4A, 0x18, 0x6D, 0x80, 0x47, 0xA4,
					0x8D, 0xE7, 0x1D, 0x48, 0xBB, 0xF9, 0xD2, 0x89, 0xC4, 0x83, 0x6C, 0x6B, 0xE5, 0xB6, 0x7D, 0x5A,
					0xED, 0xF3, 0x0F, 0x7A, 0x53, 0xE8, 0x25, 0x59, 0x67, 0x51, 0xE9, 0xD2, 0xD0, 0xE0, 0x45, 0xAE,
					0x42, 0x5D, 0xD7, 0x6B, 0x51, 0x1E, 0x30, 0xEE, 0xC8, 0x45, 0x76, 0x53, 0x8E, 0x2E, 0x75, 0xCD,
					0xD3, 0x7B, 0x34, 0x08, 0xB5, 0x2A, 0x26, 0xD1, 0xB2, 0xF1, 0x26, 0xA3, 0x8C, 0x35, 0x76, 0xC5,
					0x30, 0xE4, 0x77, 0x89, 0x6A, 0x8D, 0xA5, 0x31, 0xF4, 0x81, 0x68, 0x86, 0x78, 0x47, 0xCC, 0x03,
					0x2F, 0x0D, 0xED, 0x41, 0x18, 0x12, 0x1F, 0x95, 0x2D, 0xFE, 0xB0, 0xAE, 0x10, 0x98, 0x52, 0x1C,
					0xFB, 0x70, 0x2C, 0xFD, 0x25, 0xE1, 0xA6, 0x2A, 0xCF, 0x85, 0x1F, 0x93, 0xE5, 0x3B, 0xC9, 0x57,
					0x76, 0xFA, 0x6F, 0x19, 0xC9, 0x30, 0xEC, 0x77, 0x0E, 0x5E, 0xB3, 0x89, 0xA0, 0xAD, 0xE1, 0x96,
					0x34, 0xB8, 0xBC, 0xDC, 0x87, 0xD3, 0x43, 0xAE, 0xC9, 0xB6, 0xE7, 0x46, 0x5E, 0x05, 0x55, 0x9E,
					0x36, 0x58, 0xB2, 0xAD, 0x3C, 0x7D, 0x61, 0xA2, 0x5E, 0x3B, 0x6F, 0x23, 0xE2, 0xD8, 0xE8, 0xD3,
					0x2F, 0x07, 0x6A, 0x86, 0xA2, 0x8B, 0x05, 0x5F, 0xEC, 0xAE, 0x2F, 0x13, 0xEA, 0x3A, 0xE5, 0x67
				};
                

                // Public exponent: 65537 (0x010001)
                //static const U8 test_public_exp[3] = { 0x01, 0x00, 0x01 };

                GLogN("Key Parameters:\r\n");
                GLogN("  Target Slot: %d\r\n", key_slot);
                GLogN("  Algorithm: RSA-2048\r\n");
                GLogN("  Public Exponent: 65537 (0x010001)\r\n\r\n");

                GLogN("Modulus (first 32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", test_modulus[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                // Build RSA key structure
                //HSM_RSAKey_t rsa_key;
				HSM_RSAKeyEn_t rsa_keyEn;
                memset(&rsa_keyEn, 0, sizeof(rsa_keyEn));
                rsa_keyEn.modulus_length = 256;
                memcpy(rsa_keyEn.modulus, test_modulus, 256);
				rsa_keyEn.public_exp_size = 0;//private key only, always 0
                //rsa_keyEn.public_exp_size = 3;
                //memcpy(rsa_keyEn.public_exp, test_public_exp, 3);
                memcpy(rsa_keyEn.upper_private_exp, test_upper_private_exp, 256);
				memcpy(rsa_keyEn.lower_private_exp, test_lower_private_exp, 256);

                GLogN("Importing RSA Private Key to slot %d...\r\n", key_slot);

                // Import with private key (public_only=false)
                //HAL_StatusTypeDef status = hsm_import_rsa_key_en(key_slot, &rsa_keyEn, false, false, false, HSM_RSA_2048);
				HAL_StatusTypeDef status = hsm_import_rsa_key_en(key_slot, &rsa_keyEn, 1, true, false, HSM_RSA_2048);

                if (status == HAL_OK)
                {
                    GLogN("[OK] RSA Private Key imported to slot %d\r\n", key_slot);
                    GLogN("     Ready for SHA256 compatibility test (hsmtest 0x101)\r\n");
                }
                else
                {
                    GLogE("[FAIL] RSA Key Import failed, status: %d\r\n", status);
                }
                break;
            }

            default:
                GLogN("OpCode 0x%02X not implemented yet\r\n", opcode);
                GLogN("\r\nAvailable OpCodes:\r\n");
                GLogN("  5-byte W_CMD:\r\n");
                GLogN("    0x00 - Info (HSM Version)\r\n");
                GLogN("    0x01 - Get HSN (Serial Number)\r\n");
                GLogN("    0x02 - Cancel Job\r\n");
                GLogN("    0x0B - Get Last Error\r\n");
                GLogN("  Key Generate:\r\n");
                GLogN("    0x05 - RSA Key Generate + Export (Key #149)\r\n");
                GLogN("    0x52 [key_id] [size] - AES Key Gen (AES: #101-108, TEMP: #201-204)\r\n");
                GLogN("    0x53 [key_id] - ECC Key Generate (ECC: #161-166, TEMP: #261-263)\r\n");
                GLogN("    0x54 [key_id] [enc] - ED Key Import (ED: #161-166, TEMP: #261-263) [enc=1:encrypted(default), 0:plaintext]\r\n");
                GLogN("  Key Import:\r\n");
                GLogN("    0x51 [key_id] - Import ECU-CODE KEK (default=101)\r\n");
                GLogN("    0x55 [key_id] [size] - AES Plaintext Import (test key)\r\n");
                GLogN("    0x73 - Kauth Key Import\r\n");
                GLogN("    0x112 [mode] - Kauth Import (0=RSA encrypt, 1=placeholder)\r\n");
                GLogN("  Key Export:\r\n");
                GLogN("    0x56 [key_id] - ECC Public Key Export\r\n");
                GLogN("    0x57 [key_id] - ED Public Key Export\r\n");
                GLogN("    0x111 [key_id] - RSA Public Key Export (default=149)\r\n");
                GLogN("  Key Exchange/Derivation:\r\n");
                GLogN("    0x07 [key_id] - ECDH Key Exchange (ECC: #161-166, TEMP: #261-263)\r\n");
                GLogN("    0x16 [src_key_id] - HKDF Key Derivation\r\n");
                GLogN("  Crypto:\r\n");
                GLogN("    0x10 - Random (TRNG)\r\n");
                GLogN("    0x11 - SHA-256\r\n");
                GLogN("  AES (0x12): (Key: #101-108, TEMP: #201-204)\r\n");
                GLogN("    0x20 [key_id] [size] - AES Encrypt ECB (run 0x52 first)\r\n");
                GLogN("    0x21 [key_id] [size] - AES Decrypt ECB\r\n");
                GLogN("    0x22 [key_id] [size] - AES Encrypt CBC\r\n");
                GLogN("    0x23 [key_id] [size] - AES Decrypt CBC\r\n");
                GLogN("  RSA Encrypt/Decrypt (0x15):\r\n");
                GLogN("    0x36 [key_id] - RSA Encrypt (PKCS1-v1_5)\r\n");
                GLogN("    0x37 [key_id] - RSA Decrypt (PKCS1-v1_5)\r\n");
                GLogN("  MAC (0x13):\r\n");
                GLogN("    0x13 [key_id] - HMAC-SHA256 (HMAC: #121-124, TEMP: #221-223)\r\n");
                GLogN("    0x14 [key_id] [key_size] - CMAC-AES (AES: #101-108, TEMP: #201-204)\r\n");
                GLogN("  Sign/Verify (0x14): (ECC/ED: #161-166, TEMP: #261-263)\r\n");
                GLogN("    0x30 [key_id] - ECDSA Sign (run 0x53 first)\r\n");
                GLogN("    0x31 [key_id] - ECDSA Verify\r\n");
                GLogN("    0x32 [key_id] - EdDSA Sign (run 0x54 first)\r\n");
                GLogN("    0x33 [key_id] - EdDSA Verify\r\n");
                GLogN("    0x34 [key_id] - RSA Sign (RSA: #141-150)\r\n");
                GLogN("    0x35 [key_id] - RSA Verify\r\n");
                GLogN("  Certificate (0x06):\r\n");
                GLogN("    0x41 [cert_id] - Store Certificate\r\n");
                GLogN("    0x42 [cert_id] - Read Certificate\r\n");
                GLogN("  Authentication:\r\n");
                GLogN("    0x60 [master_id] - ASK Generate Key\r\n");
                GLogN("    0x70 [use_kauth] - Mutual Auth (AES-128)\r\n");
                GLogN("    0x71 [use_kauth] - Mutual Auth (AES-256)\r\n");
                GLogN("    0x74 - One-way Authentication\r\n");
                GLogN("  System:\r\n");
                GLogN("    0x90 [fw_type] - DFU (1=HSE_FW, 2=APP_FW)\r\n");
                GLogN("    0x91 CONFIRM - Set Complete (Lifecycle)\r\n");
                GLogN("  Hardcoded Test:\r\n");
                GLogN("    0xA0 [key_id] - RSA-2048 Public Key Import (default=241, TEMP RSA Public)\r\n");
                GLogN("    0xA1 [key_id] - ED25519 Key (placeholder)\r\n");
                GLogN("    0xA2 [key_id] - HMAC-SHA256 Generate\r\n");
                GLogN("    0xA3 [key_id] - ECU Code Key Plaintext Import (0x0115)\r\n");
                GLogN("    --- ECU-Code Key Verification ---\r\n");
                GLogN("    0xA4 [key_id] - Import Real ECU-Code Key\r\n");
                GLogN("    0xA5 [key_id] - AES-CTR Decrypt Test\r\n");
                GLogN("    0xA6         - Full Verification Flow\r\n");
                GLogN("    0xA7 [key_id] - AES-ECB Round-trip Test (debug)\r\n");
                GLogN("    --- Application Layer Tests (0x0124, 0x0126) ---\r\n");
                GLogN("    0xA8 [key_id] - HSM_READ_PUBKEY Test (default=0)\r\n");
                GLogN("    0xA9         - HSM_READ_RANDKEY Test (2-step method)\r\n");
                GLogN("    0xAA         - 1-Step Encrypted Import Test (production)\r\n");
                GLogN("  FL_Git Function Tests (Old/New HSM):\r\n");
                GLogN("    0xF0         - HSM GetVersion\r\n");
                GLogN("    0xF1         - HSM State Check\r\n");
                GLogN("    0xF2 [crl]   - CRL GetDate\r\n");
                GLogN("    0xF3 [cert]  - CRT GetHolderRef\r\n");
                GLogN("    0xF4         - RSA KeyPair Gen\r\n");
                GLogN("    0xF5         - ECU Code Key Check\r\n");
                GLogN("    0xF6         - Store Encrypt Key\r\n");
                GLogN("    0xF7 [id] [seed] - ASK v2 Gen (id:0-2, seed:1-10)\r\n");
                GLogN("    0xF8 [crl]   - CRL Restore/Store\r\n");
                GLogN("    0xF9 [cert]  - Certificate Full Read\r\n");
                GLogN("  Real Data Store Tests:\r\n");
                GLogN("    0xFA [cert]  - Store Real Certificate (600B)\r\n");
                GLogN("    0xFB [key]   - Store Real Private Key (528B, Old HSM)\r\n");
                GLogN("    0xFC [crl]   - Store Real CRL (501B)\r\n");
                break;
        }
    }
#endif
	else if( !strcmp( gCliArgs[0], "info" ) )
	{
		print_FirmwareInfo();
	}
	else if( !strcmp( gCliArgs[0], "fwinfo" ) )
	{
		printFirmwareInfo();
	}
	else if( !strcmp( gCliArgs[0], "fwinit" ) )
	{
		initFirmwareInfo();
	}
	else if( !strcmp( gCliArgs[0], "reset" ) )
	{
	  	if(g_mqtt_isconnected == 1)
		{
			status = disconnectToMQTT();
			if ( status != RSI_SUCCESS )
			{
				GLogN("MQTT Disconnect Fail(%d)\n",status);
			}
		}
		status = DisconnectWLan( );
		if ( status != RSI_SUCCESS )
		{
			GLogN("DisconnectWLan Fail(%d)\n",status);
		}
	  
//	  	gsFwInfo.mucModeChange = TRUE;
//		gsFwInfo.mucChanged = TRUE;
//		
//		saveFirmwareInfo_EMMC();
		HAL_NVIC_SystemReset();
	}
	else if( !strcmp( gCliArgs[0], "status" ) )
	{
	  	status = rsi_wlan_get_status();
		if ( status != RSI_SUCCESS )
		{
			GLogN("DisconnectWLan Fail(%d)\r\n",status);
		}
	}
	
#ifdef USE_RELAY_MOSA
	else if( !strcmp( gCliArgs[0], "neev" ) )
	{
		if( !strcmp( gCliArgs[1], "on" ) )
		{
			g_ucCanformat = CAN_FRAMEFORMAT_FDCAN;

			stMsgClst *pBatmsg;
			pBatmsg = ( stMsgClst* )osPoolCAlloc( hBatRelayConPool );

			pBatmsg->mMod = BatRelayConStatus_Init;
			pBatmsg->mSeq = BatRelayConType_NEEV;

			osMessagePut(hBatRelayConMsg, (uint32_t)pBatmsg, osWaitForever);
		}
		else if( !strcmp( gCliArgs[1], "off" ) )
		{
			stMsgClst *pBatmsg;
			pBatmsg = ( stMsgClst* )osPoolCAlloc( hBatRelayConPool );

			pBatmsg->mMod = BatRelayConStatus_Stop;
			pBatmsg->mSeq = BatRelayConType_NEEV;

			osMessagePut(hBatRelayConMsg, (uint32_t)pBatmsg, osWaitForever);
		}
	}
	else if( !strcmp( gCliArgs[0], "osev" ) )
	{
		if( !strcmp( gCliArgs[1], "on" ) )
		{
			g_ucCanformat = CAN_FRAMEFORMAT_CLASSIC;

			stMsgClst *pBatmsg;
			pBatmsg = ( stMsgClst* )osPoolCAlloc( hBatRelayConPool );

			pBatmsg->mMod = BatRelayConStatus_Init;
			pBatmsg->mSeq = BatRelayConType_OSEV;

			osMessagePut(hBatRelayConMsg, (uint32_t)pBatmsg, osWaitForever);
		}
		else if( !strcmp( gCliArgs[1], "off" ) )
		{
			stMsgClst *pBatmsg;
			pBatmsg = ( stMsgClst* )osPoolCAlloc( hBatRelayConPool );

			pBatmsg->mMod = BatRelayConStatus_Stop;
			pBatmsg->mSeq = BatRelayConType_OSEV;

			osMessagePut(hBatRelayConMsg, (uint32_t)pBatmsg, osWaitForever);
		}
	}
#endif

	/********************************************
	    Project-specific
	********************************************/
	else if( !strcmp( gCliArgs[0], "format" ) )
	{
		formatEmmc();
		InitEmmcFolder();
	}
	else if( !strcmp( gCliArgs[0], "kline" ) )
	{
		SelfTest_KLINE();
	}
	else if( !strcmp( gCliArgs[0], "repg" ) )
	{
		uint8_t	ret;

		ret = TestReprogramLine();

		if( ret & ERROR_REPROGRAM_CH03 )			GLogEE( "Reprogram Line CH03 Error...\r\n" );
		if( ret & ERROR_REPROGRAM_CH06 )			GLogEE( "Reprogram Line CH06 Error...\r\n" );
		if( ret & ERROR_REPROGRAM_CH09 )			GLogEE( "Reprogram Line CH09 Error...\r\n" );
		if( ret & ERROR_REPROGRAM_CH11 )			GLogEE( "Reprogram Line CH11 Error...\r\n" );
		if( ret & ERROR_REPROGRAM_CH12 )			GLogEE( "Reprogram Line CH12 Error...\r\n" );
		if( ret & ERROR_REPROGRAM_CH13 )			GLogEE( "Reprogram Line CH13 Error...\r\n" );
		if( ret & ERROR_REPROGRAM_CH14 )			GLogEE( "Reprogram Line CH14 Error...\r\n" );
	}
	else if( !strcmp( gCliArgs[0], "can" ) )
	{
		stMsgClst	*txMsg;
		stFdcanPkt	*txPkt;
		uint32_t	canID;
		uint32_t	i, j;

		GLogI( "[MODE_CAN_Tx_Test]\r\n");

		if( count == 2 )
		{
			switch( atoi( gCliArgs[1] ) )
			{
				case 1 :				// High Can1
				{
#ifdef USE_INTERNAL_CAN_ONLY
					CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_FDCAN );
#else
					mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_FDCAN );
#endif
					startFDCan( 1, 0, 0 );
					break;
				}
				case 2 :				// High Can2
				{
					CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_FDCAN );
					startFDCan( 0, 1, 0 );
					break;
				}

				case 3 :				// Low Can
				{
					CanSet_Baud( &hfdcan1, CAN_100KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
					startFDCan( 0, 0, 1 );
					break;
				}
			}

			osDelay( 100 );

			for( i = 0; i < 1000; i++ )
			{
				// Tx
				txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
				if( txMsg != NULL )
				{
					txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
					if( txPkt != NULL )
					{
						canID = 0x701;

						txPkt->mLen			= 8;

						for( j = 0; j < 8; j++ )			txPkt->mData[j] = j;

						switch( atoi( gCliArgs[1] ) )
						{
#ifdef USE_INTERNAL_CAN_ONLY
							case 1 :
							{
								txPkt->pTarget		= &hfdcan1;
								makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_FDCAN );
								break;
							}
#else
#endif
							case 2 :
							{
								txPkt->pTarget		= &hfdcan1;
								makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_FDCAN );
								break;
							}

							case 3 :				// Low Can
							{
								txPkt->pTarget		= &hfdcan1;
								makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
								break;
							}
						}

						txMsg->pPacket	= (void *)txPkt;

						osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
					}
				}

				osDelay( 10 );
			}
		}
		else
		{
			GLogEE( "argc is bad!!!\r\n" );
		}
	}
	else if( !strcmp( gCliArgs[0], "sensor" ) )
	{
		GLogI( "[MODE_SLEEP_SENSOR]\r\n");
		gotoStandbyMode( WAKEUP_SOURCE_SENSOR );
	}
	else if( !strcmp( gCliArgs[0], "rtc" ) )
	{
		uint8_t	datetime[7];
		Get_RTCData( datetime );
	}
	else if( !strcmp( gCliArgs[0], "bt" ) )
	{
		g_ucBTTest = 1;
	}
	else if( !strcmp( gCliArgs[0], "ethon" ) )
	{
		EnableEthDiag();
		GLogN( "EnableEthDiag\r\n");
	}
	else if( !strcmp( gCliArgs[0], "ethoff" ) )
	{
		DisableEthDiag();
		GLogN( "DisableEthDiag \r\n");
	}
	else if( !strcmp( gCliArgs[0], "make_file" ) )
	{
		GLogN( "make_file \r\n");
		cli_make_test_files();
	}
	else if( !strcmp( gCliArgs[0], "fileparsing" ) )
	{
		GLogN( "fileparsing\r\n");
		cli_test_fileparsing_root(1);
	}
	else if( !strcmp( gCliArgs[0], "emmc_unlink" ) )
	{
		GLogN( "unlink\r\n");
		cli_delete_test_files();
	}
	
#ifdef VCI3_DIAG
	else if( !strcmp( gCliArgs[0], "wifi24" ) )
	{
		switch (count)
		{
			case 1:
			{
			  	if(TestWifiConnect ("iptime") == 1)
				{
					g_ucWifiTest = 1;
				}
				else
				{
					GLogI("Wifi Connect Fail!!\r\n");
					g_ucWifiTest = 0;
				}
			}
			break;

			case 2:
			{
			  	
			}
			break;

			default :
			{
			}
			break;
		}
	}
	else if( !strcmp( gCliArgs[0], "wifi50" ) )
	{
		switch (count)
		{
			case 1:
			{
			  	if(TestWifiConnect ("iptime5G") == 1)
				{
					g_ucWifiTest = 1;
				}
				else
				{
					GLogI("Wifi Connect Fail!!/r/n");
					g_ucWifiTest = 0;
				}
			}
			break;

			case 2:
			{
			  	
			}
			break;

			default :
			{
			}
			break;
		}

	}
#endif
	else if( !strcmp( gCliArgs[0], "boot" ) )
	{
	  	uint32_t 	size = 0;
		uint16_t 	cs = 0;
		TCHAR		FWPath[30]		= { "\0", };
		char		FileName[20]	= {0,};
		FRESULT result = FR_OK;

	  	strcpy(&FileName[0],"vci3_bootloader.bin");

		size = gsFwInfo.msAppInfo[10].mSize;
		cs = gsFwInfo.msAppInfo[10].mCheckSum;

		sprintf( FWPath, "%s/%s", EMMC_APPLICATION_FOLDER, &FileName[0]);

		if ( RestoreApplication( FWPath, size, cs ) == 0 )
		{
			GLogN("RestoreApplication Success\r\n");
		}

		result = f_chdir(DIR_APP);
		if ( result == FR_OK )
		{
			GLogN( "Change Directory %s Ok\r\n", DIR_APP);
		}
		else	return FALSE;

		result = f_unlink(FILENAME_APP_LIST_INI);
		GLogN("f_unlink result : %d", result);

		f_chdir(DIR_ROOT);
	}

#if defined ( SAVE_CAN_LOG )
	else if( !strcmp( gCliArgs[0], "makelog" ) )
	{
	  	MakeCANLogFile();
	}
#endif
	else if( !strcmp( gCliArgs[0], "AppList" ) )
	{
	  	uint8_t buff[1000] = {0,};
	  	uint32_t len = 0;
	  	GetUpdateAppListFromEMMC(buff, &len);

		for(uint32_t i = 0; i < len; i++)
		{
			GLogN("%c", buff[i]);
		}
	}
	else if( !strcmp( gCliArgs[0], "mode" ) )
	{
	  	char mode = 0;
		mode = atoi(gCliArgs[1]);
	  	if( count == 2 )
		{
		  	RunModeSwitch(mode);
		}
	}
#if 0
		else if( !strcmp( gCliArgs[0], "fwcs" ) )
	{
	  	uint8_t		ucTemp[80];
		uint32_t	ret;
		uint32_t	uiEmmcCnt1	= 0;
		uint8_t		ucReadSize = 0;
		uint16_t	uiReadCnt = 0;
		//uint16_t 	uiRemainByte = 0;
		uint32_t	ulFileSize = 0;
		uint16_t	uiCheckSum = 0;
		extern FIL	USERFile;			/* File object for USER */

		ret = f_chdir("/01_Application");
		if ( ret == FR_OK )
		{
			GLogN( "Change Directory %s Ok\r\n", DIR_APP);
		}

		ret = f_open( &USERFile, gCliArgs[1], FA_OPEN_EXISTING | FA_READ );
		if( ret == FR_OK )
		{
			// check file size
			ulFileSize = f_size( &USERFile );
			if( ulFileSize == 0 )
			{
				f_close( &USERFile );
				return 1;
			}

			if(ulFileSize > sizeof(ucTemp))
			{
				ucReadSize = sizeof(ucTemp);
				uiReadCnt = ulFileSize / sizeof(ucTemp);
				//uiRemainByte = ulFileSize % sizeof(ucTemp);
			}
			else
			{
			  	uiReadCnt = 1;
				ucReadSize = ulFileSize;
			}

			if( f_lseek( &USERFile, 0 ) == FR_OK )
			{
			  	for(uint16_t l = 0; l < uiReadCnt+1; l++)
				{
					if( f_read( &USERFile, (void*)ucTemp, ucReadSize, &uiEmmcCnt1 ) == FR_OK )
					{
						for(uint16_t h = 0; h < uiEmmcCnt1; h++)
						{
							GLogN("%02X ", ucTemp[h]);
							uiCheckSum += ucTemp[h];
						}
					}
				}
//				if( uiRemainByte > 0 )
//				{
//				  	if( f_read( &USERFile, (void*)ucTemp, ucReadSize, &uiEmmcCnt1 ) == FR_OK )
//					{
//						for(uint16_t h = 0; h < uiEmmcCnt1; h++)
//						{
//							GLogN("%02X", ucTemp[h]);
//						}
//					}
//				}
				GLogN("\r\n");
				GLogN("CheckSum = %04X \r\n", uiCheckSum);
			}
		}

		f_close( &USERFile );
	}
#endif
#if defined ( SAVE_CAN_LOG )
	else if( !strcmp( gCliArgs[0], "dirlist" ) )
	{
	  	directory_list();
	}
	else if( !strcmp( gCliArgs[0], "readfile" ) )
	{
	  	uint8_t		ucTemp[80];
		uint32_t	ret;
		uint32_t	uiEmmcCnt1	= 0;
		uint8_t		ucReadSize = 0;
		uint16_t	uiReadCnt = 0;
		//uint16_t 	uiRemainByte = 0;
		uint32_t	ulFileSize = 0;
		extern FIL	USERFile;			/* File object for USER */

		f_chdir(DIR_ROOT);

		ret = f_open( &USERFile, gCliArgs[1], FA_OPEN_EXISTING | FA_READ );
		if( ret == FR_OK )
		{
			// check file size
			ulFileSize = f_size( &USERFile );
			if( ulFileSize == 0 )
			{
				f_close( &USERFile );
				return 1;
			}

			if(ulFileSize > sizeof(ucTemp))
			{
				ucReadSize = sizeof(ucTemp);
				uiReadCnt = ulFileSize / sizeof(ucTemp);
				//uiRemainByte = ulFileSize % sizeof(ucTemp);
			}
			else
			{
			  	uiReadCnt = 1;
				ucReadSize = ulFileSize;
			}

			if( f_lseek( &USERFile, 0 ) == FR_OK )
			{
			  	for(uint16_t l = 0; l < uiReadCnt+1; l++)
				{
					if( f_read( &USERFile, (void*)ucTemp, ucReadSize, &uiEmmcCnt1 ) == FR_OK )
					{
						for(uint16_t h = 0; h < uiEmmcCnt1; h++)
						{
							GLogN("%02X", ucTemp[h]);
						}
					}
				}
//				if( uiRemainByte > 0 )
//				{
//				  	if( f_read( &USERFile, (void*)ucTemp, ucReadSize, &uiEmmcCnt1 ) == FR_OK )
//					{
//						for(uint16_t h = 0; h < uiEmmcCnt1; h++)
//						{
//							GLogN("%02X", ucTemp[h]);
//						}
//					}
//				}
				GLogN("\r\n");
			}
		}

		f_close( &USERFile );
	}
#endif
	else if( !strcmp( gCliArgs[0], "mem" ) )
	{
		GLogN("xPortGetFreeHeapSize:%d\r\n",xPortGetFreeHeapSize());
		GLogN("xPortGetMinimumEverFreeHeapSize:%d\r\n",xPortGetMinimumEverFreeHeapSize());
	}
	else if( !strcmp( gCliArgs[0], "memory" ) )
	{
		int *aa=NULL;
		if( atoi(gCliArgs[1]) != 0 )
		{
			aa = (void*)pvPortMalloc(1024*atoi(gCliArgs[1]));
		}
		else
		{
			aa = (void*)pvPortMalloc(1024);
		}
		GLogN("xPortGetFreeHeapSize:%d\r\n",xPortGetFreeHeapSize());
		GLogN("xPortGetMinimumEverFreeHeapSize:%d\r\n",xPortGetMinimumEverFreeHeapSize());
	}
	else if( !strcmp( gCliArgs[0], "UpFwInfo" ) )
	{
	 	 UpdateFWVersion();
	}
	else if( !strcmp( gCliArgs[0], "wifitest" ) )
	{
		/* Connect to AP SSID=LJS, PSK=123456789a with DHCP */
		uint8_t ssid[] = "LJS";
		uint8_t psk[]  = "123456789a";

		if(ConnectAP(ssid, RSI_WPA2, psk) == 0)
		{
			if(SetIPAddressDHCP() == 0)
			{
				GLogN("wifitest OK\r\n");
			}
			else
			{
				GLogE("DHCP fail\r\n");
				DisconnectWLan();
			}
		}
		else
		{
			GLogE("ConnectAP fail\r\n");
			DisconnectWLan();
		}
	}
	else if( !strcmp( gCliArgs[0], "mqttcon" ) )
	{
		/* Connect to MQTT server */
		if(connectToMQTT() == 0)
		{
			GLogN("MQTT connect OK\r\n");
		}
		else
		{
			GLogE("MQTT connect fail\r\n");
		}
	}
	else if( !strcmp( gCliArgs[0], "mqttdiscon" ) )
	{
		/* Disconnect from MQTT server */
		if(disconnectToMQTT() == 0)
		{
			GLogN("MQTT disconnect OK\r\n");
		}
		else
		{
			GLogE("MQTT disconnect fail\r\n");
		}
	}
	else if( !strcmp( gCliArgs[0], "rssi" ) )
	{
		/* Read RSSI and show signal strength */
		int8_t rssi = RSSI_SetLedIndicator();
		if(rssi != 0)
		{
			GLogN("RSSI: %d dBm\r\n", rssi);
		}
	}
	else
	{
		GLogEE( "Not Support Command!!!\r\n" );
	}

	return 0;
}

void CheckPassword( void )
{
    /* 
     *Funtion To Compare HSM number with User Command Input.
     * When uartCliThread occur, This function is called.
     * NUM_OF_HSM is comparing size(It is defined as 6).
     */
    uint8_t ucaGetHSM[SIZE_GIT_CERTIFICATE] = {NULL, };
    uint8_t ucaASCIIHSM[20] = {'0', '0', '0', '0', '0', '0', NULL,};
    uint8_t ucaSerialThree[3] = {NULL, };
    int nGetHSM = SIZE_GIT_CERTIFICATE;
    uint32_t unIsHSMSuccess;
    int nCntNumOffset = 0;
    uint8_t ucGetUpBit;
    uint8_t ucGetDownBit;
    bool bIsNULLHSM = true;
    
    unIsHSMSuccess = ActivationHSM();
    if(unIsHSMSuccess != HSM_SUCCESS) GLogE("\r\nFail ActivationHSM");//return;
      
    unIsHSMSuccess = ReadCertificateHSM(ucaGetHSM, &nGetHSM, 1);
    if(unIsHSMSuccess != HSM_SUCCESS) 
    {
      GLogE("\r\nFail ReadCertificateHSM");//return; 
      memset(ucaGetHSM, 0x00, SIZE_GIT_CERTIFICATE);
    }

    /* To get last Three number of serial */
    for(int i = 0; i < 3; i++)
    {
        ucaSerialThree[i] = gsFwInfo.marrucSerialNo[i + 5];
    }

    for(int i = 0; i < SIZE_GIT_CERTIFICATE; i++)
    {
	    if(ucaGetHSM[i] != NULL)
        {
            bIsNULLHSM = false;
            break;
        }
    }
    
	/* ucaGetHSM Change ASCII code */
    if(!bIsNULLHSM)
    {
        for(int i = 0; i < NUM_OF_HSM; i++)
        {
            if( ('0' <= ucaGetHSM[i] && ucaGetHSM[i] <= '9') || ('A' <= ucaGetHSM[i] && ucaGetHSM[i] <= 'Z') ||	
                ('a' <= ucaGetHSM[i] && ucaGetHSM[i] <= 'z') )
            {
                ucaASCIIHSM[i + nCntNumOffset] = ucaGetHSM[i];
            }
            else
            {
                ucGetUpBit = ucaGetHSM[i] & 0xF0;
                ucGetDownBit = ucaGetHSM[i] & 0x0F;
                ucGetUpBit = ucGetUpBit >> 4;

                if(ucGetUpBit <= 0x09)	
                {
                    ucaASCIIHSM[i + nCntNumOffset] = ucGetUpBit + 0x30;	
                }
                else if(0x0A <= ucaGetHSM[i] && ucaGetHSM[i] <= 0x0F)
                {
                    ucaASCIIHSM[i + nCntNumOffset] = ucGetUpBit + 0x37;
                }
                
                nCntNumOffset++;
                
                if(ucGetDownBit <= 0x09)	
                {
                    ucaASCIIHSM[i + nCntNumOffset] = ucGetDownBit + 0x30;	
                }
                else if(0x0A <= ucaGetHSM[i] && ucaGetHSM[i] <= 0x0F)
                {
                    ucaASCIIHSM[i + nCntNumOffset] = ucGetDownBit + 0x37;
                }
            }
        }
    }

#if 0
    /* little Alpha to Cap Alpha */
    for(int i = 0; i < 16; i++)
    {
        if( gCliArgs[1][i] == NULL) break;

        
        if( ('a' <= gCliArgs[1][i] && gCliArgs[1][i] <= 'z') )
        {
            gCliArgs[1][i] = gCliArgs[1][i] - 0x20;
        }
    }
#endif
    
    
	/* Input Command Compare with HSM */
    if( !strcmp( gCliArgs[0],  (char*)ucaSerialThree ) )
    {
        if( !strcmp( gCliArgs[1],  (char*)ucaASCIIHSM ) )
        {
            /* When Password is correct, Reset FailCount and Turn On CLI */
            m_bIsCorrectPassword = true;
            
            m_ucFailCount = 0;
            WriteBackupSRAM(FAILCOUNT_ADDRESS, m_ucFailCount);

#if 0
            /* Fail Flag Reset */
            m_unFailFlag = 0;
            WriteBackupSRAM(FAILFLAG_ADDRESS + 0, m_unFailFlag);
            WriteBackupSRAM(FAILFLAG_ADDRESS + 1, m_unFailFlag);
            WriteBackupSRAM(FAILFLAG_ADDRESS + 2, m_unFailFlag);
            WriteBackupSRAM(FAILFLAG_ADDRESS + 3, m_unFailFlag);
#endif 
            GLogN("\r\nEnter CLI Mode !!! \r\n");

            return;
        }
    }

    /* When Password is discorrect */
    ReadBackupSRAM(FAILCOUNT_ADDRESS, &m_ucFailCount);
    m_ucFailCount++;
    WriteBackupSRAM(FAILCOUNT_ADDRESS, m_ucFailCount);
	
	
    ReadBackupSRAM(FAILCOUNT_ADDRESS, &m_ucFailCount);
    if(m_ucFailCount == MAX_FAILCOUNT)
    {
        m_unFailFlag = 'L';
        WriteBackupSRAM(FAILFLAG_ADDRESS + 0, m_unFailFlag);
        m_unFailFlag = 'J';
        WriteBackupSRAM(FAILFLAG_ADDRESS + 1, m_unFailFlag);
        m_unFailFlag = 'H';
        WriteBackupSRAM(FAILFLAG_ADDRESS + 2, m_unFailFlag);
        m_unFailFlag = '!';
        WriteBackupSRAM(FAILFLAG_ADDRESS + 3, m_unFailFlag);
        m_unFailFlag = 0xFF;
    }
}

/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/
void uartCliThread( void const *argument )
{
	osEvent	evt;

	uint8_t	argc	= 0;

#if 0
    /* Show Fail Count */
    ReadBackupSRAM(FAILCOUNT_ADDRESS, &m_ucFailCount);
    GLogN("Start %d \r\n", m_ucFailCount);
#endif

    ReadBackupSRAM(FAILFLAG_ADDRESS + 0, &m_unFailFlag);
    if(m_unFailFlag == 'L') ReadBackupSRAM(FAILFLAG_ADDRESS + 1, &m_unFailFlag);
    if(m_unFailFlag == 'J') ReadBackupSRAM(FAILFLAG_ADDRESS + 2, &m_unFailFlag);
    if(m_unFailFlag == 'H') ReadBackupSRAM(FAILFLAG_ADDRESS + 3, &m_unFailFlag);
    if(m_unFailFlag == '!')
    {
        m_unFailFlag = 0xFF;
        GLogN("FAIL\r\n");
    }
    
	for( ;; )
	{
		evt = osSignalWait( CLI_PARSING_START_SIGNAL, osWaitForever );
		if( evt.status == osEventSignal )
		{
            if(m_unFailFlag == 0xFF) continue;
            
			if( gIndex != 0 )
			{
				argc = parsingCliString();

				if( argc != 0 )
				{
					if(m_bIsCorrectPassword == false)
					{
						CheckPassword();
					}
					else runCliCommand( argc );
				}
			}

			if(m_bIsCorrectPassword == true) GLogN( "\r\n>> " );
			

			memset( gCliString, 0, sizeof( gCliString ) );
			gIndex = 0;
		}
	}
}

void cli_make_test_files(void)
{
    const char* DIR_PATH = "/test_file";     /* �� ����: ������ ���� ��� */
    const uint32_t FILE_COUNT   = 100U;      /* ������ ���� ���� */
    const uint32_t FILE_SIZE_MB = 70U;        /* �� ���� ũ��(MB) */
    const uint32_t WRITE_BUF_SIZE = 4096U;    /* ���� ����(4KB) */

    FIL file;
    FRESULT res;
    UINT bw;
    static uint8_t buf[4096];
    char fname[64]; // ��� ���� ������ ���� ���� �� �˳��ϰ�
    uint32_t total_write = (FILE_SIZE_MB * 1024U * 1024U) / WRITE_BUF_SIZE;

    /* �� �߰�: ���丮 ���� ���� Ȯ�� �� ���� */
    FILINFO fno;
    res = f_stat(DIR_PATH, &fno);
    if (res == FR_NO_FILE) {
        printf("Directory %s not found. Creating...\r\n", DIR_PATH);
        res = f_mkdir(DIR_PATH); // ���丮 ����
        if (res != FR_OK) {
            printf("Failed to create directory %s. Error: %d\r\n", DIR_PATH, res);
            return; // ���丮 ���� ���� �� ����
        }
    }

    for (uint32_t i = 0; i < WRITE_BUF_SIZE; i++)
    {
        buf[i] = (uint8_t)(i & 0xFFU);
    }

    for (uint32_t f = 0; f < FILE_COUNT; f++)
    {
        /* �� ����: ���ϸ� ���� �� ���� ��� ���� */
        snprintf(fname, sizeof(fname), "%s/TEST%03lu.DAT", DIR_PATH, (unsigned long)f);

        res = f_open(&file, fname, FA_CREATE_ALWAYS | FA_WRITE);
        if (res != FR_OK)
        {
            printf("f_open failed (%d) for %s\n", res, fname);
            break;
        }

        for (uint32_t w = 0; w < total_write; w++)
        {
            res = f_write(&file, buf, WRITE_BUF_SIZE, &bw);
            if (res != FR_OK || bw != WRITE_BUF_SIZE)
            {
                printf("f_write failed (%d) at %lu/%lu in %s\n",
                       res, (unsigned long)w, (unsigned long)total_write, fname);
                break;
            }
        }

        f_close(&file);
        printf("Created %s (%lu MB)\r\n", fname, (unsigned long)FILE_SIZE_MB);
    }
}

void cli_test_fileparsing_root(uint16_t seq)
{
    const char *path = "/test_file"; /* �� ����: �׽�Ʈ�� ��θ� /test_file�� ���� */
    U16 usSeq = seq;
    U16 usPathLen = (U16)strlen(path);
    U32 uiOutLen = 0;
    FRESULT frResult;
    static U8 outBuf[5000];    /* �ʿ� �� ũ�� ���� */
    uint32_t t0, t1;

    /* �� ������: ������� ���� ���۸� Ư�� ������ �ʱ�ȭ */
    memset(outBuf, 0xCC, sizeof(outBuf));

    t0 = HAL_GetTick();
    frResult = FileParsing((U8*)path, usPathLen, usSeq, outBuf, &uiOutLen);
    t1 = HAL_GetTick();

    if (frResult == FR_OK)
    {
        printf("[OK] FileParsing seq=%u, path=%s, out=%lu bytes, time=%lums\r\n",
               usSeq, path, (unsigned long)uiOutLen, (unsigned long)(t1 - t0));

        /* �� 64����Ʈ ���� */
        //uint32_t dump = (uiOutLen < 64U) ? uiOutLen : 64U;
        for (uint32_t i = 0; i < uiOutLen; i++)
        {
            printf("%02X ", outBuf[i]);
            if ((i + 1) % 16 == 0) { // ���� ���� 16����Ʈ���� �ٹٲ�
                printf("\n");
            }
        }
        printf("\n");
    }
    else
    {
        printf("[ERR] FileParsing failed: %d, time=%lums\r\n",
               frResult, (unsigned long)(t1 - t0));
    }
}

void cli_delete_test_files(void)
{
    const uint32_t FILE_COUNT = 100U;
    char fname[32];
    FRESULT res;
    uint32_t t0, t1;

    t0 = HAL_GetTick();

    for (uint32_t f = 0; f < FILE_COUNT; f++)
    {
        snprintf(fname, sizeof(fname), "TEST%03lu.DAT", (unsigned long)f);

        res = f_unlink(fname);
        if (res == FR_OK)
        {
            printf("Deleted %s\n", fname);
        }
        else if (res == FR_NO_FILE)
        {
            printf("Skip %s (not found)\n", fname);
        }
        else
        {
            printf("Error %d deleting %s\n", res, fname);
        }
    }

    t1 = HAL_GetTick();
    printf("Delete finished in %lu ms\n", (unsigned long)(t1 - t0));
}