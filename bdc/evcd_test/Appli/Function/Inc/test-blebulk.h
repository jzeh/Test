/**
 * ******************************************************************************
 * @file    test-blebulk.h
 * @brief   [TEST ONLY] BLE 대용량(512B) 전송 테스트 — FuncID 0xC1
 *
 *  ※ 이 파일과 test-blebulk.c 는 전부 테스트 코드다. 양산 코드가 아니다.
 *
 *  ─── 비활성화 (가장 간단) ─────────────────────────────────────────────
 *    아래 BLE_BULK_TEST_ENABLED 를 0 으로 바꾼다.
 *    → 0xC1 핸들러 / CLI 커맨드 / eMMC 저장 코드 전부 컴파일되지 않음.
 *      호출부(git-functionlist.c, task-cli.c)도 같은 매크로로 gate 되어 있어
 *      링크 잔여물이 남지 않는다.
 *
 *  ─── 완전 제거 ────────────────────────────────────────────────────────
 *    1) Appli/Function/Src/test-blebulk.c  삭제
 *    2) Appli/Function/Inc/test-blebulk.h  삭제 (이 파일)
 *    3) EWARM/Appli/EVCD_Test_Appli.ewp 에서 test-blebulk.c <file> 항목 삭제
 *    4) git-functionlist.c : #include "../Inc/test-blebulk.h" 와
 *                            #if BLE_BULK_TEST_ENABLED 블록(테이블 엔트리) 삭제
 *    5) task-cli.c         : #include "../Inc/test-blebulk.h" 와
 *                            #if BLE_BULK_TEST_ENABLED 블록 2곳 삭제
 *                            (ble bulk 디스패치 + CLI_PrintUsage 한 줄)
 *
 *    ★ git-comm.c 의 rx_buf 크기 수정(512 -> GITPACKET_FRAME_SIZE_MAX)은
 *      테스트 코드가 아니라 실제 버그 수정이므로 그대로 유지할 것.
 *      (payload 512B 프레임 = 519B 이므로 512 버퍼는 7바이트 오버런)
 * ******************************************************************************
 */

#ifndef __TEST_BLEBULK_H
#define __TEST_BLEBULK_H

/* ★ 테스트 코드 on/off 스위치 — 0 = 전체 비활성 */
#define BLE_BULK_TEST_ENABLED   1

#if BLE_BULK_TEST_ENABLED

#include "sys-common.h"

/* 테스트용 Function ID — 미사용 그룹(0xCx) 사용 */
#define TEST_BULK_FUNC_ID       0xC1

/**
 * @brief  0xC1 수신 핸들러 — g_Functions_GDS[] 에 등록됨 (git-functionlist.c)
 * @note   payload 0~512B 모두 정상 처리. 길이/내용/저장 성공 여부와 무관하게 항상 ACK.
 */
void FL_GDS_Test_BulkRecv(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);

/**
 * @brief  CLI "ble bulk ..." 서브커맨드 전체 처리 (task-cli.c 에서 호출)
 * @param  argc/argv  argv[0]="ble", argv[1]="bulk" 상태로 그대로 전달
 * @retval 1 = 처리 완료, 0 = 미인식 서브커맨드 (호출자가 usage 출력)
 */
int TestBulk_CliCmd(int argc, char **argv);

#endif /* BLE_BULK_TEST_ENABLED */

#endif /* __TEST_BLEBULK_H */
