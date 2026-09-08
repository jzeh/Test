/**
  ******************************************************************************
  * @file    fw_flash_xspi.c
  * @brief   XSPI1 External NOR Flash Programming API 구현 (evcd_test 포팅)
  *
  *  포팅 출처: Z:\11_Work\00_Git_Source\05_Others\frp-scan\Common\Src\fw_flash_xspi.c
  *  주요 변경:
  *    - 로깅 GLogI/GLogE → fw_compat.h 매크로 (기본 no-op)
  *    - HAL_IWDG_Refresh(&hiwdg) → FW_IWDG_REFRESH() 매크로 (기본 no-op)
  *    - .axisram 섹션 attribute 제거 (evcd_test ICF에 미정의)
  *
  *  Boot 컨텍스트 전용. EXTMEM 미들웨어 indirect mode 래퍼.
  *  Appli에서도 호출 가능하지만 XIP 영역(Slot 0, 0x90010000~)에 write 시도하지 않을 것.
  *  Slot 1 (0x04000000, no-XIP)에 staging은 Boot이 전담함.
  ******************************************************************************
  */

#include "fw_flash_xspi.h"
#include "fw_compat.h"
#include "stm32_extmem.h"

#include <string.h>

/*============================================================================
 *  Private state
 *===========================================================================*/

static uint32_t s_flash_size  = 0;   /* SFDP-detected size (bytes) */
static uint8_t  s_initialized = 0;

/* Readback verify buffer — AXI SRAM (일반 .bss).
 * 4KB는 RAM 여유 안에서 충분히 수용 가능. cache 일관성 영향 없음
 * (EXTMEM_Read는 indirect mode에서 polling 방식). */
static uint8_t s_verify_buf[XSPI_VERIFY_CHUNK_SIZE];

/*============================================================================
 *  1. xspi_flash_init
 *     EXTMEM 매니저는 이미 MX_EXTMEM_MANAGER_Init()에서 초기화됨.
 *     여기서는 SFDP 정보만 읽어 capacity 캐싱.
 *===========================================================================*/

HAL_StatusTypeDef xspi_flash_init(void)
{
    if (s_initialized) {
        return HAL_OK;
    }

    EXTMEM_NOR_SFDP_FlashInfoTypeDef flash_info;
    if (EXTMEM_GetInfo(XSPI_EXTMEM_ID, &flash_info) != EXTMEM_OK) {
        GLogE("xspi_flash_init: EXTMEM_GetInfo failed\r\n");
        return HAL_ERROR;
    }

    /* FlashSize는 2의 거듭제곱 (예: 27 = 128MB) */
    s_flash_size  = (1U << flash_info.FlashSize);
    s_initialized = 1U;

    GLogI("[XSPI] init OK, size=%uMB, page=%uB, sector=%uB\r\n",
          (unsigned)(s_flash_size / (1024U * 1024U)),
          (unsigned)flash_info.PageSize,
          (unsigned)flash_info.EraseType1Size);

    return HAL_OK;
}

/*============================================================================
 *  2. xspi_flash_get_size
 *===========================================================================*/

uint32_t xspi_flash_get_size(void)
{
    return s_flash_size;
}

/*============================================================================
 *  3. xspi_flash_erase_sector
 *     주소·크기는 XSPI_SECTOR_SIZE (4KB) 정렬 필수.
 *     섹터 사이에 IWDG 갱신 (활성 시).
 *===========================================================================*/

HAL_StatusTypeDef xspi_flash_erase_sector(uint32_t addr, uint32_t size)
{
    /* Alignment & range check */
    if ((addr % XSPI_SECTOR_SIZE) != 0U ||
        (size % XSPI_SECTOR_SIZE) != 0U ||
        size == 0U) {
        GLogE("xspi_erase: alignment error addr=0x%08X size=0x%X\r\n",
              (unsigned)addr, (unsigned)size);
        return HAL_ERROR;
    }

    if ((addr + size) > FLASH_TOTAL_SIZE) {
        GLogE("xspi_erase: out of bounds addr=0x%08X size=0x%X\r\n",
              (unsigned)addr, (unsigned)size);
        return HAL_ERROR;
    }

    uint32_t remaining = size;
    uint32_t cur_addr  = addr;

    while (remaining > 0U) {
        int32_t er = EXTMEM_EraseSector(XSPI_EXTMEM_ID, cur_addr, XSPI_SECTOR_SIZE);
        if (er != EXTMEM_OK) {
            GLogE("xspi_erase: failed at 0x%08X (extmem rc=%ld)\r\n",
                  (unsigned)cur_addr, (long)er);
            return HAL_ERROR;
        }

        cur_addr  += XSPI_SECTOR_SIZE;
        remaining -= XSPI_SECTOR_SIZE;

        FW_IWDG_REFRESH();
    }

    return HAL_OK;
}

/*============================================================================
 *  4. xspi_flash_program
 *     NOR Flash는 page(256B) 단위 program이 원칙. 마지막 청크가 page 미정렬이면
 *     0xFF로 패딩하여 page-aligned writes 강제.
 *===========================================================================*/

HAL_StatusTypeDef xspi_flash_program(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0U) {
        return HAL_ERROR;
    }

    if ((addr + len) > FLASH_TOTAL_SIZE) {
        GLogE("xspi_program: out of bounds addr=0x%08X len=0x%X\r\n",
              (unsigned)addr, (unsigned)len);
        return HAL_ERROR;
    }

    /* page-align 보장용 패딩 버퍼.
     * static으로 두어 매 호출마다 스택 4KB 차지 회피. */
    static uint8_t page_buf[XSPI_VERIFY_CHUNK_SIZE];

    uint32_t offset    = 0U;
    uint32_t remaining = len;

    while (remaining > 0U) {
        uint32_t chunk = (remaining > XSPI_VERIFY_CHUNK_SIZE)
                            ? XSPI_VERIFY_CHUNK_SIZE : remaining;

        const uint8_t *src       = data + offset;
        uint32_t       write_len = chunk;

        /* 마지막 청크가 page 미정렬이면 0xFF로 패딩 */
        if ((chunk % XSPI_PAGE_SIZE) != 0U) {
            memcpy(page_buf, data + offset, chunk);
            memset(page_buf + chunk, 0xFFU, XSPI_PAGE_SIZE - (chunk % XSPI_PAGE_SIZE));
            src       = page_buf;
            write_len = ((chunk + XSPI_PAGE_SIZE - 1U) / XSPI_PAGE_SIZE) * XSPI_PAGE_SIZE;
        }

        if (EXTMEM_Write(XSPI_EXTMEM_ID, addr + offset, (uint8_t *)src, write_len) != EXTMEM_OK) {
            GLogE("xspi_program: failed at 0x%08X\r\n", (unsigned)(addr + offset));
            return HAL_ERROR;
        }

        offset    += chunk;
        remaining -= chunk;

        FW_IWDG_REFRESH();
    }

    return HAL_OK;
}

/*============================================================================
 *  5. xspi_flash_read (indirect mode)
 *     Boot에서는 XIP가 아직 활성화되지 않았으므로 EXTMEM_Read를 통해 indirect로 읽음.
 *     Appli에서는 일반적으로 0x90000000~ 직접 dereference가 가능하지만,
 *     Slot 1(no-XIP 영역)을 읽을 때는 이 함수가 필수.
 *===========================================================================*/

HAL_StatusTypeDef xspi_flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (buf == NULL || len == 0U) {
        return HAL_ERROR;
    }

    if ((addr + len) > FLASH_TOTAL_SIZE) {
        GLogE("xspi_read: out of bounds addr=0x%08X len=0x%X\r\n",
              (unsigned)addr, (unsigned)len);
        return HAL_ERROR;
    }

    if (EXTMEM_Read(XSPI_EXTMEM_ID, addr, buf, len) != EXTMEM_OK) {
        GLogE("xspi_read: failed at 0x%08X\r\n", (unsigned)addr);
        return HAL_ERROR;
    }

    return HAL_OK;
}

/*============================================================================
 *  6. xspi_flash_verify
 *     Write 후 readback 비교. 4KB 청크 단위로 IWDG 갱신.
 *===========================================================================*/

HAL_StatusTypeDef xspi_flash_verify(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0U) {
        return HAL_ERROR;
    }

    if ((addr + len) > FLASH_TOTAL_SIZE) {
        GLogE("xspi_verify: out of bounds addr=0x%08X len=0x%X\r\n",
              (unsigned)addr, (unsigned)len);
        return HAL_ERROR;
    }

    uint32_t offset    = 0U;
    uint32_t remaining = len;

    while (remaining > 0U) {
        uint32_t chunk = (remaining > XSPI_VERIFY_CHUNK_SIZE)
                            ? XSPI_VERIFY_CHUNK_SIZE : remaining;

        if (EXTMEM_Read(XSPI_EXTMEM_ID, addr + offset, s_verify_buf, chunk) != EXTMEM_OK) {
            GLogE("xspi_verify: read failed at 0x%08X\r\n", (unsigned)(addr + offset));
            return HAL_ERROR;
        }

        if (memcmp(s_verify_buf, data + offset, chunk) != 0) {
            GLogE("xspi_verify: mismatch at 0x%08X\r\n", (unsigned)(addr + offset));
            return HAL_ERROR;
        }

        offset    += chunk;
        remaining -= chunk;

        FW_IWDG_REFRESH();
    }

    return HAL_OK;
}
