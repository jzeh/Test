/**
 * ******************************************************************************
 * @file    task-can.c
 * @brief   CAN Task - FDCAN1 TX/RX/Parsing/HW Setting/Test
 *          Supports: CAN Classic Standard/Extended, CAN FD Standard/Extended
 * ******************************************************************************
 */

/* Includes  -----------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/task-can.h"
#include "../Inc/task-plc.h"
#include "../Inc/task-bsa.h"   /* g_bsaHwProtocolId/DataRate, BSA_PROTOCOL_CAN_* */
#include <string.h>

/* Defines  ------------------------------------------------------------*/
#define CAN_RX_TIMEOUT_MS       100
#define CAN_LOOPBACK_TIMEOUT_MS 500
#define CAN_BACKGROUND_RX_POLL  0

/* Variables -----------------------------------------------------------*/
can_msg_param_t can_msg_params;

/* RX ring buffer (ISR writes, task reads) */
static stFdcanPkt g_rxPktRing[CAN_RX_RING_SIZE];
static volatile uint32_t g_rxPktRingHead = 0;

/* FDCAN1 (BSA) RX alive tick — updated in RxFifo0 ISR, read by 0x14 handler */
volatile uint32_t g_bsaCanRxLastTick = 0;

/* Test data */
static uint8_t txData8[8]  = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
static uint8_t txData64[64];

/* Thread Def */
osThreadId_t canTaskHandle;
uint32_t canTaskBuffer[2048];
osStaticThreadDef_t canTaskControlBlock;
const osThreadAttr_t canTask_attributes = {
  .name = "canTask",
  .cb_mem = &canTaskControlBlock,
  .cb_size = sizeof(canTaskControlBlock),
  .stack_mem = &canTaskBuffer[0],
  .stack_size = sizeof(canTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};

/* Forward declarations ------------------------------------------------*/
void StartCANTask(void *argument);

/*======================================================================*/
/*  Area 3: DLC Conversion Utilities                                    */
/*======================================================================*/

/**
 * @brief  Convert HAL DLC constant to actual byte count
 * @param  dlc_constant: FDCAN_DLC_BYTES_xx constant (0x00~0x0F)
 * @retval actual byte count (0-64)
 */
uint32_t CAN_dlc_to_len(uint32_t dlc_constant)
{
  static const uint8_t table[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
  };
  if (dlc_constant <= 0x0F)
    return table[dlc_constant];
  return 0;
}

/**
 * @brief  Convert byte count to HAL DLC constant
 * @param  len: data length in bytes
 * @retval FDCAN_DLC_BYTES_xx constant
 */
uint32_t CAN_len_to_dlc(uint8_t len)
{
  if (len == 0)  return FDCAN_DLC_BYTES_0;
  if (len <= 1)  return FDCAN_DLC_BYTES_1;
  if (len <= 2)  return FDCAN_DLC_BYTES_2;
  if (len <= 3)  return FDCAN_DLC_BYTES_3;
  if (len <= 4)  return FDCAN_DLC_BYTES_4;
  if (len <= 5)  return FDCAN_DLC_BYTES_5;
  if (len <= 6)  return FDCAN_DLC_BYTES_6;
  if (len <= 7)  return FDCAN_DLC_BYTES_7;
  if (len <= 8)  return FDCAN_DLC_BYTES_8;
  if (len <= 12) return FDCAN_DLC_BYTES_12;
  if (len <= 16) return FDCAN_DLC_BYTES_16;
  if (len <= 20) return FDCAN_DLC_BYTES_20;
  if (len <= 24) return FDCAN_DLC_BYTES_24;
  if (len <= 32) return FDCAN_DLC_BYTES_32;
  if (len <= 48) return FDCAN_DLC_BYTES_48;
  return FDCAN_DLC_BYTES_64;
}

/*======================================================================*/
/*  Area 4: CAN HW Setting                                             */
/*======================================================================*/

/**
 * @brief  Set FDCAN1 baud rate and re-initialize
 * @note   FDCAN kernel clock = PLL2P = 80 MHz
 *         BaudRate = fdcan_ck / (Prescaler * (1 + Seg1 + Seg2))
 * @param  nominalBaud: eCanBitTime enum for nominal bit rate
 * @param  dataBaud: eCanBitTime enum for data bit rate (FD only)
 * @param  format: eCanFrameFormat (FDCAN or CLASSIC)
 * @retval HAL status
 */
/**
 * @brief  BSA dataRate LUT (BLE 0x21/0x22 nDataRate) → CAN baud enum 매핑
 *
 *  LUT (스펙):
 *    0x00 : Nominal  1Mbps,  Data 2Mbps
 *    0x01 : Nominal  500kbps, Data 2Mbps
 *    0x02 : Nominal  250kbps, Data 2Mbps
 *    0x03 : Nominal  125kbps, Data 2Mbps
 *    0x04 : Nominal  100kbps, Data 2Mbps
 *    0x05 : Nominal  500kbps, Data 1Mbps
 *    0x06 : Nominal  500kbps, Data 2Mbps
 *    0x07 : Nominal  500kbps, Data 4Mbps
 *
 *  @param  dataRateIdx  0x00 ~ 0x07
 *  @param  outNominal   결과 nominal baud enum (eCanBitTime)
 *  @param  outData      결과 data baud enum (eCanBitTime)
 */
static void CAN_MapDataRate(uint8_t dataRateIdx, uint8_t *outNominal, uint8_t *outData)
{
  switch (dataRateIdx) {
    case 0x00: *outNominal = CAN_1MBPS;   *outData = CAN_DAT_2MBPS; break;
    case 0x01: *outNominal = CAN_500KBPS; *outData = CAN_DAT_2MBPS; break;
    case 0x02: *outNominal = CAN_250KBPS; *outData = CAN_DAT_2MBPS; break;
    case 0x03: *outNominal = CAN_125KBPS; *outData = CAN_DAT_2MBPS; break;
    case 0x04: *outNominal = CAN_100KBPS; *outData = CAN_DAT_2MBPS; break;
    case 0x05: *outNominal = CAN_500KBPS; *outData = CAN_DAT_1MBPS; break;
    case 0x06: *outNominal = CAN_500KBPS; *outData = CAN_DAT_2MBPS; break;
    case 0x07: *outNominal = CAN_500KBPS; *outData = CAN_DAT_4MBPS; break;
    default:
      /* unknown → safe default 500k/2M */
      *outNominal = CAN_500KBPS; *outData = CAN_DAT_2MBPS;
      break;
  }
}

HAL_StatusTypeDef CAN_set_baud(uint8_t nominalBaud, uint8_t dataBaud, uint8_t format)
{
  uint32_t nprec, nbs1, nbs2;
  uint32_t dprec, dbs1, dbs2;

  /* Nominal bit timing (75%) */
  /* BaudRate = 80M / (nprec * (1 + nbs1 + nbs2)) */
  nbs1 = 11;
  nbs2 = 4;
  switch (nominalBaud) {
    case CAN_1MBPS:   nprec = 5;   break;  /* 80M/(5*16) = 1M   */
    case CAN_500KBPS: nprec = 10;  break;  /* 80M/(10*16)= 500k */
    case CAN_250KBPS: nprec = 20;  break;  /* 80M/(20*16)= 250k */
    case CAN_125KBPS: nprec = 40;  break;  /* 80M/(40*16)= 125k */
    case CAN_100KBPS: nprec = 50;  break;  /* 80M/(50*16)= 100k */
    default:          nprec = 10;  break;  /* default 500k       */
  }

  /* Data bit timing (80 MHz clock, FD CAN data phase) */
  /* DataBaudRate = 80M / (dprec * (1 + dbs1 + dbs2)) */
  dbs1 = 15;
  dbs2 = 4;
  switch (dataBaud) {
    case CAN_DAT_1MBPS: dprec = 4;  break;  /* 80M/(4*20) = 1M  */
    case CAN_DAT_2MBPS: dprec = 2;  break;  /* 80M/(2*20) = 2M  */
    case CAN_DAT_4MBPS: dprec = 1;  break;  /* 80M/(1*20) = 4M  */
    default:            dprec = 2;  break;  /* default 2M        */
  }

  /* Configure FDCAN1 init structure */
  hfdcan1.Init.NominalPrescaler    = nprec;
  hfdcan1.Init.NominalSyncJumpWidth = 1;
  hfdcan1.Init.NominalTimeSeg1     = nbs1;
  hfdcan1.Init.NominalTimeSeg2     = nbs2;

  hfdcan1.Init.DataPrescaler       = dprec;
  hfdcan1.Init.DataSyncJumpWidth   = 1;
  hfdcan1.Init.DataTimeSeg1        = dbs1;
  hfdcan1.Init.DataTimeSeg2        = dbs2;

  /* Frame format */
  if (format == CAN_FRAMEFORMAT_FDCAN) {
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_FD_BRS; //FDCAN_FRAME_FD_NO_BRS; 
  } else {
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  }

  // /* CRITICAL: set filter count > 0 (main.c sets 0, which prevents filter config) */
  // hfdcan1.Init.StdFiltersNbr = 1;
  // hfdcan1.Init.ExtFiltersNbr = 1;

  /* Re-initialize FDCAN1 with new settings */
  return HAL_FDCAN_Init(&hfdcan1);
}

/**
 * @brief  Configure a single RX filter on FDCAN1
 * @param  idType: FDCAN_STANDARD_ID or FDCAN_EXTENDED_ID
 * @param  filterConfig: FDCAN_FILTER_TO_RXFIFO0, etc.
 * @param  id1: filter ID (or ID value for mask filter)
 * @param  id2: filter mask (or second ID for dual filter)
 * @retval HAL status
 */
HAL_StatusTypeDef CAN_configure_filter(uint32_t idType, uint32_t filterConfig,
                                        uint32_t id1, uint32_t id2)
{
  FDCAN_FilterTypeDef sFilterConfig = {0};

  sFilterConfig.IdType       = idType;
  sFilterConfig.FilterIndex  = 0;
  sFilterConfig.FilterType   = FDCAN_FILTER_MASK;
  sFilterConfig.FilterConfig = filterConfig;
  sFilterConfig.FilterID1    = id1;
  sFilterConfig.FilterID2    = id2;

  return HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig);
}

/**
 * @brief  Configure filters to accept all CAN messages (for testing)
 * @note   Sets mask=0 for both standard and extended IDs
 * @retval HAL status
 */
HAL_StatusTypeDef CAN_configure_filter_accept_all(void)
{
  HAL_StatusTypeDef ret;

  /* Standard ID: accept all (ID=0, Mask=0 matches everything) */
  ret = CAN_configure_filter(FDCAN_STANDARD_ID, FDCAN_FILTER_TO_RXFIFO0, 0x000, 0x000);
  if (ret != HAL_OK) return ret;

  /* Extended ID: accept all */
  FDCAN_FilterTypeDef extFilter = {0};
  extFilter.IdType       = FDCAN_EXTENDED_ID;
  extFilter.FilterIndex  = 0;
  extFilter.FilterType   = FDCAN_FILTER_MASK;
  extFilter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  extFilter.FilterID1    = 0x00000000;
  extFilter.FilterID2    = 0x00000000;
  ret = HAL_FDCAN_ConfigFilter(&hfdcan1, &extFilter);

  return ret;
}

/**
 * @brief  Complete FDCAN1 HW initialization sequence
 *         Stop -> SetBaud -> Filter -> GlobalFilter -> Notification -> Start
 * @retval HAL status
 */
HAL_StatusTypeDef CAN_HW_setting(void)
{
  HAL_StatusTypeDef ret;

  /* 1. Stop FDCAN if running */
  if (hfdcan1.State != HAL_FDCAN_STATE_RESET) {
    HAL_FDCAN_Stop(&hfdcan1);
  }

  /* 2. Set baud rate + re-init.
   *    · BLE 0x21/0x22 로 받은 protocolId / dataRate (g_bsaHwProtocolId / g_bsaHwDataRate) 적용
   *    · 미수신 시 default = FD BRS 500k/2M (전역 초기값) — 종래 동작 유지 */
  {
    uint8_t nominalBaud, dataBaud;
    uint8_t frameFormat = (g_bsaHwProtocolId == BSA_PROTOCOL_CAN_FD)
                          ? CAN_FRAMEFORMAT_FDCAN
                          : CAN_FRAMEFORMAT_CLASSIC;
    CAN_MapDataRate(g_bsaHwDataRate, &nominalBaud, &dataBaud);

    printf("[CAN] HW config: protocolId=0x%04X(%s), dataRate=0x%02X "
           "-> nominal=%u, data=%u (valid=%d)\r\n",
           g_bsaHwProtocolId,
           (frameFormat == CAN_FRAMEFORMAT_FDCAN) ? "FD" : "Classic",
           g_bsaHwDataRate, nominalBaud, dataBaud,
           (int)g_bsaHwConfigValid);

    ret = CAN_set_baud(nominalBaud, dataBaud, frameFormat);
  }
  if (ret != HAL_OK) {
    printf("[CAN] set_baud failed: %d\r\n", ret);
    return ret;
  }

  /* 3. Configure filters - accept all for test mode */
  ret = CAN_configure_filter_accept_all();
  if (ret != HAL_OK) {
    printf("[CAN] filter config failed: %d\r\n", ret);
    return ret;
  }

  /* 4. Global filter: reject non-matching standard/extended, reject remote */
  ret = HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT,
                                      FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
  if (ret != HAL_OK) {
    printf("[CAN] global filter failed: %d\r\n", ret);
    return ret;
  }

  /* 5. Activate RX FIFO0 new message interrupt */
  ret = HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
  if (ret != HAL_OK) {
    printf("[CAN] notification failed: %d\r\n", ret);
    return ret;
  }

  /* 6. CAN1 종단저항 활성화 (120Ω, PE11) */
  HAL_GPIO_WritePin(CAN1_TERM_EN_GPIO_Port, CAN1_TERM_EN_Pin, GPIO_PIN_SET);

  /* 7. Start FDCAN */
  ret = HAL_FDCAN_Start(&hfdcan1);
  if (ret != HAL_OK) {
    printf("[CAN] start failed: %d\r\n", ret);
  }

  return ret;
}

/**
 * @brief  Start FDCAN1
 */
HAL_StatusTypeDef CAN_start(void)
{
  return HAL_FDCAN_Start(&hfdcan1);
}

/**
 * @brief  Stop FDCAN1
 */
HAL_StatusTypeDef CAN_stop(void)
{
  return HAL_FDCAN_Stop(&hfdcan1);
}

/*======================================================================*/
/*  Area 1: CAN TX                                                      */
/*======================================================================*/

/**
 * @brief  Build TX header based on CAN type
 * @param  header: pointer to TX header structure to fill
 * @param  id: CAN identifier
 * @param  type: can_type_t (Standard/Extended/FD Standard/FD Extended)
 * @param  len: data length in bytes
 */
void CAN_make_tx_header(FDCAN_TxHeaderTypeDef *header, uint32_t id,
                         can_type_t type, uint8_t len)
{
  memset(header, 0, sizeof(FDCAN_TxHeaderTypeDef));

  header->Identifier          = id;
  header->TxFrameType         = FDCAN_DATA_FRAME;
  header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  header->TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
  header->MessageMarker       = 0;
  header->DataLength          = CAN_len_to_dlc(len);

  switch (type) {
    case eCAN_TYPE_STANDARD:
      header->IdType        = FDCAN_STANDARD_ID;
      header->BitRateSwitch = FDCAN_BRS_OFF;
      header->FDFormat      = FDCAN_CLASSIC_CAN;
      break;

    case eCAN_TYPE_EXTENDED:
      header->IdType        = FDCAN_EXTENDED_ID;
      header->BitRateSwitch = FDCAN_BRS_OFF;
      header->FDFormat      = FDCAN_CLASSIC_CAN;
      break;

    case eCAN_TYPE_FD_STANDARD:
      header->IdType        = FDCAN_STANDARD_ID;
      header->BitRateSwitch = FDCAN_BRS_ON;
      header->FDFormat      = FDCAN_FD_CAN;
      break;

    case eCAN_TYPE_FD_EXTENDED:
      header->IdType        = FDCAN_EXTENDED_ID;
      header->BitRateSwitch = FDCAN_BRS_ON;
      header->FDFormat      = FDCAN_FD_CAN;
      break;

    default:
      header->IdType        = FDCAN_STANDARD_ID;
      header->BitRateSwitch = FDCAN_BRS_OFF;
      header->FDFormat      = FDCAN_CLASSIC_CAN;
      break;
  }
}

/**
 * @brief  Transmit a CAN message via FDCAN1
 * @param  identifier: CAN ID
 * @param  data: pointer to data bytes
 * @param  len: data length in bytes (max 8 for classic, max 64 for FD)
 * @param  type: can_type_t
 * @retval HAL status
 */
HAL_StatusTypeDef CAN_tx(uint32_t identifier, uint8_t *data, uint8_t len,
                          can_type_t type)
{
  FDCAN_TxHeaderTypeDef txHeader;

  /* Validate length based on type */
  if ((type == eCAN_TYPE_STANDARD || type == eCAN_TYPE_EXTENDED) && len > 8) {
    len = 8;
  }
  if (len > 64) {
    len = 64;
  }

  CAN_make_tx_header(&txHeader, identifier, type, len);

  /* Wait for TX FIFO free slot (max ~5ms) */
  {
    uint32_t retry = 0;
    while (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0) {
      if (++retry > 50) {
        static uint32_t fifoFullCnt = 0;
        fifoFullCnt++;
        if ((fifoFullCnt % 100U) == 1U) {
          printf("[CAN TX] FIFO full timeout (cnt=%lu)\r\n", fifoFullCnt);
        }
        return HAL_BUSY;
      }
      osDelay(1);  /* yield 1ms tick */
    }
  }

  return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader, data);
}

/*======================================================================*/
/*  Area 2: CAN RX                                                      */
/*======================================================================*/

/**
 * @brief  FDCAN RX FIFO0 callback (ISR context)
 * @note   Overrides __weak HAL callback. Only handles FDCAN1.
 *         Stores received packets in ring buffer and sends index via receiveCAN_Q.
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == RESET) return;

  /* FDCAN2 → PLC RX handler (task-plc.c) */
  if (hfdcan->Instance == FDCAN2) {
    PLC_FDCAN2_RxCallback(hfdcan);
    return;
  }

  /* FDCAN1 → CAN test RX handler */
  if (hfdcan->Instance != FDCAN1) return;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  uint32_t fillLevel = HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0);

  while (fillLevel > 0)
  {
    uint32_t idx = g_rxPktRingHead % CAN_RX_RING_SIZE;
    stFdcanPkt *pkt = &g_rxPktRing[idx];

    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0,
                                &pkt->mRxHeader, pkt->mData) == HAL_OK)
    {
      g_bsaCanRxLastTick = HAL_GetTick();
      pkt->mLen       = CAN_dlc_to_len(pkt->mRxHeader.DataLength);
      pkt->mTimeStamp = (uint16_t)pkt->mRxHeader.RxTimestamp;
      pkt->pSource    = hfdcan;

      int ringIdx = (int)idx;
      if (xQueueSendFromISR(receiveCAN_Q, &ringIdx, &xHigherPriorityTaskWoken) == pdTRUE)
      {
        g_rxPktRingHead++;
      }
    }
    fillLevel--;
  }

  /* Re-activate notification */
  HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief  Task-level CAN RX: receive from queue, parse and print
 * @note   Blocks for CAN_RX_TIMEOUT_MS waiting for a message
 */
void CAN_rx(void)
{
  int ringIdx = 0;

  if (xQueueReceive(receiveCAN_Q, &ringIdx, pdMS_TO_TICKS(CAN_RX_TIMEOUT_MS)) == pdTRUE)
  {
    stFdcanPkt *pkt = &g_rxPktRing[ringIdx % CAN_RX_RING_SIZE];
    can_parsed_msg_t parsed;

    CAN_parse_rx_message(&pkt->mRxHeader, pkt->mData, &parsed);
    printf(".");
    // CAN_print_parsed_msg(&parsed);
  }
}

/*======================================================================*/
/*  Area 3: CAN Parsing                                                 */
/*======================================================================*/

/**
 * @brief  Parse received CAN message - extract all fields
 * @param  rxHeader: pointer to RX header from HAL
 * @param  rxData: pointer to received data bytes
 * @param  parsed: pointer to parsed result structure (output)
 */
void CAN_parse_rx_message(FDCAN_RxHeaderTypeDef *rxHeader, uint8_t *rxData,
                           can_parsed_msg_t *parsed)
{
  parsed->identifier   = rxHeader->Identifier;
  parsed->id_type      = rxHeader->IdType;
  parsed->frame_format = rxHeader->FDFormat;
  parsed->brs          = rxHeader->BitRateSwitch;
  parsed->dlc_raw      = rxHeader->DataLength;
  parsed->data_length  = CAN_dlc_to_len(rxHeader->DataLength);
  parsed->timestamp    = rxHeader->RxTimestamp;
  parsed->filter_index = rxHeader->FilterIndex;

  memset(parsed->data, 0, FDCAN_PACKET_MAX_SIZE);
  if (parsed->data_length > 0 && parsed->data_length <= FDCAN_PACKET_MAX_SIZE) {
    memcpy(parsed->data, rxData, parsed->data_length);
  }
}

/**
 * @brief  Print parsed CAN message to debug UART (printf)
 */
void CAN_print_parsed_msg(can_parsed_msg_t *msg)
{
  const char *idTypeStr   = (msg->id_type == FDCAN_STANDARD_ID) ? "Standard(11bit)" : "Extended(29bit)";
  const char *formatStr   = (msg->frame_format == FDCAN_CLASSIC_CAN) ? "Classic CAN" : "CAN FD";
  const char *brsStr      = (msg->brs == FDCAN_BRS_ON) ? "ON" : "OFF";

  printf("------ CAN RX Parsed ------\r\n");
  printf("  ID Type  : %s\r\n", idTypeStr);

  if (msg->id_type == FDCAN_STANDARD_ID) {
    printf("  ID       : 0x%03lX\r\n", msg->identifier);
  } else {
    printf("  ID       : 0x%08lX\r\n", msg->identifier);
  }

  printf("  Format   : %s\r\n", formatStr);
  printf("  BRS      : %s\r\n", brsStr);
  printf("  DLC      : %lu bytes\r\n", msg->data_length);
  // printf("  Timestamp: %lu\r\n", msg->timestamp);
  // printf("  Filter   : %lu\r\n", msg->filter_index);
  printf("  Data     : ");

  for (uint32_t i = 0; i < msg->data_length && i < FDCAN_PACKET_MAX_SIZE; i++) {
    printf("%02X ", msg->data[i]);
    if ((i + 1) % 16 == 0 && (i + 1) < msg->data_length) {
      printf("\r\n             ");
    }
  }
  printf("\r\n");
  printf("---------------------------\r\n");
}

/*======================================================================*/
/*  Area 5: Test Code                                                   */
/*======================================================================*/

/**
 * @brief  Initialize FDCAN1 hardware for testing
 */
void CAN_init_test(void)
{
  /* Prepare 64-byte test data */
  for (int i = 0; i < 64; i++) {
    txData64[i] = (uint8_t)(i + 1);
  }

  HAL_StatusTypeDef ret = CAN_HW_setting();
}

/**
 * @brief  TX test - send message with current can_msg_params type
 */
void CAN_tx_test(void)
{
  HAL_StatusTypeDef ret;
  can_type_t type = (can_type_t)can_msg_params.id_type;

  printf("====== CAN TX Test ======\r\n");

  switch (type) {
    case eCAN_TYPE_STANDARD:
      ret = CAN_tx(0x321, txData8, 8, eCAN_TYPE_STANDARD);
      printf("[TX] Classic Std ID=0x321 DLC=8: %s\r\n",
             (ret == HAL_OK) ? "OK" : "FAIL");
      break;

    case eCAN_TYPE_EXTENDED:
      ret = CAN_tx(0x1ABCDE1, txData8, 8, eCAN_TYPE_EXTENDED);
      printf("[TX] Classic Ext ID=0x1ABCDE1 DLC=8: %s\r\n",
             (ret == HAL_OK) ? "OK" : "FAIL");
      break;

    case eCAN_TYPE_FD_STANDARD:
      ret = CAN_tx(0x123, txData64, 64, eCAN_TYPE_FD_STANDARD);
      printf("[TX] FD Std ID=0x123 DLC=64: %s\r\n",
             (ret == HAL_OK) ? "OK" : "FAIL");
      break;

    case eCAN_TYPE_FD_EXTENDED:
      ret = CAN_tx(0x1ABCDE2, txData64, 64, eCAN_TYPE_FD_EXTENDED);
      printf("[TX] FD Ext ID=0x1ABCDE2 DLC=64: %s\r\n",
             (ret == HAL_OK) ? "OK" : "FAIL");
      break;

    default:
      printf("[TX] Unknown type: %lu\r\n", can_msg_params.id_type);
      break;
  }

  printf("=========================\r\n");
}

/**
 * @brief  TX test for all 4 CAN types sequentially
 */
void CAN_tx_test_all_types(void)
{
  HAL_StatusTypeDef ret;

  printf("====== CAN TX All Types Test ======\r\n");

  /* 1) Classic Standard */
  ret = CAN_tx(0x321, txData8, 8, eCAN_TYPE_STANDARD);
  printf("[1/4] Classic Std  ID=0x321       DLC=8  : %s\r\n",
         (ret == HAL_OK) ? "OK" : "FAIL");
  osDelay(50);

  /* 2) Classic Extended */
  ret = CAN_tx(0x1ABCDE1, txData8, 8, eCAN_TYPE_EXTENDED);
  printf("[2/4] Classic Ext  ID=0x1ABCDE1   DLC=8  : %s\r\n",
         (ret == HAL_OK) ? "OK" : "FAIL");
  osDelay(50);

  /* 3) FD Standard */
  ret = CAN_tx(0x123, txData64, 64, eCAN_TYPE_FD_STANDARD);
  printf("[3/4] FD Std       ID=0x123       DLC=64 : %s\r\n",
         (ret == HAL_OK) ? "OK" : "FAIL");
  osDelay(50);

  /* 4) FD Extended */
  ret = CAN_tx(0x1ABCDE2, txData64, 64, eCAN_TYPE_FD_EXTENDED);
  printf("[4/4] FD Ext       ID=0x1ABCDE2   DLC=64 : %s\r\n",
         (ret == HAL_OK) ? "OK" : "FAIL");

  printf("===================================\r\n");
}

/**
 * @brief  RX test - wait for a message and print parsed result
 */
void CAN_rx_test(void)
{
  printf("[CAN RX] Waiting for messages (%dms timeout)...\r\n", CAN_RX_TIMEOUT_MS);
  CAN_rx();
}

/**
 * @brief  Loopback test - TX then wait for RX and compare
 * @note   Requires external loopback (CAN_H/CAN_L connected) or another node echoing
 */
void CAN_loopback_test(void)
{
  uint8_t loopData[8] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04 };
  int ringIdx = 0;

  printf("====== CAN Loopback Test ======\r\n");

  /* TX: send a classic standard frame */
  HAL_StatusTypeDef ret = CAN_tx(0x100, loopData, 8, eCAN_TYPE_STANDARD);
  if (ret != HAL_OK) {
    printf("[CAN] TX failed (ret=%d)\r\n", ret);
    printf("===============================\r\n");
    return;
  }
  printf("[CAN] TX sent ID=0x100\r\n");

  /* RX: wait for the message to come back */
  osDelay(10);
  if (xQueueReceive(receiveCAN_Q, &ringIdx, pdMS_TO_TICKS(CAN_LOOPBACK_TIMEOUT_MS)) == pdTRUE)
  {
    stFdcanPkt *pkt = &g_rxPktRing[ringIdx % CAN_RX_RING_SIZE];
    can_parsed_msg_t parsed;
    CAN_parse_rx_message(&pkt->mRxHeader, pkt->mData, &parsed);

    /* Verify */
    if (parsed.identifier == 0x100 &&
        memcmp(parsed.data, loopData, 8) == 0)
    {
      printf("[CAN] Loopback OK - data matched\r\n");
      CAN_print_parsed_msg(&parsed);
    }
    else
    {
      printf("[CAN] Loopback MISMATCH\r\n");
      printf("  Expected ID=0x100, Got ID=0x%03lX\r\n", parsed.identifier);
      CAN_print_parsed_msg(&parsed);
    }
  }
  else
  {
    printf("[CAN] Loopback TIMEOUT - no RX within %dms\r\n", CAN_LOOPBACK_TIMEOUT_MS);
  }

  printf("===============================\r\n");
}

/*======================================================================*/
/*  Area 6: BSA System Interface                                        */
/*======================================================================*/

/**
 * @brief  Read CAN packet from RX queue (BSA compatible interface)
 * @param  pBuf: pointer to buffer (stCanPacket structure expected)
 * @param  timeout_ms: timeout in milliseconds
 * @retval true if packet received, false if timeout
 */
bool OemReadCanBuff(uint8_t* pBuf, uint32_t timeout_ms)
{
  int ringIdx = 0;

  if (xQueueReceive(receiveCAN_Q, &ringIdx, pdMS_TO_TICKS(timeout_ms)) == pdTRUE)
  {
    stFdcanPkt *pkt = &g_rxPktRing[ringIdx % CAN_RX_RING_SIZE];
    
    /* Convert stFdcanPkt to stCanPacket format (legacy VCI3 compatibility) */
    /* Assuming pBuf points to stCanPacket structure with compatible layout */
    typedef struct {
      union {
        struct {
          uint16_t us11BitID;
          uint8_t ucCanLen;
          uint8_t arrDataFields[64];
        } stFDStdPacket;
        struct {
          uint16_t us11BitID;
          uint8_t ucCanLen;
          uint8_t arrDataFields[8];
        } stNormalPacket;
      };
    } stCanPacket;
    
    stCanPacket *rxPkt = (stCanPacket*)pBuf;
    uint16_t canId16 = 0;
    uint8_t canLen8 = 0;
    
    /* Copy CAN ID (11-bit or 29-bit, we store lower 16 bits for compatibility) */
    if (pkt->mRxHeader.IdType == FDCAN_STANDARD_ID) {
      canId16 = (uint16_t)(pkt->mRxHeader.Identifier & 0x7FF);
    } else {
      canId16 = (uint16_t)(pkt->mRxHeader.Identifier & 0xFFFF);
    }
    memcpy(&rxPkt->stFDStdPacket.us11BitID, &canId16, sizeof(canId16));
    
    /* Copy data length */
    canLen8 = (uint8_t)pkt->mLen;
    memcpy(&rxPkt->stFDStdPacket.ucCanLen, &canLen8, sizeof(canLen8));
    
    /* Copy data */
    if (pkt->mLen > 0 && pkt->mLen <= 64) {
      memcpy(rxPkt->stFDStdPacket.arrDataFields, pkt->mData, pkt->mLen);
    }
    
    return true;
  }
  
  return false;
}

/**
 * @brief  Send CAN packet (BSA compatible interface)
 * @param  canId: CAN identifier
 * @param  canType: CAN type (CAN_TYPE_STANDARD/EXTENDED/FD_STANDARD/FD_EXTENDED)
 * @param  pData: pointer to data buffer
 * @param  len: data length in bytes
 * @retval HAL status
 */
HAL_StatusTypeDef BSA_SendCanPacket(uint32_t canId, uint8_t canType, uint8_t* pData, uint8_t len)
{
  can_type_t type;
  
  /* Map BSA canType to can_type_t */
  switch (canType) {
    case 0x00: type = eCAN_TYPE_STANDARD; break;     // CAN_TYPE_STANDARD
    case 0x01: type = eCAN_TYPE_EXTENDED; break;     // CAN_TYPE_EXTENDED
    case 0x02: type = eCAN_TYPE_FD_STANDARD; break;  // CAN_TYPE_FD_STANDARD
    case 0x03: type = eCAN_TYPE_FD_EXTENDED; break;  // CAN_TYPE_FD_EXTENDED
    default:   type = eCAN_TYPE_STANDARD; break;
  }
  
  return CAN_tx(canId, pData, len, type);
}

/*======================================================================*/
/*  CAN Task                                                            */
/*======================================================================*/

/**
 * @brief  Setup CAN message parameters
 */
void CAN_setup_parameter(uint8_t id_type, uint8_t frame_type, uint8_t dlc, uint8_t brs)
{
  can_msg_params.id_type    = id_type;
  can_msg_params.frame_type = frame_type;
  can_msg_params.dlc        = dlc;
  can_msg_params.brs        = brs;
}

/**
 * @brief  Create CAN task
 */
void InitCANTask(void)
{
  canTaskHandle = osThreadNew(StartCANTask, NULL, &canTask_attributes);
}

/**
 * @brief  CAN task main loop
 *         Receives commands via sendCAN_Q and dispatches to test functions.
 *         When idle (no command), polls for RX messages.
 */
void StartCANTask(void *argument)
{
  int cmd = 0;
  printf("start %s ...\r\n", __FUNCTION__);

  /* Initial HW setup */
  CAN_init_test();

  /* Default parameter: FD Extended for general testing */
  CAN_setup_parameter(eCAN_TYPE_FD_EXTENDED, eCAN_FRAME_DATA, 64, 1);

  for (;;)
  {
    // if (sendCAN_Q == NULL) {
    //   osDelay(1);
    //   continue;
    // }

    if (xQueueReceive(sendCAN_Q, &cmd, pdMS_TO_TICKS(CAN_RX_TIMEOUT_MS)) == pdTRUE)
    {
      switch (cmd)
      {
        case CAN_CMD_INIT:
          CAN_init_test();
          break;

        case CAN_CMD_TX_TEST:
          CAN_tx_test();
          break;

        case CAN_CMD_RX_TEST:
          CAN_rx_test();
          break;

        case CAN_CMD_LOOPBACK:
          CAN_loopback_test();
          break;

        case CAN_CMD_TX_ALL:
          CAN_tx_test_all_types();
          break;

        default:
          printf("[CAN] Unknown cmd: %d\r\n", cmd);
          break;
      }
    }
    else
    {
#if CAN_BACKGROUND_RX_POLL
      /* Optional debug polling: disabled by default to avoid competing with other RX consumers */
      int ringIdx = 0;
      if (receiveCAN_Q != NULL && xQueueReceive(receiveCAN_Q, &ringIdx, 0) == pdTRUE)
      {
        stFdcanPkt *pkt = &g_rxPktRing[ringIdx % CAN_RX_RING_SIZE];
        can_parsed_msg_t parsed;
        CAN_parse_rx_message(&pkt->mRxHeader, pkt->mData, &parsed);
      }
#endif
      osDelay(1);
    }
  }
}
