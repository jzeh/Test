/**
  ******************************************************************************
  * @file    led-indicator.c
  * @brief   SS5(R)/SS6(Y)/SS7(G)/SS8(부저) 상태 표시 — 단일 소유 모듈 (led-indicator.h 참조)
  ******************************************************************************
  */

/* Includes  -----------------------------------------------------------*/
#include "../Inc/led-indicator.h"
#include "../Inc/git-comm.h"   /* COMM_IsBTConnected() — 부작용 없는 캐시된 BLE 연결 여부 */

/* Types -----------------------------------------------------------*/

typedef enum {
    LED_Y_ON = 0,
    LED_Y_BLINK,
    LED_G_ON,
    LED_G_BLINK,
    LED_R_BLINK,
} eLedState;

/* Variables -----------------------------------------------------------*/

static volatile bool     s_criticalError  = false;  /* 과충전/과방전/과온, 센서 WARN — 명시적 해제까지 유지 */
static volatile bool     s_warningLatched = false;  /* err5/err6/0x82/0x14 NAK 등 — 명시적 해제까지 유지 */

/* Buzzer (SS8) — Critical Error 3s */
static volatile bool     s_buzzerActive = false;
static volatile uint32_t s_buzzerSince  = 0;
#define LED_BUZZER_DURATION_MS   3000U

/* Blink (R/Y/G) Phase — 500ms Toggle */
static bool     s_blinkPhaseOn   = false;
static uint32_t s_blinkPhaseTick = 0;
#define LED_BLINK_INTERVAL_MS    750U

static eLedState s_loggedState = LED_Y_ON;
static bool      s_loggedValid = false;

/* Functions -----------------------------------------------------------*/

static bool LED_HasCriticalError(void) { return s_criticalError; }
static bool LED_HasWarning(void)       { return s_warningLatched; }

static void LED_Apply(eLedState state, const char *reason);

static void LED_Setting(void)
{
    if (LED_HasCriticalError())            { LED_Apply(LED_R_BLINK, "R_BLINK (critical error)"); return; }
    if (LED_HasWarning())                  { LED_Apply(LED_Y_BLINK, "Y_BLINK (warning)");         return; }
    if (!COMM_IsBTConnected())             { LED_Apply(LED_Y_ON,    "Y_ON (disconnected)");        return; }
    if (g_device_state == eDEVICE_STATE_START) { LED_Apply(LED_G_ON, "G_ON (running)");            return; }
                                              LED_Apply(LED_G_BLINK, "G_BLINK (idle)");
}

static void LED_Apply(eLedState state, const char *reason)
{
    bool r = false, y = false, g = false;

    switch (state) {
        case LED_R_BLINK: r = s_blinkPhaseOn;                    break;
        case LED_Y_BLINK: y = s_blinkPhaseOn;                    break;
        case LED_Y_ON:     y = true;                             break;
        case LED_G_ON:     g = true;                             break;
        case LED_G_BLINK:
        default:           g = s_blinkPhaseOn;                   break;
    }

    HAL_GPIO_WritePin(SS5_EN_GPIO_Port, SS5_EN_Pin, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SS6_EN_GPIO_Port, SS6_EN_Pin, y ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SS7_EN_GPIO_Port, SS7_EN_Pin, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SS8_EN_GPIO_Port, SS8_EN_Pin, s_buzzerActive ? GPIO_PIN_SET : GPIO_PIN_RESET);

    if (!s_loggedValid || state != s_loggedState) {
        printf("[LED] -> %s\r\n", reason);
        s_loggedState = state;
        s_loggedValid = true;
    }
}

void LED_Init(void)
{
    s_criticalError  = false;
    s_warningLatched = false;
    s_buzzerActive   = false;
    s_buzzerSince    = 0;
    s_blinkPhaseOn   = false;
    s_blinkPhaseTick = HAL_GetTick();
    s_loggedValid    = false;
    LED_Setting();
}

void LED_Tick(void)
{
    if ((HAL_GetTick() - s_blinkPhaseTick) >= LED_BLINK_INTERVAL_MS) {
        s_blinkPhaseTick = HAL_GetTick();
        s_blinkPhaseOn   = !s_blinkPhaseOn;
    }

    if (s_buzzerActive && (HAL_GetTick() - s_buzzerSince) >= LED_BUZZER_DURATION_MS) {
        s_buzzerActive = false;
    }

    LED_Setting();   
}

void LED_RaiseCriticalError(void)
{
    s_criticalError = true;
    s_buzzerActive  = true;
    s_buzzerSince   = HAL_GetTick();
    LED_Setting();
}

void LED_ClearCriticalError(void)
{
    s_criticalError = false;
    s_buzzerActive  = false;
    LED_Setting();
}

void LED_RaiseWarning(void)
{
    s_warningLatched = true;
    LED_Setting();
}

void LED_ClearAll(void)
{
    s_criticalError  = false;
    s_warningLatched = false;
    s_buzzerActive   = false;
    LED_Setting();
}

bool LED_HasActiveAlarm(void)
{
    return (s_criticalError || s_warningLatched);
}
