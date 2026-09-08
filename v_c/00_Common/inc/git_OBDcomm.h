#ifndef	__GIT_OBDCOMM_H_
#define	__GIT_OBDCOMM_H_

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/


/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define CAN_FRAME_SOF			0x01
#define CAN_FRAME_EOF			0x7F
#define CAN_FRAME_STANDARD_IDE	0x00 // standard CAN:0, extend CAN : 1
#define CAN_FRAME_EXTEND_IDE	0x01 // standard CAN:0, extend CAN : 1
#define CAN_FRAME_DATA_SIZE 	0X08

#define CAN_SINGLE_FRAME		0X00
#define CAN_FIRST_FRAME			0x10
#define CAN_CONSECUTICE_FRAME	0x20
#define CAN_FLOWCTRL_FRAME 		0x30
#define CAN_FS_CTS				0x00
#define CAN_FS_WAIT				0x01
#define CAN_FS_OVERFLOW			0x02
   
#define SIGNAL_VCI_ACK_ING				(int)(0x01)
#define SIGNAL_AVAILABLE_SEND_KL_MSG	(int)(0x02)

#define MAX_CARB_RECV_ARRAY_CNT		500

#define CAN_FIRST_FRAME_DATA_SIZE   6
#define CAN_CONSECUTICE_FRAME_DATA_SIZE 7
#define CAN_STANDARD_ID_SIZE   2
#define CAN_EXTENDED_ID_SIZE   4
#define CAN_DLC_BYTE_SIZE 1

#define NEW_UDS_FILLER

/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/
typedef enum _eCanType
{
	eCAN_CLASSIC_EXTENDED	= 0x00,
	eCAN_CLASSIC_STANDARD   = 0x01,
	eCAN_FDFORMAT_EXTENDED  = 0x10,
	eCAN_FDFORMAT_STANDARD	= 0x11,
}eCanType;

typedef enum _eDiagCanState
{
	eCAN_NONE_STATE,
	eCAN_TX_NONE_PARSING,	
	eCAN_TX_SINGLE_FRAME_DTC,
	eCAN_TX_SINGLE_FRAME,
	eCAN_TX_FIRST_FRAME,
	eCAN_TX_CONSECUTIVE_FRAME,          //5
	eCAN_TX_FLOWCONTROL_FRAME,
	eCAN_TX_SINGLE_FRAME_DTC_EX,
	eCAN_TX_SINGLE_FRAME_EX,
	eCAN_TX_FIRST_FRAME_EX,
	eCAN_TX_CONSECUTIVE_FRAME_EX,       //10
	eCAN_TX_FLOWCONTROL_FRAME_EX,
	eCAN_TX_SINGLE_FRAME_DTC_29BIT,
	eCAN_TX_SINGLE_FRAME_29BIT,
	eCAN_TX_FIRST_FRAME_29BIT,
	eCAN_TX_CONSECUTIVE_FRAME_29BIT,    //15
	eCAN_TX_FLOWCONTROL_FRAME_29BIT,
	eCAN_RX_SINGLE_FRAME,				//17
	eCAN_RX_FIRST_FRAME,				//18
	eCAN_RX_CONSECUTIVE_FRAME,			//19
	eCAN_RX_FLOWCONTROL_FRAME,          //20
	eCAN_RX_BLOCK,
	eCAN_RX_SINGLE_FRAME_EX,
	eCAN_RX_FIRST_FRAME_EX,
	eCAN_RX_CONSECUTIVE_FRAME_EX,
	eCAN_RX_FLOWCONTROL_FRAME_EX,       //25
	eCAN_RX_BLOCK_EX,
	eCAN_RX_SINGLE_FRAME_29BIT,
	eCAN_RX_FIRST_FRAME_29BIT,
	eCAN_RX_CONSECUTIVE_FRAME_29BIT,
	eCAN_RX_FLOWCONTROL_FRAME_29BIT,    //30
	eCAN_RX_BLOCK_29BIT,
	eCAN_RX_FRAME_GM_DTC,
	eCAN_RX_SLEEP_CHECK,
	eCAN_RX_COMPLETE,
	eCAN_RX_FAIL,                       //35
	eCAN_RX_PENDING,
	eCAN_MAX_STATE
}eDiagCanState;
typedef enum _eDiagKLINEState
{
	eKLINE_NONE_STATE,
	eKLINE_TX_BLOCK_NON, // Siemens Simplex Comm.
	eKLINE_TX_BLOCK_NAG,
	eKLINE_RX_BLOCK_NAG,
	eKLINE_TX_BLOCK_KWP,
	eKLINE_RX_BLOCK_KWP,
	eKLINE_TX_BLOCK,
	eKLINE_TX_BLOCK_LUCAS,
	eKLINE_RX_BLOCK,
	eKLINE_RX_BLOCK_LUCAS,
	eKLINE_TX_BLOCK_BOSCH,
	eKLINE_TX_BLOCK_MCU,
	eKLINE_TX_BLOCK_MCU_REPRO,
	eKLINE_RX_BLOCK_BOSCH,
	eKLINE_RX_BLOCK_SIEMENS,
	eKLINE_RX_BLOCK_MCU,
	eKLINE_RX_BLOCK_MCU_REPRO,
	eKLINE_RX_BLOCK_DW_SIEMENSE,
	eKLINE_RX_MIL_STATUS,
	eKLINE_RX_COMPLETE,
	eKLINE_RX_FAIL,
	eKLINE_MAX_STATE
}eDiagKLINEState;
typedef struct _CARB_REVC_INFO
{
	unsigned int 	uiOrder;
	unsigned int 	uiCANRecvedLength;
	unsigned int 	uiCANTotalPacketLength;
	unsigned long 	ulCANID;
	unsigned int 	uiCANIDLength;
	unsigned int 	uiCANDataSavePos;
}CARB_REVC_INFO;

#pragma pack(push, 1) 

#define SIZE_CAN_DATA_FIELD		8
#define SIZE_FDCAN_DATA_FIELD	64

typedef struct _stStandardCanType
{
	unsigned short	ucSOF:1;			// Denotes the start of frame transmission
	unsigned short	us11BitID:11;		// A (unique) identifier for the data which also represents the message priority
	unsigned short	ucRTR:1;			// Dominant (0) (see Remote Frame below)
	unsigned short	ucIDE:1;			// Declaring if 11 bit message ID or 29 bit message ID is used. Dominate (0) indicate 11 bit message ID while Recessive (1) indicate 29 bit message.
	unsigned short	ucReserved:1;		// Reserved bit (it must be set to dominant (0), but accepted as either dominant or recessive)
	unsigned short	ucDummy1:1;			// network 전달을 위해서 dummy추가
	unsigned short	ucDLC:4;			// Number of bytes of data (0-8 bytes)	
	unsigned short	ucDummy2:12;		// network 전달을 위해서 dummy추가
	unsigned char 	arrDataFields[SIZE_CAN_DATA_FIELD];	// ata to be transmitted (length in bytes dictated by DLC field)
	unsigned short	usCRC:15;			// Cyclic redundancy check
	unsigned short	ucCRCDelimiter:1; 	// Must be recessive (1)
	unsigned short	ucACK:1;			// Transmitter sends recessive (1) and any receiver can assert a dominant (0)
	unsigned short	ucACKDelimiter:1;	// Must be recessive (1)
	unsigned short	ucEOF:7;			// Must be recessive (1)
}stStandardCanType;

typedef struct _stStdFDCanType
{
	unsigned short	ucSOF:1;			// Denotes the start of frame transmission
	unsigned short	us11BitID:11;		// A (unique) identifier for the data which also represents the message priority
	unsigned short	ucReserved1:1;		// Reserved bit (it must be set to dominant (0), but accepted as either dominant or recessive)
	unsigned short	ucIDE:1;			// Declaring if 11 bit message ID or 29 bit message ID is used. Dominate (0) indicate 11 bit message ID while Recessive (1) indicate 29 bit message.
	unsigned short	ucEDL:1;			// Extended Data Length. It only exists in CANFD format. Dominate (0) indicate CAN format. Recessive (1) indicate CANFD format.
	unsigned short	ucReserved0:1;		// Reserved bit (it must be set to dominant (0), but accepted as either dominant or recessive)
	unsigned short	ucBRS:1;			// Bit Rate Switch. Recessive (1) indicate DataFields BPS is different from arbitration fields BPS.
	unsigned short	ucESI:1;			// Error State Indicator. Dominate (0) indicate Error active nodes. Recessive (1) indicate Error passiv nodes.
	unsigned short	ucDummy1:14;		// network 전달을 위해서 dummy추가
	unsigned short	ucDLC:8;			// Number of bytes of data (0-8 bytes)	
	unsigned short	ucDummy2:12;		// network 전달을 위해서 dummy추가
	unsigned char 	arrDataFields[SIZE_FDCAN_DATA_FIELD];	// ata to be transmitted (length in bytes dictated by DLC field)
	unsigned int	usCRC:21;			// Cyclic redundancy check. CRC17 is used for frames in CANFD with a Data Field up tp sixteen byte. CRC21 is used for frames in CANFD with a Data Field longer than sixteen byte.
	unsigned short	ucCRCDelimiter:1; 	// Must be recessive (1)
	unsigned short	ucACK:1;			// Transmitter sends recessive (1) and any receiver can assert a dominant (0)
	unsigned short	ucACKDelimiter:1;	// Must be recessive (1)
	unsigned short	ucEOF:7;			// Must be recessive (1)
}stStdFDCanType;

typedef struct _stExtendCanType
{
	unsigned short	ucSOF:1;			// Denotes the start of frame transmission
	unsigned short	us11BitID:11;		// First part of the (unique) identifier for the data which also represents the message priority
	unsigned short	ucSRR:1;			// Must be recessive (1). Optional
	unsigned short	ucIDE:1;			// Must be recessive (1). Optional
	unsigned short	ucDummy0:2;			// network 전달을 위해서 dummy추가
	unsigned int	us18BitID:18;		// Second part of the (unique) identifier for the data which also represents the message priority
	unsigned short	ucRTR:4;			// Must be dominant (0)
	unsigned short	ucReserved:1;		// Reserved bit (it must be set to dominant (0), but accepted as either dominant or recessive)
	unsigned short	ucDLC:4;			// Number of bytes of data (0-8 bytes)
	unsigned short	ucDummy1:5;			// network 전달을 위해서 dummy추가
	unsigned char 	arrDataFields[SIZE_CAN_DATA_FIELD];	// ata to be transmitted (length in bytes dictated by DLC field)
	unsigned short	usCRC:15;			// Cyclic redundancy check
	unsigned short	ucCRCDelimiter:1; 	// Must be recessive (1)
	unsigned short	ucACK:1;			// Transmitter sends recessive (1) and any receiver can assert a dominant (0)
	unsigned short	ucACKDelimiter:1;	// Must be recessive (1)
	unsigned short	ucEOF:7;			// Must be recessive (1)
}stExtendCanType;

typedef struct _stExtFDCanType
{
	unsigned short	ucSOF:1;			// Denotes the start of frame transmission
	unsigned short	us11BitID:11;		// First part of the (unique) identifier for the data which also represents the message priority
	unsigned short	ucSRR:1;			// Must be recessive (1). Optional
	unsigned short	ucIDE:1;			// Declaring if 11 bit message ID or 29 bit message ID is used. Dominate (0) indicate 11 bit message ID while Recessive (1) indicate 29 bit message.
	unsigned short	ucDummy0:2;			// network 전달을 위해서 dummy추가
	unsigned int	us18BitID:18;		// Second part of the (unique) identifier for the data which also represents the message priority
	unsigned short	ucReserved1:1;		// Reserved bit (it must be set to dominant (0), but accepted as either dominant or recessive)
	unsigned short	ucEDL:1;			// Extended Data Length. It only exists in CANFD format. Dominate (0) indicate CAN format. Recessive (1) indicate CANFD format.
	unsigned short	ucReserved0:1;		// Reserved bit (it must be set to dominant (0), but accepted as either dominant or recessive)
	unsigned short	ucBRS:1;			// Bit Rate Switch. Recessive (1) indicate DataFields BPS is different from arbitration fields BPS.
	unsigned short	ucESI:1;			// Error State Indicator. Dominate (0) indicate Error active nodes. Recessive (1) indicate Error passiv nodes.
	unsigned short	ucDLC:4;			// Number of bytes of data (0-8 bytes)
	unsigned short	ucDummy1:5;			// network 전달을 위해서 dummy추가
	unsigned char 	arrDataFields[SIZE_FDCAN_DATA_FIELD];	// ata to be transmitted (length in bytes dictated by DLC field)
	unsigned int	usCRC:21;			// Cyclic redundancy check. CRC17 is used for frames in CANFD with a Data Field up tp sixteen byte. CRC21 is used for frames in CANFD with a Data Field longer than sixteen byte.
	unsigned short	ucCRCDelimiter:1; 	// Must be recessive (1)
	unsigned short	ucACK:1;			// Transmitter sends recessive (1) and any receiver can assert a dominant (0)
	unsigned short	ucACKDelimiter:1;	// Must be recessive (1)
	unsigned short	ucEOF:7;			// Must be recessive (1)
}stExtFDCanType;

typedef union _stCanPacket
{
	stStandardCanType	 stNormalPacket;
	stExtendCanType		 stExtendPacket;
	stStdFDCanType		 stFDStdPacket;
	stExtFDCanType		 stFDExtPacket;
}stCanPacket;

#define MAX_PERIODICMSG_SIZE 20
typedef struct _stPERIODIC_MSG_INFO
{
	U8  ucIndex;
	U8 		ucMode;
	//uint16_t 	uiOldTime;
	uint32_t 	uiInterval;
	uint8_t 	ucMsg[MAX_PERIODICMSG_SIZE];
	u8			ucTimerIndex;
} stPERIODIC_MSG_INFO;

typedef struct _stACK_MSG_INFO
{
	bool 		bMode;
	uint32_t 	uiInterval;
	uint8_t 	ucMsg[20];
	u8			ucTimerIndex;
} stACK_MSG_INFO;

#pragma pack(pop,1) 

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern char	initOBDComm( void );
extern void	deinitOBDComm( void );
extern U32	CAN_SavePassThruWriteMsg(void);
extern U32	KLINE_SavePassThruWriteMsg(void);
extern void	CAN_InitVariable(void);
extern void	VCI_PeriodicMessage0(void);
extern void	VCI_PeriodicMessage1(void);
extern void	VCI_PeriodicMessage2(void);
extern void	VCI_PeriodicMessage3(void);
extern void	VCI_PeriodicMessage4(void);
extern void	VCI_ACK(void);
extern void	CAN_uDelay(unsigned int uiDelay);
void ClearRxCanState();
void ClearTxCanState();
extern bool CheckNewUDS( uint32_t unProtocolID );

/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern osMessageQId	hOBDTxMessage;
extern osMessageQId	hOBDRxMessage;
extern osMessageQId	hOBDKlineTxMessage;
extern osMessageQId	hOBDKlineRxMessage;

extern osThreadId	hOBDKlineTxTh;
extern osThreadId	hOBDKlineRxTh;

extern bool         g_bSendPeriodicFlag;
extern uint32_t     g_uiSendPeriodicOldTime;
extern U32 			g_uiAckTiming;
#endif // __GIT_OBDCOMM_H_