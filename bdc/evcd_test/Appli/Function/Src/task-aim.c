#include "../Inc/sys-common.h"
#include "../Inc/git-functionlist.h"
#include "../Inc/task-hwcontrol.h"
#include "../Inc/task-lcd.h"
#include "../Inc/task-aim.h"

/*
PIN config
    AIM_RX      : UART4_RX (MCU)
    AIM_TX      : UART4_TX (MCU)
    AIM_DIR     : LOW = RX (to MCU), HIGH = TX (to AIM)  -- MAX3485 RE+DE 공통(PD14)

UART4 config
    Baudrate    : 9600 (AIM default)
    Data bits   : 8
    Parity      : None
    Stop bits   : 1
    Flow control: None

통신 방식 : Modbus-RTU 마스터. MCU가 요청(Request)해야 AIM(슬레이브)이 응답(Response).
            trigger mode = Cycle(기본) → AIM이 자체 주기 측정, MCU는 1초마다 레지스터 폴링.
*/

/* AIM 모니터링 값 (stAIM_Data, task-aim.h 정의) — consumer는 g_aimData 로 참조 */
stAIM_Data g_aimData = {0};

/* 자동 폴링 on/off (CLI 수동 테스트 시 DIR/버스 간섭 방지용). 기본 ON */
volatile bool g_aim_poll_enable = true;

osThreadId_t        aimTaskHandle;
uint32_t            aimTaskBuffer[2048];
osStaticThreadDef_t aimTaskControlBlock;
const osThreadAttr_t aimTask_attributes = {
  .name       = "aimTask",
  .cb_mem     = &aimTaskControlBlock,
  .cb_size    = sizeof(aimTaskControlBlock),
  .stack_mem  = &aimTaskBuffer[0],
  .stack_size = sizeof(aimTaskBuffer),
  .priority   = (osPriority_t) osPriorityNormal,
};

static osMutexId_t s_aim_mutex = NULL;   /* UART4 버스 접근 보호 (aimTask <-> CLI) */
static volatile HAL_StatusTypeDef s_aim_last_rx_status = HAL_OK;  /* 마지막 수신 HAL 상태(진단용) */

static uint16_t   AIM_crc16(const uint8_t *data, uint16_t len);
static int        AIM_make_frame_read_req(uint8_t *tx, uint16_t reg, uint16_t cnt);
static int        AIM_xfer(const uint8_t *tx, int txlen, uint8_t *rx, int rxexp, uint32_t timeout);
static HAL_StatusTypeDef AIM_process(uint8_t *tx, int txlen, uint8_t *rx, int rxlen, uint32_t timeout);
void StartAimTask(void *argument);

static inline void AIM_lock(void)   { if (s_aim_mutex) osMutexAcquire(s_aim_mutex, osWaitForever); }
static inline void AIM_unlock(void) { if (s_aim_mutex) osMutexRelease(s_aim_mutex); }

/* ===========================================================================
 * CRC-16/MODBUS : poly=0xA001(reflected), init=0xFFFF
 *   검증: 01 03 00 25 00 01 → 0xC195 → append(LE) = 95 C1
 * =========================================================================== */
static uint16_t AIM_crc16(const uint8_t *data, uint16_t len)
{
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (int b = 0; b < 8; b++)
      crc = (crc & 0x0001) ? (uint16_t)((crc >> 1) ^ 0xA001) : (uint16_t)(crc >> 1);
  }
  return crc;
}

void AIM_init(void)
{
  AIM_set_dir(false);                 // direction default = RX (to MCU)
  __HAL_UART_CLEAR_OREFLAG(&huart4);

  g_aimData.comm_ok         = false;
  g_aimData.fault_status    = 0;
  g_aimData.insul_res_p_kohm = 0;
  g_aimData.insul_res_n_kohm = 0;
  g_aimData.volt_p_V        = 0.0f;
  g_aimData.volt_n_V        = 0.0f;
  g_aimData.sys_volt_V      = 0.0f;
  g_aimData.last_ok_tick    = 0;
}

void AIM_set_dir(bool dir)  // MAX3485 RE pin
{
  if (dir)  IO_CONTROL_HIGH(AIM_DIR);   /* TX (to AIM) */
  else      IO_CONTROL_LOW(AIM_DIR);    /* RX (to MCU) */
}

static int AIM_make_frame_read_req(uint8_t *tx, uint16_t reg, uint16_t cnt)
{
  // [Addr][03][Reg_H Reg_L][Cnt_H Cnt_L][CRC_L CRC_H]  

  tx[0] = AIM_SLAVE_ADDRESS;
  tx[1] = AIM_FUNC_READ_03H;
  // Data Zone
  tx[2] = (uint8_t)(reg >> 8);      // register address (big-endian)
  tx[3] = (uint8_t)(reg & 0xFF);
  tx[4] = (uint8_t)(cnt >> 8);      // count (big-endian)
  tx[5] = (uint8_t)(cnt & 0xFF);

  // CRC16 (LE)
  uint16_t crc = AIM_crc16(tx, 6);
  tx[6] = (uint8_t)(crc & 0xFF);    
  tx[7] = (uint8_t)(crc >> 8);
  return 8;
}

/* 저수준 반이중 교환 (뮤텍스 보호). 실제 수신 바이트 수 반환.
 *   DIR_TX → 송신(TC까지 블로킹) → DIR_RX → 수신(timeout)
 *   ★ HAL_UART_Transmit 는 TC까지 대기 후 리턴 → 직후 RX 전환해도 마지막 바이트 안 잘림 */
static int AIM_xfer(const uint8_t *tx, int txlen, uint8_t *rx, int rxexp, uint32_t timeout)
{
  AIM_lock();

  /* --- 송신 (DIR=TX) --- HAL_UART_Transmit 는 TC(전송완료)까지 대기 후 리턴 */
  AIM_set_dir(true);                                        // DIR : to AIM (TX)
  HAL_UART_Transmit(&huart4, (uint8_t *)tx, (uint16_t)txlen, 50);

  /* --- 수신 (DIR=RX) --- */
  AIM_set_dir(false);                                       // DIR : to MCU (RX) — TC 직후
  /* 래치된 에러플래그(ORE/NE/FE/PE) 클리어 + RDR 잔류 바이트 폐기 */
  __HAL_UART_CLEAR_FLAG(&huart4, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF | UART_CLEAR_PEF);
  __HAL_UART_SEND_REQ(&huart4, UART_RXDATA_FLUSH_REQUEST);

  /* RS-485 턴어라운드로 선행 잡음 바이트(예: 0x3F)가 들어올 수 있어,
   * 프레임 시작(슬레이브 주소)까지 잡음을 버리고 그 다음부터 정프레임 수신 (frame resync) */
  int rcvd = 0;
  HAL_StatusTypeDef st = HAL_TIMEOUT;
  uint32_t t0 = HAL_GetTick();
  uint8_t  b  = 0;

  while ((uint32_t)(HAL_GetTick() - t0) < timeout)
  {
    if (HAL_UART_Receive(&huart4, &b, 1, timeout) != HAL_OK) break;   /* 타임아웃/에러 */
    if (b == AIM_SLAVE_ADDRESS) { rx[0] = b; rcvd = 1; break; }       /* 프레임 시작 발견 */
    /* else : 선행 잡음 → 폐기하고 계속 */
  }

  if (rcvd == 1 && rxexp > 1)                               /* 나머지 프레임 수신 */
  {
    st = HAL_UART_Receive(&huart4, &rx[1], (uint16_t)(rxexp - 1), timeout);
    rcvd += (rxexp - 1) - (int)huart4.RxXferCount;
  }

  s_aim_last_rx_status = (rcvd >= rxexp) ? HAL_OK : st;
  if (rcvd < rxexp) HAL_UART_AbortReceive(&huart4);         // RX 상태 리셋

  AIM_unlock();
  return (rcvd < 0) ? 0 : rcvd;
}

static HAL_StatusTypeDef AIM_process(uint8_t *tx, int txlen,
                                     uint8_t *rx, int rxlen, uint32_t timeout)
{
  int rcvd = AIM_xfer(tx, txlen, rx, rxlen, timeout);

  // verify : 길이 / slave address / function code (exception → 0x80 set) / CRC
  if (rcvd < rxlen)                          return HAL_TIMEOUT;
  if (rx[0] != AIM_SLAVE_ADDRESS)            return HAL_ERROR;
  if ((rx[1] & 0x7F) != AIM_FUNC_READ_03H)   return HAL_ERROR;
  uint16_t crc   = AIM_crc16(rx, (uint16_t)(rxlen - 2));
  uint16_t rxcrc = (uint16_t)rx[rxlen - 2] | ((uint16_t)rx[rxlen - 1] << 8);
  if (crc != rxcrc)                          return HAL_ERROR;

  return HAL_OK;
}

/* 마지막 수신 HAL 상태 (CLI 실패사유 진단용)
 *   HAL_TIMEOUT(3)=응답 전무, HAL_ERROR(1)=프레이밍/노이즈, HAL_OK(0)=수신됨(CRC/길이는 별도) */
HAL_StatusTypeDef AIM_GetLastRxStatus(void) { return s_aim_last_rx_status; }

/* 
 * 모니터링 값 갱신 : 0x20~0x25 6개 일괄 읽기 후 전역 저장
 *   요청 : 01 03 00 20 00 06 [CRC]              (8 bytes)
 *   응답 : 01 03 0C <12 data> [CRC]             (17 bytes)
 *   스케일 : 저항 x1 [kOhm], 전압 x0.1 [V]
 */
void AIM_read_monitor(void)
{
  uint8_t tx[8];
  uint8_t rx[17];

  int n = AIM_make_frame_read_req(tx, AIM_REG_FAULT_TYPE, 6);   /* 0x20 부터 6개 */

  if (AIM_process(tx, n, rx, sizeof(rx), 1500) != HAL_OK)
  {
    g_aimData.comm_ok = false;                                  /* 이전값 유지 */
    // printf("[AIM] read fail (comm timeout/crc)\r\n");
    return;
  }

  if (rx[2] != 0x0C)                                        /* ByteCount = 12 */
  {
    g_aimData.comm_ok = false;
    printf("[AIM] read fail (bytecount=0x%02X)\r\n", rx[2]);
    return;
  }

  g_aimData.fault_status     = (uint16_t)((rx[3]  << 8) | rx[4]);            /* 0x20 */
  g_aimData.insul_res_p_kohm = (int32_t) ((rx[5]  << 8) | rx[6]);            /* 0x21 kOhm */
  g_aimData.insul_res_n_kohm = (int32_t) ((rx[7]  << 8) | rx[8]);            /* 0x22 kOhm */
  g_aimData.volt_p_V         = (float)   ((rx[9]  << 8) | rx[10]) * 0.1f;    /* 0x23 V   */
  g_aimData.volt_n_V         = (float)   ((rx[11] << 8) | rx[12]) * 0.1f;    /* 0x24 V   */
  g_aimData.sys_volt_V       = (float)   ((rx[13] << 8) | rx[14]) * 0.1f;    /* 0x25 V   */

  g_aimData.comm_ok      = true;
  g_aimData.last_ok_tick = HAL_GetTick();
}

/* ===========================================================================
 * CLI/디버그용 공개 API : 매뉴얼 기반 임의 레지스터 read/write 테스트
 *   txBuf/rxBuf 에 실제 송/수신 바이트를 담고, 수신 길이를 *rxLen 에 반환.
 *   반환 : true = 주소/함수/CRC 검증 통과, false = 실패(단 rxBuf 는 덤프 가능)
 * =========================================================================== */
bool AIM_Cli_ReadReg(uint16_t reg, uint16_t cnt,
                     uint8_t *txBuf, int *txLen,
                     uint8_t *rxBuf, int rxCap, int *rxLen)
{
  int tl  = AIM_make_frame_read_req(txBuf, reg, cnt);
  *txLen  = tl;

  int exp = 5 + (int)cnt * 2;                 /* Addr+Func+ByteCount + data(2*cnt) + CRC2 */
  if (exp > rxCap) exp = rxCap;

  int rcvd = AIM_xfer(txBuf, tl, rxBuf, exp, 1500);
  *rxLen   = rcvd;

  if (rcvd < 5)                                return false;
  if (rxBuf[0] != AIM_SLAVE_ADDRESS)           return false;
  if ((rxBuf[1] & 0x7F) != AIM_FUNC_READ_03H)  return false;
  uint16_t crc   = AIM_crc16(rxBuf, (uint16_t)(rcvd - 2));
  uint16_t rxcrc = (uint16_t)rxBuf[rcvd - 2] | ((uint16_t)rxBuf[rcvd - 1] << 8);
  return (crc == rxcrc);
}

bool AIM_Cli_WriteReg(uint16_t reg, uint16_t val,
                      uint8_t *txBuf, int *txLen,
                      uint8_t *rxBuf, int rxCap, int *rxLen)
{
  /* Func 06 : [Addr][06][Reg_H Reg_L][Val_H Val_L][CRC_L CRC_H] (8B), 정상응답 = 요청 echo */
  txBuf[0] = AIM_SLAVE_ADDRESS;
  txBuf[1] = AIM_FUNC_WRITE_SINGLE;
  txBuf[2] = (uint8_t)(reg >> 8);
  txBuf[3] = (uint8_t)(reg & 0xFF);
  txBuf[4] = (uint8_t)(val >> 8);
  txBuf[5] = (uint8_t)(val & 0xFF);
  uint16_t crc = AIM_crc16(txBuf, 6);
  txBuf[6] = (uint8_t)(crc & 0xFF);
  txBuf[7] = (uint8_t)(crc >> 8);
  *txLen = 8;

  int exp = 8;
  if (exp > rxCap) exp = rxCap;
  int rcvd = AIM_xfer(txBuf, 8, rxBuf, exp, 1500);
  *rxLen = rcvd;

  if (rcvd < 8) return false;
  for (int i = 0; i < 8; i++) if (rxBuf[i] != txBuf[i]) return false;   /* echo 확인 */
  return true;
}

/* Task : 1s polling */

void StartAimTask(void *argument)
{
  (void)argument;

  vTaskDelay(50);
  AIM_init();
  printf("start %s ... \r\n", __FUNCTION__);

  for (;;)
  {
    if (g_aim_poll_enable)          /* 수동 테스트(aim poll off) 중엔 DIR/버스 건드리지 않음 */
    {
      AIM_read_monitor();

      if (g_aimData.comm_ok)
      {
        printf("[AIM] R+=%ld R-=%ld kOhm  Vsys=%ld.%ldV  st=0x%04X\r\n",
               (long)g_aimData.insul_res_p_kohm, (long)g_aimData.insul_res_n_kohm,
               (long)g_aimData.sys_volt_V,       (long)(g_aimData.sys_volt_V * 10) % 10,
               (unsigned)g_aimData.fault_status);
      }
    }

    osDelay(1000);
  }
}

void InitAimTask(void)
{
  s_aim_mutex   = osMutexNew(NULL);   /* 버스 뮤텍스: 태스크 생성 전에 준비 */
  aimTaskHandle = osThreadNew(StartAimTask, NULL, &aimTask_attributes);
}
