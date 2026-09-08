/*****************************************************************************
* Copyright(C) 2011 Dong-A University MICCA
* All right reserved.
*
* File name	    : tftlcd.c
* Last version	: 1.00
* Description	: This file is source file for disk i/o interface.
*
* History
* Date		    Version	    Author			Description
* 25/06/2011	1.00		oh woomin	    Created
*****************************************************************************/

/* Includes ----------------------------------------------------------------*/
#include <math.h>
#include "AutolinkConfig.h"
#include "diskio.h"
#include "GIT_OemInterface.h"
#include "GIT_Util.h"
#include "HalHandler.h"


/* Privated variables ------------------------------------------------------*/
static volatile DSTATUS Stat = STA_NOINIT;	/* Disk status */
static volatile DSTATUS Stat1 = STA_NOINIT;	/* Disk status */

/*****************************************************************************
* Descriptions  : Initialize Disk Drive
* Parameters    : disk drive
* Return Value  : disk status
*****************************************************************************/
DSTATUS disk_initialize (
	BYTE drv		/* Physical drive number (0) */
)
{
	switch(drv) {
		case FS_EMMC:
			break;

		case FS_SFLASH:
			sFLASH_Init();

			APP_Delay(5);

			if((sFLASH_ReadID() == sFLASH_N25Q064A_ID) || (sFLASH_ReadID() == sFLASH_FM25Q64A_ID)) {
				Stat1 &= ~STA_NOINIT;
			}

		  return Stat1;

		default:
			return STA_NOINIT;			/* Supports only single drive */
	}

	return STA_NOINIT;
}

/*****************************************************************************
* Descriptions  : Get Disk Status
* Parameters    : disk drive
* Return Value  : disk status
*****************************************************************************/
DSTATUS disk_status (
	BYTE drv		/* Physical drive number (0) */
)
{
	switch(drv)
	{
		case FS_EMMC:
	  	return Stat;

		case FS_SFLASH:
			return Stat1;

	  default:
	  	return STA_NOINIT;
	}
}


/*****************************************************************************
* Descriptions  : Read Sector(s)
* Parameters    : drive, buffer, sector, sector count
* Return Value  : disk result
*****************************************************************************/
DRESULT disk_read (
	BYTE drv,			/* Physical drive number (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
//	SD_Error Status = SD_OK;

//	Status = Status;

  if (!count)
  	return RES_PARERR;

	switch(drv)
	{
		case FS_EMMC:
//			{
//				if (Stat & STA_NOINIT)
//					return RES_NOTRDY;
//
//				if(count == 1) {
//					if(SD_ReadBlock(buff, sector, 512) == SD_ERROR)
//						return RES_ERROR;
//
//					/* Check if the Transfer is finished */
//					Status = SD_WaitReadOperation();
//					while(SD_GetStatus() != SD_TRANSFER_OK);
//				}
//				else
//				{
//					while(count) {
//						if(SD_ReadBlock((uint8_t *)buff, sector, 512) == SD_ERROR)
//							return RES_ERROR;
//
//						/* Check if the Transfer is finished */
//						Status = SD_WaitReadOperation();
//						while(SD_GetStatus() != SD_TRANSFER_OK);
//
//						buff+=512;
//						sector++;
//						count--;
//					}
//				}
//
//				return RES_OK;
//			}
		  break;

		case FS_SFLASH:
		  if (Stat1 & STA_NOINIT)
		  	return RES_NOTRDY;

			sFLASH_ReadBuffer(buff, sector * SFLASH_SECTOR_SIZE, count * SFLASH_SECTOR_SIZE);

			return RES_OK;

	  default:
	  	return RES_ERROR;
	}

	return RES_ERROR;
}

/*****************************************************************************
* Descriptions  : Write Sector(s)
* Parameters    : drive, buffer, sector, sector count
* Return Value  : disk result
*****************************************************************************/
DRESULT disk_write (
	BYTE drv,			/* Physical drive number (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
//	SD_Error Status = SD_OK;

//	Status = Status;

  if (!count)
  	return RES_PARERR;

	switch(drv)
	{
		case FS_EMMC:
//			{
//				if (Stat & STA_NOINIT)
//					return RES_NOTRDY;
//
//				if (Stat & STA_PROTECT)
//					return RES_WRPRT;
//
//				if(count == 1)
//				{
//					if(SD_WriteBlock((uint8_t *)buff, sector, 512) == SD_ERROR)
//						return RES_ERROR;
//
//					/* Check if the Transfer is finished */
//					Status = SD_WaitWriteOperation();
//					while(SD_GetStatus() != SD_TRANSFER_OK);
//				}
//				else {
//					while(count) {
//						if(SD_WriteBlock((uint8_t *)buff, sector, 512) == SD_ERROR)
//							return RES_ERROR;
//
//						/* Check if the Transfer is finished */
//						Status = SD_WaitWriteOperation();
//						while(SD_GetStatus() != SD_TRANSFER_OK);
//
//						buff += 512;
//						sector++;
//						count--;
//					}
//				}
//
//				return RES_OK;
//			}
			break;

		case FS_SFLASH:
			{
				int i;

			  if (Stat1 & STA_NOINIT)
			  	return RES_NOTRDY;

				for (i = 1; i <= count; i++)
				{
					sFLASH_EraseSubSector(sector * i * SFLASH_SECTOR_SIZE);
				}

				sFLASH_WriteBuffer((uint8_t *) buff, sector * SFLASH_SECTOR_SIZE, count * SFLASH_SECTOR_SIZE);

				return RES_OK;
			}
			break;

	  default:
	  	return RES_ERROR;
	}

	return RES_ERROR;
}

/*****************************************************************************
* Descriptions  : Miscellaneous Functions
* Parameters    : drive, control code, buffer
* Return Value  : disk result
*****************************************************************************/
DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive number (0) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
//  SD_CardInfo cardinfo;

	switch(drv)
	{
		case FS_EMMC:
//		  switch(ctrl){
//			  case CTRL_SYNC:
//		      break;
//			  case GET_SECTOR_SIZE:
//		      SD_GetCardInfo(&cardinfo);
//		      *((unsigned short int*)buff)=512;//cardinfo.csd.RdBlockLen;//_MAX_SS;
//		      break;
//			  case GET_SECTOR_COUNT:
//		      SD_GetCardInfo(&cardinfo);
//		      *((unsigned int*)buff)=cardinfo.CardCapacity/512;//cardinfo.csd.RdBlockLen;//_MAX_SS;
//		      break;
//			  case GET_BLOCK_SIZE:
//		      SD_GetCardInfo(&cardinfo);
//		      *((unsigned int*)buff)=cardinfo.CardBlockSize;
//		      break;
//			  case CTRL_ERASE_SECTOR:
//		      break;
//		  }

		  return RES_OK;

		case FS_SFLASH:
			{
				UINT *result = (UINT *)buff;

			  if (Stat1 & STA_NOINIT)
			  	return RES_NOTRDY;

				switch (ctrl) {
			    case CTRL_SYNC:
			    	break;

			    case CTRL_POWER:
			    	break;

			    case CTRL_LOCK:
			    	break;

			    case CTRL_EJECT:
			    	break;

			    case GET_SECTOR_COUNT:
                    // MONI 2018-02-06
                    // we reduce storage size from 8M to 7M992k
                    // and we use last 8k for configuration variables
			    	//*((unsigned short int*)buff) = 0x800000 / SFLASH_SECTOR_SIZE;
					// MONI 2018-03-08
					// when system request cluster count, ioctrl returned long number in opererating time.
			    	*((DWORD*)buff) = (DWORD)(((DWORD)0x800000-(DWORD)MAX_CONFIG_STORAGE_SIZE) / (DWORD)SFLASH_SECTOR_SIZE);
                    //INOM
//			    	printf("GET_SECTOR_COUNT: %d\n", *((unsigned short int*)buff));
			    	break;

			    case GET_SECTOR_SIZE:
					// MONI 2018-03-08
					// when system request cluster count, ioctrl returned long number in opererating time.				
			    	*((WORD*)buff) = (WORD)SFLASH_SECTOR_SIZE;
//			    	printf("GET_SECTOR_SIZE: %d\n", *((unsigned short int*)buff));
			    	break;

			    case GET_BLOCK_SIZE:
			    	*result = 1;/*in no.of Sectors */
			    	break;

			    default:
			    	break;
				}

				return RES_OK;
			}
	  	break;

	  default:
	  	return RES_PARERR;
	}
}
