/*******************************************************************************
 Simple SPI Transfer function

  File Name:
    drv_spi.c

  Summary:
    Initializes SPI 1. Transfers data over SPI.
    Uses SPI FIFO to speed up transfer.

  Description:
    .

  Remarks:

 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2016-2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END
#include "FreeRTOS.h"
#include "task.h"
#include "gpio.h"
#include "cmsis_os.h"

#include "drv_spi.h"

#include "spi.h"
#include "common.h"
#include "main.h"

enum {
	TRANSFER_WAIT,
	TRANSFER_COMPLETE,
	TRANSFER_ERROR
};

// 220319 test
#define SPI_DMA_MODE								0

static osMutexId	hSpiMutex1;

osMutexDef( spi1_mutex_drv );

/* transfer state */
__IO uint32_t wTransferState1 = TRANSFER_WAIT;

int8_t DRV_SPI_ChipSelectAssert(uint8_t spiSlaveDeviceIndex, bool assert)
{
	int8_t	error = 0;

	// Select Chip Select
	switch( spiSlaveDeviceIndex )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			if( assert )		HAL_GPIO_WritePin( SPI5_NSS_GPIO_Port, SPI5_NSS_Pin, GPIO_PIN_RESET );
			else				HAL_GPIO_WritePin( SPI5_NSS_GPIO_Port, SPI5_NSS_Pin, GPIO_PIN_SET );
		}
		break;
        
		default :
		{
			GLogE( "Error... Invalid index!!!\r\n" );
		}
		break;
	}

	return error;
}

void DRV_SPI_Initialize()
{
	// Mutex
	hSpiMutex1	= osMutexCreate( osMutex( spi1_mutex_drv ) );
}

#if 0
int8_t DRV_SPI_TransferData(uint8_t spiSlaveDeviceIndex, uint8_t *SpiTxData, uint8_t *SpiRxData, uint16_t spiTransferSize)
{
	SPI_HandleTypeDef	*hspi;
	HAL_StatusTypeDef	st;

	int8_t	error	= 0;

	switch( spiSlaveDeviceIndex )
	{
		case DRV_CANFDSPI_INDEX_1 :				hspi = &hspi;			break;

	}

	// Assert CS
	error = DRV_SPI_ChipSelectAssert( spiSlaveDeviceIndex, true );

#if SPI_DMA_MODE
	st = HAL_SPI_TransmitReceive_DMA( hspi, (uint8_t *)SpiTxData, (uint8_t *)SpiRxData, spiTransferSize );
	if( st != HAL_OK )
	{
		GLogE( "Error...(%d)\r\n", st );
		error = -1;
	}

	while( HAL_SPI_GetState( hspi ) != HAL_SPI_STATE_READY ){}
#else
	st = HAL_SPI_TransmitReceive( hspi, (uint8_t *)SpiTxData, (uint8_t *)SpiRxData, spiTransferSize, 5000 );
	if( st != HAL_OK)
	{
		GLogE( "Error...(%d)\r\n", st );
		error = -1;
	}

//	while( HAL_SPI_GetState( hspi ) != HAL_SPI_STATE_READY ){}
#endif

	error = DRV_SPI_ChipSelectAssert( spiSlaveDeviceIndex, false );

	return error;
}
#else
int8_t DRV_SPI_TransferData1(uint8_t spiSlaveDeviceIndex, uint8_t *SpiTxData, uint8_t *SpiRxData, uint16_t spiTransferSize)
{
	HAL_StatusTypeDef	st;
	int8_t				error	= 0;

	osMutexWait( hSpiMutex1, osWaitForever );
	
	// CS Low
	//HAL_GPIO_WritePin( SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_RESET );	
	
#if SPI_DMA_MODE
	wTransferState1 = TRANSFER_WAIT;
	st = HAL_SPI_TransmitReceive_DMA( &hspi5, (uint8_t *)SpiTxData, (uint8_t *)SpiRxData, spiTransferSize );
	if( st != HAL_OK )
	{
		GLogE( "Error1...(%d)\r\n", st );
		error = -1;
	}

//	while( HAL_SPI_GetState( &hspi1 ) != HAL_SPI_STATE_READY ){}	
	while( wTransferState1 == TRANSFER_WAIT ){}
#else
	st = HAL_SPI_TransmitReceive( &hspi5, (uint8_t *)SpiTxData, (uint8_t *)SpiRxData, spiTransferSize, 5000 );
	if( st != HAL_OK)
	{
		GLogE( "Error2...(%d)\r\n", st );
		error = -1;
	}

//	while( HAL_SPI_GetState( hspi ) != HAL_SPI_STATE_READY ){}
#endif

	// CS High
	//HAL_GPIO_WritePin( SPI5_NSS_GPIO_Port, SPI5_NSS_Pin, GPIO_PIN_SET );	
	
	osMutexRelease( hSpiMutex1 );
	
	return error;
}
#endif