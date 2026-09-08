/**
  ******************************************************************************
  * @file    boot_diskio.c
  * @brief   Boot 컨텍스트 FatFs disk I/O — eMMC via HAL_MMC (Read-Only)
  *
  *  포팅 출처: Z:\11_Work\00_Git_Source\05_Others\frp-scan\Boot\Core\Src\boot_diskio.c
  *  주요 변경: disk_write 를 FF_FS_READONLY 가드로 비활성 (사이즈 절감).
  *
  *  Appli 의 user_diskio.c(RTOS-aware) 와 달리, Boot은 single-thread이므로
  *  HAL_MMC_ReadBlocks/GetCardState 만 polling 방식으로 직접 호출.
  *
  *  hmmc1 은 Boot/Core/Src/main.c 에서 정의됨.
  ******************************************************************************
  */

#include "ff.h"
#include "diskio.h"
#include "stm32h7rsxx_hal.h"
#include <string.h>

extern MMC_HandleTypeDef hmmc1;

#define BLOCK_SIZE  512U

/* 32B 정렬 단일-섹터 bounce 버퍼.
 * FatFs 가 넘기는 buff(윈도우/사용자 버퍼)는 32B 정렬·캐시라인 정렬이 보장되지 않아,
 * IDMA + D-Cache 환경에서 직접 읽으면 sector 데이터가 깨진다(=mount/read 실패).
 * Appli(sys-emmc.c)의 검증된 패턴대로: 정렬 버퍼로 읽고 → caller 로 memcpy. */
__attribute__((aligned(32))) static uint8_t s_boot_secbuf[BLOCK_SIZE];

DSTATUS disk_initialize(BYTE pdrv)
{
    (void)pdrv;
    /* hmmc1 은 main()의 Boot_InitEmmc()에서 HAL_MMC_Init 호출 완료된 상태.
     * 여기서는 추가 작업 불필요 — 0(=STA_OK) 반환. */
    return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
    (void)pdrv;
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    (void)pdrv;

    /* 단일 섹터씩 정렬 bounce 버퍼로 읽어 caller buff 로 복사.
     * (multi-block timing race 회피 + 정렬/캐시 안전 — Appli 와 동일 전략) */
    for (UINT i = 0; i < count; i++) {

        if (HAL_MMC_ReadBlocks(&hmmc1, s_boot_secbuf,
                               (uint32_t)sector + i, 1U, 5000) != HAL_OK) {
            return RES_ERROR;
        }

        /* 전송 완료 대기 — single-thread polling */
        while (HAL_MMC_GetCardState(&hmmc1) != HAL_MMC_CARD_TRANSFER) {
            /* spin */
        }

        /* ★ 블로킹 HAL_MMC_ReadBlocks 는 CPU FIFO 로 읽어 버퍼에 캐시 코히런트하게
         *   적재됨 (IDMA 경로 아님). 여기서 D-Cache invalidate 를 하면 방금 읽은
         *   캐시라인이 버려져 stale(0) 데이터를 읽게 되므로 절대 금지. (Appli sys-emmc.c 동일) */
        memcpy(buff + (i * BLOCK_SIZE), s_boot_secbuf, BLOCK_SIZE);
    }

    return RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    (void)pdrv;

    if (HAL_MMC_WriteBlocks(&hmmc1, (uint8_t *)buff, (uint32_t)sector, count, 5000) != HAL_OK) {
        return RES_ERROR;
    }

    while (HAL_MMC_GetCardState(&hmmc1) != HAL_MMC_CARD_TRANSFER) {
        /* spin */
    }

    return RES_OK;
}
#endif /* !FF_FS_READONLY */

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    (void)pdrv;
    HAL_MMC_CardInfoTypeDef info;

    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;

    case GET_SECTOR_COUNT:
        HAL_MMC_GetCardInfo(&hmmc1, &info);
        *(DWORD *)buff = info.LogBlockNbr;
        return RES_OK;

    case GET_SECTOR_SIZE:
        *(WORD *)buff = BLOCK_SIZE;
        return RES_OK;

    case GET_BLOCK_SIZE:
        HAL_MMC_GetCardInfo(&hmmc1, &info);
        *(DWORD *)buff = info.LogBlockSize / BLOCK_SIZE;
        return RES_OK;

    default:
        return RES_PARERR;
    }
}
