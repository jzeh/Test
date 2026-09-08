/**
  ******************************************************************************
  * @file    fw_flash_xspi.h
  * @brief   XSPI1 External NOR Flash Programming API (evcd_test 포팅)
  *
  *  ST EXTMEM middleware (EXTMEMORY_1 = XSPI1 NOR SFDP) 래퍼.
  *  Boot 컨텍스트(indirect-only mode)에서 erase/program/verify/read 수행.
  *
  *  Appli는 read만 (XIP memory-mapped로 자동 접근). Slot1 staging은 Boot이 전담.
  *
  *  API 선언은 fw_update.h 의 §11에 모여 있음 (xspi_flash_*).
  *
  *  Design Ref: plans/frp-scan-fw-velvety-toucan.md Step 1
  ******************************************************************************
  */
#ifndef __FW_FLASH_XSPI_H__
#define __FW_FLASH_XSPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "fw_update.h"

/*============================================================================
 *  EXTMEM Memory ID
 *  evcd_test extmem_manager.c의 EXTMEMORY_1 (XSPI1 NOR)에 대응.
 *  EXTMEM 미들웨어는 EXTMEMORY_1 == 0 으로 정의되어 있음.
 *===========================================================================*/
#define XSPI_EXTMEM_ID          0U

#ifdef __cplusplus
}
#endif

#endif /* __FW_FLASH_XSPI_H__ */
