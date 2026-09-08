#if defined(FEATURE_EXTENSION_BOARD)

/**
******************************************************************************
* @file    SysPsExtendHdEvent.c
* @author  WoongBae.Park
* @version V1
* @date    14-Aug-2019
* @brief   Main program body
******************************************************************************

******************************************************************************
*/


//#include "SysHdQueue.h"
//#include "SysEventList.h"
#include "SysPsExtend.h"
//#include "ProductionUnitTest.h"
//#include "SysHdSwTimer.h"

#include "SysPsExtendHdControl.h"
#include "SysPsExtendHdEvent.h"
//#include "SysPsModemGpsTrackerCapitalEmulator.h"
//#include "AutolinkCapitalDefine.h"
//#include "SysPsModem.h"
#include "HdDebug.h"
#include "Share_InterFunction.h"


#define Trace(...)  GITDebug(DEBUG_MODULES_EXTENSION,__VA_ARGS__)

#define EXTBD_CONTROL_TIMEOUT (2000)

unsigned long ulExtBoardControlTimer = 0;
int32_t m_iTimerExtBDTimeOutCallback = -1;
boolean_t m_bExtBDControlTimeoutFlag = false;

extern void SetSysPsExtendStatus(SysPsExtendState eState);
extern SysPsExtendState GetSysPsExtendStatus();

#if defined(FEATURE_EXTENSION_BOARD)
/* ASSAGAOY : CarReport의 동작성을 해칠가능성 -> 제거하는 것이 좋을듯 -> 일단을 글로벌로*/
// AutolinkMessage.h파일 수정 
stReportExtendBoard g_stExtendBoard;
#endif

boolean_t IsExtboardControlCar(void* pstEventMsg);

void SetExtBoardControlCallbackStart();
boolean_t IsExtBoardControlTimeout();
void InitExtBDControlTimoutValue();
void ExtBDControlTimoutCallback();

void SetExtBoardControlCallbackStart()
{
	m_iTimerExtBDTimeOutCallback=HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, ExtBDControlTimoutCallback, TRUE);

//		ulExtBoardControlTimer = Get_Tmr();
}

void ExtBDControlTimoutCallback()
{
	m_bExtBDControlTimeoutFlag = true;
}

void InitExtBDControlTimoutValue()
{
	HalTimerClearSWTimer(m_iTimerExtBDTimeOutCallback);
	m_iTimerExtBDTimeOutCallback = -1;
}

boolean_t IsExtBoardControlTimeout()
{
//		boolean_t bResult = false;
//
//		if(Get_Tmr() - ulExtBoardControlTimer > EXTBD_CONTROL_TIMEOUT)
//		{
//			bResult = true;
//			//Send2AppAutolink(uint16_t usId, int32_t nEvent, int32_t nSubEvent, boolean_t bResult, stCarReport * pstReport, uint32_t unTracePs)
//			// 제어결과가 timeout 이기 때문에 시스템에 전달
//			SetSysPsExtendStatus(STATE_SYSPS_EXTEND_IDLE);
//		}
//
//		return bResult;
	return m_bExtBDControlTimeoutFlag;
}

boolean_t IsExtboardControlCar(void* pstEventMsg)
{
	stCarReport stReport;
	boolean_t bResult = true;
	stMsgExtend* pstMsg = (stMsgExtend*)pstEventMsg;
	int8_t cExtBDState = GetSysPsExtendStatus();

	if(cExtBDState != STATE_SYSPS_EXTEND_IDLE )
	{
		Trace("ExtendBoard wake up not yet!!!!!\n");

		if(cExtBDState == STATE_SYSPS_EXTEND_INIT)
		{
			SaveControlEvtMsg(pstMsg);
			SetExtBDControlState(eCONTROL_WATTING);
		}

		g_stExtendBoard.ucExtendBoardState = cExtBDState;
		//Send2AppAutolink(uint16_t usId, int32_t nEvent, int32_t nSubEvent, boolean_t bResult, stCarReport * pstReport, uint32_t unTracePs)
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

int SysPsExtend_EvtNone(void* pstEventMsg)
{Trace("SysPsNone_EvtNone,NotSupport\n");return 0;}

int SysPsExtend_EvtSysSystem(void* pstEventMsg)
{
	stMsgExtend* pstMsg = (stMsgExtend*)pstEventMsg;
	static int8_t s_cSleepReqTimeoutCnt =0;

    if( pstMsg->header.event == eExtend_ReqReport )
	{
		if(pstMsg->header.subEvent == eExtBD_SerialReq)
		{
			ExtendBoardSerialReq();
		}
		else if(pstMsg->header.subEvent == eExtBD_VersionInfoReq)
		{
			ExtendBoardFWInfoReq();
		}
		else if(pstMsg->header.subEvent == eExtBD_HiPassPaymenetCntReq)
		{
			ExtendBoardHiPassPaymentCntReq();
		}
		else if(pstMsg->header.subEvent == eExtBD_HiPassPaymenetDataReq)
		{
			ExtendBoardHiPassPaymentDataReq();
		}
	}
	else if(pstMsg->header.event == eExtend_ReqFota)
	{
		if(pstMsg->header.subEvent == eExtBD_FWUpdateStartReq)
		{
            stExtBDFWFileInfo stExtBDFWFileInfo;
//            memcpy(&stExtBDFWFileInfo,pstMsg->carReport.Buffer,sizeof(stExtBDFWFileInfo));
            memcpy(&stExtBDFWFileInfo,pstMsg->buffer,sizeof(stExtBDFWFileInfo));

			SetSysPsExtendStatus(STATE_SYSPS_EXTEND_FOTA);
			ExtendBoardFotaStartReq(&stExtBDFWFileInfo);
		}
		else if(pstMsg->header.subEvent == eExtBD_DeviceRestReq)
		{
			SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RESET);
			ExtendBoardReset();
		}
	}

	SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RUN);
	SetExtBoardControlCallbackStart();
	return 0;
}

int SysPsExtend_EvtSysSystemMsg(void* pstEventMsg)
{
    stMsgExtend* pstMsg = (stMsgExtend*)pstEventMsg;
	static int8_t s_cSleepReqTimeoutCnt =0;
	
	if(pstMsg->header.event  == eSYS_ReqSleep)
	{
		Trace("\n@@@@@@@@@@@@@@@@@@@\n");
		Trace("=== Set Ext board go to sleep ====\n");
		Trace("\n@@@@@@@@@@@@@@@@@@@\n");

		if(GetSysPsExtendStatus() != STATE_SYSPS_EXTEND_IDLE)
		{
			if(s_cSleepReqTimeoutCnt > 2)
			{
				SetSysPsExtendStatus(STATE_SYSPS_EXTEND_SLEEP);
				Send2MngSysMsg2(eMngExtend, eSYS_RspSleep, 0, true, NULL, eMngExtend);
			}
			s_cSleepReqTimeoutCnt++;
		}
		else
		{
            unsigned char ucFobKeyPowerControl = 1; // FOB Off;
            ExtendBoardFobKeyPowerControlReq(ucFobKeyPowerControl);
            ExtendBoardHiPassPowerOffReq();
            SetExtendBoardModuleSleep();
			//APP_Delay(10);
		}

		
		SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RUN); 
		SetExtBoardControlCallbackStart();
	}
    else if(pstMsg->header.event == eExtend_ReqControl)
	{
		if(IsExtboardControlCar(pstMsg) == true)
		{
			// idle 상태가 아니면 제어 불가하기 위해서
			// run 상태는 제어 중임을 나타내며 res 이벤트에서 idle로 상태를 변경 해준다.
			return 0;
		}

        if(pstMsg->header.subEvent == eExtBD_SetAntiThiefReq) // control
		{
			// 도난방지 설정메모리 저장
			// pstMsg->buffer[0] == 0 : 도난비활성화, 1 : 도난활성화 
            g_stExtendBoard.ucAntiThiefFlag = pstMsg->buffer[0];
        }
    }
}

int SysPsExtend_EvtSysSensor(void* pstEventMsg)
{Trace("SysPsSensor_EvtNone,NotSupport\n");return 0;}

int SysPsExtend_EvtSysBluetooth(void* pstEventMsg)
{
	stMsgExtend* pstMsg = (stMsgExtend*)pstEventMsg;

	if(pstMsg->header.event == eExtend_RspFirstGenControl)
	{
		if(IsExtboardControlCar(pstMsg) == true)
		{
			// idle 상태가 아니면 제어 불가하기 위해서
			// run 상태는 제어 중임을 나타내며 res 이벤트에서 idle로 상태를 변경 해준다.
			return 0;
		}

		if(pstMsg->header.subEvent == eExtBD_DoorCloseReq || pstMsg->header.subEvent == eExtBD_DoorOpenReq)
		{
			Trace("Door Open Command\n");
			LockUnlockSet(pstMsg->header.subEvent);
		}
		else if(pstMsg->header.subEvent == eExtBD_lampOnReq || pstMsg->header.subEvent == eExtBD_lampOffReq)
		{
			/*
				uint8_t ucSelect,uint8_t ucTime
				pstMsg->header.subEvent : lamp on / off
				pstMsg->carReport.Buffer : lamp 점등 시간
			*/
			//LampSet(pstMsg->header.subEvent, pstMsg->carReport.Buffer[0]);
			// 차후 Report에서 값 지정해줘야 함.
			LampSet(pstMsg->header.subEvent, 5000);
		}
		else if(pstMsg->header.subEvent == eExtBD_hornOnReq)
		{
			/*
				uint32_t uiTime, uint32_t uiGap
				pstMsg->carReport.Buffer[0] :
				pstMsg->carReport.Buffer[4] :
			*/
	//				uint32_t uiHornOnTime, uiHornOnGapTime;
	//				memcpy(&uiHornOnTime, &pstMsg->carReport.Buffer[0], sizeof(uiHornOnTime));
	//				memcpy(&uiHornOnGapTime, &pstMsg->carReport.Buffer[4], sizeof(uiHornOnTime));
	//				HornOn(uiHornOnTime, uiHornOnGapTime);
			// 차후 Report에서 값 지정해줘야 함.
			HornOn(5000, 5000);
		}
	}

	SetExtBoardControlCallbackStart();
	SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RUN);

    return 0;
}

int SysPsExtend_EvtSysModem(void* pstEventMsg)
{
	stMsgExtend* pstMsg = (stMsgExtend*)pstEventMsg;

	if(pstMsg->header.event == eExtend_ReqControl)
	{
		if(IsExtboardControlCar(pstMsg) == true)
		{
			// idle 상태가 아니면 제어 불가하기 위해서
			// run 상태는 제어 중임을 나타내며 res 이벤트에서 idle로 상태를 변경 해준다.
			return 0;
		}

		if(pstMsg->header.subEvent == eExtBD_WakeupReq) // control
		{
			SetExtendBoardModuleWakeup();
		}
		else if(pstMsg->header.subEvent == eExtBD_FobKeyPowerStatusReq) // control
		{
			ExtendBoardFobKeyStatusReq();
		}
		else if(pstMsg->header.subEvent == eExtBD_DoorCloseReq || pstMsg->header.subEvent == eExtBD_DoorOpenReq)
		{
			Trace("Door Open Command\n");
			LockUnlockSet(pstMsg->header.subEvent);
		}
		else if(pstMsg->header.subEvent == eExtBD_lampOnReq || pstMsg->header.subEvent == eExtBD_lampOffReq)
		{
			/*
				uint8_t ucSelect,uint8_t ucTime
				pstMsg->header.subEvent : lamp on / off
				pstMsg->carReport.Buffer : lamp 점등 시간
			*/
			//LampSet(pstMsg->header.subEvent, pstMsg->carReport.Buffer[0]);
			// 차후 Report에서 값 지정해줘야 함.
			LampSet(pstMsg->header.subEvent, 5000);
		}
		else if(pstMsg->header.subEvent == eExtBD_hornOnReq)
		{
			/*
				uint32_t uiTime, uint32_t uiGap
				pstMsg->carReport.Buffer[0] :
				pstMsg->carReport.Buffer[4] :
			*/
//				uint32_t uiHornOnTime, uiHornOnGapTime;
//				memcpy(&uiHornOnTime, &pstMsg->carReport.Buffer[0], sizeof(uiHornOnTime));
//				memcpy(&uiHornOnGapTime, &pstMsg->carReport.Buffer[4], sizeof(uiHornOnTime));
//				HornOn(uiHornOnTime, uiHornOnGapTime);
			// 차후 Report에서 값 지정해줘야 함.
			HornOn(5000, 5000);
		}

        SetExtBoardControlCallbackStart();
        SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RUN);

	}

    return 0;
}

int SysPsExtend_EvtALObd(void* pstEventMsg)
{
    stMsgObd* pstMsgObd = (stMsgObd*)pstEventMsg;
    Trace("SysPsODB_EvtNone : pstMsg->header.event %d\n", pstMsgObd->header.event);
    
    if(pstMsgObd->header.event == eExtend_ReqControl )
	{
        if ( pstMsgObd->header.subEvent == eExtBD_FobKeyControlReq )
        {
            stReportSmartReq    *pstReportSmartReq = (stReportSmartReq*)&pstMsgObd->carReport.rpSmartKey.Request;
            unsigned char       cFobOnFlag = 0;

            switch( pstReportSmartReq->CommandType )
            {
                case eREMOTE_CON_CMD_TYPE_DOOR: // door lock/unlock상태에 의하여 포브키제어 한다.
                case eREMOTE_CON_CMD_TYPE_SETTING_ANTI_THIEF: // 브레이크 발생 시 또는 Engine Off 시 FOB POWER 제어 
                    if(pstReportSmartReq->ControlType == 1)
                    	cFobOnFlag = 0; // fob power on
					else if(pstReportSmartReq->ControlType == 0)
						cFobOnFlag = 1; // fob power off
						
					if(pstReportSmartReq->ControlType != g_stExtendBoard.ucFOBOnStatus)
					{
						ExtendBoardFobKeyPowerControlReq(cFobOnFlag);
						SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RUN);
						SetExtBoardControlCallbackStart();
					}
					break;
                    
                default:
                    break;
            } 
        }
    }

    return 0;
}

int SysPsExtend_EvtALData(void* pstEventMsg)
{Trace("SysPsALData_EvtNone,NotSupport\n");return 0;}

int SysPsExtend_EvtALAutoLink(void* pstEventMsg)
{
	stMsgExtend* pstMsg = (stMsgExtend*)pstEventMsg;

	if(pstMsg->header.event == eExtend_ReqControl)
	{
		if(IsExtboardControlCar(pstMsg) == true)
		{
			// idle 상태가 아니면 제어 불가하기 위해서
			// run 상태는 제어 중임을 나타내며 res 이벤트에서 idle로 상태를 변경 해준다.
			return 0;
		}

		if(pstMsg->header.subEvent == eExtBD_HiPassPowerOnReq) // control
		{
			ExtendBoardHiPassPowerOnReq();
		}
		else if(pstMsg->header.subEvent == eExtBD_HiPassPowerOffReq) // control
		{
			ExtendBoardHiPassPowerOffReq();
		}


		SetExtBoardControlCallbackStart();
		SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RUN);
	}
#if 0 //  not used
	else if(pstMsg->header.event == eExtend_OneSideMsg)
	{
		if(pstMsg->header.subEvent == eExtBD_LoadingHipassConfig) // ConfigLoading
		{
			LoadConfigHipassPowerValue();

			if(GetHighPassPower() == SignHighPass_On)
			{
				ExtendBoardHiPassPowerOnReq();
			}
			else if(GetHighPassPower() == SignHighPass_Off)
			{
				ExtendBoardHiPassPowerOffReq();
			}
		}
	}
#endif
}

int SysPsExtend_EvtProductionTest(void* pstEventMsg)
{Trace("SysPsProductTest_EvtNone,NotSupport\n");return 0;}

int SysPsExtend_EvtExtendBoard(void* pstEventMsg)
{
	stMsgExtend* pstMsg = (stMsgExtend*)pstEventMsg;   

	InitExtBDControlTimoutValue();

	if(IsExtBoardControlTimeout() == true)
	{
		m_bExtBDControlTimeoutFlag = false;
		SetSysPsExtendStatus(STATE_SYSPS_EXTEND_IDLE);
		return 0;
	}

	if( pstMsg->header.event == eExtend_RspReport )
	{
		if(pstMsg->header.subEvent == eExtBD_ThreeSecReport)
		{
            if(GetSysPsExtendStatus() == STATE_SYSPS_EXTEND_INIT)
			{
				ExtendBoardFWInfoReq();
			}
		}
		else if(pstMsg->header.subEvent == eExtBD_SerialRes)
		{
			// send2 함수 이용해서 시리얼 전달
		}
		else if(pstMsg->header.subEvent == eExtBD_VersionInfoRes)
		{
			stExtBDVersionInfo stEXTBDVersionInfo;
			unsigned char ucFW_AppNumber[6];
            
			memcpy(ucFW_AppNumber , pstMsg->buffer, 6); // 6 = Firmware version size

            if(ucFW_AppNumber[1]==7)	//1:GDSVCI 2:VCI2 3:PDI 5: DCS 6:DCSP 7:DCSP PLUS
			{
				SetExtBDVersionInfo(ucFW_AppNumber);
			}

			if((GetSysPsExtendStatus() == STATE_SYSPS_EXTEND_INIT))
			{
				ExtendBoardFobKeyStatusReq();
			}

			GetExtDBVersionInfo(&stEXTBDVersionInfo);
			Trace("\n-------------------------------");
			Trace("\n boot : %02x, app : %02x", stEXTBDVersionInfo.usExtVersionBoot, stEXTBDVersionInfo.usExtVersionApp);
			Trace("\n-------------------------------\n");


		}
#if 0 //  not used

		else if(pstMsg->header.subEvent == eExtBD_HiPassInfoRes)
		{
			//stDriving_HighPassData stDriving_HighPassData;
			stCarReport stMessage;

			stMessage.Capital.CapitalInfo.ucDataTransferType = Capital_Socket_Comm;
			stMessage.Capital.CapitalInfo.ucSessionIDNumber = Driving_Session;
			stMessage.Capital.CapitalInfo.ucSubControlStartIndex = eMdTxSocketServiceOpen;
			stMessage.Capital.CapitalInfo.ucSendDataType = eCapitalMsg_HighPass;
			stMessage.Capital.CapitalInfo.ucSendReCommandType = eCapitalSendType_MsgA;

			memcpy(&stMessage.Capital.CapitalDriving.stHighPass, pstMsg->carReport.Buffer,sizeof(stMessage.Capital.CapitalDriving.stHighPass));

			Send2AppAutolink(eEvtALPsObd,eAPP_RspReport,eRM_DrivingInterval,true, &stMessage, 0);
			SaveHiPassCardStatus(stMessage.Capital.CapitalDriving.stHighPass.m_ucCardState);
		}
		else if(pstMsg->header.subEvent == eExtBD_HiPassStateRes)		//실제 과금 처리 사항
		{
			//stDriving_HighPassData stDriving_HighPassData;
			stCarReport stMessage;

			stMessage.Capital.CapitalInfo.ucDataTransferType = Capital_Socket_Comm;
			stMessage.Capital.CapitalInfo.ucSessionIDNumber = Driving_Session;
			stMessage.Capital.CapitalInfo.ucSubControlStartIndex = eMdTxSocketServiceOpen;
			stMessage.Capital.CapitalInfo.ucSendDataType = eCapitalMsg_HighPass;
			stMessage.Capital.CapitalInfo.ucSendReCommandType = eCapitalSendType_MsgA;

			memcpy(&stMessage.Capital.CapitalDriving.stHighPass, pstMsg->carReport.Buffer,sizeof(stMessage.Capital.CapitalDriving.stHighPass));

			Send2AppAutolink(eEvtALPsObd,eAPP_RspReport,eRM_DrivingInterval,true, &stMessage, 0);
			SaveHiPassCardStatus(stMessage.Capital.CapitalDriving.stHighPass.m_ucCardState);
		}
#endif
		else if(pstMsg->header.subEvent == eExtBD_HiPassPaymenetCntRes)
		{

		}
		else if(pstMsg->header.subEvent == eExtBD_HiPassPaymenetDataRes)
		{

		}
	}
	else if(pstMsg->header.event == eExtend_RspControl)
	{
		if(pstMsg->header.subEvent == eExtBD_WakeupRes) // control
		{
			Trace("\n@@@@@@@@@@@@@@@@@@@@@\n");
			Trace("EXTEND BOARD Wakeup!!!");
			Trace("\n@@@@@@@@@@@@@@@@@@@@@\n");

			ExtendBoardFWInfoReq();
		}
		else if(pstMsg->header.subEvent == eExtBD_SleepRes) // control
		{
			SetSysPsExtendStatus(STATE_SYSPS_EXTEND_SLEEP);
			Send2MngSysMsg2(eMngExtend, eSYS_RspSleep, 0, true, NULL, eMngExtend);

			/*********************************************************************************/
			// add waiting time for extension board stable during sleep process.
			// do not delete this delay
			APP_Delay(100);
			/*********************************************************************************/
		}
		else if(pstMsg->header.subEvent == eExtBD_HiPassPowerOnRes) // control
		{
//			Send2AppAutolink(eMngExtend, eExtend_RspControl, eExtBD_HiPassPowerOnRes, NULL, NULL, eMngExtend);
		}
		else if(pstMsg->header.subEvent == eExtBD_HiPassPowerOffRes) // control
		{
//			Send2AppAutolink(eMngExtend, eExtend_RspControl, eExtBD_HiPassPowerOffRes, NULL, NULL, eMngExtend);
		}
		else if(pstMsg->header.subEvent == eExtBD_SetAntiThiefRes) // control
		{
			Trace("eExtBD_FobKeyPowerControlResultRes");
            // pstMsg->buffer[0] == 0 : FOB on, 1 : FOB OFF, 10 : eFobkeyPower_Use
            // SendToFobKeyStatus(pstMsg->buffer[0]);
            ExtendBoardFobKeyStatusReq();
			SetSysPsExtendStatus(STATE_SYSPS_EXTEND_RUN); 
			SetExtBoardControlCallbackStart();
		}
		else if(pstMsg->header.subEvent == eExtBD_FobKeyPowerStatusRes) // control
		{
            uint8_t ucFobStatus = 0xFF;
			bool bAntiThiefFlag = 0;
            // pstMsg->buffer[0] == 0 : ON, pstMsg->buffer[0] == 1 : OFF
            // ucFobStatus == 0 : OFF, ucFobStatus == 1 : ON
            ucFobStatus = ((pstMsg->buffer[0]==0) ? 1:0);
			bAntiThiefFlag = GetAntiThiefFlagSetting();
			
            g_stExtendBoard.ucFOBOnStatus = ucFobStatus; 

			printf("eExtBD_FobKeyPowerStatusRes fob status %d\r\n", g_stExtendBoard.ucFOBOnStatus);

            if( (GetSysPsExtendStatus() == STATE_SYSPS_EXTEND_INIT) )
			{
				SysExtendBoardCheckAntiThief((boolean_t)bAntiThiefFlag, 
											 (boolean_t)ucFobStatus);
			}
            else
            {
#if defined(PROTOCOL21)
                Trace("eMESSAGE_EVENT_KEY_FOB_STATUS_ALRAM occur.. fob status %d\r\n", g_stExtendBoard.ucFOBOnStatus);
                if ( ucFobStatus != 0xFF )
                {

                    unsigned char ucEventValue = 0;
                    ucEventValue = 0x04 & (bAntiThiefFlag<<2);
                    ucEventValue += 0x01 & (ucFobStatus);
                    ReportAlramStatus(eMESSAGE_EVENT_KEY_FOB_STATUS_ALRAM, ucEventValue);
					SetSysPsExtendStatus(STATE_SYSPS_EXTEND_IDLE);
                }
#endif
            }

		}
		else if(pstMsg->header.subEvent == eExtBD_FobKeyControlRes)
		{
            // pstMsg->carReport.Buffer[0] == 0 : FOB상태 On, 1 : FOB 상태 OFF
            // pstMsg->carReport.Buffer[1] == 10 : FOB사용, 11 : FOB 미사용, 12 : 미정
            
			Trace("eExtBD_FobKeyControlRes fob status %d\r\n", pstMsg->buffer[0]);
   			Trace("Extend Setting value %d\r\n", pstMsg->buffer[1]);
		}
	}
	else if(pstMsg->header.event == eExtend_RspFota)
    {
		if(pstMsg->header.subEvent == eExtBD_FWUpdateStartRes)
		{
			//int8_t cResult = pstMsg->carReport.Buffer[0];
			int8_t cResult = pstMsg->buffer[0];
            if(cResult == 1)
			{
				// 0xA175 Req에 대한 성공 응답
				// 0XA275 Req 시작을 위해 Msg 전송
				//Send2SysExtend(eMngExtend, eAPP_ReqFota, eExtBD_FWUpdateStartRes, TRUE, NULL, eMngExtend);
				ExtendBoardFotaReq();
			}
			else
			{
				// 0xA175 Req에 대한 실패      응답
				// 시스템으로 결과 보고
				Send2MngSysMsg2(eMngExtend, eAPP_RspFota, eExtBD_FWUpdateStartRes, FALSE, NULL, eMngExtend);

				//Send2AppAutolink(eMngExtend, eAPP_RspFota, eExtBD_FWUpdateStartRes, FALSE, NULL, eMngExtend);
				SetSysPsExtendStatus(STATE_SYSPS_EXTEND_IDLE);
			}
		}
		else if(pstMsg->header.subEvent == eExtBD_FWUpdateRes)
		{
			//int8_t cResult = pstMsg->carReport.Buffer[0];
			int8_t cResult = pstMsg->buffer[0];
            if(cResult == 1)
			{
				//Send2SysExtend(eMngExtend, eAPP_ReqFota, eExtBD_FWUpdateReq, TRUE, NULL, eMngExtend);
				ExtendBoardFotaReq();
			}
			else
			{
				Send2MngSysMsg2(eMngExtend, eAPP_RspFota, eExtBD_FWUpdateRes, FALSE, NULL, eMngExtend);
				//Send2AppAutolink(eMngExtend, eAPP_RspFota, eExtBD_FWUpdateRes, FALSE, NULL, eMngExtend);
				SetSysPsExtendStatus(STATE_SYSPS_EXTEND_IDLE);
			}
		}
		else if(pstMsg->header.subEvent == eExtBD_FWUpdateEndRes)
		{
			Send2MngSysMsg2(eMngExtend, eAPP_RspFota, eExtBD_FWUpdateEndRes, true, NULL, eMngExtend);
			//Send2AppAutolink(eMngExtend, eAPP_RspFota, eExtBD_FWUpdateEndRes, FALSE, NULL, eMngExtend);
			SetSysPsExtendStatus(STATE_SYSPS_EXTEND_IDLE);
		}
    }
    else if(pstMsg->header.event == eAPP_ReqFota)
    {
//	    	if(pstMsg->header.subEvent == eExtBD_FWUpdateStartRes || pstMsg->header.subEvent == eExtBD_FWUpdateReq)
//	    	{
//	    		ExtendBoardFotaReq();
//	    	}
//	    	else
		if(pstMsg->header.subEvent == eExtBD_FWUpdateEndReq)
    	{
    		ExtendBoardFotaEndReq();
    	}
    }


    return 0;
}


void SendToFobKeyStatus(uint8_t ucFobPowerStatus)
{
    stMsgSys msg;
    memset((char*)&msg,0,sizeof(stMsgMdm));
    Trace("%s]eREMOTE_CON_CMD_TYPE_SETTING_ANTI_THIEF\n", __FUNCTION__);

    msg.header.id = eMngExtend;
    msg.header.event = eRspReport;
    msg.header.subEvent = eR_ReqFOBStatus;
    msg.rpSetting.UserSetting.stUserActionSetting.stAntiThiefCtl.bAntithiefFlag = ucFobPowerStatus;
    msg.rpSetting.UserSetting.stUserActionSetting.stAntiThiefCtl.ucResult = 1;
    msg.rpSetting.UserSetting.stUserActionSetting.stAntiThiefCtl.ucReason = 0;

    Send2MngSys2(&msg); // 확장보드에서 수신 받아 시스템으로 전달 (시스템에서 모뎀 통해서 서버로 전달해야함) 
}

void SendToDeviceVersion()
{
/*    
	stCarReport stMessage;
	stMessage.Capital.CapitalInfo.ucDataTransferType = Capital_Socket_Comm;
	stMessage.Capital.CapitalInfo.ucSessionIDNumber = Command_Session;
	stMessage.Capital.CapitalInfo.ucSubControlStartIndex = eMdTxSocketTransmitSettingReq;
	stMessage.Capital.CapitalInfo.ucSendDataType = eCapitalMsg_DeviceVersion;
	stMessage.Capital.CapitalInfo.ucSendReCommandType = eCapitalSendType_MsgBB;
	Send2AppAutolink(eEvtALPsObd,eAPP_RspReport,eRM_DrivingInterval,true, &stMessage, 0);
*/
}

#endif //#if defined(FEATURE_EXTENSION_BOARD)