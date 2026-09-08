/**
 ******************************************************************************
 * @file    sys-emmc.h
 * @brief   eMMC + FatFs 통합 헤더 — STM32H7S3I8 + KLM8G1GETF-B041
 *
 *  통합 범위 (구 git-emmc.h + task-emmc.h + 기존 sys-emmc.h) :
 *    · Backend       : emmc_*()  — user_diskio.c 가 호출 (DMA + Semaphore)
 *    · Management    : InitEMMC, mountFatFS, formatEmmc, EMMC_PrintCardInfo,
 *                      EMMC_RunBasicTest, EMMC_PerfTest, EMMC_RawSector0Test 등
 *    · Task          : EmmcCmd_t / InitEmmcTask / EMMC_PostCmd
 *                      (CLI → 비동기 명령 디스패치)
 *    · Folder consts : EMMC_APPLICATION_FOLDER 등
 *
 *  user_diskio.c/h 는 FatFs middleware (CubeMX 생성) 위치 유지.
 ******************************************************************************
 */

#ifndef SYS_EMMC_H
#define SYS_EMMC_H

#include <stdint.h>
#include <stdbool.h>
#include "ff_gen_drv.h"
#include "ff.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 *  Folder / File constants
 * --------------------------------------------------------------------------- */
#define EMMC_ROOT_FOLDER              "/"
#define EMMC_APPLICATION_FOLDER       "01_Application"
#define EMMC_BACKUP_FOLDER            "02_Backup"
#define MAIN_BACKUP_FILE_NAME         "Backup/mainApplication.bin"

/* ---------------------------------------------------------------------------
 *  Task command enum (CLI ↔ emmcTask 비동기 디스패치)
 * --------------------------------------------------------------------------- */
typedef enum {
    EMMC_CMD_INFO   = 1,   /* HAL_MMC_GetCardInfo (FAT scan 안 함, 빠름)             */
    EMMC_CMD_TEST   = 2,   /* write/read/compare/erase 4KB                          */
    EMMC_CMD_PERF   = 3,   /* 1MB throughput                                        */
    EMMC_CMD_FORMAT = 4,   /* f_mkfs (모든 데이터 소실)                              */
    EMMC_CMD_RAW    = 5,   /* HAL 직접 sector 0 W/R 검증 (FatFs 우회)                */
    EMMC_CMD_WFILE  = 6,   /* 사용자 지정 path 에 텍스트 write                       */
    EMMC_CMD_RFILE  = 7,   /* 사용자 지정 path 읽어서 콘솔에 출력                    */
    EMMC_CMD_FREE   = 8,   /* FAT total/free 조회 (f_getfree, 시간 소요 가능)        */
    EMMC_CMD_LS     = 9,   /* 디렉토리 내용 조회 (f_opendir + f_readdir)             */
    EMMC_CMD_MKDIR  = 10,  /* 새 폴더 생성 (f_mkdir)                                 */
    EMMC_CMD_BPB    = 11,  /* sector 0 raw read + BPB/MBR 필드 해석 (디스크 진단)    */
    EMMC_CMD_DEL    = 12,  /* 단일 파일/빈 폴더 삭제 (f_unlink)                      */
    EMMC_CMD_UNMOUNT= 13,  /* USB MSC 진입 — FatFs 언마운트 (PC 에 소유권 양도)       */
    EMMC_CMD_MOUNT  = 14   /* USB MSC 이탈 — FatFs 재마운트 (캐시 갱신)               */
} EmmcCmd_t;

/* 파일 경로 / 데이터 버퍼 크기 — queue 메시지 안에 동봉 (thread-safe) */
#define EMMC_PATH_MAX               64U
#define EMMC_DATA_MAX               256U

typedef enum {
    FILE_WRITE_OPEN = 0,
    FILE_READ_OPEN,
    FILE_CLOSE,
    FILE_WRITE,
    FILE_READ,
    FILE_REMOVE,
    FILE_LIST,
    FILE_SYNC,
    FILE_FS_FORMAT
} OperationStateTypeDef;

typedef enum {
    ERROR_CODE_NONE_ERROR   = 0,
    ERROR_CODE_EMMC_MOUNT   = 1,
    ERROR_CODE_EMMC_OPEN    = 2,
    ERROR_CODE_EMMC_WRITE   = 3,
    ERROR_CODE_EMMC_READ    = 4,
    ERROR_CODE_EMMC_ERASE   = 5,
    ERROR_CODE_EMMC_COMPARE = 6
} eEMMC_ERROR_CODE;

/* ---------------------------------------------------------------------------
 *  Backend API — user_diskio.c 가 호출 (FatFs disk I/O 매핑)
 * --------------------------------------------------------------------------- */
DSTATUS emmc_initialize(BYTE pdrv);
DSTATUS emmc_status    (BYTE pdrv);
DRESULT emmc_read      (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
DRESULT emmc_write     (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif
#if _USE_IOCTL == 1
DRESULT emmc_ioctl     (BYTE pdrv, BYTE cmd, void *buff);
#endif

/* ---------------------------------------------------------------------------
 *  Management API — 사용자 코드 (main.c, sys-main.c, task-cli.c) 가 호출
 * --------------------------------------------------------------------------- */
int32_t  InitEMMC          (void);
void     deinitMMC         (void);

FRESULT  mountFatFS        (void);
FRESULT  unmountFatFS      (void);
uint8_t  getMountStatus    (void);
FRESULT  formatEmmc        (void);
uint8_t  InitEmmcFolder    (void);

/* Card info / Test routines (디버그/진단용) */
void     EMMC_PrintCardInfo(void);
void     EMMC_PrintFatUsage(void);     /* f_getfree 별도 호출 (시간 소요)        */
int32_t  EMMC_RunBasicTest (void);
void     EMMC_PerfTest     (void);
void     EMMC_RawSector0Test(void);
void     EMMC_DumpBPB      (void);     /* sector 0 raw read + BPB/MBR 해석       */

/* Sector 0 backup/restore (f_mkfs 워크어라운드) */
int      EMMC_RestoreSec0  (void);

/* ---------------------------------------------------------------------------
 *  USB MSC backend — usbd_storage_if.c 가 호출 (★ USB ISR 컨텍스트에서 실행 ★)
 *    · RTOS API 미사용(폴링 HAL) → ISR-safe
 *    · 전제: USB MSC 모드 동안 FatFs 언마운트 + emmcTask idle → hmmc1 단독 점유
 *    · 반환: 0 = 성공, -1 = 실패
 * --------------------------------------------------------------------------- */
int8_t   EMMC_MSC_IsReady     (void);
int8_t   EMMC_MSC_GetCapacity (uint32_t *block_num, uint16_t *block_size);
int8_t   EMMC_MSC_Read        (uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t   EMMC_MSC_Write       (uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);

/* ---------------------------------------------------------------------------
 *  Task API — emmcTask (FreeRTOS) 관련
 * --------------------------------------------------------------------------- */
void     InitEmmcTask      (void);
bool     EMMC_PostCmd      (EmmcCmd_t cmd);

/* 파일 경로 + 데이터 인자가 필요한 비동기 명령 */
bool     EMMC_PostWriteFile(const char *path, const char *data);
bool     EMMC_PostReadFile (const char *path);
bool     EMMC_PostListDir  (const char *path);   /* "" 또는 "0:/" 도 가능 */
bool     EMMC_PostMakeDir  (const char *path);
bool     EMMC_PostDelete   (const char *path);   /* 단일 파일/빈 폴더 삭제 */

void EMMC_Disable( void );

/* ---------------------------------------------------------------------------
 *  Device Serial — BLE 광고 이름 suffix ("BDC" + 8자리), eMMC 영속 + write-once
 *    저장 파일: DEVSERIAL_FILE. 제조 시 BLE(0xB1)로 1회 설정.
 *    Get 은 RAM 캐시 반환(어느 태스크서든 안전), Load/Set 은 직접 FATFS 접근.
 * --------------------------------------------------------------------------- */
#define DEVSERIAL_FILE        "DeviceSerial.bin"
#define DEVSERIAL_PREAMBLE    0x44455631U     /* "DEV1" — preamble 컨벤션 */
#define DEVSERIAL_LOCK_MARK   0xAAU           /* write-once 잠김 (SLOT_VALID_MARK 동일) */
#define DEVSERIAL_SUFFIX_LEN  8U              /* BDC + 8자리 = 11자 (<=20 BLE 한도) */

#define GDS_FID_SET_SERIAL    0xB1U           /* payload: [char suffix[8]] */
#define GDS_FID_GET_SERIAL    0xB2U           /* 응답 payload: suffix[8] */

void        DeviceSerial_Load(void);          /* 부팅 시 1회 (InitEMMC 내부 호출) */
const char* DeviceSerial_Get(void);           /* suffix 문자열, 미설정 "00000000" */
uint8_t     DeviceSerial_IsLocked(void);      /* 1 = write-once 잠김 */
int         DeviceSerial_Set(const char *suffix); /* 0=성공, -1=잠김, -2=형식오류, -3=IO */

#ifdef __cplusplus
}
#endif

#endif /* SYS_EMMC_H */
