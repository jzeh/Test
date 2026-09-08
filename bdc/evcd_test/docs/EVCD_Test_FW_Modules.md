# EVCD (BDC) 펌웨어 — 소스 파일별 모듈 명세

> STM32H7S3I8 · FreeRTOS(CMSIS-OS2) · IAR EWARM 9.50.2 · 2-stage(Boot/Appli) XIP 구조
> 각 소스 파일을 모듈 단위로 나눠 **역할 / 특성 / 정의항목 / 상관관계**를 정리한다.

---

## 0. 모듈 지도 (계층 · 역할 요약)

| 계층 | 파일 | 한줄 역할 |
|---|---|---|
| **기반 타입** | `typedef.h` | 전역 기본 타입·통신/진단 enum·패킷 구조체 |
| **중앙 허브** | `sys-common.h` | 시스템 enum·전역·HAL 핸들·에러코드·태스크 init 프로토타입 |
| **시스템 제어** | `sys-main.c` | 상태머신(state_machine)·모드 컨트롤·운용타이머·커넥터 감시 |
| **부트/커널** | `Core/main.c`, `freertos.c` | 주변장치 init·HAL 핸들 실체·커널 시작·RTOS 훅 |
| **통신-드라이버** | `git-comm.c/.h` | UART1/2 링버퍼 드라이버 + BLE 라우팅 태스크(commTask) |
| **통신-코덱** | `git-protocol.c/.h` | GIT 패킷 프레임 make/parse/response + CRC16 |
| **통신-BLE** | `git-ble.c/.h` | BN-COM 모듈 AT 제어·응답 파싱·연결상태(GPIO) |
| **통신-디스패치** | `git-functionlist.c/.h` | FuncID→핸들러 테이블 + GDS 명령 구현 |
| **통신-OTA** | `git-functionlist-fwupdate.c` | FW 업데이트/시리얼 명령(0xA1~0xB2) 핸들러 |
| **제어-HW** | `task-hwcontrol.c/.h` | 릴레이·CP PWM(TIM4)·CP ADC·CP 상태머신·팬 |
| **제어-센서** | `task-sensor.c` | MCP3424 18bit ADC(V/C/온도)·과전압/과온 판정 |
| **제어-BSA** | `task-bsa.c/.h` | 배터리팩 CAN 시뮬(simframe TX / monitor RX) |
| **제어-CAN** | `task-can.c/.h` | FDCAN1/2 HW·TX/RX·ISR 디스패처·BSA I/F |
| **제어-PLC** | `task-plc.c/.h` | FDCAN2 SECC 급속충전 통신·스텝 시퀀스 엔진 |
| **제어-AIM** | `task-aim.c/.h` | RS-485 Modbus 절연저항계 폴링 |
| **UI-LCD** | `task-lcd.c/.h` | UART8 터치 LCD 이벤트 큐·버튼 파싱 |
| **UI-CLI** | `task-cli.c/.h`, `cli-bsa.c`, `cli-plc.c` | UART7 디버그 콘솔·도메인 명령 |
| **저장** | `sys-emmc.c/.h` | eMMC+FatFs 백엔드·관리·USB MSC·emmcTask·시리얼 |
| **공유-FW업데이트** | `Common/fw_update`, `fw_slot_manager`, `fw_flash_xspi`, `fw_crc32`, `fw_backup_sram`, `firmware_lite`, `fw_compat` | A/B 슬롯 OTA·메타데이터·XSPI·CRC32·BKPSRAM |
| **부트로더** | `Boot/main.c`, `boot_emmc.c` | fw_flag 분기·eMMC→Slot 복사·XIP 점프 |

---

## 1. 기반 · 공통 계층

### 1.1 `Appli/Function/Inc/typedef.h`
- **역할** — 프로젝트 최하위 공용 타입 헤더(기본 타입 별칭·통신/진단 enum·패킷 구조체).
- **특성**
  - HW/RTOS 비의존, `<stdbool.h>`만 include — 가장 기초 계층.
  - 통신(USB/UART/CAN/BT/WiFi)·진단(CAN/KWP/ETH/PassThru) 식별자 정의.
- **정의항목**
  - typedef: `BOOL`, `U8/U16/U32`(+`VU*`, 소문자·부호형), 함수포인터 `UserIntCallBack_t`/`pFunction`.
  - #define: `TRUE/FALSE`, `INIT_OK(0)/INIT_FAIL(-1)`, `MESSAGE_*_QUEUE_SIZE`, FAT 파일명 상수, `MAX_PASSTHRUMSG_DATA_SIZE(4128)`.
  - enum: `eCOMM_IF`, `eDIAG_IF`, `ePKT_TD`, `eMSG_TD`, `eDIAG_COMM_STATE`, `eMain_State`.
  - struct: `stCommPkt`(mData[4200]), `stMsgClst`/`MsgClst_t`, `MsgDiag_t`, `stPASSTHRU_MSG`/`PTmsgPkt_t`.
- **상관관계** — 의존 없음(표준만). **사용처**: `sys-common.h`가 include → 사실상 전 모듈이 간접 사용.

### 1.2 `Appli/Function/Inc/sys-common.h`
- **역할** — 애플리케이션 중앙 공용 헤더(상태/모드 enum·전역 extern·HAL 핸들·에러코드·태스크 init 프로토타입).
- **특성**
  - HAL·FreeRTOS·CMSIS-OS2·FatFs 및 user 헤더를 묶는 허브.
  - 보드 주변장치 HAL 핸들 전부 extern(UART1/2/4/7/8, FDCAN1/2, I2C1/2, TIM4, ADC1).
  - GPIO 제어 매크로(`IO_CONTROL_HIGH/LOW/TOGGLE/GET`), 이벤트 플래그 비트 정의.
- **정의항목**
  - enum: `SystemState_t`(NONE/INIT/IDLE/RUNNING/ERROR), `SystemMode_t`(NONE/VEHICLE_CHARGE/VEHICLE_DISCHARGE/BSA_DISCHARGE/SETTING), `DeviceState_t`, `DischargeOhm_t`.
  - struct: `stDeviceTime`(BLE 0x91 시각), `stDB_CONFIG`(차량·충방전 설정 DB, `DB_VIN_LENGTH 17`).
  - #define: `EVT_TASK_SENSOR/HWCON/BSA/PLC`(bit 0~3), `ERROR_CODE_*`(FW→APP), `ERROR_CODE_GDS_*`(APP→FW).
  - extern 전역: `g_system_state/mode`, `g_device_state/time`, 센서값(`g_discharge_current_A`/`_voltage_V`/`g_ac_charge_current_mA`/`g_temp_value[3]`/`g_power_W`), `g_dbConfig`, `g_taskEventFlags`, `g_bsaCanRxLastTick`, `g_run_time_sec/_active`, CAN 큐 3종, HAL 핸들.
  - 프로토타입: `Init*Task`(전 태스크), `SYS_init/state_machine/mode_control`, `SENSOR_ErrorCheck_Arm/Disarm`, `FAN_CooldownArm/Process`, 타이머 헬퍼.
- **상관관계** — **의존**: `typedef.h`, HAL, RTOS, `git-protocol.h`, `git-ble.h`, `sys-emmc.h`. **사용처**: 거의 전 소스가 include.

### 1.3 `Appli/Function/Src/sys-main.c`
- **역할** — 시스템 상태머신·모드 컨트롤러(NONE→INIT→IDLE→RUNNING→ERROR 전이, 모드별 태스크 트리거, 운용타이머·BSA 전원커넥터 감시·SS6 LED cooldown).
- **특성**
  - `state_machine()`은 main.c defaultTask 루프에서 10ms 폴링. IDLE→RUNNING 시 모드에 따라 `osEventFlagsSet`로 SENSOR/HWCON + (BSA→BSA / VEHICLE_DISCHARGE→PLC) 트리거.
  - fw_update Step7 자가검증(BOOT_TEST→VERIFIED), rollback 시 ini swap.
  - device START edge에서 운용타이머 리셋·LCD "00:00:00", BSA 방전 전압0 지속 감시→커넥터 fail(0x81).
  - 타이머 헬퍼(`Get_Tmr`/`Get_TmrDelta`/`GetUnixTime`) 실제 구현 위치.
- **정의항목** — 전역 정의(선언은 sys-common.h): `g_system_state/mode`, `g_taskEventFlags`, `g_device_state/time`, `g_run_time_sec/_active`. 함수: `Format_RunTime_HMS`, `SYS_init`, `state_machine`, `mode_control`, 타이머 헬퍼. 지역 #define: `BSA_PWR_CHECK_*`.
- **상관관계** — **호출(→)**: `CAN_enable`, `FL_GDS_Send_Error_Code`, `LCD_PostTextSet`, `IO_SS_control`/`FAN_Cooldown*`, `fw_get/set_flag`·fail_count·`fw_update_swap_ini_from_backup`, `g_discharge_voltage_V`. **피호출(←)**: main.c(`SYS_init`/`state_machine`), task-cli.c(`runtime`/`sys` 명령).

### 1.4 `Appli/Core/Src/main.c`
- **역할** — CubeMX 메인: 클럭/캐시/주변장치 init, HAL 핸들 실체 정의, printf 리다이렉트, 커널 시작, defaultTask 상태머신 루프.
- **특성**
  - 주변장치 init: ADC1(CP), FDCAN1/2(FD_BRS), I2C1/2, TIM4(CP PWM), UART1/2/4/7/8, FLASH OB, SDMMC1(8bit), GPIO.
  - `StartDefaultTask` 태스크 기동 순서: Comm→Lcd→SYS_init→PLC→BSA→Sensor→HwCon→CLI→USB_DEVICE, 이후 `state_machine()`+USB 폴링+alive LED 10ms 루프.
  - printf → UART7 리다이렉트(IAR `__write`, 스케줄러 실행 중 mutex 보호). `BackupSRAM_Init`으로 fw_flag 접근. CAN 큐 3종 생성.
- **정의항목** — HAL 핸들 실체(`hadc1`, `hfdcan1/2`, `hi2c1/2`, `hmmc1`, `htim4`, `huart1/2/4/7/8`), `defaultTaskHandle`, CAN 큐 핸들, `g_debug_uart_mutex`. #define `CAN_CMD_QUEUE_LEN(32)`/`CAN_RX_QUEUE_LEN(64)`. 함수: `main`, `MX_*_Init`, `Debug_Printf_Mutex_Init`, `__write`, `StartDefaultTask`, `Error_Handler`.
- **상관관계** — **호출(→)**: `SYS_init`/`state_machine`, `Init*Task`, `StartBSAThread`, `MX_USB_DEVICE_Init`, `BackupSRAM_Init`. **사용처(←)**: 전 태스크가 HAL 핸들·CAN 큐 extern 참조(sys-common.h 경유).

### 1.5 `Appli/Core/Src/freertos.c`
- **역할** — FreeRTOS 응용 훅(idle·스택 오버플로).
- **특성** — `vApplicationStackOverflowHook`이 printf 손상 회피 위해 UART7로 직접 태스크명 출력 후 무한루프. 디버그 전역 `g_dbg_last_task`.
- **정의항목** — 함수 `vApplicationIdleHook`, `vApplicationStackOverflowHook`. 전역 `g_dbg_last_task`.
- **상관관계** — **의존**: FreeRTOS, `huart7`. **사용처**: 커널이 훅 콜백 호출.

---

## 2. 통신 · 프로토콜 계층

### 2.1 `git-comm.c / git-comm.h`
- **역할** — UART1/2 저수준 드라이버(IT RX 링버퍼, 폴링 TX) + BLE 통신 관리 태스크(commTask). BT 연결 상태에 따라 GIT 프로토콜 파싱 또는 AT 응답 파싱으로 분기.
- **특성**
  - USART1(PA9/PB7), USART2(PD5/PD6, BLE), UART8(LCD 콜백 위임). RX=1B IT→링버퍼, TX=폴링.
  - 태스크 `StartCommTask` 10ms 폴링, 스택 2048, ISR `HAL_UART_RxCpltCallback`. 버퍼 UART2 TX2048/RX4096, 링버퍼 크리티컬섹션 보호(full=overwrite).
  - `UART2_Process_GitProtocol()`: SOF(0xA0) 탐색→Len(2B LE)→프레임 조립(분할 타임아웃 200ms)→`GITPACKET_parse_frame`→`g_Functions_GDS[]` 디스패치. 연결/해제 edge에서 정지 시퀀스·팝업. ASCII-HEX 테스트 모드.
- **정의항목**
  - #define: `UART2_TXBUF_LEN=2048`/`RXBUF_LEN=4096`, `PARTIAL_FRAME_TIMEOUT_MS=200`, `UART2_TEST_MODE_ASCII_HEX=1`.
  - 전역: `commTaskHandle`, 링버퍼 `g_uart1/2_rx_ring`+head/tail, `g_bt_connected_cached`, 분할프레임 상태.
  - 프로토타입: `InitCommTask`/`StartCommTask`/`COMM_IsBTConnected`; UART1/2 `*_Transmit_Polling`/`*_Receive_Interrupt`/`UART2_Process_GitProtocol`/`UART2_read_buff`.
- **상관관계** — **호출(→)**: git-ble(`BT_Initialize`/`BTGetConnectStatus`/`BLE_UART2_PushRx`/`BLE_ProcessRxQueue`), git-protocol(`GITPACKET_parse_frame`), git-functionlist(`g_Functions_GDS[]`/`FL_GDS_*`), task-lcd(`LCD_Post*`), task-plc/hwcontrol. **피호출(←)**: main/sys가 기동, git-ble·git-protocol가 `UART2_Transmit_Polling` TX 사용, HAL ISR이 콜백 호출.

### 2.2 `git-protocol.c / git-protocol.h`
- **역할** — GIT 패킷 프레임 make/parse/response(ACK·NAK) + UART2 전송 코덱.
- **특성**
  - 프레임 `SOF(0xA0)|Len(2B LE)|FuncID|Payload(0~512)|CRC16(2B LE)|EOF(0xB0)`, Len=n+4.
  - CRC-16/IBM(poly 0xA001 reflected, init 0x0000), 범위 SOF~Payload.
  - 큐 `hParsingMsg`/`hTransmitMsg`(CMSIS), `GITPACKET_transmit`이 큐→`HAL_UART_Transmit(huart2)`.
- **정의항목**
  - #define: `MAX_INTER_PROTO_DATA_LENGTH=4200`, `GITPACKET_FRAME_SIZE_MIN=7`/`MAX=519`, 풀 크기(`CAN_PACKET_POOL_SIZE=300` 등), `FDCAN_PACKET_MAX_SIZE=64`.
  - struct: `stTxMessage{pFrameData; frameSize;}`. 전역: `g_TxFrameBuffer/RxFrameBuffer`, `g_arrOutputGITPtclBuff[4200]`, 큐 핸들.
  - 프로토타입: `GITPACKET_make_frame`/`parse_frame`/`make_response`/`send_response`/`send_frame_via_uart(_hex)`/`transmit`/`InitGitProtocolTasks`.
- **상관관계** — **의존**: `git-functionlist.h`(SOF/EOF/ACK/IDX 매크로), `huart2`. **피호출(←)**: git-comm(parse), git-functionlist(+fwupdate)(make/send_response).

### 2.3 `git-ble.c / git-ble.h`
- **역할** — BN-COM BLE 모듈 제어(AT 명령 송신·테이블 기반 응답 파싱·GPIO 연결상태·광고명 설정·RX 라인 큐).
- **특성**
  - UART2 폴링 TX로 AT 전송, 연결상태 GPIO `BLE_STATUS` 폴링. 별도 원형 큐 `g_ble_rx_q[2048]`, `\r` 라인 파싱(max 256B).
  - `g_tbBTCmdList[]`(요청/응답/콜백) 테이블 매칭 디스패치. 상태머신 `eBleutoothState`. 광고명 `"BDC"+DeviceSerial`.
- **정의항목**
  - #define: `BLE_RX_QUEUE_SIZE=2048`, `BLE_RX_LINE_MAX=256`, `BNCOM`, `BLE_PREFIX_DEVICE_NAME="BDC"`, 응답문자열 `BLE_NOTI_READY`/`RES_OK`/`RES_ERROR`/`ANY_DATA`.
  - enum: `eBTCmdIndex`(SetSWReset/GetVersion/Get·SetBLE_Name/…), `eBleutoothState`(NONE→…→Connected).
  - struct: `stQueue`, `BT_ADDR`, `stBTInfo`, `tbBTCmdList`. 전역: `g_ble_rx_q`+head/tail, `g_LocalBTInfo`, `g_tbBTCmdList[]`.
  - 프로토타입: `BLE_UART2_PushRx`/`BLE_ProcessRxQueue`/`BLE_RxCount`, `BT_Initialize`/`BTGetConnectStatus`/`BTReset`, `BTSetLocalDeviceNameReq`/`BTStartAdvertisingReq`, 콜백 `BtEvt*`.
- **상관관계** — **의존**: git-comm(`UART2_Transmit_Polling`), sys-emmc(`DeviceSerial_Get`), `IO_ALL_OFF_control`. **피호출(←)**: git-comm(초기화/연결/RX), git-functionlist-fwupdate(0xB1 후 이름 갱신).

### 2.4 `git-functionlist.c / git-functionlist.h`
- **역할** — GDS BLE 프로토콜 FuncID→핸들러 디스패치 테이블 + 명령 핸들러(모드/데이터/BSA/PLC config, 주기 상태 송신, Start/Stop, 커넥터 상태, 에러).
- **특성**
  - 테이블 `g_Functions_GDS[]`(FuncID, 콜백 `void fn(void*, uint32_t, uint32_t)`). 핸들러 패턴: 길이검증→BE 파싱→전역 저장→`GITPACKET_send_response`.
  - `FL_GDS_Display_Data_Periodic`(0x51) 1초 간격 전류/전압/전력/온도 송신(`g_bDisplayActive` gate). `BDC_StopSequence` 모드별 정지 공통화.
- **정의항목**
  - 프레임 매크로(.h): `GITPACKET_SOF/EOF/ACK/NAK`, `PAYLOAD_LEN_MAX=512`, 인덱스 `*_IDX`.
  - typedef(.h): `pfnCommandLoadCB`, `stFunctionList`. enum(.c): `eBDC_StopPopupMode`.
  - 전역: `g_Functions_GDS[]`/`g_Func_GDS_cnt`, BSA config 배열, `g_dbConfig`, `g_bDisplayActive`, `g_vci3_soc`, `g_error_code`, SS LED sticky 플래그. extern(.h): `g_Functions_PLC[]`/`_LCD[]`.
  - 핸들러: 0x11 SetMode, 0x12 DataConfig, 0x13 Vehicle, 0x14 GetConnectorStatus, 0x21~23 BSA, 0x31~33 PLC, 0x51 Display, 0x53 VCI3, 0x81 Send_Error_Code, 0x82 APP_Error, 0x91 SetDevice_Start.
- **상관관계** — **호출(→)**: git-protocol(make/send), git-ble(`BTGetConnectStatus`), task-plc(`PLC_ApplyConfig`/step), task-bsa(`BSA_Start/Stop`), task-lcd(`LCD_Post*`), IO/CP/FAN/SENSOR. **피호출(←)**: git-comm 파서(`g_Functions_GDS[]`), 다수 모듈이 `FL_GDS_Send_Error_Code`/`StopDevice_WithoutPopup` 호출.

### 2.5 `git-functionlist-fwupdate.c`
- **역할** — GDS 프로토콜 FW 업데이트(0xA1~0xA6)·디바이스 시리얼(0xB1~0xB2) 핸들러.
- **특성**
  - git-functionlist.c와 동일 콜백 시그니처, 등록은 `g_Functions_GDS[]`에.
  - OTA: 0xA1 START(IDLE에서만)→0xA2 RECV(≤512B)→0xA3 END(size+CRC32 검증)→0xA5 SETLIST/0xA6 ForceApply(`FW_FLAG_COPY_PENDING`→`HAL_NVIC_SystemReset`). 0xB1 시리얼 write-once 후 광고명 갱신.
  - eMMC FatFs `f_stat/open/read/write/sync`.
- **정의항목** — 자체 매크로/타입 없음(핸들러 함수만): `FL_GDS_FwUpdate_Start/Recv/End/Check/SetList/ForceApply`, `FL_GDS_SetSerial/GetSerial`. 사용 상수(외부): `GDS_FID_*`, `SFwInfo`, `FW_FLAG_COPY_PENDING`.
- **상관관계** — **호출(→)**: git-protocol, sys-emmc(`DeviceSerial_Set/Get`, `fw_set_flag`), git-ble(`BTSetLocalDeviceNameReq`), fw_update(`fw_update_start/recv/end/set_list`), FatFs, `HAL_NVIC_SystemReset`. **피호출(←)**: git-comm 파서(전방선언은 git-functionlist.c).

---

## 3. 제어 · 하드웨어 태스크 계층

### 3.1 `task-hwcontrol.c / task-hwcontrol.h`
- **역할** — HW 제어(GPIO 릴레이 AC/DC/외부저항/SS팬·램프, TIM4 CP PWM, ADC1 CP 피드백) + IEC 61851 CP 상태머신으로 차량 AC 충전 시퀀스 구동.
- **특성**
  - TIM4 CH4(PD15) 1kHz PWM, ADC1 CH10(PC0) 500회 폴링 최댓값. 스레드 `hwconTask` `osEventFlagsWait(EVT_TASK_HWCON)` 후 1ms 루프. ISR 미사용.
  - `CP_Run()`: IDLE→WAIT4CONNECTING(6V×3)→PWM_START(DC-zero)→PWM_RUNNING(AC 과전류 26.9A즉시/16.8A 50ms)→STOP/ERROR. BSA_DISCHARGE self-healing, FAN 1분 cooldown.
- **정의항목**
  - #define(.c): `CP_READY_UPPER 1962`/`LOWER 1775`, `DC_ZERO_THRESH_V 5.0`, `AC_OVERCURR_MA 16800`/`AC_MAX_MA 26900`, `DC_RETRY_MAX 100`, TIM4(`_PRESCALER 14`/`_PERIOD 19999`), PWM pulse(`CP_PULSE_6A~16A`), `FAN_COOLDOWN_DURATION_MS`.
  - enum(.c): `RelayState_t`, `RelayType_t`, `CPState_t`, `CPErrorCode_t`.
  - 함수(공개): `HWCONTROL_init`, `StartHwConTask`, `RELAY_process`, `IO_AD_RELAY_control`, `IO_EXT_RLY_control`, `IO_ALL_OFF_control`, `IO_Resistance_Relay_Control`, `IO_SS_control`, `CP_PWM_control`, `CP_CLI_ReadADC`, `EMMC_Enable`, `FAN_CooldownArm`. CLI accessor `CP_CLI_*`.
- **상관관계** — **의존**: `sys-common.h`(HAL·`htim4`/`hadc1`·GPIO 매크로·센서 전역). **호출(→)**: HAL, `g_bBatRelayConFlag`(task-bsa). **피호출(←)**: task-bsa(릴레이/팬/저항), task-sensor(램프/팬), task-plc, cli, sys-main.

### 3.2 `task-sensor.c` (매칭 .h 없음)
- **역할** — 두 MCP3424 18bit I2C ADC로 방전 전류/전압·AC 전류·NTC 온도를 100ms 폴링, 물리량 변환·과전압/과온 판정·경고 램프/팝업.
- **특성**
  - ADC1(V/C, I2C2 @0x68): CH1 전류(ZEN-U2), CH2 전압(IVS-D4), CH3 AC(4-20mA). ADC2(Temp, I2C1 @0x68): 3ch NTC. 18bit One-Shot PGAx1.
  - 스레드 `sensorTask` `osEventFlagsWait(EVT_TASK_SENSOR)`, RUNNING 동안 100ms. I2C 블로킹(RDY 폴링, timeout 350ms).
  - 전류=Vadc×25.0, 전압=Vadc×504.8733, NTC 39점 LUT 보간. 에러 체크 START 후 30초 arm, 온도 NORMAL/CAUTION/WARN→SS 램프+LCD+STOP.
- **정의항목**
  - #define: I2C 주소, MCP3424 config 비트, `MCP3424_LSB_18BIT`, 변환상수(`CH1_VADC_TO_CURRENT 25.0`/`DISCHARGE_VADC_SCALE 504.8733`), 임계(`SENSOR_OVER_CHARGE_V 900`/`_DISCHARGE_V 200`/`_TEMPERATURE_C 100`, `_WARN_C 150`), `SENSOR_ERRCHK_START_DELAY_MS 30000`.
  - enum: `eSensorTempState`(NORMAL/CAUTION/WARN). typedef `NTC_LUT_t`+`ntc_lut[]`(39점).
  - 전역: `g_vc_adc_raw[]`, `g_discharge_current_A`/`_voltage_V`/`g_ac_charge_current_mA`, `g_temp_value[3]`, `g_power_W`.
  - 함수: `InitSensorTask`/`StartSensorTask`/`SENSOR_init/read/process`/`SENSOR_ErrorCheck_Arm/Disarm`.
- **상관관계** — **의존**: `sys-common.h`, git-functionlist.h, task-hwcontrol.h, task-lcd.h(`hi2c1/2`). **호출(→)**: `IO_SS_control`, `FL_GDS_StopDevice_WithoutPopup`/`Send_Error_Code`, `LCD_PostPopupWarnTemp/CautTemp`. **피호출(←)**: sys-main(`InitSensorTask`), git-functionlist(Arm/Disarm), hwcontrol CP_Run이 전압/전류 소비.

### 3.3 `task-bsa.c / task-bsa.h`
- **역할** — 배터리팩 CAN 시뮬(BLE 설정 config에 따라 CAN-FD로 릴레이 시뮬 프레임 주기 TX, 모니터링 프레임 RX로 전압/릴레이 추출).
- **특성**
  - FDCAN1(TX/RX는 task-can `BSA_SendCanPacket`/`OemReadCanBuff` 경유). 2 스레드 `BatRelayCon`(TX)+`BatRelayMon`(RX), 스택 2048, 1ms 루프. ISR 없음.
  - 3-tier: (a) BLE dynamic simframe, (b) 하드코딩 flexible TX, (c) legacy(미사용). CRC16-CCITT(poly 0x1021)+alive+비트필드 추출+선형변환.
  - `BSA_Start/Stop`이 FDCAN1 상태 점검·복구(BusOff→`CAN_HW_setting`) 및 스레드 최초 1회 생성.
- **정의항목**
  - #define: `CRC16_POLY 0x1021`, `MOSA_RX_TIMEOUT 55`, NE/OS EV CAN ID·factor, 배열 상한(`BSA_MAX_SIMFRAME_CONFIGS 10` 등), CAN type(0x00~0x03), protocolId(`CAN_CLASSIC 0x0100`/`CAN_FD 0x0130`), valueType, vehicle type(`MOSA_VEHICLETYPE_NEEV/DEEV/OSEV`).
  - enum: `eBatRelayConStatus`(None/Init/Run/Idle/Stop/Exit/PRA_On/PRA_Off), `eBatRelayConType`, `ePRAConStatus`, `eCANCommType`, `eBSA_MonSignalType`, 릴레이 상태 enum.
  - struct: `stBSA_COMM_DATA`, `stBSA_MON_DATA`, `stBSA_TX_CONFIG`/`_MANAGER`, `stBSA_ConvRule`, `stBSA_DataConfig`(RX), `stBSA_SimframeConfig`(TX), `stBSA_Step*`, CAN 프레임 union `stCanPacket`.
  - 전역: `g_bBatRelayConFlag`, `g_bBSA_DataConfigReady`/`SimframeConfigReady`, `g_stBSA_DataConfigs[]`/`SimframeConfigs[]`, `g_stBSA_MonData`, `g_bsaHwProtocolId`/`DataRate`.
  - 함수: `StartBSAThread`/`BatRelayCon(Monitor)Thread`/`BSA_Start`/`BSA_Stop`, `CalculateCRC16`, `BSA_ApplyDataConfig_Monitoring`/`BSA_ProcessSimframeTx`, flexible TX 세터, `Process_BSA_RxSignal`.
- **상관관계** — **의존**: sys-common, task-can.h, task-hwcontrol.h, git-comm.h, `<math.h>`. **호출(→)**: task-can(`OemReadCanBuff`/`BSA_SendCanPacket`/`CAN_HW_setting`, `hfdcan1`), task-hwcontrol(`IO_AD_RELAY_control`/`IO_SS_control`/`IO_Resistance_Relay_Control`), sys-main(타이머). **피호출(←)**: cli-bsa.c, git-functionlist(0x21~23), task-can(HW config 참조).

### 3.4 `task-can.c / task-can.h`
- **역할** — FDCAN1 HW/보레이트/필터·Classic·FD Std/Ext TX/RX/파싱·테스트 + BSA I/F(`OemReadCanBuff`/`BSA_SendCanPacket`) + FDCAN2 ISR 디스패처.
- **특성**
  - FDCAN1(kernel PLL2P 80MHz). 스레드 `canTask` `sendCAN_Q` 명령 처리. ISR `HAL_FDCAN_RxFifo0Callback`이 링버퍼(`g_rxPktRing[16]`)+`receiveCAN_Q`(FromISR); FDCAN2는 `PLC_FDCAN2_RxCallback` 위임.
  - DLC↔byte LUT, 보레이트 timing, BSA dataRate LUT(`CAN_MapDataRate`). `CAN_HW_setting`이 `g_bsaHwProtocolId/DataRate`로 frame format·baud 결정+종단저항(PE11).
- **정의항목**
  - #define: `CAN_RX_RING_SIZE 16`, `CAN_STD_FILTER_MAX 28`, `CAN_EXT_FILTER_MAX 8`.
  - enum(.h): `can_type_t`, `can_frame_t`, `eCanFrameFormat`(FDCAN/CLASSIC), `eCanBitTime`(0~7), `can_cmd_t`(INIT/TX_TEST/RX_TEST/LOOPBACK/TX_ALL).
  - struct(.h): `can_msg_param_t`, `stFdcanPkt`(data[64]), `can_parsed_msg_t`. 전역: `g_rxPktRing[]`, `g_bsaCanRxLastTick`.
  - 함수: `InitCANTask`/`StartCANTask`, `CAN_HW_setting`/`CAN_set_baud`/`CAN_configure_filter*`, `CAN_tx`/`CAN_rx`/`HAL_FDCAN_RxFifo0Callback`, `CAN_dlc_to_len`/`len_to_dlc`/`CAN_parse_rx_message`, 테스트, `OemReadCanBuff`/`BSA_SendCanPacket`.
- **상관관계** — **의존**: sys-common, task-plc.h, task-bsa.h(`hfdcan1`, 큐, BSA config). **호출(→)**: HAL FDCAN, task-plc(`PLC_FDCAN2_RxCallback`), BSA config 참조. **피호출(←)**: task-bsa(I/F), task-plc(DLC 변환), cli, sys-main.

### 3.5 `task-plc.c / task-plc.h`
- **역할** — FDCAN2(CAN 2.0B Ext, 500k, 8B)로 SECC(PLC) 통신, BLE config·스텝 시퀀스에 따라 DC 급속충전(차량 방전) 프로토콜 파싱·주기송신·단계제어.
- **특성**
  - Ext ID 필터 prefix 0x15EC0000/mask 0x1FFF0000, 종단저항 PE12, Intel(LE). 스레드 `plcTask`가 `VEHICLE_DISCHARGE`/`g_plcManualRun` 동안 RX→Step→TX. ISR `PLC_FDCAN2_RxCallback`→`g_plcRxRing[16]`+`plcRxCAN_Q`, 0x15ECC102 byte1 스누프.
  - config 기반 값 추출/저장, TX 동적값·heartbeat, BLE 0x31/0x32/0x33 파싱(BE, ASCII→hex), 스텝 엔진(substep·RX trigger min/max), stepno==99 정지 시퀀스. LCD SoC 1초 갱신.
- **정의항목**
  - #define: ID(`PLC_CAN_ID_PREFIX 0x15EC0000`/`_MASK`/`SRC_EVSE/PLC`/`DATA_LEN 8`), 상한(`PLC_MAX_RX_IDS 8`/`TX_IDS 8`/`MAX_STEP_ENTRIES 64`), device type(`_TX 0x01`/`_RX 0x02`), payload 크기, step(`PLC_STOPSTEP_NO 99`).
  - enum(.h): `ePLC_ResponseValueType`(CP/TargetV/I/SECC_STATUS/SOC…), `ePLC_RequestValueType`(DCextd/Heartbeat/EVSE*…), `ePLC_MSGDisp_SECCStatus`(SLAC~CurrentDemand~TERMINATE).
  - struct(.h): `stPLC_ConvRule`, `stPLC_DataConfigMsg`(16B), `stPLC_RxConfig`/`TxConfig`(dataTemplate), `stPLC_RxValues`/`TxValues`, `stPLC_StepMsg`/`StepConfig`/`StepData`/`StopStepData`.
  - 전역: `g_stPLC_Config`/`RxValues`/`TxValues`/`StepData`/`StopStepData`, step 엔진 상태(`g_plcStepRunning`/`CurrentStep` 등), `plcRxCAN_Q`, `g_plc15ECC102Last*`.
  - 함수: `InitPLCTask`, `PLC_CAN_Init/Start/Stop/Tx`, `PLC_FDCAN2_RxCallback`, `PLC_ApplyConfig`/`SetTxValue`/`SendReboot`, `PLC_ApplyStepConfig`/`ApplyStopSteps`/`StepSequenceStart/Stop/Process`.
- **상관관계** — **의존**: sys-common, task-can.h, task-lcd.h, task-hwcontrol.h. **호출(→)**: task-can(DLC 변환, `hfdcan2`), task-lcd(`LCD_PostTextSet`), `g_vci3_soc`. **피호출(←)**: task-can(ISR 위임), git-functionlist(0x31~33), cli-plc, sys-main, 커넥터 상태 핸들러(스누프).

### 3.6 `task-aim.c / task-aim.h`
- **역할** — AIM-D100-T 절연저항계(UART4 RS-485 Modbus-RTU 마스터)를 1초마다 폴링, 절연저항/대지전압/시스템전압/fault를 `g_aimData`에 갱신.
- **특성**
  - UART4(9600 8N1), 방향 GPIO `AIM_DIR`(PD14, LOW=RX/HIGH=TX). Modbus-RTU(slave 1, func 0x03). 스레드 `aimTask` 1초 폴링. ISR 없음(TX 50ms/RX 1500ms timeout).
  - CRC-16/MODBUS(0xA001, init 0xFFFF), 6-레지스터 일괄 read(0x20~0x25, 요청8B→응답17B), 저항 ×1kΩ·전압 ×0.1V.
- **정의항목**
  - #define: `AIM_SLAVE_ADDRESS 0x01`, func(`AIM_FUNC_READ_03H 0x03`…), 레지스터 맵(`AIM_REG_FAULT_TYPE 0x20`, 절연저항 P/N 0x21/0x22, 대지전압 0x23/0x24, `SYSTEM_VOLTAGE 0x25`), fault bit(`AIM_ST_*`, `_WIRING_ERR` bit15).
  - struct(.h): `stAIM_Data`(insul_res_p/n_kohm, volt_p/n_V, sys_volt_V, fault_status, comm_ok). 전역 `g_aimData`.
  - 함수: `AIM_init`/`AIM_set_dir`/`AIM_read_monitor`, `InitAimTask`/`StartAimTask`, static CRC/frame/process.
- **상관관계** — **의존**: sys-common, task-hwcontrol.h(`IO_CONTROL_*` AIM_DIR), `huart4`. **피호출(←)**: sys-main(`InitAimTask` — 현재 미기동), `g_aimData` 소비자.

---

## 4. UI · CLI 계층

### 4.1 `task-lcd.c / task-lcd.h`
- **역할** — UART8 터치 LCD 이벤트 기반 통신(화면/버튼/텍스트/팝업/RTC 큐 송신 + LCD→MCU 버튼 이벤트 파싱·콜백).
- **특성**
  - 스레드 `lcdTask`(스택 8KB). 외부는 `LCD_Post*()`로 큐(`stLCD_Event`, depth 16) 삽입 → 순차 실행.
  - 프로토콜 TX `EE B1 [cmd] .. FF FC FF FF`(cmd 0x00 screen/0x01 text/0x03 show-hide/0x04 touch/0x10 set/0x11 read), RX 링버퍼(256B)+EOF 파싱, 긴급정지 고정프레임. `g_lcd_ready` 핸드셰이크.
  - LCD STOP/NotifyYes 버튼이 BLE 0x91 STOP과 동일 정지 시퀀스(IO_ALL_OFF·RELAY·PLC 정지스텝·BSA_Stop·에러4·SS LED·FAN cooldown).
- **정의항목**
  - #define: 스크린 `LCD_SCR_MAIN/CHARGE/DISCHARGE/SETTING`, 버튼 `LCD_BTN_*`, RX 컨트롤ID `LCD_RX_*`, `LCD_TEXT_MAX(100)`.
  - enum: `eLCD_EventType`(BTN_STATE/SCREEN_GOTO/EMERGENCY_STOP/TEXT_SET/READ/SHOW_HIDE/RTC_SET/TOUCH_ENABLE).
  - struct: `stLCD_Event`, `stLCD_State`(prev/curr screen+텍스트 캐시). extern `g_lcdState`.
  - API: `InitLcdTask`, `LCD_UART8_RxCpltCallback`, `LCD_PostBtnState/ScreenGoto/EmergencyStop/TextSet/TextRead/ShowHide/RtcSet`, 팝업 `LCD_PostPopup*`, 버튼 핸들러 `LCD_OnCharge/Discharge/Main_*`(일부 `__weak`).
- **상관관계** — **호출(→)**: task-plc(`PLC_ApplyStopSteps`/`SetTxValue`), git-functionlist(`FL_GDS_Send_Error_Code`/`StopDevice_WithoutPopup`), git-comm(`COMM_IsBTConnected`), task-hwcontrol(`RELAY_process`/`IO_*`/`FAN_CooldownArm`), `BSA_Stop`. **피호출(←)**: main(`InitLcdTask`), sys-main(운용타이머 표시), cli(`lcd` 명령), git-comm(RX 콜백).

### 4.2 `task-cli.c / task-cli.h`
- **역할** — UART7 디버그 콘솔 CLI(GPIO/CAN/BLE/DB/시스템상태/CP-PWM/센서/LCD/eMMC/런타임 전 서브시스템 수동 테스트).
- **특성**
  - 스레드 `cliTask`(스택 4KB), `HAL_UART_Receive(huart7)` 폴링·라인 편집. `strtok` 토큰화→`CLI_Exec` 문자열 매칭. eMMC는 emmcTask 비동기 위임, BSA/PLC는 별도 파일.
  - `runtime` 명령이 device_state 토글로 상태머신 START/STOP edge 모사.
- **정의항목**
  - #define: `CLI_LINE_MAX(64)`, `CLI_MAX_ARGS(8)`. struct `cli_gpio_entry_t`+테이블 `g_cli_gpio_table[]`(릴레이/SS/CAN SW/BLE_RSTB/PLC_RST 라벨 맵).
  - 함수: `InitCLITask`/`StartCLITask`, `CLI_Exec`/`CLI_ProcessLine`/`CLI_Tokenize`, 도메인 `CLI_DB_Cmd`/`CLI_SYS_Cmd`/`CLI_CP_Cmd`/`CLI_SENSOR_Cmd`/`CLI_LCD_Cmd`.
  - 지원 명령: `gpio/can/canfd/dbg/ble/bsa/db/sys/plc/cp/emmc/sensor/lcd/fan/runtime/stop/reset`.
  - 헤더(task-cli.h): `InitCLITask`/`StartCLITask`/`CLI_BSA_Cmd`/`CLI_PLC_Cmd` 프로토타입만(얇음).
- **상관관계** — **호출(→)**: 거의 전 모듈(sys/can/plc/lcd/bsa/hwcontrol/emmc/functionlist). **피호출(←)**: main(`InitCLITask`). BSA/PLC 명령은 cli-bsa/cli-plc로 위임.

### 4.3 `cli-bsa.c`
- **역할** — task-cli에서 분리된 BSA 도메인 CLI(모니터링/시뮬프레임 CAN 설정·시작/정지/상태).
- **특성** — `bsa` 서브명령(show/setmon/setsim/setex/clear/start/stop/status). `setex`가 BLE와 동등한 예시(모니터링2+시뮬4) 코드 주입. CAN-FD CRC16/Alive/Voltage 메시지·변환규칙 설정.
- **정의항목** — 공개 `CLI_BSA_Cmd(argc,argv)`; static `CLI_BSA_Show/SetMon/SetSim/SetExample/Clear/Status`. 자체 타입 없음(task-bsa.h 심볼 사용).
- **상관관계** — **의존**: task-bsa.h(핵심), sys-common 등. **호출(→)**: `BSA_Start/Stop`/`BSA_ResetSimframeState`. **피호출(←)**: task-cli.c.

### 4.4 `cli-plc.c`
- **역할** — task-cli에서 분리된 PLC 도메인 CLI(RX/TX/스텝 설정·TX 값 주입·릴레이/SS 제어·스텝 시퀀스 제어).
- **특성** — `plc` 서브명령 다수(show/rx/tx/set/setex/stepex/stepstart/stepstop/step/pv/pc/reboot + 릴레이 단축). SECC/GDS CAN ID·변환규칙을 매크로(`PLC_SET_MSG`/`STEP_MSG`)로 구성, 충전 절차 예시(1~14+stop 99) 로드. PV/PC 수동·자동 전환.
- **정의항목** — 공개 `CLI_PLC_Cmd(argc,argv)`; static `CLI_PLC_Show/ShowRx/SetExample/SetStepExample`. 지역 매크로(정의 후 `#undef`). 자체 타입 없음(task-plc.h 심볼 사용).
- **상관관계** — **의존**: task-plc.h(핵심), task-hwcontrol.h. **피호출(←)**: task-cli.c.

---

## 5. 저장 계층

### 5.1 `sys-emmc.c / sys-emmc.h`
- **역할** — eMMC(KLM8G1GETF)+FatFs 통합(DMA/세마포어 디스크 백엔드·관리·USB MSC 백엔드·비동기 emmcTask·write-once 시리얼).
- **특성**
  - 3계층: [1] 백엔드 `emmc_*()`(user_diskio 호출, 32B aligned DMA 64섹터, IRQ→세마포어), [2] 관리 API(마운트/포맷/폴더/진단), [3] `emmcTask`(스택 32KB)+명령 큐(CLI가 `EMMC_PostCmd` 위임).
  - USB MSC 백엔드(`EMMC_MSC_*`)는 USB ISR에서 RTOS 미사용 폴링. DeviceSerial write-once("BDC"+8자, preamble+lock+CRC32). f_mkfs 후 sector0 워크어라운드.
- **정의항목**
  - #define(.h): 폴더/파일 상수(`EMMC_APPLICATION_FOLDER`/`EMMC_BACKUP_FOLDER`), `EMMC_PATH_MAX(64)`, DeviceSerial(`DEVSERIAL_SUFFIX_LEN 8`, `GDS_FID_SET_SERIAL 0xB1`/`GET 0xB2`).
  - enum: `EmmcCmd_t`(INFO/TEST/FORMAT/LS/MKDIR/…), `eEMMC_ERROR_CODE`.
  - API: 백엔드 `emmc_initialize/read/write/ioctl`; 관리 `InitEMMC`/`mountFatFS`/`formatEmmc`/`EMMC_Dump*`; USB MSC `EMMC_MSC_Read/Write`; 태스크 `InitEmmcTask`/`EMMC_PostCmd`/`EMMC_PostWriteFile` 등; 시리얼 `DeviceSerial_Load/Get/Set`.
- **상관관계** — **의존**: sys-common, task-hwcontrol.h(전원/RST), main.h(`hmmc1`), FatFs, fw_crc32.h. **사용처(←)**: user_diskio.c(백엔드), usbd_storage_if.c(MSC), task-cli.c(`emmc`), sys-main(InitEMMC — 현재 주석), git-ble/fwupdate(시리얼).

---

## 6. 공유 · 펌웨어 업데이트 계층 (`Common/`)

> **정의 소유권**: `fw_update.h`가 **중앙 헤더** — 대부분의 상수·enum·struct(`slot_info_t`/`slot_metadata_t`/`fw_update_ctx_t`)·fw_flag 상태·모든 모듈 프로토타입(`xspi_flash_*`/`slot_*`/`BackupSRAM_Init`/`fw_update_*`)이 여기 위치. `fw_slot_manager.h`/`fw_flash_xspi.h`는 `#define` 하나씩만 두고 API 선언은 fw_update.h에 위임. `firmware_lite.h`가 `SFwInfo`/`AppName` 소유.
>
> **BOOT_BUILD 분리**: Appli는 `fw_update.c`·`fw_slot_manager.c`(복사 경로 guard off)·`fw_crc32.c`·`fw_backup_sram.c` 링크. Boot는 `fw_slot_manager.c`·`fw_flash_xspi.c`·`fw_crc32.c`·`fw_backup_sram.c`·`boot_emmc.c`·`main.c` 링크(`fw_update.c` 제외). `fw_flash_xspi.c`는 사실상 Boot 전용(EXTMEM 미들웨어).

### 6.1 `fw_update.c / fw_update.h`
- **역할** — (.c) Appli 전용 OTA 다운로드 상태머신 + GDS 핸들러. (.h) 전체 FW-update 시스템 중앙 정의(Boot+Appli 공유).
- **특성** — `.c`는 Appli 전용("Boot에 link 금지"). FatFs 블로킹, SHA-256는 필드만 보존. Slot staging은 Boot 전담(Appli는 `fw_flag=COPY_PENDING`+`SystemReset`만). BSS 4KB `s_copy_buf`.
- **정의항목**
  - Flash map: `EXEC_SLOT_OFFSET=0x10000`, `STAGING_SLOT_OFFSET=0x04000000`, `EXEC_SLOT_XIP_ADDR=0x90010000`, `FLASH_TOTAL_SIZE=128MB`, `XSPI_SECTOR_SIZE=4KB`.
  - Metadata: `SLOT_META_MAGIC=0x534C5432("SLT2")`, `SLOT_VALID_MARK=0xAA`, `COPY_PENDING_FLAG=0xAA`.
  - BKPSRAM: 오프셋(`FW_FLAG=0x410`/`FAIL_COUNT=0x414`/…), `CHK_MAGIC_VALID=0xA5A5BEEF`, `DEV_BYPASS_MAGIC=0xDEC0DE01`, 접근 매크로(D-cache aware).
  - **fw_flag 상태머신**: `IDLE=0x00`→`STAGING=0xA5`→`COPY_PENDING=0xB4`→`COPYING=0xD2`→`BOOT_TEST=0x5A`→`VERIFIED=0xC3`(비트 간격 넓게).
  - GDS: `GDS_FID_FW_UPDATE_START=0xA1`…`SET_LIST=0xA5`/`FORCE=0xA6`. Recovery: `RECOVERY_MAX_FAIL_COUNT=3`, phase RECOPY/RELOAD/ROLLBACK/SAFEMODE, `COPY_MAX_RETRY=3`.
  - enum: `fw_update_state_t`, `slot_id_t`, `fw_update_error_t`(12), `fw_update_resp_t`. struct(__packed): `slot_info_t`(45B), `slot_metadata_t`(4096B), `fw_update_ctx_t`(Appli). inline BKPSRAM 접근자.
  - 함수(.c): `fw_update_init/start(0xA1)/recv(0xA2)/end(0xA3)/check(0xA4 deprecated)/set_list(0xA5)`, `fw_update_backup_before_replace`, `fw_update_swap_ini_from_backup`.
- **상관관계** — **의존**: fw_compat.h, fw_slot_manager.h, firmware_lite.h, ff.h(Appli), fw_crc32.h. **사용처(←)**: 헤더는 거의 전 FW 모듈 include; `.c`는 Appli만(git-functionlist-fwupdate가 호출).

### 6.2 `fw_slot_manager.c / fw_slot_manager.h`
- **역할** — NOR flash slot 메타데이터 관리(Primary+Backup 이중화) + Slot1→Slot0 복사·CRC 검증(Boot 전담).
- **특성** — Write order Backup→Primary(전원차단 안전), CRC 범위 95B. `slot_copy_staging_to_exec()` 본체는 `#ifdef BOOT_BUILD`(Appli는 `HAL_ERROR` 가드). 4KB 청크 readback+전체 CRC32+재시도3. static 버퍼(Boot CSTACK 보호).
- **정의항목** — 헤더: `SLOT_META_CRC_SIZE(=95B)`만. private: `s_meta`/`s_chunk_buf`, `meta_calc_crc/read/write/is_valid/flush`. 공개: `slot_manager_init`/`slot_get_exec_addr`/`slot_mark_staging_valid`/`slot_set_copy_pending`/`slot_copy_staging_to_exec`(BOOT)/`slot_verify_image_crc`.
- **상관관계** — **호출(→)**: xspi_flash_*, fw_crc32_*, `fw_set_flag`, `HAL_XSPI_Abort`, `FW_IWDG_REFRESH`. **사용처(←)**: Boot boot_emmc.c/main.c; Appli 링크하나 복사 경로 guard off.

### 6.3 `fw_flash_xspi.c / fw_flash_xspi.h`
- **역할** — ST EXTMEM(XSPI1 NOR SFDP) indirect-mode 래퍼(init/erase/program/read/verify).
- **특성** — Boot 전용(Appli는 XIP memory-mapped read만). 블로킹 폴링, 4KB마다 IWDG. program은 256B page 정렬(0xFF 패딩), erase 4KB 정렬. static 버퍼(AXI SRAM).
- **정의항목** — 헤더: `XSPI_EXTMEM_ID=0`. private: `s_flash_size`(SFDP)/`s_verify_buf`. 공개: `xspi_flash_init/get_size/erase_sector/program/read/verify`.
- **상관관계** — **호출(→)**: EXTMEM(`EXTMEM_GetInfo/EraseSector/Write/Read`). **사용처(←)**: fw_slot_manager.c, boot_emmc.c(Boot 전용).

### 6.4 `fw_crc32.c / fw_crc32.h`
- **역할** — 순수 SW CRC-32/ISO-HDLC(IEEE 802.3) 스트리밍+1회성 API.
- **특성** — 외부 의존 zero(`<stdint.h>`만), Boot+Appli 공유. poly 0xEDB88320, init/xorout 0xFFFFFFFF, RefIn/Out true, bitwise(no table). 순수 계산.
- **정의항목** — 함수 `fw_crc32`(1회), `fw_crc32_start/update/finish`(스트리밍).
- **상관관계** — **의존**: `<stdint.h>`. **사용처(←)**: fw_update.c, fw_slot_manager.c, boot_emmc.c, sys-emmc.c.

### 6.5 `fw_backup_sram.c`
- **역할** — 내부 Backup SRAM(0x38800000, 4KB) 접근 활성화(Boot↔Appli fw_flag 신호 채널).
- **특성** — Boot+Appli 공유, 각 `main()` 첫 단계 호출. VBAT 없음(BAT1 미장착)→메인 전원 동안만 유지(soft reset 생존, OTA엔 충분). 짧은 HW 셋업(DBP+BKPRAM clk+backup regulator).
- **정의항목** — 함수 `BackupSRAM_Init(void)`(선언은 fw_update.h).
- **상관관계** — **호출(→)**: HAL PWR/RCC. **사용처(←)**: Boot main.c, Appli main.c.

### 6.6 `firmware_lite.h`
- **역할** — 단순화 펌웨어 정보 정의(단일앱+Recovery용 App enum·FirmwareInfo.ini 구조체).
- **특성** — Boot+Appli 공유, 순수 정의(`<stdint.h>`만). `MAX_APP_CNT=2`. eMMC 영속 `SFwInfo`를 Boot가 읽어 부팅 모드 결정.
- **정의항목** — #define `MAX_APP_CNT=2`, `SAFE_MODE_APP=eApp_Recovery`, `VCI3_FWINFO_PREAMBLE=0x45564344("EVCD")`, `BOOT_FWINFO_FILE="FirmwareInfo.ini"`. enum `AppName`(None/Main/Recovery). struct `SAppInfo`, `SFwInfo`.
- **상관관계** — **사용처(←)**: fw_update.h(→전 모듈), boot_emmc.

### 6.7 `fw_compat.h`
- **역할** — frp-scan 원본 외부 심볼(로깅/IWDG/EXTMEM)을 evcd_test 환경으로 격리하는 호환 어댑터.
- **특성** — `extmem_manager.h` include는 `#ifdef BOOT_BUILD`. 빌드 옵션: `FW_COMPAT_VERBOSE`(기본0→로깅 no-op), `FW_COMPAT_USE_IWDG`(기본0→**IWDG no-op**). 순수 매크로 shim.
- **정의항목** — 매크로 `GLogI/GLogE`, `FW_IWDG_REFRESH()`. 조건부 extern `hiwdg`. config guard.
- **상관관계** — **의존**: main.h, extmem_manager.h(Boot). **사용처(←)**: fw_update.c, fw_slot_manager.c, fw_flash_xspi.c, boot_emmc.c.

---

## 7. 부트로더 계층 (`Boot/`)

### 7.1 `Boot/Core/Src/main.c`
- **역할** — 부트로더 엔트리(클럭/MPU/XSPI/EXTMEM/UART init → `Boot_FwUpdateCheck` fw_flag 분기 → `BOOT_Application()` XIP 점프).
- **특성** — BOOT_BUILD, `hxspi1`/`huart7` 실체 정의. `Boot_FwUpdateCheck`는 `#ifdef FW_UPDATE_ENABLED`. MPU 0x90000000 256MB cacheable/executable. checkpoint breadcrumb. IAR `__write` UART7.
- **정의항목** — 전역 `huart7`/`hxspi1`. static `SystemClock_Config`/`MPU_Config`/`MX_XSPI1_Init`/`Boot_FwUpdateCheck`(fw_flag switch). `main`/`BOOT_Application` 호출.
- **상관관계** — **호출(→)**: `BackupSRAM_Init`, `xspi_flash_init`, `slot_manager_init`, `fw_get/set_flag`, `Boot_LoadFwInfo/LoadAndStageApp`, `BOOT_Application`. Boot 전용.

### 7.2 `Boot/Core/Src/boot_emmc.c / boot_emmc.h`
- **역할** — Boot 컨텍스트 eMMC/SDMMC1+FatFs(RO) 마운트·FirmwareInfo.ini 로드·eMMC→Slot1 staging→Slot0 복사 오케스트레이션.
- **특성** — BOOT_BUILD 전용(Appli sys-emmc 대비 단순화: 폴링/직접 ReadBlocks/RO). `HAL_MMC_MspInit` weak override(SDMMC1 8bit). BSS `s_boot_fatfs`/`s_stage_buf`. staging: 4KB 청크 program+verify+CRC32→`HAL_XSPI_Abort`→mark_valid→copy_pending→copy.
- **정의항목** — 전역 `hmmc1`, extern `hxspi1`. 공개 `MX_SDMMC1_MMC_Init`/`Boot_InitEmmc`/`Boot_LoadFwInfo`/`Boot_GetFwInfo`/`Boot_LoadAndStageApp(folder, app_no)`.
- **상관관계** — **호출(→)**: FatFs, HAL_MMC, slot_*, xspi_flash_*, fw_crc32_*, `HAL_XSPI_Abort`. **사용처(←)**: Boot main.c. Boot 전용.

---

## 8. 모듈 간 상관관계 (종합)

### 8.1 핵심 데이터 흐름 — BLE 명령 경로
```
UART2 ISR(git-comm) → 링버퍼 → UART2_read_buff
  ├─ BT 연결: BLE_UART2_PushRx(git-ble) → UART2_Process_GitProtocol(git-comm)
  │            → GITPACKET_parse_frame(git-protocol) → g_Functions_GDS[FuncID]
  │            → FL_GDS_*(git-functionlist / -fwupdate)
  │            → 응답 GITPACKET_send_response(git-protocol) → UART2_Transmit_Polling(git-comm)
  └─ BT 미연결: BLE_ProcessRxQueue → BTRecvParsing(git-ble)  [AT 응답]
```

### 8.2 제어 축 — 상태머신 → 태스크
```
main.c(defaultTask 10ms) → sys-main.c state_machine()
   IDLE→RUNNING 시 osEventFlagsSet(EVT_TASK_*)
     ├ EVT_TASK_SENSOR → sensorTask   (전압/전류/온도, 이상 시 FL_GDS_Send_Error_Code)
     ├ EVT_TASK_HWCON  → hwconTask     (RELAY_process, CP_Run)
     ├ EVT_TASK_PLC    → plcTask       (VEHICLE_DISCHARGE: SECC 스텝 시퀀스)
     └ (BSA_DISCHARGE) → BSA_Start()   (g_bBatRelayConFlag로 BatRelayCon/Mon 구동)
```

### 8.3 CAN 공유 — task-can이 FDCAN1/2 게이트웨이
```
HAL_FDCAN_RxFifo0Callback(task-can)
   ├ FDCAN1 → g_rxPktRing + receiveCAN_Q → OemReadCanBuff(task-bsa)
   └ FDCAN2 → PLC_FDCAN2_RxCallback(task-plc) → g_plcRxRing + plcRxCAN_Q
task-bsa → BSA_SendCanPacket(task-can) → CAN_tx(FDCAN1)
task-plc → PLC_CAN_Tx(FDCAN2)
```

### 8.4 정지(STOP) 시퀀스 — 다중 진입점, 공통 처리
- 진입점: BLE 0x91 STOP(git-functionlist), LCD STOP 버튼(task-lcd), BLE 연결 해제(git-comm), 센서 이상(task-sensor).
- 공통: `IO_ALL_OFF_control` → `RELAY_process(eMODE_NONE)` → PLC 정지스텝/`BSA_Stop` → `FL_GDS_Send_Error_Code`(0x81) → SS LED → `FAN_CooldownArm`.

### 8.5 OTA 경로 — Appli ↔ Boot (BKPSRAM 핸드오프)
```
git-functionlist-fwupdate(BLE 0xA1~A5) → fw_update.c → eMMC(sys-emmc/FatFs, CRC32)
   → fw_set_flag(COPY_PENDING) → SystemReset
Boot/main.c Boot_FwUpdateCheck(fw_flag) → boot_emmc(Boot_LoadAndStageApp)
   → fw_flash_xspi(Slot1 program) → fw_slot_manager(Slot1→Slot0, CRC32) → BOOT_TEST → XIP 점프
```

### 8.6 주요 의존 매트릭스 (사용 → 피사용, 굵은 화살표=핵심)
| 모듈 | 주로 호출하는 대상 | 주로 호출받는 출처 |
|---|---|---|
| git-comm | git-ble, git-protocol, git-functionlist | main/sys, HAL ISR |
| git-protocol | (git-functionlist.h 매크로) | git-comm, git-functionlist(+fw) |
| git-functionlist | git-protocol, task-bsa, task-plc, task-lcd, hwcontrol | git-comm 파서 |
| task-can | task-plc(위임), task-bsa(config) | task-bsa, task-plc |
| task-bsa | task-can, task-hwcontrol | cli-bsa, git-functionlist |
| task-plc | task-can, task-lcd | task-can(ISR), git-functionlist, cli-plc |
| task-sensor | hwcontrol, git-functionlist, task-lcd | sys-main, git-functionlist |
| task-hwcontrol | HAL, (g_bBatRelayConFlag) | bsa, sensor, plc, cli, sys-main |
| task-lcd | task-plc, hwcontrol, git-functionlist, BSA_Stop | main, sys-main, cli, git-comm |
| sys-emmc | hwcontrol, FatFs, fw_crc32 | diskio, USB MSC, cli, fwupdate |
| fw_update(.c) | fw_crc32, fw_slot_manager, FatFs | git-functionlist-fwupdate (Appli) |
| fw_slot_manager | fw_flash_xspi, fw_crc32 | boot_emmc, Boot main (Boot) |
| fw_flash_xspi | EXTMEM | fw_slot_manager, boot_emmc (Boot) |

### 8.7 계층 원칙 요약
- **허브**: `typedef.h` ← `sys-common.h` 위에 모든 Appli 모듈이 얹힌다.
- **게이트웨이**: 통신은 `git-comm`(라우터)·`task-can`(CAN 디스패처)이 각 도메인으로 분배.
- **분리 컴파일**: `Common/`은 Boot/Appli 공유하되 `#ifdef BOOT_BUILD`로 실동작 분기. `fw_update.h`가 정의 소유 허브.
- **헤더 없는 파일**: sys-main.c, cli-bsa.c, cli-plc.c, main.c, freertos.c(프로토타입은 sys-common.h / task-cli.h / main.h).

---

### 참고 문서
- 아키텍처·시퀀스 다이어그램: `docs/EVCD_Test_FW_Diagrams.drawio` (8종)
- FW 설계 발표자료: `docs/EVCD_Test_FW_Design_Overview.pptx`
- 설계 스펙: `docs/EVCD_Test_FW_Design.xlsx`
