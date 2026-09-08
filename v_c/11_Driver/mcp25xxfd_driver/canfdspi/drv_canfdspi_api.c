/*******************************************************************************
 Title: .

  Company:
    Microchip Technology Inc.

  File Name:
    drv_canfdspi_api.c

  Summary:
    .

  Description:
    .
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


// *****************************************************************************
// *****************************************************************************
// Section: Included Files

#include "drv_canfdspi_api.h"
#include "drv_canfdspi_register.h"
#include "drv_canfdspi_defines.h"
#include "drv_spi.h"
#include "git_vci.h"
//#include "system_config.h"
//#include <xc.h>


// *****************************************************************************
// *****************************************************************************
// Section: Defines

#define CRCBASE    0xFFFF
#define CRCUPPER   1

#define SPI_DEFAULT_BUFFER_LENGTH	96


// *****************************************************************************
// *****************************************************************************
// Section: Variables

//! SPI Transmit buffer
uint8_t spiTransmitBuffer1[SPI_DEFAULT_BUFFER_LENGTH+32] = {0,};

//! SPI Receive buffer
uint8_t spiReceiveBuffer1[SPI_DEFAULT_BUFFER_LENGTH+32] = {0,};
extern stGITSetConfig		g_stGITSetConfig;
// *****************************************************************************
// *****************************************************************************
// Section: Reset

int8_t DRV_CANFDSPI_Reset(CANFDSPI_MODULE_ID index)
{
	uint16_t	spiTransferSize		= 2;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (uint8_t) (cINSTRUCTION_RESET << 4);
			spiTransmitBuffer1[1] = 0;

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);
		}
		break;
	}

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: SPI Access Functions

int8_t DRV_CANFDSPI_ReadByte(CANFDSPI_MODULE_ID index, uint16_t address, uint8_t *rxd)
{
	uint16_t	spiTransferSize		= 3;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (uint8_t) ((cINSTRUCTION_READ << 4) + ((address >> 8) & 0xF));
			spiTransmitBuffer1[1] = (uint8_t) (address & 0xFF);
			spiTransmitBuffer1[2] = 0;

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);

			// Update data
			*rxd = spiReceiveBuffer1[2];
		}
		break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_WriteByte(CANFDSPI_MODULE_ID index, uint16_t address, uint8_t txd)
{
	uint16_t	spiTransferSize		= 3;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (uint8_t) ((cINSTRUCTION_WRITE << 4) + ((address >> 8) & 0xF));
			spiTransmitBuffer1[1] = (uint8_t) (address & 0xFF);
			spiTransmitBuffer1[2] = txd;

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);
		}
		break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReadWord(CANFDSPI_MODULE_ID index, uint16_t address, uint32_t *rxd)
{
//	uint8_t		i;
//	uint32_t	x;
	uint16_t	spiTransferSize		= 6;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (uint8_t) ((cINSTRUCTION_READ << 4) + ((address >> 8) & 0xF));
			spiTransmitBuffer1[1] = (uint8_t) (address & 0xFF);

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);
			if (spiTransferError)
			{
				return spiTransferError;
			}

			// Update data
			*rxd  = ( (uint32_t)spiReceiveBuffer1[2] );
			*rxd += ( (uint32_t)spiReceiveBuffer1[3] ) <<  8;
			*rxd += ( (uint32_t)spiReceiveBuffer1[4] ) << 16; 
			*rxd += ( (uint32_t)spiReceiveBuffer1[5] ) << 24;
		}
		break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_WriteWord(CANFDSPI_MODULE_ID index, uint16_t address, uint32_t txd)
{
	uint8_t		i;
	uint16_t	spiTransferSize		= 6;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (uint8_t) ((cINSTRUCTION_WRITE << 4) + ((address >> 8) & 0xF));
			spiTransmitBuffer1[1] = (uint8_t) (address & 0xFF);

			// Split word into 4 bytes and add them to buffer
			for (i = 0; i < 4; i++)
			{
				spiTransmitBuffer1[i + 2] = (uint8_t) ((txd >> (i * 8)) & 0xFF);
			}

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);
		}
		break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReadHalfWord(CANFDSPI_MODULE_ID index, uint16_t address, uint16_t *rxd)
{
	uint8_t		i;
	uint32_t	x;
	uint16_t	spiTransferSize		= 4;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (uint8_t) ((cINSTRUCTION_READ << 4) + ((address >> 8) & 0xF));
			spiTransmitBuffer1[1] = (uint8_t) (address & 0xFF);

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);
			if (spiTransferError)
			{
				return spiTransferError;
			}

			// Update data
			*rxd = 0;
			for (i = 2; i < 4; i++)
			{
				x = (uint32_t) spiReceiveBuffer1[i];
				*rxd += x << ((i - 2)*8);
			}
		}
		break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_WriteHalfWord(CANFDSPI_MODULE_ID index, uint16_t address, uint16_t txd)
{
	uint8_t		i;
	uint16_t	spiTransferSize		= 4;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (uint8_t) ((cINSTRUCTION_WRITE << 4) + ((address >> 8) & 0xF));
			spiTransmitBuffer1[1] = (uint8_t) (address & 0xFF);

			// Split word into 2 bytes and add them to buffer
			for (i = 0; i < 2; i++)
			{
				spiTransmitBuffer1[i + 2] = (uint8_t) ((txd >> (i * 8)) & 0xFF);
			}

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);
		}
		break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReadByteArray(CANFDSPI_MODULE_ID index, uint16_t address, uint8_t *rxd, uint16_t nBytes)
{
//	uint16_t	i;
	uint16_t	spiTransferSize		= nBytes + 2;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (uint8_t) ((cINSTRUCTION_READ << 4) + ((address >> 8) & 0xF));
			spiTransmitBuffer1[1] = (uint8_t) (address & 0xFF);

			// Clear data
			memset( &spiTransmitBuffer1[2], 0, spiTransferSize );

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);

			// Update data
			memcpy( rxd, &spiReceiveBuffer1[2], nBytes );
		}
		break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_WriteByteArray(CANFDSPI_MODULE_ID index, uint16_t address, uint8_t *txd, uint16_t nBytes)
{
//	uint16_t	i;
	uint16_t	spiTransferSize		= nBytes + 2;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (uint8_t) ((cINSTRUCTION_WRITE << 4) + ((address >> 8) & 0xF));
			spiTransmitBuffer1[1] = (uint8_t) (address & 0xFF);

			// Add data
			memcpy( &spiTransmitBuffer1[2], &txd[0], spiTransferSize );

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);
		}
		break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReadWordArray(CANFDSPI_MODULE_ID index, uint16_t address, uint32_t *rxd, uint16_t nWords)
{
	uint16_t	i, n;
	REG_t		w;
	uint16_t	spiTransferSize		= nWords * 4 + 2;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (cINSTRUCTION_READ << 4) + ((address >> 8) & 0xF);
			spiTransmitBuffer1[1] = address & 0xFF;

			// Clear data
			memset( &spiTransmitBuffer1[2], 0, spiTransferSize );

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);
			if (spiTransferError)
			{
				return spiTransferError;
			}

			// Convert Byte array to Word array
			n = 2;
			for (i = 0; i < nWords; i++)
			{
				w.word		= 0;
				w.byte[0]	= spiReceiveBuffer1[n++];
				w.byte[1]	= spiReceiveBuffer1[n++];
				w.byte[2]	= spiReceiveBuffer1[n++];
				w.byte[3]	= spiReceiveBuffer1[n++];
				rxd[i]		= w.word;
			}
		}
		break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_WriteWordArray(CANFDSPI_MODULE_ID index, uint16_t address, uint32_t *txd, uint16_t nWords)
{
	uint16_t	i, n;
	REG_t		w;
	uint16_t	spiTransferSize		= nWords * 4 + 2;
	int8_t		spiTransferError	= 0;

	switch( index )
	{
		case DRV_CANFDSPI_INDEX_1 :
		{
			// Compose command
			spiTransmitBuffer1[0] = (cINSTRUCTION_WRITE << 4) + ((address >> 8) & 0xF);
			spiTransmitBuffer1[1] = address & 0xFF;

			// Convert ByteArray to word array
			n = 2;
			for (i = 0; i < nWords; i++)
			{
				w.word = txd[i];
				spiTransmitBuffer1[n++]	= w.byte[0];
				spiTransmitBuffer1[n++]	= w.byte[1];
				spiTransmitBuffer1[n++]	= w.byte[2];
				spiTransmitBuffer1[n++]	= w.byte[3];
			}

			spiTransferError = DRV_SPI_TransferData1(index, spiTransmitBuffer1, spiReceiveBuffer1, spiTransferSize);
		}
		break;
	}

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: Configuration

int8_t DRV_CANFDSPI_Configure(CANFDSPI_MODULE_ID index, CAN_CONFIG* config)
{
	REG_CiCON	ciCon;
	int8_t		spiTransferError = 0;

	ciCon.word = canControlResetValues[cREGADDR_CiCON / 4];

	ciCon.bF.DNetFilterCount				= config->DNetFilterCount;
	ciCon.bF.IsoCrcEnable					= config->IsoCrcEnable;
	ciCon.bF.ProtocolExceptionEventDisable	= config->ProtocolExpectionEventDisable;
	ciCon.bF.WakeUpFilterEnable				= config->WakeUpFilterEnable;
	ciCon.bF.WakeUpFilterTime				= config->WakeUpFilterTime;
	ciCon.bF.BitRateSwitchDisable			= config->BitRateSwitchDisable;
	ciCon.bF.RestrictReTxAttempts			= config->RestrictReTxAttempts;
	ciCon.bF.EsiInGatewayMode				= config->EsiInGatewayMode;
	ciCon.bF.SystemErrorToListenOnly		= config->SystemErrorToListenOnly;
	ciCon.bF.StoreInTEF						= config->StoreInTEF;
	ciCon.bF.TXQEnable						= config->TXQEnable;
	ciCon.bF.TxBandWidthSharing				= config->TxBandWidthSharing;

	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiCON, ciCon.word);
	if (spiTransferError)
	{
		return -1;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ConfigureObjectReset(CAN_CONFIG* config)
{
	REG_CiCON ciCon;

	ciCon.word = canControlResetValues[cREGADDR_CiCON / 4];

	config->DNetFilterCount					= ciCon.bF.DNetFilterCount;
	config->IsoCrcEnable					= ciCon.bF.IsoCrcEnable;
	config->ProtocolExpectionEventDisable	= ciCon.bF.ProtocolExceptionEventDisable;
	config->WakeUpFilterEnable				= ciCon.bF.WakeUpFilterEnable;
	config->WakeUpFilterTime				= ciCon.bF.WakeUpFilterTime;
	config->BitRateSwitchDisable			= ciCon.bF.BitRateSwitchDisable;
	config->RestrictReTxAttempts			= ciCon.bF.RestrictReTxAttempts;
	config->EsiInGatewayMode				= ciCon.bF.EsiInGatewayMode;
	config->SystemErrorToListenOnly			= ciCon.bF.SystemErrorToListenOnly;
	config->StoreInTEF						= ciCon.bF.StoreInTEF;
	config->TXQEnable						= ciCon.bF.TXQEnable;
	config->TxBandWidthSharing				= ciCon.bF.TxBandWidthSharing;

	return 0;
}

// *****************************************************************************
// *****************************************************************************
// Section: Operating mode

int8_t DRV_CANFDSPI_OperationModeSelect(CANFDSPI_MODULE_ID index, CAN_OPERATION_MODE opMode)
{
	uint8_t	d = 0;
	int8_t	spiTransferError = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_CiCON + 3, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d &= ~0x07;
	d |= opMode;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_CiCON + 3, d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

CAN_OPERATION_MODE DRV_CANFDSPI_OperationModeGet(CANFDSPI_MODULE_ID index)
{
	CAN_OPERATION_MODE	mode = CAN_INVALID_MODE;

	uint8_t	d = 0;
	int8_t	spiTransferError = 0;

	// Read Opmode
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_CiCON + 2, &d);
	if (spiTransferError)
	{
		return CAN_INVALID_MODE;
	}

	// Get Opmode bits
	d = (d >> 5) & 0x7;

	// Decode Opmode
	switch (d)
	{
		case CAN_NORMAL_MODE:					mode = CAN_NORMAL_MODE;						break;
		case CAN_SLEEP_MODE:					mode = CAN_SLEEP_MODE;						break;
		case CAN_INTERNAL_LOOPBACK_MODE:		mode = CAN_INTERNAL_LOOPBACK_MODE;			break;
		case CAN_EXTERNAL_LOOPBACK_MODE:		mode = CAN_EXTERNAL_LOOPBACK_MODE;			break;
		case CAN_LISTEN_ONLY_MODE:				mode = CAN_LISTEN_ONLY_MODE;				break;
		case CAN_CONFIGURATION_MODE:			mode = CAN_CONFIGURATION_MODE;				break;
		case CAN_CLASSIC_MODE:					mode = CAN_CLASSIC_MODE;					break;
		case CAN_RESTRICTED_MODE:				mode = CAN_RESTRICTED_MODE;					break;
		default:								mode = CAN_INVALID_MODE;					break;
	}

	return mode;
}

int8_t DRV_CANFDSPI_WakeUp(CANFDSPI_MODULE_ID index)
{
	int8_t	spiTransferError = 0;
	uint8_t	d = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_OSC, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d &= 0xFB;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_OSC, d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_LowPowerModeEnable(CANFDSPI_MODULE_ID index)
{
	int8_t	spiTransferError = 0;
	uint8_t	d = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_OSC, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d |= 0x08;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_OSC, d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_LowPowerModeDisable(CANFDSPI_MODULE_ID index)
{
	int8_t	spiTransferError = 0;
	uint8_t	d = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_OSC, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d &= ~0x08;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_OSC, d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: CAN Transmit

int8_t DRV_CANFDSPI_TransmitChannelConfigure(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_CONFIG* config)
{
	int8_t		spiTransferError = 0;
	uint16_t	a = 0;

	// Setup FIFO
	REG_CiFIFOCON	ciFifoCon;
	ciFifoCon.word = canFifoResetValues[0];

	ciFifoCon.txBF.TxEnable		= 1;
	ciFifoCon.txBF.FifoSize		= config->FifoSize;
	ciFifoCon.txBF.PayLoadSize	= config->PayLoadSize;
	ciFifoCon.txBF.TxAttempts	= config->TxAttempts;
	ciFifoCon.txBF.TxPriority	= config->TxPriority;
	ciFifoCon.txBF.RTREnable	= config->RTREnable;

	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_WriteWord(index, a, ciFifoCon.word);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitChannelConfigureObjectReset(CAN_TX_FIFO_CONFIG* config)
{
	REG_CiFIFOCON ciFifoCon;
	ciFifoCon.word = canFifoResetValues[0];

	config->RTREnable	= ciFifoCon.txBF.RTREnable;
	config->TxPriority	= ciFifoCon.txBF.TxPriority;
	config->TxAttempts	= ciFifoCon.txBF.TxAttempts;
	config->FifoSize	= ciFifoCon.txBF.FifoSize;
	config->PayLoadSize	= ciFifoCon.txBF.PayLoadSize;

	return 0;
}

int8_t DRV_CANFDSPI_TransmitQueueConfigure(CANFDSPI_MODULE_ID index, CAN_TX_QUEUE_CONFIG* config)
{
	int8_t		spiTransferError = 0;
	uint16_t	a = 0;

	// Setup FIFO
	REG_CiTXQCON ciFifoCon;
	ciFifoCon.word = canFifoResetValues[0];

	ciFifoCon.txBF.TxEnable		= 1;
	ciFifoCon.txBF.FifoSize		= config->FifoSize;
	ciFifoCon.txBF.PayLoadSize	= config->PayLoadSize;
	ciFifoCon.txBF.TxAttempts	= config->TxAttempts;
	ciFifoCon.txBF.TxPriority	= config->TxPriority;

	a = cREGADDR_CiTXQCON;
	spiTransferError = DRV_CANFDSPI_WriteWord(index, a, ciFifoCon.word);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitQueueConfigureObjectReset(CAN_TX_QUEUE_CONFIG* config)
{
	REG_CiFIFOCON ciFifoCon;
	ciFifoCon.word = canFifoResetValues[0];

	config->TxPriority	= ciFifoCon.txBF.TxPriority;
	config->TxAttempts	= ciFifoCon.txBF.TxAttempts;
	config->FifoSize	= ciFifoCon.txBF.FifoSize;
	config->PayLoadSize	= ciFifoCon.txBF.PayLoadSize;

	return 0;
}

int8_t DRV_CANFDSPI_TransmitChannelLoad(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_TX_MSGOBJ* txObj, uint8_t *txd, uint32_t txdNumBytes, bool flush)
{
	uint16_t	a;
	uint32_t	fifoReg[3];
	uint32_t	dataBytesInObject;

	REG_CiFIFOCON ciFifoCon;
	__attribute__((unused)) REG_CiFIFOSTA ciFifoSta;
	REG_CiFIFOUA ciFifoUa;

	int8_t		spiTransferError = 0;

	// Get FIFO registers
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_ReadWordArray(index, a, fifoReg, 3);
	if (spiTransferError)
	{
		return -1;
	}

	// Check that it is a transmit buffer
	ciFifoCon.word = fifoReg[0];
	if (!ciFifoCon.txBF.TxEnable)
	{
		return -2;
	}

	// Check that DLC is big enough for data
	dataBytesInObject = DRV_CANFDSPI_DlcToDataBytes((CAN_DLC) txObj->bF.ctrl.DLC);
	if (dataBytesInObject < txdNumBytes)
	{
		return -3;
	}

	// Get status
	ciFifoSta.word = fifoReg[1];

	// Get address
	ciFifoUa.word = fifoReg[2];
	a = ciFifoUa.bF.UserAddress;
	a += cRAMADDR_START;

	uint8_t txBuffer[MAX_MSG_SIZE];

	txBuffer[0] = txObj->byte[0]; //not using 'for' to reduce no of instructions
	txBuffer[1] = txObj->byte[1];
	txBuffer[2] = txObj->byte[2];
	txBuffer[3] = txObj->byte[3];
	txBuffer[4] = txObj->byte[4];
	txBuffer[5] = txObj->byte[5];
	txBuffer[6] = txObj->byte[6];
	txBuffer[7] = txObj->byte[7];

	uint8_t i;
	for (i = 0; i < txdNumBytes; i++)
	{
		txBuffer[i + 8] = txd[i];
	}

	// Make sure we write a multiple of 4 bytes to RAM
	uint16_t n = 0;
	uint8_t j = 0;

//	if (txdNumBytes % 4)
	if( txdNumBytes & 0x03 )
	{
		// Need to add bytes
//		n = 4 - (txdNumBytes % 4);
		n = 4 - ( txdNumBytes & 0x03 );
		i = txdNumBytes + 8;

		for (j = 0; j < n; j++)
		{
			txBuffer[i + 8 + j] = 0;
		}
	}

	spiTransferError = DRV_CANFDSPI_WriteByteArray(index, a, txBuffer, txdNumBytes + 8 + n);
	if (spiTransferError)
	{
		return -4;
	}

	// Set UINC and TXREQ
	spiTransferError = DRV_CANFDSPI_TransmitChannelUpdate(index, channel, flush);
	if (spiTransferError)
	{
		return -5;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitChannelFlush(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel)
{
	uint8_t d = 0;
	uint16_t a = 0;
	int8_t spiTransferError = 0;

	// Address of TXREQ
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);
	a += 1;

	// Set TXREQ
	d = 0x02;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, d);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitChannelStatusGet(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_STATUS* status)
{
	uint16_t a = 0;
	uint32_t sta = 0;
	uint32_t fifoReg[2];
	REG_CiFIFOSTA ciFifoSta;
	REG_CiFIFOCON ciFifoCon;
	int8_t spiTransferError = 0;

	// Get FIFO registers
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_ReadWordArray(index, a, fifoReg, 2);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	ciFifoCon.word = fifoReg[0];
	ciFifoSta.word = fifoReg[1];

	// Update status
	sta = ciFifoSta.byte[0];

	if (ciFifoCon.txBF.TxRequest)
	{
		sta |= CAN_TX_FIFO_TRANSMITTING;
	}

	*status = (CAN_TX_FIFO_STATUS) (sta & CAN_TX_FIFO_STATUS_MASK);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitChannelReset(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel)
{
	return DRV_CANFDSPI_ReceiveChannelReset(index, channel);
}

int8_t DRV_CANFDSPI_TransmitChannelUpdate(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, bool flush)
{
	uint16_t a;
	REG_CiFIFOCON ciFifoCon;
	int8_t spiTransferError = 0;

	// Set UINC
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET) + 1; // Byte that contains FRESET
	ciFifoCon.word = 0;
	ciFifoCon.txBF.UINC = 1;

	// Set TXREQ
	if (flush)
	{
		ciFifoCon.txBF.TxRequest = 1;
	}

	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, ciFifoCon.byte[1]);
	if (spiTransferError)
	{
		return -1;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitRequestSet(CANFDSPI_MODULE_ID index, CAN_TXREQ_CHANNEL txreq)
{
	int8_t spiTransferError = 0;

	// Write TXREQ register
	uint32_t w = txreq;

	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiTXREQ, w);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitRequestGet(CANFDSPI_MODULE_ID index, uint32_t* txreq)
{
	int8_t spiTransferError = 0;

	spiTransferError = DRV_CANFDSPI_ReadWord(index, cREGADDR_CiTXREQ, txreq);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitChannelAbort(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel)
{
	uint16_t a;
	uint8_t d;
	int8_t spiTransferError = 0;

	// Address
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);
	a += 1; // byte address of TXREQ

	// Clear TXREQ
	d = 0x00;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, d);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitAbortAll(CANFDSPI_MODULE_ID index)
{
	uint8_t d;
	int8_t spiTransferError = 0;

	// Read CiCON byte 3
	spiTransferError = DRV_CANFDSPI_ReadByte(index, (cREGADDR_CiCON + 3), &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d |= 0x8;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, (cREGADDR_CiCON + 3), d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitBandWidthSharingSet(CANFDSPI_MODULE_ID index, CAN_TX_BANDWITH_SHARING txbws)
{
	uint8_t d = 0;
	int8_t spiTransferError = 0;

	// Read CiCON byte 3
	spiTransferError = DRV_CANFDSPI_ReadByte(index, (cREGADDR_CiCON + 3), &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d &= 0x0f;
	d |= (txbws << 4);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, (cREGADDR_CiCON + 3), d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: CAN Receive

int8_t DRV_CANFDSPI_FilterObjectConfigure(CANFDSPI_MODULE_ID index, CAN_FILTER filter, CAN_FILTEROBJ_ID* id)
{
	uint16_t a;
	REG_CiFLTOBJ fObj;
	int8_t spiTransferError = 0;

	// Setup
	fObj.word = 0;
	fObj.bF = *id;
	a = cREGADDR_CiFLTOBJ + (filter * CiFILTER_OFFSET);

	spiTransferError = DRV_CANFDSPI_WriteWord(index, a, fObj.word);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_FilterMaskConfigure(CANFDSPI_MODULE_ID index, CAN_FILTER filter, CAN_MASKOBJ_ID* mask)
{
	uint16_t a;
	REG_CiMASK mObj;
	int8_t spiTransferError = 0;

	// Setup
	mObj.word = 0;
	mObj.bF = *mask;
	a = cREGADDR_CiMASK + (filter * CiFILTER_OFFSET);

	spiTransferError = DRV_CANFDSPI_WriteWord(index, a, mObj.word);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_FilterToFifoLink(CANFDSPI_MODULE_ID index, CAN_FILTER filter, CAN_FIFO_CHANNEL channel, bool enable)
{
	uint16_t a;
	REG_CiFLTCON_BYTE fCtrl;
	int8_t spiTransferError = 0;

	// Enable
	if (enable)				fCtrl.bF.Enable = 1;
	else					fCtrl.bF.Enable = 0;

	// Link
	fCtrl.bF.BufferPointer = channel;
	a = cREGADDR_CiFLTCON + filter;

	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, fCtrl.byte);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_FilterEnable(CANFDSPI_MODULE_ID index, CAN_FILTER filter)
{
	uint16_t a;
	REG_CiFLTCON_BYTE fCtrl;
	int8_t spiTransferError = 0;

	// Read
	a = cREGADDR_CiFLTCON + filter;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &fCtrl.byte);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	fCtrl.bF.Enable = 1;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, fCtrl.byte);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_FilterDisable(CANFDSPI_MODULE_ID index, CAN_FILTER filter)
{
	uint16_t a;
	REG_CiFLTCON_BYTE fCtrl;
	int8_t spiTransferError = 0;

	// Read
	a = cREGADDR_CiFLTCON + filter;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &fCtrl.byte);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	fCtrl.bF.Enable = 0;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, fCtrl.byte);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_DeviceNetFilterCountSet(CANFDSPI_MODULE_ID index, CAN_DNET_FILTER_SIZE dnfc)
{
	uint8_t d = 0;
	int8_t spiTransferError = 0;

	// Read CiCON byte 0
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_CiCON, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d &= 0x1f;
	d |= dnfc;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_CiCON, d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveChannelConfigure(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_CONFIG* config)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	if (channel == CAN_TXQUEUE_CH0)
	{
		return -100;
	}

	// Setup FIFO
	REG_CiFIFOCON ciFifoCon;
	ciFifoCon.word = canFifoResetValues[0];

	ciFifoCon.rxBF.TxEnable				= 0;
	ciFifoCon.rxBF.FifoSize				= config->FifoSize;
	ciFifoCon.rxBF.PayLoadSize			= config->PayLoadSize;
	ciFifoCon.rxBF.RxTimeStampEnable	= config->RxTimeStampEnable;

	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_WriteWord(index, a, ciFifoCon.word);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(CAN_RX_FIFO_CONFIG* config)
{
	REG_CiFIFOCON ciFifoCon;
	ciFifoCon.word = canFifoResetValues[0];

	config->FifoSize			= ciFifoCon.rxBF.FifoSize;
	config->PayLoadSize			= ciFifoCon.rxBF.PayLoadSize;
	config->RxTimeStampEnable	= ciFifoCon.rxBF.RxTimeStampEnable;

	return 0;
}

int8_t DRV_CANFDSPI_ReceiveChannelStatusGet(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_STATUS* status)
{
	uint16_t a;
	REG_CiFIFOSTA ciFifoSta;
	int8_t spiTransferError = 0;

	// Read
	ciFifoSta.word = 0;
	a = cREGADDR_CiFIFOSTA + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &ciFifoSta.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*status = (CAN_RX_FIFO_STATUS) (ciFifoSta.byte[0] & 0x0F);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveMessageGet(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_RX_MSGOBJ* rxObj, uint8_t *rxd, uint8_t nBytes)
{
	uint8_t n = 0;
//	uint8_t i = 0;
	uint16_t a;
	uint32_t fifoReg[3];
	REG_CiFIFOCON ciFifoCon;
	__attribute__((unused)) REG_CiFIFOSTA ciFifoSta;
	REG_CiFIFOUA ciFifoUa;
	int8_t spiTransferError = 0;

	// Get FIFO registers
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_ReadWordArray(index, a, fifoReg, 3);
	if (spiTransferError)
	{
		return -1;
	}

	// Check that it is a receive buffer
	ciFifoCon.word = fifoReg[0];
	if (ciFifoCon.rxBF.TxEnable)
	{
		return -2;
	}

	// Get Status
	ciFifoSta.word = fifoReg[1];

	// Get address
	ciFifoUa.word = fifoReg[2];
	a = ciFifoUa.bF.UserAddress;
	a += cRAMADDR_START;

	// Number of bytes to read
	n = nBytes + 8; // Add 8 header bytes

	if (ciFifoCon.rxBF.RxTimeStampEnable)
	{
		n += 4; // Add 4 time stamp bytes
	}

	// Make sure we read a multiple of 4 bytes from RAM
#if 0
	if (n % 4)
	{
		n = n + 4 - (n % 4);
	}
#else
	if( n & 0x03 )
	{
		n = n + 4;
		n = n & 0xFC;
	}
#endif

	// Read rxObj using one access
	uint8_t ba[MAX_MSG_SIZE];

	if (n > MAX_MSG_SIZE)
	{
		n = MAX_MSG_SIZE;
	}

	spiTransferError = DRV_CANFDSPI_ReadByteArray(index, a, ba, n);
	if (spiTransferError)
	{
		return -3;
	}

	// Assign message header
	REG_t myReg;

	myReg.byte[0] = ba[0];
	myReg.byte[1] = ba[1];
	myReg.byte[2] = ba[2];
	myReg.byte[3] = ba[3];
	rxObj->word[0] = myReg.word;

	myReg.byte[0] = ba[4];
	myReg.byte[1] = ba[5];
	myReg.byte[2] = ba[6];
	myReg.byte[3] = ba[7];
	rxObj->word[1] = myReg.word;

	if (ciFifoCon.rxBF.RxTimeStampEnable)
	{
		myReg.byte[0] = ba[8];
		myReg.byte[1] = ba[9];
		myReg.byte[2] = ba[10];
		myReg.byte[3] = ba[11];
		rxObj->word[2] = myReg.word;

		// Assign message data
		memcpy( rxd, &ba[12], nBytes );
//		for (i = 0; i < nBytes; i++)
//		{
//			rxd[i] = ba[i + 12];
//		}
	}
	else
	{
		rxObj->word[2] = 0;

		// Assign message data
		memcpy( rxd, &ba[8], nBytes );
//		for (i = 0; i < nBytes; i++)
//		{
//			rxd[i] = ba[i + 8];
//		}
	}

	// UINC channel
	spiTransferError = DRV_CANFDSPI_ReceiveChannelUpdate(index, channel);
	if (spiTransferError)
	{
		return -4;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveChannelReset(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel)
{
	uint16_t a = 0;
	REG_CiFIFOCON ciFifoCon;
	int8_t spiTransferError = 0;

	// Address and data
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET) + 1; // Byte that contains FRESET
	ciFifoCon.word = 0;
	ciFifoCon.rxBF.FRESET = 1;

	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, ciFifoCon.byte[1]);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveChannelUpdate(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel)
{
	uint16_t a = 0;
	REG_CiFIFOCON ciFifoCon;
	int8_t spiTransferError = 0;
	ciFifoCon.word = 0;

	// Set UINC
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET) + 1; // Byte that contains FRESET
	ciFifoCon.rxBF.UINC = 1;

	// Write byte
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, ciFifoCon.byte[1]);

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: Module Events

int8_t DRV_CANFDSPI_ModuleEventGet(CANFDSPI_MODULE_ID index, CAN_MODULE_EVENT* flags)
{
	int8_t spiTransferError = 0;

	// Read Interrupt flags
	REG_CiINTFLAG intFlags;
	intFlags.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadHalfWord(index, cREGADDR_CiINTFLAG, &intFlags.word);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*flags = (CAN_MODULE_EVENT) (intFlags.word & CAN_ALL_EVENTS);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ModuleEventEnable(CANFDSPI_MODULE_ID index, CAN_MODULE_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Interrupt Enables
	a = cREGADDR_CiINTENABLE;
	REG_CiINTENABLE intEnables;
	intEnables.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadHalfWord(index, a, &intEnables.word);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	intEnables.word |= (flags & CAN_ALL_EVENTS);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteHalfWord(index, a, intEnables.word);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ModuleEventDisable(CANFDSPI_MODULE_ID index, CAN_MODULE_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Interrupt Enables
	a = cREGADDR_CiINTENABLE;
	REG_CiINTENABLE intEnables;
	intEnables.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadHalfWord(index, a, &intEnables.word);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	intEnables.word &= ~(flags & CAN_ALL_EVENTS);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteHalfWord(index, a, intEnables.word);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ModuleEventClear(CANFDSPI_MODULE_ID index, CAN_MODULE_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Interrupt flags
	a = cREGADDR_CiINTFLAG;
	REG_CiINTFLAG intFlags;
	intFlags.word = 0;

	// Write 1 to all flags except the ones that we want to clear
	// Writing a 1 will not set the flag
	// Only writing a 0 will clear it
	// The flags are HS/C
	intFlags.word = CAN_ALL_EVENTS;
	intFlags.word &= ~flags;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteHalfWord(index, a, intFlags.word);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ModuleEventRxCodeGet(CANFDSPI_MODULE_ID index, CAN_RXCODE* rxCode)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;
	uint8_t rxCodeByte = 0;

	// Read
	a = cREGADDR_CiVEC + 3;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &rxCodeByte);
	if (spiTransferError)
	{
		return -1;
	}

	// Decode data
	// 0x40 = "no interrupt" (CAN_FIFO_CIVEC_NOINTERRUPT)
	if ((rxCodeByte < CAN_RXCODE_TOTAL_CHANNELS) || (rxCodeByte == CAN_RXCODE_NO_INT))
	{
		*rxCode = (CAN_RXCODE) rxCodeByte;
	}
	else
	{
		*rxCode = CAN_RXCODE_RESERVED; // shouldn't get here
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ModuleEventTxCodeGet(CANFDSPI_MODULE_ID index, CAN_TXCODE* txCode)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;
	uint8_t txCodeByte = 0;

	// Read
	a = cREGADDR_CiVEC + 2;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &txCodeByte);
	if (spiTransferError)
	{
		return -1;
	}

	// Decode data
	// 0x40 = "no interrupt" (CAN_FIFO_CIVEC_NOINTERRUPT)
	if ((txCodeByte < CAN_TXCODE_TOTAL_CHANNELS) || (txCodeByte == CAN_TXCODE_NO_INT))
	{
		*txCode = (CAN_TXCODE) txCodeByte;
	}
	else
	{
		*txCode = CAN_TXCODE_RESERVED; // shouldn't get here
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ModuleEventFilterHitGet(CANFDSPI_MODULE_ID index, CAN_FILTER* filterHit)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;
	uint8_t filterHitByte = 0;

	// Read
	a = cREGADDR_CiVEC + 1;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &filterHitByte);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*filterHit = (CAN_FILTER) filterHitByte;

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ModuleEventIcodeGet(CANFDSPI_MODULE_ID index, CAN_ICODE* icode)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;
	uint8_t icodeByte = 0;

	// Read
	a = cREGADDR_CiVEC;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &icodeByte);
	if (spiTransferError)
	{
		return -1;
	}

	// Decode
	if ((icodeByte < CAN_ICODE_RESERVED) && ((icodeByte < CAN_ICODE_TOTAL_CHANNELS) || (icodeByte >= CAN_ICODE_NO_INT)))
	{
		*icode = (CAN_ICODE) icodeByte;
	}
	else
	{
		*icode = CAN_ICODE_RESERVED; // shouldn't get here
	}

	return spiTransferError;
}

// *****************************************************************************
// *****************************************************************************
// Section: Transmit FIFO Events

int8_t DRV_CANFDSPI_TransmitChannelEventGet(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_EVENT* flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Interrupt flags
	REG_CiFIFOSTA ciFifoSta;
	ciFifoSta.word = 0;
	a = cREGADDR_CiFIFOSTA + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &ciFifoSta.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*flags = (CAN_TX_FIFO_EVENT) (ciFifoSta.byte[0] & CAN_TX_FIFO_ALL_EVENTS);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitEventGet(CANFDSPI_MODULE_ID index, uint32_t* txif)
{
	int8_t spiTransferError = 0;

	spiTransferError = DRV_CANFDSPI_ReadWord(index, cREGADDR_CiTXIF, txif);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitEventAttemptGet(CANFDSPI_MODULE_ID index, uint32_t* txatif)
{
	int8_t spiTransferError = 0;

	spiTransferError = DRV_CANFDSPI_ReadWord(index, cREGADDR_CiTXATIF, txatif);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitChannelIndexGet(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, uint8_t* idx)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read index
	REG_CiFIFOSTA ciFifoSta;
	ciFifoSta.word = 0;
	a = cREGADDR_CiFIFOSTA + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_ReadWord(index, a, &ciFifoSta.word);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*idx = ciFifoSta.txBF.FifoIndex;

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitChannelEventEnable(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Interrupt Enables
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);
	REG_CiFIFOCON ciFifoCon;
	ciFifoCon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &ciFifoCon.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	ciFifoCon.byte[0] |= (flags & CAN_TX_FIFO_ALL_EVENTS);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, ciFifoCon.byte[0]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitChannelEventDisable(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Interrupt Enables
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);
	REG_CiFIFOCON ciFifoCon;
	ciFifoCon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &ciFifoCon.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	ciFifoCon.byte[0] &= ~(flags & CAN_TX_FIFO_ALL_EVENTS);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, ciFifoCon.byte[0]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TransmitChannelEventAttemptClear(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Interrupt Enables
	a = cREGADDR_CiFIFOSTA + (channel * CiFIFO_OFFSET);
	REG_CiFIFOSTA ciFifoSta;
	ciFifoSta.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &ciFifoSta.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	ciFifoSta.byte[0] &= ~CAN_TX_FIFO_ATTEMPTS_EXHAUSTED_EVENT;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, ciFifoSta.byte[0]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: Receive FIFO Events

int8_t DRV_CANFDSPI_ReceiveChannelEventGet(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_EVENT* flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	if (channel == CAN_TXQUEUE_CH0) return -100;

	// Read Interrupt flags
	REG_CiFIFOSTA ciFifoSta;
	ciFifoSta.word = 0;
	a = cREGADDR_CiFIFOSTA + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &ciFifoSta.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*flags = (CAN_RX_FIFO_EVENT) (ciFifoSta.byte[0] & CAN_RX_FIFO_ALL_EVENTS);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveEventGet(CANFDSPI_MODULE_ID index, uint32_t* rxif)
{
	int8_t spiTransferError = 0;

	spiTransferError = DRV_CANFDSPI_ReadWord(index, cREGADDR_CiRXIF, rxif);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveEventOverflowGet(CANFDSPI_MODULE_ID index, uint32_t* rxovif)
{
	int8_t spiTransferError = 0;

	spiTransferError = DRV_CANFDSPI_ReadWord(index, cREGADDR_CiRXOVIF, rxovif);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveChannelIndexGet(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, uint8_t* idx)
{
	return DRV_CANFDSPI_TransmitChannelIndexGet(index, channel, idx);
}

int8_t DRV_CANFDSPI_ReceiveChannelEventEnable(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	if (channel == CAN_TXQUEUE_CH0) return -100;

	// Read Interrupt Enables
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);
	REG_CiFIFOCON ciFifoCon;
	ciFifoCon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &ciFifoCon.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	ciFifoCon.byte[0] |= (flags & CAN_RX_FIFO_ALL_EVENTS);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, ciFifoCon.byte[0]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveChannelEventDisable(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	if (channel == CAN_TXQUEUE_CH0) return -100;

	// Read Interrupt Enables
	a = cREGADDR_CiFIFOCON + (channel * CiFIFO_OFFSET);
	REG_CiFIFOCON ciFifoCon;
	ciFifoCon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &ciFifoCon.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	ciFifoCon.byte[0] &= ~(flags & CAN_RX_FIFO_ALL_EVENTS);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, ciFifoCon.byte[0]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ReceiveChannelEventOverflowClear(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	if (channel == CAN_TXQUEUE_CH0) return -100;

	// Read Interrupt Flags
	REG_CiFIFOSTA ciFifoSta;
	ciFifoSta.word = 0;
	a = cREGADDR_CiFIFOSTA + (channel * CiFIFO_OFFSET);

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &ciFifoSta.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	ciFifoSta.byte[0] &= ~(CAN_RX_FIFO_OVERFLOW_EVENT);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, ciFifoSta.byte[0]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: Error Handling

int8_t DRV_CANFDSPI_ErrorCountTransmitGet(CANFDSPI_MODULE_ID index, uint8_t* tec)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Error count
	a = cREGADDR_CiTREC + 1;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, tec);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ErrorCountReceiveGet(CANFDSPI_MODULE_ID index, uint8_t* rec)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Error count
	a = cREGADDR_CiTREC;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, rec);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ErrorStateGet(CANFDSPI_MODULE_ID index, CAN_ERROR_STATE* flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Error state
	a = cREGADDR_CiTREC + 2;
	uint8_t f = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &f);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*flags = (CAN_ERROR_STATE) (f & CAN_ERROR_ALL);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_ErrorCountStateGet(CANFDSPI_MODULE_ID index, uint8_t* tec, uint8_t* rec, CAN_ERROR_STATE* flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Error
	a = cREGADDR_CiTREC;
	REG_CiTREC ciTrec;
	ciTrec.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadWord(index, a, &ciTrec.word);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*tec = ciTrec.byte[1];
	*rec = ciTrec.byte[0];
	*flags = (CAN_ERROR_STATE) (ciTrec.byte[2] & CAN_ERROR_ALL);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_BusDiagnosticsGet(CANFDSPI_MODULE_ID index, CAN_BUS_DIAGNOSTIC* bd)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read diagnostic registers all in one shot
	a = cREGADDR_CiBDIAG0;
	uint32_t w[2];

	spiTransferError = DRV_CANFDSPI_ReadWordArray(index, a, w, 2);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	CAN_BUS_DIAGNOSTIC b;
	b.word[0] = w[0];
	b.word[1] = w[1] & 0x0000ffff;
	b.word[2] = (w[1] >> 16) & 0x0000ffff;
	*bd = b;

	return spiTransferError;
}

int8_t DRV_CANFDSPI_BusDiagnosticsClear(CANFDSPI_MODULE_ID index)
{
	int8_t spiTransferError = 0;
	uint8_t a = 0;

	// Clear diagnostic registers all in one shot
	a = cREGADDR_CiBDIAG0;
	uint32_t w[2];
	w[0] = 0;
	w[1] = 0;

	spiTransferError = DRV_CANFDSPI_WriteWordArray(index, a, w, 2);

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: ECC

int8_t DRV_CANFDSPI_EccEnable(CANFDSPI_MODULE_ID index)
{
	int8_t spiTransferError = 0;
	uint8_t d = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_ECCCON, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d |= 0x01;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_ECCCON, d);
	if (spiTransferError)
	{
		return -2;
	}

	return 0;
}

int8_t DRV_CANFDSPI_EccDisable(CANFDSPI_MODULE_ID index)
{
	int8_t spiTransferError = 0;
	uint8_t d = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_ECCCON, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d &= ~0x01;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_ECCCON, d);
	if (spiTransferError)
	{
		return -2;
	}

	return 0;
}

int8_t DRV_CANFDSPI_EccEventGet(CANFDSPI_MODULE_ID index, CAN_ECC_EVENT* flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Interrupt flags
	uint8_t eccStatus = 0;
	a = cREGADDR_ECCSTA;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &eccStatus);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*flags = (CAN_ECC_EVENT) (eccStatus & CAN_ECC_ALL_EVENTS);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_EccParitySet(CANFDSPI_MODULE_ID index, uint8_t parity)
{
	int8_t spiTransferError = 0;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_ECCCON + 1, parity);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_EccParityGet(CANFDSPI_MODULE_ID index, uint8_t* parity)
{
	int8_t spiTransferError = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_ECCCON + 1, parity);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_EccErrorAddressGet(CANFDSPI_MODULE_ID index, uint16_t* a)
{
	int8_t spiTransferError = 0;
	REG_ECCSTA reg;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadWord(index, cREGADDR_ECCSTA, &reg.word);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*a = reg.bF.ErrorAddress;

	return spiTransferError;
}

int8_t DRV_CANFDSPI_EccEventEnable(CANFDSPI_MODULE_ID index, CAN_ECC_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_ECCCON;
	uint8_t eccInterrupts = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &eccInterrupts);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	eccInterrupts |= (flags & CAN_ECC_ALL_EVENTS);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, eccInterrupts);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_EccEventDisable(CANFDSPI_MODULE_ID index, CAN_ECC_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_ECCCON;
	uint8_t eccInterrupts = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &eccInterrupts);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	eccInterrupts &= ~(flags & CAN_ECC_ALL_EVENTS);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, eccInterrupts);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_EccEventClear(CANFDSPI_MODULE_ID index, CAN_ECC_EVENT flags)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_ECCSTA;
	uint8_t eccStat = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &eccStat);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	eccStat &= ~(flags & CAN_ECC_ALL_EVENTS);

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, eccStat);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_RamInit(CANFDSPI_MODULE_ID index, uint8_t d)
{
	uint8_t txd[SPI_DEFAULT_BUFFER_LENGTH];
	uint32_t k;
	int8_t spiTransferError = 0;

	// Prepare data
	for (k = 0; k < SPI_DEFAULT_BUFFER_LENGTH; k++)
	{
		txd[k] = d;
	}

	uint16_t a = cRAMADDR_START;

	for (k = 0; k < (cRAM_SIZE / (SPI_DEFAULT_BUFFER_LENGTH)); k++)
	{
		spiTransferError = DRV_CANFDSPI_WriteByteArray(index, a, txd, SPI_DEFAULT_BUFFER_LENGTH);
		if (spiTransferError)
		{
			return -1;
		}
		a += (SPI_DEFAULT_BUFFER_LENGTH);
	}

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: Time Stamp

int8_t DRV_CANFDSPI_TimeStampEnable(CANFDSPI_MODULE_ID index)
{
	int8_t spiTransferError = 0;
	uint8_t d = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_CiTSCON + 2, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d |= 0x01;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_CiTSCON + 2, d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TimeStampDisable(CANFDSPI_MODULE_ID index)
{
	int8_t spiTransferError = 0;
	uint8_t d = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_CiTSCON + 2, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d &= 0x06;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_CiTSCON + 2, d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TimeStampGet(CANFDSPI_MODULE_ID index, uint32_t* ts)
{
	int8_t spiTransferError = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadWord(index, cREGADDR_CiTBC, ts);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TimeStampSet(CANFDSPI_MODULE_ID index, uint32_t ts)
{
	int8_t spiTransferError = 0;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiTBC, ts);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TimeStampModeConfigure(CANFDSPI_MODULE_ID index, CAN_TS_MODE mode)
{
	int8_t spiTransferError = 0;
	uint8_t d = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_CiTSCON + 2, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d &= 0x01;
	d |= mode << 1;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_CiTSCON + 2, d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_TimeStampPrescalerSet(CANFDSPI_MODULE_ID index, uint16_t ps)
{
	int8_t spiTransferError = 0;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteHalfWord(index, cREGADDR_CiTSCON, ps);

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: Oscillator and Bit Time

int8_t DRV_CANFDSPI_OscillatorEnable(CANFDSPI_MODULE_ID index)
{
	int8_t spiTransferError = 0;
	uint8_t d = 0;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_OSC, &d);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	d &= ~0x4;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_OSC, d);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_OscillatorControlSet(CANFDSPI_MODULE_ID index, CAN_OSC_CTRL ctrl)
{
	int8_t spiTransferError = 0;

	REG_OSC osc;
	osc.word = 0;

	osc.bF.PllEnable			= ctrl.PllEnable;
	osc.bF.OscDisable			= ctrl.OscDisable;
	osc.bF.SCLKDIV				= ctrl.SclkDivide;
	osc.bF.CLKODIV				= ctrl.ClkOutDivide;
	osc.bF.LowPowerModeEnable	= ctrl.LowPowerModeEnable;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, cREGADDR_OSC, osc.byte[0]);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_OscillatorControlObjectReset(CAN_OSC_CTRL* ctrl)
{
	REG_OSC osc;
	osc.word = mcp25xxfdControlResetValues[0];

	ctrl->PllEnable		= osc.bF.PllEnable;
	ctrl->OscDisable	= osc.bF.OscDisable;
	ctrl->SclkDivide	= osc.bF.SCLKDIV;
	ctrl->ClkOutDivide	= osc.bF.CLKODIV;

	return 0;
}

int8_t DRV_CANFDSPI_OscillatorStatusGet(CANFDSPI_MODULE_ID index, CAN_OSC_STATUS* status)
{
	int8_t spiTransferError = 0;

	REG_OSC osc;
	osc.word = 0;
	CAN_OSC_STATUS stat;

	// Read
	spiTransferError = DRV_CANFDSPI_ReadByte(index, cREGADDR_OSC + 1, &osc.byte[1]);
	if (spiTransferError)
	{
		return -1;
	}

	stat.PllReady = osc.bF.PllReady;
	stat.OscReady = osc.bF.OscReady;
	stat.SclkReady = osc.bF.SclkReady;

	*status = stat;

	return spiTransferError;
}

int8_t DRV_CANFDSPI_BitTimeConfigure(CANFDSPI_MODULE_ID index, CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode, CAN_SYSCLK_SPEED clk)
{
	int8_t spiTransferError = 0;

	// Decode clk
	switch (clk)
	{
		case CAN_SYSCLK_40M:
			spiTransferError = DRV_CANFDSPI_BitTimeConfigureNominal40MHz(index, bitTime);
			if (spiTransferError) return spiTransferError;

			spiTransferError = DRV_CANFDSPI_BitTimeConfigureData40MHz(index, bitTime, sspMode);
			break;

		case CAN_SYSCLK_20M:
			spiTransferError = DRV_CANFDSPI_BitTimeConfigureNominal20MHz(index, bitTime);
			if (spiTransferError) return spiTransferError;

			spiTransferError = DRV_CANFDSPI_BitTimeConfigureData20MHz(index, bitTime, sspMode);
			break;

		case CAN_SYSCLK_10M:
			spiTransferError = DRV_CANFDSPI_BitTimeConfigureNominal10MHz(index, bitTime);
			if (spiTransferError) return spiTransferError;

			spiTransferError = DRV_CANFDSPI_BitTimeConfigureData10MHz(index, bitTime, sspMode);
			break;

		default:
			spiTransferError = -1;
			break;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_BitTimeConfigureNominal40MHz(CANFDSPI_MODULE_ID index, CAN_BITTIME_SETUP bitTime)
{
	int8_t spiTransferError = 0;
	REG_CiNBTCFG ciNbtcfg;

	ciNbtcfg.word = canControlResetValues[cREGADDR_CiNBTCFG / 4];

	// Arbitration Bit rate
	switch (bitTime)
	{
		// All 500K
		case CAN_500K_1M:
		case CAN_500K_2M:
		case CAN_500K_3M:
		case CAN_500K_4M:
		case CAN_500K_5M:
		case CAN_500K_6M7:
		case CAN_500K_8M:
		case CAN_500K_10M:
#if 1
			//MCP2517FD Bit Time Calculations - UG.xlsx
			if( g_stGITSetConfig.nBitSamplePoint > 0x84 )					// 87.5%
			{
				//sample point : 87.5%	//NPRSEG 59, NPHSEG1 10, NPHSEG2 10
				ciNbtcfg.bF.BRP = 0;
				ciNbtcfg.bF.TSEG1 = 68;
				ciNbtcfg.bF.TSEG2 = 9;
				ciNbtcfg.bF.SJW = 9;
			}
			else if( g_stGITSetConfig.nBitSamplePoint > 0x80 )				// 80%
			{
				//sample point : 80%	//NPRSEG 47, NPHSEG1 16, NPHSEG2 16
				ciNbtcfg.bF.BRP = 0;
				ciNbtcfg.bF.TSEG1 = 62;
				ciNbtcfg.bF.TSEG2 = 15;
				ciNbtcfg.bF.SJW = 15;
			}
			else if( g_stGITSetConfig.nBitSamplePoint > 0x70 )				// 75%
			{
				//sample point : 75%	//NPRSEG 39, NPHSEG1 20, NPHSEG2 20
				ciNbtcfg.bF.BRP = 0;
				ciNbtcfg.bF.TSEG1 = 58;
				ciNbtcfg.bF.TSEG2 = 19;
				ciNbtcfg.bF.SJW = 20;
			}
			//else if(  )
			//{
			//	//sample point : 85%	//NPRSEG 55, NPHSEG1 12, NPHSEG2 12
			//	ciNbtcfg.bF.BRP = 0;
			//	ciNbtcfg.bF.TSEG1 = 66;
			//	ciNbtcfg.bF.TSEG2 = 11;
			//	ciNbtcfg.bF.SJW = 11;
			//}
			else															// 80%
			{
				//sample point : 80%	//NPRSEG 47, NPHSEG1 16, NPHSEG2 16
				ciNbtcfg.bF.BRP = 0;
				ciNbtcfg.bF.TSEG1 = 62;
				ciNbtcfg.bF.TSEG2 = 15;
				ciNbtcfg.bF.SJW = 15;
			}
#else
			//sample point : 80%	//NPRSEG 47, NPHSEG1 16, NPHSEG2 16
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 62;
			ciNbtcfg.bF.TSEG2 = 15;
			ciNbtcfg.bF.SJW = 15;
#endif
			break;

		// All 250K
		case CAN_250K_500K:
		case CAN_250K_833K:
		case CAN_250K_1M:
		case CAN_250K_1M5:
		case CAN_250K_2M:
		case CAN_250K_3M:
		case CAN_250K_4M:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 126;
			ciNbtcfg.bF.TSEG2 = 31;
			ciNbtcfg.bF.SJW = 31;
			break;

		case CAN_1000K_4M:
		case CAN_1000K_8M:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 30;
			ciNbtcfg.bF.TSEG2 = 7;
			ciNbtcfg.bF.SJW = 7;
			break;

		case CAN_125K_500K:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 254;
			ciNbtcfg.bF.TSEG2 = 63;
			ciNbtcfg.bF.SJW = 63;
			break;

		default:
			return -1;
	}

	// Write Bit time registers
	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiNBTCFG, ciNbtcfg.word);

	return spiTransferError;
}

int8_t DRV_CANFDSPI_BitTimeConfigureData40MHz(CANFDSPI_MODULE_ID index, CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode)
{
	int8_t spiTransferError = 0;
	REG_CiDBTCFG ciDbtcfg;
	REG_CiTDC ciTdc;
	//    sspMode;

	ciDbtcfg.word = canControlResetValues[cREGADDR_CiDBTCFG / 4];
	ciTdc.word = 0;

	// Configure Bit time and sample point
	ciTdc.bF.TDCMode = CAN_SSP_MODE_AUTO;
	uint32_t tdcValue = 0;

	// Data Bit rate and SSP
	switch (bitTime)
	{
		case CAN_500K_1M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 30;
			ciDbtcfg.bF.TSEG2 = 7;
			ciDbtcfg.bF.SJW = 7;
			// SSP
			ciTdc.bF.TDCOffset = 31;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_2M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 14;
			ciDbtcfg.bF.TSEG2 = 3;
			ciDbtcfg.bF.SJW = 3;
			// SSP
			ciTdc.bF.TDCOffset = 15;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_3M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 8;
			ciDbtcfg.bF.TSEG2 = 2;
			ciDbtcfg.bF.SJW = 2;
			// SSP
			ciTdc.bF.TDCOffset = 9;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_4M:
		case CAN_1000K_4M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 6;
			ciDbtcfg.bF.TSEG2 = 1;
			ciDbtcfg.bF.SJW = 1;
			// SSP
			ciTdc.bF.TDCOffset = 7;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_5M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 4;
			ciDbtcfg.bF.TSEG2 = 1;
			ciDbtcfg.bF.SJW = 1;
			// SSP
			ciTdc.bF.TDCOffset = 5;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_6M7:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 3;
			ciDbtcfg.bF.TSEG2 = 0;
			ciDbtcfg.bF.SJW = 0;
			// SSP
			ciTdc.bF.TDCOffset = 4;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_8M:
		case CAN_1000K_8M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 2;
			ciDbtcfg.bF.TSEG2 = 0;
			ciDbtcfg.bF.SJW = 0;
			// SSP
			ciTdc.bF.TDCOffset = 3;
			ciTdc.bF.TDCValue = 1;
			break;

		case CAN_500K_10M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 1;
			ciDbtcfg.bF.TSEG2 = 0;
			ciDbtcfg.bF.SJW = 0;
			// SSP
			ciTdc.bF.TDCOffset = 2;
			ciTdc.bF.TDCValue = 0;
			break;

		case CAN_250K_500K:
		case CAN_125K_500K:
			ciDbtcfg.bF.BRP = 1;
			ciDbtcfg.bF.TSEG1 = 30;
			ciDbtcfg.bF.TSEG2 = 7;
			ciDbtcfg.bF.SJW = 7;
			// SSP
			ciTdc.bF.TDCOffset = 31;
			ciTdc.bF.TDCValue = tdcValue;
			ciTdc.bF.TDCMode = CAN_SSP_MODE_OFF;
			break;

		case CAN_250K_833K:
			ciDbtcfg.bF.BRP = 1;
			ciDbtcfg.bF.TSEG1 = 17;
			ciDbtcfg.bF.TSEG2 = 4;
			ciDbtcfg.bF.SJW = 4;
			// SSP
			ciTdc.bF.TDCOffset = 18;
			ciTdc.bF.TDCValue = tdcValue;
			ciTdc.bF.TDCMode = CAN_SSP_MODE_OFF;
			break;

		case CAN_250K_1M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 30;
			ciDbtcfg.bF.TSEG2 = 7;
			ciDbtcfg.bF.SJW = 7;
			// SSP
			ciTdc.bF.TDCOffset = 31;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_250K_1M5:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 18;
			ciDbtcfg.bF.TSEG2 = 5;
			ciDbtcfg.bF.SJW = 5;
			// SSP
			ciTdc.bF.TDCOffset = 19;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_250K_2M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 14;
			ciDbtcfg.bF.TSEG2 = 3;
			ciDbtcfg.bF.SJW = 3;
			// SSP
			ciTdc.bF.TDCOffset = 15;
			ciTdc.bF.TDCValue = tdcValue;
			break;
		case CAN_250K_3M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 8;
			ciDbtcfg.bF.TSEG2 = 2;
			ciDbtcfg.bF.SJW = 2;
			// SSP
			ciTdc.bF.TDCOffset = 9;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_250K_4M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 6;
			ciDbtcfg.bF.TSEG2 = 1;
			ciDbtcfg.bF.SJW = 1;
			// SSP
			ciTdc.bF.TDCOffset = 7;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		default:
			return -1;
	}

	// Write Bit time registers
	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiDBTCFG, ciDbtcfg.word);
	if (spiTransferError)
	{
		return -2;
	}

	// Write Transmitter Delay Compensation
#ifdef REV_A
	ciTdc.bF.TDCOffset = 0;
	ciTdc.bF.TDCValue = 0;
#endif

	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiTDC, ciTdc.word);
	if (spiTransferError)
	{
		return -3;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_BitTimeConfigureNominal20MHz(CANFDSPI_MODULE_ID index, CAN_BITTIME_SETUP bitTime)
{
	int8_t spiTransferError = 0;
	REG_CiNBTCFG ciNbtcfg;

	ciNbtcfg.word = canControlResetValues[cREGADDR_CiNBTCFG / 4];

	// Arbitration Bit rate
	switch (bitTime)
	{
		// All 500K
		case CAN_500K_1M:
		case CAN_500K_2M:
		case CAN_500K_4M:
		case CAN_500K_5M:
		case CAN_500K_6M7:
		case CAN_500K_8M:
		case CAN_500K_10M:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 30;
			ciNbtcfg.bF.TSEG2 = 7;
			ciNbtcfg.bF.SJW = 7;
			break;

		// All 250K
		case CAN_250K_500K:
		case CAN_250K_833K:
		case CAN_250K_1M:
		case CAN_250K_1M5:
		case CAN_250K_2M:
		case CAN_250K_3M:
		case CAN_250K_4M:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 62;
			ciNbtcfg.bF.TSEG2 = 15;
			ciNbtcfg.bF.SJW = 15;
			break;

		case CAN_1000K_4M:
		case CAN_1000K_8M:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 14;
			ciNbtcfg.bF.TSEG2 = 3;
			ciNbtcfg.bF.SJW = 3;
			break;

		case CAN_125K_500K:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 126;
			ciNbtcfg.bF.TSEG2 = 31;
			ciNbtcfg.bF.SJW = 31;
			break;

		default:
		return -1;
	}

	// Write Bit time registers
	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiNBTCFG, ciNbtcfg.word);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_BitTimeConfigureData20MHz(CANFDSPI_MODULE_ID index, CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode)
{
	int8_t spiTransferError = 0;
	REG_CiDBTCFG ciDbtcfg;
	REG_CiTDC ciTdc;
	//    sspMode;

	ciDbtcfg.word = canControlResetValues[cREGADDR_CiDBTCFG / 4];
	ciTdc.word = 0;

	// Configure Bit time and sample point
	ciTdc.bF.TDCMode = CAN_SSP_MODE_AUTO;
	uint32_t tdcValue = 0;

	// Data Bit rate and SSP
	switch (bitTime)
	{
		case CAN_500K_1M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 14;
			ciDbtcfg.bF.TSEG2 = 3;
			ciDbtcfg.bF.SJW = 3;
			// SSP
			ciTdc.bF.TDCOffset = 15;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_2M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 6;
			ciDbtcfg.bF.TSEG2 = 1;
			ciDbtcfg.bF.SJW = 1;
			// SSP
			ciTdc.bF.TDCOffset = 7;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_4M:
		case CAN_1000K_4M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 2;
			ciDbtcfg.bF.TSEG2 = 0;
			ciDbtcfg.bF.SJW = 0;
			// SSP
			ciTdc.bF.TDCOffset = 3;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_5M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 1;
			ciDbtcfg.bF.TSEG2 = 0;
			ciDbtcfg.bF.SJW = 0;
			// SSP
			ciTdc.bF.TDCOffset = 2;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_6M7:
		case CAN_500K_8M:
		case CAN_500K_10M:
		case CAN_1000K_8M:
			//qDebug("Data Bitrate not feasible with this clock!");
			return -1;

		case CAN_250K_500K:
		case CAN_125K_500K:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 30;
			ciDbtcfg.bF.TSEG2 = 7;
			ciDbtcfg.bF.SJW = 7;
			// SSP
			ciTdc.bF.TDCOffset = 31;
			ciTdc.bF.TDCValue = tdcValue;
			ciTdc.bF.TDCMode = CAN_SSP_MODE_OFF;
			break;

		case CAN_250K_833K:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 17;
			ciDbtcfg.bF.TSEG2 = 4;
			ciDbtcfg.bF.SJW = 4;
			// SSP
			ciTdc.bF.TDCOffset = 18;
			ciTdc.bF.TDCValue = tdcValue;
			ciTdc.bF.TDCMode = CAN_SSP_MODE_OFF;
			break;

		case CAN_250K_1M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 14;
			ciDbtcfg.bF.TSEG2 = 3;
			ciDbtcfg.bF.SJW = 3;
			// SSP
			ciTdc.bF.TDCOffset = 15;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_250K_1M5:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 8;
			ciDbtcfg.bF.TSEG2 = 2;
			ciDbtcfg.bF.SJW = 2;
			// SSP
			ciTdc.bF.TDCOffset = 9;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_250K_2M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 6;
			ciDbtcfg.bF.TSEG2 = 1;
			ciDbtcfg.bF.SJW = 1;
			// SSP
			ciTdc.bF.TDCOffset = 7;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_250K_3M:
		//qDebug("Data Bitrate not feasible with this clock!");
		return -1;

		case CAN_250K_4M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 2;
			ciDbtcfg.bF.TSEG2 = 0;
			ciDbtcfg.bF.SJW = 0;
			// SSP
			ciTdc.bF.TDCOffset = 3;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		default:
		return -1;
	}

	// Write Bit time registers
	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiDBTCFG, ciDbtcfg.word);
	if (spiTransferError)
	{
		return -2;
	}

	// Write Transmitter Delay Compensation
#ifdef REV_A
	ciTdc.bF.TDCOffset = 0;
	ciTdc.bF.TDCValue = 0;
#endif

	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiTDC, ciTdc.word);
	if (spiTransferError)
	{
		return -3;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_BitTimeConfigureNominal10MHz(CANFDSPI_MODULE_ID index, CAN_BITTIME_SETUP bitTime)
{
	int8_t spiTransferError = 0;
	REG_CiNBTCFG ciNbtcfg;

	ciNbtcfg.word = canControlResetValues[cREGADDR_CiNBTCFG / 4];

	// Arbitration Bit rate
	switch (bitTime)
	{
		// All 500K
		case CAN_500K_1M:
		case CAN_500K_2M:
		case CAN_500K_4M:
		case CAN_500K_5M:
		case CAN_500K_6M7:
		case CAN_500K_8M:
		case CAN_500K_10M:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 14;
			ciNbtcfg.bF.TSEG2 = 3;
			ciNbtcfg.bF.SJW = 3;
			break;

		// All 250K
		case CAN_250K_500K:
		case CAN_250K_833K:
		case CAN_250K_1M:
		case CAN_250K_1M5:
		case CAN_250K_2M:
		case CAN_250K_3M:
		case CAN_250K_4M:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 30;
			ciNbtcfg.bF.TSEG2 = 7;
			ciNbtcfg.bF.SJW = 7;
			break;

		case CAN_1000K_4M:
		case CAN_1000K_8M:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 7;
			ciNbtcfg.bF.TSEG2 = 2;
			ciNbtcfg.bF.SJW = 2;
			break;

		case CAN_125K_500K:
			ciNbtcfg.bF.BRP = 0;
			ciNbtcfg.bF.TSEG1 = 62;
			ciNbtcfg.bF.TSEG2 = 15;
			ciNbtcfg.bF.SJW = 15;
			break;

		default:
			return -1;
	}

	// Write Bit time registers
	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiNBTCFG, ciNbtcfg.word);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_BitTimeConfigureData10MHz(CANFDSPI_MODULE_ID index, CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode)
{
	int8_t spiTransferError = 0;
	REG_CiDBTCFG ciDbtcfg;
	REG_CiTDC ciTdc;
	//    sspMode;

	ciDbtcfg.word = canControlResetValues[cREGADDR_CiDBTCFG / 4];
	ciTdc.word = 0;

	// Configure Bit time and sample point
	ciTdc.bF.TDCMode = CAN_SSP_MODE_AUTO;
	uint32_t tdcValue = 0;

	// Data Bit rate and SSP
	switch (bitTime) {
		case CAN_500K_1M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 6;
			ciDbtcfg.bF.TSEG2 = 1;
			ciDbtcfg.bF.SJW = 1;
			// SSP
			ciTdc.bF.TDCOffset = 7;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_2M:
			// Data BR
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 2;
			ciDbtcfg.bF.TSEG2 = 0;
			ciDbtcfg.bF.SJW = 0;
			// SSP
			ciTdc.bF.TDCOffset = 3;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_500K_4M:
		case CAN_500K_5M:
		case CAN_500K_6M7:
		case CAN_500K_8M:
		case CAN_500K_10M:
		case CAN_1000K_4M:
		case CAN_1000K_8M:
			//qDebug("Data Bitrate not feasible with this clock!");
			return -1;

		case CAN_250K_500K:
		case CAN_125K_500K:
		ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 14;
			ciDbtcfg.bF.TSEG2 = 3;
			ciDbtcfg.bF.SJW = 3;
			// SSP
			ciTdc.bF.TDCOffset = 15;
			ciTdc.bF.TDCValue = tdcValue;
			ciTdc.bF.TDCMode = CAN_SSP_MODE_OFF;
			break;

		case CAN_250K_833K:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 7;
			ciDbtcfg.bF.TSEG2 = 2;
			ciDbtcfg.bF.SJW = 2;
			// SSP
			ciTdc.bF.TDCOffset = 8;
			ciTdc.bF.TDCValue = tdcValue;
			ciTdc.bF.TDCMode = CAN_SSP_MODE_OFF;
			break;

		case CAN_250K_1M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 6;
			ciDbtcfg.bF.TSEG2 = 1;
			ciDbtcfg.bF.SJW = 1;
			// SSP
			ciTdc.bF.TDCOffset = 7;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_250K_1M5:
		//qDebug("Data Bitrate not feasible with this clock!");
		return -1;

		case CAN_250K_2M:
			ciDbtcfg.bF.BRP = 0;
			ciDbtcfg.bF.TSEG1 = 2;
			ciDbtcfg.bF.TSEG2 = 0;
			ciDbtcfg.bF.SJW = 0;
			// SSP
			ciTdc.bF.TDCOffset = 3;
			ciTdc.bF.TDCValue = tdcValue;
			break;

		case CAN_250K_3M:
		case CAN_250K_4M:
			//qDebug("Data Bitrate not feasible with this clock!");
			return -1;

		default:
			return -1;
	}

	// Write Bit time registers
	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiDBTCFG, ciDbtcfg.word);
	if (spiTransferError)
	{
		return -2;
	}

	// Write Transmitter Delay Compensation
#ifdef REV_A
	ciTdc.bF.TDCOffset = 0;
	ciTdc.bF.TDCValue = 0;
#endif

	spiTransferError = DRV_CANFDSPI_WriteWord(index, cREGADDR_CiTDC, ciTdc.word);
	if (spiTransferError)
	{
		return -3;
	}

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: GPIO

int8_t DRV_CANFDSPI_GpioModeConfigure(CANFDSPI_MODULE_ID index, GPIO_PIN_MODE gpio0, GPIO_PIN_MODE gpio1)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_IOCON + 3;
	REG_IOCON iocon;
	iocon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &iocon.byte[3]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	iocon.bF.PinMode0 = gpio0;
	iocon.bF.PinMode1 = gpio1;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, iocon.byte[3]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_GpioDirectionConfigure(CANFDSPI_MODULE_ID index, GPIO_PIN_DIRECTION gpio0, GPIO_PIN_DIRECTION gpio1)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_IOCON;
	REG_IOCON iocon;
	iocon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &iocon.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	iocon.bF.TRIS0 = gpio0;
	iocon.bF.TRIS1 = gpio1;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, iocon.byte[0]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_GpioStandbyControlEnable(CANFDSPI_MODULE_ID index)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_IOCON;
	REG_IOCON iocon;
	iocon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &iocon.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	iocon.bF.XcrSTBYEnable = 1;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, iocon.byte[0]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_GpioStandbyControlDisable(CANFDSPI_MODULE_ID index)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_IOCON;
	REG_IOCON iocon;
	iocon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &iocon.byte[0]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	iocon.bF.XcrSTBYEnable = 0;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, iocon.byte[0]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_GpioInterruptPinsOpenDrainConfigure(CANFDSPI_MODULE_ID index, GPIO_OPEN_DRAIN_MODE mode)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_IOCON + 3;
	REG_IOCON iocon;
	iocon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &iocon.byte[3]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	iocon.bF.INTPinOpenDrain = mode;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, iocon.byte[3]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_GpioTransmitPinOpenDrainConfigure(CANFDSPI_MODULE_ID index, GPIO_OPEN_DRAIN_MODE mode)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_IOCON + 3;
	REG_IOCON iocon;
	iocon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &iocon.byte[3]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	iocon.bF.TXCANOpenDrain = mode;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, iocon.byte[3]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_GpioPinSet(CANFDSPI_MODULE_ID index, GPIO_PIN_POS pos, GPIO_PIN_STATE latch)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_IOCON + 1;
	REG_IOCON iocon;
	iocon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &iocon.byte[1]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	switch (pos)
	{
		case DRV_GPIO_PIN_0:
			iocon.bF.LAT0 = latch;
			break;

		case DRV_GPIO_PIN_1:
			iocon.bF.LAT1 = latch;
			break;

		default:
			return -1;
	}

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, iocon.byte[1]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_GpioPinRead(CANFDSPI_MODULE_ID index, GPIO_PIN_POS pos, GPIO_PIN_STATE* state)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_IOCON + 2;
	REG_IOCON iocon;
	iocon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &iocon.byte[2]);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	switch (pos)
	{
		case DRV_GPIO_PIN_0:
			*state = (GPIO_PIN_STATE) iocon.bF.GPIO0;
			break;

		case DRV_GPIO_PIN_1:
			*state = (GPIO_PIN_STATE) iocon.bF.GPIO1;
			break;

		default:
			return -1;
	}

	return spiTransferError;
}

int8_t DRV_CANFDSPI_GpioClockOutputConfigure(CANFDSPI_MODULE_ID index, GPIO_CLKO_MODE mode)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read
	a = cREGADDR_IOCON + 3;
	REG_IOCON iocon;
	iocon.word = 0;

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &iocon.byte[3]);
	if (spiTransferError)
	{
		return -1;
	}

	// Modify
	iocon.bF.SOFOutputEnable = mode;

	// Write
	spiTransferError = DRV_CANFDSPI_WriteByte(index, a, iocon.byte[3]);
	if (spiTransferError)
	{
		return -2;
	}

	return spiTransferError;
}


// *****************************************************************************
// *****************************************************************************
// Section: Miscellaneous

uint32_t DRV_CANFDSPI_DlcToDataBytes(CAN_DLC dlc)
{
	uint32_t dataBytesInObject = 0;

	if (dlc < CAN_DLC_12)
	{
		dataBytesInObject = dlc;
	}
	else
	{
		switch (dlc)
		{
			case CAN_DLC_12:			dataBytesInObject = 12;			break;
			case CAN_DLC_16:			dataBytesInObject = 16;			break;
			case CAN_DLC_20:			dataBytesInObject = 20;			break;
			case CAN_DLC_24:			dataBytesInObject = 24;			break;
			case CAN_DLC_32:			dataBytesInObject = 32;			break;
			case CAN_DLC_48:			dataBytesInObject = 48;			break;
			case CAN_DLC_64:			dataBytesInObject = 64;			break;
			default:													break;
		}
	}

	return dataBytesInObject;
}

int8_t DRV_CANFDSPI_FifoIndexGet(CANFDSPI_MODULE_ID index, CAN_FIFO_CHANNEL channel, uint8_t* mi)
{
	int8_t spiTransferError = 0;
	uint16_t a = 0;

	// Read Status register
	uint8_t b = 0;
	a = cREGADDR_CiFIFOSTA + (channel * CiFIFO_OFFSET);
	a += 1; // byte[1]

	spiTransferError = DRV_CANFDSPI_ReadByte(index, a, &b);
	if (spiTransferError)
	{
		return -1;
	}

	// Update data
	*mi = b & 0x1f;

	return spiTransferError;
}

CAN_DLC DRV_CANFDSPI_DataBytesToDlc(uint8_t n)
{
	CAN_DLC dlc = CAN_DLC_0;

	if (n <= 4)					dlc = CAN_DLC_4;
	else if (n <= 8)			dlc = CAN_DLC_8;
	else if (n <= 12)			dlc = CAN_DLC_12;
	else if (n <= 16)			dlc = CAN_DLC_16;
	else if (n <= 20)			dlc = CAN_DLC_20;
	else if (n <= 24)			dlc = CAN_DLC_24;
	else if (n <= 32)			dlc = CAN_DLC_32;
	else if (n <= 48)			dlc = CAN_DLC_48;
	else if (n <= 64)			dlc = CAN_DLC_64;

	return dlc;
}