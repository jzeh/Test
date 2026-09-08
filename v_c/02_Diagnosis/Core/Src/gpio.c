/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
void HSM_Type_Detect(void);

/* HSM Type Global Variable */
volatile HSM_Type_t g_HSM_Type = HSM_TYPE_OLD;	// Default: Old HSM

GPIO_TypeDef*	SPI1_NSS_GPIO_Port;
uint16_t		SPI1_NSS_Pin;
GPIO_TypeDef*	SPI1_MISO_GPIO_Port;
uint16_t		SPI1_MISO_Pin;
GPIO_TypeDef*	REPG_CH12_GPIO_Port;
uint16_t		REPG_CH12_Pin;
GPIO_TypeDef*	REPG_CH11_GPIO_Port;
uint16_t		REPG_CH11_Pin;
GPIO_TypeDef*	SPI1_MOSI_GPIO_Port;
uint16_t		SPI1_MOSI_Pin;
GPIO_TypeDef*	REPG_CH6_GPIO_Port;
uint16_t		REPG_CH6_Pin;
GPIO_TypeDef*	SPI1_SCK_GPIO_Port;
uint16_t		SPI1_SCK_Pin;
GPIO_TypeDef*	WIFI_REST_GPIO_Port;
uint16_t		WIFI_REST_Pin;
GPIO_TypeDef*	I2CLK_GPIO_Port;
uint16_t		I2CLK_Pin;
GPIO_TypeDef*	I2DAT_GPIO_Port;
uint16_t		I2DAT_Pin;
GPIO_TypeDef*	HSM_RX_GPIO_Port;
uint16_t		HSM_RX_Pin;
GPIO_TypeDef*	HSM_TX_GPIO_Port;
uint16_t		HSM_TX_Pin;

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins
     PC14-OSC32_IN (OSC32_IN)	------> RCC_OSC32_IN
     PC15-OSC32_OUT (OSC32_OUT)	------> RCC_OSC32_OUT
     PH0-OSC_IN (PH0)			------> RCC_OSC_IN
     PH1-OSC_OUT (PH1)			------> RCC_OSC_OUT
     PA0						------> PWR_WKUP0
     PA13 (JTMS/SWDIO)			------> DEBUG_JTMS-SWDIO
     PA14 (JTCK/SWCLK)			------> DEBUG_JTCK-SWCLK
     PB3 (JTDO/TRACESWO)		------> DEBUG_JTDO-SWO
*/
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  GPIO_InitStruct.Pin   = BOARD_ID1_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(BOARD_ID1_GPIO_Port, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin   = BOARD_ID2_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(BOARD_ID2_GPIO_Port, &GPIO_InitStruct);

  HSM_Type_Detect();
  
  if (g_HSM_Type == HSM_TYPE_NEW)
  {
      SPI1_NSS_GPIO_Port      = GPIOA;    SPI1_NSS_Pin        = GPIO_PIN_15;
      SPI1_MISO_GPIO_Port     = GPIOB;    SPI1_MISO_Pin       = GPIO_PIN_4;
      REPG_CH12_GPIO_Port     = GPIOB;    REPG_CH12_Pin       = GPIO_PIN_6;
      REPG_CH11_GPIO_Port     = GPIOB;    REPG_CH11_Pin       = GPIO_PIN_7;
      SPI1_MOSI_GPIO_Port     = GPIOD;    SPI1_MOSI_Pin       = GPIO_PIN_7;
      REPG_CH6_GPIO_Port      = GPIOG;    REPG_CH6_Pin        = GPIO_PIN_9;
      SPI1_SCK_GPIO_Port      = GPIOG;    SPI1_SCK_Pin        = GPIO_PIN_11;
      WIFI_REST_GPIO_Port     = GPIOG;    WIFI_REST_Pin       = GPIO_PIN_14;
  }
  else
  {
      WIFI_REST_GPIO_Port     = GPIOA;    WIFI_REST_Pin       = GPIO_PIN_15;
      REPG_CH6_GPIO_Port      = GPIOB;    REPG_CH6_Pin        = GPIO_PIN_4;
      I2CLK_GPIO_Port         = GPIOB;    I2CLK_Pin           = GPIO_PIN_6;
      I2DAT_GPIO_Port         = GPIOB;    I2DAT_Pin           = GPIO_PIN_7;
      REPG_CH11_GPIO_Port     = GPIOD;    REPG_CH11_Pin       = GPIO_PIN_7;
      HSM_RX_GPIO_Port        = GPIOG;    HSM_RX_Pin          = GPIO_PIN_9;
      REPG_CH12_GPIO_Port     = GPIOG;    REPG_CH12_Pin       = GPIO_PIN_11;
      HSM_TX_GPIO_Port        = GPIOG;    HSM_TX_Pin          = GPIO_PIN_14;
  }

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  HAL_GPIO_WritePin(KL_TXD2_INV_GPIO_Port, KL_TXD2_INV_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(KL_TXD1_INV_GPIO_Port, KL_TXD1_INV_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOF, KL_RXD2_SEL_Pin|SPI5_NSS_Pin|OSC_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, USB_HUB_RST_Pin|USB_ETH_NRST_Pin|TRIG_LED_EN_Pin|HL_CAN_SW_EN_Pin
					|DLCB_EN14_Pin|DLCB_EN15_Pin|DLCA_EN15_1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, TXD1_510_EN_Pin|PWR_DET_EN_24V_Pin|PWR_DET_EN_12V_Pin|LAT_H_CAN_RX_EN2_Pin
					|LAT_H_CAN_RX_EN1_Pin|TRG_WAK_UP_EN_Pin|LAT_IG_DET_EN_Pin|LAT_CLK_Pin
					|LAT_SENS_EN_Pin|ETH_PWR_EN_Pin|REPG_ON_Pin|LOW_CAN_EN_Pin|H_CAN2_SW_EN_Pin
					|POC_IN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, IG_ON_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(EMMC_RST_GPIO_Port, EMMC_RST_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOH, USB_PHY_PWR_EN_Pin|USB_PHY_RST_Pin|TXD1_47K_EN_Pin|LAT_L_CAN_RX_EN_Pin|LOW_CAN_NSTB_Pin
					|TXD2_47K_EN_Pin|TXD1_2K_EN_Pin|TXD2_510_EN_Pin|DLCA_EN13_Pin|DLCA_EN1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, USB_HUB_PWR_EN_Pin|H_STB_EN1_Pin|H_STB_EN2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOI, KL_RXD1_INV_Pin|KL_RXD2_INV_Pin|SPI2_NSS_Pin
					|ETH_SW_EN_Pin|RED_LED_EN_Pin|GREEN_LED_EN_Pin|BLUE_LED_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, TXD2_2K_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOG, EMMC_PWR_EN_Pin|DLCA_EN8_1_Pin|DLCB_EN9_Pin|DLCB_EN10_Pin
					|DLCB_EN11_Pin|DLCB_EN12_Pin|HSM_RST_Pin|HSM_PWR_EN_Pin
					|REPG_CH9_Pin|REPG_CH13_Pin|REPG_CH14_Pin
					|REPG_CH3_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, DLCA_EN2_Pin|DLCA_EN3_Pin|DLCA_EN6_Pin|DLCA_EN7_Pin
					|DLCB_EN8_Pin|WIFI_BT_PWR_EN_Pin, GPIO_PIN_RESET);
                       
  HAL_GPIO_WritePin(WIFI_REST_GPIO_Port, WIFI_REST_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(REPG_CH12_GPIO_Port, REPG_CH12_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(REPG_CH11_GPIO_Port, REPG_CH11_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(REPG_CH6_GPIO_Port, REPG_CH6_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(HIGHCAN_EN614_GPIO_Port, HIGHCAN_EN614_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LP_WAKEUP_IN_GPIO_Port, LP_WAKEUP_IN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(ULP_WAKEUP_IN_GPIO_Port, ULP_WAKEUP_IN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(KL_RXD1_SEL_GPIO_Port, KL_RXD1_SEL_Pin, GPIO_PIN_RESET);

  /* --- Port A --- */
  GPIO_InitStruct.Pin = WAK_UP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SPI2_INTR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = KL_RXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = USB_HUB_RST_Pin|USB_ETH_NRST_Pin|EMMC_RST_Pin|TRIG_LED_EN_Pin|HL_CAN_SW_EN_Pin|DLCB_EN14_Pin|DLCB_EN15_Pin|DLCA_EN15_1_Pin;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* --- Port B --- */
  GPIO_InitStruct.Pin = TXD2_2K_EN_Pin;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* --- Port C --- */
  GPIO_InitStruct.Pin = USB_HUB_PWR_EN_Pin|WIFI_HOST_WAKE_Pin;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = H_STB_EN1_Pin|H_STB_EN2_Pin;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* --- Port D --- */
  GPIO_InitStruct.Pin = IG_ON_EN_Pin;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = DLCA_EN2_Pin|DLCA_EN3_Pin|DLCA_EN6_Pin|DLCA_EN7_Pin|DLCB_EN8_Pin|WIFI_BT_PWR_EN_Pin;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* --- Port E --- */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = ETH_PWR_EN_Pin|KL_TXD2_INV_Pin|TXD1_510_EN_Pin|PWR_DET_EN_24V_Pin|PWR_DET_EN_12V_Pin
                          |LAT_H_CAN_RX_EN2_Pin|LAT_H_CAN_RX_EN1_Pin|H_CAN2_SW_EN_Pin|TRG_WAK_UP_EN_Pin|LAT_IG_DET_EN_Pin
                          |LAT_CLK_Pin|REPG_ON_Pin|LOW_CAN_EN_Pin|POC_IN_Pin|LAT_SENS_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* --- Port F --- */
  GPIO_InitStruct.Pin = FD_INT2_Pin|TX_INT2_Pin|RX_INT2_Pin|TRIG_KEY_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = PAIR_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SPI5_NSS_Pin|OSC_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = KL_RXD2_SEL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(KL_RXD2_SEL_GPIO_Port, &GPIO_InitStruct);

  /* --- Port G --- */
  GPIO_InitStruct.Pin = PGANG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = HSM_RST_Pin;
  GPIO_InitStruct.Mode = (g_HSM_Type == HSM_TYPE_NEW) ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = EMMC_PWR_EN_Pin|DLCA_EN8_1_Pin|DLCB_EN9_Pin|DLCB_EN10_Pin
                          |DLCB_EN11_Pin|DLCB_EN12_Pin|HSM_PWR_EN_Pin
                          |REPG_CH9_Pin|REPG_CH13_Pin|REPG_CH14_Pin|REPG_CH3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* --- Port H --- */
  GPIO_InitStruct.Pin = KL_RXD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = USB_PHY_PWR_EN_Pin|USB_PHY_RST_Pin|TXD1_47K_EN_Pin|LAT_L_CAN_RX_EN_Pin|LOW_CAN_NSTB_Pin
                          |TXD2_47K_EN_Pin|TXD1_2K_EN_Pin|TXD2_510_EN_Pin|DLCA_EN13_Pin|DLCA_EN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /* --- Port I --- */
  GPIO_InitStruct.Pin = SPI2_NSS_Pin|KL_TXD1_INV_Pin|KL_RXD1_INV_Pin|KL_RXD2_INV_Pin|BLUE_LED_EN_Pin
                       |ETH_SW_EN_Pin|RED_LED_EN_Pin|GREEN_LED_EN_Pin;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /* Dynamic pins configuration */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  GPIO_InitStruct.Pin = WIFI_REST_Pin;
  HAL_GPIO_Init(WIFI_REST_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = REPG_CH6_Pin;
  HAL_GPIO_Init(REPG_CH6_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = REPG_CH11_Pin;
  HAL_GPIO_Init(REPG_CH11_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = REPG_CH12_Pin;
  HAL_GPIO_Init(REPG_CH12_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = LP_WAKEUP_IN_Pin;
  HAL_GPIO_Init(LP_WAKEUP_IN_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = ULP_WAKEUP_IN_Pin;
  HAL_GPIO_Init(ULP_WAKEUP_IN_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = HIGHCAN_EN614_Pin;
  HAL_GPIO_Init(HIGHCAN_EN614_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = KL_RXD1_SEL_Pin;
  HAL_GPIO_Init(KL_RXD1_SEL_GPIO_Port, &GPIO_InitStruct);

  /* 7. EXTI interrupt init */
  HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
  HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE BEGIN 2 */

/**
  * @brief  HSM type detection function
  * @note   Detects old/new HSM by reading hardware pin
  * @retval None
  */
void HSM_Type_Detect(void)
{
	GPIO_PinState board_id = HAL_GPIO_ReadPin(BOARD_ID1_GPIO_Port, BOARD_ID1_Pin);

	if (board_id == GPIO_PIN_SET)
	{
		g_HSM_Type = HSM_TYPE_NEW;		// New HSM (SPI)
	}
	else
	{
		g_HSM_Type = HSM_TYPE_OLD;		// Old HSM (UART)
	}
}

/* USER CODE END 2 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
