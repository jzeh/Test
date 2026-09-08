/*----------------------------------------------------------------------
 *   MMC Control( SDMMC, EMMC... )
 *--------------------------------------------------------------------*/
#ifndef	__BSP_MMC_H__
#define	__BSP_MMC_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "fatfs.h"

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define	MMC_TRANSFER_OK									0U
#define	MMC_TRANSFER_BUSY								1U

#define MMC_TIMEOUT										100U

/* Common Error codes */
#define	BSP_ERROR_NONE									 0
#define	BSP_ERROR_NO_INIT								-1
#define	BSP_ERROR_WRONG_PARAM							-2
#define	BSP_ERROR_BUSY									-3
#define	BSP_ERROR_PERIPH_FAILURE						-4
#define	BSP_ERROR_COMPONENT_FAILURE						-5
#define	BSP_ERROR_UNKNOWN_FAILURE						-6
#define	BSP_ERROR_UNKNOWN_COMPONENT						-7
#define	BSP_ERROR_BUS_FAILURE							-8
#define	BSP_ERROR_CLOCK_FAILURE							-9
#define	BSP_ERROR_MSP_FAILURE							-10
#define	BSP_ERROR_FEATURE_NOT_SUPPORTED					-11

#define BSP_MMC_CardInfo								HAL_MMC_CardInfoTypeDef

/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern int32_t BSP_MMC_GetCardState( void );
extern int32_t BSP_MMC_GetCardInfo( BSP_MMC_CardInfo *CardInfo );
extern int32_t BSP_MMC_ReadBlocks_DMA( uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr );
extern int32_t BSP_MMC_WriteBlocks_DMA( uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr );

/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/

#endif // __BSP_MMC_H__