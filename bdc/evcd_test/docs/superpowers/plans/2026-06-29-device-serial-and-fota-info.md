# 디바이스 시리얼 + FOTA 정보 저장 구현 플랜 (전부 NOR Flash)

> **For agentic workers:** 펌웨어 플랜 — 각 Task "검증"은 host 단위테스트가 아니라 **빌드 → 플래시 → UART 시리얼/BLE 관찰**. 체크박스(`- [ ]`)로 추적.

**Goal:** 디바이스 식별/FOTA 정보(시리얼, boot/app/recovery FW 버전·종류)를 **단일 `device_info` 구조로 XSPI NOR에 저장**. BLE 광고 이름 = `BDC` + 저장된 시리얼. 시리얼은 BLE로 설정(write-once).

**Architecture (전부 NOR, 확정):**
- 모든 정보 → **XSPI NOR `device_info_t`** (Primary `0x0000_1000` / Backup `0x07FF_1000`, 예약 메타영역 빈 섹터, slot_metadata 와 동일 이중화).
- 명명: **`preamble`**(not magic), **`app_no`**(+AppName enum), `valid`=0xAA, `version[3]`, `crc32`.
- ⚠ **Appli는 NOR 직접 쓰기 불가**(Slot0와 같은 칩 XIP 실행 → read-while-write). 따라서:
  - **읽기**: Boot=indirect(`xspi_flash_read`), Appli=XIP 직접 포인터 read.
  - **쓰기**: **Boot 전담**(내장 플래시 실행). Appli의 설정 요청은 BKPSRAM 채널로 넘겨 reset 후 Boot이 커밋.

**Tech Stack:** STM32H7S3, XSPI NOR(EXTMEM/XIP), Backup SRAM 시그널, BnCom BLE(AT+NAME), GDS BLE 프로토콜.

---

## File Structure

| 파일 | 책임 | 변경 |
|---|---|---|
| `Common/Inc/fw_devinfo.h` | `device_info_t`/`fw_entry_t` + API 선언 | Create |
| `Common/Src/fw_devinfo.c` | read(Boot indirect/Appli XIP) + write(Boot 전용) + Primary/Backup 이중화 | Create |
| `Common/Inc/fw_update.h` | `DEVINFO_OFFSET_PRIMARY/BACKUP`, BKPSRAM 프로비저닝 오프셋 추가 | Modify |
| `EWARM/Boot/*.ewp`, `EWARM/Appli/*.ewp` | `fw_devinfo.c` 빌드 추가 | Modify |
| `Boot/Core/Src/main.c` | `devinfo_init` + 시리얼 프로비저닝 커밋 + boot/app 버전 유지 | Modify |
| `Appli/Function/Src/BluetoothLowEnergy.c` | `BTSetLocalDeviceNameReq` 가 `devinfo_get_serial()` 사용 | Modify |
| `Appli/Function/Src/git-functionlist-fwupdate.c` | `0xB1 SetSerial / 0xB2 GetSerial` 핸들러 | Modify |
| `Appli/Function/Src/git-functionlist.c` | 핸들러 테이블 등록 + forward decl | Modify |

---

## 데이터 구조 (NOR, 4KB = 1섹터)

```c
/* fw_devinfo.h */
#define DEVINFO_PREAMBLE    0x44455631U   /* "DEV1" — preamble 컨벤션 (firmware_lite.h muiPreamble 와 동일) */
#define DEVINFO_MAX_FW      8U
#define DEVINFO_SERIAL_LEN  16U           /* BLE suffix("00000123") + 여유 */
#define DEVINFO_LOCK_MARK   0xAAU         /* SLOT_VALID_MARK 와 동일 — write-once 잠김 */

typedef __packed struct {
    uint8_t  app_no;        /* AppName: eApp_Main/eApp_Recovery/... (slot_info_t 와 동일) */
    uint8_t  version[3];    /* major.minor.patch */
    uint32_t crc32;         /* 해당 이미지 CRC (옵션) */
    uint8_t  valid;         /* 0xAA */
    uint8_t  rsv[3];
} fw_entry_t;               /* 12B */

typedef __packed struct {
    uint32_t   preamble;        /* DEVINFO_PREAMBLE */
    uint16_t   struct_ver;
    char       serial[DEVINFO_SERIAL_LEN];  /* BLE suffix, write-once */
    uint8_t    serial_locked;   /* DEVINFO_LOCK_MARK = 잠김 */
    uint8_t    fw_count;
    fw_entry_t fw[DEVINFO_MAX_FW];
    uint8_t    reserved[4096 - 4 - 2 - DEVINFO_SERIAL_LEN - 1 - 1 - (12*DEVINFO_MAX_FW) - 4];
    uint32_t   crc32;           /* preamble..reserved CRC (이 필드 제외) */
} device_info_t;
_Static_assert(sizeof(device_info_t) == 4096, "device_info_t must be 4096");
```

```c
/* fw_update.h 추가 */
#define DEVINFO_OFFSET_PRIMARY   0x00001000U   /* Primary 메타영역 내 빈 섹터 (slot meta 다음) */
#define DEVINFO_OFFSET_BACKUP    0x07FF1000U   /* Backup 메타영역 내 빈 섹터 */
/* BKPSRAM 시리얼 프로비저닝 채널 (Appli→Boot) */
#define BKPSRAM_DEVINFO_FLAG     0x450U        /* 1B: 0xAA = 시리얼 기록 대기 */
#define BKPSRAM_DEVINFO_SERIAL   0x454U        /* 16B: 기록할 시리얼 suffix */
#define DEVINFO_PROVISION_MARK   0xAAU
```

---

## Task 1: fw_devinfo.h/c — NOR read/write + 이중화

**Files:** Create `Common/Inc/fw_devinfo.h`, `Common/Src/fw_devinfo.c`; Modify `Common/Inc/fw_update.h`

- [ ] **Step 1: 맵/BKPSRAM 오프셋 + 구조체 정의** (위 스니펫).

- [ ] **Step 2: read 추상화 (Boot=indirect / Appli=XIP 직접)**
```c
static HAL_StatusTypeDef devinfo_read_raw(uint32_t off, device_info_t *out) {
#ifdef BOOT_BUILD
    return xspi_flash_read(off, (uint8_t*)out, sizeof(*out));   /* indirect */
#else
    memcpy(out, (const void*)(XSPI1_BASE_ADDR + off), sizeof(*out)); /* XIP 직접 */
    return HAL_OK;
#endif
}
```

- [ ] **Step 3: init / getters (Boot+Appli 공용)** — Primary 유효 → 사용 / 손상 → Backup. preamble+crc 검증(slot `meta_is_valid` 패턴).
```c
HAL_StatusTypeDef devinfo_init(void);
const char*       devinfo_get_serial(void);          /* serial[], 미설정 "00000000" */
uint8_t           devinfo_is_serial_locked(void);
const fw_entry_t* devinfo_get_fw(uint8_t app_no);
```

- [ ] **Step 4: write (BOOT_BUILD 전용)** — Appli에선 즉시 HAL_ERROR(안전 가드). Backup→Primary 이중화, 쓰기 후 XSPI Abort(대량 R/W 아님이라 불필요할 수 있으나 일관성).
```c
HAL_StatusTypeDef devinfo_set_serial(const char *suffix);   /* write-once: locked면 거부 */
HAL_StatusTypeDef devinfo_set_fw(uint8_t app_no, const uint8_t ver[3], uint32_t crc);
```

- [ ] **Step 5: .ewp 등록** — `fw_devinfo.c` 를 Boot/Appli 빌드에 추가(ff.c 추가했던 방식). Appli는 read만 사용(write는 가드).

- [ ] **Step 6: 빌드 확인** — Boot/Appli 각각 Rebuild, 0 errors.

- [ ] **Step 7: 커밋**
```bash
git add Common/Inc/fw_devinfo.h Common/Src/fw_devinfo.c Common/Inc/fw_update.h EWARM/Boot/*.ewp EWARM/Appli/*.ewp
git commit -m "feat(devinfo): NOR device_info (serial + fw table), boot-write/appli-read"
```

## Task 2: Boot — devinfo_init + 시리얼 프로비저닝 커밋 + 버전 유지

**Files:** Modify `Boot/Core/Src/main.c`

- [ ] **Step 1: Boot_FwUpdateCheck 내 init + 프로비저닝** (slot_manager_init 다음)
```c
devinfo_init();

/* Appli가 BLE로 요청한 시리얼을 NOR에 커밋 (write-once) */
if (BKPSRAM_READ8(BKPSRAM_DEVINFO_FLAG) == DEVINFO_PROVISION_MARK) {
    char s[DEVINFO_SERIAL_LEN];
    for (uint32_t i=0;i<DEVINFO_SERIAL_LEN;i++) s[i]=(char)BKPSRAM_READ8(BKPSRAM_DEVINFO_SERIAL+i);
    s[DEVINFO_SERIAL_LEN-1]='\0';
    if (!devinfo_is_serial_locked()) {
        if (devinfo_set_serial(s)==HAL_OK) printf("[BOOT] serial provisioned: BDC%s\r\n", s);
    } else printf("[BOOT] serial already locked — skip\r\n");
    BKPSRAM_WRITE8(BKPSRAM_DEVINFO_FLAG, 0);   /* 플래그 클리어 */
}
```

- [ ] **Step 2: Boot 버전 자동 유지** — `BOOT_FW_VERSION` 컴파일 상수 ≠ `devinfo_get_fw(eApp_Boot)` 면 `devinfo_set_fw(eApp_Boot, ...)`.

- [ ] **Step 3: App 버전 기록** — `Boot_LoadAndStageApp` staging 성공 시(XSPI Abort 이후) `devinfo_set_fw(app_no, ver, final_crc)`.

- [ ] **Step 4: 검증** — 정상 부팅 로그에 devinfo 로드 확인. (시리얼 커밋은 Task 4에서 E2E)

- [ ] **Step 5: 커밋** `feat(boot): devinfo init + serial provision commit + fw version maintain`

## Task 3: BLE 광고 이름을 device_info 시리얼에서 생성

**Files:** Modify `Appli/Function/Src/BluetoothLowEnergy.c:455`

- [ ] **Step 1: Appli devinfo 로드** — Appli 부팅 초기에 `devinfo_init()` 1회 호출(XIP read).

- [ ] **Step 2: 이름 조립 변경**
```c
// 변경 전: '0'×8 하드코딩
snprintf(strLocalBTDeviceName, MAX_BT_DEVICE_NAME, "%s%s",
        BLE_PREFIX_DEVICE_NAME, devinfo_get_serial());   /* "BDC"+저장 시리얼 */
```

- [ ] **Step 3: 검증** — 부팅 시 `BLE: SeriName: BDC00000000`(미설정 기본) → 앱 스캔 노출.

- [ ] **Step 4: 커밋** `feat(ble): advertised name from device_info serial`

## Task 4: BLE 0xB1 SetSerial / 0xB2 GetSerial

**Files:** Modify `git-functionlist-fwupdate.c`, `git-functionlist.c`, FuncID 정의

- [ ] **Step 1: FuncID**
```c
#define GDS_FID_SET_SERIAL   0xB1U   /* payload: [char suffix[8]] → BKPSRAM 적재 + reset */
#define GDS_FID_GET_SERIAL   0xB2U   /* 응답에 현재 suffix */
```

- [ ] **Step 2: SetSerial 핸들러 (Appli는 NOR 못 쓰니 BKPSRAM 경유 + reset)**
```c
void FL_GDS_SetSerial(void *p, uint32_t t, uint32_t len) {
    (void)t;
    if (len < 8U) { GITPACKET_send_response(GDS_FID_SET_SERIAL, GITPACKET_NAK); return; }
    if (devinfo_is_serial_locked()) {            /* 이미 잠김 → write-once 거부 */
        GITPACKET_send_response(GDS_FID_SET_SERIAL, GITPACKET_NAK); return;
    }
    const uint8_t *s = (const uint8_t*)p;
    for (uint32_t i=0;i<8U;i++) BKPSRAM_WRITE8(BKPSRAM_DEVINFO_SERIAL+i, s[i]);
    BKPSRAM_WRITE8(BKPSRAM_DEVINFO_SERIAL+8, 0);
    BKPSRAM_WRITE8(BKPSRAM_DEVINFO_FLAG, DEVINFO_PROVISION_MARK);
    GITPACKET_send_response(GDS_FID_SET_SERIAL, GITPACKET_ACK);  /* ACK 먼저 */
    HAL_Delay(100);
    HAL_NVIC_SystemReset();   /* Boot이 NOR 커밋 후 새 이름으로 광고 */
}
```

- [ ] **Step 3: GetSerial 핸들러** — `devinfo_get_serial()` 을 응답 payload로 (프로젝트 데이터응답 헬퍼 시그니처에 맞춰).

- [ ] **Step 4: 테이블 등록** `{0xB1,FL_GDS_SetSerial},{0xB2,FL_GDS_GetSerial}` + forward decl.

- [ ] **Step 5: E2E 검증 (핵심 수용 테스트)**
  1. 앱 0xB1 "00000123" → ACK → 자동 reset
  2. Boot 로그 `[BOOT] serial provisioned: BDC00000123`
  3. 재부팅 후 BLE 이름 "BDC00000123" 광고 + 0xB2 → "00000123"
  4. 0xB1 재전송 → NAK(locked) — write-once 확인

- [ ] **Step 6: 커밋** `feat(ble): 0xB1/0xB2 set/get serial via boot-commit`

## Task 5 (후속): BLE FW 인벤토리 보고

- [ ] BLE FuncID(예 0xB3 GetFwInfo)로 `devinfo_get_fw()` 기반 boot/app/recovery 버전·종류 응답 (FOTA 서버 연동).

---

## 결정 반영
- ✅ **전부 NOR** `device_info`(시리얼 + FW 테이블) — eMMC 미사용
- ✅ 명명: `preamble`(not magic), `app_no`, `valid`=0xAA
- ✅ 시리얼: `BDC`(고정) + 8자리 suffix, write-once(`serial_locked`)
- ✅ Appli read=XIP 직접 / Boot write=indirect, 시리얼 설정은 **BKPSRAM→reset→Boot 커밋**

## 트레이드오프 (NOR 일원화의 비용)
- 시리얼 설정에 **reset 1회** 필요(Boot이 NOR 커밋). 제조 1회성이라 무방.
- 장점: 단일 진실원천(NOR), eMMC 포맷/교체와 무관하게 시리얼·FW정보 보존.

## 미정
- `0xB2 GetSerial`/`0xB3 GetFwInfo` 응답 payload 포맷 — 응답 빌더 시그니처 확인 후 확정.
- `BOOT_FW_VERSION` 상수 위치/형식.

## 권장 진행 순서
Task 1 → 2 → 3 → 4 (시리얼 E2E) → 5 (FW 인벤토리).
