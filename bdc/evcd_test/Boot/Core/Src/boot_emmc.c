/**
  ******************************************************************************
  * @file    boot_emmc.c
  * @brief   Boot 컨텍스트 eMMC + FatFs 마운트 (Read-Only)
  *
  *  포팅 출처:
  *    - SDMMC1 / HAL_MMC_MspInit : Appli/Core/Src/stm32h7rsxx_hal_msp.c
  *    - Boot_InitEmmc/LoadFwInfo : frp-scan/Boot/Core/Src/main.c
  *
  *  Appli/sys-emmc.c 대비 단순화:
  *    - FreeRTOS 없음 → semaphore/queue 대신 polling
  *    - DMA 없음     → HAL_MMC_ReadBlocks 직접 (5초 timeout)
  *    - Read-Only   → f_write/f_unlink 코드 미컴파일 (ffconf.h)
  *
  *  evcd_test 핀 매핑 (Appli 동일):
  *    SDMMC1: PC6/7 D6/D7, PC8/9 D0/D1, PC10 D2, PC11 D3, PC12 CLK,
  *            PD2 CMD, PB8/9 D4/D5
  *    eMMC:   EMMC_PWR_EN (PD8), EMMC_RST_N (GPIOM PIN 2)
  ******************************************************************************
  */

#include "boot_emmc.h"
#include "ff.h"
#include "fw_compat.h"
#include "fw_update.h"
#include "fw_slot_manager.h"
#include "fw_crc32.h"
#include "fw_flash_xspi.h"
#include <string.h>
#include <stdio.h>

/*============================================================================
 *  Variables
 *===========================================================================*/

MMC_HandleTypeDef hmmc1;                 /* boot_diskio.c 가 extern으로 사용 */
extern XSPI_HandleTypeDef hxspi1;        /* Boot/Core/Src/main.c 정의 — staging 후 Abort용 */

static FATFS    s_boot_fatfs;            /* Boot 전용 FATFS 객체 */
static uint8_t  s_emmc_mounted   = 0;
static SFwInfo  s_fwinfo;
static uint8_t  s_fwinfo_loaded  = 0;

/*============================================================================
 *  HAL_MMC_MspInit — weak override
 *  Appli/stm32h7rsxx_hal_msp.c 와 동일한 핀 매핑.
 *  Boot 빌드에서는 이 파일이 link되고, Appli 빌드에서는 hal_msp.c 가 link됨.
 *===========================================================================*/

void HAL_MMC_MspInit(MMC_HandleTypeDef *hmmc)
{
    if (hmmc->Instance != SDMMC1) return;

    GPIO_InitTypeDef         GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit   = {0};

    /* SDMMC1 클럭: PLL2S */
    PeriphClkInit.PeriphClockSelection  = RCC_PERIPHCLK_SDMMC12;
    PeriphClkInit.Sdmmc12ClockSelection = RCC_SDMMC12CLKSOURCE_PLL2S;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }

    __HAL_RCC_SDMMC1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PC6/7/8/9/11/12 : D6/D7/D0/D1/D3/CLK (AF11) */
    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9
                              | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_SDMMC1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PC10 : D2 (AF12) */
    GPIO_InitStruct.Pin       = GPIO_PIN_10;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PD2 : CMD (AF11) */
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Alternate = GPIO_AF11_SDMMC1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* PB8/9 : D4/D5 (AF12) */
    GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* SDMMC1 IRQ는 polling 모드에서도 일부 path가 NVIC 의존하므로 등록만.
     * 우선순위는 낮게 (RTOS 없으므로 nesting 무관). */
    HAL_NVIC_SetPriority(SDMMC1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
}

/*============================================================================
 *  MX_SDMMC1_MMC_Init
 *===========================================================================*/

void MX_SDMMC1_MMC_Init(void)
{
    hmmc1.Instance                 = SDMMC1;
    hmmc1.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
    hmmc1.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    hmmc1.Init.BusWide             = SDMMC_BUS_WIDE_8B;
    hmmc1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    hmmc1.Init.ClockDiv            = 4;     /* 안전한 클럭 (~7.5MHz) */

    if (HAL_MMC_Init(&hmmc1) != HAL_OK) {
        Error_Handler();
    }
}

/*============================================================================
 *  Private: eMMC 전원/리셋 시퀀스
 *  EMMC_PWR_EN (PD8) HIGH → EMMC_RST_N (GPIOM2) LOW → 10ms → HIGH → 10ms
 *===========================================================================*/

static void Boot_EnableEmmcPower(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* GPIOD/GPIOM 클럭 활성 */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOM_CLK_ENABLE();

    /* EMMC_PWR_EN (PD8) — Output HIGH */
    gpio.Pin   = EMMC_PWR_EN_Pin;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(EMMC_PWR_EN_GPIO_Port, &gpio);
    HAL_GPIO_WritePin(EMMC_PWR_EN_GPIO_Port, EMMC_PWR_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(10);

    /* EMMC_RST_N (GPIOM2) — Output, 토글 LOW→HIGH */
    gpio.Pin = EMMC_RST_N_Pin;
    HAL_GPIO_Init(EMMC_RST_N_GPIO_Port, &gpio);
    HAL_GPIO_WritePin(EMMC_RST_N_GPIO_Port, EMMC_RST_N_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(EMMC_RST_N_GPIO_Port, EMMC_RST_N_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
}

/*============================================================================
 *  Boot_InitEmmc — Lazy init
 *===========================================================================*/

HAL_StatusTypeDef Boot_InitEmmc(void)
{
    if (s_emmc_mounted) {
        return HAL_OK;
    }

    Boot_EnableEmmcPower();
    MX_SDMMC1_MMC_Init();     /* 실패 시 내부 Error_Handler (hang) → 여기 도달 = HAL_MMC_Init OK */

    /* --- 진단: HAL 상태 + 카드 정보 --- */
    HAL_MMC_CardInfoTypeDef ci = {0};
    HAL_MMC_GetCardInfo(&hmmc1, &ci);
    printf("[BOOT] MMC state=%d Err=0x%08lX BlockNbr=%lu BlockSize=%lu\r\n",
           (int)hmmc1.State, (unsigned long)hmmc1.ErrorCode,
           (unsigned long)ci.LogBlockNbr, (unsigned long)ci.LogBlockSize);

    /* --- 진단: FATFS 거치지 않고 원시 sector 0 직접 read --- */
    {
        static uint8_t sec0[512] __attribute__((aligned(32)));
        HAL_StatusTypeDef rs = HAL_MMC_ReadBlocks(&hmmc1, sec0, 0U, 1U, 5000);
        while (HAL_MMC_GetCardState(&hmmc1) != HAL_MMC_CARD_TRANSFER) { /* spin */ }
        /* 블로킹 read = CPU FIFO(캐시 코히런트) — invalidate 금지 */
        printf("[BOOT] sec0 rs=%d head=%02X %02X %02X %02X  sig[510:511]=%02X %02X\r\n",
               (int)rs, sec0[0], sec0[1], sec0[2], sec0[3], sec0[510], sec0[511]);
    }

    FRESULT res = f_mount(&s_boot_fatfs, "", 1);
    if (res != FR_OK) {
        printf("[BOOT] f_mount FAILED res=%d (1=DISK_ERR 3=NOT_READY 13=NO_FILESYSTEM)\r\n",
               (int)res);
        return HAL_ERROR;
    }

    s_emmc_mounted = 1;
    printf("[BOOT] eMMC mounted (RO)\r\n");
    return HAL_OK;
}

/*============================================================================
 *  Boot_LoadFwInfo — FirmwareInfo.ini 바이너리 블롭 로드
 *===========================================================================*/

HAL_StatusTypeDef Boot_LoadFwInfo(void)
{
    if (s_fwinfo_loaded) {
        return HAL_OK;
    }

    if (Boot_InitEmmc() != HAL_OK) {
        return HAL_ERROR;
    }

    FIL  fp;
    UINT br;

    if (f_open(&fp, BOOT_FWINFO_FILE, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
        GLogE("[BOOT] " BOOT_FWINFO_FILE " not found\r\n");
        return HAL_ERROR;
    }

    if (f_read(&fp, &s_fwinfo, sizeof(SFwInfo), &br) != FR_OK
        || br != sizeof(SFwInfo)) {
        f_close(&fp);
        GLogE("[BOOT] FwInfo read size mismatch\r\n");
        return HAL_ERROR;
    }
    f_close(&fp);

    /* 프리앰블 검증 — 잘못된 파일 또는 손상된 경우 차단 */
    if (s_fwinfo.muiPreamble != VCI3_FWINFO_PREAMBLE) {
        GLogE("[BOOT] FwInfo preamble mismatch (0x%08X)\r\n",
              (unsigned)s_fwinfo.muiPreamble);
        return HAL_ERROR;
    }

    s_fwinfo_loaded = 1;
    GLogI("[BOOT] FwInfo loaded — boot_mode=%u\r\n",
          (unsigned)s_fwinfo.mucBootMode);
    return HAL_OK;
}

/*============================================================================
 *  Boot_GetFwInfo
 *===========================================================================*/

const SFwInfo* Boot_GetFwInfo(void)
{
    return s_fwinfo_loaded ? &s_fwinfo : NULL;
}

/*============================================================================
 *  Boot_LoadAndStageApp
 *    eMMC <folder>/<filename> → Slot 1 staging → Slot 0 복사.
 *
 *    단일앱 + Recovery 구성에서는 FwInfo.msAppInfo[app_no-1].marrcFilename 으로
 *    파일명 결정 (AppSwList.ini 파싱 생략 — Plan 단순화).
 *===========================================================================*/

/* 4KB chunk 버퍼 — Slot 1 program + verify 용. BSS 배치. */
static uint8_t s_stage_buf[XSPI_VERIFY_CHUNK_SIZE];

HAL_StatusTypeDef Boot_LoadAndStageApp(const char *folder, uint8_t app_no)
{
    if (folder == NULL) return HAL_ERROR;
    if (app_no == 0U || app_no >= eApp_MAX) return HAL_ERROR;

    /* (1) FwInfo 로드 + 파일명 결정 */
    if (Boot_LoadFwInfo() != HAL_OK) {
        GLogE("[STAGE] FwInfo load failed\r\n");
        return HAL_ERROR;
    }
    const SFwInfo *fi = Boot_GetFwInfo();

    char filename[40];
    strncpy(filename, fi->msAppInfo[app_no - 1U].marrcFilename, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';
    if (filename[0] == '\0') {
        GLogE("[STAGE] empty filename for app_no=%u\r\n", (unsigned)app_no);
        return HAL_ERROR;
    }

    char path[80];
    snprintf(path, sizeof(path), "%s/%s", folder, filename);

    /* (2) 파일 열기 + 크기 검증 */
    FIL  fp;
    UINT br;
    if (f_open(&fp, path, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
        GLogE("[STAGE] open failed: %s\r\n", path);
        return HAL_ERROR;
    }

    FILINFO finfo;
    if (f_stat(path, &finfo) != FR_OK) {
        f_close(&fp);
        return HAL_ERROR;
    }
    uint32_t img_size     = (uint32_t)finfo.fsize;
    uint32_t staging_addr = slot_get_staging_addr();    /* 0x04000000 (physical) */

    if (img_size == 0U || img_size > slot_get_max_image_size()) {
        f_close(&fp);
        GLogE("[STAGE] size out of range: %lu\r\n", (unsigned long)img_size);
        return HAL_ERROR;
    }

    /* (3) Slot 1 erase (4KB 정렬) */
    uint32_t erase_size = ((img_size + XSPI_SECTOR_SIZE - 1U) / XSPI_SECTOR_SIZE)
                          * XSPI_SECTOR_SIZE;
    if (xspi_flash_erase_sector(staging_addr, erase_size) != HAL_OK) {
        f_close(&fp);
        return HAL_ERROR;
    }

    /* (4) eMMC → Slot 1 청크 단위 program + verify + CRC32 누적 */
    uint32_t offset    = 0U;
    uint32_t crc_accum = fw_crc32_start();

    while (offset < img_size) {
        uint32_t chunk = img_size - offset;
        if (chunk > XSPI_VERIFY_CHUNK_SIZE) chunk = XSPI_VERIFY_CHUNK_SIZE;

        if (f_read(&fp, s_stage_buf, chunk, &br) != FR_OK || br != chunk) {
            f_close(&fp);
            GLogE("[STAGE] read fail at %lu\r\n", (unsigned long)offset);
            return HAL_ERROR;
        }

        if (xspi_flash_program(staging_addr + offset, s_stage_buf, chunk) != HAL_OK) {
            f_close(&fp);
            return HAL_ERROR;
        }
        if (xspi_flash_verify(staging_addr + offset, s_stage_buf, chunk) != HAL_OK) {
            f_close(&fp);
            GLogE("[STAGE] verify fail at %lu\r\n", (unsigned long)offset);
            return HAL_ERROR;
        }

        crc_accum = fw_crc32_update(crc_accum, s_stage_buf, chunk);

        offset += chunk;
        FW_IWDG_REFRESH();
    }
    f_close(&fp);

    /* Slot1 대량 program 직후 XSPI/EXTMEM 컨트롤러 상태 클리어.
     * 안 하면 직후 메타 erase(0x07FF0000 등)가 실패한다.
     * (slot_copy_staging_to_exec 가 copy 후 쓰는 것과 동일 패턴) */
    HAL_XSPI_Abort(&hxspi1);

    uint32_t final_crc = fw_crc32_finish(crc_accum);

    /* (5) 메타 갱신 — SHA 는 0으로 채움 (옵션 B) */
    uint8_t ver[3] = {
        fi->msAppInfo[app_no - 1U].marrucVersion[0],
        fi->msAppInfo[app_no - 1U].marrucVersion[1],
        fi->msAppInfo[app_no - 1U].marrucVersion[2]
    };
    uint8_t sha_zero[32] = {0};

    if (slot_mark_staging_valid(ver, img_size, final_crc, sha_zero, app_no) != HAL_OK) {
        GLogE("[STAGE] mark_staging_valid failed\r\n");
        return HAL_ERROR;
    }

    /* (6) copy_pending 설정 + Slot 1 → Slot 0 복사
     *     slot_copy_staging_to_exec 내부에서 fw_flag=BOOT_TEST + fail_count=0 설정. */
    if (slot_set_copy_pending() != HAL_OK)        return HAL_ERROR;
    if (slot_copy_staging_to_exec() != HAL_OK)    return HAL_ERROR;

    GLogI("[STAGE] OK — %s/%s (%lu B, CRC=0x%08lX)\r\n",
          folder, filename, (unsigned long)img_size, (unsigned long)final_crc);
    return HAL_OK;
}
