# CAN Bit Timing Configuration Guide
## STM32H7RS FDCAN1 — BSA + 진단기 공유 버스 환경

> **본 문서의 목적**: CAN bit timing 이론 정리 + 본 프로젝트의 FDCAN1 설정 근거 +
> 진단기 BUS_OFF 이슈 해결 내용 + 향후 튜닝 가이드.

---

## 1. CAN Bit Timing 기본 이론

### 1.1 한 비트의 구성

CAN 버스에서 1 비트는 시간적으로 **여러 개의 Time Quanta (TQ)** 로 나뉜다.

```
┌─────────────────────── 1 bit time ───────────────────────┐
│                                                          │
├──SYNC──┬─────── PROP_SEG + PHASE_SEG1 ─────┬── PHASE_SEG2┤
│  1 TQ  │              nbs1 TQ              │    nbs2 TQ  │
└────────┴───────────────────────────────────┴─────────────┘
         ↑                                  ↑              ↑
         Edge 동기화 기준점                 Sample Point   다음 비트 시작
                                            (비트 읽는 시점)
```

| 세그먼트 | STM32 HAL 필드명 | 역할 |
|---|---|---|
| **SYNC_SEG** | 고정 1 TQ | 비트 경계 동기화. 모든 노드가 여기서 edge 를 기다림 |
| **PROP_SEG + PHASE_SEG1** | `NominalTimeSeg1` (nbs1) | 신호 전파 지연 보상 + 비트 안정화 시간 |
| **PHASE_SEG2** | `NominalTimeSeg2` (nbs2) | Sample 후 다음 edge 까지의 마진 |
| **SJW** | `NominalSyncJumpWidth` | Edge 가 어긋날 때 보정 가능한 최대 TQ 수 |

총 TQ = 1 + nbs1 + nbs2

### 1.2 비트레이트 계산

```
1 bit time = (1 + nbs1 + nbs2) × Prescaler / FDCAN_clock
Bit Rate   = FDCAN_clock / [Prescaler × (1 + nbs1 + nbs2)]
```

**본 프로젝트** (FDCAN_clock = 80 MHz):
| 설정 | Prescaler | 총 TQ | Bit Rate |
|---|---|---|---|
| 500 kbps | 10 | 16 | 80M / (10×16) = 500 kbps ✓ |
| 1 Mbps | 5 | 16 | 80M / (5×16) = 1 Mbps ✓ |
| 250 kbps | 20 | 16 | 80M / (20×16) = 250 kbps ✓ |

### 1.3 Sample Point — 가장 중요한 개념

각 노드의 수신부가 **버스 신호를 읽는 시점**.

```
Sample Point = (1 + nbs1) / (1 + nbs1 + nbs2)
             = SYNC + PROP+PHASE1 까지의 누적 / 전체
```

**예시 (16 TQ 환경)**:
| nbs1 / nbs2 | 계산 | Sample Point |
|---|---|---|
| 12 / 3 | (1+12)/16 | 81.25% |
| 11 / 4 | (1+11)/16 | **75.0%** |
| 13 / 2 | (1+13)/16 | 87.5% |
| 10 / 5 | (1+10)/16 | 68.75% |

### 1.4 Sample Point 의 의미

Sample Point 80% 라는 건 1 비트 시간(예: 500kbps 면 2µs) 중 **앞에서부터 80% 지난 1.6µs 시점**에 비트 레벨(dominant/recessive) 을 측정한다는 뜻.

```
시간 →

[송신 노드: dominant 비트 송신]
0       0.5µs       1.0µs       1.5µs       2.0µs
│        │           │           │           │
├════════════════════════════════│        ← dominant 유지 시간
                                  ↑
                          Sample Point = 80%
                          (1.6µs 시점에 sample)

[수신 노드: 신호 안정 구간에서 sample]
                                  ↑
                          여기서 안정한 dominant 읽음 → OK
```

---

## 2. Sample Point Mismatch 의 위험성

### 2.1 다른 노드가 다른 Sample Point 사용 시

```
[노드 A: Sample Point 75%, 송신측]
0   0.25µs   0.5µs   0.75µs   1.0µs   1.25µs   1.5µs   1.75µs   2.0µs
│    │       │       │        │       │        │       │        │
├════════════════════════════│ ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│  (송신 비트)
                              ↑
                              자기 sample (75%, 1.5µs)

[노드 B: Sample Point 81.25%, 같은 비트 수신]
                                       ↑
                                       1.625µs 시점 sample
                                       (~125ns 늦음)
```

**문제 발생 시나리오**:
- 송신 노드가 1.5µs 부터 다음 비트 전환 시작
- 노드 B 가 1.625µs 에 sample → **전환 중 불안정한 값** 읽음
- Stuff bit / Form / Bit 에러로 오인 → **active error frame 송신** (6 dominant bits)
- 송신 노드 입장에서 자기 valid frame 이 corrupt → TEC + 8
- 반복 → **TEC ≥ 256 → BUS_OFF**

### 2.2 실제 시장의 Sample Point 분포

| 표준 / 도메인 | 권장 Sample Point | 비고 |
|---|---|---|
| Bosch CAN spec (1991, 초기) | **75%** | 가장 보수적, 모든 환경 호환 |
| ISO 11898-2:2003 | 75 ~ 87.5% | 광범위 허용 |
| CiA-301 (CANopen) | 87.5% | 고속 산업 자동화 |
| SAE J1939 (트럭/버스) | 87.5% | 250 kbps 환경 |
| **자동차 OBD-II / UDS 진단 도구** | **75%** | ELM327, Vector, Peak 등 다수 |
| **자동차 OEM ECU** | 75 ~ 80% | 차종/제조사별 다름 |

→ **자동차 진단 환경에선 75% 가 사실상 표준.** 차량 ECU 및 진단기와 통신하려면 우리도 75% 채택이 안전.

---

## 3. 본 프로젝트의 변경 이력 및 근거

### 3.1 이전 설정 (문제 발생 상태)

[task-can.c](Appli/Function/Src/task-can.c) `CAN_set_baud()`:

```c
nbs1 = 12;
nbs2 = 3;
/* Sample Point = (1+12)/16 = 81.25% */
```

**증상**: 같은 CAN1 라인에 연결된 진단기에서 **간헐적 BUS_OFF** 발생.

### 3.2 Root Cause

- BSA 가 FDCAN1 init 후 BSA TX 시작
- 우리 FDCAN1 의 Sample Point 81.25% vs 진단기의 75%
- **125ns 차이** (500kbps 의 2µs 비트 시간 중 6.25%)
- 진단기 송신 비트의 transition 직전 / 직후 sample → 우리 FDCAN1 이 stuff/form/bit error 오인식
- 우리 FDCAN1 (Error Active 상태) → active error frame (6 dominant bits) 송신
- 진단기 송신 중인 frame 이 corrupt → 진단기 TEC + 8
- 진단기 재전송 → 동일 반복
- 진단기 **TEC ≥ 256 → BUS_OFF**

### 3.3 변경 후 설정 (해결)

```c
nbs1 = 11;   /* 12 → 11 */
nbs2 = 4;    /* 3  → 4  */
/* Sample Point = (1+11)/16 = 75% */
```

**효과**:
- 진단기와 동일 75% sample point → 동일 시점에 비트 sample
- Stuff / Form / Bit error 오인식 빈도 격감
- 우리 FDCAN1 이 error frame 발사 안 함
- 진단기 TEC 누적 차단 → BUS_OFF 발생 빈도 격감

**비트레이트 유지**: 총 TQ = 1 + 11 + 4 = 16 → Prescaler=10 그대로 → 500 kbps 동일 ✓

---

## 4. 데이터 페이즈 (CAN FD BRS) 비트 타이밍

### 4.1 데이터 페이즈의 역할

CAN FD BRS 프레임은 **arbitration 부분은 nominal speed, 데이터 부분은 고속** 으로 전환:

```
SOF │ ID │ ... │ BRS=1 │ ESI │ DLC │ Data │ CRC │ ACK │ EOF
└── nominal (예: 500 kbps) ──┘└── data (예: 2 Mbps) ──┘└─ nominal ─┘
                          ↑                          ↑
                          BRS 비트에서 전환            EOF 전 nominal 복귀
```

### 4.2 본 프로젝트 데이터 페이즈 설정

```c
dbs1 = 15;
dbs2 = 4;
/* 데이터 페이즈 Sample Point = (1+15)/20 = 80% */
```

| Data baud | Prescaler | 총 TQ | Bit Rate | Sample Point |
|---|---|---|---|---|
| 1 Mbps | 4 | 20 | 80M/(4×20) = 1 Mbps | 80% |
| **2 Mbps** | 2 | 20 | 80M/(2×20) = 2 Mbps | **80%** |
| 4 Mbps | 1 | 20 | 80M/(1×20) = 4 Mbps | 80% |

### 4.3 데이터 페이즈 Sample Point 의 권장값

| 데이터레이트 | 비트 시간 | 트랜시버 지연 비율 | 권장 SP |
|---|---|---|---|
| 1 Mbps | 1000 ns | ~15% | 75 ~ 80% |
| **2 Mbps** | 500 ns | ~30% | **70 ~ 80%** |
| 4 Mbps | 250 ns | ~60% | **65 ~ 75%** |

**CiA-601-2 권장**: CAN FD 데이터 페이즈 SP = 70 ~ 80%.

→ 현재 80% 는 **2 Mbps 까지는 안정**. 4 Mbps 사용 시 75% 로 낮추는 게 권장.

### 4.4 데이터 페이즈 SP 가 진단기 BUS_OFF 에 미치는 영향

**거의 없음.** 이유:
- Classic CAN 진단기 는 BRS 비트 이후 데이터 페이즈를 **decode 불가** (자기 controller 가 고속 sample 못 함)
- 진단기는 BRS 까지만 따라가고 그 후는 "노이즈" 로 인식해서 무시
- 따라서 데이터 페이즈 SP 가 75% 든 80% 든 진단기에는 영향 없음

**데이터 페이즈 SP 가 영향을 주는 곳**:
1. 우리 FDCAN1 의 self-monitoring (자기 송신 비트 검증)
2. 다른 FD-capable 노드와의 통신
3. TDC (Transmitter Delay Compensation) 동작과 연동

---

## 5. SJW (Sync Jump Width)

### 5.1 SJW 의 역할

여러 노드 간 clock 발진기는 미세하게 다름 (수십 ppm). 시간이 지나면 비트 edge 가 점진적으로 어긋남 → 누적되어 동기 잃을 위험.

**SJW** = 비트 edge 가 예상보다 일찍/늦게 도착할 때, **timing 을 조정할 수 있는 최대 TQ 수**.

### 5.2 SJW 제약

```
SJW ≤ min(PHASE_SEG1, PHASE_SEG2)
```

본 프로젝트:
- Nominal: min(11, 4) = 4 → **SJW 최대 4 가능**
- Data: min(15, 4) = 4 → **SJW 최대 4 가능**

### 5.3 현재 설정 평가

```c
hfdcan1.Init.NominalSyncJumpWidth = 1;
hfdcan1.Init.DataSyncJumpWidth   = 1;
```

- SJW = 1: **가장 빡빡한 설정**. 노드 간 clock 정확도 매우 높아야 동작
- SJW = 2~4: 더 큰 보정 허용 → 발진기 오차 / 케이블 지연 / EMI 에 관대

**권장 (안정성 향상)**:
```c
hfdcan1.Init.NominalSyncJumpWidth = 2;   /* 1 → 2: nominal phase 보정 능력 2배 */
hfdcan1.Init.DataSyncJumpWidth   = 4;    /* 1 → 4: 데이터 페이즈 보정 능력 4배 */
```

위 변경은 기존 동작 그대로 + 어긋남 보정 능력만 강화. 위험 없음.

---

## 6. 본 프로젝트 최종 권장 설정 정리

### 6.1 Nominal Bit Timing (모든 비트 공통)

```c
hfdcan1.Init.NominalPrescaler     = nprec;  /* baud 별로 결정 (500k=10) */
hfdcan1.Init.NominalSyncJumpWidth = 1;      /* 또는 2 (안정성 ↑) */
hfdcan1.Init.NominalTimeSeg1      = 11;     /* nbs1, 75% SP */
hfdcan1.Init.NominalTimeSeg2      = 4;      /* nbs2 */
```

→ Sample Point **75%** (진단기 / 차량 ECU 표준)

### 6.2 Data Bit Timing (FD BRS 데이터 페이즈)

```c
hfdcan1.Init.DataPrescaler     = dprec;  /* baud 별로 결정 (2M=2) */
hfdcan1.Init.DataSyncJumpWidth = 1;      /* 또는 2~4 (안정성 ↑) */
hfdcan1.Init.DataTimeSeg1      = 15;     /* dbs1 */
hfdcan1.Init.DataTimeSeg2      = 4;      /* dbs2 */
```

→ Sample Point **80%** (CAN FD 데이터 페이즈 표준 범위 내)

### 6.3 4 Mbps 사용 시 권장 조정

```c
/* 4 Mbps 데이터 페이즈: 트랜시버 지연 마진 확보 위해 SP 낮춤 */
dbs1 = 14;   /* (1+14)/20 = 75% */
dbs2 = 5;
DataSyncJumpWidth = 4;
```

---

## 7. 트러블슈팅 체크리스트

### 7.1 다른 노드와 BUS_OFF 충돌 시

1. **각 노드의 Sample Point 확인**:
   - 진단기 / 차량 ECU / 본 디바이스 — 모두 같아야 함 (75% 권장)
2. **SJW 가 너무 작지 않은지 확인**:
   - SJW = 1 이면 → 2 또는 그 이상으로 증가
3. **종단 저항 확인**:
   - 버스 양 끝에만 120Ω, 중간은 OFF
4. **FDCAN1 의 LEC (Last Error Code) 모니터링**:
   - LEC=1 (STUFF) → SP mismatch
   - LEC=2 (FORM) → FD-Classic 충돌
   - LEC=3 (ACK) → 노드 간 ACK 타이밍 불일치
   - LEC=6 (CRC) → EMI / 신호 품질 문제

### 7.2 진단 코드 (TaskCAN idle loop 에 추가)

```c
static uint32_t s_last_log = 0;
if (HAL_GetTick() - s_last_log >= 1000) {
    s_last_log = HAL_GetTick();
    FDCAN_ProtocolStatusTypeDef ps;
    FDCAN_ErrorCountersTypeDef ec;
    HAL_FDCAN_GetProtocolStatus(&hfdcan1, &ps);
    HAL_FDCAN_GetErrorCounters(&hfdcan1, &ec);
    if (ec.TxErrorCnt > 0 || ec.RxErrorCnt > 0 || ps.LastErrorCode != 0) {
        printf("[CAN1] LEC=%lu DLEC=%lu TEC=%u REC=%u BO=%u EP=%u EW=%u\r\n",
               ps.LastErrorCode, ps.DataLastErrorCode,
               ec.TxErrorCnt, ec.RxErrorCnt,
               ps.BusOff, ps.ErrorPassive, ps.Warning);
    }
}
```

| LEC 값 | 의미 | 대응 |
|---|---|---|
| 0 | No error | 정상 |
| 1 | Stuff Error | Sample Point mismatch 의심 |
| 2 | Form Error | FD-Classic 충돌 또는 프로토콜 위반 |
| 3 | Ack Error | ACK 타이밍 불일치 / 단독 노드 |
| 4 | Bit1 Error | 송신 recessive 인데 dominant 검출 |
| 5 | Bit0 Error | 송신 dominant 인데 recessive 검출 |
| 6 | CRC Error | EMI / 신호 품질 / 클럭 오차 |
| 7 | No Change | 이전 에러 코드 동일 |

---

## 8. 검증 시나리오

### 8.1 빌드 후 확인

부팅 로그:
```
[CAN] HW config: protocolId=0x0130(FD), dataRate=0x06 -> nominal=1, data=6 (valid=0)
```

→ FDCAN1 init 정상 + 75% SP / 80% data SP 적용 확인

### 8.2 진단기 통신 검증

1. **본 디바이스 BSA 비활성 상태에서** 진단기로 차량 ECU 통신
   - BUS_OFF 발생 X → 정상
2. **BSA START 후** 진단기 통신 시도
   - 이전: 간헐적 BUS_OFF 발생
   - 변경 후: BUS_OFF 발생 빈도 격감 또는 0회 (목표)
3. 1시간 이상 운용 후 진단기 측 통계 확인

### 8.3 자체 진단 로그

`[CAN1] LEC=X` 출력이:
- 0 또는 7 위주 → 안정 ✓
- 1 (Stuff) 빈발 → SP mismatch 다른 위치에서 잔존
- 6 (CRC) 빈발 → 신호 품질 / EMI / 종단 확인

---

## 9. 참고 자료

- ISO 11898-1:2015 — Road vehicles — Controller area network (CAN) — Part 1: Data link layer and physical signalling
- ISO 11898-2:2016 — Part 2: High-speed medium access unit
- CiA-601-2 — CAN FD bit timing recommendation
- STM32H7RS Reference Manual — FDCAN section (CCCR, NBTP, DBTP registers)
- ARM Cortex-M7 Generic User Guide — FDCAN peripheral

---

## 10. 변경 이력

| 일자 | 변경 내용 | 파일 |
|---|---|---|
| 2026-XX-XX | nbs1: 12 → 11, nbs2: 3 → 4 (SP 81.25% → 75%) | task-can.c CAN_set_baud |
| 2026-XX-XX | dbs1=15, dbs2=4 유지 (SP 80%) | task-can.c CAN_set_baud |
| TBD | (옵션) NominalSyncJumpWidth: 1 → 2, DataSyncJumpWidth: 1 → 4 | task-can.c CAN_set_baud |
