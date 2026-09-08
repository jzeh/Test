
#ifndef __TASK_BSA_H__
#define __TASK_BSA_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include <string.h>
#include "../Inc/sys-common.h"
#include "../Inc/git-comm.h"

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define MESSAGE_BATRELAYCON_QUEUE_SIZE		1000

#define CRC16_POLY							0x1021
#define CRC16_INIT_VALUE					0xFFFF
#define CRC16_ADD_VALUE						0xF800

#define MOSA_RX_TIMEOUT						55
#define MOSA_RX_CANID_SIZE					2
#define MOSA_RX_CANLENTH_SIZE				1

#define NE_PRECHRGSTA_CANID					0x0235
#define NE_MAINRLYONSTA_CANID				0x02FA
#define NE_BATPACP_VOLT_DATA_SIZE			2
#define NE_BATPACP_VOLT_DATA_POSITION		13
#define NE_BATPACP_VOLT_FACTOR				0.1
#define NE_CHRG_STATE_DATA_SIZE				1
#define NE_CHRG_STATE_DATA_POSITION			3

#define OS_MONITORING_CANID					0x0595
#define OS_BATPACP_VOLT_DATA_SIZE			2
#define OS_BATPACP_VOLT_DATA_POSITION		6
#define OS_BATPACP_VOLT_FACTOR				0.1

// MOSA Configuration
#define BSA_MAX_TX_SIGNALS					10
#define BSA_MAX_RX_SIGNALS					10
#define BSA_MAX_CYCLES						5
#define BSA_MAX_DATA_LENGTH					64		// CANFD max length
#define BSA_MAX_SIMFRAME_CONFIGS			10		// Maximum simframe TX configs (BLE 0x22)
#define BSA_MAX_MONITORING_CONFIGS			5		// Maximum monitoring RX configs (BLE 0x21)

// BSA Data Config
#define BSA_DATA_CONFIG_MSG_MAX				16		// Maximum message count per config
#define BSA_DATA_CONFIG_HDR_SIZE			8		// protocolId(2) + dataRate(1) + responseValue(2) + interval_ms(2) + messageCount(1)
#define BSA_DATA_CONFIG_MSG_SIZE			16		// Per-message payload size (bytes)
#define BSA_SIMFRAME_BASE_HDR_SIZE			4		// protocolId(2) + dataRate(1) + requestValueLength(1)
#define BSA_SIMFRAME_TAIL_SIZE				3		// interval_ms(2) + messageCount(1)

// BSA Step Config (BLE 0x23)
#define BSA_STEP_HDR_SIZE					6		// stepno(1) + type(1) + messageindex(1) + interval_ms(2) + messageCount(1)
#define BSA_STEP_MSG_SIZE					7		// valueType(1)+code(2)+min(2)+max(2)
#define BSA_STEP_MAX_MSGS					8		// Max messages per step entry
#define BSA_STEP_MAX_ENTRIES				32		// Max total step entries

// CAN Packet Type
#define CAN_TYPE_STANDARD					0x00
#define CAN_TYPE_EXTENDED					0x01
#define CAN_TYPE_FD_STANDARD				0x02
#define CAN_TYPE_FD_EXTENDED				0x03

// BSA Simframe protocolId → CAN type
#define BSA_PROTOCOL_CAN_CLASSIC			0x0100
#define BSA_PROTOCOL_CAN_FD					0x0130
#define BSA_REQVAL_CANID_SIZE				2		// CAN ID prefix bytes in requestValue

/* ── BSA CAN/FD HW config 전역 — 0x21 (Monitoring) / 0x22 (Simframe) 수신 시 갱신.
 *    CAN_HW_setting() 가 이 값을 보고 protocolId / dataRate 로 baud + frame format 결정.
 *    초기값: 종래 하드코딩과 동일 (FD BRS 500k/2M) → BLE 0x21/0x22 미수신 시에도 안전. */
extern volatile uint16_t g_bsaHwProtocolId;     /* BSA_PROTOCOL_CAN_CLASSIC or BSA_PROTOCOL_CAN_FD */
extern volatile uint8_t  g_bsaHwDataRate;       /* 0x00 ~ 0x07 (LUT 인덱스) */
extern volatile bool     g_bsaHwConfigValid;    /* 0x21/0x22 로 한 번이라도 갱신되면 true */

// BSA Monitoring valueType definitions (used in stBSA_DataConfigMsg for monitoring)
#define BSA_VALUETYPE_MON_VOLTAGE			0x01
#define BSA_VALUETYPE_MON_PRECHARGE_RLY		0x02
#define BSA_VALUETYPE_MON_MAIN_RLY			0x03
#define BSA_VALUETYPE_MON_IG_STATUS			0x04

// BSA Simframe valueType definitions (used in stBSA_DataConfigMsg for TX)
#define BSA_VALUETYPE_SIM_CRC16				0x01
#define BSA_VALUETYPE_SIM_ALIVE_COUNT		0x02
#define BSA_VALUETYPE_SIM_VOLTAGE			0x03

// Application Communication
#define MOSA_PROTOCOL_ID					0x8000
#define MOSA_PROTOCOL_DATA_LEN				2
#define MOSA_RES_SUCCESS					0x00
#define MOSA_RES_FAIL						0x01
#define MOSA_VEHICLETYPE_NEEV				0x01
#define MOSA_VEHICLETYPE_DEEV				0x02
#define MOSA_VEHICLETYPE_OSEV				0x03

// Hardware Setting Types (legacy VCI3 compatibility)
#define BAT_RELAY_CON						0x0100
#define BAT_FD_RELAY_CON					0x0200


/*----------------------------------------------------------------------
 *   struck & enum
 *--------------------------------------------------------------------*/
typedef enum {
	BatRelayConStatus_None = 0,
	BatRelayConStatus_Init,
	BatRelayConStatus_Run,
	BatRelayConStatus_Idle,
	BatRelayConStatus_Stop,
	BatRelayConStatus_Exit,
	BatRelayConStatus_PRA_On,
	BatRelayConStatus_PRA_Off,
	BatRelayConStatus_Max
}eBatRelayConStatus;

typedef enum {
	BatRelayConType_None = 0,
	BatRelayConType_NEEV,
	BatRelayConType_DEEV,
	BatRelayConType_OSEV,
	BatRelayConType_Max
}eBatRelayConType;

typedef enum {
	PRAConStatus_Open = 0,
	PRAConStatus_PRA_On,
	PRAConStatus_PRA_Off,
	PRAConStatus_None
}ePRAConStatus;

typedef enum {
	CANCommType_Classic = 0,
	CANCommType_FD,
	CANCommType_Max
}eCANCommType;

typedef enum {
	eSignalType_BSAVoltage = 0,
	eSignalType_PrechargeRelayStatus,
	eSignalType_MainRelayStatus,
	eSignalType_IGStatus,
	eSignalType_Max
}eBSA_MonSignalType;

typedef enum {
	MOSAConType_Mosa_Start = 0x01,
	MOSAConType_Mosa_Stop = 0x02,
	MOSAConType_PRA_ON = 0x03,
	MOSAConType_PRA_OFF = 0x04
}eMOSAConType;

typedef enum {
	eBSA_MainRly_Off = 0x00,
	eBSA_MainRly_On = 0x01
}eBSA_MainRelayStatus;

typedef enum {
	eBSA_PreRly_Off = 0x00,
	eBSA_PreRly_On = 0x04
}eBSA_PrechargeRelayStatus;
typedef struct {
	char* pcPreData;
	char* pcData;
	uint16_t unCycle;
	char cAliveCnt;
	uint16_t usCanId;
	uint16_t usCanLen;
}stBatRelayConData;

typedef struct _stBSA_COMM_DATA{
	char 		BSA_IDX[4];
	char    	BSA_canId[4];
	uint8_t    	BSA_start_bit;			// can data field 에서의 데이터 포지션 (bit 단위)
	uint8_t    	BSA_bit_length;		// 데이터 길이 (bit 단위)
	float		BSA_scale_factor;
	float 		BSA_offset;
	char		BSA_unit[8];

	uint8_t		BSA_comm_type;			// CAN CLASSIC(0)/ CAN FD(1)
	char		BSA_alive_cnt;
	uint16_t 	BSA_cycle;				// TX Cycle (ms)
	
	uint8_t		BSA_data_len;			// DATA TOTAL LENGTH (byte)
	eBSA_MonSignalType BSA_signal_type;	// Signal type identifier
	bool		is_tx_signal;			// true: TX signal, false: RX signal
	uint16_t 	reserved[32];
} stBSA_COMM_DATA;

// 이름 공용으로 
// ㄴ 모사 		구조체
// ㄴ 모니터링	 구조체
typedef struct {
	uint16_t	mosa_BSA_voltage;		// BSA Voltage (scaled value)
	uint8_t		mosa_PreRly_status;		// Precharge Relay Status
	uint8_t		mosa_MainRly_status;	// Main Relay Status
	uint8_t		mosa_IGON_status;		// IG ON/OFF
} stBSA_MON_DATA;

typedef struct {
	uint32_t	last_time;
	uint16_t	cycle_ms;
} stBSA_TX_TIMER;

/**
 * @brief Dynamic TX packet configuration structure
 * 외부에서 설정 가능한 TX 패킷 구조체
 */
typedef struct {
	uint16_t	canId;						// CAN ID
	uint8_t		canType;					// CAN_TYPE_STANDARD/EXTENDED/FD_STANDARD/FD_EXTENDED
	uint16_t	cycle_ms;					// TX cycle in milliseconds
	uint8_t		dataLen;					// Data length (8 for CAN, up to 64 for CANFD)
	uint8_t		data[BSA_MAX_DATA_LENGTH];	// TX data buffer (초기값 0x00)
	uint8_t		aliveCntPos;				// Alive counter position in data (-1 if not used)
	uint8_t		voltagePos;					// Voltage position in data (-1 if not used)
	uint8_t		crcPos;						// CRC position in data (-1 if not used)
	bool		enabled;					// TX enable flag
	uint8_t		aliveCnt;					// Internal alive counter
} stBSA_TX_CONFIG;

/**
 * @brief TX Configuration Manager
 */
typedef struct {
	stBSA_TX_CONFIG txConfigs[BSA_MAX_TX_SIGNALS];
	uint8_t			txCount;
	stBSA_TX_TIMER	timers[BSA_MAX_CYCLES];
	uint8_t			timerCount;
} stBSA_TX_MANAGER;

/**
 * @brief BSA Data Config - Conversion Rule (conv rule)
 */
typedef struct {
	uint8_t     convType;       // conv rule type (convtype)
	uint16_t    convA;          // conv rule A (2 bytes)
	uint16_t    convB;          // conv rule B (2 bytes)
	uint8_t     convC;          // conv rule C
	uint8_t     convD;          // conv rule D
	uint8_t     convE;          // conv rule E
	uint8_t     convF;          // conv rule F
} stBSA_ConvRule;

/**
 * @brief BSA Data Config - Single Message Entry (items 8~18)
 */
typedef struct {
	uint8_t         valueType;      // value type
	uint8_t         startPosition;  // start position
	uint8_t         dataSize;       // data size
	uint32_t        maskingValue;   // masking value (4 bytes)
	stBSA_ConvRule  convRule;       // conversion rule (convtype + A~F)
} stBSA_DataConfigMsg;

/**
 * @brief BSA Data Config - Full Payload Structure (Monitoring)
 *        message count 값에 따라 message 반복
 *
 *  protocolId: 0x0100=CAN Classic, 0x0130=CAN FD
 *  dataRate:   ndatarate (0-7) bitrate config index
 *  responseValue: CAN ID to monitor (RX match)
 *  message.startPosition: byte offset in CAN data field
 */
typedef struct {
	uint16_t            protocolId;     // CAN type (0x0100=Classic, 0x0130=FD)
	uint8_t             dataRate;       // ndatarate (0-7) bitrate config index
	uint16_t            responseValue;  // CAN ID to monitor (2 bytes)
	uint16_t            interval_ms;    // (reserved, not used) RX interval placeholder
	uint8_t             messageCount;   // Message count
	stBSA_DataConfigMsg messages[BSA_DATA_CONFIG_MSG_MAX];
} stBSA_DataConfig;

/**
 * @brief BSA Simframe Config - Full Payload Structure
 *        protocolId(2) + dataRate(1)
 *        + requestValue(N, CAN type dependent)
 *        + interval_ms(2) + messageCount(1)
 *        + message × messageCount
 *
 *  protocolId: 0x0100=CAN Classic, 0x0130=CAN FD
 *  dataRate:   ndatarate (0-7) bitrate config index
 *  requestValue: [CAN_ID(2B LE)] + [CAN_DATA(N bytes)]
 *  message.startPosition: relative to CAN DATA (requestValue+2)
 */
typedef struct {
	uint16_t            protocolId;                         // CAN type (0x0100=Classic, 0x0130=FD)
	uint8_t             dataRate;                           // ndatarate (0-7) bitrate config index
	uint8_t             requestValue[BSA_MAX_DATA_LENGTH + BSA_REQVAL_CANID_SIZE];  // [CAN_ID(2)] + [CAN_DATA(max 64)]
	uint8_t             requestValueLen;                    // Actual requestValue length
	uint16_t            interval_ms;                        // TX interval (ms)
	uint8_t             messageCount;                       // Message count
	stBSA_DataConfigMsg messages[BSA_DATA_CONFIG_MSG_MAX];
} stBSA_SimframeConfig;

/**
 * @brief BSA Step Config Message (7 bytes)
 *        valueType(1) + code(2) + min(2) + max(2)
 */
typedef struct {
	uint8_t     valueType;
	uint16_t    code;
	uint16_t    minValue;
	uint16_t    maxValue;
} stBSA_StepMsg;

/* BSA Step type values */
#define BSA_STEP_TYPE_MONITOR				0x01    /* Monitor (Monitoring config 기반) */
#define BSA_STEP_TYPE_SIMFRAME				0x02    /* Simframe (TX frame config 기반) */

/**
 * @brief BSA Step Config Entry
 *        stepno(1) + type(1) + messageindex(1) + interval_ms(2 BE) + messageCount(1)
 *        + messages (messageCount × 7B)
 */
typedef struct {
	uint8_t         stepno;
	uint8_t         type;
	uint8_t         messageIndex;
	uint16_t        interval_ms;
	uint8_t         messageCount;
	stBSA_StepMsg   messages[BSA_STEP_MAX_MSGS];
} stBSA_StepConfig;

/**
 * @brief BSA Step Data (all step entries)
 */
typedef struct {
	uint16_t          count;
	stBSA_StepConfig  steps[BSA_STEP_MAX_ENTRIES];
	bool              configured;
} stBSA_StepData;

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
bool StartBSAThread(void);
void BatRelayConThread(void *argument);
void BatRelayMonitorThread(void *argument);
void BatRelayConEventListener();
void SendMSGToBatRelayCon(u16 Mode, stMsgClst *message);
unsigned short CalculateCRC16(char* pcData, unsigned int unLen);
void MakeNeEvTxPacket(char* pcData, char* pcPreData, unsigned short usCanId, char cCnt);
void MakeOsEvTxPacket(char* pcData, unsigned short usCanId);
void BatRelayCon_WriteCanPacket(stBatRelayConData* pstData, uint16_t unTime, eBatRelayConType eType, uint16_t unCount);
void NEEV_PRAContorl(ePRAConStatus eType);
unsigned short ByteSwap(unsigned short usData);

/*----------------------------------------------------------------------
 *   Dynamic Config Functions (BLE-configured monitoring/simframe)
 *--------------------------------------------------------------------*/
void BSA_ApplyDataConfig_Monitoring(uint16_t rxCanId, uint8_t* pData, uint8_t dataLen);
void BSA_ProcessSimframeTx(uint32_t currentTime);
void BSA_ResetSimframeState(void);
void BSA_Start(void);
void BSA_Stop(void);

/*----------------------------------------------------------------------
 *   New Flexible TX/RX Functions
 *--------------------------------------------------------------------*/
// TX Configuration Functions
void BSA_InitTxManager(void);
int BSA_AddTxConfig(uint16_t canId, uint8_t canType, uint16_t cycle_ms, uint8_t dataLen);
int BSA_SetTxData(uint8_t configIdx, uint8_t* pData, uint8_t len);
int BSA_SetTxAliveCounterPos(uint8_t configIdx, uint8_t pos);
int BSA_SetTxVoltagePos(uint8_t configIdx, uint8_t pos);
int BSA_SetTxCRCPos(uint8_t configIdx, uint8_t pos);
int BSA_EnableTxConfig(uint8_t configIdx, bool enable);
void BSA_EnableExternalTx(bool enable);
void BSA_ProcessTxPackets(uint32_t currentTime);
void Process_BSA_RxSignal(uint16_t canId, uint8_t* pData, uint8_t dataLen);

/*----------------------------------------------------------------------
 *   CAN Packet Union Structure (for VCI3 compatibility)
 *--------------------------------------------------------------------*/
#define SIZE_CAN_DATA_FIELD		8
#define SIZE_FDCAN_DATA_FIELD	64

#pragma pack(push, 1) 

typedef struct _stStandardCanType
{
	unsigned short	ucSOF:1;			// Start of frame
	unsigned short	us11BitID:11;		// 11-bit identifier
	unsigned short	ucRTR:1;			// Remote transmission request
	unsigned short	ucIDE:1;			// Identifier extension bit
	unsigned short	ucReserved:1;		// Reserved bit
	unsigned short	ucDummy1:1;			// Network alignment dummy
	unsigned short	ucDLC:4;			// Data length code (0-8 bytes)	
	unsigned short	ucDummy2:12;		// Network alignment dummy
	unsigned char 	arrDataFields[SIZE_CAN_DATA_FIELD];	// Data bytes
	unsigned short	usCRC:15;			// Cyclic redundancy check
	unsigned short	ucCRCDelimiter:1; 	// CRC delimiter
	unsigned short	ucACK:1;			// Acknowledgment bit
	unsigned short	ucACKDelimiter:1;	// ACK delimiter
	unsigned short	ucEOF:7;			// End of frame
}stStandardCanType;

typedef struct _stStdFDCanType
{
	unsigned short	ucSOF:1;			// Start of frame
	unsigned short	us11BitID:11;		// 11-bit identifier
	unsigned short	ucReserved1:1;		// Reserved bit
	unsigned short	ucIDE:1;			// Identifier extension (0=standard)
	unsigned short	ucEDL:1;			// Extended data length (1=CANFD)
	unsigned short	ucReserved0:1;		// Reserved bit
	unsigned short	ucBRS:1;			// Bit rate switch
	unsigned short	ucESI:1;			// Error state indicator
	unsigned short	ucDummy1:14;		// Network alignment dummy
	unsigned short	ucDLC:8;			// Data length code
	unsigned short	ucDummy2:12;		// Network alignment dummy
	unsigned char 	arrDataFields[SIZE_FDCAN_DATA_FIELD];	// Data bytes (up to 64)
	unsigned int	usCRC:21;			// CRC (17 or 21 bits for CANFD)
	unsigned short	ucCRCDelimiter:1; 	// CRC delimiter
	unsigned short	ucACK:1;			// Acknowledgment bit
	unsigned short	ucACKDelimiter:1;	// ACK delimiter
	unsigned short	ucEOF:7;			// End of frame
}stStdFDCanType;

typedef struct _stExtendCanType
{
	unsigned short	ucSOF:1;			// Start of frame
	unsigned short	us11BitID:11;		// First 11 bits of 29-bit identifier
	unsigned short	ucSRR:1;			// Substitute remote request
	unsigned short	ucIDE:1;			// Identifier extension (1=extended)
	unsigned short	ucDummy0:2;			// Network alignment dummy
	unsigned int	us18BitID:18;		// Last 18 bits of 29-bit identifier
	unsigned short	ucRTR:4;			// Remote transmission request
	unsigned short	ucReserved:1;		// Reserved bit
	unsigned short	ucDLC:4;			// Data length code
	unsigned short	ucDummy1:5;			// Network alignment dummy
	unsigned char 	arrDataFields[SIZE_CAN_DATA_FIELD];	// Data bytes
	unsigned short	usCRC:15;			// Cyclic redundancy check
	unsigned short	ucCRCDelimiter:1; 	// CRC delimiter
	unsigned short	ucACK:1;			// Acknowledgment bit
	unsigned short	ucACKDelimiter:1;	// ACK delimiter
	unsigned short	ucEOF:7;			// End of frame
}stExtendCanType;

typedef struct _stExtFDCanType
{
	unsigned short	ucSOF:1;			// Start of frame
	unsigned short	us11BitID:11;		// First 11 bits of 29-bit identifier
	unsigned short	ucSRR:1;			// Substitute remote request
	unsigned short	ucIDE:1;			// Identifier extension (1=extended)
	unsigned short	ucDummy0:2;			// Network alignment dummy
	unsigned int	us18BitID:18;		// Last 18 bits of 29-bit identifier
	unsigned short	ucReserved1:1;		// Reserved bit
	unsigned short	ucEDL:1;			// Extended data length (1=CANFD)
	unsigned short	ucReserved0:1;		// Reserved bit
	unsigned short	ucBRS:1;			// Bit rate switch
	unsigned short	ucESI:1;			// Error state indicator
	unsigned short	ucDLC:4;			// Data length code
	unsigned short	ucDummy1:5;			// Network alignment dummy
	unsigned char 	arrDataFields[SIZE_FDCAN_DATA_FIELD];	// Data bytes (up to 64)
	unsigned int	usCRC:21;			// CRC (17 or 21 bits for CANFD)
	unsigned short	ucCRCDelimiter:1; 	// CRC delimiter
	unsigned short	ucACK:1;			// Acknowledgment bit
	unsigned short	ucACKDelimiter:1;	// ACK delimiter
	unsigned short	ucEOF:7;			// End of frame
}stExtFDCanType;

typedef union _stCanPacket
{
	stStandardCanType	 stNormalPacket;
	stExtendCanType		 stExtendPacket;
	stStdFDCanType		 stFDStdPacket;
	stExtFDCanType		 stFDExtPacket;
}stCanPacket;

#pragma pack(pop,1) 

/*----------------------------------------------------------------------
 *   Helper Functions (EVCD implementations)
 *   Note: Get_Tmr / Get_TmrDelta / GetUnixTime moved to sys-main.c
 *         (declared in sys-common.h)
 *--------------------------------------------------------------------*/
void InitGITSetConfig(void);
void InitGITHWSetData(void);
void VCI_HW_Setting(uint32_t hwType);

// Logging macros (wrappers for printf)
#define GLogE(fmt, ...)  printf("[ERR] " fmt, ##__VA_ARGS__)
#define GLogN(fmt, ...)  printf("[NFO] " fmt, ##__VA_ARGS__)
#define GLogI(fmt, ...)  printf("[INF] " fmt, ##__VA_ARGS__)

/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern bool g_bBatRelayConFlag;
extern bool g_bUseExternalTxConfig;
extern bool g_bBSA_DataConfigReady;
extern bool g_bBSA_SimframeConfigReady;
extern stBSA_TX_MANAGER g_stBSA_TxManager;
extern stBSA_DataConfig g_stBSA_DataConfigs[BSA_MAX_MONITORING_CONFIGS];
extern uint8_t g_ucBSA_DataConfigCount;
extern stBSA_SimframeConfig g_stBSA_SimframeConfigs[BSA_MAX_SIMFRAME_CONFIGS];
extern uint8_t g_ucBSA_SimframeConfigCount;
extern uint8_t g_SimframeAliveCnts[BSA_MAX_SIMFRAME_CONFIGS];
extern uint32_t g_SimframeTxTimers[BSA_MAX_SIMFRAME_CONFIGS];
extern stBSA_MON_DATA g_stBSA_MonData;
extern stBSA_StepData g_stBSA_StepData;

/* ===== BLE GDS Command Handlers (git-functionlist.c 에서 이동) ==========
 *  functionlist 의 FL_GDS_BSA_* 핸들러가 아래 함수를 호출.
 *  본체/동작은 이동 전과 완전히 동일 (verbatim). */
void BSA_Cmd_ConfigMonitoring(void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void BSA_Cmd_ConfigSimframe  (void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);
void BSA_Cmd_ConfigStep      (void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength);

#endif // __TASK_BSA_H__

