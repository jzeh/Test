/**
  ******************************************************************************
  * @file    fw_compat.h
  * @brief   FW Update 모듈의 evcd_test 환경 호환 어댑터
  *
  *  frp-scan 원본 코드는 다음 외부 심볼에 의존:
  *    - GLogI / GLogE        (common.h)        — UART 로깅
  *    - HAL_IWDG_Refresh()   (git_iwdg.h)      — 워치독 갱신
  *    - hxspi1               (xspi.h)          — XSPI HAL 핸들
  *
  *  evcd_test에는 IWDG 인스턴스가 없고 자체 로깅을 사용한다. 이 어댑터에서
  *  환경 차이를 한 곳에 격리하여 frp-scan 원본 코드 수정을 최소화한다.
  *
  *  활성화 옵션 (빌드 설정 -D...):
  *    FW_COMPAT_VERBOSE=1   → 로깅 활성 (printf)
  *    FW_COMPAT_USE_IWDG=1  → 워치독 활성 (extern hiwdg 필요)
  ******************************************************************************
  */
#ifndef __FW_COMPAT_H__
#define __FW_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"               /* stm32h7rsxx_hal.h */

/* EXTMEM 매니저는 Boot 전용 (Appli 의 .ioc Appli.IPs 에 EXTMEM_MANAGER 없음).
 * fw_flash_xspi.c 가 EXTMEM_* 호출 — Boot 빌드에서만 link 됨.
 * Appli 는 fw_flash_xspi.c 자체를 link 하지 않으므로 헤더도 불필요. */
#ifdef BOOT_BUILD
#include "extmem_manager.h"     /* hxspi1, EXTMEM_* APIs */
#endif

/*============================================================================
 *  1. Logging shim
 *===========================================================================*/

#ifndef FW_COMPAT_VERBOSE
#define FW_COMPAT_VERBOSE   0
#endif

#if FW_COMPAT_VERBOSE
  #include <stdio.h>
  #define GLogI(...)    do { printf(__VA_ARGS__); } while (0)
  #define GLogE(...)    do { printf(__VA_ARGS__); } while (0)
#else
  #define GLogI(...)    ((void)0)
  #define GLogE(...)    ((void)0)
#endif

/*============================================================================
 *  2. IWDG shim — 원본 .c 코드는 FW_IWDG_REFRESH() 매크로를 호출하도록 수정됨.
 *     evcd_test에 향후 IWDG 추가 시 FW_COMPAT_USE_IWDG=1 로 빌드하면 자동 연결.
 *===========================================================================*/

#ifndef FW_COMPAT_USE_IWDG
#define FW_COMPAT_USE_IWDG  0
#endif

#if FW_COMPAT_USE_IWDG
  extern IWDG_HandleTypeDef hiwdg;
  #define FW_IWDG_REFRESH()   ((void)HAL_IWDG_Refresh(&hiwdg))
#else
  #define FW_IWDG_REFRESH()   ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __FW_COMPAT_H__ */
