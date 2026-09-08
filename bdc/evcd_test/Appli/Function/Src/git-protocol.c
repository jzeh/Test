/**
 * ******************************************************************************
 * @file    git-protocol.c
 * @brief   Git Protocol
 * ******************************************************************************
 */
/* Includes  -----------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/git-protocol.h"
#include "../Inc/git-functionlist.h"
#include "../Inc/git-comm.h"

/* Define    -----------------------------------------------------------*/
/* debug log enable define */
// #define DEBUG
//#define DEBUG_RAW_UART2   

/* External UART Handle */
extern UART_HandleTypeDef huart2;

/* Variables -----------------------------------------------------------*/
uint8_t			g_arrOutputGITPtclBuff[MAX_INTER_PROTO_DATA_LENGTH];

/* Global Frame Buffer for Message Queue */
uint8_t g_TxFrameBuffer[1024];  // Transmit Frame
uint8_t g_RxFrameBuffer[1024];  // Receive Frame 

/* Message Struct for Queue */
typedef struct {
  uint8_t* pFrameData;  // Frame Data Pointer
  uint16_t frameSize;   // Frame Size in bytes
} stTxMessage;

/* Global Message Structure (for queue stable memory) */
stTxMessage g_TxMsg;  // global message struct

osMessageQueueId_t	hParsingMsg;											// To Parsing Thread MessageQ
osMessageQueueId_t	hTransmitMsg;											// To Transmit Thread MessageQ

/* Functions -----------------------------------------------------------*/

static int hex_char_to_int(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static int hex_string_to_binary(const uint8_t *hex_str, uint16_t hex_len, uint8_t *bin_out, uint16_t bin_max)
{
  if (hex_str == NULL || bin_out == NULL) {
    return -1;
  }
  
  uint16_t bin_idx = 0;
  uint16_t i = 0;
  int high_nibble = -1;
  
  while (i < hex_len && bin_idx < bin_max) {
    char c = hex_str[i];
    
    // Skip whitespace and control characters
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',') {
      i++;
      continue;
    }
    
    int val = hex_char_to_int(c);
    if (val < 0) {
      // Invalid character, skip
      i++;
      continue;
    }
    
    if (high_nibble < 0) {
      // First nibble (high 4 bits)
      high_nibble = val;
    } else {
      // Second nibble (low 4 bits)
      bin_out[bin_idx++] = (uint8_t)((high_nibble << 4) | val);
      high_nibble = -1;
    }
    
    i++;
  }
  
  return bin_idx;
}

void InitGitProtocolTasks(void)
{

  // Create Message Queue
  // Message Size : Struct Size (Pointer + uint16_t)
  hParsingMsg = osMessageQueueNew( MESSAGE_PARSING_QUEUE_SIZE, sizeof(stTxMessage), NULL );
  hTransmitMsg = osMessageQueueNew( MESSAGE_TRANSMIT_QUEUE_SIZE, sizeof(stTxMessage), NULL );

}



/* =============================================================================
 * Protocol Packet Making Function (Based on git-functionlist.h)
 * =============================================================================
 * Frame Structure: SOF | Len(2B) | FuncID | Payload | CRC16(2B) | EOF
 * - SOF: 0xA0
 * - Len: 2 bytes Little Endian (FuncID + Payload + CRC16(2) + EOF length) = N + 4
 * - FuncID: 1 byte
 * - Payload: Variable length (0~N bytes)
 * - CRC16: 2 bytes LE, CRC-16/IBM (poly=0xA001 reflected)
 *          계산 범위: SOF ~ Payload 끝 (CRC 앞까지 전체)
 * - EOF: 0xB0
 */

/* CRC-16/IBM  (poly=0x8005, reflected → 0xA001, init=0x0000) */
static uint16_t GITPACKET_calculate_crc16(uint8_t *data, uint16_t len)
{
  uint16_t crc = 0x0000;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }
  return crc;
}

/* =============================================================================
 * GITPACKET_make_frame
 * =============================================================================
 * @brief  Protocol Frame Generation (git-functionlist.h base)
 * @param  FunctionID: Function ID (1 byte)
 * @param  pPayload: Payload data pointer (NULL allowed)
 * @param  PayloadLen: Payload length (0~N)
 * @param  pFrameBuff: Output frame buffer
 * @retval Generated frame size (bytes), -1 on failure
 *
 * Frame Structure:
 *   SOF(1) | Len(2, LE) | FuncID(1) | Payload(0~N) | CRC16(2, LE) | EOF(1)
 *   - Len = FuncID(1) + Payload(n) + CRC16(2) + EOF(1) = n + 4
 *   - CRC16 범위: SOF ~ Payload 끝 (pFrameBuff[0] ~ pFrameBuff[3+n-1])
 */
int GITPACKET_make_frame(uint16_t FunctionID, uint8_t *pPayload, uint16_t PayloadLen, uint8_t *pFrameBuff)
{
  if (pFrameBuff == NULL) {
    printf("[ERROR] Frame buffer is NULL\r\n");
    return -1;
  }

  if (FunctionID > 0xFF) {
    printf("[ERROR] FunctionID must be 1 byte (0x%04X)\r\n", FunctionID);
    return -1;
  }

  if (PayloadLen > GITPACKET_PAYLOAD_LEN_MAX) {
    printf("[ERROR] Payload too large (%d > %d)\r\n", PayloadLen, GITPACKET_PAYLOAD_LEN_MAX);
    return -1;
  }

  uint16_t frame_idx = 0;

  /* 1. SOF */
  pFrameBuff[frame_idx++] = GITPACKET_SOF;

  /* 2. Len = FuncID(1) + Payload(n) + CRC16(2) + EOF(1) = n + 4, 2 bytes LE */
  uint16_t len = PayloadLen + 4;
  pFrameBuff[frame_idx++] = (uint8_t)(len & 0xFF);
  pFrameBuff[frame_idx++] = (uint8_t)((len >> 8) & 0xFF);

  /* 3. FuncID */
  pFrameBuff[frame_idx++] = (uint8_t)FunctionID;

  /* 4. Payload (if exists) */
  if (pPayload != NULL && PayloadLen > 0) {
    memcpy(&pFrameBuff[frame_idx], pPayload, PayloadLen);
    frame_idx += PayloadLen;
  }

  /* 5. CRC16 (SOF ~ Payload 끝, 2 bytes LE) */
  uint16_t crc = GITPACKET_calculate_crc16(&pFrameBuff[0], frame_idx);
  pFrameBuff[frame_idx++] = (uint8_t)(crc & 0xFF);
  pFrameBuff[frame_idx++] = (uint8_t)((crc >> 8) & 0xFF);

  /* 6. EOF */
  pFrameBuff[frame_idx++] = GITPACKET_EOF;

  // printf("[FRAME] FuncID=0x%02X, PayloadLen=%d, Len=%d, CRC=0x%04X, TotalSize=%d\r\n",
  //        (uint8_t)FunctionID, PayloadLen, len, crc, frame_idx);

  return frame_idx;
}

/* =============================================================================
 * GITPACKET_parse_frame
 * =============================================================================
 * @brief  Get info from Frame (git-functionlist.h base)
 * @param  pFrameBuff: Input frame buffer
 * @param  FrameSize: Frame size
 * @param  pFuncID: Output function ID pointer
 * @param  ppPayload: Output payload pointer pointer
 * @param  pPayloadLen: Output payload length pointer
 * @retval 0 (success), -1 (failure)
 *
 * Frame Structure:
 *   SOF(1) | Len(2, LE) | FuncID(1) | Payload(0~N) | CRC16(2, LE) | EOF(1)
 */
int GITPACKET_parse_frame(uint8_t *pFrameBuff, uint16_t FrameSize,
                          uint8_t *pFuncID, uint8_t **ppPayload, uint16_t *pPayloadLen)
{
  if (pFrameBuff == NULL) {
    printf("[ERROR] Frame buffer is NULL\r\n");
    return -1;
  }

  /* Minimum frame size check: SOF + Len(2) + FuncID + CRC16(2) + EOF = 7 bytes */
  if (FrameSize < GITPACKET_FRAME_SIZE_MIN) {
    printf("[ERROR] Frame too small (%d < %d)\r\n", FrameSize, GITPACKET_FRAME_SIZE_MIN);
    return -1;
  }

  /* 1. SOF validation */
  if (pFrameBuff[GITPACKET_SOF_IDX] != GITPACKET_SOF) {
    printf("[ERROR] Invalid SOF: 0x%02X (expected 0x%02X)\r\n",
           pFrameBuff[GITPACKET_SOF_IDX], GITPACKET_SOF);
    return -1;
  }

  /* 2. Len extraction (2 bytes Little Endian) */
  uint16_t len = pFrameBuff[GITPACKET_LEN_IDX] | (pFrameBuff[GITPACKET_LEN_IDX + 1] << 8);

  /* 3. Frame size validation: SOF(1) + Len(2) + Len content(len) */
  uint16_t expected_size = 3 + len;
  if (FrameSize != expected_size) {
    printf("[ERROR] Frame size mismatch (actual=%d, expected=%d)\r\n",
           FrameSize, expected_size);
    return -1;
  }

  /* 4. EOF validation */
  uint16_t eof_idx = FrameSize - 1;
  if (pFrameBuff[eof_idx] != GITPACKET_EOF) {
    printf("[ERROR] Invalid EOF: 0x%02X (expected 0x%02X)\r\n",
           pFrameBuff[eof_idx], GITPACKET_EOF);
    return -1;
  }

  /* 5. FuncID extraction */
  uint8_t func_id = pFrameBuff[GITPACKET_FUNCID_IDX];
  if (pFuncID != NULL) {
    *pFuncID = func_id;
  }

  /* 6. Payload length: Len - FuncID(1) - CRC16(2) - EOF(1) = Len - 4 */
  uint16_t payload_len = (len >= 4) ? (len - 4) : 0;
  if (pPayloadLen != NULL) {
    *pPayloadLen = payload_len;
  }

  /* 7. Payload pointer */
  if (ppPayload != NULL) {
    *ppPayload = (payload_len > 0) ? &pFrameBuff[GITPACKET_PAYLOAD_IDX] : NULL;
  }

  /* 8. CRC16 validation
   *    CRC 위치: FrameSize - 3 (CRC_L), FrameSize - 2 (CRC_H)
   *    CRC 계산 범위: pFrameBuff[0] ~ pFrameBuff[FrameSize - 4]  (SOF ~ Payload 끝) */
  uint16_t crc_idx   = FrameSize - 3;
  uint16_t recv_crc  = pFrameBuff[crc_idx] | ((uint16_t)pFrameBuff[crc_idx + 1] << 8);
  uint16_t calc_crc  = GITPACKET_calculate_crc16(&pFrameBuff[0], FrameSize - 3);

  if (recv_crc != calc_crc) {
    printf("[ERROR] CRC16 mismatch (recv=0x%04X, calc=0x%04X)\r\n", recv_crc, calc_crc);
    return -1;
  }
  printf("[REQ] FuncID=0x%02X\r\n", func_id);
  // printf("[PARSE] FuncID=0x%02X, PayloadLen=%d, Len=%d, CRC=0x%04X\r\n",
  //        func_id, payload_len, len, recv_crc);
  return 0;
}

/* =============================================================================
 * GITPACKET_make_response
 * =============================================================================
 * @brief  Create response frame (ACK/NAK)
 * @param  FunctionID: Function ID (1 byte)
 * @param  isAck: true=ACK(0x00), false=NAK(0x01)
 * @param  pFrameBuff: Output frame buffer
 * @retval Created frame size (bytes), -1 on failure
 * 
 * Response Frame: SOF | Len(2B, 0x0005) | FuncID | ACK/NAK | CRC16(2B) | EOF
 */
int GITPACKET_make_response(uint8_t FunctionID, uint8_t ack, uint8_t *pFrameBuff)
{
  return GITPACKET_make_frame(FunctionID, &ack, 1, pFrameBuff);
}

/* =============================================================================
 * GITPACKET_send_response
 * =============================================================================
 * @brief  ACK/NAK 응답 프레임 생성 후 UART2로 전송 (Convenience Function)
 * @param  FunctionID: 함수 ID (1 byte)
 * @param  isAck: 1=ACK(0x00), 0=NAK(0x01)
 * @retval 0=성공, -1=실패
 *
 * Response Frame: SOF | Len(2B, 0x0005) | FuncID | ACK/NAK | CRC16(2B) | EOF
 */
int GITPACKET_send_response(uint8_t FunctionID, uint8_t ack)
{
  uint8_t frame_buff[GITPACKET_FRAME_SIZE_MIN + 1];  // SOF+Len(2)+FuncID+ACK/NAK+CRC16(2)+EOF = 8 bytes
  int frame_size = GITPACKET_make_response(FunctionID, ack, frame_buff);
  if (frame_size < 0) {
    printf("[RESP] Failed to make response frame (FuncID=0x%02X, %s)\r\n",
           FunctionID, (ack == GITPACKET_ACK) ? "ACK" : "NAK");
    return -1;
  }

  HAL_StatusTypeDef status = GITPACKET_send_frame_via_uart(frame_buff, (uint16_t)frame_size);
  if (status != HAL_OK) {
    printf("[RESP] Failed to send response frame (status=%d)\r\n", status);
    return -1;
  }

  printf("[RESP] Sent %s for FuncID=0x%02X \r\n",
         (ack == GITPACKET_ACK) ? "ACK" : "NAK", FunctionID);
  return 0;
}

/* =============================================================================
 * UART Transmit Frame Function (Direct - Binary)
 * =============================================================================
 * @brief  UART2를 통해 프레임 전송 (Binary)
 * @param  pFrameData: 전송할 프레임 데이터
 * @param  frameSize: 프레임 크기
 * @retval HAL_OK (성공), HAL_ERROR 또는 HAL_TIMEOUT (실패)
 */
HAL_StatusTypeDef GITPACKET_send_frame_via_uart(uint8_t *pFrameData, uint16_t frameSize)
{
  HAL_StatusTypeDef status;

  if (pFrameData == NULL || frameSize == 0) {
    printf("[UART-TX] Error: Invalid frame data or size\r\n");
    return HAL_ERROR;
  }

#ifdef DEBUG_RAW_UART2
  /* Frame 정보 출력 */
  uint8_t sof = pFrameData[GITPACKET_SOF_IDX];  
  uint16_t len = pFrameData[GITPACKET_LEN_IDX] | (pFrameData[GITPACKET_LEN_IDX + 1] << 8);
  uint8_t func_id = pFrameData[GITPACKET_FUNCID_IDX];

  printf("[UART-TX] Transmitting Frame (Binary)...\r\n");
  printf("[UART-TX] SOF=0x%02X, Len=%d, FuncID=0x%02X, TotalSize=%d\r\n",
         sof, len, func_id, frameSize);
  printf("[UART-TX] Frame Hex: ");
  for (uint16_t i = 0; i < frameSize; i++) {
    printf("%02X ", pFrameData[i]);
  }
  printf("\r\n");
#endif
  /* HAL UART Transmit - UART2로 Frame 전송 (타임아웃: 1000ms) */
  status = HAL_UART_Transmit(&huart2, (uint8_t *)pFrameData, frameSize, 1000);

  if (status == HAL_OK) {
    printf("[UART-TX] Transmit success! (Size: %d bytes)\r\n\r\n", frameSize);
  } else if (status == HAL_TIMEOUT) {
    printf("[UART-TX] Transmit timeout!\r\n\r\n");
  } else {
    printf("[UART-TX] Transmit error! (status: %d)\r\n\r\n", status);
  }

  return status;
}

/* =============================================================================
 * UART Transmit Frame as HEX String Function
 * =============================================================================
 * @brief  UART2를 통해 프레임을 HEX 문자열로 전송 (디버깅용)
 * @param  pFrameData: 전송할 프레임 데이터
 * @param  frameSize: 프레임 크기
 * @retval HAL_OK (성공), HAL_ERROR 또는 HAL_TIMEOUT (실패)
 * @note   Binary 0x11 0x22 0x33 -> "11 22 33\r\n" 형태로 변환
 */
HAL_StatusTypeDef GITPACKET_send_frame_via_uart_hex(uint8_t *pFrameData, uint16_t frameSize)
{
  HAL_StatusTypeDef status;
  static uint8_t hex_buffer[2048];  // HEX string buffer (max 1024 * 3 bytes)
  uint16_t hex_len = 0;

  if (pFrameData == NULL || frameSize == 0) {
    printf("[UART-TX-HEX] Error: Invalid frame data or size\r\n");
    return HAL_ERROR;
  }

  if (frameSize * 3 > sizeof(hex_buffer)) {
    printf("[UART-TX-HEX] Error: Frame too large for buffer\r\n");
    return HAL_ERROR;
  }

  /* Frame 정보 출력 */
  uint8_t sof = pFrameData[GITPACKET_SOF_IDX];
  uint16_t len = pFrameData[GITPACKET_LEN_IDX] | (pFrameData[GITPACKET_LEN_IDX + 1] << 8);
  uint8_t func_id = pFrameData[GITPACKET_FUNCID_IDX];

  printf("[UART-TX-HEX] Transmitting Frame (HEX String)...\r\n");
  printf("[UART-TX-HEX] SOF=0x%02X, Len=%d, FuncID=0x%02X, TotalSize=%d\r\n",
         sof, len, func_id, frameSize);
  printf("[UART-TX-HEX] Frame Hex: ");
  
  /* 각 바이트를 HEX 문자열로 변환 */
  for (uint16_t i = 0; i < frameSize; i++) {
    hex_len += sprintf((char*)&hex_buffer[hex_len], "%02X ", pFrameData[i]);
  }
  printf("%s\r\n", (char*)hex_buffer);

  /* 마지막에 CRLF 추가 */
  hex_buffer[hex_len++] = '\r';
  hex_buffer[hex_len++] = '\n';

  /* HAL UART Transmit - UART2로 HEX 문자열 전송 */
  status = HAL_UART_Transmit(&huart2, hex_buffer, hex_len, 1000);

  if (status == HAL_OK) {
    printf("[UART-TX-HEX] Transmit success! (HEX String Size: %d bytes)\r\n\r\n", hex_len);
  } else if (status == HAL_TIMEOUT) {
    printf("[UART-TX-HEX] Transmit timeout!\r\n\r\n");
  } else {
    printf("[UART-TX-HEX] Transmit error! (status: %d)\r\n\r\n", status);
  }

  return status;
}

/* =============================================================================
 * TEST FUNCTION : 프로토콜 패킷 생성 및 동작 예제
 * =============================================================================
 */
void GITPACKET_test_example(void)
{
  /* 기존 함수: transmit/receive 테스트를 각각 호출 */
  // GITPACKET_test_transmit();
  osDelay(100);
  GITPACKET_test_receive();
}

/* =============================================================================
 * Transmit Test Function (git-functionlist.h 구조 기반)
 * =============================================================================
 * @brief  최대 크기 프레임 생성 및 UART 전송 테스트 (259 bytes)
 *
 *  ┌──────────────────────────────────────────────────────────────┐
 *  │  프레임 구조 (Payload=252B → 총 259 bytes)                   │
 *  │                                                              │
 *  │  [0]       SOF       = 0xA0                                  │
 *  │  [1~2]     Len       = 0x00 0x01 (256, LE)                  │
 *  │  [3]       FuncID    = 0x14                                  │
 *  │  [4~255]   Payload   = 252 bytes                             │
 *  │  [256~257] CRC16     = CRC16(SOF~Payload)                   │
 *  │  [258]     EOF       = 0xB0                                  │
 *  │                                                              │
 *  │  Len = FuncID(1) + Payload(252) + CRC16(2) + EOF(1) = 256   │
 *  │  Payload 내용 (BSA 방전 모니터링 예시):                       │
 *  │    [0]     Status          (0x01 = Normal)                   │
 *  │    [1~2]   Voltage         (0x02 0x58 = 600V)                │
 *  │    [3~4]   Current         (0x00 0x14 = 20A)                 │
 *  │    [5]     Temperature     (0x23 = 35도)                     │
 *  │    [6]     SOC             (0x50 = 80%)                      │
 *  │    [7]     Error Code      (0x00 = No Error)                 │
 *  │    [8~251] Reserved/Dummy  (0x00 패딩)                       │
 *  └──────────────────────────────────────────────────────────────┘
 */
void GITPACKET_test_transmit(void)
{
  uint8_t frame_buff[GITPACKET_FRAME_SIZE_MAX];
  int frame_size = 0;

  /* 페이로드 252 bytes → Len=256(0x100), 총 프레임=259 bytes */
  #define TEST_PAYLOAD_MAX  252
  uint8_t payload[TEST_PAYLOAD_MAX] = {0};

  /* BSA 방전 모니터링 데이터 예시 */
  payload[0] = 0x01;        // Status: Normal
  payload[1] = 0x02;        // Voltage High (600V)
  payload[2] = 0x58;        // Voltage Low
  payload[3] = 0x00;        // Current High (20A)
  payload[4] = 0x14;        // Current Low
  payload[5] = 0x23;        // Temperature (35도)
  payload[6] = 0x50;        // SOC (80%)
  payload[7] = 0x00;        // Error Code (No Error)
  // payload[8~251] = 0x00  (Reserved, already zeroed)

  uint8_t func_id = 0x14;

  // printf("\r\n========== GITPACKET MAX FRAME TEST (259 bytes) ==========\r\n");

  /* 1. 최대 크기 Frame 생성 */
  frame_size = GITPACKET_make_frame(func_id, payload, TEST_PAYLOAD_MAX, frame_buff);

  if (frame_size > 0) {
    printf("  Frame Size = %d bytes (max=%d)\r\n", frame_size, GITPACKET_FRAME_SIZE_MAX);

    // /* 프레임 구조 출력 */
    // printf("  [0]     SOF     = 0x%02X\r\n", frame_buff[0]);
    // printf("  [1]     Len     = 0x%02X (%d)\r\n", frame_buff[1], frame_buff[1]);
    // printf("  [2]     FuncID  = 0x%02X\r\n", frame_buff[2]);
    // printf("  [3~254] Payload = %d bytes\r\n", TEST_PAYLOAD_MAX);
    // printf("  [255]   CS      = 0x%02X\r\n", frame_buff[frame_size - 2]);
    // printf("  [256]   EOF     = 0x%02X\r\n", frame_buff[frame_size - 1]);

    /* HEX 덤프 (16 bytes/row) */
    // printf("\r\n  HEX Dump:\r\n");
    // for (int i = 0; i < frame_size; i++) {
    //   if (i % 16 == 0) printf("  %04X: ", i);
    //   printf("%02X ", frame_buff[i]);
    //   if (i % 16 == 15 || i == frame_size - 1) printf("\r\n");
    // }
  }
  else {
    printf("  Failed to create frame!\r\n");
    return;
  }

  /* 2. UART2로 Frame 전송 */
  // printf("\r\n  Sending via UART2...\r\n");
  memcpy(g_TxFrameBuffer, frame_buff, frame_size);
  HAL_StatusTypeDef status = GITPACKET_send_frame_via_uart(g_TxFrameBuffer, frame_size);

  printf("  Result: %s\r\n", (status == HAL_OK) ? "OK" : "FAIL");
  // printf("========== GITPACKET MAX FRAME TEST END ==========\r\n\r\n");
}

/* =============================================================================
 * Receive Test Function (git-functionlist.h 구조 기반)
 * =============================================================================
 * @brief  UART2 링 버퍼에서 프레임 수신 및 파싱
 * @note   프레임 구조: SOF(0xA0) | Len(2B, LE) | FuncID | Payload | CS | EOF(0xB0)
 */
void GITPACKET_test_receive(void)
{
  static uint8_t rx_buffer[4096];      // UART2에서 읽어온 데이터 버퍼
  static uint8_t frame_buffer[1024];   // 추출된 프레임 저장
  static uint16_t buffer_len = 0;      // rx_buffer 내 유효 데이터 길이
  
  // Step 1: UART2 링 버퍼에서 새로운 데이터 읽어오기
  uint16_t new_bytes = UART2_Receive_Interrupt(rx_buffer + buffer_len, sizeof(rx_buffer) - buffer_len);

  if (new_bytes > 0) {
    buffer_len += new_bytes;
    printf("\r\n[RX] Received %u bytes, total buffered: %u\r\n", new_bytes, buffer_len);
  }
  
  // 데이터가 없으면 리턴
  if (buffer_len == 0) {
    return;
  }
  
  // Step 2: 버퍼에서 프레임 검색 및 처리
  printf("[RX] Starting frame parsing...\r\n");
  
  while (buffer_len > 0) {
    // Step 2-1: SOF 찾기 (0xA0)
    uint16_t sof_idx = 0;
    int found_sof = 0;
    
    for (uint16_t i = 0; i < buffer_len; i++) {
      if (rx_buffer[i] == GITPACKET_SOF) {
        sof_idx = i;
        found_sof = 1;
        break;
      }
    }
    
    if (!found_sof) {
      // SOF 없음: 버퍼 전체 버림
      printf("[RX] No SOF found, discarding %u bytes\r\n", buffer_len);
      buffer_len = 0;
      break;
    }
    
    // SOF 이전 데이터는 버림
    if (sof_idx > 0) {
      printf("[RX] Discarding %u bytes before SOF\r\n", sof_idx);
      memmove(rx_buffer, rx_buffer + sof_idx, buffer_len - sof_idx);
      buffer_len -= sof_idx;
    }
    
    // Step 2-2: 최소 프레임 크기 확인 (SOF + Len(2) + FuncID + CS + EOF = 6 bytes)
    if (buffer_len < GITPACKET_FRAME_SIZE_MIN) {
      printf("[RX] Insufficient data for frame header (need %d, have %u)\r\n", GITPACKET_FRAME_SIZE_MIN, buffer_len);
      break;  // 더 기다림
    }

    // Step 2-3: 프레임 전체 길이 계산
    // Frame: SOF(1) + Len(2) + (Len 내용)
    // Len = FuncID(1) + Payload(n) + CS(1) + EOF(1)
    uint16_t len = rx_buffer[GITPACKET_LEN_IDX] | (rx_buffer[GITPACKET_LEN_IDX + 1] << 8);
    uint16_t total_frame_size = 3 + len;  // SOF(1) + Len(2) + Len내용(len)
    
    printf("[RX] Frame Len field: %u, Total frame size: %u bytes\r\n", len, total_frame_size);
    
    if (total_frame_size > sizeof(frame_buffer)) {
      printf("[RX] Frame too large (%u bytes), discarding\r\n", total_frame_size);
      // SOF 제거 후 다음 SOF 탐색
      memmove(rx_buffer, rx_buffer + 1, buffer_len - 1);
      buffer_len -= 1;
      continue;
    }
    
    // Step 2-4: 전체 프레임이 버퍼에 도착했는지 확인
    if (buffer_len < total_frame_size) {
      printf("[RX] Incomplete frame (need %u, have %u), waiting...\r\n", total_frame_size, buffer_len);
      break;  // 더 기다림
    }
    
    // Step 2-5: 프레임 추출
    memcpy(frame_buffer, rx_buffer, total_frame_size);
    
    printf("[RX] Extracted frame (%u bytes): ", total_frame_size);
    for (uint16_t i = 0; i < total_frame_size; i++) {
      printf("%02X ", frame_buffer[i]);
    }
    printf("\r\n");
    
    // Step 3: 프레임 파싱
    printf("[RX] Parsing frame...\r\n");
    uint8_t func_id = 0;
    uint8_t *payload = NULL;
    uint16_t payload_len = 0;
    
    int parse_result = GITPACKET_parse_frame(frame_buffer, total_frame_size, 
                                             &func_id, &payload, &payload_len);
    
    if (parse_result == 0) {
      printf("[RX] Parse success - FuncID=0x%02X, PayloadLen=%u\r\n", func_id, payload_len);
      
      if (payload_len > 0 && payload != NULL) {
        printf("[RX] Payload data: ");
        for (uint16_t i = 0; i < payload_len; i++) {
          printf("%02X ", payload[i]);
        }
        printf("\r\n");
      }
      
      // TODO: Function ID에 따른 처리 로직
      // - Function List에서 해당 ID 찾기
      // - 콜백 함수 호출
      
      // 응답 프레임 생성 및 전송 (ACK)
      uint8_t response_frame[16];
      int response_size = GITPACKET_make_response(func_id, 1, response_frame);  // ACK
      if (response_size > 0) {
        GITPACKET_send_frame_via_uart_hex(response_frame, response_size);
      }
      
    } else {
      printf("[RX] Parse failed, discarding frame\r\n");
    }
    
    // Step 4: 처리 완료된 프레임 데이터 제거
    memmove(rx_buffer, rx_buffer + total_frame_size, buffer_len - total_frame_size);
    buffer_len -= total_frame_size;
    
    printf("[RX] Frame consumed, remaining buffer: %u bytes\r\n\r\n", buffer_len);
  }
}

/* =============================================================================
 * UART Transmit Function (Message Queue 기반)
 * =============================================================================
 * @brief  메시지 큐에서 프레임을 받아 UART2로 전송
 * @note   Task에서 호출되는 함수
 */
void GITPACKET_transmit(void)
{
  osStatus_t msg_status;

  /* Receive Message in Message Queue */
  msg_status = osMessageQueueGet(hTransmitMsg, (void*)&g_TxMsg, NULL, 10);  // 10ms timeout

  if (msg_status == osOK && g_TxMsg.pFrameData != NULL) {
    /* Message 수신 성공 */
    printf("\r\n[TX] Frame received from queue\r\n");

    /* Frame 정보 출력 */
    uint8_t sof = g_TxMsg.pFrameData[GITPACKET_SOF_IDX];
    uint16_t len = g_TxMsg.pFrameData[GITPACKET_LEN_IDX] | (g_TxMsg.pFrameData[GITPACKET_LEN_IDX + 1] << 8);
    uint8_t func_id = g_TxMsg.pFrameData[GITPACKET_FUNCID_IDX];

    printf("[TX] SOF=0x%02X, Len=%d, FuncID=0x%02X, MessageSize=%d\r\n", 
           sof, len, func_id, g_TxMsg.frameSize);
    printf("[TX] Frame Hex: ");
    for (int i = 0; i < g_TxMsg.frameSize; i++) {
      printf("%02X ", g_TxMsg.pFrameData[i]);
    }
    printf("\r\n");

    /* HAL UART Transmit - UART2로 프레임 전송 */
    HAL_StatusTypeDef uart_status = HAL_UART_Transmit(&huart2, 
                                                       (uint8_t *)g_TxMsg.pFrameData, 
                                                       g_TxMsg.frameSize, 
                                                       1000);

    if (uart_status == HAL_OK) {
      printf("[TX] UART2 transmit success! (Size: %d bytes)\r\n", g_TxMsg.frameSize);
    } else if (uart_status == HAL_TIMEOUT) {
      printf("[TX] UART2 transmit timeout!\r\n");
    } else {
      printf("[TX] UART2 transmit error! (status: %d)\r\n", uart_status);
    }

  } else if (msg_status == osErrorTimeout || msg_status == osErrorResource) {
    /* 타임아웃 - 메시지 없음 (정상) */
    // 아무 동작 없음
  } else if (msg_status != osOK) {
    printf("[TX] MessageQueue error! (status: %d)\r\n", msg_status);
  }
}
