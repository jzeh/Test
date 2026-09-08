/*----------------------------------------------------------------------
 *   FDCAN Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_MCP2518FD_H__
#define	__GIT_MCP2518FD_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "spi.h"
#include "drv_spi.h"
#include "drv_canfdspi_defines.h"
#include "drv_canfdspi_api.h"

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define APP_TX_FIFO									CAN_FIFO_CH1			// Transmit Channels
#define APP_RX_FIFO									CAN_FIFO_CH2			// Receive Channels

#define SPI_CAN_PACKET_MAX_SIZE						64

#define SPI_HW_FILTER_MAX_SIZE						32
#define SPI_SW_FILTER_MAX_SIZE						200

#define MCP2518_IDE_STANDARD 						(0) /*!< Standard ID element */
#define MCP2518_IDE_EXTENDED 						(1) /*!< Extended ID element */
// ESI: Error Status Indicator bit
#define MCP2518_ESI_PASSIVE							(1) /* Transmitting node is error passive */
#define MCP2518_ESI_ACTIVE							(0) /* Transmitting node is error active */

// DEBUG
//#define MCP2518_DEBUG_FUNC							0
/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/
typedef struct
{
	CAN_TX_MSGOBJ			mTxObj;
	CAN_RX_MSGOBJ			mRxObj;

//	SPI_HandleTypeDef		*pSource;
//	SPI_HandleTypeDef		*pTarget;

	SPI_TypeDef				*pSource;
	SPI_TypeDef				*pTarget;

//	U16		mTimeStamp;
	U8		mLen;
	U8		mData[SPI_CAN_PACKET_MAX_SIZE];
} SpiCanPkt_t;

/*
typedef struct
{

} SPICAN_HandleTypeDef;
*/
/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern int32_t	InitSPICanControler( void );
extern int32_t	DeinitMCP2518FD( void );
extern int32_t	InitMCP2518FD( void );
extern void		SetCanBaudrate( void );
extern uint8_t  mcp2518fd_set_baud( uint8_t nominalBaud, uint8_t dataBaud, uint8_t format );
extern void		SetCanMasking(void);
void SetCanMaskingMCP2518(uint32_t startMask, uint32_t endMask);
void SetCanMaskingMCP2518_EXT(uint32_t startMask, uint32_t endMask);
extern void		ClearCanMasking( uint8_t index );
extern int32_t	StartSpiThread( void );
extern int32_t	StopSpiThread( void );
extern void		SetSpiCanSTB( void );

extern void		StartSpiCan(void);
extern void		StopSpiCan(void);

extern void		EnableReceive(uint8_t index);
extern void		DisableReceive( uint8_t index );

/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern osMessageQId	hTxSpiCanMsg1;
extern osMessageQId	hTxSpiCanMsg2;
extern osMessageQId	hTxSpiCanMsg3;
extern osMessageQId	hTxSpiCanMsg4;

extern osMessageQId	hRxSpiCanMsg;

#endif // __GIT_MCP2518FD_H__