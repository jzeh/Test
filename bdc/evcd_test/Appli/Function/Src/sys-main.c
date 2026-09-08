/**
 * ******************************************************************************
 * @file    sys-main.c
 * @brief   System State Machine / Mode Controller
 *
 *  State Transition Diagram:
 *
 *    NONE ──→ INIT ──→ IDLE ──→ RUNNING ──→ ERROR
 *                        ↑          │          │
 *                        └──────────┘          │
 *                        └─────────────────────┘
 *
 *  Task Trigger:
 *    eMODE_VEHICLE_CHARGE    → HwControl (Relay + CP PWM)
 *    eMODE_VEHICLE_DISCHARGE → HwControl (Relay) + PLC Task (FDCAN2)
 *    eMODE_BSA_DISCHARGE     → HwControl (Relay) + MOSA Task (FDCAN1)
 *    eMODE_SETTING           → BLE DB Parsing active
 *
 * ******************************************************************************
 */

/* Includes  -----------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/sys-emmc.h"    
#include "../Inc/git-functionlist.h"
#include "../Inc/task-plc.h"
#include "../Inc/task-lcd.h"
#include "../Inc/git-ble.h"
#include "../Inc/task-hwcontrol.h"
#include "../Inc/task-bsa.h"
#include "../Inc/led-indicator.h"
#include "fw_update.h"

/* Define    -----------------------------------------------------------*/
#define PLC_LOG(fmt, ...)  printf("\033[32m" fmt "\033[0m", ##__VA_ARGS__)

/* Variables -----------------------------------------------------------*/
SystemState_t g_system_state = eSYSTEM_STATE_NONE;
SystemMode_t  g_system_mode  = eMODE_NONE;
osEventFlagsId_t g_taskEventFlags = NULL;
DeviceState_t g_device_state = eDEVICE_STATE_NONE;  
stDeviceTime  g_device_time  = {0};                 

volatile uint32_t g_run_time_sec       = 0;
volatile bool     g_run_time_active    = false;
static   uint32_t s_run_time_last_tick = 0;

/**
 * @brief  Operation Timer → "HH:MM:SS" string conversion
 * @param  seconds  Operation time (seconds)
 * @param  out      Output buffer (minimum 9 bytes)
 * @param  out_size Buffer size
 */
void Format_RunTime_HMS(uint32_t seconds, char *out, size_t out_size)
{
  if (!out || out_size < 9) return;
  uint32_t h = seconds / 3600U;
  uint32_t m = (seconds / 60U) % 60U;
  uint32_t s = seconds % 60U;
  snprintf(out, out_size, "%02lu:%02lu:%02lu",
           (unsigned long)h, (unsigned long)m, (unsigned long)s);
}


/* Functions -----------------------------------------------------------*/
extern void CAN_enable(int ch, bool enable);

void SYS_init(void)
{
  // BLE init
  // BT_Initialize();

  // CAN Enable
  CAN_enable(1, true);
  CAN_enable(2, true);

  /* eMMC + FATFS mount — OTA 다운로드(fw_update_*) 에 필수.
   * EMMC_PWR_EN 핀 / RST 시퀀스는 EMMC_Enable() (task-hwcontrol.c) 가 처리.
   * 만약 HW 미연결 / 마운트 실패 시 InitEMMC 가 음수 반환 — 무시하고 계속 (OTA 만 비활성). */
  int32_t emmc_rc = InitEMMC();
  if (emmc_rc != 0) {
    printf("[SYS] InitEMMC failed (rc=%ld) — OTA disabled\r\n", (long)emmc_rc);
  } else {
    printf("[SYS] eMMC mounted OK\r\n");
  }

  if (BTGetConnectStatus()) {
    BTMarkDeviceNameDirty();
  } else {
    BTSetLocalDeviceNameReq();
  }
}

void state_machine(void)
{
  switch (g_system_state)
  {
    case eSYSTEM_STATE_NONE:
      g_system_state = eSYSTEM_STATE_INIT;
      printf("[STATE] NONE -> INIT\r\n");
      break;

    case eSYSTEM_STATE_INIT:
      g_system_state = eSYSTEM_STATE_IDLE;
      printf("[STATE] INIT -> IDLE\r\n");

      /* fw_update Step 7: 새 펌웨어 자가 검증.
       * BOOT_TEST 진입 후 IDLE 정상 도달 → VERIFIED + fail_count 리셋.
       * IDLE 도달 전 hang/crash 시: IWDG 리셋 → 다음 부팅에 Boot 이 fail_count 증가
       * → RECOVERY_MAX_FAIL_COUNT(3) 도달 시 cascade 발동 (Step 8). */
      if (fw_get_flag() == FW_FLAG_BOOT_TEST)
      {
        fw_set_flag(FW_FLAG_VERIFIED);
        fw_set_fail_count(0);
        printf("[FW] BOOT_TEST -> VERIFIED (self-test OK)\r\n");

        /* Rollback (Stage 2.5) 직후라면 ini swap 수행 — best-effort */
        if (fw_get_recovery_phase() == RECOVERY_PHASE_ROLLBACK)
        {
          if (fw_update_swap_ini_from_backup() == HAL_OK) {
            printf("[FW] ini swapped from 02_Backup\r\n");
          }
          fw_set_recovery_phase(RECOVERY_PHASE_NONE);
        }
      }
      break;

    case eSYSTEM_STATE_IDLE:
      if (g_system_mode != eMODE_NONE && g_system_mode != eMODE_SETTING)
      {
        g_system_state = eSYSTEM_STATE_RUNNING;
        printf("[STATE] IDLE -> RUNNING (mode=%d)\r\n", g_system_mode);

        uint32_t flags = EVT_TASK_SENSOR | EVT_TASK_HWCON;
        if (g_system_mode == eMODE_BSA_DISCHARGE)
          flags |= EVT_TASK_BSA;
        else if (g_system_mode == eMODE_VEHICLE_DISCHARGE)
          flags |= EVT_TASK_PLC;
        osEventFlagsSet(g_taskEventFlags, flags);
      }
      break;

    case eSYSTEM_STATE_RUNNING:
      // - VEHICLE_CHARGE    : HwControl active
      // - VEHICLE_DISCHARGE : HwControl + PLC Task active
      // - BSA_DISCHARGE     : HwControl + MOSA Task active

      if (g_system_mode == eMODE_NONE)
      {
        g_system_state = eSYSTEM_STATE_IDLE;
        printf("[STATE] RUNNING -> IDLE (mode reset)\r\n");
      }

      /* ── Auto Stop - Target SOC Reached (disabled) ─── */
#if 0
      {
        bool soc_target_reached = false;
        uint16_t target = 0;
        const char *cmp = "";

        if (g_system_mode == eMODE_VEHICLE_CHARGE) {
          target = g_dbConfig.vehicle_fullcharge_SOC;
          cmp = ">=";
          if (g_vci3_soc > 0 && target > 0 && g_vci3_soc >= target)
            soc_target_reached = true;
        }
        else if (g_system_mode == eMODE_VEHICLE_DISCHARGE) {
          target = g_dbConfig.vehicle_fulldischarge_SOC;
          cmp = "<=";
          if (g_vci3_soc > 0 && target > 0 && g_vci3_soc <= target)
            soc_target_reached = true;
        }
        else if (g_system_mode == eMODE_BSA_DISCHARGE) {
          target = g_dbConfig.bsa_fulldischarge_SOC;
          cmp = "<=";
          if (g_vci3_soc > 0 && target > 0 && g_vci3_soc <= target)
            soc_target_reached = true;
        }

        if (soc_target_reached) {
          printf("[STATE] Target SOC reached (current=%d%% %s target=%d%%) -> ALL OFF\r\n",
                 g_vci3_soc, cmp, target);
          IO_ALL_OFF_control();
          g_system_mode  = eMODE_NONE;
          g_system_state = eSYSTEM_STATE_IDLE;
          g_device_state = eDEVICE_STATE_NONE;
          printf("[STATE] RUNNING -> IDLE (SOC target reached) + mode/device reset\r\n");
        }
      }
#endif
      break;

    case eSYSTEM_STATE_ERROR:
      printf("[STATE] ERROR state - safety measures active\r\n");
      break;
  }

  /* LCD Auto Screen Transition */
  static DeviceState_t s_lcd_prev_device = eDEVICE_STATE_NONE;

  /* edge 체크: 이전 != START && 현재 == START 일 때만 1회
   *  중간 단계로 MAIN(scr=0) 한 번 거친 후 모드 화면으로 이동
   *  → 이전 스크린(이전 모드/팝업)에서 깨끗하게 리셋 후 진입 */
  if (s_lcd_prev_device != eDEVICE_STATE_START &&
      g_device_state    == eDEVICE_STATE_START)
  {
    /* restart 시 LED NORMAL(G) — 이전 STOP/Disconnect 표시 클리어 */
    {
      LED_ClearAll();
      printf("[STATE] device START -> LED NORMAL (sticky cleared)\r\n");
    }

    /* 운용 시간 타이머 시작 — 0x91 START 시점부터 카운트
     *  · LCD 모드별 화면 전환은 0x11 SetMode 핸들러에서 이미 수행됨
     *  · 여기서는 타이머 초기화 + "00:00:00" 텍스트 동기화만 수행 */
    g_run_time_sec       = 0;
    s_run_time_last_tick = HAL_GetTick();
    g_run_time_active    = true;
    printf("[STATE] device START -> run-time timer started\r\n");

    if (g_system_mode == eMODE_VEHICLE_CHARGE) {
      LCD_PostTextSet(LCD_SCR_CHARGE, 8, "00:00:00");
    } else if (g_system_mode == eMODE_VEHICLE_DISCHARGE ||
               g_system_mode == eMODE_BSA_DISCHARGE) {
      LCD_PostTextSet(LCD_SCR_DISCHARGE, 18, "00:00:00");
    }
  }
  s_lcd_prev_device = g_device_state;

  /* ── BSA Power Connector 모니터 ────────────────────────────────
   *  BSA discharge 모드 START 후 5초 대기 → 그 후 5초 동안 100ms 주기 검사:
   *    · V > 0 한 번이라도 들어오면 → 정상으로 간주, 검사 종료
   *    · 5초 윈도우 내내 V == 0 (50회 연속) → Power Connector 미연결로 판단:
   *        - g_error_code = ERROR_CODE_BSA_POWER_CONNECTOR_FAIL
   *        - FL_GDS_Send_Error_Code() 로 0x81 1회 송신 후 클리어
   *  · 결과 결정 (정상/fail) 후엔 BSA STOP 까지 재검사 안 함
   *────────────────────────────────────────────────────────────────*/
  {
    #define BSA_PWR_CHECK_DELAY_MS   5000U   /* START 후 5초 대기 */
    #define BSA_PWR_CHECK_INTERVAL   100U    /* 100ms 주기 */
    #define BSA_PWR_ZERO_COUNT_MAX   50U     /* 50회 연속 0 → fail (=5000ms) */

    static uint32_t s_bsaPwrStartTick    = 0;     /* 0 = not armed */
    static uint32_t s_bsaPwrLastChkTick  = 0;
    static uint16_t s_bsaPwrZeroCnt      = 0;     /* 50회 카운트 위해 16-bit */
    static bool     s_bsaPwrCheckDone    = false; /* true = 정상/fail 결정 완료 */

    bool bsa_running = (g_system_mode  == eMODE_BSA_DISCHARGE) &&
                       (g_device_state == eDEVICE_STATE_START);

    if (bsa_running) {
      /* BSA START edge: 모니터 arm */
      if (s_bsaPwrStartTick == 0) {
        s_bsaPwrStartTick   = HAL_GetTick();
        s_bsaPwrLastChkTick = 0;
        s_bsaPwrZeroCnt     = 0;
        s_bsaPwrCheckDone   = false;
        printf("[BSA Monitor] Power connector check armed (start in 5s, window 5s)\r\n");
      }

      /* 5초 경과 후 100ms 주기 검사 (정상 또는 fail 결정되면 종료) */
      if (!s_bsaPwrCheckDone &&
          (HAL_GetTick() - s_bsaPwrStartTick) >= BSA_PWR_CHECK_DELAY_MS &&
          (HAL_GetTick() - s_bsaPwrLastChkTick) >= BSA_PWR_CHECK_INTERVAL)
      {
        s_bsaPwrLastChkTick = HAL_GetTick();

        if (g_discharge_voltage_V != 0.0f) {
          /* V > 0 한 번 감지되면 정상 → 검사 종료 */
          s_bsaPwrCheckDone = true;
          printf("[BSA Monitor] Power connector OK (V=%.1f at try %u) -> check done\r\n",
                 g_discharge_voltage_V, (unsigned)(s_bsaPwrZeroCnt + 1));
        }
        else {
          s_bsaPwrZeroCnt++;
          if (s_bsaPwrZeroCnt >= BSA_PWR_ZERO_COUNT_MAX) {
            /* 50회 연속 0 (5초 윈도우 내내 0) → fail */
            g_error_code = ERROR_CODE_BSA_POWER_CONNECTOR_FAIL;
            FL_GDS_Send_Error_Code();
            g_error_code = ERROR_CODE_NONE;
            s_bsaPwrCheckDone = true;
            printf("[BSA Monitor] Power connector FAIL (V=0 for %ums, %u tries) -> err=%d TX (0x81)\r\n",
                   (unsigned)(BSA_PWR_ZERO_COUNT_MAX * BSA_PWR_CHECK_INTERVAL),
                   (unsigned)BSA_PWR_ZERO_COUNT_MAX,
                   ERROR_CODE_BSA_POWER_CONNECTOR_FAIL);
          }
        }
      }
    } else {
      /* BSA running 아니면 모니터 상태 전체 리셋 (다음 진입 시 재검사) */
      s_bsaPwrStartTick   = 0;
      s_bsaPwrLastChkTick = 0;
      s_bsaPwrZeroCnt     = 0;
      s_bsaPwrCheckDone   = false;
    }

    #undef BSA_PWR_CHECK_DELAY_MS
    #undef BSA_PWR_CHECK_INTERVAL
    #undef BSA_PWR_ZERO_COUNT_MAX
  }

  /* err4(USER_LCD_CONTROL_STOP) LED Y 3초 cooldown 타이머는
   * led-indicator.c의 LED_Tick()으로 이동 (main.c 메인 루프에서 호출) */

  /* ── Running Timer  — 1s Increase + LCD 동기화 ──────────────
   *  start -> g_run_time_sec=0, g_run_time_active=true, last_tick=현재
   *  매 polling 마다 1초 경과 시 g_run_time_sec++ -> LCD Timer Update
   *  device_state != START 면 자동 정지 (값은 보존, 다음 START 에서 0 리셋). */
  {
    if (g_device_state != eDEVICE_STATE_START) {
      g_run_time_active = false;   // auto stop, reset 0 in next start
    }
    if (g_run_time_active) {
      uint32_t now = HAL_GetTick();
      bool changed = false;
      while ((int32_t)(now - s_run_time_last_tick) >= 1000) {
        g_run_time_sec++;
        s_run_time_last_tick += 1000U;
        changed = true;
      }

      // Update LCD when timer 1s update 
      if (changed) {
        char buf[12];
        Format_RunTime_HMS(g_run_time_sec, buf, sizeof(buf));
        if (g_system_mode == eMODE_VEHICLE_CHARGE) {
          LCD_PostTextSet(LCD_SCR_CHARGE, 8, buf);
        } else if (g_system_mode == eMODE_VEHICLE_DISCHARGE ||
                   g_system_mode == eMODE_BSA_DISCHARGE) {
          LCD_PostTextSet(LCD_SCR_DISCHARGE, 18, buf);
        }
      }
    }
  }

  /* FAN Cooldown */
  FAN_CooldownProcess();
}

/**
 * @brief  Mode Control
 *         Called from FL_GDS_SetMode() via BLE
 *         Sends signals/flags to relevant tasks upon mode change
 */
void mode_control(SystemMode_t mode)
{
  SystemMode_t prev_mode = g_system_mode;
  g_system_mode = mode;

  printf("[MODE] %d -> %d\r\n", prev_mode, mode);

  if (g_system_state >= eSYSTEM_STATE_RUNNING)
  {
    uint32_t flags = 0;
    switch (mode)
    {
      case eMODE_NONE:
        break;

      case eMODE_SETTING:
        break;

      case eMODE_VEHICLE_CHARGE:
        flags = EVT_TASK_SENSOR | EVT_TASK_HWCON;
        break;

      case eMODE_VEHICLE_DISCHARGE:
        flags = EVT_TASK_SENSOR | EVT_TASK_HWCON | EVT_TASK_PLC;
        break;

      case eMODE_BSA_DISCHARGE:
        flags = EVT_TASK_SENSOR | EVT_TASK_HWCON | EVT_TASK_BSA;
        break;
    }
    if (flags) osEventFlagsSet(g_taskEventFlags, flags);
  }
}

/**
 * @brief  장비 정지 공통 시퀀스 — 0x91 STOP, 0x82 APP Error Receive 등에서 재사용
 *  · IO_ALL_OFF + 모드별 정지 (CHARGE/DISCHARGE/BSA) + 시스템 상태 NONE 정리 + FAN cooldown arm
 *  · 응답 송출은 호출자가 담당 (FuncID 가 다를 수 있음)
 *  @param  popup_mode  NONE=팝업 없음, WORK_DONE=작업완료, APP_ERROR=Stop 팝업
 *  (git-functionlist.c 에서 이동 — 동작 동일)
 */
void BDC_StopSequence(eBDC_StopPopupMode popup_mode)
{
    SENSOR_ErrorCheck_Disarm();   /* 정지 직후 센서 에러 체크 중단 */
    g_bDisplayActive = false;     /* 정지 직후 주기 상태 송신(0x51) 중단 */
    IO_ALL_OFF_control();
    g_device_state    = eDEVICE_STATE_STOP;
    g_plcStepWaitStop = true;
    PLC_LOG("[STOP] sequence start (mode=%d, popup=%d)\r\n",
            (int)g_system_mode, (int)popup_mode);

    if (g_system_mode == eMODE_VEHICLE_CHARGE)
    {
        RELAY_process(eMODE_NONE);
        PLC_LOG("[STOP] VEHICLE_CHARGE: CP stop, relay OFF\r\n");

        if (popup_mode != BDC_STOP_POPUP_NONE) {
            LCD_PostScreenGoto(LCD_SCR_CHARGE);
            if (popup_mode == BDC_STOP_POPUP_APP_ERROR) {
                LCD_PostPopupStop(LCD_SCR_CHARGE);
                PLC_LOG("[STOP] VEHICLE_CHARGE: goto scr=1 + Stop popup\r\n");
            } else {
                LCD_PostPopupWorkDone(LCD_SCR_CHARGE);
                PLC_LOG("[STOP] VEHICLE_CHARGE: goto scr=1 + WorkDone popup\r\n");
            }
        }
    }
    else if (g_system_mode == eMODE_VEHICLE_DISCHARGE)
    {
        PLC_ApplyStopSteps();
        PLC_LOG("[STOP] VEHICLE_DISCHARGE: stop-step (step=99) applied\r\n");

        PLC_SetTxValue(PLC_REQ_ChargeControl, PLC_MSGDisp_ChargingControl_NormalStop);
        PLC_LOG("[STOP] VEHICLE_DISCHARGE: NormalStop sent to SECC\r\n");
        osDelay(200);

        PLC_LOG("[STOP] EXT_RLY OFF sequence\r\n");
        for (int rly = 4; rly >= 1; rly--) {
            IO_EXT_RLY_control(rly, false);
            PLC_LOG("[STOP] EXT_RLY%d OFF\r\n", rly);
            if (rly > 1) osDelay(100);
        }
        g_plcManualRun = false;

        if (popup_mode != BDC_STOP_POPUP_NONE) {
            LCD_PostScreenGoto(LCD_SCR_DISCHARGE);
            if (popup_mode == BDC_STOP_POPUP_APP_ERROR) {
                LCD_PostPopupStop(LCD_SCR_DISCHARGE);
                PLC_LOG("[STOP] VEHICLE_DISCHARGE: goto scr=2 + Stop popup\r\n");
            } else {
                LCD_PostPopupWorkDone(LCD_SCR_DISCHARGE);
                PLC_LOG("[STOP] VEHICLE_DISCHARGE: goto scr=2 + WorkDone popup\r\n");
            }
        }
    }
    else if (g_system_mode == eMODE_BSA_DISCHARGE)
    {
        BSA_Stop();
        if (popup_mode != BDC_STOP_POPUP_NONE) {
            LCD_PostScreenGoto(LCD_SCR_DISCHARGE);
            if (popup_mode == BDC_STOP_POPUP_APP_ERROR) {
                LCD_PostPopupStop(LCD_SCR_DISCHARGE);
                PLC_LOG("[STOP] BSA_DISCHARGE: BSA_Stop + goto scr=2 + Stop popup\r\n");
            } else {
                LCD_PostPopupWorkDone(LCD_SCR_DISCHARGE);
                PLC_LOG("[STOP] BSA_DISCHARGE: BSA_Stop + goto scr=2 + WorkDone popup\r\n");
            }
        }
    }

    /* EXT_RLY 1~4 명시적 OFF — IO_ALL_OFF + BSA_Stop 후의 안전장치
     *  · BSA 모드에서 HW task self-healing 이 IO_ALL_OFF 중 EXT_RLY1 을 다시 ON 시키는
     *    race 가 있어, BSA_Stop (self-healing 비활성화) 이후에 한번 더 OFF 보장 */
    {
        for (int rly = 1; rly <= 4; rly++) {
            IO_EXT_RLY_control(rly, false);
        }
        PLC_LOG("[STOP] EXT_RLY 1~4 forced OFF (final guard)\r\n");
    }

    g_system_mode  = eMODE_NONE;
    g_system_state = eSYSTEM_STATE_NONE;
    g_device_state = eDEVICE_STATE_NONE;
    PLC_LOG("[STOP] sequence done -> mode/state/device = NONE\r\n");

    /* 5분 후 SS1~SS4 OFF (모든 OFF-제어 릴레이 LOW 확인 시) */
    FAN_CooldownArm();
}

/*
  < INIT 루틴 >
    1. 부팅 시 진행
    - PERIPH 초기화 (기본 외에 따로 핀 설정 필요한 거)
    - BNCOM CONFIG

    2. 조건부 진행    (모드 진입/동작 시작 시)
    - CP PWM 제어 / RELAY 제어
*/

/*
  < 배터리팩 방전 >
  모드 시작 명령
    1. Response TX (ACK/NAK)
    2. 전압 / 전류 / 온도 모니터링
    3. DC Rly ON  (배터리팩에서도 쓰는건지 다시 확인)
    4. 차량 모사 (Battery Relay Con Thread + Battery Relay Monitor Thread 동작)
    5. 500ms 주기 상태 정보 전달 (진행, 전류/전압/장비 온도/SOC 등)

    모드 종료 명령
    1. Response TX (ACK/NAK)
    2. DC Rly OFF
    3. 상태 정보 전달 (종료) -- ACK/NAK 랑 같이 나가는지는 협의 필요

*/

/*======================================================================*/
/*  Timer Helper Functions (EVCD-specific implementations)             */
/*  (moved from task-bsa.c — declared in sys-common.h)  */
/*======================================================================*/

/**
 * @brief  Get current timer value in milliseconds
 * @retval Current millisecond tick count
 */
uint32_t Get_Tmr(void)
{
    return osKernelGetTickCount();  // CMSIS v2 RTOS tick count (1ms per tick)
}

/**
 * @brief  Calculate delta between two timer values (handles overflow)
 * @param  ulNew: New timer value
 * @param  ulOld: Old timer value
 * @retval Time difference in milliseconds
 */
uint32_t Get_TmrDelta(uint32_t ulNew, uint32_t ulOld)
{
    if (ulNew >= ulOld) {
        return ulNew - ulOld;
    }
    else {
        // Handle overflow (32-bit timer wraps around)
        return (0xFFFFFFFF - ulOld) + ulNew + 1;
    }
}

/**
 * @brief  Get Unix timestamp (seconds since epoch)
 * @note   This is a placeholder - implement with RTC if available
 * @retval Unix timestamp (or tick count / 1000 as fallback)
 */
uint32_t GetUnixTime(void)
{
    // TODO: Implement with RTC module when available
    // For now, return tick count converted to seconds as placeholder
    return osKernelGetTickCount() / 1000;
}
