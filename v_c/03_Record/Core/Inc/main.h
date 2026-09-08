/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define USE_HW_SPI1_NSS    1  /* 1: Hardware NSS, 0: Software NSS */
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* PORT A defines ------------------------------------------------------------*/
#define WAK_UP_Pin					GPIO_PIN_0
#define WAK_UP_GPIO_Port			GPIOA
#define USB_HUB_RST_Pin				GPIO_PIN_1
#define USB_HUB_RST_GPIO_Port		GPIOA
#define USB_ETH_NRST_Pin			GPIO_PIN_2
#define USB_ETH_NRST_GPIO_Port		GPIOA
#define USB_HS_D0_Pin				GPIO_PIN_3
#define USB_HS_D0_GPIO_Port			GPIOA
#define TRIG_LED_EN_Pin				GPIO_PIN_4
#define TRIG_LED_EN_GPIO_Port		GPIOA
#define USB_HS_CLK_Pin				GPIO_PIN_5
#define USB_HS_CLK_GPIO_Port		GPIOA
#define KL_RXD1_Pin					GPIO_PIN_6
#define KL_RXD1_GPIO_Port			GPIOA
#define HL_CAN_SW_EN_Pin			GPIO_PIN_7
#define HL_CAN_SW_EN_GPIO_Port		GPIOA
#define EMMC_RST_Pin				GPIO_PIN_8
#define EMMC_RST_GPIO_Port			GPIOA
#define SPI2_INTR_Pin				GPIO_PIN_9
#define SPI2_INTR_GPIO_Port			GPIOA
#define DLCB_EN14_Pin				GPIO_PIN_10
#define DLCB_EN14_GPIO_Port			GPIOA
#define DLCB_EN15_Pin				GPIO_PIN_11
#define DLCB_EN15_GPIO_Port			GPIOA
#define DLCA_EN15_1_Pin				GPIO_PIN_12
#define DLCA_EN15_1_GPIO_Port		GPIOA
#define SWDIO_Pin					GPIO_PIN_13
#define SWDIO_GPIO_Port				GPIOA
#define SWCLK_Pin					GPIO_PIN_14
#define SWCLK_GPIO_Port				GPIOA

/* PORT B defines -------------------------------------------------------------*/
#define USB_HS_D1_Pin				GPIO_PIN_0
#define USB_HS_D1_GPIO_Port			GPIOB
#define USB_HS_D2_Pin				GPIO_PIN_1
#define USB_HS_D2_GPIO_Port			GPIOB
#define TXD2_2K_EN_Pin				GPIO_PIN_2
#define TXD2_2K_EN_GPIO_Port		GPIOB
#define DEBUG_TRACE_SWO_Pin			GPIO_PIN_3
#define DEBUG_TRACE_SWO_GPIO_Port	GPIOB
#define USB_HS_D7_Pin				GPIO_PIN_5
#define USB_HS_D7_GPIO_Port			GPIOB
#define EMMC_D4_Pin					GPIO_PIN_8
#define EMMC_D4_GPIO_Port			GPIOB
#define EMMC_D5_Pin					GPIO_PIN_9
#define EMMC_D5_GPIO_Port			GPIOB
#define USB_HS_D3_Pin				GPIO_PIN_10
#define USB_HS_D3_GPIO_Port			GPIOB
#define USB_HS_D4_Pin				GPIO_PIN_11
#define USB_HS_D4_GPIO_Port			GPIOB
#define USB_HS_D5_Pin				GPIO_PIN_12
#define USB_HS_D5_GPIO_Port			GPIOB
#define USB_HS_D6_Pin				GPIO_PIN_13
#define USB_HS_D6_GPIO_Port			GPIOB
#define KL_TXD1_Pin					GPIO_PIN_14
#define KL_TXD1_GPIO_Port			GPIOB
#define KL_RXD1B15_Pin				GPIO_PIN_15
#define KL_RXD1B15_GPIO_Port		GPIOB

/* PORT C defines -------------------------------------------------------------*/
#define USB_HS_STP_Pin				GPIO_PIN_0
#define USB_HS_STP_GPIO_Port		GPIOC
#define USB_HUB_PWR_EN_Pin			GPIO_PIN_1
#define USB_HUB_PWR_EN_GPIO_Port	GPIOC
#define H_STB_EN1_Pin				GPIO_PIN_4
#define H_STB_EN1_GPIO_Port			GPIOC
#define H_STB_EN2_Pin				GPIO_PIN_5
#define H_STB_EN2_GPIO_Port			GPIOC
#define EMMC_D6_Pin					GPIO_PIN_6
#define EMMC_D6_GPIO_Port			GPIOC
#define EMMC_D7_Pin					GPIO_PIN_7
#define EMMC_D7_GPIO_Port			GPIOC
#define EMMC_D0_Pin					GPIO_PIN_8
#define EMMC_D0_GPIO_Port			GPIOC
#define EMMC_D1_Pin					GPIO_PIN_9
#define EMMC_D1_GPIO_Port			GPIOC
#define EMMC_D2_Pin					GPIO_PIN_10
#define EMMC_D2_GPIO_Port			GPIOC
#define EMMC_D3_Pin					GPIO_PIN_11
#define EMMC_D3_GPIO_Port			GPIOC
#define EMMC_CLK_Pin				GPIO_PIN_12
#define EMMC_CLK_GPIO_Port			GPIOC
#define WIFI_HOST_WAKE_Pin			GPIO_PIN_13
#define WIFI_HOST_WAKE_GPIO_Port	GPIOC

/* PORT D defines -------------------------------------------------------------*/
#define CAN_RX1_Pin					GPIO_PIN_0
#define CAN_RX1_GPIO_Port			GPIOD
#define CAN_TX1_Pin					GPIO_PIN_1
#define CAN_TX1_GPIO_Port			GPIOD
#define EMMC_CMD_Pin				GPIO_PIN_2
#define EMMC_CMD_GPIO_Port			GPIOD
#define WIFI_BT_PWR_EN_Pin			GPIO_PIN_3
#define WIFI_BT_PWR_EN_GPIO_Port	GPIOD
#define IG_ON_EN_Pin				GPIO_PIN_4
#define IG_ON_EN_GPIO_Port			GPIOD
#define DEBUG_TX_Pin				GPIO_PIN_5
#define DEBUG_TX_GPIO_Port			GPIOD
#define DEBUG_RX_Pin				GPIO_PIN_6
#define DEBUG_RX_GPIO_Port			GPIOD
#define KL_TXD2_Pin					GPIO_PIN_8
#define KL_TXD2_GPIO_Port			GPIOD
#define KL_RXD2D9_Pin				GPIO_PIN_9
#define KL_RXD2D9_GPIO_Port			GPIOD
#define DLCA_EN2_Pin				GPIO_PIN_10
#define DLCA_EN2_GPIO_Port			GPIOD
#define DLCA_EN3_Pin				GPIO_PIN_11
#define DLCA_EN3_GPIO_Port			GPIOD
#define DLCA_EN6_Pin				GPIO_PIN_12
#define DLCA_EN6_GPIO_Port			GPIOD
#define BUZZER_PWM_Pin				GPIO_PIN_13
#define BUZZER_PWM_GPIO_Port		GPIOD
#define DLCA_EN7_Pin				GPIO_PIN_14
#define DLCA_EN7_GPIO_Port			GPIOD
#define DLCB_EN8_Pin				GPIO_PIN_15
#define DLCB_EN8_GPIO_Port			GPIOD

/* PORT E defines -------------------------------------------------------------*/
#define ETH_PWR_EN_Pin				GPIO_PIN_0
#define ETH_PWR_EN_GPIO_Port		GPIOE
#define REPG_ON_Pin					GPIO_PIN_1
#define REPG_ON_GPIO_Port			GPIOE
#define LOW_CAN_EN_Pin				GPIO_PIN_2
#define LOW_CAN_EN_GPIO_Port		GPIOE
#define KL_TXD2_INV_Pin				GPIO_PIN_3
#define KL_TXD2_INV_GPIO_Port		GPIOE
#define BOARD_ID1_Pin				GPIO_PIN_4
#define BOARD_ID1_GPIO_Port			GPIOE
#define POC_IN_Pin					GPIO_PIN_5
#define POC_IN_GPIO_Port			GPIOE
#define WIFI_WAK_EN_Pin				GPIO_PIN_6
#define WIFI_WAK_EN_GPIO_Port		GPIOE
#define TXD1_510_EN_Pin				GPIO_PIN_7
#define TXD1_510_EN_GPIO_Port		GPIOE
#define PWR_DET_EN_24V_Pin			GPIO_PIN_8
#define PWR_DET_EN_24V_GPIO_Port	GPIOE
#define PWR_DET_EN_12V_Pin			GPIO_PIN_9
#define PWR_DET_EN_12V_GPIO_Port	GPIOE
#define LAT_H_CAN_RX_EN2_Pin		GPIO_PIN_10
#define LAT_H_CAN_RX_EN2_GPIO_Port	GPIOE
#define LAT_H_CAN_RX_EN1_Pin		GPIO_PIN_11
#define LAT_H_CAN_RX_EN1_GPIO_Port	GPIOE
#define TRG_WAK_UP_EN_Pin			GPIO_PIN_12
#define TRG_WAK_UP_EN_GPIO_Port		GPIOE
#define LAT_IG_DET_EN_Pin			GPIO_PIN_13
#define LAT_IG_DET_EN_GPIO_Port		GPIOE
#define LAT_CLK_Pin					GPIO_PIN_14
#define LAT_CLK_GPIO_Port			GPIOE
#define LAT_SENS_EN_Pin				GPIO_PIN_15
#define LAT_SENS_EN_GPIO_Port		GPIOE
#define H_CAN2_SW_EN_Pin			GPIO_PIN_6
#define H_CAN2_SW_EN_GPIO_Port		GPIOE

/* PORT F defines -------------------------------------------------------------*/
#define I2C_SDA_Pin					GPIO_PIN_0
#define I2C_SDA_GPIO_Port			GPIOF
#define I2C_SCL_Pin					GPIO_PIN_1
#define I2C_SCL_GPIO_Port			GPIOF
#define FD_INT2_Pin					GPIO_PIN_2
#define FD_INT2_GPIO_Port			GPIOF
#define RPG_ADC_Pin					GPIO_PIN_3
#define RPG_ADC_GPIO_Port			GPIOF
#define TX_INT2_Pin					GPIO_PIN_4
#define TX_INT2_GPIO_Port			GPIOF
#define KL_RXD2_SEL_Pin				GPIO_PIN_5
#define KL_RXD2_SEL_GPIO_Port		GPIOF
#define SPI5_NSS_Pin				GPIO_PIN_6
#define SPI5_NSS_GPIO_Port			GPIOF
#define SPI5_SCK_Pin				GPIO_PIN_7
#define SPI5_SCK_GPIO_Port			GPIOF
#define SPI5_MISO_Pin				GPIO_PIN_8
#define SPI5_MISO_GPIO_Port			GPIOF
#define SPI5_MOSI_Pin				GPIO_PIN_9
#define SPI5_MOSI_GPIO_Port			GPIOF
#define RX_INT2_Pin					GPIO_PIN_10
#define RX_INT2_GPIO_Port			GPIOF
#define BAT_ADC_Pin					GPIO_PIN_11
#define BAT_ADC_GPIO_Port			GPIOF
#define PAIR_SW_Pin					GPIO_PIN_12
#define PAIR_SW_GPIO_Port			GPIOF
#define OSC_EN_Pin					GPIO_PIN_13
#define OSC_EN_GPIO_Port			GPIOF
#define TRIG_KEY_INT_Pin			GPIO_PIN_14
#define TRIG_KEY_INT_GPIO_Port		GPIOF
#define PGANG_Pin					GPIO_PIN_15
#define PGANG_GPIO_Port				GPIOF

/* PORT G defines -------------------------------------------------------------*/
#define EMMC_PWR_EN_Pin				GPIO_PIN_0
#define EMMC_PWR_EN_GPIO_Port		GPIOG
#define BOARD_ID2_Pin				GPIO_PIN_1
#define BOARD_ID2_GPIO_Port			GPIOG
#define DLCA_EN8_1_Pin				GPIO_PIN_2
#define DLCA_EN8_1_GPIO_Port		GPIOG
#define DLCB_EN9_Pin				GPIO_PIN_3
#define DLCB_EN9_GPIO_Port			GPIOG
#define DLCB_EN10_Pin				GPIO_PIN_4
#define DLCB_EN10_GPIO_Port			GPIOG
#define DLCB_EN11_Pin				GPIO_PIN_5
#define DLCB_EN11_GPIO_Port			GPIOG
#define DLCB_EN12_Pin				GPIO_PIN_6
#define DLCB_EN12_GPIO_Port			GPIOG
#define HSM_RST_Pin					GPIO_PIN_7
#define HSM_RST_GPIO_Port			GPIOG
#define HSM_PWR_EN_Pin				GPIO_PIN_8
#define HSM_PWR_EN_GPIO_Port		GPIOG
#define REPG_CH9_Pin				GPIO_PIN_10
#define REPG_CH9_GPIO_Port			GPIOG
#define REPG_CH13_Pin				GPIO_PIN_12
#define REPG_CH13_GPIO_Port			GPIOG
#define REPG_CH14_Pin				GPIO_PIN_13
#define REPG_CH14_GPIO_Port			GPIOG
#define REPG_CH3_Pin				GPIO_PIN_15
#define REPG_CH3_GPIO_Port			GPIOG

/* PORT H defines -------------------------------------------------------------*/
#define OSC_IN_Pin					GPIO_PIN_0
#define OSC_IN_GPIO_Port			GPIOH
#define OSC_OUT_Pin					GPIO_PIN_1
#define OSC_OUT_GPIO_Port			GPIOH
#define USB_PHY_PWR_EN_Pin			GPIO_PIN_2
#define USB_PHY_PWR_EN_GPIO_Port	GPIOH
#define USB_PHY_RST_Pin				GPIO_PIN_3
#define USB_PHY_RST_GPIO_Port		GPIOH
#define TXD1_47K_EN_Pin				GPIO_PIN_5
#define TXD1_47K_EN_GPIO_Port		GPIOH
#define LAT_L_CAN_RX_EN_Pin			GPIO_PIN_6
#define LAT_L_CAN_RX_EN_GPIO_Port	GPIOH
#define LOW_CAN_NSTB_Pin			GPIO_PIN_7
#define LOW_CAN_NSTB_GPIO_Port		GPIOH
#define TXD2_47K_EN_Pin				GPIO_PIN_8
#define TXD2_47K_EN_GPIO_Port		GPIOH
#define TXD1_2K_EN_Pin				GPIO_PIN_9
#define TXD1_2K_EN_GPIO_Port		GPIOH
#define TXD2_510_EN_Pin				GPIO_PIN_10
#define TXD2_510_EN_GPIO_Port		GPIOH
#define KL_RXD2_Pin					GPIO_PIN_11
#define KL_RXD2_GPIO_Port			GPIOH
#define DLCA_EN13_Pin				GPIO_PIN_12
#define DLCA_EN13_GPIO_Port			GPIOH
#define DLCA_EN1_Pin				GPIO_PIN_13
#define DLCA_EN1_GPIO_Port			GPIOH

/* PORT I defines -------------------------------------------------------------*/
#define SPI2_NSS_Pin				GPIO_PIN_0
#define SPI2_NSS_GPIO_Port			GPIOI
#define SPI2_SCK_Pin				GPIO_PIN_1
#define SPI2_SCK_GPIO_Port			GPIOI
#define SPI2_MISO_Pin				GPIO_PIN_2
#define SPI2_MISO_GPIO_Port			GPIOI
#define SPI2_MOSI_Pin				GPIO_PIN_3
#define SPI2_MOSI_GPIO_Port			GPIOI
#define ETH_SW_EN_Pin				GPIO_PIN_4
#define ETH_SW_EN_GPIO_Port			GPIOI
#define RED_LED_EN_Pin				GPIO_PIN_5
#define RED_LED_EN_GPIO_Port		GPIOI
#define GREEN_LED_EN_Pin			GPIO_PIN_6
#define GREEN_LED_EN_GPIO_Port		GPIOI
#define BLUE_LED_EN_Pin				GPIO_PIN_7
#define BLUE_LED_EN_GPIO_Port		GPIOI
#define KL_TXD1_INV_Pin				GPIO_PIN_8
#define KL_TXD1_INV_GPIO_Port		GPIOI
#define KL_RXD1_INV_Pin				GPIO_PIN_9
#define KL_RXD1_INV_GPIO_Port		GPIOI
#define KL_RXD2_INV_Pin				GPIO_PIN_10
#define KL_RXD2_INV_GPIO_Port		GPIOI

/**********Changed pins**********/
extern GPIO_TypeDef* SPI1_NSS_GPIO_Port;
extern uint16_t      SPI1_NSS_Pin;
extern GPIO_TypeDef* SPI1_MISO_GPIO_Port;
extern uint16_t      SPI1_MISO_Pin;
extern GPIO_TypeDef* REPG_CH12_GPIO_Port;
extern uint16_t      REPG_CH12_Pin;
extern GPIO_TypeDef* REPG_CH11_GPIO_Port;
extern uint16_t      REPG_CH11_Pin;
extern GPIO_TypeDef* SPI1_MOSI_GPIO_Port;
extern uint16_t      SPI1_MOSI_Pin;
extern GPIO_TypeDef* REPG_CH6_GPIO_Port;
extern uint16_t      REPG_CH6_Pin;
extern GPIO_TypeDef* SPI1_SCK_GPIO_Port;
extern uint16_t      SPI1_SCK_Pin;
extern GPIO_TypeDef* WIFI_REST_GPIO_Port;
extern uint16_t      WIFI_REST_Pin;
extern GPIO_TypeDef* I2CLK_GPIO_Port;
extern uint16_t      I2CLK_Pin;
extern GPIO_TypeDef* I2DAT_GPIO_Port;
extern uint16_t      I2DAT_Pin;
extern GPIO_TypeDef* HSM_RX_GPIO_Port;
extern uint16_t      HSM_RX_Pin;
extern GPIO_TypeDef* HSM_TX_GPIO_Port;
extern uint16_t      HSM_TX_Pin;
extern GPIO_TypeDef* ULP_WAKEUP_IN_GPIO_Port;
extern uint16_t      ULP_WAKEUP_IN_Pin;
extern GPIO_TypeDef* LP_WAKEUP_IN_GPIO_Port;
extern uint16_t      LP_WAKEUP_IN_Pin;
extern GPIO_TypeDef* HIGHCAN_EN614_GPIO_Port;
extern uint16_t      HIGHCAN_EN614_Pin;
extern GPIO_TypeDef* KL_RXD1_SEL_GPIO_Port;
extern uint16_t      KL_RXD1_SEL_Pin;

#ifdef NEWBOARD
	#define USB_HS_DIR_Pin			GPIO_PIN_11
	#define USB_HS_DIR_GPIO_Port	GPIOI
	#define USB_HS_NXT_Pin			GPIO_PIN_4
	#define USB_HS_NXT_GPIO_Port	GPIOH
	#define KL_RXD1_SEL_Pin			GPIO_PIN_15
	#define KL_RXD1_SEL_GPIO_Port	GPIOH
	#define HIGHCAN_EN614_Pin		GPIO_PIN_14
	#define HIGHCAN_EN614_GPIO_Port GPIOH
	#define LP_WAKEUP_IN_Pin		GPIO_PIN_2
	#define LP_WAKEUP_IN_GPIO_Port	GPIOC
	#define ULP_WAKEUP_IN_Pin		GPIO_PIN_3
	#define ULP_WAKEUP_IN_GPIO_Port GPIOC
#else
	#define USB_HS_DIR_Pin			GPIO_PIN_2
	#define USB_HS_DIR_GPIO_Port	GPIOC
	#define USB_HS_NXT_Pin			GPIO_PIN_3
	#define USB_HS_NXT_GPIO_Port	GPIOC
	#define KL_RXD1_SEL_Pin			GPIO_PIN_11
	#define KL_RXD1_SEL_GPIO_Port	GPIOI
	#define HIGHCAN_EN614_Pin		GPIO_PIN_4
	#define HIGHCAN_EN614_GPIO_Port	GPIOH
	#define LP_WAKEUP_IN_Pin		GPIO_PIN_14
	#define LP_WAKEUP_IN_GPIO_Port	GPIOH
	#define ULP_WAKEUP_IN_Pin		GPIO_PIN_15
	#define ULP_WAKEUP_IN_GPIO_Port	GPIOH
#endif
/********************************/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
