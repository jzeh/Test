/**
  ******************************************************************************
  * @file    fw_slot_manager.h
  * @brief   Slot Manager — Dual slot metadata management (Primary+Backup 이중화)
  *
  *  공개 API 선언은 fw_update.h §11에 모여 있음 (slot_*).
  *  이 헤더는 내부 상수와 include만 제공.
  *
  *  포팅 출처: Z:\11_Work\00_Git_Source\05_Others\frp-scan\Common\Inc\fw_slot_manager.h
  ******************************************************************************
  */
#ifndef __FW_SLOT_MANAGER_H__
#define __FW_SLOT_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fw_update.h"
#include "fw_flash_xspi.h"

/*============================================================================
 *  Metadata CRC calculation scope
 *  CRC covers: magic(4) + exec(45) + staging(45) + copy_pending(1) = 95 bytes
 *  Excludes:   metadata_crc32(4) + reserved (이후 padding)
 *===========================================================================*/
#define SLOT_META_CRC_SIZE  (sizeof(uint32_t) + sizeof(slot_info_t) * 2 + sizeof(uint8_t))

#ifdef __cplusplus
}
#endif

#endif /* __FW_SLOT_MANAGER_H__ */
