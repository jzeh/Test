/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
uint8_t retUSER;    /* Return value for USER */
char USERPath[4];   /* USER logical drive path */
/* 32-byte align: D-Cache invalidate 시 cache line 단독 점유 강제.
 * 정렬 안 하면 인접 변수(IDLE TCB/stack 등)가 같은 line 에 들어가 무효화 시 손실 */
FATFS USERFatFS __attribute__((aligned(32)));
FIL   USERFile  __attribute__((aligned(32)));
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

void MX_FATFS_Init(void)
{
  /* USER CODE BEGIN Init */
  retUSER = FATFS_LinkDriver(&USER_Driver, USERPath);

  /* USER CODE END Init */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */
