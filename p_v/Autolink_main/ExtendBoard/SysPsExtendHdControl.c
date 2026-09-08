#if defined(FEATURE_EXTENSION_BOARD)

/**
******************************************************************************
* @file    SysPsExtendHdControl.c
* @author  WoongBae.Park
* @version V1
* @date    15-Aug-2019
* @brief   Main program body
******************************************************************************

******************************************************************************
*/

#include "SysPsExtendHdControl.h"
#include "SysPsExtendHdEvent.h"
#include "GIT_InterProtocol.h"
#include "HdDebug.h"
#include "HalHandler.h"
#include "HalFlashDriver.h"



#define Trace(...)  GITDebug(DEBUG_MODULES_EXTENSION,__VA_ARGS__)

#define MAX_EXTBD_PACKET_SIZE (500)

stPLUS_MODULE_CTRL	m_stPlusModuleSetData;
stGIT_PTCL_PAYLOAD	m_stPlusOutPtcl;
stSendFileStatus m_stSendFileStatus;

void SetExtendBoardPayloadMalloc(void)
{
	m_stPlusOutPtcl.pPayload = (unsigned char*)malloc(512);
}

void SetExtendBoardModuleSleep(void)
{
	stGIT_PTCL_PAYLOAD stPayload;
	stPayload.FunctionID = 0xA575;

	HalGPIOSetVaule(GPIO_USB_FS_DM, eBIT_RESET);
	SystemDelay(10);

	ExtendBoardModuleControlReq(0xA575, NULL, 0);
}

void SetExtendBoardModuleWakeup(void)
{
	HalGPIOSetVaule(GPIO_USB_FS_DM, eBIT_RESET);
	SystemDelay(10);
	HalGPIOSetVaule(GPIO_USB_FS_DM, eBIT_SET);
	SystemDelay(10);
}

void SetExtendBoardModuleReset(void)
{
	HalGPIOSetVaule(GPIO_USB_FS_DP, eBIT_SET);
	SystemDelay(10);
	HalGPIOSetVaule(GPIO_USB_FS_DP, eBIT_RESET);
	SystemDelay(10);
}

void ExtendBoardSerialReq(void)
{
	ExtendBoardModuleControlReq(0xA303, NULL,0);
}

void ExtendBoardFWInfoReq(void)
{
	ExtendBoardModuleControlReq(0xA145, NULL, 0);
}

void ExtendBoardHiPassPaymentCntReq(void)
{
	//ExtendBoardModuleControlReq(0xA006, NULL, 0); 구현 피룡
}

void ExtendBoardHiPassPaymentDataReq(void)
{
	ExtendBoardModuleControlReq(0xA009, NULL, 0);
}

void ExtendBoardHiPassPowerOnReq(void)
{
	ExtendBoardModuleControlReq(0xA00A, NULL, 0);
}

void ExtendBoardHiPassPowerOffReq(void)
{
	ExtendBoardModuleControlReq(0xA00B, NULL, 0);
}

void ExtendBoardFobKeyPowerControlReq(unsigned char ucFobKeyPower)
{
	unsigned char ucFobKeyPowerControl = ucFobKeyPower;
    // ucFobKeyPowerControl== 0 : FOB On, =< 1 : FOB off
	ExtendBoardModuleControlReq(0xC001, &ucFobKeyPowerControl, sizeof(ucFobKeyPowerControl));
}

void ExtendBoardFobKeyStatusReq()
{
	ExtendBoardModuleControlReq(0xC002, NULL, 0);
}

void ExtendBoardFotaStartReq(void *pPayload)
{
	stExtBDFWFileInfo stExtBDFWFileInfo;

	memset(&stExtBDFWFileInfo, 0, sizeof(stExtBDFWFileInfo));
	memcpy(&stExtBDFWFileInfo, pPayload, sizeof(stExtBDFWFileInfo));

	m_stSendFileStatus.usFileSize = stExtBDFWFileInfo.uiDB_Size;
	m_stSendFileStatus.usSendCnt = 0;
	m_stSendFileStatus.usLeftByte = stExtBDFWFileInfo.uiDB_Size;
	m_stSendFileStatus.uiFileStartAddress = stExtBDFWFileInfo.uiFileStartAddress;
	m_stSendFileStatus.uiFileEndAddress = stExtBDFWFileInfo.uiFileEndAddress;

	if(m_stSendFileStatus.usFileSize/MAX_EXTBD_PACKET_SIZE == 0)
	{
		m_stSendFileStatus.usLoopTotalCnt = m_stSendFileStatus.usFileSize/MAX_EXTBD_PACKET_SIZE;
	}
	else
	{
		m_stSendFileStatus.usLoopTotalCnt = (m_stSendFileStatus.usFileSize/MAX_EXTBD_PACKET_SIZE)+1;
	}

	ExtendBoardModuleControlReq(0xA175, (unsigned char*)&stExtBDFWFileInfo, sizeof(stExtBDFWFileInfo));
}

void ExtendBoardFotaReq(void)
{
	stDownloadingFile stDownloadingFile;
	m_stSendFileStatus.usSendCnt++;

	if(m_stSendFileStatus.usLeftByte - MAX_EXTBD_PACKET_SIZE >= 0)
	{
		m_stSendFileStatus.usLeftByte = m_stSendFileStatus.usFileSize - MAX_EXTBD_PACKET_SIZE;
		stDownloadingFile.Frame_Size = MAX_EXTBD_PACKET_SIZE;
	}
	else
	{
		stDownloadingFile.Frame_Size = m_stSendFileStatus.usLeftByte;
	}

	HalDrvFlashReadByteCallByRef((uint32_t*)&m_stSendFileStatus.uiFileStartAddress, (BYTE*)stDownloadingFile.DB_Data, stDownloadingFile.Frame_Size);

	m_stSendFileStatus.uiFileStartAddress += stDownloadingFile.Frame_Size;

	ExtendBoardModuleControlReq(0xA275, (unsigned char *)&stDownloadingFile,sizeof(stDownloadingFile));

//	INT16U Frame_Size;				/* 전송하는 Frame Size */
//	INT8U DB_Data[MAX_SIZE_DOWNLOADING_BUFF];	/* 최대 500byte */

}

void ExtendBoardFotaEndReq()
{
	ExtendBoardModuleControlReq(0xA375, NULL, 0);
}

void ExtendBoardReset()
{
	ExtendBoardModuleControlReq(0xA475, NULL,0);
}

void ExtendBoardModuleControlReq(uint16_t usFunctionID, unsigned char * ucPayload, unsigned char ucDataLength)
{
	stGIT_PTCL_PAYLOAD *pstPlusOutPtcl;
	eCommType	eInCommType=eCOMM_TYPE_UART_PLUSBD;

	pstPlusOutPtcl = (stGIT_PTCL_PAYLOAD *)&m_stPlusOutPtcl;

	pstPlusOutPtcl->FunctionID = usFunctionID;
	pstPlusOutPtcl->DataLength = ucDataLength;

	//memcpy(pstPlusOutPtcl->pPayload, ucPayload, ucDataLength);
#if false
	Trace("\n------------------------------------------------------\n");
	Trace("\n------------------------------------------------------\n");
	Trace("\n------------------------------------------------------\n");
	Trace("EXT] Control Type : %x\n",usFunctionID);
	Trace("EXT] Control Length : %x\n",ucDataLength);
#endif

	SendGITPtclResponse(pstPlusOutPtcl, ucPayload, ucDataLength, eInCommType);
}

void GetFOTASendBinCnt(stSendFileStatus *pstSendFileStatus)
{
	memcpy(pstSendFileStatus, &m_stSendFileStatus,sizeof(pstSendFileStatus));
}

void PlusModuleSetDataInit()
{
	m_stPlusModuleSetData.m_ucEmergencyLamp_Ctrl  		= 0;						/* Emergency Lamp 제어 (GPIO) */
	m_stPlusModuleSetData.m_nEmergencyLamp_Time 		= 0;						/* Emergency Lamp 동작시간 */
	m_stPlusModuleSetData.m_ucLock_Ctrl					= 0;						/* Lock/Unlock제어*/
	m_stPlusModuleSetData.m_ucLock_Select				= 0;						/* Lock/Unlock 선택 */
	m_stPlusModuleSetData.m_ucHorn_Ctrl					= 0;						/* Horn 제어 (GPIO) */
	m_stPlusModuleSetData.m_nHorn_Time					= 0;						/* Horn 제어 시간 */
	m_stPlusModuleSetData.m_nHorn_Gap					= 0;						/* Horn 울림 간격 */
}

void LockUnlockSet(uint8_t ucSelect)
{
	PlusModuleSetDataInit();

	if(ucSelect == eExtBD_DoorOpenReq)
	{
		m_stPlusModuleSetData.m_ucEmergencyLamp_Ctrl  			= LAMP_SET;			/* Emergency Lamp 제어 (GPIO) */
		m_stPlusModuleSetData.m_nEmergencyLamp_Time 			= 1000;						/* Emergency Lamp 동작시간 */
		m_stPlusModuleSetData.m_ucLock_Ctrl					  	= LOCK_SET;			/* Lock/Unlock제어*/
		m_stPlusModuleSetData.m_ucLock_Select					= 1;						/* Lock/Unlock 선택 */
		m_stPlusModuleSetData.m_ucHorn_Ctrl						= 0;			/* Horn 제어 (GPIO) */
		m_stPlusModuleSetData.m_nHorn_Time						= 0;						/* Horn 제어 시간 */
		m_stPlusModuleSetData.m_ucTrunk_Ctrl					= 0;
	}
	else if(ucSelect == eExtBD_DoorCloseReq)
	{
		m_stPlusModuleSetData.m_ucEmergencyLamp_Ctrl  				= LAMP_SET;			/* Emergency Lamp 제어 (GPIO) */
		m_stPlusModuleSetData.m_nEmergencyLamp_Time 				= 1000;						/* Emergency Lamp 동작시간 */
		m_stPlusModuleSetData.m_ucLock_Ctrl					  		= LOCK_SET;			/* Lock/Unlock제어*/
		m_stPlusModuleSetData.m_ucLock_Select						= 0;						/* Lock/Unlock 선택 */
		m_stPlusModuleSetData.m_ucHorn_Ctrl							= 0;			/* Horn 제어 (GPIO) */
		m_stPlusModuleSetData.m_nHorn_Time							= 0;						/* Horn 제어 시간 */
		m_stPlusModuleSetData.m_ucTrunk_Ctrl						= 0;
	}

	ExtendBoardModuleControlReq(0xA002, (unsigned char*)&m_stPlusModuleSetData, sizeof(m_stPlusModuleSetData));
}

void LampSet(uint8_t ucSelect,uint8_t ucTime)
{
	PlusModuleSetDataInit();
	if(ucSelect == eExtBD_lampOnReq)
	{
		m_stPlusModuleSetData.m_ucEmergencyLamp_Ctrl		= 1; 						/* Emergency Lamp 제어 (GPIO) */
		m_stPlusModuleSetData.m_nEmergencyLamp_Time			= ucTime; 					//20170309 - 기본 1초로 세팅 처리
		m_stPlusModuleSetData.m_ucLock_Ctrl					= 0;						/* Lock/Unlock제어*/
		m_stPlusModuleSetData.m_ucLock_Select 				= 0;						/* Lock/Unlock 선택 */
		m_stPlusModuleSetData.m_ucHorn_Ctrl					= 0;						/* Horn 제어 (GPIO) */
		m_stPlusModuleSetData.m_nHorn_Time					= 0;						/* Horn 제어 시간 */
	}
	else if(ucSelect == eExtBD_lampOffReq)
	{
		PlusModuleSetDataInit();
		m_stPlusModuleSetData.m_ucEmergencyLamp_Ctrl=1;
		m_stPlusModuleSetData.m_nEmergencyLamp_Time=0;
		m_stPlusModuleSetData.m_ucLock_Ctrl					  		= 0;			/* Lock/Unlock제어*/
		m_stPlusModuleSetData.m_ucLock_Select						= 0;			/* Lock/Unlock 선택 */
		m_stPlusModuleSetData.m_ucHorn_Ctrl							= 0;			/* Horn 제어 (GPIO) */
		m_stPlusModuleSetData.m_nHorn_Time							= 0;			/* Horn 제어 시간 */
	}

	ExtendBoardModuleControlReq(0xA002, (unsigned char*)&m_stPlusModuleSetData, sizeof(m_stPlusModuleSetData));
}

void HornOn(uint32_t uiTime, uint32_t uiGap)
{
	PlusModuleSetDataInit();
	m_stPlusModuleSetData.m_ucHorn_Ctrl=1;
	m_stPlusModuleSetData.m_nHorn_Time=uiTime;
	m_stPlusModuleSetData.m_nHorn_Gap=uiGap;
	ExtendBoardModuleControlReq(0xA002, (unsigned char*)&m_stPlusModuleSetData, sizeof(m_stPlusModuleSetData));
}

#endif //#if defined(FEATURE_EXTENSION_BOARD)
