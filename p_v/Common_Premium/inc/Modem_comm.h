/**
  ******************************************************************************
  * @file    Modem_Comm.h
  * @author  GIT Diagnosis Software Team by james jean
  * @version V1.1.0
  * @date    29-JAN-2016
  * @brief   Header for BluetoothLowEnery.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODEM_COMM__
#define __MODEM_COMM__

#include "GIT_Util.h"
#include "common.h"
#include "GIT_InterProtocol.h"

#define	MODEM_UART_PACKET_MIN_SIZE 2

#define AES_TEXT_SIZE  							2500

typedef enum _eNotiCmdIndex
{
	eNoti_Min,
	eNoti_NotUsimOpen 		= eNoti_Min,
	eNoti_Certify_Fail		= 2,
	eNoti_Service_Vaild		= 6,
	eNoti_KT_Searching		= 8,
	eNoti_KT_InputNumber	= 17,
	eNoti_BootAlert			= 34,
	eNoti_Call_Denial		= 92,
	eNoti_Max
} eNotiCmdIndex;

typedef enum _eCmdIndex
{
	eCmd_SetEcho = 0,
	eCmd_SetResult,
	eCmd_GetSysInfo,
#ifdef RF_COMMON_MODEM	
	eCmd_FactoryMode,
#endif
	eCmd_SetFlowControl,
	eCmd_GetIMEI,                   // RF_COMMON_MODEM 5
	eCmd_GetDateTime,		        // 5. 시간 데이터를 요청한다.
	eCmd_SetDateTime,
 	eCmd_GetCCIDInfo,
 	eCmd_GetPhoneNo,
 	eCmd_SetBaudrate,               // RF_COMMON_MODEM 10
 	eCmd_SetErrorMsgFormat,				// 10
 	eCmd_SetInternetConnectionProfile,
 	eCmd_SetInternetServiceProfile,
 	eCmd_SetInternetSocketOpen,
 	eCmd_SetInternetSocketClose,    // RF_COMMON_MODEM 15
 	eCmd_SetInternetSocketWriteLen,		// 15
 	eCmd_InternetSocketRead,
 	eCmd_CheckNetrorkRegistration,
 	eCmd_CheckPacketDomainRegistration,
 	eCmd_CheckSignalQuality,        // RF_COMMON_MODEM 20
 	eCmd_ReadOperatorSelection,				// 20
	eCmd_SetSMSFormat,
	eCmd_ReadSMSIndex,
	eCmd_DeleteSMSIndex,
#ifndef RF_COMMON_MODEM
	eCmd_SendSMS,
#endif
	eCmd_GetSIND,                   // RF_COMMON_MODEM 25
	eCmd_SetSMSReportCnfg,
	eCmd_SetModemConfigExt,
	eCmd_AIRPLANE_ON,
	eCmd_AIRPLANE_OFF,              
	eCmd_CPMS,                      // RF_COMMON_MODEM 30
	eCmd_CGATT,
	eCmd_CFUN,
	eCmd_COPS,
	eCmd_SPOW,
	eCmd_SCPIN,                     // RF_COMMON_MODEM 35
	eCmd_SSIO,
	eCmd_CheckInternetServiceInfo,
	eCmd_CheckInternetConnectionInfo,
	eCmd_SetModemPowerMax,
	eCmd_GetIMSI,                   // RF_COMMON_MODEM 40
	eCmd_CheckAirplane,
	eCmd_SetPowerOff,
	eCmd_SetApn,
	eCmd_SetCtzr,
	eCmd_SetRegistPhoneNumMode,     // RF_COMMON_MODEM 45
	eCmd_SetRegistPhoneNumber,

#ifdef RF_COMMON_MODEM  //mod.kks 21.10.25
	eCmd_CheckUSIMIN,
	eCmd_SetCTZU,
	eCmd_ConActive,
	eCmd_SetGNSS,                   // RF_COMMON_MODEM 50
	eCmd_SetReport,
	eCmd_SetGPIODriver,
#endif

	eCmd_SetRadioAccess,
	eCmd_SetNetworkAutoResponse,
	eCmd_Monitoring_ServingCell,    // RF_COMMON_MODEM 55
	eCmd_ExtendConfigSetting,
	eCmd_Show_PDP_address,
#ifdef RF_COMMON_MODEM //mod.kks 21.11.16
	eCmd_SBNW_Send_Start,
	eCmd_SBNW_Delete,
	eCmd_GNSS_Evt_Noti,             // RF_COMMON_MODEM 60
#endif
//	eCmd_Min,
//	eCmd_ATOK,
//	eCmd_ATERROR,
//	eCmd_GetNetworkInfo,
//	eCmd_IsOTA,
//	eCmd_IMEINumber,
//	eCmd_HttpSend,
//	eCmd_GetTelNumber,
//	eCmd_SETIMEI,
//	eCmd_SPC,
//	eCmd_OTAOpen,
//	eCmd_GPSStart,
//	eCmd_GPSStop,
//	eCmd_GetDNSQ,
//	eCmd_OpenSocket,
//	eCmd_SendSocket,
//	eCmd_CloseSocket,
//	eCmd_CheckSocket,
//	eCmd_MaxPowerOn,
//	eCmd_MaxPowerOff,
//	eCmd_AutoWakeUp,
//	eCmd_UsimCheck,
//	eCmd_SockNetbrup,
//	eCmd_SockNetTrdn,
//	eCmd_GetNetState,
//	eCmd_SetKT,
//	eCmd_ATSECCA,
//	eCmd_ATADCRD,
//	eCmd_SWVERSION,
//	eCmd_NetBrup,				// 패킷 응용 서비스를 시작 한다.
//	eCmd_NetTrdn,				// 패킷 응용 서비스를 종료 한다.
//
//	eCmd_ATSECCARead,
//	eCmd_ATSECCASave,
//	eCmd_ATSECCADelete,
//	eCmd_ATSECCAAsk,
//	eCmd_odbLock,
//	eCmd_odbLock_check,
//	eCmd_CGPADDR,
//	eCmd_HWRESET,
//	eCmd_FRST,
	eCmd_None,
	eCmd_Max
}eCmdIndex;

typedef enum _eMDResponseState
{
	eMDResponseStateWait = 0,
	eMDResponseStateOK,
	eMDResponseStateFAIL,
	eMDResponseStateNoCarrier,
	eMDResponseStateTimeOut,
	eMDResponseStateCMEError,
	eMDResponseState_ModemRegDenied,
	eMDResponseState_ModemWorkaround,
	eMDResponseState_DoNothing,
	eMDResponseState_NetworkError,
} eMDResponseState;



typedef __packed struct _stMODEM_REP
{
	eCmdIndex cmd_index;
	boolean_t start_rep;
	eMDResponseState eMDResponse;
	uint16_t nResponseLineCount;
}stMODEM_REP;

typedef __packed struct _stAMT_SOCKET_FORM
{
	U8	ucSocketID;
	U32	ucDataType;
	U16 usDataLength;
	U8	*pData;
} stAMT_SOCKET_FORM;

typedef void (*fnParsingCallBack)(uint8_t*, uint32_t);

typedef struct _tbModemNotiList
{
	eNotiCmdIndex		eRxCmdIndex;
	int8_t						*strResponseData;
	fnParsingCallBack	fp;
}tbModemNotiList;

typedef struct _tbModemCmdList
{
	eCmdIndex	eSendCmdIndex;
	int8_t *strSendDataReq;
	int8_t *strResponseData;
	fnParsingCallBack	fp;
}tbModemCmdList;

#define MAX_RETRY_COMM_BUFFER_LEN				1024

typedef struct _stModemRetryInfo
{
	uint8_t aRetryCommBuffer[MAX_RETRY_COMM_BUFFER_LEN];
	eCmdIndex eIndex;
	uint16_t nRetryCommBufferLen;
} stModemRetryInfo;

extern tbModemCmdList g_tbModemCmdList[];
extern stMODEM_REP g_stModem_Rep;
extern uint16_t g_nCurrRcvSMSIndex;
extern uint8_t g_aModemNetworkConnTimeString[36];

boolean_t Md_Make_n_SendPacket(eCmdIndex CmdIdx, uint8_t* CmdData, uint32_t nCmdDataSize);
void MDResOK(uint8_t* pData, uint32_t nLength);
#ifdef RF_COMMON_MODEM
void MDResAGPSEndOK(uint8_t* pData, uint32_t nLength);
#endif

#ifndef RF_COMMON_MODEM  //mod.kks 21.10.25
void MDRecvSystemLoading(uint8_t* pData, uint32_t nLength);
#endif

void MDRecvSystemStart(uint8_t* pData, uint32_t nLength);
void MDRecvSystemInfo(uint8_t* pData, uint32_t nLength);
void MDRecvShutDown(uint8_t* pData, uint32_t nLength);
void MDRecvPBReady(uint8_t* pData, uint32_t nLength);
void MDRecvReadyWrite(uint8_t *pData, uint32_t nLength);
void MDRecvSocketDataReadAvailable(uint8_t *pData, uint32_t nLength);
void MDRecvSocketDataReadAvailable2(uint8_t *pData, uint32_t nLength);

#ifndef RF_COMMON_MODEM  //mod.kks 21.10.25
void MDSetEchoRes(uint8_t* pData, uint32_t nLength);
void MDSetResultRes(uint8_t* pData, uint32_t nLength);
void MDGetSystemInfoRes(uint8_t* pData, uint32_t nLength);

#endif
void MDGetIMEIRes(uint8_t *pData, uint32_t nLength);
void MDGetCCIDInfo(uint8_t* pData, uint32_t nLength);
void MDGetPhoneNo(uint8_t* pData, uint32_t nLength);
void MDContinueResProc(uint8_t* pData, uint32_t nLength);
void MDResNOCarrier(uint8_t* pData, uint32_t nLength);
void MDResERROR(uint8_t* pData, uint32_t nLength);
void MDResCMEERROR(uint8_t* pData, uint32_t nLength);
void MDResCMSERROR(uint8_t* pData, uint32_t nLength);
void MDResGetDateTime(uint8_t* pData, uint32_t nLength);
void MDResDummy(uint8_t* pData, uint32_t nLength);				// 추가 2017-01-17 오후 10:04:00
void MDResNetworkRegistration(uint8_t* pData, uint32_t nLength);
void MDResSignalQuality(uint8_t* pData, uint32_t nLength);
void MDResReadOperatorSelection(uint8_t* pData, uint32_t nLength);
void MDRecvSBC(uint8_t *pData, uint32_t nLength);
void MDRecvSIS(uint8_t* pData, uint32_t nLength);
void MDRecvCMTI(uint8_t* pData, uint32_t nLength);
void MDRecvSCFG(uint8_t* pData, uint32_t nLength);
void MDRecvCGPADDR(uint8_t* pData, uint32_t nLength);
void MDRecvCTZV(uint8_t* pData, uint32_t nLength);
void MDRecvNITZInfo(BYTE* pData, uint32_t nLength);
void MDResGprs(BYTE *pData, unsigned int nLength);
void MDResCPBW(BYTE* pData, uint32_t nLength);
void MDResMonitoringService(BYTE* pData, uint32_t nLength);

#ifdef RF_COMMON_MODEM //mod.kks 21.10.25
void MDRecvprov(BYTE* pData, uint32_t nLength);
void MDResCPIN(BYTE* pData, uint32_t nLength);
void MDRecvCTZU(BYTE* pData, uint32_t nLength);
void MDRecvSISO(BYTE* pData, uint32_t nLength);
void MDRecvSMONI(BYTE* pData, uint32_t nLength);
boolean_t Md_ConActive(uint8_t * state);
#endif

boolean_t Md_SetEcho(uint8_t * state);
boolean_t Md_ResultOnOff(uint8_t * state);
boolean_t Md_ReadSystemInfo(void);
boolean_t Md_SetFlowControl(void);
boolean_t Md_GetIMEI(void);
boolean_t Md_GetDateTime(void);
boolean_t Md_SetDateTime(uint8_t * state);
boolean_t Md_GetCCIDInfo(void);
boolean_t Md_GetPhoneNo(void);
boolean_t Md_Set_3M_Baudrate(void);
boolean_t Md_Set_115200_Baudrate(void);
boolean_t Md_Set_921600_Baudrate(void);
boolean_t Md_Set_ErrorMsgFormat(uint8_t *state);

boolean_t Md_SetInternetConnectionService(uint8_t *state);
boolean_t Md_SetInternetServiceProfile(uint8_t * state);
boolean_t Md_SetInternetServiceHTTPURL(void);
boolean_t Md_SetInternetSocketOpen(uint8_t *state);
boolean_t Md_SetInternetSocketClose(uint8_t * state);
boolean_t Md_SetInternetSocketWrite(uint8_t * state);
boolean_t Md_InternetSocketRead(uint8_t *state);
boolean_t Md_CheckNetworkRegistration(void);
boolean_t Md_CheckPacketDomainRegistration(void);
boolean_t Md_CheckSignalQuality(void);
boolean_t Md_ReadOperatorSelection(void);
boolean_t Md_SetOperatorSelection(uint8_t * state);
boolean_t Md_SetPowerSaveMode(uint8_t * state);
boolean_t Md_DeleteSMSIndex(uint8_t * state);
boolean_t Md_SetSMSFormat(uint8_t * state);
boolean_t Md_SetSMSStorage(uint8_t * state);
boolean_t Md_ReadSMSforStatus(uint8_t * state);
boolean_t Md_SendSMSforStatus(uint8_t * state);
boolean_t Md_SendSMS(uint8_t * state);
boolean_t Md_GetSIND(uint8_t * state);
#ifdef RF_COMMON_MODEM //mod.kks 21.10.27
boolean_t Md_SetGNSS(uint8_t * state);
boolean_t Md_SetGPIODriver(uint8_t * state);
void MDResSGPSC(BYTE* pData, uint32_t nLength); //mod.kks 21.10.27
#endif
boolean_t Md_SetSMSReportConfig(uint8_t * state);
boolean_t Md_SetModemConfigExt(uint8_t * state);
boolean_t Md_GPRSAttachDetach(uint8_t * state);
boolean_t Md_SetFlightMode(uint8_t * state);
bool Md_OpenCloseGPIO(uint8_t * state);
bool Md_SetResetModemGPIO(uint8_t * state);
boolean_t Md_SetCTZRTime(void);
boolean_t Md_SetApn(uint8_t * state);
boolean_t Md_SetRegistPhoneNumber(uint8_t * state);
boolean_t Md_SetRadioAccess(uint8_t * state);
boolean_t Md_SetNetworkAutoResponse(uint8_t * state);
boolean_t Md_GetServiceMonitor(void);
boolean_t Md_GetModemConfigExt(uint8_t * state);

void MDResReadSMSindex(uint8_t* pData, uint32_t nLength);
void MDResReadySendSMS(uint8_t* pData, uint32_t nLength);
void MDResCFUN(uint8_t* pData, uint32_t nLength);
void MDResCPMS(uint8_t* pData, uint32_t nLength);
void MDResSendSMSstate(uint8_t* pData, uint32_t nLength);
void MDResSISI(BYTE *pData, unsigned int nLength);
void MDResSICI(BYTE *pData, unsigned int nLength);
void MDResIMSI(BYTE* pData, uint32_t nLength);

int32_t ModemRecvGetLine(stQueue *pQueue,uint32_t *pLineLength);


double ParsingGetCrd(uint8_t *cReceiveData);
void ParsingGetTelNumberRes(uint8_t *cReceiveData);
void ParsingGetSWVersionRes(uint8_t *cReceiveData);
void ParsingGetImei(uint8_t *cReceiveData);
void ParingSystemInfo(uint8_t *cReceiveData);
void ParsingCGPADDR(uint8_t *cReceiveData);

U32 ReceiveDataTokenParsing(uint8_t cReceiveData[] , uint32_t nPosition);
void Md_ReadAuthenticationFile(uint8_t * state, uint32_t nLength );
void Md_SaveAuthenticationFile(uint8_t * state, uint32_t nLength );
void Md_DeleteAuthenticationFile(uint8_t * state, uint32_t nLength );
void Md_AskAuthenticationFile(void);
void MDResSECCAOK(uint8_t* pData, uint32_t nLength);
void MDResODBLOCK(uint8_t* pData, uint32_t nLength);
void MDResCPGADDR(uint8_t* pData, uint32_t nLength);
unsigned char *GetSMONIData(void);

void Modem_ResponsePrc(uint8_t* pData, uint32_t nLength);


#ifdef RF_COMMON_MODEM
boolean_t Md_SetAGPSFileDelete(void);
boolean_t Md_SetAGPSFileStartSend(uint8_t * state);
void MDRecvSBNW(BYTE* pData, uint32_t nLength);
#endif	//RF_COMMON_MODEM 

#endif
/***************************** END OF FILE ****/

