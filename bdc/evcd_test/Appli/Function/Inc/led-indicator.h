#ifndef __LED_INDICATOR_H
#define __LED_INDICATOR_H
/**
  ******************************************************************************
  * @file    led-indicator.h
  * @brief   SS5(R)/SS6(Y)/SS7(G)/SS8(부저) 상태 표시 — 단일 소유 모듈
  *
  *    우선순위   조건                                        결과
  *    --------   -----------------------------------------   ------------------
  *    1          치명적 에러(s_criticalError)                 LED_R_BLINK (+ 부저 3초)
  *    2          경고(s_warningLatched)                       LED_Y_BLINK
  *    3          !COMM_IsBTConnected() (BLE 미연결, 부팅 포함)  LED_Y_ON
  *    4          g_device_state == eDEVICE_STATE_START (운전중) LED_G_ON
  *    5          (그 외: 연결됨 + 비운전 = 대기)                LED_G_BLINK
  *
  *  ※ eDEVICE_STATE_RUNNING 은 코드 전체에서 대입되는 곳이 없는 dead enum이므로
  *    "운전중" 판정에 쓰지 않는다 — eDEVICE_STATE_START 로 판정한다.
  *  ※ BTGetConnectStatus()(git-ble.c)는 연결 끊김 edge에서 g_system_state/
  *    g_system_mode 를 강제로 NONE 리셋하는 부작용이 있어 여기서 쓰면 안 된다 —
  *    반드시 부작용 없는 COMM_IsBTConnected() 를 사용한다.
  ******************************************************************************
  */

#include "../Inc/sys-common.h"

/* Functions -----------------------------------------------------------*/

/* 부팅 1회 — 전체 알람 clear. 이 시점엔 BLE 미연결이므로 우선순위표에 의해
 * 자동으로 LED_Y_ON 이 뜬다(별도의 "부팅" 케이스 처리 불필요). */
extern void LED_Init(void);

/* main loop 주기 호출(10ms) — 점멸 위상(500ms) 갱신 + 부저 타이머(3초) 만료 처리 +
 * 매번 무조건 재판정(연결/운전 상태 실시간 반영 + 점멸 애니메이션 유지) */
extern void LED_Tick(void);

/* 과충전/과방전/과온(err1~3) 또는 센서 온도 WARN(>150°C) → 치명적 에러 발생
 * (R 점멸 + 부저 3초 ON 후 자동 OFF). LED_ClearAll()/LED_ClearCriticalError() 까지 유지 */
extern void LED_RaiseCriticalError(void);

/* 센서 온도 NORMAL 복귀 → 치명적 에러만 해제(부저도 즉시 OFF). 다른 원인의
 * 경고(s_warningLatched)는 그대로 유지된다 */
extern void LED_ClearCriticalError(void);

/* err5(BSA_POWER_CONNECTOR_FAIL)/err6(INSUL_RESISTANCE)/0x82 APP 에러/
 * 0x14 NAK(커넥터 분리) 등 RUNNING 중 에러 발생 → 경고 발생 (Y 점멸).
 * LED_ClearAll() 까지 유지 */
extern void LED_RaiseWarning(void);

/* 0x14 ACK(커넥터 정상)/device START edge/LCD MAIN 화면 진입/DriveFailOk →
 * 전체 알람 clear (이후 표시는 우선순위표 3~5번이 실시간으로 결정) */
extern void LED_ClearAll(void);

/* 치명적 에러 또는 경고 중 하나라도 활성 여부 — LCD MAIN 화면 진입 edge에서
 * clear 여부 판단용 */
extern bool LED_HasActiveAlarm(void);

#endif /* __LED_INDICATOR_H */
