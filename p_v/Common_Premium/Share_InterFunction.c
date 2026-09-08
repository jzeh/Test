/**
  ******************************************************************************
  * @file    Share_InterFunction.c
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    19-FEB-2014
  * @brief   Manager Share_InterFunction.c module
  ******************************************************************************
 **/


//#define DEBUG_GIT_PTCL_LOG

/* Includes ------------------------------------------------------------------*/
#include "HalHandler.h"

#include "Share_InterFunction.h"
#include "GIT_CanParsingProc.h"
#include "OBD_Manager.h"
#include "OBD_Controller.h"
#include "OBD_Controller_Get.h"
#include "GIT_SensorProc.h"
#include "GIT_InterProtocol.h"
#include "GIT_OemInterface.h"
#include "Modem_comm.h"
#include "GIT_Util.h"
#include "GIT_Gps.h"
#include "FOTA_manager.h"
#include "Power_Manager.h"
#include "GIT_AesEncrypt.h"
#include "aes.h"
#include "HdDebug.h"
#include "GIT_VCI.h"
#include "dukpt_cli.h"
#include "AutolinkConfig.h"
#include "MngModem.h"
#include "MngSystem.h"
#include "MngSystemUtil.h"
#include "AutolinkConfiguration.h"
#include "HdDebug.h"
#include "CanFD_Controller.h"
#include "CanFD_Manager.h"
#include <intrinsics.h>
#include "GIT_BluetoothLowEnergy.h"
#include "GIT_base64.h"
#include "GIT_OemInterface.h"
#include "UARTDMA_Manager.h"


#define OTC_COMMAND_ID                          0x8000

#define OTC_VEHICLE_STATUS						0x03
#define OTC_START                               0x01
#define OTC_RESULT								0x02

#define BT_REMOTE_CTRL_SUCCESS      0x01
#define BT_REMOTE_CTRL_FAIL         0x00
#define BT_REMOTE_CTRL_WAIT         0x02

#define BT_OTC_RESP_SUCCESS              0x01
#define BT_OTC_RESP_FAIL                 0x00

#define DOOR_LOCK								0x00
#define DOOR_UNLOCK								0x01
#define TRUNK_OPEN								0x02
#define SendListCount							30
#define FIELD									1
#define SEND_OK 								0
#define SEND_FAIL								1
#define ACK_OLD_TYPE							0
#define ACK_NEW_TYPE 							1

#define HTTP_URL_FILENAME					"FotaURL.inf"
#define TCP_URL_FILENAME					"TcpURL.inf"
#define TCP_CMD_URL_FILENAME				"TcpCmdURL.inf"
#define HORN_TIME									500

#define OTC_COMM_SIZE 							(2)

//#define AES_ENABLE
//#define AESLOG
#define AES_ENABLE_2
//#define AES_ENABLE_2_LOG

#define BT_BASE_VALUE1 "RunGitau"
#define BT_BASE_VALUE2 "toDCSVCI"
#define BT_BASE_VALUE3 "Authorit"
#define BT_BASE_VALUE4 "R0000000"

#define CTRL_SETTING_SIZE                      8


char* g_pcCTRLFunc[CTRL_SETTING_SIZE]=
{
	"ENGINE_ON",
	"AIRCON_ON_WITH_TEMP",
	"AIRCON_VENT",
	"ENGINE_OFF",
	"DOOR_LOCK",
	"DOOR_UNLOCK",
	"HAZARD_ON",
	"HORN_ON",
};


#if defined(PROTOCOL18)
stOTC_PTCL_PAYLOAD           g_stOTCStatusPayload;
stRemote_PTCL_PAYLOAD        g_stRemotePayload;
stRemote_PTCL_PAYLOAD        g_stReqRemotePayload; //dahae
stRemote_PTCL_PAYLOAD        g_stResRemotePayload; //dahae
stAIRCON_PTCL_PAYLOAD        g_stAirconPayload;
unsigned char                g_ucBTControlKey[16];
extern stActuatorAirConData	 g_stActuatorAirConData[];
extern bool                  g_bCtrlFuncParsingFail;
extern stActuatorCtrlData    g_stActuatorCtrlData[];
extern unsigned char         g_ucTotAirCnt;
extern stMsgHandlerData      m_stMsgHdData; // dahae
#endif

extern stServerUrl g_stServerUrl;
extern unsigned char m_carrIv[];

extern stCFDControl m_stCFDCtrl;
stCANFD_BT_UPDATE_INFO g_stCFD_BT_Update_Info;

stGpsInfo 					g_stGpsInfo;
int 						g_DeviceInfoTimerIndex;
stDeviceInfo 				g_stDeviceInfo;
eLockState 					g_eLockStatus=eLOCK_STATE_LOCK;
eBTLockState 				g_eBTLockStatus;
stDownloadStartReq			g_DownloadInfo;			// 다운로드하는 f/w 또는 DB 정보를 임시로 저장하는 변수
stDownloadFileReq			g_DownloadFileInfo;		// Selftest f/w 업데이트 시 사용
eCommType 					g_eSaveCommType;		//직전에 받은 통신타입
unsigned short int 	g_SaveFunctionID;				//직전에 받은 펑션ID
unsigned char g_ucDoorCheckFlag;
unsigned char g_ucSaveReservedNumber[RESERVED_INDEX_LENGTH];
stReservInfo g_stReservInfo[RESERVED_INDEX_MAX];
//stMasterInfo g_stMasterKey[MAX_MASTER_ID];
unsigned char g_strVinNumber[17];
bool g_bAPNFlag = false;
bool g_bCanFD_FOTA_Check = false;  
unsigned char g_Check_Update = eCANFD_UPDATE_INSTALLATION_NONE;

bool g_bFWUpdateFlagFromBLE = FALSE;

int g_iTimerResetTimeOutCallback = -1;
int g_iTimerSleepTimeOutCallback = -1;

int	g_iTimerled1000msCallback = -1;

bool ucVehicleControlBlock = FALSE;
uint32_t g_nSrcAddress; //FW, DB Download Address

// SPARROW Penta Security 적용을 위한 변수
unsigned char g_ucRandValue[16];

unsigned char arrOutRandomData[65];
unsigned char arrOutHashData[65];
unsigned char arrOutSignData[513];
unsigned char g_ucRandomKey[32];

stBTBaseKeyInfo g_stBTBaseKeyInfo;
//MONI 20180518
//extern const unsigned char g_KMS_Publickey[];

unsigned char g_ucUSIMServerPhoneNo[20];
unsigned int  g_uiUSIMCountryCode;
unsigned char g_ucUpdateProgress=0;

eWakeUpPinState	g_eWakeUpPinStatus;

unsigned char g_ucSendingCount = 0;
unsigned short g_usSendingBytes = 0;
unsigned char g_ucCountModemDeniedFile = 0;
unsigned char g_ucCountModemNotCommunicationFile = 0;
unsigned char g_ucCountModemCmeErrorFile = 0;

char g_BTModemStateCollection[MAX_MODEMSTATEBUFFER_SIZE*sizeof(MODEM_STATE_COLLECTION)];

//#define INFINITE_CANFD_UPDATE

#if defined(INFINITE_CANFD_UPDATE)
extern stAutolinkConfigData m_stAutolinkConfigData;
#endif
//소켓에 대한 처리
//extern  U8 g_ucSendListForDataType[30];

//주기정보 전송처리
extern stHalRTC_TimeTypeDef g_iSaveTime ;
extern bool g_bIsCallDenialMode;
extern stMODEM_REP 	g_stModem_Rep;
extern bool g_bIsReceivedNetKTAck;

extern U8 g_arrYUJINTestIdx[2];
extern bool g_bFuelLevelCheckFlag;
#if 1 // James Jean 2018/11/14
extern unsigned int g_uiAutoVinReqPacketIdx;
extern bool g_bAutoVinNegativeResponse;
#endif
extern eCommType		g_InputCommType;
extern uint16_t	            g_u16UpdateFileCheckSum;
extern uint32_t 	        g_u32UpdateFileSize;
extern stBTInfo g_LocalBTInfo;
extern int g_nCANTransmitLedDelayCount;
extern unsigned int g_unBkramSwResetSignal;

unsigned int FineReadCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
int ALPU_Normal_Check();
int MakeDummyFile();
void Send2BTRemoteCtrlResEncPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stRemote_PTCL_PAYLOAD *pstRemoteCtrlPayload, unsigned short usCommandID, unsigned int eInCommType);
void Send2BTOTCResPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stOTC_PTCL_PAYLOAD *pstOTCPayload, unsigned char ucControlID, unsigned int eInCommType);
void Send2BTRemoteCtrlResPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stRemote_PTCL_PAYLOAD *pstRemoteCtrlPayload, unsigned short usCommandID, unsigned int eInCommType);
bool SendModemCommand(modem_process_fn processFunction);

extern void setGPSData(unsigned char *pPayload);
extern void GPS_DisableCommunication(void);
extern void SetRegModemProcessFunction(modem_process_fn processFunction, modem_process_fn resultFunction, MODEM_RESPONSE_TYPE eType);
extern void GPS_DisableCommunication(void);
extern void GPS_EnableCommunication(void);
extern uint32_t UART4Available(void);
extern void SetupForInterruptforImpulse(bool bWomActive, bool bGyroActive, unsigned char ucValue, bool bHelpInterrupSignal);
extern void printdump( char *str, unsigned char *data, size_t len );
extern void SetModemActive(bool bActive);
extern void SetWDTReset(uint16_t msDelay);
extern unsigned long Oem_GetBattVoltage(char adcx);
extern uint16_t SearchInfoString(char *pSrcString, char *pRetString, uint16_t nMaxLen);
extern void SetSensorState(eSENSOR_STATE state);
extern void RequestForwardingSmartKey(boolean_t bReqForwardingSmartKey);

extern boolean_t GetForwardingSmartKey2BtFlag();
extern void ProcessMDResponse();
extern double g_dSavedGpsLat;
extern double g_dSavedGpsLon;
extern unsigned char Get_DoorLock(void);
extern unsigned short Get_RPM(void);
extern eVEHICLE_STATE Get_VehicleStatus(void);
extern int GetPublickey(char* parrPublicKey,int* pnPublicKeySize);
extern void SelftestProcessMDResponse();
extern void ReqForcelyModemReset();
extern void SetVINCode(U8 * inputVIN);
extern uint32_t GetUTCTime();
extern void MeasureGyroAngle();
extern boolean_t DelInternalFlashofDB(int eCurrFotaFileType);
extern MODEM_STATE_STRUCTURE g_ModemStateStructure;
extern MODEM_STATE_COLLECTION g_ModemStateMessage;

stGIT_INTER_FUNCTIONS g_tbGITInterPtclFunctions[] =
{
	{0x1001, GitPassThruDisconnect},	//ok
	{0x0145, GitGetCurrentVCIMode},
	{0x0146, GitGetCertifyVCI},
	{0x0343, GitGetCertifyVCI},	//dummy
	{0x0155, GitGetCertifyVCI},	//dummy
	{0x0175, Git_FWUpdate_Start},
	{0x0245, Git_FWModeChange},		//ok
	{0x0275, Git_FWUpdate_Recv},
	{0x0375, Git_FWUpdate_End},
	{0x0475, Git_DeviceReset},
	{0xF010, DCS_DB_INFO_Res},
	{0x1301, Git_MainBoard_Test},
	{0x1302, Git_ModemBoard_Test},
	{0x1303, GitReadSerial},				//ok
	{0x1304, GitReadHiddenSerial},
};

//0146이전에 체크 1303,0145,1306
stBT_INTER_FUNCTIONS g_tbBTInterPtclFunctions[] =
{
	{0xFEF0, BTGetInfoRes},										// 단말기 정보 요청
	//{0xFEF1, BTSetReserveIndexRes},					// 핸드폰 예약 인덱스 전송	//161101 현재 미사용
//	{0xFEF2, BTGetSecurityPassRes},						// 보안승인 요청
	{0xFEF3, BTSetDoorControlRes},						// 도어 제어 요청
	{0xFEF4, BTSetRebootRes},									// REBOOT
	{0xFEF5, BTGetAccRes},										// ACC 전원 상태값
	{0xFEF6, BTGetDeviceInfoRes},							// Device 상태값
//	{0xFEFF, BTSetTestInfoRes},								// BT테스트용 세팅
    {0x0175, BTFWUpdateRes},                                //FW, DB 업데이트 시작 genebe
    {0x0275, BTFWUpdateRecv},
    {0x0375, BTFWUpdateEnd},
    {0x0475, BTFWDeviceReset},
    {0x1303, GitReadSerial},				//ok
	{0x1304, GitReadHiddenSerial},
    {0xF010, DCS_DB_INFO_Res},
	{0xF030, CANFD_Info_Res},
	{0x1105, BTGetAutoVIN},
    {0x0145, GitGetCurrentVCIMode},
#if defined(AES_ENABLE_2)
	{0x0146, Git_GetCertifyVCI},
#else
    {0x0146, GitGetCertifyVCI},
#endif
    {0x0147, BTSetAutolinkPType},    //오토링크 프리미엄 플릿용 일반용 구분
    {0x0148, BTGetCanFDAdaptorType},
	{0x0150, BTSetCARType},
	{0x0151, BTCheckModemWakeup},
    {0xF040, BTSetRtcTime},
    {0x1306, BTTERMINALInfoRes},                                //터미널 정보(차량 VIN, 유심번호, 유심전화번호)
    {0x0402, BTSetGPSInfo},                                  //GPS정보
    {0x2301, BTDoor_UnlockRes},
    {0x2302, BTDoor_LockRes},
    {0x2307, BTSensor_InitRes},
    {0x2311, BTDoor_UnlockStatusRes},
    {0x2312, BTDoor_LockStatusRes},
    {0x2313, BTEngine_StatusRes},
	{0xC010, BTLockVinRes},
    {0xC020, BTUnLockVinRes},
    {0x2308, BTCompleteRes},            // finish installation precedure
	{0x2309, BTApnSettingRes},
#if defined(PROTOCOL18)
	{0x2320, BTOTCnRemoteCtrl}, //dahae
	{0x2401, BTCtrlFuncSettingInfo}, //dahae
	{0x2402, BTAirConStatusRes}, // dahae
	{0x2310, BTGetDeviceTime}, //dahae
#endif
	{0x2420, BT_EncryptRemoteControl},
	{0xFC01, BTCheckURL},
	{0xFC02, BTChangeURL},
	{0xFC03, BTChangeSerialNumber},
	{0xFC04, BTDeleteVin},
	{0xFC05, BTCheckVersion},
	{0xFC06, BTChageVersion},
	{0xFC07, BTModuleInit},
    //{0xFC08, BTGetModuleLog},
    //{0xFC18, BTGetModuleLogDataSize},
	{0xFC09, BTCheckSerialNumber},
	{0xFC10, BTSetServiceType},
#if defined(PROTOCOL25)
    {0x3080, BTGetInstallationNetworkCheck},
    {0x3081, BTInstallationNetworkFailCode},
#endif	
};

#if defined(PROTOCOL25)
// CGREG 1,5 였으나 Network Check Fail 발생 시 Fail Code 전송
void BTInstallationNetworkFailCode(void *pInterPtcl, unsigned int eInCommType)
{

    stBT_PTCL_PAYLOAD *stBTPayload = (stBT_PTCL_PAYLOAD*)pInterPtcl;
    MODEM_STATE_STRUCTURE *stMDMStatus = &g_ModemStateStructure;
    unsigned int iPayLoadDataLen=0;
    int count =0;
 
    if(stMDMStatus->rdindex > stMDMStatus->wrindex)  //Fail Code Count
    {
        count = MAX_MODEMSTATEBUFFER_SIZE - stMDMStatus->rdindex + stMDMStatus->wrindex;
        unsigned char arrTmpBuff[MAX_MODEMSTATEBUFFER_SIZE*sizeof(MODEM_STATE_COLLECTION)+1]={0,};
        
        while(count)
        {
            GetModemStateBuffer(stMDMStatus->ModemStateCollection);
            memcpy(arrTmpBuff+iPayLoadDataLen,(void*)stMDMStatus->ModemStateCollection,sizeof(MODEM_STATE_COLLECTION)); 
            iPayLoadDataLen += sizeof(MODEM_STATE_COLLECTION);
            count--;
        }
        SendBTPtclResponse((stBT_PTCL_PAYLOAD*)stBTPayload, arrTmpBuff, iPayLoadDataLen, (eCommType)eInCommType);  
    }
    else if(stMDMStatus->rdindex < stMDMStatus->wrindex)  //Fail Code Count
    {
        count = stMDMStatus->wrindex - stMDMStatus->rdindex;
        unsigned char arrTmpBuff[MAX_MODEMSTATEBUFFER_SIZE*sizeof(MODEM_STATE_COLLECTION)+1]={0,};
        
        while(count)
        {
            GetModemStateBuffer(stMDMStatus->ModemStateCollection);
            memcpy(arrTmpBuff+iPayLoadDataLen,(void*)stMDMStatus->ModemStateCollection,sizeof(MODEM_STATE_COLLECTION)); 
            iPayLoadDataLen += sizeof(MODEM_STATE_COLLECTION);
            count--;
        }
        SendBTPtclResponse((stBT_PTCL_PAYLOAD*)stBTPayload, arrTmpBuff, iPayLoadDataLen, (eCommType)eInCommType);
    }
    else if(stMDMStatus->rdindex == stMDMStatus->wrindex){}
}

// CGREG 값 전송
void BTGetInstallationNetworkCheck(void *pInterPtcl, unsigned int eInCommType)
{
    unsigned char cResult[33] = {0,};
    stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
    
    // guid
    memcpy((char *)MessageManagerData.aRemoteControlGUID, (char *)&pPayloadPtcl->pPayload[0], MAX_GUID_LENGTH);
    
    memcpy(&cResult[0], (char *)&MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
    memcpy(&cResult[32], (unsigned char *)&ModemManagerData.eNetworkRegStatus, sizeof(ModemManagerData.eNetworkRegStatus)); //CGREG값 전송

    if(cResult[32] == 1 || cResult[32] == 5 )  // 망 연결시 
    {
        //서버로 데이터 전송 
        Send2MngSysMsg(eMngModem, eReqReport, eR_ReportInstallationNetworkCheck, (stCarReport*)NULL ,0);
    }
    SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}
#endif
#if 0
void BTGetModuleLogDataSize(void *pInterPtcl, unsigned int eInCommType)
{
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;

	unsigned short usSendingDataSize = 0;
	unsigned short usAllDataSize = 0;
	unsigned char ucFileIndex = 0;
	unsigned char ucCount = 0;
	unsigned char ucFileSendingCount = 0;

	memcpy(&usSendingDataSize, &(pPayloadPtcl->pPayload[0]), sizeof(usSendingDataSize));

	ucFileSendingCount = SetFileSendingCount((sizeof(MODEM_STATE_COLLECTION)*MAX_MODEMSTATEBUFFER_SIZE),usSendingDataSize);
		
	//Check File is exist and store File Count
	if(ExistAutolinkModemStatusData(MODEM_DENIED) == FR_OK)				g_ucCountModemDeniedFile = ucFileSendingCount + 1; // + FILE NAME COUNT
	else 																g_ucCountModemDeniedFile = 0;

	if(ExistAutolinkModemStatusData(MODEM_NOT_COMMUNICATION)== FR_OK)   g_ucCountModemNotCommunicationFile = ucFileSendingCount + 1; // + FILE NAME COUNT
	else																g_ucCountModemNotCommunicationFile = 0;

	if(ExistAutolinkModemStatusData(MODEM_CME_ERROR) == FR_OK)			g_ucCountModemCmeErrorFile = ucFileSendingCount + 1; // + FILE NAME COUNT
	else																g_ucCountModemCmeErrorFile = 0;


	//Calculate Count 
	ucCount = g_ucCountModemDeniedFile + g_ucCountModemNotCommunicationFile + g_ucCountModemCmeErrorFile;

	//Set Info
	SetSendingCount(ucCount);
	SetSendingBytes(usSendingDataSize);

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucCount, sizeof(ucCount), eCOMM_TYPE_UART_BT);
}

void BTGetModuleLog(void *pInterPtcl, unsigned int eInCommType)
{
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	
	char *ModemFileData;
	
	unsigned char ucRecieveCount = 0;
	unsigned char ucAllCount = 0;
	unsigned char ucRealCount = 0;
	
	unsigned short usLength=0;
	unsigned char ucRemainLength=0;

	unsigned char ucFileSendingCount = 0;

	ucRecieveCount = pPayloadPtcl->pPayload[0];
	
	eMODEM_ERROR_STATE eErrorState;

	usLength = GetSendingBytes();
	ucAllCount = GetSendingCount();
	
	ModemFileData = malloc(usLength);

	ucRemainLength = (sizeof(MODEM_STATE_COLLECTION)*MAX_MODEMSTATEBUFFER_SIZE) % usLength;
	ucFileSendingCount = SetFileSendingCount((sizeof(MODEM_STATE_COLLECTION)*MAX_MODEMSTATEBUFFER_SIZE),usLength);

	if(ModemFileData == NULL)
	{
		printf("\n\n");
		printf("[%s] InPtcl or OutPtcl malloc Fail !!!!!!!!\n\n", __FUNCTION__);
	}

	memset(ModemFileData,0,usLength);

	if( ucRecieveCount < ucAllCount )
	{
		if( g_ucCountModemDeniedFile != 0 )
		{
			if((ucFileSendingCount+1) == g_ucCountModemDeniedFile)  ucRealCount = 0xFF;
			else													ucRealCount = ucFileSendingCount - g_ucCountModemDeniedFile;

			eErrorState = MODEM_DENIED;
	
			g_ucCountModemDeniedFile--;
		}
		else if( g_ucCountModemNotCommunicationFile != 0 )
		{
			if((ucFileSendingCount+1) == g_ucCountModemNotCommunicationFile)  ucRealCount = 0xFF;
			else															  ucRealCount = (char)(ucFileSendingCount - g_ucCountModemNotCommunicationFile);
			eErrorState = MODEM_NOT_COMMUNICATION;
			
			g_ucCountModemNotCommunicationFile--; 
		}
		else if( g_ucCountModemCmeErrorFile != 0 )
		{
			if((ucFileSendingCount+1) == g_ucCountModemCmeErrorFile)  ucRealCount = 0xFF;
			else												      ucRealCount = (char)(ucFileSendingCount - g_ucCountModemCmeErrorFile);
			
			eErrorState = MODEM_CME_ERROR;
			
			g_ucCountModemCmeErrorFile--;
		}

		usLength = SetDataFunction(eErrorState, ucRealCount, ModemFileData);
	}
	
	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, ModemFileData, usLength, eCOMM_TYPE_UART_BT);
	
	free(ModemFileData);
}
#endif

void BTCheckModemWakeup(void *pInterPtcl, unsigned int eInCommType)
{	
	unsigned char ucResult=0;
	
	unsigned int uiFunctionTmr = 0;
	unsigned int uiModemCheckTmr = 0;
	
	boolean_t bModemWakePin = false;

	uiFunctionTmr = Get_Tmr();
	uiModemCheckTmr = Get_Tmr();
	

	while(!ucResult)
	{
		if(Get_TmrDelta(Get_Tmr(),uiFunctionTmr) >= 3000) break; 
		
		if(Get_TmrDelta(Get_Tmr(),uiModemCheckTmr) >= 1) 
		{
			bModemWakePin = HalGPIOGetStatus(GPIO_MO_WAKE);
			if(bModemWakePin == true )	uiModemCheckTmr = Get_Tmr();
			else 		               	ucResult = 1;
		}	
	}

	printf("%s] Modem Wakeup PIN : %d, Result : %d\r\n",__FUNCTION__,bModemWakePin,ucResult);
	
	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucResult,sizeof(ucResult), (eCommType)eInCommType);
}


void BTSetCARType(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucResult=0;
    stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	uint8_t ucPACVType = PA;
	uint8_t ucAutovinCANLine = HIGHCAN1;

	if(pPayloadPtcl->pPayload[0] == CV) ucPACVType = CV; //payload[0] PA : 0, CV : 1
	
	ucAutovinCANLine = 200 + pPayloadPtcl->pPayload[1]; // payload[1] HIGHCAN1 : 1, HIGHCAN2, :2, HIGHCAN3 : 3, LOWCAN : 4
	
	SetAutolinkConfigProperty(eAutoLinkConfig_PACVType,(void*)&ucPACVType);
	SetAutolinkConfigProperty(eAutoLinkConfig_AutovinCANLine,(void*)&ucAutovinCANLine);
	
	if( ucPACVType == CV )
	{
		SetPACVType(CV);
		if( ucAutovinCANLine == HIGHCAN1 )
		{
			SetCANBaudrate(eCAN_250KBPS);
		}
	}
	
	ucResult=1;
	
	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucResult,sizeof(ucResult), (eCommType)eInCommType);
}

void BTSetServiceType(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucResult=0;
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;

    //hexdump(pPayloadPtcl->pPayload,32);

    if(pPayloadPtcl->pPayload[0] == 0x02 )
    {
        g_FirmwareInfo.nServiceType = DCS_Fleet;
		SetFirmwareInfo(&g_FirmwareInfo);
        ucResult = 1;
#if defined(FEATURE_EXTENSION_BOARD)
		Uart8_Baudrate_Set(115200);
#elif defined(PROTOCOL14)
        Uart8_Baudrate_Set(9600);
#endif
    }
    else if(pPayloadPtcl->pPayload[0] == 0x01 )
   {
        g_FirmwareInfo.nServiceType = DCS_Retail;
		SetFirmwareInfo(&g_FirmwareInfo);
       	ucResult = 1;
        Uart8_Baudrate_Set(115200);
    }

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucResult,sizeof(ucResult), (eCommType)eInCommType);
}
void BTCheckURL(void *pInterPtcl, unsigned int eInCommType)
{
	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&g_stServerUrl.Url,strlen(g_stServerUrl.Url), (eCommType)eInCommType);
}

void BTChangeURL(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucResult = 0;

	unsigned char AutolinkEncryptKey[MAX_ENCRYPT_KEY_LENGTH] = {0};
	stURLInfo stTemp;
	unsigned char strTemp[MAX_SERVER_URL_LENGTH+8+1]={0,};
	unsigned char strTempURL[MAX_SERVER_URL_LENGTH+8+1]={0,};
	int nEncryptResult=0;

	stBT_PTCL_PAYLOAD *pPayloadPtcl  = (stBT_PTCL_PAYLOAD*)pInterPtcl;

	memcpy(strTempURL,pPayloadPtcl->pPayload, (pPayloadPtcl->DataLength)-6);

	memcpy(g_stServerUrl.Url, strTempURL, sizeof(strTempURL));

	memcpy(g_stServerUrl.Path,	HTTP__MESSAGE_PATH,	sizeof(HTTP__MESSAGE_PATH));

	sprintf((char*)strTemp,"https://%s",g_stServerUrl.Url);

	memset((char*)&stTemp,0x00,sizeof(stTemp));

    GetDefaultEncryptKey((char *)AutolinkEncryptKey,true);

    // we will apply encryp process after we apply damo encryption process.
    nEncryptResult = DAMO_CRYPT_AES_EncryptEx((unsigned char *)stTemp.strUrl, (size_t*)&stTemp.nEncryptedLength,
			        (const unsigned char*)&strTemp, MAX_SERVER_URL_LENGTH,
			        AutolinkEncryptKey, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, m_carrIv);

    if( nEncryptResult == 0 )	//if success
	{
		g_FirmwareInfo.m_stURLInfo.nEncryptedLength = stTemp.nEncryptedLength;
		g_FirmwareInfo.m_stURLInfo.nPreamble = 0xFE000728;
		memcpy(	g_FirmwareInfo.m_stURLInfo.strUrl,stTemp.strUrl,MAX_SERVER_URL_LENGTH+AES_PADDING_LENGTH);	//패딩포함
		SetFirmwareInfo(&g_FirmwareInfo);

		ModemManagerData.nInternetConnectionProfileId = MODEM_INTERNET_CONNECTION_ID_HTTP_COMM;
    	ModemManagerData.nInternetServiceProfileId = MODEM_INTERNET_SERVICE_ID_HTTP_COMM;

		strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].ahcProp, DEFAULT_HTTP_MESSAGE_HEADER);
        strcpy((char *)ModemManagerData.stInternetServiceProfile[ModemManagerData.nInternetServiceProfileId].aURL, g_stServerUrl.Url);

		ucResult = 1;
	}

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucResult , sizeof(ucResult), (eCommType)eInCommType);
    //SetupForSleep();
}

void BTChangeSerialNumber(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucResult = 0;
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	unsigned char ucSerial[SIZE_SERIAL_NUMBER+1] = {0,};
	stCarReport report;

    memset((char*)&report,0,sizeof(report));

	memcpy(ucSerial, &pPayloadPtcl->pPayload[0], (pPayloadPtcl->DataLength)-6);

	SetFWSerialNumber((char *)ucSerial);

	if( memcmp(GetFWSerialNumber(),ucSerial,SIZE_SERIAL_NUMBER)==0 )
	{
		ucResult=1;
		Send2MngSysMsg(eMngSys,eReqIpek,eIpekPhase1,(stCarReport *)&report,0);
	}

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucResult , sizeof(ucResult), (eCommType)eInCommType);
}

void BTDeleteVin(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucResult = 0;
	unsigned char ucAllowNewVin = 1;
	char cSystemVin[128]={0,};
	char cSystemAllowNewVin = 0;

	SetVINCode(STR_DEFAULT_VIN);

	SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)STR_DEFAULT_VIN);
	SetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,(void*)&ucAllowNewVin);

	GetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)cSystemVin);
	GetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,(void*)&cSystemAllowNewVin);

	if((memcmp(cSystemVin,STR_DEFAULT_VIN,sizeof(STR_DEFAULT_VIN)) == 0) && (cSystemAllowNewVin == 1))
	{
		ucResult = 1;
	}

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucResult, sizeof(ucResult), (eCommType)eInCommType);
}

void BTCheckVersion(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucResult[14]={0,};

	memcpy(&ucResult[0], (unsigned char*)&g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion, sizeof(g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion));
	memcpy(&ucResult[2], (unsigned char*)&g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion, sizeof(g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion));
	memcpy(&ucResult[4], (unsigned char*)&g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion, sizeof(g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion));
	memcpy(&ucResult[6], (unsigned char*)&g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion, sizeof(g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion));
	memcpy(&ucResult[8], (unsigned char*)&g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion, sizeof(g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion));
	memcpy(&ucResult[10],(unsigned char*)&m_stCFDCtrl.stVer.usBlVer, sizeof(m_stCFDCtrl.stVer.usBlVer));
	memcpy(&ucResult[12],(unsigned char*)&m_stCFDCtrl.stVer.usAppVer, sizeof(m_stCFDCtrl.stVer.usAppVer));

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)ucResult, sizeof(ucResult), (eCommType)eInCommType);
}

void BTChageVersion(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucResult = 0;

	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;

	switch(pPayloadPtcl->pPayload[0])
	{
		case 1:
			g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion = pPayloadPtcl->pPayload[1]|(pPayloadPtcl->pPayload[2]<<8);
			SetFirmwareInfo(&g_FirmwareInfo);
			ucResult = 1;
			break;
		case 2:
			g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion = pPayloadPtcl->pPayload[1]|(pPayloadPtcl->pPayload[2]<<8);
			SetFirmwareInfo(&g_FirmwareInfo);
			ucResult = 1;
			break;
		case 3:
			g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion = pPayloadPtcl->pPayload[1]|(pPayloadPtcl->pPayload[2]<<8);
			SetFirmwareInfo(&g_FirmwareInfo);
			ucResult = 1;
			break;
		case 4:
			g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion = pPayloadPtcl->pPayload[1]|(pPayloadPtcl->pPayload[2]<<8);
			SetFirmwareInfo(&g_FirmwareInfo);
			ucResult = 1;
			break;
		case 5:
			g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion = pPayloadPtcl->pPayload[1]|(pPayloadPtcl->pPayload[2]<<8);
			SetFirmwareInfo(&g_FirmwareInfo);
			ucResult = 1;
			break;
		case 6:
			if(CFD_GetCanFDAdapter())
			{
				m_stCFDCtrl.stVer.usBlVer = pPayloadPtcl->pPayload[1]|(pPayloadPtcl->pPayload[2]<<8);
				CFD_SetCanFDVersion(m_stCFDCtrl.stVer.usBlVer,m_stCFDCtrl.stVer.usAppVer,0,0);
				ucResult = 1;
			}
			break;
		case 7:
			if(CFD_GetCanFDAdapter())
			{
				m_stCFDCtrl.stVer.usAppVer = pPayloadPtcl->pPayload[1]|(pPayloadPtcl->pPayload[2]<<8);
				CFD_SetCanFDVersion(m_stCFDCtrl.stVer.usBlVer,m_stCFDCtrl.stVer.usAppVer,0,0);
				ucResult = 1;
			}
			break;
		default:
            break;
	}

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucResult , sizeof(ucResult), (eCommType)eInCommType);
}

void BTModuleInit(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucResult = 0;

	g_FirmwareInfo.nSignal = FIRMWARE_SIGNAL;

	// Bootloader 기본 정보 설정
	g_FirmwareInfo.AppProperty[eApp_Bootloader].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_Bootloader].wFirmwareSize = 100;//FIRMWARE_INFO_ADDRESS-BOOTLOADER_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_Bootloader].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Bootloader].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion = 1;

	// /APP 기본 정보 설정
	g_FirmwareInfo.AppProperty[eApp_Application].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_Application].wFirmwareSize = 100;//UPDATE_TMP_ADDRESS - APPLICATION_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_Application].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_Application].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion = 1;

	// Master DB 기본 정보 설정
	DelInternalFlashofDB(eFOTA_FILE_TYPE_DIAGNOSIS_MASTER);
	g_FirmwareInfo.AppProperty[eApp_MasterDB].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_MasterDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_MasterDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion = (uint16_t)DEFAULT_FW_VERION;

	// Slave DB 기본 정보 설정
	DelInternalFlashofDB(eFOTA_FILE_TYPE_DIAGNOSIS_SLAVE);
	g_FirmwareInfo.AppProperty[eApp_SlaveDB].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize = 100;//APPLICATION_ADDRESS-CAR_SLAVE_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_SlaveDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion = (uint16_t)DEFAULT_FW_VERION;

	// Driving DB 기본 정보 설정
	DelInternalFlashofDB(eFOTA_FILE_TYPE_DIAGNOSIS_CONTROL);
	g_FirmwareInfo.AppProperty[eApp_ControlDB].nSignal = FIRMWARE_APP_SIGNAL;
	g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize = 100;//CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS;
	g_FirmwareInfo.AppProperty[eApp_ControlDB].nCheckSum = (uint16_t)FIRMWARE_APP_SIGNAL;
	memcpy((char*)g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME);
	g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion = (uint16_t)DEFAULT_FW_VERION;


	g_FirmwareInfo.SwitchingInfo.iApplMode = eApp_Application;
	g_FirmwareInfo.SwitchingInfo.iStatus = eFW_SWITCH_COMPLETE;

	memset((char*)&g_FirmwareInfo.m_strVIN, 0x00,VIN_CODE_SIZE + 1);
	memcpy((char*)&g_FirmwareInfo.m_strVIN, STR_DEFAULT_VIN, sizeof(STR_DEFAULT_VIN));

	g_FirmwareInfo.bModemActive = FALSE;

	SetFirmwareInfo(&g_FirmwareInfo);

	ucResult = 1;

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucResult , sizeof(ucResult), (eCommType)eInCommType);
}
void BTCheckSerialNumber(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucSerial[SIZE_SERIAL_NUMBER+1] = {0,};

	memcpy(ucSerial,GetFWSerialNumber(),sizeof(ucSerial));

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)ucSerial,SIZE_SERIAL_NUMBER, (eCommType)eInCommType);

}



void ProcessBTInterProtocol(stBT_PTCL_PAYLOAD *pInterPtcl, eCommType eInCommType)
{
	unsigned int i, uiLoopCnt;

	uiLoopCnt = sizeof(g_tbBTInterPtclFunctions)/sizeof(g_tbBTInterPtclFunctions[0]);

	//GITDebugPrintf("! BTF : 0x%04X !\n", pInterPtcl->FunctionID);

#if false
	for(int j=0;j<pInterPtcl->DataLength-6;j++){
		printf("%02X ", pInterPtcl->pPayload[j]);
	}
	printf("\n");
#endif

	for ( i=0; i<uiLoopCnt; i++ )
	{
		if ( (pInterPtcl->FunctionID == g_tbBTInterPtclFunctions[i].uiFunctionID) &&
			 (g_tbBTInterPtclFunctions[i].fnPayloadCB != NULL) )
		{
#if defined(DEBUG_GIT_PTCL_LOG)
			printf("BTIN:%04X\r\n",pInterPtcl->FunctionID);
#endif
			g_tbBTInterPtclFunctions[i].fnPayloadCB(pInterPtcl, eInCommType);
			break;
		}
	}

	if ( i >= uiLoopCnt )
		GITDebugPrintf("!!!not Define Input Function ID : 0x%04X !!!\r\n", pInterPtcl->FunctionID);
}

void ProcessGITInterProtocol(stGIT_PTCL_PAYLOAD *pInterPtcl, eCommType eInCommType)
{
	unsigned int i, uiLoopCnt;

	uiLoopCnt = sizeof(g_tbGITInterPtclFunctions) / sizeof(g_tbGITInterPtclFunctions[0]);


	for(i = 0; i < uiLoopCnt; i++) {
		if((pInterPtcl->FunctionID == g_tbGITInterPtclFunctions[i].uiFunctionID) && (g_tbGITInterPtclFunctions[i].fnPayloadCB != NULL)) {
			g_tbGITInterPtclFunctions[i].fnPayloadCB(pInterPtcl, eInCommType);
			return;
		}
	}

	if ( i >= uiLoopCnt )
		GITDebugPrintf("!!!not Define Input Function ID : 0x%04X !!!\r\n", pInterPtcl->FunctionID);
}

void MakeRandomKey(unsigned char *ucTempBuff)
{
	unsigned char ucTemp=0;
	int i=0;

	srand(Get_Tmr());
	for(i=0;i<16;i++)
	{
		ucTemp = (rand()%255)+1;
		ucTempBuff[i] = g_ucRandomKey[i] = ucTemp;
	}
	for(i=0;i<15;i++)
	{
		g_ucRandomKey[i+16] = g_ucRandomKey[i+1];
	}
	g_ucRandomKey[31] = g_ucRandomKey[0];
#if defined(AESLOG)
	printf("g_ucRandomKey:");
	for(i=0;i<sizeof(g_ucRandomKey);i++)
	{
		printf("%02X ",g_ucRandomKey[i]);
	}
	printf("\r\n");
#endif
}

void MakeBTBaseKey()
{
	unsigned char ucTemp=0;
	int i=0;

	srand(Get_Tmr());
	for(i=0;i<MAX_BASEKEY_SIZE;i++)
	{
		ucTemp = (rand()%255)+1;
		g_stBTBaseKeyInfo.ucBTBaseKey[i] = ucTemp;
	}

#if defined(AES_ENABLE_2_LOG)
	printf("g_ucBaseKey:");
	for(i=0;i<sizeof(g_stBTBaseKeyInfo.ucBTBaseKey);i++)
	{
		printf("%02X ",g_stBTBaseKeyInfo.ucBTBaseKey[i]);
	}
	printf("\r\n");
#endif
}

void GetBTBaseKey(uint8_t * pKey)
{
	memcpy( pKey,g_stBTBaseKeyInfo.ucBTBaseKey, MAX_BASEKEY_SIZE );
}

void ClearBTBaseKey()
{
	memset(g_stBTBaseKeyInfo.ucBTBaseKey,0x00,MAX_BASEKEY_SIZE);
}

void GitGetCertifyVCI(void *pInterPtcl, unsigned int eInCommType)
{
#if defined(AES_ENABLE)
	if(eInCommType == eCOMM_TYPE_UART_BT){
		aes256_context ctx;
		U8 ucVciSerial[8];
		U8 ucInputbuf[16];
		U8 ucKey[32] = "RunGitautoDCSVCIAuthoritPP000001";
		stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
		unsigned char ucRetBuff[17];
		unsigned char ucTempBuff[16];

		LOCK_SET_STATE(eLOCK_STATE_LOCK);	//아래 flow 타기전에 lock으로 초기화

		memset(ucInputbuf,0x00,sizeof(ucInputbuf));
		memset(ucVciSerial,0x00,sizeof(ucVciSerial));
		memset(ucRetBuff,0x00,sizeof(ucRetBuff));
		memset(g_ucRandomKey,0x00,sizeof(g_ucRandomKey));
		printf("DataLength : %d\r\n",pPayloadPtcl->DataLength);
		memcpy(ucInputbuf, (void const*)&pPayloadPtcl->pPayload[0], pPayloadPtcl->DataLength-6);
		memcpy(ucVciSerial, g_FirmwareInfo.arrSerialNumber, 8);
		memcpy(&ucKey[24], g_FirmwareInfo.arrSerialNumber, 8);

#if defined(AESLOG)
		printf("Key : ");
		for(int i=0;i<sizeof(ucKey);i++)	printf("%02X ",ucKey[i]);
		printf("\r\n");

		GITDebugPrintf("\r\n pPayloadPtcl:");
		for(U8 ttt=0; ttt<pPayloadPtcl->DataLength-6; ttt++) GITDebugPrintf(" %02X",pPayloadPtcl->pPayload[ttt]);
		printf("\r\n");

		GITDebugPrintf("\r\n ucInputbuf:");
		for(U8 ttt=0; ttt<sizeof(ucInputbuf); ttt++)	GITDebugPrintf(" %02X",ucInputbuf[ttt]);
		printf("\r\n");
#endif
		aes256_init(&ctx, ucKey);
		aes256_decrypt_ecb(&ctx, ucInputbuf);	//Inputbuf에 decrypt해서 나옴
#if defined(AESLOG)
		GITDebugPrintf("\r\n decryptbuf:");
		for(U8 ttt=0; ttt<sizeof(ucInputbuf); ttt++)	GITDebugPrintf(" %02X",ucInputbuf[ttt]);
		printf("\r\n");
#endif
		if( strncmp((char const*)ucInputbuf, "AUTOLINK", 8) == 0)
		{
			if( strncmp((char const*)&ucInputbuf[8], (char const*)ucVciSerial, 8) == 0)
			{
				MakeRandomKey(ucTempBuff);
#if defined(AESLOG)
				printf("SendKey : ");
				for(U8 ttt=0; ttt<sizeof(ucTempBuff); ttt++)	GITDebugPrintf(" %02X",ucTempBuff[ttt]);
				printf("\r\n");
#endif
				aes256_encrypt_ecb(&ctx,ucTempBuff);
#if defined(AESLOG)
				printf("SendKeyEnc : ");
				for(U8 ttt=0; ttt<sizeof(ucTempBuff); ttt++)	GITDebugPrintf(" %02X",ucTempBuff[ttt]);
				printf("\r\n");
#endif
				ucRetBuff[0] = 2;	//성공
				memcpy(&ucRetBuff[1], ucTempBuff, sizeof(ucTempBuff));
			}
		}
		else
		{
			ucRetBuff[0] = 1;	//실패
		}
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucRetBuff, sizeof(ucRetBuff), (eCommType)eInCommType);
	}
	else if(eInCommType == eCOMM_TYPE_UART_SELFTEST){
		SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&g_eLockStatus, 1, (eCommType)eInCommType);
	}
#else   //AESLOG
  	if(eInCommType == eCOMM_TYPE_UART_BT){
		aes256_context ctx;
		U8 ucVciSerial[8]="XXXXXXXX";		////시리얼정보가 없을시 'X'로 초기화 한다
		stGIT_PTCL_PAYLOAD *pPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
		char su = 0x02;

		LOCK_SET_STATE(eLOCK_STATE_LOCK);	//아래 flow 타기전에 lock으로 초기화

		U8 ucInputbuf[16];
		U8 uckey[32] = "RunGitautoVCIIIAuthorityW000000";

		git_f_chdir(DIR_ROOT);

		memset(ucInputbuf,0x00,sizeof(ucInputbuf));
		memcpy(ucInputbuf, (void const*)&pPayloadPtcl->pPayload[1], pPayloadPtcl->DataLength-9);
		memcpy(&uckey[24],ucVciSerial,sizeof(ucVciSerial));
#if defined(AESLOG)
		GITDebugPrintf("\r\n ucInputbuf:");
		for(U8 ttt=0; ttt<16; ttt++)
			GITDebugPrintf(" %02X",ucInputbuf[ttt]);
		GITDebugPrintf("\r\n");
#endif
		aes256_init(&ctx, uckey);
		aes256_decrypt_ecb(&ctx, ucInputbuf);
		//aes_encrypt_cbc

		GITDebugPrintf("\r\n decryptbuf:");
		for(U8 ttt=0; ttt<16; ttt++)
			GITDebugPrintf(" %02X",ucInputbuf[ttt]);
		GITDebugPrintf("\r\n");

		if( strncmp((char const*)ucInputbuf, "GitVCIII", 8) == 0) {
			if( strncmp((char const*)&ucInputbuf[8], (char const*)ucVciSerial, 8) == 0) {
				LOCK_SET_STATE(eLOCK_STATE_UNLOCK);		//unlock
				GITDebugPrintf("\r\n g_ucLockStatus is unlock");
			}
		}

	#if defined(DEBUG_GIT_PTCL_LOG)
		GITDebugPrintf("[%s] run LOCK_GET_STATE %d\r\n", __FUNCTION__,LOCK_GET_STATE());
	#endif
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&su /*(unsigned char*)&g_eLockStatus*/, 1, (eCommType)eInCommType);
	}
	else if(eInCommType == eCOMM_TYPE_UART_SELFTEST){
		printf("[%s] run LOCK_GET_STATE %d\r\n", __FUNCTION__,LOCK_GET_STATE());
		SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&g_eLockStatus, 1, (eCommType)eInCommType);
	}
#endif  //AES_ENABLE
}

void Git_GetCertifyVCI(void *pInterPtcl, unsigned int eInCommType)
{
    printf("Git_GetCertifyVCI===\r\n");

	uint8_t ucArrEncryptBtSession[MAX_BASEKEY_SIZE] = {0,};
	char* pStrFwSerial = GetFWSerialNumber();

  	if(eInCommType == eCOMM_TYPE_UART_BT)
    {
		stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
		U8 ucOutputbuf[33] = {0,};
		// su : 02   : success
		// su : else : fail
		char cResult = 0x00;

		LOCK_SET_STATE(eLOCK_STATE_LOCK);	//아래 flow 타기전에 lock으로 초기화

		// ***************************************************************//
		// certification key value
		// format [fixed string : RunGitautoDCSVCIAuthorit][serial number]
		// format [RunGitautoDCSVCIAuthorit][R9999999]
		// format "RunGitautoDCSVCIAuthoritR9999999"
		// ***************************************************************//
		// ***************************************************************//
		// certification data format
		// format [service string][serial string]
		// format [AUTOLINK][R9999999]
		// basic format "GitDCSVCIR9999999"
		// premium format "AUTOLINKR9999999"
		// ***************************************************************//
		// array 0 index is result so me process array withou 0 index
		// length : protcol version(2) + legnth(2) + function id(2) + payload data(n)
		// encrypted data length = length - (protocol verions + length + fucntion id + (0 index) )
		// ***************************************************************//

		// check basic application certify
		if( IsBasicCertificationCorrect(pPayloadPtcl->pPayload,pPayloadPtcl->DataLength) == true )
		{
			LOCK_SET_STATE(eLOCK_STATE_UNLOCK);
			GITDebugPrintf("%s] basic service g_ucLockStatus is unlock\n", __FUNCTION__);

			// success : 02 / fail : else
			cResult = 0x02;
		}

		// check premium applicatino certify
		if( IsPremiumCertificationCorrect(pPayloadPtcl->pPayload,pPayloadPtcl->DataLength) == true )
		{
			LOCK_SET_STATE(eLOCK_STATE_UNLOCK);
			GITDebugPrintf("%s] premium service g_ucLockStatus is unlock\n", __FUNCTION__);

			// success : 02 / fail : else
			cResult = 0x02;
		}

		if( cResult == 0x00 )
		{
			GITDebugPrintf("%s] g_ucLockStatus is lock\n", __FUNCTION__);
		}

#if defined(DEBUG_GIT_PTCL_LOG)
	GITDebugPrintf("[%s] run LOCK_GET_STATE %d\r\n", __FUNCTION__,LOCK_GET_STATE());
#endif
//		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult /*(unsigned char*)&g_eLockStatus*/, 1, (eCommType)eInCommType);

		MakeBTBaseKey();

		GetAESEncryptBluetoothData(g_stBTBaseKeyInfo.ucBTBaseKey,MAX_BASEKEY_SIZE,ucArrEncryptBtSession);

        GetBTBaseKey(&ucOutputbuf[1]);
		memcpy(&ucOutputbuf[1],ucArrEncryptBtSession,sizeof(ucArrEncryptBtSession));

		ucOutputbuf[0] = LOCK_GET_STATE();
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucOutputbuf, sizeof(ucOutputbuf), (eCommType)eInCommType);
	}
	else if(eInCommType == eCOMM_TYPE_UART_SELFTEST){
		printf("[%s] run LOCK_GET_STATE %d\r\n", __FUNCTION__,LOCK_GET_STATE());
		SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&g_eBTLockStatus, 1, (eCommType)eInCommType);
	}

}

void GetAESEncryptBluetoothData(uint8_t* pucArrSrc, uint8_t ucSrcLength, uint8_t* pucArrDst)
{
	aes256_context ctx;

	//base key value : "RunGitautoDCSVCIAuthoritR0000000";
	U8 ucBaseValue[32] = {0,};
	U8 ucInputbuf[64] = {0,};

	char* pStrFwSerial = GetFWSerialNumber();

	memcpy(&ucBaseValue[0],BT_BASE_VALUE1,8);
	memcpy(&ucBaseValue[8],BT_BASE_VALUE2,8);
	memcpy(&ucBaseValue[16],BT_BASE_VALUE3,8);
	memcpy(&ucBaseValue[24],BT_BASE_VALUE4,8);

	memset(ucInputbuf,0x00,sizeof(ucInputbuf));
	memcpy(ucInputbuf, (void const*)pucArrSrc, ucSrcLength);
	memcpy(&ucBaseValue[24],pStrFwSerial,8);

#if defined(AES_ENABLE_2_LOG)
    printf("length : %d\n",ucSrcLength);
	printf("Encrypted Data : \n");
    hexdump(pucArrSrc, ucSrcLength);
#endif

	// decript certificaiton string
	aes256_init(&ctx, ucBaseValue);
	//aes_encrypt_cbc
	aes256_encrypt_ecb(&ctx, ucInputbuf);

#if defined(AES_ENABLE_2_LOG)
    printf("length : %d\n", ucSrcLength);
	printf("Decrypted Data : \n");
    hexdump(ucInputbuf, sizeof(ucInputbuf) );
#endif
	memcpy(pucArrDst,ucInputbuf,32);

	aes256_done(&ctx);
}

void GetAESDecriptBluetoothData(uint8_t* pucArrSrc, uint8_t ucSrcLength, uint8_t* pucArrDst)
{
	aes256_context ctx;

	//base key value : "RunGitautoDCSVCIAuthoritR0000000";
	U8 ucBaseValue[32] = {0,};
	U8 ucInputbuf[64] = {0,};

	char* pStrFwSerial = GetFWSerialNumber();

	memcpy(&ucBaseValue[0],BT_BASE_VALUE1,8);
	memcpy(&ucBaseValue[8],BT_BASE_VALUE2,8);
	memcpy(&ucBaseValue[16],BT_BASE_VALUE3,8);
	memcpy(&ucBaseValue[24],BT_BASE_VALUE4,8);

	memset(ucInputbuf,0x00,sizeof(ucInputbuf));
	memcpy(ucInputbuf, (void const*)pucArrSrc, ucSrcLength);
	memcpy(&ucBaseValue[24],pStrFwSerial,8);

#if defined(AES_ENABLE_2_LOG)
    printf("length : %d\n",ucSrcLength);
	printf("Encrypted Data : \n");
    hexdump(pucArrSrc, ucSrcLength);
#endif

	// decript certificaiton string
	aes256_init(&ctx, ucBaseValue);
	//aes_encrypt_cbc
	aes256_decrypt_ecb(&ctx, ucInputbuf);

#if defined(AES_ENABLE_2_LOG)
    printf("length : %d\n", ucSrcLength);
	printf("Decrypted Data : \n");
    hexdump(ucInputbuf, sizeof(ucInputbuf) );
#endif
	memcpy(pucArrDst,ucInputbuf,32);

}

boolean_t IsBasicCertificationCorrect(uint8_t* pucArrPayload, uint8_t ucPayloadLength)
{
	git_f_chdir(DIR_ROOT);
	U8 ucInputbuf[32] = {0,};
	char* pStrFwSerial = GetFWSerialNumber();

	// decrypte basic service data
	GetAESDecriptBluetoothData(&pucArrPayload[1],ucPayloadLength-7,ucInputbuf);

	if( strncmp((char const*)ucInputbuf, "GitDCSVCI", 9) == 0)
	{
		if( strncmp((char const*)&ucInputbuf[9], (char const*)pStrFwSerial, 8) == 0)
		{
			return true;
		}
	}

	return false;
}

boolean_t IsPremiumCertificationCorrect(uint8_t* pucArrPayload, uint8_t ucPayloadLength)
{
	U8 ucInputbuf[32] = {0,};
	char* pStrFwSerial = GetFWSerialNumber();

	// decrypte basic service data
	GetAESDecriptBluetoothData(&pucArrPayload[0],ucPayloadLength-6,ucInputbuf);

	if( strncmp((char const*)ucInputbuf, "AUTOLINK", 8) == 0)
	{
		if( strncmp((char const*)&ucInputbuf[8], (char const*)pStrFwSerial, 8) == 0)
		{
			return true;
		}
	}

	return false;
}

void BTSetAutolinkPType(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char cResult;
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	GITDebugPrintf("[%s] run\r\n", __FUNCTION__);

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

    //hexdump(pPayloadPtcl->pPayload,32);

        if(pPayloadPtcl->pPayload[0] == 0x02 )
        {
              SetServiceType(DCS_Fleet);
              cResult = 0x02;

#if defined(PROTOCOL14)
              Uart8_Baudrate_Set(9600);
#endif
        }
        else if(pPayloadPtcl->pPayload[0] == 0x01 )
       {
            SetServiceType(DCS_Retail);
            cResult = 0x01;

            Uart8_Baudrate_Set(115200);
        }
        else cResult = 0x03;
	TestFormat();

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}

void BTGetCanFDAdaptorType(void *pInterPtcl, unsigned int eInCommType)
{
    unsigned int iPayLoadDataLen=0;
    unsigned char arrTmpBuff[5]={0,};

    iPayLoadDataLen = 5;

    if ( (m_stCFDCtrl.stVer.usBlVer < CFD_ARTERY_MCU_VERSION) && (m_stCFDCtrl.stVer.usAppVer < CFD_ARTERY_MCU_VERSION) )
        arrTmpBuff[0]= 1; // STM MCU CANFD Adaptor
    else
        arrTmpBuff[0]= 2; // Artery MCU CANFD Adaptor

    memcpy(&arrTmpBuff[1], &m_stCFDCtrl.stVer.usBlVer, sizeof(m_stCFDCtrl.stVer.usBlVer));
    memcpy(&arrTmpBuff[3], &m_stCFDCtrl.stVer.usAppVer, sizeof(m_stCFDCtrl.stVer.usAppVer));

#if defined(DEBUG_GIT_PTCL_LOG)
        GITDebugPrintf("[%s] CanFDAdaptor type Mode: %d, Bootloder Ver: 0x%04X, Application Ver: 0x%04X\r\n", __FUNCTION__,
        arrTmpBuff[0], m_stCFDCtrl.stVer.usBlVer, m_stCFDCtrl.stVer.usAppVer);
#endif

    if(eInCommType == eCOMM_TYPE_UART_BT){
        SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, arrTmpBuff, iPayLoadDataLen, (eCommType)eInCommType);
    }
    else if(eInCommType == eCOMM_TYPE_UART_SELFTEST){
        SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, arrTmpBuff, iPayLoadDataLen, (eCommType)eInCommType);
    }
}


void BTSetRtcTime(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char cResult;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	cResult = 0x01;
	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}

void GitGetCurrentVCIMode(void *pInterPtcl, unsigned int eInCommType)
{
  	unsigned int iPayLoadDataLen=0;
	unsigned char arrTmpBuff[7]={0,};

      iPayLoadDataLen = 6;
      arrTmpBuff[0]= 0;	//General :0  Record:1
#ifdef RF_COMMON_MODEM //mod.pdh 20211109

#if defined(STM32F427X)
      arrTmpBuff[1]= 11;				//Premium STM Type (BNCOM, PLS): 11
#elif defined(AT32F435VMT7)
      arrTmpBuff[1]= 61;				//Premium Aretry Type (BNCOM, PLS): 61
#endif

#else //RF_COMMON_MODEM //mod.pdh 20211109

//GDS VCI :1  VCI-II :2  new PDI :3 DCS:5 Autolink Premium (Adanis) : 6
#if defined(STM32F427X)
        arrTmpBuff[1]= 6;               //Premium STM Type (Adanis, ELS)
#elif defined(AT32F435VMT7)
        arrTmpBuff[1]= 56;              //Premium Aretry Type (Adanis, ELS)
#endif

#endif// RF_COMMON_MODEM //mod.pdh 20211109

      memcpy(&arrTmpBuff[2], &g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion, sizeof(g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion));
      memcpy(&arrTmpBuff[4], &g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion, sizeof(g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion));

#if defined(DEBUG_GIT_PTCL_LOG)
		GITDebugPrintf("[%s] Mode: %d Module: %d Version: %c%c%c%c%c \r\n", __FUNCTION__,
		arrTmpBuff[0], arrTmpBuff[1], arrTmpBuff[2], arrTmpBuff[3], arrTmpBuff[4], arrTmpBuff[5], arrTmpBuff[6]);
#endif

	if(eInCommType == eCOMM_TYPE_UART_BT){
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, arrTmpBuff, iPayLoadDataLen, (eCommType)eInCommType);
	}
	else if(eInCommType == eCOMM_TYPE_UART_SELFTEST){
		SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, arrTmpBuff, iPayLoadDataLen, (eCommType)eInCommType);
	}
}

void Git_FWUpdate_Start(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char cResult;
////	unsigned int uiEraseStartAddr, uiEraseEndAddr;

	stGIT_PTCL_PAYLOAD *pPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stDownloadFileReq *pDownloadFileReq =  (stDownloadFileReq*) pPayloadPtcl->pPayload;
	stDownloadStartReq *pDownloadStartReq =  (stDownloadStartReq*) pPayloadPtcl->pPayload;

//	if(LOCK_GET_STATE() != eLOCK_STATE_UNLOCK)
//		return;

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	if(eInCommType == eCOMM_TYPE_USB) {
	}
	else 
#endif
	if(eInCommType==eCOMM_TYPE_UART_SELFTEST)
	{
		cResult = 0x01;

		if ( strncmp((char const*)pDownloadFileReq->DB_Name, "MASTER", 6 ) == 0 ){
			g_cDownloadFW_AppNumber = eApp_MasterDB;										// MASTE DB
			g_nSrcAddress = ADDR_MASTER_DB_SAVE;
		}
		else if ( strncmp((char const*)pDownloadFileReq->DB_Name, "DC", 2 ) == 0 ){
			g_cDownloadFW_AppNumber = eApp_SlaveDB;											// SLAVE DB
			g_nSrcAddress = ADDR_SLAVE_DB_SAVE;
		}
		else if ( strncmp((char const*)pDownloadFileReq->DB_Name, "CT", 2 ) == 0 ){
			g_cDownloadFW_AppNumber = eApp_ControlDB;										//CONTROL DB
			g_nSrcAddress = ADDR_CONTROL_DB_SAVE;
		}
		else if ( strncmp((char const*)pDownloadFileReq->DB_Name, "BOOTFW", 6 ) == 0 ){
			g_cDownloadFW_AppNumber = eApp_Bootloader;									//BOOT
			g_nSrcAddress = ADDR_BOOT_SAVE;
			pDownloadStartReq->DB_Name[0] = 'B';
			pDownloadStartReq->DB_Name[1] = 'O';
			pDownloadStartReq->DB_Name[2] = 'O';
			pDownloadStartReq->DB_Name[3] = 'T';
			pDownloadStartReq->DB_Name[4] = 'F';
			pDownloadStartReq->DB_Name[5] = 'W';
		}
		else if ( strncmp((char const*)pDownloadFileReq->DB_Name, "APPLFW", 6 ) == 0 ){
			g_cDownloadFW_AppNumber = eApp_Application;										//APP
			g_nSrcAddress = ADDR_APPLICATION_SAVE1;
			pDownloadStartReq->DB_Name[0] = 'A';
			pDownloadStartReq->DB_Name[1] = 'P';
			pDownloadStartReq->DB_Name[2] = 'P';
			pDownloadStartReq->DB_Name[3] = 'L';
			pDownloadStartReq->DB_Name[4] = 'F';
			pDownloadStartReq->DB_Name[5] = 'W';
		}
		else
			cResult = 0x00;	// NG 전달

		if ( cResult != 0x00 )
		{
			g_u32UpdateFileSize = 0;
			g_u16UpdateFileCheckSum = 0;

			memcpy(&g_DownloadInfo, pDownloadStartReq, sizeof(g_DownloadInfo));

			if(g_cDownloadFW_AppNumber == eApp_MasterDB){
                if (HalDrvFlashErase(ADDR_MASTER_DB_SAVE, ADDR_SLAVE_DB_SAVE - 1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
					printf("[%s] Master DB Erase Fail\r\n",__FUNCTION__);
				}
//				printf("FLASH: erase ADDR_MASTER_DB_SAVE ~ ADDR_SLAVE_DB_SAVE - 1\n");
//				printf("FLASH: erase 0x08108000 ~ 0x0810BFFF\n");
				cResult = 2;
			}
			else if(g_cDownloadFW_AppNumber == eApp_SlaveDB)
			{
                if (HalDrvFlashErase(ADDR_SLAVE_DB_SAVE, ADDR_APPLICATION_SAVE1 - 1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
					printf("[%s] Slave DB Erase Fail\r\n",__FUNCTION__);
				}
//				printf("FLASH: erase ADDR_SLAVE_DB_SAVE ~ ADDR_APPLICATION_SAVE1 - 1\n");
//				printf("FLASH: erase 0x0810C000 ~ 0x0810FFFF\n");
				cResult = 2;
			}
			else if(g_cDownloadFW_AppNumber == eApp_ControlDB)
			{
                if (HalDrvFlashErase(ADDR_CONTROL_DB_SAVE, ADDR_END_SAVE - 1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
					printf("[%s] Control DB Erase Fail\r\n",__FUNCTION__);
				}
//				printf("FLASH: erase ADDR_SLAVE_DB_SAVE ~ ADDR_APPLICATION_SAVE1 - 1\n");
//				printf("FLASH: erase 0x08180000 ~ 0x0818FFFF\n");
				cResult = 2;
			}
			else if(g_cDownloadFW_AppNumber == eApp_Bootloader)
			{
                if (HalDrvFlashErase(ADDR_BOOT_SAVE, ADDR_FW_INFO_SAVE - 1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
					printf("[%s] Boot Erase Fail\r\n",__FUNCTION__);
				}
//				printf("FLASH: erase ADDR_BOOT_SAVE ~ ADDR_FW_INFO_SAVE - 1\n");
//				printf("FLASH: erase 0x08100000 ~ 0x08103FFF\n");
				cResult = 2;
			}
			else if(g_cDownloadFW_AppNumber == eApp_Application)
			{
                if (HalDrvFlashErase(ADDR_APPLICATION_SAVE1, ADDR_CONTROL_DB_SAVE - 1, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
					printf("[%s] Appl Erase Fail\r\n",__FUNCTION__);
				}
//				printf("FLASH: erase ADDR_APPLICATION_SAVE1 ~ ADDR_CONTROL_DB_SAVE - 1\n");
//				printf("FLASH: erase 0x08110000 ~ 0x081BFFFF\n");
				cResult = 2;
			}
			else {
				cResult = 1;
			}
			//SetOBDState(eOBD_FW_DB_Update_Mode);
			SetOBDState(eOBD_Selftest_FW_Update_Mode);
            SetSensorState(eSENSOR_STATE_IDLE);
		}

//#if defined(DEBUG_GIT_PTCL_LOG)
		printf("[%s] cResult %d run, g_cDownloadFW_AppNumber %d, size %d,ver 0x%02X, nCheckSum %d\r\n", __FUNCTION__, cResult,g_cDownloadFW_AppNumber, g_DownloadFileInfo.DB_Size, g_DownloadFileInfo.DB_Ver ,g_DownloadFileInfo.nCheckSum);
//#endif
		SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);

	}
	else if(eInCommType == eCOMM_TYPE_UART_MODEM) {
#if defined(DEBUG_GIT_PTCL_LOG)
		printf("[%s] cResult %d run, g_cDownloadFW_AppNumber %d, size %d,ver 0x%02X, nCheckSum %d\r\n", __FUNCTION__, cResult, g_cDownloadFW_AppNumber, g_DownloadInfo.DB_Size, g_DownloadInfo.DB_Ver ,g_DownloadInfo.nCheckSum);
#endif
	}
}

void Git_FWUpdate_Recv(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char cResult=0;
	stGIT_PTCL_PAYLOAD *pPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	stDownloadingReq *pstDownloadingReq =  (stDownloadingReq*) pPayloadPtcl->pPayload;

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	if(eInCommType == eCOMM_TYPE_USB) {

	}
	else 
#endif
	if(eInCommType==eCOMM_TYPE_UART_MODEM) {
	}
	else if(eInCommType==eCOMM_TYPE_UART_SELFTEST){
		GITDebugPrintf("<");

		__disable_interrupt();
		cResult = GitWriteUpdateDataTemp(pstDownloadingReq->DB_Data, pstDownloadingReq->Frame_Size); //mod.pdh 22.02.07 to check error
		__enable_interrupt();
		//GITDebugPrintf("[%s] run, g_u32UpdateFileSize %d, g_u16UpdateFileCheckSum %d\r\n", __FUNCTION__, g_u32UpdateFileSize, g_u16UpdateFileCheckSum);

		// LED blink
		if ( g_nCANTransmitLedDelayCount == 0 )
			g_nCANTransmitLedDelayCount = 1;
		SetALLTransmitLedOnOff();

		//cResult = 0x01; //22.02.07 to check error
		SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
	}

}

void Git_FWUpdate_End(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char cResult = 0x00;

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	if(eInCommType==eCOMM_TYPE_USB){

	}
	else 
#endif
	if(eInCommType==eCOMM_TYPE_UART_MODEM) {
	}
	else if(eInCommType==eCOMM_TYPE_UART_SELFTEST){
		GITDebugPrintf("[%s] run\r\n", __FUNCTION__);

		SetLedOnOffCtl(LED_OFF, eLED_GPS);			//GREEN
		SetLedOnOffCtl(LED_ON, eLED_SERVER);		//RED
		SetLedOnOffCtl(LED_OFF, eLED_CAN);			//BLUE

		if ( DownloadClose() )
		{
			cResult = 0x01;
		}

		GITDebugPrintf("[%s] run \r\n", __FUNCTION__);
		SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
	}
}

void Git_DeviceReset(void *pInterPtcl, unsigned int eInCommType)
{
	printf("[%s] run\r\n", __FUNCTION__);

    g_unBkramSwResetSignal = SW_RESET_SIGNAL;
	HalDrvWrBkRam((unsigned char*)&g_unBkramSwResetSignal,BKRAM_SW_RESET_SIGNAL_ADDR,BKRAM_SW_RESET_SIGNAL_SIZE);

	HalDrvPower_ClearStandbyFlag();
	SetWDTReset(1000);
	while(1);
}

void DCS_DB_INFO_Res(void *pInterPtcl, unsigned int eInCommType)
{
#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	U16 iPayLoadDataLen = 0;
	U8 arrTmpBuff[50];
	GITDebugPrintf("[%s] run\r\n", __FUNCTION__);

	memcpy(arrTmpBuff+iPayLoadDataLen, g_FirmwareInfo.AppProperty[eApp_MasterDB].arrFWName, MAX_FW_DB_FILE_NAME);
	iPayLoadDataLen += MAX_FW_DB_FILE_NAME;
	memcpy(arrTmpBuff+iPayLoadDataLen, &g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion, SIZE_FIRMWARE_VER);
	iPayLoadDataLen += SIZE_FIRMWARE_VER;

	memcpy(arrTmpBuff+iPayLoadDataLen, g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, MAX_FW_DB_FILE_NAME);
	iPayLoadDataLen += MAX_FW_DB_FILE_NAME;
	memcpy(arrTmpBuff+iPayLoadDataLen, &g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion, SIZE_FIRMWARE_VER);
	iPayLoadDataLen += SIZE_FIRMWARE_VER;

    memcpy(arrTmpBuff+iPayLoadDataLen, g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, MAX_FW_DB_FILE_NAME);
	iPayLoadDataLen += MAX_FW_DB_FILE_NAME;
	memcpy(arrTmpBuff+iPayLoadDataLen, &g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion, SIZE_FIRMWARE_VER);
	iPayLoadDataLen += SIZE_FIRMWARE_VER;

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)arrTmpBuff, iPayLoadDataLen, (eCommType)g_InputCommType);
}

void CANFD_Info_Res(void *pInterPtcl, unsigned int eInCommType)
{
	stCanFDInfoForBT stCFDInfo;
    unsigned char arrTmpBuff[sizeof(stCFDInfo)+1]={0,};
    unsigned int iPayLoadDataLen=0;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	memset(&stCFDInfo,0x00,sizeof(stCFDInfo));

	stCFDInfo.eCFDStatus = m_stCFDCtrl.eCanFDStatusForBT;
	stCFDInfo.usBootVer = m_stCFDCtrl.stVer.usBlVer;
	stCFDInfo.usAppVer = m_stCFDCtrl.stVer.usAppVer;
	stCFDInfo.ucProgress = g_ucUpdateProgress;
   
    iPayLoadDataLen = sizeof(stCFDInfo);
    memcpy(arrTmpBuff, (void*)&stCFDInfo, iPayLoadDataLen);
            
    if ( (m_stCFDCtrl.stVer.usBlVer < CFD_ARTERY_MCU_VERSION) && (m_stCFDCtrl.stVer.usAppVer < CFD_ARTERY_MCU_VERSION) )
        arrTmpBuff[iPayLoadDataLen]= 1; // STM MCU CANFD Adaptor
    else
        arrTmpBuff[iPayLoadDataLen]= 2; // Artery MCU CANFD Adaptor
    iPayLoadDataLen += 1;

	printf("CANFD Info(state:%d) Current CANFD usBootVer:0x%X, Current CANFD usAppVer:0x%X g_ucUpdateProgress: %d \r\n",stCFDInfo.eCFDStatus,stCFDInfo.usBootVer,stCFDInfo.usAppVer,g_ucUpdateProgress);
    
    if(eInCommType == eCOMM_TYPE_UART_BT){
        SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, arrTmpBuff, iPayLoadDataLen, (eCommType)eInCommType);
    }
    else if(eInCommType == eCOMM_TYPE_UART_SELFTEST){
        SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, arrTmpBuff, iPayLoadDataLen, (eCommType)eInCommType);
    }
}

void BTGetAutoVIN(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucRet=0;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

#if 1 // James Jean 2018/11/14
    g_uiAutoVinReqPacketIdx = 0;
    g_bAutoVinNegativeResponse = FALSE;
#endif
	SetOBDState(eOBD_GetAutoVIN);
	printf("-------AppCommandGetVIN-------\r\n");
	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);
}

//////////////////////////////////////BLE MODULE////////////////////////////////////////////////////////
void BTGetInfoRes(void *pInterPtcl, unsigned int eInCommType)	//0xFEF0
{
	unsigned char ucRet[11];

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	if(GetPOWERState()==ePOWER_Sleep_Ready || GetPOWERState()==ePOWER_Sleep_Complete)
	{
		memset(&ucRet,0xFF,sizeof(ucRet));
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, ucRet, sizeof(ucRet), (eCommType)eInCommType);
	}
	else {
//		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, g_stTelNumberInfo.ucTelNumber, sizeof(g_stTelNumberInfo.ucTelNumber), (eCommType)eInCommType);	//데이터 정해지면 수정해야함-arrTelNumber
	}
	//g_SaveFunctionID=stTemp->FunctionID;
}

void BTSetReserveIndexRes(void *pInterPtcl, unsigned int eInCommType)	//0xFEF1
{
	/*unsigned char ucRet=0;
	int i=0;
	INT8U arrReservedIndex[5][16]={"1234567890ABCDEF","2234567890ABCDEF","3234567890ABCDEF","4234567890ABCDEF","5234567890ABCDEF"};
	stBT_PTCL_PAYLOAD *stTemp=(stBT_PTCL_PAYLOAD*)pInterPtcl;
	U8 ucInputbuf[16];

	memcpy(ucInputbuf, (stBT_PTCL_PAYLOAD*)stTemp->pPayload, stTemp->DataLength-6);

	for(i=0;i<5;i++)
	{
		if( strncmp(ucInputbuf, arrReservedIndex[i],sizeof(ucInputbuf))==0)
		{
			ucRet=0x00;
			break;
		}
		else																								ucRet=0x01;
	}

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);	//데이터 정해지면 수정해야함-내려온 예약 인덱스 비교후 결과 값 전송*/

	//g_SaveFunctionID=stTemp->FunctionID;
}

void BTGetSecurityPassRes(void *pInterPtcl, unsigned int eInCommType)	//0xFEF2
{
//	stBT_PTCL_PAYLOAD *stTemp=(stBT_PTCL_PAYLOAD*)pInterPtcl;
//	unsigned char ucRet=1;
//	int i=0;
//	UINT iLength=0;
//	U8 Decryptedtext[AES_TEXT_SIZE];
////	U8 Encryptedtext[AES_TEXT_SIZE];
////	U8 Paddingtext[AES_TEXT_SIZE];
////	U8 plaintext[AES_TEXT_SIZE]="vpt000000000099mbr000000000099";
//	unsigned long key_schedule[60];
//	BYTE iv[1][16] = {{0x57, 0x68, 0x61, 0x74, 0x54, 0x68, 0x65, 0x48, 0x65, 0x6c, 0x6c, 0x2e, 0x44, 0x61, 0x6d, 0x6e}};
//	BYTE uckey[33]="CarCap";	//인증키 끝에 null이 없으면 디코딩시 에러남 32->33으로 수정
//	U8 ucInputbuf[50],arrReservedNumber[20],arrUserID[20],arrAdmin[20]="ADMIN0000000000";
//	//U8 ucRebootNumber[20]="01089366284";
//
//	BT_LOCK_SET_STATE(eBT_LOCK_STATE_LOCK);		//lock
//
//	printf("0xFEF2 run %d\r\n",BT_LOCK_GET_STATE());
//
//	memset(arrReservedNumber,0x00,sizeof(arrReservedNumber));
//	memset(arrUserID,0x00,sizeof(arrUserID));
//	memset(ucInputbuf,0x00,sizeof(ucInputbuf));
//	memset(Decryptedtext, 0x00, AES_TEXT_SIZE);
////	memset(Encryptedtext, 0x00, AES_TEXT_SIZE);
////
////
////	memcpy(&uckey[0],"CarCap01223883518mbr000000000099",32);
////	aes_key_setup(uckey, key_schedule, 256);
////	memset(Paddingtext, 0x00, AES_TEXT_SIZE);
////	pkcs7_pad_write(plaintext ,16,Paddingtext);
////	aes_encrypt_cbc(Paddingtext, 32, Encryptedtext, key_schedule, 256, iv[0]);
//
////	if(GetPOWERState()==ePOWER_Sleep_Ready || GetPOWERState()==ePOWER_Sleep_Complete || g_ucTellNumFlag==1)
////	{
////		if(g_ucTellNumFlag==1)	//전화번호는 리부팅시에만 얻어오는데 리부팅시 전화번호를 얻어오지 못했을경우 BT인증을 할수없어서 실패명령을 올려준후 물어본다
////		{
////			Md_GetPhoneNumber();
////			ucRet=3;
////			//ADMIN
////			memcpy(ucInputbuf, (stBT_PTCL_PAYLOAD*)(stTemp->pPayload+1), stTemp->DataLength-7);	//BT에서 내려온 정보를 저장 예약번호+유저ID
////			memcpy(&uckey[6],ucRebootNumber,sizeof(g_stTelNumberInfo.ucTelNumber));
////			memcpy(&uckey[17],&arrAdmin[0],USER_ID_LENGTH);
////			aes_key_setup(uckey, key_schedule, 256);
////			aes_decrypt_cbc(ucInputbuf, 32, Decryptedtext, key_schedule, 256, iv[0]);
////			memcpy(arrReservedNumber,&Decryptedtext[0], RESERVED_INDEX_LENGTH-1);			//예약번호	15자리만써서 15자리만 내려옴 초기 요건협의 MISS
////			memcpy(arrUserID,&Decryptedtext[RESERVED_INDEX_LENGTH-1], USER_ID_LENGTH-1);	//유저ID	15자리만써서 15자리만 내려옴 초기 요건협의 MISS
////
////			if(strncmp((char*)arrUserID,(char*)arrAdmin,5)==0 && strncmp((char*)arrReservedNumber,(char*)ucRebootNumber,11)==0)	BT_LOCK_SET_STATE(eBT_LOCK_STATE_REBOOTUNLOCK);
////		}
////		else
////		{
////			ucRet=2;
////		}
////		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)stTemp, &ucRet, 1, (eCommType)eInCommType);
////	}
//	if(GetPOWERState() == ePOWER_Sleep_Ready || GetPOWERState()==ePOWER_Sleep_Complete) {
//		ucRet=2;
//		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)stTemp, &ucRet, 1, (eCommType)eInCommType);
//	}
//	else {
//////		//BT인증시 확장이 NONE상태이면 한번 깨워줄 필요있음(BT로깨자마자 하트비트를받아 확장슬립명령 보내고 슬립들어가려다가 BT상태보고 POWERWAKEUP으로 가서 확장웨이크업핀을 흔들었는데 확장은 자기전 1초 딜레이상태라 인식못하고 잠듬)
//////		//파워러닝상태에서도 1.5초간 확장이 NONE이면 깨우게 되어있으나, BT로 깬상태라 BT인증을 거친 후 확장으로 명령을 보낸뒤라 이미 상태가 WATING상태로 변경. 고로 NONE상태가 아니므로 파워러닝상태에서도 안흔듬
//////		//170524_확장안켜짐로그 참조
//////		if(GetExtendBoardState()== eEXTENDBOARD_NONE) {
//////			printf("PR EXT_BT\r\n");
//////			HalGPIOSetVaule(GPIO_PLUSBOARD_WAKE, eBIT_SET);
//////			Oem_GIT_mDelay(10);
//////			HalGPIOSetVaule(GPIO_PLUSBOARD_WAKE, eBIT_RESET);
//////		}
//
//		memcpy(ucInputbuf, (stBT_PTCL_PAYLOAD*)(stTemp->pPayload+1), stTemp->DataLength-7);	//BT에서 내려온 정보를 저장 예약번호+유저ID
//		git_f_chdir("/INFO");
//		if( GetVCI2FileRead(FILENAME_RESERV_INFO, &g_stReservInfo[0].arrIndex[0], &iLength)==TRUE) {
//			  //현재 암호화된 정보로 UserID가 내려오는데 암호화 푸는 키값에 UserID를 사용 나중에 방법바뀌면 수정할것-LWH
//			for(i = 0; i < RESERVED_INDEX_MAX; i++) {
//				if(g_stReservInfo[i].ucUsed==TRUE) {
////					memcpy(&uckey[6],&g_stTelNumberInfo.ucTelNumber[0],sizeof(g_stTelNumberInfo.ucTelNumber));
//					memcpy(&uckey[17],&g_stReservInfo[i].arrUserID[0],USER_ID_LENGTH);
//
//					//옵션 : AES/CBC/PKCS5Padding
//					aes_key_setup(uckey, key_schedule, 256);
//					aes_decrypt_cbc(ucInputbuf, 32, Decryptedtext, key_schedule, 256, iv[0]);
//
//					memcpy(arrReservedNumber,&Decryptedtext[0], RESERVED_INDEX_LENGTH-1);			//예약번호	15자리만써서 15자리만 내려옴 초기 요건협의 MISS
//					memcpy(arrUserID,&Decryptedtext[RESERVED_INDEX_LENGTH-1], USER_ID_LENGTH-1);	//유저ID	15자리만써서 15자리만 내려옴 초기 요건협의 MISS
//
//					ucRet=ReservedIDSearch(arrUserID,arrReservedNumber);	//ID 매치확인
//					if(ucRet==0)
//						break;
//				}
//			}
//		}
//
//		//ADMIN확인
////		memcpy(&uckey[6],&g_stTelNumberInfo.ucTelNumber[0],sizeof(g_stTelNumberInfo.ucTelNumber));
//		memcpy(&uckey[17],&arrAdmin[0],USER_ID_LENGTH);
//		aes_key_setup(uckey, key_schedule, 256);
//		aes_decrypt_cbc(ucInputbuf, 32, Decryptedtext, key_schedule, 256, iv[0]);
//		memcpy(arrReservedNumber,&Decryptedtext[0], RESERVED_INDEX_LENGTH-1);			//예약번호	15자리만써서 15자리만 내려옴 초기 요건협의 MISS
//		memcpy(arrUserID,&Decryptedtext[RESERVED_INDEX_LENGTH-1], USER_ID_LENGTH-1);	//유저ID	15자리만써서 15자리만 내려옴 초기 요건협의 MISS
//		if(strncmp((char*)arrUserID,(char*)arrAdmin,5)==0)	ucRet=0;
//
//		if(ucRet==0)
//			BT_LOCK_SET_STATE(eBT_LOCK_STATE_UNLOCK);		//unlock
//
//		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)stTemp, &ucRet, 1, (eCommType)eInCommType);
//	}
}

int ReservedIDSearch(U8 *arrUserID,U8 *arrReservedNumber)
{
//	int i=0,j=0;
//	UINT iLength=0;
//	unsigned char arrRealTime[15];
//	unsigned char arrRealTimeConvert[5];
//	U8 arrStartTime[RESERVED_TIMEINFO_SIZE+1];
//	U8 arrStartTimeConvert[6];
//	U8 arrEndTime[RESERVED_TIMEINFO_SIZE+1];
//	U8 arrEndTimeConvert[6];
////	U32 uiStartTime, uiEndTime, uiNowTime;
////        unsigned char buff[100],buff2[100];
//
////	if(strncmp((char*)arrReservedNumber,"ADMIN",5)==0)
////	{
////		GITDebugPrintf("\r\n ADMIN Match");
////		return 0;
////	}
//	printf("IDSearch run\r\n");
//
//	git_f_chdir("/INFO");
//	if( GetVCI2FileRead(FILENAME_RESERV_INFO, &g_stReservInfo[0].arrIndex[0], &iLength)==TRUE)
//	{
//		for(i=0;i<RESERVED_INDEX_MAX;i++)
//		{
//			if( strncmp((char*)arrUserID,(char*)&g_stReservInfo[i].arrUserID[0],USER_ID_LENGTH-1)==0)
//			{
//				if( strncmp((char*)arrReservedNumber, (char*)&g_stReservInfo[i].arrIndex[0],RESERVED_INDEX_LENGTH-1)==0)
//				{
//					memcpy(g_ucSaveReservedNumber, arrReservedNumber, RESERVED_INDEX_LENGTH);	//BT 디스커넥트용 저장
//					GITDebugPrintf("\r\n UserID Match");
//					VCI_GetRtcTime(arrRealTime);
//					memcpy(arrStartTime, &g_stReservInfo[i].ucStartTime[2], sizeof(arrStartTime)-2);
//					memcpy(arrEndTime, &g_stReservInfo[i].ucEndTime[2], sizeof(arrEndTime)-2);
//					arrStartTime[RESERVED_TIMEINFO_SIZE]='\0';
//					arrEndTime[RESERVED_TIMEINFO_SIZE]='\0';
//
//					memcpy(arrRealTimeConvert,&arrRealTime[1],sizeof(arrRealTimeConvert));
//
//					arrStartTime[7] = arrStartTime[7] -1; 	//대여시간 1시간전부터 차량도어 제어 가능
//
//					fnHex2Str((char *)&arrStartTimeConvert[0],(char *)&arrStartTime[0]);
//					fnHex2Str((char *)&arrEndTimeConvert[0],(char *)&arrEndTime[0]);
//					for(j=0;j<5;j++)
//					{
//						arrStartTimeConvert[j]=HexToDec(arrStartTimeConvert[j]);
//						arrEndTimeConvert[j]=HexToDec(arrEndTimeConvert[j]);
//					}
//					arrStartTimeConvert[5]=0x00;
//					arrEndTimeConvert[5]=0x00;
//
//					//if((strcmp((char *)arrRealTimeConvert,(const char *)arrStartTimeConvert)>=0)  /*&&  (strcmp((char *)arrRealTimeConvert,(const char *)arrEndTimeConvert)<=0)*/)
//					if(1)
//					{
//						GITDebugPrintf("\r\n Time Match");
//						if((g_stReservInfo[i].ucWellcome=='Y') || (g_stReservInfo[i].ucWellcome=='y'))	g_ucCertificationFlag=1;
//						return 0;
//					}
//				}
//			}
//		}
//	}
//
//	for(i=0;i<MAX_MASTER_ID;i++)
//	{
//		if( strncmp((char*)arrUserID, (char*)g_stMasterKey[i].arrUserID,USER_ID_LENGTH)==0)
//		{
//			GITDebugPrintf("\r\n MasterID Match");
//			return 0;
//		}
//	}
////	for(i=0;i<RESERVED_INDEX_MAX;i++)
////	{
////		if( strncmp((char*)arrUserID,(char*)&g_stReservInfo[i].arrUserID[0],USER_ID_LENGTH)==0)
////		{
////			if( strncmp((char*)arrReservedNumber, (char*)&g_stReservInfo[i].arrIndex[0],RESERVED_INDEX_LENGTH)==0)
////			{
////				GITDebugPrintf("\r\n UserID Match");
////				return 0;
////			}
////		}
////	}
	return 1;
}

void BTSetDoorControlRes(void *pInterPtcl, unsigned int eInCommType)	//0xFEF3
{
	unsigned char ucRet=0;
	unsigned char ucData[5];
	stBT_PTCL_PAYLOAD *stTemp=(stBT_PTCL_PAYLOAD*)pInterPtcl;

//	printf("0xFEF3 run %d \r\n",BT_LOCK_GET_STATE());

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	if(BT_LOCK_GET_STATE()==eBT_LOCK_STATE_LOCK) return;

	if(GetPOWERState()==ePOWER_Sleep_Ready || GetPOWERState()==ePOWER_Sleep_Complete) {
		ucRet=2;
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);
	}
	else {
//		if(GetExtendBoardState() == eEXTENDBOARD_NOBOARD || GetExtendBoardState() == eEXTENDBOARD_NONE) {		//확장모듈이 없거나 준비중인 경우
//			ucRet = 1;
//			SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);
//			return;
//		}

		memcpy(&ucData, (stBT_PTCL_PAYLOAD*)stTemp->pPayload, stTemp->DataLength-6);

		g_ucDoorCheckFlag=0;

		g_SaveFunctionID=stTemp->FunctionID;
		g_eSaveCommType=(eCommType)eInCommType;

////		if(g_stPlusModuleStatusPeriodic.m_ucAccState != 0) {
////			ucRet = 1;
////			SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);
////		}
////		else {
////			switch(ucData[0]) {
////				case 0x14:	//비상등제어
////					g_ucDoorCheckFlag=0xFF;
////					if(ucData[1]==0x01)
////						LampOn(ucData[2]);
////					else if(ucData[1]==0x02)
////						LampOFF();
////					else
////						ucRet=1;
////					//else							ExtendBoardAllCtrlOff();
////					break;
////
////				case 0x15:	//도어제어
////					g_ucDoorCheckFlag=ucData[1];
////					if(ucData[1]==0x00)
////						LockUnlockSet(TRUNK_OPEN);		//0x02
////					else if(ucData[1]==0x01)
////						LockUnlockSet(DOOR_LOCK);		//0x00
////					else if(ucData[1]==0x02)
////						LockUnlockSet(DOOR_UNLOCK);	//0x01
////					else
////						ucRet=1;
////					//else							ExtendBoardAllCtrlOff();
////					break;
////
////				case 0x16:	//경적
////					g_ucDoorCheckFlag=0xFF;
////					if(ucData[1]==0x01)
////						HornOn(HORN_TIME,0);
////					else
////						ucRet=1;
////					//else							ExtendBoardAllCtrlOff();
////					break;
////
////				case 0x17:	//트렁크
////					g_ucDoorCheckFlag=0xFF;
////					if(ucData[1]==0x00)
////						LockUnlockTrunkSet(TRUNK_CLOSE);		//0x00
////					else if(ucData[1]==0x01)
////						LockUnlockTrunkSet(TRUNK_OPEN);			//0x01
////					else
////						ucRet=1;
////					break;
////
////				default:
////					break;
////			}
////
////			ucRet=0;
////			SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);	//데이터 정해지면 수정해야함-내려온값 저장 후 리턴(동작후리턴이아닌 수신받으면 리턴 0x00-OK 0x01-NG)
//////			m_ucPayloadDump_BT[0]=ucData[0];
//////			m_ucPayloadDump_BT[1]=ucData[1];
//////			m_ucPayloadDump_BT[2]=ucData[2];
//////			g_iTimerPlusCheckTimeOutCallback_BT = HalTimerSetSWTimer(50, eSWTimer_INFINITE, CB_TimeOutCallback_BT, TRUE);
////		}
	}
}

void BTSetRebootRes(void *pInterPtcl, unsigned int eInCommType)	//0xFEF4
{
	unsigned char ucRet=1;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	if(GetPOWERState()==ePOWER_Sleep_Ready || GetPOWERState()==ePOWER_Sleep_Complete) {
		ucRet=2;
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);
	}
	else {
////		if(g_stPlusModuleStatusPeriodic.m_ucAccState!=0) {
////			SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);
////		}
////		else {
////			ucRet = 0;
////			SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);
////			HalGPIOSetVaule(GPIO_MA_PWEN, eBIT_RESET);
////			PlusModuleReset();
////			Git_DeviceReset(pInterPtcl,eInCommType);
////		}
	}
}

void BTGetAccRes(void *pInterPtcl, unsigned int eInCommType)	//0xFEF5
{
	unsigned char ucRet=0xFE;
	stBT_PTCL_PAYLOAD *stTemp=(stBT_PTCL_PAYLOAD*)pInterPtcl;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	if(BT_LOCK_GET_STATE()==eBT_LOCK_STATE_LOCK)
		return;

	if(GetPOWERState() == ePOWER_Sleep_Ready || GetPOWERState() == ePOWER_Sleep_Complete) {
		ucRet=0xFE;
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);
	}
	else {
		g_SaveFunctionID=stTemp->FunctionID;
		g_eSaveCommType=(eCommType)eInCommType;
//		if(GetExtendBoardState() == eEXTENDBOARD_RUNNING) {
//			GetExtendBoardModuleStatus();
//		}
//		else {	//확장이 eEXTENDBOARD_RUNNING상태가 아니면 현재 확장보드의 상태값을 리턴해줌
//			if(g_eSaveCommType==eCOMM_TYPE_UART_BT) {
////				ucRet=GetExtendBoardState();
////				SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 0x01, (eCommType)eInCommType);	//데이터 정해지면 수정해야함-내려온값 저장 후 리턴(동작후리턴이아닌 수신받으면 리턴 0x00-OK 0x01-NG)
//			}
//			else if(g_eSaveCommType==eCOMM_TYPE_UART_MODEM) {
//				}
//		}
	}
}

void BTGetDeviceInfoRes(void *pInterPtcl, unsigned int eInCommType)	//0xFEF6
{
	unsigned char ucRet=0;

	stBT_PTCL_PAYLOAD *stTemp=(stBT_PTCL_PAYLOAD*)pInterPtcl;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	if(BT_LOCK_GET_STATE()==eBT_LOCK_STATE_LOCK)
		return;

	if(GetPOWERState()==ePOWER_Sleep_Ready || GetPOWERState()==ePOWER_Sleep_Complete) {
		ucRet=0xFF;
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, &ucRet, 1, (eCommType)eInCommType);
	}
	else {
		g_SaveFunctionID=stTemp->FunctionID;
		g_eSaveCommType=(eCommType)eInCommType;

		//CAN Check
//		printf("GetOBDState()=%d\r\n",GetOBDState());
		if((GetOBDState() >= eOBD_GetAutoVIN) && (GetOBDState()<=eOBD_Sleep_Complete))
			g_stDeviceInfo.ucCanState=1;
		else
			g_stDeviceInfo.ucCanState=0;

		if(ModemManagerData.bAvailableModemCommFlag == true)
			g_stDeviceInfo.ucModemState[0]=1;
		else
			g_stDeviceInfo.ucModemState[0]=0;

		if(g_stGpsInfo.ucSatelNum >= 3)
			g_stDeviceInfo.ucGPSState=1;
		else
			g_stDeviceInfo.ucGPSState=0;

//		GetHiPass();					//HiPass Check

		if( BTGetConnectStatus() == TRUE )
			g_stDeviceInfo.ucBTState=1;	//BT Check
		else
			BTGetFWVersionReq();

		 g_DeviceInfoTimerIndex = HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, DeviceTimerCallBack, TRUE);
	}
}

void DeviceTimerCallBack()
{
	stBT_PTCL_PAYLOAD stBTOutPtcl;
	eCommType	eInCommType=eCOMM_TYPE_UART_BT;

	if(g_stDeviceInfo.ucModemState[0]==1)	{ //정상상태면 데이터 넣기
		g_stDeviceInfo.ucModemState[1]=g_stSystemInfo.ucNSI;	//신호세기
		g_stDeviceInfo.ucModemState[2]=g_stSystemInfo.ucServiceState;				//0x00초기치,  0x01서비스불가,  0x02,제한서비스,  0x03정상서비스,  0x04제한지역,  0x05전원절약
		g_stDeviceInfo.ucModemState[3]=g_stSystemInfo.ucNetworkName;			//0x00 Olleh,  0x01 이외망
		g_stDeviceInfo.ucModemState[4]=g_stSystemInfo.ucRoamingState;			//0x00 홈네트워크,  0x01:로밍네트워크
		g_stDeviceInfo.ucModemState[5]=g_stSystemInfo.ucRat;							//0x00 NONE,  0x01 GSM,  0x02 GPRS,   0x03 EGPRS,   0x04 UMTS,  0x05 HSDPA,   0x06 HSUPA,   0x07 HSPA,   0x08 LTE
	}

	memset(&stBTOutPtcl,0x00,sizeof(stBTOutPtcl));
	stBTOutPtcl.FunctionID 	= 0xFEF6;
	stBTOutPtcl.DataLength = sizeof(g_stDeviceInfo);
	stBTOutPtcl.pPayload = (unsigned char*)&g_stDeviceInfo;

	HalTimerStopSWTimer(g_DeviceInfoTimerIndex);
	HalTimerClearSWTimer(g_DeviceInfoTimerIndex);
	g_DeviceInfoTimerIndex = -1;

	SendBTPtclResponse(&stBTOutPtcl,  (unsigned char*)&g_stDeviceInfo, sizeof(g_stDeviceInfo), eInCommType);
}

void BTSetTestInfoRes(void *pInterPtcl, unsigned int eInCommType)	//0xFEFF
{
//	unsigned char ucInputbuf[100];
//	unsigned char ucOutputbuf[100];
//	UINT iLength = 0;
//	U8 i = 0,ucMode=0;
//	stBT_PTCL_PAYLOAD *stTemp=(stBT_PTCL_PAYLOAD*)pInterPtcl;
//
//	printf("0xFEFF run %d \r\n",BT_LOCK_GET_STATE());
//
//	// warning message 삭제를 위해서...
//	i = i;
//	iLength = iLength;
//
//	memset(ucInputbuf,0x00,sizeof(ucInputbuf));
//	memset(ucOutputbuf,0x00,sizeof(ucOutputbuf));
//
//	memcpy(ucInputbuf, (stBT_PTCL_PAYLOAD*)stTemp->pPayload, stTemp->DataLength-6);
//
//	memcpy(ucOutputbuf,ucInputbuf,sizeof(ucInputbuf));
//
//	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, ucInputbuf, stTemp->DataLength-6, (eCommType)eInCommType);	//데이터 정해지면 수정해야함-내려온값 저장 후 리턴(동작후리턴이아닌 수신받으면 리턴 0x00-OK 0x01-NG)
//
//
//	memcpy(&ucMode, &ucInputbuf[0], 1);
//	memcpy(g_FirmwareInfo.m_strVehicleCode, &ucInputbuf[USER_ID_LENGTH-1+RESERVED_INDEX_LENGTH-1], MAX_VEHICLECODE_SIZE-1);
//	if((g_FirmwareInfo.m_strVehicleCode[0]=='0') && (g_FirmwareInfo.m_strVehicleCode[1]=='1') && (g_FirmwareInfo.m_strVehicleCode[0]=='2')) {
//	}
//	else
//	{
//		g_FirmwareInfo.m_strVehicleCode[10] = 0x00;
//		SetPOWERState(ePOWER_Sleep_Ready);
//	}
//
////	printf("TelNum:%c%c%c%c%c%c%c%c%c%c%c%c\r\n",g_FirmwareInfo.m_strVehicleCode[0],\
////																							g_FirmwareInfo.m_strVehicleCode[1],\
////																							g_FirmwareInfo.m_strVehicleCode[2],\
////																							g_FirmwareInfo.m_strVehicleCode[3],\
////																							g_FirmwareInfo.m_strVehicleCode[4],\
////																							g_FirmwareInfo.m_strVehicleCode[5],\
////																							g_FirmwareInfo.m_strVehicleCode[6],\
////																							g_FirmwareInfo.m_strVehicleCode[7],\
////																							g_FirmwareInfo.m_strVehicleCode[8],\
////																							g_FirmwareInfo.m_strVehicleCode[9],\
////																							g_FirmwareInfo.m_strVehicleCode[10],\
////																							g_FirmwareInfo.m_strVehicleCode[11]);
//	printf("TelNum: %s\r\n", g_FirmwareInfo.m_strVehicleCode);
}


void BTFWUpdateRes(void *pInterPtcl, unsigned int eInCommType)  //FW, DB 업데이트 시작 genebe 1
{
	unsigned char cResult;
	unsigned int uiEraseStartAddr, uiEraseEndAddr;
#if defined(INFINITE_CANFD_UPDATE)
	static bool s_bBootFlag = true;
	static bool s_bAppFlag = true;
	static unsigned short s_usBootVersion = 0;
	static unsigned short s_usAppVersion = 0;
#endif
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
#if defined(AES_ENABLE)
	U8 ucInputbuf[16];
#endif
	uint32_t nLoopCount = 0;
	int i = 0;

     cResult = 0x01;
//	if(LOCK_GET_STATE() != eLOCK_STATE_UNLOCK)
//		return;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	if(eInCommType == eCOMM_TYPE_UART_BT)
    {
#if defined(AES_ENABLE)
		memset(ucInputbuf,0x00,sizeof(ucInputbuf));
		memcpy(ucInputbuf, (void const*)&pPayloadPtcl->pPayload[0], sizeof(ucInputbuf));
		if( SecurityCheck(ucInputbuf) == false )
		{
			printf("!!!!!!!!!!!!!!!!!!!!!!!FAIL!!!!!!!!!!!!!!!!!");
			cResult = 0x00;
			SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
			return ;
		}
#endif

#if defined(AES_ENABLE)
		stDownloadStartReq *pDownloadStartReq =  (stDownloadStartReq*) &ucInputbuf[2];
#else
		stDownloadStartReq *pDownloadStartReq =  (stDownloadStartReq*) pPayloadPtcl->pPayload;
#endif

     	//GITDebugPrintf("!!! ProcessGITInterProtocol Input Function ID : 0x%04X !!!\r\n", pInterPtcl->FunctionID);
        GITDebugPrintf( "[%s]NAME= %c%c%c%c%c%c, Size = %02x, Ver = %04x , CS = %02x\n", __FUNCTION__,
	        pDownloadStartReq->DB_Name[0],
	        pDownloadStartReq->DB_Name[1],
	        pDownloadStartReq->DB_Name[2],
	        pDownloadStartReq->DB_Name[3],
	        pDownloadStartReq->DB_Name[4],
	        pDownloadStartReq->DB_Name[5],
	        pDownloadStartReq->DB_Size,
	        pDownloadStartReq->DB_Ver,
	        pDownloadStartReq->nCheckSum);

		cResult = 0x01;
	  	if ( strncmp((char const*)pDownloadStartReq->DB_Name, "FDBOOT", 6 ) == 0 || strncmp((char const*)pDownloadStartReq->DB_Name, "FDAPPL", 6 ) == 0 )	//FD BOOT
        {
			g_u16UpdateFileCheckSum = 0;
            g_bCanFD_FOTA_Check = true;
			memset(&g_stCFD_BT_Update_Info,0x00,sizeof(stCANFD_BT_UPDATE_INFO));
			if( strncmp((char const*)pDownloadStartReq->DB_Name, "FDBOOT", 6 ) == 0 )
			{
#if defined(INFINITE_CANFD_UPDATE)
if( s_bBootFlag == true )
{
	s_bBootFlag = false;
	s_usBootVersion = pDownloadStartReq->DB_Ver;
	//s_usBootVersion = 0x102;
}
s_usBootVersion++;
pDownloadStartReq->DB_Ver = s_usBootVersion;
m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usBootVersion = s_usBootVersion;
#endif
				printf("\n <FD Boot>\n");
                g_Check_Update = eCANFD_UPDATE_INSTALLATION_BOOT;
				g_stCFD_BT_Update_Info.eInstallFileType = eINSTALL_FILE_TYPE_BOOT;
				g_stCFD_BT_Update_Info.nDestAddress = SECTOR_EXBOARD_BOOT_FIRMWARE;
				nLoopCount = SECTOR_EXBOARD_BOOT_FIRMWARE_SECTOR_SIZE;	//8
			}
			else
			{
#if defined(INFINITE_CANFD_UPDATE)
if( s_bAppFlag == true )
{
	s_bAppFlag = false;
	s_usAppVersion = pDownloadStartReq->DB_Ver;
//	s_usAppVersion = 0x102;
}
s_usAppVersion++;
pDownloadStartReq->DB_Ver = s_usAppVersion;
m_stAutolinkConfigData.stSystem.stFDBoardUpdateInfo.usAppVersion = s_usAppVersion;
#endif
				printf("\n <FD App>\n");
                g_Check_Update = eCANFD_UPDATE_INSTALLATION_APP;
				g_stCFD_BT_Update_Info.eInstallFileType = eINSTALL_FILE_TYPE_APP;
				g_stCFD_BT_Update_Info.nDestAddress = SECTOR_EXBOARD_APP_FIRMWARE;
				nLoopCount = SECTOR_EXBOARD_APP_FIRMWARE_SECTOR_SIZE;	//32
			}
			memcpy(&g_stCFD_BT_Update_Info.stFileInfo,pDownloadStartReq,sizeof(stDownloadStartReq));
			g_bFWUpdateFlagFromBLE = TRUE;

			for( i=0; i<nLoopCount; i++ )	sFLASH_EraseSubSector((g_stCFD_BT_Update_Info.nDestAddress + i) * SFLASH_SECTOR_SIZE);

			SetSensorState(eSENSOR_STATE_IDLE);
			// Send suspended event
			Send2MngStorage(eMngSysMsg,eReqSuspend, eSuspendStart, (stCarReport *)NULL , 0);
			Send2MngSensor(eMngSysMsg,eReqSuspend, eSuspendStart, (stCarReport *)NULL , 0);
			SetOBDState(eOBD_Selftest_FW_Update_Mode);

			printf("[%s] cResult %d run, g_stCFD_BT_Update_Info.eInstallFileType %d, size %d,ver 0x%02X, nCheckSum %d g_Check_Update: %d\r\n", __FUNCTION__, cResult,
			g_stCFD_BT_Update_Info.eInstallFileType, g_stCFD_BT_Update_Info.stFileInfo.DB_Size, g_stCFD_BT_Update_Info.stFileInfo.DB_Ver,g_stCFD_BT_Update_Info.stFileInfo.nCheckSum,g_Check_Update);
        }
		else
		{
			g_stCFD_BT_Update_Info.eInstallFileType = eINSTALL_FILE_TYPE_NONE;
			if ( strncmp((char const*)pDownloadStartReq->DB_Name, "MASTER", 6 ) == 0 )			//MASTE DB
			{
				g_cDownloadFW_AppNumber = eApp_MasterDB;
				g_nSrcAddress = ADDR_MASTER_DB_SAVE;
			}
			else if ( strncmp((char const*)pDownloadStartReq->DB_Name, "DC", 2 ) == 0 )			//SLAVE DB
			{
				g_cDownloadFW_AppNumber = eApp_SlaveDB;
				g_nSrcAddress = ADDR_SLAVE_DB_SAVE;
			}
			else if ( strncmp((char const*)pDownloadStartReq->DB_Name, "CT", 2 ) == 0 )			//CONTROL DB
			{
				g_cDownloadFW_AppNumber = eApp_ControlDB;
				g_nSrcAddress = ADDR_CONTROL_DB_SAVE;
			}
			else if ( strncmp((char const*)pDownloadStartReq->DB_Name, "BOOTFW", 6 ) == 0 )	//BOOT
			{	g_cDownloadFW_AppNumber = eApp_Bootloader;
				g_nSrcAddress = ADDR_BOOT_SAVE;
			}
			else if ( strncmp((char const*)pDownloadStartReq->DB_Name, "APPLFW", 6 ) == 0 )	//APP
			{
				g_cDownloadFW_AppNumber = eApp_Application;
				g_nSrcAddress = ADDR_APPLICATION_SAVE1;
			}
			else
				cResult = 0x00;	// NG 전달

			if ( cResult != 0x00 )
			{
				g_u32UpdateFileSize = 0;
				g_u16UpdateFileCheckSum = 0;

				memcpy(&g_DownloadInfo, pDownloadStartReq, sizeof(g_DownloadInfo));

				uiEraseStartAddr = g_nSrcAddress;
				uiEraseEndAddr = uiEraseStartAddr + g_DownloadInfo.DB_Size;

					GITDebugPrintf( " Erase Start Addr : %x , End Addr : %x , DB_Size : %x \n", uiEraseStartAddr, uiEraseEndAddr, g_DownloadInfo.DB_Size);  //erase 공간 체크


                if (HalDrvFlashErase(uiEraseStartAddr, uiEraseEndAddr, NULL, 0, 0) != HAL_RETURN_SUCCESS ) {
					cResult = 2;
				}
				else {
					cResult = 1;
				}

				g_bFWUpdateFlagFromBLE = TRUE;

				SetOBDState(eOBD_FW_DB_Update_Mode);
				SetSensorState(eSENSOR_STATE_IDLE);
				// Send suspended event
				Send2MngStorage(eMngSysMsg,eReqSuspend, eSuspendStart, (stCarReport *)NULL , 0);
				Send2MngSensor(eMngSysMsg,eReqSuspend, eSuspendStart, (stCarReport *)NULL , 0);
				SetOBDState(eOBD_Selftest_FW_Update_Mode);
			}

			//#if defined(DEBUG_GIT_PTCL_LOG)
			GITDebugPrintf("[%s] cResult %d run, g_cDownloadFW_AppNumber %d, size %d,ver 0x%02X, nCheckSum %d\r\n", __FUNCTION__, cResult,
			g_cDownloadFW_AppNumber, g_DownloadInfo.DB_Size, g_DownloadInfo.DB_Ver ,g_DownloadInfo.nCheckSum);
			//#endif
		}
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
	}
	else if(eInCommType == eCOMM_TYPE_UART_MODEM) {
#if defined(DEBUG_GIT_PTCL_LOG)
		printf("[%s] cResult %d run, g_cDownloadFW_AppNumber %d, size %d,ver 0x%02X, nCheckSum %d\r\n", __FUNCTION__, cResult, g_cDownloadFW_AppNumber, g_DownloadInfo.DB_Size, g_DownloadInfo.DB_Ver ,g_DownloadInfo.nCheckSum);
#endif
	}
}

void BTFWUpdateRecv(void *pInterPtcl, unsigned int eInCommType)	 //FW, DB 업데이트 시작 genebe 2 개발
{
  	unsigned char cResult=0;
	unsigned short usCheckSum=0;  //mod.pdh 22.02.07 to check error

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	if(eInCommType == eCOMM_TYPE_UART_BT) {

      	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
		//stDownloadStartReq *pDownloadStartReq =  (stDownloadStartReq*) pPayloadPtcl->pPayload;  //genebe
		stDownloadingReq *pstDownloadingReq =  (stDownloadingReq*) pPayloadPtcl->pPayload;  //genebe
        GITDebugPrintf(">");

		if( (g_stCFD_BT_Update_Info.eInstallFileType == eINSTALL_FILE_TYPE_BOOT) || (g_stCFD_BT_Update_Info.eInstallFileType == eINSTALL_FILE_TYPE_APP) )
		{
			__disable_interrupt();
			sFLASH_WriteBuffer((uint8_t*)pstDownloadingReq->DB_Data,(g_stCFD_BT_Update_Info.nDestAddress*SFLASH_SECTOR_SIZE)+g_stCFD_BT_Update_Info.nProgressAddress, pstDownloadingReq->Frame_Size);
			usCheckSum = CalCheckSum(pstDownloadingReq->DB_Data,pstDownloadingReq->Frame_Size);
			g_u16UpdateFileCheckSum += usCheckSum;
			__enable_interrupt();
			g_stCFD_BT_Update_Info.nProgressAddress += pstDownloadingReq->Frame_Size;
            cResult=0x01;
		}
		else
		{
			__disable_interrupt();
			cResult = GitWriteUpdateDataTemp(pstDownloadingReq->DB_Data, pstDownloadingReq->Frame_Size);  //mod.pdh 22.02.07 to check error
			__enable_interrupt();
		}

		// LED blink
		if ( g_nCANTransmitLedDelayCount == 0 )
			g_nCANTransmitLedDelayCount = 1;

		SetALLTransmitLedOnOff();

        g_bFWUpdateFlagFromBLE = TRUE;
		//cResult = 0x01; //mod.pdh 22.02.07 to check error
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
                
	    if(cResult == 0)
	    {
	        APP_Delay(100);
	        SystemForcelyReset();
	    }
	}
	else if(eInCommType==eCOMM_TYPE_UART_MODEM) {
	}
}

void BTFWUpdateEnd(void *pInterPtcl, unsigned int eInCommType)  //FW, DB 업데이트 시작 genebe 3
{
  	unsigned char cResult = 0x00;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	if(eInCommType==eCOMM_TYPE_UART_BT)
	{
		GITDebugPrintf("[%s] run\r\n", __FUNCTION__);

		SetLedOnOffCtl(LED_OFF, eLED_GPS);			//GREEN
		SetLedOnOffCtl(LED_ON, eLED_SERVER);	//RED
		SetLedOnOffCtl(LED_OFF, eLED_CAN);			//BLUE

		if( g_stCFD_BT_Update_Info.eInstallFileType == eINSTALL_FILE_TYPE_BOOT || g_stCFD_BT_Update_Info.eInstallFileType == eINSTALL_FILE_TYPE_APP )
		{
			if ( DownloadClose_CFD() )
			{
				cResult = 0x01;
			}
		}
		else
		{
			if ( DownloadClose() )
			{
				cResult = 0x01;
			}
		}

		GITDebugPrintf("[%s] run \r\n", __FUNCTION__);
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
	}
	else if(eInCommType==eCOMM_TYPE_UART_MODEM) {
	}

	cResult = cResult;
    g_bFWUpdateFlagFromBLE = FALSE;
}

bool DownloadClose_CFD()
{
	stCANFDBoardUpdateInfo stFDBoardInfo;
	if ( g_u16UpdateFileCheckSum == g_stCFD_BT_Update_Info.stFileInfo.nCheckSum )
	{
		CFD_SetInitComplete(false);
		SetOBDState(eOBD_InitCanFD);
		SetCanFDInitHandler();
		GetAutolinkConfigProperty(eAutoLinkConfig_FDBoardUpdateInfo,(void*)&stFDBoardInfo);
		if( g_stCFD_BT_Update_Info.eInstallFileType == eINSTALL_FILE_TYPE_BOOT)
		{
			stFDBoardInfo.usBootVersion = g_stCFD_BT_Update_Info.stFileInfo.DB_Ver;
			stFDBoardInfo.uiBootSize = g_stCFD_BT_Update_Info.stFileInfo.DB_Size;
			stFDBoardInfo.usBootCheckSum = g_stCFD_BT_Update_Info.stFileInfo.nCheckSum;
		}
		else	//g_stCFD_BT_Update_Info.eInstallFileType == eINSTALL_FILE_TYPE_APP
		{
			stFDBoardInfo.usAppVersion = g_stCFD_BT_Update_Info.stFileInfo.DB_Ver;
			stFDBoardInfo.uiAppSize = g_stCFD_BT_Update_Info.stFileInfo.DB_Size;
			stFDBoardInfo.usAppCheckSum = g_stCFD_BT_Update_Info.stFileInfo.nCheckSum;
		}
		SetAutolinkConfigProperty(eAutoLinkConfig_FDBoardUpdateInfo,(void*)&stFDBoardInfo);
		printf("Ver:%d Size:%d CS:%d\r\n",g_stCFD_BT_Update_Info.stFileInfo.DB_Ver,g_stCFD_BT_Update_Info.stFileInfo.DB_Size,g_stCFD_BT_Update_Info.stFileInfo.nCheckSum);
		return true;
	}
	else
	{
		printf("[%s] Don't Match Recv Checksum %d, Calc Checksum %d\r\n", __FUNCTION__, g_DownloadInfo.nCheckSum, g_u16UpdateFileCheckSum);
		return false;
	}
}

void BTFWDeviceReset(void *pInterPtcl, unsigned int eInCommType)  //FW, DB 업데이트 시작 genebe 4
{
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	U8 ucInputbuf[16];
#if defined(AES_ENABLE)
	unsigned char cResult=0;
#endif

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	memcpy(ucInputbuf, (void const*)&pPayloadPtcl->pPayload[0], pPayloadPtcl->DataLength-6);
#if defined(AES_ENABLE)
	if( SecurityCheck(ucInputbuf) == false )
	{
		printf("!!!!!!!!!!!!!!!!!!!!!!!FAIL!!!!!!!!!!!!!!!!!");
		cResult = 0x00;
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
		return ;
	}
#endif

  	GITDebugPrintf("[%s] run\r\n", __FUNCTION__);
    g_bFWUpdateFlagFromBLE = FALSE;

    g_unBkramSwResetSignal = SW_RESET_SIGNAL;
	HalDrvWrBkRam((unsigned char*)&g_unBkramSwResetSignal,BKRAM_SW_RESET_SIGNAL_ADDR,BKRAM_SW_RESET_SIGNAL_SIZE);

    HalDrvPower_ClearStandbyFlag();

	SetWDTReset(1000);

	while( TRUE ){}; // Reset될때까지 대기함. 메인루틴의 WatchDoc과 충돌 회피
}

void BTTERMINALInfoRes(void *pInterPtcl, unsigned int eInCommType)    //test get tinfo (command)
{
 // sof + reserve 0 + rev 1 + funcion id +payload ( VIN[17] + usim number [30] + phone [16] + cs + eof
 	char * carrSeriaNumber = GetFWSerialNumber();

	if(eInCommType==eCOMM_TYPE_UART_BT)
	{
      uint8_t arrTerminalbuf[512] = {0,};
      uint16_t nTerminalbufLen;
#ifdef FIX_VIN
	  memcpy(&g_FirmwareInfo.m_strVIN, FIX_VIN_NUMBER, DCS_AUTOVIN_SIZE);
#endif

//#define ENABLE_TEST_VIN
#ifdef ENABLE_TEST_VIN
	  char cArrVin[20]={0,};
	  memcpy(cArrVin,"KMTHC81CSLU026428",DCS_AUTOVIN_SIZE);
	  sprintf((char*)arrTerminalbuf, "%-18s%-24s%-16s" , "KMTHC81CSLU026428", BkSram_ModemInfo.aCCID, BkSram_ModemInfo.aPhoneNo);
	  SetVINCode((unsigned char*)cArrVin);
	  HandlerNewVin((char *)cArrVin);
#else
	  if( strstr(carrSeriaNumber,"SGK") != NULL )
	  {
      sprintf((char*)arrTerminalbuf, "%-18s%-24s%-16s%-16s" , g_FirmwareInfo.m_strVIN, BkSram_ModemInfo.aCCID, BkSram_ModemInfo.aPhoneNo,ModemManagerData.aSIMI);
	  }else
	  {
	  	sprintf((char*)arrTerminalbuf, "%-18s%-24s%-16s" , g_FirmwareInfo.m_strVIN, BkSram_ModemInfo.aCCID, BkSram_ModemInfo.aPhoneNo);
	  }
#endif
      nTerminalbufLen = strlen((char*)arrTerminalbuf);

      GetFirmwareInfo(&g_FirmwareInfo);
	  GITDebugPrintf("[%s] run \r\n", __FUNCTION__);

	  SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)arrTerminalbuf, nTerminalbufLen, (eCommType)eInCommType);
	}
}

//void BTVEHICLEInfoRes(void *pInterPtcl, unsigned int eInCommType)
//{
//
//    uint16_t i;
//    uint16_t argc;
//    char *p2str;
////    char *ptrRestList;
//    char *argv[7];
//	stVehicleInfo *ptrVehicleInfo;
//	uint16_t nRet;
//	char strLatitude[20];
//	char strLongitude[20];
//	double dLatitude=0;
//	double dLongitude=0;
//
//    ptrVehicleInfo = &g_stVehicleInfoList;
//	memset((char *)ptrVehicleInfo, 0x00, sizeof(stVehicleInfo));   //초기화부분 삭제 확인 필요
//	memset((char *)strLatitude, 0x00, sizeof(strLatitude));
//	memset((char *)strLongitude, 0x00, sizeof(strLongitude));
//
//  	unsigned char cResult = 0x00;
//    stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
////    stDownloadingReq *pstDownloadingReq = (stDownloadingReq*) pPayloadPtcl->pPayload;  //genebe
//	//pstDownloadingReq = pstDownloadingReq;
//
//	if(eInCommType==eCOMM_TYPE_UART_BT)
//	{
//		  /*
//		  memset(&g_FirmwareInfo.m_strVehicleArea, pPayloadPtcl->pPayload[0], 3);
//		  memset(&g_FirmwareInfo.m_strVehicleArea, pPayloadPtcl->pPayload[3], 4);
//		  memset(&g_FirmwareInfo.m_strVehicleArea, pPayloadPtcl->pPayload[7], 4);
//		  memset(&g_FirmwareInfo.m_strVehicleArea, pPayloadPtcl->pPayload[11], 3);
//		  */
//
//		p2str = (char *)pPayloadPtcl->pPayload;  //payload
//
//
//		argv[0] = strtok((char *)p2str, "!");
//	//	printf("%u: %s\n", 0, argv[0]);
//
//		for (argc = 1, i = 1; i < 6; i++) {   //5개 define 걸어야함.
//			argv[i] = strtok((char *)NULL, "!");
//			if (argv[i] == NULL) {
//				break;
//			}
//			else {
//			argc++;  //4개 (area, modelcode, year, enginecode)
//
//	//	    	printf("%u: %s\n", i, argv[i]);
//			}
//		}
//
//
//		// "AREA"
//		nRet = SearchInfoString((char *)argv[1], (char *)ptrVehicleInfo->m_aArea, MAX_LEN_VINFO_AREA);
//		if(nRet == 0) {
//			return ;
//		}
//
//		// "MODEL_CODE"
//		nRet = SearchInfoString((char *)argv[2], (char *)ptrVehicleInfo->m_aModelCode, MAX_LEN_VINFO_MODEL_CODE);
//		if(nRet == 0) {
//			return ;
//		}
//
//		// "YEAR"
//		nRet = SearchInfoString((char *)argv[3], (char *)ptrVehicleInfo->m_aYear, MAX_LEN_VINFO_YEAR);
//		if(nRet == 0) {
//			return ;
//		}
//
//		// "ENGINE_CODE"
//		nRet = SearchInfoString((char *)argv[4], (char *)ptrVehicleInfo->m_aEngineCode, MAX_LEN_VINFO_ENGINE_CODE);
//		if(nRet == 0) {
//			return ;
//		}
//
//		// "Latitude"
//		nRet = SearchInfoString((char *)argv[5], (char *)strLatitude, sizeof(strLatitude));
//		if(nRet == 0) {
//			return ;
//		}
//		else	dLatitude = atoi(strLatitude);
//		printf("%d\r\n",dLatitude);
//
//		// "Longitude"
//		nRet = SearchInfoString((char *)argv[6], (char *)strLongitude, sizeof(strLongitude));
//		if(nRet == 0) {
//			return ;
//		}
//		else dLongitude = atoi(strLongitude);
//		printf("%d\r\n",dLongitude);
//
//		cResult = 0x01;
//		GITDebugPrintf("[%s] run \r\n", __FUNCTION__);
//		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
//	}
//
//}

void BTSetGPSInfo(void *pInterPtcl, unsigned int eInCommType)
{
    uint16_t i;
    char *argv[6];
	char strLatitude[20];
	char strLongitude[20];
	double dLatitude=0;
	double dLongitude=0;
	unsigned char cResult = 0x00;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	memset((char *)strLatitude, 0x00, sizeof(strLatitude));
	memset((char *)strLongitude, 0x00, sizeof(strLongitude));

    stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;

	if(eInCommType==eCOMM_TYPE_UART_BT)
	{
		argv[0] = strtok((char *)pPayloadPtcl->pPayload, "!");

		for ( i=1; i<6; i++ )
		{
			argv[i] = strtok((char *)NULL, "!");
			if (argv[i] == NULL) {
				break;
			}
		}

		// "Latitude"
		memcpy(strLatitude,argv[4],strlen(argv[4]));
		dLatitude = atoi(strLatitude);
        dLatitude = dLatitude/1000000;
		printf("%f\r\n",dLatitude);
#ifndef ENABLE_UBLOX_GPS_DATA
		dLatitude = ConvReverseGPSData(dLatitude);
#endif
		printf("%f\r\n",dLatitude);

		// "Longitude"
		memcpy(strLongitude,argv[5],strlen(argv[5]));
		dLongitude = atoi(strLongitude);    
        dLongitude = dLongitude/1000000;
		printf("%f\r\n",dLongitude);
#ifndef ENABLE_UBLOX_GPS_DATA
		dLongitude = ConvReverseGPSData(dLongitude);
#endif
		printf("%f\r\n",dLongitude);

		SetAutolinkConfigProperty(eAutoLinkConfig_LastLatitude,&dLatitude);
		SetAutolinkConfigProperty(eAutoLinkConfig_LastLongitude,&dLongitude);
		g_GPSInfo.lat=g_dSavedGpsLat=dLatitude;
		g_GPSInfo.lon=g_dSavedGpsLon=dLongitude;

		cResult = 0x01;
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, 1, (eCommType)eInCommType);
	}
}

bool SecurityCheck(unsigned char *ucInputbuf)
{
	aes256_context ctx;
	U8 ucKey[32];
//	U8 ucTempBuff[16];
	bool bRet=false;

	memset(ucKey,0x00,sizeof(ucKey));
	memcpy(ucKey,g_ucRandomKey,sizeof(g_ucRandomKey));
	memset(g_ucRandomKey,0x00,sizeof(g_ucRandomKey));

	aes256_init(&ctx, ucKey);
	aes256_decrypt_ecb(&ctx, ucInputbuf);	//Inputbuf에 decrypt해서 나옴

#if defined(AESLOG)
	printf("SecurityCheck : ");
	for(int i=0;i<16;i++)		printf("%02X ",ucInputbuf[i]);
	printf("\r\n");
#endif

	if(strncmp("AL",(char const*)ucInputbuf,2)==0)	bRet = true;
	else 																bRet = false;

	return bRet;
}
////////////////생산 프로그램 개발 중///////////////
void BTDoor_UnlockRes(void *pInterPtcl, unsigned int eInCommType)	 //생산 프로그램 genebe 1 개발
{
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	U8 ucInputbuf[16];
	unsigned char cResult=0;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	memcpy(ucInputbuf, (void const*)&pPayloadPtcl->pPayload[0], pPayloadPtcl->DataLength-6);
#if defined(AES_ENABLE)
	if( SecurityCheck(ucInputbuf) == false )
	{
		printf("!!!!!!!!!!!!!!!!!!!!!!!FAIL!!!!!!!!!!!!!!!!!");
		cResult = 0x00;
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
		return ;
	}
#endif
    //RequestForwardingSmartKey(true);

    Set_Actuator( ACTUATOR_TYPE_DOORUNLOCK );

    cResult = 0x01;

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);

	//RequestForwardingSmartKey(false);

}


void BTDoor_LockRes(void *pInterPtcl, unsigned int eInCommType)
{
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	U8 ucInputbuf[16];
	unsigned char cResult=0;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	memcpy(ucInputbuf, (void const*)&pPayloadPtcl->pPayload[0], pPayloadPtcl->DataLength-6);
#if defined(AES_ENABLE)
	if( SecurityCheck(ucInputbuf) == false )
	{
		printf("!!!!!!!!!!!!!!!!!!!!!!!FAIL!!!!!!!!!!!!!!!!!");
		cResult = 0x00;
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
		return ;
	}
#endif
    //RequestForwardingSmartKey(true);

    Set_Actuator( ACTUATOR_TYPE_DOORLOCK );

    cResult = 0x01;

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
	//RequestForwardingSmartKey(false);
}

void BTSensor_InitRes(void *pInterPtcl, unsigned int eInCommType)
{
    stSensorInfo stGyroAngle;
    //RequestForwardingSmartKey(true);
    unsigned char cResult[4];

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

    //MONI 20181031
    //InitializeSensorManager();  //센서 초기화
    MeasureGyroAngle();
    GetGyroNavieAngle(&stGyroAngle);
	stGyroAngle.bCalibration = true;
    SetGyroInitializeAngle(&stGyroAngle);
	g_bFuelLevelCheckFlag = true;
	UpdataGyroDefualtAngle();

	cResult[0] = 0x01;
    cResult[1] = stGyroAngle.nX;
    cResult[2] = stGyroAngle.nY;
    cResult[3] = stGyroAngle.nZ;
	printf("BTSensor_InitRes X:%d Y:%d Z:%d\r\n",stGyroAngle.nX,stGyroAngle.nY,stGyroAngle.nZ);

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
	//RequestForwardingSmartKey(false);
}

void BTDoor_UnlockStatusRes(void *pInterPtcl, unsigned int eInCommType)
{
    unsigned int cResult = 0;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

    cResult = Get_DoorLock();

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}

void BTDoor_LockStatusRes(void *pInterPtcl, unsigned int eInCommType)
{
   unsigned int cResult = 0;

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

   cResult = Get_DoorLock();

   SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}

void BTEngine_StatusRes(void *pInterPtcl, unsigned int eInCommType)
{
   unsigned int cResult[2] = {0,};

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

   cResult[0] = Get_RPM();
   cResult[1] = Get_VehicleStatus();

   SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}

#if defined(PROTOCOL18)
void BTAirConStatusRes(void *pInterPtcl, unsigned int eInCommType)
{
 //printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@3\r\n");
#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&g_stAirconPayload, sizeof(g_stAirconPayload), (eCommType)eInCommType);
}

void BTCtrlFuncSettingInfo(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char cResult[9]={0,};
//printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@2\r\n");
#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	if(g_bCtrlFuncParsingFail == false) //
	{
		cResult[0] = 0x01; // CONTROL_FUNCTION DB 존재
	}

	for(int i=0;i<ACTUATOR_MAX_CTRL_CNT;i++)
	{
		for(int j=0;j<CTRL_SETTING_SIZE;j++)
		{
			if(strncmp(g_stActuatorCtrlData[i].m_ucCtrlFunction, g_pcCTRLFunc[j], sizeof(g_pcCTRLFunc[i]))==0)
			{
				cResult[j+1]=g_stActuatorCtrlData[i].m_ucSupport;
			}
		}
	}

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}

void BTGetDeviceTime(void *pInterPtcl, unsigned int eInCommType) //dahae
{
//printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@4\r\n");
#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	stHalRTCTypeDef stLocalDate;
	stHalRTCTypeDef stUTCDate;

	stHalRTCTypeDef stDeviceTime[2];

	uint32_t nUTCTime = GetUTCTime();
	uint32_t nLocalTime = GetLocalTimefromTime(nUTCTime);

	GetDatefromTime2(&stLocalDate, nLocalTime); // local --> stDate 구조체에 nTime을 넣어줌
	GetDatefromTime2(&stUTCDate, nUTCTime);

	stDeviceTime[0] = stLocalDate;
	stDeviceTime[1] = stUTCDate;

	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&stDeviceTime, sizeof(stDeviceTime), (eCommType)eInCommType);
}

#if defined(REASON_8BYTE)
void ResponseBtSmartkey(uint8_t ucResult, uint64_t ullReason)
#else
void ResponseBtSmartkey(uint8_t ucResult, uint32_t ucReason)
#endif
{
	stCarReport stReport;
	memset((char*)&stReport,0,sizeof(stCarReport));

	//memcpy(stMsgMdm.carReport.rpSmartKey.Response.Guid,pstMsgMdm->carReport.rpSmartKey.Request.Guid,MAX_GUID_LENGTH);

	stReport.rpSmartKey.Response.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
	stReport.rpSmartKey.Response.OccurredEventUtcTime = GetUTCTime();
	stReport.rpSmartKey.Response.Result = ucResult;
#if defined(REASON_8BYTE)
	stReport.rpSmartKey.Response.Reason = ullReason;
#else
	stReport.rpSmartKey.Response.Reason = ucReason;
#endif
	stReport.rpSmartKey.Response.ReceviedTime = GetLocalTimefromTime(GetUTCTime());
#if defined(PROTOCOL18)
	memcpy(stReport.rpSmartKey.Response.BTControlKey, g_ucBTControlKey, 16);
#endif

	// send response with protocl /40 : eSysSmartkeyReqType_Modem
	// send response with protocl /44 : eSysSmartkeyReqType_Bt
	// send response with protocl /97 : eSysSmartkeyReqType_Sys
	stReport.rpSmartKey.Response.ucSysSmartkeyReqType = eSysSmartkeyReqType_BT;

	//printf("Response Bt Smartkey in System Message Manager\n");

	Send2MngModem(eMngSysMsg,eRspReport,eR_RspSmartKey,&stReport,0);
}


bool GetActiveRemoteControl(unsigned char* ucStrItem, unsigned char unLength)
{
	boolean_t bActive = false;
	for(int i=0;i<ACTUATOR_MAX_CTRL_CNT;i++)
	{
		for(int j=0;j<CTRL_SETTING_SIZE;j++)
		{
			if( strncmp(g_stActuatorCtrlData[i].m_ucCtrlFunction, (char const*)ucStrItem, unLength) == 0 )
			{
				bActive = g_stActuatorCtrlData[i].m_ucSupport;
			}
		}
	}

	return bActive;
}


void BTOTCnRemoteCtrl(void *pInterPtcl, unsigned int eInCommType) //dahae
{
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;

	stREQ_CTRL_PAYLOAD stReqCtrlPayload;

	unsigned short usCommandID;
	unsigned char ucControlID;
//printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@1\r\n");
#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	//stMsgObd stBtObdMsg;
	stCarReport stBTmsg;

    memset(&stBTmsg,0x00,sizeof(stBTmsg));

	memcpy(&usCommandID, &(pPayloadPtcl->pPayload[BT_COMMAND_ID]), sizeof(usCommandID));
	ucControlID = pPayloadPtcl->pPayload[BT_OTC_REQ_CONTROL];

 	if(usCommandID==OTC_COMMAND_ID) // OTC COMMAND
 	{
		Send2BTOTCResPayload((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (stOTC_PTCL_PAYLOAD*)&g_stOTCStatusPayload, ucControlID, eInCommType);
	}
	else // Remote Control
	{
		if(usCommandID&BT_CTRL_COMMAND_TYPE_CHCK) // Control status
		{
			Send2BTRemoteCtrlResPayload((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (stRemote_PTCL_PAYLOAD*)&g_stResRemotePayload, usCommandID, eInCommType);

			if(g_stResRemotePayload.ucResult == BT_REMOTE_CTRL_SUCCESS || g_stResRemotePayload.ucResult == BT_REMOTE_CTRL_FAIL)
			{
				if(g_stResRemotePayload.ulReason != eRETURNFAIL_TYPE_ALREADY_RUNNING)
				{
					RequestForwardingSmartKey(false);
					m_stMsgHdData.bAlreadyRcvSmartkey = false;
				}
			}
		}
		else // Control Request
		{
		   	memcpy(&stReqCtrlPayload, (stREQ_CTRL_PAYLOAD*)(pPayloadPtcl->pPayload), sizeof(stREQ_CTRL_PAYLOAD));

			Send2BTRemoteCtrlResPayload((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (stRemote_PTCL_PAYLOAD*)&g_stReqRemotePayload, usCommandID, eInCommType);

			if(m_stMsgHdData.bAlreadyRcvSmartkey == false && GetForwardingSmartKey2BtFlag() == false)
			{
				m_stMsgHdData.bAlreadyRcvSmartkey = true;

				RequestForwardingSmartKey(true);

				memcpy(g_ucBTControlKey, g_stReqRemotePayload.ucBTControlKey, sizeof(g_ucBTControlKey));

				switch(usCommandID){
					case BT_CTRL_ENGINE_START:
						stBTmsg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_STARTING;
						if(g_stAirconPayload.ucDBCheck == 0) //공조 DB가 존재하지 않을 경우
						{
							stBTmsg.rpSmartKey.Request.Temperature = 0xFFFF;
							stBTmsg.rpSmartKey.Request.CheckTemperature = 0xFF;
						}
						else
						{
							for(int i=0;i<g_ucTotAirCnt;i++)
							{
						  		if( g_stActuatorAirConData[i].m_usTemp == stReqCtrlPayload.usTemperature )
						  		{
									stBTmsg.rpSmartKey.Request.Temperature = g_stActuatorAirConData[i].m_ucValue1|(g_stActuatorAirConData[i].m_ucValue2<<8);
									stBTmsg.rpSmartKey.Request.CheckTemperature = g_stActuatorAirConData[i].m_ucValue3;
						  		}
							}
						}
						stBTmsg.rpSmartKey.Request.KeepPowerOnTime = stReqCtrlPayload.ucDurationTime;

						// application just send dfrost properties, then me set rear defogger and defrost with dfrost of application
	        			stBTmsg.rpSmartKey.Request.Defrost= stReqCtrlPayload.ucDefrost;
						stBTmsg.rpSmartKey.Request.RearDefogger = stReqCtrlPayload.ucDefrost;

						if( stReqCtrlPayload.ucDefrost == true )
						{
						    stBTmsg.rpSmartKey.Request.Defrost = GetActiveRemoteControl("AIRCON_VENT",strlen("AIRCON_VENT"));
	        				stBTmsg.rpSmartKey.Request.RearDefogger = GetActiveRemoteControl("REARDEFOG",strlen("REARDEFOG"));
						}

						break;
					case BT_CTRL_DOOR:
						stBTmsg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_DOOR;
						break;
					case BT_CTRL_HAZARD_LAMP:
						stBTmsg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_EMERGENCY_LIGHT;
						break;
					case BT_CTRL_HORN_HAZRARD_LAMP:
						stBTmsg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_HORN_EMERGENCY_LIGHT;
						break;
					case eREMOTE_CON_CMD_TYPE_NONE:
					default:
						g_stResRemotePayload.ucResult = BT_REMOTE_CTRL_FAIL;
						g_stResRemotePayload.ulReason = eRETURNFAIL_TYPE_NONE; // 이미 제어중
						ResponseBtSmartkey(g_stResRemotePayload.ucResult,g_stResRemotePayload.ulReason);
						return;
				}
	       		stBTmsg.rpSmartKey.Request.ControlType = g_stReqRemotePayload.ucControl;
	   			stBTmsg.rpSmartKey.Request.ucSysSmartkeyReqType = eSysSmartkeyReqType_BT;

				memcpy(stBTmsg.rpSmartKey.Request.BTControlKey, g_ucBTControlKey, sizeof(g_ucBTControlKey));

				g_stResRemotePayload.ucResult = BT_REMOTE_CTRL_WAIT;

				Send2MngSysMsg(eMngBt, eReqReport, eTrue, (stCarReport*)&stBTmsg, 0);
				//Send2MngObd(eMngSysMsg,eReqReport,eR_ReqSmartKey,&stBTmsg,eMngBt);
			}
			else
			{
				g_stResRemotePayload.ucResult = BT_REMOTE_CTRL_FAIL;
#if defined(REASON_8BYTE)
				g_stResRemotePayload.ullReason = eRETURNFAIL_TYPE_ALREADY_RUNNING; // 이미 제어중
#else
				g_stResRemotePayload.ulReason = eRETURNFAIL_TYPE_ALREADY_RUNNING; // 이미 제어중
#endif

				// response result to server
				ResponseBtSmartkey(BT_REMOTE_CTRL_FAIL,eRETURNFAIL_TYPE_ALREADY_RUNNING);
			}
		}
	}

}
#endif

void BT_EncryptRemoteControl(void *pInterPtcl, unsigned int eInCommType)
{
	//uint8_t ucEncDecData[128]={0,};
	uint8_t ucCheckSum = 0;
	stCarReport stBTmsg;
	stREQ_CTRL_PAYLOAD stReqCtrlPayload;
	//uint8_t ucBuff[256]={0,};

	memset(&stReqCtrlPayload,0x00,sizeof(stReqCtrlPayload));
    memset(&stBTmsg,0x00,sizeof(stBTmsg));

	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	GITDebugPrintf("[%s] run\r\n", __FUNCTION__);

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif
//    if( LOCK_GET_STATE()!= eLOCK_STATE_UNLOCK )
//    {
//		uint8_t cResult = 0x00;
//		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
//        printf("Error:Not unlock\n");
//        return;
//    }
#if defined(AES_ENABLE_2_LOG)
	printf("Bluetooth Remote Control Data : size : %d\n",pPayloadPtcl->DataLength);
	hexdump(pPayloadPtcl->pPayload,pPayloadPtcl->DataLength-6);
#endif
	// decrypt bluetooth data
	DecryptBluetoothRemoteControlData(pPayloadPtcl->pPayload,pPayloadPtcl->DataLength,&stReqCtrlPayload);
//	printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@stReqCtrlPayload.ucIndex:%d@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n",stReqCtrlPayload.ucIndex);

    // convert message info for response
//    MakeDefaultBtRemoteControlResultforResponse(&stResponse,&stRemoteCtrl);

	// check remote control checksum
	ucCheckSum = stReqCtrlPayload.ucCheckSum;
	stReqCtrlPayload.ucCheckSum = 0;

	memcpy(pPayloadPtcl->pPayload,&stReqCtrlPayload,sizeof(stReqCtrlPayload));
//	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~0000\r\n");
	if( (uint8_t)CalCheckSum((uint8_t*)&stReqCtrlPayload,sizeof(stReqCtrlPayload)) == ucCheckSum )
	{
		if(stReqCtrlPayload.usCommand==OTC_COMMAND_ID) // OTC COMMAND
		{
		  	//
			//Send2BTOTCResPayload((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (stOTC_PTCL_PAYLOAD*)&g_stOTCStatusPayload, stReqCtrlPayload.ucControl, eInCommType);
		}
		else // Remote Control
		{
			if(stReqCtrlPayload.usCommand&BT_CTRL_COMMAND_TYPE_CHCK) // Control status
			{
				Send2BTRemoteCtrlResEncPayload((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (stRemote_PTCL_PAYLOAD*)&g_stResRemotePayload, stReqCtrlPayload.usCommand, eInCommType);
//				printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~1111\r\n");
				if(g_stResRemotePayload.ucResult == BT_REMOTE_CTRL_SUCCESS || g_stResRemotePayload.ucResult == BT_REMOTE_CTRL_FAIL)
				{
					if(g_stResRemotePayload.ulReason != eRETURNFAIL_TYPE_ALREADY_RUNNING)
					{
						RequestForwardingSmartKey(false);
						m_stMsgHdData.bAlreadyRcvSmartkey = false;
					}
				}
			}
			else // Control Request
			{
//			  	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~3333 %d %d\r\n",m_stMsgHdData.bAlreadyRcvSmartkey,GetForwardingSmartKey2BtFlag());
				Send2BTRemoteCtrlResEncPayload((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (stRemote_PTCL_PAYLOAD*)&g_stReqRemotePayload, stReqCtrlPayload.usCommand, eInCommType);

				if(m_stMsgHdData.bAlreadyRcvSmartkey == false && GetForwardingSmartKey2BtFlag() == false)
				{
//				  	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~4444\r\n");
					m_stMsgHdData.bAlreadyRcvSmartkey = true;

					RequestForwardingSmartKey(true);

					memcpy(g_ucBTControlKey, g_stReqRemotePayload.ucBTControlKey, sizeof(g_ucBTControlKey));

					switch(stReqCtrlPayload.usCommand){
						case BT_CTRL_ENGINE_START:
							stBTmsg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_STARTING;
							if(g_stAirconPayload.ucDBCheck == 0) //공조 DB가 존재하지 않을 경우
							{
								stBTmsg.rpSmartKey.Request.Temperature = 0xFFFF;
								stBTmsg.rpSmartKey.Request.CheckTemperature = 0xFF;
							}
							else
							{
								for(int i=0;i<g_ucTotAirCnt;i++)
								{
									if( g_stActuatorAirConData[i].m_usTemp == stReqCtrlPayload.usTemperature )
									{
										stBTmsg.rpSmartKey.Request.Temperature = g_stActuatorAirConData[i].m_ucValue1|(g_stActuatorAirConData[i].m_ucValue2<<8);
										stBTmsg.rpSmartKey.Request.CheckTemperature = g_stActuatorAirConData[i].m_ucValue3;
									}
								}
							}
							stBTmsg.rpSmartKey.Request.KeepPowerOnTime = stReqCtrlPayload.ucDurationTime;

							// application just send dfrost properties, then me set rear defogger and defrost with dfrost of application
							stBTmsg.rpSmartKey.Request.Defrost= stReqCtrlPayload.ucDefrost;
							stBTmsg.rpSmartKey.Request.RearDefogger = stReqCtrlPayload.ucDefrost;

							if( stReqCtrlPayload.ucDefrost == true )
							{
							    stBTmsg.rpSmartKey.Request.Defrost = GetActiveRemoteControl("AIRCON_VENT",strlen("AIRCON_VENT"));
								stBTmsg.rpSmartKey.Request.RearDefogger = GetActiveRemoteControl("REARDEFOG",strlen("REARDEFOG"));
							}

							break;
						case BT_CTRL_DOOR:
							stBTmsg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_DOOR;
							break;
						case BT_CTRL_HAZARD_LAMP:
							stBTmsg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_EMERGENCY_LIGHT;
							break;
						case BT_CTRL_HORN_HAZRARD_LAMP:
							stBTmsg.rpSmartKey.Request.CommandType = eREMOTE_CON_CMD_TYPE_HORN_EMERGENCY_LIGHT;
							break;
						case eREMOTE_CON_CMD_TYPE_NONE:
						default:
							g_stResRemotePayload.ucResult = BT_REMOTE_CTRL_FAIL;
							g_stResRemotePayload.ulReason = eRETURNFAIL_TYPE_NONE; // 이미 제어중
							RequestForwardingSmartKey(false);
							m_stMsgHdData.bAlreadyRcvSmartkey = false;
							ResponseBtSmartkey(g_stResRemotePayload.ucResult,g_stResRemotePayload.ulReason);
							return;
					}
					stBTmsg.rpSmartKey.Request.ControlType = g_stReqRemotePayload.ucControl;
					stBTmsg.rpSmartKey.Request.ucSysSmartkeyReqType = eSysSmartkeyReqType_BT;

					memcpy(stBTmsg.rpSmartKey.Request.BTControlKey, g_ucBTControlKey, sizeof(g_ucBTControlKey));

					g_stResRemotePayload.ucResult = BT_REMOTE_CTRL_WAIT;

					Send2MngSysMsg(eMngBt, eReqReport, eTrue, (stCarReport*)&stBTmsg, 0);
					//Send2MngObd(eMngSysMsg,eReqReport,eR_ReqSmartKey,&stBTmsg,eMngBt);
				}
				else
				{
//				  	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~5555\r\n");
					g_stResRemotePayload.ucResult = BT_REMOTE_CTRL_FAIL;
#if defined(REASON_8BYTE)
					g_stResRemotePayload.ullReason = eRETURNFAIL_TYPE_ALREADY_RUNNING; // 이미 제어중
#else
					g_stResRemotePayload.ulReason = eRETURNFAIL_TYPE_ALREADY_RUNNING; // 이미 제어중
#endif

					// response result to server
					ResponseBtSmartkey(BT_REMOTE_CTRL_FAIL,eRETURNFAIL_TYPE_ALREADY_RUNNING);
				}
			}
		}
	}
	else
	{

	}
}

boolean_t EncryptBluetoothRemoteControlData(uint8_t* pucSrc, uint8_t ucSrcLength, uint8_t* pucDst, uint8_t* ucSize)
{
	aes256_context ctx;
	uint8_t ucBluetoothNewKey[32] = {0,};
	uint8_t ucEncDecData[128]={0,};
	uint8_t ucBase64EncDecData[128]={0,};
	uint8_t ucBase64EncDecDataLength = 0;
	//uint8_t ucDataIndex = 0;
	uint8_t ucRet=0;

	//DAMO_CRYPT_Base64_Decode(ucBase64EncDecData,&ucBase64EncDecDataLength,ucEncDecData,strlen(ucEncDecData));
#if defined(AES_ENABLE_2_LOG)
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
	hexdump((char*)pucSrc,ucSrcLength);
#endif
	ucRet = mbedtls_base64_encode(ucBase64EncDecData,128,(size_t*)&ucBase64EncDecDataLength,pucSrc,ucSrcLength);
	if( ucRet != 0 )
	{
		printf("####packet encode fail###\r\n");
	}
#if defined(AES_ENABLE_2_LOG)
	printf("base64:\r\n");
	hexdump(ucBase64EncDecData,ucBase64EncDecDataLength);
#endif
	// copy encrypted data from application
	memcpy(ucEncDecData,&ucBase64EncDecData[0],ucBase64EncDecDataLength);
	*ucSize = strlen((char const*)ucEncDecData);	// aes256탈때 중간에 null이 들어갈수 있어서 데이터 길이를 저장했다가 사용

	// decrypt remote control data
	GetBluetoothNewKey(ucBluetoothNewKey);

	aes256_init(&ctx, ucBluetoothNewKey);
	//aes_encrypt_cbc
	aes256_encrypt_ecb(&ctx, ucEncDecData);

	// copy structure
    memcpy(pucDst,ucEncDecData,*ucSize);
#if defined(AES_ENABLE_2_LOG)
	printf("aes:\r\n");
	hexdump((char*)pucDst,*ucSize);
#endif
	aes256_done(&ctx);

	return true;
}

void Send2BTOTCResEncPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stOTC_PTCL_PAYLOAD *pstOTCPayload, unsigned char ucControlID, unsigned int eInCommType)
{
  	uint8_t ucEncDecData[128]={0,};
	uint8_t ucEncDecDataSize = 0;

  	pstOTCPayload->ucIndex = pPayloadPtcl->pPayload[BT_OTC_REQ_INDEX];
 	memcpy((stOTC_PTCL_PAYLOAD *)pstOTCPayload->usCommand, (stBT_PTCL_PAYLOAD *)(pPayloadPtcl->pPayload[BT_COMMAND_ID]), sizeof(pstOTCPayload->usCommand));
 	pstOTCPayload->ucControl = ucControlID;
 	pstOTCPayload->ucResult = BT_OTC_RESP_SUCCESS;
	pstOTCPayload->ucEngineStatus = Get_VehicleStatus();
	pstOTCPayload->usVehicleStatus = Get_OTCVehicleStatus();
	pstOTCPayload->ucChecksum =  (uint8_t)CalCheckSum((uint8_t*)pstOTCPayload, sizeof(stOTC_PTCL_PAYLOAD)-3);	//except reserved
#if defined(AES_ENABLE_2_LOG)
	printf("origin:\r\n");
	hexdump((char*)pstOTCPayload,sizeof(stOTC_PTCL_PAYLOAD)-3);	//except reserved
#endif
	EncryptBluetoothRemoteControlData((uint8_t*)pstOTCPayload, sizeof(stOTC_PTCL_PAYLOAD)-3,ucEncDecData,&ucEncDecDataSize);	//except reserved

	if(BTGetConnectStatus()==true)
	{
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (unsigned char*)ucEncDecData, ucEncDecDataSize, (eCommType)eInCommType);
	}
}

void Send2BTRemoteCtrlResEncPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stRemote_PTCL_PAYLOAD *pstRemoteCtrlPayload, unsigned short usCommandID, unsigned int eInCommType)
{
  	uint8_t ucEncDecData[128]={0,};
	uint8_t ucEncDecDataSize = 0;

	pstRemoteCtrlPayload->ucIndex = pPayloadPtcl->pPayload[BT_CTRL_REQ_INDEX];
	pstRemoteCtrlPayload->usCommand = usCommandID;
	pstRemoteCtrlPayload->ucControl = pPayloadPtcl->pPayload[BT_CTRL_REQ_CONTROL];
	pstRemoteCtrlPayload->ucEngineStatus = Get_VehicleStatus();
	pstRemoteCtrlPayload->usVehicleStatus = Get_OTCVehicleStatus();
	memcpy(pstRemoteCtrlPayload->ucBTControlKey, &(pPayloadPtcl->pPayload[BT_CTRL_REQ_BT_CTRL_KEY]), sizeof(g_ucBTControlKey));
	pstRemoteCtrlPayload->ucChecksum =  CalCheckSum((uint8_t*)pstRemoteCtrlPayload,sizeof(stRemote_PTCL_PAYLOAD)-3);	//except reserved

//	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~pstRemoteCtrlPayload->ucIndex:%d~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n",pstRemoteCtrlPayload->ucIndex);

	if((usCommandID&BT_CTRL_COMMAND_TYPE_CHCK) == 0)
	{
		pstRemoteCtrlPayload->ucResult = BT_REMOTE_CTRL_SUCCESS;
	}
#if defined(AES_ENABLE_2_LOG)
	printf("origin:\r\n");
	hexdump((char*)pstRemoteCtrlPayload,sizeof(stRemote_PTCL_PAYLOAD)-3);	//except reserved
#endif
	EncryptBluetoothRemoteControlData((uint8_t*)pstRemoteCtrlPayload, sizeof(stRemote_PTCL_PAYLOAD)-3,ucEncDecData,&ucEncDecDataSize);	//except reserved

	if(BTGetConnectStatus()==true)
	{
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (unsigned char*)ucEncDecData, ucEncDecDataSize, (eCommType)eInCommType);
	}
}

uint16_t CalCheckSum(uint8_t* pcArrBuffer,int32_t nLength)
{
	uint16_t usCheckSum = 0;
	for(int i=0;i<nLength;i++)
	{
		usCheckSum += pcArrBuffer[i];
	}
#if defined(AES_ENABLE_2_LOG)
	//printf("usCheckSum : %d\r\n",usCheckSum);
#endif
	return usCheckSum;
}

boolean_t DecryptBluetoothRemoteControlData(uint8_t* pucSrc, uint8_t ucSrcLength, stREQ_CTRL_PAYLOAD* stReqCtrlPayload)
{
	aes256_context ctx;
	uint8_t ucBluetoothNewKey[32] = {0,};
	uint8_t ucEncDecData[128]={0,};
	uint8_t ucBase64EncDecData[128]={0,};
	size_t ucBase64EncDecDataLength = 0;
	uint8_t ucDataIndex = 0;

	// index (1)/ command (2)/ control (1)/ Result (1)/ Reason (8)/ Engine Status (1)/ Vehilce Status (2)/ BT-Key (16)/ check sum(1) / Key-Index (4)
	// 1 + 2 +1 +1 8 + 1 + 2 + 16 + 4 = 16 + 16 + 1 + 4 = 33 + 4;
	// adjust index in base session
	// real new session index position
	ucDataIndex = ucSrcLength - BLUETOOTH_HEADER_EXCLUE_LENGTH - MAX_BT_SESSION_INDEX_ENGTH;

	// set last bluetooth session index
	SetBluetoothNewIndex(&pucSrc[ucDataIndex]);

	// copy encrypted data from application
	memcpy(ucEncDecData,&pucSrc[0],ucDataIndex);
#if defined(AES_ENABLE_2_LOG)
	hexdump((char*)ucEncDecData,ucDataIndex);
#endif

	// decrypt remote control data
	GetBluetoothNewKey(ucBluetoothNewKey);
	aes256_init(&ctx, ucBluetoothNewKey);

	//aes_encrypt_cbc
	aes256_decrypt_ecb(&ctx, ucEncDecData);
#if defined(AES_ENABLE_2_LOG)
	hexdump((char*)ucEncDecData,strlen((char const*)ucEncDecData));
#endif

	//DAMO_CRYPT_Base64_Decode(ucBase64EncDecData,&ucBase64EncDecDataLength,ucEncDecData,strlen(ucEncDecData));
	mbedtls_base64_decode(ucBase64EncDecData,128,&ucBase64EncDecDataLength,ucEncDecData,strlen((char const*)ucEncDecData));
#if defined(AES_ENABLE_2_LOG)
	hexdump((char*)ucBase64EncDecData,ucBase64EncDecDataLength);
#endif

	// copy structure
    memcpy(stReqCtrlPayload,ucBase64EncDecData,ucBase64EncDecDataLength);
#if defined(AES_ENABLE_2_LOG)
    hexdump((char*)stReqCtrlPayload,ucBase64EncDecDataLength);
#endif

	aes256_done(&ctx);

	return true;
}

void SetBluetoothNewIndex(uint8_t* ucArrKeyIndex)
{
	memcpy(g_stBTBaseKeyInfo.ucBTSessionIndex,ucArrKeyIndex,MAX_BT_SESSION_INDEX_ENGTH);
#if defined(AES_ENABLE_2_LOG)
	printf("Bluetooth Key Index\n");
	hexdump(g_stBTBaseKeyInfo.ucBTSessionIndex,MAX_BT_SESSION_INDEX_ENGTH);
#endif
}

void GetBluetoothNewKey(uint8_t* pucArrNewKey)
{
	// check Bluetooth session key decript
	// decript certificaiton string
	memcpy(pucArrNewKey,g_stBTBaseKeyInfo.ucBTBaseKey,MAX_BASEKEY_SIZE);

	// adjust key postion data
	pucArrNewKey[SESSION_INDEX_1] ^= g_stBTBaseKeyInfo.ucBTSessionIndex[0];
	pucArrNewKey[SESSION_INDEX_2] ^= g_stBTBaseKeyInfo.ucBTSessionIndex[1];
	pucArrNewKey[SESSION_INDEX_3] ^= g_stBTBaseKeyInfo.ucBTSessionIndex[2];
	pucArrNewKey[SESSION_INDEX_4] ^= g_stBTBaseKeyInfo.ucBTSessionIndex[3];
#if defined(AES_ENABLE_2_LOG)
	printf("%s] session key \n",__FUNCTION__);
	hexdump(pucArrNewKey,MAX_BASEKEY_SIZE);
#endif
}

//void MakeDefaultBtRemoteControlResultforResponse(stBtRemoteControlResponse* pstResponse,stBtRemoteControlRequest* pstRemoteCtrl)
//{
//    pstResponse->ucIndex = pstRemoteCtrl->ucIndex;
//    pstResponse->usCommand = pstRemoteCtrl->usCommand;
//    pstResponse->ucControl = pstRemoteCtrl->ucControl;
//    pstResponse->ucResult = true;
//    pstResponse->ulReason = 0;
//    pstResponse->ucEngineStatus = m_stBluetoothControl.stVehicleInfo.eVehicleStatus;
//    pstResponse->usVehicleStatus = GetCovertBtVehicleStatus(&m_stBluetoothControl.stVehicleInfo);
//}

void BTLockVinRes(void *pInterPtcl, unsigned int eInCommType)
{
    uint8_t cNewVin = 0;
    unsigned int cResult[2] = {0,};

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

    cResult[0] = Get_RPM();
    cResult[1] = Get_VehicleStatus();
#ifdef FIX_VIN
	SetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,FIX_VIN_NUMBER);
#else
    SetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,(void*)&cNewVin);
#endif

    SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}

void BTUnLockVinRes(void *pInterPtcl, unsigned int eInCommType)
{
    uint8_t cNewVin = 1;
    unsigned int cResult[2] = {0,};

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

    cResult[0] = Get_RPM();
    cResult[1] = Get_VehicleStatus();

    SetVINCode(STR_DEFAULT_VIN);
    SetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)STR_DEFAULT_VIN);
    SetAutolinkConfigProperty(eAutoLinkConfig_AllowNewVin,(void*)&cNewVin);

    SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}

void BTCompleteRes(void *pInterPtcl, unsigned int eInCommType)
{
    unsigned int cResult[2] = {0,};

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

    cResult[0] = Get_RPM();
    cResult[1] = Get_VehicleStatus();

    SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}

void BTApnSettingRes(void *pInterPtcl, unsigned int eInCommType)
{
	stBT_PTCL_PAYLOAD *pPayloadPtcl = (stBT_PTCL_PAYLOAD*)pInterPtcl;
	U8 ucInput=0;
    unsigned char cResult[2] = {0,};

#if defined(AES_ENABLE_2)
	// check bt certification
	MACRO_BT_CERTIFICATION(pInterPtcl,eInCommType);
#endif

	ucInput = pPayloadPtcl->pPayload[0];

//	printf("Payload : \r\n");
//	for( int i=0;i<5;i++)
//	{
//		printf("%d ",pPayloadPtcl->pPayload[i]);
//	}
//	printf("\r\nucInput = %d",ucInput);

	if( ucInput == DCS_DEF_APN || ucInput == DCS_GIT_APN )
	{
		g_FirmwareInfo.m_ucApnFlag = ucInput;
	}
	else
	{
		g_FirmwareInfo.m_ucApnFlag = DCS_DEF_APN;
	}
	SetFirmwareInfo(&g_FirmwareInfo);

	ModemReset_Install();

    cResult[0] = 1;
    cResult[1] = g_FirmwareInfo.m_ucApnFlag;
	g_bAPNFlag = true;

    SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&cResult, sizeof(cResult), (eCommType)eInCommType);
}
/////////////////////////////////생산 프로그램 개발중 /////////////////////////////////////////////////


#define FILENAME_VCI2_SERIAL_DAT	"VCI2Serial.dat"

void GitReadSerial(void *pInterPtcl, unsigned int eInCommType)
{
//	if(eInCommType==eCOMM_TYPE_UART_PLUSBD)
//	{
////		stGIT_PTCL_PAYLOAD *pPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
////
////		GITDebugPrintf("1303 run\r\n");
////		memcpy(arrPlusModuleSerialNumber, pPayloadPtcl->pPayload, sizeof(arrPlusModuleSerialNumber));
////		g_ExtendBoardAckFlag=2;
////		g_ucPlusModuleCommTest=1;	//셀프테스트용
//	}
	/*else*/    if(eInCommType==eCOMM_TYPE_UART_BT)
	{
		unsigned char arrSerialNumber[20];
		unsigned int iPayLoadDataLen=8;

		memset(arrSerialNumber, 0x00, iPayLoadDataLen);

		git_f_chdir(DIR_ROOT);
    /*
		U8 i;
		UINT iLength = 8;

		if(GetVCI2FileRead(FILENAME_VCI2_SERIAL_DAT, arrSerialNumber, &iLength)==FALSE)
		{
			for(i=0; i<8; i++)
				arrSerialNumber[i]=0x58; //시리얼정보가 없을시 'X'로 송신한다.
		}
    */

#if defined(DEBUG_GIT_PTCL_LOG)
		GITDebugPrintf("[%s] %c%c%c%c%c%c%c%c \r\n", __FUNCTION__,
			arrSerialNumber[0], arrSerialNumber[1], arrSerialNumber[2], arrSerialNumber[3],
		arrSerialNumber[4], arrSerialNumber[5], arrSerialNumber[6], arrSerialNumber[7]);
#endif
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, g_FirmwareInfo.arrSerialNumber, iPayLoadDataLen, (eCommType)eInCommType);
	}
}
void GitReadHiddenSerial(void *pInterPtcl, unsigned int eInCommType)
{
	if(eInCommType==eCOMM_TYPE_UART_BT)
	{
		unsigned char arrSerialNumber[7];
		unsigned int iPayLoadDataLen=7;

		memset(arrSerialNumber, 0x00, sizeof(arrSerialNumber));
		
#if defined(QA_FIFA)
		if(memcmp(&g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO], "QAH", 3) != 0)
		{
			memcpy(&g_FirmwareInfo.arrSerialNumber[IDX_CONTRY_CODE_SERIAL_NO],"QAH",3);
			SetFWSerialNumber(g_FirmwareInfo.arrSerialNumber);
		}
#endif

		memcpy(arrSerialNumber,&g_FirmwareInfo.arrSerialNumber[8],iPayLoadDataLen);

#if defined(DEBUG_GIT_PTCL_LOG)
		GITDebugPrintf("Hidden Serial: ");
		GITDebugPrintf("%c%c%c%c%c%c%c \r\n",arrSerialNumber[0],arrSerialNumber[1],arrSerialNumber[2],arrSerialNumber[3],arrSerialNumber[4],arrSerialNumber[5],arrSerialNumber[6]);
#endif
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pInterPtcl, arrSerialNumber, iPayLoadDataLen, (eCommType)eInCommType);
	}
}

void Git_FWModeChange(void *pInterPtcl, unsigned int eInCommType)
{
	stGIT_PTCL_PAYLOAD *pPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;
	unsigned int iPayLoadDataLen=1;
	BYTE cSwitchAppNumber;

#if defined(DEBUG_GIT_PTCL_LOG)
	GITDebugPrintf("[%s] run, change mode 0x%02X\r\n", __FUNCTION__, pPayloadPtcl->pPayload[0]);
#endif

	cSwitchAppNumber = pPayloadPtcl->pPayload[0];

	SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, &cSwitchAppNumber, iPayLoadDataLen, (eCommType)eInCommType);

//	RunModeSwitch(cSwitchAppNumber);
}

void GitPassThruDisconnect(void *pInterPtcl, unsigned int eInCommType)
{
//#if defined(DEBUG_GIT_PTCL_LOG)
//	GITDebugPrintf("[%s] run\r\n", __FUNCTION__);
//#endif
//	VCI_REINTI_COMM_STATE(VCI_GetPassThruProtocolID());
//	VCI_Clear_DLC_HW();
}

#if defined(PROTOCOL18)
void Send2BTOTCResPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stOTC_PTCL_PAYLOAD *pstOTCPayload, unsigned char ucControlID, unsigned int eInCommType)
{
	pstOTCPayload->ucIndex = pPayloadPtcl->pPayload[BT_OTC_REQ_INDEX];
 	memcpy((stOTC_PTCL_PAYLOAD *)pstOTCPayload->usCommand,(stBT_PTCL_PAYLOAD*)(pPayloadPtcl->pPayload[BT_COMMAND_ID]), sizeof(pstOTCPayload->usCommand));
 	pstOTCPayload->ucControl = ucControlID;
 	pstOTCPayload->ucResult = BT_OTC_RESP_SUCCESS;
	pstOTCPayload->ucEngineStatus = Get_VehicleStatus();
	pstOTCPayload->usVehicleStatus = Get_OTCVehicleStatus();

	if(BTGetConnectStatus()==true)
	{
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (unsigned char*)pstOTCPayload, sizeof(stOTC_PTCL_PAYLOAD), (eCommType)eInCommType);
	}
}

void Send2BTRemoteCtrlResPayload(stBT_PTCL_PAYLOAD *pPayloadPtcl, stRemote_PTCL_PAYLOAD *pstRemoteCtrlPayload, unsigned short usCommandID, unsigned int eInCommType)
{
	pstRemoteCtrlPayload->ucIndex = pPayloadPtcl->pPayload[BT_CTRL_REQ_INDEX];
	pstRemoteCtrlPayload->usCommand = usCommandID;
	pstRemoteCtrlPayload->ucControl = pPayloadPtcl->pPayload[BT_CTRL_REQ_CONTROL];
	pstRemoteCtrlPayload->ucEngineStatus = Get_VehicleStatus();
	pstRemoteCtrlPayload->usVehicleStatus = Get_OTCVehicleStatus();
	memcpy(pstRemoteCtrlPayload->ucBTControlKey, &(pPayloadPtcl->pPayload[BT_CTRL_REQ_BT_CTRL_KEY]), sizeof(g_ucBTControlKey));

	if((usCommandID&BT_CTRL_COMMAND_TYPE_CHCK) == 0)
	{
		pstRemoteCtrlPayload->ucResult = BT_REMOTE_CTRL_SUCCESS;
	}

	if(BTGetConnectStatus()==true)
	{
		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)pPayloadPtcl, (unsigned char*)pstRemoteCtrlPayload, sizeof(stRemote_PTCL_PAYLOAD), (eCommType)eInCommType);
	}
}

unsigned short int Get_OTCVehicleStatus(void)
{
    unsigned short int g_usOTCVehicleStatus=0;

	if(Get_DoorLock() & 0x01)
	{
	    g_usOTCVehicleStatus |= (1<<FL_DOOR_ULOCK);
	}
	if(Get_DoorLock() & 0x02)
	{
		g_usOTCVehicleStatus |= (1<<FR_DOOR_ULOCK);
	 	//Front-right Door unlock
	}
	if(Get_DoorLock() & 0x04)
	{
		g_usOTCVehicleStatus |= (1<<RL_DOOR_ULOCK);
	   //Rear- Left Door unLock
	}
	if(Get_DoorLock() & 0x08)
	{
		g_usOTCVehicleStatus |= (1<<RR_DOOR_ULOCK);
		//Rear-Right Door unLock
	}
	if(Get_HeadLamp_State())
	{
		g_usOTCVehicleStatus |= (1<<HEAD_LIGHT);
	}
	if(Get_EngHazardLampStatus())
	{
		g_usOTCVehicleStatus |= (1<<HAZARD_LAMP );
	}
	if(Get_HighBeamStatus())
	{
		g_usOTCVehicleStatus |= (1<<HIGH_BEAM);
	}
	if(Get_FootBrake_State())
	{
		g_usOTCVehicleStatus |= (1<<BRAKE);
	}

	return g_usOTCVehicleStatus;
}
#endif
void ManageReserveInfo()
{
	U8 arrStartTimeConvert[6];
	U8 arrEndTimeConvert[6];
        unsigned char arrRealTime[15];
	unsigned char arrRealTimeConvert[6];
	U8 arrStartTime[RESERVED_TIMEINFO_SIZE + 1];
	U8 arrEndTime[RESERVED_TIMEINFO_SIZE + 1];
//	U32 uiStartTime, uiEndTime, uiNowTime;

	for(int i=0; i< RESERVED_INDEX_MAX; i++)
	{
		if(g_stReservInfo[i].ucUsed == TRUE)
		{
			VCI_GetRtcTime(arrRealTime);
			memcpy(arrStartTime, &g_stReservInfo[i].ucStartTime[2], sizeof(arrStartTime)-2);
			memcpy(arrEndTime, &g_stReservInfo[i].ucEndTime[2], sizeof(arrEndTime)-2);
			arrStartTime[RESERVED_TIMEINFO_SIZE]='\0';
			arrEndTime[RESERVED_TIMEINFO_SIZE]='\0';

			memcpy(arrRealTimeConvert,&arrRealTime[1],sizeof(arrRealTimeConvert));

			arrStartTime[7] = arrStartTime[7] -1; 	//대여시간 1시간전부터 차량도어 제어 가능
			//arrEndTime[7]  = arrEndTime[7] ; 	//대여시간 1시간전부터 차량도어 제어 가능

			fnHex2Str((char *)&arrStartTimeConvert[0],(char *)&arrStartTime[0]);
			fnHex2Str((char *)&arrEndTimeConvert[0],(char *)&arrEndTime[0]);

			for(int j=0;j<5;j++)
			{
				arrStartTimeConvert[j]=HexToDec(arrStartTimeConvert[j]);
				arrEndTimeConvert[j]=HexToDec(arrEndTimeConvert[j]);
			}

			arrStartTimeConvert[5]=0x00;
			arrEndTimeConvert[5]=0x00;
                        arrRealTimeConvert[5]=0x00;


			if(strcmp((char *)arrRealTimeConvert,(const char *)arrEndTimeConvert) > 0)
			{
				g_stReservInfo[i].ucUsed = FALSE;
			}
		}
	}
}



//bool ValidReserveTime(char* ucEndTime)
//{
//	U8 arrStartTimeConvert[6];
//	U8 arrEndTimeConvert[6];
//	unsigned char arrRealTime[15];
//	unsigned char arrRealTimeConvert[6];
//	U8 arrStartTime[RESERVED_TIMEINFO_SIZE + 1];
//	U8 arrEndTime[RESERVED_TIMEINFO_SIZE + 1];
////	U32 uiStartTime, uiEndTime, uiNowTime;
//	bool bResult = TRUE;
//
//	//for(int i=0; i< RESERVED_INDEX_MAX; i++)
//	{
//		//if(g_stReservInfo[i].ucUsed == TRUE)
//		{
//			VCI_GetRtcTime(arrRealTime);
//			//memcpy(arrStartTime, &g_stReservInfo[i].ucStartTime[2], sizeof(arrStartTime)-2);
//			memcpy(arrEndTime, &ucEndTime[2], sizeof(arrEndTime)-2);
//			//arrStartTime[RESERVED_TIMEINFO_SIZE]='\0';
//			arrEndTime[RESERVED_TIMEINFO_SIZE]='\0';
//
//			memcpy(arrRealTimeConvert,&arrRealTime[1],sizeof(arrRealTimeConvert));
//
//			//GetConvertFrontTime((char*)&arrStartTime,1);
//			//arrStartTime[7] = arrStartTime[7] -1; 	//대여시간 1시간전부터 차량도어 제어 가능
//
//			//fnHex2Str((char *)&arrStartTimeConvert[0],(char *)&arrStartTime[0]);
//			fnHex2Str((char *)&arrEndTimeConvert[0],(char *)&arrEndTime[0]);
//			for(int j=0;j<5;j++)
//			{
//			//arrStartTimeConvert[j]=HexToDec(arrStartTimeConvert[j]);
//			arrEndTimeConvert[j]=HexToDec(arrEndTimeConvert[j]);
//			}
//			//arrStartTimeConvert[5]=0x00;
//			arrEndTimeConvert[5]=0x00;
//			arrRealTimeConvert[5]=0x00;
//
//
//			if(strcmp((char *)arrRealTimeConvert,(const char *)arrEndTimeConvert) <= 0)
//			{
//				bResult = FALSE;
//			}
//		}
//	}
//
//	return bResult;
//}

void GetConvertFrontTime(U8 *pTimeBuffer, int nTime)
{
	U8 ucBufferTime[12];
	U8 ucHour;
	U8 ucDay;
	U8 ucMonth;
	U8 ucYear;
	U8 ucCheckYear;


	memcpy(&ucBufferTime,pTimeBuffer,sizeof(ucBufferTime));
	/*
	ucYear  = HexToDec(ucBufferTime[0])*10 + HexToDec(ucBufferTime[1]);
	ucMonth = HexToDec(ucBufferTime[2])*10 + HexToDec(ucBufferTime[3]);
	ucDay   = HexToDec(ucBufferTime[4])*10 + HexToDec(ucBufferTime[5]);
	ucHour 	= HexToDec(ucBufferTime[6])*10 + HexToDec(ucBufferTime[7]);
	*/

	ucYear  = (ucBufferTime[0]-0x30)*10 + (ucBufferTime[1]-0x30);
	ucMonth = (ucBufferTime[2]-0x30)*10 + (ucBufferTime[3]-0x30);
	ucDay   = (ucBufferTime[4]-0x30)*10 + (ucBufferTime[5]-0x30);
	ucHour 	= (ucBufferTime[6]-0x30)*10 + (ucBufferTime[7]-0x30);


	if(ucHour == 0)
	{
		ucHour = 24 - nTime;

		if(ucDay == 1)
		{
			if(ucMonth == 1)
			{
				ucYear = ucYear-1;
				ucMonth = 12;
			}
			else
			{
				ucMonth = ucMonth -1;
			}

			if(ucMonth == 1 || ucMonth == 3 || ucMonth == 5 || ucMonth == 7 || ucMonth == 8 || ucMonth == 10 || ucMonth == 12)
				ucDay = 31;
			else if(ucMonth == 2)
			{
				ucCheckYear =  2000 + ucYear;

				if((ucCheckYear%4 == 0 ) && (ucCheckYear%400 == 0 ) && (ucCheckYear%100 != 0))
					ucDay = 29;
				else
					ucDay = 28;
			}
			else
				ucDay = 30;
		}
		else
		{
			ucDay = ucDay - 1;
		}
	}
	else
	{
		ucHour = ucHour -1;
	}

	sprintf((char *)ucBufferTime,"%x%x%x%x%x%x%x%x",ucYear/10,ucYear%10,ucMonth/10,ucMonth%10,ucDay/10,ucDay%10,ucHour/10,ucHour%10);
        /*
	ucBufferTime[0] = ucYear/10+0x30;
	ucBufferTime[1] = ucYear%10+0x30;

	ucBufferTime[2] = ucMonth/10+0x30;
	ucBufferTime[3] = ucMonth%10+0x30;

	ucBufferTime[4] = ucDay/10+0x30;
	ucBufferTime[5] = ucDay%10+0x30;

	ucBufferTime[6] = ucHour/10+0x30;
	ucBufferTime[7] = ucHour%10+0x30;
        */


	//sprintf(ucBufferTime,"%02x%02x%02x%02x",ucYear,ucMonth,ucDay,ucHour);

	memcpy(pTimeBuffer,&ucBufferTime,8);

}

bool ExistSameReserveID(char *pTempPayLoad)
{
	bool bResult = FALSE;
	U8 ucIDCommand[3];

	for(int i=0; i< RESERVED_INDEX_MAX; i++)
	{
		if(g_stReservInfo[i].ucUsed == TRUE)
		{
			if(memcmp(pTempPayLoad,  g_stReservInfo[i].arrIndex, 16)==0 && memcmp(&pTempPayLoad[16],  g_stReservInfo[i].arrUserID, 3)==0)
			{
				bResult = TRUE;
			}
		}
	}

	memcpy(&ucIDCommand,&pTempPayLoad[16],sizeof(ucIDCommand));

	for(int i = 0 ; i <3;i++)
    {
		if(ucIDCommand[i] >= 'a' && ucIDCommand[i] <= 'z')
		{
			ucIDCommand[i] -= 32;
		}
    }

	if(strncmp((char const*)ucIDCommand,"VPT",3) ==0 || strncmp((char const*)ucIDCommand,"MBR",3) ==0 || strncmp((char const*)ucIDCommand,"MGR",3) ==0)
	{

	}
	else
	{
		bResult = TRUE;
	}

	return bResult;
}

//void CB_TimeOutCallback_BT(void)
//{
//	unsigned char ucRet=0;
//	static unsigned int s_LoopCnt=0;
//	stBT_PTCL_PAYLOAD stOutPtcl;
//
//	if(GetExtendBoardState() == eEXTENDBOARD_RUNNING)
//	{
//		switch(m_ucPayloadDump_BT[0])
//		{
//			case 0x14:	//비상등제어
//				//g_ucDoorCheckFlag=0xFF;
//				if(m_ucPayloadDump_BT[1]==0x01)				LampOn(m_ucPayloadDump_BT[2]);
//				else if(m_ucPayloadDump_BT[1]==0x02)		LampOFF();
//				else															ucRet=1;
//				//else							ExtendBoardAllCtrlOff();
//				break;
//			case 0x15:	//도어제어
//				//g_ucDoorCheckFlag=m_ucPayloadDump_BT[1];
//				if(m_ucPayloadDump_BT[1]==0x00)				LockUnlockSet(TRUNK_OPEN);		//0x02
//				else if(m_ucPayloadDump_BT[1]==0x01)		LockUnlockSet(DOOR_LOCK);			//0x00
//				else if(m_ucPayloadDump_BT[1]==0x02)		LockUnlockSet(DOOR_UNLOCK);		//0x01
//				else															ucRet=1;
//				//else							ExtendBoardAllCtrlOff();
//				break;
//			case 0x16:	//경적
//				//g_ucDoorCheckFlag=0xFF;
//				if(m_ucPayloadDump_BT[1]==0x01)				HornOn(HORN_TIME,0);
//				else															ucRet=1;
//				//else							ExtendBoardAllCtrlOff();
//				break;
//			case 0x17:	//트렁크
//				//g_ucDoorCheckFlag=0xFF;
//				if(m_ucPayloadDump_BT[1]==0x00)				LockUnlockTrunkSet(TRUNK_CLOSE);			//0x00
//				else if(m_ucPayloadDump_BT[1]==0x01)		LockUnlockTrunkSet(TRUNK_OPEN);			//0x01
//				else															ucRet=1;
//				break;
//			default:break;
//		}
//		HalTimerStopSWTimer(g_iTimerPlusCheckTimeOutCallback_BT);
//
//		memset(&stOutPtcl,0x00,sizeof(stOutPtcl));
//		stOutPtcl.FunctionID=g_SaveFunctionID;
//		stOutPtcl.DataLength=sizeof(ucRet);
//		stOutPtcl.pPayload=&ucRet;
//
//		SendBTPtclResponse((stBT_PTCL_PAYLOAD*)&stOutPtcl, &ucRet, 1, eCOMM_TYPE_UART_BT);
//	}
//	else
//	{
//		s_LoopCnt++;
//		printf("Repeat CB_TimeOutCallback_BT %d\r\n",s_LoopCnt);
//		if(s_LoopCnt>=100)
//		{
//			HalTimerStopSWTimer(g_iTimerPlusCheckTimeOutCallback);	//5초면 정지
//			HalTimerClearSWTimer(g_iTimerPlusCheckTimeOutCallback);
//			ucVehicleControlBlock = FALSE;
//			s_LoopCnt=0;
//		}
//	}
//}

void ConvertIntArrToHexPayload(int nSize ,uint32_t nLength ,char *Buffer ,int nStartPos)
{
	//리틀앤디안
    uint8_t cBuffer[40] = {0,};
    int TempData =  0;
	uint8_t arrHexData[10];
	uint8_t arrTempHexData[10];

	memset(&cBuffer, 0x00,sizeof(cBuffer));

	for(int i = 0 ; i<nSize;i++)
	{
    TempData = (uint8_t)((nLength>>((nSize-1-i)*8))&0xFF);
    sprintf((char*)&cBuffer[i*2],"%02x",TempData);
	}

	//전송 데이터에 대한 처리
	fnHex2Str((char *)&arrHexData[0], (char *)&cBuffer[0]);

	for(int i = 0 ; i<7;i++)
	{
		arrTempHexData[6-i]=arrHexData[i];
	}

	memcpy(&Buffer[nStartPos], arrTempHexData, nSize);
}

void ConvertCharArrayToHex(int nLength ,char *nLetter ,char *Buffer ,int nStartPos )
{
	for(int i = 0 ; i < nLength ; i++)
	{
		sprintf(&Buffer[nStartPos+i*2], "%02x", nLetter[i]);
	}
}

void ConvertIntArrToHex(int nSize ,uint32_t nLength ,char *Buffer ,int nStartPos)
{
	//리틀앤디안
    uint8_t cBuffer[40] = {0,};
  int TempData =  0;

	memset(&cBuffer, 0x00,sizeof(cBuffer));

	for(int i = 0 ; i<nSize;i++)
	{
    TempData = (uint8_t)((nLength>>(i*8))&0xFF);
    sprintf((char*)&cBuffer[i*2],"%02x",TempData);
	}

	memcpy(&Buffer[nStartPos], cBuffer, nSize*2);
}

void ConvertIntToHex(int nSize ,uint8_t nLength ,char *Buffer ,int nStartPos)
{
	sprintf(&Buffer[nStartPos],"%02x",nLength);
}

void ConvertCharToHex(uint8_t nLetter ,char *Buffer,int nStartPos )
{
	sprintf(&Buffer[nStartPos], "%02x", nLetter);
}

long getDecimal(char* hex)
{
	long rtnVal;
	char end[4];
	char *pEnd = end;

	rtnVal = strtol(hex, &pEnd, 16);

	return rtnVal;
}

void fnHex2Str(char* out, char* in)
{
	int idx;
	int outIdx = 0;
	char hex[3];
	int len = strlen(in);

	for(idx=0; idx<len; idx+=2){
		strncpy(hex, in+idx, 2);
		out[outIdx++] = getDecimal(hex);
	}
}
//******************************************************************************
// Dev Selftest
//******************************************************************************
/////////////////////////////////////////////////////Git_MainBoard_Test/////////////////////////////////////////////////////////////

#define AUTOLINK_P_SELFTEST_LOG

//#define TEST_CCID
//******************************************************************************
//	GPIO : GPIO_WAKEUP (PA0)			GPIO_WAKEUP
//	GPIO : LOW_HIGH_CAN_RX (PB5)		GPIO_LOW_HIGH_CAN_RX
//	GPIO : CAN_RX_MON(PB 15) =CAN_RX	GPIO_CAN_RX_MON
//	GPIO : BT_MON (PD10)				GPIO_BT_STATUS
//	GPIO : SENSOR_INT (PC5)				GPIO_SENSOR_INT
//	GPIO : MO_WAKE (PC3)				GPIO_MO_WAKE
//	GPIO : IG_ON_DET (PB1)				GPIO_IG_ON_DET
//	GPIO : ACC_DET (PB10)				GPIO_ACC_DET

//	eWAKE_PIN_IG_ON = 0,
//	eWAKE_PIN_ACC,
//	eWAKE_PIN_MODEM,
//	eWAKE_PIN_SENSOR,
//	eWAKE_PIN_LOW_HIGH_CAN_RX,
//	eWAKE_PIN_BT_MON,
//	eWAKE_PIN_CAN_RX,
//******************************************************************************

void Git_MainBoard_Test(void *pInterPtcl, unsigned int eInCommType)
{
//  Serial IPEK 적용으로 들어오는 데이터 늘음 -> 버퍼 size늘림
//	unsigned char ucRet[100];
//	unsigned char ucData[100];
//	unsigned char ucTempBuff[512];

	unsigned char ucRet[512];
	unsigned char ucData[1024];

	unsigned int uiStartMask[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiEndMask[HAL_CAN_MAX_MASK_CNT];

	unsigned short ulDataLen = 3;

	unsigned int CANCH = 0;
	unsigned int uiOldTimer = 0;
	unsigned int uiDeltatime = 0;
	unsigned int uiRecvLen = 0;

	unsigned int	i = 0;


	stGIT_PTCL_PAYLOAD *pPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;

	memset(ucData,0x00,sizeof(ucData));
	memset(ucRet,0x00,sizeof(ucRet));
	memcpy(ucData, pPayloadPtcl->pPayload, pPayloadPtcl->DataLength-8);

	ucRet[0] = ucData[0];
	ucRet[1] = ucData[1];

	printf("Data[0] : %02X \r\n",ucData[0]);
	printf("Data[1] : %02X \r\n",ucData[1]);


	switch(ucData[0])
	{
		case eSELFTEST_ADC_IDX:													// 0x01
			{
			unsigned short ulBattVolt=0;

			ulBattVolt=Oem_GetBattVoltage(2);

			printf("ADC(mV): %d \r\n", ulBattVolt);
			memcpy(&ucRet[2],&ulBattVolt,sizeof(ulBattVolt));
			ulDataLen = 4;
			}
			break;

		case eSELFTEST_CURRENT_IDX:												// 0x02
			// MAX or 일반 Module 설정 완료 후 응답 -> PC프로그램에서 Power Supply 전류값 측정
			g_bYUJINSelftestFlag = true;
			SetOBDState(eOBD_Selftest);
			g_arrYUJINTestIdx[0] = eSELFTEST_CURRENT_IDX;
			if(ucData[1]==MODEM_POWER_MAX)
			{
				g_arrYUJINTestIdx[1] = ucData[1];

				SetModemControlEvent(eMngMdmSleep);

				ModemManagerData.nMdmSubState = 11;
				ModemManagerData.nMdmSubNextState = 0;
				if(SendModemCommand(SelftestProcessSetFlightMode)){
					printf("\r\n MODEM FLGHT MODE SETTING OK!! \r\n");
					if(SendModemCommand(SelftestProcessInitializeModem)){
						printf("\r\n MODEM INIT OK!! \r\n");
						ucRet[2]=SELFTEST_PASS;
					}
					else{
						printf("\r\n MODEM INIT FAIL!! \r\n");
						ucRet[2]=SELFTEST_FAIL;
					}
				}
				else
				{
						printf("\r\n MODEM RESET FAIL!! \r\n");
						ucRet[2]=SELFTEST_FAIL;
				}

				ModemManagerData.nMdmSubState = 0;
				ModemManagerData.nMdmSubNextState = 0;

				// AT^SCFG="MEopMode/CT","1","","127","9750" 명령에서 reset 또는 restart걸리면 명령 정상적으로 인식하지 못함
				if(SendModemCommand(SelftestProcessSetModemPower)){
					ucRet[2]=SELFTEST_PASS;
					printf("\r\n MODEM POWER SAVE MODE SETTING OK!! \r\n");
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n MODEM POWER SAVE MODE SETTING FAIL!! \r\n");
				}

				ulDataLen = 3;

				printf(" modem power max : %d \r\n",ucRet[2]);
				SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucRet, ulDataLen, (eCommType)eInCommType);
#ifdef USEBUZZER
				stHalGPIO_InitTypeDef GPIO_InitStructure;
                GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
				GPIO_InitStructure.GPIO_Pin = GPIO_BUZZER;
				GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_AF;
				GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
				GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
				GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
                HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

				GPIO_PinAFConfig(GPIOA, BUZZER_GPIO_SOURCE, GPIO_AF_TIM14);

				SetLedOnOffCtl(LED_ON, eLED_ALL);
				Buzzer_On();
				BzFrDuty_Set(4000,50);
				Oem_GIT_mDelay(SELFTEST_BUZZER_DELAY);								// 4초 (PC프로그램 요청)
				Buzzer_Off();
#endif
			}
			else if(ucData[1]==MODEM_POWER_NORMAL)
			{
				g_arrYUJINTestIdx[1] = ucData[1];
#ifdef RF_COMMON_MODEM
				ModemManagerData.nMdmSubState = 3; //mod.kks 21.12.22 to modify the sequence.
#else
				ModemManagerData.nMdmSubState = 2;
#endif
				ModemManagerData.nMdmSubNextState = 0;

				if(SendModemCommand(SelftestProcessSetModemPower)){
					ucRet[2]=SELFTEST_PASS;
					printf("\r\n SCFG \"0\" MODE SETTING OK!! \r\n");
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n SCFG \"0\" MODE SETTING FAIL!! \r\n");
				}
				Oem_GIT_mDelay(500);

				ModemManagerData.nMdmSubState = 3;
				ModemManagerData.nMdmSubNextState = 0;

				if(SendModemCommand(SelftestProcessSetFlightMode)){
					printf("\r\n MODEM FLGHT MODE SETTING OK!! \r\n");
					if(SendModemCommand(SelftestProcessInitializeModem)){
						printf("\r\n MODEM INIT OK!! \r\n");
						ucRet[2]=SELFTEST_PASS;
					}
					else{
						printf("\r\n MODEM INIT FAIL!! \r\n");
						ucRet[2]=SELFTEST_FAIL;
					}
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n MODEM FLGHT MODE SETTING FAIL!! \r\n");
				}
				ulDataLen = 3;

				printf(" modem power normal : %d \r\n",ucRet[2]);
				SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucRet, ulDataLen, (eCommType)eInCommType);
			}
			break;


		case eSELFTEST_BUZZ_IDX:												// 0x03
			{
				ucRet[1] = SELFTEST_PASS;
				ulDataLen = 2;
				SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucRet, ulDataLen, (eCommType)eInCommType);
#ifdef USEBUZZER
				stHalGPIO_InitTypeDef GPIO_InitStructure;
                GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
				GPIO_InitStructure.GPIO_Pin = GPIO_BUZZER;
				GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_AF;
				GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
				GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
				GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
                HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

				GPIO_PinAFConfig(GPIOA, BUZZER_GPIO_SOURCE, GPIO_AF_TIM14);

				Buzzer_On();
				BzFrDuty_Set(4000,50);
				Oem_GIT_mDelay(SELFTEST_BUZZER_DELAY);								// 4초 (PC프로그램 요청)
				Buzzer_Off();
#endif
			}
			break;

		case eSELFTEST_SENSOR_IDX:												// 0x04
            HalDrvI2COpen(0,0,NULL,0,0);

			if(ucData[1]==SENSOR_ALL)			//전체
			{
				if((MPU6515_readByte(MPU6515_ADDRESS, WHO_AM_I_MPU6515) == 0x74)&&(ALPU_Normal_Check()==1))
				{	ucRet[2]=SELFTEST_PASS;		}
				else
				{	ucRet[2]=SELFTEST_FAIL;		}
			}
			else if(ucData[1]==SENSOR_MPU6515)	//자이로가속도
			{
				if(MPU6515_readByte(MPU6515_ADDRESS, WHO_AM_I_MPU6515) == 0x74)		ucRet[2]=SELFTEST_PASS;
				else																ucRet[2]=SELFTEST_FAIL;
			}
			else if(ucData[1]==SENSOR_ALPU)		//ALPU
			{
				if(ALPU_Normal_Check()==1)			ucRet[2]=SELFTEST_PASS;
				else								ucRet[2]=SELFTEST_FAIL;
			}
			else
			{	ucRet[2]=SELFTEST_FAIL;		}
			ulDataLen = 3;

			break;

		case eSELFTEST_CAN_IDX:													// 0x05
			{
			unsigned int CANCH = 0;
			unsigned int nLoopCnt = 0;
			unsigned int uiStartID,uiEndID;


			stCanPacket OutCanPacket, InCanPacket;

			printf("Data[1] : %02X \r\n",ucData[1]);
			printf("Data[2] : %02X \r\n",ucData[2]);

			SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX, eWAKE_CTL_PIN_ENABLE);
			SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX, eWAKE_CTL_PIN_ENABLE);

			Oem_CAN_None_Receive_Set();

			if(ucData[1] == CAN_HIGH1) {
				printf("CAN: Enable HIGH CAN1\n");

				Oem_CAN_Initial_CH1(Highcan1, eCAN_500KBPS); //init
				CANCH = CAN_CHANNEL_1;
			}
			else if(ucData[1] == CAN_HIGH2) {
				printf("CAN: Enable HIGH CAN2\n");

				Oem_CAN_Initial_CH1(Highcan2, eCAN_500KBPS); //init
				CANCH = CAN_CHANNEL_1;
			}
			else if(ucData[1] == CAN_HIGH3) {
				printf("CAN: Enable HIGH CAN3\r\n");

				Oem_CAN_Initial_CH2(Highcan3, eCAN_500KBPS); //init
				CANCH = CAN_CHANNEL_2;
			}
			else if(ucData[1] == CAN_LOW1) {
				printf("CAN: Enable LOW CAN\r\n");

				Oem_CAN_Initial_CH2(Lowcan1, eCAN_100KBPS); //init
				CANCH = CAN_CHANNEL_2;
			}

			uiStartID=(ucData[2]<<8)|ucData[3];
			uiEndID=(ucData[12]<<8)|ucData[13];

			OutCanPacket.stNormalPacket.ucSOF 		= CAN_FRAME_SOF;
			OutCanPacket.stNormalPacket.us11BitID	= uiStartID;
			OutCanPacket.stNormalPacket.ucRTR		= 0;
			OutCanPacket.stNormalPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
			OutCanPacket.stNormalPacket.ucReserved	= 0;
			OutCanPacket.stNormalPacket.ucDLC		= CAN_FRAME_DATA_SIZE;
			OutCanPacket.stNormalPacket.usCRC		= 0;
			OutCanPacket.stNormalPacket.ucCRCDelimiter = 1;
			OutCanPacket.stNormalPacket.ucACK		= 1;
			OutCanPacket.stNormalPacket.ucACKDelimiter = 1;
			OutCanPacket.stNormalPacket.ucEOF		= CAN_FRAME_EOF;

			memcpy(&OutCanPacket.stNormalPacket.arrDataFields[0],&ucData[4],8);

			Oem_CAN_Channel_Masket_Set(CANCH, STANDARD_CAN, 1, &uiStartID, &uiEndID);
			if ( FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, CANCH) > 0 )
			{
				nLoopCnt = 0;
				uiRecvLen = 0;
				do {
					uiRecvLen = FineReadCanBuff((unsigned char*)&InCanPacket, sizeof(InCanPacket), NULL, CANCH);

					if ( nLoopCnt++ > 3 )
					{
						GITDebugPrintf("Receive FAil ~~~~~~~~~~~~~~~~~~\r\n");
						ucRet[2]=SELFTEST_FAIL;
						break;
					}
					else
					{
						if( uiRecvLen > 0 )
						{
							GITDebugPrintf("Receive success ~~~~~~~~~~~~~~~~~~\r\n");
							ucRet[2]=SELFTEST_PASS;
						}
						else
							Oem_GIT_mDelay(30);
					}
				}while(uiRecvLen == 0);
			}
			else{
				ucRet[2]=SELFTEST_FAIL;
			}
			}

			ulDataLen = 3;
			break;

		case eSELFTEST_MODEM_UART_IDX:											// 0x07

			ModemManagerData.nMdmSubState = 0;
			ModemManagerData.nMdmSubNextState = 0;

			if(SendModemCommand(SelftestProcessGetModemInfo)){
				ucRet[2]=SELFTEST_PASS;
				printf("\r\n MODEM INFO GET OK!! \r\n");
			}
			else{
				ucRet[2]=SELFTEST_FAIL;
				printf("\r\n MODEM INFO GET FAIL!! \r\n");
			}
			ulDataLen = 2;

			break;

		case eSELFTEST_MODEM_GPS_IDX:											// 0x09
			// 180212
			// 하드웨어 팀에서 제공한 GPS 지그에서 PDOP, HDOP, VDOP, inuse, in_use 다 안나옴
			// sat.id와 sig만 나옴

//			if( g_GPSInfo.satinfo.inuse == 0){
//				printf("GPS sattellite use : 0\r\n");
//				ucRet[1]=SELFTEST_FAIL;
//				ulDataLen = 2;
//			}
//			else
			{

//			printf(" GPS Structure View\r\n");
//			printf("g_GPSInfo: smask %d, sig %d, fix %d\n PDOP %f, HDOP %f, VDOP %f\n lat %f, lon %f, speed %f\n",
//				   g_GPSInfo.smask,g_GPSInfo.sig,g_GPSInfo.fix,g_GPSInfo.PDOP,g_GPSInfo.HDOP,g_GPSInfo.VDOP,g_GPSInfo.lat,g_GPSInfo.lon,g_GPSInfo.speed);
				unsigned int uiSatSigMax=0;
				unsigned int uiSatSigMin=0;
				unsigned char ucSatCnt=0;

				for(U8 i=0; i<NMEA_MAXSAT; i++){
					//if(g_GPSInfo.satinfo.sat[i].in_use == 1){
					if((g_GPSInfo.satinfo.sat[i].id != 0)&&(g_GPSInfo.satinfo.sat[i].sig != 0)){
						ucSatCnt++;
						printf("sat id %d, sat use %d, sat sig %d\n",
						   g_GPSInfo.satinfo.sat[i].id,g_GPSInfo.satinfo.sat[i].in_use,g_GPSInfo.satinfo.sat[i].sig);

						if((uiSatSigMax==0)&&(uiSatSigMin==0)){
							uiSatSigMax = g_GPSInfo.satinfo.sat[i].sig;
							uiSatSigMin = g_GPSInfo.satinfo.sat[i].sig;
						}
						if(uiSatSigMax<g_GPSInfo.satinfo.sat[i].sig)		uiSatSigMax = g_GPSInfo.satinfo.sat[i].sig;
						else if(uiSatSigMin>g_GPSInfo.satinfo.sat[i].sig)	uiSatSigMin = g_GPSInfo.satinfo.sat[i].sig;
					}
				}
				ucRet[1] = ucSatCnt;
				ucRet[2] = uiSatSigMax;
				ucRet[3] = uiSatSigMin;
				ulDataLen = 4;
			}

			break;

		case eSELFTEST_FLASH_IDX:												// 0x0A
			{
			unsigned char	ucBuff[100];
			unsigned int	uiLength=0;

			if(ucData[1]==FLASH_WRITE)
			{
				if(MakeDummyFile()==0)		ucRet[2]=SELFTEST_PASS;
				else						ucRet[2]=SELFTEST_FAIL;
			}
			else if(ucData[1]==FLASH_ERASE)
			{
				if(git_f_unlink("Dummy.bin")==0)	ucRet[2]=SELFTEST_PASS;
				else							ucRet[2]=SELFTEST_FAIL;
			}
			else if(ucData[1]==FLASH_READ)
			{
				if(GetVCI2FileRead("Dummy.bin",ucBuff,&uiLength)==TRUE)	ucRet[2]=SELFTEST_PASS;
				else													ucRet[2]=SELFTEST_FAIL;
			}
			else if(ucData[1]==FLASH_FORMAT)
			{
				printf("FS: format start\r\n");
				if ( git_f_mkfs(FAT_VOLUME_DRV, 0, 4096) == GIT_FR_OK )
				{
					printf("FS: format success\r\n");
					if ( git_f_chdir(DIR_ROOT) == GIT_FR_OK )
					{
						printf("FS: Create default foler\r\n");
						git_f_mkdir(DIR_LOG);
						git_f_mkdir(DIR_FIRMWARE);
						git_f_mkdir(DIR_INFORMATION);
						git_f_mkdir(AUTOLINKDATA_BASE_DIR);
						git_directory_list();
						ucRet[2]=SELFTEST_PASS;
					}
					else
					{
						printf("FS: GIT_FRNOK\r\n");
						ucRet[2]=SELFTEST_FAIL;
					}
				}
			}
			else{
				ucRet[2]=SELFTEST_FAIL;
			}

			}
			ulDataLen = 3;
			break;
		case eSELFTEST_SLEEP_BLE_CHIP_IDX:										// 0x0E (Micom Sleep X / BLE Sleep test)

			if(ucData[1]==BLE_SLEEP)
			{
				BTOffRepeatModeReq();
				Oem_GIT_mDelay(10);
				HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_RESET);
				ucRet[2]=SELFTEST_PASS;
				ucRet[3]=g_LocalBTInfo.bt_addr.btAddr[5];
				ucRet[4]=g_LocalBTInfo.bt_addr.btAddr[4];
				ucRet[5]=g_LocalBTInfo.bt_addr.btAddr[3];
				ucRet[6]=g_LocalBTInfo.bt_addr.btAddr[2];
				ucRet[7]=g_LocalBTInfo.bt_addr.btAddr[1];
				ucRet[8]=g_LocalBTInfo.bt_addr.btAddr[0];
				ulDataLen = 9;
			}
			else if(ucData[1]==BLE_WAKEUP)
			{
				HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_SET);
				ucRet[2]=SELFTEST_PASS;
				ucRet[3]=g_LocalBTInfo.bt_addr.btAddr[5];
				ucRet[4]=g_LocalBTInfo.bt_addr.btAddr[4];
				ucRet[5]=g_LocalBTInfo.bt_addr.btAddr[3];
				ucRet[6]=g_LocalBTInfo.bt_addr.btAddr[2];
				ucRet[7]=g_LocalBTInfo.bt_addr.btAddr[1];
				ucRet[8]=g_LocalBTInfo.bt_addr.btAddr[0];
				ulDataLen = 9;
			}
			else
			{
				ucRet[2]=SELFTEST_FAIL;
				ulDataLen = 3;
			}
			break;

		case eSELFTEST_LED_IDX:													// 0x13

			if(ucData[1] == LED_TOGGLE_ON){
				// LED ALL Blink

				g_iTimerled1000msCallback=HalTimerSetSWTimer(1000, eSWTimer_INFINITE, SetALLLedOnOff, TRUE);
				ucRet[2]=SELFTEST_PASS;
			}
			else if(LED_TOGGLE_OFF){

				HalTimerStopSWTimer(g_iTimerled1000msCallback);
				HalTimerClearSWTimer(g_iTimerled1000msCallback);
				g_iTimerled1000msCallback = -1;

				SetLedOnOffCtl(LED_ON, eLED_ALL);
				ucRet[2]=SELFTEST_PASS;
			}
			else{
				ucRet[2]=SELFTEST_FAIL;
			}
			ulDataLen = 3;

			break;

		case eSELFTEST_RTC_IDX:													// 0x15

			if(ucData[1]==RTC_SETTING)			// Setting
			{
				RTC_SetTimeRegulate(ucData[2],ucData[3],ucData[4],ucData[5],ucData[6],ucData[7]);
				ucRet[2]=SELFTEST_PASS;
				ulDataLen = 3;
			}
			else if(ucData[1]==RTC_READING)		// Reading
			{
                stHalRTCTypeDef stTimeVar;
                
                HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stTimeVar, sizeof(stHalRTCTypeDef), 0);
                
				ucRet[2]=stTimeVar.RtcDate.RTC_Year;
				ucRet[3]=stTimeVar.RtcDate.RTC_Month;
				ucRet[4]=stTimeVar.RtcDate.RTC_Date;
				ucRet[5]=stTimeVar.RtcTime.RTC_Hours;
				ucRet[6]=stTimeVar.RtcTime.RTC_Minutes;
				ucRet[7]=stTimeVar.RtcTime.RTC_Seconds;
				ulDataLen = 8;
			}
			else
			{
				ucRet[2]=SELFTEST_FAIL;
				ulDataLen = 3;
			}
			break;

		case eSELFTEST_USIM_OPERATE_IDX:										// 0x16		GetIMEI
			{
				uint8_t arrBuff[20];
				memset(arrBuff, 0x00, sizeof(arrBuff));

				if ( memcmp(ModemManagerData.aIMEI, arrBuff, sizeof(ModemManagerData.aIMEI)) == 0 ){
					ucRet[1]=SELFTEST_FAIL;
					ulDataLen = 2;
				}
				else{
					printf("** Success - IMEI !!!!!!!\r\n");
					ucRet[1]=SELFTEST_PASS;
					memcpy(&ucRet[2], ModemManagerData.aIMEI, sizeof(ModemManagerData.aIMEI));
					ulDataLen = 22;
				}
			}
			break;

		case eSELFTEST_CREATE_PUBLIC_KEY_IDX:									// 0x21
		  {
			static unsigned char arrInRandomData[32];
			static unsigned char arrInHashData[32];
			static unsigned char arrInSignData[256];

			unsigned char ucPublicEnc[512];

			if(ucData[1] == PENTA_RANDOM_DATA)		// 33byte PC -> Module
			{
				memset(arrOutRandomData,0x00,sizeof(arrOutRandomData));
				memcpy(arrInRandomData,&ucData[2],sizeof(arrInRandomData));
				//fnHex2Str(arrInRandomData,"8D9C3E626C8E540A88B76E4887F17242DF5368E8232EDBC3C39E143623D2BBD2\x00");
				Convert_Ascii_To_HexString_len((char*)arrOutRandomData,(char*)arrInRandomData,sizeof(arrInRandomData));

				ucRet[2]=SELFTEST_PASS;
				ulDataLen = 3;
			}
			else if(ucData[1] == PENTA_HASH_DATA)	// 33byte PC -> Module
			{
				memset(arrOutHashData,0x00,sizeof(arrOutHashData));
				memcpy(arrInHashData,&ucData[2],sizeof(arrInHashData));
				//fnHex2Str(arrInHashData,"FB572412B028244924CB06375536EC9A1FBC07F8B846A8E6B6B9820ED3931E86\x00");
				Convert_Ascii_To_HexString_len((char*)arrOutHashData,(char*)arrInHashData,sizeof(arrInHashData));

				ucRet[2]=SELFTEST_PASS;
				ulDataLen = 3;
			}
			else if(ucData[1] == PENTA_SIGN_DATA)	// 513byte PC -> Module
			{
				memset(arrOutSignData,0x00,sizeof(arrOutSignData));
				memcpy(arrInSignData,&ucData[2],sizeof(arrInSignData));
				//fnHex2Str(arrInSignData,"5321A0DE4B67FE1F3E0E25C8D70A0A9FABE792642D3A962482C2631D7BFE2B1D24F4B830C5CA34AC1231E6A4E4CE30A0B0CF212EE59AB0A2DCB145B5CC3A98253536292D401DF4F12401E6BEA3415E4CC3B270B01F93D91D5AAE9BAC4D2704C0228754A414531867DB8D282D97FC60B0D9E52A88A5715E6A3D4F82198EF23BBC2642D19373B85F44416231BF88DC0ADA0187A55B7EE2B70A22F35EED5957D483F1F3D5E6C7DA174A85748044585AFA3D6623C6592236DCD5A78C5103A5A076960766A6F9634D1B364CA78A49725704D60387C0C7DD7B86CB69CDC9C096540B29B3C37E399B995D41798DA29296A0C4663880D24E4A5E5B89C209AEF31E9515C5\x00");
				Convert_Ascii_To_HexString_len((char*)arrOutSignData,(char*)arrInSignData,sizeof(arrInSignData));

				ucRet[2]=SELFTEST_PASS;
				ulDataLen = 3;
			}
			else if(ucData[1] == PENTA_PUBLIC_DATA)	// 512byte Module -> PC
			{
				char ucPublic_key[1024] = {0,};
                int nPublickey_Size = 0;

                GetPublickey(ucPublic_key,&nPublickey_Size);

                //MONI 20180518
                // change public key to prevent dump ME attahced encrypted public key.
				//if(!DAMO_DUKPT_Client_PK_Auth_Start((unsigned char*)g_KMS_Publickey,arrOutRandomData,arrOutHashData,arrOutSignData,g_ucRandValue,ucPublicEnc))
				if(!DAMO_DUKPT_Client_PK_Auth_Start((unsigned char*)ucPublic_key,arrOutRandomData,arrOutHashData,arrOutSignData,g_ucRandValue,ucPublicEnc))
				{
					ucRet[2]=SELFTEST_PASS;

//					printf("\r\n ucPublicEnc\r\n");
//					for(i=0; i<sizeof(ucPublicEnc); i++)
//					{
//						printf("%02X ",ucPublicEnc[i]);
//					}
//					printf("\r\n");
					memcpy(&ucRet[3],ucPublicEnc,sizeof(ucPublicEnc));
					ulDataLen = 515;
				}
				else
				{
					ucRet[2]=SELFTEST_FAIL;
					ulDataLen = 3;
				}
			}
			else
			{
				ucRet[2]=SELFTEST_FAIL;
				ulDataLen = 3;
			}
		  }
			break;

		case eSELFTEST_SERIAL_WRITE_READ_IDX:									// 0x22
			{
			unsigned char ucOpenSerial[SIZE_SERIAL_NUMBER+1];
			unsigned char key[16];												// STM32F4 unique id address 12byte + 0x00
			unsigned char in[16];												// 암호화할 데이터
			unsigned char iv[16]={0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
			unsigned char ucEncryptionKey[32];			// out


			unsigned char arrInEnclpekValue[72];
			unsigned char arrOutEnclpekValue[145];
			unsigned char arrInKsn[11];
#ifdef RF_COMMON_MODEM
			unsigned char ucModuleSerial[8]={0,};
			unsigned int uiModuleSerialNum=0;
			boolean_t bSerialCheckResult=false;
#endif
			//uint32_t wAddress = 0x1FFF7A10;

			size_t encrypt_len;

			if(ucData[1] == SERIAL_WRITE)
			{
                uint8_t bModemActive = FALSE;   // 생산시에는 Modem을 Deactive시켜 줘야 한다.
				char ucPublic_key[1024] = {0,};
                int nPublickey_Size = 0;

				GetPublickey(ucPublic_key,&nPublickey_Size);

                memset((char *)in,0x00,sizeof(in));
				memset((char *)key,0x00,sizeof(key));
				memset((char *)ucEncryptionKey,0x00,sizeof(ucEncryptionKey));
				memset((char *)ucOpenSerial,0x00,SIZE_SERIAL_NUMBER+1);
				memcpy((char *)ucOpenSerial,&ucData[2],SIZE_SERIAL_NUMBER);
				
#ifdef RF_COMMON_MODEM
				memcpy((char *)ucModuleSerial,&ucData[3],SIZE_SERIAL_NUMBER_INT);
				uiModuleSerialNum = atoi((char*)ucModuleSerial);

				//PLS 단말시리얼 번호 범위 : R1200001 ~ R2000000
				//ELS 단말시리얼 번호 범위 : R1000001 ~ R1200000
				if(memcmp(ModemManagerData.aProductName,"PLS",3)==0)
				{
					if((uiModuleSerialNum >= PLS_RANGE_START) && (uiModuleSerialNum <= PLS_RANGE_END) )
					{
						bSerialCheckResult = true;
					}
					else
					{
						ucRet[2]=SER_NOT_CORRECT_PLS;				
                    }
				}
				else if(memcmp(ModemManagerData.aProductName,"ELS",3)==0)
				{
					if((uiModuleSerialNum >= ELS_RANGE_START) && (uiModuleSerialNum <= ELS_RANGE_END) )
					{
						bSerialCheckResult = true;
					}
					else
					{
						ucRet[2]=SER_NOT_CORRECT_ELS;
					}
				}
				else
				{
					ucRet[2]=SELFTEST_FAIL;
				}
#endif				
				// key 값 read (unique id address)
				//for(U32 index=0; index<STM32_UNIQUE_ID_ADDR_LENGTH; index++)	key[index] = *(__IO uint32_t*)(wAddress+index);
				GetDefaultEncryptKey((char *)key,false);

				 //KSN
				 memset(arrOutEnclpekValue,0x00,sizeof(arrOutEnclpekValue));
				 memcpy(arrInKsn,&ucData[17],sizeof(arrInKsn));
				 memcpy(arrInEnclpekValue,&ucData[28],sizeof(arrInEnclpekValue));
				 Convert_Ascii_To_HexString_len((char*)arrOutEnclpekValue,(char*)arrInEnclpekValue,sizeof(arrInEnclpekValue));

                //MONI 20180518
                // change public key to prevent dump ME attahced encrypted public key.
				 //if(!DAMO_DUKPT_Client_PK_Auth_End_Ex((unsigned char*)g_KMS_Publickey, arrOutEnclpekValue, arrInKsn, sizeof(arrInKsn), g_ucRandValue, in) )
				 if(!DAMO_DUKPT_Client_PK_Auth_End_Ex((unsigned char*)ucPublic_key, arrOutEnclpekValue, arrInKsn, sizeof(arrInKsn), g_ucRandValue, in) )
				 {
					 printf(" \r\n ipek OK!!!!!!!!!!\r\n");
					 for(i=0; i<sizeof(in); i++){
					 	printf("%02X ",in[i]);
					 }
					 printf("\r\n");

					printf("============================ AES-CBC ============================\n");
					printdump( "aes-cbc encrypt(plaintext)", in, sizeof(in) );
					DAMO_CRYPT_AES_EncryptEx(ucEncryptionKey, &encrypt_len, in, sizeof(in), (unsigned char const*)key, sizeof(key), AES_128, CBC_MODE, iv);
					printdump( "aes-cbc encrypt(ciphertext)", ucEncryptionKey, encrypt_len );
					printf("\r\n encrypt_len : %d\r\n",encrypt_len);

	//				확인용 decryption
					unsigned char ucDecryptionKey[64];
					size_t decrypt_len;
					DAMO_CRYPT_AES_DecryptEx(ucDecryptionKey, &decrypt_len, ucEncryptionKey, encrypt_len, (unsigned char const*)key, 16, AES_128, CBC_MODE, iv);
					printdump( "aes-cbc decrypt(plaintext)", ucDecryptionKey, decrypt_len );
					printf("\r\n decrypt_len : %d\r\n",decrypt_len);
					printf("=================================================================\r\n");

					// 20180504 VINN이 초기화되지않아 초기화 함수 넣어줌
					SelftestDefaultFWInfo();
					
					// Serial Write
#ifdef RF_COMMON_MODEM
					if(bSerialCheckResult == true)
#endif
					{
					if(SetFWSerialNumber((char *)ucOpenSerial))	ucRet[2]=SELFTEST_PASS;
					else										ucRet[2]=SELFTEST_FAIL;

					if(SetModuleKey((char *)ucEncryptionKey))	ucRet[2]=SELFTEST_PASS;
					else										ucRet[2]=SELFTEST_FAIL;
					}

				 }
				 else
				 {
					 ucRet[2]=SELFTEST_FAIL;
				 }
				ulDataLen = 3;

                //MONI 2018-03-04
                // added this code to initialize autolink configuration to serial flash
                WriteDefaultAutolinkConfigValue();
                SetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bModemActive);
			}
			else if(ucData[1] == SERIAL_READ){
				memset((char *)ucOpenSerial,0x00,sizeof(ucOpenSerial));

				memcpy(&ucOpenSerial[0],GetFWSerialNumber(), SIZE_SERIAL_NUMBER);
				printf("SERIAL_READ_IDX %s \n",ucOpenSerial);

				memcpy(&ucRet[2],ucOpenSerial, SIZE_SERIAL_NUMBER);
				ulDataLen = 17;

			}
			else{
				ucRet[2]=SELFTEST_FAIL;
				ulDataLen = 3;
			}

			}
			break;

		case eSELFTEST_PERIODIC_DATA_STOP_IDX:									// 0x33

			if(g_bUSIMInsertFlag == false){
				ucRet[1]=SELFTEST_FAIL;
			}
			else{
				ucRet[1]=SELFTEST_PASS;
				SetOBDState(eOBD_Selftest);
				// Serial이 있는 경우 생산 검사 재 동작시 eMODEM_PRODUCT_MODE로 진입안해 추가
				SetModemState(eMODEM_PRODUCT_MODE);
				SetLedOnOffCtl(LED_ON, eLED_ALL);
				g_bYUJINSelftestFlag = true;
				SetupForInterruptforImpulse(true, false, 0xAF,true);
				SetSysDebugModule(DEBUG_MODULES_SYSTEM_HADNLER,false);
			}
			ulDataLen = 2;
#if 0
			SetOBDState(eOBD_Selftest);
			SetLedOnOffCtl(LED_ON, eLED_ALL);
			g_bYUJINSelftestFlag = true;
			SetupForInterruptforImpulse(true, false, 0xAF,true);
			SetSysDebugModule(DEBUG_MODULES_SYSTEM_HADNLER,false);

			ulDataLen = 1;
#endif
			break;

		case eSELFTEST_BT_VERSION_IDX:											// 0x35
#ifdef RF_COMMON_MODEM //mod.kks 21.12.23 to array the point.
            //"v_0.0.1.5"
            ucRet[1] = 'v';
            ucRet[2] = '_';
            ucRet[3] = '0';
            ucRet[4] = '.';
			memcpy(&ucRet[5],g_LocalBTInfo.strVersion,sizeof(g_LocalBTInfo.strVersion));
#else
            memcpy(&ucRet[1],g_LocalBTInfo.strVersion,sizeof(g_LocalBTInfo.strVersion));
#endif
			ulDataLen = 13;
			break;

		case eSELFTEST_MODEM_VERSION_IDX:										// 0x36
			{
				uint8_t arrBuff[15];
				memset(arrBuff, 0x00, sizeof(arrBuff));

				if ( memcmp(ModemManagerData.aRevision, arrBuff, sizeof(arrBuff)) == 0 ){
					ucRet[1]=SELFTEST_FAIL;
					ulDataLen = 2;
				}
				else{
					printf("** Success - Modem version !!!!!!!\r\n");
					//memcpy(&ucRet[2],ModemManagerData.aRevision,sizeof(ModemManagerData.aRevision));
#ifndef RF_COMMON_MODEM
					memcpy(&ucRet[2],ModemManagerData.aRevision,sizeof(arrBuff));
#else
                    sprintf((char*)&ucRet[2],"%s%c","REVISION 01.00",ModemManagerData.aARevision[19]);
#endif
					ucRet[1]=SELFTEST_PASS;
					ulDataLen = 17;
				}

			}
			break;

		case eSELFTEST_USIM_OPEN_IDX:											// 0x3A
			{
				if ( ModemManagerData.eNetworkRegStatus != eNETWORK_REG_STATUS_REGISTERED ){
					ucRet[1]=SELFTEST_FAIL;
					ulDataLen = 2;
				}
				else{

					memcpy(&ucRet[2],ModemManagerData.aPhoneNo,sizeof(ModemManagerData.aPhoneNo));
					ucRet[1]=SELFTEST_PASS;
					ulDataLen = 2;
				}
			}
			break;

		case eSELFTEST_PHONE_NUM_IDX:											// 0x3B
			{
				uint8_t arrBuff[20];
				memset(arrBuff, 0x00, sizeof(arrBuff));

				if ( memcmp(ModemManagerData.aPhoneNo, arrBuff, sizeof(ModemManagerData.aPhoneNo)) == 0 ){
					ucRet[1]=SELFTEST_FAIL;
					ulDataLen = 2;
				}
				else{
					printf("** Success - Phone Num !!!!!!!\r\n");
					memcpy(&ucRet[2],ModemManagerData.aPhoneNo,sizeof(ModemManagerData.aPhoneNo));
					ucRet[1]=SELFTEST_PASS;
					ulDataLen = 13;
				}
			}
			break;

		case eSELFTEST_IG_ON_WAKEUP_IDX:										// 0x47

			ucRet[1] = ucData[1];
			if(ucData[1] == LATCH_SETTING)
			{
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC,		eWAKE_CTL_PIN_ENABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX,	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX,		eWAKE_CTL_PIN_DISABLE);
				ucRet[2]=SELFTEST_PASS;
			}
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				// Wake Up variable
				bool	bOldWakeUpPin = 0;
				bool	bNewWakeUpPin = 0;
				bool	bOldIGOnWakeUpPin = 0;
				bool	bNewIGOnWakeUpPin = 0;

				unsigned int uiWakeupPinToggleCnt = 0;
				unsigned int uiIGOnPinToggleCnt = 0;

				bOldWakeUpPin = GetWakeupPA0PinState();
				bOldIGOnWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_IG_ON);

				uiOldTimer = Get_Tmr();
				while(1)
				{
					bNewWakeUpPin = GetWakeupPA0PinState();
					bNewIGOnWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_IG_ON);

					if(bOldWakeUpPin !=	bNewWakeUpPin){
						bOldWakeUpPin = bNewWakeUpPin;
						uiWakeupPinToggleCnt++;
					}
					if(bOldIGOnWakeUpPin != bNewIGOnWakeUpPin){
						bOldIGOnWakeUpPin = bNewIGOnWakeUpPin;
						uiIGOnPinToggleCnt++;
					}

					uiDeltatime=Get_TmrDelta(Get_Tmr(),uiOldTimer);

					if(uiDeltatime >= SELFTEST_MODEM_RES_TIMEOUT){
						if(uiWakeupPinToggleCnt > 0 && uiIGOnPinToggleCnt > 0){
							ucRet[2]=SELFTEST_PASS;
							printf(" * cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiIGOnPinToggleCnt);
							break;
						}
						else{
							ucRet[2]=SELFTEST_FAIL;
						}
						printf(" ** cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiIGOnPinToggleCnt);
						break;
					}
				}
			}
			else{
				ucRet[2] = SELFTEST_FAIL;
			}
			ulDataLen = 3;
			break;

		case eSELFTEST_HIGHCAN1_WAKEUP:											// 0x40

			ucRet[1] = ucData[1];
			if(ucData[1] == LATCH_SETTING)
			{
				ModemManagerData.eState = eMODEM_NONE;

				printf("CAN: Enable HIGH CAN1\n");
				Oem_CAN_Initial_CH1(Highcan1, eCAN_500KBPS); //init
				CANCH = CAN_CHANNEL_1;

				DefaultMaskSet(uiStartMask,uiEndMask,8);
				Oem_CAN_Channel_Masket_Set(CANCH, STANDARD_CAN, 8, uiStartMask, uiEndMask);

				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX,	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX,		eWAKE_CTL_PIN_ENABLE);

				printf("ISR Setting\n");
				SetupForInterruptforImpulse(true, false, 0xAF, true);

				DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
				ucRet[2]=SELFTEST_PASS;
			}
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				// Wake Up variable
				bool	bOldWakeUpPin = 0;
				bool	bNewWakeUpPin = 0;
				bool	bOldCanRxWakeUpPin = 0;
				bool	bNewCanRxWakeUpPin = 0;

				unsigned int uiWakeupPinToggleCnt = 0;
				unsigned int uiCANRxPinToggleCnt = 0;

				bOldWakeUpPin = GetWakeupPA0PinState();
				bOldCanRxWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_CAN_RX);

				uiOldTimer = Get_Tmr();
				while(1)
				{
					bNewWakeUpPin = GetWakeupPA0PinState();
					bNewCanRxWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_CAN_RX);
					if(bOldWakeUpPin !=	bNewWakeUpPin){
						bOldWakeUpPin = bNewWakeUpPin;
						uiWakeupPinToggleCnt++;
					}
					if(bOldCanRxWakeUpPin != bNewCanRxWakeUpPin){
						bOldCanRxWakeUpPin = bNewCanRxWakeUpPin;
						uiCANRxPinToggleCnt++;
					}

					uiDeltatime=Get_TmrDelta(Get_Tmr(),uiOldTimer);

					if(uiWakeupPinToggleCnt>SELFTEST_TOGGLE_CNT && uiCANRxPinToggleCnt>SELFTEST_TOGGLE_CNT){
						 ucRet[2]=SELFTEST_PASS;
						 printf(" * cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiCANRxPinToggleCnt);
						 break;
					}

					if(uiDeltatime >= SELFTEST_MODEM_RES_TIMEOUT){
						if(uiWakeupPinToggleCnt>SELFTEST_TOGGLE_CNT && uiCANRxPinToggleCnt>SELFTEST_TOGGLE_CNT){
							ucRet[2]=SELFTEST_PASS;
						}
						else{
							ucRet[2]=SELFTEST_FAIL;
						}
						printf(" ** cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiCANRxPinToggleCnt);
						break;
					}
				}
			}
			else		ucRet[2]=SELFTEST_FAIL;

			ulDataLen = 3;
			break;

		case eSELFTEST_HIGHCAN2_WAKEUP:											// 0x41

			ucRet[1] = ucData[1];
			if(ucData[1] == LATCH_SETTING)
			{
				ModemManagerData.eState = eMODEM_NONE;

				printf("CAN: Enable HIGH CAN2\n");
				Oem_CAN_Initial_CH1(Highcan2, eCAN_500KBPS); //init
				CANCH = CAN_CHANNEL_1;

				DefaultMaskSet(uiStartMask,uiEndMask,8);
				Oem_CAN_Channel_Masket_Set(CANCH, STANDARD_CAN, 8, uiStartMask, uiEndMask);

				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX,	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX,		eWAKE_CTL_PIN_ENABLE);

				printf("ISR Setting\n");
				SetupForInterruptforImpulse(true, false, 0xAF,true);

				DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
				ucRet[2]=SELFTEST_PASS;
			}
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				bool	bOldWakeUpPin = 0;
				bool	bNewWakeUpPin = 0;
				bool	bOldCanRxWakeUpPin = 0;
				bool	bNewCanRxWakeUpPin = 0;

				unsigned int uiWakeupPinToggleCnt = 0;
				unsigned int uiCANRxPinToggleCnt = 0;



				bOldWakeUpPin = GetWakeupPA0PinState();
				printf("OLD GPIO_WAKEUP        : %s\n\n", (bOldWakeUpPin == true) ? "HIGH" : "LOW");
				bOldCanRxWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_CAN_RX);
				printf("OLD CAN_RX         : %s\n\n", (bOldCanRxWakeUpPin == true) ? "HIGH" : "LOW");

				uiOldTimer = Get_Tmr();
				while(1)
				{
					bNewWakeUpPin = GetWakeupPA0PinState();
					bNewCanRxWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_CAN_RX);
					if(bOldWakeUpPin !=	bNewWakeUpPin){
						bOldWakeUpPin = bNewWakeUpPin;
						uiWakeupPinToggleCnt++;
					}
					if(bOldCanRxWakeUpPin != bNewCanRxWakeUpPin){
						bOldCanRxWakeUpPin = bNewCanRxWakeUpPin;
						uiCANRxPinToggleCnt++;
					}

					uiDeltatime=Get_TmrDelta(Get_Tmr(),uiOldTimer);

					if(uiWakeupPinToggleCnt>SELFTEST_TOGGLE_CNT && uiCANRxPinToggleCnt>SELFTEST_TOGGLE_CNT){
						 ucRet[2]=SELFTEST_PASS;
						 printf(" * cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiCANRxPinToggleCnt);
						 break;
					}

					if(uiDeltatime >= SELFTEST_MODEM_RES_TIMEOUT){
						if(uiWakeupPinToggleCnt>SELFTEST_TOGGLE_CNT && uiCANRxPinToggleCnt>SELFTEST_TOGGLE_CNT){
							ucRet[2]=SELFTEST_PASS;
						}
						else{
							ucRet[2]=SELFTEST_FAIL;
						}
						printf(" ** cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiCANRxPinToggleCnt);
						break;
					}
				}
			}
			else		ucRet[2]=SELFTEST_FAIL;

			ulDataLen = 3;
			break;

		case eSELFTEST_HIGHCAN3_WAKEUP:											// 0x42

			ucRet[1] = ucData[1];
			if(ucData[1] == LATCH_SETTING)
			{
				ModemManagerData.eState = eMODEM_NONE;

				printf("CAN: Enable HIGH CAN3\r\n");
				Oem_CAN_Initial_CH2(Highcan3, eCAN_500KBPS); //init
				CANCH = CAN_CHANNEL_2;

				DefaultMaskSet(uiStartMask,uiEndMask,8);
				Oem_CAN_Channel_Masket_Set(CANCH, STANDARD_CAN, 8, uiStartMask, uiEndMask);

				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC, 		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE, 	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX,	eWAKE_CTL_PIN_ENABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX,		eWAKE_CTL_PIN_DISABLE);

				printf("ISR Setting\n");
				SetupForInterruptforImpulse(true, false, 0xAF, true);

				DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
				ucRet[2]=SELFTEST_PASS;
			}
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				bool	bOldWakeUpPin = 0;
				bool	bNewWakeUpPin = 0;
				bool	bOldLHCanRxWakeUpPin = 0;
				bool	bNewLHCanRxWakeUpPin = 0;

				unsigned int uiWakeupPinToggleCnt = 0;
				unsigned int uiLHCANRxPinToggleCnt = 0;

				bOldWakeUpPin = GetWakeupPA0PinState();
				printf("OLD GPIO_WAKEUP        : %s\n\n", (bOldWakeUpPin == true) ? "HIGH" : "LOW");
				bOldLHCanRxWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_LOW_HIGH_CAN_RX);
				printf("OLD CAN_RX         : %s\n\n", (bOldLHCanRxWakeUpPin == true) ? "HIGH" : "LOW");

				uiOldTimer = Get_Tmr();
				while(1)
				{
					bNewWakeUpPin = GetWakeupPA0PinState();
					bNewLHCanRxWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_LOW_HIGH_CAN_RX);
					if(bOldWakeUpPin !=	bNewWakeUpPin){
						bOldWakeUpPin = bNewWakeUpPin;
						uiWakeupPinToggleCnt++;
					}
					if(bOldLHCanRxWakeUpPin != bNewLHCanRxWakeUpPin){
						bOldLHCanRxWakeUpPin = bNewLHCanRxWakeUpPin;
						uiLHCANRxPinToggleCnt++;
					}

					uiDeltatime=Get_TmrDelta(Get_Tmr(),uiOldTimer);

					if(uiWakeupPinToggleCnt>SELFTEST_TOGGLE_CNT && uiLHCANRxPinToggleCnt>SELFTEST_TOGGLE_CNT){
						 ucRet[2]=SELFTEST_PASS;
						 printf(" * cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiLHCANRxPinToggleCnt);
						 break;
					}

					if(uiDeltatime >= SELFTEST_MODEM_RES_TIMEOUT){
						if(uiWakeupPinToggleCnt>SELFTEST_TOGGLE_CNT && uiLHCANRxPinToggleCnt>SELFTEST_TOGGLE_CNT){
							ucRet[2]=SELFTEST_PASS;
						}
						else{
							ucRet[2]=SELFTEST_FAIL;
						}
						printf(" ** cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiLHCANRxPinToggleCnt);
						break;
					}
				}
			}
			else		ucRet[2]=SELFTEST_FAIL;

			ulDataLen = 3;

			break;

		case eSELFTEST_LOWCAN_WAKEUP:											// 0x43
			{
			unsigned int nLoopCnt = 0;
			unsigned int uiStartID;
			unsigned int uiEndID;
			stCanPacket OutCanPacket, InCanPacket;

			ucRet[1] = ucData[1];
			if(ucData[1] == LATCH_SETTING)
			{
				ModemManagerData.eState = eMODEM_NONE;

				printf("CAN: Enable LOW CAN\r\n");
				Oem_CAN_Initial_CH2(Lowcan1, eCAN_100KBPS); //init
				CANCH = CAN_CHANNEL_2;

				DefaultMaskSet(uiStartMask,uiEndMask,8);
				Oem_CAN_Channel_Masket_Set(CANCH, STANDARD_CAN, 8, uiStartMask, uiEndMask);

				uiStartID = 0X07DF;
				uiEndID = 0X0723;
				uiEndID = uiEndID;

				OutCanPacket.stNormalPacket.ucSOF 		= CAN_FRAME_SOF;
				OutCanPacket.stNormalPacket.us11BitID	= uiStartID;
				OutCanPacket.stNormalPacket.ucRTR		= 0;
				OutCanPacket.stNormalPacket.ucIDE		= CAN_FRAME_STANDARD_IDE;
				OutCanPacket.stNormalPacket.ucReserved	= 0;
				OutCanPacket.stNormalPacket.ucDLC		= CAN_FRAME_DATA_SIZE;
				OutCanPacket.stNormalPacket.usCRC		= 0;
				OutCanPacket.stNormalPacket.ucCRCDelimiter = 1;
				OutCanPacket.stNormalPacket.ucACK		= 1;
				OutCanPacket.stNormalPacket.ucACKDelimiter = 1;
				OutCanPacket.stNormalPacket.ucEOF		= CAN_FRAME_EOF;

				OutCanPacket.stNormalPacket.arrDataFields[0] = 0x02;
				OutCanPacket.stNormalPacket.arrDataFields[1] = 0x10;
				OutCanPacket.stNormalPacket.arrDataFields[2] = 0x81;
				OutCanPacket.stNormalPacket.arrDataFields[3] = 0x00;
				OutCanPacket.stNormalPacket.arrDataFields[4] = 0x00;
				OutCanPacket.stNormalPacket.arrDataFields[5] = 0x00;
				OutCanPacket.stNormalPacket.arrDataFields[6] = 0x00;
				OutCanPacket.stNormalPacket.arrDataFields[7] = 0x00;

				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC, 		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE, 	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX,	eWAKE_CTL_PIN_ENABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX,		eWAKE_CTL_PIN_DISABLE);

				printf("ISR Setting\n");
				SetupForInterruptforImpulse(true, false, 0xAF, true);

				if ( FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, CANCH) > 0 )
				{
					nLoopCnt = 0;
					uiRecvLen = 0;
					do {
						uiRecvLen = FineReadCanBuff((unsigned char*)&InCanPacket, sizeof(InCanPacket), NULL, CANCH);

						if ( nLoopCnt++ > 3 )
						{
							GITDebugPrintf("Receive FAil ~~~~~~~~~~~~~~~~~~\r\n");
							ucRet[2]=SELFTEST_FAIL;
							break;
						}
						else
						{
							if( uiRecvLen > 0 )
							{
								GITDebugPrintf("Receive success ~~~~~~~~~~~~~~~~~~\r\n");
								ucRet[2]=SELFTEST_PASS;
							}
							else
								Oem_GIT_mDelay(30);
						}
					}while(uiRecvLen == 0);
				}
				else
				{
					GITDebugPrintf("Find Packet FAil ~~~~~~~~~~~~~~~~~~\r\n");
					ucRet[2]=SELFTEST_FAIL;
				}
			}
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				bool	bOldWakeUpPin = 0;
				bool	bNewWakeUpPin = 0;
				bool	bOldLHCanRxWakeUpPin = 0;
				bool	bNewLHCanRxWakeUpPin = 0;

				unsigned int uiWakeupPinToggleCnt = 0;
				unsigned int uiLHCANRxPinToggleCnt = 0;


				bOldWakeUpPin = GetWakeupPA0PinState();
				printf("OLD GPIO_WAKEUP        : %s\n\n", (bOldWakeUpPin == true) ? "HIGH" : "LOW");
				bOldLHCanRxWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_LOW_HIGH_CAN_RX);
				printf("OLD CAN_RX         : %s\n\n", (bOldLHCanRxWakeUpPin == true) ? "HIGH" : "LOW");

				uiOldTimer = Get_Tmr();
				while(1)
				{
					bNewWakeUpPin = GetWakeupPA0PinState();
					bNewLHCanRxWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_LOW_HIGH_CAN_RX);
					if(bOldWakeUpPin !=	bNewWakeUpPin){
						bOldWakeUpPin = bNewWakeUpPin;
						uiWakeupPinToggleCnt++;
					}
					if(bOldLHCanRxWakeUpPin != bNewLHCanRxWakeUpPin){
						bOldLHCanRxWakeUpPin = bNewLHCanRxWakeUpPin;
						uiLHCANRxPinToggleCnt++;
					}

					uiDeltatime=Get_TmrDelta(Get_Tmr(),uiOldTimer);

					if(uiWakeupPinToggleCnt>SELFTEST_TOGGLE_CNT && uiLHCANRxPinToggleCnt>SELFTEST_TOGGLE_CNT){
						 ucRet[2]=SELFTEST_PASS;
						 printf(" * cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiLHCANRxPinToggleCnt);
						 break;
					}

					if(uiDeltatime >= SELFTEST_MODEM_RES_TIMEOUT){
						if(uiWakeupPinToggleCnt>SELFTEST_TOGGLE_CNT && uiLHCANRxPinToggleCnt>SELFTEST_TOGGLE_CNT){
							ucRet[2]=SELFTEST_PASS;
						}
						else{
							ucRet[2]=SELFTEST_FAIL;
						}
						printf(" ** cnt :%d  cnt :%d \n\n",uiWakeupPinToggleCnt,uiLHCANRxPinToggleCnt);
						break;
					}
				}
			}
			else		ucRet[2]=SELFTEST_FAIL;

			ulDataLen = 3;
			}
			break;
		case eSELFTEST_BT_WAKEUP:												// 0x44

			ucRet[1] = ucData[1];
			if(ucData[1] == LATCH_SETTING)
			{
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC, 		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE, 	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX,	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON,		eWAKE_CTL_PIN_ENABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX,		eWAKE_CTL_PIN_DISABLE);

				ucRet[2]=SELFTEST_PASS;
				ucRet[3]=g_LocalBTInfo.bt_addr.btAddr[5];
				ucRet[4]=g_LocalBTInfo.bt_addr.btAddr[4];
				ucRet[5]=g_LocalBTInfo.bt_addr.btAddr[3];
				ucRet[6]=g_LocalBTInfo.bt_addr.btAddr[2];
				ucRet[7]=g_LocalBTInfo.bt_addr.btAddr[1];
				ucRet[8]=g_LocalBTInfo.bt_addr.btAddr[0];
				ulDataLen = 9;
			}
#if 0
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				bool	bWakeUpPin = 0;
				bool	bBTWakeUpPin = 0;

				bWakeUpPin = GetWakeupPA0PinState();
				bBTWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_BT_MON);
				printf("GPIO_WAKEUP        : %s\n\n", (bWakeUpPin == true) ? "HIGH" : "LOW");
				printf("BT_MON         : %s\n\n", (bBTWakeUpPin == true) ? "HIGH" : "LOW");

				if((bWakeUpPin == true) && (bBTWakeUpPin == true))	ucRet[2]=SELFTEST_PASS;
				else												ucRet[2]=SELFTEST_FAIL;
				ulDataLen = 3;
			}
#else
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				bool	bWakeUpPin = 0;
				bool	bBTWakeUpPin = 0;

				uiOldTimer = Get_Tmr();
				while(1)
				{
					bWakeUpPin = GetWakeupPA0PinState();
					bBTWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_BT_MON);

					uiDeltatime=Get_TmrDelta(Get_Tmr(),uiOldTimer);

					if((bWakeUpPin == true) && (bBTWakeUpPin == true)){
						ucRet[2]=SELFTEST_PASS;
						break;
					}
					if(uiDeltatime >= SELFTEST_MODEM_RES_TIMEOUT){
						if((bWakeUpPin == true) && (bBTWakeUpPin == true))	ucRet[2]=SELFTEST_PASS;
						else{	printf("** BT timeout FAIL\r\n");			ucRet[2]=SELFTEST_FAIL;}
						break;
					}
				}
				printf("GPIO_WAKEUP        : %s\n\n", (bWakeUpPin == true) ? "HIGH" : "LOW");
				printf("BT_MON         : %s\n\n", (bBTWakeUpPin == true) ? "HIGH" : "LOW");
				ulDataLen = 3;
			}
#endif
			else{
				ucRet[2]=SELFTEST_FAIL;
				ulDataLen = 3;
			}

			break;
		case eSELFTEST_SENSOR_WAKEUP:											// 0x45

			ucRet[1] = ucData[1];
			if(ucData[1] == LATCH_SETTING)
			{
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC, 		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE, 	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR,		eWAKE_CTL_PIN_ENABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX,	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX,		eWAKE_CTL_PIN_DISABLE);

				printf("ISR Setting\n");
				SetupForInterruptforImpulse(true, false, 0xAF,true);

				ucRet[2]=SELFTEST_PASS;
			}
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				uiOldTimer = Get_Tmr();
				while(1)
				{
					uiDeltatime=Get_TmrDelta(Get_Tmr(),uiOldTimer);

					if(uiDeltatime >= SELFTEST_BUZZER_DELAY){
						if(g_uiWakeupPinToggleCnt > 0 && g_uiSensorWakeupPinToggleCnt > 0){
							ucRet[2]=SELFTEST_PASS;
						}
						else{
							ucRet[2]=SELFTEST_FAIL;
						}
						printf(" ** cnt :%d  cnt :%d \n\n",g_uiWakeupPinToggleCnt,g_uiSensorWakeupPinToggleCnt);
						break;
					}
				}
			}
			else	ucRet[2]=SELFTEST_FAIL;

			g_uiWakeupPinToggleCnt = 0;
			g_uiSensorWakeupPinToggleCnt = 0;
			ulDataLen = 3;
			break;
		case eSELFTEST_MODEM_WAKEUP:											// 0x46

			if(ucData[1] == LATCH_SETTING)
			{
				ModemManagerData.nMdmSubState = 0;
				ModemManagerData.nMdmSubNextState = 0;

				gbModemRingPinMode = MODEM_RING_PIN_MODE_GPIO;

                SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC, eWAKE_CTL_PIN_DISABLE);
                SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE, eWAKE_CTL_PIN_ENABLE);
                SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR, eWAKE_CTL_PIN_DISABLE);
                SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX, eWAKE_CTL_PIN_DISABLE);
                SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON, eWAKE_CTL_PIN_DISABLE);
                SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX, eWAKE_CTL_PIN_DISABLE);

				if(SendModemCommand(SelftestProcessConfigModemGPIO)){
					ucRet[2]=SELFTEST_PASS;
					printf("\r\n MODEM PIN GPIO SETTING OK!! \r\n");
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n MODEM PIN GPIO SETTING FAIL!! \r\n");
				}
			}
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				boolean_t bModemWakeUpPin = 0;
				boolean_t bFirstResult = false, bSecondResult = false, bLastResult = false;  

				ModemManagerData.nMdmSubState = 0;
				ModemManagerData.nMdmSubNextState = 0;
				gbModemRingPinState = MODEM_RING_PIN_STATE_LOW;

				printf("MDM: Set & Run, MODEM Ring pin is LOW\n");
				if(SendModemCommand(SelftestProcessSetResetModemGPIO)){
					printf("\r\n MODEM RING PIN SETTING LOW OK!! \r\n");

					bModemWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_MODEM);
					if(bModemWakeUpPin == true)	bFirstResult = SELFTEST_PASS;
					else{
						printf("FAIL!! OLD MODEM WAEKUP        : %s\n\n", (bModemWakeUpPin == true) ? "HIGH" : "LOW");
						bFirstResult =SELFTEST_FAIL;
					}
				}
				else{
					bFirstResult = SELFTEST_FAIL;
					printf("\r\n MODEM PIN GPIO LOW SETTING FAIL!! \r\n");
				}
				Oem_GIT_mDelay(500);

				ModemManagerData.nMdmSubState = 0;
				ModemManagerData.nMdmSubNextState = 0;
				gbModemRingPinState = MODEM_RING_PIN_STATE_HIGH;

				printf("MDM: Set & Run, MODEM Ring pin is HIGH\n");
				if(SendModemCommand(SelftestProcessSetResetModemGPIO)){
					printf("\r\n MODEM RING PIN SETTING HIGH OK!! \r\n");

					bModemWakeUpPin = GetWakeupDetectPinState(eWAKE_PIN_MODEM);
					if(bModemWakeUpPin == false)	bSecondResult=SELFTEST_PASS;
					else{
						printf("FAIL!! OLD MODEM WAEKUP        : %s\n\n", (bModemWakeUpPin == true) ? "HIGH" : "LOW");
						bSecondResult=SELFTEST_FAIL;
					}
				}
				else{
					bSecondResult = SELFTEST_FAIL;
					printf("\r\n MODEM PIN GPIO HIGH SETTING FAIL!! \r\n");
				}

				ModemManagerData.nMdmSubState = 0;
				ModemManagerData.nMdmSubNextState = 0;

				gbModemRingPinMode = MODEM_RING_PIN_MODE_STD;					// Modem wakeup pin standard로 변경하고 완료
				
				if(SendModemCommand(SelftestProcessConfigModemGPIO)){
					bLastResult=SELFTEST_PASS;
					printf("\r\n MODEM PIN GPIO SETTING OK!! \r\n");
				}
				else{
					bLastResult=SELFTEST_FAIL;
					printf("\r\n MODEM PIN GPIO SETTING FAIL!! \r\n");
				}

				ucRet[2] = bFirstResult | bSecondResult | bLastResult; //SELFTEST_PASS = 0, SELFTEST_FAIL = 1
				
#ifdef RF_COMMON_MODEM //mod.pdh 22.01.24 
				if(Md_SetGPIODriver("0") == true) {	}
#endif
			}
                        
                 
			ulDataLen = 3;

			break;

		case eSELFTEST_ACCBATT_WAKEUP:											// 0x3D

			if(ucData[1] == MAIN_SLEEP)
			{
#ifndef RF_COMMON_MODEM
				ModemManagerData.nMdmSubState = 1;
				ModemManagerData.nMdmSubNextState = 1;
#else
				ModemManagerData.nMdmSubState = 2;  //mod.kks 21.12.22 to fix the sequence.
				ModemManagerData.nMdmSubNextState = 2;//mod.kks 21.12.22 to fix the sequence.
#endif
				if(SendModemCommand(SelftestProcessSetModemPower)){
					ucRet[2]=SELFTEST_PASS;
					printf("\r\n AT+COPS OK!! \r\n");
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n AT+COPS FAIL!! \r\n");
				}

				// Modem Flight mode On
				ModemManagerData.nMdmSubState = 0;
				ModemManagerData.nMdmSubNextState = 0;

				if(SendModemCommand(SelftestProcessSetFlightMode)){
					ucRet[2]=SELFTEST_PASS;
					printf("\r\n MODEM FLGHT MODE SETTING OK!! \r\n");
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n MODEM FLGHT MODE SETTING FAIL!! \r\n");
				}
				// Modem Power save Sleep
				ModemManagerData.nMdmSubState = 0;
				ModemManagerData.nMdmSubNextState = 0;

				if(SendModemCommand(ProcessEnterModemPowerSaveMode)){
					ucRet[2]=SELFTEST_PASS;
					printf("\r\n MODEM POWER SAVE MODE SETTING OK!! \r\n");
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n MODEM POWER SAVE MODE SETTING FAIL!! \r\n");
				}

				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC, 		eWAKE_CTL_PIN_ENABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE, 	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX,	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX,		eWAKE_CTL_PIN_DISABLE);

				ModemManagerData.eState = eMODEM_NONE;

//		#warning "//MONI 2017/12/19 it was added for current test, we needs check reset sequence."
			  //MONI 2017-12-19 it was added for current test, we needs check reset sequence.
			  SetupForInterruptforImpulse(true, false, 0xAF,true);

			  //INOM

		#if _NOT_USED_RESET_IN_POWER_COMMAND__
				//MONI 2017/12/19 It was disable because this routine is included in EnterStandbyMode() method
                HalDrvRtcIOCtrl(eRtc_IO_AlarmEnable, HAL_RTC_Alarm_A, NULL, 0, HAL_DISABLE);

				/* Allow access to BKP Domain */
				printf("Allow access to BKP Domain\n");
                HalDrvPowerIOCtrl(ePWR_IO_BK_PwAccessEnable, 0, NULL, 0, HAL_ENABLE);

				/* Clear Wakeup flag */
				printf("Clear Wakeup flag\n");
                HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_WU, NULL, 0, 0);

				/* Clear StandBy flag */
				printf("Clear StandBy flag\n");
                HalDrvPowerIOCtrl(ePWR_IO_SetFlagStatus, HAL_PWR_FLAG_SB, NULL, 0, 0);

                /* Enable WKUP pin  */
                printf("Enable WKUP pin\n");
                HalDrvPowerIOCtrl(ePWR_IO_WakeupPinEnable, 0, NULL, 0, HAL_ENABLE);

				printf("ePWR_IO_PWR_EnterStandbyMode()\n");
                HalDrvPowerIOCtrl(ePWR_IO_PWR_EnterStandbyMode, 0, NULL, 0, 0);
		#endif //_NOT_USED_RESET_IN_POWER_COMMAND__

				EnterStandbyMode();

				while(1);
			}
			else{
				ucRet[1] = ucData[1];
				ucRet[2] = SELFTEST_FAIL;
				ulDataLen = 3;
			}

			break;

		case eSELFTEST_MODEMBAUD_SET_IDX:
			{
				unsigned char ucOpenSerial[SIZE_SERIAL_NUMBER+1];

				ModemManagerData.nMdmSubState = 0;
				ModemManagerData.nMdmSubNextState = 0;
				ModemManagerData.bModemRcvPBReadyMessageFlag = 0;

				if(SendModemCommand(SelftestProcessSetModemBaudrate))
				{
					if(SendModemCommand(SelftestProcessInitializeModem))
					{
						printf("\r\n MODEM bps SETTING OK!! Set initial\r\n");
						if ( memcmp(g_FirmwareInfo.arrSerialNumber, DEFAULT_SERIAL_NUMBER, SIZE_SERIAL_NUMBER) == 0 )
						{
							memset((char *)ucOpenSerial,0x00,SIZE_SERIAL_NUMBER+1);
							memcpy((char *)ucOpenSerial,DEFAULT_SERIAL_NUMBER,SIZE_SERIAL_NUMBER);
							ucOpenSerial[13] = '1';
							ucOpenSerial[14] = 'M';

							if(SetFWSerialNumber((char *)ucOpenSerial))	ucRet[2]=SELFTEST_PASS;
							else										ucRet[2]=SELFTEST_FAIL;
						}
						else
						{
							printf("\r\n[serial Number : ");
							for ( i=0; i<SIZE_SERIAL_NUMBER; i++ )
							{
								printf("%c", g_FirmwareInfo.arrSerialNumber[i]);
							}
							printf("]\r\n");
							ucRet[2]=SELFTEST_PASS;
						}
					}
					else
					{
						ucRet[2]=SELFTEST_FAIL;
						printf("\r\n ProcessInitializeModem FAIL!! \r\n");
					}
				}
				else
				{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n MODEM bps SETTING FAIL!! \r\n");
				}
			}
			break;

		case eSELFTEST_SENSOR_WAKEUP_VARI:
			ucRet[1] = ucData[1];
			ucRet[2] = ucData[2];
			if(ucData[1] == LATCH_SETTING)
			{
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_IG_ACC, 		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_MO_WAKE, 	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_SENSOR,		eWAKE_CTL_PIN_ENABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_LOW_CAN_RX,	eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_BT_MON,		eWAKE_CTL_PIN_DISABLE);
				SetCtlWakeDetectPinState(eCTL_WAKE_DET_PIN_CAN_RX,		eWAKE_CTL_PIN_DISABLE);

				printf("ISR Setting\n");

				SetupForInterruptforImpulse(true, false, ucData[2],true);

				ucRet[3]=SELFTEST_PASS;
			}
			else if (ucData[1] == WAKEUP_PIN_READ)
			{
				uiOldTimer = Get_Tmr();
				while(1)
				{
					uiDeltatime=Get_TmrDelta(Get_Tmr(),uiOldTimer);

					if(uiDeltatime >= SELFTEST_BUZZER_DELAY){
						if(g_uiWakeupPinToggleCnt > 0 && g_uiSensorWakeupPinToggleCnt > 0){
							ucRet[3]=SELFTEST_PASS;
						}
						else{
							ucRet[3]=SELFTEST_FAIL;
						}
						printf(" ** cnt :%d  cnt :%d \n\n",g_uiWakeupPinToggleCnt,g_uiSensorWakeupPinToggleCnt);
						break;
					}
				}
			}
			else	ucRet[3]=SELFTEST_FAIL;

			g_uiWakeupPinToggleCnt = 0;
			g_uiSensorWakeupPinToggleCnt = 0;
			ulDataLen = 4;

			break;

		case eSELFTEST_ABR_USIM_REGISTER_IDX:									// 0x4B -> 해외유심
		case eSELFTEST_DOM_USIM_REGISTER_IDX:									// 0x4C -> 국내유심
		{
			unsigned char ucModemCCID[30];
			unsigned char ucDataLength = 0;

			ucRet[1] = ucData[1];
			if(ucData[1] == MODEM_READ_CCID){
				// 1. AT+CCID Read
				// 2. AT+CPBS = "on" (Mode 변경)
				ModemManagerData.nMdmSubState = 0;
				memset(ucModemCCID,0x00,sizeof(ucModemCCID));

				if(SendModemCommand(SelftestProcessRegistUSIM)){

					ucRet[2]=SELFTEST_PASS;

					strcpy((char*)ucModemCCID,(char const*)BkSram_ModemInfo.aCCID);
					ucDataLength = strlen((char const*)ucModemCCID);

					printf("\r\n MODEM GET CCID OK!! \r\n");
					printf(" CCID: %s\n", BkSram_ModemInfo.aCCID);
					printf(" CCID Len : %d\n", ucDataLength);

					printf("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \r\n",\
					ucModemCCID[0],ucModemCCID[1],ucModemCCID[2],ucModemCCID[3],ucModemCCID[4],ucModemCCID[5],ucModemCCID[6],ucModemCCID[7],ucModemCCID[8],ucModemCCID[9],ucModemCCID[10] ,ucModemCCID[11] ,ucModemCCID[12] ,ucModemCCID[13] ,ucModemCCID[14] ,ucModemCCID[15] ,ucModemCCID[16] ,ucModemCCID[17] ,ucModemCCID[18] ,ucModemCCID[19]);

					memcpy(&ucRet[3],&ucDataLength,sizeof(ucDataLength));
					memcpy(&ucRet[4],ucModemCCID,ucDataLength);
					ulDataLen = ucDataLength + 4;

				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n MODEM GET CCID FAIL!! \r\n");
					ulDataLen = 3;
				}
			}
			else if(ucData[1] == MODEM_WRITE_PHONENUM){

				ModemManagerData.nMdmSubState = 2;
				ucDataLength = ucData[2];

				memset(g_ucUSIMServerPhoneNo, 0x00, sizeof(g_ucUSIMServerPhoneNo));
				memcpy(g_ucUSIMServerPhoneNo, &ucData[3],ucDataLength);

				if(ucData[0] == eSELFTEST_DOM_USIM_REGISTER_IDX)		g_uiUSIMCountryCode = 129;		// 국내유심 : 129		  -> AT+CPBW= ,"61455123321",129  전송
				else													g_uiUSIMCountryCode = 145;		// default 해외유심 : 145 -> AT+CPBW= ,"61455123321",145  전송

				if(SendModemCommand(SelftestProcessRegistUSIM)){

					ucRet[2]=SELFTEST_PASS;
					ulDataLen = 3;
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n MODEM SET PHONE NUMBER FAIL!! \r\n");
					ulDataLen = 3;
				}
			}
			else if(ucData[1] == MODEM_READ_PHONENUM)
			{
				// AT+CNUM Read and Data 전송
				ModemManagerData.nMdmSubState = 3;
				if(SendModemCommand(SelftestProcessRegistUSIM)){
					ucRet[2]=SELFTEST_PASS;
					ucDataLength = strlen((char const*)ModemManagerData.aPhoneNo);
					if(ucData[0] == eSELFTEST_DOM_USIM_REGISTER_IDX)			ucRet[3]=ucDataLength;
					else														ucRet[3]=ucDataLength - 1;


					if(ucData[0] == eSELFTEST_DOM_USIM_REGISTER_IDX)			memcpy(&ucRet[4],&ModemManagerData.aPhoneNo[0],sizeof(ModemManagerData.aPhoneNo));
					else 														memcpy(&ucRet[4],&ModemManagerData.aPhoneNo[1],sizeof(ModemManagerData.aPhoneNo));

					ulDataLen = ucDataLength + 3;
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
					printf("\r\n MODEM READ PHONE NUMBER FAIL!! \r\n");
					ulDataLen = 3;
				}

			}
			else{
				ucRet[2]=SELFTEST_FAIL;
				printf("Input Error ucData[0] :%02X, ucData[1] :%02X \r\n", ucData[0],ucData[1]);
				ulDataLen = 3;
			}
			break;
		}
	case eSELFTEST_DEFAULT_SERIAL_WRITE_IDX:

		SelftestDefaultFWInfo();
		ucRet[1]=SELFTEST_PASS;
		ulDataLen = 2;

		break;


#ifdef USEBUZZER
		case eSELFTEST_LED_N_BUZZ_IDX:
			if(ucData[1] == LED_TOGGLE_ON){
				if(ucData[1] == LED_TOGGLE_ON){

					ucRet[2] = SELFTEST_PASS;
					ulDataLen = 3;

					Buzzer_Control(eBUZZER_ON, MSEC(3000),MSEC(0), eSWTimer_ONESHOT);
					g_iTimerled1000msCallback=HalTimerSetSWTimer(500, eSWTimer_INFINITE, SetALLLedOnOff, TRUE);
				}
				else if(LED_TOGGLE_OFF){

					HalTimerStopSWTimer(g_iTimerled1000msCallback);
					HalTimerClearSWTimer(g_iTimerled1000msCallback);
					g_iTimerled1000msCallback = -1;

					SetLedOnOffCtl(LED_ON, eLED_ALL);
					ucRet[2]=SELFTEST_PASS;
				}
				else{
					ucRet[2]=SELFTEST_FAIL;
				}
				ulDataLen = 3;
			}
			else if(LED_TOGGLE_OFF){

				HalTimerStopSWTimer(g_iTimerled1000msCallback);
				HalTimerClearSWTimer(g_iTimerled1000msCallback);
				g_iTimerled1000msCallback = -1;

				SetLedOnOffCtl(LED_ON, eLED_ALL);
				ucRet[2]=SELFTEST_PASS;
			}
			else{
				ucRet[2]=SELFTEST_FAIL;
			}
			ulDataLen = 3;


			break;
#endif
#if 0
		case eSELFTEST_FLIGHTMODE_SET_IDX:

			ModemManagerData.nMdmSubState = 3;
			ModemManagerData.nMdmSubNextState = 0;

			if(SendModemCommand(ProcessSetFlightMode)){
				ucRet[2]=SELFTEST_PASS;
				printf("\r\n MODEM FLGHT MODE OFF SETTING OK!! \r\n");
			}
			else{
				ucRet[2]=SELFTEST_FAIL;
				printf("\r\n MODEM FLGHT MODE OFF SETTING FAIL!! \r\n");
			}
			break;
#endif
        case eSELFTEST_FWVERSION_WRITE_IDX:
        case eSELFTEST_FWVERSION_WRITE_DEFAULT_IDX:
        {
            unsigned int unRcvBootVer, unRcvAppVer;
            unsigned short usCheckSum;
            
      		unRcvBootVer = ucData[2]<<8 | ucData[1];
      		unRcvAppVer  = ucData[4]<<8 | ucData[3];

            printf("Receive -> Boot Ver : 0x%04X, App Ver : 0x%04X\r\n", unRcvBootVer, unRcvAppVer);
                
            FindLastAddressofDB(eFOTA_FILE_TYPE_FIRMWARE_MAIN_BOOT,"BOOTFW",unRcvBootVer,&usCheckSum);
            FindLastAddressofDB(eFOTA_FILE_TYPE_FIRMWARE_MAIN_APP,"APPLFW",unRcvAppVer,&usCheckSum);
            ucRet[1]=SELFTEST_PASS;
            ulDataLen = 2;
        }
            break;
            
		default:
			break;
	}

		if((ucData[0] != eSELFTEST_BUZZ_IDX) && (ucData[0] != eSELFTEST_CURRENT_IDX))
		SendGITPtclResponse((stGIT_PTCL_PAYLOAD*)pInterPtcl, (unsigned char*)&ucRet, ulDataLen, (eCommType)eInCommType);
}

void Git_ModemBoard_Test(void *pInterPtcl, unsigned int eInCommType)
{
	unsigned char ucData[100];

	stGIT_PTCL_PAYLOAD *pPayloadPtcl = (stGIT_PTCL_PAYLOAD*)pInterPtcl;

	memset(ucData,0x00,sizeof(ucData));
	memcpy(ucData, pPayloadPtcl->pPayload, pPayloadPtcl->DataLength-8);


	printf("Data[0] : %02X \r\n",ucData[0]);

	switch(ucData[0])
	{
		case eSELFTEST_HYPERTEC_MODE_START:
			// UART8 (DMA) -> DEBUG (STLink)

#if defined(FEATURE_USE_UART_RX_DMA)
			printf("[%s]SELFTEST: Disble\r\n", __FUNCTION__);
			DisableSELFTESTDMA();
#endif
			stSystemTestInfo.eDebugUartCh = eDEBUG_UART_CH_UART8;
			SystemTestInfoFileWrite((char *)&stSystemTestInfo, sizeof(SYSTEM_TEST_INFO));
			GITDebugPrintf("DBG: new debug port\r\n");

			LoadSettingInfoFile(0);
            SELFTESTUart_Init(BAUDRATE_115200);
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
            InitUSBDeviceClassType();
#endif

			GITDebugPrintf("END\r\n");

			g_bHYPERTECSelftestFlag = true;			// DebugPort Uart7->Uart8로 변경
			g_bYUJINSelftestFlag = true;			// ModemManager 사용안함

			SetOBDState(eOBD_Selftest);
			SetLedOnOffCtl(LED_ON, eLED_ALL);
			SetSysDebugModule(DEBUG_MODULES_SYSTEM_HADNLER,false);
			SetSysDebugModule(DEBUG_MODULES_SYSTEM,false);

			break;
		case eSELFTEST_HYPERTEC_MODE_END:
			// UART7 -> DEBUG
			// UART8 -> PC Serial 통신
			break;
		default:
			break;
	}
}

void RTC_SetTimeRegulate(unsigned char ucYear,unsigned char ucMonth,unsigned char ucDate,unsigned char ucHour,unsigned char ucMinute,unsigned char ucSecnd)
{
	stHalRTC_TimeTypeDef RTC_TimeStructure;
	stHalRTC_DateTypeDef RTC_DateStructure;

	/* Set Date Week/Date/Month/Year */
	RTC_DateStructure.RTC_WeekDay = HAL_RTC_Weekday_Thursday;
	RTC_DateStructure.RTC_Date = ucDate;
	RTC_DateStructure.RTC_Month = ucMonth;
	RTC_DateStructure.RTC_Year = ucYear; // 2016-2000
    HalDrvRtcWrite(eRtcBin, eRtcDate, (char*)&RTC_DateStructure, sizeof(stHalRTC_DateTypeDef), 0);

	/* Set Time hh:mm:ss */
	RTC_TimeStructure.RTC_H12     = HAL_RTC_H24H;
	RTC_TimeStructure.RTC_Hours   = ucHour;
	RTC_TimeStructure.RTC_Minutes = ucMinute;
	RTC_TimeStructure.RTC_Seconds = ucSecnd;
    HalDrvRtcWrite(eRtcBin, eRtcTime, (char*)&RTC_TimeStructure, sizeof(stHalRTC_TimeTypeDef), 0);

	/* Write BkUp DR0 */
    HalDrvRtcIOCtrl(eRtc_IO_SetBackupReg, HAL_RTC_BKP_DR0, NULL, 0, HAL_BKP_DR0_RTC_WAKEUP_VALUE);
}

int ALPU_Normal_Check()
{
	U8 ret = 0,ucBuff[100];
	ret = I2C_Read((unsigned char)0x7a,(unsigned char)0x80,(unsigned char*)ucBuff,10);
	return ret;
}


int MakeDummyFile()
{
	char dummy[10]="Dummy.bin";
	char temp[100];
	int i,ret=0;

	for(i=0;i<74;i++)
	{
		temp[i]=0x30+i;
	}

//	ret=GetVCI2FileWrite(dummy,(U8*)temp,sizeof(temp));
	ret=DummyFileWrite(dummy,(U8*)temp,sizeof(temp));
	return ret;
}

bool makeEncryptionKey(unsigned char *arrSeedKey, unsigned char *arrModuleSerial)
{
	unsigned char ucSeedKey[32];
	unsigned char ucSerial[16];
	unsigned char ucBTMacTemp[12];


	memcpy(ucSeedKey,arrSeedKey,sizeof(ucSeedKey));
	memcpy(ucSerial,arrModuleSerial,sizeof(ucSerial));

	printf("\r\n %s",__FUNCTION__);
	printf("\r\n arrSeedKey : ");
	for(U32 z=0; z<sizeof(ucSeedKey); z++){	printf("%c", ucSeedKey[z]);	}

	printf("\r\n arrModuleSerial : ");
	for(U32 z=0; z<sizeof(ucSerial); z++){	printf("%c", ucSerial[z]);	}

	ucBTMacTemp[0] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[5]&0xF0)>>4);
	ucBTMacTemp[1] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[5]&0x0F));
	ucBTMacTemp[2] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[4]&0xF0)>>4);
	ucBTMacTemp[3] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[4]&0x0F));
	ucBTMacTemp[4] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[3]&0xF0)>>4);
	ucBTMacTemp[5] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[3]&0x0F));
	ucBTMacTemp[6] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[2]&0xF0)>>4);
	ucBTMacTemp[7] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[2]&0x0F));
	ucBTMacTemp[8] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[1]&0xF0)>>4);
	ucBTMacTemp[9] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[1]&0x0F));
	ucBTMacTemp[10] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[0]&0xF0)>>4);
	ucBTMacTemp[11] = HextoUpper((g_LocalBTInfo.bt_addr.btAddr[0]&0x0F));

	printf("\r\n ucBTMacTemp : ");
	for(U32 z=0; z<sizeof(ucBTMacTemp); z++){	printf("%c", ucBTMacTemp[z]);	}


	printf("\r\n serial:%d BT:%d ",sizeof(ucSerial),sizeof(ucBTMacTemp));
	memcpy(&ucSeedKey[0],ucSerial,15);
	memcpy(&ucSeedKey[16],ucBTMacTemp,sizeof(ucBTMacTemp));


	printf("\r\n ucSeedKey : ");
	for(U32 z=0; z<sizeof(ucSeedKey); z++){	printf("%c", ucSeedKey[z]);	}

	memcpy(arrSeedKey,ucSeedKey,sizeof(ucSeedKey));

	return TRUE;

}

unsigned short ModuleKeyDecryption(unsigned char *pMessage, unsigned char *arrModuleKey, unsigned short nLen)
{
	unsigned char Base64Text[AES_TEXT_SIZE];
	unsigned short nBase64Len;
	unsigned char Decryptedtext[AES_TEXT_SIZE];
	unsigned short nEncryptedTextLen;

	DWORD key_schedule[60];
	BYTE iv[1][16] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
	BYTE key[1][32];


	printf("Enter ModuleKeyDecryption()\n");

	//*******************************************************************************************************************
	// 데이터를 Decode base64 한다.
	//*******************************************************************************************************************
  memset((char *)Base64Text, 0x00, AES_TEXT_SIZE);

	if(nLen <= 11) {
		printf("MSG: error message length(%d)\n", nLen);
		return 0;				// 수신 된 암호화 데이터가 존재하지 않는다.
	}

//	if(memcmp((char *)pMessage, "{\"data\"", 7) == 0) {
//	  nBase64Len = nLen - 11;
//	  memcpy((char *)Base64Text, (char *)&pMessage[9], nBase64Len);				// 수신 된 문자열인 {"data":"~~~"} 중에서 ~~~ 만 추출한다.
//	}
//	else {
		nBase64Len = nLen;
	  memcpy((char *)Base64Text, (char *)pMessage, nBase64Len);
//	}

	printf("Rcv Message:\n");
	hexdump(Base64Text, nBase64Len);

	printf("Decode Base64(server)\n");

	if( mbedtls_base64_decode( Decryptedtext, sizeof( Decryptedtext ), (size_t *)&nEncryptedTextLen, Base64Text, nBase64Len ) != 0) {
		printf( "Base64 Decode failed\n" );

		return 0;
	}

	hexdump((char *)Decryptedtext, nEncryptedTextLen);

	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// Key 값 세팅
	// 원래 키값은 "sef99900185seqsef11170327seqeqse" 이다.
	// 하지만, aes_key_setup() 함수에 입력 되는 key 배열에는
	// "9fes0099s581esqe111f2307qes7esq" 와 같이 4바이트 배열을 바꾸어서 적용해야 한다. big endian, little endian 변환
	//*******************************************************************************************************************
	printf("arrModuleKey\r\n");
	memcpy(key[0], arrModuleKey, sizeof(key[0]));
	hexdump(key[0], 32);

	memset((char *)key_schedule, 0x00, 240);
	aes_key_setup_new(key[0], key_schedule, 256);

	printf("Key          :\n");
	hexdump(key[0], 32);

	printf("key_schedule :\n");
	hexdump((char *)key_schedule, 240);

	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// 서버에서 받은 암호화 데이터를 복호화
	//*******************************************************************************************************************
	aes_decrypt_cbc_new(Decryptedtext, nEncryptedTextLen, pMessage, key_schedule, 256, iv[0]);

	printf("Plaintext(server):\n");
	hexdump(Decryptedtext, nEncryptedTextLen);

	nEncryptedTextLen = pkcs7_pad_read((char *)pMessage, nEncryptedTextLen);

	printf("Plaintext(pkcs7):\n");
	hexdump(pMessage, nEncryptedTextLen);

	return nEncryptedTextLen;
}

//******************************************************************************
//******************************************************************************
// Selftest에서 Modem CMD 전송 & 응답 동작 Manager
//******************************************************************************
#if 1
// 2018.09.04 SPARROW : Modem cmd Send Function 구조변경
bool SendModemCommand(modem_process_fn processFunction)
{
	unsigned int	i = 0;
	unsigned int uiRecvLen = 0;
	unsigned char ucTempBuff[1024];

	unsigned int ModemManagerState = eSELFTEST_MD_SET_CMD;

	eMODEM_PROCESS_FUNC_RET ret = (eMODEM_PROCESS_FUNC_RET)eSELFTEST_SUB_STATE_MODEM_READY;

	while(1)
	{
		switch(ModemManagerState){

			case eSELFTEST_MD_SET_CMD:

				ret = processFunction();
				if(ret == eMODEM_PROCESS_FUNC_RET_OK){
					if(g_stModem_Rep.eMDResponse == eMDResponseStateWait){
						printf(" Success!! \r\n");
						return	true;

					}
					else{
						printf(" Fail!! g_stModem_Rep.eMDResponse %d\r\n",g_stModem_Rep.eMDResponse);
						return false;
					}
				}
				else if(ret == eMODEM_PROCESS_FUNC_RET_FAIL)
				{
					printf(" GeMODEM_PROCESS_FUNC_RET_FAIL!\r\n ");
					return false;
				}
				else{
					ModemManagerState = eSELFTEST_MD_SEND_CMD;
				}

				break;
			case eSELFTEST_MD_GET_CMD_RES:
				SelftestProcessMDResponse();

				ModemManagerState = eSELFTEST_MD_SET_CMD;

				break;
			case eSELFTEST_MD_SEND_CMD:
				ModemManagerState = eSELFTEST_MD_RECEIVE_CMD_RES;
				break;
			case eSELFTEST_MD_RECEIVE_CMD_RES:
				uiRecvLen = 0;
				i = eCOMM_TYPE_UART_MODEM;

				if((g_stGitCommInfo[i].eCommSate == eCOMM_STATE_CONNECTED) && (g_stGitCommInfo[i].fnRecvData != NULL))
				{
					 uiRecvLen = g_stGitCommInfo[i].fnRecvData(ucTempBuff, MAX_TEMP_BUFF_SIZE, NULL, (eCommType)i);
					 if ( uiRecvLen ){
						PushMultiDataQueue((stQueue*)g_stGitCommInfo[i].pstInQueue, ucTempBuff, uiRecvLen,(eCommType)i);
					 }
				}
				if ( g_stGitCommInfo[i].pstInQueue != NULL && g_stGitCommInfo[i].fnParsing != NULL )
					g_stGitCommInfo[i].fnParsing(g_stGitCommInfo[i].pstInQueue,
											 GetQueueDataLength(((stQueue*)g_stGitCommInfo[i].pstInQueue)),
											 NULL, (eCommType)i);

				ModemManagerState = eSELFTEST_MD_GET_CMD_RES;
				break;
			default:
				break;
		}
	}
}
#endif

void Send_IGStatus_APP(unsigned char ucVehicleStatus)
{
	stBT_PTCL_PAYLOAD stBTPayload;
	stVEHICLESTATUS_PTCL_PAYLOAD stVehicleState;

	memset(&stVehicleState,0x00,sizeof(stVehicleState));
	memset(&stBTPayload,0x00,sizeof(stBTPayload));

	stBTPayload.FunctionID = 0xF045;
	stVehicleState.ucEngineStatus = ucVehicleStatus;
	stVehicleState.usVehicleStatus = Get_OTCVehicleStatus();

	stBTPayload.pPayload = (unsigned char*)&stVehicleState;


	SendBTPtclResponse((stBT_PTCL_PAYLOAD*)&stBTPayload, (unsigned char*)&stVehicleState, sizeof(stVEHICLESTATUS_PTCL_PAYLOAD), eCOMM_TYPE_UART_BT);
	//OemWriteUartBTBuff((unsigned char*)&stVehicleState, sizeof(stVehicleState), NULL, NULL);
}

#if 0
void SetSendingCount(unsigned char ucCnt)
{
	g_ucSendingCount = ucCnt;
}

unsigned char GetSendingCount(void)
{
	return g_ucSendingCount;
}

void SetSendingBytes(unsigned short usSendingBytes)
{
	g_usSendingBytes = usSendingBytes;
}

unsigned short GetSendingBytes(void)
{
	return g_usSendingBytes;
}

unsigned char SetFileSendingCount(unsigned short usSize, unsigned short usSendingBytes)
{
	unsigned char ucFileSendingCount;
	
	ucFileSendingCount = usSize/usSendingBytes;
		
	if((usSize % usSendingBytes) > 0)
	{
		ucFileSendingCount ++;
	}

	return ucFileSendingCount;
}

unsigned short SetDataFunction(eMODEM_ERROR_STATE eErrorState, unsigned char ucDataIndex, char *pModemFileData)
{
	unsigned short usLength = 0;
	unsigned char ucRemainLength = 0;
	unsigned char ucFileSendingCount = 0;

	char DeniedFile[4]={0xFF,0xFF,0xFF,0x11};
	char ModemNotCommunicationFile[4]={0xFF,0xFF,0xFF,0x22};
	char ModemCmeError[4]={0xFF,0xFF,0xFF,0x33};
	
	usLength = GetSendingBytes();
	ucRemainLength = (sizeof(MODEM_STATE_COLLECTION)*MAX_MODEMSTATEBUFFER_SIZE) % usLength;
	
	ucFileSendingCount = SetFileSendingCount((sizeof(MODEM_STATE_COLLECTION)*MAX_MODEMSTATEBUFFER_SIZE),usLength);
	
	if(ReadAutolinkModemStatusData(eErrorState, (MODEM_STATE_COLLECTION*)g_BTModemStateCollection) == FR_OK)
	{
		DeleteAutolinkModemStatusData(eErrorState);
	}

	if(ucDataIndex == 0xFF)
	{
		switch(eErrorState)
		{
			case MODEM_DENIED:
				memcpy(pModemFileData, DeniedFile, 4);
				break;
			case MODEM_NOT_COMMUNICATION:
				memcpy(pModemFileData, ModemNotCommunicationFile, 4);
				break;
			case MODEM_CME_ERROR:
				memcpy(pModemFileData, ModemCmeError, 4);
				break;
			default:
				break;
		}

		usLength = 4;
	}
	else if(ucDataIndex != (ucFileSendingCount-1))
	{
		memcpy(pModemFileData, &g_BTModemStateCollection[(ucDataIndex)*usLength], usLength);
	}
	else 
	{
		if(ucRemainLength !=0) 
		{
			memcpy(pModemFileData, &g_BTModemStateCollection[(ucDataIndex)*usLength], ucRemainLength);
			usLength = ucRemainLength;
		}
		else
		{
			memcpy(pModemFileData, &g_BTModemStateCollection[(ucDataIndex)*usLength], usLength);
		}
		memset(g_BTModemStateCollection, 0, sizeof(g_BTModemStateCollection));
	}

	return usLength;	
}
#endif

//void SendGITPtclBTDisconnect()
//{
//	stBT_PTCL_PAYLOAD	stBTOutPtcl;
//	eCommType	eInCommType=eCOMM_TYPE_UART_BT;
//	U8 ucBTState=0;
//
//	memset(&stBTOutPtcl,0x00,sizeof(stBTOutPtcl));
//	stBTOutPtcl.FunctionID = 0xF099;
//	stBTOutPtcl.DataLength = sizeof(ucBTState);
//	stBTOutPtcl.pPayload = malloc(ucBTState);
//
//	SendBTPtclResponse(&stBTOutPtcl, (unsigned char*)&ucBTState, sizeof(ucBTState), (eCommType)eInCommType);
//
//	if(stBTOutPtcl.pPayload != NULL)
//	{
//		free(stBTOutPtcl.pPayload);
//		stBTOutPtcl.pPayload = NULL;
//	}
//}
/*****************************END OF FILE****/

