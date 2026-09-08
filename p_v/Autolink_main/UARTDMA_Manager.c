/* Includes ------------------------------------------------------------------*/

#include <stdlib.h>
#include <ctype.h>

#include "UARTDMA_Manager.h"
#include "cli.h"
#include "Modem_Manager.h"
#include "HalDmaDriver.h"

#include "HdDebug.h"

/* Private define -----------------------------------------------------------*/
#define Trace(...)  GITDebug(DEBUG_MODULES_UART,__VA_ARGS__)
#if defined(STM32F427X)
#define HAL_UART_DMA_ADDR(X)    (uint32_t)(&(X->DR))
#elif defined(AT32F435VMT7)
#define HAL_UART_DMA_ADDR(X)    (uint32_t)(&(X->dt))
#endif

/* Variable ------------------------------------------------------------------*/
uint8_t g_aUART2DMARxBuffer[HAL_UART2_DMA_BUFFERSIZE];
uint32_t g_nRx2DMAPos;

uint8_t g_aUART3DMARxBuffer[HAL_UART3_DMA_BUFFERSIZE];
uint32_t g_nRx3DMAPos;

uint8_t g_aUART4DMARxBuffer[HAL_UART4_DMA_BUFFERSIZE];
uint32_t g_nRx4DMAPos;

uint8_t g_aUART8DMARxBuffer[HAL_UART8_DMA_BUFFERSIZE];
uint32_t g_nRx8DMAPos;
/* ---------------------------------------------------------------------------*/

//******************************************************************************
void USART2_DMAConfig(void)
{
    stHalDMA_InitTypeDef  DMA_InitStructure;

    /* Enable the DMA clock */
    HalDrvRccIOCtrl(eRCC_IO_DMA_Clock, eRCC_Clock_DMA1, NULL, 0, HAL_ENABLE);
    
    HalDrvDmaIOCtrl(eDMA_IO_MuxEnable, (int)HAL_USART2_DMA, NULL, 0, HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

    /* Configure DMA Initialization Structure */
    DMA_InitStructure.DMA_BufferSize = HAL_UART2_DMA_BUFFERSIZE;
    DMA_InitStructure.DMA_FIFOMode = HAL_DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = HAL_DMA_FIFOThreshold_1QuarterFull;
    DMA_InitStructure.DMA_MemoryBurst = HAL_DMA_MemoryBurst_Single ;
    DMA_InitStructure.DMA_MemoryDataSize = HAL_DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = HAL_DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_Mode = HAL_DMA_Mode_Circular;
    DMA_InitStructure.DMA_PeripheralBaseAddr = HAL_UART_DMA_ADDR(HAL_UART2);//(uint32_t) (&(USART2->DR)) ;
    DMA_InitStructure.DMA_PeripheralBurst = HAL_DMA_PeripheralBurst_Single;
    DMA_InitStructure.DMA_PeripheralDataSize = HAL_DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = HAL_DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_Priority = HAL_DMA_Priority_High;//HAL_DMA_Priority_VeryHigh/*DMA_Priority_High*/;
    /* Configure RX DMA */
    DMA_InitStructure.DMA_Channel = HAL_USART2_RX_DMA_CHANNEL;
    DMA_InitStructure.DMA_DIR = HAL_DMA_DIR_PeripheralToMemory ;
    DMA_InitStructure.DMA_Memory0BaseAddr =(uint32_t)g_aUART2DMARxBuffer;

    HalDrvDmaIOCtrl(eDMA_IO_Init, (int)HAL_USART2_RX_DMA_STREAM, (char*)&DMA_InitStructure, sizeof(DMA_InitStructure), HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_MuxInit, (int)HAL_UART2, NULL, HAL_USART2_RX_DMA_STREAM, HAL_DMA_RX_MODE);
}

uint32_t Uart2Available(void)
{
#if 1
    unsigned int unCount, unDiff;
    unCount = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

    if ( unCount != g_nRx2DMAPos )
    {
        if ( g_nRx2DMAPos > unCount )   unDiff = g_nRx2DMAPos - unCount;
        else                            unDiff = g_nRx2DMAPos + (HAL_UART2_DMA_BUFFERSIZE - unCount);

        //GITDebugPrintf("Uart2Available: CNT %d, g_nRx2DMAPos %d, diff %d\r\n", unCount, g_nRx2DMAPos, unDiff);
        return unDiff;
    }
    else
        return 0;
#else

    unsigned int unCount = 0;
    unCount = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

	return(unCount != g_nRx2DMAPos) ? true : false;
#endif
}

uint32_t ReadUart2DmaCount(void)
{
    unsigned int unCount;
    unCount = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

	return (unCount);
}

void Uart2ClearBuffer(void)
{
    g_nRx2DMAPos = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);
}

uint16_t Uart2NumCharsAvailable(void)
{
	int32_t number;

	number = g_nRx2DMAPos - HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

	if(number >= 0)	return (uint16_t)number;
	else    		return (uint16_t)(HAL_UART2_DMA_BUFFERSIZE + number);
}

uint8_t Uart2_DMARead(void)
{
	uint8_t ch;

	ch = g_aUART2DMARxBuffer[HAL_UART2_DMA_BUFFERSIZE - g_nRx2DMAPos];
	// go back around the buffer
	if(--g_nRx2DMAPos == 0) {
		g_nRx2DMAPos = HAL_UART2_DMA_BUFFERSIZE;
	}

	return ch;
}

void InitModemDMA(void)
{
    Trace(" Init Modem UART DMA\n");
    USART2_DMAConfig();

    /* Enable USART DMA RX Requsts */
    HalDrvUartIOCtrl(eUART_IO_DMA_RTX_Enable, (int)g_stUart2.pUARTreg, NULL, HAL_USART_DMAReq_Rx, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_DMA_RTX_Enable, (int)g_stUart2.pUARTreg, NULL, HAL_USART_DMAReq_Tx, HAL_DISABLE);

    /* Enable DMA USART RX Stream */
    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, HAL_ENABLE);

    g_nRx2DMAPos = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_USART2_RX_DMA_STREAM, NULL, 0, 0);

  /* Waiting the end of Data transfer */
    if(HalUartGetFlagStatus((unsigned int)g_stUart2.pUARTreg, HAL_USART_FLAG_TC) == HAL_SET) {
      	Trace(" Start Modem DMA...\r\n");
		Trace(" clear queue\n");
		ClearQueue(g_stGitCommInfo[eCOMM_TYPE_UART_MODEM].pstInQueue);
	}
	else {
		Trace("\r\nMDM: Fail Init Modem DMA\r\n");
		while(1);
	}
}


//******************************************************************************
void UART3_DMAConfig(void)
{
    stHalDMA_InitTypeDef  DMA_InitStructure;

    /* Enable the DMA clock */
    HalDrvRccIOCtrl(eRCC_IO_DMA_Clock, eRCC_Clock_DMA1, NULL, 0, HAL_ENABLE);

    HalDrvDmaIOCtrl(eDMA_IO_MuxEnable, (int)HAL_UART3_DMA, NULL, 0, HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_UART3_RX_DMA_STREAM, NULL, 0, 0);

    /* Configure DMA Initialization Structure */
    DMA_InitStructure.DMA_BufferSize = HAL_UART3_DMA_BUFFERSIZE;
    DMA_InitStructure.DMA_FIFOMode = HAL_DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = HAL_DMA_FIFOThreshold_1QuarterFull;
    DMA_InitStructure.DMA_MemoryBurst = HAL_DMA_MemoryBurst_Single ;
    DMA_InitStructure.DMA_MemoryDataSize = HAL_DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = HAL_DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_Mode = HAL_DMA_Mode_Circular;
    DMA_InitStructure.DMA_PeripheralBaseAddr = HAL_UART_DMA_ADDR(HAL_UART3);//(uint32_t) (&(USART3->DR)) ;
    DMA_InitStructure.DMA_PeripheralBurst = HAL_DMA_PeripheralBurst_Single;
    DMA_InitStructure.DMA_PeripheralDataSize = HAL_DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = HAL_DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_Priority = HAL_DMA_Priority_High;

    /* Configure RX DMA */
    DMA_InitStructure.DMA_Channel = HAL_UART3_RX_DMA_CHANNEL;
    DMA_InitStructure.DMA_DIR = HAL_DMA_DIR_PeripheralToMemory ;
    DMA_InitStructure.DMA_Memory0BaseAddr =(uint32_t)g_aUART3DMARxBuffer;

    HalDrvDmaIOCtrl(eDMA_IO_Init, (int)HAL_UART3_RX_DMA_STREAM, (char*)&DMA_InitStructure, sizeof(DMA_InitStructure), HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_MuxInit, (int)HAL_UART3, NULL, HAL_UART3_RX_DMA_STREAM, HAL_DMA_RX_MODE);
}

uint32_t UART3Available(void)
{
#if 1
    unsigned int unCount, unDiff;
    unCount = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART3_RX_DMA_STREAM, NULL, 0, 0);

    if ( unCount != g_nRx3DMAPos )
    {
        if ( g_nRx3DMAPos > unCount )   unDiff = g_nRx3DMAPos - unCount;
        else                            unDiff = g_nRx3DMAPos + (HAL_UART3_DMA_BUFFERSIZE - unCount);

        //GITDebugPrintf("Uart3Available: CNT %d, g_nRx2DMAPos %d, diff %d\r\n", unCount, g_nRx3DMAPos, unDiff);
        return unDiff;
    }
    else
        return 0;
#else

    unsigned int unCount;
    unCount = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART3_RX_DMA_STREAM, NULL, 0, 0);

	return(unCount != g_nRx3DMAPos) ? true : false;
#endif
}

void UART3ClearBuffer(void)
{
	g_nRx3DMAPos = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART3_RX_DMA_STREAM, NULL, 0, 0);
}

uint16_t UART3NumCharsAvailable(void)
{
	int32_t number;

	number = g_nRx3DMAPos - HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART3_RX_DMA_STREAM, NULL, 0, 0);

	if(number >= 0) {
		return (uint16_t)number;
	}
	else {
		return (uint16_t)(HAL_UART3_DMA_BUFFERSIZE + number);
	}
}

uint8_t UART3_DMARead(void)
{
	uint8_t ch;

	ch = g_aUART3DMARxBuffer[HAL_UART3_DMA_BUFFERSIZE - g_nRx3DMAPos];
	// go back around the buffer
	if(--g_nRx3DMAPos == 0) {
		g_nRx3DMAPos = HAL_UART3_DMA_BUFFERSIZE;
	}

	return ch;
}

void InitBLEDMA(void)
{
    Trace("BLE: Init Bluetooth UART DMA\n");
    UART3_DMAConfig();

    Trace("BLE: Enable bluetooth dma\n");
    /* Enable USART DMA RX Requsts */
    HalDrvUartIOCtrl(eUART_IO_DMA_RTX_Enable, (int)g_stUart3.pUARTreg, NULL, HAL_USART_DMAReq_Rx, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_DMA_RTX_Enable, (int)g_stUart3.pUARTreg, NULL, HAL_USART_DMAReq_Tx, HAL_DISABLE);

    /* Enable DMA USART RX Stream */
    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, (int)HAL_UART3_RX_DMA_STREAM, NULL, 0, HAL_ENABLE);

    g_nRx3DMAPos = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART3_RX_DMA_STREAM, NULL, 0, 0);

    /* Waiting the end of Data transfer */
    if(HalUartGetFlagStatus((unsigned int)g_stUart3.pUARTreg, HAL_USART_FLAG_TC) == HAL_SET) {
    }
}


//******************************************************************************
void UART4_DMAConfig(void)
{
    stHalDMA_InitTypeDef  DMA_InitStructure;

    /* Enable the DMA clock */
    HalDrvRccIOCtrl(eRCC_IO_DMA_Clock, eRCC_Clock_DMA1, NULL, 0, HAL_ENABLE);

    HalDrvDmaIOCtrl(eDMA_IO_MuxEnable, (int)HAL_UART4_DMA, NULL, 0, HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_UART4_RX_DMA_STREAM, NULL, 0, 0);

    /* Configure DMA Initialization Structure */
    DMA_InitStructure.DMA_BufferSize = HAL_UART4_DMA_BUFFERSIZE;
    DMA_InitStructure.DMA_FIFOMode = HAL_DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = HAL_DMA_FIFOThreshold_1QuarterFull;
    DMA_InitStructure.DMA_MemoryBurst = HAL_DMA_MemoryBurst_Single ;
    DMA_InitStructure.DMA_MemoryDataSize = HAL_DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = HAL_DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_Mode = HAL_DMA_Mode_Circular;
    DMA_InitStructure.DMA_PeripheralBaseAddr =HAL_UART_DMA_ADDR(HAL_UART4);//(uint32_t) (&(UART4->DR)) ;
    DMA_InitStructure.DMA_PeripheralBurst = HAL_DMA_PeripheralBurst_Single;
    DMA_InitStructure.DMA_PeripheralDataSize = HAL_DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = HAL_DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_Priority = HAL_DMA_Priority_High;

    /* Configure RX DMA */
    DMA_InitStructure.DMA_Channel = HAL_UART4_RX_DMA_CHANNEL;
    DMA_InitStructure.DMA_DIR = HAL_DMA_DIR_PeripheralToMemory ;
    DMA_InitStructure.DMA_Memory0BaseAddr =(uint32_t)g_aUART4DMARxBuffer;

    HalDrvDmaIOCtrl(eDMA_IO_Init, (int)HAL_UART4_RX_DMA_STREAM, (char*)&DMA_InitStructure, sizeof(DMA_InitStructure), HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_MuxInit, (int)HAL_UART4, NULL, HAL_UART4_RX_DMA_STREAM, HAL_DMA_RX_MODE);
}

uint32_t UART4Available(void)
{
#if 1
        unsigned int unCount, unDiff;
        unCount = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART4_RX_DMA_STREAM, NULL, 0, 0);
    
        if ( unCount != g_nRx4DMAPos )
        {
            if ( g_nRx4DMAPos > unCount )   unDiff = g_nRx4DMAPos - unCount;
            else                            unDiff = g_nRx4DMAPos + (HAL_UART4_DMA_BUFFERSIZE - unCount);
    
            //GITDebugPrintf("Uart4Available: CNT %d, g_nRx4DMAPos %d, diff %d\r\n", unCount, g_nRx4DMAPos, unDiff);
            return unDiff;
        }
        else
            return 0;
#else

    unsigned int unCount;
    unCount = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART4_RX_DMA_STREAM, NULL, 0, 0);

	return(unCount != g_nRx4DMAPos) ? true : false;
#endif
}

void UART4ClearBuffer(void)
{
	g_nRx4DMAPos = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART4_RX_DMA_STREAM, NULL, 0, 0);
}

uint16_t UART4NumCharsAvailable(void)
{
	int32_t number;

	number = g_nRx4DMAPos - HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART4_RX_DMA_STREAM, NULL, 0, 0);

	if(number >= 0) {
		return (uint16_t)number;
	}
	else {
		return (uint16_t)(HAL_UART4_DMA_BUFFERSIZE + number);
	}
}

uint8_t UART4_DMARead(void)
{
	uint8_t ch;

	ch = g_aUART4DMARxBuffer[HAL_UART4_DMA_BUFFERSIZE - g_nRx4DMAPos];
	// go back around the buffer
	if(--g_nRx4DMAPos == 0) {
		g_nRx4DMAPos = HAL_UART4_DMA_BUFFERSIZE;
	}

	return ch;
}

void GPS_DisableCommunication(void)
{
    Trace("GPS: Disable UART4\n");
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart4.pUARTreg, NULL, 0, HAL_DISABLE);

    Trace("GPS: Disable UART4 dma\n");
    /* Enable DMA USART RX Stream */
    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, (int)HAL_UART4_RX_DMA_STREAM, NULL, 0, HAL_DISABLE);
}

void GPS_EnableCommunication(void)
{
    Trace("GPS: Enable UART4\n");
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart4.pUARTreg, NULL, 0, HAL_ENABLE);

    Trace("GPS: Enable UART4 dma\n");
    /* Enable DMA USART RX Stream */
    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, (int)HAL_UART4_RX_DMA_STREAM, NULL, 0, HAL_ENABLE);
}

void InitGPSDMA(void)
{
    Trace("GPS: Init GPS UART DMA\n");
    UART4_DMAConfig();

    Trace("GPS: Enable GPS DMA\n");

    /* Enable USART DMA RX Requsts */
    HalDrvUartIOCtrl(eUART_IO_DMA_RTX_Enable, (int)g_stUart4.pUARTreg, NULL, HAL_USART_DMAReq_Rx, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_DMA_RTX_Enable, (int)g_stUart4.pUARTreg, NULL, HAL_USART_DMAReq_Tx, HAL_DISABLE);

    /* Enable DMA USART RX Stream */
    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, (int)HAL_UART4_RX_DMA_STREAM, NULL, 0, HAL_ENABLE);

	g_nRx4DMAPos = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART4_RX_DMA_STREAM, NULL, 0, 0);

  /* Waiting the end of Data transfer */
    if(HalUartGetFlagStatus((unsigned int)g_stUart4.pUARTreg, HAL_USART_FLAG_TC) == HAL_SET) {
	}
}

void UART8_DMAConfig(void)
{
    stHalDMA_InitTypeDef  DMA_InitStructure;

    /* Enable the DMA clock */
    HalDrvRccIOCtrl(eRCC_IO_DMA_Clock, eRCC_Clock_DMA1, NULL, 0, HAL_ENABLE);

    HalDrvDmaIOCtrl(eDMA_IO_MuxEnable, (int)HAL_UART8_DMA, NULL, 0, HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_UART8_RX_DMA_STREAM, NULL, 0, 0);

    /* Configure DMA Initialization Structure */
    DMA_InitStructure.DMA_BufferSize = HAL_UART8_DMA_BUFFERSIZE;
    DMA_InitStructure.DMA_FIFOMode = HAL_DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = HAL_DMA_FIFOThreshold_1QuarterFull;
    DMA_InitStructure.DMA_MemoryBurst = HAL_DMA_MemoryBurst_Single ;
    DMA_InitStructure.DMA_MemoryDataSize = HAL_DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = HAL_DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_Mode = HAL_DMA_Mode_Circular;
    DMA_InitStructure.DMA_PeripheralBaseAddr =HAL_UART_DMA_ADDR(HAL_UART8);//(uint32_t) (&(UART8->DR)) ;
    DMA_InitStructure.DMA_PeripheralBurst = HAL_DMA_PeripheralBurst_Single;
    DMA_InitStructure.DMA_PeripheralDataSize = HAL_DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = HAL_DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_Priority = HAL_DMA_Priority_High;

    /* Configure RX DMA */
    DMA_InitStructure.DMA_Channel = HAL_UART8_RX_DMA_CHANNEL;
    DMA_InitStructure.DMA_DIR = HAL_DMA_DIR_PeripheralToMemory ;
    DMA_InitStructure.DMA_Memory0BaseAddr =(uint32_t)g_aUART8DMARxBuffer;

    HalDrvDmaIOCtrl(eDMA_IO_Init, (int)HAL_UART8_RX_DMA_STREAM, (char*)&DMA_InitStructure, sizeof(DMA_InitStructure), HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_MuxInit, (int)HAL_UART8, NULL, HAL_UART8_RX_DMA_STREAM, HAL_DMA_RX_MODE);
}

uint32_t Uart8Available(void)
{
#if 1
    unsigned int unCount, unDiff;
    unCount = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART8_RX_DMA_STREAM, NULL, 0, 0);

    if ( unCount != g_nRx8DMAPos )
    {
        if ( g_nRx8DMAPos > unCount )   unDiff = g_nRx8DMAPos - unCount;
        else                            unDiff = g_nRx8DMAPos + (HAL_UART8_DMA_BUFFERSIZE - unCount);
        //GITDebugPrintf("Uart8Available: CNT %d, g_nRx8DMAPos %d, diff %d\r\n", unCount, g_nRx8DMAPos, unDiff);
        return unDiff;
    }
    else
        return 0;
#else
    unsigned int unCount;
    unCount = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART8_RX_DMA_STREAM, NULL, 0, 0);

	return(unCount != g_nRx8DMAPos) ? true : false;
#endif
}

void Uart8ClearBuffer(void)
{
	g_nRx8DMAPos = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART8_RX_DMA_STREAM, NULL, 0, 0);
}

uint16_t Uart8NumCharsAvailable(void)
{
	int32_t number;

	number = g_nRx8DMAPos - HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART8_RX_DMA_STREAM, NULL, 0, 0);

	if(number >= 0) {
		return (uint16_t)number;
	}
	else {
		return (uint16_t)(HAL_UART8_DMA_BUFFERSIZE + number);
	}
}

uint8_t Uart8_DMARead(void)
{
	uint8_t ch;

	ch = g_aUART8DMARxBuffer[HAL_UART8_DMA_BUFFERSIZE - g_nRx8DMAPos];
	// go back around the buffer
	if(--g_nRx8DMAPos == 0) {
		g_nRx8DMAPos = HAL_UART8_DMA_BUFFERSIZE;
	}

	return ch;
}

void InitSELFTESTDMA(void)
{
    Trace(" InitSELFTESTDMA\r\n");
    UART8_DMAConfig();

    /* Enable USART DMA RX Requsts */
    HalDrvUartIOCtrl(eUART_IO_DMA_RTX_Enable, (int)g_stUart8.pUARTreg, NULL, HAL_USART_DMAReq_Rx, HAL_ENABLE);
    HalDrvUartIOCtrl(eUART_IO_DMA_RTX_Enable, (int)g_stUart8.pUARTreg, NULL, HAL_USART_DMAReq_Tx, HAL_DISABLE);

    /* Enable DMA USART RX Stream */
    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, (int)HAL_UART8_RX_DMA_STREAM, NULL, 0, HAL_ENABLE);

    g_nRx8DMAPos = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)HAL_UART8_RX_DMA_STREAM, NULL, 0, 0);

    /* Waiting the end of Data transfer */
    if(HalUartGetFlagStatus((unsigned int)g_stUart8.pUARTreg, HAL_USART_FLAG_TC) == HAL_SET) {
        Trace(" Start Modem DMA...\r\n");
        Trace(" clear queue\n");
        ClearQueue(g_stGitCommInfo[eCOMM_TYPE_UART_SELFTEST].pstInQueue);
    }
    else {
        Trace("\r\nMDM: Fail InitSELFTESTDMA\r\n");
        while(1);
    }
}

void DisableSELFTESTDMA(void)
{
	
	GITDebugPrintf("UART8DMATask\n");
	GITDebugPrintf("Deinit uart8 dma\n");
    HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)HAL_UART8_RX_DMA_STREAM, NULL, 0, 0);
	

	printf("SELFTEST: Disable SELFTEST DMA\n");

	/* Disable DMA USART RX Stream */
    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, (int)HAL_UART8_RX_DMA_STREAM, NULL, 0, HAL_DISABLE);
	/* Disable USART DMA RX Requsts */
    HalDrvUartIOCtrl(eUART_IO_DMA_RTX_Enable, (int)g_stUart8.pUARTreg, NULL, HAL_USART_DMAReq_Rx, HAL_DISABLE);	
	
	printf("SELFTEST: Disable UART8\n");
    HalDrvUartIOCtrl(eUART_IO_Port_ENABLE, (int)g_stUart8.pUARTreg, NULL, 0, HAL_DISABLE);

	/* Waiting the end of Data transfer */
    if(HalUartGetFlagStatus((unsigned int)g_stUart8.pUARTreg, HAL_USART_FLAG_TC) == HAL_SET) {
	}
	
	return;
}

//******************************************************************************
