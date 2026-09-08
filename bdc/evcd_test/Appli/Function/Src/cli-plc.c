/**
 * @file  cli-plc.c
 * @brief task-cli.c 에서 분리된 도메인 CLI (모듈화 2순위)
 */

#include "../Inc/sys-common.h"
#include "../Inc/task-cli.h"
#include "../Inc/task-can.h"
#include "../Inc/task-plc.h"
#include "../Inc/task-lcd.h"
#include "../Inc/task-bsa.h"
#include "../Inc/git-functionlist.h"
#include "../Inc/sys-emmc.h"
#include "../Inc/task-hwcontrol.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


/*----------------------------------------------------------------------
 *  PLC Config CLI Commands
 *  - plc show              : Display PLC config (RX/TX) + RX values
 *  - plc rx                : Display RX parsed values only
 *  - plc set <type> <val>  : Set TX value (type=ePLC_RequestValueType)
 *  - plc setex             : Load example config (SECC + GDS tables)
 *  - plc clear             : Reset all PLC config
 *--------------------------------------------------------------------*/
static const char* CLI_PLC_ReqTypeStr(uint8_t t)
{
    switch (t) {
        case PLC_REQ_DCextd:              return "DCextd";
        case PLC_REQ_Heartbeat:           return "Heartbeat";
        case PLC_REQ_ChargeControl:       return "ChargingCtrl";
        case PLC_REQ_EVSEIsolationStatus: return "IsolationSta";
        case PLC_REQ_EVSEProcessing:      return "Processing";
        case PLC_REQ_EVSEMaxCurrent:      return "MaxCurrent";
        case PLC_REQ_EVSEMaxPower:        return "MaxPower";
        case PLC_REQ_EVSEMaxVolt:         return "MaxVolt";
        case PLC_REQ_EVSEPresentVoltage:  return "PresentVolt";
        case PLC_REQ_EVSEPresentCurrent:  return "PresentCurr";
        default:                          return "?";
    }
}

static const char* CLI_PLC_ResTypeStr(uint8_t t)
{
    switch (t) {
        case PLC_RES_CP:             return "CP";
        case PLC_RES_TARGET_VOLTAGE: return "TargetVolt";
        case PLC_RES_TARGET_CURRENT: return "TargetCurr";
        case PLC_RES_SECC_STATUS:    return "SECCStatus";
        default:                     return "?";
    }
}

static void CLI_PLC_Show(void)
{
    uint8_t i, j;

    printf("=== PLC Config (configured=%s) ===\r\n",
           g_stPLC_Config.configured ? "YES" : "NO");

    printf("--- RX (SECC: PLC->MCU) [%d entries] ---\r\n", g_stPLC_Config.rxCount);
    for (i = 0; i < g_stPLC_Config.rxCount; i++) {
        stPLC_RxConfig *rc = &g_stPLC_Config.rxConfigs[i];
        printf(" [%d] msgIdx=%d  ID=0x%08lX  interval=%dms  values=%d  lastRx=%lu\r\n",
               i, rc->messageIndex, rc->canId, rc->interval_ms, rc->messageCount, rc->lastRxTime);
        for (j = 0; j < rc->messageCount; j++) {
            stPLC_DataConfigMsg *m = &rc->messages[j];
            printf("   [%d] %s(type=%d) pos=%d sz=%d mask=0x%08lX conv(%d A=%d B=%d)\r\n",
                   j, CLI_PLC_ResTypeStr(m->valueType), m->valueType,
                   m->startPosition, m->dataSize, m->maskingValue,
                   m->convRule.convType, m->convRule.convA, m->convRule.convB);
        }
    }

    printf("--- TX (GDS: MCU->PLC) [%d entries] ---\r\n", g_stPLC_Config.txCount);
    for (i = 0; i < g_stPLC_Config.txCount; i++) {
        stPLC_TxConfig *tc = &g_stPLC_Config.txConfigs[i];
        printf(" [%d] msgIdx=%d  ID=0x%08lX  interval=%dms  values=%d  en=%d\r\n",
               i, tc->messageIndex, tc->canId, tc->interval_ms, tc->messageCount, tc->enabled);
        printf("   Template: ");
        for (j = 0; j < PLC_CAN_DATA_LEN; j++) {
            printf("%02X ", tc->dataTemplate[j]);
        }
        printf("\r\n");
        for (j = 0; j < tc->messageCount; j++) {
            stPLC_DataConfigMsg *m = &tc->messages[j];
            printf("   [%d] %s(type=%d) pos=%d sz=%d mask=0x%08lX\r\n",
                   j, CLI_PLC_ReqTypeStr(m->valueType), m->valueType,
                   m->startPosition, m->dataSize, m->maskingValue);
        }
    }

    /* RX Values */
    printf("--- RX Values ---\r\n");
    printf("  CP                = %u\r\n",    g_stPLC_RxValues.cp);
    printf("  TargetVoltage     = %u (x0.1V)\r\n", g_stPLC_RxValues.targetVoltage);
    printf("  TargetCurrent     = %u (x0.1A)\r\n", g_stPLC_RxValues.targetCurrent);
    printf("  SECCStatus(10B)   = 0x%02X\r\n", g_stPLC_RxValues.seccStatus);
    printf("  SECCStatus(101)   = 0x%02X\r\n", g_stPLC_RxValues.seccStatusLegacy);
    printf("  SoC               = %u%%\r\n",   g_stPLC_RxValues.soc);
    printf("  ChargingComplete  = %u\r\n",     g_stPLC_RxValues.chargingComplete);
    printf("  EV MaxVoltage     = %u (x0.1V = %.1fV)\r\n", g_stPLC_RxValues.evMaxVoltage, g_stPLC_RxValues.evMaxVoltage * 0.1f);
    printf("  EV MaxCurrent     = %u (x0.1A = %.1fA)\r\n", g_stPLC_RxValues.evMaxCurrent, g_stPLC_RxValues.evMaxCurrent * 0.1f);
    printf("  EV EnergyCapacity = %u (x10Wh = %ukWh)\r\n", g_stPLC_RxValues.evEnergyCapacity, g_stPLC_RxValues.evEnergyCapacity / 100);
    printf("  EV EnergyRequest  = %u (x10Wh = %ukWh)\r\n", g_stPLC_RxValues.evEnergyRequest,  g_stPLC_RxValues.evEnergyRequest  / 100);
}

static void CLI_PLC_ShowRx(void)
{
    printf("=== PLC RX Values ===\r\n");
    printf("  CP                = %u\r\n",    g_stPLC_RxValues.cp);
    printf("  TargetVoltage     = %u (x0.1V)\r\n", g_stPLC_RxValues.targetVoltage);
    printf("  TargetCurrent     = %u (x0.1A)\r\n", g_stPLC_RxValues.targetCurrent);
    printf("  SECCStatus(10B)   = 0x%02X\r\n", g_stPLC_RxValues.seccStatus);
    printf("  SECCStatus(101)   = 0x%02X\r\n", g_stPLC_RxValues.seccStatusLegacy);
    printf("  SoC               = %u%%\r\n",   g_stPLC_RxValues.soc);
    printf("  ChargingComplete  = %u\r\n",     g_stPLC_RxValues.chargingComplete);
    printf("  EV MaxVoltage     = %u (x0.1V = %.1fV)\r\n", g_stPLC_RxValues.evMaxVoltage, g_stPLC_RxValues.evMaxVoltage * 0.1f);
    printf("  EV MaxCurrent     = %u (x0.1A = %.1fA)\r\n", g_stPLC_RxValues.evMaxCurrent, g_stPLC_RxValues.evMaxCurrent * 0.1f);
    printf("  EV EnergyCapacity = %u (x10Wh = %ukWh)\r\n", g_stPLC_RxValues.evEnergyCapacity, g_stPLC_RxValues.evEnergyCapacity / 100);
    printf("  EV EnergyRequest  = %u (x10Wh = %ukWh)\r\n", g_stPLC_RxValues.evEnergyRequest,  g_stPLC_RxValues.evEnergyRequest  / 100);
}

/**
 * @brief  Load PLC config example matching the DB text.
 */
static void CLI_PLC_SetExample(void)
{
    PLC_ResetConfig();

#define PLC_SET_CONV(msg, type, a, b, c, d, e, f)  do { \
        (msg).convRule.convType = (type); \
        (msg).convRule.convA    = (a); \
        (msg).convRule.convB    = (b); \
        (msg).convRule.convC    = (c); \
        (msg).convRule.convD    = (d); \
        (msg).convRule.convE    = (e); \
        (msg).convRule.convF    = (f); \
    } while (0)

#define PLC_SET_MSG(msg, vt, pos, size, mask, type, a, b, c, d, e, f)  do { \
        (msg).valueType     = (vt); \
        (msg).startPosition = (pos); \
        (msg).dataSize      = (size); \
        (msg).maskingValue  = (mask); \
        PLC_SET_CONV((msg), (type), (a), (b), (c), (d), (e), (f)); \
    } while (0)

    /* SECC messageindex=1, responsevalue=15ECC102, interval=50ms */
    g_stPLC_Config.rxConfigs[0].messageIndex = 1;
    g_stPLC_Config.rxConfigs[0].protocolId   = 0x0100;
    g_stPLC_Config.rxConfigs[0].dataRate     = 1;
    g_stPLC_Config.rxConfigs[0].canId        = 0x15ECC102;
    g_stPLC_Config.rxConfigs[0].interval_ms  = 50;
    g_stPLC_Config.rxConfigs[0].messageCount = 1;
    PLC_SET_MSG(g_stPLC_Config.rxConfigs[0].messages[0],
                PLC_RES_CP, 1, 1, 0x000000FF, 1, 1, 0, 0, 0, 2, 1);

    /* SECC messageindex=2, responsevalue=15ECC109, interval=50ms */
    g_stPLC_Config.rxConfigs[1].messageIndex = 2;
    g_stPLC_Config.rxConfigs[1].protocolId   = 0x0100;
    g_stPLC_Config.rxConfigs[1].dataRate     = 1;
    g_stPLC_Config.rxConfigs[1].canId        = 0x15ECC109;
    g_stPLC_Config.rxConfigs[1].interval_ms  = 50;
    g_stPLC_Config.rxConfigs[1].messageCount = 2;
    PLC_SET_MSG(g_stPLC_Config.rxConfigs[1].messages[0],
                PLC_RES_TARGET_VOLTAGE, 0, 2, 0x0000FFFF, 1, 1, 0, 0, 0, 2, 1);
    PLC_SET_MSG(g_stPLC_Config.rxConfigs[1].messages[1],
                PLC_RES_TARGET_CURRENT, 2, 2, 0x0000FFFF, 1, 1, 0, 0, 0, 2, 1);

    /* SECC messageindex=3, responsevalue=15ECC10B, interval=50ms */
    g_stPLC_Config.rxConfigs[2].messageIndex = 3;
    g_stPLC_Config.rxConfigs[2].protocolId   = 0x0100;
    g_stPLC_Config.rxConfigs[2].dataRate     = 1;
    g_stPLC_Config.rxConfigs[2].canId        = 0x15ECC10B;
    g_stPLC_Config.rxConfigs[2].interval_ms  = 50;
    g_stPLC_Config.rxConfigs[2].messageCount = 1;
    PLC_SET_MSG(g_stPLC_Config.rxConfigs[2].messages[0],
                PLC_RES_SECC_STATUS, 0, 1, 0x000000FF, 0, 0, 0, 0, 0, 0, 0);

    g_stPLC_Config.rxCount = 3;

    /* GDS messageindex=1, requestvalue=15ECC001000000XX00000000 */
    g_stPLC_Config.txConfigs[0].messageIndex = 1;
    g_stPLC_Config.txConfigs[0].protocolId   = 0x0100;
    g_stPLC_Config.txConfigs[0].dataRate     = 1;
    g_stPLC_Config.txConfigs[0].canId        = 0x15ECC001;
    g_stPLC_Config.txConfigs[0].interval_ms  = 50;
    g_stPLC_Config.txConfigs[0].messageCount = 1;
    g_stPLC_Config.txConfigs[0].enabled      = true;
    memset(g_stPLC_Config.txConfigs[0].dataTemplate, 0, PLC_CAN_DATA_LEN);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[0].messages[0],
                PLC_REQ_DCextd, 3, 1, 0x000000FF, 0, 0, 0, 0, 0, 0, 0);

    /* GDS messageindex=2, requestvalue=15ECC002XXYY0000ZZ00GG00 */
    g_stPLC_Config.txConfigs[1].messageIndex = 2;
    g_stPLC_Config.txConfigs[1].protocolId   = 0x0100;
    g_stPLC_Config.txConfigs[1].dataRate     = 1;
    g_stPLC_Config.txConfigs[1].canId        = 0x15ECC002;
    g_stPLC_Config.txConfigs[1].interval_ms  = 50;
    g_stPLC_Config.txConfigs[1].messageCount = 4;
    g_stPLC_Config.txConfigs[1].enabled      = true;
    memset(g_stPLC_Config.txConfigs[1].dataTemplate, 0, PLC_CAN_DATA_LEN);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[1].messages[0],
                PLC_REQ_Heartbeat, 0, 1, 0x000000FF, 0, 0, 0, 0, 0, 0, 0);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[1].messages[1],
                PLC_REQ_ChargeControl, 1, 1, 0x000000FF, 0, 0, 0, 0, 0, 0, 0);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[1].messages[2],
                PLC_REQ_EVSEIsolationStatus, 4, 1, 0x000000FF, 1, 1, 0, 0, 0, 0, 1);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[1].messages[3],
                PLC_REQ_EVSEProcessing, 6, 1, 0x000000FF, 1, 1, 0, 0, 0, 0, 1);

    /* GDS messageindex=3, requestvalue=15ECC003XXXXYYYYZZZZ0000 */
    g_stPLC_Config.txConfigs[2].messageIndex = 3;
    g_stPLC_Config.txConfigs[2].protocolId   = 0x0100;
    g_stPLC_Config.txConfigs[2].dataRate     = 1;
    g_stPLC_Config.txConfigs[2].canId        = 0x15ECC003;
    g_stPLC_Config.txConfigs[2].interval_ms  = 50;
    g_stPLC_Config.txConfigs[2].messageCount = 3;
    g_stPLC_Config.txConfigs[2].enabled      = true;
    memset(g_stPLC_Config.txConfigs[2].dataTemplate, 0, PLC_CAN_DATA_LEN);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[2].messages[0],
                PLC_REQ_EVSEMaxCurrent, 0, 2, 0x0000FFFF, 0, 0, 0, 0, 0, 0, 0);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[2].messages[1],
                PLC_REQ_EVSEMaxPower, 2, 2, 0x0000FFFF, 0, 0, 0, 0, 0, 0, 0);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[2].messages[2],
                PLC_REQ_EVSEMaxVolt, 4, 2, 0x0000FFFF, 0, 0, 0, 0, 0, 0, 0);

    /* GDS messageindex=4, requestvalue=15ECC005XXXXYYYY00000000 */
    g_stPLC_Config.txConfigs[3].messageIndex = 4;
    g_stPLC_Config.txConfigs[3].protocolId   = 0x0100;
    g_stPLC_Config.txConfigs[3].dataRate     = 1;
    g_stPLC_Config.txConfigs[3].canId        = 0x15ECC005;
    g_stPLC_Config.txConfigs[3].interval_ms  = 50;
    g_stPLC_Config.txConfigs[3].messageCount = 2;
    g_stPLC_Config.txConfigs[3].enabled      = true;
    memset(g_stPLC_Config.txConfigs[3].dataTemplate, 0, PLC_CAN_DATA_LEN);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[3].messages[0],
                PLC_REQ_EVSEPresentVoltage, 0, 2, 0x0000FFFF, 0, 0, 0, 0, 0, 0, 0);
    PLC_SET_MSG(g_stPLC_Config.txConfigs[3].messages[1],
                PLC_REQ_EVSEPresentCurrent, 2, 2, 0x0000FFFF, 0, 0, 0, 0, 0, 0, 0);

    g_stPLC_Config.txCount    = 4;
    g_stPLC_Config.configured = true;

    uint32_t now = osKernelGetTickCount();
    for (uint8_t i = 0; i < g_stPLC_Config.txCount; i++) {
        g_stPLC_Config.txConfigs[i].lastTxTime = now;
    }

#undef PLC_SET_MSG
#undef PLC_SET_CONV

    printf("[PLC] DB example config loaded (RX=%d, TX=%d)\r\n",
           g_stPLC_Config.rxCount, g_stPLC_Config.txCount);
}

/**
 * @brief  Load PLC step example matching the DB text.
 *         Requires plc setex (0x31/0x32 config) to be loaded first.
 */
static void CLI_PLC_SetStepExample(void)
{
    if (!g_stPLC_Config.configured) {
        printf("[PLC] Error: run 'plc setex' first (0x31/0x32 config required)\r\n");
        return;
    }

    PLC_ResetStepData();
    uint16_t idx = 0;
    uint16_t stopIdx = 0;
    stPLC_StepConfig *s;

#define STEP_MSG(s, mi, vt, cd, mn, mx)  do { \
        (s)->messages[(mi)].valueType = (vt); \
        (s)->messages[(mi)].code      = (cd); \
        (s)->messages[(mi)].minValue  = (mn); \
        (s)->messages[(mi)].maxValue  = (mx); \
    } while (0)

#define STEP_INIT(slot, no, dev, msgidx, interval, cnt) do { \
        (slot)->stepno       = (no); \
        (slot)->substep      = 0; \
        (slot)->deviceType   = (dev); \
        (slot)->messageIndex = (msgidx); \
        (slot)->interval_ms  = (interval); \
        (slot)->messageCount = (cnt); \
    } while (0)

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 1, PLC_DEVICE_TYPE_TX, 2, 50, 3);
    STEP_MSG(s, 0, PLC_REQ_ChargeControl,       0x00, 0, 0);
    STEP_MSG(s, 1, PLC_REQ_EVSEIsolationStatus, 0x00, 0, 0);
    STEP_MSG(s, 2, PLC_REQ_EVSEProcessing,      0x00, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 2, PLC_DEVICE_TYPE_RX, 1, 0, 1);
    STEP_MSG(s, 0, PLC_RES_CP, 0x00, 0x50, 0x64);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 3, PLC_DEVICE_TYPE_TX, 2, 50, 3);
    STEP_MSG(s, 0, PLC_REQ_ChargeControl,       0x01, 0, 0);
    STEP_MSG(s, 1, PLC_REQ_EVSEIsolationStatus, 0x00, 0, 0);
    STEP_MSG(s, 2, PLC_REQ_EVSEProcessing,      0x00, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 4, PLC_DEVICE_TYPE_TX, 2, 50, 3);
    STEP_MSG(s, 0, PLC_REQ_ChargeControl,       0x01, 0, 0);
    STEP_MSG(s, 1, PLC_REQ_EVSEIsolationStatus, 0x00, 0, 0);
    STEP_MSG(s, 2, PLC_REQ_EVSEProcessing,      0x01, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 5, PLC_DEVICE_TYPE_TX, 1, 0, 1);
    STEP_MSG(s, 0, PLC_REQ_DCextd, 0x08, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 6, PLC_DEVICE_TYPE_TX, 2, 50, 3);
    STEP_MSG(s, 0, PLC_REQ_ChargeControl,       0x02, 0, 0);
    STEP_MSG(s, 1, PLC_REQ_EVSEIsolationStatus, 0x00, 0, 0);
    STEP_MSG(s, 2, PLC_REQ_EVSEProcessing,      0x01, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 7, PLC_DEVICE_TYPE_RX, 3, 0, 1);
    STEP_MSG(s, 0, PLC_RES_SECC_STATUS, 0x5A, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 8, PLC_DEVICE_TYPE_TX, 3, 0, 3);
    STEP_MSG(s, 0, PLC_REQ_EVSEMaxCurrent, 0x5802, 0, 0);
    STEP_MSG(s, 1, PLC_REQ_EVSEMaxPower,   0x983A, 0, 0);
    STEP_MSG(s, 2, PLC_REQ_EVSEMaxVolt,    0x4020, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 9, PLC_DEVICE_TYPE_TX, 2, 50, 3);
    STEP_MSG(s, 0, PLC_REQ_ChargeControl,       0x02, 0, 0);
    STEP_MSG(s, 1, PLC_REQ_EVSEIsolationStatus, 0x00, 0, 0);
    STEP_MSG(s, 2, PLC_REQ_EVSEProcessing,      0x03, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 10, PLC_DEVICE_TYPE_RX, 3, 0, 1);
    STEP_MSG(s, 0, PLC_RES_SECC_STATUS, 0x64, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 11, PLC_DEVICE_TYPE_TX, 2, 50, 3);
    STEP_MSG(s, 0, PLC_REQ_ChargeControl,       0x02, 0, 0);
    STEP_MSG(s, 1, PLC_REQ_EVSEIsolationStatus, 0x01, 0, 0);
    STEP_MSG(s, 2, PLC_REQ_EVSEProcessing,      0x07, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 12, PLC_DEVICE_TYPE_RX, 3, 0, 1);
    STEP_MSG(s, 0, PLC_RES_SECC_STATUS, 0x65, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 13, PLC_DEVICE_TYPE_RX, 2, 0, 2);
    STEP_MSG(s, 0, PLC_RES_TARGET_VOLTAGE, 0xFFFF, 0, 0);
    STEP_MSG(s, 1, PLC_RES_TARGET_CURRENT, 0xFFFF, 0, 0);

    s = &g_stPLC_StepData.steps[idx++];
    STEP_INIT(s, 14, PLC_DEVICE_TYPE_TX, 4, 50, 2);
    STEP_MSG(s, 0, PLC_REQ_EVSEPresentVoltage, 0xFFFF, 0, 0);
    STEP_MSG(s, 1, PLC_REQ_EVSEPresentCurrent, 0xFFFF, 0, 0);

    s = &g_stPLC_StopStepData.steps[stopIdx++];
    STEP_INIT(s, PLC_STOPSTEP_NO, PLC_DEVICE_TYPE_TX, 2, 50, 3);
    STEP_MSG(s, 0, PLC_REQ_ChargeControl,       0x03, 0, 0);
    STEP_MSG(s, 1, PLC_REQ_EVSEIsolationStatus, 0x01, 0, 0);
    STEP_MSG(s, 2, PLC_REQ_EVSEProcessing,      0x07, 0, 0);

#undef STEP_INIT
#undef STEP_MSG

    g_stPLC_StepData.count          = idx;
    g_stPLC_StepData.configured     = true;
    g_stPLC_StopStepData.count      = stopIdx;
    g_stPLC_StopStepData.configured = true;

    printf("[PLC] DB step example loaded (%d entries, stop=%d)\r\n", idx, stopIdx);
}

void CLI_PLC_Cmd(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "show") == 0) {
        CLI_PLC_Show();
    } else if (argc >= 2 && strcmp(argv[1], "1") == 0) {
        IO_EXT_RLY_control(1, true);
    } else if (argc >= 2 && strcmp(argv[1], "2") == 0) {
        IO_EXT_RLY_control(2, true);
    } else if (argc >= 2 && strcmp(argv[1], "3") == 0) {
        IO_EXT_RLY_control(3, true);
    } else if (argc >= 2 && strcmp(argv[1], "4") == 0) {
        IO_EXT_RLY_control(4, true);
    } else if (argc >= 2 && strcmp(argv[1], "ac") == 0) {
        IO_AD_RELAY_control(0, 1, true);
    } else if (argc >= 2 && strcmp(argv[1], "s1") == 0) {
        IO_SS_control(1, true);
    } else if (argc >= 2 && strcmp(argv[1], "s2") == 0) {
        IO_SS_control(2, true);
    } else if (argc >= 2 && strcmp(argv[1], "s3") == 0) {
        IO_SS_control(3, true);
    } else if (argc >= 2 && strcmp(argv[1], "s4") == 0) {
        IO_SS_control(4, true);
    } else if (argc >= 2 && strcmp(argv[1], "s5") == 0) {
        IO_SS_control(5, true);
    } else if (argc >= 2 && strcmp(argv[1], "s6") == 0) {
        IO_SS_control(6, true);
    } else if (argc >= 2 && strcmp(argv[1], "s7") == 0) {
        IO_SS_control(7, true);
    } else if (argc >= 2 && strcmp(argv[1], "s8") == 0) {
        IO_SS_control(8, true);
    } else if (argc >= 2 && strcmp(argv[1], "33") == 0) {
        IO_EXT_RLY_control(4, true);
    } else if (argc >= 2 && strcmp(argv[1], "rx") == 0) {
        CLI_PLC_ShowRx();
    } else if (argc == 4 && strcmp(argv[1], "set") == 0) {
        extern volatile bool g_plcManualPV;
        uint8_t  type = (uint8_t)strtoul(argv[2], NULL, 0);
        uint32_t val  = strtoul(argv[3], NULL, 0);

        /* g_stPLC_TxValues에 값 저장 (PLC_ProcessTx에서 자동 반영) */
        switch (type) {
            case PLC_REQ_DCextd:              g_stPLC_TxValues.dcExtd              = (uint8_t)val;  break;
            case PLC_REQ_Heartbeat:           g_stPLC_TxValues.heartbeat           = (uint8_t)val;  break;
            case PLC_REQ_ChargeControl:       g_stPLC_TxValues.chargingControl     = (uint8_t)val;  break;
            case PLC_REQ_EVSEIsolationStatus: g_stPLC_TxValues.evseIsolationStatus = (uint8_t)val;  break;
            case PLC_REQ_EVSEProcessing:      g_stPLC_TxValues.evseProcessing      = (uint8_t)val;  break;
            case PLC_REQ_EVSEMaxCurrent:      g_stPLC_TxValues.evseMaxCurrent      = (uint16_t)val; break;
            case PLC_REQ_EVSEMaxPower:        g_stPLC_TxValues.evseMaxPower        = (uint16_t)val; break;
            case PLC_REQ_EVSEMaxVolt:         g_stPLC_TxValues.evseMaxVolt         = (uint16_t)val; break;
            case PLC_REQ_EVSEPresentVoltage:
                g_stPLC_TxValues.evsePresentVoltage  = (uint16_t)val;
                g_plcManualPV = true;
                break;
            case PLC_REQ_EVSEPresentCurrent:
                g_stPLC_TxValues.evsePresentCurrent  = (uint16_t)val;
                g_plcManualPV = true;
                break;
            default:
                printf("[PLC] Unknown type: %d\r\n", type);
                return;
        }
        printf("[PLC] TX set type=%d(%s) val=%lu (0x%lX)\r\n", type, CLI_PLC_ReqTypeStr(type), val, val);
    } else if (argc >= 2 && strcmp(argv[1], "tx") == 0) {
        /* TX 동적 값 현황 표시 */
        printf("[PLC TX Values]\r\n");
        printf("  DCextd           = 0x%02X\r\n", g_stPLC_TxValues.dcExtd);
        printf("  Heartbeat        = 0x%02X\r\n", g_stPLC_TxValues.heartbeat);
        printf("  ChargingControl  = 0x%02X\r\n", g_stPLC_TxValues.chargingControl);
        printf("  IsolationStatus  = 0x%02X\r\n", g_stPLC_TxValues.evseIsolationStatus);
        printf("  Processing       = 0x%02X\r\n", g_stPLC_TxValues.evseProcessing);
        printf("  MaxCurrent       = %u\r\n",     g_stPLC_TxValues.evseMaxCurrent);
        printf("  MaxPower         = %u\r\n",     g_stPLC_TxValues.evseMaxPower);
        printf("  MaxVolt          = %u\r\n",     g_stPLC_TxValues.evseMaxVolt);
        printf("  PresentVoltage   = %u\r\n",     g_stPLC_TxValues.evsePresentVoltage);
        printf("  PresentCurrent   = %u\r\n",     g_stPLC_TxValues.evsePresentCurrent);
    } else if (argc >= 2 && strcmp(argv[1], "dcextd") == 0) {
        /* DCextd 트리거: toggle 0x00 ↔ 0x08 */
        if (argc >= 3) {
            g_stPLC_TxValues.dcExtd = (uint8_t)strtoul(argv[2], NULL, 0);
        } else {
            g_stPLC_TxValues.dcExtd = (g_stPLC_TxValues.dcExtd == 0x00) ? 0x08 : 0x00;
        }
        printf("[PLC] DCextd = 0x%02X\r\n", g_stPLC_TxValues.dcExtd);
    } else if (argc >= 2 && strcmp(argv[1], "pv") == 0) {
        /* PresentVoltage 수동 설정 */
        extern volatile bool g_plcManualPV;
        if (argc >= 3) {
            uint16_t v = (uint16_t)strtoul(argv[2], NULL, 0);
            g_stPLC_TxValues.evsePresentVoltage = v;
            g_plcManualPV = true;
            printf("[PLC] PresentVoltage = 0x%04X (%u) [manual]\r\n", v, v);
        } else {
            printf("[PLC] PresentVoltage = 0x%04X (%u) [%s]\r\n",
                   g_stPLC_TxValues.evsePresentVoltage, g_stPLC_TxValues.evsePresentVoltage,
                   g_plcManualPV ? "manual" : "auto");
        }
    } else if (argc >= 2 && strcmp(argv[1], "pc") == 0) {
        /* PresentCurrent 수동 설정 */
        extern volatile bool g_plcManualPV;
        if (argc >= 3) {
            uint16_t v = (uint16_t)strtoul(argv[2], NULL, 0);
            g_stPLC_TxValues.evsePresentCurrent = v;
            g_plcManualPV = true;
            printf("[PLC] PresentCurrent = 0x%04X (%u) [manual]\r\n", v, v);
        } else {
            printf("[PLC] PresentCurrent = 0x%04X (%u) [%s]\r\n",
                   g_stPLC_TxValues.evsePresentCurrent, g_stPLC_TxValues.evsePresentCurrent,
                   g_plcManualPV ? "manual" : "auto");
        }
    } else if (argc >= 2 && strcmp(argv[1], "pvauto") == 0) {
        /* RX pass-through 모드 복귀 */
        extern volatile bool g_plcManualPV;
        g_plcManualPV = false;
        printf("[PLC] PresentVoltage/Current → auto (RX pass-through)\r\n");
    } else if (argc >= 2 && strcmp(argv[1], "setex") == 0) {
        CLI_PLC_SetExample();
    } else if (argc >= 2 && strcmp(argv[1], "stepex") == 0) {
        CLI_PLC_SetStepExample();
    } else if (argc >= 2 && strcmp(argv[1], "stepstart") == 0) {
        g_system_mode = eMODE_VEHICLE_DISCHARGE;
        if (HAL_GPIO_ReadPin(CAN2_SW_EN_GPIO_Port, CAN2_SW_EN_Pin) == GPIO_PIN_RESET) {
            IO_CONTROL_HIGH(CAN2_SW_EN);
            printf("[PLC] CAN2_SW_EN → HIGH\r\n");
        }
        g_plcManualRun = true;
        PLC_StepSequenceStart();
        printf("[PLC] mode=VEHICLE_DISCHARGE, manualRun=ON\r\n");
    } else if (argc >= 2 && strcmp(argv[1], "stepstart2") == 0) {
        g_system_mode = eMODE_VEHICLE_CHARGE;
        if (HAL_GPIO_ReadPin(CAN2_SW_EN_GPIO_Port, CAN2_SW_EN_Pin) == GPIO_PIN_RESET) {
            IO_CONTROL_HIGH(CAN2_SW_EN);
            printf("[PLC] CAN2_SW_EN → HIGH\r\n");
        }
        g_plcManualRun = true;
        PLC_StepSequenceStart();
        printf("[PLC] mode=VEHICLE_DISCHARGE, manualRun=ON\r\n");
    } else if (argc >= 2 && strcmp(argv[1], "stepstop") == 0) {
        PLC_StepSequenceStop();
    } else if (argc >= 2 && strcmp(argv[1], "devstop") == 0) {
        g_plcStepWaitStop = true;
        printf("[PLC] STOP trigger sent (step will advance from WAIT_STOP)\r\n");
    } else if (argc >= 2 && strcmp(argv[1], "stopstep") == 0) {
        /* BLE 0x33 step config 시 stepno=99로 등록된 정지 시퀀스의 TX 코드값을
         * g_stPLC_TxValues 에 일괄 주입 → 다음 PLC_ProcessTx 주기에 PLC CAN(FDCAN2)으로 송신.
         * 실제 운영에서는 BLE 0x91 STOP(VEHICLE_DISCHARGE)에서 자동 호출됨.
         * 본 CLI는 수동 테스트용. */
        printf("[PLC] stopstep: applying step=99 TX values\r\n");
        printf("  configured=%d entries=%d\r\n",
               (int)g_stPLC_StopStepData.configured, g_stPLC_StopStepData.count);
        for (uint16_t i = 0; i < g_stPLC_StopStepData.count; i++) {
            stPLC_StepConfig *s = &g_stPLC_StopStepData.steps[i];
            const char *dt = (s->deviceType == PLC_DEVICE_TYPE_TX) ? "TX" : "RX";
            printf("  Step%2d.%d [%s] msgIdx=%d msgs=%d\r\n",
                   s->stepno, s->substep, dt, s->messageIndex, s->messageCount);
            for (uint8_t m = 0; m < s->messageCount; m++) {
                stPLC_StepMsg *msg = &s->messages[m];
                printf("    msg[%d] type=%d  code=0x%04X\r\n",
                       m, msg->valueType, msg->code);
            }
        }
        PLC_ApplyStopSteps();
    } else if (argc >= 2 && strcmp(argv[1], "stepshow") == 0) {
        printf("[Step] running=%d  currentStep=%d.%d  txApplied=%d  totalEntries=%d\r\n",
               (int)g_plcStepRunning, g_plcCurrentStep, g_plcCurrentSubStep,
               (int)g_plcStepTxApplied, g_stPLC_StepData.count);
        for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
            stPLC_StepConfig *s = &g_stPLC_StepData.steps[i];
            const char *dt = (s->deviceType == PLC_DEVICE_TYPE_TX) ? "TX" : "RX";
            printf("  Step%2d.%d [%s] msgIdx=%d interval=%dms msgs=%d\r\n",
                   s->stepno, s->substep, dt, s->messageIndex, s->interval_ms, s->messageCount);
            for (uint8_t m = 0; m < s->messageCount; m++) {
                stPLC_StepMsg *msg = &s->messages[m];
                if (s->deviceType == PLC_DEVICE_TYPE_RX) {
                    if (msg->code != 0)
                        printf("    msg[%d] type=%d  ==0x%04X\r\n",
                               m, msg->valueType, msg->code);
                    else
                        printf("    msg[%d] type=%d  [0x%04X~0x%04X]\r\n",
                               m, msg->valueType,
                               msg->minValue, msg->maxValue);
                } else {
                    printf("    msg[%d] type=%d  code=0x%04X\r\n",
                           m, msg->valueType, msg->code);
                }
            }
        }
    } else if (argc == 4 && strcmp(argv[1], "setrx") == 0) {
        uint8_t  type = (uint8_t)strtoul(argv[2], NULL, 0);
        uint32_t val  = strtoul(argv[3], NULL, 0);
        switch (type) {
            case 1: g_stPLC_RxValues.cp            = (uint8_t)val;  break;
            case 2: g_stPLC_RxValues.targetVoltage = (uint16_t)val; break;
            case 3: g_stPLC_RxValues.targetCurrent = (uint16_t)val; break;
            case 4: g_stPLC_RxValues.seccStatus    = (uint8_t)val;  break;
            default: printf("[PLC] setrx: unknown type %d\r\n", type); return;
        }
        printf("[PLC] setrx type=%d val=0x%lX\r\n", type, val);
    } else if (argc == 3 && strcmp(argv[1], "step") == 0) {
        uint16_t n = (uint16_t)strtoul(argv[2], NULL, 0);
        g_plcCurrentStep    = n;
        g_plcCurrentSubStep = 0;
        g_plcStepTxApplied  = false;
        printf("[Step] jumped to step %d.0\r\n", n);
    } else if (argc >= 2 && strcmp(argv[1], "clear") == 0) {
        PLC_ResetConfig();
    } else if (argc >= 2 && strcmp(argv[1], "start") == 0) {
        g_system_mode = eMODE_VEHICLE_DISCHARGE;
        if (HAL_GPIO_ReadPin(CAN2_SW_EN_GPIO_Port, CAN2_SW_EN_Pin) == GPIO_PIN_RESET) {
            IO_CONTROL_HIGH(CAN2_SW_EN);
            printf("[PLC] CAN2_SW_EN → HIGH\r\n");
        }
        g_plcManualRun = true;
        printf("[PLC] Manual run STARTED (mode=VEHICLE_DISCHARGE)\r\n");
    } else if (argc >= 2 && strcmp(argv[1], "stop") == 0) {
        g_plcManualRun = false;
        printf("[PLC] Manual run STOPPED\r\n");
    } else if (argc >= 2 && strcmp(argv[1], "reboot") == 0) {
        PLC_SendReboot();
    } else {
        printf("usage:\r\n");
        printf("  plc start              : start RX/TX processing\r\n");
        printf("  plc stop               : stop RX/TX processing\r\n");
        printf("  plc show               : show config\r\n");
        printf("  plc rx                 : show RX values\r\n");
        printf("  plc tx                 : show TX values\r\n");
        printf("  plc set <type> <val>   : set TX value\r\n");
        printf("    types: 1=DCextd 2=Heartbeat 3=ChargingCtrl\r\n");
        printf("           4=IsolationSta 5=Processing\r\n");
        printf("           6=MaxCurrent 7=MaxPower 8=MaxVolt\r\n");
        printf("           9=PresentVolt 10=PresentCurr\r\n");
        printf("  plc dcextd [val]       : toggle/set DCextd (0x00/0x08)\r\n");
        printf("  plc pv [val]           : set/show PresentVoltage (manual)\r\n");
        printf("  plc pc [val]           : set/show PresentCurrent (manual)\r\n");
        printf("  plc pvauto             : restore RX pass-through mode\r\n");
        printf("  plc setex              : load example config (0x31/0x32)\r\n");
        printf("  plc stepex             : load example step data (0x33)\r\n");
        printf("  plc stepstart          : start step sequence from step 1\r\n");
        printf("  plc stepstop           : stop step sequence\r\n");
        printf("  plc devstop            : send STOP trigger (advance from WAIT_STOP)\r\n");
        printf("  plc stopstep           : apply step=99 stop-step TX values (manual)\r\n");
        printf("  plc stepshow           : show step data & current state\r\n");
        printf("  plc step <n>           : jump to step n\r\n");
        printf("  plc setrx <type> <val> : simulate RX value\r\n");
        printf("    types: 1=CP 2=TargetVolt 3=TargetCurr 4=SECCStatus\r\n");
        printf("  plc clear              : reset all config\r\n");
        printf("  plc reboot             : send PLC reboot command (FDCAN2)\r\n");
    }
}
