/**
  ******************************************************************************
  * @file    fw_image_header.h
  * @brief   OTA 펌웨어 이미지 내장 헤더 정의 (Boot + Appli 공유)
  *
  *  빌드 후처리(tools/fw_sign.py)가 순수 .bin 앞에 이 64바이트 헤더를 prepend 하여
  *  전송용 이미지(app_ota.bin)를 만든다.
  *
  *    전송 이미지 = [ fw_image_header_t (64B) ][ 실행 이미지(payload) ]
  *
  *  장치(fw_update_recv)는 수신 스트림의 선행 64바이트를 헤더로 분리·검증한 뒤,
  *  ★ 헤더를 떼어내고 payload 만 DownloadTemp.bin 에 저장한다.
  *  → 저장/실행 이미지는 벡터 테이블이 offset 0 에 그대로 있어 XIP 실행에 영향 없음.
  *    (부트로더 staging 경로 무수정)
  *
  *  검증 기준(size / CRC32 / version)은 "빌드 시점"에 헤더로 박히므로,
  *  장치가 수신 데이터로 자체 계산한 값과 비교해도 동어반복이 아닌 실제 무결성 검증이 된다.
  *
  *  CRC 알고리즘은 fw_crc32.c 와 동일: CRC-32/ISO-HDLC (poly 0xEDB88320,
  *  init 0xFFFFFFFF, xorout 0xFFFFFFFF) = Python zlib.crc32 와 호환.
  ******************************************************************************
  */
#ifndef __FW_IMAGE_HEADER_H__
#define __FW_IMAGE_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 헤더 식별자 "EVCD" (0x45 'E' 0x56 'V' 0x43 'C' 0x44 'D', LE 저장 시 0x45564344) */
#define FW_IMG_HDR_MAGIC        0x45564344U
#define FW_IMG_HDR_FMT_VERSION  0x0001U      /* 헤더 포맷 버전 */
#define FW_IMG_HDR_SIZE         64U          /* 헤더 총 크기(바이트) */
#define FW_IMG_GIT_DESC_LEN     32U          /* git describe 문자열 최대 길이 */

/**
 * OTA 이미지 헤더 (정확히 64바이트, little-endian).
 *  - image_size / image_crc32 : payload(헤더 제외 실행 이미지) 기준
 *  - hdr_crc32                : 이 필드(마지막 4B)를 제외한 선행 60바이트의 CRC32
 */
typedef __packed struct {
    uint32_t magic;                          /* [0]  0x45564344 "EVCD" */
    uint16_t hdr_version;                     /* [4]  헤더 포맷 버전 (=1) */
    uint16_t hdr_size;                        /* [6]  sizeof(header) = 64 */
    uint32_t image_size;                      /* [8]  payload 바이트 수 (헤더 제외) */
    uint32_t image_crc32;                     /* [12] payload CRC32 */
    uint8_t  fw_version[3];                   /* [16] major.minor.patch */
    uint8_t  app_no;                          /* [19] application number (AppName enum) */
    uint32_t build_epoch;                     /* [20] 빌드 시각(Unix epoch, 미사용 시 0) */
    char     git_desc[FW_IMG_GIT_DESC_LEN];   /* [24] git describe 문자열(널 종단, 선택) */
    uint8_t  reserved[4];                     /* [56] 예약(0) */
    uint32_t hdr_crc32;                       /* [60] 헤더 CRC32 (선행 60B 대상) */
} fw_image_header_t;

/* 헤더 CRC 계산 대상 길이 = 헤더 크기 - hdr_crc32(4B) */
#define FW_IMG_HDR_CRC_LEN      (FW_IMG_HDR_SIZE - 4U)   /* 60 */

#ifndef __cplusplus
_Static_assert(sizeof(fw_image_header_t) == FW_IMG_HDR_SIZE,
               "fw_image_header_t must be exactly 64 bytes");
#endif

#ifdef __cplusplus
}
#endif

#endif /* __FW_IMAGE_HEADER_H__ */
