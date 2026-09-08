/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32h7rsxx_it.c
  * @brief   Interrupt Service Routines.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32h7rsxx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile uint32_t g_fault_cfsr = 0;
volatile uint32_t g_fault_hfsr = 0;
volatile uint32_t g_fault_bfar = 0;
volatile uint32_t g_fault_mmfar = 0;
volatile uint32_t g_fault_shcsr = 0;

/* Exception frame captured from HardFault */
volatile uint32_t g_fault_r0  = 0;
volatile uint32_t g_fault_r1  = 0;
volatile uint32_t g_fault_r2  = 0;
volatile uint32_t g_fault_r3  = 0;
volatile uint32_t g_fault_r12 = 0;
volatile uint32_t g_fault_lr  = 0;   /* Link Register (caller of faulting function) */
volatile uint32_t g_fault_pc  = 0;   /* Program Counter (faulting instruction) */
volatile uint32_t g_fault_psr = 0;   /* xPSR */
volatile uint32_t g_fault_sp  = 0;   /* Stack pointer at fault time */
volatile const char *g_fault_task_name = "unknown";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart7;
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern ADC_HandleTypeDef hadc1;
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern MMC_HandleTypeDef hmmc1;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart8;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim1;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  g_fault_cfsr = SCB->CFSR;
  g_fault_hfsr = SCB->HFSR;
  g_fault_bfar = SCB->BFAR;
  g_fault_mmfar = SCB->MMFAR;
  g_fault_shcsr = SCB->SHCSR;

  /*
   * Extract exception stack frame.
   * EXC_RETURN (LR) bit 2: 0 = MSP used, 1 = PSP used.
   * In FreeRTOS, tasks use PSP; ISRs use MSP.
   */
  uint32_t *sp;
  __asm volatile ("TST LR, #4 \n"
                  "ITE EQ     \n"
                  "MRSEQ %0, MSP \n"
                  "MRSNE %0, PSP \n"
                  : "=r" (sp));

  g_fault_sp  = (uint32_t)sp;
  g_fault_r0  = sp[0];
  g_fault_r1  = sp[1];
  g_fault_r2  = sp[2];
  g_fault_r3  = sp[3];
  g_fault_r12 = sp[4];
  g_fault_lr  = sp[5];   /* Return address of caller */
  g_fault_pc  = sp[6];   /* << Faulting instruction address */
  g_fault_psr = sp[7];

  /* Capture running task name (FreeRTOS) */
  TaskHandle_t cur = xTaskGetCurrentTaskHandle();
  if (cur != NULL) {
    g_fault_task_name = pcTaskGetName(cur);
  }

  /* Print to debug UART (non-blocking, minimal) */
  char buf[128];
  int len = snprintf(buf, sizeof(buf),
    "\r\n*** HARD FAULT ***\r\n"
    "Task: %s\r\n"
    "PC:   0x%08lX\r\n"
    "LR:   0x%08lX\r\n"
    "CFSR: 0x%08lX\r\n",
    g_fault_task_name, g_fault_pc, g_fault_lr, g_fault_cfsr);
  HAL_UART_Transmit(&huart7, (uint8_t*)buf, len, 500);

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */
  g_fault_cfsr = SCB->CFSR;
  g_fault_hfsr = SCB->HFSR;
  g_fault_bfar = SCB->BFAR;
  g_fault_mmfar = SCB->MMFAR;
  g_fault_shcsr = SCB->SHCSR;

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */
  g_fault_cfsr = SCB->CFSR;
  g_fault_hfsr = SCB->HFSR;
  g_fault_bfar = SCB->BFAR;
  g_fault_mmfar = SCB->MMFAR;
  g_fault_shcsr = SCB->SHCSR;

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */
  g_fault_cfsr = SCB->CFSR;
  g_fault_hfsr = SCB->HFSR;
  g_fault_bfar = SCB->BFAR;
  g_fault_mmfar = SCB->MMFAR;
  g_fault_shcsr = SCB->SHCSR;

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32H7RSxx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7rsxx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles ADC1 and ADC2 global interrupts.
  */
void ADC1_2_IRQHandler(void)
{
  /* USER CODE BEGIN ADC1_2_IRQn 0 */

  /* USER CODE END ADC1_2_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc1);
  /* USER CODE BEGIN ADC1_2_IRQn 1 */

  /* USER CODE END ADC1_2_IRQn 1 */
}

/**
  * @brief This function handles TIM1 update interrupt.
  */
void TIM1_UP_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_IRQn 0 */

  /* USER CODE END TIM1_UP_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_IRQn 1 */

  /* USER CODE END TIM1_UP_IRQn 1 */
}

/**
  * @brief This function handles I2C1 event interrupt.
  */
void I2C1_EV_IRQHandler(void)
{
  /* USER CODE BEGIN I2C1_EV_IRQn 0 */

  /* USER CODE END I2C1_EV_IRQn 0 */
  HAL_I2C_EV_IRQHandler(&hi2c1);
  /* USER CODE BEGIN I2C1_EV_IRQn 1 */

  /* USER CODE END I2C1_EV_IRQn 1 */
}

/**
  * @brief This function handles I2C2 event interrupt.
  */
void I2C2_EV_IRQHandler(void)
{
  /* USER CODE BEGIN I2C2_EV_IRQn 0 */

  /* USER CODE END I2C2_EV_IRQn 0 */
  HAL_I2C_EV_IRQHandler(&hi2c2);
  /* USER CODE BEGIN I2C2_EV_IRQn 1 */

  /* USER CODE END I2C2_EV_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */

  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

/**
  * @brief This function handles UART4 global interrupt.
  */
void UART4_IRQHandler(void)
{
  /* USER CODE BEGIN UART4_IRQn 0 */

  /* USER CODE END UART4_IRQn 0 */
  HAL_UART_IRQHandler(&huart4);
  /* USER CODE BEGIN UART4_IRQn 1 */

  /* USER CODE END UART4_IRQn 1 */
}

/**
  * @brief This function handles UART7 global interrupt.
  */
void UART7_IRQHandler(void)
{
  /* USER CODE BEGIN UART7_IRQn 0 */

  /* USER CODE END UART7_IRQn 0 */
  HAL_UART_IRQHandler(&huart7);
  /* USER CODE BEGIN UART7_IRQn 1 */

  /* USER CODE END UART7_IRQn 1 */
}

/**
  * @brief This function handles UART8 global interrupt.
  */
void UART8_IRQHandler(void)
{
  /* USER CODE BEGIN UART8_IRQn 0 */

  /* USER CODE END UART8_IRQn 0 */
  HAL_UART_IRQHandler(&huart8);
  /* USER CODE BEGIN UART8_IRQn 1 */

  /* USER CODE END UART8_IRQn 1 */
}

/**
  * @brief This function handles SDMMC1 global interrupt.
  */
void SDMMC1_IRQHandler(void)
{
  /* USER CODE BEGIN SDMMC1_IRQn 0 */

  /* USER CODE END SDMMC1_IRQn 0 */
  HAL_MMC_IRQHandler(&hmmc1);
  /* USER CODE BEGIN SDMMC1_IRQn 1 */

  /* USER CODE END SDMMC1_IRQn 1 */
}

/**
  * @brief This function handles USB OTG FS interrupt.
  */
void OTG_FS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_FS_IRQn 0 */

  /* USER CODE END OTG_FS_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
  /* USER CODE BEGIN OTG_FS_IRQn 1 */

  /* USER CODE END OTG_FS_IRQn 1 */
}

/**
  * @brief This function handles FDCAN1 interrupt 0.
  */
void FDCAN1_IT0_IRQHandler(void)
{
  /* USER CODE BEGIN FDCAN1_IT0_IRQn 0 */

  /* USER CODE END FDCAN1_IT0_IRQn 0 */
  HAL_FDCAN_IRQHandler(&hfdcan1);
  /* USER CODE BEGIN FDCAN1_IT0_IRQn 1 */

  /* USER CODE END FDCAN1_IT0_IRQn 1 */
}

/**
  * @brief This function handles FDCAN2 interrupt0.
  */
void FDCAN2_IT0_IRQHandler(void)
{
  /* USER CODE BEGIN FDCAN2_IT0_IRQn 0 */

  /* USER CODE END FDCAN2_IT0_IRQn 0 */
  HAL_FDCAN_IRQHandler(&hfdcan2);
  /* USER CODE BEGIN FDCAN2_IT0_IRQn 1 */

  /* USER CODE END FDCAN2_IT0_IRQn 1 */
}

/**
  * @brief This function handles FDCAN1 interrupt 1.
  */
void FDCAN1_IT1_IRQHandler(void)
{
  /* USER CODE BEGIN FDCAN1_IT1_IRQn 0 */

  /* USER CODE END FDCAN1_IT1_IRQn 0 */
  HAL_FDCAN_IRQHandler(&hfdcan1);
  /* USER CODE BEGIN FDCAN1_IT1_IRQn 1 */

  /* USER CODE END FDCAN1_IT1_IRQn 1 */
}

/**
  * @brief This function handles FDCAN2 interrupt 1.
  */
void FDCAN2_IT1_IRQHandler(void)
{
  /* USER CODE BEGIN FDCAN2_IT1_IRQn 0 */

  /* USER CODE END FDCAN2_IT1_IRQn 0 */
  HAL_FDCAN_IRQHandler(&hfdcan2);
  /* USER CODE BEGIN FDCAN2_IT1_IRQn 1 */

  /* USER CODE END FDCAN2_IT1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
