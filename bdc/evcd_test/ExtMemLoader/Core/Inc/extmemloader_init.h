/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    extmemloader_init.h
  * @author  MCD Application Team
  * @brief   Header file of Loader_Src.c
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef EXTMEMLOADER_INIT_H
#define EXTMEMLOADER_INIT_H

/* Includes ------------------------------------------------------------------*/
#include "stm32h7rsxx_hal.h"

/* Exported types ------------------------------------------------------------*/

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/

/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

uint32_t extmemloader_Init(void);
void Error_Handler(void);

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#define AC_L_RLY_EN_Pin GPIO_PIN_3
#define AC_L_RLY_EN_GPIO_Port GPIOE
#define AC_N_RLY_EN_Pin GPIO_PIN_4
#define AC_N_RLY_EN_GPIO_Port GPIOE
#define DC_P_RLY_EN_Pin GPIO_PIN_5
#define DC_P_RLY_EN_GPIO_Port GPIOE
#define DC_M_RLY_EN_Pin GPIO_PIN_6
#define DC_M_RLY_EN_GPIO_Port GPIOE
#define EXT_RLY1_EN_Pin GPIO_PIN_7
#define EXT_RLY1_EN_GPIO_Port GPIOF
#define EXT_RLY2_EN_Pin GPIO_PIN_8
#define EXT_RLY2_EN_GPIO_Port GPIOF
#define EXT_RLY3_EN_Pin GPIO_PIN_9
#define EXT_RLY3_EN_GPIO_Port GPIOF
#define EXT_RLY4_EN_Pin GPIO_PIN_10
#define EXT_RLY4_EN_GPIO_Port GPIOF
#define CP_ADC1_INP10_Pin GPIO_PIN_0
#define CP_ADC1_INP10_GPIO_Port GPIOC
#define BLE_USART2_CTS_Pin GPIO_PIN_0
#define BLE_USART2_CTS_GPIO_Port GPIOA
#define BLE_USART2_RTS_Pin GPIO_PIN_1
#define BLE_USART2_RTS_GPIO_Port GPIOA
#define BLE_USART2_TX_Pin GPIO_PIN_2
#define BLE_USART2_TX_GPIO_Port GPIOA
#define BLE_USART2_RX_Pin GPIO_PIN_3
#define BLE_USART2_RX_GPIO_Port GPIOA
#define BLE_RSTB_Pin GPIO_PIN_4
#define BLE_RSTB_GPIO_Port GPIOA
#define BLE_STATUS_Pin GPIO_PIN_5
#define BLE_STATUS_GPIO_Port GPIOA
#define ER_R_Pin GPIO_PIN_7
#define ER_R_GPIO_Port GPIOA
#define ER_A_Pin GPIO_PIN_4
#define ER_A_GPIO_Port GPIOC
#define ER_G_Pin GPIO_PIN_5
#define ER_G_GPIO_Port GPIOC
#define ER_BUZ_TEMP_Pin GPIO_PIN_0
#define ER_BUZ_TEMP_GPIO_Port GPIOB
#define ER_ON_Pin GPIO_PIN_1
#define ER_ON_GPIO_Port GPIOB
#define ER_BL_Pin GPIO_PIN_2
#define ER_BL_GPIO_Port GPIOB
#define XSPIM_P1_NRESET_Pin GPIO_PIN_5
#define XSPIM_P1_NRESET_GPIO_Port GPIOO
#define EMMC_PWR_EN_Pin GPIO_PIN_8
#define EMMC_PWR_EN_GPIO_Port GPIOD
#define ADC_CH_CONT1_Pin GPIO_PIN_12
#define ADC_CH_CONT1_GPIO_Port GPIOD
#define ADC_CH_CONT2_Pin GPIO_PIN_13
#define ADC_CH_CONT2_GPIO_Port GPIOD
#define ADC_I2C2_SCL_Pin GPIO_PIN_10
#define ADC_I2C2_SCL_GPIO_Port GPIOB
#define ADC_I2C2_SDA_Pin GPIO_PIN_11
#define ADC_I2C2_SDA_GPIO_Port GPIOB
#define PLC_FDCAN2_RX_Pin GPIO_PIN_12
#define PLC_FDCAN2_RX_GPIO_Port GPIOB
#define PLC_FDCAN2_TX_Pin GPIO_PIN_13
#define PLC_FDCAN2_TX_GPIO_Port GPIOB
#define CAN2_SW_EN_Pin GPIO_PIN_14
#define CAN2_SW_EN_GPIO_Port GPIOB
#define AIM_DIR_Pin GPIO_PIN_14
#define AIM_DIR_GPIO_Port GPIOD
#define CP_PWM_TIM4_CH4_Pin GPIO_PIN_15
#define CP_PWM_TIM4_CH4_GPIO_Port GPIOD
#define PLC_RST_Pin GPIO_PIN_8
#define PLC_RST_GPIO_Port GPIOA
#define PLC_USART1_TX_Pin GPIO_PIN_9
#define PLC_USART1_TX_GPIO_Port GPIOA
#define PLC_USART1_RX_Pin GPIO_PIN_10
#define PLC_USART1_RX_GPIO_Port GPIOA
#define VCAN_FDCAN1_RX_Pin GPIO_PIN_11
#define VCAN_FDCAN1_RX_GPIO_Port GPIOA
#define VCAN_FDCAN1_TX_Pin GPIO_PIN_12
#define VCAN_FDCAN1_TX_GPIO_Port GPIOA
#define CAN1_SW_EN_Pin GPIO_PIN_15
#define CAN1_SW_EN_GPIO_Port GPIOA
#define AIM_RX_Pin GPIO_PIN_0
#define AIM_RX_GPIO_Port GPIOD
#define AIM_TX_Pin GPIO_PIN_1
#define AIM_TX_GPIO_Port GPIOD
#define CAN1_TERM_EN_Pin GPIO_PIN_11
#define CAN1_TERM_EN_GPIO_Port GPIOE
#define CAN2_TERM_EN_Pin GPIO_PIN_12
#define CAN2_TERM_EN_GPIO_Port GPIOE
#define CAN_STB_EN1_Pin GPIO_PIN_13
#define CAN_STB_EN1_GPIO_Port GPIOE
#define CAN_STB_EN2_Pin GPIO_PIN_14
#define CAN_STB_EN2_GPIO_Port GPIOE
#define MCU_ALIVE_LED_Pin GPIO_PIN_3
#define MCU_ALIVE_LED_GPIO_Port GPIOD
#define EMMC_RST_N_Pin GPIO_PIN_2
#define EMMC_RST_N_GPIO_Port GPIOM
#define SS1_EN_Pin GPIO_PIN_5
#define SS1_EN_GPIO_Port GPIOD
#define SS2_EN_Pin GPIO_PIN_6
#define SS2_EN_GPIO_Port GPIOD
#define SS3_EN_Pin GPIO_PIN_7
#define SS3_EN_GPIO_Port GPIOD
#define SS4_EN_Pin GPIO_PIN_0
#define SS4_EN_GPIO_Port GPIOF
#define SS5_EN_Pin GPIO_PIN_1
#define SS5_EN_GPIO_Port GPIOF
#define SS6_EN_Pin GPIO_PIN_2
#define SS6_EN_GPIO_Port GPIOF
#define SS7_EN_Pin GPIO_PIN_3
#define SS7_EN_GPIO_Port GPIOF
#define SS8_EN_Pin GPIO_PIN_4
#define SS8_EN_GPIO_Port GPIOF
#define TS_ADC_CH_CONT1_Pin GPIO_PIN_4
#define TS_ADC_CH_CONT1_GPIO_Port GPIOB
#define TS_ADC_CH_CONT2_Pin GPIO_PIN_5
#define TS_ADC_CH_CONT2_GPIO_Port GPIOB
#define LCD_UART8_RX_Pin GPIO_PIN_0
#define LCD_UART8_RX_GPIO_Port GPIOE
#define LCD_UART8_TX_Pin GPIO_PIN_1
#define LCD_UART8_TX_GPIO_Port GPIOE
#define TS_ADC_I2C1_SCL_Pin GPIO_PIN_6
#define TS_ADC_I2C1_SCL_GPIO_Port GPIOB
#define TS_ADC_I2C1_SDA_Pin GPIO_PIN_7
#define TS_ADC_I2C1_SDA_GPIO_Port GPIOB
#endif /* EXTMEMLOADER_INIT_H */
