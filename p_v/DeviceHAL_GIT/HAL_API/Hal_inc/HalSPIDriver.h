#ifndef __HAL_SPI_DRIVER_H__
#define __HAL_SPI_DRIVER_H__


/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "HalHandler.h"

/* Exported define #1---------------------------------------------------------*/
//#define FEATURE_USE_SPI1
//#define FEATURE_USE_SPI2
//#define FEATURE_USE_SPI3
//#define FEATURE_USE_SPI4

#if defined(FEATURE_USE_SPI1)
#define HAL_SPI1                SPI1
#define HAL_SPI1_SCK            SPI1_SCK_Pin
#define HAL_SPI1_MISO           SPI1_MISO_Pin
#define HAL_SPI1_MOSI           SPI1_MOSI_Pin
#define HAL_SPI1_CS             SPI1_NSS_Pin

//#define SPI1_USE_POLLING
#define SPI1_USE_DMA
#endif

#if defined(FEATURE_USE_SPI2)
#define HAL_SPI2                SPI2
#define HAL_SPI2_SCK            SPI2_SCK_Pin
#define HAL_SPI2_MISO           SPI2_MISO_Pin
#define HAL_SPI2_MOSI           SPI2_MOSI_Pin
#define HAL_SPI2_CS             SPI2_NSS_Pin

//#define SPI2_USE_POLLING
#define SPI2_USE_DMA
#endif
#if defined(FEATURE_USE_SPI3)
#define HAL_SPI3                SPI3
#define HAL_SPI3_SCK            SPI3_SCK_Pin
#define HAL_SPI3_MISO           SPI3_MISO_Pin
#define HAL_SPI3_MOSI           SPI3_MOSI_Pin
#define HAL_SPI3_CS             SPI3_NSS_Pin

//#define SPI3_USE_POLLING
#define SPI3_USE_DMA
#endif
#if defined(FEATURE_USE_SPI4)
#define HAL_SPI4                SPI4
#define HAL_SPI4_SCK            SPI4_SCK_Pin
#define HAL_SPI4_MISO           SPI4_MISO_Pin
#define HAL_SPI4_MOSI           SPI4_MOSI_Pin
#define HAL_SPI4_CS             SPI4_NSS_Pin

//#define SPI4_USE_POLLING
#define SPI4_USE_DMA
#endif

#define HAL_SPI_I2S_DMAReq_Tx               ((uint16_t)0x0002)
#define HAL_SPI_I2S_DMAReq_Rx               ((uint16_t)0x0001)


/* Exported define #2---------------------------------------------------------*/
// @defgroup SPI_data_direction 
#define HAL_SPI_Direction_2Lines_FullDuplex ((uint16_t)0x0000)
#define HAL_SPI_Direction_2Lines_RxOnly     ((uint16_t)0x0400)
#define HAL_SPI_Direction_1Line_Rx          ((uint16_t)0x8000)
#define HAL_SPI_Direction_1Line_Tx          ((uint16_t)0xC000)
// @defgroup SPI_mode 
#define HAL_SPI_Mode_Master                 ((uint16_t)0x0104)
#define HAL_SPI_Mode_Slave                  ((uint16_t)0x0000)
// @defgroup SPI_data_size 
#define HAL_SPI_DataSize_16b                ((uint16_t)0x0800)
#define HAL_SPI_DataSize_8b                 ((uint16_t)0x0000)
// @defgroup SPI_Clock_Polarity 
#define HAL_SPI_CPOL_Low                    ((uint16_t)0x0000)
#define HAL_SPI_CPOL_High                   ((uint16_t)0x0002)
// @defgroup SPI_Clock_Phase 
#define HAL_SPI_CPHA_1Edge                  ((uint16_t)0x0000)
#define HAL_SPI_CPHA_2Edge                  ((uint16_t)0x0001)
// @defgroup SPI_Slave_Select_management 
#define HAL_SPI_NSS_Soft                    ((uint16_t)0x0200)
#define HAL_SPI_NSS_Hard                    ((uint16_t)0x0000)
// @defgroup SPI_BaudRate_Prescaler 
#define HAL_SPI_BaudRatePrescaler_2         ((uint16_t)0x0000)
#define HAL_SPI_BaudRatePrescaler_4         ((uint16_t)0x0008)
#define HAL_SPI_BaudRatePrescaler_8         ((uint16_t)0x0010)
#define HAL_SPI_BaudRatePrescaler_16        ((uint16_t)0x0018)
#define HAL_SPI_BaudRatePrescaler_32        ((uint16_t)0x0020)
#define HAL_SPI_BaudRatePrescaler_64        ((uint16_t)0x0028)
#define HAL_SPI_BaudRatePrescaler_128       ((uint16_t)0x0030)
#define HAL_SPI_BaudRatePrescaler_256       ((uint16_t)0x0038)
// @defgroup SPI_MSB_LSB_transmission 
#define HAL_SPI_FirstBit_MSB                ((uint16_t)0x0000)
#define HAL_SPI_FirstBit_LSB                ((uint16_t)0x0080)


// @defgroup SPI_I2S_flags_definition 
#define HAL_SPI_I2S_FLAG_RXNE               ((uint16_t)0x0001)
#define HAL_SPI_I2S_FLAG_TXE                ((uint16_t)0x0002)
#define HAL_I2S_FLAG_CHSIDE                 ((uint16_t)0x0004)
#define HAL_I2S_FLAG_UDR                    ((uint16_t)0x0008)
#define HAL_SPI_FLAG_CRCERR                 ((uint16_t)0x0010)
#define HAL_SPI_FLAG_MODF                   ((uint16_t)0x0020)
#define HAL_SPI_I2S_FLAG_OVR                ((uint16_t)0x0040)
#define HAL_SPI_I2S_FLAG_BSY                ((uint16_t)0x0080)
#define HAL_SPI_I2S_FLAG_TIFRFE             ((uint16_t)0x0100)


// @defgroup SPI_I2S_interrupts_definition 
#define HAL_SPI_I2S_IT_TXE                  ((uint8_t)0x71)
#define HAL_SPI_I2S_IT_RXNE                 ((uint8_t)0x60)
#define HAL_SPI_I2S_IT_ERR                  ((uint8_t)0x50)
#define HAL_I2S_IT_UDR                      ((uint8_t)0x53)
#define HAL_SPI_I2S_IT_TIFRFE               ((uint8_t)0x58)
#define HAL_SPI_I2S_IT_OVR                  ((uint8_t)0x56)
#define HAL_SPI_IT_MODF                     ((uint8_t)0x55)
#define HAL_SPI_IT_CRCERR                   ((uint8_t)0x54)


/* Exported types - Structure, Enumeration -----------------------------------*/
typedef enum __eHalSPIIoCtlMode{
    eSPI_IO_Init,
    eSPI_IO_DeInit,
    eSPI_IO_Enable,
    eSPI_IO_GetFlagStatus,
    eSPI_IO_DMA_RTX_Enable,
}eHalSPI_IOCtlMode;


typedef __packed struct __stHalSPI_InitTypeDef
{
  unsigned short SPI_Direction;           /*!< Specifies the SPI unidirectional or bidirectional data mode.
                                         This parameter can be a value of @ref SPI_data_direction */

  unsigned short SPI_Mode;                /*!< Specifies the SPI operating mode.
                                         This parameter can be a value of @ref SPI_mode */

  unsigned short SPI_DataSize;            /*!< Specifies the SPI data size.
                                         This parameter can be a value of @ref SPI_data_size */

  unsigned short SPI_CPOL;                /*!< Specifies the serial clock steady state.
                                         This parameter can be a value of @ref SPI_Clock_Polarity */

  unsigned short SPI_CPHA;                /*!< Specifies the clock active edge for the bit capture.
                                         This parameter can be a value of @ref SPI_Clock_Phase */

  unsigned short SPI_NSS;                 /*!< Specifies whether the NSS signal is managed by
                                         hardware (NSS pin) or by software using the SSI bit.
                                         This parameter can be a value of @ref SPI_Slave_Select_management */
 
  unsigned short SPI_BaudRatePrescaler;   /*!< Specifies the Baud Rate prescaler value which will be
                                         used to configure the transmit and receive SCK clock.
                                         This parameter can be a value of @ref SPI_BaudRate_Prescaler
                                         @note The communication clock is derived from the master
                                               clock. The slave clock does not need to be set. */

  unsigned short SPI_FirstBit;            /*!< Specifies whether data transfers start from MSB or LSB bit.
                                         This parameter can be a value of @ref SPI_MSB_LSB_transmission */

  unsigned short SPI_CRCPolynomial;       /*!< Specifies the polynomial used for the CRC calculation. */
}stHalSPI_InitTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

int HalDrvSPIOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvSPIRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvSPIWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvSPIIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvSPIClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);


#endif //__HAL_SPI_DRIVER_H__
