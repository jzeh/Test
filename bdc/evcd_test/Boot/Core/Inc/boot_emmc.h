/**
  ******************************************************************************
  * @file    boot_emmc.h
  * @brief   Boot 컨텍스트 eMMC + FatFs 마운트 + FirmwareInfo 로드
  *
  *  frp-scan 의 Boot/main.c 에 인라인되어 있던 함수들을 evcd_test 에서는
  *  별도 파일로 분리 (main.c 비대 회피).
  *
  *  의존성:
  *    - HAL_MMC, HAL_GPIO, HAL_RCCEx (SDMMC1 클럭/핀)
  *    - FatFs (Boot RO 구성, Boot/FATFS/Target/ffconf.h + boot_diskio.c)
  *    - firmware_lite.h (SFwInfo, VCI3_FWINFO_PREAMBLE)
  ******************************************************************************
  */
#ifndef __BOOT_EMMC_H__
#define __BOOT_EMMC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "firmware_lite.h"

/* SDMMC1 + eMMC 핸들 (boot_emmc.c 에서 정의) */
extern MMC_HandleTypeDef hmmc1;

/**
  * @brief  SDMMC1 핸들 초기화 + HAL_MMC_Init.
  *         Boot_InitEmmc() 내부에서 호출됨 — 외부 직접 호출 불필요.
  */
void MX_SDMMC1_MMC_Init(void);

/**
  * @brief  Lazy eMMC 초기화 — eMMC RST 토글, HAL_MMC_Init, f_mount.
  *         idempotent: 두 번째 호출부터는 즉시 HAL_OK 반환.
  *
  * @retval HAL_OK     마운트 성공 (또는 이미 마운트됨)
  * @retval HAL_ERROR  HW 초기화 또는 f_mount 실패
  */
HAL_StatusTypeDef Boot_InitEmmc(void);

/**
  * @brief  eMMC 의 FirmwareInfo.ini 를 읽어 SFwInfo 로드 + 프리앰블 검증.
  *         idempotent: 두 번째 호출부터는 캐시된 결과 반환.
  *
  * @retval HAL_OK     파일 읽기 성공, preamble 일치
  * @retval HAL_ERROR  파일 없음, I/O 실패, 또는 preamble 불일치
  */
HAL_StatusTypeDef Boot_LoadFwInfo(void);

/**
  * @brief  마지막 Boot_LoadFwInfo() 성공 후 캐시된 SFwInfo 반환.
  * @retval const SFwInfo*  로드 성공 시 유효 포인터, 실패 시 NULL
  */
const SFwInfo* Boot_GetFwInfo(void);

/**
  * @brief  eMMC <folder>/<app_no.filename> → NOR Slot 1 staging → Slot 0 복사.
  *
  *         단일 호출로 OTA 적용 완료:
  *           (1) FwInfo 의 msAppInfo[app_no-1].marrcFilename 으로 파일 경로 결정
  *           (2) eMMC 파일을 4KB 청크로 읽으며 Slot 1 erase/program/verify
  *           (3) CRC32 계산 + slot_mark_staging_valid (sha 는 0으로 채움)
  *           (4) slot_set_copy_pending → slot_copy_staging_to_exec
  *                → 내부에서 fw_flag = BOOT_TEST + fail_count = 0 설정
  *
  *         성공 시 다음 BOOT_Application() 호출이 Slot 0(=새 펌웨어)로 점프.
  *
  * @param  folder  "01_Application" (정상/Stage 1/2) 또는 "02_Backup" (Stage 2.5)
  * @param  app_no  AppName enum (eApp_Main / eApp_Recovery)
  * @retval HAL_OK     staging + copy 성공 (BOOT_TEST 진입)
  * @retval HAL_ERROR  파일 없음, CRC/verify 실패 등
  */
HAL_StatusTypeDef Boot_LoadAndStageApp(const char *folder, uint8_t app_no);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_EMMC_H__ */
