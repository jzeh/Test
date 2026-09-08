/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "extmem_manager.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fw_update.h"        /* fw_flag, BKPSRAM, slot APIs */
#include "boot_emmc.h"        /* Boot_InitEmmc, Boot_LoadFwInfo */
#include "stm32_boot_xip.h"   /* BOOT_Application */
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

UART_HandleTypeDef huart7;

XSPI_HandleTypeDef hxspi1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_SBS_Init(void);
static void MX_XSPI1_Init(void);
static void MX_UART7_Init(void);
/* USER CODE BEGIN PFP */

// EWARM v9 new __write function
int iar_fputc(int ch);
__ATTRIBUTES size_t __write(int, const unsigned char *, size_t);
#define PUTCHAR_PROTOTYPE int iar_fputc(int ch)
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#ifdef FW_UPDATE_ENABLED
/**
  * @brief  부트 fw_flag 분기 처리 — fw-update Step 4 골격.
  *
  *  ⚠ FW_UPDATE_ENABLED 매크로로 gate 처리.
  *    이 함수가 호출되면 slot_manager_init이 메타 영역(0x9000_0000 + 0x07FF_0000)에
  *    자동 default 메타를 작성할 수 있음 → ICF 시프트 안 된 상태에서 기존 Appli
  *    .bin (0x9000_0000~)을 덮어씀!
  *    따라서 Step 7에서 ICF 시프트 + EXTMEM_XIP_IMAGE_OFFSET 매크로와 함께 활성화.
  *
  *  현 단계는 minimal 분기만 처리:
  *    - XSPI + Slot Manager 초기화 (메타 로드)
  *    - fw_flag 읽기
  *    - COPY_PENDING / COPYING → Slot 1 → Slot 0 복사 (slot_copy_staging_to_exec)
  *    - 그 외 → 통과 (정상 부팅)
  *
  *  Step 5에서 추가: eMMC RO 마운트, Boot_LoadAndStageApp
  *  Step 8에서 추가: BOOT_TEST/fail_count 복구 cascade, Slot 0 CRC verify
  *
  *  실패해도 BOOT_Application으로 계속 진행 — Slot 0가 유효하면 부팅 가능.
  */
static void Boot_FwUpdateCheck(void)
{
    CHK_SET_BOOT(CHK_BOOT_FWCHECK_ENTER);
    printf("[BOOT] fwcheck: enter\r\n");

    /* Step 1: XSPI + Slot Manager 초기화 */
    printf("[BOOT] fwcheck: xspi_flash_init()...\r\n");
    if (xspi_flash_init() != HAL_OK) {
        printf("[BOOT] fwcheck: xspi_flash_init FAIL\r\n");
        CHK_SET_BOOT(CHK_BOOT_FWCHECK_EXIT);
        return;
    }
    printf("[BOOT] fwcheck: slot_manager_init()...\r\n");
    if (slot_manager_init() != HAL_OK) {
        printf("[BOOT] fwcheck: slot_manager_init FAIL\r\n");
        CHK_SET_BOOT(CHK_BOOT_FWCHECK_EXIT);
        return;
    }
    printf("[BOOT] fwcheck: init done (fw_flag branch next)\r\n");

    /* Step 2: fw_flag 분기 */
    uint8_t fw_flag = fw_get_flag();

    switch (fw_flag)
    {
    case FW_FLAG_IDLE:
    case FW_FLAG_VERIFIED:
        /* 정상 부팅 — 분기 없음 */
        break;

    case FW_FLAG_STAGING:
        /* Appli가 Slot 1에 쓰는 도중 리셋 — Slot 0 무손상.
         * Slot 1만 invalidate 후 정상 부팅. */
        fw_set_flag(FW_FLAG_IDLE);
        slot_invalidate_staging();
        break;

    case FW_FLAG_COPY_PENDING:
    case FW_FLAG_COPYING:
    {
        /* Plan §5 architecture: Appli 가 fw_flag 만 set 하고 reset →
         * Boot 이 eMMC → Slot 1 → Slot 0 전체 staging 을 수행 (Step 7).
         * 멱등성: COPY_PENDING / COPYING 모두 동일 처리 (재실행 안전). */

        /* 부팅 모드 결정 — FwInfo 의 mucBootMode 우선, 실패 시 SAFE_MODE_APP */
        uint8_t app_no = (uint8_t)SAFE_MODE_APP;
        if (Boot_LoadFwInfo() == HAL_OK) {
            const SFwInfo *fi = Boot_GetFwInfo();
            if (fi->mucBootMode > 0U && fi->mucBootMode < (uint8_t)eApp_MAX) {
                app_no = fi->mucBootMode;
            }
        }

        if (Boot_LoadAndStageApp("01_Application", app_no) == HAL_OK) {
            /* slot_copy_staging_to_exec 내부에서 fw_flag=BOOT_TEST + fail_count=0 설정됨 */
            CHK_SET_BOOT(CHK_BOOT_FWCHECK_EXIT);
            return;
        }

        /* staging 실패 → IDLE fallback, 기존 Slot 0(있다면)로 부팅 시도.
         * Step 8 에서 fail_count cascade 가 Stage 2.5(rollback) 처리. */
        fw_set_flag(FW_FLAG_IDLE);
        slot_clear_copy_pending();
        break;
    }

    case FW_FLAG_BOOT_TEST:
    {
        /* 새 펌웨어 첫 부팅. fail_count 증가 — Appli 가 IDLE 도달 시
         * sys-main.c 에서 VERIFIED 전이 + fail_count = 0 으로 초기화.
         *
         * 만약 새 펌웨어가 hang/crash → IWDG 리셋 → fail_count 누적 →
         * RECOVERY_MAX_FAIL_COUNT(3) 도달 시 Step 8 cascade 발동 예정.
         * Step 7 단독 상태에서는 cascade 미구현이므로 fallback 으로 IDLE 복귀. */
        uint32_t fc = fw_get_fail_count() + 1U;
        fw_set_fail_count(fc);
        printf("[BOOT] BOOT_TEST fail_count=%lu/%u\r\n",
               (unsigned long)fc, (unsigned)RECOVERY_MAX_FAIL_COUNT);

        if (fc >= RECOVERY_MAX_FAIL_COUNT) {
            /* Step 8 에서 Stage 1/2/2.5/3 cascade 로 치환됨.
             * 현재는 IDLE 강제 복귀 — 새 펌웨어로 계속 시도하되 무한 루프 방지. */
            fw_set_flag(FW_FLAG_IDLE);
            fw_set_fail_count(0);
            printf("[BOOT] fail_count exceeded — fallback to IDLE (cascade in Step 8)\r\n");
        }
        /* 그대로 BOOT_Application 호출 흐름으로 계속 */
        break;
    }

    default:
        /* 알 수 없는 값 — 안전 디폴트로 IDLE 처리 */
        fw_set_flag(FW_FLAG_IDLE);
        break;
    }

    CHK_SET_BOOT(CHK_BOOT_FWCHECK_EXIT);
}
#endif /* FW_UPDATE_ENABLED */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

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

  /* USER CODE BEGIN SysInit */
  BackupSRAM_Init();             /* Boot ↔ Appli 신호 채널 (fw_flag 등) 활성화 */
  CHK_SET_BOOT(CHK_BOOT_CLK_OK);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SBS_Init();
  MX_UART7_Init();
  HAL_Delay(20);
  MX_XSPI1_Init();
  MX_EXTMEM_MANAGER_Init();
  CHK_SET_BOOT(CHK_BOOT_EXTMEM_OK);
  /* USER CODE BEGIN 2 */
  printf("BOOTLOADER - START \r\n");
  printf("fw_flag=0x%02X  fail_count=%lu  recovery_phase=%u\r\n",
         fw_get_flag(),
         (unsigned long)fw_get_fail_count(),
         (unsigned)fw_get_recovery_phase());

#ifdef BOOT_EMMC_TEST
  /* Step 5 검증용 — Boot이 eMMC를 RO로 마운트하고 FirmwareInfo.ini 읽기.
   * NOR Flash는 건드리지 않음 (비파괴적). 빌드 옵션에 BOOT_EMMC_TEST=1 정의로 활성화.
   * 검증 후 매크로 제거하고 Step 7에서 정식 통합. */
  if (Boot_LoadFwInfo() == HAL_OK) {
      const SFwInfo *fi = Boot_GetFwInfo();
      printf("[BOOT-TEST] FwInfo loaded: preamble=0x%08lX boot_mode=%u\r\n",
             (unsigned long)fi->muiPreamble, (unsigned)fi->mucBootMode);
  } else {
      printf("[BOOT-TEST] FwInfo load FAILED (eMMC mount or file missing)\r\n");
  }
#endif

#ifdef FW_UPDATE_ENABLED
  printf("[BOOT] FW_UPDATE_ENABLED=ON -> calling Boot_FwUpdateCheck()\r\n");
  /* fw_flag 분기 + slot copy. Step 7에서 ICF 시프트 + 매크로 정의로 활성화.
   * 미정의 상태에서는 메타 자동 생성이 기존 Appli 영역(0x9000_0000~)을 덮어쓸 위험. */
  Boot_FwUpdateCheck();

  /* === DIAGNOSTIC (임시) — slot0 벡터테이블 + 점프 offset 확인 ===
   * XSPI는 아직 indirect 모드이므로 xspi_flash_read 로 0x10000(EXEC_SLOT_OFFSET) 8바이트를 읽음.
   * 기대: MSP=0x2407xxxx/0x2007xxxx 류, PC=홀수 0x9001xxxx대, xip_off=0x10000.
   *   - MSP/PC가 0xFFFFFFFF → 앱이 0x90010000에 없음(플래시/주소 문제)
   *   - xip_off=0x0        → EXTMEM_XIP_IMAGE_OFFSET 정의가 빌드에 미반영
   * 원인 확정 후 이 블록 제거. */
#ifndef EXTMEM_XIP_IMAGE_OFFSET
#define EXTMEM_XIP_IMAGE_OFFSET 0
#endif
  {
    uint8_t vec[8];
    if (xspi_flash_read(EXEC_SLOT_OFFSET, vec, 8) == HAL_OK) {
      uint32_t msp = (uint32_t)vec[0] | ((uint32_t)vec[1] << 8)
                   | ((uint32_t)vec[2] << 16) | ((uint32_t)vec[3] << 24);
      uint32_t pc  = (uint32_t)vec[4] | ((uint32_t)vec[5] << 8)
                   | ((uint32_t)vec[6] << 16) | ((uint32_t)vec[7] << 24);
      printf("[BOOT] slot0@0x90010000 MSP=0x%08lX PC=0x%08lX  xip_off=0x%X\r\n",
             (unsigned long)msp, (unsigned long)pc, (unsigned)EXTMEM_XIP_IMAGE_OFFSET);
    } else {
      printf("[BOOT] slot0 vector read FAIL\r\n");
    }
  }
#endif
  /* USER CODE END 2 */

  /* Launch the application */
  CHK_SET_BOOT(CHK_BOOT_APP_ENTER);
  if (BOOT_OK != BOOT_Application())
  {
    Error_Handler();
  }
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL1.PLLM = 2;
  RCC_OscInitStruct.PLL1.PLLN = 50;
  RCC_OscInitStruct.PLL1.PLLP = 1;
  RCC_OscInitStruct.PLL1.PLLQ = 6;
  RCC_OscInitStruct.PLL1.PLLR = 2;
  RCC_OscInitStruct.PLL1.PLLS = 2;
  RCC_OscInitStruct.PLL1.PLLT = 2;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL2.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL2.PLLM = 2;
  RCC_OscInitStruct.PLL2.PLLN = 40;
  RCC_OscInitStruct.PLL2.PLLP = 6;
  RCC_OscInitStruct.PLL2.PLLQ = 2;
  RCC_OscInitStruct.PLL2.PLLR = 2;
  RCC_OscInitStruct.PLL2.PLLS = 8;
  RCC_OscInitStruct.PLL2.PLLT = 2;
  RCC_OscInitStruct.PLL2.PLLFractional = 0;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK4|RCC_CLOCKTYPE_PCLK5;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SBS Initialization Function
  * @param None
  * @retval None
  */
static void MX_SBS_Init(void)
{

  /* USER CODE BEGIN SBS_Init 0 */

  /* USER CODE END SBS_Init 0 */

  /* USER CODE BEGIN SBS_Init 1 */

  /* USER CODE END SBS_Init 1 */
  /* USER CODE BEGIN SBS_Init 2 */

  /* USER CODE END SBS_Init 2 */

}

/**
  * @brief UART7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART7_Init(void)
{

  /* USER CODE BEGIN UART7_Init 0 */

  /* USER CODE END UART7_Init 0 */

  /* USER CODE BEGIN UART7_Init 1 */

  /* USER CODE END UART7_Init 1 */
  huart7.Instance = UART7;
  huart7.Init.BaudRate = 115200;
  huart7.Init.WordLength = UART_WORDLENGTH_8B;
  huart7.Init.StopBits = UART_STOPBITS_1;
  huart7.Init.Parity = UART_PARITY_NONE;
  huart7.Init.Mode = UART_MODE_TX_RX;
  huart7.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart7.Init.OverSampling = UART_OVERSAMPLING_16;
  huart7.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart7.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart7.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart7) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart7, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart7, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart7) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART7_Init 2 */

  /* USER CODE END UART7_Init 2 */

}

/**
  * @brief XSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_XSPI1_Init(void)
{

  /* USER CODE BEGIN XSPI1_Init 0 */

  /* USER CODE END XSPI1_Init 0 */

  XSPIM_CfgTypeDef sXspiManagerCfg = {0};

  /* USER CODE BEGIN XSPI1_Init 1 */

  /* USER CODE END XSPI1_Init 1 */
  /* XSPI1 parameter configuration*/
  hxspi1.Instance = XSPI1;
  hxspi1.Init.FifoThresholdByte = 4;
  hxspi1.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
  hxspi1.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
  hxspi1.Init.MemorySize = HAL_XSPI_SIZE_32GB;
  hxspi1.Init.ChipSelectHighTimeCycle = 2;
  hxspi1.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
  hxspi1.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
  hxspi1.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
  hxspi1.Init.ClockPrescaler = 3;
  hxspi1.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
  hxspi1.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
  hxspi1.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_NONE;
  hxspi1.Init.MaxTran = 0;
  hxspi1.Init.Refresh = 0;
  hxspi1.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
  if (HAL_XSPI_Init(&hxspi1) != HAL_OK)
  {
    Error_Handler();
  }
  sXspiManagerCfg.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
  sXspiManagerCfg.IOPort = HAL_XSPIM_IOPORT_1;
  if (HAL_XSPIM_Config(&hxspi1, &sXspiManagerCfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN XSPI1_Init 2 */

  /* USER CODE END XSPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOP_CLK_ENABLE();
  __HAL_RCC_GPIOO_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(XSPIM_P1_NRESET_GPIO_Port, XSPIM_P1_NRESET_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : XSPIM_P1_NRESET_Pin */
  GPIO_InitStruct.Pin = XSPIM_P1_NRESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(XSPIM_P1_NRESET_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// EWARM v9 new __write function (__ICCARM__)

PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart7, (uint8_t*)&ch, 1, 0xFFFF);
  return ch;
}

size_t __write(int file, unsigned char const *ptr, size_t len)
{
  size_t idx;
  unsigned char const *pdata = ptr;

  for (idx = 0; idx < len; idx++)
  {
    iar_fputc((int)*pdata);
    pdata++;
  }
  return len;
}
/* USER CODE END 4 */

 /* MPU Configuration */

static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /* Disables all MPU regions */
  for(uint8_t i=0; i<__MPU_REGIONCOUNT; i++)
  {
    HAL_MPU_DisableRegion(i);
  }

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

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
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
