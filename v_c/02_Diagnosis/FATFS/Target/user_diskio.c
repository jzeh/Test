/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"

#include "common.h"
#include "bsp_mmc.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MMC_WRITE_TIMEOUT								1U
#define MMC_READ_TIMEOUT								1U

#define MMC_DEFAULT_BLOCK_SIZE							512
#define BLOCKSIZE										512

#define ENABLE_MMC_DMA_CACHE_MAINTENANCE_READ			0
#define ENABLE_MMC_DMA_CACHE_MAINTENANCE_WRITE			0
#define ENABLE_SD_DMA_CACHE_MAINTENANCE					0

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS	Stat		= STA_NOINIT;
static volatile UINT	WriteStatus	= 0;
static volatile UINT	ReadStatus	= 0;
static uint32_t			scratch[BLOCKSIZE];

extern MMC_HandleTypeDef hmmc1;

static DSTATUS MMC_CheckStatus(BYTE pdrv)
{
	Stat = STA_NOINIT;

	if( (BSP_MMC_GetCardState( ) == MMC_TRANSFER_OK ) );
	{
		Stat &= ~STA_NOINIT;
	}

	return Stat;
}

void HAL_MMC_TxCpltCallback( MMC_HandleTypeDef *hmmc )
{
	WriteStatus = 1;
}

void HAL_MMC_RxCpltCallback( MMC_HandleTypeDef *hmmc )
{
	ReadStatus = 1;
}

/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */
	Stat = MMC_CheckStatus( pdrv );
	return Stat;
  /* USER CODE END INIT */
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */
	return MMC_CheckStatus( pdrv );
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */
	DRESULT		res = RES_ERROR;
	uint32_t	timeout;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
  uint32_t alignedAddr;
#endif

	ReadStatus = 0;

	if( !((uint32_t)buff & 0x3) )
	{
		if( BSP_MMC_ReadBlocks_DMA( (uint32_t*)buff, (uint32_t) (sector), count) == BSP_ERROR_NONE )
		{
			/* Wait that the reading process is completed or a timeout occurs */
			timeout = HAL_GetTick();
			while( ( ReadStatus == 0 ) && ( (HAL_GetTick() - timeout) < MMC_TIMEOUT ) )
			{
			}

			/* incase of a timeout return error */
			if (ReadStatus == 0)
			{
				res = RES_ERROR;
			}
			else
			{
				ReadStatus	= 0;
				timeout		= HAL_GetTick();

				while( (HAL_GetTick() - timeout) < MMC_TIMEOUT )
				{
					if( BSP_MMC_GetCardState( ) == MMC_TRANSFER_OK )
					{
						res = RES_OK;

#if( ENABLE_MMC_DMA_CACHE_MAINTENANCE_READ == 1 )
						SCB_CleanInvalidateDCache();
#endif
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
            /*
               the SCB_InvalidateDCache_by_Addr() requires a 32-Byte aligned address,
               adjust the address and the D-Cache size to invalidate accordingly.
             */
            alignedAddr = (uint32_t)buff & ~0x1F;
            SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
#endif
						break;
					}
				}
			}
		}
	}
	else
	{
		//MONI 20230419 suresoft codecoverage bootloader num : 19 / initialize local variable 		
		int32_t ret = BSP_ERROR_PERIPH_FAILURE;
		int i;

		for( i = 0; i < count; i++ )
		{
			ret = BSP_MMC_ReadBlocks_DMA( (uint32_t*)scratch, (uint32_t)sector++, 1 );

			if( ret == BSP_ERROR_NONE )
			{
				/* Wait that the reading process is completed or a timeout occurs */
				timeout = HAL_GetTick();
				while( ( ReadStatus == 0 ) && ( (HAL_GetTick() - timeout) < MMC_TIMEOUT ) )
				{
				}

				/* incase of a timeout return error */
				if( ReadStatus == 0 )
				{
					break;
				}
				else
				{
					ReadStatus	= 0;
					timeout		= HAL_GetTick();

					while( ( HAL_GetTick() - timeout ) < MMC_TIMEOUT )
					{
						if( BSP_MMC_GetCardState( ) == MMC_TRANSFER_OK )
						{
#if( ENABLE_MMC_DMA_CACHE_MAINTENANCE_READ == 1 )
							SCB_CleanInvalidateDCache();
#endif

							memcpy( buff, scratch, BLOCKSIZE );
							buff += BLOCKSIZE;

							break;
						}
					}
				}
			}
			else
			{
				break;
			}
		}

		if( ( i == count ) && ( ret == BSP_ERROR_NONE ) )
		{
			res = RES_OK;
		}
	}

	return res;
  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{
  /* USER CODE BEGIN WRITE */
  /* USER CODE HERE */
	DRESULT		res = RES_ERROR;
	uint32_t	timeout;

	WriteStatus = 0;

#if( ENABLE_MMC_DMA_CACHE_MAINTENANCE_WRITE == 1 )
//	SCB_CleanInvalidateDCache();
#endif
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
  uint32_t alignedAddr;
  /*
   the SCB_CleanDCache_by_Addr() requires a 32-Byte aligned address
   adjust the address and the D-Cache size to clean accordingly.
   */
  alignedAddr = (uint32_t)buff &  ~0x1F;
  SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
#endif

	if( !((uint32_t)buff & 0x3) )
	{
		if( BSP_MMC_WriteBlocks_DMA( (uint32_t*)buff, (uint32_t)(sector), count) == BSP_ERROR_NONE )
		{
			/* Wait that writing process is completed or a timeout occurs */
			timeout = HAL_GetTick();
			while( ( WriteStatus == 0 ) && ( (HAL_GetTick() - timeout) < MMC_TIMEOUT ) )
			{
			}

			/* incase of a timeout return error */
			if( WriteStatus == 0 )
			{
				res = RES_ERROR;
			}
			else
			{
				WriteStatus	= 0;
				timeout		= HAL_GetTick();

				while( ( HAL_GetTick() - timeout ) < MMC_TIMEOUT )
				{
					if( BSP_MMC_GetCardState( ) == MMC_TRANSFER_OK )
					{
						res = RES_OK;
						break;
					}
				}
			}
		}
	}
	else
	{
		int i;
		int32_t ret = BSP_ERROR_PERIPH_FAILURE;
		//MONI 20230419 suresoft codecoverage bootloader num : 18 / initialize local variable 	

		for (i = 0; i < count; i++)
		{
			WriteStatus = 0;

			memcpy((void *)scratch, (void *)buff, BLOCKSIZE);
			buff += BLOCKSIZE;

			ret = BSP_MMC_WriteBlocks_DMA( (uint32_t*)scratch, (uint32_t)sector++, 1 );
			if( ret == BSP_ERROR_NONE )
			{
				/* Wait that writing process is completed or a timeout occurs */

				timeout = HAL_GetTick();
				while( ( WriteStatus == 0 ) && ( ( HAL_GetTick() - timeout ) < MMC_TIMEOUT ) )
				{
				}

				/* incase of a timeout return error */
				if( WriteStatus == 0 )
				{
					break;
				}
				else
				{
					WriteStatus	= 0;
					timeout		= HAL_GetTick();

					while( ( HAL_GetTick() - timeout ) < MMC_TIMEOUT )
					{
						if( BSP_MMC_GetCardState( ) == MMC_TRANSFER_OK )
						{
							break;
						}
					}
				}
			}
			else
			{
				break;
			}
		}

		if( ( i == count ) && ( ret == BSP_ERROR_NONE ) )
		{
			res = RES_OK;
		}
	}

	return res;
  /* USER CODE END WRITE */
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */
	DRESULT			res = RES_ERROR;
	BSP_MMC_CardInfo	CardInfo;

	if( Stat & STA_NOINIT )			return RES_NOTRDY;

	switch (cmd)
	{
		/* Make sure that no pending write process */
		case CTRL_SYNC :
			res = RES_OK;
			break;

		/* Get number of sectors on the disk (DWORD) */
		case GET_SECTOR_COUNT :
			BSP_MMC_GetCardInfo( &CardInfo );
			*(DWORD*)buff = CardInfo.LogBlockNbr;
			res = RES_OK;
			break;

		/* Get R/W sector size (WORD) */
		case GET_SECTOR_SIZE :
			BSP_MMC_GetCardInfo( &CardInfo );
			*(WORD*)buff = CardInfo.LogBlockSize;
			res = RES_OK;
			break;

		/* Get erase block size in unit of sector (DWORD) */
		case GET_BLOCK_SIZE :
			BSP_MMC_GetCardInfo( &CardInfo );
			*(DWORD*)buff = CardInfo.LogBlockSize / MMC_DEFAULT_BLOCK_SIZE;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	return res;
  /* USER CODE END IOCTL */
}
#endif /* _USE_IOCTL == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
