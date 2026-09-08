/*----------------------------------------------------------------------
 *   Buzzer Control
 *--------------------------------------------------------------------*/
#ifndef	__LED_H_
#define	__LED_H_

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "stm32h7xx.h"
#include "typedef.h"
#include "led_control.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

typedef enum _eLED_STATE
{
	eNONE				= 0,
	eLED_OFF				= 1,
	eLED_GENERAL			= 2,		// White ON (부팅)
	eLED_NORMAL			= 3,		// Green ON (단말연결) / Yellow ON (레코드모드)
	eLED_DIAG_COMM		= 4,		// Green 100ms 점멸 (통신중: DTC, CDA, ACT, VSM, 파일전송, ECU 리프로)
	eLED_REC_COMM		= 5,		// Yellow 점멸 (레코드 통신)
	eLED_NOTI			= 6,		// Red 300ms 점멸 (BT 페어링 타임아웃 1분)
	eLED_BT_SCAN			= 7,		// Red↔Blue 500ms 교대 (BT 페어링 대기중)
	eLED_HSM_ERROR		= 8,

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
	eLED_RERPROCOMM_RDBI			= 9,
	eLED_RERPROCOMM_NO_TARGET		= 10,
#endif
	// 서버 연결 관련
	eLED_SERVER_CONNECTED	= 20,	// Blue ON (서버 연결됨)
	eLED_SERVER_SCAN		= 21,	// Blue↔Yellow 500ms 교대 (서버 페어링 대기중)
	eLED_SERVER_TIMEOUT		= 22,	// Yellow 300ms 점멸 (서버 연결 타임아웃 1분)

	// 3분 미연결 알림
	eLED_NO_CONN_3MIN		= 23,	// Red 200ms 점멸 (단말 미연결 3분 초과, 30초 주기 부저)
	eLED_NO_SERVER_3MIN		= 24,	// Yellow 200ms 점멸 (서버 미연결 3분 초과, 30초 주기 부저)

	// ECU 업그레이드 관련
	eLED_ECU_ERROR			= 25,	// Red ON (ECU 리프로 에러)
	eLED_ECU_COMPLETE		= 26,	// White(미연결)/Green(연결) (ECU 리프로 완료)

	// 레코드 모드
	eLED_REC_READY			= 27,	// Yellow ON (레코드 Ready)
	eLED_REC_TRIGGER		= 28,	// Yellow 500ms 점멸 (트리거 연결)
	eLED_REC_COMM_FAIL		= 29,	// Red 500ms 점멸 (통신 5회 실패)
	// RSSI Signal Strength
	eLED_RSSI_STRONG		= 30,		// Blue 2 blink (strong)
	eLED_RSSI_MEDIUM		= 31,		// Yellow 2 blink (medium)
	eLED_RSSI_WEAK			= 32,		// Red 2 blink (weak)
} eLED_STATE;

typedef struct _stLED_INFO
{
	eLED_STATE 		eState;
	uint32_t 		unTimeout;
	uint32_t		unOldTime;
	uint8_t 		ucOnOffTime;
	uint8_t 		ucTimerIndex;
} stLED_INFO;

/*----------------------------------------------------------------------
 *    Functions
 *--------------------------------------------------------------------*/
extern void LED_SetState( eLED_STATE eState, uint32_t unTimeout , uint32_t ucOnOffTime);
extern eLED_STATE LED_GetState();
extern BOOL LED_TimerInit( void );
#ifdef NEW_VCI_III
extern BOOL LED_TimerDeinit( void );
#endif

#endif