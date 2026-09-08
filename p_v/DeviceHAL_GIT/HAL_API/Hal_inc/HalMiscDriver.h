#ifndef __HAL_MISC_DRIVER_H__
#define __HAL_MISC_DRIVER_H__

#include "HalHandler.h"

#include "common.h"
/* Exported define -----------------------------------------------------------*/
// new priority value
#define HAL_PRIORITY_0_NVIC_IRQChannelPreemptionPriority 	(0x00)
#define HAL_PRIORITY_1_NVIC_IRQChannelPreemptionPriority 	(0x01)
#define HAL_PRIORITY_2_NVIC_IRQChannelPreemptionPriority 	(0x02)
#define HAL_PRIORITY_3_NVIC_IRQChannelPreemptionPriority 	(0x03)
#define HAL_PRIORITY_4_NVIC_IRQChannelPreemptionPriority 	(0x04)
#define HAL_PRIORITY_5_NVIC_IRQChannelPreemptionPriority 	(0x05)
#define HAL_PRIORITY_6_NVIC_IRQChannelPreemptionPriority 	(0x06)
#define HAL_PRIORITY_7_NVIC_IRQChannelPreemptionPriority 	(0x07)
#define HAL_PRIORITY_8_NVIC_IRQChannelPreemptionPriority 	(0x08)
#define HAL_PRIORITY_9_NVIC_IRQChannelPreemptionPriority 	(0x09)
#define HAL_PRIORITY_10_NVIC_IRQChannelPreemptionPriority 	(0x0A)
#define HAL_PRIORITY_11_NVIC_IRQChannelPreemptionPriority 	(0x0B)
#define HAL_PRIORITY_12_NVIC_IRQChannelPreemptionPriority 	(0x0C)
#define HAL_PRIORITY_13_NVIC_IRQChannelPreemptionPriority 	(0x0D)
#define HAL_PRIORITY_14_NVIC_IRQChannelPreemptionPriority 	(0x0E)
#define HAL_PRIORITY_15_NVIC_IRQChannelPreemptionPriority 	(0x0F)


#define HAL_CAN1_NVIC_IRQChannelPreemptionPriority 			HAL_PRIORITY_0_NVIC_IRQChannelPreemptionPriority
#define HAL_CAN1_TX_NVIC_IRQChannelSubPriority				0x00
#define HAL_CAN1_RX0_NVIC_IRQChannelSubPriority				0x01
#define HAL_CAN1_RX1_NVIC_IRQChannelSubPriority				0x02
#define HAL_CAN1_SCE_NVIC_IRQChannelSubPriority				0x03

#define HAL_CAN2_NVIC_IRQChannelPreemptionPriority 			HAL_PRIORITY_1_NVIC_IRQChannelPreemptionPriority
#define HAL_CAN2_TX_NVIC_IRQChannelSubPriority				0x00
#define HAL_CAN2_RX0_NVIC_IRQChannelSubPriority				0x01
#define HAL_CAN2_RX1_NVIC_IRQChannelSubPriority				0x02
#define HAL_CAN2_SCE_NVIC_IRQChannelSubPriority				0x03

#define HAL_UART_NVIC_IRQChannelPreemptionPriority 	        HAL_PRIORITY_2_NVIC_IRQChannelPreemptionPriority
#define HAL_UART2_MODEM_NVIC_IRQChannelSubPriority			0x00
#define HAL_UART3_BLE_NVIC_IRQChannelSubPriority			0x01
#define HAL_UART4_GPS_NVIC_IRQChannelSubPriority			0x02
#define HAL_UART8_SELFTEST_IRQChannelSubPriority       		0x03

#define HAL_Group_0_NVIC_IRQChannelPreemptionPriority 		HAL_PRIORITY_3_NVIC_IRQChannelPreemptionPriority
#define HAL_RCC_NVIC_IRQChannelSubPriority					0x00
#define HAL_UART_DEBUG_IRQChannelSubPriority				0x01
#define HAL_STOPMODE_WAKEUP_IRQChannelSubPriority			0x02    // HAL_EXTI9_5_NVIC_IRQChannelSubPriority
#define HAL_RTC_NVIC_IRQChannelSubPriority                  0x03



/* Exported types - Structure, Enumeration -----------------------------------*/
typedef enum __eHalMiscIRQn_Type
{
/******  Cortex-M4 Processor Exceptions Numbers ****************************************************************/
  HAL_NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                                          */
  HAL_MemoryManagement_IRQn       = -12,    /*!< 4 Cortex-M4 Memory Management Interrupt                           */
  HAL_BusFault_IRQn               = -11,    /*!< 5 Cortex-M4 Bus Fault Interrupt                                   */
  HAL_UsageFault_IRQn             = -10,    /*!< 6 Cortex-M4 Usage Fault Interrupt                                 */
  HAL_SVCall_IRQn                 = -5,     /*!< 11 Cortex-M4 SV Call Interrupt                                    */
  HAL_DebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M4 Debug Monitor Interrupt                              */
  HAL_PendSV_IRQn                 = -2,     /*!< 14 Cortex-M4 Pend SV Interrupt                                    */
  HAL_SysTick_IRQn                = -1,     /*!< 15 Cortex-M4 System Tick Interrupt                                */
/******  STM32 specific Interrupt Numbers **********************************************************************/
  HAL_WWDG_IRQn                   = 0,      /*!< Window WatchDog Interrupt                                         */
  HAL_PVD_IRQn                    = 1,      /*!< PVD through EXTI Line detection Interrupt                         */
  HAL_TAMP_STAMP_IRQn             = 2,      /*!< Tamper and TimeStamp interrupts through the EXTI line             */
  HAL_RTC_WKUP_IRQn               = 3,      /*!< RTC Wakeup interrupt through the EXTI line                        */
  HAL_FLASH_IRQn                  = 4,      /*!< FLASH global Interrupt                                            */
  HAL_RCC_IRQn                    = 5,      /*!< RCC global Interrupt                                              */
  HAL_EXTI0_IRQn                  = 6,      /*!< EXTI Line0 Interrupt                                              */
  HAL_EXTI1_IRQn                  = 7,      /*!< EXTI Line1 Interrupt                                              */
  HAL_EXTI2_IRQn                  = 8,      /*!< EXTI Line2 Interrupt                                              */
  HAL_EXTI3_IRQn                  = 9,      /*!< EXTI Line3 Interrupt                                              */
  HAL_EXTI4_IRQn                  = 10,     /*!< EXTI Line4 Interrupt                                              */
  HAL_DMA1_Stream0_IRQn           = 11,     /*!< DMA1 Stream 0 global Interrupt                                    */
  HAL_DMA1_Stream1_IRQn           = 12,     /*!< DMA1 Stream 1 global Interrupt                                    */
  HAL_DMA1_Stream2_IRQn           = 13,     /*!< DMA1 Stream 2 global Interrupt                                    */
  HAL_DMA1_Stream3_IRQn           = 14,     /*!< DMA1 Stream 3 global Interrupt                                    */
  HAL_DMA1_Stream4_IRQn           = 15,     /*!< DMA1 Stream 4 global Interrupt                                    */
  HAL_DMA1_Stream5_IRQn           = 16,     /*!< DMA1 Stream 5 global Interrupt                                    */
  HAL_DMA1_Stream6_IRQn           = 17,     /*!< DMA1 Stream 6 global Interrupt                                    */
  HAL_ADC_IRQn                    = 18,     /*!< ADC1, ADC2 and ADC3 global Interrupts                             */
  HAL_CAN1_TX_IRQn                = 19,     /*!< CAN1 TX Interrupt                                                 */
  HAL_CAN1_RX0_IRQn               = 20,     /*!< CAN1 RX0 Interrupt                                                */
  HAL_CAN1_RX1_IRQn               = 21,     /*!< CAN1 RX1 Interrupt                                                */
  HAL_CAN1_SCE_IRQn               = 22,     /*!< CAN1 SCE Interrupt                                                */
  HAL_EXTI9_5_IRQn                = 23,     /*!< External Line[9:5] Interrupts                                     */
  HAL_TIM1_BRK_TIM9_IRQn          = 24,     /*!< TIM1 Break interrupt and TIM9 global interrupt                    */
  HAL_TIM1_UP_TIM10_IRQn          = 25,     /*!< TIM1 Update Interrupt and TIM10 global interrupt                  */
  HAL_TIM1_TRG_COM_TIM11_IRQn     = 26,     /*!< TIM1 Trigger and Commutation Interrupt and TIM11 global interrupt */
  HAL_TIM1_CC_IRQn                = 27,     /*!< TIM1 Capture Compare Interrupt                                    */
  HAL_TIM2_IRQn                   = 28,     /*!< TIM2 global Interrupt                                             */
  HAL_TIM3_IRQn                   = 29,     /*!< TIM3 global Interrupt                                             */
  HAL_TIM4_IRQn                   = 30,     /*!< TIM4 global Interrupt                                             */
  HAL_I2C1_EV_IRQn                = 31,     /*!< I2C1 Event Interrupt                                              */
  HAL_I2C1_ER_IRQn                = 32,     /*!< I2C1 Error Interrupt                                              */
  HAL_I2C2_EV_IRQn                = 33,     /*!< I2C2 Event Interrupt                                              */
  HAL_I2C2_ER_IRQn                = 34,     /*!< I2C2 Error Interrupt                                              */  
  HAL_SPI1_IRQn                   = 35,     /*!< SPI1 global Interrupt                                             */
  HAL_SPI2_IRQn                   = 36,     /*!< SPI2 global Interrupt                                             */
  HAL_USART1_IRQn                 = 37,     /*!< USART1 global Interrupt                                           */
  HAL_USART2_IRQn                 = 38,     /*!< USART2 global Interrupt                                           */
  HAL_USART3_IRQn                 = 39,     /*!< USART3 global Interrupt                                           */
  HAL_EXTI15_10_IRQn              = 40,     /*!< External Line[15:10] Interrupts                                   */
  HAL_RTC_Alarm_IRQn              = 41,     /*!< RTC Alarm (A and B) through EXTI Line Interrupt                   */
  HAL_OTG_FS_WKUP_IRQn            = 42,     /*!< USB OTG FS Wakeup through EXTI line interrupt                     */    
  HAL_TIM8_BRK_TIM12_IRQn         = 43,     /*!< TIM8 Break Interrupt and TIM12 global interrupt                   */
  HAL_TIM8_UP_TIM13_IRQn          = 44,     /*!< TIM8 Update Interrupt and TIM13 global interrupt                  */
  HAL_TIM8_TRG_COM_TIM14_IRQn     = 45,     /*!< TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt */
  HAL_TIM8_CC_IRQn                = 46,     /*!< TIM8 Capture Compare Interrupt                                    */
  HAL_DMA1_Stream7_IRQn           = 47,     /*!< DMA1 Stream7 Interrupt                                            */
  HAL_FMC_IRQn                    = 48,     /*!< FMC global Interrupt                                              */
  HAL_SDIO_IRQn                   = 49,     /*!< SDIO global Interrupt                                             */
  HAL_TIM5_IRQn                   = 50,     /*!< TIM5 global Interrupt                                             */
  HAL_SPI3_IRQn                   = 51,     /*!< SPI3 global Interrupt                                             */
  HAL_UART4_IRQn                  = 52,     /*!< UART4 global Interrupt                                            */
  HAL_UART5_IRQn                  = 53,     /*!< UART5 global Interrupt                                            */
  HAL_TIM6_DAC_IRQn               = 54,     /*!< TIM6 global and DAC1&2 underrun error  interrupts                 */
  HAL_TIM7_IRQn                   = 55,     /*!< TIM7 global interrupt                                             */
  HAL_DMA2_Stream0_IRQn           = 56,     /*!< DMA2 Stream 0 global Interrupt                                    */
  HAL_DMA2_Stream1_IRQn           = 57,     /*!< DMA2 Stream 1 global Interrupt                                    */
  HAL_DMA2_Stream2_IRQn           = 58,     /*!< DMA2 Stream 2 global Interrupt                                    */
  HAL_DMA2_Stream3_IRQn           = 59,     /*!< DMA2 Stream 3 global Interrupt                                    */
  HAL_DMA2_Stream4_IRQn           = 60,     /*!< DMA2 Stream 4 global Interrupt                                    */
  HAL_ETH_IRQn                    = 61,     /*!< Ethernet global Interrupt                                         */
  HAL_ETH_WKUP_IRQn               = 62,     /*!< Ethernet Wakeup through EXTI line Interrupt                       */
  HAL_CAN2_TX_IRQn                = 63,     /*!< CAN2 TX Interrupt                                                 */
  HAL_CAN2_RX0_IRQn               = 64,     /*!< CAN2 RX0 Interrupt                                                */
  HAL_CAN2_RX1_IRQn               = 65,     /*!< CAN2 RX1 Interrupt                                                */
  HAL_CAN2_SCE_IRQn               = 66,     /*!< CAN2 SCE Interrupt                                                */
  HAL_OTG_FS_IRQn                 = 67,     /*!< USB OTG FS global Interrupt                                       */
  HAL_DMA2_Stream5_IRQn           = 68,     /*!< DMA2 Stream 5 global interrupt                                    */
  HAL_DMA2_Stream6_IRQn           = 69,     /*!< DMA2 Stream 6 global interrupt                                    */
  HAL_DMA2_Stream7_IRQn           = 70,     /*!< DMA2 Stream 7 global interrupt                                    */
  HAL_USART6_IRQn                 = 71,     /*!< USART6 global interrupt                                           */
  HAL_I2C3_EV_IRQn                = 72,     /*!< I2C3 event interrupt                                              */
  HAL_I2C3_ER_IRQn                = 73,     /*!< I2C3 error interrupt                                              */
  HAL_OTG_HS_EP1_OUT_IRQn         = 74,     /*!< USB OTG HS End Point 1 Out global interrupt                       */
  HAL_OTG_HS_EP1_IN_IRQn          = 75,     /*!< USB OTG HS End Point 1 In global interrupt                        */
  HAL_OTG_HS_WKUP_IRQn            = 76,     /*!< USB OTG HS Wakeup through EXTI interrupt                          */
  HAL_OTG_HS_IRQn                 = 77,     /*!< USB OTG HS global interrupt                                       */
  HAL_DCMI_IRQn                   = 78,     /*!< DCMI global interrupt                                             */
  HAL_CRYP_IRQn                   = 79,     /*!< CRYP crypto global interrupt                                      */
  HAL_HASH_RNG_IRQn               = 80,     /*!< Hash and Rng global interrupt                                     */
  HAL_FPU_IRQn                    = 81,     /*!< FPU global interrupt                                              */
  HAL_UART7_IRQn                  = 82,     /*!< UART7 global interrupt                                            */
  HAL_UART8_IRQn                  = 83,     /*!< UART8 global interrupt                                            */
  HAL_SPI4_IRQn                   = 84,     /*!< SPI4 global Interrupt                                             */
  HAL_SPI5_IRQn                   = 85,     /*!< SPI5 global Interrupt                                             */
  HAL_SPI6_IRQn                   = 86,     /*!< SPI6 global Interrupt                                             */
  HAL_SAI1_IRQn                   = 87,     /*!< SAI1 global Interrupt                                             */
  HAL_DMA2D_IRQn                  = 90      /*!< DMA2D global Interrupt                                            */   
 } eHalMiscIRQn_Type;


typedef enum __eHalMiscIoCtlMode{
    eMISC_IO_RegIRQ,
    eMISC_IO_DeRegIRQ,	
}eHalMiscIoCtlMode;


typedef __packed struct __stHalNVIC_InitTypeDef
{
  uint8_t NVIC_IRQChannel;                    /*!< Specifies the IRQ channel to be enabled or disabled.
                                                   This parameter can be an enumerator of @ref IRQn_Type 
                                                   enumeration (For the complete STM32 Devices IRQ Channels
                                                   list, please refer to stm32f4xx.h file) */

  uint8_t NVIC_IRQChannelPreemptionPriority;  /*!< Specifies the pre-emption priority for the IRQ channel
                                                   specified in NVIC_IRQChannel. This parameter can be a value
                                                   between 0 and 15 as described in the table @ref MISC_NVIC_Priority_Table
                                                   A lower priority value indicates a higher priority */

  uint8_t NVIC_IRQChannelSubPriority;         /*!< Specifies the subpriority level for the IRQ channel specified
                                                   in NVIC_IRQChannel. This parameter can be a value
                                                   between 0 and 15 as described in the table @ref MISC_NVIC_Priority_Table
                                                   A lower priority value indicates a higher priority */

  BOOL NVIC_IRQChannelEnable;         /*!< Specifies whether the IRQ channel defined in NVIC_IRQChannel
                                                   will be enabled or disabled. 
                                                   This parameter can be set either to ENABLE or DISABLE */   
} stHalNVIC_InitTypeDef;
 

/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

int HalDrvMiscOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvMiscRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvMiscWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvMiscIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvMiscClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

#endif //__HAL_MISC_DRIVER_H__
