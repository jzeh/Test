/**
 * ******************************************************************************
 * @file    task-lcd.c
 * @brief   LCD Task (UART8) - Event-based
 *
 *  ┌─────────────────────────────────────────────────────────┐
 *  │              LCD 프로토콜 (TX → LCD)                     │
 *  ├───────────────┬──────────────────────────────────────────┤
 *  │ 화면제어      │ EE B1 00 00 [scr] FF FC FF FF            │
 *  │ 버튼상태 설정 │ EE B1 10 00 [scr] 00 [btn] [on/off]     │
 *  │               │              FF FC FF FF                 │
 *  │ 긴급정지      │ EE B1 AB CD EF FE DC BA FF FC FF FF      │
 *  │ 텍스트 제어   │ EE B1 10 00 [scr] 00 [ctrl] [ascii...]  │
 *  │               │              FF FC FF FF                 │
 *  │ 텍스트 읽기   │ EE B1 11 00 [scr] 00 [btn] FF FC FF FF  │
 *  │ show/hide     │ EE B1 03 00 [scr] 00 [btn] [1/0]        │
 *  │               │              FF FC FF FF                 │
 *  ├───────────────┴──────────────────────────────────────────┤
 *  │              LCD 프로토콜 (RX ← LCD)                     │
 *  ├───────────────┬──────────────────────────────────────────┤
 *  │ 현재화면 알림 │ EE B1 01 00 [page] FF FC FF FF           │
 *  │               │ page: 0=메인 1=충전 2=방전 3=설정        │
 *  └───────────────┴──────────────────────────────────────────┘
 * ******************************************************************************
 */

/* Includes  -----------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/task-lcd.h"
#include "../Inc/task-plc.h"          /* PLC_ApplyStopSteps, PLC_SetTxValue, g_plcStepWaitStop 등 */
#include "../Inc/git-functionlist.h"  /* g_error_code, FL_GDS_Send_Error_Code */
#include "../Inc/git-comm.h"
#include "../Inc/task-hwcontrol.h"
#include "../Inc/led-indicator.h"
#include <string.h>

/*----------------------------------------------------------------------
 *  내부 프로토콜 상수
 *--------------------------------------------------------------------*/
#define LCD_LOG(fmt, ...)  printf("\033[33m" fmt "\033[0m", ##__VA_ARGS__)

#define LCD_SOF1      0xEE
#define LCD_SOF2      0xB1
#define LCD_EOF1      0xFF
#define LCD_EOF2      0xFC
#define LCD_EOF3      0xFF
#define LCD_EOF4      0xFF

/* TX CMD */
#define LCD_CMD_SCREEN   0x00   /* 화면 전환 (B1+0x00) */
#define LCD_CMD_TEXT     0x01   /* 텍스트 설정 (B1+0x01) */
#define LCD_CMD_SHOWHIDE 0x03   /* show/hide (B1+0x03) */
#define LCD_CMD_TOUCH    0x04   /* 터치 활성/비활성 (B1+0x04) */
#define LCD_CMD_SET      0x10   /* 버튼 상태 설정 (B1+0x10) */
#define LCD_CMD_READ     0x11   /* 버튼/텍스트 읽기 (B1+0x11) */

/* RX CMD */
#define LCD_CMD_PAGE_CHG 0x01   /* 현재화면 알림 (B1+0x01, LCD→MCU) */

/* 긴급정지 고정 프레임 */
static const uint8_t LCD_FRAME_EMGSTOP[] = {
    0xEE, 0xB1, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0xFF, 0xFC, 0xFF, 0xFF
};
#define LCD_EMGSTOP_LEN  12

/* 최대 TX 프레임 크기 (텍스트: SOF(2)+cmd(1)+rsv(1)+scr(1)+rsv(1)+btn(1)+text(32)+EOF(4)) */
#define LCD_TX_BUF_MAX   (7 + LCD_TEXT_MAX + 4)

/* RX 링버퍼 */
#define LCD_RXBUF_SIZE    256
#define LCD_RX_FRAME_MAX  (7 + LCD_TEXT_MAX + 4)  /* 최대 RX 프레임 크기 */

/* 이벤트 큐 깊이 */
#define LCD_EVT_QUEUE_DEPTH  16

/*----------------------------------------------------------------------
 *  Variables
 *--------------------------------------------------------------------*/
/* RX 링버퍼 (ISR ↔ Task) */
static uint8_t  g_lcd_rx_ring[LCD_RXBUF_SIZE];
static uint16_t g_lcd_rx_head = 0;
static uint16_t g_lcd_rx_tail = 0;
static uint8_t  g_uart8_rx_byte = 0;

/* 현재 LCD 페이지 (RX 응답으로 갱신) */
static uint8_t g_lcd_current_page = LCD_SCR_MAIN;

/* LCD 모듈 ready 핸드셰이크 — LCD 가 부팅 완료 후 보낸 첫 valid RX 프레임 수신 시 true.
 *  · cold boot 시 LCD 모듈 자체 부팅(1~5s) 동안 MCU TX 가 무시되는 문제 해결용.
 *  · NRST(warm reset) 시는 LCD 가 이미 부팅된 상태 → 첫 TX 시도 → LCD 가 ACK 성격의
 *    RX(page notify 등) 를 즉시 회신 → ready 빠르게 set 됨. */
static volatile bool g_lcd_ready = false;
#define LCD_READY_TIMEOUT_MS    8000U   /* fallback: 8s 후 RX 없어도 강제 진행 */
#define LCD_READY_PROBE_MS      500U    /* 대기 중 500ms 마다 ScreenGoto(MAIN) probe TX */

/* LCD 상태 구조체 (외부 공개) */
stLCD_State g_lcdState = {0};

/* 이벤트 큐 */
static QueueHandle_t       g_lcd_evt_queue = NULL;
static StaticQueue_t       g_lcd_evt_queue_buf;
static uint8_t             g_lcd_evt_storage[LCD_EVT_QUEUE_DEPTH * sizeof(stLCD_Event)];

/* Thread */
osThreadId_t lcdTaskHandle;
uint32_t lcdTaskBuffer[2048];
osStaticThreadDef_t lcdTaskControlBlock;
const osThreadAttr_t lcdTask_attributes = {
    .name      = "lcdTask",
    .cb_mem    = &lcdTaskControlBlock,
    .cb_size   = sizeof(lcdTaskControlBlock),
    .stack_mem = &lcdTaskBuffer[0],
    .stack_size = sizeof(lcdTaskBuffer),
    .priority  = (osPriority_t)osPriorityNormal,
};

/*----------------------------------------------------------------------
 *  Private Function Prototypes
 *--------------------------------------------------------------------*/
void StartLcdTask(void *argument);
static void LCD_ProcessRx(void);
static void LCD_ParseRxFrame(const uint8_t *buf, uint8_t len);
static void LCD_UpdateStateFromRx(uint8_t screen, uint8_t ctrl_id,
                                   const uint8_t *data, uint8_t data_len);
static void LCD_DispatchButtonEvent(uint8_t screen, uint8_t ctrl, uint8_t value);
static void LCD_ProcessEvents(void);
static void LCD_Execute(const stLCD_Event *evt);
static void LCD_SendBtnState(uint8_t screen, uint8_t button, uint8_t on_off);
static void LCD_SendScreenGoto(uint8_t screen);
static void LCD_SendEmergencyStop(void);
static void LCD_SendTextSet(uint8_t screen, uint8_t ctrl_id,
                            const uint8_t *text, uint8_t len);
static void LCD_SendTextRead(uint8_t screen, uint8_t ctrl_id);
static void LCD_SendShowHide(uint8_t screen, uint8_t ctrl_id, uint8_t show);
static void LCD_SendTouchEnable(uint8_t screen, uint8_t ctrl_id, uint8_t enable);
static void LCD_SendRtcSet(uint16_t year, uint8_t mon, uint8_t day,
                           uint8_t hour, uint8_t min, uint8_t sec);
static void LCD_Transmit(const uint8_t *frame, uint16_t len);
static void LCD_GoMainAndUpdateBTStatus(void);

/*----------------------------------------------------------------------
 *  ISR 콜백
 *  git-comm.c의 HAL_UART_RxCpltCallback에서 UART8 분기 후 호출
 *--------------------------------------------------------------------*/
void LCD_UART8_RxCpltCallback(void)
{
    uint16_t next = (uint16_t)((g_lcd_rx_head + 1) % LCD_RXBUF_SIZE);
    if (next != g_lcd_rx_tail) {
        g_lcd_rx_ring[g_lcd_rx_head] = g_uart8_rx_byte;
        g_lcd_rx_head = next;
    }
    HAL_UART_Receive_IT(&huart8, &g_uart8_rx_byte, 1);
}

void LCD_UART8_RxErrorCallback(void)
{
    /* 에러(ORE/FE/NE) 후 HAL이 RX IT를 중단하므로 여기서 재시작 */
    HAL_UART_Receive_IT(&huart8, &g_uart8_rx_byte, 1);
}

/*----------------------------------------------------------------------
 *  Public API - 이벤트 큐 삽입
 *--------------------------------------------------------------------*/

void LCD_PostBtnState(uint8_t screen, uint8_t button, uint8_t on_off)
{
    if (!g_lcd_evt_queue) { LCD_LOG("[LCD] ERR: queue NULL\r\n"); return; }
    stLCD_Event evt = { .type = LCD_EVT_BTN_STATE,
                        .screen = screen, .ctrl_id = button, .value = on_off };
    if (xQueueSend(g_lcd_evt_queue, &evt, 0) != pdTRUE)
        LCD_LOG("[LCD] ERR: queue full (BtnState)\r\n");
}

void LCD_PostScreenGoto(uint8_t screen)
{
    if (!g_lcd_evt_queue) { LCD_LOG("[LCD] ERR: queue NULL\r\n"); return; }
    stLCD_Event evt = { .type = LCD_EVT_SCREEN_GOTO, .screen = screen };
    if (xQueueSend(g_lcd_evt_queue, &evt, 0) != pdTRUE)
        LCD_LOG("[LCD] ERR: queue full (ScreenGoto)\r\n");
}

void LCD_PostEmergencyStop(void)
{
    if (!g_lcd_evt_queue) return;
    stLCD_Event evt = { .type = LCD_EVT_EMERGENCY_STOP };
    xQueueSend(g_lcd_evt_queue, &evt, 0);
}

void LCD_PostTextSet(uint8_t screen, uint8_t ctrl_id, const char *text)
{
    if (!g_lcd_evt_queue || !text) return;
    stLCD_Event evt = { .type = LCD_EVT_TEXT_SET,
                        .screen = screen, .ctrl_id = ctrl_id };
    evt.text_len = (uint8_t)strnlen(text, LCD_TEXT_MAX);
    memcpy(evt.text, text, evt.text_len);
    xQueueSend(g_lcd_evt_queue, &evt, 0);
}

void LCD_PostTextRead(uint8_t screen, uint8_t ctrl_id)
{
    if (!g_lcd_evt_queue) return;
    stLCD_Event evt = { .type = LCD_EVT_TEXT_READ,
                        .screen = screen, .ctrl_id = ctrl_id };
    xQueueSend(g_lcd_evt_queue, &evt, 0);
}

void LCD_PostShowHide(uint8_t screen, uint8_t ctrl_id, uint8_t show)
{
    if (!g_lcd_evt_queue) { LCD_LOG("[LCD] ERR: ShowHide queue NULL\r\n"); return; }
    stLCD_Event evt = { .type = LCD_EVT_SHOW_HIDE,
                        .screen = screen, .ctrl_id = ctrl_id, .value = show };
    if (xQueueSend(g_lcd_evt_queue, &evt, 0) != pdTRUE)
        LCD_LOG("[LCD] ERR: queue full (ShowHide scr=%d ctrl=%d)\r\n", screen, ctrl_id);
}

void LCD_PostChargeTimerStart(void)
{
    /* screen=0(메인) control=3 value=1 → Lua: TIMER_FLAG=1, TIMER=0 */
    LCD_PostBtnState(LCD_SCR_MAIN, 3, LCD_ON);
}

void LCD_PostChargeTimerStop(void)
{
    /* screen=1(충전) control=2 value=1 → Lua: TIMER_FLAG=0 */
    LCD_PostBtnState(LCD_SCR_CHARGE, 2, LCD_ON);
}

void LCD_PostDischargeTimerStart(void)
{
    /* screen=0(메인) control=4 value=1 → Lua: TIMER_FLAG2=1 */
    LCD_PostBtnState(LCD_SCR_MAIN, 4, LCD_ON);
}

void LCD_PostDischargeTimerStop(void)
{
    /* screen=2(방전) control=2 value=1 → Lua: TIMER_FLAG2=0 */
    LCD_PostBtnState(LCD_SCR_DISCHARGE, 2, LCD_ON);
}

void LCD_PostTouchEnable(uint8_t screen, uint8_t ctrl_id, uint8_t enable)
{
    if (!g_lcd_evt_queue) return;
    stLCD_Event evt = { .type = LCD_EVT_TOUCH_ENABLE,
                        .screen = screen, .ctrl_id = ctrl_id, .value = enable };
    xQueueSend(g_lcd_evt_queue, &evt, 0);
}

/*----------------------------------------------------------------------
 *  팝업 표시 헬퍼
 *  B1 10은 Lua 트리거 안 됨. B1 03(show) + B1 04(touch disable) 직접 전송.
 *--------------------------------------------------------------------*/
/* 팝업별 show ctrl ID 목록 (충전/방전 공통) */
static const uint8_t s_popup_warn_ids[] = {0x0A,0x0B,0x16,0x17,0x19};
static const uint8_t s_popup_disc_ids[] = {0x15,0x18,0x1A,0x1B,0x1C};
static const uint8_t s_popup_ntfy_ids[] = {0x1D,0x1E,0x1F,0x20,0x21};
static const uint8_t s_popup_done_ids[] = {0x22,0x23,0x24,0x25,0x26};
static const uint8_t s_popup_fail_ids[] = {0x27,0x28,0x29,0x2A,0x2B};
static const uint8_t s_popup_stop_ids[] = {0x11,0x14,0x2C,0x2D,0x2E};
static const uint8_t s_popup_warn_temp_ids[] = {0x2F,0x33,0x31,0x30,0x32,0x34};
static const uint8_t s_popup_caut_temp_ids[] = {0x35,0x39,0x37,0x3B,0x36,0x38,0x3A};

// /* touch disable IDs (충전/방전 상이) */
// static const uint8_t s_chg_touch_dis[] = {0x02,0x06,0x09};  /* 충전: id=2,6,9 */
// static const uint8_t s_dis_touch_dis[] = {0x06,0x07};        /* 방전: id=6,7 */

static void LCD_DoPopup(uint8_t screen, const uint8_t *show_ids, uint8_t show_cnt)
{
    for (uint8_t i = 0; i < show_cnt; i++)
        LCD_PostShowHide(screen, show_ids[i], LCD_ON);
    /* TODO: B1 04 touch disable이 LCD show/hide를 깨는지 확인 후 재활성화
    const uint8_t *t_ids;
    uint8_t t_cnt;
    if (screen == LCD_SCR_CHARGE) {
        t_ids = s_chg_touch_dis; t_cnt = (uint8_t)sizeof(s_chg_touch_dis);
    } else {
        t_ids = s_dis_touch_dis; t_cnt = (uint8_t)sizeof(s_dis_touch_dis);
    }
    for (uint8_t i = 0; i < t_cnt; i++)
        LCD_PostTouchEnable(screen, t_ids[i], LCD_OFF);
    */
}

/* Screen0 알림 팝업2/3용 show IDs */
static const uint8_t s_popup_ntfy2_ids[] = {0x2C,0x2D,0x2E,0x2F,0x30};
static const uint8_t s_popup_ntfy3_ids[] = {0x31,0x32,0x33,0x34,0x35};

void LCD_PostPopupWarning(uint8_t screen)
    { LCD_DoPopup(screen, s_popup_warn_ids, (uint8_t)sizeof(s_popup_warn_ids)); }
void LCD_PostPopupDisconnect(uint8_t screen)
    { LCD_DoPopup(screen, s_popup_disc_ids, (uint8_t)sizeof(s_popup_disc_ids)); }
void LCD_PostPopupNotify(uint8_t screen)
    { LCD_DoPopup(screen, s_popup_ntfy_ids, (uint8_t)sizeof(s_popup_ntfy_ids)); }
void LCD_PostPopupWorkDone(uint8_t screen)
    { LCD_DoPopup(screen, s_popup_done_ids, (uint8_t)sizeof(s_popup_done_ids)); }
void LCD_PostPopupDriveFail(uint8_t screen)
    { LCD_DoPopup(screen, s_popup_fail_ids, (uint8_t)sizeof(s_popup_fail_ids)); }
void LCD_PostPopupStop(uint8_t screen)
    { LCD_DoPopup(screen, s_popup_stop_ids, (uint8_t)sizeof(s_popup_stop_ids)); }
void LCD_PostPopupCautTemp(uint8_t screen)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "제어온도 : %d℃  토출온도 : %d℃",
                (int)g_temp_value[0], (int)g_temp_value[1]);

        for (int i = 1; i < 3; i++)
        {
            LCD_PostTextSet(i, 54, "충·방전기 내부 온도가 상승 중입니다.");
            LCD_PostTextSet(i, 56, "계속 진행 하시겠습니까?");
            LCD_PostTextSet(i, 58, buf);
        }
        LCD_DoPopup(screen, s_popup_caut_temp_ids, (uint8_t)sizeof(s_popup_caut_temp_ids)); 
    }
void LCD_PostPopupWarnTemp(uint8_t screen)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "제어온도 : %d℃  토출온도 : %d℃",
                (int)g_temp_value[0], (int)g_temp_value[1]);

        for (int i = 1; i < 3; i++)
        {
            LCD_PostTextSet(i, 48, "충·방전기 내부 고온 감지로 인하여");
            LCD_PostTextSet(i, 50, "구동을 종료합니다.");
            LCD_PostTextSet(i, 52, buf);
        }
        LCD_DoPopup(screen, s_popup_warn_temp_ids, (uint8_t)sizeof(s_popup_warn_temp_ids)); 
    }



/* 통신중 (Screen0, id=1): show/hide 1개 */
void LCD_PostPopupComm(uint8_t show)
    { LCD_PostShowHide(LCD_SCR_MAIN, 0x01, show); }

/* 알림 팝업2/3 (Screen0): touch disable 없음 */
void LCD_PostPopupNotify2(void)
{
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_ntfy2_ids); i++)
        LCD_PostShowHide(LCD_SCR_MAIN, s_popup_ntfy2_ids[i], LCD_ON);
}
void LCD_PostPopupNotify3(void)
{
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_ntfy3_ids); i++)
        LCD_PostShowHide(LCD_SCR_MAIN, s_popup_ntfy3_ids[i], LCD_ON);
}

void LCD_PostRtcSet(uint16_t year, uint8_t mon, uint8_t day,
                    uint8_t hour, uint8_t min, uint8_t sec)
{
    if (!g_lcd_evt_queue) return;
    stLCD_Event evt = { .type = LCD_EVT_RTC_SET };
    evt.text[0] = (uint8_t)(year >> 8);
    evt.text[1] = (uint8_t)(year & 0xFF);
    evt.text[2] = mon;
    evt.text[3] = day;
    evt.text[4] = hour;
    evt.text[5] = min;
    evt.text[6] = sec;
    evt.text_len = 7;
    xQueueSend(g_lcd_evt_queue, &evt, 0);
}

/*----------------------------------------------------------------------
 *  Task 
 *--------------------------------------------------------------------*/

void InitLcdTask(void)
{
    LCD_LOG("[LCD Task] Init\r\n");

    g_lcd_evt_queue = xQueueCreateStatic(LCD_EVT_QUEUE_DEPTH,
                                         sizeof(stLCD_Event),
                                         g_lcd_evt_storage,
                                         &g_lcd_evt_queue_buf);

    HAL_UART_Receive_IT(&huart8, &g_uart8_rx_byte, 1);

    lcdTaskHandle = osThreadNew(StartLcdTask, NULL, &lcdTask_attributes);
}

void LCD_PostVehicleInfoDefault(void)
{
    LCD_PostTextSet(LCD_SCR_MAIN,      8,  " 차량 연결 필요 ");
    LCD_PostTextSet(LCD_SCR_CHARGE,    14, " 차량 연결 필요 ");
    LCD_PostTextSet(LCD_SCR_DISCHARGE, 14, " 차량 연결 필요 ");
    LCD_PostTextSet(LCD_SCR_SETTING,   18, " 차량 연결 필요 ");
}

static void LCD_WaitReadyAndInit(void)
{
    uint32_t start = HAL_GetTick();
    uint32_t last_probe = 0;

    LCD_LOG("[LCD] waiting for LCD module ready ...\r\n");

    while (!g_lcd_ready) {
        /* RX 폴링 — ISR 가 ring 에 넣으면 여기서 파싱 → g_lcd_ready set */
        LCD_ProcessRx();

        uint32_t now = HAL_GetTick();
        if ((now - start) >= LCD_READY_TIMEOUT_MS) {
            LCD_LOG("[LCD] ready timeout (%lums) -> proceed anyway\r\n", (unsigned long)LCD_READY_TIMEOUT_MS);
            break;
        }

        if ((now - last_probe) >= LCD_READY_PROBE_MS) {
            last_probe = now;
            uint8_t probe[9] = {
                LCD_SOF1, LCD_SOF2, LCD_CMD_SCREEN, 0x00, LCD_SCR_MAIN,
                LCD_EOF1, LCD_EOF2, LCD_EOF3, LCD_EOF4
            };
            LCD_Transmit(probe, sizeof(probe));
        }
        osDelay(20);
    }

    LCD_LOG("[LCD] init TX start (elapsed=%lums)\r\n",
            (unsigned long)(HAL_GetTick() - start));

    /* LCD TEXT initial setting — after LCD ready */
    LCD_GoMainAndUpdateBTStatus();
    LCD_PostVehicleInfoDefault();   /* 차량 정보 4개 화면 기본 문구 ("차량 연결 필요") */
    LCD_PostTextSet(LCD_SCR_CHARGE, 3, "--");
    LCD_PostTextSet(LCD_SCR_DISCHARGE, 3, "--");
}

void StartLcdTask(void *argument)
{
    LCD_LOG("start %s ... \r\n", __FUNCTION__);

    LCD_WaitReadyAndInit();

    for (;;)
    {
        LCD_ProcessRx();
        LCD_ProcessEvents();
        osDelay(10);
    }
}

/*----------------------------------------------------------------------
 *  Event Handler
 *--------------------------------------------------------------------*/

static void LCD_ProcessEvents(void) // Execute all Event in Queue
{
    stLCD_Event evt;
    while (xQueueReceive(g_lcd_evt_queue, &evt, 0) == pdTRUE) {
        LCD_Execute(&evt);
    }
}

static void LCD_Execute(const stLCD_Event *evt)
{
    switch (evt->type) {
        case LCD_EVT_BTN_STATE:
            LCD_SendBtnState(evt->screen, evt->ctrl_id, evt->value);
            break;

        case LCD_EVT_SCREEN_GOTO:
            LCD_SendScreenGoto(evt->screen);
            break;

        case LCD_EVT_EMERGENCY_STOP:
            LCD_SendEmergencyStop();
            if (g_lcd_current_page == LCD_SCR_CHARGE ||
                g_lcd_current_page == LCD_SCR_DISCHARGE) {
                LCD_SendShowHide(g_lcd_current_page, 10, 1);
                LCD_SendShowHide(g_lcd_current_page, 11, 1);
            }
            break;

        case LCD_EVT_TEXT_SET:
            LCD_SendTextSet(evt->screen, evt->ctrl_id, evt->text, evt->text_len);
            break;

        case LCD_EVT_TEXT_READ:
            LCD_SendTextRead(evt->screen, evt->ctrl_id);
            break;

        case LCD_EVT_SHOW_HIDE:
            LCD_SendShowHide(evt->screen, evt->ctrl_id, evt->value);
            break;

        case LCD_EVT_TOUCH_ENABLE:
            LCD_SendTouchEnable(evt->screen, evt->ctrl_id, evt->value);
            break;

        case LCD_EVT_RTC_SET:
        {
            uint16_t year = ((uint16_t)evt->text[0] << 8) | evt->text[1];
            LCD_SendRtcSet(year, evt->text[2], evt->text[3],
                           evt->text[4], evt->text[5], evt->text[6]);
            break;
        }

        default:
            break;
    }
}

/*----------------------------------------------------------------------
 *  TX Make Frame
 *--------------------------------------------------------------------*/

/**
 * @brief  Btn ON/OFF State Set
 *  Frame (12B): EE B1 10 00 [scr] 00 [btn] [on/off] FF FC FF FF
 *  Protocol: B1+0x10, Screen_id(2B BE) + Control_id(2B BE) + Status(1B)
 */
static void LCD_SendBtnState(uint8_t screen, uint8_t button, uint8_t on_off)
{
    uint8_t frame[12] = {
        LCD_SOF1, LCD_SOF2,
        LCD_CMD_SET,
        0x00, screen,   /* Screen_id (2B BE) */
        0x00, button,   /* Control_id (2B BE) */
        on_off,         /* Status: 0x00=bounce, 0x01=press */
        LCD_EOF1, LCD_EOF2, LCD_EOF3, LCD_EOF4
    };
    LCD_Transmit(frame, sizeof(frame));
    // LCD_LOG("[LCD TX] BtnState scr=%d btn=%d val=%d\r\n", screen, button, on_off);
}

/**
 * @brief  Scr Change
 *  Frame (9B): EE B1 00 00 [scr] FF FC FF FF
 *  Protocol: B1+0x00, Screen_id(2B BE) <- Big Endian
 */
static void LCD_SendScreenGoto(uint8_t screen)
{
    static uint8_t s_last_screen = LCD_SCR_MAIN;

    uint8_t frame[9] = {
        LCD_SOF1, LCD_SOF2,
        LCD_CMD_SCREEN,
        0x00, screen,   /* Screen_id (2B BE) */
        LCD_EOF1, LCD_EOF2, LCD_EOF3, LCD_EOF4
    };
    LCD_Transmit(frame, sizeof(frame));
    // LCD_LOG("[LCD TX] ScreenGoto scr=%d\r\n", screen);

    /* non-0 → 0 진입 edge 에서 R/Y 에러 표시 클리어 → LED NORMAL(G) */
    if (s_last_screen != LCD_SCR_MAIN && screen == LCD_SCR_MAIN) {
        if (LED_HasActiveAlarm()) {
            LED_ClearAll();
            LCD_LOG("[LCD] scr0 entry: LED NORMAL\r\n");
        }
    }
    s_last_screen = screen;
}

/**
 * @brief  Emergency  Stop
 *  Frame (12B): EE B1 AB CD EF FE DC BA FF FC FF FF
 */
static void LCD_SendEmergencyStop(void)
{
    LCD_Transmit(LCD_FRAME_EMGSTOP, LCD_EMGSTOP_LEN);
    LCD_LOG("[LCD TX] EmergencyStop\r\n");
}

/**
 * @brief  Text Set (Variable Length)
 *  Frame: EE B1 10 00 [scr] 00 [ctrl] [ascii...] FF FC FF FF
 */
static void LCD_SendTextSet(uint8_t screen, uint8_t ctrl_id,
                             const uint8_t *text, uint8_t len)
{
    if (len == 0 || len > LCD_TEXT_MAX) return;

    uint8_t frame[LCD_TX_BUF_MAX];
    uint8_t idx = 0;

    frame[idx++] = LCD_SOF1;
    frame[idx++] = LCD_SOF2;
    frame[idx++] = LCD_CMD_SET;
    frame[idx++] = 0x00;     /* Screen_id MSB */
    frame[idx++] = screen;   /* Screen_id LSB */
    frame[idx++] = 0x00;     /* Control_id MSB */
    frame[idx++] = ctrl_id;  /* Control_id LSB */
    memcpy(&frame[idx], text, len);
    idx += len;
    frame[idx++] = LCD_EOF1;
    frame[idx++] = LCD_EOF2;
    frame[idx++] = LCD_EOF3;
    frame[idx++] = LCD_EOF4;

    LCD_Transmit(frame, idx);
    // LCD_LOG("[LCD TX] TextSet scr=%d(%s) ctrl=%d len=%d\r\n", screen,
    //        (screen == LCD_SCR_MAIN)      ? "MAIN" :
    //        (screen == LCD_SCR_CHARGE)    ? "CHARGE" :
    //        (screen == LCD_SCR_DISCHARGE) ? "DISCHARGE" :
    //        (screen == LCD_SCR_SETTING)   ? "SETTING" : "?",
    //        ctrl_id, len);

    /* g_lcdState Text Field Update */
    {
        char *dst = NULL;
        if      (screen == LCD_SCR_MAIN      && ctrl_id ==  1) dst = g_lcdState.scr0_text1;
        else if (screen == LCD_SCR_MAIN      && ctrl_id ==  2) dst = g_lcdState.scr0_text2;
        else if (screen == LCD_SCR_CHARGE    && ctrl_id ==  3) dst = g_lcdState.scr1_text3;
        else if (screen == LCD_SCR_DISCHARGE && ctrl_id ==  3) dst = g_lcdState.scr2_text3;
        else if (screen == LCD_SCR_SETTING   && ctrl_id ==  2) dst = g_lcdState.scr3_text2;
        else if (screen == LCD_SCR_SETTING   && ctrl_id == 14) dst = g_lcdState.scr3_text14;
        else if (screen == LCD_SCR_SETTING   && ctrl_id == 17) dst = g_lcdState.scr3_text17;
        if (dst) {
            memcpy(dst, text, len);
            dst[len] = '\0';
        }
    }
}

/**
 * @brief  Text Read Request
 *  Frame (11B): EE B1 11 00 [scr] 00 [btn] FF FC FF FF
 *  Protocol: B1+0x11, Screen_id(2B BE) + Control_id(2B BE)
 */
static void LCD_SendTextRead(uint8_t screen, uint8_t ctrl_id)
{
    uint8_t frame[11] = {
        LCD_SOF1, LCD_SOF2,
        LCD_CMD_READ,
        0x00, screen,    /* Screen_id (2B BE) */
        0x00, ctrl_id,   /* Control_id (2B BE) */
        LCD_EOF1, LCD_EOF2, LCD_EOF3, LCD_EOF4
    };
    LCD_Transmit(frame, sizeof(frame));
    // LCD_LOG("[LCD TX] TextRead scr=%d ctrl=%d\r\n", screen, ctrl_id);
}

/**
 * @brief  Control Show/Hide
 *  Frame (12B): EE B1 03 [scr] 00 [btn] 00 [1=show|0=hide] FF FC FF FF
 *  Protocol: B1+0x03, Screen_id(2B BE) + Control_id(2B BE) + Enable(1B)
 */
static void LCD_SendShowHide(uint8_t screen, uint8_t ctrl_id, uint8_t show)
{
    uint8_t frame[12] = {
        LCD_SOF1, LCD_SOF2,
        LCD_CMD_SHOWHIDE,
        0x00, screen,    /* Screen_id (2B BE) */
        0x00, ctrl_id,   /* Control_id (2B BE) */
        show,            /* 0x00=hide, 0x01=show */
        LCD_EOF1, LCD_EOF2, LCD_EOF3, LCD_EOF4
    };
    LCD_Transmit(frame, sizeof(frame));
    // LCD_LOG("[LCD TX] ShowHide scr=%d ctrl=%d show=%d\r\n", screen, ctrl_id, show);
}

/**
 * @brief  Control Touch Enable/Disable
 *  Frame (12B): EE B1 04 [scr] 00 [ctrl_id] 00 [enable] FF FC FF FF
 *  Protocol: B1+0x04, Screen_id(2B BE) + Control_id(2B BE) + Enable(1B)
 */
static void LCD_SendTouchEnable(uint8_t screen, uint8_t ctrl_id, uint8_t enable)
{
    uint8_t frame[12] = {
        LCD_SOF1, LCD_SOF2,
        LCD_CMD_TOUCH,
        0x00, screen,    /* Screen_id (2B BE) */
        0x00, ctrl_id,   /* Control_id (2B BE) */
        enable,          /* 0x00=비활성, 0x01=활성 */
        LCD_EOF1, LCD_EOF2, LCD_EOF3, LCD_EOF4
    };
    LCD_Transmit(frame, sizeof(frame));
    LCD_LOG("[LCD TX] TouchEnable scr=%d ctrl=%d en=%d\r\n", screen, ctrl_id, enable);
}

/**
 * @brief  BCD Change 
 */
static inline uint8_t LCD_ToBcd(uint8_t val)
{
    return (uint8_t)(((val / 10) << 4) | (val % 10));
}

/**
 * @brief  WeekDay Cal (Tomohiko Sakamoto Algorithm)
 * @return 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat
 */
static uint8_t LCD_CalcWeekday(uint16_t year, uint8_t mon, uint8_t day)
{
    static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (mon < 3) year--;
    return (uint8_t)((year + year/4 - year/100 + year/400 + t[mon-1] + day) % 7);
}

/**
 * @brief  LCD RTC Time Set
 *  Frame (12B): EE 81 Sec Min Hour Day Week Mon Year FF FC FF FF
 *  Protocol: 0x81, Each 1 byte BCD, Year = Lower 2 digits
 */
static void LCD_SendRtcSet(uint16_t year, uint8_t mon, uint8_t day,
                           uint8_t hour, uint8_t min, uint8_t sec)
{
    uint8_t week = LCD_CalcWeekday(year, mon, day);
    uint8_t frame[13] = {
        LCD_SOF1, 0x81,
        LCD_ToBcd(sec),
        LCD_ToBcd(min),
        LCD_ToBcd(hour),
        LCD_ToBcd(day),
        LCD_ToBcd(week),
        LCD_ToBcd(mon),
        LCD_ToBcd((uint8_t)(year % 100)),
        LCD_EOF1, LCD_EOF2, LCD_EOF3, LCD_EOF4
    };
    LCD_Transmit(frame, sizeof(frame));
    LCD_LOG("[LCD TX] RtcSet %04d-%02d-%02d %02d:%02d:%02d week=%d\r\n",
           year, mon, day, hour, min, sec, week);
}

/**
 * @brief  UART8 TX Common Func
 */
static void LCD_Transmit(const uint8_t *frame, uint16_t len)
{
    // LCD_LOG("[LCD TX RAW]");
    // for (uint16_t i = 0; i < len; i++)
    //     LCD_LOG(" %02X", frame[i]);
    // LCD_LOG("\r\n");

    HAL_StatusTypeDef ret = HAL_UART_Transmit(&huart8, (uint8_t *)frame, len, 200);
    if (ret != HAL_OK)
        LCD_LOG("[LCD] UART8 TX ERR: %d (BUSY=2,TIMEOUT=3)\r\n", (int)ret);
}

/*----------------------------------------------------------------------
 *  RX Process - EOF(FF FC FF FF) Detection Method
 *
 *  Example Received Frames:
 *    EE B1 01 00 [page] FF FC FF FF          (9B, Page Change Notification)
 *    EE B1 11 00 [scr] 00 [btn] [data...] FF FC FF FF  (Variable, Button/Text Notification)
 *--------------------------------------------------------------------*/
static void LCD_ProcessRx(void)
{
    static uint8_t  buf[LCD_RX_FRAME_MAX];
    static uint8_t  idx = 0;
    static uint32_t last_rx_tick = 0;

    /* Clear buffer if partial frame unused for 100ms */
    if (idx > 0 && g_lcd_rx_head == g_lcd_rx_tail) {
        if ((HAL_GetTick() - last_rx_tick) > 100U) {
            idx = 0;
        }
        return;
    }

    while (g_lcd_rx_head != g_lcd_rx_tail) {
        uint8_t b = g_lcd_rx_ring[g_lcd_rx_tail];
        g_lcd_rx_tail = (uint16_t)((g_lcd_rx_tail + 1) % LCD_RXBUF_SIZE);

        /* Prevent buffer overflow */
        if (idx >= LCD_RX_FRAME_MAX) { idx = 0; }

        buf[idx++] = b;
        last_rx_tick = HAL_GetTick();

        /* SOF sync */
        if (idx == 1 && buf[0] != LCD_SOF1) { idx = 0; continue; }
        /* B1=명령응답 또는 0x00~0x03=screen_id 버튼이벤트만 허용 */
        if (idx == 2 && buf[1] != LCD_SOF2 && buf[1] > LCD_SCR_SETTING) { idx = 0; continue; }

        /* 최소 프레임(SOF2 + CMD + data1 + EOF4 = 7B) 미만이면 대기 */
        if (idx < 7) continue;

        /* EOF 탐지: 마지막 4바이트가 FF FC FF FF */
        if (buf[idx-4] == LCD_EOF1 && buf[idx-3] == LCD_EOF2 &&
            buf[idx-2] == LCD_EOF3 && buf[idx-1] == LCD_EOF4) {
            /* 프레임 전체 덤프 */
            LCD_LOG("[LCD RX]");
            for (uint8_t i = 0; i < idx; i++)
                LCD_LOG(" %02X", buf[i]);
            LCD_LOG("\r\n");
            LCD_ParseRxFrame(buf, idx);
            idx = 0;
        }
    }
}

/**
 * @brief  Parse RX Frame
 *
 *  ① EE B1 XX ... FF FC FF FF  — Command Response Frame (buf[1]==B1)
 *  ② EE [scr 2B LE] [ctrl 2B LE] [val 2B LE] FF FC FF FF (11B) — Button Event
 */
static void LCD_ParseRxFrame(const uint8_t *buf, uint8_t len)
{
    if (len < 7) return;

    /* 첫 valid RX 프레임 수신 시 LCD ready 표시 — InitLcdTask 의 init TX 대기 해제 트리거 */
    if (!g_lcd_ready) {
        g_lcd_ready = true;
        LCD_LOG("[LCD] ready (first RX frame received)\r\n");
    }

    if (buf[1] == LCD_SOF2) {
        uint8_t cmd = buf[2];

        if (cmd == LCD_CMD_PAGE_CHG && len == 9) {
            /* EE B1 01 00 [page] FF FC FF FF */
            uint8_t newPage = buf[4];
            if (newPage <= LCD_SCR_SETTING) {
                LCD_LOG("[LCD RX] page: %d -> %d\r\n", g_lcd_current_page, newPage);
                g_lcdState.prev_screen = g_lcd_current_page;
                g_lcdState.curr_screen = newPage;
                g_lcd_current_page     = newPage;
            }
        } else if (cmd == LCD_CMD_READ && len >= 14) {
            /* EE B1 11 00 [scr] 00 [btn] [ctrl_type] [...] FF FC FF FF
             *  buf[4]=screen, buf[6]=button, buf[7]=Control_type        */
            uint8_t screen       = buf[4];
            uint8_t button       = buf[6];
            uint8_t control_type = buf[7];

            if (control_type == 0x10 && len == 14) {
                uint8_t status = buf[9];
                LCD_LOG("[LCD RX] btn scr=%d btn=%d status=%d\r\n", screen, button, status);
                /* Call button event dispatcher (only execute handler for status=1 press) */
                LCD_DispatchButtonEvent(screen, button, status);
            } else if (control_type == 0x11 && len > 13) {
                uint8_t data_len = len - 13;
                if (data_len <= LCD_TEXT_MAX)
                    LCD_UpdateStateFromRx(screen, button, &buf[8], data_len);
            }
        }
    } else if (len == 11) {
        /* ② btn event : EE [scr 2B LE] [ctrl 2B LE] [val 2B LE] FF FC FF FF */
        uint8_t screen = buf[1];   /* screen LSB (MSB=buf[2] always 0x00) */
        uint8_t ctrl   = buf[3];   /* ctrl_id LSB */
        uint8_t value  = buf[5];   /* value LSB */
        LCD_DispatchButtonEvent(screen, ctrl, value);
    }
}

/**
 * @brief  LCD Button Event Dispatcher
 *  value==0(release) is ignored, only press(1) is passed to the handler
 */
static void LCD_DispatchButtonEvent(uint8_t screen, uint8_t ctrl, uint8_t value)
{
    LCD_LOG("[LCD RX] btn scr=%d ctrl=0x%02X val=%d\r\n", screen, ctrl, value);
    if (value == 0) return;

    switch (screen) {
        case LCD_SCR_CHARGE:
            switch (ctrl) {
                case LCD_RX_CHARGE_STOP:       LCD_OnCharge_Stop();         break;
                case LCD_RX_CHARGE_DISC_OK:    LCD_OnCharge_DiscOk();       break;
                case LCD_RX_CHARGE_NOTIFY_NO:  LCD_OnCharge_NotifyNo();     break;
                case LCD_RX_CHARGE_NOTIFY_YES: LCD_OnCharge_NotifyYes();    break;
                case LCD_RX_CHARGE_DONE_OK:    LCD_OnCharge_WorkDoneOk();   break;
                case LCD_RX_CHARGE_FAIL_OK:    LCD_OnCharge_DriveFailOk();  break;
                case LCD_RX_CHARGE_STOP_OK:    LCD_OnCharge_StopOk();       break;
                case LCD_RX_CHARGE_WARN_TEMP_OK:   LCD_OnCharge_WarnTempOk();   break;
                case LCD_RX_CHARGE_CAUT_TEMP_STOP: LCD_OnCharge_CautTempStop(); break;
                default: break;
            }
            break;
        case LCD_SCR_DISCHARGE:
            switch (ctrl) {
                case LCD_RX_DISC_STOP:         LCD_OnDischarge_Stop();          break;
                case LCD_RX_DISC_WARN_OK:      LCD_OnDischarge_WarnOk();        break;
                case LCD_RX_DISC_DISC_OK:      LCD_OnDischarge_DiscOk();        break;
                case LCD_RX_DISC_NOTIFY_NO:    LCD_OnDischarge_NotifyNo();      break;
                case LCD_RX_DISC_NOTIFY_YES:   LCD_OnDischarge_NotifyYes();     break;
                case LCD_RX_DISC_DONE_OK:      LCD_OnDischarge_WorkDoneOk();    break;
                case LCD_RX_DISC_FAIL_OK:      LCD_OnDischarge_DriveFailOk();   break;
                case LCD_RX_DISC_STOP_OK:      LCD_OnDischarge_StopOk();        break;
                case LCD_RX_DISC_WARN_TEMP_OK:   LCD_OnDischarge_WarnTempOk();   break;
                case LCD_RX_DISC_CAUT_TEMP_STOP: LCD_OnDischarge_CautTempStop(); break;
                default: break;
            }
            break;
        case LCD_SCR_MAIN:
            switch (ctrl) {
                case LCD_RX_MAIN_NOTIFY2_OK:   LCD_OnMain_Notify2Ok(); break;
                case LCD_RX_MAIN_NOTIFY3_OK:   LCD_OnMain_Notify3Ok(); break;
                default: break;
            }
            break;
        default:
            break;
    }
}

/*----------------------------------------------------------------------
 *  LCD Button Event Handlers
 *
 *  - weak  : Empty default implementation (can be overridden in other sources)
 *  - strong: Direct implementation in this file (LCD STOP/Notify YES 4 types)
 *
 *  Strong definitions (4 types):
 *    ① scr=1 btn=6  (LCD_RX_CHARGE_STOP)  : Charge stop button
 *         → Display Notify popup on charge screen
 *    ② scr=2 btn=7  (LCD_RX_DISC_STOP)    : Discharge stop button
 *         → Display Notify popup on discharge screen
 *    ③ scr=1 btn=33 (NOTIFY_YES, 0x21)    : Charge Notify popup "OK"
 *         → Execute the same stop sequence as BLE 0x91 STOP (CHARGE)
 *    ④ scr=2 btn=33 (NOTIFY_YES, 0x21)    : Discharge Notify popup "OK"
 *         → Execute the same stop sequence as BLE 0x91 STOP (DISCHARGE)
 *
 *  Common stop sequence: RELAY_process(NONE) + EXT_RLY 1~4 OFF + WorkDone popup
 *--------------------------------------------------------------------*/

/* Functions defined in task-hwcontrol.c (header not exposed) */
extern void RELAY_process(SystemMode_t state);
extern void IO_EXT_RLY_control(int ch, bool enable);
extern void IO_ALL_OFF_control(void);

static void LCD_GoMainAndUpdateBTStatus(void)
{
    LCD_PostScreenGoto(LCD_SCR_MAIN);
    LCD_MainScreenStatusUpdate(COMM_IsBTConnected() ? 1 : 0);
}

static void LCD_ClosePopupAndGoMain(uint8_t screen, const uint8_t *ids, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
        LCD_PostShowHide(screen, ids[i], LCD_OFF);
    LCD_GoMainAndUpdateBTStatus();
}

/* ① Charge stop button → Notify popup
 *  ※ LCD의 종료 버튼은 누르면 자체적으로 메인(page=0)으로 자동 복귀하는
 *     Lua 액션이 걸려 있어, 그냥 ShowHide만 보내면 popup이 보이지 않음.
 *     → ScreenGoto(CHARGE)로 LCD를 충전 화면으로 강제 복귀 후 popup 표출. */
void LCD_OnCharge_Stop(void)
{
    LCD_LOG("[LCD] CHARGE stop button (scr=1 btn=6) -> Notify popup\r\n");
    LCD_PostScreenGoto(LCD_SCR_CHARGE);    /* LCD 페이지 강제 복귀 */
    LCD_PostPopupNotify(LCD_SCR_CHARGE);
}

/* ② Discharge stop button → Notify popup (same as page return then popup) */
void LCD_OnDischarge_Stop(void)
{
    LCD_LOG("[LCD] DISCHARGE stop button (scr=2 btn=7) -> Notify popup\r\n");
    LCD_PostScreenGoto(LCD_SCR_DISCHARGE); /* LCD 페이지 강제 복귀 */
    LCD_PostPopupNotify(LCD_SCR_DISCHARGE);
}

/* ③ Charge Notify popup "OK" → Execute the same stop sequence as BLE 0x91 STOP (CHARGE) */
void LCD_OnCharge_NotifyYes(void)
{
    LCD_LOG("[LCD] CHARGE Notify YES (btn=33) -> STOP sequence (== BLE 0x91 STOP)\r\n");

    SENSOR_ErrorCheck_Disarm();   /* 정지 직후 센서 에러 체크 중단 */
    g_bDisplayActive = false;     /* 정지 직후 주기 상태 송신(0x51) 중단 */

    /* (1) 진입 즉시 모든 IO OFF (BLE 0x91 STOP과 동일) */
    IO_ALL_OFF_control();

    g_device_state    = eDEVICE_STATE_STOP;
    g_plcStepWaitStop = true;

    /* (2) CHARGE 분기: CP 중단 + 릴레이 OFF */
    RELAY_process(eMODE_NONE);
    LCD_LOG("[LCD] CHARGE: CP stop, relay OFF\r\n");

    /* (2b) EXT_RLY 1~4 명시적 OFF — 안전장치 */
    for (int rly = 1; rly <= 4; rly++) {
        IO_EXT_RLY_control(rly, false);
    }
    LCD_LOG("[LCD] CHARGE: EXT_RLY 1~4 forced OFF\r\n");

    /* (3) 메인 화면으로 복귀 */
    LCD_GoMainAndUpdateBTStatus();

    /* (4) 에러코드 4 (User LCD Control Stop) 1회 TX 후 클리어 */
    g_error_code = ERROR_CODE_USER_LCD_CONTROL_STOP;
    FL_GDS_Send_Error_Code();
    g_error_code = ERROR_CODE_NONE;
    LCD_LOG("[LCD] CHARGE: err=4 TX done -> cleared\r\n");

    /* (5) 시스템 안전 상태로 강제 전이 */
    g_system_mode  = eMODE_NONE;
    g_system_state = eSYSTEM_STATE_NONE;
    g_device_state = eDEVICE_STATE_NONE;  /* STOP 동작 완료 → NONE 으로 정리 */
    LCD_LOG("[LCD] CHARGE STOP done -> g_device_state = NONE\r\n");

    /* (6) LED 표시 — 별도 호출 불필요. g_device_state 가 START 를 벗어났으므로
     *     led-indicator.c 가 자동으로 LED_G_BLINK(대기)로 전환한다. */

    /* 3분 후 SS1~SS4 OFF (모든 OFF-제어 릴레이 LOW 확인 시) */
    FAN_CooldownArm();
}

/* ⑤a 충전 화면 연결해제(BLE Disconnect) 팝업 "확인" → 팝업 숨김 + 메인 복귀 */
void LCD_OnCharge_DiscOk(void)
{
    LCD_LOG("[LCD] CHARGE Disconnect popup OK -> close popup + go MAIN\r\n");
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_disc_ids); i++)
        LCD_PostShowHide(LCD_SCR_CHARGE, s_popup_disc_ids[i], LCD_OFF);
    LCD_GoMainAndUpdateBTStatus();
}

/* ⑤b 방전 화면 연결해제(BLE Disconnect) 팝업 "확인" → 팝업 숨김 + 메인 복귀 */
void LCD_OnDischarge_DiscOk(void)
{
    LCD_LOG("[LCD] DISCHARGE Disconnect popup OK -> close popup + go MAIN\r\n");
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_disc_ids); i++)
        LCD_PostShowHide(LCD_SCR_DISCHARGE, s_popup_disc_ids[i], LCD_OFF);
    LCD_GoMainAndUpdateBTStatus();
}

/* ⑤ 충전 작업완료 팝업 "확인" 클릭 → 팝업 숨김 + 메인 화면 복귀 */
void LCD_OnCharge_WorkDoneOk(void)
{
    LCD_LOG("[LCD] CHARGE WorkDone OK (btn=36) -> close popup + go MAIN\r\n");

    /* 작업완료 팝업 컨트롤 hide (5개) */
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_done_ids); i++)
        LCD_PostShowHide(LCD_SCR_CHARGE, s_popup_done_ids[i], LCD_OFF);

    /* 메인 화면으로 복귀 */
    LCD_GoMainAndUpdateBTStatus();
}

/* ⑥ 방전 작업완료 팝업 "확인" 클릭 → 팝업 숨김 + 메인 화면 복귀 */
void LCD_OnDischarge_WorkDoneOk(void)
{
    LCD_LOG("[LCD] DISCHARGE WorkDone OK (btn=36) -> close popup + go MAIN\r\n");

    /* 작업완료 팝업 컨트롤 hide (5개) */
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_done_ids); i++)
        LCD_PostShowHide(LCD_SCR_DISCHARGE, s_popup_done_ids[i], LCD_OFF);

    /* 메인 화면으로 복귀 */
    LCD_GoMainAndUpdateBTStatus();
}

/* ④ 방전 알림팝업 "확인" → BLE 0x91 STOP (DISCHARGE/BSA)과 동일 시퀀스 */
void LCD_OnDischarge_NotifyYes(void)
{
    LCD_LOG("[LCD] DISCHARGE Notify YES (btn=33) -> STOP sequence (== BLE 0x91 STOP)\r\n");

    SENSOR_ErrorCheck_Disarm();   /* 정지 직후 센서 에러 체크 중단 */
    g_bDisplayActive = false;     /* 정지 직후 주기 상태 송신(0x51) 중단 */

    /* (1) 진입 즉시 모든 IO OFF (BLE 0x91 STOP과 동일) */
    IO_ALL_OFF_control();

    g_device_state    = eDEVICE_STATE_STOP;
    g_plcStepWaitStop = true;

    /* (2) 모드별 정지 시퀀스 */
    if (g_system_mode == eMODE_VEHICLE_DISCHARGE) {
        /* VEHICLE_DISCHARGE: stepno=99 정지 데이터 + SECC NormalStop */
        PLC_ApplyStopSteps();
        PLC_SetTxValue(PLC_REQ_ChargeControl, PLC_MSGDisp_ChargingControl_NormalStop);
        LCD_LOG("[LCD] VEHICLE_DISCHARGE: stop-step + NormalStop sent to SECC\r\n");
        osDelay(200);

        /* EXT_RLY 4→1 OFF (역순 안전 종료, 100ms 간격) */
        for (int rly = 4; rly >= 1; rly--) {
            IO_EXT_RLY_control(rly, false);
            LCD_LOG("[LCD] EXT_RLY%d OFF\r\n", rly);
            if (rly > 1) osDelay(100);
        }
    }
    else if (g_system_mode == eMODE_BSA_DISCHARGE) {
        /* BSA_DISCHARGE: BSA TX 즉시 중단 (g_bBatRelayConFlag=false + FDCAN1 stop) */
        extern void BSA_Stop(void);
        BSA_Stop();
        LCD_LOG("[LCD] BSA_DISCHARGE: BSA_Stop done\r\n");
    }

    g_plcManualRun = false;

    /* (3b) EXT_RLY 1~4 명시적 OFF — BSA self-healing race 가드
     *  · BSA_Stop 으로 g_bBatRelayConFlag=false → self-healing 비활성화 후
     *    한번 더 명시 OFF 하여 race 로 다시 켜진 경우 보장 */
    for (int rly = 1; rly <= 4; rly++) {
        IO_EXT_RLY_control(rly, false);
    }
    LCD_LOG("[LCD] DISCHARGE: EXT_RLY 1~4 forced OFF (final guard)\r\n");

    /* (4) 메인 화면으로 복귀 */
    LCD_GoMainAndUpdateBTStatus();

    /* (5) 에러코드 4 (User LCD Control Stop) 1회 TX 후 클리어 */
    g_error_code = ERROR_CODE_USER_LCD_CONTROL_STOP;
    FL_GDS_Send_Error_Code();
    g_error_code = ERROR_CODE_NONE;
    LCD_LOG("[LCD] DISCHARGE: err=4 TX done -> cleared\r\n");

    /* (6) 시스템 안전 상태로 강제 전이 */
    g_system_mode  = eMODE_NONE;
    g_system_state = eSYSTEM_STATE_NONE;
    g_device_state = eDEVICE_STATE_NONE;  /* STOP 동작 완료 → NONE 으로 정리 */
    LCD_LOG("[LCD] DISCHARGE STOP done -> g_device_state = NONE\r\n");

    /* (7) LED 표시 — 별도 호출 불필요. g_device_state 가 START 를 벗어났으므로
     *     led-indicator.c 가 자동으로 LED_G_BLINK(대기)로 전환한다. */

    /* 3분 후 SS1~SS4 OFF (모든 OFF-제어 릴레이 LOW 확인 시) */
    FAN_CooldownArm();
}

/* 나머지 핸들러는 weak 기본 (필요시 재정의) */
__weak void LCD_OnCharge_NotifyNo(void)      {}

/* 충전 화면 Stop 팝업(0x82 APP Error) "확인" → 팝업 숨김 + 메인 화면 복귀 */
void LCD_OnCharge_StopOk(void)
{
    LCD_LOG("[LCD] CHARGE Stop OK (btn=0x2C) -> close popup + go MAIN\r\n");
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_stop_ids); i++)
        LCD_PostShowHide(LCD_SCR_CHARGE, s_popup_stop_ids[i], LCD_OFF);
    LCD_GoMainAndUpdateBTStatus();
}

/* 방전 화면 Stop 팝업(0x82 APP Error) "확인" → 팝업 숨김 + 메인 화면 복귀 */
void LCD_OnDischarge_StopOk(void)
{
    LCD_LOG("[LCD] DISCHARGE Stop OK (btn=0x2C) -> close popup + go MAIN\r\n");
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_stop_ids); i++)
        LCD_PostShowHide(LCD_SCR_DISCHARGE, s_popup_stop_ids[i], LCD_OFF);
    LCD_GoMainAndUpdateBTStatus();
}

void LCD_OnCharge_WarnTempOk(void)
{
    LCD_LOG("[LCD] CHARGE Temp Warn OK (btn=0x31) -> close popup + go MAIN\r\n");
    LCD_ClosePopupAndGoMain(LCD_SCR_CHARGE,
                            s_popup_warn_temp_ids,
                            (uint8_t)sizeof(s_popup_warn_temp_ids));
}

void LCD_OnDischarge_WarnTempOk(void)
{
    LCD_LOG("[LCD] DISCHARGE Temp Warn OK (btn=0x31) -> close popup + go MAIN\r\n");
    LCD_ClosePopupAndGoMain(LCD_SCR_DISCHARGE,
                            s_popup_warn_temp_ids,
                            (uint8_t)sizeof(s_popup_warn_temp_ids));
}

void LCD_OnCharge_CautTempStop(void)
{
    LCD_LOG("[LCD] CHARGE Temp Caution STOP (btn=0x37) -> STOP sequence + go MAIN\r\n");
    FL_GDS_StopDevice_WithoutPopup();
    /* LED 표시 별도 호출 불필요 — 정지 후 자동으로 LED_G_BLINK(대기)로 전환 */
    LCD_ClosePopupAndGoMain(LCD_SCR_CHARGE,
                            s_popup_caut_temp_ids,
                            (uint8_t)sizeof(s_popup_caut_temp_ids));
}

void LCD_OnDischarge_CautTempStop(void)
{
    LCD_LOG("[LCD] DISCHARGE Temp Caution STOP (btn=0x37) -> STOP sequence + go MAIN\r\n");
    FL_GDS_StopDevice_WithoutPopup();
    /* LED 표시 별도 호출 불필요 — 정지 후 자동으로 LED_G_BLINK(대기)로 전환 */
    LCD_ClosePopupAndGoMain(LCD_SCR_DISCHARGE,
                            s_popup_caut_temp_ids,
                            (uint8_t)sizeof(s_popup_caut_temp_ids));
}

/* 충전 화면 구동실패 팝업 "확인" → 팝업 숨김 + 메인 화면 + LED NORMAL(G) */
void LCD_OnCharge_DriveFailOk(void)
{
    LCD_LOG("[LCD] CHARGE DriveFail OK -> close popup + go MAIN + LED NORMAL\r\n");
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_fail_ids); i++)
        LCD_PostShowHide(LCD_SCR_CHARGE, s_popup_fail_ids[i], LCD_OFF);
    LCD_GoMainAndUpdateBTStatus();
    LED_ClearAll();
}

__weak void LCD_OnDischarge_WarnOk(void)     {}
__weak void LCD_OnDischarge_NotifyNo(void)   {}

/* 방전 화면 구동실패 팝업 "확인" → 팝업 숨김 + 메인 화면 + LED NORMAL(G) */
void LCD_OnDischarge_DriveFailOk(void)
{
    LCD_LOG("[LCD] DISCHARGE DriveFail OK -> close popup + go MAIN + LED NORMAL\r\n");
    for (uint8_t i = 0; i < (uint8_t)sizeof(s_popup_fail_ids); i++)
        LCD_PostShowHide(LCD_SCR_DISCHARGE, s_popup_fail_ids[i], LCD_OFF);
    LCD_GoMainAndUpdateBTStatus();
    LED_ClearAll();
}

__weak void LCD_OnMain_Notify2Ok(void)       {}
__weak void LCD_OnMain_Notify3Ok(void)       {}

/**
 * @brief  RX 데이터로 g_lcdState 텍스트 필드 갱신
 */
static void LCD_UpdateStateFromRx(uint8_t screen, uint8_t ctrl_id,
                                   const uint8_t *data, uint8_t data_len)
{
    char *dst = NULL;
    if      (screen == LCD_SCR_MAIN      && ctrl_id ==  1) dst = g_lcdState.scr0_text1;
    else if (screen == LCD_SCR_MAIN      && ctrl_id ==  2) dst = g_lcdState.scr0_text2;
    else if (screen == LCD_SCR_CHARGE    && ctrl_id ==  3) dst = g_lcdState.scr1_text3;
    else if (screen == LCD_SCR_DISCHARGE && ctrl_id ==  3) dst = g_lcdState.scr2_text3;
    else if (screen == LCD_SCR_SETTING   && ctrl_id ==  2) dst = g_lcdState.scr3_text2;
    else if (screen == LCD_SCR_SETTING   && ctrl_id == 14) dst = g_lcdState.scr3_text14;
    else if (screen == LCD_SCR_SETTING   && ctrl_id == 17) dst = g_lcdState.scr3_text17;

    if (dst) {
        memcpy(dst, data, data_len);
        dst[data_len] = '\0';
        LCD_LOG("[LCD RX] state update scr=%d ctrl=%d text=%s\r\n",
               screen, ctrl_id, dst);
    }
}

void LCD_MainScreenStatusUpdate(int conn)
{
    if (conn == 1)
    {
        LCD_PostShowHide(LCD_SCR_MAIN, LCD_MAIN_BT_CONN_CTRL_ID, LCD_ON);   // BT 연결 아이콘 ON
        LCD_PostShowHide(LCD_SCR_MAIN, LCD_MAIN_BT_CONN_WAITING_TEXT, LCD_OFF);  // BT 연결 대기중 메시지 HIDE
        LCD_PostPopupComm(1);                                               // 통신 중 팝업 ON
    }
    else 
    {
        LCD_PostShowHide(LCD_SCR_MAIN, LCD_MAIN_BT_CONN_CTRL_ID, LCD_OFF);  // BT 연결 아이콘 OFF 
        LCD_PostShowHide(LCD_SCR_MAIN, LCD_MAIN_BT_CONN_WAITING_TEXT, LCD_ON);   // BT 연결 대기중 메시지 SHOW
        LCD_PostPopupComm(0);                                               // 통신 중 팝업 OFF        
    }

}
