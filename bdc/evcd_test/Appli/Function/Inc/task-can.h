#ifndef TASK_CAN_H
#define TASK_CAN_H
/**
  ******************************************************************************
 * @file    task-can.h
 * @brief   CAN task header file.
  ******************************************************************************
 */

/* Includes  -----------------------------------------------------------*/
#include "sys-common.h"

/* Defines  ------------------------------------------------------------*/
#define CAN_RX_RING_SIZE        16
#define CAN_STD_FILTER_MAX      28
#define CAN_EXT_FILTER_MAX      8

/* Typedef  ------------------------------------------------------------*/

/* CAN message type */
typedef enum {
    eCAN_TYPE_STANDARD = 0,
    eCAN_TYPE_EXTENDED,
    eCAN_TYPE_FD_STANDARD,
    eCAN_TYPE_FD_EXTENDED
} can_type_t;

/* CAN frame type */
typedef enum {
    eCAN_FRAME_DATA = 0,
    eCAN_FRAME_REMOTE
} can_frame_t;

/* CAN frame format */
typedef enum {
    CAN_FRAMEFORMAT_FDCAN   = 0,
    CAN_FRAMEFORMAT_CLASSIC = 1
} eCanFrameFormat;

/* CAN baud rate */
typedef enum {
    CAN_1MBPS       = 0,
    CAN_500KBPS     = 1,
    CAN_250KBPS     = 2,
    CAN_125KBPS     = 3,
    CAN_100KBPS     = 4,
    CAN_DAT_1MBPS   = 5,
    CAN_DAT_2MBPS   = 6,
    CAN_DAT_4MBPS   = 7
} eCanBitTime;

/* CAN test command codes (sent via sendCAN_Q) */
typedef enum {
    CAN_CMD_INIT      = 0,
    CAN_CMD_TX_TEST   = 1,
    CAN_CMD_RX_TEST   = 2,
    CAN_CMD_LOOPBACK  = 3,
    CAN_CMD_TX_ALL    = 4
} can_cmd_t;

/* FDCAN parameter struct */
typedef struct {
    uint32_t id_type;       // can_type_t enum
    uint32_t frame_type;    // can_frame_t enum
    uint32_t dlc;           // data length in bytes (0-8, 12, 16, 20, 24, 32, 48, 64)
    uint8_t  brs;           // 0: bit rate switch off, 1: bit rate switch on
    uint32_t identifier;    // CAN ID for TX/filter
} can_msg_param_t;

/* FDCAN packet struct */
typedef struct _stFdcanPkt {
    FDCAN_RxHeaderTypeDef   mRxHeader;
    FDCAN_TxHeaderTypeDef   mTxHeader;
    FDCAN_HandleTypeDef     *pSource;
    FDCAN_HandleTypeDef     *pTarget;
    uint16_t    mTimeStamp;
    uint32_t    mLen;
    uint8_t     mData[FDCAN_PACKET_MAX_SIZE];
} stFdcanPkt;

/* Parsed CAN RX message */
typedef struct {
    uint32_t identifier;        // CAN ID
    uint32_t id_type;           // FDCAN_STANDARD_ID or FDCAN_EXTENDED_ID
    uint32_t frame_format;      // FDCAN_CLASSIC_CAN or FDCAN_FD_CAN
    uint32_t brs;               // FDCAN_BRS_OFF or FDCAN_BRS_ON
    uint32_t dlc_raw;           // raw HAL DLC constant
    uint32_t data_length;       // actual byte count (0-64)
    uint32_t timestamp;         // RxTimestamp
    uint32_t filter_index;      // matching filter index
    uint8_t  data[FDCAN_PACKET_MAX_SIZE];
} can_parsed_msg_t;

/* Functions -----------------------------------------------------------*/

/* Task */
extern void InitCANTask(void);

/* HW Setting (Area 4) */
extern HAL_StatusTypeDef CAN_HW_setting(void);
extern HAL_StatusTypeDef CAN_set_baud(uint8_t nominalBaud, uint8_t dataBaud, uint8_t format);
extern HAL_StatusTypeDef CAN_configure_filter(uint32_t idType, uint32_t filterConfig,
                                               uint32_t id1, uint32_t id2);
extern HAL_StatusTypeDef CAN_configure_filter_accept_all(void);
extern HAL_StatusTypeDef CAN_start(void);
extern HAL_StatusTypeDef CAN_stop(void);

/* TX (Area 1) */
extern HAL_StatusTypeDef CAN_tx(uint32_t identifier, uint8_t *data, uint8_t len,
                                 can_type_t type);
extern void CAN_make_tx_header(FDCAN_TxHeaderTypeDef *header, uint32_t id,
                                can_type_t type, uint8_t len);

/* RX (Area 2) */
extern void CAN_rx(void);

/* Parsing (Area 3) */
extern uint32_t CAN_dlc_to_len(uint32_t dlc_constant);
extern uint32_t CAN_len_to_dlc(uint8_t len);
extern void CAN_parse_rx_message(FDCAN_RxHeaderTypeDef *rxHeader, uint8_t *rxData,
                                  can_parsed_msg_t *parsed);
extern void CAN_print_parsed_msg(can_parsed_msg_t *msg);

/* Test (Area 5) */
extern void CAN_init_test(void);
extern void CAN_tx_test(void);
extern void CAN_rx_test(void);
extern void CAN_loopback_test(void);
extern void CAN_tx_test_all_types(void);
extern void CAN_setup_parameter(uint8_t id_type, uint8_t frame_type, uint8_t dlc, uint8_t brs);

/* BSA System Interface (Area 6) */
extern bool OemReadCanBuff(uint8_t* pBuf, uint32_t timeout_ms);
extern HAL_StatusTypeDef BSA_SendCanPacket(uint32_t canId, uint8_t canType, uint8_t* pData, uint8_t len);

#endif /* TASK_CAN_H */
