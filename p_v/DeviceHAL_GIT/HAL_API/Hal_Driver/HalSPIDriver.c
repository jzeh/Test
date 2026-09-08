/*
  ******************************************************************************
  * @file    HalSPIDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include "HalSPIDriver.h"

#include "SPIDriverMicron.h"
#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "STM32F4xx_spi.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_spi.h"
#endif

#include "HdDebug.h"


/* Private typedef -----------------------------------------------------------*/
void InitSPIDMAConfig(unsigned int unSPiNo);

/* Private define ------------------------------------------------------------*/
#define Trace(...)  GITDebug(DEBUG_MODULES_SPI,__VA_ARGS__)

#if defined(STM32F427X)
#define HAL_SPI_DMA_ADDR(X)    (unsigned int)(&(X->DR))
#elif defined(AT32F435VMT7)
#define HAL_SPI_DMA_ADDR(X)    (unsigned int)(&(X->dt))
#endif

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#define HAL_DMA_SPI_BUFFER_SIZE             (1024)
#if defined(SPI1_USE_DMA)
#define HAL_DMA_SPI1                        DMA2
#define HAL_DMA_SPI1_CLOCK                  eRCC_Clock_DMA2
#define HAL_DMA_SPI1_RX_CHANNEL             eDMA_Channel_3
#define HAL_DMA_SPI1_RX_STREAM              eDMA2_Stream2
#define HAL_DMA_SPI1_RX_BUFFERSIZE          (HAL_DMA_SPI_BUFFER_SIZE)
#define HAL_DMA_SPI1_TX_CHANNEL             eDMA_Channel_3
#define HAL_DMA_SPI1_TX_STREAM              eDMA2_Stream3
#define HAL_DMA_SPI1_TX_BUFFERSIZE          (HAL_DMA_SPI_BUFFER_SIZE)

unsigned char g_arrDMA_SPI1_RxBuffer[HAL_DMA_SPI1_RX_BUFFERSIZE];
unsigned int g_unDMA_SPI1_RxPos = 0;
unsigned char g_arrDMA_SPI1_TxBuffer[HAL_DMA_SPI1_TX_BUFFERSIZE];
unsigned int g_unDMA_SPI1_TxPos = 0;
#endif


#if defined(SPI2_USE_DMA)
#define HAL_DMA_SPI2                        DMA1
#define HAL_DMA_SPI2_CLOCK                  eRCC_Clock_DMA1
#define HAL_DMA_SPI2_RX_CHANNEL             eDMA_Channel_0
#define HAL_DMA_SPI2_RX_STREAM              eDMA1_Stream3
#define HAL_DMA_SPI2_RX_BUFFERSIZE          (HAL_DMA_SPI_BUFFER_SIZE)
#define HAL_DMA_SPI2_TX_CHANNEL             eDMA_Channel_0
#define HAL_DMA_SPI2_TX_STREAM              eDMA1_Stream4
#define HAL_DMA_SPI2_TX_BUFFERSIZE          (HAL_DMA_SPI_BUFFER_SIZE)

unsigned char g_arrDMA_SPI2_RxBuffer[HAL_DMA_SPI2_RX_BUFFERSIZE];
unsigned int g_unDMA_SPI2_RxPos = 0;
unsigned char g_arrDMA_SPI2_TxBuffer[HAL_DMA_SPI2_TX_BUFFERSIZE];
unsigned int g_unDMA_SPI2_TxPos = 0;
#endif

#if defined(SPI3_USE_DMA)
#define HAL_DMA_SPI3                        DMA2
#define HAL_DMA_SPI3_CLOCK                  eRCC_Clock_DMA2
#define HAL_DMA_SPI3_RX_CHANNEL             eDMA_Channel_0
#define HAL_DMA_SPI3_RX_STREAM              eDMA1_Stream0
#define HAL_DMA_SPI3_RX_BUFFERSIZE          (HAL_DMA_SPI_BUFFER_SIZE)
#define HAL_DMA_SPI3_TX_CHANNEL             eDMA_Channel_0
#define HAL_DMA_SPI3_TX_STREAM              eDMA1_Stream5
#define HAL_DMA_SPI3_TX_BUFFERSIZE          (HAL_DMA_SPI_BUFFER_SIZE)

unsigned char g_arrDMA_SPI3_RxBuffer[HAL_DMA_SPI3_RX_BUFFERSIZE];
unsigned int g_unDMA_SPI3_RxPos = 0;
unsigned char g_arrDMA_SPI3_TxBuffer[HAL_DMA_SPI3_TX_BUFFERSIZE];
unsigned int g_unDMA_SPI3_TxPos = 0;
#endif

#if defined(SPI4_USE_DMA)
#define HAL_DMA_SPI4                        DMA2
#define HAL_DMA_SPI4_CLOCK                  eRCC_Clock_DMA2
#define HAL_DMA_SPI4_RX_CHANNEL             eDMA_Channel_4
#define HAL_DMA_SPI4_RX_STREAM              eDMA2_Stream0
#define HAL_DMA_SPI4_RX_BUFFERSIZE          (HAL_DMA_SPI_BUFFER_SIZE)
#define HAL_DMA_SPI4_TX_CHANNEL             eDMA_Channel_4
#define HAL_DMA_SPI4_TX_STREAM              eDMA2_Stream1
#define HAL_DMA_SPI4_TX_BUFFERSIZE          (HAL_DMA_SPI_BUFFER_SIZE)

unsigned char g_arrDMA_SPI4_RxBuffer[HAL_DMA_SPI4_RX_BUFFERSIZE];
unsigned int g_unDMA_SPI4_RxPos = 0;
unsigned char g_arrDMA_SPI4_TxBuffer[HAL_DMA_SPI4_TX_BUFFERSIZE];
unsigned int g_unDMA_SPI4_TxPos = 0;
#endif


/* Private function prototypes -----------------------------------------------*/
void HalSPI_Init();
void HalSPI_LowLevel_Init(unsigned int nSPINo);
void HalSPI_GetGpioInfo(unsigned int unSPINo, unsigned int* punGpioGroup, unsigned int* punGpioSck, unsigned int* punGpioMiso, 
                        unsigned int* punGpioMosi, unsigned int* punGpioCS, unsigned int* punAFMappingNo);


/* Private functions ---------------------------------------------------------*/
void HalSPI_GetGpioInfo(unsigned int unSPINo, unsigned int* punGpioGroup, unsigned int* punGpioSck, unsigned int* punGpioMiso, 
                        unsigned int* punGpioMosi, unsigned int* punGpioCS, unsigned int* punAFMappingNo)
{
#if defined(FEATURE_USE_SPI1)
    if ( unSPINo == (unsigned int)SPI1 )
    {
        *punGpioGroup     = GPIOA_GROUP;
        *punGpioSck       = HAL_SPI1_SCK;
        *punGpioMiso      = HAL_SPI1_MISO;
        *punGpioMosi      = HAL_SPI1_MOSI;
        *punGpioCS        = HAL_SPI1_CS;
        *punAFMappingNo   = HAL_GPIO_AF_SPI1;
    }
#endif

#if defined(FEATURE_USE_SPI2)
    if ( unSPINo == (unsigned int)SPI2 )
    {
        *punGpioGroup     = GPIOB_GROUP;
        *punGpioSck       = HAL_SPI2_SCK;
        *punGpioMiso      = HAL_SPI2_MISO;
        *punGpioMosi      = HAL_SPI2_MOSI;
        *punGpioCS        = HAL_SPI2_CS;
        *punAFMappingNo   = HAL_GPIO_AF_SPI2;
    }
#endif

#if defined(FEATURE_USE_SPI3)
    if ( unSPINo == (unsigned int)SPI3 )
    {
        *punGpioGroup     = GPIOC_GROUP;
        *punGpioSck       = HAL_SPI3_SCK;
        *punGpioMiso      = HAL_SPI3_MISO;
        *punGpioMosi      = HAL_SPI3_MOSI;
        *punGpioCS        = HAL_SPI3_CS;
        *punAFMappingNo   = HAL_GPIO_AF_SPI3;
    }
#endif

#if defined(FEATURE_USE_SPI4)
    if ( unSPINo == (unsigned int)SPI4 )
    {
        *punGpioGroup     = GPIOE_GROUP;
        *punGpioSck       = HAL_SPI4_SCK;
        *punGpioMiso      = HAL_SPI4_MISO;
        *punGpioMosi      = HAL_SPI4_MOSI;
        *punGpioCS        = HAL_SPI4_CS;
        *punAFMappingNo   = HAL_GPIO_AF_SPI4;
    }
#endif
}

void HalSPI_LowLevel_Init(unsigned int nSPINo)
{
    stHalGPIO_InitTypeDef GPIO_InitStructure;
    unsigned int unGpioGroup, unGpioSck, unGpioMiso, unGpioMosi, unGpioCS, unAFMappingNo;

    HalSPI_GetGpioInfo(nSPINo, &unGpioGroup, &unGpioSck, &unGpioMiso, &unGpioMosi, &unGpioCS, &unAFMappingNo);


    /*!< Enable GPIO clocks */
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, unGpioGroup, NULL, 0, HAL_ENABLE);

    /*!< SPI pins configuration *************************************************/
    /*!< Connect SPI pins to AF5 */
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, unGpioSck, NULL, 0, unAFMappingNo);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, unGpioMiso, NULL, 0, unAFMappingNo);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, unGpioMosi, NULL, 0, unAFMappingNo);

    GPIO_InitStructure.GPIO_DS    = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = eGPIO_High_Speed;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_DOWN;

    /*!< SPI SCK pin configuration */
    GPIO_InitStructure.GPIO_Pin = unGpioSck;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    /*!< SPI MOSI pin configuration */
    GPIO_InitStructure.GPIO_Pin =  unGpioMiso;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    /*!< SPI MISO pin configuration */
    GPIO_InitStructure.GPIO_Pin =  unGpioMosi;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    /*!< Configure sFLASH Card CS pin in output pushpull mode ********************/
    GPIO_InitStructure.GPIO_Pin   = unGpioCS;
    GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_NOPULL;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
}

void HalSPI_Init()
{
#if defined(FEATURE_USE_SPI1)
    stHalSPI_InitTypeDef  SPI_InitStructure;

    /*!< Enable the SPI clock */
    HalDrvRccIOCtrl(eRCC_IO_SPI_Clock, eRCC_Clock_SPI1, NULL, 0, HAL_ENABLE);

    HalSPI_LowLevel_Init((unsigned int)HAL_SPI1);

    /*!< SPI configuration */
    SPI_InitStructure.SPI_Direction = HAL_SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = HAL_SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = HAL_SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = HAL_SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = HAL_SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = HAL_SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = HAL_SPI_BaudRatePrescaler_8;
    SPI_InitStructure.SPI_FirstBit = HAL_SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 10;

    HalDrvSPIIOCtrl(eSPI_IO_Init, (int)HAL_SPI1, (char*)&SPI_InitStructure, sizeof(stHalSPI_InitTypeDef), 0);

#if defined(SPI1_USE_DMA)
    InitSPIDMAConfig((unsigned int)HAL_SPI1);
#endif
    
    HalDrvSPIIOCtrl(eSPI_IO_Enable, (int)HAL_SPI1, NULL, 0, HAL_ENABLE);
#endif

#if defined(FEATURE_USE_SPI2)
    stHalSPI_InitTypeDef  SPI_InitStructure;

    /*!< Enable the SPI clock */
    HalDrvRccIOCtrl(eRCC_IO_SPI_Clock, eRCC_Clock_SPI2, NULL, 0, HAL_ENABLE);

    HalSPI_LowLevel_Init((unsigned int)HAL_SPI2);

    /*!< SPI configuration */
    SPI_InitStructure.SPI_Direction = HAL_SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = HAL_SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = HAL_SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = HAL_SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = HAL_SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = HAL_SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = HAL_SPI_BaudRatePrescaler_8;
    SPI_InitStructure.SPI_FirstBit = HAL_SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 10;

    HalDrvSPIIOCtrl(eSPI_IO_Init, (int)HAL_SPI2, (char*)&SPI_InitStructure, sizeof(stHalSPI_InitTypeDef), 0);

#if defined(SPI2_USE_DMA)
    InitSPIDMAConfig((unsigned int)HAL_SPI2);
#endif
    
    HalDrvSPIIOCtrl(eSPI_IO_Enable, (int)HAL_SPI2, NULL, 0, HAL_ENABLE);
#endif

#if defined(FEATURE_USE_SPI3)
    stHalSPI_InitTypeDef  SPI_InitStructure;

    /*!< Enable the SPI clock */
    HalDrvRccIOCtrl(eRCC_IO_SPI_Clock, eRCC_Clock_SPI3, NULL, 0, HAL_ENABLE);

    HalSPI_LowLevel_Init((unsigned int)HAL_SPI3);

    /*!< SPI configuration */
    SPI_InitStructure.SPI_Direction = HAL_SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = HAL_SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = HAL_SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = HAL_SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = HAL_SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = HAL_SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = HAL_SPI_BaudRatePrescaler_8;
    SPI_InitStructure.SPI_FirstBit = HAL_SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 10;

    HalDrvSPIIOCtrl(eSPI_IO_Init, (int)HAL_SPI3, (char*)&SPI_InitStructure, sizeof(stHalSPI_InitTypeDef), 0);

#if defined(SPI3_USE_DMA)
    InitSPIDMAConfig((unsigned int)HAL_SPI3);
#endif
    
    HalDrvSPIIOCtrl(eSPI_IO_Enable, (int)HAL_SPI3, NULL, 0, HAL_ENABLE);
#endif

#if defined(FEATURE_USE_SPI4)
    stHalSPI_InitTypeDef  SPI_InitStructure;

    /*!< Enable the SPI clock */
    HalDrvRccIOCtrl(eRCC_IO_SPI_Clock, eRCC_Clock_SPI4, NULL, 0, HAL_ENABLE);

    HalSPI_LowLevel_Init((unsigned int)HAL_SPI4);

    /*!< SPI configuration */
    SPI_InitStructure.SPI_Direction = HAL_SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = HAL_SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = HAL_SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = HAL_SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = HAL_SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = HAL_SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = HAL_SPI_BaudRatePrescaler_8;
    SPI_InitStructure.SPI_FirstBit = HAL_SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 10;

    HalDrvSPIIOCtrl(eSPI_IO_Init, (int)HAL_SPI4, (char*)&SPI_InitStructure, sizeof(stHalSPI_InitTypeDef), 0);

#if defined(SPI4_USE_DMA)
    InitSPIDMAConfig((unsigned int)HAL_SPI4);
#endif
    
    HalDrvSPIIOCtrl(eSPI_IO_Enable, (int)HAL_SPI4, NULL, 0, HAL_ENABLE);
#endif

}

void InitSPIDMAConfig(unsigned int unSPiNo)
{
#if defined(SPI1_USE_DMA) || defined(SPI2_USE_DMA) || defined(SPI3_USE_DMA) || defined(SPI4_USE_DMA)
    stHalDMA_InitTypeDef  DMA_InitStructure;
    unsigned int unDMANo, unRccClock, unDMARxStream, unDMARxChannel, unDMARxBuffSize, unDMARxBufferPoint, unDMATxBufferPoint; 
    
    unsigned int unDMATxStream, unDMATxChannel, unDMATxBuffSize;

    if ( unSPiNo == (unsigned int)SPI1 )
    {
        unDMANo         = (unsigned int)HAL_DMA_SPI1;
        unRccClock      = HAL_DMA_SPI1_CLOCK;
        unDMARxStream   = HAL_DMA_SPI1_RX_STREAM;
        unDMARxChannel  = HAL_DMA_SPI1_RX_CHANNEL;
        unDMARxBuffSize = HAL_DMA_SPI1_RX_BUFFERSIZE;
        unDMARxBufferPoint= (unsigned int)g_arrDMA_SPI1_RxBuffer;
        unDMATxStream   = HAL_DMA_SPI1_TX_CHANNEL;
        unDMATxChannel  = HAL_DMA_SPI1_TX_STREAM;
        unDMATxBuffSize = HAL_DMA_SPI1_TX_BUFFERSIZE;
        unDMATxBufferPoint= (unsigned int)g_arrDMA_SPI1_TxBuffer;
    }
    else if ( unSPiNo == (unsigned int)SPI2 )
    {
        unDMANo         = (unsigned int)HAL_DMA_SPI2;
        unRccClock      = HAL_DMA_SPI2_CLOCK;
        unDMARxStream   = HAL_DMA_SPI2_RX_STREAM;
        unDMARxChannel  = HAL_DMA_SPI2_RX_CHANNEL;
        unDMARxBuffSize = HAL_DMA_SPI2_RX_BUFFERSIZE;
        unDMARxBufferPoint= (unsigned int)g_arrDMA_SPI2_RxBuffer;
        unDMATxStream   = HAL_DMA_SPI2_TX_CHANNEL;
        unDMATxChannel  = HAL_DMA_SPI2_TX_STREAM;
        unDMATxBuffSize = HAL_DMA_SPI2_TX_BUFFERSIZE;
        unDMATxBufferPoint= (unsigned int)g_arrDMA_SPI2_TxBuffer;
    }
    else if ( unSPiNo == (unsigned int)SPI3 )
    {
        unDMANo         = (unsigned int)HAL_DMA_SPI3;
        unRccClock      = HAL_DMA_SPI3_CLOCK;
        unDMARxStream   = HAL_DMA_SPI3_RX_STREAM;
        unDMARxChannel  = HAL_DMA_SPI3_RX_CHANNEL;
        unDMARxBuffSize = HAL_DMA_SPI3_RX_BUFFERSIZE;
        unDMARxBufferPoint= (unsigned int)g_arrDMA_SPI3_RxBuffer;
        unDMATxStream   = HAL_DMA_SPI3_TX_CHANNEL;
        unDMATxChannel  = HAL_DMA_SPI3_TX_STREAM;
        unDMATxBuffSize = HAL_DMA_SPI3_TX_BUFFERSIZE;
        unDMATxBufferPoint= (unsigned int)g_arrDMA_SPI3_TxBuffer;
    }
    else if ( unSPiNo == (unsigned int)SPI4 )
    {
        unDMANo         = (unsigned int)HAL_DMA_SPI4;
        unRccClock      = HAL_DMA_SPI4_CLOCK;
        unDMARxStream   = HAL_DMA_SPI4_RX_STREAM;
        unDMARxChannel  = HAL_DMA_SPI4_RX_CHANNEL;
        unDMARxBuffSize = HAL_DMA_SPI4_RX_BUFFERSIZE;
        unDMARxBufferPoint= (unsigned int)g_arrDMA_SPI4_RxBuffer;
        unDMATxStream   = HAL_DMA_SPI4_TX_CHANNEL;
        unDMATxChannel  = HAL_DMA_SPI4_TX_STREAM;
        unDMATxBuffSize = HAL_DMA_SPI4_TX_BUFFERSIZE;
        unDMATxBufferPoint= (unsigned int)g_arrDMA_SPI4_TxBuffer;
    }

    /* Enable the DMA clock */
    HalDrvRccIOCtrl(eRCC_IO_DMA_Clock, unRccClock, NULL, 0, HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_MuxEnable, (int)unDMANo, NULL, 0, HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_DeInit, (int)unDMARxStream, NULL, 0, 0);

    /* Configure DMA Initialization Structure */
    DMA_InitStructure.DMA_FIFOMode = HAL_DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = HAL_DMA_FIFOThreshold_1QuarterFull;
    DMA_InitStructure.DMA_MemoryBurst = HAL_DMA_MemoryBurst_Single ;
    DMA_InitStructure.DMA_MemoryDataSize = HAL_DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = HAL_DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_Mode = HAL_DMA_Mode_Circular;
    DMA_InitStructure.DMA_PeripheralBaseAddr = HAL_SPI_DMA_ADDR(((SPI_TypeDef*)unSPiNo));
    DMA_InitStructure.DMA_PeripheralBurst = HAL_DMA_PeripheralBurst_Single;
    DMA_InitStructure.DMA_PeripheralDataSize = HAL_DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = HAL_DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_Priority = HAL_DMA_Priority_High;//HAL_DMA_Priority_VeryHigh/*DMA_Priority_High*/;

    /* Configure RX DMA */
    DMA_InitStructure.DMA_BufferSize = unDMARxBuffSize;
    DMA_InitStructure.DMA_Channel = unDMARxChannel;
    DMA_InitStructure.DMA_DIR = HAL_DMA_DIR_PeripheralToMemory ;
    DMA_InitStructure.DMA_Memory0BaseAddr = unDMARxBufferPoint;
    HalDrvDmaIOCtrl(eDMA_IO_Init, (int)unDMARxStream, (char*)&DMA_InitStructure, sizeof(DMA_InitStructure), HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_MuxInit, (int)unDMANo, NULL, unDMARxStream, HAL_DMA_RX_MODE);

    /* Configure TX DMA */
    DMA_InitStructure.DMA_BufferSize = unDMATxBuffSize;
    DMA_InitStructure.DMA_Channel = unDMATxChannel;
    DMA_InitStructure.DMA_DIR = HAL_DMA_DIR_MemoryToPeripheral ;
    DMA_InitStructure.DMA_Memory0BaseAddr = unDMATxBufferPoint;
    HalDrvDmaIOCtrl(eDMA_IO_Init, (int)unDMARxStream, (char*)&DMA_InitStructure, sizeof(DMA_InitStructure), HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_MuxInit, (int)unDMANo, NULL, unDMARxStream, HAL_DMA_TX_MODE);
    
    /* Enable USART DMA RX Requsts */
    HalDrvSPIIOCtrl(eSPI_IO_DMA_RTX_Enable, (int)unSPiNo, NULL, HAL_SPI_I2S_DMAReq_Rx, HAL_ENABLE);
    HalDrvSPIIOCtrl(eSPI_IO_DMA_RTX_Enable, (int)unSPiNo, NULL, HAL_SPI_I2S_DMAReq_Tx, HAL_ENABLE);

    /* Enable DMA USART RX Stream */
    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, (int)unDMARxStream, NULL, 0, HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, (int)unDMATxStream, NULL, 0, HAL_ENABLE);

    g_unDMA_SPI1_RxPos = HalDrvDmaIOCtrl(eDMA_IO_GetDMACount, (int)unDMARxStream, NULL, 0, 0);

    /* Waiting the end of Data transfer */
    if(HalDrvSPIIOCtrl(eSPI_IO_GetFlagStatus, (unsigned int)unSPiNo, NULL, 0, HAL_SPI_I2S_FLAG_TXE) == HAL_SET) {
      	Trace(" Start SPI TX DMA...\r\n");
	}
    if(HalDrvSPIIOCtrl(eSPI_IO_GetFlagStatus, (unsigned int)unSPiNo, NULL, 0, HAL_SPI_I2S_FLAG_RXNE) == HAL_SET) {
      	Trace(" Start SPI RX DMA...\r\n");
	}
	else {
		Trace("\r\nFail Init SPI DMA\r\n");
		while(1);
	}
#endif
}


//-----------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------//

int HalDrvSPIOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    sFLASH_Init();

    return HAL_RETURN_SUCCESS;
}

int HalDrvSPIRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
#if defined(STM32F427X)
    return SPI_I2S_ReceiveData((SPI_TypeDef*)nLparam);
#elif defined(AT32F435VMT7)
    return spi_i2s_data_receive((spi_type*)nLparam);
#endif
}

int HalDrvSPIWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
#if defined(STM32F427X)
        SPI_I2S_SendData((SPI_TypeDef*)nLparam, (unsigned short)*pBuffer);
#elif defined(AT32F435VMT7)
        spi_i2s_data_transmit((spi_type*)nLparam, (unsigned short)*pBuffer);
#endif
    return HAL_RETURN_SUCCESS;
}

int HalDrvSPIIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    eHalSPI_IOCtlMode nIoCtlMode = (eHalSPI_IOCtlMode)nLparam;
 
    switch( nIoCtlMode )
    {
        case eSPI_IO_Init:
        {
            stHalSPI_InitTypeDef* pHalSPIInitType = (stHalSPI_InitTypeDef*)pBuffer;

#if defined(STM32F427X)
            SPI_TypeDef* pSPIPortAddr = (SPI_TypeDef*)nRparam;
            SPI_InitTypeDef SPIInitTypeStruct;
            memcpy(&SPIInitTypeStruct, pHalSPIInitType, sizeof(SPI_InitTypeDef));
            SPI_Init(pSPIPortAddr, &SPIInitTypeStruct);

#elif defined(AT32F435VMT7)
            spi_init_type SPIInitTypeStruct;
            spi_default_para_init(&SPIInitTypeStruct);
            spi_type* pSPIPortAddr = (spi_type*)nRparam;

            if      ( pHalSPIInitType->SPI_Direction == HAL_SPI_Direction_2Lines_FullDuplex )
                SPIInitTypeStruct.transmission_mode = SPI_TRANSMIT_FULL_DUPLEX;
            else if ( pHalSPIInitType->SPI_Direction == HAL_SPI_Direction_2Lines_RxOnly )
                SPIInitTypeStruct.transmission_mode = SPI_TRANSMIT_SIMPLEX_RX;
            else if ( pHalSPIInitType->SPI_Direction == HAL_SPI_Direction_1Line_Rx )
                SPIInitTypeStruct.transmission_mode = SPI_TRANSMIT_HALF_DUPLEX_RX;
            else if ( pHalSPIInitType->SPI_Direction == HAL_SPI_Direction_1Line_Tx )
                SPIInitTypeStruct.transmission_mode = SPI_TRANSMIT_HALF_DUPLEX_TX;
            
            if      ( pHalSPIInitType->SPI_Mode == HAL_SPI_Mode_Master )
                SPIInitTypeStruct.master_slave_mode = SPI_MODE_MASTER;
            else if ( pHalSPIInitType->SPI_Mode == HAL_SPI_Mode_Slave )
                SPIInitTypeStruct.master_slave_mode = SPI_MODE_SLAVE;

            if      ( pHalSPIInitType->SPI_DataSize == HAL_SPI_DataSize_8b )
                SPIInitTypeStruct.frame_bit_num = SPI_FRAME_8BIT;
            else if ( pHalSPIInitType->SPI_DataSize == HAL_SPI_DataSize_16b )
                SPIInitTypeStruct.frame_bit_num = SPI_FRAME_16BIT;

            if      ( pHalSPIInitType->SPI_CPOL == HAL_SPI_CPOL_High )
                SPIInitTypeStruct.clock_polarity = SPI_CLOCK_POLARITY_HIGH;
            else if ( pHalSPIInitType->SPI_CPOL == HAL_SPI_CPOL_Low )
                SPIInitTypeStruct.clock_polarity = SPI_CLOCK_POLARITY_LOW;

            if      ( pHalSPIInitType->SPI_CPHA == HAL_SPI_CPHA_1Edge )
                SPIInitTypeStruct.clock_phase = SPI_CLOCK_PHASE_1EDGE;
            else if ( pHalSPIInitType->SPI_CPHA == HAL_SPI_CPHA_2Edge )
                SPIInitTypeStruct.clock_phase = SPI_CLOCK_PHASE_2EDGE;

            if      ( pHalSPIInitType->SPI_NSS == HAL_SPI_NSS_Soft )
                SPIInitTypeStruct.cs_mode_selection = SPI_CS_SOFTWARE_MODE;
            else if ( pHalSPIInitType->SPI_NSS == HAL_SPI_NSS_Hard )
                SPIInitTypeStruct.cs_mode_selection = SPI_CS_HARDWARE_MODE;
            
            if      ( pHalSPIInitType->SPI_BaudRatePrescaler == HAL_SPI_BaudRatePrescaler_2 )
                SPIInitTypeStruct.mclk_freq_division = SPI_MCLK_DIV_2;
            else if ( pHalSPIInitType->SPI_BaudRatePrescaler == HAL_SPI_BaudRatePrescaler_4 )
                SPIInitTypeStruct.mclk_freq_division = SPI_MCLK_DIV_4;
            else if ( pHalSPIInitType->SPI_BaudRatePrescaler == HAL_SPI_BaudRatePrescaler_8 )
                SPIInitTypeStruct.mclk_freq_division = SPI_MCLK_DIV_8;
            else if ( pHalSPIInitType->SPI_BaudRatePrescaler == HAL_SPI_BaudRatePrescaler_16 )
                SPIInitTypeStruct.mclk_freq_division = SPI_MCLK_DIV_16;
            else if ( pHalSPIInitType->SPI_BaudRatePrescaler == HAL_SPI_BaudRatePrescaler_32 )
                SPIInitTypeStruct.mclk_freq_division = SPI_MCLK_DIV_32;
            else if ( pHalSPIInitType->SPI_BaudRatePrescaler == HAL_SPI_BaudRatePrescaler_64 )
                SPIInitTypeStruct.mclk_freq_division = SPI_MCLK_DIV_64;
            else if ( pHalSPIInitType->SPI_BaudRatePrescaler == HAL_SPI_BaudRatePrescaler_128 )
                SPIInitTypeStruct.mclk_freq_division = SPI_MCLK_DIV_128;
            else if ( pHalSPIInitType->SPI_BaudRatePrescaler == HAL_SPI_BaudRatePrescaler_256 )
                SPIInitTypeStruct.mclk_freq_division = SPI_MCLK_DIV_256;
            // SPI_MCLK_DIV_512, SPI_MCLK_DIV_1024        


            if      ( pHalSPIInitType->SPI_FirstBit == HAL_SPI_FirstBit_MSB )
                SPIInitTypeStruct.first_bit_transmission = SPI_FIRST_BIT_MSB;
            else if ( pHalSPIInitType->SPI_FirstBit == HAL_SPI_FirstBit_LSB )
                SPIInitTypeStruct.first_bit_transmission = SPI_FIRST_BIT_LSB;

            if      ( pHalSPIInitType->SPI_CRCPolynomial == 7 )
                SPIInitTypeStruct.frame_bit_num = SPI_FRAME_8BIT;
            else if ( pHalSPIInitType->SPI_FirstBit == HAL_SPI_FirstBit_MSB )
                SPIInitTypeStruct.frame_bit_num = SPI_FRAME_8BIT;

            spi_init((spi_type*)pSPIPortAddr, &SPIInitTypeStruct);
#endif
        }
            break;
        case eSPI_IO_DeInit:
#if defined(STM32F427X)
            SPI_I2S_DeInit((SPI_TypeDef*)nRparam);
#elif defined(AT32F435VMT7)
            spi_i2s_reset((spi_type*)nRparam);
#endif
            break;
        case eSPI_IO_Enable:
        {
            int nEnable = nOverlap;
#if defined(STM32F427X)
            SPI_TypeDef* pSPIPortAddr = (SPI_TypeDef*)nRparam;
            SPI_Cmd(pSPIPortAddr, (FunctionalState)nEnable);
#elif defined(AT32F435VMT7)
            spi_type* pSPIPortAddr = (spi_type*)nRparam;
            spi_enable(pSPIPortAddr, (confirm_state)nEnable);
#endif
        }
            break;
        case eSPI_IO_GetFlagStatus:
        {
            unsigned short usFlags = nOverlap;
#if defined(STM32F427X)
            SPI_TypeDef* pSPIPortAddr = (SPI_TypeDef*)nRparam;
            return SPI_I2S_GetFlagStatus(pSPIPortAddr, usFlags);
#elif defined(AT32F435VMT7)
            spi_type* pSPIPortAddr = (spi_type*)nRparam;
            return spi_i2s_flag_get(pSPIPortAddr, usFlags);
#endif
        }
            break;
        case eSPI_IO_DMA_RTX_Enable:
        {
            int nDMAReq = nLength;
#if defined(STM32F427X)
            SPI_I2S_DMACmd((SPI_TypeDef*)nRparam, nDMAReq, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            if ( nDMAReq == HAL_SPI_I2S_DMAReq_Rx )
                spi_i2s_dma_receiver_enable((spi_type*)nRparam, (confirm_state)nOverlap);
            else if( nDMAReq == HAL_SPI_I2S_DMAReq_Tx )
                spi_i2s_dma_transmitter_enable((spi_type*)nRparam, (confirm_state)nOverlap);
#endif
        }
            break;
        default:
            break;
    }
        
    return HAL_RETURN_SUCCESS;
}

int HalDrvSPIClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

