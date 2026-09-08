/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.h
  * @brief  Header for fatfs applications
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
#ifndef __FATFS_H
#define __FATFS_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ff.h"
#include "ff_gen_drv.h"
#include "user_diskio.h"
/* USER CODE END Includes */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/* Exported functions prototypes ---------------------------------------------*/
void MX_FATFS_Init(void);
/* USER CODE BEGIN Prototypes */

extern uint8_t retUSER; /* Return value for USER */
extern char USERPath[4]; /* USER logical drive path */
extern FATFS USERFatFS; /* File system object for USER logical drive */
extern FIL USERFile; /* File object for USER */
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__FATFS_H */
