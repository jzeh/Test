/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include "HalHandler.h"
#include "GIT_BluetoothLowEnergy.h"

#include "AutolinkConfiguration.h"
#include "OBD_Manager.h"
#include "OBD_Controller.h"
#include "OBD_Controller_Get.h"
#include "GIT_PassthruDefines.h"
#include "GIT_VCI.h"
#include "GIT_OemInterface.h"
#include "KISA_SEED_ECB.h"
#include "GIT_CanParsingProc.h"
#include "GIT_InterProtocol.h"
#include "Modem_Manager.h"
#include "GIT_Gps.h"
#include "Autolink_Manager.h"
#include "GIT_Util.h"
#include "Power_Manager.h"
#include "GIT_CanParsingProc.h"
#include "HdDebug.h"
#include "MngQueue.h"
#include "MngSystem.h"
#include "HdDebug.h"
#include "DebugHandler.h"
#include "MngSystemUtil.h"
#include "math.h"
#include "Message_Make.h"
#include "MngModem.h"
#include "AutolinkConfig.h"
#include "CanFD_Defines.h"
#include "CanFD_Controller.h"
#include "Share_InterFunction.h"
#ifdef CGW_SECURITY
#include "OBD_CGW_Security.h"
#endif

#if defined(FEATURE_EXTENSION_BOARD)
#include "SysPsExtendHdEvent.h"
#endif

/* Define ------------------------------------------------------------------*/
#if defined(DRIVE_TEST)
#define NOEXTBD
#endif

#define FUEL_LEVEL_CHECK_MAX_ANGLE 4
#define FUEL_LEVEL_CHECK_MIN_ANGLE (-4)
#define MALLOC_MODIFY


//#define DISTANCE
 

#define PASSTHRU_DEFAULT_LENGTH	24

//#define ACTUATOR_LOG
//#define Trace(0,...) 	GITDebug(DEBUG_MODULES_OBD,__VA_ARGS__);

#if defined(PROTOCOL18)
#define AIRCON_TEMP_LOW              0
#define AIRCON_TEMP_HIGH          2550
#endif

//#define OBD_TEST
#define NAVI_DEBUG							1
#define ACTUATOR_PARSING_LOG		2	//LOW CAN DB파싱 로그출력
#define PERIOD_LOG							3
#define ACTUATOR_LOG						4
//#define ACTUATOR_DATA_LOG			5
#define OBD_FINISH_LOG					6
#define DBPARSING_LOG					7
#define SYSTEM_MSG						8
#define CAN_CH1						1
#define CAN_CH2						2
//#define CANFD_BYPASS_RES_WAIT
#if defined(CANFD_BYPASS_RES_WAIT)
#define CANFD_BYPASS_RES_WAIT_TIMEOUT	1000
#endif



#define USE_WAKEUP
//#define CHECK_MALLOC
//#define F_TEMPERATURE	//개발중 정지 RUSSIA 미적용 


#define Trace(x,...)  		{																															\
										if(x ==  NAVI_DEBUG ) 																						\
											GITDebug(DEBUG_MODULES_OBD_NAVI,__VA_ARGS__);									\
										else if( x == ACTUATOR_PARSING_LOG )															\
											GITDebug(DEBUG_MODULES_OBD_ACTUATOR_PARSING_LOG,__VA_ARGS__);	\
										else if( x == PERIOD_LOG )																				\
											GITDebug(DEBUG_MODULES_OBD_PERIOD_LOG,__VA_ARGS__);						\
										else if( x == ACTUATOR_LOG )																			\
											GITDebug(DEBUG_MODULES_OBD_ACTUATOR_LOG,__VA_ARGS__);					\
										else if( x == OBD_FINISH_LOG )																			\
											GITDebug(DEBUG_MODULES_OBD_FINISH_LOG,__VA_ARGS__);							\
										else if( x == DBPARSING_LOG )																			\
											GITDebug(DEBUG_MODULES_OBD_DBPARSING_LOG,__VA_ARGS__);					\
										else if( x == SYSTEM_MSG )																				\
											GITDebug(DEBUG_MODULES_OBD_SYSTEM_MSG,__VA_ARGS__);						\
										else																													\
											GITDebug(DEBUG_MODULES_OBD,__VA_ARGS__);											\
									}

/* Variable ------------------------------------------------------------------*/
stFastFunc					g_stFastFuncData={0,};						//DATA STRUCT
//stSlow1Func					g_stSlow1FuncData={0,};					//DATA STRUCT
//stSlow2Func					g_stSlow2FuncData={0,};                	//DATA STRUCT
//stSlow3Func					g_stSlow3FuncData={0,};                	//DATA STRUCT
//stAdviceFunc				g_stAdviceFuncData={0,};					//DATA STRUCT
//stAEHEVFunc					g_stAEHEVFuncData={0,};         			//AE HEV DATA STRUCT
//stLMFCEVFunc				g_stLMFCEVFuncData={0,};      			//LM FCEV  DATA STRUCT
//stSaveOBDInfo  			g_stSaveOBDdata;								//SAVED STRUCT
stStaticFunc				g_stStaticOBDdata;

eOBD_STATE 						g_eOBDState = eOBD_NONE;			// ODB manager의 초기값
eOBD_STATE 						g_eOBDNextState = eOBD_NONE;			// ODB manager의 초기값


eCAN_COMM_STATE 			g_eCanTxRxState=eCAN_COMM_NONE;
eSET_OFFSET_STATE			g_eSET_OFFSET_STATE = GET_DISTANCE;
eFCS_STATE						g_eFCSState=eFCS_NONE;
eDtcType 							g_eDTCType = eDTC_NONE;
eConsumptionType			eConsumType = eConsum_NONE;
eODO_TYPE							g_eOdoType=ODO_TYPE_NONE;
eFUNCTION_COMM_STATE 	g_Function_State = eCOMM_NONE;
eODO_SUPP_STATE				g_eOdoSuppState = ODO_SUPP_NONE;
eFREEZE_FRAME_STATE		g_eFreezeFrameState = eFREEZE_NONE;
eLAMP_STATE						g_eLampCheckState = eLAMP_STATE_NONE;
eHORN_STATE						g_eHornCheckState = eHORN_STATE_NONE;
extern stCanPacket	g_OutCanPacket,g_OutLCanPacket;
extern unsigned int		g_uiCanWriteMsgLength;
extern stPASSTHRU_MSG		g_stWritePassThruMsg,g_stWritePassThruMsg_L;
extern OBD_CONTROLLER_DATA g_OBDControllerData;
extern float g_fFuelLevelPercent;
extern stCFDControl m_stCFDCtrl;

extern bool g_bFWUpdateFlagFromBLE;
extern bool g_bTPMSConvCheckFlag;

extern stAutolinkConfigData m_stAutolinkConfigData;

extern void SetCanFDInitHandler();
extern eCanFD_STATE GetCanFDMainState();
extern void MeasureGyroAngle();

eCanFDHandlerRsp CanFDInitHandler();
extern void SetCurrentState(int nState);
extern void SetCanFDMainState(eCanFD_STATE eState);
extern unsigned short Get_ExtraChargeTime(void);

#if defined(PROTOCOL18)
extern stAIRCON_PTCL_PAYLOAD g_stAirconPayload;
unsigned char                g_ucDoneBTControlKey[16];
#if defined(REASON_8BYTE)
uint64_t                	 g_ullDoneBTControlKeyReason;
#else
unsigned long                g_ulDoneBTControlKeyReason;
#endif
unsigned char                g_ucDoneBTControlKeyResult;
bool                         g_bCtrlFuncParsingFail=true;
extern stElectricCarData     g_stElectricCarData;
#endif
stBrakeJudder g_stBrakeJudder;

//unsigned long g_ulObdRunningTime=0;						//OBD 구동시간
//unsigned long g_ulSleepReadyTime=0;						//OBD 구동시간
U8 					g_ucGearPos=0, g_ucGearLever=0;
U8 					g_ucDateKey[DCS_DATE_INDEX_SIZE]={0,};
U8 					g_ucDTCCounter[DCS_VEHICLE_SYSTEM_MAX]={0,};
//U8 					g_ucSendDataCnt=0;							//저장 data 전달 갯수
//U8 					g_ucSendDataCycle=1;						//저장 data 전달 주기
//U8 					g_ucDataSendCnt=0;
U8 					g_ucOdoGarbageTotal = 10;
U8 g_ucDoorLockState=0;
//U8 g_ucDoorOpenState=0;
unsigned int g_uiFirstTimeFlag =0;
unsigned int g_uiActCheckFirstTimeFlag =0;
unsigned int g_uiActCheckTime = 0;
unsigned char g_ucActRetry=0;
unsigned int g_uiActFailReason=eRETURNFAIL_TYPE_NONE;
unsigned int g_uiEngContinueSendTimer=0;
unsigned int g_uiEngStopTimer = 0;
unsigned int g_ui1secTimer = 0;
//unsigned int g_uiTempTimer = 0;
unsigned int g_uiEnergyOldTime = 0;
unsigned int g_uiFuelOldTime = 0;
unsigned int g_uiCalFuelConsumptionOldTime = 0;
stIndicatorCheck g_stIndicatorCheck[ePOS_INDICATOR_TIMER_MAX];

bool g_bObdRcvSleepFlag=false;
bool g_bEngRunContinueSuppFlag=false;
bool g_bEngRunKeepFlag=false;//엔진구동명령을 쏴서 성공하고 유지코드가 필요하면 true
bool g_bEngRunActFlag=false;	//엔진구동명령을 쏴서 성공하면 true
bool g_bFATCRunActFlag=false;	//EV_NONE_READY타입에서 성공하면 true
bool g_bCanDataFlag_Wakeup=false;
bool g_bCtrlDBSetFlag =false;
bool g_bCanLineStopFlag=false;
bool g_bFCSRunFlag=true;
bool g_bIndicatorFirstCheckFlag=true;
bool g_bFreezeFrameRunFlag=true;
bool g_bAutoVINRunFlag=true;
bool g_bDBParsingFailFlag=false;
bool g_bDBParsingFailFlag_Act=false;
//bool g_bWakeUpEndFlag=false;
bool g_bWakeUpSettingFlag=false;
bool g_bOBDSleepIntoFlag=true;
bool g_bLampCheckFlag = false;
bool g_bNoResponseFlag = false;	//내부적실행이므로 응답보내지 않을때 사용
bool g_bFuelLevelCheckFlag = false;						//연료잔량을 체크하기위한 플래그
bool g_bSOHCheckFlag = false;						//SOH를 체크하기위한 플래그
bool g_bFuelLevelCheckOneTimeRunFlag = false;	//한번만 체크하기위한 플래그
bool g_bFuelLevelSwitchedPercentFlag = false;	//한번만 체크하기위한 플래그
bool g_bFuelLiterTypeFlag = false;							//연료잔량을 계산하는타입 플래그
bool g_bIndicatorDBFlag = false;							//경고등 DB인지 확인하는 플래그
bool g_bFuelLevelLiterTypeFlag = false;					//연료잔량을 정차중에 얻어오는타입
bool g_bKeyTypeDBFlag = false;
#if defined(PROTOCOL18)
bool g_bSOHTypeDBFlag = false;
#endif
bool g_bBrakeJudderFirstFlag = true;
unsigned char g_ucFuelLevelMaxLiter=0;
float g_fFuelFcevMax=0;
double g_dChecked_Fuel_Consume=0;					//연료잔량 계산을 위해 적산하는 연료소모량
unsigned char g_ucDTCTotalCnt = 0;	//최초 IG ON 시 DTC가 있었으면 MIL LAMP가 ON상태여도 DTC알람 띄울필요없으므로 체크용
unsigned char g_ucHornCheckFlag = 0;
//bool g_bVehicleCheckEndFlag=false;
extern bool gbExistRemoteControlMessage;
unsigned int g_uiEngRunStartTime=0;
//unsigned int g_uiObdManagerSleepTimer=0;
unsigned int g_ContinueCodeTiming = 100;
unsigned int g_EngOnSetTime = ACTUATOR_MAX_ENGONTIME;
char g_cSavedReadDefoggerState=0;
char g_cSavedDefrostState=0;
short g_sSavedTemperature=0;
unsigned char g_ucSystemIndexFFinfo=0;	//프리즈프레임필요한시스템중 몇번째 시스템인지(DTC가있는 시스템중 몇번째 시스템인지)
unsigned char g_ucDTCIndex=0;
unsigned char g_ucSystemCount=0;		//DTC가 있는 시스템의 갯수
unsigned char g_ucFreezeFrameData[DTC_DIVIDE_MEMORY_SIZE];	//1000정도가 한계 모뎀 인코딩시 1500byte를 못넘음
stControlConfig g_stControlConfig;
stReportSmartReq g_stReportSmartReq;
int g_iTimerCan1msCallback = -1;
int g_iTimerFuelCheck30secCallback = -1;
unsigned char g_ucDBParsingFlag=0;	//DB파싱은 최초 한번만(슬립중 처음으로 돌아와도 새로하지않음)
unsigned int g_uiFotaTimer=0;
unsigned char g_ucSaveSlaveCanLine=0;

int g_nSumAnglex =0, g_nSumAngley=0, g_nAngleSumCount=0; // woong bae 18/11/26

void SetRequestDtcAlram(boolean_t bRequestDtcAlram);

//#define ENGINE_LOG // else if( eOBDState == eOBD_Actuator_Mode || ( eOBDState == eOBD_Running_Info_Mode && Get_VehicleStatus()<eVEHICLE_STATE_IGON ))	//제어DB일때 타는곳

#if defined(CARB_ODO)
U32 					g_uiDistanceafterClearDTC;
#endif

U8 									*g_pSuppRequest = NULL;
U8	g_ucSuppTableNum=0;
//stSupportData g_ucSupportedData;
//stSupportData g_ucSupportedOldData[5];

stMasterData	 				*g_pstFastFunction = NULL;			//FAST MASTER
stMasterData	 				*g_pstSlow1Function = NULL;			//SLOW1 MASTER
stMasterData	 				*g_pstSlow2Function = NULL;			//SLOW2 MASTER
stMasterData	 				*g_pstSlow3Function = NULL;			//SLOW3 MASTER
stMasterData	 				*g_pstSuppFunction = NULL;			//Suppoted SLAVE
stSlaveData					*g_pstCurrDataBase = NULL;         	//Current SLAVE
stSlaveDTCData				*g_pstDTCDataBase = NULL;         	//DTC SLAVE
stDTCFunc					*g_stDTCFuncData = NULL;           	//DTC DATA STRUCT
stFunctionRequestCnt 	stRequestCnt[eCOMM_MAX];
stReferenceTable 			*g_psrFastRefTable = NULL;			//FAST 참조테이블
stReferenceTable 			*g_psrSlow1RefTable = NULL;			//SLOW1 참조테이블
stReferenceTable 			*g_psrSlow2RefTable = NULL;			//SLOW2 참조테이블
stReferenceTable 			*g_psrSlow3RefTable = NULL;			//SLOW3 참조테이블
stReferenceTable 			*g_psrSuppRefTable = NULL;			//SUPPORTED 참조테이블

//원격구동 데이터
stActuatorReqData			g_stActuatorReqData[ACTUATOR_MAX_REQ_CNT];
stActuatorConvertData		g_stActuatorConvertData[ACTUATOR_MAX_CONVERT_CNT];
stActuatorResData			g_stActuatorResData[ACTUATOR_MAX_RES_CNT];
stActuatorReadyData		g_stActuatorReadyData[ACTUATOR_MAX_READY_CNT];
stActuatorFreezeData		g_stActuatorFreezeData[FREEZE_FRAME_SYS_CNT_MAX];	//DB파싱한 데이터

#if defined(PROTOCOL18)
stActuatorAirConData        g_stActuatorAirConData[ACTUATOR_MAX_AIR_CNT]; //dahae
#if defined(F_TEMPERATURE)
stActuatorAirConData        g_stActuatorAirCon_F_Data[ACTUATOR_MAX_AIR_CNT]; //dahae
#endif
stActuatorCtrlData		    g_stActuatorCtrlData[ACTUATOR_MAX_CTRL_CNT];//dahae
#endif

stActuatorReqInfo			g_stActuatorReqInfo[ACTUATOR_MAX_REQ_CNT];	//Req ID와 ID별갯수
stActuatorResInfo			g_stActuatorResInfo[ACTUATOR_MAX_RES_CNT];	//Res ID와 ID별갯수
stActuatorReadyInfo		g_stActuatorReadyInfo;	//Res ID와 ID별갯수
stActuatorConvertInfo		g_stActuatorConvertInfo;	//Res ID와 ID별갯수
//stActuatorFreezeInfo		g_stActuatorFreezeInfo;	//Res ID와 ID별갯수
#ifndef NO_FREEZE_FRAME
stActuatorFreezeInfo		g_stActuatorFreezeInfo[FREEZE_FRAME_SYS_CNT_MAX];	//시스템별 프리즈프레임에 대한 정보
#endif

stDistanceInfo				g_stDistanceInfo;
unsigned char g_ucTotReqCnt=0;	//Req의 총 라인수 ACTUATOR_MAX_REQ_CNT 40
unsigned char g_ucTotConvertCnt=0;	//Convert의 총 라인수 ACTUATOR_MAX_CONVERT_CNT 30
unsigned char g_ucTotFreezeCnt=0;	//Freeze 총 라인수 FREEZE_FRAME_SYS_CNT_MAX 15
unsigned char g_ucTotResCnt=0;	//Res의 총 라인수 ACTUATOR_MAX_RES_CNT 60
unsigned char g_ucTotReadyCnt=0;	//Ready의 총 라인수

#if defined(PROTOCOL18)
unsigned char g_ucTotAirCnt=0;  //dahae  AirCon의 총 라인수
unsigned char g_ucTotCtrlCnt=0; //dahae Control의 총 라인수
#endif
#if defined(F_TEMPERATURE)
unsigned char g_ucTotAirCnt_F=0;
#endif

unsigned char g_ucReqIndex=0;	//ReqInfo의 index(REQ세트의 수 ID의 갯수아님)
unsigned char g_ucResIndex=0;	//ResInfo의 index(RES세트의 수 ID의 갯수아님)
unsigned char g_ucSecquence=0;	//REQ메시지 루프용 변수(REQLIST의 몇번째 REQ)
unsigned int g_uiReqCount=0;	//REQ메시지 루프용 변수(현재REQ를 몇번 보냈는지)
unsigned char g_ucRetryCount=0;	//REQ메시지 Retry
bool g_bActuRunningFlag=false;	//한 REQ 세트에대한 flag, g_uiFirstTimeFlag와 용도다름
unsigned char g_ucActuatorStatus=0;	//구동결과
unsigned char g_ucReqIndexPos=0, g_ucResIndexPos=0, g_ucEngContinueReqIndexPos=0;
eACTUATOR_TYPE 		g_eActuatorType=ACTUATOR_TYPE_NONE;	//구동종류
eACTUATOR_STATUS	g_eActuatorStatus=ACTUATOR_STATUS_NONE;	//구동결과
unsigned int g_uiStatusCheckTime=0;
unsigned int g_uiCanStopTime=0;
unsigned int g_uiCanStoploopTime=0;
unsigned int g_uiEngStartTimeOutTimer=0;	//엔진런 명령을 받고 비클체크에 들어온 시간으로 계산
unsigned int g_uiEngStartDefaultTimer=0;	//기본적으로 30초를 대기타야 구동가능
unsigned char g_ucActuatorModeStep=0;

bool g_bAirConRunFlag = true;
bool g_bRearDefogDBInFlag = false;
bool g_bRearDefogRunFlag = false;
bool g_bTPMSConvertFlag = false;
//bool g_bLockCtrlFlag=false;

bool g_bRunningInfoFirstTimeFlag=true;
stSlaveHWSetInfo g_stSlaveHWSetInfo;
stSlaveHWSetInfo g_stControlHWSetInfo;
//stSlaveHWSetInfo g_stCH2PassMaskIDInfo;
stCanIDCheckList g_stCanIDCheckList;
stCanIDCheckList g_stCanIDCanFDUse;

stSlaveDBCount g_stSlaveDBCount;

eFUEL_TYPE					g_eFuel_Type = ETC;
eFCSMode 					g_eFCSMode = eFCS_MODE_NONE;
eCmdInput						eCommanddInput = eCMD_NONE;

U16	g_usFastTableCnt=0;				//ref table count
U16	g_usSlow1TableCnt=0;			//ref table count
U16	g_usSlow2TableCnt=0;			//ref table count
U16	g_usSlow3TableCnt=0;			//ref table count
U16	g_usCurrentCnt=0;				//slave(current) count
U8	g_ucDtcSystemCnt=0;					//slave(dtc) count
U16	g_usMasterCnt[5]={0,};			//master(navi) count
U32	g_uiDiagnosisRxCanid[10];
U8 	g_ucCylinder=0;					//기통
U8 	*g_pDTCRequest = NULL;
U8 	*g_pFastRequest = NULL;
U8 	*g_pSlow1Request = NULL;
U8 	*g_pSlow2Request = NULL;
U8 	*g_pSlow3Request = NULL;
unsigned int g_uiActStartTime=0;

bool 				g_bAuto_VIN_Retry=FALSE;
//U8					g_AUTOVIN[60]={0,};
stAutoVin			g_stAutoVin;
//U8					g_ucVINCnt=0;
float 				g_fDisplace=0;				//배기량
BOOL 				*g_pDTCState = NULL;
bool 				g_bFineCanRecieveStart=FALSE;
unsigned int g_uiTpmsAlramValueSave=0;
unsigned int g_uiTpmsAlramValue=0;
unsigned char g_ucCanLine = 0;
//공회전/워밍업
bool g_bEngineIdleFlag = FALSE;                      /* 공회전 시작 flag */
bool g_bWarmupFlag = FALSE;                         /* 워밍업 시작 flag */

// 평균속도
#define  DEFAULT_RAPID_ACCEL_DECEL_DIFF 11                         /* 급가속, 급감속 DIFF 비교 값*/
#define FUELLEVELCHECK_TIMEOUT_TIME	(5000)
#define SOHCHECK_TIMEOUT_TIME	(5000)
#define BRAKEJUDDERCHECK_TIMEOUT_TIME	(5000)

//INT32U g_uiAccState = 0;


//주행거리
INT32U g_uiVSSOldTime;                                  /*  */


void fuelConsume();
extern void SetVINCode(U8 * inputVIN);
extern void GetStartingDateTimeStamp(uint8_t *ptr, stHalRTC_DateTypeDef DateStamp, stHalRTC_TimeTypeDef TimeStamp);
extern uint16_t key_display(long long llValue);
extern int8_t *GetStringFromAlramEvent(int32_t nId);
extern int8_t *GetStringFromId(int32_t nId);
extern int8_t *GetStringFromEvent(int32_t nMode,int32_t nEvent,int32_t nSubEvent);
extern stLockState g_stLockState;
extern int	g_iTimerVSS1secCallback;
extern unsigned char g_ucFuelListCount;
extern unsigned char g_ucFuelListPosition;
extern unsigned char g_ucFuelList[FUEL_LIST_MAX_CNT];
extern bool g_bMILLampAlramFlag;

extern void Send2MngSys2(stMsgSys* pstMsgSys);
extern uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
extern uint32_t GetUTCTime();
void ShowSmartkeyAction(stCarReport report);
void Set_GPS_Lat_Origin(double lat);
void Set_GPS_Lon_Origin(double lon);
void SendObdConfig2System(uint32_t unOdometer, double dlLatitude, double dlLongitude, uint8_t ucDoorLockStatus, uint8_t ucDoorOpenStatus, uint8_t ucHeadLight, float fReaminedFuel);

extern boolean_t GetForwardingSmartKey2BtFlag();

extern double Get_GPS_Lat_Origin();
extern double Get_GPS_Lon_Origin();
extern unsigned long long g_ullIndicator;
extern unsigned char g_ucTCUSlopeAngleArrIndex;
extern eFUEL_TYPE	g_eFuel_Type;
extern unsigned char g_ucCylinder;		//기통
extern float g_fDisplace;
extern int	g_iTimerIndicator3secCallback;
extern bool g_bIndicator3secCallbackRunningFlag;
#ifdef CGW_SECURITY
unsigned char g_unCGWSeed[8];
#endif
stDieselFuelConsume g_stDieselFuelConsume;
float g_fVolumetricEfficiency=80.0;
float g_fMassAirFlow;
eMapMaf	g_eMapMaf = eMAPMAF_NONE;
float g_fEquivalenceRatio=1.0;
eACTUATOR_TYPE g_eActaveType = ACTUATOR_TYPE_NONE;
//	void fuelConsume();
void InjectionQuantity();

int g_uiWaitOBD_Idletime = 0;

ePACV_TYPE g_ePACV_Type = PA;
eCanBaudrate g_eCanBaudrate = eCAN_500KBPS;

void Current_ActuatorReady(stActuatorReadyData * pstCurrDataBase, int i);
void Current_ActuatorRes(stActuatorResData * pstCurrDataBase, int i);
void Current_SlaveData(stSlaveData * pstCurrDataBase, int i);
#if 1 // James Jean 2018/11/14
unsigned int g_uiAutoVinReqPacketIdx = 0;
eAUTOVIN_STATE g_eAutoVinReqFuelType = eAUTOVIN_ENGINE_REQ;
eCV_AUTOVIN_STATE g_eCVAutoVinReqFuelType = eAUTOVIN_CV_UNIVERSE_REQ;
bool g_bAutoVinNegativeResponse = FALSE;
#endif
extern stHalCANTX_STRUCT   g_CAN1_TxBuffCtrl;
extern stHalCANRX_STRUCT   g_CAN1_RxBuffCtrl;
extern stHalCANTX_STRUCT   g_CAN2_TxBuffCtrl;
extern stHalCANRX_STRUCT   g_CAN2_RxBuffCtrl;


extern unsigned int g_unBkramTmpOdometer;
extern unsigned int g_unBkramClearOdoFlag;

/* ---------------------------------------------------------------------------*/

//void InitializeOBDManager(void)
//{
//	SetOBDState(eOBD_NONE);
//	return;
//}

long long m_llObdDrivingKey=0;


stEvAirConControl g_stEvAirConControl;



extern unsigned int Get_Odmeter(void);


//void SetObdDrivingKey(long long value)
//{
//    Trace(0,"##########################################\r\n");
//    Trace(0,"%s] Set Driving Key : %x\r\n",__FUNCTION__,value);
//    Trace(0,"##########################################\r\n");
//    m_llObdDrivingKey = value;
//
//    key_display(m_llObdDrivingKey);
//}
//
//void GetObdDrivingKey(long long* value)
//{
//    *value = m_llObdDrivingKey;
//}
void Display_ReportDriving()
{
#if defined(PERIOD_LOG)
    stHalRTCTypeDef stHalRtcDateTime;
	uint8_t arrTemp[64];
    long long llValue=0;

	Trace(PERIOD_LOG,"!!!!!!!!!!!!!!!!!!!!!!!!!!  PERIOD !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
	Trace(PERIOD_LOG," 0. VehicleStatus                           : %d\r\n", Get_VehicleStatus());
	Trace(PERIOD_LOG," 1. currentSpeed                           : %d\r\n", Get_Speed());
	Trace(PERIOD_LOG," 2. RPM                                    : %d RPM\r\n", Get_RPM());
	Trace(PERIOD_LOG," 2-1. Motor RPM                            : %d RPM\r\n", Get_MotorRPM_Status());
    Trace(PERIOD_LOG," 3. MaxSpeed                               : %d\r\n", Get_MaxSpeed());
	Trace(PERIOD_LOG," 4. MaxRPM                                 : %d MaxRPM\r\n", Get_MaxRPM());
	Trace(PERIOD_LOG," 4-1. Motor MaxRPM                         : %d MaxRPM\r\n", Get_MotorMaxRPM());
    Trace(PERIOD_LOG," 5. Fuel_Consume                           : %f ml\r\n", Get_Total_Fuel_Consume());
	Trace(PERIOD_LOG," 6. Driving_distance                       : %d m\r\n", Get_driving_distance_int());
	Trace(PERIOD_LOG," 7. period_Rapid_acceleration_Counter      : %d\r\n", Get_period_Rapid_acceleration_Counter());
	Trace(PERIOD_LOG," 8. period_Rapid_Deceleration_Counter      : %d\r\n", Get_period_Rapid_Deceleration_Counter());
	Trace(PERIOD_LOG," 9. Get_GPS_Vailication                    : %d\r\n", Get_GPS_Vailication());
	Trace(PERIOD_LOG,"10. GPS_SatellitesNum                      : %d\r\n", Get_GPS_SatellitesNum());
	Trace(PERIOD_LOG,"11. Get_GPS_Lat                            : %f\r\n", Get_GPS_Lat() );
	Trace(PERIOD_LOG,"12. Get_GPS_Lon                            : %f\r\n", Get_GPS_Lon() );
	Trace(PERIOD_LOG,"14. Get_TPMS_RR                            : %d\r\n", Get_TPMS1(eTPMS_TIRE_STATE_RR));
	Trace(PERIOD_LOG,"15. Get_TPMS_RL                            : %d\r\n", Get_TPMS1(eTPMS_TIRE_STATE_RL));
	Trace(PERIOD_LOG,"16. Get_TPMS_FR                            : %d\r\n", Get_TPMS1(eTPMS_TIRE_STATE_FR));
	Trace(PERIOD_LOG,"17. Get_TPMS_FL                            : %d\r\n", Get_TPMS1(eTPMS_TIRE_STATE_FL));
	Trace(PERIOD_LOG,"18. Battery                                : %f V\r\n", Get_Battery());
	Trace(PERIOD_LOG,"19. RSSI                                   : %-d dBm\r\n", ModemManagerData.ndBm);
	Trace(PERIOD_LOG,"20. Door lock                              : %d\r\n", Get_DoorLock());
	Trace(PERIOD_LOG,"21. Door open                              : %d\r\n", Get_DoorOpen());
	Trace(PERIOD_LOG,"22. Get_ACC                                : %x\r\n", Get_ACC());
	Trace(PERIOD_LOG,"23. total distance                         : %d km\r\n", Get_Odmeter());
	Trace(PERIOD_LOG,"24. Get_BT_Connection_State                : %d\r\n", Get_BT_Connection_State());
	Trace(PERIOD_LOG,"25. Remain_Fuel_Percent                    : %hd\r\n", Get_Remain_Fuel_Percent());
	Trace(PERIOD_LOG,"27. tailLamp                               : %d\r\n", Get_TailLamp_State());
	Trace(PERIOD_LOG,"28. MILLamp_State                          : %d\r\n", Get_MILLamp_State());
	Trace(PERIOD_LOG,"29. mileage                                : %f\r\n", Get_MileAge_avg());
	Trace(PERIOD_LOG,"30. Period_Speed_Counter_0  : %d\r\n", Get_Period_Speed_Counter_0());
	Trace(PERIOD_LOG,"30. Period_Speed_Counter_from_0_under_10   : %d\r\n", Get_Period_Speed_Counter_from_1_under_10());
	Trace(PERIOD_LOG,"31. Period_Speed_Counter_from_40_under_50  : %d\r\n", Get_Period_Speed_Counter_from_41_under_50());
	Trace(PERIOD_LOG,"32. Period_Speed_Counter_from_80_under_90 : %d\r\n", Get_Period_Speed_Counter_from_81_under_90());
	Trace(PERIOD_LOG,"33. Period_Speed_Counter_from_120_under_130: %d\r\n", Get_Period_Speed_Counter_from_121_under_130());
	Trace(PERIOD_LOG,"34. Period_Speed_Counter_over_160          : %d\r\n", Get_Period_Speed_Counter_over_141());
	Trace(PERIOD_LOG,"28. D0          : %d\r\n", Get_PeriodDrivePattern0());
	Trace(PERIOD_LOG,"28. D1          : %d\r\n", Get_PeriodDrivePattern1());
	Trace(PERIOD_LOG,"28. D2          : %d\r\n", Get_PeriodDrivePattern2());
	Trace(PERIOD_LOG,"28. D3          : %d\r\n", Get_PeriodDrivePattern3());
	Trace(PERIOD_LOG,"28. D4          : %d\r\n", Get_PeriodDrivePattern4());
	Trace(PERIOD_LOG,"35. idletime %d\r\n",Get_EngineIdleTime());
	Trace(PERIOD_LOG,"36. warmtime %d\r\n",Get_WarmupTime());
	Trace(PERIOD_LOG,"36. fBMSBattChargeStatus %f\r\n",Get_SOC_Status());
	Trace(PERIOD_LOG,"36. fBatteryVoltage %f\r\n",Get_Battery());
	Trace(PERIOD_LOG,"36. sMotorRPM %f\r\n",Get_MotorRPM_Status());
	Trace(PERIOD_LOG,"F36. H2_Fuel_Status %f\r\n",	Get_Hydrogen_Fuel_Status());
	Trace(PERIOD_LOG,"F37. H2_AirPurif %f\r\n",		Get_AirPurification_Status());
	Trace(PERIOD_LOG,"F38. H2_CO2Reduce %f\r\n",	Get_CO2Reduction_Status());
	Trace(PERIOD_LOG,"F06. H2_Temperature %d\r\n",	Get_HydrogenCurrentTemperature_Status());
	Trace(PERIOD_LOG,"F11. H2_TankPress %f\r\n",	Get_HydrogenTankPress_Status());
	Trace(PERIOD_LOG,"F12. H2_ChargeCnt %d\r\n",	Get_HydrogenChargeCnt_Status());
    Trace(PERIOD_LOG,"F16. H2_Fuel_Consume %f\r\n", Get_Total_Fuel_Consume());
	Trace(PERIOD_LOG,"B03. fBMSBattVolt %f\r\n",	Get_BatteryPackV_Status());
	Trace(PERIOD_LOG,"B02. fBMSBattCurr %f\r\n",	Get_BatteryPackC_Status());
	Trace(PERIOD_LOG,"F32. FuelcellVolMin %f\r\n",	Get_MinBatteryCellVoltage_Status());
	Trace(PERIOD_LOG,"F33. FuelcellVolMax %f\r\n",	Get_MaxBatteryCellVoltage_Status());
	Trace(PERIOD_LOG,"!!!!!!!!!!!!!!!!!!!!!!!!!!  finish  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");

    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRtcDateTime), 0);
	GetDateTimeStamp(arrTemp,stHalRtcDateTime.RtcDate,  stHalRtcDateTime.RtcTime);
	llValue = atoll((char *)arrTemp);
	Trace(PERIOD_LOG,"0. Get_DrivingKey : %lld \r\n", Get_DrivingKey());
	Trace(PERIOD_LOG,"2. Create TIME    :  %lld\r\n", llValue);
	Trace(PERIOD_LOG,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
#endif
}
int ReportDriving()
{
    /* do gethering data all variables */
    // send to message mananger
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));


	HalTimerStopSWTimer(g_iTimerVSS1secCallback);
	
    msg.rpInterval.DrivingInfo.DrivingInfoB1.AccStatus = Get_VehicleStatus();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.CarBattery = (uint16_t)(Get_Battery() * 10);
    msg.rpInterval.DrivingInfo.DrivingInfoB1.BleStatus=Get_BT_Connection_State();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.RemainedFuel = Get_Remain_Fuel_Percent();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.DoorLockStatus = Get_DoorLock();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.DoorOpenStatus = Get_DoorOpen();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.DrivingDistance = Get_driving_distance_int();
    
    msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsLatitude = Get_GPS_Lat();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsLongitude = Get_GPS_Lon();   

    if( g_OBDControllerData.monitering_Data.GpsListCount == 0 )
        msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsListCount = 0;
    else
        msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsListCount = g_OBDControllerData.monitering_Data.GpsListCount;
    
	memcpy( msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsLatitudeList, g_OBDControllerData.monitering_Data.GpsLatitudeList, sizeof(msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsLatitudeList) );
	memcpy( msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsLongitudeList, g_OBDControllerData.monitering_Data.GpsLongitudeList, sizeof(msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsLongitudeList) );
    msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsSateliteCount = Get_GPS_SatellitesNum();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsValid = Get_GPS_Vailication();
#if defined(PROTOCOL16)
	if(FUELTYPE_GET_STATE() == ELECTRONIC || FUELTYPE_GET_STATE() == EV_NONE_READY )
	{	//기존과의 호환성을위해...
		msg.rpInterval.DrivingInfo.DrivingInfoB1.Rpm = Get_MotorRPM_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.MaxRpm = Get_MotorMaxRPM();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.ConsumedFuel = Get_Total_Energy_Consume();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.FuelEffiency = Get_ElectronicMileAge_avg();
	}
	else
	{
		msg.rpInterval.DrivingInfo.DrivingInfoB1.Rpm = Get_RPM();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.MaxRpm = Get_MaxRPM();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.ConsumedFuel = Get_Total_Fuel_Consume();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.FuelEffiency = Get_MileAge_avg();	
	}
	msg.rpInterval.DrivingInfo.DrivingInfoB1.MotorRpm = Get_MotorRPM_Status();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.MaxMotorRpm = Get_MotorMaxRPM();		
	msg.rpInterval.DrivingInfo.DrivingInfoB1.ConsumedBattery = Get_Total_Energy_Consume();
#if defined(PROTOCOL17)
	msg.rpInterval.DrivingInfo.DrivingInfoB1.ConsumedBattery_Regen = Get_Total_Energy_Consume_Regen();
#endif
	msg.rpInterval.DrivingInfo.DrivingInfoB1.RemainedFuel = Get_Remain_Fuel_Percent();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.RemainedECarBattery = (uint8_t)Get_SOC_Status();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SOH = (uint8_t)Get_SOH_Status();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.ExtraDrivingDistance = Get_ExtraDrivingDistance_Status();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.HighBatteryTemperature = Get_HighBatteryTemperature();
#if defined(PROTOCOL18)
	msg.rpInterval.DrivingInfo.DrivingInfoB1.HighBatteryTemperatureMax = Get_HighBatteryTemperatureMax(); //dahae
#endif
	msg.rpInterval.DrivingInfo.DrivingInfoB1.OutsideTemperature = Get_OutsideTemperature();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.ElectronicEffiency = Get_ElectronicMileAge_avg();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.GpsDirection = Get_GpsDirection();
#else //PROTOCOL16
    msg.rpInterval.DrivingInfo.DrivingInfoB1.RemainedECarBattery = (uint8_t)Get_SOC_Status();
    if( FUELTYPE_GET_STATE() == ELECTRONIC || FUELTYPE_GET_STATE() == EV_NONE_READY )
    {
		msg.rpInterval.DrivingInfo.DrivingInfoB1.ConsumedFuel = Get_Total_Energy_Consume();
        msg.rpInterval.DrivingInfo.DrivingInfoB1.MaxRpm = Get_MotorMaxRPM();
        msg.rpInterval.DrivingInfo.DrivingInfoB1.Rpm = Get_MotorRPM_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.FuelEffiency = Get_ElectronicMileAge_avg();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.ElectronicEffiency = Get_ElectronicMileAge_avg();
    }
    else
    {
		msg.rpInterval.DrivingInfo.DrivingInfoB1.ConsumedFuel = Get_Total_Fuel_Consume();
        msg.rpInterval.DrivingInfo.DrivingInfoB1.MaxRpm = Get_MaxRPM();
        msg.rpInterval.DrivingInfo.DrivingInfoB1.Rpm = Get_RPM();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.FuelEffiency = Get_MileAge_avg();
    }
#endif //PROTOCOL16

    msg.rpInterval.DrivingInfo.DrivingInfoB1.MaxSpeed = Get_MaxSpeed();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.MILLamp = Get_MILLamp_State();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.Odometer = Get_Odmeter();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.OperationKey = Get_DrivingKey();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.RapidAccelCount = Get_period_Rapid_acceleration_Counter();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.RapidDecelCount = Get_period_Rapid_Deceleration_Counter();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.EngRunTime = GetUTCTime()-Get_DriveStartTime_UTC();
#if defined(PROTOCOL17)
	msg.rpInterval.DrivingInfo.DrivingInfoB1.EngineIdleTime = Get_EngineIdleTime();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.WarmUpTime = Get_WarmupTime();
#endif
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCnt0km		= Get_Period_Speed_Counter_0();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper1kmUnder10km		= Get_Period_Speed_Counter_from_1_under_10();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper11kmUnder20km	= Get_Period_Speed_Counter_from_11_under_20();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper21kmUnder30km	= Get_Period_Speed_Counter_from_21_under_30();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper31kmUnder40km	= Get_Period_Speed_Counter_from_31_under_40();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper41kmUnder50km	= Get_Period_Speed_Counter_from_41_under_50();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper51kmUnder60km	= Get_Period_Speed_Counter_from_51_under_60();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper61kmUnder70km	= Get_Period_Speed_Counter_from_61_under_70();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper71kmUnder80km	= Get_Period_Speed_Counter_from_71_under_80();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper81kmUnder90km	= Get_Period_Speed_Counter_from_81_under_90();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper91kmUnder100km	= Get_Period_Speed_Counter_from_91_under_100();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper101kmUnder110km = Get_Period_Speed_Counter_from_101_under_110();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper111kmUnder120km = Get_Period_Speed_Counter_from_111_under_120();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper121kmUnder130km = Get_Period_Speed_Counter_from_121_under_130();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper131kmUnder140km = Get_Period_Speed_Counter_from_131_under_140();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.SpdCntUpper141km = Get_Period_Speed_Counter_over_141();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.IdlingCount	 				= Get_PeriodDrivePattern0();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.StopCount 					= Get_PeriodDrivePattern1();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.BreakeCount 				= Get_PeriodDrivePattern2();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.InertiaDrivingCount	 	= Get_PeriodDrivePattern3();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.NormalDrivingCount	= Get_PeriodDrivePattern4();
#if defined(PROTOCOL17)
	msg.rpInterval.DrivingInfo.DrivingInfoB1.ExtraDrivingCount		= Get_PeriodDrivePattern5();
#endif
    msg.rpInterval.DrivingInfo.DrivingInfoB1.Speed = Get_Speed();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.TailLanmp = Get_TailLamp_State();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.HeadLight = Get_HeadLamp_State();
    msg.rpInterval.DrivingInfo.DrivingInfoB1.TpmsFL = Get_TPMS1(eTPMS_TIRE_STATE_FL);
	msg.rpInterval.DrivingInfo.DrivingInfoB1.TpmsFR = Get_TPMS1(eTPMS_TIRE_STATE_FR);
	msg.rpInterval.DrivingInfo.DrivingInfoB1.TpmsRL = Get_TPMS1(eTPMS_TIRE_STATE_RL);
	msg.rpInterval.DrivingInfo.DrivingInfoB1.TpmsRR = Get_TPMS1(eTPMS_TIRE_STATE_RR);
    msg.rpInterval.DrivingInfo.DrivingInfoB1.ModemRssi = abs(ModemManagerData.ndBm);

#if defined(PROTOCOL19)
		msg.rpInterval.DrivingInfo.DrivingInfoB1.HydrogenTemperature        = Get_HydrogenCurrentTemperature_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.HydrogenTankPress          = Get_HydrogenTankPress_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.HydrogenChargeCnt          = Get_HydrogenChargeCnt_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.FuelcellVoltLow			= Get_MinBatteryCellVoltage_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.FuelcellVoltHigh			= Get_MaxBatteryCellVoltage_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.HydrogenFuel               = Get_Hydrogen_Fuel_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.AirPurification            = Get_AirPurification_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.CO2Reduction               = Get_CO2Reduction_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.BMSBatteryVolt             = Get_BatteryPackV_Status();
		msg.rpInterval.DrivingInfo.DrivingInfoB1.BMSBatteryCurr             = Get_BatteryPackC_Status();
#endif

#if defined(PROTOCOL21)
	msg.rpInterval.DrivingInfo.DrivingInfoB1.EngOilLifeRatio                = Get_EngOilLifeRatio_Status();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.EngOilLifeEna                  = Get_EngOilLifeEna_Status();
	msg.rpInterval.DrivingInfo.DrivingInfoB1.EngOilLifeWarn                 = Get_EngOilLifeWarn_Status();
#endif

#if defined(PROTOCOL22)
    msg.rpInterval.DrivingInfo.DrivingInfoB1.LowBatterySOC                  = Get_LowBatterySOC_Status();
#endif

#if defined(PROTOCOL23)
	msg.rpInterval.DrivingInfo.DrivingInfoB1.InsulationResistance			= Get_InsulationResistance();
#endif
//printf("total:%d period:%d m:%f \r\n",Get_Total_Energy_Consume(),Get_Total_Energy_Consume(),Get_MileAge_avg());

#if defined(PERIOD_LOG1)
	Display_ReportDriving();
#endif



    // MONI 20180430
    // this message must be sent to system.
	//if( g_bEngRunKeepFlag == true )		Send2MngModem(eMngObd,eReqReport,eR_DrivingInterval,&msg,0);
	//else										    	Send2MngSysMsg(eMngObd,eRspReport,eR_DrivingInterval,&msg,0);
    Send2MngSysMsg(eMngObd,eRspReport,eR_DrivingInterval,&msg,0);

	UpdateOBDPeriodData();		//주기데이터 적산
	InitializeOBDPeriodData();	//주기데이터초기화
	
	HalTimerContinueSWTimer(g_iTimerVSS1secCallback);
    return 0;
}

int ReportNoDriving()
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    //MONI 2018-03-07
    // Not used code
    msg.rpInterval.NoDrivingInfo.CarBattery = (uint16_t)(Get_Battery() * 10);
    msg.rpInterval.NoDrivingInfo.GpsLatitude = Get_GPS_Lat();
    msg.rpInterval.NoDrivingInfo.GpsLongitude = Get_GPS_Lon();
    msg.rpInterval.NoDrivingInfo.GpsValid = Get_GPS_Vailication();
    msg.rpInterval.NoDrivingInfo.DoorLockStatus = Get_DoorLock();
    msg.rpInterval.NoDrivingInfo.DoorOpenStatus = Get_DoorOpen();
    msg.rpInterval.NoDrivingInfo.HeadLight = Get_HeadLamp_State();
	msg.rpInterval.NoDrivingInfo.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
	msg.rpInterval.NoDrivingInfo.OccurredEventUtcTime = GetUTCTime();
    msg.rpInterval.NoDrivingInfo.ModemRssi = abs(ModemManagerData.ndBm);
    msg.rpInterval.NoDrivingInfo.Odometer = Get_Odmeter();
#if defined(PROTOCOL12)
	msg.rpInterval.NoDrivingInfo.VehicleStatus = Get_VehicleStatus();
#endif
#if defined(PROTOCOL18)
	msg.rpInterval.NoDrivingInfo.ExtraDrivingDistance = g_stElectricCarData.m_usRemainedDistance;
	msg.rpInterval.NoDrivingInfo.RemainedECarBattery = g_stElectricCarData.m_ucSOC;
	msg.rpInterval.NoDrivingInfo.HighBatteryTemperature =g_stElectricCarData.m_usHighBatteryTemperatureMin;
	msg.rpInterval.NoDrivingInfo.HighBatteryTemperatureMax = g_stElectricCarData.m_usHighBatteryTemperatureMax;
#endif

#if defined(PERIOD_LOG)

#endif
	if( msg.rpInterval.NoDrivingInfo.CarBattery < DECIDE_BATTERY_LOW_VOLTAGE_ON )	
	{
		ReportAlramStatus(eMESSAGE_EVENT_KEY_LOW_VOLTAGE_ALARM, msg.rpInterval.NoDrivingInfo.CarBattery);
		printf("====================================================\r\n");
		printf("Low Voltage Alarm\n");
	}
    /* do gethering data all variables */
    Send2MngSysMsg(eMngObd,eRspReport,eR_ParkingInterval,&msg,0);
 
    return 0;
}

int ReportVehicleStatus(stCarReport carReport)
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    memcpy(msg.rpSmartKey.Status.Guid,carReport.rpSmartKey.Request.Guid,MAX_GUID_LENGTH);
    msg.rpSmartKey.Status.CarBattery = (uint16_t)(Get_Battery()*10);
    msg.rpSmartKey.Status.TailLanmp = Get_TailLamp_State();
    msg.rpSmartKey.Status.HeadLight = Get_HeadLamp_State();
    msg.rpSmartKey.Status.DoorLockStatus = Get_DoorLock();
    msg.rpSmartKey.Status.DoorOpenStatus = Get_DoorOpen();
    msg.rpSmartKey.Status.AccStatus = Get_VehicleStatus();
    msg.rpSmartKey.Status.Latitude = Get_GPS_Lat();
    msg.rpSmartKey.Status.Longitude = Get_GPS_Lon();
    msg.rpSmartKey.Status.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.rpSmartKey.Status.OccurredEventUtcTime = GetUTCTime();

    Send2MngSysMsg(eMngObd,eRspReport,eR_CurrentVehicleStatus,&msg,0);

    return 0;
}

int ReportAlramDtcStatus()
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

//	U8		m_ucECU_ID[4];					/* DTC ECU ID*/
//	U16		m_usFuncType;					/* DTC FUNCTION TYPE*/
//	U8		m_ucState;						/* RESULT */
//	U8		m_ucUDSFlag;						/* UDSFlag */
//	INT32U	m_nOdometer;					/* FCS ODOMETER */
//	U8		m_ucFCSTime[7];					/* FCS START TIME*/
//	U8		m_ucCount;							//DTC 갯수
//	U8		m_ucDTCCode[DTC_DATA_MAX];		/* DTC CODE*/
//

//	LENGTH SIZE;
//	LENGTH;	//전체렝스
//	{
//		ECUID; g_stDTCFuncData[i].m_ucECU_ID
//		FUNCTIONTYPE;	g_stDTCFuncData[i].m_usFuncType
//		STATE;	g_stDTCFuncData[i].m_ucState
//		CAN_UDS_FLAG;
//		ODO;	g_stDTCFuncData[i].m_ucUDSFlag;
//		TIME;	g_stDTCFuncData[i].m_ucFCSTime
//		DTC_LENGTH SIZE;	g_stDTCFuncData[i].m_ucCount
//		DTC_LENGTH;	//DTC의 렝스
//			DTC;
//			FF_LENGTH SIZE;
//			FF_LENGTH;
//				FF_FUNCTYPE;
//				FF_STATE;
//				FF_VALUE;
//	}

	memset(&msg.rpAlram.Dtc,0x00,sizeof(msg.rpAlram.Dtc));

//	msg.rpAlram.Dtc.m_llOperationKey 			= Get_DrivingKey();
//
//	for(i=0; i<g_ucDtcSystemCnt; i++)
//	{
//		strncpy(msg.rpAlram.Dtc.Body[i].m_ucECUID, g_stDTCFuncData[i].m_ucECU_ID, sizeof(g_stDTCFuncData[i].m_ucECU_ID));
//		strncpy(msg.rpAlram.Dtc.Body[i].m_ucFCSTime, g_stDTCFuncData[i].m_ucFCSTime, sizeof(g_stDTCFuncData[i].m_ucFCSTime));
//
//		msg.rpAlram.Dtc.Body[i].m_usLength 					= 0;
//		msg.rpAlram.Dtc.Body[i].m_ucDTCType 				= 0;
//		msg.rpAlram.Dtc.Body[i].m_ucSystemCnt			= 0;
//		msg.rpAlram.Dtc.Body[i].m_usSystemLength		= 0;
//		msg.rpAlram.Dtc.Body[i].m_usFuncType 			= g_stDTCFuncData[i].m_usFuncType;
//		msg.rpAlram.Dtc.Body[i].m_ucState					= g_stDTCFuncData[i].m_ucState;		//0:fail, 1:NODTC 2:DTC
//		msg.rpAlram.Dtc.Body[i].m_ucUDSFlag				= g_stDTCFuncData[i].m_ucUDSFlag;	//1:UDS 2:CAN
//		msg.rpAlram.Dtc.Body[i].m_uiOdo						= g_stDTCFuncData[i].m_nOdometer;
//		msg.rpAlram.Dtc.Body[i].m_ucDTCCount				= g_stDTCFuncData[i].m_ucCount;
//		msg.rpAlram.Dtc.Body[i].m_ucDTC[100]				;
//		msg.rpAlram.Dtc.Body[i].m_FreezeFrameSize		= 0;
//	}
    Send2MngSysMsg(eMngObd,eReqReport,eR_AlramDTC,&msg,0);

    return 0;
}

/*
eMESSAGE_EVENT_KEY_NONE                                 = 0,
eMESSAGE_EVENT_KEY_OVER_VOLTAGE_ALARM                   = 1,
eMESSAGE_EVENT_KEY_LOW_VOLTAGE_ALARM                    = 2,
eMESSAGE_EVENT_KEY_DOOR_LOCK_ALARM                      = 3,
eMESSAGE_EVENT_KEY_DOOR_OPEN_ALARM                      = 4,
eMESSAGE_EVENT_KEY_OVER_ENGINE_TEMP_ALARM               = 5,                                    //엔진온도
eMESSAGE_EVENT_KEY_FUEL_RUN_OUT                         = 6,
eMESSAGE_EVENT_KEY_TIRE_PRESSURE                        = 7,
eMESSAGE_EVENT_KEY_VEHICLE_OVER_SPEED_ALARM             = 8,
eMESSAGE_EVENT_KEY_VEHICLE_OVER_RPM_ALARM               = 9,
eMESSAGE_EVENT_KEY_VEHICLE_ENGINE_START_ALARM           = 11,
eMESSAGE_EVENT_KEY_TAIL_LAMP_ALARM                      = 12,
eMESSAGE_EVENT_KEY_PARKING_IMPACT                       = 14,                   // 주차충격
eMESSAGE_EVENT_KEY_MIL_LAMP_ON                          = 15,
eMESSAGE_EVENT_FOTA_COMPLETE
*/

int ReportDirectAlramStatus(int32_t SubEvent, eMESSAGE_EVENT_KEY eEVENT, int iValue)
{
    uint8_t bActive = 0;
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    
    GetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bActive);
    if( bActive == 0 )
        return -1;

    msg.rpAlram.CarStatus.OperationKey = Get_DrivingKey();
    msg.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    msg.rpAlram.CarStatus.EventKey = eEVENT;
    memcpy(msg.rpAlram.CarStatus.EventKeyValue,(char*)&iValue,4);
    msg.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    msg.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
    msg.rpAlram.CarStatus.GpsSetLatitude = 0;
    msg.rpAlram.CarStatus.GpsSetLongitude = 0;
    msg.rpAlram.CarStatus.Distance = 0;
    msg.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;
	msg.rpAlram.CarStatus.Odometer = Get_Odmeter();
#if defined(PROTOCOL23)
	msg.rpAlram.CarStatus.ModemRssi = abs(ModemManagerData.ndBm);
#endif
#if defined(FEATURE_EXTENSION_BOARD)&& defined(PROTOCOL21)
    msg.rpAlram.CarStatus.cFootBreakStatus = Get_FootBrake_State();
#endif

    Send2MngModem(eMngObd,eReqReport,SubEvent,&msg,0);

    return 0;    
}


int ReportAlramStatus(eMESSAGE_EVENT_KEY eEVENT, int iValue)
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    msg.rpAlram.CarStatus.OperationKey = Get_DrivingKey();
    msg.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    msg.rpAlram.CarStatus.EventKey = eEVENT;
    memcpy(msg.rpAlram.CarStatus.EventKeyValue,(char*)&iValue,4);
    msg.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    msg.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
    msg.rpAlram.CarStatus.GpsSetLatitude = 0;
    msg.rpAlram.CarStatus.GpsSetLongitude = 0;
    msg.rpAlram.CarStatus.Distance = 0;
    msg.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;
#if defined(PROTOCOL23)
	msg.rpAlram.CarStatus.ModemRssi = abs(ModemManagerData.ndBm);
#endif
#if defined(FEATURE_EXTENSION_BOARD)
    msg.rpAlram.CarStatus.cFootBreakStatus = Get_FootBrake_State();
#endif
    Send2MngSysMsg(eMngObd,eReqReport,eR_Alram,&msg,0);

    return 0;
}

int ReportAlramStatus2(eMESSAGE_EVENT_KEY eEVENT, uint8_t* pucValue, uint8_t ucLength)
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    msg.rpAlram.CarStatus.OperationKey = Get_DrivingKey();
    msg.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    msg.rpAlram.CarStatus.EventKey = eEVENT;
    memcpy(msg.rpAlram.CarStatus.EventKeyValue,pucValue,ucLength);
    msg.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    msg.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
    msg.rpAlram.CarStatus.GpsSetLatitude = 0;
    msg.rpAlram.CarStatus.GpsSetLongitude = 0;
    msg.rpAlram.CarStatus.Distance = 0;
    msg.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;
#if defined(PROTOCOL23)
	msg.rpAlram.CarStatus.ModemRssi = abs(ModemManagerData.ndBm);
#endif
    Send2MngSysMsg(eMngObd,eReqReport,eR_Alram,&msg,0);

    return 0;
}


#if defined(PROTOCOL18)
int ReportChargingStatus()
{
	stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    msg.rpInterval.ChargingInfo.ExtraCharingTime = Get_ExtraChargeTime();
    msg.rpInterval.ChargingInfo.ExtraDrivingDistance = Get_ExtraDrivingDistance_Status();
    msg.rpInterval.ChargingInfo.HighBatteryTemperatureMin = Get_HighBatteryTemperature();
    msg.rpInterval.ChargingInfo.HighBatteryTemperatureMax = Get_HighBatteryTemperatureMax();
    msg.rpInterval.ChargingInfo.HighBatterySOC = (uint8_t)Get_SOC_Status();
    msg.rpInterval.ChargingInfo.HighBatterySOH = g_stElectricCarData.m_ucSOH;

    Send2MngSysMsg(eMngObd,eReqReport,eR_Charging,&msg,0);
	
    return 1;
}
#endif

int ReportAlramTPMS(eMESSAGE_EVENT_KEY eEVENT, char* iValue)
{
    stCarReport msg;
    memset((char*)&msg,0,sizeof(stCarReport));

    msg.rpAlram.CarStatus.OperationKey = Get_DrivingKey();
    msg.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    msg.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    msg.rpAlram.CarStatus.EventKey = eEVENT;
    memcpy(msg.rpAlram.CarStatus.EventKeyValue,iValue,MAX_EVENT_KEY_VALUE);
    msg.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    msg.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();
    msg.rpAlram.CarStatus.GpsSetLatitude = 0;
    msg.rpAlram.CarStatus.GpsSetLongitude = 0;
    msg.rpAlram.CarStatus.Distance = 0;
    msg.rpAlram.CarStatus.Boundtype = eREMOTE_CON_BOUNDTYPE_IN;

    Send2MngSysMsg(eMngObd,eReqReport,eR_Alram,&msg,0);

    return 0;
}

int ReportAfterDriving()
{
    stMsgSysMsg sysMsg;
    stHalRTCTypeDef stHalRtcDateTime;
    memset((char*)&sysMsg,0,sizeof(stMsgSysMsg));

    sysMsg.header.id = eMngObd;
    sysMsg.header.event = eReqReport;
    sysMsg.header.subEvent = eR_AfterDriving;
    sysMsg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
    sysMsg.header.unTraceMng = eMngObd;

    sysMsg.header.drivingKey = Get_DrivingKey();

    sysMsg.carReport.rpInterval.AfterDrivingInfo.AppVersion = g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion;
    sysMsg.carReport.rpInterval.AfterDrivingInfo.BootloaderVersion = g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion;

    sysMsg.carReport.rpInterval.AfterDrivingInfo.OperationKey = Get_DrivingKey();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.StartDrivingLocalTime = Get_DriveStartTime_LTC();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.StopDrivingLocalTime = Get_DriveStopTime_LTC();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.StartDrivingUTCTime = Get_DriveStartTime_UTC();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.StopDrivingUTCTime = Get_DriveStopTime_UTC();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.CarBattery = (uint16_t)(Oem_GetBattVoltage(2)/10.0);
    sysMsg.carReport.rpInterval.AfterDrivingInfo.MaxSpeed = Get_MaxSpeed();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.AvgSpeed = (unsigned int)Get_Speed_Avg()*100;
#if defined(PROTOCOL16)
	if(FUELTYPE_GET_STATE() == ELECTRONIC || FUELTYPE_GET_STATE() == EV_NONE_READY )
	{
		sysMsg.carReport.rpInterval.AfterDrivingInfo.MaxRpm = Get_MotorMaxRPM();
		sysMsg.carReport.rpInterval.AfterDrivingInfo.AvgRpm = Get_MotorRPM_Avg();
		sysMsg.carReport.rpInterval.AfterDrivingInfo.ConsumedFuel = Get_Total_Energy_Consume();
		sysMsg.carReport.rpInterval.AfterDrivingInfo.m_usChargeCount=Get_ChargeCount();
		sysMsg.carReport.rpInterval.AfterDrivingInfo.m_usChargeTime=Get_ChargeTime();
		sysMsg.carReport.rpInterval.AfterDrivingInfo.m_bChargeState=Get_ChargeState();
	}
	else
	{
		sysMsg.carReport.rpInterval.AfterDrivingInfo.MaxRpm = Get_MaxRPM();
		sysMsg.carReport.rpInterval.AfterDrivingInfo.AvgRpm = Get_RPM_Avg();
		sysMsg.carReport.rpInterval.AfterDrivingInfo.ConsumedFuel = Get_Total_Fuel_Consume();
	}
//	sysMsg.carReport.rpInterval.AfterDrivingInfo.FuelEffiency = Get_MileAge_avg();	//서버계산
//	sysMsg.carReport.rpInterval.AfterDrivingInfo.ElectronicEffiency = Get_ElectronicMileAge_avg();	//서버게산
	sysMsg.carReport.rpInterval.AfterDrivingInfo.MaxMotorRpm = Get_MotorMaxRPM();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.AvrMotorRpm = Get_MotorRPM_Avg();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.RemainedECarBattery = (uint8_t)Get_SOC_Status();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.RemainedFuel = Get_Remain_Fuel_Percent();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.ConsumedBattery = Get_Total_Energy_Consume();
#if defined(PROTOCOL17)
	sysMsg.carReport.rpInterval.AfterDrivingInfo.ConsumedBattery_Regen = Get_Total_Energy_Consume_Regen();
#endif
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SOH = (uint8_t)Get_SOH_Status();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.ExtraDrivingDistance = Get_ExtraDrivingDistance_Status();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.HighBatteryTemperature = Get_HighBatteryTemperature();
#if defined(PROTOCOL18)
	sysMsg.carReport.rpInterval.AfterDrivingInfo.HighBatteryTemperatureMax = Get_HighBatteryTemperatureMax(); //dahae
#endif
	sysMsg.carReport.rpInterval.AfterDrivingInfo.OutsideTemperature = Get_OutsideTemperature();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.GpsDirection = Get_GpsDirection();
#else //PROTOCOL16
    if( FUELTYPE_GET_STATE() == ELECTRONIC || FUELTYPE_GET_STATE() == EV_NONE_READY )
    {
        sysMsg.carReport.rpInterval.AfterDrivingInfo.MaxRpm = Get_MotorMaxRPM();
        sysMsg.carReport.rpInterval.AfterDrivingInfo.AvgRpm = Get_MotorRPM_Avg();
        sysMsg.carReport.rpInterval.AfterDrivingInfo.ConsumedFuel = Get_Total_Energy_Consume();
        sysMsg.carReport.rpInterval.AfterDrivingInfo.RemainedECarBattery = (uint8_t)Get_SOC_Status();
        sysMsg.carReport.rpInterval.AfterDrivingInfo.RemainedFuel = 0;
    }
    else
    {
        sysMsg.carReport.rpInterval.AfterDrivingInfo.MaxRpm = Get_MaxRPM();
        sysMsg.carReport.rpInterval.AfterDrivingInfo.AvgRpm = Get_RPM_Avg();
        sysMsg.carReport.rpInterval.AfterDrivingInfo.ConsumedFuel = Get_Total_Fuel_Consume();
        sysMsg.carReport.rpInterval.AfterDrivingInfo.RemainedFuel = Get_Remain_Fuel_Percent();
        sysMsg.carReport.rpInterval.AfterDrivingInfo.RemainedECarBattery = 0;
    }	
#endif	//PROTOCOL16
    sysMsg.carReport.rpInterval.AfterDrivingInfo.RapidAccelCount = Get_total_Rapid_acceleration_Counter();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.RapidDecelCount = Get_total_Rapid_Deceleration_Counter();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.FakeDrivingTime = Get_EngineIdleTime();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.WarmUpTime = Get_WarmupTime();
    //sysMsg.carReport.rpInterval.AfterDrivingInfo.FuelEffiency = Get_MileAge_avg();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.DrivingTime = Get_DrivingTime();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.DrivingDistance = Get_driving_distance_int();
//    sysMsg.carReport.rpInterval.AfterDrivingInfo.EverageSpeed = Get_Speed_Avg();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.GeerPosition = Get_GearPosition();
//    sysMsg.carReport.rpInterval.AfterDrivingInfo.DrivingConsumedFuel = (uint8_t)Get_MileAge_avg();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.StartGpsValid = Get_GpsStatOnValidation();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.PowerOnLatitude = Get_GpsStartOnLat();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.PowerOnLongitude = Get_GpsStartOnLon();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.EndGpsValid = Get_GPS_Vailication();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.EndGpsLatitude = Get_GPS_Lat();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.EndGpsLongitude = Get_GPS_Lon();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.Odometer = Get_Odmeter();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.StartOdometer = Get_StartOdmeter();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCnt0km = Get_Total_Speed_Counter_0();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper1kmUnder10km = Get_Total_Speed_Counter_from_1_under_10();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper11kmUnder20km = Get_Total_Speed_Counter_from_11_under_20();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper21kmUnder30km = Get_Total_Speed_Counter_from_21_under_30();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper31kmUnder40km = Get_Total_Speed_Counter_from_31_under_40();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper41kmUnder50km = Get_Total_Speed_Counter_from_41_under_50();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper51kmUnder60km = Get_Total_Speed_Counter_from_51_under_60();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper61kmUnder70km = Get_Total_Speed_Counter_from_61_under_70();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper71kmUnder80km = Get_Total_Speed_Counter_from_71_under_80();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper81kmUnder90km = Get_Total_Speed_Counter_from_81_under_90();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper91kmUnder100km = Get_Total_Speed_Counter_from_91_under_100();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper101kmUnder110km = Get_Total_Speed_Counter_from_101_under_110();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper111kmUnder120km = Get_Total_Speed_Counter_from_111_under_120();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper121kmUnder130km = Get_Total_Speed_Counter_from_121_under_130();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper131kmUnder140km = Get_Total_Speed_Counter_from_131_under_140();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.SpdCntUpper141km = Get_Total_Speed_Counter_over_141();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.AccIdlingCount	 			= Get_TotalDrivePattern0();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.AccStopCount 				= Get_TotalDrivePattern1();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.AccBreakeCount 				= Get_TotalDrivePattern2();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.AccInertiaDrivingCount	 	= Get_TotalDrivePattern3();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.AccNormalDrivingCount	= Get_TotalDrivePattern4();
#if defined(PROTOCOL17)
	sysMsg.carReport.rpInterval.AfterDrivingInfo.AccExtraDrivingCount	= Get_TotalDrivePattern5();
#endif
    sysMsg.carReport.rpInterval.AfterDrivingInfo.TpmsFL = Get_TPMS1(eTPMS_TIRE_STATE_FL);
	sysMsg.carReport.rpInterval.AfterDrivingInfo.TpmsFR = Get_TPMS1(eTPMS_TIRE_STATE_FR);
	sysMsg.carReport.rpInterval.AfterDrivingInfo.TpmsRL = Get_TPMS1(eTPMS_TIRE_STATE_RL);
	sysMsg.carReport.rpInterval.AfterDrivingInfo.TpmsRR = Get_TPMS1(eTPMS_TIRE_STATE_RR);
    sysMsg.carReport.rpInterval.AfterDrivingInfo.TailLanmp = Get_TailLamp_State();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.HeadLight = Get_HeadLamp_State();
    sysMsg.carReport.rpInterval.AfterDrivingInfo.MasterDBVersion = g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion;
    sysMsg.carReport.rpInterval.AfterDrivingInfo.SlaveDBVersion = g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion;
    sysMsg.carReport.rpInterval.AfterDrivingInfo.DrivingVersion = g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion;
    sysMsg.carReport.rpInterval.AfterDrivingInfo.BootloaderVersion = g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion;
    sysMsg.carReport.rpInterval.AfterDrivingInfo.AppVersion = g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion;
#if defined(PROTOCOL22)
	sysMsg.carReport.rpInterval.AfterDrivingInfo.LowBatterySOC = Get_LowBatterySOC_Status();
#endif
#if defined(PROTOCOL23)
	sysMsg.carReport.rpInterval.AfterDrivingInfo.HydrogenChargeCnt = Get_HydrogenChargeCnt_Status();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.HydrogenTankPress = Get_HydrogenTankPress_Status();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.FuelcellVoltLow = Get_MinBatteryCellVoltage_Status();;
	sysMsg.carReport.rpInterval.AfterDrivingInfo.FuelcellVoltHigh = Get_MaxBatteryCellVoltage_Status();;
	sysMsg.carReport.rpInterval.AfterDrivingInfo.HydrogenFuel = Get_Hydrogen_Fuel_Status();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.AirPurification = Get_AirPurification_Status();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.CO2Reduction = Get_CO2Reduction_Status();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.HydrogenTemperature = Get_HydrogenCurrentTemperature_Status();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.InsulationResistance = Get_InsulationResistance();
	sysMsg.carReport.rpInterval.AfterDrivingInfo.ModemRssi = abs(ModemManagerData.ndBm);
#endif
#if defined TIME_CHECK
	printf("@@@@@@@@@@@@@@@@@@@@@TotalTime : %d,%d\r\n",Get_TotalDrivePattern0()+Get_TotalDrivePattern1()+Get_TotalDrivePattern2()+Get_TotalDrivePattern3()+Get_TotalDrivePattern4()+Get_TotalDrivePattern5(),Get_DriveStopTime_LTC()-Get_DriveStartTime_LTC());
#endif
#if defined(PERIOD_LOG)
	uint8_t arrTemp[64];
    long long llValue=0;

	Trace(PERIOD_LOG,"@@@@@@@@@@@@@@@@  Trip  @@@@@@@@@@@@@@@r\n");
	Trace(PERIOD_LOG," 0. VehicleStatus                           : %d\r\n", Get_VehicleStatus());
	Trace(PERIOD_LOG," 1. start driving(local)                   : %d\r\n", Get_DriveStartTime_LTC());
	Trace(PERIOD_LOG," 2. end driving(local)                     : %d\r\n", Get_DriveStopTime_LTC());
	Trace(PERIOD_LOG," 3. start driving(utc)                     : %d\r\n", Get_DriveStartTime_UTC());
	Trace(PERIOD_LOG," 4. end driving(utc)                       : %d\r\n", Get_DriveStopTime_UTC());
	Trace(PERIOD_LOG," 5. MaxSpeed                               : %hd\r\n", Get_MaxSpeed());
	Trace(PERIOD_LOG," 6. MaxRPM                                 : %hd MaxRPM\r\n", Get_MaxRPM());
	Trace(PERIOD_LOG," 6-1. Motor MaxRPM                         : %hd MaxRPM\r\n", Get_MotorMaxRPM());
    Trace(PERIOD_LOG," 7. Get_total_Rapid_acceleration_Counter   : %d s\r\n", Get_total_Rapid_acceleration_Counter());
	Trace(PERIOD_LOG," 8. Get_total_Rapid_Deceleration_Counter   : %d s\r\n", Get_total_Rapid_Deceleration_Counter());
	Trace(PERIOD_LOG," 9. Engine_idle  time                      : %d s\r\n", Get_EngineIdleTime());
	Trace(PERIOD_LOG,"10. Warmup time                            : %d s\r\n", Get_WarmupTime());
	Trace(PERIOD_LOG,"11. total fuel use                         : %f L\r\n", Get_Total_Fuel_Consume());
	Trace(PERIOD_LOG,"12. Get_DrivingTime                        : %d \r\n", Get_DrivingTime());
	Trace(PERIOD_LOG,"13. Driving_distance                       : %d m\r\n", Get_driving_distance_int());
	Trace(PERIOD_LOG,"13_2. MileAge                              : %f\r\n",Get_MileAge_avg());
	Trace(PERIOD_LOG,"14. Gear position                          : %hd\r\n", Get_GearPosition());
	Trace(PERIOD_LOG,"15. Remain_Fuel_Percent                    : %d\r\n", Get_Remain_Fuel_Percent());
	Trace(PERIOD_LOG,"16. Remain_battery                         : %d\r\n", Get_SOC_Status());
	Trace(PERIOD_LOG,"17. start _GPS_Vailication                 : %d\r\n", Get_GpsStatOnValidation());
	Trace(PERIOD_LOG,"18. start_Get_GPS_Lat                      : %f\r\n", Get_GpsStartOnLat());
	Trace(PERIOD_LOG,"19. start_Get_GPS_Lon                      : %f\r\n", Get_GpsStartOnLon());
	Trace(PERIOD_LOG,"20. end_GPS_Vailication                    : %d\r\n", Get_GPS_Vailication());
	Trace(PERIOD_LOG,"21. end_Get_GPS_Lat                        : %f\r\n", Get_GPS_Lat());
	Trace(PERIOD_LOG,"22. end_Get_GPS_Lon                        : %f\r\n", Get_GPS_Lon());
	Trace(PERIOD_LOG,"23. total distance                         : %d km\r\n", Get_Odmeter());
	Trace(PERIOD_LOG,"24. Period_Speed_Counter_0 : %d\r\n", Get_Total_Speed_Counter_0());
	Trace(PERIOD_LOG,"24. Period_Speed_Counter_from_1_under_10 : %d\r\n", Get_Total_Speed_Counter_from_1_under_10());
	Trace(PERIOD_LOG,"25. Period_Speed_Counter_from_41_under_50 : %d\r\n", Get_Total_Speed_Counter_from_41_under_50());
	Trace(PERIOD_LOG,"26. Period_Speed_Counter_from_81_under_90 : %d\r\n", Get_Total_Speed_Counter_from_81_under_90());
	Trace(PERIOD_LOG,"27. Period_Speed_Counter_from_121_under_130: %d\r\n", Get_Total_Speed_Counter_from_121_under_130());
	Trace(PERIOD_LOG,"28. Period_Speed_Counter_over_141          : %d\r\n", Get_Total_Speed_Counter_over_141());
	Trace(PERIOD_LOG,"28. D0          : %d\r\n", Get_TotalDrivePattern0());
	Trace(PERIOD_LOG,"28. D1          : %d\r\n", Get_TotalDrivePattern1());
	Trace(PERIOD_LOG,"28. D2          : %d\r\n", Get_TotalDrivePattern2());
	Trace(PERIOD_LOG,"28. D3          : %d\r\n", Get_TotalDrivePattern3());
	Trace(PERIOD_LOG,"28. D4          : %d\r\n", Get_TotalDrivePattern4());
	Trace(PERIOD_LOG,"29. Get_TPMS_RR                            : %hd\r\n", Get_TPMS1(eTPMS_TIRE_STATE_RR));
	Trace(PERIOD_LOG,"30. Get_TPMS_RL                            : %hd\r\n", Get_TPMS1(eTPMS_TIRE_STATE_RL));
	Trace(PERIOD_LOG,"31. Get_TPMS_FR                            : %hd\r\n", Get_TPMS1(eTPMS_TIRE_STATE_FR));
	Trace(PERIOD_LOG,"32. Get_TPMS_FL                            : %hd\r\n", Get_TPMS1(eTPMS_TIRE_STATE_FL));
	Trace(PERIOD_LOG,"33. eApp_MasterDB                          : %hd\r\n", g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion);
	Trace(PERIOD_LOG,"34. eApp_SlaveDB                           : %hd\r\n", g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion);
	Trace(PERIOD_LOG,"35. eApp_ControlDB                         : %hd\r\n", g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion);
	Trace(PERIOD_LOG,"36. eApp_Bootloader                        : %hd\r\n", g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion);
	Trace(PERIOD_LOG,"37. eApp_Application                       : %hd\r\n", g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion);
	Trace(PERIOD_LOG,"@@@@@@@@@@@@@@@@  finish  @@@@@@@@@@@@@@@@s\r\n");

    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRTCTypeDef), 0);
	GetDateTimeStamp(arrTemp, stHalRtcDateTime.RtcDate, stHalRtcDateTime.RtcTime);

	llValue = atoll((char *)arrTemp);

	Trace(PERIOD_LOG,"2. CREATE TIME   : %lld\r\n", llValue);
	Trace(PERIOD_LOG,"0. Get_DrivingKey: %lld \r\n", Get_DrivingKey());
	Trace(PERIOD_LOG,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
#endif
   	Send2MngSysMsg3(&sysMsg);

	if(g_bFuelLiterTypeFlag == true && g_bFuelLevelSwitchedPercentFlag == false)	//슬립들어가기전 재주행시 잔량이갱신안되어 잔량기준으로 소모량을 계산하여 문제가됨
	{
		g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter = g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter - g_dChecked_Fuel_Consume;
	}
	
	InitializeOBDPeriodData();	//주기데이터 초기화
	InitializeOBDTripData();		//트립데이터 초기화

    // notify to system about car info
    //Send2MngSysMsg(eMngObd,eReqReport,eR_AfterDriving,&msg);
    // report to server about car info
    //Send2MngModem(eMngObd,eReqReport,eR_SmartKey,&msg);

    return 0;
}

int ReportBeforeDriving()
{
    stHalRTCTypeDef stHalRtcDateTime;

    uint32_t unKeyValue = 1;
    stMsgSysMsg sysMsg;

    memset((char*)&sysMsg,0,sizeof(stMsgSysMsg));

    sysMsg.header.id = eMngObd;
    sysMsg.header.event = eReqReport;
    sysMsg.header.subEvent = eR_BeforeDriving;
    sysMsg.header.unTraceMng = eMngObd;

    sysMsg.header.drivingKey = Get_DrivingKey();
	sysMsg.carReport.rpAlram.CarStatus.EventKey = eMESSAGE_EVENT_KEY_VEHICLE_ENGINE_START_ALARM;
	memcpy(sysMsg.carReport.rpAlram.CarStatus.EventKeyValue,(char*)&unKeyValue,4);
    sysMsg.carReport.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    sysMsg.carReport.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    sysMsg.carReport.rpAlram.CarStatus.GpsCurLatitude = Get_GPS_Lat();
    sysMsg.carReport.rpAlram.CarStatus.GpsCurLongitude = Get_GPS_Lon();    
//    sysMsg.carReport.rpInterval.BrforeDrivingInfo.AlramMaskSettingValue = 0;
//    sysMsg.carReport.rpInterval.BrforeDrivingInfo.ParkingReportInterval = 30;
//    sysMsg.carReport.rpInterval.BrforeDrivingInfo.DrivingReportInterval = 3;
//    sysMsg.carReport.rpInterval.BrforeDrivingInfo.ValetAlramSetupThreshold = 0;
//    sysMsg.carReport.rpInterval.BrforeDrivingInfo.ValetAlramSetupTime = 0;
//    sysMsg.carReport.rpInterval.BrforeDrivingInfo.TowingAlramSetupthreshold = 0;
//    sysMsg.carReport.rpInterval.BrforeDrivingInfo.TowingAlramSetupTime = 0;
#if defined(PERIOD_LOG)
	uint8_t arrTemp[64];
    long long llValue=0;

	Trace(PERIOD_LOG,"!!!!!!!!!!!!!!!!!!!!!!!!!!  ReportBeforeDriving  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
	Trace(PERIOD_LOG,"AlramMaskSettingValue					: %d\r\n", sysMsg.carReport.rpInterval.BrforeDrivingInfo.AlramMaskSettingValue);
	Trace(PERIOD_LOG,"ParkingReportInterval						: %d\r\n", sysMsg.carReport.rpInterval.BrforeDrivingInfo.ParkingReportInterval);
	Trace(PERIOD_LOG,"DrivingReportInterval						: %d\r\n", sysMsg.carReport.rpInterval.BrforeDrivingInfo.DrivingReportInterval);
	Trace(PERIOD_LOG,"ValetAlramSetupThreshold				: %d\r\n", sysMsg.carReport.rpInterval.BrforeDrivingInfo.ValetAlramSetupThreshold);
	Trace(PERIOD_LOG,"ValetAlramSetupTime						: %d\r\n", sysMsg.carReport.rpInterval.BrforeDrivingInfo.ValetAlramSetupTime);
	Trace(PERIOD_LOG,"TowingAlramSetupthreshold				: %d\r\n", sysMsg.carReport.rpInterval.BrforeDrivingInfo.TowingAlramSetupthreshold);
	Trace(PERIOD_LOG,"TowingAlramSetupTime)					: %d\r\n", sysMsg.carReport.rpInterval.BrforeDrivingInfo.TowingAlramSetupTime);

    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRTCTypeDef), 0);
	GetDateTimeStamp(arrTemp, stHalRtcDateTime.RtcDate, stHalRtcDateTime.RtcTime);

	llValue = atoll((char *)arrTemp);

	Trace(PERIOD_LOG,"2. CREATE TIME   : %lld\r\n", llValue);
	Trace(PERIOD_LOG,"0. Get_DrivingKey: %lld \r\n", Get_DrivingKey());
	Trace(PERIOD_LOG,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
#endif
    // notify to system about car info
    Send2MngSysMsg3(&sysMsg);
    // report to server about car info
    //Send2MngModem(eMngObd,eReqReport,eR_BeforeDriving,&msg);

    return 0;
}

stMsgObd m_stLastSmartKeyInfo;
#if defined(REASON_8BYTE)
int ResponseSmartKeyResult(int iActuatorType , int iResult, uint64_t ullReason)
#else
int ResponseSmartKeyResult(int iActuatorType , int iResult, int iReason)
#endif
{
    stMsgMdm msg;
    
    memset((char*)&msg,0,sizeof(stMsgMdm));

    memcpy((char*)&msg,(char*)&m_stLastSmartKeyInfo,sizeof(stMsgMdm));

    msg.header.id = eMngObd;
    msg.header.event = eRspReport;
    msg.header.subEvent = eR_RspSmartKey;
    msg.header.unEventTime = GetLocalTimefromTime(GetUTCTime());
	msg.header.drivingKey = Get_DrivingKey();

    // MONI 20180810 Bug Fixed.
    // copy request guid to send to server.
    memcpy((char*)msg.carReport.rpSmartKey.Response.Guid,(char*)m_stLastSmartKeyInfo.carReport.rpSmartKey.Request.Guid,MAX_GUID_LENGTH);
#if defined(REASON_8BYTE)
    msg.carReport.rpSmartKey.Response.Reason = ullReason;
#else
	msg.carReport.rpSmartKey.Response.Reason = iReason;
#endif
    msg.carReport.rpSmartKey.Response.Result = iResult;
	msg.carReport.rpSmartKey.Response.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
	msg.carReport.rpSmartKey.Response.OccurredEventUtcTime = GetUTCTime();

#if defined(PROTOCOL18)
	msg.carReport.rpSmartKey.Response.CommandType = m_stLastSmartKeyInfo.carReport.rpSmartKey.Request.CommandType;
	msg.carReport.rpSmartKey.Response.ControlType = m_stLastSmartKeyInfo.carReport.rpSmartKey.Request.ControlType; 
	
	msg.carReport.rpSmartKey.Response.KeepPowerOnTime = m_stLastSmartKeyInfo.carReport.rpSmartKey.Request.KeepPowerOnTime;//여기 걸림
	msg.carReport.rpSmartKey.Response.Temperature = m_stLastSmartKeyInfo.carReport.rpSmartKey.Request.Temperature;
    msg.carReport.rpSmartKey.Response.CheckTemperature = m_stLastSmartKeyInfo.carReport.rpSmartKey.Request.CheckTemperature;
    msg.carReport.rpSmartKey.Response.Defrost = m_stLastSmartKeyInfo.carReport.rpSmartKey.Request.Defrost;
	msg.carReport.rpSmartKey.Response.ucSysSmartkeyReqType = m_stLastSmartKeyInfo.carReport.rpSmartKey.Request.ucSysSmartkeyReqType;

	memcpy(msg.carReport.rpSmartKey.Response.BTControlKey, m_stLastSmartKeyInfo.carReport.rpSmartKey.Request.BTControlKey, 16);
	
#endif
	printf("************************************\r\n");
	printf("SEND2SYS: BT Remote control Result!!\r\n");
	printf("************************************\r\n");
	
    //Send2MngModem2(&msg);
    Send2MngSysMsg3((stMsgSysMsg*) &msg);

    Trace(SYSTEM_MSG,"%s] Reponse Smartkey\r\n",__FUNCTION__);
    ShowSmartkeyResponse(&msg.carReport);

    return 0;
}

void SmartkeyAction(stMsgObd* report)
{
    ShowSmartkeyAction(report->carReport);

    memcpy((char*)&m_stLastSmartKeyInfo,(char*)report,sizeof(stMsgObd));
    memset((char*)&g_stReportSmartReq,0x00,sizeof(g_stReportSmartReq));
    memcpy((char*)&g_stReportSmartReq,(char*)&m_stLastSmartKeyInfo.carReport.rpSmartKey.Request,sizeof(g_stReportSmartReq));
    
	printf("************************************\r\n");
	printf("BT SMARTKEY ACTION !!\r\n");
	printf("************************************\r\n");
	printf("temp:%02X, Ctemp:%02X, Deforgger:%02X, high:%02X, defrost:%02X\r\n",g_stReportSmartReq.Temperature,g_stReportSmartReq.CheckTemperature,g_stReportSmartReq.RearDefogger,g_stReportSmartReq.HighBeam,g_stReportSmartReq.Defrost);
	
	if( GetOBDState()==eOBD_Running_Info_Mode )
	{
		switch(g_stReportSmartReq.CommandType)
		{
			case eREMOTE_CON_CMD_TYPE_STARTING:
				if( Get_Speed() > 0 )
				{
					ResponseSmartKeyResult(ACTUATOR_TYPE_ENGINERUN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_DRIVING);	//운행중제어불가
				}
				else
				{
					if( g_stReportSmartReq.ControlType == 1 )	//엔진구동
					{
						g_uiEngStopTimer = Get_Tmr();
						
					    if( (FUELTYPE_GET_STATE() == FCEV) || FUELTYPE_GET_STATE() == ELECTRONIC || (FUELTYPE_GET_STATE() == GASOLINE_HEV) || (FUELTYPE_GET_STATE() == PLUGIN_HEV) )
				        {
				            if( Get_EngRun_Status() > eENGRUN_OFF )
    						{
    							ResponseSmartKeyResult(ACTUATOR_TYPE_ENGINERUN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_ALREADY_ENGRUN);	//이미시동걸려있음
    						}
    						else
    						{
    							Set_Actuator(ACTUATOR_TYPE_ENGINERUN);
    							if( g_stReportSmartReq.KeepPowerOnTime == 10 )	g_EngOnSetTime = 590000;	//실제 차량이측정한 10분과 우리측 측정시간 10분이 정확히 일치하지 않으므로 10분설정시 9분50초에 정지
    							else																		g_EngOnSetTime = g_stReportSmartReq.KeepPowerOnTime * 60000;
    							
    							if( (g_stReportSmartReq.Temperature == 0xFFFF) && (g_stReportSmartReq.CheckTemperature == 0xFF) )	g_bAirConRunFlag = false;
    							else
								{
									g_sSavedTemperature = g_stReportSmartReq.Temperature;	//엔진구동시에는 세팅값을 저장해둬야함 저장안하면 다른 제어명령 내려올때 지워짐
									g_cSavedDefrostState = g_stReportSmartReq.Defrost;				//엔진구동시에는 세팅값을 저장해둬야함 저장안하면 다른 제어명령 내려올때 지워짐
									g_bAirConRunFlag = true;
								}
    							
    							if( g_stReportSmartReq.RearDefogger == 1 )
								{
									g_cSavedReadDefoggerState = g_stReportSmartReq.RearDefogger;	//엔진구동시에는 세팅값을 저장해둬야함 저장안하면 다른 제어명령 내려올때 지워짐
									g_bRearDefogRunFlag = true;
								}
    							else 														g_bRearDefogRunFlag = false;
    						}
				        }
						else if( FUELTYPE_GET_STATE() == EV_NONE_READY )
				        {
				            if( Get_EngRun_Status() > eENGRUN_OFF || Get_FATCState() == 1 )
    						{
    							ResponseSmartKeyResult(ACTUATOR_TYPE_ENGINERUN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_ALREADY_ENGRUN);	//이미시동걸려있음
    						}
    						else
    						{
    							Set_Actuator(ACTUATOR_TYPE_ENGINERUN);
								//OS EV의 경우 FATC 유지중 도어락,언락이 내려오면 코드가 변경됨
								
    							if( g_stReportSmartReq.KeepPowerOnTime == 10 )	g_EngOnSetTime = 590000;	//실제 차량이측정한 10분과 우리측 측정시간 10분이 정확히 일치하지 않으므로 10분설정시 9분50초에 정지
    							else																		g_EngOnSetTime = g_stReportSmartReq.KeepPowerOnTime * 60000;
    							
    							if( (g_stReportSmartReq.Temperature == 0xFFFF) && (g_stReportSmartReq.CheckTemperature == 0xFF) )	g_bAirConRunFlag = false;
    							else
								{
									g_sSavedTemperature = g_stReportSmartReq.Temperature;	//엔진구동시에는 세팅값을 저장해둬야함 저장안하면 다른 제어명령 내려올때 지워짐
									g_cSavedDefrostState = g_stReportSmartReq.Defrost;				//엔진구동시에는 세팅값을 저장해둬야함 저장안하면 다른 제어명령 내려올때 지워짐
									g_bAirConRunFlag = true;
								}
    							
    							if( g_stReportSmartReq.RearDefogger == 1 )
								{
									g_cSavedReadDefoggerState = g_stReportSmartReq.RearDefogger;	//엔진구동시에는 세팅값을 저장해둬야함 저장안하면 다른 제어명령 내려올때 지워짐
									g_bRearDefogRunFlag = true;
								}
    							else 														g_bRearDefogRunFlag = false;
    						}
				        }
				        else
				        {
    						if( Get_RPM() > 0 )
    						{
    							ResponseSmartKeyResult(ACTUATOR_TYPE_ENGINERUN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_ALREADY_ENGRUN);	//이미시동걸려있음
    						}
    						else
    						{
    							Set_Actuator(ACTUATOR_TYPE_ENGINERUN);
    							if( g_stReportSmartReq.KeepPowerOnTime == 10 )	g_EngOnSetTime = 590000;	//실제 차량이측정한 10분과 우리측 측정시간 10분이 정확히 일치하지 않으므로 10분설정시 9분50초에 정지
    							else																		g_EngOnSetTime = g_stReportSmartReq.KeepPowerOnTime * 60000;
    							
    							if( (g_stReportSmartReq.Temperature == 0xFFFF) && (g_stReportSmartReq.CheckTemperature == 0xFF) )	g_bAirConRunFlag = false;
    							else																																					g_bAirConRunFlag = true;
    							
    							if( g_stReportSmartReq.RearDefogger == 1 ) 	g_bRearDefogRunFlag = true;
    							else 														g_bRearDefogRunFlag = false;
    						}
    					}
					}
					else	//엔진정지
					{
						printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++STOP++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
						if( Get_UserStatus() == eUSER_IN )	ResponseSmartKeyResult(ACTUATOR_TYPE_ENGINERUN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_USERCTRL);	//유저에게제어권넘어감
						else
						{
						    if( (FUELTYPE_GET_STATE() == FCEV) || FUELTYPE_GET_STATE() == ELECTRONIC || (FUELTYPE_GET_STATE() == GASOLINE_HEV) || FUELTYPE_GET_STATE() == PLUGIN_HEV )
						    {
						        if( Get_EngRun_Status() == eENGRUN_OFF)	ResponseSmartKeyResult(ACTUATOR_TYPE_ENGINERUN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_ALREADY_ENGSTOP);	//이미시동걸려있음
    							else							Set_Actuator(ACTUATOR_TYPE_ENGINESTOP);
						    }
							else if( FUELTYPE_GET_STATE() == EV_NONE_READY )
							{
								if( Get_FATCState() == 0 )	ResponseSmartKeyResult(ACTUATOR_TYPE_ENGINERUN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_ALREADY_ENGSTOP);
								else									Set_Actuator(ACTUATOR_TYPE_AIRCON_STOP);
							}
						    else
						    {
    							if( Get_RPM() == 0)	ResponseSmartKeyResult(ACTUATOR_TYPE_ENGINERUN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_ALREADY_ENGSTOP);	//이미시동걸려있음
    							else							Set_Actuator(ACTUATOR_TYPE_ENGINESTOP);
    						}
						}
					}
				}
				break;
			case eREMOTE_CON_CMD_TYPE_DOOR:
				if( g_stReportSmartReq.ControlType == 1 )
				{
					if( Get_Speed() > 0 )	ResponseSmartKeyResult(ACTUATOR_TYPE_DOORUNLOCK,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_DRIVING);	//운행중제어불가
					else							Set_Actuator(ACTUATOR_TYPE_DOORUNLOCK);
				}
				else
				{
					if( Get_Speed() > 0 )	ResponseSmartKeyResult(ACTUATOR_TYPE_DOORLOCK,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_DRIVING);	//운행중제어불가
					else							Set_Actuator(ACTUATOR_TYPE_DOORLOCK);
				}
				break;
			case eREMOTE_CON_CMD_TYPE_EMERGENCY_LIGHT:
				if( Get_Speed() > 0 )
				{
					ResponseSmartKeyResult(ACTUATOR_TYPE_LAMP,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_DRIVING);	//운행중제어불가
				}
				else
				{
					if( g_stReportSmartReq.ControlType == 1 )	Set_Actuator(ACTUATOR_TYPE_LAMP);
				}
				break;
			case eREMOTE_CON_CMD_TYPE_HORN_EMERGENCY_LIGHT:
				if( Get_Speed() > 0 )
				{
					ResponseSmartKeyResult(ACTUATOR_TYPE_LAMPHORN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_DRIVING);	//운행중제어불가
				}
				else if( (Get_DoorLock() & 0x0F) != 0x00 )
				{
					ResponseSmartKeyResult(ACTUATOR_TYPE_LAMPHORN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_DOORUNLOCK);//램프는 문열려있을시 제어안됨
				}
				else
				{
					if( g_stReportSmartReq.ControlType == 1 )
						Set_Actuator(ACTUATOR_TYPE_LAMPHORN);
				}
				break;
			case eREMOTE_CON_CMD_TYPE_RESET:
			case eREMOTE_CON_CMD_TYPE_GEO_FENCE:
			case eREMOTE_CON_CMD_TYPE_SENSOR_SENSITIVITY:
				GIT_Assert(false,eErrorCodeObd|eNotSupportCommand);
				break;
			case eREMOTE_CON_CMD_TYPE_DTC_STATUS:
				if( Get_Speed() > 0 )
				{
					ResponseSmartKeyResult(ACTUATOR_TYPE_LAMPHORN,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_DRIVING);	//운행중제어불가
				}
				else
				{
					if( g_bKeyTypeDBFlag == true )
					{
						if( Get_VehicleStatus() >= eVEHICLE_STATE_IGON )
						{
								SetRequestDtcAlram(true);
								SetOBDState(eOBD_FCS_Start);
								g_bFCSRunFlag = true;
								g_bFreezeFrameRunFlag = true;
						}
						else	// IG상태아님
						{
							ResponseSmartKeyResult(ACTUATOR_TYPE_DTC,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_IGOFF);	//IgOff or Acc상태
						}
					}
					else
					{
						if( Get_VehicleStatus() >= eVEHICLE_STATE_IGON && (Get_UserStatus() == eUSER_IN) )
						{
								SetRequestDtcAlram(true);
								SetOBDState(eOBD_FCS_Start);
								g_bFCSRunFlag = true;
								g_bFreezeFrameRunFlag = true;
						}
						else	// IG상태아님
						{
							ResponseSmartKeyResult(ACTUATOR_TYPE_DTC,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_IGOFF);	//IgOff or Acc상태
						}
					}
				}
				break;
			case eREMOTE_CON_CMD_TYPE_NONE:
			default:
                ResponseSmartKeyResult(ACTUATOR_TYPE_NONE,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_NONE);
				break;
		}
	}
	else
	{
		if( g_bDBParsingFailFlag == true )	ResponseSmartKeyResult(ACTUATOR_TYPE_FAIL,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_PARSINGFAIL);
		else												ResponseSmartKeyResult(ACTUATOR_TYPE_DTC,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_ANOTHERFUNCTION);	//다른기능 동작중
	}
}

void ShowSmartkeyResponse(stCarReport* report)
{
#if defined(REASON_8BYTE)
     int iHighbit=0;
     int iLowbit=0;
     iHighbit = report->rpSmartKey.Response.Reason>>32;
     iLowbit = report->rpSmartKey.Response.Reason;
#endif
    // do something from server
    Trace(SYSTEM_MSG,"%s] Action Reponse\r\n",__FUNCTION__);
    Trace(SYSTEM_MSG,"%s] Result : %d\r\n",__FUNCTION__,report->rpSmartKey.Response.Result);
#if defined(REASON_8BYTE)
    Trace(SYSTEM_MSG,"%s] Reason : 0x%08X%08X\r\n",__FUNCTION__,iHighbit,iLowbit);
#else
	Trace(SYSTEM_MSG,"%s] Reason : %x\r\n",__FUNCTION__,report->rpSmartKey.Response.Reason);
#endif
    //Trace(0,"%s] Guid : %s\r\n",__FUNCTION__,report->rpSmartKey.Request.Guid);
}

boolean_t m_bRequestDtcAlram = false;

void SetRequestDtcAlram(boolean_t bRequestDtcAlram)
{
    m_bRequestDtcAlram = bRequestDtcAlram;
}

boolean_t GetRequestDtcAlram()
{
    return m_bRequestDtcAlram;
}


int CheckInitializeTime(unsigned int uiNewTime, unsigned int uiOldTime, unsigned short usNewTimezone)
{
	int GapTime=0;

    if( IsInitializeTime(uiOldTime) == true )	//스타트 시간이 이상(스타트시간이 17년 12월보다 작을때)
	{   // 네트웍 세팅시의 시간 - 네트웍 세팅 전 시간 
		GapTime = (int)uiNewTime - (int)uiOldTime;
	}
	else	//스타트 시간이 정상
	{
		if(g_OBDControllerData.basicData.snDriveStartTimeZone != 0)
		{
			GapTime = ((int)usNewTimezone - (int)g_OBDControllerData.basicData.snDriveStartTimeZone)*15*60; // Timezone 값 1 = 15분 
		}
		else // 네트워크 셋팅 시간 비정상 
		{
			GapTime = (int)uiNewTime - (int)uiOldTime;	
		}
	}
    return GapTime;
}

// external event handler
void MngObdExternalEvent(stMsgObd* pstMsgObd)
{
	stNetworkTime stNetworkDate;
	stHalRTCTypeDef stDate;
	unsigned int GapTime=0;
	uint8_t arrTmp[64];
	
	memset(&stNetworkDate,0x00,sizeof(stNetworkDate));
	memset(&stDate,0x00,sizeof(stDate));
	
    if( pstMsgObd->header.id == eMngSys )
    {
        if( pstMsgObd->header.event == eReqSetting )
        {
//            Send_Odometer_From_File(pstMsgObd->carReport.rpSetting.unOdometer);
            Set_GPS_Lat_Origin(pstMsgObd->carReport.rpSetting.ObdSetting.dlLatitue);
            Set_GPS_Lon_Origin(pstMsgObd->carReport.rpSetting.ObdSetting.dlLongitude);
			g_fFuelLevelPercent = pstMsgObd->carReport.rpSetting.ObdSetting.fRemainedFuel;
			Trace(0,"MngObdExternalEvent] g_fFuelLevelPercent : %lf\r\n", g_fFuelLevelPercent); // woong bae 20181210 ??료??량 ??스??용 
        }
    }
    else if( pstMsgObd->header.id == eMngSysMsg )
    {
        if( pstMsgObd->header.event== eReqSleep )
        {
            // if other manager wants to go sleep, this mananger request upper manager
            // and wait for event from upper manager.
            Trace(SYSTEM_MSG,"%s]Get Sleep Event from other Manager\r\n",__FUNCTION__);
            
//			stCANFDBoardUpdateInfo stFDBoardInfo;
//			
//			stFDBoardInfo.usBootVersion = 3;
//			stFDBoardInfo.usAppVersion = 3;
//			
//			if(CFD_SetCanFDVersion(1,1,1,1))
//			{
//			   CFD_Sleep();
//			   MD_uDelay(500);
//			}
			SetObdRcvSleepFlag(true);

#if 1	// lwh CFD SLEEP 관련 로직 변경
			if(CFD_GetCanFDAdapter())
			{
				if(CFD_GetCanFDSleepStatus()==false)
				{
				  	SetCanFDMainState(eCanFD_SLEEP);

					//전기차 Sleep 중 Wake up 이슈로 인한 추가 
					if((m_stCFDCtrl.stVer.usAppVer >= CANFD_BYPASS_OFF_LINE_SET_VERSION)&&((FUELTYPE_GET_STATE() == ELECTRONIC) || (FUELTYPE_GET_STATE() == EV_NONE_READY)))
					{
						SetCurrentState(eFDSetCanByPassModeReq);
					}
					else
					{
    					SetCurrentState(eFDSetSleepReq);
    				}
    			}
			}
			else
			{
				Oem_CAN_None_Receive_Set();
				Send2MngSysMsg(eMngObd,eRspSleep,0, (stCarReport *)NULL,0);
			}
#else
			if(CFD_GetCanFDAdapter())
			{
			   CFD_Sleep();
			   MD_uDelay(10000);
			}
			
			Oem_CAN_None_Receive_Set();
			Send2MngSysMsg(eMngObd,eRspSleep,0, (stCarReport *)NULL,0);
#endif
        }
        else if( pstMsgObd->header.event == eReqReport )
        {
            if( pstMsgObd->header.subEvent == eR_DrivingInterval )
            {
                // change state for gethering data from can or variables
                ReportDriving();
            }
            else if( pstMsgObd->header.subEvent == eR_ParkingInterval )
            {
                // change state for gethering data from can or variables
                ReportNoDriving();
            }
            else if ( pstMsgObd->header.subEvent == eR_ReqSmartKey )
            {
               	SmartkeyAction(pstMsgObd);
#if defined(FEATURE_EXTENSION_BOARD)
                // fob전원제어 
                Send2SysExtend(eMngObd, eExtend_ReqControl,eExtBD_FobKeyControlReq, true, (stMsgExtend*)pstMsgObd, eMngExtend);
#endif
            }
            else if( pstMsgObd->header.subEvent == eR_CurrentVehicleStatus )
            {
                ReportVehicleStatus(pstMsgObd->carReport);
            }
            else if( pstMsgObd->header.subEvent == eR_AlramDTC )
            {
                SetRequestDtcAlram(true);
            }
            else
            {
                GIT_Assert(false,eErrorCodeObd|eSubEventUnknown);
            }
        }
        else if( pstMsgObd->header.event == eOBDSetting )
        {
            if( pstMsgObd->header.subEvent == eMS_NetworkTime )
            {
                // net new network time
#if 0
                stNetworkDate.unNetWrokTime = pstMsgObd->carReport.rpSetting.ModemSetting.stNetworkDate.unNetWrokTime;
                stNetworkDate.sTimeZone = pstMsgObd->carReport.rpSetting.ModemSetting.stNetworkDate.sTimeZone;
								
				GapTime = CheckInitializeTime(g_OBDControllerData.basicData.DriveStartTime_RTC,pstMsgObd->header.unEventTime);
#else
                GapTime = pstMsgObd->carReport.rpSetting.ModemSetting.stNetworkDate.iTimeGap;
#endif
				
                if( (GapTime != 0) && (g_OBDControllerData.basicData.drivingKey != 0) )
                {
                    Trace(0,"==============================================================\r\n");
                    Trace(0,"driving start time is not correct so ME adjust the time.\r\n");
					Trace(0,"==============================================================\r\n");
                    // start time is not correct
					Trace(0,"Before Adjusting the time\r\n");
					Trace(0,"DriveStartUTCTime : %d, GapTime : %d\r\n",g_OBDControllerData.basicData.DriveStartTime_UTC,GapTime);
                    // adjust new network time
					if((IsInitializeTime(g_OBDControllerData.basicData.DriveStartTime_UTC) == true) ||
						(g_OBDControllerData.basicData.snDriveStartTimeZone == 0))
                    {
						//초기 시간
						Trace(0,"Changing Start UTC Time\r\n");
	                    g_OBDControllerData.basicData.DriveStartTime_UTC = g_OBDControllerData.basicData.DriveStartTime_UTC+GapTime;
                    }

					Trace(0,"Changing Start Local Time\r\n");
					g_OBDControllerData.basicData.DriveStartTime_LocalTime = GetLocalTimefromTime(g_OBDControllerData.basicData.DriveStartTime_UTC);
					
					GetDatefromTime2(&stDate, g_OBDControllerData.basicData.DriveStartTime_LocalTime);

					sprintf((char *)arrTmp, "%04d%02d%02d%0.2d%0.2d%0.2d\x00",
								2000 + stDate.RtcDate.RTC_Year,
								stDate.RtcDate.RTC_Month,
								stDate.RtcDate.RTC_Date,
								stDate.RtcTime.RTC_Hours,
								stDate.RtcTime.RTC_Minutes,
								stDate.RtcTime.RTC_Seconds);

					printf("MSG: Adjust Driving Key : [%s]\n", arrTmp);
					g_OBDControllerData.basicData.drivingKey = (long long)atoll((char *)arrTmp);
                }
            }
            else if( pstMsgObd->header.subEvent == eInitailize )
            {
                Trace(0,"set obd status to initialize\r\n");
                SetOBDState(eOBD_Initialize);
            }
            else if( pstMsgObd->header.subEvent == eUpdateMode )
            {
                Trace(0,"set obd status to update mode\r\n");
                SetOBDState(eOBD_FOTA);
            }
        }
        else
        {
            GIT_Assert(false,eErrorCodeObd|eEventUnknown);
            //error not defined
        }
    }
    else if( pstMsgObd->header.id == eMngModem )
    {
        if( pstMsgObd->header.subEvent == eInitailize )
        {
            Trace(0,"set obd status to initialize\r\n");
            SetOBDState(eOBD_Initialize);
        }
    }
#if defined(QA_FIFA) 
	else if( pstMsgObd->header.id == eMngObd)
	{
		if( pstMsgObd->header.event == eOBDEngineStatus )
		{
			if( pstMsgObd->header.subEvent == eEngineStop)
			{
				if(GetPACVType() == CV)
				{
				Send_RPM_From_CAN(0);
					Send_EngRun_From_CAN(0);
				}
			}
		}
	}
#endif
    else
    {
        // not defined
        GIT_Assert(false,eErrorCodeObd|eUnknownId);
    }
}

void IndicatorTimeoutCheck()
{
	for(int i=0; i<ePOS_INDICATOR_TIMER_MAX; i++ )
	{
		if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[i].m_uiCanStopTimer) > IND_CANOFF_CHECK_TIME )
		{
			switch(i)
			{
				case ePOS_4WD_IND_0: Send_4WD_IND_From_CAN(0 ,0x01); break;
				case ePOS_4WD_IND_1: Send_4WD_IND_From_CAN(0 ,0x02); break;
				case ePOS_4WD_IND_2: Send_4WD_IND_From_CAN(0 ,0x04); break;
				case ePOS_4WD_IND_3: Send_4WD_IND_From_CAN(0 ,0x08); break;
				case ePOS_4WD_IND_4: Send_4WD_IND_From_CAN(0 ,0x10); break;
				case ePOS_4WD_IND_5: Send_4WD_IND_From_CAN(0 ,0x20); break;
				case ePOS_4WD_IND_6: Send_4WD_IND_From_CAN(0 ,0x40); break;
				case ePOS_4WD_IND_7: Send_4WD_IND_From_CAN(0 ,0x80); break;
				case ePOS_ABS_IND_0: Send_ABS_IND_From_CAN(0, 0x01); break;
				case ePOS_ABS_IND_1: Send_ABS_IND_From_CAN(0, 0x02); break;
				case ePOS_ACU_IND_0: Send_Airbag_IND_From_CAN(0);		break;	//10
				case ePOS_AUTOHOLD_IND_0: Send_AutoHold_IND_From_CAN(0); break;	//15
				case ePOS_BATTCHARGE_IND_0: Send_BatteryCharge_IND_From_CAN(0, 0x01); break;
				case ePOS_BATTCHARGE_IND_1: Send_BatteryCharge_IND_From_CAN(0, 0x02); break;
				case ePOS_CHECKENGINE_IND_0: Send_CheckEngine_IND_From_CAN(0, 0x01); break;	//20
				case ePOS_CHECKENGINE_IND_1: Send_CheckEngine_IND_From_CAN(0, 0x02); break;
				case ePOS_CHECKENGINE_IND_2: Send_CheckEngine_IND_From_CAN(0, 0x04); break;
				case ePOS_CHECKENGINE_IND_3: Send_CheckEngine_IND_From_CAN(0, 0x08); break;
				case ePOS_DBCWARNNING_IND: Send_DBCWarnning_IND_From_CAN(0, 0x01); break;
				case ePOS_DPFGPF_IND: Send_DPF_IND_From_CAN(0); break;	//25
				case ePOS_SCR_IND: Send_SCR_IND_From_CAN(0, 0x01); break;
				case ePOS_EPB_IND_0: Send_EPB_IND_From_CAN(0); break;
				case ePOS_TCS_IND_0: Send_TCS_IND_From_CAN(0); break;
				case ePOS_ISG_IND_0: Send_ISG_IND_From_CAN(0); break;
				case ePOS_MDPS_IND_0: Send_MDPS_IND_From_CAN(0); break;
				case ePOS_OILLEVEL_IND: Send_OilLevel_IND_From_CAN(0, 0x01); break;
				case ePOS_OILPRESS_IND: Send_OilPress_IND_From_CAN(0, 0x01); break;
				case ePOS_PARKINGBRAKE_IND_0: Send_ParkingBrake_IND_From_CAN(0, 0x01); break;
				case ePOS_PARKINGBRAKE_IND_1: Send_ParkingBrake_IND_From_CAN(0, 0x02); break;
				case ePOS_PARKINGBRAKE_IND_2: Send_ParkingBrake_IND_From_CAN(0, 0x04); break;	//40
				case ePOS_POWERDOWN_IND_0: Send_PowerDown_IND_From_CAN(0, 0x01); break;
				case ePOS_POWERDOWN_IND_1: Send_PowerDown_IND_From_CAN(0, 0x02); break;
				case ePOS_AHB_IND: Send_AHB_IND_From_CAN(0, 0x01); break;
				case ePOS_HEV_IND_0: Send_HEV_IND_From_CAN(0, 0x01);break;
				case ePOS_HEV_IND_1: Send_HEV_IND_From_CAN(0, 0x02);break;	//45
				case ePOS_HEV_IND_2: Send_HEV_IND_From_CAN(0, 0x04);break;
				case ePOS_HEV_IND_3: Send_HEV_IND_From_CAN(0, 0x08);break;
				case ePOS_HEV_IND_4: Send_HEV_IND_From_CAN(0, 0x10);break;
				case ePOS_HEV_IND_5: Send_HEV_IND_From_CAN(0, 0x20);break;
				case ePOS_HEV_IND_6: Send_HEV_IND_From_CAN(0, 0x40);break;	//50
				case ePOS_HEV_IND_7: Send_HEV_IND_From_CAN(0, 0x80);break;
				case ePOS_EV_IND_0: Send_EV_IND_From_CAN(0, 0x01); break;
				case ePOS_EV_IND_1: Send_EV_IND_From_CAN(0, 0x02); break;
				case ePOS_EV_IND_2: Send_EV_IND_From_CAN(0, 0x04); break;
				case ePOS_EV_IND_3: Send_EV_IND_From_CAN(0, 0x08); break;	//55
				case ePOS_EV_IND_4: Send_EV_IND_From_CAN(0, 0x10); break;
				case ePOS_EV_IND_5: Send_EV_IND_From_CAN(0, 0x20); break;
				case ePOS_FCEV_IND_0: Send_FCEV_IND_From_CAN(0, 0x01); break;
				case ePOS_FCEV_IND_1: Send_FCEV_IND_From_CAN(0, 0x02); break;
				case ePOS_FCEV_IND_2: Send_FCEV_IND_From_CAN(0, 0x04); break;	//60
				case ePOS_FCEV_IND_3: Send_FCEV_IND_From_CAN(0, 0x08); break;
				case ePOS_FCEV_IND_4: Send_FCEV_IND_From_CAN(0, 0x10); break;
				case ePOS_TPMS_IND_0: Send_TPMS_IND_From_CAN(0); break;
				default:	break;
			}
		}
	}
}

void OBDManager(void)
{
	static eOBD_STATE eCurrentDCSModeState = eOBD_NONE;
//	static unsigned int uiOld=0;
	static unsigned int s_uiTimeoutTimer=0,s_ucSaveStopTime=0;
	static unsigned int s_uiFotaTimerOneMin=0;
	static unsigned int s_uiIndicatorCheckTimer=0;
#if defined (KEYON_FUNCTION_START)
	static eVEHICLE_STATE s_eSaveState = eVEHICLE_STATE_OFF;
#endif
	//static unsigned int uiOld2=0,uiNew2=0,Flag=0;
	//static unsigned char s_ucDBParsingFlag=0;	//DB파싱은 최초 한번만(슬립중 처음으로 돌아와도 새로하지않음)
//	unsigned int uiTempOdo=0;
//	unsigned int CanID1 = 0x0112;
//	unsigned int CanID2 = 0x0112;
//	static bool s_bActuatFlag=true;
//	static bool s_bRunningInfoFirstTimeFlag=true;

    stMsgObd tmpMsg;
	unsigned int uiFCSTimeout=0;

	if(GetPACVType()==CV) uiFCSTimeout = CV_FCS_TIMEOUT;
	else				  uiFCSTimeout = FCS_TIMEOUT;

    // check external event from queue
#ifndef GLOBAL_SHARE_QUEUE //Get
    if( MngQueueGetMessage(ID_MNG_QUEUE_OBD, (int8_t*)&tmpMsg, sizeof(stMsgObd)) == true )
#else
	if( GetSysHdShareQueueMessage(ID_MNG_QUEUE_OBD, (uint8_t*)&tmpMsg.header, sizeof(stMsgHeader), (uint8_t*)&tmpMsg.carReport, sizeof(stCarReport)) == true )
#endif
    {
//		Trace(0,"result : %x\r\n",tmpMsg.header.result);
//        Trace(0,"event : %s, subEvent : %s, from : %s, reuslt : %x\r\n",
//            GetStringFromEvent(eGetStringEvent,tmpMsg.header.event,0),
//            GetStringFromEvent(eGetStringSubEvent,tmpMsg.header.event,tmpMsg.header.subEvent),
//            GetStringFromId(tmpMsg.header.unTraceMng&0xFF),
//            tmpMsg.header.result);

        if( tmpMsg.header.subEvent == eR_Alram )
        {
//            Trace(0,"alram event : %s\r\n",GetStringFromAlramEvent(tmpMsg.carReport.rpAlram.CarStatus.EventKey));
        }

        tmpMsg.header.unTraceMng=tmpMsg.header.unTraceMng<<8|eMngSysMsg;
        // external event process
        MngObdExternalEvent(&tmpMsg);
    }

	eCurrentDCSModeState = GetOBDState();

//	if( g_bEngRunContinueSuppFlag == true && g_bEngRunActFlag == true && (Get_TmrDelta( Get_Tmr(),g_uiEngContinueSendTimer ) >= g_ContinueCodeTiming ) && g_OBDControllerData.monitering_Data.m_ucUserCtrlStatus == eUSER_OUT )
	if( FUELTYPE_GET_STATE() == EV_NONE_READY )
	{
		if( g_bEngRunKeepFlag == true && (Get_TmrDelta( Get_Tmr(),g_uiEngContinueSendTimer ) >= g_ContinueCodeTiming ) && Get_UserStatus() == eUSER_OUT )
		{
			SendEngContinueCode();
		}
	}
	else	//추후 DB수정 및 테스트 한뒤 위로직으로 통합
	{
		if( g_bEngRunKeepFlag == true && (Get_TmrDelta( Get_Tmr(),g_uiEngContinueSendTimer ) >= 500 ) && Get_UserStatus() == eUSER_OUT )
		{
			SendEngContinueCode();
		}
	}
	
	if( Get_UserStatus() == eUSER_IN  )
	{
		if( g_bEngRunKeepFlag == true )	//엔진 유지코드 멈췄어요 이벤트
		{
//			printf("eKeepAliveStop_3\r\n");
			Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStop, (stCarReport *)NULL,0);
			g_bEngRunKeepFlag = false;
			g_bFATCRunActFlag = false;
		}
		g_bEngRunActFlag = false;
	}
	
	if( (Get_TmrDelta( Get_Tmr(),g_uiEngStopTimer) >= ACTUATOR_MAX_ENGONTIME ) || (Get_TmrDelta( Get_Tmr(),g_uiEngStopTimer) >= g_EngOnSetTime) )	//세팅된시간
	{
		if( g_bEngRunActFlag == true )
		{
			Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStop, (stCarReport *)NULL,0);
			ReportAlramStatus(eMESSAGE_EVENT_KEY_ENGKEEP_TIMEOVER_ALRAM, 0);
			Set_Actuator(ACTUATOR_TYPE_ENGINESTOP);
			g_bNoResponseFlag = true;	//내부실행일때 응답보내지 않기위해 사용
		}
		if( g_bFATCRunActFlag == true )
		{
			Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStop, (stCarReport *)NULL,0);
			ReportAlramStatus(eMESSAGE_EVENT_KEY_ENGKEEP_TIMEOVER_ALRAM, 0);
			Set_Actuator(ACTUATOR_TYPE_AIRCON_STOP);
			g_bNoResponseFlag = true;	//내부실행일때 응답보내지 않기위해 사용
		}

		/*
		if((g_bFATCRunActFlag == false) && (FUELTYPE_GET_STATE() == EV_NONE_READY) && (Get_IG3_Status() == 1) && (Get_UserStatus() == eUSER_OUT) )
		{
			printf("~~~~~~~Ionic IG3 OFF occured~~~~~~~~~~");
			Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStop, (stCarReport *)NULL,0);
			ReportAlramStatus(eMESSAGE_EVENT_KEY_ENGKEEP_TIMEOVER_ALRAM, 0);
			Set_Actuator(ACTUATOR_TYPE_AIRCON_STOP);
			g_bNoResponseFlag = true;	//내부실행일때 응답보내지 않기위해 사용
		}
		*/
		g_bFATCRunActFlag = false;
		g_bEngRunActFlag = false;
		g_bEngRunKeepFlag = false;
	}


			
	if( g_bIndicatorFirstCheckFlag == false && (Get_TmrDelta(Get_Tmr(), s_uiIndicatorCheckTimer)>ONE_SECOND) && GetOBDState()==eOBD_Running_Info_Mode )
	{
		IndicatorTimeoutCheck();
		s_uiIndicatorCheckTimer = Get_Tmr();
	}
	
//	Trace(0,"%d",BTGetConnectStatus);

	switch(eCurrentDCSModeState) {
		case eOBD_NONE:
			SetOBDState(eOBD_Initialize);
			break;
//		case eOBD_Error:
//			break;
		case eConfiguration_Error:
//			SetOBDState(eOBD_GetAutoVIN);	//최초 DB없을시 Autovin하러감
			SetOBDState(eOBD_FW_DB_Update_Mode);
			break;
		case eOBD_Initialize:
#ifdef CHECK_MALLOC
			__iar_dlmalloc_stats();
#endif
				if(CFD_GetCanFDAdapter() && CFD_GetInitComplete() == false) 
				{
					SetOBDState(eOBD_InitCanFD);
					SetCanFDInitHandler();
				}
				else
				{
					SetOBDState(eOBD_Running_Info_Mode);
				}

#ifdef CHECK_MALLOC
			__iar_dlmalloc_stats();
#endif
			break;
		case eOBD_Initialized:		//현재 미사용
			break;
		case eOBD_InitCanFD:
			switch(CanFDInitHandler())
			{
				case eCanFdhdResSuccess:
						SetOBDState(eOBD_Running_Info_Mode);
					break;
				case eCanFdhdResFail:
						//SetOBDState(eConfiguration_Error);
						SetOBDState(eOBD_Running_Info_Mode);
						ReportAlramStatus(eMESSAGE_EVENT_KEY_CANFD_CANNOT_COMMUNICATION, 1);
					break;
				case eCanFdhdResNone:
					break;
			}
			break;
		case eOBD_FCS_Start:
			if( g_bFCSRunFlag == true )
			{
				if(CFD_GetCanFDAdapter())
				{
					Oem_CAN_Initial_CH1(Highcan2, eCAN_1MBPS); 
					CFD_SetByPassFlagAllOff();
				}
					
				g_bFCSRunFlag = false;
			    DefaultCanMaskSetDCAN();
				s_uiTimeoutTimer = Get_Tmr();
			}
			
			if( Get_TmrDelta(Get_Tmr(), s_uiTimeoutTimer) > uiFCSTimeout )	//FCS시작후 30초이상 지나면 ERROR간주 TIMEOUT BASIC루틴 적용 프리미엄에서 본적없음
			{
				printf("FCS TIME OUT\r\n");
				SetOBDState(eOBD_Running_Info_Mode);
				if( g_bIndicatorFirstCheckFlag == true )
				{
					g_bIndicatorFirstCheckFlag = false;
					for( int i=0;i<ePOS_INDICATOR_TIMER_MAX;i++)	g_stIndicatorCheck[i].m_uiTimerFromLastOFF = g_stIndicatorCheck[i].m_uiTimerFromLastON = g_stIndicatorCheck[i].m_uiCanStopTimer = Get_Tmr();
					printf("Clear IndicatorTimer_FCS\r\n");
				}
				//g_bFCSRunFlag = true;
			}
			else
			{
				OBDFCS_Mode();
			}
			break;

		case eOBD_FreezeFrame:
			if( g_bFreezeFrameRunFlag == true )
			{
				if(CFD_GetCanFDAdapter())
				{
					Oem_CAN_Initial_CH1(Highcan2, eCAN_1MBPS); 
					CFD_SetByPassFlagAllOff();
				}
				
				g_bFreezeFrameRunFlag = false;
				DefaultCanMaskSetDCAN();
			}
#ifdef NO_FREEZE_FRAME
			FreezeFrameReport();	//서버로 보내자
			if(g_bSOHTypeDBFlag == true)	SetOBDState(eOBD_GetSOH);
			else
			{
				if( g_stBrakeJudder.bValid == true) SetOBDState(eOBD_GetBrakeJudder);
				else								SetOBDState(eOBD_Running_Info_Mode);
			}
#else
			OBDFreezeFrame_Mode();
#endif
			break;
		case eOBD_Running_Info_Mode:		
//			if( Get_VehicleStatus() >= eVEHICLE_STATE_IGON )
			if( g_bIndicatorDBFlag == true )
			{
#if defined (KEYON_FUNCTION_START)
				if( Get_VehicleStatus() >= eVEHICLE_STATE_IGON )
#else
				if( Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN )
#endif
				{
#if defined (KEYON_FUNCTION_START)
					if(( Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN )&& ( s_eSaveState == eVEHICLE_STATE_IGON))
					{
						g_bRunningInfoFirstTimeFlag = true;	
						
						g_bEngRunActFlag = false;
						g_bFATCRunActFlag = false;
						g_bEngineIdleFlag = FALSE;
						g_bFCSRunFlag=true;	//주행완료시 세팅해서 다음주행시작때 다시 체크하도록 함
						g_bIndicatorFirstCheckFlag=true;
						g_bFreezeFrameRunFlag=true;	//주행완료시 세팅해서 다음주행시작때 다시 체크하도록 함
						g_bMILLampAlramFlag=true;
					}
					if( s_eSaveState != Get_VehicleStatus() )
					{
				  		s_eSaveState = Get_VehicleStatus();
					}
#endif
					if( g_bRunningInfoFirstTimeFlag == true )
					{
						DefaultAllCanMaskSet();
//						if(CFD_GetCanFDAdapter())
//						{
//							CFD_SetByPassFlagAllOn();
//						}
						Trace(0,"%d\r\n",DCAN_GET_COMM_STATE());
						DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
						g_bRunningInfoFirstTimeFlag = false;
						SetActuatorStatus( ACTUATOR_STATUS_NONE );
	//					printf("~~~~~~~~~~~~~~~~~~~~~~Time : %d",Get_TmrDelta(Get_Tmr(),g_uiTempTimer) );
						printf("Clear IndicatorTimer_INFO\r\n");
						for( int i=0;i<ePOS_INDICATOR_TIMER_MAX;i++)	g_stIndicatorCheck[i].m_uiTimerFromLastOFF = g_stIndicatorCheck[i].m_uiTimerFromLastON = g_stIndicatorCheck[i].m_uiCanStopTimer = Get_Tmr();
						g_uiEngRunStartTime = Get_Tmr();	//50과 80순서가 아닌 80 50순으로 나오는 경우 방지
					}
					
					if(g_bFuelLiterTypeFlag == true)
					{
						if( ((g_bFuelLevelCheckFlag == true) && (SensorData.g_bEnableSensor_MPU6515 == true)) )
						{
							MeasureGyroAngle();
							SensorData.g_eSensorState = eSENSOR_STATE_GET_DATA_MPU6515;
							stSensorInfo stGyroAngle;
							GetGyroAngle(&stGyroAngle);

							if(stGyroAngle.bCalibration == true || g_ucTCUSlopeAngleArrIndex > 0)
							{
								if(g_iTimerFuelCheck30secCallback == -1) // 콜백 ??????
								{	
									if( ((FUEL_LEVEL_CHECK_MIN_ANGLE <= stGyroAngle.nX && stGyroAngle.nX <= FUEL_LEVEL_CHECK_MAX_ANGLE)
									&& (FUEL_LEVEL_CHECK_MIN_ANGLE <= stGyroAngle.nY && stGyroAngle.nY <= FUEL_LEVEL_CHECK_MAX_ANGLE)) 
									|| (g_fFuelLevelPercent == 0.0 || g_fFuelLevelPercent > 100.0))
									{
										SetOBDState(eOBD_GetFuelLevelCheck);
									}
									else
									{
										g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter=(float)(g_ucFuelLevelMaxLiter/100.0) * g_fFuelLevelPercent;
										g_bFuelLevelCheckFlag=false;
									}
								}
								else// 콜백??????면 
								{
									SetOBDState(eOBD_GetFuelLevelCheck);
								}
								HalTimerClearSWTimer(g_iTimerFuelCheck30secCallback);
								g_iTimerFuelCheck30secCallback = -1;
							}
							else 
							{
								if(g_fFuelLevelPercent == 0.0 || g_fFuelLevelPercent > 100.0 || g_bFuelLevelSwitchedPercentFlag == true)
								{
									SetOBDState(eOBD_GetFuelLevelCheck);
								}
								else
								{
									g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter=(float)(g_ucFuelLevelMaxLiter/100.0) * g_fFuelLevelPercent;
									g_bFuelLevelCheckFlag=false;	
								}
							}
						}
					}
					//러닝인포 이후 4초뒤에 한번만 체크
					if( g_bKeyTypeDBFlag == true )
					{
						if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= AUTOVIN_START_TIME) && (g_stAutoVin.bRxStatus==false) )
						{
							SetOBDState(eOBD_GetAutoVIN);
						}
						else if( (Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME) && (g_bFCSRunFlag == true) )	//AUTOVIN이 성공이든 실패든 끝난 후 + ENG RUN한지 2초후 시작
						{
							SetOBDState(eOBD_FCS_Start);
						}
					}
					else
					{
						if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= AUTOVIN_START_TIME) && (g_stAutoVin.bRxStatus==false) && (Get_UserStatus() == eUSER_IN) )
						{
							SetOBDState(eOBD_GetAutoVIN);
						}
						else if( (Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME) && (g_bFCSRunFlag == true) && (Get_UserStatus() == eUSER_IN) )	//AUTOVIN이 성공이든 실패든 끝난 후 + ENG RUN한지 2초후 시작
						{
							SetOBDState(eOBD_FCS_Start);
						}
					}

					OBDRunningInfo();
					g_bCtrlDBSetFlag = false;
				}
				else	//IG OFF거나 ACC인 상태
				{
					OBDRunningInfo_ControlDB();
					g_bRunningInfoFirstTimeFlag = true;
				}
			}
			else
			{
				if( Get_VehicleStatus() >= eVEHICLE_STATE_IGON )
				{
					if( g_bRunningInfoFirstTimeFlag == true )
					{
						DefaultAllCanMaskSet();
						Trace(0,"%d\r\n",DCAN_GET_COMM_STATE());
						DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
						g_bRunningInfoFirstTimeFlag = false;
						SetActuatorStatus( ACTUATOR_STATUS_NONE );
	//					printf("~~~~~~~~~~~~~~~~~~~~~~Time : %d",Get_TmrDelta(Get_Tmr(),g_uiTempTimer) );
						printf("Clear IndicatorTimer_INFO\r\n");
						for( int i=0;i<ePOS_INDICATOR_TIMER_MAX;i++)	g_stIndicatorCheck[i].m_uiTimerFromLastOFF = g_stIndicatorCheck[i].m_uiTimerFromLastON = g_stIndicatorCheck[i].m_uiCanStopTimer = Get_Tmr();
						g_uiEngRunStartTime = Get_Tmr();	//50과 80순서가 아닌 80 50순으로 나오는 경우 방지
					}
					
					if(g_bFuelLiterTypeFlag == true)
					{
						if( (g_bFuelLevelCheckFlag == true) )
						{
							stSensorInfo stGyroAngle;
							GetGyroAngle(&stGyroAngle);

							if(stGyroAngle.bCalibration == true || g_ucTCUSlopeAngleArrIndex > 0)
							{
								if(g_iTimerFuelCheck30secCallback == -1) // 콜백 ??????
								{	
									if( ((FUEL_LEVEL_CHECK_MIN_ANGLE <= stGyroAngle.nX && stGyroAngle.nX <= FUEL_LEVEL_CHECK_MAX_ANGLE)
									&& (FUEL_LEVEL_CHECK_MIN_ANGLE <= stGyroAngle.nY && stGyroAngle.nY <= FUEL_LEVEL_CHECK_MAX_ANGLE)) 
									|| (g_fFuelLevelPercent == 0.0 || g_fFuelLevelPercent > 100.0))
									{
										SetOBDState(eOBD_GetFuelLevelCheck);
									}
									else
									{
										g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter=(float)(g_ucFuelLevelMaxLiter/100.0) * g_fFuelLevelPercent;
										g_bFuelLevelCheckFlag=false;
									}
								}
								else// 콜백??????면 
								{
									SetOBDState(eOBD_GetFuelLevelCheck);
								}
								HalTimerClearSWTimer(g_iTimerFuelCheck30secCallback);
								g_iTimerFuelCheck30secCallback = -1;
							}
							else 
							{
								if(g_fFuelLevelPercent == 0.0 || g_fFuelLevelPercent > 100.0 || g_bFuelLevelSwitchedPercentFlag == true)
								{
									SetOBDState(eOBD_GetFuelLevelCheck);
								}
								else
								{
									g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter=(float)(g_ucFuelLevelMaxLiter/100.0) * g_fFuelLevelPercent;
									g_bFuelLevelCheckFlag=false;	
								}
							}
						}
					}
					//러닝인포 이후 4초뒤에 한번만 체크
					if( g_bKeyTypeDBFlag == true )
					{
						if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= AUTOVIN_START_TIME) && (g_stAutoVin.bRxStatus==false) )
						{
							SetOBDState(eOBD_GetAutoVIN);
						}
						else if( (Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME) && (g_bFCSRunFlag == true) )	//AUTOVIN이 성공이든 실패든 끝난 후 + ENG RUN한지 2초후 시작
						{
							SetOBDState(eOBD_FCS_Start);
						}
					}
					else
					{
						if((Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= AUTOVIN_START_TIME) && (g_stAutoVin.bRxStatus==false) && (Get_UserStatus() == eUSER_IN) )
						{
							SetOBDState(eOBD_GetAutoVIN);
						}
						else if( (Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME) && (g_bFCSRunFlag == true) && (Get_UserStatus() == eUSER_IN) )	//AUTOVIN이 성공이든 실패든 끝난 후 + ENG RUN한지 2초후 시작
						{
							SetOBDState(eOBD_FCS_Start);
						}
					}

					OBDRunningInfo();
					g_bCtrlDBSetFlag = false;
				}
				else	//IG OFF거나 ACC인 상태
				{
					OBDRunningInfo_ControlDB();
					g_bRunningInfoFirstTimeFlag = true;
				}
			}
			
			if( Get_TmrDelta(Get_Tmr(),g_uiCanStopTime) >= OBD_SLEEP_TIME && g_bOBDSleepIntoFlag==true && (Get_VehicleStatus() == eVEHICLE_STATE_OFF) )
			{
				Trace(0,"Go Sleep\r\n");
				SendObdConfig2System(Get_Odmeter(),Get_GPS_Lat_Origin(),Get_GPS_Lon_Origin(),Get_DoorLock(),Get_DoorOpen(),Get_HeadLamp_State(), g_fFuelLevelPercent);
				Send2MngSysMsg2(eMngObd,eOBDStatus,eIGStatus,false, (stCarReport *)NULL,0);

                g_unBkramTmpOdometer = Get_Odmeter();
                HalDrvWrBkRam((unsigned char*)&g_unBkramTmpOdometer, BKRAM_ODOMETER_ADDR, BKRAM_ODOMETER_SIZE);

				if( g_unBkramTmpOdometer != 0 )
				{
                    g_unBkramClearOdoFlag = ODO_CLEAR_SIGNAL;
                    HalDrvWrBkRam((unsigned char*)&g_unBkramClearOdoFlag, BKRAM_CLEARODO_FLAG_ADDR,BKRAM_CLEARODO_FLAG_SIZE);
				}
				g_uiCanStoploopTime = Get_Tmr();
				s_ucSaveStopTime = g_uiCanStopTime;
				g_bOBDSleepIntoFlag=false;
			}
			else
			{
#if 1	// lwh CFD SLEEP 관련 로직 변경
			  	if(CFD_GetCanFDAdapter() && g_bOBDSleepIntoFlag == false )
				{
				  	if( GetCanFDMainState() != eCanFD_INIT )
					{
						if( CanFDInitHandler() != eCanFdhdResNone)
						{
							Oem_CAN_None_Receive_Set();
							if(GetObdRcvSleepFlag() == true)
							{
								if(m_stCFDCtrl.stVer.usAppVer >= CANFD_SLEEP_RESPONSE_VERSION)
								{
									Send2MngSysMsg2(eMngObd,eRspSleep,0,!CFD_GetCanFDSleepStatus(),(stCarReport *)NULL,0);
									// CanFDSleepStatus() == 0 : Module Reset 
									// CanFDSleepStauts() == 1 : Module Sleep
								}
								else
								{
									Send2MngSysMsg(eMngObd,eRspSleep,0,(stCarReport *)NULL,0);
								}
							}
						}
					}
				}
#endif
				if( Get_TmrDelta(Get_Tmr(),g_uiCanStoploopTime) >= OBD_SLEEP_TIME && g_bOBDSleepIntoFlag == false &&  (Get_VehicleStatus() == eVEHICLE_STATE_OFF) )
				{
					Trace(0,"Go Sleep Loop\r\n");
					SendObdConfig2System(Get_Odmeter(),Get_GPS_Lat_Origin(),Get_GPS_Lon_Origin(),Get_DoorLock(),Get_DoorOpen(),Get_HeadLamp_State(), g_fFuelLevelPercent);
					Send2MngSysMsg2(eMngObd,eOBDStatus,eIGStatus,false, (stCarReport *)NULL,0);
                    g_unBkramTmpOdometer = Get_Odmeter();
					HalDrvWrBkRam((unsigned char*)&g_unBkramTmpOdometer, BKRAM_ODOMETER_ADDR, BKRAM_ODOMETER_SIZE);

					g_uiCanStoploopTime = Get_Tmr();
				}
				if( g_bOBDSleepIntoFlag == false && (s_ucSaveStopTime != g_uiCanStopTime) )	//슬립도중 CAN이 갱신되었으면
				{
					Trace(0,"Go Wakeup\r\n");
					Send2MngSysMsg2(eMngObd,eOBDStatus,eIGStatus,true, (stCarReport *)NULL,0);
					CFD_SetCanFDSleepStatus(false);
					g_bOBDSleepIntoFlag=true;
				}
			}
			break;
		case eOBD_Actuator_Mode:
			if( g_bDBParsingFailFlag == true )
			{
				g_uiActFailReason |=	eRETURNFAIL_TYPE_PARSINGFAIL;
				ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_FAIL,g_uiActFailReason);
//				SetOBDState(eOBD_NONE);
				SetOBDState(eOBD_Running_Info_Mode);
			}
			else		OBDActuatorModeManager();
			break;
		case eOBD_GetFuelLevelCheck:
			if( g_bFuelLevelCheckFlag == true )
			{
				g_bFuelLevelCheckFlag = false;
				if(CFD_GetCanFDAdapter())
				{
					Oem_CAN_Initial_CH1(Highcan2, eCAN_1MBPS); //init
					CFD_SetByPassFlagAllOff();			
				}
				DefaultCanMaskSetDCAN();
			}
			//E03,07C60322B002,07CE62B002,10,1,1,2,8,1,0.7042253, , , , ,255, ,D
			OBDFuelLevelCheck();
			break;
		case eOBD_GetSOH:
			if( g_bSOHTypeDBFlag == true )
			{
				g_bSOHTypeDBFlag = false;
				if(CFD_GetCanFDAdapter())
				{
					Oem_CAN_Initial_CH1(Highcan2, eCAN_1MBPS); //init
					CFD_SetByPassFlagAllOff();			
				}
				DefaultCanMaskSetDCAN();
			}
//			if( g_bSOHCheckFlag == true )
//			{
//				g_bSOHCheckFlag = false;
//				DefaultCanMaskSet0700();
//			}
			//E03,07C60322B002,07CE62B002,10,1,1,2,8,1,0.7042253, , , , ,255, ,D
			OBDSOHCheck();
			break;
		case eOBD_GetBrakeJudder:
			if( g_bBrakeJudderFirstFlag == true )
			{
				g_bBrakeJudderFirstFlag = false;
				if(CFD_GetCanFDAdapter())
				{
					Oem_CAN_Initial_CH1(Highcan2, eCAN_1MBPS); //init
					CFD_SetByPassFlagAllOff();			
				}
				DefaultCanMaskSetDCAN();
			}
			OBDBrakeJudderCheck();
			break;
		case eOBD_GetAutoVIN:
			if( g_bAutoVINRunFlag == true )
			{
				g_bAutoVINRunFlag = false;
				g_stAutoVin.bRxStatus = false;
				s_uiTimeoutTimer = Get_Tmr();
			}
			if( Get_TmrDelta(Get_Tmr(), s_uiTimeoutTimer) > AUTOVIN_TIMEOUT )	//FCS시작후 30초이상 지나면 ERROR간주 TIMEOUT BASIC루틴 적용 프리미엄에서 본적없음
			{
				printf("AUTOVIN TIME OUT\r\n");
				SetOBDState(eOBD_Running_Info_Mode);
				g_bAutoVINRunFlag = true;
			}
			else
			{
				OBDAutoVIN();
			}
			break;
		//VIN은 성공이든 실패든 일단 진행
		case eOBD_GetAutoVIN_Success:
		case eOBD_GetAutoVIN_Fail:
			g_bAutoVINRunFlag = true;
			if( g_bDBParsingFailFlag == true )	SetOBDState(eOBD_FW_DB_Update_Mode);
			else												SetOBDState(eOBD_Running_Info_Mode);
			break;
#ifdef CGW_SECURITY
		case eOBD_CGWAlgorithm:
			OBDCGWAlgorithm();
			break;
#endif
		case eOBD_Comm_Finishing:
			OBDCommFinishing();
			break;

		case eOBD_Sleep_Ready:
			break;
		case eOBD_Selftest:
			break;
        case eOBD_FW_DB_Update_Mode:
			if( g_bFWUpdateFlagFromBLE == TRUE )
			{	//설치프로그램이 비정상 종료일경우 빠져나갈수있도록 수정
				if( BTGetConnectStatus() == 0 )	g_bFWUpdateFlagFromBLE = FALSE;
			}
			else
			{
				if( Get_TmrDelta(Get_Tmr(),g_uiCanStopTime) >= OBD_SLEEP_TIME && g_bOBDSleepIntoFlag==true )
				{
					Trace(0,"Go Sleep\r\n");
					SendObdConfig2System(Get_Odmeter(),Get_GPS_Lat_Origin(),Get_GPS_Lon_Origin(),Get_DoorLock(),Get_DoorOpen(),Get_HeadLamp_State(), g_fFuelLevelPercent);
					Send2MngSysMsg(eMngObd,eOBDStatus,eIGStatus, (stCarReport *)NULL,0);
					s_ucSaveStopTime = g_uiCanStopTime;
					g_bOBDSleepIntoFlag=false;
				}
				else
				{
					if( g_bOBDSleepIntoFlag == false && (s_ucSaveStopTime != g_uiCanStopTime) )	//슬립도중 CAN이 갱신되었으면
					{
						Trace(0,"Go Wakeup\r\n");
	//					Send2MngSysMsg(eMngObd,eOBDStatus,eIGOn, (stCarReport *)NULL,0);
						g_bOBDSleepIntoFlag=true;
					}
				}
			}
			break;
		case eOBD_Selftest_FW_Update_Mode:
			break;
		case eOBD_FOTA:
			if( g_uiFotaTimer == 0 )
			{
				g_uiFotaTimer = Get_Tmr();
			}
			
			if( Get_TmrDelta(Get_Tmr(),g_uiFotaTimer) > 600000 )
			{
				if( s_uiFotaTimerOneMin == 0 )
				{
					Send2MngSysMsg2(eMngObd,eOBDStatus,eIGStatus,false, (stCarReport *)NULL,0);
					s_uiFotaTimerOneMin = Get_Tmr();
				}
				
				if( Get_TmrDelta(Get_Tmr(),s_uiFotaTimerOneMin) > OBD_SLEEP_TIME )
				{
					Send2MngSysMsg2(eMngObd,eOBDStatus,eIGStatus,false, (stCarReport *)NULL,0);
					s_uiFotaTimerOneMin = Get_Tmr();
				}
			}
			break;
		default:
			Trace(0,"not defined ID in OBD Manager\r\n");
			break;
	}
}

void FreezeFrameReport()
{
	unsigned int uiSystemLength[SLAVE_DTC_DATA_MAX],uiWritePos = 0, uiSystemTotalLength = 0;
	unsigned int uiFFTotalLength = 0;
	unsigned short usFFLength[SLAVE_DTC_DATA_MAX];
	int i=0,k=0,z=0;
	unsigned int uiTempExtraCnt=0;
	unsigned char ucTempData[5000],ucTempLoopCnt=0;
	stCarReport stReport;
	
	memset((char*)&stReport,0,sizeof(stCarReport));
	memset( ucTempData, 0x00, sizeof(ucTempData) );
	memset( uiSystemLength, 0x00, sizeof(uiSystemLength) );
	memset( usFFLength, 0x00, sizeof(usFFLength) );
	
	uiWritePos = 2;	//총 길이 넣을만큼 비워둠
	
	ucTempData[uiWritePos++] = g_ucDtcSystemCnt;	//1
	uiSystemLength[i]++;
	g_ucDTCTotalCnt = 0;
	for( i=0; i<g_ucDtcSystemCnt; i++ )	//DTC를 할 수 있는 시스템 갯수
	{
		memcpy(&ucTempData[uiWritePos], &g_stDTCFuncData[i].m_ucECU_ID, sizeof(g_stDTCFuncData[i].m_ucECU_ID));	//7
		uiWritePos = uiWritePos + sizeof( g_stDTCFuncData[i].m_ucECU_ID );
		
		memcpy(&ucTempData[uiWritePos], &g_stDTCFuncData[i].m_usFuncType, sizeof(g_stDTCFuncData[i].m_usFuncType));	//2
		uiWritePos = uiWritePos + sizeof( g_stDTCFuncData[i].m_usFuncType );
		
		memcpy(&ucTempData[uiWritePos], &g_stDTCFuncData[i].m_ucState, sizeof(g_stDTCFuncData[i].m_ucState));	//1
		uiWritePos = uiWritePos + sizeof( g_stDTCFuncData[i].m_ucState );
		
		memcpy(&ucTempData[uiWritePos], &g_stDTCFuncData[i].m_ucUDSFlag, sizeof(g_stDTCFuncData[i].m_ucUDSFlag));	//1
		uiWritePos = uiWritePos + sizeof( g_stDTCFuncData[i].m_ucUDSFlag );
		
		memcpy(&ucTempData[uiWritePos], &g_stDTCFuncData[i].m_nOdometer, sizeof(g_stDTCFuncData[i].m_nOdometer));	//4
		uiWritePos = uiWritePos + sizeof( g_stDTCFuncData[i].m_nOdometer );
		
		memcpy(&ucTempData[uiWritePos], &g_stDTCFuncData[i].m_ucFCSTime, sizeof(g_stDTCFuncData[i].m_ucFCSTime)-1);	//6
		uiWritePos = uiWritePos + sizeof( g_stDTCFuncData[i].m_ucFCSTime )-1;
		
		uiSystemLength[i] = uiSystemLength[i] + sizeof( g_stDTCFuncData[i].m_ucECU_ID ) + sizeof( g_stDTCFuncData[i].m_usFuncType ) + sizeof( g_stDTCFuncData[i].m_ucState ) + 
										sizeof( g_stDTCFuncData[i].m_ucUDSFlag ) + sizeof( g_stDTCFuncData[i].m_nOdometer ) + sizeof( g_stDTCFuncData[i].m_ucFCSTime )-1;	//1+7+2+1+1+4 + (6 -1)= 20
		
		ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCount;
		uiSystemLength[i]++;
		
		uiFFTotalLength=0;
		if( g_stDTCFuncData[i].m_ucDTCCount != 0 )
		{
			g_ucDTCTotalCnt = g_ucDTCTotalCnt + g_stDTCFuncData[i].m_ucDTCCount;
			
			for(k=0; k<g_ucDtcSystemCnt; k++)	//DTC가 있는 시스템 정보를 검색
			{
#ifdef NO_FREEZE_FRAME
				for( z=0; z< g_stDTCFuncData[i].m_ucDTCCount; z++ ) //DTC가 있는 시스템에서 DTC갯수만큼 돌면서 프리즈프레임을 저장
				{
					if( g_stDTCFuncData[i].m_ucUDSFlag == 1 )
					{
						ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 0 ]; //UDS
						ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 1 ]; //UDS
						ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 2 ]; //UDS
						ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 3 ]; //UDS
					}
					else
					{
						ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 0 ]; //CAN
						ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 1 ]; //CAN
						ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 3 ]; //CAN
						ucTempData[uiWritePos++] = 0x20;	//CAN
					}
					usFFLength[z] = 4;	//DTC CAN UDS 상관없이 4byte
					//FuncType
					ucTempData[uiWritePos++]=0;
					ucTempData[uiWritePos++]=0;
					//state
					ucTempData[uiWritePos++]=0;
					//Length
					ucTempData[uiWritePos++]=0;
					ucTempData[uiWritePos++]=0;
					//사이즈가 0이므로 값은 필요없음;
					usFFLength[z]+=5;
				}

				for(z=0; z<g_stDTCFuncData[i].m_ucDTCCount; z++ )	//이 시스템에서 FF의 전체 길이
				{
					uiFFTotalLength = uiFFTotalLength + usFFLength[z];
				}
				break;
#else	//NO_FREEZE_FRAME
				if( g_stActuatorFreezeInfo[ k ].m_ucDTCIndex == i )	//프리즈프레임이 있는 시스템이라면
				{
					for( z=0; z< g_stDTCFuncData[i].m_ucDTCCount; z++ )	//DTC가 있는 시스템에서 DTC갯수만큼 돌면서 프리즈프레임을 저장
					{
						if( g_stDTCFuncData[i].m_ucUDSFlag == 1 )
						{
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 0 ];	//UDS
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 1 ];	//UDS
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 2 ];	//UDS
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 3 ];	//UDS
						}
						else
						{
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 0 ];	//CAN
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 1 ];	//CAN
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 3 ];	//CAN
							ucTempData[uiWritePos++] = 0x20;	//CAN
						}
						usFFLength[z] = 4;	//DTC CAN UDS 상관없이 4byte
						
						memcpy(&ucTempData[uiWritePos], &g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usFreezeFuncType,sizeof(g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usFreezeFuncType)); //2
						uiWritePos = uiWritePos + sizeof(g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usFreezeFuncType);
						
						ucTempData[uiWritePos] = g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_ucState;	//1
						uiWritePos = uiWritePos + sizeof( g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_ucState );
						
						ucTempData[uiWritePos] = g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usLength;	//2
						uiWritePos = uiWritePos + sizeof( g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usLength );
						
						memcpy( &ucTempData[uiWritePos], &g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_ucFreezeFrameData, g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usLength );	//가변
						uiWritePos = uiWritePos + g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usLength;
						
						usFFLength[z] = usFFLength[z] + sizeof( g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usFreezeFuncType ) + sizeof( g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_ucState ) + 
																		sizeof( g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usLength ) + g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usLength;
						//여기까지 한 프리즈 프레임에 대한 길이
					}
					
					for(z=0; z<g_stDTCFuncData[i].m_ucDTCCount; z++ )	//이 시스템에서 FF의 전체 길이
					{
						uiFFTotalLength = uiFFTotalLength + usFFLength[z];
					}
					break;
				}
				else	//프리즈프레임이 없다면
				{
					for( z=0; z< g_stDTCFuncData[i].m_ucDTCCount; z++ )	//DTC가 있는 시스템에서 DTC갯수만큼 돌면서 프리즈프레임을 저장
					{
						if( g_stDTCFuncData[i].m_ucUDSFlag == 1 )
						{
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 0 ];	//UDS
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 1 ];	//UDS
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 2 ];	//UDS
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 3 ];	//UDS
						}
						else
						{
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 0 ];	//CAN
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 1 ];	//CAN
							ucTempData[uiWritePos++] = g_stDTCFuncData[i].m_ucDTCCode[ z * 4 + 3 ];	//CAN
							ucTempData[uiWritePos++] = 0x20;	//CAN
						}
						usFFLength[z] = 4;	//DTC CAN UDS 상관없이 4byte
						//FuncType
						ucTempData[uiWritePos++]=0;
						ucTempData[uiWritePos++]=0;
						//state
						ucTempData[uiWritePos++]=0;
						//Length
						ucTempData[uiWritePos++]=0;
						ucTempData[uiWritePos++]=0;
						//사이즈가 0이므로 값은 필요없음
												
						usFFLength[z] = usFFLength[z] + sizeof( g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usFreezeFuncType ) + sizeof( g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_ucState ) + 
																		sizeof( g_stActuatorFreezeInfo[ k ].m_stFreezeFrame[z].m_usLength );
						//여기까지 한 프리즈 프레임에 대한 길이
					}
					
					for(z=0; z<g_stDTCFuncData[i].m_ucDTCCount; z++ )	//이 시스템에서 FF의 전체 길이
					{
						uiFFTotalLength = uiFFTotalLength + usFFLength[z];
					}
					break;
				}
#endif	//NO_FREEZE_FRAME
			}
		}
		else
		{
			
		}
		uiSystemLength[i] = uiSystemLength[i] + uiFFTotalLength;	//프리즈프레임 총길이
	}
	uiSystemTotalLength=0;
	for( i=0; i<g_ucDtcSystemCnt; i++ )
	{
		uiSystemTotalLength = uiSystemTotalLength + uiSystemLength[i];
	}
	memcpy( &ucTempData[0], &uiSystemTotalLength, 2 );


    if( GetRequestDtcAlram() == false )
    {
        // self check report
        stReport.rpAlram.Dtc.ResponseType = 1;
    }
    else
    {
        // request from server
        stReport.rpAlram.Dtc.ResponseType = 2;
    }
	stReport.rpAlram.Dtc.MILStatus = Get_MILLamp_State();
    stReport.rpAlram.Dtc.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
    stReport.rpAlram.Dtc.OccurredEventUtcTime = GetUTCTime();
    stReport.rpAlram.Dtc.OperationKey = Get_DrivingKey();

	ucTempLoopCnt = ( uiSystemTotalLength + 2 ) / (sizeof(g_ucFreezeFrameData));
	uiTempExtraCnt = ( uiSystemTotalLength + 2  ) % (sizeof(g_ucFreezeFrameData));

#if 0
	for(i=0;i<uiSystemTotalLength+2;i++)
	{
		printf("%02X",ucTempData[i]);
	}
	printf("\r\n");
#endif

	memset( g_ucFreezeFrameData, 0x00, sizeof(g_ucFreezeFrameData) );
	for(i=0; i<=ucTempLoopCnt ; i++)
	{
		if( i == ucTempLoopCnt )	memcpy(&g_ucFreezeFrameData,&ucTempData[i*sizeof(g_ucFreezeFrameData)],uiTempExtraCnt);
		else									memcpy(&g_ucFreezeFrameData,&ucTempData[i*sizeof(g_ucFreezeFrameData)],sizeof(g_ucFreezeFrameData));
#if 0
		if( i == ucTempLoopCnt )
		{
			for(z=0;z<uiTempExtraCnt;z++)
			{
				printf("%02X",g_ucFreezeFrameData[z]);
			}
		}
		else
		{
			for(z=0;z<sizeof(g_ucFreezeFrameData);z++)
			{
				printf("%02X",g_ucFreezeFrameData[z]);
			}
		}
		printf("\r\n");
#endif
		stReport.rpAlram.Dtc.Index = i;
		
		if( g_bEngRunKeepFlag == true )
		{
			printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~DirectFreezeFrame\r\n");
			ReportDirectAlramStatus( eR_AlramDTC, eMESSAGE_EVENT_KEY_NONE, 0 );
		}
    	else
        {
            //ReportDirectAlramStatus( eR_AlramDTC, eMESSAGE_EVENT_KEY_NONE, 0 );
            //Send2MngSysMsg2(eMngObd,eReqReport, eR_AlramDTC,0,&stReport,0);
            uint8_t bActive = 0;
            GetAutolinkConfigProperty(eAutoLinkConfig_ModemActive,(void*)&bActive);
            if( bActive == 1 )
			{
				printf("FreezeFrameEvent\r\n");
                Send2MngModem3(eMngSysMsg,eReqReport, eR_AlramDTC,0,&stReport,0);
			}
    	}
	}
	
	if( GetRequestDtcAlram() == true )	
    {
        ResponseSmartKeyResult(ACTUATOR_TYPE_DTC,eREMOTE_CON_RESULT_SUCCESS,0);
        SetRequestDtcAlram(false);
	}
	
	
	if( g_bIndicatorFirstCheckFlag == true )
	{
		g_bIndicatorFirstCheckFlag = false;
		for( i=0;i<ePOS_INDICATOR_TIMER_MAX;i++)	g_stIndicatorCheck[i].m_uiTimerFromLastOFF = g_stIndicatorCheck[i].m_uiTimerFromLastON = g_stIndicatorCheck[i].m_uiCanStopTimer = Get_Tmr();
		printf("Clear IndicatorTimer_FF\r\n");
//		g_iTimerIndicator3secCallback = HalTimerSetSWTimer(3000, eSWTimer_ONESHOT, CB_Indicator_3secCallback, TRUE);
//		if( g_iTimerIndicator3secCallback != -1) g_bIndicator3secCallbackRunningFlag = true;
	}
	return;
}

void SendEngContinueCode()
{
	int i=0;
	unsigned char ucTemp[12];
	unsigned int uiDataLen;
	stPASSTHRU_MSG stData;
	BOOL bStandardCan;
	unsigned char ucCanBPS=0;

	//////////////////////////////////////////////////
	unsigned int  nCanID = 0;
	unsigned char ucLength = 0;

	stData.ProtocolID = ISO15765;
	
	g_uiCanWriteMsgLength = 0;
	uiDataLen = 10 + 24;	//24 : stPASSTHRU_MSG 의 앞부분
	stData.DataSize = uiDataLen-24;	//10

	for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
	{
		if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_ENGINERUNCO )
		{
			g_ucEngContinueReqIndexPos = i;
			break;
		}
	}
	
	for(int j=0; j<g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_uiCount ; j++)
	{
		nCanID = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal;  
		ucLength = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j ].m_ucLength;
	
		memset(&ucTemp, 0x00, sizeof(ucTemp));
		
		if(0x7FF < nCanID)
		{
			stData.ProtocolID = ISO15765_29BIT;
			ucTemp[0] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal>>24) 	& 0xFF;
			ucTemp[1] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal>>16)   & 0xFF;
			ucTemp[2] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal>>8) 	& 0xFF;
			ucTemp[3] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal)       & 0xFF;
			memcpy(&ucTemp[4],g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucData ,8);
		}
		else
		{
			ucTemp[0] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal>>8) 	& 0xFF;
			ucTemp[1] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal)       & 0xFF;
			memcpy(&ucTemp[2],g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucData ,8);
		}
		
		VCI_SetPassThruProtocolID((unsigned char*)&stData.ProtocolID);
		
		for(i=0; i<sizeof(ucTemp); i++)
		{
			if( ucTemp[i] == 'X' )	//X
			{
				ucTemp[i] = g_sSavedTemperature&0xFF;
			}
			else if( ucTemp[i] == 'Y' )	//Y
			{
				ucTemp[i] = (g_sSavedTemperature>>8)&0xFF;
			}
			else if( ucTemp[i] == 'Z' )	//Z
			{
				ucTemp[i]  = ReplaceData( g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j, 'Z', g_cSavedDefrostState);
			}
			else if( ucTemp[i] == 'L' )	//L
			{
				ucTemp[i]  = ReplaceData( g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j, 'L', 1);
			}
		}
		
		g_ContinueCodeTiming = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j ].m_uiTiming;

		switch( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j ].m_uiCanSpeed )
		{
			case 1024	:	ucCanBPS = eCAN_1MBPS;		break;
			case 500	:	ucCanBPS = eCAN_500KBPS;	break;
			case 250	:	ucCanBPS = eCAN_250KBPS;	break;
			case 125	:	ucCanBPS = eCAN_125KBPS;	break;
			case 100	:	ucCanBPS = eCAN_100KBPS;	break;
			case 50	:	ucCanBPS = eCAN_50KBPS;		break;
			default:	Trace(0,"Not Found CanBPS_R\r\n");	break;
		}

		if(g_stActuatorReqData[ g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j ].m_ucCanLine == HIGHCAN1 )
		{
			Oem_CAN_Initial_CH1(Highcan1, ucCanBPS);
		}
		else if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j ].m_ucCanLine == HIGHCAN2 )
		{
			Oem_CAN_Initial_CH1(Highcan2, ucCanBPS);
		}
		else if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j ].m_ucCanLine == HIGHCAN3 )
		{
			Oem_CAN_Initial_CH2(Highcan3, ucCanBPS);
		}
		else
		{
			Oem_CAN_Initial_CH2(Lowcan1, ucCanBPS);
		}

		//////////////////////////////////////////////////
		//CanFD 로직 보강
		//////////////////////////////////////////////////
		boolean_t     bSendCanFDType = false;
		
		if(CFD_GetCanFDAdapter())
		{
			unsigned char ucMainCanLine = 0;
			unsigned char ucFDCanLine = 0;
			unsigned char ucFDCanBaudRate = 0;
			unsigned char ucFDCanFrame = 0;
			
	
			ucMainCanLine = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine;		
			
			if(ucMainCanLine == HIGHCAN2 || ucMainCanLine == HIGHCAN3)
			{
				ucFDCanLine  	= g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucFDCanLine;
				ucFDCanFrame 	= g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucFDCanFrame;
				ucFDCanBaudRate = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucFDCanBaudRate;
				
				if(ucFDCanLine >= CANFD_SPI_1 && ucFDCanLine <= CANFD_SPI_5)
				{
					CFD_SetTransceiverMode(ucFDCanLine,ucFDCanFrame);
				}

				CFD_SetCanFDConfig(ucFDCanLine,ucFDCanBaudRate);
				CFD_SetTxSpiNumber(ucMainCanLine,ucFDCanLine);

				if(ucLength > 8)
					bSendCanFDType = true;	
			}	
		}
			
		if(bSendCanFDType)
		{
			CFD_SendMultiAllCmd(nCanID,ucLength);
		}
		else
		{
			memcpy(stData.pData,ucTemp ,stData.DataSize);
			if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j ].m_ucCanLine == HIGHCAN1 || g_stActuatorReqData[ g_stActuatorReqInfo[g_ucEngContinueReqIndexPos].m_ucIndexPos+j ].m_ucCanLine == HIGHCAN2 )
			{
				memcpy(&g_stWritePassThruMsg, &stData, uiDataLen);
				memset(&g_OutCanPacket, 0x00, sizeof(stCanPacket));
				CAN_TxParsing(&g_OutCanPacket, &g_stWritePassThruMsg, &bStandardCan, eDCAN);
				CAN_MakeTxSingleFrame(&g_OutCanPacket, bStandardCan, &g_stWritePassThruMsg, eDCAN);
			}
			else
			{
				memcpy(&g_stWritePassThruMsg_L, &stData, uiDataLen);
				memset(&g_OutLCanPacket, 0x00, sizeof(stCanPacket));
				CAN_TxParsing(&g_OutLCanPacket, &g_stWritePassThruMsg_L, &bStandardCan, eLCAN);
				CAN_MakeTxSingleFrame(&g_OutLCanPacket, bStandardCan, &g_stWritePassThruMsg_L, eLCAN);
			}
		}
	}
//	Trace(0,"*********************************8");
	g_uiEngContinueSendTimer = Get_Tmr();
}

void DefaultAllCanMaskSet()	//기본 진단용 셋팅
{
	Oem_CAN_None_Receive_Set();

	Oem_CAN_Initial_CH1(g_stSlaveHWSetInfo.m_ucCanChip, g_stSlaveHWSetInfo.m_ucCanSpeed); //init


	unsigned int m_uiStartMask[14];
	unsigned int m_uiEndMask[14];
	unsigned char ucMaskingCount = 0;
	

	if(CFD_GetCanFDAdapter() && g_stSlaveHWSetInfo.m_ucCanChip == Highcan2)
	{
		ucMaskingCount = 8;
		DefaultMaskSet(m_uiStartMask,m_uiEndMask,ucMaskingCount);
		Oem_CAN_Channel_Masket_Set(g_stSlaveHWSetInfo.m_ucCanCH, STANDARD_CAN, ucMaskingCount, (unsigned int *)m_uiStartMask, (unsigned int *)m_uiEndMask);
	}
	else
	{
		Oem_CAN_Channel_Masket_Set(g_stSlaveHWSetInfo.m_ucCanCH, STANDARD_EXTENDED_CAN, g_stSlaveHWSetInfo.m_uiMaskCount, (unsigned int *)g_stSlaveHWSetInfo.m_uiStartMask, (unsigned int *)g_stSlaveHWSetInfo.m_uiEndMask);
	}
	
	

	Oem_CAN_Initial_CH2(g_stControlHWSetInfo.m_ucCanChip, g_stControlHWSetInfo.m_ucCanSpeed); //init

	if(CFD_GetCanFDAdapter() && g_stControlHWSetInfo.m_ucCanChip == Highcan3)
	{
		ucMaskingCount = 8;
		DefaultMaskSet(m_uiStartMask,m_uiEndMask,ucMaskingCount);
		Oem_CAN_Channel_Masket_Set( g_stControlHWSetInfo.m_ucCanCH, STANDARD_CAN, ucMaskingCount, (unsigned int *)m_uiStartMask, (unsigned int *)m_uiEndMask);
	}
	else
	{
		Oem_CAN_Channel_Masket_Set( g_stControlHWSetInfo.m_ucCanCH, STANDARD_EXTENDED_CAN, g_stControlHWSetInfo.m_uiMaskCount, (unsigned int *)g_stControlHWSetInfo.m_uiStartMask, (unsigned int *)g_stControlHWSetInfo.m_uiEndMask);
	}
	
	DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
	LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
}

void DefaultCanMaskSetDCAN()	//기본 진단용 셋팅
{
	unsigned int uiStartID=0,uiEndID=0;
	uint8_t ucPACVType=0;
	GetAutolinkConfigProperty(eAutoLinkConfig_PACVType,(void*)&ucPACVType);

	if(ucPACVType == CV)
	{
		uiStartID=0x18DA0000,uiEndID=0x18DAFFFF;
	}
	else // if(ucPACVType == PA)
	{
		uiStartID=0x0700,uiEndID=0x07FF;
	}
    
    unsigned int m_uiStartMask[14];
	unsigned int m_uiEndMask[14];
//	unsigned char ucMaskingCount = 0;

    memset(&m_uiStartMask,0x00,sizeof(m_uiStartMask));
    memset(&m_uiEndMask,0x00,sizeof(m_uiEndMask));
	Oem_CAN_None_Receive_Set();

	Oem_CAN_Initial_CH1(1, g_stSlaveHWSetInfo.m_ucCanSpeed); //init
	Oem_CAN_Channel_Masket_Set(g_stSlaveHWSetInfo.m_ucCanCH, STANDARD_EXTENDED_CAN, 1, &uiStartID, &uiEndID);

	Oem_CAN_Initial_CH2(g_stControlHWSetInfo.m_ucCanChip, g_stControlHWSetInfo.m_ucCanSpeed); //init
	Oem_CAN_Channel_Masket_Set( CAN_CHANNEL_2, STANDARD_EXTENDED_CAN, 1, (unsigned int *)m_uiStartMask, (unsigned int *)m_uiEndMask);
	
	DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
	LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
}

void OBDFCS_Mode()
{
	static U8 s_ucParaCounter=0, s_ucTotalCounter=0, s_ucCount =0;

	if( s_ucTotalCounter == 0)	//최초 들어올시 저장
	{
		s_ucTotalCounter = stRequestCnt[eCOMM_DTC_FUNCTION].m_cRequestCnt;	//REQ 총 수
		if(s_ucTotalCounter == 0) // DB 내에 DTC 없을 시 지속적으로 FCS 시도하여 예외처리 추가 
		{
			printf("****There is no DTC data in SLAVE DB\r\n");
			SetOBDState(eOBD_Running_Info_Mode);
		}
		s_ucParaCounter = 0;
		s_ucCount = 0;

		DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
	}

#if 1
	
	if(( s_ucCount == 0 ) && (CFD_GetCanFDAdapter()!= 0))
	{
		if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
		{
			if(VCI_FunctionClassification(eCOMM_CARB_OPEN,0))
			{
				CAN_TxBlockProc();	//실제 REQ
			}
		}
		else
		{
			CAN_TxBlockProc();	//실제 REQ
			if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
			{
				DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
				s_ucCount++;	//다음REQ
			}
		}
	}
	else
	{
		if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
		{
			if(VCI_FunctionClassification(eCOMM_DTC_FUNCTION, s_ucParaCounter))	//REQ를 만든다-VCAN이면 FALSE
			{
				CAN_TxBlockProc();	//실제 REQ
			}
			else	s_ucParaCounter++;	//다음REQ
		}
		else
		{
			CAN_TxBlockProc();	//실제 REQ
			if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
			{
				DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
				s_ucParaCounter++;	//다음REQ

				if( s_ucTotalCounter <= s_ucParaCounter )
				{
					s_ucParaCounter = 0;
					s_ucTotalCounter = 0;
					SetOBDState(eOBD_FreezeFrame);
			}
		}
		}	
	}

#else
	if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
	{
		if(VCI_FunctionClassification(eCOMM_DTC_FUNCTION, s_ucParaCounter))	//REQ를 만든다-VCAN이면 FALSE
		{
			CAN_TxBlockProc();	//실제 REQ
		}
		else	s_ucParaCounter++;	//다음REQ
	}
	else
	{
		CAN_TxBlockProc();	//실제 REQ
		if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
		{
			DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
			s_ucParaCounter++;	//다음REQ

			if( s_ucTotalCounter <= s_ucParaCounter )
			{
				s_ucParaCounter = 0;
				s_ucTotalCounter = 0;
//				SetOBDState(eOBD_Running_Info_Mode);
				SetOBDState(eOBD_FreezeFrame);
//				ReportAlramDtcStatus();
			}
		}
	}
#endif
}

#ifndef NO_FREEZE_FRAME
void ExtractFreezeIndex()
{
	int i,k;

	for(i=0; i<g_ucDtcSystemCnt; i++)
	{
		if( g_stDTCFuncData[i].m_ucDTCCount != 0 )	//DTC REQ 끝난정보를 검색
		{
			for(k=0; k<FREEZE_FRAME_SYS_CNT_MAX; k++)	//FREEZEFRAME REQ 정보를 검색
			{
				if( memcmp( g_stActuatorFreezeData[k].m_cEcuid, g_stDTCFuncData[i].m_ucECU_ID, sizeof(g_stActuatorFreezeData[k].m_cEcuid)) == 0 )
				{
					g_stActuatorFreezeInfo[ g_ucSystemCount ].m_ucIndex 		= k;																																//DTC가 있는 시스템의 인덱스를 저장 - g_stActuatorFreezeData(DB파싱정보)의 몇번째인지
					g_stActuatorFreezeInfo[ g_ucSystemCount ].m_ucDTCIndex 	= i;																																//DTC가 있는 시스템의 DTC데이터인덱스를 저장 - g_stDTCFuncData(DTC정보)의 몇번째인지
					if( g_stDTCFuncData[i].m_ucDTCCount > FREEZE_FRAME_SYS_CNT_MAX )	g_stActuatorFreezeInfo[ g_ucSystemCount ].m_ucDTCCount 	= FREEZE_FRAME_SYS_CNT_MAX;			//DTC가 있는 시스템의 DTC갯수
					else																											g_stActuatorFreezeInfo[ g_ucSystemCount ].m_ucDTCCount 	= g_stDTCFuncData[i].m_ucDTCCount;	//DTC가 있는 시스템의 DTC갯수
					g_ucSystemCount++;																																																//DTC가 있는 시스템의 갯수
					break;
				}
			}
		}
	}
}
#endif

eFREEZE_FRAME_STATE GetFreezeFrameState()
{
	return g_eFreezeFrameState;
}

void SetFreezeFrameState(eFREEZE_FRAME_STATE eState)
{
	g_eFreezeFrameState = eState;
}

#ifndef NO_FREEZE_FRAME
void OBDFreezeFrame_Mode()
{
	eFREEZE_FRAME_STATE eState;

	eState = GetFreezeFrameState();

	if( eState ==  eFREEZE_NONE)
	{
		memset( g_stActuatorFreezeInfo,0x00,sizeof(g_stActuatorFreezeInfo) );
		ExtractFreezeIndex();

		if( g_ucSystemCount == 0 )
		{
			if(g_bSOHTypeDBFlag == true)	SetOBDState(eOBD_GetSOH);
			else
			{
				if( g_stBrakeJudder.bValid == true) SetOBDState(eOBD_GetBrakeJudder);
			else		SetOBDState(eOBD_Running_Info_Mode);
			}
			FreezeFrameReport();	//서버로 보내자
			return;
		}
		g_ucSystemIndexFFinfo = 0;	//프리즈프레임필요한시스템중 몇번째 시스템인지
		DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
		SetFreezeFrameState(eFREEZE_OPEN);
	}
	else if( eState == eFREEZE_OPEN )
	{
		if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
		{
			if(Freeze_FunctionClassification(eFREEZE_OPEN, g_stActuatorFreezeInfo[g_ucSystemIndexFFinfo].m_ucIndex))	//REQ를 만든다-VCAN이면 FALSE
			{
				CAN_TxBlockProc();	//실제 REQ
			}
			else
			{
				SetFreezeFrameState(eFREEZE_NONE);
				if(g_bSOHTypeDBFlag == true)	SetOBDState(eOBD_GetSOH);
				else
				{
					if( g_stBrakeJudder.bValid == true) SetOBDState(eOBD_GetBrakeJudder);
				else		SetOBDState(eOBD_Running_Info_Mode);
				}

			}
		}
		else
		{
			CAN_TxBlockProc();	//실제 REQ
			if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
			{
				DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
				g_ucDTCIndex = 0;
				SetFreezeFrameState(eFREEZE_REQ);
			}
		}
	}
	else if( eState == eFREEZE_CLOSE )
	{
		if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
		{
			if(Freeze_FunctionClassification(eFREEZE_CLOSE, g_stActuatorFreezeInfo[g_ucSystemIndexFFinfo].m_ucIndex))	//REQ를 만든다-VCAN이면 FALSE
			{
				CAN_TxBlockProc();	//실제 REQ
			}
			else
			{
				SetFreezeFrameState(eFREEZE_NONE);
				if(g_bSOHTypeDBFlag == true)	SetOBDState(eOBD_GetSOH);
				else
				{
					if( g_stBrakeJudder.bValid == true) SetOBDState(eOBD_GetBrakeJudder);
				else		SetOBDState(eOBD_Running_Info_Mode);
				}

			}
		}
		else
		{
			CAN_TxBlockProc();	//실제 REQ
			if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 끝
			{
				DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
				g_ucSystemIndexFFinfo++;	//다음라인 실행
				if( g_ucSystemCount == g_ucSystemIndexFFinfo)	//마지막 시스템이면
				{
					SetFreezeFrameState(eFREEZE_NONE);
					if(g_bSOHTypeDBFlag == true)	SetOBDState(eOBD_GetSOH);
					else
					{
						if( g_stBrakeJudder.bValid == true) SetOBDState(eOBD_GetBrakeJudder);
					else		SetOBDState(eOBD_Running_Info_Mode);
					}

					FreezeFrameReport();	//서버로 보내자
				}
				else	//마지막 시스템이 아니면
				{
					SetFreezeFrameState(eFREEZE_OPEN);
				}
			}
		}
	}
	else	//eFREEZE_REQ
	{
		if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
		{
			if(Freeze_FunctionClassification(eFREEZE_REQ, g_stActuatorFreezeInfo[g_ucSystemIndexFFinfo].m_ucIndex))	//REQ를 만든다-VCAN이면 FALSE	g_stDTCFuncData[ g_stActuatorFreezeInfo.m_ucDTCIndex[ ucSysIndex ] ].m_ucDTCCode[ ucDtcIndex * 4 ];
			{
				CAN_TxBlockProc();	//실제 REQ
			}
			else
			{
				SetFreezeFrameState(eFREEZE_NONE);
				if(g_bSOHTypeDBFlag == true)	SetOBDState(eOBD_GetSOH);
				else
				{
					if( g_stBrakeJudder.bValid == true) SetOBDState(eOBD_GetBrakeJudder);
				else		SetOBDState(eOBD_Running_Info_Mode);
				}
			}
		}
		else
		{
			CAN_TxBlockProc();	//실제 REQ
			if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
			{
				DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
				g_ucDTCIndex++;	//해당시스템의 몇번째 DTC인지
				if( g_ucDTCIndex == g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_ucDTCCount || g_ucDTCIndex == FREEZE_FRAME_CNT_MAX )	//시스템의 DTC갯수만큼 돌았으면 Session Close,최대갯수만큼해도 종료
				{
					SetFreezeFrameState(eFREEZE_CLOSE);
				}
			}
		}
	}

//	if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
//	{
//		if(Freeze_FunctionClassification(eCOMM_DTC_FUNCTION, s_ucParaCounter))	//REQ를 만든다-VCAN이면 FALSE
//		{
//			CAN_TxBlockProc();	//실제 REQ
//		}
//		else	s_ucParaCounter++;	//다음REQ
//	}
//	else
//	{
//		CAN_TxBlockProc();	//실제 REQ
//		if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
//		{
//			DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
//			s_ucParaCounter++;	//다음REQ
//
//			if( s_ucTotalCounter == s_ucParaCounter )
//			{
//				s_ucParaCounter = 0;
//				s_ucTotalCounter = 0;
//				SetOBDState(eOBD_Running_Info_Mode);
//				ReportAlramDtcStatus();
//			}
//		}
//	}
}
#endif

#ifndef NO_FREEZE_FRAME
bool Freeze_FunctionClassification(eFREEZE_FRAME_STATE eState,  U8 ucSysIndex )
{
	unsigned long uiProtocolID;
	stPASSTHRU_MSG stData;
	int i;
//	unsigned char s_ucCount=0;

	memset(&stData,0x00, sizeof(stData));
	g_bCARBReceiving = FALSE;

	stData.DataSize = 10;
	switch(eState)
	{
	case eFREEZE_OPEN:
		for( i=0; i< stData.DataSize; i++ )
		{
			stData.pData[i] = AsciiToHex(g_stActuatorFreezeData[ucSysIndex].m_ucOpenReq[i*2], g_stActuatorFreezeData[ucSysIndex].m_ucOpenReq[i*2+1]);
		}
//		s_ucCount = 0;
		break;

	case eFREEZE_CLOSE:
		for( i=0; i< stData.DataSize; i++ )
		{
			stData.pData[i] = AsciiToHex(g_stActuatorFreezeData[ucSysIndex].m_ucCloseReq[i*2], g_stActuatorFreezeData[ucSysIndex].m_ucCloseReq[i*2+1]);
		}
//		s_ucCount = 0;
		break;

	case eFREEZE_REQ:
		for( i=0; i< stData.DataSize; i++ )
		{
			if( (((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[i*2]) == 'X') && ((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[(i*2)+1])=='X'))  || (((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[i*2]) == 'x') && ((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[(i*2)+1])=='x')) )
			{
				if(g_stDTCFuncData[ g_stActuatorFreezeInfo[ ucSysIndex ].m_ucDTCIndex ].m_ucUDSFlag == 1)	//UDS
				{
					stData.pData[i] = g_stDTCFuncData[ g_stActuatorFreezeInfo[ ucSysIndex ].m_ucDTCIndex ].m_ucDTCCode[ g_ucDTCIndex * 4 ];
				}
				else	//CAN
				{
					stData.pData[i] = g_stDTCFuncData[ g_stActuatorFreezeInfo[ ucSysIndex ].m_ucDTCIndex ].m_ucDTCCode[ g_ucDTCIndex * 3 ];
				}
			}
			else if( (((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[i*2]) == 'Y') && ((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[(i*2)+1])=='Y')) || (((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[i*2]) == 'y') && ((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[(i*2)+1])=='y')) )
			{
				if(g_stDTCFuncData[ g_stActuatorFreezeInfo[ ucSysIndex ].m_ucDTCIndex ].m_ucUDSFlag == 1)	//UDS
				{
					stData.pData[i] = g_stDTCFuncData[ g_stActuatorFreezeInfo[ ucSysIndex ].m_ucDTCIndex ].m_ucDTCCode[ g_ucDTCIndex * 4 + 1 ];
				}
				else	//CAN
				{
					stData.pData[i] = g_stDTCFuncData[ g_stActuatorFreezeInfo[ ucSysIndex ].m_ucDTCIndex ].m_ucDTCCode[ g_ucDTCIndex * 3 + 1 ];
				}
			}
			else if( (((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[i*2]) == 'Z') && ((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[(i*2)+1])=='Z')) || (((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[i*2]) == 'z') && ((g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[(i*2)+1])=='z')) )
			{
				if(g_stDTCFuncData[ g_stActuatorFreezeInfo[ ucSysIndex ].m_ucDTCIndex ].m_ucUDSFlag == 1)	//UDS
				{
					stData.pData[i] = g_stDTCFuncData[ g_stActuatorFreezeInfo[ ucSysIndex ].m_ucDTCIndex ].m_ucDTCCode[ g_ucDTCIndex * 4 + 2 ];
				}
				else	//CAN
				{
					stData.pData[i] = g_stDTCFuncData[ g_stActuatorFreezeInfo[ ucSysIndex ].m_ucDTCIndex ].m_ucDTCCode[ g_ucDTCIndex * 3 + 2 ];
				}
			}
			else stData.pData[i] = AsciiToHex(g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[i*2], g_stActuatorFreezeData[ucSysIndex].m_ucFreezeReq[i*2+1]);
		}
		break;

	default:
		GITDebugPrintf("[%s] default!!!!! %d\r\n", __FUNCTION__,eState);
		return 0;
		break;
	}

	uiProtocolID=ISO15765;
	VCI_SetPassThruProtocolID((unsigned char*)&uiProtocolID);

	VCI_ProtocolClassification((unsigned char*)&stData, stData.DataSize+PASSTHRU_DEFAULT_LENGTH);

	return true;
}
#endif

void OBDActuatorModeManager()
{
	if( g_eActuatorType == ACTUATOR_TYPE_NONE ){}
	else if( g_eActuatorType == ACTUATOR_TYPE_ENGINERUN )
	{
		if( FUELTYPE_GET_STATE() == EV_NONE_READY )
		{
			if( g_ucActuatorModeStep == 0 )	//웨이크업
			{
				OBDActuatorModeState( ACTUATOR_TYPE_WAKEUP );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
				{
					g_ucActuatorModeStep++;
				}
				else{}
			}
			else if( g_ucActuatorModeStep == 1 )	//구동하기위한 상태체크
			{
				OBDActuatorModeState( ACTUATOR_TYPE_VEHICLE_CHECK );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CHECK_SUCCESS )
				{
					if( g_bAirConRunFlag == true )	//에어컨구동이 있는경우 에어컨 제어하러감
					{
						g_ucActuatorModeStep++;
					}
					else	//에어컨구동이 없는경우 실패리턴
					{
						ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_AIRCON_FAIL);
						g_ucActuatorModeStep=0;
						SetOBDState(eOBD_Running_Info_Mode);
					}
				}
				else{}
			}
			else if( g_ucActuatorModeStep == 2 )	//IG3ON
			{
				OBDActuatorModeState( ACTUATOR_TYPE_IG3ON );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
				{
					g_ucActuatorModeStep++;
				}
				else{}
			}
			else if( g_ucActuatorModeStep == 3 )	//에어컨 구동
			{
				OBDActuatorModeState( ACTUATOR_TYPE_AIRCON );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
				{
					if( g_bRearDefogRunFlag == true )	//열선구동이 있는경우 열선 제어하러감
					{
						g_ucActuatorModeStep++;
					}
					else	//열선구동이 없는경우 성공리턴
					{
						ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
						g_ucActuatorModeStep=0;
						SetOBDState(eOBD_Running_Info_Mode);
					}
				}
				else{}
			}
			else if( g_ucActuatorModeStep == 4 )	//열선 구동
			{
				OBDActuatorModeState( ACTUATOR_TYPE_REARDEFOG );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
				{
					ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
					g_ucActuatorModeStep=0;
					SetOBDState(eOBD_Running_Info_Mode);
				}
				else{}
			}
		}
	    else if( FUELTYPE_GET_STATE() == ELECTRONIC )
        {
            if( g_ucActuatorModeStep == 0 )	//웨이크업
			{
				OBDActuatorModeState( ACTUATOR_TYPE_WAKEUP );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
				{
					g_ucActuatorModeStep++;
				}
				else{}
			}
			else if( g_ucActuatorModeStep == 1 )	//구동하기위한 상태체크
			{
				OBDActuatorModeState( ACTUATOR_TYPE_VEHICLE_CHECK );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CHECK_SUCCESS )
				{
					if( g_bAirConRunFlag == true )	//에어컨구동이 있는경우 에어컨 제어하러감
					{
						g_ucActuatorModeStep++;
					}
					else	//에어컨구동이 없는경우 실패리턴
					{
						ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_FAIL,eRETURNFAIL_TYPE_AIRCON_FAIL);
						g_ucActuatorModeStep=0;
						SetOBDState(eOBD_Running_Info_Mode);
					}
				}
				else{}
			}
			else if( g_ucActuatorModeStep == 2 )	//IG3ON
			{
				OBDActuatorModeState( ACTUATOR_TYPE_IG3ON );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
				{
					g_ucActuatorModeStep++;
				}
				else{}
			}
			else if( g_ucActuatorModeStep == 3 )	//에어컨 구동
			{
				OBDActuatorModeState( ACTUATOR_TYPE_AIRCON );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
				{
					g_ucActuatorModeStep++;
				}
				else{}
			}
			else if( g_ucActuatorModeStep == 4 )	//엔진 구동
			{
				OBDActuatorModeState( ACTUATOR_TYPE_ENGINERUN );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
				{
					if( g_bRearDefogRunFlag == true )	//열선구동이 있는경우 열선 제어하러감
					{
						g_ucActuatorModeStep++;
					}
					else	//열선구동이 없는경우 성공리턴
					{
						ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
						if( g_bEngRunContinueSuppFlag == true )	//엔진 유지코드 시작합니다 이벤트
						{
							printf("eKeepAliveStart\r\n");
							Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
						}
						g_ucActuatorModeStep=0;
						SetOBDState(eOBD_Running_Info_Mode);
					}
				}
				else{}
			}
			else if( g_ucActuatorModeStep == 5 )	//열선 구동
			{
				OBDActuatorModeState( ACTUATOR_TYPE_REARDEFOG );
				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
				{
					ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
					if( g_bEngRunContinueSuppFlag == true )	//엔진 유지코드 시작합니다 이벤트
					{
						printf("eKeepAliveStart\r\n");
						Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
					}
					g_ucActuatorModeStep=0;
					SetOBDState(eOBD_Running_Info_Mode);
				}
				else{}
			}
        }
        else
        {
    		if( g_bRearDefogDBInFlag == true )	//후방열선 있는타입
    		{
    			if( g_ucActuatorModeStep == 0 )	//웨이크업
    			{
    				OBDActuatorModeState( ACTUATOR_TYPE_WAKEUP );
    				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
    				{
    					g_ucActuatorModeStep++;
    				}
    				else{}
    			}
    			else if( g_ucActuatorModeStep == 1 )	//구동하기위한 상태체크
    			{
    				OBDActuatorModeState( ACTUATOR_TYPE_VEHICLE_CHECK );
    				if( GetActuatorStatus() == ACTUATOR_STATUS_CHECK_SUCCESS )
    				{
    					g_ucActuatorModeStep++;
    				}
    				else{}
    			}
    			else if( g_ucActuatorModeStep == 2 )	//엔진 구동
    			{
    				OBDActuatorModeState( ACTUATOR_TYPE_ENGINERUN );
    				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
    				{
    					if( g_bAirConRunFlag == true )	//에어컨구동이 있는경우 에어컨 제어하러감
    					{
    						g_ucActuatorModeStep++;
    					}
    					else	//에어컨구동이 없는경우 성공리턴
    					{
    						ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
    						if( g_bEngRunContinueSuppFlag == true )	//엔진 유지코드 시작합니다 이벤트
    						{
    							printf("eKeepAliveStart\r\n");
    							Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
    						}
    						g_ucActuatorModeStep=0;
    						SetOBDState(eOBD_Running_Info_Mode);
    					}
    				}
    				else{}
    			}
    			else if( g_ucActuatorModeStep == 3 )	//에어컨 구동
    			{
    				OBDActuatorModeState( ACTUATOR_TYPE_AIRCON );
    				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
    				{
    					if( g_bRearDefogRunFlag == true )	//열선구동이 있는경우 열선 제어하러감
    					{
    						g_ucActuatorModeStep++;
    					}
    					else	//열선구동이 없는경우 성공리턴
    					{
    						ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
    						if( g_bEngRunContinueSuppFlag == true )	//엔진 유지코드 시작합니다 이벤트
    						{
    							printf("eKeepAliveStart\r\n");
    							Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
    						}
    						g_ucActuatorModeStep=0;
    						SetOBDState(eOBD_Running_Info_Mode);
    					}
    				}
    				else{}
    			}
    			else if( g_ucActuatorModeStep == 4 )	//열선 구동
    			{
    				OBDActuatorModeState( ACTUATOR_TYPE_REARDEFOG );
    				if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
    				{
    					ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
    					if( g_bEngRunContinueSuppFlag == true )	//엔진 유지코드 시작합니다 이벤트
    					{
    						printf("eKeepAliveStart\r\n");
    						Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
    					}
    					g_ucActuatorModeStep=0;
    					SetOBDState(eOBD_Running_Info_Mode);
    				}
    				else{}
    			}
    		}
    		else	//후방열선 없는타입
    		{
    			if( g_bAirConRunFlag == true )	//엔진과 공조 같이함
    			{
    				if( g_ucActuatorModeStep == 0 )	//웨이크업
    				{
    					OBDActuatorModeState( ACTUATOR_TYPE_WAKEUP );
    					if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
    					{
    						g_ucActuatorModeStep++;
    					}
    					else{}
    				}
    				if( g_ucActuatorModeStep == 1 )	//구동하기위한 상태체크
    				{
    					OBDActuatorModeState( ACTUATOR_TYPE_VEHICLE_CHECK );
    					if( GetActuatorStatus() == ACTUATOR_STATUS_CHECK_SUCCESS )
    					{
    						g_ucActuatorModeStep++;
    					}
    					else{}
    				}
    				else if( g_ucActuatorModeStep == 2 )	//엔진 구동
    				{
    					OBDActuatorModeState( ACTUATOR_TYPE_ENGINERUN );
    					if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
    					{
    						g_ucActuatorModeStep++;
    					}
    					else{}
    				}
    				else if( g_ucActuatorModeStep == 3 )	//에어컨 구동
    				{
    					OBDActuatorModeState( ACTUATOR_TYPE_AIRCON );
    					if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
    					{
    						ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
    						if( g_bEngRunContinueSuppFlag == true )	//엔진 유지코드 시작합니다 이벤트
    						{
    							printf("eKeepAliveStart\r\n");
    							Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
    						}
    						g_ucActuatorModeStep=0;
    						SetOBDState(eOBD_Running_Info_Mode);
    					}
    					else{}
    				}
    			}
    			else		//엔진구동만 함( 에어컨값이 이상한경우 )
    			{
    				if( g_ucActuatorModeStep == 0 )	//구동하기위한 상태체크
    				{
    					OBDActuatorModeState( ACTUATOR_TYPE_VEHICLE_CHECK );
    					if( GetActuatorStatus() == ACTUATOR_STATUS_CHECK_SUCCESS )
    					{
    						g_ucActuatorModeStep++;
    					}
    					else{}
    				}
    				else if( g_ucActuatorModeStep == 1 )	//엔진 구동
    				{
    					OBDActuatorModeState( ACTUATOR_TYPE_ENGINERUN );
    					if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
    					{
    						ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
    						if( g_bEngRunContinueSuppFlag == true )	//엔진 유지코드 시작합니다 이벤트
    						{
    							printf("eKeepAliveStart\r\n");
    							Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
    						}
    						g_ucActuatorModeStep=0;
    						SetOBDState(eOBD_Running_Info_Mode);
    					}
    					else{}
    				}
    			}
    		}
    	}
	}
	else	//엔진구동이 아니면 웨이크업->상태체크->실행 끝
	{
		if( g_ucActuatorModeStep == 0 )	//웨이크업
		{
			OBDActuatorModeState( ACTUATOR_TYPE_WAKEUP );
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
			{
				g_ucActuatorModeStep++;
			}
			else{}
		}
		else if( g_ucActuatorModeStep == 1 )	//구동하기위한 상태체크
		{
			OBDActuatorModeState( ACTUATOR_TYPE_VEHICLE_CHECK );
			if( GetActuatorStatus() == ACTUATOR_STATUS_CHECK_SUCCESS )
			{
				g_ucActuatorModeStep++;
			}
			else{}
		}
		else if( g_ucActuatorModeStep == 2 )	//실제 구동
		{
			OBDActuatorModeState( g_eActuatorType );
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_SUCCESS )
			{
				if( g_bNoResponseFlag == true )
				{
//					printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
					g_bNoResponseFlag = false;
				}
                else
				{
//					printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
					ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_SUCCESS,0);
				}
				g_ucActuatorModeStep=0;
#if defined(OBD_TEST)
				SetOBDState(eOBD_NONE);
#else
				SetOBDState(eOBD_Running_Info_Mode);
#endif
			}
			else{}
		}
	}

	if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_FAIL || GetActuatorStatus() == ACTUATOR_STATUS_CHECK_FAIL)
	{
		if( g_bDBParsingFailFlag == true || g_bDBParsingFailFlag_Act == true )	g_uiActFailReason |=	eRETURNFAIL_TYPE_PARSINGFAIL;
#if defined(ACTUATOR_LOG)
		printf("RPM : %d\r\n", Get_RPM());
			Trace(ACTUATOR_LOG,"g_uiActFailReason: 0x%X %d\r\n",g_uiActFailReason,g_eActuatorStatus);
#endif
        ResponseSmartKeyResult(g_eActuatorType,eREMOTE_CON_RESULT_FAIL,g_uiActFailReason);
		if( g_bEngRunContinueSuppFlag == true && (g_eActuatorType ==  ACTUATOR_TYPE_ENGINERUN))	g_bEngRunActFlag = false;	//엔진제어는 성공하였으나 에어컨구동시 실패하면 false로 바꾸면서 유지명령이 멈춤
		if( g_bEngRunContinueSuppFlag == true && (g_eActuatorType ==  ACTUATOR_TYPE_AIRCON_STOP))
		{
		  	g_bEngRunKeepFlag = false;
		  	g_bFATCRunActFlag = false;	//OS EV의 경우 에어컨ON은 성공하였으나 에어컨정지시 실패하면 false로 바꾸면서 유지명령이 멈춤
		}
		if( (FUELTYPE_GET_STATE() == EV_NONE_READY) && (g_eActuatorType ==  ACTUATOR_TYPE_ENGINERUN) )
		{
			Set_Actuator(ACTUATOR_TYPE_AIRCON_STOP);
			g_bNoResponseFlag = true;	//내부실행일때 응답보내지 않기위해 사용
		}
		else
		{
			SetOBDState(eOBD_NONE);
		}
		g_uiFirstTimeFlag = 0;
		g_uiActFailReason = eRETURNFAIL_TYPE_NONE;
		g_ucActuatorModeStep=0;
		g_bNoResponseFlag = false;
	}
}

unsigned char SelectReqFuncType(unsigned char ucNum)
{
	if( ucNum == g_stSlaveDBCount.m_ucSlow1Cnt )			return eCOMM_SLOW1_FUNCTION;
	else if( ucNum == g_stSlaveDBCount.m_ucSlow2Cnt )	return eCOMM_SLOW2_FUNCTION;
	else if( ucNum == g_stSlaveDBCount.m_ucSlow3Cnt )	return eCOMM_SLOW3_FUNCTION;
	else if( ucNum == g_stSlaveDBCount.m_ucSlow3Cnt )	return eCOMM_SLOW3_FUNCTION;
	else																			return eCOMM_FAST_FUNCTION;
}

void OBDRunningInfo()
{
	static stFunctionParameter s_stFunctionRequest;	//타입별로 순차적으로 쏘기위해 체크하는 구조체
	static U8 s_ucParaCounter=0;
	static U8 s_ucSequenceCounter=0;
	
	if( g_bIndicatorDBFlag == false )
	{
		if( s_ucParaCounter == 0 )
		{
			if( s_stFunctionRequest.ucFunctionType == eCOMM_NONE )	//최초한번은 FAST로 설정
			{
				memset(&s_stFunctionRequest,0x00,sizeof(s_stFunctionRequest));
				s_stFunctionRequest.ucFunctionType = eCOMM_FAST_FUNCTION;
				//현재 주기 F F S1 F F S2 F F S3
				//F S1 S2 S3 주기일때 F갱신약 450ms걸렸음
				g_stSlaveDBCount.m_ucSlow1Cnt=2;
				g_stSlaveDBCount.m_ucSlow2Cnt=5;
				g_stSlaveDBCount.m_ucSlow3Cnt=8;
				g_stSlaveDBCount.m_ucTotalCnt=9;
			}

			s_stFunctionRequest.ucParaTotalCnt = stRequestCnt[s_stFunctionRequest.ucFunctionType].m_cRequestCnt;		//타입별 REQ 총 수

			if(s_stFunctionRequest.ucParaTotalCnt==0)	//REQ가 없을때 ex)Slow1이 비어있는경우
			{
	//			s_stFunctionRequest.ucFunctionType++;	//다음타입 ex)eCOMM_FAST_FUNCTION -> eCOMM_SLOW1_FUNCTION -> eCOMM_SLOW2_FUNCTION
				s_ucSequenceCounter++;
				s_stFunctionRequest.ucFunctionType = SelectReqFuncType(s_ucSequenceCounter);

				if( s_ucSequenceCounter == g_stSlaveDBCount.m_ucTotalCnt )
					s_ucSequenceCounter = 0;
			}
			else
			{
				if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
				{
					if(VCI_FunctionClassification(s_stFunctionRequest.ucFunctionType, s_ucParaCounter))	//REQ를 만든다-VCAN이면 FALSE
					{
						CAN_TxBlockProc();	//실제 REQ
					}
					else	s_ucParaCounter++;	//다음REQ
				}
				else
				{
					CAN_TxBlockProc();	//실제 REQ
					if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
					{
						DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
						s_ucParaCounter++;	//다음REQ
					}
				}
			}
		}
		else
		{
			if( s_stFunctionRequest.ucParaTotalCnt == s_ucParaCounter )	//쏠필요없음
			{
	//			s_stFunctionRequest.ucFunctionType++;	//다음타입 ex)eCOMM_FAST_FUNCTION -> eCOMM_SLOW1_FUNCTION -> eCOMM_SLOW2_FUNCTION
				s_ucSequenceCounter++;
				s_stFunctionRequest.ucFunctionType = SelectReqFuncType(s_ucSequenceCounter);

				if( s_ucSequenceCounter == g_stSlaveDBCount.m_ucTotalCnt )
					s_ucSequenceCounter = 0;
				s_ucParaCounter = 0;
			}
			else
			{
				if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
				{
					if(VCI_FunctionClassification(s_stFunctionRequest.ucFunctionType, s_ucParaCounter))	//REQ를 만든다-VCAN이면 FALSE
					{
						CAN_TxBlockProc();	//실제 REQ
					}
					else	s_ucParaCounter++;	//다음REQ
				}
				else
				{
					CAN_TxBlockProc();	//실제 REQ
					if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
					{
						DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
						s_ucParaCounter++;	//다음REQ
					}
				}
			}
		}
	}
	else
	{
		if( s_ucParaCounter == 0 )
		{
			if( s_stFunctionRequest.ucFunctionType == eCOMM_NONE )	//최초한번은 FAST로 설정
			{
				memset(&s_stFunctionRequest,0x00,sizeof(s_stFunctionRequest));
				s_stFunctionRequest.ucFunctionType = eCOMM_FAST_FUNCTION;
				//현재 주기 F F S1 F F S2 F F S3
				//F S1 S2 S3 주기일때 F갱신약 450ms걸렸음
				g_stSlaveDBCount.m_ucSlow1Cnt=2;
				g_stSlaveDBCount.m_ucSlow2Cnt=5;
				g_stSlaveDBCount.m_ucSlow3Cnt=8;
				g_stSlaveDBCount.m_ucTotalCnt=9;
			}

			s_stFunctionRequest.ucParaTotalCnt = stRequestCnt[s_stFunctionRequest.ucFunctionType].m_cRequestCnt;		//타입별 REQ 총 수

			if(s_stFunctionRequest.ucParaTotalCnt==0)	//REQ가 없을때 ex)Slow1이 비어있는경우
			{
	//			s_stFunctionRequest.ucFunctionType++;	//다음타입 ex)eCOMM_FAST_FUNCTION -> eCOMM_SLOW1_FUNCTION -> eCOMM_SLOW2_FUNCTION
				s_ucSequenceCounter++;
				s_stFunctionRequest.ucFunctionType = SelectReqFuncType(s_ucSequenceCounter);

				if( s_ucSequenceCounter == g_stSlaveDBCount.m_ucTotalCnt )
					s_ucSequenceCounter = 0;
			}
			else
			{
				if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
				{
					if(VCI_FunctionClassification(s_stFunctionRequest.ucFunctionType, s_ucParaCounter))	//REQ를 만든다-VCAN이면 FALSE
					{
						CAN_TxBlockProc();	//실제 REQ
					}
					else
					{
						s_ucParaCounter++;	//다음REQ
						DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
					}
				}
				else
				{
					if( DCAN_GET_COMM_STATE() < eCAN_RX_SINGLE_FRAME)
					{
						CAN_TxBlockProc();	//실제 REQ
					}
					
					if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
					{
						DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
						s_ucParaCounter++;	//다음REQ
					}
				}
			}
		}
		else
		{
			if( s_stFunctionRequest.ucParaTotalCnt == s_ucParaCounter )	//쏠필요없음
			{
	//			s_stFunctionRequest.ucFunctionType++;	//다음타입 ex)eCOMM_FAST_FUNCTION -> eCOMM_SLOW1_FUNCTION -> eCOMM_SLOW2_FUNCTION
				s_ucSequenceCounter++;
				s_stFunctionRequest.ucFunctionType = SelectReqFuncType(s_ucSequenceCounter);

				if( s_ucSequenceCounter == g_stSlaveDBCount.m_ucTotalCnt )
					s_ucSequenceCounter = 0;
				s_ucParaCounter = 0;
			}
			else
			{
				if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
				{
					if(VCI_FunctionClassification(s_stFunctionRequest.ucFunctionType, s_ucParaCounter))	//REQ를 만든다-VCAN이면 FALSE
					{
						CAN_TxBlockProc();	//실제 REQ
					}
					else
					{
						s_ucParaCounter++;	//다음REQ
						DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
					}
				}
				else
				{
					if( DCAN_GET_COMM_STATE() < eCAN_RX_SINGLE_FRAME)
					{
						CAN_TxBlockProc();	//실제 REQ
					}
					
					if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
					{
						DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
						s_ucParaCounter++;	//다음REQ
					}
				}
			}
		}
	}
}

void OBDEngineRun()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_ENGINERUN )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{
			CFD_SetByPassFlagAllOff();
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_ENGINERUN, REQ_TYPE);
		SetSysDebugMode(DEBUG_MODE_NONE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Req\r\n");

            g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			while( GetActuatorStatus() != ACTUATOR_STATUS_CTRL_CHECKING )
			{
				OBDActuator_Req();
			}
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();				
				}	
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_ENGINERUN )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_ENGINERUN, RES_TYPE);
				SetSysDebugMode(DEBUG_MODE_MODULES);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
//					SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= 50)	uiDecision = OBDActuator_Res();	//여기시간을 늘리면 첫 유지코드 시간이 늦어져서 시동유지가 안됨
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
//				g_ucActRetry++;
//				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
//				{
//					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
//					g_uiActFailReason = uiDecision;
//					g_ucActRetry=0;
//				}
//				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
				g_ucActRetry++;
				if( 140 <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_CHECKING );
					g_uiActCheckTime=Get_Tmr();
				}
			}
		}
	}
}

void OBDEngineStop()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		if( g_bEngRunKeepFlag == true )	//엔진 유지코드 멈췄어요 이벤트
		{
//			printf("eKeepAliveStop_4\r\n");
			Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStop, (stCarReport *)NULL,0);
			g_bEngRunKeepFlag = false;
		}
		g_bEngRunActFlag = false;
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_ENGINESTOP )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{
			CFD_SetByPassFlagAllOff();
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_ENGINESTOP, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();
				}
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_ENGINESTOP )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_ENGINESTOP, RES_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
//					SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)	uiDecision = OBDActuator_Res();
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
	#if 0
//	static unsigned char s_ucCodeCount = 0;
//	static unsigned int s_uiFrameTime = 5, s_ucCount1 = 0;
//	unsigned long uiProtocolID;
//	unsigned int uiDataLen=10;
//	stPASSTHRU_MSG stData;
//	unsigned int uiStartMask[HAL_CAN_MAX_MASK_CNT];
//	unsigned int uiEndMask[HAL_CAN_MAX_MASK_CNT];
//	static unsigned int s_uiOldTime=0;
//	static BOOL bStandardCan;
//	BYTE ucCode1[10] = {0x01, 0x0E, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
//
//	memset(uiStartMask,0x00,sizeof(uiStartMask));
//	memset(uiEndMask,0x00,sizeof(uiEndMask));
//
//	uiStartMask[0]=uiEndMask[0]=0x0000;
//
//	if( g_uiFirstTimeFlag == 0 )
//	{
//		//Can_Initial();
//		Oem_CAN_Initial_CH2(Lowcan1, CAN_100KBPS);
//		Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_CAN,1, uiStartMask, uiEndMask);
//		g_uiFirstTimeFlag = 1;
//		s_uiOldTime = Get_Tmr();
//	}
//
//	if(Get_TmrDelta(Get_Tmr(),s_uiOldTime) >= s_uiFrameTime)
//	{
//		if( s_ucCodeCount == 0 )
//		{
//			stData.ProtocolID = ISO15765;
//			VCI_SetPassThruProtocolID((unsigned char*)&uiProtocolID);
//			s_uiFrameTime = 50;
//			uiDataLen = 10 + 24;	//24 : stPASSTHRU_MSG 의 앞부분
//			stData.DataSize = uiDataLen-24;
//			memcpy(stData.pData,&ucCode1,stData.DataSize);
//			//VCI_ProtocolClassification((unsigned char*)&stData, uiDataLen);
//			memcpy(&g_stWritePassThruMsg_L, &stData, uiDataLen);
//			g_uiCanWriteMsgLength = 0;
//			memset(&g_OutLCanPacket, 0x00, sizeof(stCanPacket));
//			CAN_TxParsing(&g_OutLCanPacket, &g_stWritePassThruMsg_L, &bStandardCan, eLCAN);
//			CAN_MakeTxSingleFrame(&g_OutLCanPacket, bStandardCan, &g_stWritePassThruMsg_L, eLCAN);
//			s_ucCount1++;
//			if( s_ucCount1 == 3 )
//			{
//				s_ucCount1=0;
//				//s_ucCodeCount++;
//				SetOBDState(eOBD_NONE);
//			}
//		}
//		s_uiOldTime = Get_Tmr();
//	}
#endif
}

void OBDFuelLevelCheck()
{
	static bool s_bFirstTime = TRUE;	
	static unsigned int s_uiTimeoutTimer = 0;

	if(s_bFirstTime == TRUE)
	{
		s_bFirstTime = FALSE;
		s_uiTimeoutTimer = Get_Tmr();
		DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
	}

	if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
	{
		if(VCI_FunctionClassification(eOBD_GetFuelLevelCheck,0))
		{
			CAN_TxBlockProc();	//실제 REQ
		}
	}
	else
	{
		if( Get_TmrDelta(Get_Tmr(),s_uiTimeoutTimer) >= FUELLEVELCHECK_TIMEOUT_TIME )
		{
			DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
			SetOBDState(eOBD_Running_Info_Mode);
			s_bFirstTime = TRUE;
			if(CFD_GetCanFDAdapter())
			{
				CFD_SetByPassFlagAllOn();			
			}
		}
		else
		{
			CAN_TxBlockProc();	//실제 REQ
			if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
			{
				DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
				SetOBDState(eOBD_Running_Info_Mode);
				s_bFirstTime = TRUE;
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();			
				}
			}
		}
	}
}

void OBDSOHCheck()
{
	static bool s_bFirstTime = TRUE;
	static unsigned int s_uiTimeoutTimer = 0;

	if(s_bFirstTime == TRUE)
	{
		s_bFirstTime = FALSE;
		s_uiTimeoutTimer = Get_Tmr();
		DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
	}

	if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
	{
		if(VCI_FunctionClassification(eOBD_GetSOH,0))
		{
			CAN_TxBlockProc();	//실제 REQ
		}
	}
	else
	{
		if( Get_TmrDelta(Get_Tmr(),s_uiTimeoutTimer) >= SOHCHECK_TIMEOUT_TIME )
		{
			DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
			if( g_stBrakeJudder.bValid == true) SetOBDState(eOBD_GetBrakeJudder);
			else								SetOBDState(eOBD_Running_Info_Mode);
			s_bFirstTime = TRUE;
			if(CFD_GetCanFDAdapter())
			{
				CFD_SetByPassFlagAllOn();			
			}
		}
		else
		{
			CAN_TxBlockProc();	//실제 REQ
			if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
			{
				DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
				if( g_stBrakeJudder.bValid == true) SetOBDState(eOBD_GetBrakeJudder);
				else								SetOBDState(eOBD_Running_Info_Mode);
				s_bFirstTime = TRUE;
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();			
				}
			}
		}
	}
}

void OBDBrakeJudderCheck()
{
	static bool s_bFirstTime = TRUE;
	static unsigned int s_uiTimeoutTimer = 0;

	if(s_bFirstTime == TRUE)
	{
		s_bFirstTime = FALSE;
		s_uiTimeoutTimer = Get_Tmr();
		DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
	}

	if( DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING )
	{
		if(VCI_FunctionClassification(eOBD_GetBrakeJudder,0))
		{
			CAN_TxBlockProc();	//실제 REQ
		}
	}
	else
	{
		if( Get_TmrDelta(Get_Tmr(),s_uiTimeoutTimer) >= BRAKEJUDDERCHECK_TIMEOUT_TIME )
		{
			DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
			SetOBDState(eOBD_Running_Info_Mode);
			s_bFirstTime = TRUE;
			if(CFD_GetCanFDAdapter())
			{
				CFD_SetByPassFlagAllOn();			
			}
		}
		else
		{
			CAN_TxBlockProc();	//실제 REQ
			if( DCAN_GET_COMM_STATE() >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
			{
				DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
				SetOBDState(eOBD_Running_Info_Mode);
				s_bFirstTime = TRUE;
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();			
				}
			}
		}
	}
}

void OBDAutoVIN()
{
	unsigned int CanID1[10];
	unsigned int CanID2[10];
	unsigned int uiVINRxCanID[4] = {0x07E8, 0x07EA, 0x07DE, 0x076C};
	unsigned int uiCV_VINRxCanID[2] = {0x18DAF900, 0x18DAF9EF};
	unsigned int uiState=0;
	static unsigned char s_ucCount=0;
	static bool s_bFirstTime = TRUE;
	uint8_t ucAutovinCANLine = 0;
	uint8_t ucPACVType=0;
	
	int i=0;

	GetAutolinkConfigProperty(eAutoLinkConfig_AutovinCANLine,(void*)&ucAutovinCANLine);
	GetAutolinkConfigProperty(eAutoLinkConfig_PACVType,(void*)&ucPACVType);

	if(g_stAutoVin.bRxStatus == true)	//AUTOVIN을 받았으면
	{
		SetOBDState(eOBD_GetAutoVIN_Success);
		//s_bFirstTime = TRUE;
		s_ucCount=0;

		//hexdump(g_stAutoVin.strVinCode, 17);

		if(strncmp((char*)g_FirmwareInfo.m_strVIN,(char*)&g_stAutoVin.strVinCode,DCS_AUTOVIN_SIZE) != 0 && (VIN_ValidCheck() == true) )
		{
			SetVINCode((unsigned char*)&g_stAutoVin.strVinCode);

            //MONI 2018-02-21
            // obd recevied new vin number from car
            // notify to system to control of trasfer or scenario
            stMsgSys stMessage;
            memset((char*)&stMessage,0,sizeof(stMsgSys));

            stMessage.header.id = eMngObd;
            stMessage.header.event = eOBDStatus;
            stMessage.header.subEvent = eNewVin;
            memcpy(stMessage.rpSetting.ObdSetting.carrVin,g_stAutoVin.strVinCode,DCS_AUTOVIN_SIZE);
            Send2MngSys2(&stMessage);
		}
		s_bFirstTime = TRUE;
		return;
	}
	else if(s_ucCount > (COUNT_AUTOVIN_REQ_PACKET)+1)	// COUNT_AUTOVIN_REQ_PACKET + eCOMM_UDS_OPEN
	{
		SetOBDState(eOBD_GetAutoVIN_Fail);
		s_bFirstTime = TRUE;
		s_ucCount=0;
		g_stAutoVin.bRxStatus = true;
		return;
	}

	if(s_bFirstTime == TRUE)
	{
		if(CFD_GetCanFDAdapter())
		{
			Oem_CAN_Initial_CH1(Highcan2, eCAN_1MBPS); //init
			CFD_SetByPassFlagAllOff();
		}

		if(ucAutovinCANLine == HIGHCAN3)
		{
			Oem_CAN_Initial_CH2(Highcan3, eCAN_500KBPS); //init	
		}
		
		s_bFirstTime = FALSE;
		memset(&g_stAutoVin,0x00,sizeof(g_stAutoVin));
		if(ucAutovinCANLine == HIGHCAN3)
		{
			LCAN_SET_COMM_STATE(eLCAN_TX_NONE_PARSING);
		}
		else
		{
		DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
	}
	}
	if(ucAutovinCANLine == HIGHCAN3 )
	{
		uiState = LCAN_GET_COMM_STATE();
	}
	else
	{		
		uiState = DCAN_GET_COMM_STATE();
	}
	if( uiState == eCAN_TX_NONE_PARSING )
	{
		if(ucPACVType == CV)
		{
			if(VCI_FunctionClassification(eCOMM_Get_AutoVIN,0))
			{
				memset(&CanID1,0x00,sizeof(CanID1));
				memset(&CanID2,0x00,sizeof(CanID2));
				Trace(0,"VIN_TX %d ",s_ucCount);
				for(i =0; i<sizeof(uiCV_VINRxCanID)/sizeof(uiCV_VINRxCanID[0]) ; i++)
				{
					CanID1[i] = uiCV_VINRxCanID[i];
					CanID2[i] = uiCV_VINRxCanID[i];
					Trace(0,"%X %X   ",CanID1[i],CanID2[i]);
				}
				Trace(0,"\r\n");
                if(ucAutovinCANLine == HIGHCAN3)
	{
                    Oem_CAN_Initial_CH2(Highcan3, GetCANBaudrate());
                    Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, EXTENDED_CAN,sizeof(uiCV_VINRxCanID)/sizeof(uiCV_VINRxCanID[0]), CanID1, CanID2);
					CanBufferClear(eCLEAR_TYPE_ALL);
					LCAN_TxBlockProc();	//실제 REQ
				}
                else
                {
                    Oem_CAN_Initial_CH1(Highcan1, GetCANBaudrate());
                    Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, EXTENDED_CAN,sizeof(uiCV_VINRxCanID)/sizeof(uiCV_VINRxCanID[0]), CanID1, CanID2);
					CanBufferClear(eCLEAR_TYPE_ALL);
					CAN_TxBlockProc();	//실제 REQ
                }
					
			}
		}
		else //if(ucPACVType == PA)
		{
			if( s_ucCount == 0 )
			{
				if(VCI_FunctionClassification(eCOMM_UDS_OPEN,0))
				{
					memset(&CanID1,0x00,sizeof(CanID1));
					memset(&CanID2,0x00,sizeof(CanID2));
					Trace(0,"VIN_TX %d ",s_ucCount);
					for(i =0; i<sizeof(uiVINRxCanID)/sizeof(uiVINRxCanID[0]) ; i++)
					{
						CanID1[i] = uiVINRxCanID[i];
						CanID2[i] = uiVINRxCanID[i];
						Trace(0,"%X %X   ",CanID1[i],CanID2[i]);
					}
					Trace(0,"OpenCode\r\n");
					Oem_CAN_Initial_CH1(Highcan1, eCAN_500KBPS); //init
					Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN,sizeof(uiVINRxCanID)/sizeof(uiVINRxCanID[0]), CanID1, CanID2);
					CanBufferClear(eCLEAR_TYPE_ALL);
					CAN_TxBlockProc();	//실제 REQ
				}
			}
			else
			{
				if(VCI_FunctionClassification(eCOMM_Get_AutoVIN,0))
				{
					memset(&CanID1,0x00,sizeof(CanID1));
					memset(&CanID2,0x00,sizeof(CanID2));
					Trace(0,"VIN_TX %d ",s_ucCount);
					for(i =0; i<sizeof(uiVINRxCanID)/sizeof(uiVINRxCanID[0]) ; i++)
					{
						CanID1[i] = uiVINRxCanID[i];
						CanID2[i] = uiVINRxCanID[i];
						Trace(0,"%X %X   ",CanID1[i],CanID2[i]);
					}
					Trace(0,"\r\n");
					Oem_CAN_Initial_CH1(Highcan1, eCAN_500KBPS); //init
					Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN,sizeof(uiVINRxCanID)/sizeof(uiVINRxCanID[0]), CanID1, CanID2);
					CanBufferClear(eCLEAR_TYPE_ALL);
					CAN_TxBlockProc();	//실제 REQ
				}
			}
		}
	}
	else
		{
		if(ucAutovinCANLine == HIGHCAN3)
			{
			LCAN_TxBlockProc();	//실제 REQ
		}
		else
				{
			CAN_TxBlockProc();	//실제 REQ
				}
				
		if(ucAutovinCANLine == HIGHCAN3)
		{
			uiState = LCAN_GET_COMM_STATE();
			}
		else
		{		
			uiState = DCAN_GET_COMM_STATE();
		}
		if( uiState >= eCAN_RX_FAIL )	//성공하거나 실패할경우 다음 REQ
		{
			if(ucAutovinCANLine == HIGHCAN3)
			{
				LCAN_SET_COMM_STATE(eLCAN_TX_NONE_PARSING);
	}
	else
	{
			DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
			}
			s_ucCount++;	//다음시도
		}
	}
}

void OBDCommFinishing()
{
	static unsigned char s_ucDCanReady=0,s_ucLCanReady=0;
	static unsigned char s_ucVCanTimeOutFlag=0;
	//unsigned char uctempbuff[30];
	static unsigned int s_uiOld=0,s_uiNew=0;
	unsigned int uiVCanTimeout=50;	//인의값임 테스트후 수정필요

	if( s_ucVCanTimeOutFlag == 0 )
	{
		s_uiOld = Get_Tmr();
		s_ucVCanTimeOutFlag=1;	//VCAN이 RXBLOCK상태로 계속 있으면 VCanReady가 셋이 안되서 타임아웃개념으로 셋시켜줌
	}

	if(DCAN_GET_COMM_STATE() < eCAN_RX_FAIL)
	{
		if( (DCAN_GET_COMM_STATE() == eCAN_TX_NONE_PARSING) || (DCAN_GET_COMM_STATE() == eCAN_NONE_STATE) )		//NONE파싱상태면 데이터를 쏘려하는데 쏠데이터가 없어서 계속 멈춰있음
		{
			s_ucDCanReady=1;
#if defined(OBD_FINISH_LOG)
			Trace(OBD_FINISH_LOG,"[DD]%dms\r\n", Get_TmrDelta(Get_Tmr(),s_uiOld));
#endif
		}
		else
		{
			CAN_TxBlockProc();	//앞단에서 하던작업이 TX중일 수 있으므로 걸어줘야함
		}
	}
	else
	{
		s_ucDCanReady=1;
#if defined(OBD_FINISH_LOG)
		Trace(OBD_FINISH_LOG,"[D1]%dms\r\n", Get_TmrDelta(Get_Tmr(),s_uiOld));
#endif
	}

//	if(VCAN_GET_COMM_STATE() < eVCAN_RX_FAIL)
//	{
//		if( (VCAN_GET_COMM_STATE() == eVCAN_TX_NONE_PARSING) || (VCAN_GET_COMM_STATE() == eVCAN_NONE_STATE) )	//NONE파싱상태면 데이터를 쏘려하는데 쏠데이터가 없어서 계속 멈춰있음
//		{
//			s_ucVCanReady=1;
//			Trace(0,"[VV]%dms\r\n", Get_TmrDelta(Get_Tmr(),s_uiOld));
//		}
//		else
//		{
//			VCAN_TxBlockProc();	//앞단에서 하던작업이 TX중일 수 있으므로 걸어줘야함
//		}
//	}
//	else
//	{
//		s_ucVCanReady=1;
//		Trace(0,"[V1]%dms\r\n", Get_TmrDelta(Get_Tmr(),s_uiOld));
//	}

	if(LCAN_GET_COMM_STATE() < eLCAN_RX_FAIL)
	{
		if( (LCAN_GET_COMM_STATE() == eLCAN_TX_NONE_PARSING) || (LCAN_GET_COMM_STATE() == eLCAN_NONE_STATE) )		//NONE파싱상태면 데이터를 쏘려하는데 쏠데이터가 없어서 계속 멈춰있음
		{
			s_ucLCanReady=1;
#if defined(OBD_FINISH_LOG)
			Trace(OBD_FINISH_LOG,"[LL]%dms\r\n", Get_TmrDelta(Get_Tmr(),s_uiOld));
#endif
		}
		else
		{
			//LCAN_TxBlockProc();	//앞단에서 하던작업이 TX중일 수 있으므로 걸어줘야함
		}
	}
	else
	{
#if defined(OBD_FINISH_LOG)
		Trace(OBD_FINISH_LOG,"[L1]%dms\r\n", Get_TmrDelta(Get_Tmr(),s_uiOld));
#endif
		s_ucLCanReady=1;
	}

	if( (uiVCanTimeout < Get_TmrDelta(Get_Tmr(),s_uiOld)) || (CAN_CheckP3MinTimeout()) )	//진단캔이 RX_BLOCK상태인데 비클캔이 게속 들어오면 타임아웃이 날수가 없어서 여기서 타임아웃도 체크해줌
	{
		s_ucLCanReady=1;
		s_ucDCanReady=1;
	}

	if( (s_ucDCanReady==1) && (s_ucLCanReady==1) )
	{
#if defined(OBD_FINISH_LOG)
		Trace(OBD_FINISH_LOG,"[1]%dms ", Get_TmrDelta(Get_Tmr(),s_uiOld));
#endif
		Oem_CAN_None_Receive_Set();	//MASK값 초기화(추후 속도가 문제될시 마스킹이 아니라 통신속도변경으로 데이터를 안받는 방법도 생각해보자)
#if defined(OBD_FINISH_LOG)
		Trace(OBD_FINISH_LOG,"[6]%dms ", Get_TmrDelta(Get_Tmr(),s_uiOld));
#endif
		SetOBDState(eOBD_Comm_Finished);
		DCAN_SET_COMM_STATE(eCAN_NONE_STATE);
		//VCAN_SET_COMM_STATE(eVCAN_NONE_STATE);
		LCAN_SET_COMM_STATE(eLCAN_NONE_STATE);
		s_ucDCanReady=0;
		s_ucLCanReady=0;
		s_ucVCanTimeOutFlag=0;
		CAN_InitVariable();
		CAN_Reinit(g_stGitCommInfo[eCOMM_TYPE_CAN1].pstInQueue);
		CAN_Reinit(g_stGitCommInfo[eCOMM_TYPE_CAN2].pstInQueue);
		g_bRunningInfoFirstTimeFlag = true;
		g_bCtrlDBSetFlag = false;
#if defined(OBD_FINISH_LOG)
		Trace(OBD_FINISH_LOG,"[7]%dms\r\n", Get_TmrDelta(Get_Tmr(),s_uiOld));
#endif
		SetFreezeFrameState(eFREEZE_NONE);
		ActRelationInit();
//		g_bWakeUpEndFlag=false;			//	ActRelationInit()에서 할경우 WAKEUP이후로 진행인 안됨(WAKEUP후 초기화 WAKEUP후 초기화 무한반복)
//		g_bVehicleCheckEndFlag=false;	// ActRelationInit()에서 할경우 WAKEUP이후로 진행인 안됨(WAKEUP후 초기화 WAKEUP후 초기화 무한반복)
	s_uiNew = Get_TmrDelta(Get_Tmr(),s_uiOld);
	Trace(0,"[O]Finish %dms\r\n", s_uiNew);
	}
}

void ActRelationInit()
{
	g_uiActCheckTime=0;
	g_uiFirstTimeFlag=0;
	g_uiActCheckFirstTimeFlag=0;
	g_uiActStartTime=0;
	g_ucSecquence = 0;	//REQ리스트중 몇번째 REQ인지
	g_uiReqCount = 0;		//해당REQ 반복횟수
	g_bActuRunningFlag = false;
	g_ucReqIndexPos=0;
	g_ucResIndexPos=0;
	g_ucActRetry=0;
	g_bWakeUpSettingFlag=false;
	g_eLampCheckState = eLAMP_STATE_NONE;
	g_eHornCheckState = eHORN_STATE_NONE;
	SetActuatorStatus( ACTUATOR_STATUS_NONE );
	Trace(0,"ActRelationInit\r\n");
}

eOBD_STATE GetOBDState(void)
{
	return g_eOBDState;
}

eOBD_STATE GetNextOBDState(void)
{
	return g_eOBDNextState;
}

void SetOBDState(eOBD_STATE state)
{
	char *strState;
	static unsigned char s_ucNextState=0;

	if( state != eOBD_Comm_Finished )
	{
		s_ucNextState = state;
		g_eOBDNextState = state; 
		state = eOBD_Comm_Finishing;
	}
	else if( state == eOBD_Comm_Finished )
	{
		state = (eOBD_STATE)s_ucNextState;
		Trace(0,"eOBD_Comm_Finished->\r\n");
	}

	switch ( state )
	{
	case eOBD_NONE:
		strState = "eOBD_NONE";
		break;
	case eOBD_Error:
		strState = "eOBD_Error";
		break;
	case eConfiguration_Error:
		strState = "eConfiguration_Error";
		break;
	case eOBD_DB_Download_Error:
		strState = "eOBD_DB_Download_Error";
		break;
	case eOBD_CheckCopyProtect:
		strState = "eOBD_CheckCopyProtect";
		break;
	case eOBD_Initialize:
		strState = "eOBD_Initialize";
		break;
	case eOBD_Initialized:
		strState = "eOBD_Initialized";
		break;
	case eOBD_GetAutoVIN:
		strState = "eOBD_GetAutoVIN";
		break;
	case eOBD_GetAutoVIN_Fail:
		strState = "eOBD_GetAutoVIN_Fail";
		break;
	case eOBD_GetAutoVIN_Success:
		strState = "eOBD_GetAutoVIN_Success";
		break;
	case eOBD_Server_Waiting:
		strState = "eOBD_Server_Waiting";
		break;
	case eOBD_Server_Connected:
		strState = "eOBD_Server_Connected";
		break;
	case eOBD_Server_Communication:
		strState = "eOBD_Server_Communication";
		break;
	case eOBD_Running_Start:
		strState = "eOBD_Running_Start";
		break;
	case eOBD_Running_Info_Mode:
		strState = "eOBD_Running_Info_Mode";
		break;
	case eOBD_Actuator_Mode:
		strState = "eOBD_Actuator_Mode";
		break;
	case eOBD_FCS_Start:
		strState = "eOBD_FCS_Start";
		break;
	case eOBD_FreezeFrame:
		strState = "eOBD_FreezeFrame";
		break;
	case eOBD_FW_DB_Update_Mode:
		strState = "eOBD_FW_DB_Update_Mode";
		break;
	case eOBD_Sleep_Ready:
		strState = "eOBD_Sleep_Ready";
		break;
	case eOBD_Sleep_Complete:
		strState = "eOBD_Sleep_Complete";
		break;
	case eOBD_WriteThru_Mode:
		strState = "eOBD_WriteThru_Mode";
		break;
	case eOBD_Idle:
		strState = "eOBD_Idle";
		break;
	case eOBD_Power_Check:
		strState = "eOBD_Power_Check";
		break;
	case eOBD_Comm_Finishing:
		strState = "eOBD_Comm_Finishing";
		break;
	case eOBD_Comm_Finished:
		strState = "eOBD_Comm_Finished";
		break;
	case eOBD_Selftest:
		strState = "eOBD_Selftest";
		break;
	case eOBD_Selftest_FW_Update_Mode:
		strState = "eOBD_Selftest_FW_Update_Mode";
		break;
	case eOBD_FOTA:
		strState = "eOBD_FOTA";
		break;
	case eOBD_GetFuelLevelCheck:
		strState = "eOBD_GetFuelLevelCheck";
		break;
	case eOBD_InitCanFD:
		strState = "eOBD_InitCanFD";
		break;
#ifdef CGW_SECURITY
	case eOBD_CGWAlgorithm:
		strState = "eOBD_CGWAlgorithm";
		break;
#endif	
	default:
		strState = "Unknown State";
		break;
	}

	g_eOBDState = state;
	Trace(0,"[O]:%s(%d)\r\n", strState, state);

	return;
}

BOOL DBParser()
{
#ifdef CHECK_MALLOC
	__iar_dlmalloc_stats();
#endif
 	printf("DBPARSER()\r\n");
	if(GetDataBaseParsing_Master()&&GetDataBaseParsing_Slave())
	{
		if(GetDataBaseParsing_Reference())
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
}

bool DBParser_Actuator()
{
	if(GetDataBaseParsing_Actuator())
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

bool GetDataBaseParsing_Actuator()
{
#if defined(MALLOC_MODIFY)
	U8 ret=TRUE;
	unsigned int uiFlashAddress;
	U8 *p_ucTempData = NULL;

    // 제어 DB의 이름이 없거나 DB 사이즈가 제한 범위보다 크면 리턴
    // DB 체크섬 체크를 하는 것이 제일 좋은 방법임.
    // 추후 해결 바람.(김철훈 책임, 마스터 슬레이브 DB도 마찬가지임.)
	if ( memcmp(g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME) == 0 ||
        (g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize > (UPDATE_TMP_ADDRESS - CAR_CTRL_DB_ADDRESS) ) )
	{
		Trace(0,"*****************************************************\r\n");
		Trace(0,"%s : there has no Control DB \r\n", __FUNCTION__);
		Trace(0,"*****************************************************\r\n");
		return false;
	}

	memset(g_stActuatorReqData,0x00,sizeof(g_stActuatorReqData));
	memset(g_stActuatorResData,0x00,sizeof(g_stActuatorResData));
	memset(g_stActuatorReadyData,0x00,sizeof(g_stActuatorReadyData));
	memset(&g_stActuatorConvertData,0x00,sizeof(g_stActuatorConvertData));
	memset(&g_stActuatorFreezeData,0x00,sizeof(g_stActuatorFreezeData));

#ifdef PROTOCOL18
	memset(&g_stActuatorAirConData,0x00,sizeof(g_stActuatorAirConData)); //dahae
	memset(&g_stActuatorCtrlData,0x00,sizeof(g_stActuatorCtrlData)); //dahae
#if defined(F_TEMPERATURE)
	memset(&g_stActuatorAirCon_F_Data,0x00,sizeof(g_stActuatorAirCon_F_Data)); //dahae
#endif
#endif

	memset(g_stActuatorReqInfo,0x00,sizeof(g_stActuatorReqInfo));
	memset(g_stActuatorResInfo,0x00,sizeof(g_stActuatorResInfo));
	memset(&g_stActuatorReadyInfo,0x00,sizeof(g_stActuatorReadyInfo));
	memset(&g_stActuatorConvertInfo,0x00,sizeof(g_stActuatorConvertInfo));
#ifndef NO_FREEZE_FRAME
	memset(&g_stActuatorFreezeInfo,0x00,sizeof(g_stActuatorFreezeInfo));
#endif
	memset(&g_stControlConfig,0x00,sizeof(g_stControlConfig));

#ifdef CHECK_MALLOC
	printf("GetDataBaseParsing_Actuator\r\n");
	__iar_dlmalloc_stats();
#endif
	p_ucTempData = malloc(g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize+1);
#ifdef CHECK_MALLOC
	printf("GetDataBaseParsing_Actuator\r\n");
	__iar_dlmalloc_stats();
#endif
	if (p_ucTempData == NULL )
	{
		//Color	Foreground	Background
		//black		30	40
		//red		31	41
		//green		32	42
		//yellow	33	43
		//blue		34	44
		//magenta	35	45
		//cyan		36	46
		//white		37	47
		printf("\033[33m !!!!!!!!!!!!!!!!!!p_ucTempData malloc fail so return false \r\n \033[0m");
		free(p_ucTempData);
		return false;
	}

	uiFlashAddress = CAR_CTRL_DB_ADDRESS;
	DecyptCarDB_FromFlash(p_ucTempData, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize);
	p_ucTempData[g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize]=0;

	if( GetActuatorData(p_ucTempData) != TRUE )		ret=false;

	if ( p_ucTempData != NULL )
	{
		free(p_ucTempData);
		p_ucTempData=NULL;
	}
	return ret;
#else	//defined(MALLOC_MODIFY)
	U8 ret=TRUE;
	unsigned int uiFlashAddress;
	U8 *p_ucOriginalData = NULL;
	U8 *p_ucTempData = NULL;

    // 제어 DB의 이름이 없거나 DB 사이즈가 제한 범위보다 크면 리턴
    // DB 체크섬 체크를 하는 것이 제일 좋은 방법임.
    // 추후 해결 바람.(김철훈 책임, 마스터 슬레이브 DB도 마찬가지임.)
	if ( memcmp(g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME) == 0 ||
        (g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize > (UPDATE_TMP_ADDRESS - CAR_CTRL_DB_ADDRESS) ) )
	{
		Trace(0,"*****************************************************\r\n");
		Trace(0,"%s : there has no Control DB \r\n", __FUNCTION__);
		Trace(0,"*****************************************************\r\n");
		return false;
	}

	memset(g_stActuatorReqData,0x00,sizeof(g_stActuatorReqData));
	memset(g_stActuatorResData,0x00,sizeof(g_stActuatorResData));
	memset(g_stActuatorReadyData,0x00,sizeof(g_stActuatorReadyData));
	memset(&g_stActuatorConvertData,0x00,sizeof(g_stActuatorConvertData));
	memset(&g_stActuatorFreezeData,0x00,sizeof(g_stActuatorFreezeData));

#ifdef PROTOCOL18
	memset(&g_stActuatorAirConData,0x00,sizeof(g_stActuatorAirConData)); //dahae
	memset(&g_stActuatorCtrlData,0x00,sizeof(g_stActuatorCtrlData)); //dahae
#endif
    
	memset(g_stActuatorReqInfo,0x00,sizeof(g_stActuatorReqInfo));
	memset(g_stActuatorResInfo,0x00,sizeof(g_stActuatorResInfo));
	memset(&g_stActuatorReadyInfo,0x00,sizeof(g_stActuatorReadyInfo));
	memset(&g_stActuatorConvertInfo,0x00,sizeof(g_stActuatorConvertInfo));
	memset(&g_stActuatorFreezeInfo,0x00,sizeof(g_stActuatorFreezeInfo));
	memset(&g_stControlConfig,0x00,sizeof(g_stControlConfig));

	p_ucOriginalData = malloc(g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize);
#ifdef CHECK_MALLOC
	printf("GetDataBaseParsing_Actuator\r\n");
	__iar_dlmalloc_stats();
#endif
	if (p_ucOriginalData == NULL )
	{
		printf("p_ucOriginalData malloc fail so return false \r\n");
		free(p_ucOriginalData);
		return false;
	}

	p_ucTempData = malloc(g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize);
#ifdef CHECK_MALLOC
	printf("GetDataBaseParsing_Actuator\r\n");
	__iar_dlmalloc_stats();
#endif
	if( p_ucTempData == NULL )
	{
		printf("p_ucTempData malloc fail so return false \r\n");
		free(p_ucTempData);
		return false;
	}

	uiFlashAddress = CAR_CTRL_DB_ADDRESS;
	DecyptCarDB_FromFlash(p_ucOriginalData, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize);

	memcpy(p_ucTempData, p_ucOriginalData, g_FirmwareInfo.AppProperty[eApp_ControlDB].wFirmwareSize);
	if( GetActuatorData(p_ucTempData) != TRUE )		ret=false;

	if ( p_ucOriginalData != NULL )
	{
		free(p_ucOriginalData);
		p_ucOriginalData=NULL;
	}
	if ( p_ucTempData != NULL )
	{
		free(p_ucTempData);
		p_ucTempData=NULL;
	}
	return ret;
#endif
}

bool GetActuatorData(U8 *p_ucTempData)	//강제구동 관련된 데이터를 읽어옴
{
	char *p;
	char *lastPos[2];
	char cReqStartMessage[]="#REQ", cReqEndMessage[]="#ENDREQ";
	char cResStartMessage[]="#RESULTSTATUS", cResEndMessage[]="#ENDRESULTSTATUS";
	char cReadyStartMessage[]="#VEHICLESTATUS", cReadyEndMessage[]="#ENDVEHICLESTATUS";
	char cConvertStartMessage[]="#CONVERT", cConvertEndMessage[]="#ENDCONVERT";
	char cFreezeStartMessage[]="#FREEZEFRAME", cFreezeEndMessage[]="#ENDFREEZEFRAME";
	char cConfigStartMessage[]="#CONFIG", cConfigEndMessage[]="#ENDCONFIG";
#if defined(PROTOCOL18)
#if defined(F_TEMPERATURE)
	char cAirConStartMessage[]="#REF_TEMP", cAirConEndMessage[]="#ENDREF_TEMP"; //dahae
	char cAirCon_F_StartMessage[]="#REF_F_TEMP", cAirCon_F_EndMessage[]="#ENDREF_F_TEMP"; //dahae
#else
	char cAirConStartMessage[]="#REF_TEMP", cAirConEndMessage[]="#ENDREF_TEMP"; //dahae
#endif
	char cCtrlStartMessage[]="#CONTROL", cCtrlEndMessage[]="#ENDCONTROL"; //dahae
#endif
	int nLineSize=0;
	bool bFoundReqStartMessage = false;
	bool bFoundResStartMessage = false;
	bool bFoundReadyStartMessage = false;
	bool bFoundConvertStartMessage = false;
	bool bFoundFreezeStartMessage = false;
	bool bFoundConfigStartMessage = false;

	bool bFoundAirConStartMessage = false; //dahae
//	bool bFoundAirCon_F_StartMessage = false; //dahae
	bool bFoundCtrlStartMessage = false; //dahae
	
	bool bConfigParsingFail = true;

	p=strtok_r((char*)p_ucTempData, "\r\n", &lastPos[0]);

	while(p!=NULL)	//LINE이 NULL
	{
#if defined(ACTUATOR_PARSING_LOG)
	if( (bFoundReqStartMessage==false) && (bFoundResStartMessage==false) && (bFoundReadyStartMessage==false) && (bFoundConvertStartMessage==false) && (bFoundFreezeStartMessage==false) && (bFoundConfigStartMessage==false) )
		Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
		if( strncmp(cReqStartMessage, p, strlen(cReqStartMessage))==0 )	//#REQ를 찾음
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//lastPos[0]는 \r\n 위치, p는
			bFoundReqStartMessage = true;
		}
		else if( strncmp(cResStartMessage, p, strlen(cResStartMessage))==0 )	//#MORNITOR를 찾음
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//lastPos[0]는 \r\n 위치, p는
			bFoundResStartMessage = true;
		}
		else if( strncmp(cReadyStartMessage, p, strlen(cReadyStartMessage))==0 )	//#READY를 찾음
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//lastPos[0]는 \r\n 위치, p는
			bFoundReadyStartMessage = true;
		}
		else if( strncmp(cConvertStartMessage, p, strlen(cConvertStartMessage))==0 )	//#CONVERT 찾음
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//lastPos[0]는 \r\n 위치, p는
			bFoundConvertStartMessage = true;
		}
		else if( strncmp(cFreezeStartMessage, p, strlen(cFreezeStartMessage))==0 )	//#FREEZEFRAME 찾음
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//lastPos[0]는 \r\n 위치, p는
			bFoundFreezeStartMessage = true;
		}
		else if( strncmp(cConfigStartMessage, p, strlen(cConfigStartMessage))==0 )	//#CONFIG 찾음
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//lastPos[0]는 \r\n 위치, p는
			bFoundConfigStartMessage = true;
			bConfigParsingFail = false;
		}
#if defined(PROTOCOL18)
#if defined(F_TEMPERATURE)
		else if( strncmp(cAirConStartMessage, p, strlen(cAirConStartMessage))==0 ) //#REF_TEMP 찾음
		{//dahae
			p=strtok_r(NULL, "\r\n",  &lastPos[0]); //lastPos[0]는 \r\n 위치, p는
			bFoundAirConStartMessage = true;
			
			g_stAirconPayload.ucDBCheck = 0x01;
		}
		else if( strncmp(cAirCon_F_StartMessage, p, strlen(cAirCon_F_StartMessage))==0 ) //#REF_F_TEMP 찾음
		{//dahae
			p=strtok_r(NULL, "\r\n",  &lastPos[0]); //lastPos[0]는 \r\n 위치, p는
			bFoundAirCon_F_StartMessage = true;
			
			g_stAirconPayload.ucDBCheck = 0x01;
		}
#else
		else if( strncmp(cAirConStartMessage, p, strlen(cAirConStartMessage))==0 ) //#REF 찾음
		{//dahae
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//lastPos[0]는 \r\n 위치, p는
			bFoundAirConStartMessage = true;
			
			g_stAirconPayload.ucDBCheck = 0x01;
		}
#endif
		else if( strncmp(cCtrlStartMessage, p, strlen(cCtrlStartMessage))==0 ) //#CONTROL 찾음
		{//dahae
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//lastPos[0]는 \r\n 위치, p는
			bFoundCtrlStartMessage = true;
			g_bCtrlFuncParsingFail = false;
		}
#endif		
		if( strncmp(cReqEndMessage, p, strlen(cReqEndMessage))==0 )	//#ENDREQ를 찾음
		{
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundReqStartMessage = false;
			g_ucReqIndex++;
		}
		else if( strncmp(cResEndMessage, p, strlen(cResEndMessage))==0 )	//#ENDMORNITOR를 찾음
		{
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundResStartMessage = false;
			g_ucResIndex++;
		}
		else if( strncmp(cReadyEndMessage, p, strlen(cReadyEndMessage))==0 )	//#ENDREADY를 찾음
		{
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundReadyStartMessage = false;
		}
		else if( strncmp(cConvertEndMessage, p, strlen(cConvertEndMessage))==0 )	//#ENDCONVERT 찾음
		{
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundConvertStartMessage = false;
		}
		else if( strncmp(cFreezeEndMessage, p, strlen(cFreezeEndMessage))==0 )	//#ENDFREEZEFRAME 찾음
		{
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundFreezeStartMessage = false;
		}
		else if( strncmp(cConfigEndMessage, p, strlen(cConfigEndMessage))==0 )	//#ENDCONFIG 찾음
		{
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundConfigStartMessage = false;
		}
#if defined(PROTOCOL18)
#if defined(F_TEMPERATURE)
		else if( strncmp(cAirConEndMessage, p, strlen(cAirConEndMessage))==0 ) //#ENDREF 찾음
		{  //dahae
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundAirConStartMessage = false;

			//LOW, HIGH 존재 여부 확인 
			if(g_stActuatorAirConData[0].m_usTemp == AIRCON_TEMP_LOW)
			{	
				g_stAirconPayload.ucLowCheck = 0x01; //LOW 존재
				g_stAirconPayload.usMinTemp = g_stActuatorAirConData[1].m_usTemp;
			}
			else
			{
				g_stAirconPayload.usMinTemp = g_stActuatorAirConData[0].m_usTemp;
			}
			
			if(g_stActuatorAirConData[g_ucTotAirCnt-1].m_usTemp == AIRCON_TEMP_HIGH)
			{	
				g_stAirconPayload.ucHighCheck = 0x01; //HIGH 존재 
				g_stAirconPayload.usMaxTemp = g_stActuatorAirConData[g_ucTotAirCnt-2].m_usTemp;
			}
			else
			{	
				g_stAirconPayload.usMaxTemp = g_stActuatorAirConData[g_ucTotAirCnt-1].m_usTemp;
			}
		}
		else if( strncmp(cAirCon_F_EndMessage, p, strlen(cAirCon_F_EndMessage))==0 ) //#ENDREF 찾음
		{  //dahae
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundAirCon_F_StartMessage = false;

			//LOW, HIGH 존재 여부 확인 
			if(g_stActuatorAirCon_F_Data[0].m_usTemp == AIRCON_TEMP_LOW)
			{
				g_stAirconPayload.ucLowCheck = 0x01; //LOW 존재
				g_stAirconPayload.usMinTemp = g_stActuatorAirCon_F_Data[1].m_usTemp;
			}
			else
			{
				g_stAirconPayload.usMinTemp = g_stActuatorAirCon_F_Data[0].m_usTemp;
			}
			
			if(g_stActuatorAirCon_F_Data[g_ucTotAirCnt-1].m_usTemp == AIRCON_TEMP_HIGH)
			{
				g_stAirconPayload.ucHighCheck = 0x01; //HIGH 존재 
				g_stAirconPayload.usMaxTemp = g_stActuatorAirCon_F_Data[g_ucTotAirCnt-2].m_usTemp;
			}
			else
			{
				g_stAirconPayload.usMaxTemp = g_stActuatorAirCon_F_Data[g_ucTotAirCnt-1].m_usTemp;
			}
		}
#else	//F_TEMPERATURE
		else if( strncmp(cAirConEndMessage, p, strlen(cAirConEndMessage))==0 ) //#ENDREF 찾음
		{  //dahae
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundAirConStartMessage = false;

			//LOW, HIGH 존재 여부 확인 
			if(g_stActuatorAirConData[0].m_usTemp == AIRCON_TEMP_LOW)
			{	
				g_stAirconPayload.ucLowCheck = 0x01; //LOW 존재
				g_stAirconPayload.usMinTemp = g_stActuatorAirConData[1].m_usTemp;
			}
			else
			{
				g_stAirconPayload.usMinTemp = g_stActuatorAirConData[0].m_usTemp;
			}
			
			if(g_stActuatorAirConData[g_ucTotAirCnt-1].m_usTemp == AIRCON_TEMP_HIGH)
			{	
				g_stAirconPayload.ucHighCheck = 0x01; //HIGH 존재 
				g_stAirconPayload.usMaxTemp = g_stActuatorAirConData[g_ucTotAirCnt-2].m_usTemp;
			}
			else
			{	
				g_stAirconPayload.usMaxTemp = g_stActuatorAirConData[g_ucTotAirCnt-1].m_usTemp;
			}
		}
#endif	//F_TEMPERATURE
		else if( strncmp(cCtrlEndMessage, p, strlen(cCtrlEndMessage))==0 ) //#ENDCONTROL 찾음
		{
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			bFoundCtrlStartMessage= false;		
		}
#endif		
		if ( bFoundReqStartMessage == true )
		{
			nLineSize = sizeof(char) * (lastPos[0]- p);
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			GetReqData(p,nLineSize);	//데이터를 한라인씩 저장
		}
		else if ( bFoundResStartMessage == true )
		{
			nLineSize = sizeof(char) * (lastPos[0]- p);
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			GetResData(p,nLineSize);	//데이터를 한라인씩 저장
		}
		else if ( bFoundReadyStartMessage == true )
		{
			nLineSize = sizeof(char) * (lastPos[0]- p);
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			GetVehicleStatusData(p,nLineSize);	//데이터를 한라인씩 저장
		}
		else if ( bFoundConvertStartMessage == true )
		{
			nLineSize = sizeof(char) * (lastPos[0]- p);
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			GetConvertData(p,nLineSize);	//데이터를 한라인씩 저장
		}
		else if ( bFoundFreezeStartMessage == true )
		{
			nLineSize = sizeof(char) * (lastPos[0]- p);
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			GetFreezeData(p,nLineSize);	//데이터를 한라인씩 저장
		}
		else if ( bFoundConfigStartMessage == true )
		{
			nLineSize = sizeof(char) * (lastPos[0]- p);
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			GetConfigData(p,nLineSize);	//데이터를 한라인씩 저장
		}
#if defined(PROTOCOL18)
		else if (bFoundAirConStartMessage == true ) //dahae
		{
			nLineSize = sizeof(char) * (lastPos[0]- p);
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			GetAirConData(p,nLineSize);	//데이터를 한라인씩 저장
		}
#if defined(F_TEMPERATURE)
		else if (bFoundAirConStartMessage == true ) //dahae
		{
			nLineSize = sizeof(char) * (lastPos[0]- p);
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			GetAirConData(p,nLineSize);	//데이터를 한라인씩 저장
		}
#endif
		else if (bFoundCtrlStartMessage == true ) //dahae
		{
			nLineSize = sizeof(char) * (lastPos[0]- p);
#if defined(ACTUATOR_PARSING_LOG)
	Trace(ACTUATOR_PARSING_LOG,"%s\r\n",p);
#endif
			GetCtrlData(p,nLineSize);	//데이터를 한라인씩 저장		
		}
#endif
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);//다음 LINE
	}
	if ( (g_ucReqIndex==0) && (g_ucResIndex==0) ) return false;		//원하는 펑션을 찾지못하면 FAIL
	if( bConfigParsingFail == true )
	{
		g_stControlConfig.m_ucAVNVersion = 4;
		g_stControlConfig.m_ucAVNType = 1;
		g_stControlConfig.m_ucEngStopType = 1;
		g_stControlConfig.m_uiWaitTime = 3000;
	}
#if defined(ACTUATOR_PARSING_LOG)
	int i=0,j=0,k=0;

	Trace(ACTUATOR_PARSING_LOG,"g_stActuatorReqData\r\n");
	for(j=0;j<g_ucTotReqCnt;j++)
	{
		Trace(ACTUATOR_PARSING_LOG,"%d\r\n",j+1);
		Trace(ACTUATOR_PARSING_LOG,"m_uiFuncIndex: %04d\r\n",g_stActuatorReqData[j].m_uiFuncIndex);
		Trace(ACTUATOR_PARSING_LOG,"m_ucWakeupUsed: %d\r\n",g_stActuatorReqData[j].m_ucWakeupUsed);
		Trace(ACTUATOR_PARSING_LOG,"m_ucRetry: %d\r\n",g_stActuatorReqData[j].m_ucRetry);
		Trace(ACTUATOR_PARSING_LOG,"m_ucCanLine: %d\r\n",g_stActuatorReqData[j].m_ucCanLine);
		Trace(ACTUATOR_PARSING_LOG,"m_uiCanSpeed: %d\r\n",g_stActuatorReqData[j].m_uiCanSpeed);
		if(0x7FF < g_stActuatorReqData[j].m_uiReqVal)
		{
			Trace(ACTUATOR_PARSING_LOG,"m_uiReqVal: %08X\r\n",g_stActuatorReqData[j].m_uiReqVal);
			Trace(ACTUATOR_PARSING_LOG,"m_uiResVal: %08X\r\n",g_stActuatorReqData[j].m_uiResVal);
		}
		else
		{
			Trace(ACTUATOR_PARSING_LOG,"m_uiReqVal: %04X\r\n",g_stActuatorReqData[j].m_uiReqVal);
			Trace(ACTUATOR_PARSING_LOG,"m_uiResVal: %04X\r\n",g_stActuatorReqData[j].m_uiResVal);
		}
		Trace(ACTUATOR_PARSING_LOG,"m_ucLength: %d\r\n",g_stActuatorReqData[j].m_ucLength);
		Trace(ACTUATOR_PARSING_LOG,"m_ucData: ");
		for(i=0;i<sizeof(g_stActuatorReqData[j].m_ucData);i++)
		{
			Trace(ACTUATOR_PARSING_LOG,"%02X ",g_stActuatorReqData[j].m_ucData[i]);
		}
		Trace(ACTUATOR_PARSING_LOG,"\r\n");
		Trace(ACTUATOR_PARSING_LOG,"m_uiTiming: %d\r\n",g_stActuatorReqData[j].m_uiTiming);
		Trace(ACTUATOR_PARSING_LOG,"m_uiTimes: %d\r\n",g_stActuatorReqData[j].m_uiTimes);
		Trace(ACTUATOR_PARSING_LOG,"m_uiMin: %d\r\n",g_stActuatorReqData[j].m_uiMin);
		Trace(ACTUATOR_PARSING_LOG,"m_uiMax: %d\r\n",g_stActuatorReqData[j].m_uiMax);
		Trace(ACTUATOR_PARSING_LOG,"m_fConvert: %f\r\n",g_stActuatorReqData[j].m_fConvert);
		Trace(ACTUATOR_PARSING_LOG,"\r\n");
	}
	Trace(ACTUATOR_PARSING_LOG,"--------------------------------------------------------------------------------------\r\n");
	Trace(ACTUATOR_PARSING_LOG,"g_stActuatorConvertData\r\n");
	for(j=0;j<g_ucTotConvertCnt;j++)
	{
		Trace(ACTUATOR_PARSING_LOG,"%d\r\n",j+1);
		Trace(ACTUATOR_PARSING_LOG,"m_uiFuncIndex:%04d\r\n",g_stActuatorConvertData[j].m_uiFuncIndex);
		Trace(ACTUATOR_PARSING_LOG,"m_ucSeparator:%c\r\n",g_stActuatorConvertData[j].m_ucSeparator);
		Trace(ACTUATOR_PARSING_LOG,"m_ucOn:0x%02X\r\n",g_stActuatorConvertData[j].m_ucOn);
		Trace(ACTUATOR_PARSING_LOG,"m_ucOff:0x%02X\r\n",g_stActuatorConvertData[j].m_ucOff);
		Trace(ACTUATOR_PARSING_LOG,"\r\n");
	}

	Trace(ACTUATOR_PARSING_LOG,"--------------------------------------------------------------------------------------\r\n");
	Trace(ACTUATOR_PARSING_LOG,"g_stActuatorResData\r\n");
	for(j=0;j<g_ucTotResCnt;j++)
	{
		Trace(ACTUATOR_PARSING_LOG,"%d\r\n",j+1);
		Trace(ACTUATOR_PARSING_LOG,"m_uiFuncIndex:%04d\r\n",g_stActuatorResData[j].m_uiFuncIndex);
		Trace(ACTUATOR_PARSING_LOG,"m_ucCanLine:%d\r\n",g_stActuatorResData[j].m_ucCanLine);
		Trace(ACTUATOR_PARSING_LOG,"m_uiCanSpeed:%d\r\n",g_stActuatorResData[j].m_uiCanSpeed);
		Trace(ACTUATOR_PARSING_LOG,"m_cIndex:%c%c%c\r\n",g_stActuatorResData[j].m_cIndex[0],g_stActuatorResData[j].m_cIndex[1],g_stActuatorResData[j].m_cIndex[2]);
		if(0x7FF < g_stActuatorResData[j].m_uiReqVal)
		{
			Trace(ACTUATOR_PARSING_LOG,"m_uiReqVal: %08X\r\n",g_stActuatorResData[j].m_uiReqVal);
			Trace(ACTUATOR_PARSING_LOG,"m_uiResVal: %08X\r\n",g_stActuatorResData[j].m_uiResVal);
		}
		else
		{
			Trace(ACTUATOR_PARSING_LOG,"m_uiReqVal: %04X\r\n",g_stActuatorResData[j].m_uiReqVal);
			Trace(ACTUATOR_PARSING_LOG,"m_uiResVal: %04X\r\n",g_stActuatorResData[j].m_uiResVal);
		}
		Trace(ACTUATOR_PARSING_LOG,"m_ucStartPos:%d\r\n",g_stActuatorResData[j].m_ucStartPos);
		Trace(ACTUATOR_PARSING_LOG,"m_ucRealPos:%d\r\n",g_stActuatorResData[j].m_ucRealPos);
		Trace(ACTUATOR_PARSING_LOG,"m_ucDataSize:%d\r\n",g_stActuatorResData[j].m_ucDataSize);
		Trace(ACTUATOR_PARSING_LOG,"m_ucMsbLsb:%d\r\n",g_stActuatorResData[j].m_ucMsbLsb);
		Trace(ACTUATOR_PARSING_LOG,"m_uiMaskVal:%04X\r\n",g_stActuatorResData[j].m_uiMaskVal);
		Trace(ACTUATOR_PARSING_LOG,"m_ucConvrule:%d\r\n",g_stActuatorResData[j].m_ucConvrule);
		Trace(ACTUATOR_PARSING_LOG,"m_A:%f\r\n",g_stActuatorResData[j].m_fA);
		Trace(ACTUATOR_PARSING_LOG,"m_B:%f\r\n",g_stActuatorResData[j].m_fB);
		Trace(ACTUATOR_PARSING_LOG,"m_C:%f\r\n",g_stActuatorResData[j].m_fC);
		Trace(ACTUATOR_PARSING_LOG,"m_D:%d\r\n",g_stActuatorResData[j].m_cD);
		Trace(ACTUATOR_PARSING_LOG,"m_E:%d\r\n",g_stActuatorResData[j].m_cE);
		Trace(ACTUATOR_PARSING_LOG,"m_F:%d\r\n",g_stActuatorResData[j].m_cF);
		//Trace(ACTUATOR_PARSING_LOG,"Lut:%s\r\n",g_stActuatorResData[j].pucArrLut);
		Trace(ACTUATOR_PARSING_LOG,"m_cCompType:%d\r\n",g_stActuatorResData[j].m_cCompType);
		Trace(ACTUATOR_PARSING_LOG,"m_uiCompVal:%d\r\n",g_stActuatorResData[j].m_uiCompVal);
		Trace(ACTUATOR_PARSING_LOG,"\r\n\r\n");
	}

	Trace(ACTUATOR_PARSING_LOG,"--------------------------------------------------------------------------------------\r\n");
	Trace(ACTUATOR_PARSING_LOG,"g_stActuatorReadyData\r\n");
	for(j=0;j<g_ucTotReadyCnt;j++)
	{
		Trace(ACTUATOR_PARSING_LOG,"%d\r\n",j+1);
		Trace(ACTUATOR_PARSING_LOG,"m_uiFuncIndex:%04d\r\n",g_stActuatorReadyData[j].m_uiFuncIndex);
		Trace(ACTUATOR_PARSING_LOG,"m_ucWakeupUsed:%d\r\n",g_stActuatorReadyData[j].m_ucWakeupUsed);
		Trace(ACTUATOR_PARSING_LOG,"m_ucCanLine:%d\r\n",g_stActuatorReadyData[j].m_ucCanLine);
		Trace(ACTUATOR_PARSING_LOG,"m_uiCanSpeed:%d\r\n",g_stActuatorReadyData[j].m_uiCanSpeed);
		Trace(ACTUATOR_PARSING_LOG,"m_cIndex:%c%c%c\r\n",g_stActuatorReadyData[j].m_cIndex[0],g_stActuatorReadyData[j].m_cIndex[1],g_stActuatorReadyData[j].m_cIndex[2]);
		if(0x7FF < g_stActuatorReadyData[j].m_uiReqVal)
		{
			Trace(ACTUATOR_PARSING_LOG,"m_uiReqVal: %08X\r\n",g_stActuatorReadyData[j].m_uiReqVal);
			Trace(ACTUATOR_PARSING_LOG,"m_uiResVal: %08X\r\n",g_stActuatorReadyData[j].m_uiResVal);
		}
		else
		{
			Trace(ACTUATOR_PARSING_LOG,"m_uiReqVal: %04X\r\n",g_stActuatorReadyData[j].m_uiReqVal);
			Trace(ACTUATOR_PARSING_LOG,"m_uiResVal: %04X\r\n",g_stActuatorReadyData[j].m_uiResVal);
		}
		Trace(ACTUATOR_PARSING_LOG,"m_ucStartPos:%d\r\n",g_stActuatorReadyData[j].m_ucStartPos);
		Trace(ACTUATOR_PARSING_LOG,"m_ucRealPos:%d\r\n",g_stActuatorReadyData[j].m_ucRealPos);
		Trace(ACTUATOR_PARSING_LOG,"m_ucDataSize:%d\r\n",g_stActuatorReadyData[j].m_ucDataSize);
		Trace(ACTUATOR_PARSING_LOG,"m_ucMsbLsb:%d\r\n",g_stActuatorReadyData[j].m_ucMsbLsb);
		Trace(ACTUATOR_PARSING_LOG,"m_uiMaskVal:%04X\r\n",g_stActuatorReadyData[j].m_uiMaskVal);
		Trace(ACTUATOR_PARSING_LOG,"m_ucConvrule:%d\r\n",g_stActuatorReadyData[j].m_ucConvrule);
		Trace(ACTUATOR_PARSING_LOG,"m_A:%f\r\n",g_stActuatorReadyData[j].m_fA);
		Trace(ACTUATOR_PARSING_LOG,"m_B:%f\r\n",g_stActuatorReadyData[j].m_fB);
		Trace(ACTUATOR_PARSING_LOG,"m_C:%f\r\n",g_stActuatorReadyData[j].m_fC);
		Trace(ACTUATOR_PARSING_LOG,"m_D:%d\r\n",g_stActuatorReadyData[j].m_cD);
		Trace(ACTUATOR_PARSING_LOG,"m_E:%d\r\n",g_stActuatorReadyData[j].m_cE);
		Trace(ACTUATOR_PARSING_LOG,"m_F:%d\r\n",g_stActuatorReadyData[j].m_cF);
		//Trace(ACTUATOR_PARSING_LOG,"Lut:%s\r\n",g_stActuatorResData[j].pucArrLut);
		Trace(ACTUATOR_PARSING_LOG,"\r\n");
//		for(i=0;i<sizeof(g_stActuatorReadyData[j].m_ucUseFunction);i++)
//		{
//			Trace(ACTUATOR_PARSING_LOG,"%c",g_stActuatorReadyData[j].m_ucUseFunction[i]);
//		}
		Trace(ACTUATOR_PARSING_LOG,"\r\n\r\n");
	}
    
#if defined(PROTOCOL18)        
	Trace(ACTUATOR_PARSING_LOG,"--------------------------------------------------------------------------------------\r\n"); //dahae
	Trace(ACTUATOR_PARSING_LOG,"g_stActuatorCtrlData\r\n");    

	for(j=0;j<g_ucTotCtrlCnt;j++){
			Trace(ACTUATOR_PARSING_LOG,"m_uiFuncIndex:%02d\r\n",g_stActuatorCtrlData[j].m_uiFuncIndex);
			Trace(ACTUATOR_PARSING_LOG,"m_ucValue1:%02X\r\n",g_stActuatorCtrlData[j].m_ucCtrlFunction);
			Trace(ACTUATOR_PARSING_LOG,"m_ucValue1:%d\r\n",g_stActuatorCtrlData[j].m_ucSupport);
			Trace(ACTUATOR_PARSING_LOG,"\r\n\r\n");
		}
	Trace(ACTUATOR_PARSING_LOG,"--------------------------------------------------------------------------------------\r\n"); //dahae
	Trace(ACTUATOR_PARSING_LOG,"g_stActuatorAirConData\r\n");

	for(j=0;j<g_ucTotAirCnt;j++){
		Trace(ACTUATOR_PARSING_LOG,"m_usTemp:%04d\r\n",g_stActuatorAirConData[j].m_usTemp);
		Trace(ACTUATOR_PARSING_LOG,"m_ucValue1:%02X\r\n",g_stActuatorAirConData[j].m_ucValue1);
		Trace(ACTUATOR_PARSING_LOG,"m_ucValue1:%02X\r\n",g_stActuatorAirConData[j].m_ucValue2);
		Trace(ACTUATOR_PARSING_LOG,"m_ucValue1:%02X\r\n",g_stActuatorAirConData[j].m_ucValue3);
		Trace(ACTUATOR_PARSING_LOG,"\r\n\r\n");
	}
#endif
	
	for(j=0;j<g_ucReqIndex;j++)
	{
		Trace(ACTUATOR_PARSING_LOG,"m_uiFuncIndex:%04d\r\n",g_stActuatorReqInfo[j].m_uiFuncIndex);
		Trace(ACTUATOR_PARSING_LOG,"m_uiCount:%d\r\n",g_stActuatorReqInfo[j].m_uiCount);
		for(i=0; i<g_stActuatorReqInfo[j].m_uiCount; i++)
		{
			Trace(ACTUATOR_PARSING_LOG,"%d : ",i);
			for(k=0; k<10; k++)	Trace(ACTUATOR_PARSING_LOG,"%02X ",g_stActuatorReqData[g_stActuatorReqInfo[j].m_ucIndexPos + i].m_ucData[k]);
			Trace(ACTUATOR_PARSING_LOG,"\r\n");
			Trace(ACTUATOR_PARSING_LOG,"m_uiTiming:%d\r\n",g_stActuatorReqData[g_stActuatorReqInfo[j].m_ucIndexPos + i].m_uiTiming);
			Trace(ACTUATOR_PARSING_LOG,"m_uiTimes:%d\r\n",g_stActuatorReqData[g_stActuatorReqInfo[j].m_ucIndexPos + i].m_uiTimes);
		}
		Trace(ACTUATOR_PARSING_LOG,"\r\n");
	}
#endif

	CH2_Default_MaskID_Set();
	return true;
}

void CH2_Default_MaskID_Set()
{
	int i=0,j=0;
	unsigned int uiMaskID[14];
	bool bISValid=true;


	memset(&uiMaskID,0x00,sizeof(uiMaskID));
	memset(&g_stControlHWSetInfo,0x00,sizeof(g_stControlHWSetInfo));

	g_stControlHWSetInfo.m_ucCanCH = CAN_CHANNEL_2;

	for( i=0; i<g_stActuatorReadyInfo.m_uiCount; i++)
	{
		if( g_stActuatorReadyData[i].m_ucCanLine == LOWCAN1 || g_stActuatorReadyData[i].m_ucCanLine == HIGHCAN3 )	//둘중하나만 설정가능 두개 동시에는 불가
		{
			if( g_stActuatorReadyData[i].m_ucCanLine == LOWCAN1 )
				g_stControlHWSetInfo.m_ucCanChip = Lowcan1;
			else
				g_stControlHWSetInfo.m_ucCanChip = Highcan3;

			switch( g_stActuatorReadyData[i].m_uiCanSpeed )
			{
				case 1024	:	g_stControlHWSetInfo.m_ucCanSpeed = eCAN_1MBPS;	break;
				case 500	:	g_stControlHWSetInfo.m_ucCanSpeed = eCAN_500KBPS;	break;
				case 250	:	g_stControlHWSetInfo.m_ucCanSpeed = eCAN_250KBPS;	break;
				case 125	:	g_stControlHWSetInfo.m_ucCanSpeed = eCAN_125KBPS;	break;
				case 100	:	g_stControlHWSetInfo.m_ucCanSpeed = eCAN_100KBPS;	break;
				case 50	:	g_stControlHWSetInfo.m_ucCanSpeed = eCAN_50KBPS;	break;
				default:
					Trace(0,"Not Found CanBPS_CH2\r\n");
					break;
			}

			bISValid = true;
			for( j=0; j<HAL_CAN_MAX_MASK_CNT; j++ )
			{
				if ( (uiMaskID[j] == g_stActuatorReadyData[i].m_uiResVal ) || (g_stActuatorReadyData[i].m_uiResVal == 0x00) )
				{
					bISValid = false;
				}
			}
			if( bISValid == true )
			{
			  	if(g_stControlHWSetInfo.m_uiMaskCount < 14)
				{
					uiMaskID[g_stControlHWSetInfo.m_uiMaskCount] = g_stActuatorReadyData[i].m_uiResVal;
					g_stControlHWSetInfo.m_uiEndMask[g_stControlHWSetInfo.m_uiMaskCount] = g_stControlHWSetInfo.m_uiStartMask[g_stControlHWSetInfo.m_uiMaskCount] = g_stActuatorReadyData[i].m_uiResVal;
	#if defined(ACTUATOR_PARSING_LOG)
					Trace(ACTUATOR_PARSING_LOG,"D_Set_CH2:[%04X][%04X]\r\n",g_stControlHWSetInfo.m_uiStartMask[g_stControlHWSetInfo.m_uiMaskCount], g_stControlHWSetInfo.m_uiEndMask[g_stControlHWSetInfo.m_uiMaskCount]);
	#endif
					g_stControlHWSetInfo.m_uiMaskCount++;
				}
			}
		}
	}
	
	memset(&uiMaskID,0x00,sizeof(uiMaskID));
	for( i=0; i<g_stActuatorReadyInfo.m_uiCount; i++)
	{
		bISValid = true;
		for( j=0; j<CAN_ID_CHECK_LIST_MAX_CNT; j++ )
		{
			if ( (g_stCanIDCheckList.m_uiStartMask[j] == g_stActuatorReadyData[i].m_uiResVal) || (g_stActuatorReadyData[i].m_uiResVal == 0x00) )
			{
				bISValid = false;
			}
		}
		if( bISValid == true )
		{
			g_stCanIDCheckList.m_uiStartMask[g_stCanIDCheckList.m_uiMaskCount] = g_stCanIDCheckList.m_uiEndMask[g_stCanIDCheckList.m_uiMaskCount] = g_stActuatorReadyData[i].m_uiResVal;
			g_stCanIDCheckList.m_uiMaskCount++;
			if( g_stCanIDCheckList.m_uiMaskCount >= CAN_ID_CHECK_LIST_MAX_CNT )
			{
				printf("%s : g_stCanIDCheckList MaskCnt overflow2\r\n", __FUNCTION__);
				g_stCanIDCheckList.m_uiMaskCount = CAN_ID_CHECK_LIST_MAX_CNT-1;
			}
		}
	}
	if( FUELTYPE_GET_STATE() == FCEV )
	{
	        for( i=0; i<g_ucTotResCnt; i++)
		{
			bISValid = true;
			for( j=0; j<CAN_ID_CHECK_LIST_MAX_CNT; j++ )
			{
				if ( (g_stCanIDCheckList.m_uiStartMask[j] == g_stActuatorResData[i].m_uiResVal) || (g_stActuatorResData[i].m_uiResVal == 0x00) )
				{
					bISValid = false;
				}
			}
			if( bISValid == true )
			{
				g_stCanIDCheckList.m_uiStartMask[g_stCanIDCheckList.m_uiMaskCount] = g_stCanIDCheckList.m_uiEndMask[g_stCanIDCheckList.m_uiMaskCount] = g_stActuatorResData[i].m_uiResVal;
				g_stCanIDCheckList.m_uiMaskCount++;
				if( g_stCanIDCheckList.m_uiMaskCount >= CAN_ID_CHECK_LIST_MAX_CNT )
				{
					printf("%s : g_stCanIDCheckList MaskCnt overflow2\r\n", __FUNCTION__);
					g_stCanIDCheckList.m_uiMaskCount = CAN_ID_CHECK_LIST_MAX_CNT-1;
			}
			}
		}
	}	
//	for( i=0; i<g_stCanIDCheckList.m_uiMaskCount; i++)
//	{
//		printf("%04X\r\n",g_stCanIDCheckList.m_uiStartMask[i]);
//	}
}

void GetConfigData(char *pData,unsigned char ucSize)
{
	char cParseData[200];
    char *temp,*word;
	unsigned char ucTokenCnt=0;

	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	temp = cParseData;

	while(( word = strsep(&temp, ",")) != NULL )	//한LINE의 끝까지
	{
			switch( ucTokenCnt )
		{
			case INDEX_CONFIG_AVN:									//0
			case INDEX_CONFIG_TYPE:									//2
			case INDEX_CONFIG_ENGSTOP:							//4
			case INDEX_CONFIG_WAITTIME:							//6
				break;
			case INDEX_CONFIG_AVN_VERSION:						//1
				g_stControlConfig.m_ucAVNVersion = atoi(word);			
				break;
			case INDEX_CONFIG_TYPE_VALUE:						//3
				g_stControlConfig.m_ucAVNType = atoi(word);
				break;
			case INDEX_CONFIG_ENGSTOP_VALUE:				//5
				g_stControlConfig.m_ucEngStopType = atoi(word);
				break;
			case INDEX_CONFIG_WAITTIME_VALUE:				//7
				g_stControlConfig.m_uiWaitTime = atoi(word);
				break;
			default:
				break;
		}
		ucTokenCnt++;
	}
}

void GetFreezeData(char *pData,unsigned char ucSize)
{
	char cParseData[200];
    char *word, *temp;
	unsigned char ucTokenCnt=0;

	if( g_ucTotFreezeCnt == FREEZE_FRAME_SYS_CNT_MAX)	return;

	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	temp = cParseData;

	while(( word = strsep(&temp, ",")) != NULL)	//한LINE의 끝까지
	{
			switch( ucTokenCnt )
		{
			case INDEX_FREEZE_SYSTEM:
				strcpy( g_stActuatorFreezeData[g_ucTotFreezeCnt].m_cSystem, word );
				break;
			case INDEX_FREEZE_ECUID:
				strncpy( g_stActuatorFreezeData[g_ucTotFreezeCnt].m_cEcuid, word, sizeof(g_stActuatorFreezeData[g_ucTotFreezeCnt].m_cEcuid) );
				break;
			case INDEX_FREEZE_FUNCTIONINDEX:
				g_stActuatorFreezeData[g_ucTotFreezeCnt].m_uiFuncIndex=atoi(word);
				break;
			case INDEX_FREEZE_OPEN_REQ:
				strcpy(g_stActuatorFreezeData[g_ucTotFreezeCnt].m_ucOpenReq, word );
				break;
			case INDEX_FREEZE_OPEN_RES:
				strcpy(g_stActuatorFreezeData[g_ucTotFreezeCnt].m_ucOpenRes, word );
				break;
			case INDEX_FREEZE_FRAME_REQ:
				strcpy(g_stActuatorFreezeData[g_ucTotFreezeCnt].m_ucFreezeReq, word );
				break;
			case INDEX_FREEZE_FRAME_RES:
				strcpy(g_stActuatorFreezeData[g_ucTotFreezeCnt].m_ucFreezeRes, word );
				break;
			case INDEX_FREEZE_CLOSE_REQ:
				strcpy(g_stActuatorFreezeData[g_ucTotFreezeCnt].m_ucCloseReq, word );
				break;
			case INDEX_FREEZE_CLOSE_RES:
				strcpy(g_stActuatorFreezeData[g_ucTotFreezeCnt].m_ucCloseRes, word );
				break;
			default:
				break;
		}
		ucTokenCnt++;
	}
	ucTokenCnt=0;
	g_ucTotFreezeCnt++;
}

void GetConvertData(char *pData,unsigned char ucSize)
{
	char cParseData[200];
    char *word,*temp;
	unsigned char ucTokenCnt=0;

	if( g_ucTotConvertCnt == ACTUATOR_MAX_CONVERT_CNT)	return;

	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	temp = cParseData;

	while(( word = strsep(&temp, ",")) != NULL)	//한LINE의 끝까지
	{
		switch( ucTokenCnt )
		{
		case INDEX_CONV_FUNCTIONINDEX:
			g_stActuatorConvertData[g_ucTotConvertCnt].m_uiFuncIndex=atoi(word);
			g_stActuatorConvertInfo.m_uiFuncIndex = g_stActuatorConvertData[g_ucTotConvertCnt].m_uiFuncIndex;
			g_stActuatorConvertInfo.m_uiCount++;
			break;
		case INDEX_CONV_SEPARATOR:
				g_stActuatorConvertData[g_ucTotConvertCnt].m_ucSeparator=word[0];
			break;
		case INDEX_CONV_ON:
			g_stActuatorConvertData[g_ucTotConvertCnt].m_ucOn=AsciiToHex(word[2], word[3]);
			break;
		case INDEX_CONV_OFF:
			g_stActuatorConvertData[g_ucTotConvertCnt].m_ucOff=AsciiToHex(word[2], word[3]);
			break;
		default:
			break;
		}

		ucTokenCnt++;
	}
	ucTokenCnt=0;
	g_ucTotConvertCnt++;
}

void GetReqData(char *pData,unsigned char ucSize)
{
	char cParseData[200];
    char *word, *temp;
	unsigned char ucTokenCnt=0;
	unsigned int i=0;
	//unsigned int nFieldNum=0;
	static unsigned int s_uiSaveFuncIndex=0;

	if( g_ucTotReqCnt == ACTUATOR_MAX_REQ_CNT)	return;

	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	temp = cParseData;

	while(( word = strsep(&temp, ",")) != NULL)	//한LINE의 끝까지, ','구분자
	{
		switch( ucTokenCnt )
		{
		case INDEX_REQ_FUNCTIONINDEX:
			g_stActuatorReqData[g_ucTotReqCnt].m_uiFuncIndex=atoi(word);
			if( g_stActuatorReqData[g_ucTotReqCnt].m_uiFuncIndex ==  ACTUATOR_TYPE_ENGINERUNCO )	g_bEngRunContinueSuppFlag = true;
			if( g_stActuatorReqData[g_ucTotReqCnt].m_uiFuncIndex ==  ACTUATOR_TYPE_REARDEFOG )		g_bRearDefogDBInFlag = true;
			g_stActuatorReqInfo[g_ucReqIndex].m_uiFuncIndex = g_stActuatorReqData[g_ucTotReqCnt].m_uiFuncIndex;

			if( s_uiSaveFuncIndex !=  g_stActuatorReqInfo[g_ucReqIndex].m_uiFuncIndex )
			{
				g_stActuatorReqInfo[g_ucReqIndex].m_ucIndexPos = g_ucTotReqCnt;	//최초 포지션을 저장하기위해
				s_uiSaveFuncIndex = g_stActuatorReqInfo[g_ucReqIndex].m_uiFuncIndex;
			}
			g_stActuatorReqInfo[g_ucReqIndex].m_uiCount++;
			break;
		case INDEX_REQ_WAKEUPUSED:
			g_stActuatorReqData[g_ucTotReqCnt].m_ucWakeupUsed=atoi(word);
			break;
		case INDEX_REQ_RETRY:
			g_stActuatorReqData[g_ucTotReqCnt].m_ucRetry=atoi(word);
			break;
		case INDEX_REQ_CANLINE:
			g_stActuatorReqData[g_ucTotReqCnt].m_ucCanLine=atoi(word);
			break;
		case INDEX_REQ_CANSPEED:
			g_stActuatorReqData[g_ucTotReqCnt].m_uiCanSpeed=atoi(word);
			break;
		case INDEX_REQ_REQVAL:
			if(strlen(word) <= 4)
			{
				g_stActuatorReqData[g_ucTotReqCnt].m_uiReqVal=(AsciiToHex(word[0],word[1])<<8) + AsciiToHex(word[2],word[3]);
			}
			else
			{
				g_stActuatorReqData[g_ucTotReqCnt].m_uiReqVal=(AsciiToHex(word[0],word[1])<<24) + (AsciiToHex(word[2],word[3])<<16) + (AsciiToHex(word[4],word[5])<<8) + (AsciiToHex(word[6],word[7]));
			}
			break;
		case INDEX_REQ_RESVAL:
			if(strlen(word) <= 4)
			{
				g_stActuatorReqData[g_ucTotReqCnt].m_uiResVal=(AsciiToHex(word[0],word[1])<<8) + AsciiToHex(word[2],word[3]);
			}
			else
			{
				g_stActuatorReqData[g_ucTotReqCnt].m_uiResVal=(AsciiToHex(word[0],word[1])<<24) + (AsciiToHex(word[2],word[3])<<16) + (AsciiToHex(word[4],word[5])<<8) + (AsciiToHex(word[6],word[7]));
			}
			break;
		case INDEX_REQ_LENGTH:
			g_stActuatorReqData[g_ucTotReqCnt].m_ucLength=atoi(word);
			break;
		case INDEX_REQ_DATA:
			for( i=0; i<g_stActuatorReqData[g_ucTotReqCnt].m_ucLength; i++ )
			{
				if( (((word[i*2]) == 'X') && ((word[(i*2)+1])=='X'))  || (((word[i*2]) == 'x') && ((word[(i*2)+1])=='x')) )
				{
					g_stActuatorReqData[g_ucTotReqCnt].m_ucData[i] = 'X';
				}
				else if( (((word[i*2]) == 'Y') && ((word[(i*2)+1])=='Y')) || (((word[i*2]) == 'y') && ((word[(i*2)+1])=='y')) )
				{
					g_stActuatorReqData[g_ucTotReqCnt].m_ucData[i] = 'Y';
				}
				else if( (((word[i*2]) == 'Z') && ((word[(i*2)+1])=='Z')) || (((word[i*2]) == 'z') && ((word[(i*2)+1])=='z')) )
				{
					g_stActuatorReqData[g_ucTotReqCnt].m_ucData[i] = 'Z';
				}
				else if( (((word[i*2]) == 'L') && ((word[(i*2)+1])=='L')) || (((word[i*2]) == 'l') && ((word[(i*2)+1])=='l')) )
				{
					g_stActuatorReqData[g_ucTotReqCnt].m_ucData[i] = 'L';
				}
				else	g_stActuatorReqData[g_ucTotReqCnt].m_ucData[i] = AsciiToHex(word[i*2],word[(i*2)+1]);
			}
			break;
		case INDEX_REQ_TIMING:
			g_stActuatorReqData[g_ucTotReqCnt].m_uiTiming=atoi(word);
			break;
		case INDEX_REQ_TIMES:
			g_stActuatorReqData[g_ucTotReqCnt].m_uiTimes=atoi(word);
			break;
		case INDEX_REQ_MIN:
			g_stActuatorReqData[g_ucTotReqCnt].m_uiMin=atoi(word);
			break;
		case INDEX_REQ_MAX:
			g_stActuatorReqData[g_ucTotReqCnt].m_uiMax=atoi(word);
			break;
		case INDEX_REQ_CONV:
			g_stActuatorReqData[g_ucTotReqCnt].m_fConvert=atoi(word);
			break;
		case INDEX_REQ_FDCANLINE:
			g_stActuatorReqData[g_ucTotReqCnt].m_ucFDCanLine=atoi(word);
			break;		
		case INDEX_REQ_FDCANSPEED:
			g_stActuatorReqData[g_ucTotReqCnt].m_ucFDCanBaudRate=atoi(word);
			break;		
		case INDEX_REQ_FDCANFRAME:
			g_stActuatorReqData[g_ucTotReqCnt].m_ucFDCanFrame=atoi(word);
			break;		
		default:
			break;
		}

		ucTokenCnt++;
	}
	ucTokenCnt=0;
	g_ucTotReqCnt++;
}

void GetResData(char *pData,unsigned char ucSize)
{
	char cParseData[200];
    char *word, *temp;
	unsigned char ucTokenCnt=0;
	int i=0;
	static unsigned int s_uiSaveFuncIndex=0;

	if( g_ucTotResCnt == ACTUATOR_MAX_RES_CNT)	return;

	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	temp = cParseData;

	while(( word = strsep(&temp, ",")) != NULL)	//한LINE의 끝까지, ',' 구분자
	{
			switch( ucTokenCnt )
		{
		case INDEX_RES_FUNCTIONINDEX:
			g_stActuatorResData[g_ucTotResCnt].m_uiFuncIndex=atoi(word);
			g_stActuatorResInfo[g_ucResIndex].m_uiFuncIndex = g_stActuatorResData[g_ucTotResCnt].m_uiFuncIndex;
			if( s_uiSaveFuncIndex !=  g_stActuatorResInfo[g_ucResIndex].m_uiFuncIndex )
			{
				g_stActuatorResInfo[g_ucResIndex].m_ucIndexPos = g_ucTotResCnt;	//최초 포지션을 저장하기위해
				s_uiSaveFuncIndex = g_stActuatorResInfo[g_ucResIndex].m_uiFuncIndex;
			}
			g_stActuatorResInfo[g_ucResIndex].m_uiCount++;
			break;
		case INDEX_RES_CANLINE:
			g_stActuatorResData[g_ucTotResCnt].m_ucCanLine=atoi(word);
			break;
		case INDEX_RES_CANSPEED:
			g_stActuatorResData[g_ucTotResCnt].m_uiCanSpeed=atoi(word);
			break;
		case INDEX_RES_INDEX:
			strncpy( g_stActuatorResData[g_ucTotResCnt].m_cIndex, word, sizeof(g_stActuatorResData[g_ucTotResCnt].m_cIndex) );
			break;
		case INDEX_RES_REQVAL:
			g_stActuatorResData[g_ucTotResCnt].m_uiReqVal=(AsciiToHex(word[0],word[1])<<8) + AsciiToHex(word[2],word[3]);
			break;
		case INDEX_RES_RESVAL:
			g_stActuatorResData[g_ucTotResCnt].m_uiResVal=(AsciiToHex(word[0],word[1])<<8) + AsciiToHex(word[2],word[3]);
			break;
		case INDEX_RES_STARTPOS:
			g_stActuatorResData[g_ucTotResCnt].m_ucStartPos=atoi(word);
			break;
		case INDEX_RES_REALPOS:
			g_stActuatorResData[g_ucTotResCnt].m_ucRealPos=atoi(word);
			break;
		case INDEX_RES_DATASIZE:
			g_stActuatorResData[g_ucTotResCnt].m_ucDataSize=atoi(word);
			break;
		case INDEX_RES_MSBLSB:
			g_stActuatorResData[g_ucTotResCnt].m_ucMsbLsb=atoi(word);
			break;
		case INDEX_RES_MASKVAL:
			if( g_stActuatorResData[g_ucTotResCnt].m_ucDataSize != 0)
			{
				for(i=0; i<g_stActuatorResData[g_ucTotResCnt].m_ucDataSize ; i++)
				{
					g_stActuatorResData[g_ucTotResCnt].m_uiMaskVal = ( (g_stActuatorResData[g_ucTotResCnt].m_uiMaskVal<<8) | (AsciiToHex(word[(i*2)+0], word[(i*2)+1])) );
				}
			}
			else
			{
				g_stActuatorResData[g_ucTotResCnt].m_uiMaskVal=0x00;
			}
			break;
		case INDEX_RES_CONVRULE:
			g_stActuatorResData[g_ucTotResCnt].m_ucConvrule=atoi(word);
			break;
		case INDEX_RES_A:
			g_stActuatorResData[g_ucTotResCnt].m_fA=atof(word);
			break;
		case INDEX_RES_B:
			g_stActuatorResData[g_ucTotResCnt].m_fB=atof(word);
			break;
		case INDEX_RES_C:
			g_stActuatorResData[g_ucTotResCnt].m_fC=atof(word);
			break;
		case INDEX_RES_D:
			if( (g_stActuatorResData[g_ucTotResCnt].m_ucConvrule == 3 || g_stActuatorResData[g_ucTotResCnt].m_ucConvrule == 4) && (strncmp(word," ",1)!=0) )
				g_stActuatorResData[g_ucTotResCnt].m_cD=AsciiToHex(word[2], word[3]);
			else
				g_stActuatorResData[g_ucTotResCnt].m_cD=atoi(word);
			break;
		case INDEX_RES_E:
			g_stActuatorResData[g_ucTotResCnt].m_cE=atoi(word);
			break;
		case INDEX_RES_F:
			g_stActuatorResData[g_ucTotResCnt].m_cF=atoi(word);
			break;
		case INDEX_RES_LUT:
			if(g_stActuatorResData[g_ucTotResCnt].m_ucConvrule==3 || g_stActuatorResData[g_ucTotResCnt].m_ucConvrule == 4 || g_stActuatorResData[g_ucTotResCnt].m_ucConvrule == 20)
			{
				//strcpy((char*)g_stActuatorResData[g_ucTotResCnt].m_ucLut, word);
			  	int pSize = 0;
				pSize = temp-word;
				
				g_stActuatorResData[g_ucTotResCnt].pucArrLut = malloc(pSize);
				if( g_stActuatorResData[g_ucTotResCnt].pucArrLut != NULL )
				{
					memset(g_stActuatorResData[g_ucTotResCnt].pucArrLut,0,pSize);
					memcpy(g_stActuatorResData[g_ucTotResCnt].pucArrLut,word,pSize);
				}
				else
				{
					printf("===================================================\n");
					printf("%s] Error Malloc Lut\n",__FUNCTION__);
				}
			}
			else
			{
			  	g_stActuatorResData[g_ucTotResCnt].pucArrLut = NULL;
			}
			break;
		case INDEX_RES_COMPARETYPE:
			g_stActuatorResData[g_ucTotResCnt].m_cCompType=atoi(word);
			break;
		case INDEX_RES_COMPAREVALUE:
			g_stActuatorResData[g_ucTotResCnt].m_uiCompVal=atoi(word);
			break;
		case INDEX_RES_FDCANLINE:
			g_stActuatorResData[g_ucTotResCnt].m_ucFDCanLine=atoi(word);
			break;
		case INDEX_RES_FDCANSPEED:
			g_stActuatorResData[g_ucTotResCnt].m_ucFDCanBaudRate=atoi(word);
			break;
		case INDEX_RES_FDCANFRAME:
			g_stActuatorResData[g_ucTotResCnt].m_ucFDCanFrame=atoi(word);
			break;
		default:
			break;
		}

		ucTokenCnt++;
	}
	ucTokenCnt=0;
	g_ucTotResCnt++;
}

#if defined(PROTOCOL18)
void GetCtrlData(char *pData,unsigned char ucSize) //dahae
{
	char cParseData[200];
    char *word, *temp;
	unsigned char ucTokenCnt=0;
	
	if( g_ucTotCtrlCnt == ACTUATOR_MAX_CTRL_CNT)	return;
	
	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	temp = cParseData;

	while(( word = strsep(&temp, ",")) != NULL)
	{
		switch( ucTokenCnt )
		{
		case INDEX_CTRL_FUNCTIONINDEX:
			g_stActuatorCtrlData[g_ucTotCtrlCnt].m_uiFuncIndex=atoi(word);
			break;
		case INDEX_CTRL_FUNCTION:
			strncpy(g_stActuatorCtrlData[g_ucTotCtrlCnt].m_ucCtrlFunction, word, strlen(word));
			break;
		case INDEX_CTRL_SUPPORT:
			g_stActuatorCtrlData[g_ucTotCtrlCnt].m_ucSupport=(unsigned char)(atoi(word));
			break;
		default:
			break;
		}
		
		ucTokenCnt++;
	}
	ucTokenCnt=0;
	g_ucTotCtrlCnt++;
	
}

void GetAirConData(char *pData,unsigned char ucSize)  // dahae
{
	char cParseData[200];
    char *word, *temp;
	unsigned char ucTokenCnt=0;
	char cAirConINDEXMessage[]="TEMP";
	
	if( g_ucTotAirCnt == ACTUATOR_MAX_AIR_CNT)	return;
	
	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	temp = cParseData;

	while(( word = strsep(&temp, ",")) != NULL)
	{
		switch( ucTokenCnt )
		{
		case INDEX_AIR_FUNCTIONINDEX:
			g_stActuatorAirConData[g_ucTotAirCnt].m_usTemp=atoi(word);
			break;
		case INDEX_AIR_VALUE1:
			g_stActuatorAirConData[g_ucTotAirCnt].m_ucValue1=AsciiToHex(word[0], word[1]);
			break;
		case INDEX_AIR_VALUE2:
			g_stActuatorAirConData[g_ucTotAirCnt].m_ucValue2=AsciiToHex(word[0], word[1]);
			break;
		case INDEX_AIR_VALUE3:
			g_stActuatorAirConData[g_ucTotAirCnt].m_ucValue3=AsciiToHex(word[0], word[1]);
			break;
		case INDEX_AIR_CS:
			g_stActuatorAirConData[g_ucTotAirCnt].m_ucCS=AsciiToHex(word[0], word[1]);
			break;
		default:
			break;
		}
		
		ucTokenCnt++;
	}
	ucTokenCnt=0;
	g_ucTotAirCnt++;

	if( strncmp(cAirConINDEXMessage, cParseData, strlen(cAirConINDEXMessage))==0 ) //TEMP가 포함된 문자열 존재 여부 확인
	{	
		g_ucTotAirCnt=0;
	}

}
#endif
#if defined(F_TEMPERATURE)
void GetAirCon_F_Data(char *pData,unsigned char ucSize)  // dahae
{
	char cParseData[200];
	char *lastPos[2],*word, *temp;
	unsigned char ucTokenCnt=0;
	char cAirConINDEXMessage[]="TEMP";
	
	if( g_ucTotAirCnt_F == ACTUATOR_MAX_AIR_CNT)	return;
	
	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	temp = cParseData;

	while(( word = strsep(&temp, ",")) != NULL)
	{
		switch( ucTokenCnt )
		{
		case INDEX_AIR_FUNCTIONINDEX:
			g_stActuatorAirCon_F_Data[g_ucTotAirCnt_F].m_usTemp=atoi(word);
			break;
		case INDEX_AIR_VALUE1:
			g_stActuatorAirCon_F_Data[g_ucTotAirCnt_F].m_ucValue1=AsciiToHex(word[0], word[1]);
			break;
		case INDEX_AIR_VALUE2:
			g_stActuatorAirCon_F_Data[g_ucTotAirCnt_F].m_ucValue2=AsciiToHex(word[0], word[1]);
			break;
		case INDEX_AIR_VALUE3:
			g_stActuatorAirCon_F_Data[g_ucTotAirCnt_F].m_ucValue3=AsciiToHex(word[0], word[1]);
			break;
		case INDEX_AIR_CS:
			g_stActuatorAirCon_F_Data[g_ucTotAirCnt_F].m_ucCS=AsciiToHex(word[0], word[1]);
			break;
		default:
			break;
		}
		
		ucTokenCnt++;
	}
	ucTokenCnt=0;
	g_ucTotAirCnt_F++;

	if( strncmp(cAirConINDEXMessage, cParseData, strlen(cAirConINDEXMessage))==0 ) //TEMP가 포함된 문자열 존재 여부 확인
	{	
		g_ucTotAirCnt_F=0;
	}

}
#endif

void GetVehicleStatusData(char *pData,unsigned char ucSize)
{
	char cParseData[200];
//	char *lastPos[2]
    char *word;
    char *temp;
	unsigned char ucTokenCnt=0;
	int i=0;

	if( g_ucTotReadyCnt == ACTUATOR_MAX_READY_CNT)	return;

	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	temp = cParseData;

	while(( word = strsep(&temp, ",")) != NULL)	//한LINE의 끝까지
	{
		switch( ucTokenCnt )
		{
		case INDEX_VEHICLESTATUS_FUNCTIONINDEX:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_uiFuncIndex=atoi(word);
			g_stActuatorReadyInfo.m_uiFuncIndex = g_stActuatorReadyData[g_ucTotReadyCnt].m_uiFuncIndex;
			g_stActuatorReadyInfo.m_uiCount++;
			break;
		case INDEX_VEHICLESTATUS_WAKEUPUSED:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucWakeupUsed=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_CANLINE:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucCanLine=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_CANSPEED:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_uiCanSpeed=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_INDEX:
			strncpy( g_stActuatorReadyData[g_ucTotReadyCnt].m_cIndex, word, sizeof(g_stActuatorReadyData[g_ucTotReadyCnt].m_cIndex) );
			break;
		case INDEX_VEHICLESTATUS_REQVAL:
			if(strlen(word) <= 4)
			{
				g_stActuatorReadyData[g_ucTotReadyCnt].m_uiReqVal = (AsciiToHex(word[0],word[1])<<8) + AsciiToHex(word[2],word[3]);
			}
			else
			{
				g_stActuatorReadyData[g_ucTotReadyCnt].m_uiReqVal = (AsciiToHex(word[0],word[1])<<24) + (AsciiToHex(word[2],word[3])<<16) + (AsciiToHex(word[4],word[5])<<8) + (AsciiToHex(word[6],word[7]));
			}
			break;
		case INDEX_VEHICLESTATUS_RESVAL:
			if(strlen(word) <= 4)
			{
				g_stActuatorReadyData[g_ucTotReadyCnt].m_uiResVal = (AsciiToHex(word[0],word[1])<<8) + AsciiToHex(word[2],word[3]);
			}
			else
			{
				g_stActuatorReadyData[g_ucTotReadyCnt].m_uiResVal = (AsciiToHex(word[0],word[1])<<24) + (AsciiToHex(word[2],word[3])<<16) + (AsciiToHex(word[4],word[5])<<8) + (AsciiToHex(word[6],word[7]));
			}
			break;
		case INDEX_VEHICLESTATUS_STARTPOS:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucStartPos=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_REALPOS:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucRealPos=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_DATASIZE:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucDataSize=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_MSBLSB:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucMsbLsb=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_MASKVAL:
			for(i=0; i<g_stActuatorReadyData[g_ucTotReadyCnt].m_ucDataSize ; i++)
			{
				g_stActuatorReadyData[g_ucTotReadyCnt].m_uiMaskVal= ( (g_stActuatorReadyData[g_ucTotReadyCnt].m_uiMaskVal<<8) | (AsciiToHex(word[(i*2)+0], word[(i*2)+1])) );
			}
			break;
		case INDEX_VEHICLESTATUS_CONVRULE:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucConvrule=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_A:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_fA=atof(word);
			break;
		case INDEX_VEHICLESTATUS_B:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_fB=atof(word);
			break;
		case INDEX_VEHICLESTATUS_C:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_fC=atof(word);
			break;
		case INDEX_VEHICLESTATUS_D:
			if( (g_stActuatorReadyData[g_ucTotReadyCnt].m_ucConvrule == 3 || g_stActuatorReadyData[g_ucTotReadyCnt].m_ucConvrule == 4) && (strncmp(word," ",1)!=0) )
				g_stActuatorReadyData[g_ucTotReadyCnt].m_cD=AsciiToHex(word[2], word[3]);
			else
				g_stActuatorReadyData[g_ucTotReadyCnt].m_cD=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_E:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_cE=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_F:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_cF=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_LUT:
			if(g_stActuatorReadyData[g_ucTotReadyCnt].m_ucConvrule==3 || g_stActuatorReadyData[g_ucTotReadyCnt].m_ucConvrule == 4 || g_stActuatorReadyData[g_ucTotReadyCnt].m_ucConvrule == 20)
			{
				//strcpy((char*)g_stActuatorReadyData[g_ucTotReadyCnt].m_ucLut, word);
			  	int pSize = 0;
				pSize = temp-word;
				
				g_stActuatorReadyData[g_ucTotReadyCnt].pucArrLut = malloc(pSize);
				if( g_stActuatorReadyData[g_ucTotReadyCnt].pucArrLut != NULL )
				{
					memset(g_stActuatorReadyData[g_ucTotReadyCnt].pucArrLut,0,pSize);
					memcpy(g_stActuatorReadyData[g_ucTotReadyCnt].pucArrLut,word,pSize);
				}
				else
				{
					printf("===================================================\n");
					printf("%s] Error Malloc Lut\n",__FUNCTION__);
				}
			}
			else
			{
			  	g_stActuatorReadyData[g_ucTotReadyCnt].pucArrLut = NULL;
			}
			break;
		case INDEX_VEHICLESTATUS_CANFDLINE:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucFDCanLine=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_CANFDSPEED:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucFDCanBaudRate=atoi(word);
			break;
		case INDEX_VEHICLESTATUS_CANFDFRAME:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_ucFDCanFrame=atoi(word);
			break;		
//		case INDEX_VEHICLESTATUS_COMPAREVALUE:
//			g_stActuatorReadyData[g_ucTotReadyCnt].m_CompVal=atoi(word);
//			break;
//		case INDEX_VEHICLESTATUS_USEFUNCTION:
//			nLineSize = sizeof(char) * (lastPos[0]- word);
//			strncpy((char*)g_stActuatorReadyData[g_ucTotReadyCnt].m_ucUseFunction, word, nLineSize);
//			break;
		default:
			break;
		}

		ucTokenCnt++;
	}
	ucTokenCnt=0;
	g_ucTotReadyCnt++;
}

void GetUseFunction(char *pData,unsigned char ucSize)
{
	char cParseData[200];
	char *lastPos[2],*word;
	unsigned char iTokenCnt=0;
	unsigned int nFieldNum=0;

	memset(cParseData,0x00,sizeof(cParseData));
	strncpy(cParseData, pData,ucSize);		//한LINE 데이터를 복사
	word=strtok_r(cParseData, ",",  &lastPos[0]);	// , 구분자

	while( word != NULL )	//한LINE의 끝까지
	{
		nFieldNum=iTokenCnt % INDEX_VEHICLESTATUS_TOTALCNT;
		switch( nFieldNum )
		{
		case INDEX_VEHICLESTATUS_FUNCTIONINDEX:
			g_stActuatorReadyData[g_ucTotReadyCnt].m_uiFuncIndex=atoi(word);
		default:
			break;
		}

		iTokenCnt++;
		word=strtok_r(NULL, ",",  &lastPos[0]);
	}
	iTokenCnt=0;
	g_ucTotReadyCnt++;
}

bool GetDataBaseParsing_Master()
{
 	bool ret=TRUE;

#if defined(SIMULATION)
	// MASTER_capital
	U8 cNaviMessage[]=			"#FAST,0x01,0x01,,,\r\nindex,func_type,ref_curdata,fine_floatrange,fine_unit,fine_datasize\r\nF1,0,C01.C02.CP0,0,0,0\r\nF2,0,E02.H01.E03.E04,0,0,0\r\nF3,0,A01.A02.E06,0,0,0\r\nF4,0,E0C.E0D,0,0,0\r\n#FAST_end,,,,,\r\n#SLOW1,0x01,0x02,,,\r\n#SLOW1_end,,,,,\r\n#SLOW2,0x01,0x03,,,\r\nSA,0,E01.E08.E0E,0,0,0\r\nSB,0,T01.T02.T03.T04,0,0,0\r\n#SLOW2_end,,,,,\r\n#SLOW3,0x01,0x04,,,\r\nT1,0,A03.E11.E13,0,0,0\r\n#SLOW2_end,,,,,\r\n";
	U8 cNaviOriginal[]=			"#FAST,0x01,0x01,,,\r\nindex,func_type,ref_curdata,fine_floatrange,fine_unit,fine_datasize\r\nF1,0,C01.C02.CP0,0,0,0\r\nF2,0,E02.H01.E03.E04,0,0,0\r\nF3,0,A01.A02.E06,0,0,0\r\nF4,0,E0C.E0D,0,0,0\r\n#FAST_end,,,,,\r\n#SLOW1,0x01,0x02,,,\r\n#SLOW1_end,,,,,\r\n#SLOW2,0x01,0x03,,,\r\nSA,0,E01.E08.E0E,0,0,0\r\nSB,0,T01.T02.T03.T04,0,0,0\r\n#SLOW2_end,,,,,\r\n#SLOW3,0x01,0x04,,,\r\nT1,0,A03.E11.E13,0,0,0\r\n#SLOW2_end,,,,,\r\n";

	//#FAST READ
	if(!GetMasterData(cNaviMessage, "#FAST", "#FAST_end", &g_usMasterCnt[0])) ret=FALSE;
	//#SLOW1 READ
	memcpy(cNaviMessage,cNaviOriginal, sizeof(cNaviOriginal)/sizeof(cNaviOriginal[0]));
	if(!GetMasterData(cNaviMessage, "#SLOW1", "#SLOW1_end", &g_usMasterCnt[0])) ret=FALSE;
	//#SLOW2 READ
	memcpy(cNaviMessage,cNaviOriginal, sizeof(cNaviOriginal)/sizeof(cNaviOriginal[0]));
	if(!GetMasterData(cNaviMessage, "#SLOW2", "#SLOW2_end", &g_usMasterCnt[0])) ret=FALSE;
	//#SLOW3 READ
	memcpy(cNaviMessage,cNaviOriginal, sizeof(cNaviOriginal)/sizeof(cNaviOriginal[0]));
	if(!GetMasterData(cNaviMessage, "#SLOW3", "#SLOW3_end", &g_usMasterCnt[0])) ret=FALSE;

#else
	unsigned int uiFlashAddress;
	U8 *cNaviMessage = NULL;
	U8 *cNaviOriginal = NULL;

	if ( memcmp(g_FirmwareInfo.AppProperty[eApp_MasterDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME) == 0 ||
         g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize > (CAR_SLAVE_ADDRESS-CAR_MASTER_DB_ADDRESS) )
	{
		Trace(0,"*****************************************************\r\n");
		Trace(0,"%s : there has no Master DB \r\n", __FUNCTION__);
		Trace(0,"*****************************************************\r\n");
		return 0;
	}
	cNaviMessage = malloc(g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize);
	cNaviOriginal = malloc(g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize);
#ifdef CHECK_MALLOC
	printf("GetDataBaseParsing_Master\r\n");
	__iar_dlmalloc_stats();
#endif
	if ( cNaviMessage == NULL || cNaviOriginal == NULL )
	{
		printf("%s : malloc fail so return false \r\n", __FUNCTION__);

		if ( cNaviMessage != NULL )
		{
			free(cNaviMessage);
			cNaviMessage=NULL;
		}
		if ( cNaviOriginal != NULL )
		{
			free(cNaviOriginal);
			cNaviOriginal=NULL;
		}
		return 0;
	}

	uiFlashAddress = CAR_MASTER_DB_ADDRESS;
	DecyptCarDB_FromFlash(cNaviMessage, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize);
	memcpy(cNaviOriginal, cNaviMessage, g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize);

	//#FAST READ
	if(!GetMasterData(cNaviMessage, "#FAST", "#FAST_end", &g_usMasterCnt[0])) ret=FALSE;
	//#SLOW1 READ
	memcpy(cNaviMessage,cNaviOriginal, g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize);
	if(!GetMasterData(cNaviMessage, "#SLOW1", "#SLOW1_end", &g_usMasterCnt[0])) ret=FALSE;
	//#SLOW2 READ
	memcpy(cNaviMessage,cNaviOriginal, g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize);
	if(!GetMasterData(cNaviMessage, "#SLOW2", "#SLOW2_end", &g_usMasterCnt[0])) ret=FALSE;
	//#SLOW3 READ
	memcpy(cNaviMessage,cNaviOriginal, g_FirmwareInfo.AppProperty[eApp_MasterDB].wFirmwareSize);
	if(!GetMasterData(cNaviMessage, "#SLOW3", "#SLOW3_end", &g_usMasterCnt[0])) ret=FALSE;

	if ( cNaviMessage != NULL )
	{
		free(cNaviMessage);
		cNaviMessage=NULL;
	}
	if ( cNaviOriginal != NULL )
	{
		free(cNaviOriginal);
		cNaviOriginal=NULL;
	}
#endif
	return ret;
}

bool GetSlaveDataSize(U8 *cVehicleMassage, U16 *nCnt)
{
	BOOL bFoundStartMessage = false;
	char *p;
	char *lastPos[2];
	int i=0;
	char message[]="#currentdata", endMessage[]="#currentdata_end";

	p=strtok_r((char*)cVehicleMassage, "\r\n", &lastPos[0]);
#ifdef CHECK_MALLOC
	__iar_dlmalloc_stats();
#endif
	while(p!=NULL)	//LINE이 NULL
	{
		if (strncmp(message, p, strlen(message))==0 && bFoundStartMessage == false)		//LINE 첫 문장이 message와 비교(첫 # SEARCHING)
		{
			bFoundStartMessage = true;
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp("system", p, strlen("system"))==0)	//"index_FIN(임시)" 열 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp(endMessage, p, strlen(endMessage))==0)	//LINE 첫 문장이 message와 비교 종료(첫 # SEARCHING)
		{
			break;
		}
		else if(bFoundStartMessage == false)							//시작점"#currentdata" 찾기 전까지 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		i++;
		if( i==SLAVE_DATA_MAX )
		{
			printf("SLAVE_DATA_MAX_Size over\r\n");
		}
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//다음 LINE
	}
	if ( bFoundStartMessage == false ) return 0;		//원하는 펑션을 찾지못하면 FAIL

	g_pstCurrDataBase=(stSlaveData *)malloc(sizeof(stSlaveData)*i);
//	printf("GetSlaveDataSize %d\r\n",sizeof(stSlaveData));
//	printf("stActuatorResData %d\r\n",sizeof(stActuatorResData));
//	printf("stActuatorReadyData %d\r\n",sizeof(stActuatorReadyData));
//	printf("stActuatorConvertData %d\r\n",sizeof(stActuatorConvertData));
//	printf("stActuatorReqData %d\r\n",sizeof(stActuatorReqData));
//	
//	printf("GetSlaveDataSize %d\r\n",sizeof(stSlaveData));
//	printf("stActuatorResData %d\r\n",(sizeof(stActuatorResData)*ACTUATOR_MAX_RES_CNT));
//	printf("stActuatorReadyData %d\r\n",(sizeof(stActuatorReadyData)*ACTUATOR_MAX_READY_CNT));
//	printf("stActuatorConvertData %d\r\n",(sizeof(stActuatorConvertData)*ACTUATOR_MAX_CONVERT_CNT));
//	printf("stActuatorReqData %d\r\n",(sizeof(stActuatorReqData)*ACTUATOR_MAX_REQ_CNT));
//	printf("g_stActuatorReqInfo %d\r\n",(sizeof(stActuatorReqInfo)*ACTUATOR_MAX_REQ_CNT));
#ifdef CHECK_MALLOC
	__iar_dlmalloc_stats();
#endif
	if ( g_pstCurrDataBase != NULL)
	{
		memset(g_pstCurrDataBase,0x00,sizeof(stSlaveData)*i);
	}
	else
	{
		printf("Size = %d\r\n",sizeof(stSlaveData)*i);
		printf("%s : malloc fail so return false \r\n", __FUNCTION__);
		return 0;
	}
	*nCnt=i;	//navi 갯수

	return 1;	//SLAVE_DATA_MAX 크기때문에 죽을 수 있다. 체크 필요
}

bool GetDataBaseParsing_Slave()
{
 	bool ret=TRUE;
#if defined(SIMULATION)
//
//	//U8 cVehicleMessage[]=	"#HWSET,0x06,05B0,0545,0316,07E8,0329,0200,0113,0111,07E9,07DE,0553,0541,07A8,0779\r\n#DISPLACE,G,0,6\r\n#currentdata, , , , , , , , , , , , , , , , \r\nIndex,requestcode,response,startpos,realpos,datasize,datatype,unit,convtype,A,B,C,D,E,F,lut,protocol type\r\nC01,05B0,05B0,4,1,2,2,34,1,0.1, , , , , , ,V\r\nE01,0545,0545,6,1,1,1,4,1,0.1015625, , , , , , ,V\r\nE02,0316,0316,5,1,2,2,5,1,0.25, , , , , , ,V\r\nE03,07E0022101,07E86101,20,21,1,2,8,1,1, , , , , , ,D\r\nE04,0316,0316,9,1,1,2,6,1,1, , , , , , ,V\r\nE05,0545,0545,3,1,1,1, ,3,1,1,1,0x02, , ,$0$1$,V\r\nE06,07E0022104,07E86104,9,5,1,2, ,3,1,1,1,0x08, , , ,D\r\nE07,07E002210A,07E8610A,12,4,1,2,8,1,1, , , , ,253, ,D\r\nE08,07E002210A,07E8610A,12,5,1,2,8,1,1, , , , ,253, ,D\r\nE09,07E002210A,07E8610A,14,9,1,2,71,1,1, , , , , , ,D\r\nE0A,07E002210A,07E8610A,9,1,2,2,33,2,0.1, , , , , , ,D\r\nE0B,0329,0329,9,1,1,2,8,1,0.392156863, , , , , , ,V\r\nE0C,0200,0200,3,1,2,2,1016,1,0.128, , , , , , ,V\r\nE0D,0329,0329,7,1,1,2, ,3,1,1,1,0x02, , ,$-$1$,V\r\nE0E,07E0022101,07E86101,17,14,1,2,3,1,0.75,-48, , , , , ,D\r\nE0F,07E0022104,07E86104,9,2,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nE10,07E0022104,07E86104,9,12,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nA01,0113,0113,5,1,1,1, ,3,1,1,1,0xF0, , ,$-$1$2$3$4$5$6$7$-$-$-$-$-$-$-$,V\r\nA02,0111,0111,4,1,1,1, ,3,1,1,1,0x0F, , ,$P$-$-$-$-$D$N$R$D$-$-$-$-$-$-$-$,V\r\nA03,07E10221A0,07E961A0,6,13,1,1,3,1,1,-40, , , , , ,D\r\nT01,07D6022106,07DE6106,6,4,1,1,35,1,0.25, , , , ,251, ,D\r\nT02,07D6022106,07DE6106,6,12,1,1,35,1,0.25, , , , ,251, ,D\r\nT03,07D6022106,07DE6106,6,20,1,1,35,1,0.25, , , , ,251, ,D\r\nT04,07D6022106,07DE6106,6,28,1,1,35,1,0.25, , , , ,251, ,D\r\nE12,0541,0541,5,1,1,1, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS06,0541,0541,4,1,1,1, ,3,1,1,1,0x10, , ,$0$1$,V\r\nS07,07A00322B005,07A862B005,10,1,1,2, ,3,1,1,1,0x20, , ,$0$1$,D\r\n#currentdata_end, , , , , , , , , , , , , , , , \r\n#readDTC, , , , , , , , , , , , , ,\r\nsystem,ecuid,functiontype,requesttime,searchtype,startpos,readno,skipno,totalmasking,openrequest,openresponse,readdtcrequest,readdtcresponse,closedtcrequest,closedtcresponse\r\nENGINE,ESN8,250,3,2,5,2,1,FFFF,07E0021081,07E850,07E0041800FF00,07E858,07E00120,07E860\r\nAT,TS0A,250,3,2,5,2,1,FFFF,07E1021081,07E950,07E1041800FF00,07E958,07E10120,07E960\r\nTPMS,TPM4,250,3,2,5,2,1,FFFF,07D6021081,07DE50,07D60418004000,07DE58,07D60120,07DE60\r\nABSVDC,D1M0,251,3,2,6,3,1,FFFFFF,07D1021001,07D950,07D103190208,07D959,07D10120,07D960\r\nEPB,D5M0,251,3,2,6,3,1,FFFFFF,07D5021001,07DD50,07D503190208,07DD59,07D50120,07DD60\r\nAEB,3700,251,3,2,6,3,1,FFFFFF,0737021001,073F50,073703190208,073F59,07370120,073F60\r\nEPS,EPL1,250,3,2,5,2,1,FFFF,07D4021081,07DC50,07D40418004000,07DC58,07D40120,07DC60\r\nSCC,D0M3,251,3,2,6,3,1,FFFFFF,07D0021001,07D850,07D003190208,07D859,07D00120,07D860\r\nLDWS,C4M6,251,3,2,6,3,1,FFFFFF,07C4021001,07CC50,07C403190208,07CC59,07C40120,07CC60\r\nBSD-R,BSDC,251,3,2,6,3,1,FFFFFF,0755021001,075D50,075503190208,075D59,07550120,075D60\r\nBSD-L,BSDB,251,3,2,6,3,1,FFFFFF,07B7021001,07BF50,07B703190208,07BF59,07B70120,07BF60\r\n#readDTC_end, , , , , , , , , , , , , ,\r\n";
//	//U8 cVehicleOriginal[]=	"#HWSET,0x06,05B0,0545,0316,07E8,0329,0200,0113,0111,07E9,07DE,0553,0541,07A8,0779\r\n#DISPLACE,G,0,6\r\n#currentdata, , , , , , , , , , , , , , , , \r\nIndex,requestcode,response,startpos,realpos,datasize,datatype,unit,convtype,A,B,C,D,E,F,lut,protocol type\r\nC01,05B0,05B0,4,1,2,2,34,1,0.1, , , , , , ,V\r\nE01,0545,0545,6,1,1,1,4,1,0.1015625, , , , , , ,V\r\nE02,0316,0316,5,1,2,2,5,1,0.25, , , , , , ,V\r\nE03,07E0022101,07E86101,20,21,1,2,8,1,1, , , , , , ,D\r\nE04,0316,0316,9,1,1,2,6,1,1, , , , , , ,V\r\nE05,0545,0545,3,1,1,1, ,3,1,1,1,0x02, , ,$0$1$,V\r\nE06,07E0022104,07E86104,9,5,1,2, ,3,1,1,1,0x08, , , ,D\r\nE07,07E002210A,07E8610A,12,4,1,2,8,1,1, , , , ,253, ,D\r\nE08,07E002210A,07E8610A,12,5,1,2,8,1,1, , , , ,253, ,D\r\nE09,07E002210A,07E8610A,14,9,1,2,71,1,1, , , , , , ,D\r\nE0A,07E002210A,07E8610A,9,1,2,2,33,2,0.1, , , , , , ,D\r\nE0B,0329,0329,9,1,1,2,8,1,0.392156863, , , , , , ,V\r\nE0C,0200,0200,3,1,2,2,1016,1,0.128, , , , , , ,V\r\nE0D,0329,0329,7,1,1,2, ,3,1,1,1,0x02, , ,$-$1$,V\r\nE0E,07E0022101,07E86101,17,14,1,2,3,1,0.75,-48, , , , , ,D\r\nE0F,07E0022104,07E86104,9,2,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nE10,07E0022104,07E86104,9,12,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nA01,0113,0113,5,1,1,1, ,3,1,1,1,0xF0, , ,$-$1$2$3$4$5$6$7$-$-$-$-$-$-$-$,V\r\nA02,0111,0111,4,1,1,1, ,3,1,1,1,0x0F, , ,$P$-$-$-$-$D$N$R$D$-$-$-$-$-$-$-$,V\r\nA03,07E10221A0,07E961A0,6,13,1,1,3,1,1,-40, , , , , ,D\r\nT01,07D6022106,07DE6106,6,4,1,1,35,1,0.25, , , , ,251, ,D\r\nT02,07D6022106,07DE6106,6,12,1,1,35,1,0.25, , , , ,251, ,D\r\nT03,07D6022106,07DE6106,6,20,1,1,35,1,0.25, , , , ,251, ,D\r\nT04,07D6022106,07DE6106,6,28,1,1,35,1,0.25, , , , ,251, ,D\r\nE12,0541,0541,5,1,1,1, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS06,0541,0541,4,1,1,1, ,3,1,1,1,0x10, , ,$0$1$,V\r\nS07,07A00322B005,07A862B005,10,1,1,2, ,3,1,1,1,0x20, , ,$0$1$,D\r\n#currentdata_end, , , , , , , , , , , , , , , , \r\n#readDTC, , , , , , , , , , , , , ,\r\nsystem,ecuid,functiontype,requesttime,searchtype,startpos,readno,skipno,totalmasking,openrequest,openresponse,readdtcrequest,readdtcresponse,closedtcrequest,closedtcresponse\r\nENGINE,ESN8,250,3,2,5,2,1,FFFF,07E0021081,07E850,07E0041800FF00,07E858,07E00120,07E860\r\nAT,TS0A,250,3,2,5,2,1,FFFF,07E1021081,07E950,07E1041800FF00,07E958,07E10120,07E960\r\nTPMS,TPM4,250,3,2,5,2,1,FFFF,07D6021081,07DE50,07D60418004000,07DE58,07D60120,07DE60\r\nABSVDC,D1M0,251,3,2,6,3,1,FFFFFF,07D1021001,07D950,07D103190208,07D959,07D10120,07D960\r\nEPB,D5M0,251,3,2,6,3,1,FFFFFF,07D5021001,07DD50,07D503190208,07DD59,07D50120,07DD60\r\nAEB,3700,251,3,2,6,3,1,FFFFFF,0737021001,073F50,073703190208,073F59,07370120,073F60\r\nEPS,EPL1,250,3,2,5,2,1,FFFF,07D4021081,07DC50,07D40418004000,07DC58,07D40120,07DC60\r\nSCC,D0M3,251,3,2,6,3,1,FFFFFF,07D0021001,07D850,07D003190208,07D859,07D00120,07D860\r\nLDWS,C4M6,251,3,2,6,3,1,FFFFFF,07C4021001,07CC50,07C403190208,07CC59,07C40120,07CC60\r\nBSD-R,BSDC,251,3,2,6,3,1,FFFFFF,0755021001,075D50,075503190208,075D59,07550120,075D60\r\nBSD-L,BSDB,251,3,2,6,3,1,FFFFFF,07B7021001,07BF50,07B703190208,07BF59,07B70120,07BF60\r\n#readDTC_end, , , , , , , , , , , , , ,\r\n";
//
//	//AE EV - DC2A01
//	//U8 cVehicleMessage[]=	"#HWSET,0x06,07CE,07EA,0778,07A8,07EC,07DF\r\n#DISPLACE,L,2000,4, , , , , , , , , , , , ,\r\n#currentdata, , , , , , , , , , , , , , , ,\r\nIndex,requestcode,response,startpos,realpos,datasize,datatype,unit,convtype,A,B,C,D,E,F,lut,protocol type\r\nC01,07C60322B002,07CE62B002,10,3,3,1,34,1,1, , , , , , ,D\r\nE01,07E2022101,07EA6101,13,8,2,2,1034,1,0.000488281, , , , , , ,D\r\nE04,07E2022101,07EA6101,12,7,2,2,6,2,0.015625, , , , , , ,D\r\nE0D,07700322BC06,077862BC06,10,1,1,2, ,3,1,1,1,0x10, , ,$0$1$,D\r\nE0B,07E2022101,07EA6101,11,6,2,2,8,1,0.001953125, , , , , , ,D\r\nA04,07E2022101,07EA6101,9,2,1,2, ,3,40191,1,1,0x01, , ,$0$1$,D\r\nA05,07E2022101,07EA6101,9,2,1,2, ,3,40191,1,1,0x02, , ,$0$1$,D\r\nA06,07E2022101,07EA6101,9,2,1,2, ,3,40191,1,1,0x04, , ,$0$1$,D\r\nA07,07E2022101,07EA6101,9,2,1,2, ,3,40191,1,1,0x08, , ,$0$1$,D\r\nT01,07A00322C00B,07A862C00B,10,1,1,1,11,1,1.373, , , , ,252, ,D\r\nT02,07A00322C00B,07A862C00B,10,5,1,1,11,1,1.373, , , , ,252, ,D\r\nT03,07A00322C00B,07A862C00B,10,9,1,1,11,1,1.373, , , , ,252, ,D\r\nT04,07A00322C00B,07A862C00B,10,13,1,1,11,1,1.373, , , , ,252, ,D\r\nS01,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x01, , ,$0$1$,D\r\nS02,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x04, , ,$0$1$,D\r\nS03,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x10, , ,$0$1$,D\r\nS04,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x20, , ,$0$1$,D\r\nE12,07700322BC03,077862BC03,10,2,1,2, ,3,74,1,1,0x01, , ,$0$1$,D\r\nS06,07700322BC03,077862BC03,10,1,1,2, ,3,1,1,1,0x80, , ,$0$1$,D\r\nH01,07E2022103,07EA6103,15,10,2,2,5,2,1, , , , , , ,D\r\nB01,07E4022101,07EC6101,9,1,1,1,8,1,0.5, , , , , , ,D\r\nB02,07E4022101,07EC6101,11,5,2,1,33,2,0.1, , , , , , ,D\r\nB03,07E4022101,07EC6101,12,6,2,1,4,1,0.1, , , , , , ,D\r\nB04,07E4022101,07EC6101,13,7,1,1,3,2,1, , , , , , ,D\r\nB05,07E4022101,07EC6101,13,8,1,1,3,2,1, , , , , , ,D\r\nS0C,07700322BC03,077862BC03,10,3,1,2, ,3,1,1,1,0x02, , ,$0$1$,D\r\nS0D,07700322BC06,077862BC03,10,1,1,2, ,3,1,1,1,0x10, , ,$0$1$,D\r\nS07,07700322BC03,077862BC03,10,3,1,2, ,3,1,1,1,0x04, , ,$0$1$,D\r\nS05,07700322BC03,077862BC03,10,3,1,2, ,3,1,1,1,0x20, , ,$0$1$,D\r\nS0E,07700322BC03,077862BC03,10,3,1,2, ,3,1,1,1,0x10, , ,$0$1$,D\r\nS12,07700322BC04,077862BC04,10,2,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nS11,07700322BC04,077862BC04,10,2,1,2, ,3,1,1,1,0x02, , ,$0$1$,D\r\nS0F,07700322BC04,077862BC04,10,2,1,2, ,3,1,1,1,0x04, , ,$0$1$,D\r\n#currentdata_end, , , , , , , , , , , , , , , , \r\n#readDTC, , , , , , , , , , , , ,\r\nsystem,ECUID,functiontype,requesttime,searchtype,startpos,readno,skipno,totalmasking,openrequest,openresponse,readdtcrequest,readdtcresponse,closedtcrequest,closedtcresponse\r\nCARB,CARB03,0,3,3,5,2,0,000FFF,07DF020100,XX,07DF0103,07XX43, ,\r\nCARB,CARB03,0,3,3,5,2,0,000FFF,07DF020100,XX,07DF0107,07XX47, ,\r\n#readDTC_end, , , , , , , , , , , , ,";
//	//U8 cVehicleOriginal[]=	"#HWSET,0x06,07CE,07EA,0778,07A8,07EC,07DF\r\n#DISPLACE,L,2000,4, , , , , , , , , , , , ,\r\n#currentdata, , , , , , , , , , , , , , , ,\r\nIndex,requestcode,response,startpos,realpos,datasize,datatype,unit,convtype,A,B,C,D,E,F,lut,protocol type\r\nC01,07C60322B002,07CE62B002,10,3,3,1,34,1,1, , , , , , ,D\r\nE01,07E2022101,07EA6101,13,8,2,2,1034,1,0.000488281, , , , , , ,D\r\nE04,07E2022101,07EA6101,12,7,2,2,6,2,0.015625, , , , , , ,D\r\nE0D,07700322BC06,077862BC06,10,1,1,2, ,3,1,1,1,0x10, , ,$0$1$,D\r\nE0B,07E2022101,07EA6101,11,6,2,2,8,1,0.001953125, , , , , , ,D\r\nA04,07E2022101,07EA6101,9,2,1,2, ,3,40191,1,1,0x01, , ,$0$1$,D\r\nA05,07E2022101,07EA6101,9,2,1,2, ,3,40191,1,1,0x02, , ,$0$1$,D\r\nA06,07E2022101,07EA6101,9,2,1,2, ,3,40191,1,1,0x04, , ,$0$1$,D\r\nA07,07E2022101,07EA6101,9,2,1,2, ,3,40191,1,1,0x08, , ,$0$1$,D\r\nT01,07A00322C00B,07A862C00B,10,1,1,1,11,1,1.373, , , , ,252, ,D\r\nT02,07A00322C00B,07A862C00B,10,5,1,1,11,1,1.373, , , , ,252, ,D\r\nT03,07A00322C00B,07A862C00B,10,9,1,1,11,1,1.373, , , , ,252, ,D\r\nT04,07A00322C00B,07A862C00B,10,13,1,1,11,1,1.373, , , , ,252, ,D\r\nS01,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x01, , ,$0$1$,D\r\nS02,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x04, , ,$0$1$,D\r\nS03,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x10, , ,$0$1$,D\r\nS04,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x20, , ,$0$1$,D\r\nE12,07700322BC03,077862BC03,10,2,1,2, ,3,74,1,1,0x01, , ,$0$1$,D\r\nS06,07700322BC03,077862BC03,10,1,1,2, ,3,1,1,1,0x80, , ,$0$1$,D\r\nH01,07E2022103,07EA6103,15,10,2,2,5,2,1, , , , , , ,D\r\nB01,07E4022101,07EC6101,9,1,1,1,8,1,0.5, , , , , , ,D\r\nB02,07E4022101,07EC6101,11,5,2,1,33,2,0.1, , , , , , ,D\r\nB03,07E4022101,07EC6101,12,6,2,1,4,1,0.1, , , , , , ,D\r\nB04,07E4022101,07EC6101,13,7,1,1,3,2,1, , , , , , ,D\r\nB05,07E4022101,07EC6101,13,8,1,1,3,2,1, , , , , , ,D\r\nS0C,07700322BC03,077862BC03,10,3,1,2, ,3,1,1,1,0x02, , ,$0$1$,D\r\nS0D,07700322BC06,077862BC03,10,1,1,2, ,3,1,1,1,0x10, , ,$0$1$,D\r\nS07,07700322BC03,077862BC03,10,3,1,2, ,3,1,1,1,0x04, , ,$0$1$,D\r\nS05,07700322BC03,077862BC03,10,3,1,2, ,3,1,1,1,0x20, , ,$0$1$,D\r\nS0E,07700322BC03,077862BC03,10,3,1,2, ,3,1,1,1,0x10, , ,$0$1$,D\r\nS12,07700322BC04,077862BC04,10,2,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nS11,07700322BC04,077862BC04,10,2,1,2, ,3,1,1,1,0x02, , ,$0$1$,D\r\nS0F,07700322BC04,077862BC04,10,2,1,2, ,3,1,1,1,0x04, , ,$0$1$,D\r\n#currentdata_end, , , , , , , , , , , , , , , , \r\n#readDTC, , , , , , , , , , , , ,\r\nsystem,ECUID,functiontype,requesttime,searchtype,startpos,readno,skipno,totalmasking,openrequest,openresponse,readdtcrequest,readdtcresponse,closedtcrequest,closedtcresponse\r\nCARB,CARB03,0,3,3,5,2,0,000FFF,07DF020100,XX,07DF0103,07XX43, ,\r\nCARB,CARB03,0,3,3,5,2,0,000FFF,07DF020100,XX,07DF0107,07XX47, ,\r\n#readDTC_end, , , , , , , , , , , , ,";
//
//	//AE HEV - DCAH16
//	//U8 cVehicleMessage[]=	"#HWSET,0x06,0778,07A8,07CE,07D8,07D9,07DC,07E8,07E9,07EA,07EB,07EC,07EE\r\n#DISPLACE,G,1600,4\r\n#currentdata, , , , , , , , , , , , , , , ,\r\nIndex,requestcode,response,startpos,realpos,datasize,datatype,unit,convtype,A,B,C,D,E,F,lut,protocol type\r\nE05,07E00322E004,07E862E004,10,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,D\r\nE01,07E00322E001,07E862E001,10,1,1,2,4,1,0.0681, , , , , , ,D\r\nE02,07E00322E001,07E862E001,10,4,2,2,5,1,0.25, , , , , , ,D\r\nE04,07E00322E002,07E862E002,12,7,1,2,6,1,1.25, , , , , , ,D\r\nE0D,07700322BC06,077862BC06,10,1,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nE0B,07E00322E01F,07E862E01F,20,11,1,2,8,1,0.390625, , , , , , ,D\r\nE08,07E00322E00A,07E862E00A,13,5,1,2,8,1,1, , , , ,253,$-$1$2$3$4$5$6$7$-$-$-$-$-$-$-$,D\r\nE09,07E00322E00A,07E862E00A,15,9,1,2,71,1,1, , , , , , ,D\r\nE0A,07E00322E00A,07E862E00A,10,1,2,2,33,2,0.1, , , , , , ,D\r\nE07,07E00322E00A,07E862E00A,13,4,1,2,8,1,1, , , , ,253, ,D\r\nE0C,07E00322E0F1,07E862E0F1,18,57,1,2,1016,1,2, , , , , , ,D\r\nE0E,07E00322E001,07E862E001,18,14,1,2,3,1,0.75,-48, , , , , ,D\r\nE0F,07E00322E004,07E862E004,10,2,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nE12,07700322BC03,077862BC03,10,2,1,2, ,3,74,1,1,0X01, , ,$0$1$,D\r\nE13,07E00322E001,07E862E001,21,20,1,2,3,1,0.75,-48, , , , , ,D\r\nA01,07E1032201A4,07E96201A4,16,14,1,1, ,3,40021,4,1,0x0F, , ,$-$1$2$3$4$5$6$7$8$-$-$-$-$-$-$-$,D\r\nA02,07E1032201A4,07E96201A4,16,13,1,1, ,3,40676,4,1,0x0F, , ,$-$P$-$R$-$N$-$D$-$-$D$-$-$-$-$-$,D\r\nT01,07A00322C00B,07A862C00B,10,1,1,1,11,1,1.373, , , , ,252, ,D\r\nT02,07A00322C00B,07A862C00B,10,5,1,1,11,1,1.373, , , , ,252, ,D\r\nT03,07A00322C00B,07A862C00B,10,9,1,1,11,1,1.373, , , , ,252, ,D\r\nT04,07A00322C00B,07A862C00B,10,13,1,1,11,1,1.373, , , , ,252, ,D\r\nS01,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x01, , ,$0$1$,D\r\nS02,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0X04, , ,$0$1$,D\r\nS03,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0X10, , ,$0$1$,D\r\nS04,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0X20, , ,$0$1$,D\r\nS06,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0X80, , ,$0$1$,D\r\nH01,07E2022103,07EA6103,15,10,2,2,5,2,1, , , , , , ,D\r\nB01,07E4022101,07EC6101,9,1,1,1,8,1,0.5, , , , , , ,D\r\nB02,07E4022101,07EC6101,11,5,2,1,33,2,0.1, , , , , , ,D\r\nB03,07E4022101,07EC6101,12,6,2,1,4,1,0.1, , , , , , ,D\r\nB04,07E4022101,07EC6101,13,7,1,1,3,2,1, , , , , , ,D\r\nB05,07E4022101,07EC6101,13,8,1,1,3,2,1, , , , , , ,D\r\nB06,07E4022101,07EC6101,28,28,1,1, ,3,33,1,2,0X04, , ,$0$1$,D\r\nB07,07E4022101,07EC6101,11,4,1,1, ,3,33,1,2,0X20, , ,$0$1$,D\r\n#currentdata_end, , , , , , , , , , , , , , , ,\r\n#readDTC, , , , , , , , , , , ,\r\nsystem,requesttime,searchtype,startpos,readno,skipno,totalmasking,openrequest,openresponse,readdtcrequest,readdtcresponse,closedtcrequest,closedtcresponse\r\nENGINE,ESU1,261,3,2,6,3,1,FFFFFF,07E0021001,07E850,07E00319028D,07E859,07E00120,07E860\r\nAT,TSD6,261,3,2,6,3,1,FFFFFF,07E1021001,07E950,07E10319020D,07E959,07E10120,07E960\r\nVDCAHB,D1H1,251,3,2,6,3,1,FFFFFF,07D1021001,07D950,07D103190208,07D959,07D10120,07D960\r\nSCCAEB,D0D7,251,3,2,6,3,1,FFFFFF,07D0021001,07D850,07D003190208,07D859,07D00120,07D860\r\nAAF-L,AAF4,250,3,2,5,2,1,FFFF,07E6021081,07EE50,07E6041800FF00,07EE58,07E60120,07EE60\r\nEPS,D4AE,251,3,2,6,3,1,FFFFFF,07D4021001,07DC50,07D403190208,07DC59,07D40120,07DC60\r\nTPMS,TPAD,251,3,6,6,3,1,FFFFFF,07A0021001,07A850,07A003190208,07A859,07A00120,07A860\r\nHCULDC,HH0A,250,3,2,5,2,1,FFFF,07E2021081,07EA50,07E2041800FF00,07EA58,07E20120,07EA60\r\nMCU,HM0A,250,3,2,5,2,1,FFFF,07E3021081,07EB50,07E3041800FF00,07EB58,07E30120,07EB60\r\nBMS,HM0D,250,3,2,5,2,1,FFFF,07E4021081,07EC50,07E4041800FF00,07EC58,07E40120,07EC60\r\n#readDTC_end, , , , , , , , , , , ,";
//	//U8 cVehicleOriginal[]=	"#HWSET,0x06,0778,07A8,07CE,07D8,07D9,07DC,07E8,07E9,07EA,07EB,07EC,07EE\r\n#DISPLACE,G,1600,4\r\n#currentdata, , , , , , , , , , , , , , , ,\r\nIndex,requestcode,response,startpos,realpos,datasize,datatype,unit,convtype,A,B,C,D,E,F,lut,protocol type\r\nE05,07E00322E004,07E862E004,10,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,D\r\nE01,07E00322E001,07E862E001,10,1,1,2,4,1,0.0681, , , , , , ,D\r\nE02,07E00322E001,07E862E001,10,4,2,2,5,1,0.25, , , , , , ,D\r\nE04,07E00322E002,07E862E002,12,7,1,2,6,1,1.25, , , , , , ,D\r\nE0D,07700322BC06,077862BC06,10,1,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nE0B,07E00322E01F,07E862E01F,20,11,1,2,8,1,0.390625, , , , , , ,D\r\nE08,07E00322E00A,07E862E00A,13,5,1,2,8,1,1, , , , ,253,$-$1$2$3$4$5$6$7$-$-$-$-$-$-$-$,D\r\nE09,07E00322E00A,07E862E00A,15,9,1,2,71,1,1, , , , , , ,D\r\nE0A,07E00322E00A,07E862E00A,10,1,2,2,33,2,0.1, , , , , , ,D\r\nE07,07E00322E00A,07E862E00A,13,4,1,2,8,1,1, , , , ,253, ,D\r\nE0C,07E00322E0F1,07E862E0F1,18,57,1,2,1016,1,2, , , , , , ,D\r\nE0E,07E00322E001,07E862E001,18,14,1,2,3,1,0.75,-48, , , , , ,D\r\nE0F,07E00322E004,07E862E004,10,2,1,2, ,3,1,1,1,0x01, , ,$0$1$,D\r\nE12,07700322BC03,077862BC03,10,2,1,2, ,3,74,1,1,0X01, , ,$0$1$,D\r\nE13,07E00322E001,07E862E001,21,20,1,2,3,1,0.75,-48, , , , , ,D\r\nA01,07E1032201A4,07E96201A4,16,14,1,1, ,3,40021,4,1,0x0F, , ,$-$1$2$3$4$5$6$7$8$-$-$-$-$-$-$-$,D\r\nA02,07E1032201A4,07E96201A4,16,13,1,1, ,3,40676,4,1,0x0F, , ,$-$P$-$R$-$N$-$D$-$-$D$-$-$-$-$-$,D\r\nT01,07A00322C00B,07A862C00B,10,1,1,1,11,1,1.373, , , , ,252, ,D\r\nT02,07A00322C00B,07A862C00B,10,5,1,1,11,1,1.373, , , , ,252, ,D\r\nT03,07A00322C00B,07A862C00B,10,9,1,1,11,1,1.373, , , , ,252, ,D\r\nT04,07A00322C00B,07A862C00B,10,13,1,1,11,1,1.373, , , , ,252, ,D\r\nS01,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0x01, , ,$0$1$,D\r\nS02,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0X04, , ,$0$1$,D\r\nS03,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0X10, , ,$0$1$,D\r\nS04,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0X20, , ,$0$1$,D\r\nS06,07700322BC03,077862BC03,10,1,1,2, ,3,74,1,1,0X80, , ,$0$1$,D\r\nH01,07E2022103,07EA6103,15,10,2,2,5,2,1, , , , , , ,D\r\nB01,07E4022101,07EC6101,9,1,1,1,8,1,0.5, , , , , , ,D\r\nB02,07E4022101,07EC6101,11,5,2,1,33,2,0.1, , , , , , ,D\r\nB03,07E4022101,07EC6101,12,6,2,1,4,1,0.1, , , , , , ,D\r\nB04,07E4022101,07EC6101,13,7,1,1,3,2,1, , , , , , ,D\r\nB05,07E4022101,07EC6101,13,8,1,1,3,2,1, , , , , , ,D\r\nB06,07E4022101,07EC6101,28,28,1,1, ,3,33,1,2,0X04, , ,$0$1$,D\r\nB07,07E4022101,07EC6101,11,4,1,1, ,3,33,1,2,0X20, , ,$0$1$,D\r\n#currentdata_end, , , , , , , , , , , , , , , ,\r\n#readDTC, , , , , , , , , , , ,\r\nsystem,requesttime,searchtype,startpos,readno,skipno,totalmasking,openrequest,openresponse,readdtcrequest,readdtcresponse,closedtcrequest,closedtcresponse\r\nENGINE,ESU1,261,3,2,6,3,1,FFFFFF,07E0021001,07E850,07E00319028D,07E859,07E00120,07E860\r\nAT,TSD6,261,3,2,6,3,1,FFFFFF,07E1021001,07E950,07E10319020D,07E959,07E10120,07E960\r\nVDCAHB,D1H1,251,3,2,6,3,1,FFFFFF,07D1021001,07D950,07D103190208,07D959,07D10120,07D960\r\nSCCAEB,D0D7,251,3,2,6,3,1,FFFFFF,07D0021001,07D850,07D003190208,07D859,07D00120,07D860\r\nAAF-L,AAF4,250,3,2,5,2,1,FFFF,07E6021081,07EE50,07E6041800FF00,07EE58,07E60120,07EE60\r\nEPS,D4AE,251,3,2,6,3,1,FFFFFF,07D4021001,07DC50,07D403190208,07DC59,07D40120,07DC60\r\nTPMS,TPAD,251,3,6,6,3,1,FFFFFF,07A0021001,07A850,07A003190208,07A859,07A00120,07A860\r\nHCULDC,HH0A,250,3,2,5,2,1,FFFF,07E2021081,07EA50,07E2041800FF00,07EA58,07E20120,07EA60\r\nMCU,HM0A,250,3,2,5,2,1,FFFF,07E3021081,07EB50,07E3041800FF00,07EB58,07E30120,07EB60\r\nBMS,HM0D,250,3,2,5,2,1,FFFF,07E4021081,07EC50,07E4041800FF00,07EC58,07E40120,07EC60\r\n#readDTC_end, , , , , , , , , , , ,";
//
//	//MD G1.6- DC2C00
//	U8 cVehicleMessage[]=	"#HWSET,0x06,04F0,0545,0316,0329,07E8,059B,0370,07DE,0018,0690,07DF\r\n#DISPLACE,G,1600,4,,,,,,,,,,,,,\r\n#currentdata, , , , , , , , , , , , , , , ,\r\nIndex,requestcode,response,startpos,realpos,datasize,datatype,unit,convtype,A,B,C,D,E,F,lut,protocol type\r\nC01,04F0,04F0,8,1,3,2,34,1,0.1, , , , , , ,V\r\nE01,0545,0545,6,1,1,2,1034,1,0.1015625, , , , , , ,V\r\nE02,0316,0316,5,1,2,2,5,1,0.25, , , , , , ,V\r\nE04,0316,0316,9,1,1,2,6,1,1, , , , , , ,V\r\nE05,0545,0545,3,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nE06,07E0022100,07E86100,13,4,1,2, ,3,1,1,1,0x20, , , ,D\r\nE07,07E0022104,07E86104,27,23,2,2,8,1,0.012207, , , , ,253, ,D\r\nE08,07E0022104,07E86104,12,5,1,2,8,1,1, , , , ,253, ,D\r\nE09,07E0022104,07E86104,28,24,1,2,71,1,1, , , , , , ,D\r\nE0A,07E0022104,07E86104,18,11,2,2,33,2,0.001, , , , , , ,D\r\nE0B,0329,0329,9,1,1,2,8,1,0.392156863, , , , , , ,V\r\nE0C,0545,0545,4,1,2,2,1016,1,0.128, , , , , , ,V\r\nE0D,0329,0329,7,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nE12,0690,0690,5,1,1,2, ,3,1,1,1,0x40, , ,$0$1$,V\r\nA01,0370,0370,5,1,1,2, ,3,1,1,1,0xF0, , ,$-$1$2$3$4$5$6$7$-$-$-$-$-$-$-$,V\r\nA02,059B,059B,4,1,1,2, ,3,1,1,1,0x0F, , ,$P$-$-$-$-$D$N$R$D$-$-$-$-$-$-$,V\r\nT01,07D6022106,07DE6106,6,0,2,1,23,1,1, , , , ,252, ,D\r\nT02,07D6022106,07DE6106,6,6,2,1,23,1,1, , , , ,252, ,D\r\nT03,07D6022106,07DE6106,6,18,2,1,23,1,1, , , , ,252, ,D\r\nT04,07D6022106,07DE6106,6,12,2,1,23,1,1, , , , ,252, ,D\r\nS04,0690,0690,3,1,1,2, ,3,1,1,1,0x40, , ,$0$1$,V\r\nS05,0690,0690,7,1,1,2, ,3,1,1,1,0x20, , ,$0$1$,V\r\nS06,0690,0690,5,1,1,2, ,3,1,1,1,0x04, , ,$0$1$,V\r\nS07,0018,0018,4,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS0C,0018,0018,4,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS0D,0329,0329,7,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS0E,0018,0018,5,1,1,2, ,3,1,1,1,0x60, , ,$0$1$,V\r\nS0F,0690,0690,6,1,1,2, ,3,1,1,1,0x03, , ,$0$1$,V\r\nS11,0690,0690,8,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS12,0690,0690,8,1,1,2, ,3,1,1,1,0x01, , ,$0$1$,V\r\n#currentdata_end, , , , , , , , , , , , , , , , \r\n#readDTC, , , , , , , , , , , , , ,\r\nsystem,ecuid,functiontype,requesttime,searchtype,startpos,readno,skipno,totalmasking,openrequest,openresponse,readdtcrequest,readdtcresponse,closedtcrequest,closedtcresponse\r\nCARB,CARB03,0,3,3,5,2,0,000FFF,07DF020100,XX,07DF0103,07XX43, ,\r\nCARB,CARB03,0,3,3,5,2,0,000FFF,07DF020100,XX,07DF0107,07XX47, ,\r\n#readDTC_end, , , , , , , , , , , , , ,";
//	U8 cVehicleOriginal[]=	"#HWSET,0x06,04F0,0545,0316,0329,07E8,059B,0370,07DE,0018,0690,07DF\r\n#DISPLACE,G,1600,4,,,,,,,,,,,,,\r\n#currentdata, , , , , , , , , , , , , , , ,\r\nIndex,requestcode,response,startpos,realpos,datasize,datatype,unit,convtype,A,B,C,D,E,F,lut,protocol type\r\nC01,04F0,04F0,8,1,3,2,34,1,0.1, , , , , , ,V\r\nE01,0545,0545,6,1,1,2,1034,1,0.1015625, , , , , , ,V\r\nE02,0316,0316,5,1,2,2,5,1,0.25, , , , , , ,V\r\nE04,0316,0316,9,1,1,2,6,1,1, , , , , , ,V\r\nE05,0545,0545,3,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nE06,07E0022100,07E86100,13,4,1,2, ,3,1,1,1,0x20, , , ,D\r\nE07,07E0022104,07E86104,27,23,2,2,8,1,0.012207, , , , ,253, ,D\r\nE08,07E0022104,07E86104,12,5,1,2,8,1,1, , , , ,253, ,D\r\nE09,07E0022104,07E86104,28,24,1,2,71,1,1, , , , , , ,D\r\nE0A,07E0022104,07E86104,18,11,2,2,33,2,0.001, , , , , , ,D\r\nE0B,0329,0329,9,1,1,2,8,1,0.392156863, , , , , , ,V\r\nE0C,0545,0545,4,1,2,2,1016,1,0.128, , , , , , ,V\r\nE0D,0329,0329,7,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nE12,0690,0690,5,1,1,2, ,3,1,1,1,0x40, , ,$0$1$,V\r\nA01,0370,0370,5,1,1,2, ,3,1,1,1,0xF0, , ,$-$1$2$3$4$5$6$7$-$-$-$-$-$-$-$,V\r\nA02,059B,059B,4,1,1,2, ,3,1,1,1,0x0F, , ,$P$-$-$-$-$D$N$R$D$-$-$-$-$-$-$,V\r\nT01,07D6022106,07DE6106,6,0,2,1,23,1,1, , , , ,252, ,D\r\nT02,07D6022106,07DE6106,6,6,2,1,23,1,1, , , , ,252, ,D\r\nT03,07D6022106,07DE6106,6,18,2,1,23,1,1, , , , ,252, ,D\r\nT04,07D6022106,07DE6106,6,12,2,1,23,1,1, , , , ,252, ,D\r\nS04,0690,0690,3,1,1,2, ,3,1,1,1,0x40, , ,$0$1$,V\r\nS05,0690,0690,7,1,1,2, ,3,1,1,1,0x20, , ,$0$1$,V\r\nS06,0690,0690,5,1,1,2, ,3,1,1,1,0x04, , ,$0$1$,V\r\nS07,0018,0018,4,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS0C,0018,0018,4,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS0D,0329,0329,7,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS0E,0018,0018,5,1,1,2, ,3,1,1,1,0x60, , ,$0$1$,V\r\nS0F,0690,0690,6,1,1,2, ,3,1,1,1,0x03, , ,$0$1$,V\r\nS11,0690,0690,8,1,1,2, ,3,1,1,1,0x02, , ,$0$1$,V\r\nS12,0690,0690,8,1,1,2, ,3,1,1,1,0x01, , ,$0$1$,V\r\n#currentdata_end, , , , , , , , , , , , , , , , \r\n#readDTC, , , , , , , , , , , , , ,\r\nsystem,ecuid,functiontype,requesttime,searchtype,startpos,readno,skipno,totalmasking,openrequest,openresponse,readdtcrequest,readdtcresponse,closedtcrequest,closedtcresponse\r\nCARB,CARB03,0,3,3,5,2,0,000FFF,07DF020100,XX,07DF0103,07XX43, ,\r\nCARB,CARB03,0,3,3,5,2,0,000FFF,07DF020100,XX,07DF0107,07XX47, ,\r\n#readDTC_end, , , , , , , , , , , , , ,";
//
//	//Current DB READ
//	if(!GetSlaveData(cVehicleMessage,&g_usCurrentCnt)) ret=FALSE;
//
//	//Current DB DTC READ
//	memcpy(cVehicleMessage,cVehicleOriginal, sizeof(cVehicleMessage)/sizeof(cVehicleMessage[0]));
//	if(!GetSlaveDTCData(cVehicleMessage,&g_ucDtcSystemCnt)) ret=FALSE;
//
//	//NAVI #HWSET READ
//    memcpy(cVehicleMessage,cVehicleOriginal, sizeof(cVehicleMessage)/sizeof(cVehicleMessage[0]));
//	if(!HardwareSetData(cVehicleMessage, "#HWSET") )ret=FALSE;
//
//	//NAVI  #DISPLACE
//	memcpy(cVehicleMessage,cVehicleOriginal, sizeof(cVehicleMessage)/sizeof(cVehicleMessage[0]));
//	if(!DisplaceSetData(cVehicleMessage, "#DISPLACE") )ret=FALSE;

#else	//defined(SIMULATION)
	
#if defined(MALLOC_MODIFY)
	unsigned int uiFlashAddress;
	U8 *cVehicleMessage = NULL;

	if ( memcmp(g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME) == 0 ||
         g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize > (APPLICATION_ADDRESS - CAR_SLAVE_ADDRESS)   )
	{
		Trace(0,"*****************************************************\r\n");
		Trace(0,"%s : there has no Slave DB \r\n", __FUNCTION__);
		Trace(0,"*****************************************************\r\n");
		return 0;
	}
	cVehicleMessage = malloc(g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
#ifdef CHECK_MALLOC
	__iar_dlmalloc_stats();
#endif
	uiFlashAddress = CAR_SLAVE_ADDRESS;
	DecyptCarDB_FromFlash(cVehicleMessage, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	
	//Current DB READ
	if(!GetSlaveDataSize(cVehicleMessage,&g_usCurrentCnt)) ret=FALSE;
	if( ret == true )
	{
		DecyptCarDB_FromFlash(cVehicleMessage, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
		if(!GetSlaveData_Modify(cVehicleMessage,&g_usCurrentCnt)) ret=FALSE;
	}

	//Current DB DTC READ
	memset(cVehicleMessage,0x00,sizeof(g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize));
	DecyptCarDB_FromFlash(cVehicleMessage, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	if(!GetSlaveDTCData(cVehicleMessage,&g_ucDtcSystemCnt)) ret=FALSE;

	//NAVI  #DISPLACE
	memset(cVehicleMessage,0x00,sizeof(g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize));
	DecyptCarDB_FromFlash(cVehicleMessage, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	if(!DisplaceSetData(cVehicleMessage, "#DISPLACE") )ret=FALSE;
	
	//NAVI #HWSET READ
    memset(cVehicleMessage,0x00,sizeof(g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize));
	DecyptCarDB_FromFlash(cVehicleMessage, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	if(!HardwareSetData(cVehicleMessage, "#HWSET") )ret=FALSE;

	//#function READ
	memset(cVehicleMessage,0x00,sizeof(g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize));
	DecyptCarDB_FromFlash(cVehicleMessage, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	FunctionSetData(cVehicleMessage,"#function"); //ret=FALSE;
	
	//TPMS 알람
	memset(cVehicleMessage,0x00,sizeof(g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize));
	DecyptCarDB_FromFlash(cVehicleMessage, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	if(!TpmsAlramSetData(cVehicleMessage, "#TPMSALRAM") )
	{	//알람값은 DB에없어도 기본설정
		//ret = TRUE;
		g_uiTpmsAlramValueSave = g_uiTpmsAlramValue = 172;		/* 25 PSI  = 172.368332 kPa*/
	}
	
    for(int i=0; i<g_usCurrentCnt; i++)
    {
        if(g_pstCurrDataBase[i].m_cCommType == 'D')
        {
            if(CFD_GetCanFDAdapter()) 
            {
                g_pstCurrDataBase[i].m_ucCanLine = CANFD_SPI_1;
            }
            else
            {
            	g_pstCurrDataBase[i].m_ucCanLine = HIGHCAN1;
            }
        }
        else
        {
        g_pstCurrDataBase[i].m_ucCanLine = g_ucSaveSlaveCanLine;
    }
    }
	
	if ( cVehicleMessage != NULL )
	{
		free(cVehicleMessage);
		cVehicleMessage=NULL;
	}
#else	//defined(MALLOC_MODIFY)
	unsigned int uiFlashAddress;
	U8 *cVehicleMessage = NULL;
	U8 *cVehicleOriginal = NULL;

	if ( memcmp(g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, DEFAULT_FILE_NAME, MAX_FW_DB_FILE_NAME) == 0 ||
         g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize > (APPLICATION_ADDRESS - CAR_SLAVE_ADDRESS)   )
	{
		Trace(0,"*****************************************************\r\n");
		Trace(0,"%s : there has no Slave DB \r\n", __FUNCTION__);
		Trace(0,"*****************************************************\r\n");
		return 0;
	}
	cVehicleMessage = malloc(g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	cVehicleOriginal = malloc(g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);

	if (cVehicleMessage == NULL || cVehicleOriginal == NULL )
	{
		printf("%s : malloc fail so return false \r\n", __FUNCTION__);

		if ( cVehicleMessage != NULL )
		{
			free(cVehicleMessage);
			cVehicleMessage=NULL;
		}
		if ( cVehicleOriginal != NULL )
		{
			free(cVehicleOriginal);
			cVehicleOriginal=NULL;
		}
		return 0;
	}

	uiFlashAddress = CAR_SLAVE_ADDRESS;
	DecyptCarDB_FromFlash(cVehicleMessage, uiFlashAddress, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	memcpy(cVehicleOriginal, cVehicleMessage, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);

	//Current DB READ
	if(!GetSlaveData(cVehicleMessage,&g_usCurrentCnt)) ret=FALSE;

	//Current DB DTC READ
	memcpy(cVehicleMessage,cVehicleOriginal,g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	if(!GetSlaveDTCData(cVehicleMessage,&g_ucDtcSystemCnt)) ret=FALSE;

	//NAVI  #DISPLACE
	memcpy(cVehicleMessage,cVehicleOriginal, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	if(!DisplaceSetData(cVehicleMessage, "#DISPLACE") )ret=FALSE;
	
	//NAVI #HWSET READ
    memcpy(cVehicleMessage,cVehicleOriginal,g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	if(!HardwareSetData(cVehicleMessage, "#HWSET") )ret=FALSE;

	//#function READ
	memcpy(cVehicleMessage,cVehicleOriginal,g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	FunctionSetData(cVehicleMessage,"#function");//ret=FALSE; 
	
	//TPMS 알람
	memcpy(cVehicleMessage,cVehicleOriginal, g_FirmwareInfo.AppProperty[eApp_SlaveDB].wFirmwareSize);
	if(!TpmsAlramSetData(cVehicleMessage, "#TPMSALRAM") )
	{	//알람값은 DB에없어도 기본설정
		//ret = TRUE;
		g_uiTpmsAlramValueSave = g_uiTpmsAlramValue = 172;		/* 25 PSI  = 172.368332 kPa*/
	}
	
	if ( cVehicleMessage != NULL )
	{
		free(cVehicleMessage);
		cVehicleMessage=NULL;
	}
	if ( cVehicleOriginal != NULL )
	{
		free(cVehicleOriginal);
		cVehicleOriginal=NULL;
	}
#endif	//defined(MALLOC_MODIFY)
#endif	//defined(SIMULATION)
	return ret;
}

bool GetDataBaseParsing_Reference()
{
	//리퀘스트 추출

	stRequestCnt[eCOMM_DTC_FUNCTION].m_cRequestCnt = SetRequest(0, g_ucDtcSystemCnt, eCOMM_DTC_FUNCTION);

//	if(ODO_SUPP_GET_STATE() == ODO_SUPP_READY)	//supported 있을경우	- 현재 사용하지 않는다 뜯어 고쳐야 한다
//	{
//		stRequestCnt[eCOMM_SUPP_FUNCTION].m_cRequestCnt = SetRequest(0, 1, eCOMM_SUPP_FUNCTION);
//	}

	//참조 테이블
	g_psrFastRefTable=SetReferenceTable(g_pstFastFunction, g_pstCurrDataBase, g_usMasterCnt[0], g_usCurrentCnt, &g_usFastTableCnt);
	if ( g_pstFastFunction != NULL)
	{
		free(g_pstFastFunction);
		g_pstFastFunction=NULL;
	}
	//리퀘스트 추출
	if(g_usFastTableCnt!=0) stRequestCnt[eCOMM_FAST_FUNCTION].m_cRequestCnt = SetRequest(g_psrFastRefTable, g_usFastTableCnt, eCOMM_FAST_FUNCTION);

	//참조 테이블
	g_psrSlow1RefTable=SetReferenceTable(g_pstSlow1Function, g_pstCurrDataBase, g_usMasterCnt[1], g_usCurrentCnt, &g_usSlow1TableCnt);
	if ( g_pstSlow1Function != NULL)
	{
		free(g_pstSlow1Function);
		g_pstSlow1Function=NULL;
	}
	//리퀘스트 추출
	if(g_usSlow1TableCnt!=0) stRequestCnt[eCOMM_SLOW1_FUNCTION].m_cRequestCnt = SetRequest(g_psrSlow1RefTable, g_usSlow1TableCnt, eCOMM_SLOW1_FUNCTION);

	//참조 테이블
	g_psrSlow2RefTable=SetReferenceTable(g_pstSlow2Function, g_pstCurrDataBase, g_usMasterCnt[2], g_usCurrentCnt, &g_usSlow2TableCnt);
	if ( g_pstSlow2Function != NULL)
	{
		free(g_pstSlow2Function);
		g_pstSlow2Function=NULL;
	}
	//리퀘스트 추출
	if(g_usSlow2TableCnt!=0) stRequestCnt[eCOMM_SLOW2_FUNCTION].m_cRequestCnt = SetRequest(g_psrSlow2RefTable, g_usSlow2TableCnt, eCOMM_SLOW2_FUNCTION);

	//참조 테이블
	g_psrSlow3RefTable=SetReferenceTable(g_pstSlow3Function, g_pstCurrDataBase, g_usMasterCnt[3], g_usCurrentCnt, &g_usSlow3TableCnt);
	if ( g_pstSlow3Function != NULL)
	{
		free(g_pstSlow3Function);
		g_pstSlow3Function=NULL;
	}
	//리퀘스트 추출
	if(g_usSlow3TableCnt!=0) stRequestCnt[eCOMM_SLOW3_FUNCTION].m_cRequestCnt = SetRequest(g_psrSlow3RefTable, g_usSlow3TableCnt, eCOMM_SLOW3_FUNCTION);

	return 1;
}


bool GetMasterData(U8 *cMassage, char message[], char endMessage[], U16 *nCnt)
{
	BOOL bFoundStartMessage = false;
	char *p, *word;
	char *lastPos[2];
	int i=0, iTokenCnt=0;
	stMasterData Function[MASTER_DATA_MAX]={0,};

	p=strtok_r((char*)cMassage, "\r\n", &lastPos[0]);

	while(p!=NULL)	//LINE이 NULL
	{
		if (strncmp(message, p, strlen(message))==0 && bFoundStartMessage == false)		//LINE 첫 문장이 message와 비교(첫 # SEARCHING)
		{
			bFoundStartMessage = true;
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp("index", p, strlen("index"))==0)				//"index" 열 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp(endMessage, p, strlen(endMessage))==0)	//LINE 첫 문장이 message와 비교 종료(첫 # SEARCHING)
		{
			break;
		}

		if ( bFoundStartMessage == true )
		{
			char *cParseData;
			int size = sizeof(char) * (lastPos[0]- p);

			cParseData=(char *)malloc(size);
			if(cParseData == NULL)
			{
				printf("%s : malloc fail so return false \r\n", __FUNCTION__);
				return 0;
			}
			strncpy(cParseData, p, size);
			word=strtok_r(cParseData, ",",  &lastPos[1]);	// , 구분자

			while( word != NULL )	//한LINE의 끝까지
			{
				//DebugPrint(word);
				if( iTokenCnt%MASTER_DATA_FIELD == 0 )	//Index
				{
					strncpy(Function[i].m_cIndex, word, sizeof(Function[i].m_cIndex)/sizeof(Function[i].m_cIndex[0]));
				}
				else if( iTokenCnt%MASTER_DATA_FIELD == 1 )	//Function Type
				{
					Function[i].m_cFuncType = atoi(word);
				}
				else if( iTokenCnt%MASTER_DATA_FIELD == 2 )	//Ref_Current
				{
					strcpy(Function[i].m_cRef, word);
				}
				else if( iTokenCnt%MASTER_DATA_FIELD == 3 )	//Float Range
				{
					Function[i].m_cFloatRange =*word;
				}
				else if( iTokenCnt%MASTER_DATA_FIELD == 4 )	//UNIT
				{
					Function[i].m_cDescUnit=*word;
				}
				else if( iTokenCnt%MASTER_DATA_FIELD == 5 )	//Conversion Rule
				{
					Function[i].m_cDataSize=*word;
				}
				iTokenCnt++;
				word=strtok_r(NULL, ",",  &lastPos[1]);
			}
			iTokenCnt=0;
			if(cParseData != NULL)
			{
				free(cParseData);
				cParseData = NULL;
			}
		}
		if(bFoundStartMessage==true)	i++;
		if( i == MASTER_DATA_MAX )
		{
			Trace(0,"MASTER_DATA_MAX over\r\n");
			break;
		}
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//다음 LINE
	}
	if ( bFoundStartMessage == false ) return 0;		//원하는 펑션을 찾지못하면 FAIL

	if(strcmp(message, "#FAST")==0)
	{
		g_pstFastFunction=(stMasterData *)malloc(sizeof(stMasterData)*i);
		if(g_pstFastFunction != NULL)
			memcpy(g_pstFastFunction,Function,sizeof(stMasterData)*i);
		else
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}
		nCnt[0]=i;
	}
	else if(strcmp(message, "#SLOW1")==0)
	{
		g_pstSlow1Function=(stMasterData *)malloc(sizeof(stMasterData)*i);
		if(g_pstSlow1Function != NULL)
			memcpy(g_pstSlow1Function,Function,sizeof(stMasterData)*i);
		else
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}
		nCnt[1]=i;
	}
	else if(strcmp(message, "#SLOW2")==0)
	{
		g_pstSlow2Function=(stMasterData *)malloc(sizeof(stMasterData)*i);
		if(g_pstSlow2Function != NULL)
			memcpy(g_pstSlow2Function,Function,sizeof(stMasterData)*i);
		else
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}
		nCnt[2]=i;
	}
	else if(strcmp(message, "#SLOW3")==0)
	{
		g_pstSlow3Function=(stMasterData *)malloc(sizeof(stMasterData)*i);
		if(g_pstSlow3Function != NULL)
			memcpy(g_pstSlow3Function,Function,sizeof(stMasterData)*i);
		else
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}
		nCnt[3]=i;
	}
#ifdef CHECK_MALLOC
		printf("GetMasterData\r\n");
	__iar_dlmalloc_stats();
#endif
	Trace(0,"[%s] \r\n", __FUNCTION__);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(strcmp(message, "#FAST")==0)
	{
		Trace(DBPARSING_LOG,"%s MESSAGE %d \r\n" ,message,  nCnt[0]);
		for(i=0; i<nCnt[0]; i++)
		{
		Trace(DBPARSING_LOG,"%s %d index : %s\r\n", message, i+1, g_pstFastFunction[i].m_cIndex);
		}
	}
	else if(strcmp(message, "#SLOW1")==0)
	{
		Trace(DBPARSING_LOG,"%s MESSAGE %d \r\n" ,message,  nCnt[1]);
		for(i=0; i<nCnt[1]; i++)
		{
			Trace(DBPARSING_LOG,"%s %d index : %s\r\n", message, i+1, g_pstSlow1Function[i].m_cIndex);
		}
	}
	else if(strcmp(message, "#SLOW2")==0)
	{
		Trace(DBPARSING_LOG,"%s MESSAGE %d \r\n" ,message,  nCnt[2]);
		for(i=0; i<nCnt[2]; i++)
		{
			Trace(DBPARSING_LOG,"%s %d index : %s\r\n", message, i+1, g_pstSlow2Function[i].m_cIndex);
		}
	}
	else if(strcmp(message, "#SLOW3")==0)
	{
		Trace(DBPARSING_LOG,"%s MESSAGE %d \r\n" ,message,  nCnt[3]);
		for(i=0; i<nCnt[3]; i++)
		{
			Trace(DBPARSING_LOG,"%s %d index : %s\r\n", message, i+1, g_pstSlow3Function[i].m_cIndex);
		}
	}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	return 1;
}

#if 0
bool GetSlaveData(U8 *cVehicleMassage, U16 *nCnt)
{
	BOOL bFoundStartMessage = false;
	char *p, *word;
	char *lastPos[2];
	int i=0;
	unsigned char ucTokenCnt=0;
	char message[]="#currentdata", endMessage[]="#currentdata_end";
	char ucData[100];

	stSlaveData Data[SLAVE_DATA_MAX]={0,};
	p=strtok_r((char*)cVehicleMassage, "\r\n", &lastPos[0]);

	while(p!=NULL)	//LINE이 NULL
	{
		if (strncmp(message, p, strlen(message))==0 && bFoundStartMessage == false)		//LINE 첫 문장이 message와 비교(첫 # SEARCHING)
		{
			bFoundStartMessage = true;
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp("Index", p, strlen("Index"))==0)	//"index_FIN(임시)" 열 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp(endMessage, p, strlen(endMessage))==0)	//LINE 첫 문장이 message와 비교 종료(첫 # SEARCHING)
		{
			break;
		}
		else if(bFoundStartMessage == false)							//시작점"#currentdata" 찾기 전까지 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}

		if ( bFoundStartMessage == true )
		{
			//char *cParseData;
			char cParseData[200];
			int size = sizeof(char) * (lastPos[0]- p);

			strncpy(cParseData, p,size);
			word=strtok_r(cParseData, ",",  &lastPos[1]);	// , 구분자

			while( word != NULL )	//한LINE의 끝까지
			{
					switch( ucTokenCnt )
				{
				case eParse_Index_IDX:		//Index Fine
					strncpy(Data[i].m_cIndexFine, word, sizeof(Data[i].m_cIndexFine) / sizeof(Data[i].m_cIndexFine[0]));
					if(strncmp(Data[i].m_cIndexFine, DCS_ODOMETER_SUPPORTED, DCS_CURRENT_INDEX_SIZE)==0)
					{
						g_ucSuppTableNum = i;
						ODO_SUPP_SET_STATE(ODO_SUPP_HI);		//ODO_SUPP_READY 가 되어야 할 것 같은데...
					}
					if(strncmp(Data[i].m_cIndexFine, DCS_ODOMETER, DCS_CURRENT_INDEX_SIZE)==0 && ODO_SUPP_GET_STATE() == ODO_SUPP_NONE)
					{
						ODO_TYPE_SET_STATE(ODO_TYPE_SUPP_DCS_ODOMETER);
						Trace(0,"ODO_TYPE_SUPP_DCS_ODOMETER \r\n");
					}
					if(strncmp(Data[i].m_cIndexFine, DCS_DISTANCE_CLEAR_DTC, DCS_CURRENT_INDEX_SIZE)==0 && ODO_SUPP_GET_STATE() == ODO_SUPP_NONE)
					{
						ODO_TYPE_SET_STATE(ODO_TYPE_CARB);
					}
					if(strncmp(Data[i].m_cIndexFine, DCS_MAF_SENSOR, DCS_CURRENT_INDEX_SIZE)==0)
					{
						g_eMapMaf = eMAPMAF_MAF;
					}
					if(strncmp(Data[i].m_cIndexFine, DCS_MAP_SENSOR, DCS_CURRENT_INDEX_SIZE)==0)
					{
						if(g_eMapMaf!=eMAPMAF_MAF) g_eMapMaf = eMAPMAF_MAP;
					}
					if(strncmp(Data[i].m_cIndexFine, DCS_TIRE_CONVERT, DCS_CURRENT_INDEX_SIZE)==0)
					{
						printf("g_bTPMSConvertFlag On\r\n");
						g_bTPMSConvertFlag = true;
					}
					if(strncmp(Data[i].m_cIndexFine, DCS_ODOMETER, DCS_CURRENT_INDEX_SIZE)==0)
					{
						if(Data[i].m_cCommType == 'V')	g_ucOdoGarbageTotal = 20;    // Vehicle CAN _시동 초기 data 20ea 버림
						else											g_ucOdoGarbageTotal = 10;    // Diagnosis CAN _시동 초기 data 10ea 버림
					}
					break;
				case eParse_Request_IDX:	//Request
					strcpy(Data[i].m_cReqNode, word);
					break;
				case eParse_Response_IDX:	//Response
					strcpy(Data[i].m_cResNode, word);
					break;
				case eParse_StartPos_IDX:	//Start Pos
					Data[i].m_cStartPos =atoi(word);
					break;
				case eParse_RealPos_IDX:	//Real Pos
					Data[i].m_cRealPos=atoi(word);
					break;
				case eParse_DataSize_IDX:	//Data Size
					Data[i].m_cDataSize=atoi(word);
					break;
				case eParse_DataType_IDX:	//Data Type
					Data[i].m_cDataType=atoi(word);
					break;
				case eParse_Unit_IDX:			//Unit
					Data[i].m_cUnit=atoi(word);
					break;
				case eParse_ConvRule_IDX:	//ConvRule
					Data[i].m_cCunvRule=atoi(word);
					break;
				case eParse_A_IDX:			//A
					Data[i].m_cA=atof(word);
					break;
				case eParse_B_IDX:			//B
					Data[i].m_cB=atof(word);
					break;
				case eParse_C_IDX:			//C
					if( Data[i].m_cCunvRule == 5 )
					{
						memset(ucData,0x00,sizeof(ucData));
						strcpy(ucData, word);
						Data[i].m_cC=strtol(ucData, NULL, 16);
					}
					else
						Data[i].m_cC=atof(word);
					break;
				case eParse_D_IDX:			//D
                    if( Data[i].m_cCunvRule == 5 )		//!!!!!!!!!!!!!!!!!!!!!!!!!상준책임 확인필요!!!!!!!!!!!!!!!!!!!!!!
					{
                        Data[i].m_cD=atoi(word);
                    }
                    else
                    {
                        Data[i].m_cD=AsciiToHex(word[2], word[3]);
                    }
					break;
				case eParse_E_IDX:			//E
					Data[i].m_cE=atoi(word);
					break;
				case eParse_F_IDX:			//F
					Data[i].m_cF=atoi(word);
					break;
				case eParse_LUT_IDX:			//LUT
					if(Data[i].m_cCunvRule==3)
						strcpy(Data[i].m_cLut, word);
					else if(Data[i].m_cCunvRule==1 || Data[i].m_cCunvRule==2)
						memset(Data[i].m_cLut,0x00,sizeof(Data[i].m_cLut));
					else
						strcpy(Data[i].m_cLut, LUT_20_Parsing(word));
					break;
				case eParse_CommType_IDX:
					Data[i].m_cCommType= *word;
					break;
				default:
					break;
				}

				ucTokenCnt++;
				if(ucTokenCnt==15 && Data[i].m_cCunvRule==20)
				{
					word=strtok_r(NULL, "\r\n",  &lastPos[1]);
				}
				else
				{
					word=strtok_r(NULL, ",",  &lastPos[1]);
				}
			}
			ucTokenCnt=0;
		}
		i++;
		if( i == SLAVE_DATA_MAX )
		{
			Trace(0,"SLAVE_DATA_MAX over\r\n");
			break;
		}
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//다음 LINE
	}
	if ( bFoundStartMessage == false ) return 0;		//원하는 펑션을 찾지못하면 FAIL



	//m =__iar_dlmallinfo();


	g_pstCurrDataBase=(stSlaveData *)malloc(sizeof(stSlaveData)*i);
	if ( g_pstCurrDataBase != NULL)
		memcpy(g_pstCurrDataBase, Data, sizeof(stSlaveData)*i);
	else
	{
		Trace(DBPARSING_LOG,"%s : malloc fail so return false \r\n", __FUNCTION__);
		return 0;
	}
	//__iar_dlmalloc_stats();
	*nCnt=i;	//navi 갯수

	Trace(DBPARSING_LOG,"[%s] \r\n", __FUNCTION__);
	Trace(DBPARSING_LOG,"SLAVE Current %d \r\n" , *nCnt);
	for(i=0; i<*nCnt; i++)
	{
		Trace(DBPARSING_LOG,"%d index : %c%c%c \r\n", i+1,
					   g_pstCurrDataBase[i].m_cIndexFine[0],
					   g_pstCurrDataBase[i].m_cIndexFine[1],
					   (g_pstCurrDataBase[i].m_cIndexFine[2]=='\0') ? ' ':g_pstCurrDataBase[i].m_cIndexFine[2]);
		//Trace(DBPARSING_LOG,"\r\n");
	}
//        uint16_t nTemp;
	Trace(DBPARSING_LOG,"DEBUG_CAN_PARSING\r\n");
	for(i=0; i<*nCnt; i++)
   {
			if(g_pstCurrDataBase[i].m_cCommType == 'V'){
					  Trace(DBPARSING_LOG,"ODO Data - Vehicle CAN, %c \r\n",g_pstCurrDataBase[i].m_cCommType);
					   break;
			 }
			 else{
					  Trace(DBPARSING_LOG,"ODO Data - Diagnosis CAN, %c \r\n",g_pstCurrDataBase[i].m_cCommType);
					   break;
			 }
   }

	return 1;	//SLAVE_DATA_MAX 크기때문에 죽을 수 있다. 체크 필요
}
#endif

#if defined(MALLOC_MODIFY)
bool GetSlaveData_Modify(U8 *cVehicleMassage, U16 *nCnt)
{
	BOOL bFoundStartMessage = false;
	char *p, *word, *temp;
	char *lastPos[2];
	int i=0;
	unsigned char ucTokenCnt=0;
	char message[]="#currentdata", endMessage[]="#currentdata_end";
	char ucData[100];

//	stSlaveData Data;
	p=strtok_r((char*)cVehicleMassage, "\r\n", &lastPos[0]);

	while(p!=NULL)	//LINE이 NULL
	{
		if (strncmp(message, p, strlen(message))==0 && bFoundStartMessage == false)		//LINE 첫 문장이 message와 비교(첫 # SEARCHING)
		{
			bFoundStartMessage = true;
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp("Index", p, strlen("Index"))==0)	//"index_FIN(임시)" 열 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp(endMessage, p, strlen(endMessage))==0)	//LINE 첫 문장이 message와 비교 종료(첫 # SEARCHING)
		{
			break;
		}
		else if(bFoundStartMessage == false)							//시작점"#currentdata" 찾기 전까지 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}

		if ( bFoundStartMessage == true )
		{
			char cParseData[200];
			int size = sizeof(char) * (lastPos[0]- p);

			strncpy(cParseData, p,size);
			//word=strtok_r(cParseData, ",",  &lastPos[1]);	// , 구분자

            
            temp = cParseData;
			while(( word = strsep(&temp, ",")) != NULL )	//한LINE의 끝까지
			{
				switch( ucTokenCnt )
				{
				case eParse_Index_IDX:		//Index Fine
					strncpy(g_pstCurrDataBase[i].m_cIndexFine, word, sizeof(g_pstCurrDataBase[i].m_cIndexFine) / sizeof(g_pstCurrDataBase[i].m_cIndexFine[0]));
					if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ODOMETER_SUPPORTED, DCS_CURRENT_INDEX_SIZE)==0)
					{
						g_ucSuppTableNum = i;
						ODO_SUPP_SET_STATE(ODO_SUPP_HI);		//ODO_SUPP_READY 가 되어야 할 것 같은데...
					}
					if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ODOMETER, DCS_CURRENT_INDEX_SIZE)==0 && ODO_SUPP_GET_STATE() == ODO_SUPP_NONE)
					{
						ODO_TYPE_SET_STATE(ODO_TYPE_SUPP_DCS_ODOMETER);
						Trace(0,"ODO_TYPE_SUPP_DCS_ODOMETER \r\n");
					}
					if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_DISTANCE_CLEAR_DTC, DCS_CURRENT_INDEX_SIZE)==0 && ODO_SUPP_GET_STATE() == ODO_SUPP_NONE)
					{
						ODO_TYPE_SET_STATE(ODO_TYPE_CARB);
					}
					if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_MAF_SENSOR, DCS_CURRENT_INDEX_SIZE)==0)
					{
						g_eMapMaf = eMAPMAF_MAF;
					}
					if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_MAP_SENSOR, DCS_CURRENT_INDEX_SIZE)==0)
					{
						if(g_eMapMaf!=eMAPMAF_MAF) g_eMapMaf = eMAPMAF_MAP;
					}
					if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_CONVERT, DCS_CURRENT_INDEX_SIZE)==0)
					{
						printf("g_bTPMSConvertFlag On\r\n");
						g_bTPMSConvertFlag = true;
					}
					if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ODOMETER, DCS_CURRENT_INDEX_SIZE)==0)
					{
						if(g_pstCurrDataBase[i].m_cCommType == 'V')	g_ucOdoGarbageTotal = 20;    // Vehicle CAN _시동 초기 data 20ea 버림
						else											g_ucOdoGarbageTotal = 10;    // Diagnosis CAN _시동 초기 data 10ea 버림
					}
					break;
				case eParse_Request_IDX:	//Request
					strcpy(g_pstCurrDataBase[i].m_cReqNode, word);
					break;
				case eParse_Response_IDX:	//Response
					strcpy(g_pstCurrDataBase[i].m_cResNode, word);
					break;
				case eParse_StartPos_IDX:	//Start Pos
					g_pstCurrDataBase[i].m_cStartPos =atoi(word);
					break;
				case eParse_RealPos_IDX:	//Real Pos
					g_pstCurrDataBase[i].m_cRealPos=atoi(word);
					break;
				case eParse_DataSize_IDX:	//Data Size
					g_pstCurrDataBase[i].m_cDataSize=atoi(word);
					break;
				case eParse_DataType_IDX:	//Data Type
					g_pstCurrDataBase[i].m_cDataType=atoi(word);
					break;
				case eParse_Unit_IDX:			//Unit
					g_pstCurrDataBase[i].m_usUnit=atoi(word);
					break;
				case eParse_ConvRule_IDX:	//ConvRule
					g_pstCurrDataBase[i].m_cCunvRule=atoi(word);
					break;
				case eParse_A_IDX:			//A
					g_pstCurrDataBase[i].m_cA=atof(word);
					break;
				case eParse_B_IDX:			//B
					g_pstCurrDataBase[i].m_cB=atof(word);
					break;
				case eParse_C_IDX:			//C
					if( g_pstCurrDataBase[i].m_cCunvRule == 5 )
					{
						memset(ucData,0x00,sizeof(ucData));
						strcpy(ucData, word);
						g_pstCurrDataBase[i].m_cC=strtol(ucData, NULL, 16);
					}
					else if( g_pstCurrDataBase[i].m_cCunvRule == 6 )
					{
						if( g_pstCurrDataBase[i].m_cDataSize != 0)
						{
                            if((*word)!=' ')
							{   for(int j=0; j<g_pstCurrDataBase[i].m_cDataSize ; j++)
                                {
                                    g_pstCurrDataBase[i].m_cC = (int)(((int)(g_pstCurrDataBase[i].m_cC)<<8) | (int)(AsciiToHex(word[(j*2)+2], word[(j*2)+3])));
                                }
                            }
                            else
                            {
                                g_pstCurrDataBase[i].m_cC=0xFF;
                            }
						}
						else
						{
							g_pstCurrDataBase[i].m_cC=0xFF;
						}
					}
					else
						g_pstCurrDataBase[i].m_cC=atof(word);
					break;
				case eParse_D_IDX:			//D
                    if( g_pstCurrDataBase[i].m_cCunvRule == 5 || g_pstCurrDataBase[i].m_cCunvRule == 6)
                    {
                      g_pstCurrDataBase[i].m_cD=atoi(word);
                    }
                    else
                    {
                        g_pstCurrDataBase[i].m_cD=AsciiToHex(word[2], word[3]);
                    }
					if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BRAKEJUDDER_VALUE, DCS_CURRENT_INDEX_SIZE)==0)
					{
						g_stBrakeJudder.bValid = true;
						g_stBrakeJudder.uiValue = g_pstCurrDataBase[i].m_cD;
					}
					break;
				case eParse_E_IDX:			//E
					g_pstCurrDataBase[i].m_cE=atoi(word);
					if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BRAKEJUDDER_VALUE, DCS_CURRENT_INDEX_SIZE)==0)
					{
						g_stBrakeJudder.ucCheckType = g_pstCurrDataBase[i].m_cE;
					}
					break;
				case eParse_F_IDX:			//F
					g_pstCurrDataBase[i].m_cF=atoi(word);
					break;
				case eParse_LUT_IDX:			//LUT
					if(g_pstCurrDataBase[i].m_cCunvRule==3)
						strcpy(g_pstCurrDataBase[i].m_cLut, word);
					else if(g_pstCurrDataBase[i].m_cCunvRule==1 || g_pstCurrDataBase[i].m_cCunvRule==2)
						memset(g_pstCurrDataBase[i].m_cLut,0x00,sizeof(g_pstCurrDataBase[i].m_cLut));
					else
						strcpy(g_pstCurrDataBase[i].m_cLut, LUT_20_Parsing(word));
					break;
				case eParse_CommType_IDX:
					g_pstCurrDataBase[i].m_cCommType= *word;
					break;
				default:
					break;
				}

				ucTokenCnt++;
#if 0 
                // DB 에서 현재 ConvRule 20 은 사용하지 않음 
				if(ucTokenCnt==15 && Data.m_cCunvRule==20) 
				{
					word=strtok_r(NULL, "\r\n",  &lastPos[1]);
					//word = strsep(&temp, "\r\n");
				}
#endif
			}
			ucTokenCnt=0;
		}
		i++;
		if( i == SLAVE_DATA_MAX )
		{
			Trace(0,"SLAVE_DATA_MAX over\r\n");
			break;
		}
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//다음 LINE
	}
	if ( bFoundStartMessage == false ) return 0;		//원하는 펑션을 찾지못하면 FAIL
	
	Trace(DBPARSING_LOG,"[%s] \r\n", __FUNCTION__);
	Trace(DBPARSING_LOG,"SLAVE Current %d \r\n" , *nCnt);
	for(i=0; i<*nCnt; i++)
	{
		Trace(DBPARSING_LOG,"%d index : %c%c%c \r\n", i+1,
					   g_pstCurrDataBase[i].m_cIndexFine[0],
					   g_pstCurrDataBase[i].m_cIndexFine[1],
					   (g_pstCurrDataBase[i].m_cIndexFine[2]=='\0') ? ' ':g_pstCurrDataBase[i].m_cIndexFine[2]);
		//Trace(DBPARSING_LOG,"\r\n");
	}
//        uint16_t nTemp;
	Trace(DBPARSING_LOG,"DEBUG_CAN_PARSING\r\n");
	for(i=0; i<*nCnt; i++)
   {
			if(g_pstCurrDataBase[i].m_cCommType == 'V'){
					  Trace(DBPARSING_LOG,"ODO Data - Vehicle CAN, %c \r\n",g_pstCurrDataBase[i].m_cCommType);
					   break;
			 }
			 else{
					  Trace(DBPARSING_LOG,"ODO Data - Diagnosis CAN, %c \r\n",g_pstCurrDataBase[i].m_cCommType);
					   break;
			 }
   }
	return 1;	//SLAVE_DATA_MAX 크기때문에 죽을 수 있다. 체크 필요
}
#endif

bool GetSlaveDTCData(U8 *cVehicleMassage, U8 *nCnt)
{
	BOOL bFoundStartMessage = false;
	char *p, *word, *temp;
	char *lastPos[2];
	char i=0;
	unsigned char ucTokenCnt=0;
	char message[]="#readDTC", endMessage[]="#readDTC_end";

	stSlaveDTCData Data[SLAVE_DTC_DATA_MAX]={0,};
	p=strtok_r((char*)cVehicleMassage, "\r\n", &lastPos[0]);

	while(p!=NULL)	//LINE이 NULL
	{
		if (strncmp(message, p, strlen(message))==0 && bFoundStartMessage == false)		//LINE 첫 문장이 message와 비교(첫 # SEARCHING)
		{
			bFoundStartMessage = true;
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp("system", p, strlen("system"))==0)	//"index_FIN(임시)" 열 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp(endMessage, p, strlen(endMessage))==0)	//LINE 첫 문장이 message와 비교 종료(첫 # SEARCHING)
		{
			break;
		}
		else if(bFoundStartMessage == false)							//시작점"#currentdata" 찾기 전까지 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}

		if ( bFoundStartMessage == true )
		{
			char cParseData[200];
			int size = sizeof(char) * (lastPos[0]- p);

			strncpy(cParseData, p,size);
			temp = cParseData;

			while(( word = strsep(&temp, ",")) != NULL)	//한LINE의 끝까지
			{
					switch( ucTokenCnt )
				{
				case eParse_DTC_ID_IDX:
					strncpy(Data[i].m_cEcuID, word, 4);
					break;
				case eParse_DTC_INDEX_IDX:			//Index Fine
					Data[i].m_cFunctiontype=atoi(word);
					break;
				case eParse_DTC_RRQ_TIME_IDX:	//Requesttime
					Data[i].m_cRequesttime =atoi(word);
					break;
				case eParse_DTC_SEARCH_TYPE_IDX://m_cSearchtype
					Data[i].m_cSearchtype=atoi(word);
					break;
				case eParse_DTC_STARTPOS_IDX:	//StartPos
					Data[i].m_cStartPos=atoi(word);
					break;
				case eParse_DTC_READ_NO_IDX:		//Readno
					Data[i].m_cReadno=atoi(word);
					break;
				case eParse_DTC_SKIP_NO_IDX:		//Skipno
					Data[i].m_cSkipno=atoi(word);
					break;
				case eParse_DTC_MASKING_IDX:		//Masking
					strcpy((char *)Data[i].m_cTotalmasking, word);
					//Data[i].m_cTotalmasking=atoi(word);
					break;
				case eParse_DTC_OPENREQ_IDX:
					strcpy(Data[i].m_cOpenReqNode, word);
					break;
				case eParse_DTC_OPENRES_IDX:
					strcpy(Data[i].m_cOpenResNode, word);
					break;
				case eParse_DTC_REQ_IDX:				//Request
					strcpy(Data[i].m_cReqNode, word);
					break;
				case eParse_DTC_RES_IDX:				//Response
					strcpy(Data[i].m_cResNode, word);
					break;
				case eParse_DTC_CLOSEREQ_IDX:
					strcpy(Data[i].m_cCloseReqNode, word);
					break;
				case eParse_DTC_CLOSERES_IDX:
					strcpy(Data[i].m_cCloseResNode, word);
					break;
				default:
					break;
				}

				ucTokenCnt++;

			}
			ucTokenCnt=0;
		}
		i++;
		if( i==SLAVE_DTC_DATA_MAX )
		{
			Trace(0,"SLAVE_DTC_DATA_MAX over\r\n");
		}
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//다음 LINE
	}
	if ( bFoundStartMessage == false ) return 0;		//원하는 펑션을 찾지못하면 FAIL

	g_pstDTCDataBase=(stSlaveDTCData *)malloc(sizeof(stSlaveDTCData)*i);
	if ( g_pstDTCDataBase != NULL)
		memcpy(g_pstDTCDataBase,Data,sizeof(stSlaveDTCData)*i);
	else
	{
		printf("%s : malloc fail so return false \r\n", __FUNCTION__);
		return 0;
	}
	*nCnt=i;	//navi 갯수
	//__iar_dlmalloc_stats();
	g_stDTCFuncData = (stDTCFunc *)malloc(sizeof(stDTCFunc)*i);		//FCS 결과저장용
	g_pDTCState = (BOOL *)malloc(sizeof(BOOL)*i);
	//__iar_dlmalloc_stats();
	memset(g_stDTCFuncData, 0x00, sizeof(stDTCFunc)*i);
	memset(g_pDTCState, 0x00, sizeof(BOOL)*i);

	if ( g_stDTCFuncData != NULL)
	{
		for(int j=0; j<i; j++)
		{
			strncpy((char *)g_stDTCFuncData[j].m_ucECU_ID, g_pstDTCDataBase[j].m_cEcuID, MAX_ECU_ID_SIZE);
			g_stDTCFuncData[j].m_usFuncType = g_pstDTCDataBase[j].m_cFunctiontype;
		}
	}
	else
	{
		printf("%s : g_stDTCFuncData malloc fail so return false \r\n", __FUNCTION__);
		return 0;
	}
#ifdef CHECK_MALLOC
printf("GetSlaveDTCData\r\n");
	__iar_dlmalloc_stats();
#endif
	Trace(DBPARSING_LOG,"[%s] \r\n", __FUNCTION__);
	Trace(DBPARSING_LOG,"SLAVE DTC %d \r\n" , *nCnt);
	for(i=0; i<*nCnt; i++)
	{
		Trace(DBPARSING_LOG,"%d REQ : %s\r\n", i+1, g_pstDTCDataBase[i].m_cReqNode);
	}
	return 1;
}

char cLutData[150]={0,};

char* LUT_20_Parsing(char lut_buff[])	//불필요 LUT 삭제
{
	char *p, *word, *lastPos[2], iTokenCnt=0;
//	char cLutData[150]={0,};				// 함수위에서 global 변수로 다시 선언했다.				2017-01-25 오후 9:05:29
	char cLutBuff[50]={0,};
	U8 ucValue=0;

	memset(cLutData,0x00,sizeof(cLutData));

	p=strtok_r(lut_buff, "$", &lastPos[0]);
	while(p!=NULL)
	{
		strcpy(cLutBuff, p);
		word=strtok_r(p, ",",  &lastPos[1]);

		while( word != NULL )
		{
			int nFieldNum=iTokenCnt%3;
			switch(nFieldNum)
			{
			case 2:		//DATA SAVE
				ucValue= *word;
				break;
			default:
				break;
			}
			iTokenCnt++;
			word=strtok_r(NULL, ",",  &lastPos[1]);
		}
		if(ucValue!=0x2d)
		{
			strcat(cLutData, "$");
			strcat(cLutData, cLutBuff);
		}

		p=strtok_r(NULL, "$", &lastPos[0]);
	}

	strcat(cLutData, "$");

	return cLutData;				// 지역변수의 번지를 반환했다. 이러면 안된다... 2017-01-25 오후 9:04:59
}

bool HardwareSetData(U8 *cMassage, char message[])
{
	BOOL bFoundStartMessage = false;
	char *p, *word, *lastPos[2], *temp;
	char cParseData[300]={0,};	//임시저장
	char i;
	unsigned char ucTokenCnt=0;
	unsigned char tempPid[8];
	unsigned int tempCanID;
	unsigned int uiStartMask[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiEndMask[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiMaskCount=0;

	memset(uiStartMask,0x00,sizeof(uiStartMask));
	memset(uiEndMask,0x00,sizeof(uiEndMask));
	memset(&g_uiDiagnosisRxCanid, 0x00, sizeof(g_uiDiagnosisRxCanid));
	memset(&g_stSlaveHWSetInfo,0x00,sizeof(g_stSlaveHWSetInfo));
	memset(&g_stCanIDCheckList,0x00,sizeof(g_stCanIDCheckList));	
	
	memset(tempPid,NULL, sizeof(tempPid)/sizeof(tempPid[0]));
	p=strtok_r((char*)cMassage, "\r\n", &lastPos[0]);

	while(p!=NULL)	//LINE이 NULL
	{
		if (strncmp(message, p, strlen(message))==0 && bFoundStartMessage == false)		//LINE 첫 문장이 message와 비교(첫 # SEARCHING)
		{
			bFoundStartMessage = true;
			continue;
		}
		if ( bFoundStartMessage == true )
		{
			strncpy(cParseData, p,sizeof(cParseData)/sizeof(cParseData[0]));
			temp = cParseData;
			while(( word = strsep(&temp, ",")) != NULL)	//한LINE의 끝까지
			{
				if( ucTokenCnt == 0 )	//진단캔은 모두 받도록 설정
				{
					if(GetPACVType() == CV)
					{
						g_stSlaveHWSetInfo.m_uiStartMask[uiMaskCount] = 0x18DA0000;
						g_stSlaveHWSetInfo.m_uiEndMask[uiMaskCount]  = 0x18DAFFFF;
					}
					else//if(ucPACVType == PA)
					{
						g_stSlaveHWSetInfo.m_uiStartMask[uiMaskCount] = 0x0700;
						g_stSlaveHWSetInfo.m_uiEndMask[uiMaskCount]  = 0x07FF;
					}
					g_stSlaveHWSetInfo.m_uiMaskCount++;
				}
				else if( ucTokenCnt == 1 )	//Protocol ID 저장
				{
					for(i=2; ;i++)
					{
						if( word[i] == NULL )	break;
						tempPid[i-2] = AsciiToHex(word[(i*2)-2],word[(i*2)-1]);
					}
				}
				else
				{
					if(GetPACVType() == CV)
					{
						tempCanID = (AsciiToHex(word[0],word[1])<<24) + (AsciiToHex(word[2],word[3])<<16) + (AsciiToHex(word[4],word[5])<<8) + (AsciiToHex(word[6],word[7]));
					}
					else//if(ucPACVType == PA)
					{
						tempCanID = ((AsciiToHex(word[0],word[1])<<8) + AsciiToHex(word[2],word[3]));
					}
					
					if((tempCanID < 0x0700) || ((tempCanID>>16) != 0))		//151027 LWH GM의 경우 077F 0780 과 같은 진단 CAN영역에서 차량에서 나옴 그래서 DB에 있는 RX CAN ID를 전부 쓰도록 변경
					{
						if(g_stSlaveHWSetInfo.m_uiMaskCount < 14)
						{
							g_stSlaveHWSetInfo.m_uiStartMask[g_stSlaveHWSetInfo.m_uiMaskCount] = g_stSlaveHWSetInfo.m_uiEndMask[g_stSlaveHWSetInfo.m_uiMaskCount]  = tempCanID;
							g_stSlaveHWSetInfo.m_uiMaskCount++;
							//경고등이 들어가면서 마스킹값이 14개가 넘어감으로 인해 마스킹은 전체 다 받고 걸러내는 로직을 사용하기위해 저장
							g_stCanIDCheckList.m_uiStartMask[g_stCanIDCheckList.m_uiMaskCount] = g_stCanIDCheckList.m_uiEndMask[g_stCanIDCheckList.m_uiMaskCount] = tempCanID;
							g_stCanIDCheckList.m_uiMaskCount++;

							g_stCanIDCanFDUse.m_uiStartMask[g_stCanIDCanFDUse.m_uiMaskCount] = g_stCanIDCanFDUse.m_uiEndMask[g_stCanIDCanFDUse.m_uiMaskCount] = tempCanID;							
							g_stCanIDCanFDUse.m_uiMaskCount++;
						}
						else
						{
//							Trace(0,"%s : MaskCnt overflow \r\n", __FUNCTION__);
							g_stSlaveHWSetInfo.m_uiStartMask[1] = 0x0000;
							g_stSlaveHWSetInfo.m_uiEndMask[1] =  0x00FF;
							g_stSlaveHWSetInfo.m_uiStartMask[2] = 0x0100;
							g_stSlaveHWSetInfo.m_uiEndMask[2] =  0x01FF;
							g_stSlaveHWSetInfo.m_uiStartMask[3] = 0x0200;
							g_stSlaveHWSetInfo.m_uiEndMask[3] =  0x02FF;
							g_stSlaveHWSetInfo.m_uiStartMask[4] = 0x0300;
							g_stSlaveHWSetInfo.m_uiEndMask[4] =  0x03FF;
							g_stSlaveHWSetInfo.m_uiStartMask[5] = 0x0400;
							g_stSlaveHWSetInfo.m_uiEndMask[5] =  0x04FF;
							g_stSlaveHWSetInfo.m_uiStartMask[6] = 0x0500;
							g_stSlaveHWSetInfo.m_uiEndMask[6] =  0x05FF;
							g_stSlaveHWSetInfo.m_uiStartMask[7] = 0x0600;
							g_stSlaveHWSetInfo.m_uiEndMask[7] =  0x06FF;
							
							g_stSlaveHWSetInfo.m_uiStartMask[8] = 0x0000;
							g_stSlaveHWSetInfo.m_uiEndMask[8] =  0x0000;
							g_stSlaveHWSetInfo.m_uiStartMask[9] = 0x0000;
							g_stSlaveHWSetInfo.m_uiEndMask[9] =  0x0000;
							g_stSlaveHWSetInfo.m_uiStartMask[10] = 0x0000;
							g_stSlaveHWSetInfo.m_uiEndMask[10] =  0x0000;
							g_stSlaveHWSetInfo.m_uiStartMask[11] = 0x0000;
							g_stSlaveHWSetInfo.m_uiEndMask[11] =  0x0000;
							g_stSlaveHWSetInfo.m_uiStartMask[12] = 0x0000;
							g_stSlaveHWSetInfo.m_uiEndMask[12] =  0x0000;
							g_stSlaveHWSetInfo.m_uiStartMask[13] = 0x0000;
							g_stSlaveHWSetInfo.m_uiEndMask[13] =  0x0000;
							
							//경고등이 들어가면서 마스킹값이 14개가 넘어감으로 인해 마스킹은 전체 다 받고 걸러내는 로직을 사용하기위해 저장
							g_stCanIDCheckList.m_uiStartMask[g_stCanIDCheckList.m_uiMaskCount] = g_stCanIDCheckList.m_uiEndMask[g_stCanIDCheckList.m_uiMaskCount] = tempCanID;
							g_stCanIDCheckList.m_uiMaskCount++;
							g_stCanIDCanFDUse.m_uiStartMask[g_stCanIDCanFDUse.m_uiMaskCount] = g_stCanIDCanFDUse.m_uiEndMask[g_stCanIDCanFDUse.m_uiMaskCount] = tempCanID;							
							g_stCanIDCanFDUse.m_uiMaskCount++;
							if( g_stCanIDCheckList.m_uiMaskCount >= CAN_ID_CHECK_LIST_MAX_CNT )
							{
//								Trace(0,"%s : g_stCanIDCheckList MaskCnt overflow \r\n", __FUNCTION__);
								printf("%s : g_stCanIDCheckList MaskCnt overflow \r\n", __FUNCTION__);
								g_stCanIDCheckList.m_uiMaskCount = CAN_ID_CHECK_LIST_MAX_CNT-1;
							}
						}
					}
				}
				ucTokenCnt++;
			}
			break;
		}
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//다음 LINE
	}

	if ( bFoundStartMessage == false )
		return 0;		//원하는 펑션을 찾지못하면 FAIL

	VCI_SetPassThruProtocolID(tempPid);

	
	if ( g_ucCanLine == HIGHCAN1 )
	{
		g_stSlaveHWSetInfo.m_ucCanCH = CAN_CHANNEL_1;
		g_stSlaveHWSetInfo.m_ucCanChip = Highcan1;
		g_stSlaveHWSetInfo.m_ucCanSpeed = GetCANBaudrate();
	}
	else if( g_ucCanLine == HIGHCAN2 )
	{
		g_stSlaveHWSetInfo.m_ucCanCH = CAN_CHANNEL_1;
		g_stSlaveHWSetInfo.m_ucCanChip = Highcan2;
		g_stSlaveHWSetInfo.m_ucCanSpeed = GetCANBaudrate();
	}
	else if( g_ucCanLine == HIGHCAN3 )
	{
		g_stSlaveHWSetInfo.m_ucCanCH = CAN_CHANNEL_2;
		g_stSlaveHWSetInfo.m_ucCanChip = Highcan3;
		g_stSlaveHWSetInfo.m_ucCanSpeed = GetCANBaudrate();
	}
	else  if( g_ucCanLine == LOWCAN1 )
	{
		g_stSlaveHWSetInfo.m_ucCanCH = CAN_CHANNEL_2;
		g_stSlaveHWSetInfo.m_ucCanChip = Lowcan1;
		g_stSlaveHWSetInfo.m_ucCanSpeed = eCAN_100KBPS;
	}
	else
	{
		g_stSlaveHWSetInfo.m_ucCanCH = CAN_CHANNEL_1;
		g_stSlaveHWSetInfo.m_ucCanChip = Highcan1;
		g_stSlaveHWSetInfo.m_ucCanSpeed = GetCANBaudrate();
	}
	
	

//	Oem_CAN_Initial_CH1(g_stSlaveHWSetInfo.m_ucCanChip, g_stSlaveHWSetInfo.m_ucCanSpeed); //init
//	Oem_CAN_Channel_Masket_Set(g_stSlaveHWSetInfo.m_ucCanCH, STANDARD_CAN, g_stSlaveHWSetInfo.m_uiMaskCount, g_stSlaveHWSetInfo.m_uiStartMask, g_stSlaveHWSetInfo.m_uiEndMask);
//	Oem_CAN_Initial_CH1(Highcan1, CAN_500KBPS); //init
//	Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN, uiMaskCount, uiStartMask, uiEndMask);

	return 1;
}

bool FunctionSetData(U8 *cMassage, char message[])
{
	BOOL bFoundStartMessage = false;
	char *p, *word, *temp;
	char *lastPos[2];
	char cParseData[100]={0,};	//임시저장
	unsigned char ucTokenCnt=0;
	char endMessage[]="#function_end";
	char ucSOHType = false;
	
	p=strtok_r((char*)cMassage, "\r\n", &lastPos[0]);

	while(p!=NULL)	//LINE이 NULL
	{
		if (strncmp(message, p, strlen(message))==0 && bFoundStartMessage == false)		//LINE 첫 문장이 message와 비교(첫 #function)
		{
			bFoundStartMessage = true;
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		else if(strncmp(endMessage, p, strlen(endMessage))==0)	//LINE 첫 문장이 message와 비교 종료( #function_end )
		{
			break;
		}
		else if(bFoundStartMessage == false)							//시작점"#function" 찾기 전까지 skip
		{
			p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}

		if ( bFoundStartMessage == true )
		{
			strncpy(cParseData, p,sizeof(cParseData)/sizeof(cParseData[0]));
			temp = cParseData;

			while(( word = strsep(&temp, ",")) != NULL)	//한LINE의 끝까지
			{
				if( ucTokenCnt == eParse_FUNCTION_SOHDIAG_IDX )
				{
					ucSOHType = atoi(word);
					if( ucSOHType == 1 )	g_bSOHTypeDBFlag = true;
					else							g_bSOHTypeDBFlag = false;
					printf("g_bSOHTypeDBFlag : %d \r\n",g_bSOHTypeDBFlag);
				}
				ucTokenCnt++;
			}
		}
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//다음 LINE
	}
	if ( bFoundStartMessage == false ) return 0;			

	return 1;
}

bool DisplaceSetData(U8 *cMassage, char message[])
{
	BOOL bFoundStartMessage = false;
	char *p, *word, *lastPos[2], *temp;
	char cParseData[100]={0,};	//임시저장
	unsigned char ucTokenCnt=0;
	char ucFuelLiterType=false;
	char ucIndicator=false;
	char ucKeyType=false;
	char ucCanFDAdapter=false;
	
	int i=0;

	p=strtok_r((char*)cMassage, "\r\n", &lastPos[0]);

	while(p!=NULL)	//LINE이 NULL
	{
		if (strncmp(message, p, strlen(message))==0 && bFoundStartMessage == false)		//LINE 첫 문장이 message와 비교(첫 # SEARCHING)
		{
			bFoundStartMessage = true;
			//p=strtok_r(NULL, "\r\n",  &lastPos[0]);
			continue;
		}
		if ( bFoundStartMessage == true )
		{
			strncpy(cParseData, p,sizeof(cParseData)/sizeof(cParseData[0]));
			temp = cParseData;
			while(( word = strsep(&temp, ",")) != NULL)	//한LINE의 끝까지
			{
				if( ucTokenCnt == eParse_DISPLACE_NONE_IDX ){}
				else if( ucTokenCnt == eParse_DISPLACE_DISPLACEMENT_IDX )								//배기량 정보
				{
					g_fDisplace = atoi(word);
				}
				else if( ucTokenCnt == eParse_DISPLACE_FUELTYPE_IDX )								//유종
				{
					if(strncmp(word, "G", 1)==0)					FUELTYPE_SET_STATE(GASOLINE);
					else if(strncmp(word, "D", 1)==0)			FUELTYPE_SET_STATE(DIESEL);
					else if(strncmp(word, "L", 1)==0)				FUELTYPE_SET_STATE(LPG);
					else if(strncmp(word, "B", 1)==0)			FUELTYPE_SET_STATE(BI_FUEL);
					else if(strncmp(word, "A", 1)==0)			FUELTYPE_SET_STATE(ELECTRONIC);
					else if(strncmp(word, "C", 1)==0)			FUELTYPE_SET_STATE(GASOLINE_HEV);
					else if(strncmp(word, "E", 1)==0)			FUELTYPE_SET_STATE(DIESEL_HEV);
					else if(strncmp(word, "F", 1)==0)				FUELTYPE_SET_STATE(PLUGIN_HEV);
					else if(strncmp(word, "H", 1)==0)			FUELTYPE_SET_STATE(FCEV);
					else if(strncmp(word, "I", 1)==0)				FUELTYPE_SET_STATE(FFV);
					else if(strncmp(word, "J", 1)==0)				FUELTYPE_SET_STATE(EV_NONE_READY);
					else														FUELTYPE_SET_STATE(ETC);
					Trace(0,"%s : %s %d \r\n", __FUNCTION__,word ,FUELTYPE_GET_STATE());
				}
				else if( ucTokenCnt == eParse_DISPLACE_CYLINDER_IDX )								//기통
				{
					g_ucCylinder = atoi(word);
				}
				else if( ucTokenCnt == eParse_DISPLACE_CGW_IDX ){}		//베이직과 동기화를 위해 추가 Premium에서는 미사용
				else if( ucTokenCnt == eParse_DISPLACE_SLEEP_IDX ){}	//베이직과 동기화를 위해 추가 Premium에서는 미사용
				else if( ucTokenCnt == eParse_DISPLACE_CANLINE_IDX )
				{
					g_ucCanLine = atoi(word);
					if( (g_ucCanLine != HIGHCAN1) && (g_ucCanLine != HIGHCAN2) && (g_ucCanLine != HIGHCAN3) && (g_ucCanLine != LOWCAN1) )
					{
						g_ucCanLine = HIGHCAN1;
					}
						g_ucSaveSlaveCanLine = g_ucCanLine;
				}
				else if( ucTokenCnt == eParse_DISPLACE_FUEL_LITER_IDX )
				{
					ucFuelLiterType = atoi(word);
					if( ucFuelLiterType == 1 )
					{
						g_bFuelLiterTypeFlag = true;
						for( i =0; i<g_usCurrentCnt ; i++)
						{
							if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FUEL_LEVEL, DCS_CURRENT_INDEX_SIZE)==0)
							{
								g_ucFuelLevelMaxLiter = g_pstCurrDataBase[i].m_cE;
							}
						}

						if( FUELTYPE_GET_STATE() == FCEV )
							g_bFuelLevelSwitchedPercentFlag = true;
					}
					else									g_bFuelLiterTypeFlag = false;
					printf("g_bFuelLiterTypeFlag : %d \r\n",g_bFuelLiterTypeFlag);
				}
				else if( ucTokenCnt == eParse_DISPLACE_INDICATOR_IDX )
				{
					ucIndicator = atoi(word);
					if( ucIndicator == 1 )	g_bIndicatorDBFlag = true;
					else							g_bIndicatorDBFlag = false;
					printf("g_bIndicatorDBFlag : %d \r\n",g_bIndicatorDBFlag);
				}
				else if( ucTokenCnt == eParse_DISPLACE_KEYTYPEDB_IDX )
				{
					ucKeyType = atoi(word);
					if( ucKeyType == 1 )	g_bKeyTypeDBFlag = true;
					else							g_bKeyTypeDBFlag = false;
					printf("g_bKeyTypeDBFlag : %d \r\n",g_bKeyTypeDBFlag);
				}
#if defined(QA_FIFA)
				else if( ucTokenCnt == eParse_DISPLACE_PACV_SEPERATOR_IDX)
				{
					if(strncmp(word, "CV", 2)==0) // 상용 
					{	
						SetPACVType(CV);
						if(strncmp(word+2,"1",1)==0)  SetCANBaudrate(eCAN_500KBPS);                     //CV1 : Baudrate 500K
						else 		                  SetCANBaudrate(eCAN_250KBPS);                     //CV2 : Baudrate 250K
					}
					else
					{
						SetPACVType(PA); // 승용 
					}
				}
#endif
				else if( ucTokenCnt == eParse_DISPLACE_FUELTANKSIZE_IDX )
				{
					if( word[0] != ' ' )
					{
						g_fFuelFcevMax = atof(word);
                        g_ucFuelLevelMaxLiter = atoi(word);
						printf("g_fFuelFcevMax : %f \r\n",g_fFuelFcevMax);
					}
					else
					{
						// default hydrogen tank size
						g_fFuelFcevMax = 6.33;
					}
				}
				else if( ucTokenCnt == eParse_DISPLACE_CANFDADAPTER_IDX )
				{
					ucCanFDAdapter = atoi(word);

					if( ucCanFDAdapter == 221 || ucCanFDAdapter == 222 || ucCanFDAdapter == 223 || 
					    ucCanFDAdapter == 224 || ucCanFDAdapter == 225)	
					{
						// CFD_SetCanFDAdapter(true);
						CFD_SetCanFDAdapter(false);
						g_ucSaveSlaveCanLine = ucCanFDAdapter;
					}
					else						
						CFD_SetCanFDAdapter(false);
					
				}
				
				ucTokenCnt++;
			}
			break;
		}
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//다음 LINE
	}
	if ( bFoundStartMessage == false ) return 0;		//원하는 펑션을 찾지못하면 FAIL

	return 1;
}

bool TpmsAlramSetData(U8 *cMassage, char message[])
{
	BOOL bFoundStartMessage = false;
	char *p, *word, *lastPos[2], *temp;
	char cParseData[500];	//임시저장
	char iTokenCnt=0;

	memset(cParseData,0x00,sizeof(cParseData));
	memcpy(cParseData,cMassage,sizeof(cParseData));	//DB앞쪽만 보고 검색 전체검색해보기에는 너무 큼;;복사안하고 검색할시에는 검색실패시 원본변형으로 인해 free실패

	p=strtok_r((char*)cMassage, "\r\n", &lastPos[0]);

	while(p!=NULL)	//LINE이 NULL
	{
		if (strncmp(message, p, strlen(message))==0 && bFoundStartMessage == false)
		{
			bFoundStartMessage = true;
			continue;
		}
		if ( bFoundStartMessage == true )
		{
			strncpy(cParseData, p,sizeof(cParseData)/sizeof(cParseData[0]));
			temp = cParseData;
			while( ( word = strsep(&temp, ",")) != NULL )	//한LINE의 끝까지
			{
				if( iTokenCnt == eParse_TPMSALRAM_NONE_IDX ){}
				else if( iTokenCnt == eParse_TPMSALRAM_VALUE_IDX )
				{
					g_uiTpmsAlramValueSave = g_uiTpmsAlramValue = atoi(word);
				}
				iTokenCnt++;
			}
			break;
		}
		p=strtok_r(NULL, "\r\n",  &lastPos[0]);	//다음 LINE
	}
	if ( bFoundStartMessage == false ) return 0;		//원하는 펑션을 찾지못하면 FAIL

	return 1;
}

stReferenceTable* SetReferenceTable(stMasterData *NaviData, stSlaveData *CurrData,U16 nReqCnt, U16 nCurrCnt, U16 *nCnt)
{
	int nTCnt=0, nMastercnt=0, repeatcnt=0, nMaterSubCnt=0;
	int k=0;
	char cTmp[MAX_REF_INDEX_SIZE]={0,};
	char *p=NULL;
	BOOL bCompare=true;
	stReferenceTable Table[MAX_REF_TABLE_SIZE]={0,};
	stMasterData *pTmpNaviData;
	unsigned char arrTmp[sizeof(stMasterData)+1];
	memset(arrTmp, 0x00, sizeof(arrTmp)/sizeof(arrTmp[0]));

	for(nMastercnt=0; nMastercnt<nReqCnt; nMastercnt++)
	{
		memcpy(arrTmp, &NaviData[nMaterSubCnt], sizeof(stMasterData));
		pTmpNaviData = (stMasterData*)arrTmp;
		memset(&cTmp,0x00,sizeof(cTmp));
		strncpy(cTmp, pTmpNaviData->m_cRef,sizeof(pTmpNaviData->m_cRef));
		p=strtok(cTmp, ".");

		while(p !=NULL )
		{
			bCompare=true;
			for(repeatcnt=0; repeatcnt<nMastercnt; repeatcnt++)
			{
				if(strncmp(p, Table[repeatcnt].m_cIndexFine, sizeof(Table[repeatcnt].m_cIndexFine)/sizeof(Table[repeatcnt].m_cIndexFine[0]))==0)	//중복되는게 있으면 중단
				{
					Table[repeatcnt].m_cNaviCounter++;
					Table[repeatcnt].m_cNaviNumber[Table[repeatcnt].m_cNaviCounter]=nMaterSubCnt;
					bCompare=false;
					break;
				}
			}

			if(bCompare!=false)	//2개이상의 index포함의경우 앞에 중복일때 다음줄 파싱하는 오류 수정
			{
				if(nMastercnt==repeatcnt)	//중복 없을경우
				{
					for(k=0; k<nCurrCnt; k++)
					{
						if(strncmp(p, CurrData[k].m_cIndexFine, sizeof(CurrData[k].m_cIndexFine)/sizeof(CurrData[k].m_cIndexFine[0]))==0)	        //Fine index로 DB검색
						{
							strncpy(Table[nTCnt].m_cIndexFine, p, sizeof(Table[nTCnt].m_cIndexFine)/sizeof(Table[nTCnt].m_cIndexFine[0]));	        //index
							Table[nTCnt].m_cCurrNumber=k;			//index의 current DB상의 위치
							Table[nTCnt].m_cNaviNumber[0]=nMaterSubCnt;	//index의 Navi DB상의 위치
							nTCnt++;
							break;
						}
					}
				}
			}
			p=strtok(NULL, ".");
			if(p != NULL)
			{
				nReqCnt++;	//req cnt
				nMastercnt++;	//table 추가 cnt
			}
		}
		nMaterSubCnt++;	//line
	}
	if(nTCnt!=0)
	{	//__iar_dlmalloc_stats();
		stReferenceTable *RefTb=(stReferenceTable *)malloc(sizeof(stReferenceTable)*nTCnt);
		if ( RefTb != NULL)
			memcpy(RefTb, Table, sizeof(stReferenceTable)*nTCnt);
		else
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}
		*nCnt=nTCnt;	//Table 갯수
		return RefTb;
	}
	*nCnt=nTCnt;	//Table 갯수


	Trace(DBPARSING_LOG,"=============SetReferenceTable============\r\n");
	Trace(DBPARSING_LOG,"Reference Table %d \r\n",  *nCnt);
	for(int i=0; i<*nCnt; i++)
	{
		//Trace(DBPARSING_LOG,"%d번째 index : %s CurrDB : %d NaviDB : %d\r\n",i+1, RefTb[i].m_cIndexFine, RefTb[i].m_cCurrNumber, RefTb[i].m_cNaviNumber);
	}
	return 0;
}

int SetRequest(stReferenceTable *Table, int nTableCnt, eFUNCTION_COMM_STATE eFunction)
{
	char i=0,j=0, k=0, nSize;
	char reqCnt=0, nReqSize=0;
	BYTE hexcode;

	//BYTE DTCcode[2][10] = {{0x07, 0xDF, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},{0x07, 0xDF, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

	nSize = (sizeof(g_pstCurrDataBase[Table[0].m_cCurrNumber].m_cReqNode)/sizeof(g_pstCurrDataBase[Table[0].m_cCurrNumber].m_cReqNode[0]))/2;
	nSize=10;
	switch(eFunction)
	{
	case eCOMM_DTC_FUNCTION:
		{
		g_pDTCRequest=(U8 *)malloc((nTableCnt * 3)*10);		//*3 (open, read, close)
		if(g_pDTCRequest != NULL)
			memset(&(*g_pDTCRequest), 0x00, (nTableCnt * 3)*10);
		else
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}

		U8 ucOpenLength = sizeof(g_pstDTCDataBase[i].m_cOpenReqNode)/sizeof(g_pstDTCDataBase[i].m_cOpenReqNode[0]);
		U8 ucDtcLength = sizeof(g_pstDTCDataBase[i].m_cReqNode)/sizeof(g_pstDTCDataBase[i].m_cReqNode[0]);
		U8 ucCloseLength = sizeof(g_pstDTCDataBase[i].m_cCloseReqNode)/sizeof(g_pstDTCDataBase[i].m_cCloseReqNode[0]);
		for(i=0; i<nTableCnt; i++)
		{

			for(k=0; k<ucOpenLength/2; k++)
			{
				g_pDTCRequest[(i * 3 * nSize) + k]=AsciiToHex(g_pstDTCDataBase[i].m_cOpenReqNode[k*2], g_pstDTCDataBase[i].m_cOpenReqNode[k*2+1]);
			}

			//DTC Read
			for(k=0; k<ucDtcLength/2; k++)
			{
				g_pDTCRequest[((i * 3  + 1) * nSize) + k]=AsciiToHex(g_pstDTCDataBase[i].m_cReqNode[k*2], g_pstDTCDataBase[i].m_cReqNode[k*2+1]);
			}

			//close
			for(k=0; k<ucCloseLength/2; k++)
			{
				g_pDTCRequest[((i * 3 + 2) * nSize) + k]=AsciiToHex(g_pstDTCDataBase[i].m_cCloseReqNode[k*2], g_pstDTCDataBase[i].m_cCloseReqNode[k*2+1]);
			}
			/*
			for(j=0; j<nSize; j++)
			{
			g_pDTCRequest[(i*nSize)+j]=AsciiToHex(g_pstDTCDataBase[nTableCnt-1].m_cReqNode[j*2], g_pstDTCDataBase[nTableCnt-1].m_cReqNode[j*2+1]);
		}
			*/

		}

		reqCnt=nTableCnt * 3;
		}
		break;
//	case eCOMM_SUPP_FUNCTION:
//		g_pSuppRequest=(U8 *)malloc(nTableCnt*nSize);
//
//		if(g_pSuppRequest == NULL)
//		{
//			Trace(0,"%s : malloc fail so return false \r\n", __FUNCTION__);
//			return 0;
//		}
//
//		nReqSize=strlen(g_pstCurrDataBase[g_ucSuppTableNum].m_cReqNode)/2;
//
//		for(j=0; j<nSize; j++)
//		{
//			hexcode=AsciiToHex(g_pstCurrDataBase[g_ucSuppTableNum].m_cReqNode[j*2], g_pstCurrDataBase[g_ucSuppTableNum].m_cReqNode[j*2+1]);
//			g_pSuppRequest[j]=hexcode;
//		}
//		reqCnt++;
//
//		break;
	case eCOMM_FAST_FUNCTION:
		g_pFastRequest=(U8 *)malloc(nTableCnt*nSize);
		if(g_pFastRequest == NULL)
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}

		for(i=0; i<nTableCnt; i++)	//참조테이블 만큼
		{
			nReqSize=strlen(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode)/2;
			for(k=0; k<reqCnt; k++)	//중복검사
			{
				if(nReqSize==5)
				{
					if(g_pFastRequest[k*nSize]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[0], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[1]) &&
					   g_pFastRequest[k*nSize+1]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[3]) &&
						   g_pFastRequest[k*nSize+2]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[4], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[5]) &&
							   g_pFastRequest[k*nSize+3]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[6], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[7]) &&
								   g_pFastRequest[k*nSize+4]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[8], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[9]))
						break;
				}
				else if(nReqSize==6)
				{
					if(g_pFastRequest[k*nSize]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[0], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[1]) &&
					   g_pFastRequest[k*nSize+1]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[3]) &&
						   g_pFastRequest[k*nSize+2]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[4], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[5]) &&
							   g_pFastRequest[k*nSize+3]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[6], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[7]) &&
								   g_pFastRequest[k*nSize+4]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[8], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[9]) &&
									   g_pFastRequest[k*nSize+5]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[10], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[11]))
						break;
				}
			}
			if(reqCnt==k)	//중복 없을경우
			{
				for(j=0; j<nSize; j++)
				{
					hexcode=AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[j*2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[j*2+1]);
					g_pFastRequest[(reqCnt*nSize)+j]=hexcode;
				}
				reqCnt++;
			}
		}
		break;
	case eCOMM_SLOW1_FUNCTION:
		g_pSlow1Request=(U8 *)malloc(nTableCnt*nSize);
		if(g_pSlow1Request == NULL)
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}

		for(i=0; i<nTableCnt; i++)	//참조테이블 만큼
		{
			nReqSize=strlen(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode) / 2;
			for(k=0; k<reqCnt; k++)	//중복검사
			{
				if(nReqSize==5)
				{
					if(g_pSlow1Request[k*nSize]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[0], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[1]) &&
					   g_pSlow1Request[k*nSize+1]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[3]) &&
						   g_pSlow1Request[k*nSize+2]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[4], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[5]) &&
							   g_pSlow1Request[k*nSize+3]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[6], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[7]) &&
								   g_pSlow1Request[k*nSize+4]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[8], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[9]))
						break;
				}
				else if(nReqSize==6)
				{
					if(g_pSlow1Request[k*nSize]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[0], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[1]) &&
					   g_pSlow1Request[k*nSize+1]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[3]) &&
						   g_pSlow1Request[k*nSize+2]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[4], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[5]) &&
							   g_pSlow1Request[k*nSize+3]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[6], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[7]) &&
								   g_pSlow1Request[k*nSize+4]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[8], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[9]) &&
									   g_pSlow1Request[k*nSize+5]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[10], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[11]))
						break;
				}
			}
			if(reqCnt==k)	//중복 없을경우
			{
				for(j=0; j<nSize; j++)
				{
					hexcode=AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[j*2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[j*2+1]);
					g_pSlow1Request[(reqCnt*nSize)+j]=hexcode;
				}
				reqCnt++;
			}
		}
		break;
	case eCOMM_SLOW2_FUNCTION:
		g_pSlow2Request=(U8 *)malloc(nTableCnt*nSize);
		if(g_pSlow2Request == NULL)
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}

		for(i=0; i<nTableCnt; i++)	//참조테이블 만큼
		{
			nReqSize=strlen(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode) / 2;
			for(k=0; k<reqCnt; k++)	//중복검사
			{
				if(nReqSize==5)
				{
					if(g_pSlow2Request[k*nSize]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[0], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[1]) &&
					   g_pSlow2Request[k*nSize+1]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[3]) &&
						   g_pSlow2Request[k*nSize+2]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[4], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[5]) &&
							   g_pSlow2Request[k*nSize+3]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[6], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[7]) &&
								   g_pSlow2Request[k*nSize+4]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[8], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[9]))
						break;
				}
				else if(nReqSize==6)
				{
					if(g_pSlow2Request[k*nSize]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[0], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[1]) &&
					   g_pSlow2Request[k*nSize+1]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[3]) &&
						   g_pSlow2Request[k*nSize+2]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[4], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[5]) &&
							   g_pSlow2Request[k*nSize+3]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[6], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[7]) &&
								   g_pSlow2Request[k*nSize+4]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[8], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[9]) &&
									   g_pSlow2Request[k*nSize+5]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[10], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[11]))
						break;
				}
			}
			if(reqCnt==k)	//중복 없을경우
			{
				for(j=0; j<nSize; j++)
				{
					hexcode=AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[j*2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[j*2+1]);
					g_pSlow2Request[(reqCnt*nSize)+j]=hexcode;
				}
				reqCnt++;
			}
		}
		break;
	case eCOMM_SLOW3_FUNCTION:
		g_pSlow3Request=(U8 *)malloc(nTableCnt*nSize);
		if(g_pSlow3Request == NULL)
		{
			printf("%s : malloc fail so return false \r\n", __FUNCTION__);
			return 0;
		}

		for(i=0; i<nTableCnt; i++)	//참조테이블 만큼
		{
			nReqSize=strlen(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode) / 2;
			for(k=0; k<reqCnt; k++)	//중복검사
			{
				if(nReqSize==5)
				{
					if(g_pSlow3Request[k*nSize]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[0], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[1]) &&
					   g_pSlow3Request[k*nSize+1]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[3]) &&
						   g_pSlow3Request[k*nSize+2]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[4], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[5]) &&
							   g_pSlow3Request[k*nSize+3]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[6], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[7]) &&
								   g_pSlow3Request[k*nSize+4]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[8], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[9]))
						break;
				}
				else if(nReqSize==6)
				{
					if(g_pSlow3Request[k*nSize]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[0], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[1]) &&
					   g_pSlow3Request[k*nSize+1]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[3]) &&
						   g_pSlow3Request[k*nSize+2]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[4], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[5]) &&
							   g_pSlow3Request[k*nSize+3]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[6], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[7]) &&
								   g_pSlow3Request[k*nSize+4]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[8], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[9]) &&
									   g_pSlow3Request[k*nSize+5]==AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[10], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[11]))
						break;
				}
			}
			if(reqCnt==k)	//중복 없을경우
			{
				for(j=0; j<nSize; j++)
				{
					hexcode=AsciiToHex(g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[j*2], g_pstCurrDataBase[Table[i].m_cCurrNumber].m_cReqNode[j*2+1]);
					g_pSlow3Request[(reqCnt*nSize)+j]=hexcode;
				}
				reqCnt++;
			}
		}
		break;
	}
///////////////////////////////////////////////////////////////////////////////////여기부터
	Trace(DBPARSING_LOG,"[%s] \r\n", __FUNCTION__);
	switch(eFunction)
	{
	case eCOMM_DTC_FUNCTION:
		Trace(DBPARSING_LOG,"DTC\r\n");
		break;
	case eCOMM_FAST_FUNCTION:
		Trace(DBPARSING_LOG,"FAST\r\n");
		break;
	case eCOMM_SLOW1_FUNCTION:
		Trace(DBPARSING_LOG,"SLOW1\r\n");
		break;
	case eCOMM_SLOW2_FUNCTION:
		Trace(DBPARSING_LOG,"SLOW2\r\n");
		break;
	case eCOMM_SLOW3_FUNCTION:
		Trace(DBPARSING_LOG,"SLOW3\r\n");
		break;
	}
	for(i=0; i<reqCnt; i++)
	{
		for(j=0; j<nSize; j++)
		{
			switch(eFunction)
			{
			case eCOMM_DTC_FUNCTION:
				Trace(DBPARSING_LOG,"%02x ", g_pDTCRequest[(i*nSize)+j]);
				break;
			case eCOMM_FAST_FUNCTION:
				Trace(DBPARSING_LOG,"%02x ", g_pFastRequest[(i*nSize)+j]);
				break;
			case eCOMM_SLOW1_FUNCTION:
				Trace(DBPARSING_LOG,"%02x ", g_pSlow1Request[(i*nSize)+j]);
				break;
			case eCOMM_SLOW2_FUNCTION:
				Trace(DBPARSING_LOG,"%02x ", g_pSlow2Request[(i*nSize)+j]);
				break;
			case eCOMM_SLOW3_FUNCTION:
				Trace(DBPARSING_LOG,"%02x ", g_pSlow3Request[(i*nSize)+j]);
				break;
			}
		}
		Trace(DBPARSING_LOG,"\r\n");
	}
#ifdef CHECK_MALLOC
	printf("SetRequest\r\n");
	__iar_dlmalloc_stats();
#endif
///////////////////////////////////////////////////////////////////////////////////여기까지
	return reqCnt;
}

/*  -- GetSavedOBDdata() 요구사항
	SAVE_OBDATA_INFO 파일이 없으면 만든다 -> 전체 데이터 초기화
	비클코드 비교해서 다르면 주행 데이터 초기화
	  							같으면 총 데이터 읽기
	현재 시간의 사용할 ID와 저장된 ID를 비교해서 다르면 예약 데이터 초기화, 저장 ID 변경
																		같으면  정보 읽기
					ucGetStatus가 1 경우 다음 시간 정보로 취득 <- 두 사용자의 시간대가 겹칠 경우
*/
void GetSavedOBDdata(U8 ucGetStatus)	// 0: boot&wakeup 1: UserChange
{
//  int iLength,i;
//	unsigned char arrRealTime[15];
//	U8 arrStartTime[RESERVED_TIMEINFO_SIZE + 1];
//	U8 arrRealTimeConvert[6];
//	U8 arrStartTimeConvert[6];
//
//	Trace(0,"[%s- %d]", __FUNCTION__, ucGetStatus);
//	f_chdir("/INFO");
//
//	if( GetVCI2FileRead(SAVE_OBDATA_INFO, (BYTE*)&g_stSaveOBDdata, (UINT*)&iLength)==FALSE)	//파일 미존재시 생성
//	{
//	  	memset((void*)&g_stSaveOBDdata, 0x00, sizeof(stSaveOBDInfo));
//		memcpy(g_stSaveOBDdata.m_strVehicleCode, g_FirmwareInfo.m_strVehicleCode,  sizeof(g_FirmwareInfo.m_strVehicleCode));
//		GetVCI2FileWrite(SAVE_OBDATA_INFO, (U8*)&g_stSaveOBDdata, sizeof(stSaveOBDInfo));
//
//		Trace(0,"[No SAVE_OBDATA_INFO]\r\n");
//	}
//
//	if(strcmp((char *)g_stSaveOBDdata.m_strVehicleCode,(const char *)g_FirmwareInfo.m_strVehicleCode)!=0)
//	{
//		memset((void*)&g_stSaveOBDdata, 0x00, sizeof(stSaveOBDInfo));
//		memcpy(g_stSaveOBDdata.m_strVehicleCode, g_FirmwareInfo.m_strVehicleCode,  sizeof(g_FirmwareInfo.m_strVehicleCode));
//		GetVCI2FileWrite(SAVE_OBDATA_INFO, (U8*)&g_stSaveOBDdata, sizeof(stSaveOBDInfo));
//
//		Trace(0,"[Not Match VehicleCode]\r\n");
//	}
//
//	////////총 주행거리 총 연료소모량 읽기
//	g_stFastFuncData.m_nFuel_vehicle = g_stSaveOBDdata.m_fFuel_vehicle;
//	g_stFastFuncData.m_nOdometer_t	= g_stSaveOBDdata.m_nOdometer_t;
//	////////
//
//	VCI_GetRtcTime(arrRealTime);
//	memcpy(arrRealTimeConvert,&arrRealTime[1],sizeof(arrRealTimeConvert));
//	arrRealTimeConvert[5]=0x00;
//
//	if( GetVCI2FileRead(FILENAME_RESERV_INFO, &g_stReservInfo[0].arrIndex[0], (UINT*)&iLength)==TRUE)
//	{
//		for(i=0; i< RESERVED_INDEX_MAX; i++)
//		{
//			memcpy(&arrStartTime, &g_stReservInfo[i].ucStartTime[2], sizeof(arrStartTime)-2);
//			arrStartTime[7] = arrStartTime[7] -1; 	//대여시간 1시간전부터 차량도어 제어 가능
//			fnHex2Str((char *)&arrStartTimeConvert[0],(char *)&arrStartTime[0]);
//			for(int j=0;j<5;j++)
//			{
//				arrStartTimeConvert[j]=HexToDec(arrStartTimeConvert[j]);
//			}
//			arrStartTimeConvert[5]=0x00;
//
//			if(strcmp((char *)arrRealTimeConvert,(const char *)arrStartTimeConvert)>0)
//			{
//				Trace(0," RealTime > StartTime \r\n");
//
//				g_stFastFuncData.m_nFuel_r 		= g_stSaveOBDdata.m_fFuel_rev ;
//				g_stFastFuncData.m_nOdometer_r	= g_stSaveOBDdata.m_nOdometer_rev;
//				if(strcmp((char const*)g_stSaveOBDdata.arrUserID, (char const*)g_stReservInfo[i].arrUserID) == 0)
//				{
//					if(ucGetStatus == 0) break;
//				}
//				else
//				{
//					memcpy(g_stSaveOBDdata.arrUserID,  g_stReservInfo[i].arrUserID, USER_ID_LENGTH);
//				}
//			}
//		}
//	}
//	f_chdir(DIR_ROOT);
}

void SetSavedOBDdata()
{
//	Trace(0,"[%s]run\r\n", __FUNCTION__);
//	f_chdir("/INFO");
//
//	g_stSaveOBDdata.m_fFuel_vehicle = g_stFastFuncData.m_nFuel_vehicle;
//	g_stSaveOBDdata.m_nOdometer_t	= g_stFastFuncData.m_nOdometer_t;		// use		// clu odo없을땐 개발 필요
//	g_stSaveOBDdata.m_fFuel_rev 		= g_stFastFuncData.m_nFuel_r;
//	g_stSaveOBDdata.m_nOdometer_rev	= g_stFastFuncData.m_nOdometer_r;
//	memcpy(g_stSaveOBDdata.m_strVehicleCode, g_FirmwareInfo.m_strVehicleCode, sizeof(g_FirmwareInfo.m_strVehicleCode));
//
//	GetVCI2FileWrite(SAVE_OBDATA_INFO, (U8*)&g_stSaveOBDdata, sizeof(stSaveOBDInfo));
//	f_chdir(DIR_ROOT);
}

BOOL CurrentCalculate_V(int CurrNum, int *data, U8 *Response)
{
	int arry=1;
	U8 ucCommData[5];     //DATA저장

//if( strncmp(g_stActuatorReadyData[CurrNum ].m_cIndex, DCS_RPM, DCS_CURRENT_INDEX_SIZE) == 0 )	//E02
//{
//	Trace(0,"%02X %02X %02X %02X %02X %02X %02X %02X\r\n",Response[0],Response[1],Response[2],Response[3],Response[4],Response[5],Response[6],Response[7]);
//	Trace(0,"%02X %02X %d %d\r\n",g_stActuatorReadyData[ CurrNum ].m_ucStartPos,g_stActuatorReadyData[ CurrNum ].m_ucRealPos,arry,g_stActuatorReadyData[ CurrNum ].m_ucMsbLsb);
//	Trace(0,"%02X %02X\r\n",Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos - 1 - arry      ],Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos - 1 - arry   +1   ]);
//}
	if(g_stActuatorReadyData[ CurrNum ].m_ucDataSize==4)		//4BYTE
	{
		ucCommData[0]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos -1 - arry      ];
		ucCommData[1]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos -1 - arry + 1 ];
		ucCommData[2]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos -1 - arry + 2 ];
		ucCommData[3]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos -1 - arry + 3 ];

		if(g_stActuatorReadyData[ CurrNum ].m_ucMsbLsb==1)	//MSB
		{
			*data=(((unsigned int)ucCommData[0]*256*256*256) +
				((unsigned int)ucCommData[1]*256*256) +
				((unsigned int)ucCommData[2]*256) +
				((unsigned int)ucCommData[3]));
		}
		else											//LSB
		{
			*data=(((unsigned int)ucCommData[3]*256*256*256) +
				((unsigned int)ucCommData[2]*256*256) +
				((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[0]));
		}
	}
	else if(g_stActuatorReadyData[ CurrNum ].m_ucDataSize==3)	//3BYTE
	{

		ucCommData[0]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos -1 - arry     ];
		ucCommData[1]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos -1 - arry +1 ];
		ucCommData[2]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos -1 - arry +2 ];

		if(g_stActuatorReadyData[ CurrNum ].m_ucMsbLsb==1)	//MSB
		{
			*data=(((unsigned int)ucCommData[0]*256*256) +
				((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[2]));
		}
		else											//LSB
		{
			*data=(((unsigned int)ucCommData[2]*256*256) +
				((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[0]));
		}
	}
	else if(g_stActuatorReadyData[ CurrNum ].m_ucDataSize==2)	//2BYTE
	{

		ucCommData[0]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos - 1 - arry      ];
		ucCommData[1]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos - 1 - arry + 1 ];

		if(g_stActuatorReadyData[ CurrNum ].m_ucMsbLsb==1)	//MSB
		{
			*data=(((unsigned int)ucCommData[0]*256) +
				((unsigned int)ucCommData[1]));
		}
		else											//LSB
		{
			*data=(((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[0]));
		}
	}
	else
	{
		ucCommData[0]=Response[ g_stActuatorReadyData[ CurrNum ].m_ucStartPos + g_stActuatorReadyData[ CurrNum ].m_ucRealPos - 1 - arry ];	//응답코드[startPos + realPos - 1(L) + arry(배열특성0base)]
		ucCommData[1]=0x00;

		*data=(unsigned int)ucCommData[0];
	}

	return true;
}

BOOL CurrentCalculate_V2(int CurrNum, int *data, U8 *Response)
{
	int arry=1;
	U8 ucCommData[5];     //DATA저장

//if( strncmp(g_stActuatorResData[CurrNum ].m_cIndex, DCS_RPM, DCS_CURRENT_INDEX_SIZE) == 0 )	//E02
//{
//	Trace(0,"%02X %02X %02X %02X %02X %02X %02X %02X\r\n",Response[0],Response[1],Response[2],Response[3],Response[4],Response[5],Response[6],Response[7]);
//	Trace(0,"%02X %02X %d %d\r\n",g_stActuatorResData[ CurrNum ].m_ucStartPos,g_stActuatorResData[ CurrNum ].m_ucRealPos,arry,g_stActuatorResData[ CurrNum ].m_ucMsbLsb);
//	Trace(0,"%02X %02X\r\n",Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos - 1 - arry      ],Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos - 1 - arry   +1   ]);
//}
	if(g_stActuatorResData[ CurrNum ].m_ucDataSize==4)		//4BYTE
	{
		ucCommData[0]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos -1 - arry      ];
		ucCommData[1]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos -1 - arry + 1 ];
		ucCommData[2]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos -1 - arry + 2 ];
		ucCommData[3]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos -1 - arry + 3 ];

		if(g_stActuatorResData[ CurrNum ].m_ucMsbLsb==1)	//MSB
		{
			*data=(((unsigned int)ucCommData[0]*256*256*256) +
				((unsigned int)ucCommData[1]*256*256) +
				((unsigned int)ucCommData[2]*256) +
				((unsigned int)ucCommData[3]));
		}
		else											//LSB
		{
			*data=(((unsigned int)ucCommData[3]*256*256*256) +
				((unsigned int)ucCommData[2]*256*256) +
				((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[0]));
		}
	}
	else if(g_stActuatorResData[ CurrNum ].m_ucDataSize==3)	//3BYTE
	{

		ucCommData[0]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos -1 - arry     ];
		ucCommData[1]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos -1 - arry +1 ];
		ucCommData[2]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos -1 - arry +2 ];

		if(g_stActuatorResData[ CurrNum ].m_ucMsbLsb==1)	//MSB
		{
			*data=(((unsigned int)ucCommData[0]*256*256) +
				((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[2]));
		}
		else											//LSB
		{
			*data=(((unsigned int)ucCommData[2]*256*256) +
				((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[0]));
		}
	}
	else if(g_stActuatorResData[ CurrNum ].m_ucDataSize==2)	//2BYTE
	{

		ucCommData[0]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos - 1 - arry      ];
		ucCommData[1]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos - 1 - arry + 1 ];

		if(g_stActuatorResData[ CurrNum ].m_ucMsbLsb==1)	//MSB
		{
			*data=(((unsigned int)ucCommData[0]*256) +
				((unsigned int)ucCommData[1]));
		}
		else											//LSB
		{
			*data=(((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[0]));
		}
	}
	else
	{
		ucCommData[0]=Response[ g_stActuatorResData[ CurrNum ].m_ucStartPos + g_stActuatorResData[ CurrNum ].m_ucRealPos - 1 - arry ];	//응답코드[startPos + realPos - 1(L) + arry(배열특성0base)]
		ucCommData[1]=0x00;

		*data=(unsigned int)ucCommData[0];
	}

	return true;
}

BOOL CurrentCalculate(int CurrNum, int *data, U8 *Response)
{
	int arry=1;
	U8 ucCommData[5];     //DATA저장
	
	for( int i=0; i<5; i++ )
	{
		ucCommData[i] = 0;
	}

	if(g_pstCurrDataBase[CurrNum].m_cCommType == 'D')
	{
		if( (Response[0]!=AsciiToHex(g_pstCurrDataBase[CurrNum].m_cResNode[0], g_pstCurrDataBase[CurrNum].m_cResNode[1])) ||
			(Response[1]!=AsciiToHex(g_pstCurrDataBase[CurrNum].m_cResNode[2], g_pstCurrDataBase[CurrNum].m_cResNode[3])) ||
			(Response[2]!=AsciiToHex(g_pstCurrDataBase[CurrNum].m_cResNode[4], g_pstCurrDataBase[CurrNum].m_cResNode[5])) ||
			(Response[3]!=AsciiToHex(g_pstCurrDataBase[CurrNum].m_cResNode[6], g_pstCurrDataBase[CurrNum].m_cResNode[7])) )
		{	//원하는 응답아닐경우
			return false;
		}
	}
	else
	{
		if( (Response[0]!=AsciiToHex(g_pstCurrDataBase[CurrNum].m_cResNode[0], g_pstCurrDataBase[CurrNum].m_cResNode[1])) ||
			(Response[1]!=AsciiToHex(g_pstCurrDataBase[CurrNum].m_cResNode[2], g_pstCurrDataBase[CurrNum].m_cResNode[3])) )
		{	//원하는 응답아닐경우
			return false;
		}
	}

	if(g_pstCurrDataBase[ CurrNum ].m_cDataSize==4)		//4BYTE
	{
		ucCommData[0]=Response[ g_pstCurrDataBase[CurrNum].m_cStartPos + g_pstCurrDataBase[CurrNum].m_cRealPos -1 - arry     ];
		ucCommData[1]=Response[ g_pstCurrDataBase[CurrNum].m_cStartPos + g_pstCurrDataBase[CurrNum].m_cRealPos -1 - arry + 1 ];
		ucCommData[2]=Response[ g_pstCurrDataBase[CurrNum].m_cStartPos + g_pstCurrDataBase[CurrNum].m_cRealPos -1 - arry + 2 ];
		ucCommData[3]=Response[ g_pstCurrDataBase[CurrNum].m_cStartPos + g_pstCurrDataBase[CurrNum].m_cRealPos -1 - arry + 3 ];

		if(g_pstCurrDataBase[ CurrNum ].m_cDataType==1)	//MSB
		{
			*data=(((unsigned int)ucCommData[0]*256*256*256) +
				((unsigned int)ucCommData[1]*256*256) +
				((unsigned int)ucCommData[2]*256) +
				((unsigned int)ucCommData[3]));
		}
		else											//LSB
		{
			*data=(((unsigned int)ucCommData[3]*256*256*256) +
				((unsigned int)ucCommData[2]*256*256) +
				((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[0]));
		}
	}
	else if(g_pstCurrDataBase[ CurrNum ].m_cDataSize==3)	//3BYTE
	{

		ucCommData[0]=Response[ g_pstCurrDataBase[ CurrNum ].m_cStartPos + g_pstCurrDataBase[ CurrNum ].m_cRealPos -1 - arry    ];
		ucCommData[1]=Response[ g_pstCurrDataBase[ CurrNum ].m_cStartPos + g_pstCurrDataBase[ CurrNum ].m_cRealPos -1 - arry +1 ];
		ucCommData[2]=Response[ g_pstCurrDataBase[ CurrNum ].m_cStartPos + g_pstCurrDataBase[ CurrNum ].m_cRealPos -1 - arry +2 ];

		if(g_pstCurrDataBase[ CurrNum ].m_cDataType==1)	//MSB
		{
			*data=(((unsigned int)ucCommData[0]*256*256) +
				((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[2]));
		}
		else											//LSB
		{
			*data=(((unsigned int)ucCommData[2]*256*256) +
				((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[0]));
		}
	}
	else if(g_pstCurrDataBase[ CurrNum ].m_cDataSize==2)	//2BYTE
	{

		ucCommData[0]=Response[ g_pstCurrDataBase[ CurrNum ].m_cStartPos + g_pstCurrDataBase[ CurrNum ].m_cRealPos - 1 - arry     ];
		ucCommData[1]=Response[ g_pstCurrDataBase[ CurrNum ].m_cStartPos + g_pstCurrDataBase[ CurrNum ].m_cRealPos - 1 - arry + 1 ];

		if(g_pstCurrDataBase[ CurrNum ].m_cDataType==1)	//MSB
		{
			*data=(((unsigned int)ucCommData[0]*256) +
				((unsigned int)ucCommData[1]));
		}
		else											//LSB
		{
			*data=(((unsigned int)ucCommData[1]*256) +
				((unsigned int)ucCommData[0]));
		}
	}
	else
	{
		ucCommData[0]=Response[ g_pstCurrDataBase[ CurrNum ].m_cStartPos + g_pstCurrDataBase[ CurrNum ].m_cRealPos - 1 - arry ];	//응답코드[startPos + realPos - 1(L) + arry(배열특성0base)]
		ucCommData[1]=0x00;

		*data=(unsigned int)ucCommData[0];
	}

	return true;
}

int8_t Compute_Data_V(stActuatorReadyData *pCurrDataBase, int data)
{
	float float_data=0.0, flo1=0.0, flo2=0.0;
	unsigned int int_data=0, len_pos=0, lut_no=0, order=0, sintmask=0, iTemp=0;
	char lut_buff[150]={0,};
	U8 mask=0;
	U8 ucRShift =0, ucLShift = 0;

	// convrule.F이 127이면 수신값을 0x7F와 Masking한다.
	if(pCurrDataBase->m_cF==127)
	{
		data &= 0x7F;
	}
	else if(pCurrDataBase->m_cF==251)
	{
		if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0xFEFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==252)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0xFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0xFFFF || data == 0x7FFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==253 || pCurrDataBase->m_cF==255)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0xFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if((data == 0xFFFF)||(data == 0xFEFF))		//FEFF는 오토링크에 존재
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 4)
		{
			if(data == 0xFFFFFFFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==254)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0xFF || data == 0x00)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return -1;		// 초기값
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0xFFFF || data == 0x0000)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return -1;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==240)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0xFF)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0xFFFF)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
		
	}
	else if(pCurrDataBase->m_cF==241)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0x7F)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0x7FFF)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
	}
	else if(pCurrDataBase->m_cF==242)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0x40)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
	}
	else if(pCurrDataBase->m_cF==243)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0x0F)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
	}

	switch( pCurrDataBase->m_ucConvrule )
	{
	case 1:		//unsigned Type
	case 12:
		flo1=pCurrDataBase->m_fA;
		flo2=pCurrDataBase->m_fB;

		float_data = ((float)(unsigned int)data) * flo1 + flo2;

		//UNIT Conv
		//float_data=unit_conv(float_data, g_pstCurrDataBase[pTable->m_cCurrNumber].m_cUnit,
		//Float Range

		pCurrDataBase->m_fData=float_data;
		break;

	case 2:		//signed Type
		flo1=pCurrDataBase->m_fA;
		flo2=pCurrDataBase->m_fB;

		// 환산식 2번에서 D열에 있는 값은 signed, unsigned로 인식하는 경계선 값으로 사용

		if( pCurrDataBase->m_ucDataSize == 2)
				float_data = ((float)(signed short int)data) * flo1 + flo2;
		else
			float_data = ((float)(signed char)data) * flo1 + flo2;
/*
		if(pCurrDataBase->m_cD > 0 && (unsigned int)data <= pCurrDataBase->m_cD)
			float_data = ((float)(unsigned int)data) * flo1 + flo2;
		else
			//float_data = (float)((signed int)data);		//g-sensor 용 곱하기 위에서는 *1을 한다
			float_data = ((float)(signed int)data) * flo1 + flo2;
*/
		//UNIT Conv
		//Float Range

		pCurrDataBase->m_fData=float_data;
		break;

	case 3:
		lut_no = (unsigned int)pCurrDataBase->m_fA;		//LUT Number
		len_pos= (unsigned int)pCurrDataBase->m_fB;		//LUT Length
		order  = (unsigned int)pCurrDataBase->m_fC;		//LUT Order
		mask   = (unsigned char)pCurrDataBase->m_cD;				//LUT Masking value
		sintmask   = (unsigned int)pCurrDataBase->m_cD;
		strncpy(lut_buff, (char *)pCurrDataBase->pucArrLut, strlen((char *)pCurrDataBase->pucArrLut));

		switch(len_pos)
		{
		case 9:
			int_data = bit_mask( data);
			break;
		case 10:
			int_data = bit_mask3( data, mask);
			break;
		case 77:
			int_data = bit_mask2( data, sintmask);
			break;
		default:
			int_data = bit_mask1( data, mask);
			break;
		}

		pCurrDataBase->m_fData=LUT_Data_compute(lut_no, order, int_data, lut_buff);

		break;
		
	case 4:
		lut_no = (unsigned int)pCurrDataBase->m_fA;		//LUT Number
		len_pos= (unsigned int)pCurrDataBase->m_fB;		//LUT Length
		order  = (unsigned int)pCurrDataBase->m_fC;		//LUT Order
		mask   = (unsigned char)pCurrDataBase->m_cD;				//LUT Masking value
		sintmask   = (unsigned int)pCurrDataBase->m_cD;
		strncpy(lut_buff, (char *)pCurrDataBase->pucArrLut, strlen((char *)pCurrDataBase->pucArrLut));

		int_data = bit_mask4( data, mask);
		
		pCurrDataBase->m_fData=LUT_Data_compute(lut_no, order, int_data, lut_buff);
		break;
	case 5:
		flo1=pCurrDataBase->m_fA;
		flo2=pCurrDataBase->m_fB;
		iTemp=(int)pCurrDataBase->m_fC;
		ucLShift = pCurrDataBase->m_cD;		//!!!!!!!!!!!!!!!!!!!!!!상준책임 확인필요!!!!!!!!!!!!!!!!!!
		ucRShift = pCurrDataBase->m_cE;
	
		data = data & iTemp;

		if(ucLShift !=0)
			data = data<<ucLShift;
	
		if(ucRShift !=0)
			data = data>>ucRShift;
			
		float_data = ((float)(unsigned int)data) * flo1 + flo2;

		pCurrDataBase->m_fData=float_data;
		break;
	case 6:
		flo1=pCurrDataBase->m_fA;
		flo2=pCurrDataBase->m_fB;
		iTemp=(int)pCurrDataBase->m_fC;
	
		data = data & iTemp;
			
		float_data = ((float)(unsigned int)data) * flo1 + flo2;

		pCurrDataBase->m_fData=float_data;
		break;
	case 20:
		lut_no = (unsigned int)pCurrDataBase->m_fA;		//LUT Number
		len_pos= (unsigned int)pCurrDataBase->m_fB;		//LUT Length
		order  = (unsigned int)pCurrDataBase->m_fC;		//LUT Order

		strncpy(lut_buff, (char *)pCurrDataBase->pucArrLut, strlen((char *)pCurrDataBase->pucArrLut));

		pCurrDataBase->m_fData=LUT_Data_compute_20(lut_no, order, data, lut_buff);
		break;
	}
//#if defined(NAVI_DEBUG)
//	Trace(NAVI_DEBUG,"================ Compute result================\r\n");
//	Trace(NAVI_DEBUG,"INDEX : %d\r\n", pCurrDataBase->m_uiFuncIndex);
//	Trace(NAVI_DEBUG,"StartPos : %d\r\n", pCurrDataBase->m_ucStartPos);
//	Trace(NAVI_DEBUG,"RealPos : %d\r\n", pCurrDataBase->m_ucRealPos);
//
//	Trace(NAVI_DEBUG,"DataSize : %d\r\n", pCurrDataBase->m_ucDataSize);
//	Trace(NAVI_DEBUG,"data : %f\r\n", pCurrDataBase->m_fData);
//	Trace(NAVI_DEBUG,"================================================\r\n");
//
//
//#endif


	return true;
}

int8_t Compute_Data_V2(stActuatorResData *pCurrDataBase, int data)
{
	float float_data=0.0, flo1=0.0, flo2=0.0;
	unsigned int int_data=0, len_pos=0, lut_no=0, order=0, sintmask=0, iTemp=0;
	char lut_buff[150]={0,};
	U8 mask=0;
	U8 ucRShift =0, ucLShift = 0;

	// convrule.F이 127이면 수신값을 0x7F와 Masking한다.
	if(pCurrDataBase->m_cF==127)
	{
		data &= 0x7F;
	}
	else if(pCurrDataBase->m_cF==251)
	{
		if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0xFEFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==252)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0xFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0xFFFF || data == 0x7FFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==253 || pCurrDataBase->m_cF==255)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0xFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if((data == 0xFFFF)||(data == 0xFEFF))		//FEFF는 오토링크에 존재
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 4)
		{
			if(data == 0xFFFFFFFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==254)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0xFF || data == 0x00)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return -1;		// 초기값
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0xFFFF || data == 0x0000)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return -1;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==240)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0xFF)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0xFFFF)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
		
	}
	else if(pCurrDataBase->m_cF==241)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0x7F)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
		else if(pCurrDataBase->m_ucDataSize == 2)
		{
			if(data == 0x7FFF)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
	}
	else if(pCurrDataBase->m_cF==242)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0x40)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
	}
	else if(pCurrDataBase->m_cF==243)
	{
		if(pCurrDataBase->m_ucDataSize == 1)
		{
			if(data == 0x0F)
			{
				pCurrDataBase->m_fData = data;
				return 1;
			}
		}
	}

	switch( pCurrDataBase->m_ucConvrule )
	{
	case 1:		//unsigned Type
	case 12:
		flo1=pCurrDataBase->m_fA;
		flo2=pCurrDataBase->m_fB;

		float_data = ((float)(unsigned int)data) * flo1 + flo2;

		//UNIT Conv
		//float_data=unit_conv(float_data, g_pstCurrDataBase[pTable->m_cCurrNumber].m_cUnit,
		//Float Range

		pCurrDataBase->m_fData=float_data;
		break;

	case 2:		//signed Type
		flo1=pCurrDataBase->m_fA;
		flo2=pCurrDataBase->m_fB;

		// 환산식 2번에서 D열에 있는 값은 signed, unsigned로 인식하는 경계선 값으로 사용

		if( pCurrDataBase->m_ucDataSize == 2)
				float_data = ((float)(signed short int)data) * flo1 + flo2;
		else
			float_data = ((float)(signed char)data) * flo1 + flo2;
/*
		if(pCurrDataBase->m_cD > 0 && (unsigned int)data <= pCurrDataBase->m_cD)
			float_data = ((float)(unsigned int)data) * flo1 + flo2;
		else
			//float_data = (float)((signed int)data);		//g-sensor 용 곱하기 위에서는 *1을 한다
			float_data = ((float)(signed int)data) * flo1 + flo2;
*/
		//UNIT Conv
		//Float Range

		pCurrDataBase->m_fData=float_data;
		break;

	case 3:
		lut_no = (unsigned int)pCurrDataBase->m_fA;		//LUT Number
		len_pos= (unsigned int)pCurrDataBase->m_fB;		//LUT Length
		order  = (unsigned int)pCurrDataBase->m_fC;		//LUT Order
		mask   = (unsigned char)pCurrDataBase->m_cD;				//LUT Masking value
		sintmask   = (unsigned int)pCurrDataBase->m_cD;
		strncpy(lut_buff, (char *)pCurrDataBase->pucArrLut, strlen((char *)pCurrDataBase->pucArrLut));

		switch(len_pos)
		{
		case 9:
			int_data = bit_mask( data);
			break;
		case 10:
			int_data = bit_mask3( data, mask);
			break;
		case 77:
			int_data = bit_mask2( data, sintmask);
			break;
		default:
			int_data = bit_mask1( data, mask);
			break;
		}

		pCurrDataBase->m_fData=LUT_Data_compute(lut_no, order, int_data, lut_buff);

		break;
		
	case 4:
		lut_no = (unsigned int)pCurrDataBase->m_fA;		//LUT Number
		len_pos= (unsigned int)pCurrDataBase->m_fB;		//LUT Length
		order  = (unsigned int)pCurrDataBase->m_fC;		//LUT Order
		mask   = (unsigned char)pCurrDataBase->m_cD;				//LUT Masking value
		sintmask   = (unsigned int)pCurrDataBase->m_cD;
		strncpy(lut_buff, (char *)pCurrDataBase->pucArrLut, strlen((char *)pCurrDataBase->pucArrLut));

		int_data = bit_mask4( data, mask);
		
		pCurrDataBase->m_fData=LUT_Data_compute(lut_no, order, int_data, lut_buff);
		break;
	case 5:
		flo1=pCurrDataBase->m_fA;
		flo2=pCurrDataBase->m_fB;
		iTemp=(int)pCurrDataBase->m_fC;
		ucLShift = pCurrDataBase->m_cD;		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!상준책임 확인필요!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		ucRShift = pCurrDataBase->m_cE;
		
		data = data & iTemp;

		if(ucLShift !=0)
			data = data<<ucLShift;
	
		if(ucRShift !=0)
			data = data>>ucRShift;
			
		float_data = ((float)(unsigned int)data) * flo1 + flo2;

		pCurrDataBase->m_fData=float_data;
		break;
	case 6:
		flo1=pCurrDataBase->m_fA;
		flo2=pCurrDataBase->m_fB;
		iTemp=(int)pCurrDataBase->m_fC;
	
		data = data & iTemp;
			
		float_data = ((float)(unsigned int)data) * flo1 + flo2;

		pCurrDataBase->m_fData=float_data;
		break;
		
	case 20:
		lut_no = (unsigned int)pCurrDataBase->m_fA;		//LUT Number
		len_pos= (unsigned int)pCurrDataBase->m_fB;		//LUT Length
		order  = (unsigned int)pCurrDataBase->m_fC;		//LUT Order

		strncpy(lut_buff, (char *)pCurrDataBase->pucArrLut, strlen((char *)pCurrDataBase->pucArrLut));

		pCurrDataBase->m_fData=LUT_Data_compute_20(lut_no, order, data, lut_buff);
		break;
	}
//#if defined(NAVI_DEBUG)
//	Trace(NAVI_DEBUG,"================ Compute result================\r\n");
//	Trace(NAVI_DEBUG,"INDEX : %d\r\n", pCurrDataBase->m_uiFuncIndex);
//	Trace(NAVI_DEBUG,"StartPos : %d\r\n", pCurrDataBase->m_ucStartPos);
//	Trace(NAVI_DEBUG,"RealPos : %d\r\n", pCurrDataBase->m_ucRealPos);
//
//	Trace(NAVI_DEBUG,"DataSize : %d\r\n", pCurrDataBase->m_ucDataSize);
//	Trace(NAVI_DEBUG,"data : %f\r\n", pCurrDataBase->m_fData);
//	Trace(NAVI_DEBUG,"================================================\r\n");
//
//
//#endif


	return true;
}

//170218 LWH 함수 형태 변경 F의 예외처리가 된 후에 데이터 처리방식 추가
int8_t Compute_Data(stSlaveData *pCurrDataBase, int data)
{
	float float_data=0.0, flo1=0.0, flo2=0.0;
	unsigned int int_data=0, len_pos=0, lut_no=0, order=0, sintmask=0;
	char lut_buff[150]={0,};
	int iTemp=0;
	U8 mask=0;
	U8 ucRShift =0, ucLShift = 0;

	// convrule.F이 127이면 수신값을 0x7F와 Masking한다.
	if(pCurrDataBase->m_cF==127)
	{
		data &= 0x7F;
	}
	else if(pCurrDataBase->m_cF==251)
	{
		if(pCurrDataBase->m_cDataSize == 2)
		{
			if(data == 0xFEFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==252)
	{
		if(pCurrDataBase->m_cDataSize == 1)
		{
			if(data == 0xFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
		else if(pCurrDataBase->m_cDataSize == 2)
		{
			if(data == 0xFFFF || data == 0x7FFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==253 || pCurrDataBase->m_cF==255)
	{
		if(pCurrDataBase->m_cDataSize == 1)
		{
			if(data == 0xFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
		else if(pCurrDataBase->m_cDataSize == 2)
		{
			if((data == 0xFFFF)||(data == 0xFEFF))		//FEFF는 오토링크에 존재
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
		else if(pCurrDataBase->m_cDataSize == 4)
		{
			if(data == 0xFFFFFFFF)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return 0;		// 초기값
			}
		}
	}
	else if(pCurrDataBase->m_cF==254)
	{
		if(pCurrDataBase->m_cDataSize == 1)
		{
			if(data == 0xFF || data == 0x00)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return -1;		// 초기값
			}
		}
		else if(pCurrDataBase->m_cDataSize == 2)
		{
			if(data == 0xFFFF || data == 0x0000)
			{
				float_data=0;		//FF일경우 0
				pCurrDataBase->m_fData = float_data;
				return -1;		// 초기값
			}
		}
	}

	switch( pCurrDataBase->m_cCunvRule )
	{
	case 1:		//unsigned Type
	case 12:
		flo1=pCurrDataBase->m_cA;
		flo2=pCurrDataBase->m_cB;

		float_data = ((float)(unsigned int)data) * flo1 + flo2;

		//UNIT Conv
		//float_data=unit_conv(float_data, g_pstCurrDataBase[pTable->m_cCurrNumber].m_cUnit,
		//Float Range

		pCurrDataBase->m_fData=float_data;
		break;

	case 2:		//signed Type
		flo1=pCurrDataBase->m_cA;
		flo2=pCurrDataBase->m_cB;

		// 환산식 2번에서 D열에 있는 값은 signed, unsigned로 인식하는 경계선 값으로 사용

		if( pCurrDataBase->m_cDataSize == 2)
				float_data = ((float)(signed short int)data) * flo1 + flo2;
		else
			float_data = ((float)(signed char)data) * flo1 + flo2;
/*
		if(pCurrDataBase->m_cD > 0 && (unsigned int)data <= pCurrDataBase->m_cD)
			float_data = ((float)(unsigned int)data) * flo1 + flo2;
		else
			//float_data = (float)((signed int)data);		//g-sensor 용 곱하기 위에서는 *1을 한다
			float_data = ((float)(signed int)data) * flo1 + flo2;
*/
		//UNIT Conv
		//Float Range

		pCurrDataBase->m_fData=float_data;
		break;

	case 3:
		lut_no = (unsigned int)pCurrDataBase->m_cA;		//LUT Number
		len_pos= (unsigned int)pCurrDataBase->m_cB;		//LUT Length
		order  = (unsigned int)pCurrDataBase->m_cC;		//LUT Order
		mask   = (unsigned char)pCurrDataBase->m_cD;				//LUT Masking value
		sintmask   = (unsigned int)pCurrDataBase->m_cD;
		strncpy(lut_buff, pCurrDataBase->m_cLut, strlen(pCurrDataBase->m_cLut));

		switch(len_pos)
		{
		case 9:
			int_data = bit_mask( data);
			break;
		case 10:
			int_data = bit_mask3( data, mask);
			break;
		case 77:
			int_data = bit_mask2( data, sintmask);
			break;
		default:
			int_data = bit_mask1( data, mask);
			break;
		}

		pCurrDataBase->m_fData=LUT_Data_compute(lut_no, order, int_data, lut_buff);

		break;
		
	case 4:
		lut_no = (unsigned int)pCurrDataBase->m_cA;		//LUT Number
		len_pos= (unsigned int)pCurrDataBase->m_cB;		//LUT Length
		order  = (unsigned int)pCurrDataBase->m_cC;		//LUT Order
		mask   = (unsigned char)pCurrDataBase->m_cD;				//LUT Masking value
		sintmask   = (unsigned int)pCurrDataBase->m_cD;
		strncpy(lut_buff, (char *)pCurrDataBase->m_cLut, strlen((char *)pCurrDataBase->m_cLut));

		int_data = bit_mask4( data, mask);
		
		pCurrDataBase->m_fData=LUT_Data_compute(lut_no, order, int_data, lut_buff);
		break;
	case 5:
		flo1=pCurrDataBase->m_cA;
		flo2=pCurrDataBase->m_cB;
		iTemp=(int)pCurrDataBase->m_cC;
		ucLShift = pCurrDataBase->m_cD;		//!!!!!!!!!!!!!!!!!!!상준책임 확인필요!!!!!!!!!!!!!!!
		ucRShift = pCurrDataBase->m_cE;
		
		data = data & iTemp;

		if(ucLShift !=0)
			data = data<<ucLShift;
	
		if(ucRShift !=0)
			data = data>>ucRShift;
			
		float_data = ((float)(unsigned int)data) * flo1 + flo2;

		pCurrDataBase->m_fData=float_data;
		break;
	case 6:
		flo1=pCurrDataBase->m_cA;
		flo2=pCurrDataBase->m_cB;
		iTemp=(int)pCurrDataBase->m_cC;
	
		data = data & iTemp;
			
		float_data = ((float)(unsigned int)data) * flo1 + flo2;

		pCurrDataBase->m_fData=float_data;
		break;
	case 20:
		lut_no = (unsigned int)pCurrDataBase->m_cA;		//LUT Number
		len_pos= (unsigned int)pCurrDataBase->m_cB;		//LUT Length
		order  = (unsigned int)pCurrDataBase->m_cC;		//LUT Order

		strncpy(lut_buff, pCurrDataBase->m_cLut, strlen(pCurrDataBase->m_cLut));

		pCurrDataBase->m_fData=LUT_Data_compute_20(lut_no, order, data, lut_buff);
		break;
	}

//#if defined(NAVI_DEBUG)
//	Trace(NAVI_DEBUG,"================ Compute result================\r\n");
//	Trace(NAVI_DEBUG,"INDEX : %s\r\n", pCurrDataBase->m_cIndexFine);
//	Trace(NAVI_DEBUG,"StartPos : %d\r\n", pCurrDataBase->m_cStartPos);
//	Trace(NAVI_DEBUG,"RealPos : %d\r\n", pCurrDataBase->m_cRealPos);
//
//	Trace(NAVI_DEBUG,"DataSize : %d\r\n", pCurrDataBase->m_cDataSize);
//	Trace(NAVI_DEBUG,"data : %f\r\n", pCurrDataBase->m_fData);
//	Trace(NAVI_DEBUG,"================================================\r\n");
//
//
//#endif

	return true;
}

void SupportRecordValue_FuncType14(U8* pData, U8 pos)
{
	BYTE ucPid, ucData, ucBit;
	char ucLength=0, ucStartCnt[2]={0,},  ucStartPos=0, ucPidPos=0;
	U8 ucRescode[20]={0,}, ucCompareLength=0;
	char ucResBuff[20]={0,};

	ucLength = strlen(g_pstCurrDataBase[pos].m_cResNode);

	if((strlen(g_pstCurrDataBase[pos].m_cReqNode)-2)/2 == 5)		//UDS REQ - LENGTH
	{
		sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)
		ucCompareLength = 10;
	}
	else if((strlen(g_pstCurrDataBase[pos].m_cReqNode)-2)/2 == 4)	//CAN REQ - LENGTH
	{
		sprintf((char*)ucRescode, "%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3]);								//respons copy(ASCII)
		ucCompareLength = 8;
	}


	if(strncmp((char const*)ucRescode, g_pstCurrDataBase[pos].m_cResNode, ucCompareLength)==0)	// Response 비교
	{
		strcpy(ucResBuff, g_pstCurrDataBase[pos].m_cResNode);

		if(strcmp(g_pstCurrDataBase[pos].m_cIndexFine, DCS_ODOMETER_SUPPORTED)==0)
		{
			ucPid=g_pstCurrDataBase[getSlaveTableNum(DCS_ODOMETER)].m_cRealPos;	//PID저장
		}

		//07 D9 62 01 04 ## 06 FF
		for(int i=0; i<ucLength; i++)
		{
			if(ucResBuff[i] == '#' && ucResBuff[i+1] == '#')
			{
				sprintf(ucStartCnt, "%02x", AsciiToHex(ucResBuff[i+2], ucResBuff[i+3]));
				ucStartPos=atoi((char const*)ucStartCnt);
				break;
			}
		}

		if(ucResBuff[ucLength-2] == 'F' && ucResBuff[ucLength-1] == 'F')
			ucBit=0x80;
		else
			ucBit=0x01;

		if(ucPid<=0x08)
		{
			ucData=pData[ucStartPos-1];
			ucPidPos=0;
		}
		else if(ucPid<=0x10)
		{
			ucData=pData[ucStartPos-1+1];
			ucPidPos=1;
		}
		else if(ucPid<=0x18)
		{
			ucData=pData[ucStartPos-1+2];
			ucPidPos=2;
		}
		else if(ucPid<=0x20)
		{
			ucData=pData[ucStartPos-1+3];
			ucPidPos=3;
		}

		for(int b=0; b<8; b++)
		{
			if(ucData & ucBit)
			{
				if(ucPid == ((8 * ucPidPos) + (b + 1)))
					ucStartPos = TRUE;
			}

			if(ucResBuff[ucLength-2] == 'F' && ucResBuff[ucLength-1] == 'F')		// FF
				ucBit=ucBit>>1;
			else						// 00
				ucBit=ucBit<<1;
		}

		if(ucStartPos == TRUE)
		{
			ODO_TYPE_SET_STATE(ODO_TYPE_SUPP_DCS_ODOMETER);
		}
		else
		{
			/*if(getSlaveTableNum(DCS_ODOMETER_SUB) != INVALID_INDEX)
			{
				ODO_TYPE_SET_STATE(ODO_TYPE_SUPP_DCS_ODOMETER_SUB);
			}
			else*/
			{
				ODO_TYPE_SET_STATE(ODO_TYPE_NONE);
			}
		}

	}
}

void SupportRecordValue(U8* pData, U8 pos)
{
	switch(g_pstCurrDataBase[pos].m_cDataType)		//FUNCTION TYPE
	{
	case 4:
		//SupportRecordValue_FuncType4(pData, pos);
		break;
	case 14:
		SupportRecordValue_FuncType14(pData, pos);
		break;
	default:
		//SupportRecordValue_FuncType4(pData, pos);
		break;
	}
}

U8 getSlaveTableNum(char index[])
{
	int i=0;
	BOOL bState=FALSE;
	for(i=0; i<g_usCurrentCnt; i++)
	{
		if(strcmp(g_pstCurrDataBase[i].m_cIndexFine, index)==0)
		{
			bState=TRUE;
			break;
		}
	}
	if(bState == TRUE)
		return i;
	else
		return INVALID_INDEX;
}

BOOL ValidationODO(U32 odoData)
{
	//static U32 arruiOdoTmp[5]={0,};
	static U32 s_uiOldTime=0;
	static U32 s_uiOldOdo=0;
	static uint32_t s_unSuccessCount=0;
	static uint32_t s_unFailOldOdo=0;
	U32 uiNewTimer=0, uiDeltaTimer=0;
//	U32 uiSpeed=0;
	float fSpeed=0;
	uiNewTimer 			= Get_Tmr();
	uiDeltaTimer			= Get_TmrDelta(uiNewTimer, s_uiOldTime);
	s_uiOldTime  		= uiNewTimer;



	if(uiNewTimer!= uiDeltaTimer)
	{
		//360km/h = 6km/m = 0.1km/s = 0.0001km/ms	= 0.1m/ms
		//360km/h로 1km 가는데 걸리는 시간 = 10000ms
		//270km/h로 1km 가는데 걸리는 시간 = 15000ms
		//180km/h로 1km 가는데 걸리는 시간 = 20000ms
		//90km/h로 1km 가는데 걸리는 시간 = 40000ms
		//45km/h로 1km 가는데 걸리는 시간 = 80000ms

		//속도 계산해서 비이상적일경우 FALSE 처리
//
//		uiSpeed = (odoData - s_uiOldOdo) / (uiDeltaTimer * 1000/*1000.0*/);
		fSpeed=(float)((odoData - s_uiOldOdo) / (float)(uiDeltaTimer/1000.0 /3600.0));

//		fTime = uiDeltaTimer/1000.0 /3600.0;
//		fSpd = odoData - s_uiOldOdo / fTime;
//		Trace(0,"time : %d s1:%f s2:%f \r\n", uiDeltaTimer, fSpeed, fSpeed2);
//		//if((odoData <= 0) || (uiSpeed >= DCS_ODO_ERROR))T
		if((odoData <= 0) || (fSpeed >= DCS_ODO_ERROR))
		{
			
			fSpeed=(float)((odoData - s_unFailOldOdo) / (float)(uiDeltaTimer/1000.0 /3600.0));
			
			// recovery function if fail state.
			if((odoData <= 0) || ( (int32_t)fSpeed >= DCS_ODO_ERROR))
			{
				s_unSuccessCount = 0;
				s_unFailOldOdo = odoData;
				return false;
			}
			if( s_unSuccessCount++ > 3 )
			{
				s_uiOldOdo = odoData;
				return true;
			}
			return FALSE;
		}
	}
	
	s_unSuccessCount = 0;
	s_uiOldOdo = odoData;
	return TRUE;
}

void freeitems()
{
	if ( g_pstFastFunction != NULL)
	{
		free(g_pstFastFunction);
		g_pstFastFunction=NULL;
	}
	if ( g_pstSlow1Function != NULL)
	{
		free(g_pstSlow1Function);
		g_pstSlow1Function=NULL;
	}
	if ( g_pstSlow2Function != NULL)
	{
		free(g_pstSlow2Function);
		g_pstSlow2Function=NULL;
	}
	if ( g_pstSlow3Function != NULL)
	{
		free(g_pstSlow3Function);
		g_pstSlow3Function=NULL;
	}
	if ( g_pstSuppFunction != NULL)
	{
		free(g_pstSuppFunction);
		g_pstSuppFunction=NULL;
	}

	if ( g_pstCurrDataBase != NULL)
	{
		free(g_pstCurrDataBase);
		g_pstCurrDataBase=NULL;
	}
	if ( g_pstDTCDataBase != NULL)
	{
		free(g_pstDTCDataBase);
		g_pstDTCDataBase=NULL;
	}

	if ( g_pDTCRequest != NULL)
	{
		free(g_pDTCRequest);
		g_pDTCRequest=NULL;
	}
	if ( g_pFastRequest != NULL)
	{
		free(g_pFastRequest);
		g_pFastRequest=NULL;
	}
	if ( g_pSlow1Request != NULL)
	{
		free(g_pSlow1Request);
		g_pSlow1Request=NULL;
	}
	if ( g_pSlow2Request != NULL)
	{
		free(g_pSlow2Request);
		g_pSlow2Request=NULL;
	}
	if ( g_pSlow3Request != NULL)
	{
		free(g_pSlow3Request);
		g_pSlow3Request=NULL;
	}
	if ( g_pSuppRequest != NULL)
	{
		free(g_pSuppRequest);
		g_pSuppRequest=NULL;
	}

	if ( g_stDTCFuncData != NULL)
	{
		free(g_stDTCFuncData);
		g_stDTCFuncData=NULL;
	}

    if ( g_pDTCState != NULL )
    {
            free(g_pDTCState);
            g_pDTCState = NULL;
    }

	if ( g_psrFastRefTable != NULL)
	{
		free(g_psrFastRefTable);
		g_psrFastRefTable=NULL;
	}
	if ( g_psrSlow1RefTable != NULL)
	{
		free(g_psrSlow1RefTable);
		g_psrSlow1RefTable=NULL;
	}
	if ( g_psrSlow2RefTable != NULL)
	{
		free(g_psrSlow2RefTable);
		g_psrSlow2RefTable=NULL;
	}
	if ( g_psrSlow3RefTable != NULL)
	{
		free(g_psrSlow3RefTable);
		g_psrSlow3RefTable=NULL;
	}
	if ( g_psrSuppRefTable != NULL)
	{
		free(g_psrSuppRefTable);
		g_psrSuppRefTable=NULL;
	}
}
bool VIN_ValidCheck()
{
	int i=0;
	bool bRet=false;
	for( i=0; i<DCS_AUTOVIN_SIZE; i++ )
	{
		if( (g_stAutoVin.strVinCode[i]>=0x30 && g_stAutoVin.strVinCode[i]<=0x39) ||  (g_stAutoVin.strVinCode[i]>=0x41 && g_stAutoVin.strVinCode[i]<=0x5A) ) bRet = true;
		else
		{
			bRet = false;
			break;
		}
	}
	return bRet;
}


void CB_FuelCheck_30secCallback()
{
	if ((Get_Speed()==0) && (Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN)
//		&& ( -2 <= (float)((float)g_nSumAnglex/(float)g_nAngleSumCount) && (float)((float)g_nSumAnglex/(float)g_nAngleSumCount) <= 2)
//		&& ( -2 <= (float)((float)g_nSumAngley/(float)g_nAngleSumCount) && (float)((float)g_nSumAngley/(float)g_nAngleSumCount)<= 2))
		&& ((float)       (((float)g_nSumAnglex / (float)g_nAngleSumCount ) <= 2))
		&& ((float)       (((float)g_nSumAngley / (float)g_nAngleSumCount ) <= 2))
        || ( (Get_Speed()==0) && (Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN) && (g_bFuelLevelSwitchedPercentFlag == true) ) )
	{
//		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Go FuelCheck~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
		g_bFuelLevelCheckFlag = true;
		g_bFuelLevelCheckOneTimeRunFlag = false;

	}
	else
	{
//		printf("CB_FuelCheck_30secCallback] Out of Gyro Range !!!!!!!!!!!!!!!!!!!\r\n");
		HalTimerClearSWTimer(g_iTimerFuelCheck30secCallback);
		g_iTimerFuelCheck30secCallback = -1;
	}
		//printf("avg gyro cal]%f %f-------------\r\n", (float)((float)g_nSumAnglex/(float)g_nAngleSumCount),(float)((float)g_nSumAngley/(float)g_nAngleSumCount));
//		printf("%d %d %d %d-------------\r\n",Get_Speed(), g_nSumTmpx,  g_nSumTmpy, g_nAngleSumCount);
	g_nAngleSumCount=0; g_nSumAnglex=0; g_nSumAngley=0;
}

//채널 1번으로 들어오는 데이터
void GitCANReadMsgs(stPASSTHRU_MSG *pReadMsg, unsigned int uiReadMsgLen, unsigned int eInCommType, boolean_t bIsStandard)
{
	eOBD_STATE eOBDState;
	int data=0;
	int i=0;
//	int cnt;
	U8 * pData;
	U8 ucRescode[20];
//	static U8  ucGarbageOdoCnt=0;
//	static INT16U OldRpm=0, s_uiOldMotorRpm = 0;
//	static U8 s_ucSavedPowerReady=0;
//	U8 ucBuff=0;
	int8_t scResult;
//	int nVinCnt=0;
	int iCnt=0;
//	float fTemp=0;
	unsigned int uiResID=0;
#ifndef NO_FREEZE_FRAME
	unsigned char ucTempPosFreeze = 0;
#endif
//	int iTPMS_Temp=0;
	uint8_t ucPACVType=0;

	GetAutolinkConfigProperty(eAutoLinkConfig_PACVType,(void*)&ucPACVType);
	
	eOBDState = GetOBDState();

	pData = &pReadMsg->pData[0];
	memset(ucRescode, 0x00, sizeof(ucRescode)/sizeof(ucRescode[0]));

	if(pReadMsg->pData[2]==0x7F)		//negative 처리
	{
		if(pReadMsg->pData[4]!=0x78) return;
	}

	if( eOBDState == eOBD_GetAutoVIN )
	{
		if(uiReadMsgLen>50)
			uiReadMsgLen = 0x00;		//cubis관련 버퍼 overflow 방지


#if 1 // James Jean 2018/11/14
		g_stAutoVin.ucSize = DCS_AUTOVIN_SIZE;
		
		if(ucPACVType == CV)
		{
			if(g_eCVAutoVinReqFuelType == eAUTOVIN_CV_UNIVERSE_REQ)
			{
				// REQ : 18 DA 00 F9 04 21 41 01 01
	            // RES : 18 DA F9 00 10 CA 61 41 DATA1~ DATA200 
				memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[67], DCS_AUTOVIN_SIZE);
			}
			else if(g_eCVAutoVinReqFuelType == eAUTOVIN_CV_ELECCITY_REQ)
			{
				// REQ : 18 DA EF F9 03 22 F1 90
	            // RES : 18 DA F9 EF 10 14 62 F1 90 DATA1~ DATA17
				memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[7], DCS_AUTOVIN_SIZE);
			}
        }
		else//if(ucPACVType==PA)
		{
	        if( g_eAutoVinReqFuelType == eAUTOVIN_ELECTIC_REQ )
	        {
	            memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[4], DCS_AUTOVIN_SIZE);
	        }
			else if( (g_eAutoVinReqFuelType == eAUTOVIN_HYDRO_REQ) || (g_eAutoVinReqFuelType == eAUTOVIN_ELECTIC_UDS_REQ) )
	        {
	            memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[5], DCS_AUTOVIN_SIZE);
	        }
	        else/* ( (g_eAutoVinReqFuelType == eAUTOVIN_ENGINE_REQ) ||
	                 (g_eAutoVinReqFuelType == eAUTOVIN_HYDRO_REQ) )*/
	        {
	            if(pReadMsg->pData[2] != 0x49 || pReadMsg->pData[3] != 0x02)
	            {
	                Trace(0,"AutoVIN Read Fail!!\r\n");
	                //SetOBDState(eOBD_GetAutoVIN_Fail);
	                return; //주행중 autovin data꼬임
	            }
	            // 인도 IB(컨티넨탈 디젤 엔진) 에서 부정응담이 온다.
				if(pReadMsg->pData[0] == 0x07 && pReadMsg->pData[2] == 0x7F/* || pReadMsg->pData[3] == 0x09 && pReadMsg->pData[4] == 0x12*/)
				{
				      // 부정응답 이후 07DF 02 09 00 00 00 00 00 00을 전달한 후 AutoVin을 읽어면 정상적으로 읽어 온다.
				      // 재시도시에 위 패킷을 전달하는 로직을 만듦
				      g_bAutoVinNegativeResponse = TRUE;
				      return;
				}
				else if(pReadMsg->pData[0] == 0x07 && pReadMsg->pData[2] == 0x49 && pReadMsg->pData[3] == 0x00 ) 
				{
				      return;
				}

	            g_bAutoVinNegativeResponse = FALSE;
	            memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[5], DCS_AUTOVIN_SIZE);
	        }
		}
#else

		if(pReadMsg->pData[2] != 0x49 || pReadMsg->pData[3] != 0x02)
		{
			Trace(0,"AutoVIN Read Fail!!\r\n");
			//SetOBDState(eOBD_GetAutoVIN_Fail);
			return;	//주행중 autovin data꼬임
		}
		g_stAutoVin.ucSize = DCS_AUTOVIN_SIZE;

        memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[5], DCS_AUTOVIN_SIZE);
#endif

		g_stAutoVin.strVinCode[DCS_AUTOVIN_SIZE] = 0x00;
		if( VIN_ValidCheck() == true )
		{
			g_stAutoVin.bRxStatus = true;	//CAN RX TRUE
		}
		else
		{
			g_stAutoVin.bRxStatus = false;
			Trace(0,"AutoVIN Read Fail!!2\r\n");
			return;
		}
#ifdef FIX_VIN
		memcpy(&g_stAutoVin.strVinCode, FIX_VIN_NUMBER, DCS_AUTOVIN_SIZE);
#endif
		Trace(0,"~~~~~~~~~~~~~~~~~~~~~GetAutoVIN : %s  ~~~~~~~~~~~~~~~~~~~~~\r\n",g_stAutoVin.strVinCode);

		printf("\n\n\n================================================\r\n");
		printf("AUTO VIN RESULT\n");
		hexdump(g_stAutoVin.strVinCode, DCS_AUTOVIN_SIZE);
//		for(nVinCnt=0; nVinCnt<DCS_AUTOVIN_SIZE; nVinCnt++ )
//			Trace(0,"%c", g_stAutoVin.strVinCode[nVinCnt]);
//		Trace(0,"\r\n");

		//SetOBDState(eOBD_GetAutoVIN_Success);
	}
#ifdef CGW_SECURITY
	else if( eOBDState == eOBD_CGWAlgorithm )
	{
		memset(g_unCGWSeed,0,sizeof(g_unCGWSeed));
		for(int j=0;j<8;j++)
		{
			g_unCGWSeed[j] = pData[4+j];
		}
	}
#endif
	else if(g_eActaveType == ACTUATOR_TYPE_WAKEUP){}	// 204라인 Wakeup 체크하려하는데 204에서 203의 IG 상태와 똑같은 CANID가 들어와서 순간 시동OFF로 체크됨 - 웨이크업시는 데이터 처리 안하도록 수정(203,204구분불가)
	else if( eOBDState == eOBD_Actuator_Mode )	//제어DB일때 타는곳
	{
		if( bIsStandard == true ) 
        {
            uiResID = (unsigned int)(pData[0]*256) + pData[1];
		}
        else
        {
            uiResID = (unsigned int)(pData[0]<<24) + (unsigned int)(pData[1]<<16) + (unsigned int)(pData[2]<<8) + pData[3];
        }

		if( ( GetActuatorStatus() != ACTUATOR_STATUS_CTRL_RUNNING ) || ( FUELTYPE_GET_STATE() == EV_NONE_READY ))	//러닝일때는 최대한 영향없게 하기위해
		{
			for( i=0; i<g_stActuatorReadyInfo.m_uiCount; i++ )
			{
				if( uiResID == g_stActuatorReadyData[ i ].m_uiResVal )	// Response 비교
				{
					if(CFD_GetCanFDAdapter())
					{
					  	if(( g_stActuatorReadyData[ i ].m_ucFDCanLine)== pReadMsg->RxStatus)
						{
							CurrentCalculate_V(i, &data, pData);
							scResult = Compute_Data_V(&g_stActuatorReadyData[ i ], data);
							if(scResult != -1) Current_ActuatorReady(g_stActuatorReadyData,i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH
						}
					}
					else
					{
					  	if( ((g_stActuatorReadyData[ i ].m_ucCanLine == HIGHCAN1) || (g_stActuatorReadyData[ i ].m_ucCanLine == HIGHCAN2)) && (pReadMsg->RxStatus==CAN_CH1))
						{
							CurrentCalculate_V(i, &data, pData);
							scResult = Compute_Data_V(&g_stActuatorReadyData[ i ], data);
							if(scResult != -1) Current_ActuatorReady(g_stActuatorReadyData,i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH
						}
					}
				}
			}
		}
		else
		{

		}
	}
	else if( eOBDState == eOBD_Running_Info_Mode || eOBDState == eOBD_GetFuelLevelCheck || eOBDState == eOBD_GetSOH|| eOBDState == eOBD_GetBrakeJudder)	//진단DB일때 타는곳
	{
		if(pReadMsg->pData[0]==0x07 && pReadMsg->DataSize != g_uiCanReadMsgLength) {
			pReadMsg->pData[0]=0x07;
			Trace(0,"[DRV]D_Data size Miss!%02X %02X %02X %02X %02X %02X %02X\r\n",g_uiCanReadMsgLength,pReadMsg->DataSize,pReadMsg->pData[1],pReadMsg->pData[2],pReadMsg->pData[3],pReadMsg->pData[4],pReadMsg->pData[5]);
			return;
		}
		g_ucCanCommSucces++;
		
		if( bIsStandard == true ) 
        {
            uiResID = (unsigned int)(pData[0]*256) + pData[1];
			if(ucPACVType == CV)		sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", 0, 0, pData[0], pData[1], pData[2]);
			else/*(ucPACVType == PA)*/ 	sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)
		}
        else
        {
            uiResID = (unsigned int)(pData[0]<<24) + (unsigned int)(pData[1]<<16) + (unsigned int)(pData[2]<<8) + pData[3];
			sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)
        }

		for(i=0; i<g_usCurrentCnt; i++)
		{
			U8 ucResLength=0;
			if(g_pstCurrDataBase[i].m_cCommType == 'V')
			{
				if(ucPACVType == CV)		ucResLength = 8;	
				else/*if(ucPACVType== PA)*/ ucResLength = 4;
			}
			else
			{
				ucResLength = strlen(g_pstCurrDataBase[i].m_cResNode);

				if(ucResLength > 10)
				{
					ucResLength = 10;
				}
			}

			if(strncmp((char const*)ucRescode, g_pstCurrDataBase[i].m_cResNode, ucResLength)==0)	// Response 비교
			{
				if( ((g_pstCurrDataBase[ i ].m_ucCanLine == HIGHCAN1) || (g_pstCurrDataBase[ i ].m_ucCanLine == HIGHCAN2)) && (pReadMsg->RxStatus==CAN_CH1))
				{
					CurrentCalculate(i, &data, pData);
					scResult = Compute_Data(&g_pstCurrDataBase[i], data);
					if(scResult != -1) Current_SlaveData(g_pstCurrDataBase, i);	//예외처리된 데이터는 처리하지 않는다 170218 LWH
				}
			}
		}
		for( i=0; i<g_stActuatorReadyInfo.m_uiCount; i++ )		//210310 lwh 수소 특성화 데이터의 202 라인을 보게 하기 위해 넣음
		{
			if( uiResID == g_stActuatorReadyData[ i ].m_uiResVal )	// Response 비교
			{
				if(CFD_GetCanFDAdapter())
				{
					if(( g_stActuatorReadyData[ i ].m_ucFDCanLine)== pReadMsg->RxStatus)
					{
						CurrentCalculate_V(i, &data, pData);
						scResult = Compute_Data_V(&g_stActuatorReadyData[ i ], data);
						if(scResult != -1) Current_ActuatorReady(g_stActuatorReadyData,i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH
					}
				}
				else
				{
					if( ((g_stActuatorReadyData[ i ].m_ucCanLine == HIGHCAN1) || (g_stActuatorReadyData[ i ].m_ucCanLine == HIGHCAN2)) && (pReadMsg->RxStatus==CAN_CH1))
					{
						CurrentCalculate_V(i, &data, pData);
						scResult = Compute_Data_V(&g_stActuatorReadyData[ i ], data);
						if(scResult != -1) Current_ActuatorReady(g_stActuatorReadyData,i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH
					}
				}
			}
		}

	}
	else if( eOBDState == eOBD_FCS_Start )
	{
		if(pReadMsg->pData[0]==0x07 && pReadMsg->DataSize != g_uiCanReadMsgLength)
		{
			pReadMsg->pData[0]=0x07;
			Trace(0,"[FCS]Data size Miss!%02X %02X %02X %02X %02X %02X %02X\r\n",g_uiCanReadMsgLength,pReadMsg->DataSize,pReadMsg->pData[1],pReadMsg->pData[2],pReadMsg->pData[3],pReadMsg->pData[4],pReadMsg->pData[5]);
		   return;
		}
		if( bIsStandard == true ) 
        {
        	uiResID = (unsigned int)(pData[0]*256) + pData[1];
			if(ucPACVType == CV)			sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", 0, 0, pData[0], pData[1], pData[2]);
        	else/*if(ucPACVType == PA)*/ 	sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)
		}
        else
        {
            uiResID = (unsigned int)(pData[0]<<24) + (unsigned int)(pData[1]<<16) + (unsigned int)(pData[2]<<8) + pData[3];
			sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)
        }
		for(i=0; i<g_ucDtcSystemCnt; i++)
		{
            if(g_pstDTCDataBase[i].m_cReadno == 2)
			{
				g_stDTCFuncData[i].m_ucUDSFlag = eDTC_CAN;
			}
			else if(g_pstDTCDataBase[i].m_cReadno == 3)
			{
				g_stDTCFuncData[i].m_ucUDSFlag = eDTC_UDS;
			}
            
			if(strncmp(g_pstDTCDataBase[i].m_cResNode, (char const*)ucRescode, strlen(g_pstDTCDataBase[i].m_cResNode))==0)
			{
				int iStartPos=0, iReadNum;

				iStartPos = g_pstDTCDataBase[i].m_cStartPos;
				iReadNum = g_pstDTCDataBase[i].m_cReadno;
				if(g_pstDTCDataBase[i].m_cReadno == 2)
				{
					g_stDTCFuncData[i].m_ucDTCCount = pData[3];	//DTC 갯수
				}
				else if(g_pstDTCDataBase[i].m_cReadno == 3)
				{
					g_stDTCFuncData[i].m_ucDTCCount = (uiReadMsgLen - (iStartPos - 1))/DCS_DTC_CODE_SIZE;	//DTC 갯수
				}

				do
				{
					for(iCnt=0; iCnt<g_stDTCFuncData[i].m_ucDTCCount; iCnt++)
					{
						for(int dtcloop=0; dtcloop<iReadNum+1; dtcloop++)
						{
							U8 ucCanBlank=0;
							if((g_stDTCFuncData[i].m_ucUDSFlag == eDTC_CAN) && dtcloop==iReadNum)
							{
								g_stDTCFuncData[i].m_ucDTCCode[(DCS_DTC_CODE_SIZE*iCnt) + dtcloop] = 0xFF;
								ucCanBlank=1;
							}
							g_stDTCFuncData[i].m_ucDTCCode[(DCS_DTC_CODE_SIZE*iCnt) + dtcloop + ucCanBlank] = pData[(iStartPos-1) +(((iReadNum+1)*iCnt)+dtcloop)];
						}
						if(iCnt * DCS_DTC_CODE_SIZE > DTC_DATA_MAX) break;	//DCT MAX 100
					}
				}while(pReadMsg->DataSize>(iStartPos-1)+((iReadNum+1)*iCnt));

				if(g_stDTCFuncData[i].m_ucDTCCount>0)
				{
					g_stDTCFuncData[i].m_ucState=2;	//DTC 있음
				}
				else
					g_stDTCFuncData[i].m_ucState=1;	//DTC 없음
			}
		}
	}
#ifndef NO_FREEZE_FRAME
	else if( eOBDState == eOBD_FreezeFrame )
	{
		if(pReadMsg->pData[0]==0x07 && pReadMsg->DataSize != g_uiCanReadMsgLength)
		{
			pReadMsg->pData[0]=0x07;
			Trace(0,"[FF]Data size Miss!%02X %02X %02X %02X %02X %02X %02X\r\n",g_uiCanReadMsgLength,pReadMsg->DataSize,pReadMsg->pData[1],pReadMsg->pData[2],pReadMsg->pData[3],pReadMsg->pData[4],pReadMsg->pData[5]);
			return;
		}
		if( bIsStandard == true ) 
        {
        	uiResID = (unsigned int)(pData[0]*256) + pData[1];
			if(ucPACVType == CV)		    sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", 0, 0, pData[0], pData[1], pData[2]);
        	else /*if(ucPACVType == PA)*/ 	sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)
		}
        else
        {
            uiResID = (unsigned int)(pData[0]<<24) + (unsigned int)(pData[1]<<16) + (unsigned int)(pData[2]<<8) + pData[3];
			sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)
        }
//		ucTempPosDTC = g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_ucDTCIndex;		//보기 힘들어서 치환
		ucTempPosFreeze = g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_ucIndex;			//보기 힘들어서 치환
		
		if( strncmp( g_stActuatorFreezeData[ ucTempPosFreeze ].m_ucFreezeRes, (char const*)ucRescode, strlen(g_stActuatorFreezeData[ ucTempPosFreeze ].m_ucFreezeRes))==0 )
		{
			g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_usFreezeFuncType = g_stActuatorFreezeData[ ucTempPosFreeze ].m_uiFuncIndex;
			g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_ucState = 1;
			
			//혹시나 100byte 넘으면 100byte만 저장
			if( pReadMsg->DataSize > FREEZE_FRAME_DATA_MAX )
			{
				pReadMsg->DataSize = FREEZE_FRAME_DATA_MAX;
			}
			g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_usLength = pReadMsg->DataSize;
			memcpy( &g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_ucFreezeFrameData[0], pReadMsg->pData , pReadMsg->DataSize );
			g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_ucFreezeFrameData[pReadMsg->DataSize] = 0x00;
		}
	}
#endif
}

//CAN FD Channel 1 
void GitCANFDReadMsgs(stPASSTHRU_MSG * pReadMsg,unsigned int uiReadMsgLen,unsigned int eInCommType, boolean_t bIsStandard)
{
	eOBD_STATE eOBDState;
	int data=0;
	int i=0;
//	int cnt;
	U8 * pData;
	U8 ucRescode[20];
//	static U8  ucGarbageOdoCnt=0;
//	static INT16U OldRpm=0, s_uiOldMotorRpm = 0;
//	static U8 s_ucSavedPowerReady=0;
//	U8 ucBuff=0;
	int8_t scResult;
//	int nVinCnt=0;
	int iCnt=0;
//	float fTemp=0;
	unsigned int ucResID=0;
//	unsigned char ucTempPosFreeze = 0;
//	int iTPMS_Temp=0;

	eOBDState = GetOBDState();

	pData = &pReadMsg->pData[0];
	memset(ucRescode, 0x00, sizeof(ucRescode)/sizeof(ucRescode[0]));

	if(pReadMsg->pData[2]==0x7F)		//negative 처리
	{
		if(pReadMsg->pData[4]!=0x78) return;
	}

	if( eOBDState == eOBD_GetAutoVIN )
	{
		if(uiReadMsgLen>50)
			uiReadMsgLen = 0x00;		//cubis관련 버퍼 overflow 방지


#if 1 // James Jean 2018/11/14
		g_stAutoVin.ucSize = DCS_AUTOVIN_SIZE;

        if( g_eAutoVinReqFuelType == eAUTOVIN_ELECTIC_REQ )
        {
            memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[4], DCS_AUTOVIN_SIZE);
        }
		else if((g_eAutoVinReqFuelType == eAUTOVIN_HYDRO_REQ) || (g_eAutoVinReqFuelType == eAUTOVIN_ELECTIC_UDS_REQ))
        {
            memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[5], DCS_AUTOVIN_SIZE);
        }
        else/* ( (g_eAutoVinReqFuelType == eAUTOVIN_ENGINE_REQ) ||
                 (g_eAutoVinReqFuelType == eAUTOVIN_HYDRO_REQ) )*/
        {
            if(pReadMsg->pData[2] != 0x49 || pReadMsg->pData[3] != 0x02)
            {
                Trace(0,"AutoVIN Read Fail!!\r\n");
                //SetOBDState(eOBD_GetAutoVIN_Fail);
                return; //주행중 autovin data꼬임
            }
            // 인도 IB(컨티넨탈 디젤 엔진) 에서 부정응담이 온다.
			if(pReadMsg->pData[0] == 0x07 && pReadMsg->pData[2] == 0x7F/* || pReadMsg->pData[3] == 0x09 && pReadMsg->pData[4] == 0x12*/)
			{
			      // 부정응답 이후 07DF 02 09 00 00 00 00 00 00을 전달한 후 AutoVin을 읽어면 정상적으로 읽어 온다.
			      // 재시도시에 위 패킷을 전달하는 로직을 만듦
			      g_bAutoVinNegativeResponse = TRUE;
			      return;
			}
			else if(pReadMsg->pData[0] == 0x07 && pReadMsg->pData[2] == 0x49 && pReadMsg->pData[3] == 0x00 )
			{
			      return;
			}

            g_bAutoVinNegativeResponse = FALSE;
            memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[5], DCS_AUTOVIN_SIZE);
        }
#else

		if(pReadMsg->pData[2] != 0x49 || pReadMsg->pData[3] != 0x02)
		{
			Trace(0,"AutoVIN Read Fail!!\r\n");
			//SetOBDState(eOBD_GetAutoVIN_Fail);
			return;	//주행중 autovin data꼬임
		}
		g_stAutoVin.ucSize = DCS_AUTOVIN_SIZE;

        memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[5], DCS_AUTOVIN_SIZE);
#endif

		g_stAutoVin.strVinCode[DCS_AUTOVIN_SIZE] = 0x00;
		if( VIN_ValidCheck() == true )
		{
			g_stAutoVin.bRxStatus = true;	//CAN RX TRUE
		}
		else
		{
			g_stAutoVin.bRxStatus = false;
			Trace(0,"AutoVIN Read Fail!!2\r\n");
			return;
		}
#ifdef FIX_VIN
		memcpy(&g_stAutoVin.strVinCode, FIX_VIN_NUMBER, DCS_AUTOVIN_SIZE);
#endif
		Trace(0,"~~~~~~~~~~~~~~~~~~~~~GetAutoVIN : %s  ~~~~~~~~~~~~~~~~~~~~~\r\n",g_stAutoVin.strVinCode);

		printf("\n\n\n================================================\r\n");
		printf("AUTO VIN RESULT\n");
		hexdump(g_stAutoVin.strVinCode, DCS_AUTOVIN_SIZE);
//		for(nVinCnt=0; nVinCnt<DCS_AUTOVIN_SIZE; nVinCnt++ )
//			Trace(0,"%c", g_stAutoVin.strVinCode[nVinCnt]);
//		Trace(0,"\r\n");

		//SetOBDState(eOBD_GetAutoVIN_Success);
	}
	if(g_eActaveType == ACTUATOR_TYPE_WAKEUP){}	// 204라인 Wakeup 체크하려하는데 204에서 203의 IG 상태와 똑같은 CANID가 들어와서 순간 시동OFF로 체크됨 - 웨이크업시는 데이터 처리 안하도록 수정(203,204구분불가)
	else if( eOBDState == eOBD_Actuator_Mode )	//제어DB일때 타는곳
	{
		ucResID = (unsigned int)(pData[0]*256) + pData[1];

		if( ( GetActuatorStatus() != ACTUATOR_STATUS_CTRL_RUNNING ) || ( FUELTYPE_GET_STATE() == EV_NONE_READY ))	//러닝일때는 최대한 영향없게 하기위해
		{
			for( i=0; i<g_stActuatorReadyInfo.m_uiCount; i++ )
			{
				if( ucResID == g_stActuatorReadyData[ i ].m_uiResVal )	// Response 비교
				{
				  	if(( g_stActuatorReadyData[ i ].m_ucFDCanLine)== pReadMsg->RxStatus)
					{
						CurrentCalculate_V(i, &data, pData);
						scResult = Compute_Data_V(&g_stActuatorReadyData[ i ], data);
						if(scResult != -1) Current_ActuatorReady(g_stActuatorReadyData,i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH
					}
				}
			}
		}
		else
		{

		}
	}
	else  if( eOBDState == eOBD_Running_Info_Mode || eOBDState == eOBD_GetFuelLevelCheck || eOBDState == eOBD_GetSOH  || eOBDState == eOBD_GetBrakeJudder)	//진단DB일때 타는곳
	{
		if(pReadMsg->pData[0]==0x07 && pReadMsg->DataSize != g_uiCanReadMsgLength) {
			pReadMsg->pData[0]=0x07;
			Trace(0,"[DRV]D_Data size Miss!%02X %02X %02X %02X %02X %02X %02X\r\n",g_uiCanReadMsgLength,pReadMsg->DataSize,pReadMsg->pData[1],pReadMsg->pData[2],pReadMsg->pData[3],pReadMsg->pData[4],pReadMsg->pData[5]);
			return;
		}
		g_ucCanCommSucces++;

		ucResID = (unsigned int)(pData[0]*256) + pData[1];
		sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)

		for(i=0; i<g_usCurrentCnt; i++)
		{
			U8 ucResLength=0;
			if(g_pstCurrDataBase[i].m_cCommType == 'V')
				ucResLength = 4;
			else
			{
				ucResLength = strlen(g_pstCurrDataBase[i].m_cResNode);

				if(ucResLength > 10)
				{
					ucResLength = 10;
				}
			}
			if(strncmp((char const*)ucRescode, g_pstCurrDataBase[i].m_cResNode, ucResLength)==0)	// Response 비교
			{
				if((g_pstCurrDataBase[i].m_ucCanLine)== pReadMsg->RxStatus)	//SLAVEPARSING
				{
					CurrentCalculate(i, &data, pData);
					scResult = Compute_Data(&g_pstCurrDataBase[i], data);
					if(scResult != -1) Current_SlaveData(g_pstCurrDataBase, i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH	
				}
			}
		}

		if( eOBDState == eOBD_Running_Info_Mode )	//제어DB일때 타는곳
		{
			ucResID = (unsigned int)(pData[0]*256) + pData[1];

			if( ( GetActuatorStatus() != ACTUATOR_STATUS_CTRL_RUNNING ) || ( FUELTYPE_GET_STATE() == EV_NONE_READY ))	//러닝일때는 최대한 영향없게 하기위해
			{
				for( i=0; i<g_stActuatorReadyInfo.m_uiCount; i++ )
				{
					 if( ucResID == g_stActuatorReadyData[ i ].m_uiResVal )	// Response 비교
					{
						if(( g_stActuatorReadyData[ i ].m_ucFDCanLine)== pReadMsg->RxStatus)
						{
							CurrentCalculate_V(i, &data, pData);
							scResult = Compute_Data_V(&g_stActuatorReadyData[ i ], data);
							if(scResult != -1) Current_ActuatorReady(g_stActuatorReadyData,i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH
						}
					}
				}
			}
			else
			{
			}
		}
	}
	else if( eOBDState == eOBD_FCS_Start )
	{
		if(pReadMsg->pData[0]==0x07 && pReadMsg->DataSize != g_uiCanReadMsgLength)
		{
			pReadMsg->pData[0]=0x07;
			Trace(0,"[FCS]Data size Miss!%02X %02X %02X %02X %02X %02X %02X\r\n",g_uiCanReadMsgLength,pReadMsg->DataSize,pReadMsg->pData[1],pReadMsg->pData[2],pReadMsg->pData[3],pReadMsg->pData[4],pReadMsg->pData[5]);
		   return;
		}
		sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)

		for(i=0; i<g_ucDtcSystemCnt; i++)
		{
            if(g_pstDTCDataBase[i].m_cReadno == 2)
			{
				g_stDTCFuncData[i].m_ucUDSFlag = eDTC_CAN;
			}
			else if(g_pstDTCDataBase[i].m_cReadno == 3)
			{
				g_stDTCFuncData[i].m_ucUDSFlag = eDTC_UDS;
			}

			if(strncmp(g_pstDTCDataBase[i].m_cResNode, (char const*)ucRescode, strlen(g_pstDTCDataBase[i].m_cResNode))==0)
			{
				int iStartPos=0, iReadNum;

				iStartPos = g_pstDTCDataBase[i].m_cStartPos;
				iReadNum = g_pstDTCDataBase[i].m_cReadno;
				if(g_pstDTCDataBase[i].m_cReadno == 2)
				{
					g_stDTCFuncData[i].m_ucDTCCount = pData[3];	//DTC 갯수
				}
				else if(g_pstDTCDataBase[i].m_cReadno == 3)
				{
					g_stDTCFuncData[i].m_ucDTCCount = (uiReadMsgLen - (iStartPos - 1))/DCS_DTC_CODE_SIZE;	//DTC 갯수
				}

				do
				{
					for(iCnt=0; iCnt<g_stDTCFuncData[i].m_ucDTCCount; iCnt++)
					{
						for(int dtcloop=0; dtcloop<iReadNum+1; dtcloop++)
						{
							U8 ucCanBlank=0;
							if((g_stDTCFuncData[i].m_ucUDSFlag == eDTC_CAN) && dtcloop==iReadNum)
							{
								g_stDTCFuncData[i].m_ucDTCCode[(DCS_DTC_CODE_SIZE*iCnt) + dtcloop] = 0xFF;
								ucCanBlank=1;
							}
							g_stDTCFuncData[i].m_ucDTCCode[(DCS_DTC_CODE_SIZE*iCnt) + dtcloop + ucCanBlank] = pData[(iStartPos-1) +(((iReadNum+1)*iCnt)+dtcloop)];
						}
						if(iCnt * DCS_DTC_CODE_SIZE > DTC_DATA_MAX) break;	//DCT MAX 100
					}
				}while(pReadMsg->DataSize>(iStartPos-1)+((iReadNum+1)*iCnt));

				if(g_stDTCFuncData[i].m_ucDTCCount>0)
				{
					g_stDTCFuncData[i].m_ucState=2;	//DTC 있음
				}
				else
					g_stDTCFuncData[i].m_ucState=1;	//DTC 없음
			}
		}
	}
#ifndef NO_FREEZE_FRAME
	else if( eOBDState == eOBD_FreezeFrame )
	{
		if(pReadMsg->pData[0]==0x07 && pReadMsg->DataSize != g_uiCanReadMsgLength)
		{
			pReadMsg->pData[0]=0x07;
			Trace(0,"[FF]Data size Miss!%02X %02X %02X %02X %02X %02X %02X\r\n",g_uiCanReadMsgLength,pReadMsg->DataSize,pReadMsg->pData[1],pReadMsg->pData[2],pReadMsg->pData[3],pReadMsg->pData[4],pReadMsg->pData[5]);
			return;
		}
		sprintf((char*)ucRescode, "%02X%02X%02X%02X%02X", pData[0], pData[1], pData[2], pData[3], pData[4]);		//respons copy(ASCII)

//		ucTempPosDTC = g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_ucDTCIndex;		//보기 힘들어서 치환
		ucTempPosFreeze = g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_ucIndex;			//보기 힘들어서 치환

		if( strncmp( g_stActuatorFreezeData[ ucTempPosFreeze ].m_ucFreezeRes, (char const*)ucRescode, strlen(g_stActuatorFreezeData[ ucTempPosFreeze ].m_ucFreezeRes))==0 )
		{
			g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_usFreezeFuncType = g_stActuatorFreezeData[ ucTempPosFreeze ].m_uiFuncIndex;
			g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_ucState = 1;

			//혹시나 100byte 넘으면 100byte만 저장
			if( pReadMsg->DataSize > FREEZE_FRAME_DATA_MAX )
			{
				pReadMsg->DataSize = FREEZE_FRAME_DATA_MAX;
			}
			g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_usLength = pReadMsg->DataSize;
			memcpy( &g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_ucFreezeFrameData[0], pReadMsg->pData , pReadMsg->DataSize );
			g_stActuatorFreezeInfo[ g_ucSystemIndexFFinfo ].m_stFreezeFrame[g_ucDTCIndex].m_ucFreezeFrameData[pReadMsg->DataSize] = 0x00;
		}
	}
#endif
}

//채널2번으로 들어오는 데이터
void Git_LCANReadMsgs(stPASSTHRU_MSG *pReadMsg, unsigned int uiReadMsgLen, unsigned int eInCommType, boolean_t bIsStandard)
{
	eOBD_STATE eOBDState;
	int data=0;
	int i=0;
	U8 * pData;
	U8 ucRescode[20];
	int8_t scResult;
	unsigned int uiResID=0;
	uint8_t ucPACVType=0;
	GetAutolinkConfigProperty(eAutoLinkConfig_PACVType,(void*)&ucPACVType);

	eOBDState = GetOBDState();

	pData = &pReadMsg->pData[0];
	memset(ucRescode, 0x00, sizeof(ucRescode)/sizeof(ucRescode[0]));
	
	if( eOBDState == eOBD_GetAutoVIN )
	{
		if(uiReadMsgLen>50)
			uiReadMsgLen = 0x00;		//cubis관련 버퍼 overflow 방지

		g_stAutoVin.ucSize = DCS_AUTOVIN_SIZE;
		
		if(ucPACVType == CV)
		{
			if(g_eCVAutoVinReqFuelType == eAUTOVIN_CV_UNIVERSE_REQ)
			{
				// REQ : 18 DA 00 F9 04 21 41 01 01
	            // RES : 18 DA F9 00 10 CA 61 41 DATA1~ DATA200 
				memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[65], DCS_AUTOVIN_SIZE);
			}
			else if(g_eCVAutoVinReqFuelType == eAUTOVIN_CV_ELECCITY_REQ)
			{
				// REQ : 18 DA EF F9 03 22 F1 90
	            // RES : 18 DA F9 EF 10 14 62 F1 90 DATA1~ DATA17
				memcpy(&g_stAutoVin.strVinCode, &pReadMsg->pData[7], DCS_AUTOVIN_SIZE);
			}
        }

		g_stAutoVin.strVinCode[DCS_AUTOVIN_SIZE] = 0x00;
		if( VIN_ValidCheck() == true )
		{
			g_stAutoVin.bRxStatus = true;	//CAN RX TRUE
		}
		else
		{
			g_stAutoVin.bRxStatus = false;
			Trace(0,"AutoVIN Read Fail!!2\r\n");
			return;
		}
#ifdef FIX_VIN
		memcpy(&g_stAutoVin.strVinCode, FIX_VIN_NUMBER, DCS_AUTOVIN_SIZE);
#endif
		Trace(0,"~~~~~~~~~~~~~~~~~~~~~GetAutoVIN : %s  ~~~~~~~~~~~~~~~~~~~~~\r\n",g_stAutoVin.strVinCode);

		printf("\n\n\n================================================\r\n");
		printf("AUTO VIN RESULT\n");
		hexdump(g_stAutoVin.strVinCode, DCS_AUTOVIN_SIZE);
	}
	else if(g_eActaveType == ACTUATOR_TYPE_WAKEUP){}	// 204라인 Wakeup 체크하려하는데 204에서 203의 IG 상태와 똑같은 CANID가 들어와서 순간 시동OFF로 체크됨 - 웨이크업시는 데이터 처리 안하도록 수정(203,204구분불가)
	else if( eOBDState == eOBD_Actuator_Mode || eOBDState == eOBD_Running_Info_Mode )
	{
		if( bIsStandard == true ) 
        {
            uiResID = (unsigned int)(pData[0]*256) + pData[1];
		}
        else
        {
            uiResID = (unsigned int)(pData[0]<<24) + (unsigned int)(pData[1]<<16) + (unsigned int)(pData[2]<<8) + pData[3];
        }

		if( GetActuatorStatus() != ACTUATOR_STATUS_CTRL_RUNNING )	//러닝일때는 최대한 영향없게 하기위해
		{
			for( i=0; i<g_stActuatorReadyInfo.m_uiCount; i++ )
			{
				if( uiResID == g_stActuatorReadyData[ i ].m_uiResVal )	// Response 비교
				{
					if(CFD_GetCanFDAdapter())
					{
						if(( g_stActuatorReadyData[ i ].m_ucFDCanLine)== pReadMsg->RxStatus)
						{
							CurrentCalculate_V(i, &data, pData);
							scResult = Compute_Data_V(&g_stActuatorReadyData[ i ], data);
							if(scResult != -1) Current_ActuatorReady(g_stActuatorReadyData,i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH
						}
					}
					else
					{
						if( ((g_stActuatorReadyData[ i ].m_ucCanLine == HIGHCAN3) || (g_stActuatorReadyData[ i ].m_ucCanLine == LOWCAN1)) && (pReadMsg->RxStatus==CAN_CH2))
						{
							CurrentCalculate_V(i, &data, pData);
							scResult = Compute_Data_V(&g_stActuatorReadyData[ i ], data);
							if(scResult != -1) Current_ActuatorReady(g_stActuatorReadyData,i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH
						}
					}
				}
			}
            if( FUELTYPE_GET_STATE() == FCEV )
            {
                for( i=0; i<g_ucTotResCnt; i++ )
                {
					if( uiResID == g_stActuatorResData[ i ].m_uiResVal )	// Response 비교
					{
						if(CFD_GetCanFDAdapter())
						{
							//if(( g_stActuatorResData[ i ].m_ucFDCanLine)== pReadMsg->RxStatus)		// 210302 lwh 아답터가 있는데 lowcan 으로 셋팅하면 처리 불가될 것이다. 그래서 linecheck 사용하지 않는다
							{
								  CurrentCalculate_V2(i, &data, pData);
								  scResult = Compute_Data_V2(&g_stActuatorResData[ i ], data);
								  if(scResult != -1) Current_ActuatorRes(g_stActuatorResData,i);	//예외처리된 데이터는 처리하지 않는다 170218 LWH
							}
						}
						else
						{
							// 210302 lwh 204와 203을 구분하지 못한다. 추후 res 데이터 외에 다른 데이터도 구분을 못할 수도 있다. 중요한건 ch1 data와 ch2 데이터를 구분하는 것이다.
							if( ((g_stActuatorReadyData[ i ].m_ucCanLine == HIGHCAN3) || (g_stActuatorReadyData[ i ].m_ucCanLine == LOWCAN1)) && (pReadMsg->RxStatus==CAN_CH2))
							{
								CurrentCalculate_V2(i, &data, pData);
								scResult = Compute_Data_V2(&g_stActuatorResData[ i ], data);
								if(scResult != -1) Current_ActuatorRes(g_stActuatorResData,i); 	//예외처리된 데이터는 처리하지 않는다 170218 LWH
							}
						}
					}
                }
            }
			
		}
		else
		{

		}
	}
}

void PassThruReadMsgs(stPASSTHRU_MSG *pReadMsg, unsigned int uiReadMsgLen, unsigned int eInCommType, boolean_t bIsStandard)
{
	if(uiReadMsgLen == 0) {
		if(pReadMsg->RxStatus == eCOMM_TYPE_CAN2)	  LCAN_SET_COMM_STATE(eLCAN_RX_FAIL);
		else                                DCAN_SET_COMM_STATE(eCAN_RX_FAIL);
		pReadMsg->DataSize = 0;
		return;
	}
	else 
	{
		if(CFD_GetCanFDAdapter())
		{
			GitCANFDReadMsgs(pReadMsg, uiReadMsgLen, eInCommType, bIsStandard);
		}
		else
		{
			if(pReadMsg->RxStatus == eCOMM_TYPE_CAN2)	Git_LCANReadMsgs(pReadMsg, uiReadMsgLen, eInCommType, bIsStandard);
			else 							                             	GitCANReadMsgs(pReadMsg, uiReadMsgLen, eInCommType, bIsStandard);
		}

		if(bIsStandard)
		{
		if(pReadMsg->pData[0]>=0x07)	DCAN_SET_COMM_STATE(eCAN_RX_DONE);	//진단캔이면 RES받고 REQ를해야하므로 RX_DONE 상태로 변경,비클캔이면 RX_BLOCK에 계속있어야함
		}
		else
		{
			
			if((pReadMsg->pData[0]==0x18) && (pReadMsg->pData[1]==0xDA)) 
			{
				if(pReadMsg->RxStatus == eCOMM_TYPE_CAN2)	
				{
					LCAN_SET_COMM_STATE(eLCAN_RX_DONE);
				}
				else
				{
					DCAN_SET_COMM_STATE(eCAN_RX_DONE);
				}
			}
		}

		memset(pReadMsg, 0x00, sizeof(stPASSTHRU_MSG));
		pReadMsg->DataSize = 0;
	}
}

void Mileage(unsigned char ucType)
{
	float fTemp;
	
	if( ucType == ELECTRONIC_TYPE )
	{
		if( (Get_driving_distance_int()) != 0 && Get_Total_Energy_Consume() != 0.0)
		{
			fTemp = (float)(Get_driving_distance() /1000.0) / (Get_Total_Energy_Consume());
			Send_ElectronicMileage_Avg(fTemp);
		}
	}
	else
	{
		if( (Get_driving_distance_int()) != 0 && Get_Total_Fuel_Consume() != 0.0)
		{
			fTemp = (float)(Get_driving_distance() /1000.0) / (Get_Total_Fuel_Consume());		//평균연비 km/L
			Send_Mileage_Avg(fTemp);
		}
	}
}

float DCS_FuncType(float nDataA, float nDataB, unsigned long ulCnt, int funcType)
{
	float result=0;

	switch(funcType)
	{
	case 0:	//그대로 표출
		result=nDataA;
		break;
	case 1:	//합산
		result=nDataA + nDataB;
		break;
	case 2:	//MAX 값
		if(nDataA > nDataB)
			result=nDataA;
		else
			result=nDataB;
		break;
	case 3:	//평균 값
		nDataA += nDataB;
		result = nDataA / ulCnt;
		break;
	case 4:	//
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:	//
		break;
	case 8:		//
		break;
	case 9:	//
		break;
	case 11:	//
		break;

	default:
		break;
	}
	return result;
}

void CalOdoMeter_CARB()
{
//	if(g_stStaticOBDdata.s_bStartState == FALSE)
//	{
//		g_stStaticOBDdata.s_uiStartOdoMeter = g_uiDistanceafterClearDTC;
//		g_stStaticOBDdata.s_uiOldOdoMeter = g_uiDistanceafterClearDTC;
//		g_stStaticOBDdata.s_bStartState = TRUE;
//	}
//	if(g_stStaticOBDdata.s_bStartState == TRUE)
//	{
//		//data -= g_stStaticOBDdata.s_uiStartOdoMeter;
//		//g_stFastFuncData.m_nOdometer_a = data - g_stStaticOBDdata.s_uiStartOdoMeter - g_stStaticOBDdata.g_nOffset2;
//	}

//	if(g_uiDistanceafterClearDTC != g_stStaticOBDdata.s_uiOldOdoMeter)
//	{
//		g_stStaticOBDdata.s_uiOldOdoMeter = g_uiDistanceafterClearDTC;
//
//		if(g_uiDistanceafterClearDTC >= g_stStaticOBDdata.s_uiStartOdoMeter)
//		{
//			g_stStaticOBDdata.g_fOdometer_run = (g_uiDistanceafterClearDTC - g_stStaticOBDdata.s_uiStartOdoMeter) * 1000 - g_stStaticOBDdata.g_nOffset2 -  g_stStaticOBDdata.g_nOffset;
//			g_stFastFuncData.m_nOdometer_a = (INT32U)g_stStaticOBDdata.g_fOdometer_run;
//			g_stFastFuncData.m_nOdometer_r = g_stSaveOBDdata.m_nOdometer_rev + g_stFastFuncData.m_nOdometer_a;
//			//Trace(0,"[Distance] Odometer_run %f | StartOdoMeter %u | Odometer_t %u | Offset2 %d | Offset %d \r\n", g_stStaticOBDdata.g_fOdometer_run, g_stStaticOBDdata.s_uiStartOdoMeter, g_uiDistanceafterClearDTC, g_stStaticOBDdata.g_nOffset2, g_stStaticOBDdata.g_nOffset);
//		}
//		else
//		{
//			Trace(0,"[OdoMeter ERROR!!] ODO : %u \r\n", g_uiDistanceafterClearDTC);
//		}
//	}
}

void CalOdoMeter()
{
// 지금은 실제로 ODO를 최초받았을때 저장
//	if(g_stDistanceInfo.m_bStartState == FALSE)	//이함수탄게 최초면??
//	{
//		g_OBDControllerData.basicData.startOdometer = g_uiDistanceafterClearDTC;
//		g_OBDControllerData.s_uiOldOdoMeter = g_uiDistanceafterClearDTC;
//		g_stDistanceInfo.m_bStartState = TRUE;
//	}

	if(g_uiDistanceafterClearDTC != g_OBDControllerData.basicData.m_uiOldOdometer)
	{
#ifdef DISTANCE
		Trace(0,"[CalOdoMeter] Odo : %d DTC : %d \r\n",g_OBDControllerData.basicData.m_uiOldOdometer, g_uiDistanceafterClearDTC);
#endif

		g_OBDControllerData.basicData.m_uiOldOdometer = g_uiDistanceafterClearDTC;

		if(g_uiDistanceafterClearDTC >= g_OBDControllerData.basicData.m_uiStartOdometer)
		{
			g_stDistanceInfo.m_fOdometer_run = (g_uiDistanceafterClearDTC - g_OBDControllerData.basicData.m_uiStartOdometer) * 1000 - g_stDistanceInfo.m_nOffset2 -  g_stDistanceInfo.m_nOffset;

#ifdef DISTANCE
			Trace(0,"[CalOdoMeter2] run : %f = DTD: %d - Start : %d off2:%d - off:%d\r\n",g_stDistanceInfo.m_fOdometer_run,
				                                                                                                              g_uiDistanceafterClearDTC,
																															  g_OBDControllerData.basicData.m_uiStartOdometer,
																															  g_stDistanceInfo.m_nOffset2 ,
																															  g_stDistanceInfo.m_nOffset);
#endif
			Send_Driving_Distance_From_CAN(g_stDistanceInfo.m_fOdometer_run);
                        //g_stFastFuncData.m_nOdometer_r = g_stSaveOBDdata.m_nOdometer_rev + g_stFastFuncData.m_nOdometer_a;
			//g_uiOdometer_total = g_stSaveOBDdata.m_nOdometer_Calc + g_stFastFuncData.m_nOdometer_a;


                        //Trace(0,"[Distance] CARB Odometer_run %f | StartOdoMeter %u | Odometer_t %u | Offset2 %d | Offset %d \r\n", g_stStaticOBDdata.g_fOdometer_run, g_stStaticOBDdata.s_uiStartOdoMeter, g_uiDistanceafterClearDTC, g_stStaticOBDdata.g_nOffset2, g_stStaticOBDdata.g_nOffset);
			//Trace(0,"[Distance] g_uiOdometer_total %d | Odometer_Calc %d | Odometer_a %d |  \r\n",g_uiOdometer_total,  g_stSaveOBDdata.m_nOdometer_Calc, g_stFastFuncData.m_nOdometer_a  );
		}
		else
		{
			Trace(0,"[OdoMeter ERROR!!] ODO : %u \r\n", g_uiDistanceafterClearDTC);
		}
	}
}

void CalFuelConsumption(double data, U8 type)
{
	U32 uiNewTimer=0, uiDeltaTimer=0;
	double ConsumptionTmp=0;
	double cnt=0;
	//1리터	l(ℓ) -> 1000ml -> 1000000μL

	uiNewTimer 			= Get_Tmr();
	uiDeltaTimer	= Get_TmrDelta(uiNewTimer,g_uiCalFuelConsumptionOldTime);
	g_uiCalFuelConsumptionOldTime  = uiNewTimer;

	switch(type)		//Feul Type
	{
	case FUEL_TYPE:
		g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume = data;
		g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume += g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume;		//누적 소모량 적산
//		Trace(0,"P : %f %f\r\n",g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume,g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume);
		break;
	case ELECTRONIC_TYPE:
		g_OBDControllerData.monitering_Data.m_fPeriod_Energy_Consume = data;
		g_OBDControllerData.monitering_Data.m_fTotal_Energy_Consume += g_OBDControllerData.monitering_Data.m_fPeriod_Energy_Consume;		//누적 소모량 적산
#if defined(PROTOCOL17)
		if( data > 0 )
		{
		 	g_OBDControllerData.monitering_Data.m_fPeriod_Energy_Consume_Regen = data;
			g_OBDControllerData.monitering_Data.m_fTotal_Energy_Consume_Regen += g_OBDControllerData.monitering_Data.m_fPeriod_Energy_Consume_Regen;		//누적 소모량 적산
		}
#endif
//		Trace(0,"P : %f %f\r\n",g_OBDControllerData.monitering_Data.m_fPeriod_Energy_Consume,g_OBDControllerData.monitering_Data.m_fTotal_Energy_Consume);
		break;
	default:
		//general - 10ms 1C 1C 1C 1C
//		//순간소모량 기준으로 한시간썼을때의 양
//		g_OBDControllerData.monitering_Data.m_fFlashFuel_Consume = data * 360000.0;			//10ms -> 1Hour
//		//한시간썼을때의 양(L)
//		g_OBDControllerData.monitering_Data.m_fFlashFuel_Consume = g_OBDControllerData.monitering_Data.m_fFlashFuel_Consume / 1000000.0;	//μL -> L환산
		//총소모량(적산)
//		g_OBDControllerData.monitering_Data.m_fFlashFuel_Consume = data;	//μL	//사용여부 모르겠으나 일단 저장
		if(FUELTYPE_GET_STATE() == FCEV || g_bIndicatorDBFlag == true ) //m_cE가 아닌 Fuel Type 으로 처리 
		{			
			ConsumptionTmp = data / 1000000.0;						//mg -> kg 환산
		}
		else
		{
			cnt = uiDeltaTimer / 10.0;	//10ms 단위
			ConsumptionTmp = data / 1000000.0;						//μL -> L환산
			ConsumptionTmp = ConsumptionTmp * cnt;			//delta Time
		}
		//한번받고 다음번 받았을때 까지 쓴 기름량(방금받은양을 기준으로 계산)
		g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume = ConsumptionTmp;
		g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume += g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume;//누적 소모량 적산
		break;
	}
	if(g_bFuelLiterTypeFlag == true)
	{
		g_dChecked_Fuel_Consume += g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume;
	}
//	Trace(0,"P : %lf %lf\r\n",g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume,g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume);
}


void energyConsume()
{
	unsigned int uiFuelDeltaTime=0;
	float fTemp = 0;
	
	uiFuelDeltaTime = Get_TmrDelta(Get_Tmr(),g_uiEnergyOldTime);
	g_uiEnergyOldTime=Get_Tmr();
	
	fTemp = (float)((Get_BatteryPackV_Status() * Get_BatteryPackC_Status()) *(uiFuelDeltaTime/3600.0)/1000000.0); //genebe25 test용	
	
	Send_Fuel_Consume_From_CAN((double)fTemp,ELECTRONIC_TYPE);
}

float GetFuelConsumption(eConsumptionType eFuelConsumMode)
{
	if(eFuelConsumMode == eConsum_Instant)
		return g_OBDControllerData.monitering_Data.m_fPeriod_Fuel_Consume;
	else if(eFuelConsumMode == eConsum_Total)
		return g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume;
	else
		return 0.0;
}

U16 CalGearInfo(U8 ucGearPos, U8 ucGearLever)
{
	U16 usGear=0;

	if(ucGearLever == 'P' || ucGearLever == 'R' || ucGearLever == 'N')
		ucGearPos=0x00;

	usGear =  ucGearPos | (ucGearLever<<8);

	return usGear;
}


void SetOffset()	//ODO값을 얻어와 옵셋을 계산할 시점을 잡아주는 함수
{
	static U32 s_uiStartDistnace = 0;		//시작 distance
	//static U8 ucDistanceCnt=0;
	unsigned int uiStandardDistance=0;

	uiStandardDistance=1;	//1km(1000m)

	if(OFFSET_GET_STATE() == GET_DISTANCE /*&& ucDistanceCnt>5*/)
	{
		//ucDistanceCnt=0;
		s_uiStartDistnace = Get_Odmeter(); //초기값	ex)4901000
#ifdef DISTANCE
		Trace(0,"SetOffset:%d \r\n", s_uiStartDistnace);
#endif
		OFFSET_SET_STATE(GET_MILEAGE);
	}
	// 4901000 + 1000 <= 4902000
	if( (s_uiStartDistnace + uiStandardDistance) <= Get_Odmeter() )     //ODO가 기준단위만큼(1000m)변한시점
	{
#ifdef DISTANCE
//		Trace(0,"SetOffset2:%d %d %d\r\n", s_uiStartDistnace,uiStandardDistance,Get_Odmeter());
#endif
		if(OFFSET_GET_STATE()==GET_MILEAGE)
		{
			OFFSET_SET_STATE(GET_OFFSET);
		}

		if(s_uiStartDistnace+(uiStandardDistance*2)<=Get_Odmeter() && (OFFSET_GET_STATE()==GETED_OFFSET))
		{
			OFFSET_SET_STATE(END_OFFSET);
		}
	}
}

void Distance(void)	//속도받을때호출
{
  	//이동거리, 차량 총 이동거리, 연비 리셋 후 이동거리
  	//이동거리는 차속 과 delta 시간
	unsigned int uiVSSDeltaTime=0;
	INT8U  m_ucSpeed = Get_Speed();
	float	fDriving_Distance;

	uiVSSDeltaTime = Get_TmrDelta(Get_Tmr(),g_uiVSSOldTime);
	g_uiVSSOldTime=Get_Tmr();
	// 																		10000m/h * 200ms / 3600*1000
	// 거리 = 속력*시간			시속/(3600*1000) = ms속 		ms속 * 델타타임 = 거리
	fDriving_Distance = Get_driving_distance() + (m_ucSpeed *1000.0*uiVSSDeltaTime/(3600.0*1000.0));		//주행 거리 m - ms단위변동시간
#ifdef DISTANCE
	Trace(0,"Dis:%f Time:%d \r\n",fDriving_Distance, uiVSSDeltaTime);
#endif
	Send_Driving_Distance_From_CAN(fDriving_Distance);

	//g_stFastFuncData.m_nOdometer_a   = (INT32U)g_stStaticOBDdata.g_fOdometer_run;
	//g_stFastFuncData.m_nOdometer_r = g_stSaveOBDdata.m_nOdometer_rev + g_stFastFuncData.m_nOdometer_a;


	if(OFFSET_GET_STATE()==GET_OFFSET)	//ODO가 기준단위만큼(1km) 변했을때
	{
		g_stDistanceInfo.m_uiOldDistance=Get_driving_distance_int();		//시작 distanc -> 1 증가
		OFFSET_SET_STATE(GETED_OFFSET);

		//1000 - 최초오도가 바꼈을때의 모듈이 계산한 거리
		g_stDistanceInfo.m_nOffset2=1000-g_stDistanceInfo.m_uiOldDistance;	//시작 distance보정값
#ifdef DISTANCE
		Trace(0,"[GET_OFFSET] uiOldDistance %u | Offset2 %d\r\n", g_stDistanceInfo.m_uiOldDistance,  g_stDistanceInfo.m_nOffset2);
#endif
	}
	else if(OFFSET_GET_STATE()==END_OFFSET)
	{
		//두번째오도가 바꼈을시점의 거리 - 최초오도가 바꼈을때의 모듈이 계산한 거리
		g_stDistanceInfo.m_nOffset = Get_driving_distance_int() - g_stDistanceInfo.m_uiOldDistance;
#ifdef DISTANCE
		Trace(0,"[END_OFFSET] %d\r\n", g_stDistanceInfo.m_nOffset);
#endif
		if(g_stDistanceInfo.m_nOffset >= 1000)
			g_stDistanceInfo.m_nOffset = g_stDistanceInfo.m_nOffset % 1000;
		else
			g_stDistanceInfo.m_nOffset = g_stDistanceInfo.m_nOffset - 1000;
		OFFSET_SET_STATE(NONE_OFFSET);
#ifdef DISTANCE
		Trace(0,"[END_OFFSET2] %d\r\n", g_stDistanceInfo.m_nOffset);
#endif

	}

	if( g_stDistanceInfo.m_bOdometerRcv == TRUE )	//ODO를 받았다면
		CalOdoMeter();		//시동후이동거리

}


void VSS_1SecCallback()
{
	g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_tempVSSoldPosition] = Get_Speed();

	if(g_stStaticOBDdata.s_tempVSSoldPosition <ARRAY_VSS_CNT)	g_stStaticOBDdata.s_thisposition = g_stStaticOBDdata.s_tempVSSoldPosition+ ARRAY_VSS_MAX - ARRAY_VSS_CNT;
	else 																						g_stStaticOBDdata.s_thisposition = g_stStaticOBDdata.s_tempVSSoldPosition -ARRAY_VSS_CNT ;

	if( g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_tempVSSoldPosition] > g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_thisposition] ) //가속  =>
	{
		g_stStaticOBDdata.s_oldRetardcnt = 0;
//		if(g_stStaticOBDdata.s_oldAccelcnt ==0)
		{
			g_stStaticOBDdata.s_ucDiffVss = g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_tempVSSoldPosition] - g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_thisposition];

			if( (g_stStaticOBDdata.s_ucDiffVss >= DEFAULT_RAPID_ACCEL_DECEL_DIFF) //||
//			   	((g_stStaticOBDdata.s_ucDiffVss >= 10)&&(g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_tempVSSoldPosition]>80)) ||	베이직서PD예외처리
//			   	((g_stStaticOBDdata.s_ucDiffVss >= 14)&&(g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_tempVSSoldPosition]>40)) ||
//			   	((g_stStaticOBDdata.s_ucDiffVss >= 17)&&(g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_tempVSSoldPosition]<=40)))
				)
			{
				if(g_stStaticOBDdata.s_oldAccelcnt%3 ==0)	Increase_period_Rapid_acceleration_Counter();
				g_stStaticOBDdata.s_oldAccelcnt++;
				//GITDebugPrintf("1Acceleration_t0 %d %d \r\n",ucDiffVss,g_stFastFuncData.m_ucSpeed);
			}
			else 	g_stStaticOBDdata.s_oldAccelcnt = 0;
//			if(g_stStaticOBDdata.s_ucDiffVss >= DEFAULT_RAPID_ACCEL_DECEL_DIFF)
//			{
//				g_stStaticOBDdata.s_oldAccelcnt++;
//				Increase_period_Rapid_acceleration_Counter();
//				//Trace(0,"1Acceleration_t0 %d %d \r\n",g_stStaticOBDdata.s_OldVSS,g_stFastFuncData.m_ucSpeed);
//			}
		}
	}
	else	//감속
	{
		g_stStaticOBDdata.s_oldAccelcnt = 0;
//		if(g_stStaticOBDdata.s_oldRetardcnt ==0)
		{
			g_stStaticOBDdata.s_ucDiffVss = g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_thisposition] - g_stStaticOBDdata.s_arrVSSold[g_stStaticOBDdata.s_tempVSSoldPosition];

			if( g_stStaticOBDdata.s_ucDiffVss > DEFAULT_RAPID_ACCEL_DECEL_DIFF+1 )
			{
			  	if(g_stStaticOBDdata.s_oldRetardcnt%3 ==0)	Increase_period_Rapid_Deceleration_Counter();
			  	g_stStaticOBDdata.s_oldRetardcnt++;
			}
			else 	g_stStaticOBDdata.s_oldRetardcnt = 0;
//			if(g_stStaticOBDdata.s_ucDiffVss >= DEFAULT_RAPID_ACCEL_DECEL_DIFF)
//			{
//			  	g_stStaticOBDdata.s_oldRetardcnt++;
//				Increase_period_Rapid_Deceleration_Counter();
//				//Trace(0,"1Retardation_t0 %d  %d \r\n",g_stStaticOBDdata.s_OldVSS,g_stFastFuncData.m_ucSpeed);
//			}
		}
	}
	g_stStaticOBDdata.s_tempVSSoldPosition++;
	g_stStaticOBDdata.s_tempVSSoldPosition = g_stStaticOBDdata.s_tempVSSoldPosition%ARRAY_VSS_MAX;

        /*
        //////////////// 운행 시간 ///////////////////////////////////////////////////////////////////

		if(( g_stFastFuncData.m_usRpm !=0)||( ISENGON(g_stSlow3FuncData.m_ucPower_Switch)))
		{
			g_stStaticOBDdata.s_EngineRunningTime++;
			g_stStaticOBDdata.g_ucFcsTime++;
			if(g_stStaticOBDdata.g_ucFcsTime > 200)
				g_stStaticOBDdata.g_ucFcsTime=200;
		}
		//if( g_stFastFuncData.m_usRpm !=0 || g_stSlow3FuncData.m_ucIsg_State == 1) g_stStaticOBDdata.s_EngineRunningTime++;
		g_stFastFuncData.m_usEngine_run_t = g_stStaticOBDdata.s_EngineRunningTime;

        */
	
	if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed == 0)																												g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_0++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >=1 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 10)		g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_1_under_10++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 11 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 20)		g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_11_under_20++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 21 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 30)		g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_21_under_30++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 31 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 40)		g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_31_under_40++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 41 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 50)		g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_41_under_50++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 51 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 60)		g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_51_under_60++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 61 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 70)		g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_61_under_70++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 71 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 80)		g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_71_under_80++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 81 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 90)		g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_81_under_90++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 91 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 100)	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_91_under_100++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 101 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 110)	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_101_under_110++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 111 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 120)	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_111_under_120++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 121 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 130)	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_121_under_130++;
	else if (g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 131 && g_OBDControllerData.monitering_Data.m_usCurrentSpeed <= 140)	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_from_131_under_140++;
	else if(g_OBDControllerData.monitering_Data.m_usCurrentSpeed >= 141)																									 	g_OBDControllerData.monitering_Data.m_uiPeriod_Speed_Counter_over_141++;
	
	if( (FUELTYPE_GET_STATE() == FCEV) || (FUELTYPE_GET_STATE() == ELECTRONIC) || (FUELTYPE_GET_STATE() == GASOLINE_HEV) || (FUELTYPE_GET_STATE() == PLUGIN_HEV) || (FUELTYPE_GET_STATE() == EV_NONE_READY) )
	{		
		/////////////// 워밍업 시간 ///////////////////////////////////////////////////////////////////
		if(g_bWarmupFlag == TRUE &&  Get_EngRun_Status()>eENGRUN_OFF)
		{
			if(Get_Speed() == 0)
			{
				Increase_WarmupTime();
			}
			else if(Get_Speed() > 0)
			{
				g_bWarmupFlag = FALSE;
				g_bEngineIdleFlag = TRUE;
			}
			else
			{
				Trace(0," Warming up / Get_Speed() ERROR \r\n");
			}
		}
		/////////////// 가솔린 차량 공회전 시간 ///////////////////////////////////////////////////////////////////
		if( (Get_Speed()==0 && Get_EngRun_Status()>eENGRUN_OFF ) && g_bEngineIdleFlag==TRUE)
		{
			Increase_EngineIdleTime();
		}
		
		if( Get_EngRun_Status()>eENGRUN_OFF )
		{
			g_ui1secTimer++;
#if defined(QA_FIFA)
			if( (Get_GPS_Lat()!=0) && (Get_GPS_Lon()!=0) && (Get_GPS_Vailication() >= NMEA_SIG_FIXED_2D_3D) )
#else
			if( ((g_ui1secTimer % MAX_GPS_SAMPLE_TIME ) == 0) && (Get_GPS_Lat()!=0) && (Get_GPS_Lon()!=0) && (Get_GPS_Vailication() >= NMEA_SIG_FIXED_2D_3D) )
#endif
			{
				if( g_OBDControllerData.monitering_Data.GpsListCount<MAX_GPS_LIST_COUNT )
				{
					g_OBDControllerData.monitering_Data.GpsLatitudeList[g_OBDControllerData.monitering_Data.GpsListCount]=Get_GPS_Lat();
					g_OBDControllerData.monitering_Data.GpsLongitudeList[g_OBDControllerData.monitering_Data.GpsListCount]=Get_GPS_Lon();
					g_OBDControllerData.monitering_Data.GpsListCount++;
				}
				else
				{
	//				printf("MAX_GPS_LIST_COUNT OVER\r\n");
				}
			}
		}
		
		//패턴0 차량 공회전 시간 (속도 ==0 && RPM > 0 && 풋브레이크 == 0)
		if( (Get_Speed()==0) && (Get_EngRun_Status()>eENGRUN_OFF) && (Get_FootBrake_State()==0) )
		{
			Increase_DrivePattern0();
		}
		//패턴1 정차 누적 시간  (속도 == 0 &&  RPM > 0 && 풋브레이크 ==1
		else if( (Get_Speed()==0) && (Get_EngRun_Status()>eENGRUN_OFF) && (Get_FootBrake_State()==1) )
		{
			Increase_DrivePattern1();
		}
		//패턴2 제동 누적 시간 (속도 > 0  && RPM > 0 && 풋브레이크 ==1
		else if( (Get_Speed()>0) && (Get_EngRun_Status()>eENGRUN_OFF) && (Get_FootBrake_State()==1) )
		{
			Increase_DrivePattern2();
		}
		//패턴3 관성 주행 (속도 > 0 && RPM >0 && 풋 브레이크 == 0 && Acc_Pos ==0)
		else if( (Get_Speed()>0) && (Get_EngRun_Status()>eENGRUN_OFF) && (Get_FootBrake_State()==0) && (Get_AccelPos()==0))
		{
			Increase_DrivePattern3();
		}
		//패턴4 일반 주행 (속도  > 0 && RPM > 0 &&풋 브레이크 ==0 && Acc_Pos >0)
		else if( (Get_Speed()>0) && (Get_EngRun_Status()>eENGRUN_OFF) && (Get_FootBrake_State()==0) && (Get_AccelPos()>0))
		{
			Increase_DrivePattern4();
		}
#if defined(PROTOCOL17)
		else
		{
			Increase_DrivePattern5();
		}
#endif
	}
	else
	{
		/////////////// 워밍업 시간 ///////////////////////////////////////////////////////////////////
		if(g_bWarmupFlag == TRUE &&  Get_RPM()>0)
		{
			if(Get_Speed() == 0)
			{
				Increase_WarmupTime();
			}
			else if(Get_Speed() > 0)
			{
				g_bWarmupFlag = FALSE;
				g_bEngineIdleFlag = TRUE;
			}
			else
			{
				Trace(0," Warming up / Get_Speed() ERROR \r\n");
			}
		}
		/////////////// 가솔린 차량 공회전 시간 ///////////////////////////////////////////////////////////////////
		if( (Get_Speed()==0 && Get_RPM()>0 ) && g_bEngineIdleFlag==TRUE)
		{
			Increase_EngineIdleTime();
		}
		
		if( Get_RPM()>0 )
		{
			g_ui1secTimer++;
			if( ((g_ui1secTimer % MAX_GPS_SAMPLE_TIME ) == 0) && (Get_GPS_Lat()!=0) && (Get_GPS_Lon()!=0) && (Get_GPS_Vailication() >= NMEA_SIG_FIXED_2D_3D) )
			{
				if( g_OBDControllerData.monitering_Data.GpsListCount<MAX_GPS_LIST_COUNT )
				{
					g_OBDControllerData.monitering_Data.GpsLatitudeList[g_OBDControllerData.monitering_Data.GpsListCount]=Get_GPS_Lat();
					g_OBDControllerData.monitering_Data.GpsLongitudeList[g_OBDControllerData.monitering_Data.GpsListCount]=Get_GPS_Lon();
					g_OBDControllerData.monitering_Data.GpsListCount++;
				}
				else
				{
	//				printf("MAX_GPS_LIST_COUNT OVER\r\n");
				}
			}
		}
		
		//패턴0 차량 공회전 시간 (속도 ==0 && RPM > 0 && 풋브레이크 == 0)
		if( (Get_Speed()==0) && (Get_RPM()>0) && (Get_FootBrake_State()==0) )
		{
			Increase_DrivePattern0();
		}
		//패턴1 정차 누적 시간  (속도 == 0 &&  RPM > 0 && 풋브레이크 ==1
		else if( (Get_Speed()==0) && (Get_RPM()>0) && (Get_FootBrake_State()==1) )
		{
			Increase_DrivePattern1();
		}
		//패턴2 제동 누적 시간 (속도 > 0  && RPM > 0 && 풋브레이크 ==1
		else if( (Get_Speed()>0) && (Get_RPM()>0) && (Get_FootBrake_State()==1) )
		{
			Increase_DrivePattern2();
		}
		//패턴3 관성 주행 (속도 > 0 && RPM >0 && 풋 브레이크 == 0 && Acc_Pos ==0)
		else if( (Get_Speed()>0) && (Get_RPM()>0) && (Get_FootBrake_State()==0) && (Get_AccelPos()==0))
		{
			Increase_DrivePattern3();
		}
		//패턴4 일반 주행 (속도  > 0 && RPM > 0 &&풋 브레이크 ==0 && Acc_Pos >0)
		else if( (Get_Speed()>0) && (Get_RPM()>0) && (Get_FootBrake_State()==0) && (Get_AccelPos()>0))
		{
			Increase_DrivePattern4();
		}
#if defined(PROTOCOL17)
		else
		{
			Increase_DrivePattern5();
		}
#endif
	}
}


void InjectionQuantity()
{
	static unsigned int s_uiInjectionOldTime=0;
	unsigned int uiInjectionDeltaTime=0;
	float fInjection_Quantity;
	float fSpecificGravity=0.86;

	uiInjectionDeltaTime = Get_TmrDelta(Get_Tmr(),s_uiInjectionOldTime);
	s_uiInjectionOldTime=Get_Tmr();

	//1mm3 = 0.001ml
	//1mm3 = 0.000001l
	//1cc = 1ml

	//인젝터 분사량 * rpm * 4 * 0.5 / 0.794
	//디젤 비중 : 0.82~0.87
	//ECU에서 분사된 연료량의합(Main+Post연료량)*rpm*4(실린더갯수)*0.5/비중'입니다.

	//1.Main+Post연료량 == 분사량 비교
	//fInjection_Quantity = (g_fMI1+g_fPIL1+g_fPIL2)*0.000001 * g_stFastFuncData.m_usRpm * 4  *(uiInjectionDeltaTime/1000.0) /0.87;		//분사량(ms/ml) = 1ms당 분사량 * delta Time(ms)


	//분사량이 분당 분사량(mm3/min)
	//real	fInjection_Quantity = (g_ucInjectionQuantity*0.001/60) * g_stFastFuncData.m_usRpm * g_ucCylinder * 0.5  *(uiInjectionDeltaTime/1000.0)/0.86 ;	//분사량(ms/ml) = 1s당 분사량 * delta Time(s)
	//		fInjection_Quantity = (g_ucInjectionQuantity*0.001/60) * (g_stFastFuncData.m_usRpm+40) * g_ucCylinder * 0.5  *(uiInjectionDeltaTime/1000.0)/0.86 ;	//분사량(ms/ml) = 1s당 분사량 * delta Time(s)  GD - RPM 계기반 대비 -60~70  (+40해주면 연비 동일)
	//fInjection_Quantity = (g_ucInjectionQuantity*0.001/60) * g_stFastFuncData.m_usRpm * g_ucCylinder * 0.5  *(uiInjectionDeltaTime/1000.0)/0.85 ;	//분사량(ms/ml) = 1s당 분사량 * delta Time(s)  gd 16my trip과 동일하게 동작함


	//분사량 + 후분사1 + 후분사2(20151025 TL) - OK
	//	fInjection_Quantity = ((g_ucInjectionQuantity +g_fPOL1 +g_fPOL2)*0.001/60) * g_stFastFuncData.m_usRpm * g_ucCylinder * 0.5  *(uiInjectionDeltaTime/1000.0)/0.86 ;	//분사량(ms/ml) = 1s당 분사량 * delta Time(s)
	//연료분사량 + 파일럿 + 후분사(20151025 TL) -  OK -저속구간 연비 조금 낮음
	// fine과는 다르게 ml가 아닌 L로 연비를 구하고 있어서 *0.001 을 더 해줘야 함

	//PD 디젤 GD 디젤, HG 디젤 을 RPM-40 해서 연비 최대 +- 0.4 차이남
	if(Get_RPM() < 1500)
		fInjection_Quantity = ((Get_InjectionQuantity() +Get_POL1() +Get_POL2())*0.000001/60.0) * (Get_RPM()-40) * g_ucCylinder * 0.5  *(uiInjectionDeltaTime/1000.0)/fSpecificGravity ;	//분사량(ms/ml) = 1s당 분사량 * delta Time(s)
	else
		fInjection_Quantity = ((Get_InjectionQuantity()+Get_PIL1()+Get_PIL2()+Get_PIL3()+Get_POL1()+Get_POL2())*0.000001/60.0) * (Get_RPM()-40) * g_ucCylinder * 0.5  *(uiInjectionDeltaTime/1000.0)/fSpecificGravity ;	//분사량(ms/ml) = 1s당 분사량 * delta Time(s)

	//주분사량 + 파일럿 + 후분사
	//fInjection_Quantity = ((g_fMI1+g_fPIL1+g_fPIL2+g_fPIL3+g_fPOL1+g_fPOL2)*0.001/60.0) * g_stFastFuncData.m_usRpm * g_ucCylinder * 0.5  *(uiInjectionDeltaTime/1000.0)/0.86 ;	//분사량(ms/ml) = 1s당 분사량 * delta Time(s)

	Send_Fuel_Consume_From_CAN((double)fInjection_Quantity,FUEL_TYPE);
	//Trace(0,"[F%.4f  T%.4f]", g_stStaticOBDdata.g_fFuelConsumption, g_stStaticOBDdata.g_fFuelConsumptionT);

}

#define ARRAY_MAX 1
void fuelConsume()
{
	unsigned int uiFuelDeltaTime=0 ;
	float ftempMassAirFlow=0, fTemp=0, fFuel=0;

	static float s_tempMassAirFlow[ARRAY_MAX];
	static U8 s_tempMassAirFlowPosition=0;

	uiFuelDeltaTime = Get_TmrDelta(Get_Tmr(),g_uiFuelOldTime);
	g_uiFuelOldTime=Get_Tmr();

	//printf("FuelCut:%d RPM:%d\r\n",Get_FuelCut(),Get_RPM());
	if( (Get_FuelCut() == 0x00) && (Get_RPM() !=0x00) )		//fuel cut이 아니면 계산한다
	{
		s_tempMassAirFlow[s_tempMassAirFlowPosition] = Get_Mass_Air_Flow();
		s_tempMassAirFlowPosition++;
		s_tempMassAirFlowPosition = s_tempMassAirFlowPosition%ARRAY_MAX;
		ftempMassAirFlow =0;
		for(U8 i=0; i<ARRAY_MAX; i++) ftempMassAirFlow += s_tempMassAirFlow[i];
		ftempMassAirFlow = ftempMassAirFlow/ARRAY_MAX;

		//Trace(0,"s_tempMassAirFlowPosition : %hd , ftempMassAirFlow : %f  ftempMassAirFlow : %f \r\n", s_tempMassAirFlowPosition, ftempMassAirFlow,ftempMassAirFlow);
		//공연비 휘발유 14.7, LPG(Propane) 15.5, 메탄올 6.4, 에탄올 9.0, CNG 17.2, 경유 14.6
		if(FUELTYPE_GET_STATE()==GASOLINE)
		{
			fFuel = (14.7*g_fEquivalenceRatio*0.73);
			if(fFuel != 0)
			{
				fTemp  =  (float)((ftempMassAirFlow/ fFuel)*(uiFuelDeltaTime/1000.0)/1000.0);	//연료 소비량(ml) gasoline
				//printf(" 0 [temp :%.4f   fFuel : %.4f]\r\n", fTemp, fFuel);
			}
			else
			{
//				Trace(0," 1 [temp :%.4f   fFuel : %.4f]\r\n", fTemp, fFuel);
			}
		}
		else
		{
			Trace(0," FUELTYPE_GET_STATE()  : %d \r\n",FUELTYPE_GET_STATE());
		}
		Send_Fuel_Consume_From_CAN((double)fTemp,FUEL_TYPE);
        //Trace(0," FUEL consume: %f \r\n",Get_Total_Fuel_Consume());
	}
	else
	{
		//Trace(0,"[fuelconsuem] fuel_cut : %hd , RPM : %d \r\n", Get_FuelCut(), Get_RPM());
	}
}


void Set_Actuator( eACTUATOR_TYPE eInput )		//다른매니저에서 OBD매니저를 제어하기위한 함수-구동할기능을 인자로 준다
{
	g_eActuatorType = eInput;
	SetOBDState(eOBD_Actuator_Mode);
}

void SetActuatorStatus( eACTUATOR_STATUS eInput )
{
	static unsigned char s_ucSaveinfo = 0;
	if( s_ucSaveinfo != eInput )
	{
		Trace(0,"Change ACTUATOR_STATUS %d -> %d, %d\r\n",s_ucSaveinfo,eInput,g_eActuatorType);
		s_ucSaveinfo = eInput;
//		Trace(0,"Change ACTUATOR_STATUS -> %d, %d\r\n",eInput,g_eActuatorType);
	}
	g_eActuatorStatus = eInput;
}

eACTUATOR_STATUS GetActuatorStatus()
{
	return g_eActuatorStatus;
}

void OBDActuatorModeState( eACTUATOR_TYPE eInput)
{
	//static eACTUATOR_TYPE s_eSaveType = ACTUATOR_TYPE_NONE;

	if(eInput != g_eActaveType)	//명령이 바뀔때 마다 초기화
	{
		Trace(0,"ActStatus %d->%d\r\n",g_eActaveType,eInput);
		g_eActaveType = eInput;
		ActRelationInit();
	}

	switch( eInput )
	{
		case ACTUATOR_TYPE_NONE:
			break;
		case ACTUATOR_TYPE_WAKEUP:
			OBDWakeUp();
			break;
		case ACTUATOR_TYPE_ENGINERUN:
			OBDEngineRun();
			break;
		case ACTUATOR_TYPE_ENGINESTOP:
			OBDEngineStop();
			break;
		case ACTUATOR_TYPE_DOORLOCK:
			OBDDoorLock();
			break;
		case ACTUATOR_TYPE_DOORUNLOCK:
			OBDDoorUnlock();
			break;
		case ACTUATOR_TYPE_LAMP:
			OBDLamp();
			break;
		case ACTUATOR_TYPE_LAMPHORN:
			OBDLampHorn();
			break;
		case ACTUATOR_TYPE_AIRCON:
			OBDAircon();
			break;
		case ACTUATOR_TYPE_VENT:
			OBDVent();
			break;
		case ACTUATOR_TYPE_REARDEFOG:
			OBDRearDefog();
			break;
		case ACTUATOR_TYPE_HIGHBEAM:
//			OBDHighBeam();
			break;
		case ACTUATOR_TYPE_VEHICLE_CHECK:
			OBDVehicleCheck();
			break;
		case ACTUATOR_TYPE_IG3ON:
			SetEvAirControl(eIGONSet);
			OBDIG3On();
			break;
		case ACTUATOR_TYPE_AIRCON_STOP:
			SetClearAirControl();
			OBDFATCStop();
			break;
		default:
			break;
	}
}

bool IsOverlap(unsigned int iCanid, unsigned int *iCheckList)
{
	for(int i=0;i<HAL_CAN_MAX_MASK_CNT;i++)
	{
		if( iCheckList[i] == 0x00)
		{
			iCheckList[i] = iCanid;
			break;
		}
		if( iCheckList[i] == iCanid )
		{
			return true;
		}
	}
	return false;
}

bool Can_Actuator_Set( eACTUATOR_TYPE eInput, eCOMMSET_TYPE eType)
{
//stActuatorReqInfo			g_stActuatorReqInfo[ACTUATOR_MAX_REQ_CNT];	//Req ID와 ID별갯수
//stActuatorResInfo			g_stActuatorResInfo[ACTUATOR_MAX_RES_CNT];	//Res ID와 ID별갯수
//unsigned char g_ucTotReqCnt=0;	//Req의 총 라인수
//unsigned char g_ucTotResCnt=0;	//Res의 총 라인수
//unsigned char g_ucReqIndex=0;	//ReqInfo의 index(REQ세트의 수 ID의 갯수아님)
//unsigned char g_ucResIndex=0;	//ResInfo의 index(RES세트의 수 ID의 갯수아님)
	int i=0;
	char cCnt=0;
	unsigned char ucCanBPS[2];
	unsigned char ucMaskingCnt[2], ucMaskingOver[2];
	unsigned int uiStartMask[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiEndMask[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiStartMask2[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiEndMask2[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiOverlapMask1[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiOverlapMask2[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiCanSpeed[2];
	unsigned int uiCanLine[2];

	for(i=0; i<2; i++)	//초기화
	{
		ucCanBPS[i]=0;
		ucMaskingCnt[i]=0;
		uiCanSpeed[i]=0;
		uiCanLine[i]=0;
		ucMaskingOver[i]=0;
	}

	memset(uiStartMask,0x00,sizeof(uiStartMask));
	memset(uiEndMask,0x00,sizeof(uiEndMask));
	memset(uiStartMask2,0x00,sizeof(uiStartMask2));
	memset(uiEndMask2,0x00,sizeof(uiEndMask2));
	memset(uiOverlapMask1,0x00,sizeof(uiOverlapMask1));
	memset(uiOverlapMask2,0x00,sizeof(uiOverlapMask2));

	if( eInput == ACTUATOR_TYPE_VEHICLE_CHECK )
	{
#if defined(ACTUATOR_LOG)
		Trace(ACTUATOR_LOG,"Can_Actuator_Set_VEH\r\n");
#endif
		uiCanSpeed[0]	= g_stActuatorReadyData[0].m_uiCanSpeed;
		uiCanLine[0]	= g_stActuatorReadyData[0].m_ucCanLine;

		for( i=0; i<g_stActuatorReadyInfo.m_uiCount; i++)
		{
			if( uiCanSpeed[0] == g_stActuatorReadyData[i].m_uiCanSpeed && 
			     uiCanLine[0] == g_stActuatorReadyData[i].m_ucCanLine)
			{
				switch( g_stActuatorReadyData[i].m_uiCanSpeed )
				{
					case 1024	:	ucCanBPS[0] = eCAN_1MBPS;	break;
					case 500	:	ucCanBPS[0] = eCAN_500KBPS;	break;
					case 250	:	ucCanBPS[0] = eCAN_250KBPS;	break;
					case 125	:	ucCanBPS[0] = eCAN_125KBPS;	break;
					case 100	:	ucCanBPS[0] = eCAN_100KBPS;	break;
					case 50	    :	ucCanBPS[0] = eCAN_50KBPS;	break;
					default:
						Trace(0,"Not Found CanBPS_V\r\n");
						return false;
				}
#if 1
				if(IsOverlap(g_stActuatorReadyData[i].m_uiResVal,uiOverlapMask1)==true){}
				else
				{
					if( ucMaskingCnt[0]<HAL_CAN_MAX_MASK_CNT )
                    {
						uiEndMask[ucMaskingCnt[0]]=uiStartMask[ucMaskingCnt[0]]=g_stActuatorReadyData[i].m_uiResVal;
#if defined(ACTUATOR_LOG)
						Trace(ACTUATOR_LOG,"Set_V:[%04X][%04X]\r\n",uiStartMask[ucMaskingCnt[0]], uiEndMask[ucMaskingCnt[0]]);
#endif
						ucMaskingCnt[0]++;
					}
					else
					{
						ucMaskingOver[1] =1;
						g_bDBParsingFailFlag=1;
                    }
				}
#else	// 1
				if(ucMaskingCnt[0] == 0)
				{
					uiEndMask[ucMaskingCnt[0]]=uiStartMask[ucMaskingCnt[0]]=g_stActuatorReadyData[i].m_uiResVal;
#if defined(ACTUATOR_LOG)
				Trace(ACTUATOR_LOG,"Set_V:[%04X][%04X]\r\n",uiStartMask[ucMaskingCnt[0]], uiEndMask[ucMaskingCnt[0]]);
#endif
					ucMaskingCnt[0]++;
				}
				else
				{
					if( uiStartMask[ucMaskingCnt[0]-1] != g_stActuatorReadyData[i].m_uiResVal )
					{
						if( ucMaskingCnt[0]<HAL_CAN_MAX_MASK_CNT )
						{
							uiEndMask[ucMaskingCnt[0]]=uiStartMask[ucMaskingCnt[0]]=g_stActuatorReadyData[i].m_uiResVal;
#if defined(ACTUATOR_LOG)
				Trace(ACTUATOR_LOG,"Set_V:[%04X][%04X]\r\n",uiStartMask[ucMaskingCnt[0]], uiEndMask[ucMaskingCnt[0]]);
#endif
							ucMaskingCnt[0]++;
						}
						else
						{
						  	ucMaskingOver[0] = 1;
							g_bDBParsingFailFlag=1;
						}
					}
				}			
#endif	// 1

			}
			else
			{
				switch( g_stActuatorReadyData[i].m_uiCanSpeed )
				{
					case 1024	:	ucCanBPS[1] = eCAN_1MBPS;	break;
					case 500	:	ucCanBPS[1] = eCAN_500KBPS;	break;
					case 250	:	ucCanBPS[1] = eCAN_250KBPS;	break;
					case 125	:	ucCanBPS[1] = eCAN_125KBPS;	break;
					case 100	:	ucCanBPS[1] = eCAN_100KBPS;	break;
					case 50	    :	ucCanBPS[1] = eCAN_50KBPS;	break;
					default:
						Trace(0,"Not Found CanBPS_V2\r\n");
						return false;
				}
				uiCanSpeed[1]	= g_stActuatorReadyData[i].m_uiCanSpeed;
				uiCanLine[1]	= g_stActuatorReadyData[i].m_ucCanLine;
#if 1
				if(IsOverlap(g_stActuatorReadyData[i].m_uiResVal,uiOverlapMask2)==true){}
				else
				{
					if( ucMaskingCnt[1]<HAL_CAN_MAX_MASK_CNT )
					{
						uiEndMask2[ucMaskingCnt[1]]=uiStartMask2[ucMaskingCnt[1]]=g_stActuatorReadyData[i].m_uiResVal;
#if defined(ACTUATOR_LOG)
						Trace(ACTUATOR_LOG,"Set_V2:[%04X][%04X]\r\n",uiStartMask2[ucMaskingCnt[1]], uiEndMask2[ucMaskingCnt[1]]);
#endif
						ucMaskingCnt[1]++;
					}
					else
					{
						ucMaskingOver[1] =1;
						g_bDBParsingFailFlag=1;
                    }
				}
#else	// 1
				if( ucMaskingCnt[1] == 0)
				{
					uiEndMask2[ucMaskingCnt[1]]=uiStartMask2[ucMaskingCnt[1]]=g_stActuatorReadyData[i].m_uiResVal;
#if defined(ACTUATOR_LOG)
				Trace(ACTUATOR_LOG,"Set_V2:[%04X][%04X]\r\n",uiStartMask2[ucMaskingCnt[1]], uiEndMask2[ucMaskingCnt[1]]);
#endif
					ucMaskingCnt[1]++;
				}
				else
				{
					if( uiStartMask2[ucMaskingCnt[1]-1] != g_stActuatorReadyData[i].m_uiResVal )
					{
						if( ucMaskingCnt[1]<HAL_CAN_MAX_MASK_CNT )
						{
							uiEndMask2[ucMaskingCnt[1]]=uiStartMask2[ucMaskingCnt[1]]=g_stActuatorReadyData[i].m_uiResVal;
#if defined(ACTUATOR_LOG)
				Trace(ACTUATOR_LOG,"Set_V2:[%04X][%04X]\r\n",uiStartMask2[ucMaskingCnt[1]], uiEndMask2[ucMaskingCnt[1]]);
#endif
							ucMaskingCnt[1]++;
						}
						else
						{
						  	ucMaskingOver[1] =1;
							g_bDBParsingFailFlag=1;
						}
					}
				}
#endif	// 1
			}
		}
		//if(CFD_GetCanFDAdapter())
		if((CFD_GetCanFDAdapter() == true)||(g_bDBParsingFailFlag == 1))
		{
		  	g_bDBParsingFailFlag = 0;	//DB 오버플로우인지 확인하기 위해서 넣은건데.. 초기화를 해줘도 되나?
			if( CFD_GetCanFDAdapter() == true )
			{
				ucMaskingOver[0] = 1;
				ucMaskingOver[1] = 1;
			}
			//if(uiCanLine[0] == HIGHCAN2)	조건없어도 된다. 위에서 uiStartMask 와 uiCanLine 매칭되어 있다
			if(ucMaskingOver[0] == 1)
			{	
				ucMaskingCnt[0] = 8;
				//ucMaskingCnt[0] = 7;	//test
				DefaultMaskSet(uiStartMask,uiEndMask,ucMaskingCnt[0]);
			}

			//if(uiCanLine[1] == HIGHCAN3)
			if(ucMaskingOver[1] == 1)
			{
				ucMaskingCnt[1] = 8;
				//ucMaskingCnt[1] = 7;
				DefaultMaskSet(uiStartMask2,uiEndMask2,ucMaskingCnt[1]);			
			}

			//for(int i = 0 ; i < ucMaskingCnt[0] ; i++)
				//printf("[0] uiStartMask : %x ---- uiEndMask : %x ----\n",uiStartMask[i],uiEndMask[i]);

			//for(int i = 0 ; i < ucMaskingCnt[1] ; i++)
				//printf("[1] uiStartMask2 : %x ---- uiEndMask2 : %x ----\n",uiStartMask2[i],uiEndMask2[i]);
		}
		
#if defined(ACTUATOR_LOG)
				Trace(ACTUATOR_LOG,"ucMaskingCnt:[%d][%d]\r\n",ucMaskingCnt[0], ucMaskingCnt[1]);
#endif
		if( ucCanBPS[1] == 0 )	cCnt=1;	//CAN세팅할께 하나면 한번만타도록 하기위해
		else					cCnt=2;

		for(i=0; i<cCnt; i++)
		{
			//CAN1 세팅
			if( uiCanLine[i] == HIGHCAN1 || uiCanLine[i] == HIGHCAN2 )
			{
				if(	uiCanLine[i] == HIGHCAN1 )			Oem_CAN_Initial_CH1(Highcan1, ucCanBPS[i]);
				else	if(	uiCanLine[i] == HIGHCAN2 )	Oem_CAN_Initial_CH1(Highcan2, ucCanBPS[i]);
				else
				{
					Trace(0,"Not Found CanLine_RES\r\n");
					return false;
				}

				if( ucMaskingCnt[i] > HAL_CAN_MAX_MASK_CNT )
				{
					ucMaskingCnt[i] = HAL_CAN_MAX_MASK_CNT;
					Trace(0,"Not Found CanLine_RES\r\n");
					return false;
				}
				
				if(i == 0)	Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_EXTENDED_CAN, ucMaskingCnt[i], uiStartMask, uiEndMask);
				else			Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_EXTENDED_CAN, ucMaskingCnt[i], uiStartMask2, uiEndMask2);
				DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);		//D캔상태를 안바꿔주면 RX를 못함
			}

			//CAN2 세팅
			if( uiCanLine[i] == HIGHCAN3 || uiCanLine[i] == LOWCAN1 )
			{
				if(	uiCanLine[i] == HIGHCAN3 )			Oem_CAN_Initial_CH2(Highcan3, ucCanBPS[i]);
				else	if(	uiCanLine[i] == LOWCAN1 )	Oem_CAN_Initial_CH2(Lowcan1, ucCanBPS[i]);
				else
				{
					Trace(0,"Not Found CanLine_RES\r\n");
					return false;
				}

				if(i == 0)	Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_EXTENDED_CAN, ucMaskingCnt[i], uiStartMask, uiEndMask);
				else			Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_EXTENDED_CAN, ucMaskingCnt[i], uiStartMask2, uiEndMask2);
				LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
			}
		}

		if(CFD_GetCanFDAdapter())
		{
			for( i=0; i<g_stActuatorReadyInfo.m_uiCount; i++)
			{
				CFD_SetCanFDConfig(g_stActuatorReadyData[i].m_ucFDCanLine,g_stActuatorReadyData[i].m_ucFDCanBaudRate);
			}
			
			CanFD_ChannelSet();
		}
		
	}
	else
	{
		if( eType == REQ_TYPE )	//REQ 세팅
		{
#if defined(ACTUATOR_LOG)
			Trace(ACTUATOR_LOG,"Can_Actuator_Set_REQ\r\n");
#endif
			switch( g_stActuatorReqData[g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos].m_uiCanSpeed )
			{
				case 1024	:	ucCanBPS[0] = eCAN_1MBPS;		break;
				case 500	:	ucCanBPS[0] = eCAN_500KBPS;	break;
				case 250	:	ucCanBPS[0] = eCAN_250KBPS;	break;
				case 125	:	ucCanBPS[0] = eCAN_125KBPS;	break;
				case 100	:	ucCanBPS[0] = eCAN_100KBPS;	break;
				case 50	:	ucCanBPS[0] = eCAN_50KBPS;		break;
				default:
					Trace(0,"Not Found CanBPS_REQ\r\n");
					return false;
			}
//Oem_CAN_None_Receive_Set();
			if(	g_stActuatorReqData[g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos].m_ucCanLine == HIGHCAN1 )		Oem_CAN_Initial_CH1(Highcan1, ucCanBPS[0]);
			else if(g_stActuatorReqData[g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos].m_ucCanLine == HIGHCAN2 )	Oem_CAN_Initial_CH1(Highcan2, ucCanBPS[0]);
			else if(g_stActuatorReqData[g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos].m_ucCanLine == HIGHCAN3 )	Oem_CAN_Initial_CH2(Highcan3, ucCanBPS[0]);
			else if( g_stActuatorReqData[g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos].m_ucCanLine == LOWCAN1 ) 	Oem_CAN_Initial_CH2(Lowcan1, ucCanBPS[0]);
			else
			{
				Trace(0,"Not Found CanLine_REQ\r\n");
				return false;
			}

	//		memset(uiStartMask,0x00,sizeof(uiStartMask));
	//		memset(uiEndMask,0x00,sizeof(uiEndMask));

			for(i=0;i<HAL_CAN_MAX_MASK_CNT;i++)
			{
				uiEndMask[i]=uiStartMask[i]=0xFFFF;
			}

			/*
			if(CFD_GetCanFDAdapter())
			{
				printf(" CFD_GetCanFDAdapter() Set eCanFDLineMode_ALLCAN1 \n");
				
				//CFD_SetCanLineMode(eCanFDLineMode_ALLCAN1);
				ucMaskingCnt[0] = 8;
				DefaultMaskSet(uiStartMask,uiEndMask,ucMaskingCnt[0]);

				Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN,8, uiStartMask, uiEndMask);
				Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_CAN,8, uiStartMask, uiEndMask);
				CanBufferClear(eCLEAR_TYPE_ALL);	
			}
			else
			*/
			{	
				Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_EXTENDED_CAN,HAL_CAN_MAX_MASK_CNT, uiStartMask, uiEndMask);
				Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_EXTENDED_CAN,HAL_CAN_MAX_MASK_CNT, uiStartMask, uiEndMask);
			}

			if(CFD_GetCanFDAdapter())
			{
				CanBufferClear(eCLEAR_TYPE_ALL);
			}
			

Trace(0,"Can_Actuator_Set_REQ_%d\r\n",g_stActuatorReqInfo[g_ucReqIndexPos].m_uiCount);
//			if(	g_stActuatorReqData[g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos].m_ucCanLine == HIGHCAN1 || g_stActuatorReqData[g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos].m_ucCanLine == HIGHCAN2)
//				Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN,g_stActuatorResInfo[g_ucReqIndexPos].m_uiCount, uiStartMask, uiEndMask);
//			else	//203, 204
//				Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_CAN,g_stActuatorResInfo[g_ucReqIndexPos].m_uiCount, uiStartMask, uiEndMask);
			
		}
		else if( eType == RES_TYPE )	//RES 세팅
		{
			if(CFD_GetCanFDAdapter())
			{
				printf(" CFD_GetCanFDAdapter() Set eCanFDLineMode_Default \n");			
				//CFD_SetCanLineMode(eCanFDLineMode_Default);
				//CFD_SetByPassFlagAllOn();
			}
			
			Oem_CAN_None_Receive_Set();

#if defined(ACTUATOR_LOG)
			Trace(ACTUATOR_LOG,"Can_Actuator_Set_RES\r\n");
#endif
			//처음 온값을 저장
			uiCanSpeed[0]	= g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos].m_uiCanSpeed;
			uiCanLine[0]		= g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos].m_ucCanLine;

			for( i=0; i<g_stActuatorResInfo[g_ucResIndexPos].m_uiCount; i++)
			{
				//저장된 값과 같을때
				if( uiCanSpeed[0] == g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i].m_uiCanSpeed &&
					uiCanLine[0]  == g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i].m_ucCanLine)
				{
					switch( g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i ].m_uiCanSpeed )
					{
						case 1024	:	ucCanBPS[0] = eCAN_1MBPS;		break;
						case 500	:	ucCanBPS[0] = eCAN_500KBPS;	break;
						case 250	:	ucCanBPS[0] = eCAN_250KBPS;	break;
						case 125	:	ucCanBPS[0] = eCAN_125KBPS;	break;
						case 100	:	ucCanBPS[0] = eCAN_100KBPS;	break;
						case 50	:	ucCanBPS[0] = eCAN_50KBPS;		break;
						default:
							Trace(0,"Not Found CanBPS_RES\r\n");
							return false;
					}
					uiEndMask[ucMaskingCnt[0]]=uiStartMask[ucMaskingCnt[0]]=g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i ].m_uiResVal;
#if defined(ACTUATOR_LOG)
					Trace(ACTUATOR_LOG,"Res:[%04X][%04X]\r\n",uiStartMask[ucMaskingCnt[0]], uiEndMask[ucMaskingCnt[0]]);
#endif
					ucMaskingCnt[0]++;
				}
				else
				{
					switch( g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos+ i].m_uiCanSpeed )
					{
						case 1024	:	ucCanBPS[1] = eCAN_1MBPS;		break;
						case 500	:	ucCanBPS[1] = eCAN_500KBPS;	break;
						case 250	:	ucCanBPS[1] = eCAN_250KBPS;	break;
						case 125	:	ucCanBPS[1] = eCAN_125KBPS;	break;
						case 100	:	ucCanBPS[1] = eCAN_100KBPS;	break;
						case 50	:	ucCanBPS[1] = eCAN_50KBPS;		break;
						default:
							Trace(0,"Not Found CanBPS_RES2\r\n");
							return false;
					}
					uiCanSpeed[1]	= g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos+ i].m_uiCanSpeed;
					uiCanLine[1]		= g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos+ i].m_ucCanLine;
					uiEndMask2[ucMaskingCnt[1]]=uiStartMask2[ucMaskingCnt[1]]=g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i ].m_uiResVal;
#if defined(ACTUATOR_LOG)
					Trace(ACTUATOR_LOG,"Res2:[%04X][%04X]\r\n",uiStartMask2[ucMaskingCnt[1]], uiEndMask2[ucMaskingCnt[1]]);
#endif
					ucMaskingCnt[1]++;
				}
			}

			if(CFD_GetCanFDAdapter())
			{
				if(uiCanLine[0] == HIGHCAN2 || uiCanLine[0] == HIGHCAN3)
				{
					ucMaskingCnt[0] = 8;	
					DefaultMaskSet(uiStartMask,uiEndMask,ucMaskingCnt[0]);					
				}

				if(uiCanLine[1] == HIGHCAN3 || uiCanLine[1] == HIGHCAN2)
				{
					ucMaskingCnt[1] = 8;										
					DefaultMaskSet(uiStartMask2,uiEndMask2,ucMaskingCnt[1]);			
				}

				//Ex
				if(ucCanBPS[1] == eCAN_1MBPS)
				{
					printf("Can_Act Res 111 \r\n");
					
					if(uiCanLine[0] == HIGHCAN2)
					{
						ucCanBPS[1] = 2;
						uiCanLine[1] = HIGHCAN3;
						ucMaskingCnt[1] = 8;										
						DefaultMaskSet(uiStartMask2,uiEndMask2,ucMaskingCnt[1]);			
					}
				}
				
				//for(int i = 0 ; i < ucMaskingCnt[0] ; i++)
					//printf("[0] uiStartMask : %d ---- uiEndMask : %d ----\n",uiStartMask[i],uiEndMask[i]);

				//for(int i = 0 ; i < ucMaskingCnt[1] ; i++)
					//printf("[1] uiStartMask2 : %d ---- uiEndMask2 : %d ----\n",uiStartMask2[i],uiEndMask2[i]);
			}
		

			if( ucCanBPS[1] == eCAN_1MBPS )	cCnt=1;	//CAN세팅할께 하나면 한번만타도록 하기위해
			else								cCnt=2;

			for(i=0; i<cCnt; i++)
			{
				//CAN1 세팅
				if( uiCanLine[i] == HIGHCAN1 || uiCanLine[i] == HIGHCAN2 )
				{
					if(	uiCanLine[i] == HIGHCAN1 )			Oem_CAN_Initial_CH1(Highcan1, ucCanBPS[i]);
					else	if(	uiCanLine[i] == HIGHCAN2 )	Oem_CAN_Initial_CH1(Highcan2, ucCanBPS[i]);
					else
					{
						Trace(0,"Not Found CanLine_RES\r\n");
						return false;
					}

					if( eInput == ACTUATOR_TYPE_WAKEUP)
					{
						//0001~0xFFFF, 0001~07FF등은 마스킹이 먹지않음
						//0300~0x03FF, 0700~07FF는 가능
						ucMaskingCnt[0]=8;
						DefaultMaskSet(uiStartMask,uiEndMask,ucMaskingCnt[0]);
						g_bCanDataFlag_Wakeup = false;
					}

					if(i == 0)	Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_EXTENDED_CAN, ucMaskingCnt[i], uiStartMask, uiEndMask);
					else			Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_EXTENDED_CAN, ucMaskingCnt[i], uiStartMask2, uiEndMask2);
					DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);		//D캔상태를 안바꿔주면 RX를 못함
					//VCAN_SET_COMM_STATE(eVCAN_RX_BLOCK);
				}

				//CAN2 세팅
				if( uiCanLine[i] == HIGHCAN3 || uiCanLine[i] == LOWCAN1 )
				{
					if(	uiCanLine[i] == HIGHCAN3 )			Oem_CAN_Initial_CH2(Highcan3, ucCanBPS[i]);
					else	if(	uiCanLine[i] == LOWCAN1 )	Oem_CAN_Initial_CH2(Lowcan1, ucCanBPS[i]);
					else
					{
						Trace(0,"Not Found CanLine_RES\r\n");
						return false;
					}

					if( eInput == ACTUATOR_TYPE_WAKEUP)
					{
						//0001~0xFFFF, 0001~07FF등은 마스킹이 먹지않음
						//0300~0x03FF, 0700~07FF는 가능
						ucMaskingCnt[0]=8;
						DefaultMaskSet(uiStartMask,uiEndMask,ucMaskingCnt[0]);
						g_bCanDataFlag_Wakeup = false;
					}

					if(i == 0)	Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_EXTENDED_CAN, ucMaskingCnt[i], uiStartMask, uiEndMask);
					else			Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_EXTENDED_CAN, ucMaskingCnt[i], uiStartMask2, uiEndMask2);
					LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
				}
			}

			if(CFD_GetCanFDAdapter())
			{
				for( i=0; i<g_stActuatorResInfo[g_ucResIndexPos].m_uiCount; i++)
				{
					CFD_SetCanFDConfig(g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos].m_ucFDCanLine,
				  					   g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos].m_ucFDCanBaudRate);
				}
				CanFD_ChannelSet();
			}
		}
	}

	return true;
}

bool Can_Actuator_Set_Wakeup( char cNum)
{
	int i=0;
	unsigned char ucCanBPS[2];
	unsigned char ucMaskingCnt[2];
	unsigned int uiStartMask[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiEndMask[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiStartMask2[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiEndMask2[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiCanSpeed[2];
	unsigned int uiCanLine[2];

	for(i=0; i<2; i++)	//초기화
	{
		ucCanBPS[i]=0;
		ucMaskingCnt[i]=0;
		uiCanSpeed[i]=0;
		uiCanLine[i]=0;
	}

	memset(uiStartMask,0x00,sizeof(uiStartMask));
	memset(uiEndMask,0x00,sizeof(uiEndMask));
	memset(uiStartMask2,0x00,sizeof(uiStartMask2));
	memset(uiEndMask2,0x00,sizeof(uiEndMask2));

#if defined(ACTUATOR_LOG)
	Trace(ACTUATOR_LOG,"Can_Actuator_Set_Wakeup %d\r\n",cNum);
#endif
	//처음 온값을 저장
	uiCanSpeed[0]	= g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos].m_uiCanSpeed;
	uiCanLine[0]		= g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos].m_ucCanLine;

	for( i=0; i<g_stActuatorResInfo[g_ucResIndexPos].m_uiCount; i++)
	{
		//저장된 값과 같을때
		if( uiCanSpeed[0] == g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i].m_uiCanSpeed &&
			uiCanLine[0]	  == g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i].m_ucCanLine)
		{
			switch( g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i ].m_uiCanSpeed )
			{
				case 1024	:	ucCanBPS[0] = eCAN_1MBPS;		break;
				case 500	:	ucCanBPS[0] = eCAN_500KBPS;	break;
				case 250	:	ucCanBPS[0] = eCAN_250KBPS;	break;
				case 125	:	ucCanBPS[0] = eCAN_125KBPS;	break;
				case 100	:	ucCanBPS[0] = eCAN_100KBPS;	break;
				case 50	:	ucCanBPS[0] = eCAN_50KBPS;		break;
				default:
					Trace(0,"Not Found CanBPS_RES\r\n");
					return false;
			}
			uiEndMask[ucMaskingCnt[0]]=uiStartMask[ucMaskingCnt[0]]=g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i ].m_uiResVal;
#if defined(ACTUATOR_LOG)
			Trace(ACTUATOR_LOG,"Can_Actuator_Set_Wakeup_Res:[%04X][%04X]\r\n",uiStartMask[ucMaskingCnt[0]], uiEndMask[ucMaskingCnt[0]]);
#endif
			ucMaskingCnt[0]++;
		}
		else
		{
			switch( g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos+ i].m_uiCanSpeed )
			{
				case 1024	:	ucCanBPS[1] = eCAN_1MBPS;		break;
				case 500	:	ucCanBPS[1] = eCAN_500KBPS;	break;
				case 250	:	ucCanBPS[1] = eCAN_250KBPS;	break;
				case 125	:	ucCanBPS[1] = eCAN_125KBPS;	break;
				case 100	:	ucCanBPS[1] = eCAN_100KBPS;	break;
				case 50	:	ucCanBPS[1] = eCAN_50KBPS;		break;
				default:
					Trace(0,"Not Found CanBPS_RES2\r\n");
					return false;
			}
			uiCanSpeed[1]	= g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos+ i].m_uiCanSpeed;
			uiCanLine[1]		= g_stActuatorResData[g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos+ i].m_ucCanLine;
			uiEndMask2[ucMaskingCnt[1]]=uiStartMask2[ucMaskingCnt[1]]=g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i ].m_uiResVal;
#if defined(ACTUATOR_LOG)
			Trace(ACTUATOR_LOG,"Can_Actuator_Set_Wakeup_Res2:[%04X][%04X]\r\n",uiStartMask2[ucMaskingCnt[1]], uiEndMask2[ucMaskingCnt[1]]);
#endif
			ucMaskingCnt[1]++;
		}
	}

	//CAN1 세팅
	if( uiCanLine[cNum] == HIGHCAN1 || uiCanLine[cNum] == HIGHCAN2 )
	{
		if(	uiCanLine[cNum] == HIGHCAN1 )			Oem_CAN_Initial_CH1(Highcan1, ucCanBPS[cNum]);
		else	if(	uiCanLine[cNum] == HIGHCAN2 )	Oem_CAN_Initial_CH1(Highcan2, ucCanBPS[cNum]);
		else
		{
			Trace(0,"Not Found CanLine_RES\r\n");
			return false;
		}

		//0001~0xFFFF, 0001~07FF등은 마스킹이 먹지않음
		//0300~0x03FF, 0700~07FF는 가능
		ucMaskingCnt[0]=8;
		ucMaskingCnt[1]=8;
		DefaultMaskSet(uiStartMask,uiEndMask,ucMaskingCnt[0]);
		DefaultMaskSet(uiStartMask2,uiEndMask2,ucMaskingCnt[1]);

		if(i == 0)	Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN, ucMaskingCnt[cNum], uiStartMask, uiEndMask);
		else			Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_1, STANDARD_CAN, ucMaskingCnt[cNum], uiStartMask2, uiEndMask2);
		DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);		//D캔상태를 안바꿔주면 RX를 못함
		g_bCanDataFlag_Wakeup = false;
	}

	//CAN2 세팅
	if( uiCanLine[cNum] == HIGHCAN3 || uiCanLine[cNum] == LOWCAN1 )
	{
		if(	uiCanLine[cNum] == HIGHCAN3 )			Oem_CAN_Initial_CH2(Highcan3, ucCanBPS[cNum]);
		else	if(	uiCanLine[cNum] == LOWCAN1 )	Oem_CAN_Initial_CH2(Lowcan1, ucCanBPS[cNum]);
		else
		{
			Trace(0,"Not Found CanLine_RES\r\n");
			return false;
		}

		//0001~0xFFFF, 0001~07FF등은 마스킹이 먹지않음
		//0300~0x03FF, 0700~07FF는 가능
		ucMaskingCnt[0]=8;
		ucMaskingCnt[1]=8;
		DefaultMaskSet(uiStartMask,uiEndMask,ucMaskingCnt[0]);
		DefaultMaskSet(uiStartMask2,uiEndMask2,ucMaskingCnt[1]);

		if(i == 0)	Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_CAN, ucMaskingCnt[cNum], uiStartMask, uiEndMask);
		else			Oem_CAN_Channel_Masket_Set(CAN_CHANNEL_2, STANDARD_CAN, ucMaskingCnt[cNum], uiStartMask2, uiEndMask2);
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
		g_bCanDataFlag_Wakeup = false;
	}

	if(CFD_GetCanFDAdapter())
	{
		for( i=0; i<g_stActuatorResInfo[g_ucResIndexPos].m_uiCount; i++)
		{
			CFD_SetCanFDConfig(g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i].m_ucFDCanLine,
							   g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i].m_ucFDCanBaudRate);
		}

		CanFD_ChannelSet();
	}
		
	return true;
}

void DefaultMaskSet(unsigned int  *StartMask,unsigned int  *EndMask, char Count)
{
	int i=0;
	for( i=0; i<Count; i++ )
	{
		StartMask[i] = i * 0x0100;
		EndMask[i] = (i * 0x0100) + 0x0100 - 1;
	}
}

void OBDDoorLock()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;
#if defined(CANFD_BYPASS_RES_WAIT)
	static unsigned char s_ucCanFDState = CANFD_BYPASS_REQUEST;
	static unsigned int s_uiTimeoutTimer = 0;
#endif

	if( g_uiFirstTimeFlag == 0 )
	{
#if defined(CANFD_BYPASS_RES_WAIT)
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_DOORLOCK )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{	
			if(s_ucCanFDState == CANFD_BYPASS_REQUEST)
			{
				m_stCFDCtrl.bCanFDByPassOffStatus = 0;
				s_ucCanFDState = CANFD_BYPASS_RESPONSE; 
				printf("--- Send %s CFD_SetByPassFlagAllOff---\n",__FUNCTION__);
				s_uiTimeoutTimer = Get_Tmr();
				CFD_SetByPassFlagAllOff();
			}
			else if(s_ucCanFDState == CANFD_BYPASS_RESPONSE)
			{
				if(m_stCFDCtrl.bCanFDByPassOffStatus == 1)
				{
					printf("--- Checked %s CanFD Response---\n",__FUNCTION__);	
					s_ucCanFDState = CANFD_BYPASS_NEXT; 
				}
				else
				{
					if( Get_TmrDelta(Get_Tmr(),s_uiTimeoutTimer) > CANFD_BYPASS_RES_WAIT_TIMEOUT )
					{
						SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
						g_uiActFailReason = eRETURNFAIL_OBD_BUSY;
						s_ucCanFDState = CANFD_BYPASS_REQUEST;
                        CFD_SetByPassFlagAllOn();
						return;
					}
				}
			}
			else if(s_ucCanFDState == CANFD_BYPASS_NEXT)
			{
				printf("--- Execute %s Original Logic---\n",__FUNCTION__);
				
				s_ucCanFDState = CANFD_BYPASS_REQUEST;
				g_uiFirstTimeFlag = 1;
			
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_DOORLOCK, REQ_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if( i==g_ucReqIndex || bRet==false )
				{
		//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Req\r\n");
					g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				OBDActuator_Req();
			}
		}
		else
		{
			g_uiFirstTimeFlag = 1;
			
			bRet = Can_Actuator_Set(ACTUATOR_TYPE_DOORLOCK, REQ_TYPE);
			g_bDBParsingFailFlag_Act=false;
			if( i==g_ucReqIndex || bRet==false )
			{
	//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
				Trace(0,"Can't find FuncIndex_Req\r\n");
	          	g_bDBParsingFailFlag_Act=true;
				SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
				return;
			}
			OBDActuator_Req();
		}
#else	//#if defined(CANFD_BYPASS_RES_WAIT)
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_DOORLOCK )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{
			CFD_SetByPassFlagAllOff();
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_DOORLOCK, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
#endif	//#if defined(CANFD_BYPASS_RES_WAIT)
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();				
				}	
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_DOORLOCK )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_DOORLOCK, RES_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
//					SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)	uiDecision = OBDActuator_Res();
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDDoorUnlock()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;
#if defined(CANFD_BYPASS_RES_WAIT)
	static unsigned char s_ucCanFDState = CANFD_BYPASS_REQUEST;
	static unsigned int s_uiTimeoutTimer = 0;
#endif
	if( g_uiFirstTimeFlag == 0 )
	{
#if defined(CANFD_BYPASS_RES_WAIT)
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_DOORUNLOCK )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{
			if(s_ucCanFDState == CANFD_BYPASS_REQUEST)
			{
				m_stCFDCtrl.bCanFDByPassOffStatus = 0;
				s_ucCanFDState = CANFD_BYPASS_RESPONSE; 
				printf("--- Send %s CFD_SetByPassFlagAllOff---\n",__FUNCTION__);
				s_uiTimeoutTimer = Get_Tmr();
				CFD_SetByPassFlagAllOff();
			}
			else if(s_ucCanFDState == CANFD_BYPASS_RESPONSE)
			{
				if(m_stCFDCtrl.bCanFDByPassOffStatus == 1)
				{
					printf("--- Checked %s CanFD Response---\n",__FUNCTION__);
					s_ucCanFDState = CANFD_BYPASS_NEXT;
				}
				else
				{
					if( Get_TmrDelta(Get_Tmr(),s_uiTimeoutTimer) > CANFD_BYPASS_RES_WAIT_TIMEOUT )
					{
						SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
						g_uiActFailReason = eRETURNFAIL_OBD_BUSY;
						s_ucCanFDState = CANFD_BYPASS_REQUEST;
                        CFD_SetByPassFlagAllOn();
						return;
					}
				}
			}
			else if(s_ucCanFDState == CANFD_BYPASS_NEXT)
			{
				printf("--- Execute %s Original Logic---\n",__FUNCTION__);
				s_ucCanFDState = CANFD_BYPASS_REQUEST;
				g_uiFirstTimeFlag = 1;
			
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_DOORUNLOCK, REQ_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if( i==g_ucReqIndex || bRet==false )
				{
	//				SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Req\r\n");
	    	      	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				OBDActuator_Req();
			}
		}
		else
		{
			g_uiFirstTimeFlag = 1;
			
			bRet = Can_Actuator_Set(ACTUATOR_TYPE_DOORUNLOCK, REQ_TYPE);
			g_bDBParsingFailFlag_Act=false;
			if( i==g_ucReqIndex || bRet==false )
			{
//				SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
				Trace(0,"Can't find FuncIndex_Req\r\n");
    	      	g_bDBParsingFailFlag_Act=true;
				SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
				return;
			}
			OBDActuator_Req();
		}
#else	//#if defined(CANFD_BYPASS_RES_WAIT)
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_DOORUNLOCK )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{
			CFD_SetByPassFlagAllOff();
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_DOORUNLOCK, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
#endif	//#if defined(CANFD_BYPASS_RES_WAIT)
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();
				}
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_DOORUNLOCK )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_DOORUNLOCK, RES_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
//					SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)	uiDecision = OBDActuator_Res();
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDLamp()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_LAMP )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{
			CFD_SetByPassFlagAllOff();
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_LAMP, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
		g_eLampCheckState = eLAMP_STATE_CHECKING;
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();
				}
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_LAMP )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_LAMP, RES_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
//					SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)	uiDecision = OBDActuator_Res();
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDLampHorn()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_LAMPHORN )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{
			CFD_SetByPassFlagAllOff();
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_LAMPHORN, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
		g_eLampCheckState = eLAMP_STATE_CHECKING;
		g_eHornCheckState = eHORN_STATE_CHECKING;
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();
				}
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_LAMPHORN )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_LAMPHORN, RES_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
//					SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)	uiDecision = OBDActuator_Res();
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDAircon()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;
	static unsigned int s_uiTimerCnt=0;
	if( g_uiFirstTimeFlag == 0 )
	{
		uint8_t bFahrenheitState = 0;
		GetAutolinkConfigProperty(eAutoLinkConfig_Fahrenheit,(void*)&bFahrenheitState);
		if( bFahrenheitState == 1 )
		{
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			g_uiActFailReason = eRETURNFAIL_TYPE_ENGON_AIRCONFAIL;
			g_ucActRetry=0;
			s_uiTimerCnt = 0;
			return;
		}
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_AIRCON )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{
			CFD_SetByPassFlagAllOff();
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_AIRCON, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();
				}
				s_uiTimerCnt = 0;
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_AIRCON )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
//				Trace(0,"-----------  11111  \r\n");
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_AIRCON, RES_TYPE);
//				Trace(0,"---------- %d \r\n",bRet);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
//					SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if( FUELTYPE_GET_STATE() == EV_NONE_READY )
			{
				if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) > 0)	// aircon 켜고 바로 체크 - 기존에는 Base타임 간격으로 체크했었음
				{
					if( g_stReportSmartReq.Defrost==0x01 )
					{
						if( (Get_EngAirconTemperature() == g_stReportSmartReq.CheckTemperature && Get_FATCState() == 1 ) && ((Get_EngVentStatus()&0x01)== 0x01) )
						{
							uiDecision = OBDActuator_Res();
							s_uiTimerCnt = 0;
							if( (uiDecision == eRETURNFAIL_TYPE_NONE) )
							{
								if( g_bEngRunContinueSuppFlag == true )
								{
									g_bEngRunKeepFlag = true;
									printf("eKeepAliveStart 77777\r\n");
									Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
									g_uiEngContinueSendTimer = Get_Tmr();
								}
								g_bFATCRunActFlag = true;
							}
						}
					}
					else
					{
						if( Get_EngAirconTemperature() == g_stReportSmartReq.CheckTemperature && Get_FATCState() == 1)
						{
							uiDecision = OBDActuator_Res();
							s_uiTimerCnt = 0;
							if( (uiDecision == eRETURNFAIL_TYPE_NONE) )
							{
								if( g_bEngRunContinueSuppFlag == true )
								{
									g_bEngRunKeepFlag = true;
									printf("eKeepAliveStart 77777\r\n");
									Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
									g_uiEngContinueSendTimer = Get_Tmr();
								}
								g_bFATCRunActFlag = true;
							}
						}
					}
					g_uiActCheckTime=Get_Tmr(); //명령쏘고 대기하는 시간 초기화
					s_uiTimerCnt++;
					if(s_uiTimerCnt >= ACTUATOR_CHECK_WAIT_TIME)//EVNONEREADY TYPE 에서  체크하지 못하는 경우 존재 
					{
						uiDecision = OBDActuator_Res();
					}
				}
			}
			else
			{
				if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)
				{
					uiDecision = OBDActuator_Res();
				}
			}
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
				  	if( FUELTYPE_GET_STATE() == EV_NONE_READY )
					{
					  	if( g_bEngRunContinueSuppFlag == true )
						{
						  	if((uiDecision&eRETURNFAIL_TYPE_AIRCON_FAIL) != eRETURNFAIL_TYPE_AIRCON_FAIL)	//에어컨은 성공했으나 Vent가 실패했을때
							{
							  	g_bEngRunKeepFlag = true;
								printf("eKeepAliveStart 77777\r\n");
								Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStart, (stCarReport *)NULL,0);
								g_uiEngContinueSendTimer = Get_Tmr();
							}
						}
						g_bFATCRunActFlag = true;
					}
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
					s_uiTimerCnt = 0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDVent()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_VENT )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_VENT, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_VENT )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
//				Trace(0,"-----------  11111  \r\n");
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_VENT, RES_TYPE);
//				Trace(0,"---------- %d \r\n",bRet);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
//					SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)	uiDecision = OBDActuator_Res();
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDRearDefog()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_REARDEFOG )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
		if(CFD_GetCanFDAdapter())
		{
			CFD_SetByPassFlagAllOff();
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_REARDEFOG, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				// 2020.11.27 CANFD 시동 제어시 차량에서 나오는 데이터로 인해 제어 간격 틀어짐 발생(canfd tx틀어짐방지)
				if(CFD_GetCanFDAdapter())
				{
					CFD_SetByPassFlagAllOn();
				}
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_REARDEFOG )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_REARDEFOG, RES_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)	uiDecision = OBDActuator_Res();
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDHighBeam()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_HIGHBEAM )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_HIGHBEAM, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_HIGHBEAM )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_HIGHBEAM, RES_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)	uiDecision = OBDActuator_Res();
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDIG3On()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;
	static unsigned int s_uiTimerCnt=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_IG3ON )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_IG3ON, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				s_uiTimerCnt = 0;
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_IG3ON )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_IG3ON, RES_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_BASE_TIME)
			{
			    if( Get_IG3_Status() == 1 )
				{
					uiDecision = OBDActuator_Res();
					s_uiTimerCnt = 0;
				}
				g_uiActCheckTime=Get_Tmr(); //명령쏘고 대기하는 시간 초기화
				s_uiTimerCnt++;
				if(s_uiTimerCnt >= (ACTUATOR_CHECK_WAIT_TIME/ACTUATOR_CHECK_BASE_TIME))
				{
					uiDecision = OBDActuator_Res();
				}
			}
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
					s_uiTimerCnt = 0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDFATCStop()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;
	static unsigned int s_uiTimerCnt=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		if( g_bEngRunKeepFlag == true )	//엔진 유지코드 멈췄어요 이벤트
		{
//			printf("eKeepAliveStop_4\r\n");
			Send2MngSysMsg(eMngObd,eOBDKeepAlive,eKeepAliveStop, (stCarReport *)NULL,0);
			g_bEngRunKeepFlag = false;
		}
		g_bFATCRunActFlag = false;
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_AIRCON_STOP )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_AIRCON_STOP, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
			Trace(0,"Can't find FuncIndex_Req\r\n");
          	g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		OBDActuator_Req();
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				s_uiTimerCnt = 0;
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_AIRCON_STOP )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set(ACTUATOR_TYPE_AIRCON_STOP, RES_TYPE);
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
					Trace(0,"Can't find FuncIndex_Res\r\n");
					g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_BASE_TIME)
			{
				g_uiActCheckTime=Get_Tmr(); //명령쏘고 대기하는 시간 초기화
				s_uiTimerCnt++;
				if( Get_FATCState() == 0 )
				{
					uiDecision = OBDActuator_Res();
					s_uiTimerCnt = 0;
				}
				if(s_uiTimerCnt >= (ACTUATOR_CHECK_WAIT_TIME/ACTUATOR_CHECK_BASE_TIME))
				{
					uiDecision = OBDActuator_Res();
				}
			}
			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
					s_uiTimerCnt = 0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDWakeUp()
{
	int i=0;
	bool bRet=true;
	unsigned int uiDecision=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		g_uiFirstTimeFlag = 1;
		g_uiActStartTime = Get_Tmr();
		for(i=0;i<g_ucReqIndex;i++)	//REQ세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
		{
			if( g_stActuatorReqInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_WAKEUP )
			{
				g_ucReqIndexPos = i;
				break;
			}
		}
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_WAKEUP, REQ_TYPE);
		g_bDBParsingFailFlag_Act=false;
		if( i==g_ucReqIndex || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Req\r\n");
            g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
        
		OBDActuator_Req();
	}
	else	//최초한번보내고 RUNNING상태로 변경되면서 계속 보냄
	{
		if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RUNNING )
		{
			OBDActuator_Req();
		}
		else if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_CHECKING )
		{
			if( g_uiActCheckFirstTimeFlag == 0 )
			{
				g_uiActCheckFirstTimeFlag=1;
				for(i=0;i<g_ucResIndex;i++)	//RES세트수만큼 루프 ex) DB에 0101,0301,0401만 있다면 3번
				{
					if( g_stActuatorResInfo[i].m_uiFuncIndex == ACTUATOR_TYPE_WAKEUP )
					{
						g_ucResIndexPos = i;
						break;
					}
				}
				bRet = Can_Actuator_Set_Wakeup( 0 );		//추후 함수 수정필요 현재 DB상관없이 2번만 돌도록 되어있음
				g_bDBParsingFailFlag_Act=false;
				if (i==g_ucResIndex || bRet==false )
				{
//					SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
					Trace(0,"Can't find FuncIndex_Res\r\n");
                  	g_bDBParsingFailFlag_Act=true;
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					return;
				}
				g_uiActCheckTime=Get_Tmr();
			}

			if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= 500)
			{
				uiDecision = OBDActuator_Res();
				if( g_stActuatorResInfo[i].m_uiCount == 1 )	//웨이크업이 하나인경우 한번만 돌도록 수정
				{}
				else
				{
					if( (uiDecision == 0) && (g_bWakeUpSettingFlag == false) )	//첫체크시 성공이면 두번째 세팅
					{
						g_bWakeUpSettingFlag = true;
						bRet = Can_Actuator_Set_Wakeup( 1 );		//추후 함수 수정필요 현재 DB상관없이 2번만 돌도록 되어있음
						g_bDBParsingFailFlag_Act=false;
						if (i==g_ucResIndex || bRet==false )
						{
//							SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
							Trace(0,"Can't find FuncIndex_Res\r\n");
                          	g_bDBParsingFailFlag_Act=true;
							SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
							return;
						}
						SetActuatorStatus( ACTUATOR_STATUS_CTRL_CHECKING );
						g_uiActCheckTime=Get_Tmr();
					}
				}
			}


			if( GetActuatorStatus() == ACTUATOR_STATUS_CTRL_RETRY )
			{
				g_ucActRetry++;
				g_bWakeUpSettingFlag = false;
				if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos ].m_ucRetry <= g_ucActRetry )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
					g_uiActFailReason = uiDecision;
					g_ucActRetry=0;
				}
				else	SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
			}
		}
	}
}

void OBDVehicleCheck()
{
	bool bRet=true;
//	unsigned int uiDecision=0;
	unsigned int uiDoorLock=0;
	unsigned int uiDoorOpen=0;
	unsigned int uiVehicleStatus=0;
	unsigned int uiRPM=0;

	if( g_uiFirstTimeFlag == 0 )
	{
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_VEHICLE_CHECK, RES_TYPE);
		SetActuatorStatus( ACTUATOR_STATUS_CHECK_RUNNING );
		g_uiFirstTimeFlag = 1;
		g_bDBParsingFailFlag_Act=false;
		if (g_stActuatorReadyInfo.m_uiCount==0 || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Res\r\n");
            g_bDBParsingFailFlag_Act=true;
			SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
		g_uiStatusCheckTime = Get_Tmr();
	}
	else
	{
		if( g_uiActCheckFirstTimeFlag == 0 )
		{
			g_uiActCheckFirstTimeFlag=1;
			g_uiActCheckTime=Get_Tmr();
			g_uiCanStopTime = Get_Tmr();	//CAN을 한번도 받아온적없을때를 위해 처음 들어와서 시간설정필요(정확한 시간을 위해서는 CAN세팅넣는시점이 더 적함하나 1초라 보기좋게 여기에 넣음)
			g_uiEngStartTimeOutTimer = Get_Tmr();
			g_uiEngStartDefaultTimer = Get_Tmr();
		}

		if(Get_TmrDelta(Get_Tmr(),g_uiActCheckTime) >= ACTUATOR_CHECK_WAIT_TIME)
		{
			uiDoorLock = Get_DoorLock();
			uiDoorOpen = Get_DoorOpen();
			uiVehicleStatus = Get_VehicleStatus();
			uiRPM = Get_RPM();
#if defined(ACTUATOR_LOG)
			if(g_eActuatorType != ACTUATOR_TYPE_ENGINERUN)
				Trace(ACTUATOR_LOG,"uiDoorLock : 0x%X uiDoorOpen : 0x%X uiVehicleStatus:%d uiRPM:%d \r\n",uiDoorLock,uiDoorOpen,uiVehicleStatus,uiRPM);
#endif
			//제어하기전 상태체크
			if( g_eActuatorType ==  ACTUATOR_TYPE_DOORLOCK)
			{
				if( uiDoorOpen == 0 )
				{
					SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
				}
				else
				{
					SetActuatorStatus( ACTUATOR_STATUS_CHECK_FAIL );
					g_uiActFailReason = eRETURNFAIL_TYPE_DOOROPEN;	//문열려있음
				}
			}
			else if( g_eActuatorType ==  ACTUATOR_TYPE_DOORUNLOCK)
			{
				SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
			}
			else if( g_eActuatorType ==  ACTUATOR_TYPE_LAMP)
			{
				SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
			}
			else if( g_eActuatorType ==  ACTUATOR_TYPE_LAMPHORN)
			{
				SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
			}
			else if( g_eActuatorType ==  ACTUATOR_TYPE_ENGINERUN)
			{
				if( Get_TmrDelta(Get_Tmr(),g_uiEngStartTimeOutTimer) >= 50000 )	//엔진런 명령을 받고 비클체크에 들어온 시간
				{
					SetActuatorStatus( ACTUATOR_STATUS_CHECK_FAIL );
					g_uiActFailReason = 0x20;	//통신실패
					if( uiDoorOpen != 0 )	g_uiActFailReason = g_uiActFailReason | eRETURNFAIL_TYPE_DOOROPEN;	//문열려있음
				}
				else	//모든 시동은 LOCK이후에는 무조건 일정시간(약30초) 후에만 걸림
				{	//호주 DH PE
					if( g_stControlConfig.m_ucAVNVersion == 3 )	//CAN이 안멈춰도 시동이 걸리는 타입-AVN3세대
					{
						if( uiDoorOpen == 0 && uiDoorLock == 0 )
						{
							if( g_stControlConfig.m_ucAVNType == 2)	//명령후 무조건 40초 대기
							{
								if( (Get_TmrDelta(Get_Tmr(),g_uiEngStartTimeOutTimer) >= 40000)  )
								{
									printf("Curr : %d\r\n",GetUTCTime());
									printf("Last : %d\r\n",g_stLockState.m_uiLastLockTime);
									SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
								}
							}
							else	//엔진런명령을 받고 45초 후거나 도어락이후 45초
							{
								if( (Get_TmrDelta(Get_Tmr(),g_uiEngStartTimeOutTimer) >= 45000) || ((GetUTCTime() - g_stLockState.m_uiLastLockTime) >= 45) )
								{
									printf("Curr : %d\r\n",GetUTCTime());
									printf("Last : %d\r\n",g_stLockState.m_uiLastLockTime);
									SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
								}
							}
						}
						else
						{
							SetActuatorStatus( ACTUATOR_STATUS_CHECK_FAIL );
							if( uiDoorLock != 0 )
							{
								g_uiActFailReason = eRETURNFAIL_TYPE_DOORUNLOCK;
								if( (uiDoorLock & 0x01) == 0x01 )	g_uiActFailReason |= eRETURNFAIL_TYPE_FL;	//FL
								if( (uiDoorLock & 0x02) == 0x02 )	g_uiActFailReason |= eRETURNFAIL_TYPE_FR;	//FR
								if( (uiDoorLock & 0x04) == 0x04 )	g_uiActFailReason |= eRETURNFAIL_TYPE_RL;	//RL
								if( (uiDoorLock & 0x08) == 0x08 )	g_uiActFailReason |= eRETURNFAIL_TYPE_RR;	//RR
							}
							else	//잠긴상태가 아니면 어디가 열렸는지도 확인
							{
								if( (uiDoorOpen & 0x01) == 0x01 )	g_uiActFailReason |= eRETURNFAIL_TYPE_FL;	//FL
								if( (uiDoorOpen & 0x02) == 0x02 )	g_uiActFailReason |= eRETURNFAIL_TYPE_FR;	//FR
								if( (uiDoorOpen & 0x04) == 0x04 )	g_uiActFailReason |= eRETURNFAIL_TYPE_RL;	//RL
								if( (uiDoorOpen & 0x08) == 0x08 )	g_uiActFailReason |= eRETURNFAIL_TYPE_RR;	//RR
								
								if( (uiDoorOpen & 0x30) != 0x00 )
								{
									if( (uiDoorOpen & 0x10) == 0x10 )	g_uiActFailReason |= eRETURNFAIL_TYPE_TRUNKOPEN;	//트렁크
									if( (uiDoorOpen & 0x20) == 0x20 )	g_uiActFailReason |= eRETURNFAIL_TYPE_HOODOPEN;	//후드
								}
								else												g_uiActFailReason |= eRETURNFAIL_TYPE_DOOROPEN;
							}
						}
					}
					else	//CAN멈추길 기다려야하는 타입- AVN4세대
					{
						if( uiDoorOpen == 0 && uiDoorLock == 0 )
						{
//								if( (Get_TmrDelta(Get_Tmr(),g_uiEngStartTimeOutTimer) >= 35000) || ( ((GetLocalTime() - g_stLockState.m_uiLastLockTime) >= 35)  && (Get_TmrDelta(Get_Tmr(),g_uiCanStopTime) >= 1000) ) )
								//엔진런명령을 받고 45초 후거나 도어락이후 45초
								if( (Get_TmrDelta(Get_Tmr(),g_uiEngStartTimeOutTimer) >= 45000) || ((GetUTCTime() - g_stLockState.m_uiLastLockTime) >= 45) )
								{
									printf("Curr2 : %d\r\n",GetUTCTime());
									printf("Last2 : %d\r\n",g_stLockState.m_uiLastLockTime);
									SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
								}
						}
						else
						{
							SetActuatorStatus( ACTUATOR_STATUS_CHECK_FAIL );
							if( uiDoorLock != 0 )
							{
								g_uiActFailReason = eRETURNFAIL_TYPE_DOORUNLOCK;
								if( (uiDoorLock & 0x01) == 0x01 )	g_uiActFailReason |= eRETURNFAIL_TYPE_FL;	//FL
								if( (uiDoorLock & 0x02) == 0x02 )	g_uiActFailReason |= eRETURNFAIL_TYPE_FR;	//FR
								if( (uiDoorLock & 0x04) == 0x04 )	g_uiActFailReason |= eRETURNFAIL_TYPE_RL;	//RL
								if( (uiDoorLock & 0x08) == 0x08 )	g_uiActFailReason |= eRETURNFAIL_TYPE_RR;	//RR
							}
							else	//잠긴상태가 아니면 어디가 열렸는지도 확인
							{
								if( (uiDoorOpen & 0x01) == 0x01 )	g_uiActFailReason |= eRETURNFAIL_TYPE_FL;	//FL
								if( (uiDoorOpen & 0x02) == 0x02 )	g_uiActFailReason |= eRETURNFAIL_TYPE_FR;	//FR
								if( (uiDoorOpen & 0x04) == 0x04 )	g_uiActFailReason |= eRETURNFAIL_TYPE_RL;	//RL
								if( (uiDoorOpen & 0x08) == 0x08 )	g_uiActFailReason |= eRETURNFAIL_TYPE_RR;	//RR
								
								if( (uiDoorOpen & 0x30) != 0x00 )
								{
									if( (uiDoorOpen & 0x10) == 0x10 )	g_uiActFailReason |= eRETURNFAIL_TYPE_TRUNKOPEN;	//트렁크
									if( (uiDoorOpen & 0x20) == 0x20 )	g_uiActFailReason |= eRETURNFAIL_TYPE_HOODOPEN;	//후드
								}
								else												g_uiActFailReason |= eRETURNFAIL_TYPE_DOOROPEN;
							}
						}
					}
				}
			}
			else if( g_eActuatorType ==  ACTUATOR_TYPE_ENGINESTOP)
			{
				SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
			}
			else if( g_eActuatorType ==  ACTUATOR_TYPE_AIRCON)
			{
				SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
			}
			else if( g_eActuatorType ==  ACTUATOR_TYPE_REARDEFOG)
			{
				SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
			}
			else if( g_eActuatorType ==  ACTUATOR_TYPE_HIGHBEAM)
			{
				SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
			}
			else if( g_eActuatorType ==  ACTUATOR_TYPE_AIRCON_STOP)
			{
				SetActuatorStatus( ACTUATOR_STATUS_CHECK_SUCCESS );
			}
		}
	}
}

void OBDRunningInfo_ControlDB()
{
	bool bRet=true;

	if( g_bCtrlDBSetFlag == false )
	{
		Oem_CAN_None_Receive_Set();	//여기서 설정안해주면 채널1번이 열려있어서 오버플로우남 이함수가 문제되면 CAN1만 막아도될듯
//		if(CFD_GetCanFDAdapter())
//		{
//			CFD_SetByPassFlagAllOn();
//		}
		g_bCtrlDBSetFlag = true;
		bRet = Can_Actuator_Set(ACTUATOR_TYPE_VEHICLE_CHECK, RES_TYPE);
		SetActuatorStatus( ACTUATOR_STATUS_CTRL_CHECKING );
		g_bDBParsingFailFlag_Act=false;
		if (g_stActuatorReadyInfo.m_uiCount==0 || bRet==false )
		{
//			SetActuatorStatus( ACTUATOR_STATUS_NONEDB );
			Trace(0,"Can't find FuncIndex_Res\r\n");
             g_bDBParsingFailFlag_Act=true;
             SetActuatorStatus( ACTUATOR_STATUS_CTRL_FAIL );
			return;
		}
	}
	else
	{
		if( g_uiActCheckFirstTimeFlag == 0 )
		{
			g_uiActCheckFirstTimeFlag=1;
			g_uiActCheckTime=Get_Tmr();
		}
	}
}

void OBDActuator_Req()
{
	static unsigned int s_uiFrameTime = 0;
	unsigned int uiDataLen=10;
	stPASSTHRU_MSG stData;
	static unsigned int s_uiOldTime=0;
	BOOL bStandardCan;
	unsigned char ucTemp[12]={0,};
	int i=0;
	static unsigned int s_uiSaveCanSpeed=0;
	static unsigned char s_ucSaveCanLine=0;
	unsigned char ucCanBPS=0;

	//////////////////////////////////////////////////
	unsigned int  nCanID = 0;
	unsigned char ucLength = 0;
	unsigned char ucMainCanLine = 0;
	unsigned char ucFDCanLine = 0;
	unsigned char ucFDCanBaudRate = 0;
	unsigned char ucFDCanFrame = 0;
	boolean_t     bSendCanFDType = false;
	
	if( g_bActuRunningFlag == false )	//REQ중이면 true
	{
		g_ucSecquence = 0;	//REQ리스트중 몇번째 REQ인지
		g_uiReqCount = 0;		//해당REQ 반복횟수
#if 0
		if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence].m_uiTiming == 0)	s_uiFrameTime=1;	//0일경우아래 -1해줘서 에러남
		else	s_uiFrameTime = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence].m_uiTiming;	//REQ사이의 간격
#else
		s_uiFrameTime = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence].m_uiTiming;	//REQ사이의 간격
#endif
		s_uiOldTime = 0;
		g_bActuRunningFlag = true;
		SetActuatorStatus( ACTUATOR_STATUS_CTRL_RUNNING );
	}

	stData.ProtocolID = ISO15765;
	
	g_uiCanWriteMsgLength = 0;
	uiDataLen = 10 + 24;	//24 : stPASSTHRU_MSG 의 앞부분
	stData.DataSize = uiDataLen-24;	//10

//	if(Get_TmrDelta(Get_Tmr(),s_uiOldTime) >= (s_uiFrameTime-1))		//실제 1ms넣었을경우 2.x이상의 속도가 나옴, 추후 개선필요(DB에 1ms일경우 -1해서 0ms 기준으로 1.2ms정도로 나감)
	
	if(Get_TmrDelta(Get_Tmr(),s_uiOldTime) >= (s_uiFrameTime))		//실제 1ms넣었을경우 2.x이상의 속도가 나옴, 추후 개선필요(DB에 1ms일경우 -1해서 0ms 기준으로 1.2ms정도로 나감)
	{
//		memset(&ucTemp, 0x00, sizeof(ucTemp));

		//CanFD
		nCanID = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal;  
		ucLength = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucLength;
		
		memset(&ucTemp, 0x00, sizeof(ucTemp));
		
		if(0x7FF < nCanID)
		{
			stData.ProtocolID = ISO15765_29BIT;
			ucTemp[0] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal>>24) 	& 0xFF;
			ucTemp[1] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal>>16)   & 0xFF;
			ucTemp[2] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal>>8) 	& 0xFF;
			ucTemp[3] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal)       & 0xFF;
			memcpy(&ucTemp[4],g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucData ,8);
		}
		else
		{
			ucTemp[0] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal>>8) 	& 0xFF;
			ucTemp[1] = (g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiReqVal)       & 0xFF;
			memcpy(&ucTemp[2],g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucData ,8);
		}
		VCI_SetPassThruProtocolID((unsigned char*)&stData.ProtocolID);
//#define ENABLE_LOG_AIRCON
#ifdef ENABLE_LOG_AIRCON
		if( g_stActuatorReqInfo[g_ucReqIndexPos].m_uiFuncIndex == ACTUATOR_TYPE_AIRCON )
			printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
#endif

		for(i=0; i<sizeof(ucTemp); i++)
		{
			if( ucTemp[i] == 'X' )	//X
			{
				ucTemp[i] = g_stReportSmartReq.Temperature&0xFF;
			}
			else if( ucTemp[i] == 'Y' )	//Y
			{
				ucTemp[i] = (g_stReportSmartReq.Temperature>>8)&0xFF;
			}
			else if( ucTemp[i] == 'Z' )	//Z
			{
				ucTemp[i]  = ReplaceData( g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence , 'Z', g_stReportSmartReq.Defrost);
			}
			else if( ucTemp[i] == 'L' )	//L
			{
				ucTemp[i]  = ReplaceData( g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence , 'L', 1);
			}
			
#ifdef ENABLE_LOG_AIRCON
			if( g_stActuatorReqInfo[g_ucReqIndexPos].m_uiFuncIndex == ACTUATOR_TYPE_AIRCON || g_stActuatorReqInfo[g_ucReqIndexPos].m_uiFuncIndex == ACTUATOR_TYPE_VENT)
				printf("%02X ",ucTemp[i] );
#endif
		}

#ifdef ENABLE_LOG_AIRCON		
		if( g_stActuatorReqInfo[g_ucReqIndexPos].m_uiFuncIndex == ACTUATOR_TYPE_AIRCON  || g_stActuatorReqInfo[g_ucReqIndexPos].m_uiFuncIndex == ACTUATOR_TYPE_VENT)
			printf("\r\n");
#endif
		switch( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiCanSpeed )
		{
			case 1024	:	ucCanBPS = eCAN_1MBPS;		break;
			case 500	:	ucCanBPS = eCAN_500KBPS;	break;
			case 250	:	ucCanBPS = eCAN_250KBPS;	break;
			case 125	:	ucCanBPS = eCAN_125KBPS;	break;
			case 100	:	ucCanBPS = eCAN_100KBPS;	break;
			case 50	:	ucCanBPS = eCAN_50KBPS;		break;
			default:	Trace(0,"Not Found CanBPS_R\r\n");	break;
		}

		if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiCanSpeed != s_uiSaveCanSpeed ||
		    g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine != s_ucSaveCanLine)	//같은 REQ내에서 라인이나 속도가 달라질 경우 재설정 필요
		{
			s_uiSaveCanSpeed = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_uiCanSpeed;
			s_ucSaveCanLine = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine;

#if defined(ACTUATOR_LOG)
//		Trace(ACTUATOR_LOG,"send :%d\r\n",g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine);
#endif
			if(g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine == HIGHCAN1 )
			{
				Oem_CAN_Initial_CH1(Highcan1, ucCanBPS);
			}
			else if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine == HIGHCAN2 )
			{
				Oem_CAN_Initial_CH1(Highcan2, ucCanBPS);
			}
			else if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine == HIGHCAN3 )
			{
				Oem_CAN_Initial_CH2(Highcan3, ucCanBPS);
			}
			else
			{
				Oem_CAN_Initial_CH2(Lowcan1, ucCanBPS);
			}
		}

		//////////////////////////////////////////////////
		//CanFD 로직 보강
		//////////////////////////////////////////////////
		
		if(CFD_GetCanFDAdapter())
		{
			ucMainCanLine = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine;		
			
			if(ucMainCanLine == HIGHCAN2 || ucMainCanLine == HIGHCAN3)
			{
				ucFDCanLine  	= g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucFDCanLine;
				ucFDCanFrame 	= g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucFDCanFrame;
				ucFDCanBaudRate = g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucFDCanBaudRate;
				
				if(ucFDCanLine >= CANFD_SPI_1 && ucFDCanLine <= CANFD_SPI_5)
				{
					CFD_SetTransceiverMode(ucFDCanLine,ucFDCanFrame);
				}

				CFD_SetCanFDConfig(ucFDCanLine,ucFDCanBaudRate);
				CFD_SetTxSpiNumber(ucMainCanLine,ucFDCanLine);

				if(ucLength > 8)
					bSendCanFDType = true;	
			}	
		}
			
		if(CFD_GetCanFDAdapter() && bSendCanFDType)
            {
                CFD_SendMultiAllCmd(nCanID,ucLength);
            }
		else 
		{
			memcpy(stData.pData,ucTemp ,stData.DataSize);
			if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine == HIGHCAN1 || g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine == HIGHCAN2 )
			{
				if( g_stActuatorReqInfo[g_ucReqIndexPos].m_uiFuncIndex == ACTUATOR_TYPE_AIRCON || g_stActuatorReqInfo[g_ucReqIndexPos].m_uiFuncIndex == ACTUATOR_TYPE_VENT)
				{
					//Trace(0,"defrost[%02X] ",g_stReportSmartReq.Defrost );
					//Trace(0,"%02X %02X %02X %02X %02X %02X %02X\r\n ",stData.pData[0],stData.pData[1],stData.pData[2],stData.pData[3],stData.pData[4],stData.pData[5],stData.pData[6] );
				}
				memcpy(&g_stWritePassThruMsg, &stData, uiDataLen);
				memset(&g_OutCanPacket, 0x00, sizeof(stCanPacket));
				CAN_TxParsing(&g_OutCanPacket, &g_stWritePassThruMsg, &bStandardCan, eDCAN);
				CAN_MakeTxSingleFrame(&g_OutCanPacket, bStandardCan, &g_stWritePassThruMsg, eDCAN);
			}
			else
			{
				memcpy(&g_stWritePassThruMsg_L, &stData, uiDataLen);
				memset(&g_OutLCanPacket, 0x00, sizeof(stCanPacket));
				CAN_TxParsing(&g_OutLCanPacket, &g_stWritePassThruMsg_L, &bStandardCan, eLCAN);
				CAN_MakeTxSingleFrame(&g_OutLCanPacket, bStandardCan, &g_stWritePassThruMsg_L, eLCAN);
			}
		}

//		if(g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine == HIGHCAN1 )
//		{
//			CAN_TxParsing(&g_OutCanPacket, &g_stWritePassThruMsg, &bStandardCan, eDCAN);
//			CAN_MakeTxSingleFrame(&g_OutCanPacket, bStandardCan, &g_stWritePassThruMsg, eDCAN);
//		}
//		else if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine == HIGHCAN2 )
//		{
//			CAN_TxParsing(&g_OutCanPacket, &g_stWritePassThruMsg, &bStandardCan, eDCAN);
//			CAN_MakeTxSingleFrame(&g_OutCanPacket, bStandardCan, &g_stWritePassThruMsg, eDCAN);
//		}
//		else if( g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence ].m_ucCanLine == HIGHCAN3 )
//		{
//			CAN_TxParsing(&g_OutLCanPacket, &g_stWritePassThruMsg_L, &bStandardCan, eLCAN);
//			CAN_MakeTxSingleFrame(&g_OutLCanPacket, bStandardCan, &g_stWritePassThruMsg_L, eLCAN);
//		}
//		else
//		{
//			CAN_TxParsing(&g_OutLCanPacket, &g_stWritePassThruMsg_L, &bStandardCan, eLCAN);
//			CAN_MakeTxSingleFrame(&g_OutLCanPacket, bStandardCan, &g_stWritePassThruMsg_L, eLCAN);
//		}
#if defined(ACTUATOR_LOG)
//		Trace(ACTUATOR_LOG,"g_uiReq: %d\r\n",g_uiReqCount);
#endif
		g_uiReqCount++;
		if( g_uiReqCount >= g_stActuatorReqData[ g_stActuatorReqInfo[g_ucReqIndexPos].m_ucIndexPos + g_ucSecquence].m_uiTimes )	//REQ반복 횟수
		{
			s_ucSaveCanLine=0;
			s_uiSaveCanSpeed=0;
			g_uiReqCount=0;
			g_ucSecquence++;
			if( g_ucSecquence >= g_stActuatorReqInfo[g_ucReqIndexPos].m_uiCount )	//REQ리스트의 마지막 라인
			{
				g_ucSecquence = 0;
				g_bActuRunningFlag = false;	//한 REQ 세트를 끝냈다
				g_ucRetryCount++;
				SetActuatorStatus( ACTUATOR_STATUS_CTRL_CHECKING );
				g_uiActCheckFirstTimeFlag = 0;
			}
		}
		s_uiOldTime = Get_Tmr();
	}
}

unsigned char ReplaceData(unsigned int uiIndex, unsigned char ucSeparator, unsigned char ucOrder)
{
	int i=0;

	for(i=0; i<ACTUATOR_MAX_CONVERT_CNT; i++)
	{
		if( (g_stActuatorConvertData[i].m_uiFuncIndex == g_stActuatorReqData[uiIndex].m_uiFuncIndex) &&  (g_stActuatorConvertData[i].m_ucSeparator == ucSeparator) )
		{
			if( ucOrder == 1 )
				return g_stActuatorConvertData[i].m_ucOn;
			else if( ucOrder == 0 )
				return g_stActuatorConvertData[i].m_ucOff;
			else return 0x00;
		}
	}
	return 0;
}

unsigned int OBDActuator_Res()
{
	unsigned int uiDecision=0;
	unsigned int uiInputStatus = 0;
	char * carrSeriaNumber = GetFWSerialNumber();

	uiInputStatus = g_stActuatorResData[ g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos ].m_uiFuncIndex;

	if( uiInputStatus == ACTUATOR_TYPE_DOORUNLOCK )
	{
		if( (Get_DoorLock()&0x0F) == 0x0F )	//언락일시 다열려있어야 성공
		{
			uiDecision = 0;
		}
		else
		{
			uiDecision = (~(Get_DoorLock()))&0x0F;
		}
#if defined(ACTUATOR_LOG)
				Trace(ACTUATOR_LOG,"DCS_DOOR_UNLOCK: %d \r\n",uiDecision);
#endif
	}
	else if( uiInputStatus == ACTUATOR_TYPE_DOORLOCK )
	{
		if( (Get_DoorLock()&0x0F) == 0x00 && (Get_DoorLock()&eRETURNFAIL_TYPE_TRUNKOPEN) == 0x00)	//언락일시 다닫혀있어야 성공
		{
			uiDecision = 0;
		}
		else
		{
			uiDecision = Get_DoorLock() & 0x0F;
			uiDecision = uiDecision | (Get_DoorOpen()&eRETURNFAIL_TYPE_TRUNKOPEN);	//트렁크열림상태
		}
#if defined(ACTUATOR_LOG)
				Trace(ACTUATOR_LOG,"DCS_DOOR_LOCK: %d \r\n",uiDecision);
#endif
	}
	else if( uiInputStatus == ACTUATOR_TYPE_WAKEUP )
	{
#if defined(ACTUATOR_LOG)
		Trace(ACTUATOR_LOG,"%d\r\n",g_ucActRetry);
#endif
        //국가코드 AU, NZ , EV 및 PHEV 개발당시 발생한 이슈로 인한 적용 , 정확한 이력파악이 어려워 지울 수 없음 
		if( ( strstr(carrSeriaNumber,"AU") != NULL ) || (strstr(carrSeriaNumber,"NZ") != NULL ))
		{ 
			g_bCanDataFlag_Wakeup=true; //20210412 확인완료 
		}
		
		if( g_bCanDataFlag_Wakeup == true )	uiDecision = 0;			//ACTUATOR_STATUS_CTRL_SUCCESS
		else													uiDecision = eRETURNFAIL_TYPE_WAKEUPFAIL;	//ACTUATOR_STATUS_CTRL_RETRY
	}
	else if( uiInputStatus == ACTUATOR_TYPE_LAMP )
	{
		if( g_eLampCheckState == eLAMP_STATE_LAMPON )
		{
			uiDecision = 0;
		}
		else
		{
			uiDecision = eRETURNFAIL_TYPE_LAMPOFF;
		}
//		for(i=0; i<g_stActuatorResInfo[g_ucResIndexPos].m_uiCount; i++)
//		{
//			ucTempPos = g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i;	//보기 힘들어서 대치
//			if(strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_HAZARD_LAMP, DCS_CURRENT_INDEX_SIZE)==0)
//			{
//				uiTempVal = Get_EngHazardLampStatus();
//				uiTempVal = uiTempVal>>1 & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal)<<2);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_HAZARD_LAMP: %d \r\n",uiTempVal);
//#endif
//			}
//		}
	}
	else if( uiInputStatus == ACTUATOR_TYPE_LAMPHORN )
	{
		if( (g_eLampCheckState == eLAMP_STATE_LAMPON) && (g_eHornCheckState == eHORN_STATE_HORNON) )
		{
			uiDecision = 0;
		}
		else
		{
			if( g_eLampCheckState != eLAMP_STATE_LAMPON )	uiDecision = eRETURNFAIL_TYPE_LAMPOFF;	//램프실패
			if( g_eHornCheckState != eHORN_STATE_HORNON )		uiDecision = uiDecision | eRETURNFAIL_TYPE_HORNOFF;	//혼실패
		}
//		for(i=0; i<g_stActuatorResInfo[g_ucResIndexPos].m_uiCount; i++)
//		{
//			ucTempPos = g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i;	//보기 힘들어서 대치
//			if(strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_HAZARD_LAMP, DCS_CURRENT_INDEX_SIZE)==0)
//			{
//				uiTempVal = Get_EngHazardLampStatus();
//				uiTempVal = uiTempVal & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal));
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_HAZARD_LAMP: %d \r\n",uiTempVal);
//#endif
//			}
//			if(strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_HORN_STATUS, DCS_CURRENT_INDEX_SIZE)==0)
//			{
//				uiTempVal = Get_EngHornStatus();
//				uiTempVal = uiTempVal & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal)<<1);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_HORN_STATUS: %d \r\n",uiTempVal);
//#endif
//			}
//		}
	}
	else if( uiInputStatus == ACTUATOR_TYPE_AIRCON )
	{
		if( FUELTYPE_GET_STATE() == EV_NONE_READY )
		{
			if( g_stReportSmartReq.Defrost==0x01 )
			{
				if( (Get_EngAirconTemperature() == g_stReportSmartReq.CheckTemperature && Get_FATCState() == 1 ) && ((Get_EngVentStatus()&0x01)== 0x01) )
				{
					g_uiEngStopTimer = Get_Tmr();
					uiDecision = eRETURNFAIL_TYPE_NONE;
				}
				else
				{
					if( Get_EngAirconTemperature() != g_stReportSmartReq.CheckTemperature || Get_FATCState() != 1 )	uiDecision = eRETURNFAIL_TYPE_AIRCON_FAIL;	//에어컨
					if(( Get_EngVentStatus()&0x01) != 0x01 )		uiDecision = uiDecision | eRETURNFAIL_TYPE_ENGON_VENTFAIL;	//성애제거
				}
			}
			else	//에어컨제어만
			{
				if( Get_EngAirconTemperature() == g_stReportSmartReq.CheckTemperature && Get_FATCState() == 1)
				{
					if( g_bEngRunContinueSuppFlag == true )	g_bEngRunKeepFlag = true;
					g_uiEngContinueSendTimer = Get_Tmr();
					g_uiEngStopTimer = Get_Tmr();
					uiDecision = eRETURNFAIL_TYPE_NONE;
				}
				else
				{
					uiDecision = eRETURNFAIL_TYPE_AIRCON_FAIL;	//에어컨
				}
			}
		}
		else
		{
			if( g_stReportSmartReq.Defrost==0x01 )
			{
				if( (Get_EngAirconTemperature() == g_stReportSmartReq.CheckTemperature ) && ((Get_EngVentStatus()&0x01)== 0x01) )
				{
					uiDecision = eRETURNFAIL_TYPE_NONE;
				}
				else
				{
					if( Get_EngAirconTemperature() != g_stReportSmartReq.CheckTemperature )	uiDecision = eRETURNFAIL_TYPE_ENGON_AIRCONFAIL;	//에어컨
					if(( Get_EngVentStatus()&0x01) != 0x01 )		uiDecision = uiDecision | eRETURNFAIL_TYPE_ENGON_VENTFAIL;	//성애제거
				}
			}
			else	//에어컨제어만
			{
				if( Get_EngAirconTemperature() == g_stReportSmartReq.CheckTemperature )
				{
					uiDecision = eRETURNFAIL_TYPE_NONE;
				}
				else
				{
					uiDecision = eRETURNFAIL_TYPE_ENGON_AIRCONFAIL;	//에어컨
				}
			}
#if 1 // James Jean 2018/11/18
			// 전기차인 경우에는 엔진 시동후에 엔진과 공조 체크를 동시에하도록 변경. 여기서는 성공으로 리턴
			if( FUELTYPE_GET_STATE() == ELECTRONIC )
				uiDecision = eRETURNFAIL_TYPE_NONE;
#endif
		}
	}
	else if( uiInputStatus == ACTUATOR_TYPE_REARDEFOG )
	{
		if( Get_RearDefogStatus() == (bool)g_stReportSmartReq.RearDefogger )
		{
			uiDecision = 0;
		}
		else
		{
			uiDecision = eRETURNFAIL_TYPE_REARDEFOG_FAIL;
		}
	}
//	else if( uiInputStatus == ACTUATOR_TYPE_HIGHBEAM )
//	{
//		if( Get_HighBeamStatus() == (bool)g_stReportSmartReq.HighBeam )
//		{
//			uiDecision = 0;
//		}
//		else
//		{
//			uiDecision = eRETURNFAIL_TYPE_HIGHBEAM_FAIL;
//		}
//	}
	else if( uiInputStatus == ACTUATOR_TYPE_IG3ON )
	{
		if( Get_IG3_Status() == 1 )
		{
			uiDecision = 0;
		}
		else
		{
			uiDecision = eRETURNFAIL_TYPE_IG3_FAIL;
		}
	}
	else if( uiInputStatus == ACTUATOR_TYPE_ENGINERUN )
	{
		if( (FUELTYPE_GET_STATE() == FCEV) || FUELTYPE_GET_STATE() == ELECTRONIC || (FUELTYPE_GET_STATE() == GASOLINE_HEV) || FUELTYPE_GET_STATE() == PLUGIN_HEV )
		{
			printf("[POWER : %d %d]", Get_EngRun_Status(),FUELTYPE_GET_STATE());
			if( Get_EngRun_Status() == eENGRUN_ON)
			{
				if( g_bEngRunContinueSuppFlag == true )	g_bEngRunKeepFlag = true;
				g_bEngRunActFlag = true;
				g_uiEngContinueSendTimer = Get_Tmr();
				g_uiEngStopTimer = Get_Tmr();
				uiDecision = eRETURNFAIL_TYPE_NONE;

#if 1 // James Jean 2018/11/18
                if( FUELTYPE_GET_STATE() == ELECTRONIC )
                {
                    // 전기차인 경우에는 엔진 시동후에 엔진과 공조 체크를 동시에하여 성공 실패를 올려 준다.
                    unsigned int uiAirConDecision = eRETURNFAIL_TYPE_NONE;
                    
                     if( g_stReportSmartReq.Defrost==0x01 )
            		 {
            			if( (Get_EngAirconTemperature() == g_stReportSmartReq.CheckTemperature ) && ((Get_EngVentStatus()&0x01)== 0x01) )
            			{
            				uiAirConDecision = eRETURNFAIL_TYPE_NONE;
            			}
            			else
            			{
            				if( Get_EngAirconTemperature() != g_stReportSmartReq.CheckTemperature )	uiAirConDecision = eRETURNFAIL_TYPE_ENGON_AIRCONFAIL;	//에어컨
            				if(( Get_EngVentStatus()&0x01) != 0x01 )		uiAirConDecision = uiAirConDecision | eRETURNFAIL_TYPE_ENGON_VENTFAIL;	//성애제거
            			}
            		 }
            		 else
            		 {
            			 if( Get_EngAirconTemperature() == g_stReportSmartReq.CheckTemperature )
            				uiAirConDecision = eRETURNFAIL_TYPE_NONE;
            		 }

                     if( uiAirConDecision != eRETURNFAIL_TYPE_NONE )
                     {
                        uiDecision = uiAirConDecision;
                     }
                }
#endif
			}
			else
			{
				uiDecision = eRETURNFAIL_TYPE_ENGON_FAIL;	//RPM없음
			}
		}
		else
		{
			printf("[RPM : %d]", Get_RPM());
			if( Get_RPM() >= DECIDE_ENGINE_STARTING_RPM )
			{
				if( g_bEngRunContinueSuppFlag == true )	g_bEngRunKeepFlag = true;
				g_bEngRunActFlag = true;
				g_uiEngContinueSendTimer = Get_Tmr();
				g_uiEngStopTimer = Get_Tmr();
				uiDecision = 0;
			}
			else
			{
				uiDecision = eRETURNFAIL_TYPE_ENGON_FAIL;	//RPM없음
			}
		}
	}
	else if( uiInputStatus == ACTUATOR_TYPE_AIRCON_STOP )
	{
		if( FUELTYPE_GET_STATE() == EV_NONE_READY )
		{
			if( Get_FATCState() == 0 )
			{
				uiDecision = 0;
			}
			else
			{
				uiDecision = eRETURNFAIL_TYPE_ENGOFF_FAIL;	//에어컨정지실패 추후 수정필요
			}
		}
	}
	else if( uiInputStatus == ACTUATOR_TYPE_ENGINESTOP )
	{
		if( (FUELTYPE_GET_STATE() == FCEV) || FUELTYPE_GET_STATE() == ELECTRONIC || (FUELTYPE_GET_STATE() == GASOLINE_HEV) || FUELTYPE_GET_STATE() == PLUGIN_HEV || FUELTYPE_GET_STATE() == EV_NONE_READY )
		{
			if( Get_EngRun_Status() == eENGRUN_OFF )
			{
				uiDecision = 0;
			}
			else
			{
				uiDecision = eRETURNFAIL_TYPE_ENGOFF_FAIL;	//RPM있음
			}
		}
		else
		{
			printf("[RPM : %d]", Get_RPM());
			if( Get_RPM() == DECIDE_ENGINE_STOP_RPM )
			{
				uiDecision = 0;
			}
			else
			{
				uiDecision = eRETURNFAIL_TYPE_ENGOFF_FAIL;	//RPM있음
			}
		}
//		for(i=0; i<g_stActuatorResInfo[g_ucResIndexPos].m_uiCount; i++)
//		{
//			ucTempPos = g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i;	//보기 힘들어서 대치
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_RPM, DCS_CURRENT_INDEX_SIZE) == 0 )	//E02
//			{
//				uiTempVal = Get_RPM();
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal ));
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_RPM: %d \r\n",uiTempVal);
//#endif
//			}
//		}
	}
	else	//현재 else문 미사용중 사용할때 도어상태 다시체크 해야함
	{
//		for(i=0; i<g_stActuatorResInfo[g_ucResIndexPos].m_uiCount; i++)
//		{
//			ucTempPos = g_stActuatorResInfo[g_ucResIndexPos].m_ucIndexPos + i;	//보기 힘들어서 대치
//
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_DOOR_RL_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S08
//			{
//				uiTempVal = Get_DoorLock();
//				uiTempVal = uiTempVal>>2 & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal));
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_DOOR_RL_UNLOCK: %d \r\n",uiTempVal);
//#endif
//			}
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_DOOR_RR_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S09
//			{
//				uiTempVal = Get_DoorLock();
//				uiTempVal = uiTempVal>>3 & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal)<<1);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_DOOR_RR_UNLOCK: %d \r\n",uiTempVal);
//#endif
//			}
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_DOOR_FR_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S0A
//			{
//				uiTempVal = Get_DoorLock();
//				uiTempVal = uiTempVal>>1 & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal)<<2);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_DOOR_FR_UNLOCK: %d \r\n",uiTempVal);
//#endif
//			}
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_DOOR_FL_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S0B
//			{
//				uiTempVal = Get_DoorLock();
//				uiTempVal = uiTempVal & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal)<<3);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_DOOR_FL_UNLOCK: %d \r\n",uiTempVal);
//#endif
//			}
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_DOOR_RL_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S01
//			{
//				uiTempVal = Get_DoorOpen();
//				uiTempVal = uiTempVal>>2 & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal )<<6);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_DOOR_RL_OPEN: %d \r\n",uiTempVal);
//#endif
//			}
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_DOOR_RR_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S02
//			{
//				uiTempVal = Get_DoorOpen();
//				uiTempVal = uiTempVal>>3 & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal )<<7);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_DOOR_RR_OPEN: %d \r\n",uiTempVal);
//#endif
//			}
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_DOOR_FR_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S03
//			{
//				uiTempVal = Get_DoorOpen();
//				uiTempVal = uiTempVal>>1 & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal)<<5);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_DOOR_FR_OPEN: %d \r\n",uiTempVal);
//#endif
//			}
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_DOOR_FL_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S04
//			{
//				uiTempVal = Get_DoorOpen();
//				uiTempVal = uiTempVal & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal )<<4);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_DOOR_FL_OPEN: %d \r\n",uiTempVal);
//#endif
//			}
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_TRUNK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S06
//			{
//				uiTempVal = Get_DoorOpen();
//				uiTempVal = uiTempVal>>4 & 0x01;
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal )<<8);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_TRUNK: %d \r\n",uiTempVal);
//#endif
//			}
//			if( strncmp(g_stActuatorResData[ ucTempPos ].m_cIndex, DCS_RPM, DCS_CURRENT_INDEX_SIZE) == 0 )	//E02
//			{
//				uiTempVal = Get_RPM();
//				uiDecision |= (StatusDecision( ucTempPos, uiTempVal )<<12);
//#if defined(ACTUATOR_LOG)
//				Trace(ACTUATOR_LOG,"DCS_RPM: %d \r\n",uiTempVal);
//#endif
//			}
//		}
	}

	if( uiDecision == 0 )
	{
//		if( uiInputStatus == ACTUATOR_TYPE_DOORLOCK )	g_bLockCtrlFlag = true;
		SetActuatorStatus( ACTUATOR_STATUS_CTRL_SUCCESS );
	}
	else
	{
		SetActuatorStatus( ACTUATOR_STATUS_CTRL_RETRY );
		g_uiActCheckTime=Get_Tmr(); //명령쏘고 대기하는 시간 초기화
	}
//0000 0000 0000 0000 0000 0000 0000 0001b : RL락/언락(언락시 락상태/락시 언락상태)
//0000 0000 0000 0000 0000 0000 0000 0010b : RR락/언락(언락시 락상태/락시 언락상태)
//0000 0000 0000 0000 0000 0000 0000 0100b : FR락/언락(언락시 락상태/락시 언락상태)
//0000 0000 0000 0000 0000 0000 0000 1000b : FL락/언락(언락시 락상태/락시 언락상태)
//0000 0000 0000 0000 0000 0000 0001 0000b : 트렁크열림상태
//0000 0000 0000 0000 0000 0000 0010 0000b : CAN있음(통신실패)
//0000 0000 0000 0000 0000 0000 0100 0000b : LAMP OFF
//0000 0000 0000 0000 0000 0000 1000 0000b : HORN OFF
//0000 0000 0000 0000 0000 0001 0000 0000b : A/C실패
//0000 0000 0000 0000 0000 0010 0000 0000b : VENT실패
//0000 0000 0000 0000 0000 0100 0000 0000b : RPM없음
//0000 0000 0000 0000 0000 1000 0000 0000b : RPM있음
//0000 0000 0000 0000 0001 0000 0000 0000b : DB파싱실패
//0000 0000 0000 0000 1000 0000 0000 0000b : WAKE UP실패
	return uiDecision;
}

bool StatusDecision(unsigned char ucTempPos, unsigned int uiInputValue)
{
	unsigned char ucDecision=1;
#if defined(ACTUATOR_LOG)
	Trace(ACTUATOR_LOG,"StatusDecision: Pos:0x%X(%d) uiInputValue:0x%X(%d) m_cCompType:0x%X(%d)\r\n",ucTempPos,ucTempPos,uiInputValue,uiInputValue,g_stActuatorResData[ ucTempPos ].m_cCompType,g_stActuatorResData[ ucTempPos ].m_cCompType);
#endif
	if( g_stActuatorResData[ ucTempPos ].m_cCompType == 0 )						ucDecision = 0;
	else if( g_stActuatorResData[ ucTempPos ].m_cCompType == 1 ){
		if( uiInputValue > g_stActuatorResData[ ucTempPos ].m_uiCompVal )	ucDecision = 0;
		else																									ucDecision = 1;
	}
	else if( g_stActuatorResData[ ucTempPos ].m_cCompType == 2 ){
		if( uiInputValue < g_stActuatorResData[ ucTempPos ].m_uiCompVal )	ucDecision = 0;
		else																									ucDecision = 1;
	}
	else if( g_stActuatorResData[ ucTempPos ].m_cCompType == 3 ){
		if( uiInputValue >= g_stActuatorResData[ ucTempPos ].m_uiCompVal )	ucDecision = 0;
		else																									ucDecision = 1;
	}
	else if( g_stActuatorResData[ ucTempPos ].m_cCompType == 4 ){
		if( uiInputValue <= g_stActuatorResData[ ucTempPos ].m_uiCompVal )	ucDecision = 0;
		else																									ucDecision = 1;
	}
	else if( g_stActuatorResData[ ucTempPos ].m_cCompType == 5 ){
		if( uiInputValue == g_stActuatorResData[ ucTempPos ].m_uiCompVal )	ucDecision = 0;
		else																									ucDecision = 1;
	}
	else if( g_stActuatorResData[ ucTempPos ].m_cCompType == 6 ){
		if( uiInputValue != g_stActuatorResData[ ucTempPos ].m_uiCompVal )	ucDecision = 0;
		else																									ucDecision = 1;
	}
	else
	{
		ucDecision = 0xFF;
	}
	return ucDecision;
}

bool StatusDecision_char(unsigned char ucTempPos, unsigned char ucCompValue)
{
	unsigned char ucDecision=1;

//	if( strncmp(&ucCompValue,g_stActuatorResData[ ucTempPos ].m_uiCompVal,1) == 0 )	ucDecision = true;
//	else																														ucDecision = false;

	return ucDecision;
}

boolean_t IsDBCorrect()
{
	return (g_bDBParsingFailFlag==false)?true:false;
}






void SetEvAirControl(eEvReadySet eEvSet)
{
	if(eEvSet == eIGONSet)
	{
		g_stEvAirConControl.m_bSetIG3On = true;
	}
}

bool IsNeedAirConOff()
{
	bool bResult = false;
	
	if(g_stEvAirConControl.m_bSetIG3On)
	{
		bResult = true;
	}

	return bResult;
}

void SetClearAirControl()
{
	g_stEvAirConControl.m_bSetIG3On = false;
}

bool CanBufferClear(eCLEAR_TYPE eType)
{
	unsigned int uiRet=0,uiRet2=0;
	
	if( eType == eCLEAR_TYPE_CAN1 )
	{
		while(HalCan_GetRxQueueCount(&g_CAN1_RxBuffCtrl) != 0)
		{
			uiRet = OemClearCanBuff(eCOMM_TYPE_CAN1);
		}
		if( uiRet == 0 )	return true;
	}
	else if( eType == eCLEAR_TYPE_CAN2 )
	{
		while(HalCan_GetRxQueueCount(&g_CAN2_RxBuffCtrl) != 0)
		{
			uiRet2 = OemClearCanBuff(eCOMM_TYPE_CAN2);
		}
		if( uiRet2 == 0 )	return true;
	}
	else
	{
		while(HalCan_GetRxQueueCount(&g_CAN1_RxBuffCtrl) != 0)
		{
			uiRet = OemClearCanBuff(eCOMM_TYPE_CAN1);
		}
		while(HalCan_GetRxQueueCount(&g_CAN2_RxBuffCtrl) != 0)
		{
			uiRet2 = OemClearCanBuff(eCOMM_TYPE_CAN2);
		}
		if( uiRet == 0 && uiRet2 == 0 )	return true;
	}
    return false;
}

void OBDManagerInit()
{
	if( g_ucDBParsingFlag == 0 )
	{
	freeitems();
	if(DBParser())
	{
		memset( &g_OBDControllerData,0x00,sizeof(g_OBDControllerData) );
		memset(&g_stEvAirConControl,0x00,sizeof(stEvAirConControl));

		g_OBDControllerData.currentState.doorOpen = g_stLockState.m_ucDoorOpen;
		g_OBDControllerData.currentState.doorLock = g_stLockState.m_ucDoorLock;

		//연료잔량변수 초기화
		if( g_bFuelLiterTypeFlag == true && g_bFuelLevelSwitchedPercentFlag == false )
		{
			g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter = (float)(g_ucFuelLevelMaxLiter/100.0) * g_fFuelLevelPercent;
		}
		else												g_OBDControllerData.monitering_Data.m_ucFuel_Level = (unsigned char)g_fFuelLevelPercent;

        HalDrvRdBkRam((unsigned char*)&g_unBkramTmpOdometer, BKRAM_ODOMETER_ADDR, BKRAM_ODOMETER_SIZE);
        Send_Odometer_From_CAN(g_unBkramTmpOdometer);

		memset(&g_ucFuelList,0x00,sizeof(g_ucFuelList));
		g_ucFuelListCount = 0;
		g_ucFuelListPosition = 0;
		printf("m_fFuel_Level_Liter : %f,m_ucFuel_Level:%d\r\n",g_OBDControllerData.monitering_Data.m_fFuel_Level_Liter,g_OBDControllerData.monitering_Data.m_ucFuel_Level);
		printf("StartLockState : %X %X,%d\r\n",g_stLockState.m_ucDoorOpen,g_stLockState.m_ucDoorLock,Get_Odmeter());
		DCAN_SET_COMM_STATE(eCAN_TX_NONE_PARSING);
		LCAN_SET_COMM_STATE(eLCAN_TX_NONE_PARSING);
	}
	else
	{
		g_bDBParsingFailFlag=true;
		SetOBDState(eConfiguration_Error);
	}

	if(DBParser_Actuator()==true){}		//강제구동 DB파싱
	else
	{
		g_bDBParsingFailFlag=true;
		SetOBDState(eConfiguration_Error);
		}

		g_ucDBParsingFlag = 1;
	}
	g_uiFotaTimer=0;

	SetOBDState(eOBD_Running_Info_Mode);
#ifdef CHECK_MALLOC
	__iar_dlmalloc_stats();
#endif
}
void Current_ActuatorReady(stActuatorReadyData * pstCurrDataBase, int i)
{
	if(strncmp(pstCurrDataBase[ i ].m_cIndex, DCS_H2_CONSUM, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_Fuel_Consume_From_CAN((double)g_stActuatorReadyData[ i ].m_fData, g_stActuatorReadyData[ i ].m_cE);
	}
	else if(strncmp(pstCurrDataBase[i].m_cIndex, DCS_SPEED, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_Speed_From_CAN(abs((unsigned int)g_stActuatorReadyData[i].m_fData));
	}
    else if(strncmp(pstCurrDataBase[ i ].m_cIndex, DCS_CELSIUS, DCS_CURRENT_INDEX_SIZE)==0)
    {
        Send_Celsius_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
    }
    else if(strncmp(pstCurrDataBase[ i ].m_cIndex, DCS_FAHRENHEIT, DCS_CURRENT_INDEX_SIZE)==0)
    {
        Send_Fahrenheit_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
    }
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_DOOR_RL_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S08
	{
#if defined(ACTUATOR_DATA_LOG)
		Trace(0,"RL_UNLOCK: %f \r\n",g_stActuatorReadyData[ i ].m_fData);
#endif
		Send_DoorLock_RL_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_DOOR_RR_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S09
	{
#if defined(ACTUATOR_DATA_LOG)
		Trace(0,"RR_UNLOCK: %f \r\n",g_stActuatorReadyData[ i ].m_fData);
#endif
		Send_DoorLock_RR_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_DOOR_FR_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S0A
	{
#if defined(ACTUATOR_DATA_LOG)
		Trace(0,"FR_UNLOCK: %f \r\n",g_stActuatorReadyData[ i ].m_fData);
#endif
		Send_DoorLock_FR_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_DOOR_FL_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S0B
	{
#if defined(ACTUATOR_DATA_LOG)
		Trace(0,"FL_UNLOCK: %f \r\n",g_stActuatorReadyData[ i ].m_fData);
#endif
		Send_DoorLock_FL_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_DOOR_RL_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S01
	{
		Send_DoorOpen_RL_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_DOOR_RR_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S02
	{
		Send_DoorOpen_RR_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_DOOR_FR_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S03
	{
//						Trace(0,"FR_L : %d \r\n",(unsigned char)g_stActuatorReadyData[ i ].m_fData);
		Send_DoorOpen_FR_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_DOOR_FL_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S04
	{
//						Trace(0,"FL_L : %d \r\n",(unsigned char)g_stActuatorReadyData[ i ].m_fData);
		Send_DoorOpen_FL_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_TRUNK, DCS_CURRENT_INDEX_SIZE) == 0 )					//S06
	{
		Send_TrunkOpen_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HOOD, DCS_CURRENT_INDEX_SIZE) == 0 )	//E12
	{
		Send_HoodOpen_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_AIRCON_TEMPERATURE, DCS_CURRENT_INDEX_SIZE)==0)			//E26
	{
		Send_AirconTemperature_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_VENT_STATUS, DCS_CURRENT_INDEX_SIZE)==0)		//E27
	{
		Send_VentStatus_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_POWER_ACC, DCS_CURRENT_INDEX_SIZE)==0)	//B09
	{
		if( g_bIndicatorDBFlag == true )
		{
			//if(Get_UserStatus() == eUSER_IN)	//사양에서는 거의 ACC의 값으로 USER 값을 대입한다고 함. 하지만 시동OFF 즉시 eUSER_OUT 되므로 DATA 체크하지 못하는 문제 발생하여 막음 210303 LWH NEXO
			{
#if defined(ENGINE_LOG)
			printf("ACC_! : %f\r\n",g_stActuatorReadyData[ i ].m_fData);
			Trace(0,"ACC_CTRL : %f\r\n",g_stActuatorReadyData[ i ].m_fData);
#endif
				Send_ACC_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
			}
		}
		else Send_ACC_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_POWER_IG1, DCS_CURRENT_INDEX_SIZE)==0)		//B0A
	{
		if( g_bIndicatorDBFlag == true )
		{
			//if(Get_UserStatus() == eUSER_IN)		//사양에서는 거의 ACC의 값으로 USER 값을 대입한다고 함. 하지만 시동OFF 즉시 eUSER_OUT 되므로 DATA 체크하지 못하는 문제 발생하여 막음 210303 LWH NEXO
			{
#if defined(ENGINE_LOG)
			printf("IG1! : %f\r\n",g_stActuatorReadyData[ i ].m_fData);
#endif
				Send_IG1_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
			}
		}
		else Send_IG1_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_POWER_IG2, DCS_CURRENT_INDEX_SIZE)==0)		//B0B
	{
		if( g_bIndicatorDBFlag == true )
		{
			//if(Get_UserStatus() == eUSER_IN)		//사양에서는 거의 ACC의 값으로 USER 값을 대입한다고 함. 하지만 시동OFF 즉시 eUSER_OUT 되므로 DATA 체크하지 못하는 문제 발생하여 막음 210303 LWH NEXO
			{
#if defined(ENGINE_LOG)
			printf("IG2! : %f\r\n",g_stActuatorReadyData[ i ].m_fData);
#endif
				Send_IG2_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
			}
		}
		else Send_IG2_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_POWER_IG3, DCS_CURRENT_INDEX_SIZE)==0)		//B1D
	{
		Send_IG3_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[i].m_cIndex, DCS_POWER_READY, DCS_CURRENT_INDEX_SIZE)==0)	//
	{
		if( g_bIndicatorDBFlag == true )
		{
			//if(Get_UserStatus() == eUSER_IN)		//사양에서는 거의 ACC의 값으로 USER 값을 대입한다고 함. 하지만 시동OFF 즉시 eUSER_OUT 되므로 DATA 체크하지 못하는 문제 발생하여 막음 210303 LWH NEXO
			{
#if defined(ENGINE_LOG)
			printf("ISG! : %f\r\n",g_stActuatorReadyData[ i ].m_fData);
#endif
				Send_EngRun_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
			}
		}
		else Send_EngRun_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_CRASH_SIGNAL, DCS_CURRENT_INDEX_SIZE)==0)		//B0D
	{
		Send_CrashSignal_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HORN_STATUS, DCS_CURRENT_INDEX_SIZE)==0)		//B0F
	{
		Send_HornStatus_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_USER_CTRL_STATUS, DCS_CURRENT_INDEX_SIZE)==0)		//B10
	{
		Send_UserStatus_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HAZARD_LAMP, DCS_CURRENT_INDEX_SIZE)==0)		//S0F
	{
		Send_hazardLamp_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_REAR_DEFOG, DCS_CURRENT_INDEX_SIZE)==0)		//B18
	{
		Send_RearDefog_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_RPM, DCS_CURRENT_INDEX_SIZE) == 0 )	//E02
	{
		Send_RPM_From_CAN((unsigned int)g_stActuatorReadyData[ i ].m_fData);
#if defined(ENGINE_LOG)
		Trace(0,"L_RPM : %f\r\n",g_stActuatorReadyData[ i ].m_fData);
#endif
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_WEAK_LAMP, DCS_CURRENT_INDEX_SIZE) == 0 )	//S07
	{
		Send_TailLamp_From_CAN((unsigned int)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HEADLAMP, DCS_CURRENT_INDEX_SIZE) == 0 )	//S05
	{
		Send_HeadLamp_From_CAN((unsigned int)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_CHARGE_STATE, DCS_CURRENT_INDEX_SIZE) == 0 )
	{
		Send_ChargeState_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_FATC_STATUS, DCS_CURRENT_INDEX_SIZE) == 0 )
	{
		Send_FATCState_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_BRAKE_SW, DCS_CURRENT_INDEX_SIZE) == 0 )
	{
		Send_FootBrake_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HIGHLIGHT_LAMP, DCS_CURRENT_INDEX_SIZE) == 0 )
	{
		Send_HighBeam_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_REAR_SEAT_SET_STATE, DCS_CURRENT_INDEX_SIZE) == 0 )
	{
		Send_RearSeatSet_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_REAR_SEAT_OCCURED, DCS_CURRENT_INDEX_SIZE) == 0 )
	{
		//printf("seat:%d\r\n",g_stActuatorReadyData[ i ].m_fData);
		Send_RearSeatOccured_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
#if defined(PROTOCOL18)
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HIGHBATT_TEMP_MAX, DCS_CURRENT_INDEX_SIZE)==0) //dahae
	{
		Send_HighBatteryTemperatureMAX_From_CAN((short)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HIGHBATT_TEMP, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_HighBatteryTemperature_From_CAN((short)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex,DCS_CHARGING_STATE,DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_ChargingState_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex,DCS_EXTRA_CHARGE_TIME,DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_ExtraChargeTime_From_CAN((unsigned char)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_EXTRA_DISTANCE, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_ExtraDrivingDistance_From_CAN((unsigned short)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_SOC_STATE, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_SOCState_From_CAN(g_stActuatorReadyData[ i ].m_fData);
	}
#endif
#if defined(PROTOCOL19)
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HYDROGEN_FUEL_LEVEL, DCS_CURRENT_INDEX_SIZE) == 0 )	//F36 수소 연료 레벨 %
	{
		Send_Hydrogen_Fuel_From_CAN(g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_AIR_PURIFICATION, DCS_CURRENT_INDEX_SIZE) == 0 )	//F37 공기 정화량 KL
	{
		Send_AirPurification_From_CAN(g_stActuatorReadyData[ i ].m_fData);
	}
	else if( strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_CO2_REDUCTION, DCS_CURRENT_INDEX_SIZE) == 0 )	//F38 CO2 감축량 Kg
	{
		Send_CO2Reduction_From_CAN(g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_BATTERY_CELL_MIN, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_MinBatteryCellVoltage_From_CAN(g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_BATTERY_CELL_MAX, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_MaxBatteryCellVoltage_From_CAN(g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HYDROGEN_CHARGE_CNT, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_HydrogenChargeCnt_From_CAN((unsigned int)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HYDROGEN_TANK_PRES, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_HydrogenTankPress_From_CAN(g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_HYDROGEN_CUR_TEMP, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_HydrogenCurrentTemperature_From_CAN((short)g_stActuatorReadyData[ i ].m_fData);
	} 
#endif
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_ABS_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_ABSIndicator_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_ISG_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_ISGIndicator_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_EPB_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_EPBIndicator_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_BRAKEJUDDER_VALUE, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_BrakeJudder_From_CAN((unsigned int)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_BMS_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_BMSIndicator_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_ENGOILPRESS_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_ENGOILPRESSIndicator_From_CAN((bool)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_INSULATION_RESISTANCE, DCS_CURRENT_INDEX_SIZE)==0) //B2A
	{
		Send_InsulationResistance_From_CAN((unsigned short)g_stActuatorReadyData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex, DCS_BATTERY_CELL_VOL_MAX, DCS_CURRENT_INDEX_SIZE)==0) // B36
	{
		Send_MaxBatteryCellVoltage_From_CAN(g_stActuatorResData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[ i ].m_cIndex,DCS_BATTERY_CELL_VOL_MIN,DCS_CURRENT_INDEX_SIZE)==0) //B37
	{
		Send_MinBatteryCellVoltage_From_CAN(g_stActuatorResData[ i ].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[i].m_cIndex, DCS_BATTERY_PACK_C, DCS_CURRENT_INDEX_SIZE)==0)	//B02
	{
		Send_BatteryPackC_From_CAN(g_stActuatorReadyData[i].m_fData);
	}
	else if(strncmp(g_stActuatorReadyData[i].m_cIndex, DCS_SOH_STATE, DCS_CURRENT_INDEX_SIZE)==0)
	{
		Send_SOHState_From_CAN(g_stActuatorReadyData[i].m_fData);
	}
}
void Current_ActuatorRes(stActuatorResData * pstCurrDataBase, int i)
{
	  if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_DOOR_RL_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S08
	  {
#if defined(ACTUATOR_DATA_LOG)
			  Trace(0,"RL_UNLOCK: %f \r\n",g_stActuatorResData[ i ].m_fData);
#endif
			  Send_DoorLock_RL_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_DOOR_RR_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S09
	  {
#if defined(ACTUATOR_DATA_LOG)
			  Trace(0,"RR_UNLOCK: %f \r\n",g_stActuatorResData[ i ].m_fData);
#endif
			  Send_DoorLock_RR_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_DOOR_FR_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S0A
	  {
#if defined(ACTUATOR_DATA_LOG)
			  Trace(0,"FR_UNLOCK: %f \r\n",g_stActuatorResData[ i ].m_fData);
#endif
			  Send_DoorLock_FR_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_DOOR_FL_UNLOCK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S0B
	  {
#if defined(ACTUATOR_DATA_LOG)
			  Trace(0,"FL_UNLOCK: %f \r\n",g_stActuatorResData[ i ].m_fData);
#endif
			  Send_DoorLock_FL_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_DOOR_RL_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S01
	  {
			  Send_DoorOpen_RL_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_DOOR_RR_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S02
	  {
			  Send_DoorOpen_RR_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_DOOR_FR_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S03
	  {
//						Trace(0,"FR_L : %d \r\n",(unsigned char)g_stActuatorResData[ i ].m_fData);
			  Send_DoorOpen_FR_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_DOOR_FL_OPEN, DCS_CURRENT_INDEX_SIZE) == 0 )	//S04
	  {
//						Trace(0,"FL_L : %d \r\n",(unsigned char)g_stActuatorResData[ i ].m_fData);
			  Send_DoorOpen_FL_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_TRUNK, DCS_CURRENT_INDEX_SIZE) == 0 )	//S06
	  {
			  Send_TrunkOpen_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_HOOD, DCS_CURRENT_INDEX_SIZE) == 0 )	//E12
	  {
			  Send_HoodOpen_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[i].m_cIndex, DCS_SPEED, DCS_CURRENT_INDEX_SIZE)==0)
	  {
		  Send_Speed_From_CAN(abs((unsigned int)g_stActuatorResData[i].m_fData));
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_AIRCON_TEMPERATURE, DCS_CURRENT_INDEX_SIZE)==0)			//E26
	  {
			  Send_AirconTemperature_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_VENT_STATUS, DCS_CURRENT_INDEX_SIZE)==0)		//E27
	  {
			  Send_VentStatus_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_POWER_ACC, DCS_CURRENT_INDEX_SIZE)==0)	//B09
	  {
#if defined(ENGINE_LOG)
				printf("ACC: %f\r\n",g_stActuatorResData[ i ].m_fData);
				Trace(0,"ACC_L : %f\r\n",g_stActuatorResData[ i ].m_fData);
#endif
			  Send_ACC_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_POWER_IG1, DCS_CURRENT_INDEX_SIZE)==0)			//B0A
	  {
#if defined(ENGINE_LOG)
			printf("IG1 : %f %d\r\n",g_stActuatorResData[ i ].m_fData,g_OBDControllerData.monitering_Data.m_usRPM);
#endif
			  Send_IG1_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_POWER_IG2, DCS_CURRENT_INDEX_SIZE)==0)			//B0B
	  {
#if defined(ENGINE_LOG)
				printf("IG2 : %f %d\r\n",g_stActuatorResData[ i ].m_fData,g_OBDControllerData.monitering_Data.m_usMaxRPM);
#endif
			  Send_IG2_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_POWER_IG3, DCS_CURRENT_INDEX_SIZE)==0)			//B1D
	  {
#if defined(ENGINE_LOG)
				printf("IG3 : %f\r\n",g_stActuatorResData[ i ].m_fData);
#endif
			  Send_IG3_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[i].m_cIndex, DCS_POWER_READY, DCS_CURRENT_INDEX_SIZE)==0)
	  {
#if defined(ENGINE_LOG)
				printf("ISG : %f\r\n",g_stActuatorResData[ i ].m_fData);
#endif
			  Send_EngRun_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_CRASH_SIGNAL, DCS_CURRENT_INDEX_SIZE)==0)		//B0D
	  {
			  Send_CrashSignal_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_HORN_STATUS, DCS_CURRENT_INDEX_SIZE)==0)		//B0F
	  {
			  Send_HornStatus_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_USER_CTRL_STATUS, DCS_CURRENT_INDEX_SIZE)==0)		//B10
	  {
//						printf("USER : %f\r\n",g_stActuatorResData[ i ].m_fData);
			  Send_UserStatus_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_HAZARD_LAMP, DCS_CURRENT_INDEX_SIZE)==0)		//S0F
	  {
	   // printf("###########%f\r\n",g_stActuatorResData[ i ].m_fData);
			  Send_hazardLamp_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_REAR_DEFOG, DCS_CURRENT_INDEX_SIZE)==0)		//S0F
	  {
			  Send_RearDefog_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
//					else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_OUT_LAMP, DCS_CURRENT_INDEX_SIZE)==0)		//S13
//					{
//						Send_OutLamp_From_CAN(g_stActuatorResData[ i ].m_fData);
//					}
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_RPM, DCS_CURRENT_INDEX_SIZE) == 0 )	//E02
	  {
			  Send_RPM_From_CAN((unsigned int)g_stActuatorResData[ i ].m_fData);
#if defined(ENGINE_LOG)
			Trace(0,"L_RPM2 : %f\r\n",g_stActuatorResData[ i ].m_fData);
#endif
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_WEAK_LAMP, DCS_CURRENT_INDEX_SIZE) == 0 )	//S07
	  {
			  Send_TailLamp_From_CAN((unsigned int)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_HEADLAMP, DCS_CURRENT_INDEX_SIZE) == 0 )	//S05
	  {
			  Send_HeadLamp_From_CAN((unsigned int)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_CHARGE_STATE, DCS_CURRENT_INDEX_SIZE) == 0 )
	  {
			  Send_ChargeState_From_CAN((bool)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_BRAKE_SW, DCS_CURRENT_INDEX_SIZE) == 0 )
	  {
			  Send_FootBrake_From_CAN((bool)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_HIGHLIGHT_LAMP, DCS_CURRENT_INDEX_SIZE) == 0 )
	  {
			  Send_HighBeam_From_CAN((bool)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_REAR_SEAT_SET_STATE, DCS_CURRENT_INDEX_SIZE) == 0 )
	  {
			  Send_RearSeatSet_From_CAN((bool)g_stActuatorResData[ i ].m_fData);
	  }
	  else if( strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_REAR_SEAT_OCCURED, DCS_CURRENT_INDEX_SIZE) == 0 )
	  {
			  //printf("seat:%d\r\n",g_stActuatorResData[ i ].m_fData);
			  Send_RearSeatOccured_From_CAN((bool)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_CELSIUS, DCS_CURRENT_INDEX_SIZE)==0)
	  {
	  		Send_Celsius_From_CAN((bool)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_FAHRENHEIT, DCS_CURRENT_INDEX_SIZE)==0)
	  {
	  		Send_Fahrenheit_From_CAN((bool)g_stActuatorResData[ i ].m_fData);
	  }
#if defined(PROTOCOL18)
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_HIGHBATT_TEMP_MAX, DCS_CURRENT_INDEX_SIZE)==0) //dahae
	  {
			  Send_HighBatteryTemperatureMAX_From_CAN((short)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_HIGHBATT_TEMP, DCS_CURRENT_INDEX_SIZE)==0)
	  {
			  Send_HighBatteryTemperature_From_CAN((short)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex,DCS_CHARGING_STATE,DCS_CURRENT_INDEX_SIZE)==0) 
	  {
			  Send_ChargingState_From_CAN((bool)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex,DCS_EXTRA_CHARGE_TIME,DCS_CURRENT_INDEX_SIZE)==0)
	  {
			  Send_ExtraChargeTime_From_CAN((unsigned char)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_EXTRA_DISTANCE, DCS_CURRENT_INDEX_SIZE)==0)
	  {
			  Send_ExtraDrivingDistance_From_CAN((unsigned short)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_SOC_STATE, DCS_CURRENT_INDEX_SIZE)==0)
	  {
			  Send_SOCState_From_CAN(g_stActuatorResData[ i ].m_fData);
	  }
#endif
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_INSULATION_RESISTANCE, DCS_CURRENT_INDEX_SIZE)==0) //B2A
	  {
			  Send_InsulationResistance_From_CAN((unsigned short)g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex, DCS_BATTERY_CELL_VOL_MAX, DCS_CURRENT_INDEX_SIZE)==0) // B36
	  {
			  Send_MaxBatteryCellVoltage_From_CAN(g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[ i ].m_cIndex,DCS_BATTERY_CELL_VOL_MIN,DCS_CURRENT_INDEX_SIZE)==0) //B37
	  {
	  		  Send_MinBatteryCellVoltage_From_CAN(g_stActuatorResData[ i ].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[i].m_cIndex, DCS_BATTERY_PACK_C, DCS_CURRENT_INDEX_SIZE)==0)	//B02
	  {
			 Send_BatteryPackC_From_CAN(g_stActuatorResData[i].m_fData);
	  }
	  else if(strncmp(g_stActuatorResData[i].m_cIndex, DCS_SOH_STATE, DCS_CURRENT_INDEX_SIZE)==0)
	  {
			 Send_SOHState_From_CAN(g_stActuatorResData[i].m_fData);
	  }
}
void Current_SlaveData(stSlaveData * pstCurrDataBase, int i)
{
	static INT16U OldRpm=0, s_uiOldMotorRpm = 0;
	static U8  ucGarbageOdoCnt=0;
	float fTemp=0;
	static char s_cEvenState=0;
	
				if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_SPEED, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_Speed_From_CAN(abs((unsigned int)g_pstCurrDataBase[i].m_fData));

					stSensorInfo stGyroAngle;
					GetGyroAngle(&stGyroAngle);
					if( (Get_Speed()==0) && (g_bFuelLevelCheckOneTimeRunFlag==true) && (stGyroAngle.bCalibration == true || g_ucTCUSlopeAngleArrIndex != 0) )						
					{
//						printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Go FuelCheck~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
//						g_bFuelLevelCheckFlag = true;
//						g_bFuelLevelCheckOneTimeRunFlag = false;

						static int s_nTmpx =0,s_nTmpy=0;
						
						if(s_nTmpx != stGyroAngle.nX || s_nTmpy != stGyroAngle.nY)
						{
//								printf("Before call FuelCheckCallback] x:%d, y:%d, z:%d\r\n", stGyroAngle.nX, stGyroAngle.nY, stGyroAngle.nZ);
							if(g_iTimerFuelCheck30secCallback != -1)
							{
								g_nSumAnglex += abs(stGyroAngle.nX);
								g_nSumAngley += abs(stGyroAngle.nY);
								g_nAngleSumCount++;
							}
						}
						s_nTmpx = stGyroAngle.nX;
						s_nTmpy = stGyroAngle.nY;
						
						if( (FUEL_LEVEL_CHECK_MIN_ANGLE <= stGyroAngle.nX && stGyroAngle.nX <= FUEL_LEVEL_CHECK_MAX_ANGLE)
							&& (FUEL_LEVEL_CHECK_MIN_ANGLE <= stGyroAngle.nY && stGyroAngle.nY <= FUEL_LEVEL_CHECK_MAX_ANGLE) )

						{
							if( g_iTimerFuelCheck30secCallback == -1 )
							{
//									printf("＠＠＠＠＠＠Before call FuelCheckCallback] x:%d, y:%d, z:%d\r\n", stGyroAngle.nX, stGyroAngle.nY, stGyroAngle.nZ);
								g_iTimerFuelCheck30secCallback = HalTimerSetSWTimer(30000, eSWTimer_ONESHOT, CB_FuelCheck_30secCallback, TRUE);
							}
							
						}
					}

					if(((Get_Speed()==0) && (g_bFuelLevelCheckOneTimeRunFlag==true) && g_bFuelLevelSwitchedPercentFlag == true) )
					{
						if( g_iTimerFuelCheck30secCallback == -1 )
						{
							g_iTimerFuelCheck30secCallback = HalTimerSetSWTimer(30000, eSWTimer_ONESHOT, CB_FuelCheck_30secCallback, TRUE);
						}
					}
					
					if( g_bTPMSConvCheckFlag == false ) //정차중 TPMS 압력단위 변경을 위해
					{
						g_bTPMSConvCheckFlag = true;
						g_uiTpmsAlramValue = g_uiTpmsAlramValueSave;
					}
					
					if( Get_Speed()>0 )
					{
						g_bFuelLevelCheckOneTimeRunFlag = true;	//움직였으면 체크플래그를 세팅
						HalTimerClearSWTimer(g_iTimerFuelCheck30secCallback);
						g_iTimerFuelCheck30secCallback = -1;
						g_nSumAnglex=0; g_nSumAngley=0; g_nAngleSumCount=0;
						increase_Speed_Cnt();
					}
					
					Send_Speed_Total(Get_Speed_Total() + (long long)Get_Speed());
					if( Get_Speed_Cnt() >0 )
					{
					Send_Speed_Avg( (float)(Get_Speed_Total()/ Get_Speed_Cnt()) );
					}
					Distance();	//속도갱신시마다 이동거리도 갱신함
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_RPM, DCS_CURRENT_INDEX_SIZE)==0)
				{
                    Send_RPM_From_CAN((INT16U)g_pstCurrDataBase[i].m_fData);
//					Trace(0,"RPM:%f\r\n",g_pstCurrDataBase[i].m_fData);
					if(OldRpm>0 && Get_RPM() == 0 && g_OBDControllerData.monitering_Data.m_bENGRun==eENGRUN_OFF)	//시동 OFF시 ODO 버리는 갯수 초기화
					{
						ucGarbageOdoCnt=0;
					}
					OldRpm =Get_RPM();

					if(Get_RPM() != 0)
					{
						Send_RPM_Total(Get_RPM_Total() + (long long)Get_RPM());
						increase_RPM_Cnt();
						Send_RPM_Avg( (unsigned int)(Get_RPM_Total()/Get_RPM_Cnt()) );
					}
					
					if(g_eMapMaf == eMAPMAF_MAP)
					{
						float fVolumetricEfficiency=80.0;                     /* 체적효율 */
						U16 ucFuelEffRpm[9] = {1000,1500,2000,2500,3000,4000,5000,6000,10000};
						U8 *pucFuelEffRpmValue=NULL;
						U8 ucFuelEffRpmValue1400[9] = {70,76,77,78,79,85,83,76,60};
						U8 ucFuelEffRpmValue1600[9] = {65,72,76,80,82,92,86,78,65};
						U8 ucFuelEffRpmValue2000[9] = {77,80,82,85,90,94,90,85,60};
						U8 ucFuelEffRpmValueOver[9] = {64,72,76,80,82,92,86,78,65};
						INT16U usRpm = Get_RPM();
						float fMassAirPressure = 0.0;
						float fIntakeAirTempearature = 0.0;
						float IMAP = 0.0;
//						static INT16U OldRpm=0;
						INT16U usFilterRpm=0;
						unsigned short usFindRpm = 0;
						
						fMassAirPressure = Get_MassAirPressure();
						fIntakeAirTempearature = Get_IntakeAirTemperature();
						if( fIntakeAirTempearature != 0.0 )	IMAP = usRpm*fMassAirPressure/fIntakeAirTempearature;
						else												IMAP = 0;
						
						// 연비가 나쁘면 효율이 크다라는 뜻
						if(g_fDisplace<=1400)				pucFuelEffRpmValue = ucFuelEffRpmValue1400;
						else if(g_fDisplace<=1600)		pucFuelEffRpmValue = ucFuelEffRpmValue1600;
						else if(g_fDisplace<=2000)		pucFuelEffRpmValue = ucFuelEffRpmValue2000;
						else										pucFuelEffRpmValue = ucFuelEffRpmValueOver;

//						if(g_stFastFuncData.m_usRpm < 1000)
							usFilterRpm = Get_RPM();	
//						else
//							usFilterRpm = (INT16U)(OldRpm * 0.80 + Get_RPM() *0.20);
							
						for(int i = 0 ; i<9;i++)
						{
							if(usFilterRpm < ucFuelEffRpm[i])
							{
								usFindRpm = i;
								break;
							}
						}

						
						//printf("usFindRpm : %d pucFuelEffRpmValue[usFindRpm-1]:%d ",usFindRpm,pucFuelEffRpmValue[usFindRpm-1]);
						if(usFindRpm > 0)
						{

							if(usFindRpm > 5)
								g_fVolumetricEfficiency = pucFuelEffRpmValue[usFindRpm-1] - (float)(usFilterRpm-ucFuelEffRpm[usFindRpm-1])*(float)((pucFuelEffRpmValue[usFindRpm-1]-pucFuelEffRpmValue[usFindRpm])/ (ucFuelEffRpm[usFindRpm] - ucFuelEffRpm[usFindRpm-1]));
							else
								g_fVolumetricEfficiency = pucFuelEffRpmValue[usFindRpm] - (float)(ucFuelEffRpm[usFindRpm] - usFilterRpm)*(float)((pucFuelEffRpmValue[usFindRpm]-pucFuelEffRpmValue[usFindRpm-1])/ (ucFuelEffRpm[usFindRpm] - ucFuelEffRpm[usFindRpm-1]));
								// ex) 76 - ( 1500 -  1001 ) * ((  76 -  70 ) / ( 1500 - 1000))
						}
						else
						{
						  g_fVolumetricEfficiency = pucFuelEffRpmValue[0];
						}

						if(Get_MassAirPressure() != 0.0 && Get_IntakeAirTemperature() != 0.0)
						{
						  IMAP = (float)(usFilterRpm *Get_MassAirPressure()/Get_IntakeAirTemperature());
						}
						
						//printf("Map:%f IAT:%f IMAP:%f VE:%f\r\n",fMassAirPressure,fIntakeAirTempearature,IMAP,g_fVolumetricEfficiency);
						
						//Trace(0,"fMassAirPressure : %f, fIntakeAirTempearature:%f, IMAP :%f\n",fMassAirPressure,fIntakeAirTempearature,IMAP);
						//MAF = (IMAP/120) * (VE/100) * ED * MM / R
						//VE:  volumetric efficiency 체적효율 ( 80%로 입력)
						//ED: Engine Displacement 엔진 베기량 (2000cc)
						//MM: 공기 평균 분자량 28.9644 g/mol
						//R: 연료 상수 8.314472 J/mol.k
						fTemp = (IMAP/120.0)*(fVolumetricEfficiency/100.0)*(g_fDisplace * 0.001)*28.9644/8.314472;
						//printf("fTemp : %f\r\n",fTemp);
						Send_MassAirFlow_From_CAN(fTemp);
						/// 여기까지 MAP일때만 써야한다
						fuelConsume();
					}
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ODOMETER, DCS_CURRENT_INDEX_SIZE)==0)
				{
					INT32U uiTmpOdo=(INT32U)g_pstCurrDataBase[i].m_fData;		//소수점이하 버림(불규칙적 데이터 - m단위)
								//Trace(0,"[DCS_ODOMETER] %d \r\n",uiTmpOdo);
					BOOL bOdoValidation = FALSE;
					
					if(ucGarbageOdoCnt >= g_ucOdoGarbageTotal)	//시동 초기 data 버림(garbage)
					{
						if(ODO_TYPE_GET_STATE() == ODO_TYPE_SUPP_DCS_ODOMETER )
						{
							if( uiTmpOdo < MAX_ODOMETER_SKIP_DATA && (Get_VehicleStatus() >= eVEHICLE_STATE_IGON && Get_VehicleStatus() <= eVEHICLE_STATE_EV_ON) )
							{
								if( g_OBDControllerData.monitering_Data.m_uiOdometer != uiTmpOdo )
								{
									bOdoValidation = ValidationODO(uiTmpOdo);
								}
								else
								{
									bOdoValidation = TRUE;
								}

								if((Get_Odmeter() <= uiTmpOdo ) && bOdoValidation == TRUE)	//ODO가 이전ODO보다 작으면 ERROR - 20160619 VF초기통신시 0KM 수신
								{
#ifdef DISTANCE
	//							Trace(0,"[bOdoValidation] %d %d\r\n",uiTmpOdo,g_OBDControllerData.monitering_Data.m_uiOdometer);
#endif
									g_stDistanceInfo.m_bOdometerRcv = TRUE;
									g_uiDistanceafterClearDTC = uiTmpOdo;								//주행거리 계산 용
									Send_Odometer_From_CAN(uiTmpOdo);
								}
								SetOffset();	//주행거리 보정 시점을 잡아주는 함수
							}
						}
						else
						{
							Trace(0,"[odo fail] supported \r\n");
						}
					}
					else
					{
						ucGarbageOdoCnt++;
						Trace(0,"[GARBAGE - ODO %d] odo meter : %u km\r\n", ucGarbageOdoCnt, uiTmpOdo);
					}
					//Trace(0,"TRIP-A[%d], TRIP-B[%d] \r\n", g_stFastFuncData.m_nOdometer_a, g_stFastFuncData.m_nOdometer_t);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ODOMETER_SUB, DCS_CURRENT_INDEX_SIZE)==0 && (ODO_TYPE_GET_STATE() == ODO_TYPE_SUPP_DCS_ODOMETER_SUB))
				{
					float uiTmpOdo=g_pstCurrDataBase[i].m_fData;		//소수점이하 버림(불규칙적 데이터 - m단위)
					uiTmpOdo *= DCS_ODO_MILE_CONVERSION;

					if(ucGarbageOdoCnt >= g_ucOdoGarbageTotal)	//시동 초기 data 버림(garbage)
					{
						if(g_stFastFuncData.m_nOdometer_t <= uiTmpOdo * 1000)	//ODO가 이전ODO보다 작으면 ERROR - 20160619 VF초기통신시 0KM 수신
						{
							g_stDistanceInfo.m_bOdometerRcv=TRUE;
							g_stFastFuncData.m_nOdometer_t = (INT32U)uiTmpOdo * 1000;			//trip(총주행거리)
						}

						if(OFFSET_GET_STATE()!=GETED_OFFSET)
							SetOffset();	//주행거리 보정
					}
					else
					{
						ucGarbageOdoCnt++;
						Trace(0,"[GARBAGE - ODO %d] odo meter : %f km - mile conversion\r\n", ucGarbageOdoCnt, uiTmpOdo);
					}
					//Trace(0,"TRIP-A[%d], TRIP-B[%d] \r\n", g_stFastFuncData.m_nOdometer_a, g_stFastFuncData.m_nOdometer_t);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_DISTANCE_CLEAR_DTC, DCS_CURRENT_INDEX_SIZE)==0)
				{
                    g_stStaticOBDdata.g_bOdometerRcv=TRUE;
					g_uiDistanceafterClearDTC = (U32)g_pstCurrDataBase[i].m_fData;
                    SetOffset();	//주행거리 보정
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_AVR_FUEL_EFFICIENCY_M, DCS_CURRENT_INDEX_SIZE)==0)
				{
					//FUEL_EFFICIENCY_TYPE_SET_STATE(NOT_CALCULATION);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_AVR_FUEL_EFFICIENCY_K, DCS_CURRENT_INDEX_SIZE)==0)
				{
					//FUEL_EFFICIENCY_TYPE_SET_STATE(NOT_CALCULATION);
					g_stFastFuncData.m_nMileage_a = g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FUEL_CONSUM, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_Fuel_Consume_From_CAN((double)g_pstCurrDataBase[i].m_fData, g_pstCurrDataBase[i].m_cE);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_H2_CONSUM, DCS_CURRENT_INDEX_SIZE)==0)
				{
#if false				
					{
						static int s_nCount = 0;
						if( s_nCount++ % 10 == 0 )
						{
							printf("FCEV Current Consume : %f, Total Consume : %f\n",(double)g_pstCurrDataBase[i].m_fData,g_OBDControllerData.monitering_Data.m_fTotal_Fuel_Consume);
							printf("FCEV Driving Distance : %f\n",g_OBDControllerData.monitering_Data.m_fDriving_Distance);
							printf("FCEV Mileage : %f\n",g_OBDControllerData.monitering_Data.MileageAvg);
							printf("FCEV Fuel Level : %d\n",Get_Remain_Fuel_Percent());
						}
					}
#endif					
					Send_Fuel_Consume_From_CAN((double)g_pstCurrDataBase[i].m_fData, g_pstCurrDataBase[i].m_cE);
				}
				// support FCEV reamined fuel
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HYDROGEN_CUR_STOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					printf("FCEV Remained Fuel : %f\r\n",g_pstCurrDataBase[i].m_fData);
					Send_Remain_Fuel_Percent_From_CAN(g_pstCurrDataBase[i].m_fData);
				}				
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FUEL_EFFICIENCY_K, DCS_CURRENT_INDEX_SIZE)==0)
				{
					//순간연비
				}
				
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_MAF_SENSOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_MassAirFlow_From_CAN(g_pstCurrDataBase[i].m_fData);
					fuelConsume();
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_MAP_SENSOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_MassAirPressure_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_LAMBDA_SENSOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					g_fEquivalenceRatio =  g_pstCurrDataBase[i].m_fData;
					if(g_fEquivalenceRatio==0)
					{
						g_fEquivalenceRatio=1;
					}
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FUELTERM_PERCENT, DCS_CURRENT_INDEX_SIZE)==0)
				{
					g_fEquivalenceRatio =  g_pstCurrDataBase[i].m_fData;
					g_fEquivalenceRatio = 1 + (1 * (g_fEquivalenceRatio/100));
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_INJECTOR_QUANTITY, DCS_CURRENT_INDEX_SIZE)==0)  				//INJECTION Quantity
				{
					Send_InjectionQuantity_From_CAN(g_pstCurrDataBase[i].m_fData);
					if(FUELTYPE_GET_STATE() == DIESEL)
					{
						InjectionQuantity();
					}
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_MAIN_QUANTITY, DCS_CURRENT_INDEX_SIZE)==0)  				//목표 주 분사량
				{
					Send_MI_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_PILOT1_QUANTITY, DCS_CURRENT_INDEX_SIZE)==0)  				//목표 파일럿 1
				{
					Send_PIL1_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_PILOT2_QUANTITY, DCS_CURRENT_INDEX_SIZE)==0)  				//목표 파일럿 2
				{
					Send_PIL2_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_PILOT3_QUANTITY, DCS_CURRENT_INDEX_SIZE)==0)  				//목표 파일럿 3
				{
					Send_PIL3_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_POST1_QUANTITY, DCS_CURRENT_INDEX_SIZE)==0)  				//목표 후 분사 1
				{
					Send_POL1_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_POST2_QUANTITY, DCS_CURRENT_INDEX_SIZE)==0)  				//목표 후 분사 2
				{
					Send_POL2_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_IAT_SENSOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_IntakeAirTemperature_From_CAN(g_pstCurrDataBase[i].m_fData + 273.15);
				}
				
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_GEAR_POS, DCS_CURRENT_INDEX_SIZE)==0)
				{
					//g_stFastFuncData.m_ucGearPos = (U8)g_pstCurrDataBase[i].m_fData;
					g_ucGearPos = (U8)g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_GEAR_LEVER, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if(g_pstCurrDataBase[i].m_fData == 0) {}	//engine off시 데이터가 초기화 case 발생 LF  MD(문자가 아니면 무시해야 한다)
					else		Send_GearPosition_From_CAN((INT8U)g_pstCurrDataBase[i].m_fData) ;
					//Trace(0,"[GL %c %d %d %X]",g_stFastFuncData.m_ucAuto_gear,g_stFastFuncData.m_ucAuto_gear, i, data);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_GEAR_SBW_P, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if(g_pstCurrDataBase[i].m_fData == 1) Send_GearPosition_From_CAN('P');
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_GEAR_SBW_R, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if(g_pstCurrDataBase[i].m_fData == 1) Send_GearPosition_From_CAN('R');
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_GEAR_SBW_N, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if(g_pstCurrDataBase[i].m_fData == 1) Send_GearPosition_From_CAN('N');
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_GEAR_SBW_D, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if(g_pstCurrDataBase[i].m_fData == 1) Send_GearPosition_From_CAN('D');
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FUELCUT, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_FuelCut_From_CAN((INT8U)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FUEL_LEVEL, DCS_CURRENT_INDEX_SIZE)==0)
				{
					//printf("Remained Fuel: %f, \r\n",g_pstCurrDataBase[i].m_fData);
					Send_Remain_Fuel_Percent_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_SLOPE_TCU, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_Slope_TCU_From_CAN(g_pstCurrDataBase[i].m_fData); 
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ACCEL_POS, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_AccelPos_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTERY_V, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_Battery_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTERY_CURRENT, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stSlow2FuncData.m_nBatt_c = g_pstCurrDataBase[i].m_fData;
				}
//				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_LOWBATTERY_SOC, DCS_CURRENT_INDEX_SIZE)==0)
//				{
//					g_stSlow2FuncData.m_ucBatt_Charge =  (INT8U)g_pstCurrDataBase[i].m_fData;
//				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_LOWBATTERY_SOC, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_LowBatterySOC_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTERY_HEALTH, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stSlow2FuncData.m_ucBatt_Health = (INT8U)g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTERY_CAPACITY, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stSlow2FuncData.m_ucBatt_Capacity = (INT8U)g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_PRESS_FL, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if( g_bTPMSConvertFlag == true )
					{
						if( Get_TMPS_Conv() != 0 )
						{
//							printf("%f,%f,%f\r\n",g_pstCurrDataBase[i].m_fData ,g_OBDControllerData.currentState.fTPMS_Conv, (float)(g_pstCurrDataBase[i].m_fData * g_OBDControllerData.currentState.fTPMS_Conv) );
							Send_TPMS_FL_From_CAN((float)(g_pstCurrDataBase[i].m_fData * g_OBDControllerData.currentState.fTPMS_Conv));
						}
					}
					else	Send_TPMS_FL_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_PRESS_FR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if( g_bTPMSConvertFlag == true )
					{
						if( Get_TMPS_Conv() != 0 )
						{
							Send_TPMS_FR_From_CAN((float)(g_pstCurrDataBase[i].m_fData * g_OBDControllerData.currentState.fTPMS_Conv));
						}
					}
					else	Send_TPMS_FR_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_PRESS_RL, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if( g_bTPMSConvertFlag == true )
					{
						if( Get_TMPS_Conv() != 0 )
						{
							Send_TPMS_RL_From_CAN((float)(g_pstCurrDataBase[i].m_fData * g_OBDControllerData.currentState.fTPMS_Conv));
						}
					}
					else	Send_TPMS_RL_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_PRESS_RR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if( g_bTPMSConvertFlag == true )
					{
						if( Get_TMPS_Conv() != 0 )
						{
							Send_TPMS_RR_From_CAN((float)(g_pstCurrDataBase[i].m_fData * g_OBDControllerData.currentState.fTPMS_Conv));
						}
					}
					else	Send_TPMS_RR_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_MIL_LAMP, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_MILLamp_From_CAN((INT32U)g_pstCurrDataBase[i].m_fData) ;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_CONVERT, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_TPMS_Unit_From_CAN((unsigned short)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HIGHLIGHT_LAMP, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_HighBeam_From_CAN((INT8U)g_pstCurrDataBase[ i ].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HEADLAMP, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_HeadLamp_From_CAN((INT32U)g_pstCurrDataBase[ i ].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_COOLANT_TEMP, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stSlow2FuncData.m_ucCoolant_Temp = (INT8U)g_pstCurrDataBase[i].m_fData;;
                    //MONI 20180810
                    // we changed engine temperature with coolant temperature.
                    Send_Coolant_Temperature_From_CAN((INT8U)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CRUIES, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stAdviceFuncData.m_ucCruies_State = (INT8U)g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_THROTTLE_CLOSE, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stAdviceFuncData.m_ucThrottle_State = (INT8U)g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ISG, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stAdviceFuncData.m_ucIsg_State = (INT8U)g_pstCurrDataBase[i].m_fData;
                    if(g_pstCurrDataBase[ i ].m_fData != 0)		//!!!!!!!!!!!!!!!!!!!상준책임 확인필요!!!!!!!!!!!!!!!
                    {
                        Send_ISG_From_CAN((unsigned char)g_pstCurrDataBase[ i ].m_fData);
                    }
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HOOD, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_HoodOpen_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ENGINE_LOAD, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stFastFuncData.m_ucEngine_load = (INT8U)g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_TEMP_FL, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stSlow2FuncData.m_usTire_Temp_FL = g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_TEMP_FR, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stSlow2FuncData.m_usTire_Temp_FR = g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_TEMP_RR, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stSlow2FuncData.m_usTire_Temp_RL = g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TIRE_TEMP_RL, DCS_CURRENT_INDEX_SIZE)==0)
				{
//					g_stSlow2FuncData.m_usTire_Temp_RR = g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ENGINEOIL_TEMP, DCS_CURRENT_INDEX_SIZE)==0)// 수정
				{
//					printf("DCS_ENGINEOIL_TEMP : %f\r\n",g_pstCurrDataBase[i].m_fData);
                    // MONI 20180810
                    // block this code it is not used.
//					Send_Engine_Temperature_From_CAN((short)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_OILTEMP_TCU, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_TransmissionOil_Temperature_From_CAN((INT32U)g_pstCurrDataBase[i].m_fData) ;
					//g_stSlow3FuncData.m_ucMissionOil_Temp = (INT8S)g_pstCurrDataBase[i].m_fData;
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_POWER_ACC, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if( g_bIndicatorDBFlag == true )
					{
						//if(Get_UserStatus() == eUSER_IN)		//사양에서는 거의 ACC의 값으로 USER 값을 대입한다고 함. 하지만 시동OFF 즉시 eUSER_OUT 되므로 DATA 체크하지 못하는 문제 발생하여 막음 210303 LWH NEXO
						{
#if defined(ENGINE_LOG)
						    printf("ACC_ : %f\r\n",g_pstCurrDataBase[i].m_fData);
						    Trace(0,"ACC_D : %f\r\n",g_pstCurrDataBase[ i ].m_fData);
#endif
							Send_ACC_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
						}
					}
					else	Send_ACC_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_POWER_IG1, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if( g_bIndicatorDBFlag == true )
					{
						//if(Get_UserStatus() == eUSER_IN)		//사양에서는 거의 ACC의 값으로 USER 값을 대입한다고 함. 하지만 시동OFF 즉시 eUSER_OUT 되므로 DATA 체크하지 못하는 문제 발생하여 막음 210303 LWH NEXO
						{
#if defined(ENGINE_LOG)
							printf("IG1_ : %f\r\n",g_pstCurrDataBase[i].m_fData);
							Trace(0,"IG1 : %f\r\n",g_pstCurrDataBase[ i ].m_fData);
#endif
							Send_IG1_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
						}
					}
					else Send_IG1_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_POWER_IG2, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if( g_bIndicatorDBFlag == true )
					{
						//if(Get_UserStatus() == eUSER_IN)		//사양에서는 거의 ACC의 값으로 USER 값을 대입한다고 함. 하지만 시동OFF 즉시 eUSER_OUT 되므로 DATA 체크하지 못하는 문제 발생하여 막음 210303 LWH NEXO
						{
#if defined(ENGINE_LOG)
						printf("IG2_ : %f\r\n",g_pstCurrDataBase[i].m_fData);
						Trace(0,"IG2 : %f\r\n",g_pstCurrDataBase[ i ].m_fData);
#endif
							Send_IG2_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
						}
					}
					else Send_IG2_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_POWER_IG3, DCS_CURRENT_INDEX_SIZE)==0)   //B1D
				{
					if( g_bIndicatorDBFlag == true )
					{
						//if(Get_UserStatus() == eUSER_IN)		//사양에서는 거의 ACC의 값으로 USER 값을 대입한다고 함. 하지만 시동OFF 즉시 eUSER_OUT 되므로 DATA 체크하지 못하는 문제 발생하여 막음 210303 LWH NEXO
						{
 #if defined(ENGINE_LOG)                         
					    printf("IG3_ : %f\r\n",g_pstCurrDataBase[i].m_fData);
						Trace(0,"IG3 : %f\r\n",g_pstCurrDataBase[ i ].m_fData);
 #endif                      
							Send_IG3_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
						}
					}
					else Send_IG3_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_POWER_READY, DCS_CURRENT_INDEX_SIZE)==0)
				{
					if( g_bIndicatorDBFlag == true )
					{
						//if(Get_UserStatus() == eUSER_IN)		//사양에서는 거의 ACC의 값으로 USER 값을 대입한다고 함. 하지만 시동OFF 즉시 eUSER_OUT 되므로 DATA 체크하지 못하는 문제 발생하여 막음 210303 LWH NEXO
						{
#if defined(ENGINE_LOG)       
 					printf("B0C:%f\r\n",g_pstCurrDataBase[i].m_fData);
 #endif                   
							Send_EngRun_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
						}
					}
					else Send_EngRun_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_SOC_STATE, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_SOCState_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTERY_PACK_C, DCS_CURRENT_INDEX_SIZE)==0)	//B02
				{
					Send_BatteryPackC_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTERY_PACK_V, DCS_CURRENT_INDEX_SIZE)==0)	//B03
				{
					Send_BatteryPackV_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_SOH_STATE, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_SOHState_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EXTRA_DISTANCE, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_ExtraDrivingDistance_From_CAN((unsigned short)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HIGHBATT_TEMP, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_HighBatteryTemperature_From_CAN((short)g_pstCurrDataBase[i].m_fData);
				}
#if defined(PROTOCOL18)
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HIGHBATT_TEMP_MAX, DCS_CURRENT_INDEX_SIZE)==0) //dahae
				{
					Send_HighBatteryTemperatureMAX_From_CAN((short)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine,DCS_CHARGING_STATE,DCS_CURRENT_INDEX_SIZE)==0) 
				{
					Send_ChargingState_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine,DCS_EXTRA_CHARGE_TIME,DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_ExtraChargeTime_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
#endif
#if defined(PROTOCOL19)
				else if( strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HYDROGEN_FUEL_LEVEL, DCS_CURRENT_INDEX_SIZE) == 0 )	//F36 수소 연료 레벨 %
				{
					Send_Hydrogen_Fuel_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if( strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_AIR_PURIFICATION, DCS_CURRENT_INDEX_SIZE) == 0 )	//F37 공기 정화량 KL
				{
					Send_AirPurification_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if( strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CO2_REDUCTION, DCS_CURRENT_INDEX_SIZE) == 0 )	//F38 CO2 감축량 Kg
				{
					Send_CO2Reduction_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTERY_CELL_MIN, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_MinBatteryCellVoltage_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTERY_CELL_MAX, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_MaxBatteryCellVoltage_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HYDROGEN_CHARGE_CNT, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_HydrogenChargeCnt_From_CAN((unsigned int)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HYDROGEN_TANK_PRES, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_HydrogenTankPress_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HYDROGEN_CUR_TEMP, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_HydrogenCurrentTemperature_From_CAN((short)g_pstCurrDataBase[i].m_fData);
				}
#endif
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_OUTSIDE_TEMPERATURE, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_OutsideTemperature_From_CAN((char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CHARGE_COUNT, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_ChargeCount_From_CAN((unsigned short)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CHARGE_TIME, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_ChargeTime_From_CAN((unsigned int)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[ i ].m_cIndexFine, DCS_BRAKEJUDDER_VALUE, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_BrakeJudder_From_CAN((unsigned int)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_MOTOR_REVOL, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_MotorRPM_From_CAN(abs((int)g_pstCurrDataBase[i].m_fData));

#if 1 // James Jean 2018/11/18
//					Trace(0,"Motor RPM:%f\r\n",g_pstCurrDataBase[i].m_fData);
					if(s_uiOldMotorRpm >0 && Get_MotorRPM_Status() == 0 && g_OBDControllerData.monitering_Data.m_bENGRun == eENGRUN_OFF)	//시동 OFF시 ODO 버리는 갯수 초기화
					{
						ucGarbageOdoCnt=0;
					}
					s_uiOldMotorRpm = Get_MotorRPM_Status();
					s_cEvenState++;

					if(Get_MotorRPM_Status() != 0 && s_cEvenState%2==0 )
					{
						s_cEvenState=0;
						Send_MotorRPM_Total(Get_MotorRPM_Total() + Get_MotorRPM_Status());
						increase_MotorRPM_Cnt();
						Send_MotorRPM_Avg( (unsigned int)(Get_MotorRPM_Total()/Get_MotorRPM_Cnt()) );
					}
#endif
					energyConsume();
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_INSULATION_RESISTANCE, DCS_CURRENT_INDEX_SIZE)==0) //B2A
			    {
					Send_InsulationResistance_From_CAN((unsigned short)g_pstCurrDataBase[i].m_fData);
			    }
			 	else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTERY_CELL_VOL_MAX, DCS_CURRENT_INDEX_SIZE)==0) // B36
				{
				 	Send_MaxBatteryCellVoltage_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine,DCS_BATTERY_CELL_VOL_MIN,DCS_CURRENT_INDEX_SIZE)==0) //B37
				{
					Send_MinBatteryCellVoltage_From_CAN(g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CRASH_SIGNAL, DCS_CURRENT_INDEX_SIZE)==0)	//B0D
				{
					Send_CrashSignal_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ABS_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_ABSIndicator_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ISG_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_ISGIndicator_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EPB_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_EPBIndicator_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BRAKEJUDDER_VALUE, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_BrakeJudder_From_CAN((unsigned int)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BMS_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_BMSIndicator_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ENGOILPRESS_INDICATOR, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_ENGOILPRESSIndicator_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
				//DCS_4WD_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_4WD_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!01
				{
					g_stIndicatorCheck[ePOS_4WD_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_4WD_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_4WD_IND_0].m_uiTimerFromLastOFF = Get_Tmr();

					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_4WD_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_4WD_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!02
				{
					g_stIndicatorCheck[ePOS_4WD_IND_1].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_4WD_IND_1].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_4WD_IND_1].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_1].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_4WD_IND_From_CAN(0, 0x02);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_1].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//					Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_4WD_IND_2, DCS_CURRENT_INDEX_SIZE)==0)	//!03
				{
					g_stIndicatorCheck[ePOS_4WD_IND_2].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_4WD_IND_2].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_4WD_IND_2].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_2].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_4WD_IND_From_CAN(0, 0x04);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_2].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
//					Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_4WD_IND_3, DCS_CURRENT_INDEX_SIZE)==0)	//!04
				{
					g_stIndicatorCheck[ePOS_4WD_IND_3].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_4WD_IND_3].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_4WD_IND_3].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_3].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_4WD_IND_From_CAN(0, 0x08);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_3].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
//					Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_4WD_IND_4, DCS_CURRENT_INDEX_SIZE)==0)	//!05
				{
					g_stIndicatorCheck[ePOS_4WD_IND_4].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_4WD_IND_4].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_4WD_IND_4].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_4].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_4WD_IND_From_CAN(0, 0x10);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_4].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
//					Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_4WD_IND_5, DCS_CURRENT_INDEX_SIZE)==0)	//!06
				{
					g_stIndicatorCheck[ePOS_4WD_IND_5].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_4WD_IND_5].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_4WD_IND_5].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_5].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_4WD_IND_From_CAN(0, 0x20);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x20);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_5].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x20);
//					Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x20);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_4WD_IND_6, DCS_CURRENT_INDEX_SIZE)==0)	//!07
				{
					g_stIndicatorCheck[ePOS_4WD_IND_6].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_4WD_IND_6].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_4WD_IND_6].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_6].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_4WD_IND_From_CAN(0, 0x40);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x40);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_6].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x40);
//					Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x40);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_4WD_IND_7, DCS_CURRENT_INDEX_SIZE)==0)	//!08
				{
					g_stIndicatorCheck[ePOS_4WD_IND_7].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_4WD_IND_7].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_4WD_IND_7].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_7].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_4WD_IND_From_CAN(0, 0x80);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x80);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_4WD_IND_7].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x80);
//					Send_4WD_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x80);
				}
				//DCS_ABS_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ABS_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!10
				{
					g_stIndicatorCheck[ePOS_ABS_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_ABS_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_ABS_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_ABS_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_ABS_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_ABS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_ABS_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_ABS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_ABS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ABS_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!11
				{
					g_stIndicatorCheck[ePOS_ABS_IND_1].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_ABS_IND_1].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_ABS_IND_1].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_ABS_IND_1].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_ABS_IND_From_CAN(0, 0x02);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_ABS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_ABS_IND_1].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_ABS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//					Send_ABS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
				}
				//DCS_ACU_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ACU_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!20
				{
					g_stIndicatorCheck[ePOS_ACU_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_ACU_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_ACU_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_ACU_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_Airbag_IND_From_CAN(0);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_Airbag_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_ACU_IND_0].m_uiTimerFromLastOFF) > 10000 )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_Airbag_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
//					Send_Airbag_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				//DCS_AUTOHOLD_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_AUTOHOLD_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!30
				{
					g_stIndicatorCheck[ePOS_AUTOHOLD_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_AUTOHOLD_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_AUTOHOLD_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_AUTOHOLD_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_AutoHold_IND_From_CAN(0);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_AutoHold_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_AUTOHOLD_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_AutoHold_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
//					else Send_AutoHold_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				//DCS_BATTCHARGE_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTCHARGE_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!40
				{
					g_stIndicatorCheck[ePOS_BATTCHARGE_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_BATTCHARGE_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_BATTCHARGE_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_BATTCHARGE_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_BatteryCharge_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_BatteryCharge_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_BATTCHARGE_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_BatteryCharge_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_BatteryCharge_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BATTCHARGE_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!41
				{
					g_stIndicatorCheck[ePOS_BATTCHARGE_IND_1].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_BATTCHARGE_IND_1].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_BATTCHARGE_IND_1].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_BATTCHARGE_IND_1].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_BatteryCharge_IND_From_CAN(0, 0x02);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_BatteryCharge_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_BATTCHARGE_IND_1].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_BatteryCharge_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//					Send_BatteryCharge_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
				}
				//DCS_CHECKENGINE_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CHECKENGINE_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!50
				{
					g_stIndicatorCheck[ePOS_CHECKENGINE_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_CHECKENGINE_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_CHECKENGINE_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_CHECKENGINE_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_CheckEngine_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )
						{
							Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
							if( (Get_TmrDelta(Get_Tmr(),g_uiEngRunStartTime) >= FCS_START_TIME) && (g_bMILLampAlramFlag==true) && (Get_VehicleStatus() >= eVEHICLE_STATE_ENGRUN) )
							{
								ReportAlramStatus(eMESSAGE_EVENT_KEY_MIL_LAMP_ON , 1);
								if( (g_ucDTCTotalCnt==0) && GetOBDState()==eOBD_Running_Info_Mode )	//부팅시 DTC가 없었으면 MIL알람시 DTC도 체크
								{
									SetOBDState(eOBD_FCS_Start);
									g_bFCSRunFlag = true;
									g_bFreezeFrameRunFlag = true;
									g_bIndicatorFirstCheckFlag=true;	//여기서 안바꿔주면 급작스런 시동 OFF ON시 경고등이 체크되어 서버로 데이터 보냄
								}
								printf("Mil Lamp 1 ALARM \r\n" );
								g_bMILLampAlramFlag = false;
							}
						}
					}
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CHECKENGINE_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!51
				{
					g_stIndicatorCheck[ePOS_CHECKENGINE_IND_1].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_CHECKENGINE_IND_1].m_uiTimerFromLastON = Get_Tmr();
					else																				g_stIndicatorCheck[ePOS_CHECKENGINE_IND_1].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_CHECKENGINE_IND_1].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_CheckEngine_IND_From_CAN(0, 0x02);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_CHECKENGINE_IND_1].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//					Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CHECKENGINE_IND_2, DCS_CURRENT_INDEX_SIZE)==0)	//!52
				{
					g_stIndicatorCheck[ePOS_CHECKENGINE_IND_2].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_CHECKENGINE_IND_2].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_CHECKENGINE_IND_2].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_CHECKENGINE_IND_2].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_CheckEngine_IND_From_CAN(0, 0x04);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
					}
//					else		Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_CHECKENGINE_IND_2].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
//					Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CHECKENGINE_IND_3, DCS_CURRENT_INDEX_SIZE)==0)	//!53
				{
					g_stIndicatorCheck[ePOS_CHECKENGINE_IND_3].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_CHECKENGINE_IND_3].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_CHECKENGINE_IND_3].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_CHECKENGINE_IND_3].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_CheckEngine_IND_From_CAN(0, 0x08);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_CHECKENGINE_IND_3].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
//					Send_CheckEngine_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
				}
				//DCS_DBCWARNNING_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_DBCWARNNING_IND, DCS_CURRENT_INDEX_SIZE)==0)	//!60
				{
					g_stIndicatorCheck[ePOS_DBCWARNNING_IND].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_DBCWARNNING_IND].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_DBCWARNNING_IND].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_DBCWARNNING_IND].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_DBCWarnning_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_DBCWarnning_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_DBCWARNNING_IND].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_DBCWarnning_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_DBCWarnning_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				//DCS_DPFGPF_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_DPFGPF_IND, DCS_CURRENT_INDEX_SIZE)==0)	//!70
				{
					g_stIndicatorCheck[ePOS_DPFGPF_IND].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_DPFGPF_IND].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_DPFGPF_IND].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_DPFGPF_IND].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_DPF_IND_From_CAN(0);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_DPF_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_DPFGPF_IND].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_DPF_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
//					Send_DPF_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
//				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_DPFGPF_IND, DCS_CURRENT_INDEX_SIZE)==0)	//!70
//				{
//					Send_DPF_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//				}
				//DCS_SCR_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_SCR_IND, DCS_CURRENT_INDEX_SIZE)==0)	//!80
				{
					g_stIndicatorCheck[ePOS_SCR_IND].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_SCR_IND].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_SCR_IND].m_uiTimerFromLastOFF = Get_Tmr();
					
					if(  Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_SCR_IND].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_SCR_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_SCR_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_SCR_IND].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_SCR_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_SCR_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				//DCS_EPB_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EPB_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!90
				{
					g_stIndicatorCheck[ePOS_EPB_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_EPB_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_EPB_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EPB_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_EPB_IND_From_CAN(0);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_EPB_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EPB_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_EPB_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
//					Send_EPB_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
//				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EPB_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!90
//				{
//					Send_EPB_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//				}
//				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EPB_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!91
//				{
//					Send_EPB_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//				}
				//DCS_TCS_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TCS_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!A0
				{
					g_stIndicatorCheck[ePOS_TCS_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_TCS_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_TCS_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_TCS_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_TCS_IND_From_CAN(0);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_TCS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_TCS_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_TCS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
//					Send_TCS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
//				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TCS_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!A0
//				{
//					Send_TCS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//				}
//				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TCS_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!A1
//				{
//					Send_TCS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//				}
				//DCS_ISG_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ISG_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!B0
				{
					g_stIndicatorCheck[ePOS_ISG_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_ISG_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_ISG_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_ISG_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_ISG_IND_From_CAN(0);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_ISG_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData );
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_ISG_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_ISG_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
//					Send_ISG_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData );
				}
				//DCS_MDPS_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_MDPS_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!C0
				{
					g_stIndicatorCheck[ePOS_MDPS_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_MDPS_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_MDPS_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_MDPS_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_MDPS_IND_From_CAN(0);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_MDPS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_MDPS_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_MDPS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
//					Send_MDPS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				//DCS_OILLEVEL_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_OILLEVEL_IND, DCS_CURRENT_INDEX_SIZE)==0)	//!D0
				{
					g_stIndicatorCheck[ePOS_OILLEVEL_IND].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_OILLEVEL_IND].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_OILLEVEL_IND].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_OILLEVEL_IND].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_OilLevel_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_OilLevel_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_OILLEVEL_IND].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_OilLevel_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_OilLevel_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				//DCS_OILPRESS_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_OILPRESS_IND, DCS_CURRENT_INDEX_SIZE)==0)	//!E0
				{
					g_stIndicatorCheck[ePOS_OILPRESS_IND].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_OILPRESS_IND].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_OILPRESS_IND].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_OILPRESS_IND].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_OilPress_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_OilPress_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_OILPRESS_IND].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_OilPress_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_OilPress_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				//DCS_PARKINGBRAKE_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_PARKINGBRAKE_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!F0
				{
					g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_ParkingBrake_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_ParkingBrake_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_ParkingBrake_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_ParkingBrake_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_PARKINGBRAKE_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!F1
				{
					g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_1].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_1].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_1].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_1].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_ParkingBrake_IND_From_CAN(0, 0x02);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_ParkingBrake_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_1].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_ParkingBrake_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//					Send_ParkingBrake_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_PARKINGBRAKE_IND_2, DCS_CURRENT_INDEX_SIZE)==0)	//!F2
				{
					g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_2].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_2].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_2].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_2].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_ParkingBrake_IND_From_CAN(0, 0x04);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_ParkingBrake_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_PARKINGBRAKE_IND_2].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_ParkingBrake_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
//					Send_ParkingBrake_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
				}
				//DCS_POWERDOWN_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_POWERDOWN_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!G0
				{
					g_stIndicatorCheck[ePOS_POWERDOWN_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_POWERDOWN_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_POWERDOWN_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_POWERDOWN_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_PowerDown_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_PowerDown_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_POWERDOWN_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_PowerDown_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_PowerDown_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_POWERDOWN_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!G1
				{
					g_stIndicatorCheck[ePOS_POWERDOWN_IND_1].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_POWERDOWN_IND_1].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_POWERDOWN_IND_1].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_POWERDOWN_IND_1].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_PowerDown_IND_From_CAN(0, 0x02);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_PowerDown_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_POWERDOWN_IND_1].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_PowerDown_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//					Send_PowerDown_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
				}
				//DCS_AHB_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_AHB_IND, DCS_CURRENT_INDEX_SIZE)==0)	//!H0
				{
					g_stIndicatorCheck[ePOS_AHB_IND].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_AHB_IND].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_AHB_IND].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_AHB_IND].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_AHB_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_AHB_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_AHB_IND].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_AHB_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_AHB_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				//DCS_HEV_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HEV_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!I0
				{
					g_stIndicatorCheck[ePOS_HEV_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_HEV_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_HEV_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_HEV_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HEV_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!I1
				{
					g_stIndicatorCheck[ePOS_HEV_IND_1].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_HEV_IND_1].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_HEV_IND_1].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_1].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_HEV_IND_From_CAN(0, 0x02);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_1].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//					Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HEV_IND_2, DCS_CURRENT_INDEX_SIZE)==0)	//!I2
				{
					g_stIndicatorCheck[ePOS_HEV_IND_2].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_HEV_IND_2].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_HEV_IND_2].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_2].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_HEV_IND_From_CAN(0, 0x04);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_2].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
//					Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HEV_IND_3, DCS_CURRENT_INDEX_SIZE)==0)	//!I3
				{
					g_stIndicatorCheck[ePOS_HEV_IND_3].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_HEV_IND_3].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_HEV_IND_3].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_3].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_HEV_IND_From_CAN(0, 0x08);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_3].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
//					Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HEV_IND_4, DCS_CURRENT_INDEX_SIZE)==0)	//!I4
				{
					g_stIndicatorCheck[ePOS_HEV_IND_4].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_HEV_IND_4].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_HEV_IND_4].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_4].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_HEV_IND_From_CAN(0, 0x10);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_4].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
//					Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HEV_IND_5, DCS_CURRENT_INDEX_SIZE)==0)	//!I5
				{
					g_stIndicatorCheck[ePOS_HEV_IND_5].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_HEV_IND_5].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_HEV_IND_5].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_5].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_HEV_IND_From_CAN(0, 0x20);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x20);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_5].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x20);
//					Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x20);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HEV_IND_6, DCS_CURRENT_INDEX_SIZE)==0)	//!I6
				{
					g_stIndicatorCheck[ePOS_HEV_IND_6].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_HEV_IND_6].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_HEV_IND_6].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_6].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_HEV_IND_From_CAN(0, 0x40);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x40);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_6].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x40);
//					Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x40);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HEV_IND_7, DCS_CURRENT_INDEX_SIZE)==0)	//!I7
				{
					g_stIndicatorCheck[ePOS_HEV_IND_7].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_HEV_IND_7].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_HEV_IND_7].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_7].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_HEV_IND_From_CAN(0, 0x80);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x80);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_HEV_IND_7].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x80);
//					Send_HEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x80);
				}
				//DCS_EV_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EV_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!J0
				{
					g_stIndicatorCheck[ePOS_EV_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_EV_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_EV_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_EV_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EV_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!J1
				{
					g_stIndicatorCheck[ePOS_EV_IND_1].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_EV_IND_1].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_EV_IND_1].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_1].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_EV_IND_From_CAN(0, 0x02);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_1].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//					Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EV_IND_2, DCS_CURRENT_INDEX_SIZE)==0)	//!J2
				{
					g_stIndicatorCheck[ePOS_EV_IND_2].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_EV_IND_2].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_EV_IND_2].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_2].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_EV_IND_From_CAN(0, 0x04);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_2].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
//					Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EV_IND_3, DCS_CURRENT_INDEX_SIZE)==0)	//!J3
				{
					g_stIndicatorCheck[ePOS_EV_IND_3].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_EV_IND_3].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_EV_IND_3].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_3].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_EV_IND_From_CAN(0, 0x08);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_3].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
//					Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EV_IND_4, DCS_CURRENT_INDEX_SIZE)==0)	//!J4
				{
					g_stIndicatorCheck[ePOS_EV_IND_4].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_EV_IND_4].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_EV_IND_4].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_4].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_EV_IND_From_CAN(0, 0x10);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_4].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
//					Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_EV_IND_5, DCS_CURRENT_INDEX_SIZE)==0)	//!J5
				{
					g_stIndicatorCheck[ePOS_EV_IND_5].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_EV_IND_5].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_EV_IND_5].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_5].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_EV_IND_From_CAN(0, 0x20);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x20);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_EV_IND_5].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x20);
//					Send_EV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x20);
				}
				//DCS_FCEV_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FCEV_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!K0
				{
					g_stIndicatorCheck[ePOS_FCEV_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_FCEV_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_FCEV_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_FCEV_IND_From_CAN(0, 0x01);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
//					Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x01);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FCEV_IND_1, DCS_CURRENT_INDEX_SIZE)==0)	//!K1
				{
					g_stIndicatorCheck[ePOS_FCEV_IND_1].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_FCEV_IND_1].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_FCEV_IND_1].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_1].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_FCEV_IND_From_CAN(0, 0x02);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_1].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
//					Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x02);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FCEV_IND_2, DCS_CURRENT_INDEX_SIZE)==0)	//!K2
				{
					g_stIndicatorCheck[ePOS_FCEV_IND_2].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_FCEV_IND_2].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_FCEV_IND_2].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_2].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_FCEV_IND_From_CAN(0, 0x04);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_2].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
//					Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x04);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FCEV_IND_3, DCS_CURRENT_INDEX_SIZE)==0)	//!K3
				{
					g_stIndicatorCheck[ePOS_FCEV_IND_3].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_FCEV_IND_3].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_FCEV_IND_3].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_3].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_FCEV_IND_From_CAN(0, 0x08);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_3].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
//					Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x08);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FCEV_IND_4, DCS_CURRENT_INDEX_SIZE)==0)	//!K4
				{
					g_stIndicatorCheck[ePOS_FCEV_IND_4].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_FCEV_IND_4].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_FCEV_IND_4].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_4].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_FCEV_IND_From_CAN(0, 0x10);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_FCEV_IND_4].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
//					Send_FCEV_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData,0x10);
				}
				//DCS_TPMS_IND
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_TPMS_IND_0, DCS_CURRENT_INDEX_SIZE)==0)	//!L0
				{
					g_stIndicatorCheck[ePOS_TPMS_IND_0].m_uiCanStopTimer = Get_Tmr();
					if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	g_stIndicatorCheck[ePOS_TPMS_IND_0].m_uiTimerFromLastON = Get_Tmr();
//					else																				g_stIndicatorCheck[ePOS_TPMS_IND_0].m_uiTimerFromLastOFF = Get_Tmr();
					
					if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_TPMS_IND_0].m_uiTimerFromLastON) > IND_LASTON_CHECK_TIME )	//마지막 0이외값이 오고 1초가 지났다(1초이상 0 으로 온다)
						Send_TPMS_IND_From_CAN(0);
					else
					{
						if( (unsigned char)g_pstCurrDataBase[i].m_fData != 0 )	Send_TPMS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
					}
//					else if( Get_TmrDelta(Get_Tmr(), g_stIndicatorCheck[ePOS_TPMS_IND_0].m_uiTimerFromLastOFF) > IND_LASTOFF_CHECK_TIME )	//마지막 0값이 오고 1초가 지났다(데이터가 1초이상 0이외값으로 오고있다)
//						Send_TPMS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
//					Send_TPMS_IND_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData );
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_BRAKE_SW, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_FootBrake_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_HAZARD_LAMP, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_hazardLamp_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_CELSIUS, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_Celsius_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_FAHRENHEIT, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_Fahrenheit_From_CAN((bool)g_pstCurrDataBase[i].m_fData);
				}
#if defined(PROTOCOL21)
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ENG_OILLIFE_RATIO, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_EngOilLifeRatio_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ENG_OILLIFE_ENA, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_EngOilLifeEna_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
				else if(strncmp(g_pstCurrDataBase[i].m_cIndexFine, DCS_ENG_OILLIFE_WARN, DCS_CURRENT_INDEX_SIZE)==0)
				{
					Send_EngOilLifeWarn_From_CAN((unsigned char)g_pstCurrDataBase[i].m_fData);
				}
#endif

}

char *strsep(char **stringp, const char *delim) { 
	char *ptr = *stringp; 
	if(ptr == NULL) 
	{ 
		return NULL; 
	} 
	while(**stringp) 
	{ 
		if(strchr(delim, **stringp) != NULL) 
		{ 
			**stringp = 0x00; 
			(*stringp)++; 
			return ptr; 
		} 
		(*stringp)++; 
	} 
	*stringp = NULL; 
	return ptr; 
}

boolean_t GetObdRcvSleepFlag()
{
	return g_bObdRcvSleepFlag;
}

void SetObdRcvSleepFlag(boolean_t bRcvFlag)
{
	g_bObdRcvSleepFlag	= bRcvFlag;
}

void CheckFahrenheit()
{
	boolean_t bOldValue = 0;
	bOldValue = Get_Fahrenheit();
	GetAutolinkConfigProperty(eAutoLinkConfig_Fahrenheit,(void*)&bOldValue);
    boolean_t bFahrenheit = Get_Fahrenheit();
	//boolean_t bUpdatedFlag = Get_Fahrenheit_UpdatedFlag();
	//char buff[1]={0};

    //if( (bOldValue != bFahrenheit) && (bUpdatedFlag == true) )
	if( (bOldValue != bFahrenheit) )
    {
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~change temp unit : %d~~~~~~~~~~~~~~~~~~~~~~~\r\n",bFahrenheit);
    	SetAutolinkConfigProperty(eAutoLinkConfig_Fahrenheit,(void*)&bFahrenheit);
    	/*if( bFahrenheit == 0x01 )	buff[0]='0';
        else			        	buff[0]='1';
    	ReportAlramStatus2(eMESSAGE_EVENT_KEY_FAHRENHEIT_ALRAM,buff,sizeof(buff));*/
    }
}

void SetPACVType(ePACV_TYPE PACVType)
{
	if(PACVType == CV) g_ePACV_Type = PACVType;
	else			  g_ePACV_Type = PA;
}

ePACV_TYPE GetPACVType(void)
{
	return g_ePACV_Type;
}

void SetCANBaudrate(eCanBaudrate CanBaudrate)
{
	if(GetPACVType() == CV)	g_eCanBaudrate = CanBaudrate; 
	else					g_eCanBaudrate = eCAN_500KBPS;
}

eCanBaudrate GetCANBaudrate()
{
	return g_eCanBaudrate;
}
void ProcessSecurityAlarm()
{
	static boolean_t s_bPreHornStatus = false;
	static boolean_t s_bStartCheckFlag = false;
	static boolean_t s_bAlarmOccurred = false;
	static uint32_t s_unActiveCount = 0;
	static unsigned long s_ulDecideTimer = 0;
	static unsigned long s_ulHornStopCheckTimer = 0;
	
	if( (Get_VehicleStatus() == eVEHICLE_STATE_OFF) && (GetRemoteControlStatus() == false) )
	{
		if( s_bPreHornStatus != (boolean_t)g_OBDControllerData.monitering_Data.m_ucHornStatus )
		{
			if( s_bStartCheckFlag == false )
			{
				s_bStartCheckFlag = true;
				s_ulDecideTimer = Get_Tmr();
			}
			
			s_unActiveCount++;
			
			if( (s_unActiveCount >= 5) && (Get_TmrDelta(Get_Tmr(),s_ulDecideTimer)<=9000) )	//5???????? ???? ?и? ?? Modem Reset ???? ?????ð? ???? 
			{
				if( s_bAlarmOccurred == false )
				{
					ReportAlramStatus(eMESSAGE_EVENT_KEY_SECURITY_ALRAM,true);
					s_bAlarmOccurred = true;
				}
			}

			if( (s_bPreHornStatus == 1) && (g_OBDControllerData.monitering_Data.m_ucHornStatus == 0) )
			{
				s_ulHornStopCheckTimer = Get_Tmr();
			}
		}
		else
		{ 	if( s_bStartCheckFlag == true )
			{
				if( (g_OBDControllerData.monitering_Data.m_ucHornStatus == 0) && (Get_TmrDelta(Get_Tmr(),s_ulHornStopCheckTimer)>=3000) )
				{
					s_unActiveCount = 0;
					s_bPreHornStatus = false;
					s_bStartCheckFlag = false;
					s_bAlarmOccurred = false;
				}
			}
		}
	}
	else
	{
		s_unActiveCount = 0;
		s_bPreHornStatus = false;
		s_bStartCheckFlag = false;
		s_bAlarmOccurred = false;
	}
	s_bPreHornStatus = (boolean_t)g_OBDControllerData.monitering_Data.m_ucHornStatus;
}

