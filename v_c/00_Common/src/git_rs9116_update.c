/*******************************************************************************
* @file  rsi_firmware_update_app.c
* @brief
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* The licensor of this software is Silicon Laboratories Inc. Your use of this
* software is governed by the terms of Silicon Labs Master Software License
* Agreement (MSLA) available at
* www.silabs.com/about-us/legal/master-software-license-agreement. This
* software is distributed to you in Source Code format and is governed by the
* sections of the MSLA applicable to Source Code.
*
******************************************************************************/

/*================================================================================
 * @brief : This file contains example application for firmware upgradation from
 * server
 * @section Description :
 * This application demonstrates how to upgrade new firmware to Silicon Labs device
 * using remote TCP server.
 =================================================================================*/

/**
 * Include files
 * */

// include file to refer data types
#include "rsi_data_types.h"

// COMMON include file to refer wlan APIs
#include "rsi_common_apis.h"

// WLAN include file to refer wlan APIs
#include "rsi_wlan_apis.h"
#include "rsi_wlan_non_rom.h"

// socket include file to refer socket APIs
#include "rsi_socket.h"

// Error include files
#include "rsi_error.h"

#include "rsi_bootup_config.h"
#include "rsi_utils.h"
// OS include file to refer OS specific functionality
#include "rsi_os.h"

// socket include file to firmware upgrade APIs
#include "rsi_firmware_upgradation.h"
#include "string.h"
#ifdef RSI_M4_INTERFACE
#include "rsi_board.h"
#include "rsi_chip.h"
#endif
#include "git_global.h"
#include "git_rs9116.h"

// Access point SSID to connect
#define SSID "iPhone_moon"

// Security type
#define SECURITY_TYPE RSI_WPA2

// Password
#define PSK "moonhk800"

// DHCP mode 1- Enable 0- Disable
#define DHCP_MODE 1

// If DHCP mode is disabled given IP statically
#if !(DHCP_MODE)

// IP address of the module
// E.g: 0x650AA8C0 == 192.168.10.101
#define DEVICE_IP "192.168.10.101" //0x650AA8C0

// IP address of Gateway
// E.g: 0x010AA8C0 == 192.168.10.1
#define GATEWAY "192.168.10.1" //0x010AA8C0

// IP address of netmask
// E.g: 0x00FFFFFF == 255.255.255.0
#define NETMASK "255.255.255.0" //0x00FFFFFF

#endif

// Device port number
#define DEVICE_PORT 5001

// Server port number
#define SERVER_PORT 5001

// Server IP address.
//#define SERVER_IP_ADDRESS "192.168.10.100"
#define SERVER_IP_ADDRESS "172.20.10.5"


// Receive data length
#define RECV_BUFFER_SIZE 1027

// Wlan task priority
#define RSI_APPLICATION_TASK_PRIORITY 1

// Wireless driver task priority
#define RSI_DRIVER_TASK_PRIORITY 2

// Wlan task stack size
#define RSI_APPLICATION_TASK_STACK_SIZE 1024

// Wireless driver task stack size
#define RSI_DRIVER_TASK_STACK_SIZE 500

// Memory to initialize driver

//GIT ADD~
#include "ff.h"
#include "common.h"
#include "Rsi_wlan.h"
#include "Rsi_bt_config.h"
#include "Rsi_wlan.h"
#include "firmware.h"

#define UPDATE_1KB 1024
#define RS9116_HEADER_SIZE 64
typedef enum {
    RS_SUCCESS = 1,
    RS_BUFFER_READ_FAIL = -1,
    RS_BUFFER_SIZE_FAIL = -2,
    RS_LSEEK_FAIL = -3,
    RS_INVALID_FILESIZE = -4,
    RS_OPEN_FAIL = -5,
	RS_START_FAIL = -6
} RS9116_UPDATE;

extern rsi_task_handle_t driver_task_handle;
//~GIT ADD

int32_t application_tcp()
{
  uint8_t ip_buff[20];
  int32_t client_socket;
  struct rsi_sockaddr_in server_addr, client_addr;
  int32_t status    = RSI_SUCCESS;
  int32_t recv_size = 0;
  uint8_t resp_buf[20];
#if !(DHCP_MODE)
  uint32_t ip_addr      = ip_to_reverse_hex(DEVICE_IP);
  uint32_t network_mask = ip_to_reverse_hex(NETMASK);
  uint32_t gateway      = ip_to_reverse_hex(GATEWAY);
#else
  uint8_t dhcp_mode = (RSI_DHCP | RSI_DHCP_UNICAST_OFFER);
#endif
  uint8_t send_buffer[3];
  uint8_t recv_buffer[RECV_BUFFER_SIZE];
  uint32_t chunk = 1, fwup_chunk_length, recv_offset = 0, fwup_chunk_type;

#ifndef RSI_M4_INTERFACE
  // Driver initialization
  status = rsi_driver_init(global_buf, GLOBAL_BUFF_LEN);
  if ((status < 0) || (status > GLOBAL_BUFF_LEN)) {
    return status;
  }

  // Silicon Labs module intialisation
  status = rsi_device_init(LOAD_NWP_FW);
  if (status != RSI_SUCCESS) {
    LOG_PRINT("\r\nDevice Initialization Failed, Error Code : 0x%lX\r\n", status);
    return status;
  }
  LOG_PRINT("\r\nDevice Initialization Success\r\n");
#endif

#ifdef RSI_WITH_OS
  rsi_task_handle_t driver_task_handle = NULL;
  // Task created for Driver task
  rsi_task_create((rsi_task_function_t)rsi_wireless_driver_task,
                  (uint8_t *)"driver_task",
                  RSI_DRIVER_TASK_STACK_SIZE,
                  NULL,
                  RSI_DRIVER_TASK_PRIORITY,
                  &driver_task_handle);
#endif

  // WC initialization
  status = rsi_wireless_init(0, 0);
  if (status != RSI_SUCCESS) {
    LOG_PRINT("\r\nWireless Initialization Failed, Error Code : 0x%lX\r\n", status);
    return status;
  }
  LOG_PRINT("\r\nWireless Initialization Success\r\n");

  status = rsi_wlan_get(RSI_FW_VERSION, resp_buf, 18);

  LOG_PRINT("\r\nFirmware version before update: %s\r\n", resp_buf);

  // Scan for Access points
  status = rsi_wlan_scan((int8_t *)SSID, 0, NULL, 0);
  if (status != RSI_SUCCESS) {
    LOG_PRINT("\r\nWLAN AP Scan Failed, Error Code : 0x%lX\r\n", status);
    return status;
  }
  LOG_PRINT("\r\nWLAN AP Scan Success\r\n");

  // Connect to an Access point
  status = rsi_wlan_connect((int8_t *)SSID, SECURITY_TYPE, PSK);
  if (status != RSI_SUCCESS) {
    LOG_PRINT("\r\nWLAN AP Connect Failed, Error Code : 0x%lX\r\n", status);
    return status;
  }
  LOG_PRINT("\r\nWLAN AP Connect Success\r\n");

  // Configure IP
#if DHCP_MODE
  status = rsi_config_ipaddress(RSI_IP_VERSION_4, dhcp_mode, 0, 0, 0, ip_buff, sizeof(ip_buff), 0);
#else
  status            = rsi_config_ipaddress(RSI_IP_VERSION_4,
                                RSI_STATIC,
                                (uint8_t *)&ip_addr,
                                (uint8_t *)&network_mask,
                                (uint8_t *)&gateway,
                                NULL,
                                0,
                                0);
#endif
  if (status != RSI_SUCCESS) {
    LOG_PRINT("\r\nIP Config Failed, Error Code : 0x%lX\r\n", status);
    return status;
  }
  LOG_PRINT("\r\nIP Config Success\r\n");
  LOG_PRINT("RSI_STA IP ADDR: %d.%d.%d.%d \r\n", ip_buff[6], ip_buff[7], ip_buff[8], ip_buff[9]);

  // Create socket
  client_socket = rsi_socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    status = rsi_wlan_get_status();
    LOG_PRINT("\r\nSocket Create Failed, Error Code : 0x%lX\r\n", status);
    return status;
  }
  LOG_PRINT("\r\nSocket Create Success\r\n");

  // Memset client structrue
  memset(&client_addr, 0, sizeof(client_addr));

  // Set family type
  client_addr.sin_family = AF_INET;

  // Set local port number
  client_addr.sin_port = htons(DEVICE_PORT);

  // Bind socket
  status = rsi_bind(client_socket, (struct rsi_sockaddr *)&client_addr, sizeof(client_addr));
  if (status != RSI_SUCCESS) {
    status = rsi_wlan_get_status();
    LOG_PRINT("\r\nBind Failed, Error code : 0x%lX\r\n", status);
    return status;
  }
  LOG_PRINT("\r\nBind Success\r\n");

  // Set server structure
  memset(&server_addr, 0, sizeof(server_addr));

  // Set server address family
  server_addr.sin_family = AF_INET;

  // Set server port number, using htons function to use proper byte order
  server_addr.sin_port = htons(SERVER_PORT);

  // Set IP address to localhost
  server_addr.sin_addr.s_addr = ip_to_reverse_hex(SERVER_IP_ADDRESS);

  // Connect to server socket
  status = rsi_connect(client_socket, (struct rsi_sockaddr *)&server_addr, sizeof(server_addr));
  if (status != RSI_SUCCESS) {
    status = rsi_wlan_get_status();
    LOG_PRINT("\r\nSocket Create Failed, Error Code : 0x%lX\r\n", status);
    return status;
  }
  LOG_PRINT("\r\nConnect to TCP Server Success\r\n");

  LOG_PRINT("\r\nFirmware update start\r\n");

  while (1) {
    // Fill packet type
    if (chunk == 1) {
      send_buffer[0] = RSI_FWUP_RPS_HEADER;
    } else {
      send_buffer[0] = RSI_FWUP_RPS_CONTENT;
    }

    // Fill packet number
    rsi_uint16_to_2bytes(&send_buffer[1], chunk);

    // Send firmware upgrade request to remote peer
    status = rsi_send(client_socket, (int8_t *)send_buffer, 3, 0);
    if (status < 0) {
      status = rsi_wlan_get_status();
      LOG_PRINT("\r\nFailed to Send data to TCP Server, Error Code : 0x%lX\r\n", status);
      return status;
    }

    // Get first 3 bytes from remote peer
    recv_offset = 0;
    recv_size   = 3;
    do {
      status = rsi_recv(client_socket, (recv_buffer + recv_offset), recv_size, 0);
      if (status < 0) {
        status = rsi_wlan_get_status();
        LOG_PRINT("\r\nFailed to Receive data, Error Code : 0x%lX\r\n", status);
        return status;
      }

      // Subtract received bytes from required bytes
      recv_size -= status;

      // Move the receive offset
      recv_offset += status;

    } while (recv_size > 0);

    // Get the received chunk type
    fwup_chunk_type = recv_buffer[0];

    // Get the received chunk length
    fwup_chunk_length = rsi_bytes2R_to_uint16(&recv_buffer[1]);

    // Get packet of chunk length from remote peer
    recv_offset = 0;
    recv_size   = fwup_chunk_length;
    do {
      status = rsi_recv(client_socket, (recv_buffer + recv_offset), recv_size, 0);
      if (status < 0) {
        status = rsi_wlan_get_status();
        LOG_PRINT("\r\nFailed to Receive data from remote peer, Error Code : 0x%lX\r\n", status);
        return status;
      }

      // Subtract received bytes from required bytes
      recv_size -= status;

      // Move the receive offset
      recv_offset += status;

    } while (recv_size > 0);

    // Call corresponding firmware upgrade API based on the chunk type
    if (fwup_chunk_type == RSI_FWUP_RPS_HEADER) {
      // Send RPS header which is received as first chunk
      status = rsi_fwup_start(recv_buffer);
    } else {
      // Send RPS content
      status = rsi_fwup_load(recv_buffer, fwup_chunk_length);
    }

    if (status != RSI_SUCCESS) {
      if (status == 3) {
        // Close the socket
        rsi_shutdown(client_socket, 0);

        LOG_PRINT("\r\nFirmware update complete\r\n");

#ifndef RSI_M4_INTERFACE
#ifdef RSI_WITH_OS
        status = rsi_destroy_driver_task_and_driver_deinit(driver_task_handle);
        if (status != RSI_SUCCESS) {
          LOG_PRINT("\r\nDriver deinit failed, Error Code : 0x%lX\r\n", status);
          return status;
        } else {
          LOG_PRINT("\r\nTask destroy and driver deinit success\r\n");
        }
#endif
#endif
        status = rsi_wireless_deinit();
        if (status != RSI_SUCCESS) {
          LOG_PRINT("\r\nWireless deinit failed, Error Code : 0x%1X\r\n", status);
          return status;
        }

#ifdef RSI_M4_INTERFACE
        RSI_CLK_M4ssRefClkConfig(M4CLK, ULP_32MHZ_RC_CLK);
#endif

#ifndef RSI_M4_INTERFACE
#ifdef RSI_WITH_OS
        // Task created for Driver task
        rsi_task_create((rsi_task_function_t)rsi_wireless_driver_task,
                        (uint8_t *)"driver_task",
                        RSI_DRIVER_TASK_STACK_SIZE,
                        NULL,
                        RSI_DRIVER_TASK_PRIORITY,
                        &driver_task_handle);
#endif
#endif

        status = rsi_wireless_init(0, 0);

        if (status == 0) {
          status = rsi_wlan_get(RSI_FW_VERSION, resp_buf, 18);
          LOG_PRINT("\r\nFirmware version after update: %s\r\n", resp_buf);
        }
        return 0;
      } else {
        LOG_PRINT("\r\nFirmware update failed\n");
        return status;
      }
    }
    chunk++;
  }
}

char Rs9116Update( char *strOpenFileName )
{
	int32_t iStatus    = RSI_SUCCESS;
	uint8_t recv_buffer[RECV_BUFFER_SIZE]={0,};
	uint32_t uiBuffLength=0;
	uint32_t uilseek=0;
	uint32_t uiChunkCount=0;
	uint32_t uiFilesize=0;
	FIL fpVer;
	
	if ( f_open(&fpVer, strOpenFileName, FA_OPEN_EXISTING | FA_READ) == FR_OK )
	{
        if(f_size(&fpVer) >= RS9116_HEADER_SIZE)
        {
        	uiFilesize = f_size(&fpVer)-RS9116_HEADER_SIZE;	//remove header
        	            
            if( (uiFilesize%UPDATE_1KB) == 0 )	{	uiChunkCount = uiFilesize/UPDATE_1KB;	}
            else								{	uiChunkCount = (uiFilesize/UPDATE_1KB)+1;	}

			if ( f_read(&fpVer, recv_buffer, RS9116_HEADER_SIZE, &uiBuffLength) == FR_OK )	//first, send header
	        {
	        	if( uiBuffLength != 0 )
	    		{
	    			iStatus = rsi_fwup_start(recv_buffer);
					GLogN("__iStatus=%d\r\n",iStatus);
					if( iStatus != 0 )	return RS_START_FAIL;
	        	}
				else
				{
					f_close(&fpVer);
					return RS_BUFFER_SIZE_FAIL;
				}
			}
			else
			{
				f_close(&fpVer);
				return RS_BUFFER_READ_FAIL;
			}
			
            uilseek=RS9116_HEADER_SIZE;

			for( int i=0; i<uiChunkCount; i++)
			{
	            if(f_lseek(&fpVer,uilseek) == FR_OK)
	            {
	                if ( f_read(&fpVer, recv_buffer, UPDATE_1KB, &uiBuffLength) == FR_OK )
	                {
	                	if( uiBuffLength != 0 )
                		{
                            //GLogN("%d~%d\r\n",uilseek,uilseek+uiBuffLength);
                            if(i%30 == 0) 
                            {
                            GLogN(".");
                            }
                            else{}

	           				iStatus = rsi_fwup_load(recv_buffer, uiBuffLength);
                            if( iStatus != 0 )
                            {
								GLogN("iStatus=%d\r\n",iStatus);
								f_close(&fpVer);
								return RS_SUCCESS;
                            }
                		}
						else
						{
							f_close(&fpVer);
							return RS_BUFFER_READ_FAIL;
						}
					}
					else
					{
						f_close(&fpVer);
						return RS_BUFFER_READ_FAIL;
					}
					uilseek = uilseek+UPDATE_1KB;
                    osDelay(1);
	            }
				else
				{
					f_close(&fpVer);
					return RS_LSEEK_FAIL;
				}
				
			}
        }
		else
		{
			f_close(&fpVer);
			return RS_INVALID_FILESIZE;
		}
		f_close(&fpVer);
	}
	else
	{
		return RS_OPEN_FAIL;
	}
}

int32_t ReinitRS9116( void )
{
	int32_t status;
	GLogN("~~~~~~~~~~~~~~~~~~~ReinitRS9116~~~~~~~~~~~~~~~~~~~~\r\n");

	rsi_shutdown(0, 0);
	rsi_clear_sockets(0);
	DisconnectWLan();
    if( driver_task_handle != NULL)
    {
        rsi_task_destroy(driver_task_handle);
        driver_task_handle=NULL;
    }
	rsi_device_deinit();
	rsi_driver_deinit();    

	status = rsi_driver_init(global_buf, GLOBAL_BUFF_LEN);
	if((status < 0) || (status > GLOBAL_BUFF_LEN))
	{
		GLogE( "error... rsi_driver_init(global_buf)%d!!!\r\n",status );
		return status;
	}
	GLogN("RSI Driver Initalization %d\r\n", status);

    //rsi_hal_intr_config(rsi_interrupt_handler); //mod.kks to set the interrupt call back 22.07.05

	//! Redpine module initialization
	status = rsi_device_init(LOAD_NWP_FW);

	if(status != RSI_SUCCESS)
	{
		GLogE( "error... rsi_device_init!!!%d\r\n",status );
		return status;
	}

	GLogN("RSI Device Initialization %d\r\n", status);

	rsi_task_create((rsi_task_function_t)rsi_wireless_driver_task, (uint8_t *)"driver_task", RSI_DRIVER_TASK_STACK_SIZE, NULL, RSI_DRIVER_TASK_PRIORITY, &driver_task_handle);

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

	rsi_bt_app_init();

	return INIT_OK;
}

int Updatestart()
{
	int32_t status=0;

	rsi_shutdown(0, 0);

	if( driver_task_handle != NULL )
	{
		status = rsi_destroy_driver_task_and_driver_deinit(driver_task_handle); ///update start
		driver_task_handle= NULL;
	    if (status != RSI_SUCCESS) {
	        GLogN("\r\nDriver deinit failed, Error Code : 0x%lX\r\n", status);
	        return status;
	    } else {
	        GLogN("\r\nTask destroy and driver deinit success\r\n");
	    }
	}
	
	status = rsi_wireless_deinit();
    if (status != RSI_SUCCESS) {
      GLogN("\r\nWireless deinit failed, Error Code : 0x%1X\r\n", status);
      return status;
    }
    GLogN("~~~~~~~~~~~~~~~~~~~~1!!!~~~~%d~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n",status);
}

void RS9116FWUpdate(void)
{
    char cRet=0;
    uint32_t uiOldtime=0;
    uint32_t uiTimeout=40000;
    uint32_t uiRetryCnt=0,uiRetryCntMax=5,uiRet=0;
    char strOpenFileName[]="/01_Application/RS9116fw.bin"; 	//2.8.0

#ifdef FW_TEST_VERSION_OVERRIDE
    /* Persist test version unconditionally before upgrade attempt
     * (success path may end with HAL_NVIC_SystemReset, so save first) */
    (void)Save_RS9116_TestVersion(RS9116_TEST_VERSION_AFTER_UPG);
#endif

	RequestBtDisconnect();
	osDelay(2000);
    cRet = Rs9116Update(strOpenFileName);
	GLogN("Rs9116Update result=%d\r\n",cRet);
	if( cRet != RS_SUCCESS )
	{
    	ReinitRS9116();
		osDelay(2000);
		cRet = Rs9116Update(strOpenFileName);
		GLogN("Rs9116Update2 result=%d\r\n",cRet);
	}

    //Updatestart();
	ReinitRS9116();

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
//					gsFwInfo.mucChanged = TRUE;
//					saveFirmwareInfo_EMMC();
					osDelay(1000);
                    HAL_NVIC_SystemReset();
                    //GLogN("uiRet=%d\r\n",uiRet);
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
}

#ifdef FW_TEST_VERSION_OVERRIDE
/* ============================================================
 * RS9116 Test Version persistence helpers (FW_TEST_VERSION_OVERRIDE 전용)
 *  - File: /01_Application/WLAN_TEST_VER.txt (text, max 20 bytes)
 * ============================================================ */
void Load_RS9116_TestVersion(uint8_t *buf, uint32_t buflen)
{
    FIL     fpVer;
    FRESULT res;
    UINT    uiBytesRead = 0U;
    UINT    i;

    if((buf == NULL) || (buflen == 0U))
    {
        return;
    }

    /* default first - caller always gets a valid string even on IO failure */
    (void)memset(buf, 0, buflen);
    (void)strncpy((char*)buf, RS9116_TEST_VERSION_DEFAULT, buflen - 1U);

    res = f_open(&fpVer, RS9116_TEST_VERSION_FILE, (BYTE)(FA_OPEN_EXISTING | FA_READ));
    if(res != FR_OK)
    {
        GLogN("[Load_RS9116_TestVersion] file not found, use default %s\r\n", buf);
        return;
    }

    (void)memset(buf, 0, buflen);
    res = f_read(&fpVer, buf, (UINT)(buflen - 1U), &uiBytesRead);
    (void)f_close(&fpVer);

    if((res != FR_OK) || (uiBytesRead == 0U))
    {
        GLogE("[Load_RS9116_TestVersion] read fail (res=%d, n=%u), use default\r\n",
              res, (unsigned)uiBytesRead);
        (void)memset(buf, 0, buflen);
        (void)strncpy((char*)buf, RS9116_TEST_VERSION_DEFAULT, buflen - 1U);
        return;
    }

    /* trim CR/LF if present */
    for(i = 0U; i < uiBytesRead; i++)
    {
        if((buf[i] == '\r') || (buf[i] == '\n'))
        {
            buf[i] = 0;
            break;
        }
    }
    buf[buflen - 1U] = 0;
    GLogN("[Load_RS9116_TestVersion] loaded = %s\r\n", buf);
}

int Save_RS9116_TestVersion(const char *version)
{
    FIL     fpVer;
    FRESULT res;
    UINT    uiBytesWritten = 0U;
    UINT    uiLen;

    if(version == NULL)
    {
        return -1;
    }
    uiLen = (UINT)strlen(version);
    if((uiLen == 0U) || (uiLen >= RS9116_TEST_VERSION_MAXLEN))
    {
        GLogE("[Save_RS9116_TestVersion] invalid len %u\r\n", (unsigned)uiLen);
        return -1;
    }

    res = f_open(&fpVer, RS9116_TEST_VERSION_FILE, (BYTE)(FA_CREATE_ALWAYS | FA_WRITE));
    if(res != FR_OK)
    {
        GLogE("[Save_RS9116_TestVersion] open fail (%d)\r\n", res);
        return -1;
    }

    res = f_write(&fpVer, version, uiLen, &uiBytesWritten);
    (void)f_close(&fpVer);

    if((res != FR_OK) || (uiBytesWritten != uiLen))
    {
        GLogE("[Save_RS9116_TestVersion] write fail (res=%d, n=%u)\r\n",
              res, (unsigned)uiBytesWritten);
        return -1;
    }

    GLogN("[Save_RS9116_TestVersion] saved = %s\r\n", version);
    return 0;
}
#endif /* FW_TEST_VERSION_OVERRIDE */
