/*******************************************************************************
* @file  rsi_hal_mcu_platform_init.c
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
 * @file       rsi_hal_mcu_platform_init.c
 * @version    0.1
 * @date       11 OCT 2018
 *
 *
 *
 * @brief HAL Board Init: Functions related to platform initialization
 *
 * @section Description
 * This file contains the list of functions for configuring the microcontroller clock.
 * Following are list of API's which need to be defined in this file.
 *
 */
/**
 * Includes
 */
#include "rsi_driver.h"
#include "stm32h7xx_hal.h"

#include "rsi_driver.h"
#include "rsi_hal.h"
#include "rsi_board_configuration.h"
#include "stdio.h"
#ifdef RSI_M4_INTERFACE
#include "rsi_m4.h"
#endif

//#ifdef RSI_WITH_OS
//#include "FreeRTOS.h"
////#include "cmsis_os.h"
//#include "task.h"
////osThreadId_t defaultTaskHandle;
//void StartDefaultTask(void *argument);
//volatile TickType_t xTickCount;
//#endif
//
////#include "stm32f4xx_hal_uart.h"
//#if defined(RSI_SPI_INTERFACE)
////SPI_HandleTypeDef hspi1;	//move to main.c
//#elif defined(RSI_UART_INTERFACE)
//UART_HandleTypeDef huart1;
//#endif
//TIM_HandleTypeDef htim2;
//
//uint8_t platform_initialized;
//
//uint8_t	com_port_data;
//
//#ifdef RSI_DEBUG_PRINTS
//static void com_port_init(void);
//UART_HandleTypeDef com_port;
//
//#ifdef __GNUC__
//  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
//#else
//  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
//#endif /* __GNUC__ */
//
//#endif
//
//#if defined(RSI_SPI_INTERFACE)
////SPI_HandleTypeDef hspi1;	//move to main.c
////static void MX_SPI1_Init(void);
//#elif defined(RSI_UART_INTERFACE)
//UART_HandleTypeDef huart1;
//static void MX_USART1_UART_Init(void);
//#endif
//
////static void SystemClock_Config(void);
////static void MX_GPIO_Init(void);
////static void MX_DMA_Init(void);
//#ifdef RSI_WITH_OS
//static void MX_TIM2_Init(void);
//#endif

/*==============================================*/
/**
 * @fn           void rsi_hal_board_init()
 * @brief        This function Initializes the platform
 * @param[in]    none
 * @param[out]   none
 * @return       none
 * @section description
 * This function initializes the platform
 *
 */

//void rsi_hal_board_init()
//{
//  //! Initializes the platform
//  HAL_Init();
//	
//  /* Configure the system clock */
//  SystemClock_Config();
//	
//  /* Initialize all configured peripherals */
//  MX_GPIO_Init();
//#ifdef RSI_SPI_INTERFACE
//
//  //! Intializes SPI
//  MX_SPI1_Init();
//	
//#elif RSI_UART_INTERFACE
//
//  //! Intializes UART	
//  HAL_UART_Init(&huart1);
//
//  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
//
//  //! abrd detection
//  ABRD();
//	
//#endif
//}

/**
  * @brief System Clock Configuration
  * @retval None
  */ //move to main.c
//void SystemClock_Config(void)
//{
//  RCC_OscInitTypeDef RCC_OscInitStruct;
//
//  __PWR_CLK_ENABLE();
//
//  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
//
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
//  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
//  RCC_OscInitStruct.HSICalibrationValue = 16;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
//  HAL_RCC_OscConfig(&RCC_OscInitStruct);
//
//  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
//
//  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
//
//  /* SysTick_IRQn interrupt configuration */
//  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
//}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
//move to main.c
//#ifdef RSI_SPI_INTERFACE
///* SPI1 init function */
//void MX_SPI1_Init(void)
//{
//  /* SPI1 parameter configuration*/
//  hspi1.Instance = SPI1;
//  hspi1.Init.Mode = SPI_MODE_MASTER;
//  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
//  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
//  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
//  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
//  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
//  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
//  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
//  hspi1.Init.TIMode = SPI_TIMODE_DISABLED;
//  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
//  hspi1.Init.CRCPolynomial = 10;
//  HAL_SPI_Init(&hspi1);
//}
//#endif

#ifdef RSI_WITH_OS
/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
//void MX_TIM2_Init(void)
//{
//
//	/* USER CODE BEGIN TIM2_Init 0 */
//
//	/* USER CODE END TIM2_Init 0 */
//
//	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
//	TIM_MasterConfigTypeDef sMasterConfig = {0};
//
//	/* USER CODE BEGIN TIM2_Init 1 */
//
//	/* USER CODE END TIM2_Init 1 */
//	htim2.Instance = TIM2;
//	htim2.Init.Prescaler = 48000;   //As the clock is running at 48MHz
//	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
//	//htim2.Init.Period = 1000;  //1 second
//	htim2.Init.Period = 1000*30; // 30 sec
//
//	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
//	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
//	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
//	{
//		Error_Handler();
//	}
//	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
//	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
//	{
//		Error_Handler();
//	}
//	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
//	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
//	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
//	{
//		Error_Handler();
//	}
//	/* USER CODE BEGIN TIM2_Init 2 */
//
//	/* USER CODE END TIM2_Init 2 */
//
//}
#endif

/**
 * Enable DMA controller clock
 */
//void MX_DMA_Init(void)
//{
//
//  /* DMA controller clock enable */
//  __HAL_RCC_DMA2_CLK_ENABLE();
//
//  /* DMA interrupt init */
//  /* DMA2_Stream0_IRQn interrupt configuration */
//  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
//  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
//  /* DMA2_Stream2_IRQn interrupt configuration */
//  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0);
//  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
//
//}
#if defined(RSI_UART_INTERFACE)
/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
//static void MX_USART1_UART_Init(void)
//{
//
//  /* USER CODE BEGIN USART1_Init 0 */
//
//  /* USER CODE END USART1_Init 0 */
//
//  /* USER CODE BEGIN USART1_Init 1 */
//
//  /* USER CODE END USART1_Init 1 */
//  huart1.Instance = USART1;
//  huart1.Init.BaudRate = 115200;
//  huart1.Init.WordLength = UART_WORDLENGTH_8B;
//  huart1.Init.StopBits = UART_STOPBITS_1;
//  huart1.Init.Parity = UART_PARITY_NONE;
//  huart1.Init.Mode = UART_MODE_TX_RX;
//  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
//  if (HAL_UART_Init(&huart1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN USART1_Init 2 */
//
//  /* USER CODE END USART1_Init 2 */
//
//}

#endif

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 *///move to main.c
//void MX_GPIO_Init(void)
//{
//  /* GPIO Ports Clock Enable */
//  __GPIOA_CLK_ENABLE();
//#ifdef RSI_SPI_INTERFACE
//  GPIO_InitTypeDef GPIO_InitStruct;
//
//  __GPIOB_CLK_ENABLE();
//  __GPIOC_CLK_ENABLE();
//
//  /*Configure GPIO pin : PA9 */
//  GPIO_InitStruct.Pin = GPIO_PIN_9;
//  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING; // Used for active high firmware image
//  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//  /*Configure GPIO pin : PB6 */
//  GPIO_InitStruct.Pin = GPIO_PIN_6;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
//  GPIO_InitStruct.Pull = GPIO_PULLUP;
//  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//	
//  /*Configure GPIO pin : PB3 */
//  GPIO_InitStruct.Pin = GPIO_PIN_3;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
//  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
//  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//
//  /* EXTI interrupt init*/
//  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
//  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
//#endif
//}


/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
//move to main.c
//void Error_Handler(void)
//{
//  /* USER CODE BEGIN Error_Handler_Debug */
//  /* User can add his own implementation to report the HAL error return state */
//	ALL_LED_OFF;
//
//	while( 1 )
//	{
//		SD_LED_TOGGLE;
//		PWR_LED_TOGGLE;
//		WIFI_G_LED_TOGGLE;
//		VHC_LED_TOGGLE;
//				
//		osDelay( 300 );
//	}
//  /* USER CODE END Error_Handler_Debug */
//}

/*==============================================*/
/**
 * @fn           void rsi_switch_to_high_clk_freq()
 * @brief        This function intializes SPI to high clock
 * @param[in]    none
 * @param[out]   none
 * @return       none
 * @section description
 * This function intializes SPI to high clock
 *
 *
 */

void rsi_switch_to_high_clk_freq()
{
  //! Initializes the high speed clock
//  hspi1.Instance = SPI1;
//  hspi1.Init.Mode = SPI_MODE_MASTER;
//  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
//  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
//  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
//  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
//  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
//  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
//  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
//  hspi1.Init.TIMode = SPI_TIMODE_DISABLED;
//  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
//  hspi1.Init.CRCPolynomial = 10;
//  HAL_SPI_Init(&hspi1);
}


#ifdef RSI_DEBUG_PRINTS
static void com_port_init(void)
{

  com_port.Instance = COM_PORT_PERIPHERAL;
  com_port.Init.BaudRate = 115200;
  com_port.Init.WordLength = UART_WORDLENGTH_8B;
  com_port.Init.StopBits = UART_STOPBITS_1;
  com_port.Init.Parity = UART_PARITY_NONE;
  com_port.Init.Mode = UART_MODE_TX_RX;
  com_port.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  com_port.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&com_port) != HAL_OK)
  {
    Error_Handler();
  }
}
#endif

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
//move to main.c
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//	/* USER CODE BEGIN Callback 0 */
//
//	/* USER CODE END Callback 0 */
//	if (htim->Instance == TIM1) {
//		HAL_IncTick();
//	}
//#if defined(RSI_WITH_OS) && defined(FREERTOS)
//	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
//	{
//		xPortSysTickHandler();
//	}
//#endif
//	/* USER CODE BEGIN Callback 1 */
//
//	/* USER CODE END Callback 1 */
//}
//#ifdef RSI_DEBUG_PRINTS
//PUTCHAR_PROTOTYPE
//{
//	HAL_UART_Transmit(&com_port, (uint8_t *)&ch, 1, 0xFFFF);
//	return ch;
//}
//#endif
