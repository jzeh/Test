# OTA P0 수정: Dual-Slot ICF 전환 + IWDG 활성화 구현 플랜

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Dual-slot OTA를 실제 동작 가능 상태로 만들기 위해 (1) Appli를 Slot 0(`0x90010000`)에 적재하도록 링커/부트 점프 오프셋을 정합화하고, (2) IWDG를 활성화하여 "불량 펌웨어 hang → 리셋 → fail_count 증가" 복구 모델의 전제를 성립시킨다.

**Architecture:** Bootloader는 내장 플래시(`0x08000000`)에서 실행되며 외부 NOR Flash의 Slot 0(`0x90010000`)로 점프한다. 메타데이터는 `0x90000000`(Primary)/`0x97FF0000`(Backup)에 위치하므로 Appli intvec가 `0x90010000`로 이동해야 메타 영역과 겹치지 않는다. IWDG는 Boot에서 시작해 점프를 건너 살아있고, Appli는 FreeRTOS idle hook에서 갱신한다. Hang 시 갱신 누락 → IWDG 리셋 → Boot가 fail_count 증가.

**Tech Stack:** STM32H7S3I8, IAR EWARM (ICF 링커), STM32 HAL (IWDG/XSPI), ST EXTMEM Manager (XIP), FreeRTOS, FATFS/eMMC.

**검증 방식 주의:** 타깃 보드 펌웨어이므로 host 단위테스트가 없다. 각 Task의 "테스트"는 **빌드 → 플래시 → UART7(115200) 시리얼 로그 + BKPSRAM breadcrumb 관찰**로 수행한다. Boot 로그는 `printf`로 UART7에 출력되고, breadcrumb은 ST-Link로 `0x38800430`(Boot)/`0x38800431`(Appli)을 읽어 확인한다.

**⚠ 절대 규칙:** Task 1(ICF)과 Task 2(XIP offset)는 **반드시 같은 플래시 사이클에 함께 반영**한다. 한쪽만 반영하면 즉시 부팅 불능(brick)이 된다. 두 Task가 끝나기 전에는 보드에 플래시하지 않는다.

---

## File Structure

| 파일 | 책임 | 변경 |
|---|---|---|
| `EWARM/Appli/EVCD_Test_Appli.ewp` | Appli 링커 ICF 선택 | Modify — ICF를 `_slot0.icf`로 |
| `EWARM/Boot/EVCD_Test_Boot.ewp` | Boot 전처리기 정의 | Modify — `EXTMEM_XIP_IMAGE_OFFSET=0x10000` 추가 |
| `EWARM/Appli/stm32h7rsxx_ROMxspi1_slot0.icf` | Slot 0 메모리 배치 | 검증만 (이미 존재) |
| `Appli/Core/Inc/stm32h7rsxx_hal_conf.h` | HAL IWDG 모듈 활성 | Modify |
| `Boot/Core/Inc/stm32h7rsxx_hal_conf.h` | HAL IWDG 모듈 활성 | Modify |
| `Appli/Core/Src/main.c` | Appli IWDG 인스턴스/초기화 | Modify |
| `Appli/Core/Src/freertos.c` | idle hook에서 IWDG 갱신 | Modify |
| `Boot/Core/Src/main.c` | Boot IWDG 인스턴스/초기화 | Modify |
| `EWARM/Appli/EVCD_Test_Appli.ewp` | Appli `FW_COMPAT_USE_IWDG=1` 정의 | Modify |
| `EWARM/Boot/EVCD_Test_Boot.ewp` | Boot `FW_COMPAT_USE_IWDG=1` 정의 | Modify |

---

## Part A — Dual-Slot ICF/XIP 정합화 (P0 #1)

### Task 1: Appli 링커를 Slot 0 ICF로 전환

**Files:**
- Modify: `EWARM/Appli/EVCD_Test_Appli.ewp:801-802` (IlinkIcfFile state)
- Verify: `EWARM/Appli/stm32h7rsxx_ROMxspi1_slot0.icf` (intvec `0x90010000`)

- [ ] **Step 1: 현재 ICF 설정 확인**

Run:
```bash
grep -n "IlinkIcfOverride\|IlinkIcfFile" EWARM/Appli/EVCD_Test_Appli.ewp | head
```
Expected: `IlinkIcfFile` state가 `$PROJ_DIR$/stm32h7rsxx_ROMxspi1.icf` (offset 0 버전).

- [ ] **Step 2: slot0 ICF의 배치 주소 검증**

Run:
```bash
grep -nE "intvec_start__|region_ROM_(start|end)" EWARM/Appli/stm32h7rsxx_ROMxspi1_slot0.icf
```
Expected:
```
__ICFEDIT_intvec_start__     = 0x90010000;
__ICFEDIT_region_ROM_start__ = 0x90010000;
__ICFEDIT_region_ROM_end__   = 0x93FFFFFF;
```
이 값이면 메타 Primary(`0x90000000`, 64KB)와 겹치지 않고 Slot 1(`0x94000000`) 직전까지 사용.

- [ ] **Step 3: ewp의 IlinkIcfFile 경로 교체**

`EWARM/Appli/EVCD_Test_Appli.ewp`에서:
```xml
<state>$PROJ_DIR$/stm32h7rsxx_ROMxspi1.icf</state>
```
를 다음으로 변경:
```xml
<state>$PROJ_DIR$/stm32h7rsxx_ROMxspi1_slot0.icf</state>
```
그리고 같은 옵션 그룹의 `IlinkIcfOverride` state가 `1`인지 확인(아니면 `1`로 설정). IAR IDE에서 작업 시: Project → Options → Linker → Config → "Override default" 체크 후 `stm32h7rsxx_ROMxspi1_slot0.icf` 선택.

- [ ] **Step 4: 변경 확인 (아직 빌드/플래시 금지 — Task 2와 함께 진행)**

Run:
```bash
grep -n "stm32h7rsxx_ROMxspi1_slot0.icf" EWARM/Appli/EVCD_Test_Appli.ewp
```
Expected: 1건 매칭. **여기서 멈춘다 — Task 2 완료 전 플래시하면 brick.**

- [ ] **Step 5: 커밋**

```bash
git add EWARM/Appli/EVCD_Test_Appli.ewp
git commit -m "build(appli): switch linker to Slot 0 ICF (intvec 0x90010000)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 2: Boot XIP 점프 오프셋을 0x10000으로 설정

**Files:**
- Modify: `EWARM/Boot/EVCD_Test_Boot.ewp:229-233` (CCDefines)
- Reference: `Middlewares/ST/STM32_ExtMem_Manager/boot/stm32_boot_xip.c:152` (점프 주소 계산)

- [ ] **Step 1: 점프 주소 계산식 확인**

Run:
```bash
grep -nE "EXTMEM_XIP_IMAGE_OFFSET|Application_vector \+=|GetMapAddress" Middlewares/ST/STM32_ExtMem_Manager/boot/stm32_boot_xip.c
```
Expected: `Application_vector += EXTMEM_XIP_IMAGE_OFFSET + EXTMEM_HEADER_OFFSET;` — 기본 `EXTMEM_XIP_IMAGE_OFFSET=0`이므로 현재는 `0x90000000`로 점프(잘못됨). `0x10000`을 더해야 `0x90010000`(Slot 0)로 점프.

- [ ] **Step 2: Boot CCDefines에 매크로 추가**

`EWARM/Boot/EVCD_Test_Boot.ewp`의 CCDefines 옵션(현재 `BOOT_BUILD`, `FW_UPDATE_ENABLED` 정의 위치, line 232-233)에 한 줄 추가:
```xml
                    <state>BOOT_BUILD</state>
                    <state>FW_UPDATE_ENABLED</state>
                    <state>EXTMEM_XIP_IMAGE_OFFSET=0x10000</state>
```
IAR IDE: Boot 프로젝트 → Options → C/C++ Compiler → Preprocessor → Defined symbols에 `EXTMEM_XIP_IMAGE_OFFSET=0x10000` 추가.

- [ ] **Step 3: 매크로 반영 확인**

Run:
```bash
grep -n "EXTMEM_XIP_IMAGE_OFFSET=0x10000" EWARM/Boot/EVCD_Test_Boot.ewp
```
Expected: 1건 매칭.

- [ ] **Step 4: Boot + Appli 빌드 (둘 다 무에러)**

IAR에서 Boot 프로젝트와 Appli 프로젝트를 각각 Rebuild All.
Expected: 두 프로젝트 모두 0 errors. Appli 맵 파일에서 `.intvec`가 `0x90010000`에 배치됐는지 확인:
```bash
grep -nE "\.intvec|0x90010000" EWARM/Appli/*/List/*.map 2>/dev/null | head
```

- [ ] **Step 5: 플래시 + 부팅 검증 (Task 1+2 통합 검증)**

플래시 절차(아래 "구동 방법" §1 참조):
1. ExtMemLoader로 Appli `.bin`을 외부 NOR `0x90010000`에 기록
2. ST-Link로 Boot `.out`을 내장 플래시 `0x08000000`에 기록
3. UART7(115200) 시리얼 모니터 연결 후 리셋

Expected 시리얼 로그:
```
BOOTLOADER - START
fw_flag=0x00  fail_count=0  recovery_phase=0
[XSPI] init OK, size=128MB ...
[SLOT] ... (Primary invalid → defaults 또는 OK)
```
그리고 Appli가 정상 부팅(LCD/BLE 기동, MCU_ALIVE_LED 깜빡임). HardFault/무한리셋이 없어야 함.

- [ ] **Step 6: breadcrumb로 점프 성공 확인**

ST-Link 또는 CLI로 BKPSRAM 확인:
- `0x38800430` (CHK_BOOT) == `0x40`(APP_ENTER) 이상 도달
- `0x38800431` (CHK_APPLI) == `0x90`(TASK_ENTRY) 도달 → Appli가 `0x90010000`에서 정상 실행 중

- [ ] **Step 7: 커밋**

```bash
git add EWARM/Boot/EVCD_Test_Boot.ewp
git commit -m "build(boot): set EXTMEM_XIP_IMAGE_OFFSET=0x10000 to jump into Slot 0

Pairs with Appli Slot 0 ICF switch. Boot now jumps to 0x90010000.

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 3: 메타 영역 비파괴 회귀 검증

ICF 전환 후 Boot의 `slot_manager_init()`이 메타를 `0x90000000`/`0x97FF0000`에 써도 Appli(`0x90010000~`)를 손상시키지 않음을 확인한다.

**Files:**
- Reference: `Common/Src/fw_slot_manager.c:90-116` (meta_create_default / meta_flush)

- [ ] **Step 1: 콜드 부팅 2회 반복**

보드 전원을 완전히 내렸다가 켜는 콜드 부팅을 2회 수행. 1회차는 메타 default 생성, 2회차는 Primary 로드 경로를 타야 함.

- [ ] **Step 2: 로그로 경로 분기 확인 (FW_COMPAT_VERBOSE=1 필요 — Task 8에서 활성)**

Expected 1회차: `[SLOT] Both invalid, creating defaults` 또는 `Primary invalid, trying Backup`.
Expected 2회차: `[SLOT] Primary OK (...)`.
두 경우 모두 Appli가 정상 부팅(LED 깜빡임)하면 메타-앱 비충돌 확인 완료.

- [ ] **Step 3: 회귀 없음 확인 (커밋 불필요 — 검증 전용)**

기존 3가지 제어모드(차상충전/차상방전/배터리팩방전)가 정상 동작하는지 BLE APP으로 1회씩 확인. 코드 변경이 없으므로 이상 시 ICF 배치 문제로 의심.

---

## Part B — IWDG 활성화 (P0 #2)

### Task 4: Boot HAL IWDG 모듈 활성화

**Files:**
- Modify: `Boot/Core/Inc/stm32h7rsxx_hal_conf.h:59`

- [ ] **Step 1: HAL_IWDG_MODULE_ENABLED 주석 해제**

`Boot/Core/Inc/stm32h7rsxx_hal_conf.h`에서:
```c
/* #define HAL_IWDG_MODULE_ENABLED   */
```
를:
```c
#define HAL_IWDG_MODULE_ENABLED
```

- [ ] **Step 2: 빌드로 헤더 링크 확인**

Boot Rebuild. Expected: `stm32h7rsxx_hal_iwdg.h`가 포함되어 0 errors.

- [ ] **Step 3: 커밋**

```bash
git add Boot/Core/Inc/stm32h7rsxx_hal_conf.h
git commit -m "build(boot): enable HAL IWDG module

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 5: Boot에 IWDG 인스턴스 + 초기화 추가

IWDG를 부팅 초기에 시작하여 점프 이후까지 살려두고, 긴 복사/검증 루프는 기존 `FW_IWDG_REFRESH()`(Task 8에서 실효화)로 갱신한다.

**Files:**
- Modify: `Boot/Core/Src/main.c:47` (전역 핸들), `Boot/Core/Src/main.c:62` (프로토타입), `Boot/Core/Src/main.c:225-236` (init 호출)

- [ ] **Step 1: 전역 IWDG 핸들 선언**

`Boot/Core/Src/main.c`의 `XSPI_HandleTypeDef hxspi1;`(line 49) 아래에 추가:
```c
IWDG_HandleTypeDef hiwdg;
```
(이름 `hiwdg`는 `fw_compat.h:62`의 `extern IWDG_HandleTypeDef hiwdg;`와 정확히 일치해야 함.)

- [ ] **Step 2: MX_IWDG_Init 프로토타입 추가**

`static void MX_XSPI1_Init(void);`(line 60) 아래에 추가:
```c
static void MX_IWDG_Init(void);
```

- [ ] **Step 3: MX_IWDG_Init 구현 추가**

`MX_XSPI1_Init` 함수 구현 뒤(line 469 이후)에 추가. 타임아웃은 약 8초로 설정(LSI 32kHz, prescaler 256, reload 1000 → 약 8s). 긴 eMMC 읽기/플래시 복사 stall에도 per-chunk 갱신으로 충분히 커버.
```c
/**
  * @brief IWDG Initialization Function — 약 8초 타임아웃.
  *        Boot에서 시작해 점프 이후 Appli idle hook이 갱신을 인계.
  */
static void MX_IWDG_Init(void)
{
  hiwdg.Instance       = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_256;   /* 32kHz/256 = 125Hz (8ms/tick) */
  hiwdg.Init.Window    = IWDG_WINDOW_DISABLE;
  hiwdg.Init.Reload    = 1000;                 /* 1000 * 8ms ≈ 8.0s */
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
}
```

- [ ] **Step 4: SystemClock 직후 IWDG 시작**

`BackupSRAM_Init();`(line 225) 호출 직후에 추가:
```c
  MX_IWDG_Init();                /* 워치독 시작 — 이후 모든 경로에서 주기 갱신 필요 */
```
주의: IWDG는 한 번 시작하면 멈출 수 없다. 이후 Boot의 모든 blocking 경로(eMMC 마운트, 파일 읽기, 슬롯 복사)가 8초 내 갱신되어야 한다. 복사/검증 루프는 `FW_IWDG_REFRESH()`가 이미 삽입되어 있고(Task 8에서 실효화), eMMC 마운트 직전/직후는 Step 5에서 보강한다.

- [ ] **Step 5: eMMC 마운트 전후 명시적 갱신 보강**

`Boot/Core/Src/boot_emmc.c`의 `Boot_InitEmmc()`에서 `f_mount` 호출(line 160) 직전에 갱신 한 줄 추가(헤더 `fw_compat.h`는 이미 include됨):
```c
    FW_IWDG_REFRESH();
    if (f_mount(&s_boot_fatfs, "", 1) != FR_OK) {
```

- [ ] **Step 6: 빌드 확인 (아직 동작은 no-op — FW_COMPAT_USE_IWDG 미정의)**

Boot Rebuild. Expected: 0 errors. 이 시점 `FW_IWDG_REFRESH()`는 여전히 no-op이지만 `MX_IWDG_Init`은 실제 IWDG를 시작하므로, Task 8 전에 단독 플래시하면 8초 후 리셋될 수 있음 → **Task 8까지 함께 진행 후 플래시.**

- [ ] **Step 7: 커밋**

```bash
git add Boot/Core/Src/main.c Boot/Core/Src/boot_emmc.c
git commit -m "feat(boot): init IWDG (~8s) and refresh around eMMC mount

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 6: Appli HAL IWDG 모듈 활성화

**Files:**
- Modify: `Appli/Core/Inc/stm32h7rsxx_hal_conf.h:59`

- [ ] **Step 1: HAL_IWDG_MODULE_ENABLED 주석 해제**

`Appli/Core/Inc/stm32h7rsxx_hal_conf.h`에서:
```c
/* #define HAL_IWDG_MODULE_ENABLED   */
```
를:
```c
#define HAL_IWDG_MODULE_ENABLED
```

- [ ] **Step 2: 빌드 확인**

Appli Rebuild. Expected: 0 errors.

- [ ] **Step 3: 커밋**

```bash
git add Appli/Core/Inc/stm32h7rsxx_hal_conf.h
git commit -m "build(appli): enable HAL IWDG module

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 7: Appli IWDG 핸들 인계 + idle hook 갱신

Boot이 시작한 IWDG는 점프 후에도 계속 카운트한다. Appli는 핸들을 재구성(`hiwdg.Instance=IWDG`)하여 갱신 권한을 잡고, FreeRTOS idle hook에서 주기 갱신한다. 정상 동작 시 idle이 자주 돌아 갱신되고, 어느 태스크가 hang하면 idle이 굶어 갱신 누락 → 리셋.

**Files:**
- Modify: `Appli/Core/Src/main.c` (전역 핸들 + 재구성), `Appli/Core/Src/freertos.c:62-73` (idle hook)

- [ ] **Step 1: Appli 전역 IWDG 핸들 선언**

`Appli/Core/Src/main.c`의 주변장치 핸들 선언부(다른 `*_HandleTypeDef` 전역 옆)에 추가:
```c
IWDG_HandleTypeDef hiwdg;
```
(`fw_compat.h`의 `extern`과 이름 일치.)

- [ ] **Step 2: Appli에서 IWDG 핸들 재구성**

`Appli/Core/Src/main.c`의 하드웨어 초기화 단계(HAL_Init 이후, 스케줄러 시작 전, `BackupSRAM_Init()` 부근)에 추가. Boot이 이미 IWDG를 켰으므로 `HAL_IWDG_Init`을 재호출해도 무방하나, 안전하게 핸들만 바인딩하고 즉시 1회 갱신:
```c
  hiwdg.Instance = IWDG;          /* Boot이 시작한 IWDG 핸들 인계 */
  HAL_IWDG_Refresh(&hiwdg);       /* 인계 직후 1회 갱신 */
```
주의: Appli 초기화(eMMC 마운트 등)가 8초를 넘기지 않아야 한다. 넘길 우려가 있으면 `SYS_init()` 내 장시간 구간에 `HAL_IWDG_Refresh(&hiwdg);`를 추가한다.

- [ ] **Step 3: idle hook에서 IWDG 갱신**

`Appli/Core/Src/freertos.c`의 `vApplicationIdleHook`(line 62) 본문에 추가:
```c
void vApplicationIdleHook( void )
{
  /* IWDG 갱신: idle 태스크가 도는 한(=모든 태스크가 굶지 않는 한) 워치독 유지.
     어느 태스크가 busy-hang하면 idle이 실행되지 못해 갱신 누락 → IWDG 리셋. */
  extern IWDG_HandleTypeDef hiwdg;
  HAL_IWDG_Refresh(&hiwdg);
}
```

- [ ] **Step 4: 빌드 확인**

Appli Rebuild. Expected: 0 errors.

- [ ] **Step 5: 커밋**

```bash
git add Appli/Core/Src/main.c Appli/Core/Src/freertos.c
git commit -m "feat(appli): take over IWDG and refresh in idle hook

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 8: FW_COMPAT_USE_IWDG 활성화 (Boot + Appli)

`fw_compat.h`의 `FW_IWDG_REFRESH()`를 실효화하여 긴 플래시/복사 루프가 실제로 워치독을 갱신하도록 한다.

**Files:**
- Modify: `EWARM/Boot/EVCD_Test_Boot.ewp` (CCDefines), `EWARM/Appli/EVCD_Test_Appli.ewp` (CCDefines)
- Reference: `Common/Inc/fw_compat.h:57-66`

- [ ] **Step 1: Boot CCDefines에 추가**

`EWARM/Boot/EVCD_Test_Boot.ewp` CCDefines에:
```xml
                    <state>FW_COMPAT_USE_IWDG=1</state>
```
(원하면 디버깅용 `FW_COMPAT_VERBOSE=1`도 함께 추가하여 `[SLOT]`/`[STAGE]` 로그 활성화 — Task 3 검증에 유용.)

- [ ] **Step 2: Appli CCDefines에 추가**

`EWARM/Appli/EVCD_Test_Appli.ewp` CCDefines(line 230-231, `USE_HAL_DRIVER`/`STM32H7S3xx` 옆)에:
```xml
          <state>FW_COMPAT_USE_IWDG=1</state>
```

- [ ] **Step 3: 정의 반영 확인**

Run:
```bash
grep -n "FW_COMPAT_USE_IWDG=1" EWARM/Boot/EVCD_Test_Boot.ewp EWARM/Appli/EVCD_Test_Appli.ewp
```
Expected: 2건(각 파일 1건).

- [ ] **Step 4: 전체 리빌드 + 통합 플래시 + 검증**

Boot/Appli 모두 Rebuild → 플래시(아래 §1) → 리셋. Expected:
- 정상 부팅 후 장시간(>30초) 무리셋 동작(idle hook 갱신 정상)
- OTA 1사이클(아래 §2) 수행 중 Boot 복사/검증 구간(수 초)에 리셋 없음(`FW_IWDG_REFRESH` 실효 확인)

- [ ] **Step 5: 의도적 hang 복구 검증 (핵심 수용 테스트)**

임시 디버그 코드로 hang을 주입하여 복구 모델을 검증한다. 예: CLI 명령 핸들러에 `while(1){}` 추가 후 호출, 또는 한 태스크에서 인터럽트 비활성+무한루프.
Expected:
1. hang 발생 → 약 8초 후 IWDG 리셋
2. Boot 재진입 → 로그 `[BOOT] BOOT_TEST fail_count=N/3` (단, 이 시나리오는 fw_flag=VERIFIED 상태에서는 fail_count가 안 오를 수 있음 — hang 검증은 "리셋이 실제로 발생하는가"가 핵심)
3. 리셋이 발생하면 IWDG 동작 확인 완료. **검증 후 디버그 hang 코드 반드시 제거.**

- [ ] **Step 6: 커밋**

```bash
git add EWARM/Boot/EVCD_Test_Boot.ewp EWARM/Appli/EVCD_Test_Appli.ewp
git commit -m "build: enable FW_COMPAT_USE_IWDG so flash loops refresh watchdog

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Part C — 통합 수용 테스트

### Task 9: End-to-End OTA 사이클 검증

**Files:** (변경 없음 — 검증 전용)

- [ ] **Step 1: 초기 상태 확인**

리셋 후 시리얼: `fw_flag=0x00(IDLE)`, Appli 정상 부팅, SystemState IDLE.

- [ ] **Step 2: BLE APP으로 OTA 5단계 수행**

`0xA1`(START, app_no+파일명) → `0xA2`(청크 반복) → `0xA3`(END, total size) → `0xA4`(CHECK, CRC32) → `0xA5`(SET_LIST, boot_app+ver). 각 단계 ACK 수신 확인. 0xA5 후 자동 리셋.

- [ ] **Step 3: Boot 스테이징 로그 확인**

Expected(VERBOSE 활성 시):
```
fw_flag=0xB4(COPY_PENDING)
[STAGE] OK — 01_Application/<bin> (NNNN B, CRC=0x........)
[SLOT] Copy complete (NNNN bytes), BOOT_TEST set
```

- [ ] **Step 4: 자가검증 전이 확인**

새 Appli 부팅 후 IDLE 도달 시 시리얼: `[FW] BOOT_TEST -> VERIFIED (self-test OK)`. BKPSRAM `fw_flag`(`0x38800410`)가 `0xC3`(VERIFIED), `fail_count`(`0x38800414`)가 `0`.

- [ ] **Step 5: 새 펌웨어 버전/기능 확인**

LCD/CLI에서 펌웨어 버전이 갱신됐고 3가지 제어모드가 정상 동작하는지 확인. 통과 시 OTA 경로 전체 정상.

---

## 구동(Flashing/Run) 방법

### §1. 클린 보드 최초 플래시 (제조/개발 부트스트랩)

외부 NOR는 ST-Link로 직접 못 굽고 **ExtMemLoader**(외부 메모리 로더)를 거친다. 본 프로젝트에 `EWARM/ExtMemLoader/`가 이미 빌드되어 있다.

1. **ExtMemLoader 빌드(최초 1회):** IAR에서 `EWARM/ExtMemLoader/EVCD_Test_ExtMemLoader.ewp` 열고 Rebuild → `.out` 산출. (STM32CubeProgrammer의 External Loader로 등록되는 `*.stldr`도 동일 산출물 계열.)
2. **Appli → 외부 NOR `0x90010000`:**
   - STM32CubeProgrammer 실행 → ST-Link 연결
   - External loader 탭에서 위 ExtMemLoader 선택(Macronix XSPI1)
   - Appli `.bin`을 **Start address `0x90010000`** 로 Download. (`.out`/`.hex`는 주소가 내장돼 있으나, `.bin`은 반드시 `0x90010000` 명시.)
3. **Boot → 내장 플래시 `0x08000000`:**
   - External loader 불필요(내장 플래시)
   - Boot `.out`(또는 `.hex`)을 Download → 자동으로 `0x08000000`.
4. **부팅 모드 핀:** STM32H7S는 BOOT0 설정에 따라 내장 플래시(`0x08000000`)에서 부팅하도록 옵션바이트/BOOT0 확인. Boot이 먼저 실행되어야 함.
5. **eMMC 준비:** OTA 테스트 전 eMMC에 `FirmwareInfo.ini`(preamble `"EVCD"`=0x45564344, mucBootMode=1) + `01_Application/<bin>`을 USB MSC 모드로 미리 적재해두면 Boot 스테이징 경로도 검증 가능. (콜드 부트스트랩만이면 §1-2의 직접 굽기로 충분.)
6. 리셋 → UART7(115200,8N1) 시리얼 모니터로 `BOOTLOADER - START` 확인.

### §2. OTA(현장 업데이트) 경로

ST-Link 없이 BLE APP만으로 갱신:
1. APP이 `.bin`을 `0xA1~0xA5` 프로토콜로 전송 → Appli가 eMMC `01_Application/`에 저장 + CRC32 검증
2. `0xA5` 수신 시 `fw_flag=COPY_PENDING` 후 자동 리셋
3. Boot이 eMMC → Slot 1 스테이징 → Slot 0 복사·검증 → `BOOT_TEST` 후 새 Appli 점프
4. 새 Appli가 IDLE 도달 → `VERIFIED` 자가확정

### §3. 롤백/디버그 시 안전수칙

- **brick 방지:** Part A 두 Task(ICF + XIP offset)는 항상 함께 플래시. 의심 시 `git diff`로 두 변경이 모두 있는지 확인.
- **회귀 복구:** 기존 무-OTA 동작으로 되돌리려면 Appli ICF를 `stm32h7rsxx_ROMxspi1.icf`(offset 0)로, Boot에서 `EXTMEM_XIP_IMAGE_OFFSET` 정의 제거 후 둘 다 플래시.
- **IWDG 디버깅 방해:** ST-Link 중단점(halt) 중 IWDG가 리셋시킬 수 있음. 디버그 시 IAR에서 IWDG의 "Stop in debug" (DBGMCU `DBG_IWDG_STOP`) 옵션 활성 권장.
- **BKPSRAM 관찰:** 부팅 단계 추적은 `0x38800430`(Boot)/`0x38800431`(Appli) breadcrumb, 상태는 `0x38800410`(fw_flag)/`0x38800414`(fail_count) 직접 읽기.

---

## Self-Review 메모

- **Spec coverage:** P0 #1(ICF Task 1 + XIP offset Task 2 + 회귀검증 Task 3), P0 #2(HAL 활성 Task 4·6 + Boot init Task 5 + Appli 인계/idle Task 7 + shim 실효 Task 8), 구동방법(§1~§3), E2E(Task 9) 모두 포함.
- **이름 일치:** 전역 핸들 `hiwdg`는 Boot/Appli/`fw_compat.h`(extern)에서 동일. `FW_IWDG_REFRESH()`/`FW_COMPAT_USE_IWDG`/`EXTMEM_XIP_IMAGE_OFFSET` 철자 일치 확인.
- **주소 일치:** intvec `0x90010000` = `EXEC_SLOT_XIP_ADDR`(fw_update.h) = slot0 ICF region start = base(`0x90000000`)+offset(`0x10000`). 정합.
- **순서 의존:** Task 1↔2 동시 플래시, Task 5·7↔8 동시 플래시 규칙 명시.
