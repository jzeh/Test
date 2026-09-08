/**
 ******************************************************************************
 * @file    sys-emmc.c
 * @brief   eMMC + FatFs 통합 구현 — STM32H7S3I8 + KLM8G1GETF-B041
 *
 *  통합 범위 (구 git-emmc.c + task-emmc.c + 기존 sys-emmc.c) :
 *    [1] Backend       : DMA + Semaphore 기반 emmc_*() (FRP_scan 이식)
 *                        · 전용 32B aligned DMA buffer (64 sectors)
 *                        · HAL_MMC_*Blocks_DMA + IRQ callback → semaphore Give
 *                        · 단일 sector 단위 R/W (multi-block timing race 회피)
 *                        · vTaskDelay yield (FreeRTOS 정상 동작)
 *
 *    [2] Management    : eMMC HW init/deinit, mount/unmount, format,
 *                        folder make, card info, test routines, sector 0 restore
 *
 *    [3] Task          : FreeRTOS emmcTask + 비동기 명령 큐
 *                        · CLI 가 EMMC_PostCmd() 로 명령 enqueue
 *                        · emmcTask 가 큐 받아 management API 호출
 *                        · CLI stack 부담 0 (FatFs 작업은 emmcTask 가)
 *
 *  user_diskio.c 는 thin wrapper — 이 파일의 emmc_*() 호출만 함.
 ******************************************************************************
 */

/* Includes  ----------------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/sys-emmc.h"
#include "../Inc/task-hwcontrol.h"   
#include "main.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "fatfs.h"
#include "ff_gen_drv.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>      /* offsetof — DeviceSerial CRC 범위 */
#include "fw_crc32.h"    /* fw_crc32() — Common/Inc (Appli 빌드에 이미 링크됨) */

/* Defines   ----------------------------------------------------------------*/
#define HMMC                    (&hmmc1)
#define EMMC_SECTOR_SIZE        MMC_BLOCKSIZE                   /* 512 */
#define EMMC_BUFFER_COUNT       64                              /* 64 sectors = 32 KB */
#define EMMC_BUFFER_SIZE        (EMMC_BUFFER_COUNT * EMMC_SECTOR_SIZE)

#define MMC_DMA_TIMEOUT_MS      5000U
#define MMC_STATE_TIMEOUT_MS    10000U   /* f_mkfs erase 등 긴 동작 대응 */
#define MMC_BUSY_TIMEOUT_MS     5000U

#define FILE_DATA_BUFFER_SIZE   4096
#define FILE_INFO_MAX           10
#define EMMC_QUEUE_DEPTH        4

/* emmcTask 명령 메시지 (path/data 동봉으로 thread-safe) */
typedef struct {
    EmmcCmd_t cmd;
    char      path[EMMC_PATH_MAX];
    char      data[EMMC_DATA_MAX];
} EmmcMsg_t;

/* External declarations (peripheral / FATFS link) --------------------------*/
extern MMC_HandleTypeDef hmmc1;
extern uint8_t           retUSER;       /* FATFS_LinkDriver 반환값 */
extern char              USERPath[4];
extern FATFS             USERFatFS;
extern FIL               USERFile;
extern void              MX_SDMMC1_MMC_Init(void);

/* FatFs 에러 코드 표 (디버그용) */
static const char FR_Table[][24] = {
    "FR_OK",                  "FR_DISK_ERR",        "FR_INT_ERR",
    "FR_NOT_READY",           "FR_NO_FILE",         "FR_NO_PATH",
    "FR_INVALID_NAME",        "FR_DENIED",          "FR_EXIST",
    "FR_INVALID_OBJECT",      "FR_WRITE_PROTECTED", "FR_INVALID_DRIVE",
    "FR_NOT_ENABLED",         "FR_NO_FILESYSTEM",   "FR_MKFS_ABORTED",
    "FR_TIMEOUT",             "FR_LOCKED",          "FR_NOT_ENOUGH_CORE",
    "FR_TOO_MANY_OPEN_FILES", "FR_INVALID_PARAMETER"
};

/* Variables - Backend ------------------------------------------------------*/

/* 32B aligned dedicated DMA buffer
 *   FatFs 의 비정렬 buffer 가 들어와도 안전, D-Cache 일관성 영향 격리 */
#pragma data_alignment=32
__attribute__((aligned(32))) static uint8_t emmc_buffer[EMMC_BUFFER_SIZE];

/* Sector 0 backup — f_mkfs 후 sector 0 이 zero 가 되는 현상 워크어라운드 */
__attribute__((aligned(32))) static uint8_t s_sec0_backup[EMMC_SECTOR_SIZE];
static volatile bool                       s_sec0_backup_valid = false;

/* Disk status (FatFs 가 USER_status 호출 시 사용) */
static volatile DSTATUS Stat = STA_NOINIT;

/* DMA 완료 동기화 semaphore */
static StaticSemaphore_t s_fatfs_sema_buffer;
static SemaphoreHandle_t s_fatfs_sema_handle = NULL;
static volatile int      s_mmc_dma_error    = 0;

/* Variables - Management ---------------------------------------------------*/

static uint8_t  g_IsMounted = 0;

/* f_mkfs / file I/O 작업 영역 (DMA/cache 안전, 32B align) */
ALIGN_32BYTES(static uint8_t workBuffer[8 * FF_MAX_SS]);
ALIGN_32BYTES(static char    g_FsReadBuf [FILE_DATA_BUFFER_SIZE]);
ALIGN_32BYTES(static char    g_FsWriteBuf[FILE_DATA_BUFFER_SIZE]);

/* Variables - Task ---------------------------------------------------------*/

static osThreadId_t        s_emmcTaskHandle;
static uint32_t            s_emmcTaskBuffer[8192];   /* 32KB stack */
static StaticTask_t        s_emmcTaskControlBlock;
static const osThreadAttr_t s_emmcTask_attr = {
    .name       = "emmcTask",
    .cb_mem     = &s_emmcTaskControlBlock,
    .cb_size    = sizeof(s_emmcTaskControlBlock),
    .stack_mem  = &s_emmcTaskBuffer[0],
    .stack_size = sizeof(s_emmcTaskBuffer),
    .priority   = (osPriority_t)osPriorityNormal,
};

static osMessageQueueId_t  s_emmcCmdQueue;
static StaticQueue_t       s_emmcCmdQueueCB;
static uint8_t             s_emmcCmdQueueStorage[EMMC_QUEUE_DEPTH * sizeof(EmmcMsg_t)];

/* Internal prototypes ------------------------------------------------------*/
static void EMMC_DumpRegs   (const char *tag, DWORD sector, UINT count);
static void mmc_dma_done    (int error);
static int  MMC_WaitCardReady(uint32_t timeout_ms);
static void StartEmmcTask   (void *argument);
static HAL_StatusTypeDef EMMC_TryInit(void);

/*=============================================================================
 *  [1] BACKEND — DMA + Semaphore 기반 sector R/W (user_diskio.c 가 호출)
 *=============================================================================*/

/* SDMMC 레지스터 덤프 (실패 진단용)
 *   · hmmc1.ErrorCode 비트를 사람이 읽을 수 있는 약어로 디코딩
 *   · sector / count / LogBlockNbr / HAL State / Context 함께 출력
 *
 *  주요 ErrorCode 값 (stm32h7rsxx_ll_sdmmc.h 참고):
 *      0x00000040 = ADDR_MISALIGNED   (4KB 단위가 아닌 BlockAdd)
 *      0x00000080 = BLOCK_LEN_ERR     (4KB 모드에서 NumberOfBlocks % 8 != 0)
 *      0x00100000 = DATA_TIMEOUT
 *      0x00200000 = DATA_CRC_FAIL
 *      0x02000000 = ADDR_OUT_OF_RANGE (BlockAdd + count > LogBlockNbr)
 *      0x10000000 = TIMEOUT (CMD)
 *      0x20000000 = BUSY              (HAL state != READY)
 */
static const char *EMMC_DecodeError(uint32_t err)
{
    if (err == 0U)                                        return "NONE";
    if (err & 0x20000000U /* BUSY */)                     return "BUSY";
    if (err & 0x02000000U /* ADDR_OUT_OF_RANGE */)        return "OOR";
    if (err & 0x00000040U /* ADDR_MISALIGNED */)          return "MISALIGN";
    if (err & 0x00000080U /* BLOCK_LEN_ERR */)            return "BLKLEN";
    if (err & 0x00200000U /* DATA_CRC_FAIL */)            return "DCRC";
    if (err & 0x00100000U /* DATA_TIMEOUT */)             return "DTO";
    if (err & 0x10000000U /* TIMEOUT */)                  return "TO";
    return "OTHER";
}

static void EMMC_DumpRegs(const char *tag, DWORD sector, UINT count)
{
    printf("[eMMC]%s [%s] sec=%lu cnt=%u Err=0x%08lX LBN=%lu HALst=%u Ctx=%lu "
           "STA=0x%08lX RESPCMD=%lu RESP1=0x%08lX DCTRL=0x%08lX DLEN=%lu "
           "IDMABASE=0x%08lX CLKCR=0x%08lX\r\n",
           tag, EMMC_DecodeError(hmmc1.ErrorCode),
           (unsigned long)sector, (unsigned)count,
           hmmc1.ErrorCode,
           (unsigned long)hmmc1.MmcCard.LogBlockNbr,
           (unsigned)hmmc1.State,
           (unsigned long)hmmc1.Context,
           SDMMC1->STA, SDMMC1->RESPCMD, SDMMC1->RESP1,
           SDMMC1->DCTRL, SDMMC1->DLEN, SDMMC1->IDMABASER, SDMMC1->CLKCR);
}

/* DMA 완료 콜백 — IRQ 컨텍스트에서 semaphore Give */
static void mmc_dma_done(int error)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    s_mmc_dma_error = error;
    if (s_fatfs_sema_handle != NULL) {
        xSemaphoreGiveFromISR(s_fatfs_sema_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_MMC_TxCpltCallback(MMC_HandleTypeDef *hmmc) { (void)hmmc; mmc_dma_done(0); }
void HAL_MMC_RxCpltCallback(MMC_HandleTypeDef *hmmc) { (void)hmmc; mmc_dma_done(0); }
void HAL_MMC_AbortCallback (MMC_HandleTypeDef *hmmc) { (void)hmmc; mmc_dma_done(1); }

/* 카드 TRANSFER 상태 진입 대기 (vTaskDelay yield) */
static int MMC_WaitCardReady(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while (HAL_MMC_GetCardState(HMMC) != HAL_MMC_CARD_TRANSFER) {
        if ((HAL_GetTick() - start) > timeout_ms) return -1;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return 0;
}

/* HAL_MMC 내부 state 가 READY 상태로 진입할 때까지 대기
 *   IRQ callback 이 처리되기 전에 다음 호출이 들어오면 HAL_BUSY 반환됨
 *   → 짧은 race window 를 vTaskDelay yield 로 해소
 *   timeout 도달 시에도 ErrorCode/State 를 강제 reset 후 -1 반환 */
#define MMC_HAL_STATE_TIMEOUT_MS    100U
static int MMC_WaitHalReady(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while (HAL_MMC_GetState(HMMC) != HAL_MMC_STATE_READY) {
        if ((HAL_GetTick() - start) > timeout_ms) {
            /* 마지막 수단 : HAL state 강제 복귀 (잘못된 상태에 끼었을 수 있음) */
            hmmc1.ErrorCode = HAL_MMC_ERROR_NONE;
            hmmc1.State     = HAL_MMC_STATE_READY;
            return -1;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return 0;
}

/* HAL_MMC 강제 복구 — HAL_BUSY 가 지속될 때 사용
 *   · HAL_MMC_Abort 호출 (DMA/IDMA/CMD/Context 모두 cleanup)
 *   · State/ErrorCode 명시적 reset
 *   · CMD13 으로 카드 ready 확인
 * @retval 0=성공, -1=Abort 실패 (HW 문제)                                    */
static int MMC_ForceRecover(const char *tag)
{
    printf("[eMMC] %s -> ForceRecover (Abort + reset)\r\n", tag);
    (void)HAL_MMC_Abort(HMMC);
    hmmc1.ErrorCode = HAL_MMC_ERROR_NONE;
    hmmc1.Context   = MMC_CONTEXT_NONE;
    hmmc1.State     = HAL_MMC_STATE_READY;

    /* 카드도 정리될 시간 부여 */
    vTaskDelay(pdMS_TO_TICKS(5));
    if (MMC_WaitCardReady(500) != 0) {
        printf("[eMMC] ForceRecover: card not ready after Abort\r\n");
        return -1;
    }
    return 0;
}

DSTATUS emmc_initialize(BYTE pdrv)
{
    (void)pdrv;
    if (s_fatfs_sema_handle == NULL) {
        s_fatfs_sema_handle = xSemaphoreCreateBinaryStatic(&s_fatfs_sema_buffer);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    Stat = (HAL_MMC_GetState(HMMC) == HAL_MMC_STATE_READY) ? 0 : STA_NOINIT;
    return Stat;
}

DSTATUS emmc_status(BYTE pdrv)
{
    (void)pdrv;
    Stat = (HAL_MMC_GetState(HMMC) == HAL_MMC_STATE_READY) ? 0 : STA_NOINIT;
    return Stat;
}

DRESULT emmc_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    (void)pdrv;
    if (Stat & STA_NOINIT) return RES_NOTRDY;
    if (count == 0)        return RES_PARERR;

    uint8_t *ptr = buff;
    while (count > 0) {
        UINT chunk = 1;                          /* 단일 sector */
        UINT size  = chunk * EMMC_SECTOR_SIZE;
        HAL_StatusTypeDef hs;
        int attempt;

        if (MMC_WaitCardReady(MMC_STATE_TIMEOUT_MS) != 0) {
            EMMC_DumpRegs("R-not_ready", sector, chunk); return RES_ERROR;
        }

        /* HAL state READY 대기 (IRQ callback ↔ Task race 해소) */
        if (MMC_WaitHalReady(MMC_HAL_STATE_TIMEOUT_MS) != 0) {
            EMMC_DumpRegs("R-HAL_busy", sector, chunk);
            /* state 는 위에서 강제 reset 했음 → 1회 retry 시도 가능 */
        }

        /* HAL_MMC_ReadBlocks_DMA retry up to 3 attempts
         *   attempt 0 : BUSY → 단순 state reset / 그 외 에러 → ForceRecover
         *   attempt 1~2 : MMC_ForceRecover (HAL_MMC_Abort + Context reset)
         *
         *  ★ HAL_MMC_ERROR_ADDR_OUT_OF_RANGE (0x02000000) 발생 시 :
         *     - HAL 이 LogBlockNbr 정보를 잃거나 hmmc1 구조체가 부분 reset 된 상황
         *     - ForceRecover 만으로는 복구 불가 → 즉시 종료해서 호출자가 재마운트하도록 유도
         */
        hs = HAL_ERROR;
        for (attempt = 0; attempt < 3; attempt++) {
            hs = HAL_MMC_ReadBlocks_DMA(HMMC, emmc_buffer, sector, chunk);
            if (hs == HAL_OK) break;

            uint32_t err = hmmc1.ErrorCode;

            /* ADDR_OUT_OF_RANGE — 재시도해도 동일 결과, 즉시 종료 */
            if (err & 0x02000000U /* HAL_MMC_ERROR_ADDR_OUT_OF_RANGE */) {
                printf("[eMMC] R-OOR sec=%lu LBN=%lu — abort retry\r\n",
                       (unsigned long)sector,
                       (unsigned long)hmmc1.MmcCard.LogBlockNbr);
                break;
            }

            /* BUSY — state race window. attempt 0 은 silent reset, 그 외 ForceRecover */
            if (err & HAL_MMC_ERROR_BUSY) {
                if (attempt == 0) {
                    hmmc1.ErrorCode = HAL_MMC_ERROR_NONE;
                    hmmc1.State     = HAL_MMC_STATE_READY;
                    vTaskDelay(pdMS_TO_TICKS(2));
                } else {
                    (void)MMC_ForceRecover("R-BUSY");
                    vTaskDelay(pdMS_TO_TICKS(5 + attempt * 5));
                }
                continue;
            }

            /* HAL 이 return HAL_BUSY 만 했고 ErrorCode 는 그대로인 경우
             * (DMA 함수의 state-not-ready 경로). 한번은 ForceRecover 시도 */
            if (hs == HAL_BUSY && err == 0U) {
                (void)MMC_ForceRecover("R-HALBSY");
                vTaskDelay(pdMS_TO_TICKS(5 + attempt * 5));
                continue;
            }

            /* 기타 에러 — 한 번은 복구 시도 후 종료 */
            if (attempt < 2) {
                (void)MMC_ForceRecover("R-OTHER");
                vTaskDelay(pdMS_TO_TICKS(5 + attempt * 5));
                continue;
            }
            break;
        }
        if (hs != HAL_OK) {
            EMMC_DumpRegs("R-DMA_fail", sector, chunk);
            return RES_ERROR;
        }

        if (xSemaphoreTake(s_fatfs_sema_handle, pdMS_TO_TICKS(MMC_DMA_TIMEOUT_MS)) != pdTRUE) {
            EMMC_DumpRegs("R-DMA_timeout", sector, chunk); return RES_ERROR;
        }

        SCB_InvalidateDCache_by_Addr(
            (uint32_t *)(((uintptr_t)emmc_buffer) & ~31),
            (size + 31) & ~31);
        memcpy(ptr, emmc_buffer, size);

        ptr    += size;
        sector += chunk;
        count  -= chunk;
    }
    return RES_OK;
}

#if _USE_WRITE == 1
DRESULT emmc_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    (void)pdrv;
    if (Stat & STA_NOINIT)  return RES_NOTRDY;
    if (Stat & STA_PROTECT) return RES_WRPRT;
    if (count == 0)         return RES_PARERR;

    const uint8_t *ptr = buff;
    while (count > 0) {
        UINT chunk = 1;                          /* 단일 sector */
        UINT size  = chunk * EMMC_SECTOR_SIZE;
        HAL_StatusTypeDef hs;
        int attempt;

        if (MMC_WaitCardReady(MMC_STATE_TIMEOUT_MS) != 0) {
            EMMC_DumpRegs("W-not_ready", sector, chunk); return RES_ERROR;
        }

        memcpy(emmc_buffer, ptr, size);
        SCB_CleanDCache_by_Addr((uint32_t *)emmc_buffer, (size + 31) & ~31);

        /* sector 0 자동 backup 제거 — 어떤 0x55AA 도 valid BPB 로 신뢰할 수 없음
         * (RAW test pattern, garbage 등이 우연히 0x55AA 갖고 backup 될 위험).
         * EMMC_RestoreSec0 가 필요하면 별도 명령으로 backup/restore 호출하도록 분리. */

        /* HAL state READY 대기 (IRQ callback ↔ Task race 해소) */
        if (MMC_WaitHalReady(MMC_HAL_STATE_TIMEOUT_MS) != 0) {
            EMMC_DumpRegs("W-HAL_busy", sector, chunk);
        }

        /* HAL_MMC_WriteBlocks_DMA retry up to 3 attempts (read 측과 동일 정책) */
        hs = HAL_ERROR;
        for (attempt = 0; attempt < 3; attempt++) {
            hs = HAL_MMC_WriteBlocks_DMA(HMMC, emmc_buffer, sector, chunk);
            if (hs == HAL_OK) break;

            uint32_t err = hmmc1.ErrorCode;

            if (err & 0x02000000U /* HAL_MMC_ERROR_ADDR_OUT_OF_RANGE */) {
                printf("[eMMC] W-OOR sec=%lu LBN=%lu — abort retry\r\n",
                       (unsigned long)sector,
                       (unsigned long)hmmc1.MmcCard.LogBlockNbr);
                break;
            }

            if (err & HAL_MMC_ERROR_BUSY) {
                if (attempt == 0) {
                    hmmc1.ErrorCode = HAL_MMC_ERROR_NONE;
                    hmmc1.State     = HAL_MMC_STATE_READY;
                    vTaskDelay(pdMS_TO_TICKS(2));
                } else {
                    (void)MMC_ForceRecover("W-BUSY");
                    vTaskDelay(pdMS_TO_TICKS(5 + attempt * 5));
                }
                continue;
            }

            if (hs == HAL_BUSY && err == 0U) {
                (void)MMC_ForceRecover("W-HALBSY");
                vTaskDelay(pdMS_TO_TICKS(5 + attempt * 5));
                continue;
            }

            if (attempt < 2) {
                (void)MMC_ForceRecover("W-OTHER");
                vTaskDelay(pdMS_TO_TICKS(5 + attempt * 5));
                continue;
            }
            break;
        }
        if (hs != HAL_OK) {
            EMMC_DumpRegs("W-DMA_fail", sector, chunk);
            return RES_ERROR;
        }

        if (xSemaphoreTake(s_fatfs_sema_handle, pdMS_TO_TICKS(MMC_DMA_TIMEOUT_MS)) != pdTRUE) {
            EMMC_DumpRegs("W-DMA_timeout", sector, chunk); return RES_ERROR;
        }
        if (MMC_WaitCardReady(MMC_BUSY_TIMEOUT_MS) != 0) {
            EMMC_DumpRegs("W-busy_after", sector, chunk); return RES_ERROR;
        }

        ptr    += size;
        sector += chunk;
        count  -= chunk;
    }
    return RES_OK;
}
#endif

#if _USE_IOCTL == 1
DRESULT emmc_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    (void)pdrv;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    DRESULT                  res = RES_ERROR;
    HAL_MMC_CardInfoTypeDef  info;

    switch (cmd) {
        case CTRL_SYNC:
            if (MMC_WaitCardReady(MMC_STATE_TIMEOUT_MS) == 0) res = RES_OK;
            break;
        case GET_SECTOR_COUNT:
            if (HAL_MMC_GetCardInfo(HMMC, &info) == HAL_OK) {
                *(DWORD *)buff = info.LogBlockNbr;
                res = RES_OK;
            }
            break;
        case GET_SECTOR_SIZE:
            if (HAL_MMC_GetCardInfo(HMMC, &info) == HAL_OK) {
                *(WORD *)buff = (WORD)info.LogBlockSize;
                res = RES_OK;
            }
            break;
        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            res = RES_OK;
            break;
        default:
            res = RES_PARERR;
            break;
    }
    return res;
}
#endif

/*=============================================================================
 *  [1b] USB MSC BACKEND — usbd_storage_if.c 가 호출 (★ USB OTG ISR 컨텍스트 ★)
 *
 *  주의/설계
 *    · 호출 컨텍스트가 ISR 이므로 FreeRTOS API(세마포어/vTaskDelay) 금지.
 *      → DMA(IRQ)+세마포어 방식인 emmc_read/emmc_write 를 그대로 쓸 수 없어
 *        폴링(blocking) HAL_MMC_ReadBlocks/WriteBlocks 로 별도 구현.
 *    · 전제: USB MSC 모드 동안 FatFs 언마운트 + emmcTask idle → hmmc1 단독 점유.
 *      (EMMC_CMD_UNMOUNT 로 진입, EMMC_CMD_MOUNT 로 복귀)
 *    · 블로킹 HAL_MMC_ReadBlocks/WriteBlocks 는 IDMA 가 아니라 CPU FIFO 전송이라
 *      캐시 코히런트 → D-Cache invalidate/clean 하면 안 됨(stale 데이터 유발).
 *    · 전용 32B 정렬 버퍼(s_msc_buffer) 경유 — FatFs 경로(emmc_buffer)와 분리해
 *      버퍼 경합 차단 + FIFO 접근용 4B 정렬 보장.
 *=============================================================================*/
#define EMMC_MSC_HAL_TIMEOUT_MS   1000U
#define EMMC_MSC_WAIT_GUARD       8000000UL   /* busy-spin 상한 (ISR 무한루프 방지) */
#define EMMC_MSC_BUF_SECTORS      8U          /* MSC 전용 bounce 버퍼 (4KB) */

/* MSC 전용 32B 정렬 bounce 버퍼.
 *   FatFs 경로의 emmc_buffer 와 분리 → 호스트 연결 직후 짧은 윈도우에서
 *   emmcTask 의 FatFs I/O 와 USB ISR 의 MSC I/O 가 버퍼를 다투지 않도록 격리. */
__attribute__((aligned(32))) static uint8_t s_msc_buffer[EMMC_MSC_BUF_SECTORS * EMMC_SECTOR_SIZE];

/* vTaskDelay 금지 컨텍스트용 — 카드가 TRANSFER 상태로 돌아올 때까지 busy-spin */
static int EMMC_MSC_WaitTransfer(void)
{
    uint32_t guard = 0;
    while (HAL_MMC_GetCardState(HMMC) != HAL_MMC_CARD_TRANSFER) {
        if (++guard > EMMC_MSC_WAIT_GUARD) return -1;
    }
    return 0;
}

/* ISR-safe busy-wait — vTaskDelay 대용. 정밀하지 않아도 됨(복구 유예 시간용) */
static void EMMC_MSC_BusyWait(uint32_t loops)
{
    volatile uint32_t i;
    for (i = 0; i < loops; i++) { __NOP(); }
}

/* ISR-safe 강제 복구 — MMC_ForceRecover 의 ISR 버전 (printf/vTaskDelay 제거).
 *   HAL_BUSY / transient 에러가 지속될 때 Abort + state reset 후 카드 정리 대기. */
static void EMMC_MSC_ForceRecover(void)
{
    (void)HAL_MMC_Abort(HMMC);
    hmmc1.ErrorCode = HAL_MMC_ERROR_NONE;
    hmmc1.Context   = MMC_CONTEXT_NONE;
    hmmc1.State     = HAL_MMC_STATE_READY;
    EMMC_MSC_BusyWait(300000UL);        /* 카드/HAL 정리 유예 */
    (void)EMMC_MSC_WaitTransfer();
}

int8_t EMMC_MSC_IsReady(void)
{
    return (HAL_MMC_GetState(HMMC) == HAL_MMC_STATE_READY) ? 0 : -1;
}

int8_t EMMC_MSC_GetCapacity(uint32_t *block_num, uint16_t *block_size)
{
    HAL_MMC_CardInfoTypeDef info;
    if (HAL_MMC_GetCardInfo(HMMC, &info) != HAL_OK) return -1;
    if (info.LogBlockNbr == 0)                       return -1;
    *block_num  = info.LogBlockNbr;
    *block_size = (uint16_t)info.LogBlockSize;     /* 512 */
    return 0;
}

int8_t EMMC_MSC_Read(uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    if (Stat & STA_NOINIT) return -1;

    while (blk_len > 0) {
        uint16_t chunk = (blk_len > EMMC_MSC_BUF_SECTORS) ? EMMC_MSC_BUF_SECTORS : blk_len;
        uint32_t size  = (uint32_t)chunk * EMMC_SECTOR_SIZE;
        HAL_StatusTypeDef hs = HAL_ERROR;

        /* 최대 3회 재시도 — emmc_read 의 ISR-safe 버전(busy-spin 복구) */
        for (int attempt = 0; attempt < 3; attempt++) {
            if (EMMC_MSC_WaitTransfer() != 0) EMMC_MSC_ForceRecover();

            hs = HAL_MMC_ReadBlocks(HMMC, s_msc_buffer, blk_addr, chunk,
                                    EMMC_MSC_HAL_TIMEOUT_MS);
            if (hs == HAL_OK) break;

            /* ADDR_OUT_OF_RANGE — 재시도해도 동일, 즉시 종료 */
            if (hmmc1.ErrorCode & 0x02000000U) break;

            EMMC_MSC_ForceRecover();
            EMMC_MSC_BusyWait(300000UL * (uint32_t)(attempt + 1));
        }
        if (hs != HAL_OK) return -1;
        if (EMMC_MSC_WaitTransfer() != 0) return -1;

        /* ★ HAL_MMC_ReadBlocks(블로킹)는 CPU FIFO 로 읽어 s_msc_buffer 에 캐시
         *   코히런트하게 적재된다. 여기서 D-Cache invalidate 를 하면 방금 읽은
         *   캐시라인이 버려져 stale 데이터를 읽게 되므로 절대 금지(IDMA 경로 아님). */
        memcpy(buf, s_msc_buffer, size);

        buf      += size;
        blk_addr += chunk;
        blk_len  -= chunk;
    }
    return 0;
}

int8_t EMMC_MSC_Write(uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    if (Stat & STA_NOINIT) return -1;

    while (blk_len > 0) {
        uint16_t chunk = (blk_len > EMMC_MSC_BUF_SECTORS) ? EMMC_MSC_BUF_SECTORS : blk_len;
        uint32_t size  = (uint32_t)chunk * EMMC_SECTOR_SIZE;
        HAL_StatusTypeDef hs = HAL_ERROR;

        memcpy(s_msc_buffer, buf, size);
        /* HAL_MMC_WriteBlocks(블로킹)도 CPU FIFO 전송 → 캐시 코히런트, 별도 유지 불필요. */

        /* 최대 3회 재시도 — emmc_write 의 ISR-safe 버전(busy-spin 복구).
         * Windows 포맷/파일복사처럼 대량 연속 쓰기에서 transient BUSY 를 흡수. */
        for (int attempt = 0; attempt < 3; attempt++) {
            if (EMMC_MSC_WaitTransfer() != 0) EMMC_MSC_ForceRecover();

            hs = HAL_MMC_WriteBlocks(HMMC, s_msc_buffer, blk_addr, chunk,
                                     EMMC_MSC_HAL_TIMEOUT_MS);
            if (hs == HAL_OK) break;

            if (hmmc1.ErrorCode & 0x02000000U) break;   /* ADDR_OUT_OF_RANGE */

            EMMC_MSC_ForceRecover();
            EMMC_MSC_BusyWait(300000UL * (uint32_t)(attempt + 1));
        }
        if (hs != HAL_OK) return -1;
        /* 쓰기 후 programming(BUSY) 완료까지 대기 */
        if (EMMC_MSC_WaitTransfer() != 0) return -1;

        buf      += size;
        blk_addr += chunk;
        blk_len  -= chunk;
    }
    return 0;
}

/*=============================================================================
 *  [2] MANAGEMENT — init / mount / format / test / card info
 *=============================================================================*/

/* HAL_MMC_Init retry — 실패 시 최대 3회 재시도 (전원 재인가 포함) */
static HAL_StatusTypeDef EMMC_TryInit(void)
{
    const int max_retry = 3;
    HAL_StatusTypeDef status = HAL_ERROR;

    for (int attempt = 1; attempt <= max_retry; attempt++)
    {
        if (hmmc1.State != HAL_MMC_STATE_RESET) {
            HAL_MMC_DeInit(&hmmc1);
        }
        EMMC_Enable();

        hmmc1.Instance                  = SDMMC1;
        hmmc1.Init.ClockEdge            = SDMMC_CLOCK_EDGE_RISING;
        hmmc1.Init.ClockPowerSave       = SDMMC_CLOCK_POWER_SAVE_DISABLE;
        hmmc1.Init.BusWide              = SDMMC_BUS_WIDE_8B;
        hmmc1.Init.HardwareFlowControl  = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
        hmmc1.Init.ClockDiv             = 2;     /* PLL2S 60MHz / (2*2) = 15MHz DS */

        status = HAL_MMC_Init(&hmmc1);
        if (status == HAL_OK) {
            printf("[eMMC] HAL_MMC_Init OK (attempt %d/%d)\r\n", attempt, max_retry);
            return HAL_OK;
        }
        printf("[eMMC] HAL_MMC_Init FAIL %d/%d (status=%d, ErrorCode=0x%08lX)\r\n",
               attempt, max_retry, status, hmmc1.ErrorCode);
        HAL_Delay(50);
    }
    printf("[eMMC] HAL_MMC_Init FAILED after %d attempts\r\n", max_retry);
    return HAL_ERROR;
}

/* OTA 폴더 보장 — f_stat 으로 먼저 read 하여 이미 있으면 skip, 없으면 f_mkdir */
static FRESULT EMMC_EnsureDir(const char *path)
{
    FILINFO fno;
    FRESULT res = f_stat(path, &fno);
    if (res == FR_OK) {
        printf("[eMMC] dir \"%s\" exists (skip)\r\n", path);
        return FR_OK;
    }

    res = f_mkdir(path);
    if (res == FR_OK) {
        printf("[eMMC] dir \"%s\" created\r\n", path);
    } else {
        printf("[eMMC] dir \"%s\" mkdir FAIL : %s (%d)\r\n", path, FR_Table[res], res);
    }
    return res;
}

int32_t InitEMMC(void)
{
#if 0
    /* Reset cause 로깅 */
    {
        uint32_t rsr = RCC->RSR;
        printf("[eMMC] reset cause: RSR=0x%08lX [%s%s%s%s%s]\r\n",
               rsr,
               (rsr & RCC_RSR_PORRSTF) ? "POR "   : "",
               (rsr & RCC_RSR_SFTRSTF) ? "SWRST " : "",
               (rsr & RCC_RSR_PINRSTF) ? "PIN "   : "",
               (rsr & RCC_RSR_BORRSTF) ? "BOR "   : "",
               (rsr & RCC_RSR_IWDGRSTF)? "IWDG "  : "");
    }
#endif
    if (EMMC_TryInit() != HAL_OK) {
        printf("[eMMC] init failed — eMMC unavailable, system continues without storage\r\n");
        return -3;
    }

    MX_FATFS_Init();

    if (retUSER == 0) {
        if (mountFatFS() != FR_OK) return -1;
    } else {
        return -2;
    }

    /* 마운트 직후 OTA 폴더 보장 — 이미 있으면 건너뜀 */
    EMMC_EnsureDir("01_Application");
    EMMC_EnsureDir("02_Backup");

    /* 디바이스 시리얼 로드 (BLE 광고 이름용) — 마운트 직후, 단일 스레드 컨텍스트 */
    DeviceSerial_Load();

    printf("EMMC Init Success\r\n");
    return 0;
}

/*=============================================================================
 *  Device Serial — BLE 광고 이름 suffix ("BDC"+8자리), eMMC 영속 + write-once
 *=============================================================================*/

typedef __packed struct {
    uint32_t preamble;                          /* DEVSERIAL_PREAMBLE */
    uint8_t  locked;                            /* DEVSERIAL_LOCK_MARK = 잠김 */
    char     serial[DEVSERIAL_SUFFIX_LEN + 1];  /* 8자리 + '\0' */
    uint32_t crc32;                             /* preamble..serial CRC */
} device_serial_t;

static device_serial_t s_dser;
static uint8_t         s_dser_loaded = 0;

void DeviceSerial_Load(void)
{
    FIL  fp;
    UINT br;

    /* 기본값 — 미설정/손상 시 "00000000" */
    memset(&s_dser, 0, sizeof(s_dser));
    s_dser.preamble = DEVSERIAL_PREAMBLE;
    s_dser.locked   = 0;
    strcpy(s_dser.serial, "00000000");

    if (getMountStatus() != 1) {
        /* eMMC 미마운트 상태에서의 호출(정상 순서에서는 발생하지 않아야 함 — 방어 코드).
         * s_dser_loaded 를 세우지 않아 DeviceSerial_Get() 의 지연로딩이 마운트 후
         * 다시 시도하도록 한다. 여기서 세워버리면 기본값("00000000")이 영구 캐시되어
         * BLE 광고 이름이 실제 저장된 시리얼과 어긋나는 문제가 재발한다. */
        // printf("[SERIAL] eMMC not mounted yet — using default, will retry\r\n");
        return;
    }

    if (f_open(&fp, DEVSERIAL_FILE, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
        device_serial_t t;
        if (f_read(&fp, &t, sizeof(t), &br) == FR_OK && br == sizeof(t)
            && t.preamble == DEVSERIAL_PREAMBLE
            && t.crc32 == fw_crc32((const uint8_t *)&t, offsetof(device_serial_t, crc32))) {
            t.serial[DEVSERIAL_SUFFIX_LEN] = '\0';
            memcpy(&s_dser, &t, sizeof(t));
        } else {
            printf("[SERIAL] file invalid — using default\r\n");
        }
        f_close(&fp);
    }

    s_dser_loaded = 1;
    printf("[SERIAL] suffix=%s locked=%u\r\n", s_dser.serial, (unsigned)s_dser.locked);
}

const char* DeviceSerial_Get(void)
{
    if (!s_dser_loaded) DeviceSerial_Load();
    return s_dser.serial;
}

uint8_t DeviceSerial_IsLocked(void)
{
    if (!s_dser_loaded) DeviceSerial_Load();
    return (s_dser.locked == DEVSERIAL_LOCK_MARK) ? 1U : 0U;
}

int DeviceSerial_Set(const char *suffix)
{
    if (!s_dser_loaded) DeviceSerial_Load();

    /* write-once 제거 — 언제든 재설정(덮어쓰기) 허용.
     * locked 필드는 "유효 레코드가 기록됨" 표식으로만 유지(재설정 차단 X). */
    if (suffix == NULL) return -2;
    for (uint32_t i = 0; i < DEVSERIAL_SUFFIX_LEN; i++) {
        if (suffix[i] < '0' || suffix[i] > '9') return -2;   /* 8자리 숫자만 */
    }

    device_serial_t d;
    memset(&d, 0, sizeof(d));
    d.preamble = DEVSERIAL_PREAMBLE;
    d.locked   = DEVSERIAL_LOCK_MARK;
    memcpy(d.serial, suffix, DEVSERIAL_SUFFIX_LEN);
    d.serial[DEVSERIAL_SUFFIX_LEN] = '\0';
    d.crc32 = fw_crc32((const uint8_t *)&d, offsetof(device_serial_t, crc32));

    FIL  fp;
    UINT bw;
    if (f_open(&fp, DEVSERIAL_FILE, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) return -3;
    f_write(&fp, &d, sizeof(d), &bw);
    f_sync(&fp);
    f_close(&fp);
    if (bw != sizeof(d)) return -3;

    memcpy(&s_dser, &d, sizeof(d));
    printf("[SERIAL] set suffix=%s (saved)\r\n", d.serial);
    return 0;
}

void deinitMMC(void)
{
    unmountFatFS();
    EMMC_Disable();
}

FRESULT mountFatFS(void)
{
    FRESULT res = FR_OK;

    if (g_IsMounted != 1) {
        /* mode=0: LAZY mount — sector 0 read 안 함, 첫 f_open 시점에 검증 */
        res = f_mount(&USERFatFS, (TCHAR const*)USERPath, 0);
        if (res == FR_OK) {
            printf("Success... Mount(%s)\r\n", USERPath);
            g_IsMounted = 1;
        } else {
            printf("Error... fail mount(%s) res=%s\r\n", USERPath, FR_Table[res]);
            g_IsMounted = 0;
        }
    } else {
        printf("Already mounted...\r\n");
    }
    return res;
}

FRESULT unmountFatFS(void)
{
    FRESULT res = FR_OK;
    if (g_IsMounted == 1) {
        g_IsMounted = 0;
        res = f_mount(NULL, (TCHAR const*)"", 0);
        if (res == FR_OK) {
            g_IsMounted = 0;
            printf("Success... Unmount!!!\r\n");
        } else {
            printf("Failed... Unmount!!!(%s)\r\n", FR_Table[res]);
        }
    } else {
        printf("Already unmounted!!!\r\n");
    }
    return res;
}

uint8_t getMountStatus(void)
{
    return g_IsMounted;
}

FRESULT formatEmmc(void)
{
    FRESULT res;
    /* FM_SFD: Super-Floppy Disk — partition table 없이 sector 0 부터 VBR 직접 */
    MKFS_PARM opt = { FM_FAT32 | FM_SFD, 0, 0, 0, 4096 };

    if (g_IsMounted) unmountFatFS();

    printf("[formatEmmc] f_mkfs start...\r\n");
    res = f_mkfs(USERPath, &opt, workBuffer, sizeof(workBuffer));

    if (res != FR_OK) {
        printf("[formatEmmc] f_mkfs FAILED: %s (%d)\r\n", FR_Table[res], res);
    } else {
        printf("[formatEmmc] f_mkfs OK\r\n");
    }

    FRESULT mres = mountFatFS();
    if (mres != FR_OK) {
        printf("[formatEmmc] remount failed: %s\r\n", FR_Table[mres]);
        return (res == FR_OK) ? mres : res;
    }
    printf("[formatEmmc] remounted OK\r\n");
    return res;
}

uint8_t InitEmmcFolder(void)
{
    FRESULT res;
    char    path[40];

    if (g_IsMounted == 0) mountFatFS();

    /* Application folder */
    snprintf(path, sizeof(path), "%s%s", USERPath, EMMC_APPLICATION_FOLDER);
    res = f_mkdir(path);
    if (res != FR_OK && res != FR_EXIST) {
        printf("Failed...(%s, %s)\r\n", path, FR_Table[res]);
        return 1;
    }

    /* Backup folder */
    snprintf(path, sizeof(path), "%s%s", USERPath, EMMC_BACKUP_FOLDER);
    res = f_mkdir(path);
    if (res != FR_OK && res != FR_EXIST) {
        printf("Failed...(%s, %s)\r\n", path, FR_Table[res]);
        return 2;
    }
    return 0;
}

/* Raw HAL 단위 W/R 검증 (FatFs 우회)
 *
 *  WARNING — 이전 버전은 sector 0 (BPB 위치) 에 패턴을 직접 기록해서
 *    FAT 파일시스템을 영구 파괴했음. 한번 실행하면 다음 부팅부터 mount 실패,
 *    f_opendir / f_open 모두 FR_DISK_ERR + OOR(0x02000000) 발생.
 *    포맷 (f_mkfs) 만이 복구 수단이었음.
 *
 *  현재 버전은 마지막 sector (LogBlockNbr - 1) 를 사용해서 BPB / FAT /
 *    데이터 영역을 절대 건드리지 않음. mkfs 가 만든 파일시스템이 마지막
 *    sector 까지 데이터를 채워둔 상태가 아닌 한 (8GB eMMC 에서는 사실상
 *    영구 free), 손상 위험 없음.
 */
void EMMC_RawSector0Test(void)
{
    ALIGN_32BYTES(static uint8_t wbuf[512]);
    ALIGN_32BYTES(static uint8_t rbuf[512]);

    /* 카드 용량 기반 안전한 sector 선택 — 사용자 데이터 영역 밖 */
    if (hmmc1.MmcCard.LogBlockNbr == 0) {
        printf("[RAW] LogBlockNbr=0 — eMMC not initialized, abort\r\n");
        return;
    }
    uint32_t target_sec = hmmc1.MmcCard.LogBlockNbr - 1U;

    /* 패턴 — boot signature 안 씀 (혹시 잘못 sector 0 으로 가도 파일시스템 손상
     * 가능성 낮추기 위해 0x55 0xAA 제거). 대신 magic prefix 로 검증. */
    for (int i = 0; i < 512; i++) wbuf[i] = (uint8_t)((i + 0x37) & 0xFF);
    wbuf[0] = 0xDE; wbuf[1] = 0xAD; wbuf[2] = 0xBE; wbuf[3] = 0xEF;
    wbuf[508] = 'R'; wbuf[509] = 'A'; wbuf[510] = 'W'; wbuf[511] = '!';

    printf("[RAW] write sector %lu (pattern, BPB-safe area)\r\n",
           (unsigned long)target_sec);
    SCB_CleanDCache_by_Addr((uint32_t*)wbuf, 512);
    HAL_StatusTypeDef wstatus = HAL_MMC_WriteBlocks(&hmmc1, wbuf, target_sec, 1, 5000);
    while (HAL_MMC_GetCardState(&hmmc1) != HAL_MMC_CARD_TRANSFER) { vTaskDelay(pdMS_TO_TICKS(1)); }
    printf("[RAW] write status=%d\r\n", wstatus);

    HAL_Delay(50);

    memset(rbuf, 0xCC, 512);
    SCB_InvalidateDCache_by_Addr((uint32_t*)rbuf, 512);
    HAL_StatusTypeDef rstatus = HAL_MMC_ReadBlocks(&hmmc1, rbuf, target_sec, 1, 5000);
    while (HAL_MMC_GetCardState(&hmmc1) != HAL_MMC_CARD_TRANSFER) { vTaskDelay(pdMS_TO_TICKS(1)); }
    SCB_InvalidateDCache_by_Addr((uint32_t*)rbuf, 512);
    printf("[RAW] read status=%d\r\n", rstatus);

    printf("[RAW] readback first 16: ");
    for (int i = 0; i < 16; i++) printf("%02X ", rbuf[i]);
    printf("\r\n[RAW] readback [508..511]: %02X %02X %02X %02X\r\n",
           rbuf[508], rbuf[509], rbuf[510], rbuf[511]);

    int mismatch = memcmp(wbuf, rbuf, 512);
    printf("[RAW] %s (mismatch=%d, sector=%lu — sector 0 NOT touched)\r\n",
           (mismatch == 0) ? "PASS — last-sector W/R OK" : "FAIL",
           mismatch, (unsigned long)target_sec);
}

/* --- helpers : little-endian unpack -------------------------------------- */
static inline uint16_t le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

/**
 * @brief  Sector 0 raw read (FatFs / HAL_BUSY 우회) + BPB/MBR 필드 해석
 *
 *  목적 :
 *    · LogBlockNbr 은 정상인데 OOR sector 가 요청되는 경우 → sector 0 의 데이터가
 *      BPB 로 해석 불가능한 garbage 거나 MBR 의 partition entry LBA 가 손상된 것
 *    · FatFs 의 sector 계산식 :
 *         root_dir_sec = RsvdSecCnt + (NumFATs × FATSz32) + (RootClus - 2) × SecPerClus
 *         fat_sec      = RsvdSecCnt + cluster / 128
 *      이 값들 중 하나라도 비정상이면 huge sector 요청 발생.
 *
 *  진단 절차 (사용자) :
 *    1. emmc bpb    → 현재 sector 0 상태 확인
 *    2. emmc format → 새 BPB 작성
 *    3. emmc bpb    → 새로 작성된 BPB 가 정상인지 비교
 *    4. 정상 사용 후 다시 fail 시점에 emmc bpb → corruption 여부 추적
 */
void EMMC_DumpBPB(void)
{
    ALIGN_32BYTES(static uint8_t rbuf[512]);

    /* HAL state 정리 후 polling read (DMA 우회 — bounce buffer 격리) */
    if (MMC_WaitCardReady(MMC_STATE_TIMEOUT_MS) != 0) {
        printf("[BPB] card not ready\r\n");
        return;
    }
    if (MMC_WaitHalReady(MMC_HAL_STATE_TIMEOUT_MS) != 0) {
        printf("[BPB] HAL not ready — proceeding after forced reset\r\n");
    }

    memset(rbuf, 0xCC, 512);
    SCB_InvalidateDCache_by_Addr((uint32_t *)rbuf, 512);

    HAL_StatusTypeDef rst = HAL_MMC_ReadBlocks(&hmmc1, rbuf, 0, 1, 5000);
    /* polling read 후 카드가 TRANSFER 로 복귀할 때까지 대기 */
    while (HAL_MMC_GetCardState(&hmmc1) != HAL_MMC_CARD_TRANSFER) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    SCB_InvalidateDCache_by_Addr((uint32_t *)rbuf, 512);

    if (rst != HAL_OK) {
        printf("[BPB] HAL_MMC_ReadBlocks FAILED status=%d Err=0x%08lX\r\n",
               rst, hmmc1.ErrorCode);
        return;
    }

    /* Hex dump — 처음 96 B (BPB 핵심 영역) + MBR partition entries (446..509) */
    printf("[BPB] ===== sector 0 raw dump =====\r\n");
    printf("[BPB] [000..05F] BPB / Boot record area\r\n");
    for (int row = 0; row < 6; row++) {
        printf("[BPB] %03X:", row * 16);
        for (int c = 0; c < 16; c++) printf(" %02X", rbuf[row * 16 + c]);
        printf("\r\n");
    }

    /* MBR partition entries at 0x1BE, 0x1CE, 0x1DE, 0x1EE (16B each) */
    printf("[BPB] [1BE..1FD] MBR partition entries\r\n");
    for (int p = 0; p < 4; p++) {
        const uint8_t *pe = &rbuf[0x1BE + p * 16];
        printf("[BPB] PT%d: boot=%02X type=%02X LBAstart=%lu NumSec=%lu\r\n",
               p, pe[0], pe[4], (unsigned long)le32(&pe[8]),
               (unsigned long)le32(&pe[12]));
    }
    printf("[BPB] [1FE]=0x%02X [1FF]=0x%02X (signature should be 55 AA)\r\n",
           rbuf[0x1FE], rbuf[0x1FF]);

    /* BPB 필드 해석 — FatFs 의 sector 계산에 직접 영향 주는 값들 */
    uint16_t BytsPerSec  = le16(&rbuf[0x0B]);
    uint8_t  SecPerClus  = rbuf[0x0D];
    uint16_t RsvdSecCnt  = le16(&rbuf[0x0E]);
    uint8_t  NumFATs     = rbuf[0x10];
    uint16_t RootEntCnt  = le16(&rbuf[0x11]);
    uint16_t TotSec16    = le16(&rbuf[0x13]);
    uint16_t FATSz16     = le16(&rbuf[0x16]);
    uint32_t TotSec32    = le32(&rbuf[0x20]);
    uint32_t FATSz32     = le32(&rbuf[0x24]);
    uint32_t RootClus    = le32(&rbuf[0x2C]);
    uint16_t FSInfo      = le16(&rbuf[0x30]);

    printf("[BPB] ----- decoded BPB fields -----\r\n");
    printf("[BPB] OEMName       : %.8s\r\n", &rbuf[0x03]);
    printf("[BPB] BytsPerSec    : %u\r\n", BytsPerSec);
    printf("[BPB] SecPerClus    : %u\r\n", SecPerClus);
    printf("[BPB] RsvdSecCnt    : %u\r\n", RsvdSecCnt);
    printf("[BPB] NumFATs       : %u\r\n", NumFATs);
    printf("[BPB] RootEntCnt    : %u\r\n", RootEntCnt);
    printf("[BPB] TotSec16      : %u\r\n", TotSec16);
    printf("[BPB] FATSz16       : %u\r\n", FATSz16);
    printf("[BPB] TotSec32      : %lu\r\n", (unsigned long)TotSec32);
    printf("[BPB] FATSz32       : %lu\r\n", (unsigned long)FATSz32);
    printf("[BPB] RootClus      : %lu (0x%08lX)\r\n",
           (unsigned long)RootClus, (unsigned long)RootClus);
    printf("[BPB] FSInfo        : %u\r\n", FSInfo);

    /* 정상성 sanity check — 비정상이면 손상 의심 */
    int sane = 1;
    if (BytsPerSec  != 512)                                    sane = 0;
    if (SecPerClus  == 0 || (SecPerClus & (SecPerClus - 1)))   sane = 0;
    if (RsvdSecCnt  == 0)                                      sane = 0;
    if (NumFATs     == 0 || NumFATs > 2)                       sane = 0;
    if (FATSz32     == 0 || FATSz32 > 0x00100000U)             sane = 0;
    if (RootClus    < 2  || RootClus > 0x0FFFFFFFU)            sane = 0;
    if (TotSec32    > 0x10000000U)                             sane = 0;
    if (rbuf[0x1FE] != 0x55 || rbuf[0x1FF] != 0xAA)            sane = 0;

    /* 계산된 sector 번호 (FatFs 가 실제로 요청하는 값) */
    if (sane) {
        uint64_t fat_start  = (uint64_t)RsvdSecCnt;
        uint64_t data_start = fat_start + (uint64_t)NumFATs * FATSz32;
        uint64_t root_sec   = data_start + (uint64_t)(RootClus - 2) * SecPerClus;
        printf("[BPB] >>> fat_start =%llu data_start=%llu root_sec=%llu (LBN=%lu)\r\n",
               (unsigned long long)fat_start,
               (unsigned long long)data_start,
               (unsigned long long)root_sec,
               (unsigned long)hmmc1.MmcCard.LogBlockNbr);
        printf("[BPB] >>> BPB SANE — FAT/root LBA all within LogBlockNbr\r\n");
    } else {
        printf("[BPB] >>> BPB CORRUPT — at least one field out of range\r\n");
        printf("[BPB] >>> Recovery: 'emmc format' will rewrite BPB cleanly\r\n");
    }
    printf("[BPB] ============================\r\n");
}

void EMMC_PrintCardInfo(void)
{
    HAL_MMC_CardInfoTypeDef info;
    if (HAL_MMC_GetCardInfo(&hmmc1, &info) != HAL_OK) {
        printf("[eMMC] GetCardInfo failed\r\n");
        return;
    }

    uint64_t totalBytes  = (uint64_t)info.LogBlockNbr * (uint64_t)info.LogBlockSize;
    uint32_t totalMB     = (uint32_t)(totalBytes / (1024ULL * 1024ULL));
    uint32_t totalGB_int = totalMB / 1024;
    uint32_t totalGB_fr  = (totalMB % 1024) * 10 / 1024;

    printf("[eMMC] ===== Card Info =====\r\n");
    printf("[eMMC] CardType    : %lu\r\n",     info.CardType);
    printf("[eMMC] CardClass   : 0x%04lX\r\n", info.Class);
    printf("[eMMC] RelCardAdd  : 0x%04lX\r\n", info.RelCardAdd);
    printf("[eMMC] BlockNbr    : %lu\r\n",     info.BlockNbr);
    printf("[eMMC] BlockSize   : %lu\r\n",     info.BlockSize);
    printf("[eMMC] LogBlockNbr : %lu\r\n",     info.LogBlockNbr);
    printf("[eMMC] LogBlockSize: %lu\r\n",     info.LogBlockSize);
    printf("[eMMC] Capacity    : %lu MB (%lu.%lu GB)\r\n",
           totalMB, totalGB_int, totalGB_fr);

    /* ★ FAT total/free 정보는 f_getfree 가 FAT 전체 스캔을 트리거 (16K+ sector read)
     *    → HAL_BUSY 폭주 원인. 별도 함수 EMMC_PrintFatUsage() 로 분리.
     *    필요 시 "emmc free" 명령으로 별도 호출. */
    if (getMountStatus()) {
        printf("[eMMC] Mount    : OK (use 'emmc free' for FAT usage)\r\n");
    } else {
        printf("[eMMC] Mount    : NOT mounted\r\n");
    }
    printf("[eMMC] =====================\r\n");
}

/**
 * @brief  FAT 사용량 별도 조회 — 필요할 때만 호출
 *  · f_getfree 는 FSInfo 캐시 invalid 시 FAT 전체 스캔 → 시간/race 위험
 *  · 정상 동작 가능하지만 무거운 작업이라 PrintCardInfo 에서 분리
 */
void EMMC_PrintFatUsage(void)
{
    if (!getMountStatus()) {
        printf("[eMMC] FAT usage: not mounted\r\n");
        return;
    }

    DWORD free_clusters = 0;
    FATFS *fs_ptr = NULL;
    FRESULT res = f_getfree(USERPath, &free_clusters, &fs_ptr);
    if (res != FR_OK || fs_ptr == NULL) {
        printf("[eMMC] f_getfree failed: %s (%d)\r\n", FR_Table[res], res);
        return;
    }

    DWORD    total_sect = (fs_ptr->n_fatent - 2) * fs_ptr->csize;
    DWORD    free_sect  = free_clusters * fs_ptr->csize;
    uint32_t total_MB   = (total_sect / 2) / 1024;
    uint32_t free_MB    = (free_sect  / 2) / 1024;
    printf("[eMMC] FAT total   : %lu MB\r\n", total_MB);
    printf("[eMMC] FAT free    : %lu MB\r\n", free_MB);
    printf("[eMMC] cluster sz  : %u sect (%lu B)\r\n",
           fs_ptr->csize, (uint32_t)fs_ptr->csize * 512U);
}

int32_t EMMC_RunBasicTest(void)
{
    const char    *test_path = "0:/test.bin";
    const uint32_t test_size = 4096;
    FIL            fp;
    FRESULT        res;
    UINT           bw, br;
    int32_t        ret = 0;

    if (!getMountStatus()) {
        printf("[EMMC TEST] not mounted -> abort\r\n");
        return -99;
    }

    printf("[EMMC TEST] === Basic R/W/Erase Test ===\r\n");

    /* Write phase */
    for (uint32_t i = 0; i < test_size; i++) g_FsWriteBuf[i] = (char)(i & 0xFF);
    res = f_open(&fp, test_path, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) { printf("[EMMC TEST] f_open(W) %s\r\n", FR_Table[res]); ret = -1; goto done; }
    res = f_write(&fp, g_FsWriteBuf, test_size, &bw);
    if (res != FR_OK || bw != test_size) {
        printf("[EMMC TEST] f_write %s (%u/%lu)\r\n", FR_Table[res], bw, test_size);
        f_close(&fp); ret = -2; goto done;
    }
    f_sync(&fp); f_close(&fp);
    printf("[EMMC TEST] [1/4] write %lu B OK\r\n", test_size);

    /* Read phase */
    memset(g_FsReadBuf, 0, sizeof(g_FsReadBuf));
    res = f_open(&fp, test_path, FA_READ);
    if (res != FR_OK) { printf("[EMMC TEST] f_open(R) %s\r\n", FR_Table[res]); ret = -3; goto done; }
    res = f_read(&fp, g_FsReadBuf, test_size, &br);
    if (res != FR_OK || br != test_size) {
        printf("[EMMC TEST] f_read %s (%u/%lu)\r\n", FR_Table[res], br, test_size);
        f_close(&fp); ret = -4; goto done;
    }
    f_close(&fp);
    printf("[EMMC TEST] [2/4] read  %lu B OK\r\n", test_size);

    /* Compare phase */
    for (uint32_t i = 0; i < test_size; i++) {
        if (g_FsReadBuf[i] != g_FsWriteBuf[i]) {
            printf("[EMMC TEST] mismatch @%lu: w=0x%02X r=0x%02X\r\n",
                   i, (uint8_t)g_FsWriteBuf[i], (uint8_t)g_FsReadBuf[i]);
            ret = -5; goto done;
        }
    }
    printf("[EMMC TEST] [3/4] compare OK\r\n");

    /* Erase phase */
    res = f_unlink(test_path);
    if (res != FR_OK) { printf("[EMMC TEST] f_unlink %s\r\n", FR_Table[res]); ret = -6; goto done; }

    FILINFO fno;
    res = f_stat(test_path, &fno);
    if (res != FR_NO_FILE) { printf("[EMMC TEST] still exists %s\r\n", FR_Table[res]); ret = -7; goto done; }
    printf("[EMMC TEST] [4/4] erase OK\r\n");
    printf("[EMMC TEST] === ALL PASSED ===\r\n");

done:
    if (ret != 0) printf("[EMMC TEST] === FAILED at step %ld ===\r\n", -ret);
    return ret;
}

void EMMC_PerfTest(void)
{
    const char    *path  = "0:/perf.bin";
    const uint32_t chunk = sizeof(g_FsWriteBuf);   /* 4096 */
    const uint32_t loops = 256;                    /* 1MB */
    FIL            fp;
    FRESULT        res;
    UINT           bw, br;

    if (!getMountStatus()) { printf("[EMMC PERF] not mounted\r\n"); return; }
    memset(g_FsWriteBuf, 0x5A, chunk);

    /* Write */
    res = f_open(&fp, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) { printf("[EMMC PERF] open(W) %s\r\n", FR_Table[res]); return; }

    uint32_t t0 = HAL_GetTick();
    for (uint32_t i = 0; i < loops; i++) {
        res = f_write(&fp, g_FsWriteBuf, chunk, &bw);
        if (res != FR_OK || bw != chunk) {
            printf("[EMMC PERF] write fail @%lu %s\r\n", i, FR_Table[res]);
            f_close(&fp); return;
        }
    }
    f_sync(&fp);
    uint32_t dt_w = HAL_GetTick() - t0;
    f_close(&fp);

    /* Read */
    res = f_open(&fp, path, FA_READ);
    if (res != FR_OK) { printf("[EMMC PERF] open(R) %s\r\n", FR_Table[res]); return; }

    t0 = HAL_GetTick();
    for (uint32_t i = 0; i < loops; i++) {
        res = f_read(&fp, g_FsReadBuf, chunk, &br);
        if (res != FR_OK || br != chunk) {
            printf("[EMMC PERF] read fail @%lu %s\r\n", i, FR_Table[res]);
            f_close(&fp); return;
        }
    }
    uint32_t dt_r = HAL_GetTick() - t0;
    f_close(&fp);
    f_unlink(path);

    uint32_t total_kb = (chunk * loops) / 1024;
    printf("[EMMC PERF] write %lu KB in %lu ms (%lu KB/s)\r\n",
           total_kb, dt_w, (dt_w > 0) ? (total_kb * 1000 / dt_w) : 0);
    printf("[EMMC PERF] read  %lu KB in %lu ms (%lu KB/s)\r\n",
           total_kb, dt_r, (dt_r > 0) ? (total_kb * 1000 / dt_r) : 0);
}

/* f_mkfs 후 sector 0 복구 (워크어라운드) */
int EMMC_RestoreSec0(void)
{
    if (!s_sec0_backup_valid) {
        printf("[eMMC] no VBR backup available\r\n");
        return -1;
    }

    memcpy(emmc_buffer, s_sec0_backup, EMMC_SECTOR_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)emmc_buffer, EMMC_SECTOR_SIZE);

    if (MMC_WaitCardReady(MMC_STATE_TIMEOUT_MS) != 0) {
        printf("[eMMC] sec0 restore: not ready\r\n"); return -2;
    }
    if (HAL_MMC_WriteBlocks_DMA(HMMC, emmc_buffer, 0, 1) != HAL_OK) {
        printf("[eMMC] sec0 restore: write fail\r\n"); return -2;
    }
    if (xSemaphoreTake(s_fatfs_sema_handle, pdMS_TO_TICKS(MMC_DMA_TIMEOUT_MS)) != pdTRUE) {
        printf("[eMMC] sec0 restore: write timeout\r\n"); return -2;
    }
    MMC_WaitCardReady(MMC_STATE_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Verify */
    static uint8_t verify_buf[EMMC_SECTOR_SIZE] __attribute__((aligned(32)));
    if (HAL_MMC_ReadBlocks_DMA(HMMC, verify_buf, 0, 1) != HAL_OK) {
        printf("[eMMC] sec0 restore: verify read fail\r\n"); return -3;
    }
    if (xSemaphoreTake(s_fatfs_sema_handle, pdMS_TO_TICKS(MMC_DMA_TIMEOUT_MS)) != pdTRUE) {
        printf("[eMMC] sec0 restore: verify timeout\r\n"); return -3;
    }
    SCB_InvalidateDCache_by_Addr((uint32_t *)verify_buf, EMMC_SECTOR_SIZE);

    if (memcmp(s_sec0_backup, verify_buf, EMMC_SECTOR_SIZE) != 0) {
        printf("[eMMC] sec0 restore: MISMATCH\r\n");
        return -3;
    }
    printf("[eMMC] sec0 restore: VERIFIED (sig=%02X%02X)\r\n",
           verify_buf[510], verify_buf[511]);
    return 0;
}

/*=============================================================================
 *  [3] TASK — FreeRTOS emmcTask + 비동기 명령 큐
 *=============================================================================*/

/* 사용자 path 에 텍스트 write — emmcTask 내부에서 호출 */
static FRESULT EMMC_WriteTextFile(const char *path, const char *data)
{
    FIL     fp;
    FRESULT res;
    UINT    bw;
    size_t  len;

    if (!getMountStatus()) {
        printf("[EMMC WFILE] not mounted -> abort\r\n");
        return FR_NOT_READY;
    }
    if (!path || !data || path[0] == '\0') {
        printf("[EMMC WFILE] invalid args\r\n");
        return FR_INVALID_PARAMETER;
    }

    res = f_open(&fp, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        printf("[EMMC WFILE] f_open(W) \"%s\" : %s\r\n", path, FR_Table[res]);
        return res;
    }

    len = strlen(data);
    res = f_write(&fp, data, (UINT)len, &bw);
    if (res != FR_OK || bw != (UINT)len) {
        printf("[EMMC WFILE] f_write fail %s (wrote %u/%u)\r\n",
               FR_Table[res], bw, (unsigned)len);
        f_close(&fp);
        return res;
    }

    f_sync(&fp);
    f_close(&fp);
    printf("[EMMC WFILE] OK \"%s\" wrote %u B : \"%s\"\r\n", path, bw, data);
    return FR_OK;
}

/* 사용자 path 읽어서 콘솔에 출력 — emmcTask 내부에서 호출 */
static FRESULT EMMC_ReadTextFile(const char *path)
{
    FIL         fp;
    FRESULT     res;
    UINT        br;
    FSIZE_t     fsize;
    UINT        to_read;
    static char readbuf[EMMC_DATA_MAX];  /* emmcTask 전용 (다른 호출자와 공유 X) */

    if (!getMountStatus()) {
        printf("[EMMC RFILE] not mounted -> abort\r\n");
        return FR_NOT_READY;
    }
    if (!path || path[0] == '\0') {
        printf("[EMMC RFILE] invalid args\r\n");
        return FR_INVALID_PARAMETER;
    }

    res = f_open(&fp, path, FA_READ);
    if (res != FR_OK) {
        printf("[EMMC RFILE] f_open(R) \"%s\" : %s\r\n", path, FR_Table[res]);
        return res;
    }

    fsize   = f_size(&fp);
    to_read = (fsize > (FSIZE_t)(EMMC_DATA_MAX - 1)) ? (UINT)(EMMC_DATA_MAX - 1) : (UINT)fsize;

    memset(readbuf, 0, sizeof(readbuf));
    res = f_read(&fp, readbuf, to_read, &br);
    f_close(&fp);

    if (res != FR_OK) {
        printf("[EMMC RFILE] f_read fail %s\r\n", FR_Table[res]);
        return res;
    }
    if (br < sizeof(readbuf)) readbuf[br] = '\0';
    else                      readbuf[sizeof(readbuf) - 1] = '\0';

    printf("[EMMC RFILE] OK \"%s\" size=%lu B, read=%u B : \"%s\"\r\n",
           path, (uint32_t)fsize, br, readbuf);
    if (fsize > br) {
        printf("[EMMC RFILE] truncated (buffer max %u B)\r\n", EMMC_DATA_MAX - 1);
    }
    return FR_OK;
}

/* 디렉토리 내용 조회 (f_opendir + f_readdir 반복)
 *   path 가 비어있거나 NULL 이면 "0:/" 사용                                  */
static FRESULT EMMC_ListDir(const char *path)
{
    DIR        dir;
    FILINFO    fno;
    FRESULT    res;
    const char *p = (path && path[0] != '\0') ? path : "0:/";
    int        count       = 0;
    uint32_t   total_bytes = 0;
    int        n_dir       = 0;
    int        n_file      = 0;

    if (!getMountStatus()) {
        printf("[EMMC LS] not mounted -> abort\r\n");
        return FR_NOT_READY;
    }

    res = f_opendir(&dir, p);
    if (res != FR_OK) {
        printf("[EMMC LS] f_opendir \"%s\" : %s\r\n", p, FR_Table[res]);
        return res;
    }

    printf("[EMMC LS] ===== Listing \"%s\" =====\r\n", p);
    printf("[EMMC LS]  Type  Size         Date       Time   Name\r\n");
    printf("[EMMC LS]  ----  -----------  ---------- -----  --------------------------\r\n");

    for (;;) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK)    { printf("[EMMC LS] f_readdir : %s\r\n", FR_Table[res]); break; }
        if (fno.fname[0] == '\0') break;       /* 끝 */

        bool     is_dir = (fno.fattrib & AM_DIR) != 0;
        uint16_t year   = (fno.fdate >> 9) + 1980;
        uint8_t  mon    = (fno.fdate >> 5) & 0x0F;
        uint8_t  day    =  fno.fdate       & 0x1F;
        uint8_t  hr     = (fno.ftime >> 11) & 0x1F;
        uint8_t  min    = (fno.ftime >> 5)  & 0x3F;

        printf("[EMMC LS]  %s  %11lu  %04u-%02u-%02u %02u:%02u  %s%s\r\n",
               is_dir ? "DIR " : "FILE",
               (uint32_t)fno.fsize,
               year, mon, day, hr, min,
               fno.fname,
               is_dir ? "/" : "");

        if (is_dir) n_dir++;
        else        { n_file++; total_bytes += fno.fsize; }
        count++;
    }

    f_closedir(&dir);

    printf("[EMMC LS] ===== %d entries (dir=%d, file=%d), total %lu bytes =====\r\n",
           count, n_dir, n_file, total_bytes);
    return FR_OK;
}

/* 새 폴더 생성 (f_mkdir)
 *   path 예 :  "0:/logs"  /  "0:/logs/2026"                                  */
static FRESULT EMMC_MakeDir(const char *path)
{
    FRESULT res;

    if (!getMountStatus()) {
        printf("[EMMC MKDIR] not mounted -> abort\r\n");
        return FR_NOT_READY;
    }
    if (!path || path[0] == '\0') {
        printf("[EMMC MKDIR] invalid path\r\n");
        return FR_INVALID_PARAMETER;
    }

    res = f_mkdir(path);
    if (res == FR_OK) {
        printf("[EMMC MKDIR] OK \"%s\" created\r\n", path);
    } else if (res == FR_EXIST) {
        printf("[EMMC MKDIR] \"%s\" already exists (skipped)\r\n", path);
    } else {
        printf("[EMMC MKDIR] FAIL \"%s\" : %s (%d)\r\n", path, FR_Table[res], res);
    }
    return res;
}

/* 단일 파일/빈 폴더 삭제 (f_unlink)
 *   f_stat 으로 먼저 존재 확인 → 없으면 skip, 있으면 삭제.
 *   주의: 내용이 있는 폴더는 f_unlink 가 FR_DENIED 반환 — 통째 삭제 불가.  */
static FRESULT EMMC_DeletePath(const char *path)
{
    FRESULT res;

    if (!getMountStatus()) {
        printf("[EMMC DEL] not mounted -> abort\r\n");
        return FR_NOT_READY;
    }
    if (!path || path[0] == '\0') {
        printf("[EMMC DEL] invalid path\r\n");
        return FR_INVALID_PARAMETER;
    }

    FILINFO fno;
    res = f_stat(path, &fno);
    if (res == FR_NO_FILE || res == FR_NO_PATH) {
        printf("[EMMC DEL] \"%s\" not found (skip)\r\n", path);
        return FR_OK;
    }
    if (res != FR_OK) {
        printf("[EMMC DEL] stat FAIL \"%s\" : %s (%d)\r\n", path, FR_Table[res], res);
        return res;
    }

    res = f_unlink(path);
    if (res == FR_OK) {
        printf("[EMMC DEL] OK \"%s\" deleted\r\n", path);
    } else if (res == FR_DENIED) {
        printf("[EMMC DEL] DENIED \"%s\" (read-only or non-empty dir)\r\n", path);
    } else {
        printf("[EMMC DEL] FAIL \"%s\" : %s (%d)\r\n", path, FR_Table[res], res);
    }
    return res;
}

void InitEmmcTask(void)
{
    osMessageQueueAttr_t qattr = {
        .name    = "emmcQ",
        .cb_mem  = &s_emmcCmdQueueCB,
        .cb_size = sizeof(s_emmcCmdQueueCB),
        .mq_mem  = s_emmcCmdQueueStorage,
        .mq_size = sizeof(s_emmcCmdQueueStorage),
    };
    s_emmcCmdQueue   = osMessageQueueNew(EMMC_QUEUE_DEPTH, sizeof(EmmcMsg_t), &qattr);
    s_emmcTaskHandle = osThreadNew(StartEmmcTask, NULL, &s_emmcTask_attr);
}

bool EMMC_PostCmd(EmmcCmd_t cmd)
{
    if (!s_emmcCmdQueue) return false;
    EmmcMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd = cmd;
    return (osMessageQueuePut(s_emmcCmdQueue, &msg, 0, 0) == osOK);
}

bool EMMC_PostWriteFile(const char *path, const char *data)
{
    if (!s_emmcCmdQueue) return false;
    if (!path || !data)  return false;
    EmmcMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd = EMMC_CMD_WFILE;
    strncpy(msg.path, path, EMMC_PATH_MAX - 1);
    msg.path[EMMC_PATH_MAX - 1] = '\0';
    strncpy(msg.data, data, EMMC_DATA_MAX - 1);
    msg.data[EMMC_DATA_MAX - 1] = '\0';
    return (osMessageQueuePut(s_emmcCmdQueue, &msg, 0, 0) == osOK);
}

bool EMMC_PostReadFile(const char *path)
{
    if (!s_emmcCmdQueue) return false;
    if (!path)           return false;
    EmmcMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd = EMMC_CMD_RFILE;
    strncpy(msg.path, path, EMMC_PATH_MAX - 1);
    msg.path[EMMC_PATH_MAX - 1] = '\0';
    return (osMessageQueuePut(s_emmcCmdQueue, &msg, 0, 0) == osOK);
}

bool EMMC_PostListDir(const char *path)
{
    if (!s_emmcCmdQueue) return false;
    EmmcMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd = EMMC_CMD_LS;
    /* path 가 NULL/빈 문자열이면 "0:/" 기본값 사용 */
    if (path && path[0] != '\0') {
        strncpy(msg.path, path, EMMC_PATH_MAX - 1);
        msg.path[EMMC_PATH_MAX - 1] = '\0';
    } else {
        strncpy(msg.path, "0:/", EMMC_PATH_MAX - 1);
    }
    return (osMessageQueuePut(s_emmcCmdQueue, &msg, 0, 0) == osOK);
}

bool EMMC_PostMakeDir(const char *path)
{
    if (!s_emmcCmdQueue) return false;
    if (!path || path[0] == '\0') return false;
    EmmcMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd = EMMC_CMD_MKDIR;
    strncpy(msg.path, path, EMMC_PATH_MAX - 1);
    msg.path[EMMC_PATH_MAX - 1] = '\0';
    return (osMessageQueuePut(s_emmcCmdQueue, &msg, 0, 0) == osOK);
}

bool EMMC_PostDelete(const char *path)
{
    if (!s_emmcCmdQueue) return false;
    if (!path || path[0] == '\0') return false;
    EmmcMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd = EMMC_CMD_DEL;
    strncpy(msg.path, path, EMMC_PATH_MAX - 1);
    msg.path[EMMC_PATH_MAX - 1] = '\0';
    return (osMessageQueuePut(s_emmcCmdQueue, &msg, 0, 0) == osOK);
}

static void StartEmmcTask(void *argument)
{
    (void)argument;
    EmmcMsg_t msg;

    printf("start %s ... \r\n", __FUNCTION__);

    for (;;)
    {
        if (osMessageQueueGet(s_emmcCmdQueue, &msg, NULL, osWaitForever) != osOK) continue;

        // printf("[eMMC] stack HWM before: %lu words\r\n",
        //        (unsigned long)uxTaskGetStackHighWaterMark(NULL));

        switch (msg.cmd) {
            case EMMC_CMD_INFO:
                printf("[eMMC] >>> INFO\r\n");
                EMMC_PrintCardInfo();
                break;
            case EMMC_CMD_TEST:
                printf("[eMMC] >>> TEST\r\n");
                (void)EMMC_RunBasicTest();
                break;
            case EMMC_CMD_PERF:
                printf("[eMMC] >>> PERF\r\n");
                EMMC_PerfTest();
                break;
            case EMMC_CMD_FORMAT:
                printf("[eMMC] >>> FORMAT (all data will be lost)\r\n");
                {
                    FRESULT r = formatEmmc();
                    printf("[eMMC] FORMAT result: %d\r\n", (int)r);
                }
                break;
            case EMMC_CMD_RAW:
                printf("[eMMC] >>> RAW sector 0 test\r\n");
                EMMC_RawSector0Test();
                break;
            case EMMC_CMD_WFILE:
                printf("[eMMC] >>> WRITE FILE \"%s\"\r\n", msg.path);
                (void)EMMC_WriteTextFile(msg.path, msg.data);
                break;
            case EMMC_CMD_RFILE:
                printf("[eMMC] >>> READ FILE \"%s\"\r\n", msg.path);
                (void)EMMC_ReadTextFile(msg.path);
                break;
            case EMMC_CMD_FREE:
                printf("[eMMC] >>> FAT FREE (f_getfree, 시간 소요)\r\n");
                EMMC_PrintFatUsage();
                break;
            case EMMC_CMD_LS:
                printf("[eMMC] >>> LS \"%s\"\r\n", msg.path);
                (void)EMMC_ListDir(msg.path);
                break;
            case EMMC_CMD_MKDIR:
                printf("[eMMC] >>> MKDIR \"%s\"\r\n", msg.path);
                (void)EMMC_MakeDir(msg.path);
                break;
            case EMMC_CMD_DEL:
                printf("[eMMC] >>> DEL \"%s\"\r\n", msg.path);
                (void)EMMC_DeletePath(msg.path);
                break;
            case EMMC_CMD_BPB:
                printf("[eMMC] >>> BPB (sector 0 raw dump)\r\n");
                EMMC_DumpBPB();
                break;
            case EMMC_CMD_UNMOUNT:
                printf("[eMMC] >>> UNMOUNT (USB MSC enter)\r\n");
                (void)unmountFatFS();   /* 캐시 flush + 소유권 PC 에 양도 */
                break;
            case EMMC_CMD_MOUNT:
                printf("[eMMC] >>> MOUNT (USB MSC exit)\r\n");
                (void)mountFatFS();     /* fresh 재마운트 → FAT 캐시 갱신 */
                break;
            default:
                printf("[eMMC] unknown cmd: %d\r\n", (int)msg.cmd);
                break;
        }

        // printf("[eMMC] stack HWM after: %lu words\r\n",
        //        (unsigned long)uxTaskGetStackHighWaterMark(NULL));
        printf("[eMMC] <<< done\r\n");
    }
}
