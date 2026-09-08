#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
fw_sign.py — OTA 이미지에 내장 헤더(fw_image_header_t, 64B)를 prepend 한다.

사용법:
    python fw_sign.py <input.bin> <output_ota.bin> [--version-h <fw_version.h>]

동작:
    1) input.bin(순수 실행 이미지) 을 읽는다
    2) payload CRC32 (CRC-32/ISO-HDLC = zlib.crc32) 계산
    3) 버전은 fw_version.h 에서 파싱 (없으면 0.0.0), git describe 는 부가정보로 기록
    4) 64B 헤더를 만들어 payload 앞에 붙여 output_ota.bin 으로 저장

    앱은 이 output_ota.bin 을 512B 청크로 그대로 전송하면 된다.
    장치는 수신하면서 선행 64B 헤더를 분리·검증하고 payload 만 저장한다(XIP-safe).

주의: CRC 알고리즘이 펌웨어 fw_crc32.c 와 반드시 일치해야 한다.
      fw_crc32.c = CRC-32/ISO-HDLC 이므로 zlib.crc32 와 동일하다.
"""
import sys, os, struct, zlib, re, subprocess

MAGIC        = 0x45564344   # "EVCD"
HDR_FMT_VER  = 1
HDR_SIZE     = 64
GIT_DESC_LEN = 32


def parse_version_h(path):
    """fw_version.h 에서 major/minor/patch/app_no 를 뽑아낸다. 실패 시 0."""
    major = minor = patch = 0
    app_no = 1
    try:
        text = open(path, "r", encoding="utf-8", errors="ignore").read()
    except OSError:
        print("[fw_sign] warning: %s 없음 → version 0.0.0" % path)
        return major, minor, patch, app_no

    def find(name, default):
        m = re.search(r"#define\s+%s\s+(\d+)" % name, text)
        return int(m.group(1)) if m else default

    major  = find("FW_VERSION_MAJOR", 0)
    minor  = find("FW_VERSION_MINOR", 0)
    patch  = find("FW_VERSION_PATCH", 0)
    app_no = find("FW_APP_NO", 1)
    return major, minor, patch, app_no


def git_describe():
    try:
        out = subprocess.check_output(
            ["git", "describe", "--long", "--dirty", "--tags"],
            stderr=subprocess.DEVNULL)
        return out.decode("utf-8", "ignore").strip()
    except Exception:
        return ""


def build_epoch():
    """재현 가능하도록 git commit 시각을 우선 사용, 없으면 0."""
    try:
        out = subprocess.check_output(
            ["git", "log", "-1", "--format=%ct"], stderr=subprocess.DEVNULL)
        return int(out.decode().strip())
    except Exception:
        return 0


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1

    in_path  = sys.argv[1]
    out_path = sys.argv[2]

    ver_h = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "..", "Common", "Inc", "fw_version.h")
    if "--version-h" in sys.argv:
        ver_h = sys.argv[sys.argv.index("--version-h") + 1]

    payload = open(in_path, "rb").read()
    if len(payload) == 0:
        print("[fw_sign] error: %s 가 비어있음" % in_path)
        return 1

    image_crc = zlib.crc32(payload) & 0xFFFFFFFF
    major, minor, patch, app_no = parse_version_h(ver_h)
    desc = git_describe().encode("ascii", "ignore")[:GIT_DESC_LEN - 1]
    epoch = build_epoch()

    # 선행 60바이트(hdr_crc32 제외) 조립
    #  <I H H I I 3B B I 32s 4B>
    head = struct.pack(
        "<IHHII3BBI32s4B",
        MAGIC,               # magic
        HDR_FMT_VER,         # hdr_version
        HDR_SIZE,            # hdr_size
        len(payload),        # image_size
        image_crc,           # image_crc32
        major, minor, patch, # fw_version[3]
        app_no & 0xFF,       # app_no
        epoch & 0xFFFFFFFF,  # build_epoch
        desc,                # git_desc[32] (자동 널패딩)
        0, 0, 0, 0,          # reserved[4]
    )
    assert len(head) == HDR_SIZE - 4, "header prefix must be 60 bytes, got %d" % len(head)

    hdr_crc = zlib.crc32(head) & 0xFFFFFFFF
    header  = head + struct.pack("<I", hdr_crc)
    assert len(header) == HDR_SIZE

    with open(out_path, "wb") as f:
        f.write(header)
        f.write(payload)

    print("[fw_sign] %s -> %s" % (in_path, out_path))
    print("  version   : %d.%d.%d  (app_no=%d)" % (major, minor, patch, app_no))
    print("  git_desc  : %s" % (desc.decode() or "(none)"))
    print("  image_size: %d bytes" % len(payload))
    print("  image_crc : 0x%08X" % image_crc)
    print("  hdr_crc   : 0x%08X" % hdr_crc)
    print("  total     : %d bytes (header %d + payload)" % (len(header) + len(payload), HDR_SIZE))
    return 0


if __name__ == "__main__":
    sys.exit(main())
