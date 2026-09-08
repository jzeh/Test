/*
  ******************************************************************************
  * @file    HalUartDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "STM32F4xx_usart.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "At32f435_437_usart.h"
#endif
#include "HalHandler.h"

#include "Autolink_Manager.h"
#include "Modem_Comm.h"
#include "UARTDMA_Manager.h"

#include "HdDebug.h"


/* Private typedef -----------------------------------------------------------*/
//#define TX_MODEM_CTS_CHECK_TIME     50
#define TX_MODEM_CTS_CHECK_TIME         3

/* Private define ------------------------------------------------------------*/
#define Trace(...)  GITDebug(DEBUG_MODULES_UART,__VA_ARGS__)


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
//***************************************************
// modem
//***************************************************
u8 Uart2_rxbuf[UART2_RXBUF_LEN];
u8 Uart2_txbuf[UART2_TXBUF_LEN];
stHalUartBuffCtrl g_stUart2;
//***************************************************

//***************************************************
// BLE
//***************************************************
u8 Uart3_rxbuf[UART3_RXBUF_LEN];
u8 Uart3_txbuf[UART3_TXBUF_LEN];
stHalUartBuffCtrl g_stUart3;
//***************************************************

//***************************************************
// GPS
//***************************************************
u8 Uart4_rxbuf[UART4_RXBUF_LEN];
u8 Uart4_txbuf[UART4_TXBUF_LEN];
stHalUartBuffCtrl g_stUart4;
//***************************************************

//***************************************************
// debug(예전에 확장 보드와 통신 하던 포트)
//***************************************************
u8 Uart8_rxbuf[UART8_RXBUF_LEN];
u8 Uart8_txbuf[UART8_TXBUF_LEN];
stHalUartBuffCtrl g_stUart8;

//***************************************************
// debug
//***************************************************
u8 Uart7_rxbuf[UART7_RXBUF_LEN];
u8 Uart7_txbuf[UART7_TXBUF_LEN];
stHalUartBuffCtrl g_stUart7;
//***************************************************

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
u8 USBCDC_rxbuf[USBCDC_RXBUF_LEN];
u8 USBCDC_txbuf[USBCDC_RXBUF_LEN];
stHalUartBuffCtrl g_stUSBCDC;
#endif

#if !defined(FEATURE_BOOTLOADER)
extern SYSTEM_TEST_INFO stSystemTestInfo;
#endif

int g_iTimerCheckModemCTSCallback = -1;

//--------------------------------------------------------------------------------

/* Private function prototypes -----------------------------------------------*/
void UART_ISR(stHalUartBuffCtrl *pUart);
eHalFlagStatus HalUartGetFlagStatus(unsigned int unUartAddr, unsigned int unUartFlags);
eHalFlagStatus HalUartGetITStatus(unsigned int unUartAddr, unsigned int unUartFlags);
eHalFlagStatus HalUartClearITBit(unsigned int unUartAddr, unsigned int unUartFlags);
eHalFlagStatus HalUartGetRxITEnable(stHalUartBuffCtrl *pUart);
void GPSUart_Init(void);
void HalUartCheckModemCTSCallback();



/* Private functions ---------------------------------------------------------*/
void HalUart_DebugUart_Init(void)
{
	stHalGPIO_InitTypeDef   GPIO_InitStructure;
	stHalUSART_InitTypeDef  USART_InitStructure;
	stHalNVIC_InitTypeDef   NVIC_InitStructure;

#ifdef DEBUG_CH_UART8
    memset(&g_stUart8, 0x0, sizeof(stHalUartBuffCtrl));
    g_stUart8.ptxbuf    = Uart8_txbuf;
    g_stUart8.txbuf_size= sizeof(Uart8_txbuf);
    g_stUart8.prxbuf    = Uart8_rxbuf;
    g_stUart8.rxbuf_size= sizeof(Uart8_rxbuf);
    g_stUart8.pUARTreg  = HAL_UART8;
	//*******************************************************************************************
	// UART8: debug 용도로 사용
	//*******************************************************************************************
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOE_GROUP, NULL, 0, HAL_ENABLE);
    HalDrvRccIOCtrl(eRCC_IO_UART_Clock, eRCC_Clock_Uart8, NULL, 0, HAL_ENABLE);

    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART8_TX_PIN, NULL, 0, HAL_GPIO_AF_UART8);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART8_RX_PIN, NULL, 0, HAL_GPIO_AF_UART8);

    GPIO_InitStructure.GPIO_DS      = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_OType   = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd    = eGPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Mode    = eGPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed   = eGPIO_Speed_100MHz;

	GPIO_InitStructure.GPIO_Pin = UART8_TX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_Pin = UART8_RX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	USART_InitStructure.USART_WordLength    = HAL_USART_WordLength_8b;
	USART_InitStructure.USART_StopBits      = HAL_USART_StopBits_1;
	USART_InitStructure.USART_Parity        = HAL_USART_Parity_No;
	USART_InitStructure.USART_Mode          = HAL_USART_Mode_Rx | HAL_USART_Mode_Tx;	
    USART_InitStructure.USART_BaudRate      = BAUDRATE_115200;
	USART_InitStructure.USART_HardwareFlowControl = HAL_USART_HWFlowCtrl_None;

    // 1. uart init
    HalDrvUartIOCtrl(eUART_IO_Init, (int)g_stUart8.pUARTreg, (char*)&USART_InitStructure, sizeof(USART_InitStructure), 0);

    // rx & tx enable
    HalDrvUartIOCtrl(eUART_IO_Tx_ENABLE, (int)g_stUart8.pUARTreg, NULL, 0, HAL_DISABLE);    
    HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)g_stUart8.pUARTreg, NULL, 0, HAL_DISABLE);
    // rx & tx interrupt enable
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart8.pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart8.pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart8.pUARTreg, NULL, 0, HAL_ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = HAL_UART8_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_Group_0_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_UART_DEBUG_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

    UartRxTxbufClear(&g_stUart8);

#else
    memset(&g_stUart7, 0x0, sizeof(stHalUartBuffCtrl));
    g_stUart7.ptxbuf    = Uart7_txbuf;
    g_stUart7.txbuf_size= sizeof(Uart7_txbuf);
    g_stUart7.prxbuf    = Uart7_rxbuf;
    g_stUart7.rxbuf_size= sizeof(Uart7_rxbuf);
    g_stUart7.pUARTreg  = HAL_UART7;

	//*******************************************************************************************
	// UART7: debug 용도로 사용
	//*******************************************************************************************
	// gpio CLOCK
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOE_GROUP, NULL, 0, HAL_ENABLE);
	// UART CLOCK
    HalDrvRccIOCtrl(eRCC_IO_UART_Clock, eRCC_Clock_Uart7, NULL, 0, HAL_ENABLE);
	// 매핑
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART7_TX_PIN, NULL, 0, HAL_GPIO_AF_UART7);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART7_RX_PIN, NULL, 0, HAL_GPIO_AF_UART7);

    GPIO_InitStructure.GPIO_DS      = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_OType   = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd    = eGPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Mode    = eGPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed   = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Pin     = UART7_TX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_Pin = UART7_RX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	USART_InitStructure.USART_WordLength    = HAL_USART_WordLength_8b;
	USART_InitStructure.USART_StopBits      = HAL_USART_StopBits_1;
	USART_InitStructure.USART_Parity        = HAL_USART_Parity_No;
	USART_InitStructure.USART_Mode          = HAL_USART_Mode_Rx | HAL_USART_Mode_Tx;	
    USART_InitStructure.USART_BaudRate      = BAUDRATE_115200;
	USART_InitStructure.USART_HardwareFlowControl = HAL_USART_HWFlowCtrl_None;
    HalDrvUartIOCtrl(eUART_IO_Init, (int)g_stUart7.pUARTreg, (char*)&USART_InitStructure, sizeof(USART_InitStructure), 0);

    HalDrvUartIOCtrl(eUART_IO_Tx_ENABLE, (int)g_stUart7.pUARTreg, NULL, 0, HAL_DISABLE);    
    HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)g_stUart7.pUARTreg, NULL, 0, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart7.pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart7.pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart7.pUARTreg, NULL, 0, HAL_ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = HAL_UART7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_Group_0_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_UART_DEBUG_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

    UartRxTxbufClear(&g_stUart7);
#endif
}

#if !defined(FEATURE_BOOTLOADER)
void HalUart_UartInitialize(void)
{
#if defined(FEATURE_USE_UART_RX_DMA)
    /* Enable the DMA clock */
    HalDrvRccIOCtrl(eRCC_IO_DMA_Clock, eRCC_Clock_DMA1, NULL, 0, HAL_ENABLE);
    HalDrvRccIOCtrl(eRCC_IO_DMA_Reset_Clock, eRCC_Clock_DMA1, NULL, 0, HAL_ENABLE);
#endif
	//****************************************************************************
	// UART2: MODEM
	//****************************************************************************
	// 2018.09.04 SPARROW : Modem baudrate 초기값 : 921600bps고정  //todo check the default of RF Tracker modem. kks.

#ifdef RF_COMMON_MODEM	 //mod.kks 21.10.25  need to set the initial value on the factory line.
	ModemUart_Init(BAUDRATE_115200);
	Trace(" Init UART2(115200bps)\n");
#else
	ModemUart_Init(BAUDRATE_921600);
     Trace(" Init UART2(921600bps)\n");
#endif

	//****************************************************************************

	//****************************************************************************
	// UART3: Bluetooth
	//****************************************************************************

	Trace("BLE: Init \n");
	BLEUart_Init();

	//****************************************************************************

	//****************************************************************************
	// UART4: GPS
	//****************************************************************************
	Trace("GPS: Init \n");
	GPSUart_Init();

	//****************************************************************************

	//****************************************************************************
	// UART8: YUJIN SELFTEST
	//****************************************************************************
	SELFTESTUart_Init(BAUDRATE_115200);
	//****************************************************************************
}

void SELFTESTUart_Init(unsigned int uiInput)
{
    stHalGPIO_InitTypeDef GPIO_InitStructure;
    stHalUSART_InitTypeDef USART_InitStructure;
	stHalNVIC_InitTypeDef NVIC_InitStructure;

	memset(&g_stUart8, 0x0, sizeof(stHalUartBuffCtrl));
	g_stUart8.ptxbuf    = Uart8_txbuf;
	g_stUart8.txbuf_size= sizeof(Uart8_txbuf);
	g_stUart8.prxbuf    = Uart8_rxbuf;
	g_stUart8.rxbuf_size= sizeof(Uart8_rxbuf);
    g_stUart8.pUARTreg  = HAL_UART8;

	//*******************************************************************************************
	// UART8: Selftest 용도로 사용
	//*******************************************************************************************
    // gpio CLOCK
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOE_GROUP, NULL, 0, HAL_ENABLE);
    // UART CLOCK
    HalDrvRccIOCtrl(eRCC_IO_UART_Clock, eRCC_Clock_Uart8, NULL, 0, HAL_ENABLE);
    // 매핑
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART8_TX_PIN, NULL, 0, HAL_GPIO_AF_UART8);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART8_RX_PIN, NULL, 0, HAL_GPIO_AF_UART8);


    GPIO_InitStructure.GPIO_DS      = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_OType   = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd    = eGPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Mode    = eGPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed   = eGPIO_Speed_100MHz;

    GPIO_InitStructure.GPIO_Pin = UART8_TX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_Pin = UART8_RX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	USART_InitStructure.USART_WordLength    = HAL_USART_WordLength_8b;
	USART_InitStructure.USART_StopBits      = HAL_USART_StopBits_1;
	USART_InitStructure.USART_Parity        = HAL_USART_Parity_No;
	USART_InitStructure.USART_Mode          = HAL_USART_Mode_Rx | HAL_USART_Mode_Tx;	
    USART_InitStructure.USART_BaudRate      = uiInput;
	USART_InitStructure.USART_HardwareFlowControl = HAL_USART_HWFlowCtrl_None;
    HalDrvUartIOCtrl(eUART_IO_Init, (int)g_stUart8.pUARTreg, (char*)&USART_InitStructure, sizeof(USART_InitStructure), 0);

    HalDrvUartIOCtrl(eUART_IO_Tx_ENABLE, (int)g_stUart8.pUARTreg, NULL, 0, HAL_ENABLE);    
    HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)g_stUart8.pUARTreg, NULL, 0, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart8.pUARTreg, NULL, HAL_USART_IT_TXE, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart8.pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart8.pUARTreg, NULL, 0, HAL_ENABLE);     

	NVIC_InitStructure.NVIC_IRQChannel = HAL_UART8_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_UART_NVIC_IRQChannelPreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_UART8_SELFTEST_IRQChannelSubPriority;
	NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

#if defined(FEATURE_USE_UART_RX_DMA)
    InitSELFTESTDMA();
#endif

    UartRxTxbufClear(&g_stUart8);
}


void ModemUart_Init(int32_t wBaudrate)
{
	stHalGPIO_InitTypeDef GPIO_InitStructure;
	stHalUSART_InitTypeDef USART_InitStructure;
	stHalNVIC_InitTypeDef NVIC_InitStructure;

	memset(&g_stUart2, 0x0, sizeof(stHalUartBuffCtrl));
	g_stUart2.ptxbuf = Uart2_txbuf;
	g_stUart2.txbuf_size = sizeof(Uart2_txbuf);
    g_stUart2.prxbuf = Uart2_rxbuf;
    g_stUart2.rxbuf_size = sizeof(Uart2_rxbuf);
	g_stUart2.pUARTreg = HAL_UART2;

    GPIO_InitStructure.GPIO_DS    = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;

	//****************************************************************************
	// UART2: MODEM
	//****************************************************************************
	// gpio CLOCK
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOD_GROUP, NULL, 0, HAL_ENABLE);
	// UART CLOCK
    HalDrvRccIOCtrl(eRCC_IO_UART_ResetClock, eRCC_Clock_Uart2, NULL, 0, HAL_ENABLE);
    HalDrvRccIOCtrl(eRCC_IO_UART_Clock, eRCC_Clock_Uart2, NULL, 0, HAL_ENABLE);

	GPIO_InitStructure.GPIO_Pin  = UART2_TX_PIN;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_Pin  = UART2_RX_PIN;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
  	// 매핑
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART2_TX_PIN, NULL, 0, HAL_GPIO_AF_USART2);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART2_RX_PIN, NULL, 0, HAL_GPIO_AF_USART2);

    stSystemTestInfo.eModemFlowControlType = eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS_GPIO;

	if(stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS) {
        HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART2_CTS_PIN, NULL, 0, HAL_GPIO_AF_USART2);
        HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART2_RTS_PIN, NULL, 0, HAL_GPIO_AF_USART2);

		GPIO_InitStructure.GPIO_Pin  = UART2_CTS_PIN;
		GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_NOPULL; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		GPIO_InitStructure.GPIO_Pin  = UART2_RTS_PIN;
		GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_NOPULL; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
	}
	else if(stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_CTS) {
        HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART2_CTS_PIN, NULL, 0, HAL_GPIO_AF_USART2);

		GPIO_InitStructure.GPIO_Pin  = UART2_CTS_PIN;
		GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_NOPULL; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
		GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Pin   = UART2_RTS_PIN;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		HalGPIOSetVaule(UART2_RTS_PIN, eBIT_RESET);
	}
	else if(stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS_GPIO) {
		GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
		GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Pin   = UART2_RTS_PIN;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
		GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_IN;
		GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_Pin   = UART2_CTS_PIN;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		HalGPIOSetVaule(UART2_RTS_PIN, eBIT_RESET);
	}
	else if(stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_NONE) {
		GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
		GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;

		GPIO_InitStructure.GPIO_Pin = UART2_CTS_PIN;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		GPIO_InitStructure.GPIO_Pin = UART2_RTS_PIN;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

		HalGPIOSetVaule(UART2_RTS_PIN, eBIT_RESET);
		HalGPIOSetVaule(UART2_CTS_PIN, eBIT_RESET);
	}

    HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);
	USART_InitStructure.USART_WordLength = HAL_USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = HAL_USART_StopBits_1;
	USART_InitStructure.USART_Parity = HAL_USART_Parity_No;
	USART_InitStructure.USART_Mode = HAL_USART_Mode_Rx | HAL_USART_Mode_Tx;
	USART_InitStructure.USART_BaudRate = wBaudrate;
	Trace(" baudrate - %d\n", wBaudrate);

	if(stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS) {
		Trace(" Flow control - rts + cts\n");
		USART_InitStructure.USART_HardwareFlowControl = HAL_USART_HWFlowCtrl_RTS_CTS;
	}
	else if(stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS_GPIO) {
		Trace(" Flow control - GPIO rts + cts\n");
		USART_InitStructure.USART_HardwareFlowControl = HAL_USART_HWFlowCtrl_None;
	}
	else if(stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_CTS) {
		Trace(" Flow control - only cts\n");
		USART_InitStructure.USART_HardwareFlowControl = HAL_USART_HWFlowCtrl_CTS;
	}
	else if(stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_NONE) {
		Trace(" Flow control - none\n");
		USART_InitStructure.USART_HardwareFlowControl = HAL_USART_HWFlowCtrl_None;
	}

    HalDrvUartIOCtrl(eUART_IO_Init, (int)g_stUart2.pUARTreg, (char*)&USART_InitStructure, sizeof(USART_InitStructure), 0);
    HalDrvUartIOCtrl(eUART_IO_Tx_ENABLE, (int)g_stUart2.pUARTreg, NULL, 0, HAL_ENABLE);    
    HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)g_stUart2.pUARTreg, NULL, 0, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart2.pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart2.pUARTreg, NULL, HAL_USART_IT_TXE, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart2.pUARTreg, NULL, 0, HAL_ENABLE);    
    //****************************************************************************
	NVIC_InitStructure.NVIC_IRQChannel = HAL_USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_UART_NVIC_IRQChannelPreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_UART2_MODEM_NVIC_IRQChannelSubPriority;
	NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);
    

#if defined(FEATURE_USE_UART_RX_DMA)
    InitModemDMA();
#endif


	UartRxTxbufClear(&g_stUart2);
}

void BLEUart_Init(void)
{
	stHalGPIO_InitTypeDef GPIO_InitStructure;
	stHalUSART_InitTypeDef USART_InitStructure;
	stHalNVIC_InitTypeDef NVIC_InitStructure;

    memset(&g_stUart3, 0x0, sizeof(stHalUartBuffCtrl));
    g_stUart3.ptxbuf = Uart3_txbuf;
    g_stUart3.txbuf_size=sizeof(Uart3_txbuf);
    g_stUart3.prxbuf = Uart3_rxbuf;
    g_stUart3.rxbuf_size = sizeof(Uart3_rxbuf);
    g_stUart3.pUARTreg = HAL_UART3;

    GPIO_InitStructure.GPIO_DS    = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_NOPULL; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;

	//****************************************************************************
	// UART3: BLE
	//****************************************************************************
	// gpio CLOCK
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOD_GROUP, NULL, 0, HAL_ENABLE);
	// UART CLOCK
    HalDrvRccIOCtrl(eRCC_IO_UART_Clock, eRCC_Clock_Uart3, NULL, 0, HAL_ENABLE);
	// 매핑
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART3_TX_PIN, NULL, 0, HAL_GPIO_AF_USART3);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART3_RX_PIN, NULL, 0, HAL_GPIO_AF_USART3);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART3_CTS_PIN, NULL, 0, HAL_GPIO_AF_USART3);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART3_RTS_PIN, NULL, 0, HAL_GPIO_AF_USART3);

	GPIO_InitStructure.GPIO_Pin = UART3_TX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_Pin = UART3_RX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_Pin = UART3_CTS_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_Pin = UART3_RTS_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);


	USART_InitStructure.USART_WordLength = HAL_USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = HAL_USART_StopBits_1;
	USART_InitStructure.USART_Parity = HAL_USART_Parity_No;
	USART_InitStructure.USART_Mode = HAL_USART_Mode_Rx | HAL_USART_Mode_Tx;
	USART_InitStructure.USART_BaudRate = BAUDRATE_115200;
	USART_InitStructure.USART_HardwareFlowControl = HAL_USART_HWFlowCtrl_RTS_CTS;
    HalDrvUartIOCtrl(eUART_IO_Init, (int)g_stUart3.pUARTreg, (char*)&USART_InitStructure, sizeof(USART_InitStructure), 0);

    HalDrvUartIOCtrl(eUART_IO_Tx_ENABLE, (int)g_stUart3.pUARTreg, NULL, 0, HAL_ENABLE);    
    HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)g_stUart3.pUARTreg, NULL, 0, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart3.pUARTreg, NULL, HAL_USART_IT_TXE, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart3.pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart3.pUARTreg, NULL, 0, HAL_ENABLE);  

	NVIC_InitStructure.NVIC_IRQChannel = HAL_USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_UART_NVIC_IRQChannelPreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_UART3_BLE_NVIC_IRQChannelSubPriority;
	NVIC_InitStructure.NVIC_IRQChannelEnable= HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

#if defined(FEATURE_USE_UART_RX_DMA)
    InitBLEDMA();
#endif

	UartRxTxbufClear(&g_stUart3);
}


void GPSUart_Init(void)
{
    stHalGPIO_InitTypeDef GPIO_InitStructure;
    stHalUSART_InitTypeDef USART_InitStructure;
	stHalNVIC_InitTypeDef NVIC_InitStructure;

	memset(&g_stUart4, 0x00, sizeof(stHalUartBuffCtrl));
	g_stUart4.ptxbuf    = Uart4_txbuf;
	g_stUart4.txbuf_size= sizeof(Uart4_txbuf);
	g_stUart4.prxbuf    = Uart4_rxbuf;
	g_stUart4.rxbuf_size= sizeof(Uart4_rxbuf);
    g_stUart4.pUARTreg  = HAL_UART4;
    //****************************************************************************
	// UART4: GPS
	//****************************************************************************
    // gpio CLOCK
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOC_GROUP, NULL, 0, HAL_ENABLE);
	// UART CLOCK
    HalDrvRccIOCtrl(eRCC_IO_UART_Clock, eRCC_Clock_Uart4, NULL, 0, HAL_ENABLE);
	// 매핑
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART4_TX_PIN, NULL, 0, HAL_GPIO_AF_UART4);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, UART4_RX_PIN, NULL, 0, HAL_GPIO_AF_UART4);

    GPIO_InitStructure.GPIO_DS    = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP; //eGPIO_PuPd_UP; //eGPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_Pin   = UART4_TX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    GPIO_InitStructure.GPIO_Pin = UART4_RX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    USART_InitStructure.USART_WordLength    = HAL_USART_WordLength_8b;
	USART_InitStructure.USART_StopBits      = HAL_USART_StopBits_1;
	USART_InitStructure.USART_Parity        = HAL_USART_Parity_No;
	USART_InitStructure.USART_Mode          = HAL_USART_Mode_Rx | HAL_USART_Mode_Tx;	
#ifdef RF_COMMON_MODEM //mod.kks 21.10.27
    USART_InitStructure.USART_BaudRate = BAUDRATE_115200;
#else
    USART_InitStructure.USART_BaudRate = BAUDRATE_9600;
#endif
	USART_InitStructure.USART_HardwareFlowControl = HAL_USART_HWFlowCtrl_None;
    HalDrvUartIOCtrl(eUART_IO_Init, (int)g_stUart4.pUARTreg, (char*)&USART_InitStructure, sizeof(USART_InitStructure), 0);
    
    HalDrvUartIOCtrl(eUART_IO_Tx_ENABLE, (int)g_stUart4.pUARTreg, NULL, 0, HAL_ENABLE);    
    HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)g_stUart4.pUARTreg, NULL, 0, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart4.pUARTreg, NULL, HAL_USART_IT_TXE, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)g_stUart4.pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart4.pUARTreg, NULL, 0, HAL_ENABLE);   

    NVIC_InitStructure.NVIC_IRQChannel = HAL_UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_UART_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_UART4_GPS_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

#if defined(FEATURE_USE_UART_RX_DMA)
    InitGPSDMA();
#endif

	UartRxTxbufClear(&g_stUart4);
}
#endif


void UartRxTxbufClear(stHalUartBuffCtrl *pUart)
{
#if defined(FEATURE_USE_UART_RX_DMA)
    // g_stUart7만 Interrup Rx수신함. // 나머지 Uart는 DMA 수신함.
    if ( pUart == &g_stUart7 )
#endif
    {
        HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)pUart->pUARTreg, NULL, 0, HAL_DISABLE);
        HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_DISABLE);
    }

    pUart->rx_bufcnt=0;
    pUart->rx_rdindex=0;
    pUart->rx_wrindex=0;
#if defined(FEATURE_USE_UART_RX_DMA)
    // g_stUart7만 Interrup Rx수신함. // 나머지 Uart는 DMA 수신함.
    if ( pUart == &g_stUart7 )
#endif
    {
        HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)pUart->pUARTreg, NULL, 0, HAL_ENABLE);
        HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_ENABLE);
    }

    // g_stUart7포함 모든 uart가 TX Interrup Tx함.
    HalDrvUartIOCtrl(eUART_IO_Tx_ENABLE, (int)pUart->pUARTreg, NULL, 0, HAL_DISABLE);    
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
    pUart->txcnt=0;
    pUart->tx_rdindex=0;
    pUart->tx_wrindex=0;

    HalDrvUartIOCtrl(eUART_IO_Tx_ENABLE, (int)pUart->pUARTreg, NULL, 0, HAL_ENABLE);    
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_ENABLE);
}

int fputc(int ch, FILE *f)
{
	stHalUartBuffCtrl *pDebugUart;
#ifdef DEBUG_CH_UART8
        pDebugUart = &g_stUart8;
#elif DEBUG_CH_UART7
#if defined(FEATURE_BOOTLOADER)
        pDebugUart = &g_stUart7;
#else
        if(g_bHYPERTECSelftestFlag == true) pDebugUart = &g_stUart8;
        else                                pDebugUart = &g_stUart7;
#endif
#endif

    UartWriteBuf(pDebugUart, (unsigned char*)&ch, sizeof(unsigned char));

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	if(gbUSBCDCConnected == true && gbEnableUSBCDC == true) {
		VCP_DataTxByte(ch);
		if(ch == '\n') {
			VCP_DataTxByte((uint8_t) '\r');
		}
	}
#endif				// #if !defined(FEATURE_BOOTLOADER)

	return ch;
}

void UartWriteBuf(stHalUartBuffCtrl *pUart, u8 *buf, u16 size)
{
    u16 tmp1;
    while(HalUartGetFlagStatus((int)pUart->pUARTreg, HAL_USART_FLAG_TC)== HAL_RESET);

    tmp1 = pUart->txbuf_size;

	for(; size > 0; size--) 
	{
        pUart->ptxbuf[pUart->tx_wrindex++] = *buf;
        tmp1 = pUart->txbuf_size;
        pUart->tx_wrindex %= tmp1;
        pUart->txcnt++;
		buf++;
    }
    HalDrvUartIOCtrl(eUART_IO_Tx_ENABLE, (int)pUart->pUARTreg, NULL, 0, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_ENABLE);
}

u8 UartGetKey(stHalUartBuffCtrl *pUart)
{
   while(!pUart->rx_bufcnt);

   return (u8)UartGetCh(pUart);
}

s16 UartGetCh(stHalUartBuffCtrl *pUart)
{
    u8 ret; 
    u16 tmp1;

    if ( !pUart->rx_bufcnt )        return(s16) -1;

#if 0 // 2022/10/07 Rx interrupt 제어 변경
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_DISABLE);
    HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)pUart->pUARTreg, NULL, 0, HAL_DISABLE);
#endif

    pUart->rx_bufcnt--; 
    ret = pUart->prxbuf[pUart->rx_rdindex++];
    tmp1 = pUart->rxbuf_size;
    pUart->rx_rdindex %= tmp1;
    
#if 0 // 2022/10/07 Rx interrupt 제어 변경
    HalDrvUartIOCtrl(eUART_IO_Rx_ENABLE, (int)pUart->pUARTreg, NULL, 0, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_RXNE, HAL_ENABLE);
#endif
	return (s16)ret;
}

u16 UartReadBuf(stHalUartBuffCtrl *pUart,u8*buf,u16 cnt)
{
	u16 size=0;
	s16 ret;

	for (; cnt > 0; cnt--) {
		ret = UartGetCh(pUart);
		if (ret < 0)
			break;

		buf[size++]=(u8)ret;
	}

	return size;
}


////////////////////////////////////////////////////////////////////////////////
//
//      UART ISR
//
////////////////////////////////////////////////////////////////////////////////
/*
void UART_ISR(stHalUartBuffCtrl *pUart)
{
    u16 tmp1;
    
    if ( (HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE) == HAL_SET) &&
          (HalUartGetRxITEnable(pUart) == HAL_SET) )// rx interrupt Enable Sate
    {
        tmp1 = HalDrvUartRead((int)pUart->pUARTreg, NULL, 0, 0, 0);

        pUart->prxbuf[pUart->rx_wrindex] = tmp1;
		tmp1 = pUart->rxbuf_size;
		pUart->rx_wrindex = (pUart->rx_wrindex+1) % tmp1;
		pUart->rx_bufcnt++;

		HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE);
    }

	if ( HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_TXE) == HAL_SET ) // tx interrupt
	{
		if (pUart->txcnt > 0)
		{
            HalDrvUartWrite((int)pUart->pUARTreg, 0, (char*)&pUart->ptxbuf[pUart->tx_rdindex], sizeof(unsigned char), 0);
            --pUart->txcnt;
            tmp1 = pUart->txbuf_size;
            pUart->tx_rdindex = (pUart->tx_rdindex + 1) % tmp1;
        }

		if (!pUart->txcnt)
            HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
	}

    if (HalUartGetITStatus((int)pUart->pUARTreg, HAL_USART_IT_ORE_ER))
    {
        //ovwer run err interrupt.
        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_ORE);
    } 
}
*/
void USART2_IRQHandler(void)
{
    u16 tmp1;
    stHalUartBuffCtrl *pUart = &g_stUart2;

#if !defined(FEATURE_USE_UART_RX_DMA)    
    if ( (HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE) == HAL_SET) &&
          (HalUartGetRxITEnable(pUart) == HAL_SET) )// rx interrupt Enable Sate
    {
        tmp1 = HalDrvUartRead((int)pUart->pUARTreg, NULL, 0, 0, 0);

        pUart->prxbuf[pUart->rx_wrindex] = tmp1;
		tmp1 = pUart->rxbuf_size;
		pUart->rx_wrindex = (pUart->rx_wrindex+1) % tmp1;
		pUart->rx_bufcnt++;

		HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE);
    }
#endif 

	if ( HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_TXE) == HAL_SET ) // tx interrupt
	{
		if (pUart->txcnt > 0)
		{
#if !defined(FEATURE_BOOTLOADER)
            if( (stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS_GPIO) )
            {
                if( HalGPIOGetStatus(UART2_CTS_PIN) == HAL_SET )
                {
                    HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
                    HalTimerStartSWTimer(g_iTimerCheckModemCTSCallback, eSWTimer_ONESHOT);
                    return;
                }
            }
#endif
            HalDrvUartWrite((int)pUart->pUARTreg, 0, (char*)&pUart->ptxbuf[pUart->tx_rdindex], sizeof(unsigned char), 0);
            --pUart->txcnt;
            tmp1 = pUart->txbuf_size;
            pUart->tx_rdindex = (pUart->tx_rdindex + 1) % tmp1;
        }

		if (!pUart->txcnt)
            HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
	}

    if (HalUartGetITStatus((int)pUart->pUARTreg, HAL_USART_IT_ORE_ER))
    {
        //ovwer run err interrupt.
        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_ORE);
    } 
}

void USART3_IRQHandler(void)
{
    u16 tmp1;
    stHalUartBuffCtrl *pUart = &g_stUart3;

#if !defined(FEATURE_USE_UART_RX_DMA)    
    if ( (HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE) == HAL_SET) &&
          (HalUartGetRxITEnable(pUart) == HAL_SET) )// rx interrupt Enable Sate
    {
        tmp1 = HalDrvUartRead((int)pUart->pUARTreg, NULL, 0, 0, 0);

        pUart->prxbuf[pUart->rx_wrindex] = tmp1;
        tmp1 = pUart->rxbuf_size;
        pUart->rx_wrindex = (pUart->rx_wrindex+1) % tmp1;
        pUart->rx_bufcnt++;

        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE);
    }
#endif
    if ( HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_TXE) == HAL_SET ) // tx interrupt
    {
        if (pUart->txcnt > 0)
        {
            HalDrvUartWrite((int)pUart->pUARTreg, 0, (char*)&pUart->ptxbuf[pUart->tx_rdindex], sizeof(unsigned char), 0);
            --pUart->txcnt;
            tmp1 = pUart->txbuf_size;
            pUart->tx_rdindex = (pUart->tx_rdindex + 1) % tmp1;
        }

        if (!pUart->txcnt)
            HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
    }

    if (HalUartGetITStatus((int)pUart->pUARTreg, HAL_USART_IT_ORE_ER))
    {
        //ovwer run err interrupt.
        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_ORE);
    }
}

void UART4_IRQHandler(void)
{
    u16 tmp1;
    stHalUartBuffCtrl *pUart = &g_stUart4;

#if !defined(FEATURE_USE_UART_RX_DMA)    
    if ( (HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE) == HAL_SET) &&
          (HalUartGetRxITEnable(pUart) == HAL_SET) )// rx interrupt Enable Sate
    {
        tmp1 = HalDrvUartRead((int)pUart->pUARTreg, NULL, 0, 0, 0);

        pUart->prxbuf[pUart->rx_wrindex] = tmp1;
        tmp1 = pUart->rxbuf_size;
        pUart->rx_wrindex = (pUart->rx_wrindex+1) % tmp1;
        pUart->rx_bufcnt++;

        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE);
    }
#endif
    if ( HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_TXE) == HAL_SET ) // tx interrupt
    {
        if (pUart->txcnt > 0)
        {
            HalDrvUartWrite((int)pUart->pUARTreg, 0, (char*)&pUart->ptxbuf[pUart->tx_rdindex], sizeof(unsigned char), 0);
            --pUart->txcnt;
            tmp1 = pUart->txbuf_size;
            pUart->tx_rdindex = (pUart->tx_rdindex + 1) % tmp1;
        }

        if (!pUart->txcnt)
            HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
    }

    if (HalUartGetITStatus((int)pUart->pUARTreg, HAL_USART_IT_ORE_ER))
    {
        //ovwer run err interrupt.
        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_ORE);
    }
}

void UART7_IRQHandler(void)
{
    u16 tmp1;
    stHalUartBuffCtrl *pUart = &g_stUart7;

    if ( (HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE) == HAL_SET) &&
          (HalUartGetRxITEnable(pUart) == HAL_SET) )// rx interrupt Enable Sate
    {
        tmp1 = HalDrvUartRead((int)pUart->pUARTreg, NULL, 0, 0, 0);

        pUart->prxbuf[pUart->rx_wrindex] = tmp1;
        tmp1 = pUart->rxbuf_size;
        pUart->rx_wrindex = (pUart->rx_wrindex+1) % tmp1;
        pUart->rx_bufcnt++;

        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE);
    }
    if ( HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_TXE) == HAL_SET ) // tx interrupt
    {
        if (pUart->txcnt > 0)
        {
            HalDrvUartWrite((int)pUart->pUARTreg, 0, (char*)&pUart->ptxbuf[pUart->tx_rdindex], sizeof(unsigned char), 0);
            --pUart->txcnt;
            tmp1 = pUart->txbuf_size;
            pUart->tx_rdindex = (pUart->tx_rdindex + 1) % tmp1;
        }

        if (!pUart->txcnt)
            HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
    }

    if (HalUartGetITStatus((int)pUart->pUARTreg, HAL_USART_IT_ORE_ER))
    {
        //ovwer run err interrupt.
        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_ORE);
    }
}

void UART8_IRQHandler(void)
{
    u16 tmp1;
    stHalUartBuffCtrl *pUart = &g_stUart8;

#if !defined(FEATURE_USE_UART_RX_DMA)    
    if ( (HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE) == HAL_SET) &&
          (HalUartGetRxITEnable(pUart) == HAL_SET) )// rx interrupt Enable Sate
    {
        tmp1 = HalDrvUartRead((int)pUart->pUARTreg, NULL, 0, 0, 0);

        pUart->prxbuf[pUart->rx_wrindex] = tmp1;
        tmp1 = pUart->rxbuf_size;
        pUart->rx_wrindex = (pUart->rx_wrindex+1) % tmp1;
        pUart->rx_bufcnt++;

        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_RXNE);
    }
#endif
    if ( HalUartGetITStatus((unsigned int)pUart->pUARTreg, HAL_USART_IT_TXE) == HAL_SET ) // tx interrupt
    {
        if (pUart->txcnt > 0)
        {
            HalDrvUartWrite((int)pUart->pUARTreg, 0, (char*)&pUart->ptxbuf[pUart->tx_rdindex], sizeof(unsigned char), 0);
            --pUart->txcnt;
            tmp1 = pUart->txbuf_size;
            pUart->tx_rdindex = (pUart->tx_rdindex + 1) % tmp1;
        }

        if (!pUart->txcnt)
            HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)pUart->pUARTreg, NULL, HAL_USART_IT_TXE, HAL_DISABLE);
    }

    if (HalUartGetITStatus((int)pUart->pUARTreg, HAL_USART_IT_ORE_ER))
    {
        //ovwer run err interrupt.
        HalUartClearITBit((unsigned int)pUart->pUARTreg, HAL_USART_IT_ORE);
    }

}

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
void InitUSBCDCRxBuffer(void)
{
	Trace("Init USB CDC Rx Buffer\n");

	memset(&g_stUSBCDC, 0x0, sizeof(stHalUartBuffCtrl));
	g_stUSBCDC.ptxbuf = USBCDC_txbuf;
	g_stUSBCDC.txbuf_size = sizeof(USBCDC_txbuf);
	g_stUSBCDC.prxbuf = USBCDC_rxbuf;
	g_stUSBCDC.rxbuf_size = sizeof(USBCDC_rxbuf);
}

int16_t USBCDCRxGetCh(void)
{
	uint8_t ret;
	uint16_t tmp1;

	if ( !g_stUSBCDC.rx_bufcnt )
		return(int16_t) -1;

	ret = g_stUSBCDC.prxbuf[g_stUSBCDC.rx_rdindex];

	tmp1 = g_stUSBCDC.rxbuf_size;
	g_stUSBCDC.rx_rdindex = (g_stUSBCDC.rx_rdindex + 1) % tmp1;
	g_stUSBCDC.rx_bufcnt--;

	return (int16_t)ret;
}

void USBCDCRxPutCh(uint8_t data)
{
	uint16_t tmp1;

	g_stUSBCDC.prxbuf[g_stUSBCDC.rx_wrindex] = data;

	tmp1 = g_stUSBCDC.rxbuf_size;
	g_stUSBCDC.rx_wrindex = (g_stUSBCDC.rx_wrindex+1) % tmp1;

	g_stUSBCDC.rx_bufcnt++;
}

void USBCDCTxPutCh(uint8_t data)
{
	uint16_t tmp1;

	tmp1 = g_stUSBCDC.txbuf_size;
	while(g_stUSBCDC.txcnt == tmp1)
	{
		tmp1 = g_stUSBCDC.txbuf_size;
	}

	g_stUSBCDC.ptxbuf[g_stUSBCDC.tx_wrindex] = data;
	++g_stUSBCDC.tx_wrindex;
	tmp1 = g_stUSBCDC.txbuf_size;
	g_stUSBCDC.tx_wrindex %= tmp1;

	++g_stUSBCDC.txcnt;
}

void USBCDCTxGetData(void)
{
	uint16_t tmp1;

	while(1) {
		if (g_stUSBCDC.txcnt) {
			--g_stUSBCDC.txcnt;

			VCP_DataTxByte(g_stUSBCDC.ptxbuf[g_stUSBCDC.tx_rdindex]);

			tmp1 = g_stUSBCDC.txbuf_size;
			g_stUSBCDC.tx_rdindex = (g_stUSBCDC.tx_rdindex + 1) % tmp1;
		}
		else {
			break;
		}
	}
}
#endif //defined(FEATURE_USE_USB_DRIVE)



eHalFlagStatus HalUartGetFlagStatus(unsigned int unUartAddr, unsigned int unUartFlags)
{
    return (eHalFlagStatus)HalDrvUartIOCtrl(eUART_IO_GetFlagStatus, unUartAddr, NULL, 0, unUartFlags);
}

eHalFlagStatus HalUartGetITStatus(unsigned int unUartAddr, unsigned int unUartFlags)
{
    return (eHalFlagStatus)HalDrvUartIOCtrl(eUART_IO_GetITStatus, unUartAddr, NULL, 0, unUartFlags);
}

eHalFlagStatus HalUartClearITBit(unsigned int unUartAddr, unsigned int unUartFlags)
{
    return (eHalFlagStatus)HalDrvUartIOCtrl(eUART_IO_ClearITBit, unUartAddr, NULL, 0, unUartFlags);
}

eHalFlagStatus HalUartGetRxITEnable(stHalUartBuffCtrl *pUart)
{
#if defined(STM32F427X)
#elif defined(AT32F435VMT7)
    if ( pUart->pUARTreg->ctrl1_bit.rdbfien == 0 )
        return HAL_RESET;
#endif
    return HAL_SET;
}


void HalUartCheckModemCTSCallback()
{
	if( g_iTimerCheckModemCTSCallback != -1 )
	{
        if( HalGPIOGetStatus(UART2_CTS_PIN) == HAL_SET )
        {
		    HalTimerStartSWTimer(g_iTimerCheckModemCTSCallback, eSWTimer_ONESHOT);
        }
        else 
        {
            HalTimerStopSWTimer(g_iTimerCheckModemCTSCallback);
            HalDrvUartIOCtrl(eUART_IO_INT_ENABLE, (int)USART2, NULL, HAL_USART_IT_TXE, HAL_ENABLE);
        }
	}
}

//-----------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------//
int HalDrvUartOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    HalUart_DebugUart_Init();
#if !defined(FEATURE_BOOTLOADER)
    HalUart_UartInitialize();
#endif

    g_iTimerCheckModemCTSCallback = HalTimerSetSWTimer(TX_MODEM_CTS_CHECK_TIME, eSWTimer_ONESHOT, HalUartCheckModemCTSCallback, FALSE);
    return HAL_RETURN_SUCCESS;
}

int HalDrvUartRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
#if defined(STM32F427X)
    return USART_ReceiveData((USART_TypeDef*)nLparam);
#elif defined(AT32F435VMT7)
    return usart_data_receive((usart_type*)nLparam);
#endif
}

int HalDrvUartWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
   unsigned short usData = (unsigned short)*pBuffer;

#if defined(STM32F427X)
    USART_SendData((USART_TypeDef*)nLparam, usData);
#elif defined(AT32F435VMT7)
    usart_data_transmit((usart_type*)nLparam, usData);
#endif
    return HAL_RETURN_SUCCESS;
}

int HalDrvUartIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    eHalUart_IOCtlMode nIoCtlMode = (eHalUart_IOCtlMode)nLparam;
    unsigned int unUartTypeAddr = nRparam;
    
    switch( nIoCtlMode )
    {
    case eUART_IO_Init:
        {
            stHalUSART_InitTypeDef* pHalUartInitType = (stHalUSART_InitTypeDef*)pBuffer;
#if defined(STM32F427X) //2/3/7/8
            USART_InitTypeDef stUartInitType;
            stUartInitType.USART_BaudRate       = pHalUartInitType->USART_BaudRate;
            stUartInitType.USART_WordLength         = pHalUartInitType->USART_WordLength;
            stUartInitType.USART_StopBits           = pHalUartInitType->USART_StopBits;
            stUartInitType.USART_Parity             = pHalUartInitType->USART_Parity;
            stUartInitType.USART_Mode               = pHalUartInitType->USART_Mode;
            stUartInitType.USART_HardwareFlowControl= pHalUartInitType->USART_HardwareFlowControl;

            USART_OverSampling8Cmd((USART_TypeDef*)unUartTypeAddr, ENABLE);
            USART_Init((USART_TypeDef*)unUartTypeAddr, &stUartInitType);
#elif defined(AT32F435VMT7)
            usart_parity_selection_type PairtyType = USART_PARITY_NONE;
            usart_data_bit_num_type DataBitNum = USART_DATA_8BITS;
            usart_stop_bit_num_type StopBitNum = USART_STOP_1_BIT;
            usart_hardware_flow_control_type UartFlowCtrl = USART_HARDWARE_FLOW_NONE;

            if      ( pHalUartInitType->USART_WordLength == HAL_USART_WordLength_8b )
                DataBitNum = USART_DATA_8BITS;
            else if ( pHalUartInitType->USART_WordLength == HAL_USART_WordLength_9b )
                DataBitNum = USART_DATA_9BITS;            

            if      ( pHalUartInitType->USART_StopBits == HAL_USART_StopBits_1 )
                StopBitNum = USART_STOP_1_BIT;
            else if ( pHalUartInitType->USART_StopBits == HAL_USART_StopBits_0_5 )
                StopBitNum = USART_STOP_0_5_BIT;            
            else if ( pHalUartInitType->USART_StopBits == HAL_USART_StopBits_2 )
                StopBitNum = USART_STOP_2_BIT;            
            else if ( pHalUartInitType->USART_StopBits == HAL_USART_StopBits_1_5 )
                StopBitNum = USART_STOP_1_5_BIT;            

            usart_init((usart_type*)unUartTypeAddr, 
                            pHalUartInitType->USART_BaudRate, DataBitNum, StopBitNum);

            if ( pHalUartInitType->USART_Parity == HAL_USART_Parity_No )
                PairtyType = USART_PARITY_NONE;
            else if ( pHalUartInitType->USART_Parity == HAL_USART_Parity_Even )
                PairtyType = USART_PARITY_EVEN;
            else if ( pHalUartInitType->USART_Parity == HAL_USART_Parity_Odd )
                PairtyType = USART_PARITY_ODD;
            usart_parity_selection_config((usart_type*)unUartTypeAddr, PairtyType);

            if  ( pHalUartInitType->USART_HardwareFlowControl == HAL_USART_HWFlowCtrl_None )
                UartFlowCtrl = USART_HARDWARE_FLOW_NONE;
            else if ( pHalUartInitType->USART_HardwareFlowControl == HAL_USART_HWFlowCtrl_RTS )
                UartFlowCtrl = USART_HARDWARE_FLOW_RTS;
            else if ( pHalUartInitType->USART_HardwareFlowControl == HAL_USART_HWFlowCtrl_CTS )
                UartFlowCtrl = USART_HARDWARE_FLOW_CTS;
            else if ( pHalUartInitType->USART_HardwareFlowControl == HAL_USART_HWFlowCtrl_RTS_CTS )
                UartFlowCtrl = USART_HARDWARE_FLOW_RTS_CTS;
                
            usart_hardware_flow_control_set((usart_type*)unUartTypeAddr, UartFlowCtrl);
#endif       
        }
        break;
    case eUART_IO_DeInit:
#if defined(STM32F427X) 
        USART_Cmd((USART_TypeDef*)unUartTypeAddr, DISABLE);
        USART_DeInit((USART_TypeDef*)unUartTypeAddr);
#elif defined(AT32F435VMT7)
        usart_enable((usart_type*)unUartTypeAddr, FALSE);
        usart_reset((usart_type*)unUartTypeAddr);
#endif
        break;
    case eUART_IO_Rx_ENABLE:
#if defined(STM32F427X)
#elif defined(AT32F435VMT7)
        usart_receiver_enable((usart_type*)unUartTypeAddr, (confirm_state)nOverlap);
#endif       
        break;
    case eUART_IO_Tx_ENABLE:
#if defined(STM32F427X)
#elif defined(AT32F435VMT7)
        usart_transmitter_enable((usart_type*)unUartTypeAddr, (confirm_state)nOverlap);
#endif       
        break;
    case eUART_IO_INT_ENABLE:
    {
        unsigned int unITFlag = (unsigned int)nLength;
#if defined(STM32F427X)
        USART_ITConfig((USART_TypeDef*)unUartTypeAddr, unITFlag, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
        if      ( unITFlag == HAL_USART_IT_RXNE ) unITFlag = USART_RDBF_INT;
        else if ( unITFlag == HAL_USART_IT_TXE  ) unITFlag = USART_TDBE_INT;
        usart_interrupt_enable((usart_type*)unUartTypeAddr, unITFlag, (confirm_state)nOverlap);
#endif
    }
        break;
        
    case eUART_IO_Port_ENABLE:
#if defined(STM32F427X) //2/3/7/8
        USART_Cmd((USART_TypeDef*)unUartTypeAddr, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
        usart_enable((usart_type*)unUartTypeAddr, (confirm_state)nOverlap);  
#endif  
        break;
    case eUART_IO_GetFlagStatus:
    {
        int nUartFlag = nOverlap;
#if defined(STM32F427X)   
        return USART_GetFlagStatus((USART_TypeDef*)unUartTypeAddr, nUartFlag);
#elif defined(AT32F435VMT7)
        return usart_flag_get((usart_type*)unUartTypeAddr, nUartFlag);
#endif       
        }        
        break;
    case eUART_IO_GetITStatus:
        {
        int nUartITStatus = nOverlap;
#if defined(STM32F427X)    
        return USART_GetITStatus((USART_TypeDef*)unUartTypeAddr, nUartITStatus);
#elif defined(AT32F435VMT7)
        if      ( nUartITStatus == HAL_USART_IT_PE      ) nUartITStatus = USART_PERR_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_TXE     ) nUartITStatus = USART_TDBE_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_TC      ) nUartITStatus = USART_TDC_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_RXNE    ) nUartITStatus = USART_RDBF_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_IDLE    ) nUartITStatus = USART_IDLEF_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_CTS     ) nUartITStatus = USART_CTSCF_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_FE      ) nUartITStatus = USART_BFF_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_ORE_RX  ) nUartITStatus = USART_ROERR_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_ORE_ER  ) nUartITStatus = USART_ROERR_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_NE      ) nUartITStatus = USART_NERR_FLAG;
//        else if ( nUartITStatus == HAL_USART_IT_LBD     ) nUartITStatus = USART_ERR_INT;

        return usart_flag_get((usart_type*)unUartTypeAddr, nUartITStatus);
#endif       
        }
        break;
    
    case eUART_IO_ClearITBit:
        {
        int nUartITStatus = nOverlap;
#if defined(STM32F427X)  


        USART_ClearITPendingBit((USART_TypeDef*)unUartTypeAddr, nUartITStatus);
#elif defined(AT32F435VMT7)
        if      ( nUartITStatus == HAL_USART_IT_PE      ) nUartITStatus = USART_PERR_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_TXE     ) nUartITStatus = USART_TDBE_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_TC      ) nUartITStatus = USART_TDC_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_RXNE    ) nUartITStatus = USART_RDBF_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_IDLE    ) nUartITStatus = USART_IDLEF_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_CTS     ) nUartITStatus = USART_CTSCF_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_FE      ) nUartITStatus = USART_BFF_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_ORE_RX  ) nUartITStatus = USART_ROERR_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_ORE_ER  ) nUartITStatus = USART_ROERR_FLAG;
        else if ( nUartITStatus == HAL_USART_IT_NE      ) nUartITStatus = USART_NERR_FLAG;
//        else if ( nUartITStatus == HAL_USART_IT_LBD     ) nUartITStatus = USART_ERR_INT;

        usart_flag_clear((usart_type*)unUartTypeAddr, nUartITStatus);
#endif
        }
        break;    
    case eUART_IO_DMA_RTX_Enable:
    {
        int nDMAReq = nLength;

#if defined(STM32F427X)
        USART_DMACmd((USART_TypeDef*)nRparam, nDMAReq, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
        if ( nDMAReq == HAL_USART_DMAReq_Rx )
            usart_dma_receiver_enable((usart_type*)nRparam, (confirm_state)nOverlap);
        else if( nDMAReq == HAL_USART_DMAReq_Tx )
            usart_dma_transmitter_enable((usart_type*)nRparam, (confirm_state)nOverlap);
#endif
    }
        break;
    default:
        break;
    }
    
    return HAL_RETURN_SUCCESS;
}

int HalDrvUartClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
#ifdef RF_COMMON_MODEM
		if(stSystemTestInfo.eModemFlowControlType == eMODEM_UART_FLOWCONTROL_TYPE_RTS_CTS)
		{
			stHalGPIO_InitTypeDef GPIO_InitStructure;
			
			GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
			GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
			GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_OUT;
			GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Pin   = UART2_RTS_PIN;
			HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
	
			GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
			GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP; //GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
			GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_IN;
			GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
			GPIO_InitStructure.GPIO_Pin   = UART2_CTS_PIN;
			HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
		}
		HalGPIOSetVaule(UART2_RTS_PIN, eBIT_SET);
		HalGPIOSetVaule(UART2_CTS_PIN, eBIT_SET);
#endif
		
		HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart2.pUARTreg, NULL, 0, HAL_DISABLE);
		HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart3.pUARTreg, NULL, 0, HAL_DISABLE);
		HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart4.pUARTreg, NULL, 0, HAL_DISABLE);
		HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart7.pUARTreg, NULL, 0, HAL_DISABLE);
		HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart8.pUARTreg, NULL, 0, HAL_DISABLE);
	
		HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart2.pUARTreg, NULL, 0, 0);
		HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart3.pUARTreg, NULL, 0, 0);
		HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart4.pUARTreg, NULL, 0, 0);
		HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart7.pUARTreg, NULL, 0, 0); 
		HalDrvUartIOCtrl(eUART_IO_DeInit, (int)g_stUart8.pUARTreg, NULL, 0, 0); 
	
#if 0 //mod.kks 22.03.19 to fix the RF Module.
		stHalGPIO_InitTypeDef  GPIO_InitStructure;
	
		GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
		GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP;
		GPIO_InitStructure.GPIO_Pin = UART2_RX_PIN;
		HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
	
		GPIO_InitStructure.GPIO_Pin =  UART2_TX_PIN;
		HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
	
		HalGPIOSetVaule(UART2_TX_PIN, eBIT_RESET);
		HalGPIOSetVaule(UART2_RX_PIN, eBIT_RESET);
#endif

    return HAL_RETURN_SUCCESS;
}
/////////////////////////////////////////////////////////////////////////////////////



