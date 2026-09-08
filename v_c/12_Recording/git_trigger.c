/*************************************************************
 * NOTE : git_record.c
 *      
 * Author : 
 * Since : 2019.12.18
**************************************************************/
#include <string.h>

#include "typedef.h"

#include "git_PassthruDefines.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "common.h"
#include "firmware.h"
#include "git_protocol.h"
#include "sw_timer.h"
#include "buzzer.h"
#include "git_fsutil.h"
#include "git_rtc.h"
#include "git_ioctl.h"

#include "git_function_list.h"


#include "git_record.h"
#include "git_record_file.h"
#include "git_trigger.h"

#include "ff.h"

#include "git_rs9116.h"

#include "led.h"

#include "Git_pm.h"


#define MAX_RPM_ZERO_COUNT (3)
#define MAX_RPM_COMM_FAIL_COUNT (3)


#define TMP_BUFF_SIZE (1024)
#define MAX_CONFIG_TMP_BUFFER_LENGTH (3*1024)


extern eLockState 	g_eLockStatus;
extern osMessageQId	hTransmitMsg;
#ifdef PRINT_MESSAGE_ID
	extern stMESSAGE_ID_INFO stMessageIdInfo[20];
 	extern uint8_t ucMessageIdInfoCnt;
#endif

#define MESSAGE_TRIGGER_APP_QUEUE_SIZE  (10)


// relates with trigger module
#define MAX_TRIGGER_MODULE_VERSION_LENGTH (12)

#define MAX_WAIT_CONFIG_TIME_OUT (30*1000)

#define MAX_BLUETOOTH_ALIVE_TIME_OUT (1*1000)
#define MAX_WAIT_BLUETOOTH_ALIVE_TIME_OUT (2*100)
#define MAX_BLUETOOTH_ALIVE_TIME_OUT_COUNT (30)
#define MAX_WAIT_TRIGGER_MONITOR_TIME_OUT (10*1000)
#define MAX_WAIT_TRIGGER_WAIT_TIME_OUT (1*1000)



// Thread
static void TriggerTh( void const * argument );

static osThreadId	hTriggerAppTh;
osThreadDef( triggerappth,	TriggerTh,	osPriorityBelowNormal, 0, 2 * configMINIMAL_STACK_SIZE );

// Message
osMessageQId	hTriggerAppMsg;
osMessageQDef( triggerappqueue, MESSAGE_TRIGGER_APP_QUEUE_SIZE, uint32_t );

stTriggerControl m_stTrgCtrl;


extern uint8_t	GetBluetoothConnectionStatus();


//###############################################################################################
//###############################################################################################
//###############################################################################################
bool StartTriggerThread( void )
{
	hTriggerAppMsg = osMessageCreate( osMessageQ( triggerappqueue ), NULL );
	if( hTriggerAppMsg == NULL )
	{
		GLogE( "error... osMessageCreate hTriggerAppMsg\r\n" );
		return false;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hTriggerAppMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hTriggerAppMsg",sizeof("hTriggerAppMsg"));
#endif


	hTriggerAppTh = osThreadCreate( osThread(triggerappth), NULL );
	if( hTriggerAppTh == NULL )
	{
		GLogE( "Error... fail create record threads!!!\r\n" );
                return false;
	}
        
        return true;
}




void vTaskStartTrace( 	portCHAR * pcBuffer, unsigned portLONG ulBufferSize )
{
	printf("%s] size : %d\r\n", __func__, ulBufferSize);
}

unsigned portLONG ulTaskEndTrace(	void )
{

}

#if (configGENERATE_RUN_TIME_STATS == 1 )

unsigned long ulHighFrequencyTimerTicks;

void vConfigureTimerForRunTimeStats( void )
{
	ulHighFrequencyTimerTicks = 0UL;
}
unsigned long vConfigureRunTimeValue( void )
{
	return ulHighFrequencyTimerTicks;
}

#endif

void vApplicationIdleHook( void )
{
	static uint32_t s_ntime = 0;

	if( Get_Tmr()-s_ntime > 500)
	{

		static int task_monitor(void);

		task_monitor();
		s_ntime = Get_Tmr();
	}
}

uint8_t s_buffer[1024*5];
static int task_monitor(void)
{
	
#if false
	if (osOK == osThreadList(s_buffer))
	{
		printf("\r\n");
		printf("Name          State  Priority  Stack   Num\r\n");
		printf("******************************************\r\n");
		printf((char *)s_buffer);
		printf("******************************************\r\n");
		printf("Free Heap: %d\r\n", xPortGetMinimumEverFreeHeapSize());
	}
#endif

	vTaskGetRunTimeStats(s_buffer);
	printf("\r\n");
	printf("Name		  State  Stack	RunTime\r\n");
	printf("******************************************\r\n");
	printf((char *)s_buffer);
	printf("******************************************\r\n");
	printf("HEAP Minimum Heap: %d, free Heap : %d\r\n", xPortGetMinimumEverFreeHeapSize(), xPortGetFreeHeapSize());


}

//###############################################################################################
//###############################################################################################
//###############################################################################################




/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/

osThreadId	GetTriggerHandle()
{
	return hTriggerAppTh;
}


void ClearTriggerHandler()
{
	m_stTrgCtrl.eState = eTCS_Init;
}

void SetCurTriggerModuleTriggerButton(uint8_t* ucTriggerButtonStatus)
{
	m_stTrgCtrl.bActiveRecordButton = true;
}


void SetCurTriggerModuleInfo(uint8_t* ucModuleInfo)
{
	//[0] : current mode
	//[1] : module type
	//[2] : app f/w version(12bytes)	
	m_stTrgCtrl.stTrgCtrl.ucCurMode = ucModuleInfo[0];
	m_stTrgCtrl.stTrgCtrl.ucType = ucModuleInfo[1];

	memcpy(&m_stTrgCtrl.stTrgCtrl.ucVersion[0], &ucModuleInfo[2], MAX_TRIGGER_MODULE_VERSION_LENGTH);


	m_stTrgCtrl.bSendModeChanged = true;
}


void ClearBluetoothReponseSignal()
{
	//clear signal setting
	//MONI 20230213 suresoft defect num : 33 
	for(int i=0;i<3;i++)
	{
		osEvent event = osSignalWait(SIGNAL_TRIGGER_ALIVE_RESPONSE, 100);
		if( event.status != osEventSignal )
		{
			break;
		}
		
		osDelay(1000);
	}
}


void SendSignalBluetoothAlive()
{
	// set signal to tirgger for bluetooth response
	osSignalSet(GetTriggerHandle(), SIGNAL_TRIGGER_ALIVE_RESPONSE);
}


void SetCurTriggerModuleLedObdStatus()
{
	m_stTrgCtrl.bDlyReqObdLed = false;
}


void SetCurTriggerModuleConnection()
{
	// execeptino case
	// tirgger module reponse with request version -> set connection mode.
	SendEvent2TriggerThread(eEVT_TRG_CONNECTED);
}


void SetCurTriggerModuleButtonStatus(uint8_t* pucButtonStatus)
{
	SendEvent2TriggerThread(eEVT_TRG_BTN_ACTIVE);

	if( m_stTrgCtrl.stMonitoringCtrl.eState == eTrgMonitoring_WaitTimeOut )
	{

		// set signal to tirgger for bluetooth response
		osSignalSet(GetTriggerHandle(), SIGNAL_TRIGGER_BTN_EVENT);
	}


	if( m_stTrgCtrl.bBtConnected == true )
	{
		// send respose 
		SendByteEvent2TriggerModule(eEVT_TM_CTRL_REC_BTN_STATUS,eTM_ACK);
	}
}


void SetUsbTriggerState(bool bActive)
{
	m_stTrgCtrl.bUsbConnected = true;

	m_stTrgCtrl.bUsbBtnPressed = true;
}


int16_t GetTrigFunctionId(eTriggerModuleEvent eEvent)
{
	uint16_t usFunctionId = 0;
	
	switch(eEvent)	
	{
		case eEVT_TM_CTRL_LED_OBD_CCP:					
			usFunctionId = 0xD001;
			break;
		case eEVT_TM_CTRL_REC_BTN_STATUS:
			usFunctionId = 0xD002;
			break;
		case eEVT_TM_CTRL_SLEEP:
			usFunctionId = 0xD003;
			break;
		case eEVT_TM_CTRL_VER_INFO:
			usFunctionId = 0xD004;
			break;
		case eEVT_TM_CTRL_FW_UPDATE_START:
			usFunctionId = 0xD005;
			break;
		case eEVT_TM_CTRL_FW_UPDATE_WRITE:
			usFunctionId = 0xD006;
			break;
		case eEVT_TM_CTRL_FW_CHECKSUM:
			usFunctionId = 0xD007;
			break;
		case eEVT_TM_CTRL_FW_UPDATE_CLOSE:
			usFunctionId = 0xD008;
			break;
		case eEVT_TM_CTRL_SW_APP_LIST_RCV:
			usFunctionId = 0xD009;
			break;
		case eEVT_TM_CTRL_SW_APP_LIST_SEND:
			usFunctionId = 0xD00A;
			break;
		case eEVT_TM_CTRL_LED_GREEN:
			usFunctionId = 0xD00B;
			break;
		case eEVT_TM_CTRL_CUR_MODE_INFO:
			usFunctionId = 0xD00C;
			break;
		case eEVT_TM_CTRL_FW_MODE_CHANGE:
			usFunctionId = 0xD00D;
			break;
		case eEVT_TM_CTRL_TRIGGER:
			usFunctionId = 0xD00E;
			break;
		case eEVT_TM_CTRL_SET_SERIAL_NUMBER:
			usFunctionId = 0xD02F;
			break;
		case eEVT_TM_CTRL_GET_SERIAL_NUMBER:
			usFunctionId = 0xD030;
			break;
		case eEVT_TM_CTRL_SELFTEST_LED_BLINK_DELAY_TIME:
			usFunctionId = 0xD03E;
			break;
		case eEVT_TM_CTRL_SELFTEST_START:
			usFunctionId = 0xD03F;
			break;
	}	

	return usFunctionId;
}


void SendByteEvent2TriggerModule(eTriggerModuleEvent eEvent, uint8_t ucValue)
{
	// send event 
	stCommPkt	*packet;
	stMsgClst	*message;

	uint32_t eInCommType = PACKET_UART;
    
	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		GLogEE( "%s] Fail... hMsgPool Alloc!!!\r\n", __func__);
		return;
	}

	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		GLogEE( "%s] Fail... hCommPKPool Alloc!!!\r\n", __func__);
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
	
	//GLogI("*");

	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = GetTrigFunctionId(eEvent);

	packet->mData[0] = ucValue;
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);
	
	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	while( osMessageAvailableSpace( hTransmitMsg ) == 0 )
	{
		osDelay( 1 );
	}

	osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );

	//hexdump((uint8_t*)message,	sizeof(message));


	//GLogI("%s] send spp message to trigger\r\n", __func__);
}

void SendEvent2TriggerModule(eTriggerModuleEvent eEvent, uint8_t* pucPayload, uint8_t ucPayloadLength)
{
	// send event 
	stCommPkt	*packet;
	stMsgClst	*message;

	uint32_t eInCommType = PACKET_UART;
    
	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		GLogEE( "%s] Fail... hMsgPool Alloc!!!\r\n", __func__);
		return;
	}

	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		GLogEE( "%s] Fail... hCommPKPool Alloc!!!\r\n", __func__);
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
	
	GLogI("*");

	packet->mLen	= ucPayloadLength;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = GetTrigFunctionId(eEvent);

	if( ucPayloadLength > 0 )
		memcpy(&packet->mData[0], pucPayload, ucPayloadLength);
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);
	
	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	while( osMessageAvailableSpace( hTransmitMsg ) == 0 )
	{
		osDelay( 1 );
	}

	osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );

	//hexdump((uint8_t*)message,	sizeof(message));


	GLogI("%s] send spp message to trigger\r\n", __func__);
}


void SendEvent2TriggerThread(eEvtTrigger eEvt)
{   
	GLogI("*");

	if(osMessageAvailableSpace(hTriggerAppMsg) == 0)
	{
		GLogE("%s] error not enough memory\r\n", __func__);
	}
	else
	{
		osMessagePut( hTriggerAppMsg, (uint32_t)eEvt, osWaitForever );
	}	
}

eTrgMonitoringMode GetTrgMonitorMode(uint8_t ucMode)
{
	eTrgMonitoringMode eMode = eTrgMonitorMode_Menual;
	
	switch(ucMode)
	{
		case 1:
			eMode = eTrgMonitorMode_Menual;			
			break;
		case 2:
			eMode = eTrgMonitorMode_Dtc;
			break;
		case 4:			
			eMode = eTrgMonitorMode_EngineStart;
			break;
		case 8:
			eMode = eTrgMonitorMode_EngineStop;
			break;
		default:
			eMode = eTrgMonitorMode_Menual;
			break;
	}

	return eMode;
}


eTrgMonitoringTime GetTrgMonitorTime(uint8_t ucTime)
{
	eTrgMonitoringTime eTime = eTrgMonitorTime_10m;
	switch(ucTime)
	{
		case 1:
			eTime = eTrgMonitorTime_10m;
			break;
		case 2:
			eTime = eTrgMonitorTime_30m;
			break;			
		case 3:
			eTime = eTrgMonitorTime_60m;
			break;
		default:
			eTime = eTrgMonitorTime_10m;
			break;
	}

	return eTime;
}


bool UpdateTriggerInfo()
{
	memset(&m_stTrgCtrl.stMonitoringCtrl,0,sizeof(stTriggerMonitoringInfo));

	if( GetRecordingConfigStatus() == false )
	{
		return false;
	}
	
	GetTriggerConfig(&m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo);

	GetEngineConfig(&m_stTrgCtrl.stMonitoringCtrl.stEngineCtrl.stEngStopInfo);

	GetEcuConfig(&m_stTrgCtrl.stEcuInfo);


	int nCodeLength = strlen(m_stTrgCtrl.stMonitoringCtrl.stEngineCtrl.stEngStopInfo.strRequestCode);
	if( nCodeLength >= 4 )
	{
		printf("enginecontrol is 1\r\n");
		m_stTrgCtrl.stMonitoringCtrl.stEngineCtrl.ucCount = 1;
	}
	
	// setting trigger monitoring mode
	m_stTrgCtrl.stMonitoringCtrl.ucMode = m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo.ucTrigMode;

	// setting recording time
	m_stTrgCtrl.stMonitoringCtrl.eTime = GetTrgMonitorTime(m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo.ucRecordTime);

	GLogI("%s] trigger mode : %d, time : %d\r\n", __func__, m_stTrgCtrl.stMonitoringCtrl.ucMode, m_stTrgCtrl.stMonitoringCtrl.eTime);
	GLogI("%s] stTrgInfo.ucDtcReqCodeCount : %d\r\n", __func__, m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo.ucDtcReqCodeCount);
	//GLogI("%s] \r\n", __func__, );

	return true;
}

void IntializeTriggerControlInfo()
{
	// send trigger dtc data to diagnostic message 
	m_stTrgCtrl.bConnected = false;
	m_stTrgCtrl.bBtConnected = false;
	
	m_stTrgCtrl.bDlyReqObdLed = true;	
	m_stTrgCtrl.bActiveRecordButton = false;
}

void SetDtcTriggerState(eTrgMonitoringStates eState)
{
	m_stTrgCtrl.stMonitoringCtrl.eState = eState;
}


void MakeDtcMessage(uint8_t ucIndex, stTriggerInfo* pstTrgInfo, PTmsgPkt_t* pPTpacket)
{
	int32_t nLength = pstTrgInfo->ucDtcReqCodes[ucIndex][0];
	uint8_t ucBuffer[64] = {0,};

	memcpy(ucBuffer, &pstTrgInfo->ucDtcReqCodes[ucIndex][1], nLength);	

	CovertBuffer2PassThruMessage(ucBuffer, nLength, &pPTpacket);
}


bool SendTriggerPassThrouMessage(PTmsgPkt_t* pPTpacket)
{
	// send event to diagnostic thread
	//SendRecordingMessage2DiagThread(m_stRecordingControl.stCanCtrl.pPTpacket[0]);
	SendRecordingMessage2DiagThread(pPTpacket);

	// wait event for result
	if( WaitDiagThreadResponsofRecordingSystem(&m_stTrgCtrl.stMonitoringCtrl.stRxPacket) == false )
	{
		GLogE("%s] error send record message\r\n",__func__);
		return false;
	}

	if( (m_stTrgCtrl.stMonitoringCtrl.stRxPacket.mLen - MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER) == 0 )
		return false;

	return true;
}

uint8_t GetTriggerDtcData(char* pcData)
{
	uint8_t ucDataSize = 0;
	if( m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex > 0 )
	{
		memset(pcData, 0, MAX_TRIGGER_DTC_LENGTH);
		memcpy(pcData, m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock, m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex+1);

		ucDataSize = m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex;
	}

	return 0;
}

void ClearTriggerDtcData()
{
	m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex = 0;
	memcpy(m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock, 0, MAX_TRIGGER_DTC_LENGTH);
}

int32_t TriggerMatchingHandler(uint32_t unProtocolType, stCommPkt* pstRxPacket, stTriggerInfo* pstTrgInfo)
{
	// length(1) | id1(1) | id2(1) | service type1(1) | service type2(1) | data ||
	// data : 04 07 e8 92 02
	//int nDtcLength = (pstRxPacket->mLen - MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER) - (pstTrgInfo->ucDtcStartPos - 1);
	int nDtcLength = pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER-8];

	//MONI 20230419 static analysis num : 26 / for checking devide by 0
	if( pstTrgInfo->ucDtcReadNo == 0 && pstTrgInfo->ucDtcSkipNo == 0)
	{
		printf("%s] error db not configured\r\n", __func__);
		return 0;
	}

	
	int nDtcCount = ( nDtcLength-(pstTrgInfo->ucDtcStartPos - 1)) / (pstTrgInfo->ucDtcReadNo + pstTrgInfo->ucDtcSkipNo );

	printf("%s] nDtcLength : %d, nDtcCount : %d\r\n", __func__, nDtcLength, nDtcCount);

	printf("%s] dtc data \r\n", __func__);
	hexdump(&pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER], (pstRxPacket->mLen - MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER) );

	// exception case
	if( nDtcLength < 0 )
	{
		printf("%s] error length is weired : %d\r\n", __func__, nDtcLength);
		return 0;
	}

	// setting first setting info
	if( m_stTrgCtrl.stMonitoringCtrl.bDtcActive == false )
	{
		m_stTrgCtrl.stMonitoringCtrl.bDtcActive = true;
		m_stTrgCtrl.stMonitoringCtrl.ucPreDtcCount = nDtcCount;

		printf("%s] first dtc count : %d\r\n", __func__, nDtcCount);
		return 0;
	}

	// check dtc count increase or there is no dtc count
	if( m_stTrgCtrl.stMonitoringCtrl.ucPreDtcCount == nDtcCount || nDtcCount == 0 )
	{
		printf("%s] dtc count isn't changed : %d, do nothing\r\n", __func__, nDtcCount);
		// dtc count didn't changed do nothing
		return 0;
	}

	if( (m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex + nDtcLength) >= MAX_TRIGGER_DTC_LENGTH )
	{		
		GLogE("%s] error dtc buffer will be full, adjust dtc length\r\n", __func__);
		nDtcLength = MAX_TRIGGER_DTC_LENGTH - m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex;
		
		int Q = (pstTrgInfo->ucDtcReadNo + pstTrgInfo->ucDtcSkipNo );
		nDtcCount = nDtcLength/Q;
		nDtcLength = nDtcCount * Q;

		printf("%s] adjust dtc count : %d, length : %d\r\n", __func__, Q, nDtcLength);
	}
	
	if( unProtocolType == ISO9141_2 ||
		unProtocolType == ISO9141_2_SyncTime ||
		unProtocolType == ISO9141_2_DW_ABS ||
		unProtocolType == ISO14230 ||
		unProtocolType == ISO14230_ETC ||
		unProtocolType == ISO14230_POWERTEC ||
		unProtocolType == ISO9141_2_5BPSTXONLY ||
		unProtocolType == ISO14230_LLINE_LOW ||
		unProtocolType == ISO9141_2_DW_SIEMENSE||
		unProtocolType == ISO9141_2_00D1 )
	{			
		if( unProtocolType == ISO14230 || 
			unProtocolType == ISO14230_POWERTEC || 
			unProtocolType == ISO14230_ETC || 
			unProtocolType == ISO14230_LLINE_LOW)
		{
			if( pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-3]==0x58 ||
		 		pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-3]==0x59 ||
		 		pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-3]==0x53 ||
		 		pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-3]==0x57 ||
		 		pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-3]==0x43 ||
		 		pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-3]==0x47 ||
		 		pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-3]==0x7F)
	 		{
	 			if(pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-2]==0x00 || 
					pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-3]==0x7F)
	 			{
					pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-1]=0x00;
					pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos]  =0x00;							 				
	 			}
	 		}			
		}
		// MONI 20230213, suresoft defect num 36 : bug fixed
		if( unProtocolType == ISO14230_POWERTEC )
		{
			GLogI("%s] powertec dtc data copy\r\n", __func__);
			if( m_stTrgCtrl.stMonitoringCtrl.ucPreDtcCount < pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-2] )
			{
				// set size
				m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock[0] = nDtcLength;

				// copy dtc
				for(int i=0;i< nDtcLength; i++)
				{
					m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock[1+i] = pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + i];
				}

				// copy length
				m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex += nDtcLength;
			}

			m_stTrgCtrl.stMonitoringCtrl.ucPreDtcCount = pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-2];
		}
		else
		{
			
			if( (pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-1] + 
				pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos]) != 0x00 )
			{
				// set size
				m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock[0] = nDtcLength;
				
				for(int i=0;i< nDtcLength; i++)
				{
					m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock[1+i] = pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + i];
				}

				m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex += nDtcLength;
			}
			else
			{
			}
		}
	}
	else if( unProtocolType == ISO9141_BOSCH_AIRBAG ||
			unProtocolType == ISO9141_BOSCH ||
			unProtocolType == WABCO_ABS
			//unProtocolType == NISSAN_TxRx ||
			)
	{
		GLogI("%s] boschs dtc data copy\r\n", __func__);
		if( pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + pstTrgInfo->ucDtcStartPos-1] != 0x00 )
		{
			// set size
			m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock[0] = nDtcLength;
			
			for(int i=0;i< nDtcLength; i++)
			{
				m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock[1+i] = pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + i];
			}

			m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex += nDtcLength;
		}
	}
	else
	{
		GLogI("%s] defualt dtc data copy\r\n", __func__);

		// copy new dtc data		
		for(int i=0;i< nDtcLength; i++)
		{
			m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock[1 + m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex + i] = pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER + i];
		}

		m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex += nDtcLength;
		m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock[0] = m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex;

		m_stTrgCtrl.stMonitoringCtrl.ucPreDtcCount = nDtcCount;
	}	

	GLogI("%s] dtc data \r\n", __func__);
	hexdump(m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock, m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex);

	return nDtcCount;
}


void DtcMonitoringHanlder()
{
	GLogI("%s] dtc request start\r\n", __func__);
	
	for(int i=0; i<m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo.ucDtcReqCodeCount; i++)
	{

		GLogI("%s] make dtc reuest\r\n", __func__);
		//MakeDtcMessage(m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.ucDtcCurIndex, &m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo, &m_stTrgCtrl.stMonitoringCtrl.stTxPacket);
		MakeDtcMessage(i, &m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo, &m_stTrgCtrl.stMonitoringCtrl.stTxPacket);

		//hexdump(&m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo.ucDtcReqCodes[0],MAX_REQCODE_SIZE);
		//hexdump((uint8_t*)&m_stTrgCtrl.stMonitoringCtrl.stTxPacket.pData, m_stTrgCtrl.stMonitoringCtrl.stTxPacket.DataSize);

		if( SendTriggerPassThrouMessage(&m_stTrgCtrl.stMonitoringCtrl.stTxPacket) == true )
		{
			GLogI("dtc tringgering monitoring ok\r\n");

			// save dtc data for recording file
			//hexdump(&m_stTrgCtrl.stMonitoringCtrl.stRxPacket.mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER], m_stTrgCtrl.stMonitoringCtrl.stRxPacket.mLen-MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER);
		
			// check dtc code with trigger info
			if( TriggerMatchingHandler(m_stTrgCtrl.stEcuInfo.unProtocolType,&m_stTrgCtrl.stMonitoringCtrl.stRxPacket, &m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo) > 0 )
			{
				// set next dtc index			
				//m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.ucDtcCurIndex++;
			
				SendEvent2RecordThread(eEVT_REC_DTC_TRIGGER_START, m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock, m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex);
			}
		}
		else
		{
			GLogE("%s] error rx fail\r\n", __func__);
		}	
		
		if( GetActiveFlightRecording() == false )	
		{
			return;
		}
	}
}

void MakeEngineStallMessage(char* pucRequestCode, PTmsgPkt_t* pPTpacket)
{
	uint8_t ucBuffer[64] = {0,};
	uint32_t nLength = 0;
	for(int i=0;i<10;i++)
	{
		ucBuffer[i] = AsciiToHex(pucRequestCode[i*2],pucRequestCode[i*2+1]);
	}

	// ucBuffer[2] is service data length 
	// 3 is can id 2bytes + length 1byte
	nLength = ucBuffer[2]+3;

	CovertBuffer2PassThruMessage(ucBuffer, nLength, &pPTpacket);
}

void EngineMatchingHandler(uint32_t unProtocolType, stCommPkt* pstRxPacket, stTriggerInfo* pstTrgInfo)
{
	int nDtcLength = (pstRxPacket->mLen - MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER);

	if( (m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex + nDtcLength) > MAX_TRIGGER_DTC_LENGTH )
	{
		GLogE("%s] error dtc buffer is full\r\n", __func__);
		return;
	}
	
	GLogI("%s] dtc data \r\n", __func__);
	//hexdump(m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock, m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex);
}


bool EngineMonitoringHandler()
{
	GLogI("%s] engine tringgering monitoring start\r\n", __func__);	

	if( m_stTrgCtrl.stMonitoringCtrl.stEngineCtrl.ucCount == 0 )
		return false;

	MakeEngineStallMessage(m_stTrgCtrl.stMonitoringCtrl.stEngineCtrl.stEngStopInfo.strRequestCode, &m_stTrgCtrl.stMonitoringCtrl.stTxPacket);

	//hexdump(&m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo.ucDtcReqCodes[0],MAX_REQCODE_SIZE);
	//hexdump((uint8_t*)&m_stTrgCtrl.stMonitoringCtrl.stTxPacket.pData, m_stTrgCtrl.stMonitoringCtrl.stTxPacket.DataSize);

	if( SendTriggerPassThrouMessage(&m_stTrgCtrl.stMonitoringCtrl.stTxPacket) == true )
	{
		GLogI("dtc tringgering monitoring ok\r\n");
		//m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.ucDtcCurIndex++;

		// check dtc code with trigger info
		//EngineMatchingHandler(m_stTrgCtrl.stEcuInfo.unProtocolType, &m_stTrgCtrl.stMonitoringCtrl.stRxPacket, &m_stTrgCtrl.stMonitoringCtrl.stDtcCtrl.stTrgInfo);

		// rpm data for recording file
		//hexdump(&m_stTrgCtrl.stMonitoringCtrl.stRxPacket.mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER], m_stTrgCtrl.stMonitoringCtrl.stRxPacket.mLen-MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER);

		return true;
	}
	else
	{
		GLogE("%s] error rx fail\r\n", __func__);
	}

	return false;
}

uint16_t AnalyzeRpmData(uint8_t* pucRpmResData, uint32_t unLength)
{
	uint16_t usRpm = 0;
	// if single frame = 1, multi frame = 0,
	uint16_t usPos = 1;
	uint16_t usRealPos = m_stTrgCtrl.stMonitoringCtrl.stEngineCtrl.stEngStopInfo.usRealPos;
	uint16_t usStartPos = m_stTrgCtrl.stMonitoringCtrl.stEngineCtrl.stEngStopInfo.usStartPos;
	uint16_t usDataSize = 2;

	if( pucRpmResData == NULL || unLength == 0 )
	{
		printf("%s] error respoinse buffer\r\n", __func__);
		return 0;
	}

	// if single frame = 1, multi frame = 0, device the packet tyep wit length
	if( unLength > 8 )
		usPos = 0;
	
        printf("%s] analyzer rpm data\r\n", __func__);
        hexdump(pucRpmResData,unLength);
        
	if( m_stTrgCtrl.stMonitoringCtrl.stEngineCtrl.stEngStopInfo.usDataSize == usDataSize )
	{
		usRpm = pucRpmResData[usStartPos + usRealPos -2 -usPos]*0x100 + pucRpmResData[usStartPos + usRealPos -1 -usPos];
	}
	else
	{
		usRpm = pucRpmResData[usStartPos + usRealPos -2 -usPos];
	}

	// test code for aging test
	//usRpm = 0;

	GLogE("----------------------------------->>>>> %s] rpm : %d\r\n", __func__, usRpm);

	// adjust rpm dat
	if( usRpm < 50 )
		usRpm = 0;
	
	return usRpm;
}

void UsbTriggerMonitoring()
{
	if( m_stTrgCtrl.bUsbBtnPressed == true )
	{
		m_stTrgCtrl.bUsbBtnPressed = false;

		SendEvent2TriggerThread(eEVT_TRG_BTN_ACTIVE);

		GLogI("%s] button pressed\r\n", __func__);
	}
}


bool BtTriggerMonitoring()
{
	static uint32_t s_unAliveCheckTimeout = 0;

	if( Get_Tmr()-s_unAliveCheckTimeout > MAX_BLUETOOTH_ALIVE_TIME_OUT )
	{
		s_unAliveCheckTimeout = Get_Tmr();
		
		// send obd led enalbe
		SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_OBD_CCP,eTM_LED_OBD_ON);
		
		osEvent event = osSignalWait(SIGNAL_TRIGGER_ALIVE_RESPONSE, MAX_WAIT_BLUETOOTH_ALIVE_TIME_OUT);
		if(  event.status == osEventSignal )
		{	
			//GLogI("%s] rcv event for bluetooth alive\r\n", __func__);

			m_stTrgCtrl.ucNoBtRespAliveCount = 0;

			return true;
		}


		// send key enable
		//SendByteEvent2TriggerModule(eEVT_TM_CTRL_TRIGGER,eTM_TRIGGER_ON);              

		// time out : 3seconds : 100*30 :3000
		if( m_stTrgCtrl.ucNoBtRespAliveCount++ > MAX_BLUETOOTH_ALIVE_TIME_OUT_COUNT )
		{
			GLogI("%s] error bluetooth didn't response\r\n", __func__);
			m_stTrgCtrl.ucNoBtRespAliveCount = 0 ;
			return false;
		}
	}

		// send key enable
		//SendByteEvent2TriggerModule(eEVT_TM_CTRL_TRIGGER,eTM_TRIGGER_ON);              


	return true;
}

void EngineProcessHandler()
{
}

bool EcuOpenHandler()
{
	GLogI("EcuOpenHandler\r\n");

	uint8_t ucErrorCount = 0;
	uint8_t ucSuccessCount = 0;

	// set exception case ecu
	Record_OpenEcuException();

	// retry ecu open 
	for(int j=0; j<5;j++)
	{
		for(int i=0; i<m_stTrgCtrl.stEcuInfo.ucCommCodeCount; i++)
		{
			// send event to diagnostic thread	
			MakeOpenRecordMessage(i, &m_stTrgCtrl.stEcuInfo, &m_stTrgCtrl.stMonitoringCtrl.stTxPacket);

			// sending a data
			if( SendPassThrouMessage(&m_stTrgCtrl.stMonitoringCtrl.stTxPacket) == true )
			{
				GLogI("StartComm OK,EcuOpenHandler \r\n");

				//if( m_stTrgCtrl.stEcuInfo.stHwSet.ackmessage[0] != 0 )
				//{
				//	VCI_ACK();
				//}

				ucSuccessCount++;

				return true;
			}
			else
			{
				GLogE("StartComm fail \r\n");							

				ucErrorCount++;
			}

			// to break ecu open prcess in diagnostic process
			if( GetActiveFlightRecording() == false )	
			{
				return true;
			}
		}
		
		osDelay(10);
	}
        
	if( ucSuccessCount > 0 )
		return true;

	printf("%s] error open ecu\r\n", __func__);
	
	return false;
}


void InitializeTriggerMonitoringState()
{
	printf("intialize tirgger monitoring state\r\n");
	m_stTrgCtrl.stMonitoringCtrl.eState = eTrgMonitoring_Init;
}


void ClearTriggerMonitoringHandler()
{
	printf("clear tirgger monitoring thread\r\n");
	m_stTrgCtrl.stMonitoringCtrl.eState = eTrgMonitoring_WaitTimeOut;
}


void HandlerTriggerMonitoring()
{
	static uint32_t s_unRpmZeroCount = 0;
	static uint32_t s_unRpmCommFailCount = 0;
	static uint32_t s_unTimeOut = 0;
	
	osEvent event;
				
	// step#1 trigger time wait
	// step#2 to hault recording process sending a wait siganl
	// step#3 wait 500ms for diagnostic prcoess idle,
	// step#4 send trigger message to diagnostic
	// step#5 send a resume message to recording process.
	// step#6 change to idle states.

//used about 0.5 seconds
//static uint32_t s_untime_test =0;

	
	switch(m_stTrgCtrl.stMonitoringCtrl.eState)
	{
		case eTrgMonitoring_Init:
			s_unRpmZeroCount = 0;
			s_unRpmCommFailCount = 0;
			
			m_stTrgCtrl.bEngineOn = false;

			// fw wakeup info file 
			if( GetActiveFlightRecording() == true )
			{	
				// service mode
				if( GetCurFwServiceMode() == eApp_Inside )
				{
					SetDtcTriggerState(eTrgMonitoring_WaitTimeOut);
				}
				else
				{
					SetDtcTriggerState(eTrgMonitoring_Idle);
				}

				// db check
				if( GetRecordingConfigStatus() == false )
				{
					SetDtcTriggerState(eTrgMonitoring_Idle);
				}
			}
			else
			{
				SetDtcTriggerState(eTrgMonitoring_Idle);				
			}
			
			break;
		case eTrgMonitoring_WaitTimeOut:
			
			// dtc interval wait 1 seconds until getting a event from trigger module
			event = osSignalWait(SIGNAL_TRIGGER_BTN_EVENT, MAX_WAIT_TRIGGER_WAIT_TIME_OUT);
			if(  event.status == osEventSignal )
			{	
				GLogI("%s] rcv button event from trigger module\r\n", __func__);

				SetDtcTriggerState(eTrgMonitoring_WaitTimeOut);
			}
			else
			{
				// wait 10 seconds 
				if( Get_Tmr() - s_unTimeOut > MAX_WAIT_TRIGGER_MONITOR_TIME_OUT )
				{
					SetDtcTriggerState(eTrgMonitoring_Process);
					
					// send an event to record process for dtc diagnostic to prevent disturbing in diagnositc thread.
					// notifi to record thread
					SendEvent2RecordThread(eEVT_REC_FREEZE, NULL, 0);

					//osDelay(1);
					// wait until response with freeze event
					osSignalWait(SIGNAL_TRIGGER_FREEZE_RESPONSE, osWaitForever);
					
					s_unTimeOut = Get_Tmr();
				}
			}

			// exception case
			if( GetActiveFlightRecording() == false )
			{
				if( m_stTrgCtrl.stMonitoringCtrl.eState == eTrgMonitoring_Process )				
					SetDtcTriggerState(eTrgMonitoring_ResumeProcess);
				else
					SetDtcTriggerState(eTrgMonitoring_Idle);
			}
			
			break;
		case eTrgMonitoring_Process:

			SetDtcTriggerState(eTrgMonitoring_ResumeProcess);

			// check ecu open for wake up because of dtc requesting
			//if( EcuOpenHandler() == true )
			//{
				// normal just wait button status;
				if( (m_stTrgCtrl.stMonitoringCtrl.ucMode&eTrgMonitorMode_Dtc) == eTrgMonitorMode_Dtc )
				{
					// dtc check
					DtcMonitoringHanlder();
				}

				// engine check
				if( isTriggerInterfaceMode(eTrgMonitorMode_EngineStart) == true || isTriggerInterfaceMode(eTrgMonitorMode_EngineStop) == true )
				{
					//EngineProcessHandler();
					// start / stop engine monitoring
					if( EngineMonitoringHandler() == true )
					{
						// rpm value check
						if( AnalyzeRpmData(&m_stTrgCtrl.stMonitoringCtrl.stRxPacket.mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER], m_stTrgCtrl.stMonitoringCtrl.stRxPacket.mLen-MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER) == 0 )
						{
							// rpm is zero : engine stop 							
							if( s_unRpmZeroCount++ > MAX_RPM_ZERO_COUNT )
							{
								if( m_stTrgCtrl.bEngineOn == true )
								{
									// send egine stop event to recording thread
									SendEvent2RecordThread(eEVT_REC_ENGINE_STOP, NULL, 0);

									m_stTrgCtrl.bEngineOn = false;
								}
								
								s_unRpmZeroCount = 0;
							}
						}
						else
						{
							// rpm is not zero : engine start
							s_unRpmZeroCount = 0;

							if( m_stTrgCtrl.bEngineOn == false )
							{
								//osSignalSet(GetRecordingHandle(), SIGNAL_RECORD_WAKE_UP);
								
								// send egine stop event to recording thread							
								SendEvent2RecordThread(eEVT_REC_ENGINE_START, NULL, 0);
								
								m_stTrgCtrl.bEngineOn = true; 
							}
						}
					}
					else
					{				
						// failed to send engine info 
					}
				}

				s_unRpmCommFailCount = 0;

				SetDtcTriggerState(eTrgMonitoring_ResumeProcess);

			//}
			//else
			//{
			//	if( s_unRpmCommFailCount++ > 5 )
			//	{
			//		// ecu open fail 
			//		SetDtcTriggerState(eTrgMonitoring_SleepProcess);
			//	}
			//	
			//	osDelay(1000);
			//}

			break;
		case eTrgMonitoring_ResumeProcess:

			SetDtcTriggerState(eTrgMonitoring_WaitTimeOut);

			GLogI("%s] rezume recording system\r\n", __func__);
			// resume for recording
			// set signal to tirgger thread start triggger
			osSignalSet(GetRecordingHandle(), SIGNAL_RECORD_RESUME);

//printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s] tirgger time : %d\r\n", __func__,  Get_Tmr() - s_untime_test );

			if( GetActiveFlightRecording() == false )				
				SetDtcTriggerState(eTrgMonitoring_Idle);


			break;
		case eTrgMonitoring_SleepProcess:

			if( EcuOpenHandler() == true )
			{
				// rpm is not zero changed state for recoding 
				SetDtcTriggerState(eTrgMonitoring_ResumeProcess);		
			}
			else
			{
				if( s_unRpmCommFailCount++ > MAX_RPM_COMM_FAIL_COUNT )
				{
					GLogE(" ===============>>> %s] sleep waitting\r\n",__func__);
					// send event to trigger module to stop sending event to trigger module 	
					LED_ALL_OFF;

					// send obd led on event to trigger module
					//SendEvent2TriggerThread(eEVT_TRG_LED_OBD_OFF);

					// send event to trigger module for waiting data save. green led blink 5
					SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_GREEN, eTM_LED_GREEN_OFF);
					osDelay(1000);

					//SendByteEvent2TriggerModule(eEVT_TM_CTRL_TRIGGER, eTM_TRIGGER_OFF);
					//osDelay(10000);
				
					// send obd led on event to trigger module
					SendByteEvent2TriggerModule(eEVT_TM_CTRL_SLEEP, eTM_SLEEE_ACTIVE);	
					osDelay(1000);					

					// request sleep 
					SystemStandby();
				}

#if false
				//MONI test code recovery
				if( s_unRpmCommFailCount++ > 50 )
				{
					// rpm is not zero changed state for recoding 
					SetDtcTriggerState(eTrgMonitoring_Init);
				}
#endif				
			}

			osDelay(1000);
			
			break;
		case eTrgMonitoring_Idle:

			if( m_stTrgCtrl.bConnected == true )
			{
				// wait for trigger connection
				SetDtcTriggerState(eTrgMonitoring_Init);
			}
			
			osDelay(1000);
			break;
	}
	
}

void EvtHandlerTrigger(osEvent event)
{
	if( event.value.signals == eEVT_TRG_CONNECTED )
	{
		m_stTrgCtrl.bConnected = true;
	}
	else if( event.value.signals == eEVT_TRG_BT_CONNECT )
	{
		GLogI("%s] rcv bt connection event\r\n",__func__);

		m_stTrgCtrl.bBtConnected = true;
	}
	else if(event.value.signals == eEVT_TRG_BT_DISCONNECT )
	{
		GLogI("%s] rcv bt disconnection event\r\n",__func__);
		m_stTrgCtrl.bConnected = false;
		m_stTrgCtrl.bBtConnected = false;
		
		// clear button active status for sync
		m_stTrgCtrl.bActiveRecordButton = false;

		m_stTrgCtrl.eState = eTCS_Disconnect;
	}
	else if(event.value.signals == eEVT_TRG_LED_OBD_ON )
	{
		if( m_stTrgCtrl.bConnected == true )
		{
			SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_OBD_CCP,eTM_LED_OBD_ON);
		}
		else
		{
			m_stTrgCtrl.bDlyReqObdLed = true;
			m_stTrgCtrl.ucDlyReqObdLedValue = eTM_LED_OBD_ON;
		}
	}
	else if(event.value.signals == eEVT_TRG_LED_OBD_OFF )
	{
		if( m_stTrgCtrl.bConnected == true )
		{
			SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_OBD_CCP,eTM_LED_OBD_OFF);
		}
		else
		{
			m_stTrgCtrl.bDlyReqObdLed = true;
			m_stTrgCtrl.ucDlyReqObdLedValue = eTM_LED_OBD_OFF;
		}
	}
	else if(event.value.signals == eEVT_TRG_BTN_ACTIVE )
	{
		GLogI("%s] trigger moudule pressed under 10 minutes\r\n", __func__);

		// notifi to record trigger 
		SendEvent2RecordThread(eEVT_REC_TRIGGER_START, NULL, 0);
	}
	else if(event.value.signals == eEVT_TRG_CLEAR )
	{
		GLogI("%s] initilalize monitoring state\r\n", __func__);
		InitializeTriggerMonitoringState();
	}
	else
	{
		GLogE("%s] error not support event\r\n",__func__);
	}
}


bool SettingTriggerModule()
{
	if( m_stTrgCtrl.bConnected == false )
		return false;
	
	// usb trigger
	if( m_stTrgCtrl.bUsbConnected == true )
	{
		return true;
	}
	
	// bluetooth trigger
	if( m_stTrgCtrl.bBtConnected == true )
	{
		if( m_stTrgCtrl.bSendModeChanged == false )
		{
			// enable trigger button
			// first step after connection 
			// mode changed to normal firmware.
			//트리거 업데이트 실패시에 트리거.BIN으로 점프해야 함	140624 LWH
			//SendByteEvent2TriggerModule(eEVT_TM_CTRL_FW_MODE_CHANGE,eTM_FW_MODE_APP);
			SendEvent2TriggerModule(eEVT_TM_CTRL_CUR_MODE_INFO,NULL,0);

			osDelay(100);

			return false;
		}
		else
		{
			// send trigger dtc data to diagnostic message 
			if( m_stTrgCtrl.bDlyReqObdLed == true )
			{
				SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_OBD_CCP,m_stTrgCtrl.ucDlyReqObdLedValue);

				osDelay(100);

				return false;
			}
			else
			{
				// active trigger button
				if( m_stTrgCtrl.bActiveRecordButton == false )
				{
					SendByteEvent2TriggerModule(eEVT_TM_CTRL_TRIGGER,eTM_TRIGGER_ON);

					osDelay(100);

					return false;
				}
			}
		}

		return true;
		
	}

	return false;

}


void ShowTriggerStructureSize()
{
	static uint32_t s_unTimeOut;


	if( Get_Tmr()-s_unTimeOut  > 1000 )
	{
		s_unTimeOut = Get_Tmr();

		extern osMessageQId hOBDTxMessage;
		extern osMessageQId hOBDRxMessage;
		extern osMessageQId hWriteMsg;
		extern osMessageQId hDiagMsg;
		extern osMessageQId hOBDKlineTxMessage;
		extern osMessageQId hOBDKlineRxMessage;
		extern osMessageQId hRecordDataMsg;
		extern osMessageQId hFDTxMsg;
		extern osMessageQId hTCPRxMessage;
		extern osMessageQId hRecordAppMsg;

#if false
		GLogI("%s] hOBDTxMessage : %d\r\n", __func__, osMessageAvailableSpace(hOBDTxMessage));
		GLogI("%s] hOBDKlineTxMessage : %d\r\n", __func__,	osMessageAvailableSpace(hOBDKlineTxMessage));
		GLogI("%s] hRecordDataMsg : %d\r\n", __func__,	osMessageAvailableSpace(hRecordDataMsg));
		GLogI("%s] hTransmitMsg : %d\r\n", __func__,  osMessageAvailableSpace(hTransmitMsg));
		GLogI("%s] hTCPRxMessage : %d\r\n", __func__,  osMessageAvailableSpace(hTCPRxMessage));
		GLogI("%s] hDiagMsg : %d\r\n", __func__,  osMessageAvailableSpace(hDiagMsg));
		GLogI("%s] hOBDRxMessage : %d\r\n", __func__,  osMessageAvailableSpace(hOBDRxMessage));
		GLogI("%s] hOBDTxMessage : %d\r\n", __func__,  osMessageAvailableSpace(hOBDTxMessage));
		GLogI("%s] hParsingMsg : %d\r\n", __func__,  osMessageAvailableSpace(hParsingMsg));
		GLogI("%s] hRecordAppMsg : %d\r\n", __func__,  osMessageAvailableSpace(hRecordAppMsg));
		GLogI("%s] hTriggerAppMsg : %d\r\n", __func__,	osMessageAvailableSpace(hTriggerAppMsg));
#endif


#if false

		GLogI("%s] xPortGetFreeHeapSize() : %d\r\n", __func__,	xPortGetFreeHeapSize());

		HeapStats_t xHeapStats;

		vPortGetHeapStats(&xHeapStats);

		GLogI("%s] xAvailableHeapSpaceInBytes : %d\r\n", __func__, xHeapStats.xAvailableHeapSpaceInBytes);
		GLogI("%s] xSizeOfLargestFreeBlockInBytes : %d\r\n", __func__, xHeapStats.xSizeOfLargestFreeBlockInBytes);
		GLogI("%s] xSizeOfSmallestFreeBlockInBytes : %d\r\n", __func__, xHeapStats.xSizeOfSmallestFreeBlockInBytes);
		GLogI("%s] xNumberOfFreeBlocks : %d\r\n", __func__, xHeapStats.xNumberOfFreeBlocks);
		GLogI("%s] xMinimumEverFreeBytesRemaining : %d\r\n", __func__, xHeapStats.xMinimumEverFreeBytesRemaining);
		GLogI("%s] xNumberOfSuccessfulAllocations : %d\r\n", __func__, xHeapStats.xNumberOfSuccessfulAllocations);
		GLogI("%s] xNumberOfSuccessfulFrees : %d\r\n", __func__, xHeapStats.xNumberOfSuccessfulFrees);		
#endif

		//extern rsi_driver_cb_t *rsi_driver_cb;
	}
}


void HandlerTriggerInterfaceMonitor()
{
	char buffer[64]={0,};
        
	// check bluetooth trigger
	if( m_stTrgCtrl.bBtConnected == true )
	{
		if( BtTriggerMonitoring() == false )
		{
			// deinitalize bluetooth device
			//git_rs_reinit();
	
			//RequestBtDisconnect();
			//osDelay(100);
			
			GLogE("%s] bluetooth was disconnected\r\n", __func__);
			m_stTrgCtrl.eState = eTCS_Disconnect;
		}
	}

	// check usb trigger
	if( m_stTrgCtrl.bUsbConnected == true )
	{
		UsbTriggerMonitoring();
	}
	
	//rsi_bt_get_rssi(GetBtConnectedAddr(),buffer);
}

void TriggerHandler()
{
	// wait connection 	
	
	// if connected
	switch(m_stTrgCtrl.eState)
	{
		case eTCS_Init:
#if(!VCI_III_ASING_MODE)
			GLogI("%s] eTCS_Init\r\n", __func__);
#endif
			m_stTrgCtrl.bConnected = false;
			m_stTrgCtrl.bBtConnected = false;
			m_stTrgCtrl.bUsbConnected = false;
			m_stTrgCtrl.bSendModeChanged = false;

			m_stTrgCtrl.bEngineOn = false;

			m_stTrgCtrl.stMonitoringCtrl.bDtcActive = false;
			m_stTrgCtrl.stMonitoringCtrl.bEngineStopActive = false;

			m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlockIndex = 0;
			memset( &m_stTrgCtrl.stMonitoringCtrl.ucTrigDtcBlock[0], 0, MAX_TRIGGER_DTC_LENGTH);
			m_stTrgCtrl.stMonitoringCtrl.ucPreDtcCount = 0;

			extern char GetCurFwServiceMode();
			if( GetCurFwServiceMode() == eApp_Inside )
				m_stTrgCtrl.eState = eTCS_SetConfig;
			else
				m_stTrgCtrl.eState = eTCS_Idle;

			if( GetBluetoothConnectionStatus() == BT_TRIG_CONNECT )
			{
				m_stTrgCtrl.bBtConnected = true;
			}

			osDelay(100);
			
			break;
		case eTCS_SetConfig:

			GLogI("%s] rcv event for config parsing\r\n", __func__);
			
			// update dtc data for trigger of dtc.
			UpdateTriggerInfo();

			// clear dtc info
			ClearTriggerDtcData();

			m_stTrgCtrl.eState = eTCS_Connecting;

			osDelay(100);
			
			break;
						
		case eTCS_Connecting:

			if( m_stTrgCtrl.bConnected == false )
			{
                if( GetBluetoothConnectionStatus() == BT_TRIG_CONNECT )
                {
					m_stTrgCtrl.bBtConnected = true;
                }
                                
				if( m_stTrgCtrl.bBtConnected == true )
				{
					GLogI("%s] eTCS_Connecting\r\n", __func__);				
					// wait connection of bluetooth

					// to decide between diagnostic applcaion and triggeer module
					// checking of appcalition of diagnostic		
					// exception case for recording mode
					if( GetBluetoothConnectionStatus() == BT_SPP_CONNECT && LOCK_GET_STATE() == eLOCK_STATE_UNLOCK )
					{
						// if new connection with app, then recording mode should be idle not to disturbing with diagnostic of application
						if( GetActiveFlightRecording() == true )
							SetActiveFlightRecording(false);
					}
					else
					{
						SendEvent2TriggerModule(eEVT_TM_CTRL_VER_INFO,NULL,0);
					}
					
				}
				else
				{				
					// update recording active flag
					if( GetActiveFlightRecording() != GetFwWakeupInfo() )
						RecoveryActiveFlightRecording();
				}
				
				if( m_stTrgCtrl.bUsbConnected == true )
				{
					m_stTrgCtrl.bConnected = true;
				}

			}
			else 
			//if( m_stTrgCtrl.bConnected == true )
			{
				// connected with trigger module
							
				m_stTrgCtrl.eState = eTCS_Connected;
				m_stTrgCtrl.bSendModeChanged = false;

				// clear reponse signal
				ClearBluetoothReponseSignal();
			}

			// get dtc trgigger mode
			//if( m_stTrgCtrl.stMonitoringCtrl.bDtcActive == true )
			//	m_stTrgCtrl.eState = eTCS_Connected;

			osDelay(100);
			break;
		case eTCS_Connected:

			if( SettingTriggerModule() == true )
			{
				// active trigger
				if(m_stTrgCtrl.bActiveTrigger == false )
				{
					// notifi to record thread
					SendEvent2RecordThread(eEVT_REC_TRIGGER_CONNECTED, NULL, 0);

					m_stTrgCtrl.bActiveTrigger = true;


					// fw wakeup is false then reconfigure config file
					if( GetActiveFlightRecording() == false )
					{
						printf("%s] reconfigure trigger info\r\n", __func__);
						osSignalWait(SIGNAL_RECORD_CONFIG_PARSING, MAX_WAIT_CONFIG_TIME_OUT);
						m_stTrgCtrl.eState = eTCS_Init;
					}
				}
			}

			// monitoring interface
			HandlerTriggerInterfaceMonitor();

			break;
		case eTCS_Disconnect:
			
			m_stTrgCtrl.eState = eTCS_Idle;

			m_stTrgCtrl.bActiveTrigger = false;

			//rsi_bt_app_init();

			//initialize all control variable
			IntializeTriggerControlInfo();

			// monitoring interface
			HandlerTriggerInterfaceMonitor();

			// notifi to record thread
			SendEvent2RecordThread(eEVT_REC_TRIGGER_DISCONNECT, NULL, 0);
			
			break;
			
		case eTCS_Idle:

			if( GetCurFwServiceMode() == eApp_Inside )
			{
				// check bluetooth connection 
				m_stTrgCtrl.eState = eTCS_Connecting;
			}
			
			break;
		default:
			break;
	}	
}

static void TriggerTh( void const * argument )
{
	osEvent event;
	
	// wait bluetoot connection
	// request trigger info to recording thread.
	// wait event for paring dtc config.
	event = osSignalWait(SIGNAL_RECORD_CONFIG_PARSING, MAX_WAIT_CONFIG_TIME_OUT);
	if(  event.status == osEventSignal )
	{	
		GLogI("%s] rcv event for config parsing\r\n", __func__);
		// update dtc data for trigger of dtc.

		m_stTrgCtrl.eState = eTCS_Init;
	}

	// initialize states
	ClearTriggerHandler();
	
	while( 1 )
	{
		event = osMessageGet( hTriggerAppMsg, 10 );

		if( event.status != osEventMessage )
		{
			//! if events are not received loop will be continued.
			// GLogE ("error!! TriggerTh \r\n");
		}
		else
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hTriggerAppMsg));
#endif
			GLogI("get TriggerTh\r\n");

			// first distirbution of event 
			EvtHandlerTrigger(event);
		}

		// usb / wireless trigger bluetooth handler
		TriggerHandler();

		// dtc / engine monitoring // wait setting a config
		if( m_stTrgCtrl.eState >= eTCS_Connecting && m_stTrgCtrl.eState  <= eTCS_Idle )
		{
			HandlerTriggerMonitoring();
		}
		
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#else
        osDelay( 1 );
#endif
	}
}


