/**
 * ******************************************************************************
 * @file    git-comm.c
 * @brief   BLE Task (UART2) - Communication Manager
 *
 *          ┌──────────────────────────────────────────────┐
 *          │  BLE Task (UART2)          [상시 실행]        │
 *          │  ┌────────────────────────────────────────┐  │
 *          │  │           UART2 RX (인터럽트)          │  │
 *          │  │    ISR → Ring Buffer → UART2_read_buff │  │
 *          │  └──────────────┬─────────────────────────┘  │
 *          │                 │                            │
 *          │       ┌────────▼─────────┐                   │
 *          │       │  BT Connected?   │                   │
 *          │       └───┬──────────┬───┘                   │
 *          │       YES │          │ NO                    │
 *          │   ┌───────▼──────┐ ┌─▼──────────────────┐   │
 *          │   │ GIT Protocol │ │  AT CMD Response    │   │
 *          │   │ SOF(0xA0)    │ │  "READY","OK",...   │   │
 *          │   │ → FuncID     │ │  BLE_ProcessRxQueue │   │
 *          │   │ → Dispatch   │ └─────────────────────┘   │
 *          │   └──────────────┘                           │
 *          └──────────────────────────────────────────────┘
 *
 *  트리거: INIT 완료 후 상시 실행
 *  웨이크: vTaskDelay(10) 폴링 (UART2 RX 인터럽트 → 링버퍼 → Task에서 처리)
 *
 *  RX 분기 로직:
 *   1. BT Connected → GIT Protocol 파싱 (SOF 0xA0 감지 → FuncID → 함수 디스패치)
 *   2. BT Disconnected → AT CMD 응답 파싱 ("READY", "OK", "+BTSTATE" 등)
 *
 *    USART1 TX : PA9      USART2 TX : PD5
 *    USART1 RX : PB7      USART2 RX : PD6
 *    USART3 TX : PD8 (debug)
 *    USART3 RX : PB11
 *    UART4  TX : PA12
 *    UART4  RX : PA11
 * ******************************************************************************
 */

/* Define    -----------------------------------------------------------*/
#define UART1_TXBUF_LEN     512
#define UART1_RXBUF_LEN     512

#define UART2_TXBUF_LEN     2048
#define UART2_RXBUF_LEN     4096

#define UART4_TXBUF_LEN     512
#define UART4_RXBUF_LEN     512

/* UART2 Test Mode (ASCII HEX conversion for TeraTerm testing) */
#define UART2_TEST_MODE_ASCII_HEX   1   // 1: Enable ASCII HEX to Binary conversion (for test)
                                        // 0: Binary only (for production)
/* ★ OTA(0xA2) 수신 중에는 위 테스트 변환을 런타임으로 강제 비활성화.
 *   이유: OTA_RECV_HEX 모드에서 0xA2 payload 가 ASCII HEX 라인이므로,
 *         UART read 경계가 라인 중간에 걸치고 CR/LF 가 섞이면
 *         is_ascii_hex_format() 이 오탐 → ascii_hex_to_binary() 가 데이터를 변환·손상시킴
 *         (bin_buf[256] 절단 위험 포함).
 *   구현: fw_update_is_receiving() == 1 (0xA1 START ~ 0xA3 END 구간) 이면 변환 우회. */
// #define DEBUG_RAW_UART2   0// UART2로 들어오는 원시 데이터 출력 (디버그용)

/* Includes  -----------------------------------------------------------*/
#include "../Inc/sys-common.h"
#include "../Inc/task-lcd.h"
#include "../Inc/git-comm.h"
#include "../Inc/git-protocol.h"
#include "../Inc/git-functionlist.h"
#include "../Inc/git-ble.h"
#include "../Inc/task-plc.h"
#include "../Inc/git-functionlist.h"
#include "../Inc/task-hwcontrol.h"
#include "../Inc/led-indicator.h"
#include "fw_update.h"      /* Common/Inc — fw_update_is_receiving() (OTA 수신 중 전처리 우회) */

/* Variables -----------------------------------------------------------*/
extern uint32_t g_Func_GDS_cnt;
extern stFunctionList g_Functions_GDS[];

// Thread Def
osThreadId_t commTaskHandle;
uint32_t commTaskBuffer[ 2048 ];
osStaticThreadDef_t commTaskControlBlock;
const osThreadAttr_t commTask_attributes = {
  .name = "commTask",
  .cb_mem = &commTaskControlBlock,
  .cb_size = sizeof(commTaskControlBlock),
  .stack_mem = &commTaskBuffer[0],
  .stack_size = sizeof(commTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};

// ============================================================================
// UART1 Variables (Polling TX + Interrupt RX)
// ============================================================================
static uint8_t g_uart1_rx_it_byte = 0;
static uint8_t g_uart1_rx_ring[UART1_RXBUF_LEN];
static volatile uint16_t g_uart1_rx_head = 0;
static volatile uint16_t g_uart1_rx_tail = 0;
static volatile uint8_t g_uart1_rx_it_active = 0;

// ============================================================================
// UART2 Variables (Polling TX + Interrupt RX)
// ============================================================================
// ring buffer
static uint8_t g_uart2_rx_it_byte = 0;
static uint8_t g_uart2_rx_ring[UART2_RXBUF_LEN];
static volatile uint16_t g_uart2_rx_head = 0;
static volatile uint16_t g_uart2_rx_tail = 0;

// flags
static volatile uint8_t g_uart2_rx_it_active = 0;  // Interrupt RX active flag
static volatile bool g_bt_connected_cached = false;

static void UART2_push_rx_bytes_from_isr(const uint8_t *src, uint16_t len);
static HAL_StatusTypeDef UART2_start_rx_interrupt(void);

static HAL_StatusTypeDef UART1_start_rx_interrupt(void);
static void UART1_push_rx_bytes_from_isr(const uint8_t *src, uint16_t len);
unsigned int UART2_write_buff_test_1(int cmd);

/* Fragmented Frame Handling (분할 수신 처리) */
static uint32_t g_last_partial_frame_time = 0;  // 마지막 바이트 도착 시간
static uint16_t g_last_partial_frame_size = 0;  // 마지막 확인된 available 바이트 수
static uint16_t g_last_partial_expected   = 0;  // 예상 프레임 크기
#define PARTIAL_FRAME_TIMEOUT_MS  200   // 200ms 타임아웃

/* Functions -----------------------------------------------------------*/

bool COMM_IsBTConnected(void)
{
    return g_bt_connected_cached;
}

void InitCommTask(void)
{
    extern void Debug_Printf_Mutex_Init(void);
    Debug_Printf_Mutex_Init();   /* printf 멀티태스크 보호용 뮤텍스 초기화 */

    BT_Initialize();

    // Initialize UART2 RX interrupt with hardware flow control
    HAL_StatusTypeDef status = UART2_start_rx_interrupt();
    // if (status == HAL_OK) {
    //     printf("[UART2] RX Interrupt started OK (115200 8N1 RTS/CTS)\r\n");
    // } else {
    //     printf("[UART2] *** RX Interrupt start FAILED, status=%d (0=OK,1=ERR,2=BUSY,3=TIMEOUT)\r\n",
    //            (int)status);
    //     printf("[UART2] huart2 RxState=0x%lx, ErrorCode=0x%lx\r\n",
    //            (unsigned long)huart2.RxState, (unsigned long)huart2.ErrorCode);
    // }

    // Create Comm Task
    commTaskHandle = osThreadNew(StartCommTask, NULL, &commTask_attributes);
}

void StartCommTask(void *argument)
{
    bool          prev_bt_connected = false;
    /* BLE 연결 동안의 mode 를 매 cycle 백업해두고, disconnect edge 처리 시 사용.
     * BTGetConnectStatus 가 호출 시점에 mode/state 를 NONE 으로 강제하므로,
     * 호출 후 g_system_mode 검사하면 항상 NONE 으로 보임 → popup 분기 진입 못함.
     * 백업값으로 disconnect 직전 모드를 안전하게 보존. */
    SystemMode_t  last_bt_mode      = eMODE_NONE;

    printf("start %s ... \r\n", __FUNCTION__);

    for(;;)
    {
        UART2_read_buff();

        /* BLE disconnect (connected→disconnected edge) → STOP  */
        bool cur_bt_connected = (bool)BTGetConnectStatus();
        g_bt_connected_cached = cur_bt_connected;
        if (prev_bt_connected && !cur_bt_connected) {
            bool was_running = (g_device_state == eDEVICE_STATE_START ||
                                g_device_state == eDEVICE_STATE_RUNNING);

            printf("[COMM] BLE disconnected: start stop sequence (last_mode=%d, was_running=%d)\r\n",
                   (int)last_bt_mode, (int)was_running);

            SENSOR_ErrorCheck_Disarm();   /* 정지 직후 센서 에러 체크 중단 */

            // new main screen
            LCD_PostShowHide(LCD_SCR_MAIN, LCD_MAIN_BT_CONN_CTRL_ID, LCD_OFF);  // BT Connection Indicator
            LCD_PostShowHide(LCD_SCR_MAIN, LCD_MAIN_BT_CONN_WAITING_TEXT, LCD_ON);   // BT Disconnect Message
            LCD_PostPopupComm(0);                           // COMM Popup HIDE
            LCD_PostVehicleInfoDefault();                   // 차량 정보 기본 문구("차량 연결 필요")로 복원

            /* 1. periodic data TX STOP*/
            g_bDisplayActive = false;

            /* 2. STOP seq — last_bt_mode 기준 분기 (mode 가 이미 NONE 됐을 수 있어 백업값 사용)
             *    · VEHICLE_DISCHARGE: SECC NormalStop 송신 + 200ms 대기
             *    · BSA_DISCHARGE   : BSA_Stop (FDCAN1 stop, g_bBatRelayConFlag=false)
             *    · 모든 모드      : IO_ALL_OFF_control — AC/DC + EXT_RLY 1~4 OFF
             *                       (FAN: SS1~SS4 는 그대로 ON 유지) */
            if (last_bt_mode == eMODE_VEHICLE_DISCHARGE && g_plcStepRunning) {
                PLC_SetTxValue(PLC_REQ_ChargeControl, PLC_MSGDisp_ChargingControl_NormalStop);
                printf("[COMM] BLE disconnected: NormalStop sent to SECC\r\n");
                osDelay(200);  /* TX 한 주기 나갈 시간 확보 */
            }
            else if (last_bt_mode == eMODE_BSA_DISCHARGE) {
                extern void BSA_Stop(void);
                BSA_Stop();
                printf("[COMM] BLE disconnected: BSA_Stop done\r\n");
            }

            if (last_bt_mode != eMODE_NONE) {
                IO_ALL_OFF_control();
                g_plcManualRun = false;
                if (g_plcStepRunning) {
                    g_plcStepRunning = false;
                }
                for (int rly = 1; rly <= 4; rly++) {
                    IO_EXT_RLY_control(rly, false);
                }
                printf("[COMM] BLE disconnected: IO_ALL_OFF + EXT_RLY 1~4 OFF done (FAN SS1~4 preserved)\r\n");
            }

            g_device_state = eDEVICE_STATE_NONE;
            printf("[COMM] BLE disconnected: stop sequence done\r\n");

            /* 3. STOP 완료 후 LCD 연결해제 팝업 1회 표출
             *    · was_running == true (start 이후 running 상태에서 disconnect) 인 경우에만 표출
             *    · STOP 명령으로 정지된 후 disconnect 면 popup 생략 (사용자가 이미 정지를 인지)
             *    · popup 객체는 scr=1(충전) / scr=2(방전) 에만 등록되어 있음
             *    · 사용자 확인 버튼 (LCD btn 0x1A) → LCD_OnCharge/DiscOk → popup hide + scr=MAIN */
            uint8_t disc_scr = 0xFF;  /* 0xFF = popup 미표출 */
            if (was_running) {
                if (last_bt_mode == eMODE_VEHICLE_CHARGE) {
                    disc_scr = LCD_SCR_CHARGE;
                } else if (last_bt_mode == eMODE_VEHICLE_DISCHARGE ||
                           last_bt_mode == eMODE_BSA_DISCHARGE) {
                    disc_scr = LCD_SCR_DISCHARGE;
                }
            }
            if (disc_scr != 0xFF) {
                LCD_PostScreenGoto(disc_scr);
                LCD_PostPopupDisconnect(disc_scr);
                printf("[COMM] BLE disconnected: LCD scr=%d goto + Disconnect popup\r\n",
                       disc_scr);
            } else {
                printf("[COMM] BLE disconnected: popup skip (was_running=%d, last_mode=%d)\r\n",
                       (int)was_running, (int)last_bt_mode);
            }

            /* 4. LED 표시 — 별도 호출 불필요. led-indicator.c 의 LED_Setting() 이
             *    COMM_IsBTConnected() 를 매 tick 직접 읽어 BLE 미연결 상태를
             *    조건 없이 즉시 LED_Y_ON 으로 반영한다(was_running 여부 무관). */

            /* 5. FAN cooldown arm — STOP path 와 동일하게 N분 후 SS1~SS4 자동 OFF
             *    · last_bt_mode != NONE (실제 동작 중이었음) 인 경우만 의미 있음 */
            if (last_bt_mode != eMODE_NONE) {
                FAN_CooldownArm();
                printf("[COMM] BLE disconnected: FAN cooldown armed\r\n");
            }

            /* 6. 연결 중 SetSerial(0xB1) 로 변경된 광고 이름을 지금(명령 모드) 반영.
             *    연결 중에는 UART2가 데이터 파이프라 AT+BTNAME이 모듈에 안 먹으므로
             *    여기(disconnect edge = 명령 모드)에서 AT+BTNAME 재전송 → 재광고 반영. */
            BTApplyPendingDeviceName();
        }

        /* BLE 연결 동안 매 cycle mode 백업 — disconnect edge 직전 값 보존 */
        if (cur_bt_connected && g_system_mode != eMODE_NONE) {
            last_bt_mode = g_system_mode;
        }

        /* BLE connect 감지 (disconnected→connected edge) → 에러코드 1회 TX */
        if (!prev_bt_connected && cur_bt_connected) {
            printf("[COMM] BLE connected\r\n");

            // new main screen
            LCD_PostShowHide(LCD_SCR_MAIN, LCD_MAIN_BT_CONN_CTRL_ID, LCD_ON);
            LCD_PostShowHide(LCD_SCR_MAIN, LCD_MAIN_BT_CONN_WAITING_TEXT, LCD_OFF);
            LCD_PostPopupComm(1);
            /* g_error_code != 0 이면 연결 직후 1회 0x81 프레임 송신 */
            FL_GDS_Send_Error_Code();
        }
        prev_bt_connected = cur_bt_connected;

        FL_GDS_Display_Data_Periodic(); // 0x51 -> active (periodic data tx)

        osDelay(10);

    }
}


unsigned int UART2_write_buff_test_1(int cmd)
{
    if (cmd == 0)
    {
        const char tx_msg[] = "AT\r";
        return UART2_Transmit_Polling((const uint8_t*)tx_msg, (uint16_t)strlen(tx_msg));
    }
    else if (cmd == 1)
    {
        const char tx_msg[] = "ATZ\r";
        return UART2_Transmit_Polling((const uint8_t*)tx_msg, (uint16_t)strlen(tx_msg));
    }
    else
    {
        const char tx_msg[] = "ATI\r";
        return UART2_Transmit_Polling((const uint8_t*)tx_msg, (uint16_t)strlen(tx_msg));

    }

}

// =============================================================================
// UART2 Functions (Polling TX + Interrupt RX)
// =============================================================================

/**
 * @brief  Start UART2 Interrupt RX (continuous 1-byte reception)
 * @retval HAL_StatusTypeDef
 * @note   Called at initialization, restarted in callback after RX complete
 */
static HAL_StatusTypeDef UART2_start_rx_interrupt(void)
{
    HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart2, (uint8_t*)&g_uart2_rx_it_byte, 1);

    if (status == HAL_OK) {
        g_uart2_rx_it_active = 1;
    } else {
        g_uart2_rx_it_active = 0;
    }

    return status;
}

/**
 * @brief  UART RX Complete Callback (full buffer received)
 * @param  huart: UART handle
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uint32_t err = huart->ErrorCode;
    (void)err;

    if (huart->Instance == USART1) {
        if (g_uart1_rx_it_active) {
            HAL_UART_Receive_IT(&huart1, (uint8_t*)&g_uart1_rx_it_byte, 1);
        }
    }
    else if (huart->Instance == USART2) {
        if (g_uart2_rx_it_active) {
            HAL_UART_Receive_IT(&huart2, (uint8_t*)&g_uart2_rx_it_byte, 1);
        }
    }
    else if (huart->Instance == UART8) {
        LCD_UART8_RxErrorCallback();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        // UART1 Interrupt mode: 1-byte RX complete
        if (g_uart1_rx_it_active) {
            // Store to ring buffer
            UART1_push_rx_bytes_from_isr(&g_uart1_rx_it_byte, 1);

            // Start next byte RX
            HAL_UART_Receive_IT(&huart1, (uint8_t*)&g_uart1_rx_it_byte, 1);
        }
    }
    else if (huart->Instance == USART2) {
        // UART2 Interrupt mode: 1-byte RX complete
        if (g_uart2_rx_it_active) {
            // Store to ring buffer
            UART2_push_rx_bytes_from_isr(&g_uart2_rx_it_byte, 1);

            // Start next byte RX
            HAL_UART_Receive_IT(&huart2, (uint8_t*)&g_uart2_rx_it_byte, 1);
        }
    }
    else if (huart->Instance == UART8) {
        LCD_UART8_RxCpltCallback();
    }
}

/**
 * @brief  Store received data to ring buffer from ISR
 * @param  src: Source data pointer
 * @param  len: Data length
 */
static void UART2_push_rx_bytes_from_isr(const uint8_t *src, uint16_t len)
{
    if (src == NULL || len == 0) {
        return;
    }
    UBaseType_t saved = taskENTER_CRITICAL_FROM_ISR();

    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (uint16_t)((g_uart2_rx_head + 1) % UART2_RXBUF_LEN);

        // If buffer full, move tail (overwrite old data)
        if (next == g_uart2_rx_tail) {
            g_uart2_rx_tail = (uint16_t)((g_uart2_rx_tail + 1) % UART2_RXBUF_LEN);
        }

        g_uart2_rx_ring[g_uart2_rx_head] = src[i];
        g_uart2_rx_head = next;
    }
    taskEXIT_CRITICAL_FROM_ISR(saved);
}

// =============================================================================
// UART1 Functions (Polling TX + Interrupt RX)
// =============================================================================

/**
 * @brief  Start UART1 Interrupt RX (continuous 1-byte reception)
 * @retval HAL_StatusTypeDef
 */
static HAL_StatusTypeDef UART1_start_rx_interrupt(void)
{
    HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart1, (uint8_t*)&g_uart1_rx_it_byte, 1);

    if (status == HAL_OK) {
        g_uart1_rx_it_active = 1;
    } else {
        g_uart1_rx_it_active = 0;
    }

    return status;
}

/**
 * @brief  Store UART1 received data to ring buffer from ISR
 * @param  src: Source data pointer
 * @param  len: Data length
 */
static void UART1_push_rx_bytes_from_isr(const uint8_t *src, uint16_t len)
{
    if (src == NULL || len == 0) {
        return;
    }
    // UBaseType_t saved = taskENTER_CRITICAL_FROM_ISR();
    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (uint16_t)((g_uart1_rx_head + 1) % UART1_RXBUF_LEN);

        // if buffer full -> tail move (overwrite old data)
        if (next == g_uart1_rx_tail) {
            g_uart1_rx_tail = (uint16_t)((g_uart1_rx_tail + 1) % UART1_RXBUF_LEN);
        }

        g_uart1_rx_ring[g_uart1_rx_head] = src[i];
        g_uart1_rx_head = next;
    }
    // taskEXIT_CRITICAL_FROM_ISR(saved);
}

/**
 * @brief  Read data from UART1 ring buffer
 * @param  buf: Buffer to store data
 * @param  maxlen: Maximum length to read
 * @retval Number of bytes read
 */
unsigned int UART1_read_from_ring(uint8_t *buf, unsigned int maxlen)
{
    if (buf == NULL || maxlen == 0) {
        return 0;
    }

    unsigned int copied = 0;
    taskENTER_CRITICAL();

    while ((copied < maxlen) && (g_uart1_rx_head != g_uart1_rx_tail)) {
        buf[copied++] = g_uart1_rx_ring[g_uart1_rx_tail];
        g_uart1_rx_tail = (uint16_t)((g_uart1_rx_tail + 1) % UART1_RXBUF_LEN);
    }

    taskEXIT_CRITICAL();
    return copied;
}

/**
 * @brief  UART1 Polling mode transmit
 * @param  pData: Pointer to data to transmit
 * @param  Size: Size of data to transmit
 * @retval Number of bytes transmitted (0 on failure)
 */
unsigned int UART1_Transmit_Polling(const uint8_t *pData, uint16_t Size)
{
    if (pData == NULL || Size == 0) {
        return 0;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (uint8_t*)pData, Size, 1000);

    if (status == HAL_OK) {
        return Size;
    }
    return 0;
}

/**
 * @brief  UART1 Polling mode receive (read from Interrupt-based ring buffer)
 * @param  pData: Buffer to store received data
 * @param  Size: Maximum size to receive
 * @retval Number of bytes received (0 if no data)
 * @note   Continuous RX via Interrupt in background, stored to ring buffer
 */
unsigned int UART1_Receive_Interrupt(uint8_t *pData, uint16_t Size)
{
    if (pData == NULL || Size == 0) {
        return 0;
    }

    // Start Interrupt RX if not already started
    if (!g_uart1_rx_it_active) {
        UART1_start_rx_interrupt();
    }

    // Read data from ring buffer
    unsigned int received = UART1_read_from_ring(pData, Size);
    return received;
}

/**
 * @brief  UART1 initialization function (start Interrupt RX)
 * @note   Call only once
 */
void UART1_Init_IT(void)
{
    // printf("[UART1] Initializing Interrupt RX...\r\n");

    // Initialize ring buffer
    taskENTER_CRITICAL();
    g_uart1_rx_head = 0;
    g_uart1_rx_tail = 0;
    g_uart1_rx_it_active = 0;
    taskEXIT_CRITICAL();

    memset(g_uart1_rx_ring, 0, sizeof(g_uart1_rx_ring));

    // Interrupt RX 시작
    HAL_StatusTypeDef status = UART1_start_rx_interrupt();

    // if (status == HAL_OK) {
    //     printf("[UART1] Interrupt RX started successfully\r\n");
    // } else {
    //     printf("[UART1] Interrupt RX start failed, status=%d\r\n", (int)status);
    // }
}

// =============================================================================
// UART2 Functions (Polling TX + Interrupt RX)
// =============================================================================

/**
 * @brief  Read data from ring buffer
 * @param  buf: Buffer to store data
 * @param  maxlen: Maximum length to read
 * @retval Number of bytes read
 */
unsigned int UART2_read_from_ring(uint8_t *buf, unsigned int maxlen)
{
    if (buf == NULL || maxlen == 0) {
        return 0;
    }

    unsigned int copied = 0;
    taskENTER_CRITICAL();

    while ((copied < maxlen) && (g_uart2_rx_head != g_uart2_rx_tail)) {
        buf[copied++] = g_uart2_rx_ring[g_uart2_rx_tail];
        g_uart2_rx_tail = (uint16_t)((g_uart2_rx_tail + 1) % UART2_RXBUF_LEN);
    }

    taskEXIT_CRITICAL();
    return copied;
}


// =============================================================================
// Polling Mode Functions (for continuous communication)
// =============================================================================

/**
 * @brief  UART2 Polling mode transmit
 * @param  pData: Pointer to data to transmit
 * @param  Size: Size of data to transmit
 * @retval Number of bytes transmitted (0 on failure)
 */
unsigned int UART2_Transmit_Polling(const uint8_t *pData, uint16_t Size)
{
    if (pData == NULL || Size == 0) {
        return 0;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart2, (uint8_t*)pData, Size, 1000);

    if (status == HAL_OK) {
        return Size;
    }
    return 0;
}

/**
 * @brief  UART2 Polling mode receive (read from Interrupt-based ring buffer)
 * @param  pData: Buffer to store received data
 * @param  Size: Maximum size to receive
 * @retval Number of bytes received (0 if no data)
 * @note   Continuous RX via Interrupt in background, stored to ring buffer
 */
unsigned int UART2_Receive_Interrupt(uint8_t *pData, uint16_t Size)
{
    if (pData == NULL || Size == 0) {
        return 0;
    }

    // Start Interrupt RX if not already started
    if (!g_uart2_rx_it_active) {
        UART2_start_rx_interrupt();
    }

    // Read data from ring buffer
    unsigned int received = UART2_read_from_ring(pData, Size);
    return received;
}
/**
 * @brief  Helper: Convert ASCII HEX character to integer
 * @param  c: ASCII character ('0'-'9', 'A'-'F', 'a'-'f')
 * @retval Integer value (0-15), or -1 on error
 */
static int hex_char_to_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/**
 * @brief  Helper: Convert ASCII HEX string to binary (for TeraTerm test)
 * @param  hex_str: Input HEX string (e.g., "A0 08 10" or "A00810")
 * @param  hex_len: Length of HEX string
 * @param  bin_out: Output binary buffer
 * @param  bin_max: Maximum size of output buffer
 * @retval Number of binary bytes converted
 * @note   Supports space-separated and continuous HEX strings
 */
static int ascii_hex_to_binary(const uint8_t *hex_str, uint16_t hex_len,
                                uint8_t *bin_out, uint16_t bin_max)
{
    if (hex_str == NULL || bin_out == NULL) {
        return 0;
    }

    uint16_t bin_idx = 0;
    int high_nibble = -1;

    for (uint16_t i = 0; i < hex_len && bin_idx < bin_max; i++) {
        char c = hex_str[i];

        // Skip whitespace and control characters
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }

        int val = hex_char_to_val(c);
        if (val < 0) {
            continue;  // Invalid character, skip
        }

        if (high_nibble < 0) {
            high_nibble = val;  // First nibble (high 4 bits)
        } else {
            bin_out[bin_idx++] = (uint8_t)((high_nibble << 4) | val);
            high_nibble = -1;
        }
    }

    return bin_idx;
}

/**
 * @brief  Check if data is ASCII HEX format (for auto-detection)
 * @param  data: Input data
 * @param  len: Data length
 * @retval 1 if ASCII HEX format, 0 otherwise
 * @note   Checks if data starts with valid HEX chars and spaces
 */
static int is_ascii_hex_format(const uint8_t *data, uint16_t len)
{
    if (data == NULL || len < 2) {
        return 0;
    }

    // Check first few bytes for HEX pattern (0-9, A-F, a-f, space)
    int hex_count = 0;
    int space_count = 0;

    for (uint16_t i = 0; i < len && i < 10; i++) {
        char c = data[i];
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
            hex_count++;
        } else if (c == ' ' || c == '\r' || c == '\n') {
            space_count++;
        } else {
            return 0;  // Invalid character for HEX format
        }
    }

    // ASCII HEX format requires BOTH hex chars AND spaces (e.g. "A0 08 10")
    // Binary data may contain 0x30-0x39('0'-'9'), 0x41-0x46('A'-'F') but has NO spaces
    return (hex_count >= 2 && space_count > 0);
}

unsigned int UART2_read_buff()
{
    // UART2 RX get + BT parsing
    uint8_t rx_tmp[512];
    unsigned int len = UART2_Receive_Interrupt(rx_tmp, sizeof(rx_tmp));

    if (len > 0) {
#ifdef DEBUG_RAW_UART2
                printf("  Input : ");
                for (uint16_t i = 0; i < len ; i++) {
                    printf("%02X ", rx_tmp[i]);
                }
                printf("\r\n");
#endif
#if (UART2_TEST_MODE_ASCII_HEX == 1)
        // TeraTerm ASCII HEX 입력 자동 감지 및 변환 (테스트 모드)
        // 예: "A0 08 10" → 0xA0 0x08 0x10
        // ★ OTA 수신 중(0xA1 START ~ 0xA3 END)에는 변환을 우회 — 0xA2 의 ASCII HEX
        //    payload 를 오탐 변환하여 이미지를 손상시키는 것을 방지.
        if (fw_update_is_receiving()) {
            BLE_UART2_PushRx(rx_tmp, (uint16_t)len);   // 원본 그대로 (변환 금지)
        }
        else if (is_ascii_hex_format(rx_tmp, len)) {
            uint8_t bin_buf[256];
            int bin_len = ascii_hex_to_binary(rx_tmp, len, bin_buf, sizeof(bin_buf));

            if (bin_len > 0) {
                // printf("[UART2 TEST] ASCII HEX detected, converted %u bytes to %d binary bytes\r\n", len, bin_len);
#ifdef DEBUG_RAW_UART2
                printf("  Input : ");
                for (uint16_t i = 0; i < len ; i++) {
                    printf("%02X ", rx_tmp[i]);
                }
                printf("\r\n  Binary: ");
                for (int i = 0; i < bin_len ; i++) {
                    printf("%02X ", bin_buf[i]);
                }
                printf("\r\n");
#endif
                // Circular buffer에 변환된 바이너리 데이터 저장
                BLE_UART2_PushRx(bin_buf, (uint16_t)bin_len);
            } else {
                // 변환 실패시 원본 데이터 저장
                BLE_UART2_PushRx(rx_tmp, (uint16_t)len);
            }
        } else {
            // Binary 데이터로 판단 - 그대로 저장
            BLE_UART2_PushRx(rx_tmp, (uint16_t)len);
        }
#else
        // 운영 모드: Binary 데이터만 처리 (ASCII HEX 변환 비활성화)
        BLE_UART2_PushRx(rx_tmp, (uint16_t)len);
#endif
    }

    // BT Connect Status Check => parse
    if (BTGetConnectStatus()) {
        // BT Connected: git-protocol parsing (bypass mode)
        // git-protocol frame parse (SOF, EOF, Function ID, Payload extract)
        UART2_Process_GitProtocol();
    } else {
        // BT Disconnected: BT Command Response parsing
        // \r 기준으로 한 줄씩 잘라서 READY, OK 등의 응답 처리
        BLE_ProcessRxQueue();
    }

    return len;
}

/**
 * @brief  UART2에서 git-protocol frame을 parsing
 * @note   BT Connected 상태에서 호출됨. 한 번 호출에 큐의 모든 완성된 프레임 처리.
 */
void UART2_Process_GitProtocol(void)
{
    /* 프로토콜 최대 프레임(519B = SOF1+Len2+FuncID1+Payload512+CRC2+EOF1) 수용.
     * 512로 두면 payload 512B 프레임에서 아래 복사 루프가 7바이트 오버런함. */
    uint8_t rx_buf[GITPACKET_FRAME_SIZE_MAX];

    // Circular buffer에서 데이터 확인 (읽지 않고 peek)
    extern uint8_t g_ble_rx_q[];
    extern uint16_t g_ble_rx_tail;
    extern uint16_t g_ble_rx_head;

    // 큐에 남은 모든 완성된 프레임을 처리
    while (1) {
        uint16_t available = BLE_RxCount();

        if (available == 0) {
            return;  // No received data
        }

        // SOF(0xA0) 찾기
        uint16_t sof_pos = 0;
        bool sof_found = false;
        for (uint16_t i = 0; i < available && i < sizeof(rx_buf); i++) {
            uint16_t idx = (g_ble_rx_tail + i) % BLE_RX_QUEUE_SIZE;
            if (g_ble_rx_q[idx] == GITPACKET_SOF) {
                sof_pos = i;
                sof_found = true;
                break;
            }
        }

        // SOF를 찾지 못한 경우 - 유효하지 않은 데이터
        if (!sof_found) {
    #if 0 //DEBUG_RAW_UART2
            uint16_t read_len = (available < sizeof(rx_buf)) ? available : sizeof(rx_buf);
            printf("[BT Connected] No valid frame (no SOF), Raw data (%u bytes): ", read_len);
            for (uint16_t i = 0; i < read_len; i++) {
                uint16_t idx = (g_ble_rx_tail + i) % BLE_RX_QUEUE_SIZE;
                printf("%02X ", g_ble_rx_q[idx]);
            }
            printf("\r\n");
    #endif
            // 잘못된 데이터 버림 (tail 이동)
            g_ble_rx_tail = g_ble_rx_head;
            return;
        }

        // SOF 이전의 잘못된 데이터가 있으면 버림
        if (sof_pos > 0) {
            printf("[BT Connected] Discarding %u bytes before SOF\r\n", sof_pos);
            for (uint16_t i = 0; i < sof_pos; i++) {
                g_ble_rx_tail = (g_ble_rx_tail + 1) % BLE_RX_QUEUE_SIZE;
            }
            available -= sof_pos;
        }

        // 최소 프레임 크기 확인 (SOF + Len(2) + FuncID + CRC16(2) + EOF = 7 bytes)
        if (available < GITPACKET_FRAME_SIZE_MIN) {
            return;  // 아직 전체 프레임이 도착하지 않음
        }

        // Len 값 읽기 (SOF 다음, 2 bytes Little Endian)
        uint16_t len_idx_lo = (g_ble_rx_tail + 1) % BLE_RX_QUEUE_SIZE;
        uint16_t len_idx_hi = (g_ble_rx_tail + 2) % BLE_RX_QUEUE_SIZE;
        uint16_t len = g_ble_rx_q[len_idx_lo] | (g_ble_rx_q[len_idx_hi] << 8);

        // 예상 프레임 크기: SOF(1) + Len(2) + Len내용
        uint16_t expected_frame_size = 3 + len;

        // 프레임 크기 유효성 검사
        if (expected_frame_size < GITPACKET_FRAME_SIZE_MIN || expected_frame_size > GITPACKET_FRAME_SIZE_MAX) {
            printf("[BT Connected] Invalid frame length: %u (expected %u-%u)\r\n",
                   expected_frame_size, GITPACKET_FRAME_SIZE_MIN, GITPACKET_FRAME_SIZE_MAX);
            // SOF를 버리고 다음 데이터부터 다시 시도
            g_ble_rx_tail = (g_ble_rx_tail + 1) % BLE_RX_QUEUE_SIZE;
            continue;  // 다음 SOF 재탐색
        }

        // 전체 프레임이 도착했는지 확인 (분할 수신 처리)
        if (available < expected_frame_size) {
            // 불완전한 프레임 - 다음 호출 대기
            uint32_t current_time = HAL_GetTick();

            if (g_last_partial_expected != expected_frame_size) {
                // 새로운 프레임 수신 시작
                g_last_partial_frame_time = current_time;
                g_last_partial_frame_size = available;
                g_last_partial_expected   = expected_frame_size;
                printf("[BT Connected] Partial frame detected: %u/%u bytes (waiting for %u more bytes)\r\n",
                       available, expected_frame_size, expected_frame_size - available);
            } else if (available != g_last_partial_frame_size) {
                // 새 바이트 도착 → 타이머 리셋
                g_last_partial_frame_time = current_time;
                g_last_partial_frame_size = available;
            } else {
                // 바이트 수 변동 없음 → 타임아웃 체크
                if ((current_time - g_last_partial_frame_time) > PARTIAL_FRAME_TIMEOUT_MS) {
                    printf("[BT Connected] Partial frame timeout (%u ms), discarding %u bytes\r\n",
                           PARTIAL_FRAME_TIMEOUT_MS, available);
                    for (uint16_t i = 0; i < available; i++) {
                        g_ble_rx_tail = (g_ble_rx_tail + 1) % BLE_RX_QUEUE_SIZE;
                    }
                    g_last_partial_frame_size = 0;
                    g_last_partial_frame_time = 0;
                    g_last_partial_expected   = 0;
                }
            }
            return;  // 아직 프레임이 완전히 도착하지 않음
        }

        // 완전한 프레임이 도착함 - 분할 수신 상태 초기화
        if (g_last_partial_expected > 0) {
    #ifdef DEBUG_RAW_UART2
            printf("[BT Connected] Complete frame received after fragmentation (%u bytes)\r\n",
                   expected_frame_size);
    #endif
            g_last_partial_frame_size = 0;
            g_last_partial_frame_time = 0;
            g_last_partial_expected   = 0;
        }

        // 프레임 데이터를 버퍼로 복사
        for (uint16_t i = 0; i < expected_frame_size; i++) {
            uint16_t idx = (g_ble_rx_tail + i) % BLE_RX_QUEUE_SIZE;
            rx_buf[i] = g_ble_rx_q[idx];
        }

        // git-protocol parsing 함수 호출
        uint8_t func_id = 0;
        uint8_t *payload = NULL;
        uint16_t payload_len = 0;

        int parse_result = GITPACKET_parse_frame(rx_buf, expected_frame_size,
                                                 &func_id, &payload, &payload_len);

        if (parse_result == 0) {
    #ifdef DEBUG_RAW_UART2
            printf("[BT Connected] Valid Frame: FuncID=0x%02X, PayloadLen=%u\r\n",
                   func_id, payload_len);
    #endif
            // Function ID에 따른 처리 (git-functionlist.c 테이블 참조)
            for (uint32_t i = 0; i < g_Func_GDS_cnt; i++) {
                if (g_Functions_GDS[i].uiFunctionID == func_id && g_Functions_GDS[i].fnPayloadCB != NULL) {
                    g_Functions_GDS[i].fnPayloadCB(payload, (uint32_t)0 /*commType*/, payload_len);
                    break;
                }
            }

            // 처리 완료된 프레임 제거
            for (uint16_t i = 0; i < expected_frame_size; i++) {
                g_ble_rx_tail = (g_ble_rx_tail + 1) % BLE_RX_QUEUE_SIZE;
            }
            // continue → 큐에 남은 다음 프레임 처리
        } else {
            // 파싱 실패 - Raw 데이터 출력
            printf("[BT Connected] Invalid frame, Raw data (%u bytes): ", expected_frame_size);
            for (uint16_t i = 0; i < expected_frame_size; i++) {
                printf("%02X ", rx_buf[i]);
            }
            printf("\r\n");

            // 손상된 프레임 전체를 버리고 다음 SOF부터 다시 시도
            for (uint16_t i = 0; i < expected_frame_size; i++) {
                g_ble_rx_tail = (g_ble_rx_tail + 1) % BLE_RX_QUEUE_SIZE;
            }
            g_last_partial_frame_size = 0;
            g_last_partial_frame_time = 0;
            g_last_partial_expected   = 0;
            // continue → 큐에 남은 다음 프레임 시도
        }
    } // while(1)
}


#if 0
// =============================================================================
// Test Functions (for testing)
// =============================================================================
/**
 * @brief  UART1 transmit test
 */
unsigned int UART1_write_buff_test(void)
{
    const char tx_msg[] = "UART1 Polling TX Test\r\n";
    return UART1_Transmit_Polling((const uint8_t*)tx_msg, (uint16_t)strlen(tx_msg));
}

/**
 * @brief  UART1 receive test
 */
unsigned int UART1_read_buff_test(void)
{
    uint8_t rx_buf[512];
    unsigned int len_rd = UART1_Receive_Interrupt(rx_buf, sizeof(rx_buf) - 1);

    if (len_rd > 0) {
        rx_buf[len_rd] = '\0';  // Null termination
        printf("UART1 IT RX (%u): %s\r\n", len_rd, rx_buf);
    }

    return len_rd;
}

/**
 * @brief  UART2 transmit test
 */
unsigned int UART2_write_buff_test()
{
    const char tx_msg[] = "UART2 Polling TX Test\r\n";
    return UART2_Transmit_Polling((const uint8_t*)tx_msg, (uint16_t)strlen(tx_msg));
}
#endif
