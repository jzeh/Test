#ifndef TASK_PLC_H
#define TASK_PLC_H
/**
  ******************************************************************************
 * @file    task-plc.h
 * @brief   PLC Task (FDCAN2) - CAN 2.0B Extended ID, 500kbps
 *
 *          CAN ID Format (29-bit Extended):
 *          ┌────────────────┬─────┬──────┬──────┬───────────┐
 *          │ Prefix (15-13) │MsgT │Prefix│GrpID │  Source+   │
 *          │  0x15EC        │(2b) │(1b)  │(2b)  │ MsgType   │
 *          │ Bit28..16      │15-14│ 13   │12-11 │  10..0    │
 *          └────────────────┴─────┴──────┴──────┴───────────┘
 *          HEX: 1  5  E  C  x  x  x  x
  ******************************************************************************
 */

/* Includes  -----------------------------------------------------------*/
#include "sys-common.h"

/* Defines  ------------------------------------------------------------*/

/* CAN ID prefix: upper 13 bits = 0x15EC shifted */
/* 29-bit ID: 0x15EC_xxxx → bits[28:16] = 0x15EC >> 3 ...
 * Actually: 0x15EC in hex nibbles at bit[28:16]:
 *   bit28..24 = 0x15 >> 1 = 0b1_0101 → bit28=1, bit27-24=0101
 *   bit23..20 = 0xE = 1110
 *   bit19..16 = 0xC = 1100
 * So prefix value in bits[28:16] = 0x15EC0000 >> 16 shifted into 29-bit ID
 */
#define PLC_CAN_ID_PREFIX       0x15EC0000U   /* Fixed prefix (bits 28..16) */
#define PLC_CAN_ID_PREFIX_MASK  0x1FFF0000U   /* Mask for bits 28..16 */

/* Source flag: bit 7 of lower portion */
#define PLC_CAN_SRC_EVSE        0x00U         /* Source = EVSE Controller */
#define PLC_CAN_SRC_PLC         0x80U         /* Source = GAEVSE01RH (PLC) */

/* Group ID: bits 9..8 */
#define PLC_CAN_GROUP_ID(g)     (((uint32_t)(g) & 0x03U) << 8)

/* Message Type extraction */
#define PLC_CAN_MSG_TYPE(id)    ((uint8_t)((id) & 0x7FU))
#define PLC_CAN_GROUP(id)       ((uint8_t)(((id) >> 8) & 0x03U))
#define PLC_CAN_SOURCE(id)      ((uint8_t)(((id) >> 7) & 0x01U))

/* Build a full 29-bit CAN ID */
#define PLC_CAN_MAKE_ID(msgType, groupId, source) \
    (PLC_CAN_ID_PREFIX | ((uint32_t)(source) << 7) | PLC_CAN_GROUP_ID(groupId) | ((uint32_t)(msgType) & 0x7FU))

/* CAN data length (CAN 2.0B = fixed 8 bytes) */
#define PLC_CAN_DATA_LEN        8

/* RX ring buffer size */
#define PLC_RX_RING_SIZE        16
#define PLC_RX_QUEUE_LEN        32

/* Data Config limits */
#define PLC_MAX_VALUES_PER_ID   8       /* max values per CAN ID */
#define PLC_MAX_RX_IDS          8       /* max SECC (PLC→MCU) CAN IDs */
#define PLC_MAX_TX_IDS          8       /* max GDS  (MCU→PLC) CAN IDs */
#define PLC_DATA_CONFIG_MSG_SIZE 16     /* wire format: 16 bytes per value entry */

/* BLE 0x31 device type */
#define PLC_DEVICE_TYPE_TX      0x01    /* GDS:  MCU→PLC (TX) */
#define PLC_DEVICE_TYPE_RX      0x02    /* SECC: PLC→MCU (RX) */

/* BLE 0x31 (SECC RX) / 0x32 (GDS TX) payload sizes */
#define PLC_CFG_SECC_HDR_SIZE   11  /* protocolId(2)+datarate(1)+msgidx(1)+responseVal(4)+interval_ms(2)+msgCount(1) */
#define PLC_CFG_GDS_HDR_SIZE    29  /* protocolId(2)+datarate(1)+msgidx(1)+requestVal(24 ASCII)+msgCount(1) */
#define PLC_CFG_REQVAL_RX       4   /* SECC response value: CAN ID (4B, full 29-bit) */
#define PLC_CFG_REQVAL_TX       12  /* GDS request value after ASCII→Hex: canId(4B)+dataTemplate(8B) */

/* Typedef  ------------------------------------------------------------*/

/* PLC CAN RX packet */
typedef struct {
    uint32_t  canId;                        /* 29-bit Extended CAN ID */
    uint8_t   data[PLC_CAN_DATA_LEN];      /* 8 bytes data (Intel byte order) */
    uint8_t   len;                          /* actual data length */
    uint16_t  timestamp;                    /* RX timestamp */
} PLC_CanPacket_t;

/* [RES] PLC -> MCU */
typedef enum {
    PLC_RES_NONE            = 0,
    PLC_RES_CP              = 1,
    PLC_RES_TARGET_VOLTAGE  = 2,
    PLC_RES_TARGET_CURRENT  = 3,
    PLC_RES_SECC_STATUS     = 4,
    PLC_RES_SOC                 = 5,
    PLC_RES_CHARGING_COMPLETE   = 6,
    PLC_RES_SECC_STATUS_LEGACY  = 7,
    PLC_RES_EV_MAX_VOLTAGE      = 8,
    PLC_RES_EV_MAX_CURRENT      = 9,
    PLC_RES_EV_ENERGY_CAPACITY  = 10,
    PLC_RES_EV_ENERGY_REQUEST   = 11,
} ePLC_ResponseValueType;

/* [REQ] MCU -> PLC */
typedef enum {
    PLC_CMD_NONE                     = 0,
    PLC_REQ_DCextd                   = 1,
    PLC_REQ_Heartbeat                = 2,
    PLC_REQ_ChargeControl            = 3,
    PLC_REQ_EVSEIsolationStatus      = 4,
    PLC_REQ_EVSEProcessing           = 5,
    PLC_REQ_EVSEMaxCurrent           = 6,
    PLC_REQ_EVSEMaxPower             = 7,
    PLC_REQ_EVSEMaxVolt              = 8,
    PLC_REQ_EVSEPresentVoltage       = 9,
    PLC_REQ_EVSEPresentCurrent       = 10,
} ePLC_RequestValueType;

/* [MSGDisp] ifmessagedisplay */

typedef enum {
    PLC_MSGDisp_ChargingControl_NONE                     = 0x00,
    PLC_MSGDisp_ChargingControl_InitializePPMT           = 0x01,
    PLC_MSGDisp_ChargingControl_StartCharging            = 0x02,
    PLC_MSGDisp_ChargingControl_NormalStop               = 0x03,
    PLC_MSGDisp_ChargingControl_EmergencyStop            = 0x04,
} ePLC_MSGDisp_ChargingControl;

typedef enum {
    PLC_MSGDisp_EVSEIsolationStatus_Invalid = 0x00,
    PLC_MSGDisp_EVSEIsolationStatus_Valid   = 0x01,
    PLC_MSGDisp_EVSEIsolationStatus_Warning = 0x02,
    PLC_MSGDisp_EVSEIsolationStatus_Fault   = 0x03,
} ePLC_MSGDisp_EVSEIsolationStatus;

typedef enum {
    PLC_MSGDisp_EVSEProcessing_None        = 0x00,
    PLC_MSGDisp_EVSEProcessing_AuthEIM     = 0x01,
    PLC_MSGDisp_EVSEProcessing_CPD         = 0x02,
    PLC_MSGDisp_EVSEProcessing_CableCheck  = 0x07,
} ePLC_MSGDisp_EVSEProcessing;

typedef enum {
    PLC_MSGDisp_SECCStatus_IDLE                     = 0x00,
    PLC_MSGDisp_SECCStatus_Initialized              = 0x01,
    PLC_MSGDisp_SECCStatus_WaitingPlugIn            = 0x02,
    PLC_MSGDisp_SECCStatus_WaitingSLAC              = 0x0A,
    PLC_MSGDisp_SECCStatus_ProcessingSLAC           = 0x0B,
    PLC_MSGDisp_SECCStatus_SDP                       = 0x14,
    PLC_MSGDisp_SECCStatus_EstablishingTCP_TLS       = 0x15,
    PLC_MSGDisp_SECCStatus_SAP                       = 0x1E,
    PLC_MSGDisp_SECCStatus_SessionSetup             = 0x28,
    PLC_MSGDisp_SECCStatus_SessionStop_Terminate    = 0x29,
    PLC_MSGDisp_SECCStatus_SessionStop_Pause        = 0x2A,
    PLC_MSGDisp_SECCStatus_ServiceDiscovery         = 0x32,
    PLC_MSGDisp_SECCStatus_ServiceDetails           = 0x33,
    PLC_MSGDisp_SECCStatus_PaymentServiceSelection  = 0x3C,
    PLC_MSGDisp_SECCStatus_CertificateInstallation  = 0x46,
    PLC_MSGDisp_SECCStatus_CertificateUpdate        = 0x47,
    PLC_MSGDisp_SECCStatus_PaymentDetails           = 0x50,
    PLC_MSGDisp_SECCStatus_Authorization_EIM        = 0x51,
    PLC_MSGDisp_SECCStatus_Authorization_PnC        = 0x52,
    PLC_MSGDisp_SECCStatus_ChargeParameterDiscovery = 0x5A,
    PLC_MSGDisp_SECCStatus_CableCheck               = 0x64,
    PLC_MSGDisp_SECCStatus_PreCharge                = 0x65,
    PLC_MSGDisp_SECCStatus_WeldingDetection         = 0x66,
    PLC_MSGDisp_SECCStatus_PowerDelivery_Start      = 0x6E,
    PLC_MSGDisp_SECCStatus_PowerDelivery_EVInit_Stop   = 0x6F,
    PLC_MSGDisp_SECCStatus_PowerDelivery_EVSEInit_Stop = 0x70,
    PLC_MSGDisp_SECCStatus_PowerDelivery_ReNegotiate = 0x71,
    PLC_MSGDisp_SECCStatus_CurrentDemand             = 0x78,
    PLC_MSGDisp_SECCStatus_MeteringReceipt           = 0x79,
    PLC_MSGDisp_SECCStatus_TERMINATE                 = 0xFA,
    PLC_MSGDisp_SECCStatus_PAUSE                     = 0xFB,
    PLC_MSGDisp_SECCStatus_ERROR                     = 0xFC
} ePLC_MSGDisp_SECCStatus;

/**
 * @brief  Conversion Rule (동일 wire format: BSA stBSA_ConvRule)
 *         convType(1) + A(2) + B(2) + C(1) + D(1) + E(1) + F(1) = 9 bytes
 */
typedef struct {
    uint8_t     convType;       /* conversion formula index */
    uint16_t    convA;          /* parameter A (2 bytes, LE) */
    uint16_t    convB;          /* parameter B (2 bytes, LE) */
    uint8_t     convC;          /* parameter C */
    uint8_t     convD;          /* parameter D */
    uint8_t     convE;          /* parameter E */
    uint8_t     convF;          /* parameter F */
} stPLC_ConvRule;

/**
 * @brief  Single Value Entry (16 bytes wire format)
 *         valueType(1) + startPos(1) + dataSize(1) + mask(4) + convRule(9) = 16
 */
typedef struct {
    uint8_t         valueType;      /* ePLC_ResponseValueType or ePLC_RequestValueType */
    uint8_t         startPosition;  /* byte offset in CAN data field (0-7) */
    uint8_t         dataSize;       /* data size in bytes (1 or 2) */
    uint32_t        maskingValue;   /* data masking value */
    stPLC_ConvRule  convRule;       /* conversion parameters */
} stPLC_DataConfigMsg;

/**
 * @brief  RX Config (SECC: PLC → MCU) - 1개 CAN ID 단위
 *
 *  BLE 0x31 payload:
 *  [0..1]  protocolId (2B BE)
 *  [2]     dataRate (1B)
 *  [3]     messageIndex (1B)
 *  [4..5]  responseValue/canId (2B BE)
 *  [6..7]  interval_ms (2B BE, 예약)
 *  [8]     messageCount
 *  [9..]   messages × messageCount (16B each, BE)
 */
typedef struct {
    uint8_t             messageIndex;               /* BLE 0x31 messageindex (1B) */
    uint16_t            protocolId;                 /* 0x0100=CAN Classic, 0x0130=CAN FD */
    uint8_t             dataRate;                   /* datarate index (FDCAN 속도) */
    uint32_t            canId;                      /* 29-bit Extended CAN ID */
    uint16_t            interval_ms;                /* expected RX interval (ms) */
    uint8_t             messageCount;               /* number of value entries */
    stPLC_DataConfigMsg messages[PLC_MAX_VALUES_PER_ID];
    /* runtime */
    uint32_t            lastRxTime;                 /* last RX tick (timeout detection) */
} stPLC_RxConfig;

/**
 * @brief  TX Config (GDS: MCU → PLC) - 1개 CAN ID 단위
 *
 *  BLE 0x32 payload:
 *  [0..1]  protocolId (2B BE)
 *  [2]     dataRate (1B)
 *  [3]     messageIndex (1B)
 *  [4..27] requestValue (24B ASCII → 12B hex: canId(4B) + dataTemplate(8B))
 *  [28]    messageCount (1B)
 *  [29..]  messages × messageCount (16B each, BE)
 *  ※ interval_ms는 payload에 없음 → 기본 50ms 적용
 */
typedef struct {
    uint8_t             messageIndex;               /* BLE 0x32 messageindex (1B) */
    uint16_t            protocolId;                 /* 0x0100=CAN Classic, 0x0130=CAN FD */
    uint8_t             dataRate;                   /* datarate index (FDCAN 속도) */
    uint32_t            canId;                      /* 29-bit Extended CAN ID (4B) */
    uint8_t             dataTemplate[PLC_CAN_DATA_LEN]; /* CAN data 초기값 template (8B) */
    uint16_t            interval_ms;                /* TX interval (ms) */
    uint8_t             messageCount;               /* number of value entries */
    stPLC_DataConfigMsg messages[PLC_MAX_VALUES_PER_ID];
    /* runtime */
    uint32_t            lastTxTime;                 /* last TX tick */
    bool                enabled;                    /* TX enable flag */
} stPLC_TxConfig;

/**
 * @brief  PLC 전체 설정 (BLE 0x31로 수신, CAN ID 1개씩 누적)
 */
typedef struct {
    uint8_t         rxCount;                        /* SECC entry count */
    stPLC_RxConfig  rxConfigs[PLC_MAX_RX_IDS];
    uint8_t         txCount;                        /* GDS entry count */
    stPLC_TxConfig  txConfigs[PLC_MAX_TX_IDS];
    bool            configured;                     /* config 수신 완료 flag */
} stPLC_DataConfig;

/**
 * @brief  RX 파싱 결과 저장용 (실시간 값)
 */
typedef struct {
    uint8_t     cp;                     /* CP value (PLC_RES_CP) */
    uint16_t    targetVoltage;          /* Target Voltage (PLC_RES_TARGET_VOLTAGE) */
    uint16_t    targetCurrent;          /* Target Current (PLC_RES_TARGET_CURRENT) */
    uint8_t     seccStatus;             /* SECC Status (PLC_RES_SECC_STATUS) */
    uint8_t     soc;                    /* EV SoC 0~100% (PLC_RES_SOC) */
    uint8_t     chargingComplete;       /* chargingComplete bit0 (PLC_RES_CHARGING_COMPLETE) */
    uint8_t     seccStatusLegacy;       /* 0x15ECC101 Byte1 legacy status (PLC_RES_SECC_STATUS_LEGACY) */
    uint16_t    evMaxVoltage;           /* EV max voltage limit ×0.1V (PLC_RES_EV_MAX_VOLTAGE) */
    uint16_t    evMaxCurrent;           /* EV max current limit ×0.1A (PLC_RES_EV_MAX_CURRENT) */
    uint16_t    evEnergyCapacity;       /* EV battery total capacity ×10Wh (PLC_RES_EV_ENERGY_CAPACITY) */
    uint16_t    evEnergyRequest;        /* EV energy request ×10Wh (PLC_RES_EV_ENERGY_REQUEST) */
} stPLC_RxValues;

/**
 * @brief  TX 동적 값 저장용 (런타임)
 *         PLC_ProcessTx()에서 전송 직전에 dataTemplate에 반영
 */
typedef struct {
    uint8_t     heartbeat;              /* 0x00~0xFF 자동 증가 (TX마다 +1) */
    uint8_t     dcExtd;                 /* DC Extended: 0x00(default) or 0x08 */
    uint8_t     chargingControl;        /* ePLC_MSGDisp_ChargingControl */
    uint8_t     evseIsolationStatus;    /* ePLC_MSGDisp_EVSEIsolationStatus */
    uint8_t     evseProcessing;         /* ePLC_MSGDisp_EVSEProcessing */
    uint16_t    evseMaxCurrent;         /* EVSE Max Current */
    uint16_t    evseMaxPower;           /* EVSE Max Power */
    uint16_t    evseMaxVolt;            /* EVSE Max Voltage */
    uint16_t    evsePresentVoltage;     /* EVSE Present Voltage */
    uint16_t    evsePresentCurrent;     /* EVSE Present Current */
} stPLC_TxValues;

/*======================================================================*/
/*  BLE 0x32: PLC Step Data Config                                      */
/*======================================================================*/

/* Step message entry wire size: valueType(1)+startPos(1)+dataSize(1)+mask(4)+code(2)+min(2)+max(2) = 13 */
#define PLC_STEP_MSG_SIZE       7       /* valueType(1)+code(2)+min(2)+max(2) */
#define PLC_MAX_STEP_MSGS       8       /* max values per step */
#define PLC_MAX_STEPS_PER_IDX   16      /* max steps per messageIndex */
#define PLC_MAX_STEP_ENTRIES    64      /* max total step entries (all messageIndex combined) */

/**
 * @brief  Step Message Entry (BLE 0x33 per-value)
 *         valueType(1) + code(2 BE) + min(2 BE) + max(2 BE) = 7B
 */
typedef struct {
    uint8_t     valueType;      /* value name (same enum as PLC config) */
    uint16_t    code;           /* value to set (TX) or exact match (RX) */
    uint16_t    minValue;       /* trigger min (0=ignore) */
    uint16_t    maxValue;       /* trigger max (0=ignore) */
} stPLC_StepMsg;

/**
 * @brief  Single Step Config (1 step for 1 messageIndex)
 *
 *  BLE 0x33 payload (Big-Endian):
 *    [0]     stepno (1B)
 *    [1]     substep (1B) — sub-step within stepno (0-based, sequential)
 *    [2]     devicetype (0x01=TX, 0x02=RX)
 *    [3]     messageindex (1B)
 *    [4..5]  interval_ms (2B BE)
 *    [6]     messageCount
 *    [7..]   messages × messageCount (7B each)
 *              valueType(1) + code(2 BE) + min(2 BE) + max(2 BE)
 */
typedef struct {
    uint8_t         stepno;         /* step sequence number */
    uint8_t         substep;        /* sub-step within stepno (0-based) */
    uint8_t         deviceType;     /* 0x01=TX(GDS), 0x02=RX(SECC) */
    uint8_t         messageIndex;   /* matches txConfigs/rxConfigs messageIndex */
    uint16_t        interval_ms;    /* step interval (0=immediate) */
    uint8_t         messageCount;   /* number of value entries */
    stPLC_StepMsg   messages[PLC_MAX_STEP_MSGS];
} stPLC_StepConfig;

/**
 * @brief  PLC Step 전체 저장소 (BLE 0x32로 수신, step 단위 누적)
 */
typedef struct {
    uint16_t          count;                            /* total step entries */
    stPLC_StepConfig  steps[PLC_MAX_STEP_ENTRIES];
    bool              configured;                       /* step 수신 완료 flag */
} stPLC_StepData;

/*======================================================================*/
/*  Stop Sequence: stepno == PLC_STOPSTEP_NO(99) 전용 저장소            */
/*  - BLE 0x33 step 엔트리 중 stepno==99로 들어오는 항목은 normal       */
/*    step engine 배열과 분리되어 여기에 누적됨.                         */
/*  - 장비 STOP 트리거 시 PLC_ApplyStopSteps() 호출로                   */
/*    g_stPLC_TxValues에 일괄 주입 → 다음 PLC_ProcessTx 주기에 송신.     */
/*======================================================================*/
#define PLC_STOPSTEP_NO              99
#define PLC_MAX_STOPSTEP_ENTRIES     8

typedef struct {
    uint16_t          count;
    stPLC_StepConfig  steps[PLC_MAX_STOPSTEP_ENTRIES];
    bool              configured;
} stPLC_StopStepData;

/* Variables -----------------------------------------------------------*/
extern stPLC_DataConfig   g_stPLC_Config;
extern stPLC_RxValues     g_stPLC_RxValues;
extern stPLC_TxValues     g_stPLC_TxValues;
extern stPLC_StepData     g_stPLC_StepData;
extern stPLC_StopStepData g_stPLC_StopStepData;
extern volatile bool     g_plcManualRun;

/* Step sequence engine */
extern volatile bool     g_plcStepRunning;
extern volatile uint16_t g_plcCurrentStep;
extern volatile uint8_t  g_plcCurrentSubStep;
extern volatile bool     g_plcStepTxApplied;
extern volatile bool     g_plcStepWaitStop;  /* STOP 명령 대기 중 */
extern void PLC_StepSequenceStart(void);
extern void PLC_StepSequenceStop(void);
extern void PLC_StepSequenceProcess(void);

/* Functions -----------------------------------------------------------*/
extern void InitPLCTask(void);

/* PLC FDCAN2 HW control */
extern HAL_StatusTypeDef PLC_CAN_Init(void);
extern HAL_StatusTypeDef PLC_CAN_Start(void);
extern HAL_StatusTypeDef PLC_CAN_Stop(void);

/* PLC TX */
extern HAL_StatusTypeDef PLC_CAN_Tx(uint32_t canId, uint8_t *data, uint8_t len);

/* PLC RX ISR callback (called from HAL_FDCAN_RxFifo0Callback for FDCAN2) */
extern void PLC_FDCAN2_RxCallback(FDCAN_HandleTypeDef *hfdcan);

/* PLC Config (BLE 0x31 → config 파싱/적용) */
extern void PLC_ApplyConfig(uint8_t *pData, uint32_t len, uint8_t deviceType);
extern void PLC_SetTxValue(uint8_t valueType, uint32_t value);
extern void PLC_ResetConfig(void);
extern void PLC_SendReboot(void);

/* PLC Step Data Config (BLE 0x32 → step 파싱/적용) */
extern bool PLC_ApplyStepConfig(uint8_t *pData, uint32_t len);
extern void PLC_ResetStepData(void);

/* Stop sequence helpers (stepno==99 entries) */
extern void PLC_ApplyStopSteps(void);      /* TX 엔트리 code → g_stPLC_TxValues 주입 */

/* BLE GDS 0x33 Step Data Config 핸들러 (git-functionlist.c 에서 이동, 동작 동일) */
extern void PLC_Cmd_StepDataConfig(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
extern void PLC_ResetStopStepData(void);   /* stop-step 저장소 초기화 */

/* Queue handle (created in task-plc.c, extern for ISR callback) */
extern QueueHandle_t plcRxCAN_Q;

/* RX ring buffer (written in ISR, read in task) */
extern PLC_CanPacket_t g_plcRxRing[PLC_RX_RING_SIZE];
extern volatile uint32_t g_plcRxRingHead;

/* 0x15ECC102 ISR snoop (FL_GDS_GetConnectorStatus 0x14/0x20 전용) */
extern volatile uint32_t g_plc15ECC102LastTick;
extern volatile uint8_t  g_plc15ECC102LastByte1;

#endif /* TASK_PLC_H */
