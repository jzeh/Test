#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
bin2hex.py — 서명된 OTA 바이너리(app_ota.bin)를 Intel-HEX 로 인코딩한다.

사용법:
    python bin2hex.py <input.bin> <output.hex> [--bytes-per-line N]

전제 (중요):
    입력은 반드시 fw_sign.py 로 "서명된" 바이너리여야 한다.
        app.bin --fw_sign.py--> app_ota.bin([64B 헤더][이미지]) --bin2hex.py--> app_ota.hex
    IAR 가 직접 출력한 .hex 를 쓰면 내장 헤더가 없어 장치가 END 에서 거부한다.

출력 형식:
    · 주소는 0 부터 연속 (장치는 순차 concat 으로 재조립)
    · 기본 112 바이트/라인  → 라인 길이 11 + 2*112 = 235 자
      → BLE 프레임 = 235 + 1(CR) + 7(SOF/LEN/FID/CRC16/EOF) = 243 B

      ★ 라인 크기는 BLE ATT payload(=협상 MTU - 3) 안에 들어가야 한다.
        frame = 2*N + 19  이므로
            N=112 → 243 B  (MTU 247 / ATT 244 에서 통과)  ← 기본값
            N=128 → 275 B  (ATT 244 초과 → write 절단/거부 → 장치 무응답)
            N=64  → 147 B  (여유 큰 보수적 설정, 64KB 경계도 안 걸침)
        MTU 를 278 이상으로 확보했거나 앱이 분할 write 를 하는 경우에만 N 을 키울 것.
    · 64KB 경계마다 확장 선형 주소 레코드(type 04) 삽입
    · 마지막에 EOF 레코드(:00000001FF)

레코드 구조: ':' + LL + AAAA + TT + DATA + CC   (모두 대문자 ASCII 16진)
    CC = (0x100 - (sum(LL..DATA) & 0xFF)) & 0xFF
"""
import sys

DEFAULT_BYTES_PER_LINE = 112      # BLE ATT payload 244(MTU 247) 기준 안전 최대값
MAX_BYTES_PER_LINE     = 250      # GIT payload(512) 제약: 11 + 2*250 = 511


def emit_record(rec):
    """rec = [LL, addr_hi, addr_lo, type, *data] → ':....CC\n' 문자열"""
    cc = (0x100 - (sum(rec) & 0xFF)) & 0xFF
    return ":" + "".join("%02X" % b for b in rec) + "%02X\n" % cc


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1

    in_path  = sys.argv[1]
    out_path = sys.argv[2]

    bpl = DEFAULT_BYTES_PER_LINE
    if "--bytes-per-line" in sys.argv:
        bpl = int(sys.argv[sys.argv.index("--bytes-per-line") + 1])
    if not (1 <= bpl <= MAX_BYTES_PER_LINE):
        print("[bin2hex] error: bytes-per-line 은 1~%d 범위" % MAX_BYTES_PER_LINE)
        return 1
    if (0x10000 % bpl) != 0:
        # 64KB 경계를 걸치는 라인이 생기지만, 장치는 type-04 로 base 를 갱신한 뒤
        # 순차 오프셋으로 재조립하므로 무해하다(검증됨). 정보성 안내만 출력.
        print("[bin2hex] note: %d bytes/line 은 64KB 경계를 걸치는 라인이 생김 "
              "(순차 수신 방식이라 무해)" % bpl)

    data = open(in_path, "rb").read()
    if len(data) == 0:
        print("[bin2hex] error: %s 가 비어있음" % in_path)
        return 1

    lines = 0
    upper = -1
    with open(out_path, "w", newline="") as f:
        for base in range(0, len(data), bpl):
            hi = (base >> 16) & 0xFFFF
            if hi != upper:
                # 확장 선형 주소(type 04): 상위 16bit 갱신
                f.write(emit_record([0x02, 0x00, 0x00, 0x04,
                                     (hi >> 8) & 0xFF, hi & 0xFF]))
                lines += 1
                upper = hi

            chunk = data[base:base + bpl]
            addr  = base & 0xFFFF
            f.write(emit_record([len(chunk), (addr >> 8) & 0xFF, addr & 0xFF,
                                 0x00] + list(chunk)))
            lines += 1

        f.write(":00000001FF\n")   # EOF
        lines += 1

    print("[bin2hex] %s -> %s" % (in_path, out_path))
    print("  input     : %d bytes" % len(data))
    print("  per line  : %d bytes  (line %d chars -> BLE frame %d B, ATT limit 244)"
          % (bpl, 11 + 2 * bpl, 2 * bpl + 19))
    print("  records   : %d lines (EOF 포함)" % lines)
    return 0


if __name__ == "__main__":
    sys.exit(main())
