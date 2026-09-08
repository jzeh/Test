#include <stdint.h>
#include "firmware.h"
#include "git_protocol.h"
#include "git_vci.h"
#include "git_rs9116.h"
#include "git_global.h"

//git_protocol.c

uint8_t					g_ucTempBuffer[CDP_PACKET_RECEIVE_SIZE]	= {0};                   // 임시 버퍼 (누적 버퍼)
uint8_t					g_ucSocketRxBuffer[1470]				= {0};                     // 수신 버퍼
unsigned char			g_ucByteArray[4200]						= {0};
uint8_t					g_arrOutputGITPtclBuff[MAX_INTER_PROTO_DATA_LENGTH];
uint8_t             	g_cdp_UUID[40]							= {0};
uint32_t				g_cdp_UUID_length						= 0;
uint8_t             	g_git_UUID[40]							= {0};
uint32_t				g_git_UUID_length						= 0;
uint8_t             	g_6005_cdp_UUID[40]						= {0};
uint32_t				g_6005_cdp_UUID_length					= 0;
uint8_t             	g_6005_git_UUID[40]						= {0};
uint32_t				g_6005_git_UUID_length					= 0;
uint8_t             	CDP_Packet[CDP_PACKET_TRANSMIT_SIZE]	= {0};
uint32_t    			g_packet_count            				= 0;
uint32_t				g_temp									= 0;
BOOL					g_OBD_Processing 						= false;
BOOL					g_diagpairflag							= false;
BOOL					g_ListSensor_Endflag					= true;
BOOL					g_mqtt_isconnected						= false;
BOOL					g_websocket_isconnected					= false;
uint16_t				g_connectinfo_address_offset			= 0x40e;
uint32_t				g_fwupdate_crc32						= 0;
//git_hsm
uint8_t 				g_ucWorking_Buffer[1500]; //for RSA, Sign
UUID_Struct				g_HSM_UpdateAck_UUID					= {0};	// cached from 0x121D HSM_UpdateStart
UUID_Struct				g_FWUpdate_UUID							= {0};	// cached from 0x0155 FW Update Start

//git_vci
U8 g_ucRxBuff[MAX_RX_BUFF_SIZE];

//git_rs9116
uint8_t	global_buf[GLOBAL_BUFF_LEN];
uint8_t	g_strWifiMacAddress[18]						= {0};
uint8_t	g_SaveLastTxPacket[RSI_BT_MAX_PAYLOAD_SIZE]	= {0,};
uint8_t	g_SaveLastRxPacket[RSI_BT_MAX_PAYLOAD_SIZE]	= {0,};