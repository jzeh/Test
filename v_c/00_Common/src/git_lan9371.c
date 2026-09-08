/*************************************************************
 * NOTE : git_lan9371.c
 *      Lan9371 Ethnet Switch control
 * Author : Lee junho
 * Since : 2021.02.01
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include "common.h"
#include "gpio.h"
#include "spi.h"
#include "git_ioctl.h"

#include "git_lan9371.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
// Lan9371 Common
#define LAN9371_SPI_MAX_DATA_SIZE						1024
#define LAN9371_COMMAND_SIZE							4

#define	KSZ9477_SPI_CMD_WRITE							0x40000000
#define	KSZ9477_SPI_CMD_READ							0x60000000

// Lan9371 Register
#define	REG_GLOBAL_OPERATIONAL_CONTROL					0x0000
#define	REG_GLOBAL_IO_CONTROL							0x0100
#define	REG_GLOBAL_PHY_CONTROL							0x0200
#define	REG_GLOBAL_SWICH_CONTROL						0x0300
#define	REG_GLOBAL_SWICH_LUE_CONTROL					0x0400
#define	REG_GLOBAL_SWICH_PTP_CONTROL					0x0500
#define	REG_GLOBAL_SWICH_HSR_CONTROL					0x0600
#define	REG_GLOBAL_VIRTUAL_PHY_CONTROL					0x0700
#define	REG_GLOBAL_PACKET_GENERATOR_CONTROL				0x0800
#define	REG_GLOBAL_TX_PHY_CONTROL						0x0900

#define	REG_PORT_OPERATIONAL_CONTROL					0x0000
#define	REG_PORT_T1_PHY									0x0100
#define	REG_PORT_SGMII_CONTROL							0x0200
#define	REG_PORT_TX_PHY									0x0280
#define	REG_PORT_RGMII_CONTROL							0x0300
#define	REG_PORT_SWITCH_MAC_CONTROL						0x0400
#define	REG_PORT_SWITCH_MIB_COUNTER						0x0500
#define	REG_PORT_SWITCH_ACL_CONTROL						0x0600
#define	REG_PORT_RESERVED								0x0700
#define	REG_PORT_SWITCH_INGRESS_CONTROL					0x0800
#define	REG_PORT_SWITCH_EGRESS_CONTROL					0x0900
#define	REG_PORT_QUEUE_MANAGE_CONTROL					0x0A00
#define	REG_PORT_SWITCH_LUE_CONTROL						0x0B00
#define	REG_PORT_SWITCH_PTP_CONTROL						0x0C00

// Debug
#define	DEBUG_SPI_RX_ENABLE								0
#define	DEBUG_SPI_TX_ENABLE								0

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
int32_t		InitLan9371( void );
int32_t		StartLan9371Thread( void );

void CS_Assert( bool assert );

int8_t LAN9371_RegRead( uint32_t port, uint32_t function, uint32_t addr, uint32_t size );

void LAN9371_ReadChipID( void );
int8_t LAN9371_SoftReset( void );

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
// global variables
uint8_t gSpiTxData[LAN9371_SPI_MAX_DATA_SIZE];
uint8_t gSpiRxData[LAN9371_SPI_MAX_DATA_SIZE];

/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/
int32_t InitLan9371( void )
{
	EnableLAN9371();

	LAN9371_ReadChipID();

//	LAN9371_SoftReset();

	return INIT_OK;
}

/*----------------------------------------------------------------------
 *   SPI API
 *--------------------------------------------------------------------*/
void CS_Assert( bool assert )
{
	if( assert )		HAL_GPIO_WritePin( SPI4_NSS_GPIO_Port, SPI4_NSS_Pin, GPIO_PIN_RESET );
	else				HAL_GPIO_WritePin( SPI4_NSS_GPIO_Port, SPI4_NSS_Pin, GPIO_PIN_SET );
}

int8_t LAN9371_SPI_TransferData( uint8_t *txData, uint8_t *rxData, uint32_t transferSize )
{
	HAL_StatusTypeDef	st;
	int8_t				error = 0;

	CS_Assert( true );

	HAL_Delay( 1 );

	st = HAL_SPI_TransmitReceive( LAN9371_SPI_PORT, (uint8_t *)txData, (uint8_t *)rxData, transferSize, 5000 );
	if( st != HAL_OK)
	{
		GLogE( "Error...(%d)\r\n", st );
		error = -1;
	}

	while( HAL_SPI_GetState( LAN9371_SPI_PORT ) != HAL_SPI_STATE_READY ){}

	CS_Assert( false );

	return error;
}

/*----------------------------------------------------------------------
 *   LAN9371 API
 *--------------------------------------------------------------------*/
int8_t LAN9371_SPI_ReadByte( uint16_t addr, uint8_t *rxd )
{
	uint32_t	command;
	uint16_t	transferSize	= LAN9371_COMMAND_SIZE + 1;
	int8_t		error = 0;

	command =  KSZ9477_SPI_CMD_READ;
	command |= addr << 5;

#if DEBUG_SPI_RX_ENABLE
	GLogN( "Command  : %s\r\n", ( ( ( command >> 29 ) & 0x0f )== 3 ) ? "READ" : "WRITE" );
	GLogN( "Port     : 0x%02x\r\n", ( ( command >> 17 ) & 0x0F ) );
	GLogN( "Function : 0x%02x\r\n", ( ( command >> 13 ) & 0x0F ) );
	GLogN( "Register : 0x%02x\r\n", ( ( command >>  5 ) & 0xFF ) );
#endif	// DEBUG_SPI_RX_ENABLE

	gSpiTxData[0] = (command >> 24) & 0xFF;
	gSpiTxData[1] = (command >> 16) & 0xFF;
	gSpiTxData[2] = (command >> 8) & 0xFF;
	gSpiTxData[3] = (command & 0xFF);

	error = LAN9371_SPI_TransferData( gSpiTxData, gSpiRxData, transferSize );

	*rxd = gSpiRxData[ transferSize - 1 ];

	return error;
}

int8_t LAN9371_SPI_ReadByteArray( uint16_t addr, uint8_t *rxd, uint32_t nBytes )
{
	uint32_t	command;
	uint16_t	transferSize	= LAN9371_COMMAND_SIZE + nBytes;
	int8_t		error = 0;
	uint32_t	i;

	command =  KSZ9477_SPI_CMD_READ;
	command |= addr << 5;

#if DEBUG_SPI_RX_ENABLE
	GLogN( "Command  : %s\r\n", ( ( ( command >> 29 ) & 0x0f )== 3 ) ? "READ" : "WRITE" );
	GLogN( "Port     : 0x%02x\r\n", ( ( command >> 17 ) & 0x0F ) );
	GLogN( "Function : 0x%02x\r\n", ( ( command >> 13 ) & 0x0F ) );
	GLogN( "Register : 0x%02x\r\n", ( ( command >>  5 ) & 0xFF ) );
#endif	// DEBUG_SPI_RX_ENABLE

	gSpiTxData[0] = (command >> 24) & 0xFF;
	gSpiTxData[1] = (command >> 16) & 0xFF;
	gSpiTxData[2] = (command >> 8) & 0xFF;
	gSpiTxData[3] = (command & 0xFF);

	// Clear Rxdata
	for( i = LAN9371_COMMAND_SIZE; i < transferSize; i++ )
	{
		gSpiRxData[ i ]	= 0;
	}

	error = LAN9371_SPI_TransferData( gSpiTxData, gSpiRxData, transferSize );

	// Updata RxData
	for( i = 0; i < nBytes; i++ )
	{
		rxd[i] = gSpiRxData[ i + LAN9371_COMMAND_SIZE ];
	}

	return error;
}

int8_t LAN9371_SPI_WriteByte( uint16_t addr, uint8_t *txd )
{
	uint32_t	command;
	uint16_t	transferSize	= LAN9371_COMMAND_SIZE + 1;
	int8_t		error = 0;

	command =  KSZ9477_SPI_CMD_WRITE;
	command |= addr << 5;

#if DEBUG_SPI_RX_ENABLE
	GLogN( "Command  : %s\r\n", ( ( ( command >> 29 ) & 0x0f )== 3 ) ? "READ" : "WRITE" );
	GLogN( "Port     : 0x%02x\r\n", ( ( command >> 17 ) & 0x0F ) );
	GLogN( "Function : 0x%02x\r\n", ( ( command >> 13 ) & 0x0F ) );
	GLogN( "Register : 0x%02x\r\n", ( ( command >>  5 ) & 0xFF ) );
#endif	// DEBUG_SPI_RX_ENABLE

	gSpiTxData[0] = (command >> 24) & 0xFF;
	gSpiTxData[1] = (command >> 16) & 0xFF;
	gSpiTxData[2] = (command >> 8) & 0xFF;
	gSpiTxData[3] = (command & 0xFF);
	gSpiTxData[4] = *txd;

	error = LAN9371_SPI_TransferData( gSpiTxData, gSpiRxData, transferSize );

	return error;
}

/*----------------------------------------------------------------------
 *   Global Register
 *--------------------------------------------------------------------*/
void LAN9371_ReadChipID( void )
{
	uint8_t	id[4];

#if 0
	uint8_t	i;

	for( i = 0; i < 4; i++ )
	{
		LAN9371_SPI_ReadByte( REG_GLOBAL_OPERATIONAL_CONTROL + i, &id[i] );
		GLogN( "0x%02x\r\n", id[i] );
	}
#else
	LAN9371_SPI_ReadByteArray( REG_GLOBAL_OPERATIONAL_CONTROL, id, 4 );
	GLogN( "Found LAN%02x%02x, Revision : %d\r\n", id[1], id[2], id[3] >> 4 );
#endif

	LAN9371_SPI_ReadByte( REG_GLOBAL_OPERATIONAL_CONTROL + 0x0f, &id[0] );
	GLogN( "Bond_PAD %02x\r\n", id[0] );
}

int8_t LAN9371_SoftReset( void )
{
/*
	uint8_t	data = 0;

	LAN9371_SPI_ReadByte( REG_GLOBAL_SWICH_CONTROL, &data );
	GLogN( "GLB_SW_CONTROL0 : 0x%02x\r\n", data );

	data = 0x02;

	LAN9371_SPI_WriteByte( REG_GLOBAL_SWICH_CONTROL, &data );

	LAN9371_SPI_ReadByte( REG_GLOBAL_SWICH_CONTROL, &data );
	GLogN( "GLB_SW_CONTROL0 : 0x%02x\r\n", data );
*/
/*
	uint8_t data[4];
	LAN9371_SPI_ReadByteArray( REG_GLOBAL_PHY_CONTROL + 0x10, data, 4 );
	GLogN( "0x%02x 0x%02x 0x%02x 0x%02x \r\n", data[0], data[1], data[2], data[3] );
*/

#if 0 // PHY Configration
	LAN9371_SPI_ReadByte( REG_GLOBAL_PHY_CONTROL + 0x10, &data );
	GLogN( "[KKS TEST] 0x0200 + 0x10 : 0x%02x\r\n", data );

	LAN9371_SPI_ReadByte( REG_GLOBAL_PHY_CONTROL + 0x11, &data );
	GLogN( "[KKS TEST] 0x0200 + 0x11 : 0x%02x\r\n", data );

	LAN9371_SPI_ReadByte( REG_GLOBAL_PHY_CONTROL + 0x12, &data );
	GLogN( "[KKS TEST] 0x0200 + 0x12 : 0x%02x\r\n", data );

	LAN9371_SPI_ReadByte( REG_GLOBAL_PHY_CONTROL + 0x13, &data );
	GLogN( "[KKS TEST] 0x0200 + 0x13 : 0x%02x\r\n", data );


	LAN9371_SPI_ReadByte( REG_GLOBAL_PHY_CONTROL + 0x14, &data );
	GLogN( "[KKS TEST] 0x0200 + 0x14 : 0x%02x\r\n", data );

	LAN9371_SPI_ReadByte( REG_GLOBAL_PHY_CONTROL + 0x15, &data );
	GLogN( "[KKS TEST] 0x0200 + 0x15 : 0x%02x\r\n", data );

	LAN9371_SPI_ReadByte( REG_GLOBAL_PHY_CONTROL + 0x16, &data );
	GLogN( "[KKS TEST] 0x0200 + 0x16 : 0x%02x\r\n", data );

	LAN9371_SPI_ReadByte( REG_GLOBAL_PHY_CONTROL + 0x17, &data );
	GLogN( "[KKS TEST] 0x0200 + 0x16 : 0x%02x\r\n", data );

	data = 0x01;
	LAN9371_SPI_WriteByte( REG_GLOBAL_IO_CONTROL + 0x03, &data );
	LAN9371_SPI_ReadByte( REG_GLOBAL_IO_CONTROL + 0x03, &data );
	GLogN( "[KKS MOD] Step0 . REG_GLOBAL_IO_CONTROL : 0x%02x\r\n", data );
#endif

	return 0;
}

/*----------------------------------------------------------------------
 *   Port Register
 *--------------------------------------------------------------------*/
int8_t LAN9371_CheckLinkStatus( uint8_t port )
{
	uint8_t		link;
	uint16_t	addr;

	addr = port;
	addr = addr << 12;
	addr = ( addr | REG_PORT_RGMII_CONTROL ) + 0x03;

	LAN9371_SPI_ReadByte( addr, &link );

	return link;
}

int8_t LAN9371_CheckPortOperationStatus( uint8_t port )
{
	uint8_t		link;
	uint16_t	addr;

	addr = port;
	addr = addr << 12;
	addr = ( addr | REG_PORT_OPERATIONAL_CONTROL ) + 0x30;

	LAN9371_SPI_ReadByte( addr, &link );

	return link;
}