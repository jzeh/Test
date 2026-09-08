#ifndef __GIT_PROTOCOL_H__
#define __GIT_PROTOCOL_H__

/*----------------------------------------------------------------------
	includes
----------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include "stream_buffer.h"

#include "typedef.h"

#include "git_pool.h"

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define PROTOCOL_UART_ENABLE                            1
#define PROTOCOL_USB_ENABLE                             1
#define PROTOCOL_WIFI_ENABLE                            1
#define PROTOCOL_WEBSOCKET_ENABLE                       1
#define PROTOCOL_MQTT_ENABLE                            1

#define	MESSAGE_DECOMPRESSION_QUEUE_SIZE				10
#define MESSAGE_PARSING_QUEUE_SIZE                      10
#define MESSAGE_TRANSMIT_QUEUE_SIZE                     100
#define MESSAGE_DIAGNOSTIC_QUEUE_SIZE                   7				// To Diagnostic Thread MessageQ
#define	MAX_INTER_PROTO_DATA_LENGTH						4200 			//	J2534 MSG : 4128 + 8*6(PassThurMsg Header) = 4176
#define EVENT_BIT_WS									(1 << 0)
#define EVENT_BIT_MQTT									(1 << 1)

/*----------------------------------------------------------------------
 *   Variable
 *--------------------------------------------------------------------*/
#if( PROTOCOL_USB_ENABLE )
	extern StreamBufferHandle_t		hSBUsbRx;
#endif	// PROTOCOL_USB_ENABLE

#if( PROTOCOL_UART_ENABLE )
	extern StreamBufferHandle_t		hSBUartRx;
	extern StreamBufferHandle_t		hSBUartTx;
#endif	// PROTOCOL_UART_ENABLE

#if( PROTOCOL_WIFI_ENABLE )
	extern StreamBufferHandle_t		hSBWifiRx;
	extern StreamBufferHandle_t		hSBWifiTx;
#endif	// PROTOCOL_WIFI_ENABLE
	
#if( PROTOCOL_WEBSOCKET_ENABLE )
	extern StreamBufferHandle_t		hSBWebSocketRx;
	extern StreamBufferHandle_t		hSBWebSocketTx;
#endif	// PROTOCOL_WEBSOCKET_ENABLE
	
#if( PROTOCOL_MQTT_ENABLE )
	extern StreamBufferHandle_t		hSBMqttRx;
	extern StreamBufferHandle_t		hSBMqttTx;
#endif	// PROTOCOL_MQTT_ENABLE

extern osMessageQId					hParsingMsg;							// To Parsing Thread MessageQ
extern osMessageQId					hTransmitMsg;							// To Transmit Thread MessageQ
extern osThreadId					hParsingTh;
extern osThreadId 					hTransmitTh;
extern osMessageQId                 hDecompressionMsg;
extern uint32_t    					diag_timer;
extern EventGroupHandle_t			hEventGroup;

// MQTT Message Processing State (for abort functionality)
typedef enum
{
	MSG_STATE_IDLE = 0,
	MSG_STATE_PROCESSING,
	MSG_STATE_ABORT_REQUESTED
} msg_processing_state_t;

extern volatile msg_processing_state_t g_msg_processing_state;
extern uint8_t g_bUsbBtBlocked;

/*----------------------------------------------------------------------
	Functions
----------------------------------------------------------------------*/
extern 	int32_t		InitGitProtocol( void );
extern 	void		deinitGitProtocol( void );
extern	int32_t		StartProtocolThread( void );
extern	uint16_t	CalcChecksumGITPtclPayloadFrame( stCommPkt *pInterPtcl );
extern  uint16_t	CalcChecksumPayloadFrame( uint8_t* pPayload, uint32_t unLength );

typedef enum
{	
	NONE_ERROR		=0,
	LENGTH_ERROR	=1,
	ALLOC_ERROR		=2,
	P_LENGTH_ERROR	=3,
	ID_ERROR		=4,
	FRAME_ERROR		=5,
	P_CS_ERROR		=6,
	F_CS_ERROR		=7,
	COUNT_ERROR		=8,
	EOF_ERROR		=9,
	F_CS_COUNT_ERROR=10,
	TIMEOUT_ERROR	=11
}eNakValue;

#endif	// __GIT_PROTOCOL_H__
