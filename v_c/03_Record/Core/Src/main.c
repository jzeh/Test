/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "fatfs.h"
#include "fdcan.h"
#include "i2c.h"
#include "mdma.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"
#include "rng.h"
#include "crc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "common.h"
#include "led.h"
#include "firmware.h"
#include "sw_timer.h"
#include "git_rs9116.h"
#include "Portable.h"
#include "git_mmc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool	g_pair_state	= true;						// for pair switch debounce
bool	g_trig_state	= true;						// for trigger switch debounce
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern TIM_HandleTypeDef htim7;

#ifdef FEATURE_MCP2518FD
extern osThreadId		hRxMonTh;
#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
#define RAM1_HEAP_SIZE ( 390 * 1024 )
	
static uint8_t ucHeap[ RAM1_HEAP_SIZE ]; 

const HeapRegion_t xHeapRegions[] =
{
	{ ucHeap, RAM1_HEAP_SIZE },					//readwrite 512kb
    { ( uint8_t * ) 0x30000000UL, 0x20000 },	//SRAM1 128kb
	{ ( uint8_t * ) 0x30020000UL, 0x20000 },	//SRAM2 128kb
	{ ( uint8_t * ) 0x30040000UL, 0x8000 },		//SRAM3 32kb
	{ ( uint8_t * ) 0x38000000UL, 0x10000 },	//SRAM4 64kb
    { NULL, 0 } // Terminates the array.
};

int main(void)
{
  int32_t	ret = 0;
  
  /* USER CODE BEGIN 1 */
  vPortDefineHeapRegions( xHeapRegions );
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_RNG_Init();
#ifdef AddToTimer3 //mod.kks 22.04.29
  MX_TIM3_Init();
#endif
  MX_ADC3_Init();
  MX_FDCAN1_Init();
  MX_I2C2_Init();
  if(g_HSM_Type != HSM_TYPE_NEW)
  {
    MX_I2C4_Init();
  }
  MX_RTC_Init();
  MX_SPI2_Init();
  MX_TIM4_Init();
  MX_TIM7_Init();
  MX_TIM17_Init();
  MX_MDMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  if (g_HSM_Type != HSM_TYPE_NEW)
  {
    MX_USART6_UART_Init();
  }
  MX_SPI5_Init();
  MX_CRC_Init();
  if (g_HSM_Type == HSM_TYPE_NEW)
  {
    MX_SPI1_Init();
  }
  /* USER CODE BEGIN 2 */
  IO_CONTROL_LOW( LAT_CLK );

  //if you want use, set the low(active) - When i booting set deactive state
#if 0
  IO_CONTROL_LOW( H_CAN2_SW_EN );       //E6
  IO_CONTROL_LOW( LAT_H_CAN_RX_EN1);    //E11   //LowActive
  IO_CONTROL_LOW( LAT_H_CAN_RX_EN2);    //E10   //LowActive
  IO_CONTROL_LOW( LAT_L_CAN_RX_EN);     //H6    //LowActive
  IO_CONTROL_HIGH( WIFI_BT_PWR_EN);     //D3
#else	//for test
  IO_CONTROL_LOW( H_CAN2_SW_EN );       //E6
  IO_CONTROL_HIGH( LAT_H_CAN_RX_EN1);    //E11  //LowActive
  IO_CONTROL_HIGH( LAT_H_CAN_RX_EN2);    //E10  //LowActive
  IO_CONTROL_HIGH( LAT_L_CAN_RX_EN);     //H6   //LowActive
  IO_CONTROL_HIGH( WIFI_BT_PWR_EN);     //D3
#endif
  
  IO_CONTROL_HIGH( LAT_CLK );
  IO_CONTROL_LOW( LAT_CLK );
  /* USER CODE END 2 */
  ret = initMMC();							// EMMC initialize
  if( ret < 0 )
  {
    GLogEE( "Fail... EMMC Init( %d )\r\n", ret );
    Error_Handler();
  }
  else{}

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();
  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_SDMMC
                              |RCC_PERIPHCLK_SPI2|RCC_PERIPHCLK_SPI5
                              |RCC_PERIPHCLK_FDCAN;
  PeriphClkInitStruct.PLL2.PLL2M = 5;
  PeriphClkInitStruct.PLL2.PLL2N = 160;
  PeriphClkInitStruct.PLL2.PLL2P = 10;
  PeriphClkInitStruct.PLL2.PLL2Q = 10;
  PeriphClkInitStruct.PLL2.PLL2R = 4;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_2;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL2;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
  PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_PLL2;
  PeriphClkInitStruct.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL2;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void)
{
#if 0
	MPU_Region_InitTypeDef MPU_InitStruct = {0};

	/* Disables the MPU */
	HAL_MPU_Disable();
	/** Initializes and configures the Region and the memory to be protected
	*/
	HAL_MPU_Disable();
	MPU_InitStruct.Enable			= MPU_REGION_ENABLE;
	MPU_InitStruct.Number			= MPU_REGION_NUMBER0;
	MPU_InitStruct.BaseAddress		= 0x24000000;
	MPU_InitStruct.Size				= MPU_REGION_SIZE_512KB;
	MPU_InitStruct.SubRegionDisable	= 0x00;
	MPU_InitStruct.TypeExtField		= MPU_TEX_LEVEL1;
	MPU_InitStruct.AccessPermission	= MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec		= MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.IsShareable		= MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable		= MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable		= MPU_ACCESS_NOT_SHAREABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	MPU_InitStruct.Enable			= MPU_REGION_ENABLE;
	MPU_InitStruct.Number			= MPU_REGION_NUMBER1;
	MPU_InitStruct.BaseAddress		= 0x30020000;
	MPU_InitStruct.Size				= MPU_REGION_SIZE_128KB;
	MPU_InitStruct.SubRegionDisable	= 0x0;
	MPU_InitStruct.TypeExtField		= MPU_TEX_LEVEL1;
	MPU_InitStruct.AccessPermission	= MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec		= MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.IsShareable		= MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable		= MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable		= MPU_ACCESS_NOT_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	MPU_InitStruct.Enable			= MPU_REGION_ENABLE;
	MPU_InitStruct.Number			= MPU_REGION_NUMBER2;
	MPU_InitStruct.BaseAddress		= 0x30040000;
	MPU_InitStruct.Size				= MPU_REGION_SIZE_32KB;
	MPU_InitStruct.SubRegionDisable	= 0x0;
	MPU_InitStruct.TypeExtField		= MPU_TEX_LEVEL0;
	MPU_InitStruct.AccessPermission	= MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec		= MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.IsShareable		= MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable		= MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable		= MPU_ACCESS_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	MPU_InitStruct.Enable			= MPU_REGION_ENABLE;
	MPU_InitStruct.Number			= MPU_REGION_NUMBER3;
	MPU_InitStruct.BaseAddress		= 0x30044000;
	MPU_InitStruct.Size				= MPU_REGION_SIZE_16KB;
	MPU_InitStruct.SubRegionDisable	= 0x0;
	MPU_InitStruct.TypeExtField		= MPU_TEX_LEVEL1;
	MPU_InitStruct.AccessPermission	= MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec		= MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.IsShareable		= MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.IsCacheable		= MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable		= MPU_ACCESS_NOT_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	MPU_InitStruct.Enable			= MPU_REGION_ENABLE;
	MPU_InitStruct.Number			= MPU_REGION_NUMBER4;
	MPU_InitStruct.BaseAddress		= 0x60000000;
	MPU_InitStruct.Size				= MPU_REGION_SIZE_64KB;
	MPU_InitStruct.SubRegionDisable	= 0x0;
	MPU_InitStruct.TypeExtField		= MPU_TEX_LEVEL0;
	MPU_InitStruct.AccessPermission	= MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec		= MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.IsShareable		= MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable		= MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable		= MPU_ACCESS_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	MPU_InitStruct.Enable			= MPU_REGION_ENABLE;
	MPU_InitStruct.Number			= MPU_REGION_NUMBER5;
	MPU_InitStruct.BaseAddress		= 0xC0000000;
	MPU_InitStruct.Size				= MPU_REGION_SIZE_4MB;
	MPU_InitStruct.SubRegionDisable	= 0x00;
	MPU_InitStruct.TypeExtField		= MPU_TEX_LEVEL1;
	MPU_InitStruct.AccessPermission	= MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec		= MPU_INSTRUCTION_ACCESS_DISABLE;
	MPU_InitStruct.IsShareable		= MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable		= MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable		= MPU_ACCESS_NOT_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
#else
	MPU_Region_InitTypeDef MPU_InitStruct = {0};
	HAL_MPU_Disable();
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER0;
	MPU_InitStruct.BaseAddress = 0x24000000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	MPU_InitStruct.Number = MPU_REGION_NUMBER1;
	MPU_InitStruct.BaseAddress = 0x30000000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	MPU_InitStruct.Number = MPU_REGION_NUMBER2;
	MPU_InitStruct.BaseAddress = 0x30020000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	MPU_InitStruct.Number = MPU_REGION_NUMBER3;
	MPU_InitStruct.BaseAddress = 0x30040000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	MPU_InitStruct.Number = MPU_REGION_NUMBER4;
	MPU_InitStruct.BaseAddress = 0x38000000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/** Initializes and configures the Region and the memory to be protected
	*/
	MPU_InitStruct.Number = MPU_REGION_NUMBER5;
	MPU_InitStruct.BaseAddress = 0x60000000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	
	MPU_InitStruct.Enable			= MPU_REGION_ENABLE;
	MPU_InitStruct.Number			= MPU_REGION_NUMBER6;
	MPU_InitStruct.BaseAddress		= 0xC0000000;
	MPU_InitStruct.Size				= MPU_REGION_SIZE_4MB;
	MPU_InitStruct.SubRegionDisable	= 0x00;
	MPU_InitStruct.TypeExtField		= MPU_TEX_LEVEL1;
	MPU_InitStruct.AccessPermission	= MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec		= MPU_INSTRUCTION_ACCESS_DISABLE;
	MPU_InitStruct.IsShareable		= MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable		= MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable		= MPU_ACCESS_NOT_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
#if( VCI_III_ASING_MODE ) // mod.kks Request by HW LJH to reduce current level.
int32_t gbasetimer=0;
int8_t gModule_step=0;
/*
Test Sequence.
 Step 1: Ethernet
 Step 2: WAIT
 Step 3: CAN
 Step 4: WAIT
 Step 5: KLINE
 Step 6: WAIT
 Step 7: BT
 Step 8: WAIT
*/
#endif
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

#ifdef AddToTimer3 //mod.kks 22.04.29
	if (htim->Instance == TIM3) {			// 50us
#ifdef FEATURE_MCP2518FD
		if(hRxMonTh != NULL)
			osSignalSet( hRxMonTh, 0x0001 );
#endif
	}
#endif
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
	else if( htim->Instance == TIM17 )						// 1ms tick
	{
		Internal_timer_Proc();
#if( VCI_III_ASING_MODE )//mod.kks to reduce current level request by HW Team LJH. 22.05.
		extern uint8_t g_TriggerKey ;

        if (gbasetimer > 5000) {
            gbasetimer = 0;

            gModule_step++;

			/* HW POWER CONTRL and Stable */
            if(gModule_step == 1)
            {
			GLogN( "\r\n" );
               // CAN Test
            }
			/* HW POWER CONTRL and Stable */
            else if(gModule_step == 2)
            {
			   // Do not waitting
            }
            else if(gModule_step == 3)
            {
			GLogN( "\r\n" );
                // KLINE Test
            }
            else if (gModule_step == 4)
            {
			   // Do not waitting
            }
            else if (gModule_step == 5)
            {
			GLogN( "\r\n" );
                // BT Test
            }
            else if (gModule_step == 6)
            {
                // Do not waitting
            }
            else if (gModule_step == 7)
            {
			gModule_step = 0 ;
            }
		/*
            else if (gModule_step == 8)
			{
                // Do not waitting
            }
            else if (gModule_step == 9)
            {
                // Do not waitting
                if(g_TriggerKey ==1){
                    extern void lan9371Reset(void);
                    lan9371Reset();
                }
                
            }
			*/
            else
                ;

#ifdef KKS_TEST_LOG
		GLogN( "Step(%d) \r\n", gModule_step );
#endif
        }
        else
        {
            gbasetimer++;
        }
#endif
	}
	else if( htim->Instance == TIM7 )					// 50ms sw debounce tick
	{

#if false
		if( g_pair_state == false )
		{
			g_pair_state = true;
			rsi_bt_app_on_scan_req();
		}

#if(!VCI_III_ASING_MODE)
		else if( g_trig_state == false )
		{
			g_trig_state = true;
			GLogN( "Input Trigger Key!!!\r\n" );
			IO_CONTROL_TOGGLE( TRIG_LED_EN );
		}
#endif

#endif

		HAL_TIM_Base_Stop_IT( &htim7 );
	}
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	LED_ALL_OFF;
	__disable_irq();
	while (1)
	{
		LED_RED_TOGGLE;
		HAL_Delay( 200 );
		NVIC_SystemReset();
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
