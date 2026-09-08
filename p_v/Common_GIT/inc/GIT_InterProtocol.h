/**
  ******************************************************************************
  * @file    GIT_InterProtocol.h
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    19-FEB-2014
  * @brief   Header for GIT_InterProtocol.h module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GIT_INTER_PROTOCOL_H__
#define __GIT_INTER_PROTOCOL_H__

#include "common.h"
#include "GIT_VCI.h"
#include "GIT_PassthruDefines.h"
//#include "J2534_1_define.h"
//#include "J2534_2_define.h"


typedef unsigned int (*pfnReadCallBack)(unsigned char*, unsigned int, void*, unsigned int);
typedef unsigned int (*pfnWriteCallBack)(unsigned char*, unsigned int, void*, unsigned int);
typedef unsigned int (*pfnCheckReadBuff)();
typedef unsigned int (*pfnParsingCallBack)(unsigned char*, unsigned int, void*, unsigned int);
typedef void			(*pfnParsingPayLoadCB)(void *pInterPtcl, unsigned int eInCommType);

typedef enum _eCommType
{
 eCOMM_TYPE_UART_MODEM,
 eCOMM_TYPE_CAN1,
 eCOMM_TYPE_CAN2,
 eCOMM_TYPE_UART_BT,
 eCOMM_TYPE_UART_GPS,

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
 eCOMM_TYPE_USB,
#endif
 eCOMM_TYPE_UART_SELFTEST,
#if defined(FEATURE_EXTENSION_BOARD)
 eCOMM_TYPE_UART_PLUSBD = eCOMM_TYPE_UART_SELFTEST,
#endif
 eCOMM_TYPE_MAX
}eCommType;





typedef enum _eCommState
{
	eCOMM_STATE_INIT,
	eCOMM_STATE_CONNECTING,
	eCOMM_STATE_CONNECTED,
	eCOMM_STATE_DISCONNECTING,
	eCOMM_STATE_DISCONNECTED,
	eCOMM_STATE_MAX
}eCommState;


///////////////////////////    BLE PROTOCOL   //////////////////////////////////////////////////////////////
/************************************************************************************************/
/* 		 0    |   1    |   2    |   3    |  4   |   5    |   6   |   7     | n-2 | 					*/
/* 		sof   |  Rev0  |  Rev1  |  Len0  | Len1 | FuncID1|FuncID2| Payload | CS	  					*/
// 재정의 필요
/************************************************************************************************/
#define	MAX_DCS_PROTO_DATA_LENGTH				512
#define MIN_DCS_PROTO_DATA_LENGTH				6	//Rev0 / Rev1 | Len0 | Len1 | FUNCID0 | FUNCID0

#define	DCSPACKET_SERIAL_SOF				0x02	//Start of Frame
#define	DCSPACKET_RESERVE1					0x00
#define	DCSPACKET_RESERVE2					0x00

#define DCSPACKET_SOF_IDX			        0
#define DCSPACKET_RESERVE1_IDX		        1
#define DCSPACKET_RESERVE2_IDX		        2
#define DCSPACKET_PAYLOAD_LEN0_IDX			3
#define DCSPACKET_PAYLOAD_LEN1_IDX			4
#define DCSPACKET_PAYLOAD_FUNC0_IDX			5
#define DCSPACKET_PAYLOAD_FUNC1_IDX			6
#define DCSPACKET_PAYLOAD_DATA_IDX			7
#define DCSPACKET_PAYLOAD_CS_IDX			8

#define DCSPACKET_SOF_SIZE					1
#define DCSPACKET_PAYLOAD_FUNC_SIZE			2
#define DCSPACKET_PAYLOAD_LENGTH_SIZE		2
#define DCSPACKET_PAYLOAD_RESERVE_SIZE		2
#define DCSPACKET_PAYLOAD_HEADER_SIZE		5
#define GITPACKET_MODE1_PAD		0xF1

#define DCS_CURRENT_INDEX_SIZE		3

#define DCS_DATE_INDEX_SIZE			7
#define DCS_AUTOVIN_SIZE			17
#define DCS_DTC_CODE_SIZE			4
#define DCS_ECU_ID_SIZE				4
#define DCS_VEHICLE_SYSTEM_MAX		30
#define DCS_DTCPACKET_HEADER_SIZE		18



///////////////////////////    PLUSBD PROTOCOL   //////////////////////////////////////////////////////////////
/************************************************************************************************/
/* 		 0  |  1   |  2   |  3   |  4   |  5  |  6     | n-2 | n-1								*/
/* 		sof | Len0 | Len1 | Mod0 | Mod1 | Seq |Payload | Eof | CS								*/
/* 		sof : Start Of Frame (0x02)																*/
/* 		Len0 & Len1 : sof & eof & cs 제외														*/
/* 		Mod0 & Mod1 : Mod0는 사용하고 있지 않음													*/
/*					  Mod1은 0xF1=PAD(tablet), 0xF3=VCI_II 무선 보드, 0xF7=VCI2, 0xFA = 트리거	*/
/* 		Seq : 0~255																				*/
/* 		Eof : End Of Frame(0x03)																*/
/* 		CS : CheckSum field	(1'st byte ~ n-2(Eof))까지의 모든 데이터의 합(sof만 제외됨)			*/
/************************************************************************************************/
/*			Payload frame																		*/
/* 		  D0  |   D1 |    D2   |   D3   |    D4     |     D5     |    D6     |     p-2   | p-1	*/
/*       len0 | len1 | funcID0 |funcID1 |CurrFrame0 | CurrFrame1 | Checksum0 | Checksum1 | DATA	*/
/*			Data -> len0 & len1 :  D0 ~ p-1까지의 길이											*/
/*			Data -> funcID0 & funcID1 :  Function ID of Frame									*/
/*			Data -> CurrFrame0 : not used														*/
/*			Data -> CurrFrame1 : not used														*/
/*			Data -> Checksum0 & Checksum : checksum이 0인 상태에서 D0 ~ p-1까지의 모든 합		*/
/************************************************************************************************/



////////////////////////////////////////////////////////////////////////////////
// woong bae 18/08/30
// RFID 카드 유효성 검사 함수
#define RFID_IDENTIFIER									"0B005F"
#define RFID_08C_PACKET_LENGTH 				12
#define RFID_08C_DATA_LENGTH 					10
#define RFID_08C_UID_LENGTH						 4
#define RFID_08C_CID_LENGTH						 6
#define RFID_08C_DATA_POSITION					 1
#define RFID_08C_SOF									0x02
#define RFID_08C_EOF									0x03



#define	MAX_INTER_PROTO_DATA_LENGTH				512 //
#define MIN_INTER_PROTO_DATA_LENGTH				8	// sof+ Len0 + Len1 + Mod0 + Mod1 + Seq + eof + checksum

#define	GITPACKET_SERIAL_SOF				0x02	//Start of Frame
#define	GITPACKET_SERIAL_EOF				0x03	//End of Frame
#define	GITPACKET_SERIAL_ENQ				0x05	//Enquiry
#define	GITPACKET_SERIAL_ACK				0x06	//Acknowledge
#define	GITPACKET_SERIAL_NAK				0x15	//Negative Acknowledge
#define GITPACKET_SERIAL_MODE0_LAST_PACKET 	0x80 	// Last packet

#define GITPACKET_SOF_IDX		0
#define GITPACKET_LEN_IDX		1
#define GITPACKET_MODE0_IDX		3
#define GITPACKET_MODE1_IDX		4
#define GITPACKET_SEQ_IDX		5
#define GITPACKET_PAYLOAD_IDX	6

#define GITPACKET_PAYLOAD_LEN0_IDX			0
#define GITPACKET_PAYLOAD_LEN1_IDX			1
#define GITPACKET_PAYLOAD_FUNC0_IDX			2
#define GITPACKET_PAYLOAD_FUNC1_IDX			3
#define GITPACKET_PAYLOAD_CURR_FRAME0_IDX	4
#define GITPACKET_PAYLOAD_CURR_FRAME1_IDX	5
#define GITPACKET_PAYLOAD_CHECKSUM0_IDX		6
#define GITPACKET_PAYLOAD_CHECKSUM1_IDX		7
#define GITPACKET_PAYLOAD_DATA_IDX			8
#define GITPACKET_PAYLOAD_HEADER_SIZE		8
#define GITPACKET_MODE1_DCSP		0x00



#define	MAX_MODEM_PROTO_DATA_LENGTH				1600
#define MIN_MODEM_PROTO_DATA_LENGTH				6
#define MAX_TEMP_BUFF_SIZE						512
#define MAX_GPS_BUFF_SIZE						2048
#define MAX_CAN_BUFF_SIZE						3000


#pragma pack(push, 1)

typedef __packed struct _stBT_PTCL_PAYLOAD
{
	unsigned short int 	DataLength;
	unsigned short int 	FunctionID;
	unsigned char 		*pPayload;
} stBT_PTCL_PAYLOAD;

typedef __packed struct _stBT_OTC_PAYLOAD
{
	unsigned short int DataLength;
	unsigned short int FunctionID;
	unsigned char      *pPayload;
}stBT_OTC_PAYLOAD;

typedef __packed struct _stGIT_PTCL_PAYLOAD
{
	unsigned short int 	DataLength;
	unsigned short int 	FunctionID;
	unsigned short int 	CurrentFrame;
	unsigned short int 	CheckSum;
	unsigned char 		*pPayload;
} stGIT_PTCL_PAYLOAD;

typedef __packed struct _stAMT_PTCL_PAYLOAD
{
	unsigned short int 	DataLength;
	unsigned char 		pPayload[MAX_MODEM_PROTO_DATA_LENGTH];
} stAMT_PTCL_PAYLOAD;

typedef struct _stGIT_COMM_INFO
{
	pfnReadCallBack			fnRecvData;
	pfnWriteCallBack		fnSendData;
	pfnParsingCallBack	    fnParsing;
	eCommState 				eCommSate;	// it relay on eCommType. 0 is eCOMM_TYPE_PC,1 is eCOMM_TYPE_DLC...
	void					*pstInQueue;	// In Queue Structure : GIT_Util.h
	unsigned int			uiInMaxQueueSize;
	pfnCheckReadBuff		fnCheckReadBuff;
}stGIT_COMM_INFO, stGitCommInfo;


extern stGIT_COMM_INFO g_stGitCommInfo[eCOMM_TYPE_MAX];


#pragma pack(pop,1)

void InitDCSProtocol(void);
void DeInitDCSProtocol(void);
void CommuicationManager(void);
void SendCommManager(void);
void ParsingDCSProtocol(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType);
void ParsingGITProtocol(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType);
void ParsingGITProtocol_SELFTEST(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType);
//void ParsingGITProtocol_PLUS(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType);
void ParsingCommandModeProtocol(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType);
void ParsingBypassModeProtocol(unsigned char* pBuff, unsigned int nBuffCount, eCommType eWhatCommType);
void ParsingGemaltoProtocol(unsigned char* pBuff, unsigned int nBuffCount, eCommType eWhatCommType);

#ifdef BNCOM // mod.pdh 2021.10.27
void ParsingBnComProtocol(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType);
#endif

BOOL FindDCSPacket(unsigned char* pBuff);					// Queue Structure : GIT_Util.h
BOOL FindGITPacket(unsigned char* pBuff);					// Queue Structure : GIT_Util.h
void CopyFromQueueToDCSPayload(unsigned char* pBuff, unsigned char* pInterPtcl);
void CopyFromQueueToGITPayload(unsigned char* pBuff, unsigned char* pInterPtcl, unsigned char ucMode0);

//void SendGITPtclFrame(unsigned char* bSendBuff, unsigned int uiSendLen, eCommType eWhatCommType, void* lParam, unsigned int wParam);
void SendGITPtclFrame(char* bSendBuff, unsigned int uiSendLen, eCommType eWhatCommType, void* lParam, unsigned int wParam);
void SendGITPtclResponse_DTC();
// Make GIT Protocol packet
void MakeBTPtclFrame_n_Send			(unsigned char* bSendBuff, stBT_PTCL_PAYLOAD *pSendInterPtcl, 	eCommType eWhatCommType);
void MakeAMThdlcPtclFrame_n_Send	(unsigned char* bSendBuff, stAMT_PTCL_PAYLOAD *pSendInterPtcl,	eCommType eWhatCommType);
void MakeGITPtclFrame_n_Send			(unsigned char* bSendBuff, stGIT_PTCL_PAYLOAD *pSendInterPtcl, unsigned char ucTarget, eCommType eWhatCommType);
unsigned char CalcChecksumGITPtclFromQueue(unsigned char* pBuff, unsigned int uiLength);
unsigned char CalcChecksumGITPtclFromArray(unsigned char* pBuff, unsigned int uiLength);

unsigned char CalcChecksumBTPtclFromQueue(unsigned char* pBuff, unsigned int uiLength);
unsigned char CalcChecksumBTPtclFromArray(unsigned char* pBuff, unsigned int uiLength);

// Make GIT protocol Payload packet
void MakeBTPtclPayloadFrame(stBT_PTCL_PAYLOAD *pInterPtcl, unsigned short int unFunctionID, unsigned char *pData, int nDataSize);
void MakeGITPtclPayloadFrame(stGIT_PTCL_PAYLOAD *pInterPtcl, unsigned short int unFunctionID, unsigned char *pData, int nDataSize);
unsigned short int CalcChecksumGITPtclPayloadFrame(stGIT_PTCL_PAYLOAD *pInterPtcl);
unsigned short int CalcChecksumBTPtclPayloadFrame(stBT_PTCL_PAYLOAD *pInterPtcl);
void SendGITPtclResponse(stGIT_PTCL_PAYLOAD *pInterPaylod, unsigned char* pResponseData, unsigned int uiResDataLen, eCommType eWhatCommType);
void SendBTPtclResponse(stBT_PTCL_PAYLOAD *pInterPaylod, unsigned char* pResponseData, unsigned int uiResDataLen, eCommType eWhatCommType);
bool FindRFIDPacket(unsigned char* pBuff);

#define LOCK_GET_STATE() (g_eLockStatus)

#define MACRO_BT_CERTIFICATION(x,z)  {\
    if( LOCK_GET_STATE()!= eLOCK_STATE_UNLOCK)\
    {\
		uint8_t cResult = 0x00;\
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)x, (unsigned char*)&cResult, sizeof(cResult), (eCommType)z);\
        printf("Error:Not unlock\n");\
        return;\
    }\
}

#define LOCK_SET_STATE(X) (g_eLockStatus = X)
#define BT_LOCK_GET_STATE() (g_eBTLockStatus)
#define BT_LOCK_SET_STATE(X) (g_eBTLockStatus = X)
#define GET_PTCL_STATE() (g_eSavePtclStatus)
#define SET_PTCL_STATE(X) (g_eSavePtclStatus = X)

////////////////////////////////////////////////////////////////////////////////
// QUEUE
typedef struct _stQUEUE
{
	unsigned int 	uiQueueSize;
	unsigned int 	uiFront;
	unsigned int 	uiRear;
	BYTE*		 	pData;
}stQueue;

stQueue* CreateQueue(unsigned uiQueueSize);
void DestoryQueue(stQueue* pQueue);
BOOL PushQueue(stQueue* pQueue, BYTE ucPushData, eCommType eCh);
unsigned int PushMultiDataQueue(stQueue* pQueue, BYTE* pPushData, unsigned int uiPushDataLen, eCommType eCh);
BOOL PopQueue(stQueue* pQueue,  BYTE *pPopData);
unsigned int PopMultiDataQueue(stQueue* pQueue, BYTE* pPopData, unsigned int uiPopDataLen);
BOOL IsEmptyQueue(stQueue* pQueue);
unsigned int GetQueueDataLength(stQueue* pQueue);
BOOL ClearQueue(stQueue* pQueue);
////////////////////////////////////////////////////////////////////////////////

#endif /* __GIT_INTER_PROTOCOL_H__ */

/***************************** END OF FILE ****/
