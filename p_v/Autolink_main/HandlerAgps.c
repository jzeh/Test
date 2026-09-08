/* Includes ------------------------------------------------------------------*/
#include "AutolinkMessage.h"

#include "HalHandler.h"
#include "MngSystem.h"
#include "MngSystemUtil.h"
#include "GIT_Util.h"
#include "GIT_Gps.h"

#include "HdDebug.h"
#include "Message_make.h"
#include "Modem_Manager.h"
#include "GIT_OemInterface.h"
#include "SysHalFileSystem.h"
#include "MngModem.h"

#include <math.h>

#define AUTOLINK_AGPS_DATA      "UbloxAgpsData"

#define UBLOX_HEADER_LENGTH     6
#define UBLOX_CHECKSUM_LENGTH   2

#define UBX_PREFIX1 0xB5
#define UBX_PREFIX2 0x62

#define MAX_UBLOX_SENT_SKIP_TIME		200
#define MAX_UBLOX_SENT_WAIT_TIME		2

#define ENABLE_UBLOX_AGPS
//#define ENABLE_PERIOD_NAV_STATUS

#ifdef ENABLE_UBLOX_GPS_DATA
#define ENABLE_PERIOD_NAV_PVT
#endif

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)

#define AGPS_EXPIRED_DATE (7*(24*(60*60)))
extern BR_SystemInfo BkSram_SystemInfo;
extern boolean_t IsAgpsFileExist();
extern eGitFresult ReadAgpsDataSize(char* pstrFileName, int* pnSize);
extern int ReadAgpsData(char* pstrFileName, char* pcarrBuff, int nSize, int nIndex);
extern eGitFresult DeleteAgpsData(char* pstrFileName);
extern bool SystemDelayProcess(unsigned long* nBaseTime, int nDelay);
extern uint32_t GetUTCTime();
extern boolean_t UbloxInitHandler();

boolean_t CheckUbloxCheckSum(char* pcarrBuff, int nBufSize);

typedef enum __UBLOX_RCV_TYPE{
    eUbloxNone = 0,
    eUbloxData,
    eUbloxProtocol,
}UBLOX_RCV_TYPE;

typedef __packed struct __stUbloxHeader
{
    uint8_t preamble1;
    uint8_t preamble2;
    uint8_t classId;
    uint8_t msgId;
    uint16_t length;
}stUbloxHeader;

typedef __packed struct __stUbloxPacket
{
    stUbloxHeader header;
    char buffer[512];
    uint8_t checksumx;
    uint8_t checksumy;
}stUbloxPacket;

/*
NAV 0x01 Navigation Results Messages: Position, Speed, Time, Acceleration, Heading, DOP, SVs used
RXM 0x02 Receiver Manager Messages: Satellite Status, RTC Status
INF 0x04 Information Messages: Printf-Style Messages, with IDs such as Error, Warning, Notice
ACK 0x05 Ack/Nak Messages: Acknowledge or Reject messages to CFG input messages
CFG 0x06 Configuration Input Messages: Set Dynamic Model, Set DOP Mask, Set Baud Rate, etc.
UPD 0x09 Firmware Update Messages: Memory/Flash erase/write, Reboot, Flash identification, etc.
MON 0x0A Monitoring Messages: Communication Status, CPU Load, Stack Usage, Task Status
AID 0x0B AssistNow Aiding Messages: Ephemeris, Almanac, other A-GPS data input
TIM 0x0D Timing Messages: Time Pulse Output, Time Mark Results
ESF 0x10 External Sensor Fusion Messages: External Sensor Measurements and Status Information
MGA 0x13 Multiple GNSS Assistance Messages: Assistance data for various GNSS
LOG 0x21 Logging Messages: Log creation, deletion, info and retrieval
UBX-13003221 - R15 Early Production Information Page 137 of 386
u-blox 8 / u-blox M8 Receiver Description - Manual
UBX Class IDs continued
Name Class Description
SEC 0x27 Security Feature Messages
HNR 0x28 High Rate Navigation Results Messages: High rate time, position, speed, heading
All remaining
*/

enum{
    eUbloxClassId_Nav = 0x01,
    eUbloxClassId_Rxm = 0x02,
    eUbloxClassId_Inf = 0x04,
    eUbloxClassId_AckNak = 0x05,
    eUbloxClassId_Cfg = 0x06,
    eUbloxClassId_Upd = 0x09,
    eUbloxClassId_Mon = 0x0A,
    eUbloxClassId_Aid = 0x0B,
    eUbloxClassId_Tim = 0x0D,
    eUbloxClassId_Esf = 0x10,
    eUbloxClassId_Mga = 0x13,
    eUbloxClassId_Log = 0x21,
    eUbloxClassId_Max = 12,
};

enum{
    eUBX_AID_ALM = 0x0B30,
    eUBX_AID_AOP = 0x0B33,
    eUBX_AID_EPH = 0x0B31,
    eUBX_AID_HUI = 0x0B02,
    eUBX_AID_INI = 0x0B01,
    eUBX_AID_MAPM = 0x0B05,
    eUBX_CFG_ANT = 0x0613,
    eUBX_CFG_BATCH = 0x0693,
    eUBX_CFG_CFG = 0x0609,
    eUBX_CFG_DAT = 0x0606,
    eUBX_CFG_MSG = 0x0601,
    eUBX_CFG_INF = 0x0602,
    eUBX_MGA_ACK = 0x1360,
    eUBX_MGA_ANO = 0x1320,
    eUBX_MGA_BDS = 0x1303,
    eUBX_MGA_DBD = 0x1380,
    eUBX_MGA_FLASH = 0x1321,
    eUBX_MGA_GAL = 0x1302,
    eUBX_MGA_GLO = 0x1306,
    eUBX_MGA_GPS = 0x1300,
    eUBX_MGA_INI = 0x1340,
    eUBX_MGA_QZSS = 0x1305,
    eUBX_NAV_PVT = 0x0107,
    eUBX_NAV_SAT = 0x0135,
};

enum {
    eUbloxInitState_SendReset = 0,
    eUbloxInitState_SendResetRsp,
    eUbloxInitState_SendAutomotive,
    eUbloxInitState_SendAutomotiveRsp,
    eUbloxInitState_SendAssistnow,
    eUbloxInitState_SendAssistnowRsp,
    eUbloxInitState_SendMgaIni,
    eUbloxInitState_SendMgaIniRsp,
    eUbloxInitState_SendPollingPvt,
    eUbloxInitState_SendPollingPvtRsp,
    eUbloxInitState_SendPollingSat,
    eUbloxInitState_SendPollingSatRsp,
    eUbloxInitState_Skip,
    eUbloxInitState_SkipRsp,
    eUbloxInitState_Done,
};



enum{
    eUbloxUploadInit = 0,
    eUbloxUploadSendAgpsData,
    eUbloxUploadWaitAgpsDataRsp,
    eUbloxUploadSendAgpsStop,
    eUbloxUploadSendAgpsStopRsp,
    eUbloxUploadSendAgpsDone,
};


typedef void (*fnpUbloxHandler)(char*,int32_t);

typedef struct __stUbloxParseHandle{
    uint8_t ucHandlerType;
    fnpUbloxHandler fpnHandler;
}stUbloxParseHandle;

stUbloxHeader m_cReqeustUbloxProtocol;
boolean_t m_bReqeustUbloxProtocol = false;
boolean_t m_bRequestUbloxProtocolResult = false;
int m_nRequestUbloxProtocolResult = -1;
int iTimer_AGPS_Send_TimeoutDly = -1;

void SetUbloxHandlerStates(uint32_t unState);
void MakeUbloxPacket(uint8_t ucClassId, uint8_t ucMsgId, char* parrDstBuffer, uint32_t* pnSrcSize);

void SetUBX_NAVX5(void);
void SetUBX_NAV5(void);
void SetAGPSEnd(void);
void SetUbxMgaIni();
void SetTimePolling(uint16_t usMsgid);
void SetPollingSat(uint16_t usMsgid);
void SetRestUBlox();
void SetUbxNavStatus();
void SetUbxNavPvt();
void SetUBX_MgaAck();

boolean_t GetRequestUbloxProtcolResult(int* pnResult);
void SetReqeustUbloxProtocol(uint8_t ucClassId, uint8_t ucMsgId);
void GetReqeustUbloxProtocol(stUbloxHeader* header);
void HandlerUbloxProtocol(char* arrNmeaData, int uiReceivedPacket);


void UbloxProtocol_Nav(char* carrBuff,int32_t nLength);
void UbloxProtocol_Rxm(char* carrBuff,int32_t nLength);
void UbloxProtocol_Inf(char* carrBuff,int32_t nLength);
void UbloxProtocol_AckNak(char* carrBuff,int32_t nLength);
void UbloxProtocol_Cfg(char* carrBuff,int32_t nLength);
void UbloxProtocol_Upd(char* carrBuff,int32_t nLength);
void UbloxProtocol_Mon(char* carrBuff,int32_t nLength);
void UbloxProtocol_Aid(char* carrBuff,int32_t nLength);
void UbloxProtocol_Tim(char* carrBuff,int32_t nLength);
void UbloxProtocol_Esf(char* carrBuff,int32_t nLength);
void UbloxProtocol_Mga(char* carrBuff,int32_t nLength);
void UbloxProtocol_Log(char* carrBuff,int32_t nLength);

extern int Get_GPS_Vailication();
extern int Get_GPS_SatellitesNum();
extern int Get_GPS_SatellitesUsedNum();
extern double Get_GPS_Lat();
extern double Get_GPS_Lon();
extern void SetNewGpsInfo(int32_t nlat,int32_t nlon,uint8_t fixType,uint8_t numSV, nmeaTIME tm, int32_t direction);

boolean_t UbloxDisableNmea();

// parsing handler list
stUbloxParseHandle m_stUbloxParsingHandler[eUbloxClassId_Max]=
{
    {eUbloxClassId_Nav,UbloxProtocol_Nav},
    {eUbloxClassId_Rxm,UbloxProtocol_Rxm},
    {eUbloxClassId_Inf,UbloxProtocol_Inf},
    {eUbloxClassId_AckNak,UbloxProtocol_AckNak},
    {eUbloxClassId_Cfg,UbloxProtocol_Cfg},
    {eUbloxClassId_Upd,UbloxProtocol_Upd},
    {eUbloxClassId_Mon,UbloxProtocol_Mon},
    {eUbloxClassId_Aid,UbloxProtocol_Aid},
    {eUbloxClassId_Tim,UbloxProtocol_Tim},
    {eUbloxClassId_Esf,UbloxProtocol_Esf},
    {eUbloxClassId_Mga,UbloxProtocol_Mga},
    {eUbloxClassId_Log,UbloxProtocol_Log},
};

void HandlerAgps(stMsgSysMsg* pstMsgSysMsg)
{
    if( pstMsgSysMsg->header.event == eReqAgps )
    {
        //Trace("Req Agps\r\n");
        if( pstMsgSysMsg->header.subEvent == eAgpsDownload )
        {
#ifdef RF_COMMON_MODEM
        	if( CheckProductMode()==true )
        	{
        		Send2MngSysMsg(eMngModem,eRspAgps,eAgpsDownloadDone,(stCarReport *)NULL,0);
        	}
			else
			{
            	Send2MngModem3(eMngSysMsg,eReqAgps,eAgpsDownload,0,(stCarReport *)NULL,0);
			}
#else
			Send2MngModem3(eMngSysMsg,eReqAgps,eAgpsDownload,0,(stCarReport *)NULL,0);
#endif
        }
    }
    else if( pstMsgSysMsg->header.event == eRspAgps )
    {
        //Trace("Rsp Agps\r\n");
        if( pstMsgSysMsg->header.subEvent == eAgpsDownloadDone )
        {
        	printf("AGPS Download Success\r\n");
#ifdef RF_COMMON_MODEM //THALES_AGPS
			iTimer_AGPS_Send_TimeoutDly = HalTimerSetSWTimer(MDM_AGPS_SEND_TIMEOUT, eSWTimer_ONESHOT, MDM_AGPS_Send_Timeout_CallBack, true);
            SetRegModemProcessFunction(ProcessSendAGPSData, ProcessSendAGPSDataResult, MODEM_RESPONSE_TYPE_AGPS);
#else	//USE_UBLOX_GPS
			// start update agps data to ublox
			SetUbloxHandlerStates(0);
#endif	//USE_UBLOX_GPS
        }
		else if( pstMsgSysMsg->header.subEvent == eAgpsDownloadFail )
		{
	
			printf("AGPS Download Fail\r\n");
			SetReqWaitNetworkCheck(false);
   			SetModemState(eMODEM_READY);
		}
    }
}

void SetUbloxHandlerStates(uint32_t unState)
{
}

uint32_t m_unUbloxStates = eUbloxInit;
unsigned long m_ulAssisnowSentTime = 0;

uint32_t m_unUbloxInitStates = eUbloxInitState_SendReset;
#ifdef USE_UBLOX_GPS
boolean_t UbloxInitHandler()
{
    int nResult;

    switch(m_unUbloxInitStates)
    {
        case eUbloxInitState_SendReset:
            //SetRestUBlox();
            m_unUbloxInitStates = eUbloxInitState_SendAutomotive;
            break;
        case eUbloxInitState_SendAutomotive:
           // Trace("GPS: eUbloxInitState_SendAutomotive\r\n");
            SetUBX_NAV5();

            SetReqeustUbloxProtocol(eUbloxClassId_Cfg,0x24);
            m_unUbloxInitStates = eUbloxInitState_SendAutomotiveRsp;

            m_ulAssisnowSentTime = Get_Tmr();
            break;
        case eUbloxInitState_SendAutomotiveRsp:
            if( GetRequestUbloxProtcolResult(&nResult) == true )
            {
                m_unUbloxInitStates = eUbloxInitState_SendAssistnow;
            }
            else
            {
                if(Get_TmrDelta(Get_Tmr(), m_ulAssisnowSentTime)>= MAX_UBLOX_SENT_SKIP_TIME)
                {
                    m_unUbloxInitStates = eUbloxInitState_SendAutomotive;
                }
            }

            break;
        case eUbloxInitState_SendAssistnow:
            Trace("AGPS: eUbloxInitState_SendAssistnow\r\n");
            SetUBX_NAVX5();

            SetReqeustUbloxProtocol(eUbloxClassId_Cfg,0x23);
            m_unUbloxInitStates = eUbloxInitState_SendAssistnowRsp;

            m_ulAssisnowSentTime = Get_Tmr();
            break;
        case eUbloxInitState_SendAssistnowRsp:
            if( GetRequestUbloxProtcolResult(&nResult) == true )
            {
                m_unUbloxInitStates = eUbloxInitState_SendMgaIni;
            }
            else
            {
    		    if(Get_TmrDelta(Get_Tmr(), m_ulAssisnowSentTime)>= MAX_UBLOX_SENT_SKIP_TIME)
    		    {
                    m_unUbloxInitStates = eUbloxInitState_SendAssistnow;
    		    }
            }
            break;
        case eUbloxInitState_SendMgaIni:
            Trace("GPS: eUbloxInitState_SendMgaIni\r\n");
            SetUbxMgaIni();

            SetReqeustUbloxProtocol(eUbloxClassId_Mga,0x60);
            m_unUbloxInitStates = eUbloxInitState_SendMgaIniRsp;

            m_ulAssisnowSentTime = Get_Tmr();
            break;
        case eUbloxInitState_SendMgaIniRsp:
            if( GetRequestUbloxProtcolResult(&nResult) == true )
            {
#ifdef ENABLE_PERIOD_NAV_PVT
                m_unUbloxInitStates = eUbloxInitState_SendPollingPvt;
#else //#ifdef ENABLE_PERIOD_NAV_PVT

// 180717 SPARROW : 생산 중에는 동작이 달라야 하여 수정
//#ifdef ENABLE_PERIOD_NAV_SAT
//                m_unUbloxInitStates = eUbloxInitState_SendPollingSat;
//#else //#ifdef ENABLE_PERIOD_NAV_SAT
//                m_unUbloxInitStates = eUbloxInitState_Done;
//#endif //#ifdef ENABLE_PERIOD_NAV_SAT

				if( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_NOSERIAL_NUMBER) == 0 )	m_unUbloxInitStates = eUbloxInitState_SendPollingSat;
				else																							m_unUbloxInitStates = eUbloxInitState_Done;

#endif //#ifdef ENABLE_PERIOD_NAV_PVT
            }
            else
            {
                if(Get_TmrDelta(Get_Tmr(), m_ulAssisnowSentTime)>= (MAX_UBLOX_SENT_SKIP_TIME))
                {
                    m_unUbloxInitStates = eUbloxInitState_SendMgaIni;
                }
            }
            break;
        case eUbloxInitState_SendPollingPvt:
            // ME could check ano message with this message
            Trace("GPS: eUbloxInitState_SendPollingPvt\r\n");
            SetUbxNavPvt();
            SetReqeustUbloxProtocol(eUbloxClassId_Cfg,0x01);
            m_unUbloxInitStates = eUbloxInitState_SendPollingPvtRsp;

            m_ulAssisnowSentTime = Get_Tmr();
            break;
        case eUbloxInitState_SendPollingPvtRsp:
            if( GetRequestUbloxProtcolResult(&nResult) == true )
            {
// 180717 SPARROW : 생산 중에는 동작이 달라야 하여 수정
//#ifdef ENABLE_PERIOD_NAV_SAT
//                m_unUbloxInitStates = eUbloxInitState_SendPollingSat;
//#else //#ifdef ENABLE_PERIOD_NAV_SAT
//                m_unUbloxInitStates = eUbloxInitState_Done;
//#endif //#ifdef ENABLE_PERIOD_NAV_SAT
				if( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_NOSERIAL_NUMBER) == 0 )	m_unUbloxInitStates = eUbloxInitState_SendPollingSat;
				else																							m_unUbloxInitStates = eUbloxInitState_Done;

            }
            else
            {
                if(Get_TmrDelta(Get_Tmr(), m_ulAssisnowSentTime)>= MAX_UBLOX_SENT_SKIP_TIME)
                {
                    m_unUbloxInitStates = eUbloxInitState_SendPollingPvt;
                }
            }
            break;
        case eUbloxInitState_SendPollingSat:
            // ME could check ano message with this message
            Trace("GPS: eUbloxInitState_SendPollingSat\r\n");
            //SetUbxNavStatus();
            SetPollingSat(eUBX_CFG_MSG);
            SetReqeustUbloxProtocol(eUbloxClassId_Cfg,0x01);
            m_unUbloxInitStates = eUbloxInitState_SendPollingSatRsp;

            m_ulAssisnowSentTime = Get_Tmr();
            break;
        case eUbloxInitState_SendPollingSatRsp:
            if( GetRequestUbloxProtcolResult(&nResult) == true )
            {
                m_unUbloxInitStates = eUbloxInitState_Done;
            }
            else
            {
                if(Get_TmrDelta(Get_Tmr(), m_ulAssisnowSentTime)>= MAX_UBLOX_SENT_SKIP_TIME)
                {
                    m_unUbloxInitStates = eUbloxInitState_SendPollingSat;
                }
            }
            break;
        case eUbloxInitState_Done:
            return true;
            break;
        case eUbloxInitState_Skip:
        case eUbloxInitState_SkipRsp:
        default:
            Trace("Not defined\r\n");
            break;
    }

    return false;
}
#endif
boolean_t ExistAgpsFile()
{
    if( IsAgpsFileExist() == true )
        return true;

    return false;
}

uint32_t m_unUbloxUpdateStates = eUbloxUploadInit;
uint32_t m_unUbloxUpdateDataIndex = 0;
uint32_t m_unUbloxUpdateDataSize = 0;

boolean_t IsMatchANOUtcTime(char* pcarrAnoMessage,int nReadSize)
{
    char cYear;
    char cMonth;
    char cDay;
    stHalRTCTypeDef stDate;
    uint32_t unUtcTime;

    if( pcarrAnoMessage[0] != UBX_PREFIX1 && pcarrAnoMessage[1] != UBX_PREFIX2 )
        return false;

    //if( pcarrAnoMessage[2] != ((eUBX_MGA_ANO>>8)&0xFF )&& pcarrAnoMessage[3] != ((eUBX_MGA_ANO)&0xFF ) )
    if( pcarrAnoMessage[2]==0x013 && pcarrAnoMessage[3]==0x020 )
    {
        cYear = pcarrAnoMessage[UBLOX_HEADER_LENGTH+4];
        cMonth = pcarrAnoMessage[UBLOX_HEADER_LENGTH+5];
        cDay = pcarrAnoMessage[UBLOX_HEADER_LENGTH+6];

        unUtcTime = GetUTCTime();
        //unUtcTime = GetLocalTime();
        GetDatefromTime2(&stDate, unUtcTime);

#if false
        Trace("Y:%d,M:%d,D:%d,y:%d,m:%d,d:%d\r\n",
                    stDate.RtcDate.RTC_Year,stDate.RtcDate.RTC_Month,stDate.RtcDate.RTC_Date, 
                    cYear,cMonth,cDay);
#endif

        if( stDate.RtcDate.RTC_Year == cYear && stDate.RtcDate.RTC_Month == cMonth &&
            stDate.RtcDate.RTC_Date == cDay )
        {

            return true;
        }
    }

    return false;
}

#if defined(USE_UBLOX_GPS)
boolean_t UbloxUploadHandler()
{
    int nResult;
    char buffer[1024];
    int nReadSize;
    stUbloxHeader stHeader;

    switch(m_unUbloxUpdateStates)
    {
        case eUbloxUploadInit:
            ReadAgpsDataSize(AUTOLINK_AGPS_DATA,(int*)&m_unUbloxUpdateDataSize);
            Trace("AGPS Read Data from File : %d\r\n",m_unUbloxUpdateDataSize);

#if false
            for(int i=0;i<nSize/1024;i++)
            {
                ReadAgpsData(AUTOLINK_AGPS_DATA,buffer,1024,i*1024);
                hexdump(buffer,1024);
            }

            ReadAgpsData(AUTOLINK_AGPS_DATA,buffer,nSize%1024,(nSize/1024)*1024);
            hexdump(buffer,nSize%1024);
#endif
            m_unUbloxUpdateDataIndex = 0;
            m_unUbloxUpdateStates = eUbloxUploadSendAgpsData;
            break;
        case eUbloxUploadSendAgpsData:

            //Trace("AGPS: eUbloxUploadSendAgpsData\r\n");
            m_ulAssisnowSentTime = Get_Tmr();
            m_unUbloxUpdateStates = eUbloxInitState_SendAssistnowRsp;

            //printf("file size : %x, index : %x\n",m_unUbloxUpdateDataSize,m_unUbloxUpdateDataIndex);
            // read header
            ReadAgpsData(AUTOLINK_AGPS_DATA,(char*)&stHeader,sizeof(stHeader),m_unUbloxUpdateDataIndex);
            // calculate header size
            nReadSize = sizeof(stHeader)+stHeader.length+UBLOX_CHECKSUM_LENGTH;

            if( nReadSize > 512 )
            {
                Trace("AGPS File was broken. Delete APGS file and will be download later\r\n");
                m_unUbloxUpdateStates = eUbloxUploadSendAgpsDone;
                DeleteAgpsData(AUTOLINK_AGPS_DATA);

                // make expire date over the 7 days.
                BkSram_SystemInfo.unAGPSExireDate += AGPS_EXPIRED_DATE;
                return false;
            }

            ReadAgpsData(AUTOLINK_AGPS_DATA,buffer,nReadSize,m_unUbloxUpdateDataIndex);

            if( IsMatchANOUtcTime(buffer,nReadSize) == true )
            {
                printf(">");
                //Trace("IsMatchANOUtcTime:True\r\n");
                SendGITPtclFrame((char*)buffer, nReadSize, eCOMM_TYPE_UART_GPS, NULL, 0);

                //hexdump(buffer,16);

                // 0x60 is response message id
                SetReqeustUbloxProtocol(eUbloxClassId_Mga,0x60);
                m_unUbloxUpdateStates = eUbloxUploadWaitAgpsDataRsp;
            }
            else
            {
                m_unUbloxUpdateStates = eUbloxUploadSendAgpsData;

                //hexdump(buffer,16);
            }

            if( (m_unUbloxUpdateDataIndex+nReadSize) >= m_unUbloxUpdateDataSize)
            {
                //wait respoinse
                m_unUbloxUpdateStates = eUbloxUploadSendAgpsStop;
            }

            //wait respoinse
            m_unUbloxUpdateDataIndex += nReadSize;

            break;
        case eUbloxUploadWaitAgpsDataRsp:
            if( m_unUbloxUpdateDataIndex == m_unUbloxUpdateDataSize )
            {
                m_unUbloxUpdateStates = eUbloxUploadSendAgpsStop;
            }
            else
            {
                if( GetRequestUbloxProtcolResult(&nResult) == true )
                {
                    GetReqeustUbloxProtocol(&stHeader);
                    m_unUbloxUpdateStates = eUbloxUploadSendAgpsData;

                }
                else
                {
                    if(Get_TmrDelta(Get_Tmr(), m_ulAssisnowSentTime)>= MAX_UBLOX_SENT_WAIT_TIME)
                    {
                        //Trace("Ano Skip\r\n");
                        m_unUbloxUpdateStates = eUbloxUploadSendAgpsData;
                    }
                }
            }
            break;
        case eUbloxUploadSendAgpsStop:
            m_unUbloxUpdateStates = eUbloxUploadSendAgpsDone;
#if false
            SetAGPSEnd();

            SetReqeustUbloxProtocol(eUbloxClassId_Mga,0x21);
            m_unUbloxUpdateStates = eUbloxUploadSendAgpsStopRsp;

            m_ulAssisnowSentTime = Get_Tmr();
#endif
            break;
        case eUbloxUploadSendAgpsStopRsp:
            if( GetRequestUbloxProtcolResult(&nResult) == true )
            {
                m_unUbloxUpdateStates = eUbloxUploadSendAgpsDone;
            }
            else
            {
    		    if(Get_TmrDelta(Get_Tmr(), m_ulAssisnowSentTime)>= MAX_UBLOX_SENT_SKIP_TIME)
    		    {
                    //m_unUbloxUpdateStates = eUbloxUploadSendAgpsStop;
                    m_unUbloxUpdateStates = eUbloxUploadSendAgpsDone;
    		    }
            }
            break;
        case eUbloxUploadSendAgpsDone:
            Trace("update done\r\n");
            return true;
            break;
        default:
            break;
    }

    return false;
}
#endif
uint32_t GetUbloxStates()
{
    return m_unUbloxStates;
}

unsigned long m_ulAgpsPollingTimeStamp = 0;

uint32_t HandlerUblox()
{
    switch(m_unUbloxStates)
    {
        case eUbloxInit:
            if( UbloxInitHandler() == true )
            {
#ifdef ENABLE_UBLOX_AGPS
                if( ExistAgpsFile() == true )
                    m_unUbloxStates = eUbloxUpload;
                else
                    m_unUbloxStates = eUbloxIdle;
#else //#ifdef ENABLE_UBLOX_AGPS
                m_unUbloxStates = eUbloxIdle;
#endif //#ifdef ENABLE_UBLOX_AGPS

#ifdef ENABLE_PERIOD_NAV_PVT
                APP_Delay(10);
                SetUbxNavPvt();
                APP_Delay(10);
                SetUbxNavPvt();
#endif //#ifdef ENABLE_PERIOD_NAV_PVT
            }
            break;
        case eUbloxUpload:
#if defined(USE_UBLOX_GPS)
            if( UbloxUploadHandler() == true )
            {
                m_unUbloxStates = eUbloxIdle;
            }
#else
            m_unUbloxStates = eUbloxIdle;
#endif
            break;
        case eUbloxDownload:
            break;
        case eUbloxIdle:
            if( SystemDelayProcess(&m_ulAgpsPollingTimeStamp,5*ONE_SECOND) == true )
            {
                //SetUbxNavStatus();
            }
            break;
        case eUbloxDisableNmea:
            if( UbloxDisableNmea() == true )
            {
                m_unUbloxStates = eUbloxInit;
            }
            break;
        default:
            break;
    }

    return m_unUbloxStates;
}

void SetReqeustUbloxProtocol(uint8_t ucClassId, uint8_t ucMsgId)
{
    stUbloxHeader stHeader;
    stHeader.classId = ucClassId;
    stHeader.msgId = ucMsgId;

    memcpy((char*)&m_cReqeustUbloxProtocol,(char*)&stHeader,sizeof(stUbloxHeader));
    m_bReqeustUbloxProtocol = true;
}

void GetReqeustUbloxProtocol(stUbloxHeader* header)
{
    memcpy((char*)header,(char*)&m_cReqeustUbloxProtocol,sizeof(stUbloxHeader));
}

boolean_t m_bRequestUblocProtocolResult;
void SetRequestUbloxProtcolResult(int nResult)
{
    m_bRequestUbloxProtocolResult = true;
    m_nRequestUbloxProtocolResult = nResult;
}

boolean_t GetRequestUbloxProtcolResult(int* pnResult)
{
    if( m_bRequestUbloxProtocolResult == true )
    {
        *pnResult = m_nRequestUbloxProtocolResult;

        m_bRequestUbloxProtocolResult = false;

        return true;
    }

    *pnResult = 0;

    return false;
}

// parsing handler
void HandlerUbloxProtocol(char* arrNmeaData, int uiReceivedPacket)
{
    stUbloxHeader stRspHeader;
    memcpy((char*)&stRspHeader,arrNmeaData,sizeof(stUbloxHeader));

    for( int i=0;i<eUbloxClassId_Max;i++)
    {
        if( m_stUbloxParsingHandler[i].ucHandlerType == stRspHeader.classId )
        {
            m_stUbloxParsingHandler[i].fpnHandler(arrNmeaData,uiReceivedPacket);
        }
    }
}


void UbloxProtocol_Rxm(char* carrBuff,int32_t nLength)
{
}
void UbloxProtocol_Inf(char* carrBuff,int32_t nLength)
{
}
void UbloxProtocol_AckNak(char* carrBuff,int32_t nLength)
{
    stUbloxHeader stReqHeader;
    uint8_t cClassId;
    uint8_t cMsgId;

    GetReqeustUbloxProtocol(&stReqHeader);

    cClassId = carrBuff[sizeof(stUbloxHeader)];
    cMsgId = carrBuff[sizeof(stUbloxHeader)+1];

    if( stReqHeader.classId == cClassId && stReqHeader.msgId == cMsgId )
    {
        if( stReqHeader.msgId == 0x01 )
            SetRequestUbloxProtcolResult(true);
        else
            SetRequestUbloxProtcolResult(false);
    }
}
void UbloxProtocol_Cfg(char* carrBuff,int32_t nLength)
{
}
void UbloxProtocol_Upd(char* carrBuff,int32_t nLength)
{
}
void UbloxProtocol_Mon(char* carrBuff,int32_t nLength)
{
}
void UbloxProtocol_Aid(char* carrBuff,int32_t nLength)
{
}
void UbloxProtocol_Tim(char* carrBuff,int32_t nLength)
{
}
void UbloxProtocol_Esf(char* carrBuff,int32_t nLength)
{
}

typedef __packed struct __stMgaAck{
    uint8_t type;
    uint8_t version;
    uint8_t infoCode;
    uint8_t msgId;
    uint32_t msgPayloadStart;
}stMgaAck;

void UbloxProtocol_Mga(char* carrBuff,int32_t nLength)
{
    stUbloxHeader stReqHeader;
    stUbloxHeader stRspHeader;
    stMgaAck stAck;
    GetReqeustUbloxProtocol(&stReqHeader);

    memcpy((char*)&stRspHeader,&carrBuff[0],sizeof(stUbloxHeader));
    memcpy((char*)&stAck,&carrBuff[UBLOX_HEADER_LENGTH],sizeof(stMgaAck));

    if( stReqHeader.classId == stRspHeader.classId && stReqHeader.msgId == stRspHeader.msgId )
    {
        if( stAck.type == 0 ) // not acceptable parameter
        {
            printf("<");
            //Trace("Not Acceptable Mga Data\r\n");
        }
        else if( stAck.type == 1 ) // acceptable parameter
        {
            printf("<");
            //Trace("Acceptable Mga Data\r\n");
        }

        SetRequestUbloxProtcolResult(true);
    }
}
void UbloxProtocol_Log(char* carrBuff,int32_t nLength)
{
}

enum{
    eUbxNavStat = 0x03,
    eUbxNavPvt = 0x07,
    eUbxNavSat = 0x35,
};

typedef __packed struct __stSatelliteContents
{
    uint32_t iTow;
    uint8_t version;
    uint8_t numSvs;
    uint16_t reserved;
}stSatelliteContents;

typedef __packed struct __stSatellite{
    uint8_t gnssId;
    uint8_t svId;
    uint8_t cno;
    int8_t elev;
    int16_t azim;
    int16_t prRes;
    uint32_t flag;
}stSatellite;

typedef __packed struct __stNavStatus{
    uint32_t iTow;
    uint8_t gpsFix;
    uint8_t flags;
    uint8_t fixStat;
    uint8_t flags2;
    uint32_t ttff;//time to first fix(milliseconds)
    uint32_t msss;//miliiseconds since startup, reset
}stNavStatus;

typedef __packed struct __stNavPvt{
    uint32_t iTow;          //0
    uint16_t year;          //4
    uint8_t month;          //6
    uint8_t day;            //7
    uint8_t hour;           //8
    uint8_t min;            //9
    uint8_t sec;            //10
    uint8_t valid;          //11
    uint32_t tAcc;          //12
    int32_t nano;           //16
    uint8_t fixType;        //20
    uint8_t flags;          //21
    uint8_t flags2;         //22
    uint8_t numSV;          //23
    int32_t lon;            //24
    int32_t lat;            //28
    int32_t height;         //32
    int32_t hMsl;           //36
    uint32_t hAcc;          //40
    uint32_t vAcc;          //44
    int32_t velN;           //48
    int32_t velE;           //52
    int32_t velD;           //56
    int32_t gSpeed;         //60
    int32_t headMot;        //64
    uint32_t sAcc;          //68
    uint32_t headAcc;       //72
    uint16_t pDop;          //76
    uint8_t reserved1[6];   //78
    int32_t headVeh;        //84
    int16_t magDec;         //88
    uint16_t magAcc;        //90
}stNavPvt;


#if true
stNavPvt m_stPreNavPvtMessage;
extern double GetDistance2(double lat1, double lon1, double lat2, double lon2);
#endif

void UbloxProtocol_Nav(char* carrBuff,int32_t nLength)
{
    uint8_t cMsgid = carrBuff[3];
    uint16_t usSize = carrBuff[4]|carrBuff[5]<<8;

    if( nLength != UBLOX_HEADER_LENGTH+UBLOX_CHECKSUM_LENGTH+usSize )
    {
        Trace("ublox packet is broken, drop this packet\r\n");
        return;
    }

    if( CheckUbloxCheckSum(carrBuff,nLength) == false )
    {
        Trace("ublox packet checksum is broken, drop this packet\r\n");
        return;
    }

    if( cMsgid == eUbxNavStat )
    {
        // nav status
        if( usSize ==  0x10 )
        {
            stNavStatus stNavStat;
            memcpy((char*)&stNavStat,&carrBuff[UBLOX_HEADER_LENGTH],sizeof(stNavStatus));
#if false
            Trace("iTow : %x, gpsFix : %x, flags : %x, fixStat : %d, flags2 : %d\r\n",stNavStat.iTow,
                stNavStat.gpsFix,
                stNavStat.flags,
                stNavStat.fixStat,
                stNavStat.flags2);
#endif

            //Trace("=================================================\r\n");
            //Trace("ttff : %d, msss : %d\n\n",stNavStat.ttff,stNavStat.msss);
        }
    }
    else if( cMsgid == eUbxNavPvt )
    {
        // PVT message
        stNavPvt stNavPvtMessage;
        nmeaTIME utc;

        memcpy((char*)&stNavPvtMessage,&carrBuff[UBLOX_HEADER_LENGTH],sizeof(stNavPvt));

#if false
		Trace("====================================================\r\n");
        Trace("UBLOX: Y:%d,M:%d,D:%d,h:%d,m:%d,s:%d\r\n",
            stNavPvtMessage.year,stNavPvtMessage.month,stNavPvtMessage.day,
            stNavPvtMessage.hour,stNavPvtMessage.min,stNavPvtMessage.sec);

        Trace("UBLOX: fixType:%d,svUsedNum:%d,lat:%f,lon:%f\r\n",
            stNavPvtMessage.fixType,stNavPvtMessage.numSV,
            (float)stNavPvtMessage.lat/10000000.0,(float)stNavPvtMessage.lon/10000000.0);

        Trace("LIB:  valid:%d,svViewNum:%d,svUsedNum:%d,lat:%f,lon:%f\r\n\r\n",Get_GPS_Vailication(),
            Get_GPS_SatellitesNum(),
            Get_GPS_SatellitesUsedNum(),
            Get_GPS_Lat(),
            Get_GPS_Lon());
#endif
        utc.year = stNavPvtMessage.year;
        utc.mon = stNavPvtMessage.month;
        utc.day = stNavPvtMessage.day;
        utc.hour = stNavPvtMessage.hour;
        utc.min = stNavPvtMessage.min;
        utc.sec = stNavPvtMessage.sec;

        //Trace("ublox#1 : ft : %d, lat : %f, lon : %f\r\n",stNavPvtMessage.fixType,stNavPvtMessage.lat/10000000.0,stNavPvtMessage.lon/10000000.0);

        // check exceptional 3 case
        // 1. distance // 2. date // 3. satelite count
        if( stNavPvtMessage.fixType >= NMEA_FIX_3D )
        {
#if true
            if( stNavPvtMessage.lat != 0 && stNavPvtMessage.lon != 0 && m_stPreNavPvtMessage.lat != 0 && m_stPreNavPvtMessage.lon != 0 )
            {
                double distance = GetDistance2((double)stNavPvtMessage.lat/10000000.0,(double)stNavPvtMessage.lon/10000000.0,
                                            (double)m_stPreNavPvtMessage.lat/10000000.0,(double)m_stPreNavPvtMessage.lon/10000000.0);
                if( distance > 100.0 )
                {
                    Trace("gps coordinate weired is over 100m per 1 seconds\r\n");
                    Trace("Distance : %d\r\n",distance);
                    stNavPvtMessage.fixType = NMEA_FIX_NO_FIX;
                    stNavPvtMessage.lat = 0;
                    stNavPvtMessage.lon = 0;
                    // ME will set after monitoring gps data
                    //stNavPvtMessage.numSV = 0;
                }
            }
#endif
            if( utc.year > 2050 )
            {
                Trace("gps uts year weired is over 2050\r\n");
                Trace("year : %d\r\n",utc.year);
                stNavPvtMessage.fixType = NMEA_FIX_NO_FIX;

                stNavPvtMessage.lat = 0;
                stNavPvtMessage.lon = 0;
                // ME will set after monitoring gps data
                //stNavPvtMessage.numSV = 0;
            }

            if( stNavPvtMessage.numSV < 3 )
            {
                Trace("ublox weired status because ublox is 3d fixed but satelite count is under 3\r\n");
                Trace("SN num : %d\r\n",stNavPvtMessage.numSV);
                stNavPvtMessage.fixType = NMEA_FIX_NO_FIX;

                stNavPvtMessage.lat = 0;
                stNavPvtMessage.lon = 0;
                // ME will set after monitoring gps data
                //stNavPvtMessage.numSV = 0;
            }
        }
        else
        {
            stNavPvtMessage.lat = 0;
            stNavPvtMessage.lon = 0;
            // ME will set after monitoring gps data
            //stNavPvtMessage.numSV = 0;
        }

        //Trace("ublox#2 : ft : %d, lat : %f, lon : %f\r\n",stNavPvtMessage.fixType,stNavPvtMessage.lat/10000000.0,stNavPvtMessage.lon/10000000.0);

        //if( stNavPvtMessage.fixType >= NMEA_FIX_3D )
        {
            // new gps info from ublox
            SetNewGpsInfo(stNavPvtMessage.lat,stNavPvtMessage.lon,stNavPvtMessage.fixType,stNavPvtMessage.numSV,utc,stNavPvtMessage.headMot);
        }

        memcpy((char*)&m_stPreNavPvtMessage,(char*)&stNavPvtMessage,sizeof(stNavPvt));

        /*
        gnssId GNSS
        0 GPS
        1 SBAS
        2 Galileo
        3 BeiDou
        4 IMES
        5 QZSS
        6 GLONASS

        0: no orbit information is available for this SV
        1: ephemeris is used
        2: almanac is used
        3: AssistNow Offline orbit is used
        4: AssistNow Autonomous orbit is used
        5, 6, 7: other orbit information is used


        GNSSfix Type:
        0: no fix
        1: dead reckoning only
        2: 2D-fix
        3: 3D-fix
        4: GNSS + dead reckoning combined
        5: time only fix
        */
    }
    else if( cMsgid == eUbxNavSat )
    {
        // satellite inforamtion
		 if( usSize == 0x08 )
        {
            //default value there is no satellite
        }
        else
        {
            stSatellite stNav5;
            stSatelliteContents stNav5Contents;

            memcpy((char*)&stNav5Contents,&carrBuff[UBLOX_HEADER_LENGTH],sizeof(stSatelliteContents));
//            Trace("=============================================================\r\n");
//            Trace("iTow : %d, ver : %d, numSv : %d\r\n",stNav5Contents.iTow,
//                stNav5Contents.version,stNav5Contents.numSvs);

            for(int i=0;i<stNav5Contents.numSvs;i++)
            {
                if( nLength < UBLOX_HEADER_LENGTH+sizeof(stSatelliteContents)+(i*12) )
                {
                    Trace("ublox send less size data\r\n");
                    break;
                }
                memcpy((char*)&stNav5,&carrBuff[UBLOX_HEADER_LENGTH+sizeof(stSatelliteContents)+(i*12)],sizeof(stSatellite));
                g_GPSInfo.satinfo.sat[i].id = stNav5.svId;
                g_GPSInfo.satinfo.sat[i].sig = stNav5.cno;

#if false
                Trace("gnssId : %x,svId : %x, cno : %x, elev : %x, azim : %x, prRes : %x, flag : %x\r\n",
                    stNav5.gnssId,
                    stNav5.svId,
                    stNav5.cno,
                    stNav5.elev,
                    stNav5.azim,
                    stNav5.prRes,
                    stNav5.flag);
                Trace("flag : %02x%02x%02x%02x\r\n",stNav5.flag>>24&0xFF,stNav5.flag>>16&0xFF,
                    stNav5.flag>>8&0xFF,stNav5.flag&0xFF);
                Trace("ANO Type : %d\r\n\r\n",stNav5.flag>>8&0x7);
#endif
            }
        }
    }
}

#if defined(USE_UBLOX_GPS)

void SetRestUBlox()
{
    unsigned char arrSendData [] = { 	0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, //header
										0xFF, 0xFF, 0x02, 0x00, 0x0E, 0x61};//check sum
										//0xFF, 0xFF, 0x01, 0x00, 0x0D, 0x60};//check sum

	SendGITPtclFrame((char*)arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]), eCOMM_TYPE_UART_GPS, NULL, 0);

    //hexdump(arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]));

}

void SetUBX_NAVX5(void)
{
	unsigned char arrSendData [] = { 	0xB5, 0x62, 0x06, 0x23, 0x28, 0x00, //header
                                        0x02, 0x00,                         //0
                                        0x4C, 0x66,                         //2
										0xC0, 0x00, 0x00, 0x00,             //4
										0x00, 0x00,                         //8
										0x03,                               //10
										0x20,                               //11
										0x0C,                               //12
										0x00,                               //13
										0x00,                               //14
										0x00, 0x00,                         //15
										0x01,                               //17 // mga ack
										0x4B, 0x07,                         //18
										0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //20
										0x00,                               //26
										0x00,                               //27
										0x00, 0x00,                         //28
										0x64, 0x00,                         //30
										0x00, 0x00, 0x00, 0x00,             //32
										0x00, 0x00, 0x00,                   //36
										0x00,                               //39
										0xAB, 0x1D };                       //check sum
	// Response packet :	B5 62 05 01 02 00 06 23 31 5A
	SendGITPtclFrame((char*)arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]), eCOMM_TYPE_UART_GPS, NULL, 0);

    //hexdump(arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]));
}

//void SetUBX_NAV5(void)
//{
//	unsigned char arrSendData [] = { 	0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, //header
//	                                    0xFF, 0xFF,                         //0
//	                                    0x04,                               //2
//	                                    0x03,                               //3
//		 								0x00, 0x00, 0x00, 0x00,             //4
//		 								0x10, 0x27, 0x00, 0x00,             //8
//		 								0x05,                               //12
//		 								0x00,                               //13
//										0xFA, 0x00,                         //14
//										0xFA, 0x00,                         //16
//										0x64, 0x00,                         //18
//										0x5E, 0x01,                         //20
//										0x64,                               //22
//										0x3C,                               //23
//										0x00,                               //24
//										0x00,                               //25
//										0x00, 0x00,                         //26
//										0x00, 0x00,                         //28
//										0x00,                               //30
//										0x00, 0x00, 0x00,0x00, 0x00,        //31
//										0xE6, 0x3C };                       //check sum
//
//	// Response packet B5 62 05 01 02 00 06 24 32 5B
//	SendGITPtclFrame(arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]), eCOMM_TYPE_UART_GPS, NULL, 0);
//
//    //hexdump(arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]));
//}

// 0.3ms로 세팅
void SetUBX_NAV5(void)
{
	unsigned char arrSendData [] = { 	0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, //header
	                                    0xFF, 0xFF,                         //0
	                                    0x04,                               //2
	                                    0x03,                               //3
		 								0x00, 0x00, 0x00, 0x00,             //4
		 								0x10, 0x27, 0x00, 0x00,             //8
		 								0x05,                               //12
		 								0x00,                               //13
										0xFA, 0x00,                         //14
										0xFA, 0x00,                         //16
										0x64, 0x00,                         //18
										0x2C, 0x01,                         //20
										0x1E,                               //22
										0x00,                               //23
										0x00,                               //24
										0x00,                               //25
										0x10, 0x27,                         //26
										0x00, 0x00,                         //28
										0x00,                               //30
										0x00, 0x00, 0x00,0x00, 0x00,        //31
										0x69, 0x3B };                       //check sum
	// B5 62 06 24 24 00 FF FF 04 03
	// 00 00 00 00 10 27 00 00 05 00
	// FA 00 FA 00 64 00 2C 01 1E 00
	// 00 00 10 27 00 00 00 00 00 00
	// 00 00 69 3B
	// Response packet B5 62 05 01 02 00 06 24 32 5B
	SendGITPtclFrame((char*)arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]), eCOMM_TYPE_UART_GPS, NULL, 0);

    //hexdump(arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]));
}

void SetAGPSEnd(void)
{
	unsigned char arrSendData [] = { 	0xB5, 0x62, 0x13, 0x21, 0x02, 0x00, //header
	                                    0x00, 0x00,                         //0
										0x36, 0x83 };                       //check sum

	// Response packet B5 62 05 01 02 00 06 24 32 5B
	SendGITPtclFrame((char*)arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]), eCOMM_TYPE_UART_GPS, NULL, 0);

    //hexdump(arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]));
}

void SetPollingSat(uint16_t usMsgid)
{
    char carrBuffer[256] = {0,};
    int nSize;
    uint16_t temp = eUBX_NAV_SAT;
    memcpy(carrBuffer,(char*)&temp,2);
    MakeUbloxPacket(usMsgid>>8&0xF,usMsgid&0xF,carrBuffer,(uint32_t*)&nSize);

    //hexdump(carrBuffer,nSize);

    SendGITPtclFrame((char*)carrBuffer, nSize, eCOMM_TYPE_UART_GPS, NULL, 0);
}


void SetUbxMgaIni()
{
	unsigned char arrSendData [] = { 	0xB5, 0x62, 0x13, 0x40, 0x18, 0x00, //header
	                                    0x10, 0x00, 0x00, 0x80, 0xE2, 0x07,
	                                    0x05, 0x1D, 0x00, 0x0D, 0x23, 0x00,
	                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	                                    0x00, 0x00, 0x00, 0x65, 0xCD, 0x1D,
										0x85, 0x4B };                       //check sum

	// Response packet B5 62 05 01 02 00 06 24 32 5B
	SendGITPtclFrame((char*)arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]), eCOMM_TYPE_UART_GPS, NULL, 0);

    //hexdump(arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]));
}

void SetTimePolling(uint16_t usMsgid)
{
    char carrBuffer[256] = {0,};
    int nSize;

    uint16_t temp = eUBX_NAV_PVT;
    memcpy(carrBuffer,(char*)&temp,2);
    MakeUbloxPacket(usMsgid>>8&0xF,usMsgid&0xF,carrBuffer,(unsigned int*)&nSize);

    //hexdump(carrBuffer,nSize);

    SendGITPtclFrame((char*)carrBuffer, nSize, eCOMM_TYPE_UART_GPS, NULL, 0);
}

void SetUbxNavPvt()
{
    unsigned char arrSendData [] = { 	0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, //header
	                                    0x01, 0x07, 0x01,
	                                    0x13, 0x51};                       //check sum

	SendGITPtclFrame((char*)arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]), eCOMM_TYPE_UART_GPS, NULL, 0);

    //hexdump(arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]));
}

void SetUbxNavStatus()
{
    unsigned char arrSendData [] = { 	0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, //header
	                                    0x01, 0x03, 0x01,
	                                    0x0F, 0x49};                       //check sum

	SendGITPtclFrame((char*)arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]), eCOMM_TYPE_UART_GPS, NULL, 0);

    //hexdump(arrSendData, sizeof(arrSendData)/sizeof(arrSendData[0]));
}

void MakeUbloxHeader(uint8_t ucClassId, uint8_t ucId, stUbloxHeader* pstHeader)
{
    pstHeader->preamble1 = 0xb5;
    pstHeader->preamble2 = 0x62;
    pstHeader->classId = ucClassId;
    pstHeader->msgId = ucId;
}

int MakeUbloxPayload(uint8_t ucClassId, uint8_t ucMsgid, stUbloxHeader stHeader, char* pcarrBuff)
{
    char * p;
    char * q;

    switch(ucClassId)
    {
        case eUbloxClassId_Cfg:
        {
            uint16_t usSize = 0;
            uint16_t usReqId;

            switch(ucMsgid)
            {
                case 0x01: // msg
                    memcpy((char*)&usReqId,pcarrBuff,2);
                    stHeader.length = 3;
                    p = &pcarrBuff[sizeof(stUbloxHeader)+stHeader.length];
                    q = &pcarrBuff[2];

                    memcpy(pcarrBuff,(char*)&stHeader,sizeof(stUbloxHeader));
                    pcarrBuff[sizeof(stUbloxHeader)] = usReqId>>8;
                    pcarrBuff[sizeof(stUbloxHeader)+1] = usReqId&0xFF;
                    pcarrBuff[sizeof(stUbloxHeader)+2] = 0x01;

                    // Calculate Checksum
                    while(q<p)
                    {
                        *p     += *q++;
                        *(p+1) += *p;
                    }

                    usSize = (sizeof(stUbloxHeader)+stHeader.length+2);

                    break;
                case 0x35: // sat
                    break;
                case 0x30: // alm
                    break;
                case 0x02:
                    stHeader.length = 1;
                    p = &pcarrBuff[sizeof(stUbloxHeader)+stHeader.length];
                    q = &pcarrBuff[2];

                    memcpy(pcarrBuff,(char*)&stHeader,sizeof(stUbloxHeader));
                    pcarrBuff[sizeof(stUbloxHeader)] = 0x00;

                    // Calculate Checksum
                    while(q<p)
                    {
                        *p     += *q++;
                        *(p+1) += *p;
                    }

                    usSize = (sizeof(stUbloxHeader)+2+stHeader.length);
                    break;
            }

            return usSize;
        }
            break;
        case eUbloxClassId_Nav:
            break;
        case eUbloxClassId_Rxm:
        case eUbloxClassId_Inf:
        case eUbloxClassId_AckNak:
        case eUbloxClassId_Upd:
        case eUbloxClassId_Mon:
        case eUbloxClassId_Aid:
        case eUbloxClassId_Tim:
        case eUbloxClassId_Esf:
        case eUbloxClassId_Mga:
        case eUbloxClassId_Log:
            break;
        default:
            break;
    }

    return 0;
}

boolean_t CheckUbloxCheckSum(char* pcarrBuff, int nBufSize)
{
    stUbloxHeader stHeader;
    char * p;
    char * q;

    memcpy((char*)&stHeader,pcarrBuff,sizeof(stUbloxHeader));
    char cCheckSum1 = pcarrBuff[sizeof(stUbloxHeader)+stHeader.length];
    char cCheckSum2 = pcarrBuff[sizeof(stUbloxHeader)+stHeader.length+1];

    pcarrBuff[sizeof(stUbloxHeader)+stHeader.length] = 0;
    pcarrBuff[sizeof(stUbloxHeader)+stHeader.length+1]= 0;

    p = &pcarrBuff[sizeof(stUbloxHeader)+stHeader.length];
    q = &pcarrBuff[2];

    // Calculate Checksum
    while(q<p)
    {
        *p     += *q++;
        *(p+1) += *p;
    }

    //hexdump(pcarrBuff,nBufSize);
    //printf("checksum1 : %x, checksum1-1 : %x\n",cCheckSum1,cCheckSum2);
    //printf("checksum2 : %x, checksum2-1 : %x\n",pcarrBuff[sizeof(stUbloxHeader)+stHeader.length],pcarrBuff[sizeof(stUbloxHeader)+stHeader.length+1]);

    if( cCheckSum1 == pcarrBuff[sizeof(stUbloxHeader)+stHeader.length] &&
        cCheckSum2 == pcarrBuff[sizeof(stUbloxHeader)+stHeader.length+1] )
        return true;

    return false;
}


void MakeUbloxPacket(uint8_t ucClassId, uint8_t ucMsgId, char* parrDstBuffer, uint32_t* pnSrcSize)
{
    stUbloxHeader stHeader;

    switch(ucClassId)
    {
        case eUbloxClassId_Nav:
            MakeUbloxHeader(ucClassId,ucMsgId,&stHeader);
            *pnSrcSize = MakeUbloxPayload(ucClassId,ucMsgId,stHeader,parrDstBuffer);
            break;
        case eUbloxClassId_Rxm:
            break;
        case eUbloxClassId_Inf:
            break;
        case eUbloxClassId_AckNak:
            break;
        case eUbloxClassId_Cfg:
            MakeUbloxHeader(ucClassId,ucMsgId,&stHeader);
            *pnSrcSize = MakeUbloxPayload(ucClassId,ucMsgId,stHeader,parrDstBuffer);
            break;
        case eUbloxClassId_Upd:
            break;
        case eUbloxClassId_Mon:
            break;
        case eUbloxClassId_Aid:
            MakeUbloxHeader(ucClassId,ucMsgId,&stHeader);
            *pnSrcSize = MakeUbloxPayload(ucClassId,ucMsgId,stHeader,parrDstBuffer);
            break;
        case eUbloxClassId_Tim:
            break;
        case eUbloxClassId_Esf:
            break;
        case eUbloxClassId_Mga:
            break;
        case eUbloxClassId_Log:
            break;
        default:
            break;
    }
}

typedef __packed struct __stNmeaMessageDisableQuery
{
    char data[11];
}stNmeaMessageDisableQuery;

stNmeaMessageDisableQuery m_arrNmeaList[] = {
#if false
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0A,0x00,0x05,0x24},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0A,0x00,0x05,0x24},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0A,0x00,0x05,0x24},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0A,0x00,0x05,0x24},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0A,0x00,0x05,0x24},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0A,0x00,0x05,0x24},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0A,0x00,0x05,0x24},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x09,0x00,0x04,0x22},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x09,0x00,0x04,0x22},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x09,0x00,0x04,0x22},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x09,0x00,0x04,0x22},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x09,0x00,0x04,0x22},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x09,0x00,0x04,0x22},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x09,0x00,0x04,0x22},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x00,0x00,0xFB,0x10},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x00,0x00,0xFB,0x10},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x00,0x00,0xFB,0x10},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x00,0x00,0xFB,0x10},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x00,0x00,0xFB,0x10},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x00,0x00,0xFB,0x10},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x00,0x00,0xFB,0x10},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x01,0x00,0xFC,0x12},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x01,0x00,0xFC,0x12},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x01,0x00,0xFC,0x12},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x01,0x00,0xFC,0x12},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x01,0x00,0xFC,0x12},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x01,0x00,0xFC,0x12},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x01,0x00,0xFC,0x12},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0D,0x00,0x08,0x2A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0D,0x00,0x08,0x2A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0D,0x00,0x08,0x2A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0D,0x00,0x08,0x2A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0D,0x00,0x08,0x2A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0D,0x00,0x08,0x2A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0D,0x00,0x08,0x2A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x06,0x00,0x01,0x1C},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x06,0x00,0x01,0x1C},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x06,0x00,0x01,0x1C},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x06,0x00,0x01,0x1C},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x06,0x00,0x01,0x1C},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x06,0x00,0x01,0x1C},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x06,0x00,0x01,0x1C},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x02,0x00,0xFD,0x14},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x02,0x00,0xFD,0x14},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x02,0x00,0xFD,0x14},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x02,0x00,0xFD,0x14},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x02,0x00,0xFD,0x14},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x02,0x00,0xFD,0x14},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x02,0x00,0xFD,0x14},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x07,0x00,0x02,0x1E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x07,0x00,0x02,0x1E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x07,0x00,0x02,0x1E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x07,0x00,0x02,0x1E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x07,0x00,0x02,0x1E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x07,0x00,0x02,0x1E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x07,0x00,0x02,0x1E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFE,0x16},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFE,0x16},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFE,0x16},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFE,0x16},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFE,0x16},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFE,0x16},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFE,0x16},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFE,0x16},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFE,0x16},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x04,0x00,0xFF,0x18},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x04,0x00,0xFF,0x18},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x04,0x00,0xFF,0x18},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x04,0x00,0xFF,0x18},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x04,0x00,0xFF,0x18},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x04,0x00,0xFF,0x18},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x04,0x00,0xFF,0x18},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0F,0x00,0x0A,0x2E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0F,0x00,0x0A,0x2E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0F,0x00,0x0A,0x2E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0F,0x00,0x0A,0x2E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0F,0x00,0x0A,0x2E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0F,0x00,0x0A,0x2E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x0F,0x00,0x0A,0x2E},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x05,0x00,0x00,0x1A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x05,0x00,0x00,0x1A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x05,0x00,0x00,0x1A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x05,0x00,0x00,0x1A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x05,0x00,0x00,0x1A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x05,0x00,0x00,0x1A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x05,0x00,0x00,0x1A},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x08,0x00,0x03,0x20},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x08,0x00,0x03,0x20},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x08,0x00,0x03,0x20},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x08,0x00,0x03,0x20},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x08,0x00,0x03,0x20},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x08,0x00,0x03,0x20},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x08,0x00,0x03,0x20},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF1,0x00,0x00,0xFC,0x13},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF1,0x01,0x00,0xFD,0x15},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF1,0x03,0x00,0xFF,0x19},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF1,0x04,0x00,0x00,0x1B},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF1,0x05,0x00,0x01,0x1D},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF1,0x06,0x00,0x02,0x1F}
#else
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x00,0x00,0xFA,0x0F},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x01,0x00,0xFB,0x11},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x02,0x00,0xFC,0x13},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x03,0x00,0xFD,0x15},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x04,0x00,0xFE,0x17},
{0xB5,0x62,0x06,0x01,0x03,0x00,0xF0,0x05,0x00,0xFF,0x19},
#endif
};                       //check sum

void SetDiableNmea()
{
    // block nmea message
    m_unUbloxStates = eUbloxDisableNmea;
}

enum {
    eUbloxDisableNmeaInit = 0,
    eUbloxDisableNmeaReqNmeaReq,
    eUbloxDisableNmeaReqNmeaRsp,
    eUbloxDisableNmeaReqNmeaDone,
};

int32_t m_nNmeaMessageIndex = 0;
int m_nUbloxDisableState = eUbloxDisableNmeaInit;
unsigned long m_ulUbloxDsiableNmeaSentTime = 0;

boolean_t UbloxDisableNmea()
{
    int nResult;

    switch(m_nUbloxDisableState)
    {
        case eUbloxDisableNmeaInit:
            m_nNmeaMessageIndex = 0;
            m_nUbloxDisableState = eUbloxDisableNmeaReqNmeaReq;
            Trace("nmea block initailze done\r\n");
            break;
        case eUbloxDisableNmeaReqNmeaReq:
            m_ulUbloxDsiableNmeaSentTime = Get_Tmr();
            SendGITPtclFrame((char*)m_arrNmeaList[m_nNmeaMessageIndex++].data, sizeof(m_arrNmeaList[m_nNmeaMessageIndex].data), eCOMM_TYPE_UART_GPS, NULL, 0);
            //hexdump(m_arrNmeaList[m_nNmeaMessageIndex-1].data,sizeof(m_arrNmeaList[m_nNmeaMessageIndex-1].data));
            m_nUbloxDisableState = eUbloxDisableNmeaReqNmeaRsp;

            if( sizeof(m_arrNmeaList)/sizeof(stNmeaMessageDisableQuery) < m_nNmeaMessageIndex)
                m_nUbloxDisableState = eUbloxDisableNmeaReqNmeaDone;
            else
                m_nUbloxDisableState = eUbloxDisableNmeaReqNmeaRsp;

            break;
        case eUbloxDisableNmeaReqNmeaRsp:
            if( GetRequestUbloxProtcolResult(&nResult) == true )
            {
                m_nUbloxDisableState = eUbloxDisableNmeaReqNmeaDone;
            }
            else
            {
                if(Get_TmrDelta(Get_Tmr(), m_ulUbloxDsiableNmeaSentTime)>= MAX_UBLOX_SENT_WAIT_TIME)
                {
                    m_nUbloxDisableState = eUbloxDisableNmeaReqNmeaReq;
                }
            }
            break;
        case eUbloxDisableNmeaReqNmeaDone:
            return true;
            break;
    }

    return false;
}

#endif

void CheckExpiretAGPSData()
{
    uint32_t unUTCTime;
	static bool s_bOneTimeRunFlag=true;

	if( s_bOneTimeRunFlag == true )
	{
		unUTCTime = GetUTCTime();

		if( unUTCTime >= BkSram_SystemInfo.unAGPSExireDate )
		{
			//send event for download agps data
			Send2MngSysMsg(eMngSys,eReqAgps,eAgpsDownload,(stCarReport *)NULL,0);

			// update expire date
			BkSram_SystemInfo.unAGPSExireDate = unUTCTime + AGPS_EXPIRED_DATE;
			s_bOneTimeRunFlag=false;
		}
	}
}
