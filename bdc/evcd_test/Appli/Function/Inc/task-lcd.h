/**
 * ******************************************************************************
 * @file    task-lcd.h
 * @brief   LCD Task (UART8) - Event-based API
 *
 *  외부 태스크에서 LCD_Post*() 함수로 이벤트를 큐에 삽입하면
 *  LCD Task가 순차적으로 처리하여 UART8으로 전송한다.
 *
 *  사용 예:
 *    LCD_PostBtnState(LCD_SCR_MAIN, LCD_BTN_CHG_START, LCD_ON);
 *    LCD_PostTextSet(LCD_SCR_CHARGE, LCD_BTN_VOLT_VAL, "350.0");
 *    LCD_PostScreenGoto(LCD_SCR_CHARGE);
 *    LCD_PostEmergencyStop();
 * ******************************************************************************
 */
#ifndef TASK_LCD_H
#define TASK_LCD_H

#include "sys-common.h"

/*----------------------------------------------------------------------
 *  스크린 번호 (화면제어 / 버튼 명령의 screen 필드)
 *--------------------------------------------------------------------*/
#define LCD_SCR_MAIN       0x00
#define LCD_SCR_CHARGE     0x01
#define LCD_SCR_DISCHARGE  0x02
#define LCD_SCR_SETTING    0x03

/*----------------------------------------------------------------------
 *  버튼 번호 (각 스크린 기준)
 *
 *  메인 스크린 (LCD_SCR_MAIN)
 *--------------------------------------------------------------------*/
#define LCD_BTN_HOME            0x02
#define LCD_BTN_CHG_START       0x03   /* 충전시작 버튼 */
#define LCD_BTN_DIS_START       0x04   /* 방전시작 버튼 */
#define LCD_BTN_MAIN_CHG_TAB    0x05   /* 충전모드 탭 */
#define LCD_BTN_MAIN_DIS_TAB    0x06   /* 방전모드 탭 */
#define LCD_BTN_MAIN_SET_TAB    0x07   /* 설정모드 탭 */

#define LCD_MAIN_BT_CONN_CTRL_ID       0x02
#define LCD_MAIN_BT_CONN_WAITING_TEXT  0x03

/* 충전/방전 스크린 공통 탭 */
#define LCD_BTN_PAGE_CHG_TAB    0x06
#define LCD_BTN_PAGE_DIS_TAB    0x07
#define LCD_BTN_PAGE_SET_TAB    0x08

/*----------------------------------------------------------------------
 *  ON / OFF 값
 *--------------------------------------------------------------------*/
#define LCD_ON   0x01
#define LCD_OFF  0x00

/*----------------------------------------------------------------------
 *  LCD→MCU 버튼 이벤트 컨트롤 ID (RX, LE 2B의 LSB)
 *--------------------------------------------------------------------*/
/* 충전 모드 (LCD_SCR_CHARGE = 1) */
#define LCD_RX_CHARGE_STOP         0x06   /* 충전 종료 버튼 */
#define LCD_RX_CHARGE_DISC_OK      0x1A   /* 연결해제(BLE Disconnect) 팝업 확인 */
#define LCD_RX_CHARGE_NOTIFY_NO    0x1F   /* 알림팝업 아니요 */
#define LCD_RX_CHARGE_NOTIFY_YES   0x21   /* 알림팝업 예 */
#define LCD_RX_CHARGE_DONE_OK      0x24   /* 작업완료 확인 */
#define LCD_RX_CHARGE_FAIL_OK      0x29   /* 구동실패 확인 */
#define LCD_RX_CHARGE_STOP_OK      0x2C   /* APP Error Stop 팝업 확인 */
#define LCD_RX_CHARGE_WARN_TEMP_OK 0x31   /* 온도 경고 팝업 확인 */
#define LCD_RX_CHARGE_CAUT_TEMP_STOP 0x37 /* 온도 주의 팝업 종료 */
/* 방전 모드 (LCD_SCR_DISCHARGE = 2) */
#define LCD_RX_DISC_STOP           0x07   /* 방전 종료 버튼 */
#define LCD_RX_DISC_WARN_OK        0x16   /* 경고팝업 확인 */
#define LCD_RX_DISC_DISC_OK        0x1A   /* 연결해제 확인 */
#define LCD_RX_DISC_NOTIFY_NO      0x1F   /* 알림팝업 아니요 */
#define LCD_RX_DISC_NOTIFY_YES     0x21   /* 알림팝업 예 */
#define LCD_RX_DISC_DONE_OK        0x24   /* 작업완료 확인 */
#define LCD_RX_DISC_FAIL_OK        0x29   /* 구동실패 확인 */
#define LCD_RX_DISC_STOP_OK        0x2C   /* APP Error Stop 팝업 확인 */
#define LCD_RX_DISC_WARN_TEMP_OK   0x31   /* 온도 경고 팝업 확인 */
#define LCD_RX_DISC_CAUT_TEMP_STOP 0x37   /* 온도 주의 팝업 종료 */
/* 메인 스크린 (LCD_SCR_MAIN = 0) */
#define LCD_RX_MAIN_NOTIFY2_OK     0x2E   /* 알림팝업2 확인 */
#define LCD_RX_MAIN_NOTIFY3_OK     0x33   /* 알림팝업3 확인 */


/*----------------------------------------------------------------------
 *  이벤트 타입
 *--------------------------------------------------------------------*/
typedef enum {
    LCD_EVT_BTN_STATE,      /* 버튼 ON/OFF   : screen, button, value=on/off */
    LCD_EVT_SCREEN_GOTO,    /* 화면 전환     : screen */
    LCD_EVT_EMERGENCY_STOP, /* 긴급정지      : (파라미터 없음) */
    LCD_EVT_TEXT_SET,       /* 텍스트 변경   : screen, ctrl_id, text[] */
    LCD_EVT_TEXT_READ,      /* 텍스트 읽기   : screen, ctrl_id */
    LCD_EVT_SHOW_HIDE,      /* 표시/숨김     : screen, ctrl_id, value=1(show)/0(hide) */
    LCD_EVT_RTC_SET,        /* RTC 시각 설정 : text[0..1]=year(big-endian), text[2]=mon,
                             *                text[3]=day, text[4]=hour, text[5]=min,
                             *                text[6]=sec, text_len=7 */
    LCD_EVT_TOUCH_ENABLE,   /* 터치 활성/비활성 : screen, ctrl_id, value=1(on)/0(off) */
} eLCD_EventType;

#define LCD_TEXT_MAX  100
#define LCD_TEXT_BUF  (LCD_TEXT_MAX + 1)  /* null 포함 버퍼 크기 */

typedef struct {
    eLCD_EventType type;
    uint8_t  screen;
    uint8_t  ctrl_id;           /* 버튼/컨트롤 번호 */
    uint8_t  value;              /* on/off 또는 show/hide */
    uint8_t  text[LCD_TEXT_MAX]; /* 텍스트 (ASCII, null 미포함) */
    uint8_t  text_len;
} stLCD_Event;

/*----------------------------------------------------------------------
 *  LCD 상태 구조체
 *  LCD로부터 수신된 이벤트 및 MCU가 표출한 텍스트 정보를 관리
 *--------------------------------------------------------------------*/
typedef struct {
    uint8_t  prev_screen;                   /* 이전 스크린 번호 */
    uint8_t  curr_screen;                   /* 현재 스크린 번호 */

    /* 메인 스크린 (LCD_SCR_MAIN = 0) */
    char     scr0_text1[LCD_TEXT_BUF];      /* 텍스트 컨트롤 1 */
    char     scr0_text2[LCD_TEXT_BUF];      /* 텍스트 컨트롤 2 */

    /* 충전 스크린 (LCD_SCR_CHARGE = 1) */
    char     scr1_text3[LCD_TEXT_BUF];      /* 텍스트 컨트롤 3 */

    /* 방전 스크린 (LCD_SCR_DISCHARGE = 2) */
    char     scr2_text3[LCD_TEXT_BUF];      /* 텍스트 컨트롤 3 */

    /* 설정 스크린 (LCD_SCR_SETTING = 3) */
    char     scr3_text2[LCD_TEXT_BUF];      /* 텍스트 컨트롤 2  */
    char     scr3_text14[LCD_TEXT_BUF];     /* 텍스트 컨트롤 14 */
    char     scr3_text17[LCD_TEXT_BUF];     /* 텍스트 컨트롤 17 */
} stLCD_State;

extern stLCD_State g_lcdState;

/*----------------------------------------------------------------------
 *  외부 API
 *--------------------------------------------------------------------*/

/** 태스크 초기화 (main.c에서 호출) */
void InitLcdTask(void);

/** UART8 RX 콜백 (git-comm.c HAL_UART_RxCpltCallback에서 호출) */
void LCD_UART8_RxCpltCallback(void);

/** UART8 에러 콜백 (git-comm.c HAL_UART_ErrorCallback에서 호출, RX IT 재시작) */
void LCD_UART8_RxErrorCallback(void);

/**
 * @brief  버튼 ON/OFF 상태 전송
 * @param  screen  : LCD_SCR_*
 * @param  button  : LCD_BTN_*
 * @param  on_off  : LCD_ON / LCD_OFF
 */
void LCD_PostBtnState(uint8_t screen, uint8_t button, uint8_t on_off);

/**
 * @brief  화면 전환 명령
 * @param  screen  : LCD_SCR_*
 */
void LCD_PostScreenGoto(uint8_t screen);

/** 긴급정지 명령 */
void LCD_PostEmergencyStop(void);

/** 충전 진행 타이머 시작 (Lua TIMER_FLAG=1) */
void LCD_PostChargeTimerStart(void);

/** 충전 진행 타이머 정지 (Lua TIMER_FLAG=0) */
void LCD_PostChargeTimerStop(void);

/** 방전 진행 타이머 시작 (Lua TIMER_FLAG2=1) */
void LCD_PostDischargeTimerStart(void);

/** 방전 진행 타이머 정지 (Lua TIMER_FLAG2=0) */
void LCD_PostDischargeTimerStop(void);

/**
 * @brief  텍스트 변경 (ASCII 문자열)
 * @param  screen  : LCD_SCR_*
 * @param  ctrl_id : 텍스트 위젯 번호
 * @param  text    : null-terminated ASCII 문자열 (최대 LCD_TEXT_MAX)
 */
void LCD_PostTextSet(uint8_t screen, uint8_t ctrl_id, const char *text);

/* 차량 정보 기본 문구("차량 연결 필요") 4개 화면 표출 — 부팅 init / BLE disconnect 시 */
void LCD_PostVehicleInfoDefault(void);

/**
 * @brief  텍스트 읽기 요청
 * @param  screen  : LCD_SCR_*
 * @param  ctrl_id : 텍스트 위젯 번호
 */
void LCD_PostTextRead(uint8_t screen, uint8_t ctrl_id);

/**
 * @brief  컨트롤 표시/숨김
 * @param  screen  : LCD_SCR_*
 * @param  ctrl_id : 컨트롤 번호
 * @param  show    : 1=표시, 0=숨김
 */
void LCD_PostShowHide(uint8_t screen, uint8_t ctrl_id, uint8_t show);

/**
 * @brief  LCD RTC 시각 설정 (0x81 명령)
 * @param  year  : 연도 (예: 2026)  - 요일은 내부 자동 계산
 * @param  mon   : 월  (1~12)
 * @param  day   : 일  (1~31)
 * @param  hour  : 시  (0~23)
 * @param  min   : 분  (0~59)
 * @param  sec   : 초  (0~59)
 */
void LCD_PostRtcSet(uint16_t year, uint8_t mon, uint8_t day,
                    uint8_t hour, uint8_t min, uint8_t sec);

/**
 * @brief  컨트롤 터치 활성/비활성 (B1 04 명령)
 * @param  screen  : LCD_SCR_*
 * @param  ctrl_id : 컨트롤 번호
 * @param  enable  : 1=활성, 0=비활성
 */
void LCD_PostTouchEnable(uint8_t screen, uint8_t ctrl_id, uint8_t enable);

/**
 * @brief  팝업 트리거 함수 (B1 10 명령, LCD_CTRL_POPUP_* 컨트롤)
 * @param  screen  : LCD_SCR_CHARGE 또는 LCD_SCR_DISCHARGE
 */
void LCD_PostPopupWarning(uint8_t screen);    /* 경고 팝업    (control 98) */
void LCD_PostPopupDisconnect(uint8_t screen); /* 연결해제 팝업 (control 97) */
void LCD_PostPopupNotify(uint8_t screen);     /* 알림 팝업    (control 96) */
void LCD_PostPopupWorkDone(uint8_t screen);   /* 작업완료 팝업 (control 95) */
void LCD_PostPopupDriveFail(uint8_t screen);  /* 구동실패 팝업 (control 94) */
void LCD_PostPopupStop(uint8_t screen);       /* APP Error Stop 팝업 */
void LCD_PostPopupCautTemp(uint8_t screen);   /* 온도 주의 팝업 */
void LCD_PostPopupWarnTemp(uint8_t screen);   /* 온도 경고 팝업 */

/** 통신중 표시/숨김 (메인 Screen0, control 91) */
void LCD_PostPopupComm(uint8_t show);

/** 알림 팝업2 (메인 Screen0, control 93) */
void LCD_PostPopupNotify2(void);

/** 알림 팝업3 (메인 Screen0, control 92) */
void LCD_PostPopupNotify3(void);

/*----------------------------------------------------------------------
 *  LCD→MCU 버튼 이벤트 핸들러 (weak — 필요한 태스크에서 재정의)
 *  LCD가 버튼 이벤트를 보내면 task-lcd가 파싱 후 해당 함수 호출
 *  다른 태스크로 이벤트를 전달하려면 이 함수 안에서 xQueueSend 등 사용
 *--------------------------------------------------------------------*/
/* 충전 모드 */
void LCD_OnCharge_Stop(void);
void LCD_OnCharge_DiscOk(void);
void LCD_OnCharge_NotifyNo(void);
void LCD_OnCharge_NotifyYes(void);
void LCD_OnCharge_WorkDoneOk(void);
void LCD_OnCharge_DriveFailOk(void);
void LCD_OnCharge_StopOk(void);     /* APP Error Stop 팝업 OK (btn 0x2C) */
void LCD_OnCharge_WarnTempOk(void);
void LCD_OnCharge_CautTempStop(void);
/* 방전 모드 */
void LCD_OnDischarge_Stop(void);
void LCD_OnDischarge_WarnOk(void);
void LCD_OnDischarge_DiscOk(void);
void LCD_OnDischarge_NotifyNo(void);
void LCD_OnDischarge_NotifyYes(void);
void LCD_OnDischarge_WorkDoneOk(void);
void LCD_OnDischarge_DriveFailOk(void);
void LCD_OnDischarge_StopOk(void);  /* APP Error Stop 팝업 OK (btn 0x2C) */
void LCD_OnDischarge_WarnTempOk(void);
void LCD_OnDischarge_CautTempStop(void);
/* 메인 */
void LCD_OnMain_Notify2Ok(void);
void LCD_OnMain_Notify3Ok(void);
void LCD_MainScreenStatusUpdate(int conn);

#endif /* TASK_LCD_H */
