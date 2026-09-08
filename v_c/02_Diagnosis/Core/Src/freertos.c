/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>

#include "tim.h"

#include "common.h"
#include "buzzer.h"
#include "sw_timer.h"
#include "firmware.h"
#include "led.h"
#include "git_ioctl.h"
#include "git_mmc.h"
#include "git_cli.h"
#include "git_pool.h"
#include "git_protocol.h"
#include "git_pm.h"
#include "git_rs9116.h"
#include "git_can.h"
#include "git_global.h"
#include "git_eth.h"
#include "git_hsm.h"
#include "git_kl.h"
#include "git_sensor.h"
#include "git_test.h"
#include "typedef.h"
#include "git_PassthruDefines.h"
#include "git_OBDcomm.h"
#include "git_fsutil.h"
#include "usb_device.h"
#include "git_vci.h"
#include "Git_mcp2518fd.h"

#include "gpio.h"
#ifdef VCI3_RECORD
#include "Git_record.h"
#include "Git_trigger.h"
#endif
#include "Git_rs9116.h"
#ifdef LISTDIAG
#include "Git_ListDiag.h"
#endif
#ifdef USE_RELAY_MOSA
#include "Git_BatteryRelayControl.h"
#endif
#include "Git_CsacDiag.h"
#include "rsi_mqtt_client.h"
#include "time.h"
// for test
#include "git_i2c.h"
#include "git_adc.h"
#include "spi.h"
#include "git_HSM_SPICommand.h"
#include "git_HSM_Operations.h"
#include "git_function_list.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern I2C_HandleTypeDef	hi2c2;

extern char			USERPath[4];		/* USER logical drive path */
extern BOOL			g_bAckflag;
extern BOOL			g_bAckTimer=OFF;

osThreadId hALDiagTh;
extern osMessageQId	hDiagMsg;
extern osPoolId		hDiagPool;

extern osPoolId		hPTPKPool;
extern osMessageQId	hOBDTxMessage;
extern osMessageQId	hOBDKlineTxMessage;

eMain_State		g_eMainState;
extern stSWTimerInfo g_SWTimer[MAX_SW_TIMER];
extern stACK_MSG_INFO stAckMsgInfo;
extern BOOL	g_bIsFastInit;
extern BOOL g_bFastInit_Success;
extern U8	g_ucFastInitRetryCnt;
#ifdef LISTDIAG
extern osMessageQId hListDiagMsg;
#endif
#ifdef USE_RELAY_MOSA
extern osMessageQId hBatRelayConMsg;

#endif
extern osMessageQId hCsacDiagMsg;

extern uint32_t g_ulPGN;
extern uint32_t g_ulProtocolID;
extern U8 g_ucVehicle_Current_Read;

uint32_t m_unBtConnCheck_Timer = 0;
bool 	m_ubBtConnCheck_Start = false;
static bool m_bServerTimeout1min = false;	// 서버 1분 타임아웃 플래그
static bool m_bServerTimeout3min = false;	// 서버 3분 타임아웃 플래그

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
#include "Git_function_list.h"

extern eLockState 	g_eLockStatus;
#define LOCK_GET_STATE() 	(g_eLockStatus)
#define LOCK_SET_STATE(X)	(g_eLockStatus = X)

extern void Get_AUTOVINData(U8 ucFlag);
extern BOOL LoadRunRepro_Info();
extern BOOL LoadRDBI_Info();
extern void CreateRunRepro_Info();
extern void DeleteRunRepro_Info();
extern void SaveRepro_Result(unsigned short usEcuIndex);
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
void Backup_Repro_Result();
#endif
extern void VCI_DataSniffering(stMsgClst *msg, stCommPkt *pkt, ePKT_TD type);
extern BOOL VCI_SendECUInfoPacket(eMain_State eMainState, BOOL bWaitForResPacket);
extern void FL_GitPassThruConnect( void *pInterPtcl, uint32_t eInCommType, uint32_t usLength  );
extern void FL_GitPassThruDisconnect( void *pInterPtcl, uint32_t eInCommType, uint32_t usLength  );
extern void TransmitFunction( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID );

extern stRunRepro *g_pstRunReproInfo;
extern stRDBIInfo *g_pstRDBIInfo ;
extern BYTE        g_ucCommRDBIIdx;
extern BYTE g_arrRecvPartNo[32];
extern BYTE g_arrRecvSwVer[32];
extern BOOL g_bWaitForResPacket_InterAnalysis;
#endif

    


/* USER CODE END Variables */
osThreadId	defaultTaskHandle;
osThreadId	hDecompressionTh;
void DecompressionThread( void const *argument );

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static int32_t InitVariable( void );
void diagnosticThread( void const *argument );
extern void VCI_Initialize(void);
extern u32	VCI_ProtocolClassification();
extern int	git_usb_state_get(void);


void ClearConnctionLedCheck();
void Connection_Check( void );


/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
//	uint32_t	ret		= 0;

	GLogN( "\r\n\n" );
	GLogI( "==================================================\r\n" );
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
	GLogI( "   Start %s vci3_reprocommon(%s)...\r\n", MODEL_NAME, __DATE__ );
#elif defined(VCI3_RECOVERY)
	GLogI( "   Start %s C_vci3_recovery(%s)...\r\n", MODEL_NAME, __DATE__ );
#else
	GLogI( "   Start %s C_vci3_main(%s)...\r\n", MODEL_NAME, __DATE__ );
#endif
	GLogI( "==================================================\r\n" );
#ifdef VCI_III_USB_HS
    volatile int32_t ret;
#else
	//getBoardID();
#endif
	//printMainCLKs();//TARA, delet important info

#if( VCI_III_ASING_MODE )
	//IO_CONTROL_HIGH( TRIG_KEY_INT );
	g_TriggerKey = IO_CONTROL_GET( TRIG_KEY_INT );
#endif	// VCI_III_ASING_MODE

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, (1024)*2);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
	osThreadDef( ALDiag, diagnosticThread, osPriorityNormal, 0, 128 );
	hALDiagTh = osThreadCreate( osThread( ALDiag ), NULL );
	if( hALDiagTh == NULL )
	{
		GLogE( "Error... fail create hALDiagTh Thread!!!\r\n" );
		//return 0;
	}

    osThreadDef( Decompression, DecompressionThread, osPriorityNormal, 0, 512);
    hDecompressionTh = osThreadCreate( osThread( Decompression ), NULL );

#ifdef LISTDIAG
		StartlistdiagThread();
#endif

#ifdef USE_RELAY_MOSA
		StartBatRelayConThread();
#endif
        StartCsacdiagThread();
	
#ifndef VCI_III_USB_HS
    	hCommandTh = osThreadCreate( osThread(commandth), NULL );
    	if( hCommandTh == NULL )
    	{
    		GLogE( "Error... fail create hCommandTh Thread!!!\r\n" );
    	}

    	hCanToSpiTh = osThreadCreate( osThread(cantospith), NULL );
    	if( hCanToSpiTh == NULL )
    	{
    		GLogE( "Error... fail create hCanToSpiTh Thread!!!\r\n" );
    	}

    	hSpitoCanTh = osThreadCreate( osThread(spitocanth), NULL );
    	if( hSpitoCanTh == NULL )
    	{
    		GLogE( "Error... fail create hSpitoCanTh Thread!!!\r\n" );
    	}

        ret = StartSpiThread();
        if( ret != 0 )
        {
           GLogE( "Error... fail Start Spi Thread!!!\r\n" );
        }
#endif

  /* USER CODE END RTOS_THREADS */

}

uint8_t tx_data[DUMMY_SIZE] = {0x5A, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t rx_data[DUMMY_SIZE] = {0};

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
	bool bCheck_hsm_staus = OFF;
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
    static unsigned char s_ucAutoVinCnt = 0;
    static unsigned int s_unExitStateTmr;
#endif

#if 0
	  osDelay(5000);
	  gPMFlag = WAKEUP_SOURCE_IG;
	  gotoStandbyMode( gPMFlag );
		while(1);
#endif

  /* init code for USB_DEVICE */
	
    EnableGL850G();
    
    EnableLAN9514();
    
    EnableUSB3300();
    
  /* USER CODE BEGIN StartDefaultTask */
	//EnableEthDiag();
	
	if( InitVariable() < 0 )
	{
		Error_Handler();
	}
	/* init code for USB_DEVICE */	
	MX_USB_DEVICE_Init();	//change function position //If USB data is received immediately after MX_USB_DEVICE_Init(), there will be a problem with the operation of 'InitVariable()'
	BackupSRAM_Init();
#ifdef VCI3_RECORD
	StartRecordThread();
	StartTriggerThread();
#endif
#ifdef FEATURE_MCP2518FD
	GLogI( "MCP2518FD Enable!!\r\n" );

	InitMCP2518FD();
	SetSpiCanSTB();
	initRS9116();
	if( gsFwInfo.mucModeChange == TRUE )
	{
	  	rsi_driver_cb->common_cb->state = RSI_COMMON_OPERMODE_DONE;
		rsi_driver_cb->wlan_cb->state = RSI_WLAN_STATE_OPERMODE_DONE;
		rsi_driver_cb_non_rom->device_state = RSI_DEVICE_INIT_DONE;
		g_ucBtConnected = TRUE;
		gsFwInfo.mucChanged = TRUE;
		gsFwInfo.mucModeChange = FALSE;
		saveFirmwareInfo_EMMC(false);
	}
	StartRS9116Thread();
#endif
#ifdef USE_INTERNAL_CAN_ONLY
	//GLogN( "MCP2518FD Disble!!\r\n" );
	initRS9116();
	if( gsFwInfo.mucModeChange == TRUE )
	{
		rsi_driver_cb->common_cb->state = RSI_COMMON_OPERMODE_DONE;
		rsi_driver_cb->wlan_cb->state = RSI_WLAN_STATE_OPERMODE_DONE;
		rsi_driver_cb_non_rom->device_state = RSI_DEVICE_INIT_DONE;
		g_ucBtConnected = TRUE;
		gsFwInfo.mucChanged = TRUE;
		gsFwInfo.mucModeChange = FALSE;
		saveFirmwareInfo_EMMC(false);
		bCheck_hsm_staus = OFF;
	}
    else //Check HSM Status when VCI3 boot up
    {
        bCheck_hsm_staus = ON;//Save_HSM_Status();
    }
	StartRS9116Thread();
#else	
	StartSpiThread(); // 220319 test
#endif
	
	Buzzer_Control( eBUZZER_DOMISOLDO, MSEC(200),MSEC(200), 4 );
	osDelay( 1200 );


	// start other thread
	StartCli();								// Start Thread for Command Line Interface
	StartProtocolThread();					// Start Thread for GIT Protocol Parsing
	startFDCanThread();						// Start Thread for FDCan Tx
#if defined (KKT_TEST)
	EnableEthDiag(); 
	//eth_SetIPAddressStatic( RS9116_IPV4_ADDR( 192, 168, 0, 77 ), RS9116_IPV4_ADDR( 255, 255, 255,   0 ), RS9116_IPV4_ADDR( 192, 168,   0,   1 ) );  //jkc NEW
#endif

#if( VCI_III_ASING_MODE )
	initTestModule();
	startTestThread();
#endif	// VCI_III_ASING_MODE
	g_eMainState=eMain_Init;
	osSignalSet( hOBDKlineTxTh, SIGNAL_AVAILABLE_SEND_KL_MSG );
#if 0
	osDelay(1000);
	gPMFlag = WAKEUP_SOURCE_HCAN1 | WAKEUP_SOURCE_HCAN2 | WAKEUP_SOURCE_LCAN | WAKEUP_SOURCE_12V_DET;
	g_eMainState = eMain_Sleep;
#endif
	//EnableEthDiag();//for factory test
	DisableEthDiag();	//Ethernet is need to close, when ME is booting
	
	EnableHSM_AT();

	if(bCheck_hsm_staus == ON) Save_HSM_Status();
	
	for(;;)
	{
        //jkc printf("%s] enter\r\n", __func__);
		switch( g_eMainState )
		{
			case eMain_Init :
			{
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
                LED_WHITE_ON;
                Backup_Repro_Result();
                g_eMainState = eMain_AutoVin;
#else
                g_eMainState = eMain_Run;
#endif
				break;
			}

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
            case eMain_AutoVin:
                GLogN( "State Change -> eMain_AutoVin\r\n");
        		Get_AUTOVINData(0x01);
        		VCI_Clear_DLC_HW();

                if ( g_ucAUTOVIN[0] != 0x00 )
                    g_eMainState = eMain_LoadFile_RunRepro;
                else
                {
                    if ( s_ucAutoVinCnt++ < 3 )
                        osDelay(500);
                    else 
                    {
                        s_ucAutoVinCnt = 0;
                        LED_YELLOW_ON;
                        g_eMainState = eMain_Run;
                        
                        GLogN( "State Change[Autovin fail] -> eMain_Run\r\n");
                    }
                }
				break;

            case eMain_LoadFile_RunRepro:
                GLogN( "State Change -> eMain_LoadFile_RunRepro\r\n");
                if ( LoadRunRepro_Info() == TRUE )
                {
                    if ( g_pstRunReproInfo[g_ucCommRDBIIdx].ucRetryCount >= 0 &&
                              g_pstRunReproInfo[g_ucCommRDBIIdx].ucRetryCount <= 2 )
                    {
                        // ���׷��̵����� ����� �����ϴ� ����
                        g_eMainState = eMain_JUMP_to_ECU_Upgrade;
                    }
                    else if ( g_pstRunReproInfo[g_ucCommRDBIIdx].ucRetryCount >= 3 &&
                              g_pstRunReproInfo[g_ucCommRDBIIdx].ucRetryCount < 0xFF ) // 0xFF�� ���� 
                    {
                        DeleteRunRepro_Info();
                        g_eMainState = eMain_Run;

                        LED_RED_ON;
                        Buzzer_On();
                        osDelay(2000);
                        Buzzer_Off();
                    }
                }
                else
                {
                    g_eMainState = eMain_LoadFile_RDBI;
                }

                break;

            case eMain_LoadFile_RDBI:
                GLogN( "State Change -> eMain_LoadFile_RDBI\r\n");
                if ( LoadRDBI_Info() == TRUE )
                {
                    BYTE arrSendData[14];
                    uint32_t nProtocolID = ISO15765; // ISO14230�� �ʿ��ϸ� RDBI�� �������� �������� �ؾ���.

                    LED_SetState(eLED_RERPROCOMM_RDBI, TIMER_LOOP_INFINITE, 100);

                    memcpy(arrSendData, (char*)&nProtocolID, sizeof(uint32_t));
                    memset(&arrSendData[4], 0x00, sizeof(uint32_t));
                    arrSendData[8] = 0x24;
                    arrSendData[9] = 0x01;
                    arrSendData[10] = 0x01;
                    arrSendData[11] = 0x80;
                    arrSendData[12] = 0x00;
                    arrSendData[13] = 0x00;

                    LOCK_SET_STATE(eLOCK_STATE_UNLOCK);
                    FL_GitPassThruConnect(arrSendData, PACKET_INTER_ANALYSIS, sizeof(arrSendData));
                    
                    g_eMainState = eMain_GET_ECU_SESSION_OPEN_1;
			}
                else
                     g_eMainState = eMain_Run;
                break;

            case eMain_GET_ECU_SESSION_OPEN_1:
                GLogN( "State Change -> eMain_GET_ECU_SESSION_OPEN_1\r\n");
                if ( VCI_SendECUInfoPacket(eMain_GET_ECU_SESSION_OPEN_1, FALSE) )
                    g_eMainState = eMain_GET_ECU_SESSION_OPEN_2;
                break;

            case eMain_GET_ECU_SESSION_OPEN_2:
                GLogN( "State Change -> eMain_GET_ECU_SESSION_OPEN_2\r\n");
                if ( VCI_SendECUInfoPacket(eMain_GET_ECU_SESSION_OPEN_2, FALSE) )
                    g_eMainState = eMain_GET_ECU_PART_NO;
                break;
                
            case eMain_GET_ECU_PART_NO:
                if ( g_bWaitForResPacket_InterAnalysis == FALSE )
                {
                    s_unExitStateTmr = Get_Tmr();
                    GLogN( "State Change -> eMain_GET_ECU_PART_NO\r\n");
                }
                
                if ( Get_TmrDelta( Get_Tmr(), s_unExitStateTmr ) > 5000 )
                {
                    // exception 
                    g_eMainState = eMain_Run;
                    GLogN( "State Change because of timeout eMain_GET_ECU_PART_NO -> eMain_Run\r\n");
                }
                else
                {
                // VCI_DataSniffering()���������� ȹ�� 
                VCI_SendECUInfoPacket(eMain_GET_ECU_PART_NO, TRUE);                
                }
                break;

            case eMain_GET_ECU_SW_VERION:
                if ( g_bWaitForResPacket_InterAnalysis == FALSE )
                {
                    s_unExitStateTmr = Get_Tmr();
                GLogN( "State Change -> eMain_GET_ECU_SW_VERION\r\n");
                }
                if ( Get_TmrDelta( Get_Tmr(), s_unExitStateTmr ) > 5000 )
                {
                    // exception 
                    g_eMainState = eMain_Run;
                    GLogN( "State Change because of timeout eMain_GET_ECU_SW_VERION -> eMain_Run\r\n");
                }
                else
                {
                // VCI_DataSniffering()���������� ȹ�� 
                    VCI_SendECUInfoPacket(eMain_GET_ECU_SW_VERION, TRUE);                
                }
                break;
                
            case eMain_Determin_ECU_To_Upgrade:
                {
                    BOOL bFoundPartNo = FALSE, bCreateRunReproFile = FALSE;
                    GLogN( "State Change -> eMain_Determin_ECU_To_Upgrade\r\n");
                LOCK_SET_STATE(eLOCK_STATE_LOCK);
                FL_GitPassThruDisconnect(NULL, PACKET_INTER_ANALYSIS, 0);

                    LED_SetState(eLED_NORMAL, 0, 0);

                    for ( int i=0; i<g_pstRDBIInfo[0].unECUCount; i++ )
                    {
                        bFoundPartNo = FALSE;
                        g_pstRDBIInfo[i].bIsUpgradeTarget = FALSE;
                        SaveRepro_Result(i); // ��� ����;

                        for ( int j=0; j<g_pstRDBIInfo[i].ucPartNoCnt; j++ )
                        {
                            if ( (memcmp(g_pstRDBIInfo[i].parrPartNo[j], 
                                   g_arrRecvPartNo, g_pstRDBIInfo[i].ucPartByteSize) == 0) )
                            {
                                bFoundPartNo = TRUE;
                                break;
                            }
                        }
                        
                        if ( bFoundPartNo == TRUE )
                        {
                            if ( memcmp(g_arrRecvSwVer, g_pstRDBIInfo[i].pSwVer, g_pstRDBIInfo[i].ucSwVerByteSize) > 0 ) 
                            {
                                bCreateRunReproFile = g_pstRDBIInfo[i].bIsUpgradeTarget = TRUE;
                            }
                            else if ( memcmp(g_arrRecvSwVer, g_pstRDBIInfo[i].pSwVer, g_pstRDBIInfo[i].ucSwVerByteSize) == 0 ) 
                            {
                                if ( g_pstRunReproInfo[i].ucRetryCount == 0xFF ) // 0xFF�� ����
                                {
                                    LED_BLUE_ON;
                                    Buzzer_Control( eBUZZER_SOLPAMI, MSEC(200),MSEC(200), 3);
                                    GLogN( "Target ECU is to be upgraded\r\n");
                                }
                                else
                                {
                                    // �Ϸ�
                                    LED_MAGENTA_ON;
                                    GLogN( "Target ECU is Already Upgraded\r\n");
                                }
                            }
                            else
                            {
                                // 2024/01/17 ����ȣ å�� ��û(���߹�������) : ������ �޶� ���׷��̵� 
                                bCreateRunReproFile = g_pstRDBIInfo[i].bIsUpgradeTarget = TRUE;
/*
                                // �̴�� 
                                LED_SetState(eLED_RERPROCOMM_NO_TARGET, TIMER_LOOP_INFINITE, 100);
*/
                            }
                        }
                        else
                        {
                             // �̴�� 
                            LED_SetState(eLED_RERPROCOMM_NO_TARGET, TIMER_LOOP_INFINITE, 100);
                            GLogN( "It is not target of ECU upgrade.\r\n");
                        }
                    }

                    if ( bCreateRunReproFile == TRUE )
                    {
                        CreateRunRepro_Info();  // RunRepor.ini����
                g_eMainState = eMain_JUMP_to_ECU_Upgrade;
                    }
                    else
                    {
                        DeleteRunRepro_Info();
                        g_eMainState = eMain_Run;

                       GLogN( "State Change -> eMain_Run\r\n");
                    }
                    Backup_Repro_Result();
                }
                break;

            case eMain_JUMP_to_ECU_Upgrade:
                GLogN( "State Change -> eMain_JUMP_to_ECU_Upgrade\r\n");
                LED_CYAN_ON;

                RunModeSwitch(eApp_ECUUP_STDA);
                break;
#endif    
			case eMain_Run :
			{
				//evt = osSignalWait( PMMAIN_SLEEP_SIGNAL, osWaitForever );
				//if( evt.status == osEventSignal )
				//{
				//	gsPropertie.eMainState = g_eMainState;
				//}

				if( intDlccomCount == 0 && g_bAckflag == 1 && g_ulProtocolID != WABCO_ABS)
				{
				  	xTaskNotifyStateClear( hOBDKlineTxTh );
					VCI_ACK();
					intDlccomCount = g_uiAckTiming;
				}
				if(healthcheckcnt == 0 && g_mqtt_isconnected == 1)
				{
					int8_t rssi;

					rssi = RSSI_SetLedIndicator();
					MQTTPacketSend(MQTT_MESSAGE_TYPE_HEALTH_CHECK, (uint8_t *)&rssi, 1);

					healthcheckcnt = 30000;
				}
				if(healthcheckcnt == 0 && g_websocket_isconnected == 1)
				{
					WebsockPacketSend( "1\0", 1);
				  	
					healthcheckcnt = 30000;
				}
				osSignalSet( hOBDKlineTxTh, SIGNAL_AVAILABLE_SEND_KL_MSG );

				g_iUSBConnected = git_usb_state_get();
				//if(ret==3)	GLogI( "USB CONNECTED!!\r\n" );

				// vci scan button handler
				rsi_bt_scan_handler();

				if( GetCurFwServiceMode() != eApp_Inside )
					Connection_Check();
				break;
			}

			case eMain_Sleep :
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

				gotoStandbyMode( gPMFlag );
				while(1);
				break;
			}

			default :
			{
				GLogE( "unknown Main State!!! \r\n" );
				break;
			}
		}

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#else
        osDelay( 1 );
#endif
	}
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static int32_t InitVariable( void )
{
	int32_t	ret = 0;
	char cRet = 0;
	bool bRet = false;

	HAL_TIM_Base_Start_IT( &htim17 );
	initPowerManagement();						// Power management initialize
	InitIOCTL();								// IO Control Pin init
	VCI_Initialize();
    
	//-----------------------------------------------------------------
	//   For System status
	//-----------------------------------------------------------------
#if 0
	ret = initMMC();							// EMMC initialize
	if( ret < 0 )
	{
		GLogEE( "Fail... EMMC Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
	else 
	{
	 	//directory_list(); //TARA, delet important info
	}
#endif
	
	ret = loadFirmwareInfo_EMMC();					// Load F/W Info

	if( ret <  0 )
	{
		GLogEE( "Fail... Load Firmware Info(%d)!!!\r\n", ret );
		return INIT_FAIL;
	}

	ret = Load_ServerInfo_EMMC();
	if( ret != FR_OK )
	{
		GLogEE( "Fail... Load Server Info(%d)!!!\r\n", ret );
	}
	
	if( gsFwInfo.msWifiConnectInfo.initialized != 1 )
	{
		gsFwInfo.msWifiConnectInfo.initialized			= 1;					// AP infomation initialized
		gsFwInfo.msWifiConnectInfo.mode					= 2;					// 0:OPEN, 1,WPA, 2:WPA2, 3:WEP
		gsFwInfo.msWifiConnectInfo.ipSetting			= 0;					// 0 : DHCP, 1 : STATIC
#ifdef TEST_MODE
		gsFwInfo.msWifiConnectInfo.PSK_length			= sizeof(GIT_PSK_KEY);	// PSK_length
		sprintf( gsFwInfo.msWifiConnectInfo.PSK_Key, 	"GIT_PSK_KEY" );		// PSK_Key
		gsFwInfo.msWifiConnectInfo.SSIDlength			= sizeof(GIT_SSID_NAME);// SSIDlength
		sprintf( gsFwInfo.msWifiConnectInfo.SSIDname, 	"GIT_SSID_NAME" );		// SSIDname
#endif
#ifdef VCI_III_USB_HS
		gsFwInfo.mucCommandPort	= 2;
		gsFwInfo.mucSwitchST	= 0;
		gsFwInfo.mucCan1Connect	= 1;
		gsFwInfo.mucCan2Connect	= 1;
		
	   memset( (void*)&gsFwInfo.msFDCan1Info, 0, sizeof( SFDCanInfo ) );
	   
	   gsFwInfo.msFDCan1Info.mBaudrateIndex	= 1;
#endif
	}
	 
	//-----------------------------------------------------------------
	//   Devices Initialize
	//-----------------------------------------------------------------
	ret = Buzzer_Init();						// Buzzer initialize
	if( ret < 0 )
	{
		GLogEE( "Fail... Buzzer Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
	
	bRet = LED_TimerInit();						// Initialize OBD
	if( bRet != true )
	{
		GLogEE( "Fail... OBD LED_TimerInit( %d )\r\n", ret );
		return INIT_FAIL;
	}
	else
	{
	  	LED_ALL_OFF;
	  	if( gsFwInfo.mucCurrentMode == eApp_VCI_2 )
		  	LED_SetState(eLED_GENERAL, 0, 0);//white on
		else 
			//LED_SetState(eLED_GENERAL, 0, 0);
		  	LED_SetState(eLED_NORMAL, 0, 0);//green on
	}

	ret = initFDCan();							// FDCAN initialize
	if( ret < 0 )
	{
		GLogEE( "Fail... FDCan Init( %d )\r\n", ret );
		return INIT_FAIL;
	}

	ret = DWT_Delay_Init();						// us DWT Delay initialize
	if( ret < 0 )
	{
		GLogEE( "Fail... DWT Timer Init( %d )\r\n", ret );
		return INIT_FAIL;
	}

	ret = InitCli();							// Initialize Command Line Interface(CLI)
	if( ret < 0 )
	{
		GLogEE( "Fail... CLI Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
	
#ifndef NEW_VCI_III 
	ret = InitEthernet();						// Initialize Ethernet
	if( ret < 0 )
	{
		GLogEE( "Fail... Ethernet Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
#endif

	ret = InitHSM();							// Initialize Hsm
	if( ret < 0 )
	{
		GLogEE( "Fail... Hsm Init( %d )\r\n", ret );
		return INIT_FAIL;
	}

	ret = InitKL();								// Initialize KL Line
	if( ret < 0 )
	{
		GLogEE( "Fail... KL Line Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
	ret = InitGyroSensor();					// Initialize Gyro Sensor
	if( ret != INIT_OK )
	{
		GLogEE( "Fail... GyroSensor Init( %d )\r\n", ret );
//		return INIT_FAIL;
	}
	cRet = initOBDComm();						// Initialize OBD
	if( cRet != INIT_OK )
	{
		GLogEE( "Fail... OBD Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
	
	//-----------------------------------------------------------------
	//   Memory Pool variable
	//-----------------------------------------------------------------
#ifdef OS_POOL_ID_PRINT
	extern stOS_POOL_ID_INFO stOsPoolIdInfo[10];
	memset(stOsPoolIdInfo,0x00,sizeof(stOsPoolIdInfo));
#endif
	ret = InitCanPool();						// CanFD packet memory pool
	if( ret < 0 )
	{
		GLogEE( "Fail... CANFD packet memory pool Init( %d )\r\n", ret );
		return INIT_FAIL;
	}

	ret = InitCommPool();						// Communication packet memory pool
	if( ret < 0 )
	{
		GLogEE( "Fail... Communication packet memory pool Init( %d )\r\n", ret );
		return INIT_FAIL;
	}

	ret = InitMessagePool();					// Message memory pool
	if( ret < 0 )
	{
		GLogEE( "Fail... Message memory pool Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
#ifndef NEW_VCI_III
	ret = InitEthPool();						// Ehternet Packet, Message memory pool
	if( ret < 0 )
	{
		GLogEE( "Fail... Ethernet memory pool Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
#endif

	ret = InitGitProtocol();					// GIT Protocol
	if( ret < 0 )
	{
		GLogEE( "Fail... Git Protocol Init( %d )\r\n", ret );
		return INIT_FAIL;
	}

	ret = InitDiagPool();						// Diag Packet, Message memory pool
	if( ret < 0 )
	{
		GLogEE( "Fail... Diag memory pool Init( %d )\r\n", ret );
		return INIT_FAIL;
	}

	ret = InitSemaphore();						// Semaphore
	if( ret < 0 )
	{
		GLogEE( "Fail... Semaphore memory pool Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
	
#ifdef FEATURE_MCP2518FD
    ret = InitSpiCanPool();
	if( ret < 0 )
	{
		GLogE( "Fail... fail InitSpiCanPool!!!\r\n" );
		return INIT_FAIL;
	}
	ret = InitSPICanControler();
	if ( ret < 0)
	{
		GLogE( "Fail... fail InitSPICanControler!!!\r\n" );
		return INIT_FAIL;
	}
#endif

#ifdef USE_RELAY_MOSA
	ret = InitBatRelayConPool();
	if( ret < 0 )
	{
		GLogEE( "Fail... BatRelayCon memory pool Init( %d )\r\n", ret );
		return INIT_FAIL;
	}
#endif

	f_chdir(DIR_ROOT);			// reset cwd to root after boot (in case init moved it into a subfolder)

    return INIT_OK;
}

void ClearConnctionLedCheck()
{
	m_ubBtConnCheck_Start = false;
	m_unBtConnCheck_Timer = Get_Tmr();
}

void Connection_Check( void )
{
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
    static int nLEDTimeOut = 1800000;
#endif
	if( g_ucBtConnected == 0 && g_mqtt_isconnected == 0 && g_iUSBConnected != USBD_STATE_CONFIGURED )
	{
	  	if( m_ubBtConnCheck_Start == false )
		{
		  	m_ubBtConnCheck_Start = true;
			m_unBtConnCheck_Timer = Get_Tmr();
			m_bServerTimeout1min = false;
			m_bServerTimeout3min = false;
		}
		else
		{
			g_bUsingKM = ON;//use KM when reconnect
#if(!VCI_III_ASING_MODE)
			uint32_t elapsed = Get_TmrDelta( Get_Tmr(), m_unBtConnCheck_Timer );

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
		  	if( elapsed > nLEDTimeOut )
			{
                nLEDTimeOut = 30000;
				m_bServerTimeout3min = true;
			  	LED_ALL_OFF;
			  	LED_SetState(eLED_NO_SERVER_3MIN, 30000, 2);
			}
#else
			if( !m_bServerTimeout1min && elapsed > 60000 )
			{
				m_bServerTimeout1min = true;
				LED_ALL_OFF;
				LED_SetState(eLED_SERVER_TIMEOUT, 30000, 3);
			}
			if( !m_bServerTimeout3min && elapsed > 180000 )
			{
				m_bServerTimeout3min = true;
				LED_ALL_OFF;
				LED_SetState(eLED_NO_SERVER_3MIN, 30000, 2);
			}
#endif
#endif
		}
	}
	else
	{
	  	m_unBtConnCheck_Timer = 0;
	 	m_ubBtConnCheck_Start = false;
		m_bServerTimeout1min = false;
		m_bServerTimeout3min = false;
	}
}

void diagnosticThread( void const *argument )
{
	osEvent			event;
	MsgDiag_t		*message;
	PTmsgPkt_t		*packet;
	stMsgClst		*pClstMsg;
	stCommPkt		*pCommPkt;
	u16				usDiagtype;
	unsigned long 	ulIoctlID;

	for(;;)
	{
		event = osMessageGet( hDiagMsg, osWaitForever );

		if( event.status == osEventMessage )
		{
#ifdef PRINT_MESSAGE_ID
			printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hDiagMsg));
#endif
			message = ( MsgDiag_t * )event.value.p;
			packet	= ( PTmsgPkt_t* )message->pPacket;
			usDiagtype	= message->event;

			switch( usDiagtype )
			{
				case AUTO_VIN :
				{

				}
				break;

				case DIAG_PASSTHRU :
				{
					if( message->subEvent == eDIAG_COMM_TX_START )
					{

						message->subEvent = VCI_ProtocolClassification();

						if(message->subEvent == eCAN_TX_NONE_PARSING)
						{
							if(osMessageAvailableSpace(hOBDTxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
								osMessagePut( hOBDTxMessage, (uint32_t)message, osWaitForever );
							}
						}
						else// if(pDiagmsg->subEvent == eKLINE_TX_BLOCK)
						{
							if(osMessageAvailableSpace(hOBDKlineTxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
								osMessagePut( hOBDKlineTxMessage, (uint32_t)message, osWaitForever );
							}
						}
					}
					else if( message->subEvent == eDIAG_COMM_TX_FAIL )
					{


					}
					else if ( message->subEvent == eDIAG_COMM_RX_ING )
					{
					  	GLogI("I\r\n");	
						pClstMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
						if( pClstMsg == NULL )
						{
							break;
						}
						pCommPkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
						if( pCommPkt == NULL )
						{
							//osPoolFree( hMsgPool, (void *)message );
							osPoolFree( hMsgPool, (void *)pClstMsg );
							break;
						}
						
						pClstMsg->mPktType = message->mPktType;
						pClstMsg->mSeq = message->subEvent;
                        pCommPkt->mLen	= packet->DataSize;
                        pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
                        pCommPkt->mFuncID  = 0x1008;
						memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
						pCommPkt->mCS 	= CalcChecksumGITPtclPayloadFrame(pCommPkt);
						
						pClstMsg->pPacket	= (void *)pCommPkt;
						
						if(osMessageAvailableSpace(hTransmitMsg) == 0)
						{
							osPoolFree( hCommPKPool, (void *)pCommPkt );
							osPoolFree( hMsgPool, (void *)pClstMsg );
						}
						else
						{
							osMessagePut( hTransmitMsg, (uint32_t)pClstMsg, osWaitForever );
						}
						osPoolFree( hPTPKPool, (void *)packet );
						osPoolFree( hDiagPool, (void *)message );
					}
					else if( message->subEvent == eDIAG_COMM_RX_OK )
					{
						GLogI("^");						
#ifdef VCI3_RECORD						
						if( message->mPktType == PACKET_RECORD || message->mPktType == PACKET_TRIGGER )
						{
							pCommPkt = ( stCommPkt* )osPoolCAlloc( hCommPKPool );
							if( pCommPkt == NULL )
							{								
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );								
								break;
							}
							pCommPkt->mLen	= packet->DataSize + 24;

							pCommPkt->pTarget  = (void*)message->mPktType;
							memcpy( pCommPkt->mData, packet, pCommPkt->mLen );
							memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
							message->pPacket	= (void *)pCommPkt;	
							
							extern osMessageQId	hRecordDataMsg;
							uint32_t nLength = osMessageAvailableSpace(hRecordDataMsg);
							//if(osMessageAvailableSpace(hRecordDataMsg) == 0)
							if( nLength == 0 )
							{
								GLogE("%s] queue length : %d\r\n", __func__, nLength);
								
								osPoolFree( hCommPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
								osMessagePut( hRecordDataMsg, (uint32_t)message, osWaitForever );
							}

							osPoolFree( hPTPKPool, (void *)packet );
						}
						else
#endif
						{
							pClstMsg = ( stMsgClst* )osPoolCAlloc( hMsgPool );
							if( pClstMsg == NULL )
							{
								break;
							}
							pCommPkt = ( stCommPkt* )osPoolCAlloc( hCommPKPool );
							if( pCommPkt == NULL )
							{
								osPoolFree( hMsgPool, (void *)pClstMsg );
								break;
							}
							if((g_ulProtocolID == ISO15765_29BIT)						||
	  							(g_ulProtocolID == ISO15765_29BIT_EXCEPT)				||
	  							(g_ulProtocolID == ISO15765_29BIT_REPRO_PENNIMG_TIME)	||
	  							(g_ulProtocolID == ISO15765_CARB_29BIT)
#if 0//NEW_29BIT_CAN
                                || (g_ulProtocolID == ISO15765_ES95486_29bit)
#endif
								)
							{
							 	pClstMsg->mPktType = message->mPktType;
#if 0//NEW_29BIT_CAN
                                if(g_ulProtocolID == ISO15765_ES95486_29bit) 
                                {
                                    packet->DataSize = packet->DataSize;
                                }
                                else 
#endif
                                {
                                    packet->DataSize = packet->DataSize - 2;//VCI2와 패킷맞춤 CANID 전부를 싣지않고 끝에 두개만 사용 다보내고 싶으면 여기 수정
                                }
								pCommPkt->mLen	= packet->DataSize+ 24;

								pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
								pCommPkt->mFuncID  = 0x1002;
								g_bIsFastInit=FALSE;
								memcpy( pCommPkt->mData, packet, 24 );
								memcpy( &pCommPkt->mData[24], &packet->pData[2], packet->DataSize);
								memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
								
								pClstMsg->pPacket	= (void *)pCommPkt;
								if(osMessageAvailableSpace(hTransmitMsg) == 0)
								{
									osPoolFree( hCommPKPool, (void *)pCommPkt );
									osPoolFree( hMsgPool, (void *)pClstMsg );
								}
								else
								{
									osMessagePut( hTransmitMsg, (uint32_t)pClstMsg, osWaitForever );
								}
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
	                            pClstMsg->mPktType = message->mPktType;
	                            pCommPkt->mLen	= packet->DataSize + 24;
								memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
	                            pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
	                            //eDIAG_COMM_RX_OK
	                            if(g_bIsFastInit==TRUE)
	                            {
	                                pCommPkt->mLen	= packet->DataSize + 24 + 4;// +4 : IoctlID size add
	                                pCommPkt->mFuncID  = 0x1006;

	                                ulIoctlID = FAST_INIT;
	                            
	                                memcpy( pCommPkt->mData, &ulIoctlID, 4 ); //Add 'IoctlID' to 'pCommPkt'
	                                memcpy( pCommPkt->mData+4, packet, pCommPkt->mLen-4 );
	                            }
	                            else if( g_ucVehicle_Current_Read == 2)
	                            {
	                                pCommPkt->mFuncID  = 0x2315;
	                            
	                                memcpy( pCommPkt->mData, packet, pCommPkt->mLen );
	                            }
	                            else					
	                            {
	                                pCommPkt->mFuncID  = 0x1002;
	                            
	                                memcpy( pCommPkt->mData, packet, pCommPkt->mLen );
	                            }

	                            pClstMsg->pPacket	= (void *)pCommPkt;						
	                            if(osMessageAvailableSpace(hTransmitMsg) == 0)
	                            {
	                                osPoolFree( hCommPKPool, (void *)pCommPkt );
	                                osPoolFree( hMsgPool, (void *)pClstMsg );
	                            }
	                            else
	                            {
	                                osMessagePut( hTransmitMsg, (uint32_t)pClstMsg, osWaitForever );
	                            }
	                            osPoolFree( hPTPKPool, (void *)packet );
	                            osPoolFree( hDiagPool, (void *)message );
							}
						}
					}
					else if( message->subEvent == eDIAG_COMM_RX_FAIL )
					{
						GLogI("&");						
#ifdef VCI3_RECORD
						if( message->mPktType == PACKET_RECORD || message->mPktType == PACKET_TRIGGER )
						{
							pCommPkt = ( stCommPkt* )osPoolCAlloc( hCommPKPool );
							if( pCommPkt == NULL )
							{								
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );								
								break;
							}
							pCommPkt->mLen	= packet->DataSize + 24;

							pCommPkt->pTarget  = (void*)message->mPktType;
							memcpy( pCommPkt->mData, packet, pCommPkt->mLen );
							memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
							
							message->pPacket	= (void *)pCommPkt;
							
							extern osMessageQId	hRecordDataMsg;
							if(osMessageAvailableSpace(hRecordDataMsg) == 0)
							{
								osPoolFree( hCommPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
								osMessagePut( hRecordDataMsg, (uint32_t)message, osWaitForever );
							}
							
							osPoolFree( hPTPKPool, (void *)packet );
						}
						else
#endif
						{

							pClstMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
							if( pClstMsg == NULL )
							{
								break;
							}
							pCommPkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
							if( pCommPkt == NULL )
							{
								//osPoolFree( hMsgPool, (void *)message );
								osPoolFree( hMsgPool, (void *)pClstMsg );
								break;
							}

							pClstMsg->mPktType = message->mPktType;
							pCommPkt->mLen	= packet->DataSize + 24;
							memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
							
							pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
							//eDIAG_COMM_RX_FAIL
							if(g_bIsFastInit==TRUE)
							{
								pCommPkt->mLen	= packet->DataSize + 24 + 4;// +4 : IoctlID size add
								pCommPkt->mFuncID  = 0x1006;

								ulIoctlID = FAST_INIT;
							
								memcpy( pCommPkt->mData, &ulIoctlID, 4 ); //Add 'IoctlID' to 'pCommPkt'
								memcpy( pCommPkt->mData+4, packet, pCommPkt->mLen-4 );
							}
							else					
							{
								pCommPkt->mFuncID  = 0x1002;
							
								memcpy( pCommPkt->mData, packet, pCommPkt->mLen );
							}

							pClstMsg->pPacket	= (void *)pCommPkt;						
							if(osMessageAvailableSpace(hTransmitMsg) == 0)
							{
								osPoolFree( hCommPKPool, (void *)pCommPkt );
								osPoolFree( hMsgPool, (void *)pClstMsg );
							}
							else
							{
								osMessagePut( hTransmitMsg, (uint32_t)pClstMsg, osWaitForever );
							}
							osPoolFree( hPTPKPool, (void *)packet );
							osPoolFree( hDiagPool, (void *)message );
						}
					}
					else if( message->subEvent == eDIAG_COMM_RX_PENDING )
					{
						GLogI("%");						

						pClstMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
						if( pClstMsg == NULL )
						{
							break;
						}
						pCommPkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
						if( pCommPkt == NULL )
						{
							osPoolFree( hMsgPool, (void *)pClstMsg );
							break;
						}

						pClstMsg->mPktType = message->mPktType;
						pCommPkt->mLen	= packet->DataSize + 24;
						memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
						pCommPkt->UUID.pendingflag = true;
						
						pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
						//eDIAG_COMM_RX_PENDING
						
						pCommPkt->mFuncID  = 0x1002;						// PENDING FUNCTION ID
						memcpy( pCommPkt->mData, packet, pCommPkt->mLen );

						pClstMsg->pPacket	= (void *)pCommPkt;						
						if(osMessageAvailableSpace(hTransmitMsg) == 0)
						{
							osPoolFree( hCommPKPool, (void *)pCommPkt );
							osPoolFree( hMsgPool, (void *)pClstMsg );
						}
						else
						{
							osMessagePut( hTransmitMsg, (uint32_t)pClstMsg, osWaitForever );
						}
						osPoolFree( hPTPKPool, (void *)packet );
						osPoolFree( hDiagPool, (void *)message );
					}
					else
					{
						osPoolFree( hPTPKPool, (void *)packet );
						osPoolFree( hDiagPool, (void *)message );
						
					}
				}
				break;
#ifdef LISTDIAG
				case FAST_FCS :
				{
					if( message->subEvent == eDIAG_COMM_TX_START )
					{

						message->subEvent = VCI_ProtocolClassification();

						if(message->subEvent == eCAN_TX_NONE_PARSING)
						{
							if(osMessageAvailableSpace(hOBDTxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
								osMessagePut( hOBDTxMessage, (uint32_t)message, osWaitForever );
							}
						}
						else// if(pDiagmsg->subEvent == eKLINE_TX_BLOCK)
						{
							if(osMessageAvailableSpace(hOBDKlineTxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
								osMessagePut( hOBDKlineTxMessage, (uint32_t)message, osWaitForever );
							}
						}
					}
					else if( message->subEvent == eDIAG_COMM_TX_FAIL ){}
					else if( message->subEvent == eDIAG_COMM_RX_OK )
					{
						GLogI("^");
						
						pClstMsg = ( stMsgClst* )osPoolCAlloc( hMsgPool );
						if( pClstMsg == NULL )
						{
							break;
						}
						pCommPkt = ( stCommPkt* )osPoolCAlloc( hCommPKPool );
						if( pCommPkt == NULL )
						{
							//osPoolFree( hMsgPool, (void *)message );
							osPoolFree( hMsgPool, (void *)pClstMsg );
							break;
						}
						
                        pClstMsg->mPktType = message->mPktType;
                        pClstMsg->mMod = LISTDIAG_CHECK;
						pClstMsg->mSeq = message->subEvent;
                        pCommPkt->mLen	= packet->DataSize + 24;
						memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
						
                        pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
                        //eDIAG_COMM_RX_OK
                        if(g_bIsFastInit==TRUE)
                        {
                            pCommPkt->mLen	= packet->DataSize + 24 + 4;// +4 : IoctlID size add
                            pCommPkt->mFuncID  = 0x1006;

                            ulIoctlID = FAST_INIT;
                        
                            memcpy( pCommPkt->mData, &ulIoctlID, 4 ); //Add 'IoctlID' to 'pCommPkt'
                            memcpy( pCommPkt->mData+4, packet, pCommPkt->mLen-4 );
                        }
                        else					
                        {
                            pCommPkt->mFuncID  = LISTDIAG_RES;
                            memcpy( pCommPkt->mData, packet, pCommPkt->mLen );
                        }

                        pClstMsg->pPacket	= (void *)pCommPkt;						
                        if(osMessageAvailableSpace(hListDiagMsg) == 0)
                        {
                            osPoolFree( hCommPKPool, (void *)pCommPkt );
                            osPoolFree( hMsgPool, (void *)pClstMsg );
                        }
                        else
                        {
                            osMessagePut( hListDiagMsg, (uint32_t)pClstMsg, osWaitForever );
                        }
                        osPoolFree( hPTPKPool, (void *)packet );
                        osPoolFree( hDiagPool, (void *)message );
					}
					else if( message->subEvent == eDIAG_COMM_RX_FAIL )
					{
						GLogI("&");

						pClstMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
						if( pClstMsg == NULL )
						{
							break;
						}
						pCommPkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
						if( pCommPkt == NULL )
						{
							//osPoolFree( hMsgPool, (void *)message );
							osPoolFree( hMsgPool, (void *)pClstMsg );
							break;
						}

						pClstMsg->mPktType = message->mPktType;
                        pClstMsg->mMod = LISTDIAG_CHECK;
						pClstMsg->mSeq = message->subEvent;
						pCommPkt->mLen	= packet->DataSize + 24;
						memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
						
						pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
						//eDIAG_COMM_RX_FAIL
						if(g_bIsFastInit==TRUE)
						{
							pCommPkt->mLen	= packet->DataSize + 24 + 4;// +4 : IoctlID size add
							pCommPkt->mFuncID  = 0x1006;

							ulIoctlID = FAST_INIT;
						
							memcpy( pCommPkt->mData, &ulIoctlID, 4 ); //Add 'IoctlID' to 'pCommPkt'
							memcpy( pCommPkt->mData+4, packet, pCommPkt->mLen-4 );
						}
						else
						{
							pCommPkt->mFuncID  = LISTDIAG_RES;
						
							memcpy( pCommPkt->mData, packet, pCommPkt->mLen );
						}

						pClstMsg->pPacket	= (void *)pCommPkt;						
						if(osMessageAvailableSpace(hListDiagMsg) == 0)
						{
							osPoolFree( hCommPKPool, (void *)pCommPkt );
							osPoolFree( hMsgPool, (void *)pClstMsg );
						}
						else
						{
							osMessagePut( hListDiagMsg, (uint32_t)pClstMsg, osWaitForever );
						}
						osPoolFree( hPTPKPool, (void *)packet );
						osPoolFree( hDiagPool, (void *)message );
					}
					else
					{
						osPoolFree( hPTPKPool, (void *)packet );
						osPoolFree( hDiagPool, (void *)message );
					}
				}
				break;
				case LIST_SENSOR :
				{
					if( message->subEvent == eDIAG_COMM_TX_START )
					{

						message->subEvent = VCI_ProtocolClassification();

						if(message->subEvent == eCAN_TX_NONE_PARSING)
						{
							if(osMessageAvailableSpace(hOBDTxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
								osMessagePut( hOBDTxMessage, (uint32_t)message, osWaitForever );
							}
						}
						else// if(pDiagmsg->subEvent == eKLINE_TX_BLOCK)
						{
							if(osMessageAvailableSpace(hOBDKlineTxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
								osMessagePut( hOBDKlineTxMessage, (uint32_t)message, osWaitForever );
							}
						}
					}
					else if( message->subEvent == eDIAG_COMM_TX_FAIL ){}
					else if( message->subEvent == eDIAG_COMM_RX_OK )
					{
						GLogI("^");
						
						pClstMsg = ( stMsgClst* )osPoolCAlloc( hMsgPool );
						if( pClstMsg == NULL )
						{
							break;
						}
						pCommPkt = ( stCommPkt* )osPoolCAlloc( hCommPKPool );
						if( pCommPkt == NULL )
						{
							//osPoolFree( hMsgPool, (void *)message );
							osPoolFree( hMsgPool, (void *)pClstMsg );
							break;
						}
						
                        pClstMsg->mPktType = message->mPktType;
                        pClstMsg->mMod = LISTSENSOR_CHECK;
						pClstMsg->mSeq = message->subEvent;
                        pCommPkt->mLen	= packet->DataSize + 24;
						memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
						
                        pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
                        //eDIAG_COMM_RX_OK
                        if(g_bIsFastInit==TRUE)
                        {
                            pCommPkt->mLen	= packet->DataSize + 24 + 4;// +4 : IoctlID size add
                            pCommPkt->mFuncID  = 0x1006;

                            ulIoctlID = FAST_INIT;
                        
                            memcpy( pCommPkt->mData, &ulIoctlID, 4 ); //Add 'IoctlID' to 'pCommPkt'
                            memcpy( pCommPkt->mData+4, packet, pCommPkt->mLen-4 );
                        }
                        else					
                        {
                            pCommPkt->mFuncID  = LISTSENSOR_RES;
                            memcpy( pCommPkt->mData, packet, pCommPkt->mLen );
                        }

                        pClstMsg->pPacket	= (void *)pCommPkt;						
                        if(osMessageAvailableSpace(hListDiagMsg) == 0)
                        {
                            osPoolFree( hCommPKPool, (void *)pCommPkt );
                            osPoolFree( hMsgPool, (void *)pClstMsg );
                        }
                        else
                        {
                            osMessagePut( hListDiagMsg, (uint32_t)pClstMsg, osWaitForever );
                        }
                        osPoolFree( hPTPKPool, (void *)packet );
                        osPoolFree( hDiagPool, (void *)message );
					}
					else if( message->subEvent == eDIAG_COMM_RX_FAIL )
					{
						GLogI("&");

						pClstMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
						if( pClstMsg == NULL )
						{
							break;
						}
						pCommPkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
						if( pCommPkt == NULL )
						{
							//osPoolFree( hMsgPool, (void *)message );
							osPoolFree( hMsgPool, (void *)pClstMsg );
							break;
						}

						pClstMsg->mPktType = message->mPktType;
                        pClstMsg->mMod = LISTSENSOR_CHECK;
						pClstMsg->mSeq = message->subEvent;
						pCommPkt->mLen	= packet->DataSize + 24;
						memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
						
						pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
						//eDIAG_COMM_RX_FAIL
						if(g_bIsFastInit==TRUE)
						{
							pCommPkt->mLen	= packet->DataSize + 24 + 4;// +4 : IoctlID size add
							pCommPkt->mFuncID  = 0x1006;

							ulIoctlID = FAST_INIT;
						
							memcpy( pCommPkt->mData, &ulIoctlID, 4 ); //Add 'IoctlID' to 'pCommPkt'
							memcpy( pCommPkt->mData+4, packet, pCommPkt->mLen-4 );
						}
						else
						{
							pCommPkt->mFuncID  = LISTSENSOR_RES;
						
							memcpy( pCommPkt->mData, packet, pCommPkt->mLen );
						}

						pClstMsg->pPacket	= (void *)pCommPkt;						
						if(osMessageAvailableSpace(hListDiagMsg) == 0)
						{
							osPoolFree( hCommPKPool, (void *)pCommPkt );
							osPoolFree( hMsgPool, (void *)pClstMsg );
						}
						else
						{
							osMessagePut( hListDiagMsg, (uint32_t)pClstMsg, osWaitForever );
						}
						osPoolFree( hPTPKPool, (void *)packet );
						osPoolFree( hDiagPool, (void *)message );
					}
					else if( message->subEvent == eDIAG_COMM_RX_PENDING )
					{
						GLogI("%");

						pClstMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
						if( pClstMsg == NULL )
						{
							break;
						}
						pCommPkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
						if( pCommPkt == NULL )
						{
							osPoolFree( hMsgPool, (void *)pClstMsg );
							break;
						}

						pClstMsg->mPktType = message->mPktType;
						pCommPkt->mLen	= packet->DataSize + 24;
						memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
						pCommPkt->UUID.pendingflag = true;
						
						pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
						//eDIAG_COMM_RX_PENDING
						
						pCommPkt->mFuncID  = 0x1002;						// PENDING FUNCTION ID 1004 -> 1002 ����
						memcpy( pCommPkt->mData, packet, pCommPkt->mLen );

						pClstMsg->pPacket	= (void *)pCommPkt;						
						if(osMessageAvailableSpace(hTransmitMsg) == 0)
						{
							osPoolFree( hCommPKPool, (void *)pCommPkt );
							osPoolFree( hMsgPool, (void *)pClstMsg );
						}
						else
						{
							osMessagePut( hTransmitMsg, (uint32_t)pClstMsg, osWaitForever );
						}
						osPoolFree( hPTPKPool, (void *)packet );
						osPoolFree( hDiagPool, (void *)message );
					}
					else
					{
						osPoolFree( hPTPKPool, (void *)packet );
						osPoolFree( hDiagPool, (void *)message );
					}
				}
				break;
#endif

#ifdef USE_RELAY_MOSA
				case CAN_MOSA:
				{
					printf(">>>>>>>>>>>>>>> MOSAMOSA\r\n");


				}
				break;
#endif
                case CAN_CSAC :
				{
					if( message->subEvent == eDIAG_COMM_TX_START )
					{

						message->subEvent = VCI_ProtocolClassification();

						if(message->subEvent == eCAN_TX_NONE_PARSING)
						{
							if(osMessageAvailableSpace(hOBDTxMessage) == 0)
							{
								osPoolFree( hPTPKPool, (void *)packet );
								osPoolFree( hDiagPool, (void *)message );
							}
							else
							{
								osMessagePut( hOBDTxMessage, (uint32_t)message, osWaitForever );
							}
                        }
                        else{}
					}
                    else if( message->subEvent == eDIAG_COMM_TX_FAIL ){}
					else if(( message->subEvent == eDIAG_COMM_RX_OK ) || ( message->subEvent == eDIAG_COMM_RX_FAIL ))
					{
						GLogI("^");
						
						pClstMsg = ( stMsgClst* )osPoolCAlloc( hMsgPool );
						if( pClstMsg == NULL )
						{
							break;
						}
						pCommPkt = ( stCommPkt* )osPoolCAlloc( hCommPKPool );
						if( pCommPkt == NULL )
						{
							//osPoolFree( hMsgPool, (void *)message );
							osPoolFree( hMsgPool, (void *)pClstMsg );
							break;
						}
						
                        pClstMsg->mPktType = message->mPktType;
                        pClstMsg->mMod = CSACDIAG_CHECK;
						pClstMsg->mSeq = message->subEvent;
                        pCommPkt->mLen	= packet->DataSize + 24;
						memcpy(&(pCommPkt->UUID), &(packet->UUID), sizeof(UUID_Struct));
						
                        pCommPkt->pTarget  = (void*)pClstMsg->mPktType;
                        //eDIAG_COMM_RX_OK
                        pCommPkt->mFuncID  = LISTDIAG_RES;
                        memcpy( pCommPkt->mData, packet, pCommPkt->mLen );

                        pClstMsg->pPacket	= (void *)pCommPkt;						
                        if(osMessageAvailableSpace(hCsacDiagMsg) == 0)
                        {
                            osPoolFree( hCommPKPool, (void *)pCommPkt );
                            osPoolFree( hMsgPool, (void *)pClstMsg );
                        }
                        else
                        {
                            osMessagePut( hCsacDiagMsg, (uint32_t)pClstMsg, osWaitForever );
                        }
                        
                        osPoolFree( hPTPKPool, (void *)packet );
                        osPoolFree( hDiagPool, (void *)message );
					}
                }
                break;
                
				default:
				{
					GLogE( "unknown Diag type( %d )!!! \r\n", usDiagtype );
				}
				break;
			}
		}
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
    osThreadYield();
#endif
	}
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
