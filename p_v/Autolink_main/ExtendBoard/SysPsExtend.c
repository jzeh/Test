#if defined(FEATURE_EXTENSION_BOARD)
/**
******************************************************************************
* @file    SysPsExtend.c
* @author  WoongBae.Park
* @version V1
* @date    14-Aug-2019
* @brief   Main program body
******************************************************************************

******************************************************************************
*/

#include "HalHandler.h"
#include "SysPsExtend.h"

#include "HdDebug.h"
#include "SysPsExtendHdEvent.h"

//#include "HalHandler.h"

//#include "SysHdSwTimer.h"
//#include "CMProtocolInterface.h"
//#include "ALPsAutoLink.h"
//#include "AutolinkCapitalDefine.h"
#include "OBD_Controller.h"
#include "MngSystem.h"

#include "AutolinkConfig.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_EXTENSION,__VA_ARGS__)

#define EXTEND_BOARD_WAKEUP_MAX_TIME (3000)
#define EXTEND_BOARD_RESET_MAX_COUNT (3)

#define SignHighPass_On  0xAF
#define SignHighPass_Off  0xCD


typedef int (*fnpEventHanderList)(void* pstEventMsg);

SysPsExtendState    m_eSysExtendProcessState = STATE_SYSPS_EXTEND_INIT;
stPLUS_MODULE_STATE m_stPlusModuleState;
int8_t              cExtBDControlState = 0;
unsigned long       m_ulExtBDControTimer = 0;
stMsgExtend         m_stExtBDEvtMsg;
boolean_t           m_bActiveWattingControl = false;
uint16_t            m_usExtBDVersionBoot = 0;
uint16_t            m_usExtBDVersionApp = 0;
uint8_t             m_cHiPassCardStatus;

#pragma section="BKSRAM"
BR_EXTBDInfo BkSram_EXTBDInfo @"BKSRAM";

U8 m_uchipassPowerOn = SignHighPass_On;

extern stUserActionSetting m_stUserActionSetting;

fnpEventHanderList m_pfnSysPsExtendEventList[eMngMax] = {
        SysPsExtend_EvtNone,            // eMngNone
        SysPsExtend_EvtSysSystem,       // eMngSys
        SysPsExtend_EvtNone,            // eMngSysSub
        SysPsExtend_EvtSysSystemMsg,    // eMngSysMsg
        SysPsExtend_EvtSysSensor,       // eMngSysSensor
        SysPsExtend_EvtNone,            // eMngSysFota
        SysPsExtend_EvtSysModem,        // eMngModem
        SysPsExtend_EvtNone,            // eMngSensor
        SysPsExtend_EvtNone,            // eMngStorage
        SysPsExtend_EvtALObd,           // eMngObd
        SysPsExtend_EvtSysBluetooth,    // eMngBt
        SysPsExtend_EvtExtendBoard,     // eMngExtend
        };

extern stReportExtendBoard g_stExtendBoard;

stGIT_INTER_FUNCTIONS g_tbExtendInterPtclFunctions[] =
{
	{0xA001, ExtendBoard3secReport},
	{0xA002, ExtendBoardGetModuleStateRes},
	{0xA202, ExtendBoardGetModuleStateRes},
	{0xA006, ExtendBoardHiPassStateRes},
	{0xA007, ExtendBoardGetHipassInfoRes},
	{0xA008, ExtendBoardGetHiPassPaymentCountRes},
	{0xA009, ExtendBoardGetHipassPaymentDataRes},
	{0x1FFF, ExtendBoardModuleWakeupNoti},
	{0xA145, ExtendBoardFWInfoRes},
	{0xA175, ExtendBoardFWUpdateStartRes},
	{0xA275, ExtendBoardFWUpdateDataRes},
	{0xA375, ExtendBoardFWUpdateEndRes},
	{0xA303, ExtendBoardSerialRes},
	{0xA575, ExtendBoardSleepRes},
	{0xA00A, ExtendBoardHiPassPowerOnRes},				//CIAIÆÐ½ºÆA¿oON
	{0xA00B, ExtendBoardHiPassPowerOffRes},				//CIAIÆÐ½ºÆA¿oOFF
	{0xC001, ExtendBoardFobKeyPowerControlResultRes},	//Fob ¼³A¤ Result
	{0xC002, ExtendBoardFobKeyPowerStatusRes},			//Fob ¼³A¤ Result
	{0x5201, ExtendBoardGetRFIDInfoRes}
};

void SetAntiThiefFlagSetting(bool bAntiThiefFlag)
{
	m_stUserActionSetting.bAntithiefFlag = bAntiThiefFlag;

    // save setting value 
    WriteConfig(false,true);
}

bool GetAntiThiefFlagSetting()
{
	return m_stUserActionSetting.bAntithiefFlag;
}

void SysPsExtensionInit()
{
	// initialize all process.

	// wake up extension
	SetExtendBoardModuleWakeup();
}

void SaveHiPassCardStatus(uint8_t cHiPassCardStatus)
{
	m_cHiPassCardStatus = cHiPassCardStatus;
}

uint8_t GetHiPassCardStatus()
{
	return m_cHiPassCardStatus;
}


bool IsEngOn()
{
	if(Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN && Get_VehicleStatus() <= eVEHICLE_STATE_EV_ON)
	{
		return true;
	}

	return false;
}

void HandlerHipassOnOff()
{
	static unsigned long s_ulHiPassTimer = 0;
	static uint8_t s_ucHipassOnReqCnt=0, s_ucHipassOffReqCnt=0;
	if(Get_Tmr() - s_ulHiPassTimer > 1500)
	{

		if(IsEngOn() == true)
		{
			if(s_ucHipassOnReqCnt<5)
			{
				ExtendBoardHiPassPowerOnReq();
				s_ucHipassOnReqCnt++;
			}

			s_ucHipassOffReqCnt=0;
		}

		if(IsEngOn() == false)
		{
			if( s_ucHipassOffReqCnt < 5 )
			{
				ExtendBoardHiPassPowerOffReq();
				s_ucHipassOffReqCnt++;
			}

			s_ucHipassOnReqCnt=0;
		}

		s_ulHiPassTimer = Get_Tmr();
	}
}

void SetExtBDVersionInfo(uint8_t * ucExtBDVersion)
{
	memset(&BkSram_EXTBDInfo, 0x00, sizeof(BkSram_EXTBDInfo));

	BkSram_EXTBDInfo.usEXTBDVersionBoot=ucExtBDVersion[2];
	BkSram_EXTBDInfo.usEXTBDVersionBoot=BkSram_EXTBDInfo.usEXTBDVersionBoot|ucExtBDVersion[3]<<8;
	BkSram_EXTBDInfo.usEXTBDVersionApp=ucExtBDVersion[4];
	BkSram_EXTBDInfo.usEXTBDVersionApp=BkSram_EXTBDInfo.usEXTBDVersionApp|ucExtBDVersion[5]<<8;

	if( AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON )
	{
		Trace("--- SendToDeviceVersion !!!! ---\n");
		SendToDeviceVersion();
	}
}

void GetExtDBVersionInfo(stExtBDVersionInfo * pstExtBDFWFileInfo)
{
	memcpy(&(pstExtBDFWFileInfo->usExtVersionBoot), &BkSram_EXTBDInfo.usEXTBDVersionBoot, sizeof(BkSram_EXTBDInfo.usEXTBDVersionBoot));
	memcpy(&(pstExtBDFWFileInfo->usExtVersionApp), &BkSram_EXTBDInfo.usEXTBDVersionApp, sizeof(BkSram_EXTBDInfo.usEXTBDVersionApp));

#if 0
	Trace("\n---------------------------------------------\n");
	Trace("ExtBoot Version : %02x, ExtApp Version : %02x", m_usExtBDVersionBoot, m_usExtBDVersionApp);
	Trace("\n---------------------------------------------\n");
#endif
}

void SetExtBDControlState(int8_t cState)
{
	cExtBDControlState = cState;
	m_bActiveWattingControl = true;
}

int8_t GetExtBDControlState()
{
	return cExtBDControlState;
}

void SaveControlEvtMsg(void* pstEventMsg)
{
	Trace("Save Msg and start timer\n");

	memcpy(&m_stExtBDEvtMsg, pstEventMsg, sizeof(stMsgExtend));
	m_ulExtBDControTimer = Get_Tmr();
}

void GetSavedControlEvtMsg(void* pstEventMsg)
{
	Trace("Get saved Msg\n");

	memcpy(pstEventMsg, &m_stExtBDEvtMsg, sizeof(stMsgExtend));
}

void SendExtendBoardSysPsState(int8_t cNewState)
{
	static int8_t s_cBeforeState = STATE_SYSPS_EXTEND_INIT;
	stMsgExtend stReport;
	g_stExtendBoard.ucExtendBoardState = cNewState;

	if(s_cBeforeState != cNewState)
	{
		s_cBeforeState = cNewState;
	}
}

void SetSysPsExtendStatus(SysPsExtendState eState)
{
	m_eSysExtendProcessState = eState;

	SendExtendBoardSysPsState(GetSysPsExtendStatus());
}

SysPsExtendState GetSysPsExtendStatus()
{
	return m_eSysExtendProcessState;
}

void SysPsExtendEventHandler(stMsgExtend* pstReport)
{
    if(pstReport->header.id < eMngMax)
    {
printf("SysPsExtendEventHandler : id %d, event 0x%04X ~~~~~~~\r\n",pstReport->header.id, pstReport->header.event);
        
        m_pfnSysPsExtendEventList[pstReport->header.id]((void*)pstReport);
    }
    else
    {
        Trace("ALPsEventHandler:id mismatch\n");
    }
}

void SysExtendBoardProcess()
{

	stMsgExtend stMessage;
	static int8_t s_tStartFlag = false;

	if(s_tStartFlag == false)
	{
		SetExtendBoardPayloadMalloc();
		s_tStartFlag = true;
	}

    // check external event
#ifndef GLOBAL_SHARE_QUEUE //Get
    if( MngQueueGetMessage(ID_MNG_QUEUE_EXTEND,(int8_t*)&stMessage, sizeof(stMsgExtend)) == true )
#else
	if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_EXTEND, (uint8_t*)&stMessage.header, sizeof(stMsgExtend), (uint8_t*)&stMessage.carReport, sizeof(stCarReport)) == true )
#endif
	{
/*
	      Trace("[%s] event : %x, subEvent : %x, from : %x, reuslt : %x\n", __FUNCTION__,
	      stMessage.header.event,
	      stMessage.header.subEvent,
	      stMessage.header.unTracePs,
	      stMessage.header.result);
*/
        // handle external event
        SysPsExtendEventHandler(&stMessage);
    }

	switch (m_eSysExtendProcessState)
	{
		case STATE_SYSPS_EXTEND_INIT :

			static unsigned long s_ulExtendBoardWakeupTimer = 0;
			static unsigned long s_ulExtendBoardResetCnt = 0;

			if((Get_Tmr()-s_ulExtendBoardWakeupTimer)>EXTEND_BOARD_WAKEUP_MAX_TIME)
			{
				SetExtendBoardModuleWakeup();
				s_ulExtendBoardWakeupTimer=Get_Tmr();

				s_ulExtendBoardResetCnt++;

				Trace("Extned Board Not wakeup. Retry request Extboard wakeup\r\n");
			}

			if(s_ulExtendBoardResetCnt > EXTEND_BOARD_RESET_MAX_COUNT)
			{
				SetSysPsExtendStatus(STATE_SYSPS_EXTEND_ERROR);
			}

			LoadConfigHipassPowerValue();

			break;

		case STATE_SYSPS_EXTEND_RUN :

			break;

		case STATE_SYSPS_EXTEND_SLEEP :

			break;

		case STATE_SYSPS_EXTEND_IDLE :

			unsigned long ulExtBDControDeltaTimer = 0;

			ulExtBDControDeltaTimer = Get_Tmr() - m_ulExtBDControTimer;
			if(m_bActiveWattingControl == true)
			{
				if( GetExtBDControlState() == eCONTROL_WATTING )//&& ulExtBDControDeltaTimer > 5000 )
				{
					GetSavedControlEvtMsg(&stMessage);
					SetExtBDControlState(eCONTROL_IDLE);

					HandlerSaveMsg(stMessage);
					//Send2SysExtend(eMngModem, eExtend_ReqControl, stMessage.header.subEvent, stMessage.header.result, (stCarReport*)&stMessage.carReport, stMessage.header.unTracePs);
					Trace("Send 2 ExtBD saved command\n");
					m_bActiveWattingControl = false;
				}
//					else if( ulExtBDControDeltaTimer > 5000)
//					{
//						memset(m_pstExtBDEvtMsg,0x00, sizeof(m_pstExtBDEvtMsg));
//						m_ulExtBDControTimer = 0;
//						m_bActiveWattingControl = false;
//						Trace("Save Msg Timeout \n");
//					}
			}
#if 0 //  not used

			if(GetHighPassPower() != SignHighPass_Off)
			{
				HandlerHipassOnOff();
			}
#endif
			break;

		case STATE_SYSPS_EXTEND_FOTA :

			break;

		case STATE_SYSPS_EXTEND_RESET :

			static unsigned long s_ulExtendBoardWakeupTimer2 = 0;
			static unsigned long s_ulExtendBoardResetCnt2 = 0;

			if((Get_Tmr()-s_ulExtendBoardWakeupTimer2)>EXTEND_BOARD_WAKEUP_MAX_TIME)
			{
				SetExtendBoardModuleWakeup();
				s_ulExtendBoardWakeupTimer2=Get_Tmr();

				s_ulExtendBoardResetCnt2++;

				Trace("Extned Board Not wakeup. Retry request Extboard wakeup\r\n");
			}

			if(s_ulExtendBoardResetCnt2 == EXTEND_BOARD_RESET_MAX_COUNT)
			{
				SetSysPsExtendStatus(STATE_SYSPS_EXTEND_ERROR);
			}

			break;

		case STATE_SYSPS_EXTEND_ERROR :

			// 에러이면 리셋 핀을 몇 번을 흔들어 주고 그래도 안되면 복구 불가로 설정해야 함
			static int8_t s_cExtBDResetCnt = 0;

			if(s_cExtBDResetCnt == 0)
			{
				// 여기에서는 리셋핀을 흔들어야 하는지 확인 필요.
				SetExtendBoardModuleWakeup();
			}

			if(s_cExtBDResetCnt == 3 )
			{
				SetSysPsExtendStatus(STATE_SYSPS_EXTEND_CANNOTRECOVERY);
			}
			else
			{
				SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RESET);
			}

			s_cExtBDResetCnt++;

			break;

		case STATE_SYSPS_EXTEND_CANNOTRECOVERY :
			// 복구 불가 상태 전달
			break;
	}
}

void HandlerSaveMsg(stMsgExtend stEventMsg)
{
	uint8_t ucControlType;
	stMsgExtend stMessage = stEventMsg;

	ucControlType = stMessage.header.subEvent;

	switch(ucControlType)
	{
		case eExtBD_DoorCloseReq:
			LockUnlockSet(eExtBD_DoorCloseReq);
			break;
		case eExtBD_DoorOpenReq:
			LockUnlockSet(eExtBD_DoorOpenReq);
			break;
		case eExtBD_lampOnReq:
			LampSet(eExtBD_lampOnReq, 5000);
			break;
		case eExtBD_hornOnReq:
			HornOn(5000, 5000);
			break;
		case eExtBD_SetAntiThiefReq:
			ExtendBoardFobKeyPowerControlReq(stMessage.buffer[0]);
			break;
		case eExtBD_FobKeyPowerStatusReq:
			ExtendBoardFobKeyStatusReq();
			break;
		default:
			Send2SysExtend(eMngModem, eExtend_ReqControl, stMessage.header.subEvent, stMessage.header.result, 
			                &stMessage, stMessage.header.unTraceMng);
			break;
	}
}

void LoadConfigHipassPowerValue()
{
#if 0 //  not used
	stUtilitySet stHighPassOnOff;
	memset(&stHighPassOnOff,NULL,sizeof(stUtilitySet));
	GetCapitalConfigProperty(eCapitalConfig_EnableHighPass,(void*)&stHighPassOnOff);
	SetHighPassPower(stHighPassOnOff.ucUseHighPass);
#endif
}

void SetHighPassPower(U8 ucIsPowerOn)
{
	m_uchipassPowerOn = ucIsPowerOn;
}

U8 GetHighPassPower()
{
	return m_uchipassPowerOn;
}

void ProcessExtendInterProtocol(stGIT_PTCL_PAYLOAD *pInterPtcl, eCommType eInCommType)
{
	unsigned int i, uiLoopCnt;

	uiLoopCnt = sizeof(g_tbExtendInterPtclFunctions) / sizeof(g_tbExtendInterPtclFunctions[0]);
    printf("!!!Define Input Function ID (ProcessExtendInterProtocol) : 0x%04X !!!\r\n", pInterPtcl->FunctionID);

	for(i = 0; i < uiLoopCnt; i++) {
		if((pInterPtcl->FunctionID == g_tbExtendInterPtclFunctions[i].uiFunctionID) && (g_tbExtendInterPtclFunctions[i].fnPayloadCB != NULL)) {
			g_tbExtendInterPtclFunctions[i].fnPayloadCB(pInterPtcl, eInCommType);
			return;
		}
	}

	if ( i >= uiLoopCnt )
		Trace("!!!not Define Input Function ID (ProcessExtendInterProtocol) : 0x%04X !!!\r\n", pInterPtcl->FunctionID);
}


void ExtendBoard3secReport(void *pInterPtcl, unsigned int eInCommType)
{
	static boolean_t s_bOnetimeFlag = false;

	if(s_bOnetimeFlag == false)
	{
		Send2SysExtend(eMngExtend, eExtend_RspReport, eExtBD_ThreeSecReport, NULL, NULL, eMngExtend);
		s_bOnetimeFlag = true;
	}
}

void ExtendBoardModuleWakeupNoti(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	Trace("Get ExtendBoardModuleWakeupState\n");
	Trace("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

	Send2SysExtend(eMngExtend, eExtend_RspControl, eExtBD_WakeupRes, NULL, NULL, eMngExtend);
}

void ExtendBoardFWUpdateStartRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardFWUpdateStartRes\n");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspFota, eExtBD_FWUpdateStartRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardFWUpdateDataRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardFWUpdateDataRes\n");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	stSendFileStatus stSendFileStatus;
	GetFOTASendBinCnt(&stSendFileStatus);

	if(stSendFileStatus.usSendCnt > stSendFileStatus.usLoopTotalCnt)
	{
		Send2SysExtend(eMngExtend, eExtend_RspFota, eExtBD_FWUpdateEndReq, NULL, &stExtBoardMsg, eMngExtend);
	}
	else
	{
		Send2SysExtend(eMngExtend, eExtend_RspFota, eExtBD_FWUpdateRes, NULL, &stExtBoardMsg, eMngExtend);
	}

}

void ExtendBoardFWUpdateEndRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardFWUpdateEndRes\n");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspFota, eExtBD_FWUpdateEndRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardSerialRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardSerialRes\n");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspReport, eExtBD_SerialRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardSleepRes(void *pInterPtcl, unsigned int eInCommType)
{
    Trace("ExtendBoardSleepRes\n");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspControl, eExtBD_SleepRes, NULL, &stExtBoardMsg, eMngExtend);
}
void ExtendBoardFWInfoRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardFWInfoRes\n");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;

    if ( pstPayloadPtcl->DataLength > 0)
    	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload,pstPayloadPtcl->DataLength);
    else
        stExtBoardMsg.buffer[0] = NULL;
	Send2SysExtend(eMngExtend, eExtend_RspReport, eExtBD_VersionInfoRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardGetHipassInfoRes(void *pInterPtcl, unsigned int eInCommType)
{
    Trace("ExtendBoardGetHipassInfoRes\n");

    stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspReport, eExtBD_HiPassInfoRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardHiPassStateRes(void *pInterPtcl, unsigned int eInCommType)
{
#if 0 //  not used

	Trace("ExtendBoardHiPassStateRes\n");

	int uiTollFareIdx1 = 0,uiTollFareIdx2 = 0,uiTollFareIdx3 = 0;
	double	 udTollFare = 0;

	stDrvingHighPass stReceiveHighPassData;
	stMsgExtend stExtBoardMsg;

	memset(&stReceiveHighPassData,NULL,sizeof(stDrvingHighPass));

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;

	memcpy(&stReceiveHighPassData, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	if(stReceiveHighPassData.m_ucAttriteID == HISSPASS_ATT_EVENT)
	{
		uiTollFareIdx1 = stReceiveHighPassData.m_ucCharge_1Step[2];
		uiTollFareIdx2 = stReceiveHighPassData.m_ucCharge_1Step[1];
		uiTollFareIdx3 = stReceiveHighPassData.m_ucCharge_1Step[0];

		udTollFare = uiTollFareIdx1  + (uiTollFareIdx2<<8 ) + (uiTollFareIdx3<<16 );

		//if(udTollFare >= HISSPASS_FARE_MAX && Get_TmrDelta(Get_Tmr(), g_iAcc1OverTime) < HIGH_ACCON_TIME)
		if(udTollFare >= HISSPASS_FARE_MAX)
		{
			//¹≪½AA³¸®
		}
		else if(udTollFare > 0)
		{
			Send2SysExtend(eMngExtend, eExtend_RspReport, eExtBD_HiPassStateRes, NULL, &stExtBoardMsg, eMngExtend);
		}
	}
	else if(stReceiveHighPassData.m_ucAttriteID == HISSPASS_ATT_BOOT)
	{
		Send2SysExtend(eMngExtend, eExtend_RspReport, eExtBD_HiPassInfoRes, NULL, &stExtBoardMsg, eMngExtend);
	}
#endif
}

void ExtendBoardGetHiPassPaymentCountRes(void *pInterPtcl, unsigned int eInCommType)
{
    Trace("ExtendBoardGetHiPassPaymentCountRes\n");
    
	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspReport, eExtBD_HiPassPaymenetCntRes, NULL, &stExtBoardMsg, eMngExtend);
}
void ExtendBoardGetHipassPaymentDataRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardGetHiPassPaymentCountRes\n");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspReport, eExtBD_HiPassPaymenetDataRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardGetModuleStateRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("A002");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspControl, eExtBD_FobKeyControlRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardHiPassPowerOnRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardHiPassPowerOnRes");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspControl, eExtBD_HiPassPowerOnRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardHiPassPowerOffRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardHiPassPowerOffRes");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspControl, eExtBD_HiPassPowerOffRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardFobKeyPowerControlResultRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardFobKeyPowerControlResultRes");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspControl, eExtBD_SetAntiThiefRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardFobKeyPowerStatusRes(void *pInterPtcl, unsigned int eInCommType)
{
	Trace("ExtendBoardFobKeyPowerStatusRes");

	stGIT_PTCL_PAYLOAD *pstPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stMsgExtend stExtBoardMsg;
	memcpy(stExtBoardMsg.buffer, pstPayloadPtcl->pPayload, pstPayloadPtcl->DataLength);

	Send2SysExtend(eMngExtend, eExtend_RspControl, eExtBD_FobKeyPowerStatusRes, NULL, &stExtBoardMsg, eMngExtend);
}

void ExtendBoardGetRFIDInfoRes(void *pInterPtcl, unsigned int eInCommType)
{
#if 0 //  not used
//		stMsgExtend stExtBoardMsg;
//		memcpy(stExtBoardMsg.buffer, pInterPtcl,sizeof(pInterPtcl));

	//Send2SysExtend(eMngExtend, eExtend_RspControl, eExtBD_FobKeyPowerStatusRes, NULL, &stExtBoardMsg, eMngExtend);

	Trace("\n-----------------------------------------\n");
	Trace("RFID INFO RES !!!!");
	Trace("\n-----------------------------------------\n");

	bool bFindReserveInfo = false;
	static int siGetTaggingTime = 0;
	stRFID_CARD_INFO_RSP_Stype g_stRFIDCardInfo;
	stSend_RFID_CARD_INFO	   g_stSend_RFID_CARD_INFO;

	stReservInfo stReservationInfo[RESERVED_INDEX_MAX];
	stMasterInfo stMasterKeyInfo[MAX_MASTER_ID];
	U8	ucTempNullCheck[USER_ID_LENGTH] = {0,};

	memset(&stReservationInfo,NULL,sizeof(stReservInfo)*RESERVED_INDEX_MAX);
	GetCapitalConfigProperty(eCapitalConfig_Reservation,(void*)&stReservationInfo);

	memset(&stMasterKeyInfo,NULL,sizeof(stMasterInfo)*MAX_MASTER_ID);
	GetCapitalConfigProperty(eCapitalConfig_MasterKey,(void*)&stMasterKeyInfo);

	if(Get_TmrDelta(Get_Tmr(), siGetTaggingTime) < (3 *1000) && siGetTaggingTime != 0 )
	{
		return;
	}

	siGetTaggingTime = Get_Tmr();
	stGIT_PTCL_PAYLOAD *pPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	memcpy(&g_stRFIDCardInfo, (stRFID_CARD_INFO_RSP_Stype *)pPayloadPtcl->pPayload, sizeof(g_stRFIDCardInfo));

	for(int i=0; i<g_stRFIDCardInfo.ucCardNumSZ; i++)
	{
		g_stSend_RFID_CARD_INFO.arrCardNumData[i*2] = (g_stRFIDCardInfo.arrCardNumData[i]>>4)+0x30; // 18 = card number index
		g_stSend_RFID_CARD_INFO.arrCardNumData[(i*2)+1] = (g_stRFIDCardInfo.arrCardNumData[i]&0x0F)+0x30;
		Trace("%x%x", g_stSend_RFID_CARD_INFO.arrCardNumData[i*2], g_stSend_RFID_CARD_INFO.arrCardNumData[(i*2)+1]);
	}

	Trace("\r\n");

    GetTimeCapitalReportTIme((char *)&g_stSend_RFID_CARD_INFO.arrDoorUnlockTime[0]);

	g_stSend_RFID_CARD_INFO.ucTransSZ = g_stRFIDCardInfo.ucTransSZ;
	memcpy(g_stSend_RFID_CARD_INFO.arrTransData, g_stRFIDCardInfo.arrTransData, g_stRFIDCardInfo.ucTransSZ );

	//////////////////////////////////////////////////////////////////////
	//Reservation Info
	//////////////////////////////////////////////////////////////////////
	for(int i=0; i< RESERVED_INDEX_MAX; i++)
	{
		if(memcmp(g_stSend_RFID_CARD_INFO.arrCardNumData,stReservationInfo[i].arrUserID, 16) == 0 && stReservationInfo[i].ucUsed == true)
		{
			bFindReserveInfo = true;
			Trace(" Find Reservation Info Index : %d \n",i);
			break;
		}
	}

	//////////////////////////////////////////////////////////////////////
	//MasterKey Search
	//////////////////////////////////////////////////////////////////////

	if(bFindReserveInfo == false)
	{
		for(int i=0; i< MAX_MASTER_ID; i++)
		{
			if(memcmp(g_stSend_RFID_CARD_INFO.arrCardNumData,&stMasterKeyInfo[i].arrUserID, USER_ID_LENGTH) == 0)
			{
				if(memcmp(g_stSend_RFID_CARD_INFO.arrCardNumData,ucTempNullCheck,USER_ID_LENGTH) != 0)
				{
					bFindReserveInfo = true;
					Trace(" Find M Info Index : %d \n",i);
					break;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////
	Trace(" Test Code Operation \n");
	//bFindReserveInfo = true;
	//////////////////////////////////////////////////////////////////////

	if(bFindReserveInfo)
	{
		if(m_stPsObdCtrl.stFastBootMode.bRcvRemoteControl == false )
		{
			if(((BkSram_ObdInfo.stStatus.ucPreDoorLock&MASK_DOOR_LOCK)>>4) == 0)
			{
				//UnLock
				Trace("----Unlock Process ---\n");

				Send2SysExtend(eMngModem, eExtend_ReqControl, eExtBD_DoorOpenReq, NULL, NULL, eMngModem);
				Send2AppObd(eMngModem, eAPP_ReqSmartKey, eRC_SmartKey, NULL, NULL, eMngModem);
			}
			else
			{
				//Lock
				Trace("----lock Process ---\n");

				Send2SysExtend(eMngModem, eExtend_ReqControl, eExtBD_DoorCloseReq, NULL, NULL, eMngModem);
				Send2AppObd(eMngModem, eAPP_ReqSmartKey, eRC_SmartKey, NULL, NULL, eMngModem);
			}
		}
		else
		{
			//UnLock
			Trace("---- RFid  m_stPsObdCtrl.stFastBootMode.bRcvRemoteControl is false ---\n");

			if(((m_stAutolinkMonitorCtrl.stBkStatus.ucPreDoorLock&MASK_DOOR_LOCK)>>4) == 0)
			{
				//UnLock
				Trace("----Unlock Process ---\n");

				Send2SysExtend(eMngModem, eExtend_ReqControl, eExtBD_DoorOpenReq, NULL, NULL, eMngModem);
				Send2AppObd(eMngModem, eAPP_ReqSmartKey, eRC_SmartKey, NULL, NULL, eMngModem);
			}
			else
			{
				//Lock
				Trace("----lock Process ---\n");

				Send2SysExtend(eMngModem, eExtend_ReqControl, eExtBD_DoorCloseReq, NULL, NULL, eMngModem);
				Send2AppObd(eMngModem, eAPP_ReqSmartKey, eRC_SmartKey, NULL, NULL, eMngModem);
			}
		}
	}
#endif
}


void SysExtendBoardCheckAntiThief(boolean_t bAntiThiefFlag, boolean_t bFOBOnStatus)
{
    if ( (bAntiThiefFlag == TRUE)  )
    {
        Trace("[%s] AntiThief is true, FOB Power has to be set off \r\n", __FUNCTION__);
        if ( bFOBOnStatus == TRUE )
        {
            unsigned char ucFobKeyPowerControl = 1; // FOB Off;
            ExtendBoardFobKeyPowerControlReq(ucFobKeyPowerControl);
        	SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RUN); 
			SetExtBoardControlCallbackStart();
		}
		else
		{
			SetSysPsExtendStatus(STATE_SYSPS_EXTEND_IDLE);
		}
    }
    else
    {
    	SetSysPsExtendStatus(STATE_SYSPS_EXTEND_IDLE);
    }
}

#endif //#if defined(FEATURE_EXTENSION_BOARD)