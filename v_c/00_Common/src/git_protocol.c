/*************************************************************
* NOTE : git_protocol.c
*      protocol
* Author : Lee junho
* Since : 2019.06.11
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include "main.h"
#include "common.h"
#include "usart.h"

#include "usbd_cdc_if.h"
#include "rsi_bt_apis.h"
#include "git_protocol.h"
#include "git_pool.h"
#include "git_function_list.h"
#include "led.h"	//JAY_LED
#include "git_rs9116.h"
#include "git_PassthruDefines.h"
#include "ff.h"
#include "git_hsm.h"
#include "firmware.h"
#include "git_AesEncrypt.h"
#include "git_vci.h"
#include "sw_timer.h"
#include "git_global.h"
#include "git_OBDcomm.h"
#include "git_cli.h"
#include "git_ListDiag.h"

/********************************************************************************************************/
/*   Frame Struct                                                                                       */
/********************************************************************************************************/
/*         0    |    1    |    2    |    3    |    4    |    5    |   ...   |   n-2   |    n-1          */
/*        SOF   |   Len0  |   Len1  |   Mod0  |   Mod1  |   Seq   | Payload |   EOF   |     CS          */
/********************************************************************************************************/
/*   SOF : 0x02   EOF : 0x03                                                                            */
/********************************************************************************************************/
/*   Payload Struct( n = Data + 8 )                                                                     */
/********************************************************************************************************/
/*         0    |   1    |     2     |     3     |      4      |     5      |   6   |   7   |  ... n-1  */
/*        Len0  |  Len1  |  FuncID0  |  FuncID1  |  CurFrame0  | CurFrame1  |  CS0  |  CS1  |  Data     */
/********************************************************************************************************/
#define	MAX_INTER_PROTO_DATA_LENGTH						4200 			//	J2534 MSG : 4128 + 8*6(PassThurMsg Header) = 4176
#define	GITPACKET_FRAME_SIZE_MIN						16				// data size 0
#define GITPACKET_SOF									0x02			// Start of Frame
#define GITPACKET_EOF									0x03			// End of Frame
#define GITPACKET_USB_ACK								0x08
#define GITPACKET_USB_NAK								0xF8
#define	GITPACKET_SEND_ACK								0x06
#define	GITPACKET_SEND_NAK								0x15

#define GITPACKET_SERIAL_MODE0_LAST_PACKET 				0x80 			// Last packet
#define GITPACKET_SOF_IDX								0
#define GITPACKET_LEN_IDX								1
#define GITPACKET_MODE0_IDX								3
#define GITPACKET_MODE1_IDX								4
#define GITPACKET_SEQ_IDX								5
#define GITPACKET_PAYLOAD_IDX							6

#define GITPACKET_PAYLOAD_LEN0_IDX						0
#define GITPACKET_PAYLOAD_LEN1_IDX						1
#define GITPACKET_PAYLOAD_FUNC0_IDX						2
#define GITPACKET_PAYLOAD_FUNC1_IDX						3
#define GITPACKET_PAYLOAD_CURR_FRAME0_IDX				4
#define GITPACKET_PAYLOAD_CURR_FRAME1_IDX				5
#define GITPACKET_PAYLOAD_CHECKSUM0_IDX					6
#define GITPACKET_PAYLOAD_CHECKSUM1_IDX					7
#define GITPACKET_PAYLOAD_DATA_IDX						8
#define GITPACKET_PAYLOAD_HEADER_SIZE					8

#define GITPACKET_MODE1_PAD								0xF1
#define GITPACKET_MODE1_WLAN_BD							0xF3
#define GITPACKET_MODE1_VCI_II							0xF7
#define GITPACKET_MODE1_TRIGGER							0xFA
#define GITPACKET_MODE1_J2534							0xFF

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define UART_STREAM_BUFFER_SIZE							8400
#define USB_STREAM_BUFFER_SIZE							8400
#define WIFI_STREAM_BUFFER_SIZE							1460

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void transmitThread( void const *argument );
void parsingThread( void const *argument );

#if ( PROTOCOL_USB_ENABLE )
	void combineUsbThread( void const *argument );
#endif	// PROTOCOL_USB_ENABLE

#if( PROTOCOL_UART_ENABLE )
	void combineUartThread( void const *argument );
#endif	// PROTOCOL_UART_ENABLE

#if( PROTOCOL_WIFI_ENABLE )
	void combineWifiThread( void const *argument );
#endif	// PROTOCOL_WIFI_ENABLE

#if( PROTOCOL_WEBSOCKET_ENABLE )
	void combineWebSocketThread( void const *argument );
#endif	// PROTOCOL_WEBSOCKET_ENABLE
	
#if( PROTOCOL_MQTT_ENABLE )
	void combineMqttThread( void const *argument );
#endif	// PROTOCOL_MQTT_ENABLE

extern int32_t	bt_spp_transfer( uint8_t *data, uint16_t length );
extern uint32_t	VCI_GetPassThruProtocolID(void);
extern void		TransmitFunction( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID );

#ifdef VCI3_RECORD
extern uint32_t	guiTriggerFuncCnt;
extern stFunctionList gsTriggerFunctions[];
#endif

#ifdef PRINT_MESSAGE_ID
	extern stMESSAGE_ID_INFO stMessageIdInfo[20];
 	extern uint8_t ucMessageIdInfoCnt;
#endif
    
    
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
extern void VCI_DataSniffering(stMsgClst *msg, stCommPkt *pkt, ePKT_TD type);
#endif
/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
#if( PROTOCOL_USB_ENABLE )
	osThreadId				combineUsbTh;
	osThreadDef( cbusb, combineUsbThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );

	StreamBufferHandle_t	hSBUsbRx;
#endif	//PROTOCOL_USB_ENABLE

#if( PROTOCOL_UART_ENABLE )
	osThreadId				combineUartTh;
	osThreadDef( cbuart, combineUartThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );

	StreamBufferHandle_t	hSBUartRx;
	StreamBufferHandle_t	hSBUartTx;
#endif	// PROTOCOL_UART_ENABLE

#if( PROTOCOL_WIFI_ENABLE )
	osThreadId				combineWifiTh;
	osThreadDef( cbwifi, combineWifiThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );

	StreamBufferHandle_t	hSBWifiRx;
	StreamBufferHandle_t	hSBWifiTx;
#endif	// PROTOCOL_WIFI_ENABLE
	
#if( PROTOCOL_WEBSOCKET_ENABLE )
	osThreadId				combineWebSocketTh;
	osThreadDef( cbwebsocket, combineWebSocketThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );

	StreamBufferHandle_t	hSBWebSocketRx;
	StreamBufferHandle_t	hSBWebSocketTx;
#endif	// PROTOCOL_WEBSOCKET_ENABLE
	
#if( PROTOCOL_MQTT_ENABLE )
	osThreadId				combineMqttTh;
	osThreadDef( cbmqtt, combineMqttThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE );

	StreamBufferHandle_t	hSBMqttRx;
	StreamBufferHandle_t	hSBMqttTx;
#endif	// PROTOCOL_MQTT_ENABLE

EventGroupHandle_t		hEventGroup;	
BOOL		        	g_bCsDiffFlag			= 0;
uint8_t             	g_Listdiag_flag 		= 0;
uint8_t					g_parsing_flag			= 0;

// MQTT Message Processing State Management
volatile msg_processing_state_t g_msg_processing_state = MSG_STATE_IDLE;

// New message detection threshold (bytes)
#define NEW_MESSAGE_THRESHOLD_BYTES     40

uint32_t    			diag_timer 				= 0;
extern BOOL				g_bRcvChgFlag;
extern FIL          	g_TempFilepnt,g_SaveTempFilepnt;
extern bool         	g_bDeleteFileFlag;
extern stGITSetConfig	g_stGITSetConfig;

extern uint8_t a1[5];
extern uint8_t aaa_len;

osMessageQId	hParsingMsg;											// To Parsing Thread MessageQ
osMessageQDef( parsing, MESSAGE_PARSING_QUEUE_SIZE, int );

osMessageQId	hTransmitMsg;											// To Transmit Thread MessageQ
osMessageQDef( transmit, MESSAGE_TRANSMIT_QUEUE_SIZE, int );

osThreadId hParsingTh;
osThreadDef( parsing, parsingThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE*4 );

osThreadId hTransmitTh;
osThreadDef( transmit, transmitThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE*4 );//*4 : for AES logic (need at least *3)

uint16_t		g_usLastSentGITFrameLen;
uint8_t 		g_b1003Lock=false;
uint8_t 		g_b3002Lock=false;
uint8_t			g_bUsbBtBlocked = 1U;		/* default: blocked at boot */

extern uint8_t 	g_usSelfTestSocketConnect;
extern uint8_t 	g_ucWifiTest;
extern uint8_t 	g_ucBTTest;

extern BOOL		g_bIsFastInit;
extern BOOL		g_bFastInit_Success;
extern U8		g_ucFastInitRetryCnt;

bool 			g_bEncryptFlag=OFF;

#if ENCRYPT_PRJ
#define DATA_HEADER_SIZE	8
#define DATA_CHECKSUM_SIZE	2
#endif
//extern uint8_t g_ucAES256_Key[32];
uint8_t g_ucAES256_Key[32]={0,};
//uint8_t g_ucAES256_Key[32]="12345678901234567890123456789012";

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
int32_t InitGitProtocol( void )
{
#if( PROTOCOL_USB_ENABLE )
	hSBUsbRx  = xStreamBufferCreate( USB_STREAM_BUFFER_SIZE, 0 );
	if( hSBUsbRx == NULL )
	{
		return -1;
	}
#endif	// PROTOCOL_USB_ENABLE

#if( PROTOCOL_UART_ENABLE )
	hSBUartRx = xStreamBufferCreate( UART_STREAM_BUFFER_SIZE, 0 );
	if( hSBUartRx == NULL )
	{
		return -2;
	}

	hSBUartTx = xStreamBufferCreate( UART_STREAM_BUFFER_SIZE, 0 );
	if( hSBUartTx == NULL )
	{
		return -3;
	}
#endif	// PROTOCOL_UART_ENABLE

#if( PROTOCOL_WIFI_ENABLE )
	hSBWifiRx = xStreamBufferCreate( WIFI_STREAM_BUFFER_SIZE*2, 0 );
	if( hSBWifiRx == NULL )
	{
		return -4;
	}

	hSBWifiTx = xStreamBufferCreate( WIFI_STREAM_BUFFER_SIZE*2, 0 );
	if( hSBWifiTx == NULL )
	{
		return -5;
	}
#endif // PROTOCOL_WIFI_ENABLE
	
#if( PROTOCOL_WEBSOCKET_ENABLE )
	hSBWebSocketRx = xStreamBufferCreate( WIFI_STREAM_BUFFER_SIZE*2, 0 );
	if( hSBWebSocketRx == NULL )
	{
		return -8;
	}

	hSBWebSocketTx = xStreamBufferCreate( WIFI_STREAM_BUFFER_SIZE*2, 0 );
	if( hSBWebSocketTx == NULL )
	{
		return -9;
	}
#endif // PROTOCOL_WEBSOCKET_ENABLE
	
#if( PROTOCOL_MQTT_ENABLE )
	hSBMqttRx = xStreamBufferCreate( WIFI_STREAM_BUFFER_SIZE*15, 0 );
	if( hSBMqttRx == NULL )
	{
		return -10;
	}

	hSBMqttTx = xStreamBufferCreate( WIFI_STREAM_BUFFER_SIZE*2, 0 );
	if( hSBMqttTx == NULL )
	{
		return -11;
	}
#endif // PROTOCOL_WIFI_ENABLE

	//-----------------------------------------------------------------
	//   MessageQ variable
	//-----------------------------------------------------------------
	// To Parsing Thread MessageQ
	hParsingMsg = osMessageCreate( osMessageQ( parsing ), NULL );
	if( hParsingMsg == NULL )
	{
		return -6;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hParsingMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hParsingMsg",sizeof("hParsingMsg"));
#endif

	// To Transmit Thread MessageQ
	hTransmitMsg = osMessageCreate( osMessageQ( transmit ), NULL );
	if( hTransmitMsg == NULL )
	{
		return -7;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hTransmitMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hTransmitMsg",sizeof("hTransmitMsg"));
#endif
	
	hEventGroup = xEventGroupCreate();
	return 0;
}

void deinitGitProtocol( void )
{
}

int32_t StartProtocolThread( void )
{
	/* Create crypto mutex before parsing(decrypt)/transmit(encrypt) threads start,
	 * so concurrent AES/RSA calls are serialized on the shared HW CRC peripheral. */
	Crypto_Mutex_Init();

#if( PROTOCOL_USB_ENABLE )
	combineUsbTh = osThreadCreate( osThread( cbusb ), NULL );
	if( combineUsbTh == NULL )
	{
		return -1;
	}
#endif	// PROTOCOL_USB_ENABLE

#if( PROTOCOL_UART_ENABLE )
	combineUartTh = osThreadCreate( osThread( cbuart ), NULL );
	if( combineUartTh == NULL )
	{
		return -2;
	}
#endif	// PROTOCOL_UART_ENABLE

#if( PROTOCOL_WIFI_ENABLE )
	combineWifiTh = osThreadCreate( osThread( cbwifi ), NULL );
	if( combineWifiTh == NULL )
	{
		return -3;
	}
#endif // PROTOCOL_WIFI_ENABLE
	
#if( PROTOCOL_WEBSOCKET_ENABLE )
	combineWebSocketTh = osThreadCreate( osThread( cbwebsocket ), NULL );
	if( combineWebSocketTh == NULL )
	{
		return -6;
	}
#endif // PROTOCOL_WEBSOCKET_ENABLE
	
#if( PROTOCOL_MQTT_ENABLE )
	combineMqttTh = osThreadCreate( osThread( cbmqtt ), NULL );
	if( combineMqttTh == NULL )
	{
		return -7;
	}
#endif // PROTOCOL_WIFI_ENABLE

	hParsingTh = osThreadCreate( osThread( parsing ), NULL );
	if( hParsingTh == NULL )
	{
		return -4;
	}

	hTransmitTh = osThreadCreate( osThread( transmit ), NULL );
	if( hTransmitTh == NULL )
	{
		return -5;
	}

	return 0;
}

u16 CalcChecksumGITPtclPayloadFrame( stCommPkt *pInterPtcl )
{
	u16	usReturnVaule	= GITPACKET_PAYLOAD_DATA_IDX;
	u16	usPayloadLen	= pInterPtcl->mLen;
	u8* p				= (unsigned char*)&pInterPtcl->mLen;

	u16	i;

	pInterPtcl->mCS = 0;

	// copy payload header
	for( i = 0; i < GITPACKET_PAYLOAD_DATA_IDX; i++ )			usReturnVaule += *(p+i);

	// copy payload data
	for( i = 0; i < usPayloadLen; i++ )							usReturnVaule += *(pInterPtcl->mData+i);

	return usReturnVaule;
}

u16 CalcChecksumPayloadFrame( uint8_t* pPayload, uint32_t unLength )
{
	u16	usReturnVaule	= 0;

	// copy payload data
	for(int i = 0; i < unLength; i++ )
		usReturnVaule += pPayload[i];

	return usReturnVaule;
}

u8 CalcChecksumGITPtclFromArray( u8 *pBuff, u32 uiLength )
{
	u8	ucCalcCheckSum = 0;

	for( uint32_t i = 1; i < uiLength; i++ )					ucCalcCheckSum += pBuff[i];

	return ucCalcCheckSum;
}

u8 CalcChecksumGITPtclFromQueue( u8 *pBuff, u32 uiLength )
{
	u8	ucCalcCheckSum = 0;

	for( uint32_t i = 0; i < uiLength; i++ )					ucCalcCheckSum += pBuff[i];

	return ucCalcCheckSum;
}
#if defined (USB_SPEED_TEST)
u32 g_uiDeltaTime = 0;
u32 g_uiOldTimer_Timeout = 0;;
#endif
/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/
void transmitThread( void const *argument )
{
	osEvent		evt;

	stMsgClst	*msg;
	stCommPkt	*pkt;
	ePKT_TD		type;

	uint8_t		*bSendBuff	= g_arrOutputGITPtclBuff;
	uint16_t	usPacketLen;
	uint8_t		ucTarget	= 0x00;
#if ENCRYPT_PRJ
		U32 OutputMessageLength=0,i;
		U16 u16PlainDataSize=0;
		U16 u16EncryptDataSize=0;
#endif

	U8		mData[4200];
		
	for(;;)
	{
		evt		= osMessageGet( hTransmitMsg, osWaitForever );
		if( evt.status == osEventMessage )
		{
			msg 	= ( stMsgClst * )evt.value.p;
			pkt		= ( stCommPkt * )msg->pPacket;
			type	= msg->mPktType;
#if ENCRYPT_PRJ
			uint32_t 	T_offset = 0;
			memset(CDP_Packet,0,sizeof(CDP_Packet));
			pkt->mCurFrame = 0;//buffer clear(plain text size)
			if((pkt->mFuncID == 0x1303)
			   	||(pkt->mFuncID == 0x1301)
				||(pkt->mFuncID == 0x0145)
				||(pkt->mFuncID == 0x0146)
				||(pkt->mFuncID == 0x1238) //VCI2 only
				||(pkt->mFuncID == 0x1239)
				||(pkt->mFuncID == 0x123A)
				||(pkt->mFuncID == 0x123B)
				||(pkt->mFuncID == 0x1701)
				||(pkt->mFuncID == 0x1702)
				||(pkt->mFuncID == 0x1703)
				||(pkt->mFuncID == 0x1704)
				||(pkt->mFuncID == 0x1705)
				||(pkt->mFuncID == 0x1706)  
				||(pkt->mFuncID == 0x1707)
				||(pkt->mFuncID == 0x1803)
				//||(((pkt->mFuncID>>8)&0x00FF) == 0xC0)//C0XX (to wlan) //VCI2 only
				||(((pkt->mFuncID>>8)&0x00FF) == 0xD0)//D0XX (to trigger)
				||(eApp_Inside == gsFwInfo.mucCurrentMode)//recording mode
				)
			{
				//plain text only
#if ENCRYPT_PRJ_LOG
				GLogI("\r\nplain text only(0x%04X)",pkt->mFuncID);
#endif
				//memcpy(&pBuffer[6], &J2534SendDataBuffer[J2534SendFrameDataCounter], SendDataLength);
			}
			else if((g_bEncryptFlag==ON)&&(pkt->mLen>0))
				//&&(ModeCode2!=0xF3)&&(ModeCode2!=0xFA))//except wlan, trigger 
			{
		
				if(g_bEncryptLogOnFlag == true)
				{
					GLogI("\r\nEncrypt");

					GLogN( "\r\nPlainMSG(tx) :" );
					for(i=0;i<pkt->mLen;i++)
					{
						GLogN( "%02X ", pkt->mData[i]);
					}
					GLogN( "\r\n" );
				}

				u16PlainDataSize = pkt->mLen;
                if(pkt->mFuncID == 0x0456)
                {
                    getAESEncoding_ECB(&pkt->mData[0], u16PlainDataSize, g_ucAES256_Key, AES256, &mData[0], &OutputMessageLength );
                }
				getAESEncoding_ECB(&pkt->mData[0], u16PlainDataSize, g_ucAES256_Key, AES256, &mData[0], &OutputMessageLength );
                
                memcpy(pkt->mData, &mData[0], OutputMessageLength);
				//u16EncryptDataSize = (OutputMessageLength+DATA_HEADER_SIZE);
				u16EncryptDataSize = (OutputMessageLength);
#if ENCRYPT_PRJ_LOG
				GLogN( "\r\nEncryMSG :" );
				for(i=0;i<OutputMessageLength;i++)
				{
					GLogN( "%02X ", pkt->mData[i]);
				}
				GLogN( "\r\n" );
#endif
				if(u16EncryptDataSize>0)
				{
					pkt->mLen = u16EncryptDataSize;//change length (plain data size -> encrypt data size)
					pkt->mCurFrame = u16PlainDataSize;//current frame = plain data size
					//if(0 < (pkt->mCurFrame))
					//{
					//	GLogN( "mCurFrame : %d\r\n", pkt->mCurFrame);
					//}
		
					//update checksum
					pkt->mCS = 0;
					pkt->mCS = CalcChecksumGITPtclPayloadFrame(pkt);
				}
			}
			
#endif

			// copy SOF
			bSendBuff[GITPACKET_SOF_IDX] = GITPACKET_SOF;

			// copy Length
			pkt->mLen += 8;
			usPacketLen = pkt->mLen + 5;
			memcpy(&bSendBuff[GITPACKET_LEN_IDX], &usPacketLen, sizeof(usPacketLen));

			// copy payload header
			memcpy(&bSendBuff[GITPACKET_PAYLOAD_IDX], &pkt->mLen, GITPACKET_PAYLOAD_HEADER_SIZE);

			// copy payload data
			memcpy(&bSendBuff[GITPACKET_PAYLOAD_IDX+GITPACKET_PAYLOAD_HEADER_SIZE],
				   pkt->mData,
				   pkt->mLen-GITPACKET_PAYLOAD_HEADER_SIZE);

			bSendBuff[GITPACKET_MODE0_IDX] = GITPACKET_SERIAL_MODE0_LAST_PACKET;
			bSendBuff[GITPACKET_MODE1_IDX] = ucTarget;

			bSendBuff[GITPACKET_SEQ_IDX] = ++(msg->mSeq);
			bSendBuff[usPacketLen+1] = GITPACKET_EOF;

			// checksum
			bSendBuff[usPacketLen+2] = CalcChecksumGITPtclFromArray(bSendBuff, usPacketLen + 2);

			g_usLastSentGITFrameLen = usPacketLen + 3;

			if(type == PACKET_MQTT || type == PACKET_WEBSOCKET )
			{
				memcpy(&CDP_Packet[T_offset], "SOF", 3);														//	SOF
				T_offset = 3;
				
				CDP_Packet[T_offset] = (g_usLastSentGITFrameLen + 39) & 0xff;									//	Total length
				CDP_Packet[T_offset + 1] = ((g_usLastSentGITFrameLen + 39) >> 8) & 0xff;
				T_offset += 2;
				
				CDP_Packet[T_offset] = pkt->UUID.cdpUUIDLen;												//	UUID length
				CDP_Packet[T_offset + 1] = 0;
				T_offset += 2;
				
				memcpy(&CDP_Packet[T_offset], pkt->UUID.cdpUUID, pkt->UUID.cdpUUIDLen);					//	UUID
				T_offset += pkt->UUID.cdpUUIDLen;
				
				CDP_Packet[T_offset] = 0x01;																	//	Packet Count
				CDP_Packet[T_offset + 1] = 0x00;
				T_offset += 2;
				
				CDP_Packet[T_offset] = pkt->UUID.gitUUIDLen;												// packet1 UUID length
				CDP_Packet[T_offset + 1] = 0x00;
				T_offset += 2;
				
				memcpy(&CDP_Packet[T_offset], pkt->UUID.gitUUID, pkt->UUID.gitUUIDLen);					//	packet1 UUID
				T_offset += pkt->UUID.gitUUIDLen;
				
				CDP_Packet[T_offset] = g_usLastSentGITFrameLen & 0xff;											//	packet1 length
				CDP_Packet[T_offset + 1] = (g_usLastSentGITFrameLen >> 8) & 0xff;
				T_offset += 2;
				
				memcpy(&CDP_Packet[T_offset], bSendBuff, g_usLastSentGITFrameLen);								//	packet1
				T_offset += g_usLastSentGITFrameLen;
				
				CDP_Packet[T_offset] = CalcChecksumPayloadFrame(CDP_Packet + 3, T_offset - 3);					// Total CS
				T_offset += 1;
				
				memcpy(&CDP_Packet[T_offset], "EOF", 3);														// EOF
				T_offset += 3;
			}
            
			if(g_bIsFastInit==TRUE&&g_bFastInit_Success==0&&g_ucFastInitRetryCnt<2)
			{
				g_bIsFastInit=FALSE;
			}
			else
			{
				if(bSendBuff[GITPACKET_PAYLOAD_IDX+GITPACKET_PAYLOAD_FUNC0_IDX]==0x02 && bSendBuff[GITPACKET_PAYLOAD_IDX+GITPACKET_PAYLOAD_FUNC1_IDX]==0x10)
				{
					g_b1003Lock = false;
					g_b3002Lock = false;
				}
				switch( type )
				{
					case PACKET_UART :
					{
						bt_spp_transfer(bSendBuff, g_usLastSentGITFrameLen);
						//GLogI( "out(%02X%02X): ",bSendBuff[9],bSendBuff[8]);
						osSemaphoreRelease(hpairflagSemaphore);
						break;
					}

					case PACKET_USB :
					{
						//g_ret_value_fail_count = 0;
						//while(1)
						//{
							//ret_value = 
								CDC_Transmit_HS((uint8_t *)bSendBuff, g_usLastSentGITFrameLen);
								osSemaphoreRelease(hpairflagSemaphore);
							//if(USBD_BUSY != ret_value)
							//{
							//	break;
							//}
							//g_ret_value_fail_count++;
						//}
						//GLogI( "out(%02X%02X): ",bSendBuff[9],bSendBuff[8]);
#if defined (USB_SPEED_TEST)
						if(g_uiUSBRx_Cnt > 0)
						{
						 	 g_uiOldTimer_Timeout = Get_Tmr();
						}
#endif					
						break;
					}

					case PACKET_WIFI :
					{
#if DEBUG_WIFI_PACKET
						U32 i;
						GLogI( "Wifi data out(%d): ",T_offset );
						for(i = 0; i < T_offset; i++) GLogI( "%02X ", CDP_Packet[i]);
						GLogI( "\r\n" );
#endif
						WifiPacketSend(CDP_Packet, T_offset);

						break;
					}
					case PACKET_WEBSOCKET :
					{
#if 1
						U32 i;
						GLogI( "out(%02X%02X): ",bSendBuff[9],bSendBuff[8]);
						if( g_bwebLogOnTxFlag == true)
						{
							GLogI( "WebSocket data out(%d): ",T_offset );
							for(i = 0; i < T_offset; i++) GLogI( "%02X ", CDP_Packet[i]);
						}
						GLogI( "\r\n" );
#endif
						WebsockPacketSend(CDP_Packet, T_offset);
						if(pkt->mFuncID != 0x1003 && pkt->UUID.pendingflag != true)
						{
						  	g_temp++;
							osSemaphoreRelease(hpairflagSemaphore);
						}
//                        tt_end = rsi_hal_gettickcount();
//                        GLogI("tt_start : %d tt_end : %d   %d \r\n", tt_start, tt_end, tt_end- tt_start);
						break;
					}
					case PACKET_MQTT :
					{
#if 1					
					  	healthcheckcnt = 30000;
						U32 i;
						GLogI( "out(%02X%02X): ",bSendBuff[9],bSendBuff[8]);
						if( g_bMqttLogOnTxFlag == true)
						{
							for(i = 0; i < T_offset; i++) GLogI( "%02X ", CDP_Packet[i]);
						}
						GLogI( "\r\n" );
#endif
						MQTTPacketSend(MQTT_MESSAGE_TYPE_GENERAL, CDP_Packet, T_offset);
						if(pkt->mFuncID != 0x1003 && pkt->UUID.pendingflag != true && pkt->mFuncID != 0x1006)
						{
						  	g_temp++;
							osSemaphoreRelease(hpairflagSemaphore);
						}
                        //tt_end = rsi_hal_gettickcount();
                        //GLogI("tt_start : %d tt_end : %d   %d \r\n", tt_start, tt_end, tt_end- tt_start);
						break;
					}

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
                    case PACKET_INTER_ANALYSIS:
                        if ( pkt->mFuncID == 0x1002 /*FL_GitPassThruReadMsgs*/ ) // 
                        {
                            VCI_DataSniffering(msg, pkt, type);
                        }
                        break;
#endif

					default :
					{
						GLogE( "unknown PacketTpye( %d )!!! \r\n", msg->mPktType );
						break;
					}
				}

			}

			osPoolFree( hCommPKPool, (void *)pkt );
			osPoolFree( hMsgPool, (void *)msg );
		}

		//osDelay(1); 
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#else
        //osDelay( 1 );
#endif

	}
}

/*----------------------------------------------------------------------
 *   Combine Thread
 *--------------------------------------------------------------------*/
#if( PROTOCOL_UART_ENABLE )

#define BT_MAX_TIMEOUT 1000 // 1 second

#if true //mod.kks ORG Code
void combineUartThread( void const *argument )
{
	uint8_t		dummy[2], Header[6], oldSeqNum;
	stMsgClst	*msg;
	stCommPkt	*pkt;

	uint32_t	waitTime	= 0;
	uint32_t	count		= 0;
	uint32_t	uiMutiFrameCopyIndex = 0;
	uint32_t	uiCopyLen	= 0;
	uint8_t		ucCalcChecksum = 0;
	uint8_t		ucResult = NONE_ERROR;
	u16			usRecieve_mFuncID = 0;

	for(;;)
	{
		ucCalcChecksum = 0;

		if(g_ucBTTest == 1)
		{
		  	count = xStreamBufferReceive( hSBUartRx, (void *)&Header[0], 6, osWaitForever);
		  	if( g_ucBtConnected == BT_SPP_CONNECT )
			{
				bt_spp_transfer( (uint8_t *)Header, 6 );
			}
			continue;
		}
		else
		  	count = xStreamBufferReceive( hSBUartRx, (void *)&Header[0], 1, osWaitForever);

		if( count != 1 )
		{
			GLogE( "Error... receive SOF from Stream Buffer!!!\r\n" );
			xStreamBufferReset( hSBUartRx );
			continue;
		}

		if( Header[0] == GITPACKET_SOF )					// start of frame
		{
			count = xStreamBufferReceive( hSBUartRx, (void *)&Header[1], 5, pdMS_TO_TICKS( BT_MAX_TIMEOUT ) );
			if( count != 5 )
			{
				GLogE( "Error... receive Frame Length from Stream Buffer!!!\r\n" );
				ucResult = LENGTH_ERROR;
				goto CBU_ERROR;
			}
			ucCalcChecksum += CalcChecksumGITPtclFromQueue(&Header[1], 5);

			uiCopyLen = (Header[1] & 0x00FF) + ((Header[2] << 8) & 0xFF00);
			if ( uiMutiFrameCopyIndex == 0 )
			{
				msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
				if( msg == NULL )
				{
                    vTaskDelay(0);
                    msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
                    if( msg == NULL )
                    {
                        ucResult = ALLOC_ERROR;
                        goto CBU_ERROR;
                    }
				}

				pkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
				if( pkt == NULL )
				{
                    vTaskDelay(5);
                    pkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
                    if( pkt == NULL )
                    {
                        osPoolFree( hMsgPool, (void *)msg );
                        ucResult = ALLOC_ERROR;					
                        goto CBU_ERROR;
                    }
				}

				// Frame
				// Get length
				msg->mLen = Header[2];
				msg->mLen <<= 8;
				msg->mLen += Header[1];

				// Get Mode
				msg->mMod = Header[3];
				msg->mMod <<= 8;
				msg->mMod += Header[4];

				// Get Sequence
				msg->mSeq = Header[5];

				// Payload
				// Get length
				count = xStreamBufferReceive( hSBUartRx, (void *)dummy, 2, pdMS_TO_TICKS( BT_MAX_TIMEOUT ) );
				if( count != 2 )
				{
					GLogE( "Error... receive Payload Length from Stream Buffer!!!\r\n" );
					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					ucResult = P_LENGTH_ERROR;
					
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(dummy, 2);

				pkt->mLen = dummy[1];
				pkt->mLen <<= 8;
				pkt->mLen += dummy[0];

				// Get functionID
				count = xStreamBufferReceive( hSBUartRx, (void *)dummy, 2, pdMS_TO_TICKS( BT_MAX_TIMEOUT ) );
				if( count != 2 )
				{
					GLogE( "Error... receive Payload FunctionID from Stream Buffer!!!\r\n" );
					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					ucResult = ID_ERROR;
					
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(dummy, 2);

				pkt->mFuncID = dummy[1];
				pkt->mFuncID <<= 8;
				pkt->mFuncID += dummy[0];
				usRecieve_mFuncID = pkt->mFuncID;

				// Get current frame
				count = xStreamBufferReceive( hSBUartRx, (void *)dummy, 2, pdMS_TO_TICKS( BT_MAX_TIMEOUT ) );
				if( count != 2 )
				{
					GLogE( "Error... receive Payload Current Frame from Stream Buffer!!!\r\n" );
					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					ucResult = FRAME_ERROR;
					
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(dummy, 2);

				pkt->mCurFrame = dummy[1];
				pkt->mCurFrame <<= 8;
				pkt->mCurFrame += dummy[0];

				// Get Payload Checksum
				count = xStreamBufferReceive( hSBUartRx, (void *)dummy, 2, pdMS_TO_TICKS( BT_MAX_TIMEOUT ) );
				if( count != 2 )
				{
					GLogE( "Error... receive Payload Checksum from Stream Buffer!!!\r\n" );
					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					ucResult = P_CS_ERROR;
					
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(dummy, 2);

				pkt->mCS = dummy[1];
				pkt->mCS <<= 8;
				pkt->mCS += dummy[0];

				uiCopyLen -= (5 + GITPACKET_PAYLOAD_HEADER_SIZE);
			}
			else
			{
				// Frame
				// Get length
				msg->mLen = Header[2];
				msg->mLen <<= 8;
				msg->mLen += Header[1];

				// Get Mode
				msg->mMod = Header[3];
				msg->mMod <<= 8;
				msg->mMod += Header[4];

				// Get Sequence
				msg->mSeq = Header[5];

				uiCopyLen -= 5;
			}

			if( uiCopyLen != 0 )
			{
				waitTime = 300;			// 3s
				while( xStreamBufferBytesAvailable( hSBUartRx ) < uiCopyLen )
				{
					if(  waitTime == 0 )		break;
					waitTime--;
					osDelay( 10 );
				}

				if( waitTime == 0 )
				{
					GLogE( "Error... receive Payload Data from uart!!!\r\n" );

					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					ucResult = TIMEOUT_ERROR;
					
					goto CBU_ERROR;
				}

				count = xStreamBufferReceive( hSBUartRx, (void *)&pkt->mData[uiMutiFrameCopyIndex], uiCopyLen, pdMS_TO_TICKS( BT_MAX_TIMEOUT ) );
				if( count != uiCopyLen )
				{
					GLogE( "Error... receive Payload Data from Stream Buffer!!!\r\n" );
					ucResult = COUNT_ERROR;
					
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(&pkt->mData[uiMutiFrameCopyIndex], uiCopyLen);
			}

			if(Header[3] == 0x00)
			{
				uiMutiFrameCopyIndex += uiCopyLen;
			}
			else
			{
				uiMutiFrameCopyIndex = 0;
			}

			// Get EOF
			count = xStreamBufferReceive( hSBUartRx, (void *)&dummy[0], 1, pdMS_TO_TICKS( BT_MAX_TIMEOUT ) );
			if( count != 1 )
			{
				GLogE( "Error... receive EOF from Stream Buffer!!!\r\n" );

				osPoolFree( hMsgPool, (void *)msg );
				osPoolFree( hCommPKPool, (void *)pkt );
				ucResult = EOF_ERROR;
				
				goto CBU_ERROR;
			}
			ucCalcChecksum += dummy[0];

			// Get Frame Checksum
			count = xStreamBufferReceive( hSBUartRx, (void *)&msg->mCS, 1, pdMS_TO_TICKS( BT_MAX_TIMEOUT ) );
			if( count != 1 )
			{
				GLogE( "Error... receive Frame Checksum from Stream Buffer!!!\r\n" );

				osPoolFree( hMsgPool, (void *)msg );
				osPoolFree( hCommPKPool, (void *)pkt );
				ucResult = F_CS_COUNT_ERROR;

				goto CBU_ERROR;
			}

			if ( msg->mCS != ucCalcChecksum )
			{
				GLogE( "Error... Checksum diff  %02X %02X\r\n", msg->mCS, ucCalcChecksum);

				osPoolFree( hMsgPool, (void *)msg );
				osPoolFree( hCommPKPool, (void *)pkt );
				ucResult = F_CS_ERROR;

				goto CBU_ERROR;
			}

			msg->mMsgType	= MSG_COMM;
			msg->mPktType	= PACKET_UART;
			msg->pPacket	= (void*)pkt;

			if( ( msg->mMod & 0xFF00 ) == 0x8000 )
			{
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
			else if( ( msg->mMod & 0xFF00) == 0x4000 )
			{
				if ( oldSeqNum == msg->mSeq )
				{
					dummy[0] = GITPACKET_SEND_ACK;
					HAL_UART_Transmit( &huart2, (uint8_t *)dummy, 1, 0xFFFF );
				}

				osPoolFree( hCommPKPool, (void *)pkt );
				osPoolFree( hMsgPool, (void *)msg );
			}

			oldSeqNum = msg->mSeq;
		}
		else
		{
			GLogE( "uError... receive %02X \r\n",Header[0] );
		}

		continue;

CBU_ERROR :
#if 1
		TransmitFunction(PACKET_UART, &ucResult, 1, usRecieve_mFuncID);
#endif
		xStreamBufferReset( hSBUartRx );
		g_bDeleteFileFlag = true;
		g_SaveTempFilepnt = g_TempFilepnt;
		g_bWifiAutoConDisable	= 0;
		GLogI( "E]_%d\r\n",ucResult );
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#endif

	}
}
#else //RX test mod.kks
#define PACKETSIZE 20
void combineUartThread( void const *argument )
{


#define ENABLE_BT_RX_TEST


	uint8_t	buffer[PACKETSIZE];
    static uint8_t	s_buffer2[PACKETSIZE];
	int read;
	int write;
	int count;
	int precount;

	uint32_t unTimeout = 0;

//#if defined(ENABLE_BT_TX_TEST)	
	for(int i=0;i<PACKETSIZE;i++)
    {
        buffer[i]=i;
		s_buffer2[i]=i;
    }
//#endif
	extern bool g_bBTLogOnRxFlag;

	//g_bBTLogOnRxFlag = true;

    
    static int s_nRcvCount=0;
    
	for(;;)
	{


#if defined(ENABLE_BT_RX_TX_TEST)

	  	read = xStreamBufferReceive( hSBUartRx, (void *)&buffer[0], 20, 1);

		if( read > 0 )
		{
          
		  	if( g_ucBtConnected == BT_SPP_CONNECT )
			{
				bt_spp_transfer( (uint8_t *)buffer, read );
			}

			count += read;
				
			if( Get_Tmr() - unTimeout > 1000 )
			{				
				printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>%s] rx data : %d\r\n", __func__, count - precount);
				precount = count;
				unTimeout = Get_Tmr();
			}
		}
		
#elif defined(ENABLE_BT_TX_TEST)

		if( g_ucBtConnected == BT_SPP_CONNECT )
		{		
			bt_spp_transfer( (uint8_t *)buffer, 100);
		}

		osDelay(rand()%2000);

		printf("%s] rand() : %d\r\n", __func__, rand()%2000);

		write += 1024;

		if( Get_Tmr() - unTimeout > 1000 )
		{
			printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>%s] tx data : %d\r\n", __func__, write);

			write = 0;
			unTimeout = Get_Tmr();
		}
		
#elif defined(ENABLE_BT_RX_TEST)

	  	read = xStreamBufferReceive( hSBUartRx, (void *)&buffer[0], PACKETSIZE, 10);

        static uint8_t s_ucStart;
        static bool s_bStart = false;

        if( read > 0 )
        {
            count += read;
                
            if( Get_Tmr() - unTimeout > 1000 )
            {                
                printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>%s] rx data : %d\r\n", __func__, count - precount);
                precount = count;
                unTimeout = Get_Tmr();
            }

            uint8_t ucStart = buffer[0];

            if( s_bStart == false )
            {
                s_bStart = true;
                s_ucStart = ucStart;
            }

            //printf("%s] rx data : %d\r\n", __func__, read);
            if( memcmp(buffer, &s_buffer2[ucStart], read) != 0 )
            {
                printf("%s] packet error read : %d\r\n", __func__, read);

                //hexdump(buffer, read);

                s_bStart = false;
            }

//            if( (s_ucStart) != ucStart )
//            {
//                printf("%s] packet start error expected start : %x, rcv start : %x\r\n", __func__, s_ucStart, ucStart);
//            }

            s_ucStart = buffer[read-1] + 1;
            

        }

#endif
		//xStreamBufferReset( hSBUartRx );
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#endif
	}
}

#endif //false

#endif	// PROTOCOL_UART_ENABLE



#if( PROTOCOL_USB_ENABLE )
void combineUsbThread( void const *argument )
{
	uint8_t		dummy[2], Header[6], oldSeqNum;
	stMsgClst	*msg;
	stCommPkt	*pkt;

	uint32_t	waitTime	= 0;
	uint32_t	count		= 0;
	uint32_t	uiMutiFrameCopyIndex = 0;
	uint32_t	uiCopyLen	= 0;
	uint8_t		ucCalcChecksum = 0;
	uint8_t		ucResult = NONE_ERROR;
	uint16_t	usRecieve_mFuncID = 0;

	for(;;)
	{
		ucCalcChecksum = 0;
		count = xStreamBufferReceive( hSBUsbRx, (void *)&Header[0], 1, osWaitForever);

		if( count != 1 )
		{
			GLogE( "Error... receive SOF from Stream Buffer!!!\r\n" );
			xStreamBufferReset( hSBUsbRx );
			continue;
		}

		if( Header[0] == GITPACKET_SOF )					// start of frame
		{
			count = xStreamBufferReceive( hSBUsbRx, (void *)&Header[1], 5, pdMS_TO_TICKS( 20 ) );
			if( count != 5 )
			{
				GLogE( "Error... receive Frame Length from Stream Buffer(%d)!!!\r\n",count );
				xStreamBufferReset( hSBUsbRx );
				ucResult = LENGTH_ERROR;
				goto CBU_ERROR;
			}
			ucCalcChecksum += CalcChecksumGITPtclFromQueue(&Header[1], 5);

			uiCopyLen = (Header[1] & 0x00FF) + ((Header[2] << 8) & 0xFF00);
			if ( uiMutiFrameCopyIndex == 0 )
			{
				msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
				if( msg == NULL )
				{
					GLogE( "hMsgPool osPoolCAlloc!!!\r\n" );
					ucResult = ALLOC_ERROR;
					goto CBU_ERROR;
				}
				pkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
				if( pkt == NULL )
				{
					GLogE( "hCommPKPool osPoolCAlloc!!!\r\n" );
					osPoolFree( hMsgPool, (void *)msg );
					ucResult = ALLOC_ERROR;					
					goto CBU_ERROR;
				}

				// Frame
				// Get length
				msg->mLen = Header[2];
				msg->mLen <<= 8;
				msg->mLen += Header[1];

				// Get Mode
				msg->mMod = Header[3];
				msg->mMod <<= 8;
				msg->mMod += Header[4];

				// Get Sequence
				msg->mSeq = Header[5];

				// Payload
				// Get length
				count = xStreamBufferReceive( hSBUsbRx, (void *)dummy, 2, pdMS_TO_TICKS( 20 ) );
				if( count != 2 )
				{
					GLogE( "Error... receive Payload Length from Stream Buffer!!!\r\n" );
					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					xStreamBufferReset( hSBUsbRx );
					ucResult = P_LENGTH_ERROR;
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(dummy, 2);
				pkt->mLen = dummy[1];
				pkt->mLen <<= 8;
				pkt->mLen += dummy[0];
				
				// Get functionID
				count = xStreamBufferReceive( hSBUsbRx, (void *)dummy, 2, pdMS_TO_TICKS( 20 ) );
				if( count != 2 )
				{
					GLogE( "Error... receive Payload FunctionID from Stream Buffer!!!\r\n" );
					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					xStreamBufferReset( hSBUsbRx );
					ucResult = ID_ERROR;
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(dummy, 2);
				pkt->mFuncID = dummy[1];
				pkt->mFuncID <<= 8;
				pkt->mFuncID += dummy[0];
				usRecieve_mFuncID = pkt->mFuncID;

				// Get current frame
				count = xStreamBufferReceive( hSBUsbRx, (void *)dummy, 2, pdMS_TO_TICKS( 20 ) );
				if( count != 2 )
				{
					GLogE( "Error... receive Payload Current Frame from Stream Buffer!!!\r\n" );
					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					xStreamBufferReset( hSBUsbRx );
					ucResult = FRAME_ERROR;
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(dummy, 2);
				pkt->mCurFrame = dummy[1];
				pkt->mCurFrame <<= 8;
				pkt->mCurFrame += dummy[0];

				// Get Payload Checksum
				count = xStreamBufferReceive( hSBUsbRx, (void *)dummy, 2, pdMS_TO_TICKS( 20 ) );
				if( count != 2 )
				{
					GLogE( "Error... receive Payload Checksum from Stream Buffer!!!\r\n" );
					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					xStreamBufferReset( hSBUsbRx );
					ucResult = P_CS_ERROR;
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(dummy, 2);
				pkt->mCS = dummy[1];
				pkt->mCS <<= 8;
				pkt->mCS += dummy[0];

				uiCopyLen -= (5 + GITPACKET_PAYLOAD_HEADER_SIZE);
			}
			else
			{
				// Frame
				// Get length
				msg->mLen = Header[2];
				msg->mLen <<= 8;
				msg->mLen += Header[1];

				// Get Mode
				msg->mMod = Header[3];
				msg->mMod <<= 8;
				msg->mMod += Header[4];

				// Get Sequence
				msg->mSeq = Header[5];

				uiCopyLen -= 5;
			}

			if(uiCopyLen != 0)
			{
				waitTime = 300;			// 3s
				while( xStreamBufferBytesAvailable( hSBUsbRx ) < uiCopyLen )
				{
					if(  waitTime == 0 )		break;
					waitTime--;
					osDelay( 10 );

				}

				if( waitTime == 0 )
				{
					GLogE( "Error... receive Payload Data from USB!!!\r\n" );

					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );

					xStreamBufferReset( hSBUsbRx );
					ucResult = TIMEOUT_ERROR;
					goto CBU_ERROR;
				}

				count = xStreamBufferReceive( hSBUsbRx, (void *)&pkt->mData[uiMutiFrameCopyIndex], uiCopyLen, pdMS_TO_TICKS( 20 ) );
				if( count != uiCopyLen )
				{
					GLogE( "Error... receive Payload Data from Stream Buffer!!!\r\n" );
					osPoolFree( hCommPKPool, (void *)pkt );
					osPoolFree( hMsgPool, (void *)msg );
					xStreamBufferReset( hSBUsbRx );
					ucResult = COUNT_ERROR;
					goto CBU_ERROR;
				}
				ucCalcChecksum += CalcChecksumGITPtclFromQueue(&pkt->mData[uiMutiFrameCopyIndex], uiCopyLen);
			}

			if(Header[3] == 0x00)
			{
				uiMutiFrameCopyIndex += uiCopyLen;
			}
			else
			{
				uiMutiFrameCopyIndex = 0;
			}

			// Get EOF
			count = xStreamBufferReceive( hSBUsbRx, (void *)&dummy[0], 1, pdMS_TO_TICKS( 20 ) );
			if( count != 1 )
			{
				GLogE( "Error... receive EOF from Stream Buffer!!!\r\n" );
				osPoolFree( hCommPKPool, (void *)pkt );
				osPoolFree( hMsgPool, (void *)msg );
				xStreamBufferReset( hSBUsbRx );
				ucResult = F_CS_COUNT_ERROR;
				goto CBU_ERROR;
			}
			ucCalcChecksum += dummy[0];

			// Get Frame Checksum
			count = xStreamBufferReceive( hSBUsbRx, (void *)&msg->mCS, 1, pdMS_TO_TICKS( 20 ) );
			if( count != 1 )
			{
				GLogE( "Error... receive Frame Checksum from Stream Buffer!!!\r\n" );
				osPoolFree( hCommPKPool, (void *)pkt );
				osPoolFree( hMsgPool, (void *)msg );
				xStreamBufferReset( hSBUsbRx );
				ucResult = F_CS_COUNT_ERROR;
				goto CBU_ERROR;
			}

			if ( msg->mCS != ucCalcChecksum )
			{
				GLogE( "Error... Checksum diff  %02X %02X\r\n", msg->mCS, ucCalcChecksum);

				osPoolFree( hMsgPool, (void *)msg );
				osPoolFree( hCommPKPool, (void *)pkt );

				xStreamBufferReset( hSBUsbRx );
				ucResult = F_CS_ERROR;
				goto CBU_ERROR;
			}

			msg->mMsgType	= MSG_COMM;
			msg->mPktType	= PACKET_USB;
			msg->pPacket	= (void*)pkt;

			if((msg->mMod&0xFF00) == 0x8000)
			{
				if(osMessageAvailableSpace(hParsingMsg) == 0)
				{
					osPoolFree( hMsgPool, (void *)msg );
					osPoolFree( hCommPKPool, (void *)pkt );
				}
				else
				{
#if defined (USB_SPEED_TEST)
				  	if(g_uiUSBRx_Cnt > 0)
					{
						g_uiDeltaTime = Get_TmrDelta(Get_Tmr(), g_uiOldTimer_Timeout);
						GLogN("%d\r\n", g_uiDeltaTime);
					}
#endif
					osMessagePut( hParsingMsg, (uint32_t)msg, osWaitForever );
				}
			}
			else if((msg->mMod&0xFF00) == 0x4000)
			{
				if ( oldSeqNum == msg->mSeq  )
				{
					dummy[0] = GITPACKET_SEND_ACK;
					CDC_Transmit_HS((uint8_t *)dummy, 1);
				}

				osPoolFree( hCommPKPool, (void *)pkt );
				osPoolFree( hMsgPool, (void *)msg );
			}
			else
			{
			}
			oldSeqNum = msg->mSeq;
		}
		else if(Header[0] == GITPACKET_USB_ACK)
		{
			//xStreamBufferReset( hSBUsbRx );
		}
		else if(Header[0] == GITPACKET_USB_NAK)
		{
			GLogE( "receive USB NAK!!!\r\n" );
			CDC_Transmit_HS((uint8_t *)g_arrOutputGITPtclBuff, g_usLastSentGITFrameLen);
		}
		else
		{
			GLogE( "Error... receive %02X \r\n",Header[0] );
		}
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
    osThreadYield();
#else
    osDelay( 1 );
#endif
		continue;
		
CBU_ERROR :
#if 1
		TransmitFunction(PACKET_USB, &ucResult, 1, usRecieve_mFuncID);
		GLogN("ucResult : %d\r\n", ucResult);
#endif
		
	}
}
#endif	// PROTOCOL_WIFI_ENABLE

#if( PROTOCOL_USB_ENABLE )
void combineWifiThread( void const *argument )
{
	uint8_t		dummy[2], Header[6], oldSeqNum;
	stMsgClst	*msg;
	stCommPkt	*pkt;

	uint32_t	waitTime	= 0;
	uint32_t	count		= 0;
	uint32_t	uiMutiFrameCopyIndex = 0;
	uint32_t	uiCopyLen	= 0;
	uint8_t		ucCalcChecksum = 0;
	EventBits_t Bits;
	for(;;)
	{
		Bits = xEventGroupWaitBits( hEventGroup,					// �̺�Ʈ �׷� �ڵ�
									EVENT_BIT_WS | EVENT_BIT_MQTT,	// ��ٸ� ��Ʈ
									pdTRUE,          				// �а� ���� �ڵ����� Ŭ����
									pdFALSE,						// ��� ��Ʈ�� ��ٸ��� ���� (OR ����)
									portMAX_DELAY					// ���� ���
		);
		
		if (Bits & EVENT_BIT_WS)
		{
		   GLogN("okok111\r\n");
		}
		
		if (Bits & EVENT_BIT_MQTT)
		{
		   GLogN("okok222\r\n");
		}
		osDelay( 10 );
	}
}
#endif	// PROTOCOL_WIFI_ENABLE

#if( PROTOCOL_WEBSOCKET_ENABLE )
void combineWebSocketThread(void const *argument)
{
	uint8_t		dummy[2], Header[6], oldSeqNum;
	stMsgClst	*msg;
	stCommPkt	*pkt;

	uint32_t	waitTime                = 0;
	uint32_t	count			        = 0;
	uint32_t	uiMutiFrameCopyIndex	= 0;
	uint32_t	uiCopyLen		        = 0;
	uint8_t		ucCalcChecksum		    = 0;


	size_t		totalReceivedBytes		= 0;
	size_t		xReceivedBytes			= 0;
	char		*StrStart				= NULL;
	char		*StrEnd					= NULL;

	size_t 		byte_array_len = 0;

	uint32_t	Total_length			= 0;
	uint32_t	Packet_Length			= 0;

	for (;;)
	{
		// ��Ʈ�� ���ۿ��� ������ ���� (���� ���)
		xReceivedBytes = xStreamBufferReceive(hSBWebSocketRx, (void *)g_ucSocketRxBuffer, sizeof(g_ucSocketRxBuffer), portMAX_DELAY);

		if (xReceivedBytes != 0)
		{
			// ���ŵ� �����͸� �ӽ� ���ۿ� ����
			if (totalReceivedBytes + xReceivedBytes < sizeof(g_ucTempBuffer))
			{
				memcpy(&g_ucTempBuffer[totalReceivedBytes], g_ucSocketRxBuffer, xReceivedBytes);
				totalReceivedBytes += xReceivedBytes;
				GLogN("!%d %d\r\n",totalReceivedBytes,xReceivedBytes);
				g_ucTempBuffer[totalReceivedBytes] = '\0'; // ���ڿ� ���� ó��

				// StrStart �ʱ�ȭ
				StrStart = (char *)g_ucTempBuffer;
				unsigned int uiOffset_SOF=0;
				uiOffset_SOF=(int)StrStart;
				if(( StrStart = memstr(StrStart, totalReceivedBytes, "SOF", 3)) != NULL )
				{
					uiOffset_SOF=(int)StrStart - uiOffset_SOF; // ���� ���� �� ����
					StrEnd = memstr(StrStart, totalReceivedBytes - uiOffset_SOF, "EOF", 3);
					if (StrEnd != NULL)
					{
						StrEnd += 3; // "EOF" ���ڿ��� �� ��ġ�� �̵�

						uint32_t offset = 3;	//SOF offset

						Total_length = StrStart[offset] + (StrStart[offset + 1] << 8);
						offset += 2;

						g_cdp_UUID_length = StrStart[offset] + (StrStart[offset + 1] << 8);
						offset += 2;

						memcpy(g_cdp_UUID, &StrStart[offset], g_cdp_UUID_length);
						offset += g_cdp_UUID_length;

						g_packet_count = StrStart[offset] + (StrStart[offset + 1] << 8);
						offset += 2;
						g_temp = 0;

						for(int i = 0; i < g_packet_count; i++)
						{
							g_git_UUID_length = StrStart[offset] + (StrStart[offset + 1] << 8);
							offset += 2;

							memcpy(g_git_UUID, &StrStart[offset], g_git_UUID_length);
							offset += g_git_UUID_length;

							Packet_Length = StrStart[offset] + (StrStart[offset + 1] << 8);
							offset += 2;

							memcpy(g_ucByteArray, &StrStart[offset], Packet_Length);

							// gitprotocol Data paring
							if( g_ucByteArray[0] == GITPACKET_SOF )					// start of frame
							{
								ucCalcChecksum += CalcChecksumGITPtclFromQueue(&g_ucByteArray[1], 5);

								uiCopyLen = (g_ucByteArray[1] & 0x00FF) + ((g_ucByteArray[2] << 8) & 0xFF00);
								if ( uiMutiFrameCopyIndex == 0 )
								{
									msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
									if( msg == NULL )
									{
										continue;
									}
									if (osSemaphoreWait(hMqttPKPoolSemaphore, osWaitForever) != osOK)
									{
										GLogE("Packet pool semaphore wait failed\n");
										continue;  // �Ҵ� �õ����� �ʰ� ���� �����
									}
									pkt	= ( stCommPkt* )osPoolCAlloc( hMqttPKPool );
									if( pkt == NULL )
									{
										osPoolFree( hMsgPool, (void *)msg );
										continue;
									}

									// Frame
									// length
									msg->mLen = g_ucByteArray[2];
									msg->mLen <<= 8;
									msg->mLen += g_ucByteArray[1];

									// Mode
									msg->mMod = g_ucByteArray[3];
									msg->mMod <<= 8;
									msg->mMod += g_ucByteArray[4];

									// Sequence
									msg->mSeq = g_ucByteArray[5];

									// Payload
									pkt->mLen = g_ucByteArray[7];
									pkt->mLen <<= 8;
									pkt->mLen += g_ucByteArray[6];


									pkt->mFuncID = g_ucByteArray[9];
									pkt->mFuncID <<= 8;
									pkt->mFuncID += g_ucByteArray[8];

									// current frame
									pkt->mCurFrame = g_ucByteArray[11];
									pkt->mCurFrame <<= 8;
									pkt->mCurFrame += g_ucByteArray[10];

									// cdp uuid
									memcpy(&(pkt->UUID.cdpUUID), &(g_cdp_UUID), g_cdp_UUID_length);
									pkt->UUID.cdpUUIDLen = g_cdp_UUID_length;

									// git_uuid
									memcpy(&(pkt->UUID.gitUUID), &(g_git_UUID), g_git_UUID_length);
									pkt->UUID.gitUUIDLen = g_git_UUID_length;

									// git_pendingflag 
									pkt->UUID.pendingflag = false;

									// Payload Checksum
									pkt->mCS = g_ucByteArray[13];
									pkt->mCS <<= 8;
									pkt->mCS += g_ucByteArray[12];
									uiCopyLen -= (5 + GITPACKET_PAYLOAD_HEADER_SIZE);
								}
								else
								{
									// Frame
									// length
									msg->mLen = g_ucByteArray[2];
									msg->mLen <<= 8;
									msg->mLen += g_ucByteArray[1];

									// Mode
									msg->mMod = g_ucByteArray[3];
									msg->mMod <<= 8;
									msg->mMod += g_ucByteArray[4];

									// Sequence
									msg->mSeq = g_ucByteArray[5];
									uiCopyLen -= 5 ;
								}
								// Checksum
								if(uiCopyLen != 0)
								{
									memcpy(&pkt->mData[uiMutiFrameCopyIndex], &g_ucByteArray[14], uiCopyLen);
									ucCalcChecksum = CalcChecksumGITPtclFromQueue(&g_ucByteArray[1], uiCopyLen + 5 + GITPACKET_PAYLOAD_HEADER_SIZE + 1);
								}
								else
								{
									ucCalcChecksum = CalcChecksumGITPtclFromQueue(&g_ucByteArray[1], 5 + GITPACKET_PAYLOAD_HEADER_SIZE + 1);
								}

								if(g_ucByteArray[3] == 0x00)
								{
									uiMutiFrameCopyIndex += uiCopyLen;
								}
								else
								{
									uiMutiFrameCopyIndex = 0;
								}

								// Get EOF
								msg->mCS = g_ucByteArray[uiCopyLen + (5 + GITPACKET_PAYLOAD_HEADER_SIZE) + 2];

								if ( msg->mCS != ucCalcChecksum )
								{
									GLogE( "Error... Checksum diffW  %02X %02X\r\n", msg->mCS, ucCalcChecksum);

									if(pkt->mFuncID==0x1202)
									{
										GLogE( "0x1202 cs diff return \r\n");
										g_bCsDiffFlag=1;
										xStreamBufferReset( hSBWebSocketRx );
									}
									else //MONI 20230307 [suresoft]106) : block this code for misraif(pkt->mFuncID!=0x1202)
									{
										osPoolFree( hMsgPool, (void *)msg );
										osPoolFree( hCommPKPool, (void *)pkt );
										uiMutiFrameCopyIndex = 0;
										offset += Packet_Length;
										continue;
									}
								}

								msg->mMsgType	= MSG_COMM;
								msg->mPktType	= PACKET_WEBSOCKET;
								msg->pPacket	= (void*)pkt;

								if((msg->mMod&0xFF00) == 0x8000)
								{
									if(osMessageAvailableSpace(hParsingMsg) == 0)
									{
										osPoolFree( hMqttPKPool, (void *)pkt );
										osPoolFree( hMsgPool, (void *)msg );
									}
									else
									{
										osMessagePut( hParsingMsg, (uint32_t)msg, osWaitForever );
									}
								}
								else if((msg->mMod&0xFF00) == 0x4000)
								{
									if ( oldSeqNum == msg->mSeq  )
									{
										dummy[0] = GITPACKET_SEND_ACK;
										WebsockPacketSend((uint8_t *)dummy, 1);
									}

									osPoolFree( hMqttPKPool, (void *)pkt );
									osPoolFree( hMsgPool, (void *)msg );
								}
								else
								{
									osPoolFree( hMqttPKPool, (void *)pkt );
                                    osPoolFree( hMsgPool, (void *)msg );
								}
								oldSeqNum = msg->mSeq;
							}

							offset += Packet_Length;
						}
						GLogN("Fin Websocket Message\r\n");
						memset(g_ucTempBuffer, 0, sizeof(g_ucTempBuffer));
						totalReceivedBytes = 0;
					}
					else
					{
						// �����Ͱ� �������� ������ ���� ������ ��ٸ�
						continue;
					}
				}
				else
				{
					memset(g_ucTempBuffer, 0, totalReceivedBytes);
					totalReceivedBytes = 0;
					continue;
				}
			}
			else
			{
				// ���� �����÷ο� ó�� (�����Ͱ� �ʹ� Ŀ�� ���۸� �ʰ��ϴ� ���)
				GLogN("Buffer overflow, data too large to handle.\n");
				totalReceivedBytes = 0; // �ӽ� ���� �ʱ�ȭ
			}
		}
		else
		{
			GLogE( "Error... receive from Stream Buffer!!!(%d)\r\n", xReceivedBytes );
			xStreamBufferReset( hSBWebSocketRx );
			continue;
		}
	}
}
#endif	// PROTOCOL_WEBSOCKET_ENABLE

#if( PROTOCOL_MQTT_ENABLE )
/*----------------------------------------------------------------------
 * Function: CleanupParsingQueue
 * Description: Clean up parsing queue and free all allocated resources
 * Parameters: None
 * Return: None
 *--------------------------------------------------------------------*/
static void CleanupParsingQueue(void)
{
	osEvent evt;

	// Free all messages in parsing queue
	while(osMessageAvailableSpace(hParsingMsg) != MESSAGE_PARSING_QUEUE_SIZE)
	{
		evt = osMessageGet(hParsingMsg, 0);
		if (evt.status == osEventMessage)
		{
			stMsgClst *temp_msg = (stMsgClst *)evt.value.p;
			stCommPkt *temp_pkt = (stCommPkt *)temp_msg->pPacket;

			osPoolFree(hMqttPKPool, (void *)temp_pkt);
			osPoolFree(hMsgPool, (void *)temp_msg);
		}
		else
		{
			break;
		}
	}

	GLogN("Parsing queue cleaned\r\n");
}

void combineMqttThread( void const *argument )
{
	uint8_t		dummy[2], Header[6], oldSeqNum;
	stMsgClst	*msg;
	stCommPkt	*pkt;
    
    uint32_t	waitTime                = 0;
	uint32_t	count			        = 0;
	uint32_t	uiMutiFrameCopyIndex	= 0;
	uint32_t	uiCopyLen		        = 0;
	uint8_t		ucCalcChecksum		    = 0;
    
    
    size_t		totalReceivedBytes		= 0;
    size_t		xReceivedBytes			= 0;
    char		*StrStart				= NULL;
    char		*StrEnd					= NULL;
    
    size_t 		byte_array_len = 0;

    uint32_t	Total_length			= 0;
    uint32_t	Packet_Length			= 0;
    
    for (;;)
    {
        xReceivedBytes = xStreamBufferReceive(hSBMqttRx, (void *)g_ucSocketRxBuffer, sizeof(g_ucSocketRxBuffer), portMAX_DELAY);
        
        if (xReceivedBytes != 0)
        {
            if (totalReceivedBytes + xReceivedBytes < sizeof(g_ucTempBuffer))
            {
                memcpy(&g_ucTempBuffer[totalReceivedBytes], g_ucSocketRxBuffer, xReceivedBytes);
                totalReceivedBytes += xReceivedBytes;
				GLogN("!%d %d\r\n",totalReceivedBytes,xReceivedBytes);
                g_ucTempBuffer[totalReceivedBytes] = '\0';

                StrStart = (char *)g_ucTempBuffer;
				unsigned int uiOffset_SOF = 0;
				uiOffset_SOF = (int)StrStart;
				if(( StrStart = memstr(StrStart, totalReceivedBytes, "SOF", 3)) != NULL )
                {
					uiOffset_SOF=(int)StrStart - uiOffset_SOF;

					/* Length-based frame end detection (avoid false EOF in payload) */
					uint32_t remaining_after_sof = totalReceivedBytes - uiOffset_SOF;

					/* Need at least "SOF"(3) + TotalLen(2) to read length */
					if(remaining_after_sof < 5)
					{
						continue;
					}

					Total_length = (uint8_t)StrStart[3] + ((uint8_t)StrStart[4] << 8);

					/* frame_size = "SOF"(3) + TotLen_field(2) + TotalLen(includes CS, excludes EOF) + "EOF"(3) */
					{
						uint32_t frame_size = 3 + 2 + Total_length + 3;

						if(remaining_after_sof < frame_size)
						{
							/* Frame not fully received yet, wait for more data */
							continue;
						}

						/* Verify "EOF" at calculated position */
						StrEnd = StrStart + frame_size;
						if(memcmp(StrStart + frame_size - 3, "EOF", 3) != 0)
						{
							GLogE("Error... EOF mismatch at expected pos (len=%d)\r\n", Total_length);
							memset(g_ucTempBuffer, 0, totalReceivedBytes);
							totalReceivedBytes = 0;
							continue;
						}
					}

                    {
						uint32_t offset = 5; /* "SOF"(3) + TotalLen(2) already consumed */

                        g_cdp_UUID_length = StrStart[offset] + (StrStart[offset + 1] << 8);
                        offset += 2;
                        
						memcpy(g_cdp_UUID, &StrStart[offset], g_cdp_UUID_length);
						offset += g_cdp_UUID_length;
                        
                        g_packet_count = StrStart[offset] + (StrStart[offset + 1] << 8);
                        offset += 2;
						g_temp = 0;

						g_msg_processing_state = MSG_STATE_PROCESSING;

						bool should_abort = false;

                        for(int i = 0; i < g_packet_count; i++)
                        {
							if (g_msg_processing_state == MSG_STATE_ABORT_REQUESTED)
							{
								GLogN("Abort: External request\r\n");
								should_abort = true;
								break;
							}
							
							size_t available_bytes = xStreamBufferBytesAvailable(hSBMqttRx);
							if (available_bytes >= NEW_MESSAGE_THRESHOLD_BYTES)
							{
								GLogN("Abort: New message detected (buf:%d bytes)\r\n", available_bytes);
								should_abort = true;
								break;
							}

                          	g_git_UUID_length = StrStart[offset] + (StrStart[offset + 1] << 8);
                            offset += 2;
							
							memcpy(g_git_UUID, &StrStart[offset], g_git_UUID_length);
						  	offset += g_git_UUID_length;
							
						  	Packet_Length = StrStart[offset] + (StrStart[offset + 1] << 8);
                            offset += 2;
							
                            memcpy(g_ucByteArray, &StrStart[offset], Packet_Length);
                            
                            if( g_ucByteArray[0] == GITPACKET_SOF )
                            {
                                ucCalcChecksum += CalcChecksumGITPtclFromQueue(&g_ucByteArray[1], 5);

                                uiCopyLen = (g_ucByteArray[1] & 0x00FF) + ((g_ucByteArray[2] << 8) & 0xFF00);
                                if ( uiMutiFrameCopyIndex == 0 )
                                {
                                    msg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
                                    if( msg == NULL )
                                    {
                                        continue;
                                    }
									if (osSemaphoreWait(hMqttPKPoolSemaphore, osWaitForever) != osOK)
									{
										GLogE("Packet pool semaphore wait failed\n");
										continue;
									}
                                    pkt	= ( stCommPkt* )osPoolCAlloc( hMqttPKPool );
                                    if( pkt == NULL )
                                    {
                                        osPoolFree( hMsgPool, (void *)msg );
                                        continue;
                                    }
                                    
                                    // Frame
                                    // length
                                    msg->mLen = g_ucByteArray[2];
                                    msg->mLen <<= 8;
                                    msg->mLen += g_ucByteArray[1];

                                    // Mode
                                    msg->mMod = g_ucByteArray[3];
                                    msg->mMod <<= 8;
                                    msg->mMod += g_ucByteArray[4];

                                    // Sequence
                                    msg->mSeq = g_ucByteArray[5];

                                    // Payload
                                    pkt->mLen = g_ucByteArray[7];
                                    pkt->mLen <<= 8;
                                    pkt->mLen += g_ucByteArray[6];

                                    
                                    pkt->mFuncID = g_ucByteArray[9];
                                    pkt->mFuncID <<= 8;
                                    pkt->mFuncID += g_ucByteArray[8];

                                    // current frame
                                    pkt->mCurFrame = g_ucByteArray[11];
                                    pkt->mCurFrame <<= 8;
                                    pkt->mCurFrame += g_ucByteArray[10];
									
									// cdp uuid
									memcpy(&(pkt->UUID.cdpUUID), &(g_cdp_UUID), g_cdp_UUID_length);
									pkt->UUID.cdpUUIDLen = g_cdp_UUID_length;
									
									// git_uuid
									memcpy(&(pkt->UUID.gitUUID), &(g_git_UUID), g_git_UUID_length);
									pkt->UUID.gitUUIDLen = g_git_UUID_length;
									
									// git_pendingflag 
									pkt->UUID.pendingflag = false;
									
                                    // Payload Checksum
                                    pkt->mCS = g_ucByteArray[13];
                                    pkt->mCS <<= 8;
                                    pkt->mCS += g_ucByteArray[12];
                                    uiCopyLen -= (5 + GITPACKET_PAYLOAD_HEADER_SIZE);
                                }
                                else
                                {
                                    // Frame
                                    // length
                                    msg->mLen = g_ucByteArray[2];
                                    msg->mLen <<= 8;
                                    msg->mLen += g_ucByteArray[1];

                                    // Mode
                                    msg->mMod = g_ucByteArray[3];
                                    msg->mMod <<= 8;
                                    msg->mMod += g_ucByteArray[4];

                                    // Sequence
                                    msg->mSeq = g_ucByteArray[5];
                                    uiCopyLen -= 5 ;
                                }
                                // Checksum
                                if(uiCopyLen != 0)
                                {
                                    memcpy(&pkt->mData[uiMutiFrameCopyIndex], &g_ucByteArray[14], uiCopyLen);
                                    ucCalcChecksum = CalcChecksumGITPtclFromQueue(&g_ucByteArray[1], uiCopyLen + 5 + GITPACKET_PAYLOAD_HEADER_SIZE + 1);
                                }
                                else{
                                    ucCalcChecksum = CalcChecksumGITPtclFromQueue(&g_ucByteArray[1], 5 + GITPACKET_PAYLOAD_HEADER_SIZE + 1);
                                }
                                
                                if(g_ucByteArray[3] == 0x00)
                                {
                                    uiMutiFrameCopyIndex += uiCopyLen;
                                }
                                else
                                {
                                    uiMutiFrameCopyIndex = 0;
                                }
                                
                                // Get EOF

                                //ucCalcChecksum += g_ucByteArray[uiCopyLen + (5 + GITPACKET_PAYLOAD_HEADER_SIZE) + 1];

                                msg->mCS = g_ucByteArray[uiCopyLen + (5 + GITPACKET_PAYLOAD_HEADER_SIZE) + 2];
                                
                                if ( msg->mCS != ucCalcChecksum )
                                {
                                    GLogE( "Error... Checksum diffW  %02X %02X\r\n", msg->mCS, ucCalcChecksum);

                                    if(pkt->mFuncID==0x1202)
                                    {
                                        GLogE( "0x1202 cs diff return \r\n");
                                        g_bCsDiffFlag=1;
                                        xStreamBufferReset( hSBMqttRx );
                                    }
                                    else //MONI 20230307 [suresoft]106) : block this code for misraif(pkt->mFuncID!=0x1202)
                                    {
                                        osPoolFree( hMsgPool, (void *)msg );
                                        osPoolFree( hMqttPKPool, (void *)pkt );
                                        uiMutiFrameCopyIndex = 0;
										offset += Packet_Length;
                                        continue;
                                    }
                                }

                                msg->mMsgType	= MSG_COMM;
                                msg->mPktType	= PACKET_MQTT;
                                msg->pPacket	= (void*)pkt;

                                if((msg->mMod&0xFF00) == 0x8000)
                                {
                                    if(osMessageAvailableSpace(hParsingMsg) == 0)
                                    {
										osPoolFree( hMqttPKPool, (void *)pkt );
                                        osPoolFree( hMsgPool, (void *)msg );
                                    }
                                    else
                                    {
                                        osMessagePut( hParsingMsg, (uint32_t)msg, osWaitForever );
                                    }
                                }
                                else if((msg->mMod&0xFF00) == 0x4000)
                                {
                                    if ( oldSeqNum == msg->mSeq  )
                                    {
                                        dummy[0] = GITPACKET_SEND_ACK;
                                        MQTTPacketSend((uint8_t *)dummy, 1);
                                    }

                                    osPoolFree( hMqttPKPool, (void *)pkt );
                                    osPoolFree( hMsgPool, (void *)msg );
                                }
                                else
                                {
									osPoolFree( hMqttPKPool, (void *)pkt );
                                    osPoolFree( hMsgPool, (void *)msg );
                                }
                                oldSeqNum = msg->mSeq;
                            }
                            
                            offset += Packet_Length;
                        }

						if (should_abort)
						{
							// Aborted case
							GLogN("Message processing aborted\r\n");

							// Clean up parsing queue
							CleanupParsingQueue();

							// Reset buffers
							memset(g_ucTempBuffer, 0, sizeof(g_ucTempBuffer));
							totalReceivedBytes = 0;
							uiMutiFrameCopyIndex = 0;

							// Reset state
							g_msg_processing_state = MSG_STATE_IDLE;
						}
						else
						{
							// Normal completion
							g_msg_processing_state = MSG_STATE_IDLE;
							GLogN("Fin MQTT Message\r\n");
							memset(g_ucTempBuffer, 0, sizeof(g_ucTempBuffer));
							totalReceivedBytes = 0;
							uiMutiFrameCopyIndex = 0;
						}
                    }
                }
				else
				{
					memset(g_ucTempBuffer, 0, totalReceivedBytes);
					totalReceivedBytes = 0;
					continue;
				}
            }
            else
            {
                printf("Buffer overflow, data too large to handle.\n");
                totalReceivedBytes = 0;
                uiMutiFrameCopyIndex = 0;
            }
        }
        else
        {
            GLogE( "Error... receive from Stream Buffer!!!(%d)\r\n", xReceivedBytes );
			xStreamBufferReset( hSBMqttRx );
			continue;
        }
    }
}
#endif	// PROTOCOL_MQTT_ENABLE

/*----------------------------------------------------------------------
 *   USB/BT block whitelist
 *--------------------------------------------------------------------*/
static const uint16_t g_ausUsbBtWhitelist[] =
{
	0x1301U,	/* FL_Selftest_Factory (#050 unblock) */
	0x1701U,	/* FL_WifiScan                        */
	0x1702U,	/* FL_WifiConnect                     */
	0x1703U,	/* FL_WifiDisconnect                  */
	0x1704U,	/* FL_WifiStatus                      */
	0x1707U,	/* FL_WifiInfo                        */
	0x1803U,	/* FL_GitDevice_info                  */
	0xE037U,
	0xE036U,
	0x1238U,
	0x1303U,
	0x0145U,
	0x0146U,
	0x1239U,
	0x123AU,
	0x123BU,
	0x121CU,
	0x1220U,
};
static const uint32_t g_uiUsbBtWhitelistCnt =
	sizeof(g_ausUsbBtWhitelist) / sizeof(g_ausUsbBtWhitelist[0]);

static uint8_t IsUsbBtWhitelisted(uint16_t u16FuncID)
{
	uint32_t i;

	for (i = 0U; i < g_uiUsbBtWhitelistCnt; i++)
	{
		if (u16FuncID == g_ausUsbBtWhitelist[i])
		{
			return 1U;
		}
	}
	return 0U;
}

/* Post-decrypt plausibility check: catches silent AES decrypt corruption
 * (shared HW-CRC race) that yields a valid length but garbage plaintext, which
 * the length-only retry guard cannot detect. Returns false when the payload
 * must be re-decrypted/dropped. Only FuncID 0x1000 (PassThruConnect) has a
 * layout we can validate: PID(4)+flags(4)+RTC BCD date (year/mon/day at +8/+9/
 * +10, see Set_RTCData). The server always sends a valid timestamp regardless
 * of PID support, so an out-of-range date == corruption (this distinguishes
 * corruption from a genuinely-unsupported-but-valid PID). Unknown FuncIDs pass
 * (no known layout to validate). */
static bool IsDecryptPayloadPlausible(uint16_t funcID, const uint8_t *data, uint32_t plainLen)
{
	if(funcID == 0x1000 && plainLen >= 11)
	{
		uint8_t year  = HexToDec(data[8]);
		uint8_t month = HexToDec(data[9]);
		uint8_t day   = HexToDec(data[10]);
		if(month < 1  || month > 12) return false;
		if(day   < 1  || day   > 31) return false;
		if(year  < 20 || year  > 99) return false;   /* 2020..2099 */
	}
	return true;
}

void parsingThread( void const *argument )
{
	osEvent		evt;

	stMsgClst	*msg;
	stCommPkt	*pkt;

	ePKT_TD		type;

	uint32_t	i;
#ifdef VCI3_RECORD
	uint32_t	j;
#endif
#if ENCRYPT_PRJ
	uint32_t OutputMessageLength;
	U16 u16PlainDataSize=0;
	U8 FunctionIDHigh;
#endif

	for(;;)
	{
		evt		= osMessageGet( hParsingMsg, osWaitForever );
		if( evt.status == osEventMessage)
		{
			msg 	= ( stMsgClst * )evt.value.p;
			type	= msg->mPktType;

			switch( type )
			{
				case PACKET_UART :
				case PACKET_USB :
				case PACKET_WIFI :
				{
					pkt		= ( stCommPkt * )msg->pPacket;
		#if ENCRYPT_PRJ
							FunctionIDHigh=(pkt->mFuncID>>8);
							u16PlainDataSize=(pkt->mCurFrame);
							if((g_bEncryptFlag==ON)&&(pkt->mLen>8)
								&&(FunctionIDHigh!=0xD0)&&(FunctionIDHigh!=0xC0)//except wlan, trigger 
								&&(u16PlainDataSize>0))
								//&&(u16PlainDataSize==0))
							{
								if(g_bEncryptLogOnFlag == true)
								{
									GLogI( "\r\nDecrypt(%d) : ", u16PlainDataSize);
								}

								{
									uint32_t encLen = pkt->mLen - DATA_HEADER_SIZE;
									static uint8_t ucDecryptBackup[4200];
									uint8_t retryCount = 0;
									memcpy(ucDecryptBackup, pkt->mData, encLen);
									do {
										if(retryCount > 0) {
											memcpy(pkt->mData, ucDecryptBackup, encLen);
											osDelay(5);
											GLogE("Decrypt retry %d FuncID 0x%04X\r\n", retryCount, pkt->mFuncID);
										}
										{
											int32_t decStatus = getAESDecoding_ECB(pkt->mData,
																encLen,
																g_ucAES256_Key,
																AES256,
																pkt->mData,
																&OutputMessageLength);
											/* Log only on actual retries (try>0) to avoid flooding on every normal decrypt */
											if(retryCount > 0)
											{
												GLogN("AES dec: status=0x%08X outLen=%d FuncID=0x%04X try=%d\r\n",
													(uint32_t)decStatus, OutputMessageLength, pkt->mFuncID, retryCount);
											}
										}
										retryCount++;
									} while((OutputMessageLength == 0 || !IsDecryptPayloadPlausible(pkt->mFuncID, pkt->mData, u16PlainDataSize)) && retryCount <= 3);

									if(OutputMessageLength == 0 || !IsDecryptPayloadPlausible(pkt->mFuncID, pkt->mData, u16PlainDataSize)) {
										GLogE("Decrypt fail(drop) FuncID 0x%04X\r\n", pkt->mFuncID);
										osPoolFree(hCommPKPool, (void *)pkt);
										osPoolFree(hMsgPool, (void *)msg);
										break;
									}
								}

								pkt->mLen = (u16PlainDataSize+DATA_HEADER_SIZE);//update length
								if(g_bEncryptLogOnFlag == true)
								{
									for(i=0; i<u16PlainDataSize; i++)
									{
										GLogI("%02X ",pkt->mData[i]);
									}
									GLogI("\r\n");
								}
							}
		#endif
							/* USB/BT block filter */
							if ((g_bUsbBtBlocked != 0U)
								&& (type == PACKET_USB || type == PACKET_UART)
								&& (IsUsbBtWhitelisted(pkt->mFuncID) == 0U))
							{
								GLogN("Blocked(0x%04X)\r\n", pkt->mFuncID);
								osPoolFree(hCommPKPool, (void *)pkt);
								osPoolFree(hMsgPool, (void *)msg);
								break;
							}

							if( pkt->mFuncID == 0x3002 && g_b3002Lock == true ){}
							else
							{
								g_b3002Lock = false;
								//GLogI( "Rcv PacketTpye( %d )!!! \r\n", msg->mPktType );

								for( i = 0; i < guiFuncCnt; i++ )
								{
									if( ( pkt->mFuncID == gsFunctions[i].uiFunctionID ) && ( gsFunctions[i].fnPayloadCB != NULL ) )
									{
										LED_SetState(eLED_DIAG_COMM, 1000, 100);
										if(pkt->mFuncID!=0x0255) GLogN( "Fin(%04X)\r\n", pkt->mFuncID);
										gsFunctions[i].fnPayloadCB( pkt, type );
										break;
									}
								}

		#ifdef VCI3_RECORD
								for( j = 0; j < guiTriggerFuncCnt; j++)
								{
									if( ( pkt->mFuncID == gsTriggerFunctions[j].uiFunctionID ) && ( gsTriggerFunctions[j].fnPayloadCB != NULL ) )
									{
										gsTriggerFunctions[j].fnPayloadCB( pkt, type );
										break;
									}
								}
		#endif

								if( i >= guiFuncCnt )
								{
		//							unsigned char ucData[10]={0,};
		//							unsigned short usLength=0;
		//							for(int i=0;i<10;i++)
		//							{
		//								ucData[i]=0x00;
		//							}
									//TransmitFunction(type, ucData, 10, pkt->mFuncID);
		#ifdef VCI3_RECORD
									if(j >= guiTriggerFuncCnt )
		#endif
										FL_NotSupport( pkt->mFuncID );
								}
							}
							osPoolFree( hCommPKPool, (void *)pkt );
							osPoolFree( hMsgPool, (void *)msg );
							break;
						}
						case PACKET_MQTT :
						{
							pkt		= ( stCommPkt * )msg->pPacket;
		#if ENCRYPT_PRJ
							FunctionIDHigh=(pkt->mFuncID>>8);
							u16PlainDataSize=(pkt->mCurFrame);
							if((g_bEncryptFlag==ON)&&(pkt->mLen>8)
								&&(FunctionIDHigh!=0xD0)&&(FunctionIDHigh!=0xC0)//except wlan, trigger 
								&&(u16PlainDataSize>0))
								//&&(u16PlainDataSize==0))
							{
								if(g_bEncryptLogOnFlag == true)
								{
									GLogI( "\r\nDecrypt(rx) : ");
								}

								{
									uint32_t encLen = pkt->mLen - DATA_HEADER_SIZE;
									static uint8_t ucDecryptBackup[4200];
									uint8_t retryCount = 0;
									memcpy(ucDecryptBackup, pkt->mData, encLen);
									do {
										if(retryCount > 0) {
											memcpy(pkt->mData, ucDecryptBackup, encLen);
											osDelay(5);
											GLogE("Decrypt retry %d FuncID 0x%04X\r\n", retryCount, pkt->mFuncID);
										}
										{
											int32_t decStatus = getAESDecoding_ECB(pkt->mData,
																encLen,
																g_ucAES256_Key,
																AES256,
																pkt->mData,
																&OutputMessageLength);
											/* Log only on actual retries (try>0) to avoid flooding on every normal decrypt */
											if(retryCount > 0)
											{
												GLogN("AES dec: status=0x%08X outLen=%d FuncID=0x%04X try=%d\r\n",
													(uint32_t)decStatus, OutputMessageLength, pkt->mFuncID, retryCount);
											}
										}
										retryCount++;
									} while((OutputMessageLength == 0 || !IsDecryptPayloadPlausible(pkt->mFuncID, pkt->mData, u16PlainDataSize)) && retryCount <= 3);

									if(OutputMessageLength == 0 || !IsDecryptPayloadPlausible(pkt->mFuncID, pkt->mData, u16PlainDataSize)) {
										GLogE("Decrypt fail(drop) FuncID 0x%04X\r\n", pkt->mFuncID);
										osPoolFree(hCommPKPool, (void *)pkt);
										osPoolFree(hMsgPool, (void *)msg);
										break;
									}
								}

								pkt->mLen = (u16PlainDataSize+DATA_HEADER_SIZE);//update length
								if(g_bEncryptLogOnFlag == true)
								{
									for(i=0; i<u16PlainDataSize; i++)
									{
										GLogI("%02X ",pkt->mData[i]);
									}
									GLogI("\r\n");
								}

							}
		#endif
							if( pkt->mFuncID == 0x3002 && g_b3002Lock == true ){}
							else					
							{
								g_b3002Lock = false;
								//GLogI( "Rcv PacketTpye( %d )!!! \r\n", msg->mPktType );
							
								for( i = 0; i < guiFuncCnt; i++ )
								{
									if( ( pkt->mFuncID == gsFunctions[i].uiFunctionID ) && ( gsFunctions[i].fnPayloadCB != NULL ) )
									{
										LED_SetState(eLED_DIAG_COMM, 1000, 100);
										osSemaphoreWait(hpairflagSemaphore, osWaitForever);
										if(pkt->mFuncID!=0x0255) GLogN( "Fin(%04X)\r\n", pkt->mFuncID);
										gsFunctions[i].fnPayloadCB( pkt, type);
										break;
									}
								}

		#ifdef VCI3_RECORD
								for( j = 0; j < guiTriggerFuncCnt; j++)
								{
									if( ( pkt->mFuncID == gsTriggerFunctions[j].uiFunctionID ) && ( gsTriggerFunctions[j].fnPayloadCB != NULL ) )
									{
										gsTriggerFunctions[j].fnPayloadCB( pkt, type );
										break;
									}
								}
		#endif

								if( i >= guiFuncCnt )
								{
		//							unsigned char ucData[10]={0,};
		//							unsigned short usLength=0;
		//							for(int i=0;i<10;i++)
		//							{
		//								ucData[i]=0x00;
		//							}
									//TransmitFunction(type, ucData, 10, pkt->mFuncID);
		#ifdef VCI3_RECORD
									if(j >= guiTriggerFuncCnt )
		#endif
										FL_NotSupport( pkt->mFuncID );
								}
							}
							osPoolFree( hMqttPKPool, (void *)pkt );
							osPoolFree( hMsgPool, (void *)msg );
							break;
						}
						case PACKET_WEBSOCKET :
						{
							pkt		= ( stCommPkt * )msg->pPacket;
		#if ENCRYPT_PRJ
							FunctionIDHigh=(pkt->mFuncID>>8);
							u16PlainDataSize=(pkt->mCurFrame);
							if((g_bEncryptFlag==ON)&&(pkt->mLen>8)
								&&(FunctionIDHigh!=0xD0)&&(FunctionIDHigh!=0xC0)//except wlan, trigger 
								&&(u16PlainDataSize>0))
								//&&(u16PlainDataSize==0))
							{
		#if 1
								GLogI( "\r\nDecrypt : ");
		#endif

								{
									uint32_t encLen = pkt->mLen - DATA_HEADER_SIZE;
									static uint8_t ucDecryptBackup[4200];
									uint8_t retryCount = 0;
									memcpy(ucDecryptBackup, pkt->mData, encLen);
									do {
										if(retryCount > 0) {
											memcpy(pkt->mData, ucDecryptBackup, encLen);
											osDelay(5);
											GLogE("Decrypt retry %d FuncID 0x%04X\r\n", retryCount, pkt->mFuncID);
										}
										{
											int32_t decStatus = getAESDecoding_ECB(pkt->mData,
																encLen,
																g_ucAES256_Key,
																AES256,
																pkt->mData,
																&OutputMessageLength);
											/* Log only on actual retries (try>0) to avoid flooding on every normal decrypt */
											if(retryCount > 0)
											{
												GLogN("AES dec: status=0x%08X outLen=%d FuncID=0x%04X try=%d\r\n",
													(uint32_t)decStatus, OutputMessageLength, pkt->mFuncID, retryCount);
											}
										}
										retryCount++;
									} while((OutputMessageLength == 0 || !IsDecryptPayloadPlausible(pkt->mFuncID, pkt->mData, u16PlainDataSize)) && retryCount <= 3);

									if(OutputMessageLength == 0 || !IsDecryptPayloadPlausible(pkt->mFuncID, pkt->mData, u16PlainDataSize)) {
										GLogE("Decrypt fail(drop) FuncID 0x%04X\r\n", pkt->mFuncID);
										osPoolFree(hCommPKPool, (void *)pkt);
										osPoolFree(hMsgPool, (void *)msg);
										break;
									}
								}

								pkt->mLen = (u16PlainDataSize+DATA_HEADER_SIZE);//update length
		#if 1
								for(i=0; i<u16PlainDataSize; i++)
								{
									GLogI("%02X ",pkt->mData[i]);
								}
								GLogI("\r\n");
		#endif

							}
		#endif
							if( pkt->mFuncID == 0x3002 && g_b3002Lock == true ){}
							else					
							{
								g_b3002Lock = false;
								//GLogI( "Rcv PacketTpye( %d )!!! \r\n", msg->mPktType );
							
								for( i = 0; i < guiFuncCnt; i++ )
								{
									if( ( pkt->mFuncID == gsFunctions[i].uiFunctionID ) && ( gsFunctions[i].fnPayloadCB != NULL ) )
									{
										LED_SetState(eLED_DIAG_COMM, 1000, 100);
										osSemaphoreWait(hpairflagSemaphore, osWaitForever);
										if(pkt->mFuncID!=0x0255) GLogN( "Fin(%04X)\r\n", pkt->mFuncID);
										gsFunctions[i].fnPayloadCB( pkt, type );
										break;
									}
								}

		#ifdef VCI3_RECORD
								for( j = 0; j < guiTriggerFuncCnt; j++)
								{
									if( ( pkt->mFuncID == gsTriggerFunctions[j].uiFunctionID ) && ( gsTriggerFunctions[j].fnPayloadCB != NULL ) )
									{
										gsTriggerFunctions[j].fnPayloadCB( pkt, type );
										break;
									}
								}
		#endif

								if( i >= guiFuncCnt )
								{
		//							unsigned char ucData[10]={0,};
		//							unsigned short usLength=0;
		//							for(int i=0;i<10;i++)
		//							{
		//								ucData[i]=0x00;
		//							}
									//TransmitFunction(type, ucData, 10, pkt->mFuncID);
		#ifdef VCI3_RECORD
									if(j >= guiTriggerFuncCnt )
		#endif
										FL_NotSupport( pkt->mFuncID );
								}
							}
							osPoolFree( hMqttPKPool, (void *)pkt );
							osPoolFree( hMsgPool, (void *)msg );
							break;
						}

				default :
				{
					GLogE( "unknown PacketTpye( %d )!!! \r\n", msg->mPktType );
					break;
				}
			}
		}
		
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#else
        //osDelay( 1 );
#endif
	}
}
