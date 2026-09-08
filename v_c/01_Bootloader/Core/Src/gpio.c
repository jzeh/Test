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

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins
     PC14-OSC32_IN (OSC32_IN)   ------> RCC_OSC32_IN
     PC15-OSC32_OUT (OSC32_OUT)   ------> RCC_OSC32_OUT
     PH0-OSC_IN (PH0)   ------> RCC_OSC_IN
     PH1-OSC_OUT (PH1)   ------> RCC_OSC_OUT
     PA0   ------> PWR_WKUP0
     PA13 (JTMS/SWDIO)   ------> DEBUG_JTMS-SWDIO
     PA14 (JTCK/SWCLK)   ------> DEBUG_JTCK-SWCLK
     PB3 (JTDO/TRACESWO)   ------> DEBUG_JTDO-SWO
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, KL_TXD2_INV_Pin|TXD1_510_EN_Pin|PWR_DET_EN_24V_Pin|PWR_DET_EN_12V_Pin
                          |LAT_H_CAN_RX_EN2_Pin|LAT_H_CAN_RX_EN1_Pin|TRG_WAK_UP_EN_Pin|LAT_IG_DET_EN_Pin
                          |LAT_CLK_Pin|LAT_SENS_EN_Pin|ETH_PWR_EN_Pin|REPG_ON_Pin|LOW_CAN_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
#ifdef NEWBOARD  
  HAL_GPIO_WritePin(GPIOI, KL_TXD1_INV_Pin|KL_RXD1_INV_Pin|KL_RXD2_INV_Pin|BLUE_LED_EN_Pin
                            |SPI2_NSS_Pin|ETH_SW_EN_Pin|RED_LED_EN_Pin|GREEN_LED_EN_Pin, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOH, USB_PHY_PWR_EN_Pin|USB_PHY_RST_Pin|HIGHCAN_EN614_Pin|TXD1_47K_EN_Pin
                          |LAT_L_CAN_RX_EN_Pin|LOW_CAN_NSTB_Pin|TXD2_47K_EN_Pin|TXD1_2K_EN_Pin
                          |TXD2_510_EN_Pin|DLCA_EN13_Pin|DLCA_EN1_Pin|KL_RXD1_SEL_Pin, GPIO_PIN_RESET);
  
    /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, USB_HUB_PWR_EN_Pin|H_STB_EN1_Pin|H_STB_EN2_Pin|LP_WAKEUP_IN_Pin
                          |ULP_WAKEUP_IN_Pin, GPIO_PIN_RESET);
#else  
  HAL_GPIO_WritePin(GPIOI, KL_TXD1_INV_Pin|KL_RXD1_INV_Pin|KL_RXD2_INV_Pin|KL_RXD1_SEL_Pin
                            |SPI2_NSS_Pin|ETH_SW_EN_Pin|RED_LED_EN_Pin|GREEN_LED_EN_Pin
                            |BLUE_LED_EN_Pin, GPIO_PIN_RESET);
  
    /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOH, USB_PHY_PWR_EN_Pin|USB_PHY_RST_Pin|HIGHCAN_EN614_Pin|TXD1_47K_EN_Pin
                          |LAT_L_CAN_RX_EN_Pin|LOW_CAN_NSTB_Pin|TXD2_47K_EN_Pin|TXD1_2K_EN_Pin
                          |TXD2_510_EN_Pin|DLCA_EN13_Pin|DLCA_EN1_Pin|LP_WAKEUP_IN_Pin
                          |ULP_WAKEUP_IN_Pin, GPIO_PIN_RESET);
    
      /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, USB_HUB_PWR_EN_Pin|H_STB_EN1_Pin|H_STB_EN2_Pin, GPIO_PIN_RESET);
#endif
  
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, KL_RXD2_SEL_Pin|SPI5_NSS_Pin|OSC_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, USB_HUB_RST_Pin|USB_ETH_NRST_Pin|TRIG_LED_EN_Pin|HL_CAN_SW_EN_Pin
                          |EMMC_RST_Pin|DLCB_EN14_Pin|DLCB_EN15_Pin|DLCA_EN15_1_Pin
                          , GPIO_PIN_RESET);
  
  HAL_GPIO_WritePin(GPIOA, WIFI_REST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TXD2_2K_EN_Pin|REPG_CH6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, EMMC_PWR_EN_Pin|DLCA_EN8_1_Pin|DLCB_EN9_Pin|DLCB_EN10_Pin
                          |DLCB_EN11_Pin|DLCB_EN12_Pin|HSM_RST_Pin|HSM_PWR_EN_Pin
                          |REPG_CH9_Pin|REPG_CH12_Pin|REPG_CH13_Pin|REPG_CH14_Pin
                          |REPG_CH3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, DLCA_EN2_Pin|DLCA_EN3_Pin|DLCA_EN6_Pin|DLCA_EN7_Pin
                          |DLCB_EN8_Pin|WIFI_BT_PWR_EN_Pin/*|IG_ON_EN_Pin*/|REPG_CH11_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, IG_ON_EN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PE2 PE4 PE5 PE6 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PEPin PEPin PEPin PEPin
                           PEPin PEPin PEPin PEPin
                           PEPin PEPin PEPin PEPin 
                           PEPin */
  GPIO_InitStruct.Pin = KL_TXD2_INV_Pin|TXD1_510_EN_Pin|PWR_DET_EN_24V_Pin|PWR_DET_EN_12V_Pin
                          |LAT_H_CAN_RX_EN2_Pin|LAT_H_CAN_RX_EN1_Pin|TRG_WAK_UP_EN_Pin|LAT_IG_DET_EN_Pin
                          |LAT_CLK_Pin|LAT_SENS_EN_Pin|ETH_PWR_EN_Pin|REPG_ON_Pin
                          |LOW_CAN_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  
#ifdef NEWBOARD
  /*Configure GPIO pins : PIPin PIPin PIPin PIPin
                           PIPin PIPin PIPin PIPin */
  GPIO_InitStruct.Pin = KL_TXD1_INV_Pin|KL_RXD1_INV_Pin|KL_RXD2_INV_Pin|BLUE_LED_EN_Pin
                          |SPI2_NSS_Pin|ETH_SW_EN_Pin|RED_LED_EN_Pin|GREEN_LED_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
  
  /*Configure GPIO pins : PHPin PHPin PHPin PHPin
                           PHPin PHPin PHPin PHPin
                           PHPin PHPin PHPin PHPin */
  GPIO_InitStruct.Pin = USB_PHY_PWR_EN_Pin|USB_PHY_RST_Pin|HIGHCAN_EN614_Pin|TXD1_47K_EN_Pin
                          |LAT_L_CAN_RX_EN_Pin|LOW_CAN_NSTB_Pin|TXD2_47K_EN_Pin|TXD1_2K_EN_Pin
                          |TXD2_510_EN_Pin|DLCA_EN13_Pin|DLCA_EN1_Pin|KL_RXD1_SEL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
  
  /*Configure GPIO pins : PCPin PCPin PCPin PCPin
                           PCPin */
  GPIO_InitStruct.Pin = USB_HUB_PWR_EN_Pin|H_STB_EN1_Pin|H_STB_EN2_Pin|LP_WAKEUP_IN_Pin
                          |ULP_WAKEUP_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
#else
  /*Configure GPIO pins : PIPin PIPin PIPin PIPin
                        PIPin PIPin PIPin PIPin
                        PIPin */
  GPIO_InitStruct.Pin = KL_TXD1_INV_Pin|KL_RXD1_INV_Pin|KL_RXD2_INV_Pin|KL_RXD1_SEL_Pin
                       |SPI2_NSS_Pin|ETH_SW_EN_Pin|RED_LED_EN_Pin|GREEN_LED_EN_Pin
                       |BLUE_LED_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
  
  /*Configure GPIO pins : PHPin PHPin PHPin PHPin
                        PHPin PHPin PHPin PHPin
                        PHPin PHPin PHPin PHPin
                        PHPin */
  GPIO_InitStruct.Pin = USB_PHY_PWR_EN_Pin|USB_PHY_RST_Pin|HIGHCAN_EN614_Pin|TXD1_47K_EN_Pin
                       |LAT_L_CAN_RX_EN_Pin|LOW_CAN_NSTB_Pin|TXD2_47K_EN_Pin|TXD1_2K_EN_Pin
                       |TXD2_510_EN_Pin|DLCA_EN13_Pin|DLCA_EN1_Pin|LP_WAKEUP_IN_Pin
                       |ULP_WAKEUP_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
  
    /*Configure GPIO pins : PCPin PCPin PCPin */
  GPIO_InitStruct.Pin = USB_HUB_PWR_EN_Pin|H_STB_EN1_Pin|H_STB_EN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
#endif


  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = WIFI_HOST_WAKE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  //GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(WIFI_HOST_WAKE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PF0 PF1 PF3 PF7
                           PF8 PF9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PFPin PFPin PFPin PFPin
                           PFPin */
  GPIO_InitStruct.Pin = FD_INT2_Pin|TX_INT2_Pin|RX_INT2_Pin|PAIR_SW_Pin
                          |TRIG_KEY_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PFPin PFPin PFPin */
  GPIO_InitStruct.Pin = KL_RXD2_SEL_Pin|SPI5_NSS_Pin|OSC_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PAPin PAPin PAPin PAPin
                           PAPin PAPin PAPin PAPin
                           PAPin */
  GPIO_InitStruct.Pin = USB_HUB_RST_Pin|USB_ETH_NRST_Pin|TRIG_LED_EN_Pin|HL_CAN_SW_EN_Pin
                          |EMMC_RST_Pin|DLCB_EN14_Pin|DLCB_EN15_Pin|DLCA_EN15_1_Pin
                          |WIFI_REST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = KL_RXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KL_RXD1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PBPin PBPin */
  GPIO_InitStruct.Pin = TXD2_2K_EN_Pin|REPG_CH6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = PGANG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PGANG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = EMMC_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EMMC_PWR_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = EMMC_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(EMMC_DATA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = KL_RXD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KL_RXD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB14 PB15 PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD0 PD1 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PDPin PDPin PDPin PDPin
                           PDPin PDPin PDPin PDPin */
  GPIO_InitStruct.Pin = DLCA_EN2_Pin|DLCA_EN3_Pin|DLCA_EN6_Pin|DLCA_EN7_Pin
                          |DLCB_EN8_Pin|WIFI_BT_PWR_EN_Pin|IG_ON_EN_Pin|REPG_CH11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PGPin PGPin PGPin PGPin
                           PGPin PGPin PGPin PGPin
                           PGPin PGPin PGPin PGPin */
  GPIO_InitStruct.Pin = DLCA_EN8_1_Pin|DLCB_EN9_Pin|DLCB_EN10_Pin|DLCB_EN11_Pin
                          |DLCB_EN12_Pin|HSM_RST_Pin|HSM_PWR_EN_Pin|REPG_CH9_Pin
                          |REPG_CH12_Pin|REPG_CH13_Pin|REPG_CH14_Pin|REPG_CH3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = SPI2_INTR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SPI2_INTR_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PI1 PI2 PI3 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
