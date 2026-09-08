/**
  ******************************************************************************
  * @file    firmware_lite.h
  * @brief   evcd_test 단순화 펌웨어 정보 정의 (frp-scan firmware.h 대체)
  *
  *  frp-scan의 firmware.h는 MAX_APP_CNT=40 다중앱을 지원하지만,
  *  evcd_test는 단일 앱 + Recovery FW 1개 구성이므로 단순화.
  *
  *  Design Ref: plans/frp-scan-fw-velvety-toucan.md §4 (firmware_lite.h)
  ******************************************************************************
  */
#ifndef __FIRMWARE_LITE_H__
#define __FIRMWARE_LITE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*============================================================================
 *  1. App enumeration
 *     evcd_test는 단일 운영앱 + Recovery FW 구성.
 *     SAFE_MODE_APP은 복구 cascade Stage 3에서 강제 로드됨.
 *===========================================================================*/

#define MAX_APP_CNT       2U

typedef enum {
    eApp_None     = 0,
    eApp_Main     = 1,    /* 운영 펌웨어 — 차상충전/방전/BSA방전 전체 기능 */
    eApp_Recovery = 2,    /* 최후 복구 펌웨어 — BLE OTA + eMMC + 경광등만 */
    eApp_MAX
} AppName;

#define SAFE_MODE_APP     ((uint8_t)eApp_Recovery)

/*============================================================================
 *  2. FirmwareInfo.ini 바이너리 구조체 (eMMC 영속 저장)
 *     Boot이 부팅 시 읽어 부팅 모드 결정.
 *===========================================================================*/

#define VCI3_FWINFO_PREAMBLE  0x45564344U   /* "EVCD" little-endian */

typedef struct {
    uint8_t marrucVersion[3];   /* M.m.p — 슬롯과 매칭되는 버전 */
    uint8_t pad;
    char    marrcFilename[32];  /* "main.bin" / "recovery.bin" */
} SAppInfo;

typedef struct {
    uint32_t muiPreamble;         /* VCI3_FWINFO_PREAMBLE */
    uint8_t  mucInitialized;      /* 0xAA = 정상 초기화 */
    uint8_t  mucBootMode;         /* 다음 부팅에 띄울 앱 (AppName) */
    uint8_t  mucCurrentMode;      /* 현재 실행 중인 앱 (AppName) */
    uint8_t  pad;
    SAppInfo msAppInfo[MAX_APP_CNT];
} SFwInfo;

/*============================================================================
 *  3. Constants
 *===========================================================================*/

#define APPLICATION_INFO_FILE_NAME   "AppSwList.ini"
#define BOOT_FWINFO_FILE             "FirmwareInfo.ini"

#ifdef __cplusplus
}
#endif

#endif /* __FIRMWARE_LITE_H__ */
