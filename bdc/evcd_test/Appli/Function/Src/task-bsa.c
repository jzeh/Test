
#include <math.h>

#include "../Inc/sys-common.h"
#include "../Inc/task-bsa.h"
#include "../Inc/task-can.h"
#include "../Inc/task-hwcontrol.h"
#include "../Inc/git-functionlist.h"   /* GITPACKET_ACK/NAK (send_response 는 git-protocol.h) */   


osMessageQueueId_t hBatRelayConMsg;  
osThreadId_t hBatRelayConTh;  
osThreadId_t hBatRelayMonitorTh;  

osMessageQueueId_t hDiagMsg = NULL;
osMemoryPoolId_t hDiagPool = NULL;
osMemoryPoolId_t hPTPKPool = NULL;
osMemoryPoolId_t hBatRelayConPool = NULL;
osMemoryPoolId_t hBatRelayConPKPool = NULL;

uint32_t m_unBatRelayCon_10ms_Timer = 0;
uint32_t m_unBatRelayCon_100ms_Timer = 0;
char g_cAliveCount_0035 = 0;
stCanPacket g_BsaRxCanPacket;

#pragma pack(push, 1)
typedef struct {
    uint16_t canId;           // byte 0-1: CAN ID (plain uint16_t)
    uint8_t  canLen;          // byte 2:   data length
    uint8_t  data[64];        // byte 3~66: CAN data fields
} stBSA_RxPacket;
#pragma pack(pop)
static stBSA_RxPacket g_BsaRxPkt;

bool g_bBatRelayConFlag = false;
eBatRelayConType g_eBatRelayConType = BatRelayConType_None;

/* BSA CAN HW config — 0x21/0x22 수신 시 갱신.
 *  초기값: BLE 미수신 시 종래 동작 유지 (FD BRS 500k/2M) */
volatile uint16_t g_bsaHwProtocolId  = BSA_PROTOCOL_CAN_FD;  /* default FD */
volatile uint8_t  g_bsaHwDataRate    = 0x06;                  /* default 500k/2M (LUT 0x06) */
volatile bool     g_bsaHwConfigValid = false;

// BSA TX 디버그 카운터 (BSA_Start에서 리셋)
static uint32_t s_txOkCnt = 0, s_txFailCnt = 0;
static uint32_t s_rxCnt = 0;

/////////////////////////////////////////////`///////////////////
// NE EV - Read CAN Data Variable
uint16_t g_usNEBattPackVolt = 0; // 0x0235, 14~15 Bytes
uint8_t g_ucNEPreChrgState = 0;
uint8_t g_ucNEMainRlyOnState = 0;
// OS EV - Read CAN Data Variable
uint16_t g_usOSBattPackVolt = 0;
ePRAConStatus g_ePRAConStatus = PRAConStatus_None;
uint32_t m_unPRADig_Timer = 0;
bool g_bPRADigOpenFlag = false;

/*--------------------------------------------------------------------*/

static const osThreadAttr_t batRelayConAttr = {
    .name = "BatRelayCon",
    .stack_size = 2048,
    .priority = (osPriority_t)osPriorityNormal,
};

static const osThreadAttr_t batRelayMonAttr = {
    .name = "BatRelayMon",
    .stack_size = 2048,
    .priority = (osPriority_t)osPriorityNormal,
};

bool StartBSAThread(void)
{
    hBatRelayConMsg = osMessageQueueNew(MESSAGE_BATRELAYCON_QUEUE_SIZE, sizeof(int), NULL);
    if( hBatRelayConMsg == NULL )
    {
        GLogE( "Error... fail create hBatRelayConMsg!!!\r\n" );
        return false;
    }

    hBatRelayConTh = osThreadNew(BatRelayConThread, NULL, &batRelayConAttr);
    if( hBatRelayConTh == NULL )
    {
        GLogE( "Error... fail create hBatRelayConTh Thread!!!\r\n" );
        return false;
    }

    hBatRelayMonitorTh = osThreadNew(BatRelayMonitorThread, NULL, &batRelayMonAttr);
    if( hBatRelayMonitorTh == NULL )
    {
        GLogE( "Error... fail create hBatRelayMonitorTh Thread!!!\r\n" );
        return false;
    }
    
    return true;
}
/* DB Config */
// Call Function in 0x21 (FL_GDS_BSA_Data_Config_Monitoring) 
void BSA_Cmd_ConfigMonitoring(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    // Minimum payload: header 8 bytes
    if (uiLength < BSA_DATA_CONFIG_HDR_SIZE) 
    {
        printf("[Monitoring] Invalid payload length: %lu (min %d)\r\n",
               uiLength, BSA_DATA_CONFIG_HDR_SIZE);
        GITPACKET_send_response(0x21, GITPACKET_NAK);
        return;
    }

    // Parse header: protocolId(2) + dataRate(1) + responseValue(2) + interval_ms(2) + messageCount(1)
    uint16_t protocolId    = ((uint16_t)pData[0] << 8) | pData[1];
    uint8_t  dataRate      = pData[2];
    uint16_t responseValue = ((uint16_t)pData[3] << 8) | pData[4];
    uint16_t interval_ms   = ((uint16_t)pData[5] << 8) | pData[6];
    uint8_t  messageCount  = pData[7];

    // Validate message count
    if (messageCount == 0 || messageCount > BSA_DATA_CONFIG_MSG_MAX) 
    {
        printf("[Monitoring] Invalid message count: %d (max %d)\r\n",
               messageCount, BSA_DATA_CONFIG_MSG_MAX);
        GITPACKET_send_response(0x21, GITPACKET_NAK);
        return;
    }

    // Validate total payload length: header(6) + messageCount * 16
    uint32_t expectedLen = BSA_DATA_CONFIG_HDR_SIZE + (messageCount * BSA_DATA_CONFIG_MSG_SIZE);
    if (uiLength < expectedLen) 
    {
        printf("[Monitoring] Payload too short: %lu (expected %lu)\r\n", uiLength, expectedLen);
        GITPACKET_send_response(0x21, GITPACKET_NAK);
        return;
    }

    // 배열 슬롯 확인 (같은 CAN ID면 업데이트, 아니면 새 슬롯)
    int cfgIdx = -1;
    for (uint8_t k = 0; k < g_ucBSA_DataConfigCount; k++) 
    {
        if (g_stBSA_DataConfigs[k].responseValue == responseValue) 
        {
            cfgIdx = k;
            break;
        }
    }
    if (cfgIdx < 0) 
    {
        if (g_ucBSA_DataConfigCount >= BSA_MAX_MONITORING_CONFIGS) 
        {
            printf("[Monitoring] Config array full (max %d)\r\n", BSA_MAX_MONITORING_CONFIGS);
            GITPACKET_send_response(0x21, GITPACKET_NAK);
            return;
        }
        cfgIdx = g_ucBSA_DataConfigCount;
        g_ucBSA_DataConfigCount++;
    }

    stBSA_DataConfig *pCfg = &g_stBSA_DataConfigs[cfgIdx];
    pCfg->protocolId    = protocolId;
    pCfg->dataRate      = dataRate;
    pCfg->responseValue = responseValue;
    pCfg->interval_ms   = interval_ms;
    pCfg->messageCount  = messageCount;

    printf("[Monitoring] [%d] ProtocolID=0x%04X(%s), nDataRate=%d, RxCAN_ID=0x%04X, interval=%dms, MsgCount=%d\r\n",
           cfgIdx, protocolId, (protocolId == BSA_PROTOCOL_CAN_FD) ? "FD" : "Classic",
           dataRate, responseValue, interval_ms, messageCount);

    /* BSA CAN HW config 전역 갱신 — 다음 CAN_HW_setting() 호출 시 반영
     *  · 같은 BSA 세션 내 여러 0x21 가 와도 마지막 값으로 덮어쓰기 (보통 동일 값) */
    g_bsaHwProtocolId  = protocolId;
    g_bsaHwDataRate    = dataRate;
    g_bsaHwConfigValid = true;

    // Parse each message (16 bytes each)
    for (uint8_t i = 0; i < messageCount; i++) 
    {
        uint32_t ofs = BSA_DATA_CONFIG_HDR_SIZE + (i * BSA_DATA_CONFIG_MSG_SIZE);
        stBSA_DataConfigMsg *pMsg = &pCfg->messages[i];

        pMsg->valueType     = pData[ofs + 0];
        pMsg->startPosition = pData[ofs + 1];
        pMsg->dataSize      = pData[ofs + 2];
        pMsg->maskingValue  = ((uint32_t)pData[ofs + 3] << 24)
                            | ((uint32_t)pData[ofs + 4] << 16)
                            | ((uint32_t)pData[ofs + 5] <<  8)
                            |  (uint32_t)pData[ofs + 6];

        pMsg->convRule.convType = pData[ofs + 7];
        pMsg->convRule.convA    = ((uint16_t)pData[ofs +  8] << 8) | pData[ofs +  9];
        pMsg->convRule.convB    = ((uint16_t)pData[ofs + 10] << 8) | pData[ofs + 11];
        pMsg->convRule.convC    = pData[ofs + 12];
        pMsg->convRule.convD    = pData[ofs + 13];
        pMsg->convRule.convE    = pData[ofs + 14];
        pMsg->convRule.convF    = pData[ofs + 15];

        printf("  Msg[%d]: type=%d, pos=%d, size=%d, mask=0x%08lX, "
               "conv(%d: A=0x%04X B=0x%04X C=0x%02X D=0x%02X E=0x%02X F=0x%02X)\r\n",
               i, pMsg->valueType, pMsg->startPosition, pMsg->dataSize, pMsg->maskingValue,
               pMsg->convRule.convType, pMsg->convRule.convA, pMsg->convRule.convB,
               pMsg->convRule.convC, pMsg->convRule.convD, pMsg->convRule.convE, pMsg->convRule.convF);
    }

    g_bBSA_DataConfigReady = true;
    printf("[Monitoring] Config[%d] ready (RxCAN_ID=0x%04X, %s, nDataRate=%d, %d msgs) total=%d\r\n",
           cfgIdx, responseValue, (protocolId == BSA_PROTOCOL_CAN_FD) ? "FD" : "Classic",
           dataRate, messageCount, g_ucBSA_DataConfigCount);

    ack = GITPACKET_ACK;
    GITPACKET_send_response(0x21, ack);
}

// Call Function in 0x22 (FL_GDS_BSA_Data_Config_Simframe)
void BSA_Cmd_ConfigSimframe(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    // Minimum: base_hdr(4) + requestValue(CAN_ID(2)+data(8) min) + tail(3) = 17
    if (uiLength < (BSA_SIMFRAME_BASE_HDR_SIZE + BSA_REQVAL_CANID_SIZE + 8 + BSA_SIMFRAME_TAIL_SIZE)) 
    {
        printf("[Simframe] Invalid payload length: %lu (min %d)\r\n",
               uiLength, BSA_SIMFRAME_BASE_HDR_SIZE + BSA_REQVAL_CANID_SIZE + 8 + BSA_SIMFRAME_TAIL_SIZE);
        GITPACKET_send_response(0x22, GITPACKET_NAK);
        return;
    }

    // ── 1) Parse base header: protocolId(2) + dataRate(1) ──
    //   protocolId: 0x0100=CAN Classic, 0x0130=CAN FD
    //   dataRate:   ndatarate (bitrate config index 0~7)
    uint16_t protocolId   = ((uint16_t)pData[0] << 8) | pData[1];
    uint8_t  dataRate     = pData[2];   // ndatarate (bitrate index)

    // ── 2) requestValueLen은 pData[3]에서 직접 수신 ──
    //   requestValue = [CAN_ID(2 bytes)] + [CAN_DATA(N bytes)]
    //   message.startPosition is relative to CAN_DATA (requestValue+2)
    uint8_t requestValueLen = pData[3];

    if (requestValueLen < BSA_REQVAL_CANID_SIZE) 
    {
        printf("[Simframe] Invalid requestValueLen: %d (min %d)\r\n",
               requestValueLen, BSA_REQVAL_CANID_SIZE);
        GITPACKET_send_response(0x22, GITPACKET_NAK);
        return;
    }

    if (protocolId != BSA_PROTOCOL_CAN_CLASSIC && protocolId != BSA_PROTOCOL_CAN_FD) 
    {
        printf("[Simframe] Unknown protocolId: 0x%04X (expected 0x%04X or 0x%04X)\r\n",
               protocolId, BSA_PROTOCOL_CAN_CLASSIC, BSA_PROTOCOL_CAN_FD);
        GITPACKET_send_response(0x22, GITPACKET_NAK);
        return;
    }

    /* BSA CAN HW config 전역 갱신 — 0x21 과 동일하게 다음 CAN_HW_setting() 시 반영 */
    g_bsaHwProtocolId  = protocolId;
    g_bsaHwDataRate    = dataRate;
    g_bsaHwConfigValid = true;

    // ── 3) ASCII→Hex 변환: requestValue는 ASCII hex 문자열로 수신됨 ──
    //   예: "30 31 32 30" (ASCII "0120") → 저장: [0x01, 0x20]
    //   ASCII 2문자 = 1바이트, 변환 후 길이 = requestValueLen / 2
    uint32_t reqValOfs = BSA_SIMFRAME_BASE_HDR_SIZE;  // offset 4

    if (requestValueLen % 2 != 0) 
    {
        printf("[Simframe] requestValueLen must be even (got %d)\r\n", requestValueLen);
        GITPACKET_send_response(0x22, GITPACKET_NAK);
        return;
    }

    uint8_t convertedLen = requestValueLen / 2;
    uint8_t convertedBuf[BSA_MAX_DATA_LENGTH + BSA_REQVAL_CANID_SIZE];
    memset(convertedBuf, 0, sizeof(convertedBuf));

    for (uint8_t i = 0; i < convertedLen; i++) 
    {
        uint8_t hi = pData[reqValOfs + i * 2];
        uint8_t lo = pData[reqValOfs + i * 2 + 1];
        /* ASCII char → nibble */
        uint8_t hn = (hi >= '0' && hi <= '9') ? (hi - '0') :
                     (hi >= 'A' && hi <= 'F') ? (hi - 'A' + 10) :
                     (hi >= 'a' && hi <= 'f') ? (hi - 'a' + 10) : 0;
        uint8_t ln = (lo >= '0' && lo <= '9') ? (lo - '0') :
                     (lo >= 'A' && lo <= 'F') ? (lo - 'A' + 10) :
                     (lo >= 'a' && lo <= 'f') ? (lo - 'a' + 10) : 0;
        convertedBuf[i] = (hn << 4) | ln;
    }

    uint16_t canId = ((uint16_t)convertedBuf[0] << 8) | convertedBuf[1];

    // ── 4) Parse tail: interval_ms(2) + messageCount(1) ──
    uint32_t tailOfs     = reqValOfs + requestValueLen;
    uint16_t interval_ms = ((uint16_t)pData[tailOfs] << 8) | pData[tailOfs + 1];
    uint8_t  messageCount = pData[tailOfs + 2];

    // Validate total payload length
    uint32_t expectedLen = BSA_SIMFRAME_BASE_HDR_SIZE + requestValueLen
                         + BSA_SIMFRAME_TAIL_SIZE
                         + (uint32_t)messageCount * BSA_DATA_CONFIG_MSG_SIZE;
    if (uiLength < expectedLen) 
    {
        printf("[Simframe] Payload too short: %lu (expected %lu)\r\n", uiLength, expectedLen);
        GITPACKET_send_response(0x22, GITPACKET_NAK);
        return;
    }

    // ── 5) Store to config array ──
    // 같은 CAN ID면 업데이트, 아니면 새 슬롯
    int cfgIdx = -1;
    for (uint8_t k = 0; k < g_ucBSA_SimframeConfigCount; k++) 
    {
        uint16_t existId = g_stBSA_SimframeConfigs[k].requestValue[0] | ((uint16_t)g_stBSA_SimframeConfigs[k].requestValue[1] << 8);
        if (existId == canId) 
        {
            cfgIdx = k;
            break;
        }
    }
    if (cfgIdx < 0) 
    {
        if (g_ucBSA_SimframeConfigCount >= BSA_MAX_SIMFRAME_CONFIGS) 
        {
            printf("[Simframe] Config array full (max %d)\r\n", BSA_MAX_SIMFRAME_CONFIGS);
            GITPACKET_send_response(0x22, GITPACKET_NAK);
            return;
        }
        cfgIdx = g_ucBSA_SimframeConfigCount;
        g_ucBSA_SimframeConfigCount++;
    }

    stBSA_SimframeConfig *pCfg = &g_stBSA_SimframeConfigs[cfgIdx];
    pCfg->protocolId      = protocolId;
    pCfg->dataRate        = dataRate;
    pCfg->requestValueLen = convertedLen;  /* 변환 후 실제 바이트 길이 저장 */
    memset(pCfg->requestValue, 0, BSA_MAX_DATA_LENGTH + BSA_REQVAL_CANID_SIZE);
    /* CAN ID: little endian 저장 (기존 컨벤션에 맞춤) */
    pCfg->requestValue[0] = (uint8_t)(canId & 0xFF);         // LO byte
    pCfg->requestValue[1] = (uint8_t)((canId >> 8) & 0xFF);  // HI byte
    memcpy(&pCfg->requestValue[BSA_REQVAL_CANID_SIZE],
           &convertedBuf[BSA_REQVAL_CANID_SIZE],
           convertedLen - BSA_REQVAL_CANID_SIZE);
    pCfg->interval_ms     = interval_ms;
    pCfg->messageCount    = messageCount;

    printf("[Simframe] [%d] ProtocolID=0x%04X, nDataRate=%d, CAN_ID=0x%04X, "
           "ASCII_Len=%d->Hex_Len=%d(data=%d), Interval=%dms, MsgCount=%d\r\n",
           cfgIdx, protocolId, dataRate, canId,
           requestValueLen, convertedLen, convertedLen - BSA_REQVAL_CANID_SIZE,
           interval_ms, messageCount);

    // Print converted requestValue hex dump
    printf("  ReqVal(hex): ");
    for (uint8_t j = 0; j < convertedLen; j++) 
    {
        printf("%02X ", pCfg->requestValue[j]);
    }
    printf("\r\n");

    // ── 6) Parse each message (16 bytes each) ──
    uint32_t msgBaseOfs = BSA_SIMFRAME_BASE_HDR_SIZE + requestValueLen + BSA_SIMFRAME_TAIL_SIZE;
    for (uint8_t i = 0; i < messageCount; i++) 
    {
        uint32_t ofs = msgBaseOfs + (i * BSA_DATA_CONFIG_MSG_SIZE);
        stBSA_DataConfigMsg *pMsg = &pCfg->messages[i];

        pMsg->valueType     = pData[ofs + 0];
        pMsg->startPosition = pData[ofs + 1];
        pMsg->dataSize      = pData[ofs + 2];
        pMsg->maskingValue  = ((uint32_t)pData[ofs + 3] << 24)
                            | ((uint32_t)pData[ofs + 4] << 16)
                            | ((uint32_t)pData[ofs + 5] <<  8)
                            |  (uint32_t)pData[ofs + 6];

        pMsg->convRule.convType = pData[ofs + 7];
        pMsg->convRule.convA    = ((uint16_t)pData[ofs +  8] << 8) | pData[ofs +  9];
        pMsg->convRule.convB    = ((uint16_t)pData[ofs + 10] << 8) | pData[ofs + 11];
        pMsg->convRule.convC    = pData[ofs + 12];
        pMsg->convRule.convD    = pData[ofs + 13];
        pMsg->convRule.convE    = pData[ofs + 14];
        pMsg->convRule.convF    = pData[ofs + 15];

        printf("  Msg[%d]: type=%d, pos=%d, size=%d, mask=0x%08lX, "
               "conv(%d: A=0x%04X B=0x%04X C=0x%02X D=0x%02X E=0x%02X F=0x%02X)\r\n",
               i, pMsg->valueType, pMsg->startPosition, pMsg->dataSize, pMsg->maskingValue,
               pMsg->convRule.convType, pMsg->convRule.convA, pMsg->convRule.convB,
               pMsg->convRule.convC, pMsg->convRule.convD, pMsg->convRule.convE, pMsg->convRule.convF);
    }

    // 해당 슬롯의 alive counter / timer 초기화
    g_SimframeAliveCnts[cfgIdx] = 0;
    g_SimframeTxTimers[cfgIdx] = Get_Tmr();

    g_bBSA_SimframeConfigReady = true;
    printf("[Simframe] Config[%d] ready (CAN_ID=0x%04X, %s, nDataRate=%d, interval=%dms, %d msgs) total=%d\r\n",
           cfgIdx, canId, (protocolId == BSA_PROTOCOL_CAN_FD) ? "FD" : "Classic",
           dataRate, interval_ms, messageCount, g_ucBSA_SimframeConfigCount);

    ack = GITPACKET_ACK;
    GITPACKET_send_response(0x22, ack);
}

// Call Function in 0x23 (FL_GDS_BSA_Step_Config)
void BSA_Cmd_ConfigStep(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    if (uiLength < BSA_STEP_HDR_SIZE) {
        printf("[BSA Step] Invalid payload length: %lu (min %d)\r\n",
               uiLength, BSA_STEP_HDR_SIZE);
        GITPACKET_send_response(0x23, GITPACKET_NAK);
        return;
    }

    uint8_t  stepno       = pData[0];
    uint8_t  type         = pData[1];
    uint8_t  messageIndex = pData[2];
    uint16_t interval_ms  = ((uint16_t)pData[3] << 8) | pData[4];
    uint8_t  messageCount = pData[5];

    if (type != BSA_STEP_TYPE_MONITOR && type != BSA_STEP_TYPE_SIMFRAME) {
        printf("[BSA Step] Unknown type: 0x%02X (0x01=Monitor, 0x02=Simframe)\r\n", type);
        GITPACKET_send_response(0x23, GITPACKET_NAK);
        return;
    }

    if (messageCount > BSA_STEP_MAX_MSGS) {
        printf("[BSA Step] messageCount too large: %d (max %d)\r\n",
               messageCount, BSA_STEP_MAX_MSGS);
        GITPACKET_send_response(0x23, GITPACKET_NAK);
        return;
    }

    uint32_t expectedLen = BSA_STEP_HDR_SIZE + (uint32_t)messageCount * BSA_STEP_MSG_SIZE;
    if (uiLength < expectedLen) {
        printf("[BSA Step] Payload too short: %lu (expected %lu)\r\n",
               uiLength, expectedLen);
        GITPACKET_send_response(0x23, GITPACKET_NAK);
        return;
    }

    /* Find existing slot (same stepno + type + messageIndex) or allocate new */
    int slotIdx = -1;
    for (uint16_t s = 0; s < g_stBSA_StepData.count; s++) {
        stBSA_StepConfig *p = &g_stBSA_StepData.steps[s];
        if (p->stepno == stepno && p->type == type && p->messageIndex == messageIndex) {
            slotIdx = s;
            break;
        }
    }
    if (slotIdx < 0) {
        if (g_stBSA_StepData.count >= BSA_STEP_MAX_ENTRIES) {
            printf("[BSA Step] Array full (max %d)\r\n", BSA_STEP_MAX_ENTRIES);
            GITPACKET_send_response(0x23, GITPACKET_NAK);
            return;
        }
        slotIdx = g_stBSA_StepData.count;
        g_stBSA_StepData.count++;
    }

    stBSA_StepConfig *pStep = &g_stBSA_StepData.steps[slotIdx];
    pStep->stepno       = stepno;
    pStep->type         = type;
    pStep->messageIndex = messageIndex;
    pStep->interval_ms  = interval_ms;
    pStep->messageCount = messageCount;

    /* Parse messages (7 bytes each): valueType(1) + code(2 BE) + min(2 BE) + max(2 BE) */
    uint32_t msgOfs = BSA_STEP_HDR_SIZE;
    for (uint8_t i = 0; i < messageCount; i++) {
        uint32_t ofs = msgOfs + (i * BSA_STEP_MSG_SIZE);
        stBSA_StepMsg *pMsg = &pStep->messages[i];

        pMsg->valueType = pData[ofs + 0];
        pMsg->code      = ((uint16_t)pData[ofs + 1] << 8) | pData[ofs + 2];
        pMsg->minValue  = ((uint16_t)pData[ofs + 3] << 8) | pData[ofs + 4];
        pMsg->maxValue  = ((uint16_t)pData[ofs + 5] << 8) | pData[ofs + 6];

        printf("  Step[%d] Msg[%d]: type=%d "
               "code=0x%04X min=0x%04X max=0x%04X\r\n",
               stepno, i, pMsg->valueType,
               pMsg->code, pMsg->minValue, pMsg->maxValue);
    }

    g_stBSA_StepData.configured = true;

    printf("[BSA Step] [%d] step=%d type=%s msgIdx=%d interval=%dms msgs=%d total=%d\r\n",
           slotIdx, stepno,
           (type == BSA_STEP_TYPE_MONITOR) ? "Monitor" : "Simframe",
           messageIndex, interval_ms, messageCount, g_stBSA_StepData.count);

    ack = GITPACKET_ACK;
    GITPACKET_send_response(0x23, ack);
}

/* Thread Def */
void BatRelayConThread(void *argument)
{
    for(;;)
    {
        BatRelayConEventListener();

        if( g_bBatRelayConFlag == true )
        {
            uint32_t currentTime = Get_Tmr();

            // Dynamic BLE-configured simframe TX (highest priority)
            if (g_bBSA_SimframeConfigReady) {
                BSA_ProcessSimframeTx(currentTime);
            }
            // BSA Flexible TX system (hardcoded configs)
            else if (g_bUseExternalTxConfig) {
                BSA_ProcessTxPackets(currentTime);
            }
            else {
                // Use legacy hardcoded TX system - deleted
                printf("BSA legacy logic enterd - not using\r\n");
            }

            // if( g_ePRAConStatus != PRAConStatus_None )
            // {
            //     printf("PRA control logic enterd - not using\r\n");
            // }

        }

        osDelay(1);
    }
}

void BatRelayMonitorThread(void *argument)
{
    // uint16_t usVolt = 0;
    // float fVolt = 0.0;
    // static uint16_t s_usNeBatPackVolt = 0;
    
    for(;;)
    {
        if( g_bBatRelayConFlag == true )
        {
            memset(&g_BsaRxPkt, 0, sizeof(g_BsaRxPkt));

            if( OemReadCanBuff((U8*)&g_BsaRxPkt, MOSA_RX_TIMEOUT) )
            {
                // OemReadCanBuff 가 쓰는 레이아웃: [canId(2B)][canLen(1B)][data(64B)]
                uint16_t rxCanId  = g_BsaRxPkt.canId;
                uint8_t  rxLen    = g_BsaRxPkt.canLen;
                uint8_t *rxData   = g_BsaRxPkt.data;

                // RX 디버그 로그 (처음 5개만, BSA_Start에서 리셋)
                s_rxCnt++;
                if (s_rxCnt <= 5) {
                    printf("[BSA RX] #%lu ID=0x%04X len=%d data[0..3]=%02X %02X %02X %02X\r\n",
                           s_rxCnt, rxCanId, rxLen,
                           rxLen > 0 ? rxData[0] : 0, rxLen > 1 ? rxData[1] : 0,
                           rxLen > 2 ? rxData[2] : 0, rxLen > 3 ? rxData[3] : 0);
                }

                // ── Dynamic BLE-configured monitoring (g_stBSA_DataConfigs[]) ──
                if (g_bBSA_DataConfigReady) {
                    uint8_t dataLen = (rxLen > 0 && rxLen <= 64) ? rxLen : 64;
                    BSA_ApplyDataConfig_Monitoring(rxCanId, rxData, dataLen);
                }
                // ── BSA Flexible RX system (hardcoded configs) ──
                else if (g_bUseExternalTxConfig) {
                    uint8_t dataLen = (rxLen > 0 && rxLen <= 64) ? rxLen : 8;
                    Process_BSA_RxSignal(rxCanId, rxData, dataLen);
                }
                else {
                    // Use legacy hardcoded RX system
                    printf("BSA legacy RX logic enterd - not using\r\n");
                    // if( g_eBatRelayConType == BatRelayConType_NEEV )
                    // {
                    //     if( rxCanId == NE_PRECHRGSTA_CANID )
                    //     {
                    //         memcpy(&g_ucNEPreChrgState, &rxData[NE_CHRG_STATE_DATA_POSITION], NE_CHRG_STATE_DATA_SIZE);
                    //         g_ucNEPreChrgState = (g_ucNEPreChrgState>>4)&0x03;

                    //         memcpy(&usVolt, &rxData[NE_BATPACP_VOLT_DATA_POSITION], NE_BATPACP_VOLT_DATA_SIZE);
                    //         fVolt = ((float)usVolt*NE_BATPACP_VOLT_FACTOR);
                    //         s_usNeBatPackVolt = (uint16_t)round(fVolt);
                    //     }
                    //     else if( rxCanId == NE_MAINRLYONSTA_CANID )
                    //     {
                    //         memcpy(&g_ucNEMainRlyOnState, &rxData[NE_CHRG_STATE_DATA_POSITION], NE_CHRG_STATE_DATA_SIZE);
                    //         g_ucNEMainRlyOnState = g_ucNEMainRlyOnState&0x0C;
                    //     }

                    //     // Set Baterry Pack Voltage Value
                    //     if( g_ucNEPreChrgState > 0 || g_ucNEMainRlyOnState > 0 )
                    //     {
                    //         if( g_usNEBattPackVolt != s_usNeBatPackVolt )
                    //         {
                    //             g_usNEBattPackVolt = s_usNeBatPackVolt;
                    //         }
                    //     }
                    //     else
                    //     {
                    //         if( g_usNEBattPackVolt != 0 )
                    //         {
                    //             g_usNEBattPackVolt = 0;
                    //         }
                    //     }
                    // }
                    // else if( g_eBatRelayConType == BatRelayConType_OSEV )
                    // {
                    //     if( rxCanId == OS_MONITORING_CANID )
                    //     {
                    //         memcpy(&usVolt, &rxData[OS_BATPACP_VOLT_DATA_POSITION], OS_BATPACP_VOLT_DATA_SIZE);

                    //         if( !(usVolt < 0x000A || usVolt == 0xFFFF) )
                    //         {
                    //             fVolt = ((float)usVolt*OS_BATPACP_VOLT_FACTOR);
                    //             g_usOSBattPackVolt = (uint16_t)round(fVolt);
                    //         }
                    //         else
                    //         {
                    //             g_usOSBattPackVolt = 0;
                    //         }
                    //     }
                    // }
                }
            }
        }
        else
        {
            // memset(&g_BsaRxCanPacket, 0, sizeof(stCanPacket));
            // g_usNEBattPackVolt = 0;
            // g_ucNEPreChrgState = 0;
            // g_usOSBattPackVolt = 0;
        }
        
        osDelay(1);
    }
}

void BatRelayConEventListener()
{
    osStatus_t status;  // Changed for CMSIS v2
    stMsgClst *message;
    stCommPkt *packet;
    
    // Changed to CMSIS v2: osMessageQueueGet
    if(hBatRelayConMsg == NULL) return;
    status = osMessageQueueGet(hBatRelayConMsg, &message, NULL, 1);
    if( status == osOK )
    {
        packet = (stCommPkt*)message->pPacket;

        switch(message->mMod)
        {
            case BatRelayConStatus_Init:
                InitGITSetConfig();
                InitGITHWSetData();

                // if( message->mSeq == BatRelayConType_NEEV )
                // {
                //     VCI_HW_Setting(BAT_FD_RELAY_CON);
                //     g_eBatRelayConType = BatRelayConType_NEEV;
                    
                //     // Initialize BSA Flexible system for NE EV
                //     BSA_ConfigureForNEEV();
                // }
                // else if( message->mSeq == BatRelayConType_OSEV )
                // {
                //     VCI_HW_Setting(BAT_RELAY_CON);
                //     g_eBatRelayConType = BatRelayConType_OSEV;
                    
                //     // Initialize BSA Flexible system for OS EV
                //     BSA_ConfigureForOSEV();
                // }
                
                m_unBatRelayCon_10ms_Timer = Get_Tmr();
                m_unBatRelayCon_100ms_Timer = Get_Tmr();

                SendMSGToBatRelayCon(BatRelayConStatus_Run, message);
                
                break;

            case BatRelayConStatus_Run:	
                g_bBatRelayConFlag = true;	
                SendMSGToBatRelayCon(BatRelayConStatus_Idle, message);

                break;

            case BatRelayConStatus_Idle:
                // (g_bBatRelayConFlag) 1 : Run - CAN Writting & Read Monitoring
                // (g_bBatRelayConFlag) 0 : Waitting
                break;

            case BatRelayConStatus_Stop:
                g_bBatRelayConFlag = false;
                SendMSGToBatRelayCon(BatRelayConStatus_Exit, message);

                break;
                
            case BatRelayConStatus_Exit:
                memset(&g_BsaRxCanPacket, 0, sizeof(stCanPacket));
                g_usNEBattPackVolt = 0;
                g_ucNEPreChrgState = 0;
                g_bPRADigOpenFlag = false;
                g_ePRAConStatus = PRAConStatus_None;
                g_usOSBattPackVolt = 0;
                BSA_ResetSimframeState();
                g_bBSA_SimframeConfigReady = false;
                g_ucBSA_SimframeConfigCount = 0;
                g_bBSA_DataConfigReady = false;
                g_ucBSA_DataConfigCount = 0;
                memset(&g_stBSA_MonData, 0, sizeof(stBSA_MON_DATA));
              
                SendMSGToBatRelayCon(BatRelayConStatus_Idle, message);

                break;

            case BatRelayConStatus_PRA_On:
                if( g_bBatRelayConFlag == true )
                {
                    if( message->mSeq == BatRelayConType_NEEV )
                    {
                        g_ePRAConStatus = PRAConStatus_PRA_On;
                        m_unPRADig_Timer = Get_Tmr();
                    }

                    SendMSGToBatRelayCon(BatRelayConStatus_Idle, message);
                }

                break;

            case BatRelayConStatus_PRA_Off:
                if( g_bBatRelayConFlag == true )
                {
                    if( message->mSeq == BatRelayConType_NEEV )
                    {
                        g_ePRAConStatus = PRAConStatus_PRA_Off;
                        m_unPRADig_Timer = Get_Tmr();
                    }

                    SendMSGToBatRelayCon(BatRelayConStatus_Idle, message);
                }

                break;

            default:
                break;
        }
        
        // Changed to CMSIS v2: osMemoryPoolFree
        if(hBatRelayConPKPool) osMemoryPoolFree(hBatRelayConPKPool, packet);
        if(hBatRelayConPool)   osMemoryPoolFree(hBatRelayConPool, message);
    }
}

void SendMSGToBatRelayCon(u16 Mode, stMsgClst *message)
{
    stMsgClst *msg;
    stCommPkt *pkt;
    
    // Changed to CMSIS v2: osMemoryPoolAlloc
    if(hBatRelayConPool == NULL || hBatRelayConPKPool == NULL || hBatRelayConMsg == NULL) return;
    msg = (stMsgClst*)osMemoryPoolAlloc(hBatRelayConPool, osWaitForever);
    if( msg == NULL )
    {
        return;
    }
    pkt = (stCommPkt*)osMemoryPoolAlloc(hBatRelayConPKPool, osWaitForever);
    if( pkt == NULL )
    {
        osMemoryPoolFree(hBatRelayConPool, msg);
        return;
    }
    
    msg->mPktType = message->mPktType;
    msg->mMsgType = message->mMsgType;
    msg->mMod = Mode;
    memcpy(pkt, message->pPacket, sizeof(stMsgClst));
    msg->pPacket = (void *)pkt;
    
    // Changed to CMSIS v2: osMessageQueueSpacesAvailable and osMessageQueuePut
    if( osMessageQueueGetSpace(hBatRelayConMsg) == 0 )
    {
        osMemoryPoolFree(hBatRelayConPKPool, pkt);
        osMemoryPoolFree(hBatRelayConPool, msg);
    }
    else
    {
        osMessageQueuePut(hBatRelayConMsg, &msg, 0, osWaitForever);
        //GLogN("SendMSGToListDiag\r\n");
    }
}

unsigned short CalculateCRC16(char* pcData, unsigned int unLen)
{
    unsigned short usCrc = CRC16_INIT_VALUE;

    for(int i=0; i<unLen; i++)
    {   
        usCrc = usCrc ^ ((*pcData) << 8); 
        pcData++;

        for(int j=0; j<8; j++)
        {
            if(usCrc & 0x8000)
            {
                usCrc = (usCrc<<1) ^ CRC16_POLY;
            }
            else
            {
                usCrc = usCrc<<1;
            }
        }
    }   

    return usCrc;
}

void MakeNeEvTxPacket(char* pcData, char* pcPreData, unsigned short usCanId, char cCnt)
{
    char cCheckData[32] = {0,};
    char cFinalData[32] = {0,};
    unsigned short usCalData = 0;
    unsigned short usCRC16Data = 0;

    memcpy(cCheckData, &pcPreData[2], 30);
    memcpy(&cCheckData[0], &cCnt, 1);

    usCalData = usCanId + CRC16_ADD_VALUE;
    memcpy(&cCheckData[30], &usCalData, 2);

    usCRC16Data = CalculateCRC16(cCheckData, 32);

    memcpy(&cFinalData[0], &usCRC16Data, 2);
    memcpy(&cFinalData[2], cCheckData, 30);

    if( usCanId == 0x010A || usCanId == 0x0120 )
    {
        memcpy(&cFinalData[16], &g_usNEBattPackVolt, 2);
    }

    memcpy(pcData, cFinalData, sizeof(cCheckData));
}

void MakeOsEvTxPacket(char* pcData, unsigned short usCanId)
{
    if( usCanId == 0x0524 )
    {
        memcpy(&pcData[0], &g_usOSBattPackVolt, 2);
    }
}

// void BatRelayCon_WriteCanPacket(stBatRelayConData* pstData, uint16_t unTime, eBatRelayConType eType, uint16_t unCount)
// {
//     unsigned short usCanId = 0, usSwapCanId = 0;

//     MsgDiag_t *msgDiag[5];
//     PTmsgPkt_t *pktDiag[5];

//     if(hDiagPool == NULL || hPTPKPool == NULL || hDiagMsg == NULL) return;

//     for(int i=0; i<unCount; i++)
//     {
//         if( pstData[i].unCycle == unTime )
//         {
//             // Changed to CMSIS v2: osMemoryPoolAlloc
//             msgDiag[i] = (MsgDiag_t*)osMemoryPoolAlloc(hDiagPool, osWaitForever);
//             if( msgDiag[i] == NULL )
//             {
//                 return;
//             }
//             pktDiag[i] = (PTmsgPkt_t*)osMemoryPoolAlloc(hPTPKPool, osWaitForever);
//             if( pktDiag[i] == NULL )
//             {
//                 osMemoryPoolFree(hDiagPool, msgDiag[i]);
//                 return;
//             }
            
//             if( eType == BatRelayConType_NEEV )
//             {
//                 memset(pstData[i].pcData, 0, sizeof(pstData[i].pcData));
//                 MakeNeEvTxPacket(pstData[i].pcData, pstData[i].pcPreData, pstData[i].usCanId, pstData[i].cAliveCnt);
//             }
//             else if( eType == BatRelayConType_OSEV )
//             {
//                 MakeOsEvTxPacket(pstData[i].pcData, pstData[i].usCanId);
//             }
//             //printf("CAN TIME : %d, CAN ID : %X\n",unTime,pstData[i].usCanId);
//             //hexdump(pstData[i].pcData, pstData[i].usCanLen);

//             memcpy(&usCanId, &pstData[i].usCanId, MOSA_RX_CANID_SIZE);
//             usSwapCanId = ByteSwap(usCanId);
             
//             memcpy(&pktDiag[i]->pData[0], &usSwapCanId, sizeof(unsigned short));
//             memcpy(&pktDiag[i]->pData[2], &pstData[i].usCanLen, MOSA_RX_CANLENTH_SIZE);
//             memcpy(&pktDiag[i]->pData[3], pstData[i].pcData, pstData[i].usCanLen);
//             //hexdump(pktDiag[i]->pData, pstData[i].usCanLen+3);

//             pktDiag[i]->DataSize = pstData[i].usCanLen;
            
//             msgDiag[i]->mMsgType = MSG_DIAG;
//             msgDiag[i]->mPktType = PACKET_CAN;
//             msgDiag[i]->event = DIAG_PASSTHRU;
//             msgDiag[i]->subEvent = eDIAG_COMM_TX_START;
//             msgDiag[i]->unEventTime = GetUnixTime();
//             msgDiag[i]->pPacket = (void *)pktDiag[i];

//             // Changed to CMSIS v2: osMessageQueueSpacesAvailable and osMessageQueuePut
//             if( osMessageQueueGetSpace(hDiagMsg) == 0 )
//             {
//                 osMemoryPoolFree(hPTPKPool, pktDiag[i]);
//                 osMemoryPoolFree(hDiagPool, msgDiag[i]);
//             }
//             else
//             {
//                 osMessageQueuePut(hDiagMsg, &msgDiag[i], 0, osWaitForever);
//             }

//             if( eType == BatRelayConType_NEEV )
//             {            
//                 pstData[i].cAliveCnt++;
//                 memcpy(pstData[i].pcPreData, pstData[i].pcData, pstData[i].usCanLen);
//             }
//         }

//         osDelay(1);
//     }
// }

unsigned short ByteSwap(unsigned short usData)
{
    return ((usData >> 8) | (usData << 8));
}

/*======================================================================*/
/*  BSA Flexible TX/RX System Implementation                            */
/*======================================================================*/

// Global Variables for BSA Flexible System
stBSA_TX_MANAGER g_stBSA_TxManager;
bool g_bUseExternalTxConfig = false;
bool g_bBSA_DataConfigReady = false;
bool g_bBSA_SimframeConfigReady = false;
// static stBSA_COMM_DATA g_stBSA_TxSignals[BSA_MAX_TX_SIGNALS];
static stBSA_COMM_DATA g_stBSA_RxSignals[BSA_MAX_RX_SIGNALS];
// static uint8_t g_ucBSA_TxSignalCount = 0;
static uint8_t g_ucBSA_RxSignalCount = 0;
stBSA_MON_DATA g_stBSA_MonData;
stBSA_StepData g_stBSA_StepData = {0};

/**
 * @brief  Initialize BSA TX Manager structure
 */
void BSA_InitTxManager(void)
{
    memset(&g_stBSA_TxManager, 0, sizeof(stBSA_TX_MANAGER));
    g_stBSA_TxManager.txCount = 0;
    g_stBSA_TxManager.timerCount = 0;
}

/**
 * @brief  Add a new TX configuration
 * @param  canId: CAN identifier
 * @param  canType: CAN type (CAN_TYPE_STANDARD/EXTENDED/FD_STANDARD/FD_EXTENDED)
 * @param  cycle_ms: transmission cycle in milliseconds
 * @param  dataLen: data length in bytes
 * @retval configuration index (0-9), or -1 if full
 */
int BSA_AddTxConfig(uint16_t canId, uint8_t canType, uint16_t cycle_ms, uint8_t dataLen)
{
    if (g_stBSA_TxManager.txCount >= BSA_MAX_TX_SIGNALS) {
        return -1;  // TX signals array full
    }
    
    uint8_t idx = g_stBSA_TxManager.txCount;
    stBSA_TX_CONFIG *pCfg = &g_stBSA_TxManager.txConfigs[idx];
    
    pCfg->canId = canId;
    pCfg->canType = canType;
    pCfg->cycle_ms = cycle_ms;
    pCfg->dataLen = dataLen;
    pCfg->aliveCntPos = 0xFF;  // Not used by default
    pCfg->voltagePos = 0xFF;   // Not used by default
    pCfg->crcPos = 0xFF;       // Not used by default
    pCfg->enabled = false;
    pCfg->aliveCnt = 0;
    memset(pCfg->data, 0, BSA_MAX_DATA_LENGTH);
    
    g_stBSA_TxManager.txCount++;
    
    return idx;
}

/**
 * @brief  Set TX data buffer
 * @param  configIdx: configuration index
 * @param  pData: pointer to data buffer
 * @param  len: data length
 * @retval 0 if success, -1 if error
 */
int BSA_SetTxData(uint8_t configIdx, uint8_t* pData, uint8_t len)
{
    if (configIdx >= g_stBSA_TxManager.txCount) {
        return -1;
    }
    
    stBSA_TX_CONFIG *pCfg = &g_stBSA_TxManager.txConfigs[configIdx];
    
    if (len > pCfg->dataLen) {
        len = pCfg->dataLen;
    }
    
    memcpy(pCfg->data, pData, len);
    return 0;
}


/**
 * @brief  Set alive counter position
 * @param  configIdx: configuration index
 * @param  pos: byte position for alive counter
 * @retval 0 if success, -1 if error
 */
int BSA_SetTxAliveCounterPos(uint8_t configIdx, uint8_t pos)
{
    if (configIdx >= g_stBSA_TxManager.txCount) {
        return -1;
    }
    
    g_stBSA_TxManager.txConfigs[configIdx].aliveCntPos = pos;
    return 0;
}

/**
 * @brief  Set voltage position
 * @param  configIdx: configuration index
 * @param  pos: byte position for voltage (2 bytes)
 * @retval 0 if success, -1 if error
 */
int BSA_SetTxVoltagePos(uint8_t configIdx, uint8_t pos)
{
    if (configIdx >= g_stBSA_TxManager.txCount) {
        return -1;
    }
    
    g_stBSA_TxManager.txConfigs[configIdx].voltagePos = pos;
    return 0;
}

/**
 * @brief  Set CRC position
 * @param  configIdx: configuration index
 * @param  pos: byte position for CRC (2 bytes)
 * @retval 0 if success, -1 if error
 */
int BSA_SetTxCRCPos(uint8_t configIdx, uint8_t pos)
{
    if (configIdx >= g_stBSA_TxManager.txCount) {
        return -1;
    }
    
    g_stBSA_TxManager.txConfigs[configIdx].crcPos = pos;
    return 0;
}

/**
 * @brief  Enable or disable a TX configuration
 * @param  configIdx: configuration index
 * @param  enable: true to enable, false to disable
 * @retval 0 if success, -1 if error
 */
int BSA_EnableTxConfig(uint8_t configIdx, bool enable)
{
    if (configIdx >= g_stBSA_TxManager.txCount) {
        return -1;
    }
    
    g_stBSA_TxManager.txConfigs[configIdx].enabled = enable;
    return 0;
}

/**
 * @brief  Enable or disable external TX configuration
 * @param  enable: true to use BSA flexible TX, false to use legacy TX
 */
void BSA_EnableExternalTx(bool enable)
{
    g_bUseExternalTxConfig = enable;
}

/**
 * @brief  Process TX packets based on cycle timers
 * @param  currentTime: current time in milliseconds
 * @note   This function should be called periodically from BatRelayConThread
 */
void BSA_ProcessTxPackets(uint32_t currentTime)
{
    if (!g_bUseExternalTxConfig) {
        return;  // External TX not enabled
    }
    
    for (uint8_t i = 0; i < g_stBSA_TxManager.txCount; i++) {
        stBSA_TX_CONFIG *pCfg = &g_stBSA_TxManager.txConfigs[i];
        
        if (!pCfg->enabled) {
            continue;  // Skip disabled configs
        }
        
        // Find matching timer
        int timerIdx = -1;
        for (uint8_t j = 0; j < g_stBSA_TxManager.timerCount; j++) {
            if (g_stBSA_TxManager.timers[j].cycle_ms == pCfg->cycle_ms) {
                timerIdx = j;
                break;
            }
        }
        
        // Create timer if not found
        if (timerIdx == -1) {
            if (g_stBSA_TxManager.timerCount >= BSA_MAX_CYCLES) {
                continue;  // Too many different cycles
            }
            timerIdx = g_stBSA_TxManager.timerCount;
            g_stBSA_TxManager.timers[timerIdx].cycle_ms = pCfg->cycle_ms;
            g_stBSA_TxManager.timers[timerIdx].last_time = currentTime;
            g_stBSA_TxManager.timerCount++;
        }
        
        // Check if it's time to transmit
        stBSA_TX_TIMER *pTimer = &g_stBSA_TxManager.timers[timerIdx];
        uint32_t elapsed = Get_TmrDelta(currentTime, pTimer->last_time);
        
        if (elapsed >= pCfg->cycle_ms) {
            uint8_t txData[BSA_MAX_DATA_LENGTH];
            memcpy(txData, pCfg->data, pCfg->dataLen);
            
            // Update alive counter if configured
            if (pCfg->aliveCntPos != 0xFF && pCfg->aliveCntPos < pCfg->dataLen) {
                txData[pCfg->aliveCntPos] = pCfg->aliveCnt;
                pCfg->aliveCnt = (pCfg->aliveCnt + 1) & 0x0F;  // 4-bit counter
            }
            
            // Update voltage if configured
            if (pCfg->voltagePos != 0xFF && (pCfg->voltagePos + 1) < pCfg->dataLen) {
                uint16_t voltage = 0;
                if (g_eBatRelayConType == BatRelayConType_NEEV) {
                    voltage = g_usNEBattPackVolt;
                } else if (g_eBatRelayConType == BatRelayConType_OSEV) {
                    voltage = g_usOSBattPackVolt;
                }
                memcpy(&txData[pCfg->voltagePos], &voltage, 2);
            }
            
            // Calculate and insert CRC if configured
            if (pCfg->crcPos != 0xFF && (pCfg->crcPos + 1) < pCfg->dataLen) {
                // Prepare CRC calculation buffer (same as MakeNeEvTxPacket logic)
                uint8_t crcBuf[32];
                memset(crcBuf, 0, 32);
                
                // Copy data starting from byte 2 to byte 31 (30 bytes)
                if (pCfg->dataLen >= 32) {
                    memcpy(crcBuf, &txData[2], 30);
                    
                    // Insert alive counter at position 0 of CRC buffer
                    if (pCfg->aliveCntPos != 0xFF) {
                        crcBuf[0] = txData[pCfg->aliveCntPos];
                    }
                    
                    // Add CAN ID + offset at position 30-31
                    uint16_t canIdCalc = pCfg->canId + CRC16_ADD_VALUE;
                    memcpy(&crcBuf[30], &canIdCalc, 2);
                    
                    // Calculate CRC16
                    uint16_t crc = CalculateCRC16((char*)crcBuf, 32);
                    memcpy(&txData[pCfg->crcPos], &crc, 2);
                }
            }
            
            // Transmit packet
            BSA_SendCanPacket(pCfg->canId, pCfg->canType, txData, pCfg->dataLen);
            
            // Update timer
            pTimer->last_time = currentTime;
        }
    }
}

/**
 * @brief  Extract bits from CAN data field
 * @param  pData: pointer to data buffer
 * @param  startBit: start bit position
 * @param  bitLength: number of bits to extract
 * @retval extracted value (up to 32 bits)
 */
static uint32_t ExtractBitsFromData(uint8_t* pData, uint8_t startBit, uint8_t bitLength)
{
    uint32_t value = 0;
    uint8_t startByte = startBit / 8;
    uint8_t startBitInByte = startBit % 8;
    
    // Read up to 5 bytes to cover any bit range up to 32 bits
    uint64_t temp = 0;
    for (int i = 0; i < 5 && (startByte + i) < BSA_MAX_DATA_LENGTH; i++) {
        temp |= ((uint64_t)pData[startByte + i]) << (i * 8);
    }
    
    // Shift and mask to extract the desired bits
    temp >>= startBitInByte;
    value = (uint32_t)(temp & ((1ULL << bitLength) - 1));
    
    return value;
}

/**
 * @brief  Process received BSA RX signal
 * @param  canId: received CAN ID
 * @param  pData: pointer to received data
 * @param  dataLen: data length
 */
void Process_BSA_RxSignal(uint16_t canId, uint8_t* pData, uint8_t dataLen)
{
    // Process all configured RX signals
    for (uint8_t i = 0; i < g_ucBSA_RxSignalCount; i++) {
        stBSA_COMM_DATA *pSig = &g_stBSA_RxSignals[i];
        
        // Check if CAN ID matches
        uint32_t sigCanId = 0;
        memcpy(&sigCanId, pSig->BSA_canId, 4);
        
        if (sigCanId == canId) {
            // Extract signal value
            uint32_t rawValue = ExtractBitsFromData(pData, pSig->BSA_start_bit, pSig->BSA_bit_length);
            
            // Apply scale factor and offset
            float scaledValue = (rawValue * pSig->BSA_scale_factor) + pSig->BSA_offset;
            
            // Store in monitoring data structure based on signal type
            switch (pSig->BSA_signal_type) {
                case eSignalType_BSAVoltage:
                    g_stBSA_MonData.mosa_BSA_voltage = (uint16_t)scaledValue;
                    
                    // Update global voltage variables
                    if (g_eBatRelayConType == BatRelayConType_NEEV) {
                        g_usNEBattPackVolt = (uint16_t)scaledValue;
                    } else if (g_eBatRelayConType == BatRelayConType_OSEV) {
                        g_usOSBattPackVolt = (uint16_t)scaledValue;
                    }
                    break;
                    
                case eSignalType_PrechargeRelayStatus:
                    g_stBSA_MonData.mosa_PreRly_status = (uint8_t)rawValue;
                    g_ucNEPreChrgState = (uint8_t)rawValue;
                    break;
                    
                case eSignalType_MainRelayStatus:
                    g_stBSA_MonData.mosa_MainRly_status = (uint8_t)rawValue;
                    g_ucNEMainRlyOnState = (uint8_t)rawValue;
                    break;
                    
                case eSignalType_IGStatus:
                    g_stBSA_MonData.mosa_IGON_status = (uint8_t)rawValue;
                    break;
                    
                default:
                    break;
            }
        }
    }
}

/*======================================================================*/
/*  Dynamic Config Functions (BLE-configured monitoring/simframe)       */
/*======================================================================*/

/**
 * @brief  Apply conversion rule to raw value
 * @param  rawValue: extracted raw data
 * @param  pRule: conversion rule to apply
 * @retval converted float value
 */
static float BSA_ApplyConversion(uint32_t rawValue, stBSA_ConvRule *pRule)
{
    switch (pRule->convType) {
        case 0:  // No conversion
            return (float)rawValue;

        case 1: {
            // Linear conversion: result = rawValue * convA
            //   convB != 0 이면:  result /= convB
            //   convF > 0 이면:   result /= 10^convF  (소수점 자릿수)
            //
            // 예: rawValue=8960, A=1, B=0, F=1 → 8960 * 1 / 10^1 = 896
            // 예: rawValue=500,  A=2, B=5, F=0 → 500 * 2 / 5 = 200
            float result = (float)rawValue * (float)pRule->convA;
            if (pRule->convB != 0) {
                result /= (float)pRule->convB;
            }
            if (pRule->convF > 0) {
                float divisor = 1.0f;
                for (uint8_t d = 0; d < pRule->convF; d++) {
                    divisor *= 10.0f;
                }
                result /= divisor;
            }
            return result;
        }

        default:
            return (float)rawValue;
    }
}

/**
 * @brief  Dynamic monitoring using g_stBSA_DataConfigs[] 배열
 *         Called from BatRelayMonitorThread when BLE config is ready
 *
 *  모든 monitoring config를 순회하며, rxCanId가 매칭되는 config의 messages 처리
 *
 *  valueType 처리:
 *    0x01 = Voltage, 0x02 = Precharge Relay, 0x03 = Main Relay, 0x04 = IG Status
 *    0x00 = Auto-detect (BLE에서 type=0으로 보내는 경우, dataSize/conv 기반 자동 판별)
 *
 *  Each message entry:
 *    valueType     → signal type (or 0 for auto)
 *    startPosition → byte offset in CAN data
 *    dataSize      → bytes to extract
 *    maskingValue  → bit mask to apply
 *    convRule      → conversion (scale/offset)
 */
void BSA_ApplyDataConfig_Monitoring(uint16_t rxCanId, uint8_t* pData, uint8_t dataLen)
{
    if (!g_bBSA_DataConfigReady) return;

    for (uint8_t cfgIdx = 0; cfgIdx < g_ucBSA_DataConfigCount; cfgIdx++) {
        stBSA_DataConfig *pCfg = &g_stBSA_DataConfigs[cfgIdx];

        if (pCfg->messageCount == 0) continue;
        if (rxCanId != pCfg->responseValue) continue;

        for (uint8_t i = 0; i < pCfg->messageCount; i++) {
            stBSA_DataConfigMsg *pMsg = &pCfg->messages[i];

            // Bounds check
            if (pMsg->startPosition + pMsg->dataSize > dataLen) continue;

            // Extract raw value (little-endian, up to 4 bytes)
            uint32_t rawValue = 0;
            for (uint8_t b = 0; b < pMsg->dataSize && b < 4; b++) {
                rawValue |= ((uint32_t)pData[pMsg->startPosition + b]) << (b * 8);
            }

            // Apply masking
            if (pMsg->maskingValue != 0) {
                rawValue &= pMsg->maskingValue;
            }

            // Apply conversion
            float convertedValue = BSA_ApplyConversion(rawValue, &pMsg->convRule);

            // Determine effective valueType
            // BLE에서 type=0으로 보내는 경우: dataSize와 convRule로 자동 판별
            uint8_t effectiveType = pMsg->valueType;
            if (effectiveType == 0) {
                if (pMsg->dataSize >= 2 && pMsg->convRule.convType != 0) {
                    // conversion이 있고 2바이트 이상 → voltage
                    effectiveType = BSA_VALUETYPE_MON_VOLTAGE;
                } else if (pMsg->dataSize == 1) {
                    // 1바이트: 해당 Config 내 relay 메시지 순서로 판별
                    // relayMsgIdx: 이 Config 내에서 몇 번째 1바이트 메시지인지
                    uint8_t relayMsgIdx = 0;
                    for (uint8_t k = 0; k < i; k++) {
                        if (pCfg->messages[k].dataSize == 1 &&
                            pCfg->messages[k].valueType == 0) {
                            relayMsgIdx++;
                        }
                    }
                    // 해당 Config에 1바이트 메시지가 1개만 있으면:
                    //   전체 Config 중 이것이 첫 번째 config면 mainRly, 아니면 preRly
                    if (pCfg->messageCount == 1) {
                        // 단독 1바이트 Config → main relay
                        effectiveType = BSA_VALUETYPE_MON_MAIN_RLY;
                    } else if (relayMsgIdx == 0) {
                        effectiveType = BSA_VALUETYPE_MON_PRECHARGE_RLY;
                    } else {
                        effectiveType = BSA_VALUETYPE_MON_MAIN_RLY;
                    }
                } else if (pMsg->dataSize >= 2) {
                    // 2바이트 + no conversion → voltage (raw)
                    effectiveType = BSA_VALUETYPE_MON_VOLTAGE;
                }
            }

            // Store based on effectiveType
            switch (effectiveType) {
                case BSA_VALUETYPE_MON_VOLTAGE:
                    g_stBSA_MonData.mosa_BSA_voltage = (uint16_t)convertedValue;
                    g_usNEBattPackVolt = (uint16_t)convertedValue;
                    break;

                case BSA_VALUETYPE_MON_PRECHARGE_RLY:
                    g_stBSA_MonData.mosa_PreRly_status = (uint8_t)rawValue;
                    g_ucNEPreChrgState = (uint8_t)rawValue;
                    break;

                case BSA_VALUETYPE_MON_MAIN_RLY:
                    g_stBSA_MonData.mosa_MainRly_status = (uint8_t)rawValue;
                    g_ucNEMainRlyOnState = (uint8_t)rawValue;
                    break;

                case BSA_VALUETYPE_MON_IG_STATUS:
                    g_stBSA_MonData.mosa_IGON_status = (uint8_t)rawValue;
                    break;

                default:
                    break;
            }
        }
    }
}

/**
 * @brief  Reset simframe TX state (call on mode stop/exit)
 *         모든 simframe 슬롯의 alive counter / timer 초기화
 */
void BSA_ResetSimframeState(void)
{
    uint32_t now = Get_Tmr();
    for (uint8_t i = 0; i < BSA_MAX_SIMFRAME_CONFIGS; i++) {
        g_SimframeTxTimers[i] = now;
        g_SimframeAliveCnts[i] = 0;
    }
}

/**
 * @brief  Start BSA operation using BLE-configured values
 *         BLE에서 설정된 Monitoring/Simframe config로 CAN TX/RX 시작
 *         CLI 'bsa start' 에서 호출
 */
void BSA_Start(void)
{
    // 이미 동작 중이면 중복 시작 방지
    if (g_bBatRelayConFlag) {
        printf("[BSA] Already running. Stop first.\r\n");
        return;
    }

    // Config 존재 여부 확인
    if (g_ucBSA_SimframeConfigCount == 0 && g_ucBSA_DataConfigCount == 0) {
        printf("[BSA] No config loaded. Send BLE 0x21/0x22 or use 'bsa setmon/setsim' first.\r\n");
        return;
    }

    // FDCAN 상태 확인 및 에러 복구
    //  · BUSY 가 아닌 모든 상태 (RESET / READY / ERROR) 에서 재초기화 필요
    //    - BSA_Stop 후엔 READY 상태 → CAN_HW_setting 으로 재init+start 해야 TX 가능
    extern FDCAN_HandleTypeDef hfdcan1;
    if (hfdcan1.State != HAL_FDCAN_STATE_BUSY) {
        printf("[BSA] FDCAN1 not running (state=%d). Initializing...\r\n", hfdcan1.State);
        HAL_StatusTypeDef ret = CAN_HW_setting();
        if (ret != HAL_OK) {
            printf("[BSA] CAN HW init failed (ret=%d)\r\n", ret);
            return;
        }
        printf("[BSA] CAN HW init OK\r\n");
    }

    osDelay(500);

    // FDCAN 프로토콜 상태 출력 및 에러 복구
    FDCAN_ProtocolStatusTypeDef protStatus;
    if (HAL_FDCAN_GetProtocolStatus(&hfdcan1, &protStatus) == HAL_OK) {
        printf("[BSA] FDCAN PSR: BusOff=%d, ErrPassive=%d, ErrWarn=%d, LEC=%lu, DLEC=%lu\r\n",
               protStatus.BusOff, protStatus.ErrorPassive, protStatus.Warning,
               protStatus.LastErrorCode, protStatus.DataLastErrorCode);

        // BusOff, ErrPassive, ErrWarn 중 하나라도 있으면 FDCAN 재초기화
        // → 에러 카운터가 쌓여있으면 TX FIFO가 영원히 안 비워지므로 반드시 재초기화 필요
        if (protStatus.BusOff || protStatus.ErrorPassive || protStatus.Warning) {
            printf("[BSA] FDCAN has errors (BusOff=%d,ErrPassive=%d,Warn=%d). Reinitializing CAN HW...\r\n",
                   protStatus.BusOff, protStatus.ErrorPassive, protStatus.Warning);
            HAL_StatusTypeDef ret = CAN_HW_setting();
            if (ret != HAL_OK) {
                printf("[BSA] CAN HW re-init failed (ret=%d)\r\n", ret);
                return;
            }
            // 재초기화 후 상태 확인
            HAL_FDCAN_GetProtocolStatus(&hfdcan1, &protStatus);
            printf("[BSA] CAN HW re-init OK. PSR: BusOff=%d, ErrPassive=%d, ErrWarn=%d\r\n",
                   protStatus.BusOff, protStatus.ErrorPassive, protStatus.Warning);
        }
    }

    // RX 큐 flush (이전 세션의 stale data 제거)
    if (receiveCAN_Q != NULL) {
        xQueueReset(receiveCAN_Q);
        printf("[BSA] RX queue flushed\r\n");
    }

    // TX/RX 스레드 생성 (최초 1회만)
    static bool bThreadsCreated = false;
    if (!bThreadsCreated) {
        if (!StartBSAThread()) {
            printf("[BSA] Failed to create TX/RX threads!\r\n");
            return;
        }
        bThreadsCreated = true;
        printf("[BSA] TX/RX threads created\r\n");
    }

    // Config Ready 플래그 설정
    if (g_ucBSA_SimframeConfigCount > 0) {
        g_bBSA_SimframeConfigReady = true;
    }
    if (g_ucBSA_DataConfigCount > 0) {
        g_bBSA_DataConfigReady = true;
    }

    // Simframe 타이머/카운터 초기화
    BSA_ResetSimframeState();

    // Legacy 타이머도 초기화 (fallback 대비)
    m_unBatRelayCon_10ms_Timer = Get_Tmr();
    m_unBatRelayCon_100ms_Timer = Get_Tmr();

    // 모니터링 데이터 초기화
    memset(&g_stBSA_MonData, 0, sizeof(stBSA_MON_DATA));
    g_usNEBattPackVolt = 0;
    g_ucNEPreChrgState = 0;
    g_ucNEMainRlyOnState = 0;

    // 디버그 카운터 리셋
    s_txOkCnt = 0;
    s_txFailCnt = 0;
    s_rxCnt = 0;

    // 동작 시작
    g_bBatRelayConFlag = true;

    // TX config 정보 출력
    for (uint8_t i = 0; i < g_ucBSA_SimframeConfigCount; i++) {
        stBSA_SimframeConfig *p = &g_stBSA_SimframeConfigs[i];
        uint16_t cid = p->requestValue[0] | (p->requestValue[1] << 8);
        bool fd = (p->protocolId == BSA_PROTOCOL_CAN_FD);
        printf("[BSA] TX[%d] CAN_ID=0x%04X %s interval=%dms dataLen=%d\r\n",
               i, cid, fd ? "FD" : "Classic",
               p->interval_ms, p->requestValueLen - BSA_REQVAL_CANID_SIZE);
    }

    // RX config 정보 출력
    for (uint8_t i = 0; i < g_ucBSA_DataConfigCount; i++) {
        stBSA_DataConfig *p = &g_stBSA_DataConfigs[i];
        printf("[BSA] RX[%d] CAN_ID=0x%04X msgCount=%d\r\n",
               i, p->responseValue, p->messageCount);
    }

    printf("[BSA] Started - TX:%d configs, RX:%d configs\r\n",
           g_ucBSA_SimframeConfigCount, g_ucBSA_DataConfigCount);

    // /* EXT_RLY1 ON (21.5Ω) */
    // IO_EXT_RLY_control(1, true);
    IO_AD_RELAY_control(1, 1, true);

    IO_Resistance_Relay_Control(g_dbConfig.resistance, true);

    /* FAN ON */
    IO_SS_control(1, true); 
    IO_SS_control(2, true);
    IO_SS_control(3, true);
    IO_SS_control(4, true);
}

/**
 * @brief  Stop BSA operation
 *         CAN TX/RX 중단, 상태 초기화 (config는 유지)
 *         CLI 'bsa stop' 에서 호출
 */
void BSA_Stop(void)
{
    if (!g_bBatRelayConFlag) {
        printf("[BSA] Already stopped.\r\n");
        return;
    }

    // 동작 중단 (SW level: BatRelayConThread 의 TX 호출 차단)
    g_bBatRelayConFlag = false;

    // Ready 플래그 해제 (config 데이터는 유지)
    g_bBSA_SimframeConfigReady = false;
    g_bBSA_DataConfigReady = false;

    // Simframe 상태 초기화
    BSA_ResetSimframeState();

    // 모니터링 데이터 초기화
    memset(&g_stBSA_MonData, 0, sizeof(stBSA_MON_DATA));
    g_usNEBattPackVolt = 0;
    g_ucNEPreChrgState = 0;
    g_ucNEMainRlyOnState = 0;
    g_usOSBattPackVolt = 0;

    /* HW level: FDCAN1 stop → TX FIFO 잔존 frame flush + AutoRetransmission 중단
     *  · BSA_Start 시 CAN_HW_setting() 으로 재init+start 되므로 재시작 OK
     *  · 추가: state 를 RESET 으로 강제 → 다음 HAL_FDCAN_Init 가 첫 init 처럼
     *    MspInit 부터 깨끗하게 재실행 (READY 상태에서 재 init 시 간헐적 HAL_BUSY
     *    실패 방지) */
    extern FDCAN_HandleTypeDef hfdcan1;
    if (hfdcan1.State == HAL_FDCAN_STATE_BUSY) {
        HAL_FDCAN_Stop(&hfdcan1);
        printf("[BSA] FDCAN1 stopped (TX FIFO flushed)\r\n");
    }
    hfdcan1.State = HAL_FDCAN_STATE_RESET;
    printf("[BSA] FDCAN1 state forced to RESET (clean re-init on next start)\r\n");

    printf("[BSA] Stopped. Configs preserved (%d TX, %d RX). Use 'bsa start' to restart.\r\n",
           g_ucBSA_SimframeConfigCount, g_ucBSA_DataConfigCount);
}

/**
 * @brief  Dynamic TX using g_stBSA_SimframeConfigs[] 배열
 *         Called from BatRelayConThread when BLE config is ready
 *
 *  모든 simframe 설정을 순회하며, 각각 독립된 interval/alive counter로 TX
 *
 *  Flow per config:
 *    1. Extract CAN ID from requestValue[0..1]
 *    2. Copy CAN data from requestValue[2:] as base TX data
 *    3. Insert alive counter at message startPosition (valueType=0x02)
 *    4. Insert battery voltage at message startPosition (valueType=0x03)
 *    5. For CAN FD: calculate CRC16 over (data excluding CRC bytes + CAN_ID+0xF800),
 *       insert at message startPosition (valueType=0x01)
 *    6. Transmit via BSA_SendCanPacket
 */
void BSA_ProcessSimframeTx(uint32_t currentTime)
{
    if (!g_bBSA_SimframeConfigReady) return;
    if (g_ucBSA_SimframeConfigCount == 0) return;

    for (uint8_t cfgIdx = 0; cfgIdx < g_ucBSA_SimframeConfigCount; cfgIdx++) {
        stBSA_SimframeConfig *pSimCfg = &g_stBSA_SimframeConfigs[cfgIdx];

        if (pSimCfg->messageCount == 0) continue;
        if (pSimCfg->requestValueLen <= BSA_REQVAL_CANID_SIZE) continue;

        // Check interval (각 config 독립)
        if (Get_TmrDelta(currentTime, g_SimframeTxTimers[cfgIdx]) < pSimCfg->interval_ms) continue;
        g_SimframeTxTimers[cfgIdx] = currentTime;

        // ── Extract CAN ID from requestValue[0..1] ──
        uint16_t canId = pSimCfg->requestValue[0] | (pSimCfg->requestValue[1] << 8);

        // ── Copy CAN data (requestValue[2:]) as TX template ──
        uint8_t txData[BSA_MAX_DATA_LENGTH];
        uint8_t dataLen = pSimCfg->requestValueLen - BSA_REQVAL_CANID_SIZE;
        if (dataLen > BSA_MAX_DATA_LENGTH) dataLen = BSA_MAX_DATA_LENGTH;
        memcpy(txData, &pSimCfg->requestValue[BSA_REQVAL_CANID_SIZE], dataLen);

        // ── Determine CAN type from protocolId + CAN ID ──
        bool isFD = (pSimCfg->protocolId == BSA_PROTOCOL_CAN_FD);
        uint8_t canType;
        if (isFD) {
            canType = (canId > 0x7FF) ? CAN_TYPE_FD_EXTENDED : CAN_TYPE_FD_STANDARD;
        } else {
            canType = (canId > 0x7FF) ? CAN_TYPE_EXTENDED : CAN_TYPE_STANDARD;
        }

        uint8_t crcPos = 0xFF;
        uint8_t crcSize = 0;

        // ── Pass 1: Insert alive counter & voltage, record CRC position ──
        for (uint8_t i = 0; i < pSimCfg->messageCount; i++) {
            stBSA_DataConfigMsg *pMsg = &pSimCfg->messages[i];

            switch (pMsg->valueType) {
                case BSA_VALUETYPE_SIM_ALIVE_COUNT:
                    if (pMsg->startPosition < dataLen) {
                        txData[pMsg->startPosition] = g_SimframeAliveCnts[cfgIdx];
                    }
                    break;

                case BSA_VALUETYPE_SIM_VOLTAGE:
                    if (pMsg->startPosition + pMsg->dataSize <= dataLen) {
                        uint16_t voltage = g_stBSA_MonData.mosa_BSA_voltage;
                        memcpy(&txData[pMsg->startPosition], &voltage,
                               (pMsg->dataSize > 2) ? 2 : pMsg->dataSize);
                    }
                    break;

                case BSA_VALUETYPE_SIM_CRC16:
                    crcPos = pMsg->startPosition;
                    crcSize = pMsg->dataSize;
                    break;

                default:
                    break;
            }
        }

        // ── Pass 2: Calculate CRC16 for CAN FD packets ──
        if (isFD && crcPos != 0xFF && crcSize >= 2) {
            uint8_t crcBuf[BSA_MAX_DATA_LENGTH + 2];
            uint8_t crcBufLen = 0;

            for (uint8_t j = 0; j < dataLen; j++) {
                if (j >= crcPos && j < crcPos + crcSize) continue;
                crcBuf[crcBufLen++] = txData[j];
            }

            // Append CAN ID + 0xF800
            uint16_t canIdCalc = canId + CRC16_ADD_VALUE;
            memcpy(&crcBuf[crcBufLen], &canIdCalc, 2);
            crcBufLen += 2;

            uint16_t crc = CalculateCRC16((char*)crcBuf, crcBufLen);
            memcpy(&txData[crcPos], &crc, 2);
        }

        // ── Increment alive counter (각 config 독립) ──
        g_SimframeAliveCnts[cfgIdx]++;

        // ── Transmit ──
        HAL_StatusTypeDef txRet = BSA_SendCanPacket(canId, canType, txData, dataLen);

        // 첫 전송 또는 에러 시 로그 출력 (BSA_Start에서 리셋됨)
        if (txRet == HAL_OK) {
            s_txOkCnt++;
            if (s_txOkCnt <= 3) {
                printf("[BSA TX] OK cfg[%d] ID=0x%04X type=%d len=%d alive=%d\r\n",
                       cfgIdx, canId, canType, dataLen, g_SimframeAliveCnts[cfgIdx] - 1);
            }
        } else {
            s_txFailCnt++;
            if (s_txFailCnt <= 5 || (s_txFailCnt % 100) == 0) {
                extern FDCAN_HandleTypeDef hfdcan1;
                FDCAN_ProtocolStatusTypeDef psr;
                HAL_FDCAN_GetProtocolStatus(&hfdcan1, &psr);
                printf("[BSA TX] FAIL cfg[%d] ID=0x%04X type=%d len=%d ret=%d (ok=%lu,fail=%lu) BusOff=%d LEC=%lu\r\n",
                       cfgIdx, canId, canType, dataLen, txRet,
                       s_txOkCnt, s_txFailCnt, psr.BusOff, psr.LastErrorCode);
            }
        }
    }
}

/* Timer helpers (Get_Tmr / Get_TmrDelta / GetUnixTime) moved to sys-main.c.
 * Declarations are in sys-common.h (included above). */

/**
 * @brief  Initialize GIT Set Configuration structure
 * @note   Placeholder for legacy VCI3 configuration initialization
 */
void InitGITSetConfig(void)
{
    // TODO: Implement configuration initialization when needed
    // This function was used in VCI3 to initialize communication parameters
    // For EVCD, these may be handled differently
}

/**
 * @brief  Initialize GIT HW Set Data structure
 * @note   Placeholder for legacy VCI3 hardware data initialization
 */
void InitGITHWSetData(void)
{
    // TODO: Implement hardware data initialization when needed
    // This function was used in VCI3 to set hardware-specific parameters
    // For EVCD, hardware configuration is handled via CubeMX/HAL
}

/**
 * @brief  VCI Hardware Setting (legacy compatibility function)
 * @param  hwType: Hardware type identifier
 * @note   Placeholder - EVCD uses different hardware configuration approach
 */
void VCI_HW_Setting(uint32_t hwType)
{
    // TODO: Implement hardware-specific settings when needed
    // In VCI3, this configured CAN controller, filters, etc.
    // In EVCD, use CAN_HW_setting() from task-can.c instead
    (void)hwType;  // Suppress unused parameter warning
}

#if 0
/**
 * @brief  Initialize BSA database for NE EV vehicle type
 * @note   This configures the flexible TX/RX system for NE EV
 */
void BSA_ConfigureForNEEV(void)
{
    BSA_InitTxManager();
    g_ucBSA_TxSignalCount = 0;
    g_ucBSA_RxSignalCount = 0;
    
    // Configure TX signals for NE EV
    // TX Signal 1: CAN ID 0x0035 (32 bytes, 10ms cycle, CAN FD)
    int idx0 = BSA_AddTxConfig(0x0035, CAN_TYPE_FD_STANDARD, 10, 32);
    if (idx0 >= 0) {
        uint8_t initData[32] = {
            0x00, 0x00, 0x00, 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00
        };
        BSA_SetTxData(idx0, initData, 32);
        BSA_SetTxAliveCounterPos(idx0, 2);  // Alive counter at byte 2
        BSA_SetTxCRCPos(idx0, 0);           // CRC at byte 0-1
        BSA_EnableTxConfig(idx0, true);
    }
    
    // TX Signal 2: CAN ID 0x010A (32 bytes, 10ms cycle, CAN FD)
    int idx1 = BSA_AddTxConfig(0x010A, CAN_TYPE_FD_STANDARD, 10, 32);
    if (idx1 >= 0) {
        uint8_t initData[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00
        };
        BSA_SetTxData(idx1, initData, 32);
        BSA_SetTxAliveCounterPos(idx1, 2);
        BSA_SetTxVoltagePos(idx1, 16);      // Voltage at byte 16-17
        BSA_SetTxCRCPos(idx1, 0);
        BSA_EnableTxConfig(idx1, true);
    }
    
    // TX Signal 3: CAN ID 0x0120 (32 bytes, 10ms cycle, CAN FD)
    int idx2 = BSA_AddTxConfig(0x0120, CAN_TYPE_FD_STANDARD, 10, 32);
    if (idx2 >= 0) {
        uint8_t initData[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00
        };
        BSA_SetTxData(idx2, initData, 32);
        BSA_SetTxAliveCounterPos(idx2, 2);
        BSA_SetTxVoltagePos(idx2, 16);
        BSA_SetTxCRCPos(idx2, 0);
        BSA_EnableTxConfig(idx2, true);
    }
    
    // TX Signal 4: CAN ID 0x02AA (32 bytes, 100ms cycle, CAN FD)
    int idx3 = BSA_AddTxConfig(0x02AA, CAN_TYPE_FD_STANDARD, 100, 32);
    if (idx3 >= 0) {
        BSA_SetTxAliveCounterPos(idx3, 2);
        BSA_SetTxCRCPos(idx3, 0);
        BSA_EnableTxConfig(idx3, true);
    }
    
    // Configure RX signals for NE EV
    // RX Signal 1: Battery Voltage from 0x0235
    g_stBSA_RxSignals[0].BSA_canId[0] = 0x35;
    g_stBSA_RxSignals[0].BSA_canId[1] = 0x02;
    g_stBSA_RxSignals[0].BSA_start_bit = NE_BATPACP_VOLT_DATA_POSITION * 8;  // Byte 13 = bit 104
    g_stBSA_RxSignals[0].BSA_bit_length = 16;
    g_stBSA_RxSignals[0].BSA_scale_factor = NE_BATPACP_VOLT_FACTOR;
    g_stBSA_RxSignals[0].BSA_offset = 0.0f;
    g_stBSA_RxSignals[0].BSA_signal_type = eSignalType_BSAVoltage;
    g_ucBSA_RxSignalCount++;
    
    // RX Signal 2: Precharge Relay Status from 0x0235
    g_stBSA_RxSignals[1].BSA_canId[0] = 0x35;
    g_stBSA_RxSignals[1].BSA_canId[1] = 0x02;
    g_stBSA_RxSignals[1].BSA_start_bit = (NE_CHRG_STATE_DATA_POSITION * 8) + 4;  // Byte 3, bit 4
    g_stBSA_RxSignals[1].BSA_bit_length = 2;
    g_stBSA_RxSignals[1].BSA_scale_factor = 1.0f;
    g_stBSA_RxSignals[1].BSA_offset = 0.0f;
    g_stBSA_RxSignals[1].BSA_signal_type = eSignalType_PrechargeRelayStatus;
    g_ucBSA_RxSignalCount++;
    
    // RX Signal 3: Main Relay Status from 0x02FA
    g_stBSA_RxSignals[2].BSA_canId[0] = 0xFA;
    g_stBSA_RxSignals[2].BSA_canId[1] = 0x02;
    g_stBSA_RxSignals[2].BSA_start_bit = (NE_CHRG_STATE_DATA_POSITION * 8) + 2;  // Byte 3, bit 2
    g_stBSA_RxSignals[2].BSA_bit_length = 2;
    g_stBSA_RxSignals[2].BSA_scale_factor = 1.0f;
    g_stBSA_RxSignals[2].BSA_offset = 0.0f;
    g_stBSA_RxSignals[2].BSA_signal_type = eSignalType_MainRelayStatus;
    g_ucBSA_RxSignalCount++;
    
    // Enable external TX configuration
    BSA_EnableExternalTx(true);
    
    printf("[BSA] Configured for NE EV - TX:%d signals, RX:%d signals\r\n", 
           g_stBSA_TxManager.txCount, g_ucBSA_RxSignalCount);
}

/**
 * @brief  Initialize BSA database for OS EV vehicle type
 * @note   This configures the flexible TX/RX system for OS EV
 */
void BSA_ConfigureForOSEV(void)
{
    BSA_InitTxManager();
    g_ucBSA_TxSignalCount = 0;
    g_ucBSA_RxSignalCount = 0;
    
    // Configure TX signals for OS EV (Classic CAN, 8 bytes)
    // TX Signal 1: CAN ID 0x0200 (8 bytes, 100ms cycle)
    int idx0 = BSA_AddTxConfig(0x0200, CAN_TYPE_STANDARD, 100, 8);
    if (idx0 >= 0) {
        uint8_t initData[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00};
        BSA_SetTxData(idx0, initData, 8);
        BSA_EnableTxConfig(idx0, true);
    }
    
    // TX Signal 2: CAN ID 0x0291 (8 bytes, 100ms cycle)
    int idx1 = BSA_AddTxConfig(0x0291, CAN_TYPE_STANDARD, 100, 8);
    if (idx1 >= 0) {
        BSA_EnableTxConfig(idx1, true);
    }
    
    // TX Signal 3: CAN ID 0x0523 (8 bytes, 100ms cycle)
    int idx2 = BSA_AddTxConfig(0x0523, CAN_TYPE_STANDARD, 100, 8);
    if (idx2 >= 0) {
        uint8_t initData[8] = {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        BSA_SetTxData(idx2, initData, 8);
        BSA_EnableTxConfig(idx2, true);
    }
    
    // TX Signal 4: CAN ID 0x0524 (8 bytes, 100ms cycle)
    int idx3 = BSA_AddTxConfig(0x0524, CAN_TYPE_STANDARD, 100, 8);
    if (idx3 >= 0) {
        BSA_SetTxVoltagePos(idx3, 0);  // Voltage at byte 0-1
        BSA_EnableTxConfig(idx3, true);
    }
    
    // TX Signal 5: CAN ID 0x0211 (8 bytes, 100ms cycle)
    int idx4 = BSA_AddTxConfig(0x0211, CAN_TYPE_STANDARD, 100, 8);
    if (idx4 >= 0) {
        uint8_t initData[8] = {0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        BSA_SetTxData(idx4, initData, 8);
        BSA_EnableTxConfig(idx4, true);
    }
    
    // Configure RX signals for OS EV
    // RX Signal 1: Battery Voltage from 0x0595
    g_stBSA_RxSignals[0].BSA_canId[0] = 0x95;
    g_stBSA_RxSignals[0].BSA_canId[1] = 0x05;
    g_stBSA_RxSignals[0].BSA_start_bit = OS_BATPACP_VOLT_DATA_POSITION * 8;  // Byte 6 = bit 48
    g_stBSA_RxSignals[0].BSA_bit_length = 16;
    g_stBSA_RxSignals[0].BSA_scale_factor = OS_BATPACP_VOLT_FACTOR;
    g_stBSA_RxSignals[0].BSA_offset = 0.0f;
    g_stBSA_RxSignals[0].BSA_signal_type = eSignalType_BSAVoltage;
    g_ucBSA_RxSignalCount++;
    
    // Enable external TX configuration
    BSA_EnableExternalTx(true);
    
    printf("[BSA] Configured for OS EV - TX:%d signals, RX:%d signals\r\n", 
           g_stBSA_TxManager.txCount, g_ucBSA_RxSignalCount);
}
#endif