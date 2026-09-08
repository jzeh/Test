#ifndef GIT_PROTOCOL_H
#define GIT_PROTOCOL_H

#include <stdint.h>
#include "git_protocol.h"
#include "git_rs9116.h"
#include "git_vci.h"
// Buffer sizes or limits (use the actual values you have in your implementation)

#define GIT_UUID_SIZE	300
#define CDP_PACKET_RECEIVE_SIZE 20000
#define CDP_PACKET_TRANSMIT_SIZE 5000
// External global variables used in different source files
extern uint8_t 			g_ucTempBuffer[CDP_PACKET_RECEIVE_SIZE];
extern uint8_t 			g_ucSocketRxBuffer[1470];
extern unsigned char 	g_ucByteArray[4200];
extern uint8_t 			g_arrOutputGITPtclBuff[MAX_INTER_PROTO_DATA_LENGTH];
extern uint8_t 			g_cdp_UUID[40];
extern uint32_t 		g_cdp_UUID_length;
extern uint8_t 			g_git_UUID[40];
extern uint32_t 		g_git_UUID_length;
extern uint8_t			g_6005_UUID[40];
extern uint32_t			g_6005_UUID_length;
extern uint8_t 			g_6005_gitUUID[40];
extern uint32_t 		g_6005_gitUUID_length;
extern uint8_t 			CDP_Packet[CDP_PACKET_TRANSMIT_SIZE];
extern uint32_t			g_packet_count;
extern uint32_t			g_temp;
extern BOOL				g_OBD_Processing;
extern BOOL				g_diagpairflag;
extern BOOL				g_ListSensor_Endflag;
extern BOOL				g_mqtt_isconnected;
extern BOOL				g_websocket_isconnected;
extern uint16_t			g_connectinfo_address_offset;
extern uint32_t			g_fwupdate_crc32;
// git_hsm
extern uint8_t 			g_ucWorking_Buffer[1500];
extern UUID_Struct		g_HSM_UpdateAck_UUID;	// UUID cached from 0x121D HSM_UpdateStart, used by HSM_Update_Ack (0x121E) for MQTT/WebSocket routing
extern UUID_Struct		g_FWUpdate_UUID;		// UUID cached from 0x0155 FW Update Start, used by 0x0255/0x0257/0x0455 acks (server requires consistent UUID across the FW download session)

// git_vci
extern uint8_t 			g_ucRxBuff[MAX_RX_BUFF_SIZE];

// git_rs9116
extern uint8_t			global_buf[GLOBAL_BUFF_LEN];
extern uint8_t			g_strWifiMacAddress[18];
extern uint8_t 			g_SaveLastTxPacket[RSI_BT_MAX_PAYLOAD_SIZE];
extern uint8_t			g_SaveLastRxPacket[RSI_BT_MAX_PAYLOAD_SIZE];
extern uint8_t			g_arrOutputGITPtclBuff02[3600];

#endif // GIT_PROTOCOL_H
