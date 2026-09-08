/**
 * ******************************************************************************
 * @file    test-blebulk.c
 * @brief   [TEST ONLY] BLE 대용량(512B) 전송 테스트 — FuncID 0xC1
 *
 *  ※ 전부 테스트 코드. 제거/비활성 절차는 test-blebulk.h 헤더 주석 참조.
 *
 *  구성:
 *    1) FL_GDS_Test_BulkRecv   0xC1 수신 → 로그 + (옵션) eMMC 저장 → 항상 ACK
 *    2) eMMC 저장 제어          create / start / stop / stat
 *    3) CLI 커맨드              ble bulk ...
 *
 *  CLI 사용법:
 *    ble bulk tx [len]            0xC1 프레임을 UART2(BLE)로 송신 — BT 연결 시에만 허용
 *    ble bulk rx [len]            0xC1 프레임을 로컬에서 파싱 + 핸들러 직접 호출
 *                                 (공유 RX 큐를 거치지 않으므로 AT 응답 스트림 무영향)
 *    ble bulk file create <path>  eMMC 에 빈 파일 생성 (기존 내용 truncate)
 *    ble bulk file start [path]   0xC1 수신 데이터 저장 시작 (append, 한 파일에 누적)
 *    ble bulk file stop           저장 종료 (f_sync + f_close)
 *    ble bulk file stat           현재 상태 조회
 *
 *  전송 프레임 예 (payload 512B):
 *    A0 04 02 C1 <512B payload> <CRC16 2B> B0     (Len = 1+512+2+1 = 0x0204 LE)
 * ******************************************************************************
 */

#include "../Inc/test-blebulk.h"

#if BLE_BULK_TEST_ENABLED

#include "../Inc/git-protocol.h"
#include "../Inc/git-functionlist.h"   /* GITPACKET_ACK / GITPACKET_PAYLOAD_LEN_MAX */
#include "../Inc/git-ble.h"            /* BTGetConnectStatus */
#include "../Inc/sys-emmc.h"           /* FATFS + getMountStatus + EMMC_PATH_MAX */
#include <string.h>
#include <stdlib.h>

/* Define ---------------------------------------------------------------*/
#define BULK_LOG(fmt, ...)  printf("\033[32m" fmt "\033[0m", ##__VA_ARGS__)

#define TEST_BULK_MAX_LEN       GITPACKET_PAYLOAD_LEN_MAX  /* 512 — 프로토콜 상한 */
#define TEST_BULK_DUMP_BYTES    16      /* head/tail hex dump 바이트 수 */
#define TEST_BULK_SESSION_GAP   3000    /* 이 시간(ms) 이상 무수신이면 카운터 리셋 */
#define TEST_BULK_SYNC_FRAMES   32      /* N 프레임마다 f_sync — stop 누락 시 손실 최소화 */

/* Private prototypes ---------------------------------------------------*/
static void TestBulk_WriteToFile(const uint8_t *data, uint32_t len);
static void TestBulk_Frame(uint16_t len, bool loopback);
static int  TestBulk_FileCreate(const char *path);
static int  TestBulk_FileStart (const char *path);
static int  TestBulk_FileStop  (void);
static void TestBulk_FileStat  (void);

/*============================================================================
 *  eMMC 저장 컨텍스트
 *
 *  컨텍스트 주의: create/start/stop/stat 은 cliTask, 실제 f_write 는 commTask.
 *  두 태스크가 같은 FIL 을 만지므로 mutex 로 직렬화한다.
 *  (FF_FS_REENTRANT=1 이라 FatFs 내부는 보호되지만, "active 확인 + f_write" 와
 *   "f_close" 사이의 경합은 FatFs 가 막아주지 않음)
 *===========================================================================*/

static struct {
    volatile uint8_t active;             /* 1 = 수신 데이터를 파일에 기록 중 */
    FIL              fp;
    char             path[EMMC_PATH_MAX];
    uint32_t         frames;             /* 이번 구간에 기록한 프레임 수 */
    uint32_t         bytes;              /* 이번 구간에 기록한 바이트 수 */
    uint32_t         err_cnt;            /* f_write 실패 횟수 */
} s_rec = {0};

static osMutexId_t s_rec_mutex = NULL;

/* cliTask 에서만 호출됨 → 생성 경합 없음 */
static void TestBulk_LockInit(void)
{
    if (s_rec_mutex == NULL) {
        s_rec_mutex = osMutexNew(NULL);
    }
}

static inline void TestBulk_Lock(void)
{
    if (s_rec_mutex != NULL) osMutexAcquire(s_rec_mutex, osWaitForever);
}

static inline void TestBulk_Unlock(void)
{
    if (s_rec_mutex != NULL) osMutexRelease(s_rec_mutex);
}

/**
 * @brief  파일 생성 — 지정 경로에 빈 파일을 만들고 즉시 닫는다. 저장은 시작하지 않음.
 * @retval 0=성공, -1=마운트 안 됨, -2=인자 오류, -3=이미 저장 중, -4=f_open 실패
 */
static int TestBulk_FileCreate(const char *path)
{
    if (path == NULL || path[0] == '\0' || strlen(path) >= EMMC_PATH_MAX) return -2;
    if (getMountStatus() == 0) return -1;

    TestBulk_LockInit();
    TestBulk_Lock();

    if (s_rec.active) {          /* 저장 중 truncate 는 금지 */
        TestBulk_Unlock();
        return -3;
    }

    FIL     fp;
    FRESULT rc = f_open(&fp, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (rc != FR_OK) {
        TestBulk_Unlock();
        printf("[0xC1 FILE] create failed: %s (FRESULT=%d)\r\n", path, (int)rc);
        return -4;
    }
    f_close(&fp);

    strncpy(s_rec.path, path, sizeof(s_rec.path) - 1);
    s_rec.path[sizeof(s_rec.path) - 1] = '\0';

    TestBulk_Unlock();
    printf("[0xC1 FILE] created: %s (0 bytes)\r\n", s_rec.path);
    return 0;
}

/**
 * @brief  저장 시작 — append 모드로 열고 0xC1 수신 데이터를 기록 시작.
 * @param  path  NULL 이면 create 로 지정한 경로 사용. 파일이 없으면 새로 만든다.
 * @retval 0=성공, -1=마운트 안 됨, -2=경로 없음, -3=이미 저장 중, -4=f_open 실패
 */
static int TestBulk_FileStart(const char *path)
{
    if (getMountStatus() == 0) return -1;

    TestBulk_LockInit();
    TestBulk_Lock();

    if (s_rec.active) {
        TestBulk_Unlock();
        return -3;
    }

    if (path != NULL && path[0] != '\0') {
        if (strlen(path) >= EMMC_PATH_MAX) { TestBulk_Unlock(); return -2; }
        strncpy(s_rec.path, path, sizeof(s_rec.path) - 1);
        s_rec.path[sizeof(s_rec.path) - 1] = '\0';
    }
    if (s_rec.path[0] == '\0') {
        TestBulk_Unlock();
        return -2;
    }

    /* OPEN_ALWAYS + f_lseek(끝) = append. 한 파일에 계속 누적된다. */
    FRESULT rc = f_open(&s_rec.fp, s_rec.path, FA_OPEN_ALWAYS | FA_WRITE);
    if (rc != FR_OK) {
        TestBulk_Unlock();
        printf("[0xC1 FILE] open failed: %s (FRESULT=%d)\r\n", s_rec.path, (int)rc);
        return -4;
    }
    (void)f_lseek(&s_rec.fp, f_size(&s_rec.fp));

    s_rec.frames  = 0;
    s_rec.bytes   = 0;
    s_rec.err_cnt = 0;
    s_rec.active  = 1;           /* ★ 파일 열기 완료 후 마지막에 set */

    TestBulk_Unlock();
    printf("[0xC1 FILE] recording START -> %s (offset=%lu)\r\n",
           s_rec.path, (unsigned long)f_size(&s_rec.fp));
    return 0;
}

/**
 * @brief  저장 종료 — f_sync + f_close 후 기록 통계 출력.
 * @retval 0=성공, -1=저장 중이 아님
 */
static int TestBulk_FileStop(void)
{
    TestBulk_Lock();

    if (!s_rec.active) {
        TestBulk_Unlock();
        return -1;
    }

    s_rec.active = 0;            /* ★ 먼저 clear → 이후 수신분은 기록 안 됨 */
    f_sync(&s_rec.fp);
    FSIZE_t total = f_size(&s_rec.fp);
    f_close(&s_rec.fp);

    uint32_t frames = s_rec.frames;
    uint32_t bytes  = s_rec.bytes;
    uint32_t errs   = s_rec.err_cnt;

    TestBulk_Unlock();

    printf("[0xC1 FILE] recording STOP  -> %s\r\n", s_rec.path);
    printf("[0xC1 FILE] frames=%lu written=%luB file_size=%luB write_err=%lu\r\n",
           (unsigned long)frames, (unsigned long)bytes,
           (unsigned long)total,  (unsigned long)errs);
    return 0;
}

/**
 * @brief  현재 저장 상태 출력
 */
static void TestBulk_FileStat(void)
{
    printf("[0xC1 FILE] mounted=%u active=%u path=\"%s\"\r\n",
           (unsigned)getMountStatus(),
           (unsigned)s_rec.active,
           (s_rec.path[0] != '\0') ? s_rec.path : "(none)");
    printf("[0xC1 FILE] frames=%lu written=%luB write_err=%lu\r\n",
           (unsigned long)s_rec.frames,
           (unsigned long)s_rec.bytes,
           (unsigned long)s_rec.err_cnt);
}

/**
 * @brief  0xC1 payload 를 파일에 append. 저장 중이 아니면 아무것도 하지 않음.
 * @note   commTask 컨텍스트. 실패해도 응답은 항상 ACK — 로그로만 알림.
 */
static void TestBulk_WriteToFile(const uint8_t *data, uint32_t len)
{
    if (!s_rec.active || data == NULL || len == 0U) return;

    TestBulk_Lock();

    /* lock 획득 대기 중 stop 이 걸렸을 수 있음 — 재확인 */
    if (!s_rec.active) {
        TestBulk_Unlock();
        return;
    }

    UINT    bw = 0;
    FRESULT rc = f_write(&s_rec.fp, data, len, &bw);

    if (rc != FR_OK || bw != len) {
        s_rec.err_cnt++;
        printf("[0xC1 FILE] write FAIL (FRESULT=%d, %u/%lu bytes, err_cnt=%lu)\r\n",
               (int)rc, (unsigned)bw, (unsigned long)len,
               (unsigned long)s_rec.err_cnt);
    } else {
        s_rec.frames++;
        s_rec.bytes += len;

        /* 주기적 flush — stop 을 안 치고 리셋해도 여기까지는 남는다 */
        if ((s_rec.frames % TEST_BULK_SYNC_FRAMES) == 0U) {
            f_sync(&s_rec.fp);
        }
    }

    TestBulk_Unlock();
}

/*============================================================================
 *  0xC1 수신 핸들러
 *===========================================================================*/

/**
 * @brief  [TEST] BLE FuncID 0xC1: 대용량 전송 수신
 *
 *  RX Payload : 임의 데이터 0~512B (작아도 에러 아님)
 *  Response   : 0xC1 + ACK(0x00) — 길이/내용/저장 결과와 무관하게 항상 ACK
 *
 *  Log 항목:
 *    #n        수신 순번 (TEST_BULK_SESSION_GAP 이상 공백 시 1부터 재시작)
 *    len       이번 프레임 payload 길이 (1~512 모두 정상)
 *    xor       payload 전체 XOR — 앱이 보낸 값과 비교하면 내용 무결성 확인
 *    gap       직전 프레임과의 간격
 *    total     세션 누적 바이트
 *    rate      세션 평균 처리율 (B/s)
 *    head/tail 앞뒤 16바이트 — 오프셋 밀림/중간 잘림 확인용
 */
void FL_GDS_Test_BulkRecv(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    (void)uiInCommType;

    static uint32_t s_count      = 0;   /* 세션 내 수신 프레임 수 */
    static uint32_t s_total      = 0;   /* 세션 내 누적 payload 바이트 */
    static uint32_t s_first_tick = 0;   /* 세션 첫 프레임 tick */
    static uint32_t s_last_tick  = 0;   /* 직전 프레임 tick */

    uint8_t  *pData = (uint8_t*)pInterPtcl;
    uint32_t  now   = HAL_GetTick();

    /* 새 세션 판정 — 첫 수신이거나 일정 시간 이상 공백이면 카운터 리셋 */
    if (s_count == 0 || (now - s_last_tick) > TEST_BULK_SESSION_GAP) {
        s_count      = 0;
        s_total      = 0;
        s_first_tick = now;
        BULK_LOG("[0xC1 BULK] ---- new session ----\r\n");
    }

    s_count++;
    s_total   += uiLength;
    uint32_t gap = (s_count == 1) ? 0 : (now - s_last_tick);
    s_last_tick  = now;

    /* payload 전체 XOR — 앱이 계산한 값과 비교해 내용 무결성 확인 */
    uint8_t xorsum = 0;
    for (uint32_t i = 0; i < uiLength; i++) {
        xorsum ^= pData[i];
    }

    uint32_t elapsed = now - s_first_tick;
    uint32_t rate    = (elapsed > 0) ? ((s_total * 1000U) / elapsed) : 0;

    /* 512B 미만도 정상 — 상한만 표시용으로 체크.
     * 프로토콜 파서가 이미 512B 로 제한하므로 초과는 여기 도달하지 않는다. */
    BULK_LOG("[0xC1 BULK] #%lu len=%lu%s xor=0x%02X gap=%lums total=%luB elapsed=%lums rate=%luB/s\r\n",
             (unsigned long)s_count,
             (unsigned long)uiLength,
             (uiLength > TEST_BULK_MAX_LEN) ? " (>512!)" : "",
             xorsum,
             (unsigned long)gap,
             (unsigned long)s_total,
             (unsigned long)elapsed,
             (unsigned long)rate);

    /* head/tail hex dump — 오프셋 밀림, 중간 잘림 확인용 */
    if (uiLength > 0) {
        uint32_t n = (uiLength < TEST_BULK_DUMP_BYTES) ? uiLength : TEST_BULK_DUMP_BYTES;

        printf("[0xC1 BULK] head:");
        for (uint32_t i = 0; i < n; i++) printf(" %02X", pData[i]);

        if (uiLength > TEST_BULK_DUMP_BYTES) {
            printf("  tail:");
            for (uint32_t i = uiLength - n; i < uiLength; i++) printf(" %02X", pData[i]);
        }
        printf("\r\n");
    } else {
        printf("[0xC1 BULK] head: (empty payload)\r\n");
    }

    /* eMMC 저장 — 'ble bulk file start' 로 활성화된 경우에만 기록 */
    TestBulk_WriteToFile(pData, uiLength);

    /* 응답은 조건 없이 항상 ACK */
    GITPACKET_send_response(TEST_BULK_FUNC_ID, GITPACKET_ACK);
}

/*============================================================================
 *  테스트 프레임 생성 — 송신 / 로컬 파싱
 *===========================================================================*/

/**
 * @brief  0xC1 테스트 프레임 생성 후 송신 또는 로컬 파싱
 *
 * @param  len       payload 길이 (1 ~ 512)
 * @param  loopback  false = UART2(BLE)로 송신 (device -> app)
 *                   true  = 로컬에서 파싱 + 핸들러 직접 호출 (app 없이 수신 경로 검증)
 *
 *  payload 는 0x00,0x01,... 증가 패턴. 512B 일 때 전체 XOR 이 0x00 이 되므로
 *  수신측 로그의 xor 값만 보고 내용 무결성을 바로 판단할 수 있음.
 *
 *  ★ loopback 은 공유 RX 큐(g_ble_rx_q)를 건드리지 않는다.
 *    큐에 주입하면 BT 미연결 상태에서 AT 응답 파서(BLE_ProcessRxQueue)의
 *    스트림을 오염시키고, UART2_Process_GitProtocol 의 no-SOF 처리가
 *    큐 전체를 flush 해서 READY/OK 응답까지 날려버린다.
 */
static void TestBulk_Frame(uint16_t len, bool loopback)
{
    /* cliTask 스택은 4KB — 1KB 를 스택에 얹지 않도록 static 으로 둠
     * (CLI 단일 태스크에서만 호출되므로 재진입 문제 없음) */
    static uint8_t payload[TEST_BULK_MAX_LEN];
    static uint8_t frame[GITPACKET_FRAME_SIZE_MAX];

    if (len == 0 || len > sizeof(payload)) {
        printf("[BLE] bulk: invalid len=%u\r\n", len);
        return;
    }

    for (uint16_t i = 0; i < len; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }

    int fsize = GITPACKET_make_frame(TEST_BULK_FUNC_ID, payload, len, frame);
    if (fsize <= 0) {
        printf("[BLE] bulk: make_frame failed (len=%u, rc=%d)\r\n", len, fsize);
        return;
    }

    if (loopback)
    {
        /* 프레임 파싱(CRC 포함) → 핸들러 직접 호출. RX 큐 미사용. */
        uint8_t  fid     = 0;
        uint8_t *pl      = NULL;
        uint16_t pl_len  = 0;

        if (GITPACKET_parse_frame(frame, (uint16_t)fsize, &fid, &pl, &pl_len) != 0) {
            printf("[BLE] bulk rx: parse FAILED (%d bytes)\r\n", fsize);
            return;
        }

        printf("[BLE] bulk rx: local parse OK %d bytes (fid=0x%02X payload=%u) -> dispatch\r\n",
               fsize, fid, pl_len);
        FL_GDS_Test_BulkRecv(pl, 0, (uint32_t)pl_len);
    }
    else
    {
        /* BT 미연결 상태의 모듈은 AT 커맨드 모드다. 바이너리를 그대로 밀어넣으면
         * 모듈이 이를 명령으로 해석해 상태가 깨질 수 있으므로 차단한다. */
        if (!BTGetConnectStatus()) {
            printf("[BLE] bulk tx: BLOCKED - BT not connected\r\n");
            printf("[BLE] bulk tx: binary TX in AT mode can corrupt the BLE module.\r\n");
            printf("[BLE] bulk tx: connect first, or use 'ble bulk rx' for offline test.\r\n");
            return;
        }

        HAL_StatusTypeDef st = GITPACKET_send_frame_via_uart(frame, (uint16_t)fsize);
        printf("[BLE] bulk tx: %d bytes (payload=%u) uart_st=%d\r\n", fsize, len, st);
    }
}

/*============================================================================
 *  CLI — "ble bulk ..."
 *===========================================================================*/

static void TestBulk_CliUsage(void)
{
    printf("usage: ble bulk tx|rx [len(1..%u)]\r\n", TEST_BULK_MAX_LEN);
    printf("       ble bulk file create <path>|start [path]|stop|stat\r\n");
}

int TestBulk_CliCmd(int argc, char **argv)
{
    if (argc < 3) {
        TestBulk_CliUsage();
        return 1;
    }

    /* --- 파일 저장 제어 --- */
    if (strcmp(argv[2], "file") == 0)
    {
        if (argc < 4) {
            TestBulk_CliUsage();
            return 1;
        }

        if (strcmp(argv[3], "create") == 0)
        {
            if (argc < 5) {
                printf("usage: ble bulk file create <path>   (ex: 0:/BulkTest.bin)\r\n");
                return 1;
            }
            int rc = TestBulk_FileCreate(argv[4]);
            if (rc != 0) {
                printf("[BLE] bulk file create failed (rc=%d: %s)\r\n", rc,
                       (rc == -1) ? "eMMC not mounted" :
                       (rc == -2) ? "invalid path"     :
                       (rc == -3) ? "recording active - stop first" : "f_open failed");
            }
            return 1;
        }

        if (strcmp(argv[3], "start") == 0)
        {
            const char *path = (argc >= 5) ? argv[4] : NULL;
            int rc = TestBulk_FileStart(path);
            if (rc != 0) {
                printf("[BLE] bulk file start failed (rc=%d: %s)\r\n", rc,
                       (rc == -1) ? "eMMC not mounted" :
                       (rc == -2) ? "no path - run 'file create <path>' first" :
                       (rc == -3) ? "already recording" : "f_open failed");
            }
            return 1;
        }

        if (strcmp(argv[3], "stop") == 0)
        {
            if (TestBulk_FileStop() != 0) {
                printf("[BLE] bulk file stop: not recording\r\n");
            }
            return 1;
        }

        if (strcmp(argv[3], "stat") == 0)
        {
            TestBulk_FileStat();
            return 1;
        }

        TestBulk_CliUsage();
        return 1;
    }

    /* --- 프레임 송신 / 로컬 파싱 --- */
    uint16_t len = TEST_BULK_MAX_LEN;
    if (argc >= 4) {
        long v = strtol(argv[3], NULL, 0);
        if (v <= 0 || v > TEST_BULK_MAX_LEN) {
            printf("[BLE] bulk: len out of range (1..%u)\r\n", TEST_BULK_MAX_LEN);
            return 1;
        }
        len = (uint16_t)v;
    }

    if (strcmp(argv[2], "tx") == 0) {
        TestBulk_Frame(len, false);
        return 1;
    }
    if (strcmp(argv[2], "rx") == 0) {
        TestBulk_Frame(len, true);
        return 1;
    }

    return 0;   /* 미인식 — 호출자가 usage 출력 */
}

#endif /* BLE_BULK_TEST_ENABLED */
