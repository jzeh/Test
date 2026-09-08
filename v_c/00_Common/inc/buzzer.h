/*----------------------------------------------------------------------
 *   Buzzer Control
 *--------------------------------------------------------------------*/
#ifndef	__BUZZER_H_
#define	__BUZZER_H_

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "stm32h7xx.h"
#include "typedef.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
typedef enum _eBuzzertype
{
	eBUZZER_OFF			= 0,
	eBUZZER_ON			= 1,
	eBUZZER_DOMISOLDO	= 2,
	eBUZZER_DOMISOL		= 3,
	eBUZZER_DOPARA		= 4,
	eBUZZER_SIRESOL		= 5,

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
	eBUZZER_SOLPAMI		= 6,
#endif
} eBuzzertype;

typedef enum _eBUZZER_STATE
{
	eBUZZER_NONE,       //  초기값
	ePOWER_ON,          // 부팅시

    eSERVER_LOSS,       // 서버 통신 없음
    eBT_PAIRING,        // 페어링 시작

	eDATA_SAVED,        // 데이터 저장 완료시
  	eDATA_FULL,         // EMMC 데이터 용량 초과시
}eBUZZER_STATE;

typedef struct _stBUZZER_INFO
{
	eBuzzertype	eBuzzerIndex;						// 1: on,  2부터는 sounds
	u32			unBuzzerLoopCnt;
	u32			unBuzzerOnTime;						// ms
	u32			unBuzzerOffTime;					// ms
	u8			ucTimerIndex;
} stBUZZER_INFO;

/*----------------------------------------------------------------------
 *    Functions
 *--------------------------------------------------------------------*/
extern int32_t	Buzzer_Init( void );
extern void		Buzzer_Control( eBuzzertype ucBuzzer,  uint32_t unOnTime_ms, uint32_t unOffTime_ms, uint32_t unloopCnt );

#endif