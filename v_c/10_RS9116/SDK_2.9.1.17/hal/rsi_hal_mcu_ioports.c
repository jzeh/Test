/*******************************************************************************
* @file  rsi_hal_mcu_ioports.c
* @brief 
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* The licensor of this software is Silicon Laboratories Inc. Your use of this
* software is governed by the terms of Silicon Labs Master Software License
* Agreement (MSLA) available at
* www.silabs.com/about-us/legal/master-software-license-agreement. This
* software is distributed to you in Source Code format and is governed by the
* sections of the MSLA applicable to Source Code.
*
******************************************************************************/
/**
 * @file       rsi_hal_mcu_ioports.c
 * @version    0.1
 * @date       18 sept 2015
 *
 *
 *
 * @brief Functions to control IO pins of the microcontroller
 *  
 * @section Description
 * This file contains API to control different pins of the microcontroller 
 * which interface with the module and other components related to the module. 
 *
 */


/**
 * Includes
 */
#include "rsi_driver.h"
#include "stm32h7xx_hal.h"
#include "main.h"
#include "rsi_hal.h"
/**
 * Global Variales
 */





/*===========================================================*/
/**
 * @fn            void rsi_hal_config_gpio(uint8_t gpio_number,uint8_t mode,uint8_t value)
 * @brief         Configures gpio pin in output mode,with a value
 * @param[in]     uint8_t gpio_number, gpio pin number to be configured
 * @param[in]     uint8_t mode , input/output mode of the gpio pin to configure
 *                0 - input mode
 *                1 - output mode
 * @param[in]     uint8_t value, default value to be driven if gpio is configured in output mode
 *                0 - low
 *                1 - high
 * @param[out]    none
 * @return        none
 * @description This API is used to configure host gpio pin in output mode. 
 */
void rsi_hal_config_gpio(uint8_t gpio_number,uint8_t mode,uint8_t value)
{
	GPIO_InitTypeDef GPIO_InitStruct;

    //! Initialise the gpio pins in input/output mode
	switch( gpio_number )
	{
		case RSI_HAL_RESET_PIN	:										// 1 - PF4
		{
			GPIO_InitStruct.Pin		= WIFI_REST_Pin;
			GPIO_InitStruct.Mode	= GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull	= GPIO_PULLUP;
			GPIO_InitStruct.Speed	= GPIO_SPEED_LOW;
			HAL_GPIO_Init(WIFI_REST_GPIO_Port, &GPIO_InitStruct);

			HAL_GPIO_WritePin( WIFI_REST_GPIO_Port, WIFI_REST_Pin, (GPIO_PinState)value );
			break;
		}

//		case RSI_HAL_MODULE_INTERRUPT_PIN :								// 2 - PF5
//		{
//			GPIO_InitStruct.Pin		= GPIO_PIN_5;
//			GPIO_InitStruct.Mode	= GPIO_MODE_INPUT;
//			GPIO_InitStruct.Pull	= GPIO_PULLDOWN;
//			HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
//			break;
//		}

//		case RSI_HAL_WAKEUP_INDICATION_PIN :							// 3 - PA0
//		{
//			GPIO_InitStruct.Pin		= GPIO_PIN_0;
//			GPIO_InitStruct.Mode	= GPIO_MODE_INPUT;
//			GPIO_InitStruct.Pull	= GPIO_NOPULL;
//			HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//			break;
//		}

//		case RSI_HAL_SLEEP_CONFIRM_PIN :								// 4 - PC0
//		{
//			GPIO_InitStruct.Pin		= GPIO_PIN_0;
//			GPIO_InitStruct.Mode	= GPIO_MODE_OUTPUT_PP;
//			GPIO_InitStruct.Pull	= GPIO_NOPULL;
//			GPIO_InitStruct.Speed	= GPIO_SPEED_LOW;
//			HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
//
//			HAL_GPIO_WritePin( GPIOC, GPIO_PIN_0, (GPIO_PinState)value );
//			break;
//		}

//		case RSI_HAL_INTERFACE_READY_PIN :								// 5 - PA4
//		{
//			GPIO_InitStruct.Pin		= GPIO_PIN_4;
//			GPIO_InitStruct.Mode	= GPIO_MODE_OUTPUT_PP;
//			GPIO_InitStruct.Pull	= GPIO_PULLUP;
//			GPIO_InitStruct.Speed	= GPIO_SPEED_LOW;
//			HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//			HAL_GPIO_WritePin( GPIOA, GPIO_PIN_4, (GPIO_PinState)value );
//			break;
//		}

//		case RSI_HAL_LP_SLEEP_CONFIRM_PIN : 							// 6 - PF10
//		{
//			GPIO_InitStruct.Pin		= GPIO_PIN_10;
//			GPIO_InitStruct.Mode	= GPIO_MODE_OUTPUT_PP;
//			GPIO_InitStruct.Pull	= GPIO_NOPULL;
//			GPIO_InitStruct.Speed	= GPIO_SPEED_LOW;
//			HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
//
//			HAL_GPIO_WritePin( GPIOF, GPIO_PIN_10, (GPIO_PinState)value );
//			break;
//		}

		case RSI_HAL_MODULE_POWER_PIN :									// 10  - PF8
		{
			GPIO_InitStruct.Pin		= WIFI_BT_PWR_EN_Pin;
			GPIO_InitStruct.Mode	= GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull	= GPIO_PULLUP;
			GPIO_InitStruct.Speed	= GPIO_SPEED_LOW;
			HAL_GPIO_Init(WIFI_BT_PWR_EN_GPIO_Port, &GPIO_InitStruct);

			HAL_GPIO_WritePin( WIFI_BT_PWR_EN_GPIO_Port, WIFI_BT_PWR_EN_Pin, (GPIO_PinState)value );
			break;
		}
		
		case RSI_HAL_POC_IN_PIN :
		{
		  	GPIO_InitStruct.Pin		= POC_IN_Pin;
			GPIO_InitStruct.Mode	= GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull	= GPIO_PULLUP;
			GPIO_InitStruct.Speed	= GPIO_SPEED_LOW;
			HAL_GPIO_Init(POC_IN_GPIO_Port, &GPIO_InitStruct);

			HAL_GPIO_WritePin( POC_IN_GPIO_Port, POC_IN_Pin, (GPIO_PinState)value );
		}
	}
}

/*===========================================================*/
/**
 * @fn            void rsi_hal_set_gpio(uint8_t gpio_number)
 * @brief         Makes/drives the gpio  value high
 * @param[in]     uint8_t gpio_number, gpio pin number
 * @param[out]    none
 * @return        none 
 * @description   This API is used to drives or makes the host gpio value high. 
 */


void rsi_hal_set_gpio(uint8_t gpio_number)
{
	
  //! drives a high value on GPIO 
	switch( gpio_number )
	{
		case RSI_HAL_RESET_PIN	:			HAL_GPIO_WritePin(WIFI_REST_GPIO_Port, WIFI_REST_Pin,GPIO_PIN_SET);						    break;
//		case RSI_HAL_MODULE_INTERRUPT_PIN :	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_5, GPIO_PIN_SET);				break;
//		case RSI_HAL_WAKEUP_INDICATION_PIN :HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);				break;
//		case RSI_HAL_SLEEP_CONFIRM_PIN :	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);				break;
//		case RSI_HAL_INTERFACE_READY_PIN :	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);				break;
//		case RSI_HAL_LP_SLEEP_CONFIRM_PIN : HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10,GPIO_PIN_SET);				break;
		case RSI_HAL_MODULE_POWER_PIN :		HAL_GPIO_WritePin(WIFI_BT_PWR_EN_GPIO_Port, WIFI_BT_PWR_EN_Pin,GPIO_PIN_SET);				break;
		case RSI_HAL_POC_IN_PIN :			HAL_GPIO_WritePin(POC_IN_GPIO_Port, POC_IN_Pin, GPIO_PIN_SET);
	}
}




/*===========================================================*/
/**
 * @fn          uint8_t rsi_hal_get_gpio(void)
 * @brief       get the gpio pin value
 * @param[in]   uint8_t gpio_number, gpio pin number
 * @param[out]  none  
 * @return      gpio pin value 
 * @description This API is used to configure get the gpio pin value. 
 */
uint8_t rsi_hal_get_gpio(uint8_t gpio_number)
{
  volatile uint8_t gpio_value = 0;

	//! Get the gpio value
	switch( gpio_number )
	{
//		case RSI_HAL_RESET_PIN	:		gpio_value = HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_10 );			break;
		case RSI_HAL_MODULE_INTERRUPT_PIN :	gpio_value = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9 );			break;
//		case RSI_HAL_WAKEUP_INDICATION_PIN :	gpio_value = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0 );			break;
//		case RSI_HAL_SLEEP_CONFIRM_PIN :	gpio_value = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0 );			break;
//		case RSI_HAL_INTERFACE_READY_PIN :	gpio_value = HAL_GPIO_ReadPin(SPI2_INTR_GPIO_Port, SPI2_INTR_Pin );			break;
//		case RSI_HAL_MODULE_INTERRUPT_PIN:	gpio_value = HAL_GPIO_ReadPin(SPI2_INTR_GPIO_Port, SPI2_INTR_Pin );			break;
//		case RSI_HAL_LP_SLEEP_CONFIRM_PIN : 	gpio_value = HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_10);			break;
		case RSI_HAL_MODULE_POWER_PIN :		gpio_value = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3);			break;
	}

	return gpio_value;

}




/*===========================================================*/
/**
 * @fn            void rsi_hal_set_gpio(uint8_t gpio_number)
 * @brief         Makes/drives the gpio value to low
 * @param[in]     uint8_t gpio_number, gpio pin number
 * @param[out]    none
 * @return        none 
 * @description   This API is used to drives or makes the host gpio value low. 
 */
void rsi_hal_clear_gpio(uint8_t gpio_number)
{
	//! drives a low value on GPIO
	switch( gpio_number )
	{
		case RSI_HAL_RESET_PIN	:			HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);			break;
//		case RSI_HAL_MODULE_INTERRUPT_PIN :	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_5, GPIO_PIN_RESET);			break;
//		case RSI_HAL_WAKEUP_INDICATION_PIN :HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);			break;
//		case RSI_HAL_SLEEP_CONFIRM_PIN :	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);			break;
//		case RSI_HAL_INTERFACE_READY_PIN :	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);			break;
//		case RSI_HAL_LP_SLEEP_CONFIRM_PIN : HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10,GPIO_PIN_RESET);			break;
		case RSI_HAL_MODULE_POWER_PIN :		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8,GPIO_PIN_RESET);			break;
	}
}


