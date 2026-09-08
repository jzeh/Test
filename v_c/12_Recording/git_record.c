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
#include "git_record_config.h"
#include "git_record_file.h"
#include "git_trigger.h"

#include "ff.h"

#include "Git_pm.h"

#include "Led.h"


//#define ENABLE_TEST_DATA_FEEDER
//#define ENABLE_TEST_SHORT_RECORD
//#define ENABLE_TEST_STALL_BUFFER

#ifdef ENABLE_TEST_SHORT_RECORD
// unit is seconds
#define FILE_SIZE_CHECK_OVER_TIME (5*1000)
#else		
// unit is seconds
#define FILE_SIZE_CHECK_OVER_TIME (60*1000)
#endif


#define TMP_BUFF_SIZE (1024)


#define MAX_CONFIG_TMP_BUFFER_LENGTH (3*1024)
#define MAX_CAN_RX_WAITING_TIME (5*1000)

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define MESSAGE_RECORD_APP_QUEUE_SIZE   (10)
#define MESSAGE_RECORD_DATA_QUEUE_SIZE   (5)


#define MAX_RECORD_SLEEP_WAIT_TIME_OUT (1*1000)
#define MAX_RECORD_ENGINE_WAIT_TIME_OUT (10*1000)


/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/




extern osMessageQId	hDiagMsg;
extern osPoolId		hPTPKPool;
extern osPoolId		hDiagPool;
extern eLockState 	g_eLockStatus;
extern osThreadId	hTriggerAppTh;
extern stTriggerControl m_stTrgCtrl;
unsigned int g_unSleepWaitTimeout=0;
#ifdef PRINT_MESSAGE_ID
	extern stMESSAGE_ID_INFO stMessageIdInfo[20];
 	extern uint8_t ucMessageIdInfoCnt;
#endif


// Thread
static void RecordTh( void const * argument );

static osThreadId	hRecordAppTh;
osThreadDef( recordappth,	RecordTh,	osPriorityNormal, 0, 3 * 768 );

// Message
osMessageQId	hRecordAppMsg;
osMessageQDef( recordappqueue, MESSAGE_RECORD_APP_QUEUE_SIZE, uint32_t );
osMessageQId	hRecordDataMsg;
osMessageQDef( recorddataqueue, MESSAGE_RECORD_DATA_QUEUE_SIZE, uint32_t );


eRecordingStateResult Record_Init();
eRecordingStateResult Record_ReadConfig();
eRecordingStateResult Record_HwSetting();
eRecordingStateResult Record_Init5bps();
eRecordingStateResult Record_InitKwp();
eRecordingStateResult Record_OpenEcu();
eRecordingStateResult Record_Monitoring();
eRecordingStateResult Record_Idle();
eRecordingStateResult Record_Error();
eRecordingStateResult Record_Sleep();


bool CheckRecordEcuOpen();
void ClearConfigInfo();
void ReClearConfigInfo();

void UpdateMonitoringInfo();
bool ReadFwWakeupInfo(uint8_t* pucBuffer, int32_t* pnLength);
bool WriteFwWakeupInfo(uint8_t* pucBuffer, int32_t nLength);
bool GetFwWakeupInfo();


uint32_t WrBkRam(uint8_t* pucSrc, uint32_t unDataSize);



typedef eRecordingStateResult (*fnpRecordMainHandler)(void);



typedef struct __stRecordingHandler
{
	fnpRecordMainHandler fnpFunc;	
}stRecordingHandler;		



stRecordingHandler m_stRecordingHandler[] = 
{
	Record_Init,
	Record_ReadConfig,
	Record_HwSetting,
	Record_Init5bps,
	Record_InitKwp,
	Record_OpenEcu,
	Record_Monitoring,
	Record_Idle,
	Record_Error,
	Record_Sleep,
};

/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/


stRecordingControl m_stRecordingControl;


// temp buffer 
stVehicleInfo m_stVcInfo;
stConfigControl m_stSysCfg;
stFwRecordingWakeupInfo m_stFwWakeupInfo;


static AppName m_eRecordingModeState = eApp_VCI_2;
static AppName m_eOldRecordingModeState = eApp_VCI_2;

void RecordingSystemModeHandler();
AppName GetRecordingSystemModeState();


osThreadId	GetRecordingHandle()
{
	return hRecordAppTh;
}

void InitlalizeRecordingState()
{
	m_stRecordingControl.eRcdState = eRecord_Init;
};

void hexdump(uint8_t* pucBuffer, int32_t nLength)
{
	GLogI(" read size : %d\r\n", nLength );

	int nQ = nLength / 16;
	int nR = nLength % 16;

	for(int ii =0; ii<nQ; ii++)
	{
		for(int kk=0; kk<16;kk++)
		{			
			GLogI(" %02X",pucBuffer[ii*16+kk]);
		}
		GLogI(" \r\n");	
	}
	GLogI(" \r\n");

	for(int kk=0; kk<nR;kk++)
	{
		GLogI(" %02X",pucBuffer[nQ*16+kk]);
	}
	GLogI(" \r\n"); 


	GLogI(" \r\n");	
	GLogI(" \r\n");	
}


void ShowRecordingStructureSize()
{
		GLogI("%s] stConfigControl size : %d\r\n", __func__, sizeof(stConfigControl));
		GLogI("%s] tagEngineStopTiggerInfo size : %d\r\n", __func__, sizeof(tagEngineStopTiggerInfo));
		GLogI("%s] stFlightRecordConfigControl size : %d\r\n", __func__, sizeof(stFlightRecordConfigControl));
		GLogI("%s] stVehicleInfo size : %d\r\n", __func__, sizeof(stVehicleInfo));	
		GLogI("%s] stECUIDInfo size : %d\r\n", __func__, sizeof(stECUIDInfo));
		GLogI("%s] stTriggerInfo size : %d\r\n", __func__, sizeof(stTriggerInfo));
		GLogI("%s] stRecordItems size : %d\r\n", __func__, sizeof(stRecordItems));
		GLogI("%s] tagHARDWARESET size : %d\r\n", __func__, sizeof(tagHARDWARESET));
		GLogI("%s] tagJ2534SETCONFIG size : %d\r\n", __func__, sizeof(tagJ2534SETCONFIG));
}


void ClearFlightRecording()
{
	m_stRecordingControl.eRcdState = eRecord_Init;

	// clear allocated config info
	ReClearConfigInfo();

	//clear config info
	ClearConfigInfo();
}



bool GetTriggerConfig(stTriggerInfo* pstTrgInfo)
{
	if( GetRecordingConfigStatus() == false )
	{
		return false;
	}

	if( pstTrgInfo == NULL )
	{
		GLogE("%s] error read trigger config\r\n", __func__);

		return false;
	}

	if( m_stRecordingControl.pstCfgCtrl != NULL )	
	{
		memcpy(pstTrgInfo, &m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stTrgInfo, sizeof(stTriggerInfo));
	}
	else
	{
		return false;
	}

	return true;
}

bool GetEngineConfig(tagEngineStopTiggerInfo* pstEngStopTrgInfo)
{
	if( GetRecordingConfigStatus() == false )
	{
		return false;
	}

	if( pstEngStopTrgInfo == NULL )
	{
		GLogE("%s] error read engine config\r\n", __func__);

		return false;
	}

	if( m_stRecordingControl.pstCfgCtrl != NULL )	
	{
		memcpy(pstEngStopTrgInfo, &m_stRecordingControl.pstCfgCtrl[0].stEngStopTrgInfo, sizeof(tagEngineStopTiggerInfo));
	}
	else
	{
		return false;
	}

	return true;
}

bool GetEcuConfig(stECUIDInfo*	pstEcuIdInfo)
{
	if( GetRecordingConfigStatus() == false )
	{
		return false;
	}

	if( pstEcuIdInfo == NULL )
	{
		GLogE("%s] error read engine config\r\n", __func__);
		return false;
	}

	if( m_stRecordingControl.pstCfgCtrl != NULL )	
	{
		memcpy(pstEcuIdInfo, &m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo, sizeof(stECUIDInfo));
	}
	else
	{
		return false;
	}

	return true;
}


bool GetRecordingConfigStatus()
{
	if( m_stRecordingControl.bConfigError == true )
	{
		return false;
	}

	return true;
}

bool GetActiveFlightRecording()
{
	return m_stRecordingControl.bActiveFlightRecording;
}

void SetActiveFlightRecording(bool bActive)
{
	m_stRecordingControl.bActiveFlightRecording = bActive;
}

void RecoveryActiveFlightRecording()
{
	m_stRecordingControl.bActiveFlightRecording = GetFwWakeupInfo();
}


void RunRecordMainHandler()
{
	
}


eRecordingStateResult Record_Init()
{
	char ucPath[MAX_FILE_PATH_LENGTH]={0,};
	
	// delete remained temp file
	f_unlink(STORE_REC_TEMP_FILE_NAME);
	f_unlink(STORE_REC_NEW_TEMP_FILE_NAME);

	InitializeTempFile();


	//mountFatFS();
	//formatEmmc();

	memcpy(ucPath,ROOT_PATH,sizeof(ROOT_PATH));
	scan_files(ucPath);
	memset(ucPath,0x00,MAX_FILE_PATH_LENGTH);
	memcpy(ucPath,RECORD_PATH,sizeof(RECORD_PATH));
	scan_files(ucPath);

	f_chdir(DIR_RECORD);

	return eRecordingStateResult_Success;
}


bool AdjustTriggerInfo()
{
	//memcpy(&, &ucConfigBuffer[Addr_SelectTrigTime-Addr_ProtocolID], sizeof(uint16_t));
	
	// Select Trigger Time default일 경우...10초로 기본 세팅
#ifdef ENABLE_RECORED_MALLOC  
	if(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stTrgInfo.ucRecordTime == 0x0000)
#else
	if(m_stRecordingControl.stCfgCtrl.stCfgInfo.stTrgInfo.ucRecordTime == 0x0000)
#endif          
	{
		// set defualt 10 min seconds.
		m_stRecordingControl.stDataCtrl.unMaxRecordingTime = 10*60;
	}

#warning "need to check"
#if false 
	if((m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stTrgInfo.ucTrigMode&0x08)==0x08)
	{		
		m_stRecordEngStopTrgCtrl.EngineStopReqCodeLength = m_stRecordingControl.stEngCtrl.EngineStopReqCode[2]+3;			// can 센서출력 송신코드 사이즈
	}
#endif

	return true;
}


void DisplayRecordingInfo()
{
	if( m_stRecordingControl.pstCfgCtrl == NULL )
	{
		printf("%s] error\r\n",__func__);
		return;
	}
	
	printf("%s] recording info, count : %d\r\n", __func__, m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stRecInfo.usRecordItemCount);

	for(int i=0;i<m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stRecInfo.usRecordItemCount;i++)
	{	
		hexdump(&m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stRecInfo.pucRecordItemReqCodes[i][0],50);
	}
}


bool InitializeConfigInfo()
{
	GLogI("%s] start parsing config data file\r\n", __func__);

	if( ReadVehicleInfoFromConfig(&m_stVcInfo) == false )
	{
		
		GLogE("%s] error : ReadVehicleInfoFromConfig\r\n",__func__);
	
		// check config file error
		m_stRecordingControl.bConfigError = 1;
		
		// initlaize all control
		m_stRecordingControl.eRcdState = eRecord_Idle;

		return false;
	}

	// multi record suppport
	if(m_stVcInfo.ucVin[17] == 1)
	{
		m_stRecordingControl.bIsMultiRecord = true;
		m_stRecordingControl.ucCurSysCnt = m_stVcInfo.ucVin[18];
		m_stRecordingControl.ucTotalSysCnt = m_stVcInfo.ucVin[19];
	}
	else
	{		
		m_stRecordingControl.bIsMultiRecord = false;
		m_stRecordingControl.ucCurSysCnt = 0;
		m_stRecordingControl.ucTotalSysCnt = 1;
	}

	
	/******************************************************************************************/
	// if system need to support mulity system // the sytem need to read all config of each system
	// now just read 1 system.
	/******************************************************************************************/
	//taskENTER_CRITICAL(); 
	//m_stRecordingControl.pstCfgCtrl = (stConfigControl*)pvPortMalloc(sizeof(stConfigControl)*m_stRecordingControl.ucTotalSysCnt);
	//taskEXIT_CRITICAL();

	GLogI("%s] system count : %d, system struct size : %d\r\n", __func__, m_stRecordingControl.ucTotalSysCnt, sizeof(stConfigControl)*m_stRecordingControl.ucTotalSysCnt);
	GLogI("%s] available heap size : %d\r\n", __func__, xPortGetFreeHeapSize());
	
#ifdef ENABLE_RECORED_MALLOC
	m_stRecordingControl.pstCfgCtrl = (stConfigControl*)pvPortMalloc(sizeof(stConfigControl)*m_stRecordingControl.ucTotalSysCnt);

	if( m_stRecordingControl.pstCfgCtrl == NULL )
	{
		GLogE("%s] error buffer is null\r\n",__func__);
		return false;
	}

	memset( &m_stRecordingControl.pstCfgCtrl[0], 0 ,sizeof(stConfigControl)*m_stRecordingControl.ucTotalSysCnt);
#else
	memset( &m_stRecordingControl.stCfgCtrl, 0 ,sizeof(stConfigControl));
#endif

	GLogI("\r\n\r\n%s] total system : %d, memory : %d\r\n", __func__, m_stRecordingControl.ucTotalSysCnt, sizeof(stConfigControl)*m_stRecordingControl.ucTotalSysCnt);

	for(int i=0;i<m_stRecordingControl.ucTotalSysCnt;i++)		
	{	
		if( ReadEcuSystemInfoFromConfig(i,&m_stSysCfg) == false )
		{
			GLogE("%s] ERROR : ReadEcuSystemInfoFromConfig\r\n",__func__);			
			// check config file error
			m_stRecordingControl.bConfigError = 1;
			
			// initlaize all control
			m_stRecordingControl.eRcdState = eRecord_Idle;

			break;
		}
		
#ifdef ENABLE_RECORED_MALLOC
		memcpy(&m_stRecordingControl.pstCfgCtrl[i], &m_stSysCfg, sizeof(stConfigControl));
#else
		memcpy(&m_stRecordingControl.stCfgCtrl, &m_stSysCfg, sizeof(stConfigControl));
#endif

		/**************************************************************************************************/
		// exception case need to be check // adjust system just 1 
		if( m_stSysCfg.stCfgInfo.stEcuIdInfo.unProtocolType == RS232_MCU )
		{
			m_stRecordingControl.ucTotalSysCnt = 1;
			break;
		}
		/**************************************************************************************************/
	}

	/**************************************************************************************************/
	// read stalll data if is needed
	if( m_stRecordingControl.ucTotalSysCnt > 0  )
	{
		if( ((uint8_t)(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stTrgInfo.ucTrigMode & eTrgMonitorMode_EngineStart) == (uint8_t)eTrgMonitorMode_EngineStart) ||
		    ((uint8_t)(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stTrgInfo.ucTrigMode & eTrgMonitorMode_EngineStop) == (uint8_t)eTrgMonitorMode_EngineStop) )
		{
#ifdef ENABLE_RECORED_MALLOC		
			if( ReadEcuStallConfig(0, &m_stRecordingControl.pstCfgCtrl[0].stEngStopTrgInfo) == false )
#else
			if( ReadEcuStallConfig(0, &m_stRecordingControl.stCfgCtrl.stEngStopTrgInfo) == false )
#endif
			{			
				// check config file error
				//m_stRecordingControl.bConfigError = 1;

				GLogE("%s] ERROR : ReadEcuStallConfig\r\n",__func__);			
			}
		}
	}

	//DisplayRecordingInfo();

	return true;
}


void InitializeRecordingInfo()
{
	// backup ram position
	m_stRecordingControl.stDataCtrl.usBackupDataPosition = 0;
	
	// clear buffer
	memset(m_stRecordingControl.stDataCtrl.strStartTime,0,MAX_TRIP_TIME_LENGTH);
	memset(m_stRecordingControl.stDataCtrl.strTrigTime,0,MAX_TRIP_TIME_LENGTH);
	memset(m_stRecordingControl.stDataCtrl.strEndTime,0,MAX_TRIP_TIME_LENGTH);
	
	memset(m_stRecordingControl.stDtcCtrl.ucTrigDtcBlock,0,MAX_TRIGGER_DTC_LENGTH);
	
	m_stRecordingControl.stDataCtrl.usMsgCount = 0;
	m_stRecordingControl.stDtcCtrl.usTrigMode = 0;
}

void InitSleepWaitControl()
{
#if true
	int32_t nLength = 0;

	if( ReadFwWakeupInfo((uint8_t*)&m_stFwWakeupInfo, &nLength) == true )
	{
		//printf("%s] fw wakeup mode : %d\r\n", __func__, m_stFwWakeupInfo.bActiveRecording);
	}
	else
	{
		// clear all variable
		memset(&m_stFwWakeupInfo, 0, sizeof(stFwRecordingWakeupInfo));
	}

	// update local control variable
	SetActiveFlightRecording(m_stFwWakeupInfo.bActiveRecording);

	printf("%s] fw wakeup mode : %d\r\n", __func__, GetActiveFlightRecording());

#else
	if( GetVCI2FileRead(FW_WAKEUP_DATA, &eWakeupModeStatus, &iLength)==TRUE)
	{
		if(eWakeupModeStatus==eFW_MODE_WAKEUP)  { }
	}
	else
	{
		intLEDCount=0;
		
 		if(m_stRecordingControl.stCanCtrl.unCanCommErrorCount > 4)	
			LedStaus=9;		//140307 LWH 통신 실패 표출
		else                        
			LedStaus=2;		//초기상태 130924 LWH
			
		if(g_bNormalBoot==1)
		{
			RES0xD001(LED_OBD_II_OFF);
			if( TriggerModuleControl(LED_RED_ON_GREEN_OFF) == FAIL )
			{
				m_stRecordingControl.eRcdState = eRecord_Error;
				return;
			}
		}
	}
#endif
}


bool ReadFwWakeupInfo(uint8_t* pucBuffer, int32_t* pnLength)
{
	int32_t nLength;
	if( GetVCI2FileRead(FW_WAKEUP_DATA, (U8*)pucBuffer, &nLength) == false )
	{
		printf("%s] error read fw wakup info\r\n", __func__);

		return false;
	}

	*pnLength = nLength;

	return true;
}

bool WriteFwWakeupInfo(uint8_t* pucBuffer, int32_t nLength)
{
	if( GetVCI2FileWrite(FW_WAKEUP_DATA, (U8*)pucBuffer, nLength) == false )
	{
		printf("%s] error write fw wakup info\r\n", __func__);
		return false;
	}

	return true;
}

void ClearFwWakeupInfo()
{
	m_stFwWakeupInfo.bActiveRecording = false;
				
	if( WriteFwWakeupInfo( (uint8_t*)&m_stFwWakeupInfo, sizeof(m_stFwWakeupInfo) ) == true )
	{
	}

	SetActiveFlightRecording(false);
}

void UpdateFwWakeupInfo(bool bActive)
{
	m_stFwWakeupInfo.bActiveRecording = bActive;
	
	if( WriteFwWakeupInfo( (uint8_t*)&m_stFwWakeupInfo, sizeof(m_stFwWakeupInfo) ) == true )
	{
	
	}	
}

bool GetFwWakeupInfo()
{
	return m_stFwWakeupInfo.bActiveRecording;
}

void InitializeRecordingControl()
{
 	//need to check  is it need the iniDlccomCount
	// adjust hw set	
	//SetHWAndConfigPara();			// HW Setting 함수 

	// set record related initalize
#ifdef ENABLE_RECORED_MALLOC  
	switch( m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stTrgInfo.ucRecordTime )
#else
	switch( m_stRecordingControl.stCfgCtrl.stCfgInfo.stTrgInfo.ucRecordTime )
#endif
	{
		// 10Min = 60*10
		case 0:
		case 1:
			m_stRecordingControl.stDataCtrl.unMaxRecordingTime = 10*60;	
			break;
		// 30Min
		case 2:
			m_stRecordingControl.stDataCtrl.unMaxRecordingTime = 30*60;	
			break;
		// 60Min
		case 3:
			m_stRecordingControl.stDataCtrl.unMaxRecordingTime = 60*60;	
			break;
		default:
			// for debugging mode
			m_stRecordingControl.ucRecordingMode = 1; 
			GLogE("SETPARA_1 ERROR\r\n");
			break;
	}

	// unit is second.
	m_stRecordingControl.stDataCtrl.usRecordTimeAfterTrigger = m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stTrgInfo.usRpmDataIndex;

	
#ifdef ENABLE_RECORED_MALLOC        
	m_stRecordingControl.stDataCtrl.ucRecordItemCount = m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stRecInfo.usRecordItemCount;
#else
	m_stRecordingControl.stDataCtrl.ucRecordItemCount = m_stRecordingControl.stCfgCtrl.stCfgInfo.stRecInfo.usRecordItemCount;
#endif

	// set can dtc control related
#ifdef ENABLE_RECORED_MALLOC
	if( m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType == RS232_MCU )
#else
	if( m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType == RS232_MCU )		          
#endif          
		m_stRecordingControl.stDtcCtrl.ucDtcCommCnt = 255; 			
	else								
		m_stRecordingControl.stDtcCtrl.ucDtcCommCnt = 30; // DTC Request one time per anothre Request 30th times

	// set ca control realted 
	m_stRecordingControl.stCanCtrl.ucCurCommIndex = 0;

#ifdef ENABLE_RECORED_MALLOC        
	m_stRecordingControl.stCanCtrl.ucMaxCommIndex = m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.ucCommCodeCount;	
	m_stRecordingControl.stCanCtrl.ucOpenCommCount = m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.ucCommCodeCount;
	m_stRecordingControl.stCanCtrl.ucCloseCommCount = m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.ucCommCodeCount; 
#else
	m_stRecordingControl.stCanCtrl.ucMaxCommIndex = m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.ucCommCodeCount;	
	m_stRecordingControl.stCanCtrl.ucOpenCommCount = m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.ucCommCodeCount;
	m_stRecordingControl.stCanCtrl.ucCloseCommCount = m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.ucCommCodeCount; 
#endif        
	m_stRecordingControl.stCanCtrl.unCanCommErrorCount=0;
	

	// adjust trigger info // for exception case
	if( m_stRecordingControl.pstCfgCtrl[0].stEngStopTrgInfo.usRealPos == 0 )
		m_stRecordingControl.pstCfgCtrl[0].stEngStopTrgInfo.usRealPos = 1;


#if false
	if(g_bNormalBoot==1)
	{
		RES0xD001(LED_OBD_II_ON);
		if( TriggerModuleControl(LED_RED_ON_GREEN_ON) == FAIL )
		{
			m_stRecordingControl.eRcdState = eRecord_Error;
			return;
		}
		if( TriggerModuleControl(ENTER_KEY_ENABLE) == FAIL )
		{
			m_stRecordingControl.eRcdState = eRecord_Error;
			return;
		}
	}
#endif

#if false
	if( GetVCI2FileRead(FW_WAKEUP_DATA, &eWakeupModeStatus, &iLength)==TRUE)	//140306 LWH WAKEUP ?시 무선 부팅되기전에 무선으로 주는 정보때문에 부팅이 안되는 현상 막음
	{
		f_unlink(FW_WAKEUP_DATA);		// 140307 LWH 중간에 다시 트리거를 연결했더라도 0xC053에서 곧바로 체크하지 못함. 여기서 삭제하면 WAKEUP 후 한번만 수행하게 됨
		
		if(eWakeupModeStatus==eFW_MODE_WAKEUP)//wakeup 모드일경우 오픈 이후에 led점등 및 부저동작	
		{
		  LedStaus=99;
		  Delay(700);
		}
	}
#endif
		
}


void SetFlightRecordingState(eRecordingState eState)
{
	m_stRecordingControl.ePreRcdState = m_stRecordingControl.eRcdState;
	m_stRecordingControl.eRcdState = eState;
}


eRecordingState GetFlightRecordingState()
{
	return m_stRecordingControl.eRcdState;
}


void ClearConfigInfo()
{
#ifdef ENABLE_RECORED_MALLOC

	m_stRecordingControl.ucTotalSysCnt = 0;

	m_stRecordingControl.pstCfgCtrl->stCfgInfo.stRecInfo.usRecordItemCount = 0;

	for(int i=0;i < MAX_ITEM_COUNT; i++)
	{
		m_stRecordingControl.pstCfgCtrl->stCfgInfo.stRecInfo.pucRecordItemReqCodes[i] = NULL;
	}

	m_stRecordingControl.pstCfgCtrl = NULL;

#endif
}

void ReClearConfigInfo()
{
#ifdef ENABLE_RECORED_MALLOC
	if( m_stRecordingControl.ucTotalSysCnt > 0 )
	{
		//MONI 2023.03.10 [suresoft]97) : improve the code stability move null check code.
		if( m_stRecordingControl.pstCfgCtrl != NULL )
		{
			for(int i=0;i < m_stRecordingControl.pstCfgCtrl->stCfgInfo.stRecInfo.usRecordItemCount; i++)
			{
				if( m_stRecordingControl.pstCfgCtrl->stCfgInfo.stRecInfo.pucRecordItemReqCodes[i] != NULL )
				vPortFree(m_stRecordingControl.pstCfgCtrl->stCfgInfo.stRecInfo.pucRecordItemReqCodes[i]);
			}

			vPortFree(m_stRecordingControl.pstCfgCtrl);
		}

		m_stRecordingControl.ucTotalSysCnt = 0;
	}	
#endif
}

eRecordingStateResult Record_ReadConfig()
{
	eWakeupStatus eWakeupModeStatus;

	InitializeRecordingInfo();

	ReClearConfigInfo();

	ClearConfigInfo();
	
	// set config file data
	if( InitializeConfigInfo() == false )
	{
		GLogE("error : InitializeRecordConfigInfo\r\n");
		m_stRecordingControl.bConfigError = true;
		
		return eRecordingStateResult_Fail;
	}

	// initialize all control variables
	InitializeRecordingControl();
	
	//exception case
	if( m_stRecordingControl.stCanCtrl.ucMaxCommIndex == 0 )
	{
		m_stRecordingControl.bConfigError = true;
	}

	return eRecordingStateResult_Success;
}


eRecordingStateResult Record_HwSetting()
{
	//#Step#1
	//setting and wait 5 seconds for hardware initialize.
	// after setting 
	GLogI("%s] start\r\n", __func__);
	
	// set first system protcol type
	// if system need to more then 1 system then changed this hw set every new system.
#ifdef ENABLE_RECORED_MALLOC        
	VCI_HW_Setting(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType);
#else
	VCI_HW_Setting(m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType);
#endif



	//#Step#2
	// wait 5 seocnds. // set timer and wait event 




	//#Step#3
	// check protocol for new haredware initalize.




#ifdef ENABLE_RECORED_MALLOC        
	if( m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230 || m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230_POWERTEC || 
		m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230_ETC|| m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230_LLINE_LOW)
#else
        if( m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230 || m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230_POWERTEC || 
		m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230_ETC|| m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230_LLINE_LOW)
#endif          
	{
		osDelay(g_stGITSetConfig.nTIdle);
		osDelay(g_stGITSetConfig.nP3Min);//waiting for P3MIN in the VCI_FastInit(). So the next retry time is at least after P3MIN (only VCI3)
#ifdef ENABLE_RECORED_MALLOC          
		if(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230_POWERTEC)		
#else
		if(m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType == ISO14230_POWERTEC)		                  
#endif                  
		{
			KW_fast_init_Powertec(g_stGITSetConfig.nTInil,(g_stGITSetConfig.nTWUp-g_stGITSetConfig.nTInil));	// Powertec Protocol by kyc 2007.06.19
			osDelay(800);
			KW_fast_init_Powertec(g_stGITSetConfig.nTInil,(g_stGITSetConfig.nTWUp-g_stGITSetConfig.nTInil));	// Powertec Protocol by kyc 2007.06.19
		}
		else
		{
			KW_fast_init(g_stGITSetConfig.nTInil,(g_stGITSetConfig.nTWUp-g_stGITSetConfig.nTInil));
		}
	}
	else
	{
		// CHJ  InitAddrCount : SetRecordPara() 함수에서 값이 셋팅되는  변수
#warning "need to check system index"
#ifdef ENABLE_RECORED_MALLOC
		if( m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.ucInitialAddrCount != 0 )
#else
		if( m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.ucInitialAddrCount != 0 )                  
#endif                  
		{
#if false			
			VCI_5bpsInit();
#endif
		}

	}

	return eRecordingStateResult_Success;
}


void Record_OpenEcuException()
{
	uint8_t KEY1, KEY2;	// seo

#ifdef ENABLE_RECORED_MALLOC
	if(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5014)
#else
	if(m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5014)
#endif          
	{
#ifdef ENABLE_RECORED_MALLOC          
		m_stRecordingControl.unPreProtocolType = m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType;
#else
		m_stRecordingControl.unPreProtocolType = m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType;
#endif                
		VCI_HW_Setting(ISO15765_29BIT);
	}
	
	// tx
#ifdef ENABLE_RECORED_MALLOC        
	if(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5011 || m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5012 || 
	   m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5013 || m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5014)
#else
	if(m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5011 || m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5012 || 
	   m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5013 || m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5014)          
#endif          
	{
		// in this is exception case do all process even if respose is fail.
		// way index max is 3? 
		for(int j=0; j<3; j++)
		{
			uint8_t ucTxBuffer[20] = {0,};
			int32_t nLength;

			memset(ucTxBuffer,0,sizeof(ucTxBuffer));
			
			if(j==0)
			{
				// we need to check way first index 0x88???
				ucTxBuffer[0]=0x88;
				ucTxBuffer[1]=(uint8_t)((0x18DAE6F9<<3)>>24)&0xFF;
				ucTxBuffer[2]=(uint8_t)((0x18DAE6F9<<3)>>16)&0xFF;
				ucTxBuffer[3]=(uint8_t)((0x18DAE6F9<<3)>>8)&0xFF;
				ucTxBuffer[4]=(uint8_t)((0x18DAE6F9<<3))&0xFF;
				ucTxBuffer[5] = 0x02;
				ucTxBuffer[6] = 0x10;
				ucTxBuffer[7] = 0x01;

				nLength = 8;
					
				//CanTx_OutEn_EX(TxBuff); // 1	
				//Can0WriteBuff_29bit(CAN_CH, m_stRecordingControl.stListCtrl.TxBuff+5, 0x18DAE6F9, m_stRecordingControl.stListCtrl.TxBuff[0]);
				// send event to diagnostic thread
                PTmsgPkt_t* pstTxPacket = &m_stRecordingControl.stCanCtrl.stTxPacket;
				CovertBuffer2PassThruMessage(ucTxBuffer, nLength, &pstTxPacket);

				// need to check do app need to wait a response?
				// sending a data 
				if( SendPassThrouMessage(&m_stRecordingControl.stCanCtrl.stTxPacket) == true )
				{
				}
				else
				{
				}

				
			}
			else if(j==1)
			{			
				ucTxBuffer[0]=0x88;
				ucTxBuffer[1]=(U8)((0x18DAE6F9<<3)>>24)&0xFF;
				ucTxBuffer[2]=(U8)((0x18DAE6F9<<3)>>16)&0xFF;
				ucTxBuffer[3]=(U8)((0x18DAE6F9<<3)>>8)&0xFF;
				ucTxBuffer[4]=(U8)((0x18DAE6F9<<3))&0xFF;
				ucTxBuffer[5] = 0x02;
				ucTxBuffer[6] = 0x27;
				ucTxBuffer[7] = 0x05;

				nLength = 8;
				//CanTx_OutEn_EX(TxBuff); // 2
				//Can0WriteBuff_29bit(CAN_CH, m_stRecordingControl.stListCtrl.TxBuff+5, 0x18DAE6F9, m_stRecordingControl.stListCtrl.TxBuff[0]);
				// send event to diagnostic thread	
//MONI				CovertBuffer2PassThruMessage(ucTxBuffer, nLength, &m_stRecordingControl.stCanCtrl.pPTpacket[0]);

				// need to check do app need to wait a response?
				// sending a data 
				if( SendPassThrouMessage(&m_stRecordingControl.stCanCtrl.stTxPacket) == true )
				{
				}
				else
				{
				}
			}
			else if(j==2)
			{
				if(m_stRecordingControl.stCanCtrl.stRxPacket.mData[8] == 0 && m_stRecordingControl.stCanCtrl.stRxPacket.mData[9] == 0)
				{
				
				}
				else 
				{
#ifdef ENABLE_RECORED_MALLOC                                  
					if(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5011 || m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 ==  0x5012 )
#else
					if(m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5011 || m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 ==  0x5012 )
#endif                                          
					{
						KEY1 = (m_stRecordingControl.stCanCtrl.stRxPacket.mData[8] << 3) ^ m_stRecordingControl.stCanCtrl.stRxPacket.mData[9];
						KEY2 = (m_stRecordingControl.stCanCtrl.stRxPacket.mData[9] >> 4) ^ m_stRecordingControl.stCanCtrl.stRxPacket.mData[8];
					}
#ifdef ENABLE_RECORED_MALLOC                                        
					else if(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5013 || m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5014)
#else
					else if(m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5013 || m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5014)
#endif                                          
					{
						KEY1 = (m_stRecordingControl.stCanCtrl.stRxPacket.mData[8] << 2) ^ m_stRecordingControl.stCanCtrl.stRxPacket.mData[9];
						KEY2 = (m_stRecordingControl.stCanCtrl.stRxPacket.mData[9] << 3) ^ m_stRecordingControl.stCanCtrl.stRxPacket.mData[8];
					}
				
					ucTxBuffer[0]=0x88;
					ucTxBuffer[1]=(uint8_t)((0x18DAE6F9<<3)>>24)&0xFF;
					ucTxBuffer[2]=(uint8_t)((0x18DAE6F9<<3)>>16)&0xFF;
					ucTxBuffer[3]=(uint8_t)((0x18DAE6F9<<3)>>8)&0xFF;
					ucTxBuffer[4]=(uint8_t)((0x18DAE6F9<<3))&0xFF;
					ucTxBuffer[5] = 0x04;
					ucTxBuffer[6] = 0x27;
					ucTxBuffer[7] = 0x06;
					ucTxBuffer[8] = KEY1;
					ucTxBuffer[9] = KEY2;

					nLength = 10;
					//CanTx_OutEn_EX(TxBuff);	// 3
					//Can0WriteBuff_29bit(CAN_CH, m_stRecordingControl.stListCtrl.TxBuff+5, 0x18DAE6F9, m_stRecordingControl.stListCtrl.TxBuff[0]);
					// send event to diagnostic thread	
//MONI					CovertBuffer2PassThruMessage(ucTxBuffer, nLength, &m_stRecordingControl.stCanCtrl.pPTpacket[0]);

					// need to check do app need to wait a response?
					// sending a data 
					if( SendPassThrouMessage(&m_stRecordingControl.stCanCtrl.stTxPacket) == true )
					{
					}
					else
					{
					}
				}
			}
			
			//m_stRecordingControl.PGN = 0xF9E6;	// receive, 18 DA F9 E6... 				
			//m_stRecordingControl.stListCtrl.intTxdRxdCount = m_stRecordingControl.stListCtrl.VP3_MIN;			
			//while(m_stRecordingControl.stListCtrl.intTxdRxdCount)
			//{
			//	//if(CanReceive_29BIT(PGN) == TRUE)
			//	if(Can0ReadBuff_29bit(CAN_CH, m_stRecordingControl.stListCtrl.DlcRxBuff, 0, m_stRecordingControl.PGN) == TRUE)
			//		break;
			//}
			//if(m_stRecordingControl.stListCtrl.intTxdRxdCount == 0)
			//{
			//	break;		//응답이 없어도 fail처리 하지 않는다
			//}
		}
	}
#ifdef ENABLE_RECORED_MALLOC	
	if(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5014)
#else
	if(m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.nEtc3 == 0x5014)
#endif
	{
		//MONI need to check about old pid process
#warning "need to check about old pid process"
		//VCI_HW_Setting(m_stRecordingControl.unPreProtocolType);


#ifdef ENABLE_RECORED_MALLOC        
		VCI_HW_Setting(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType);
#else
		VCI_HW_Setting(m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType);
#endif
		

	}
}


void SendRecordingMessage2DiagThread(PTmsgPkt_t	*pstTxPacket)
{
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;	
	uint32_t eInCommType = PACKET_RECORD;

	//GLogI("*");
	
	//MONI block can tx
	//VCI_SetPassThruWirteMsgTimeout(pPayloadPtcl, sizeof(unsigned long));

	pDiagmsg = ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
	if( pDiagmsg == NULL )
	{
		GLogE("%s] errorr malloc fail1\r\n", __func__);
		return;
	}
	pPTpacket = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
	if( pPTpacket == NULL )
	{
		GLogE("%s] errorr malloc fail2\r\n", __func__);
		osPoolFree( hDiagPool, (void *)pDiagmsg );
		return;
	}
	memcpy(pPTpacket, pstTxPacket, sizeof(PTmsgPkt_t));
	RxCANID_Set(pPTpacket->pData[0],pPTpacket->pData[1]);

	pDiagmsg->mMsgType 		= MSG_RECORD;
	pDiagmsg->mPktType 		= (ePKT_TD)eInCommType;
	pDiagmsg->event 		= DIAG_PASSTHRU;
	pDiagmsg->subEvent 		= eDIAG_COMM_TX_START;
	pDiagmsg->unEventTime 	= GetUnixTime();
	pDiagmsg->pPacket 		= (void *)pPTpacket;

	if(osMessageAvailableSpace(hDiagMsg) == 0)
	{
		osPoolFree( hPTPKPool, (void *)pPTpacket );
		osPoolFree( hDiagPool, (void *)pDiagmsg );
		GLogE("%s] errorr malloc fail3\r\n", __func__);
	}
	else
	{
		osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
	}

	//GLogI("%s] send can data\r\n", __func__);
}

bool CovertBuffer2PassThruMessage(uint8_t* pucPayload, uint32_t unLength, PTmsgPkt_t** ppPTpacket)
{
	//hexdump(pucPayload, unLength);
	
	if( *ppPTpacket == NULL )
	{
		GLogE("%s] error memory full\r\n", __func__);
		return false;
	}

	memset((*ppPTpacket)->pData, 0, MAX_PASSTHRUMSG_DATA_SIZE);

	(*ppPTpacket)->DataSize 		= unLength;
	(*ppPTpacket)->ExtraDataIndex 	= 4;
	(*ppPTpacket)->ProtocolID 		= 0;
	(*ppPTpacket)->RxStatus			= 0;
	(*ppPTpacket)->Timestamp		= 0;
	(*ppPTpacket)->TxFlags			= 0;
	
	if(unLength>0)
		memcpy((*ppPTpacket)->pData, pucPayload, unLength);

	return true;
}

void MakeOpenRecordMessage(uint8_t ucIndex, stECUIDInfo* pstEcuIdInfo, PTmsgPkt_t* pPTpacket)
{
	int32_t nLength = pstEcuIdInfo->ucCommCodes[ucIndex][0];
	uint8_t ucBuffer[256] = {0,};

	if( nLength > 256 )
	{
		printf("%s] error size over\r\n", __func__);
		nLength = 255;
	}

	memcpy(ucBuffer, &pstEcuIdInfo->ucCommCodes[ucIndex][1], nLength);	

	if( pstEcuIdInfo->unProtocolType == RS232_MCU )
	{
		ucBuffer[6] = 0x03;
		ucBuffer[7] = 0x00;
		ucBuffer[8] = 0x00;
		ucBuffer[10] = 0x6b;
		//ucBuffer[1][6]=0x03;
		//ucBuffer[1][7]=0x00;
		//ucBuffer[1][8]=0x00;
		//ucBuffer[1][10]=0x6b;
	}

	if( CovertBuffer2PassThruMessage(ucBuffer, nLength, &pPTpacket) == false )
	{
		GLogE("%s] error make message\r\n",__func__);

		return;
	}
}


void MakeCloseRecordMessage(uint8_t ucIndex, stECUIDInfo* pstEcuIdInfo, PTmsgPkt_t* pPTpacket)
{
	int32_t nLength = pstEcuIdInfo->ucCommCodes[ucIndex][0];
	uint8_t ucBuffer[256] = {0,};

	memcpy(ucBuffer, &pstEcuIdInfo->ucCommCodes[ucIndex][1], nLength);	

	ucBuffer[nLength-2] = 0x82;
	ucBuffer[nLength-1] = ucBuffer[nLength-1] + 1;

//MONI	CovertBuffer2PassThruMessage(ucBuffer, nLength, pPTpacket);
}


void MakeDtcRecordMessage(uint8_t ucIndex, stTriggerInfo* pstTrgInfo, PTmsgPkt_t* pPTpacket)
{
	int32_t nLength = pstTrgInfo->ucDtcReqCodes[ucIndex][0];
	uint8_t ucBuffer[256] = {0,};

	memcpy(ucBuffer, &pstTrgInfo->ucDtcReqCodes[ucIndex][1], nLength);	

	CovertBuffer2PassThruMessage(ucBuffer, nLength, &pPTpacket);
}


void MakeEngineStopMessage(uint8_t ucIndex, tagEngineStopTiggerInfo* pstEngStopCtrl, PTmsgPkt_t* pPTpacket)
{
    
	uint8_t ucBuffer[256] = {0,};	
	uint16_t usCanId = (pstEngStopCtrl->strRequestCode[0]<<8) + pstEngStopCtrl->strRequestCode[1];
	int32_t nLength = 11;

	ucBuffer[0]=8;
	ucBuffer[1]=(uint8_t)((usCanId<<5)>>8)&0xFF;
	ucBuffer[2]=(uint8_t)(usCanId<<5)&0xFF;


	CovertBuffer2PassThruMessage(ucBuffer, nLength, &pPTpacket);

	// need to check about making packet, payload is weired
}


void MakeRecordMessage(uint8_t ucIndex, stRecordItems* pstRecItemsInfo, PTmsgPkt_t* pPTpacket)
{
#ifdef ENABLE_RECORED_MALLOC
	int32_t nLength = pstRecItemsInfo->pucRecordItemReqCodes[ucIndex][0];
#else
	int32_t nLength = pstRecItemsInfo->ucRecordItemReqCodes[ucIndex][0];
#endif
	uint8_t ucBuffer[256] = {0,};

#ifdef ENABLE_RECORED_MALLOC
	memcpy(ucBuffer, &pstRecItemsInfo->pucRecordItemReqCodes[ucIndex][1], nLength);	
#else
	memcpy(ucBuffer, &pstRecItemsInfo->ucRecordItemReqCodes[ucIndex][1], nLength);	
#endif

	CovertBuffer2PassThruMessage(ucBuffer, nLength, &pPTpacket);

#if false
	printf("%s] recording monitoring data\r\n", __func__);
	hexdump(&pPTpacket->pData[0], pPTpacket->DataSize);
#endif

}

bool WaitDiagThreadResponsofRecordingSystem(stCommPkt*	pstRxPacket)
{
	osEvent 		event;
	MsgDiag_t		*message;
	stCommPkt		*packet;

	if( pstRxPacket == NULL )
	{
		GLogE("%s] error buffer is null\r\n",__func__);
		return false;
	}
	
	event = osMessageGet( hRecordDataMsg, MAX_CAN_RX_WAITING_TIME );
	//event = osMessageGet( hRecordDataMsg, osWaitForever );
	

	if( event.status != osEventMessage )
	{
		//! if events are not received loop will be continued.
		GLogE ("error!! WaitDiagThreadResponsofRecordingSystem\r\n");

		return false;
	}
#ifdef PRINT_MESSAGE_ID
	printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hRecordDataMsg));
#endif

	message = ( MsgDiag_t * )event.value.p;
	packet	= ( stCommPkt* )message->pPacket;
	

	//GLogI("^");

	// copy data to rx buffer of record can control
	memcpy( pstRxPacket, packet, sizeof(stCommPkt));

	// first distirbution of event 
	osPoolFree( hCommPKPool, (void *)packet );
	osPoolFree( hDiagPool, (void *)message );


	return true;
}


bool SendPassThrouMessage(PTmsgPkt_t* pPTpacket)
{
	// send event to diagnostic thread
	//SendRecordingMessage2DiagThread(m_stRecordingControl.stCanCtrl.pPTpacket[0]);


	SendRecordingMessage2DiagThread(pPTpacket);

	// wait event for result
	if( WaitDiagThreadResponsofRecordingSystem(&m_stRecordingControl.stCanCtrl.stRxPacket) == false )
	{
		GLogE("%s] error send record message\r\n",__func__);
		return false;
	}

	if( (m_stRecordingControl.stCanCtrl.stRxPacket.mLen - MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER) == 0 )
		return false;

	return true;
}

void UpdateMonitoringControl()
{
	m_stRecordingControl.stCanCtrl.ucCurCommIndex = 0;
#ifdef ENABLE_RECORED_MALLOC        
	if( m_stRecordingControl.pstCfgCtrl != NULL )
		m_stRecordingControl.stCanCtrl.ucMaxCommIndex = m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stRecInfo.usRecordItemCount;
#else
	m_stRecordingControl.stCanCtrl.ucMaxCommIndex = m_stRecordingControl.stCfgCtrl.stCfgInfo.stRecInfo.usRecordItemCount;
#endif        
}


eRecordingStateResult Record_OpenEcu()
{
	//GLogI("Step#6, Record_OpenEnu\r\n");	//do not use Log because you print log that make delay

	// set exception case ecu
	Record_OpenEcuException();
		
	// check next index
	if( m_stRecordingControl.stCanCtrl.ucCurCommIndex < m_stRecordingControl.stCanCtrl.ucMaxCommIndex )
	{
		// send event to diagnostic thread	
#ifdef ENABLE_RECORED_MALLOC          
		MakeOpenRecordMessage(m_stRecordingControl.stCanCtrl.ucCurCommIndex, &m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo, &m_stRecordingControl.stCanCtrl.stTxPacket);
#else
		MakeOpenRecordMessage(m_stRecordingControl.stCanCtrl.ucCurCommIndex, &m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo, &m_stRecordingControl.stCanCtrl.stTxPacket);
#endif                
		
		// sending a data
		if( SendPassThrouMessage(&m_stRecordingControl.stCanCtrl.stTxPacket) == true )
		{
			GLogI("StartComm OK,Record_OpenEcu \r\n");		
#ifdef ENABLE_RECORED_MALLOC			
			if( m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.ackmessage[0] != 0 )
#else
			if( m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.ackmessage[0] != 0 )
#endif                          
			{
				VCI_ACK();
			}
			
			// set next tx index
			m_stRecordingControl.stCanCtrl.ucCurCommIndex++;
		}
		else
		{
#if ENABLE_ECU_OPEN_CHECK							
			m_stRecordingControl.stCanCtrl.unCanCommErrorCount++;		//140401 LWH
#else
			// set next tx index
			m_stRecordingControl.stCanCtrl.ucCurCommIndex++;
#endif
			osDelay(100);

			if( m_stRecordingControl.stCanCtrl.ucCurCommIndex < m_stRecordingControl.stCanCtrl.ucMaxCommIndex )
				return eRecordingStateResult_Retry;
			else
			{
	            GLogE("StartComm fail \r\n");
				return eRecordingStateResult_Fail;
			}
		}
	}
	else
	{
		// clear can communication count
		m_stRecordingControl.stCanCtrl.unCanCommErrorCount = 0;

		// update recording info to control variable.
		UpdateMonitoringControl();

		if( m_stRecordingControl.bTriggerObdLedOn == false )
		{
			// send obd led on event to trigger module
			SendEvent2TriggerThread(eEVT_TRG_LED_OBD_ON);

			m_stRecordingControl.bTriggerObdLedOn = true;
		}


		return eRecordingStateResult_Success;
	}


	osDelay(100);

	return eRecordingStateResult_None;
}


bool CheckRecordEcuOpen()
{
	GLogI("CheckRecordEcuOpen\r\n");
	bool bResult = false;
	
	// set exception case ecu
	Record_OpenEcuException();
		
	// check next index
	if( m_stRecordingControl.stCanCtrl.ucCurCommIndex < m_stRecordingControl.stCanCtrl.ucMaxCommIndex )
	{
		// send event to diagnostic thread	
#ifdef ENABLE_RECORED_MALLOC          
		MakeOpenRecordMessage(m_stRecordingControl.stCanCtrl.ucCurCommIndex, &m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo, &m_stRecordingControl.stCanCtrl.stTxPacket);
#else
		MakeOpenRecordMessage(m_stRecordingControl.stCanCtrl.ucCurCommIndex, &m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo, &m_stRecordingControl.stCanCtrl.stTxPacket);
#endif                
		
		// sending a data
		if( SendPassThrouMessage(&m_stRecordingControl.stCanCtrl.stTxPacket) == true )
		{
			GLogI("StartComm OK,CheckRecordEcuOpen \r\n");		
#ifdef ENABLE_RECORED_MALLOC			
			if( m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet.ackmessage[0] != 0 )
#else
			if( m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.stHwSet.ackmessage[0] != 0 )
#endif                          
			{
				VCI_ACK();
			}			

			// clear error count in ecu open request
			m_stRecordingControl.stCanCtrl.unCanCommErrorCount = 0;

			bResult = true;
		}
		else
		{
            GLogE("StartComm fail \r\n");
							
			m_stRecordingControl.stCanCtrl.unCanCommErrorCount++;
		}

		osDelay(1);
		
		// set next tx index
		m_stRecordingControl.stCanCtrl.ucCurCommIndex++;

	}
	else
	{
		// clear request index
		m_stRecordingControl.stCanCtrl.ucCurCommIndex = 0;
	}
	
	// keep this state		
	SetFlightRecordingState(eRecord_WaitTriggerModule); 

	return bResult;
}


void LedControl()
{
#if false	
	if (Get_TmrDelta(Get_Tmr(), m_stRecordingControl.old_timer) >= 10000)
	{
		
		//GLogI(&U1,"@");
		RES0xC053();		//131021 LWH	//led요건변경 풀음
		m_stRecordingControl.old_timer = Get_Tmr(); 									//131120 LWH BT이 떨어졌다 붙었는데, 그것이 미처 0xC053으로 체크안된 경우가 있다.	
		
		if(BTConnect==1) RES0xD001(LED_OBD_II_ON);
		if(BTConnect==1) RES0xD00E(ENTER_KEY_ENABLE-40);
		
	}	
#endif	
}

void Record_CheckEngine()
{

	GLogI("%s] start\r\n", __func__);

	// 1프레임 엔진 회전수 항목 검출
#ifdef ENABLE_RECORED_MALLOC
	if( (m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stTrgInfo.ucTrigMode & 0x08) != 0x08 )
#else
        if( (m_stRecordingControl.stCfgCtrl.stCfgInfo.stTrgInfo.ucTrigMode & 0x08) != 0x08 )          
#endif          
	{		
		// CAN id 일 경우..
#ifdef ENABLE_RECORED_MALLOC          
		if(m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType != 0x06)
#else
		if(m_stRecordingControl.stCfgCtrl.stCfgInfo.stEcuIdInfo.unProtocolType != 0x06)
#endif                  
		{
			// done sending all open comm data 
			SetFlightRecordingState(eRecord_Monitoring);

			// skip checking engine stop
			return;
		}
	}
	
	// check next index
	if( m_stRecordingControl.stCanCtrl.ucCurCommIndex < m_stRecordingControl.stCanCtrl.ucMaxCommIndex )
	{
   
		// send event to diagnostic thread	
#ifdef ENABLE_RECORED_MALLOC          
		MakeEngineStopMessage(m_stRecordingControl.stCanCtrl.ucCurCommIndex, &m_stRecordingControl.pstCfgCtrl[0].stEngStopTrgInfo, &m_stRecordingControl.stCanCtrl.stTxPacket);
#else
		MakeEngineStopMessage(m_stRecordingControl.stCanCtrl.ucCurCommIndex, &m_stRecordingControl.stCfgCtrl.stEngStopTrgInfo, &m_stRecordingControl.stCanCtrl.stTxPacket);
#endif                
		
		// sending a data
		if( SendPassThrouMessage(&m_stRecordingControl.stCanCtrl.stTxPacket) == true )
		{
			GLogI("Engine Check OK \r\n");
			
			
			// set next tx index
			m_stRecordingControl.stCanCtrl.ucCurCommIndex++;
		}
		else
		{
            GLogE("Engine Check fail \r\n");

			// error occured
			SetFlightRecordingState(eRecord_Error);
							
			m_stRecordingControl.stCanCtrl.unCanCommErrorCount++;		//140401 LWH
		}

	}
	else
	{
		// done sending all open comm data 
		SetFlightRecordingState(eRecord_Monitoring);	
	}


}


void DisplayFreeSpaceInfo()
{
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;


    /* Get volume information and free clusters of drive 1 */
    FRESULT res = f_getfree("0:", &fre_clust, &fs);
    if (res)
	{
		GLogE("%s] error get free : result : %d\r\n", __func__, res);
		return;
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    /* Print the free space (assuming 512 bytes/sector) */
    GLogI("%s] %10lu KB total drive space. --> %10lu KB available.\r\n", __func__, tot_sect / 2, fre_sect / 2)	
}




eRecordingStateResult Record_Monitoring()
{
#ifdef ENABLE_TEST_SHORT_RECORD	
	static uint32_t s_unLogTimer = 0;
#endif
	static uint32_t s_unMonitoringTimeout = 0;

	// check next index
	if( m_stRecordingControl.stCanCtrl.ucCurCommIndex < m_stRecordingControl.stCanCtrl.ucMaxCommIndex )
	{
		// #Step 2, request recording data,
		//LedControl();

		// send monitoring event to 
		// send event to diagnostic thread	
#ifdef ENABLE_RECORED_MALLOC                
		MakeRecordMessage(m_stRecordingControl.stCanCtrl.ucCurCommIndex, &m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stRecInfo, &m_stRecordingControl.stCanCtrl.stTxPacket);
#else
		MakeRecordMessage(m_stRecordingControl.stCanCtrl.ucCurCommIndex, &m_stRecordingControl.stCfgCtrl.stCfgInfo.stRecInfo, &m_stRecordingControl.stCanCtrl.stTxPacket);
#endif                

#ifdef ENABLE_TEST_SHORT_RECORD
		if( Get_Tmr() - s_unLogTimer > 1000 )
		{
			GLogI("\r\n%s] ucCurCommIndex : %d, ucMaxCommIndex : %d\r\n",__func__, m_stRecordingControl.stCanCtrl.ucCurCommIndex, m_stRecordingControl.stCanCtrl.ucMaxCommIndex);
		}
#endif		

		// wait monitoring result
		// sending a data // time out 5secods,
		if( SendPassThrouMessage(&m_stRecordingControl.stCanCtrl.stTxPacket) == true )
		{
			//GLogI("record monitoring ok\r\n");

			// save records data to bakcup buffer
			UpdateRecordMsg2Backup(&m_stRecordingControl.stCanCtrl.stRxPacket);

#ifdef ENABLE_TEST_SHORT_RECORD
			if( Get_Tmr() - s_unLogTimer > 1000 )
			{
				GLogI("\r\n%s] update backup // record index : %d, DataSize : %d\r\n", __func__, m_stRecordingControl.stDataCtrl.usBackupDataPosition, m_stRecordingControl.stCanCtrl.stRxPacket.mLen);
			}
#endif

			// save backup data to file
			SaveBackupData2File(false);

			// set next tx index
			m_stRecordingControl.stCanCtrl.ucCurCommIndex++;

			// every 60 seconds check the file size
			if( (Get_Tmr() - s_unMonitoringTimeout) > FILE_SIZE_CHECK_OVER_TIME )
			{
				GLogI("record monitoring for DistributionDataFileofRecoredingTime\r\n");
				
				DistributionDataFileofRecoredingTime(false);

				s_unMonitoringTimeout = Get_Tmr();
			}

			m_stRecordingControl.stCanCtrl.unCanCommErrorCount = 0;
		}
		else
		{
			GLogE("record monitoring fail\r\n");

			// error occured						
			if( m_stRecordingControl.stCanCtrl.unCanCommErrorCount++ > 3 )
			{
				SetFlightRecordingState(eRecord_HwSetting);
				g_unSleepWaitTimeout = Get_Tmr();
			}

			osDelay(1000);
		}

#ifdef ENABLE_TEST_SHORT_RECORD
		if( Get_Tmr() - s_unLogTimer > 1000 )
		{
			//MONI delete after test
			DisplayFreeSpaceInfo();
			s_unLogTimer = Get_Tmr();
		}
#endif

	}
	else
	{
		// done sending all open comm data 
		SetFlightRecordingState(eRecord_Monitoring);

		// initalize recording index for resending a record data.
		m_stRecordingControl.stCanCtrl.ucCurCommIndex = 0;

		//GLogI("%s] all recording data was sent, initialize index\r\n",__func__);
	}


	return eRecordingStateResult_Success;
}


eRecordingStateResult Record_InitKwp()
{
#if false	
	GLogI("Step#5, Record_ReadConfig\r\n");
	GLogI("\n\r Fast Init... ucTrigMode: %02X", m_stRecordingControl.ucTrigMode );

	DLC_RX_BUFF_CLEAR();
	
	if(m_stRecordingControl.stListCtrl.ProtocolType==ISO14230_POWERTEC)		
	{
//				KW_fast_init_Powertec(VTINIL,(VTWUP-VTINIL));	// Powertec Protocol by kyc 2007.06.19
		Delay(800);
//				KW_fast_init_Powertec(VTINIL,(VTWUP-VTINIL));	// Powertec Protocol by kyc 2007.06.19
	}
	else
	{
		KW_fast_init(VTINIL,(VTWUP-VTINIL));
	}

	DLC_RX_BUFF_CLEAR();		//꼭 해줘야 함!! fastinit 파형을 rx으로 판단하고 있어 버퍼에 0x00이 들어옴 130802 LWH
	m_stRecordingControl.stListCtrl.CurMode = TX_BLOCK;

	while(1)
	{
		int i = VCI_RECORD_COMM();
		if( i == Rx_Completed )
		{					
			
			GLogI("\nStartComm OK %d \n", m_stRecordingControl.eRcdState);
			if (f_open(&stRecFile, STORE_REC_TEMP_FILE_NAME, FA_CREATE_ALWAYS) == FR_OK )
			{
				GLogI("\n\r %s File Open Success\r\n", STORE_REC_TEMP_FILE_NAME);
			}
			
			m_stRecordingControl.stListCtrl.DlcTxBlock = 0;
			m_stRecordingControl.stDtcCtrl.DtcTxBlock = 0;
			m_stRecordingControl.stDtcCtrl.RecordCommCnt = 30; // DTC Request one time per anothre Request 30th times

			m_stRecordingControl.ePreRcdState = m_stRecordingControl.eRcdState;
			m_stRecordingControl.eRcdState = eFR_CurNodeREQ;
			
			return;
		}
		else if( i == Rx_Fail )
		{
		 	DLC_HW_Clear();
			m_stRecordingControl.eRcdState = eRecord_ReadConfig;

		 	GLogI("\n\r FastInit Error");
		 	for(i=0; i<m_stRecordingControl.RecordItemCount; i++)
			{
				free(m_stRecordingControl.stListCtrl.SBYTE_ARRAYBuff_test[i].BytePtr);	
			}
		 	
			m_stRecordingControl.unCanCommErrorCount++;
		 	return;
		}
	}
#endif	

	return eRecordingStateResult_Success;
}


eRecordingStateResult Record_Idle()
{
	// active recording
	if( GetActiveFlightRecording() == true )
	{
		if( GetCurFwServiceMode() == eApp_Inside )
		{
			if( m_stRecordingControl.bConfigError == false )
			{		
				if( isTriggerInterfaceMode(eTrgMonitorMode_Menual) == true )
				{
					SetFlightRecordingState(eRecord_WaitTriggerModule);
				}

				if( isTriggerInterfaceMode(eTrgMonitorMode_EngineStart) == true || isTriggerInterfaceMode(eTrgMonitorMode_EngineStop) == true )
				{
					SetFlightRecordingState(eRecord_WaitTriggerModule);
				}
			}
			else
			{
				GLogE("%s] recording file was broken\r\n", __func__);
			}		
		}
		else
		{
			//GLogI("%s] service is not recording mode\r\n", __func__);
		}
	}

	osDelay(1000);

	return eRecordingStateResult_Success;
}

void SystemStandby()
{

	// git_can
	deinitFDCan();

	// git_mmc
	//kkt deinitMMC();
	
	// git_OBDcomm
	deinitOBDComm();

	// git_protocol
	deinitGitProtocol();

	// wlan
	deinitRS9116();

	gPMFlag = WAKEUP_SOURCE_HCAN1 | WAKEUP_SOURCE_HCAN2 | WAKEUP_SOURCE_LCAN | WAKEUP_SOURCE_12V_DET;

	gotoStandbyMode( gPMFlag );
	while(1);
}

eRecordingStateResult Record_Error()
{
	GLogI("Record_Error\r\n");
	SetFlightRecordingState(eRecord_Idle);

	// if can didn't work go to sleep 
	if(m_stRecordingControl.stCanCtrl.unCanCommErrorCount >= MAX_CAN_COMMUNICATION_ERROR_COUNT)	
	{
		// send event to trigger module to stop sending event to trigger module		
		LED_ALL_OFF;
		
		m_stRecordingControl.bTriggerObdLedOn = false;
		
		//send obd led on event to trigger module
		//SendEvent2TriggerThread(eEVT_TRG_LED_OBD_OFF);
		//osDelay(1000);
		
		// send event to trigger module for waiting data save. green led blink 5
		SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_GREEN, eTM_LED_GREEN_OFF);
		osDelay(1000);

		//SendByteEvent2TriggerModule(eEVT_TM_CTRL_TRIGGER, eTM_TRIGGER_OFF);
		//osDelay(10000);
	
		// send obd led on event to trigger module
		SendByteEvent2TriggerModule(eEVT_TM_CTRL_SLEEP, eTM_SLEEE_ACTIVE);	
		osDelay(1000);

		//rsi_bt_disconnect(GetBtConnectedAddr());
		//osDelay(3000);

		// request sleep 
		SystemStandby();
	}


	osDelay(1000);

	printf("%s] error communication count : %d\r\n", __func__, m_stRecordingControl.stCanCtrl.unCanCommErrorCount);

	return eRecordingStateResult_Success;
}


eRecordingStateResult Record_Sleep()
{
#if false
	osEvent event = osSignalWait(SIGNAL_RECORD_WAKE_UP, MAX_RECORD_SLEEP_WAIT_TIME_OUT );
	if(  event.status == osEventSignal )
	{	
		GLogI("%s] wait up from sleep\r\n", __func__);

		SetFlightRecordingState(eRecord_WaitTriggerModule);
	}	
#endif

	return eRecordingStateResult_Success;
}


uint8_t DecToHex(int32_t data)
{
	unsigned char CharTemp=0,CharTemp1=0,CharTemp2=0;
	
	CharTemp1 = data/10;
	CharTemp2 = data%10;
	CharTemp = CharTemp1*0x10+CharTemp2;	// Hex 값
	
	return CharTemp;
}


void DecToAscii(uint32_t data, uint8_t* pcAscii)
{
	unsigned char CharTemp[2] = {0,};
	
	CharTemp[0] = data/10;
	CharTemp[1] = data%10;
	
	if((CharTemp[0]+0x30>0x29)&&(CharTemp[0]+0x30<0x3A))		// 숫자
	{
		CharTemp[0] = CharTemp[0]+0x30;
	}
	else
	{
		GLogE("RTC Data is wrong\r\n");
	}
		
	if((CharTemp[1]+0x30>0x29)&&(CharTemp[1]+0x30<0x3A))		// 숫자
	{
		CharTemp[1] = CharTemp[1]+0x30;
	}
	else
	{
		GLogI("RTC Data is wrong\r\n");
	}

	memcpy(pcAscii, CharTemp, 2);
}

uint32_t WrBkRam(uint8_t* pucSrc, uint32_t unDataSize)
{
	if(pucSrc == NULL )
	{
		GLogE("%s] error buffer is null\r\n",__func__);
		return 0;
	}

	if( m_stRecordingControl.stDataCtrl.usBackupDataPosition + unDataSize < MAX_BACKUP_BUFFER_LENGTH + 512 )
	{
		memcpy(&m_stRecordingControl.stDataCtrl.ucBackupData[m_stRecordingControl.stDataCtrl.usBackupDataPosition], pucSrc, unDataSize);
		
		// update packet length
		m_stRecordingControl.stDataCtrl.usBackupDataPosition += unDataSize;
	}
	else
	{
		unDataSize = 0;
	}

	return unDataSize;
}

void ClearBkRam()
{
	m_stRecordingControl.stDataCtrl.usBackupDataPosition = 0;
	//memset(m_stRecordingControl.stDataCtrl.ucBackupData,0,sizeof(m_stRecordingControl.stDataCtrl.ucBackupData));
}

bool isAvailableBkRam()
{
	if( m_stRecordingControl.stDataCtrl.usBackupDataPosition < MAX_BACKUP_BUFFER_LENGTH )
		return true;

	return false;
}


void ActFirstTriggerEnter()
{
	GLogI("%s] ==================================================================\r\n", __func__);
	GLogI("%s] ==================================================================\r\n", __func__);
	GLogI("%s] TRIGER START\r\n", __func__);
	GLogI("%s] ==================================================================\r\n", __func__);
	
	// save tmpbuffer data to file
	SaveBackupData2File(false);

//MONI 20230106 not needed distribution of file
#if false 
	GLogI("%s] recording time : %d, used recording time : %d\r\n", __func__, m_stRecordingControl.stDataCtrl.unMaxRecordingTime, (uint32_t)((Get_Tmr()-m_stRecordingControl.stDataCtrl.unUsedRecTime)/1000));

	if( m_stRecordingControl.stDataCtrl.unMaxRecordingTime <= (uint32_t)((Get_Tmr()-m_stRecordingControl.stDataCtrl.unUsedRecTime)/1000) )
	{
		// save distribution with setting time, 10m, 30m and 60, before first trigger enter		
		DistributionDataFileofRecoredingTime(true);
	}
#endif	
}

void ActSecondTriggerStop()
{
	GLogI("%s] ==================================================================\r\n", __func__);
	GLogI("%s] ==================================================================\r\n", __func__);
	GLogI("%s] ==================================================================\r\n", __func__);
	GLogI("%s] RECORD FILE GENERATE\r\n", __func__);
	GLogI("%s] ==================================================================\r\n", __func__);
	// update tail packet of recording in backup buffer
	UpdateFlightRecordingTailPacket();
	// save backup buffer between first trigger enter and second trigger stop	
	SaveBackupData2File(true);

	// make reporting reocording file.
	MakeFlightRecordFile();
	GLogI("%s] ==================================================================\r\n", __func__);
	GLogI("%s] ==================================================================\r\n", __func__);	

	ClearTriggerDtcData();
	ClearTriggerMonitoringHandler();

	InitializeRecordingInfo();
}

void MakeRecordTailPacket(uint8_t* pucPacket, uint32_t* punLength)
{
	if( pucPacket == NULL )
	{
		GLogI("%s] error : buffer is null\r\n", __func__);
		return;
	}
	
	pucPacket[0] = 0xF5;
	pucPacket[1] = 0xF6;
	pucPacket[2] = 0xF7;
	pucPacket[3] = 0xF8;
	pucPacket[4] = 0xF9;
	pucPacket[5] = 0xFA;

	for( int i = 0; i < 10; i++ )
	{
		pucPacket[6+i] = m_stRecordingControl.stDataCtrl.strStartTime[i];
		pucPacket[16+i] = m_stRecordingControl.stDataCtrl.strTrigTime[i];
		pucPacket[26+i] = m_stRecordingControl.stDataCtrl.strEndTime[i];
	}

	GLogI("%s] record start time\r\n", __func__);
	hexdump(m_stRecordingControl.stDataCtrl.strStartTime, MAX_TRIP_TIME_LENGTH);
	GLogI("%s] record trig time\r\n", __func__);
	hexdump(m_stRecordingControl.stDataCtrl.strTrigTime, MAX_TRIP_TIME_LENGTH);
	GLogI("%s] record end time\r\n", __func__);
	hexdump(m_stRecordingControl.stDataCtrl.strEndTime, MAX_TRIP_TIME_LENGTH);


	// get dtc data from trigger thread
	GetTriggerDtcData(&m_stRecordingControl.stDtcCtrl.ucTrigDtcBlock[0]);
		
	for( int i = 0; i < MAX_TRIGGER_DTC_LENGTH; i++ )								
	{
		pucPacket[36+i] = m_stRecordingControl.stDtcCtrl.ucTrigDtcBlock[i];
	}

	GLogI("%s] record trigger dtc :\r\n", __func__);
	hexdump(m_stRecordingControl.stDtcCtrl.ucTrigDtcBlock, MAX_TRIGGER_DTC_LENGTH);

	GLogI("%s] record trigger mode : %d\r\n", __func__, m_stRecordingControl.stDtcCtrl.usTrigMode);
	GLogI("%s] record trigger message count : %d\r\n", __func__, m_stRecordingControl.stDataCtrl.usMsgCount);

	
	pucPacket[36 + 40 ] = m_stRecordingControl.stDtcCtrl.usTrigMode&0x0F;
	pucPacket[36 + 40 + 1] = m_stRecordingControl.stDtcCtrl.usTrigMode>>4;

	pucPacket[36 + 40 + 2] = m_stRecordingControl.stDataCtrl.usMsgCount&0x0F;
	pucPacket[36 + 40 + 3] =m_stRecordingControl.stDataCtrl.usMsgCount>>4;	

	*punLength = 80;
}

void UpdateFlightRecordingTailPacket()
{	
	// save header after second trigger stop
	uint32_t ucRecordTailPacketLength=0;
	uint8_t ucRecordTailPacket[TMP_BUFF_SIZE]={0,};

	// make recored tail packet 
	MakeRecordTailPacket(ucRecordTailPacket,&ucRecordTailPacketLength);
	
	if(ucRecordTailPacketLength == WrBkRam(ucRecordTailPacket,ucRecordTailPacketLength))
	{	
		GLogI("2 SaveHeader Write ok\r\n");
	}
	else
	{	
		GLogE("2 SaveHeader Write error\r\n");
		GLogE("2 SaveHeader Write error\r\n");
		GLogE("2 SaveHeader Write error\r\n");
	}
}

void UpdateRecordMsg2Backup( stCommPkt* pstRxPacket )
{
	uint16_t usLength;
	//uint8_t ucRecordPacketBuff[TMP_BUFF_SIZE] = {0,};
	uint8_t ucRecordPacketBuff[256] = {0,};
	uint32_t unRecordPacketLength;
	uint8_t* pucRxBuffer;

#ifdef ENABLE_TEST_DATA_FEEDER
	pstRxPacket->mLen = 100;

	for(int i=0;i<pstRxPacket->mLen;i++)
	{
		pstRxPacket->mData[i]=i;
	}
#endif

	if( pstRxPacket->mLen ==  0 )
	{
		GLogE("error data length is 0\r\n");
		return;
	}

	// adjust minus MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER because diagnostic add 24 length
	usLength = pstRxPacket->mLen - MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER;// - pstRxPacket->ExtraDataIndex;
	pucRxBuffer = &pstRxPacket->mData[MAX_PASSTHRU_MSG_OFFSET_WITH_TRIGGER];

	//hexdump(pucRxBuffer, usLength);
       
	// save only monitoring data exclude open / close / dtc
	if( m_stRecordingControl.eRcdState == eRecord_Monitoring )
	{
		// first packet index is 4, : include first packet index(0xF5,F6,F7) and size
		// after first packet index is 1 : include only size 
		// can id was followed after size.
		MakeRecordPacket(ucRecordPacketBuff, &unRecordPacketLength, &pucRxBuffer[0], usLength);

		// backup ram display
		//hexdump(&pucRxBuffer[0], usLength);
			
	
		if(unRecordPacketLength != WrBkRam(ucRecordPacketBuff,unRecordPacketLength))
		{	
			GLogI("%s] Backup Frame Write error %d\r\n", __func__, unRecordPacketLength);	
			return;
		}
	}
	
	// show temp data
	//DisplayFileData(STORE_REC_TEMP_FILE_NAME);
}

bool StartRecordThread( void )
{
	hRecordAppMsg = osMessageCreate( osMessageQ( recordappqueue ), NULL );
	if( hRecordAppMsg == NULL )
	{
		GLogE( "error... osMessageCreate hRecordAppMsg\r\n" );
		return false;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hRecordAppMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hRecordAppMsg",sizeof("hRecordAppMsg"));
#endif

	hRecordDataMsg = osMessageCreate( osMessageQ( recorddataqueue ), NULL );
	if( hRecordDataMsg == NULL )
	{
		GLogE( "error... osMessageCreate hRecordDataMsg\r\n" );
		return false;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hRecordDataMsg;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hRecordDataMsg",sizeof("hRecordDataMsg"));
#endif

	hRecordAppTh = osThreadCreate( osThread(recordappth), NULL );
	if( hRecordAppTh == NULL )
	{
		GLogE( "Error... fail create hBTAppTh Thread!!!\r\n" );
                return false;
	}
 
	return true;
}


void SendEvent2RecordThread(eEvtRecording eEvt, uint8_t* pucPayload, uint32_t unLength)
{
	// send event 
	MsgDiag_t	*pRecordmsg;
	PTmsgPkt_t	*pPTpacket;
	
	GLogI("*");
	GLogI("%s] MsgRecord_t size : %d\r\n", __func__, sizeof(MsgDiag_t));

	pRecordmsg	= ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
	if( pRecordmsg == NULL )
	{
		return;
	}
	
	pPTpacket = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
	if( pPTpacket == NULL )
	{
		GLogE("%s] errorr malloc fail2\r\n", __func__);
		osPoolFree( hDiagPool, (void *)pRecordmsg );
		return;
	}	

	if( pPTpacket != NULL && unLength > 0 )
	{
		// check data size for packing 
		pPTpacket->DataSize = unLength;
		pPTpacket->ExtraDataIndex = 0;
		
		memcpy(pPTpacket->pData, pucPayload, unLength);
	}

	
	pRecordmsg->mMsgType		= MSG_RECORD;
	pRecordmsg->mPktType		= PACKET_UART;
    pRecordmsg->event			= (uint16_t)eEvt;
    pRecordmsg->subEvent		= 0;
    pRecordmsg->result			= 0;
    pRecordmsg->seq				= 0;
    pRecordmsg->unTracePs		= 0;
    pRecordmsg->unEventTime		= 0;
	pRecordmsg->pPacket 		= pPTpacket;


#ifdef USE_DIAG_CHECKSUM
	pRecordmsg->usCS			= 0;
	
	pRecordmsg->usCS 	= CalcChecksumPayloadFrame((uint8_t*)pRecordmsg, sizeof(MsgDiag_t));

	GLogI("%s] msgrecord memory : %x, checkesum : %d\r\n", __func__, pRecordmsg, pRecordmsg->usCS);
#endif
        
	if(osMessageAvailableSpace(hRecordAppMsg) == 0)
	{
		osPoolFree( hDiagPool, (void *)pRecordmsg );
		osPoolFree( hPTPKPool, (void *)pPTpacket );
	}
	else
	{
		osMessagePut( hRecordAppMsg, (uint32_t)pRecordmsg, osWaitForever );
	}	
}


bool GetRecordEvent(osEvent event, uint16_t* pusEvent, uint16_t* pusSubEvent, 	PTmsgPkt_t	*pPTpacket)
{
	// send event 
	MsgDiag_t *pRecordmsg;
	
#ifdef USE_DIAG_CHECKSUM
	uint16_t usCheckSum;
#endif        
	
	//GLogI("*");
	//GLogI("%s] MsgDiag_t size : %d\r\n", __func__, sizeof(MsgDiag_t));
	
    if( event.value.p == NULL ) 
    {
      GLogI("%s] error buffer is null\r\n", __func__);
      return false;
    }
        
	pRecordmsg	= ( MsgDiag_t* )event.value.p;


	if( pRecordmsg->pPacket != NULL )
	{
		memcpy(pPTpacket, pRecordmsg->pPacket, sizeof(PTmsgPkt_t));
	}
	
#ifdef USE_DIAG_CHECKSUM
	usCheckSum = pRecordmsg->usCS; 	
	pRecordmsg->usCS = 0;

	GLogI("%s] msgrecord memory : %x, checkesum : %d\r\n", __func__, pRecordmsg, usCheckSum);

	if( usCheckSum != CalcChecksumPayloadFrame((uint8_t*)pRecordmsg, sizeof(MsgDiag_t)) )
	{
		GLogE("%s] error event was broken\r\n", __func__);
		
		// clear event
		osPoolFree( hDiagPool, (void *)pRecordmsg );
		return false;	
	}
#endif
        
	*pusEvent = pRecordmsg->event;
	*pusSubEvent = pRecordmsg->subEvent;

	// clear event
	osPoolFree( hDiagPool, (void *)pRecordmsg );

	if( pRecordmsg->pPacket != NULL )
	{
		osPoolFree( hPTPKPool, (void *)pRecordmsg->pPacket );
	}


	return true;
}

void UpdateCurrentRtcInfo(uint8_t* pucTime, uint8_t ucLength)
{
	uint8_t ucRtcData[MAX_TRIP_TIME_LENGTH] = {0,};
	
	memset(pucTime,0x00,ucLength);	

	VCI_GetRtcTime(ucRtcData);

	pucTime[0]=0x20;
	for( int i = 0; i < 7; i++ )
	{
		pucTime[i+1] = DecToHex(ucRtcData[i]);
	}	
}

void UpdateMonitoringInfo()
{
	m_stRecordingControl.stCanCtrl.ucCurCommIndex = 0;
	m_stRecordingControl.bTriggerObdLedOn = false;	
}


void UpdateOpenEcuInfo()
{
	m_stRecordingControl.stCanCtrl.ucCurCommIndex = 0;	

	
#ifdef ENABLE_RECORED_MALLOC        
	if( m_stRecordingControl.pstCfgCtrl != NULL )
		m_stRecordingControl.stCanCtrl.ucMaxCommIndex = m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.ucCommCodeCount;
#else
		m_stRecordingControl.stCanCtrl.ucMaxCommIndex = m_stRecordingControl.stCfgCtrl.stCfgInfo.stRecInfo.usRecordItemCount;
#endif  

	
	m_stRecordingControl.bTriggerObdLedOn = false;	
}


void UpdateRecordStartInfo()
{
	// trigger button active flag
	m_stRecordingControl.bActiveTrigger = false;

	// recording time control data
	m_stRecordingControl.stDataCtrl.unUsedRecTime = Get_Tmr();

	// not used
	m_stRecordingControl.stDataCtrl.usMsgCount = 0;

	// start recording time
	UpdateCurrentRtcInfo(m_stRecordingControl.stDataCtrl.strStartTime,MAX_TRIP_TIME_LENGTH);
}


void UpdateTriggerControlInfo(bool bActive)
{
	m_stRecordingControl.bActiveTrigger = bActive;
	
	// tigger button pressed
	if( bActive == true )
	{
		m_stRecordingControl.stDataCtrl.unTriggerTime = Get_Tmr();
		
		// trigger occurred time
		UpdateCurrentRtcInfo(m_stRecordingControl.stDataCtrl.strTrigTime,MAX_TRIP_TIME_LENGTH);	
	}
	else
	{
		// stop record time
		UpdateCurrentRtcInfo(m_stRecordingControl.stDataCtrl.strEndTime,MAX_TRIP_TIME_LENGTH);	
	}
}

void InitializeSystemModeHandler()
{
	m_eRecordingModeState = eApp_VCI_2;
	m_eOldRecordingModeState = eApp_VCI_2;
}

void RecordingSystemModeHandler()
{

	m_eRecordingModeState = GetCurFwServiceMode();

	if( m_eRecordingModeState != m_eOldRecordingModeState )
	{
		printf("%s] mode changed, go to initialize for reading config cur state : %d, old state : %d\r\n", __func__, m_eRecordingModeState, m_eOldRecordingModeState);	
		
		// new mode is recording mode
		if( m_eRecordingModeState == eApp_VCI_2 )
		{
			// stop data recording and changed state
			SetFlightRecordingState(eRecord_Idle);
		}

		if( m_eRecordingModeState == eApp_Inside )
		{
			SetFlightRecordingState(eRecord_Init);
			m_stRecordingControl.bConfigError = false;

			SendEvent2TriggerThread(eEVT_TRG_CLEAR);
		}
		
		m_eOldRecordingModeState = m_eRecordingModeState;
	}
}

AppName GetRecordingSystemModeState()
{
	return m_eOldRecordingModeState;
}

void SetRecordingSystemModeState(AppName eState)
{
	m_eOldRecordingModeState = eState;
}

void EvtHandlerFlightRecording(osEvent event)
{
	uint16_t usEvent,usSubEvent;
	PTmsgPkt_t	stPTpacket;

	if( (uint32_t)event.value.p < 0x20000000 )
	{
		GLogE("%s] error stack was broken must be reset\r\n",__func__);
		return;
	}

	
	// convert event and clear malloc buffer	
	GetRecordEvent(event, &usEvent, &usSubEvent, &stPTpacket);

	if( usEvent == eEVT_REC_TRIGGER_START )
	{
		GLogI("%s] rcv trigger start\r\n", __func__);
		
		if( m_stRecordingControl.bActiveTrigger == false )
		{
			// trigger start
			// save all data that reived in buffer
			//m_stRecordingControl.
			ActFirstTriggerEnter();

			UpdateTriggerControlInfo(true);

			// send event to trigger module for waiting data save. green led blink 5
			SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_GREEN, eTM_LED_GREEN_BLINK);

			LED_ALL_OFF;
			LED_SetState(eLED_REC_COMM, 0xffffffff, 100);


			m_stRecordingControl.stDtcCtrl.usTrigMode = eTrgMonitorMode_Menual;
		}
		else
		{
			GLogE("%s] in process to make recording file process\r\n", __func__);
		}
	}
	else if( usEvent == eEVT_REC_TRIGGER_STOP )
	{
		GLogI("%s] rcv trigger stop\r\n", __func__);
		// trigger stop
		UpdateTriggerControlInfo(false);

		
		// send event to trigger module for waiting data save. green led blink 5
		SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_GREEN, eTM_LED_GREEN_BLINK);

		
		// recording until time out
		ActSecondTriggerStop();


		// send event to tiggger module for save complete
		SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_GREEN, eTM_LED_GREEN_ON);
		//SendByteEvent2TriggerModule(eEVT_TM_CTRL_TRIGGER, eTM_TRIGGER_ON);
		

		

		SetFlightRecordingState(eRecord_WaitTriggerModule);


		// send trig clear for new recording data
		SendEvent2TriggerThread(eEVT_TRG_LED_OBD_ON);


		LED_ALL_OFF;
		LED_SetState(eLED_REC_COMM, 0xffffffff, 500);

		
	}
	else if( usEvent == eEVT_REC_TRIGGER_CONNECTED )
	{
		m_stRecordingControl.bTrgModuleConnected = true;

		if( m_stFwWakeupInfo.bActiveRecording == false )
		{
			// for recording
			SetActiveFlightRecording(true);

			// for wakeup info
			UpdateFwWakeupInfo(true);

			// initialize recording mode;
			InitlalizeRecordingState();
		}
		
		// make start soude
		//SendByteEvent2TriggerModule(eEVT_TM_CTRL_TRIGGER, eTM_TRIGGER_ON);
		SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_GREEN, eTM_LED_GREEN_ON);		
	}
	else if( usEvent == eEVT_REC_TRIGGER_DISCONNECT )
	{	
		m_stRecordingControl.bTrgModuleConnected = false;

//#define ENABLE_SAVE_LAST_DATA_AFTER_BT_DISCONNECTION
#ifdef ENABLE_SAVE_LAST_DATA_AFTER_BT_DISCONNECTION
		// changed record status to waiting for new recording
		ActSecondTriggerStop();
#endif
		// not need this state 
		//SetFlightRecordingState(eRecord_Idle);
	}	
	else if( usEvent == eEVT_REC_DTC_TRIGGER_START )
	{		
		GLogI("%s] rcv dtc trigger start\r\n", __func__);

		if( m_stRecordingControl.bActiveTrigger == false )
		{
			// skip generate recording file in bluetooth trigger mode and not connected
			if( isTriggerInterfaceMode(eTrgMonitorMode_Menual) == true )
			{
				if( m_stRecordingControl.bTrgModuleConnected == false )
				{
					GLogE("%s] in bluetooth trigger mode, bluetooth didn't connected\r\n", __func__);
					return;
				}
			}
		
			// trigger start
			// save all data that reived in buffer
			//m_stRecordingControl.
			ActFirstTriggerEnter();

			UpdateTriggerControlInfo(true);

			// send event to trigger module for waiting data save. green led blink 5
			SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_GREEN, eTM_LED_GREEN_BLINK);

			LED_SetState(eLED_REC_COMM, 0xffffffff, 100);

			// copy trigger block data
			memcpy(m_stRecordingControl.stDtcCtrl.ucTrigDtcBlock, stPTpacket.pData, stPTpacket.DataSize);

			m_stRecordingControl.stDtcCtrl.usTrigMode = eTrgMonitorMode_Dtc;
		}
		else
		{
			GLogE("%s] in process to make recording file process\r\n", __func__);
		}
	}
	else if( usEvent == eEVT_REC_FREEZE )
	{	
		GLogI("%s] freeze recording system for dtc process\r\n", __func__);

		osSignalSet(GetTriggerHandle(), SIGNAL_TRIGGER_FREEZE_RESPONSE);

		osDelay(1);
		
		// set signal to tirgger thread start triggger
		osSignalWait(SIGNAL_RECORD_RESUME,  osWaitForever);
	}
	else if( usEvent == eEVT_REC_RESUME )
	{	
		//m_stRecordingControl.bTrgModuleConnected = false;
	}
	else if( usEvent == eEVT_REC_ENGINE_START )
	{
		GLogI("%s] engine start\r\n", __func__);
		//SetFlightRecordingState(eRecord_WaitTriggerModule);
	}
	else if( usEvent == eEVT_REC_ENGINE_STOP )
	{
		GLogI("%s] engine stop\r\n", __func__);

		if( m_stRecordingControl.bActiveTrigger == false )
		{
			m_stRecordingControl.stDtcCtrl.usTrigMode = eTrgMonitorMode_EngineStop;

			// save record data to file
			ActFirstTriggerEnter();

			UpdateTriggerControlInfo(true);
			
			UpdateTriggerControlInfo(false);

			// send event to trigger module for waiting data save. green led blink 5
			SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_GREEN, eTM_LED_GREEN_BLINK);

			// trigger stop
			// recording until time out
			ActSecondTriggerStop();

			// send event to tiggger module for save complete
			SendByteEvent2TriggerModule(eEVT_TM_CTRL_LED_GREEN, eTM_LED_GREEN_ON);
			//SendByteEvent2TriggerModule(eEVT_TM_CTRL_TRIGGER, eTM_TRIGGER_ON);
			
			SetFlightRecordingState(eRecord_WaitTriggerModule);

			// send trig clear for new recording data
			SendEvent2TriggerThread(eEVT_TRG_LED_OBD_ON);

			LED_ALL_OFF;
			LED_SetState(eLED_REC_COMM, 0xffffffff, 0);
			
		}
		else
		{
			GLogE("%s] in process to make recording file process\r\n", __func__);
		}
	}
	else if( usEvent == eEVT_REC_CLEAR )
	{
		GLogI("%s] initialize recording monitoring state\r\n", __func__);

		//InitializeSystemModeHandler();		
		// set default system mode
		//RecordingSystemModeHandler();		
		//InitlalizeRecordingState();		
	}
}

bool isTriggerInterfaceMode(uint8_t ucMode)
{
	if( m_stRecordingControl.bConfigError == true )
	{
		printf("%s] error config is broken\r\n", __func__);
		return false;
	}
	
	if( (m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stTrgInfo.ucTrigMode & ucMode) == ucMode )
	{
		return true;
	}

	return false;
}

bool isTriggerModeActive()
{
	// usb / wireless bluetooth trigger mode
	if( isTriggerInterfaceMode(eTrgMonitorMode_Menual) == true )
	{
		return true;		
	}
	
	// engine start trigger mode not usb/wirelss bluetooth trigger mode
	if( isTriggerInterfaceMode(eTrgMonitorMode_Dtc) == true ||
		(isTriggerInterfaceMode(eTrgMonitorMode_EngineStart) == true || isTriggerInterfaceMode(eTrgMonitorMode_EngineStop) == true) )
	{
		return true;
	}

	return false;
}


// recodring status
// 1. is reading a config needed every time?
// 2. is it needed sending a close can communication?

// vci2 control can / dtc in a one thread
// reading config -> hw setting -> send open can communication -> send dtc -> send recording can -> trigger -> save file -> reading config


// vci3 control can / dtc different thread
// can // recording 
// dtc // trigger
// trigger
// 1. if changed config then reading a config -> if changed config then hw setting -> send open can communication -> send dtc -> trigger -> send event -> waiting event
// recording
// 1. if changed config then reading a config -> if changed config then hw setting -> send open can communication -> send recording can -> save file (rcv trigger event) -> reading config

static void RecordTh( void const * argument )
{
	osEvent event;

	static uint32_t s_unTimeout;

	// initialize states
	m_stRecordingControl.eRcdState = eRecord_Init;
	
#if true
	ShowRecordingStructureSize();
#endif

	// set default system mode
	RecordingSystemModeHandler();

	// clear config info
	ClearConfigInfo();

	while( 1 )
	{
		event = osMessageGet( hRecordAppMsg, 1 );

		if( event.status != osEventMessage )
		{
			//! if events are not received loop will be continued.
			//GLogE ("error!! RecordTh \r\n");
		}
		else
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hRecordAppMsg));
#endif
			GLogI("RecordTh \r\n");

			// first distirbution of event 
			EvtHandlerFlightRecording(event);			
		}

		//SetFlightRecordingState(100);

		switch(m_stRecordingControl.eRcdState)
		{
			case eRecord_Init:
				Record_Init();

				if( GetRecordingSystemModeState() == eApp_Inside )
				{
					SetFlightRecordingState(eRecord_ReadConfig);

					
					LED_ALL_OFF;
					LED_YELLOW_ON;
					//LED_SetState(eLED_NORMAL, 30*1000, 200);
				}
				else
				{
					SetFlightRecordingState(eRecord_Idle);
					printf("fw service is not a recording mode\r\n");
					m_stRecordingControl.bConfigError = false;
				}
				
				break;
			case eRecord_ReadConfig:
				Record_ReadConfig();

				// need to check
				InitSleepWaitControl();


				// test
				//m_stFwWakeupInfo.bActiveRecording = 0;
				

				// set signal to tirgger thread start triggger
				osSignalSet(GetTriggerHandle(), SIGNAL_RECORD_CONFIG_PARSING);

				SetFlightRecordingState(eRecord_WaitTriggerModule);

				// if file is broken then parking to idle
				if( m_stRecordingControl.bConfigError == true )
				{
					SetFlightRecordingState(eRecord_Idle);
				}
				else
				{
					VCI_SetPassThruProtocolID((uint8_t*)&m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.unProtocolType);

					VCI_Clear_DLC_HW();
					VCI_HWSetParameter((uint8_t *)&m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stHwSet);
					VCI_SetSConfigList((uint8_t *)&m_stRecordingControl.pstCfgCtrl[0].stCfgInfo.stEcuIdInfo.stJ2534SetConfig, TRUE);
				}

				// for trigger ecu open process. 
				Record_HwSetting();

				// clear dtc info
				ClearTriggerDtcData();

				// clear request index
				UpdateMonitoringInfo();			

				// set connection time timer
				s_unTimeout = Get_Tmr();
				
				break;
			case eRecord_WaitTriggerModule:

				if( GetActiveFlightRecording() == true )
				{					
					// check trgigger mode
					if( isTriggerModeActive() == true )
					{
						SetFlightRecordingState(eRecord_HwSetting);
						
						//initalize recording data variables
						UpdateRecordStartInfo();
					}


#if false
					// check time out for sleep when trigger module wasn't connected
					//if( (Get_Tmr() - s_unTimeout) > 10*60*1000)
					if( (Get_Tmr() - s_unTimeout) > 3*1000)
					{
						if( m_stRecordingControl.stCanCtrl.unCanCommErrorCount != m_stRecordingControl.stCanCtrl.unPreCanCommErrorCount )
						{
							Record_HwSetting();				
							m_stRecordingControl.stCanCtrl.unPreCanCommErrorCount = m_stRecordingControl.stCanCtrl.unCanCommErrorCount;
						}
						
						printf("eRecord_WaitTriggerModule : sleep time out\r\n");				
						
						if( CheckRecordEcuOpen() == false )
						{					
							printf("eRecord_WaitTriggerModule : ecu time out, error count : %d\r\n", m_stRecordingControl.stCanCtrl.unCanCommErrorCount);
							
							// to make time out commmunication
							if( m_stRecordingControl.stCanCtrl.unCanCommErrorCount >= MAX_CAN_COMMUNICATION_ERROR_COUNT )
							{						
								printf("eRecord_WaitTriggerModule : ecu error time out -> go to sleep\r\n");
								SetFlightRecordingState(eRecord_Error);
							}
						}

						s_unTimeout = Get_Tmr();
					}
#endif					
				}
				else
				{				
					// wait for trigger connection
					SetFlightRecordingState(eRecord_Idle);
				}
				
				osDelay(1000);
				break;				
			case eRecord_HwSetting:
				
				Record_HwSetting();

				UpdateOpenEcuInfo();

				// start recording 
				SetFlightRecordingState(eRecord_OpenEcu);

				if( (LED_GetState() != eLED_BT_SCAN) && (LED_GetState() != eLED_NOTI) )
				{
					LED_ALL_OFF;
					LED_SetState(eLED_REC_COMM, 0xffffffff, 500);
				}
				
				break;
//			case eRecord_Init5bps:
//				Record_Init5bps();
//				break;
//			case eRecord_InitKwp:
//				Record_InitKwp();
//				break;
			case eRecord_OpenEcu:
				
				switch(Record_OpenEcu())
				{
					case eRecordingStateResult_Success:
						// done sending all open comm data 
						SetFlightRecordingState(eRecord_Monitoring);
						
						if( (LED_GetState() != eLED_BT_SCAN) && (LED_GetState() != eLED_NOTI) )
						{
							LED_ALL_OFF;
							LED_SetState(eLED_REC_COMM, 0xffffffff, 500);
						}
						
						break;
					case eRecordingStateResult_Fail:

						if( Get_TmrDelta(Get_Tmr(),g_unSleepWaitTimeout) >= MAX_SLEEP_WAIT_TIME )
						{
							// error occured
							SetFlightRecordingState(eRecord_Error);
						}
						else
						{
							SetFlightRecordingState(eRecord_HwSetting);
						}
						
						if( (LED_GetState() != eLED_BT_SCAN) && (LED_GetState() != eLED_NOTI) )
						{
							LED_ALL_OFF;
							LED_YELLOW_ON;
						}
						
						break;
					case eRecordingStateResult_Retry:
						break;
					case eRecordingStateResult_None:

						break;
				}
				
				break;
			case eRecord_Monitoring:
				Record_Monitoring();

				// check seconds trigger 
				if( m_stRecordingControl.bActiveTrigger == true )
				{
					// check margin time from Config info
					if( (m_stRecordingControl.stDataCtrl.unTriggerTime + (m_stRecordingControl.stDataCtrl.usRecordTimeAfterTrigger*1000)) <= Get_Tmr() )
					{
						printf(">>>>>>>>>>>>>>>>>>>>>>>>> %s] recording time out\r\n", __func__);
						printf(">>>>>>>>>>>>>>>>>>>>>>>>> %s] unTriggerTime : %d\r\n", __func__, m_stRecordingControl.stDataCtrl.unTriggerTime);
						printf(">>>>>>>>>>>>>>>>>>>>>>>>> %s] usRecordTimeAfterTrigger : %d\r\n", __func__, m_stRecordingControl.stDataCtrl.usRecordTimeAfterTrigger);
						printf(">>>>>>>>>>>>>>>>>>>>>>>>> %s] diff time : %d\r\n", __func__, Get_Tmr()-m_stRecordingControl.stDataCtrl.unTriggerTime);

						SendEvent2RecordThread(eEVT_REC_TRIGGER_STOP, NULL, 0);

						m_stRecordingControl.bActiveTrigger = false;
					}
				}

				if( GetActiveFlightRecording() == false )
					SetFlightRecordingState(eRecord_Idle);	
				
				break;
			case eRecord_Idle:
				Record_Idle();				
				break;
			case eRecord_Error:
				Record_Error();
				break;
			case eRecord_Sleep:
				Record_Sleep();
				break;
			default:
				break;
		}			

		
		//RunRecordMainHandler();
		RecordingSystemModeHandler();
		
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#else
        osDelay( 1 );
#endif
	}
}

