/**
  ******************************************************************************
  * @file    fw_backup_sram.c
  * @brief   Backup SRAM 초기화 (Boot + Appli 공유)
  *
  *  STM32H7S3I8 의 내부 Backup SRAM (0x38800000, 4KB) 접근 활성화.
  *  Boot/Appli main() 초기화 흐름에서 첫 번째 단계로 호출.
  *
  *  특성:
  *    - 메인 전원 ON 동안 데이터 유지 (소프트 리셋 너머도 OK)
  *    - VBAT 핀에 백업 전원이 있으면 메인 전원 OFF 후에도 유지
  *    - evcd_test 보드는 VBAT 백업 없음(BAT1 미장착) → 메인 전원 동안만 유지
  *      → OTA에는 충분 (HAL_NVIC_SystemReset 너머만 보존되면 됨)
  *
  *  Function 선언은 fw_update.h §11.
  ******************************************************************************
  */

#include "fw_update.h"

void BackupSRAM_Init(void)
{
    /* 1. Backup 도메인 접근 권한 부여 (DBP bit in PWR_CR1) */
    HAL_PWR_EnableBkUpAccess();

    /* 2. BKPSRAM 클럭 활성화 */
    __HAL_RCC_BKPRAM_CLK_ENABLE();

    /* 3. Backup regulator 활성화 (VBAT 백업 시에만 의미, 없어도 무해)
     *    이 호출 후 BRR(Backup Regulator Ready) flag set 까지 대기.
     *    HAL_PWREx_EnableBkUpReg 내부에서 polling 처리됨. */
    HAL_PWREx_EnableBkUpReg();
}
