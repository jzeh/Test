/**
 * ******************************************************************************
 * @file    task-sensor.c
 * @brief   Sensor Task (I2C1, I2C2) - MCP3424 ADC
 *
 *  트리거: g_system_state >= eSYSTEM_STATE_RUNNING 일 때 주기적 실행
 *  웨이크: osEventFlagsWait (EVT_TASK_SENSOR signal 대기 → 조건 충족 시 동작)
 *
 *  ADC : MCP3424
 *  ADC 1 ) V/C SENSOR ADC (I2C2, addr 0x68 고정 - 채널은 config 레지스터로 선택)
 *    - CH1+ : DISCHARGE_CURRENT_IN (ZEN-U2 직선형, 0~50A : 0.0 - 2.0V, 오프셋 없음)
 *    - CH2+ : DISCHARGE_VOLTAGE_IN (0~1000V : 0.0 - 2.0V, IVS-D4 직선형 Vin=V_adc×500, 실측 보정 스케일 적용)
 *    - CH3+ : AC_CHARGE_CURRENT_IN (0~50A : 0.4 - 2.0V, 4-20mA current loop, 100ohm shunt)
 *    - ADR0 : ADC_CH_CONT2   (PD13)
 *    - ADR1 : ADC_CH_CONT1   (PD12)
 *    - SDA  : ADC_SDA        (PB11)
 *    - SCL  : ADC_SCL        (PB10)
 *  ADC 2 ) TEMP SENSOR ADC (I2C1, addr 0x68 - NTC 분압 출력 V_ADC → LUT 보간, -40~150°C)
 *    - CH1+ : TS_1 (온도 1)
 *    - CH2+ : TS_2 (온도 2)
 *    - CH3+ : TS_3 (온도 3)
 *    - ADR0 : TS_ADC_CH_CONT2  (PB5)
 *    - ADR1 : TS_ADC_CH_CONT1  (PB4)
 *    - SDA  : TS_ADC_SDA       (PB7)
 *    - SCL  : TS_ADC_SCL       (PB6)
 * ******************************************************************************
 */

/* Includes  -----------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/git-functionlist.h"
#include "../Inc/task-hwcontrol.h"
#include "../Inc/task-lcd.h"
#include "../Inc/led-indicator.h"
// i2c.h not needed - hi2c1/hi2c2 declared extern in sys-common.h

/* Define    -----------------------------------------------------------*/

// MCP3424 I2C Address (7-bit) = 0b1101[ADR1][ADR0]
// 아래는 ADR 핀 조합별 주소 참고표. 실제 구현은 ADR 핀(ADC_CH_CONT1/2)을 LOW로 고정하여
// 주소는 0x68 하나만 사용하고, CH1~CH3 채널 전환은 Config 레지스터의 C1:C0 비트로 수행한다
// (SENSOR_read() 참고). 아래 CH2/CH3 주소는 정의되어 있으나 코드에서 사용되지 않음.
//   CH1: ADR1=Low,  ADR0=Low  → 0x68  (실사용)
//   CH2: ADR1=High, ADR0=Low  → 0x6A  (미사용)
//   CH3: ADR1=Low,  ADR0=High → 0x69  (미사용)
//   CH4: ADR1=High, ADR0=High → 0x6B  (미사용)
#define MCP3424_ADDR_VC_CH1   0x68    // ADC 1 : V/C Sensor 실사용 주소 (CH1~3 공용, config 레지스터로 채널 구분)
#define MCP3424_ADDR_VC_CH2   0x6A    // 미사용 (참고용 정의)
#define MCP3424_ADDR_VC_CH3   0x69    // 미사용 (참고용 정의)
#define MCP3424_ADDR_TEMP     0x68    // ADC 2 : Temp Sensor

// Configuration Register Bits
// Bit 7   : RDY   (1=Start Conversion / Read: 0=New Data Ready)
// Bit 6-5 : C1-C0 (Channel Select)
// Bit 4   : O/C   (0=One-Shot, 1=Continuous)
// Bit 3-2 : S1-S0 (Sample Rate / Resolution)
// Bit 1-0 : G1-G0 (PGA Gain)

#define MCP3424_RDY           (1 << 7)

#define MCP3424_CH1           (0 << 5)
#define MCP3424_CH2           (1 << 5)
#define MCP3424_CH3           (2 << 5)
#define MCP3424_CH4           (3 << 5)

#define MCP3424_MODE_ONESHOT  (0 << 4)
#define MCP3424_MODE_CONT     (1 << 4)

#define MCP3424_RES_12BIT     (0 << 2)   // 240 SPS,  LSB = 1mV
#define MCP3424_RES_14BIT     (1 << 2)   // 60 SPS,   LSB = 250uV
#define MCP3424_RES_16BIT     (2 << 2)   // 15 SPS,   LSB = 62.5uV
#define MCP3424_RES_18BIT     (3 << 2)   // 3.75 SPS, LSB = 15.625uV

#define MCP3424_GAIN_X1       (0)
#define MCP3424_GAIN_X2       (1)
#define MCP3424_GAIN_X4       (2)
#define MCP3424_GAIN_X8       (3)

// 사용할 기본 설정 : 18bit, PGA x1, One-Shot
#define MCP3424_DEFAULT_CFG   (MCP3424_RES_18BIT | MCP3424_GAIN_X1 | MCP3424_MODE_ONESHOT)

// 18bit 변환 시간: 1/3.75 SPS = ~267ms. RDY 폴링 최대 대기(여유 포함)
#define MCP3424_CONV_TIMEOUT_MS  350

// 18bit LSB (PGA x1) = 2.048V / 2^17 = 15.625uV
#define MCP3424_LSB_18BIT     (15.625e-6f)

// I2C timeout
#define MCP3424_I2C_TIMEOUT   100

// 채널 수
#define MCP3424_VC_CH_COUNT   3   // ADC 1: CH1=방전전류, CH2=방전전압, CH3=AC충전전류
#define MCP3424_TEMP_CH_COUNT 3   // ADC 2: 온도1, 온도2, 온도3

// 센서 변환 상수
// CH1 : ZEN-U2 직선형 전류 출력 (실측 테이블 기준)
//   전류(A)  ZEN-U2출력(V)  V_adc(V)
//    0       0.0000         0.0000
//   10       1.0000         0.4000
//   25       2.5000         1.0000
//   50       5.0000         2.0000
//   변환: current(A) = V_adc × 25.0  (50A / 2.0V)
//
// CH3 : 4-20mA 전류루프 + 100Ω shunt 방식
//   공통 변환: value = (V_adc - 0.4) / 1.6 × fullscale

// CH1: 충방전 전류 — ZEN-U2 직선형 (0~50A → 0~2.0V, 오프셋 없음)
#define CH1_VADC_TO_CURRENT   (50.0f / 2.0f)       // = 25.0 A/V

// 4-20mA 전류루프 공통 (100Ω shunt) — CH3 전용
#define LOOP_V_MIN      0.4f    // 4mA × 100Ω = 0.4V (zero point)
#define LOOP_V_SPAN     1.6f    // (2.0V - 0.4V) = 1.6V (full span)

// CH2: 방전 전압 (IVS-D4 직선형 전압 출력, 4-20mA 아님)
//   IVS-D4 출력 = Vin / 200,  ADC 입력 분압 후 V_adc = 출력 / 2.5 = Vin / 500
//   이론값: Vin(V) = V_adc(V) × 500
//   실측 보정 (선형회귀, 101~800V 15점 캘리브레이션):
//     Vin(V) = V_adc(V) × 504.8733 + 1.3126  (최대 오차: ±0.28V, 보정 전 ±9V)
//   ※ 현재 SENSOR_convert() 에서는 OFFSET 항이 주석 처리되어 미적용
//      → 실제 계산: Vin(V) = V_adc(V) × 504.8733 만 사용
//   ※ vc_adc_voltage[1] 에 V_adc 전압이 들어옴
#define DISCHARGE_VADC_SCALE    504.8733f  // Calibrated scale: V_adc(V) to Vin(V)
#define DISCHARGE_VADC_OFFSET     1.3126f  // Calibrated offset (V) - 현재 SENSOR_convert()에서 미사용(비활성화)

// CH3: AC 충전 전류 (4-20mA → 0~50A)
#define CH3_FULLSCALE   50.0f   // A

// Sensor error thresholds
#define SENSOR_OVER_CHARGE_V       900.0f
#define SENSOR_OVER_DISCHARGE_V    200.0f
#define SENSOR_OVER_TEMPERATURE_C  100.0f
#define SENSOR_TEMP_CAUTION_C      100.0f
#define SENSOR_TEMP_CAUTION_MAX_C  149.0f
#define SENSOR_TEMP_WARN_C         150.0f

// 센서 에러 체크 시작 지연: 장비 START 후 이 시간 경과 후부터 에러 체크 수행
#define SENSOR_ERRCHK_START_DELAY_MS  30000U   // 30초

/* Variables -----------------------------------------------------------*/

// Thread Def
osThreadId_t sensorTaskHandle;
uint32_t sensorTaskBuffer[ 2048 ];
osStaticThreadDef_t sensorTaskControlBlock;
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .cb_mem = &sensorTaskControlBlock,
  .cb_size = sizeof(sensorTaskControlBlock),
  .stack_mem = &sensorTaskBuffer[0],
  .stack_size = sizeof(sensorTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};

// ADC 1 (V/C Sensor)
int32_t   g_vc_adc_raw[MCP3424_VC_CH_COUNT];   // 18bit signed raw (-131072~131071)
float     vc_adc_voltage[MCP3424_VC_CH_COUNT];

// ADC 2 (Temp Sensor)
int32_t   g_temp_adc_raw[MCP3424_TEMP_CH_COUNT];   // 18bit signed raw
float     temp_adc_voltage[MCP3424_TEMP_CH_COUNT];

// Converted Value
float     g_discharge_current_A;    // Discharge Current (A)  - ZEN-U2 (CH1)
float     g_discharge_voltage_V;    // Discharge Voltage (V)  - IVS-D4 (CH2)
float     g_ac_charge_current_mA;   // AC Charge Current (mA) - H-1W (CH3), 4~20mA loop
float     g_temp_value[MCP3424_TEMP_CH_COUNT];  // Temperature (°C) - NTC LUT 보간 변환 (NTC_ADCtoTemp) 적용됨

// Power (Voltage * Current)
float     g_power_W;                  // Power (W) = Voltage (V) × Current (A)

// CH select label
static const uint8_t mcp3424_ch_bits[] = { MCP3424_CH1, MCP3424_CH2, MCP3424_CH3, MCP3424_CH4 };

void StartSensorTask(void *argument);
void SENSOR_init(void);
void SENSOR_read(void);
void SENSOR_process(void);
static void SENSOR_convert(void);
static float NTC_ADCtoTemp(float v_adc);
static uint8_t SENSOR_check_error(void);
static void SENSOR_update_error_state(uint8_t error_code);
static void SENSOR_update_temperature_monitor(void);
static void SENSOR_reset_temperature_monitor(void);

typedef enum {
  SENSOR_TEMP_STATE_NORMAL = 0,
  SENSOR_TEMP_STATE_CAUTION,
  SENSOR_TEMP_STATE_WARN,
} eSensorTempState;

static eSensorTempState s_last_temp_state = SENSOR_TEMP_STATE_NORMAL;

/* 센서 에러 체크 게이트
 *  · START(0x91 등) 시 SENSOR_ErrorCheck_Arm() → arm_tick 기록, armed=true
 *  · SENSOR_process 는 armed && device_state==START && (경과 ≥ 30초) 일 때만 에러 체크
 *  · 정지(0x91 STOP / LCD 정지 / Disconnect 등) 시 SENSOR_ErrorCheck_Disarm() 또는
 *    g_device_state 가 START 를 벗어나면 즉시 중단 */
static volatile bool     s_errchk_armed      = false;
static volatile uint32_t s_errchk_arm_tick   = 0;
static uint8_t           s_last_sensor_error = ERROR_CODE_NONE;

/* ===== NTC Temperature Lookup Table ==================================== */
/*
 * NTC 분압 출력 전압 → 온도 룩업 테이블 (NTCALUG01T103G501A)
 *
 *   회로: Vcc ── NTC(상단) ──[노드→ADC]── R(하단) ── GND
 *   V_ADC = Vcc × R / (R_NTC + R) = 600 / (R_NTC + 120)   (Vcc=5.0V, R=120Ω)
 *
 *   온도↑ → R_NTC↓ → V_ADC↑  (테이블은 V_ADC 오름차순)
 *   범위: -40 ~ 150°C, 5°C 간격 (39 entries)
 *
 *   vadc 값은 데이터시트 공칭 R_NTC 로부터 위 공식으로 정밀 계산.
 *   (테이블 공칭 V_ADC 3자리 표기와 일치, 저온부 단조성 확보용 6자리 사용)
 *   비교는 temp_adc_voltage[ch] (= raw × 15.625uV) 와 직접 수행.
 */
typedef struct {
  float   vadc;       // NTC 분압 출력 전압 V_ADC (V)
  int16_t temp_x10;   // Temperature × 10 (e.g., 250 = 25.0°C, -400 = -40.0°C)
} NTC_LUT_t;

static const NTC_LUT_t ntc_lut[] = {
  /*   V_ADC(V),  Temp×10 */
  { 0.001806f,   -400 },  // -40°C
  { 0.002500f,   -350 },  // -35°C
  { 0.003422f,   -300 },  // -30°C
  { 0.004637f,   -250 },  // -25°C
  { 0.006219f,   -200 },  // -20°C
  { 0.008262f,   -150 },  // -15°C
  { 0.010876f,   -100 },  // -10°C
  { 0.014192f,    -50 },  //  -5°C
  { 0.018363f,      0 },  //   0°C
  { 0.023567f,     50 },  //   5°C
  { 0.030012f,    100 },  //  10°C
  { 0.037931f,    150 },  //  15°C
  { 0.047589f,    200 },  //  20°C
  { 0.059289f,    250 },  //  25°C
  { 0.073359f,    300 },  //  30°C
  { 0.090158f,    350 },  //  35°C
  { 0.110092f,    400 },  //  40°C
  { 0.133571f,    450 },  //  45°C
  { 0.161074f,    500 },  //  50°C
  { 0.192988f,    550 },  //  55°C
  { 0.229885f,    600 },  //  60°C
  { 0.272232f,    650 },  //  65°C
  { 0.320342f,    700 },  //  70°C
  { 0.374766f,    750 },  //  75°C
  { 0.436047f,    800 },  //  80°C
  { 0.504202f,    850 },  //  85°C
  { 0.579448f,    900 },  //  90°C
  { 0.662252f,    950 },  //  95°C
  { 0.752540f,   1000 },  // 100°C
  { 0.850220f,   1050 },  // 105°C
  { 0.954958f,   1100 },  // 110°C
  { 1.066470f,   1150 },  // 115°C
  { 1.184366f,   1200 },  // 120°C
  { 1.308044f,   1250 },  // 125°C
  { 1.436438f,   1300 },  // 130°C
  { 1.569038f,   1350 },  // 135°C
  { 1.705030f,   1400 },  // 140°C
  { 1.843318f,   1450 },  // 145°C
  { 1.982816f,   1500 },  // 150°C
};
#define NTC_LUT_SIZE  (sizeof(ntc_lut) / sizeof(ntc_lut[0]))

/* Private Functions ---------------------------------------------------*/

/**
 * @brief  MCP3424에 Config 1바이트 Write → 변환 시작
 *         ┌─────────────────────────────────────────────────┐
 *         │  I2C Write (1 byte)                             │
 *         │  [S][Addr+W][Config Byte][P]                    │
 *         │                                                 │
 *         │  Config Byte 구조:                               │
 *         │  [RDY | C1 C0 | O/C | S1 S0 | G1 G0]            │
 *         │   bit7  bit6-5  bit4  bit3-2  bit1-0            │
 *         │                                                 │
 *         │  RDY=1로 Write → 변환 시작 (One-Shot)            │
 *         └─────────────────────────────────────────────────┘
 */
static HAL_StatusTypeDef MCP3424_WriteConfig(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t config)
{
  uint8_t data = MCP3424_RDY | config;  // RDY=1 : 변환 시작
  return HAL_I2C_Master_Transmit(hi2c, (uint16_t)(addr << 1), &data, 1, MCP3424_I2C_TIMEOUT);
}
/**
 * @brief  MCP3424 데이터 Read (18bit 모드)
 *         ┌─────────────────────────────────────────────────┐
 *         │  I2C Read (4 bytes for 18bit)                   │
 *         │  [S][Addr+R][D2][D1][D0][Config][P]             │
 *         │                                                 │
 *         │  Config의 RDY bit(bit7) 확인:                    │
 *         │    0 = 새 데이터 준비 완료 (유효)                 │
 *         │    1 = 아직 변환 중 (이전 데이터)                 │
 *         │                                                 │
 *         │  18bit: D2 상위 6bit = 부호확장, 하위 2bit=D17:D16│
 *         │  Data = D2[1:0]<<16 | D1<<8 | D0 (signed 18bit) │
 *         └─────────────────────────────────────────────────┘
 * @retval HAL_OK=새 데이터 읽기 성공, HAL_BUSY=변환 미완료
 */
static HAL_StatusTypeDef MCP3424_ReadData18(I2C_HandleTypeDef *hi2c, uint8_t addr, int32_t *raw)
{
  uint8_t buf[4]; // [D2(MSB), D1, D0(LSB), Config]
  HAL_StatusTypeDef ret;

  ret = HAL_I2C_Master_Receive(hi2c, (uint16_t)(addr << 1), buf, 4, MCP3424_I2C_TIMEOUT);
  if (ret != HAL_OK) return ret;

  // RDY bit 확인 (bit7 of Config byte)
  if (buf[3] & MCP3424_RDY) {
    return HAL_BUSY; // 변환 미완료
  }

  // signed 18bit 데이터 조합 + bit17 부호확장 (-131072~131071)
  int32_t v = ((int32_t)(buf[0] & 0x03) << 16) | ((int32_t)buf[1] << 8) | buf[2];
  v = (v ^ 0x20000) - 0x20000;   // bit17 sign-extend
  *raw = v;
  return HAL_OK;
}
/**
 * @brief  특정 채널 1회 변환 후 읽기 (One-Shot)
 *         ┌─────────────────────────────────────────────────┐
 *         │  1. Config Write (채널 + RDY=1) → 변환 시작      │
 *         │  2. RDY 비트 폴링 (18bit 변환 ~267ms)            │
 *         │  3. Data Read (4 bytes) → RDY 확인 → Raw 반환    │
 *         └─────────────────────────────────────────────────┘
 */
static HAL_StatusTypeDef MCP3424_ReadChannel(I2C_HandleTypeDef *hi2c, uint8_t addr,
                                              uint8_t channel, int32_t *raw)
{
  HAL_StatusTypeDef ret;
  uint8_t config = mcp3424_ch_bits[channel] | MCP3424_DEFAULT_CFG;

  // Step 1: Config Write → 변환 시작
  ret = MCP3424_WriteConfig(hi2c, addr, config);
  if (ret != HAL_OK) return ret;

  // Step 2~3: RDY 클리어(변환 완료)까지 폴링하며 Data Read
  const uint32_t STEP = 10;   // 10ms 간격 폴링
  uint32_t waited = 0;
  for (;;) {
    ret = MCP3424_ReadData18(hi2c, addr, raw);
    if (ret != HAL_BUSY) return ret;   // HAL_OK(새 데이터) 또는 I2C 에러
    if (waited >= MCP3424_CONV_TIMEOUT_MS) return HAL_TIMEOUT;
    osDelay(STEP);
    waited += STEP;
  }
}

/* Public Functions ----------------------------------------------------*/

void InitSensorTask(void)
{
  SENSOR_init();
  sensorTaskHandle = osThreadNew(StartSensorTask, NULL, &sensorTask_attributes);
}

/**
 * @brief  Sensor Task 메인 루프
 *         g_system_state >= eSYSTEM_STATE_RUNNING 일 때만 센서 읽기 수행
 *         100ms 주기로 전류/전압/온도 센서 데이터 수집 및 처리
 */
void StartSensorTask(void *argument)
{
  printf("start %s ... \r\n", __FUNCTION__);

  for(;;)
  {
    // 이벤트 플래그 대기 (RUNNING 진입 signal까지 블로킹)
    osEventFlagsWait(g_taskEventFlags, EVT_TASK_SENSOR, osFlagsWaitAny, osWaitForever);

    while (g_system_state >= eSYSTEM_STATE_RUNNING)
    {
      SENSOR_read();
      SENSOR_process();
      osDelay(100);
    }
    SENSOR_reset_temperature_monitor();
  }
}

/**
 * @brief  센서 초기화
 *         MCP3424는 별도의 Init 레지스터가 없음.
 *         Config 1바이트를 Write하면 그것이 설정이자 변환 시작 명령.
 *         여기서는 ADR 핀 설정 + 첫 번째 Config Write로 초기화.
 */
void SENSOR_init(void)
{
  // ADR 핀 설정 (I2C Address 결정)
  // ADC 1: ADR1=Low(PD12), ADR0=Low(PD13) → 0x68
  IO_CONTROL_LOW(ADC_CH_CONT1);   // ADR1 = Low
  IO_CONTROL_LOW(ADC_CH_CONT2);   // ADR0 = Low

  // ADC 2: ADR1=Low(PB4), ADR0=Low(PB5) → 0x68
  // TODO: ADC 2는 별도 I2C 버스이므로 같은 주소 사용 가능
  IO_CONTROL_LOW(TS_ADC_CH_CONT1);
  IO_CONTROL_LOW(TS_ADC_CH_CONT2);

  // 초기 Config Write (CH1, 16bit, Gain x1, One-Shot)
  // → 첫 번째 변환이 시작됨
  MCP3424_WriteConfig(&hi2c2, MCP3424_ADDR_VC_CH1,
                      MCP3424_CH1 | MCP3424_DEFAULT_CFG);
}

/**
 * @brief  전체 채널 순차 읽기
 *         ┌────────────────────────────────────────────────┐
 *         │  ADC 1 (V/C Sensor) - I2C2, addr 0x68 고정      │
 *         │    CH1: Config Write → Wait → Read → 방전전류 Raw│
 *         │    CH2: Config Write → Wait → Read → 방전전압 Raw│
 *         │    CH3: Config Write → Wait → Read → AC충전전류 Raw│
 *         │                                                │
 *         │  ADC 2 (Temp Sensor) - I2C1                    │
 *         │    CH1~CH3: 동일 방식으로 온도 Raw              │
 *         │                                                │
 *         │  Raw → Voltage 변환                            │
 *         └────────────────────────────────────────────────┘
 */
void SENSOR_read(void)
{
  // --- ADC 1 : V/C Sensor (I2C2) ---
  // U17 MCP3424 단일 칩: CH1+, CH2+, CH3+ 모두 같은 칩
  // ADR 핀 고정 (LOW/LOW → 0x68), config 레지스터 채널 선택 비트로 전환
  IO_CONTROL_LOW(ADC_CH_CONT1);
  IO_CONTROL_LOW(ADC_CH_CONT2);

  // CH1(방전전류): config channel=0
  if (MCP3424_ReadChannel(&hi2c2, MCP3424_ADDR_VC_CH1, 0, &g_vc_adc_raw[0]) == HAL_OK)
  {
    vc_adc_voltage[0] = (float)g_vc_adc_raw[0] * MCP3424_LSB_18BIT;
  }

  // CH2(방전전압): config channel=1
  if (MCP3424_ReadChannel(&hi2c2, MCP3424_ADDR_VC_CH1, 1, &g_vc_adc_raw[1]) == HAL_OK)
  {
    vc_adc_voltage[1] = (float)g_vc_adc_raw[1] * MCP3424_LSB_18BIT;
  }

  // CH3(AC충전전류): config channel=2
  if (MCP3424_ReadChannel(&hi2c2, MCP3424_ADDR_VC_CH1, 2, &g_vc_adc_raw[2]) == HAL_OK)
  {
    vc_adc_voltage[2] = (float)g_vc_adc_raw[2] * MCP3424_LSB_18BIT;
  }

  // --- ADC 2 : Temp Sensor (I2C1) ---
  for (uint8_t ch = 0; ch < MCP3424_TEMP_CH_COUNT; ch++)
  {
    if (MCP3424_ReadChannel(&hi2c1, MCP3424_ADDR_TEMP, ch, &g_temp_adc_raw[ch]) == HAL_OK)
    {
      temp_adc_voltage[ch] = (float)g_temp_adc_raw[ch] * MCP3424_LSB_18BIT;
    }
  }
}

/* ===== NTC ADC → Temperature Conversion ================================ */
/*
 * NTC 분압 출력 전압 V_ADC 로 룩업 테이블 검색 + 선형 보간
 * 테이블: V_ADC 오름차순 (ntc_lut[0]=최소V/-40°C, ntc_lut[N-1]=최대V/150°C)
 *
 * @param  v_adc  NTC 분압 출력 전압 (V) = raw × 15.625uV
 * @return 온도 (°C, float). 범위 밖이면 -40.0 또는 150.0 클램프
 */
static float NTC_ADCtoTemp(float v_adc)
{
  // 범위 밖 클램프
  if (v_adc <= ntc_lut[0].vadc)
    return (float)ntc_lut[0].temp_x10 / 10.0f;

  if (v_adc >= ntc_lut[NTC_LUT_SIZE - 1].vadc)
    return (float)ntc_lut[NTC_LUT_SIZE - 1].temp_x10 / 10.0f;

  // 선형 검색 (테이블 V_ADC 오름차순이므로 v_adc 보다 커지는 지점 탐색)
  for (uint8_t i = 0; i < NTC_LUT_SIZE - 1; i++)
  {
    if (v_adc >= ntc_lut[i].vadc && v_adc < ntc_lut[i + 1].vadc)
    {
      // 선형 보간: temp = t1 + (t2-t1) * (v_adc-v1) / (v2-v1)
      float v1 = ntc_lut[i].vadc;
      float v2 = ntc_lut[i + 1].vadc;
      float t1 = (float)ntc_lut[i].temp_x10;
      float t2 = (float)ntc_lut[i + 1].temp_x10;
      float ratio = (v_adc - v1) / (v2 - v1);
      return (t1 + (t2 - t1) * ratio) / 10.0f;
    }
  }

  return 0.0f; // fallback
}

/**
 * @brief  4-20mA current convert
 *         value = (V_adc - 0.4) / 1.6 × fullscale, 하한 0 클램프
 */
static float loop_4_20mA_convert(float v_adc, float fullscale)
{
  float val = (v_adc - LOOP_V_MIN) / LOOP_V_SPAN * fullscale;
  return (val < 0.0f) ? 0.0f : val;
}

static void SENSOR_convert(void)
{
  // CH1: Charge/Discharge current (A) - ZEN-U2 linear: 0~50A → 0~2.0V
  g_discharge_current_A = vc_adc_voltage[0] * CH1_VADC_TO_CURRENT;
  if (g_discharge_current_A < 0.0f) g_discharge_current_A = 0.0f;

  // CH2:  Charge/Discharge Voltage (V) - IVS-D4 linear output (calibrated)
  g_discharge_voltage_V = vc_adc_voltage[1] * DISCHARGE_VADC_SCALE;// + DISCHARGE_VADC_OFFSET;

  // CH3: AC Charge Current (mA) - 4-20mA → 0~50A, keep unit in mA
  g_ac_charge_current_mA = loop_4_20mA_convert(vc_adc_voltage[2], CH3_FULLSCALE) * 1000.0f;

  // 온도 (°C) - NTC (V_ADC) LUT + linear interpolation
  for (uint8_t ch = 0; ch < MCP3424_TEMP_CH_COUNT; ch++)
  {
    g_temp_value[ch] = NTC_ADCtoTemp(temp_adc_voltage[ch]);
  }

  // Power (W) = Voltage (V) × Current (A)
  g_power_W = g_discharge_voltage_V * g_discharge_current_A;
}

static uint8_t SENSOR_check_error(void)
{
  if (g_discharge_voltage_V > SENSOR_OVER_CHARGE_V)
  {
    return ERROR_CODE_OVER_CHARGE;
  }

  // temp : don't check for test 
  // if (g_discharge_voltage_V < SENSOR_OVER_DISCHARGE_V)
  // {
  //   return ERROR_CODE_OVER_DISCHARGE;
  // }

  for (uint8_t ch = 0; ch < MCP3424_TEMP_CH_COUNT; ch++)
  {
    if (g_temp_value[ch] > SENSOR_OVER_TEMPERATURE_C)
    {
      return ERROR_CODE_OVER_TEMPERATURE;
    }
  }

  return ERROR_CODE_NONE;
}

static bool SENSOR_error_requires_stop(uint8_t error_code)
{
  switch (error_code)
  {
    case ERROR_CODE_OVER_CHARGE:
    case ERROR_CODE_OVER_DISCHARGE:
      return true;
    default:
      return false;
  }
}

static void SENSOR_update_error_state(uint8_t error_code)
{
  if (error_code == s_last_sensor_error)
  {
    return;
  }

  if (error_code == ERROR_CODE_NONE)
  {
    if (g_error_code == s_last_sensor_error)
    {
      g_error_code = ERROR_CODE_NONE;
    }
    s_last_sensor_error = ERROR_CODE_NONE;
    return;
  }

  s_last_sensor_error = error_code;
  g_error_code = error_code;

  printf("[SENSOR] error=%u DV=%.1fV T1=%.1fC T2=%.1fC T3=%.1fC\r\n",
         error_code,
         g_discharge_voltage_V,
         g_temp_value[0],
         g_temp_value[1],
         g_temp_value[2]);

  if (SENSOR_error_requires_stop(error_code))
  {
    printf("[SENSOR] error=%u -> STOP sequence + BLE TX\r\n", error_code);
    FL_GDS_StopDevice_WithoutPopup();
  }

  FL_GDS_Send_Error_Code();
}

static bool SENSOR_is_running_mode(void)
{
  return (g_system_mode == eMODE_VEHICLE_CHARGE ||
          g_system_mode == eMODE_VEHICLE_DISCHARGE ||
          g_system_mode == eMODE_BSA_DISCHARGE);
}

static uint8_t SENSOR_get_running_screen(void)
{
  return (g_system_mode == eMODE_VEHICLE_CHARGE) ? LCD_SCR_CHARGE : LCD_SCR_DISCHARGE;
}

static void SENSOR_update_temperature_monitor(void)
{
  float max_temp = g_temp_value[0];
  if (g_temp_value[1] > max_temp)
  {
    max_temp = g_temp_value[1];
  }

  eSensorTempState state = SENSOR_TEMP_STATE_NORMAL;
  if (max_temp > SENSOR_TEMP_WARN_C)
  {
    state = SENSOR_TEMP_STATE_WARN;
  }
  else if (max_temp > SENSOR_TEMP_CAUTION_C && max_temp <= SENSOR_TEMP_CAUTION_MAX_C)
  {
    state = SENSOR_TEMP_STATE_CAUTION;
  }

  if (!SENSOR_is_running_mode())
  {
    s_last_temp_state = SENSOR_TEMP_STATE_NORMAL;
    return;
  }

  if (state == SENSOR_TEMP_STATE_NORMAL)
  {
    if (s_last_temp_state != SENSOR_TEMP_STATE_NORMAL)
    {
      LED_ClearCriticalError();
      s_last_temp_state = SENSOR_TEMP_STATE_NORMAL;
    }
    return;
  }

  if (state == s_last_temp_state)
  {
    return;
  }

  uint8_t screen = SENSOR_get_running_screen();
  s_last_temp_state = state;

  if (state == SENSOR_TEMP_STATE_WARN)
  {
    LED_RaiseCriticalError();

    printf("[SENSOR] temperature WARN max=%.1fC -> stop + popup\r\n", max_temp);
    FL_GDS_StopDevice_WithoutPopup();

    LCD_PostPopupWarnTemp(screen);
  }
  else
  {
    /* CAUTION: 정식 ERROR_CODE 가 없어 LED 표시 없음(운전 계속 → G_ON 유지).
     * 향후 CAUTION 에 에러코드가 생기면 LED_RaiseWarning() 을 그대로 재사용하면 됨. */
    printf("[SENSOR] temperature CAUTION max=%.1fC -> popup\r\n", max_temp);
    LCD_PostPopupCautTemp(screen);
  }
}

static void SENSOR_reset_temperature_monitor(void)
{
  s_last_temp_state = SENSOR_TEMP_STATE_NORMAL;
}

/**
 * @brief  센서 에러 체크 Arm — 장비 START 시 호출
 *         호출 시점부터 SENSOR_ERRCHK_START_DELAY_MS(30초) 경과 후 에러 체크 시작.
 *         직전 run 의 잔류 에러 상태(s_last_sensor_error)도 초기화하여 재검출 보장.
 */
void SENSOR_ErrorCheck_Arm(void)
{
  s_errchk_arm_tick   = HAL_GetTick();
  s_errchk_armed      = true;
  s_last_sensor_error = ERROR_CODE_NONE;
  printf("[SENSOR] error-check armed (start after %lus)\r\n",
         (unsigned long)(SENSOR_ERRCHK_START_DELAY_MS / 1000U));
}

/**
 * @brief  센서 에러 체크 Disarm — 장비 정지(STOP/LCD정지/Disconnect 등) 시 호출
 *         즉시 에러 체크 중단. 잔류 에러 상태도 초기화.
 */
void SENSOR_ErrorCheck_Disarm(void)
{
  if (s_errchk_armed) {
    s_errchk_armed = false;
    printf("[SENSOR] error-check disarmed (device stopped)\r\n");
  }
  s_last_sensor_error = ERROR_CODE_NONE;
}

void SENSOR_process(void)
{
  SENSOR_convert();

  /* 센서 에러 체크: START 후 30초 경과 + 장비 구동(device_state==START) 중일 때만.
   *  · FL_GDS_SetDevice_Start(START) 의 SENSOR_ErrorCheck_Arm() 로 30초 카운트 시작
   *  · 0x91 STOP / LCD 정지 / Disconnect 등 정지 시 Disarm() 호출 + device_state 가
   *    START 를 벗어나므로 어느 쪽이든 즉시 중단 (이중 안전장치) */
  if (s_errchk_armed &&
      g_device_state == eDEVICE_STATE_START &&
      (uint32_t)(HAL_GetTick() - s_errchk_arm_tick) >= SENSOR_ERRCHK_START_DELAY_MS)
  {
    SENSOR_update_error_state(SENSOR_check_error());
  }

  SENSOR_update_temperature_monitor();

  // printf("[SENSOR] DI:%.2fA DV:%.1fV AC:%.1fmA T1:%d T2:%d T3:%d\r\n",
  //     g_discharge_current_A, g_discharge_voltage_V,
  //     g_ac_charge_current_mA,
  //     g_temp_value[0], g_temp_value[1], g_temp_value[2]);
}
