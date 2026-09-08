#ifndef __UARTDMA_MANAGER_H__
#define __UARTDMA_MANAGER_H__

#include "HalHandler.h"

//******************************************************************************
// UART2 DMA
//******************************************************************************
#define HAL_USART2_DMA                       DMA1
#define HAL_USART2_RX_DMA_CHANNEL            eDMA_Channel_4
#define HAL_USART2_RX_DMA_STREAM             eDMA1_Stream5
#define HAL_UART2_DMA_BUFFERSIZE							(1800)

//******************************************************************************
// UART3 DMA
//******************************************************************************
#define HAL_UART3_DMA                       DMA1
#define HAL_UART3_RX_DMA_CHANNEL            eDMA_Channel_4
#define HAL_UART3_RX_DMA_STREAM             eDMA1_Stream1
#define HAL_UART3_DMA_BUFFERSIZE							(384)
//******************************************************************************

//******************************************************************************
// UART4 DMA
//******************************************************************************
#define HAL_UART4_DMA                       DMA1
#define HAL_UART4_RX_DMA_CHANNEL            eDMA_Channel_4
#define HAL_UART4_RX_DMA_STREAM             eDMA1_Stream2
#define HAL_UART4_DMA_BUFFERSIZE							(384)

//******************************************************************************
// UART8 DMA
//******************************************************************************
#define HAL_UART8_DMA                       DMA1
#define HAL_UART8_RX_DMA_CHANNEL            eDMA_Channel_5
#define HAL_UART8_RX_DMA_STREAM             eDMA1_Stream6
#define HAL_UART8_DMA_BUFFERSIZE							(512)



void InitModemDMA(void);
void InitGPSDMA(void);
void InitBLEDMA(void);
void InitSELFTESTDMA(void);
void DisableSELFTESTDMA(void);

void ModemDMA_Manager(void);

void USART2_DMAConfig(void);
uint32_t Uart2Available(void);
void Uart2ClearBuffer(void);
uint16_t Uart2NumCharsAvailable(void);

void UART8_DMAConfig(void);
uint32_t Uart8Available(void);
void Uart8ClearBuffer(void);
uint16_t Uart8NumCharsAvailable(void);

void BluetoothDMA_Manager(void);
void GPSDMA_Manager(void);

void GPS_DisableCommunication(void);
void GPS_EnableCommunication(void);

uint32_t Uart2Available(void);
uint32_t UART3Available(void);
uint32_t UART4Available(void);

uint8_t Uart2_DMARead(void);
uint8_t UART3_DMARead(void);
uint8_t UART4_DMARead(void);
uint8_t Uart8_DMARead(void);

#endif
