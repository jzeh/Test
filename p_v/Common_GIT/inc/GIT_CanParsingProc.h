/**
  ******************************************************************************
  * @file    Share_CanParsingProc.h
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    20-MAR-2014
  * @brief   Header for GIT_CanParsingProc.h module
  ******************************************************************************
 **/

#ifndef __GIT_CAN_PARSING_PROC_H__
#define __GIT_CAN_PARSING_PROC_H__
#include "common.h"
#include "GIT_PassThruDefines.h"
#include "GIT_Interprotocol.h"
#include "GIT_Util.h"
#include "GIT_VCI.h"
#include "HalCanDriver.h"


/* Define to prevent recursive inclusion -------------------------------------*/
#define CAN_FRAME_SOF			0x01
#define CAN_FRAME_EOF			0x7F
#define CAN_FRAME_STANDARD_IDE	0x00 // standard CAN:0, extend CAN : 1
#define CAN_FRAME_EXTEND_IDE	0x01 // standard CAN:0, extend CAN : 1
#define CAN_FRAME_DATA_SIZE 	HAL_CAN_FRAME_DATA_SIZE


#define CAN_SINGLE_FRAME		0X00
#define CAN_FIRST_FRAME			0x10
#define CAN_CONSECUTICE_FRAME	0x20
#define CAN_FLOWCTRL_FRAME 		0x30
#define CAN_FS_CTS				0x00
#define CAN_FS_WAIT				0x01
#define CAN_FS_OVERFLOW			0x02

#define DCAN_GET_COMM_STATE() (g_eCanCommState)
//#define VCAN_GET_COMM_STATE() (g_eVCanCommState)
#define LCAN_GET_COMM_STATE() (g_eLCanCommState)
#define DCAN_SET_COMM_STATE(X) (g_eCanCommState = X)
//#define VCAN_SET_COMM_STATE(X) (g_eVCanCommState = X)
#define LCAN_SET_COMM_STATE(X) (g_eLCanCommState = X)

typedef enum _eCanComRxTxState
{
	eCAN_NONE_STATE,					//0
	eCAN_TX_NONE_PARSING,
	eCAN_TX_SINGLE_FRAME,
	eCAN_TX_FIRST_FRAME,
	eCAN_TX_CONSECUTIVE_FRAME,
	eCAN_TX_FLOWCONTROL_FRAME,	//5
	eCAN_RX_SINGLE_FRAME,
	eCAN_RX_FIRST_FRAME,
	eCAN_RX_CONSECUTIVE_FRAME,
	eCAN_RX_FLOWCONTROL_FRAME,
	eCAN_RX_BLOCK,							//10
	eCAN_RX_SLEEP_CHECK,
	eCAN_RX_IDLE,
	eCAN_RX_FAIL,
	eCAN_RX_DONE,
	eCAN_MAX_STATE
}eCanComRxTxState;

//typedef enum _eVCanComRxTxState
//{
//	eVCAN_NONE_STATE,
//	eVCAN_TX_NONE_PARSING,
//	eVCAN_TX_SINGLE_FRAME,
//	eVCAN_TX_FIRST_FRAME,
//	eVCAN_TX_CONSECUTIVE_FRAME,
//	eVCAN_TX_FLOWCONTROL_FRAME,
//	eVCAN_RX_SINGLE_FRAME,
//	eVCAN_RX_FIRST_FRAME,
//	eVCAN_RX_CONSECUTIVE_FRAME,
//	eVCAN_RX_FLOWCONTROL_FRAME,
//	eVCAN_RX_BLOCK,
//	eVCAN_RX_SLEEP_CHECK,
//	eVCAN_RX_IDLE,
//	eVCAN_RX_FAIL,
//	eVCAN_RX_DONE,
//	eVCAN_MAX_STATE
//}eVCanComRxTxState;

typedef enum _eLCanComRxTxState
{
	eLCAN_NONE_STATE,					//0
	eLCAN_TX_NONE_PARSING,
	eLCAN_TX_SINGLE_FRAME,
	eLCAN_TX_FIRST_FRAME,
	eLCAN_TX_CONSECUTIVE_FRAME,
	eLCAN_TX_FLOWCONTROL_FRAME,	//5
	eLCAN_RX_SINGLE_FRAME,
	eLCAN_RX_FIRST_FRAME,
	eLCAN_RX_CONSECUTIVE_FRAME,
	eLCAN_RX_FLOWCONTROL_FRAME,
	eLCAN_RX_BLOCK,							//10
	eLCAN_RX_SLEEP_CHECK,
	eLCAN_RX_IDLE,
	eLCAN_RX_FAIL,
	eLCAN_RX_DONE,
	eLCAN_MAX_STATE
}eLCanComRxTxState;

typedef enum _eCanType
{
	eDCAN,
	eVCAN,
	eLCAN,
	eFAIL
}eCanType;

#pragma pack(push, 1)

#define SIZE_CAN_DATA_FIELD		8
typedef __packed struct _stStandardCanType
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

typedef __packed struct _stExtendCanType
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


typedef __packed union _stCanPacket
{
	stStandardCanType stNormalPacket;
	stExtendCanType stExtendPacket;
}stCanPacket;

#pragma pack(pop,1)


extern BOOL	g_bCARBReceiving;
extern U8 							g_ucCanCommSucces;
extern unsigned int				g_uiCanReadMsgLength;
extern unsigned long 		g_ulCanWrittenTick;
extern eCanComRxTxState 	g_eCanCommState;
//extern eVCanComRxTxState 	g_eVCanCommState;
extern eLCanComRxTxState 	g_eLCanCommState;


void CAN_InitVariable(void);
void CAN_uDelay(unsigned int uiDelay);
void CAN_mDelay(unsigned int uiDelay);
void CAN_Reinit(stQueue *pQue);

unsigned int CAN_WriteBuff(unsigned char* pBuff, unsigned int nCount, unsigned int uiCANChannel, int nCanFrameType);
unsigned int CAN_TxBlockProc(void);
//unsigned int VCAN_TxBlockProc(void);
unsigned int LCAN_TxBlockProc(void);
void CAN_TxParsing(stCanPacket *pOutCanPacket, stPASSTHRU_MSG *pWriteMsg, BOOL *pbStandardCAN, eCanType eCurrCanType);
BOOL IsCanPassThruWriteMsgSaved(void);
void CAN_SavePassThruWriteMsg(unsigned char *pData, unsigned int uiDataLen);
void CAN_MakeSendFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, int nCanFrameType, stPASSTHRU_MSG *pWriteMsg);
void CAN_MakeSendFrame_Lcan(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, int nCanFrameType, stPASSTHRU_MSG *pWriteMsg);
void CAN_MakeReadMsgFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg);
BOOL CAN_CheckStandardTxFrameType(unsigned short *pusDLCLength, stPASSTHRU_MSG *pWriteMsg);
unsigned int CAN_MakeTxSingleFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, stPASSTHRU_MSG *pWriteMsg, eCanType eCurrCanType);
void CAN_MakeTxFlowControlFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pWriteMsg, eCanType eCurrCanType);
unsigned int CAN_MakeTxFirstFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pWriteMsg, eCanType eCurrCanType);
unsigned int CAN_MakeTxConsecutiveFrame(stCanPacket *pOutCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pWriteMsg, eCanType eCurrCanType);

eCanType CAN_FindCanPacket(unsigned char* pBuff, stCanPacket *pInCanPacket, BOOL *pbStandardCan);
eCanType LCAN_FindCanPacket(unsigned char* pBuff, stCanPacket *pInCanPacket, BOOL *pbStandardCan);
eCanType LCAN_FindCanPacket_FD(unsigned char* pBuff, stCanPacket *pInCanPacket, BOOL *pbStandardCan);
void CAN_RxParsing(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
void CAN_RxParsing_Low(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
void CANFD_RxParsing(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
void CANFD_RxParsing_Low(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);

BOOL CAN_RxBlockProc(stCanPacket *pInCanPacket, BOOL bStandardCan, stPASSTHRU_MSG *pReadMsg);
BOOL VCAN_RxBlockProc(stCanPacket *pInCanPacket, BOOL bStandardCan, stPASSTHRU_MSG *pReadMsg);
BOOL LCAN_RxBlockProc(stCanPacket *pInCanPacket, BOOL bStandardCan, stPASSTHRU_MSG *pReadMsg);
void CAN_MakeRxSingleFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg);
void CAN_MakeRxFirstFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg);
void CAN_MakeRxFlowControlFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg);
void CAN_MakeRxConsecutiveFrame(stCanPacket *pInCanPacket, BOOL bIsStandardCan, eCanComRxTxState eCanRxState, stPASSTHRU_MSG *pReadMsg);

void VCI_PeriodicMessage(void);
unsigned int FineWriteCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int FineReadCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);

int TestCANMasking(unsigned int *puiStartCANID, unsigned int* puiEndCANID, unsigned int nMaskCount);

BOOL CAN_CheckP3MinTimeout(void);
void Can_DeInit_Sleep(void);
#endif
/***************************** END OF FILE ****/
