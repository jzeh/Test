#ifndef __HAL_UART_DRIVER_H__
#define __HAL_UART_DRIVER_H__


/* Includes ------------------------------------------------------------------*/
#include "common.h"
#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "STM32F4xx_usart.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_usart.h"
#endif


/* Exported define -----------------------------------------------------------*/
// UART2 define ----------------------------------------------------------------
#define HAL_UART2                   USART2
#define UART2_TX_PIN                (GPIO_MODEM_TX)
#define UART2_RX_PIN                (GPIO_MODEM_RX)
#define UART2_CTS_PIN               (GPIO_MODEM_CTS)    //GPIO_MODEM_CTS
#define UART2_RTS_PIN               (GPIO_MODEM_RTS)   //GPIO_MODEM_RTS
#define UART2_TXBUF_LEN             (1024+256)
#define UART2_RXBUF_LEN             1800
//--------------------------------------------------------------------------------

// BLE
// UART3 define ----------------------------------------------------------------
#define HAL_UART3                   USART3
#define UART3_TX_PIN                (GPIO_BT_TX)   //GPIO_BT_TX
#define UART3_RX_PIN                (GPIO_BT_RX)   //GPIO_BT_RX
#define UART3_CTS_PIN               (GPIO_BT_CTS)   //GPIO_BT_CTS
#define UART3_RTS_PIN               (GPIO_BT_RTS)  //GPIO_BT_RTS
#define UART3_TXBUF_LEN             512
#define UART3_RXBUF_LEN             512
//--------------------------------------------------------------------------------

// GPS
// UART4 define ----------------------------------------------------------------
#define HAL_UART4                   UART4
#define UART4_TX_PIN                (GPIO_GPS_TX)
#define UART4_RX_PIN                (GPIO_GPS_RX)  //GPIO_GPS_RX
#define UART4_TXBUF_LEN             512
#define UART4_RXBUF_LEN             512
//--------------------------------------------------------------------------------

// UART7 define ----------------------------------------------------------------
#define HAL_UART7                   UART7
#define UART7_TX_PIN                (GPIO_DBG_TX)  //GPIO_DBG_TX
#define UART7_RX_PIN                (GPIO_DBG_RX)  //GPIO_DBG_RX
#define UART7_TXBUF_LEN     		512
#define UART7_RXBUF_LEN     		512
//--------------------------------------------------------------------------------

// UART8 define ----------------------------------------------------------------
#define HAL_UART8                   UART8
#define UART8_TX_PIN                (GPIO_INT_TX)   //GPIO_INT_TX
#define UART8_RX_PIN                (GPIO_INT_RX)   //GPIO_INT_RX
#define UART8_TXBUF_LEN     		512
#define UART8_RXBUF_LEN     		512
//--------------------------------------------------------------------------------


#define USBCDC_RXBUF_LEN			256
//---------------------------------------------------------------------------------

#define BAUDRATE_9600   (9600)
#define BAUDRATE_115200 (115200)
#define BAUDRATE_921600 (921600)


// @defgroup USART_Word_Length 
#define HAL_USART_WordLength_8b                  ((unsigned short)0x0000)
#define HAL_USART_WordLength_9b                  ((unsigned short)0x1000)
// @defgroup USART_Stop_Bits 
#define HAL_USART_StopBits_1                     ((unsigned short)0x0000)
#define HAL_USART_StopBits_0_5                   ((unsigned short)0x1000)
#define HAL_USART_StopBits_2                     ((unsigned short)0x2000)
#define HAL_USART_StopBits_1_5                   ((unsigned short)0x3000)
// @defgroup USART_Parity 
#define HAL_USART_Parity_No                      ((unsigned short)0x0000)
#define HAL_USART_Parity_Even                    ((unsigned short)0x0400)
#define HAL_USART_Parity_Odd                     ((unsigned short)0x0600) 
// @defgroup USART_Mode 
#define HAL_USART_Mode_Rx                        ((unsigned short)0x0004)
#define HAL_USART_Mode_Tx                        ((unsigned short)0x0008)
// @defgroup USART_Hardware_Flow_Control 
#define HAL_USART_HWFlowCtrl_None               ((unsigned short)0x0000)
#define HAL_USART_HWFlowCtrl_RTS                ((unsigned short)0x0100)
#define HAL_USART_HWFlowCtrl_CTS                ((unsigned short)0x0200)
#define HAL_USART_HWFlowCtrl_RTS_CTS            ((unsigned short)0x0300)

// @defgroup USART_Flags 
#define HAL_USART_FLAG_CTS                       ((uint16_t)0x0200)
#define HAL_USART_FLAG_LBD                       ((uint16_t)0x0100)
#define HAL_USART_FLAG_TXE                       ((uint16_t)0x0080)
#define HAL_USART_FLAG_TC                        ((uint16_t)0x0040)
#define HAL_USART_FLAG_RXNE                      ((uint16_t)0x0020)
#define HAL_USART_FLAG_IDLE                      ((uint16_t)0x0010)
#define HAL_USART_FLAG_ORE                       ((uint16_t)0x0008)
#define HAL_USART_FLAG_NE                        ((uint16_t)0x0004)
#define HAL_USART_FLAG_FE                        ((uint16_t)0x0002)
#define HAL_USART_FLAG_PE                        ((uint16_t)0x0001)

// @defgroup USART_Interrupt_definition 
#define HAL_USART_IT_PE                          ((uint16_t)0x0028)
#define HAL_USART_IT_TXE                         ((uint16_t)0x0727)
#define HAL_USART_IT_TC                          ((uint16_t)0x0626)
#define HAL_USART_IT_RXNE                        ((uint16_t)0x0525)
#define HAL_USART_IT_ORE_RX                      ((uint16_t)0x0325) /* In case interrupt is generated if the RXNEIE bit is set */
#define HAL_USART_IT_IDLE                        ((uint16_t)0x0424)
#define HAL_USART_IT_LBD                         ((uint16_t)0x0846)
#define HAL_USART_IT_CTS                         ((uint16_t)0x096A)
#define HAL_USART_IT_ERR                         ((uint16_t)0x0060)
#define HAL_USART_IT_ORE_ER                      ((uint16_t)0x0360) /* In case interrupt is generated if the EIE bit is set */
#define HAL_USART_IT_NE                          ((uint16_t)0x0260)
#define HAL_USART_IT_FE                          ((uint16_t)0x0160)

// @defgroup USART_Legacy 
#define HAL_USART_IT_ORE                          HAL_USART_IT_ORE_ER 

// @defgroup USART_DMA_Requests 
#define HAL_USART_DMAReq_Tx                      ((uint16_t)0x0080)
#define HAL_USART_DMAReq_Rx                      ((uint16_t)0x0040)

/* Exported types - Structure, Enumeration -----------------------------------*/
/*
typedef enum __eUartControl
{
    eUart2Disable = 0,
    eUart3Disable,
    eUart4Disable,
    eUart7Disable,
}eUartControl;
*/

typedef enum __eHalUartIoCtlMode{
    eUART_IO_Init,
    eUART_IO_DeInit,
    eUART_IO_Rx_ENABLE,
    eUART_IO_Tx_ENABLE,
    eUART_IO_INT_ENABLE,       // Interrrupt enable/disable
    eUART_IO_Port_ENABLE,
    eUART_IO_GetFlagStatus,
    eUART_IO_GetITStatus,
    eUART_IO_ClearITBit,
    eUART_IO_DMA_RTX_Enable,

}eHalUart_IOCtlMode;

typedef __packed struct __stHalUartBuffCtrl{
unsigned char *ptxbuf;
unsigned short tx_rdindex;
unsigned short tx_wrindex;
unsigned short txcnt;
unsigned short txbuf_size;

unsigned char *prxbuf;
unsigned short rx_rdindex;
unsigned short rx_wrindex;
unsigned short rx_bufcnt;
unsigned short rxbuf_size;

#if defined(STM32F427X) 
USART_TypeDef   *pUARTreg;
#elif defined(AT32F435VMT7)
usart_type      *pUARTreg;
#endif
}stHalUartBuffCtrl;

typedef __packed struct __stHalUSART_InitTypeDef
{
  uint32_t USART_BaudRate;            /*!< This member configures the USART communication baud rate.
                                           The baud rate is computed using the following formula:
                                            - IntegerDivider = ((PCLKx) / (8 * (OVR8+1) * (USART_InitStruct->USART_BaudRate)))
                                            - FractionalDivider = ((IntegerDivider - ((u32) IntegerDivider)) * 8 * (OVR8+1)) + 0.5 
                                           Where OVR8 is the "oversampling by 8 mode" configuration bit in the CR1 register. */

  uint16_t USART_WordLength;          /*!< Specifies the number of data bits transmitted or received in a frame.
                                           This parameter can be a value of @ref USART_Word_Length */

  uint16_t USART_StopBits;            /*!< Specifies the number of stop bits transmitted.
                                           This parameter can be a value of @ref USART_Stop_Bits */

  uint16_t USART_Parity;              /*!< Specifies the parity mode.
                                           This parameter can be a value of @ref USART_Parity
                                           @note When parity is enabled, the computed parity is inserted
                                                 at the MSB position of the transmitted data (9th bit when
                                                 the word length is set to 9 data bits; 8th bit when the
                                                 word length is set to 8 data bits). */
 
  uint16_t USART_Mode;                /*!< Specifies whether the Receive or Transmit mode is enabled or disabled.
                                           This parameter can be a value of @ref USART_Mode */

  uint16_t USART_HardwareFlowControl; /*!< Specifies wether the hardware flow control mode is enabled
                                           or disabled.
                                           This parameter can be a value of @ref USART_Hardware_Flow_Control */
}stHalUSART_InitTypeDef;


/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

int HalDrvUartOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvUartRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvUartWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvUartIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvUartClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);


eHalFlagStatus HalUartGetFlagStatus(unsigned int unUartAddr, unsigned int unUartFlags);
eHalFlagStatus HalUartGetITStatus(unsigned int unUartAddr, unsigned int unUartFlags);
eHalFlagStatus HalUartClearITBit(unsigned int unUartAddr, unsigned int unUartFlags);



void Uart_Init(void);
void Uart2PutCh(stHalUartBuffCtrl *pUart,u8 c);
void UartPutStr(stHalUartBuffCtrl *pUart,char* pdata);
void UartPrintf(stHalUartBuffCtrl *pUart,char *fmt,...);
s16 UartGetCh(stHalUartBuffCtrl *pUart);
u8 UartGetKey(stHalUartBuffCtrl *pUart);
u16 UartReadBuf(stHalUartBuffCtrl *pUart,u8*buf,u16 cnt);
void UartWriteBuf(stHalUartBuffCtrl *pUart,u8*buf, u16 size);
void Uart2WriteBuf(stHalUartBuffCtrl *pUart,u8*buf, u16 size);
void U1_Txpin_Set(u8 mode);
void U1_Rxpin_Set(u8 mode);
void U6_Rxpin_Set(u8 mode);
void UartRxTxbufClear(stHalUartBuffCtrl *pUart);
void UartTxRxbufClear(stHalUartBuffCtrl *pUart);
void HalUart_DebugUart_Init(void);
void ModemUart_Init(int32_t wBaudrate);
void BLEUart_Init(void);
void InitUSBCDCRxBuffer(void);
int16_t USBCDCRxGetCh(void);
void USBCDCRxPutCh(uint8_t data);
void USBCDCTxPutCh(uint8_t data);
void USBCDCTxGetData(void);
void UartModemTxService(void);
void HalUart_UartInitialize(void);
void SELFTESTUart_Init(unsigned int uiInput);


// UART define -----------------------------------------------------------------
// don't move Uart extern & define : compile error
extern stHalUartBuffCtrl g_stUart2;
extern stHalUartBuffCtrl g_stUart3;
extern stHalUartBuffCtrl g_stUart4;
extern stHalUartBuffCtrl g_stUart7;
extern stHalUartBuffCtrl g_stUart8;

#define UART_MODEM      &g_stUart2
#define UART_BLUETOOTH  &g_stUart3
#define UART_GPS        &g_stUart4
#if DEBUG_CH_UART7
#define UART_DEBUG      &g_stUart7
#else
#define UART_DEBUG      &g_stUart8
#endif

#endif //__HAL_UART_DRIVER_H__
