// fcntl.h

#ifndef _FCNTL_H
#define _FCNTL_H

// 파일 제어 관련 상수 정의
#define O_RDONLY    0x0000  // 읽기 전용
#define O_WRONLY    0x0001  // 쓰기 전용
#define O_RDWR      0x0002  // 읽기 및 쓰기
#define O_CREAT     0x0100  // 파일이 없으면 생성
#define O_TRUNC     0x0200  // 파일 크기를 0으로 설정
#define O_APPEND    0x0400  // 파일 끝에 추가

// 필요한 경우 추가 상수 정의

#endif // _FCNTL_H

