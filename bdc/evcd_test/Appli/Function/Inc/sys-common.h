#ifndef SYS_COMMON_H
#define SYS_COMMON_H
/**
  ******************************************************************************
 * @file    sys-common.h
 * @brief   common header file for EVCD (STM32H7S3I8)
 *          Migrated from h7s3l8_nucleo_prj - adapted for evcd board
  ******************************************************************************
 */

/* Includes  -----------------------------------------------------------*/
// standard
#include <stdio.h>
#include <stdbool.h>
#include "typedef.h"
#include "string.h"
// hal
#include "stm32h7rsxx_hal.h"

// peripheral (evcd: all peripheral init in main.c, no separate .h files)
#include "main.h"

// middleware
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os2.h"

// user
#include "git-protocol.h"
// #include "git-comm.h"
#include "git-ble.h"
#include "sys-emmc.h"   
// #include "task-hwcontrol.h"

/* ===== IO Control Macros ================================================ */
#define IO_CONTROL_HIGH( __name )							HAL_GPIO_WritePin( __name ## _GPIO_Port, __name ## _Pin, GPIO_PIN_SET )
#define IO_CONTROL_LOW( __name )							HAL_GPIO_WritePin( __name ## _GPIO_Port, __name ## _Pin, GPIO_PIN_RESET )
#define IO_CONTROL_TOGGLE( __name )						HAL_GPIO_TogglePin( __name ## _GPIO_Port, __name ## _Pin )
#define IO_CONTROL_GET( __name )							HAL_GPIO_ReadPin( __name ## _GPIO_Port, __name ## _Pin )


/* eMMC 폴더 / 파일 상수는 sys-emmc.h 에 통합 — 필요 시 include "sys-emmc.h" */

/* ===== System State / Mode Enums ======================================== */
typedef enum
{
  eSYSTEM_STATE_NONE = 0,
  eSYSTEM_STATE_INIT,
  eSYSTEM_STATE_IDLE,
  eSYSTEM_STATE_RUNNING,
  eSYSTEM_STATE_ERROR
} SystemState_t;

typedef enum
{
  eMODE_NONE = 0,
  eMODE_VEHICLE_CHARGE,
  eMODE_VEHICLE_DISCHARGE,
  eMODE_BSA_DISCHARGE,
  eMODE_SETTING
} SystemMode_t;

typedef enum
{
  eDEVICE_STATE_NONE = 0,
  eDEVICE_STATE_START = 1,
  eDEVICE_STATE_STOP = 2,
  eDEVICE_STATE_RUNNING = 3,
  eDEVICE_STATE_ERROR = 4
} DeviceState_t;

/* ===== Global System Variables ========================================== */

extern SystemState_t g_system_state;
extern SystemMode_t  g_system_mode;

// Sensor data (task-sensor.c)
extern int32_t   g_vc_adc_raw[3];       // ADC 1 (18bit): [0]=Discharge current, [1]=Discharge voltage, [2]=AC charge current
extern int32_t   g_temp_adc_raw[3];     // ADC 2 (18bit): [0]=Temperature1, [1]=Temperature2, [2]=Temperature3
extern float     g_discharge_current_A; // Discharge current (A) - ZEN-U2 (CH1)
extern float     g_discharge_voltage_V; // Discharge voltage (V) - IVT-D4 (CH2)
extern float     g_ac_charge_current_mA;// AC charge current (mA) - H-1W (CH3) =>  NOT USING 
extern float     g_temp_value[3];       // Temperature converted value (TODO: Apply NTC)
extern float     g_power_W;            // Power (W) = Voltage (V) × Current (A)

/**
 * @brief Device time structure  (BLE 0x91 payload 14B)
 *   year(2B LE) + month(2B LE) + day(2B LE) + hour(2B LE)
 *   + minute(2B LE) + second(2B LE) + millisecond(2B LE)
 */
typedef struct {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint16_t millisecond;
} stDeviceTime;

// Device state (sys-main.c)
extern DeviceState_t g_device_state;
extern stDeviceTime  g_device_time;

// BSA CAN RX alive (task-can.c FDCAN1 RxFifo0 ISR) - HAL_GetTick() 기준 마지막 수신 시각
extern volatile uint32_t g_bsaCanRxLastTick;

#define DB_VEHICLE_INFO_MAX		64		
#define DB_VIN_LENGTH			    17		

typedef struct {
	uint8_t			mode;					      // MODE: 0x10=Veh Charge, 0x20=Veh Discharge, 0x30=BSA Discharge
	uint8_t			vehicle_info_len;		// Vehicle info length (variable, max DB_VEHICLE_INFO_MAX)
	char			vehicle_info[DB_VEHICLE_INFO_MAX];	// Vehicle info (string)
	char			vin[DB_VIN_LENGTH + 1];	
  
	uint16_t		vehicle_fullcharge_SOC;		  // Vehicle charge target SOC
	uint16_t		vehicle_fulldischarge_SOC;	// Vehicle discharge target SOC
	uint16_t		bsa_fulldischarge_SOC;		  // Battery pack discharge target SOC
	uint16_t		max_charge_current;			    // Maximum charge current
	uint8_t			expected_charge_hour;		    // Expected charge time (hour, 0x12 payload[2])
	uint8_t			expected_charge_minute;		   // Expected charge time (minute, 0x12 payload[3])
	uint16_t		max_discharge_current;		// Maximum discharge current
	uint16_t		max_discharge_voltage;		// Maximum discharge voltage
	uint16_t		min_charge_current;			// Minimum charge current
	uint16_t		min_discharge_current;		// Minimum discharge current

	uint16_t		max_active_temp;			// Maximum operating temperature
	uint16_t		max_charge_time;			// Maximum charge time
	uint16_t		max_discharge_time;			// Maximum discharge time

	uint16_t		fan_maintain_time;			// Fan maintain time
	uint16_t		fan_stop_temp;				// Fan stop temperature
	uint16_t		battery_nominal_capacity;	// Battery nominal capacity
	uint8_t			resistance;					// Resistance (0x12 payload[4])

	uint16_t		reserved[10];
} stDB_CONFIG;

extern stDB_CONFIG g_dbConfig;

/* ===== Task Event Flags ================================================= */
extern osEventFlagsId_t g_taskEventFlags;

#define EVT_TASK_SENSOR    (1U << 0)
#define EVT_TASK_HWCON     (1U << 1)
#define EVT_TASK_BSA       (1U << 2)
#define EVT_TASK_PLC       (1U << 3)

/* ===== Exported types ==================================================== */

// Queue / Task extern
typedef StaticTask_t osStaticThreadDef_t;

extern QueueHandle_t sendCAN_Q;
extern QueueHandle_t receiveCAN_Q;
extern QueueHandle_t startCAN_Q;

// HAL handle extern (evcd board peripherals)
extern UART_HandleTypeDef huart1;     // PLC UART (USART1)
extern UART_HandleTypeDef huart2;     // BLE UART (USART2 + RTS/CTS)
extern UART_HandleTypeDef huart4;     // AIM UART (UART4) - RS485 절연저항기(AIM-D100)
extern UART_HandleTypeDef huart7;     // Debug UART (UART7) - printf
extern UART_HandleTypeDef huart8;     // LCD UART (UART8)
extern FDCAN_HandleTypeDef hfdcan1;   // Vehicle CAN (FDCAN1)
extern FDCAN_HandleTypeDef hfdcan2;   // PLC CAN (FDCAN2)
extern I2C_HandleTypeDef hi2c1;       // Temp Sensor I2C (I2C1)
extern I2C_HandleTypeDef hi2c2;       // V/C Sensor I2C (I2C2)
extern TIM_HandleTypeDef htim4;       // CP PWM Timer (TIM4 CH4)
extern ADC_HandleTypeDef hadc1;       // CP ADC (ADC1)

/* ===== Error Codes ==================================================== */
// FW -> APP (TX by FUNCID 0x81)
#define ERROR_CODE_NONE                       0
#define ERROR_CODE_OVER_CHARGE                1   // 900V Exceed
#define ERROR_CODE_OVER_DISCHARGE             2   // 200V Under
#define ERROR_CODE_OVER_TEMPERATURE           3   // 100°C Exceed -> Alert / 150°C Exceed -> Emergency Stop
#define ERROR_CODE_USER_LCD_CONTROL_STOP      4   // Stop Running by LCD Stop button
#define ERROR_CODE_BSA_POWER_CONNECTOR_FAIL   5   // BSA Power Connector Fail detected after start
#define ERROR_CODE_INSUL_RESISTANCE           6   // Resistance Error

// APP -> FW (RX by FUNCID 0x82)
#define ERROR_CODE_GDS_NONE                   0
#define ERROR_CODE_GDS_MAINRLY_CONTROL_FAIL   1   // Fail Control BSA Main Relay by VCI3 
#define ERROR_CODE_GDS_USER_STOP_COMMAND      2   // User Stop before Finishing Process by APP
#define ERROR_CODE_GDS_BSA_READ_FAIL          3   // Fail reading battery status value from VCI3
#define ERROR_CODE_GDS_OVER_DISCHARGE         4   // Cell Voltage Under 2.5V detected by VCI3

/* ===== Discharge Resistance Table ======================================== */
typedef enum{
  DISCHARGE_RESISTANCE_0k = 0,
  DISCHARGE_RESISTANCE_21p5k,
  DISCHARGE_RESISTANCE_16p5k,
  DISCHARGE_RESISTANCE_7k,
  DISCHARGE_RESISTANCE_165k,
} DischargeOhm_t;


/* ===== Task Init Function Prototypes ==================================== */

// Always Run
extern void InitCommTask(void);       // BLE Task (UART2)
extern void InitLcdTask(void);        // LCD Task (UART8)

// Optional Run
extern void InitPLCTask(void);        // PLC Task (FDCAN2)
extern void InitSensorTask(void);     // Sensor Task (I2C1, I2C2)
extern void InitHwConTask(void);      // HW Control (GPIO, PWM)
extern void InitAimTask(void);        // AIM Task (UART4) - 절연저항 모니터링
extern bool StartBSAThread(void);

/* ===== task-sensor.c — Error Check Arm/Disarm =============================
 *  · SENSOR_ErrorCheck_Arm    : Call when device START → 30 seconds later, start error check
 *  · SENSOR_ErrorCheck_Disarm : Call when device STOP (STOP/LCD stop/Disconnect, etc.) → immediately stop */
extern void SENSOR_ErrorCheck_Arm(void);
extern void SENSOR_ErrorCheck_Disarm(void);
extern void InitCANTask(void);        // CAN Task (for test) - task-can.c

/* ===== Device Stop Sequence (sys-main.c) ================================
 *  · 장비 정지 공통 시퀀스에서 사용하는 팝업 모드
 *  · BDC_StopSequence 는 git-functionlist.c → sys-main.c 로 이동됨 */
typedef enum
{
  BDC_STOP_POPUP_NONE = 0,
  BDC_STOP_POPUP_WORK_DONE,
  BDC_STOP_POPUP_APP_ERROR,
} eBDC_StopPopupMode;

extern void BDC_StopSequence(eBDC_StopPopupMode popup_mode);

/* ===== sys-main.c Functions ============================================= */
extern void SYS_init(void);
extern void state_machine(void);
extern void mode_control(SystemMode_t mode);

/* ===== task-hwcontrol.c — FAN Cooldown (3min after STOP) ================= */
extern void FAN_CooldownArm(void);     /* Call at the end of STOP sequence */
extern void FAN_CooldownProcess(void); /* Call periodically in state_machine() */

/* ===== Operation Time Timer (start at state_machine NONE→START edge) ========
 *  · g_run_time_sec    : Accumulated seconds after start (reset to 0 at start, hold value at stop)
 *  · g_run_time_active : true if state_machine increments g_run_time_sec every second
 *  · Format_RunTime_HMS : seconds → "HH:MM:SS" 문자열 변환 (out 버퍼 최소 9B) */
extern volatile uint32_t g_run_time_sec;
extern volatile bool     g_run_time_active;
extern void Format_RunTime_HMS(uint32_t seconds, char *out, size_t out_size);

/* ===== Timer Helpers (sys-main.c) ======================================= */
uint32_t Get_Tmr(void);                                /* ms tick (osKernelGetTickCount) */
uint32_t Get_TmrDelta(uint32_t ulNew, uint32_t ulOld); /* delta, 32-bit overflow 처리 */
uint32_t GetUnixTime(void);                            /* placeholder: tick/1000 (TODO: RTC) */

#endif // SYS_COMMON_H
