/**
  ******************************************************************************
  * @file    fw_update.c
  * @brief   FW Update — Appli OTA 다운로드 상태머신 + 명령 핸들러
  *
  *  Appli 전용. Boot 빌드에는 link하지 말 것.
  *
  *  포팅 출처: Z:\11_Work\00_Git_Source\05_Others\frp-scan\Common\Src\fw_update.c
  *  주요 변경:
  *    - SHA-256 호출 모두 제거 (옵션 B: 필드만 보존, 구현 제거)
  *    - VCI3 프로토콜 의존 제거 → GDS 핸들러는 git-functionlist.c 측에서 호출
  *    - 다중앱 → 단일앱(Main) + Recovery FW 1개로 단순화
  *    - 슬롯 staging 은 Boot 전담 (Plan §5) — Appli 측은 fw_flag=COPY_PENDING + reset만
  *
  *  OTA 흐름:
  *    [Host] ─ 0xA1 START ─►  fw_update_start         eMMC: 02_Backup 백업 + DownloadTemp.bin
  *    [Host] ─ 0xA2 RECV  ─►  fw_update_recv          청크 append + CRC32 누적
  *    [Host] ─ 0xA3 END   ─►  fw_update_end           크기검증 + CRC32 검증 → 통과 시 01_Application/<name>
  *    [Host] ─ 0xA4 CHECK ─►  fw_update_check         [DEPRECATED] CRC는 0xA3로 병합 (호환 스텁)
  *    [Host] ─ 0xA5 LIST  ─►  fw_update_set_list      FirmwareInfo 갱신 + COPY_PENDING + reset
  ******************************************************************************
  */

#include "fw_update.h"
#include "fw_compat.h"
#include "fw_slot_manager.h"

#include <string.h>
#include <stdio.h>

/* Note: SystemState 검증은 호출자(git-functionlist.c::FL_GDS_FwUpdate_Start)에서 수행.
 * Common 모듈은 Appli 의존성을 갖지 않도록 격리. */

/*============================================================================
 *  Private state
 *===========================================================================*/

static fw_update_ctx_t s_ctx;

/* OTA 임시 파일 경로 */
#define OTA_TEMP_PATH       DOWNLOAD_TEMP_FILE                   /* "DownloadTemp.bin" */
#define OTA_APP_DIR         "01_Application"
#define OTA_BACKUP_DIR      "02_Backup"

/* 4KB 청크 버퍼 — 02_Backup 복사용. BSS 배치. */
static uint8_t s_copy_buf[4096];

/*============================================================================
 *  Private: helpers
 *===========================================================================*/

static void ctx_reset(void)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.state           = FW_STATE_IDLE;
    s_ctx.error           = FW_ERR_NONE;
    s_ctx.crc_accumulator = fw_crc32_start();
    s_ctx.expected_seq    = 0U;
}

static HAL_StatusTypeDef copy_file(const char *src_path, const char *dst_path)
{
    FIL  src, dst;
    UINT br, bw;
    FRESULT rc;

    rc = f_open(&src, src_path, FA_OPEN_EXISTING | FA_READ);
    if (rc != FR_OK) return HAL_ERROR;

    rc = f_open(&dst, dst_path, FA_CREATE_ALWAYS | FA_WRITE);
    if (rc != FR_OK) {
        f_close(&src);
        return HAL_ERROR;
    }

    HAL_StatusTypeDef ret = HAL_OK;
    for (;;) {
        rc = f_read(&src, s_copy_buf, sizeof(s_copy_buf), &br);
        if (rc != FR_OK)    { ret = HAL_ERROR; break; }
        if (br == 0)        break;

        rc = f_write(&dst, s_copy_buf, br, &bw);
        if (rc != FR_OK || bw != br) { ret = HAL_ERROR; break; }
    }

    f_close(&dst);
    f_close(&src);
    return ret;
}

/*============================================================================
 *  fw_update_init — Appli 부팅 후 한 번 호출 (sys-main.c 또는 main.c)
 *===========================================================================*/

void fw_update_init(void)
{
    ctx_reset();

    /* BKPSRAM fw_flag = BOOT_TEST 이면 자가 검증 대기 상태.
     * sys-main.c 의 IDLE 도달 감시 로직이 VERIFIED 로 전이시킴 (Plan §8). */

    /* 현재 실행 버전을 저장소에서 캐시 (best-effort — eMMC 미마운트면 lazy 로드로 넘어감) */
    fw_update_load_running_version();
}

/*============================================================================
 *  Getters
 *===========================================================================*/

fw_update_state_t fw_update_get_state(void)    { return s_ctx.state; }
fw_update_error_t fw_update_get_error(void)    { return s_ctx.error; }
uint8_t           fw_update_get_progress(void) { return s_ctx.progress_percent; }

const fw_image_header_t* fw_update_get_header(void)
{
    return s_ctx.hdr_valid ? &s_ctx.hdr : NULL;
}

uint8_t fw_update_is_receiving(void)
{
    return (s_ctx.state == FW_STATE_RECEIVING) ? 1U : 0U;
}

/*============================================================================
 *  현재 실행 펌웨어 버전 — 저장소(FirmwareInfo.ini) 읽기 + 캐시
 *    OTA(0xA5/END)가 기록한 버전을 그대로 읽어온다(저장/읽기 대칭).
 *    ※ eMMC 가 마운트되어 있어야 읽힘 — 미마운트 시 로드 실패(0.0.0 유지, 재시도 가능).
 *===========================================================================*/

static uint8_t s_run_ver[3] = { 0, 0, 0 };
static uint8_t s_run_ver_ok = 0U;   /* 1 = 저장소에서 유효값 로드됨 */

void fw_update_load_running_version(void)
{
    SFwInfo fi;
    FIL     fp;
    UINT    br;

    if (f_open(&fp, BOOT_FWINFO_FILE, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
        return;   /* eMMC 미마운트/파일없음 — 캐시 유지, 다음 호출에 재시도 */
    }
    FRESULT rc = f_read(&fp, &fi, sizeof(fi), &br);
    f_close(&fp);
    if (rc != FR_OK || br != sizeof(fi) || fi.muiPreamble != VCI3_FWINFO_PREAMBLE) {
        return;
    }

    /* 현재 실행 앱 선택: mucCurrentMode 우선, 없으면 mucBootMode */
    uint8_t app = fi.mucCurrentMode;
    if (app == 0U || app >= (uint8_t)eApp_MAX) app = fi.mucBootMode;
    if (app == 0U || app >= (uint8_t)eApp_MAX) return;

    s_run_ver[0] = fi.msAppInfo[app - 1U].marrucVersion[0];
    s_run_ver[1] = fi.msAppInfo[app - 1U].marrucVersion[1];
    s_run_ver[2] = fi.msAppInfo[app - 1U].marrucVersion[2];
    s_run_ver_ok = 1U;
    GLogI("[FW] running version loaded: v%u.%u.%u (app=%u)\r\n",
          (unsigned)s_run_ver[0], (unsigned)s_run_ver[1],
          (unsigned)s_run_ver[2], (unsigned)app);
}

HAL_StatusTypeDef fw_update_get_running_version(uint8_t out_ver[3])
{
    if (out_ver == NULL) return HAL_ERROR;
    if (!s_run_ver_ok) {
        fw_update_load_running_version();   /* lazy: 첫 조회 시 저장소에서 로드 */
    }
    out_ver[0] = s_run_ver[0];
    out_ver[1] = s_run_ver[1];
    out_ver[2] = s_run_ver[2];
    return s_run_ver_ok ? HAL_OK : HAL_ERROR;
}

/*============================================================================
 *  fw_update_backup_before_replace
 *    01_Application/<filename> → 02_Backup/<filename> 사전 백업.
 *    OTA가 새 .bin을 받기 전에 호출 — 기존 안정 버전 보존 (rollback Stage 2.5).
 *===========================================================================*/

HAL_StatusTypeDef fw_update_backup_before_replace(const char *filename)
{
    if (filename == NULL) return HAL_ERROR;

    char src_path[80];
    char dst_path[80];
    snprintf(src_path, sizeof(src_path), "%s/%s", OTA_APP_DIR, filename);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", OTA_BACKUP_DIR, filename);

    /* 원본이 없으면 graceful skip (best-effort) */
    FILINFO fi;
    if (f_stat(src_path, &fi) != FR_OK) {
        GLogI("[OTA] backup skip — no src %s\r\n", src_path);
        return HAL_OK;
    }

    HAL_StatusTypeDef rc = copy_file(src_path, dst_path);
    if (rc == HAL_OK) {
        GLogI("[OTA] backup: %s → %s (%lu B)\r\n",
              src_path, dst_path, (unsigned long)fi.fsize);
    } else {
        GLogE("[OTA] backup FAILED\r\n");
    }
    return rc;
}

/*============================================================================
 *  0xA1 — fw_update_start
 *    Pre-condition: SystemState == IDLE
 *    Side effects:  02_Backup 복사 + DownloadTemp.bin 생성 + fw_flag = STAGING
 *===========================================================================*/

HAL_StatusTypeDef fw_update_start(uint8_t file_no, const char *filename)
{
    /* SystemState 검증은 호출자가 책임. 여기서는 인자만 검증. */
    if (filename == NULL || filename[0] == '\0') {
        s_ctx.error = FW_ERR_INVALID_HEADER;
        return HAL_ERROR;
    }

    /* (2) 이전 컨텍스트 클리어 */
    ctx_reset();
    s_ctx.cur_file_no = file_no;
    strncpy(s_ctx.cur_file_name, filename, sizeof(s_ctx.cur_file_name) - 1);

    /* (3) 02_Backup/<filename> 사전 백업 (best-effort) */
    (void)fw_update_backup_before_replace(filename);

    /* (4) DownloadTemp.bin 새로 생성 */
    FRESULT rc = f_open(&s_ctx.file_handle, OTA_TEMP_PATH,
                        FA_CREATE_ALWAYS | FA_WRITE);
    if (rc != FR_OK) {
        s_ctx.error = FW_ERR_FILE_IO;
        return HAL_ERROR;
    }

    /* (5) 상태 전이 */
    s_ctx.state = FW_STATE_RECEIVING;
    fw_set_flag(FW_FLAG_STAGING);

    GLogI("[OTA] START app=%u file=%s\r\n", (unsigned)file_no, filename);
    return HAL_OK;
}

/*============================================================================
 *  0xA2 — fw_update_recv  (chunk 수신)
 *===========================================================================*/

HAL_StatusTypeDef fw_update_recv(const uint8_t *data, uint32_t len)
{
    if (s_ctx.state != FW_STATE_RECEIVING) {
        s_ctx.error = FW_ERR_UNKNOWN;
        return HAL_ERROR;
    }
    if (data == NULL || len == 0U) {
        s_ctx.error = FW_ERR_INVALID_HEADER;
        return HAL_ERROR;
    }

    /* (A) 선행 FW_IMG_HDR_SIZE(64) 바이트 = 내장 헤더.
     *     파일에는 쓰지 않고 별도 버퍼에 모은다(여러 청크에 걸칠 수 있음). */
    if (s_ctx.hdr_filled < FW_IMG_HDR_SIZE) {
        uint32_t need = (uint32_t)FW_IMG_HDR_SIZE - s_ctx.hdr_filled;
        uint32_t take = (len < need) ? len : need;
        memcpy(&s_ctx.hdr_bytes[s_ctx.hdr_filled], data, take);
        s_ctx.hdr_filled += (uint16_t)take;
        data += take;
        len  -= take;

        /* 헤더가 완성되면 magic + 헤더 CRC 검증 */
        if (s_ctx.hdr_filled == FW_IMG_HDR_SIZE) {
            memcpy(&s_ctx.hdr, s_ctx.hdr_bytes, sizeof(s_ctx.hdr));

            if (s_ctx.hdr.magic != FW_IMG_HDR_MAGIC) {
                GLogE("[OTA] bad image magic: 0x%08lX (expect 0x%08lX)\r\n",
                      (unsigned long)s_ctx.hdr.magic, (unsigned long)FW_IMG_HDR_MAGIC);
                s_ctx.error = FW_ERR_INVALID_HEADER;
                f_close(&s_ctx.file_handle);
                s_ctx.state = FW_STATE_ERROR;
                return HAL_ERROR;
            }
            uint32_t hcrc = fw_crc32(s_ctx.hdr_bytes, FW_IMG_HDR_CRC_LEN);
            if (hcrc != s_ctx.hdr.hdr_crc32) {
                GLogE("[OTA] header CRC mismatch: calc=0x%08lX hdr=0x%08lX\r\n",
                      (unsigned long)hcrc, (unsigned long)s_ctx.hdr.hdr_crc32);
                s_ctx.error = FW_ERR_INVALID_HEADER;
                f_close(&s_ctx.file_handle);
                s_ctx.state = FW_STATE_ERROR;
                return HAL_ERROR;
            }
            s_ctx.hdr_valid = 1U;
            GLogI("[OTA] header OK: v%u.%u.%u app=%u size=%lu crc=0x%08lX\r\n",
                  (unsigned)s_ctx.hdr.fw_version[0], (unsigned)s_ctx.hdr.fw_version[1],
                  (unsigned)s_ctx.hdr.fw_version[2], (unsigned)s_ctx.hdr.app_no,
                  (unsigned long)s_ctx.hdr.image_size, (unsigned long)s_ctx.hdr.image_crc32);
        }

        if (len == 0U) {
            return HAL_OK;   /* 이 청크는 헤더 바이트뿐 — payload 없음 */
        }
    }

    /* (B) 나머지 = payload → 파일 기록 + CRC32 누적 */
    UINT bw;
    FRESULT rc = f_write(&s_ctx.file_handle, data, len, &bw);
    if (rc != FR_OK || bw != len) {
        s_ctx.error = FW_ERR_FILE_IO;
        f_close(&s_ctx.file_handle);
        s_ctx.state = FW_STATE_ERROR;
        return HAL_ERROR;
    }

    s_ctx.crc_accumulator = fw_crc32_update(s_ctx.crc_accumulator, data, len);
    s_ctx.received_bytes += len;

    return HAL_OK;
}

/*============================================================================
 *  0xA3 — fw_update_end  (다운로드 종료 + 크기검증 + CRC32 검증 병합)
 *    구 0xA4(fw_update_check)의 CRC32 검증을 여기로 병합.
 *    순서: close → 크기검증 → CRC32 검증 → (통과 시) 01_Application 으로 rename.
 *    ★ rename 전에 CRC 를 검증하므로, 불량 이미지는 01_Application 에 절대 들어가지 않고
 *      기존 01_Application/<name> 도 무손상으로 보존됨(덮어쓰기 전이라 복원 불필요).
 *===========================================================================*/

HAL_StatusTypeDef fw_update_end(uint32_t expected_size, uint32_t expected_crc32)
{
    if (s_ctx.state != FW_STATE_RECEIVING) {
        s_ctx.error = FW_ERR_UNKNOWN;
        return HAL_ERROR;
    }

    /* (1) close 후 sync */
    f_close(&s_ctx.file_handle);

    /* (1-a) 내장 헤더가 검증되지 않았으면 거부 — 이미지에 EVCD 헤더가 없는 것.
     *       (헤더 magic/CRC 는 fw_update_recv 수신 중에 이미 검증됨) */
    if (!s_ctx.hdr_valid) {
        GLogE("[OTA] no valid embedded header — reject\r\n");
        s_ctx.error = FW_ERR_INVALID_HEADER;
        f_unlink(OTA_TEMP_PATH);
        s_ctx.state = FW_STATE_ERROR;
        fw_set_flag(FW_FLAG_IDLE);
        return HAL_ERROR;
    }

    /* (1-b) 앱이 size/CRC 를 함께 보냈으면(0 아님) 내장 헤더와 교차검증.
     *       불일치 시 앱↔이미지 헤더 간 불일치이므로 거부. */
    if (expected_size != 0U && expected_size != s_ctx.hdr.image_size) {
        GLogE("[OTA] app size(%lu) != header size(%lu)\r\n",
              (unsigned long)expected_size, (unsigned long)s_ctx.hdr.image_size);
        s_ctx.error = FW_ERR_SIZE_EXCEED;
        f_unlink(OTA_TEMP_PATH);
        s_ctx.state = FW_STATE_ERROR;
        fw_set_flag(FW_FLAG_IDLE);
        return HAL_ERROR;
    }
    if (expected_crc32 != 0U && expected_crc32 != s_ctx.hdr.image_crc32) {
        GLogE("[OTA] app crc(0x%08lX) != header crc(0x%08lX)\r\n",
              (unsigned long)expected_crc32, (unsigned long)s_ctx.hdr.image_crc32);
        s_ctx.error = FW_ERR_CRC_MISMATCH;
        f_unlink(OTA_TEMP_PATH);
        s_ctx.state = FW_STATE_ERROR;
        fw_set_flag(FW_FLAG_IDLE);
        return HAL_ERROR;
    }

    /* (2) 크기 검증 — 수신 payload 크기 vs 내장 헤더값 */
    if (s_ctx.received_bytes != s_ctx.hdr.image_size) {
        GLogE("[OTA] size mismatch: got %lu header %lu\r\n",
              (unsigned long)s_ctx.received_bytes,
              (unsigned long)s_ctx.hdr.image_size);
        s_ctx.error = FW_ERR_SIZE_EXCEED;
        f_unlink(OTA_TEMP_PATH);
        s_ctx.state = FW_STATE_ERROR;
        return HAL_ERROR;
    }
    s_ctx.expected_size = s_ctx.hdr.image_size;

    /* (3) CRC32 검증 — 0xA2 수신 중 payload 로 누적한 CRC vs 내장 헤더값.
     *     기준값(header)이 빌드 시점 독립 출처이므로 전송/저장 손상을 검출한다.
     *     실패 시 임시파일만 제거, 기존 01_Application 은 건드리지 않음. */
    uint32_t calc = fw_crc32_finish(s_ctx.crc_accumulator);
    if (calc != s_ctx.hdr.image_crc32) {
        GLogE("[OTA] CRC mismatch: calc=0x%08lX header=0x%08lX\r\n",
              (unsigned long)calc, (unsigned long)s_ctx.hdr.image_crc32);
        s_ctx.error = FW_ERR_CRC_MISMATCH;
        f_unlink(OTA_TEMP_PATH);
        s_ctx.state = FW_STATE_ERROR;
        fw_set_flag(FW_FLAG_IDLE);
        return HAL_ERROR;
    }

    /* (4) 검증 통과 → DownloadTemp.bin → 01_Application/<filename> 으로 확정 이동 */
    char dst[80];
    snprintf(dst, sizeof(dst), "%s/%s", OTA_APP_DIR, s_ctx.cur_file_name);

    f_unlink(dst);   /* best-effort — 없으면 무시 */

    FRESULT rc = f_rename(OTA_TEMP_PATH, dst);
    if (rc != FR_OK) {
        GLogE("[OTA] rename failed (%d)\r\n", (int)rc);
        s_ctx.error = FW_ERR_FILE_IO;
        s_ctx.state = FW_STATE_ERROR;
        return HAL_ERROR;
    }

    /* CRC 까지 통과했으므로 바로 COMPLETE (구 VERIFY_CRC 단계 생략) */
    s_ctx.state            = FW_STATE_COMPLETE;
    s_ctx.progress_percent = 100;
    GLogI("[OTA] END+CHECK OK size=%lu CRC=0x%08lX file=%s\r\n",
          (unsigned long)s_ctx.received_bytes, (unsigned long)calc, dst);
    return HAL_OK;
}

/*============================================================================
 *  0xA4 — fw_update_check  [DEPRECATED]
 *    CRC32 검증은 fw_update_end(0xA3)로 병합됨. 이 함수는 하위호환용 스텁:
 *    이미 END 에서 검증/COMPLETE 되었으면 OK, 아니면 ERROR 만 반환.
 *    (호스트가 0xA4 를 더 이상 보내지 않아도 0xA5 는 정상 동작함)
 *===========================================================================*/

HAL_StatusTypeDef fw_update_check(uint32_t expected_crc32)
{
    (void)expected_crc32;   /* CRC 검증은 fw_update_end 에서 이미 수행 */
    return (s_ctx.state == FW_STATE_COMPLETE) ? HAL_OK : HAL_ERROR;
}

/*============================================================================
 *  0xA5 — fw_update_set_list  (FirmwareInfo 갱신 + COPY_PENDING + reset)
 *
 *  Plan §5: Slot 1 staging 은 Boot 전담.
 *           여기서는 fw_flag = COPY_PENDING + System Reset 만.
 *           Boot 이 부팅 시 eMMC → Slot 1 → Slot 0 복사.
 *===========================================================================*/

HAL_StatusTypeDef fw_update_set_list_no_reset(uint8_t boot_app_no, const uint8_t ver[3])
{
    if (s_ctx.state != FW_STATE_COMPLETE) {
        s_ctx.error = FW_ERR_UNKNOWN;
        return HAL_ERROR;
    }
    if (boot_app_no == 0U || boot_app_no >= eApp_MAX) {
        s_ctx.error = FW_ERR_INVALID_HEADER;
        return HAL_ERROR;
    }

    /* (1) FirmwareInfo.ini 읽고 — 없으면 새로 생성 */
    SFwInfo fi;
    memset(&fi, 0, sizeof(fi));
    fi.muiPreamble    = VCI3_FWINFO_PREAMBLE;
    fi.mucInitialized = 0xAAU;

    FIL fp;
    UINT br, bw;
    if (f_open(&fp, BOOT_FWINFO_FILE, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
        f_read(&fp, &fi, sizeof(fi), &br);
        f_close(&fp);
        if (fi.muiPreamble != VCI3_FWINFO_PREAMBLE) {
            memset(&fi, 0, sizeof(fi));
            fi.muiPreamble    = VCI3_FWINFO_PREAMBLE;
            fi.mucInitialized = 0xAAU;
        }
    }

    /* (2) 부팅 모드 + 버전 갱신 */
    fi.mucBootMode = boot_app_no;
    if (boot_app_no < MAX_APP_CNT) {
        fi.msAppInfo[boot_app_no - 1].marrucVersion[0] = ver ? ver[0] : 0;
        fi.msAppInfo[boot_app_no - 1].marrucVersion[1] = ver ? ver[1] : 0;
        fi.msAppInfo[boot_app_no - 1].marrucVersion[2] = ver ? ver[2] : 0;
        strncpy(fi.msAppInfo[boot_app_no - 1].marrcFilename,
                s_ctx.cur_file_name,
                sizeof(fi.msAppInfo[boot_app_no - 1].marrcFilename) - 1);
    }

    /* (3) 파일 다시 쓰기 (FA_CREATE_ALWAYS) */
    if (f_open(&fp, BOOT_FWINFO_FILE, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        s_ctx.error = FW_ERR_FILE_IO;
        return HAL_ERROR;
    }
    f_write(&fp, &fi, sizeof(fi), &bw);
    f_sync(&fp);       /* FAT/디렉토리 엔트리를 eMMC에 즉시 flush — reset 전 필수 */
    f_close(&fp);

    /* (4) fw_flag = COPY_PENDING — Boot 이 다음 부팅에 staging 수행 */
    fw_set_flag(FW_FLAG_COPY_PENDING);

    GLogI("[OTA] SET_LIST boot_app=%u — staging armed (ini+flag done)\r\n",
          (unsigned)boot_app_no);

    /* 여기까지가 "모든 처리". 리셋은 호출자가 응답 송신 후 수행. */
    return HAL_OK;
}

/*  fw_update_set_list — 위 처리 + 리셋 (하위호환 래퍼).
 *  응답을 리셋 직전에 보내야 하는 경로에서는 fw_update_set_list_no_reset() 를
 *  직접 호출하고, 호출자가 ACK 송신 후 리셋할 것.
 */
HAL_StatusTypeDef fw_update_set_list(uint8_t boot_app_no, const uint8_t ver[3])
{
    HAL_StatusTypeDef rc = fw_update_set_list_no_reset(boot_app_no, ver);
    if (rc != HAL_OK) {
        return rc;
    }

    GLogI("[OTA] reset in 100ms\r\n");
    HAL_Delay(100);
    HAL_NVIC_SystemReset();
    /* unreachable */
    return HAL_OK;
}

/*============================================================================
 *  fw_update_swap_ini_from_backup
 *  Boot 의 rollback(Stage 2.5) 직후 Appli 가 호출 — ini를 02_Backup 쪽으로 동기화.
 *  현재 evcd_test 단일앱 구성에서는 ini 단순 — 호출 시 best-effort 처리.
 *===========================================================================*/

HAL_StatusTypeDef fw_update_swap_ini_from_backup(void)
{
    /* 단일앱 + Recovery FW 구성에서는 AppSwList.ini 가 사실상 정적.
     * Backup .bak 가 있으면 복원, 없으면 skip — 비파괴적. */
    const char *src = OTA_BACKUP_DIR "/" APPLICATION_INFO_FILE_NAME ".bak";
    const char *dst = OTA_APP_DIR    "/" APPLICATION_INFO_FILE_NAME;

    FILINFO fi;
    if (f_stat(src, &fi) != FR_OK) return HAL_OK;   /* 없으면 skip */
    return copy_file(src, dst);
}
