/* Includes ------------------------------------------------------------------*/
#include "GIT_Util.h"
#include "Modem_comm.h"
#include "Message_Manager.h"
#include "Modem_Manager.h"
#include "FOTA_Manager.h"
#include "OBD_Controller.h"
#include "Message_Make.h"
#include "ff.h"
#include "HalLedDriver.h"

#include "AutolinkConfiguration.h"
#include "AutolinkMessage.h"

#include "MngModem.h"
#include "MngSystem.h"
#include "MngQueue.h"
#include "HdDebug.h"
#include "GIT_OemInterface.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_MODEM,__VA_ARGS__)

extern stServerUrl g_stServerUrl;
extern unsigned char g_SavedGUID[MAX_GUID_LENGTH+1];
extern unsigned int g_uiAGPSFilesize;

extern void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern void SetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern void SetRemoteControlStatus(boolean_t bRemoteControlStatus);;
extern uint32_t GetUTCTime(); //mod.kks 21.11.05 to warnning check

extern bool ApplyEncryption3(uint8_t *arrPlainText,uint16_t nPlainTextLength, uint8_t *arrEncryptionText, uint16_t *nEncryptionTextLen, DukptPinEntry* pstEncryptKeyEntry);

void SetLastUsedDukptPinEntry(DukptPinEntry* pstEncryptKeyEntry);

extern boolean_t MakeAutoLinkPacket(eMESSAGE_TYPE eMode, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm);
extern void SetModemProcessStatus(int32_t status);
extern boolean_t ParseRemoteMessage(uint32_t* punOccurredEventTime);
extern unsigned long m_ulOldStartSendReportTime;
extern int GetDUKPTPinEntry(DukptPinEntry* pstEntry);

void ModemReportHandler(int32_t iEventType, stMsgMdm* pstMsgMdm);
void ModemFotaHandler(int32_t iEventType, stMsgMdm* pstMsgMdm);

extern void ClearModemResetRetryCount();
extern int8_t GetModemResetRetryCount();
extern void SetLastSendMessage(stMsgMdm* pstMsgMdm);

void HandlerRemoteCommand();
void HandlerModemReset();
void RequestNetworkTime();
void RequestRssi();
extern void ParseSetTrackingMessage(stMsgSysMsg* pstMessage);

extern int32_t GetModemStatus();
extern void HandlerRequestSendResult();
extern bool ParseGeoFenceMessage(stCarReport* pstGeoFenceSetting);
extern void SetModemNotWorkingFlag(boolean_t bModemNotWorkingFlag);
extern void ShowSmartkeyAction(stCarReport report);
extern bool SystemDelayProcess(unsigned long* nBaseTime, int nDelay);
extern boolean_t SetRequestActionFromManager(int32_t nRequestActionFromManager,boolean_t bForce);

extern bool ParseModemActivateMessage(stMsgSysMsg* pstMessage);

extern uint16_t ConversionMultiByteToHexStr_n4(uint8_t *ptrBuffer,uint16_t *pnPos, uint8_t *pMultiByte, uint16_t n);
extern void HandlerModemState();
extern bool ParseSettingInfoMessage();
extern void GetIpekProperties(stReportIpek* pstIpek);
extern bool ParseIpekPhase1Message(stReportIpek* pstIpekProperties);
extern void SetIpekProperties(stReportIpek* pstIpek);
extern bool ParseIpekPhase2Message(stReportIpek* pstIpekProperties);
extern bool ParsePolygonGeoFenceMessage(stCarReport* pstPolygonGeoFenceSetting);

extern bool ParseRsvEngCtrlMessage(stCarReport* pstRsvEngCtrl);

bool GetActiveRemoteControl(unsigned char* ucStrItem, unsigned char unLength);


void SetLastRemoteControlMessage(stMsgMdm* pstMsgMdm);

#ifdef ENABLE_FOTA
void ModemFotaHandler(int32_t iEventType, stMsgMdm* pstMsgMdm)
{
    if( iEventType == eFwVehicleInfo )
    {
        ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_VEHICLE_INFO;
        SetLastSendMessage(pstMsgMdm);
    }
    else if( iEventType == eFwInfo )
    {
        ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_VERSION;
        SetLastSendMessage(pstMsgMdm);
    }
    else if( iEventType == eFwBin )
    {
        ModemManagerData.eFOTAStartState = eFOTA_START_STATE_GET_BIN;
        SetLastSendMessage(pstMsgMdm);
    }
    else
    {
        GIT_Assert(false,eErrorCodeMdm|eFotaEventUnknown);
    }
}
#endif

void ModemAgpsHandler(int32_t iEventType, stMsgMdm* pstMsgMdm)
{
    uint8_t *ptrHTTPBuffer;

    SetLastSendMessage(pstMsgMdm);

    // display message
    DisplayReport("[ModemAgpsHandler]Send report to server",(stMsgSysMsg*)pstMsgMdm);
    // save start time
    m_ulOldStartSendReportTime = OemGetTmr();

    // common attribute.
    ModemManagerData.nInternetConnectionProfileId = MODEM_INTERNET_CONNECTION_ID_HTTP_COMM;
    ModemManagerData.nInternetServiceProfileId = MODEM_INTERNET_SERVICE_ID_HTTP_COMM;

    // set http url
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, HTTP_AGPS_MESSAGE_URL);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);

    // set global http buffer
    ptrHTTPBuffer = gaModemCommTxDataBuffer;
    memset(gaModemCommTxDataBuffer,0,sizeof(gaModemCommTxDataBuffer));

    ModemManagerData.eCurrentMessageSendingType = eMESSAGE_APGS_DATA;

    // set auto link request page
    //sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
    //    ,"%s\x00", "GetOfflineData.ashx?token=_e_O3Zx1e0y8kwzFuy-Inw;gnss=gps;period=1;resolution=1;days=1;" );
    sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
        ,"%s\x00",HTTP_AGPS_MESSAGE_PATH);

    // set buffer for sending data to server
    ModemManagerData.paModemCommDataTxBuffer = (uint8_t *)ptrHTTPBuffer;
    //ModemManagerData.nWriteDataLength = strlen((char *)ptrHTTPBuffer);
    //ModemManagerData.nWriteDataLength = strlen(ptrHTTPBuffer);
    ModemManagerData.nWriteDataLength = 0;

    Trace("#############################################################\r\n");
    Trace("Packet Size : %d\r\n",ModemManagerData.nWriteDataLength);

    ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;
    ModemManagerData.bRequestAgpsCommFlag = true;
    // false이면, modem에서 수신 받은 데이터를 internal flash에 저장하지 않는다.
    ModemManagerData.bModemDataSaveToFlashFlag = false;
    // 서버로 부터 수신 받는 데이터 타입은 ascii 문자열이다.
    geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;

    // clear network service
    BkSram_ModemInfo.bHTTPSuccessServiceConnection[ModemManagerData.nInternetServiceProfileId] = false;

	g_uiAGPSFilesize = 0;
}

void ModemIpekHandler(int32_t iEventType, stMsgMdm* pstMsgMdm)
{
    uint8_t arrTempBuffer[1024+512]={0,};
    uint8_t arrEncryptionText[1024+512]={0,};
    uint16_t totalLength = 0;
    uint16_t nDataLength;
    uint16_t nLen = 0;

    uint8_t *ptrHTTPBuffer;

    SetLastSendMessage(pstMsgMdm);

    // display message
    DisplayReport("[ModemIpekHandler]Send report to server",(stMsgSysMsg*)pstMsgMdm);
    // save start time
    m_ulOldStartSendReportTime = OemGetTmr();

    // common attribute.
    ModemManagerData.nInternetConnectionProfileId = MODEM_INTERNET_CONNECTION_ID_HTTP_COMM;
    ModemManagerData.nInternetServiceProfileId = MODEM_INTERNET_SERVICE_ID_HTTP_COMM;

    // set http url
    //strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, HTTP_RETAIL_MESSAGE_URL);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);
    strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, g_stServerUrl.Url);

    // set global http buffer
    ptrHTTPBuffer = gaModemCommTxDataBuffer;
    memset(gaModemCommTxDataBuffer,0,sizeof(gaModemCommTxDataBuffer));

    switch(iEventType)
    {
        case eIpekPhase1:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_IPEK_PHASE1;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_IPEK_PHASE1,arrTempBuffer,pstMsgMdm);

            break;
        case eIpekPhase2:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_IPEK_PHASE2;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_IPEK_PHASE2,arrTempBuffer,pstMsgMdm);

            break;
    }

    // set auto link request page
    //sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
    //    ,"%s%d\x00", HTTP_RETAIL_MESSAGE_PATH, ModemManagerData.eCurrentMessageSendingType);
    sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
        ,"%s%d\x00", g_stServerUrl.Path, ModemManagerData.eCurrentMessageSendingType);

    ApplyEncryption(arrTempBuffer, arrEncryptionText, &nLen, totalLength);

	// set auto link packet in http buffer
#if defined(QA_FIFA)
    sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','data':['%s']}\x00", GetServerProtocolVersion(), arrEncryptionText);
    nDataLength = strlen((char const *)ptrHTTPBuffer);
#else
	if(g_FirmwareInfo.arrSerialNumber[IDX_MANUFACTURER_CODE]=='K')
    {
	    sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','data':['%s']}\x00", GetServerProtocolVersion(), arrEncryptionText);
	    nDataLength = strlen((char const *)ptrHTTPBuffer);
	}
	else
	{
	    sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','data':'%s'}\x00", GetServerProtocolVersion(), arrEncryptionText);
	    nDataLength = strlen((char const *)ptrHTTPBuffer);
	}
#endif
    // set buffer for sending data to server
    ModemManagerData.paModemCommDataTxBuffer = (uint8_t *)ptrHTTPBuffer;
    //ModemManagerData.nWriteDataLength = strlen((char *)ptrHTTPBuffer);
    ModemManagerData.nWriteDataLength = nDataLength;

    Trace("#############################################################\n");
    Trace("Packet Size : %d\r\n",ModemManagerData.nWriteDataLength);

    ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;
    ModemManagerData.bRequestMessageCommFlag = true;
    // false이면, modem에서 수신 받은 데이터를 internal flash에 저장하지 않는다.
    ModemManagerData.bModemDataSaveToFlashFlag = false;
    // 서버로 부터 수신 받는 데이터 타입은 ascii 문자열이다.
    geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;
}

void ModemReportHandler(int32_t iEventType, stMsgMdm* pstMsgMdm)
{
    // damo // aes 128
    uint8_t arrType[32]={0,};
    uint16_t totalLength;
    uint16_t totalLength2;
    uint8_t arrTempBuffer[2048]={0,};
    uint8_t arrEncryptType[64]={0,}; //암호화
    uint8_t arrEncryptionText[1024*3]={0,};
    uint8_t arrEncryptionText2[1024+512]={0,};
    static uint8_t arrTempPeriodMessageBuffer[1024]={0,};
    stMsgMdm tmpMsgMdm;

    uint8_t *ptrHTTPBuffer;
    uint16_t nLen;
    uint16_t nLen2;
    uint16_t nLen3;

    uint16_t nDataLength = 0;

    boolean_t bSendOddMessage;

    DukptPinEntry stEncryptKeyEntry;
    uint8_t arrEncryptionKSN[64]={0,};
    uint8_t arrEncryptionKSNStr[64]={0,};
    uint16_t nKsnLen;

    DCSEncryptType nEncryptType;
    DCSServiceType nServiceType;
    // get service type fleet / retail
    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);
    // get service type fleet / retail
    GetAutolinkConfigProperty(eAutoLinkConfig_EncryptType,(void*)&nEncryptType);

    Trace("Transfer Service Type : %x, Encrypt Type : %x\r\n",nServiceType,nEncryptType);

    if( nEncryptType == DCS_ENC_AES )
    {
        memcpy(arrType,"AES",3);
    }
    else if( nEncryptType == DCS_ENC_KMS )
    {
        memcpy(arrType,"DAMO_BASE64",11);
    }
    else
    {
        //nEncryptType = DCS_ENC_KMS;
        memcpy(arrType,"DAMO_BASE64",11);
    }

    SetLastSendMessage(pstMsgMdm);

    // display message
    DisplayReport("[ModemReportHandler]Send report to server",(stMsgSysMsg*)pstMsgMdm);

    // save start time
    m_ulOldStartSendReportTime = OemGetTmr();

    // common attribute.
    ModemManagerData.nInternetConnectionProfileId = MODEM_INTERNET_CONNECTION_ID_HTTP_COMM;
    ModemManagerData.nInternetServiceProfileId = MODEM_INTERNET_SERVICE_ID_HTTP_COMM;

    // set global http buffer
    ptrHTTPBuffer = gaModemCommTxDataBuffer;
    memset(gaModemCommTxDataBuffer,0,sizeof(gaModemCommTxDataBuffer));

    if( nEncryptType == DCS_ENC_KMS )
    {
        // get a new encrypt key from kms
        if( GetDUKPTPinEntry(&stEncryptKeyEntry) != 0 )
        {
//#warning "how to handler if key is not available"
            Trace("error : encrypt key is not set.\r\n");
        }

        // save last used dukpt entry for decrypt
        SetLastUsedDukptPinEntry(&stEncryptKeyEntry);

#ifdef ENABL_KMS_LOG
        Trace("ksn : \r\n");
        hexdump(stEncryptKeyEntry.ksn,sizeof(stEncryptKeyEntry.ksn));
        Trace("enc key : \r\n");
        hexdump(stEncryptKeyEntry.req_enc_key,sizeof(stEncryptKeyEntry.req_enc_key));
#endif //#ifdef ENABL_KMS_LOG
    }

    switch(iEventType)
    {
        case eR_BeforeDriving:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_ALARM_EVENT;
            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_ALARM_EVENT,arrTempBuffer,pstMsgMdm);
            break;
        case eR_DrivingInterval:
            memcpy((char*)&tmpMsgMdm,(char*)pstMsgMdm,sizeof(stMsgMdm));
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_PERIOD_INFORMATION;

            // make auto link packet BODY1
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_PERIOD_INFORMATION,arrTempBuffer,&tmpMsgMdm);
			//strcpy((char *)arrPreviousPeriodMessageBuffer, (char *)arrTempBuffer);

            if( pstMsgMdm->carReport.rpInterval.DrivingInfo.DrivingInfoB2.OccurredEventTime != 0 )
            {
                // change body1 with body2
                memcpy((char*)&tmpMsgMdm.carReport.rpInterval.DrivingInfo.DrivingInfoB1,
                       (char*)&pstMsgMdm->carReport.rpInterval.DrivingInfo.DrivingInfoB2,
                       sizeof(stReportDrivingInfoBody));

                // make auto link packet BODY2
                totalLength2 = MakeAutoLinkPacket(eMESSAGE_TYPE_PERIOD_INFORMATION,arrTempPeriodMessageBuffer,&tmpMsgMdm);

                if( nEncryptType == DCS_ENC_AES )
                {
                    ApplyEncryption(arrTempPeriodMessageBuffer, arrEncryptionText2, &nLen2, totalLength2);
                }
                else if( nEncryptType == DCS_ENC_KMS )
                {
                    nLen2 = 0;
                    // encrypt data with kms
                    ApplyEncryption3(arrTempPeriodMessageBuffer,totalLength2, arrEncryptionText2, &nLen2, &stEncryptKeyEntry);

                    //totalLength2 = 0;
                    //memset(arrTempPeriodMessageBuffer,0,sizeof(arrTempPeriodMessageBuffer));
                    //ConversionMultiByteToHexStr_n4(arrTempPeriodMessageBuffer,&totalLength2,arrEncryptionText2,nLen2);
                    //memset(arrEncryptionText2,0,sizeof(arrEncryptionText2));
                    //memcpy(arrEncryptionText2,arrTempPeriodMessageBuffer,nLen2);
                }
                else
                {
                    nLen2 = 0;
                    // encrypt data with kms
                    ApplyEncryption3(arrTempPeriodMessageBuffer,totalLength2, arrEncryptionText2, &nLen2, &stEncryptKeyEntry);
                }

                bSendOddMessage = false;
            }
            else
            {
                bSendOddMessage = true;
            }

            break;
        case eR_AfterDriving:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_TRIP_REPORTING;
            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_TRIP_REPORTING,arrTempBuffer,pstMsgMdm);
            break;
        case eR_ParkingInterval:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_PERIOD_INFORMATION_SLEEP;
            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_PERIOD_INFORMATION_SLEEP,arrTempBuffer,pstMsgMdm);
            break;
        case eR_ReqSmartKey:
            SetRemoteControlStatus(true);

            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST; // 원격 제어 요청 = 41
            // -->server에서 받앗으면 41, 내부에서 쓸때는 40 (구분을 주기 위해서)

            ModemManagerData.bCheckNetworkStatusFlag = false;
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

            //HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly, MDM_CHECK_NETWORK_STATUS_DLY, eSWTimer_ONESHOT, MDM_SetNetworkRegistrationFlag_CallBack, true);

            //MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_PROCESS;
            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            //pstMsgMdm->header.unEventTime = pstMsgMdm->carReport.rpSmartKey.Request.OccurredEventTime;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST,arrTempBuffer,pstMsgMdm);
            // set last remote control message for sms time check
            SetLastRemoteControlMessage(pstMsgMdm);

            break;
        case eR_RspSmartKey:
            SetRemoteControlStatus(false);

            Trace("BT Remote control Result to server!!!\r\n");

			if( pstMsgMdm->carReport.rpSmartKey.Response.ucSysSmartkeyReqType == eSysSmartkeyReqType_Modem )
			{
				ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_REMOTE_CONTROL_REPORT; //원격 제어 결과 보고 = 40
				totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_REMOTE_CONTROL_REPORT,arrTempBuffer,pstMsgMdm);
			}
			else if(pstMsgMdm->carReport.rpSmartKey.Response.ucSysSmartkeyReqType == eSysSmartkeyReqType_BT)
			{
				// remote control finished
				ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_BT_REMOTE_CONTROL_REPORT; // 원격 제어 결과 보고 = 44
				totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_BT_REMOTE_CONTROL_REPORT,arrTempBuffer,pstMsgMdm);
			}
			else if(pstMsgMdm->carReport.rpSmartKey.Response.ucSysSmartkeyReqType == eSysSmartkeyReqType_Sys)
			{
				ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_RESULT; // 예약시동 제어 결과 보고 = 95
				totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_RESULT,arrTempBuffer,pstMsgMdm);
			}

            ModemManagerData.bCheckNetworkStatusFlag = false;
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            //totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_REMOTE_CONTROL_REPORT,arrTempBuffer,pstMsgMdm);
            break;
        case eR_CurrentVehicleStatus:

            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_CURRENT_VEHICLE_STATUS;
            ModemManagerData.bCheckNetworkStatusFlag = false;
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_CURRENT_VEHICLE_STATUS,arrTempBuffer,pstMsgMdm);
            break;
        case eR_AlramMasking:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_ALARM_MASKING;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_ALARM_MASKING,arrTempBuffer,pstMsgMdm);
            break;
        case eR_Alram:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_ALARM_EVENT;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_ALARM_EVENT,arrTempBuffer,pstMsgMdm);
            break;
        case eR_AlramDTC:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_DTC_ERROR;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_DTC_ERROR,arrTempBuffer,pstMsgMdm);
            break;
        case eR_SettingGeofence:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_SETTING_GEOFENCE;
            ModemManagerData.bCheckNetworkStatusFlag = false;
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_SETTING_GEOFENCE,arrTempBuffer,pstMsgMdm);
            break;
        case eR_SettingPolygonGeofence:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_SETTING_POLYGON_GEOFENCE;
            ModemManagerData.bCheckNetworkStatusFlag = false;
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_SETTING_POLYGON_GEOFENCE,arrTempBuffer,pstMsgMdm);
            break;
        case eR_SmsTest:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_SMS_TEST;
            ModemManagerData.bCheckNetworkStatusFlag = false;
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_SMS_TEST,arrTempBuffer,pstMsgMdm);
            break;
        case eR_SettingInfo:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_ENGINE_START;
            ModemManagerData.bCheckNetworkStatusFlag = false;
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_ENGINE_START,arrTempBuffer,pstMsgMdm);
            break;
        case eR_WriteTest:
            memcpy((char*)&tmpMsgMdm,(char*)pstMsgMdm,sizeof(stMsgMdm));
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_PERIOD_INFORMATION;

            nLen2 = 0;

            // make auto link packet BODY1
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_PERIOD_INFORMATION,arrTempBuffer,&tmpMsgMdm);
			//strcpy((char *)arrPreviousPeriodMessageBuffer, (char *)arrTempBuffer);

            if( pstMsgMdm->carReport.rpInterval.DrivingInfo.DrivingInfoB2.OccurredEventTime != 0 )
            {
                // change body1 with body2
                memcpy((char*)&tmpMsgMdm.carReport.rpInterval.DrivingInfo.DrivingInfoB1,
                       (char*)&pstMsgMdm->carReport.rpInterval.DrivingInfo.DrivingInfoB2,
                       sizeof(stReportDrivingInfoBody));

                // make auto link packet BODY2
                totalLength2 = MakeAutoLinkPacket(eMESSAGE_TYPE_PERIOD_INFORMATION,arrTempPeriodMessageBuffer,&tmpMsgMdm);

                if( nEncryptType == DCS_ENC_AES )
                {
                    ApplyEncryption(arrTempPeriodMessageBuffer, arrEncryptionText2, &nLen2, totalLength2);
                }
                else if( nEncryptType == DCS_ENC_KMS )
                {
                    nLen2 = 0;
                    // encrypt data with kms
                    ApplyEncryption3(arrTempPeriodMessageBuffer,totalLength2, arrEncryptionText2, &nLen2, &stEncryptKeyEntry);

                    //totalLength2 = 0;
                    //memset(arrTempPeriodMessageBuffer,0,sizeof(arrTempPeriodMessageBuffer));
                    //ConversionMultiByteToHexStr_n4(arrTempPeriodMessageBuffer,&totalLength2,arrEncryptionText2,nLen2);
                    //memset(arrEncryptionText2,0,sizeof(arrEncryptionText2));
                    //memcpy(arrEncryptionText2,arrTempPeriodMessageBuffer,nLen2);
                }
                bSendOddMessage = false;


            }
            else
            {
                bSendOddMessage = true;
            }

            memset(&arrEncryptionText2[nLen2],0xcc,nLen2*2);
            nLen2*=3;

            break;
        case eR_ReqModemActivate:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_REQUEST_MODEM_ACTIVATE;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            //MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_PROCESS;
            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_REQUEST_MODEM_ACTIVATE,arrTempBuffer,pstMsgMdm);
            // set last remote control message for sms time check
            SetLastRemoteControlMessage(pstMsgMdm);
            break;
        case eR_RspModemActivate:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_RESPONSE_MODEM_ACTIVATE;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            //MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_PROCESS;
            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_RESPONSE_MODEM_ACTIVATE,arrTempBuffer,pstMsgMdm);
            break;
#if defined(PROTOCOL15)
        case eR_ReqSensorInitialize:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_REQUEST_SENSOR_INITIALIZE;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            //MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_PROCESS;
            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_REQUEST_SENSOR_INITIALIZE,arrTempBuffer,pstMsgMdm);
            break;
        case eR_RspSensorInitialize:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_RESPONSE_SENSOR_INITIALIZE;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            //MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_PROCESS;
            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_RESPONSE_SENSOR_INITIALIZE,arrTempBuffer,pstMsgMdm);
            break;
#endif
#if defined(PROTOCOL17)
        case eR_ReqSetURL:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_REQUEST_SETURL;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_REQUEST_SETURL,arrTempBuffer,pstMsgMdm);
			break;
		case eR_ResSetURL:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_RESPONSE_SETURL;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_RESPONSE_SETURL,arrTempBuffer,pstMsgMdm);
            break;
		case eR_ReqSetURLSave:
			ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_RESPONSE_SETURL_COMPLETE;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_RESPONSE_SETURL_COMPLETE,arrTempBuffer,pstMsgMdm);
			break;
		case eR_ReqSetURLInit:
			ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_RESPONSE_INITURL_COMPLETE;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_RESPONSE_INITURL_COMPLETE,arrTempBuffer,pstMsgMdm);
			break;
#endif
#if defined(PROTOCOL18)
		case eR_ReqSettingRsvEngCtrl:
			ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_REQUEST_RESERVATION_ENGINE_CONTROL_SETTING;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_REQUEST_RESERVATION_ENGINE_CONTROL_SETTING,arrTempBuffer,pstMsgMdm);
			break;
		case eR_RspSettingRsvEngCtrl:

			ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_SETTING;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_SETTING,arrTempBuffer,pstMsgMdm);
			break;
		case eR_Charging:
			ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_CHARGING_REPORT;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            // make auto link packet
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_CHARGING_REPORT,arrTempBuffer,pstMsgMdm);
			break;
#endif
		case eR_SetTrackingMode:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_TRACKING_REQUEST;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_TRACKING_REQUEST,arrTempBuffer,pstMsgMdm);
			break;
		case eR_SetTrackingModeRsp:
			ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_TRACKING_RESPONSE;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_TRACKING_RESPONSE,arrTempBuffer,pstMsgMdm);
			break;
		case eR_TrackingReport:
			ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_TRACKING_DATA;
            ModemManagerData.bCheckNetworkStatusFlag = false;

            MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;

            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_TRACKING_DATA,arrTempBuffer,pstMsgMdm);
			break;
#if defined(PROTOCOL24)
	 	case eR_ReportNetworkStatus:
			ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_MODEM_STATUS_REPORT;
            ModemManagerData.bCheckNetworkStatusFlag = false;
			totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_MODEM_STATUS_REPORT,arrTempBuffer,pstMsgMdm);
			break;
#endif	
#if defined(PROTOCOL25)
        case eR_ReportInstallationNetworkCheck:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_INSTALLATION_NETWORK_CHECK_TEST;
            ModemManagerData.bCheckNetworkStatusFlag = false;
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);
            
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_INSTALLATION_NETWORK_CHECK_TEST,arrTempBuffer,pstMsgMdm);
            break;
        case eR_ReportInstallationSMSCheck:
            ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_INSTALLATION_SMS_CHECK_TEST;
            ModemManagerData.bCheckNetworkStatusFlag = false;
            HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Check_Network_Status_Dly);
            
            totalLength = MakeAutoLinkPacket(eMESSAGE_TYPE_INSTALLATION_SMS_CHECK_TEST,arrTempBuffer,pstMsgMdm);
            break;
#endif	
        case eR_Max:
        case eR_None:
        default:
            GIT_Assert(false,eErrorCodeMdm|eEventUnknown);
        break;
    }

#if defined(PROTOCOL17)
	// set https url
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, g_stServerUrl.Url);
	strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);

	// set auto link request page
	if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST )
	{
	  sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
			,"%s%d\x00", g_stServerUrl.Path, eMESSAGE_TYPE_REMOTE_CONTROL_REPORT);
	}
	else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_RESPONSE_SETURL )
	{
	  sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
			,"%s%d\x00", g_stServerUrl.Path, eMESSAGE_TYPE_REQUEST_SETURL);
	}
	else
	{
		//sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
		//    ,"%s%d\x00", HTTP_RETAIL_MESSAGE_PATH, ModemManagerData.eCurrentMessageSendingType);
		  sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
			,"%s%d\x00", g_stServerUrl.Path, ModemManagerData.eCurrentMessageSendingType);
	}
#else
    // set service url
    if( nServiceType == DCS_Retail )
    {
        // set https url
        //strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, HTTP_RETAIL_MESSAGE_URL);
        strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, g_stServerUrl.Url);
        strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);

        // set auto link request page
        if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST )
        {
            //sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
            //    ,"%s%d\x00", HTTP_RETAIL_MESSAGE_PATH, eMESSAGE_TYPE_REMOTE_CONTROL_REPORT);
          sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
                ,"%s%d\x00", g_stServerUrl.Path, eMESSAGE_TYPE_REMOTE_CONTROL_REPORT);
        }
        else
        {
            //sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
            //    ,"%s%d\x00", HTTP_RETAIL_MESSAGE_PATH, ModemManagerData.eCurrentMessageSendingType);
              sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
                ,"%s%d\x00", g_stServerUrl.Path, ModemManagerData.eCurrentMessageSendingType);
        }

    }
    else if( nServiceType == DCS_Fleet )
    {
        // set https url
        strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, HTTP_FLEET_MESSAGE_URL);
        strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);

		if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST )
        {
            //sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
            //    ,"%s%d\x00", HTTP_RETAIL_MESSAGE_PATH, eMESSAGE_TYPE_REMOTE_CONTROL_REPORT);
			sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
                ,"%s%d\x00", HTTP_FLEET_MESSAGE_PATH, eMESSAGE_TYPE_REMOTE_CONTROL_REPORT);
        }
		else
		{
			// set auto link request page
			sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
				,"%s%d\x00", HTTP_FLEET_MESSAGE_PATH, ModemManagerData.eCurrentMessageSendingType);
		}
    }
    else
    {
        Trace("Not defined service type / set up retail service type\r\n");
        // set https url
        strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, g_stServerUrl.Url);
        strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);

        // set auto link request page
        //sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
        //    ,"%s%d\x00", HTTP_RETAIL_MESSAGE_PATH, ModemManagerData.eCurrentMessageSendingType);
        sprintf((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath \
            ,"%s%d\x00", g_stServerUrl.Path, ModemManagerData.eCurrentMessageSendingType);
    }
#endif

    if( nEncryptType == DCS_ENC_AES )
    {
        // encrypt auto link packet
        ApplyEncryption(arrTempBuffer, arrEncryptionText, &nLen, totalLength);
    }
    else if( nEncryptType == DCS_ENC_KMS )
    {
#ifdef ENABL_KMS_LOG
        Trace("Raw Data : \r\n");
        hexdump(arrTempBuffer,totalLength);
#endif //#ifdef ENABL_KMS_LOG

        nLen = 0;
        // encrypt data with kms
        ApplyEncryption3(arrTempBuffer,totalLength, arrEncryptionText, &nLen, &stEncryptKeyEntry);
        // encrypt ksn with basic key encrypt

        nLen3 = 0;
        ConversionMultiByteToHexStr_n4(arrEncryptionKSNStr,&nLen3,stEncryptKeyEntry.ksn,10);
        ApplyEncryption(arrEncryptionKSNStr,arrEncryptionKSN,&nKsnLen,nLen3);

        nLen3 = 0;
        ApplyEncryption(arrType,arrEncryptType,&nLen3,4);
    }
    else
    {
        nLen = 0;
        // encrypt data with kms
        ApplyEncryption3(arrTempBuffer,totalLength, arrEncryptionText, &nLen, &stEncryptKeyEntry);
        // encrypt ksn with basic key encrypt

        nLen3 = 0;
        ConversionMultiByteToHexStr_n4(arrEncryptionKSNStr,&nLen3,stEncryptKeyEntry.ksn,10);
        ApplyEncryption(arrEncryptionKSNStr,arrEncryptionKSN,&nKsnLen,nLen3);

        nLen3 = 0;
        ApplyEncryption(arrType,arrEncryptType,&nLen3,4);
    }

    if( iEventType == eR_DrivingInterval )
    {
        if( bSendOddMessage == false )
        {
            if( nEncryptType == DCS_ENC_AES )
            {
                // set auto link packet in http buffer
                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','data':['%s','%s']}\x00", GetServerProtocolVersion(),arrEncryptionText, arrEncryptionText2);
                nDataLength = strlen((char const *)ptrHTTPBuffer);
            }
            else if( nEncryptType == DCS_ENC_KMS )
            {
                //sprintf((char *)ptrHTTPBuffer, "{'type':'%s','ksn':'%s','data':['%s','%s']}\x00",arrEncryptType,arrEncryptionKSN, arrEncryptionText, arrEncryptionText2);
                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'%s','data':['",GetServerProtocolVersion(),arrEncryptType, arrEncryptionKSN);
                int p1Size = strlen((char const *)ptrHTTPBuffer);
                memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionText,nLen);
                memcpy(&ptrHTTPBuffer[p1Size+nLen],"','",3);
                nDataLength = p1Size+nLen+3;
                memcpy(&ptrHTTPBuffer[nDataLength],arrEncryptionText2,nLen2);
                nDataLength += nLen2;
                memcpy(&ptrHTTPBuffer[nDataLength],"']}",3);
                nDataLength = nDataLength+3;
            }
            else
            {
                //sprintf((char *)ptrHTTPBuffer, "{'type':'%s','ksn':'%s','data':['%s','%s']}\x00",arrEncryptType,arrEncryptionKSN, arrEncryptionText, arrEncryptionText2);
                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'%s','data':['",GetServerProtocolVersion(),arrEncryptType, arrEncryptionKSN);
                int p1Size = strlen((char const *)ptrHTTPBuffer);
                memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionText,nLen);
                memcpy(&ptrHTTPBuffer[p1Size+nLen],"','",3);
                nDataLength = p1Size+nLen+3;
                memcpy(&ptrHTTPBuffer[nDataLength],arrEncryptionText2,nLen2);
                nDataLength += nLen2;
                memcpy(&ptrHTTPBuffer[nDataLength],"']}",3);
                nDataLength = nDataLength+3;
            }
        }
        else
        {
#if defined(QA_FIFA)
            // set auto link odd packet in http buffer
            if( nEncryptType == DCS_ENC_AES )
            {
                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','data':['%s']}\x00", GetServerProtocolVersion(),arrEncryptionText);
                nDataLength = strlen((char const *)ptrHTTPBuffer);
            }
            else if( nEncryptType == DCS_ENC_KMS )
            {
                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'%s','data':['",GetServerProtocolVersion(),arrEncryptType, arrEncryptionKSN);
                int p1Size = strlen((char const *)ptrHTTPBuffer);
                memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionText,nLen);
                memcpy(&ptrHTTPBuffer[p1Size+nLen],"']}",3);
                nDataLength = p1Size+nLen+3;
            }
            else
            {
                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'%s','data':['",GetServerProtocolVersion(),arrEncryptType, arrEncryptionKSN);
                int p1Size = strlen((char const *)ptrHTTPBuffer);
                memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionText,nLen);
                memcpy(&ptrHTTPBuffer[p1Size+nLen],"']}",3);
                nDataLength = p1Size+nLen+3;
            }
#else
            if(g_FirmwareInfo.arrSerialNumber[IDX_MANUFACTURER_CODE]=='K')
        	{
				// set auto link odd packet in http buffer
	            if( nEncryptType == DCS_ENC_AES )
	            {
	                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','data':['%s']}\x00", GetServerProtocolVersion(),arrEncryptionText);
	                nDataLength = strlen((char const *)ptrHTTPBuffer);
	            }
	            else if( nEncryptType == DCS_ENC_KMS )
	            {
	                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'%s','data':['",GetServerProtocolVersion(),arrEncryptType, arrEncryptionKSN);
	                int p1Size = strlen((char const *)ptrHTTPBuffer);
	                memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionText,nLen);
	                memcpy(&ptrHTTPBuffer[p1Size+nLen],"']}",3);
	                nDataLength = p1Size+nLen+3;
	            }
	            else
	            {
	                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'%s','data':['",GetServerProtocolVersion(),arrEncryptType, arrEncryptionKSN);
	                int p1Size = strlen((char const *)ptrHTTPBuffer);
	                memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionText,nLen);
	                memcpy(&ptrHTTPBuffer[p1Size+nLen],"']}",3);
	                nDataLength = p1Size+nLen+3;
	            }
        	}
			else
			{
				// set auto link odd packet in http buffer
	            if( nEncryptType == DCS_ENC_AES )
	            {
	                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','data':'%s'}\x00", GetServerProtocolVersion(),arrEncryptionText);
	                nDataLength = strlen((char const *)ptrHTTPBuffer);
	            }
	            else if( nEncryptType == DCS_ENC_KMS )
	            {
	                //sprintf((char *)ptrHTTPBuffer, "{'type':'%s','ksn':'%s','data':'%s'}\x00",arrEncryptType, arrEncryptionKSN, arrEncryptionText);
	                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'%s','data':'",GetServerProtocolVersion(),arrEncryptType, arrEncryptionKSN);
	                int p1Size = strlen((char const *)ptrHTTPBuffer);
	                memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionText,nLen);
	                memcpy(&ptrHTTPBuffer[p1Size+nLen],"'}",2);
	                nDataLength = p1Size+nLen+2;
	            }
	            else
	            {
	                //sprintf((char *)ptrHTTPBuffer, "{'type':'%s','ksn':'%s','data':'%s'}\x00",arrEncryptType, arrEncryptionKSN, arrEncryptionText);
	                sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'%s','data':'",GetServerProtocolVersion(),arrEncryptType, arrEncryptionKSN);
	                int p1Size = strlen((char const *)ptrHTTPBuffer);
	                memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionText,nLen);
	                memcpy(&ptrHTTPBuffer[p1Size+nLen],"'}",2);
	                nDataLength = p1Size+nLen+2;
	            }
			}
#endif
        }
    }
    else
    {
        // set auto link packet in http buffer
        if( nEncryptType == DCS_ENC_AES )
        {
            sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','data':['%s']}\x00", GetServerProtocolVersion(),arrEncryptionText);
            nDataLength = strlen((char const *)ptrHTTPBuffer);
        }
        else if( nEncryptType == DCS_ENC_KMS )
        {
            //sprintf((char *)ptrHTTPBuffer, "{'type':'%s','ksn':'%s','data':'%s'}\x00",arrEncryptType, arrEncryptionKSN, arrEncryptionText);
            sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'",GetServerProtocolVersion(),arrEncryptType);
            int p1Size = strlen((char const *)ptrHTTPBuffer);
            memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionKSN,nKsnLen);
#if defined(QA_FIFA)
            memcpy(&ptrHTTPBuffer[p1Size+nKsnLen],"','data':['",11);
            memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+11],arrEncryptionText,nLen);
            memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+11+nLen],"']}",3);
            nDataLength = p1Size+nKsnLen+11+nLen+3;
#else
            if(g_FirmwareInfo.arrSerialNumber[IDX_MANUFACTURER_CODE]=='K')
            {
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen],"','data':['",11);
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+11],arrEncryptionText,nLen);
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+11+nLen],"']}",3);
                nDataLength = p1Size+nKsnLen+11+nLen+3;
            }
            else
            {
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen],"','data':'",10);
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+10],arrEncryptionText,nLen);
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+10+nLen],"'}",2);
                nDataLength = p1Size+nKsnLen+10+nLen+2;
            }
#endif
        }
        else
        {
            //sprintf((char *)ptrHTTPBuffer, "{'type':'%s','ksn':'%s','data':'%s'}\x00",arrEncryptType, arrEncryptionKSN, arrEncryptionText);
            sprintf((char *)ptrHTTPBuffer, "{'ver':'%s','type':'%s','ksn':'",GetServerProtocolVersion(),arrEncryptType);
            int p1Size = strlen((char const *)ptrHTTPBuffer);
            memcpy(&ptrHTTPBuffer[p1Size],arrEncryptionKSN,nKsnLen);
#if defined(QA_FIFA)
            memcpy(&ptrHTTPBuffer[p1Size+nKsnLen],"','data':['",11);
            memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+11],arrEncryptionText,nLen);
            memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+11+nLen],"']}",3);
            nDataLength = p1Size+nKsnLen+11+nLen+3;
#else
            if(g_FirmwareInfo.arrSerialNumber[IDX_MANUFACTURER_CODE]=='K')
            {
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen],"','data':['",11);
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+11],arrEncryptionText,nLen);
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+11+nLen],"']}",3);
                nDataLength = p1Size+nKsnLen+11+nLen+3;
            }
            else
            {
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen],"','data':'",10);
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+10],arrEncryptionText,nLen);
                memcpy(&ptrHTTPBuffer[p1Size+nKsnLen+10+nLen],"'}",2);
                nDataLength = p1Size+nKsnLen+10+nLen+2;
            }
#endif
        }
    }

    // set buffer for sending data to server
    ModemManagerData.paModemCommDataTxBuffer = (uint8_t *)ptrHTTPBuffer;
    //ModemManagerData.nWriteDataLength = strlen((char *)ptrHTTPBuffer);
    ModemManagerData.nWriteDataLength = nDataLength;

    Trace("#############################################################\r\n");
    Trace("Packet : Type : %d, Size : %d\r\n",ModemManagerData.eCurrentMessageSendingType,ModemManagerData.nWriteDataLength);
    Trace("Url : %s/%s\r\n",ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL,
          ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aPath);

    Trace("#############################################################\r\n");

#ifdef ENABL_KMS_LOG
    hexdump(ModemManagerData.paModemCommDataTxBuffer,ModemManagerData.nWriteDataLength);
#endif //#ifdef ENABL_KMS_LOG

    ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;
    ModemManagerData.bRequestMessageCommFlag = true;
    // false이면, modem에서 수신 받은 데이터를 internal flash에 저장하지 않는다.
    ModemManagerData.bModemDataSaveToFlashFlag = false;
    // 서버로 부터 수신 받는 데이터 타입은 ascii 문자열이다.
    geModemReceiveDataType = eMODEM_RECEIVE_DATA_TYPE_ASCII;

    GIT_Assert(ModemManagerData.nWriteDataLength<MAX_MODEM_COMM_BUFFER_LENGTH,eErrorCodeMdm|eWriteSizeError);
    GIT_Assert(ModemManagerData.nWriteDataLength<MAX_MODEM_COMM_BUFFER_LENGTH,ModemManagerData.nWriteDataLength);
    if( ModemManagerData.nWriteDataLength>MAX_MODEM_COMM_BUFFER_LENGTH )
    {
        ModemManagerData.eCurrentMessageSendingType = eMESSAGE_TYPE_NONE;
        ModemManagerData.nWriteDataLength = 0;
        ModemManagerData.nMaxReadDataLength = MAX_MODEM_READ_DATA_LEHGTH;
        ModemManagerData.bRequestMessageCommFlag = false;
        Trace("========================================================\r\n");
        Trace("Make Message Error\r\n");
    }
}

void ModemRcvProcess()
{
    // check is this remote command
    HandlerRemoteCommand();

    // check modem work arount
    // if modem reset count over 3, system will be sleep or reset
    // MONI 2018-03-01
    // block this process it cause error in week signal area
    // HandlerModemReset();

    // handler for controling the modem.
    HandlerModemState();
}

void PrepareReportMessageAccording2Command(stMsgSysMsg* pstMessage)
{
    pstMessage->header.id = eMngModem;
    pstMessage->header.event = eReqReport;
    pstMessage->header.unTraceMng = eMngModem;

    pstMessage->carReport.rpSmartKey.Request.ucSysSmartkeyReqType = eSysSmartkeyReqType_Modem;

    switch(stRemoteCommand.eCommandType)
    {
        // related with obd request
        case eREMOTE_CON_CMD_TYPE_STARTING:
        case eREMOTE_CON_CMD_TYPE_DOOR:
        case eREMOTE_CON_CMD_TYPE_EMERGENCY_LIGHT:
        case eREMOTE_CON_CMD_TYPE_HORN_EMERGENCY_LIGHT:
        case eREMOTE_CON_CMD_TYPE_RESET:
        case eREMOTE_CON_CMD_TYPE_DTC_STATUS:
        {
            Trace("Remote Contro Message\r\n");
            pstMessage->header.subEvent = eR_ReqSmartKey;

            pstMessage->carReport.rpSmartKey.Request.CommandType = stRemoteCommand.eCommandType;
            memcpy((char*)pstMessage->carReport.rpSmartKey.Request.Guid,(char*)&stRemoteCommand.guid,MAX_GUID_LENGTH);
            pstMessage->carReport.rpSmartKey.Request.ControlType = stRemoteCommand.bControlType;
            pstMessage->carReport.rpSmartKey.Request.KeepPowerOnTime = stRemoteCommand.bStartUpTime;
            pstMessage->carReport.rpSmartKey.Request.Temperature = stRemoteCommand.nTemperature;
			pstMessage->carReport.rpSmartKey.Request.CheckTemperature = stRemoteCommand.unCheckTemperature;
			pstMessage->carReport.rpSmartKey.Request.Defrost = stRemoteCommand.bRemoveFrost;
#if defined(PROTOCOL13)
            pstMessage->carReport.rpSmartKey.Request.RearDefogger = stRemoteCommand.bRearDefogger;

			if( stRemoteCommand.bRearDefogger == true )
			{
				pstMessage->carReport.rpSmartKey.Request.RearDefogger = GetActiveRemoteControl("REARDEFOG",strlen("REARDEFOG"));
			}

			if( stRemoteCommand.bRemoveFrost == true)
			{
				pstMessage->carReport.rpSmartKey.Request.Defrost = GetActiveRemoteControl("AIRCON_VENT",strlen("AIRCON_VENT"));
			}	
            pstMessage->carReport.rpSmartKey.Request.HighBeam = stRemoteCommand.bHighBeam;
#endif //#if defined(PROTOCOL13)
            pstMessage->carReport.rpSmartKey.Request.Latitude = stRemoteCommand.fLatitude;
            pstMessage->carReport.rpSmartKey.Request.Longitude = stRemoteCommand.fLongitude;
            pstMessage->carReport.rpSmartKey.Request.BoundType = stRemoteCommand.eBoundType;
            pstMessage->carReport.rpSmartKey.Request.Distance = stRemoteCommand.unDistance;
            pstMessage->carReport.rpSmartKey.Request.OccurredEventTime = stRemoteCommand.unOccurredDateTime;

#if defined(PROTOCOL18)
			memcpy((char*)pstMessage->carReport.rpSmartKey.Request.BTControlKey,(char*)&stRemoteCommand.BTControlKey, MAX_BT_CONTROLKEY_LENGTH);
#endif
            ShowSmartkeyAction(pstMessage->carReport);
        }
        break;

        // related with setting
        case eREMOTE_CON_CMD_TYPE_SENSOR_SENSITIVITY:
            Trace("UserActionSetting : Sensitivity\r\n");
            pstMessage->header.subEvent = eR_ReqUserActionSetting;
            pstMessage->header.unEventTime = stRemoteCommand.unOccurredDateTime;

            pstMessage->carReport.rpSetting.UserSetting.ucCommandType = stRemoteCommand.eCommandType;
            memcpy((char*)pstMessage->carReport.rpSetting.UserSetting.RequestGUID,
                   (char*)stRemoteCommand.guid,MAX_GUID_LENGTH);

            // High : 1 / Middle : 2 / Low : 3
            if( stRemoteCommand.bControlType == 1 )
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Guard.stImpulseSetting.ucHigh = 20;
            else if( stRemoteCommand.bControlType == 2 )
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Guard.stImpulseSetting.ucMiddle = 50;
            else if( stRemoteCommand.bControlType == 3 )
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Guard.stImpulseSetting.ucLow = 70;

            //set active level
            pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Guard.stImpulseSetting.ucActiveLevel = stRemoteCommand.bControlType;

            break;
        case eREMOTE_CON_CMD_TYPE_VALET:
            Trace("UserActionSetting : Valet\r\n");
            pstMessage->header.subEvent = eR_ReqUserActionSetting;
            pstMessage->header.unEventTime = stRemoteCommand.unOccurredDateTime;

            pstMessage->carReport.rpSetting.UserSetting.ucCommandType = stRemoteCommand.eCommandType;
            memcpy((char*)pstMessage->carReport.rpSetting.UserSetting.RequestGUID,
                   (char*)stRemoteCommand.guid,MAX_GUID_LENGTH);

            if( stRemoteCommand.bControlType == 1 )
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Valet.bActive = true;
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Valet.nDistance = 500;
            }
            else
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Valet.bActive = false;
            }
            break;
        case eREMOTE_CON_CMD_TYPE_TOWING:
            Trace("UserActionSetting : Towing\r\n");
            pstMessage->header.subEvent = eR_ReqUserActionSetting;
            pstMessage->header.unEventTime = stRemoteCommand.unOccurredDateTime;

            pstMessage->carReport.rpSetting.UserSetting.ucCommandType = stRemoteCommand.eCommandType;
            memcpy((char*)pstMessage->carReport.rpSetting.UserSetting.RequestGUID,
                   (char*)stRemoteCommand.guid,MAX_GUID_LENGTH);

            if( stRemoteCommand.bControlType == 1 )
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Towing.bActive = true;
            }
            else
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Towing.bActive = false;
            }
            break;
        case eREMOTE_CON_CMD_TYPE_GUARD:
            Trace("UserActionSetting : Guard\r\n");
            pstMessage->header.subEvent = eR_ReqUserActionSetting;
            pstMessage->header.unEventTime = stRemoteCommand.unOccurredDateTime;

            pstMessage->carReport.rpSetting.UserSetting.ucCommandType = stRemoteCommand.eCommandType;
            memcpy((char*)pstMessage->carReport.rpSetting.UserSetting.RequestGUID,
                   (char*)stRemoteCommand.guid,MAX_GUID_LENGTH);

            if( stRemoteCommand.bControlType == 1 )
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Guard.bActive = true;
            }
            else
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.Guard.bActive = false;
            }
            break;
        case eREMOTE_CON_CMD_TYPE_DATA:
            Trace("UserActionSetting : ActiveModem\r\n");
            pstMessage->header.subEvent = eR_ReqUserActionSetting;
            pstMessage->header.unEventTime = stRemoteCommand.unOccurredDateTime;

            pstMessage->carReport.rpSetting.UserSetting.ucCommandType = stRemoteCommand.eCommandType;
            memcpy((char*)pstMessage->carReport.rpSetting.UserSetting.RequestGUID,
                   (char*)stRemoteCommand.guid,MAX_GUID_LENGTH);

            if( stRemoteCommand.bControlType == 1 )
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.ActiveModem.bActive = true;
            }
            else
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.ActiveModem.bActive = false;
            }
            break;
        // this is fota request
        case eREMOTE_CON_CMD_TYPE_FOTA:
            Trace("Remote Contro Message\r\n");
            pstMessage->header.event = eReqFota;

            if( stRemoteCommand.bControlType == eREMOTE_CON_FOTA_TYPE_UPDATE )
                pstMessage->header.subEvent = eFwUpdate;
            else if( stRemoteCommand.bControlType == eREMOTE_CON_FOTA_TYPE_TEST )
                pstMessage->header.subEvent = eFwTestUpdate;

            pstMessage->carReport.rpSmartKey.Request.CommandType = stRemoteCommand.eCommandType;
            memcpy((char*)pstMessage->carReport.rpSmartKey.Request.Guid,(char*)&stRemoteCommand.guid,MAX_GUID_LENGTH);
            pstMessage->carReport.rpSmartKey.Request.ControlType = stRemoteCommand.bControlType;
            pstMessage->carReport.rpSmartKey.Request.KeepPowerOnTime = stRemoteCommand.bStartUpTime;
            pstMessage->carReport.rpSmartKey.Request.OccurredEventTime = stRemoteCommand.unOccurredDateTime;

            ShowSmartkeyAction(pstMessage->carReport);
            break;
        case eREMOTE_CON_CMD_TYPE_SERVICE_TYPE:
            Trace("UserActionSetting : ServiceType\r\n");
            pstMessage->header.subEvent = eR_ReqUserActionSetting;
            pstMessage->header.unEventTime = stRemoteCommand.unOccurredDateTime;

            pstMessage->carReport.rpSetting.UserSetting.ucCommandType = stRemoteCommand.eCommandType;
            memcpy((char*)pstMessage->carReport.rpSetting.UserSetting.RequestGUID,
                               (char*)stRemoteCommand.guid,MAX_GUID_LENGTH);

            if( stRemoteCommand.bControlType == 1 )
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.ServiceType.unServiceType = DCS_Retail;
            }
            if( stRemoteCommand.bControlType == 2 )
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.ServiceType.unServiceType = DCS_Fleet;
            }
            break;
        case eREMOTE_CON_CMD_TYPE_ENCRYPT_TYPE:
            Trace("UserActionSetting : EncryptType\r\n");
            pstMessage->header.subEvent = eR_ReqUserActionSetting;
            pstMessage->header.unEventTime = stRemoteCommand.unOccurredDateTime;

            pstMessage->carReport.rpSetting.UserSetting.ucCommandType = stRemoteCommand.eCommandType;
            memcpy((char*)pstMessage->carReport.rpSetting.UserSetting.RequestGUID,
                               (char*)stRemoteCommand.guid,MAX_GUID_LENGTH);

            if( stRemoteCommand.bControlType == 1 )
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.EncrryptType.unEncryptType = DCS_ENC_AES;
            }
            if( stRemoteCommand.bControlType == 2 )
            {
                pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.EncrryptType.unEncryptType = DCS_ENC_KMS;
            }
            break;
#if defined(FEATURE_EXTENSION_BOARD)
        case eREMOTE_CON_CMD_TYPE_SETTING_ANTI_THIEF:
            pstMessage->header.subEvent = eR_ReqUserActionSetting;
            pstMessage->header.unEventTime = stRemoteCommand.unOccurredDateTime;
            pstMessage->carReport.rpSetting.UserSetting.ucCommandType = stRemoteCommand.eCommandType;

            memcpy((char*)pstMessage->carReport.rpSetting.UserSetting.RequestGUID,
                   (char*)stRemoteCommand.guid,MAX_GUID_LENGTH);

            pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.stAntiThiefCtl.bAntithiefFlag = stRemoteCommand.bControlType;

            Trace("##################################################\r\n");
            Trace("Set FOB On/Off Setting value %d\r\n", stRemoteCommand.bControlType);
            Trace("##################################################\r\n");
            break;
#endif
        case eREMOTE_CON_CMD_TYPE_NONE:
        case eREMOTE_CON_CMD_TYPE_GEO_FENCE:
        case eREMOTE_CON_CMD_TYPE_POLYGON_GEO_FENCE:
        default:
            Trace("UserActionSetting : Not Defined\r\n");
            break;
    }
}

stMsgMdm m_stLastReceivedRemoteControlMessage;

void SetLastRemoteControlMessage(stMsgMdm* pstMsgMdm)
{
    memcpy((char*)&m_stLastReceivedRemoteControlMessage,(char*)pstMsgMdm,sizeof(stMsgMdm));
}

void GetLastRemoteControlMessage(stMsgMdm* pstMsgMdm)
{
    memcpy((char*)pstMsgMdm,(char*)&m_stLastReceivedRemoteControlMessage,sizeof(stMsgMdm));
}

void HandlerRemoteCommand()
{
    stMsgMdm stModem;

    if(MessageManagerData.bReceivedRequestRemoteMessageFlag == eMESSAGE_REMOTE_CONTROL_STATE_OCCURE)
    {
		// parse remote control message.
		MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_PROCESS;

        // this is parsing process of recevied from server

        if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST )
        {
            uint32_t unOccurredEventTime;
            stMsgSysMsg stMessage;

            boolean_t ret = ParseRemoteMessage(&unOccurredEventTime); //parsing

        	if(ret == false) {
                Trace("Guid Wrong\r\n");
        		MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;
                return;
                //return true;
        	}

            GetLastRemoteControlMessage(&stModem);
            // add check between occurred time and received sms time
            // if different time is over 40-50s, then remove this command
            // if we need to notify to server, then report reject report.
            if( stModem.carReport.rpSmartKey.Request.OccurredEventTime - unOccurredEventTime > 40 )
            {
                // notify to server if need
                Trace("Occurred Event Time - Sms Received Time : %d\r\n", stModem.carReport.rpSmartKey.Request.OccurredEventTime - unOccurredEventTime);
                //return
            }

        	printf("\r\n\r\n");
        	printf("****************************************\r\n");
        	printf("MSG: Remote control event!!!\r\n");
        	printf("****************************************\r\n\r\n");

            PrepareReportMessageAccording2Command(&stMessage);

            Send2MngSysMsg3(&stMessage);
        }
        else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_SETTING_GEOFENCE )
        {
            Trace("##################################################\r\n");
            Trace("Get GeoFence Setting value\r\n");
            Trace("##################################################\r\n");

            stCarReport stGeofenceSetting;
            memset((char*)&stGeofenceSetting,0,sizeof(stCarReport));

            ParseGeoFenceMessage(&stGeofenceSetting);

            GetLastRemoteControlMessage(&stModem);
            // add check between occurred time and received sms time
            // if different time is over 40-50s, then remove this command
            // if we need to notify to server, then report reject report.
            if( stModem.carReport.rpSmartKey.Request.OccurredEventTime - stGeofenceSetting.rpSetting.UserSetting.stUserActionSetting.Geofence.unDateTime > 40 )
            {
                // notify to server if need
                Trace("Occurred Event Time - Sms Received Time : %d\r\n", stModem.carReport.rpSmartKey.Request.OccurredEventTime - stGeofenceSetting.rpSetting.UserSetting.stUserActionSetting.Geofence.unDateTime);
                //return
            }

            stGeofenceSetting.rpSetting.UserSetting.ucCommandType = eREMOTE_CON_CMD_TYPE_GEO_FENCE;

            Send2MngSysMsg(eMngModem, eReqReport, eR_ReqUserActionSetting, (stCarReport*)&stGeofenceSetting,0);
        }
#if defined(PROTOCOL18)
		else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_REQUEST_RESERVATION_ENGINE_CONTROL_SETTING )
		{
            Trace("##################################################\r\n");
            Trace("Get Reservation Engine Control Setting value\r\n");
            Trace("##################################################\r\n");

			stCarReport stRsvEngCtrl;

			memset((char*)&stRsvEngCtrl,0,sizeof(stCarReport));
			ParseRsvEngCtrlMessage(&stRsvEngCtrl);

			stRsvEngCtrl.rpSetting.UserSetting.ucCommandType = eREMOTE_CON_CMD_TYPE_SETTING_RSV_ENG_CTRL;

			Send2MngSysMsg(eMngModem, eReqReport, eR_ReqUserActionSetting, (stCarReport*)&stRsvEngCtrl,0);

			//GetLastRemoteControlMessage(&stModem); 불필요 한듯하다 확인 필요
		}
#endif
        else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_SETTING_POLYGON_GEOFENCE )
        {
            Trace("##################################################\r\n");
            Trace("Get Polygon GeoFence Setting value\r\n");
            Trace("##################################################\r\n");

            stCarReport stGeofenceSetting;
            memset((char*)&stGeofenceSetting,0,sizeof(stCarReport));

            ParsePolygonGeoFenceMessage(&stGeofenceSetting);

            GetLastRemoteControlMessage(&stModem);
            // add check between occurred time and received sms time
            // if different time is over 40-50s, then remove this command
            // if we need to notify to server, then report reject report.
            if( stModem.carReport.rpSmartKey.Request.OccurredEventTime - stGeofenceSetting.rpSetting.UserSetting.stUserActionSetting.Geofence.unDateTime > 40 )
            {
                // notify to server if need
                Trace("Occurred Event Time - Sms Received Time : %d\r\n", stModem.carReport.rpSmartKey.Request.OccurredEventTime - stGeofenceSetting.rpSetting.UserSetting.stUserActionSetting.Geofence.unDateTime);
                //return
            }

            stGeofenceSetting.rpSetting.UserSetting.ucCommandType = eREMOTE_CON_CMD_TYPE_POLYGON_GEO_FENCE;

            Send2MngSysMsg(eMngModem, eReqReport, eR_ReqUserActionSetting, (stCarReport*)&stGeofenceSetting,0);
        }
        else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_ENGINE_START )
        {
            Trace("##################################################\r\n");
            Trace("Get Setting Info\r\n");
            Trace("##################################################\r\n");

            ParseSettingInfoMessage();
        }
        else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_IPEK_PHASE1 )
        {
            Trace("##################################################\r\n");
            Trace("Get IPEK Phase 1 value\r\n");
            Trace("##################################################\r\n");

            stReportIpek IpekProperties;
            GetIpekProperties(&IpekProperties);

            if( ParseIpekPhase1Message(&IpekProperties) == true )
            {
                SetIpekProperties(&IpekProperties);
                Send2MngSysMsg2(eMngModem, eRspIpek,eIpekPhase1,eTrue, (stCarReport *)NULL,0);
            }
            else
            {
                Send2MngSysMsg2(eMngModem, eRspIpek,eIpekPhase1,eFalse, (stCarReport *)NULL,0);
            }
        }
        else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_IPEK_PHASE2 )
        {
            Trace("##################################################\r\n");
            Trace("Get IPEK Phase 2 value\r\n");
            Trace("##################################################\r\n");

            stReportIpek IpekProperties;
            GetIpekProperties(&IpekProperties);

            if( ParseIpekPhase2Message(&IpekProperties) == true )
            {
                SetIpekProperties(&IpekProperties);
                Send2MngSysMsg2(eMngModem, eRspIpek,eIpekPhase2,eTrue,(stCarReport *)NULL,0);
            }
            else
            {
                Send2MngSysMsg2(eMngModem, eRspIpek,eIpekPhase2,eFalse,(stCarReport *)NULL,0);
            }
        }
#if defined(PROTOCOL12)
        else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_REQUEST_MODEM_ACTIVATE )
        {
            Trace("##################################################\r\n");
            Trace("Get Modem Activate\r\n");
            Trace("##################################################\r\n");
            stMsgSysMsg stMessage;

            ParseModemActivateMessage(&stMessage);

            Trace("UserActionSetting : ActiveModem\r\n");
            stMessage.header.id = eMngModem;
            stMessage.header.event = eReqReport;
            stMessage.header.subEvent = eR_ReqUserActionSetting;
            stMessage.header.unEventTime = GetLocalTimefromTime(GetUTCTime());

            stMessage.carReport.rpSetting.UserSetting.ucCommandType = eREMOTE_CON_CMD_TYPE_MODEM_ACTIVE;

            Send2MngSysMsg3(&stMessage);
        }
#endif
#if defined(PROTOCOL15)
        else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_REQUEST_SENSOR_INITIALIZE )
        {
            Trace("##################################################\r\n");
            Trace("Set Sensor Initialize\r\n");
            Trace("##################################################\r\n");
            stMsgSysMsg stMessage;

            //ParseModemActivateMessage(&stMessage);

            Trace("UserActionSetting : ActiveModem\r\n");
            stMessage.header.id = eMngModem;
            stMessage.header.event = eReqReport;
            stMessage.header.subEvent = eR_ReqUserActionSetting;
            stMessage.header.unEventTime = GetLocalTimefromTime(GetUTCTime());

            stMessage.carReport.rpSetting.UserSetting.ucCommandType = eREMOTE_CON_CMD_TYPE_SENSOR_INITIALIZE;

            Send2MngSysMsg3(&stMessage);
        }
#endif
#if defined(PROTOCOL17)
        else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_REQUEST_SETURL )
        {
            Trace("##################################################\r\n");
            Trace("Set URL Parsing\r\n");
            Trace("##################################################\r\n");
            stMsgSysMsg stMessage;

			ParseSetURLMessage(&stMessage);

            stMessage.header.id = eMngModem;
            stMessage.header.event = eReqSysSetting;
            stMessage.header.subEvent = eR_ReqSetURL;
            stMessage.header.unEventTime = GetLocalTimefromTime(GetUTCTime());

            Send2MngSysMsg3(&stMessage);
        }
		else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_RESPONSE_SETURL )
        {
            Trace("##################################################\r\n");
            Trace("Send SetURL to System\r\n");
            Trace("##################################################\r\n");
            stMsgSysMsg stMessage;

			memcpy((char*)&stMessage.carReport.rpSetting.UserSetting.RequestGUID,(char*)&g_SavedGUID,MAX_GUID_LENGTH);
            stMessage.header.id = eMngModem;
            stMessage.header.event = eReqSysSetting;
            stMessage.header.subEvent = eR_ReqSetURLSave;
            stMessage.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
			stMessage.carReport.rpSetting.UserSetting.stUserActionSetting.SetUrl.unEventTime = GetLocalTimefromTime(GetUTCTime());

            Send2MngSysMsg3(&stMessage);
        }
		else if( ModemManagerData.eCurrentMessageSendingType == eMESSAGE_TYPE_TRACKING_REQUEST )
		{
            printf("!!TrackingMode_Set!!\n");
			stMsgSysMsg stMessage;

			ParseSetTrackingMessage(&stMessage);

            stMessage.header.id = eMngModem;
            stMessage.header.event = eReqSysSetting;
            stMessage.header.subEvent = eR_SetTrackingMode;
            stMessage.header.unEventTime = GetLocalTimefromTime(GetUTCTime());

            Send2MngSysMsg3(&stMessage);
		}
#endif

        MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;
    }
}

boolean_t m_bIsItCheckedResetFlag = false;

void HandlerModemReset()
{
    if( GetModemStatus() < eMngMdmRegistedNetwork )
    {
        if( GetModemResetRetryCount() >= MAX_MODEM_RESET_RETRY_COUNT )
        {
            uint8_t bSystemReset;

            SetModemNotWorkingFlag(true);

            SetModemState(eMODEM_READY);
            ModemManagerData.bNeedModemHWResetFlag = false;
            ModemManagerData.bRunWorkaroundFlag = false;
            ModemManagerData.bCheckNetworkStatusFlag = false;
            ModemManagerData.eFOTAStartState = eFOTA_START_STATE_STOP;
            ModemManagerData.bRequestMessageCommFlag = false;
            ModemManagerData.bAvailableModemCommFlag = false;

            // save device reset flag for device reset
            GetAutolinkConfigProperty(eAutoLinkConfig_SystemResetFlag,(void*)&bSystemReset);

            if( bSystemReset == true )
            {
                Trace("#########################################################\r\n");
                Trace("### This is a sleep exception process of modem ###\r\n");
                Trace("#########################################################\r\n");

                // request sleep to system
                Send2MngSys(eMngModem,eReqSleep,0,(stCarReport *)NULL,0);
            }
            else
            {
                Trace("#########################################################\r\n");
                Trace("### This is a device reset exception process of modem ###\r\n");
                Trace("#########################################################\r\n");

                bSystemReset = true;
                SetAutolinkConfigProperty(eAutoLinkConfig_SystemResetFlag,(void*)&bSystemReset);

                // request sleep to system
                Send2MngSys(eMngModem,eReqSystemReset,0,(stCarReport *)NULL,0);
            }

            ClearModemResetRetryCount();
        }
    }

    if( m_bIsItCheckedResetFlag == false && GetModemStatus() >= eMngMdmInitializeHardware )
    {
        boolean_t bSystemReset;
        GetAutolinkConfigProperty(eAutoLinkConfig_SystemResetFlag,(void*)&bSystemReset);

        if( bSystemReset == true )
        {
            bSystemReset = false;
            SetAutolinkConfigProperty(eAutoLinkConfig_SystemResetFlag,(void*)&bSystemReset);
        }

        m_bIsItCheckedResetFlag = true;
    }
}

unsigned long m_ulRequestNetworkTimeStamp = 0;

void RequestNetworkTime()
{
    if( (SystemDelayProcess(&m_ulRequestNetworkTimeStamp,20*ONE_SECOND) == true) && (ModemManagerData.bAvailableModemCommFlag==true) )
    {
        // request network time
        if( SetRequestActionFromManager(eMdmReqNetworkTime,false) == true )
        {
            //Trace("request network time\r\n");
        }
    }
}

unsigned long m_ulRequestRssiTimeStamp = 0;

void RequestRssi()
{
    if( (SystemDelayProcess(&m_ulRequestRssiTimeStamp,3*ONE_SECOND) == true) && (ModemManagerData.bAvailableModemCommFlag==true) )
    {
        // request rssi
        if( SetRequestActionFromManager(eMdmReqRssi,false) == true )
        {
            //Trace("request rssi\r\n");
        }
    }
}


uint8_t m_cCurModemLedStatus;
uint8_t m_cOldModemLedStatus;

uint8_t m_cModemLedStatus;

void SetModemLedStatus(uint8_t cLedStatus)
{
    m_cCurModemLedStatus = cLedStatus;
}

enum{
    eLedMode_SlowBlink,
    eLedMode_Steady,
    eLedMode_FastBlink,
};

uint8_t m_cLedMode = eLedMode_SlowBlink;
boolean_t m_bLedBlink;

unsigned long m_ulModemLedBlinkTime = 0;


void LedBlink(uint32_t nBlinkTime)
{
    if( OemGetTmrDelta(OemGetTmr(), m_ulModemLedBlinkTime) > nBlinkTime )
    {
        if( m_bLedBlink == true )
        {
            SetLedOnOffCtl(LED_ON, eLED_SERVER);
            m_bLedBlink = false;
        }
        else
        {
            SetLedOnOffCtl(LED_OFF, eLED_SERVER);
            m_bLedBlink = true;
        }

        m_ulModemLedBlinkTime = OemGetTmr();
    }
}

void ProcessModemLedControl()
{
    if( m_cOldModemLedStatus != m_cCurModemLedStatus )
    {
        switch(m_cCurModemLedStatus)
        {
            case eModemLedStatus_Disconnect:
                m_cLedMode = eLedMode_SlowBlink;
                break;
            case eModemLedStatus_Connected:
                m_cLedMode = eLedMode_Steady;
                break;
            case eModemLedStatus_Transfer:
                m_cLedMode = eLedMode_FastBlink;
                break;
        }

        m_cOldModemLedStatus = m_cCurModemLedStatus;
    }

    if((g_bYUJINSelftestFlag != true)&&(g_bHYPERTECSelftestFlag != true)){
		switch(m_cLedMode)
		{

			case eLedMode_Steady:
				SetLedOnOffCtl(LED_ON, eLED_SERVER);
				break;
			case eLedMode_FastBlink:
				LedBlink(50);
				break;
			case eLedMode_SlowBlink:
				LedBlink(500);
				break;
		}
	}
}


void CallBackModemLedStatus()
{
    // LED Control
    ProcessModemLedControl();
}
