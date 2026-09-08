/**
 * ******************************************************************************
 * @file    git-functionlist.c
 * ******************************************************************************
 */


/* Includes  -----------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/git-protocol.h"
#include "../Inc/git-functionlist.h"
#include "../Inc/task-bsa.h"
#include "../Inc/task-plc.h"
#include "../Inc/task-lcd.h"
#include "../Inc/task-hwcontrol.h"
#include "../Inc/led-indicator.h"
#include "../Inc/test-blebulk.h"   /* [TEST] 0xC1 bulk test — 제거 가능 */

/* Define    -----------------------------------------------------------*/
#define PLC_LOG(fmt, ...)  printf("\033[32m" fmt "\033[0m", ##__VA_ARGS__)

/* Variables -----------------------------------------------------------*/
// Monitoring/Simframe 
stBSA_DataConfig g_stBSA_DataConfigs[BSA_MAX_MONITORING_CONFIGS];
uint8_t g_ucBSA_DataConfigCount = 0;
stBSA_SimframeConfig g_stBSA_SimframeConfigs[BSA_MAX_SIMFRAME_CONFIGS];
uint8_t g_ucBSA_SimframeConfigCount = 0;
uint8_t g_SimframeAliveCnts[BSA_MAX_SIMFRAME_CONFIGS];
uint32_t g_SimframeTxTimers[BSA_MAX_SIMFRAME_CONFIGS];

stDB_CONFIG g_dbConfig;

/* 0x51 periodic TX state */
bool            g_bDisplayActive  = false;
static uint32_t g_DisplayTxTimer  = 0;
#define DISPLAY_TX_INTERVAL_MS  1000

/* 0x53 VCI3 collected data */
uint8_t g_vci3_soc = 0;

/* 0x81 Error Code (장비에서 ERROR 발생 시 set)
 *   값은 sys-common.h 의 ERROR_CODE_* 매크로 중 하나
 *   BLE 연결 edge(disconnected→connected)에서 1회만 전송 (git-comm.c)
 */
uint8_t g_error_code = ERROR_CODE_NONE;


/* Private Function Prototypes ------------------------------------------*/
void FL_GDS_SetMode(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_DataConfig(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_Vehicle_DataConfig(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_GetConnectorStatus(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_BSA_Data_Config_Monitoring(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_BSA_Data_Config_Simframe(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_BSA_Step_Config(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_PLC_SECC_Config(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_PLC_GDS_Config(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_PLC_Step_Data_Config(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_Display_Data_Req(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_Display_Data_Immediate(void);
void FL_GDS_Recv_VCI3Data(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_SetDevice_Start(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_Send_Error_Code(void);  /* 0x81: TX-only, call function in comm task loop */
void FL_GDS_APP_Error_Receive(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);

/* FW Update — 정의는 git-functionlist-fwupdate.c (Step 6) */
void FL_GDS_FwUpdate_Start  (void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_FwUpdate_Recv   (void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_FwUpdate_End    (void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_FwUpdate_Check  (void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_FwUpdate_SetList(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_FwUpdate_ForceApply(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);

/* Device Serial — 정의는 git-functionlist-fwupdate.c */
void FL_GDS_SetSerial(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void FL_GDS_GetSerial(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);

// GDS Protocol Function List (BLE -> UART2)
stFunctionList g_Functions_GDS[] =    
{
    // Device Setting Value 
    {0x11, FL_GDS_SetMode},
    {0x12, FL_GDS_DataConfig},
    {0x13, FL_GDS_Vehicle_DataConfig},
    {0x14, FL_GDS_GetConnectorStatus},

    // BSA Config
    {0x21, FL_GDS_BSA_Data_Config_Monitoring},
    {0x22, FL_GDS_BSA_Data_Config_Simframe},
    {0x23, FL_GDS_BSA_Step_Config},

    // PLC Module Comm Config
    {0x31, FL_GDS_PLC_SECC_Config},     // SECC RX config (deviceType=0x02)
    {0x32, FL_GDS_PLC_GDS_Config},      // GDS TX config  (deviceType=0x01)
    {0x33, FL_GDS_PLC_Step_Data_Config},

    // Step + Data Request
    {0x51, FL_GDS_Display_Data_Req},
    {0x53, FL_GDS_Recv_VCI3Data},

    // Error Code Send
    // {0x81, FL_GDS_Send_Error_Code},
    {0x82, FL_GDS_APP_Error_Receive},

    // Device Control
    {0x91, FL_GDS_SetDevice_Start},

    // FW Update (Step 6/7) — git-functionlist-fwupdate.c
    {0xA1, FL_GDS_FwUpdate_Start},
    {0xA2, FL_GDS_FwUpdate_Recv},
    {0xA3, FL_GDS_FwUpdate_End},
    {0xA4, FL_GDS_FwUpdate_Check},
    {0xA5, FL_GDS_FwUpdate_SetList},
    {0xA6, FL_GDS_FwUpdate_ForceApply},   /* [TEST] eMMC 즉시 적용 */

    /* Device Serial (BLE 광고 이름) */
    {0xB1, FL_GDS_SetSerial},             /* serial set(write-once) */
    {0xB2, FL_GDS_GetSerial},             /* serial get */

#if BLE_BULK_TEST_ENABLED
    /* [TEST] 대용량(512B) 수신 테스트 — 구현은 test-blebulk.c
     *   제거 시 test-blebulk.h 헤더 주석의 절차 참조 */
    {TEST_BULK_FUNC_ID, FL_GDS_Test_BulkRecv},
#endif
};

uint32_t	g_Func_GDS_cnt = sizeof( g_Functions_GDS ) / sizeof( stFunctionList );

void FL_GDS_SetMode(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    // Payload Validation
    if (uiLength < 1) {
        PLC_LOG("[FL_GDS_SetMode] Invalid payload length\r\n");
        GITPACKET_send_response(0x11, GITPACKET_NAK);
        return;
    }

    // Payload Data -> Mode Setting
    uint8_t mode = pData[0];
    switch (mode)
    {
        case 0x10:
            PLC_LOG("[FL_GDS_SetMode] Vehicle Charge Mode (0x10)\r\n");
            g_system_mode = eMODE_VEHICLE_CHARGE;
            ack = GITPACKET_ACK;
            break;
        case 0x20:
            PLC_LOG("[FL_GDS_SetMode] Vehicle Discharge Mode (0x20)\r\n");
            g_system_mode = eMODE_VEHICLE_DISCHARGE;
            ack = GITPACKET_ACK;
            break;
        case 0x30:
            PLC_LOG("[FL_GDS_SetMode] BSA Discharge Mode (0x30)\r\n");
            g_system_mode = eMODE_BSA_DISCHARGE;
            ack = GITPACKET_ACK;
            break;
        case 0x60:
            PLC_LOG("[FL_GDS_SetMode] Setting Mode (0x60)\r\n");
            g_system_mode = eMODE_SETTING;
            ack = GITPACKET_ACK;
            break;
        default:
            PLC_LOG("[FL_GDS_SetMode] Unknown mode 0x%02X\r\n", mode);
            ack = GITPACKET_NAK;
            break;
    }

    // Send ACK/NAK response frame: SOF | Len | FuncID(0x11) | ACK/NAK | CS | EOF
    GITPACKET_send_response(0x11, ack);

    /* 모드 설정 ACK 시 LCD 화면을 모드별 화면으로 즉시 전환
     *  · MAIN(scr=0) 한 번 거친 후 모드 화면으로 이동 (이전 팝업/잔여 상태 클리어)
     *  · 타이머 텍스트 "00:00:00" 초기화 (실제 카운트는 0x91 START 이후 시작)
     *  · DISCHARGE: ETA(예상 소요시간) 도 함께 셋팅 */
    if (ack == GITPACKET_ACK)
    {
        if (g_system_mode == eMODE_VEHICLE_CHARGE) {
            LCD_PostScreenGoto(LCD_SCR_MAIN);
            LCD_PostScreenGoto(LCD_SCR_CHARGE);
            LCD_PostTextSet(LCD_SCR_CHARGE, 8, "00:00:00");
            PLC_LOG("[FL_GDS_SetMode] LCD goto MAIN -> CHARGE (scr=1)\r\n");
        }
        else if (g_system_mode == eMODE_VEHICLE_DISCHARGE ||
                 g_system_mode == eMODE_BSA_DISCHARGE) {
            LCD_PostScreenGoto(LCD_SCR_MAIN);
            LCD_PostScreenGoto(LCD_SCR_DISCHARGE);
            LCD_PostTextSet(LCD_SCR_DISCHARGE, 18, "00:00:00");

            char eta_str[12];
            snprintf(eta_str, sizeof(eta_str), "%02u:%02u:00",
                     (unsigned)g_dbConfig.expected_charge_hour,
                     (unsigned)g_dbConfig.expected_charge_minute);
            LCD_PostTextSet(LCD_SCR_DISCHARGE, 8, eta_str);        // ETA (18 -> 8)
            PLC_LOG("[FL_GDS_SetMode] LCD goto MAIN -> DISCHARGE (scr=2) ETA=\"%s\"\r\n", eta_str);
        }
    }
}
void FL_GDS_DataConfig(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    /*
     * Payload format (FuncID 0x12):
     *   [0] : 목표 SOC (1B)
     *   [1] : 충전 목표 전류 (1B)
     *   [2] : 예상 소요 시간 - 시 (1B HEX, BE high)
     *   [3] : 예상 소요 시간 - 분 (1B HEX, BE low)
     *   [4] : Resistance (1B)
     */
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    if (uiLength < 5) {
        PLC_LOG("[FL_GDS_DataConfig] Invalid payload length: %lu (min 5)\r\n", uiLength);
        GITPACKET_send_response(0x12, GITPACKET_NAK);
        return;
    }

    g_dbConfig.vehicle_fullcharge_SOC  = pData[0];
    g_dbConfig.max_charge_current      = pData[1];
    g_dbConfig.expected_charge_hour    = pData[2];
    g_dbConfig.expected_charge_minute  = pData[3];
    g_dbConfig.resistance              = pData[4];

    /* CP Current adjust — 로직은 task-hwcontrol.c 로 이동 (동작 동일) */
    HWCON_ApplyChargeCurrent();

    PLC_LOG("[FL_GDS_DataConfig] target_SOC=%d charge_current=%d expected=%02dh %02dm resistance=%u\r\n",
           g_dbConfig.vehicle_fullcharge_SOC, g_dbConfig.max_charge_current,
           g_dbConfig.expected_charge_hour, g_dbConfig.expected_charge_minute,
           g_dbConfig.resistance);

    /* DISCHARGE/BSA 모드인 경우 LCD scr=2 ctrl=18 ETA 즉시 갱신
     *  · 0x11 SetMode 시점엔 g_dbConfig 가 stale 일 수 있으므로
     *    실제 ETA 값을 받는 0x12 시점에 한 번 더 push 해야 LCD 반영됨 */
    if (g_system_mode == eMODE_VEHICLE_DISCHARGE ||
        g_system_mode == eMODE_BSA_DISCHARGE)
    {
        char eta_str[12];
        snprintf(eta_str, sizeof(eta_str), "%02u:%02u:00",
                 (unsigned)g_dbConfig.expected_charge_hour,
                 (unsigned)g_dbConfig.expected_charge_minute);
        LCD_PostTextSet(LCD_SCR_DISCHARGE, 8, eta_str);        // ETA (18 -> 8)
        PLC_LOG("[FL_GDS_DataConfig] LCD ETA updated -> scr=2 ctrl=18 \"%s\"\r\n", eta_str);
    }

    ack = GITPACKET_ACK;
    GITPACKET_send_response(0x12, ack);
}

void FL_GDS_Vehicle_DataConfig(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    /*
     * Payload format (FuncID 0x13):
     *  [0] : Vehicle info Length (1B)
     *  [1..n] : Vehicle info (variable, max DB_VEHICLE_INFO_MAX) 
     *           
     * Function : Get Vehicle Information for LCD display
     *  - string format : Vehicle Name (Code) / Year
     *  - example : "EV6 (CV) / 2022", "아이오닉 일렉트릭 (AE EV) / 2027"
     *  - type : ASCII for english/num/symbol, UTF-8 for korean (up to DB_VEHICLE_INFO_MAX bytes)
     */
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    uint8_t infoLen = pData[0];
    if (infoLen > DB_VEHICLE_INFO_MAX) infoLen = DB_VEHICLE_INFO_MAX;

    g_dbConfig.vehicle_info_len = infoLen;
    if (infoLen > 0) {
        memcpy(g_dbConfig.vehicle_info, &pData[1], infoLen);
    }
    g_dbConfig.vehicle_info[infoLen] = '\0';

    /* LCD 4개 화면에 차량 정보 텍스트 표시
     *  (영문/숫자/특수문자 = ASCII, 한글 = UTF-8 그대로 전달) */
    LCD_PostTextSet(LCD_SCR_MAIN,      8,  g_dbConfig.vehicle_info);
    LCD_PostTextSet(LCD_SCR_CHARGE,    14, g_dbConfig.vehicle_info);
    LCD_PostTextSet(LCD_SCR_DISCHARGE, 14, g_dbConfig.vehicle_info);
    LCD_PostTextSet(LCD_SCR_SETTING,   18, g_dbConfig.vehicle_info);

    PLC_LOG("[FL_GDS_Vehicle_DataConfig] vehicle_info=\"%s\" (len=%u) -> LCD 4scr\r\n",
            g_dbConfig.vehicle_info, g_dbConfig.vehicle_info_len);

    ack = GITPACKET_ACK;
    GITPACKET_send_response(0x13, ack);
}

/*******************************************************************************
 * @brief  BSA Monitoring Config (FuncID 0x21)
 *
 *  Payload Layout:
 *  ┌─────────┬──────────────────┬──────┬───────────────────┐
 *  │  Offset │  Field           │ Size │  Description       │
 *  ├─────────┼──────────────────┼──────┼───────────────────┤
 *  │  [0-1]  │ protocolId       │  2B  │  BE               │
 *  │  [2]    │ dataRate         │  1B  │                   │
 *  │  [3-4]  │ responseValue    │  2B  │  BE (CAN ID)     │
 *  │  [5-6]  │ interval_ms      │  2B  │  BE               │
 *  │  [7]    │ messageCount     │  1B  │                   │
 *  ├─────────┼──────────────────┼──────┼───────────────────┤
 *  │  Per message (×messageCount, 16 bytes each)           │
 *  │  [+0]   │ valueType        │  1B  │                   │
 *  │  [+1]   │ startPosition    │  1B  │                   │
 *  │  [+2]   │ dataSize         │  1B  │                   │
 *  │  [+3~6] │ maskingValue     │  4B  │  BE               │
 *  │  [+7]   │ convType         │  1B  │                   │
 *  │  [+8~9] │ convA            │  2B  │  BE               │
 *  │ [+10~11]│ convB            │  2B  │  BE               │
 *  │  [+12]  │ convC            │  1B  │                   │
 *  │  [+13]  │ convD            │  1B  │                   │
 *  │  [+14]  │ convE            │  1B  │                   │
 *  │  [+15]  │ convF            │  1B  │                   │
 *  └─────────┴──────────────────┴──────┴───────────────────┘
 *  Header = 8B, Message = 16B each
 ******************************************************************************/
void FL_GDS_BSA_Data_Config_Monitoring(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    /* BSA 0x21 Monitoring config 파싱 로직은 task-bsa.c 로 이동 (동작 동일) */
    BSA_Cmd_ConfigMonitoring(pInterPtcl, uiInCommType, uiLength);
}

void FL_GDS_BSA_Data_Config_Simframe(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    /* BSA 0x22 Simframe config 파싱 로직은 task-bsa.c 로 이동 (동작 동일) */
    BSA_Cmd_ConfigSimframe(pInterPtcl, uiInCommType, uiLength);
}


/**
 * @brief  BLE FuncID 0x23: BSA Step Config
 *         BSA 통신 시퀀스 step 데이터 저장 (시퀀스 적용 미포함)
 *
 *  Payload format (all multi-byte fields: Big-Endian):
 *    [0]     stepno        (1B)
 *    [1]     type          (1B): 0x01=Monitor, 0x02=Simframe
 *    [2]     messageindex  (1B)
 *    [3..4]  interval_ms   (2B BE)
 *    [5]     message count (1B)
 *    --- messages (7B each) ---
 *    valueType(1) + code(2 BE) + min(2 BE) + max(2 BE)
 */
void FL_GDS_BSA_Step_Config(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    /* BSA 0x23 Step config 파싱 로직은 task-bsa.c 로 이동 (동작 동일) */
    BSA_Cmd_ConfigStep(pInterPtcl, uiInCommType, uiLength);
}

/**
 * @brief  BLE FuncID 0x31: PLC SECC Config (RX)
 *         SECC→MCU 방향 CAN RX 설정, deviceType=PLC_DEVICE_TYPE_RX 자동 적용
 *
 *  Payload format (all multi-byte fields: Big-Endian):
 *    [0..1]  protocolId    (2B BE): 0x0100=Classic, 0x0130=FD
 *    [2]     datarate      (1B)
 *    [3]     messageindex  (1B)
 *    [4..7]  response value(4B BE): CAN ID (29-bit extended)
 *    [8..9]  interval_ms   (2B BE)
 *    [10]    messageCount  (1B)
 *    --- messages (16B each) ---
 *    valueType(1) + startPos(1) + dataSize(1) + mask(4 BE) + conv(9)
 */
void FL_GDS_PLC_SECC_Config(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    if (uiLength < PLC_CFG_SECC_HDR_SIZE) {
        printf("[PLC SECC Config] Invalid payload length: %lu (min %d)\r\n",
               uiLength, PLC_CFG_SECC_HDR_SIZE);
        GITPACKET_send_response(0x31, GITPACKET_NAK);
        return;
    }

    PLC_ApplyConfig(pData, uiLength, PLC_DEVICE_TYPE_RX);

    ack = GITPACKET_ACK;
    GITPACKET_send_response(0x31, ack);
}

/**
 * @brief  BLE FuncID 0x32: PLC GDS Config (TX)
 *         MCU->PLC 방향 CAN TX 설정, deviceType=PLC_DEVICE_TYPE_TX 자동 적용
 *
 *  Payload format (all multi-byte fields: Big-Endian):
 *    [0..1]   protocolId    (2B BE): 0x0100=Classic, 0x0130=FD
 *    [2]      datarate      (1B)
 *    [3]      messageindex  (1B)
 *    [4..27]  request value (24B): canId(4B BE) + dataTemplate(8B) + reserved(12B)
 *    [28..29] interval_ms   (2B BE)
 *    [30]     messageCount  (1B)
 *    --- messages (16B each) ---
 *    valueType(1) + startPos(1) + dataSize(1) + mask(4 BE) + conv(9)
 */
void FL_GDS_PLC_GDS_Config(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    if (uiLength < PLC_CFG_GDS_HDR_SIZE) {
        printf("[PLC GDS Config] Invalid payload length: %lu (min %d)\r\n",
               uiLength, PLC_CFG_GDS_HDR_SIZE);
        GITPACKET_send_response(0x32, GITPACKET_NAK);
        return;
    }

    PLC_ApplyConfig(pData, uiLength, PLC_DEVICE_TYPE_TX);

    ack = GITPACKET_ACK;
    GITPACKET_send_response(0x32, ack);
}

/**
 * @brief  BLE FuncID 0x33: PLC Step Data Config
 *         0x31/0x32로 설정된 CAN ID별 messageIndex에 대한 시퀀스 step 값 설정
 *
 *  BLE Payload format (all multi-byte fields: Big-Endian):
 *    [0]     stepno       (1B)
 *    [1]     devicetype   (1B: 0x01=TX/GDS, 0x02=RX/SECC)
 *    [2]     messageindex (1B)
 *    [3..4]  interval_ms  (2B BE)
 *    [5]     messageCount (1B)
 *    --- messages (7B each) ---
 *    valueType(1) + code(2 BE) + min(2 BE) + max(2 BE)
 *
 *  내부적으로 substep(1B, 0x00)을 삽입하여 PLC_ApplyStepConfig에 전달
 */
void FL_GDS_PLC_Step_Data_Config(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    /* PLC 0x33 Step Data config 파싱 로직은 task-plc.c 로 이동 (동작 동일) */
    PLC_Cmd_StepDataConfig(pInterPtcl, uiInCommType, uiLength);
}


void FL_GDS_Display_Data_Req(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    /*
     * Payload format (FuncID 0x51):
     *   [0] : active (1B)
     *           0x01 = 1000ms interval TX start
     */
    uint8_t *pData = (uint8_t*)pInterPtcl;

    if (uiLength < 1) {
        printf("[Display] Invalid payload length\r\n");
        GITPACKET_send_response(0x51, GITPACKET_NAK);
        return;
    }

    uint8_t action = pData[0];
    if (action == 0x01) {
        g_bDisplayActive = true;
        g_DisplayTxTimer = Get_Tmr();
        printf("[Display] Periodic TX started (interval=%dms)\r\n", DISPLAY_TX_INTERVAL_MS);
        GITPACKET_send_response(0x51, GITPACKET_ACK);
    } 
    else if (action == 0x02) {
        g_bDisplayActive = false;
        printf("[Display] Periodic TX stopped\r\n");
        GITPACKET_send_response(0x51, GITPACKET_ACK);
    }
    else if (action == 0x03) {
        printf("[Display] Immediate TX requested\r\n");
        FL_GDS_Display_Data_Immediate();
    }
    else {
        printf("[Display] Unknown action: 0x%02X\r\n", action);
        GITPACKET_send_response(0x51, GITPACKET_NAK);
    }
}

void FL_GDS_Display_Data_Periodic(void)
{
    /*
     * Call period : DISPLAY_TX_INTERVAL_MS
     * activate by 0x51 payload 0x01
     * TX payload 
     * [0..1] : Current (2B BE, A) , 소수점 1자리까지 * 10 -> HEX
     * [2..3] : Voltage (2B BE, V) , 소수점 1자리까지 * 10 -> HEX
     * [4..5] : Power (2B BE, kW)  , 소수점 1자리까지 * 10 -> HEX
     * [6..7] : Temperature Sensor 1 (2B BE, °C) , 소수점 1자리까지 * 10 -> HEX
     * [8..9] : Temperature Sensor 2 (2B BE, °C) , 소수점 1자리까지 * 10 -> HEX
     */
    if (!g_bDisplayActive) return;
    if (Get_TmrDelta(Get_Tmr(), g_DisplayTxTimer) < DISPLAY_TX_INTERVAL_MS) return;

    g_DisplayTxTimer = Get_Tmr();

    uint8_t payload[10];
    uint32_t ofs = 0;

    /* [0..1] Current */
    uint16_t dischargeCurrent = (uint16_t)(g_discharge_current_A * 10.0f);
    payload[ofs++] = (uint8_t)((dischargeCurrent >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(dischargeCurrent & 0xFF);

    /* [2..3] Voltage */
    uint16_t dischargeVoltage = (uint16_t)(g_discharge_voltage_V * 10.0f);
    payload[ofs++] = (uint8_t)((dischargeVoltage >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(dischargeVoltage & 0xFF);

    /* [4..5] : Power */
    uint16_t power = (int16_t)(g_power_W / 100.0f);
    payload[ofs++] = (uint8_t)((power >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(power & 0xFF);

    /* [6..7] : Temperature Sensor 1  */
    int16_t temp0 = (int16_t)(g_temp_value[0] * 10.0f);
    payload[ofs++] = (uint8_t)((temp0 >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(temp0 & 0xFF);

    /* [8..9] : Temperature Sensor 2  */
    int16_t temp1 = (int16_t)(g_temp_value[1] * 10.0f);
    payload[ofs++] = (uint8_t)((temp1 >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(temp1 & 0xFF);

    // int16_t temp2 = (int16_t)(g_temp_value[2] * 10.0f);
    // payload[ofs++] = (uint8_t)((temp2 >> 8) & 0xFF);
    // payload[ofs++] = (uint8_t)(temp2 & 0xFF);

    printf("curr : %.1f A, volt : %.1f V\r\n", g_discharge_current_A, g_discharge_voltage_V);
    printf("power : %.1f W (%dkW)\r\n", g_power_W, power);
    printf("temp1 : %.1f °C, temp2 : %.1f °C, temp3 : %.1f °C\r\n", g_temp_value[0], g_temp_value[1], g_temp_value[2]);

    printf("[0x51 TX] ");
    for (uint32_t i = 0; i < sizeof(payload); i++) printf("%02X ", payload[i]);
    printf("\r\n");

    uint8_t frame_buff[GITPACKET_FRAME_SIZE_MAX];
    int frame_size = GITPACKET_make_frame(0x51, payload, sizeof(payload), frame_buff);
    if (frame_size > 0) {
        GITPACKET_send_frame_via_uart(frame_buff, (uint16_t)frame_size);
    }
}

/**
 * @brief  Status Send Immediate (0x51) 
 *         
 */
void FL_GDS_Display_Data_Immediate(void)
{
    uint8_t payload[10];
    uint32_t ofs = 0;

    uint16_t dischargeCurrent = (uint16_t)(g_discharge_current_A * 100.0f);
    payload[ofs++] = (uint8_t)((dischargeCurrent >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(dischargeCurrent & 0xFF);

    uint16_t dischargeVoltage = (uint16_t)(g_discharge_voltage_V * 10.0f);
    payload[ofs++] = (uint8_t)((dischargeVoltage >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(dischargeVoltage & 0xFF);

    uint16_t power = (int16_t)(g_power_W / 100.0f);
    payload[ofs++] = (uint8_t)((power >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(power & 0xFF);

    int16_t temp0 = (int16_t)(g_temp_value[0] * 10.0f);
    payload[ofs++] = (uint8_t)((temp0 >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(temp0 & 0xFF);

    int16_t temp1 = (int16_t)(g_temp_value[1] * 10.0f);
    payload[ofs++] = (uint8_t)((temp1 >> 8) & 0xFF);
    payload[ofs++] = (uint8_t)(temp1 & 0xFF);


    uint8_t frame_buff[GITPACKET_FRAME_SIZE_MAX];
    int frame_size = GITPACKET_make_frame(0x51, payload, sizeof(payload), frame_buff);
    if (frame_size > 0) {
        GITPACKET_send_frame_via_uart(frame_buff, (uint16_t)frame_size);
    }
}

/**
 * @brief  0x81: Device Error Code TX (TX only, No request)
 *
 *  호출 시점은 BLE 연결 edge (disconnected → connected) 에서 1회.
 *  (git-comm.c 의 StartCommTask 연결 감지 지점에서 호출됨)
 *
 *  TX Condition:
 *   - BT connected
 *   - g_error_code != ERROR_CODE_NONE
 *
 *  Payload (1B):
 *    [0] : error_code  (ERROR_CODE_* value)
 */
void FL_GDS_Send_Error_Code(void)
{
    /* 에러 없음 → 처리 불필요 */
    if (g_error_code == ERROR_CODE_NONE) return;

    /* 에러 코드별 LED 분기 (실제 GPIO 적용은 led-indicator.c 가 전담).
     * ★ BLE 연결 여부와 무관하게 항상 실행 — 물리 안전 표시(R/Y/부저)가 앱
     *   연결 여부에 종속되면 안 된다(연결 안 된 상태에서 과충전 등 발생 시에도
     *   LED/부저는 반드시 동작해야 함). BLE 프레임 송신만 아래에서 연결 여부로 게이팅.
     *  · err 1,2,3 (OVER_CHARGE/DISCHARGE/TEMP) → 치명적 에러 (R 점멸 + 부저 3초)
     *  · err 4 (USER_LCD_CONTROL_STOP)          → 정지성 이벤트, LED 표시 없음
     *                                              (정지 후 자동으로 대기(G 점멸)로 전환)
     *  · err 5 (BSA_POWER_CONNECTOR_FAIL)       → 경고 (Y 점멸)
     *  · err 6 (INSUL_RESISTANCE)               → 경고 (Y 점멸) */
    switch (g_error_code) {
        case ERROR_CODE_OVER_CHARGE:
        case ERROR_CODE_OVER_DISCHARGE:
        case ERROR_CODE_OVER_TEMPERATURE:
            LED_RaiseCriticalError();
            PLC_LOG("[FL_GDS_Send_Error_Code] err=%d -> LED R_BLINK + buzzer\r\n", g_error_code);
            break;

        case ERROR_CODE_USER_LCD_CONTROL_STOP:
            PLC_LOG("[FL_GDS_Send_Error_Code] err=4 (stop event, no LED change)\r\n");
            break;

        case ERROR_CODE_BSA_POWER_CONNECTOR_FAIL:
        case ERROR_CODE_INSUL_RESISTANCE:
            LED_RaiseWarning();
            PLC_LOG("[FL_GDS_Send_Error_Code] err=%d -> LED Y_BLINK\r\n", g_error_code);
            break;

        default:
            PLC_LOG("[FL_GDS_Send_Error_Code] err=%d (no LED change)\r\n", g_error_code);
            break;
    }

    /* 안전 가드: BT 미연결이면 프레임 송신만 스킵 (LED/부저는 위에서 이미 처리됨) */
    if (!BTGetConnectStatus()) return;

    uint8_t payload[1];
    payload[0] = g_error_code;

    uint8_t frame_buff[GITPACKET_FRAME_SIZE_MAX];
    int frame_size = GITPACKET_make_frame(0x81, payload, sizeof(payload), frame_buff);
    if (frame_size > 0) {
        GITPACKET_send_frame_via_uart(frame_buff, (uint16_t)frame_size);
        PLC_LOG("[FL_GDS_Send_Error_Code] TX err=%d (one-shot on BLE connect)\r\n",
                g_error_code);
    }
}

/**
 * @brief  BLE FuncID 0x53: VCI3 수집 데이터 수신
 *         현재 SOC 값 (1B) 저장
 *
 *  Payload format:
 *    [0] : 현재 SOC (1B, 0~100%)
 */
void FL_GDS_Recv_VCI3Data(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    if (uiLength < 1) {
        PLC_LOG("[VCI3] Invalid payload length: %lu (min 1)\r\n", uiLength);
        GITPACKET_send_response(0x53, GITPACKET_NAK);
        return;
    }

    g_vci3_soc = pData[0];

    PLC_LOG("[VCI3] SOC=%d%%\r\n", g_vci3_soc);

    /* 0x53 수신 시점에 현재 모드 기준으로 LCD SoC 텍스트 즉시 갱신
     *   · CHARGE                       → scr=1 ctrl=3
     *   · VEHICLE_DISCHARGE / BSA      → scr=2 ctrl=3 */
    {
        char soc_str[5];
        snprintf(soc_str, sizeof(soc_str), "%d", g_vci3_soc);
        if (g_system_mode == eMODE_VEHICLE_CHARGE) {
            LCD_PostTextSet(LCD_SCR_CHARGE, 3, soc_str);
        } else if (g_system_mode == eMODE_VEHICLE_DISCHARGE ||
                   g_system_mode == eMODE_BSA_DISCHARGE) {
            LCD_PostTextSet(LCD_SCR_DISCHARGE, 3, soc_str);
        }
    }

    ack = GITPACKET_ACK;
    GITPACKET_send_response(0x53, ack);
}

/* eBDC_StopPopupMode 와 BDC_StopSequence() 정의는 sys-main.c 로 이동됨
 *  · enum / 프로토타입은 sys-common.h 에 선언 (양쪽 파일에서 사용)
 *  · 아래 호출부(FL_GDS_StopDevice_WithoutPopup / 0x91 STOP / 0x82)는 그대로 유지 */

void FL_GDS_StopDevice_WithoutPopup(void)
{
    BDC_StopSequence(BDC_STOP_POPUP_NONE);
}

/**
 * @brief  BLE FuncID 0x91: SetDevice Start/Stop
 *
 *  Payload format (15B, all multi-byte: Big-Endian):
 *    [0]      : action (1B)  0x01=Start → eDEVICE_STATE_START
 *                            0x02=Stop  → eDEVICE_STATE_STOP
 *    [1..2]   : year         (2B BE)
 *    [3..4]   : month        (2B BE)
 *    [5..6]   : day          (2B BE)
 *    [7..8]   : hour         (2B BE)
 *    [9..10]  : minute       (2B BE)
 *    [11..12] : second       (2B BE)
 *    [13..14] : millisecond  (2B BE)
 */
void FL_GDS_SetDevice_Start(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    if (uiLength < 15) {
        PLC_LOG("[FL_GDS_SetDevice] Invalid payload length: %lu (min 15)\r\n", uiLength);
        GITPACKET_send_response(0x91, GITPACKET_NAK);
        return;
    }

    uint8_t action = pData[0];

    /* 시간 데이터 파싱 (14B ASCII) — 2바이트씩 ASCII decimal 변환
     * 예: 32 30 32 36 30 33 32 33 31 33 32 33 30 38
     *   → "20""26""03""23""13""23""08"
     *   → year=2026, month=03, day=23, hour=13, min=23, sec=08 */
    #define ASCII2DEC(h, l)  (uint16_t)(((h) - '0') * 10 + ((l) - '0'))
    g_device_time.year        = ASCII2DEC(pData[1], pData[2]) * 100
                              + ASCII2DEC(pData[3], pData[4]);
    g_device_time.month       = ASCII2DEC(pData[5],  pData[6]);
    g_device_time.day         = ASCII2DEC(pData[7],  pData[8]);
    g_device_time.hour        = ASCII2DEC(pData[9],  pData[10]);
    g_device_time.minute      = ASCII2DEC(pData[11], pData[12]);
    g_device_time.second      = ASCII2DEC(pData[13], pData[14]);
    g_device_time.millisecond = 0;
    #undef ASCII2DEC

    if (action == 0x01) {
        g_device_state = eDEVICE_STATE_START;
        SENSOR_ErrorCheck_Arm();   /* START 후 30초 경과 시부터 센서 에러 체크 시작 */
        PLC_LOG("[FL_GDS_SetDevice] START  %04d-%02d-%02d %02d:%02d:%02d.%03d\r\n",
               g_device_time.year, g_device_time.month, g_device_time.day,
               g_device_time.hour, g_device_time.minute, g_device_time.second,
               g_device_time.millisecond);

        if (g_system_mode == eMODE_VEHICLE_CHARGE) {
            /* g_device_state = START → task-hwcontrol.c에서 CP 상태머신 시작 */
            PLC_LOG("[FL_GDS_SetDevice] VEHICLE_CHARGE: CP start (current=%dA)\r\n",
                   g_dbConfig.max_charge_current);
        }
        /* mode==VEHICLE_DISCHARGE 상태면 CAN2 SW EN + PLC step 시퀀스 자동 시작 */
        if (g_system_mode == eMODE_VEHICLE_DISCHARGE)
        {
            if (HAL_GPIO_ReadPin(CAN2_SW_EN_GPIO_Port, CAN2_SW_EN_Pin) == GPIO_PIN_RESET) {
                IO_CONTROL_HIGH(CAN2_SW_EN);
                PLC_LOG("[FL_GDS_SetDevice] CAN2_SW_EN → HIGH\r\n");
            }
            g_plcManualRun = true;
            PLC_StepSequenceStart();
            PLC_LOG("[FL_GDS_SetDevice] PLC StepSequence started\r\n");
        }
        /* mode==BSA_DISCHARGE 상태면 CAN1 SW EN */
        if (g_system_mode == eMODE_BSA_DISCHARGE)
        {
            if (HAL_GPIO_ReadPin(CAN1_SW_EN_GPIO_Port, CAN1_SW_EN_Pin) == GPIO_PIN_RESET) {
                IO_CONTROL_HIGH(CAN1_SW_EN);
                PLC_LOG("[FL_GDS_SetDevice] CAN1_SW_EN → HIGH\r\n");
            }
            BSA_Start();

        }
        ack = GITPACKET_ACK;
    }
    else if (action == 0x02) {
        PLC_LOG("[FL_GDS_SetDevice] STOP   %04d-%02d-%02d %02d:%02d:%02d.%03d\r\n",
               g_device_time.year, g_device_time.month, g_device_time.day,
               g_device_time.hour, g_device_time.minute, g_device_time.second,
               g_device_time.millisecond);
        BDC_StopSequence(BDC_STOP_POPUP_WORK_DONE);   /* 정상 STOP: WorkDone 팝업 */
        ack = GITPACKET_ACK;
    } else {
        PLC_LOG("[FL_GDS_SetDevice] Unknown action: 0x%02X\r\n", action);
    }

    /* 응답: [0]=action(1or2), [1]=ACK(0x00)/NAK(0x01) */
    {
        uint8_t resp[2] = { action, ack };
        uint8_t frame[64];
        int fsize = GITPACKET_make_frame(0x91, resp, sizeof(resp), frame);
        if (fsize > 0) GITPACKET_send_frame_via_uart(frame, (uint16_t)fsize);
    }
}

/**
 * @brief  0x14: Connector Status 
 */
void FL_GDS_GetConnectorStatus(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    /* no rx payload */
    (void)pInterPtcl;
    (void)uiInCommType;
    (void)uiLength;
    uint8_t ack = GITPACKET_NAK;

    SystemMode_t mode = g_system_mode;   

#define CP_ADC_8V_LOWER   2649   /* 8V lower limit (ADC 12-bit) */
#define CP_ADC_10V_UPPER  3311   /* 10V upper limit (ADC 12-bit) */
#define BSA_RX_ALIVE_MS   2000   /* Alive if received within 2 seconds */

    switch (mode)
    {
        case eMODE_VEHICLE_CHARGE:  
        {
            /* CP ADC Value */
            uint16_t adc = CP_CLI_ReadADC();
            bool connected = (adc >= CP_ADC_8V_LOWER && adc <= CP_ADC_10V_UPPER);
            ack = connected ? GITPACKET_ACK : GITPACKET_NAK;
            printf("[ConnectorStatus] mode=0x%02X ADC=%u -> %s\r\n",
                   mode, adc, connected ? "CONNECTED" : "NOT CONNECTED");
            break;
        }
        case eMODE_VEHICLE_DISCHARGE:  
        {
            /* PLC(FDCAN2) 0x15ECC102 - byte[1] value : CP Voltage */
            #define DISC_BYTE1_MIN  85
            #define DISC_BYTE1_MAX  95

            bool can2_was_off = (HAL_GPIO_ReadPin(CAN2_SW_EN_GPIO_Port, CAN2_SW_EN_Pin) == GPIO_PIN_RESET);
            if (can2_was_off) {
                IO_CONTROL_HIGH(CAN2_SW_EN);
                printf("[ConnectorStatus] DISCHARGE: CAN2_SW_EN temp HIGH\r\n");
            }

            uint32_t entry_tick = HAL_GetTick();

            osDelay(500);

            uint32_t last_tick = g_plc15ECC102LastTick;
            uint8_t  byte1     = g_plc15ECC102LastByte1;
            bool     fresh     = (last_tick != 0) &&
                                 ((int32_t)(last_tick - entry_tick) >= 0);
            bool     connected = fresh && (byte1 >= DISC_BYTE1_MIN && byte1 <= DISC_BYTE1_MAX);
            ack = connected ? GITPACKET_ACK : GITPACKET_NAK;
            printf("[ConnectorStatus] DISCHARGE 0x15ECC102 byte[1]=%u(0x%02X) "
                   "lastTick=%lu entry=%lu -> %s\r\n",
                   byte1, byte1,
                   (unsigned long)last_tick, (unsigned long)entry_tick,
                   connected ? "CONNECTED" : (fresh ? "OUT_OF_RANGE" : "NO_RX"));

            if (can2_was_off) {
                IO_CONTROL_LOW(CAN2_SW_EN);
                printf("[ConnectorStatus] DISCHARGE: CAN2_SW_EN restored LOW\r\n");
            }

            #undef DISC_BYTE1_MIN
            #undef DISC_BYTE1_MAX
            break;
        }
        case eMODE_BSA_DISCHARGE:  
        {
            // 1. BSA Comm Connector Status
            extern FDCAN_HandleTypeDef hfdcan1;
            extern HAL_StatusTypeDef CAN_HW_setting(void);
            extern HAL_StatusTypeDef CAN_stop(void);

            bool bTempStart = (hfdcan1.State != HAL_FDCAN_STATE_BUSY);
            if (bTempStart) {
                g_bsaCanRxLastTick = 0;  /* 이전 잔류값 초기화 */
                CAN_HW_setting();
                printf("[ConnectorStatus] BSA: FDCAN1 temp start\r\n");
            }

            osDelay(500);

            uint32_t elapsed = HAL_GetTick() - g_bsaCanRxLastTick;
            bool connected = (g_bsaCanRxLastTick != 0 && elapsed <= BSA_RX_ALIVE_MS);
            ack = connected ? GITPACKET_ACK : GITPACKET_NAK;
            printf("[ConnectorStatus] BSA elapsed=%lums -> %s\r\n",
                   elapsed, connected ? "CONNECTED" : "NOT CONNECTED");

            if (bTempStart) {
                CAN_stop();
                printf("[ConnectorStatus] BSA: FDCAN1 temp stop\r\n");
            }
            
            // // 2. BSA Power Connector --> Check by sensor CH2
            // // ==> if   g_discharge_voltage_V == 0 -> DISCONNECTED
            // if (g_discharge_voltage_V == 0) {
            //     ack = GITPACKET_NAK;
            //     printf("[ConnectorStatus] BSA: Voltage=0V -> NOT CONNECTED\r\n");
            // } else {
            //     printf("[ConnectorStatus] BSA: Voltage=%.1fV -> CONNECTED\r\n", g_discharge_voltage_V);
            // }

            break;
        }
        default:
            printf("[ConnectorStatus] Unknown mode: 0x%02X\r\n", mode);
            break;
    }

#undef CP_ADC_8V_LOWER
#undef CP_ADC_10V_UPPER
#undef BSA_RX_ALIVE_MS

    if (ack == GITPACKET_NAK)
    {
        LED_RaiseWarning();
        printf("[ConnectorStatus] disconnect -> LED Y (sticky)\r\n");

        /* LCD -> FAIL POPUP */ 
        if (mode == eMODE_VEHICLE_CHARGE) {
            LCD_PostScreenGoto(LCD_SCR_CHARGE);
            LCD_PostPopupDriveFail(LCD_SCR_CHARGE);
            printf("[ConnectorStatus] disconnect -> LCD scr=1 + DriveFail popup\r\n");
        } else if (mode == eMODE_VEHICLE_DISCHARGE || mode == eMODE_BSA_DISCHARGE) {
            LCD_PostScreenGoto(LCD_SCR_DISCHARGE);
            LCD_PostPopupDriveFail(LCD_SCR_DISCHARGE);
            printf("[ConnectorStatus] disconnect -> LCD scr=2 + DriveFail popup\r\n");
        }
    } 
    else
    {
        LED_ClearAll();
        printf("[ConnectorStatus] connect -> LED NORMAL (G)\r\n");
    }

    GITPACKET_send_response(0x14, ack);
}

/**
 * @brief  BLE FuncID 0x82: APP → FW Error Code Receive (Device Stop Trigger)
 *  payload[0] : error code (1B)
 *  Response   : 0x82 + ACK/NAK (1B)
 */
void FL_GDS_APP_Error_Receive(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    if (uiLength < 1) {
        PLC_LOG("[FL_GDS_APP_Error_Receive] Invalid payload length: %lu\r\n", uiLength);
        GITPACKET_send_response(0x82, GITPACKET_NAK);
        return;
    }

    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t errorCode = pData[0];
    PLC_LOG("[FL_GDS_APP_Error_Receive] APP error=0x%02X -> STOP sequence\r\n", errorCode);

    /* shut down device */
    BDC_StopSequence(BDC_STOP_POPUP_APP_ERROR);

    /* APP error 수신 → LED Y (sticky) — START / 0x14 ACK -> clear */
    LED_RaiseWarning();
    PLC_LOG("[FL_GDS_APP_Error_Receive] LED Y (sticky)\r\n");

    GITPACKET_send_response(0x82, GITPACKET_ACK);
}



