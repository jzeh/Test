 /*************************************************************
  * NOTE : git_hsm.c
  * 	 HSM control
  * Author : Lee junho
  * Since : 2019.10.29
 **************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "gpio.h"
#include "cmsis_os.h"
 
#include "main.h"
#include "usart.h"
 
#include "common.h"
#include "typedef.h"
#include "git_ioctl.h"
#include "git_function_list.h"
#include "crypto.h"
 
#include "git_hsm.h"
#include "git_fsutil.h"

#include "cmox_init.h"
#include "cmox_low_level.h"
#include "cmox_crypto.h"
#include "sw_timer.h"
#include "usbd_def.h"
#include "led_control.h"
#include "led.h"
#include "git_rs9116.h"
#include "crc.h"
#include "git_global.h"
 /*----------------------------------------------------------------------
  *   Define
  *--------------------------------------------------------------------*/
#define HSM_UART_PORT										huart6
#define HSM_STREAM_BUFFER_SIZE								1000 //256
#define AES128_KEY_SIZE										16
#define AES256_KEY_SIZE										32
#define AES_IV_SIZE											16
 
#define HSM_OSC_CLK											8000000					// 3.686Mhz
 
#define ISO7816_TIMEOUT										1000
#define ISO7816_BUFFER_SIZE									256
#define ISO7816_PPS											16				/* 16cycle */
 
#define	ISO7816_BAUDRATE									HSM_OSC_CLK / 372
#define	ISO7816_BAUDRATE2									HSM_OSC_CLK / ISO7816_PPS		// 515625
#define	ISO7816_BAUDRATE3									HSM_OSC_CLK / 32
#define	ISO7816_BAUDRATE4									HSM_OSC_CLK / 16

 
#define MAX_NUM_RETRY										32
 
//#define	DBG_MSG

//#define HSM_ATR_LEN											34

 /*----------------------------------------------------------------------
  *   Functions declaration
  *--------------------------------------------------------------------*/
 int32_t InitHSMUart( uint32_t baudrate );
 
 int32_t se_open( void );
 int32_t se_tranreceive( uint8_t *apdu, uint32_t apdulen, uint8_t *resp, uint32_t *resplen );
 int32_t se_pps( void );
 int32_t VCI3_HSM_UPDATE( uint8_t *AuthKey );//tara critical function delete
 int32_t HSM_ASK_Applet_Update( uint8_t *AuthKey, bool UpdateObj, uint8_t ASKType );
 int16_t HSM_Version_Check( int *version );
 void HSM_Update_Ack( bool state, int progress );
 void DeleteHSMFile( void );
 void DeleteHSMFile_AT( uint8_t FwType );
 void Save_HSM_Status( void );
 void Save_HSM_Error_Status( U32 ErrorCode );
 uint32_t Read_HSM_Status( void );
 void Print_HSM_Update_Result( U32 result );
 int32_t UpdateHSMASK( uint8_t ucASK_Num );
 void Save_HSM_UpdateFailCount( void );
 uint8_t Check_HSM_UpdateFailCount( void );
 extern void TransmitFunction_IT( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID );
 extern void TransmitFunction( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID );
 extern void TransmitFunction_WithUUID( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID, const UUID_Struct *pUUID );
 
 static uint8_t *get_pps_cmd( void );
 
 /*static*/ uint32_t read_bytes( uint8_t *buf, uint32_t len );
 uint32_t read_bytes_atr( uint8_t *buf, uint32_t len );
 
 static uint32_t write_bytes( uint8_t *buf, uint32_t len );
 
 //static int32_t RSA_Encrypt(RSApubKey_stt *P_pPubKey, const uint8_t *P_pInputMessage,  int32_t P_InputSize, uint8_t *P_pOutput);
 //static int32_t STM32_AES_ECB_Encrypt(uint8_t* InputMessage, uint32_t InputMessageLength, uint8_t  *AES256_Key, uint8_t  *OutputMessage, uint32_t *OutputMessageLength);
 
static int32_t STM32_AES_ECB_Encrypt(uint8_t* InputMessage, uint32_t InputMessageLength, uint8_t  *AES256_Key, uint8_t AES_Type, uint8_t  *OutputMessage, uint32_t *OutputMessageLength);
static int32_t STM32_AES_ECB_Decrypt(uint8_t* InputMessage, uint32_t InputMessageLength, uint8_t  *AES256_Key, uint8_t AES_Type, uint8_t  *OutputMessage, uint32_t *OutputMessageLength);
static int32_t STM32_AES_CTR_Encrypt(uint8_t* InputMessage, uint32_t InputMessageLength, uint8_t  *AES256_Key, uint8_t  *iv, uint8_t  *OutputMessage, uint32_t *OutputMessageLength);
static int32_t STM32_AES_CBC_Encrypt(uint8_t* InputMessage, uint32_t InputMessageLength, uint8_t  *AES256_Key, uint8_t  *OutputMessage, uint32_t *OutputMessageLength);
static int32_t STM32_TDES_ECB_Encrypt(uint8_t* InputMessage, uint32_t InputMessageLength, uint8_t  *TDES_Key, uint8_t  *OutputMessage, uint32_t *OutputMessageLength);
static int32_t STM32_TDES_CBC_Encrypt(uint8_t*  InputMessage, uint32_t  InputMessageLength, uint8_t  *DES_Key, uint8_t  *InitializationVector, uint32_t  IvLength, uint8_t  *OutputMessage, uint32_t *OutputMessageLength);
static int32_t STM32_DES_ECB_Encrypt(uint8_t* InputMessage, uint32_t InputMessageLength, uint8_t  *TDES_Key, uint8_t  *OutputMessage, uint32_t *OutputMessageLength);
static int32_t STM32_DES_ECB_Decrypt(uint8_t* InputMessage, uint32_t InputMessageLength, uint8_t  *TDES_Key, uint8_t  *OutputMessage, uint32_t *OutputMessageLength);
 
 /*----------------------------------------------------------------------
  *   Variables
  *--------------------------------------------------------------------*/
 StreamBufferHandle_t hSBHsmRx;
 
 uint8_t	 gucHSMRxDummy;

 extern uint8_t g_HSM_update_flag;
 extern int g_iUSBConnected;
 extern uint8_t g_ucBtConnected;
 uint8_t g_HSM_ATR_Flag = 0;
 U8 g_HSM_Timer = 0;
 SHSMUpdateAck *pHSMAck;
 
//uint8_t Working_Buffer[150]; //for RSA, Sign
//uint8_t Computed_Signature[100];
uint8_t Working_Buffer[3000]; //for RSA-2048, Sign (increased from 1500)
uint8_t Computed_Signature[1000];

U8 gHSM_Info_Name[18] = {0x00, };

extern uint8_t DK_ENC[24];
extern uint8_t DK_MAC[24];
extern uint8_t DK_DEK[24];

extern uint8_t g_ucAES256_Key[32];
 
 static st_PPSReq pps[] =
 {
	 {
		 .cycles_per_etu = 8,
		 .cmd = { 0xff, 0x10, 0x97, 0x78 },
	 },
	 {
		 .cycles_per_etu = 16,
		 .cmd = { 0xff, 0x10, 0x96, 0x79 },
	 },
	 {
		 .cycles_per_etu = 32,
		 .cmd = { 0xff, 0x10, 0x95, 0x7a },
	 },
 };
 
 //static const uint8_t ATR[16] = { 0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63, 0x72, 0x65, 0x74, 0x53, 0x45, 0x32, 0x30, 0x31 };
// static const uint8_t ATR[] = { 0x3B, 0x6B, 0x00, 0x00, 0x43, 0x4F, 0x4D, 0x45, 0x54, 0x35, 0x30, 0x34, 0x56, 0x31, 0x30 };
//Samsung Chip ATR
static const uint8_t ATR_Samsung[16] = { 0x3B, 0x7B, 0x18, 0x00, 0x00, 0x53, 0x79, 0x63, 0x72, 0x65, 0x74, 0x53, 0x45, 0x32, 0x30, 0x31 }; 
//Coregate Chip ATR
static const uint8_t ATR_Coregate[15] = { 0x3B, 0x6B, 0x00, 0x00, 0x43, 0x4F, 0x4D, 0x45, 0x54, 0x35, 0x30, 0x34, 0x56, 0x31, 0x30 };
static const uint8_t ATR_Coregate_NonOS_V1[16] = {0x3B, 0x1D, 0x96, 0x00, 0x31, 0x08, 0x47, 0x48, 0x01, 0x71, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00};
static const uint8_t ATR_Coregate_NonOS_V2[16] = {0x3B, 0x1D, 0x96, 0x00, 0x31, 0x08, 0xC3, 0xEB, 0x90, 0x27, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00}; 


 /*----------------------------------------------------------------------
  *   Functions definition
  *--------------------------------------------------------------------*/
 /*----------------------------------------------------------------------
  *   Common
  *--------------------------------------------------------------------*/
 int8_t InitHSM( void )
 {
	 EnableHSM();
 
	 // create streamBuffer
	 hSBHsmRx = xStreamBufferCreate( HSM_STREAM_BUFFER_SIZE, 1 );
	 if( hSBHsmRx == NULL )
	 {
		 return INIT_FAIL;
	 }
 
	 return INIT_OK;
 }
 
 void DeinitHSM( void )
 {
	 DisableHSM();
 }
 
 void ResetHSM( uint8_t rst )
 {
#if defined( DBG_MSG )
	 GLogN( ">>>>>>>>>>>>>>>>> (%s)\r\n", __FUNCTION__ );
#endif
	 if( rst )
	 {
		 IO_CONTROL_HIGH( HSM_RST );
		 HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );
	 }
	 else
	 {
		 IO_CONTROL_LOW( HSM_RST );
	 }
 }
void ColdResetHSM( uint8_t rst )
{
#if defined( DBG_MSG )
	GLogN( ">>>>>>>>>>>>>>>>> (%s)\r\n", __FUNCTION__ );
#endif
	IO_CONTROL_LOW( HSM_PWR_EN );
	osDelay( 10 );
	IO_CONTROL_HIGH( HSM_PWR_EN );
	osDelay( 10 );
	if( rst )
	{
		IO_CONTROL_HIGH( HSM_RST );
		HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );
	}
	else
	{
		IO_CONTROL_LOW( HSM_RST );
	}
}
 int32_t InitHSMUart( uint32_t baudrate )
 {
#if defined( DBG_MSG )
	 GLogN( "[%s] %d\r\n", __FUNCTION__, baudrate );
#endif
	 HSM_UART_PORT.Instance 					 = HSM_UART_PORT.Instance;//UART8;
	 HSM_UART_PORT.Init.BaudRate				 = baudrate;
	 HSM_UART_PORT.Init.WordLength				 = UART_WORDLENGTH_9B;
	 HSM_UART_PORT.Init.StopBits				 = UART_STOPBITS_2;
	 HSM_UART_PORT.Init.Parity					 = UART_PARITY_EVEN;
	 HSM_UART_PORT.Init.Mode					 = UART_MODE_TX_RX;
	 HSM_UART_PORT.Init.HwFlowCtl				 = UART_HWCONTROL_NONE;
	 HSM_UART_PORT.Init.OverSampling			 = UART_OVERSAMPLING_16;
	 HSM_UART_PORT.Init.OneBitSampling			 = UART_ONE_BIT_SAMPLE_DISABLE;
	 HSM_UART_PORT.Init.ClockPrescaler			 = UART_PRESCALER_DIV1;
	 HSM_UART_PORT.AdvancedInit.AdvFeatureInit	 = UART_ADVFEATURE_NO_INIT;
 
	 if( HAL_UART_Init( &HSM_UART_PORT ) != HAL_OK )			 return -1;
 
	 return 0;
 }
 
 void transmitHSMCommand( uint8_t *data, uint8_t len )
 {
	 // Uart Receive disable
	 HSM_UART_PORT.RxState = HAL_UART_STATE_READY;
 
	 for( uint8_t i = 0; i < len; i++ ) 		 HAL_UART_Transmit( &HSM_UART_PORT, (uint8_t *)&data[i], 1, 0xFFFF );
 
	 HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );
 }
 
 void clearHSMReceiveData( void )
 {
	 xStreamBufferReset( hSBHsmRx );
 }
 
 uint32_t getHSMReceiveDataSize( void )
 {
	 return (uint32_t)xStreamBufferBytesAvailable( hSBHsmRx );
 }
 
 uint32_t getHSMReceiveData( uint8_t *data, uint32_t len )
 {
	 return xStreamBufferReceive( hSBHsmRx, (void *)data, len, pdMS_TO_TICKS( 1000 ) );
 }
 
 /*----------------------------------------------------------------------
  *   ISO7816
  *--------------------------------------------------------------------*/
 int32_t se_open( void )
 {
	 int32_t ret	 = ISO7816_OK;
	 uint8_t atr[HSM_ATR_LEN] = {0};
	 int32_t szAtr	 = 0;
	 int32_t retry	 = 5;
     U8 i = 0;
#if defined( DBG_MSG )
	 GLogN( "[%s]\r\n", __FUNCTION__ );
#endif
 
	 while( retry-- )
	 {
		 // initialize Uart for HSM
		 ret = InitHSMUart( ISO7816_BAUDRATE );
		 if( ret != 0 )
		 {
			 GLogE( "Error... Fail init hsm uart(%d)!!!\r\n", ret );
			 return ISO7816_ERR;
		 }
 
		 clearHSMReceiveData();
		 HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );
        
         IO_CONTROL_HIGH( HSM_PWR_EN );
         osDelay( 10 );
		 // HSM Reset
		 IO_CONTROL_LOW( HSM_RST );
		 osDelay( 100 );
         //clearHSMReceiveData();
		 IO_CONTROL_HIGH( HSM_RST );
		 osDelay( 100 );
 
		 szAtr = read_bytes_atr( atr, HSM_ATR_LEN );
		 if( szAtr == 0 )
		 {
			 ret = ISO7816_ERR_ATR_MUTE;
		 }
		 else
		 {
		 	if(( !memcmp( ATR_Coregate_NonOS_V1, atr, 3/*HSM_ATR_LEN*/ ) ||
                  !memcmp( ATR_Coregate_NonOS_V2, atr, 3/*HSM_ATR_LEN*/ ) ||
                  !memcmp( ATR_Coregate_NonOS_V1, atr+1, 3/*HSM_ATR_LEN*/ ) ||
                  !memcmp( ATR_Coregate_NonOS_V2, atr+1, 3/*HSM_ATR_LEN*/ )) &&
                  g_HSM_ATR_Flag == 1) //Check ATR
		 	{
				g_HSM_update_flag = HSM_UPDATE_MODE;
		 	}
			 if(( !memcmp( ATR_Samsung, atr, 3/*HSM_ATR_LEN*/ ) || 
                  !memcmp( ATR_Coregate, atr, 3/*HSM_ATR_LEN*/ )  || 
                  !memcmp( ATR_Coregate_NonOS_V1, atr, 3/*HSM_ATR_LEN*/ ) ||
                  !memcmp( ATR_Coregate_NonOS_V2, atr, 3/*HSM_ATR_LEN*/ ) ||
                  !memcmp( ATR_Samsung, atr+1, 3/*HSM_ATR_LEN*/ ) || 
                  !memcmp( ATR_Coregate, atr+1, 3/*HSM_ATR_LEN*/ )  || 
                  !memcmp( ATR_Coregate_NonOS_V1, atr+1, 3/*HSM_ATR_LEN*/ ) ||
                  !memcmp( ATR_Coregate_NonOS_V2, atr+1, 3/*HSM_ATR_LEN*/ )) ||
                g_HSM_update_flag) //Check ATR
			{
				ret = ISO7816_OK;
                //GLogN(" <<<<<<<<<<<< SUCESS >>>>>>>>>>>> \n\r");
                //GLogN(" <<<<<<<< CLK : %d >>>>>>>>> \n\r", HSM_OSC_CLK);
                
                if (g_HSM_update_flag != HSM_UPDATE_MODE) 
                {
                  U8 req[4] = {0x00, };
                  //HSM Baudrate Change //9600 -> 115200
                  U8 baud115200[4] = {0xFF, 0x10, 0x18, 0xF7}; // 18, F7 : 115200 
                  //U8 baud9600[4] = {0xFF, 0x10, 0x11, 0xFE}; // 11, FE : 9600
                  //U8 baud38400[4] = {0xFF, 0x10, 0x13, 0xFC}; // 13, FC : 38400
                  U8 baud115200_HSM_UPDATE[4] = {0xFF, 0x10, 0x96, 0x79};
                  
                  U8 resp[6] = {0x00, };
                  
                  if(g_HSM_update_flag == HSM_SEND_HSM_DATA)
                  {
                    memcpy(req, baud115200_HSM_UPDATE, 4);
                    U8 count = 0;
                    
                    clearHSMReceiveData();
                    transmitHSMCommand(req, 4);
                    count = read_bytes( resp, 4 );
                    
                    for (i = 0; i < count; i++)
                    {
                      if(req[i]!=resp[i]) {break;}
                    }
                    
                    if(i==count)
                    {
                      //success
                      //GLogN( "HSM Update Baudrate Change : 115200\r\n" );
                      InitHSMUart( ISO7816_BAUDRATE4 );
                      HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );
                      break;
                    }
                    else
                    {
                      //fail
                      GLogN( "BaudrateChange fail!!\r\n" );
                    }
                    
                  }
                  
                  else
                  {
                    memcpy(req, baud115200, 4);
                    U8 count = 0;
                    
                    clearHSMReceiveData();
                    transmitHSMCommand(req, 4);
                    count = read_bytes( resp, 4 );
                    
                    for (i = 0; i < count; i++)
                    {
                      if(req[i]!=resp[i]) {break;}
                    }
                    
                    if(i==count)
                    {
                      //success
                      //GLogN( "HSM Baudrate Change : 115200\r\n" );
                      InitHSMUart( ISO7816_BAUDRATE3 );
                      HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );
                      break;
                    }
                    else
                    {
                      //fail
                      GLogN( "BaudrateChange fail!!\r\n" );
                    }
                  }
                }
                else {break;}
			}
			else
			{
                GLogN("\n\r --------------- ATR RETRY ---------------- \n\r"); //ATR Retry
			  	ret = ISO7816_ERR_ATR_WRONG;
			}
		 }
	 }
     
     if( retry <= 0 )
     {
        LED_SetState( eLED_HSM_ERROR, 1500 , 300);
     }
 
 //  se_pps();		 //?´ê±° ????��??ë©??hsm ??????�??????
 
	 return ret;
 }
 
 int32_t check_se_open( void )
 {
	 int32_t ret	 = ISO7816_OK;
	 uint8_t atr[16] = { 0, };
	 int32_t szAtr	 = 0;
	 int32_t retry	 = 2;
     uint32_t sz     = 0;
     uint32_t size   = 0;
 
	 while( retry-- )
	 {
		 // initialize Uart for HSM
		 ret = InitHSMUart( ISO7816_BAUDRATE );
		 if( ret != 0 )
		 {
			 GLogE( "Error... Fail init hsm uart(%d)!!!\r\n", ret );
			 return ISO7816_ERR;
		 }
 
		 clearHSMReceiveData();
		 HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );
 
		 IO_CONTROL_HIGH( HSM_PWR_EN );
         osDelay( 10 );
		 // HSM Reset
		 IO_CONTROL_LOW( HSM_RST );
		 osDelay( 100 );
         //clearHSMReceiveData();
		 IO_CONTROL_HIGH( HSM_RST );
		 osDelay( 100 );
 
		 szAtr = read_bytes_atr( atr, HSM_ATR_LEN );
         
		 if( szAtr == 0 )
		 {
			 ret = ISO7816_ERR_ATR_MUTE;
		 }
		 else
		 {
			 if(( !memcmp( ATR_Samsung, atr, 3/*HSM_ATR_LEN*/ ) || 
                  !memcmp( ATR_Coregate, atr, 3/*HSM_ATR_LEN*/ )  || 
                  !memcmp( ATR_Coregate_NonOS_V1, atr, 3/*HSM_ATR_LEN*/ ) ||
                  !memcmp( ATR_Coregate_NonOS_V2, atr, 3/*HSM_ATR_LEN*/ ) ||
                  !memcmp( ATR_Samsung, atr+1, 3/*HSM_ATR_LEN*/ ) || 
                  !memcmp( ATR_Coregate, atr+1, 3/*HSM_ATR_LEN*/ )  || 
                  !memcmp( ATR_Coregate_NonOS_V1, atr+1, 3/*HSM_ATR_LEN*/ ) ||
                  !memcmp( ATR_Coregate_NonOS_V2, atr+1, 3/*HSM_ATR_LEN*/ )) ||
                g_HSM_update_flag) //Check ATR
			{
				ret = ISO7816_OK;
			}
			else
			{
                GLogN("\n\r --------------- ATR RETRY ---------------- \n\r"); //ATR Retry
			  	ret = ISO7816_ERR_ATR_WRONG;
			}
		 }
	 }
 
	 return ret;
 }
 
 int32_t se_close( void )
 {
	 DeinitHSM();
 
	 return ISO7816_OK;
 }
 
 int32_t se_tranreceive( uint8_t *apdu, uint32_t apdulen, uint8_t *resp, uint32_t *resplen )
 {
	 uint32_t	 n, le, lc, retry;
 
	 uint8_t	 *optr		 = resp;
	 int32_t	 ret		 = ISO7816_OK;
	 int		 wait_sw2	 = 0;
	 uint8_t	 byte		 = 0;
#if defined( DBG_MSG )
	 GLogN( "[%s]\r\n", __FUNCTION__ );
#endif
	 *resplen	 = 0;
	 le = lc	 = 0;
 
	 /* we do not support case 2E, 3E, 4* */
	 if( apdulen == 4 ) 						 /* Case 1 */
	 {
		 n = 4;
	 }
	 else if( apdulen == 5 )					 /* Case 2S */
	 {
		 le = apdu[4] ? apdu[4] : 256;
		 n = 5;
	 }
	 else										 /* Case 3S */
	 {
		 lc = apdu[4] ? apdu[4] : 256;
		 n = 5 + lc;
	 }
 
	 if( n != apdulen )
	 {
		 return ISO7816_ERR;
	 }
 
	 /* Transmit command header */
	 if( apdulen == 4 )
	 {
		 write_bytes( apdu, 4 );
		 byte = 0x00;
		 write_bytes( &byte, 1 );
	 }
	 else
	 {
		 write_bytes( apdu, 5 );
	 }
 
	 retry = MAX_NUM_RETRY;
 
	 /* wait for procedure byte */
	 while( retry-- )
	 {
		 byte = 0;
 
		 if( !read_bytes( &byte, 1 ) )
		 {
			 return ISO7816_ERR_TIMEPOUT;
		 }
 
		 //GLogN( "Got 0x%02X -> ", byte );
 
		 if( byte == 0x60 )
		 {
			 continue;
		 }
 
		 if( (byte & 0xf0) == 0x60 || (byte & 0xf0) == 0x90 )
		 {
			 //GLogN( "0x6x or 0x9x is set \r\n" );
			 *optr++ = byte;
			 wait_sw2 = 1;
			 break;
		 }
		 else if( byte == apdu[1] )
		 {
			 //GLogN( "Got an ACK - le:%d, lc:%d\r\n", le, lc );
			 if( le )
			 {
				 optr += read_bytes( optr, le );
			 }
			 else if( lc )
			 {
				 write_bytes( &apdu[5], lc );
			 }
			 else
			 {
				 //GLogN( "Got an ACK, but le/lc = 0\r\n" );
			 }
		 }
		 else
		 {
			 GLogN( "We've got an unexpected byte.\r\n" );
			 *optr++ = byte;
			 ret = ISO7816_ERR;
			 read_bytes( optr++, 1 );
			 break;
		 }
         
         osDelay(1);
	 }
 
	 if( retry == 0 )
	 {
		 GLogN( "Reached at maximum retry count - Give up\r\n" );
		 return ISO7816_ERR;
	 }
 
	 if( wait_sw2 )  read_bytes( optr++, 1 );
 
	 osDelay( 1 );
 
	 *resplen = optr - resp;
 
	 return ret;
 }
 
 int32_t se_pps( void )
 {
	 uint8_t buf[4] = { 0, };
	 uint8_t *cmd;
 
	 cmd = get_pps_cmd();
	 if( cmd == NULL )			 return 0;
 
	 write_bytes( cmd, 4 );
	 read_bytes( buf, 4 );
 
	 GLogN( "PPS req : %02X %02X %02X %02X\r\n", cmd[0], cmd[1], cmd[2], cmd[3] );
	 GLogN( "PPS rsp : %02X %02X %02X %02X\r\n", buf[0], buf[1], buf[2], buf[3] );
 
	 if( !memcmp( buf, cmd, 4 ) )
	 {
		 InitHSMUart( ISO7816_BAUDRATE2 );
		 osDelay( 1 );
	 }
 
	 return 0;
 }
 
 static uint8_t *get_pps_cmd( void )
 {
	 int i = 0;
	 for( i = 0; i < (int)(sizeof(pps) / sizeof(*pps)); i++ )
	 {
		 if( pps[i].cycles_per_etu == ISO7816_PPS ) 	 return pps[i].cmd;
	 }
 
	 return NULL;
 }

#if 0
uint32_t read_bytes( uint8_t *buf, uint32_t len )
 {
	 //HAL_StatusTypeDef st;
 
	 uint32_t	 remain 	 = len;
	 uint32_t	 sz 		 = 0;
	 uint8_t	 waittime	 = 30;				 // 3 seconds
	 uint32_t	 size		 = 0;
	 uint32_t	 count		 = 0;
 
	 while( waittime )
	 {
		 size = getHSMReceiveDataSize();
		 if( size >= remain )
		 {
			 break;
		 }
 
		 waittime--;
 
		 osDelay( 100 );
	 }
 
	 sz = ( remain > ISO7816_BUFFER_SIZE ) ? ISO7816_BUFFER_SIZE : remain;
	 count = getHSMReceiveData( buf, sz );
 
#if defined( DBG_MSG )
 {
	 uint32_t	 i;
	 GLogI( "HSM Rx : " );
	 for( i = 0; i < sz; i++ )
	 {
		 GLogN( "%02X ", buf[i] );
	 }
	 GLogN("\r\n");
 }
#endif
 
	 return count;
 }
#else
uint32_t read_bytes(uint8_t *buf, uint32_t len)
{
	uint32_t remain = len;
    uint32_t total = 0U;
    uint32_t got = 0;
    const TickType_t timeoutTicks = pdMS_TO_TICKS(3000U);
    TickType_t startTick = xTaskGetTickCount();

    //xStreamBufferSetTriggerLevel(hSBHsmRx, len);

    while (remain > 0U)
    {
        uint32_t chunk = (remain > ISO7816_BUFFER_SIZE) ? ISO7816_BUFFER_SIZE : remain;

        TickType_t nowTick = xTaskGetTickCount();
        TickType_t elapsed = nowTick - startTick;

        if (elapsed >= timeoutTicks)
        {
            GLogN("\r\nlen : %d        got : %d        remain : %d         total : %d", len, got, remain, total);
            break;
        }

        TickType_t waitTicks = timeoutTicks - elapsed;

        got = xStreamBufferReceive(hSBHsmRx, buf, chunk, waitTicks);

        buf += got;
        remain -= got;
        total += got;
        
        if (remain == 0)
        {
            break;
        }

        // startTick = xTaskGetTickCount();
    }
    
    return total;
}
#endif
 
uint32_t read_bytes_atr( uint8_t *buf, uint32_t bufsize )
{
    uint32_t got  = 0U;

    /* ----------- 1-1???: TS ????? ???? ----------- */
    for( ;; )
    {
        /* TS ??? 1??????? ???? */
        uint32_t chunk = xStreamBufferReceive( hSBHsmRx, &buf[0], 1U, pdMS_TO_TICKS(1000) );
        if( chunk == 0U )
        {
            return 0U;                     /* ????? */
        }

        /* ??? TS???? ??? (0x3B: direct, 0x3F: inverse)          */
        if( (buf[0] == 0x3BU) || (buf[0] == 0x3FU) )
        {
            got = 1U;                      /* TS ??? */
            break;                         /* ?????? ??? ???        */
        }
    }

    /* ----------- 1-2???: T0 ???? ----------- */
    while( got < 2U )
    {
        uint32_t chunk = xStreamBufferReceive( hSBHsmRx, &buf[got], 2U - got, pdMS_TO_TICKS(1000) );
        if( chunk == 0U )
        {
            return 0U;
        }
        got += chunk;
    }

    /* ----------- 2???: T0 ?��??? ??u ???? ??? ----------- */
    uint8_t  K         = buf[1U] & 0x0FU;   /* ???????? ????? ??   */
    uint8_t  Y         = buf[1U] >> 4U;     /* ????????? Y1 ???    */
    uint32_t iface_cnt = 0U;
    uint8_t  proto_tck = 0U;                /* T??0 ???? ???? ?��???  */

    uint32_t idx = 2U;                      /* ??? ???? ???? ?��??? */

    /* TD u???? ?????? ????????? ????? ?? ???? */
    while (Y != 0U)
    {
        if (Y & 0x1U)   iface_cnt++;        /* TAx */
        if (Y & 0x2U)   iface_cnt++;        /* TBx */
        if (Y & 0x4U)   iface_cnt++;        /* TCx */
        if (Y & 0x8U)                       /* TDx: ?? ??? Y ????   */
        {
            iface_cnt++;
        }
        else
        {
            break;                          /* TDx ?????? ??          */
        }

        uint32_t td_pos = 2U + iface_cnt - 1U;
        uint32_t need   = td_pos + 1U;

        while (got < need)
        {
            uint32_t chunk = xStreamBufferReceive( hSBHsmRx, &buf[got], need - got, pdMS_TO_TICKS(1000) );
            if (chunk == 0U)
            {
                return 0U;
            }
            got += chunk;
        }

        uint8_t proto = buf[td_pos] & 0x0FU;
        if (proto != 0x00U)
        {
            proto_tck = 1U;
        }

        Y   = buf[td_pos] >> 4U;
        idx = td_pos + 1U;
    }

    uint32_t need = 2U + iface_cnt + K + proto_tck; /* ??u ATR ???? ???? */

    if (need > bufsize)
    {
        return 0U;
    }

    while (got < need)
    {
        uint32_t chunk = xStreamBufferReceive( hSBHsmRx, &buf[got], need - got, pdMS_TO_TICKS(1000) );
        if (chunk == 0U)
        {
            return 0U;
        }
        got += chunk;
    }

#if defined( DBG_MSG )
    {
        uint32_t i;
        GLogI( "HSM Rx : " );
        for( i = 0; i < got; i++ )
        {
            GLogN( "%02X ", buf[i] );
        }
        GLogN("\r\n");
    }
#endif

    return got;
}

 static uint32_t write_bytes( uint8_t *buf, uint32_t len )
 {
	 HAL_StatusTypeDef	 st;
 
	 int	 remain 	 = len;
 
#if defined( DBG_MSG )
 {
	 uint32_t	 i;
	 GLogI( "HSM Tx : " );
	 for( i = 0; i < len; i++ )
	 {
		 GLogN( "%02X ", buf[i] );
	 }
	 GLogN("\r\n");
 }
#endif
 
	 HSM_UART_PORT.RxState = HAL_UART_STATE_READY;
	 while( remain )
	 {
		 int sz = remain > ISO7816_BUFFER_SIZE ? ISO7816_BUFFER_SIZE : remain;
 
		 st = HAL_UART_Transmit( &HSM_UART_PORT, buf, sz, ISO7816_TIMEOUT );
		 if( st != HAL_OK )
		 {
			 GLogE( "Error... Fail write uart!!!\r\n" );
			 return 0;
		 }
 
		 /* as Tx and Rx are muxed, we need to clean the Rx buffer */
		 //read_bytes( tmp_buf, sz );	 //???ë¡œ???¤ì??´?¤ë???��??????????????¤ì???¤ë???buffer ??????
 
		 buf	 += sz;
		 remain  -= sz;
	 }
 
	 HAL_UART_Receive_IT( &HSM_UART_PORT, &gucHSMRxDummy, 1 );
 
	 return len;
 }
 
 /*----------------------------------------------------------------------
  *   Security
  *--------------------------------------------------------------------*/
/* --- Crypto serialization (fix intermittent AES/RSA garbage) ---------------
 * CMOX AES/RSA use the shared STM32 hardware CRC peripheral (hcrc1) with no HW
 * arbitration. transmitThread (encrypt) and parsingThread (decrypt) call these
 * concurrently, so unguarded access races on hcrc1 and yields intermittent
 * garbage output (e.g. corrupted PID). One mutex serializes every public crypto
 * wrapper. Lock ONLY at this wrapper layer (never the internal STM32_* layer)
 * and never nest wrappers -> non-recursive mutex is deadlock-free. NULL-guarded
 * so crypto still works if called before Crypto_Mutex_Init() (boot is single-
 * threaded, and key exchange -- first crypto use -- happens after connect). */
osMutexDef(CryptoMutex);
osMutexId hCryptoMutex = NULL;
void Crypto_Mutex_Init(void)
{
	if(hCryptoMutex == NULL)
	{
		hCryptoMutex = osMutexCreate(osMutex(CryptoMutex));
	}
}
#define CRYPTO_LOCK()    do { if(hCryptoMutex != NULL) { osMutexWait(hCryptoMutex, osWaitForever); } } while(0)
#define CRYPTO_UNLOCK()  do { if(hCryptoMutex != NULL) { osMutexRelease(hCryptoMutex); } } while(0)

 int32_t getAESEncoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t AES_Type, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength )
{
	CRYPTO_LOCK();
	 int32_t		 status = CMOX_CIPHER_SUCCESS;
	 uint32_t		 uiLength = 0;
	 
	 MX_CRC_Init_CMOX();
	 __CRC_CLK_ENABLE();
	 if( MsgLength%16 )
	 {
		 uiLength = ((MsgLength/16)+1)*16;
	 }
	 else
	 {
		 uiLength = MsgLength;
	 }

	 status = STM32_AES_ECB_Encrypt( (uint8_t *) pPlaintext, uiLength, pKeyString, AES_Type, pOutputMessage, pOutputMessageLength);
	 //if(status == CMOX_CIPHER_SUCCESS)
	 //{
	//	 *pOutputMessageLength = MsgLength;
	 //}
     HAL_CRC_DeInit(&hcrc1);
	 CRYPTO_UNLOCK();
	 return status;
 }

int32_t getAESDecoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t AES_Type, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength )
{
	CRYPTO_LOCK();
	 int32_t		 status = CMOX_CIPHER_SUCCESS;
	 uint32_t		 uiLength = 0;
	 
	 MX_CRC_Init_CMOX();
	 __CRC_CLK_ENABLE();
	 if( MsgLength%16 )
	 {
		 uiLength = ((MsgLength/16)+1)*16;
	 }
	 else
	 {
		 uiLength = MsgLength;
	 }

	 status = STM32_AES_ECB_Decrypt( (uint8_t *) pPlaintext, uiLength, pKeyString, AES_Type, pOutputMessage, pOutputMessageLength);
	 //if(status == CMOX_CIPHER_SUCCESS)
	 //{
	//	 *pOutputMessageLength = MsgLength;
	 //}
	 HAL_CRC_DeInit(&hcrc1);
	 CRYPTO_UNLOCK();
	 return status;
 }

 int32_t getAESEncoding_CTR(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *iv, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength )
{
	CRYPTO_LOCK();
  	int32_t 		status = CMOX_CIPHER_SUCCESS;

	
	MX_CRC_Init_CMOX();
	__CRC_CLK_ENABLE();

	status = STM32_AES_CTR_Encrypt( (uint8_t *) pPlaintext, MsgLength, pKeyString, iv, pOutputMessage, pOutputMessageLength);

	CRYPTO_UNLOCK();
	return status;
}

int32_t getAESEncoding_CBC(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength )
{
	CRYPTO_LOCK();
  	int32_t 		status = CMOX_CIPHER_SUCCESS;

	
	MX_CRC_Init_CMOX();
	__CRC_CLK_ENABLE();

	status = STM32_AES_CBC_Encrypt( (uint8_t *) pPlaintext, MsgLength, pKeyString, pOutputMessage, pOutputMessageLength);

	CRYPTO_UNLOCK();
	return status;
}

int32_t getTDESEncoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength )
{
  	int32_t 		status = TDES_SUCCESS;
	
	MX_CRC_Init_CMOX();
	__CRC_CLK_ENABLE();
        
	status = STM32_TDES_ECB_Encrypt( (uint8_t *) pPlaintext, MsgLength, pKeyString, pOutputMessage, pOutputMessageLength);

	return status;
}

int32_t getTDESEncoding_CBC(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *IV, uint32_t IVLength, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength )
{
  	int32_t 		status = TDES_SUCCESS;
	
	MX_CRC_Init_CMOX();
	__CRC_CLK_ENABLE();
        
	status = STM32_TDES_CBC_Encrypt( (uint8_t *) pPlaintext, MsgLength, pKeyString,(uint8_t *) IV, IVLength, pOutputMessage, pOutputMessageLength);

	return status;
}

int32_t getDESEncoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength )
{
  	int32_t 		status = TDES_SUCCESS;
	
	MX_CRC_Init_CMOX();
	__CRC_CLK_ENABLE();
        
	status = STM32_DES_ECB_Encrypt( (uint8_t *) pPlaintext, MsgLength, pKeyString, pOutputMessage, pOutputMessageLength);

	return status;
}

int32_t getDESDecoding_ECB(uint8_t *pPlaintext, uint32_t MsgLength, uint8_t *pKeyString, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength )
{
  	int32_t 		status = TDES_SUCCESS;
	
	MX_CRC_Init_CMOX();
	__CRC_CLK_ENABLE();
        
	status = STM32_DES_ECB_Decrypt( (uint8_t *) pPlaintext, MsgLength, pKeyString, pOutputMessage, pOutputMessageLength);

	return status;
}

U32 EncLen;
int32_t setRSA_Encrypt(uint8_t *pInMessage, uint32_t MsgLength, uint8_t *pKeyString, uint32_t KeySize, uint8_t *pOutMessage)
{
	CRYPTO_LOCK();
        MX_CRC_Init_CMOX();
		__CRC_CLK_ENABLE();
        
        // DRBG START
        cmox_ctr_drbg_handle_t Drbg_Ctx;
        cmox_drbg_handle_t *drgb_ctx;
        
        int32_t status = CMOX_INIT_FAIL;
        EncLen = MsgLength;
        uint8_t entropy_data[32] = /* String of entropy */
        {
          0x91, 0x20, 0x1a, 0x18, 0x9b, 0x6d, 0x1a, 0xa7,
          0x0e, 0x69, 0x57, 0x6f, 0x36, 0xb6, 0xaa, 0x88,
          0x55, 0xfd, 0x4a, 0x7f, 0x97, 0xe9, 0x72, 0x69,
          0xb6, 0x60, 0x88, 0x78, 0xe1, 0x9c, 0x8c, 0xa5
        };
        
        /*uint8_t EntropyInputReseed[] =
        {
          0xb7, 0xec, 0x46, 0x07, 0x23, 0x63, 0x83, 0x4a, 
          0x1b, 0x01, 0x33, 0xf2, 0xc2, 0x38, 0x91, 0xdb,
          0x4f, 0x11, 0xa6, 0x86, 0x51, 0xf2, 0x3e, 0x3a, 
          0x8b, 0x1f, 0xdc, 0x03, 0xb1, 0x92, 0xc7, 0xe7
        };*/
        
        uint8_t RNG_Data[sizeof(entropy_data)] = {0, };
        
        if (cmox_initialize(NULL) == CMOX_INIT_SUCCESS)
        {
          //if((drgb_ctx = cmox_ctr_drbg_construct(&Drbg_Ctx, CMOX_CTR_DRBG_AES256)) != NULL)
          if((drgb_ctx = cmox_ctr_drbg_construct(&Drbg_Ctx, CMOX_CTR_DRBG_AES256)) != NULL) //mod.kks todo check
          {
            if(cmox_drbg_init(drgb_ctx, entropy_data, sizeof(entropy_data), 0, 0, 0, 0) == CMOX_DRBG_SUCCESS)
            {
              //cmox_drbg_reseed(drgb_ctx, EntropyInputReseed, sizeof(EntropyInputReseed), NULL, 0);
              if(cmox_drbg_generate(drgb_ctx, NULL, 0, RNG_Data, sizeof(RNG_Data)) == CMOX_DRBG_SUCCESS)
              {
                if(cmox_drbg_cleanup(drgb_ctx) == CMOX_DRBG_SUCCESS) // issue Length 16 --> 0 !!!!!!
                {
                  if (cmox_finalize(NULL) == CMOX_INIT_SUCCESS)
                  {
                    status = CMOX_INIT_SUCCESS;
                  }
                }
              }
            }
          }
        }
        
        //status = cmox_drbg_reseed(drgb_ctx,                                           /* DRBG context */
        //                          EntropyInputReseed, sizeof(EntropyInputReseed),     /* Entropy reseed data */
        //                          NULL, 0);                                           /* No additional reseed data */
        //if (status != CMOX_DRBG_SUCCESS)
        //{
        //  status = CMOX_INIT_FAIL;
        //}
        
        // RSA ENCRYPT START
        if(status == CMOX_INIT_SUCCESS)
        {
          status = CMOX_INIT_FAIL;
          
          cmox_rsa_handle_t Rsa_Ctx;
          cmox_rsa_key_t Rsa_Key;
          
          // RSA-2048: modulus=256, exp=3 (259 total)
          // RSA-1024: modulus=128, exp=3 (131 total)
          int32_t ModulusSize = (KeySize >= 259) ? 256 : 128;
          int32_t ExponentSize = 3;  // Exponent is always 3 bytes (01 00 01)

          uint8_t KeySt[256];  // Support RSA-2048
          uint8_t Exponent[3];
          
          if (cmox_initialize(NULL) == CMOX_INIT_SUCCESS)
          {
            cmox_rsa_construct(&Rsa_Ctx, CMOX_RSA_MATH_FUNCS, CMOX_MODEXP_PUBLIC, Working_Buffer, sizeof(Working_Buffer));
            memcpy(KeySt, pKeyString, ModulusSize);
            memcpy(Exponent, &pKeyString[ModulusSize], ExponentSize);
            
            /*GLogN("Modulus : ");
            for(int i=0; i<ModulusSize; i++) {
            GLogN("0x%x ", KeySt[i]);
          }
            
            GLogN("\r\nExponent : ");
            for(int i=0; i<ExponentSize; i++) {
            GLogN("0x%x ", Exponent[i]);
          }*/
            
            if(cmox_rsa_setKey(&Rsa_Key, KeySt, ModulusSize, Exponent, ExponentSize) == CMOX_RSA_SUCCESS)
            {
              /*status = cmox_rsa_pkcs1v22_encrypt(&Rsa_Ctx,
              &Rsa_Key,
              pInMessage, sizeof(pInMessage),
              CMOX_RSA_PKCS1V22_HASH_SHA1,
              RNG_Data, sizeof(RNG_Data), 
              NULL, NULL, 
              pOutMessage, &MsgLength);*/
              status = cmox_rsa_pkcs1v15_encrypt(&Rsa_Ctx,
                                                 &Rsa_Key,
                                                 pInMessage, EncLen,        
                                                 RNG_Data, sizeof(RNG_Data), 
                                                 pOutMessage, &MsgLength);
            }
          }
        }
        
        CRYPTO_UNLOCK();
        return status;
}

/**
  * @brief  RSA Encryption with PKCS#1v1.5
  * @param  P_pPubKey The RSA public key structure, already initialized
  * @param  P_pInputMessage Input Message to be signed
  * @param  P_MessageSize Size of input message
  * @param  P_pOutput Pointer to output buffer
  * @retval error status: can be RSA_SUCCESS if success or one of
  * RSA_ERR_BAD_PARAMETER, RSA_ERR_MESSAGE_TOO_LONG, RSA_ERR_BAD_OPERATION
*/

#if 0
static int32_t RSA_Encrypt(RSApubKey_stt *P_pPubKey,
                    const uint8_t *P_pInputMessage,
                    int32_t P_InputSize,
                    uint8_t *P_pOutput)
{
	uint8_t entropy_data[32] = /* String of entropy */
	{
		0x91, 0x20, 0x1a, 0x18, 0x9b, 0x6d, 0x1a, 0xa7,
		0x0e, 0x69, 0x57, 0x6f, 0x36, 0xb6, 0xaa, 0x88,
		0x55, 0xfd, 0x4a, 0x7f, 0x97, 0xe9, 0x72, 0x69,
		0xb6, 0x60, 0x88, 0x78, 0xe1, 0x9c, 0x8c, 0xa5
	};

	int32_t status = RNG_SUCCESS ;
	RNGstate_stt RNGstate;
	RNGinitInput_stt RNGinit_st;

	RNGinit_st.pmEntropyData = entropy_data;
	RNGinit_st.mEntropyDataSize = sizeof(entropy_data);
	RNGinit_st.mPersDataSize = 0;
	RNGinit_st.mNonceSize = 0;

	status = RNGinit(&RNGinit_st, &RNGstate);
	if (status == RNG_SUCCESS)
	{
		RSAinOut_stt inOut_st;
		membuf_stt mb;
		//mb.mSize = sizeof(preallocated_buffer);
		mb.mSize = 4096;
		mb.mUsed = 0;
		//mb.pmBuf = preallocated_buffer;
		mb.pmBuf = (uint8_t*)malloc( mb.mSize );

		/* Fill the RSAinOut_stt */
		inOut_st.pmInput = P_pInputMessage;
		inOut_st.mInputSize = P_InputSize;
		inOut_st.pmOutput = P_pOutput;

		/* Encrypt the message, this function will write sizeof(modulus) data */
		status = RSA_PKCS1v15_Encrypt(P_pPubKey, &inOut_st, &RNGstate, &mb);
		free(mb.pmBuf);
	}
	return(status);
}
#endif

/**
  * @brief  AES ECB Encryption example.
  * @param  InputMessage: pointer to input message to be encrypted.
  * @param  InputMessageLength: input data message length in byte.
  * @param  AES128_Key: pointer to the AES key to be used in the operation
  * @param  OutputMessage: pointer to output parameter that will handle the encrypted message
  * @param  OutputMessageLength: pointer to encrypted message length.
  * @retval error status: can be AES_SUCCESS if success or one of
  *         AES_ERR_BAD_INPUT_SIZE, AES_ERR_BAD_OPERATION, AES_ERR_BAD_CONTEXT
  *         AES_ERR_BAD_PARAMETER if error occured.
  */
static int32_t STM32_AES_ECB_Encrypt(uint8_t* InputMessage,
                              uint32_t InputMessageLength,
                              uint8_t  *AES256_Key,
                              uint8_t AES_Type,
                              uint8_t  *OutputMessage,
                              uint32_t *OutputMessageLength)
{
  __CRC_CLK_ENABLE();
  
  uint32_t error_status = CMOX_CIPHER_SUCCESS;
  cmox_cipher_keyLen_t cmAESType;
  
  if(AES_Type == AES128) cmAESType = AES128_KEY_SIZE;
  else cmAESType = AES256_KEY_SIZE;
  
  cmox_cipher_retval_t retval;
  
  if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
  {
    error_status = CMOX_CIPHER_ERR_BAD_PARAMETER;
  }
  
  retval = cmox_cipher_encrypt(CMOX_AES_ECB_ENC_ALGO,              
                               InputMessage, InputMessageLength,
                               AES256_Key, cmAESType,          
                               NULL, NULL,                        
                               OutputMessage, OutputMessageLength); 
  
  if (retval != CMOX_CIPHER_SUCCESS)
  {
    error_status = retval;
  }

  return error_status;
}

static int32_t STM32_AES_ECB_Decrypt(uint8_t* InputMessage,
                              uint32_t InputMessageLength,
                              uint8_t  *AES256_Key,
                              uint8_t AES_Type,
                              uint8_t  *OutputMessage,
                              uint32_t *OutputMessageLength)
{
  __CRC_CLK_ENABLE();
  
  uint32_t error_status = CMOX_CIPHER_SUCCESS;
  cmox_cipher_keyLen_t cmAESType;
  
  if(AES_Type == AES128) cmAESType = AES128_KEY_SIZE;
  else cmAESType = AES256_KEY_SIZE;
  
  cmox_cipher_retval_t retval;
  
  if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
  {
    error_status = CMOX_CIPHER_ERR_BAD_PARAMETER;
  }
  
  retval = cmox_cipher_decrypt(CMOX_AES_ECB_DEC_ALGO,              
                               InputMessage, InputMessageLength,
                               AES256_Key, cmAESType,          
                               NULL, NULL,                        
                               OutputMessage, OutputMessageLength); 
  
  if (retval != CMOX_CIPHER_SUCCESS)
  {
    error_status = retval;
  }

  return error_status;
}

static int32_t STM32_AES_CTR_Encrypt(uint8_t* InputMessage,
                              uint32_t InputMessageLength,
                              uint8_t  *AES256_Key,
                              uint8_t  *iv,
                              uint8_t  *OutputMessage,
                              uint32_t *OutputMessageLength)
{
  __CRC_CLK_ENABLE();
  
  uint32_t error_status = CMOX_CIPHER_SUCCESS;
  
  cmox_cipher_retval_t retval;
  
  if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
  {
    error_status = CMOX_CIPHER_ERR_BAD_PARAMETER;
  }

  retval = cmox_cipher_encrypt(CMOX_AES_CTR_ENC_ALGO,
                               InputMessage, InputMessageLength,
                               AES256_Key, AES256_KEY_SIZE,
                               iv, AES_IV_SIZE,
                               OutputMessage, OutputMessageLength); 
  
  if (retval != CMOX_CIPHER_SUCCESS)
  {
    error_status = retval;
  }

  return error_status;
}

static int32_t STM32_AES_CBC_Encrypt(uint8_t* InputMessage,
                              uint32_t InputMessageLength,
                              uint8_t  *AES256_Key,
                              uint8_t  *OutputMessage,
                              uint32_t *OutputMessageLength)
{
  __CRC_CLK_ENABLE();
  
  uint32_t error_status = CMOX_CIPHER_SUCCESS;
  
  cmox_cipher_retval_t retval;
  
  if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
  {
    error_status = CMOX_CIPHER_ERR_BAD_PARAMETER;
  }
  
  U8 iv[16] = {0x00, };
  
  retval = cmox_cipher_encrypt(CMOX_AES_CBC_ENC_ALGO,              
                               InputMessage, InputMessageLength,
                               AES256_Key, AES256_KEY_SIZE,          
                               iv, 16,                        
                               OutputMessage, OutputMessageLength); 
  
  if (retval != CMOX_CIPHER_SUCCESS)
  {
    error_status = retval;
  }

  return error_status;
}

static int32_t STM32_TDES_ECB_Encrypt(uint8_t* InputMessage,
                                      uint32_t InputMessageLength,
                                      uint8_t  *TDES_Key,
                                      uint8_t  *OutputMessage,
                                      uint32_t *OutputMessageLength)
{
  TDESECBctx_stt TDESctx;

  uint32_t error_status = TDES_SUCCESS;

  int32_t outputLength = 0;

  /* Set flag field to default value */
  TDESctx.mFlags = E_SK_DEFAULT;

  /* Initialize the operation, by passing the key.
   * Third parameter is NULL because ECB doesn't use any IV */
  error_status = TDES_ECB_Encrypt_Init(&TDESctx, TDES_Key, NULL );

  /* check for initialization errors */
  if (error_status == TDES_SUCCESS)
  {
    /* Encrypt Data */
    error_status = TDES_ECB_Encrypt_Append(&TDESctx,
                                           InputMessage,
                                           InputMessageLength,
                                           OutputMessage,
                                           &outputLength);

    if (error_status == TDES_SUCCESS)
    {
      /* Write the number of data written*/
      *OutputMessageLength = outputLength;
      /* Do the Finalization */
      error_status = TDES_ECB_Encrypt_Finish(&TDESctx, OutputMessage + *OutputMessageLength, &outputLength);
      /* Add data written to the information to be returned */
      *OutputMessageLength += outputLength;
    }
  }

  return error_status;
}

int32_t STM32_TDES_CBC_Encrypt(uint8_t* InputMessage,
                               uint32_t InputMessageLength,
                               uint8_t  *TDES_Key,
                               uint8_t  *InitializationVector,
                               uint32_t  IvLength,
                               uint8_t  *OutputMessage,
                               uint32_t *OutputMessageLength)
{
  TDESCBCctx_stt TDESctx;

  uint32_t error_status = TDES_SUCCESS;

  int32_t outputLength = 0;

  /* Set flag field to default value */
  TDESctx.mFlags = E_SK_DEFAULT;

  /* Set iv size field to IvLength*/
  TDESctx.mIvSize = IvLength;

  error_status = TDES_CBC_Encrypt_Init(&TDESctx, TDES_Key, InitializationVector );


  /* check for initialization errors */
  if (error_status == TDES_SUCCESS)
  {

    /* Encrypt Data */
    error_status = TDES_CBC_Encrypt_Append(&TDESctx,
                                           InputMessage,
                                           InputMessageLength,
                                           OutputMessage,
                                           &outputLength);


    if (error_status == TDES_SUCCESS)
    {
      /* Write the number of data written*/
      *OutputMessageLength = outputLength;
      /* Do the Finalization */
      error_status = TDES_CBC_Encrypt_Finish(&TDESctx, OutputMessage + *OutputMessageLength, &outputLength);
      /* Add data written to the information to be returned */
      *OutputMessageLength += outputLength;
    }
  }

  return error_status;
}

int32_t STM32_DES_ECB_Encrypt(uint8_t* InputMessage,
                              uint32_t InputMessageLength,
                              uint8_t  *DES_Key,
                              uint8_t  *OutputMessage,
                              uint32_t *OutputMessageLength)
{
  DESECBctx_stt DESctx;

  uint32_t error_status = DES_SUCCESS;

  int32_t outputLength = 0;

  /* Set flag field to default value */
  DESctx.mFlags = E_SK_DEFAULT;

  /* Initialize the operation, by passing the key.
   * Third parameter is NULL because ECB doesn't use any IV */
  error_status = DES_ECB_Encrypt_Init(&DESctx, DES_Key, NULL );

  /* check for initialization errors */
  if (error_status == DES_SUCCESS)
  {
    /* Encrypt Data */
    error_status = DES_ECB_Encrypt_Append(&DESctx,
                                          InputMessage,
                                          InputMessageLength,
                                          OutputMessage,
                                          &outputLength);

    if (error_status == DES_SUCCESS)
    {
      /* Write the number of data written*/
      *OutputMessageLength = outputLength;
      /* Do the Finalization */
      error_status = DES_ECB_Encrypt_Finish(&DESctx, OutputMessage + *OutputMessageLength, &outputLength);
      /* Add data written to the information to be returned */
      *OutputMessageLength += outputLength;
    }
  }

  return error_status;
}

int32_t STM32_DES_ECB_Decrypt(uint8_t* InputMessage,
                              uint32_t InputMessageLength,
                              uint8_t  *DES_Key,
                              uint8_t  *OutputMessage,
                              uint32_t *OutputMessageLength)
{
  DESECBctx_stt DESctx;

  uint32_t error_status = DES_SUCCESS;

  int32_t outputLength = 0;

  /* Set flag field to default value */
  DESctx.mFlags = E_SK_DEFAULT;

  /* Initialize the operation, by passing the key.
  * Third parameter is NULL because ECB doesn't use any IV */
  error_status = DES_ECB_Decrypt_Init(&DESctx, DES_Key, NULL );

  /* check for initialization errors */
  if (error_status == DES_SUCCESS)
  {
    /* Decrypt Data */
    error_status = DES_ECB_Decrypt_Append(&DESctx,
                                          InputMessage,
                                          InputMessageLength,
                                          OutputMessage,
                                          &outputLength);

    if (error_status == DES_SUCCESS)
    {
      /* Write the number of data written*/
      *OutputMessageLength = outputLength;
      /* Do the Finalization */
      error_status = DES_ECB_Decrypt_Finish(&DESctx, OutputMessage + *OutputMessageLength, &outputLength);
      /* Add data written to the information to be returned */
      *OutputMessageLength += outputLength;
    }
  }

  return error_status;
}

/* Remove annotations and use if necessary (Q_hyek) */
#if 0
uint8_t Working_Buffer[15000];
U8 Computed_Signature[10000];
int32_t RSA_Sign_SHA1(uint8_t *Seed, uint32_t SeedLength, uint8_t *pOutputMessage, uint32_t *pOutputMessageLength )
{
  U8 SEED[20] = {0x00, };
  //U32 SEED_SIZE = SeedLength;
        
  /* RSA context */
  cmox_rsa_handle_t Rsa_Ctx;
  /* RSA key */
  cmox_rsa_key_t Rsa_Key;
  
  /* RSA working buffer */
  
  cmox_rsa_retval_t retval;
  
  U8 Modulus[128] = {0x00, };
  U8 Private_Exponent[128] = {0x00, };
  //U8 Modulus[] = {0xAF, 0xA8, 0x15, 0xBD, 0x3C, 0x1A, 0x2B, 0xCD, 0x98, 0x72, 0x0D, 0x4F, 0x1A, 0xB8, 0x9C, 0x0C, 0xC4, 0x76, 0xBE, 0x7D, 0x7D, 0x27, 0xA0, 0xE9, 0x35, 0x97, 0xE9, 0x7D, 0x7C, 0x54, 0xBB, 0x15, 0x75, 0x99, 0x59, 0x59, 0x0F, 0x69, 0x9D, 0x12, 0xCE, 0x14, 0x58, 0xF6, 0x42, 0xD4, 0xB0, 0x1C, 0x7F, 0xA2, 0x03, 0xA3, 0x78, 0x88, 0xD7, 0xE9, 0x02, 0x48, 0x05, 0x81, 0x1E, 0xF0, 0xF4, 0x06, 0x7B, 0x9D, 0x50, 0xBD, 0x7F, 0x9F, 0x51, 0x2E, 0x32, 0x4D, 0x3D, 0xA9, 0xF7, 0xF9, 0x3A, 0xFC, 0xD3, 0x2B, 0x10, 0xE3, 0xAB, 0x7E, 0xB7, 0x6C, 0x2A, 0x44, 0x10, 0x76, 0xE4, 0x10, 0x8B, 0x5F, 0x7E, 0x47, 0xF5, 0xAA, 0x9A, 0x4E, 0x95, 0xF8, 0xAD, 0x05, 0xD1, 0x27, 0x1A, 0xCB, 0xF0, 0x59, 0x56, 0xAB, 0x20, 0xA0, 0x30, 0x31, 0x9B, 0xCF, 0x4B, 0x86, 0x5B, 0x7D, 0xF2, 0x79, 0x39, 0xEC, 0xF0, 0x45, 0x54, 0xF2, 0x1B, 0x4A, 0x75, 0xA9, 0x99, 0x0C, 0x99, 0xA8, 0x00, 0x02, 0x06, 0x61, 0x7F, 0x99, 0xA4, 0x92, 0xCF, 0x7C, 0x49, 0x5C, 0x6C, 0x28, 0x9C, 0xCE, 0x32, 0x55, 0x7F, 0x3A, 0x7E, 0xA2, 0x82, 0x0C, 0x6F, 0x92, 0xEF, 0xFA, 0xFE, 0x98, 0x04, 0x94, 0xB7, 0xC4, 0xF6, 0x0B, 0x97, 0x24, 0xA8, 0x50, 0x63, 0x32, 0xE2, 0xDF, 0xE0, 0x42, 0x57, 0x4D, 0xCA, 0xC1, 0x7D, 0x0E, 0xE1, 0x16, 0x55, 0xDA, 0x1B, 0x23, 0x32, 0x92, 0x60, 0x48, 0x6A, 0xF8, 0x5C, 0xA9, 0x3F, 0xA2, 0x5C, 0x51, 0xA8, 0x74, 0x6B, 0xD5, 0x50, 0x2F, 0xD3, 0x71, 0xE7, 0x00, 0x33, 0x14, 0xA1, 0x9D, 0x4C, 0x8B, 0x57, 0x65, 0x02, 0xC2, 0xB5, 0x42, 0xDC, 0x04, 0xD9, 0x9A, 0x8D, 0xCB, 0x29, 0xC2, 0xAB, 0x58, 0x7A, 0x83, 0x55, 0x5A, 0x33, 0x0C, 0x4C, 0x66, 0xBB, 0xA8, 0x49, 0xDC, 0xB4, 0x9F};
  //U8 Private_Exponent[] = {0x34, 0xB7, 0xD8, 0x96, 0x6B, 0xB4, 0x5B, 0x13, 0x20, 0x3E, 0x92, 0x99, 0xFC, 0x26, 0x0D, 0xC7, 0x93, 0x2B, 0x24, 0x2E, 0x9B, 0x62, 0x4A, 0x5B, 0xD3, 0x96, 0xDD, 0x1F, 0xA9, 0x31, 0xF0, 0xD8, 0x16, 0xEC, 0x48, 0x31, 0x7F, 0x7B, 0xAE, 0x05, 0x97, 0xCD, 0xE9, 0x89, 0x37, 0x33, 0x88, 0x0B, 0x75, 0x8E, 0xD5, 0x52, 0xB9, 0x79, 0x30, 0x7B, 0xA1, 0x10, 0xA6, 0x7E, 0x43, 0x20, 0x03, 0xCC, 0xF2, 0x4A, 0x9B, 0x91, 0xD3, 0xFC, 0xA7, 0xAB, 0x9A, 0xB3, 0x2D, 0xDF, 0x96, 0x29, 0xB9, 0xB7, 0x69, 0x55, 0x12, 0x83, 0x91, 0xD3, 0xBD, 0x0E, 0xFC, 0x7C, 0x63, 0x08, 0x9B, 0x98, 0x0F, 0x8C, 0x40, 0x65, 0x2E, 0x68, 0xDE, 0x58, 0xD9, 0x8F, 0xC1, 0xB5, 0xEE, 0x38, 0xB3, 0x45, 0x23, 0xC4, 0x5C, 0x4B, 0x8D, 0xC6, 0x25, 0xD0, 0xAD, 0xB7, 0x80, 0x2E, 0x39, 0xC5, 0x4A, 0x01, 0x72, 0x0D, 0x98, 0x1C, 0x01, 0x8E, 0xE9, 0xC5, 0x7F, 0xC4, 0x01, 0x9F, 0xCA, 0x61, 0x5A, 0x12, 0x10, 0xE7, 0xCD, 0xF3, 0x76, 0x29, 0xE6, 0xEE, 0xA2, 0xD7, 0xCD, 0x45, 0x9E, 0x4A, 0x2B, 0x3C, 0xAE, 0x7F, 0xF5, 0x98, 0xB4, 0xCA, 0xC4, 0x77, 0x8E, 0x29, 0xC0, 0x24, 0xD0, 0x58, 0x67, 0xF4, 0xBB, 0xAA, 0xDB, 0x66, 0x28, 0xCB, 0x49, 0x77, 0x6D, 0x50, 0x81, 0xDD, 0x96, 0xD4, 0x37, 0x54, 0x30, 0x4B, 0x91, 0x5D, 0x0B, 0x49, 0xD7, 0xB5, 0xEB, 0x3A, 0x3A, 0x69, 0x65, 0x99, 0x62, 0x8A, 0xB1, 0xAA, 0x0B, 0xDE, 0x2B, 0x61, 0x7D, 0x91, 0xE2, 0xCF, 0xD3, 0x82, 0x6B, 0xD7, 0xD3, 0x49, 0xAA, 0xA4, 0x01, 0x96, 0xA5, 0x24, 0x18, 0x19, 0xF3, 0x5D, 0xF0, 0x75, 0x4A, 0x60, 0x53, 0x81, 0x8F, 0x4B, 0xAB, 0xA1, 0x3A, 0xAD, 0xAC, 0xA3, 0x63, 0x86, 0x24, 0x20, 0x3C, 0x5C, 0xEF, 0x0E, 0x8C, 0xB1};
  //U8 Modulus[] = {0xa2, 0x3a, 0xe9, 0x9f, 0x31, 0x9f, 0x9a, 0xb2, 0x3e, 0x85, 0xdc, 0x38, 0x62, 0x42, 0xcd, 0x06, 0x2d, 0x9f, 0xdf, 0xbf, 0x18, 0x07, 0x15, 0x71, 0x97, 0x20, 0x52, 0x8b, 0xf0, 0x95, 0xe6, 0x3c, 0x20, 0xc0, 0x56, 0x73, 0x67, 0x2e, 0x3a, 0x6a, 0x8e, 0xcb, 0x74, 0x31, 0x3a, 0x59, 0x20, 0xe2, 0x2a, 0x68, 0x2b, 0x67, 0x89, 0x02, 0xa4, 0xe2, 0x26, 0x87, 0xee, 0x8e, 0xc0, 0x9f, 0xbd, 0xfb, 0x2c, 0x22, 0x21, 0x88, 0xbb, 0x8c, 0x01, 0x75, 0xd9, 0x4e, 0x65, 0xfe, 0xa0, 0xf1, 0x6b, 0xd0, 0x82, 0xc9, 0x6c, 0xa7, 0x2f, 0x66, 0x06, 0x02, 0x93, 0x7a, 0x0b, 0x77, 0x1b, 0x5c, 0x07, 0x64, 0x23, 0x7c, 0x19, 0xdf, 0x97, 0x9d, 0xf8, 0x7e, 0xa3, 0xad, 0x56, 0x0e, 0x8e, 0x68, 0x0f, 0x6c, 0x9d, 0xf6, 0xff, 0xbf, 0x28, 0xf1, 0xf0, 0x9a, 0x2b, 0xf7, 0xfa, 0xce, 0x2d, 0x3e, 0x95, 0xe7, 0x15, 0x1a, 0x92, 0x36, 0xd5, 0x70, 0x72, 0xae, 0xee, 0x73, 0xb1, 0xec, 0x49, 0x52, 0x37, 0xdb, 0xa9, 0xa9, 0x0b, 0xb3, 0x60, 0xf1, 0xf5, 0xa4, 0x23, 0xdd, 0x40, 0x25, 0x6a, 0xb8, 0x59, 0x23, 0x7f, 0x14, 0xa2, 0x9d, 0x5a, 0x12, 0x1f, 0xd5, 0x98, 0x36, 0x4b, 0x73, 0x20, 0x70, 0x64, 0x5f, 0x1d, 0xbc, 0x78, 0x7b, 0x79, 0xc4, 0x26, 0xdd, 0x05, 0x86, 0x38, 0xfa, 0x2e, 0x71, 0xde, 0x90, 0x4b, 0xd6, 0x33, 0x02, 0x6d, 0xd0, 0xd2, 0x7e, 0x9c, 0xcb, 0x6f, 0x3d, 0xf4, 0xc8, 0xfb, 0x73, 0x22, 0x40, 0x69, 0x00, 0x2e, 0xc4, 0xc1, 0x20, 0xc1, 0x89, 0x94, 0xa1, 0x11, 0xfa, 0x59, 0x64, 0x36, 0x02, 0x84, 0x94, 0xbc, 0x7d, 0x43, 0x3b, 0xc9, 0x54, 0x78, 0xe6, 0x44, 0x96, 0x65, 0x82, 0x9c, 0x1c, 0x15, 0xd8, 0xa0, 0x82, 0x7f, 0xce, 0x6c, 0x40, 0x65, 0x77, 0x70, 0x5f, 0xf9, 0xbf};
  //U8 Private_Exponent[] = {0x1e, 0x67, 0x20, 0x05, 0x3f, 0x8d, 0x83, 0xb6, 0x47, 0xd5, 0x5a, 0x52, 0x0e, 0xc8, 0x2a, 0x18, 0x30, 0x50, 0xb7, 0xe3, 0xde, 0x22, 0xe5, 0xb7, 0xe5, 0xf4, 0x9e, 0xc3, 0xaf, 0x10, 0xc8, 0x97, 0x18, 0x24, 0x33, 0x8f, 0x69, 0x14, 0xd6, 0xc9, 0xc5, 0x7d, 0x07, 0x87, 0x7a, 0x41, 0xdd, 0xd1, 0xc4, 0xd4, 0xc4, 0xdd, 0xa0, 0x65, 0xa8, 0x74, 0xe1, 0x77, 0xb4, 0x13, 0x78, 0xb1, 0xf7, 0x2e, 0xad, 0x34, 0xb0, 0x48, 0x29, 0xad, 0xb1, 0x50, 0x5e, 0xab, 0x3c, 0x9e, 0x1d, 0x97, 0x60, 0xf7, 0x30, 0xab, 0x82, 0xfb, 0x49, 0xfc, 0xba, 0xbf, 0x9c, 0xd7, 0xd0, 0x72, 0x3f, 0xe4, 0x5d, 0x56, 0x3c, 0xf8, 0x32, 0xf9, 0x1e, 0x36, 0xf2, 0xac, 0xf9, 0x0b, 0xb1, 0x07, 0x05, 0x55, 0x27, 0x1e, 0xd9, 0xf3, 0xc2, 0x36, 0x16, 0xfc, 0x50, 0x74, 0xf4, 0x2c, 0x83, 0x0a, 0x44, 0x9c, 0x68, 0x07, 0x1a, 0xd3, 0xda, 0x09, 0xfd, 0x69, 0x4e, 0x53, 0xd7, 0x2b, 0x90, 0x34, 0xa2, 0x9e, 0x99, 0x7f, 0xc7, 0x64, 0x96, 0x49, 0xf2, 0x39, 0x0c, 0x4a, 0x75, 0x69, 0xe2, 0x9e, 0x65, 0x83, 0xa6, 0xf4, 0x3f, 0x17, 0x14, 0xa0, 0x21, 0xc3, 0x00, 0x74, 0xfb, 0xfc, 0xc4, 0xa0, 0xbd, 0xa6, 0x57, 0xac, 0xd2, 0x6c, 0xf6, 0xba, 0xbc, 0x16, 0xad, 0x25, 0xde, 0x36, 0xbe, 0x84, 0xee, 0x76, 0x19, 0x5e, 0xc8, 0x7f, 0x97, 0xea, 0x7f, 0xa1, 0x82, 0x5b, 0x5a, 0xd7, 0x43, 0xf4, 0xe7, 0x81, 0x7b, 0x28, 0xb4, 0x7f, 0x4e, 0x5a, 0x4d, 0xdb, 0x81, 0xa1, 0x88, 0xfa, 0xba, 0x36, 0x9e, 0x12, 0xb4, 0xb9, 0xce, 0xa4, 0xf0, 0x43, 0x1f, 0xe7, 0x37, 0x13, 0x2c, 0x3f, 0xd9, 0x15, 0x4f, 0xb6, 0x10, 0xa8, 0x59, 0x38, 0xe4, 0xde, 0x34, 0x9b, 0xcd, 0x19, 0xe5, 0x10, 0xdf, 0xeb, 0xfc, 0x74, 0x9d, 0xd1};
  //memcpy(&SEED[0], Seed, SEED_SIZE);
  
  if (cmox_initialize(NULL) == CMOX_INIT_SUCCESS)
  {
    cmox_rsa_construct(&Rsa_Ctx, CMOX_RSA_MATH_FUNCS, CMOX_MODEXP_PRIVATE, Working_Buffer, sizeof(Working_Buffer));
    if ((retval = cmox_rsa_setKey(&Rsa_Key, Modulus, sizeof(Modulus), Private_Exponent, sizeof(Private_Exponent))) == CMOX_RSA_SUCCESS);
    {
      retval = cmox_rsa_pkcs1v15_sign(&Rsa_Ctx,      
                                      &Rsa_Key,               
                                      SEED,       
                                      CMOX_RSA_PKCS1V15_HASH_SHA1,             
                                      Computed_Signature, pOutputMessageLength);
    }
  }
  
  memcpy(pOutputMessage, &Computed_Signature[0], 256);
  
  return(retval);
}

void getCSNrCSN(uint8_t * csn, uint8_t * CSN_rCSN) 
{
	uint32_t csnLength = 8;
	uint8_t  rCSN[8];
	uint32_t temp;
	
	for (int i = 0; i < csnLength; i++) 
	{
		temp = csn[i];
		rCSN[i] = (uint8_t) (~temp & 0xFF);
	}

	memcpy(CSN_rCSN, csn, 8);
	memcpy(&CSN_rCSN[8], rCSN, 8);
}

void mergeByte(int splitIndex, uint8_t * data1, uint8_t * data2, uint8_t * ret) 
{
	memcpy(ret, data1, splitIndex);
	memcpy(&ret[splitIndex], data2, splitIndex);
}

bool veryfiSign1( uint8_t *pSamRandomKey, uint8_t *pCardRandomKey,  uint8_t *pSign1)
{
  	uint8_t 	MasterKey[16] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00};
	uint8_t 	DefaultKey[16];
	uint8_t 	sessionKey[16];
	uint8_t 	csn[8];
	uint8_t 	CSN_rCSN[16];
	uint8_t 	CR_SR[16];
	uint8_t 	SR_CR[16];
	uint8_t 	mySign1[16];
	uint8_t 	sign2[16];
	uint8_t 	CSN_RCSN_XOR_CR_SR[16];
	uint32_t 	srLength = 8;
	uint32_t 	DefaultKeyLength;
	uint32_t 	sessionKeyLength;
	uint32_t 	sign2Length;
	uint32_t 	mySign1Length;
	uint32_t	temp;
	
	if( ReadCSNHSM(csn) == HSM_SUCCESS )
	{
		getCSNrCSN(csn, CSN_rCSN);
		mergeByte(srLength, pCardRandomKey, pSamRandomKey, CR_SR);
		mergeByte(srLength, pSamRandomKey, pCardRandomKey, SR_CR);
	}
	
	for (int i = 0; i < 16; i++) 
	{
		temp = CSN_rCSN[i] ^ CR_SR[i];
		CSN_RCSN_XOR_CR_SR[i] = (uint8_t) (temp & 0xFF);
	}
	
	getAESEncoding_ECB(CSN_rCSN, 16, MasterKey, AES256, DefaultKey, &DefaultKeyLength );
	getAESEncoding_ECB(CSN_RCSN_XOR_CR_SR, 16, DefaultKey, AES256, sessionKey, &sessionKeyLength );
	getAESEncoding_ECB(SR_CR, 16, sessionKey, AES256, sign2, &sign2Length );
	getAESEncoding_ECB(CR_SR, 16, sessionKey, AES256, mySign1, &mySign1Length );
	
	if ( memcmp(pSign1, mySign1, 16 ) == 0 ) return true;

	return false;
}
#endif

int16_t HSM_Version_Check( int *version )
{
	U8 mHSM_Ver[2] = {0x00, 0x00};
	U32 Version_state = 300;
	*version = 0;
  
	if( (Version_state = ActivationHSM()) == HSM_SUCCESS )
	{
		Version_state = ReadVersionHSM(mHSM_Ver); 
		if(Version_state == HSM_SUCCESS)
		{
			*version = ((mHSM_Ver[0]-0x30)*10) + (mHSM_Ver[1]-0x30);
		}
	}
  
	return Version_state; //Version_state = HSM_Version_Check((int*)&HSM_Version);
}
#if 1//tara critical function delete
int32_t VCI3_HSM_UPDATE( uint8_t *AuthKey )
{
  U32 UpdateSize = 0;
  U32 uiReturnCode = 300;
  U32 HSMFileSize = 0;
  U16 HSM_Current_Version = 0;
  U16 HSM_Latest_Version = 0;
  U8 ATR[HSM_ATR_LEN] = {0x00, };
  
  g_HSM_update_flag = HSM_NORMAL_MODE;
  g_HSM_ATR_Flag = HSM_UPDATE_MODE;
  uint8_t ATR_Coregate_NonOS_V1[16] = {0x3B, 0x1D, 0x96, 0x00, 0x31, 0x08, 0x47, 0x48, 0x01, 0x71, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00};
  uint8_t ATR_Coregate_NonOS_V2[16] = {0x3B, 0x1D, 0x96, 0x00, 0x31, 0x08, 0xC3, 0xEB, 0x90, 0x27, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00}; 
  
  //Set MutualAuth Value
  memcpy(&DK_ENC[0], &AuthKey[0], 24);
  memcpy(&DK_MAC[0], &AuthKey[24], 24);
  memcpy(&DK_DEK[0], &AuthKey[48], 24);
  
  //Version Check
  memcpy(gHSM_Info_Name, "HSM_Applet_V10.bin", 18);
  uiReturnCode = HSM_Version_Check((int*)&HSM_Current_Version);
  HSM_Latest_Version = HSM_File_VersionInfo();
  
#if 1
  if((HSM_Current_Version < HSM_Latest_Version) &&
     (HSM_Latest_Version != NULL) &&
       ((uiReturnCode == HSM_SUCCESS)||
        (uiReturnCode == HSM_NOT_RESPONSE))) //Removed condition required after HSM update (Version_state = )
#else //App Test
  if(1)
#endif
  {
    pHSMAck->mResult = 0;
    pHSMAck->mProgress = 0;
    pHSMAck->mSelftestMode = 0;
    
    se_close();
    se_open();
	uiReturnCode = BackToLoadHSM(ATR);
	
	if(!memcmp(ATR, ATR_Coregate_NonOS_V1, sizeof(ATR_Coregate_NonOS_V1))) memcpy(gHSM_Info_Name, "HSM_Applet_V10.bin", 18);
	else if(!memcmp(ATR, ATR_Coregate_NonOS_V2, sizeof(ATR_Coregate_NonOS_V2))) memcpy(gHSM_Info_Name, "HSM_Applet_V20.bin", 18);	
	else if(!memcmp(ATR+1, ATR_Coregate_NonOS_V1, sizeof(ATR_Coregate_NonOS_V1))) memcpy(gHSM_Info_Name, "HSM_Applet_V10.bin", 18);
    else if(!memcmp(ATR+1, ATR_Coregate_NonOS_V2, sizeof(ATR_Coregate_NonOS_V2))) memcpy(gHSM_Info_Name, "HSM_Applet_V20.bin", 18);	
	else 
    {
      pHSMAck->mResult = 1;
      GLogEE("\r\nHSM Back to load Fail (%d) !!!\r\n", uiReturnCode);
      return HSM_UNKNOWN_ERROR;
    }
	
	if(HSM_File_SizeInfo((int*)&HSMFileSize) && uiReturnCode == HSM_SUCCESS)
	{
      UpdateSize = HSMFileSize / 257;
      
      uiReturnCode = se_open();
      g_HSM_ATR_Flag = HSM_NORMAL_MODE;
      
      if(uiReturnCode == ISO7816_OK)
      {
        GLogN("\r\n===================================");
        GLogN("\r\nHSM UPDATE START !!!!!!!\r\n");
        GLogN("===================================\r\n");
        
        //HSM OS Update
        uiReturnCode = UpdateOSHSM(UpdateSize);
        
        if(uiReturnCode == HSM_SUCCESS)
        {
            //HSM ASK Master Update
            for(int AskSeq=0; AskSeq<4; AskSeq++)
            {
                GLogN("\r\n=====> ASK Update(%d)", AskSeq);
                pHSMAck->mProgress = 70 + (6*AskSeq);
                //ASK_DIAG_PV         0
                //ASK_REWORK_PV       1
                //ASK_DIAG_CV         2
                //ASK_REWORK_CV       3
                uiReturnCode = UpdateHSMASK(AskSeq);
                
                if(uiReturnCode != HSM_SUCCESS)
                {
                    break;
                }
                else{}
            }
              
            memcpy(gHSM_Info_Name, "HSM_mainApplet.bin", 18);
            if(HSM_File_SizeInfo((int*)&HSMFileSize) && uiReturnCode == HSM_SUCCESS)
            {
                //HSM Applet Update
                UpdateSize = HSMFileSize / 260;
                uiReturnCode = UpdateAppletHSM(UpdateSize);
            }
            else{}
        }
        else{}
        
        if(uiReturnCode == HSM_SUCCESS)
        {
          Print_HSM_Update_Result( HSM_SUCCESS );
        }
        else
        {
          Print_HSM_Update_Result( 1 );
        }
      }
      
      else
      {
        Print_HSM_Update_Result( 2 );
      }
      g_HSM_update_flag = HSM_NORMAL_MODE;
	}
    else
    {
      pHSMAck->mResult = 1;
      GLogEE("\r\nHSM file size check Fail !!!\r\n");
    }
  }
  else if(HSM_Current_Version >= HSM_Latest_Version)
  {
    GLogN("\r\nHSM is already the latest version !!!\r\n");
    pHSMAck->mResult = 0;
    pHSMAck->mProgress = 100;

    DeleteHSMFile();
  }
  else
  {
    GLogEE("\r\nHSM Version Check Fail (%d) !!!\r\n", uiReturnCode);
    pHSMAck->mResult = 1;
    pHSMAck->mProgress = 0;
  }
  
  Save_HSM_Status();
  HSM_Update_Ack( pHSMAck->mResult, pHSMAck->mProgress );
  
  return uiReturnCode;
}
#endif
int32_t DecryptAES128CTR(uint8_t *EncryptedData, uint32_t DataLength, uint8_t *InitVector, uint8_t Index, uint8_t *DecryptedData )
{
    /*Additional processing due to maximum byte limit for HSM decryption (Q_hyek)*/
    U32 DecSize = 0;
    U8 ctr[32] = {0x00, };
    U32 iv_Loc = 15;
    U32 cnt = 0;
    U32 uiReturnCode = 0;
    
    U8 input[500] = {0x00, };
    U8 output[500] = {0x00, };
    U8 iv[16] = {0x00, };
    
    memcpy(input, EncryptedData, DataLength);
    memcpy(iv, InitVector, 16);
    
    DecSize = DataLength / 16; 
    
    for(int i=0; i<=DecSize; i++)
    {
      memcpy(&ctr[0], iv, 16);
      memcpy(&ctr[16], &input[cnt], 16);
      uiReturnCode = DecryptAES128HSM(&ctr[0], &output[cnt], AES_CTR, Index, 32);
      
      while(1)
      {
        if(iv[iv_Loc] == 0xFF)
        {
          memset(&iv[iv_Loc], 0x00, 1);
          iv_Loc -= 1;
          //I have no idea when Every IV have 0xFF (Q_hyek)
        }
        
        else
        {
          memset(&iv[iv_Loc], iv[iv_Loc] + 0x01, 1);
          break;
        }
      }
      
      /*if(i == DecSize-1)
      {
        memset(&output[DataLength], 0x00, DataLength-(16*DecSize)); //Processing For Length
      }*/
      
      cnt += 16;
    }
    
    memcpy(DecryptedData, output, DataLength);
    
    return uiReturnCode;
}

#if 0 // Use Timer
void HSM_Update_Ack( void )
{
    U8 uchsmack[2] = {0x00, };
    
    if(pHSMAck->mResult == 1 || pHSMAck->mProgress == 100)
    {
        StopSWTimer( g_HSM_Timer );
        GLogN("\r\n Stop g_HSM_Timer");
    }
    
    uchsmack[0] = pHSMAck->mResult;
    uchsmack[1] = pHSMAck->mProgress;
      
    TransmitFunction_IT(PACKET_UART, uchsmack, 2, 0x121E);
    GLogN("\r\n --> HSM Update ACK !!! (Res : %d, Progress : %d)", pHSMAck->mResult, pHSMAck->mProgress);
}
#else
void HSM_Update_Ack( bool state, int progress )
{
    U8 uchsmack[2] = {0x00, };
    U8 ucsthsmack[3] = {0x00, };
    
    uchsmack[0] = state;
    uchsmack[1] = progress;
#if 1
	U8 data[2] = {0x00, 0x5D};
    U8 out[20] = {0x00, };
    U32 OutputMessageLength = 0;
#if 0          
    //Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
    getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ); 
#endif
	//Immediately after running "ASKAppletLoadHSM", run it once due to an error when using "getAESEncoding_ECB".
    for(int i=0; i<=10; i++)
    {
        if(getAESEncoding_ECB(&data[0], 2, g_ucAES256_Key, AES256, &out[0], &OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
        {
            if(getAESDecoding_ECB(out, OutputMessageLength, g_ucAES256_Key, AES256, out, &OutputMessageLength ) == CMOX_CIPHER_SUCCESS)
            {
                if((data[0] == out[0]) && (data[1] == out[1])) 
                {
                	GLogN("\r\n %02X %02X",data[0],data[1]);
                	break;
                }
				else
				{
					GLogN("#%d,i:%d",progress,i);
				}
            }
        }
		else
		{
			GLogN("^%d,i:%d",progress,i);
		}
    }
#endif
    /* USB/UART transports do not carry UUID in the wire format, so use the
     * plain TransmitFunction(). MQTT/WebSocket transports embed UUID in the
     * CDP wrapper, so we must pass the UUID cached by FL_Git_HSM_UpdateStart
     * to correlate this 0x121E ACK with the originating 0x121D session. */
    if( g_iUSBConnected == USBD_STATE_CONFIGURED )
    {
        if(pHSMAck->mSelftestMode == true)
        {
            memcpy(&ucsthsmack[1], uchsmack, sizeof(uchsmack));
            ucsthsmack[0] = 0x71;
            TransmitFunction(PACKET_USB, ucsthsmack, 3, 0x1301);
        }
        else TransmitFunction(PACKET_USB, uchsmack, 2, 0x121E);
    }
	else if( g_ucBtConnected == BT_SPP_CONNECT )
    {
        TransmitFunction(PACKET_UART, uchsmack, 2, 0x121E);
    }
	else if(g_mqtt_isconnected == true)
	{
		TransmitFunction_WithUUID(PACKET_MQTT, uchsmack, 2, 0x121E, &g_HSM_UpdateAck_UUID);
	}
    else
    {
        //GLogN("\r\n --> HSM Update !!! (Res : %d, Progress : %d) - Communication cut off", pHSMAck->mResult, pHSMAck->mProgress);
        GLogN("\r\n%d%dn", pHSMAck->mResult, pHSMAck->mProgress);
        return;
    }
    
    //GLogN("\r\n --> HSM Update ACK !!! (Res : %d, Progress : %d)", pHSMAck->mResult, pHSMAck->mProgress);
    GLogN("\r\n %d%d)", pHSMAck->mResult, pHSMAck->mProgress);
}
#endif

int32_t HSM_ASK_Applet_Update( uint8_t *AuthKey, bool UpdateObj, uint8_t ASKType )
{
#if 0 //It's a function that is no longer in use.
    U32 UpdateSize = 0;
    U32 uiReturnCode = 300;
    U32 HSMFileSize = 0;
    U16 HSM_Current_Version = 0;

    //Set MutualAuth Value
    memcpy(&DK_ENC[0], &AuthKey[0], 24);
    memcpy(&DK_MAC[0], &AuthKey[24], 24);
    memcpy(&DK_DEK[0], &AuthKey[48], 24);

    //Version Check
    uiReturnCode = HSM_Version_Check((int*)&HSM_Current_Version);
    
    if((HSM_Current_Version >= 11) && (uiReturnCode == HSM_SUCCESS))
    {
        g_HSM_update_flag = HSM_SEND_HSM_DATA;
        g_HSM_ATR_Flag = HSM_NORMAL_MODE;
    
        se_close();
        se_open();
        
        if( ASKType == ASK_MASTER ) memcpy(gHSM_Info_Name, "ASK_Master_App.bin", 18);
        else if( ASKType == ASK_REWORK ) memcpy(gHSM_Info_Name, "ASK_Master_RW1.bin", 18);
        else{GLogE("\r\nWrong ASK index number !!!"); return HSM_UNKNOWN_ERROR;}
        
        GLogN("\r\nDelete HSM Applet.....");
        uiReturnCode = DeleteAppletHSM();
        if(UpdateObj && (uiReturnCode == HSM_SUCCESS))
        {
            GLogN("\r\nDelete HSM ASK.....");
            uiReturnCode = ASKAppletDeleteHSM();
              
            if(HSM_File_SizeInfo((int*)&HSMFileSize) && uiReturnCode == HSM_SUCCESS)
            {
                //HSM ASK Master Update
                UpdateSize = HSMFileSize / 260;
                GLogN("\r\nUpload HSM ASK App.....");
                uiReturnCode = ASKAppletLoadHSM(UpdateSize);
                
                if( uiReturnCode != HSM_SUCCESS ) GLogE("\r\nFail ASKAppletLoadHSM !!! (%d)", uiReturnCode);
            }
            else GLogE("\r\nDelete HSM ASK Fail !!!");
        }
        else if(UpdateObj)
        {
            GLogE("\r\nDelete HSM Applet Fail !!!");
        }
        else{}
        
        memcpy(gHSM_Info_Name, "HSM_mainApplet.bin", 18);
        if(HSM_File_SizeInfo((int*)&HSMFileSize) && uiReturnCode == HSM_SUCCESS)
        {
            GLogN("\r\n Start HSM Applet Update !!!");
            //HSM Applet Update
            UpdateSize = HSMFileSize / 260;
            uiReturnCode = UpdateAppletHSM(UpdateSize);
            
            if( uiReturnCode != HSM_SUCCESS ) GLogE("\r\nFail UpdateAppletHSM !!! (%d)", uiReturnCode);
        }
        else{}
    }
    else
    {
        GLogN("HSM Version is lower than v11");
    }
    
    if(uiReturnCode == HSM_SUCCESS)
    {
        DeleteHSMFile();
    }
    
    g_HSM_update_flag = HSM_NORMAL_MODE;
    return uiReturnCode;
#else
    return HSM_INVALID_REQUEST;
#endif
}

void DeleteHSMFile( void )
{
#if 1
    //Delete security data
    f_unlink("HSM_Applet_V10.bin");
    f_unlink("HSM_Applet_V20.bin");
    f_unlink("ASK_Master_App.bin");
    f_unlink("HSM_mainApplet.bin");
    f_unlink("ASK_Master_RW1.bin");
    f_unlink("ASK_Master_DCV.bin");
    f_unlink("ASK_Master_DPV.bin");
    f_unlink("ASK_Master_RCV.bin");
    f_unlink("ASK_Master_RPV.bin");
#endif
}
#define HSM_DFU_TYPE_HSE_FW				0x01 // HSE Firmware
#define HSM_DFU_TYPE_APP_FW				0x02 // Application Firmware
void DeleteHSMFile_AT( uint8_t FwType )
{
#if 1
    //Delete security data
    if(FwType==HSM_DFU_TYPE_HSE_FW)
	{
		GLogN("delete security data1\r\n");
		f_unlink("AT_HSM_HSE.bin");
    }
	else if(FwType==HSM_DFU_TYPE_APP_FW)
	{
		GLogN("delete security data2\r\n");
		f_unlink("AT_HSM_App.bin");
	}
#endif
}


void Save_HSM_Status( void )
{
    FIL Filepnt;
    UINT uiWriteNum;
    U8 res=0;
    U32 temp = 0;
    
    res = f_chdir(DIR_ROOT);
    if (g_HSM_Type == HSM_TYPE_OLD)
    {
	    if(res == FR_OK)
	    {
	        if(Check_HSM_UpdateFailCount() == HSM_UPDATE_MAX_COUNT)
	        {
	            temp = HSM_UPDATE_FAIL_STATUS;
	        }
	        else
	        {
	            temp = check_se_open();
	        }

	        res|=f_unlink(HSM_STATUS_FILE_NAME);
	        res|=f_open(&Filepnt, HSM_STATUS_FILE_NAME, FA_CREATE_ALWAYS| FA_WRITE);
	        res|=f_truncate(&Filepnt);
	        res|=f_write(&Filepnt, &temp, sizeof(temp), &uiWriteNum);
	        res|=f_close(&Filepnt);
	        
	        GLogN("Save HSM Status(%d, %d)\r\n", temp, res);
	    }
	    else{GLogN("Fail HSM f_chdir(%d)\r\n", res);}
    }
	else
	{
        uint8_t serial_number[8];
		HAL_StatusTypeDef status;
#if 0
		if(Check_HSM_UpdateFailCount() == HSM_UPDATE_MAX_COUNT)
        {
            status = HSM_UPDATE_FAIL_STATUS;
        }
        else
#endif
        {
			for(int i=0; i<3; i++)
			{
		        status = hsm_get_serial_number(serial_number);
		        if (status == HAL_OK)
		        {
		        	//test_index = 0;
		            GLogN("HSM [OK]\r\n");
					break;
		        }
		        else
		        {
		        	osDelay(1000);
		        	//test_index = 1;
		            GLogE("[FAIL] Get HSN failed, status: %d\r\n", status);
		        }
			}
        }
		temp = status;
		res|=f_unlink(HSM_STATUS_FILE_NAME);
        res|=f_open(&Filepnt, HSM_STATUS_FILE_NAME, FA_CREATE_ALWAYS| FA_WRITE);
        res|=f_truncate(&Filepnt);
        res|=f_write(&Filepnt, &temp, sizeof(temp), &uiWriteNum);
        res|=f_close(&Filepnt);
		GLogN("Save HSM Status(%d, %d)\r\n", temp, res);
		
	}
}
void Save_HSM_Error_Status( U32 ErrorCode )
{
    FIL Filepnt;
    UINT uiWriteNum;
    U8 res=0;
    U32 temp = 0;
    
    res = f_chdir(DIR_ROOT);
    
    if(res == FR_OK)
    {
        temp = ErrorCode;
        
        res|=f_unlink(HSM_STATUS_FILE_NAME);
        res|=f_open(&Filepnt, HSM_STATUS_FILE_NAME, FA_CREATE_ALWAYS| FA_WRITE);
        res|=f_truncate(&Filepnt);
        res|=f_write(&Filepnt, &temp, sizeof(temp), &uiWriteNum);
        res|=f_close(&Filepnt);
        
        GLogN("\r\nSave HSM Status2(%d, %d)", temp, res);
    }
    else{GLogN("\r\nFail HSM f_chdir(%d)", res);}
}
uint32_t Read_HSM_Status( void )
{
    FIL Filepnt;
    U8 res=0;
    U32 ret = 0;
    U32 temp = 0;
    UINT uireadsize = 0;
    
    for(int i=0; i<2; i++)
    {
        if( (res = f_chdir(DIR_ROOT)) == FR_OK)
        {
            if( (res = f_open(&Filepnt, HSM_STATUS_FILE_NAME, FA_READ|FA_OPEN_EXISTING)) == FR_OK )
            {
                if( (res = f_lseek(&Filepnt, 0)) == FR_OK)
                {
                    res |= f_read(&Filepnt, &temp, sizeof(temp), &uireadsize);
                    res |= f_close(&Filepnt);
                    GLogN("HSM Read Status (%d, %d, %d)\r\n", temp, res, uireadsize);
                    
                    if( (uireadsize == sizeof(temp)) && (res == FR_OK) )
                    {
                        ret = temp;
                    }
                    else
                    {
                        ret = (res*10) + uireadsize;
                    }
                    
                    break;
                }
                else{GLogN("HSM Read Status Fail f_lseek (%d)\r\n", res); ret = res;}
            }
            else{GLogN("HSM Read Status Fail f_open (%d)\r\n", res); ret = res;}
        }
        else{GLogN("Fail HSM f_chdir(%d)\r\n", res); ret = res;}
        
        if(i == 0) 
        {
            Save_HSM_Status();
            f_close(&Filepnt);
        }
        else{}
    }
    
    return ret;
}

void Print_HSM_Update_Result( U32 result )
{
    if(result == HSM_SUCCESS)
    {
        g_HSM_update_flag = HSM_NORMAL_MODE;
          
        pHSMAck->mResult = 0;
        pHSMAck->mProgress = 100;

        DeleteHSMFile();

        GLogN("\r\n===================================");
        GLogN("\r\nHSM UPDATE SUCCESS !!!!!!!\r\n");
        GLogN("===================================\r\n");
    }
    else
    {
        pHSMAck->mResult = 1;
        GLogEE("\r\n===================================");
        GLogEE("\r\nHSM UPDATE FAIL(result) !!!!!!!\r\n");
        GLogEE("===================================\r\n");
    }
}

int32_t UpdateHSMASK( uint8_t ucASK_Num )
{
    U32 HSMFileSize = 0;
    U32 UpdateSize = 0;
    U32 uiReturnCode = 0;

    const char* fileNames[] = 
    {
        "ASK_Master_DPV.bin",  // ASK_DIAG_PV
        "ASK_Master_RPV.bin",  // ASK_REWORK_PV
        "ASK_Master_DCV.bin",  // ASK_DIAG_CV
        "ASK_Master_RCV.bin"   // ASK_REWORK_CV
    };
    
    int (*ASKFuncs[])(uint8_t) = 
    {
        ASKAppletLoadHSM_AID90,
        ASKAppletLoadHSM_AID92,
        ASKAppletLoadHSM_AID93,
        ASKAppletLoadHSM_AID94
    };
    
    uiReturnCode = HSM_UNKNOWN_ERROR;

    memcpy(gHSM_Info_Name, fileNames[ucASK_Num], strlen(fileNames[ucASK_Num]) + 1);

    if ( HSM_File_SizeInfo((int*)&HSMFileSize) ) 
    {
        UpdateSize = HSMFileSize / 260;
        uiReturnCode = ASKFuncs[ucASK_Num](UpdateSize);
    }

    return uiReturnCode;
}

void Save_HSM_UpdateFailCount( void )
{
    FIL Filepnt;
    UINT uiWriteNum;
    U8 res=0;
    UINT temp = 0;
    UINT wrtbuff = 0;
    UINT uireadsize = 0;
    
    res = f_chdir(DIR_ROOT);
    
    if(res == FR_OK)
    {
        //res|=f_unlink(HSM_UPDATE_FAIL_INFO);
        res|=f_open(&Filepnt, HSM_UPDATE_FAIL_INFO, FA_OPEN_ALWAYS| FA_WRITE | FA_READ);
        res|=f_lseek(&Filepnt, 0);
        res|=f_read(&Filepnt, &temp, 1, &uireadsize);
        wrtbuff = temp + 1;
        res|=f_lseek(&Filepnt, 0);
        res|=f_write(&Filepnt, &wrtbuff, 1, &uiWriteNum);
        res|=f_close(&Filepnt);
        
        GLogN("\r\nSave HSM Update Fail Count(%d, %d)", wrtbuff, res);
    }
    else{GLogE("\r\nFail Save_HSM_UpdateFailCount f_chdir(%d)", res);}
}

uint8_t Check_HSM_UpdateFailCount( void )
{
    FIL Filepnt;
    FILINFO fno;
    U8 res=0;
    UINT temp = 0;
    UINT uireadsize = 0;
    
    res = f_chdir(DIR_ROOT);
    
    if(res == FR_OK)
    {
        res = f_stat(HSM_UPDATE_FAIL_INFO, &fno);
        if(res == FR_OK)
        {
            res|=f_open(&Filepnt, HSM_UPDATE_FAIL_INFO, FA_READ);
            res|=f_lseek(&Filepnt, 0);
            res|=f_read(&Filepnt, &temp, 1, &uireadsize);
            res|=f_close(&Filepnt);
        
            GLogN("\r\nCheck HSM Update Fail Count(%d, %d)", temp, res);
            
            if(res != FR_OK)
            {
                temp = HSM_UPDATE_ERROR_NUM;
                GLogE("\r\nFail Check_HSM_UpdateFailCount res(%d)", res);
            }
            else{}
        }
        else
        {
            if(res!=FR_NO_FILE)
            {
                temp = HSM_UPDATE_ERROR_NUM;
                GLogE("\r\nCheck_HSM_UpdateFailCount f_stat(%d)", res);
            }
            else
            {
                temp = 0;
            }
        }
    }
    else
    {
        temp = HSM_UPDATE_ERROR_NUM;
        GLogE("\r\nFail Check_HSM_UpdateFailCount f_chdir(%d)", res);
    }
    
    return temp;
}
