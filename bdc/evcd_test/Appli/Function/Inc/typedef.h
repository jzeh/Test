#ifndef __TYPEDEF_H
#define __TYPEDEF_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------
	includes
----------------------------------------------------------------------*/
#include <stdbool.h>

/*----------------------------------------------------------------------
	typedefs
----------------------------------------------------------------------*/
typedef	bool									BOOL;

typedef	unsigned char							U8;
typedef	unsigned short							U16;
typedef	unsigned int							U32;
typedef	volatile unsigned char					VU8;
typedef	volatile unsigned short					VU16;
typedef	volatile unsigned int					VU32;
typedef	signed char								S8;
typedef	signed short							S16;
typedef	signed int								S32;

typedef	unsigned char							u8;
typedef	unsigned short							u16;
typedef	unsigned int							u32;
typedef	volatile unsigned char					vu8;
typedef	volatile unsigned short					vu16;
typedef	volatile unsigned int					vu32;
typedef	signed char								s8;
typedef	signed short							s16;
typedef	signed int								s32;

typedef void (* UserIntCallBack_t)(void);
typedef void (*pFunction)(void);

/*----------------------------------------------------------------------
	defines
----------------------------------------------------------------------*/
#ifndef FALSE
	#define FALSE										false
	#define TRUE										true
#endif

#define	INIT_OK											0
#define	INIT_FAIL										-1

// Message
//#define	MESSAGE_PARSING_QUEUE_SIZE				20						// To Parsing Thread MessageQ
//#define	MESSAGE_TRANSMIT_QUEUE_SIZE				20						// To Transmit Thread MessageQ
//#define MESSAGE_DIAGNOSTIC_QUEUE_SIZE				10						// To Diagnostic Thread MessageQ
#define MESSAGE_LOGGING_QUEUE_SIZE					10						// To Logging Thread MessageQ
#define	MESSAGE_SDCARD_QUEUE_SIZE					20						// To SDCARD Thread MessageQ
#define MESSAGE_WRITE_QUEUE_SIZE					20						// To Write Thread MessageQ
#define MESSAGE_EXTLED_QUEUE_SIZE					10						// To ExtLed Thread MessageQ
#define MESSAGE_TRIGGER_QUEUE_SIZE					10						// To Trigger Thread MessageQ

////////// FAT DEFINE
	#define FILENAME_MODE_SWITCH_INI	"ModeSwitch.ini"
	#define FILENAME_APP_LIST_INI		"AppSwList.ini"
	#define FILENAME_CV_ADAPTOR_INI		"CVAdaptorVer.ini"
	#define FILENAME_AUTOVIN_CONFIG_DAT	"AutoVinConfig.dat"
	#define FILENAME_VCI2_SERIAL_DAT	"VCI2Serial.dat"
	#define STR_FW_SWITCH_COMPLETE		"FW_SWITCH_COMPLETE"
	#define STR_FW_SWITCH_READY  		"FW_SWITCH_READY"
	#define BlockBox_STR_DAT			"BlockBox_STR.dat"
	#define FILENAME_FW_INFO_INI		"FirmwareInfo.ini"
	#define FILENAME_FW_INFO_BK_INI		"FirmwareInfoBackUp.ini"
	#define FILENAME_CRL_GIT			"CrlEmmc.git"
	#define S_REPRO_JUMP_FILE_NAME      "StandAloneReproStart.ini"
#if 0 // ASSAGAORY
	#define FILENAME_WDF_CH_INI			"WFD_chNum.ini"
	#define DEFAULT_WDF_CH_NUM			(48)
#endif
/*----------------------------------------------------------------------
	Message
----------------------------------------------------------------------*/
typedef enum                // VCI, PC, Server 통신 방법
{
	COMM_USB	= 0 ,
	COMM_UART	= 1 ,
	COMM_WIFI	= 2 ,
	COMM_BT		= 3 ,
	COMM_CAN1	= 4 ,
	COMM_CAN2	= 5
} eCOMM_IF;

typedef enum				// Diagnostic Interface
{
	DIAG_CAN		= 0 ,
	DIAG_KWP		= 1 ,
	DIAG_ETH		= 2 ,
	DIAG_PASSTHRU	= 3 ,
	AUTO_VIN		= 4 ,
	FAST_FCS		= 5 ,
	CAN_MOSA		= 6 ,
    CAN_CSAC		= 7 ,
} eDIAG_IF;

typedef enum				// Packet type
{
	PACKET_USB		= 0 ,
	PACKET_UART		= 1 ,
	PACKET_CAN		= 2 ,
	PACKET_FDCAN	= 3 ,
	PACKET_BT		= 4 ,
	PACKET_RECORD	= 5 ,	
} ePKT_TD;

typedef enum				// Message type
{
	MSG_COMM		= 0 ,
	MSG_DIAG		= 1 ,
	MSG_LOG			= 2 ,
	MSG_MMC			= 3 ,
    MSG_RECORD      = 4 , 
    MSG_TRIGGER		= 5 ,
	MSG_PERIODIC	= 6 ,
} eMSG_TD;

typedef enum				// Diagnostic State
{
	eDIAG_COMM_NONE			= 0 ,
	eDIAG_COMM_TX_START		= 1 ,
	eDIAG_COMM_TX_OK		= 2 ,
	eDIAG_COMM_TX_FAIL		= 3 ,
	eDIAG_COMM_RX_ING		= 4 ,
	eDIAG_COMM_RX_OK		= 5 ,
	eDIAG_COMM_RX_FAIL		= 6 ,
	eDIAG_COMM_RX_COMPLETE	= 7 ,
	eDIAG_COMM_MAX			= 8 ,
} eDIAG_COMM_STATE;

typedef enum				// Main State
{
	eMain_NONE		= 0 ,
	eMain_Init		= 1 ,
	eMain_Run		= 2 ,
	eMain_Sleep		= 3
} eMain_State;

/*----------------------------------------------------------------------
 *   Structure
 *--------------------------------------------------------------------*/
typedef struct _CommPkt
{
	void	*pSource;							// Source I/F
	void	*pTarget;							// Targst T/F

	U16		mLen;
	u16		mFuncID;
	u16		mCurFrame;
	u16		mCS;

	U8		mData[4200];
} stCommPkt;

typedef struct _stMsgClst
{
	eMSG_TD	mMsgType;							// Message Type
	ePKT_TD	mPktType;							// Packet Type

	u16		mLen;
	u16		mMod;
	u8		mSeq;
	u8		mCS;

	void	*pPacket;							// Packet
} stMsgClst;

typedef struct
{
	eMSG_TD	mMsgType;							// Message Type
	ePKT_TD	mPktType;							// Packet Type

    u16		event;
    u32		subEvent;
    u8		result;
    u16		seq;
    u32		unTracePs;
    u32		unEventTime;

	void	*pPacket;							// Packet
} MsgDiag_t;

/*----------------------------------------------------------------------
	Message
----------------------------------------------------------------------*/
typedef struct
{
	eMSG_TD	mMsgType;							// Message Type
	ePKT_TD	mPktType;							// Packet Type

	u16		mLen;
	u16		mMod;
	u8		mSeq;
	u8		mCS;

	void	*pPacket;							// Packet
} MsgClst_t;

/*----------------------------------------------------------------------
 *   PassThru Message Structure (for CAN diagnostics)
 *--------------------------------------------------------------------*/
#define SIZE_PASSTHRU_HEADER			(sizeof(unsigned long) * 6)
#define MAX_PASSTHRUMSG_DATA_SIZE		4128

typedef struct _stPASSTHRU_MSG
{
    unsigned long ProtocolID; 	/**< Protocol type */
    unsigned long RxStatus; 	/**< Receive message status */
    unsigned long TxFlags; 		/**< Transmit message flags */
    unsigned long Timestamp; 	/**< Received message timestamp (microseconds) */
    unsigned long DataSize; 	/**< Data size in bytes */
    unsigned long ExtraDataIndex; /**< Start position of extra data */
	unsigned char pData[MAX_PASSTHRUMSG_DATA_SIZE]; /**< Array of data bytes */
} stPASSTHRU_MSG, PTmsgPkt_t;

#ifdef __cplusplus
}
#endif

#endif /* __TYPEDEF_H */