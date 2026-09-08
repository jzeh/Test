/**
  ******************************************************************************
  * @file    fw_crc32.h
  * @brief   S/W CRC32 (IEEE 802.3) 공통 API
  *
  *  fw_slot_manager.c / fw_update.c 양쪽에서 사용. HW CRC 주변장치와 무관
  *  (순수 SW). Boot + App 양쪽에서 충돌 없이 사용 가능.
  *
  *  알고리즘: CRC-32/ISO-HDLC (IEEE 802.3)
  *    Poly: 0xEDB88320 (reversed), Init: 0xFFFFFFFF, XorOut: 0xFFFFFFFF
  *
  *  @note 구현은 fw_crc32.c (Step 2에서 이식).
  ******************************************************************************
  */
#ifndef __FW_CRC32_H__
#define __FW_CRC32_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 1회성 API */
uint32_t fw_crc32(const uint8_t *data, uint32_t len);

/* 스트리밍 API (대용량/청크 단위 처리) */
uint32_t fw_crc32_start(void);
uint32_t fw_crc32_update(uint32_t crc, const uint8_t *data, uint32_t len);
uint32_t fw_crc32_finish(uint32_t crc);

#ifdef __cplusplus
}
#endif

#endif /* __FW_CRC32_H__ */
