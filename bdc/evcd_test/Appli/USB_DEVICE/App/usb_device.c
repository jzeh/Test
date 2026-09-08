/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           usb_device.c
  * @author         MCD Application Team
  * @brief          This file implements the USB Device
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

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"

/* USER CODE BEGIN Includes */
#include "../../Function/Inc/sys-emmc.h"   /* EMMC_PostCmd / EMMC_CMD_MOUNT / EMMC_CMD_UNMOUNT */
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* USB Device Core handle declaration. */
USBD_HandleTypeDef hUsbDeviceFS;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */
/**
  * @brief  USB 연결 상태 폴링 — dev_state 전이(edge)에서만 eMMC mount/unmount 트리거.
  *
  *  버스 인터럽트(Reset/Suspend) 대신 디바이스 상태를 직접 보고 판단하므로
  *  VBUS 핀이 없어도 안정적이고, suspend/resume thrashing 이 없음.
  *
  *    dev_state == USBD_STATE_CONFIGURED  → 호스트가 드라이브로 정상 마운트 = 연결
  *      → EMMC_CMD_UNMOUNT (eMMC 소유권을 PC 에 양도)
  *    그 외(DEFAULT / SUSPENDED / 미설정) → 미연결
  *      → EMMC_CMD_MOUNT   (MCU 가 소유권 회수, FatFs 재마운트)
  *
  *  메인 루프 등에서 수십 ms 주기로 호출.
  */
void USB_MSC_PollConnection(void)
{
  static uint8_t s_connected_prev = 0xFF;   /* 0xFF: 최초 1회 강제 동기화 */

  uint8_t connected = (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) ? 1U : 0U;

  if (connected != s_connected_prev)
  {
    s_connected_prev = connected;
    (void)EMMC_PostCmd(connected ? EMMC_CMD_UNMOUNT : EMMC_CMD_MOUNT);
  }
}
/* USER CODE END 1 */

/**
  * Init USB device Library, add supported class and start the library
  * @retval None
  */
void MX_USB_DEVICE_Init(void)
{
  /* USER CODE BEGIN USB_DEVICE_Init_PreTreatment */

  /* USER CODE END USB_DEVICE_Init_PreTreatment */

  /* Init Device Library, add supported class and start the library. */
  if (USBD_Init(&hUsbDeviceFS, &MSC_Desc, DEVICE_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_MSC) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN USB_DEVICE_Init_PostTreatment */

  /* USER CODE END USB_DEVICE_Init_PostTreatment */
}

/**
  * @}
  */

/**
  * @}
  */

