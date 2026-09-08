/**
  ******************************************************************************
  * @file    fw_slot_manager.c
  * @brief   Slot Manager — Metadata management (Primary+Backup 이중화)
  *
  *  포팅 출처: Z:\11_Work\00_Git_Source\05_Others\frp-scan\Common\Src\fw_slot_manager.c
  *  주요 변경:
  *    - common.h / git_iwdg.h 제거 → fw_compat.h 사용
  *    - HAL_IWDG_Refresh(&hiwdg) → FW_IWDG_REFRESH()
  *    - HAL_XSPI_Abort(&hxspi1) → BOOT_BUILD 가드 (Appli link 보호)
  *    - slot_copy_staging_to_exec() 본체는 BOOT_BUILD 한정
  *      (Plan §5: Slot1→Slot0 복사는 Boot 전담, Appli는 호출 금지)
  *
  *  Metadata layout:
  *    Primary  @ SLOT_META_OFFSET_PRIMARY  (0x000000)
  *    Backup   @ SLOT_META_OFFSET_BACKUP   (0x7FF0000)
  *
  *  Write order: Backup first → Primary second
  *    → Power-cut during write: at least Backup remains valid
  *
  *  CRC scope: magic(4) + exec(45) + staging(45) + copy_pending(1) = 95 bytes
  ******************************************************************************
  */

#include "fw_slot_manager.h"
#include "fw_compat.h"
#include "fw_crc32.h"
#include "stm32h7rsxx_hal_xspi.h"

#include <string.h>

#ifdef BOOT_BUILD
/* hxspi1은 Boot/Core/Src/main.c:47에 정의 (Boot 빌드 전용).
 * Appli 빌드에서는 사용하지 않음 (Slot 복사는 Boot 전담). */
extern XSPI_HandleTypeDef hxspi1;
#endif

/*============================================================================
 *  Private state — RAM cached metadata
 *===========================================================================*/

static slot_metadata_t s_meta;
static uint8_t         s_meta_loaded = 0;

/* slot_copy_staging_to_exec / slot_verify_image_crc 공용 4KB 버퍼.
 * BSS에 둠 — 스택 4KB 회피. */
static uint8_t s_chunk_buf[XSPI_VERIFY_CHUNK_SIZE];

/*============================================================================
 *  Private helpers
 *===========================================================================*/

static uint32_t meta_calc_crc(const slot_metadata_t *meta)
{
    return fw_crc32((const uint8_t *)meta, SLOT_META_CRC_SIZE);
}

static HAL_StatusTypeDef meta_read(uint32_t flash_offset, slot_metadata_t *out)
{
    return xspi_flash_read(flash_offset, (uint8_t *)out, sizeof(slot_metadata_t));
}

static HAL_StatusTypeDef meta_write(uint32_t flash_offset, const slot_metadata_t *meta)
{
    if (xspi_flash_erase_sector(flash_offset, XSPI_SECTOR_SIZE) != HAL_OK) {
        return HAL_ERROR;
    }

    if (xspi_flash_program(flash_offset, (const uint8_t *)meta,
                           sizeof(slot_metadata_t)) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static uint8_t meta_is_valid(const slot_metadata_t *meta)
{
    if (meta->magic != SLOT_META_MAGIC) {
        return 0;
    }

    if (meta->metadata_crc32 != meta_calc_crc(meta)) {
        return 0;
    }

    return 1;
}

static void meta_create_default(slot_metadata_t *meta)
{
    memset(meta, 0, sizeof(slot_metadata_t));
    meta->magic          = SLOT_META_MAGIC;
    meta->exec.valid     = SLOT_INVALID_MARK;
    meta->staging.valid  = SLOT_INVALID_MARK;
    meta->copy_pending   = COPY_NONE_FLAG;
    meta->metadata_crc32 = meta_calc_crc(meta);
}

/* Write order: Backup first → Primary (power-cut safety) */
static HAL_StatusTypeDef meta_flush(void)
{
    s_meta.metadata_crc32 = meta_calc_crc(&s_meta);

    if (meta_write(SLOT_META_OFFSET_BACKUP, &s_meta) != HAL_OK) {
        GLogE("meta_flush: Backup write failed\r\n");
        return HAL_ERROR;
    }

    if (meta_write(SLOT_META_OFFSET_PRIMARY, &s_meta) != HAL_OK) {
        GLogE("meta_flush: Primary write failed\r\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}

/*============================================================================
 *  1. slot_manager_init
 *     Primary 우선 → 실패 시 Backup → 둘 다 실패 시 default 생성.
 *===========================================================================*/

HAL_StatusTypeDef slot_manager_init(void)
{
    /* static(BSS) — 4KB 스택 회피. Boot CSTACK가 작아(1KB) 지역변수로 두면 즉시 오버플로.
     * 같은 파일의 s_chunk_buf 와 동일한 의도. 부트는 단일 스레드라 static 안전. */
    static slot_metadata_t tmp;

    /* Primary 시도 */
    if (meta_read(SLOT_META_OFFSET_PRIMARY, &tmp) == HAL_OK && meta_is_valid(&tmp)) {
        memcpy(&s_meta, &tmp, sizeof(slot_metadata_t));
        s_meta_loaded = 1;
        GLogI("[SLOT] Primary OK (exec v%d.%d.%d, staging:%s)\r\n",
              s_meta.exec.version[0], s_meta.exec.version[1], s_meta.exec.version[2],
              s_meta.staging.valid == SLOT_VALID_MARK ? "valid" : "invalid");
        return HAL_OK;
    }

    GLogI("[SLOT] Primary invalid, trying Backup\r\n");

    /* Backup 시도 → Primary 복원 */
    if (meta_read(SLOT_META_OFFSET_BACKUP, &tmp) == HAL_OK && meta_is_valid(&tmp)) {
        memcpy(&s_meta, &tmp, sizeof(slot_metadata_t));
        s_meta_loaded = 1;

        if (meta_write(SLOT_META_OFFSET_PRIMARY, &s_meta) != HAL_OK) {
            GLogE("[SLOT] Primary restore failed\r\n");
        } else {
            GLogI("[SLOT] Primary restored from Backup\r\n");
        }
        return HAL_OK;
    }

    /* 둘 다 실패 → default 생성 */
    GLogI("[SLOT] Both invalid, creating defaults\r\n");
    meta_create_default(&s_meta);
    s_meta_loaded = 1;
    meta_flush();

    return HAL_OK;
}

/*============================================================================
 *  2~3. Slot 주소 getters (고정값)
 *===========================================================================*/

uint32_t slot_get_exec_addr(void)
{
    return EXEC_SLOT_XIP_ADDR;        /* 0x90010000 (XIP) */
}

uint32_t slot_get_staging_addr(void)
{
    return STAGING_SLOT_OFFSET;        /* 0x04000000 (physical, no XIP) */
}

/*============================================================================
 *  4~5. Slot info getters
 *===========================================================================*/

const slot_info_t *slot_get_exec_info(void)
{
    return s_meta_loaded ? &s_meta.exec : NULL;
}

const slot_info_t *slot_get_staging_info(void)
{
    return s_meta_loaded ? &s_meta.staging : NULL;
}

/*============================================================================
 *  6. slot_mark_staging_valid
 *     Staging program + verify 완료 후 호출. 메타에 ver/size/CRC/SHA 기록.
 *===========================================================================*/

HAL_StatusTypeDef slot_mark_staging_valid(uint8_t ver[3], uint32_t size,
                                          uint32_t crc, uint8_t sha[32],
                                          uint8_t app_no)
{
    if (!s_meta_loaded) {
        return HAL_ERROR;
    }

    s_meta.staging.version[0] = ver[0];
    s_meta.staging.version[1] = ver[1];
    s_meta.staging.version[2] = ver[2];
    s_meta.staging.image_size = size;
    s_meta.staging.crc32      = crc;
    memcpy(s_meta.staging.sha256, sha, 32);
    s_meta.staging.valid      = SLOT_VALID_MARK;
    s_meta.staging.app_no     = app_no;

    return meta_flush();
}

/*============================================================================
 *  7. slot_set_copy_pending
 *     Appli에서 reset 직전 호출. Boot이 다음 부팅 때 Slot1→Slot0 복사.
 *===========================================================================*/

HAL_StatusTypeDef slot_set_copy_pending(void)
{
    if (!s_meta_loaded) {
        return HAL_ERROR;
    }

    s_meta.copy_pending = COPY_PENDING_FLAG;

    /* BKPSRAM의 fw_flag도 같이 설정 — 메타 손상 시 fallback 신호 */
    fw_set_flag(FW_FLAG_COPY_PENDING);

    return meta_flush();
}

/*============================================================================
 *  8. slot_copy_staging_to_exec  [BOOTLOADER ONLY]
 *     Slot 1 → Slot 0 복사 + 검증 + 재시도.
 *
 *     1. fw_flag = COPYING
 *     2. Slot 0 erase (staging_size 분량)
 *     3. Copy + 4KB readback verify
 *     4. 전체 CRC32 verify (실패 시 최대 3회 재시도)
 *     5. exec 메타 = staging 메타 복사
 *     6. copy_pending 클리어
 *     7. Dual-write (Backup → Primary)
 *     8. fw_flag = BOOT_TEST, fail_count = 0
 *
 *     Appli 빌드에서는 즉시 HAL_ERROR 반환 (안전 가드).
 *===========================================================================*/

HAL_StatusTypeDef slot_copy_staging_to_exec(void)
{
#ifndef BOOT_BUILD
    /* Appli는 Slot1→Slot0 복사 안 함 — Boot 전담.
     * 실수로 호출되어도 메타/Slot 무손상. */
    return HAL_ERROR;
#else
    if (!s_meta_loaded) {
        return HAL_ERROR;
    }
    if (s_meta.staging.valid != SLOT_VALID_MARK) {
        return HAL_ERROR;
    }

    uint32_t img_size   = s_meta.staging.image_size;
    uint32_t erase_size = ((img_size + XSPI_SECTOR_SIZE - 1U) / XSPI_SECTOR_SIZE)
                           * XSPI_SECTOR_SIZE;

    fw_set_flag(FW_FLAG_COPYING);

    for (uint8_t retry = 0; retry < COPY_MAX_RETRY; retry++) {
        /* Step 2: Slot 0 erase */
        if (xspi_flash_erase_sector(EXEC_SLOT_OFFSET, erase_size) != HAL_OK) {
            GLogE("[SLOT] Exec erase failed (retry %d)\r\n", retry);
            continue;
        }

        /* Step 3: 4KB 청크 단위 copy + readback verify */
        uint32_t remaining = img_size;
        uint32_t offset    = 0U;
        uint8_t  copy_ok   = 1U;

        while (remaining > 0U) {
            /* 항상 4KB 청크 (마지막은 0xFF erased 영역 포함 — page-aligned 보장) */
            uint32_t chunk = XSPI_VERIFY_CHUNK_SIZE;

            if (xspi_flash_read(STAGING_SLOT_OFFSET + offset, s_chunk_buf, chunk) != HAL_OK) {
                copy_ok = 0; break;
            }
            if (xspi_flash_program(EXEC_SLOT_OFFSET + offset, s_chunk_buf, chunk) != HAL_OK) {
                copy_ok = 0; break;
            }
            if (xspi_flash_verify(EXEC_SLOT_OFFSET + offset, s_chunk_buf, chunk) != HAL_OK) {
                copy_ok = 0; break;
            }

            offset    += chunk;
            remaining  = (remaining > chunk) ? (remaining - chunk) : 0U;

            FW_IWDG_REFRESH();
        }

        if (!copy_ok) {
            GLogE("[SLOT] Copy failed at 0x%X (retry %d)\r\n", (unsigned)offset, retry);
            continue;
        }

        /* Step 4: 전체 CRC32 verify */
        uint32_t crc_accum = fw_crc32_start();
        remaining = img_size;
        offset    = 0U;
        uint8_t crc_ok = 1U;

        while (remaining > 0U) {
            uint32_t chunk = (remaining > XSPI_VERIFY_CHUNK_SIZE)
                                ? XSPI_VERIFY_CHUNK_SIZE : remaining;

            if (xspi_flash_read(EXEC_SLOT_OFFSET + offset, s_chunk_buf, chunk) != HAL_OK) {
                crc_ok = 0; break;
            }
            crc_accum  = fw_crc32_update(crc_accum, s_chunk_buf, chunk);
            offset    += chunk;
            remaining -= chunk;
            FW_IWDG_REFRESH();
        }

        crc_accum = fw_crc32_finish(crc_accum);

        if (!crc_ok || crc_accum != s_meta.staging.crc32) {
            GLogE("[SLOT] CRC mismatch: got 0x%08X expected 0x%08X (retry %d)\r\n",
                  (unsigned)crc_accum, (unsigned)s_meta.staging.crc32, retry);
            continue;
        }

        /* Step 5~7: 메타 갱신 + dual-write */
        memcpy(&s_meta.exec, &s_meta.staging, sizeof(slot_info_t));
        s_meta.copy_pending = COPY_NONE_FLAG;

        /* XSPI controller reset — extensive R/W 후 EXTMEM state 클리어 */
        HAL_XSPI_Abort(&hxspi1);

        if (meta_flush() != HAL_OK) {
            GLogE("[SLOT] meta flush after copy failed\r\n");
            return HAL_ERROR;
        }

        /* Step 8: BOOT_TEST 진입 */
        fw_set_flag(FW_FLAG_BOOT_TEST);
        fw_set_fail_count(0);

        GLogI("[SLOT] Copy complete (%u bytes), BOOT_TEST set\r\n", (unsigned)img_size);
        return HAL_OK;
    }

    GLogE("[SLOT] Copy failed after %d retries\r\n", COPY_MAX_RETRY);
    return HAL_ERROR;
#endif /* BOOT_BUILD */
}

/*============================================================================
 *  9. slot_clear_copy_pending
 *===========================================================================*/

HAL_StatusTypeDef slot_clear_copy_pending(void)
{
    if (!s_meta_loaded) {
        return HAL_ERROR;
    }

    s_meta.copy_pending = COPY_NONE_FLAG;
    return meta_flush();
}

/*============================================================================
 *  10~11. Slot invalidation
 *===========================================================================*/

HAL_StatusTypeDef slot_invalidate_exec(void)
{
    if (!s_meta_loaded) {
        return HAL_ERROR;
    }
    s_meta.exec.valid = SLOT_INVALID_MARK;
    return meta_flush();
}

HAL_StatusTypeDef slot_invalidate_staging(void)
{
    if (!s_meta_loaded) {
        return HAL_ERROR;
    }
    s_meta.staging.valid = SLOT_INVALID_MARK;
    return meta_flush();
}

/*============================================================================
 *  12. slot_get_max_image_size
 *===========================================================================*/

uint32_t slot_get_max_image_size(void)
{
    return SLOT_MAX_IMAGE_SIZE;
}

/*============================================================================
 *  13. slot_verify_image_crc
 *     실측 CRC32 계산 후 메타.crc32와 비교. Boot의 부팅 검증에 사용.
 *
 *  ★ EXEC도 indirect xspi_flash_read 사용 (Boot은 indirect-only mode).
 *    XIP memory-mapped는 BOOT_Application → MapMemory() 시점에 활성화되므로
 *    Boot_FwUpdateCheck 내에서 0x9001_0000 직접 dereference는 HardFault 유발.
 *===========================================================================*/

HAL_StatusTypeDef slot_verify_image_crc(slot_id_t slot, uint32_t *out_calc)
{
    const slot_info_t *info =
        (slot == SLOT_ID_EXEC) ? slot_get_exec_info() : slot_get_staging_info();

    if (info == NULL) return HAL_ERROR;
    if (info->valid != SLOT_VALID_MARK) return HAL_ERROR;
    if (info->image_size == 0U || info->image_size > SLOT_MAX_IMAGE_SIZE) {
        return HAL_ERROR;
    }

    uint32_t flash_base = (slot == SLOT_ID_EXEC) ? EXEC_SLOT_OFFSET
                                                 : STAGING_SLOT_OFFSET;

    uint32_t crc    = fw_crc32_start();
    uint32_t remain = info->image_size;
    uint32_t offset = 0U;
    uint32_t since_refresh = 0U;
    const uint32_t REFRESH_TICK = 1U << 20;   /* 1 MiB */

    while (remain > 0U) {
        uint32_t chunk = (remain > XSPI_VERIFY_CHUNK_SIZE)
                            ? XSPI_VERIFY_CHUNK_SIZE : remain;

        if (xspi_flash_read(flash_base + offset, s_chunk_buf, chunk) != HAL_OK) {
            GLogE("[SLOT] verify_crc: read fail slot=%d offset=0x%X\r\n",
                  (int)slot, (unsigned)offset);
            return HAL_ERROR;
        }

        crc            = fw_crc32_update(crc, s_chunk_buf, chunk);
        offset        += chunk;
        remain        -= chunk;
        since_refresh += chunk;

        if (since_refresh >= REFRESH_TICK) {
            FW_IWDG_REFRESH();
            since_refresh = 0U;
        }
    }

    uint32_t calc = fw_crc32_finish(crc);
    if (out_calc != NULL) *out_calc = calc;

    if (calc != info->crc32) {
        GLogI("[SLOT] verify_crc: slot=%d calc=0x%08X meta=0x%08X MISMATCH\r\n",
              (int)slot, (unsigned)calc, (unsigned)info->crc32);
        return HAL_ERROR;
    }

    return HAL_OK;
}
