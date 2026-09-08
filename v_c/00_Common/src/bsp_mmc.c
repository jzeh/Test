/*************************************************************
 * NOTE : bsp_mmc.c
 *      emmc bsp
 * Author : Lee junho
 * Since : 2021.08.26
**************************************************************/
#include "common.h"
#include "typedef.h"
#include "bsp_mmc.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define MMC_INSTANCES_NBR								1UL

#define MMC_INSTANCE_HANDLER							hmmc1

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
int32_t BSP_MMC_GetCardState( void );
int32_t BSP_MMC_GetCardInfo( BSP_MMC_CardInfo *CardInfo );
int32_t BSP_MMC_ReadBlocks_DMA( uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr );
int32_t BSP_MMC_WriteBlocks_DMA( uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr );

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   External Variables
 *--------------------------------------------------------------------*/
extern MMC_HandleTypeDef hmmc1;

/*----------------------------------------------------------------------
 *   Functions implementation
 *--------------------------------------------------------------------*/
int32_t BSP_MMC_GetCardState( void )
{
	return (int32_t)((HAL_MMC_GetCardState( &MMC_INSTANCE_HANDLER ) == HAL_MMC_CARD_TRANSFER ) ? MMC_TRANSFER_OK : MMC_TRANSFER_BUSY);
}

/**
  * @brief  Get MMC information about specific MMC card.
  * @param  CardInfo : Pointer to HAL_MMC_CardInfoTypedef structure
  * @retval None
  */
int32_t BSP_MMC_GetCardInfo( BSP_MMC_CardInfo *CardInfo )
{
	int32_t ret;

	if( HAL_MMC_GetCardInfo( &MMC_INSTANCE_HANDLER, CardInfo) != HAL_OK )
	{
		ret = BSP_ERROR_PERIPH_FAILURE;
	}
	else
	{
		ret = BSP_ERROR_NONE;
	}

	/* Return BSP status */
	return ret;
}

/**
  * @brief  Reads block(s) from a specified address in an SD card, in DMA mode.
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be read
  * @param  BlocksNbr  Number of SD blocks to read
  * @retval BSP status
  */
int32_t BSP_MMC_ReadBlocks_DMA( uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr )
{
	int32_t ret;

	if( HAL_MMC_ReadBlocks_DMA( &MMC_INSTANCE_HANDLER, (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK )
	{
		ret = BSP_ERROR_PERIPH_FAILURE;
	}
	else
	{
		ret = BSP_ERROR_NONE;
	}

	/* Return BSP status */
	return ret;
}

/**
  * @brief  Writes block(s) to a specified address in an SD card, in DMA mode.
  * @param  pData      Pointer to the buffer that will contain the data to transmit
  * @param  BlockIdx   Block index from where data is to be written
  * @param  BlocksNbr  Number of SD blocks to write
  * @retval BSP status
  */
int32_t BSP_MMC_WriteBlocks_DMA( uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr )
{
	int32_t ret;

	if( HAL_MMC_WriteBlocks_DMA( &MMC_INSTANCE_HANDLER, (uint8_t *)pData, BlockIdx, BlocksNbr) != HAL_OK )
	{
		ret = BSP_ERROR_PERIPH_FAILURE;
	}
	else
	{
		ret = BSP_ERROR_NONE;
	}

	/* Return BSP status */
	return ret;
}