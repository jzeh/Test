#ifndef __HAL_RCC_DRIVER_H__
#define __HAL_RCC_DRIVER_H__
/*
  ******************************************************************************
  * @file    HalRccDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *  Reset & Clock Control file
  *
  ******************************************************************************
*/


/* include -------------------------------------------------------------------*/
#include "common.h"

/* Exported define -----------------------------------------------------------*/
#define HAL_RCC_SYSCLKSource_HSI             ((uint32_t)0x00000000)
#define HAL_RCC_SYSCLKSource_HSE             ((uint32_t)0x00000001)
#define HAL_RCC_SYSCLKSource_PLLCLK          ((uint32_t)0x00000002)
/*
#define RCC_AHB1Periph_GPIOA             ((unsigned int)0x00000001)
#define RCC_AHB1Periph_GPIOB             ((unsigned int)0x00000002)
#define RCC_AHB1Periph_GPIOC             ((unsigned int)0x00000004)
#define RCC_AHB1Periph_GPIOD             ((unsigned int)0x00000008)
#define RCC_AHB1Periph_GPIOE             ((unsigned int)0x00000010)
#define RCC_AHB1Periph_GPIOF             ((unsigned int)0x00000020)
#define RCC_AHB1Periph_GPIOG             ((unsigned int)0x00000040)
#define RCC_AHB1Periph_GPIOH             ((unsigned int)0x00000080)
#define RCC_AHB1Periph_GPIOI             ((unsigned int)0x00000100) 
#define RCC_AHB1Periph_GPIOJ             ((unsigned int)0x00000200)
#define RCC_AHB1Periph_GPIOK             ((unsigned int)0x00000400)
#define RCC_AHB1Periph_CRC               ((unsigned int)0x00001000)
#define RCC_AHB1Periph_FLITF             ((unsigned int)0x00008000)
#define RCC_AHB1Periph_SRAM1             ((unsigned int)0x00010000)
#define RCC_AHB1Periph_SRAM2             ((unsigned int)0x00020000)
#define RCC_AHB1Periph_BKPSRAM           ((unsigned int)0x00040000)
#define RCC_AHB1Periph_SRAM3             ((unsigned int)0x00080000)
#define RCC_AHB1Periph_CCMDATARAMEN      ((unsigned int)0x00100000)
#define RCC_AHB1Periph_DMA1              ((unsigned int)0x00200000)
#define RCC_AHB1Periph_DMA2              ((unsigned int)0x00400000)
#define RCC_AHB1Periph_DMA2D             ((unsigned int)0x00800000)
#define RCC_AHB1Periph_ETH_MAC           ((unsigned int)0x02000000)
#define RCC_AHB1Periph_ETH_MAC_Tx        ((unsigned int)0x04000000)
#define RCC_AHB1Periph_ETH_MAC_Rx        ((unsigned int)0x08000000)
#define RCC_AHB1Periph_ETH_MAC_PTP       ((unsigned int)0x10000000)
#define RCC_AHB1Periph_OTG_HS            ((unsigned int)0x20000000)
#define RCC_AHB1Periph_OTG_HS_ULPI       ((unsigned int)0x40000000)
#define RCC_AHB2Periph_DCMI              ((unsigned int)0x00000001)
#define RCC_AHB2Periph_CRYP              ((unsigned int)0x00000010)
#define RCC_AHB2Periph_HASH              ((unsigned int)0x00000020)
#define RCC_AHB2Periph_RNG               ((unsigned int)0x00000040)
#define RCC_AHB2Periph_OTG_FS            ((unsigned int)0x00000080)
#define RCC_AHB3Periph_FMC                 ((unsigned int)0x00000001)
#define RCC_APB1Periph_TIM2              ((uint32_t)0x00000001)
#define RCC_APB1Periph_TIM3              ((uint32_t)0x00000002)
#define RCC_APB1Periph_TIM4              ((uint32_t)0x00000004)
#define RCC_APB1Periph_TIM5              ((uint32_t)0x00000008)
#define RCC_APB1Periph_TIM6              ((uint32_t)0x00000010)
#define RCC_APB1Periph_TIM7              ((uint32_t)0x00000020)
#define RCC_APB1Periph_TIM12             ((uint32_t)0x00000040)
#define RCC_APB1Periph_TIM13             ((uint32_t)0x00000080)
#define RCC_APB1Periph_TIM14             ((uint32_t)0x00000100)
#define RCC_APB1Periph_WWDG              ((uint32_t)0x00000800)
#define RCC_APB1Periph_SPI2              ((uint32_t)0x00004000)
#define RCC_APB1Periph_SPI3              ((uint32_t)0x00008000)
#define RCC_APB1Periph_USART2            ((uint32_t)0x00020000)
#define RCC_APB1Periph_USART3            ((uint32_t)0x00040000)
#define RCC_APB1Periph_UART4             ((uint32_t)0x00080000)
#define RCC_APB1Periph_UART5             ((uint32_t)0x00100000)
#define RCC_APB1Periph_I2C1              ((uint32_t)0x00200000)
#define RCC_APB1Periph_I2C2              ((uint32_t)0x00400000)
#define RCC_APB1Periph_I2C3              ((uint32_t)0x00800000)
#define RCC_APB1Periph_CAN1              ((uint32_t)0x02000000)
#define RCC_APB1Periph_CAN2              ((uint32_t)0x04000000)
#define RCC_APB1Periph_PWR               ((uint32_t)0x10000000)
#define RCC_APB1Periph_DAC               ((uint32_t)0x20000000)
#define RCC_APB1Periph_UART7             ((uint32_t)0x40000000)
#define RCC_APB1Periph_UART8             ((uint32_t)0x80000000)
#define RCC_APB2Periph_TIM1              ((uint32_t)0x00000001)
#define RCC_APB2Periph_TIM8              ((uint32_t)0x00000002)
#define RCC_APB2Periph_USART1            ((uint32_t)0x00000010)
#define RCC_APB2Periph_USART6            ((uint32_t)0x00000020)
#define RCC_APB2Periph_ADC               ((uint32_t)0x00000100)
#define RCC_APB2Periph_ADC1              ((uint32_t)0x00000100)
#define RCC_APB2Periph_ADC2              ((uint32_t)0x00000200)
#define RCC_APB2Periph_ADC3              ((uint32_t)0x00000400)
#define RCC_APB2Periph_SDIO              ((uint32_t)0x00000800)
#define RCC_APB2Periph_SPI1              ((uint32_t)0x00001000)
#define RCC_APB2Periph_SPI4              ((uint32_t)0x00002000)
#define RCC_APB2Periph_SYSCFG            ((uint32_t)0x00004000)
#define RCC_APB2Periph_TIM9              ((uint32_t)0x00010000)
#define RCC_APB2Periph_TIM10             ((uint32_t)0x00020000)
#define RCC_APB2Periph_TIM11             ((uint32_t)0x00040000)
#define RCC_APB2Periph_SPI5              ((uint32_t)0x00100000)
#define RCC_APB2Periph_SPI6              ((uint32_t)0x00200000)
#define RCC_APB2Periph_SAI1              ((uint32_t)0x00400000)
*/

/** @defgroup RCC_Flag */
#define HAL_RCC_FLAG_HSIRDY                  ((uint8_t)0x21)    // HSI oscillator clock ready
#define HAL_RCC_FLAG_HSERDY                  ((uint8_t)0x31)    // HSE oscillator clock ready
#define HAL_RCC_FLAG_PLLRDY                  ((uint8_t)0x39)    // main PLL clock ready
#define HAL_RCC_FLAG_PLLI2SRDY               ((uint8_t)0x3B)    // PLLI2S clock ready
#define HAL_RCC_FLAG_PLLSAIRDY               ((uint8_t)0x3D)    // PLLSAI clock ready 
#define HAL_RCC_FLAG_LSERDY                  ((uint8_t)0x41)    // LSE oscillator clock ready
#define HAL_RCC_FLAG_LSIRDY                  ((uint8_t)0x61)    // LSI oscillator clock ready
#define HAL_RCC_FLAG_BORRST                  ((uint8_t)0x79)    // POR/PDR or BOR reset
#define HAL_RCC_FLAG_PINRST                  ((uint8_t)0x7A)    // Pin reset
#define HAL_RCC_FLAG_PORRST                  ((uint8_t)0x7B)    // POR/PDR reset
#define HAL_RCC_FLAG_SFTRST                  ((uint8_t)0x7C)    // Software reset
#define HAL_RCC_FLAG_IWDGRST                 ((uint8_t)0x7D)    // Independent Watchdog reset
#define HAL_RCC_FLAG_WWDGRST                 ((uint8_t)0x7E)    // Window Watchdog reset
#define HAL_RCC_FLAG_LPWRRST                 ((uint8_t)0x7F)    // Low Power reset

// @defgroup RCC_HSE_configuration 
#define HAL_RCC_HSE_OFF                      ((uint8_t)0x00)
#define HAL_RCC_HSE_ON                       ((uint8_t)0x01)
#define HAL_RCC_HSE_Bypass                   ((uint8_t)0x05)

// @defgroup RCC_MCO1_Clock_Source_Prescaler
#define HAL_RCC_MCO1Source_HSI               ((uint32_t)0x00000000)
#define HAL_RCC_MCO1Source_LSE               ((uint32_t)0x00200000)
#define HAL_RCC_MCO1Source_HSE               ((uint32_t)0x00400000)
#define HAL_RCC_MCO1Source_PLLCLK            ((uint32_t)0x00600000)
#define HAL_RCC_MCO1Div_1                    ((uint32_t)0x00000000)
#define HAL_RCC_MCO1Div_2                    ((uint32_t)0x04000000)
#define HAL_RCC_MCO1Div_3                    ((uint32_t)0x05000000)
#define HAL_RCC_MCO1Div_4                    ((uint32_t)0x06000000)
#define HAL_RCC_MCO1Div_5                    ((uint32_t)0x07000000)

// @defgroup RCC_MCO2_Clock_Source_Prescaler
#define HAL_RCC_MCO2Source_SYSCLK            ((uint32_t)0x00000000)
#define HAL_RCC_MCO2Source_PLLI2SCLK         ((uint32_t)0x40000000)
#define HAL_RCC_MCO2Source_HSE               ((uint32_t)0x80000000)
#define HAL_RCC_MCO2Source_PLLCLK            ((uint32_t)0xC0000000)
#define HAL_RCC_MCO2Div_1                    ((uint32_t)0x00000000)
#define HAL_RCC_MCO2Div_2                    ((uint32_t)0x20000000)
#define HAL_RCC_MCO2Div_3                    ((uint32_t)0x28000000)
#define HAL_RCC_MCO2Div_4                    ((uint32_t)0x30000000)
#define HAL_RCC_MCO2Div_5                    ((uint32_t)0x38000000)

// @defgroup RCC_LSE_Configuration 
#define HAL_RCC_LSE_OFF                      ((uint8_t)0x00)
#define HAL_RCC_LSE_ON                       ((uint8_t)0x01)
#define HAL_RCC_LSE_Bypass                   ((uint8_t)0x04)

// @defgroup RCC_RTC_Clock_Source
#define HAL_RCC_RTCCLKSource_LSE             ((uint32_t)0x00000100)
#define HAL_RCC_RTCCLKSource_LSI             ((uint32_t)0x00000200)
#define HAL_RCC_RTCCLKSource_HSE_Div2        ((uint32_t)0x00020300)
#define HAL_RCC_RTCCLKSource_HSE_Div3        ((uint32_t)0x00030300)
#define HAL_RCC_RTCCLKSource_HSE_Div4        ((uint32_t)0x00040300)
#define HAL_RCC_RTCCLKSource_HSE_Div5        ((uint32_t)0x00050300)
#define HAL_RCC_RTCCLKSource_HSE_Div6        ((uint32_t)0x00060300)
#define HAL_RCC_RTCCLKSource_HSE_Div7        ((uint32_t)0x00070300)
#define HAL_RCC_RTCCLKSource_HSE_Div8        ((uint32_t)0x00080300)
#define HAL_RCC_RTCCLKSource_HSE_Div9        ((uint32_t)0x00090300)
#define HAL_RCC_RTCCLKSource_HSE_Div10       ((uint32_t)0x000A0300)
#define HAL_RCC_RTCCLKSource_HSE_Div11       ((uint32_t)0x000B0300)
#define HAL_RCC_RTCCLKSource_HSE_Div12       ((uint32_t)0x000C0300)
#define HAL_RCC_RTCCLKSource_HSE_Div13       ((uint32_t)0x000D0300)
#define HAL_RCC_RTCCLKSource_HSE_Div14       ((uint32_t)0x000E0300)
#define HAL_RCC_RTCCLKSource_HSE_Div15       ((uint32_t)0x000F0300)
#define HAL_RCC_RTCCLKSource_HSE_Div16       ((uint32_t)0x00100300)
#define HAL_RCC_RTCCLKSource_HSE_Div17       ((uint32_t)0x00110300)
#define HAL_RCC_RTCCLKSource_HSE_Div18       ((uint32_t)0x00120300)
#define HAL_RCC_RTCCLKSource_HSE_Div19       ((uint32_t)0x00130300)
#define HAL_RCC_RTCCLKSource_HSE_Div20       ((uint32_t)0x00140300)
#define HAL_RCC_RTCCLKSource_HSE_Div21       ((uint32_t)0x00150300)
#define HAL_RCC_RTCCLKSource_HSE_Div22       ((uint32_t)0x00160300)
#define HAL_RCC_RTCCLKSource_HSE_Div23       ((uint32_t)0x00170300)
#define HAL_RCC_RTCCLKSource_HSE_Div24       ((uint32_t)0x00180300)
#define HAL_RCC_RTCCLKSource_HSE_Div25       ((uint32_t)0x00190300)
#define HAL_RCC_RTCCLKSource_HSE_Div26       ((uint32_t)0x001A0300)
#define HAL_RCC_RTCCLKSource_HSE_Div27       ((uint32_t)0x001B0300)
#define HAL_RCC_RTCCLKSource_HSE_Div28       ((uint32_t)0x001C0300)
#define HAL_RCC_RTCCLKSource_HSE_Div29       ((uint32_t)0x001D0300)
#define HAL_RCC_RTCCLKSource_HSE_Div30       ((uint32_t)0x001E0300)
#define HAL_RCC_RTCCLKSource_HSE_Div31       ((uint32_t)0x001F0300)


/* Exported types - Structure, Enumeration -----------------------------------*/

typedef enum _eHalRccClock
{
    eRCC_IO_SYSCFG_Clock,
    eRCC_IO_GETSYS_Clock,
    eRCC_IO_SetSourceClock,
    eRCC_IO_GetSourceClock,
    eRCC_IO_GET_FLAG_STATUS,
    eRCC_IO_Clear_FLAG_STATUS,
    eRCC_IO_SET_LSE_CONFIG,
    eRCC_IO_SET_HSE_CONFIG,
    eRCC_IO_SET_RTC_Clock_CONFIG,
    eRCC_IO_SET_RTC_Clock_ENABLE,
    eRCC_IO_SET_PLL_ENABLE,
    eRCC_IO_GPIO_Clock,
    eRCC_IO_UART_Clock,
    eRCC_IO_UART_ResetClock,
    eRCC_IO_SPI_Clock,
    eRCC_IO_I2C_Clock,
    eRCC_IO_I2C_RESET_Clock,
    eRCC_IO_Power_Clock,
    eRCC_IO_BKRam_Clock,
    eRCC_IO_BKRam_Clock_Reset,
    eRCC_IO_MCOX_Clock_config,
    eRCC_IO_CAN_Clock,
    eRCC_IO_ADC_Clock,
    eRCC_IO_DMA_Clock,
    eRCC_IO_DMA_Reset_Clock,
    eRCC_IO_TIM_Clock,

}eHalRccClock;

typedef enum _eHalRccClockUart
{
    eRCC_Clock_Uart1,
    eRCC_Clock_Uart2,
    eRCC_Clock_Uart3,
    eRCC_Clock_Uart4,
    eRCC_Clock_Uart5,
    eRCC_Clock_Uart6,
    eRCC_Clock_Uart7,
    eRCC_Clock_Uart8,
}eHalRccClockUart;

typedef enum _eHalRccClockSPI
{
    eRCC_Clock_SPI1,
    eRCC_Clock_SPI2,
    eRCC_Clock_SPI3,
    eRCC_Clock_SPI4,
    eRCC_Clock_SPI5,
    eRCC_Clock_SPI6,
}eHalRccClockSPI;

typedef enum _eHalRccClockI2C
{
    eRCC_Clock_I2C1,
    eRCC_Clock_I2C2,
    eRCC_Clock_I2C3,
}eHalRccClockI2C;

typedef enum _eHalRccClockCAN
{
    eRCC_Clock_CAN1,
    eRCC_Clock_CAN2,
}eHalRccClockCAN;

typedef enum _eHalRccClockADC
{
    eRCC_Clock_ADC1,
    eRCC_Clock_ADC2,
}eHalRccClockADC;

typedef enum _eHalRccClockDMA
{
    eRCC_Clock_DMA1,
    eRCC_Clock_DMA2,
    eRCC_Clock_DMA2D,

}eHalRccClockDMA;

typedef enum _eHalRccClockTIM
{
    eRCC_Clock_TIM1,
    eRCC_Clock_TIM2,
    eRCC_Clock_TIM3,
    eRCC_Clock_TIM4,
    eRCC_Clock_TIM5,
    eRCC_Clock_TIM6,
    eRCC_Clock_TIM7,
    eRCC_Clock_TIM8,
    eRCC_Clock_TIM9,
    eRCC_Clock_TIM10,
    eRCC_Clock_TIM11,
    eRCC_Clock_TIM12,
    eRCC_Clock_TIM13,
    eRCC_Clock_TIM14,
}eHalRccClockTIM;

typedef __packed struct __stHalRCC_ClocksTypeDef
{
  uint32_t SYSCLK_Frequency; /*!<  SYSCLK clock frequency expressed in Hz */
  uint32_t HCLK_Frequency;   /*!<  HCLK clock frequency expressed in Hz   */
  uint32_t PCLK1_Frequency;  /*!<  PCLK1 clock frequency expressed in Hz  */
  uint32_t PCLK2_Frequency;  /*!<  PCLK2 clock frequency expressed in Hz  */
}stHalRCC_ClocksTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/
void HalRccSystemClock_Init(void);
void HalRcc_printSystemCLKs( void );

///////////////////////////////////////////////////////////////////////////////////////
int HalDrvRccOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvRccRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvRccWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvRccIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvRccClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

#endif //__HAL_RCC_DRIVER_H__
