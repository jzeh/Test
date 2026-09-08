#ifndef __SYS_PS_EXTEND_HD_CONTROL_H__
#define __SYS_PS_EXTEND_HD_CONTROL_H__

#if defined(FEATURE_EXTENSION_BOARD)

#include "Autolink_Manager.h"
#include "common.h"

#define MAX_SIZE_DB_NAME (6)
#define MAX_SIZE_DOWNLOADING_BUFF (500)

typedef __packed struct _stPLUS_MODULE_CTRL{
	int8_t		m_ucEmergencyLamp_Ctrl;			/* Emergency Lamp 제어 (GPIO) */
	uint32_t	m_nEmergencyLamp_Time;		/* Emergency Lamp 동작시간 */
	int8_t 		m_ucLock_Ctrl;							/* Lock/UnLock제어 */
	int8_t 		m_ucLock_Select;						/* Lock/UnLock선택*/		// lock =0, unlock =1
	int8_t 		m_ucHorn_Ctrl;							/* Horn 제어 (GPIO) */
	uint32_t	m_nHorn_Time;							/* Horn 제어 시간 */
	uint32_t	m_nHorn_Gap;							/* Horn 울림 간격 */
	int8_t		m_ucTrunk_Ctrl; 					/* Trunk 제어 */
	int8_t		m_ucTrunk_Select; 					/* Trunk 제어 */
}stPLUS_MODULE_CTRL;

typedef __packed struct _stDownloadingFile
{
	INT16U Frame_Size;				/* 전송하는 Frame Size */
	INT8U DB_Data[MAX_SIZE_DOWNLOADING_BUFF];	/* 최대 500byte */
}stDownloadingFile;

typedef __packed struct _stExtBDFWFileInfo
{
	uint8_t ucDB_Name[MAX_SIZE_DB_NAME];/* 모듈에 저당해야할 db name */
	uint32_t uiDB_Size;					/*DB의 사이즈 정보 */
	uint16_t usDB_Ver;					// DB 버전 또는 F/W버전 정보를 전달
	uint16_t usCheckSum; 				// 전달할 db  또는 f/w의 checksum (파일 사이즈에 대한 모든 byte의 합)
	uint32_t uiFileStartAddress;        // file 저장 시작 주소
	uint32_t uiFileEndAddress;	        // file 저장 종료 주소
}stExtBDFWFileInfo;

typedef __packed struct _stSendFileStatus
{
	uint16_t usSendCnt; 		// 전송한 업데이트 패킷의 수
	uint16_t usFileSize;		// 전송할 File size
	uint16_t usLoopTotalCnt;	// 전송 회수
	uint16_t usLeftByte;  		// 전송한 byte
	uint32_t uiFileStartAddress;// file 저장 시작 주소
	uint32_t uiFileEndAddress;	// file 저장 종료 주소
}stSendFileStatus;

enum {
	eExtendUnlock=0,
	eExtendLock
};


#define LOCK_SET	1
#define LAMP_SET	1
#define HORN_SET	1
#define TRUNK_SET	1


void ExtendBoardModuleControlReq(uint16_t usFunctionID, unsigned char * ucPayload, unsigned char ucDataLength);
void SetExtendBoardPayloadMalloc(void);

void ExtendBoardSerialReq(void);
void ExtendBoardFWInfoReq(void);
void ExtendBoardHiPassPaymentCntReq(void);
void ExtendBoardHiPassPaymentDataReq(void);
void ExtendBoardHiPassPowerOnReq(void);
void ExtendBoardHiPassPowerOffReq(void);
void ExtendBoardFobKeyStatusReq();
void ExtendBoardFotaStartReq();
void GetFOTASendBinCnt(stSendFileStatus *pstSendFileStatus);

#endif //#if defined(FEATURE_EXTENSION_BOARD)

#endif //__SYS_PS_EXTEND_HD_CONTROL_H__