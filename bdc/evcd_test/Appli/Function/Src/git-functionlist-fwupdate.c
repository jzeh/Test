/**
  ******************************************************************************
  * @file    git-functionlist-fwupdate.c
  * @brief   GDS BLE Protocol — FW Update 핸들러 (FuncID 0xA1~0xA5)
  *
  *  git-functionlist.c 의 다른 핸들러들과 동일 시그니처:
  *    void FL_GDS_xxx(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
  *
  *  배열 등록(g_Functions_GDS[])과 forward declaration은 git-functionlist.c 에서.
  *
  *  ★ 간소화된 앱 프로토콜 (검증·적용은 장치가 자체 수행):
  *    0xA1 START   : payload 공란 → 기본 파일명/app_no 로 시작 (제어만)
  *                   (하위호환: [u8 app_no][u8 fname_len][char fname[]] 도 허용)
  *    0xA2 RECV    : 수신 포맷은 OTA_RECV_HEX 로 선택 (fw_update.h)
  *                   · ==1 (기본) Intel-HEX 라인 1개(ASCII) — tools/bin2hex.py 산출 .hex
  *                                ":LLAAAATTDD..CC" (112B/라인 → 235자, BLE 프레임 243B.
  *                                라인 크기는 ATT payload(=MTU-3) 이내여야 함). 라인 체크섬 검증 후
  *                                type-00 데이터만 fw_update_recv() 로 투입(순차 concat).
  *                   · ==0        바이너리 청크 ≤512B — tools/fw_sign.py 산출 .bin
  *                   어느 쪽이든 재조립 스트림 선행 64B = 내장 헤더 → 검증 로직 동일
  *    0xA3 END     : payload 공란 → 장치가 내장 헤더로 크기/CRC 검증 후
  *                   ★ 자체 적용(FirmwareInfo.ini 갱신 + COPY_PENDING + reset)까지 수행.
  *                   앱은 0xA5 를 보낼 필요 없음.
  *                   (하위호환: [u32 size][u32 crc32] 를 보내면 헤더와 교차검증)
  *    0xA4 CHECK   : [DEPRECATED] 검증은 0xA3 END 로 병합
  *    0xA5 SETLIST : [u8 boot_app_no][u8 ver×3] — 이제 선택(END 가 이미 적용). 수동 적용용 유지.
  *
  *  검증 기준(size/crc/version)은 이미지에 내장된 헤더(fw_image_header_t, tools/fw_sign.py)에서 취득.
  *  ACK/NAK 모두 페이로드 없이 GITPACKET_send_response 사용.
  ******************************************************************************
  */

#include "../Inc/sys-common.h"
#include "../Inc/git-protocol.h"
#include "../Inc/git-functionlist.h"   /* GITPACKET_ACK / GITPACKET_NAK 매크로 */
#include "../Inc/sys-emmc.h"           /* DeviceSerial_* + GDS_FID_*_SERIAL */
#include "../Inc/git-ble.h" /* BTSetLocalDeviceNameReq */
#include "fw_update.h"                 /* Common/Inc — fw_update_* API */

/* 0xA2 HEX 수신 상태 초기화 (OTA_RECV_HEX==0 이면 no-op). 정의는 0xA2 섹션. */
static void hex_rx_reset(void);

/*============================================================================
 *  0xA1 — FL_GDS_FwUpdate_Start
 *===========================================================================*/

void FL_GDS_FwUpdate_Start(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)uiInCommType;

    uint8_t *p = (uint8_t *)pInterPtcl;

    /* (1) SystemState 검증 — IDLE 외에는 거부 (Plan §8) */
    if (g_system_state != eSYSTEM_STATE_IDLE) {
        printf("[A1] NAK: state=%d (need IDLE=%d)\r\n",
               (int)g_system_state, (int)eSYSTEM_STATE_IDLE);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_START, GITPACKET_NAK);
        return;
    }

    /* (2) Payload 파싱 — 이제 공란(제어 전용) 허용.
     *   · 공란(len<2)          : 기본 파일명/app_no 사용. app_no·버전은 END 에서
     *                            내장 헤더값으로 최종 반영됨. (EVCT/GLT 와 유사한 단순 시작)
     *   · [app_no][fname_len][fname] : 하위호환 — 명시 파일명 사용 */
    uint8_t app_no = OTA_DEFAULT_APP_NO;
    char    filename[64];
    strncpy(filename, OTA_DEFAULT_FILENAME, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    if (uiLength >= 2U && p != NULL) {
        uint8_t in_app  = p[0];
        uint8_t fn_len  = p[1];
        if (fn_len == 0U || fn_len > 63U || uiLength < (2U + fn_len)) {
            printf("[A1] NAK: fname_len=%u, uiLength=%lu (need >=%u)\r\n", (unsigned)fn_len, (unsigned long)uiLength, (unsigned)(2U + fn_len));
            GITPACKET_send_response(GDS_FID_FW_UPDATE_START, GITPACKET_NAK);
            return;
        }
        app_no = in_app;
        memcpy(filename, &p[2], fn_len);
        filename[fn_len] = '\0';
    }

    /* (3) fw_update_start 호출 (HEX 수신 상태도 함께 초기화) */
    hex_rx_reset();
    HAL_StatusTypeDef rc = fw_update_start(app_no, filename);
    if (rc != HAL_OK) {
        printf("[A1] NAK: fw_update_start failed (rc=%d, app=%u, file=%s)\r\n", (int)rc, (unsigned)app_no, filename);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_START, GITPACKET_NAK);
        return;
    }

    printf("[A1] ACK: app=%u file=%s\r\n", (unsigned)app_no, filename);
    GITPACKET_send_response(GDS_FID_FW_UPDATE_START, GITPACKET_ACK);
}

/*============================================================================
 *  0xA2 — FL_GDS_FwUpdate_Recv
 *===========================================================================*/

#if (OTA_RECV_HEX == 1)

/*----------------------------------------------------------------------------
 *  Intel-HEX 라인 디코드 계층 (OTA_RECV_HEX == 1)
 *
 *  payload = ASCII HEX 라인 1개  ":LL AAAA TT DD..DD CC"  (CR/LF 유무 무관)
 *  디코드하여 type-00 의 데이터 바이트(DD)만 fw_update_recv() 에 투입한다.
 *  → 헤더 분리 / CRC32 누적 / eMMC 저장 / END 검증 로직은 전부 그대로 재사용.
 *
 *  주소 처리: 순차(concat) 방식. bin2hex.py 가 0 번지부터 연속 생성하므로
 *            수신 주소가 누적 오프셋과 일치해야 한다(유실/순서오류 즉시 검출).
 *            · addr == expected  → 정상 기록
 *            · addr <  expected  → 이미 받은 구간(ACK 유실 후 재전송) → 기록 없이 ACK
 *            · addr >  expected  → 라인 유실(gap) → NAK
 *---------------------------------------------------------------------------*/

static uint32_t s_hex_base   = 0U;   /* type-04 확장 선형 주소 상위 16bit << 16 */
static uint32_t s_hex_expect = 0U;   /* 다음에 와야 할 누적 오프셋 */

static void hex_rx_reset(void)
{
    s_hex_base   = 0U;
    s_hex_expect = 0U;
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
    return -1;
}

/* ASCII HEX 라인 → 바이트 배열. 반환 = 디코드된 바이트 수(비16진 문자에서 종료). */
static int hex_decode_line(const char *s, uint32_t len, uint8_t *out, int cap)
{
    uint32_t i  = 0U;
    int      oi = 0;

    if (len > 0U && s[0] == ':') i = 1U;      /* 시작 ':' 스킵 */

    while ((i + 1U) < len && oi < cap) {
        int hi = hexval(s[i]);
        int lo = hexval(s[i + 1U]);
        if (hi < 0 || lo < 0) break;          /* CR/LF 등 → 정상 종료 */
        out[oi++] = (uint8_t)((hi << 4) | lo);
        i += 2U;
    }
    return oi;
}

void FL_GDS_FwUpdate_Recv(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)uiInCommType;

    const char *s = (const char *)pInterPtcl;

    if (s == NULL || uiLength == 0U) {
        GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
        return;
    }

    /* (1) 라인 디코드 */
    uint8_t rec[OTA_HEX_REC_MAX];
    int rn = hex_decode_line(s, uiLength, rec, (int)sizeof(rec));
    if (rn < 5) {                                       /* 최소 LL+addr(2)+TT+CC */
        printf("[A2] NAK: hex decode too short (rn=%d)\r\n", rn);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
        return;
    }

    uint8_t ll   = rec[0];
    uint8_t type = rec[3];

    /* (2) 길이 일관성: 전체 = LL + [LL,addr(2),TT,CC] */
    if (rn != (int)ll + 5) {
        printf("[A2] NAK: len mismatch (LL=%u rn=%d)\r\n", (unsigned)ll, rn);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
        return;
    }

    /* (3) Intel-HEX 라인 체크섬: 전체 바이트 합(CC 포함) & 0xFF == 0 */
    uint8_t sum = 0U;
    for (int i = 0; i < rn; i++) sum = (uint8_t)(sum + rec[i]);
    if (sum != 0U) {
        printf("[A2] NAK: line checksum (sum=0x%02X)\r\n", (unsigned)sum);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
        return;
    }

    /* (4) 레코드 타입별 처리 */
    switch (type) {
    case 0x00: {                                        /* 데이터 */
        uint32_t addr = s_hex_base + ((uint32_t)rec[1] << 8) + (uint32_t)rec[2];

        if (addr > s_hex_expect) {                      /* gap = 라인 유실 */
            printf("[A2] NAK: gap addr=0x%08lX expect=0x%08lX\r\n",
                   (unsigned long)addr, (unsigned long)s_hex_expect);
            GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
            return;
        }
        if ((addr + ll) <= s_hex_expect) {              /* 중복(ACK 유실 재전송) */
            printf("[A2] dup skip addr=0x%08lX\r\n", (unsigned long)addr);
            GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_ACK);
            return;
        }
        if (addr != s_hex_expect) {                     /* 부분 겹침 — 처리 불가 */
            printf("[A2] NAK: overlap addr=0x%08lX expect=0x%08lX\r\n",
                   (unsigned long)addr, (unsigned long)s_hex_expect);
            GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
            return;
        }

        if (fw_update_recv(&rec[4], (uint32_t)ll) != HAL_OK) {
            GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
            return;
        }
        s_hex_expect += ll;
        break;
    }

    case 0x04:                                          /* 확장 선형 주소(상위 16bit) */
        if (ll != 2U) {
            GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
            return;
        }
        s_hex_base = ((uint32_t)(((uint32_t)rec[4] << 8) | rec[5])) << 16;
        break;

    case 0x02:                                          /* 확장 세그먼트 주소 */
        if (ll != 2U) {
            GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
            return;
        }
        s_hex_base = ((uint32_t)(((uint32_t)rec[4] << 8) | rec[5])) << 4;
        break;

    case 0x01:                                          /* EOF — 종료는 0xA3 가 담당 */
    case 0x03:                                          /* 시작 세그먼트 주소 */
    case 0x05:                                          /* 시작 선형 주소 */
        break;

    default:
        printf("[A2] NAK: unknown record type 0x%02X\r\n", (unsigned)type);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
        return;
    }

    GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_ACK);
}

#else  /* OTA_RECV_HEX == 0 — 바이너리 청크 경로 (기존) */

static void hex_rx_reset(void) { }   /* no-op — START 공통 호출용 */

void FL_GDS_FwUpdate_Recv(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)uiInCommType;

    uint8_t *p = (uint8_t *)pInterPtcl;

    if (p == NULL || uiLength == 0U) {
        GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
        return;
    }

    if (fw_update_recv(p, uiLength) != HAL_OK) {
        GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_NAK);
        return;
    }

    GITPACKET_send_response(GDS_FID_FW_UPDATE_RECV, GITPACKET_ACK);
}

#endif /* OTA_RECV_HEX */

/*============================================================================
 *  0xA3 — FL_GDS_FwUpdate_End
 *===========================================================================*/

void FL_GDS_FwUpdate_End(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)uiInCommType;

    /* payload(선택): [u32 expected_size][u32 expected_crc32]  (8B, LE)
     * 이제 검증 기준은 이미지에 내장된 헤더(fw_image_header_t)다.
     *  · payload 공란(len<8)  : 내장 헤더값으로만 검증
     *  · payload 8B 이상       : 앱 전달 size/crc 를 내장 헤더와 교차검증(불일치 시 거부)
     * 0 은 "검증 생략(헤더 사용)" 의미로 fw_update_end 에 전달. */
    uint32_t expected_size = 0U;
    uint32_t expected_crc  = 0U;
    if (uiLength >= 8U && pInterPtcl != NULL) {
        uint8_t *p = (uint8_t *)pInterPtcl;
        expected_size = (uint32_t)p[0]
                      | ((uint32_t)p[1] << 8)
                      | ((uint32_t)p[2] << 16)
                      | ((uint32_t)p[3] << 24);
        expected_crc  = (uint32_t)p[4]
                      | ((uint32_t)p[5] << 8)
                      | ((uint32_t)p[6] << 16)
                      | ((uint32_t)p[7] << 24);
    }

    /* ── (1) 검증 + 확정 이동(rename) ─────────────────────────────────── */
    if (fw_update_end(expected_size, expected_crc) != HAL_OK) {
        GITPACKET_send_response(GDS_FID_FW_UPDATE_END, GITPACKET_NAK);
        return;
    }

    /* ── (2) 적용 준비: FirmwareInfo.ini 갱신 + fw_flag=COPY_PENDING ────
     *   앱이 별도 0xA5 를 보낼 필요 없음. 버전/app_no 는 내장 헤더에서 취득.
     *   ★ 리셋 전에 "모든 처리"를 여기서 끝내고 성공 여부를 확인한다. */
    const fw_image_header_t *hdr = fw_update_get_header();
    uint8_t apply_app = OTA_DEFAULT_APP_NO;
    uint8_t apply_ver[3] = { 0, 0, 0 };
    if (hdr != NULL) {
        if (hdr->app_no != 0U && hdr->app_no < (uint8_t)eApp_MAX) {
            apply_app = hdr->app_no;
        }
        apply_ver[0] = hdr->fw_version[0];
        apply_ver[1] = hdr->fw_version[1];
        apply_ver[2] = hdr->fw_version[2];
    }

    if (fw_update_set_list_no_reset(apply_app, apply_ver) != HAL_OK) {
        /* ini 쓰기 실패 등 — 적용 불가. 조용히 미적용되지 않도록 NAK 로 알린다. */
        printf("[A3] NAK: set_list failed (app=%u) — not applied\r\n",
               (unsigned)apply_app);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_END, GITPACKET_NAK);
        return;
    }

    printf("[A3] verified + armed — app=%u v%u.%u.%u, ACK then reset\r\n",
           (unsigned)apply_app, (unsigned)apply_ver[0],
           (unsigned)apply_ver[1], (unsigned)apply_ver[2]);

    /* ── (3) 모든 처리 완료 → 응답 송신 (리셋 직전) ───────────────────── */
    GITPACKET_send_response(GDS_FID_FW_UPDATE_END, GITPACKET_ACK);

    /* ── (4) 응답이 호스트에 닿을 시간 확보 후 리셋 (미복귀) ──────────── */
    HAL_Delay(100);
    HAL_NVIC_SystemReset();
}

/*============================================================================
 *  0xA4 — FL_GDS_FwUpdate_Check  → [재활용] Get App FW Version
 *    구 CRC 검증 기능은 0xA3 END 로 병합되어 미사용이었으므로, 이 FuncID 를
 *    "현재 실행 펌웨어 버전 조회" 용도로 재활용한다.
 *
 *    요청 : payload 무시(공란 가능)
 *    응답 : [u8 major][u8 minor]   (2바이트, 저장소 FirmwareInfo.ini 에서 읽음)
 *           patch 는 생략 — 필요 시 3바이트로 확장 가능.
 *===========================================================================*/

void FL_GDS_FwUpdate_Check(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)pInterPtcl;
    (void)uiInCommType;
    (void)uiLength;

    /* 저장소에서 현재 실행 버전 취득(캐시/lazy 로드). 실패해도 0.0 반환. */
    uint8_t ver[3] = { 0, 0, 0 };
    HAL_StatusTypeDef rc = fw_update_get_running_version(ver);

    /* 응답 payload = [major][minor] (2바이트) */
    uint8_t payload[2] = { ver[0], ver[1] };

    uint8_t frame[16];
    int n = GITPACKET_make_frame(GDS_FID_FW_UPDATE_CHECK, payload, 2U, frame);
    if (n > 0) {
        GITPACKET_send_frame_via_uart(frame, (uint16_t)n);
        printf("[A4] app version %u.%u (%s)\r\n",
               (unsigned)ver[0], (unsigned)ver[1],
               (rc == HAL_OK) ? "from storage" : "default/unavailable");
    }
}

/*============================================================================
 *  0xA5 — FL_GDS_FwUpdate_SetList  (FirmwareInfo 갱신 + COPY_PENDING + reset)
 *
 *  ⚠ Step 6 단계: 핸들러 등록만 (Boot 측 staging cascade는 Step 7에서 활성화).
 *    현재는 fw_update_set_list 가 fw_flag=COPY_PENDING + reset 만 수행.
 *    Boot main.c 의 Boot_FwUpdateCheck() 는 FW_UPDATE_ENABLED 매크로 gate 상태이므로
 *    재부팅 후 정상 부팅 경로(BOOT_Application)로 그대로 통과 — 무손상.
 *    Step 7에서 FW_UPDATE_ENABLED 매크로 활성화 + ICF 시프트로 정식 동작.
 *===========================================================================*/

void FL_GDS_FwUpdate_SetList(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)uiInCommType;

    if (uiLength < 4U) {
        GITPACKET_send_response(GDS_FID_FW_UPDATE_SET_LIST, GITPACKET_NAK);
        return;
    }

    uint8_t *p = (uint8_t *)pInterPtcl;
    uint8_t boot_app_no = p[0];
    uint8_t ver[3]      = { p[1], p[2], p[3] };

    /* 앱이 0(미지정)을 보내면 방금 수신한 이미지의 내장 헤더값을 사용.
     * → 버전/app_no 의 단일 진실 출처를 "이미지 헤더"로 통일. */
    const fw_image_header_t *hdr = fw_update_get_header();
    if (hdr != NULL) {
        if (boot_app_no == 0U) boot_app_no = hdr->app_no;
        if (ver[0] == 0U && ver[1] == 0U && ver[2] == 0U) {
            ver[0] = hdr->fw_version[0];
            ver[1] = hdr->fw_version[1];
            ver[2] = hdr->fw_version[2];
        }
    }

    /* (1) 모든 처리 먼저 — ini 갱신 + fw_flag=COPY_PENDING (리셋 없음) */
    if (fw_update_set_list_no_reset(boot_app_no, ver) != HAL_OK) {
        printf("[A5] NAK: set_list failed (app=%u) — not applied\r\n",
               (unsigned)boot_app_no);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_SET_LIST, GITPACKET_NAK);
        return;
    }

    /* (2) 처리 완료 → 응답 송신 (리셋 직전) */
    GITPACKET_send_response(GDS_FID_FW_UPDATE_SET_LIST, GITPACKET_ACK);

    /* (3) 응답 전달 시간 확보 후 리셋 — 미복귀 */
    HAL_Delay(100);
    HAL_NVIC_SystemReset();
}

/*============================================================================
 *  0xA6 — FL_GDS_FwUpdate_ForceApply  [TEST ONLY]
 *
 *  eMMC 01_Application 에 이미 존재하는 FW 를 즉시 적용하는 테스트 커맨드.
 *  OTA 다운로드 시퀀스(0xA1~0xA3) 없이, eMMC 파일을 바로 Flash 에 반영.
 *
 *  Payload (≤36 B):
 *    [u8 boot_app_no][u8 ver_major][u8 ver_minor][u8 ver_patch]
 *    [u8 fname_len][char fname[fname_len]]
 *
 *  동작:
 *    1. eMMC 01_Application/<fname> 존재 확인 (f_stat)
 *    2. FirmwareInfo.ini 갱신 (파일명 + 버전 + mucBootMode)
 *    3. fw_flag = COPY_PENDING
 *    4. ACK 전송 후 SystemReset
 *    → Boot 이 COPY_PENDING 감지 → eMMC→Slot1→Slot0 복사 → 새 FW 실행
 *===========================================================================*/

void FL_GDS_FwUpdate_ForceApply(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)uiInCommType;

    /* (1) 최소 페이로드: app_no(1) + ver(3) + fname_len(1) + fname(1+) = 6B */
    if (uiLength < 6U) {
        printf("[A6] NAK: payload too short (len=%lu, need >= 6)\r\n",
               (unsigned long)uiLength);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_FORCE, GITPACKET_NAK);
        return;
    }

    uint8_t *p = (uint8_t *)pInterPtcl;
    uint8_t boot_app_no = p[0];
    uint8_t ver[3]      = { p[1], p[2], p[3] };
    uint8_t fname_len   = p[4];

    if (fname_len == 0U || fname_len > 32U || uiLength < (5U + fname_len)) {
        printf("[A6] NAK: fname_len=%u invalid\r\n", (unsigned)fname_len);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_FORCE, GITPACKET_NAK);
        return;
    }
    if (boot_app_no == 0U || boot_app_no >= eApp_MAX) {
        printf("[A6] NAK: app_no=%u out of range\r\n", (unsigned)boot_app_no);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_FORCE, GITPACKET_NAK);
        return;
    }

    char filename[40];
    memcpy(filename, &p[5], fname_len);
    filename[fname_len] = '\0';

    /* (2) eMMC 파일 존재 확인 */
    char path[80];
    snprintf(path, sizeof(path), "01_Application/%s", filename);

    FILINFO fi_stat;
    if (f_stat(path, &fi_stat) != FR_OK || fi_stat.fsize == 0U) {
        printf("[A6] NAK: file not found — %s\r\n", path);
        GITPACKET_send_response(GDS_FID_FW_UPDATE_FORCE, GITPACKET_NAK);
        return;
    }
    printf("[A6] found: %s (%lu bytes)\r\n", path, (unsigned long)fi_stat.fsize);

    /* (3) FirmwareInfo.ini 갱신 */
    SFwInfo fw_info;
    memset(&fw_info, 0, sizeof(fw_info));
    fw_info.muiPreamble    = VCI3_FWINFO_PREAMBLE;
    fw_info.mucInitialized = 0xAAU;

    /* 기존 ini 가 있으면 읽어서 보존 */
    FIL fp;
    UINT br, bw;
    if (f_open(&fp, BOOT_FWINFO_FILE, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
        f_read(&fp, &fw_info, sizeof(fw_info), &br);
        f_close(&fp);
        if (fw_info.muiPreamble != VCI3_FWINFO_PREAMBLE) {
            memset(&fw_info, 0, sizeof(fw_info));
            fw_info.muiPreamble    = VCI3_FWINFO_PREAMBLE;
            fw_info.mucInitialized = 0xAAU;
        }
    }

    fw_info.mucBootMode = boot_app_no;
    if (boot_app_no < MAX_APP_CNT + 1U) {
        fw_info.msAppInfo[boot_app_no - 1U].marrucVersion[0] = ver[0];
        fw_info.msAppInfo[boot_app_no - 1U].marrucVersion[1] = ver[1];
        fw_info.msAppInfo[boot_app_no - 1U].marrucVersion[2] = ver[2];
        strncpy(fw_info.msAppInfo[boot_app_no - 1U].marrcFilename,
                filename,
                sizeof(fw_info.msAppInfo[boot_app_no - 1U].marrcFilename) - 1);
    }

    if (f_open(&fp, BOOT_FWINFO_FILE, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        printf("[A6] NAK: FirmwareInfo.ini write fail\r\n");
        GITPACKET_send_response(GDS_FID_FW_UPDATE_FORCE, GITPACKET_NAK);
        return;
    }
    f_write(&fp, &fw_info, sizeof(fw_info), &bw);
    f_sync(&fp);       /* FAT/디렉토리 엔트리를 eMMC에 즉시 flush — reset 전 필수 */
    f_close(&fp);

    printf("[A6] FirmwareInfo.ini written (%u bytes)\r\n", (unsigned)bw);

    /* (4) fw_flag = COPY_PENDING — Boot 이 eMMC→Slot1→Slot0 수행 */
    fw_set_flag(FW_FLAG_COPY_PENDING);

    printf("[A6] ACK: force apply %s v%u.%u.%u — reset in 100ms\r\n",
           filename, (unsigned)ver[0], (unsigned)ver[1], (unsigned)ver[2]);

    /* ACK 먼저 전송 — reset 후에는 응답 불가 */
    GITPACKET_send_response(GDS_FID_FW_UPDATE_FORCE, GITPACKET_ACK);

    HAL_Delay(100);
    HAL_NVIC_SystemReset();
    /* unreachable */
}

/*============================================================================
 *  0xB1 — FL_GDS_SetSerial  (디바이스 시리얼 설정 — eMMC 저장, 재설정 허용)
 *    payload: [char suffix[8]]  (8자리 숫자, 예 "00000123")
 *    성공 시:
 *      · eMMC(DeviceSerial.bin) + RAM 캐시에 즉시 반영 (GetSerial 즉시 반영)
 *      · BLE 광고 이름 "BDC<suffix>" 반영:
 *          - 연결 중 : UART2가 데이터 파이프라 AT+BTNAME이 모듈에 안 먹음.
 *                      → 대기 플래그만 세팅(BTMarkDeviceNameDirty). 연결 해제
 *                        edge(명령 모드)에서 git-comm이 BTApplyPendingDeviceName()
 *                        호출 → AT+BTNAME 전송 → 재광고 시 새 이름 broadcast.
 *          - 미연결 : 즉시 BTSetLocalDeviceNameReq()로 반영(부팅/CLI 경로).
 *    write-once 제거 — 언제든 덮어쓰기(재설정) 가능.
 *===========================================================================*/

void FL_GDS_SetSerial(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)uiInCommType;

    if (pInterPtcl == NULL || uiLength < DEVSERIAL_SUFFIX_LEN) {
        printf("[B1] NAK: payload len=%lu (need %u)\r\n",
               (unsigned long)uiLength, (unsigned)DEVSERIAL_SUFFIX_LEN);
        GITPACKET_send_response(GDS_FID_SET_SERIAL, GITPACKET_NAK);
        return;
    }

    char suffix[DEVSERIAL_SUFFIX_LEN + 1];
    memcpy(suffix, pInterPtcl, DEVSERIAL_SUFFIX_LEN);
    suffix[DEVSERIAL_SUFFIX_LEN] = '\0';

    int rc = DeviceSerial_Set(suffix);
    if (rc != 0) {
        printf("[B1] NAK: DeviceSerial_Set rc=%d (-2=format -3=io)\r\n", rc);
        GITPACKET_send_response(GDS_FID_SET_SERIAL, GITPACKET_NAK);
        return;
    }

    /* BLE 광고 이름 반영: 연결 중이면 대기(연결 해제 시 적용), 미연결이면 즉시 */
    if (BTGetConnectStatus()) {
        BTMarkDeviceNameDirty();
        printf("[B1] ACK: serial saved -> BDC%s (BLE name applies on disconnect)\r\n", suffix);
    } else {
        BTSetLocalDeviceNameReq();   /* 명령 모드 — 즉시 반영 */
        printf("[B1] ACK: serial set -> BDC%s (BLE name applied)\r\n", suffix);
    }
    GITPACKET_send_response(GDS_FID_SET_SERIAL, GITPACKET_ACK);
}

/*============================================================================
 *  0xB2 — FL_GDS_GetSerial  (현재 디바이스 시리얼 조회)
 *    응답 payload: [char suffix[8]]
 *===========================================================================*/

void FL_GDS_GetSerial(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)pInterPtcl;
    (void)uiInCommType;
    (void)uiLength;

    const char *suffix = DeviceSerial_Get();

    uint8_t frame[32];
    int n = GITPACKET_make_frame(GDS_FID_GET_SERIAL,
                                 (uint8_t *)suffix, DEVSERIAL_SUFFIX_LEN, frame);
    if (n > 0) {
        GITPACKET_send_frame_via_uart(frame, (uint16_t)n);
        printf("[B2] sent serial=BDC%s\r\n", suffix);
    } else {
        GITPACKET_send_response(GDS_FID_GET_SERIAL, GITPACKET_NAK);
    }
}
