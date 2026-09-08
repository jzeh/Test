#ifndef TASK_AIM_H
#define TASK_AIM_H

#include "sys-common.h"

/*
    AIM-D100-T Protocol 
    Frame Structure (Modbus-RTU)
    ┌─────────┬──────────┬──────────────┬───────────┐ - Address  : Slave address (1~247, default 1)
    │ Address │ Function │   Data Zone  │  CRC16    │ - Function : 03H/04H = Read, 06H = Write, 10H = Write Multiple
    │  1 byte │  1 byte  │    N bytes   │  2 bytes  │ - Data     : depends on req/res
    └─────────┴──────────┴──────────────┴───────────┘ - CRC16    : Modbus CRC (init 0xFFFF), LE  
*/

/* Address */
#define AIM_SLAVE_ADDRESS 0x01

/* Function */
#define AIM_FUNC_READ_03H       0x03
#define AIM_FUNC_READ_04H       0x04
#define AIM_FUNC_WRITE_SINGLE   0x06
#define AIM_FUNC_WRITE_MULTIPLE 0x10

/* Data Zone */
// Register Address Table
#define AIM_REG_ADDRESS             0x01
#define AIM_REG_BAUDRATE            0x02
#define AIM_REG_LANGUAGE            0x03
#define AIM_REG_LCD_CONTRAST        0x04
#define AIM_REG_LCD_BACKLIGHT_TIME  0x05
#define AIM_REG_YEAR                0x06
#define AIM_REG_MONTH               0x07
#define AIM_REG_DAY                 0x08
#define AIM_REG_HOUR                0x09
#define AIM_REG_MINUTE              0x0A
#define AIM_REG_SECOND              0x0B
#define AIM_REG_SW_NUMBER           0x0C
#define AIM_REG_SW_VERSION          0x0D

#define AIM_REG_FAULT_TYPE          0x20

#define AIM_REG_POLE_INSULATION_RES_P          0x21  // + 양극 절연저항 (unit : kOhm, ratio : 1)
#define AIM_REG_POLE_INSULATION_RES_N          0x22  // - 음극 절연저항 (unit : kOhm, ratio : 1)
#define AIM_REG_POLE_VOLTAGE_TO_GND_P          0x23  // + 양극 대지전압 (unit : V   , ratio : 0.1)
#define AIM_REG_POLE_VOLTAGE_TO_GND_N          0x24  // - 음극 대지전압 (unit : V   , ratio : 0.1)
#define AIM_REG_SYSTEM_VOLTAGE                 0x25  // unit : V, ratio : 0.1

#define AIM_REG_VOLTAGE_ALARM_SWITCH           0x30  // ON : 0xFEFE , OFF : 0xEFEF (default : OFF)
#define AIM_REG_VOLTAGE_RATED_VALUE            0x31  // 0V ~ 1000V (default : 1000)
#define AIM_REG_OVERVOLTAGE_VALUE              0x32  // 100 ~ 120% (default : 120)
#define AIM_REG_UNDERVOLTAGE_VALUE             0x33  // 80  ~ 100% (default : 80)

#define AIM_REG_INSULATION_ALARM_SWITCH             0x34  // ON : 0xFEFE , OFF : 0xEFEF (default : ON)
#define AIM_REG_POLE_INSULATION_RES_ALRAM_VALUE_P   0x35  // 10 ~ 10000kOhm (default : 100)
#define AIM_REG_POLE_INSULATION_RES_ALRAM_VALUE_N   0x36  // 10 ~ 10000kOhm (default : 50)

#define AIM_REG_INSULTAION_MONITOR_TIME        0x3F // 0 : 500ms/cycle, 1 : 1000ms/cycle
#define AIM_REG_INSULATION_MONITOR_TRIG_MODE   0x40 // 0x01 : Cycle (default), 0x10 : Comm, 0x11 : Cycle + Comm 
#define AIM_REG_CAP_DELAY_TIME                 0x41 // 0 ~ 60000ms (default : 0)
#define AIM_REG_RES_MONITORING_DELAY_TIME      0x42 // 5 ~ 500ms (default : 5)

#define AIM_REG_RESET_MODE          0x43 // 0: Auto, 1: Manual (default : 0)
#define AIM_REG_DO_RELAY_MODE       0x44 // 0: N/O, 1: N/C (default : 0)

#define AIM_REG_RESET_METER         0x46
#define AIM_REG_CLEAR_SOE           0x47

// SOE (Event Log, R)
#define AIM_REG_SOE1_FAULT_TYPE      0x50
#define AIM_REG_SOE1_FAULT_VALUE     0x51


// Fault Type (0x20) Bit Def
#define AIM_ST_OVERVOLT     (1U << 0)    
#define AIM_ST_UNDERVOLT    (1U << 1)    
#define AIM_ST_POS_INS_ALARM    (1U << 2)    // 양극 절연 알람
#define AIM_ST_POS_INS_WARN     (1U << 3)    // 양극 절연 경고
#define AIM_ST_NEG_INS_ALARM    (1U << 4)    // 음극 절연 알람
#define AIM_ST_NEG_INS_WARN     (1U << 5)    // 음극 절연 경고
#define AIM_ST_WIRING_ERR   (1U << 15)       // DC+/DC- connection error


// AIM monitoring value
typedef struct
{
    int32_t           insul_res_p_kohm;   // 0x21 양극(+) 절연저항 [kOhm]
    int32_t           insul_res_n_kohm;   // 0x22 음극(-) 절연저항 [kOhm]
    float             volt_p_V;           // 0x23 양극 대지전압 [V]
    float             volt_n_V;           // 0x24 음극 대지전압 [V]
    float             sys_volt_V;         // 0x25 시스템 전압   [V]
    uint16_t          fault_status;       // 0x20 bit -> status
    volatile uint32_t last_ok_tick;       // 마지막 통신 성공 tick
    bool              comm_ok;            // 최근 트랜잭션 성공 여부
} stAIM_Data;

extern stAIM_Data g_aimData;

void AIM_init(void);
void AIM_set_dir(bool dir);
void AIM_read_monitor(void);   /* 0x20~0x25 폴링 후 g_aimData 갱신 */

extern volatile bool g_aim_poll_enable;   /* 자동 폴링 on/off (CLI 수동 테스트용) */

/* CLI/디버그용 : 매뉴얼 기반 임의 레지스터 read(03)/write(06) 테스트
 *   txBuf/rxBuf 에 실제 송/수신 raw 를 담고 rxLen 에 수신 길이 반환.
 *   반환 true = 주소/함수/CRC(또는 echo) 검증 통과 */
bool AIM_Cli_ReadReg(uint16_t reg, uint16_t cnt,
                     uint8_t *txBuf, int *txLen,
                     uint8_t *rxBuf, int rxCap, int *rxLen);
bool AIM_Cli_WriteReg(uint16_t reg, uint16_t val,
                      uint8_t *txBuf, int *txLen,
                      uint8_t *rxBuf, int rxCap, int *rxLen);

HAL_StatusTypeDef AIM_GetLastRxStatus(void);   /* 마지막 수신 HAL 상태 (실패사유 진단) */


#endif // TASK_AIM_H