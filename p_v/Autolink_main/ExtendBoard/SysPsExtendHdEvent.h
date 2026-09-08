#ifndef __SYS_PS_EXTEND_HD_EVENT_H__
#define __SYS_PS_EXTEND_HD_EVENT_H__


#if defined(FEATURE_EXTENSION_BOARD)
#include "Share_InterFunction.h"
#include "SysPsExtendHdControl.h"

/****************************************************/
// Sub Event List Related with eExtend_ReqControl/eExtend_RspControl
/****************************************************/
enum {
	eExtBD_None = 0,
	eExtBD_WakeupReq,
	eExtBD_WakeupRes,
	eExtBD_SerialReq,
	eExtBD_SerialRes,
	eExtBD_VersionInfoReq,
	eExtBD_VersionInfoRes,
	eExtBD_FWUpdateStartReq,
	eExtBD_FWUpdateStartRes,
	eExtBD_FWUpdateReq,
	eExtBD_FWUpdateRes,
	eExtBD_FWUpdateEndReq,
	eExtBD_FWUpdateEndRes,
	eExtBD_DeviceRestReq,
	eExtBD_SleepReq,
	eExtBD_SleepRes,
	eExtBD_DoorCloseReq,
	eExtBD_DoorOpenReq,
	eExtBD_lampOnReq,
	eExtBD_lampOffReq,
	eExtBD_hornOnReq,
	eExtBD_FobKeyControlReq,
	eExtBD_FobKeyControlRes,
//	eExtBD_HiPassStateReq,
	eExtBD_HiPassStateRes, // 0xA006
//	eExtBD_HiPassInfoReq,
	eExtBD_HiPassInfoRes,  // 0xA007
	eExtBD_HiPassPaymenetCntReq,
	eExtBD_HiPassPaymenetCntRes, // 0xA008
	eExtBD_HiPassPaymenetDataReq,
	eExtBD_HiPassPaymenetDataRes,// 0xA009
	eExtBD_HiPassPowerOnReq,
	eExtBD_HiPassPowerOnRes,	// 0xA00A
	eExtBD_HiPassPowerOffReq,
	eExtBD_HiPassPowerOffRes,	// 0xA00B
	eExtBD_SetAntiThiefReq,
	eExtBD_SetAntiThiefRes,
	eExtBD_FobKeyPowerStatusReq,
	eExtBD_FobKeyPowerStatusRes,
	eExtBD_ThreeSecReport,
	eExtBD_LoadingHipassConfig,
	eExtBD_Max
};


typedef __packed struct _stDrvingHighPass
{
	U8	m_ucAttriteID;
	U8	m_ucCardState;						//0x00:정상	0x01:카드없음	0x02:카드오삽입		0x03:비인가카드		0x04:카드오류	0x05:잔액부족	0x06:할인카드	0x07:면제카드	0x08:후불카드	0x09:후불할인카드	0x60:자동충전카드 0x66:자동충전할인카드
	U8 	m_ucHiPassOpcode;					//0x55 고정
	U8 	m_ucCardNum[8];						//0x00 0x20 0x01 0x00 0x00 0x02 0x95 0x70
	U16 m_usNowTollGate;					//0x10 0x21
	U16 m_usPrevTollGate;					//0x10 0x21
	U8	m_ucChargeState;
	U8	m_ucCharge_1Step[3];
	U8	m_ucCharge_2Step[3];
	U8 	m_ucCarLineType;
	U8 	m_arrDate[4];						//0x20 0x16 0x12 0x13	2016/12/13
	U8 	m_arrTime[3];						//0x20 0x20 0x42	pm 08:20:42
	U8 	m_ucProcResult;						//0x20 0x20 0x42	pm 08:20:42
	U8  m_ucOperAgency;
}stDrvingHighPass;

typedef __packed struct _stReportExtendBoard
{
	uint8_t	ucExtendBoardState;
	uint8_t ucHipassWakeupState;
	uint8_t ucControlFailResult;
  	uint8_t ucFOBOnStatus;   // 0:off, 1:On
  	uint8_t ucAntiThiefFlag;
}stReportExtendBoard;


void SetExtendBoardModuleSleep(void);
void SetExtendBoardModuleWakeup(void);
void PlusModuleSetDataInit();
void LockUnlockSet(uint8_t ucSelect);
void LampSet(uint8_t ucSelect,uint8_t ucTime);
void HornOn(uint32_t uiTime, uint32_t uiGap);


void GetFOTASendBinCnt(stSendFileStatus *pstSendFileStatus);

void SendToFobKeyStatus(uint8_t ucFobPowerStatus);
void SendToDeviceVersion();


#endif // FEATURE_EXTENSION_BOARD


#endif //__SYS_PS_EXTEND_HD_EVENT_H__

