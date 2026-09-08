/* Includes ------------------------------------------------------------------*/
#include "GIT_Util.h"
// member variable for work list.
#include "Autolink_Manager.h"
#include "AutolinkConfig.h"
#include "Message_Manager.h"
#include "Modem_Manager.h"
#include "Power_Manager.h"
#include "MngSystem.h"
#include "MngSystemUtil.h"
#include "GIT_BluetoothLowEnergy.h"
#include "MngQueue.h"
#include "HdDebug.h"
#include "DebugHandler.h"
#include "MngModem.h"
#include "HalHandler.h"
#include "OBD_Controller.h"
#include "Share_InterFunction.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)


#pragma section="BKSRAM"
BR_SystemInfo BkSram_SystemInfo @"BKSRAM";

extern eVEHICLE_STATE Get_VehicleStatus(void);
extern unsigned int g_unBkramSwResetSignal;
extern unsigned short int g_usBkramResetCount;

enum{
    STAT_SYS_MNG_INIT = 0,
    STAT_SYS_MNG_SLEEP,
    STAT_SYS_MNG_RUN,
    STAT_SYS_MNG_IDLE,
};


uint32_t m_unSleepEvent = 0;
uint32_t m_unWaitInterrupFlag = 0;
uint32_t m_unWakeupInterrupFlag = 0;

boolean_t m_bDeviceReset = false;
boolean_t m_bRequestSleepModem = false;

int32_t m_nMngSysState = STAT_SYS_MNG_INIT;
boolean_t m_bWaitForSleep = false;
boolean_t m_bWaitBluetoothResponse = false;

// internal function list
int32_t doNothing(int32_t a,int32_t b){return -1;}
int32_t HdMngSysResp(int32_t nEvt,int32_t nReserved);
//int32_t HdMngSysBtResp(int32_t nEvt,int32_t nReserved);
int32_t HdMngSysResp(int32_t nEvt,int32_t nReserved);
int32_t HdMngSysMsgResp(int32_t nEvt,int32_t nReserved);
int32_t HdMngSysSensorResp(int32_t nEvt,int32_t nReserved);
int32_t HdMngSysMsgInit();
void MngSystem();
void MngSystemInit();
void MngSysExternalEvent(stMsgSys* pstReport);

void SetMngSysState(int32_t nState);
void CheckSysMngEvent(int32_t* pnLparam,int32_t* pnRparam);
void SetupInterruptLatch(boolean_t bPowerOff);
boolean_t CheckInterruptSignal();
void SendObdSetting();

void SystemForcelyResetForPowerOn();

void SleepTimerCallBack(void);
void CheckColdBooting();
void SetupForSleep();
boolean_t GetRequestSystemReset();
boolean_t m_bRcvSystemResetCmd = false;
extern unsigned char g_ucI2CFlag[2];
extern uint32_t m_unModemControlOldState;

// extennal function list
extern void SetupForInterruptforImpulse(bool bWomActive, bool bGyroActive, uint8_t ucValue, bool bHelpInterrupSignal);
extern boolean_t WriteBackupRam();
extern stMsgHandlerData m_stMsgHdData;
extern void ConfigforAlram(boolean_t bWakeupSoon, boolean_t bReqLongSleep);
extern void SystemDelay(uint16_t delay);
extern int8_t *GetStringFromEvent(int32_t nMode,int32_t nEvent,int32_t nSubEvent);
extern int8_t *GetStringFromId(int32_t nId);
extern void HandlerNewVin(char* carrNewVin);
extern void SendFotaAlramReport(int32_t nEvent, int32_t nResult);
extern void ReqForcelyModemReset();
extern void Can_DeInit_Sleep(void);
extern uint8_t GetGuardActiveLevel();
extern void SetWDTReset(uint16_t msDelay);
extern MODEM_MANAGER_DATA ModemManagerData;
extern void MD_mDelay (const uint32_t msec);
extern void EnterStandbyModeforSystemOnly(void);

// this is not used because bt module is not implemented.
//stMngSysData m_MngSysData = {{doNothing,HdMngSys,HdMngSysMsg,HdMngSysBt,HdMngSysSensor},
//                             {doNothing,HdMngSysResp,HdMngSysMsgResp,HdMngSysBtResp,HdMngSysSensorResp}};

// this work list is just test.
stMngSysData m_MngSysData = {false,true,SYSTEM_MODE_PARKING,
                            {doNothing,HdMngSys,HdMngSysMsg,HdMngSysSensor},
                            {doNothing,HdMngSysResp,HdMngSysMsgResp,HdMngSysSensorResp}};


// check event from system or managers
// not defined yet,
void CheckSysMngEvent(int32_t* pnLparam,int32_t* pnRparam)
{
    *pnLparam =0;
    *pnRparam =0;
}


// time out interval
void SleepTimerCallBack(void)
{
    // if other manager wants to go sleep, this mananger request upper manager
    // and wait for event from upper manager.
    SetMngSysState(STAT_SYS_MNG_SLEEP);

    if( m_bWaitForSleep == false )
    {
        // sleep check bit clear
        m_unSleepEvent = 0x0;
    }

#ifndef USE_GEMALTO_MODEM
    m_unSleepEvent = SYS_MNG_HD_BT_SLEEP_OK;
#endif

    m_bWaitForSleep = false;
    Trace("==========================================\r\n");
    Trace("%s] Start Sleep Process\r\n",__FUNCTION__);
    Trace("==========================================\r\n");

#ifndef BLE_NEW_VERSION
    // request advertising to blueooth module
    BTSetRepeatModeReq(1000,1000);
#endif

#ifdef RF_COMMON_MODEM //BNCOM
	BTSetRepeatModeReq(1000,1000);
#endif
    //BTStartAdvertisingReq();
}


void MngSystemInit()
{
    m_bWaitForSleep = false;
}

void MngSystem(void)
{
    int32_t nLparam, nRparam;
    stMsgSys stSysMessage;
    //Check Event from Manager or System
    CheckSysMngEvent(&nLparam,&nRparam);
	static bool bIsNeedUpdateStatus=false;

    // check external event
#ifndef GLOBAL_SHARE_QUEUE //Get
    if( MngQueueGetMessage(ID_MNG_QUEUE_SYS,(int8_t*)&stSysMessage, sizeof(stSysMessage)) == true )
#else
	if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_SYS, (uint8_t*)&stSysMessage.header, sizeof(stMsgHeader), (uint8_t*)&stSysMessage.rpSetting, sizeof(stReportSetting)) == true )
#endif
    {
#if defined(MNG_QUEUE_DEBUG)
        Trace("event : %s, subEvent : %s, from : %s, reuslt : %x\r\n",
            GetStringFromEvent(eGetStringEvent,stSysMessage.header.event,0),
            GetStringFromEvent(eGetStringSubEvent,stSysMessage.header.event,stSysMessage.header.subEvent),
            GetStringFromId(stSysMessage.header.unTraceMng&0xFF),
            stSysMessage.header.result);
#endif

        // handle external event
        MngSysExternalEvent(&stSysMessage);
    }

    //Msg Main State Handler
    switch(m_nMngSysState)
    {
        case STAT_SYS_MNG_INIT:

            // for test always parking mode
            m_MngSysData.m_iSystemMode = SYSTEM_MODE_PARKING;

#ifndef BLE_NEW_VERSION
            m_bWaitBluetoothResponse = false;
#endif

            // send interrupt check message to sensor manager
            Send2MngSensor(eMngSys, eReqWakeupInterruptInfo, 0, (stCarReport *)NULL,0);

            // Send odometer to obd manager
            // SendObdSetting();

            // send interrupt check message to system message handler
            nLparam = eMngSys;
            nRparam = eReqWakeupInterruptInfo;

            // MONI 20180529
            // move this code to network check process because the event time
            // cold booting check
            // if cold booting, ME'll report firmware info alram
            // CheckColdBooting();

            // change main state
            SetMngSysState(STAT_SYS_MNG_RUN);

            break;
        case STAT_SYS_MNG_SLEEP:

            if( m_bWaitForSleep == false )
            {
                m_bWaitForSleep = true;

                // request sleep to all handler once.
                nLparam = eMngSys;
                nRparam = eReqSleep;
            }

#if defined(BLE_NEW_VERSION)
			m_unSleepEvent|=SYS_MNG_HD_BT_SLEEP_OK;
#ifdef RF_COMMON_MODEM //BNCOM
			HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_RESET); //mod.kks need to set the pin after ADVINTERVAL
#endif
#else
            if( BTGetState() == eBT_Sleep_Complete && m_bWaitBluetoothResponse == false )
            {
                m_bWaitBluetoothResponse = true;
                m_unSleepEvent|=SYS_MNG_HD_BT_SLEEP_OK;
            }
#endif

            //Trace("%s] state : STAT_SYS_MNG_SLEEP\r\n",__FUNCTION__);
            if( m_unSleepEvent == SYS_MNG_SLEEP_MASK )
            {
                // go to sleep all peripheral was gone to sleep.
                Trace("%s] state : STAT_SYS_MNG_SLEEP\r\n",__FUNCTION__);

                // set up all peripheral
                SetupForSleep();
            }

            break;
        case STAT_SYS_MNG_RUN:
            //Trace("%s] state : STAT_SYS_MNG_RUN\r\n",__FUNCTION__);
            break;
        case STAT_SYS_MNG_IDLE:
            //Trace("%s] state : STAT_SYS_MNG_IDLE\r\n",__FUNCTION__);
            if( m_bWaitForSleep == false )
            {
                m_bWaitForSleep = true;

                // request sleep to all handler once.
                nLparam = eMngSys;
                nRparam = eReqChangeMode;
            }
            break;
        default:
            Trace("%s] state : default\r\n",__FUNCTION__);
            Trace("%s]Error not define state\r\n",__FUNCTION__);
            break;
    }

	if(BTGetConnectStatus() == true)
	{
		bIsNeedUpdateStatus = true;
	}
	else
	{
		if(bIsNeedUpdateStatus == true)
		{
			BTSend_VehicleStatus(Get_VehicleStatus());
			bIsNeedUpdateStatus = false;
		}
	}
    //Work List Process
    //1. Sys Handler
    //2. Obd Handler
    //3. Mdm Handler
    //4. Bt Handler
    //5. Sensor Handler

    for(int32_t i=SYS_WORK_NONE+1;i<MAX_SYS_WORK_LIST;i++)
    {
        if( m_MngSysData.fnMngSysWorkList[i] != NULL )
        {
            int32_t ret = m_MngSysData.fnMngSysWorkList[i](nLparam,nRparam);

            // if exits event from mananger hander,
            // process after handelr functions
            if( ret != -1 )
            {
                m_MngSysData.fnMngSysWorkListResp[i](ret,0);
            }
        }
    }
}

boolean_t m_bNetworkTime = false;

void SetReceivedNetworkTime(boolean_t bNetworkTime)
{
    m_bNetworkTime = bNetworkTime;
}

boolean_t GetReceivedNetworkTime()
{
    return m_bNetworkTime;
}

void CheckColdBooting()
{
    if( AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON )
    {
        // we should report firmware info alram
        SendFotaAlramReport(eMESSAGE_EVENT_KEY_FOTA_COMPLETE_ALRAM,eREMOTE_CON_FOTA_TYPE_BOOT);// complete
    }
}

int32_t HdMngSysBt(int32_t nLparam, int32_t nRparam)
{
    return 0;
}

int32_t HdMngSysResp(int32_t nEvent,int32_t nReserved)
{
    //Trace("%s] event = %x\r\n",__FUNCTION__,evt);

    if( nEvent == eRspSleep )
    {
        Trace("%s] sub system sleep is ready\r\n",__FUNCTION__);
        //m_bWaitForSleep = true;
        //SetMngSysState(STAT_SYS_MNG_SLEEP);

        m_unSleepEvent |= SYS_MNG_HD_SYS_SLEEP_OK;
    }

    return 0;
}
int32_t HdMngSysMsgResp(int32_t nEvent,int32_t nReserved)
{

    if( nEvent == eRspSleep )
    {
        Trace("%s] sub message sleep is ready\r\n",__FUNCTION__);

        m_unSleepEvent |= SYS_MNG_HD_MSG_SLEEP_OK;
    }
    //Trace("%s] event = %x\r\n",__FUNCTION__,evt);
    return 0;
}
int32_t HdMngSysBtResp(int32_t nEvent,int32_t nReserved)
{
    if( nEvent == eRspSleep )
    {
        Trace("%s] sub bt sleep is ready\n",__FUNCTION__);

        m_unSleepEvent |= SYS_MNG_HD_BT_SLEEP_OK;
    }

    //Trace("%s] event = %x\r\n",__FUNCTION__,evt);
    return -1;
}
int32_t HdMngSysSensorResp(int32_t nEvent,int32_t nReserved)
{
    if( nEvent == eRspSleep )
    {
        Trace("%s] sub bt sleep is ready\r\n",__FUNCTION__);

        m_unSleepEvent |= SYS_MNG_HD_SENSOR_SLEEP_OK;
    }

    //Trace("%s] event = %x\r\n",__FUNCTION__,evt);
    return 0;
}


/* Mananger roll
1. System Handler
    - related with power such as sleep, wake up, reset etc.
2. Obd Handler
    - related with report to the server
3. Mdm Handler
    - related with send / receive the message */

uint8_t m_cRcvReqEvent = 0;

void MngSysExternalEvent(stMsgSys* pstReport)
{
    if( pstReport->header.id == eMngObd )
    {
        if( pstReport->header.event == eReqSleep )
        {
            // not used this event
            // sleep process is in system message handler
            //EnableSleepTimer(20000);
            //EnableSleepTimer(3000);
            //Trace("%s]Get Sleep Event from other Manager\r\n",__FUNCTION__);
            //Trace("%s]Wait 10s and then go to sleep\r\n",__FUNCTION__);
        }
        else if( pstReport->header.event == eReqSetting )
        {
            m_stMsgHdData.stObdSettingValue.ObdSetting.unOdometer = pstReport->rpSetting.ObdSetting.unOdometer;
            m_stMsgHdData.stObdSettingValue.ObdSetting.dlLatitue = pstReport->rpSetting.ObdSetting.dlLatitue;
            m_stMsgHdData.stObdSettingValue.ObdSetting.dlLongitude = pstReport->rpSetting.ObdSetting.dlLongitude;
            m_stMsgHdData.stObdSettingValue.ObdSetting.fRemainedFuel = pstReport->rpSetting.ObdSetting.fRemainedFuel;

            Trace("Setting from obd Odometer : %d\r\n",m_stMsgHdData.stObdSettingValue.ObdSetting.unOdometer);
            Trace("Setting from obd latitude : %f\r\n",m_stMsgHdData.stObdSettingValue.ObdSetting.dlLatitue);
            Trace("Setting from obd longitude : %f\r\n",m_stMsgHdData.stObdSettingValue.ObdSetting.dlLongitude);
            Trace("Setting from obd fRemainedFuel : %f\r\n",m_stMsgHdData.stObdSettingValue.ObdSetting.fRemainedFuel);
        }
        else if( pstReport->header.event == eOBDStatus )
        {
            if( pstReport->header.subEvent == eNewVin )
            {
                Trace("RCV : NEW BIN : %s\r\n",pstReport->rpSetting.ObdSetting.carrVin);

                // received new vin / notify system to check it is allowed or not
                HandlerNewVin((char *)pstReport->rpSetting.ObdSetting.carrVin);
            }
        }
        else
        {
            GIT_Assert(false,eErrorCodeSys|eEventUnknown);
        }
    }
    else if( pstReport->header.id == eMngSysMsg )
    {
        if( pstReport->header.event == eReqSleep )
        {
            m_cRcvReqEvent++;
            // start sleep process
            SleepTimerCallBack();
            Trace("%s]Rcv Sleep Event from Message Manager\r\n",__FUNCTION__);
            Trace("%s]Request sleep count : %d, go to sleep \r\n",__FUNCTION__,m_cRcvReqEvent);

            if( m_cRcvReqEvent > 6 )
            {
                // this is emergency sleep
                // Step#1, reset modem
                // Step#2, set up before go sleep
                // Step#3, Sleep.
                ReqForcelyModemReset();

                SetupForSleep();
            }
        }
        else if( pstReport->header.event == eReqSystemReset )
        {
            Trace("#########################################################\r\n");
            Trace("### After fota, ME will be reset system.###\r\n");

            m_bDeviceReset = true;
            SleepTimerCallBack();
        }
    }
    else if( pstReport->header.id == eMngModem )
    {
        if( pstReport->header.event == eReqSystemReset )
        {
            Trace("#########################################################\r\n");
            Trace("### This is a device reset exception process of modem ###\r\n");
            Trace("#########################################################\r\n");

            m_bDeviceReset = true;
            SleepTimerCallBack();

        }
        else if( pstReport->header.event == eReqSleep )
        {
            Trace("#########################################################\r\n");
            Trace("### This is a sleep exception process of modem ###\r\n");
            Trace("#########################################################\r\n");
            // after that we should add recovery process
            // start sleep process

            m_bRequestSleepModem = true;
            SleepTimerCallBack();
            // Trace("%s]Get Sleep Event from other Manager\r\n",__FUNCTION__);
            // Trace("%s]Wait 10s and then go to sleep\r\n",__FUNCTION__);
        }
    }
    else if( pstReport->header.id == eMngSys )
    {
        if( pstReport->header.event == eReqChangeMode )
        {
            SetMngSysState(STAT_SYS_MNG_IDLE);
        }
    }
    else
    {
        // not defined
        // error
        GIT_Assert(false,eErrorCodeSys|eUnknownId);
    }
}

void SetMngSysState(int32_t nState)
{
    m_nMngSysState = nState;
}

void SetupInterruptLatch(boolean_t bPowerOff)
{
    /*
    GPIO_LOW_CAN_RX_CTL     //PA5
    GPIO_CAN_RX_CTL         //PB11
    GPIO_MO_WAKE_CTL        //PB12
    GPIO_BT_MON_CTL         //PB8
    GPIO_IG_ACC_DET_CTL     //PC7
    GPIO_LATCH_CLK          //PC0
    GPIO_HI_CAN2_CHK_CTL    //PE9
    GPIO_LOW_HIGH_CAN_SEL   //PE10
    */
    //HalGPIOGetStatus(GPIOC, GPIO_MO_WAKE);

    if( bPowerOff == true )
    {
        // all interrup set low
        HalGPIOSetVaule(GPIO_LOW_CAN_RX_CTL, eBIT_RESET);
        HalGPIOSetVaule(GPIO_CAN_RX_CTL, eBIT_RESET);
        HalGPIOSetVaule(GPIO_MO_WAKE_CTL, eBIT_RESET);
        HalGPIOSetVaule(GPIO_BT_MON_CTL, eBIT_RESET);
		// blcok ig off for exception case in maxico
		// some car ig held with high until battery is go down 12.6
        HalGPIOSetVaule(GPIO_IG_ACC_DET_CTL, eBIT_SET);
        // select high can 1 : false
        // select high can 2 : true
        //HalGPIOSetVaule(GPIOE,GPIO_HI_CAN2_CHK_CTL,false);
        // select low can : true
        // select high can 3 : false
        HalGPIOSetVaule(GPIO_LOW_HIGH_CAN_SEL, eBIT_RESET);

    }
    else
    {
        // all interrup set high
        HalGPIOSetVaule(GPIO_LOW_CAN_RX_CTL, eBIT_SET);
        HalGPIOSetVaule(GPIO_CAN_RX_CTL, eBIT_SET);
        HalGPIOSetVaule(GPIO_MO_WAKE_CTL, eBIT_RESET);
        HalGPIOSetVaule(GPIO_BT_MON_CTL, eBIT_RESET);
        HalGPIOSetVaule(GPIO_IG_ACC_DET_CTL, eBIT_SET);
        // select high can 1 : false
        // select high can 2 : true
        //HalGPIOSetVaule(GPIOE,GPIO_HI_CAN2_CHK_CTL,false);
        // select low can : true
        // select high can 3 : false
        HalGPIOSetVaule(GPIO_LOW_HIGH_CAN_SEL, eBIT_SET);

    }
  
    HalGpioGenerateLatchClock();
}

//typedef enum _eWAKE_PINS
//{
//	eWAKE_PIN_IG_ON = 0,
//	eWAKE_PIN_ACC,
//	eWAKE_PIN_MODEM,
//	eWAKE_PIN_SENSOR,
//	eWAKE_PIN_LOW_HIGH_CAN_RX,
//	eWAKE_PIN_BT_MON,
//	eWAKE_PIN_CAN_RX,
//
//	eWAKE_PIN_MAX,
//} eWAKE_PINS;

boolean_t CheckCanInerruptSignal()
{
    int nChannel1HighCount = 0;
    int nChannel1LowCount = 0;
    int nChannel2HighCount = 0;
    int nChannel2LowCount = 0;

    for(int i=0;i<100;i++)
    {
        if( GetWakeupDetectPinState(eWAKE_PIN_CAN_RX) == true)
            nChannel1HighCount++;
        else
            nChannel1LowCount++;

        if( GetWakeupDetectPinState(eWAKE_PIN_LOW_HIGH_CAN_RX) == true)
            nChannel2HighCount++;
        else
            nChannel2LowCount++;

        MD_mDelay(1);
    }

    printf(" Channel1 high : %d, low high can : %d\r\n",nChannel1HighCount,nChannel1LowCount);
    printf(" Channel2 high : %d, low high can : %d\r\n",nChannel2HighCount,nChannel2LowCount);

    if( nChannel1HighCount>0 && nChannel1LowCount>0 )
        return true;

    if( nChannel2HighCount>0 && nChannel2LowCount>0 )
        return true;

    return false;
}

boolean_t CheckInterruptSignal()
{
    boolean_t bIntModem = GetWakeupDetectPinState(eWAKE_PIN_MODEM);
    boolean_t bIntSensor = GetWakeupDetectPinState(eWAKE_PIN_SENSOR/*GPIO_SENSOR_INT*/);
    boolean_t bIntBt = GetWakeupDetectPinState(eWAKE_PIN_BT_MON);

    printf("Mdm : %d, Sensor : %d, Bt : %d\r\n",bIntModem,bIntSensor,bIntBt);

    // we need to test ring signal because duration of ring singal is 1secods
    // whether it is enough or not
    if( (bIntModem == true) ||
        (bIntBt == true) ||
        (bIntSensor == true) ||
        (CheckCanInerruptSignal() == true)
        )
        // we don't need to check sensor signal
        // because if we check sensor signal, after go to sleep.
        // the device will be wake up right away because of sensor signal
        //|| (GetWakeupDetectPinState(eWAKE_PIN_SENSOR/*GPIO_SENSOR_INT*/) == true)
        //|| (GetWakeupDetectPinState(eWAKE_PIN_BT_MON) == true)
    {
        printf("Interrupt Occurred before sleep\r\n");

        return true;
    }
    else
    {
        printf("Interrupt Not Occurred before sleep\r\n");
    }

    return false;
}


boolean_t CheckInterruptSignal2()
{
    boolean_t bIntModem = false;
    boolean_t bIntSensor = false;
    boolean_t bIntBt = false;
    int nChannel1HighCount = 0;
    int nChannel1LowCount = 0;
    int nChannel2HighCount = 0;
    int nChannel2LowCount = 0;

    for(int i=0;i<100;i++)
    {
        if( HalGPIOGetStatus(GPIO_CAN_RX_MON) == true)
            nChannel1HighCount++;
        else
            nChannel1LowCount++;

        if( HalGPIOGetStatus(GPIO_LOW_HIGH_CAN_RX_MON) == true)
            nChannel2HighCount++;
        else
            nChannel2LowCount++;

        if( (HalGPIOGetStatus(GPIO_MO_WAKE) == true) ||
        (HalGPIOGetStatus(GPIO_SENSOR_INT) == true) ||
        (HalGPIOGetStatus(GPIO_BT_STATUS) == true) )
        {
        	bIntModem = HalGPIOGetStatus(GPIO_MO_WAKE);
			bIntSensor = HalGPIOGetStatus(GPIO_SENSOR_INT);
			bIntBt = HalGPIOGetStatus(GPIO_BT_STATUS);
            printf("Mdm2 : %d, Sensor : %d, Bt : %d\r\n",bIntModem,bIntSensor,bIntBt);
            printf("Interrupt Occurred before sleep\r\n");
            return true;
        }

		if( ModemManagerData.bNeedStartUpCheckSMSFlag == true && (m_unModemControlOldState == eModemControlStateReConnected || m_unModemControlOldState == eModemControlStateConnected) )
		{
            printf("Rcv SMS during Sleep\r\n");
            return true;
		}

        MD_mDelay(1);
    }

	bIntModem = HalGPIOGetStatus(GPIO_MO_WAKE);
	bIntSensor = HalGPIOGetStatus(GPIO_SENSOR_INT);
	bIntBt = HalGPIOGetStatus(GPIO_BT_STATUS);
    printf("Mdm3 : %d, Sensor : %d, Bt : %d\r\n",bIntModem,bIntSensor,bIntBt);
    printf(" Channel1 high : %d, low high can : %d\r\n",nChannel1HighCount,nChannel1LowCount);
    printf(" Channel2 high : %d, low high can : %d\r\n",nChannel2HighCount,nChannel2LowCount);

    if( nChannel1HighCount>0 && nChannel1LowCount>0 )
        return true;

    if( nChannel2HighCount>0 && nChannel2LowCount>0 )
        return true;

    printf("Interrupt Not Occurred before sleep\r\n");

    return false;
}

void SetObdSetting(stAutolinkConfigData* pstAutolinkConfigData)
{
    stMsgObd stMsgOdometer;

    // setting properties of system
    m_stMsgHdData.stObdSettingValue.ObdSetting.unOdometer = pstAutolinkConfigData->stSystem.unOdometer;
    m_stMsgHdData.stObdSettingValue.ObdSetting.dlLatitue = pstAutolinkConfigData->stSystem.dlLastLatitude;
    m_stMsgHdData.stObdSettingValue.ObdSetting.dlLongitude = pstAutolinkConfigData->stSystem.dlLastLongitude;
	m_stMsgHdData.stObdSettingValue.ObdSetting.fRemainedFuel = pstAutolinkConfigData->stSystem.fRemainedFuel;
    m_stMsgHdData.stObdSettingValue.ObdSetting.ucDoorLockStatus = BkSram_SystemInfo.ucDoorLockStatus;
    m_stMsgHdData.stObdSettingValue.ObdSetting.ucDoorOpenStatus = BkSram_SystemInfo.ucDoorOpenStatus;
    m_stMsgHdData.stObdSettingValue.ObdSetting.ucHeadLight = BkSram_SystemInfo.ucHeadLight;

    //sending data to obd manager.
    stMsgOdometer.header.id = eMngSys;
    stMsgOdometer.header.event = eReqSetting;
    // set iniiatlize value for reset

    stMsgOdometer.carReport.rpSetting.ObdSetting.unOdometer = m_stMsgHdData.stObdSettingValue.ObdSetting.unOdometer;
    stMsgOdometer.carReport.rpSetting.ObdSetting.dlLatitue = m_stMsgHdData.stObdSettingValue.ObdSetting.dlLatitue;
    stMsgOdometer.carReport.rpSetting.ObdSetting.dlLongitude = m_stMsgHdData.stObdSettingValue.ObdSetting.dlLongitude;
    stMsgOdometer.carReport.rpSetting.ObdSetting.ucDoorLockStatus = m_stMsgHdData.stObdSettingValue.ObdSetting.ucDoorLockStatus;
    stMsgOdometer.carReport.rpSetting.ObdSetting.ucDoorOpenStatus = m_stMsgHdData.stObdSettingValue.ObdSetting.ucDoorOpenStatus;
    stMsgOdometer.carReport.rpSetting.ObdSetting.ucHeadLight = m_stMsgHdData.stObdSettingValue.ObdSetting.ucHeadLight;
	stMsgOdometer.carReport.rpSetting.ObdSetting.fRemainedFuel = m_stMsgHdData.stObdSettingValue.ObdSetting.fRemainedFuel;

    Send2MngObd2(&stMsgOdometer);
}

void EnableInteruptSources()
{
    // set up latch for interrupt
    SetupInterruptLatch(true);
}

void DsiableInterruptSources()
{
    MPU6515_writeByte(MPU6515_ADDRESS, INT_ENABLE, 0x00);
    // disable gyro and accelerometer
    MPU6515_writeByte(MPU6515_ADDRESS, PWR_MGMT_2, 0x3F);
    // disable latch
    SetupInterruptLatch(false);
}

bool CheckDeviceResetSignal()
{
	HalDrvRdBkRam((unsigned char*)&g_usBkramResetCount, BKRAM_RESETCOUNT_ADDR, BKRAM_RESETCOUNT_SIZE);
	if( g_usBkramResetCount >= DEVICE_RESET_COUNT_LIMIT )
		return true;
	else
		return false;
}
void PlusDeviceResetCount()
{
	HalDrvRdBkRam((unsigned char*)&g_usBkramResetCount, BKRAM_RESETCOUNT_ADDR, BKRAM_RESETCOUNT_SIZE);
    g_usBkramResetCount++;
	HalDrvWrBkRam((unsigned char*)&g_usBkramResetCount, BKRAM_RESETCOUNT_ADDR, BKRAM_RESETCOUNT_SIZE);
}
void ClearDeviceResetCount()
{
    g_usBkramResetCount = 0;
	HalDrvWrBkRam((unsigned char*)&g_usBkramResetCount, BKRAM_RESETCOUNT_ADDR, BKRAM_RESETCOUNT_SIZE);
}
// [TEST] true 이면 module sleep(standby) 진입을 하지 않는다. (bench test 용)
boolean_t g_bTestDisableSleep = false;

void SetupForSleep()
{
    // [TEST] sleep 을 disable 한 경우, standby 진입 없이 즉시 반환하여 깨어있는 상태를 유지한다.
    if( g_bTestDisableSleep == true )
    {
        Trace("SetupForSleep SKIPPED (test mode: sleep disabled)\r\n");
        return;
    }

    // write configuration data to serial flash
    WriteConfig(true, false);

    // check ring signal before going to sleep
    // if ring signal is high, we should wake up right away
    if( CheckInterruptSignal2() == true )
    {
        m_bDeviceReset = true;
    }

    // disable all interrupt
    DsiableInterruptSources();

	// set up main chip for sleep
	EnterStandbyModeforSystemOnly();	

#ifdef USE_96_HOURS_POWER_OFF
    // do not wake up with rtc alram and modem.
    // 1. power off modem
    // 2. not sett up rtc
    // modem will be power off in the modem manager
    // now ME just don't set up rtc
    // ME will be wake up by impulse or can
    if( BkSram_SystemInfo.bModemPowerOff ==  false && m_bDeviceReset == false )
    {
        uint8_t bActive = 0;
        GetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bActive);

        if( bActive == true )
        {
            ConfigforAlram(m_bDeviceReset,m_bRequestSleepModem);
        }
        else
        {
            Trace("RTC Alarm Disable because of Modem Active\r\n");
            // disable alram interrupt
            HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_DISABLE);
        }
    }
    else
    {
        Trace("Modem Power Off is Enabled\r\n");
        // disable alram interrupt
        HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_DISABLE);
    }  
    
#else //#ifdef USE_96_HOURS_POWER_OFF
    // set wakeup alram
    // m_bDeviceReset default is false
    // when modem request device reset then m_bDeviceReset is true;
    // m_bRequestSleepModem deufalt is false
    // when modem request device sleep then m_bRequestSleepModem is true;
    ConfigforAlram(m_bDeviceReset,m_bRequestSleepModem);
#endif //#ifdef USE_96_HOURS_POWER_OFF

	if( (g_ucI2CFlag[0] != 0) || (g_ucI2CFlag[1] != 0) )	SetupForInterruptforImpulse(true,false,GetGuardActiveLevel(),false);
	else													SetupForInterruptforImpulse(true,false,GetGuardActiveLevel(),true);

    // enable all interrupt
    EnableInteruptSources();

    printf("%s] GetRequestSystemReset : %d, m_bDeviceReset :%d\r\n",__FUNCTION__,GetRequestSystemReset(),m_bDeviceReset);
    // check ring signal before going to sleep
    // if ring signal is high, we should wake up right away
    if( (CheckInterruptSignal2() == true) || (CheckDeviceResetSignal() == true) )
    {
        m_bDeviceReset = true;
    }

#ifndef ENABLE_STANDBY_MODE	
    UpdateBackupRam(true);	// update all backup ram structure
#endif

    if( GetRequestSystemReset() == false && m_bDeviceReset == false )
    {
        unsigned int unSystick_index = 0;

        printf("ENTER STANDBY OR STOP(DeepSleep) MODE\r\n");
        while(HalUartGetFlagStatus((int)g_stUart7.pUARTreg, HAL_USART_FLAG_TC)== HAL_RESET);
        /********************************************************************/
        /* Don't print Debug Messages below									*/
        /********************************************************************/		
        HalHandlerDeInit();

#ifdef ENABLE_STANDBY_MODE
        HalDrvPowerIOCtrl(ePWR_IO_WakeupPinEnable, 0, NULL, 0, HAL_ENABLE);
        HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStandbyMode, 0, NULL, 0, 0);
#else
        HalDrvRtcIOCtrl(eRtc_IO_ClearFlagStatus, HAL_RTC_FLAG_ALRAF, NULL, 0, 0);
		HalDrvPwr_WkupPin_Config(HAL_WKUPPIN_INT);

        /* save systick register configuration */
        unSystick_index = SysTick->CTRL;
        unSystick_index &= ~((uint32_t)0xFFFFFFFE);
        /* disable systick */
        SysTick->CTRL &= (uint32_t)0xFFFFFFFE;

        HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStopMode, HAL_PWR_Regulator_LowPower, NULL, 0, HAL_PWR_STOPEntry_WFI);
        HalDrvPower_SYSCLKConfig_STOP();

        /* restore systick register configuration */
        SysTick->CTRL |= unSystick_index;

        NVIC_SystemReset();
        while(1)
        {
            MD_mDelay(10);
            NVIC_SystemReset();
        }
#endif
    }
    else
    {
        printf("!! Device Reset : GetRequestSystemReset %d, m_bDeviceReset %d\r\n", GetRequestSystemReset(), m_bDeviceReset);
        while(HalUartGetFlagStatus((int)g_stUart7.pUARTreg, HAL_USART_FLAG_TC)== HAL_RESET);
        MPU6515_CheckInterrupt();
	
        // this is process according to user command
        if( CheckDeviceResetSignal() == true )		SystemForcelyResetForPowerOn();
        else       					SystemForcelyReset();
    }
}

void SetRequestSystemReset(boolean_t bSystemReset)
{
    m_bRcvSystemResetCmd = bSystemReset;
}

boolean_t GetRequestSystemReset()
{
    return m_bRcvSystemResetCmd;
}

void SystemForcelyReset()
{
    g_unBkramSwResetSignal = SW_RESET_SIGNAL;
	HalDrvWrBkRam((unsigned char*)&g_unBkramSwResetSignal,BKRAM_SW_RESET_SIGNAL_ADDR,BKRAM_SW_RESET_SIGNAL_SIZE);

    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);

    SetWDTReset(1);

    while(1);
}

void SystemForcelyReset_PowerOn()
{
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);

    SetWDTReset(1);

    while(1);
}


void SystemForcelyResetForPowerOn()
{
    g_unBkramSwResetSignal = 0;
	HalDrvWrBkRam((unsigned char*)&g_unBkramSwResetSignal,BKRAM_SW_RESET_SIGNAL_ADDR,BKRAM_SW_RESET_SIGNAL_SIZE);

    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);
    HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);
    SetWDTReset(1);
    while(1);
}
void SetGyroInitializeAngle(stSensorInfo* pstGyroAngle)
{
    SetAutolinkConfigProperty(eAutoLinkConfig_SensorInfo,(void*)pstGyroAngle);
}

void GetGyroInitializeAngle(stSensorInfo* pstGyroAngle)
{
    GetAutolinkConfigProperty(eAutoLinkConfig_SensorInfo,(void*)pstGyroAngle);
}

#ifndef ENABLE_STANDBY_MODE	
boolean_t LoadSystemBackupRam()
{
    uint16_t usBkCheckSum = BkSram_SystemInfo.checksum;
	BkSram_SystemInfo.checksum = 0;
    uint8_t usCalCheckSum = (uint8_t)CalCheckSum((uint8_t*)&BkSram_SystemInfo,sizeof(BkSram_SystemInfo));

    if( usBkCheckSum != usCalCheckSum )
    {
    	Trace(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    	Trace(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    	Trace(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		Trace(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		Trace(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		Trace("SYSTEM CHECKSUM ERROR\n");
		Trace("BK CheckSum : %d, Cal CheckSum : %d\n",usBkCheckSum, usCalCheckSum);
		Trace(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

    	BkSram_SystemInfo.checksum = usCalCheckSum;
		return false;
    }
	return true;
}

void UpdateSystemBackupRam(boolean_t bForceReset)
{
  	Trace("upate system backup ram / forcereset : %d\r\n", bForceReset);
	BkSram_SystemInfo.ucForceReset = bForceReset;
	BkSram_SystemInfo.checksum = 0;
	BkSram_SystemInfo.checksum = CalCheckSum((uint8_t*)&BkSram_SystemInfo,sizeof(BkSram_SystemInfo));
}

void UpdateBackupRam(boolean_t bForceReset)
{
	UpdateSystemBackupRam(bForceReset);
}
#endif

