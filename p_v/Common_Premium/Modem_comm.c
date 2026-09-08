/**
  ******************************************************************************
  * @file    BluetoothLowEnery.h
  * @author  GIT Diagnosis Software Team by james jean
  * @version V1.1.0
  * @date    29-JAN-2016
  * @brief   Manager BluetoothLowEnery.c module
  ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/

#include "Modem_comm.h"
#include "Modem_Manager.h"
#include <time.h>
#include "Modem_comm.h"
#include "FOTA_Manager.h"
#include "Power_Manager.h"

#include "MngSystem.h"
#include "MngSystemUtil.h"
#include "MngQueue.h"
#include "MngModem.h"
#include "HdDebug.h"
#include "DebugHandler.h"
#include "Autolink_Manager.h"
#include "HalHandler.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_MODEM_COM,__VA_ARGS__)

#define GMT_KOREA_TIME_OFFSET				(9 * 3600)				// +9 시간

// these definition for aotolinkconfig
// PLMN  // nationality // service provider
// 50501 // australia   // telstra
// 50502 // autstralia  // optus
// 45008 // korea       // kt
// 45005 // korea       // sk
// 45006 // korea       // u+
// sample simi = 505013499250517

//	2018.05.03 SPARROW : 생산용 테스트유심용 추가
//#define MAX_PLMN_LIST 6
#define MAX_PLMN_LIST 20
#define MAX_PLMN_LENGTH 15
#define PLMN_COUNTRY_AU	"505"
#define PLMN_COUNTRY_SIZE	3

char* m_strPLMNListofUsim[MAX_PLMN_LIST]=
{
    "45005",//korea/sk
    "45006",//korea/u+
    "45008",//korea/kt
    "50501",//australia/telstra
    "50502",//australia/optus	//5
    "53005",//newzealand/spark
    "53006",//newzealand/spark
	"00101",//TestUsim :"001010123456789\r\n"
	"25099", //Russia/Beeline
	"20404", //SpainWIBLE_SK_VODA	//10
	"23450", //SpainWIBLE_KT
	"90128", //SpainWIBLE_SK_VODA
	"44010", //NTT docomo
	"44020", //SoftBank
	"44021", //SoftBank_2
	"24042", //Telenor
	"52501", // Singtel_Singapore
	"45202", //Vietnam_SK_VODA_Vinaphone
	"45204", //Vietnam_SK_VODA_	Viettel Mobile
    "24008"  //Telenor _Sweden_Singapore
};

char* m_strApnListofPLMN[MAX_PLMN_LIST]=
{
    "lte.sktelecom.com",
    "internet.lguplus.co.kr",
    "lte.ktfwing.com",
    "telstra.m2m",	//	"git.telstra.m2m",
    "yesinternet",	//5
    "m2m",//"wap.telecom.co.nz",
    "internet",
    "testusim",
    "m2m.beeline.ru",
    "internet4gd.gdsp",	//10
    "internet.cs101",
    "skt.iot.kr",
    "skt.iot.kr",
    "skt.iot.kr",
    "skt.iot.kr",
    "connect.cxn",
    "public.dcp.singtel.com",
    "skt.iot.kr",
    "skt.iot.kr",
    "connect.cxn" //20
};

#define AU_GIT_APN 	"git.telstra.m2m"

#if defined(USE_GPS_LGPL_PARSER)
#include <nmea/nmea.h>
#endif

eSmsRcvStatus m_eSmsRcvStatus = eSmsRcvStatusNone;

#define DEBUG_AMT_PTCL_LOG
#define GMT_KOREA_TIME_OFFSET				(9 * 3600)				// +9 시간

void SetModemControlEvent(int32_t nStatus);
void SetModemNetworkStatus(eNETWORK_REG_STATUS eNetworkStatus,int nRssi);

extern void SetRequestSms2Gemalto(boolean_t bRequest);
extern void SetReqWaitNetworkCheck(boolean_t bReqWaitNetworkCheck);
extern void SetNetworkUtcTime(stHalRTCTypeDef stDate);
extern eGitFresult WriteAgpsData(char* pstrFileName, char* pcarrBuff, int nSize);
extern void ClearNetworkDelayProcess();
extern uint32_t GetModemControlOldState();
extern uint8_t g_bDisconnectTestFlag;
extern boolean_t Md_SetFactorymode(void);
extern long long m_llStorageDrivingKey;

tbModemCmdList g_tbModemCmdList[] =
{
	// Cmd index		 		Request Cmd	 	Response Cmd1, 	Response Cmd2
	//*******************************************************************************
	// gemalto ELS61 modem command start
	//*******************************************************************************
#ifdef RF_COMMON_MODEM  //mod.kks 21.10.25
	{eCmd_SetEcho,							"ATE",	 		"OK", 						MDResOK},
	{eCmd_SetResult,						"ATQ",	 		"OK",						MDResOK},
	{eCmd_GetSysInfo,						"ATI1",	 		"OK", 						MDResOK},
	{eCmd_FactoryMode,						"AT+CFUN=5", 	"OK", 						MDResOK},
#else
	{eCmd_SetEcho,							"ATE",	 		"ATE", 						MDSetEchoRes},
	{eCmd_SetResult,						"ATQ",	 		"ATQ",						MDSetResultRes},
	{eCmd_GetSysInfo,						"ATI1",	 		"ATI", 						MDGetSystemInfoRes},
#endif
	{eCmd_SetFlowControl, 					"AT\\Q3",	 	"OK", 						MDResOK},
	{eCmd_GetIMEI,							"AT+CGSN",	 	"OK",						MDResOK},
	{eCmd_GetDateTime,						"AT+CCLK?",		"+CCLK:", 					MDResGetDateTime},
	{eCmd_SetDateTime,						"AT+CCLK=",		"OK", 						MDResOK},
#ifdef RF_COMMON_MODEM //mod.kks 21.10.26
	{eCmd_GetCCIDInfo,						"AT+CCID",		"+CCID", 					MDGetCCIDInfo},
#else
	{eCmd_GetCCIDInfo,						"AT+CCID?",		"+CCID", 					MDGetCCIDInfo},
#endif
	{eCmd_GetPhoneNo,						"AT+CNUM",		"+CNUM", 					MDGetPhoneNo},
	{eCmd_SetBaudrate,						"AT+IPR=",		"OK", 						MDResOK},
	{eCmd_SetErrorMsgFormat,				"AT+CMEE=",		"OK", 						MDResOK},
	{eCmd_SetInternetConnectionProfile,		"AT^SICS=",		"OK", 						MDResOK},
	{eCmd_SetInternetServiceProfile, 		"AT^SISS=",		"OK", 						MDResOK},
	{eCmd_SetInternetSocketOpen, 			"AT^SISO=",		"OK", 						MDResOK},
	{eCmd_SetInternetSocketClose,			"AT^SISC=",		"OK", 						MDResOK},
	{eCmd_SetInternetSocketWriteLen,		"\r\nAT^SISW=",	"OK", 						MDResOK},
//	{eCmd_SetInternetSocketWriteLen,		"AT^SISW=",		"OK", 						MDResOK},
	{eCmd_InternetSocketRead,				"AT^SISR=",		"OK", 						MDResOK},
	{eCmd_CheckNetrorkRegistration,			"AT+CREG",		"+CREG:", 					MDResNetworkRegistration},
	{eCmd_CheckPacketDomainRegistration,	"AT+CGREG",		"+CGREG:", 					MDResNetworkRegistration},
	{eCmd_CheckSignalQuality,				"AT+CSQ",		"+CSQ:", 					MDResSignalQuality},
	{eCmd_ReadOperatorSelection,			"AT+COPS",		"+COPS:", 					MDResReadOperatorSelection},
	{eCmd_SetSMSFormat,						"AT+CMGF=",		"OK", 						MDResOK},
	{eCmd_ReadSMSIndex,						"AT+CMGL=",		"+CMGL", 					MDResReadSMSindex},
	{eCmd_DeleteSMSIndex,					"AT+CMGD=",		"OK", 						MDResOK},
#ifndef RF_COMMON_MODEM
	{eCmd_SendSMS,							"AT+CMGS=",		">", 						MDResReadySendSMS},	//미사용 로직 삭제 AGPS 데이터중 ">" 가 존재하여 문제 발생
#endif
	{eCmd_GetSIND,							"AT^SIND=",		"^SIND:",					MDResGetDateTime},
	{eCmd_SetSMSReportCnfg,					"AT+CNMI=",		"OK", 						MDResOK},
	{eCmd_SetModemConfigExt,				"AT^SCFG=",		"OK", 						MDResOK},
	{eCmd_AIRPLANE_ON,						"AT+CFUN=4",	"+CFUN",					MDResCFUN},
	{eCmd_AIRPLANE_OFF,						"AT+CFUN=1", 	"+CFUN", 					MDResCFUN},
	{eCmd_CPMS,								"AT+CPMS=",  	"+CPMS", 					MDResCPMS},
	{eCmd_CGATT,							"AT+CGATT",	    "OK", 						MDResGprs},
	{eCmd_CFUN,								"AT+CFUN=",  	"OK", 						MDResOK},
	{eCmd_COPS,								"AT+COPS=",		"OK", 						MDResOK},
	{eCmd_SPOW,								"AT^SPOW=",		"OK", 						MDResOK},
	{eCmd_SCPIN,							"AT^SCPIN=",	"OK", 						MDResOK},
	{eCmd_SSIO,								"AT^SSIO=",		"OK", 						MDResOK},
	{eCmd_CheckInternetServiceInfo, 		"AT^SISI?",		"^SISI", 					MDResSISI},
	{eCmd_CheckInternetConnectionInfo,		"AT^SICI?",		"^SICI", 					MDResSICI},
	{eCmd_SetModemPowerMax,					"AT^SCFG=",		"OK", 						MDResOK},
	{eCmd_GetIMSI,                          "AT+CIMI",      "OK",                       MDResIMSI},
    {eCmd_CheckAirplane,                    "AT+CFUN",      "OK",                       MDResOK},
    {eCmd_SetPowerOff,                      "AT^SMSO",      "OK",                       MDResOK},
	{eCmd_SetApn,                           "AT+CGDCONT=",  "OK",                       MDResOK},
	{eCmd_SetCtzr,                          "AT+CTZR=1",    "OK",                       MDResOK},
	{eCmd_SetRegistPhoneNumMode,			"AT+CPBS=",		"OK",						MDResOK},
	{eCmd_SetRegistPhoneNumber,				"AT+CPBW",		"+CPBW:",					MDResCPBW},

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
	{eCmd_CheckUSIMIN,						"AT+CPIN?",		"+CPIN:",					MDResCPIN},
	{eCmd_SetCTZU,							"AT+CTZU=1",	"OK",						MDResOK},
	{eCmd_ConActive,						"AT^SICA=",  	"OK", 						MDResOK},
	{eCmd_SetGNSS,							"AT^SGPSC=",	"^SGPSC:",					MDResSGPSC},
	{eCmd_SetReport,						"AT^SISE=",	    "OK",				    	MDResOK},
	{eCmd_SetGPIODriver,					"AT^SPIO=",	    "OK",				    	MDResOK},
#endif
	{eCmd_SetRadioAccess,					"AT^SXRAT=",	"OK",						MDResOK},
	{eCmd_SetNetworkAutoResponse,			"AT+CGAUTO=",	"OK",						MDResOK},
	{eCmd_Monitoring_ServingCell,			"AT^SMONI",		"^SMONI",					MDResMonitoringService},
	{eCmd_ExtendConfigSetting,				"AT^SCFG",		"^SCFG:",	 				MDRecvSCFG},
	{eCmd_Show_PDP_address,					"AT+CGPADDR",	"+CGPADDR",	 				MDRecvCGPADDR},
#ifdef RF_COMMON_MODEM //THALES_AGPS
	{eCmd_SBNW_Send_Start, 					"AT^SBNW=",		"AGPS READY: SEND FILE", 					MDRecvSBNW},
	{eCmd_SBNW_Delete, 						"AT^SBNW=agps,-1",		"AGPS: END OK", 					MDResAGPSEndOK},
	{eCmd_GNSS_Evt_Noti, 					"AT^SGPSE=1",		"OK", 					MDResOK},
//  for error test
//	{eCmd_SBNW_Send_Start, 					"AT^SBNW=",		"CONNECT", 					MDRecvSBNW},
//	{eCmd_SBNW_Delete, 						"AT^SBNW=agps,-1",		"CONNECT", 					MDResAGPSEndOK},
#endif	//RF_COMMON_MODEM //THALES_AGPS

#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
	{eCmd_None,															NULL,					"^SYSLOADING",		MDRecvSystemLoading},
#endif
	{eCmd_None,															NULL,					"^SYSSTART",	 	MDRecvSystemStart},
	{eCmd_None,															NULL,					"^SYSINFO",	 		MDRecvSystemInfo},
	{eCmd_None,															NULL,					"^SHUTDOWN",	 	MDRecvShutDown},
	{eCmd_None,															NULL,					"+PBREADY",	 		MDRecvPBReady},
	{eCmd_None,															NULL,					"+CMGS:", 			MDResSendSMSstate},
	{eCmd_None,															NULL,					"^SISW:",	 		MDRecvReadyWrite},
	{eCmd_None,															NULL,					"^SISR:",	 		MDRecvSocketDataReadAvailable2},
	{eCmd_None,															NULL,					"^SBC:",	 		MDRecvSBC},
	{eCmd_None,															NULL,					"^SIS:",	 		MDRecvSIS},
	{eCmd_None,															NULL,					"+CMTI:",	 		MDRecvCMTI},
	{eCmd_None,															NULL,					"^SCFG:",	 		MDRecvSCFG},
	{eCmd_None,															NULL,					"OK",	 			MDResOK},
	{eCmd_None,															NULL,					"ERROR",	 		MDResERROR},
	{eCmd_None,															NULL,					"+CME ERROR:",		MDResCMEERROR},
	{eCmd_None,															NULL,					"+CMS ERROR:",		MDResCMSERROR},
	{eCmd_None,															NULL,					"NO CARRIOR",	 	MDResNOCarrier},
    {eCmd_None,															NULL,					"+CTZV:",	 	    MDRecvCTZV},
    {eCmd_None,															NULL,					"+NITZINFO:",	 	MDRecvNITZInfo},
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
    {eCmd_None,															NULL,					"+CIEV:",		 	MDRecvprov},
    {eCmd_None,															NULL,					"+CTZU:",		 	MDRecvCTZU},
    {eCmd_None,															NULL,					"^SISO:",		 	MDRecvSISO},
    {eCmd_None,															NULL,					"+CGPADDR:",		MDRecvCGPADDR},
    {eCmd_None,															NULL,					"^SMONI:",		 	MDRecvSMONI},
    {eCmd_None,															NULL,					"AGPS READY:",	 	MDResOK},
    {eCmd_None,															NULL,					"AGPS: END OK",		MDResAGPSEndOK},
    {eCmd_None,															NULL,					"AGPS: GENERAL FAILURE",MDResERROR},
    {eCmd_None,															NULL,					"AGPS: I/O ERROR",  MDResERROR},
    {eCmd_None,															NULL,					"AGPS: UNSUPPORTED",MDResERROR},
#endif
	//*******************************************************************************
	// gemalto ELS61 modem command end
	//*******************************************************************************
};

stMODEM_REP g_stModem_Rep;

uint8_t g_aModemNetworkConnTimeString[36];
uint16_t g_nCurrRcvSMSIndex;
boolean_t gbStartInterceptModemRcvData;
boolean_t gbEnableShowRSSI;
boolean_t g_bCompleteDataFlag=false;
unsigned int g_uiAGPSFilesize=0;
char g_ucSMONIData[100];

extern uint8_t g_arrXXXCompareString[64];
extern uint16_t g_nXXXCompareStringLength;

int32_t ModemRecvGetLine(stQueue *pQueue, uint32_t *pLineLength)
{
	int32_t bRet = 0; 	// -1 : no receive, 0 : recv packet, 1 : recv packet & found \r\n
	uint32_t uiReceivedPacket=0;
	int32_t i=0;
	uint8_t ucTmp=0;

	uiReceivedPacket = GetQueueDataLength(pQueue);
#ifndef RF_COMMON_MODEM
	if(g_stModem_Rep.cmd_index == eCmd_SendSMS) {
		if ( uiReceivedPacket < MODEM_UART_PACKET_MIN_SIZE ) {
			return 0;
		}

		if ( uiReceivedPacket == 2) {
			i = 1;
			if(pQueue->pData[((pQueue->uiFront + i - 1 )) % pQueue->uiQueueSize] == '>' && pQueue->pData[((pQueue->uiFront + i - 0)) % pQueue->uiQueueSize] == ' ') {
				bRet = 1;
				i = 2;

				*pLineLength = i;

				Trace("1.%d\n", uiReceivedPacket);

				return bRet;
			}
		}
	}
	else {
#endif
		if ( uiReceivedPacket <= MODEM_UART_PACKET_MIN_SIZE ) {				// MODEM_UART_PACKET_MIN_SIZE --> 2
			return 0;
		}
#ifndef RF_COMMON_MODEM
	}
#endif

	if(gbStartInterceptModemRcvData == false) {
		for(i = 1; i < uiReceivedPacket; i++) {
			if( (pQueue->pData[((pQueue->uiFront + i - 1 )) % pQueue->uiQueueSize] == 0x0D) && (pQueue->pData[((pQueue->uiFront + i)) % pQueue->uiQueueSize] ==  0x0A)) {
				if(i == 1) { //패킷 앞에 \r\n은 버려야 한다.
					// ex) <CR><LF>$GPGGA,,,,,,중략,,,<CR><CR><LF>$GPVTG,,,,중략,,,<CR><CR><LF><CR><LF><CR><LF>OK<CR><LF>
					PopQueue(pQueue, &ucTmp); // <CR>제거
					PopQueue(pQueue, &ucTmp); // <LF>제거

					// <CR><LF>제거후 다시 패킷을 찾는다.
					uiReceivedPacket = GetQueueDataLength(pQueue);
					i = 0;
					continue;
				}
				else {
					bRet = 1;
					i += 1; // <LF> size

					break;
				}
			}
		}
	}
	else {
//		Trace("intercept %d:%d\n", uiReceivedPacket, ModemManagerData.nAvailableReadDataLength);
		if(uiReceivedPacket >= ModemManagerData.nAvailableReadDataLength) {
			i = ModemManagerData.nAvailableReadDataLength;
//			Trace("intercept rcv: %d\n", i);
			bRet = 1;
		}
		else {
			bRet = 0;
		}
	}

	*pLineLength = i;

	return bRet;
}

boolean_t IsGemaltoAvailable()
{
    return g_stModem_Rep.start_rep;
}

boolean_t Md_Make_n_SendPacket(eCmdIndex CmdIdx, uint8_t* CmdData, uint32_t nCmdDataSize)
{
	int8_t* strAmtCmd;
	stAMT_PTCL_PAYLOAD  stModemInPtcl;

	HalTimerStartSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly, eSWTimer_ONESHOT);

	g_stModem_Rep.start_rep = true;
	g_stModem_Rep.cmd_index = CmdIdx;

    Trace(" cmd index: %d, [%s]\r\n", g_stModem_Rep.cmd_index, g_tbModemCmdList[g_stModem_Rep.cmd_index].strSendDataReq);
	g_stModem_Rep.eMDResponse = eMDResponseStateWait;
	g_stModem_Rep.nResponseLineCount = 0;


    memset(stModemInPtcl.pPayload,0,MAX_MODEM_PROTO_DATA_LENGTH);


	stModemInPtcl.DataLength = 0;
	strAmtCmd = g_tbModemCmdList[CmdIdx].strSendDataReq;
	sprintf((char*)stModemInPtcl.pPayload,"%s",strAmtCmd);
	stModemInPtcl.DataLength = strlen((char*)stModemInPtcl.pPayload);

    if ( CmdData != NULL && nCmdDataSize > 0 )
    {
    	memcpy(&stModemInPtcl.pPayload[strlen((char*)stModemInPtcl.pPayload)], CmdData, nCmdDataSize);
        stModemInPtcl.DataLength += nCmdDataSize;
    }

    stModemInPtcl.pPayload[stModemInPtcl.DataLength]= 0x00;
//     Trace(" cmd index: %d, [%s]\r\n", g_stModem_Rep.cmd_index, stModemInPtcl.pPayload);

	stModemInPtcl.DataLength += 2;
	stModemInPtcl.pPayload[stModemInPtcl.DataLength - 2] = 0x0D;	// CR
	stModemInPtcl.pPayload[stModemInPtcl.DataLength - 1] = 0x0A;	// LF
	stModemInPtcl.pPayload[stModemInPtcl.DataLength]= 0x00;	// NULL

	OemWriteUartModemBuff(stModemInPtcl.pPayload, stModemInPtcl.DataLength, NULL, eCOMM_TYPE_UART_MODEM);
    //hexdump(stModemInPtcl.pPayload,stModemInPtcl.DataLength);

	return true;
}

void Modem_ResponsePrc(BYTE* pData, uint32_t nLength)
{
	int32_t i, nloopCnt;

	if(gbStartInterceptModemRcvData == false) {
		nloopCnt = sizeof(g_tbModemCmdList) / sizeof(g_tbModemCmdList[0]);

		for(i = 0; i < nloopCnt; i++ ) {
			if(memcmp((char *)pData, (char *)g_tbModemCmdList[i].strResponseData, strlen((char *)g_tbModemCmdList[i].strResponseData)) == 0 ) {
				if ( g_tbModemCmdList[i].fp != NULL ) {
	//				Trace("*comm name: [%s]\n", g_tbModemCmdList[i].strResponseData);
					g_tbModemCmdList[i].fp(pData, nLength);

					//mod.pdh 2022.05.16 to do check modem state
					if(g_bYUJINSelftestFlag != true)
					{
						HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Recieved_TimeoutDly, MDM_NORECIEVE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoRecieve_Callback, true);
					}
					
					return;
				}
			}
		}

		MDContinueResProc(pData, nLength);
	}
	else {
		//mod.pdh 2022.05.18 to do check modem state
		if(g_stModem_Rep.cmd_index == eCmd_InternetSocketRead)
		{
			HalTimerChangeSWTimer(ModemManagerData.iTimer_MDM_Recieved_TimeoutDly, MDM_NORECIEVE_TIMEOUT, eSWTimer_ONESHOT, MDM_NoRecieve_Callback, true);
		}
		MDContinueResProc(pData, nLength);
		gbStartInterceptModemRcvData = false;				// ������ ���������� ������ �޴´�.
	}

}

void FindContryCode(uint8_t* pData,int size)
{
    int bFindApn = false;
	char * carrSeriaNumber = GetFWSerialNumber();

    if( size == MAX_PLMN_LENGTH )
    {
        for(int i=0;i<MAX_PLMN_LIST;i++)
        {
            if( memcmp(pData,m_strPLMNListofUsim[i],5) == 0 )
            {
                // find country code // set apn from list
                Trace("Find APN set APN : %s, PLMN : %s\n",m_strApnListofPLMN[i], m_strPLMNListofUsim[i]);
				if( memcmp(PLMN_COUNTRY_AU,m_strPLMNListofUsim[i],PLMN_COUNTRY_SIZE) == 0 )	//호주일경우만
				{
					if( g_FirmwareInfo.m_ucApnFlag == DCS_GIT_APN )			SetBackupRamConfigProperty(eBackupRamConfig_Apn, AU_GIT_APN );
					else															                	SetBackupRamConfigProperty(eBackupRamConfig_Apn, (void*)m_strApnListofPLMN[i]);
				}
				else
				{
					SetBackupRamConfigProperty(eBackupRamConfig_Apn, (void*)m_strApnListofPLMN[i]);
				}

                bFindApn = true;
                break;
            }
        }
    }

    if( bFindApn == false )
    {
		if( strstr(carrSeriaNumber,"JPH") != NULL )
		{
			Trace("Not Find APN set Default APN : %s\r\n",SKT_ROAMING_APN_URL);
        	SetBackupRamConfigProperty(eBackupRamConfig_Apn, (void*)SKT_ROAMING_APN_URL);
		}
		else if( strstr(carrSeriaNumber,"VNK") != NULL)
		{
			Trace("Not Find APN set Default APN : %s\r\n",SKT_ROAMING_APN_URL_2);
        	SetBackupRamConfigProperty(eBackupRamConfig_Apn, (void*)SKT_ROAMING_APN_URL_2);
		}
		else
		{
			Trace("Not Find APN set Default APN : %s\r\n",DEFAULT_APN_URL);
        	SetBackupRamConfigProperty(eBackupRamConfig_Apn, (void*)TELSTRA_APN_URL);
		}

        Trace("PLMN : %c%c%c%c%c\r\n",pData[0],pData[1],pData[2],pData[3],pData[4]);
    }
}

void MDContinueResProc(BYTE* pData, uint32_t nLength)
{
	uint16_t i=0;

//	hexdump(pData, nLength);

	switch(g_stModem_Rep.cmd_index) {
        case eCmd_CheckAirplane:
            break;
		case eCmd_SetEcho:
			break;
		case eCmd_SetResult:
			break;
		case eCmd_GetSysInfo:
			switch(g_stModem_Rep.nResponseLineCount) {
				case 0:
#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
					memcpy(ModemManagerData.aVendorName, pData, nLength - 2);
#endif
					break;
				case 1:
					memcpy(ModemManagerData.aProductName, pData, nLength - 2);
					break;
				case 2:
					memcpy(ModemManagerData.aRevision, pData, nLength - 2);
					break;
				case 3:
					memcpy(ModemManagerData.aARevision, pData, nLength - 2);
					break;
			}

			g_stModem_Rep.nResponseLineCount++;
			break;
		case eCmd_SetFlowControl:
			break;
		case eCmd_GetIMEI:
			memcpy(ModemManagerData.aIMEI, pData, nLength - 2);
			break;
        case eCmd_GetIMSI:
            {
                memcpy(ModemManagerData.aSIMI, pData, nLength - 2);

                FindContryCode(pData,nLength - 2);
            }
            break;
		case eCmd_InternetSocketRead:
//			Trace("rcv len: %d\r\n", nLength);
//			Trace(".");

//			if(nLength < 1500) {
//				Trace("\r\n");
//				Trace("ModemManagerData.nAvailableReadDataLength: %d\r\n", ModemManagerData.nAvailableReadDataLength);
//				hexdump(pData, nLength);
//			}

			// 응답 받는 data 내에 file 종료를 의미하는 "^SISR: <id>,2" 문자열이 포함되어 전송되는 문제를 해결하기 위해서, 해당 문자열을 data 내에서 검색하는 루틴을 만들어 두었다..
			{
				uint16_t x;
				uint8_t arrTempBuffer[1600];

				for(x = 0; x < nLength; x++) {
					// 테스트 결과
					// 데이터 내에서
					// URC message 인 <0x0d>, <0x0a> "^SISR: 1,2" <0x0d>, <0x0a> 가 검출되는 경우도 있고
					// <0x0d>, <0x0a> "^SISR: 1,2" 만 검출 되는 경우도 있다.
					// 추후에 또 무슨 경우가 있을 지 가늠하기 어렵다. 2018-02-23 오전 11:23:45

					if(memcmp((char *)&pData[x], (char *)g_arrXXXCompareString, g_nXXXCompareStringLength) == 0) {
						printf("MDM: !!! Found URC message in data\r\n");
						printf("[%s]\r\n", g_arrXXXCompareString);

						Trace("\r\n");
						Trace("ModemManagerData.nAvailableReadDataLength: %d\r\n", ModemManagerData.nAvailableReadDataLength);
						//hexdump(pData, nLength);

						ModemManagerData.bEndOfData = true;					// 수신 받을 데이터가 없다. file 끝.

						memcpy((char *)arrTempBuffer, (char *)pData, x);
						memcpy((char *)&arrTempBuffer[x], (char *)(&pData[x] + g_nXXXCompareStringLength), nLength - x - g_nXXXCompareStringLength);

//						hexdump(arrTempBuffer, nLength - g_nXXXCompareStringLength);

						nLength = nLength - g_nXXXCompareStringLength;
						memcpy((char *)pData, (char *)arrTempBuffer, nLength);
					}
					else if(memcmp((char *)&pData[x], (char *)g_arrXXXCompareString, g_nXXXCompareStringLength - 2) == 0) {
						g_nXXXCompareStringLength -= 2;

						printf("MDM: !!! Found URC message in data\r\n");
						printf("[%s]\r\n", g_arrXXXCompareString);

						Trace("\r\n");
						Trace("ModemManagerData.nAvailableReadDataLength: %d\r\n", ModemManagerData.nAvailableReadDataLength);
						//hexdump(pData, nLength);

						ModemManagerData.bEndOfData = true;					// 수신 받을 데이터가 없다. file 끝.

						memcpy((char *)arrTempBuffer, (char *)pData, x);
						memcpy((char *)&arrTempBuffer[x], (char *)(&pData[x] + g_nXXXCompareStringLength), nLength - x - g_nXXXCompareStringLength);

//						hexdump(arrTempBuffer, nLength - g_nXXXCompareStringLength);

						nLength = nLength - g_nXXXCompareStringLength;
						memcpy((char *)pData, (char *)arrTempBuffer, nLength);
					}
				}
			}

			if(ModemManagerData.nAvailableReadDataLength == nLength) {
//				Trace("<1.Read Socket Data:(%d)>\r\n", nLength);
				if(ModemManagerData.bModemDataSaveToFlashFlag == true) {
					FOTA_SaveBinDataToInternalFlash(pData, nLength);
					gwTotalReceiveBinDataLength += nLength;
				}
				else {
                    if( ModemManagerData.eCurrentMessageSendingType != eMESSAGE_APGS_DATA )
                    {
					    memcpy(&gaModemCommRxDataBuffer[gwModemCommRxDataLength], pData, nLength);
					    //gwModemCommRxDataLength += ModemManagerData.nAvailableReadDataLength;
					    gwModemCommRxDataLength += nLength;
						if( (gwModemCommRxDataLength > 2) && (gaModemCommRxDataBuffer[gwModemCommRxDataLength-1]=='}') && (gaModemCommRxDataBuffer[gwModemCommRxDataLength-2]=='}'))
						{
							g_bCompleteDataFlag = true;
						}
                    }
                    else
                    {
                        if( WriteAgpsData(AUTOLINK_AGPS_DATA,(char*)pData,nLength) != GIT_FR_OK )
                        {
                            GIT_Assert(false,eErrorCodeMdm|eWriteFail);
                            Trace("Error\r\n");
                        }
#ifdef RF_COMMON_MODEM
						else
						{
							printf("now:%d,tot:%d\r\n",nLength,g_uiAGPSFilesize);
							//hexdump(pData,nLength);
							g_uiAGPSFilesize += nLength;
							//printf("g_uiAGPSFilesize:%d\r\n",g_uiAGPSFilesize);
						}
#endif
                        //hexdump(pData,nLength);
                    }
				}

				ModemManagerData.nAvailableReadDataLength = 0;
			}
			else if(ModemManagerData.nAvailableReadDataLength == nLength - 2) {
				if(ModemManagerData.bModemDataSaveToFlashFlag == true) {
					FOTA_SaveBinDataToInternalFlash(pData, nLength - 2);
					gwTotalReceiveBinDataLength += (nLength - 2);
				}
				else {
                    if( ModemManagerData.eCurrentMessageSendingType != eMESSAGE_APGS_DATA )
                    {
    					memcpy(&gaModemCommRxDataBuffer[gwModemCommRxDataLength], pData, nLength - 2);
    					//gwModemCommRxDataLength += ModemManagerData.nAvailableReadDataLength;
    					gwModemCommRxDataLength += (nLength - 2);
						if( (gwModemCommRxDataLength > 2) && (gaModemCommRxDataBuffer[gwModemCommRxDataLength-1]=='}') && (gaModemCommRxDataBuffer[gwModemCommRxDataLength-2]=='}'))
						{
							g_bCompleteDataFlag = true;
						}
                    }
                    else
                    {
#ifdef RF_COMMON_MODEM //THALES_AGPS
              	        //printf("%d_%d_%d\r\n",nLength,nLength-2,ModemManagerData.nAvailableReadDataLength);
						if( (nLength-2) == ModemManagerData.nAvailableReadDataLength )
						{
							if( WriteAgpsData(AUTOLINK_AGPS_DATA,(char*)pData,nLength-2) != GIT_FR_OK )
	                        {
	                            GIT_Assert(false,eErrorCodeMdm|eWriteFail);
	                            Trace("Error\r\n");
	                        }
							else
							{
								//printf("now2:%d,tot2:%d\r\n",nLength-2,g_uiAGPSFilesize);
								//hexdump(pData,nLength-2);
								g_uiAGPSFilesize += (nLength-2);
								//printf("g_uiAGPSFilesize:%d\r\n",g_uiAGPSFilesize);
								printf("<");
							}
						}
						else
						{
							if( WriteAgpsData(AUTOLINK_AGPS_DATA,(char*)pData,nLength) != GIT_FR_OK )
	                        {
	                            GIT_Assert(false,eErrorCodeMdm|eWriteFail);
	                            Trace("Error\r\n");
	                        }
							else
							{
								//printf("now22:%d,tot22:%d\r\n",nLength,g_uiAGPSFilesize);
								g_uiAGPSFilesize += (nLength);
								//printf("g_uiAGPSFilesize:%d\r\n",g_uiAGPSFilesize);
							}
						}
#else
						if( WriteAgpsData(AUTOLINK_AGPS_DATA,(char*)pData,nLength-2) != GIT_FR_OK )
                        {
                            GIT_Assert(false,eErrorCodeMdm|eWriteFail);
                            Trace("Error\r\n");
                        }
						else
						{
							//hexdump(pData,nLength-2);
							g_uiAGPSFilesize += (nLength-2);
						}
#endif
                        //hexdump(pData,nLength);
                    }
				}

				ModemManagerData.nAvailableReadDataLength = 0;
//				Trace("\n<2.Read Socket Data:(%d)>\r\n", nLength);
			}
			else if(ModemManagerData.nAvailableReadDataLength - g_nXXXCompareStringLength == nLength) {
				if(ModemManagerData.bModemDataSaveToFlashFlag == true) {
					FOTA_SaveBinDataToInternalFlash(pData, nLength);
					gwTotalReceiveBinDataLength += nLength;
				}
				else {
                    if( ModemManagerData.eCurrentMessageSendingType != eMESSAGE_APGS_DATA )
                    {
    					memcpy(&gaModemCommRxDataBuffer[gwModemCommRxDataLength], pData, nLength);
    					//gwModemCommRxDataLength += ModemManagerData.nAvailableReadDataLength;
                        gwModemCommRxDataLength += nLength;
						if( (gwModemCommRxDataLength > 2) && (gaModemCommRxDataBuffer[gwModemCommRxDataLength-1]=='}') && (gaModemCommRxDataBuffer[gwModemCommRxDataLength-2]=='}'))
						{
							g_bCompleteDataFlag = true;
						}
                    }
                    else
                    {
                        if( WriteAgpsData(AUTOLINK_AGPS_DATA,(char*)pData,nLength) != GIT_FR_OK )
                        {
                            GIT_Assert(false,eErrorCodeMdm|eWriteFail);
                            Trace("Error\r\n");
                        }
#ifdef RF_COMMON_MODEM
						else
						{
							//printf("now3:%d,tot:3%d\r\n",nLength,g_uiAGPSFilesize);
							//hexdump(pData,nLength);
							g_uiAGPSFilesize += nLength;
							//printf("g_uiAGPSFilesize:%d\r\n",g_uiAGPSFilesize);
						}
#endif

                        //hexdump(pData,nLength);
                    }
				}

				ModemManagerData.nAvailableReadDataLength = g_nXXXCompareStringLength;
			}
			else {
				if(ModemManagerData.bModemDataSaveToFlashFlag == true) {
					FOTA_SaveBinDataToInternalFlash(pData, nLength - 2);
//					Trace("3.rcv len: %d\r\n", nLength);
					gwTotalReceiveBinDataLength += (nLength - 2);
				}
				else 
				{
                    if( ModemManagerData.eCurrentMessageSendingType != eMESSAGE_APGS_DATA )
                    {
    					memcpy(&gaModemCommRxDataBuffer[gwModemCommRxDataLength], pData, nLength - 2);
						gwModemCommRxDataLength += (nLength - 2);		
						if( (gwModemCommRxDataLength > 2) && (gaModemCommRxDataBuffer[gwModemCommRxDataLength-1]=='}') && (gaModemCommRxDataBuffer[gwModemCommRxDataLength-2]=='}'))
						{
							g_bCompleteDataFlag = true;
						}

						// exception case : split data was transfered, the last data will be size - 2 then all data received
						/*		// anaylyze data	  
						-----------------------------------------------
						case over 3 split
						-----------------------------------------------
						^SISR 535 : only data size
						recevied total size 537 : include last 0x0d,0x0a
						-----------------------------------------------
						535  : 537
						-----------------------------------------------
						535  261 // split recived data w/ 0x0d,0x0a
						535  78  // split recived data w/ 0x0d,0x0a
						535  66  // split recived data w/ 0x0d,0x0a
						535  132 // split recived data w/ 0x0d,0x0a
						-----------------------------------------------
						535  537	// real recevied data
						-----------------------------------------------
						// recive process  : -2 (w/o 0x0d,0x0a)
						-----------------------------------------------
						533  259	
						531  76	
						529  64	
						527  130	
						-----------------------------------------------
						527  529 
						-----------------------------------------------
						case 2 split
						-----------------------------------------------
						147	  149
						-----------------------------------------------
						147	  102
						147	  47
						-----------------------------------------------
						145	  100
						143	  45
						-----------------------------------------------
						143	  145
						----------------------------------------------- */

						ModemManagerData.nAvailableReadDataLength -= 2;
						if( (ModemManagerData.nAvailableReadDataLength + 2) == gwModemCommRxDataLength )
						{
							ModemManagerData.nAvailableReadDataLength = 0;
						}
                    }
                    else
                    {
#ifdef RF_COMMON_MODEM //THALES_AGPS
						if( (nLength-2) == ModemManagerData.nAvailableReadDataLength )
						{
							if( WriteAgpsData(AUTOLINK_AGPS_DATA,(char*)pData,nLength-2) != GIT_FR_OK )
	                        {
	                            GIT_Assert(false,eErrorCodeMdm|eWriteFail);
	                            Trace("Error\r\n");
	                        }
							else
							{
								//printf("now4:%d,tot4:%d\r\n",nLength,g_uiAGPSFilesize);
								//hexdump(pData,nLength-2);
								g_uiAGPSFilesize += nLength;
								//printf("g_uiAGPSFilesize:%d\r\n",g_uiAGPSFilesize);
							}
						}
						else
						{
							if( WriteAgpsData(AUTOLINK_AGPS_DATA,(char*)pData,nLength) != GIT_FR_OK )
	                        {
	                            GIT_Assert(false,eErrorCodeMdm|eWriteFail);
	                            Trace("Error\r\n");
	                        }
							else
							{
								//printf("now44:%d,tot44:%d\r\n",nLength,g_uiAGPSFilesize);
								//hexdump(pData,nLength-2);
								g_uiAGPSFilesize += nLength;
								//printf("g_uiAGPSFilesize:%d\r\n",g_uiAGPSFilesize);
							}
						}
#else
                        if( WriteAgpsData(AUTOLINK_AGPS_DATA,(char*)pData,nLength-2) != GIT_FR_OK )
                        {
                            GIT_Assert(false,eErrorCodeMdm|eWriteFail);
                            Trace("Error\r\n");
                        }
						else
						{
							//printf("now4:%d,tot4:%d\r\n",nLength,g_uiAGPSFilesize);
							//hexdump(pData,nLength-2);
							g_uiAGPSFilesize += nLength;
							//printf("g_uiAGPSFilesize:%d\r\n",g_uiAGPSFilesize);
						}
#endif
                        ModemManagerData.nAvailableReadDataLength-=(nLength);

                        //hexdump(pData,nLength);
                    }
				}
			}
			break;

		case eCmd_ReadSMSIndex:
			{
				uint16_t n;
                //uint8_t aTemp[10];
                Trace("Rcv Sms Data\r\n");

                m_eSmsRcvStatus = eSmsRcvStatusRcvData;
				n = g_nCurrRcvSMSIndex;
                //g_stModem_Rep.eMDResponse = eMDResponseStateOK;

                ModemManagerData.nCurrentSMSCount++;

//				Trace("nLength: %d\r\n", nLength);
//				hexdump(pData, nLength);

//				Trace("Current SMS Index: %d[%d]\r\n", g_nCurrRcvSMSIndex, n);
//				Convert_HexString_To_Bin((int8_t *)ModemManagerData.stRcvSMSInfos[n].aData, (int8_t *)pData, (nLength - 2) / 2);
				memcpy((int8_t *)ModemManagerData.stRcvSMSInfos[n].aData, pData, nLength - 2);
				ModemManagerData.stRcvSMSInfos[n].nDataLen = nLength - 2;
				ModemManagerData.stRcvSMSInfos[n].aData[nLength - 2] = 0x00;
                ModemManagerData.stRcvSMSInfos[n].nIndex = n;

                SetRequestSms2Gemalto(false);
//				hexdump(ModemManagerData.stRcvSMSInfos[n].aData, (nLength - 2));
//              sprintf((char *)aTemp, "%d, 4\x00", ModemManagerData.stRcvSMSInfos[nIndex].nIndex);             // 4: Delete all messages from preferred message storage including unread messages.
//                sprintf((char *)aTemp, "%d, 0\x00", g_nCurrRcvSMSIndex);             // 0: Delete the message specified index
//                if(Md_DeleteSMSIndex(aTemp) == true) {
//                    Trace("Delete SMS index: %d\r\n", g_nCurrRcvSMSIndex);
//                }

			}
			break;
        case eCmd_SetPowerOff:
            // do nothing
            break;
        case eCmd_SetCtzr:
            break;
        case eCmd_CGATT:
            MDResGprs(pData,nLength);
            break;
#ifdef RF_COMMON_MODEM //mod.kks 21.11.03
        case eCmd_SetReport:
            break;
		case eCmd_SBNW_Send_Start:
			break;
		case eCmd_SBNW_Delete:
			break;
		case eCmd_SetGNSS:
			break;
#endif
        case eCmd_None:
            break;
        case eCmd_SPOW: // TOWDAM TEST
            break;
		default:
			printf("!!![Modem Res- not Define Input:g_stModem_Rep.cmd_index: %d, g_stModem_Rep.start_rep:%d,g_stModem_Rep.eMDResponse: %d, g_stModem_Rep.nResponseLineCount: %d\r\n", g_stModem_Rep.cmd_index,g_stModem_Rep.start_rep,g_stModem_Rep.eMDResponse,g_stModem_Rep.nResponseLineCount);
			Trace("ModemManagerData.eCurrentMessageSendingType: %d, ModemManagerData.bRetryCommFlag %d, ModemManagerData.nRetryCommCount: %d,\r\n",ModemManagerData.eCurrentMessageSendingType,ModemManagerData.bRetryCommFlag,ModemManagerData.nRetryCommCount);
			Trace("ModemManagerData.bWriteReady %d,ModemManagerData.bConfirmWriteLen %d,ModemManagerData.bFinishWrite %d,\r\n",ModemManagerData.bWriteReady,ModemManagerData.bConfirmWriteLen,ModemManagerData.bFinishWrite);
			Trace("ModemManagerData.bReadyReadData %d, ModemManagerData.eState %d,\r\n",ModemManagerData.bReadyReadData,ModemManagerData.eState);
			Trace("MessageManagerData.eMessageCommResultCode %d\r\n",MessageManagerData.eMessageCommResultCode);
			Trace("MessageManagerData.bReceivedRequestRemoteMessageFlag %d\r\n",MessageManagerData.bReceivedRequestRemoteMessageFlag);
			Trace("GetModemControlOldState()%d,GetModemControlOldEvent()%d,GetModemControlEvent()%d\r\n",GetModemControlOldState(),GetModemControlOldEvent(),GetModemControlEvent());
			Trace("m_llStorageDrivingKey%lld",m_llStorageDrivingKey);
			
			for(i = 0; i < nLength; i++) {
				printf("[%c, %02X]\r\n", pData[i], pData[i]);
			}

            ClearQueue(g_stGitCommInfo[eCOMM_TYPE_UART_MODEM].pstInQueue);

			break;
	}
}

//*********************************************************************************
// Function of Modem Command Receivng
//*********************************************************************************
void MDResOK(BYTE* pData, uint32_t nLength)
{
    eCmdIndex eOldCmd=eCmd_None;
	if(g_stModem_Rep.start_rep == true) {
        eOldCmd = g_stModem_Rep.cmd_index;
		g_stModem_Rep.start_rep = false;
		g_stModem_Rep.cmd_index = eCmd_None;
		g_stModem_Rep.eMDResponse = eMDResponseStateOK;
	}

    if( eOldCmd == eCmd_ReadSMSIndex )
    {
        Trace("Rcv Sms Ok\r\n");
        //SetRequestSms2Gemalto(false);
        //g_stModem_Rep.cmd_index = eOldCmd;
    }
#ifdef RF_COMMON_MODEM
    if( eOldCmd == eCmd_SBNW_Delete )
    {
        g_stModem_Rep.start_rep = false;
		g_stModem_Rep.cmd_index = eCmd_SBNW_Delete;
		g_stModem_Rep.eMDResponse = eMDResponseStateWait;
    }
	if( eOldCmd == eCmd_SBNW_Send_Start )
    {
        g_stModem_Rep.start_rep = false;
		g_stModem_Rep.cmd_index = eCmd_SBNW_Send_Start;
		g_stModem_Rep.eMDResponse = eMDResponseStateWait;
    }
#endif
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

//	Trace("< OK\r\n");
#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Run, OK\r\n", __FUNCTION__);
#endif
}

void MDResAGPSEndOK(BYTE* pData, uint32_t nLength)
{
	//uint8_t aTemp[64];
    //eCmdIndex eOldCmd;

	g_stModem_Rep.start_rep = false;
	g_stModem_Rep.cmd_index = eCmd_None;
	g_stModem_Rep.eMDResponse = eMDResponseStateOK;

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
}


//MONI 2018-02-21
void MDResIMSI(BYTE* pData, uint32_t nLength)
{
    // PLMN  // nationality // service provider
    // 50501 // australia   // telstra
    // 50502 // autstralia  // optus
    // 45008 // korea       // kt
    // 45005 // korea       // sk
    // 45006 // korea       // u+
    // sample simi = 505013499250517
	uint8_t aTemp[64]={0,};
//	int32_t nInfoNo;
//	uint16_t nRes;

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	Trace("system simi\r\n");
	Trace("%s\r\n", aTemp);

	//nRes = ssscanf((char *)aTemp, nLength, "^SYSINFO: %d", &nInfoNo);
	//if(nRes == 1) {
	//	Trace(" Unsoliccited Result Code: %d\r\n\r\n", nInfoNo);
	//}
}

#ifndef RF_COMMON_MODEM //mod.kks 21.10.25
void MDSetEchoRes(BYTE* pData, uint32_t nLength)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Run\r\n", __FUNCTION__);
//#endif
}

void MDSetResultRes(BYTE* pData, uint32_t nLength)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Run\r\n", __FUNCTION__);
//#endif
}

void MDRecvSystemLoading(BYTE* pData, uint32_t nLength)
{
	Trace(" Rcv ^SYSLOADING\r\n\r\n");
	ModemManagerData.bModemRcvSysLoadingMessageFlag = true;

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
}
#endif

void MDRecvSystemStart(BYTE* pData, uint32_t nLength)
{
	Trace(" Rcv ^SYSSTART\r\n\r\n");

#ifndef RF_COMMON_MODEM //mod.kks 21.10.26   move the message to "MDRecvprov" step to avoid the conflict.
	ModemManagerData.bModemRcvSysStartMessageFlag = true;
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
#endif
}

void MDRecvSystemInfo(BYTE* pData, uint32_t nLength)
{
	uint8_t aTemp[64]={0,};
	int32_t nInfoNo=0;
	uint16_t nRes=0;

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	Trace("system information\r\n");
	Trace("%s\n", aTemp);

	nRes = ssscanf((char *)aTemp, nLength, "^SYSINFO: %d", &nInfoNo);
	if(nRes == 1) {
		Trace(" Unsoliccited Result Code: %d\r\n\r\n", nInfoNo);
	}
}

void MDRecvShutDown(BYTE* pData, uint32_t nLength)
{
	Trace("^SHUTDOWN\r\n\r\n");
}

void MDRecvPBReady(BYTE* pData, uint32_t nLength)
{
	Trace("Rcv +PBREADY\r\n\r\n");
	ModemManagerData.bModemRcvPBReadyMessageFlag = true;
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
	if( ModemManagerData.eState == eMODEM_WAIT_RCV_SYSSTART )	// Sysstart가 안오고 바로 PBReady가 오는 경우가 존재
	{
		ModemManagerData.bModemRcvSysStartMessageFlag = true;
		SetModemState(eMODEM_Wait_Message_PBREADY);
	}
}

void MDRecvReadyWrite(BYTE *pData, uint32_t nLength)
{
	uint8_t aTemp[64]={0,};
	uint16_t nRes=0;
	int32_t nId=0;
	int32_t nReqWriteLength=0;
	int32_t nUnackData=0;

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	Trace(" Rcv < %s\r\n", aTemp);

	nRes = ssscanf((char *)aTemp, nLength, "^SISW: %d,%d,%d", &nId, &nReqWriteLength, &nUnackData);
	if(nRes == 2) {
		Trace(" Rcv URC\r\n");
//		Trace("id: %d, ret: %d\n", nId, nReqWriteLength);

		if(nId == ModemManagerData.nInternetServiceProfileId) {
			if(nReqWriteLength == 1) {
                ModemManagerData.bWriteReady = true;

				Trace(" Ready to Write\n");
			}
			else if(nReqWriteLength == 2) {
				Trace(" Data transfer has been finished successfully\r\n");
				Trace(" and Internet service may be closed without loss of data\r\n");
				ModemManagerData.bFinishWrite = true;
				gbStartInterceptModemRcvData = false;
#ifdef RF_COMMON_MODEM //21.12.30 mod.kks to suport the sequence in PLS63W.
                ModemManagerData.bWriteReady = true;
#endif

#ifdef RF_COMMON_MODEM_TEMP //mod.kks 21.11.03  todo check issue NOT receive 200.
                ModemManagerData.nHTTPResponseCode = 200;
				ModemManagerData.bHTTPRcvPostResponse = true;
				HalTimerStartSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly, eSWTimer_ONESHOT); //temp .....
#endif
			}
		}

		return;
	}
	else if(nRes == 3) {
		Trace(" Rcv resp of AT^SISW\r\n");

//		wTemp = MDM_CMD_NORESPONSE_TIMEOUT - HalTimerGetSwTimerCount(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
//		Trace(" Resp delay: %d\r\n", wTemp);

		if(nId == ModemManagerData.nInternetServiceProfileId && nReqWriteLength <= ModemManagerData.nWriteDataLength) {
			ModemManagerData.bConfirmWriteLen = true;

//			Trace(" Confirm Write Length Ok(%d)\r\n", nReqWriteLength);

			if(nReqWriteLength == 0) {
//				Trace(" Complete Wirite data...\r\n");
			}
			else {
//				Trace(" Now Wirite the data...\r\n");
			}
		}
	}
	else {
		return;
	}

	return;
}

void MDRecvSocketDataReadAvailable2(BYTE *pData, uint32_t nLength)
{
	uint8_t aTemp[48]={0,};

	uint16_t nRes=0;
	int32_t nId=0;
	int32_t nRcvLen=0;

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	Trace(" Rcv < %s\r\n", aTemp);

    nRes = ssscanf((char *)aTemp, nLength, "^SISR: %d,%d", &nId, &nRcvLen);
    if(nRes != 2) {
    	return;
    }

    if(nId != ModemManagerData.nInternetServiceProfileId)
        return;

    if( nRcvLen == 1 )  // notify exist received data
    {
#ifdef RF_COMMON_MODEM_TEMP //mod.kks todo check issue NOT receive 200.
        HalTimerStopSWTimer(ModemManagerData.iTimer_Rcv_Bin_Data_TimeoutDly);
#endif
        ModemManagerData.bReadyReadData = true;
        // 수신 받을 데이터가 있다.
        ModemManagerData.bEndOfData = false;
        Trace(" URC - Ready to Read\r\n");
    }
    else if( nRcvLen == 2 ) // finished sending data
    {
        // 수신 받을 데이터가 없다. file 끝.
        ModemManagerData.bEndOfData = true;
        Trace(" URC - End of Data(^SISR: id, 2)\r\n");
    }
    else if( nRcvLen == -2 ) // there is no data
    {
        // there is no data
        ModemManagerData.nAvailableReadDataLength = -2;
        //Trace("rcv len: %d\r\n", ModemManagerData.nAvailableReadDataLength);

		// 수신 받을 데이터가 없다. file 끝.
		// 여기서 끝이 아니고, -2 값이 수신 된 후에 다시한번 URC가 수신된다.
		// ^SISR: 3,2 가 다시한번 온다. 3은 ID, 2는 end of data
		ModemManagerData.bExpectEndOfData = true;
		Trace(" End of Data(next time)(^SISR:-2)\r\n");
//#warning "timeout 필요"
    }
    else  // notify data length
    {
        if(g_stModem_Rep.start_rep == true && g_stModem_Rep.cmd_index == eCmd_InternetSocketRead && ModemManagerData.bRespSISR == false)
        {
            // AT^SISR 명령에 대한 OK 응답이 오기전에 read data의 end of data(^SISR: 0,2)가 먼저 오는 경우가 발생하기도 한다.
		    ModemManagerData.bRespSISR = true;

			if(nRcvLen >= 0)
            {
				ModemManagerData.nAvailableReadDataLength = nRcvLen;
				ModemManagerData.bExpectEndOfData = false;
                //Trace("nAvailable rcv len: %d\r\n", nRcvLen);

				if(geModemReceiveDataType == eMODEM_RECEIVE_DATA_TYPE_BIN)
                {
					if(nRcvLen == 0)
                    {
						if(ModemManagerData.bRcvBinDataZeroFlag == false)
                        {
                            //Trace(" received data length 0!!!\r\n");
							ModemManagerData.bRcvBinDataZeroFlag = true;
						}
					}

                    //Trace(" start data intercept!\r\n");
                    // modem에서 응답 오는 data 중에 bin data는 ModemRecvGetLine() 함수에서 0D, 0A를 체크 하지 않는다.
					gbStartInterceptModemRcvData = true;
				}
				else {
					gbStartInterceptModemRcvData = false;
				}
			}
        }
    }

	return;
}


void MDRecvSocketDataReadAvailable(BYTE *pData, uint32_t nLength)
{
	uint8_t aTemp[48]={0,};

	uint16_t nRes=0;
	int32_t nId=0;
	int32_t nRcvLen=0;

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	Trace(" Rcv < %s\r", aTemp);

	if(g_stModem_Rep.start_rep == true && g_stModem_Rep.cmd_index == eCmd_InternetSocketRead && ModemManagerData.bRespSISR == false) {
		// AT^SISR 명령에 대한 OK 응답이 오기전에 read data의 end of data(^SISR: 0,2)가 먼저 오는 경우가 발생하기도 한다.
		ModemManagerData.bRespSISR = true;

		// AT^SISR 명령에 대한 응답
//		Trace(" resp AT+SISR\r\n");
		nRes = ssscanf((char *)aTemp, nLength, "^SISR: %d,%d", &nId, &nRcvLen);
		if(nRes == 2) {
//			Trace("id: %d\r\n", nId);

			if(nId == ModemManagerData.nInternetServiceProfileId) {
				if(nRcvLen >= 0) {
					ModemManagerData.nAvailableReadDataLength = nRcvLen;
					ModemManagerData.bExpectEndOfData = false;
//					Trace("nAvailable rcv len: %d\r\n", nRcvLen);

					if(geModemReceiveDataType == eMODEM_RECEIVE_DATA_TYPE_BIN) {
						if(nRcvLen == 0) {
							if(ModemManagerData.bRcvBinDataZeroFlag == false) {
//								Trace(" received data length 0!!!\r\n");
								ModemManagerData.bRcvBinDataZeroFlag = true;
							}
						}

//						Trace(" start data intercept!\r\n");
						gbStartInterceptModemRcvData = true;				// modem에서 응답 오는 data 중에 bin data는 ModemRecvGetLine() 함수에서 0D, 0A를 체크 하지 않는다.
					}
					else {
						gbStartInterceptModemRcvData = false;
					}
				}
				else {
					ModemManagerData.nAvailableReadDataLength = -2;
//					Trace("rcv len: %d\r\n", ModemManagerData.nAvailableReadDataLength);

					// 수신 받을 데이터가 없다. file 끝.
					// 여기서 끝이 아니고, -2 값이 수신 된 후에 다시한번 URC가 수신된다.
					// ^SISR: 3,2 가 다시한번 온다. 3은 ID, 2는 end of data
					ModemManagerData.bExpectEndOfData = true;
					ModemManagerData.bEndOfData = false;
					Trace(" End of Data(next time)(^SISR:-2)\r\n");
				}
			}
		}
	}
	else {
		// URC 응답
		Trace(" resp URC\r\n");

		// ^SISR URC는 각종 modem data 응답 받을때만 유효하다고 판단하자. 그 외 루틴에서는 무시하자.
		nRes = ssscanf((char *)aTemp, nLength, "^SISR: %d,%d", &nId, &nRcvLen);
		if(nRes == 2) {
//			Trace("id: %d,  Indicates: %d\r\n", nId, nRcvLen);

			if(nId == ModemManagerData.nInternetServiceProfileId) {
				ModemManagerData.bReadyReadData = true;
				if(nRcvLen == 1) {
					ModemManagerData.bEndOfData = false;			// 수신 받을 데이터가 있다.
					Trace(" URC - Ready to Read\r\n");
				}
				else if(nRcvLen == 2) {
					// end of data
					ModemManagerData.bEndOfData = true;				// 수신 받을 데이터가 없다. file 끝.
					Trace(" URC - End of Data(^SISR: id, 2)\r\n");
				}
			}
		}
	}

	return;
}

void MDRecvSBC(BYTE *pData, uint32_t nLength)
{
	uint8_t aTemp[32]={0,};

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	Trace("%s\r\n", aTemp);

	return;
}

void MDRecvSIS(BYTE* pData, uint32_t nLength)
{
	uint8_t aTemp[256 + 40]={0,};
	uint16_t nRes=0;
	int32_t nId=0;
	int32_t nURCCouseId=0;
	int32_t nURCInfoId=0;
	int8_t aURCInfoText[256]={0,};
	int32_t nHttpResponse=0;

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	Trace(" Rcv < %s\r\n", aTemp);

	nRes = ssscanf((char *)aTemp, nLength, "^SIS: %d,%d,%d,\"%s\"", &nId, &nURCCouseId, &nURCInfoId, aURCInfoText);
	if(nRes != 4) {
		return;
	}

//	Trace("nRes: %d\r\n", nRes);
//	Trace("nId: %d\r\n", nId);
//	Trace("nURCCouseId: %d\r\n", nURCCouseId);
//	Trace("nURCInfoId: %d\r\n", nURCInfoId);
//	Trace("aURCInfoText: %s\r\n", aURCInfoText);

    // urcInfoId : description
    // 1 - 2000 : Error // service aborted
    // 2001-4000 : Information related to progress of service
    // 4001-6000 : Warning, but no service abort,
    // 6001-8000 : Notes

	ModemManagerData.nHTTPURCInfoId = nURCInfoId;
#ifdef RF_COMMON_MODEM //mod.kks 21.11.23
    if(nURCInfoId == 48) { //ELS61 ^SISR=2,-2 ..
        ModemManagerData.nAvailableReadDataLength = -2;
        ModemManagerData.bExpectEndOfData = true;
        ModemManagerData.bEndOfData = false;
        ModemManagerData.bHTTPRcvPostResponse = true;
        return;
    }
#endif
	if(nURCInfoId == 2200) {
		if(memcmp((char *)aURCInfoText, "HTTP POST: ", 11) == 0) {
			Trace(" Received HTTP Post message\r\n");
			ModemManagerData.bHTTPRcvPostURL = true;
		}
#ifdef RF_COMMON_MODEM
        else if(memcmp((char *)aURCInfoText, "HTTP/1.1", 8) == 0) {
            nRes = ssscanf((char *)aURCInfoText, strlen((char *)aURCInfoText), "HTTP/1.1 %d", &nHttpResponse);
#else
		else if(memcmp((char *)aURCInfoText, "HTTP POST Response: ", 20) == 0) {

			nRes = ssscanf((char *)aURCInfoText, strlen((char *)aURCInfoText), "HTTP POST Response: %d", &nHttpResponse);
#endif
			if(nRes == 1) {
				Trace("nHttpResponse: %d\r\n", nHttpResponse);
				ModemManagerData.nHTTPResponseCode = nHttpResponse;
				ModemManagerData.bHTTPRcvPostResponse = true;
				if(ModemManagerData.nHTTPResponseCode == 200) {
					Trace(" HTTP response OK\r\n");
				}
				else {
					Trace(" Fail HTTP Write(%d)\r\n", ModemManagerData.nHTTPResponseCode);
					g_stModem_Rep.eMDResponse = eMDResponseState_NetworkError;				
				}

				return;
			}
		}
        else
        {
            Trace(" Received HTTP Post message\r\n");
			ModemManagerData.bHTTPRcvPostURL = true;
        }
	}
	else {
		Trace(" nURCInfoId: %d\r\n", nURCInfoId);

		if(nURCInfoId == 15) {
			if(memcmp((char *)aURCInfoText, "Remote host has reset the connection", strlen("Remote host has reset the connection")) == 0) {
				Trace(" Remote host has reset the connection\r\n");
				// fota 진행중이다.
				Trace(" retry fota...\r\n\r\n");
				ModemManagerData.nHTTPResponseCode = 0xFFFF;
				ModemManagerData.bHTTPRcvPostResponse = true;

                g_stModem_Rep.eMDResponse = eMDResponseState_NetworkError;
			}
		}
		else if(nURCInfoId == 8002) {
			if(memcmp((char *)aURCInfoText, "HttpHTTP POST: IllegalArgumentException Socket-Error:", strlen("HttpHTTP POST: IllegalArgumentException Socket-Error:")) == 0) {
				nRes = ssscanf((char *)aURCInfoText, strlen((char *)aURCInfoText), "HttpHTTP POST: IllegalArgumentException Socket-Error:%d", &nHttpResponse);
				if(nRes == 1) {
					ModemManagerData.nHTTPResponseCode = nHttpResponse;
					ModemManagerData.bHTTPRcvPostResponse = true;

					Trace(" Socket Error:%d\r\n", ModemManagerData.nHTTPResponseCode);
				}
			}
			else if(memcmp((char *)aURCInfoText, "HttpHTTP POST: IllegalArgumentException HTTP-CODE: ", strlen("HttpHTTP POST: IllegalArgumentException HTTP-CODE: ")) == 0) {
				nRes = ssscanf((char *)aURCInfoText, strlen((char *)aURCInfoText), "HttpHTTP POST: IllegalArgumentException HTTP-CODE: %d", &nHttpResponse);
				if(nRes == 1) {
					Trace("nHttpResponse: %d\r\n", nHttpResponse);
					ModemManagerData.nHTTPResponseCode = nHttpResponse;
					ModemManagerData.bHTTPRcvPostResponse = true;

					Trace(" Fail HTTP Write(%d)\r\n", ModemManagerData.nHTTPResponseCode);
				}
			}
            else if(memcmp((char*)aURCInfoText, "HttpHTTP POST: IOException error in sendRequest SSL: underlaying socket closed",
                strlen("HttpHTTP POST: IOException error in sendRequest SSL: underlaying socket closed")) == 0 )
            {
                Trace("IOException error in sendRequest SSL: underlaying socket closed\n");
				ModemManagerData.nHTTPResponseCode = 0xFFF0;
				ModemManagerData.bHTTPRcvPostResponse = true;
				Trace(" Fail HTTP Write(%x)\n", ModemManagerData.nHTTPResponseCode);
            }
            // this is occurred when we use not define apn
            else if(memcmp((char*)aURCInfoText, "HttpHTTP POST: IOException error in sendRequest Profile could not be activated",
                strlen("HttpHTTP POST: IOException error in sendRequest Profile could not be activated")) == 0 )
            {
                Trace("IOException error in sendRequest Profile could not be activated\r\n");
				ModemManagerData.nHTTPResponseCode = 0xFFF1;
				ModemManagerData.bHTTPRcvPostResponse = true;
				Trace(" Fail HTTP Write(%x)\n", ModemManagerData.nHTTPResponseCode);
            }
            // this is occurred when we use wrong site(address)
            else if(memcmp((char*)aURCInfoText, "HttpHTTP POST: IOException error in sendRequest Could not resolve hostname",
                strlen("HttpHTTP POST: IOException error in sendRequest Could not resolve hostname")) == 0 )
            {
                Trace("HttpHTTP POST: IOException error in sendRequest Could not resolve hostname\r\n");
				ModemManagerData.nHTTPResponseCode = 0xFFF2;
				ModemManagerData.bHTTPRcvPostResponse = true;
				Trace(" Fail HTTP Write(%x)\r\n", ModemManagerData.nHTTPResponseCode);
            }
			else {
				ModemManagerData.nHTTPResponseCode = 0xFFFF;
				ModemManagerData.bHTTPRcvPostResponse = true;
			}

            g_stModem_Rep.eMDResponse = eMDResponseState_NetworkError;
		}
		else if(nURCInfoId == 200 && memcmp((char *)aURCInfoText, "HTTP-CODE: ", 11) == 0) {
			nRes = ssscanf((char *)aURCInfoText, strlen((char *)aURCInfoText), "HTTP-CODE: %d", &nHttpResponse);
			if(nRes == 1) {
				Trace("nHttpResponse: %d\r\n", nHttpResponse);
				if(nHttpResponse == 500) {
				}

				ModemManagerData.nHTTPResponseCode = nHttpResponse;
				ModemManagerData.bHTTPRcvPostResponse = true;

				Trace(" Fail HTTP Write(%d)\r\n", ModemManagerData.nHTTPResponseCode);

                g_stModem_Rep.eMDResponse = eMDResponseState_NetworkError;

				return;
			}
		}
		else {
			ModemManagerData.nHTTPResponseCode = 0xFFFF;
			ModemManagerData.bHTTPRcvPostResponse = true;

            g_stModem_Rep.eMDResponse = eMDResponseState_NetworkError;


			Trace(" Fail HTTP Write(%d)\r\n", ModemManagerData.nHTTPResponseCode);
			return;
		}
	}

	if(nId != ModemManagerData.nInternetServiceProfileId) {
		return;
	}

	if(nURCCouseId != 0) {
		return;
	}

	if(nURCInfoId != 2200) {
		return;
	}

	ModemManagerData.bServiceOpen = true;

	return;
}

int m_iTimer_MDM_SMS_Req_TimeoutDly = -1;


//void SmsRequestReadTimeoutTimerCallBack()
//{
//    Trace("reqeust read sms in comm\r\n");
//    ModemManagerData.bNeedStartUpCheckSMSFlag = true;
//    Send2MngModem(eMngModem,eDummy, 0,(stCarReport *)NULL, 0);
//}


void MDRecvCMTI(BYTE* pData, uint32_t nLength)
{
	uint8_t aTemp[40]={0,};
	uint8_t arrMem[6]={0,};
	int32_t nIndex=0;
	int32_t nRes=0;

	memset((int8_t *)aTemp, 0x00, 40);
	memcpy((int8_t *)aTemp, pData, nLength);

	Trace(" rcv %s\r\n", aTemp);

	nRes = ssscanf((char *)aTemp, nLength, "+CMTI: \"%s\",%d", arrMem, &nIndex);
	if(nRes == 2) {
        Trace(" SMS Memory Storage - [%s](%d), \r\n", arrMem, nIndex);
        //SetReqWaitNetworkCheck(true);
        //EnableSystemMessageTimer(&m_iTimer_MDM_SMS_Req_TimeoutDly,3000,eSWTimer_ONESHOT,SmsRequestReadTimeoutTimerCallBack);
        //Trace("Request Sms Timer id : %d\r\n",m_iTimer_MDM_SMS_Req_TimeoutDly);
        ModemManagerData.bNeedStartUpCheckSMSFlag = true;
        //Send2MngModem(eMngModem,eDummy, 0,(stCarReport *)NULL, 0);

        // if fota process, stop
        if( GetModemState() == eMODEM_RUNNING_FOTA )
        {
            SetModemState(eMODEM_READY);
        }
	}

	return;
}

void MDRecvCTZV(BYTE* pData, uint32_t nLength)
{
    Trace(" Res MDRecvCTZV - %s\r\n", pData);
}

void MDRecvNITZInfo(BYTE* pData, uint32_t nLength)
{
    Trace(" Res MDRecvNITZInfo - %s\r\n", pData);
}

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
void MDRecvprov(BYTE* pData, uint32_t nLength)
{
	ModemManagerData.bModemRcvSysStartMessageFlag = true; //mod.kks 21.10.26  to keep the stable status.
	ModemManagerData.bModemRcvPBReadyMessageFlag = true;
    Trace(" Res MDRecvCprov - %s\r\n", pData);
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_URC_Resp_TimeoutDly);
}
void MDRecvSMONI(BYTE* pData, uint32_t nLength)
{
    Trace(" Res MDRecvSMONI - %s\r\n", pData);
}

void MDRecvCTZU(BYTE* pData, uint32_t nLength)
{
    Trace(" Res MDRecvCTZU - %s\r\n", pData);
}

void MDRecvSISO(BYTE* pData, uint32_t nLength)
{
    Trace(" Res MDRecvSISO - %s\r\n", pData);
}
#endif

void MDRecvSCFG(BYTE* pData, uint32_t nLength)
{
	uint8_t aTemp[128]={0,};

	if(nLength <= 7) {
		return;
	}

	memcpy((int8_t *)aTemp, (pData + 7), nLength - 7);
	aTemp[nLength - 7] = 0x00;

	Trace(" Res AT^SCFG - %s\r\n", aTemp);

	return;
}

void MDRecvCGPADDR(BYTE* pData, uint32_t nLength)
{
	uint8_t aTemp[128]={0,};

	if(nLength <= 7) {
		return;
	}

	memcpy((int8_t *)aTemp, (pData + 7), nLength - 7);
	aTemp[nLength - 7] = 0x00;

	Trace(" Res AT+CGPADDR - %s\r\n", aTemp);

	return;
}


void MDGetSystemInfoRes(BYTE* pData, uint32_t nLength)
{
#if defined(DEBUG_AMT_PTCL_LOG)
	Trace(" [Modem - %s] Run\r\n", __FUNCTION__);
#endif

	return;
}

void MDGetIMEIRes(BYTE *pData, uint32_t nLength)
{
	Trace("IMEI: (%d)[%s]\r\n", nLength, pData);

#if defined(DEBUG_AMT_PTCL_LOG)
	Trace(" [Modem - %s] Run\r\n", __FUNCTION__);
#endif

	return;
}

void MDGetCCIDInfo(BYTE* pData, uint32_t nLength)
{
	uint8_t aTmp1[64]={0,};
	uint16_t nRes=0;
	uint8_t arrTmp[64]={0,};

	memset(arrTmp, 0x00, 64);
	memcpy(arrTmp, pData, nLength - 2);
	memset(aTmp1, 0x00, 64);

	nRes = ssscanf((char *)arrTmp, nLength, "+CCID: %s", aTmp1);
	if(nRes == 1) {
		memset((int8_t *)BkSram_ModemInfo.aCCID, 0x00, sizeof(BkSram_ModemInfo.aCCID));
		memcpy((int8_t *)BkSram_ModemInfo.aCCID, aTmp1, strlen((char *)aTmp1));
	 	Trace(" CCID: %s\r\n", BkSram_ModemInfo.aCCID);
	}

	return;
}

void MDGetPhoneNo(BYTE* pData, uint32_t nLength)
{
	uint8_t arrTmp[128]={0,};
	uint8_t aTmp1[64]={0,};
	uint8_t aTmp2[64]={0,};
	
	uint8_t aTmp3[64]={0,};
	uint16_t nRes=0;

	memset(arrTmp, 0x00, 128);
	memcpy(arrTmp, pData, nLength - 2);

 	//Trace(" Phone No - %s\r\n", arrTmp);

	memset(aTmp1, 0x00, 64);
	memset(aTmp2, 0x00, 64);
#ifdef RF_COMMON_MODEM
    uint16_t nType=0;
	nRes = ssscanf((char *)arrTmp, nLength, "+CNUM: ,\"%s\",%d", aTmp2, nType);
	if(nRes == 0)
	{
		nRes = ssscanf((char *)arrTmp, nLength, "+CNUM: \"%s\",\"%s\",%d",aTmp3, aTmp2, nType);
	}
#else
	nRes = ssscanf((char *)arrTmp, nLength, "+CNUM: \"%s\",\"%s\"", aTmp1, aTmp2);
#endif
	if((nRes == 2) || (nRes == 3)) {
		memset((int8_t *)ModemManagerData.aPhoneNo, 0x00, sizeof(ModemManagerData.aPhoneNo));
		memset((int8_t *)BkSram_ModemInfo.aPhoneNo, 0x00, sizeof(BkSram_ModemInfo.aPhoneNo));
		memcpy((int8_t *)ModemManagerData.aPhoneNo, aTmp2, strlen((char *)aTmp2));
		memcpy((int8_t *)BkSram_ModemInfo.aPhoneNo, aTmp2, strlen((char *)aTmp2));

	 	Trace(" Phone No - %s\r\n", ModemManagerData.aPhoneNo);

        stCarReport stReport;
        stUsimInfo info;
        memset((char *)&info,0,sizeof(info));
        memcpy((char *)info.cellPhoneNum,ModemManagerData.aPhoneNo,strlen((char *)ModemManagerData.aPhoneNo));
        memcpy(&stReport.buffer,(int8_t*)&info,sizeof(stUsimInfo));
        Send2MngSysMsg(eMngModem,eMdmStatus,eMS_PhoneNum,&stReport,0);
	}

	return;
}

void MDResERROR(BYTE* pData, uint32_t nLength)
{
	g_stModem_Rep.start_rep = false;
//	g_stModem_Rep.cmd_index = eCmd_None;    // 2018/03/22 save command index
	g_stModem_Rep.eMDResponse = eMDResponseStateFAIL;

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

	Trace(" res ERROR\r\n");

#if defined(DEBUG_AMT_PTCL_LOG)
	Trace(" [Modem - %s] Run\r\n", __FUNCTION__);
#endif
}

void MDResCMEERROR(BYTE* pData, uint32_t nLength)
{
	uint8_t arrTmp[128]={0,};
	uint16_t nRes=0;

	memset(arrTmp, 0x00, 128);
	memcpy(arrTmp, pData, nLength);

	Trace(" rcv [%s]\r\n", arrTmp);

	g_stModem_Rep.start_rep = false;
	g_stModem_Rep.eMDResponse = eMDResponseStateCMEError;

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly );
#if 0//def RF_COMMON_MODEM
	int32_t n1;
	nRes = ssscanf((char *)arrTmp, nLength, "+CME ERROR: %d", &n1);
	if(nRes == 1) {
		Trace("[%d]\r\n", n1);
		if(n1 == 3) {
			Trace("\nrcv +CME ERROR: 3 \r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_OPERATION_NOT_ALLOWED;
		}
		else if(n1 == 256) {
			Trace("\nrcv +CME ERROR: 256\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_OPERATION_TEMPORARY_NOT_ALLOWED;
		}
		else if(n1 == 21) {
			Trace("\nrcv +CME ERROR: 21\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_INVALID_INDEX;
		}
		else if(n1 == 13) {
			Trace("\nrcv +CME ERROR: 13\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_SIM_FAILURE;
		}
		else if(n1 == 10) {
			Trace("\nrcv +CME ERROR: 10\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_SIM_NOT_INSERTED;
		}
		else {
			Trace("\nrcv +CME ERROR: %d\r\n",n1);
			ModemManagerData.eCMEErrorCode = (eCME_ERROR_NO)n1;
		}
	}
#else
	uint8_t arrTmp2[128];
	nRes = ssscanf((char *)arrTmp, nLength, "+CME ERROR: %s", arrTmp2);
	if(nRes == 1) {
		Trace("[%s]\r\n", arrTmp2);
		if(strncmp((char *)arrTmp2, "operation not allowed", 21) == 0) {
			Trace("\nrcv +CME ERROR: 3 \r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_OPERATION_NOT_ALLOWED;
		}
		else if(strncmp((char *)arrTmp2, "operation temporary not allowed", 30) == 0) {
			Trace("\nrcv +CME ERROR: 256\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_OPERATION_TEMPORARY_NOT_ALLOWED;
		}
		else if(strncmp((char *)arrTmp2, "invalid index", 13) == 0) {
			Trace("\nrcv +CME ERROR: 21\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_INVALID_INDEX;
		}
		else if(strncmp((char *)arrTmp2, "SIM failure", 11) == 0) {
			Trace("\nrcv +CME ERROR: 13\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_SIM_FAILURE;
		}
		else if(strncmp((char *)arrTmp2, "SIM not inserted", 16) == 0) {
			Trace("\nrcv +CME ERROR: 10\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_SIM_NOT_INSERTED;
		}
		else if(strncmp((char *)arrTmp2, "unknown", 7) == 0) {
			Trace("\nrcv +CME ERROR: 100\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_UNKNOWN;
		}
		else if(strncmp((char *)arrTmp2, "text string too long", 20) == 0) {
			Trace("\nrcv +CME ERROR: 24\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_TEXT_STRING_TOO_LONG;
		}
		else {
			Trace("\nrcv +CME ERROR: ?\r\n");
			ModemManagerData.eCMEErrorCode = eCME_ERROR_NO_FFFF;
		}
	}
#endif

    // ModemManagerData.bNeedModemHWResetFlag = true;

//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Run\r\n", __FUNCTION__);
//#endif
}

void MDResCMSERROR(BYTE* pData, uint32_t nLength)
{
	uint8_t arrTmp[128]={0,};

	g_stModem_Rep.start_rep = false;
	g_stModem_Rep.cmd_index = eCmd_None;
	g_stModem_Rep.eMDResponse = eMDResponseStateFAIL;

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

	memset(arrTmp, 0x00, 128);
	memcpy(arrTmp, pData, nLength);

	Trace("%s\r\n", arrTmp);

	return;
}

void MDResNOCarrier(BYTE* pData, uint32_t nLength)
{
	g_stModem_Rep.start_rep = false;
	g_stModem_Rep.cmd_index = eCmd_None;
	g_stModem_Rep.eMDResponse = eMDResponseStateFAIL;
	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

#if defined(DEBUG_AMT_PTCL_LOG)
	Trace(" [Modem - %s] Run\r\n", __FUNCTION__);
#endif
}

int32_t day_of_week(int32_t year, int32_t mon, int32_t day)
{
	int32_t weekday=0, c=0;

	if(mon > 2) {
		mon -= 2;
	}
	else {
		mon += 10;
		year--;
	}

	c = year / 100;
	year %= 100;

	weekday = ((13 * mon - 1) / 5) + day + year + year / 4 + c / 4 - 2 * c + 77;

	weekday %= 7;

	return weekday;
}

void MDResGetDateTime(BYTE* pData, uint32_t nLength)
{
	static bool s_bOnceRunTimeSetFlag = false;
    stCarReport report;
	//struct tm strCurrTime, *ptrTime;
	//time_t currTime;

	uint8_t *ptr;
	uint8_t arrTmp[60]={0,};
	uint8_t arrBuff[4]={0,};
	u16 ret=0;
	int32_t nTimeZone=0;
	uint8_t mode=0;
	int8_t sign=0;
    uint32_t unNetworkTime=0;

    stHalRTCTypeDef stDate;

	// < ~ > 사이에 있는 문자열의 형태로 수신 된다.			2016-12-20 오후 5:32:09
	// <+CCLK: "17/07/04,08:39:13+00">
	// <^SIND: nitz,0,"17/07/04,07:21:33",+36,0>
	// 친철하게도 time zone 정보까지 같이 온다. time zone 1 당, 15분이다.
	// GMT 표준시로 응답이 온다.
	// 대한민국은 GMT: +9 이다.
	memset(arrTmp, 0x00, sizeof(arrTmp));
	memcpy(arrTmp, pData, nLength);
//	Trace("\r\n");
//	Trace("%s\r\n", arrTmp);

    // MONI 2018-03-07
    // block this code for checking network time because we need to check time zone
    // some big contry have different time zone
	//if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_SOFTWARE) {
	//	Trace("ePOWER_Wakeup\r\n");
	//	return;
	//}

	if(memcmp((char *)arrTmp, "^SIND", 5) == 0) {
		mode = 0;
	}
	else if(memcmp((char *)arrTmp, "+CCLK", 5) == 0) {
		// AT+CCLK 명령을 이용해서 읽은 시간 데이터로는 STM32의 RTC를 초기화 하지 않도록 했다.
		// 설정 해도 상관은 없다.
		mode = 1;
		return;
	}
	else {
		return;
	}

	ptr = arrTmp;

	//********************************************
	// year
	//********************************************
	ptr = (uint8_t *)strchr((const char *)ptr, '"');
	if(ptr == NULL) {
		return;
	}

	ptr++;
	memcpy(arrBuff, ptr, 2);
	arrBuff[2] = 0x00;

	stDate.RtcDate.RTC_Year = atoi((const char *)arrBuff);
	if(stDate.RtcDate.RTC_Year > 99) {
		return;
	}
	//********************************************

	//********************************************
	// month
	//********************************************
	ptr = (uint8_t *)strchr((const char *)ptr, '/');
	if(ptr == NULL) {
		return;
	}

	ptr++;
	memcpy(arrBuff, ptr, 2);
	arrBuff[2] = 0x00;

	stDate.RtcDate.RTC_Month = atoi((const char *)arrBuff);
	if(stDate.RtcDate.RTC_Month == 0 || stDate.RtcDate.RTC_Month > 12) {
		return;
	}
	//********************************************

	//********************************************
	// Date
	//********************************************
	ptr = (uint8_t *)strchr((const char *)ptr, '/');
	if(ptr == NULL) {
		return;
	}

	ptr++;
	memcpy(arrBuff, ptr, 2);
	arrBuff[2] = 0x00;

	stDate.RtcDate.RTC_Date = atoi((const char *)arrBuff);

	if(stDate.RtcDate.RTC_Date == 0 || stDate.RtcDate.RTC_Date > 31) {
		return;
	}
	//********************************************

	//********************************************
	// hour
	//********************************************
	ptr = (uint8_t *)strchr((const char *)ptr, ',');
	if(ptr == NULL) {
		return;
	}

	ptr++;
	memcpy(arrBuff, ptr, 2);
	arrBuff[2] = 0x00;

	stDate.RtcTime.RTC_Hours = atoi((const char *)arrBuff);
	if(stDate.RtcTime.RTC_Hours > 24) {
		return;
	}
	//********************************************

	//********************************************
	// min
	//********************************************
	ptr = (uint8_t *)strchr((const char *)ptr, ':');
	if(ptr == NULL) {
		return;
	}

	ptr++;
	memcpy(arrBuff, ptr, 2);
	arrBuff[2] = 0x00;

	stDate.RtcTime.RTC_Minutes = atoi((const char *)arrBuff);
	if(stDate.RtcTime.RTC_Minutes > 59) {
		return;
	}
	//********************************************

	//********************************************
	// sec
	//********************************************
	ptr = (uint8_t *)strchr((const char *)ptr, ':');
	if(ptr == NULL) {
		return;
	}

	ptr++;
	memcpy(arrBuff, ptr, 2);
	arrBuff[2] = 0x00;

	stDate.RtcTime.RTC_Seconds = atoi((const char *)arrBuff);
	if(stDate.RtcTime.RTC_Seconds > 59) {
		return;
	}
	//********************************************

	//********************************************
	// time zone
	//********************************************
	if(mode == 0) {
		// ^SIND 응답
		ptr = (uint8_t *)strchr((const char *)ptr, ',');
		if(ptr == NULL) {
			return;
		}

		// time zone의 부호를 검사한다.
		ptr++;
		if(*ptr == '+') {
			sign = 1;
		}
		else {
			sign = -1;
		}

		ptr++;
		memcpy(arrBuff, ptr, 2);
		arrBuff[2] = 0x00;

		nTimeZone = atoi((const char *)arrBuff);
	}
	else {
		// +CCLK 응답
		ptr++;
		ptr++;

		// time zone의 부호를 검사한다.
		if(*ptr == '+') {
			sign = 1;
		}
		else {
			sign = -1;
		}

		ptr++;
		memcpy(arrBuff, ptr, 2);
		arrBuff[2] = 0x00;

		nTimeZone = atoi((const char *)arrBuff);
	}

//	Trace(" Time Zone: (%c)%d(1 unit = 15min)\r\n", ((sign > 0) ? '+' : '-'), nTimeZone);
    nTimeZone = (nTimeZone * sign);

    // settup new time to adjust correct time
    SetNetworkUtcTime(stDate);

    // check time zone is changed or not
	//RTC Setting 변경 : Local 시간을 UTC 시간으로 변경
	//FOTA 완료시한번만 탈 수 있도록 FirmwareInfo에 RTC Setting 완료 Flag 설정
	if(((BkSram_ModemInfo.snTimeZone != nTimeZone)||
		((AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON) && (s_bOnceRunTimeSetFlag == false))) && (nTimeZone!=0))
	{
    	s_bOnceRunTimeSetFlag = true;
	}
	else
	{
        Trace("Timezone is same do nothing\r\n");
        return;
	}

    // set new time zone
	BkSram_ModemInfo.snTimeZone = nTimeZone;
	Trace("New Time Zone: %d\r\n", BkSram_ModemInfo.snTimeZone);

	ret = day_of_week(2000 + stDate.RtcDate.RTC_Year, stDate.RtcDate.RTC_Month, stDate.RtcDate.RTC_Date);
//	Trace("weekday: %d\r\n", ret);				// 1: ������

	stDate.RtcDate.RTC_WeekDay = ret;
	stDate.RtcTime.RTC_H12 = HAL_RTC_H24H;

	Trace(" modem date/time(GMT:%c%d): %04d/%02d/%02d(%d)-%0.2d:%0.2d:%0.2d\r\n",(nTimeZone>0?'+':'-'),nTimeZone,
				2000 + stDate.RtcDate.RTC_Year,
				stDate.RtcDate.RTC_Month,
				stDate.RtcDate.RTC_Date,
				stDate.RtcDate.RTC_WeekDay,
				stDate.RtcTime.RTC_Hours,
				stDate.RtcTime.RTC_Minutes,
				stDate.RtcTime.RTC_Seconds);

    unNetworkTime = GetTimefromDate2(stDate);

    // time zone unit is 15 mins
	//nTimeZone = (nTimeZone * 15) * 60;
	//nTimeZone = nTimeZone * sign;
	//unNetworkTime += nTimeZone;

    // calculate local time with new time adjusted time zone
    GetDatefromTime2(&stDate, unNetworkTime);

    // set the weekday
	stDate.RtcDate.RTC_WeekDay = ret;
    stDate.RtcTime.RTC_H12 = HAL_RTC_H24H;

//#define TEST_INIT_NETWORK_TIME
#ifdef TEST_INIT_NETWORK_TIME

        stDate.RtcDate.RTC_Year = 17;
        stDate.RtcDate.RTC_Month = 1;
        stDate.RtcDate.RTC_Date = 1;
        stDate.RtcDate.RTC_WeekDay = 7; // HAL_RTC_Weekday_Sunday
        stDate.RtcTime.RTC_Hours = 0;
        stDate.RtcTime.RTC_Minutes = 0;
        stDate.RtcTime.RTC_Seconds = 0;

        sprintf((char *)arrTmp, "%04d%02d%02d%0.2d%0.2d%0.2d\x00",
                    2000 + stDate.RtcDate.RTC_Year,
                    stDate.RtcDate.RTC_Month,
                    stDate.RtcDate.RTC_Date,
                    stDate.RtcTime.RTC_Hours,
                    stDate.RtcTime.RTC_Minutes,
                    stDate.RtcTime.RTC_Seconds);

    // set network time to rtc time
    HalDrvRtcWrite(eRtcBin, eRtcAll, stDate, sizeof(stHalRTCTypeDef), 0);

#else
    // set network time to rtc time
    // MONI 20180511 Time will be set in message manager.
    //RTC_SetDate(RTC_Format_BIN, &stDate.date);
    //RTC_SetTime(RTC_Format_BIN, &stDate.time);

#endif

	//*********************************************************************
	sprintf((char *)g_aModemNetworkConnTimeString, "\"%02d/%02d/%02d,%02d:%02d:%02d\"\x00", 
	                        stDate.RtcDate.RTC_Year,
							stDate.RtcDate.RTC_Month,
							stDate.RtcDate.RTC_Date,
							stDate.RtcTime.RTC_Hours,
							stDate.RtcTime.RTC_Minutes,
							stDate.RtcTime.RTC_Seconds);
//	Trace("CCLK: %s\r\n", g_aModemNetworkConnTimeString);

	Trace(" Time/Date Setting Complete\r\n");
	ModemManagerData.bRTCSettingCompleteFlag = true;

    report.rpSetting.ModemSetting.stNetworkDate.unNetWrokTime = unNetworkTime;
    report.rpSetting.ModemSetting.stNetworkDate.sTimeZone = BkSram_ModemInfo.snTimeZone;

    Send2MngSysMsg(eMngModem,eMdmStatus,eMS_NetworkTime, &report,0);

	return;
}

void MDResNetworkRegistration(BYTE* pData, uint32_t nLength)
{
	int32_t n1=0;
	int32_t n2=0;
	uint8_t aTemp[64]={0,};
	uint16_t nRes=0;
	static boolean_t sbDeniedCheckFlag = false, sbUnknownCheckFlag;

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

//	Trace("%s\r\n", aTemp);
	if(memcmp((char *)aTemp, "+CREG:", 6) == 0) {
		nRes = ssscanf((char *)aTemp, nLength, "+CREG: %d,%d", &n1, &n2);
	}
	else if(memcmp((char *)aTemp, "+CGREG:", 7) == 0) {
		nRes = ssscanf((char *)aTemp, nLength, "+CGREG: %d,%d", &n1, &n2);
	}
	else {
		return;
	}

	if(nRes == 2) {
//			Trace("+C(G)REG - mode: %d, regStatus: %d\r\n", n1, n2);
#if 0
		if( g_bDisconnectTestFlag == true )
			ModemManagerData.eNetworkRegStatus = 2;
		else
			ModemManagerData.eNetworkRegStatus = (eNETWORK_REG_STATUS)n2;
#else
		ModemManagerData.eNetworkRegStatus = (eNETWORK_REG_STATUS)n2;
#endif

		if((ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REGISTERED )
			||(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REG_ROAMING))  
		{
			ModemManagerData.nModemNoRegCount = 0;
            sbDeniedCheckFlag = false;
			sbUnknownCheckFlag = false;
            //MONI 2018-02-09
            // led control process is changed
			//SetLedOnOffCtl(LED_ON, eLED_SERVER);
		}
		else if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_REG_DENIED) 
        {
			ModemManagerData.bAvailableModemCommFlag = false;

			// 망거부 응답이 오게 되면, modem을 SYSTEM을 reset 하자.
			g_stModem_Rep.eMDResponse = eMDResponseState_ModemRegDenied;
            if( sbDeniedCheckFlag == false)
			{
				sbDeniedCheckFlag = true;
#if defined(PROTOCOL24)
				ReportModemStatus(ModemManagerData.eNetworkRegStatus);
#endif
			}
		}
        else if(ModemManagerData.eNetworkRegStatus == eNETWORK_REG_STATUS_UNKNOWN)
        {
            ModemManagerData.nModemNoRegCount = 0;
            if( sbUnknownCheckFlag == false)
			{
				sbUnknownCheckFlag = true;
#if defined(PROTOCOL24)
				ReportModemStatus(ModemManagerData.eNetworkRegStatus);
#endif				
                //WriteAutoLinkModemsStatusData(MODEM_DENIED);
			}
        }
		else {
			ModemManagerData.bAvailableModemCommFlag = false;
		}

        SetModemNetworkStatus((eNETWORK_REG_STATUS)n2,0);
	}

	return;
}

void MDResSignalQuality(BYTE* pData, uint32_t nLength)
{
	int32_t n1=0;
	int32_t n2=0;
	uint8_t aTemp[24]={0,};
	uint16_t nRes=0;

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	//Trace("%s\r\n", aTemp);
	nRes = ssscanf((char *)aTemp, nLength, "+CSQ: %d,%d", &n1, &n2);
	if(nRes == 2) {
		if(n1 == 99) {
			ModemManagerData.nRSSI = n1;
			ModemManagerData.ndBm = 0;
			Trace("not connected network!!!\r\n\r\n");
			ModemManagerData.eNetworkRegStatus = eNETWORK_REG_STATUS_NOT_REG;

            SetModemNetworkStatus(eNETWORK_REG_STATUS_NOT_REG,0);

			return;
		}

		// datasheet에 명시된 RSSI 값과 dBm의 상관관계를 수식으로 표현하면
		// y = 2x - 113 이다. y: dBm, x: RSSI
		ModemManagerData.nRSSI = n1;
		ModemManagerData.ndBm = 2 * n1 - 113;

        //Trace("rssi: %d(%+d dBm), ber: %d\r\n", n1, ModemManagerData.ndBm, n2);

		if(gbEnableShowRSSI == true)
		{
			printf("rssi: %d(%+d dBm), ber: %d\r\n", n1, ModemManagerData.ndBm, n2);
		}

        SetModemNetworkStatus(eNETWORK_REG_STATUS_IDLE,ModemManagerData.ndBm);
	}

	return;
}

void MDResReadOperatorSelection(BYTE* pData, uint32_t nLength)
{
	uint16_t n1=0;
	uint16_t n2=0;
	uint16_t n4=0;
	uint8_t aTemp[36]={0,};
	uint8_t aTemp1[36]={0,};
	uint16_t nRes=0;

	memcpy((int8_t *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	//Trace("%s\r\n", aTemp);

	memset((int8_t *)aTemp1, 0x00, 36);
	nRes = ssscanf((char *)aTemp, nLength, "+COPS: %d,%d,\"%s\",%d", &n1, &n2, aTemp1, &n4);
	if(nRes == 4) {
        if( strncmp((char const*)ModemManagerData.aOperator,(char const*)aTemp1,sizeof(ModemManagerData.aOperator))!=0)
        {
            strcpy((char *)ModemManagerData.aOperator, (char *)aTemp1);
            printf("mode: %d, format: %d, opName: %s, act: %d\r\n", n1, n2, ModemManagerData.aOperator, n4);
        }
	}

	return;
}

void MDResReadSMSindex(BYTE* pData, uint32_t nLength) // 문자 시간 ??
{
	int32_t n=0;
	uint8_t arrTmp[256]={0,};
	uint8_t aState[64]={0,};
	uint8_t aOA[64]={0,};
	uint8_t aAlpha[64]={0,};
	int32_t nYear=0;
	int32_t nMon=0;
	int32_t nDay=0;
	int32_t nHour=0;
	int32_t nMin=0;
	int32_t nSec=0;
	int8_t cSign=0;
	int32_t nTimeZone=0;
	uint16_t nRes=0;

	memcpy(arrTmp, pData, nLength);
	arrTmp[nLength - 2] = 0;

	Trace("%s\r\n", arrTmp);
	memset(aState, 0x00, 64);
	memset(aOA, 0x00, 64);

	nRes = ssscanf((char *)arrTmp, nLength, "+CMGL: %d,\"%s\",\"%s\",%s,\"%d/%d/%d,%d:%d:%d%+d\"", &n, aState, aOA, aAlpha, &nYear, &nMon, &nDay, &nHour, &nMin, &nSec, &cSign, &nTimeZone);
//	Trace("%d\n", nRes);
//	Trace("%d\n", n);
//	Trace("%s\n", aState);
//	Trace("%s\n", aOA);
//	Trace("%s\n", aAlpha);
//	Trace("%d\n", nYear);
//	Trace("%d\n", nMon);
//	Trace("%d\n", nDay);
//	Trace("%d\n", nHour);
//	Trace("%d\n", nMin);
//	Trace("%d\n", nSec);
////	Trace("%c\n", cSign);
//	Trace("%+d\n", nTimeZone);

	if(nRes == 10) {
        Trace("Rcv Sms Index\r\n");
        m_eSmsRcvStatus = eSmsRcvStatusRcvIndex;
        g_stModem_Rep.cmd_index = eCmd_ReadSMSIndex;

        SetRequestSms2Gemalto(true);
        ClearNetworkDelayProcess();

        //ModemManagerData.nCurrentSMSCount++;
		// index
		g_nCurrRcvSMSIndex = n;
		Trace("n: %d(count: %d)\r\n", n, ModemManagerData.nCurrentSMSCount);

		ModemManagerData.stRcvSMSInfos[n].nIndex = n;

		//status
		if(memcmp((char *)aState, "REC UNREAD", 10) == 0) {
			ModemManagerData.stRcvSMSInfos[n].eStatus = eSMS_STATUS_REC_UNREAD;
		}
		else if(memcmp((char *)aState, "REC READ", 8) == 0) {
			ModemManagerData.stRcvSMSInfos[n].eStatus = eSMS_STATUS_REC_READ;
		}
		else {
			ModemManagerData.stRcvSMSInfos[n].eStatus = eSMS_STATUS_UNKNOWN;
		}

		// Origination Address
		memcpy(ModemManagerData.stRcvSMSInfos[n].aOriginatingAddr, (char *)aOA, strlen((char *)aOA));
		ModemManagerData.stRcvSMSInfos[n].aOriginatingAddr[strlen((char *)aOA) - 1] = 0x00;

		// time: year
	  ModemManagerData.stRcvSMSInfos[n].strTime.tm_year = 2000 + nYear;

		// time: mon
	  ModemManagerData.stRcvSMSInfos[n].strTime.tm_mon = nMon;

		// time: date
	  ModemManagerData.stRcvSMSInfos[n].strTime.tm_mday = nDay;

		// time: hour
	  ModemManagerData.stRcvSMSInfos[n].strTime.tm_hour = nHour;

		// time: min
	  ModemManagerData.stRcvSMSInfos[n].strTime.tm_min = nMin;

		// time: sec
	  ModemManagerData.stRcvSMSInfos[n].strTime.tm_sec = nSec;

	  ModemManagerData.stRcvSMSInfos[n].strTime.tm_wday = 0;				// 요일(0 ~ 6)				// 가능하면 정의해 주자. 모르면 0이라도...
	  ModemManagerData.stRcvSMSInfos[n].strTime.tm_yday = 0;				// day in the year
	  ModemManagerData.stRcvSMSInfos[n].strTime.tm_isdst = 0;				// 서머 타임 시간

//		Trace("year: %d\n", ModemManagerData.stRcvSMSInfos[n].strTime.tm_year);
//		Trace(" mon: %d\n", ModemManagerData.stRcvSMSInfos[n].strTime.tm_mon);
//		Trace(" day: %d\n", ModemManagerData.stRcvSMSInfos[n].strTime.tm_mday);
//		Trace("hour: %d\n", ModemManagerData.stRcvSMSInfos[n].strTime.tm_hour);
//		Trace(" min: %d\n", ModemManagerData.stRcvSMSInfos[n].strTime.tm_min);
//		Trace(" sec: %d\n", ModemManagerData.stRcvSMSInfos[n].strTime.tm_sec);
	}
}

void MDResCPMS(BYTE* pData, uint32_t nLength)
{
	uint8_t arrTmp[128]={0,};
	int32_t nCnt1=0, nMaxCnt1=0;
	int32_t nCnt2=0, nMaxCnt2=0;
	int32_t nCnt3=0, nMaxCnt3=0;
	uint16_t nRes=0;

	memset(arrTmp, 0x00, 128);
	memcpy(arrTmp, pData, nLength);

	nRes = ssscanf((char *)arrTmp, nLength, "+CPMS: %d,%d,%d,%d,%d,%d", &nCnt1, &nMaxCnt1, &nCnt2, &nMaxCnt2, &nCnt3, &nMaxCnt3);
	if(nRes == 6) {
		Trace("%s\r\n", arrTmp);
//		Trace("%d,%d,%d,%d,%d,%d\r\n", nCnt1, nMaxCnt1, nCnt2, nMaxCnt2, nCnt3, nMaxCnt3);

		if(nCnt1 > 0 || nCnt2 > 0 || nCnt3 > 0) {
			ModemManagerData.bNeedStartUpCheckSMSFlag = true;
		}
	}

	return;
}
#ifdef RF_COMMON_MODEM
/*mod.kks todo GPS Set value */
void MDResSGPSC(BYTE* pData, uint32_t nLength)
{
  ;
}
#endif

void MDResReadySendSMS(BYTE* pData, uint32_t nLength)
{
	pData = pData;
	nLength = nLength;

	g_stModem_Rep.start_rep = false;
	g_stModem_Rep.eMDResponse = eMDResponseStateOK;
	g_stModem_Rep.cmd_index = eCmd_None;

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

	Trace(" res \"> \"\r\n");
	Trace(" Ready SMS Sending...\r\n");
	Trace(" SMS Data [%s]\r\n", ModemManagerData.aSMSSendData);
	Trace(" SMS Data Len [%d]\r\n", ModemManagerData.nLengthSMSSendData - 1);

	OemWriteUart2Buff((uint8_t *)ModemManagerData.aSMSSendData, ModemManagerData.nLengthSMSSendData, NULL, 0);

	return;
}

void MDResSendSMSstate(BYTE* pData, uint32_t nLength)
{
	uint16_t nRes=0;
	int32_t nMessageReference=0;
	uint8_t arrTmp[20]={0,};

	memset(arrTmp, 0x00, 20);
	memcpy(arrTmp, pData, nLength);

	Trace("%s\r\n", arrTmp);

	nRes = ssscanf((char *)arrTmp, nLength, "+CMGS: %d", &nMessageReference);

	if(nRes == 1) {
		Trace(" Sucess SMS Data Sending(%d)\r\n", nMessageReference);
	}

	return;
}

void MDResCFUN(BYTE* pData, uint32_t nLength)
{
    uint16_t n1=0;
#ifndef RF_COMMON_MODEM
	uint16_t n2=0;
#endif
    uint16_t nRes=0;
    uint8_t aTemp[36] = {0,};

    //g_stModem_Rep.start_rep = false;
    //g_stModem_Rep.cmd_index = eCmd_None;
    //g_stModem_Rep.eMDResponse = eMDResponseStateOK;

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);

    memcpy(aTemp, pData, nLength);
#ifdef RF_COMMON_MODEM
    nRes = ssscanf((char *)aTemp, nLength, "+CFUN: %d,%d", &n1);
//    Trace("cfun:%s\r\n",pData);
    if( nRes == 1 )
#else
    nRes = ssscanf((char *)aTemp, nLength, "+CFUN: %d,%d", &n1, &n2);
//    Trace("cfun:%s\r\n",pData);
    if( nRes == 2 )
#endif


    {
        HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
        if( n1 == 1 )
        {
            Trace("gemalto is normal mode\r\n");
            // mode is normal
            ModemManagerData.bAirplaneMode = false;
        }
        else if( n1 == 4 )
        {
            Trace("gemalto is airplane mode\r\n");
            ModemManagerData.bAirplaneMode = true;
        }
    }

#if defined(DEBUG_AMT_PTCL_LOG)
	Trace(" [Modem - %s] Run\r\n", __FUNCTION__);
#endif
}

void MDResSISI(BYTE *pData, unsigned int nLength)
{
	uint16_t n1=0;
	uint16_t n2=0;
	uint16_t n3=0;
	uint16_t n4=0;
	uint16_t n5=0;
	uint16_t n6=0;
	uint8_t aTemp[36]={0,};
	uint16_t nRes=0;

	memcpy((char *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	printf("\nMDM: %s\r\n", aTemp);

	memset((char *)aTemp, 0x00, 36);
	nRes = ssscanf((char *)aTemp, nLength, "^SISI: %d,%d,%d,%d,%d,%d", &n1, &n2, &n3, &n4, &n5, &n6);
	if(nRes == 6) {
		printf("MDM: n1:%d, n2:%d, n3:%d, n4:%d, n5:%d, n6:%d\r\n", n1, n2, n3, n4, n5, n6);
	}

	return;
}

void MDResSICI(BYTE *pData, unsigned int nLength)
{
	uint16_t n1=0;
	uint16_t n2=0;
	uint16_t n3=0;
	uint8_t aTemp[36]={0,};
	uint8_t aAddr[36]={0,};
	uint16_t nRes=0;

	memcpy((char *)aTemp, pData, nLength);
	aTemp[nLength] = 0x00;

	printf("\nMDM: %s\r\n", aTemp);

	memset((char *)aTemp, 0x00, 36);
	nRes = ssscanf((char *)aTemp, nLength, "^SICI: %d,%d,%d,\"%s\"", &n1, &n2, &n3, aAddr);
	if(nRes == 6) {
		printf("MDM: n1:%d, n2:%d, n3:%d, addr: %s\r\n", n1, n2, n3, aAddr);
	}

	return;
}

int m_nSGPRSDetacchedCount = 0;
int m_nSGPRSAttacchedCount = 0;


void MDResGprs(BYTE *pData, unsigned int nLength)
{
    uint16_t n1=0;
    uint16_t nRes=0;
    uint8_t aTemp[36] = {0,};

    memcpy(aTemp, pData, nLength);
    nRes = ssscanf((char *)aTemp, nLength, "+CGATT: %d", &n1);
    //Trace("%s\r\n",aTemp);
    if( nRes == 1 )
    {
        if( n1 == 1 )
        {
            // gprs service is attached
            m_nSGPRSDetacchedCount = 0;

            // gprs service is detached
            if( GetModemControlOldState() != eModemControlStateConnected &&
                GetModemControlOldState() != eModemControlStateReConnected )
            {
                if( m_nSGPRSAttacchedCount++ > 20 )
                {
                    // this is weired status so request reinitalize
                    SetModemControlEvent(eMngMdmDetachedBaseStation);

                    m_nSGPRSDetacchedCount = 0;
                }
            }
        }
        else //if( n1 == 0 )
        {
            m_nSGPRSAttacchedCount = 0;

            // gprs service is detached
            if( GetModemControlOldState() == eModemControlStateConnected ||
                GetModemControlOldState() == eModemControlStateReConnected )
            {
                if( m_nSGPRSDetacchedCount++ > 20 )
                {
                    // this is weired status so request reinitalize
                    SetModemControlEvent(eMngMdmDetachedBaseStation);

                    m_nSGPRSDetacchedCount = 0;
                }
            }
        }
    }
}
void MDResCPBW(BYTE* pData, uint32_t nLength)
{
	uint8_t arrTmp[64]={0,};

	memset(arrTmp, 0x00, 64);
	memcpy(arrTmp, pData, nLength - 2);

	printf(" CPBW : %s \r\n",arrTmp);
	return;
}

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
void MDResCPIN(BYTE* pData, uint32_t nLength)
{
	uint8_t arrTmp[128];
	uint8_t aState[32];
	uint16_t nRes;

	memset(arrTmp, 0x00, 128);
	memcpy(arrTmp, pData, nLength);


	memset(aState, 0x00, 32);
	nRes = ssscanf((char *)arrTmp, nLength, "+CPIN: %s", aState);

	if(nRes == 1) {
		//status
		if(memcmp((char *)aState, "READY", 5) == 0) {
			Trace("USIM Check %s \r\n", (char *)aState);
		}
        else
        {
            Trace("Please Check the USIM State:%s \r\n", (char *)aState);
        }
	}

	return;
}
#endif
void MDResMonitoringService(BYTE* pData, uint32_t nLength)
{
  //^SMONI: 3G,10737,	131,	-5,-93,		260,01,7D3D,	C80BC9A,--,--,			----,---,-,-5,-93,0,00,00
  //^SMONI: 3G,10737,	131,	-5,-93,		260,01,7D3D,	C80BC9A,--,--,			----,---,-,-5,-93,0,01,06
  //^SMONI: 4G,6300,	20,		10,10,		FDD,262,02,		BF75,0345103,350,		90,-94,-7,CONN
  //^SMONI: 4G,6300,	20,		10,10,		FDD,262,02,		BF75,0345103,350,		33,-94,-7,LIMSRV
  //^SMONI: 3G,10564,	96,		-7.5,		-79,262,02,		0143,00228FF,-92,		-78,LIMSRV
  //^SMONI: 4G,6300,	20,		10,10,		FDD,262,02,		BF75,0345103,350,		33,-94,-7,NOCONN
  //^SMONI: 3G,10737,	131,	-7.5,		-103,260,01,	7D3D,C80BC9A,21,		11,NOCONN
  //^SMONI: 4G,6300,	20,		10,10,		FDD,262,02,		BF75,0345103,350,		90,-94,-7,CONN
  //^SMONI: 4G,6300,	20,		10,10,		FDD,262,02,		BF75,0345103,350,		33,-94,-7,LIMSRV
  //^SMONI: 3G,10564,	96,		-7.5,		-79,262,02,		0143,00228FF,-92,		-78,LIMSRV
  //^SMONI: 4G,SEARCH
  //^SMONI: 3G,SEARCH,	SEARCH
  //^SMONI: 4G,6300,	20,		10,10,		FDD,262,02,		BF75,0345103,350,		33,-94,-7,NOCONN
  //^SMONI: 3G,10564,	296,	-7.5,-		79,262,02,		0143,00228FF,-92,		-78,NOCONN
	uint8_t arrTmp[100];
    uint8_t ucAct[3]={0,};
    uint8_t ucMode[4]={0,};
    uint32_t nEarfcn=0;
    uint32_t nBand=0;
    uint32_t nDLBand=0;
    uint32_t nULBand=0;
    uint32_t nMCC=0;
    uint32_t nMNC=0;
	static uint8_t s_arrTmpSave[100]={0,};

    static uint8_t s_ucAct[3]={0,};
    static uint8_t s_ucMode[4]={0,};
    static uint32_t s_nEarfcn=0;
    static uint32_t s_nBand=0;
    static uint32_t s_nMCC=0;
    static uint32_t s_nMNC=0;

	memset(arrTmp, 0x00, 100);
	memcpy(arrTmp, pData, nLength - 2);
    
	memset(g_ucSMONIData,0x00,sizeof(g_ucSMONIData));

    ssscanf((char *)arrTmp, nLength, "^SMONI: %s,%d,%d,%d,%d,%s,%d,%d",ucAct, &nEarfcn, &nBand, &nDLBand,&nULBand,ucMode,&nMCC,&nMNC);

	strcpy(g_ucSMONIData,arrTmp);

    if( (nEarfcn != s_nEarfcn) || (nBand!=s_nBand) || (nMCC!=s_nMCC) || (nMNC!=s_nMNC) || (strncmp((char const*)ucAct,(char const*)s_ucAct,2)!=0) || (strncmp((char const*)ucMode,(char const*)s_ucMode,3)!=0) )
    {
        s_nEarfcn = nEarfcn;
        s_nBand = nBand;
        s_nMCC = nMCC;
        s_nMNC = nMNC;
        memcpy(s_ucAct, ucAct, sizeof(ucAct));
        memcpy(s_ucMode, ucMode, sizeof(ucMode));

		printf(" SMONI : %s \r\n",arrTmp);
		memcpy(s_arrTmpSave, pData, nLength - 2);
	}

#if 0
	if( strncmp(&arrTmp[8],"4G,SEARCH",9)==0 )	ReportAlramStatus(eMESSAGE_EVENT_KEY_DOOR_LOCK_ALARM , 1);
#endif

	return;
}

unsigned char *GetSMONIData(void)
{
	return g_ucSMONIData;
}

//*********************************************************************************
// Function of Modem Command Sending
//*********************************************************************************
boolean_t Md_SetEcho(uint8_t * state)	// 1 : echo enable   0 : echo disable
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif
	return Md_Make_n_SendPacket(eCmd_SetEcho, state, strlen((char*)state));
}

boolean_t Md_ReadSystemInfo(void)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif

	return Md_Make_n_SendPacket(eCmd_GetSysInfo, NULL, 0);
}

boolean_t Md_SetFlowControl(void)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif

	return Md_Make_n_SendPacket(eCmd_SetFlowControl, NULL, 0);
}

boolean_t Md_GetIMEI(void)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif

	return Md_Make_n_SendPacket(eCmd_GetIMEI, NULL, 0);
}

boolean_t Md_GetDateTime(void)
{
	return Md_Make_n_SendPacket(eCmd_GetDateTime, NULL, 0);
}

boolean_t Md_SetDateTime(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_SetDateTime, state,  strlen((char*)state));
}

boolean_t Md_GetCCIDInfo(void)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif
	return Md_Make_n_SendPacket(eCmd_GetCCIDInfo, NULL, 0);
}

boolean_t Md_GetPhoneNo(void)
{
	return Md_Make_n_SendPacket(eCmd_GetPhoneNo, NULL, 0);
}

boolean_t Md_Set_3M_Baudrate(void)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif

	return Md_Make_n_SendPacket(eCmd_SetBaudrate, (uint8_t *)"3000000", 7);
}

boolean_t Md_Set_115200_Baudrate(void)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif

	return Md_Make_n_SendPacket(eCmd_SetBaudrate, (uint8_t *)"115200", 6);
}

boolean_t Md_Set_921600_Baudrate(void)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif

	return Md_Make_n_SendPacket(eCmd_SetBaudrate, (uint8_t *)"921600", 6);
}

boolean_t Md_Set_ErrorMsgFormat(uint8_t *state)
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif

	return Md_Make_n_SendPacket(eCmd_SetErrorMsgFormat, (uint8_t *)state, strlen((char *)state));
}

boolean_t Md_GetIMSI()	// 1 : Result Off   0 : Result On
{
	return Md_Make_n_SendPacket(eCmd_GetIMSI, NULL, 0);
}


boolean_t Md_SetInternetConnectionService(uint8_t *state)
{
	Trace("*send: AT^SICS=%s\r\n", (char *)state);

	return Md_Make_n_SendPacket(eCmd_SetInternetConnectionProfile, (uint8_t *)state, strlen((char *)state));
}

boolean_t Md_SetInternetServiceProfile(uint8_t * state)
{
	Trace("*send: AT^SISS=%s\r\n", (char *)state);

	return Md_Make_n_SendPacket(eCmd_SetInternetServiceProfile, (uint8_t *)state, strlen((char *)state));
}

bool Md_CheckInternetServiceInfo(uint8_t * state)
{
	printf("MDM: *send: AT^SISI?\r\n");

	return Md_Make_n_SendPacket(eCmd_CheckInternetServiceInfo, (unsigned char *)state, strlen((char *)state));
}

bool Md_CheckInternetConnectionInfo(uint8_t * state)
{
	printf("MDM: *send: AT^SICI?\r\n");

	return Md_Make_n_SendPacket(eCmd_CheckInternetConnectionInfo, (unsigned char *)state, strlen((char *)state));
}
boolean_t Md_SetInternetSocketOpen(uint8_t * state)
{
	Trace("*send: AT^SISO=%s\r\n", (char *)state);
	return Md_Make_n_SendPacket(eCmd_SetInternetSocketOpen, (uint8_t *)state, strlen((char *)state));
}

#ifdef RF_COMMON_MODEM //mod.pdh 21.12.13
boolean_t Md_SetGPIODriver(uint8_t * state)
{
	Trace("*send: AT^SPIO=%s\r\n", (char *)state);
	return Md_Make_n_SendPacket(eCmd_SetGPIODriver, (uint8_t *)state, strlen((char *)state));
}
#endif //#ifdef RF_COMMON_MODEM //mod.pdh 21.12.13

boolean_t Md_SetInternetSocketClose(uint8_t * state)
{
	Trace("*send: AT^SISC=%s\r\n", (char *)state);

	return Md_Make_n_SendPacket(eCmd_SetInternetSocketClose, (uint8_t *)state, strlen((char *)state));
}

boolean_t Md_SetInternetSocketWrite(uint8_t * state)
{
	Trace("*send: AT^SISW=%s\r\n", (char *)state);
//	hexdump(state, 16);

	return Md_Make_n_SendPacket(eCmd_SetInternetSocketWriteLen, (uint8_t *)state, strlen((char *)state));
}

boolean_t Md_InternetSocketRead(uint8_t * state)
{
	Trace("*send: AT^SISR=%s\r\n", state);

	return Md_Make_n_SendPacket(eCmd_InternetSocketRead, (uint8_t *)state, strlen((char *)state));
}

boolean_t Md_CheckNetworkRegistration(void)
{
	uint8_t aTemp[8]={0,};

	sprintf((char *)aTemp, "?");

	return Md_Make_n_SendPacket(eCmd_CheckNetrorkRegistration, (uint8_t *)aTemp, strlen((char *)aTemp));
}

boolean_t Md_CheckPacketDomainRegistration(void)
{
	uint8_t aTemp[8]={0,};

	sprintf((char *)aTemp, "?");

	return Md_Make_n_SendPacket(eCmd_CheckPacketDomainRegistration, (uint8_t *)aTemp, strlen((char *)aTemp));
}

boolean_t Md_CheckSignalQuality(void)
{
	return Md_Make_n_SendPacket(eCmd_CheckSignalQuality, NULL, 0);
}

boolean_t Md_ReadOperatorSelection(void)
{
	uint8_t aTemp[8]={0,};

	sprintf((char *)aTemp, "?");

	return Md_Make_n_SendPacket(eCmd_ReadOperatorSelection, (uint8_t *)aTemp, strlen((char *)aTemp));
}
boolean_t Md_SetOperatorSelection(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_COPS, state, strlen((char*)state));
}

boolean_t Md_SetPowerSaveMode(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_SPOW, state, strlen((char*)state));
}

bool Md_OpenCloseGPIO(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_SCPIN, state, strlen((char const*)state));
}

bool Md_SetResetModemGPIO(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_SSIO, state, strlen((char const*)state));
}
boolean_t Md_SetSMSFormat(uint8_t * state)	//	 1: Text mode   0: PDU mode
{
	return Md_Make_n_SendPacket(eCmd_SetSMSFormat, state, strlen((char*)state));
}

boolean_t Md_ReadSMSforStatus(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_ReadSMSIndex, state,  strlen((char*)state));
}

boolean_t Md_DeleteSMSIndex(uint8_t * state)
{
	Trace("*send: AT_CMGD=%s\r\n", (char *)state);

	return Md_Make_n_SendPacket(eCmd_DeleteSMSIndex, state,  strlen((char*)state));
}
#ifndef RF_COMMON_MODEM
boolean_t Md_SendSMS(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_SendSMS, state,  strlen((char*)state));
}
#endif
boolean_t Md_GetSIND(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_GetSIND, state,  strlen((char*)state));
}

#ifdef RF_COMMON_MODEM //mod.kks gps 21.10.27
boolean_t Md_SetGNSS(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_SetGNSS, state,  strlen((char*)state));
}
boolean_t Md_SetReport(uint8_t * state)   //here
{
	return Md_Make_n_SendPacket(eCmd_SetReport, state,  strlen((char*)state));
}
#endif

boolean_t Md_SetSIND(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_GetSIND, state,  strlen((char*)state));
}

boolean_t Md_SetSMSReportConfig(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_SetSMSReportCnfg, state,  strlen((char*)state));
}

boolean_t Md_SetModemConfigExt(uint8_t * state)
{
	Trace(" AT^SCFG=%s\r\n", (char *)state);

	return Md_Make_n_SendPacket(eCmd_SetModemConfigExt, state,  strlen((char*)state));
}

boolean_t Md_SetSMSStorage(uint8_t * state)
{
	Trace(" AT+CPMS=%s\r\n", (char *)state);

	return Md_Make_n_SendPacket(eCmd_CPMS, state,  strlen((char*)state));
}

boolean_t Md_ResultOnOff(uint8_t * state)	// 1 : Result Off   0 : Result On
{
//#if defined(DEBUG_AMT_PTCL_LOG)
//	Trace(" [Modem - %s] Go\r\n", __FUNCTION__);
//#endif
	return Md_Make_n_SendPacket(eCmd_SetResult, state, strlen((char*)state));
}

boolean_t Md_GPRSAttachDetach(uint8_t * state)	// 1: Attach   0: Detach
{
	//Trace("*AT_CGATT%s\r\n", (char *)state);
	return Md_Make_n_SendPacket(eCmd_CGATT, state, strlen((char*)state));
}

boolean_t Md_SetFlightMode(uint8_t * state)
{
	Trace("*AT+CFUN=%s\r\n", (char *)state);

	return Md_Make_n_SendPacket(eCmd_CFUN, state,  strlen((char*)state));
}

boolean_t Md_CheckAirplane(uint8_t * state)
{
	Trace("*AT+CFUN%s\r\n", (char *)state);

	return Md_Make_n_SendPacket(eCmd_CheckAirplane, state,  strlen((char*)state));
}

#ifdef RF_COMMON_MODEM   //mod.kks 21.12.22
boolean_t Md_SetFactorymode(void)
{
	Trace("*AT+CFUN=5\r\n");

	return Md_Make_n_SendPacket(eCmd_FactoryMode, NULL,  0);
}

boolean_t Md_ConActive(uint8_t * state)
{
	Trace("*AT^SICA=%s\r\n", (char *)state);

	return Md_Make_n_SendPacket(eCmd_ConActive, state,  strlen((char*)state));

}
#endif

boolean_t Md_SetModemPowerMax(uint8_t * state)
{
	printf("MDM: AT^SCFG=%s\r\n", state);

	return Md_Make_n_SendPacket(eCmd_SetModemPowerMax, state,  strlen((char const*)state));
}

boolean_t Md_SetPowerOff(void)
{
	return Md_Make_n_SendPacket(eCmd_SetPowerOff, NULL, 0);
}

boolean_t Md_SetApn(uint8_t * state)
{
	printf("MDM: AT+CGDCONT=%s\r\n", state);

	return Md_Make_n_SendPacket(eCmd_SetApn, state,  strlen((char const*)state));
}

boolean_t Md_SetCTZRTime(void)
{
    printf("MDM: AT+CTZR=1\r\n");

	return Md_Make_n_SendPacket(eCmd_SetCtzr, NULL, 0);
}

boolean_t Md_SetRegistPhoneNumMode(uint8_t * state)
{
	printf("MDM: AT+CPBS%s\r\n", state);

	return Md_Make_n_SendPacket(eCmd_SetRegistPhoneNumMode, state, strlen((char const*)state));
}

boolean_t Md_SetRegistPhoneNumber(uint8_t * state)
{
	printf("MDM: AT+CPBW%s\r\n", state);

	return Md_Make_n_SendPacket(eCmd_SetRegistPhoneNumber, state, strlen((char const*)state));
}
boolean_t Md_SetRadioAccess(uint8_t * state)
{
	Trace("AT+SXRAT=%s\r\n", state);
	return Md_Make_n_SendPacket(eCmd_SetRadioAccess, state, strlen((char const*)state));
}
boolean_t Md_SetNetworkAutoResponse(uint8_t * state)
{
	Trace("AT+GAUTO=%s\r\n", state);
	return Md_Make_n_SendPacket(eCmd_SetNetworkAutoResponse, state, strlen((char const*)state));
}
boolean_t Md_GetServiceMonitor(void)
{
	return Md_Make_n_SendPacket(eCmd_Monitoring_ServingCell, NULL, 0);
}
boolean_t Md_GetPDPAddress(void)
{
	return Md_Make_n_SendPacket(eCmd_Show_PDP_address, NULL, 0);
}
#ifdef RF_COMMON_MODEM //mod.kks 21.11.16
boolean_t Md_SetGNSSEvtNoti(void)
{
	return Md_Make_n_SendPacket(eCmd_GNSS_Evt_Noti, NULL, 0);
}
#endif

boolean_t Md_GetModemConfigExt(uint8_t * state)
{
	Trace(" AT^SCFG%s\r\n", (char *)state);
	return Md_Make_n_SendPacket(eCmd_ExtendConfigSetting, state,  strlen((char*)state));
}
boolean_t Md_SetAirPlaneOn(void)
{
	return Md_Make_n_SendPacket(eCmd_AIRPLANE_ON, NULL, 0);
}
boolean_t Md_SetAirPlaneOff(void)
{
	return Md_Make_n_SendPacket(eCmd_AIRPLANE_OFF, NULL, 0);
}

#ifdef RF_COMMON_MODEM
boolean_t Md_SetAGPSFileDelete(void)
{
	//unsigned char strTemp[]={"agps,-1"};
	//return Md_Make_n_SendPacket(eCmd_SBNW, strTemp, strlen((char const*)strTemp));
	return Md_Make_n_SendPacket(eCmd_SBNW_Delete, NULL, 0);
}

boolean_t Md_SetAGPSFileStartSend(uint8_t * state)
{
	return Md_Make_n_SendPacket(eCmd_SBNW_Send_Start, state, strlen((char const*)state));
}

void MDRecvSBNW(BYTE* pData, uint32_t nLength)
{
	g_stModem_Rep.start_rep = false;
	g_stModem_Rep.cmd_index = eCmd_None;
	g_stModem_Rep.eMDResponse = eMDResponseStateOK;

	HalTimerStopSWTimer(ModemManagerData.iTimer_MDM_Cmd_Resp_TimeoutDly);
}
#endif	//#ifdef RF_COMMON_MODEM

//*********************************************************************************
//*********************************************************************************
/*****************************END OF FILE****/
