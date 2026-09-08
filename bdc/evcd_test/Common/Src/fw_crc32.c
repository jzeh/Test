/**
  ******************************************************************************
  * @file    fw_crc32.c
  * @brief   S/W CRC32 (IEEE 802.3) 공통 구현
  *
  *  포팅 출처: Z:\11_Work\00_Git_Source\05_Others\frp-scan\Common\Src\fw_crc32.c
  *  변경 없음 — 외부 의존성 zero.
  *
  *  알고리즘: CRC-32/ISO-HDLC
  *    Poly: 0xEDB88320 (bit-reversed 0x04C11DB7)
  *    Init: 0xFFFFFFFF, XorOut: 0xFFFFFFFF, RefIn: true, RefOut: true
  ******************************************************************************
  */

#include "fw_crc32.h"

/*============================================================================
 *  스트리밍 API
 *===========================================================================*/

uint32_t fw_crc32_start(void)
{
    return 0xFFFFFFFFU;
}

uint32_t fw_crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= (uint32_t)data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 1U)
                crc = (crc >> 1) ^ 0xEDB88320U;
            else
                crc >>= 1;
        }
    }
    return crc;
}

uint32_t fw_crc32_finish(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFU;
}

/*============================================================================
 *  1회성 API
 *===========================================================================*/

uint32_t fw_crc32(const uint8_t *data, uint32_t len)
{
    return fw_crc32_finish(fw_crc32_update(fw_crc32_start(), data, len));
}
