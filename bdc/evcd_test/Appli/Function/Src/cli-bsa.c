/**
 * @file  cli-bsa.c
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
 *  BSA Config CLI Commands
 *  - bsa show       : Display current DataConfig / SimframeConfig / MonData
 *  - bsa setmon 1|2 : Set NE_type monitoring config
 *  - bsa setsim 1~4 : Set NE_type simframe config
 *  - bsa clear      : Reset all BSA configs
 *  - bsa start      : Start BSA CAN TX/RX using loaded configs
 *  - bsa stop       : Stop BSA CAN TX/RX (configs preserved)
 *  - bsa status     : Show current running state
 *--------------------------------------------------------------------*/
static void CLI_BSA_Show(void)
{
    uint8_t i, j;

    printf("=== DataConfig (Monitoring) [%d configs] ===\r\n", g_ucBSA_DataConfigCount);
    printf("  Ready: %s\r\n", g_bBSA_DataConfigReady ? "YES" : "NO");
    for (j = 0; j < g_ucBSA_DataConfigCount; j++) {
        stBSA_DataConfig *dc = &g_stBSA_DataConfigs[j];
        printf(" [Config %d] Proto=0x%04X(%s) nDataRate=%d RxCAN_ID=0x%04X MsgCnt=%d\r\n",
               j, dc->protocolId,
               (dc->protocolId == BSA_PROTOCOL_CAN_FD) ? "FD" : "Classic",
               dc->dataRate, dc->responseValue, dc->messageCount);
        for (i = 0; i < dc->messageCount; i++) {
            stBSA_DataConfigMsg *m = &dc->messages[i];
            printf("  [%d] vType=%d pos=%d sz=%d mask=0x%08lX conv(%d A=%d B=%d C=%d D=%d E=%d F=%d)\r\n",
                   i, m->valueType, m->startPosition, m->dataSize, m->maskingValue,
                   m->convRule.convType, m->convRule.convA, m->convRule.convB,
                   m->convRule.convC, m->convRule.convD, m->convRule.convE, m->convRule.convF);
        }
    }

    printf("=== SimframeConfig [%d configs] ===\r\n", g_ucBSA_SimframeConfigCount);
    printf("  Ready: %s\r\n", g_bBSA_SimframeConfigReady ? "YES" : "NO");
    for (j = 0; j < g_ucBSA_SimframeConfigCount; j++) {
        stBSA_SimframeConfig *sc = &g_stBSA_SimframeConfigs[j];
        printf(" [Config %d] ProtocolID=0x%04X nDataRate=%d ReqLen=%d Interval=%dms MsgCnt=%d\r\n",
               j, sc->protocolId, sc->dataRate,
               sc->requestValueLen, sc->interval_ms, sc->messageCount);
        printf("  ReqVal: ");
        for (i = 0; i < sc->requestValueLen; i++) {
            printf("%02X ", sc->requestValue[i]);
        }
        printf("\r\n");
        for (i = 0; i < sc->messageCount; i++) {
            stBSA_DataConfigMsg *m = &sc->messages[i];
            printf("  [%d] vType=%d pos=%d sz=%d mask=0x%08lX conv(%d A=%d B=%d C=%d D=%d E=%d F=%d)\r\n",
                   i, m->valueType, m->startPosition, m->dataSize, m->maskingValue,
                   m->convRule.convType, m->convRule.convA, m->convRule.convB,
                   m->convRule.convC, m->convRule.convD, m->convRule.convE, m->convRule.convF);
        }
    }

    printf("=== MonData ===\r\n");
    printf("  Voltage=%u PreRly=%u MainRly=%u IGON=%u\r\n",
           g_stBSA_MonData.mosa_BSA_voltage, g_stBSA_MonData.mosa_PreRly_status,
           g_stBSA_MonData.mosa_MainRly_status, g_stBSA_MonData.mosa_IGON_status);
}

static void CLI_BSA_SetMon(int index)
{
    memset(&g_stBSA_DataConfigs[0], 0, sizeof(g_stBSA_DataConfigs[0]));
    g_stBSA_DataConfigs[0].protocolId = BSA_PROTOCOL_CAN_FD;  // 0x0130 = CAN FD
    g_stBSA_DataConfigs[0].dataRate   = 6;                     // ndatarate 6 = 500K/2M

    switch (index) {
    case 1:
        // monitoring 1: responseValue=0x02FA, MainRelay
        g_stBSA_DataConfigs[0].responseValue = 0x02FA;
        g_stBSA_DataConfigs[0].messageCount  = 1;
        g_stBSA_DataConfigs[0].messages[0].valueType     = BSA_VALUETYPE_MON_MAIN_RLY;
        g_stBSA_DataConfigs[0].messages[0].startPosition = 3;
        g_stBSA_DataConfigs[0].messages[0].dataSize      = 1;
        g_stBSA_DataConfigs[0].messages[0].maskingValue  = 0x000000FF;
        g_stBSA_DataConfigs[0].messages[0].convRule.convType = 0;
        break;

    case 2:
        // monitoring 2: responseValue=0x0235, PreChrgSta + BattpckVoltVal
        g_stBSA_DataConfigs[0].responseValue = 0x0235;
        g_stBSA_DataConfigs[0].messageCount  = 2;
        // PreChrgSta
        g_stBSA_DataConfigs[0].messages[0].valueType     = BSA_VALUETYPE_MON_PRECHARGE_RLY;
        g_stBSA_DataConfigs[0].messages[0].startPosition = 3;
        g_stBSA_DataConfigs[0].messages[0].dataSize      = 1;
        g_stBSA_DataConfigs[0].messages[0].maskingValue  = 0x000000FF;
        g_stBSA_DataConfigs[0].messages[0].convRule.convType = 0;
        // BattpckVoltVal (convType=1, A=1, B=0 → returns raw)
        g_stBSA_DataConfigs[0].messages[1].valueType     = BSA_VALUETYPE_MON_VOLTAGE;
        g_stBSA_DataConfigs[0].messages[1].startPosition = 13;
        g_stBSA_DataConfigs[0].messages[1].dataSize      = 2;
        g_stBSA_DataConfigs[0].messages[1].maskingValue  = 0x0000FFFF;
        g_stBSA_DataConfigs[0].messages[1].convRule.convType = 1;
        g_stBSA_DataConfigs[0].messages[1].convRule.convA    = 1;
        g_stBSA_DataConfigs[0].messages[1].convRule.convF    = 1;
        break;

    default:
        printf("[BSA] Invalid mon index (1-2)\r\n");
        return;
    }

    g_ucBSA_DataConfigCount = 1;
    g_bBSA_DataConfigReady = true;
    printf("[BSA] Monitoring %d set (Proto=0x%04X, RxCAN_ID=0x%04X, Rate=%d, %d msgs)\r\n",
           index, g_stBSA_DataConfigs[0].protocolId,
           g_stBSA_DataConfigs[0].responseValue, g_stBSA_DataConfigs[0].dataRate,
           g_stBSA_DataConfigs[0].messageCount);
}

static void CLI_BSA_SetSim(int index)
{
    memset(&g_stBSA_SimframeConfigs[0], 0, sizeof(g_stBSA_SimframeConfigs[0]));
    g_stBSA_SimframeConfigs[0].protocolId      = BSA_PROTOCOL_CAN_FD;  // 0x0130 = CAN FD
    g_stBSA_SimframeConfigs[0].dataRate        = 6;                     // ndatarate 6 = 500K/2M
    g_stBSA_SimframeConfigs[0].requestValueLen = BSA_REQVAL_CANID_SIZE + 32;  // CAN_ID(2) + CAN_DATA(32) = 34

    // Common message definitions for NE_type simframes
    // startPosition is relative to CAN DATA (requestValue+2)
    // CRC16: pos=0, sz=2
    stBSA_DataConfigMsg msgCRC;
    memset(&msgCRC, 0, sizeof(msgCRC));
    msgCRC.valueType     = BSA_VALUETYPE_SIM_CRC16;
    msgCRC.startPosition = 0;
    msgCRC.dataSize      = 2;
    msgCRC.maskingValue  = 0x0000FFFF;

    // Alive Counter: pos=2, sz=1
    stBSA_DataConfigMsg msgAC;
    memset(&msgAC, 0, sizeof(msgAC));
    msgAC.valueType     = BSA_VALUETYPE_SIM_ALIVE_COUNT;
    msgAC.startPosition = 2;
    msgAC.dataSize      = 1;
    msgAC.maskingValue  = 0x000000FF;

    // Voltage: pos=16, sz=2 (convType=1, A=1, B=0, F=5)
    stBSA_DataConfigMsg msgVolt;
    memset(&msgVolt, 0, sizeof(msgVolt));
    msgVolt.valueType          = BSA_VALUETYPE_SIM_VOLTAGE;
    msgVolt.startPosition      = 16;
    msgVolt.dataSize           = 2;
    msgVolt.maskingValue       = 0x0000FFFF;
    msgVolt.convRule.convType  = 1;
    msgVolt.convRule.convA     = 1;
    msgVolt.convRule.convF     = 5;

    // requestValue format: [CAN_ID_LO, CAN_ID_HI, CAN_DATA[0], CAN_DATA[1], ...]
    uint16_t canId = 0;

    switch (index) {
    case 1:
        // CAN ID 0x010A, 10ms, CRC+AC+Voltage
        // CAN DATA template: [7]=0x01 (rest 0x00)
        canId = 0x010A;
        g_stBSA_SimframeConfigs[0].requestValue[0] = (uint8_t)(canId & 0xFF);        // 0x0A
        g_stBSA_SimframeConfigs[0].requestValue[1] = (uint8_t)((canId >> 8) & 0xFF); // 0x01
        g_stBSA_SimframeConfigs[0].requestValue[BSA_REQVAL_CANID_SIZE + 7] = 0x01;   // CAN_DATA[7]

        g_stBSA_SimframeConfigs[0].interval_ms  = 10;
        g_stBSA_SimframeConfigs[0].messageCount = 3;
        g_stBSA_SimframeConfigs[0].messages[0]  = msgCRC;
        g_stBSA_SimframeConfigs[0].messages[1]  = msgAC;
        g_stBSA_SimframeConfigs[0].messages[2]  = msgVolt;
        break;

    case 2:
        // CAN ID 0x0120, 10ms, CRC+AC+Voltage
        // CAN DATA template: [7]=0x01
        canId = 0x0120;
        g_stBSA_SimframeConfigs[0].requestValue[0] = (uint8_t)(canId & 0xFF);        // 0x20
        g_stBSA_SimframeConfigs[0].requestValue[1] = (uint8_t)((canId >> 8) & 0xFF); // 0x01
        g_stBSA_SimframeConfigs[0].requestValue[BSA_REQVAL_CANID_SIZE + 7] = 0x01;   // CAN_DATA[7]

        g_stBSA_SimframeConfigs[0].interval_ms  = 10;
        g_stBSA_SimframeConfigs[0].messageCount = 3;
        g_stBSA_SimframeConfigs[0].messages[0]  = msgCRC;
        g_stBSA_SimframeConfigs[0].messages[1]  = msgAC;
        g_stBSA_SimframeConfigs[0].messages[2]  = msgVolt;
        break;

    case 3:
        // CAN ID 0x0035, 10ms, CRC+AC (no voltage)
        // CAN DATA template: [3]=0x41, [4]=0x01
        canId = 0x0035;
        g_stBSA_SimframeConfigs[0].requestValue[0] = (uint8_t)(canId & 0xFF);        // 0x35
        g_stBSA_SimframeConfigs[0].requestValue[1] = (uint8_t)((canId >> 8) & 0xFF); // 0x00
        g_stBSA_SimframeConfigs[0].requestValue[BSA_REQVAL_CANID_SIZE + 3] = 0x41;   // CAN_DATA[3]
        g_stBSA_SimframeConfigs[0].requestValue[BSA_REQVAL_CANID_SIZE + 4] = 0x01;   // CAN_DATA[4]

        g_stBSA_SimframeConfigs[0].interval_ms  = 10;
        g_stBSA_SimframeConfigs[0].messageCount = 2;
        g_stBSA_SimframeConfigs[0].messages[0]  = msgCRC;
        g_stBSA_SimframeConfigs[0].messages[1]  = msgAC;
        break;

    case 4:
        // CAN ID 0x02AA, 100ms, CRC+AC (no voltage)
        // CAN DATA template: all 0x00
        canId = 0x02AA;
        g_stBSA_SimframeConfigs[0].requestValue[0] = (uint8_t)(canId & 0xFF);        // 0xAA
        g_stBSA_SimframeConfigs[0].requestValue[1] = (uint8_t)((canId >> 8) & 0xFF); // 0x02

        g_stBSA_SimframeConfigs[0].interval_ms  = 100;
        g_stBSA_SimframeConfigs[0].messageCount = 2;
        g_stBSA_SimframeConfigs[0].messages[0]  = msgCRC;
        g_stBSA_SimframeConfigs[0].messages[1]  = msgAC;
        break;

    default:
        printf("[BSA] Invalid sim index (1-4)\r\n");
        return;
    }

    g_ucBSA_SimframeConfigCount = 1;
    g_bBSA_SimframeConfigReady = true;
    BSA_ResetSimframeState();
    printf("[BSA] Simframe %d set (Proto=0x%04X, CAN_ID=0x%04X, Rate=%d, ReqLen=%d, %dms, %d msgs)\r\n",
           index, g_stBSA_SimframeConfigs[0].protocolId, canId,
           g_stBSA_SimframeConfigs[0].dataRate, g_stBSA_SimframeConfigs[0].requestValueLen,
           g_stBSA_SimframeConfigs[0].interval_ms, g_stBSA_SimframeConfigs[0].messageCount);
}

/**
 * @brief  BSA 예시 전체 세팅 (BLE config과 동일)
 *         Monitoring 2개 + Simframe 4개 한번에 설정
 */
static void CLI_BSA_SetExample(void)
{
    /* ── Clear ── */
    memset(g_stBSA_DataConfigs, 0, sizeof(g_stBSA_DataConfigs));
    memset(g_stBSA_SimframeConfigs, 0, sizeof(g_stBSA_SimframeConfigs));
    memset(g_SimframeAliveCnts, 0, sizeof(g_SimframeAliveCnts));
    memset(g_SimframeTxTimers, 0, sizeof(g_SimframeTxTimers));

    /* ================================================================
     *  Monitoring Config (FuncID 0x21) — 2개
     * ================================================================ */

    /* [0] RxCAN_ID=0x02FA, 1 msg: MainRelay (type=0, pos=3, sz=1) */
    g_stBSA_DataConfigs[0].protocolId    = BSA_PROTOCOL_CAN_FD;
    g_stBSA_DataConfigs[0].dataRate      = 6;
    g_stBSA_DataConfigs[0].responseValue = 0x02FA;
    g_stBSA_DataConfigs[0].messageCount  = 1;
    g_stBSA_DataConfigs[0].messages[0].valueType     = 0;
    g_stBSA_DataConfigs[0].messages[0].startPosition = 3;
    g_stBSA_DataConfigs[0].messages[0].dataSize      = 1;
    g_stBSA_DataConfigs[0].messages[0].maskingValue  = 0x000000FF;
    g_stBSA_DataConfigs[0].messages[0].convRule.convType = 0;

    /* [1] RxCAN_ID=0x0235, 2 msgs: PreChrgSta + BattpckVoltVal */
    g_stBSA_DataConfigs[1].protocolId    = BSA_PROTOCOL_CAN_FD;
    g_stBSA_DataConfigs[1].dataRate      = 6;
    g_stBSA_DataConfigs[1].responseValue = 0x0235;
    g_stBSA_DataConfigs[1].messageCount  = 2;
    // msg[0]: type=0, pos=3, sz=1
    g_stBSA_DataConfigs[1].messages[0].valueType     = 0;
    g_stBSA_DataConfigs[1].messages[0].startPosition = 3;
    g_stBSA_DataConfigs[1].messages[0].dataSize      = 1;
    g_stBSA_DataConfigs[1].messages[0].maskingValue  = 0x000000FF;
    g_stBSA_DataConfigs[1].messages[0].convRule.convType = 0;
    // msg[1]: type=0, pos=13, sz=2, conv(1: A=1, F=1)
    g_stBSA_DataConfigs[1].messages[1].valueType     = 0;
    g_stBSA_DataConfigs[1].messages[1].startPosition = 13;
    g_stBSA_DataConfigs[1].messages[1].dataSize      = 2;
    g_stBSA_DataConfigs[1].messages[1].maskingValue  = 0x0000FFFF;
    g_stBSA_DataConfigs[1].messages[1].convRule.convType = 1;
    g_stBSA_DataConfigs[1].messages[1].convRule.convA    = 0x0001;
    g_stBSA_DataConfigs[1].messages[1].convRule.convF    = 0x01;

    g_ucBSA_DataConfigCount = 2;
    g_bBSA_DataConfigReady  = true;

    printf("[BSA] Monitoring: 2 configs set\r\n");
    printf("  [0] RxCAN_ID=0x02FA, 1 msg\r\n");
    printf("  [1] RxCAN_ID=0x0235, 2 msgs\r\n");

    /* ================================================================
     *  Simframe Config (FuncID 0x22) — 4개
     * ================================================================ */

    /* Common message templates */
    stBSA_DataConfigMsg msgCRC;
    memset(&msgCRC, 0, sizeof(msgCRC));
    msgCRC.valueType     = BSA_VALUETYPE_SIM_CRC16;       // 1
    msgCRC.startPosition = 0;
    msgCRC.dataSize      = 2;
    msgCRC.maskingValue  = 0x0000FFFF;

    stBSA_DataConfigMsg msgAC;
    memset(&msgAC, 0, sizeof(msgAC));
    msgAC.valueType     = BSA_VALUETYPE_SIM_ALIVE_COUNT;  // 2
    msgAC.startPosition = 2;
    msgAC.dataSize      = 1;
    msgAC.maskingValue  = 0x000000FF;

    stBSA_DataConfigMsg msgVolt;
    memset(&msgVolt, 0, sizeof(msgVolt));
    msgVolt.valueType          = BSA_VALUETYPE_SIM_VOLTAGE;  // 3
    msgVolt.startPosition      = 16;
    msgVolt.dataSize           = 2;
    msgVolt.maskingValue       = 0x0000FFFF;
    msgVolt.convRule.convType  = 1;
    msgVolt.convRule.convA     = 0x0001;
    msgVolt.convRule.convF     = 0x05;

    uint8_t reqValLen = BSA_REQVAL_CANID_SIZE + 32;  // CAN_ID(2) + CAN_DATA(32) = 34

    /* [0] CAN_ID=0x010A, 10ms, CRC+AC+Voltage, DATA[7]=0x01 */
    g_stBSA_SimframeConfigs[0].protocolId      = BSA_PROTOCOL_CAN_FD;
    g_stBSA_SimframeConfigs[0].dataRate        = 6;
    g_stBSA_SimframeConfigs[0].requestValueLen = reqValLen;
    g_stBSA_SimframeConfigs[0].requestValue[0] = 0x0A;  // CAN_ID LO
    g_stBSA_SimframeConfigs[0].requestValue[1] = 0x01;  // CAN_ID HI
    g_stBSA_SimframeConfigs[0].requestValue[BSA_REQVAL_CANID_SIZE + 7] = 0x01;
    g_stBSA_SimframeConfigs[0].interval_ms     = 10;
    g_stBSA_SimframeConfigs[0].messageCount    = 3;
    g_stBSA_SimframeConfigs[0].messages[0]     = msgCRC;
    g_stBSA_SimframeConfigs[0].messages[1]     = msgAC;
    g_stBSA_SimframeConfigs[0].messages[2]     = msgVolt;

    /* [1] CAN_ID=0x0120, 10ms, CRC+AC+Voltage, DATA[7]=0x01 */
    g_stBSA_SimframeConfigs[1].protocolId      = BSA_PROTOCOL_CAN_FD;
    g_stBSA_SimframeConfigs[1].dataRate        = 6;
    g_stBSA_SimframeConfigs[1].requestValueLen = reqValLen;
    g_stBSA_SimframeConfigs[1].requestValue[0] = 0x20;
    g_stBSA_SimframeConfigs[1].requestValue[1] = 0x01;
    g_stBSA_SimframeConfigs[1].requestValue[BSA_REQVAL_CANID_SIZE + 7] = 0x01;
    g_stBSA_SimframeConfigs[1].interval_ms     = 10;
    g_stBSA_SimframeConfigs[1].messageCount    = 3;
    g_stBSA_SimframeConfigs[1].messages[0]     = msgCRC;
    g_stBSA_SimframeConfigs[1].messages[1]     = msgAC;
    g_stBSA_SimframeConfigs[1].messages[2]     = msgVolt;

    /* [2] CAN_ID=0x0035, 10ms, CRC+AC, DATA[3]=0x41 DATA[4]=0x01 */
    g_stBSA_SimframeConfigs[2].protocolId      = BSA_PROTOCOL_CAN_FD;
    g_stBSA_SimframeConfigs[2].dataRate        = 6;
    g_stBSA_SimframeConfigs[2].requestValueLen = reqValLen;
    g_stBSA_SimframeConfigs[2].requestValue[0] = 0x35;
    g_stBSA_SimframeConfigs[2].requestValue[1] = 0x00;
    g_stBSA_SimframeConfigs[2].requestValue[BSA_REQVAL_CANID_SIZE + 3] = 0x41;
    g_stBSA_SimframeConfigs[2].requestValue[BSA_REQVAL_CANID_SIZE + 4] = 0x01;
    g_stBSA_SimframeConfigs[2].interval_ms     = 10;
    g_stBSA_SimframeConfigs[2].messageCount    = 2;
    g_stBSA_SimframeConfigs[2].messages[0]     = msgCRC;
    g_stBSA_SimframeConfigs[2].messages[1]     = msgAC;

    /* [3] CAN_ID=0x02AA, 100ms, CRC+AC */
    g_stBSA_SimframeConfigs[3].protocolId      = BSA_PROTOCOL_CAN_FD;
    g_stBSA_SimframeConfigs[3].dataRate        = 6;
    g_stBSA_SimframeConfigs[3].requestValueLen = reqValLen;
    g_stBSA_SimframeConfigs[3].requestValue[0] = 0xAA;
    g_stBSA_SimframeConfigs[3].requestValue[1] = 0x02;
    g_stBSA_SimframeConfigs[3].interval_ms     = 100;
    g_stBSA_SimframeConfigs[3].messageCount    = 2;
    g_stBSA_SimframeConfigs[3].messages[0]     = msgCRC;
    g_stBSA_SimframeConfigs[3].messages[1]     = msgAC;

    g_ucBSA_SimframeConfigCount = 4;
    g_bBSA_SimframeConfigReady  = true;
    BSA_ResetSimframeState();

    printf("[BSA] Simframe: 4 configs set\r\n");
    printf("  [0] CAN_ID=0x010A, 10ms, 3 msgs (CRC+AC+Volt)\r\n");
    printf("  [1] CAN_ID=0x0120, 10ms, 3 msgs (CRC+AC+Volt)\r\n");
    printf("  [2] CAN_ID=0x0035, 10ms, 2 msgs (CRC+AC)\r\n");
    printf("  [3] CAN_ID=0x02AA, 100ms, 2 msgs (CRC+AC)\r\n");
}

static void CLI_BSA_Clear(void)
{
    g_bBSA_DataConfigReady = false;
    g_bBSA_SimframeConfigReady = false;
    g_ucBSA_DataConfigCount = 0;
    g_ucBSA_SimframeConfigCount = 0;
    memset(g_stBSA_DataConfigs, 0, sizeof(g_stBSA_DataConfigs));
    memset(g_stBSA_SimframeConfigs, 0, sizeof(g_stBSA_SimframeConfigs));
    memset(g_SimframeAliveCnts, 0, sizeof(g_SimframeAliveCnts));
    memset(g_SimframeTxTimers, 0, sizeof(g_SimframeTxTimers));
    memset(&g_stBSA_MonData, 0, sizeof(g_stBSA_MonData));
    printf("[BSA] All configs cleared\r\n");
}

static void CLI_BSA_Status(void)
{
    printf("=== BSA Status ===\r\n");
    printf("  Running    : %s\r\n", g_bBatRelayConFlag ? "YES" : "NO");
    printf("  TX Ready   : %s (%d configs)\r\n",
           g_bBSA_SimframeConfigReady ? "YES" : "NO", g_ucBSA_SimframeConfigCount);
    printf("  RX Ready   : %s (%d configs)\r\n",
           g_bBSA_DataConfigReady ? "YES" : "NO", g_ucBSA_DataConfigCount);
    printf("  MonData    : Volt=%u PreRly=%u MainRly=%u IG=%u\r\n",
           g_stBSA_MonData.mosa_BSA_voltage, g_stBSA_MonData.mosa_PreRly_status,
           g_stBSA_MonData.mosa_MainRly_status, g_stBSA_MonData.mosa_IGON_status);

    if (g_ucBSA_SimframeConfigCount > 0) {
        printf("  TX Configs :\r\n");
        for (uint8_t i = 0; i < g_ucBSA_SimframeConfigCount; i++) {
            uint16_t canId = g_stBSA_SimframeConfigs[i].requestValue[0]
                           | (g_stBSA_SimframeConfigs[i].requestValue[1] << 8);
            printf("    [%d] CAN_ID=0x%04X %dms %dmsg AC=%d\r\n",
                   i, canId, g_stBSA_SimframeConfigs[i].interval_ms,
                   g_stBSA_SimframeConfigs[i].messageCount, g_SimframeAliveCnts[i]);
        }
    }
    if (g_ucBSA_DataConfigCount > 0) {
        printf("  RX Configs :\r\n");
        for (uint8_t i = 0; i < g_ucBSA_DataConfigCount; i++) {
            printf("    [%d] RxCAN_ID=0x%04X %dmsg\r\n",
                   i, g_stBSA_DataConfigs[i].responseValue,
                   g_stBSA_DataConfigs[i].messageCount);
        }
    }
}

void CLI_BSA_Cmd(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "show") == 0) {
        CLI_BSA_Show();
    } else if (argc >= 3 && strcmp(argv[1], "setmon") == 0) {
        CLI_BSA_SetMon(atoi(argv[2]));
    } else if (argc >= 3 && strcmp(argv[1], "setsim") == 0) {
        CLI_BSA_SetSim(atoi(argv[2]));
    } else if (argc >= 2 && strcmp(argv[1], "setex") == 0) {
        CLI_BSA_SetExample();
    } else if (argc >= 2 && strcmp(argv[1], "clear") == 0) {
        CLI_BSA_Clear();
    } else if (argc >= 2 && strcmp(argv[1], "start") == 0) {
        BSA_Start();
    } else if (argc >= 2 && strcmp(argv[1], "stop") == 0) {
        BSA_Stop();
    } else if (argc >= 2 && strcmp(argv[1], "status") == 0) {
        CLI_BSA_Status();
    } else {
        printf("usage: bsa show|setmon N|setsim N|setex|clear|start|stop|status\r\n");
    }
}
