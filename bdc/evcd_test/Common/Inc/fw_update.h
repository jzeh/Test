/**
  ******************************************************************************
  * @file    fw_update.h
  * @brief   FW Update System — Dual Slot Swap(Copy) Architecture (evcd_test 포팅)
  *
  *  포팅 출처: Z:\11_Work\00_Git_Source\05_Others\frp-scan\Common\Inc\fw_update.h
  *  주요 변경: firmware.h → firmware_lite.h, typedef.h 의존 제거 (Boot+Appli 공유)
  *
  *  Slot 0 (Exec, XIP 0x90010000) + Slot 1 (Staging, NOR offset 0x04000000)
  *  Bootloader copies Slot 1 → Slot 0 on copy_pending flag.
  *
  *  Design Ref: plans/frp-scan-fw-velvety-toucan.md
  ******************************************************************************
  */
#ifndef __FW_UPDATE_H__
#define __FW_UPDATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "firmware_lite.h"
#ifndef BOOT_BUILD
#include "ff.h"          /* FatFs — Appli OTA 파일 수신용. Boot 빌드는 FatFs 미링크 */
#endif
#include "fw_crc32.h"
#include "fw_image_header.h"   /* OTA 이미지 내장 헤더 (버전/크기/CRC) */


/*============================================================================
 *  1. External Flash Memory Map (128MB Macronix NOR, XSPI1)
 *===========================================================================*/

/* Physical offsets in External Flash */
#define SLOT_META_OFFSET_PRIMARY    0x00000000U   /* Slot Metadata (Primary) */
#define SLOT_META_SIZE              0x00010000U   /* 64KB (16 sectors) */
#define EXEC_SLOT_OFFSET            0x00010000U   /* Slot 0 (Execution) - XIP */
#define STAGING_SLOT_OFFSET         0x04000000U   /* Slot 1 (Staging) - no XIP */
#define SLOT_META_OFFSET_BACKUP     0x07FF0000U   /* Slot Metadata (Backup) */

/* XIP base addresses */
#define XSPI1_BASE_ADDR            0x90000000U
#define EXEC_SLOT_XIP_ADDR         (XSPI1_BASE_ADDR + EXEC_SLOT_OFFSET)  /* 0x90010000 */
#define META_BACKUP_XIP_ADDR       (XSPI1_BASE_ADDR + SLOT_META_OFFSET_BACKUP)

/* Slot size calculation */
#define FLASH_TOTAL_SIZE            0x08000000U   /* 128MB */
#define SLOT_MAX_IMAGE_SIZE         (STAGING_SLOT_OFFSET - EXEC_SLOT_OFFSET)  /* ~64MB */

/* Flash sector/page sizes (SFDP에서 자동 감지된 값과 일치) */
#define XSPI_SECTOR_SIZE            0x00001000U   /* 4KB sector (EraseType1) */
#define XSPI_PAGE_SIZE              256U          /* 256B program page */
#define XSPI_VERIFY_CHUNK_SIZE      4096U         /* 4KB readback verify unit */

/*============================================================================
 *  2. Slot Metadata Constants
 *===========================================================================*/

#define SLOT_META_MAGIC             0x534C5432U   /* "SLT2" */
#define SLOT_VALID_MARK             0xAAU
#define SLOT_INVALID_MARK           0x00U
#define COPY_PENDING_FLAG           0xAAU
#define COPY_NONE_FLAG              0x00U

/*============================================================================
 *  3. Backup SRAM Layout (base: 0x38800000)
 *     evcd_test에 기존 사용 없음 — frp-scan 레이아웃 그대로 채택.
 *===========================================================================*/

#ifndef BKPSRAM_BASE
#define BKPSRAM_BASE                0x38800000U
#endif

/* Reserved by legacy system (보존) */
#define BKPSRAM_FAILFLAG_OFFSET     0x400U

/* FW Update flags */
#define BKPSRAM_FW_FLAG_OFFSET      0x410U    /* fw_flag: 1B */
#define BKPSRAM_FAIL_COUNT_OFFSET   0x414U    /* fail_count: 4B */
#define BKPSRAM_RESERVED_418        0x418U
#define BKPSRAM_RECOVERY_PHASE      0x41CU    /* recovery_phase: 1B */
#define BKPSRAM_ORIG_BOOT_MODE      0x420U    /* original_boot_mode: 1B */

/* Checkpoint Breadcrumbs — 부팅/실행 단계 추적 (BKPSRAM 1바이트 마커).
 * hang/crash 시 ST-Link로 BKPSRAM 읽어 마지막 도달 단계 확인용.
 * Appli 의 차량 진단(DIAG_CAN 등, typedef.h)과 의미 다름 — prefix CHK_ 로 구분. */
#define BKPSRAM_CHK_BOOT_STAGE     0x430U
#define BKPSRAM_CHK_APPLI_STAGE    0x431U
#define BKPSRAM_CHK_APPLI_SHADOW   0x432U    /* shadow slot — preserves true crash stage */
#define BKPSRAM_CHK_BOOT_RETVAL    0x434U
#define BKPSRAM_CHK_MAGIC          0x438U
#define CHK_MAGIC_VALID            0xA5A5BEEFU

/* Dev-mode CRC bypass marker (SWD 직접 굽기 후 한 번 사용) */
#define BKPSRAM_DEV_BYPASS_OFFSET   0x448U
#define DEV_BYPASS_MAGIC            0xDEC0DE01U

/* Backup SRAM direct access macros
 * D-Cache 주의: BKPSRAM (0x38800000) 은 Cortex-M7 Normal memory 영역이므로
 * D-Cache 활성화 시 캐시에만 기록되고 실제 SRAM 에 반영되지 않을 수 있음.
 * 쓰기 후 SCB_CleanDCache_by_Addr 로 캐시 → SRAM flush 필수.
 * 읽기 전 SCB_InvalidateDCache_by_Addr 로 캐시 무효화하여 실제 SRAM 값 읽기. */
#define BKPSRAM_WRITE8(offset, val)  do { \
    *(__IO uint8_t  *)(BKPSRAM_BASE + (offset)) = (uint8_t)(val); \
    __DSB(); \
    SCB_CleanDCache_by_Addr((uint32_t *)(BKPSRAM_BASE + ((offset) & ~0x1FU)), 32); \
    } while(0)
#define BKPSRAM_READ8(offset)        \
    (SCB_InvalidateDCache_by_Addr((uint32_t *)(BKPSRAM_BASE + ((offset) & ~0x1FU)), 32), \
     *(__IO uint8_t  *)(BKPSRAM_BASE + (offset)))
#define BKPSRAM_WRITE32(offset, val) do { \
    *(__IO uint32_t *)(BKPSRAM_BASE + (offset)) = (uint32_t)(val); \
    __DSB(); \
    SCB_CleanDCache_by_Addr((uint32_t *)(BKPSRAM_BASE + ((offset) & ~0x1FU)), 32); \
    } while(0)
#define BKPSRAM_READ32(offset)       \
    (SCB_InvalidateDCache_by_Addr((uint32_t *)(BKPSRAM_BASE + ((offset) & ~0x1FU)), 32), \
     *(__IO uint32_t *)(BKPSRAM_BASE + (offset)))

/* Boot stage breadcrumbs */
#define CHK_BOOT_CLK_OK            0x10U
#define CHK_BOOT_EXTMEM_OK         0x20U
#define CHK_BOOT_FWCHECK_ENTER     0x30U
#define CHK_BOOT_FWCHECK_EXIT      0x35U
#define CHK_BOOT_APP_ENTER         0x40U
#define CHK_BOOT_MAP_OK            0x50U
#define CHK_BOOT_JUMP_ENTER        0x60U
#define CHK_BOOT_JUMP_PREP         0x70U
#define CHK_BOOT_CRC_VERIFY_START  0x80U
#define CHK_BOOT_CRC_MATCH_S0      0x81U
#define CHK_BOOT_CRC_MISMATCH_S0   0x82U
#define CHK_BOOT_CRC_MISMATCH_BOTH 0x83U
#define CHK_BOOT_CRC_SKIP_BADMETA  0x84U

/* Appli stage breadcrumbs (간소화 — 필요 시 확장) */
#define CHK_APPLI_MAIN_ENTRY       0x20U
#define CHK_APPLI_KERNEL_INIT_OK   0x80U
#define CHK_APPLI_TASK_ENTRY       0x90U

/* Writers */
#define CHK_SET_BOOT(x)    BKPSRAM_WRITE8 (BKPSRAM_CHK_BOOT_STAGE,  (x))
#define CHK_SET_APPLI(x)   BKPSRAM_WRITE8 (BKPSRAM_CHK_APPLI_STAGE, (x))
#define CHK_SET_RETVAL(x)  BKPSRAM_WRITE32(BKPSRAM_CHK_BOOT_RETVAL, (uint32_t)(x))
#define CHK_MARK_VALID()   BKPSRAM_WRITE32(BKPSRAM_CHK_MAGIC, CHK_MAGIC_VALID)
#define CHK_CLEAR_STAGES() do { \
    BKPSRAM_WRITE8(BKPSRAM_CHK_BOOT_STAGE,  0); \
    BKPSRAM_WRITE8(BKPSRAM_CHK_APPLI_STAGE, 0); \
} while (0)

/*============================================================================
 *  4. fw_flag State Machine Values
 *     IDLE → STAGING → COPY_PENDING → COPYING → BOOT_TEST → VERIFIED
 *
 *     각 값은 의도적으로 떨어진 비트 패턴 — 1비트 노이즈로 인접 상태에
 *     잘못 점프하지 않도록.
 *===========================================================================*/

#define FW_FLAG_IDLE                0x00U
#define FW_FLAG_STAGING             0xA5U
#define FW_FLAG_COPY_PENDING        0xB4U
#define FW_FLAG_COPYING             0xD2U
#define FW_FLAG_BOOT_TEST           0x5AU
#define FW_FLAG_VERIFIED            0xC3U

/*============================================================================
 *  5. GDS Protocol Command IDs (evcd_test BLE FuncID)
 *     frp-scan VCI3 명령 → evcd_test GDS 매핑 (Plan §5).
 *===========================================================================*/

#define GDS_FID_FW_UPDATE_START     0xA1U   /* 다운로드 시작 */
#define GDS_FID_FW_UPDATE_RECV      0xA2U   /* chunk 수신 */
#define GDS_FID_FW_UPDATE_END       0xA3U   /* 다운로드 종료 */
#define GDS_FID_FW_UPDATE_CHECK     0xA4U   /* CRC/SHA 검증 */
#define GDS_FID_FW_UPDATE_SET_LIST  0xA5U   /* 슬롯 갱신 + 리셋 */
#define GDS_FID_FW_UPDATE_FORCE     0xA6U   /* [TEST] eMMC 즉시 적용 + 리셋 */

/*============================================================================
 *  6. Recovery Cascade Constants
 *===========================================================================*/

#define RECOVERY_MAX_FAIL_COUNT     3U
#define RECOVERY_PHASE_NONE         0U
#define RECOVERY_PHASE_RECOPY       1U      /* Stage 1: Slot1 → Slot0 재복사 */
#define RECOVERY_PHASE_RELOAD       2U      /* Stage 2: eMMC 01_App → Slot1 → Slot0 */
#define RECOVERY_PHASE_ROLLBACK     4U      /* Stage 2.5: eMMC 02_Backup → Slot1 → Slot0 */
#define RECOVERY_PHASE_SAFEMODE     3U      /* Stage 3: Recovery FW (SAFE_MODE_APP) 강제 로드 */

#define COPY_MAX_RETRY              3U

/*============================================================================
 *  7. Multi-FW File Management
 *===========================================================================*/

#define DOWNLOAD_TEMP_FILE          "DownloadTemp.bin"

/* 앱이 0xA1 START 를 payload 공란으로 보낼 때 사용하는 기본값.
 * 파일명은 헤더에 없으므로 고정값 사용(boot 는 ini 에 기록된 이 이름을 그대로 사용하므로
 * 자체 일관성 유지). app_no/버전은 END 에서 내장 헤더값으로 최종 반영된다. */
#define OTA_DEFAULT_FILENAME        "mainApplication.bin"
#define OTA_DEFAULT_APP_NO          ((uint8_t)eApp_Main)

/* 0xA2 수신 포맷 선택 (빌드 심볼로 override 가능)
 *   1 = Intel-HEX 라인 1개/프레임 (tools/bin2hex.py 산출 .hex 전송)
 *   0 = 바이너리 청크 (tools/fw_sign.py 산출 .bin 전송, ≤512B)
 * 어느 쪽이든 스트림 선행 64B 는 내장 헤더이며 검증 로직은 동일하다. */
#ifndef OTA_RECV_HEX
#define OTA_RECV_HEX                1
#endif

/* HEX 라인 디코드 버퍼 크기: LL(≤255) + [LL,addr(2),type,CC] = 260 */
#define OTA_HEX_REC_MAX             260U

/*============================================================================
 *  8. Enumerations
 *===========================================================================*/

/** FW Update 상태 머신 (Appli 측 OTA 수신 진행) */
typedef enum {
    FW_STATE_IDLE = 0,
    FW_STATE_RECEIVING,
    FW_STATE_VERIFY_CRC,
    FW_STATE_VERIFY_SHA256,
    FW_STATE_STAGING,
    FW_STATE_COMPLETE,
    FW_STATE_ERROR
} fw_update_state_t;

/** 슬롯 식별자 (slot_verify_image_crc 인자) */
typedef enum {
    SLOT_ID_EXEC    = 0,        /* Slot 0 — XIP @ 0x90010000 */
    SLOT_ID_STAGING = 1         /* Slot 1 — physical 0x04000000, no XIP */
} slot_id_t;

/** FW Update 에러 코드 */
typedef enum {
    FW_ERR_NONE = 0,
    FW_ERR_FLASH_ERASE,
    FW_ERR_FLASH_PROGRAM,
    FW_ERR_FLASH_VERIFY,
    FW_ERR_CRC_MISMATCH,
    FW_ERR_SHA256_MISMATCH,
    FW_ERR_FILE_IO,
    FW_ERR_SIZE_EXCEED,
    FW_ERR_INVALID_HEADER,
    FW_ERR_SLOT_INVALID,
    FW_ERR_TIMEOUT,
    FW_ERR_UNKNOWN
} fw_update_error_t;

/** OTA 응답 코드 (GDS 0xA1 응답의 페이로드 1바이트) */
typedef enum {
    FW_RESP_OK              = 0x00U,
    FW_RESP_GENERIC_ERR     = 0x01U,
    FW_RESP_NOT_IDLE        = 0x02U,    /* SystemState가 IDLE이 아님 */
    FW_RESP_FILE_OPEN_FAIL  = 0x03U,
    FW_RESP_VBAT_LOST       = 0x05U
} fw_update_resp_t;

/*============================================================================
 *  9. Data Structures
 *===========================================================================*/

/** Per-slot information (45 bytes) */
typedef __packed struct {
    uint8_t  version[3];        /* Major.Minor.Patch */
    uint32_t image_size;        /* FW image size in bytes */
    uint32_t crc32;             /* CRC32 checksum */
    uint8_t  sha256[32];        /* SHA-256 hash */
    uint8_t  valid;             /* 0xAA = valid */
    uint8_t  app_no;            /* App number (AppName enum) */
} slot_info_t;

/** Slot Metadata — Primary + Backup 두 위치에 저장 (4096 bytes = 1 sector) */
typedef __packed struct {
    uint32_t    magic;          /* 0x534C5432 "SLT2" */
    slot_info_t exec;           /* Slot 0 정보 */
    slot_info_t staging;        /* Slot 1 정보 */
    uint8_t     copy_pending;   /* 0xAA = copy 필요 */
    uint32_t    metadata_crc32; /* 이 구조체의 CRC32 (이 필드 + reserved 제외) */
    uint8_t     reserved[4096 - 4 - 45 - 45 - 1 - 4];
} slot_metadata_t;

/* 컴파일 타임 사이즈 검증 */
_Static_assert(sizeof(slot_metadata_t) == 4096, "slot_metadata_t must be exactly 4096 bytes");
_Static_assert(sizeof(slot_info_t) == 45, "slot_info_t must be exactly 45 bytes");

/** FW Update 런타임 컨텍스트 (Appli 측)
 *  FIL 멤버 때문에 FatFs 의존 — Appli 전용. Boot 빌드는 이 구조체를 쓰지 않음. */
#ifndef BOOT_BUILD
typedef struct {
    fw_update_state_t state;
    fw_update_error_t error;

    /* 파일 수신 */
    FIL               file_handle;
    uint32_t          received_bytes;
    uint32_t          expected_size;
    uint32_t          crc_accumulator;

    /* 메타 */
    uint8_t           cur_file_no;
    char              cur_file_name[64];

    /* 진행률 */
    uint8_t           progress_percent;

    /* 시퀀스 (0xA2 retransmit 용) */
    uint16_t          expected_seq;

    /* OTA 이미지 내장 헤더 — 수신 스트림 선행 FW_IMG_HDR_SIZE(64) 바이트를
     * 여기에 모아 검증한 뒤 떼어내고, payload 만 DownloadTemp.bin 에 저장한다. */
    uint8_t           hdr_bytes[FW_IMG_HDR_SIZE];  /* 헤더 원본 누적 버퍼 */
    uint16_t          hdr_filled;                  /* 지금까지 모은 헤더 바이트 수 */
    uint8_t           hdr_valid;                   /* 1 = 헤더 magic+CRC 검증 통과 */
    fw_image_header_t hdr;                          /* 파싱된 헤더 */
} fw_update_ctx_t;
#endif /* !BOOT_BUILD */

/*============================================================================
 *  10. Backup SRAM Accessor Inlines (Boot + Appli 공통)
 *===========================================================================*/

static inline void fw_set_flag(uint8_t flag)
{
    BKPSRAM_WRITE8(BKPSRAM_FW_FLAG_OFFSET, flag);
}

static inline uint8_t fw_get_flag(void)
{
    return BKPSRAM_READ8(BKPSRAM_FW_FLAG_OFFSET);
}

static inline void fw_set_fail_count(uint32_t count)
{
    BKPSRAM_WRITE32(BKPSRAM_FAIL_COUNT_OFFSET, count);
}

static inline uint32_t fw_get_fail_count(void)
{
    return BKPSRAM_READ32(BKPSRAM_FAIL_COUNT_OFFSET);
}

static inline void fw_set_recovery_phase(uint8_t phase)
{
    BKPSRAM_WRITE8(BKPSRAM_RECOVERY_PHASE, phase);
}

static inline uint8_t fw_get_recovery_phase(void)
{
    return BKPSRAM_READ8(BKPSRAM_RECOVERY_PHASE);
}

static inline void fw_set_original_boot_mode(uint8_t mode)
{
    BKPSRAM_WRITE8(BKPSRAM_ORIG_BOOT_MODE, mode);
}

static inline uint8_t fw_get_original_boot_mode(void)
{
    return BKPSRAM_READ8(BKPSRAM_ORIG_BOOT_MODE);
}

/*============================================================================
 *  11. Function Declarations
 *     Step 별로 구현 채워짐.
 *===========================================================================*/

/* --- Step 1: XSPI Flash API (fw_flash_xspi.c) --- */
HAL_StatusTypeDef xspi_flash_init(void);
uint32_t          xspi_flash_get_size(void);
HAL_StatusTypeDef xspi_flash_erase_sector(uint32_t addr, uint32_t size);
HAL_StatusTypeDef xspi_flash_program(uint32_t addr, const uint8_t *data, uint32_t len);
HAL_StatusTypeDef xspi_flash_read(uint32_t addr, uint8_t *buf, uint32_t len);
HAL_StatusTypeDef xspi_flash_verify(uint32_t addr, const uint8_t *data, uint32_t len);

/* --- Step 2: Slot Manager (fw_slot_manager.c) --- */
HAL_StatusTypeDef       slot_manager_init(void);
uint32_t                slot_get_exec_addr(void);
uint32_t                slot_get_staging_addr(void);
const slot_info_t*      slot_get_exec_info(void);
const slot_info_t*      slot_get_staging_info(void);
HAL_StatusTypeDef       slot_mark_staging_valid(uint8_t ver[3], uint32_t size,
                                                uint32_t crc, uint8_t sha[32],
                                                uint8_t app_no);
HAL_StatusTypeDef       slot_set_copy_pending(void);
HAL_StatusTypeDef       slot_copy_staging_to_exec(void);
HAL_StatusTypeDef       slot_clear_copy_pending(void);
HAL_StatusTypeDef       slot_invalidate_exec(void);
HAL_StatusTypeDef       slot_invalidate_staging(void);
uint32_t                slot_get_max_image_size(void);
HAL_StatusTypeDef       slot_verify_image_crc(slot_id_t slot, uint32_t *out_calc);

/* --- Step 3: Backup SRAM init (Boot+Appli main에서 호출) --- */
void                    BackupSRAM_Init(void);

/* --- Step 6: FW Update Core (fw_update.c) --- */
void                    fw_update_init(void);
fw_update_state_t       fw_update_get_state(void);
fw_update_error_t       fw_update_get_error(void);
uint8_t                 fw_update_get_progress(void);

/* 수신 이미지에 내장된 헤더 반환. 헤더 검증 전이면 NULL.
 * 0xA5 SetList 등에서 버전/app_no 를 이 헤더에서 가져올 수 있다. */
const fw_image_header_t* fw_update_get_header(void);

/* OTA 청크 수신 진행 중(0xA1 START ~ 0xA3 END 사이) 여부. 1 = 수신 중.
 *  · enum 타입 의존이 없어 다른 모듈(git-comm.c)에서 extern 선언만으로 사용 가능
 *  · 용도: 수신 구간에서 UART2 ASCII-HEX 자동변환 같은 전처리를 우회 */
uint8_t fw_update_is_receiving(void);

/* 현재 실행 중인 펌웨어 버전 — 저장소(FirmwareInfo.ini)에서 읽어 캐시.
 *  · fw_update_load_running_version : eMMC 마운트 후 명시적 로드(부팅 시 1회 권장)
 *  · fw_update_get_running_version  : 캐시 반환(미로드 시 lazy 로드 시도)
 *                                     out_ver[0..2] = major.minor.patch
 *                                     반환 HAL_OK = 저장소에서 유효값 로드됨 */
void              fw_update_load_running_version(void);
HAL_StatusTypeDef fw_update_get_running_version(uint8_t out_ver[3]);

HAL_StatusTypeDef       fw_update_start(uint8_t file_no, const char *filename);
HAL_StatusTypeDef       fw_update_recv(const uint8_t *data, uint32_t len);
HAL_StatusTypeDef       fw_update_check(uint32_t expected_crc32);  /* DEPRECATED: CRC는 fw_update_end 로 병합 */
HAL_StatusTypeDef       fw_update_end(uint32_t expected_size, uint32_t expected_crc32);
/* FirmwareInfo.ini 갱신 + fw_flag=COPY_PENDING 까지만 수행하고 결과를 반환(리셋 없음).
 * 호출자가 성공 여부를 확인한 뒤 ACK 를 보내고 직접 리셋하는 용도.
 *   → "모든 처리 완료 → 응답 → 리셋" 순서를 보장하기 위함. */
HAL_StatusTypeDef       fw_update_set_list_no_reset(uint8_t boot_app_no, const uint8_t ver[3]);

/* 위 함수 + 100ms 대기 후 SystemReset (성공 시 미복귀). 하위호환용. */
HAL_StatusTypeDef       fw_update_set_list(uint8_t boot_app_no, const uint8_t ver[3]);
HAL_StatusTypeDef       fw_update_backup_before_replace(const char *filename);
HAL_StatusTypeDef       fw_update_swap_ini_from_backup(void);

#ifdef __cplusplus
}
#endif

#endif /* __FW_UPDATE_H__ */
