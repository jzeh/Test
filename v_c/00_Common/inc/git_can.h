/*----------------------------------------------------------------------
 *   FDCAN Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_CAN_H_
#define	__GIT_CAN_H_

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "fdcan.h"
#include "git_protocol.h"

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define	FDCAN_POOL_SIZE								200
#define FDCAN_MESSAGE_QUEUE_SIZE					200
#define FDCAN_PACKET_MAX_SIZE						64

#define ON_FDCAN_FDFORMAT(FDFORMAT) ((FDFORMAT == FDCAN_FD_CAN) ? 1 : 0)
#define ON_FDCAN_BRS(BRS) 			((BRS == FDCAN_BRS_ON)	   ? 1 : 0)
#define ON_FDCAN_ESI(ESI) 			((ESI == FDCAN_ESI_PASSIVE) ? 1 : 0)

// DEBUG
//#define GITCAN_DEBUG_FUNC							0

typedef enum
{
	CAN_FRAMEFORMAT_FDCAN		= 0,
	CAN_FRAMEFORMAT_CLASSIC		= 1
} eCanFrameFormat;

typedef enum
{
	CAN_IDTYPE_STANDARD			= 0,
	CAN_IDTYPE_EXTENDED			= 1
} eCanIDType;

typedef enum
{
	CAN_1MBPS		= 0,
	CAN_500KBPS		= 1,
	CAN_250KBPS		= 2,
	CAN_125KBPS		= 3,
	CAN_100KBPS		= 4,
	CAN_DAT_1MBPS	= 5,
	CAN_DAT_2MBPS	= 6,
	CAN_DAT_4MBPS	= 7
 } eCanBitTime;

/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/
typedef struct _stFdcanPkt
{
	FDCAN_RxHeaderTypeDef	mRxHeader;
	FDCAN_TxHeaderTypeDef	mTxHeader;

	FDCAN_HandleTypeDef		*pSource;
	FDCAN_HandleTypeDef		*pTarget;

	uint16_t	mTimeStamp;
	uint32_t	mLen;
	uint8_t		mData[FDCAN_PACKET_MAX_SIZE];
} stFdcanPkt;

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern int32_t 	initFDCan( void );																				// FDCAN init
extern void		deinitFDCan( void );																			// FDCAN deinit
extern uint8_t	startFDCanThread( void );																		// Start Thread

extern uint8_t	startFDCan( uint8_t high1, uint8_t high2, uint8_t low );
extern uint8_t	stopFDCan( FDCAN_HandleTypeDef* hfdcan );														// FDCAN HAL stop

extern uint8_t	setFilter1( uint32_t config, uint32_t id1, uint32_t id2 );										// FDCAN set range filter
extern uint8_t	setFilter2( uint32_t config, uint32_t id1, uint32_t id2 );										// FDCAN set range filter
extern uint8_t	clearFliter( FDCAN_HandleTypeDef *fdcan );														// FDCAN clear filter

extern uint8_t	CanSet_Baud( FDCAN_HandleTypeDef* hfdcan, uint8_t nominalBaud, uint8_t dataBaud, uint8_t format );				// FDCAN Baudrate Set
extern void		makeTxHeaderCAN( FDCAN_TxHeaderTypeDef *header, uint32_t iden, uint8_t type, uint8_t len, uint8_t format );		// 11byte identifier

extern void		clearRXCanMessage( void );																		// Clear ReceiveMessageQueue
extern void		clearTXCanMessage( void );																		// Clear TransmitMessageQueue

extern void			CAN_Initial_CH( U8 CAN_CHANNEL, U8 HighCan, U8 Can_BPS, U8 format, U8 rxfifo, u8 CANIDType, u8 MaskNum, u32 *StartMaskValue, u32 *EndMaskValue);
extern u8			CAN_Channel_Masket_Set(FDCAN_HandleTypeDef *hfdcan, u8 nRxFifo, u8 nCANIDType, u8 nMaskNum, u32 *pStartMaskValue, u32 *pEndMaskValue);
extern unsigned char Oem_CAN_Channel_Masket_Set(unsigned char nChannel, unsigned char nCANIDType, unsigned char nMaskNum, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue);
U8 Check_PGN(stFdcanPkt *packet,U32 PGN);		//Can0ReadBuff_29bit
extern void CanTx(uint32_t CanID, unsigned char* pData, unsigned char length, unsigned char* pLog, unsigned char LogLoc);
extern void CanRx(uint32_t* CanID, unsigned char* RxData, unsigned char* length, unsigned char* pLog, unsigned char LogLoc);
extern void CanTx_Multi(uint32_t CanID, unsigned char* pData, uint16_t length, unsigned char* pLog, unsigned char LogLoc);
/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern osMessageQId	hFDRxMsg;
extern osMessageQId	hFDTxMsg;

extern osPoolId		hFdcanPktPool;
extern osPoolId		hFdcanMsgPool;

#endif // __GIT_CAN_H_
