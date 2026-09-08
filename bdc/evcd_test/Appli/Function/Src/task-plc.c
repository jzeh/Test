/**
 * ******************************************************************************
 * @file    task-plc.c
 * @brief   PLC Task (FDCAN2) - CAN 2.0B Extended ID, 500kbps
 *
 *          ┌──────────────────────────────────────┐
 *          │  PLC Task (FDCAN2)    [조건부 실행]   │
 *          │  ┌────────┐  ┌────────┐              │
 *          │  │   RX   │  │   TX   │              │
 *          │  └───┬────┘  └───▲────┘              │
 *          │      │           │                   │
 *          │      ▼           │                   │
 *          │  ┌────────────────────┐              │
 *          │  │  PLC Protocol      │              │
 *          │  └────────────────────┘              │
 *          └──────────────────────────────────────┘
 *
 *  CAN Spec:
 *    - Protocol  : CAN 2.0B Active
 *    - Format    : Extended (29-bit ID)
 *    - Baud Rate : 500 kbps
 *    - Data Len  : Fixed 8 bytes
 *    - Byte Order: Intel (Little-Endian)
 *    - ID Prefix : 0x15EC (bits 28..16)
 *
 *  Config Flow:
 *    BLE 0x31 → FL_GDS_PLC_Config() → PLC_ApplyConfig()
 *    → g_stPLC_Config (RX/TX configs 누적)
 *    → PLC_ProcessRx(): config 기반 RX 파싱 → g_stPLC_RxValues
 *    → PLC_ProcessTx(): config 기반 주기적 TX
 *
 *  트리거: g_system_mode == eMODE_VEHICLE_DISCHARGE (차상 방전 모드)
 *  웨이크: osEventFlagsWait (EVT_TASK_PLC signal 대기)
 * ******************************************************************************
 */

/* Includes  -----------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/task-plc.h"
#include "../Inc/task-can.h"
#include "../Inc/task-lcd.h"
#include "../Inc/task-hwcontrol.h"
#include "../Inc/git-functionlist.h"   /* GITPACKET_ACK/NAK (send_response 는 git-protocol.h) */
#include <string.h>

/* Defines  ------------------------------------------------------------*/
#define PLC_LOG(fmt, ...)  printf("\033[32m" fmt "\033[0m", ##__VA_ARGS__)
#define PLC_RX_TIMEOUT_MS       100
#define PLC_TX_FIFO_RETRY_MAX   50
// #define PLC_DEBUG_TX              /* TX 데이터 디버그 출력 (주석 처리로 비활성화) */

/* Variables -----------------------------------------------------------*/

/* Thread Def */
osThreadId_t plcTaskHandle;
uint32_t plcTaskBuffer[ 2048 ];
osStaticThreadDef_t plcTaskControlBlock;
const osThreadAttr_t plcTask_attributes = {
    .name = "plcTask",
    .cb_mem = &plcTaskControlBlock,
    .cb_size = sizeof(plcTaskControlBlock),
    .stack_mem = &plcTaskBuffer[0],
    .stack_size = sizeof(plcTaskBuffer),
    .priority = (osPriority_t) osPriorityNormal,
};

/* PLC RX Queue (ISR → Task) */
QueueHandle_t plcRxCAN_Q = NULL;

/* PLC RX Ring Buffer */
PLC_CanPacket_t g_plcRxRing[PLC_RX_RING_SIZE];
volatile uint32_t g_plcRxRingHead = 0;

/* canId 0x15ECC102 전용 ISR 스누프 (FL_GDS_GetConnectorStatus 0x14/0x20 용)
 *  큐/링 상태와 무관하게 ISR에서 직접 갱신 → 0x14 핸들러가 바로 읽음 */
volatile uint32_t g_plc15ECC102LastTick  = 0;     /* HAL_GetTick() 시점 */
volatile uint8_t  g_plc15ECC102LastByte1 = 0xFF;  /* data[1] 값 */

/* PLC Data Config (get from 0x31) */
stPLC_DataConfig g_stPLC_Config;

/* PLC RX (real time) */
stPLC_RxValues g_stPLC_RxValues;

/* PLC TX */
stPLC_TxValues g_stPLC_TxValues;

/* PLC Step Data (get from 0x33) */
stPLC_StepData g_stPLC_StepData;
  
/* PLC Stop-step Data (get from 0x33, stepno==PLC_STOPSTEP_NO(99) entry) */
stPLC_StopStepData g_stPLC_StopStepData;

/* PLC CLI manual run flag */
volatile bool g_plcManualRun = false;

/* Step sequence engine state */
volatile bool     g_plcStepRunning   = false;
volatile uint16_t g_plcCurrentStep   = 0;
volatile uint8_t  g_plcCurrentSubStep = 0;
volatile bool     g_plcStepTxApplied = false;
volatile bool     g_plcStepWaitStop  = false;
volatile bool     g_plcManualPV      = false;  /* true: CLI 수동 PresentVoltage/Current 사용 */
static   uint32_t s_plcSubStepTime   = 0;

/* Private Function Prototypes -------------------------------------------*/
static void     PLC_StepApplyTxCode(uint8_t valueType, uint16_t code);
static uint32_t PLC_StepGetRxValue(uint8_t valueType);
static bool     PLC_StepHasRx(uint16_t stepno);
static bool     PLC_StepHasTx(uint16_t stepno);
static bool     PLC_StepCheckRxTrigger(uint16_t stepno);
static uint16_t PLC_StepGetMaxStepno(void);
/* Substep helpers */
static bool     PLC_SubHasTx(uint16_t stepno, uint8_t substep);
static bool     PLC_SubHasRx(uint16_t stepno, uint8_t substep);
static bool     PLC_SubHasAny(uint16_t stepno, uint8_t substep);
static bool     PLC_SubCheckRxTrigger(uint16_t stepno, uint8_t substep);
// static uint8_t  PLC_StepGetMaxSubStep(uint16_t stepno);
static uint16_t PLC_SubGetInterval(uint16_t stepno, uint8_t substep);
void StartPLCTask(void *argument);
static void PLC_ProcessRx(void);
static void PLC_ProcessTx(void);
static void PLC_ApplyTxValues(void);
static void PLC_ParseRxPacket(PLC_CanPacket_t *pkt);
static uint32_t PLC_ExtractValue(uint8_t *data, uint8_t startPos, uint8_t dataSize, uint32_t mask);
static void PLC_StoreRxValue(uint8_t valueType, uint32_t rawValue, stPLC_ConvRule *conv);

/*======================================================================*/
/*  FDCAN2 HW Init / Start / Stop                                      */
/*======================================================================*/

/**
 * @brief  Initialize FDCAN2 for CAN 2.0B Extended, 500kbps
 * @note   FDCAN kernel clock = PLL2P = 80 MHz
 *         500kbps = 80M / (Prescaler * (1 + Seg1 + Seg2))
 *                 = 80M / (10 * (1 + 12 + 3)) = 80M / 160 = 500k
 *         Sample point = (1 + 12) / 16 = 81.25%
 * @retval HAL status
 */
HAL_StatusTypeDef PLC_CAN_Init(void)
{
  HAL_StatusTypeDef ret;

  /* 1. Stop FDCAN2 if running */
  if (hfdcan2.State != HAL_FDCAN_STATE_RESET &&
      hfdcan2.State != HAL_FDCAN_STATE_READY) {
    HAL_FDCAN_Stop(&hfdcan2);
  }

  /* 2. Re-configure for CAN 2.0B Classic, 500kbps */
  hfdcan2.Init.FrameFormat        = FDCAN_FRAME_CLASSIC;
  hfdcan2.Init.Mode               = FDCAN_MODE_NORMAL;
  hfdcan2.Init.AutoRetransmission = ENABLE;
  hfdcan2.Init.TransmitPause      = DISABLE;
  hfdcan2.Init.ProtocolException  = DISABLE;

  /* Nominal bit timing: 500kbps, sample point 81.25% */
  hfdcan2.Init.NominalPrescaler     = 10;
  hfdcan2.Init.NominalSyncJumpWidth = 1;
  hfdcan2.Init.NominalTimeSeg1      = 12;
  hfdcan2.Init.NominalTimeSeg2      = 3;

  /* Data bit timing: not used for Classic CAN, set same as nominal */
  hfdcan2.Init.DataPrescaler        = 10;
  hfdcan2.Init.DataSyncJumpWidth    = 1;
  hfdcan2.Init.DataTimeSeg1         = 12;
  hfdcan2.Init.DataTimeSeg2         = 3;

  /* Filter allocation */
  hfdcan2.Init.StdFiltersNbr = 0;    /* Not using Standard ID */
  hfdcan2.Init.ExtFiltersNbr = 1;    /* 1 Extended ID filter */
  hfdcan2.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

  ret = HAL_FDCAN_Init(&hfdcan2);
  if (ret != HAL_OK) {
    PLC_LOG("[PLC] FDCAN2 Init failed: %d\r\n", ret);
    return ret;
  }

  /* 3. Configure Extended ID filter: accept prefix 0x15ECxxxx */
  FDCAN_FilterTypeDef filterConfig = {0};
  filterConfig.IdType       = FDCAN_EXTENDED_ID;
  filterConfig.FilterIndex  = 0;
  filterConfig.FilterType   = FDCAN_FILTER_MASK;
  filterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  filterConfig.FilterID1    = PLC_CAN_ID_PREFIX;
  filterConfig.FilterID2    = PLC_CAN_ID_PREFIX_MASK;

  ret = HAL_FDCAN_ConfigFilter(&hfdcan2, &filterConfig);
  if (ret != HAL_OK) {
    PLC_LOG("[PLC] FDCAN2 filter config failed: %d\r\n", ret);
    return ret;
  }

  /* 4. Global filter: reject non-matching frames */
  ret = HAL_FDCAN_ConfigGlobalFilter(&hfdcan2,
                                      FDCAN_REJECT, FDCAN_REJECT,
                                      FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
  if (ret != HAL_OK) {
    PLC_LOG("[PLC] FDCAN2 global filter failed: %d\r\n", ret);
    return ret;
  }

  /* 5. Activate RX FIFO0 new message interrupt */
  ret = HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
  if (ret != HAL_OK) {
    PLC_LOG("[PLC] FDCAN2 notification failed: %d\r\n", ret);
    return ret;
  }

  PLC_LOG("[PLC] FDCAN2 Init OK\r\n");
  return HAL_OK;
}

/**
 * @brief  Start FDCAN2
 */
HAL_StatusTypeDef PLC_CAN_Start(void)
{
  /* CAN2 종단저항 active (120Ω, PE12) */
  HAL_GPIO_WritePin(CAN2_TERM_EN_GPIO_Port, CAN2_TERM_EN_Pin, GPIO_PIN_SET);

  HAL_StatusTypeDef ret = HAL_FDCAN_Start(&hfdcan2);
  if (ret != HAL_OK) {
    PLC_LOG("[PLC] FDCAN2 Start failed: %d\r\n", ret);
  } else {
    PLC_LOG("[PLC] FDCAN2 Start OK\r\n");
  }
  return ret;
}

/**
 * @brief  Stop FDCAN2
 */
HAL_StatusTypeDef PLC_CAN_Stop(void)
{
  return HAL_FDCAN_Stop(&hfdcan2);
}

/*======================================================================*/
/*  FDCAN2 TX                                                           */
/*======================================================================*/

/**
 * @brief  Transmit a CAN 2.0B Extended frame via FDCAN2
 * @param  canId: 29-bit Extended CAN ID
 * @param  data: pointer to data bytes (max 8)
 * @param  len: data length (0~8, will be clamped)
 * @retval HAL status
 */
HAL_StatusTypeDef PLC_CAN_Tx(uint32_t canId, uint8_t *data, uint8_t len)
{
  FDCAN_TxHeaderTypeDef txHeader;

  if (len > PLC_CAN_DATA_LEN) {
    len = PLC_CAN_DATA_LEN;
  }

  memset(&txHeader, 0, sizeof(txHeader));
  txHeader.Identifier          = canId;
  txHeader.IdType              = FDCAN_EXTENDED_ID;
  txHeader.TxFrameType         = FDCAN_DATA_FRAME;
  txHeader.DataLength          = CAN_len_to_dlc(len);
  txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  txHeader.BitRateSwitch       = FDCAN_BRS_OFF;
  txHeader.FDFormat            = FDCAN_CLASSIC_CAN;
  txHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
  txHeader.MessageMarker       = 0;

  /* Wait for TX FIFO free slot */
  uint32_t retry = 0;
  while (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan2) == 0) {
    if (++retry > PLC_TX_FIFO_RETRY_MAX) {
      /* 진단: bus-off / error state / 트랜시버 GPIO 상태 (read-only)
       *  - BusOff/ErrorPassive=1 → CAN bus 응답 없음 (차량 미응답 / 트랜시버 OFF / 라인 단선)
       *  - SW_EN=0 → CAN2 트랜시버 OFF 상태
       *  - LEC/DLEC: 마지막 에러 종류 (0=NoError, 1=Stuff, 2=Form, 3=ACK, 4=Bit1, 5=Bit0, 6=CRC)
       *  - TEC/REC: TX/RX 에러 카운터 (TEC≥256 → BusOff) */
      FDCAN_ProtocolStatusTypeDef psr;
      FDCAN_ErrorCountersTypeDef  ec;
      memset(&psr, 0, sizeof(psr));
      memset(&ec, 0, sizeof(ec));
      HAL_FDCAN_GetProtocolStatus(&hfdcan2, &psr);
      HAL_FDCAN_GetErrorCounters(&hfdcan2, &ec);
      GPIO_PinState sw_en = HAL_GPIO_ReadPin(CAN2_SW_EN_GPIO_Port, CAN2_SW_EN_Pin);
      uint32_t fifoFree = HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan2);
      PLC_LOG("[PLC TX] FIFO full timeout ID=0x%08lX | BusOff=%lu ErrPassive=%lu Warn=%lu | "
              "LEC=%lu DLEC=%lu TEC=%lu REC=%lu | SW_EN=%d FifoFree=%lu\r\n",
              canId,
              (unsigned long)psr.BusOff,
              (unsigned long)psr.ErrorPassive,
              (unsigned long)psr.Warning,
              (unsigned long)psr.LastErrorCode,
              (unsigned long)psr.DataLastErrorCode,
              (unsigned long)ec.TxErrorCnt,
              (unsigned long)ec.RxErrorCnt,
              (int)sw_en,
              (unsigned long)fifoFree);
      return HAL_BUSY;
    }
    osDelay(1);
  }

  return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &txHeader, data);
}

/*======================================================================*/
/*  FDCAN2 RX (ISR → Ring Buffer → Queue)                              */
/*======================================================================*/

/**
 * @brief  FDCAN2 RX FIFO0 callback handler
 * @note   Called from HAL_FDCAN_RxFifo0Callback when hfdcan->Instance == FDCAN2.
 */
void PLC_FDCAN2_RxCallback(FDCAN_HandleTypeDef *hfdcan)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  FDCAN_RxHeaderTypeDef rxHeader;
  uint8_t rxData[PLC_CAN_DATA_LEN];

  uint32_t fillLevel = HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0);

  while (fillLevel > 0) {
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
    {
      uint32_t idx = g_plcRxRingHead % PLC_RX_RING_SIZE;
      PLC_CanPacket_t *pkt = &g_plcRxRing[idx];

      pkt->canId     = rxHeader.Identifier;
      pkt->len       = (uint8_t)CAN_dlc_to_len(rxHeader.DataLength);
      pkt->timestamp = (uint16_t)rxHeader.RxTimestamp;
      if (pkt->len > PLC_CAN_DATA_LEN) pkt->len = PLC_CAN_DATA_LEN;
      memcpy(pkt->data, rxData, pkt->len);

      /* 0x15ECC102 직접 스누프 (큐/링과 독립) */
      if (rxHeader.Identifier == 0x15ECC102U && pkt->len >= 2) {
        g_plc15ECC102LastByte1 = pkt->data[1];
        g_plc15ECC102LastTick  = HAL_GetTick();
      }

      if (plcRxCAN_Q != NULL) {
        int ringIdx = (int)idx;
        if (xQueueSendFromISR(plcRxCAN_Q, &ringIdx, &xHigherPriorityTaskWoken) == pdTRUE) {
          g_plcRxRingHead++;
        }
      }
    }
    fillLevel--;
  }

  HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* DB Config */
// Call Function in 0x31, 0x32 
void PLC_ApplyConfig(uint8_t *pData, uint32_t len, uint8_t deviceType)
{
  /*
   * BLE 0x31 (SECC RX) payload — all multi-byte: Big-Endian:
   *   [0..1]   protocolId    (2B BE): 0x0100=Classic, 0x0130=FD
   *   [2]      datarate      (1B)
   *   [3]      messageindex  (1B)
   *   [4..7]   response value(4B BE): CAN ID (29-bit extended, full)
   *   [8..9]   interval_ms   (2B BE)
   *   [10]     messageCount  (1B)
   *   [11..]   messages × N  (16B each)
   *
   * BLE 0x32 (GDS TX) payload — all multi-byte: Big-Endian:
   *   [0..1]   protocolId    (2B BE): 0x0100=Classic, 0x0130=FD
   *   [2]      datarate      (1B)
   *   [3]      messageindex  (1B)
   *   [4..51]  request value (48B ASCII → 24B hex 변환)
   *            변환 후: canId(4B) + dataTemplate(8B) + reserved(12B)
   *   [52..53] interval_ms   (2B BE)
   *   [54]     messageCount  (1B)
   *   [55..]   messages × N  (16B each)
   */
  uint32_t minLen = (deviceType == PLC_DEVICE_TYPE_RX) ? PLC_CFG_SECC_HDR_SIZE
                                                        : PLC_CFG_GDS_HDR_SIZE;
  if (len < minLen) {
    PLC_LOG("[PLC Config] payload too short: %lu (min %lu)\r\n", len, minLen);
    return;
  }

  /* Common header: protocolId(2) + field2(1) + field3(1)
   * RX(SECC): field2=dataRate, field3=msgIndex (스펙 기준)
   * TX(GDS):  field2=dataRate, field3=msgIndex */
  uint16_t protocolId = ((uint16_t)pData[0] << 8) | pData[1];
  // uint8_t  dataRate   = (deviceType == PLC_DEVICE_TYPE_RX) ? pData[3] : pData[2];  /* 구버전: RX 스왑 */
  // uint8_t  msgIndex   = (deviceType == PLC_DEVICE_TYPE_RX) ? pData[2] : pData[3];  /* 구버전: RX 스왑 */
  uint8_t  dataRate   = pData[2];
  uint8_t  msgIndex   = pData[3];

  /* Helper: parse one message block (16 bytes, Big-Endian) */
  #define PARSE_MSG(pMsg, ofs)  do { \
    (pMsg)->valueType     = pData[(ofs) + 0]; \
    (pMsg)->startPosition = pData[(ofs) + 1]; \
    (pMsg)->dataSize      = pData[(ofs) + 2]; \
    (pMsg)->maskingValue  = ((uint32_t)pData[(ofs)+3]<<24) | ((uint32_t)pData[(ofs)+4]<<16) \
                          | ((uint32_t)pData[(ofs)+5]<<8)  |  (uint32_t)pData[(ofs)+6]; \
    (pMsg)->convRule.convType = pData[(ofs) + 7]; \
    (pMsg)->convRule.convA    = ((uint16_t)pData[(ofs)+8]  << 8) | pData[(ofs)+9]; \
    (pMsg)->convRule.convB    = ((uint16_t)pData[(ofs)+10] << 8) | pData[(ofs)+11]; \
    (pMsg)->convRule.convC    = pData[(ofs) + 12]; \
    (pMsg)->convRule.convD    = pData[(ofs) + 13]; \
    (pMsg)->convRule.convE    = pData[(ofs) + 14]; \
    (pMsg)->convRule.convF    = pData[(ofs) + 15]; \
  } while(0)

  if (deviceType == PLC_DEVICE_TYPE_RX) {
    /* ── SECC RX: response value(4B BE = full CAN ID) + interval_ms(2B BE) + msgCount(1) ── */
    uint32_t canId        = ((uint32_t)pData[4] << 24) | ((uint32_t)pData[5] << 16)
                          | ((uint32_t)pData[6] <<  8) |  (uint32_t)pData[7];
    uint16_t interval_ms  = ((uint16_t)pData[8] << 8) | pData[9];
    uint8_t  messageCount = pData[10];
    uint32_t msgOfs       = 11;

    if (messageCount > PLC_MAX_VALUES_PER_ID) {
      PLC_LOG("[PLC SECC Config] messageCount too large: %d\r\n", messageCount);
      return;
    }
    uint32_t expectedLen = msgOfs + (uint32_t)messageCount * PLC_DATA_CONFIG_MSG_SIZE;
    if (len < expectedLen) {
      PLC_LOG("[PLC SECC Config] payload short: %lu (need %lu)\r\n", len, expectedLen);
      return;
    }

    /* Find existing slot by canId, or allocate new */
    int cfgIdx = -1;
    for (uint8_t k = 0; k < g_stPLC_Config.rxCount; k++) {
      if (g_stPLC_Config.rxConfigs[k].canId == canId) { cfgIdx = k; break; }
    }
    if (cfgIdx < 0) {
      if (g_stPLC_Config.rxCount >= PLC_MAX_RX_IDS) {
        PLC_LOG("[PLC SECC Config] array full (max %d)\r\n", PLC_MAX_RX_IDS);
        return;
      }
      cfgIdx = g_stPLC_Config.rxCount++;
    }

    stPLC_RxConfig *pCfg = &g_stPLC_Config.rxConfigs[cfgIdx];
    pCfg->messageIndex = msgIndex;
    pCfg->protocolId   = protocolId;
    pCfg->dataRate     = dataRate;
    pCfg->canId        = canId;
    pCfg->interval_ms  = interval_ms;
    pCfg->messageCount = messageCount;
    pCfg->lastRxTime   = 0;

    for (uint8_t i = 0; i < messageCount; i++) {
      uint32_t ofs = msgOfs + (i * PLC_DATA_CONFIG_MSG_SIZE);
      PARSE_MSG(&pCfg->messages[i], ofs);
      PLC_LOG("  SECC Msg[%d]: type=%d pos=%d sz=%d mask=0x%08lX "
             "conv(%d A=0x%04X B=0x%04X C=%02X D=%02X E=%02X F=%02X)\r\n",
             i, pCfg->messages[i].valueType, pCfg->messages[i].startPosition,
             pCfg->messages[i].dataSize, pCfg->messages[i].maskingValue,
             pCfg->messages[i].convRule.convType, pCfg->messages[i].convRule.convA,
             pCfg->messages[i].convRule.convB, pCfg->messages[i].convRule.convC,
             pCfg->messages[i].convRule.convD, pCfg->messages[i].convRule.convE,
             pCfg->messages[i].convRule.convF);
    }

    PLC_LOG("[PLC SECC Config] [%d] proto=0x%04X rate=%d msgIdx=%d "
           "CAN_ID=0x%04lX msgCnt=%d total=%d\r\n",
           cfgIdx, protocolId, dataRate, msgIndex, canId,
           messageCount, g_stPLC_Config.rxCount);

  } else if (deviceType == PLC_DEVICE_TYPE_TX) {
    /* ── GDS TX: requestVal(12B ASCII = 24B raw) → 12B hex 변환 ── */
    /* ASCII 변환 후: canId(4B) + dataTemplate(8B) = 12B */
    uint8_t reqValRaw[PLC_CFG_REQVAL_TX];  /* 12B converted */
    uint8_t asciiLen = PLC_CFG_REQVAL_TX * 2;  /* 24B ASCII */
    memset(reqValRaw, 0, sizeof(reqValRaw));

    for (uint8_t i = 0; i < PLC_CFG_REQVAL_TX; i++) {
      uint8_t hi = pData[4 + i * 2];
      uint8_t lo = pData[4 + i * 2 + 1];
      uint8_t hn = (hi >= '0' && hi <= '9') ? (hi - '0') :
                   (hi >= 'A' && hi <= 'F') ? (hi - 'A' + 10) :
                   (hi >= 'a' && hi <= 'f') ? (hi - 'a' + 10) : 0;
      uint8_t ln = (lo >= '0' && lo <= '9') ? (lo - '0') :
                   (lo >= 'A' && lo <= 'F') ? (lo - 'A' + 10) :
                   (lo >= 'a' && lo <= 'f') ? (lo - 'a' + 10) : 0;
      reqValRaw[i] = (hn << 4) | ln;
    }

    uint32_t canId = ((uint32_t)reqValRaw[0] << 24) | ((uint32_t)reqValRaw[1] << 16)
                   | ((uint32_t)reqValRaw[2] <<  8) |  (uint32_t)reqValRaw[3];
    /* reqValRaw[4..11] = dataTemplate (8B) */
    uint8_t  messageCount = pData[4 + asciiLen];          /* offset 28 */
    uint32_t msgOfs       = 4 + asciiLen + 1;             /* offset 29 */
    uint16_t interval_ms  = 50;                           /* BLE GDS에 interval 없음, 기본 50ms */

    if (messageCount > PLC_MAX_VALUES_PER_ID) {
      PLC_LOG("[PLC GDS Config] messageCount too large: %d\r\n", messageCount);
      return;
    }
    uint32_t expectedLen = msgOfs + (uint32_t)messageCount * PLC_DATA_CONFIG_MSG_SIZE;
    if (len < expectedLen) {
      PLC_LOG("[PLC GDS Config] payload short: %lu (need %lu)\r\n", len, expectedLen);
      return;
    }

    /* Find existing slot by canId, or allocate new */
    int cfgIdx = -1;
    for (uint8_t k = 0; k < g_stPLC_Config.txCount; k++) {
      if (g_stPLC_Config.txConfigs[k].canId == canId) { cfgIdx = k; break; }
    }
    if (cfgIdx < 0) {
      if (g_stPLC_Config.txCount >= PLC_MAX_TX_IDS) {
        PLC_LOG("[PLC GDS Config] array full (max %d)\r\n", PLC_MAX_TX_IDS);
        return;
      }
      cfgIdx = g_stPLC_Config.txCount++;
    }

    stPLC_TxConfig *pCfg = &g_stPLC_Config.txConfigs[cfgIdx];
    pCfg->messageIndex = msgIndex;
    pCfg->protocolId   = protocolId;
    pCfg->dataRate     = dataRate;
    pCfg->canId        = canId;
    pCfg->interval_ms  = interval_ms;
    pCfg->messageCount = messageCount;
    pCfg->lastTxTime   = 0;
    pCfg->enabled      = true;
    memcpy(pCfg->dataTemplate, &reqValRaw[4], PLC_CAN_DATA_LEN);  /* 변환 후 [4..11] */

    PLC_LOG("  Template: ");
    for (int j = 0; j < PLC_CAN_DATA_LEN; j++) PLC_LOG("%02X ", pCfg->dataTemplate[j]);
    PLC_LOG("\r\n");

    for (uint8_t i = 0; i < messageCount; i++) {
      uint32_t ofs = msgOfs + (i * PLC_DATA_CONFIG_MSG_SIZE);
      PARSE_MSG(&pCfg->messages[i], ofs);
      PLC_LOG("  GDS Msg[%d]: type=%d pos=%d sz=%d mask=0x%08lX "
             "conv(%d A=0x%04X B=0x%04X C=%02X D=%02X E=%02X F=%02X)\r\n",
             i, pCfg->messages[i].valueType, pCfg->messages[i].startPosition,
             pCfg->messages[i].dataSize, pCfg->messages[i].maskingValue,
             pCfg->messages[i].convRule.convType, pCfg->messages[i].convRule.convA,
             pCfg->messages[i].convRule.convB, pCfg->messages[i].convRule.convC,
             pCfg->messages[i].convRule.convD, pCfg->messages[i].convRule.convE,
             pCfg->messages[i].convRule.convF);
    }

    PLC_LOG("[PLC GDS Config] [%d] proto=0x%04X rate=%d msgIdx=%d "
           "CAN_ID=0x%08lX interval=%dms msgCnt=%d total=%d\r\n",
           cfgIdx, protocolId, dataRate, msgIndex, canId,
           interval_ms, messageCount, g_stPLC_Config.txCount);
  }

  #undef PARSE_MSG

  g_stPLC_Config.configured = true;
}

// Call Function in 0x33 (FL_GDS_PLC_Step_Config)
void PLC_Cmd_StepDataConfig(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength)
{
    uint8_t *pData = (uint8_t*)pInterPtcl;
    uint8_t ack = GITPACKET_NAK;

    if (uiLength < 6) {  /* stepno(1)+devtype(1)+msgidx(1)+interval(2)+msgcnt(1) */
        printf("[PLC StepConfig] Invalid payload length: %lu (min 6)\r\n", uiLength);
        GITPACKET_send_response(0x33, GITPACKET_NAK);
        return;
    }

    if (!g_stPLC_Config.configured) {
        printf("[PLC StepConfig] PLC Config (0x31/0x32) not received yet\r\n");
        GITPACKET_send_response(0x33, GITPACKET_NAK);
        return;
    }

    /* BLE payload에 substep 없음 → substep=0을 삽입하여 내부 포맷으로 변환
     * BLE:     stepno(1) + devtype(1) + msgidx(1) + interval(2) + msgcnt(1) + msgs...
     * 내부:    stepno(1) + substep(1) + devtype(1) + msgidx(1) + interval(2) + msgcnt(1) + msgs...
     */
    uint8_t internalBuf[256];
    if (uiLength + 1 > sizeof(internalBuf)) {
        printf("[PLC StepConfig] payload too large: %lu\r\n", uiLength);
        GITPACKET_send_response(0x33, GITPACKET_NAK);
        return;
    }
    internalBuf[0] = pData[0];              /* stepno */
    internalBuf[1] = 0x00;                  /* substep = 0 (auto) */
    memcpy(&internalBuf[2], &pData[1], uiLength - 1);  /* 나머지 복사 */

    if (PLC_ApplyStepConfig(internalBuf, uiLength + 1)) {
        ack = GITPACKET_ACK;
    }
    GITPACKET_send_response(0x33, ack);
}

void PLC_ResetStepData(void)
{
  memset(&g_stPLC_StepData, 0, sizeof(g_stPLC_StepData));
  memset(&g_stPLC_StopStepData, 0, sizeof(g_stPLC_StopStepData));
  PLC_LOG("[PLC Step] Reset (cleared normal + stop-step entries)\r\n");
}

void PLC_ResetStopStepData(void)
{
  memset(&g_stPLC_StopStepData, 0, sizeof(g_stPLC_StopStepData));
  PLC_LOG("[PLC StopStep] Reset\r\n");
}

/**
 * @brief  stepno == PLC_STOPSTEP_NO(99)로 등록된 TX 엔트리 값을
 *         g_stPLC_TxValues에 일괄 주입.
 *
 *  이후 PLC_ProcessTx()가 다음 TX 주기에 이 값들을 dataTemplate에 반영하여
 *  해당 CAN ID로 송신. 호출처는 osDelay(~interval_ms) 등으로 TX 한 사이클 이상
 *  대기한 뒤 mode exit / 릴레이 OFF 등 정지 절차를 이어가면 됨.
 *
 *  RX 엔트리(stepno=99, devType=RX)는 무시한다 (stop은 TX 일괄 전송 용도).
 */
void PLC_ApplyStopSteps(void)
{
  if (!g_stPLC_StopStepData.configured || g_stPLC_StopStepData.count == 0) {
    PLC_LOG("[PLC StopStep] no stop-step (step=99) entries configured\r\n");
    return;
  }

  uint8_t applied = 0;
  for (uint16_t s = 0; s < g_stPLC_StopStepData.count; s++) 
  {
    stPLC_StepConfig *p = &g_stPLC_StopStepData.steps[s];
    if (p->deviceType != PLC_DEVICE_TYPE_TX) continue;
    for (uint8_t m = 0; m < p->messageCount; m++) 
    {
      PLC_StepApplyTxCode(p->messages[m].valueType, p->messages[m].code);
      applied++;
    }
  }
  PLC_LOG("[PLC StopStep] applied %d TX code(s) to TxValues\r\n", applied);
}

/**
 * @brief  BLE 0x33 payload 파싱 → g_stPLC_StepData 에 step 누적
 *
 *  Payload format (all multi-byte: Big-Endian):
 *    [0]     stepno (1B)
 *    [1]     devicetype (0x01=TX/GDS, 0x02=RX/SECC)
 *    [2]     messageindex (1B)
 *    [3..4]  interval_ms (2B BE)
 *    [5]     messageCount (1B)
 *    [6..]   messages × messageCount (7B each)
 *              valueType(1) + code(2 BE) + min(2 BE) + max(2 BE)
 */
bool PLC_ApplyStepConfig(uint8_t *pData, uint32_t len)
{
  /* stepno(1)+substep(1)+devicetype(1)+msgidx(1)+interval_ms(2)+msgcnt(1) = 7 */
  if (len < 7) {
    PLC_LOG("[PLC Step] payload too short: %lu\r\n", len);
    return false;
  }

  uint8_t  stepno     = pData[0];
  uint8_t  substep    = pData[1];
  uint8_t  deviceType = pData[2];
  uint8_t  msgIndex   = pData[3];
  uint16_t interval_ms  = ((uint16_t)pData[4] << 8) | pData[5];
  uint8_t  messageCount = pData[6];
  uint32_t msgOfs       = 7;  /* messages start at offset 7 */

  if (deviceType != PLC_DEVICE_TYPE_TX && deviceType != PLC_DEVICE_TYPE_RX) {
    PLC_LOG("[PLC Step] unknown deviceType: 0x%02X\r\n", deviceType);
    return false;
  }

  /* Validate messageIndex against existing config */
  bool idxFound = false;
  if (deviceType == PLC_DEVICE_TYPE_TX) {
    for (uint8_t k = 0; k < g_stPLC_Config.txCount; k++) {
      if (g_stPLC_Config.txConfigs[k].messageIndex == msgIndex) {
        idxFound = true;
        break;
      }
    }
  } else {
    for (uint8_t k = 0; k < g_stPLC_Config.rxCount; k++) {
      if (g_stPLC_Config.rxConfigs[k].messageIndex == msgIndex) {
        idxFound = true;
        break;
      }
    }
  }
  if (!idxFound) {
    PLC_LOG("[PLC Step] msgIdx=%d not found in %s config\r\n",
           msgIndex, (deviceType == PLC_DEVICE_TYPE_TX) ? "TX" : "RX");
    return false;
  }

  if (messageCount > PLC_MAX_STEP_MSGS) {
    PLC_LOG("[PLC Step] messageCount too large: %d (max %d)\r\n",
           messageCount, PLC_MAX_STEP_MSGS);
    return false;
  }

  uint32_t expectedLen = msgOfs + (uint32_t)messageCount * PLC_STEP_MSG_SIZE;
  if (len < expectedLen) {
    PLC_LOG("[PLC Step] payload short: %lu (need %lu)\r\n", len, expectedLen);
    return false;
  }

  /* stepno == PLC_STOPSTEP_NO(99) → 정지 시퀀스 저장소로 분리.
   * 분리 저장해야 하는 이유:
   *   - normal step engine(PLC_StepSequenceProcess)은 g_stPLC_StepData만 순회
   *   - 99가 같은 배열에 섞이면 PLC_StepGetMaxStepno 결과가 오염되어 시퀀스가 99까지 달림
   *   - 정지 시 호출(PLC_ApplyStopSteps)에서 별도로 꺼내 쓰기 위함 */
  bool              isStop     = (stepno == PLC_STOPSTEP_NO);
  stPLC_StepConfig *slots      = isStop ? g_stPLC_StopStepData.steps : g_stPLC_StepData.steps;
  uint16_t         *pCount     = isStop ? &g_stPLC_StopStepData.count : &g_stPLC_StepData.count;
  uint16_t          maxEntries = isStop ? PLC_MAX_STOPSTEP_ENTRIES : PLC_MAX_STEP_ENTRIES;
  const char       *tag        = isStop ? "StopStep" : "Step";

  /* Find existing slot (same stepno + substep + messageIndex + devType) or allocate new */
  int slotIdx = -1;
  for (uint16_t s = 0; s < *pCount; s++) {
    stPLC_StepConfig *p = &slots[s];
    if (p->stepno == stepno && p->substep == substep &&
        p->messageIndex == msgIndex && p->deviceType == deviceType) {
      slotIdx = s;
      break;
    }
  }
  if (slotIdx < 0) {
    if (*pCount >= maxEntries) {
      PLC_LOG("[PLC %s] array full (max %d)\r\n", tag, maxEntries);
      return false;
    }
    slotIdx = (int)*pCount;
    (*pCount)++;
  }

  stPLC_StepConfig *pStep = &slots[slotIdx];
  pStep->stepno       = stepno;
  pStep->substep      = substep;
  pStep->deviceType   = deviceType;
  pStep->messageIndex = msgIndex;
  pStep->interval_ms  = interval_ms;
  pStep->messageCount = messageCount;

  /* Parse messages (7 bytes each): valueType(1) + code(2 BE) + min(2 BE) + max(2 BE) */
  for (uint8_t i = 0; i < messageCount; i++) {
    uint32_t ofs = msgOfs + (i * PLC_STEP_MSG_SIZE);
    stPLC_StepMsg *pMsg = &pStep->messages[i];

    pMsg->valueType = pData[ofs + 0];
    pMsg->code      = ((uint16_t)pData[ofs + 1] << 8) | pData[ofs + 2];
    pMsg->minValue  = ((uint16_t)pData[ofs + 3] << 8) | pData[ofs + 4];
    pMsg->maxValue  = ((uint16_t)pData[ofs + 5] << 8) | pData[ofs + 6];

    PLC_LOG("  %s[%d.%d] Msg[%d]: type=%d "
           "code=0x%04X min=0x%04X max=0x%04X\r\n",
           tag, stepno, substep, i, pMsg->valueType,
           pMsg->code, pMsg->minValue, pMsg->maxValue);
  }

  if (isStop) g_stPLC_StopStepData.configured = true;
  else        g_stPLC_StepData.configured     = true;

  PLC_LOG("[PLC %s] [%d] step=%d.%d, %s, msgIdx=%d, interval=%dms, %d msgs, total=%d\r\n",
         tag, slotIdx, stepno, substep,
         (deviceType == PLC_DEVICE_TYPE_TX) ? "TX" : "RX",
         msgIndex, interval_ms, messageCount, *pCount);

  return true;
}

/**
 * @brief  TX value 설정 (외부에서 동적 값 주입)
 *         valueType에 해당하는 TX config의 dataTemplate 내 해당 위치에 값 삽입
 * @param  valueType: ePLC_RequestValueType
 * @param  value: 설정할 값 (1~2 bytes, Little-Endian)
 */
void PLC_SetTxValue(uint8_t valueType, uint32_t value)
{
  for (uint8_t t = 0; t < g_stPLC_Config.txCount; t++) {
    stPLC_TxConfig *pCfg = &g_stPLC_Config.txConfigs[t];
    for (uint8_t m = 0; m < pCfg->messageCount; m++) {
      stPLC_DataConfigMsg *pMsg = &pCfg->messages[m];
      if (pMsg->valueType == valueType) {
        /* Intel byte order: write value at startPosition */
        uint8_t pos = pMsg->startPosition;
        if (pMsg->dataSize == 1 && pos < PLC_CAN_DATA_LEN) {
          pCfg->dataTemplate[pos] = (uint8_t)(value & pMsg->maskingValue);
        } else if (pMsg->dataSize == 2 && (pos + 1) < PLC_CAN_DATA_LEN) {
          uint16_t masked = (uint16_t)(value & pMsg->maskingValue);
          pCfg->dataTemplate[pos]     = (uint8_t)(masked & 0xFF);
          pCfg->dataTemplate[pos + 1] = (uint8_t)((masked >> 8) & 0xFF);
        }
        return;
      }
    }
  }
}

/**
 * @brief  PLC Reboot 명령 전송 (FDCAN2)
 *         CAN ID: 0x15ECC0FF, Data: "REBOOT\0\0"
 */
void PLC_SendReboot(void)
{
  uint8_t data[8] = { 0x52, 0x45, 0x42, 0x4F, 0x4F, 0x54, 0x00, 0x00 };  /* "REBOOT\0\0" */
  HAL_StatusTypeDef ret = PLC_CAN_Tx(0x15ECC0FF, data, 8);
  if (ret == HAL_OK) {
    PLC_LOG("[PLC] Reboot command sent (ID=0x15ECC0FF)\r\n");
  } else {
    PLC_LOG("[PLC] Reboot TX failed: %d\r\n", ret);
  }
}

/**
 * @brief  설정 초기화
 */
void PLC_ResetConfig(void)
{
  memset(&g_stPLC_Config, 0, sizeof(g_stPLC_Config));
  memset(&g_stPLC_RxValues, 0, sizeof(g_stPLC_RxValues));
  memset(&g_stPLC_TxValues, 0, sizeof(g_stPLC_TxValues));
  PLC_LOG("[PLC] Config reset\r\n");
}

/*======================================================================*/
/*  PLC RX Parsing (config 기반)                                        */
/*======================================================================*/

/**
 * @brief  CAN data field에서 value 추출 (Intel byte order)
 * @param  data: 8-byte CAN data
 * @param  startPos: byte offset (0-7)
 * @param  dataSize: 1 or 2 bytes
 * @param  mask: masking value
 * @retval extracted raw value
 */
static uint32_t PLC_ExtractValue(uint8_t *data, uint8_t startPos, uint8_t dataSize, uint32_t mask)
{
  uint32_t raw = 0;

  if (startPos >= PLC_CAN_DATA_LEN) return 0;

  if (dataSize == 1) {
    raw = data[startPos];
  } else if (dataSize == 2 && (startPos + 1) < PLC_CAN_DATA_LEN) {
    raw = data[startPos] | ((uint32_t)data[startPos + 1] << 8);  /* Intel LE */
  }

  return raw & mask;
}

/**
 * @brief  RX 파싱된 raw value를 g_stPLC_RxValues에 저장
 * @param  valueType: ePLC_ResponseValueType
 * @param  rawValue: 마스킹 후 raw 값
 * @param  conv: conversion rule (향후 변환 수식 적용용)
 */
static void PLC_StoreRxValue(uint8_t valueType, uint32_t rawValue, stPLC_ConvRule *conv)
{
  /* TODO: conv rule에 따른 변환 적용
   *   convType 0: direct (no conversion)
   *   convType 1: value = rawValue * convA (fixed-point)
   *   etc.
   */
  (void)conv;

  switch (valueType) {
    case PLC_RES_CP:
      g_stPLC_RxValues.cp = (uint8_t)rawValue;
      break;
    case PLC_RES_TARGET_VOLTAGE:
      g_stPLC_RxValues.targetVoltage = (uint16_t)rawValue;
      // /* RX target voltage 가 갱신될 때마다 TX present voltage 도 즉시 동기화
      //  *  → 다음 PLC_ProcessTx 에서 새 값으로 송신 (step 무관, 지속 추종)
      //  *  · g_plcManualPV=true (CLI 'plc pv' 수동 설정) 시엔 덮어쓰지 않음 */
      // if (!g_plcManualPV) {
      //   g_stPLC_TxValues.evsePresentVoltage = g_stPLC_RxValues.targetVoltage;
      // }
      break;
    case PLC_RES_TARGET_CURRENT:
      g_stPLC_RxValues.targetCurrent = (uint16_t)rawValue;
      break;
    case PLC_RES_SECC_STATUS:
      g_stPLC_RxValues.seccStatus = (uint8_t)rawValue;
      break;
    case PLC_RES_SOC:
      g_stPLC_RxValues.soc = (uint8_t)rawValue;
      break;
    case PLC_RES_CHARGING_COMPLETE:
      g_stPLC_RxValues.chargingComplete = (uint8_t)rawValue;
      break;
    case PLC_RES_SECC_STATUS_LEGACY:
      g_stPLC_RxValues.seccStatusLegacy = (uint8_t)rawValue;
      break;
    case PLC_RES_EV_MAX_VOLTAGE:
      g_stPLC_RxValues.evMaxVoltage = (uint16_t)rawValue;
      break;
    case PLC_RES_EV_MAX_CURRENT:
      g_stPLC_RxValues.evMaxCurrent = (uint16_t)rawValue;
      break;
    case PLC_RES_EV_ENERGY_CAPACITY:
      g_stPLC_RxValues.evEnergyCapacity = (uint16_t)rawValue;
      break;
    case PLC_RES_EV_ENERGY_REQUEST:
      g_stPLC_RxValues.evEnergyRequest = (uint16_t)rawValue;
      break;
    default:
      break;
  }
}

/**
 * @brief  수신된 CAN 패킷을 config 기반으로 파싱
 *         canId 매칭 → messages[] 순회 → value 추출 → 저장
 */
static void PLC_ParseRxPacket(PLC_CanPacket_t *pkt)
{
  for (uint8_t r = 0; r < g_stPLC_Config.rxCount; r++) {
    stPLC_RxConfig *pCfg = &g_stPLC_Config.rxConfigs[r];

    if (pCfg->canId != pkt->canId) continue;

    pCfg->lastRxTime = osKernelGetTickCount();

    for (uint8_t m = 0; m < pCfg->messageCount; m++) {
      stPLC_DataConfigMsg *pMsg = &pCfg->messages[m];
      uint32_t rawVal = PLC_ExtractValue(pkt->data, pMsg->startPosition,
                                          pMsg->dataSize, pMsg->maskingValue);
      PLC_StoreRxValue(pMsg->valueType, rawVal, &pMsg->convRule);
    }
    return;
  }
}

/*======================================================================*/
/*  PLC Task                                                            */
/*======================================================================*/

void InitPLCTask(void)
{
  /* Init config */
  memset(&g_stPLC_Config, 0, sizeof(g_stPLC_Config));
  memset(&g_stPLC_StepData, 0, sizeof(g_stPLC_StepData));
  memset(&g_stPLC_StopStepData, 0, sizeof(g_stPLC_StopStepData));
  memset(&g_stPLC_RxValues, 0, sizeof(g_stPLC_RxValues));

  /* Create PLC RX queue */
  plcRxCAN_Q = xQueueCreate(PLC_RX_QUEUE_LEN, sizeof(int));

  plcTaskHandle = osThreadNew(StartPLCTask, NULL, &plcTask_attributes);
}

/**
 * @brief  PLC Task 메인 루프
 */
void StartPLCTask(void *argument)
{
  PLC_LOG("start %s ...\r\n", __FUNCTION__);

  /* FDCAN2 HW Init & Start */
  if (PLC_CAN_Init() != HAL_OK) {
    PLC_LOG("[PLC] FDCAN2 init failed, task suspended\r\n");
    osThreadSuspend(plcTaskHandle);
    return;
  }

  if (PLC_CAN_Start() != HAL_OK) {
    PLC_LOG("[PLC] FDCAN2 start failed, task suspended\r\n");
    osThreadSuspend(plcTaskHandle);
    return;
  }

  for(;;)
  {
    /* Manual run (CLI plc start) 또는 이벤트 플래그 대기 */
    if (!g_plcManualRun) {
      osEventFlagsWait(g_taskEventFlags, EVT_TASK_PLC, osFlagsWaitAny, osWaitForever);
    }

    static stPLC_RxValues s_rxPrev   = {0};
    static uint16_t       s_stepPrev = 0;
    static uint8_t        s_subPrev  = 0;
    static uint32_t       s_lcdSocUpdateTick = 0;

    while (g_system_mode == eMODE_VEHICLE_DISCHARGE || g_plcManualRun)
    {
      PLC_ProcessRx();
      PLC_StepSequenceProcess();
      PLC_ProcessTx();

      /* 1초 주기 LCD SoC 갱신 (값 변경 시에만) */
      uint32_t now = osKernelGetTickCount();
      if (now - s_lcdSocUpdateTick >= 1000) {
        s_lcdSocUpdateTick = now;
        extern uint8_t g_vci3_soc;
        static uint8_t s_socPrev = 0xFF;
        uint8_t soc = g_stPLC_RxValues.soc;
        //uint8_t soc = g_vci3_soc;
        if (soc != s_socPrev) {
          s_socPrev = soc;
          char soc_str[5];
          snprintf(soc_str, sizeof(soc_str), "%d", soc);
          LCD_PostTextSet(LCD_SCR_DISCHARGE, 3, soc_str);
        }
      }

      /* 값 변경 시에만 RX 파싱값 출력 */
      if (g_stPLC_Config.configured) {
        bool changed = (g_stPLC_RxValues.cp            != s_rxPrev.cp)            ||
                       (g_stPLC_RxValues.targetVoltage != s_rxPrev.targetVoltage) ||
                       (g_stPLC_RxValues.targetCurrent != s_rxPrev.targetCurrent) ||
                       (g_stPLC_RxValues.seccStatus    != s_rxPrev.seccStatus)    ||
                       (g_stPLC_RxValues.soc               != s_rxPrev.soc)               ||
                       (g_stPLC_RxValues.chargingComplete  != s_rxPrev.chargingComplete)  ||
                       (g_stPLC_RxValues.seccStatusLegacy  != s_rxPrev.seccStatusLegacy)  ||
                       (g_stPLC_RxValues.evMaxVoltage      != s_rxPrev.evMaxVoltage)      ||
                       (g_stPLC_RxValues.evMaxCurrent      != s_rxPrev.evMaxCurrent)      ||
                       (g_stPLC_RxValues.evEnergyCapacity  != s_rxPrev.evEnergyCapacity)  ||
                       (g_stPLC_RxValues.evEnergyRequest   != s_rxPrev.evEnergyRequest)   ||
                       (g_plcCurrentStep               != s_stepPrev)             ||
                       (g_plcCurrentSubStep            != s_subPrev);
        if (changed) {
          s_rxPrev   = g_stPLC_RxValues;
          s_stepPrev = g_plcCurrentStep;
          s_subPrev  = g_plcCurrentSubStep;
          for (uint8_t r = 0; r < g_stPLC_Config.rxCount; r++) {
            stPLC_RxConfig *pCfg = &g_stPLC_Config.rxConfigs[r];
            // PLC_LOG("[PLC RX] ID=0x%08lX msgIdx=%d",
            //        pCfg->canId, pCfg->messageIndex);
            for (uint8_t m = 0; m < pCfg->messageCount; m++) {
#if 0   // for debug
              stPLC_DataConfigMsg *pMsg = &pCfg->messages[m];
              uint32_t val = 0;
              switch (pMsg->valueType) {
                case PLC_RES_CP:             val = g_stPLC_RxValues.cp;            break;
                case PLC_RES_TARGET_VOLTAGE: val = g_stPLC_RxValues.targetVoltage; break;
                case PLC_RES_TARGET_CURRENT: val = g_stPLC_RxValues.targetCurrent; break;
                case PLC_RES_SECC_STATUS:    val = g_stPLC_RxValues.seccStatus;    break;
                case PLC_RES_SOC:                val = g_stPLC_RxValues.soc;               break;
                case PLC_RES_CHARGING_COMPLETE:  val = g_stPLC_RxValues.chargingComplete;  break;
                case PLC_RES_SECC_STATUS_LEGACY: val = g_stPLC_RxValues.seccStatusLegacy;  break;
                case PLC_RES_EV_MAX_VOLTAGE:     val = g_stPLC_RxValues.evMaxVoltage;      break;
                case PLC_RES_EV_MAX_CURRENT:     val = g_stPLC_RxValues.evMaxCurrent;      break;
                case PLC_RES_EV_ENERGY_CAPACITY: val = g_stPLC_RxValues.evEnergyCapacity;  break;
                case PLC_RES_EV_ENERGY_REQUEST:  val = g_stPLC_RxValues.evEnergyRequest;   break;
                default:                         val = 0;                                   break;
              }
              // PLC_LOG("  [pos=%d sz=%d type=%d]=0x%lX",
              //        pMsg->startPosition, pMsg->dataSize, pMsg->valueType, val);
#endif
            }
            // PLC_LOG("\r\n");
          }
          PLC_LOG("[PLC Step] running=%d step=%d.%d\r\n",
                 (int)g_plcStepRunning, g_plcCurrentStep, g_plcCurrentSubStep);
          {
            uint8_t st = g_stPLC_RxValues.seccStatus;
            const char *stStr =
              (st == PLC_MSGDisp_SECCStatus_IDLE)                         ? "IDLE" :
              (st == PLC_MSGDisp_SECCStatus_Initialized)                  ? "Initialized" :
              (st == PLC_MSGDisp_SECCStatus_WaitingPlugIn)                ? "WaitingPlugIn" :
              (st == PLC_MSGDisp_SECCStatus_WaitingSLAC)                  ? "WaitingSLAC" :
              (st == PLC_MSGDisp_SECCStatus_ProcessingSLAC)               ? "ProcessingSLAC" :
              (st == PLC_MSGDisp_SECCStatus_SDP)                          ? "SDP" :
              (st == PLC_MSGDisp_SECCStatus_EstablishingTCP_TLS)          ? "EstablishingTCP" :
              (st == PLC_MSGDisp_SECCStatus_SAP)                          ? "SAP" :
              (st == PLC_MSGDisp_SECCStatus_SessionSetup)                 ? "SessionSetup" :
              (st == PLC_MSGDisp_SECCStatus_SessionStop_Terminate)        ? "SessionStop_Terminate" :
              (st == PLC_MSGDisp_SECCStatus_SessionStop_Pause)            ? "SessionStop_Pause" :
              (st == PLC_MSGDisp_SECCStatus_ServiceDiscovery)             ? "ServiceDiscovery" :
              (st == PLC_MSGDisp_SECCStatus_ServiceDetails)               ? "ServiceDetails" :
              (st == PLC_MSGDisp_SECCStatus_PaymentServiceSelection)      ? "PaymentServiceSelection" :
              (st == PLC_MSGDisp_SECCStatus_CertificateInstallation)      ? "CertificateInstallation" :
              (st == PLC_MSGDisp_SECCStatus_CertificateUpdate)            ? "CertificateUpdate" :
              (st == PLC_MSGDisp_SECCStatus_PaymentDetails)               ? "PaymentDetails" :
              (st == PLC_MSGDisp_SECCStatus_Authorization_EIM)            ? "Authorization_EIM" :
              (st == PLC_MSGDisp_SECCStatus_Authorization_PnC)            ? "Authorization_PnC" :
              (st == PLC_MSGDisp_SECCStatus_ChargeParameterDiscovery)     ? "ChargeParamDiscovery" :
              (st == PLC_MSGDisp_SECCStatus_CableCheck)                   ? "CableCheck" :
              (st == PLC_MSGDisp_SECCStatus_PreCharge)                    ? "PreCharge" :
              (st == PLC_MSGDisp_SECCStatus_WeldingDetection)             ? "WeldingDetection" :
              (st == PLC_MSGDisp_SECCStatus_PowerDelivery_Start)          ? "PowerDelivery_Start" :
              (st == PLC_MSGDisp_SECCStatus_PowerDelivery_EVInit_Stop)    ? "PowerDelivery_EVStop" :
              (st == PLC_MSGDisp_SECCStatus_PowerDelivery_EVSEInit_Stop)  ? "PowerDelivery_EVSEStop" :
              (st == PLC_MSGDisp_SECCStatus_PowerDelivery_ReNegotiate)    ? "PowerDelivery_ReNeg" :
              (st == PLC_MSGDisp_SECCStatus_CurrentDemand)                ? "CurrentDemand" :
              (st == PLC_MSGDisp_SECCStatus_MeteringReceipt)              ? "MeteringReceipt" :
              (st == PLC_MSGDisp_SECCStatus_TERMINATE)                    ? "TERMINATE" :
              (st == PLC_MSGDisp_SECCStatus_PAUSE)                        ? "PAUSE" :
              (st == PLC_MSGDisp_SECCStatus_ERROR)                        ? "ERROR" : "Unknown";
            printf("\033[1;36m[PLC Status] 0x%02X(%s) SoC=%d%% targetV=%.1fV targetA=%.1fA\033[0m\r\n",
                   st, stStr, g_stPLC_RxValues.soc,
                   g_stPLC_RxValues.targetVoltage * 0.1f,
                   g_stPLC_RxValues.targetCurrent * 0.1f);
          }
        }
      }

      osDelay(1);
    }

    /* VEHICLE_DISCHARGE 종료 → 외부 릴레이 4→1 OFF (100ms 간격) */
    {
      PLC_LOG("[PLC] Mode exit → EXT_RLY OFF sequence\r\n");
      for (int rly = 4; rly >= 1; rly--) {
        IO_EXT_RLY_control(rly, false);
        PLC_LOG("[PLC] EXT_RLY%d OFF\r\n", rly);
        if (rly > 1) osDelay(100);
      }
    }
  }
}

/**
 * @brief  PLC FDCAN2 수신 데이터 처리
 *         - config 없으면: raw 데이터 printf
 *         - config 있으면: config 기반 value 파싱 → g_stPLC_RxValues 저장
 */
static void PLC_ProcessRx(void)
{
  int ringIdx = 0;

  while (xQueueReceive(plcRxCAN_Q, &ringIdx, 0) == pdTRUE)
  {
    PLC_CanPacket_t *pkt = &g_plcRxRing[ringIdx % PLC_RX_RING_SIZE];

    if (g_stPLC_Config.configured && g_stPLC_Config.rxCount > 0) {
      /* Config 기반 파싱 */
      PLC_ParseRxPacket(pkt);
    } else {
      /* Config 없을 때: raw dump */
      // PLC_LOG("[PLC RX] ID=0x%08lX Data=", pkt->canId);
      // for (int i = 0; i < pkt->len; i++) {
      //   PLC_LOG("%02X ", pkt->data[i]);
      // }
      // PLC_LOG("\r\n");
    }
  }
}

/**
 * @brief  g_stPLC_TxValues → dataTemplate 반영 (TX 전 호출)
 *         valueType별로 매핑하여 해당 위치에 값 기록
 */
static void PLC_ApplyTxValues(void)
{
  for (uint8_t t = 0; t < g_stPLC_Config.txCount; t++) {
    stPLC_TxConfig *pCfg = &g_stPLC_Config.txConfigs[t];
    for (uint8_t m = 0; m < pCfg->messageCount; m++) {
      stPLC_DataConfigMsg *pMsg = &pCfg->messages[m];
      uint32_t val = 0;

      switch (pMsg->valueType) {
        case PLC_REQ_DCextd:              val = g_stPLC_TxValues.dcExtd;              break;
        case PLC_REQ_Heartbeat:           val = g_stPLC_TxValues.heartbeat;           break;
        case PLC_REQ_ChargeControl:       val = g_stPLC_TxValues.chargingControl;     break;
        case PLC_REQ_EVSEIsolationStatus: val = g_stPLC_TxValues.evseIsolationStatus; break;
        case PLC_REQ_EVSEProcessing:      val = g_stPLC_TxValues.evseProcessing;      break;
        case PLC_REQ_EVSEMaxCurrent:      val = g_stPLC_TxValues.evseMaxCurrent;      break;
        case PLC_REQ_EVSEMaxPower:        val = g_stPLC_TxValues.evseMaxPower;        break;
        case PLC_REQ_EVSEMaxVolt:         val = g_stPLC_TxValues.evseMaxVolt;         break;
        case PLC_REQ_EVSEPresentVoltage:  val = g_stPLC_TxValues.evsePresentVoltage;  break;
        case PLC_REQ_EVSEPresentCurrent:  val = g_stPLC_TxValues.evsePresentCurrent;  break;
        default: continue;
      }

      uint8_t pos = pMsg->startPosition;
      if (pMsg->dataSize == 1 && pos < PLC_CAN_DATA_LEN) {
        pCfg->dataTemplate[pos] = (uint8_t)(val & pMsg->maskingValue);
      } else if (pMsg->dataSize == 2 && (pos + 1) < PLC_CAN_DATA_LEN) {
        uint16_t masked = (uint16_t)(val & pMsg->maskingValue);
        pCfg->dataTemplate[pos]     = (uint8_t)(masked & 0xFF);
        pCfg->dataTemplate[pos + 1] = (uint8_t)((masked >> 8) & 0xFF);
      }
    }
  }
}

/**
 * @brief  PLC FDCAN2 송신 처리
 *         config 기반: 각 TX entry의 interval에 맞춰 dataTemplate 전송
 *         Heartbeat: TX마다 0x00~0xFF 자동 증가
 */
static void PLC_ProcessTx(void)
{
  if (!g_stPLC_Config.configured || g_stPLC_Config.txCount == 0) return;

  /* g_stPLC_TxValues → dataTemplate 반영 */
  PLC_ApplyTxValues();

  uint32_t now = osKernelGetTickCount();

  for (uint8_t t = 0; t < g_stPLC_Config.txCount; t++) {
    stPLC_TxConfig *pCfg = &g_stPLC_Config.txConfigs[t];

    if (!pCfg->enabled || pCfg->interval_ms == 0) continue;

    if ((now - pCfg->lastTxTime) >= pCfg->interval_ms) {
      pCfg->lastTxTime = now;

      HAL_StatusTypeDef ret = PLC_CAN_Tx(pCfg->canId, pCfg->dataTemplate, PLC_CAN_DATA_LEN);
      if (ret != HAL_OK) {
        PLC_LOG("[PLC TX] ID=0x%08lX failed: %d\r\n", pCfg->canId, ret);
      }

      if (pCfg->canId == 0x15ECC005) {
        static uint32_t s_lastPrintTick = 0;
        if ((now - s_lastPrintTick) >= 1000) {
          s_lastPrintTick = now;
          PLC_LOG("[0x15ECC005] Sensor=%.1fV/%.1fA / CAN_TX=%u(%.1fV) %u(%.1fA) / step=%d.%d\r\n",
                 g_discharge_voltage_V,
                 g_discharge_current_A,
                 g_stPLC_TxValues.evsePresentVoltage,
                 g_stPLC_TxValues.evsePresentVoltage * 0.1f,
                 g_stPLC_TxValues.evsePresentCurrent,
                 g_stPLC_TxValues.evsePresentCurrent * 0.1f,
                 g_plcCurrentStep, g_plcCurrentSubStep);
        }
      }
#ifdef PLC_DEBUG_TX
      else {
        PLC_LOG("[PLC TX] ID=0x%08lX Data=", pCfg->canId);
        for (uint8_t d = 0; d < PLC_CAN_DATA_LEN; d++) PLC_LOG("%02X ", pCfg->dataTemplate[d]);
        PLC_LOG("\r\n");
      }
#endif

      /* Heartbeat: PLC 통신 중이면 TX마다 무조건 증가 (step 무관) */
      for (uint8_t m = 0; m < pCfg->messageCount; m++) {
        if (pCfg->messages[m].valueType == PLC_REQ_Heartbeat) {
          g_stPLC_TxValues.heartbeat++;  /* 0xFF → 0x00 자동 wrap (uint8_t) */
          break;
        }
      }
    }
  }
}

/*======================================================================*/
/*  Step Sequence Engine                                                 */
/*======================================================================*/

/**
 * @brief  TX step: valueType → g_stPLC_TxValues 필드에 code 값 적용
 */
static void PLC_StepApplyTxCode(uint8_t valueType, uint16_t code)
{
  switch (valueType) {
    case PLC_REQ_DCextd:              g_stPLC_TxValues.dcExtd              = (uint8_t)code;  break;
    case PLC_REQ_ChargeControl:       g_stPLC_TxValues.chargingControl     = (uint8_t)code;  break;
    case PLC_REQ_EVSEIsolationStatus: g_stPLC_TxValues.evseIsolationStatus = (uint8_t)code;  break;
    case PLC_REQ_EVSEProcessing:      g_stPLC_TxValues.evseProcessing      = (uint8_t)code;  break;
    case PLC_REQ_EVSEMaxCurrent:      g_stPLC_TxValues.evseMaxCurrent      = code;           break;
    case PLC_REQ_EVSEMaxPower:        g_stPLC_TxValues.evseMaxPower        = code;           break;
    case PLC_REQ_EVSEMaxVolt:         g_stPLC_TxValues.evseMaxVolt         = code;           break;
    case PLC_REQ_EVSEPresentVoltage:
      if (!g_plcManualPV)
        //g_stPLC_TxValues.evsePresentVoltage = (uint16_t)(g_discharge_voltage_V * 10.0f);  /* 센서 실측값 (x0.1V) */
        //if(g_stPLC_RxValues.targetVoltage < 0x1770 )
        if (g_stPLC_RxValues.seccStatus == PLC_MSGDisp_SECCStatus_PreCharge)
          g_stPLC_TxValues.evsePresentVoltage = g_stPLC_RxValues.targetVoltage;    /* PreCharge 구간에서만 targetVoltage 기반 갱신 */
          printf("debug - target voltage (%d) -> present voltage (%d)\r\n", g_stPLC_RxValues.targetVoltage, g_stPLC_TxValues.evsePresentVoltage);
      /* g_plcManualPV=true: plc pv 명령으로 설정된 값이 g_stPLC_TxValues.evsePresentVoltage에 이미 저장되어 있음 → 덮어쓰지 않음 */
      break;
    case PLC_REQ_EVSEPresentCurrent:
      if (!g_plcManualPV)
        //g_stPLC_TxValues.evsePresentCurrent = (uint16_t)(g_discharge_current_A * 10.0f);  /* 센서 실측값 (x0.1A) */
        //g_stPLC_TxValues.evsePresentCurrent = 0x000A;                                    /* 고정값 1.0A */
        if (g_stPLC_RxValues.seccStatus == PLC_MSGDisp_SECCStatus_PreCharge)
          g_stPLC_TxValues.evsePresentCurrent = g_stPLC_RxValues.targetCurrent;    /* PreCharge 구간에서만 targetCurrent 기반 갱신 */
      /* g_plcManualPV=true: plc pc 명령으로 설정된 값이 g_stPLC_TxValues.evsePresentCurrent에 이미 저장되어 있음 → 덮어쓰지 않음 */
      break;
    default: break;
  }
}

/**
 * @brief  RX 트리거 확인용: valueType → 현재 g_stPLC_RxValues 값 반환
 */
static uint32_t PLC_StepGetRxValue(uint8_t valueType)
{
  switch (valueType) {
    case PLC_RES_CP:             return g_stPLC_RxValues.cp;
    case PLC_RES_TARGET_VOLTAGE: return g_stPLC_RxValues.targetVoltage;
    case PLC_RES_TARGET_CURRENT: return g_stPLC_RxValues.targetCurrent;
    case PLC_RES_SECC_STATUS:    return g_stPLC_RxValues.seccStatus;
    case PLC_RES_SOC:                return g_stPLC_RxValues.soc;
    case PLC_RES_CHARGING_COMPLETE:  return g_stPLC_RxValues.chargingComplete;
    case PLC_RES_SECC_STATUS_LEGACY: return g_stPLC_RxValues.seccStatusLegacy;
    case PLC_RES_EV_MAX_VOLTAGE:     return g_stPLC_RxValues.evMaxVoltage;
    case PLC_RES_EV_MAX_CURRENT:     return g_stPLC_RxValues.evMaxCurrent;
    case PLC_RES_EV_ENERGY_CAPACITY: return g_stPLC_RxValues.evEnergyCapacity;
    case PLC_RES_EV_ENERGY_REQUEST:  return g_stPLC_RxValues.evEnergyRequest;
    default:                         return 0;
  }
}

/** @brief  step data에서 가장 큰 stepno 반환 */
static uint16_t PLC_StepGetMaxStepno(void)
{
  uint16_t maxStep = 0;
  for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
    if (g_stPLC_StepData.steps[i].stepno > maxStep)
      maxStep = g_stPLC_StepData.steps[i].stepno;
  }
  return maxStep;
}

/** @brief  해당 stepno에 RX 항목이 있는지 확인 */
static bool PLC_StepHasRx(uint16_t stepno)
{
  for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
    stPLC_StepConfig *p = &g_stPLC_StepData.steps[i];
    if (p->stepno == stepno && p->deviceType == PLC_DEVICE_TYPE_RX)
      return true;
  }
  return false;
}

/** @brief  해당 stepno에 TX 항목이 있는지 확인 */
static bool PLC_StepHasTx(uint16_t stepno)
{
  for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
    stPLC_StepConfig *p = &g_stPLC_StepData.steps[i];
    if (p->stepno == stepno && p->deviceType == PLC_DEVICE_TYPE_TX)
      return true;
  }
  return false;
}

/**
 * @brief  해당 stepno의 RX 트리거 조건을 모두 검사
 *         code != 0 → 정확히 일치, min/max != 0 → 범위 검사
 * @retval true = 모든 조건 충족
 */
static bool PLC_StepCheckRxTrigger(uint16_t stepno)
{
  bool anyRx = false;
  for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
    stPLC_StepConfig *pStep = &g_stPLC_StepData.steps[i];
    if (pStep->stepno != stepno) continue;
    if (pStep->deviceType != PLC_DEVICE_TYPE_RX) continue;
    anyRx = true;
    for (uint8_t m = 0; m < pStep->messageCount; m++) {
      stPLC_StepMsg *pMsg = &pStep->messages[m];
      uint32_t rxVal = PLC_StepGetRxValue(pMsg->valueType);
      if (pMsg->code == 0xFFFF) {
        if (rxVal == 0) return false;          /* 0xFFFF: any non-zero */
      } else if (pMsg->code != 0) {
        if (rxVal != pMsg->code) return false; /* 정확 일치 */
      }
      if (pMsg->minValue != 0 || pMsg->maxValue != 0) {
        if (rxVal < pMsg->minValue || rxVal > pMsg->maxValue) return false;
      }
    }
  }
  return anyRx;   /* RX 항목이 없으면 트리거 없음 → false */
}

/*----------------------------------------------------------------------
 *  Substep helpers
 *--------------------------------------------------------------------*/

// /** @brief  해당 stepno의 최대 substep 반환 */
// static uint8_t PLC_StepGetMaxSubStep(uint16_t stepno)
// {
//   uint8_t maxSub = 0;
//   for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
//     if (g_stPLC_StepData.steps[i].stepno == stepno &&
//         g_stPLC_StepData.steps[i].substep > maxSub)
//       maxSub = g_stPLC_StepData.steps[i].substep;
//   }
//   return maxSub;
// }

/** @brief  (step, substep)에 TX 항목 존재 여부 */
static bool PLC_SubHasTx(uint16_t stepno, uint8_t substep)
{
  for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
    stPLC_StepConfig *p = &g_stPLC_StepData.steps[i];
    if (p->stepno == stepno && p->substep == substep &&
        p->deviceType == PLC_DEVICE_TYPE_TX)
      return true;
  }
  return false;
}

/** @brief  (step, substep)에 RX 항목 존재 여부 */
static bool PLC_SubHasRx(uint16_t stepno, uint8_t substep)
{
  for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
    stPLC_StepConfig *p = &g_stPLC_StepData.steps[i];
    if (p->stepno == stepno && p->substep == substep &&
        p->deviceType == PLC_DEVICE_TYPE_RX)
      return true;
  }
  return false;
}

/** @brief  (step, substep)에 어떤 항목이든 존재 여부 */
static bool PLC_SubHasAny(uint16_t stepno, uint8_t substep)
{
  for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
    stPLC_StepConfig *p = &g_stPLC_StepData.steps[i];
    if (p->stepno == stepno && p->substep == substep)
      return true;
  }
  return false;
}

/** @brief  (step, substep) RX 트리거 검사 */
static bool PLC_SubCheckRxTrigger(uint16_t stepno, uint8_t substep)
{
  bool anyRx = false;
  for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
    stPLC_StepConfig *pStep = &g_stPLC_StepData.steps[i];
    if (pStep->stepno != stepno || pStep->substep != substep) continue;
    if (pStep->deviceType != PLC_DEVICE_TYPE_RX) continue;
    anyRx = true;
    for (uint8_t m = 0; m < pStep->messageCount; m++) {
      stPLC_StepMsg *pMsg = &pStep->messages[m];
      uint32_t rxVal = PLC_StepGetRxValue(pMsg->valueType);
      if (pMsg->code == 0xFFFF) {
        if (rxVal == 0) return false;          /* 0xFFFF: any non-zero */
      } else if (pMsg->code != 0) {
        if (rxVal != pMsg->code) return false; /* 정확 일치 */
      }
      if (pMsg->minValue != 0 || pMsg->maxValue != 0) {
        if (rxVal < pMsg->minValue || rxVal > pMsg->maxValue) return false;
      }
    }
  }
  return anyRx;
}

/** @brief  (step, substep)의 interval_ms 반환 */
static uint16_t PLC_SubGetInterval(uint16_t stepno, uint8_t substep)
{
  for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
    stPLC_StepConfig *p = &g_stPLC_StepData.steps[i];
    if (p->stepno == stepno && p->substep == substep)
      return p->interval_ms;
  }
  return 0;
}

/**
 * @brief  시퀀스 시작 (step 1, substep 0부터)
 */
void PLC_StepSequenceStart(void)
{
  g_plcCurrentStep    = 1;
  g_plcCurrentSubStep = 0;
  g_plcStepTxApplied  = false;
  g_plcStepWaitStop   = false;
  s_plcSubStepTime    = 0;
  g_plcStepRunning    = true;
  PLC_LOG("[Step] START  step=1.0\r\n");
}

/**
 * @brief  시퀀스 정지
 */
void PLC_StepSequenceStop(void)
{
  g_plcStepRunning = false;
  PLC_LOG("[Step] STOP  (last step=%d.%d)\r\n", g_plcCurrentStep, g_plcCurrentSubStep);
}

/**
 * @brief  스텝 시퀀스 메인 엔진 (PLC task 루프에서 매 주기 호출)
 *
 *  동작 방식 (substep 지원):
 *    같은 stepno 안에서 substep 0, 1, 2, ... 순으로 순차 실행
 *    TX substep: TX 값 적용(1회) → interval_ms 대기 후 다음 substep으로
 *    RX substep: 트리거 충족 시 다음 substep/step으로 전환
 *    TX+RX substep: TX 적용 후 RX 트리거 대기
 *    마지막 substep 완료 → 다음 step(substep 0)으로 전환
 *    look-ahead: 다음 step이 RX-only면 현 위치에서 트리거 대기
 */
void PLC_StepSequenceProcess(void)
{
  if (!g_plcStepRunning) return;
  if (!g_stPLC_StepData.configured || g_stPLC_StepData.count == 0) return;

  uint16_t cur  = g_plcCurrentStep;
  uint8_t  sub  = g_plcCurrentSubStep;
  uint16_t maxS = PLC_StepGetMaxStepno();
  if (cur == 0 || cur > maxS) { PLC_StepSequenceStop(); return; }

  bool hasTx = PLC_SubHasTx(cur, sub);
  bool hasRx = PLC_SubHasRx(cur, sub);

  /* 현재 (step, substep)에 항목 없음 → 다음 step으로 */
  if (!hasTx && !hasRx) {
    uint16_t next = cur + 1;
    if (next > maxS) { PLC_StepSequenceStop(); return; }
    g_plcCurrentStep    = next;
    g_plcCurrentSubStep = 0;
    g_plcStepTxApplied  = false;
    s_plcSubStepTime    = 0;
    return;
  }

  /* ── Phase 1: TX 값 적용 (substep 진입 시 1회) ── */
  if (hasTx && !g_plcStepTxApplied) {
    for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
      stPLC_StepConfig *p = &g_stPLC_StepData.steps[i];
      if (p->stepno != cur || p->substep != sub ||
          p->deviceType != PLC_DEVICE_TYPE_TX) continue;
      for (uint8_t m = 0; m < p->messageCount; m++) {
        PLC_StepApplyTxCode(p->messages[m].valueType, p->messages[m].code);
      }
    }
    g_plcStepTxApplied = true;
    s_plcSubStepTime   = osKernelGetTickCount();
    PLC_LOG("[Step] TX applied  step=%d.%d\r\n", cur, sub);

    /* SECCStatus==PreCharge(0x65) 시 외부 릴레이 1(PF7) ON */
    if (g_stPLC_RxValues.seccStatus == PLC_MSGDisp_SECCStatus_PreCharge) {
      IO_AD_RELAY_control(1, 1, true);

      // /* Resistance Relay Control - ON (by battery type)*/
      // // IO_EXT_RLY_control(1, true);   // IONIC 6
      // IO_EXT_RLY_control(2, true);    // EV6

      IO_Resistance_Relay_Control(g_dbConfig.resistance, true); // get data from func id 0x12 

      /* FAN Control - ON */
      IO_SS_control(1, true);
      // IO_SS_control(2, true);
      IO_SS_control(3, true);
      IO_SS_control(4, true);
    }
  }

  /* ── Phase 2: 전환 조건 판단 ── */
  if (hasRx) {
    /* RX 항목이 있는 substep: 트리거 충족 시 다음으로 */
    if (PLC_SubCheckRxTrigger(cur, sub)) {
      PLC_LOG("[Step] RX trigger OK  step=%d.%d\r\n", cur, sub);
      goto advance;
    }
  } else if (hasTx) {
    uint16_t interval = PLC_SubGetInterval(cur, sub);
    if (interval == 0xFFFF) {
      /* STOP 명령 대기: TX 값을 매 주기 갱신 (RX pass-through 반영) */
      for (uint16_t i = 0; i < g_stPLC_StepData.count; i++) {
        stPLC_StepConfig *p = &g_stPLC_StepData.steps[i];
        if (p->stepno != cur || p->substep != sub ||
            p->deviceType != PLC_DEVICE_TYPE_TX) continue;
        for (uint8_t m = 0; m < p->messageCount; m++) {
          PLC_StepApplyTxCode(p->messages[m].valueType, p->messages[m].code);
        }
      }
      g_plcStepTxApplied = true;
      /* STOP 트리거 확인 */
      if (!g_plcStepWaitStop) return;
      g_plcStepWaitStop = false;
      PLC_LOG("[Step] STOP trigger  step=%d.%d\r\n", cur, sub);
      goto advance;
    }
    /* TX-only substep: interval 대기 후 전환 */
    uint32_t now = osKernelGetTickCount();
    if (interval == 0 || (now - s_plcSubStepTime) >= interval) {
      goto advance;
    }
  }
  return;

advance:
  {
    /* 다음 substep 존재 확인 */
    uint8_t nextSub = sub + 1;
    if (PLC_SubHasAny(cur, nextSub)) {
      PLC_LOG("[Step] step=%d.%d → %d.%d\r\n", cur, sub, cur, nextSub);
      g_plcCurrentSubStep = nextSub;
      g_plcStepTxApplied  = false;
      s_plcSubStepTime    = 0;
      return;
    }

    /* substep 완료 → 다음 step으로 */
    uint16_t next = cur + 1;
    if (next > maxS) {
      PLC_LOG("[Step] COMPLETE  (last step=%d.%d)\r\n", cur, sub);
      PLC_StepSequenceStop();
      return;
    }

    /* Look-ahead: 다음 step이 RX-only → 트리거 대기 */
    if (!PLC_StepHasTx(next) && PLC_StepHasRx(next)) {
      if (PLC_StepCheckRxTrigger(next)) {
        PLC_LOG("[Step] Look-ahead RX OK  skip step=%d → %d\r\n", next, next + 1);
        g_plcCurrentStep    = next + 1;
        g_plcCurrentSubStep = 0;
        g_plcStepTxApplied  = false;
        s_plcSubStepTime    = 0;
        return;
      }
      /* 트리거 미충족:
       * - TX step에서 진입: 계속 TX 송신하며 대기 (return)
       * - RX step에서 진입: 무한 재발화 방지를 위해 next step으로 전환하여 대기 */
      if (!hasTx) {
        PLC_LOG("[Step] step=%d.%d → %d.0\r\n", cur, sub, next);
        g_plcCurrentStep    = next;
        g_plcCurrentSubStep = 0;
        g_plcStepTxApplied  = false;
        s_plcSubStepTime    = 0;
      }
      return;
    }

    /* 다음 step으로 전환 */
    PLC_LOG("[Step] step=%d.%d → %d.0\r\n", cur, sub, next);
    g_plcCurrentStep    = next;
    g_plcCurrentSubStep = 0;
    g_plcStepTxApplied  = false;
    s_plcSubStepTime    = 0;
  }
}
