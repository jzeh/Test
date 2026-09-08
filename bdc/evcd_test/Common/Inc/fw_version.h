/**
  ******************************************************************************
  * @file    fw_version.h
  * @brief   펌웨어 버전 정의 (수동 관리)
  *
  *  이 값이 OTA 이미지 헤더(fw_image_header_t.fw_version)에 박힌다.
  *  빌드 후처리 스크립트(tools/fw_sign.py)가 이 파일을 파싱해 헤더를 생성한다.
  *  릴리스 시 아래 세 값을 갱신할 것.
  ******************************************************************************
  */
#ifndef __FW_VERSION_H__
#define __FW_VERSION_H__

#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    1

/* 이미지가 속한 application 번호 (firmware_lite.h 의 AppName enum, 통상 Main=1) */
#define FW_APP_NO           1

#endif /* __FW_VERSION_H__ */
