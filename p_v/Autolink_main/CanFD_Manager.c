/**
******************************************************************************
* @file    CanFD_Manager.c
* @author  Cho Sang Jun
* @version V1
* @date    7-May-2020
* @brief   CanFD Communication Logic Ba
******************************************************************************
**/



/* CanFD Controller */

#include "CanFD_Manager.h"
#include "CanFD_Controller.h"
#include "OBD_Manager.h"
#include "CanFD_RecvHandler.h"
#include "GIT_Util.h"
#include "GIT_CanParsingProc.h"
#include "HdDebug.h"

#include "HalHandler.h"

#define Trace(...)	GITDebug(DEBUG_MODULES_OBD_CANFD,__VA_ARGS__);


extern stCFDControl m_stCFDCtrl;
extern stAutolinkConfigData m_stAutolinkConfigData;
extern stCanIDCheckList g_stCanIDCheckList;
extern stCanIDCheckList g_stCanIDCanFDUse;
extern stActuatorResData g_stActuatorResData[ACTUATOR_MAX_RES_CNT];
extern stActuatorReadyData	g_stActuatorReadyData[ACTUATOR_MAX_READY_CNT];
extern unsigned char g_ucTotResCnt;
extern unsigned char g_ucTotReadyCnt;
extern unsigned char g_ucUpdateProgress;  
extern unsigned char g_ucSaveSlaveCanLine;
extern stHalCANTX_STRUCT   g_CAN1_TxBuffCtrl;
extern bool g_bCanFD_FOTA_Check;  
extern unsigned char g_Check_Update;


void SetCanFDStatusForBT(eCFD_STATUS_BT eStatus)
{
	m_stCFDCtrl.eCanFDStatusForBT = eStatus;
	//Trace("%s] %d\r\n", __FUNCTION__, eStatus);
}

void SetCanFDInitHandler()
{
	SetDefaultFDState(eFDDummySignalReq);		//임시설정
	SetCanFDMainState(eCanFD_INIT);
	m_stCFDCtrl.stStateCtrl.nInitTryCount = 0;
	SetCanFDStatusForBT(eCFD_STATUS_BT_CHECKING);
	MakeCanIDMergeList();
}

void SetCanFDMainState(eCanFD_STATE eState)
{
	m_stCFDCtrl.stStateCtrl.eCanFDState = eState;
}

eCanFD_STATE GetCanFDMainState()
{
	return m_stCFDCtrl.stStateCtrl.eCanFDState;
}

uint32_t GetRetryInitCount()
{
	uint32_t nCount = m_stCFDCtrl.stStateCtrl.nInitTryCount++;
	return nCount;
}

eCanFDHandlerRsp CanFDInitHandler()
{		
	//static eCanFD_STATE eCanFDMainState = eCanFD_INIT;
	eCanFDSubControlResult eCanFDSubControlResult = eCanFdSubControl_None;
	switch(GetCanFDMainState())
	{
		case eCanFD_INIT:
				eCanFDSubControlResult = InitCanFD();
				if(eCanFDSubControlResult == eCanFdSubControl_Success)
				{
				    SetCurrentState(eFDGetVersionReq);
					SetCanFDMainState(eCanFD_FOTA);
				}
				else if(eCanFDSubControlResult == eCanFdSubControl_Fail)
				{
				  	SetCanFDMainState(eCanFD_NORSP);
				}
			break;
		case eCanFD_FOTA:
				switch(CheckCanFDFota())
				{
					case eCanFdSubControl_Success:
						SetCanFDMainState(eCanFD_VARIFYMASK);
						break;
					case eCanFdSubControl_Fail:
						SetCanFDMainState(eCanFD_NORSP);
						break;
					case eCanFdSubControl_UpdateFail:
						SetCanFDMainState(eCanFD_VARIFYMASK);
						break;
					case eCanFdSubControl_UpdateComplete:
						SetCurrentState(eFDGetCanLineReq);
						SetCanFDMainState(eCanFD_INIT);
						break;						
				}
			break;
        case eCanFD_VARIFYMASK:
                eCanFDSubControlResult = JudgeCanFDMaskingSet();
				if(eCanFDSubControlResult == eCanFdSubControl_SetMask)
				{
				    SetCurrentState(eFDSetMaskingClearReq);
					SetCanFDMainState(eCanFD_MASK);
				} 
				else if(eCanFDSubControlResult == eCanFdSubControl_Next)
				{
					SetCurrentState(eFDSetCanByPassModeReq);
				    SetCanFDMainState(eCanFD_COMPLETE);
				}
            break;
		case eCanFD_MASK:
    			eCanFDSubControlResult = WriteCanFDMasking();
				if(eCanFDSubControlResult == eCanFdSubControl_Success)
				{
					SetCurrentState(eFDSetCanByPassModeReq);				
					SetCanFDMainState(eCanFD_COMPLETE);
				}
				else if(eCanFDSubControlResult == eCanFdSubControl_Fail)
					SetCanFDMainState(eCanFD_NORSP);
			break;	
		case eCanFD_NORSP:
		  		//Trace("eCanFD_NORSP\r\n");
				SetCanFDStatusForBT(eCFD_STATUS_BT_NOMODULE);
				return eCanFdhdResFail;
		  	break;
		case eCanFD_COMPLETE:
    			eCanFDSubControlResult = InitComplete();
				if(eCanFDSubControlResult == eCanFdSubControl_Success)
				{
					CFD_SetInitComplete(true);
					SetCanFDStatusForBT(eCFD_STATUS_BT_WORKING);
					SetCanFDMainState(eCanFD_FINISH);
				}
				else if(eCanFDSubControlResult == eCanFdSubControl_Fail)
				{
					SetCanFDStatusForBT(eCFD_STATUS_BT_NOMODULE);
					SetCanFDMainState(eCanFD_NORSP);		
				}
			break;
#if 1	// lwh CFD SLEEP 관련 로직 변경
		case eCanFD_SLEEP:
    			eCanFDSubControlResult = CFAM_GotoSleep();
				if(eCanFDSubControlResult == eCanFdSubControl_Success || eCanFDSubControlResult == eCanFdSubControl_Fail)
				{
					SetCanFDMainState(eCanFD_FINISH);
				}
			break;
#endif
		case eCanFD_FINISH:				
        		SetCanFDMainState(eCanFD_INIT);
    	        return eCanFdhdResSuccess;
		    break;	
	}

	return eCanFdhdResNone;
}

void SetDefaultFDState(eInitCanFDSequence eInitType)
{
	m_stCFDCtrl.stStateCtrl.nCurrentState = eInitType;
}

#define SEND_BUFFER_SIZE 1024

eCanFDSubControlResult InitCanFD()
{
	//static int8_t s_ucTimeOutCount = 0;
	static int8_t s_ucMaskingIndex = 0;
	static int s_nDisplayCurretState = 0;

    if(s_nDisplayCurretState != GetCurrentState())
    {
        Trace("[%s] InitCanFD() %d \r\n", __FUNCTION__,GetCurrentState());            
        s_nDisplayCurretState = GetCurrentState();
    }

	switch(GetCurrentState())
	{	
        case eFDWaitSignal:
        		SetCanFDRequest(eFDNotifyCanFDWakeUp);
				SetFDRunState(eFDTxWait,eFDWaitSignal,eFDGetVersionReq,1000*10,0);
			break;
        case eFDDummySignalReq:	
        		DeFaultCanSetting();
                SetCanFDRequest(eFDNotifyCanFDWakeUp);
                CFD_DummyController();
                SetFDRunState(eFDTxWait,eFDDummySignalReq,eFDDummySignalRsp,1000,10);
			break;		
        case eFDDummySignalRsp:		
                //SetFDRunState(eFDGetVersionReq,eFDDummySignalRsp,eFDDummySignalRsp,0,0);
                SetFDRunState(eFDSetCanByPassModeReq,eFDDummySignalRsp,eFDDummySignalRsp,0,0);
			break;        
        case eFDGetVersionReq:
				SetCanFDRequest(eFDGetVersionRsp);
				CFD_GetCanFDVersion();
				//CFD_ReadBatteryVoltage();
				SetFDRunState(eFDTxWait,eFDGetVersionReq,eFDGetVersionRsp,5000,3);
			break;
			/*
		case eFDLogShowReq: 		
				SetCanFDRequest(eFDLogShowRsp);
				CFD_DisplayLog();
				SetFDRunState(eFDTxWait,eFDLogShowReq,eFDLogShowRsp,5000,0);				
			break;
		case eFDLogShowRsp: 		
				SetFDRunState(eFDGetVersionRsp,eFDLogShowRsp,eFDLogShowRsp,0,0);
			break;
			*/
		case eFDGetVersionRsp:			
				//Version Work
				SetFDRunState(eFDGetCanLineReq,eFDGetVersionRsp,eFDGetVersionRsp,0,0);
			break;
		case eFDGetCanLineReq:
				SetCanFDRequest(eFDGetCanLineRsp);
				CFD_GetCanLine();
				SetFDRunState(eFDTxWait,eFDGetCanLineReq,eFDGetCanLineRsp,3000,3);
			break;
		case eFDGetCanLineRsp:
				//CanLine Work
//SetFDRunState(eFDSetCanByPassModeReq,eFDGetCanLineRsp,eFDGetCanLineRsp,0,0);
				SetFDRunState(eFDSetCanLineReq,eFDGetCanLineRsp,eFDGetCanLineRsp,0,0);
			break;			
			
		case eFDSetCanByPassModeReq:
				SetCanFDRequest(eFDSetCanByPassModeRsp);
				CFD_SetByPassFlagAllOff();
				SetFDRunState(eFDTxWait,eFDSetCanByPassModeReq,eFDSetCanByPassModeRsp,3000,3);
			break;
case eFDSetCanByPassModeRsp:
		//CanLine Work
//SetFDRunState(eFDSetCanLineReq,eFDSetCanByPassModeRsp,eFDSetCanByPassModeRsp,0,0);
		SetFDRunState(eFDGetVersionReq,eFDSetCanByPassModeRsp,eFDSetCanByPassModeRsp,0,0);
	break;

case eFDSetCanLineReq:
		SetCanFDRequest(eFDSetCanLineRsp);
		CFD_SetCanLineMode(eCanFDLineMode_Default);
		SetFDRunState(eFDTxWait,eFDSetCanLineReq,eFDSetCanLineRsp,3000,3);
	break;
case eFDSetCanLineRsp:
		//CanLine Work
		SetFDRunState(eFDGetCanBaudRateReq,eFDSetCanByPassModeRsp,eFDSetCanByPassModeRsp,0,0);
	break;			
		case eFDGetCanBaudRateReq:
				SetCanFDRequest(eFDGetCanBaudRateRsp);
				CFD_GetCanBaudRate();
				SetFDRunState(eFDTxWait,eFDGetCanBaudRateReq,eFDGetCanBaudRateRsp,3000,3);
			break;
		case eFDGetCanBaudRateRsp:
				SetFDRunState(eFDGetAnalogSwitchReq,eFDGetCanBaudRateRsp,eFDGetCanBaudRateRsp,0,0);
                //SetFDRunState(eFDGetMaskingInfoReq,eFDGetCanBaudRateRsp,eFDGetCanBaudRateRsp,0);
			break;						
			
		case eFDGetAnalogSwitchReq:
				SetCanFDRequest(eFDGetAnalogSwitchRsp);
				CFD_GetAnalogSwitch();
				SetFDRunState(eFDTxWait,eFDGetAnalogSwitchReq,eFDGetAnalogSwitchRsp,3000,3);
			break;
		case eFDGetAnalogSwitchRsp:
				SetFDRunState(eFDGetMaskingInfoReq,eFDGetVersionRsp,eFDGetMaskingInfoReq,0,0);
			break;			
		case eFDGetMaskingInfoReq:		
				s_ucMaskingIndex++;
				SetCanFDRequest(eFDGetMaskingInfoRsp);
				CFD_GetCanMaskingInfo(s_ucMaskingIndex);
				SetFDRunState(eFDTxWait,eFDGetMaskingInfoReq,eFDGetMaskingInfoRsp,3000,3);		
			break;
		case eFDGetMaskingInfoRsp:		
				//Masking Work
				if(s_ucMaskingIndex >= eCanFD_Spi_Number_4)
				{
					s_ucMaskingIndex = 0;
					return eCanFdSubControl_Success;							
				}
				else
				{
					SetFDRunState(eFDGetMaskingInfoReq,eFDGetMaskingInfoRsp,eFDGetMaskingInfoRsp,0,0);
				}
			break;
		case eFDTxWait:
				if( (Get_Tmr() - m_stCFDCtrl.stStateCtrl.ulTimeout) > m_stCFDCtrl.stStateCtrl.nMaxTimeout)
				{
					if(m_stCFDCtrl.stStateCtrl.nRetryCount <= m_stCFDCtrl.stStateCtrl.nCurrentCount)
					{
                        Trace("[%s] eFDTxWait Occur TimeOut !!!!! \r\n", __FUNCTION__);                                        
						SetInitCurretCount();
                        return eCanFdSubControl_Fail;                   
					}
					else 
					{
					    m_stCFDCtrl.stStateCtrl.nCurrentCount++;
					    Trace("[%s] eFDTxWait Occur TimeOut - Retry Process [%d][%d] !!!!! \r\n",__FUNCTION__,m_stCFDCtrl.stStateCtrl.nCurrentCount,m_stCFDCtrl.stStateCtrl.nPrevState);
					    m_stCFDCtrl.stStateCtrl.nCurrentState = m_stCFDCtrl.stStateCtrl.nPrevState;
					}
				}
				else
				{
					switch(CanFDCompareReqRsp())
					{
						case eCanFdResult_YES:
//Trace("[%s] 00000 GetCanFDRequest() == GetCanFDResponse()-1 is Same \r\n", __FUNCTION__);												
                                SetInitCurretCount();
								SetCurrentState(GetNextState());													
							break;
					}
				}
			break;
	}

	return eCanFdSubControl_None;
}

eCanFDSubControlResult CheckCanFDFota()
{
	static int s_nDisplayCurretState = 0;
	static unsigned int s_uiTransmitCurCount=0;
	static unsigned short s_usTransmit8ByteLoopCount=0;
	static unsigned short s_usTransmitSectorCount=0;
	static unsigned int s_uiTransmitTotSize=0;
	static unsigned int s_nStartAddress=0;
	static unsigned short s_nCheckSum=0;
	//static unsigned int s_uiWaitTimer=0;
	//static boolean_t s_bFirstTimeFlag=true;
	uint8_t ucSendBuffer[8]={0,};
	boolean_t bSendFlag = false;
	static unsigned char s_ucReadBuffer[SEND_BUFFER_SIZE]={0,};
	static boolean_t s_bUpdateComplete = false;
	int i = 0;

    if(s_nDisplayCurretState != GetCurrentState())
    {
//      Trace("[%s] CheckCanFDFota() %d \r\n", __FUNCTION__,GetCurrentState());            
        s_nDisplayCurretState = GetCurrentState();
    }

	switch(GetCurrentState())
	{	
        case eFDGetVersionReq:
				SetCanFDRequest(eFDGetVersionRsp);
				CFD_GetCanFDVersion();
				SetFDRunState(eFDTxWait,eFDGetVersionReq,eFDGetVersionRsp,5000,3);
			break;
		case eFDGetVersionRsp:			
				//Version Work
				m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.eNeedToUpdate = NeedToUpdateCheck2();
				
				printf("eFDGetVersionRsp:eNeedToUpdate %d\r\n",m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.eNeedToUpdate);
                printf("eFDGetVersionRsp current bootloader:0x%04X, current Application:0x%04X\r\n",m_stCFDCtrl.stVer.usBlVer, m_stCFDCtrl.stVer.usAppVer);
                printf("eFDGetVersionRsp saved bootloader:0x%04X, saved Application:0x%04X\r\n",m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion, m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion);
//				printf("BootSize %d, CS:0x%04X,AppSize %d, CS:0x%04X\r\n",m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiBootSize,m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootCheckSum,m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiAppSize,m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppCheckSum);
				
				switch( m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.eNeedToUpdate )
				{
					case eCANFD_BOARD_NEED_UPDATE_BOOT:
					  	if( m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiBootSize == 0 )
						{
						  	printf("!!!!!!!!!!!!!eCANFD_BOARD_NEED_UPDATE_BOOT UpdateFail!!!!!!!\r\n");
						  	return eCanFdSubControl_UpdateFail;
						}
						else	SetFDRunState(eFDUpdateStartReq,eFDGetVersionRsp,eFDGetVersionRsp,0,0);
					  	break;
					case eCANFD_BOARD_NEED_UPDATE_APP:
					  	if( m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiAppSize == 0 )
						{
						  	printf("!!!!!!!!!!!!!eCANFD_BOARD_NEED_UPDATE_APP UpdateFail!!!!!!!\r\n");
						  	return eCanFdSubControl_UpdateFail;
						}
						else	SetFDRunState(eFDUpdateStartReq,eFDGetVersionRsp,eFDGetVersionRsp,0,0);
						break;
					default:
                        g_bCanFD_FOTA_Check = false;
						if(s_bUpdateComplete == true)
						{
							s_bUpdateComplete = false;
							printf("!!!!!!!!!!!!!default eCanFdSubControl_UpdateComplete!!!!!!!\r\n");
							return eCanFdSubControl_UpdateComplete;
						}
						else 
						{
						  	printf("!!!!!!!!!!!!!default eCanFdSubControl_Success!!!!!!!\r\n"); 
							return eCanFdSubControl_Success;
						}
						break;
				}
			break;
		case eFDUpdateStartReq:
				SetCanFDStatusForBT(eCFD_STATUS_BT_UPDATING);
				printf("eFDUpdateStart\r\n");
		 		if( m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.eNeedToUpdate == eCANFD_BOARD_NEED_UPDATE_BOOT )
		 		{
					printf("usBootVersion:0x%04X,uiBootSize:%d\r\n",m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion,m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiBootSize);
					CFD_SetFWUpdateStart(m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion,m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiBootSize,CANFD_UPDATE_BOOT);
					s_uiTransmitTotSize = m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiBootSize;
					s_nStartAddress = SECTOR_EXBOARD_BOOT_FIRMWARE;
		 		}
				else if( m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.eNeedToUpdate == eCANFD_BOARD_NEED_UPDATE_APP )
				{
					printf("usAppVersion:0x%04X,uiAppSize:%d\r\n",m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion,m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiAppSize);
					CFD_SetFWUpdateStart(m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion,m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiAppSize,CANFD_UPDATE_APPLICATION);
					s_uiTransmitTotSize = m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.uiAppSize;
					s_nStartAddress = SECTOR_EXBOARD_APP_FIRMWARE;
				}

				s_uiTransmitCurCount = 0;
				s_usTransmitSectorCount = 0;
				g_ucUpdateProgress = 0;
				s_nCheckSum = 0;

				SetCanFDRequest(eFDUpdateStartRsp);
				SetFDRunState(eFDTxWait,eFDUpdateStartReq,eFDUpdateStartRsp,5000,0);
			break;
		case eFDUpdateStartRsp:
		 	 	SetFDRunState(eFDUpdateTransmitReq,eFDUpdateStartRsp,eFDUpdateStartRsp,0,0);
		  	break;
		case eFDUpdateTransmitReq:
				memset(ucSendBuffer,0x00,sizeof(ucSendBuffer));

				if( ((s_uiTransmitCurCount % SEND_BUFFER_SIZE) == 0) && (s_uiTransmitCurCount < s_uiTransmitTotSize) )
				{
					memset(s_ucReadBuffer,0x00,sizeof(s_ucReadBuffer));
					//Trace("Read address:%X\r\n",(s_nStartAddress*SFLASH_SECTOR_SIZE)+(s_usTransmitSectorCount*SEND_BUFFER_SIZE),SEND_BUFFER_SIZE);
					sFLASH_ReadBuffer((uint8_t *)s_ucReadBuffer, (s_nStartAddress*SFLASH_SECTOR_SIZE)+(s_usTransmitSectorCount*SEND_BUFFER_SIZE), SEND_BUFFER_SIZE );
					s_usTransmitSectorCount++;
					s_usTransmit8ByteLoopCount = 0;
				}

				for(i =0; i<8; i++)
				{
				    //Trace("s_usTransmit8ByteLoopCount%d,s_uiTransmitCurCount:%d,ReadBufferPos:%d,s_uiTransmitTotSize:%d\r\n",s_usTransmit8ByteLoopCount,s_uiTransmitCurCount,((s_usTransmitSectorCount-1)*SFLASH_SECTOR_SIZE)+((s_usTransmit8ByteLoopCount*8)+i),s_uiTransmitTotSize);
					if( s_uiTransmitCurCount < s_uiTransmitTotSize )
					{
						ucSendBuffer[i] = s_ucReadBuffer[(s_usTransmit8ByteLoopCount*8)+i];
						//Trace("%02X ",ucSendBuffer[i]);
						s_nCheckSum += ucSendBuffer[i];
						s_uiTransmitCurCount++;
						bSendFlag = true;
						if( s_uiTransmitCurCount >= s_uiTransmitTotSize ) 	break;
					}
					else
					{
						bSendFlag = false;
						break;
					}
				}
				//Trace("\r\n");
				if( s_uiTransmitTotSize != 0) g_ucUpdateProgress = (s_uiTransmitCurCount*100)/s_uiTransmitTotSize;
				//Trace("g_ucUpdateProgress:%d\r\n",g_ucUpdateProgress);
				s_usTransmit8ByteLoopCount++;
#if 1
				if( bSendFlag == true )
				{
//					Trace("-");
					if( i == 8 )
					{
						CFD_SetFWUpdateTransmit2(ucSendBuffer,i);
					}
					else
					{
						CFD_SetFWUpdateTransmit2(ucSendBuffer,i+1);
					}
					SetFDRunState(eFDTxWait,eFDUpdateTransmitReq,eFDUpdateTransmitRsp,5000,0);
//					s_uiWaitTimer = Get_Tmr();
					SetCanFDRequest(eFDUpdateTransmitRsp);
				}
				else
				{
					g_ucUpdateProgress = 100; 
					printf("go eFDUpdateEnd : %d s_usTransmitSectorCount: %d\r\n",s_uiTransmitCurCount,s_usTransmitSectorCount);
					SetFDRunState(eFDUpdateEndReq,eFDUpdateTransmitReq,eFDUpdateTransmitReq,0,0);
					SetCanFDRequest(eFDUpdateTransmitReq);
				}
			break;
			case eFDUpdateTransmitRsp:
			  	if( m_stCFDCtrl.eFotaResult == eCanFDFotaResult_Success )
				{
					SetFDRunState(eFDUpdateTransmitReq,eFDUpdateTransmitRsp,eFDUpdateTransmitRsp,0,0);
				}
				else
				{
				  	return eCanFdSubControl_Fail;
				}
			break;
#else
//				if( bSendFlag == true )
//				{
//					Trace("-",s_uiTransmitCurCount);
//					if( i == 8 )
//					{
//						CFD_SetFWUpdateTransmit2(ucSendBuffer,i);
//					}
//					else
//					{
//						CFD_SetFWUpdateTransmit2(ucSendBuffer,i+1);
//					}
//					SetFDRunState(eFDUpdateTransmitRsp,eFDUpdateTransmitReq,eFDUpdateTransmit,0,0);
//					s_uiWaitTimer = Get_Tmr();
//				}
//				else
//				{
//					printf("go eFDUpdateEnd : %d\r\n",s_uiTransmitCurCount);
//					SetFDRunState(eFDUpdateEndReq,eFDUpdateTransmitReq,eFDUpdateTransmit,0,0);
//				}
//			
//				SetCanFDRequest(eFDUpdateTransmitReq);
//			break;
//		case eFDUpdateTransmitRsp:
//				if( Get_TmrDelta(Get_Tmr(),s_uiWaitTimer) > 1 )
//				{
//					SetFDRunState(eFDUpdateTransmitReq,eFDUpdateTransmitRsp,eFDUpdateTransmitRsp,0,0);
//					//s_uiWaitTimer = Get_Tmr();
//					//s_bFirstTimeFlag = true;
//				}
//			break;
#endif
		case eFDUpdateEndReq:
		        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@Calc s_nCheckSum : 0x%04X\r\n",s_nCheckSum);////2
		  		if( m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.eNeedToUpdate == eCANFD_BOARD_NEED_UPDATE_BOOT )
		  		{
					CFD_SetFWUpdateEnd(m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootCheckSum);
		  		}
				else if( m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.eNeedToUpdate == eCANFD_BOARD_NEED_UPDATE_APP )
				{
					CFD_SetFWUpdateEnd(m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppCheckSum);
				}
				
				SetFDRunState(eFDTxWait,eFDUpdateEndReq,eFDUpdateEndRsp,10000,0);
				SetCanFDRequest(eFDUpdateEndRsp);
			break;
		case eFDUpdateEndRsp:
		  		if( m_stCFDCtrl.eFotaResult == eCanFDFotaResult_Success )
				{
				  	SetFDRunState(eFDUpdateAfterResetReq,eFDUpdateEndRsp,eFDUpdateEndRsp,0,0);
				}
				else
				{
//                  NONE = 0x00;SUCCESS = 01;SIZE = 02;CHECKSUM = 03;COPY = 04;TARGET = 05;ERASE = 06;
				  	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
				  	printf("m_stCFDCtrl.eFotaResult : %d\r\n",m_stCFDCtrl.eFotaResult);
					printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
				  	return eCanFdSubControl_Fail; 
				}
				/*
		  		if( m_stAutolinkConfigData.stSystem.stExtBoardUpdateInfo.eNeedToUpdate == eCANFD_BOARD_NEED_UPDATE_BOOT )
		  		{
					SetFDRunState(eFDUpdateAfterResetReq,eFDUpdateEndRsp,eFDUpdateEndRsp,0,0);
		  		}
				else if( m_stAutolinkConfigData.stSystem.stExtBoardUpdateInfo.eNeedToUpdate == eCANFD_BOARD_NEED_UPDATE_APP )
				{
					SetFDRunState(eFDUpdateAfterResetReq,eFDUpdateEndRsp,eFDUpdateEndRsp,0,0);
				}
				*/
		  	break;
		case eFDUpdateAfterResetReq:
		  		printf("~~~~eFDUpdateAfterResetReq~~~~\r\n");  ////3
				CFD_Reset();
				SetCanFDRequest(eFDUpdateAfterResetRsp);				
				SetFDRunState(eFDTxWait,eFDUpdateAfterResetReq,eFDUpdateAfterResetRsp,10000,0);
			break;
		case eFDUpdateAfterResetRsp:
		 		printf("~~~~!!!!eFDUpdateAfterResetRsp!!!!~~~~\r\n");    ////4
				SetFDRunState(eFDTxWait,eFDUpdateAfterResetRsp,eFDGetVersionReq,1000*30,0);
				s_bUpdateComplete = true;
				SetCanFDRequest(eFDNotifyCanFDWakeUp);
			break;
		case eFDUpdateWaitBooting:
			break;
		case eFDTxWait:
				if( (Get_Tmr() - m_stCFDCtrl.stStateCtrl.ulTimeout) > m_stCFDCtrl.stStateCtrl.nMaxTimeout)
				{
					if(m_stCFDCtrl.stStateCtrl.nRetryCount <= m_stCFDCtrl.stStateCtrl.nCurrentCount)
					{
                        printf("[%s] eFDTxWait Occur TimeOut !!!!!%d,%d \r\n", __FUNCTION__,m_stCFDCtrl.stStateCtrl.nPrevState,m_stCFDCtrl.stStateCtrl.nNextState);
						SetInitCurretCount();
                        return eCanFdSubControl_Fail;                   
					}
					else 
					{
					    m_stCFDCtrl.stStateCtrl.nCurrentCount++;
					    printf("[%s] eFDTxWait Occur TimeOut - Retry Process [%d] !!!!! \r\n",__FUNCTION__,m_stCFDCtrl.stStateCtrl.nCurrentCount);                                        					    
					    m_stCFDCtrl.stStateCtrl.nCurrentState = m_stCFDCtrl.stStateCtrl.nPrevState;
					}
				}
				else
				{
					switch(CanFDCompareReqRsp())
					{
						case eCanFdResult_YES:
//								printf("[%s] 00000 GetCanFDRequest() == GetCanFDResponse()-1 is Same \r\n", __FUNCTION__);												
                                SetInitCurretCount();
								SetCurrentState(GetNextState());													
							break;
					}
				}
			break;
	}

	return eCanFdSubControl_None;
}

eCanFDSubControlResult JudgeCanFDMaskingSet()
{
    //boolean_t bCertifyResult = false;
    for(int i = 0 ; i < MAX_SPI_COUNT ; i++)
    {
       if(m_stCFDCtrl.stMask.ucSpiMaskCertify[i] == false)
       {
            return eCanFdSubControl_SetMask;
            //break;
       }    
    }
    
    return eCanFdSubControl_Next;

}

eCanFDSubControlResult WriteCanFDMasking()
{
	//static int8_t s_ucTimeOutCount = 0;
	static int8_t s_ucMaskingIndex = 0;

    Trace("[%s] GetCurrentState() : %d \r\n", __FUNCTION__,GetCurrentState());        

	switch(GetCurrentState())
	{	
        case eFDSetMaskingClearReq:
        		s_ucMaskingIndex++;
				SetCanFDRequest(eFDSetMaskingClearRsp);
				CFD_SetClearCanMasking(s_ucMaskingIndex);
				SetFDRunState(eFDTxWait,eFDSetMaskingClearRsp,eFDSetMaskingClearRsp,3000,3);
			break;
        case eFDSetMaskingClearRsp:
				if(s_ucMaskingIndex >= eCanFD_Spi_Number_4)
				{
					s_ucMaskingIndex = 0;								
					SetFDRunState(eFDSetMaskingInfoReq,eFDSetMaskingClearRsp,eFDSetMaskingClearRsp,0,0);
				}
				else
					SetFDRunState(eFDSetMaskingClearReq,eFDSetMaskingClearRsp,eFDSetMaskingClearRsp,0,0);
			break;	
		case eFDSetMaskingInfoReq:
				s_ucMaskingIndex++;
                Trace("[%s] s_ucMaskingIndex : %d , m_stCFDCtrl.stCanIDMerge.ucMaskCount : %d \r\n",__FUNCTION__,s_ucMaskingIndex,m_stCFDCtrl.stCanIDMerge.ucMaskCount);        				
				SetCanFDRequest(eFDSetMaskingInfoRsp);

				if(m_stCFDCtrl.stCanIDMerge.ucMaskCount > CANMASKING_HW_MAXCOUNT)
				{
					CFD_Channel_Masket_Set(s_ucMaskingIndex,(unsigned char)m_stCFDCtrl.stCanIDMerge.ucMaskCount,CANMASKING_SW,(unsigned int *)m_stCFDCtrl.stCanIDMerge.nStartMask,(unsigned int *)m_stCFDCtrl.stCanIDMerge.nEndMask);					
				}
				else
				{
					//Common Setting
					CFD_Channel_Masket_Set(s_ucMaskingIndex,(unsigned char)m_stCFDCtrl.stCanIDMerge.ucMaskCount,CANMASKING_HW,(unsigned int *)m_stCFDCtrl.stCanIDMerge.nStartMask,(unsigned int *)m_stCFDCtrl.stCanIDMerge.nEndMask);
				}
				//CFD_Channel_Masket_Set(s_ucMaskingIndex,(unsigned char)m_stCFDCtrl.stCanIDMerge.ucMaskCount,CANMASKING_SW,(unsigned int *)m_stCFDCtrl.stCanIDMerge.nStartMask,(unsigned int *)m_stCFDCtrl.stCanIDMerge.nEndMask);
				//Manual Setting
				//CFD_Channel_Masket_Set_Manual(s_ucMaskingIndex,(unsigned char)m_stCFDCtrl.stCanIDMerge.ucMaskCount,CANMASKING_HW,(unsigned int *)m_stCFDCtrl.stCanIDMerge.nStartMask,(unsigned int *)m_stCFDCtrl.stCanIDMerge.nEndMask,(unsigned char *)m_stCFDCtrl.stCanIDMerge.ucLineIndex);
				//unsigned char ucSpiNum, unsigned char ucMaskNum, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue)				
				SetFDRunState(eFDTxWait,eFDSetMaskingInfoReq,eFDSetMaskingInfoRsp,3000,3);
			break;	
		case eFDSetMaskingInfoRsp:
				if(s_ucMaskingIndex >= eCanFD_Spi_Number_4)
				{
					s_ucMaskingIndex = 0;
					return eCanFdSubControl_Success;
				}
				else
				{
					SetFDRunState(eFDSetMaskingInfoReq,eFDSetMaskingInfoRsp,eFDSetMaskingInfoRsp,0,0);
				}				
			break;				
		case eFDTxWait:
				if((Get_Tmr() - m_stCFDCtrl.stStateCtrl.ulTimeout) > m_stCFDCtrl.stStateCtrl.nMaxTimeout)
				{
                    if(m_stCFDCtrl.stStateCtrl.nRetryCount <= m_stCFDCtrl.stStateCtrl.nCurrentCount)
					{
                        printf("[%s] eFDTxWait Occur TimeOut !!!!! \r\n", __FUNCTION__);                                        
						SetInitCurretCount();
                        return eCanFdSubControl_Fail;                   
					}
					else 
					{
					    m_stCFDCtrl.stStateCtrl.nCurrentCount++;
					    printf("[%s] eFDTxWait Occur TimeOut - Retry Process [%d] !!!!! \r\n",__FUNCTION__,m_stCFDCtrl.stStateCtrl.nCurrentCount);                                        
					    m_stCFDCtrl.stStateCtrl.nCurrentState = m_stCFDCtrl.stStateCtrl.nPrevState;
					}
				}
				else
				{
					switch(CanFDCompareReqRsp())
					{
						case eCanFdResult_YES:
						        SetInitCurretCount();
								SetCurrentState(GetNextState());
							break;
					}
				}
			break;
	}

	return eCanFdSubControl_None;
}

eCanFDSubControlResult InitComplete()
{
	//static int8_t s_ucTimeOutCount = 0;
	//static int8_t s_ucMaskingIndex = 0;

    Trace("[%s] GetCurrentState() : %d \r\n", __FUNCTION__,GetCurrentState());        

	switch(GetCurrentState())
	{	
		case eFDSetCanByPassModeReq:
				SetCanFDRequest(eFDSetCanByPassModeRsp);
				CFD_SetByPassFlagAllOn();
				SetFDRunState(eFDTxWait,eFDSetCanByPassModeReq,eFDSetCanByPassModeRsp,3000,3);
			break;
		case eFDSetCanByPassModeRsp:
				return eCanFdSubControl_Success;
			break;
		case eFDTxWait:
				if((Get_Tmr() - m_stCFDCtrl.stStateCtrl.ulTimeout) > m_stCFDCtrl.stStateCtrl.nMaxTimeout)
				{
                    if(m_stCFDCtrl.stStateCtrl.nRetryCount <= m_stCFDCtrl.stStateCtrl.nCurrentCount)
					{
                        printf("[%s] eFDTxWait Occur TimeOut !!!!! \r\n", __FUNCTION__);                                        
						SetInitCurretCount();
                        return eCanFdSubControl_Fail;                   
					}
					else 
					{
					    m_stCFDCtrl.stStateCtrl.nCurrentCount++;
					    printf("[%s] eFDTxWait Occur TimeOut - Retry Process [%d] !!!!!%d!%d! \r\n",__FUNCTION__,m_stCFDCtrl.stStateCtrl.nCurrentCount,g_eCanCommState,g_eLCanCommState);
					    m_stCFDCtrl.stStateCtrl.nCurrentState = m_stCFDCtrl.stStateCtrl.nPrevState;
					}
				}
				else
				{
					switch(CanFDCompareReqRsp())
					{
						case eCanFdResult_YES:
						        SetInitCurretCount();
								SetCurrentState(GetNextState());
							break;
					}
				}
			break;
	}

	return eCanFdSubControl_None;
}

#if 1	// lwh CFD SLEEP 관련 로직 변경
eCanFDSubControlResult CFAM_GotoSleep()
{
	//Trace("[%s] GetCurrentState() : %d \r\n", __FUNCTION__,GetCurrentState());        
	unsigned int uiWaitTime = 0;
	unsigned char ucRetryCount = 0;
    unsigned char cBypass[LENGTH_CANFRAME_DATA]={0,};

	if(m_stCFDCtrl.stVer.usAppVer >= CANFD_SLEEP_RESPONSE_VERSION)
	{
		uiWaitTime = 1200; //CANFD Adapter에서 sleep request 받고 1초 뒤에 Wakeup pin Check 후 응답 
		ucRetryCount = 2;
	}else
	{
		uiWaitTime = 15;
		ucRetryCount = 5;
	}
	
	switch(GetCurrentState())
	{	
		case eFDSetCanByPassModeReq: //전기차 Sleep 중 Wake up 이슈로 인한 추가, G-CAN 에서 데이터가 나오므로 B-CAN 만 수신 가능하도록 Bypass 설정 
			SetCanFDRequest(eFDSetCanByPassModeRsp);
			cBypass[2]=0x01; //223 Line (B-CAN)
			CFD_SetByPassFlag(cBypass);
			SetFDRunState(eFDTxWait,eFDSetCanByPassModeReq,eFDSetCanByPassModeRsp,uiWaitTime,ucRetryCount); 
			break;
		case eFDSetCanByPassModeRsp:    
			//SetCanFDRequest(eFDSetSleepReq);
            SetCurrentState(eFDSetSleepReq);    
			break;      
		case eFDSetSleepReq:
				SetCanFDRequest(eFDSetSleepRsp);
				CFD_Sleep();
				SetFDRunState(eFDTxWait,eFDSetSleepReq,eFDSetSleepRsp,uiWaitTime,ucRetryCount); 
				Trace("[%s]____Request\r\n",__FUNCTION__);
			break;
		case eFDSetSleepRsp:
				Trace("eFDSetSleepRsp : CANFD Sleep Response Success\r\n");
				if(m_stCFDCtrl.stVer.usAppVer >= CANFD_SLEEP_RESPONSE_VERSION) //PDH
				{
					if(CFD_GetCanFDSleepStatus() == true) return eCanFdSubControl_Success;
					else                                  return eCanFdSubControl_Fail;
				}else
				{
					return eCanFdSubControl_Success;
				}
		case eFDTxWait:
				if((Get_Tmr() - m_stCFDCtrl.stStateCtrl.ulTimeout) > m_stCFDCtrl.stStateCtrl.nMaxTimeout)
				{
					Trace("Total RetryCount : %d, CurrentCount : %d\r\n", m_stCFDCtrl.stStateCtrl.nRetryCount, m_stCFDCtrl.stStateCtrl.nCurrentCount);
					if(m_stCFDCtrl.stStateCtrl.nRetryCount <= m_stCFDCtrl.stStateCtrl.nCurrentCount)
					{
						Trace("[%s] eFDTxWait Occur TimeOut !!!!! \r\n", __FUNCTION__);
						CFD_SetCanFDSleepStatus(true); // CANFD 통신 실패시, 메인 모듈 Sleep 처리
						SetInitCurretCount(); //dahae 
						return eCanFdSubControl_Fail;					
					}
					else 
					{
						m_stCFDCtrl.stStateCtrl.nCurrentCount++;
						Trace("[%s] eFDTxWait Occur TimeOut - Retry Process [%d] !!!!! \r\n",__FUNCTION__,m_stCFDCtrl.stStateCtrl.nCurrentCount);										  
						m_stCFDCtrl.stStateCtrl.nCurrentState = m_stCFDCtrl.stStateCtrl.nPrevState;
					}
				}
				else
				{	
					switch(CanFDCompareReqRsp())
					{
						case eCanFdResult_YES:
								SetInitCurretCount();
								SetCurrentState(GetNextState());
						default:
							break;
					}
				}
				break;
		default:
				break;
	}

	return eCanFdSubControl_None;
}

#endif


void SetFDRunState(eInitCanFDSequence eState, eInitCanFDSequence ePreState, eInitCanFDSequence eNextState, int nMaxTimeout, int nRetryCount)
{
	m_stCFDCtrl.stStateCtrl.nCurrentState = eState;
	m_stCFDCtrl.stStateCtrl.nPrevState    = ePreState;
	m_stCFDCtrl.stStateCtrl.nNextState    = eNextState;
	m_stCFDCtrl.stStateCtrl.ulTimeout     = Get_Tmr();
	m_stCFDCtrl.stStateCtrl.nMaxTimeout   = nMaxTimeout;
	m_stCFDCtrl.stStateCtrl.nRetryCount   = nRetryCount;
}

void SetInitCurretCount()
{
	m_stCFDCtrl.stStateCtrl.nCurrentCount = 0;
}

void SetCurrentState(int nState)
{
	m_stCFDCtrl.stStateCtrl.nCurrentState = nState;
}

int GetPrevState()
{
	return m_stCFDCtrl.stStateCtrl.nPrevState;
}

int GetCurrentState()
{
	return m_stCFDCtrl.stStateCtrl.nCurrentState;
}

int GetNextState()
{
	return m_stCFDCtrl.stStateCtrl.nNextState;
}

void SetCanFDRequest(eInitCanFDSequence eCanFDRequest)
{
	m_stCFDCtrl.stStateCtrl.eCanFdRequest = eCanFDRequest;
	m_stCFDCtrl.stStateCtrl.eCanFdResponse = eFDDefaultValue;
}

eInitCanFDSequence GetCanFDRequest()
{
	return m_stCFDCtrl.stStateCtrl.eCanFdRequest;
}

eInitCanFDSequence GetCanFDResponse()
{
	return m_stCFDCtrl.stStateCtrl.eCanFdResponse;
}

eCanFDResult CanFDCompareReqRsp()
{
	if(GetCanFDResponse() > 0)
	{
		if(GetCanFDRequest() == GetCanFDResponse())
		{
			return eCanFdResult_YES;
		}
	}
	return eCanFdResult_NO;
}

//bool LoadFirmwareData(char *pInputBuffer,unsigned char ucFirmwareType)
//{
//	if( pInputBuffer == NULL )
//	{
//	  __iar_dlmalloc_stats();
//	  	if( ucFirmwareType == CANFD_UPDATE_BOOT )	pInputBuffer = (char*)malloc(m_stAutolinkConfigData.stSystem.stExtBoardUpdateInfo.uiBootSize);
//		else										pInputBuffer = (char*)malloc(m_stAutolinkConfigData.stSystem.stExtBoardUpdateInfo.uiAppSize);
//		__iar_dlmalloc_stats();
//		if( pInputBuffer != NULL )
//		{
//			if( ucFirmwareType == CANFD_UPDATE_BOOT )	sFLASH_ReadBuffer((char *)pInputBuffer, SECTOR_EXBOARD_BOOT_FIRMWARE*SFLASH_SECTOR_SIZE, sizeof(m_stAutolinkConfigData.stSystem.stExtBoardUpdateInfo.uiBootSize) );
//			else										sFLASH_ReadBuffer((char *)pInputBuffer, SECTOR_EXBOARD_APP_FIRMWARE*SFLASH_SECTOR_SIZE, sizeof(m_stAutolinkConfigData.stSystem.stExtBoardUpdateInfo.uiAppSize) );
//			
//			return true;
//		}
//		return false;
//	}
//	else return false;
//}


eCANFD_BOARD_NEED_UPDATE NeedToUpdateCheck()
{
	eCANFD_BOARD_NEED_UPDATE eRet = eCANFD_BOARD_NEED_UPDATE_NONE;
	if( m_stCFDCtrl.stVer.usBlVer < m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion )
	{
		if( m_stCFDCtrl.stVer.usAppVer < m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion )
		{
			eRet = eCANFD_BOARD_NEED_UPDATE_BOTH;
		}
		else
		{
			eRet = eCANFD_BOARD_NEED_UPDATE_BOOT;
		}
	}
	else if( m_stCFDCtrl.stVer.usAppVer < m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion )
	{
		eRet = eCANFD_BOARD_NEED_UPDATE_APP;
	}

	return eRet;
}

eCANFD_BOARD_NEED_UPDATE NeedToUpdateCheck2()
{
	eCANFD_BOARD_NEED_UPDATE eRet = eCANFD_BOARD_NEED_UPDATE_NONE;

    if ( m_stCFDCtrl.stVer.usBlVer >= CFD_ARTERY_MCU_VERSION )
    {
        // STM CANFD어댑터버전으로 저장된 버전을 Artery버전으로 동기화 시킴. 
        if ( m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion < CFD_ARTERY_MCU_VERSION ||
            m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion < CFD_ARTERY_MCU_VERSION)
        {
            m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion = m_stCFDCtrl.stVer.usBlVer;
            m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion = m_stCFDCtrl.stVer.usAppVer;
        }
    }
    else
    {
        // Artery CANFD어댑터버전으로 저장된 버전을 STM버전으로 동기화 시킴.
        if ( m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion >= CFD_ARTERY_MCU_VERSION ||
             m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion >= CFD_ARTERY_MCU_VERSION )
        {
            m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion = m_stCFDCtrl.stVer.usBlVer;
            m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion = m_stCFDCtrl.stVer.usAppVer;
        }
    }
        //printf("1. g_bCanFD_FOTA_Check : %d g_Check_Update : %d\r\n",g_bCanFD_FOTA_Check,g_Check_Update );
        
    if(g_bCanFD_FOTA_Check == true)  //FOTA 진행 시 
    {
        
        if( g_Check_Update == eCANFD_UPDATE_INSTALLATION_BOOT)
        {
            eRet = eCANFD_BOARD_NEED_UPDATE_BOOT;
            g_Check_Update = eCANFD_UPDATE_INSTALLATION_NONE; 
        }
        else if(g_Check_Update == eCANFD_UPDATE_INSTALLATION_APP)
        {
            eRet = eCANFD_BOARD_NEED_UPDATE_APP;
            g_Check_Update = eCANFD_UPDATE_INSTALLATION_NONE;
        }
        //printf("2. g_bCanFD_FOTA_Check : %d g_Check_Update : %d\r\n",g_bCanFD_FOTA_Check,g_Check_Update );
    }
    else  //일반 부팅 
    {            
        if( m_stCFDCtrl.stVer.usBlVer < m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion )
        {
            eRet = eCANFD_BOARD_NEED_UPDATE_BOOT;
        }
        else if( m_stCFDCtrl.stVer.usAppVer < m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion )
        {
            eRet = eCANFD_BOARD_NEED_UPDATE_APP;
        }

    }


	return eRet;
}


boolean_t ExistCanID(unsigned int nCanID, unsigned char ucLine)
{
	//boolean_t bResult = false;
	int8_t ucNowCanIDCount = 0 ;
	ucNowCanIDCount =  m_stCFDCtrl.stCanIDMerge.ucMaskCount;

	if(ucNowCanIDCount == 0)
	{
		return false;
	}
	else
	{
		for(int i = 0 ; i < ucNowCanIDCount ; i++)
		{
			if(nCanID == m_stCFDCtrl.stCanIDMerge.nStartMask[i] && ucLine == m_stCFDCtrl.stCanIDMerge.ucLineIndex[i])
			{
				return true;
			}
		}
	}

	return false;
}


boolean_t AddMaskCanID(unsigned int nCanID , unsigned char ucLineInfo)
{
	int8_t ucCanIDNumber = 0;
	ucCanIDNumber = m_stCFDCtrl.stCanIDMerge.ucMaskCount;
	if(ucCanIDNumber == (CAN_ID_MERGE_MAX_CNT-1))
	{
		return false;
	}

	m_stCFDCtrl.stCanIDMerge.nStartMask[ucCanIDNumber] = nCanID;
	m_stCFDCtrl.stCanIDMerge.nEndMask[ucCanIDNumber] = nCanID;
	m_stCFDCtrl.stCanIDMerge.ucLineIndex[ucCanIDNumber] = ucLineInfo;
	m_stCFDCtrl.stCanIDMerge.ucMaskCount++;
	return true;
}


/*
extern stCanIDCheckList g_stCanIDCheckList;
extern unsigned char g_ucTotResCnt;
extern unsigned char g_ucTotReadyCnt

typedef __packed struct _stCanIDMergeList{
	int8_t  	 ucMaskCount;
	unsigned int nStartMask[CAN_ID_MERGE_MAX_CNT];
	unsigned int nEndMask[CAN_ID_MERGE_MAX_CNT];
}stCanIDMergeList;
*/

boolean_t MakeCanIDMergeList()
{
	unsigned int nCanId = 0;
	unsigned char ucLineInfo = CANFD_SPI_DEFAULT ;
	//3개의 ID를 통합 처리
	for(int i = 0 ; i < g_stCanIDCanFDUse.m_uiMaskCount ; i++)
	{
		nCanId = g_stCanIDCanFDUse.m_uiStartMask[i];
		
		if(ExistCanID(nCanId,g_ucSaveSlaveCanLine) == false)
		{
			if(AddMaskCanID(nCanId,g_ucSaveSlaveCanLine) == false)
			{
				Trace("[%s] MakeCanIDMergeList is Max Count : %d \r\n", __FUNCTION__,m_stCFDCtrl.stCanIDMerge.ucMaskCount);  
			}
		}
	}

	for(int i = 0 ; i < g_ucTotResCnt ; i++)
	{
		nCanId = g_stActuatorResData[i].m_uiResVal;
		ucLineInfo = g_stActuatorResData[i].m_ucFDCanLine;
		
		if(ExistCanID(nCanId,g_stActuatorResData[i].m_ucFDCanLine) == false)
		{
			if(AddMaskCanID(nCanId,ucLineInfo) == false)
			{
				Trace("[%s] g_stActuatorResData is Max Count : %d \r\n", __FUNCTION__,m_stCFDCtrl.stCanIDMerge.ucMaskCount);  
			}
		}		
	}

	for(int i = 0 ; i < g_ucTotReadyCnt ; i++)
	{
		nCanId = g_stActuatorReadyData[i].m_uiResVal;
		ucLineInfo = g_stActuatorReadyData[i].m_ucFDCanLine;
		
		if(ExistCanID(nCanId,g_stActuatorReadyData[i].m_ucFDCanLine) == false)
		{
			if(AddMaskCanID(nCanId,ucLineInfo) == false)
			{
				Trace("[%s] g_stActuatorReadyData is Max Count : %d \r\n", __FUNCTION__,m_stCFDCtrl.stCanIDMerge.ucMaskCount);  
			}
		}		
	}

#ifdef TEST_CAN_ID_MASK
	int nIDList[] = {0x041,0x042,0x043,0x044,0x04b,0x04C,0x04D,0x04E,0x061,0x062,
					 0x063,0x064,0x0A1,0x0A2,0x0A3,0x0B6,0x0B7,0x0B8,0x0B9,0x101,
					 0x102,0x103,0x104,0x11B,0x11C,0x126,0x127,0x128,0x12C,0x146,
					 0x147,0x148,0x149,0x156,0x157,0x161,0x162,0x166,0x167,0x168,
					 0x16B,0x16C,0x16D,0x16E,0x171,0x172,0x1A1,0x1A2,0x1A3,0x1A4
					 
					 };

	for(int i = 0 ; i < 0 ; i++)
	{
		nCanId = nIDList[i];
		ucLineInfo = 221;
		
		if(ExistCanID(nCanId) == false)
		{
			if(AddMaskCanID(nCanId,ucLineInfo) == false)
			{
				Trace("[%s] g_stActuatorReadyData is Max Count : %d \r\n", __FUNCTION__,m_stCFDCtrl.stCanIDMerge.ucMaskCount);  
			}
		}		
	}
#endif

//	DisplayMergeCanIDList();
	return true;
}

void DisplayMergeCanIDList()
{
	int8_t ucCount = 0 ;
	ucCount = m_stCFDCtrl.stCanIDMerge.ucMaskCount;

	for(int i = 0 ; i < ucCount ;i ++)
	{
		Trace("[%s] %d : %04X LineNumber : %d \r\n", __FUNCTION__,i,m_stCFDCtrl.stCanIDMerge.nStartMask[i],m_stCFDCtrl.stCanIDMerge.ucLineIndex[i]);  		
	}

	Trace("[%s] ucMaskCount : %d , CheckSum : %d \r\n", __FUNCTION__,GetMaskingCountForDB(),GetMaskingCheckSumForDB());  		
}
