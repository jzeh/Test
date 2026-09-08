/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "../../Function/Inc/sys-common.h"
#include "../../Function/Inc/task-can.h"
#include <string.h>
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
/* USER CODE BEGIN Variables */
/* CAN queues are defined in main.c (where xQueueCreate is called) */
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Hook prototypes */
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, char *pcTaskName);

/* USER CODE BEGIN 2 */
void vApplicationIdleHook( void )
{
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
}
/* USER CODE END 2 */

/* USER CODE BEGIN 4 */
// void vApplicationStackOverflowHook(xTaskHandle xTask, char *pcTaskName)
// {
//    /* Run time stack overflow checking is performed if
//    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
//    called if a stack overflow is detected. */
// }
/* USER CODE END 4 */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* Debug: 마지막으로 실행 중이던 태스크 이름 (prvTaskExitError 디버깅용) */
volatile const char *g_dbg_last_task = "none";

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;
  /* printf 대신 HAL_UART_Transmit 직접 사용 (printf 자체가 손상 원인일 수 있음) */
  const char msg1[] = "\r\n!!! STACK OVERFLOW: ";
  const char msg2[] = " !!!\r\n";
  HAL_UART_Transmit(&huart7, (uint8_t*)msg1, sizeof(msg1)-1, 100);
  HAL_UART_Transmit(&huart7, (uint8_t*)pcTaskName, strlen(pcTaskName), 100);
  HAL_UART_Transmit(&huart7, (uint8_t*)msg2, sizeof(msg2)-1, 100);
  taskDISABLE_INTERRUPTS();
  for(;;);
}

/* USER CODE END Application */

