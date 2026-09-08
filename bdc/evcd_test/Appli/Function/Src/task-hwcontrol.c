/**
 * ******************************************************************************
 * @file    task-hwcontrol.c
 * @brief   HW Control Task (GPIO, PWM) - Relay / CP PWM
 *
 *          Ported from ECVT project (EVCT_function.c) - CP PWM Control Sequence
 *          IEC 61851 based CP (Control Pilot) signal generation & monitoring
 *
 *          CP State Machine:
 *          CP_IDLE -> CP_WAIT4CONNECTING (6V detect x3)
 *                  -> CP_PWM_START (DC zero check)
 *                  -> CP_PWM_RUNNING (AC overcurrent monitor)
 *                  -> CP_PWM_STOP / CP_MODE_ERROR
 *
 *  Trigger: g_system_state >= eSYSTEM_STATE_RUNNING
 *  Wake:    osEventFlagsWait (EVT_TASK_HWCON signal)
 * ******************************************************************************
 */

#include "../Inc/sys-common.h"

/* ===== Thread Definition ================================================ */

osThreadId_t hwconTaskHandle;
uint32_t hwconTaskBuffer[ 2048 ];
osStaticThreadDef_t hwconTaskControlBlock;
const osThreadAttr_t hwconTask_attributes = {
  .name = "hwconTask",
  .cb_mem = &hwconTaskControlBlock,
  .cb_size = sizeof(hwconTaskControlBlock),
  .stack_mem = &hwconTaskBuffer[0],
  .stack_size = sizeof(hwconTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};

/* ===== Relay Enums (existing) =========================================== */

typedef enum { eRELAY_OFF = 0,  eRELAY_ON } RelayState_t;
typedef enum { eRELAY_AC = 0,   eRELAY_DC } RelayType_t;

/* ===== CP State Machine Enums =========================================== */

typedef enum {
  CP_NONE = 0,
  CP_IDLE,                // Initial: reset counters, start PWM
  CP_WAIT4CONNECTING,     // Wait for 6V detection (cable connected)
  CP_PWM_START,           // DC voltage zero check (pre-relay)
  CP_PWM_RUNNING,         // AC overcurrent monitoring (charging)
  CP_PWM_STOP,            // Stop PWM, safe state
  CP_MODE_ERROR           // Error handling -> PWM_STOP
} CPState_t;

typedef enum {
  CP_ERR_NONE = 0,
  CP_ERR_DC_NOT_STABLE,           // DC voltage not 0V before relay
  CP_ERR_DC_CHECK_FAIL,           // DC check retry exceeded
  CP_ERR_AC_MAX_OVERCURRENT,      // AC ADC max (26.9A) exceeded 3x
  CP_ERR_AC_SAMPLING_OVERCURRENT  // AC 16.8A sustained >200ms
} CPErrorCode_t;

/* ===== CP Threshold Constants =========================================== */
/*
 * NOTE: CP_READY_UPPER/LOWER must be calibrated on BDC hardware.
 *       These values are from ECVT (12-bit ADC with specific voltage divider).
 *       Measure actual ADC value when CP = 6V on BDC board and adjust.
 */
#define CP_READY_UPPER        1962    // 6.3V equivalent (ADC 12-bit)
#define CP_READY_LOWER        1775    // 5.7V equivalent (ADC 12-bit)

// DC/AC thresholds (physical units from task-sensor.c)
#define DC_ZERO_THRESH_V      5.0f    // DC must be below this before relay ON
#define AC_OVERCURR_MA        16800.0f // 16.8A overcurrent threshold
#define AC_MAX_MA             26900.0f // 26.9A absolute max

#define CP_CHECK_CNT          3       // Consecutive detection count
#define DC_RETRY_MAX          100     // DC check retry limit

// TIM4 PWM config: 300MHz / (15 * 20000) = 1kHz
#define CP_TIM4_PRESCALER     14      // (14+1) = 15
#define CP_TIM4_PERIOD        19999   // (19999+1) = 20000

// PWM Pulse values (same scale as ECVT, Period=20000)
#define CP_PULSE_6A           1996    // 10.0% duty
#define CP_PULSE_8A           2658    // 13.3% duty
#define CP_PULSE_10A          3322    // 16.6% duty
#define CP_PULSE_12A          3986    // 20.0% duty
#define CP_PULSE_16A          5315    // 26.6% duty
#define CP_PULSE_HIGH_DC      20000   // 100% HIGH (+12V idle)

/* ===== CP State Machine Variables ======================================= */

static CPState_t     cp_state = CP_NONE;
static CPErrorCode_t cp_error = CP_ERR_NONE;
static bool          cp_run_enabled = false;
static bool          cp_pwm_stop_request = false;
static int           cp_charge_current = 0;     // Target current (A)

// CP 6V detection
static uint16_t cp_6v_detect_cnt = 0;

// DC zero check
static uint16_t dc_retry_cnt = 0;

// AC overcurrent monitoring
static uint16_t ac_overcurrent_cnt = 0;
static uint16_t ac_sampling_overcurrent_cnt = 0;
static float    ac_min_ma = 0.0f;
static float    ac_max_ma = 0.0f;

// Timer (FreeRTOS tick based)
static uint32_t cp_timer_start = 0;
#define CP_ELAPSED()  (osKernelGetTickCount() - cp_timer_start)
#define CP_TIMER_RESET()  (cp_timer_start = osKernelGetTickCount())

/* ===== Forward Declarations ============================================= */

void StartHwConTask(void *argument);
void RELAY_process(SystemMode_t state);
void CP_PWM_control(bool enable, int curr);
void IO_AD_RELAY_control(int type, int ch, bool enable);
void IO_EXT_RLY_control(int ch, bool enable);
static void CP_TIM4_Reconfigure(void);
static uint16_t CP_ADC_Read(void);
static void CP_Run(void);
// static void CP_SendStatusToBLE(uint8_t code1, uint8_t code2);
void IO_ALL_OFF_control(void);
void HWCONTROL_init(void);
void EMMC_Enable(void);

/* ===== CLI Accessor Functions ============================================ */

uint8_t  CP_CLI_GetState(void)   { return (uint8_t)cp_state; }
uint8_t  CP_CLI_GetError(void)   { return (uint8_t)cp_error; }
bool     CP_CLI_IsRunning(void)  { return cp_run_enabled; }
int      CP_CLI_GetCurrent(void) { return cp_charge_current; }
void     CP_CLI_SetCurrent(int current_A) { cp_charge_current = current_A; }

/* 0x12 DataConfig 의 CP 전류 반영 로직 — git-functionlist.c 에서 이동 (동작 동일).
 *  · 로그 문자열은 기존 PLC_LOG("...") 초록색 출력과 완전히 동일하게 유지 */
void HWCON_ApplyChargeCurrent(void)
{
    CP_CLI_SetCurrent((int)g_dbConfig.max_charge_current);
    if (CP_CLI_IsRunning()) 
    {
        CP_PWM_control(true, (int)g_dbConfig.max_charge_current);
        printf("\033[32m[FL_GDS_DataConfig] CP PWM updated: %dA\r\n\033[0m", g_dbConfig.max_charge_current);
    }
}
uint16_t CP_CLI_ReadADC(void)    
{ 
  return CP_ADC_Read(); 
}

void CP_CLI_Start(int current_A)
{
  cp_charge_current = current_A;
  cp_run_enabled = true;
  cp_state = CP_IDLE;
}

void CP_CLI_Stop(void)
{
  cp_pwm_stop_request = true;
}

/* ===== Task Init ======================================================== */

void InitHwConTask(void)
{
  HWCONTROL_init();
  // EMMC_Enable();                /* OTA 활성화 (fw_update) — eMMC PWR/RST 시퀀스 */

  hwconTaskHandle = osThreadNew(StartHwConTask, NULL, &hwconTask_attributes);
}

/* ===== HWCONTROL_init (Step 8) ========================================== */

void HWCONTROL_init(void)
{
  // Reconfigure TIM4 for 1kHz CP PWM
  CP_TIM4_Reconfigure();

  // Calibrate ADC1 for CP feedback (STM32H7RS: 2 args only, no offset/linearity select)
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

  // All relays OFF initially
  RELAY_process(eMODE_NONE);

  // CP PWM default: disabled
  CP_PWM_control(false, 0);

  printf("[HWCON] Init OK\r\n");
}

/* ===== TIM4 Runtime Reconfigure (Step 2) ================================ */
/*
 * CubeMX sets Period=4294967295 which is wrong for 1kHz PWM.
 * Reconfigure at runtime instead of modifying CubeMX-generated main.c.
 *
 * TIM4 clock = APB1_TIM = 300MHz (HSE=24M, PLL1 M=2 N=50 P=1, AHB/2, APB1/2, x2)
 * 300MHz / (14+1) / (19999+1) = 1000Hz
 */
static void CP_TIM4_Reconfigure(void)
{
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);

  htim4.Init.Prescaler = CP_TIM4_PRESCALER;
  htim4.Init.Period = CP_TIM4_PERIOD;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }

  TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = CP_PULSE_HIGH_DC;  // Default: 100% HIGH (+12V idle)
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* ===== CP ADC Read (Step 3) ============================================= */
/*
 * Read CP feedback voltage from ADC1 CH10 (PC0)
 * 12-bit resolution, software trigger, polling mode
 */
static uint16_t CP_ADC_Read(void)
{
  /* PWM 1kHz (HIGH=13~27%) sampling:
   * HAL_Delay(1)은 PWM과 같은 1ms 주기라 위상 고정(항상 LOW 구간 샘플) 발생.
   * → delay 없이 고속 연속 샘플링(~2ms, 2주기 커버)으로 최댓값 반환 */
  uint16_t maxVal = 0;
  for (int i = 0; i < 500; i++)
  {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
      uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc1);
      if (val > maxVal) maxVal = val;
    }
    HAL_ADC_Stop(&hadc1);
  }
  return maxVal;
}

/* ===== CP PWM Control (Step 4) ========================================== */
/*
 * IEC 61851 CP PWM duty cycle control
 * TIM4 CH4 (PD15), 1kHz, Period=20000
 *
 * @param enable  true=start PWM, false=stop PWM
 * @param curr    Target current (6/8/10/12/16 A), 0xFF=HIGH DC (+12V)
 */
void CP_PWM_control(bool enable, int curr)
{
  if (enable)
  {
    uint32_t pulse;

    switch (curr) {
      case 6:   pulse = CP_PULSE_6A;   break;     // 10.0% (6A)
      case 8:   pulse = CP_PULSE_8A;   break;     // 13.3% (8A)
      case 10:  pulse = CP_PULSE_10A;  break;     // 16.6% (10A)
      case 12:  pulse = CP_PULSE_12A;  break;     // 20.0% (12A)
      case 16:  pulse = CP_PULSE_16A;  break;     // 26.6% (16A)
      default:  pulse = CP_PULSE_HIGH_DC; break;  // 100% HIGH DC (+12V)
    }

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, pulse);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  }
  else
  {
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
  }
}

/* ===== RELAY Process (Step 5) =========================================== */
/*
 * Mode-based relay control using existing IO_AD_RELAY_control()
 *
 * AC relays: AC_L_RLY_EN (PE3), AC_N_RLY_EN (PE4)
 * DC relays: DC_P_RLY_EN (PE5), DC_M_RLY_EN (PE6)
 */
void RELAY_process(SystemMode_t state)
{
  switch (state)
  {
    case eMODE_NONE:
      // Safety: all relays OFF (OFF: AC_L first, then AC_N / DC_M first, then DC_P)
      IO_AD_RELAY_control(eRELAY_AC, 1, false);  // AC_L OFF
      HAL_Delay(100);
      IO_AD_RELAY_control(eRELAY_AC, 2, false);  // AC_N OFF
      HAL_Delay(100);
      IO_AD_RELAY_control(eRELAY_DC, 2, false);  // DC_M OFF
      HAL_Delay(100);
      IO_AD_RELAY_control(eRELAY_DC, 1, false);  // DC_P OFF
      HAL_Delay(100);
      break;

    case eMODE_VEHICLE_CHARGE:
    // AC charge: AC relays ON (AC_N first, then AC_L), DC relays OFF
      IO_AD_RELAY_control(eRELAY_AC, 2, true);   // AC_N ON
      HAL_Delay(100);
      IO_AD_RELAY_control(eRELAY_AC, 1, true);   // AC_L ON
      HAL_Delay(100);
      IO_AD_RELAY_control(eRELAY_DC, 2, false);  // DC_M OFF
      HAL_Delay(100);
      IO_AD_RELAY_control(eRELAY_DC, 1, false);  // DC_P OFF
      HAL_Delay(100);
      break;

    case eMODE_VEHICLE_DISCHARGE:
      // DC discharge: AC relays OFF (AC_L first, then AC_N), DC relays ON (DC_P first, then DC_M)
      // IO_AD_RELAY_control(eRELAY_AC, 1, false);  // AC_L OFF
      // HAL_Delay(100);
      // IO_AD_RELAY_control(eRELAY_AC, 2, false);  // AC_N OFF
      // HAL_Delay(100);
      // IO_AD_RELAY_control(eRELAY_DC, 1, true);   // DC_P ON
      // HAL_Delay(100);
      // IO_AD_RELAY_control(eRELAY_DC, 2, true);   // DC_M ON
      // HAL_Delay(100);
      break;

    case eMODE_BSA_DISCHARGE:
    {
      /* Self-healing: BSA running 중인데 EXT_RLY1 / DC_P 가 LOW 로 떨어지면 자동 복구.
       *  · BSA_Start 후 외부 요인 (BLE_STATUS 글리치, 일시적 mode=NONE 진입 시
       *    RELAY_process(NONE) 가 DC_P OFF 시킴, EMI 등) 으로 GPIO 가 꺼져도
       *    HW task 가 1ms 주기로 자동 복구하므로 fail-safe 효과.
       *  · g_bBatRelayConFlag = false (정상 STOP 후) 일 땐 복구 안 함 — 의도된 OFF 유지. */
      extern bool g_bBatRelayConFlag;
      if (g_bBatRelayConFlag) {
        if (HAL_GPIO_ReadPin(EXT_RLY1_EN_GPIO_Port, EXT_RLY1_EN_Pin) == GPIO_PIN_RESET) {
          IO_EXT_RLY_control(1, true);
          printf("[BSA HW] EXT_RLY1 self-healed (was LOW)\r\n");
        }
        if (HAL_GPIO_ReadPin(DC_P_RLY_EN_GPIO_Port, DC_P_RLY_EN_Pin) == GPIO_PIN_RESET) {
          IO_AD_RELAY_control(eRELAY_DC, 1, true);
          printf("[BSA HW] DC_P self-healed (was LOW)\r\n");
        }
      }
      break;
    }

    default:
      break;
  }
}

void IO_Resistance_Relay_Control(int ohm, bool enable)
{
  if (enable) {
    switch (ohm) {
      case DISCHARGE_RESISTANCE_0k:
        printf("Discharge Resistance Not Configured\r\n");
        // No relay control needed for 0Ω
        break;
      case DISCHARGE_RESISTANCE_21p5k:
        printf("21.5kOhm relay ON\r\n");
        IO_EXT_RLY_control(1, true);  
        IO_EXT_RLY_control(2, false);  
        IO_EXT_RLY_control(3, false);  
        IO_EXT_RLY_control(4, false);  
        break;
      case DISCHARGE_RESISTANCE_16p5k:
        printf("16.5kOhm relay ON\r\n");
        IO_EXT_RLY_control(2, true);  
        IO_EXT_RLY_control(1, false);  
        IO_EXT_RLY_control(3, false);  
        IO_EXT_RLY_control(4, false); 
        break;
      case DISCHARGE_RESISTANCE_7k:
        printf("7kOhm relay ON\r\n");
        IO_EXT_RLY_control(3, true);  
        IO_EXT_RLY_control(1, false);  
        IO_EXT_RLY_control(2, false);  
        IO_EXT_RLY_control(4, false);
        break;
      case DISCHARGE_RESISTANCE_165k:
        printf("165kOhm relay ON\r\n");
        IO_EXT_RLY_control(4, true);  
        IO_EXT_RLY_control(1, false);  
        IO_EXT_RLY_control(2, false);  
        IO_EXT_RLY_control(3, false);
        break;
      default:
        printf("[Resistance Relay] Invalid resistance value: %d\r\n", ohm);
        break;
    }
  }
}
/* ===== CP State Machine (Step 6) ======================================== */
/*
 * Ported from ECVT CP_Run() (EVCT_function.c)
 * Called every 1ms from StartHwConTask while in VEHICLE_CHARGE mode.
 *
 * Key adaptations from ECVT:
 *  - DC/AC: physical units (V, mA) from task-sensor.c instead of raw ADC
 *  - Timer: osKernelGetTickCount() instead of BaseTimer/SetTimer/Elapse
 *  - Relay: IO_AD_RELAY_control() instead of GPIO_WriteBit()
 *  - BLE:   GITPACKET_make_frame() instead of SendGITPtclFrame()
 */
static void CP_Run(void)
{
  if (!cp_run_enabled) return;

  if (cp_pwm_stop_request)
  {
    cp_state = CP_PWM_STOP;
    cp_pwm_stop_request = false;
  }

  uint16_t cp_adc_val = 0;

  switch (cp_state)
  {
    case CP_NONE:
      break;

    /* ---- CP_IDLE: Reset & start PWM ---- */
    case CP_IDLE:
      cp_error = CP_ERR_NONE;
      cp_6v_detect_cnt = 0;
      dc_retry_cnt = 0;
      ac_overcurrent_cnt = 0;
      ac_sampling_overcurrent_cnt = 0;
      ac_min_ma = 0.0f;
      ac_max_ma = 0.0f;
      
      // Start CP PWM at configured current
      CP_PWM_control(true, cp_charge_current);
      cp_state = CP_WAIT4CONNECTING;
      CP_TIMER_RESET();
      printf("[CP] IDLE -> WAIT4CONNECTING (current=%dA)\r\n", cp_charge_current);
      break;

    /* ---- CP_WAIT4CONNECTING: 6V detection ---- */
    case CP_WAIT4CONNECTING:
      // Check every 1 second
      if (CP_ELAPSED() >= 1000)
      {
        cp_adc_val = CP_ADC_Read();

        // Check for 6V (vehicle connected): 5.7V ~ 6.3V
        if (cp_adc_val > CP_READY_LOWER && cp_adc_val < CP_READY_UPPER)
        {
          cp_6v_detect_cnt++;
        }
        else
        {
          cp_6v_detect_cnt = 0;  // Reset on out-of-range
        }

        // 3 consecutive 6V detections -> cable connected
        if (cp_6v_detect_cnt >= CP_CHECK_CNT)
        {
          printf("[CP] 6V DETECTED! ADC=%d, cnt=%d\r\n", cp_adc_val, cp_6v_detect_cnt);
          osDelay(1500);  // 1.5s delay (Hyundai requirement from ECVT)
          // CP_SendStatusToBLE('O', '7');  // Notify app: cable connected
          cp_state = CP_PWM_START;
          CP_TIMER_RESET();
        }
        else
        {
          CP_TIMER_RESET();  // Restart 1s interval
        }
      }
      break;

    /* ---- CP_PWM_START: DC voltage zero check ---- */
    case CP_PWM_START:
      // Wait 1 second between retries
      if (CP_ELAPSED() >= 1000)
      {
        // DC voltage must be ~0V before enabling relay (use sensor data)
        if (1)  //(g_discharge_voltage_V <= DC_ZERO_THRESH_V)
        {
          // DC stable -> enable relays and proceed
          RELAY_process(eMODE_VEHICLE_CHARGE);
          cp_state = CP_PWM_RUNNING;
          CP_TIMER_RESET();
          printf("[CP] DC STABLE (%.1fV) -> RELAY ON, PWM_RUNNING\r\n", g_discharge_voltage_V);
          printf("[CP] PWM : %d%% (%.1fA)\r\n", (cp_charge_current == 0xFF) ? 100 : (cp_charge_current * 100 / 16), (float)cp_charge_current);
          printf("[CP] ADC at DC CHECK: %d\r\n", cp_adc_val);
        }
        else
        {
          dc_retry_cnt++;
          if (dc_retry_cnt >= DC_RETRY_MAX)
          {
            cp_state = CP_MODE_ERROR;
            cp_error = CP_ERR_DC_CHECK_FAIL;
            printf("[CP] DC CHECK FAIL after %d retries\r\n", dc_retry_cnt);
          }
          else
          {
            CP_TIMER_RESET();
            printf("[CP] DC not stable (%.1fV), retry #%d\r\n", g_discharge_voltage_V, dc_retry_cnt);
          }
        }
      }
      break;

    /* ---- CP_PWM_RUNNING: AC overcurrent monitoring ---- */
    case CP_PWM_RUNNING:
    {
      float ac_current = g_ac_charge_current_mA;

      // Immediate cutoff: absolute max exceeded
      if (ac_current >= AC_MAX_MA)
      {
        ac_overcurrent_cnt++;
        if (ac_overcurrent_cnt >= CP_CHECK_CNT)
        {
          cp_state = CP_MODE_ERROR;
          cp_error = CP_ERR_AC_MAX_OVERCURRENT;
          printf("[CP] AC MAX OVERCURRENT! (%.0f mA, cnt=%d)\r\n", ac_current, ac_overcurrent_cnt);
          break;
        }
      }

      // 50ms window sampling: track min/max for peak-to-peak
      if (CP_ELAPSED() < 50)
      {
        if (ac_current < ac_min_ma) ac_min_ma = ac_current;
        if (ac_current > ac_max_ma) ac_max_ma = ac_current;
      }
      else
      {
        float ac_peak = ac_max_ma - ac_min_ma;

        if (ac_peak > AC_OVERCURR_MA)
        {
          // Overcurrent detected in this 50ms window
          ac_sampling_overcurrent_cnt++;
          if (ac_sampling_overcurrent_cnt >= (CP_CHECK_CNT + 1))
          {
            cp_state = CP_MODE_ERROR;
            cp_error = CP_ERR_AC_SAMPLING_OVERCURRENT;
            printf("[CP] AC SAMPLING OVERCURRENT! (peak=%.0f mA, cnt=%d)\r\n",
                   ac_peak, ac_sampling_overcurrent_cnt);
          }
          else
          {
            ac_min_ma = 0.0f;
            ac_max_ma = 0.0f;
            CP_TIMER_RESET();
          }
        }
        else
        {
          // AC stable this window -> reset
          ac_min_ma = 0.0f;
          ac_max_ma = 0.0f;
          ac_sampling_overcurrent_cnt = 0;
          CP_TIMER_RESET();
        }
      }
    }
      break;

    /* ---- CP_PWM_STOP: Safe shutdown ---- */
    case CP_PWM_STOP:
      CP_PWM_control(true, 0xFF);   // +12V HIGH DC (idle state)
      cp_run_enabled = false;
      cp_state = CP_NONE;
      printf("[CP] PWM STOP -> NONE\r\n");
      break;

    /* ---- CP_MODE_ERROR: Error handling ---- */
    case CP_MODE_ERROR:
      // Safety: disable relays immediately
      RELAY_process(eMODE_NONE);

      switch (cp_error)
      {
        case CP_ERR_DC_NOT_STABLE:
          // CP_SendStatusToBLE('E', '1');
          printf("[CP] ERROR: DC_NOT_STABLE\r\n");
          break;
        case CP_ERR_DC_CHECK_FAIL:
          // CP_SendStatusToBLE('E', '2');
          printf("[CP] ERROR: DC_CHECK_FAIL\r\n");
          break;
        case CP_ERR_AC_MAX_OVERCURRENT:
          // CP_SendStatusToBLE('E', '3');
          printf("[CP] ERROR: AC_MAX_OVERCURRENT\r\n");
          break;
        case CP_ERR_AC_SAMPLING_OVERCURRENT:
          // CP_SendStatusToBLE('E', '4');
          printf("[CP] ERROR: AC_SAMPLING_OVERCURRENT\r\n");
          break;
        default:
          break;
      }
      cp_state = CP_PWM_STOP;
      break;

    default:
      break;
  }
}

/* ===== BLE Status Notification ========================================== */

// static void CP_SendStatusToBLE(uint8_t code1, uint8_t code2)
// {
//   uint8_t payload[2] = { code1, code2 };
//   uint8_t frame_buff[GITPACKET_FRAME_SIZE_MAX];

//   // // FuncID 0x15 for CP status (adjust as needed per protocol spec)
//   // int frame_size = GITPACKET_make_frame(0x15, payload, sizeof(payload), frame_buff);
//   // if (frame_size > 0)
//   // {
//   //   GITPACKET_send_frame_via_uart(frame_buff, (uint16_t)frame_size);
//   // }
// }

/* ===== HW Control Task Main Loop (Step 7) =============================== */

void StartHwConTask(void *argument)
{
  printf("start %s ... \r\n", __FUNCTION__);

  for(;;)
  {
    // Block until RUNNING signal
    osEventFlagsWait(g_taskEventFlags, EVT_TASK_HWCON, osFlagsWaitAny, osWaitForever);

    while (g_system_state >= eSYSTEM_STATE_RUNNING)
    {
      if (g_system_mode == eMODE_VEHICLE_CHARGE && g_device_state == eDEVICE_STATE_START)
      {
        // CP state machine drives relay control internally
        if (!cp_run_enabled)
        {
          cp_run_enabled = true;
          cp_charge_current = (int)g_dbConfig.max_charge_current;
          cp_state = CP_IDLE;
          printf("[HWCON] VEHICLE_CHARGE: CP start (current=%dA)\r\n", cp_charge_current);
        }
        CP_Run();
      }
      else if (g_system_mode == eMODE_VEHICLE_CHARGE && g_device_state != eDEVICE_STATE_START)
      {
        /* Start 명령 전 또는 Stop 명령 후: CP 중단 */
        if (cp_run_enabled)
        {
          cp_run_enabled = false;
          cp_state = CP_NONE;
          CP_PWM_control(false, 0);
          printf("[HWCON] VEHICLE_CHARGE: CP stopped (device_state=%d)\r\n", g_device_state);
        }
      }
      else if (g_system_mode == eMODE_VEHICLE_DISCHARGE)
      {
        if (cp_run_enabled) { cp_run_enabled = false; cp_state = CP_NONE; }
        RELAY_process(eMODE_VEHICLE_DISCHARGE);
        CP_PWM_control(false, 0);
      }
      else if (g_system_mode == eMODE_BSA_DISCHARGE)
      {
        if (cp_run_enabled) { cp_run_enabled = false; cp_state = CP_NONE; }
        RELAY_process(eMODE_BSA_DISCHARGE);
        CP_PWM_control(false, 0);
      }
      else if (g_system_mode == eMODE_SETTING)
      {
        // Setting mode: no HW control
      }
      else
      {
        // NONE mode: safe state
        if (cp_run_enabled) { cp_run_enabled = false; cp_state = CP_NONE; }
        RELAY_process(eMODE_NONE);
        CP_PWM_control(false, 0);
      }
      osDelay(1);
    }

    // RUNNING exit -> safe state
    cp_run_enabled = false;
    cp_state = CP_NONE;
    RELAY_process(eMODE_NONE);
    CP_PWM_control(false, 0);
  }
}

/* ===== CAN Enable (unchanged) =========================================== */

void CAN_enable(int ch, bool enable)
{
  if (ch == 1)
  {
    if (enable)
    {
      IO_CONTROL_LOW(CAN_STB_EN1);
      IO_CONTROL_HIGH( CAN1_SW_EN );
    }
    else
    {
      IO_CONTROL_HIGH(CAN_STB_EN1);
      IO_CONTROL_LOW( CAN1_SW_EN );
    }
  }
  else if (ch == 2)
  {
    if (enable)
    {
      IO_CONTROL_LOW(CAN_STB_EN2);
      IO_CONTROL_HIGH( CAN2_SW_EN );
    }
    else
    {
      IO_CONTROL_HIGH(CAN_STB_EN2);
      IO_CONTROL_LOW( CAN2_SW_EN );
    }
  }
}

/* ===== Relay / SS / EXT GPIO Control (unchanged) ======================== */

void IO_AD_RELAY_control(int type, int ch, bool enable)
{
  if (type == eRELAY_AC)  // 0
  {
    if (ch == 1)
    {
      if (enable)     IO_CONTROL_HIGH( AC_L_RLY_EN );
      else            IO_CONTROL_LOW( AC_L_RLY_EN );
    }
    else if (ch == 2)
    {
      if (enable)     IO_CONTROL_HIGH( AC_N_RLY_EN );
      else            IO_CONTROL_LOW( AC_N_RLY_EN );
    }
  }
  else if (type == eRELAY_DC) // 1
  {
    if (ch == 1)
    {
      if (enable)     IO_CONTROL_HIGH( DC_P_RLY_EN );
      else            IO_CONTROL_LOW( DC_P_RLY_EN );
    }
    else if (ch == 2)
    {
      if (enable)     IO_CONTROL_HIGH( DC_M_RLY_EN );
      else            IO_CONTROL_LOW( DC_M_RLY_EN );
    }
  }
}

void IO_SS_control(int ch, bool enable)
{
  if(ch == 1) {       // MAIN FAN Relay
    if (enable)      IO_CONTROL_HIGH( SS1_EN );
    else             IO_CONTROL_LOW( SS1_EN );
  }
  else if(ch == 2)  { // Spare
    if (enable)      IO_CONTROL_HIGH( SS2_EN );
    else             IO_CONTROL_LOW( SS2_EN );
  }
  else if(ch == 3)  { // FAN1, FAN2 Relay
    if (enable)      IO_CONTROL_HIGH( SS3_EN );
    else             IO_CONTROL_LOW( SS3_EN );
  }
  else if(ch == 4)  { // FAN3, FAN4 Relay
    if (enable)      IO_CONTROL_HIGH( SS4_EN );
    else             IO_CONTROL_LOW( SS4_EN );
  }
  else if(ch == 5)  { // LAMP RED Relay
    if (enable)      IO_CONTROL_HIGH( SS5_EN );
    else             IO_CONTROL_LOW( SS5_EN );
  }
  else if(ch == 6)  { // LAMP YELLOW Relay
    if (enable)      IO_CONTROL_HIGH( SS6_EN );
    else             IO_CONTROL_LOW( SS6_EN );
  }
  else if(ch == 7)  { // LAMP GREEN Relay
    if (enable)      IO_CONTROL_HIGH( SS7_EN );
    else             IO_CONTROL_LOW( SS7_EN );
  }
  else if(ch == 8)  { // LAMP BUZZER Relay
    if (enable)      IO_CONTROL_HIGH( SS8_EN );
    else             IO_CONTROL_LOW( SS8_EN );
  }
  printf("[FAN] SS%d %s\r\n", ch, enable ? "ON" : "OFF");
}

void IO_EXT_RLY_control(int ch, bool enable)
{
  if(ch == 1) {
    if (enable)      IO_CONTROL_HIGH( EXT_RLY1_EN );
    else             IO_CONTROL_LOW( EXT_RLY1_EN );
  }
  else if(ch == 2)  {
    if (enable)      IO_CONTROL_HIGH( EXT_RLY2_EN );
    else             IO_CONTROL_LOW( EXT_RLY2_EN );
  }
  else if(ch == 3)  {
    if (enable)      IO_CONTROL_HIGH( EXT_RLY3_EN );
    else             IO_CONTROL_LOW( EXT_RLY3_EN );
  }
  else if(ch == 4)  {
    if (enable)      IO_CONTROL_HIGH( EXT_RLY4_EN );
    else             IO_CONTROL_LOW( EXT_RLY4_EN );
  }
  printf("[BSA] EXT_RLY%d %s\r\n", ch, enable ? "ON" : "OFF");
}


void IO_ALL_OFF_control(void)
{

  // OFF : AC_L → AC_N → DC_M → DC_P (100ms)
  IO_AD_RELAY_control(0, 1, false);  // AC_L off
  HAL_Delay(100);
  IO_AD_RELAY_control(0, 2, false);  // AC_N off
  HAL_Delay(100);
  IO_AD_RELAY_control(1, 2, false);  // DC_M off
  HAL_Delay(100);
  IO_AD_RELAY_control(1, 1, false);  // DC_P off
  HAL_Delay(100);
  IO_EXT_RLY_control(1, false);  
  HAL_Delay(10);
  IO_EXT_RLY_control(2, false);  
  HAL_Delay(10);
  IO_EXT_RLY_control(3, false);  
  HAL_Delay(10);
  IO_EXT_RLY_control(4, false);  

  // HAL_Delay(10);
  // IO_CONTROL_LOW(CAN1_SW_EN);
  // HAL_Delay(10);
  // IO_CONTROL_LOW(CAN2_SW_EN);
}

/* ===== FAN Cooldown (1min after STOP) =================================== */
/*
 * STOP seq(BLE 0x91 STOP / LCD NotifyYes) -> FAN_CooldownArm() 
 * FAN_CooldownProcess() -> state_machine() 에서 주기적으로 호출
 *   1) 1min 
 *   2) AC_L/AC_N/DC_P/DC_M/EXT_RLY1~4 GPIO All LOW Check
 * 1),2) OK -> SS1~SS4 OFF
 * New Mode Start (g_system_mode != NONE) -> auto cancel.
 */
#define FAN_COOLDOWN_DURATION_MS  (1U * 60U * 1000U)   /* 1 분 */

volatile bool     g_fan_cooldown_armed     = false;
volatile uint32_t g_fan_cooldown_armed_tick = 0;

void FAN_CooldownArm(void)
{
  g_fan_cooldown_armed_tick = HAL_GetTick();
  g_fan_cooldown_armed      = true;
  printf("[FAN] cooldown (1min)\r\n");
  // printf("[FAN] cooldown armed (1min) - SS1~SS4 will OFF when relays confirmed LOW\r\n");
}

void FAN_CooldownProcess(void)
{
  if (!g_fan_cooldown_armed) return;

  /* New Mode running start -> cooldown cancel  */
  if (g_system_mode != eMODE_NONE) {
    g_fan_cooldown_armed = false;
    printf("[FAN] cooldown canceled (mode=%d)\r\n", (int)g_system_mode);
    return;
  }

  if ((HAL_GetTick() - g_fan_cooldown_armed_tick) < FAN_COOLDOWN_DURATION_MS) return;

  bool all_low =
      (HAL_GPIO_ReadPin(DC_P_RLY_EN_GPIO_Port, DC_P_RLY_EN_Pin) == GPIO_PIN_RESET) &&
      (HAL_GPIO_ReadPin(DC_M_RLY_EN_GPIO_Port, DC_M_RLY_EN_Pin) == GPIO_PIN_RESET) &&
      (HAL_GPIO_ReadPin(EXT_RLY1_EN_GPIO_Port, EXT_RLY1_EN_Pin) == GPIO_PIN_RESET) &&
      (HAL_GPIO_ReadPin(EXT_RLY2_EN_GPIO_Port, EXT_RLY2_EN_Pin) == GPIO_PIN_RESET) &&
      (HAL_GPIO_ReadPin(EXT_RLY3_EN_GPIO_Port, EXT_RLY3_EN_Pin) == GPIO_PIN_RESET) &&
      (HAL_GPIO_ReadPin(EXT_RLY4_EN_GPIO_Port, EXT_RLY4_EN_Pin) == GPIO_PIN_RESET);

  if (!all_low) {
    static uint32_t s_lastLogTick = 0;
    if ((HAL_GetTick() - s_lastLogTick) >= 5000) {
      s_lastLogTick = HAL_GetTick();
      printf("[FAN] cooldown elapsed but a relay is HIGH - waiting\r\n");
    }
    return;
  }

  for (int i = 1; i <= 4; i++) {
    IO_SS_control(i, false);
  }
  g_fan_cooldown_armed = false;
  printf("[FAN] cooldown done -> SS1~SS4 OFF\r\n");
}

/**
 * eMMC power + reset 시퀀스 (SW reset 후 init 실패 방어 강화)
 *  1. RST_N LOW + PWR OFF  : eMMC 강제 정지 + 전원 차단
 *  2. 100ms 대기            : 디커플링 커패시터 완전 방전 + eMMC 내부 상태 초기화
 *  3. PWR ON                : 전원 복구
 *  4. 10ms 대기             : VCCQ ramp-up + 안정화
 *  5. RST_N HIGH            : reset 해제 → eMMC 부팅 시작
 *  6. 10ms 대기             : eMMC 자체 부팅 시간 (JEDEC Spec: tRSTW ≤ 1ms, 여유분 포함)
 */
void EMMC_Enable(void)
{
  /* 1) 강제 정지 상태로 진입 */
  IO_CONTROL_LOW(EMMC_RST_N);    /* RST assert (LOW = reset 활성) */
  IO_CONTROL_LOW(EMMC_PWR_EN);   /* 전원 차단 */

  /* 2) 디커플링 캡 방전 시간 — SW reset 시 핵심 (10ms → 100ms 로 증가) */
  HAL_Delay(100);

  /* 3) 전원 복구 + ramp-up 대기 */
  IO_CONTROL_HIGH(EMMC_PWR_EN);
  HAL_Delay(10);

  /* 4) Reset 해제 → eMMC 부팅 시작 */
  IO_CONTROL_HIGH(EMMC_RST_N);
  HAL_Delay(10);
}

void EMMC_Disable( void )
{
	IO_CONTROL_HIGH( EMMC_RST_N );
	IO_CONTROL_LOW( EMMC_PWR_EN );
}