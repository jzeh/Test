/**
  ******************************************************************************
  * @file    ffconf.h  (Boot Read-Only 구성)
  * @brief   FatFs 설정 — Boot 컨텍스트 전용
  *
  *  Appli/FATFS/Target/ffconf.h (RW, LFN, RTOS-aware)와 다름:
  *    - FF_FS_READONLY = 1     write API 제거 → ROM ~8KB 절감
  *    - FF_USE_LFN     = 0     ffunicode.c 불필요 → ROM ~3KB 절감
  *    - FF_FS_REENTRANT= 0     Boot은 single-thread
  *    - FF_FS_LOCK     = 0     RO 강제
  *    - FF_USE_STRFUNC = 1     f_gets() 사용 (AppSwList.ini 파싱)
  *
  *  Boot.ewp 의 Include Path 우선순위:
  *    Boot/FATFS/Target  →  이 ffconf.h 가 먼저 잡힘
  *    Appli/FATFS/Target →  Boot 빌드에는 제외
  ******************************************************************************
  */
#define FFCONF_DEF 80286

/* CubeMX legacy guards — disk_write/ioctl 함수가 user_diskio.c 와 무관하게
 * boot_diskio.c 에서 항상 활성 (그러나 RO 모드에서 disk_write 는 호출되지 않음). */
#define _USE_WRITE   1
#define _USE_IOCTL   1

#include "main.h"
#include "stm32h7rsxx_hal.h"

/*---------------------------------------------------------------------------/
/ Function Configurations
/---------------------------------------------------------------------------*/

#define FF_FS_READONLY  1   /* Read-only — Boot은 eMMC 쓰기 금지 (Plan §6) */
#define FF_FS_MINIMIZE  0   /* f_stat / f_unlink 등 기본 함수 유지 */

#define FF_USE_FIND     0
#define FF_USE_MKFS     0
#define FF_USE_FASTSEEK 0
#define FF_USE_EXPAND   0
#define FF_USE_CHMOD    0   /* RO이면 자동 0 */
#define FF_USE_LABEL    0
#define FF_USE_FORWARD  0

/* f_gets() 만 사용 (AppSwList.ini 라인 파싱).
 * f_putc/puts/printf 는 RO에서 자동 비활성. */
#define FF_USE_STRFUNC  1
#define FF_PRINT_LLI    0
#define FF_PRINT_FLOAT  0
#define FF_STRF_ENCODE  0

/*---------------------------------------------------------------------------/
/ Locale and Namespace
/---------------------------------------------------------------------------*/

#define FF_CODE_PAGE    437   /* ASCII U.S. — Boot은 ASCII만 처리 */

#define FF_USE_LFN      1     /* Long File Name 활성(static 버퍼) — Appli가 롱네임으로 생성한
                               * 01_Application / FirmwareInfo.ini / *.bin 인식 필수.
                               * ffunicode.c 를 Boot 빌드에 포함해야 함. */
#define FF_MAX_LFN      255
#define FF_LFN_UNICODE  0
#define FF_LFN_BUF      255
#define FF_SFN_BUF      12
#define FF_FS_RPATH     0

/*---------------------------------------------------------------------------/
/ Drive/Volume Configurations
/---------------------------------------------------------------------------*/

#define FF_VOLUMES        1
#define FF_STR_VOLUME_ID  0
#define FF_VOLUME_STRS    "RAM","NAND","CF","SD","SD2","USB","USB2","USB3"
#define FF_MULTI_PARTITION 0

#define FF_MIN_SS  512
#define FF_MAX_SS  512
#define FF_LBA64   0
#define FF_MIN_GPT valueNotSetted
#define FF_USE_TRIM 0

/*---------------------------------------------------------------------------/
/ System Configurations
/---------------------------------------------------------------------------*/

#define FF_FS_TINY      0
#define FF_FS_EXFAT     0

/* RTC 없이 timestamp 사용 안 함 — RO이므로 timestamp는 의미 없음. */
#define FF_FS_NORTC     1
#define FF_NORTC_MON    1
#define FF_NORTC_MDAY   1
#define FF_NORTC_YEAR   2026

#define FF_FS_NOFSINFO  0

#define FF_FS_LOCK      0     /* RO 모드 → 0 강제 */
#define FF_FS_REENTRANT 0     /* Boot single-thread — mutex 불필요 */
#define FF_FS_TIMEOUT   1000
