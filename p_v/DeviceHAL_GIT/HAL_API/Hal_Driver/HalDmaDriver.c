/*
  ******************************************************************************
  * @file    HalDMADriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include "HalDMADriver.h"
#include "HalHandler.h"
#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "stm32f4xx_dma.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_dma.h"
#endif


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
unsigned int HalDrvDma_ConvertDMAChannel(eHalDmaChannel eDmaChNo)
{
   unsigned int unDMAChannelNo;
#if defined(STM32F427X)
    if      ( eDmaChNo == eDMA_Channel_0 ) unDMAChannelNo = (unsigned int)DMA_Channel_0;
    else if ( eDmaChNo == eDMA_Channel_1 ) unDMAChannelNo = (unsigned int)DMA_Channel_1;
    else if ( eDmaChNo == eDMA_Channel_2 ) unDMAChannelNo = (unsigned int)DMA_Channel_2;
    else if ( eDmaChNo == eDMA_Channel_3 ) unDMAChannelNo = (unsigned int)DMA_Channel_3;
    else if ( eDmaChNo == eDMA_Channel_4 ) unDMAChannelNo = (unsigned int)DMA_Channel_4;
    else if ( eDmaChNo == eDMA_Channel_5 ) unDMAChannelNo = (unsigned int)DMA_Channel_5;
    else if ( eDmaChNo == eDMA_Channel_6 ) unDMAChannelNo = (unsigned int)DMA_Channel_6;
    else if ( eDmaChNo == eDMA_Channel_7 ) unDMAChannelNo = (unsigned int)DMA_Channel_7;
    else if ( eDmaChNo == eDMA_Channel_7 ) unDMAChannelNo = (unsigned int)DMA_Channel_7;
#elif defined(AT32F435VMT7)
/*
    if      ( eDmaChNo == eDMA_Channel_1 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL1;
    else if ( eDmaChNo == eDMA_Channel_2 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL2;
    else if ( eDmaChNo == eDMA_Channel_3 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL3;
    else if ( eDmaChNo == eDMA_Channel_4 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL4;
    else if ( eDmaChNo == eDMA_Channel_5 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL5;
    else if ( eDmaChNo == eDMA_Channel_6 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL6;
    else if ( eDmaChNo == eDMA_Channel_7 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL7;
*/
    eHalDmaStream eDmaStreamNo = (eHalDmaStream)eDmaChNo;
    if      ( eDmaStreamNo == eDMA1_Stream0 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL1;
    else if ( eDmaStreamNo == eDMA1_Stream1 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL2;
    else if ( eDmaStreamNo == eDMA1_Stream2 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL3;
    else if ( eDmaStreamNo == eDMA1_Stream3 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL4;
    else if ( eDmaStreamNo == eDMA1_Stream4 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL5;
    else if ( eDmaStreamNo == eDMA1_Stream5 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL6;
    else if ( eDmaStreamNo == eDMA1_Stream6 ) unDMAChannelNo = (unsigned int)DMA1MUX_CHANNEL7;
    else if ( eDmaStreamNo == eDMA2_Stream0 ) unDMAChannelNo = (unsigned int)DMA2MUX_CHANNEL1;
    else if ( eDmaStreamNo == eDMA2_Stream1 ) unDMAChannelNo = (unsigned int)DMA2MUX_CHANNEL2;
    else if ( eDmaStreamNo == eDMA2_Stream2 ) unDMAChannelNo = (unsigned int)DMA2MUX_CHANNEL3;
    else if ( eDmaStreamNo == eDMA2_Stream3 ) unDMAChannelNo = (unsigned int)DMA2MUX_CHANNEL4;
    else if ( eDmaStreamNo == eDMA2_Stream4 ) unDMAChannelNo = (unsigned int)DMA2MUX_CHANNEL5;
    else if ( eDmaStreamNo == eDMA2_Stream5 ) unDMAChannelNo = (unsigned int)DMA2MUX_CHANNEL6;
    else if ( eDmaStreamNo == eDMA2_Stream6 ) unDMAChannelNo = (unsigned int)DMA2MUX_CHANNEL7;

#endif
 
    return unDMAChannelNo;
}

unsigned int HalDrvDma_ConvertDMAStream(eHalDmaStream eDmaStreamCh)
{
    unsigned int unDMAStream;
#if defined(STM32F427X)
    if      ( eDmaStreamCh == eDMA1_Stream0 ) unDMAStream = (unsigned int)DMA1_Stream0;
    else if ( eDmaStreamCh == eDMA1_Stream1 ) unDMAStream = (unsigned int)DMA1_Stream1;
    else if ( eDmaStreamCh == eDMA1_Stream2 ) unDMAStream = (unsigned int)DMA1_Stream2;
    else if ( eDmaStreamCh == eDMA1_Stream3 ) unDMAStream = (unsigned int)DMA1_Stream3;
    else if ( eDmaStreamCh == eDMA1_Stream4 ) unDMAStream = (unsigned int)DMA1_Stream4;
    else if ( eDmaStreamCh == eDMA1_Stream5 ) unDMAStream = (unsigned int)DMA1_Stream5;
    else if ( eDmaStreamCh == eDMA1_Stream6 ) unDMAStream = (unsigned int)DMA1_Stream6;
    else if ( eDmaStreamCh == eDMA2_Stream0 ) unDMAStream = (unsigned int)DMA2_Stream0;
    else if ( eDmaStreamCh == eDMA2_Stream1 ) unDMAStream = (unsigned int)DMA2_Stream1;
    else if ( eDmaStreamCh == eDMA2_Stream2 ) unDMAStream = (unsigned int)DMA2_Stream2;
    else if ( eDmaStreamCh == eDMA2_Stream3 ) unDMAStream = (unsigned int)DMA2_Stream3;
    else if ( eDmaStreamCh == eDMA2_Stream4 ) unDMAStream = (unsigned int)DMA2_Stream4;
    else if ( eDmaStreamCh == eDMA2_Stream5 ) unDMAStream = (unsigned int)DMA2_Stream5;
    else if ( eDmaStreamCh == eDMA2_Stream6 ) unDMAStream = (unsigned int)DMA2_Stream6;
#elif defined(AT32F435VMT7)
    if      ( eDmaStreamCh == eDMA1_Stream0 ) unDMAStream = (unsigned int)DMA1_CHANNEL1;
    else if ( eDmaStreamCh == eDMA1_Stream1 ) unDMAStream = (unsigned int)DMA1_CHANNEL2;
    else if ( eDmaStreamCh == eDMA1_Stream2 ) unDMAStream = (unsigned int)DMA1_CHANNEL3;
    else if ( eDmaStreamCh == eDMA1_Stream3 ) unDMAStream = (unsigned int)DMA1_CHANNEL4;
    else if ( eDmaStreamCh == eDMA1_Stream4 ) unDMAStream = (unsigned int)DMA1_CHANNEL5;
    else if ( eDmaStreamCh == eDMA1_Stream5 ) unDMAStream = (unsigned int)DMA1_CHANNEL6;
    else if ( eDmaStreamCh == eDMA1_Stream6 ) unDMAStream = (unsigned int)DMA1_CHANNEL7;
    else if ( eDmaStreamCh == eDMA2_Stream0 ) unDMAStream = (unsigned int)DMA2_CHANNEL1;
    else if ( eDmaStreamCh == eDMA2_Stream1 ) unDMAStream = (unsigned int)DMA2_CHANNEL2;
    else if ( eDmaStreamCh == eDMA2_Stream2 ) unDMAStream = (unsigned int)DMA2_CHANNEL3;
    else if ( eDmaStreamCh == eDMA2_Stream3 ) unDMAStream = (unsigned int)DMA2_CHANNEL4;
    else if ( eDmaStreamCh == eDMA2_Stream4 ) unDMAStream = (unsigned int)DMA2_CHANNEL5;
    else if ( eDmaStreamCh == eDMA2_Stream5 ) unDMAStream = (unsigned int)DMA2_CHANNEL6;
    else if ( eDmaStreamCh == eDMA2_Stream6 ) unDMAStream = (unsigned int)DMA2_CHANNEL7;
//    else if ( eDmaStreamCh == DMA2D        ) unDMAStream = (unsigned int)DMA2_CHANNEL1;
#endif
    return unDMAStream;
}

#if defined(AT32F435VMT7)
dmamux_requst_id_sel_type HalDrvDma_GetDMAReqID(unsigned int unPeriph, int nRXMode)
{
    dmamux_requst_id_sel_type dmamux_req_sel;
    if ( unPeriph == (unsigned int)USART1 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_USART1_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_USART1_TX;
    }
    else if ( unPeriph == (unsigned int)USART2 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_USART2_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_USART2_TX;
    }
    else if ( unPeriph == (unsigned int)USART3 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_USART3_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_USART3_TX;
    }
    else if ( unPeriph == (unsigned int)UART4 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_UART4_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_UART4_TX;
    }
    else if ( unPeriph == (unsigned int)UART5 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_UART5_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_UART5_TX;
    }
    else if ( unPeriph == (unsigned int)USART6 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_USART6_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_USART6_TX;
    }
    else if ( unPeriph == (unsigned int)UART7 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_UART7_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_UART7_TX;
    }
    else if ( unPeriph == (unsigned int)UART8 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_UART8_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_UART8_TX;
    }

    if ( unPeriph == (unsigned int)ADC1 )
        dmamux_req_sel = DMAMUX_DMAREQ_ID_ADC1;
    else if ( unPeriph == (unsigned int)ADC2 )
        dmamux_req_sel = DMAMUX_DMAREQ_ID_ADC2;
    else if ( unPeriph == (unsigned int)ADC3 )
        dmamux_req_sel = DMAMUX_DMAREQ_ID_ADC3;
//    else if ( unPeriph == &ADCCOM )

    if ( unPeriph == (unsigned int)SPI1 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_SPI1_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_SPI1_TX;
    }
    else if ( unPeriph == (unsigned int)SPI2 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_SPI2_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_SPI2_TX;
    }
    else if ( unPeriph == (unsigned int)SPI3 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_SPI3_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_UART8_TX;
    }
    else if ( unPeriph == (unsigned int)SPI4 )
    {
        if ( nRXMode == 0 ) dmamux_req_sel = DMAMUX_DMAREQ_ID_SPI4_RX;
        else                dmamux_req_sel = DMAMUX_DMAREQ_ID_SPI4_TX;
    }

    return dmamux_req_sel;
}
#endif

int HalDrvDmaOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvDmaRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvDmaWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{

    return HAL_RETURN_SUCCESS;
}

int HalDrvDmaIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    eHalDmaIoCtlMode nIoCtlMode = (eHalDmaIoCtlMode)nLparam;
    
    switch ( nIoCtlMode )
    {
        case eDMA_IO_Init:
        {
            unsigned int unDMAStreamCh = HalDrvDma_ConvertDMAStream((eHalDmaStream)nRparam);
            stHalDMA_InitTypeDef* pDMA_Init = (stHalDMA_InitTypeDef*)pBuffer;
#if defined(STM32F427X)
            pDMA_Init->DMA_Channel = HalDrvDma_ConvertDMAChannel((eHalDmaChannel)pDMA_Init->DMA_Channel);
            DMA_Init((DMA_Stream_TypeDef*)unDMAStreamCh, (DMA_InitTypeDef*)pDMA_Init);
#elif defined(AT32F435VMT7)
            dma_init_type dma_init_struct;

            dma_init_struct.peripheral_base_addr = pDMA_Init->DMA_PeripheralBaseAddr;
            dma_init_struct.memory_base_addr     = pDMA_Init->DMA_Memory0BaseAddr;

            if ( pDMA_Init->DMA_DIR == HAL_DMA_DIR_PeripheralToMemory )
                dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
            else if ( pDMA_Init->DMA_DIR == HAL_DMA_DIR_MemoryToPeripheral )
                dma_init_struct.direction = DMA_DIR_MEMORY_TO_PERIPHERAL;
            else if ( pDMA_Init->DMA_DIR == HAL_DMA_DIR_MemoryToMemory )
                dma_init_struct.direction = DMA_DIR_MEMORY_TO_MEMORY;
                
            dma_init_struct.buffer_size             = pDMA_Init->DMA_BufferSize;

            if ( pDMA_Init->DMA_PeripheralInc == HAL_DMA_PeripheralInc_Disable )
                dma_init_struct.peripheral_inc_enable   = FALSE;
            else
                dma_init_struct.peripheral_inc_enable   = TRUE;

            if ( pDMA_Init->DMA_MemoryInc == HAL_DMA_MemoryInc_Disable )
                dma_init_struct.memory_inc_enable   = FALSE;
            else
                dma_init_struct.memory_inc_enable   = TRUE;

            if ( pDMA_Init->DMA_PeripheralDataSize == HAL_DMA_PeripheralDataSize_Byte )
                dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
            else if ( pDMA_Init->DMA_PeripheralDataSize == HAL_DMA_PeripheralDataSize_HalfWord )
                dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
            else if ( pDMA_Init->DMA_PeripheralDataSize == HAL_DMA_PeripheralDataSize_Word)
                dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_WORD ;

            if ( pDMA_Init->DMA_MemoryDataSize == HAL_DMA_MemoryDataSize_Byte )
                dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;
            else if ( pDMA_Init->DMA_MemoryDataSize == HAL_DMA_MemoryDataSize_HalfWord )
                dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_HALFWORD;
            else if ( pDMA_Init->DMA_MemoryDataSize == HAL_DMA_MemoryDataSize_Word )
                dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_WORD;
                    
            dma_init_struct.loop_mode_enable = (confirm_state)nOverlap;

            if ( pDMA_Init->DMA_Priority == HAL_DMA_Priority_Low )
                dma_init_struct.priority = DMA_PRIORITY_LOW;
            else if ( pDMA_Init->DMA_Priority == HAL_DMA_Priority_Medium )
                dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
            else if ( pDMA_Init->DMA_Priority == HAL_DMA_Priority_High )
                dma_init_struct.priority = DMA_PRIORITY_HIGH;
            else if ( pDMA_Init->DMA_Priority == HAL_DMA_Priority_VeryHigh )
                dma_init_struct.priority = DMA_PRIORITY_VERY_HIGH;

            dma_init((dma_channel_type*)unDMAStreamCh, &dma_init_struct);
#endif
        }
            break;
        case eDMA_IO_DeInit:
        {
            unsigned int unDMAStreamCh = HalDrvDma_ConvertDMAStream((eHalDmaStream)nRparam);
#if defined(STM32F427X)
            DMA_DeInit((DMA_Stream_TypeDef*)unDMAStreamCh);
#elif defined(AT32F435VMT7)
            dma_reset((dma_channel_type*)unDMAStreamCh);
#endif
        }
            break;
        case eDMA_IO_MuxInit:
#if defined(STM32F427X)
#elif defined(AT32F435VMT7)
{           
            int nReqRxTxMode = nOverlap; // 0:Rx Req, 1:TxReq
            dmamux_requst_id_sel_type dmamux_req_sel = HalDrvDma_GetDMAReqID(nRparam, nReqRxTxMode);
            unsigned int unDmaMuxChannel = HalDrvDma_ConvertDMAChannel((eHalDmaChannel)nLength);

            dmamux_init((dmamux_channel_type*)unDmaMuxChannel, dmamux_req_sel);
}
#endif
        
            break;
        case eDMA_IO_MuxEnable:
#if defined(STM32F427X)    
#elif defined(AT32F435VMT7)
            dmamux_enable((dma_type*)nRparam, (confirm_state)nOverlap);
#endif
            break;
        case eDMA_IO_ChannelEnable:
        {
            unsigned int unDMAStreamCh = HalDrvDma_ConvertDMAStream((eHalDmaStream)nRparam);

#if defined(STM32F427X)
            DMA_Cmd((DMA_Stream_TypeDef*)unDMAStreamCh, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            dma_channel_enable((dma_channel_type*)unDMAStreamCh, (confirm_state)nOverlap);
#endif
        }
            break;
        case eDMA_IO_INT_Enable:
#if defined(STM32F427X)
#elif defined(AT32F435VMT7)
            //dma_interrupt_enable(DMA2_CHANNEL3, DMA_FDT_INT, FALSE);
#endif
            break;
        case eDMA_IO_GetDMACount:
        {
            unsigned int unDMAStreamCh = HalDrvDma_ConvertDMAStream((eHalDmaStream)nRparam);
#if defined(STM32F427X)    
            return DMA_GetCurrDataCounter((DMA_Stream_TypeDef*)unDMAStreamCh);
#elif defined(AT32F435VMT7)
            return dma_data_number_get((dma_channel_type*)unDMAStreamCh);
#endif
        }
            break;
        default:
            break;
    }

    return HAL_RETURN_SUCCESS;
}

int HalDrvDmaClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

