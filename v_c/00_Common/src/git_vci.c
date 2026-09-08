/*************************************************************
* NOTE : git_vci.c
*      vci common applications
* Author : Lee junho
* Since : 2019.06.11
**************************************************************/
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "common.h"
#include "git_can.h"
#include "git_PassthruDefines.h"
#include "git_rtc.h"
#include "git_OBDcomm.h"

#include "git_vci.h"
#include "git_ioctl.h"
#include "git_kl.h"
#include "ff.h"
#include "git_function_list.h"
#include "git_mmc.h"
#include "Usart.h"
#ifdef VCI3_DIAG
#include "git_mcp2518fd.h"
#endif
#include "sw_timer.h"
#include "git_OBDcomm.h"
/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
//#define DEBUG_GIT_VCI_LOG
#define VCI1_TEST 0

/////// 5bps init define ///////
#define DLC_5BPS			1
#define RX_SYNC				2
#define RX_KEY1				3
#define RX_KEY2				4
#define TX_KEY2				5
#define RX_INIT				6
#define BOSCH_RX_ID			12
#define RX_DATA_CHECK		13
////////////////////////////////
#define FAIL			0x100

#define AUTOVIN_LENGTH      17   
#define CRC32_POLYNOMIAL 0xEDB88320  // CRC-32 Îã§Ìï≠Ïãù

uint32_t g_CRCval = 0xFFFFFFFF;  // Ï¥àÍ∏∞ Í∞í
/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void DLC_HW_Set(U32 CommRelay,U32 KlineSelect,U32 KlineStatus,U32 LlineSelect,U32 LlineStatus,
                U32 KlineSwitchStatus,U32 LlineSwitchStatus,U32 RxLineSelect,U32 RxLineStatus,
                U32 PullupRelay,U32 KlinePullup,U32 LlinePullup,U32 LineGnd,U32 KlineChSet,
                U32 LlineChSet,U32 ReproChSet);
void DLC_CH_Set(U32 KlineChSet,U32 LlineChSet);
void VCI_FastInit(uint8_t* pData, uint32_t eInCommType);
//void VCI_FastInit(void);

void KW_fast_init (uint32_t FastTime,uint32_t FastTime1);
void KW_fast_init_Powertec (uint32_t FastTime, uint32_t FastTime1);
void VCI_5bpsInit(void);

void Get_AUTOVINData(U8 ucFlag);
U8 CheckVIN_CAN_29bit(void);
U8 CheckVIN_CAN(void);
U8 CheckVIN_EV_NEW(void);
U8 CheckVIN_EV(void);
U8 CheckVIN_FCEV(void);
U8 CheckVIN_CLU(void);
U8 CheckVIN_TM(void);
U8 CheckVIN_KWP2000(void);
U8 CheckVIN_J1979_2_3_11BIT(void);

#ifdef CV_AUTOVIN
//CV AutoVIN
U8 CheckCV_VIN_CAN(void);
U8 CheckCV_VIN_ExtCAN(void);
U8 CheckCV_VIN_PubEngine(void);
U8 CheckCV_VIN_FGEngine(void);
U8 CheckCV_VIN_HLEngine(void);
U8 CheckCV_VIN_VCU( U8 CAN_Line );
U8 CheckCV_VIN_DTG( U8 CAN_Line );
#endif

void DLC_RX_BUFF_CLEAR(void);
U8 GetKlineSelect(void);
//void DLC_TX_5BPS(U8 State);
void DLC_TX_5BPS(U8 ChOutSel,U8 State);
void DLC_5Bps_Set(U32 SyncTime,U8 intDlcOutData);
void DLC_Pulse_Set(U8 ChOutSel);
void DLC_Serial_Set(U8 ChOutSel);
extern u32	Get_TmrDelta( u32 ulNew, u32 ulOld );
extern u32	Get_Tmr( void );
void ETHERNET_LINE_CONTROL();
extern uint8_t  mcp2518fd_set_baud( uint8_t nominalBaud, uint8_t dataBaud, uint8_t format );
extern void		SetCanMasking(void);
void RxCANID_Set(U16 CanTxId1, U16 CanTxId2);
U16 KeylessCodingOmron( void );
U16 KL_RXD_PULSE_READ();
U32 GitEtcFunction(U8 *Receive_Value, U8 *Return_Value);
int FindSamePeriodicMsg(stPERIODIC_MSG_INFO* Info);
int FindEmptyPeriodicMsgInfo(stPERIODIC_MSG_INFO* Info);
void little_to_big_endian(U8 *ucData, U32 uiNum, U8 ucSize) ;

extern void CAN_uDelay(unsigned int uiDelay);
/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/

uint32_t 		    g_ulDeviceID;
uint32_t		    g_ulProtocolID;
uint32_t		    g_ulProtocolFlag;

stGITSetConfig 		g_stGITSetConfig;
stRECORD_HW_SET		g_stGITHWSetData;
stRECORD_HW_SET_ETH	g_stGITHWSetDataEth;
stSBYTE_ARRAY		g_stGITSByteArray;
U8					g_ucCAN_CH;
//U8					g_ucUDS_Data_Struct=0;
U8 					g_ucAUTOVIN[60] = {0};

//stGITSetConfig		g_stGITSetConfig;

extern osMessageQId	hDiagMsg;
extern osPoolId		hDiagPool;
extern osPoolId		hPTPKPool;
extern BOOL			g_bIsFastInit;
extern BOOL			g_bAckflag;
extern BOOL			g_bJ2534AckModeStatus;
extern u32	        intTxdRxdCount;
BOOL	            g_bIsAckWorking=FALSE;
U8                  g_ucCVAdaptorFlag=1;
BOOL                g_bExtCanFlag=false;

extern U8 			g_ucRxBuff[MAX_RX_BUFF_SIZE];
U8 					g_ucVFIVE_BAUD_MOD;
U16 				g_usES95486_RxCANID;//TX CAN ID +8 ∏∏ ºˆΩ≈√≥∏Æ 20220409 KKT
U32 				g_us29bitES95486_RxCANID;//TX CAN ID TA SA ƒ°»Ø∏∏ √≥∏Æ 20230912 Q_hyek
//extern U8  g_ucCanBuffClearFlag;
extern u32	        intDlccomCount;
BOOL                g_bIsAutoVIN=FALSE;

extern stPERIODIC_MSG_INFO stPeriodicMsgInfo[MAX_PERIODICMSG_CNT];
#ifdef CANFD_qhyek //Q_hyek CANFD
extern uint8_t g_ucCanformat;
#endif

bool g_bCanIdSwFilterEn = true;

/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void VCI_Initialize(void)
{
	//DLC_RxBuffClear();

	// Init SConfig list
	memset((void*)&g_stGITSetConfig, 0x00, sizeof(stGITSetConfig));
#if defined(HS_MODIFY)
	memset((void*)&g_stGITSetConfig_2,0x00, sizeof(stGITSetConfig_2));
#endif
	g_stGITSetConfig.nDataRate 		= 10417;
	g_stGITSetConfig.nLoopBack 		= OFF;
	g_stGITSetConfig.nNodeAddress 	= 0;
	g_stGITSetConfig.nNetworkLine 	= 0;
	g_stGITSetConfig.nP1Min 		= 0;
	g_stGITSetConfig.nP1Max 		= 20;
	g_stGITSetConfig.nP2Min 		= 25;
	g_stGITSetConfig.nP2Max 		= 50;
	g_stGITSetConfig.nP3Min 		= 60;
	g_stGITSetConfig.nP3Max 		= 5000;
	g_stGITSetConfig.nP4Min 		= 5;
	g_stGITSetConfig.nP4Max 		= 20;
//	g_stGITSetConfig.nW0 			= 2;
	g_stGITSetConfig.nW1 			= 1000;
	g_stGITSetConfig.nW2 			= 1000;
	g_stGITSetConfig.nW3 			= 1000;
	g_stGITSetConfig.nW4 			= 30;
	g_stGITSetConfig.nW5 			= 300;
	g_stGITSetConfig.nTIdle 		= 300;
	g_stGITSetConfig.nTInil 		= 25;
	g_stGITSetConfig.nTWUp 			= 50;
	g_stGITSetConfig.nParity 		= 0;
	g_stGITSetConfig.nBitSamplePoint = 80;
	g_stGITSetConfig.nSyncJumpWidth = 15;
	g_stGITSetConfig.nT1Max 		= 20;
	g_stGITSetConfig.nT2Max 		= 100;
	g_stGITSetConfig.nT3Max 		= 50;
	g_stGITSetConfig.nT4Max 		= 20;
	g_stGITSetConfig.nT5Max 		= 100;
	g_stGITSetConfig.nIso15765BS 	= 8;
	g_stGITSetConfig.nIso15765STMin = 2;
	g_stGITSetConfig.nBSTx 			= 0xFFFF;
	g_stGITSetConfig.nSTMinTx 		= 0xFFFF;
	g_stGITSetConfig.nDataBits 		= 0;
	g_stGITSetConfig.nFiveBaudMod 	= 0;
	g_stGITSetConfig.nToolManufacturerSpec = 0;
	g_stGITSetConfig.nEtc1 			= 0;
	g_stGITSetConfig.nEtc2 			= 0;
	g_stGITSetConfig.nEtc3 			= 0;
	g_stGITSetConfig.nEtc4 			= 0;
	g_stGITSetConfig.nEtc5 			= 0;

#if defined(HS_MODIFY)
	//J2534-2 SetConfig List Initial
	g_stGITSetConfig_2.nCanMixedFormat        = 0;
	g_stGITSetConfig_2.nJ1962_Pins            = 0;
	g_stGITSetConfig_2.nSWCanDataRate         = 8333;
	g_stGITSetConfig_2.nSWCanSpeedChange      = 0;
	g_stGITSetConfig_2.nSWCanResSwitch        = 0;
	g_stGITSetConfig_2.nActiveChannel         = 0;         //Hardware dependent
	g_stGITSetConfig_2.nSampleRate            = 0;
	g_stGITSetConfig_2.nSamplePerReading      = 1;
	g_stGITSetConfig_2.nReadingPerMsg         = 1;
	g_stGITSetConfig_2.nAverageMethod         = 0;
	g_stGITSetConfig_2.nSampleResolution      = 0x01;      //Hardware dependent
	g_stGITSetConfig_2.nInputRangeLow         = 0x7FFFFFFF;//Hardware dependent
	g_stGITSetConfig_2.nInputRangeHigh        = 0x7FFFFFFF;//Hardware dependent
	g_stGITSetConfig_2.nUEB_T0_Min            = 0x10;
	g_stGITSetConfig_2.nUEB_T1_Max            = 400;
	g_stGITSetConfig_2.nUEB_T2_Max            = 200;
	g_stGITSetConfig_2.nUEB_T3_Max            = 200;
	g_stGITSetConfig_2.nUEB_T4_Min            = 1;
	g_stGITSetConfig_2.nUEB_T5_Max            = 1000;
	g_stGITSetConfig_2.nUEB_T6_Max            = 200;
	g_stGITSetConfig_2.nUEB_T7_Min            = 1;
	g_stGITSetConfig_2.nUEB_T7_Max            = 40;
	g_stGITSetConfig_2.nUEB_T9_Min            = 1;
	g_stGITSetConfig_2.nJ1939_Pins            = 0;
	g_stGITSetConfig_2.nJ1708_Pins            = 0;
	g_stGITSetConfig_2.nJ1939_T1              = 750;
	g_stGITSetConfig_2.nJ1939_T2              = 1250;
	g_stGITSetConfig_2.nJ1939_T3              = 1250;
	g_stGITSetConfig_2.nJ1939_T4              = 1050;
	g_stGITSetConfig_2.nJ1939_Brdcst_MinDelay = 50;
	g_stGITSetConfig_2.nTP20_T_BR_Int         = 20;
	g_stGITSetConfig_2.nTP20_T_E              = 100;
	g_stGITSetConfig_2.nTP20_MNTC             = 10;
	g_stGITSetConfig_2.nTP20_T_Cta            = 1000;
	g_stGITSetConfig_2.nTP20_MNCT             = 5;
	g_stGITSetConfig_2.nTP20_MNTB             = 5;
	g_stGITSetConfig_2.nTP20_MNT              = 2;
	g_stGITSetConfig_2.nTP20_T_Wait           = 100;
	g_stGITSetConfig_2.nTP20_T1               = 100;
	g_stGITSetConfig_2.nTP20_T3               = 0;
	g_stGITSetConfig_2.nTP20_Identifer        = 0;
	g_stGITSetConfig_2.nTP20_RxIdPassive      = 0;
#endif

	// init SByte Array
	memset((void*)&g_stGITSByteArray, 0x00, sizeof(g_stGITSByteArray));

	// init HWSetting Data
	memset((void*)&g_stGITHWSetData, 0x00, sizeof(g_stGITHWSetData));

	g_stGITHWSetData.nCommRelay 	= KLLINE_RELAY;
	g_stGITHWSetData.nKlineSelect 	= K_SERIAL;
	g_stGITHWSetData.nKlineStatus 	= K_NORMAL;
	g_stGITHWSetData.nLlineSelect 	= L_PULSE;
	g_stGITHWSetData.nLlineStatus 	= L_NORMAL;
	g_stGITHWSetData.nKlineSwitchStatus = K_PULSE_LOW;
	g_stGITHWSetData.nLlineSwitchStatus = L_PULSE_HIGH;
	g_stGITHWSetData.nRxLineSelect 	= RXD_OBD2;
	g_stGITHWSetData.nRxLineStatus 	= RXD_NORMAL;
	g_stGITHWSetData.nPullupRelay 	= Pullup;
	g_stGITHWSetData.nKlinePullup 	= K_Pullup_510;
	g_stGITHWSetData.nLlinePullup 	= L_Pullup_510;
	g_stGITHWSetData.nLlineGnd 		= 0;
	g_stGITHWSetData.nKlineCh 		= KL_LINE1_CONNECT_CH07;
	g_stGITHWSetData.nLlineCh 		= L_ChOff;
	g_stGITHWSetData.nRProgramCh 	= R_ChOff;
//	g_stGITHWSetData.nEtc1 			=
//	g_stGITHWSetData.nEtc2 			=
//	g_stGITHWSetData.nEtc3 			=
//	g_stGITHWSetData.nEtc4 			=
//	g_stGITHWSetData.nEtc5 			=
//	g_stGITHWSetData.ackmessage[20] =

	g_ucCAN_CH = 1;
	g_uiAckTiming = 1000;
	g_bIsFastInit = FALSE;
	g_bAckflag = 0;
	g_bJ2534AckModeStatus = 1;
	//g_ucCanBuffClearFlag=ENABLE;//DISABLE ¿Ã∏È πˆ∆€≈¨∏ÆæÓ «œ¡ˆ æ ¿Ω ENABLE¿Ã∏È πˆ∆€≈¨∏ÆæÓ «‘

}

void VCI_REINTI_COMM_STATE(uint32_t ulPID)
{
	switch( ulPID )
	{
#if defined(HS_MODIFY)
	case PID_CAN_PS:
	case Single_CAN_GM:
	case PID_ISO15765_PS:
	case PID_SW_ISO15765_PS:
	case PID_SW_CAN_PS:
	case PID_FT_CAN_PS:
	case PID_FT_ISO15765_PS:
#endif
	case PID_CAN:
	case ISO15765:
	case ISO15765_NEW:
	case ISO14229_UDS:
	case ISO14229:
	case ISO15765_CUBIS:
	case ISO15765_SINGLE:
	case ISO15765_SINGLE_SMK:
	case ISO15765_SMK:
	case ISO15765_SINGLE_PODS:
	case ISO15765_CARB:
	case ISO15765_CARB_NEW:
	case ISO15765_CARB_NEW_LENGTH:
    case J1939_23_CARB_NEW_LENGTH:
	case ISO15765_REPRO_PENNIMG:
	case ISO15765_REPRO_PENNIMG_TIME:
	case ISO15765_NORMAL:
	case ISO15765_CAN_HWSET_DB:
	case ISO14229_UDS_HWSET_DB:
	case ISO15765_CAN_HWSET_DB_SINGLE:
	case ISO15765_EXCEPT:
	case ISO15765_CARB_29BIT:
    case ISO15765_29BIT:
	case ISO14229_ES95486_02_100:
	case ISO14229_ES95486_02_102:
	case ISO14229_ES95486_02_103:
	case ISO14229_ES95486_02_104:
	case ISO14229_ES95486_02_105:
	case ISO14229_ES95486_02_106:
	case ISO14229_ES95486_02_107:
	case ISO14229_ES95486_02_108:
	case ISO14229_ES95486_02_109:
	case ISO14229_ES95486_02_10A:
	case ISO14229_ES95486_02_10B:
	case ISO14229_ES95486_02_10C:
	case ISO14229_ES95486_02_10D:
	case ISO14229_ES95486_02_10E:
	case ISO14229_ES95486_02_10F:
	case ISO14229_ES95486_02_100_NEW:
#ifdef HOTA
    case ISO14229_ES95486_02_HOTA:
#endif
      
#ifdef CANFD_PROTOCOL
    case ISO14229_ES95486_02_130_CANFD:
    case ISO14229_ES95486_02_131_CANFD:
    case ISO15765_ES95486_135_29bit_CANFD:
    case ISO15765_ES95486_136_29bit_CANFD:
#endif
    case ISO15765_ES95486_29bit:
    case ISO15765_ES95486_DOIP_29BIT:
	case ISO14230_ES95486_DOIP_120:
	case ISO14230_ES95486_DOIP_121:
	case ISO14230_ES95486_DOIP_122:
	case ISO14230_ES95486_DOIP_123:
	case ISO14230_ES95486_DOIP_124:
	case ISO14230_ES95486_DOIP_125:
	case ISO14230_ES95486_DOIP_126:
	case ISO14230_ES95486_DOIP_127:
	case ISO14230_ES95486_DOIP_128:
	case ISO14230_ES95486_DOIP_129:
	case ISO14230_ES95486_DOIP_12A:
	case ISO14230_ES95486_DOIP_12B:
	case ISO14230_ES95486_DOIP_12C:
	case ISO14230_ES95486_DOIP_12D:
	case ISO14230_ES95486_DOIP_12E:
	case ISO14230_ES95486_DOIP_12F:
    case ISO14229_ES95486_170:
    case ISO14229_ES95486_170_MMCAN:
    case ISO14230_ES95486_170:
    case ISO14230_ES95486_170_MMCAN:
		CAN_InitVariable();
		clearRXCanMessage();
		clearTXCanMessage();
		break;
	case ISO14230_POWERTEC:
		Disable_KL_Interrupt(KL_LINE1);
		Disable_KL_Interrupt(KL_LINE2);
		clearKLReceiveData(KL_LINE1);
		clearKLReceiveData(KL_LINE2);
		break;
	default:
		GLogI("[%s] not support PID, ulPID 0x%X\r\n", __FUNCTION__, ulPID);
		break;
	}
}

#if defined(HS_MODIFY)
void VCI_GetDeviceInfoList(uint8_t* pSconfigList, uint8_t* pOutData, uint32_t *pulOutDataLen)
{
	SPARAM_LIST stDeviceInfoList;
	stSPARAM 	*pstTmpDeviceInfo;
	char* 		strDeviceID;
	int i,First_Line,Second_Line;

#if defined(DEBUG_GIT_VCI_LOG)
	GLogI("[%s] run\r\n", __FUNCTION__);
#endif
	*pulOutDataLen = 0;
	memcpy(&stDeviceInfoList, pSconfigList, sizeof(SPARAM_LIST));

	*pulOutDataLen += sizeof(SPARAM_LIST);

	if ( stDeviceInfoList.NumOfParams > 0 )
	{
		pstTmpDeviceInfo = malloc(stDeviceInfoList.NumOfParams*sizeof(stSPARAM));
		if ( pstTmpDeviceInfo == NULL )
		{
			GLogI("[%s] DeviceInfo malloc fail......\r\n", __FUNCTION__);
			return;
		}
		for ( i=0; stDeviceInfoList.NumOfParams; i++ )
		{
			memcpy(&pstTmpDeviceInfo[i], &stDeviceInfoList.ParamPtr[i], sizeof(stSPARAM));

			switch(pstTmpDeviceInfo[i].Parameter)
			{
				//case CONF_ID_SERIAL_NUMBER:
				//case CONF_ID_PART_NUMBER  :
				case CONF_ID_FT_CAN_SUPPORTED:
				case CONF_ID_FT_ISO15765_SUPPORTED:
				case CONF_ID_SW_ISO15765_SUPPORTED:
				case CONF_ID_SW_CAN_SUPPORTED:
					pstTmpDeviceInfo[i].Supported = pstTmpDeviceInfo[i].Value; 	break;
				case CONF_ID_J1850PWM_SUPPORTED:
				case CONF_ID_J1850VPW_SUPPORTED:
				case CONF_ID_ISO9141_SUPPORTED:
				case CONF_ID_ISO14230_SUPPORTED:
				case CONF_ID_CAN_SUPPORTED:
				case CONF_ID_ISO15765_SUPPORTED:
				case CONF_ID_J1850PWM_SIMULTANEOUS://?ïÏù∏ ?ÑÏöî
				case CONF_ID_J1850VPW_SIMULTANEOUS:
				case CONF_ID_ISO9141_SIMULTANEOUS:
				case CONF_ID_ISO14230_SIMULTANEOUS:
				case CAN_SIMULTANEOUS:
				case CONF_ID_ISO15765_SIMULTANEOUS:
				case CONF_ID_FT_CAN_SIMULTANEOUS:
				case CONF_ID_FT_ISO15765_SIMULTANEOUS:
				case CONF_ID_SW_ISO15765_SIMULTANEOUS:
				case CONF_ID_SW_CAN_SIMULTANEOUS:
					pstTmpDeviceInfo[i].Supported = pstTmpDeviceInfo[i].Value & 0x00FFFF00;
					pstTmpDeviceInfo[i].Supported |= 0x00000001;                 break;

				case CONF_ID_SHORT_TO_GND_J1962:
					pstTmpDeviceInfo[i].Supported = pstTmpDeviceInfo[i].Value & 0x00000018;//PIN 4 & 5
					break;
				case CONF_ID_PGM_VOLTAGE_J1962:
					break;
				case CONF_ID_J1850PWM_PS_J1962:
					break;
				case CONF_ID_J1850VPW_PS_J1962:
					break;
				case CONF_ID_ISO9141_PS_K_LINE_J1962:
				case CONF_ID_ISO14230_PS_K_LINE_J1962:
				case CONF_ID_CAN_PS_J1962:
				case CONF_ID_ISO9141_PS_L_LINE_J1962:
				case CONF_ID_ISO14230_PS_L_LINE_J1962:
				case CONF_ID_ISO15765_PS_J1962:
				case CONF_ID_FT_CAN_PS_1962:
				case CONF_ID_FT_ISO15765_PS_1962:
					First_Line = (g_stGITSetConfig_2.nJ1962_Pins & 0x0000FF00)>>8;
					Second_Line = (g_stGITSetConfig_2.nJ1962_Pins & 0x000000FF);

					if(First_Line == 0)
						pstTmpDeviceInfo[i].Supported = 0;
					else
						pstTmpDeviceInfo[i].Supported = (0x00010000<<(First_Line-1));

					if(Second_Line == 0)
						pstTmpDeviceInfo[i].Supported &= 0xFFFF0000;
					else
						pstTmpDeviceInfo[i].Supported |= (0x00000001<<(Second_Line-1));
					break;

				case CONF_ID_SW_CAN_PS_J1962:
				case CONF_ID_SW_ISO15765_PS_J1962:
					First_Line = (g_stGITSetConfig_2.nJ1962_Pins & 0x0000FF00)>>8;
					if(First_Line == 0)
						pstTmpDeviceInfo[i].Supported = 0;
					else
						pstTmpDeviceInfo[i].Supported = (0x00010000<<(First_Line-1));
					break;

				default :
					pstTmpDeviceInfo[i].Supported = 0;

					strDeviceID = "~~~~~~~~ not defined ~~~~~~~~~~~~~";
					break;
			}
			GLogI("[%s] IOCTL section %s(0x%X), Value 0x%X\r\n", __FUNCTION__, strDeviceID, pstTmpDeviceInfo[i].Parameter, pstTmpDeviceInfo[i].Value);

			*pulOutDataLen += (i*sizeof(stSPARAM));
			memcpy(pOutData+*pulOutDataLen, &pstTmpDeviceInfo[i], sizeof(stSPARAM));
		}

		*pulOutDataLen = sizeof(SPARAM_LIST) + (stDeviceInfoList.NumOfParams*sizeof(stSPARAM));

		free(pstTmpDeviceInfo);
	}
	else
	{
		GLogI("[%s] Parameter invalid\r\n", __FUNCTION__);
	}
}

void VCI_GetProtocolInfoList(uint8_t* pSconfigList, uint8_t* pOutData, uint32_t *pulOutDataLen)
{
	SPARAM_LIST stProtocolInfoList;
	stSPARAM 	*pstTmpProtocolInfo;
	char* 		strDeviceID;
	int i;

#if defined(DEBUG_GIT_VCI_LOG)
	GLogI("[%s] run\r\n", __FUNCTION__);
#endif
	*pulOutDataLen = 0;
	memcpy(&stProtocolInfoList, pSconfigList, sizeof(SPARAM_LIST));

	*pulOutDataLen += sizeof(SPARAM_LIST);

	if ( stProtocolInfoList.NumOfParams > 0 )
	{
		pstTmpProtocolInfo = malloc(stProtocolInfoList.NumOfParams*sizeof(stSPARAM));
		if ( pstTmpProtocolInfo == NULL )
		{
			GLogI("[%s] ProtocolInfo malloc fail......\r\n", __FUNCTION__);
			return;
		}

		for ( i=0; stProtocolInfoList.NumOfParams; i++ )
		{
			memcpy(&pstTmpProtocolInfo[i], &stProtocolInfoList.ParamPtr[i], sizeof(stSPARAM));

			switch(pstTmpProtocolInfo[i].Parameter)
			{
				case CONF_ID_RESOURCE_GROUP:
					pstTmpProtocolInfo[i].Supported = 0; 	break;
				case CONF_ID_TIMESTAMP_RESOLUTION:
					pstTmpProtocolInfo[i].Value= g_stWritePassThruMsg.Timestamp; break;
				case CONF_ID_MAX_RX_BUFFER_SIZE:
					pstTmpProtocolInfo[i].Supported = MAX_PASSTHRUMSG_DATA_SIZE; break;
				case CONF_ID_MAX_PASS_FILTER:
				case CONF_ID_MAX_BLOCK_FILTER:
				case CONF_ID_MAX_FILTER_MSG_LENGTH:
					pstTmpProtocolInfo[i].Supported = MAX_NUM_MSG_N_FILTER; break;
				case CONF_ID_MAX_PERIODIC_MSGS:
				case CONF_ID_MAX_PERIODIC_MSG_LENGTH:
					pstTmpProtocolInfo[i].Supported = SIZE_PERIODIC_MSG; break;
				case CONF_ID_DESIRED_DATA_RATE:
					pstTmpProtocolInfo[i].Supported = pstTmpProtocolInfo[i].Value;  break;
				case CONF_ID_MAX_REPEAT_MESSAGING:
				case CONF_ID_MAX_REPEAT_MESSAGING_LENGTH:
					pstTmpProtocolInfo[i].Supported = 0; break;
				case CONF_ID_NETWORK_LINE_SUPPORTED://J1850
					pstTmpProtocolInfo[i].Supported = 0; break;
				case CONF_ID_MAX_FUNCT_MSG_LOOKUP://J1850
					pstTmpProtocolInfo[i].Supported = 0; break;
				case CONF_ID_PARITY_SUPPORTED:
					pstTmpProtocolInfo[i].Supported = 0x00000001; break;//No Parity
				case CONF_ID_DATA_BITS_SUPPORTED:
					pstTmpProtocolInfo[i].Supported = 0x00000001; break;//Data Bit Supported(8bit)
				case CONF_ID_FIVE_BAUD_MOD_SUPPORTED:
					pstTmpProtocolInfo[i].Supported = 0; break;
				case CONF_ID_L_LINE_SUPPORTED:
					pstTmpProtocolInfo[i].Supported = 0; break;
				case CONF_ID_CAN_11_29_IDS_SUPPORTED:
					pstTmpProtocolInfo[i].Supported = 0; break;
				case CONF_ID_CAN_MIXED_FORMAT_SUPPORTED:
					pstTmpProtocolInfo[i].Supported = 0; break;
				case CONF_ID_MAX_FLOW_CONTROL_FILTER:
					pstTmpProtocolInfo[i].Supported = SIZE_FLOWCTRL_FILTER; break;
				case CONF_ID_MAX_ISO15765_WFT_MAX:
					pstTmpProtocolInfo[i].Supported = 0; break;
				default :
					pstTmpProtocolInfo[i].Supported = 0;

					strDeviceID = "~~~~~~~~ not defined ~~~~~~~~~~~~~";
					break;
			}
			GLogI("[%s] IOCTL section %s(0x%X), Value 0x%X\r\n", __FUNCTION__, strDeviceID, pstTmpProtocolInfo[i].Parameter, pstTmpProtocolInfo[i].Value);

			*pulOutDataLen += (i*sizeof(stSPARAM));
			memcpy(pOutData+*pulOutDataLen, &pstTmpProtocolInfo[i], sizeof(stSPARAM));
		}

		*pulOutDataLen = sizeof(SPARAM_LIST) + (stProtocolInfoList.NumOfParams*sizeof(stSPARAM));

		free(pstTmpProtocolInfo);
	}
	else
	{
		GLogI("[%s] Parameter invalid\r\n", __FUNCTION__);
	}
}
#endif

void VCI_GetSconfigList(uint8_t* pSconfigList, uint8_t* pOutData, uint32_t *pulOutDataLen)
{
	SCONFIG_LIST stSconfigList;
	stSCONFIG 	*pstTmpSConfig;
	char* 		strConfigID;
	int i;

#if defined(DEBUG_GIT_VCI_LOG)
	GLogI("[%s] run\r\n", __FUNCTION__);
#endif
	*pulOutDataLen = 0;
	memcpy(&stSconfigList, pOutData, sizeof(SCONFIG_LIST));
	*pulOutDataLen += sizeof(SCONFIG_LIST);

	if ( stSconfigList.NumOfParams > 0 )
	{
		pstTmpSConfig = (stSCONFIG *)malloc(stSconfigList.NumOfParams*sizeof(stSCONFIG));
		if ( pstTmpSConfig == NULL )
		{
			GLogI("[%s] SConfig malloc fail......\r\n", __FUNCTION__);
			return;
		}

		//MONI 20230315 - [suresoft] 108) bug fixed for for condition.
		for ( i=0; i<stSconfigList.NumOfParams; i++ )
		{
			memcpy(&pstTmpSConfig[i], &stSconfigList.ConfigPtr[i], sizeof(stSCONFIG));

			switch(pstTmpSConfig[i].Parameter)
			{
				case CONF_ID_DATA_RATE:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nDataRate; 	strConfigID = "CONF_ID_DATA_RATE";		break;
				case CONF_ID_LOOPBACK:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nLoopBack; 	strConfigID = "CONF_ID_LOOPBACK";		break;
				case CONF_ID_NODE_ADDRESS:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nNodeAddress; strConfigID = "CONF_ID_NODE_ADDRESS";	break;
				case CONF_ID_NETWORK_LINE:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nNetworkLine; strConfigID = "CONF_ID_NETWORK_LINE";	break;
				case CONF_ID_P1_MIN:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nP1Min; 		strConfigID = "CONF_ID_P1_MIN";			break;
				case CONF_ID_P1_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nP1Max; 		strConfigID = "CONF_ID_P1_MAX";			break;
				case CONF_ID_P2_MIN:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nP2Min; 		strConfigID = "CONF_ID_P2_MIN";			break;
				case CONF_ID_P2_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nP2Max; 		strConfigID = "CONF_ID_P2_MAX";			break;
				case CONF_ID_P3_MIN:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nP3Min; 		strConfigID = "CONF_ID_P3_MIN";			break;
				case CONF_ID_P3_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nP3Max; 		strConfigID = "CONF_ID_P3_MAX";			break;
				case CONF_ID_P4_MIN:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nP4Min; 		strConfigID = "CONF_ID_P4_MIN";			break;
				case CONF_ID_P4_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nP4Max; 		strConfigID = "CONF_ID_P4_MAX";			break;
				case CONF_ID_W1:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nW1; 			strConfigID = "CONF_ID_W1";				break;
				case CONF_ID_W2:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nW2; 			strConfigID = "CONF_ID_W2";				break;
				case CONF_ID_W3:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nLoopBack; 	strConfigID = "CONF_ID_LOOPBACK";		break;
				case CONF_ID_W4:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nW4; 			strConfigID = "CONF_ID_W4";				break;
				case CONF_ID_W5:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nW5; 			strConfigID = "CONF_ID_W5";				break;
				case CONF_ID_TIDLE:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nTIdle; 		strConfigID = "CONF_ID_TIDLE";			break;
				case CONF_ID_TINIL:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nTInil; 		strConfigID = "CONF_ID_TINIL";			break;
				case CONF_ID_TWUP:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nTWUp; 		strConfigID = "CONF_ID_TWUP";			break;
				case CONF_ID_PARITY:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nParity; 		strConfigID = "CONF_ID_PARITY";			break;
				case CONF_ID_BIT_SAMPLE_POINT:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nBitSamplePoint; strConfigID = "CONF_ID_BIT_SAMPLE_POINT";break;
				case CONF_ID_SYNC_JUMP_WIDTH:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nSyncJumpWidth; strConfigID = "CONF_ID_SYNC_JUMP_WIDTH";	break;
				case CONF_ID_T1_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nT1Max; 		strConfigID = "CONF_ID_T1_MAX";			break;
				case CONF_ID_T2_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nT2Max; 		strConfigID = "CONF_ID_T2_MAX";			break;
				case CONF_ID_T3_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nT3Max; 		strConfigID = "CONF_ID_T3_MAX";			break;
				case CONF_ID_T4_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nT4Max; 		strConfigID = "CONF_ID_T4_MAX";			break;
				case CONF_ID_T5_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nT5Max; 		strConfigID = "CONF_ID_T5_MAX";			break;
				case CONF_ID_ISO15765_BS:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nIso15765BS; 	strConfigID = "CONF_ID_ISO15765_BS";	break;
				case CONF_ID_ISO15765_STMIN:
					pstTmpSConfig[i].Value = g_stGITSetConfig.nIso15765STMin; strConfigID = "CONF_ID_ISO15765_STMIN";break;

#if defined(HS_MODIFY)
				case CONF_ID_CAN_MIXED_FORMAT:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nCanMixedFormat;		 strConfigID = "CONF_ID_CAN_MIXED_FORMAT"; break;
				case CONF_ID_J1962_PINS:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nJ1962_Pins;			 strConfigID = "CONF_ID_J1962_PINS"; break;
				case CONF_ID_SW_CAN_HS_DATA_RATE:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nSWCanDataRate;		     strConfigID = "CONF_ID_SW_CAN_HS_DATA_RATE"; break;
				case CONF_ID_SW_CAN_SPEEDCHANGE_ENABLE:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nSWCanSpeedChange;	     strConfigID = "CONF_ID_SW_CAN_SPEEDCHANGE_ENABLE"; break;
				case CONF_ID_SW_CAN_RES_SWITCH:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nSWCanResSwitch;		 strConfigID = "CONF_ID_SW_CAN_RES_SWITCH"; break;
				case CONF_ID_ACTIVE_CHANNELS:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nActiveChannel;		     strConfigID = "CONF_ID_ACTIVE_CHANNELS"; break;
				case CONF_ID_SAMPLE_RATE:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nSampleRate;			 strConfigID = "CONF_ID_SAMPLE_RATE"; break;
				case CONF_ID_SAMPLES_PER_READING:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nSamplePerReading;	     strConfigID = "CONF_ID_SAMPLES_PER_READING"; break;
				case CONF_ID_READINGS_PER_MSG:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nReadingPerMsg;		     strConfigID = "CONF_ID_READINGS_PER_MSG"; break;
				case CONF_ID_AVERAGING_METHOD:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nAverageMethod;		     strConfigID = "CONF_ID_AVERAGING_METHOD"; break;
				case CONF_ID_SAMPLE_RESOLUTION:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nSampleResolution;	     strConfigID = "CONF_ID_SAMPLE_RESOLUTION"; break;
				case CONF_ID_INPUT_RANGE_LOW:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nInputRangeLow;		     strConfigID = "CONF_ID_INPUT_RANGE_LOW"; break;
				case CONF_ID_INPUT_RANGE_HIGH:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nInputRangeHigh;		 strConfigID = "CONF_ID_INPUT_RANGE_HIGH"; break;
				case CONF_ID_UEB_T0_MIN:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T0_Min;			 strConfigID = "CONF_ID_UEB_T0_MIN"; break;
				case CONF_ID_UEB_T1_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T1_Max;			 strConfigID = "CONF_ID_UEB_T1_MAX"; break;
				case CONF_ID_UEB_T2_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T2_Max;			 strConfigID = "CONF_ID_UEB_T2_MAX"; break;
				case CONF_ID_UEB_T3_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T3_Max;			 strConfigID = "CONF_ID_UEB_T3_MAX"; break;
				case CONF_ID_UEB_T4_MIN:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T4_Min;			 strConfigID = "CONF_ID_UEB_T4_MIN"; break;
				case CONF_ID_UEB_T5_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T5_Max;			 strConfigID = "CONF_ID_UEB_T5_MAX"; break;
				case CONF_ID_UEB_T6_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T6_Max;			 strConfigID = "CONF_ID_UEB_T5_MAX"; break;
				case CONF_ID_UEB_T7_MIN:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T7_Min;			 strConfigID = "CONF_ID_UEB_T7_MIN"; break;
				case CONF_ID_UEB_T7_MAX:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T7_Max;			 strConfigID = "CONF_ID_UEB_T7_MAX"; break;
				case CONF_ID_UEB_T9_MIN:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nUEB_T9_Min;			 strConfigID = "CONF_ID_UEB_T9_MIN"; break;
				case CONF_ID_J1939_PINS:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nJ1939_Pins;			 strConfigID = "CONF_ID_J1939_PINS"; break;
				case CONF_ID_J1708_PINS:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nJ1708_Pins;			 strConfigID = "CONF_ID_J1708_PINS"; break;
				case CONF_ID_J1939_T1:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nJ1939_T1;			     strConfigID = "CONF_ID_J1939_T1"; break;
				case CONF_ID_J1939_T2:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nJ1939_T2;			     strConfigID = "CONF_ID_J1939_T2"; break;
				case CONF_ID_J1939_T3:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nJ1939_T3;			     strConfigID = "CONF_ID_J1939_T3"; break;
				case CONF_ID_J1939_T4:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nJ1939_T4;			     strConfigID = "CONF_ID_J1939_T4"; break;
				case CONF_ID_J1939_BRDCST_MIN_DELAY:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nJ1939_Brdcst_MinDelay;  strConfigID = "CONF_ID_J1939_BRDCST_MIN_DELAY"; break;
				case CONF_ID_TP2_0_T_BR_INT:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_T_BR_Int;		     strConfigID = "CONF_ID_TP2_0_T_BR_INT"; break;
				case CONF_ID_TP2_0_T_E:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_T_E;			     strConfigID = "CONF_ID_TP2_0_T_E"; break;
				case CONF_ID_TP2_0_MNTC:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_MNTC;			     strConfigID = "CONF_ID_TP2_0_MNTC"; break;
				case CONF_ID_TP2_0_T_CTA:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_T_Cta;			 strConfigID = "CONF_ID_TP2_0_T_CTA"; break;
				case CONF_ID_TP2_0_MNCT:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_MNCT;			     strConfigID = "CONF_ID_TP2_0_MNCT"; break;
				case CONF_ID_TP2_0_MNTB:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_MNTB;			     strConfigID = "CONF_ID_TP2_0_MNTB"; break;
				case CONF_ID_TP2_0_MNT:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_MNT;			     strConfigID = "CONF_ID_TP2_0_MNT"; break;
				case CONF_ID_TP2_0_T_WAIT:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_T_Wait; 		     strConfigID = "CONF_ID_TP2_0_T_WAIT"; break;
				case CONF_ID_TP2_0_T1:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_T1; 			     strConfigID = "CONF_ID_TP2_0_T1"; break;
				case CONF_ID_TP2_0_T3:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_T3; 			     strConfigID = "CONF_ID_TP2_0_T3"; break;
				case CONF_ID_TP2_0_IDENTIFER:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_Identifer;		 strConfigID = "CONF_ID_TP2_0_IDENTIFER"; break;
				case CONF_ID_TP2_0_RXIDPASSIVE:
					pstTmpSConfig[i].Value = g_stGITSetConfig_2.nTP20_RxIdPassive;	     strConfigID = "CONF_ID_TP2_0_RXIDPASSIVE"; break;
#endif
				default :
					strConfigID = "~~~~~~~~ not defined ~~~~~~~~~~~~~";
					break;
			}
			GLogI("[%s] IOCTL section %s(0x%X), Value 0x%X(%d)\r\n",
				__FUNCTION__, strConfigID, pstTmpSConfig[i].Parameter, pstTmpSConfig[i].Value, pstTmpSConfig[i].Value);

			*pulOutDataLen += (i*sizeof(stSCONFIG));
			memcpy(pOutData+*pulOutDataLen, &pstTmpSConfig[i], sizeof(stSCONFIG));
		}

		*pulOutDataLen = sizeof(SCONFIG_LIST) + (stSconfigList.NumOfParams*sizeof(stSCONFIG));

		free(pstTmpSConfig);
	}
	else
	{
		GLogI("[%s] Parameter invalid\r\n", __FUNCTION__);
	}
}

void VCI_SetSConfigList(uint8_t* pData, BOOL bIsStructCopy)//void SetHWAndConfigPara(void)
{
#if defined(DEBUG_GIT_VCI_LOG)
	GLogI("[%s] bIsStructCopy %d run\r\n", __FUNCTION__, bIsStructCopy);
#endif

	if ( bIsStructCopy )
	{
		memcpy((void*)&g_stGITSetConfig.nDataRate, pData, sizeof(stGITSetConfig));
        
#if 0//CANFD_qhyek //Q_hyek CANFD
        if((g_stGITSetConfig.nEtc3 & 0x8000) == 0x8000)
        {
          GLogN("\r\n++++++++++ CANFD MODE ++++++++++\r\n");
          g_ucCanformat = CAN_FRAMEFORMAT_FDCAN;
          g_stGITSetConfig.nEtc3 = g_stGITSetConfig.nEtc3^0x8000;
        }
        else
        {
          g_ucCanformat = CAN_FRAMEFORMAT_CLASSIC;
        }
#endif
        
#if defined(HS_MODIFY)
		// g_stGITSetConfig?∏Ï?? g_stGITSetConfig_2 ?∏Ï?? Íµ¨Î∂Ñ?¥Ïïº ?úÎã§.
		//memcpy((void*)&g_stGITSetConfig_2.nCanMixedFormat, pData, sizeof(stGITSetConfig_2));
#endif
	}
	else
	{
		char* strConfigID;
		uint32_t i, uiNumOfParms;
		stSCONFIG stTmpSConfig;

		memcpy(&uiNumOfParms, pData, sizeof(uint32_t));

		for ( i=0; i<uiNumOfParms; i++)
		{
			memcpy(&stTmpSConfig, pData+4+(i*8), sizeof(stSCONFIG));

			switch(stTmpSConfig.Parameter)
			{
				case CONF_ID_DATA_RATE:
					g_stGITSetConfig.nDataRate 		= stTmpSConfig.Value;
					//Oem_UartSetBaudRate(UART_DLC, g_stGITSetConfig.nDataRate);
					strConfigID = "CONF_ID_DATA_RATE";
					break;
				case CONF_ID_LOOPBACK:
					g_stGITSetConfig.nLoopBack 		= stTmpSConfig.Value; strConfigID = "CONF_ID_LOOPBACK"; break;
				case CONF_ID_NODE_ADDRESS:
					g_stGITSetConfig.nNodeAddress 	= stTmpSConfig.Value; strConfigID = "CONF_ID_NODE_ADDRESS"; break;
				case CONF_ID_NETWORK_LINE:
					g_stGITSetConfig.nNetworkLine 	= stTmpSConfig.Value; strConfigID = "CONF_ID_NETWORK_LINE"; break;
				case CONF_ID_P1_MIN:
					g_stGITSetConfig.nP1Min 		= stTmpSConfig.Value; strConfigID = "CONF_ID_P1_MIN"; break;
				case CONF_ID_P1_MAX:
					g_stGITSetConfig.nP1Max 		= stTmpSConfig.Value; strConfigID = "CONF_ID_P1_MAX"; break;
				case CONF_ID_P2_MIN:
					g_stGITSetConfig.nP2Min 		= stTmpSConfig.Value; strConfigID = "CONF_ID_P2_MIN"; break;
				case CONF_ID_P2_MAX:
					g_stGITSetConfig.nP2Max 		= stTmpSConfig.Value; strConfigID = "CONF_ID_P2_MAX"; break;
				case CONF_ID_P3_MIN:
					g_stGITSetConfig.nP3Min 		= stTmpSConfig.Value; strConfigID = "CONF_ID_P3_MIN"; break;
				case CONF_ID_P3_MAX:
					g_stGITSetConfig.nP3Max 		= stTmpSConfig.Value; strConfigID = "CONF_ID_P3_MAX"; break;
				case CONF_ID_P4_MIN:
					g_stGITSetConfig.nP4Min 		= stTmpSConfig.Value; strConfigID = "CONF_ID_P4_MIN"; break;
				case CONF_ID_P4_MAX:
					g_stGITSetConfig.nP4Max 		= stTmpSConfig.Value; strConfigID = "CONF_ID_P4_MAX"; break;
				case CONF_ID_W1:
					g_stGITSetConfig.nW1 			= stTmpSConfig.Value; strConfigID = "CONF_ID_W1"; break;
				case CONF_ID_W2:
					g_stGITSetConfig.nW2 			= stTmpSConfig.Value; strConfigID = "CONF_ID_W2"; break;
				case CONF_ID_W3:
					g_stGITSetConfig.nW3 			= stTmpSConfig.Value; strConfigID = "CONF_ID_W3"; break;
				case CONF_ID_W4:
					g_stGITSetConfig.nW4 			= stTmpSConfig.Value; strConfigID = "CONF_ID_W4"; break;
				case CONF_ID_W5:
					g_stGITSetConfig.nW5 			= stTmpSConfig.Value; strConfigID = "CONF_ID_W5"; break;
				case CONF_ID_TIDLE:
					g_stGITSetConfig.nTIdle 		= stTmpSConfig.Value; strConfigID = "CONF_ID_TIDLE"; break;
				case CONF_ID_TINIL:
					g_stGITSetConfig.nTInil 		= stTmpSConfig.Value; strConfigID = "CONF_ID_TINIL"; break;
				case CONF_ID_TWUP:
					g_stGITSetConfig.nTWUp 			= stTmpSConfig.Value; strConfigID = "CONF_ID_TWUP"; break;
				case CONF_ID_PARITY:
					g_stGITSetConfig.nParity 		= stTmpSConfig.Value; strConfigID = "CONF_ID_PARITY"; break;
				case CONF_ID_BIT_SAMPLE_POINT:
					g_stGITSetConfig.nBitSamplePoint = stTmpSConfig.Value; strConfigID = "CONF_ID_BIT_SAMPLE_POINT"; break;
				case CONF_ID_SYNC_JUMP_WIDTH:
					g_stGITSetConfig.nSyncJumpWidth = stTmpSConfig.Value; strConfigID = "CONF_ID_SYNC_JUMP_WIDTH"; break;
				case CONF_ID_W0:
					/*g_stGITSetConfig.nW0 		= stTmpSConfig.Value;*/	  strConfigID = "CONF_ID_W0"; break;
				case CONF_ID_T1_MAX:
					g_stGITSetConfig.nT1Max 		= stTmpSConfig.Value; strConfigID = "CONF_ID_T1_MAX"; break;
				case CONF_ID_T2_MAX:
					g_stGITSetConfig.nT2Max 		= stTmpSConfig.Value; strConfigID = "CONF_ID_T2_MAX"; break;
				case CONF_ID_T3_MAX:
					g_stGITSetConfig.nT3Max 		= stTmpSConfig.Value; strConfigID = "CONF_ID_T3_MAX"; break;
				case CONF_ID_T4_MAX:
					g_stGITSetConfig.nT4Max 		= stTmpSConfig.Value; strConfigID = "CONF_ID_T4_MAX"; break;
				case CONF_ID_T5_MAX:
					g_stGITSetConfig.nT5Max  		= stTmpSConfig.Value; strConfigID = "CONF_ID_T5_MAX"; break;
				case CONF_ID_ISO15765_BS:
					g_stGITSetConfig.nIso15765BS 	= stTmpSConfig.Value; strConfigID = "CONF_ID_ISO15765_BS"; break;
				case CONF_ID_ISO15765_STMIN:
					g_stGITSetConfig.nIso15765STMin = stTmpSConfig.Value; strConfigID = "CONF_ID_ISO15765_STMIN"; break;
				case CONF_ID_DATA_BITS:
					g_stGITSetConfig.nDataBits		= stTmpSConfig.Value; strConfigID = "CONF_ID_DATA_BITS"; break;
				case CONF_ID_FIVE_BAUD_MOD:
					g_stGITSetConfig.nFiveBaudMod 	= stTmpSConfig.Value; strConfigID = "CONF_ID_FIVE_BAUD_MOD"; break;
				case CONF_ID_BS_TX:
					g_stGITSetConfig.nBSTx			= stTmpSConfig.Value; strConfigID = "CONF_ID_BS_TX"; break;
				case CONF_ID_STMIN_TX:
					g_stGITSetConfig.nSTMinTx		= stTmpSConfig.Value; strConfigID = "CONF_ID_STMIN_TX"; break;
				case PENDING_ACK:
#ifdef CANFD_qhyek //Q_hyek CANFD
                    if((stTmpSConfig.Value & 0x8000) == 0x8000)
                    {
                      GLogN("\r\n++++++++++ CANFD MODE ++++++++++\r\n");
                      g_ucCanformat = CAN_FRAMEFORMAT_FDCAN;
                      stTmpSConfig.Value = stTmpSConfig.Value^0x8000;
                    }
                    else
                    {
                      g_ucCanformat = CAN_FRAMEFORMAT_CLASSIC;
                    }
                    VCI_HW_Setting(VCI_GetPassThruProtocolID());
#endif
					g_stGITSetConfig.nEtc3			= stTmpSConfig.Value; strConfigID = "PENDING_ACK"; break;
				case CONF_ID_ISO15765_WFT_MAX:
				//case CONF_ID_N_BR_MIN:
				case CONF_ID_ISO15765_PAD_VALUE:
				case CONF_ID_N_AS_MAX:
				case CONF_ID_N_AR_MAX:
				case CONF_ID_N_BS_MAX:
				case CONF_ID_N_CR_MAX:
				case CONF_ID_N_CS_MIN:
					strConfigID = "~~~~~~~~ not approved yet ~~~~~~~~ \r\n";
					break;
				default :
					strConfigID = "~~~~~~~~ not defined ~~~~~~~~~~~~~\r\n";
					break;
			}

			GLogN("[%s] IOCTL section %s(0x%X), Value 0x%X(%d)\r\n", __FUNCTION__, strConfigID, stTmpSConfig.Parameter, stTmpSConfig.Value, stTmpSConfig.Value);
		}
	}

	// ETC3¿« øÎµµ : «œ¿ß πŸ¿Ã∆Æ¥¬ øπø‹√≥∏Æ «√∑π±◊. ªÛ¿ßπŸ¿Ã∆Æ¥¬ OR√≥∏Æ ∞°¥…«— ∫Ò∆Æ∫∞ øπø‹√≥∏Æ «√∑π±◊.
    // ¿Ã»ƒ √ﬂ∞°µ«¥¬ «œ¿ß πŸ¿Ã∆Æ¥¬ ªÛ¿ßπŸ¿Ã∆ÆøÕ «‘≤≤ ªÁøÎ«“ºˆ ¿÷µµ∑œ ∏∂Ω∫≈∑√≥∏Æ «“∞Õ 190521 KKT
	/*
	if( g_stGITSetConfig.nEtc3== 0x01)		VPENDING_ACK = g_stGITSetConfig.nEtc3;
	else if( g_stGITSetConfig.nEtc3== 0x03)	GM_SID_MASK = g_stGITSetConfig.nEtc3;		//120224 LWH
	else if( g_stGITSetConfig.nEtc3== 0x04)	CV_VGT = g_stGITSetConfig.nEtc3;		//120321 LWH
	*/
	//else if( J2534SetConfigData.nEtc3== 0x05)	CV_CM_ACK = J2534SetConfigData.nEtc3;		//130111 LWH ¿œ¥‹ ∏∑¿Ω

	/*
	if((g_stGITSetConfig.nEtc3&0x0800)==0x0800)	g_ucCanBuffClearFlag = DISABLE;	// functionID:1003 ¿∏∑Œ CAN º€Ω≈ ¿¸ πˆ∆€≈¨∏ÆæÓ ºˆ«‡ «√∑°±◊, DEFAULT : ENABLE(ºˆ«‡) 190507 KKT
	else g_ucCanBuffClearFlag = ENABLE;
	if((g_stGITSetConfig.nEtc3&0x0100)==0x0100)	g_ucUDS_Data_Struct = 1;
	g_stGITSetConfig.nTIdle=1000;
	if((g_stGITSetConfig.nEtc3&0x0400)==0x0400)	//170825 KKT
	{	
		// 1. ªÛøÎ ∏Æ«¡∑Œ±◊∑•ø°º≠ ECUø°º≠ ∫∏≥ª¡÷¥¬ FLOW CONTROL¿« STMIN¿ª π´Ω√«œ∞Ì ∆Ø¡§ STMIN∞™¿∏∑Œ º€Ω≈«ÿæﬂ «œ¥¬ ∞ÊøÏ∞° ¿÷¿Ω.
		//    ¿Ã∑± ∞ÊøÏø° DB ¿« ETC3 ∞™¿Ã 0x300π¯¥Î¿Ã∏È µ⁄ µŒ¿⁄∏Æ∏¶ STMIN¿∏∑Œ ªÁøÎ«—¥Ÿ.
		// 2. 29bit CAN ¿« ∞ÊøÏ ECUø°º≠ ∫∏≥ª¡÷¥¬ FLOW CONTROL¿« BSMax ∞™¿ª π´Ω√«œ∞Ì π´¡∂∞« 0¿∏∑Œ ∫Ø∞Ê«—¥Ÿ.
		//    ¿Ã∑± ∞ÊøÏø° DB ¿« ETC3 ∞™¿Ã 0x300π¯¥Î¿Ã∏È BSMax ∞™¿ª ECUø°º≠ ∫∏≥ª¡÷¥¬ ∞™¿∏∑Œ ªÁøÎ«—¥Ÿ.
		g_ucCV_Except=1;
		g_ucCV_STMIN=g_stGITSetConfig.nSTMinTx;//(J2534SetConfigData.nEtc3&0x00FF);
	}
	else 
	{
		g_ucCV_Except=0;
		g_ucCV_STMIN=0;
	}
	*/

}

void VCI_SetSByteArray(uint8_t* pData)
{
	memcpy(&g_stGITSByteArray.NumOfBytes, pData, sizeof(g_stGITSByteArray.NumOfBytes));

	if ( g_stGITSByteArray.BytePtr != NULL ) free(g_stGITSByteArray.BytePtr);
	g_stGITSByteArray.BytePtr = malloc(g_stGITSByteArray.NumOfBytes);

	if ( g_stGITSByteArray.BytePtr )
		memcpy(g_stGITSByteArray.BytePtr, pData+sizeof(g_stGITSByteArray.NumOfBytes), g_stGITSByteArray.NumOfBytes);
	else
		GLogI("[%s] malloc fail ------------------ SBYTE Array\r\n", __FUNCTION__);

#if defined(DEBUG_GIT_VCI_LOG)
	{
		int i;
		GLogI("[%s] SByte NumOfBytes %d, SByte : ", __FUNCTION__, g_stGITSByteArray.NumOfBytes);
		for ( i=0; i<g_stGITSByteArray.NumOfBytes; i++ )
		{
			GLogI("[0x%02X] ", g_stGITSByteArray.BytePtr[i]);
		}
		GLogI("\r\n");
	}
#endif
}

void VCI_Clear_DLC_HW(void)
{
	InitIOCTL();
	deinitFDCan();
}

void VCI_HWSetParameter(uint8_t* pData)
{
	memcpy((void*)&g_stGITHWSetData, pData, sizeof(tagHARDWARESET));

	switch ( g_stGITHWSetData.nCommRelay )
	{
#if VCI1_TEST
		if(      g_stGITHWSetData.nCommRelay == 0x0000 ) g_stGITHWSetData.nCommRelay = OFF_RELAY;
		else if( g_stGITHWSetData.nCommRelay == 0x0080 ) g_stGITHWSetData.nCommRelay = ADIN_RELAY;
		else if( g_stGITHWSetData.nCommRelay == 0x0100 ) g_stGITHWSetData.nCommRelay = UART_RELAY;
		else if( g_stGITHWSetData.nCommRelay == 0x0200 ) g_stGITHWSetData.nCommRelay = J1850_RELAY;
		else if( g_stGITHWSetData.nCommRelay == 0x0400 ) g_stGITHWSetData.nCommRelay = J1708_RELAY;
		else if( g_stGITHWSetData.nCommRelay == 0x0800 ) g_stGITHWSetData.nCommRelay = CANLOW_RELAY;
		else if( g_stGITHWSetData.nCommRelay == 0x1000 ) g_stGITHWSetData.nCommRelay = CANHIGH_RELAY;
		else if( g_stGITHWSetData.nCommRelay == 0x2000 ) g_stGITHWSetData.nCommRelay = KLLINE_RELAY;
		else if( g_stGITHWSetData.nCommRelay == 0x2400 ) g_stGITHWSetData.nCommRelay = CANSIG_RELAY;
		else                           g_stGITHWSetData.nCommRelay = KLLINE_RELAY;
#endif
		case 0x0080:	g_stGITHWSetData.nCommRelay = ADIN_RELAY;	break;
		case 0x0100:	g_stGITHWSetData.nCommRelay = UART_RELAY;	break;
		case 0x0200:	g_stGITHWSetData.nCommRelay = J1850_RELAY;	break;
		case 0x0400:	g_stGITHWSetData.nCommRelay = J1708_RELAY;	break;
		//case 0x0800:	g_stGITHWSetData.nCommRelay = CANLOW_RELAY;	break;
		//case 0x1000:	g_stGITHWSetData.nCommRelay = CANHIGH_RELAY;break;
		case 0x2400:	g_stGITHWSetData.nCommRelay = CANSIG_RELAY;	break;		
		case 0x0800:
			g_stGITHWSetData.nCommRelay = Lowcan1;//CANLOW_RELAY;
            g_ucCAN_CH = 2;
            break;
		case 0x1000:
            g_stGITHWSetData.nCommRelay = Highcan1;//CANHIGH_RELAY;
            g_ucCAN_CH = 1;
            break;
		case 0x0000:
		//case 0x0100:
		case 0x2000:
		default: 	 g_stGITHWSetData.nCommRelay = KLLINE_RELAY; break;
	}

	switch ( g_stGITHWSetData.nKlineSelect )
	{
		case 0x0000: g_stGITHWSetData.nKlineSelect = K_SERIAL; break;//Serial
		default: 	 g_stGITHWSetData.nKlineSelect = K_PULSE; break;//Pulse
	}

	switch ( g_stGITHWSetData.nKlineStatus )
	{
		case 0x0000: g_stGITHWSetData.nKlineStatus = K_INVERSE; break;//NormalLow
		default: 	 g_stGITHWSetData.nKlineStatus = K_NORMAL; break;//NormalHigh
	}

	switch ( g_stGITHWSetData.nLlineSelect )
	{
		case 0x0000: g_stGITHWSetData.nLlineSelect = L_SERIAL; break;
		default: 	 g_stGITHWSetData.nLlineSelect = L_PULSE; break;
	}

	switch ( g_stGITHWSetData.nLlineStatus )
	{
		case 0x0000: g_stGITHWSetData.nLlineStatus = L_INVERSE; break;//NormalLow
		default: 	 g_stGITHWSetData.nLlineStatus = L_NORMAL; break;//NormalHigh
	}
	
#if 1	// ?¨Ïö©?òÎäî?∞Í? ?ÜÎäî ??
	switch ( g_stGITHWSetData.nKlineSwitchStatus )
	{
		case 0x0000: g_stGITHWSetData.nKlineSwitchStatus = K_PULSE_LOW; break;
		default: 	 g_stGITHWSetData.nKlineSwitchStatus = K_PULSE_HIGH; break;
	}

	switch ( g_stGITHWSetData.nLlineSwitchStatus )
	{
		case 0x0000: g_stGITHWSetData.nLlineSwitchStatus = L_PULSE_LOW; break;
		default: 	 g_stGITHWSetData.nLlineSwitchStatus = L_PULSE_HIGH; break;
	}
#endif

	switch ( g_stGITHWSetData.nRxLineSelect )
	{
		case 0x000C: g_stGITHWSetData.nRxLineSelect = RXD_HIGHCAN;	break;
		case 0x002C: g_stGITHWSetData.nRxLineSelect = RXD_LOWCAN;	break;
		case 0x004C: g_stGITHWSetData.nRxLineSelect = RXD_SIGCAN;	break;
		case 0x0060: g_stGITHWSetData.nRxLineSelect = RXD_L_FB;		break;
		case 0x0061: g_stGITHWSetData.nRxLineSelect = RXD_J1850;	break;
		case 0x0062: g_stGITHWSetData.nRxLineSelect = RXD_OBD1;		break;
		case 0x0063: g_stGITHWSetData.nRxLineSelect = RXD_485;		break;
		case 0x0064: g_stGITHWSetData.nRxLineSelect = RXD_RS232;	break;
		case 0x0068: g_stGITHWSetData.nRxLineSelect = RXD_OBD2;		break;
		default:	g_stGITHWSetData.nRxLineSelect = RXD_OBD1;		break;
	}

	switch ( g_stGITHWSetData.nRxLineStatus )
	{
		case 0x0000: g_stGITHWSetData.nRxLineStatus = RXD_NORMAL; break;//OBD2
		default: 	 g_stGITHWSetData.nRxLineStatus = RXD_INVERSE; break;//OBD1
	}

	switch ( g_stGITHWSetData.nKlinePullup )
	{
		case 0x0001: g_stGITHWSetData.nKlinePullup = K_Pullup_510; break;
		case 0x0004: g_stGITHWSetData.nKlinePullup = K_Pullup_47k; break;
		case 0x0020: g_stGITHWSetData.nKlinePullup = K_Pullup_2k; break;
		default: 	 g_stGITHWSetData.nKlinePullup = K_Pullup_Off; break;
	}

	switch ( g_stGITHWSetData.nLlinePullup )
	{
		case 0x0002: g_stGITHWSetData.nLlinePullup = L_Pullup_510; break;
		case 0x0008: g_stGITHWSetData.nLlinePullup = L_Pullup_47k; break;
		case 0x0040: g_stGITHWSetData.nLlinePullup = L_Pullup_2k; break;
		default: 	 g_stGITHWSetData.nLlinePullup = L_Pullup_Off; break;
	}

	switch ( g_stGITHWSetData.nKlineCh )
	{
		case  0x0000: g_stGITHWSetData.nKlineCh = K_ChOff;	break;
		case  0x0001: g_stGITHWSetData.nKlineCh = KL_LINE1_CONNECT_CH01;	break;//1pin
		case  0x0002: g_stGITHWSetData.nKlineCh = KL_LINE1_CONNECT_CH02;	break;//2pin
		case  0x0004: g_stGITHWSetData.nKlineCh = KL_LINE1_CONNECT_CH03;	break;//3pin
		case  0x0008: g_stGITHWSetData.nKlineCh = KL_LINE1_CONNECT_CH06;	break;//6pin
		case  0x0010: g_stGITHWSetData.nKlineCh = KL_LINE1_CONNECT_CH07;	break;//7pin
		case  0x0020: g_stGITHWSetData.nKlineCh = KL_LINE1_CONNECT_CH08;	break;//8pin
		case  0x0040: g_stGITHWSetData.nKlineCh = KL_LINE2_CONNECT_CH09;	break;//9pin
		case  0x0080: g_stGITHWSetData.nKlineCh = KL_LINE2_CONNECT_CH10;	break;//10pin
		case  0x0100: g_stGITHWSetData.nKlineCh = KL_LINE2_CONNECT_CH11;	break;//11pin
		case  0x0200: g_stGITHWSetData.nKlineCh = KL_LINE2_CONNECT_CH12;	break;//12pin
		case  0x0400: g_stGITHWSetData.nKlineCh = KL_LINE1_CONNECT_CH13;	break;//13pin
		case  0x0800: g_stGITHWSetData.nKlineCh = KL_LINE2_CONNECT_CH14;	break;//14pin
		case  0x1000: g_stGITHWSetData.nKlineCh = KL_LINE1_CONNECT_CH15;	break;//15pin
		case  0x1001: g_stGITHWSetData.nKlineCh = K_ChOff;	break;//16pin
		default:	g_stGITHWSetData.nKlineCh = KL_LINE1_CONNECT_CH07;	 break;//7pin
	}

	switch ( g_stGITHWSetData.nLlineCh )
	{
		case  0x0000: g_stGITHWSetData.nLlineCh = K_ChOff;	break;
		case  0x0001: g_stGITHWSetData.nLlineCh = KL_LINE1_CONNECT_CH01;	break;//1pin
		case  0x0002: g_stGITHWSetData.nLlineCh = KL_LINE1_CONNECT_CH02;	break;//2pin
		case  0x0004: g_stGITHWSetData.nLlineCh = KL_LINE1_CONNECT_CH03;	break;//3pin
		case  0x0008: g_stGITHWSetData.nLlineCh = KL_LINE1_CONNECT_CH06;	break;//6pin
		case  0x0010: g_stGITHWSetData.nLlineCh = KL_LINE1_CONNECT_CH07;	break;//7pin
		case  0x0020: g_stGITHWSetData.nLlineCh = KL_LINE1_CONNECT_CH08;	break;//8pin
		case  0x0040: g_stGITHWSetData.nLlineCh = KL_LINE2_CONNECT_CH09;	break;//9pin
		case  0x0080: g_stGITHWSetData.nLlineCh = KL_LINE2_CONNECT_CH10;	break;//10pin
		case  0x0100: g_stGITHWSetData.nLlineCh = KL_LINE2_CONNECT_CH11;	break;//11pin
		case  0x0200: g_stGITHWSetData.nLlineCh = KL_LINE2_CONNECT_CH12;	break;//12pin
		case  0x0400: g_stGITHWSetData.nLlineCh = KL_LINE1_CONNECT_CH13;	break;//13pin
		case  0x0800: g_stGITHWSetData.nLlineCh = KL_LINE2_CONNECT_CH14;	break;//14pin
		case  0x1000: g_stGITHWSetData.nLlineCh = KL_LINE1_CONNECT_CH15;	break;//15pin
		case  0x1001: g_stGITHWSetData.nLlineCh = K_ChOff;	break;//16pin
		default:	g_stGITHWSetData.nLlineCh = KL_LINE1_CONNECT_CH07;	 break;//7pin
	}

#if defined(DEBUG_GIT_VCI_LOG)
	GLogI("[%s]nCommRelay %d, nKlineSelect %d, KlineStatus %d, nLlineSelect %d, nLlineStatus %d, nKlineSwitchStatus %d\r\n",
		__FUNCTION__, g_stGITHWSetData.nCommRelay, g_stGITHWSetData.nKlineSelect, g_stGITHWSetData.nKlineStatus,
		g_stGITHWSetData.nLlineSelect, g_stGITHWSetData.nLlineStatus, g_stGITHWSetData.nKlineSwitchStatus);
	GLogI("[%s]nLlineSwitchStatus %d, nRxLineSelect %d, nRxLineStatus %d, nPullupRelay %d, nKlinePullup %d, nLlinePullup %d\r\n",
		__FUNCTION__, g_stGITHWSetData.nLlineSwitchStatus, g_stGITHWSetData.nRxLineSelect, g_stGITHWSetData.nRxLineStatus,
		g_stGITHWSetData.nPullupRelay, g_stGITHWSetData.nKlinePullup, g_stGITHWSetData.nLlinePullup);
	GLogI("[%s]nLlineGnd %d, nKlineCh %d, nLlineCh %d, nRProgramCh %d, nEtc1 %d, nEtc2 %d, nEtc3 %d, nEtc4 %d, nEtc5 %d\r\n",
		__FUNCTION__, g_stGITHWSetData.nLlineGnd, g_stGITHWSetData.nKlineCh, g_stGITHWSetData.nLlineCh, g_stGITHWSetData.nRProgramCh ,
		g_stGITHWSetData.nEtc1, g_stGITHWSetData.nEtc2, g_stGITHWSetData.nEtc3, g_stGITHWSetData.nEtc4, g_stGITHWSetData.nEtc5);
#endif
}


////////////////////////////////////////////////////////////////////////////////
void VCI_SetRtcTime(uint8_t *pSetDate)
{
	Set_RTCData(pSetDate, 1);
}

void VCI_GetRtcTime(uint8_t *pSetDate)
{
	Get_RTCData(pSetDate);
}

void VCI_HW_Setting(uint32_t uiPID)
{
	u32 nStartMask[5];
	u32 nEndMask[5];
    U8 MaskNum = 0;
	u32 nCanType;
	u32 nCANBaudrate;
	u32 ret;
#ifdef CANFD_qhyek//Q_hyek CANFD
	U8 CANformat;
#endif

	g_ulProtocolID = uiPID;
	VCI_REINTI_COMM_STATE(uiPID);

#if defined(HS_MODIFY)
	if( g_stGITSetConfig_2.nJ1962_Pins > 0 )
	{
		Oem_J1962PinUse(g_stGITSetConfig_2.nJ1962_Pins);//J2534-2 Pin Select
	}
#endif
	g_bAckflag = 0;
	//DTC_flag= 0;
	g_uiAckTiming = 1000;

	switch(uiPID)
	{
		case ISO9141_2:       //OK
		case ISO9141_2_00D1:				// ?êÏø†??lz) ?§ÌÜ†?êÏñ¥ÏΩ?ÍπÄ?ôÌòÑ Ï∂îÍ? ?îÏ≤≠(qir) PROTOCOL ?ÜÏùå
		case ISO14230:        //OK
		case ISO14230_ETC:	
		case ISO9141_2_5BPSTXONLY:  // OK
		case ISO9141_2_DW_SIEMENSE:
		case ISO9141_2_SyncTime:  //OK
        //case ISO9141_2_CV_4BYTE:
		
			 if(g_stGITSetConfig.nParity==1)
			 {
				DLC_HW_Set(KLLINE_RELAY,K_SERIAL,K_NORMAL,L_PULSE,L_NORMAL,0,L_PULSE_HIGH,RXD_OBD1,RXD_NORMAL,Pullup,K_Pullup_510,L_Pullup_510,0,g_stGITHWSetData.nKlineCh,g_stGITHWSetData.nLlineCh,R_ChOff);
			 }
			 else
			 {
			 	DLC_HW_Set(KLLINE_RELAY,K_SERIAL,K_NORMAL,L_PULSE,L_NORMAL,0,L_PULSE_HIGH,RXD_OBD2,RXD_NORMAL,Pullup,K_Pullup_510,L_Pullup_510,0,g_stGITHWSetData.nKlineCh,g_stGITHWSetData.nLlineCh,R_ChOff);
			 }
//			 DLC_HW_Set(KLLINE_RELAY,K_SERIAL,K_NORMAL,L_PULSE,L_NORMAL,0,L_PULSE_HIGH,RXD_OBD1,RXD_NORMAL,Pullup,K_Pullup_510,L_Pullup_510,0,KlineCh,LlineCh,R_ChOff);
			 //uartSetBaudRate(UART_DLC,VDATA_RATE);
			 //SetBaudrateKLine(GetKlineSelect(), 10416 );
			 ret = uartSetBaudRate(GetKlineSelect(), g_stGITSetConfig.nDataRate );
			 if( ret != 0 )
			 {
				GLogE( "Error... Fail uart baurate set(%d)!!!\r\n", ret );
			 }
			 g_uiAckTiming = 1000;
			 
			 break;
			 
		case ISO14230_POWERTEC:
		  	DLC_HW_Set(KLLINE_RELAY,K_SERIAL,K_NORMAL,L_PULSE,L_NORMAL,0,L_PULSE_HIGH,RXD_OBD1,RXD_NORMAL,Pullup,K_Pullup_510,L_Pullup_510,0,g_stGITHWSetData.nKlineCh,g_stGITHWSetData.nLlineCh,R_ChOff);
			ret = uartSetBaudRate(GetKlineSelect(), g_stGITSetConfig.nDataRate );
			if( ret != 0 )
			{
				GLogE( "Error... Fail uart baurate set(%d)!!!\r\n", ret );
			}
		  	break;
		
	case ISO9141_BOSCH:
	//case IS09141_2_ALLISON_ATM:
	 		DLC_HW_Set(KLLINE_RELAY,K_SERIAL,K_NORMAL,L_PULSE,L_NORMAL,0,L_PULSE_HIGH,RXD_OBD1,RXD_NORMAL,Pullup,K_Pullup_47k,L_Pullup_47k,0,g_stGITHWSetData.nKlineCh,g_stGITHWSetData.nLlineCh,R_ChOff);
	  		ret = uartSetBaudRate(GetKlineSelect(), g_stGITSetConfig.nDataRate );
			if( ret != 0 )
			{
				GLogE( "Error... Fail uart baurate set(%d)!!!\r\n", ret );
			}
			 g_uiAckTiming = 700;
	  		break;
			 
		case PID_CAN:
		case ISO15765:   //OK
		case ISO15765_NEW:
		case ISO14229_UDS: //OK
		case ISO15765_SMK://for reprogram rom id read 20221211 kkt
		//case ISO15765_SMK_NEW:			//110908 LWH
		case ISO14229:
		case ISO14229_ES95486_170:
		case ISO14229_ES95486_170_MMCAN:
		case ISO14230_ES95486_170:
		case ISO14230_ES95486_170_MMCAN:
		case ISO14229_ES95486_02_100:
		case ISO14229_ES95486_02_100_NEW:			//20190918 Jay
#ifdef HOTA
        case ISO14229_ES95486_02_HOTA:
#endif
          
#ifdef CANFD_PROTOCOL
        case ISO14229_ES95486_02_130_CANFD:
        case ISO14229_ES95486_02_131_CANFD:
#endif
		case ISO14229_ES95486_02_102:
		case ISO14229_ES95486_02_103:
		case ISO14229_ES95486_02_104:
		case ISO14229_ES95486_02_105:
		case ISO14229_ES95486_02_106:
		case ISO14229_ES95486_02_107:
		case ISO14229_ES95486_02_108:
		case ISO14229_ES95486_02_109:
		case ISO14229_ES95486_02_10A:
		case ISO14229_ES95486_02_10B:
		case ISO14229_ES95486_02_10C:
		case ISO14229_ES95486_02_10D:
		case ISO14229_ES95486_02_10E:
		case ISO14229_ES95486_02_10F:
		case ISO14230_ES95486_DOIP_120:
		case ISO14230_ES95486_DOIP_121:
		case ISO14230_ES95486_DOIP_122:
		case ISO14230_ES95486_DOIP_123:
		case ISO14230_ES95486_DOIP_124:
		case ISO14230_ES95486_DOIP_125:
		case ISO14230_ES95486_DOIP_126:
		case ISO14230_ES95486_DOIP_127:
		case ISO14230_ES95486_DOIP_128:
		case ISO14230_ES95486_DOIP_129:
		case ISO14230_ES95486_DOIP_12A:
		case ISO14230_ES95486_DOIP_12B:
		case ISO14230_ES95486_DOIP_12C:
		case ISO14230_ES95486_DOIP_12D:
		case ISO14230_ES95486_DOIP_12E:
		case ISO14230_ES95486_DOIP_12F:
#ifdef NEW_29BIT_CAN
        case ISO15765_ES95486_29bit:
		case ISO15765_ES95486_DOIP_29BIT:
#ifdef CANFD_PROTOCOL
        case ISO15765_ES95486_135_29bit_CANFD:
        case ISO15765_ES95486_136_29bit_CANFD:
#endif
#endif
		case ISO15765_CUBIS:
		case ISO15765_SINGLE:
		case ISO15765_SINGLE_SMK:
		case ISO15765_SINGLE_PODS:		//101224 LWH
		//case ISO15765_SID36:			//110411 LWH
		case ISO15765_CARB:     //OK
		case ISO15765_CARB_NEW: 
		case ISO15765_CARB_NEW_LENGTH:
        case J1939_23_CARB_NEW_LENGTH:
		case ISO15765_CARB_29BIT:
		case ISO15765_CARB_29BIT_NEW:
        case J1939_23_CARB_29BIT_NEW:
		case ISO15765_REPRO_PENNIMG:
		case ISO15765_REPRO_PENNIMG_TIME:
		//case ISO15765_SM_MULTI:			//101102 LWH
		case ISO15765_NORMAL:			//110602 LWH
		case ISO15765_CAN_HWSET_DB:		//110620 LWH
		case ISO15765_CAN_HWSET_DB_SINGLE://111013 LWH
		case ISO15765_CAN_HWSET_DB_NEW:	// 150224 seo
		case ISO15765_EXCEPT:			//110715 LWH     OK
		//case ISO15765_GM:
		//case ISO15765_GM_100K:
		case ISO15765_ACU_SINGLE:		// 130619 √÷«ˆ¡§
		case ISO14229_UDS_HWSET_DB:
        case ISO15765_29BIT:
		case ISO15765_29BIT_EXCEPT:		//110609 LWH
		case J1939:
	  	case J1939_4PGN:
		//default:
			if ( uiPID == ISO15765_CARB_29BIT		|| 
				uiPID == ISO15765_CARB_29BIT_NEW 	|| 
                uiPID == J1939_23_CARB_29BIT_NEW    ||
				uiPID == ISO15765_29BIT				|| 
				uiPID == ISO15765_29BIT_EXCEPT		||
				uiPID ==	J1939					||	
				uiPID ==	J1939_4PGN							
#ifdef NEW_29BIT_CAN
                  || uiPID ==	ISO15765_ES95486_29bit
                  || uiPID ==	ISO15765_ES95486_DOIP_29BIT
#ifdef CANFD_PROTOCOL
                  || uiPID ==	ISO15765_ES95486_135_29bit_CANFD
                  || uiPID ==	ISO15765_ES95486_136_29bit_CANFD
#endif
#endif
                    )	nCanType = EXTENDED_CAN;
			else 															nCanType = STANDARD_CAN;

			if ( nCanType == EXTENDED_CAN )
			{
                if( uiPID == ISO15765_29BIT )
                {
                	g_uiAckTiming = 1000;
                    nStartMask[0] = 0x18DA0000;//2018.08.07 LJH MASKING ?ïÏù∏?ÑÏöî
                    nEndMask[0] = 0x18DAFFFF;
                    MaskNum = 1;
                }
				else if( uiPID ==	J1939 || uiPID == J1939_4PGN )
				{
				  	g_uiAckTiming = 3000;
				  	nStartMask[0] = 0x00000000;
					nEndMask[0] = 0x1FFFFFFF;
                    MaskNum = 1;
					if((g_stGITSetConfig.nEtc4&0x00FF) == 0x04) g_uiAckTiming = 500;
				}
				else if( uiPID == ISO14229_ES95486_02_10F || uiPID == ISO14230_ES95486_DOIP_12F )
				{
				  	g_uiAckTiming = 3000;
				  	nStartMask[0]	= 0x500;
					nEndMask[0]		= 0x5FF;
					nStartMask[1]	= 0x700;
					nEndMask[1]		= 0x7FF;
					nStartMask[2]	= 0x17FF0000;
					nEndMask[2]		= 0x17FFFFFF;
					nStartMask[3]	= 0x18DA0000;
					nEndMask[3]		= 0x18DAFFFF;
                    MaskNum = 4;
				}
                else if( uiPID == ISO15765_ES95486_29bit || uiPID == ISO15765_ES95486_DOIP_29BIT )
                {
                    g_uiAckTiming = 3000;
                    MaskNum = 1;
                    
                     if (( g_stGITSetConfig.nEtc4 != 0 )&&( g_stGITSetConfig.nEtc5 != 0 ))
                    {
                        nStartMask[0] = g_stGITSetConfig.nEtc4;
                        nEndMask[0] = g_stGITSetConfig.nEtc5;
                    }
                    else if( g_stGITSetConfig.nEtc4 != 0 )
                    {
                        nStartMask[0] = g_stGITSetConfig.nEtc4;
                        nEndMask[0] = g_stGITSetConfig.nEtc4;
                    }
                    else if( g_stGITSetConfig.nEtc5 != 0 )
                    {
                        nStartMask[0] = g_stGITSetConfig.nEtc5;
                        nEndMask[0] = g_stGITSetConfig.nEtc5;
                    }
                    else
                    {
                        nStartMask[0] = 0x17FF0000;
                        nEndMask[0] = 0x17FFFFFF;
                    }
                }
#ifdef CANFD_PROTOCOL
                else if(uiPID ==	ISO15765_ES95486_135_29bit_CANFD || uiPID ==	ISO15765_ES95486_136_29bit_CANFD)
                {
                    g_uiAckTiming = 3000;
                    nStartMask[0] = 0x17FF0000;
                    nEndMask[0] = 0x18DAFFFF;
                    MaskNum = 1;
                }
#endif
                else
                {
				  	g_uiAckTiming = 3000;
                    MaskNum = 1;
                    nStartMask[0] = 0x001A0000;//2018.08.07 LJH MASKING ?ïÏù∏?ÑÏöî
                    nEndMask[0] = 0x18DAFFFF;
                }
			}
			else
			{
				g_uiAckTiming = 3000;
				if (( g_stGITSetConfig.nEtc4 != 0 )&&( g_stGITSetConfig.nEtc5 != 0 ))       //160725 LWH ÎßàÏä§??Î°úÏßÅ ?òÏ†ï g_stGITHWSetData -> g_stGITSetConfig
				{
					nStartMask[0] = g_stGITSetConfig.nEtc4;
					nEndMask[0] = g_stGITSetConfig.nEtc5;
                    MaskNum = 1;
				}
                else if( g_stGITSetConfig.nEtc4 != 0 )
                {
					nStartMask[0] = g_stGITSetConfig.nEtc4;
					nEndMask[0] = g_stGITSetConfig.nEtc4;
                    MaskNum = 1;
                }
                else if( g_stGITSetConfig.nEtc5 != 0 )
                {
					nStartMask[0] = g_stGITSetConfig.nEtc5;
					nEndMask[0] = g_stGITSetConfig.nEtc5;
                    MaskNum = 1;
                }
                else if((uiPID == ISO14229_ES95486_02_100)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_102)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_103)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_104)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_105)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_106)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_107)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_108)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_109)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_10A)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_10B)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_10C)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_10D)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_10E)||				//20190918 Jay
						(uiPID == ISO14229_ES95486_02_100_NEW)				//20190918 Jay
#ifdef HOTA
                        ||(uiPID == ISO14229_ES95486_02_HOTA)
#endif
#ifdef CANFD_PROTOCOL
                        ||(uiPID == ISO14229_ES95486_02_130_CANFD)
						||(uiPID == ISO14229_ES95486_02_131_CANFD)
#endif
						||(uiPID == ISO14230_ES95486_DOIP_120)
						||(uiPID == ISO14230_ES95486_DOIP_121)
						||(uiPID == ISO14230_ES95486_DOIP_122)
						||(uiPID == ISO14230_ES95486_DOIP_123)
						||(uiPID == ISO14230_ES95486_DOIP_124)
						||(uiPID == ISO14230_ES95486_DOIP_125)
						||(uiPID == ISO14230_ES95486_DOIP_126)
						||(uiPID == ISO14230_ES95486_DOIP_127)
						||(uiPID == ISO14230_ES95486_DOIP_128)
						||(uiPID == ISO14230_ES95486_DOIP_129)
						||(uiPID == ISO14230_ES95486_DOIP_12A)
						||(uiPID == ISO14230_ES95486_DOIP_12B)
						||(uiPID == ISO14230_ES95486_DOIP_12C)
						||(uiPID == ISO14230_ES95486_DOIP_12D)
						||(uiPID == ISO14230_ES95486_DOIP_12E)
						)
                    	//(uiPID == ISO14229_ES95486_170)||
                    	//(uiPID == ISO14229_ES95486_170_MMCAN)||
                    	//(uiPID == ISO14230_ES95486_170)||
                    	//(uiPID == ISO14230_ES95486_170_MMCAN))
                {
#if 1
					nStartMask[0] = 0x0580; //180510 ÏßÑÎã®?¨Ïñë ?îÏ≤≠?ºÎ°ú 0x0500Î∂Ä??Î∞õÏùÑ ???àÎèÑÎ°??òÏ†ï??
					nEndMask[0] = 0x05FF;
                    nStartMask[1] = 0x0700; //180510 ÏßÑÎã®?¨Ïñë ?îÏ≤≠?ºÎ°ú 0x0500Î∂Ä??Î∞õÏùÑ ???àÎèÑÎ°??òÏ†ï??
					nEndMask[1] = 0x07FF;
                    MaskNum = 2;
#else
                    nStartMask[0] = 0x0500; //180510 ÏßÑÎã®?¨Ïñë ?îÏ≤≠?ºÎ°ú 0x0500Î∂Ä??Î∞õÏùÑ ???àÎèÑÎ°??òÏ†ï??
					nEndMask[0] = 0x07FF;
                    MaskNum = 1;
#endif
                }
				else if( uiPID == ISO14229_ES95486_02_10F || uiPID == ISO14230_ES95486_DOIP_12F )
				{
				  	g_uiAckTiming = 3000;
				  	nStartMask[0]	= 0x500;
					nEndMask[0]		= 0x5FF;
					nStartMask[1]	= 0x700;
					nEndMask[1]		= 0x7FF;
					nStartMask[2]	= 0x17FF0000;
					nEndMask[2]		= 0x17FFFFFF;
					nStartMask[3]	= 0x18DA0000;
					nEndMask[3]		= 0x18DAFFFF;
                    MaskNum = 4;
				}
				else
				{
					nStartMask[0] = 0x0700; // set Default Masking value
					nEndMask[0] = 0x07FF;
                    MaskNum = 1;
				}
			}
			// CAN ID???Ä???àÏô∏Ï≤òÎ¶¨ Ï∂îÍ?
            for(int i=0; i<MaskNum; i++)
            {
                if ( nStartMask[i] > nEndMask[i] )
                {
                    u32 nTmp = nStartMask[i];
                    nStartMask[i] = nEndMask[i];
                    nEndMask[i] = nTmp;
                }
            }

            if( (uiPID == ISO15765_CAN_HWSET_DB )||
				(uiPID == ISO14229_ES95486_02_100)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_102)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_103)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_104)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_105)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_106)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_107)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_108)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_109)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_10A)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_10B)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_10C)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_10D)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_10E)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_10F)||				//20190918 Jay
				(uiPID == ISO14229_ES95486_02_100_NEW)||		//20190918 Jay
#ifdef HOTA
                (uiPID == ISO14229_ES95486_02_HOTA)||
#endif
#ifdef CANFD_PROTOCOL
                (uiPID == ISO14229_ES95486_02_130_CANFD)||
                (uiPID == ISO14229_ES95486_02_131_CANFD)||
#endif
				(uiPID == ISO14230_ES95486_DOIP_120)||
				(uiPID == ISO14230_ES95486_DOIP_121)||
				(uiPID == ISO14230_ES95486_DOIP_122)||
				(uiPID == ISO14230_ES95486_DOIP_123)||
				(uiPID == ISO14230_ES95486_DOIP_124)||
				(uiPID == ISO14230_ES95486_DOIP_125)||
				(uiPID == ISO14230_ES95486_DOIP_126)||
				(uiPID == ISO14230_ES95486_DOIP_127)||
				(uiPID == ISO14230_ES95486_DOIP_128)||
				(uiPID == ISO14230_ES95486_DOIP_129)||
				(uiPID == ISO14230_ES95486_DOIP_12A)||
				(uiPID == ISO14230_ES95486_DOIP_12B)||
				(uiPID == ISO14230_ES95486_DOIP_12C)||
				(uiPID == ISO14230_ES95486_DOIP_12D)||
				(uiPID == ISO14230_ES95486_DOIP_12E)||
				(uiPID == ISO14230_ES95486_DOIP_12F)||
				(uiPID == ISO14229_ES95486_170)||
				(uiPID == ISO14229_ES95486_170_MMCAN)||
				(uiPID == ISO14230_ES95486_170)||
				(uiPID == ISO14230_ES95486_170_MMCAN)||
                (uiPID == ISO15765_CAN_HWSET_DB_SINGLE)||
                (uiPID == ISO15765_CAN_HWSET_DB_NEW)||
                (uiPID == ISO14229_UDS_HWSET_DB)
#ifdef NEW_29BIT_CAN
                  ||(uiPID == ISO15765_ES95486_29bit)
                  ||(uiPID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
                || (uiPID ==	ISO15765_ES95486_135_29bit_CANFD)
                || (uiPID ==	ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
                  )
			{
#ifdef CANFD_PROTOCOL
                if( (uiPID == ISO14229_ES95486_02_130_CANFD)||
                    (uiPID == ISO14229_ES95486_02_131_CANFD) )
                {
                    g_ucCanformat = CAN_FRAMEFORMAT_FDCAN;
                }
#endif
				nCANBaudrate = g_stGITSetConfig.nDataRate;
			}
			else if ( uiPID == ISO15765_CARB_29BIT 			|| 
						uiPID == ISO15765_CARB_29BIT_NEW 	|| 
                        uiPID == J1939_23_CARB_29BIT_NEW  ||
						uiPID == ISO15765_29BIT 						|| 
						uiPID == ISO15765_29BIT_EXCEPT		||
						uiPID ==	J1939									||	
						uiPID ==	J1939_4PGN							
#ifdef NEW_29BIT_CAN
                  //||(uiPID == ISO15765_ES95486_29bit)
                  //||(uiPID == ISO15765_ES95486_DOIP_29BIT)
#endif
                    )
			{
				nCANBaudrate = g_stGITSetConfig.nDataRate;
				g_stGITHWSetData.nCommRelay = Highcan1;
			}
			else
			{
				nCANBaudrate = CAN_500KBPS;
				g_stGITHWSetData.nCommRelay = Highcan1;
			}
			//g_ucCAN_CH=2;//kkt. internal can contriller test

#ifdef CANFD_qhyek //Q_hyek CANFD
            
			CANformat=g_ucCanformat;	// 0:FDCAN, 1:CAN
			CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, CANformat, 0,	nCanType, MaskNum, &nStartMask[0], &nEndMask[0]);
#else
			CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, 1, 0,	nCanType, MaskNum, &nStartMask[0], &nEndMask[0]);
#endif

//#if defined(DEBUG_GIT_VCI_LOG)
            GLogI("Mask1 : ");
			 GLogI("[%s] CANBaudrate %d, CanType %d, CanChannel %d, StartMask 0x%X, EndMask 0x%X, CommRelay %d\r\n",
			 	__FUNCTION__, nCANBaudrate, nCanType, g_ucCAN_CH, nStartMask[0], nEndMask[0], g_stGITHWSetData.nCommRelay);
             if(MaskNum == 2)
             {
               GLogI("Mask2 : ");
               GLogI("[%s] CANBaudrate %d, CanType %d, CanChannel %d, StartMask 0x%X, EndMask 0x%X, CommRelay %d\r\n",
			 	__FUNCTION__, nCANBaudrate, nCanType, g_ucCAN_CH, nStartMask[1], nEndMask[1], g_stGITHWSetData.nCommRelay);
             }
//#endif
			 break;

#ifdef USE_RELAY_MOSA
		case BAT_RELAY_CON:
		case BAT_FD_RELAY_CON:
			if( uiPID == BAT_RELAY_CON )
			{
				nStartMask[0] = 0x0200;
				nEndMask[0] = 0x05FF;
				MaskNum = 1;

				nCANBaudrate = CAN_500KBPS;
			}
			else if( uiPID == BAT_FD_RELAY_CON )
			{	
				nStartMask[0] = 0x0030;
				nEndMask[0] = 0x05FF;
				nStartMask[1] = 0x0700;
				nEndMask[1] = 0x07FF;
				MaskNum = 2;

				nCANBaudrate = CAN_500KBPS;
			}

			nCanType = STANDARD_CAN;
			CANformat=g_ucCanformat;
			
			CAN_Initial_CH(g_ucCAN_CH, g_stGITHWSetData.nCommRelay, nCANBaudrate, CANformat, 0,	nCanType, MaskNum, &nStartMask[0], &nEndMask[0]);
			//CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_FDCAN );
			//startFDCan( 1, 0, 0 );
			
            GLogI("Mask1 : ");
			GLogI("[%s] CANBaudrate %d, CanType %d, CanChannel %d, StartMask 0x%X, EndMask 0x%X, CommRelay %d\r\n",
				__FUNCTION__, nCANBaudrate, nCanType, g_ucCAN_CH, nStartMask[0], nEndMask[0], g_stGITHWSetData.nCommRelay);

			if(MaskNum == 2)
			{
				GLogI("Mask2 : ");
				GLogI("[%s] CANBaudrate %d, CanType %d, CanChannel %d, StartMask 0x%X, EndMask 0x%X, CommRelay %d\r\n",
					__FUNCTION__, nCANBaudrate, nCanType, g_ucCAN_CH, nStartMask[1], nEndMask[1], g_stGITHWSetData.nCommRelay);
			}

			break;
#endif

		default:
			DLC_HW_Set(g_stGITHWSetData.nCommRelay,g_stGITHWSetData.nKlineSelect,g_stGITHWSetData.nKlineStatus,g_stGITHWSetData.nLlineSelect
				,g_stGITHWSetData.nLlineStatus,g_stGITHWSetData.nKlineSwitchStatus,g_stGITHWSetData.nLlineSwitchStatus,g_stGITHWSetData.nRxLineSelect
				,g_stGITHWSetData.nRxLineStatus,g_stGITHWSetData.nPullupRelay,g_stGITHWSetData.nKlinePullup,g_stGITHWSetData.nLlinePullup
				,g_stGITHWSetData.nLlineGnd,g_stGITHWSetData.nKlineCh,g_stGITHWSetData.nLlineCh,g_stGITHWSetData.nRProgramCh);
			 //uartSetBaudRate(UART_DLC,VDATA_RATE);
			 uartSetBaudRate(GetKlineSelect(), g_stGITSetConfig.nDataRate );

	}
	osDelay(g_stGITSetConfig.nTIdle);
}

void VCI_SetPassThruDeviceID(uint8_t *pData)
{
	memcpy(&g_ulDeviceID, pData, sizeof(g_ulDeviceID));
}

uint32_t VCI_GetPassThruDeviceID(void)
{
	return g_ulDeviceID;
}

void VCI_SetPassThruProtocolID(uint8_t *pData)
{
	memcpy(&g_ulProtocolID, pData, sizeof(g_ulProtocolID));
}

uint32_t VCI_GetPassThruProtocolID(void)
{
#if defined(DEBUG_GIT_VCI_LOG)
	GLogI("[%s] protocol ID 0x%0X\r\n", __FUNCTION__, g_ulProtocolID);
#endif
	return g_ulProtocolID;
}

void VCI_SetPassThruConnectFlags(uint8_t *pData)
{
	memcpy(&g_ulProtocolFlag, pData, sizeof(g_ulProtocolFlag));
}

uint32_t VCI_GetPassThruConnectFlags(void)
{
	return g_ulProtocolFlag;
}

void VCI_SetPassThruBaudrate(uint8_t *pData)
{
	memcpy(&g_stGITSetConfig.nDataRate, pData, sizeof(g_stGITSetConfig.nDataRate));

#if defined(DEBUG_GIT_VCI_LOG)
	GLogI("[%s] g_stGITSetConfig.nDataRate %d\r\n", __FUNCTION__, g_stGITSetConfig.nDataRate);
#endif
}

uint32_t VCI_GetPassThruBaudrate(void)
{
	return g_stGITSetConfig.nDataRate;
}

void VCI_SetPassThruWirteMsgTimeout(uint8_t *pData, uint32_t uiDataLen)
{
	uint32_t ulTimeout;
	memcpy(&ulTimeout, pData, uiDataLen); // VCI ???ÑÏßÅ Ï∞∏Ï°∞???ÑÏöî ?ÜÏùå.
}

u32 VCI_ProtocolClassification()
{
	// pPassThruMsg
	uint32_t uiProtocolID;

	uiProtocolID = VCI_GetPassThruProtocolID();

	if( (uiProtocolID == ISO15765) 					||
		(uiProtocolID == ISO15765_NEW) 				||
		(uiProtocolID == ISO14229_UDS) 				||
		(uiProtocolID == ISO14229) 					||
		(uiProtocolID == ISO15765_CUBIS)			||
		(uiProtocolID == ISO15765_SINGLE)			||
		(uiProtocolID == ISO15765_SINGLE_SMK)		||
		(uiProtocolID == ISO15765_SMK)				||
		(uiProtocolID == ISO15765_SINGLE_PODS)		||
		(uiProtocolID == ISO15765_ACU_SINGLE)		||
		(uiProtocolID == ISO15765_CARB) 			||
		(uiProtocolID == ISO15765_CARB_NEW) 		||
		(uiProtocolID == ISO15765_CARB_NEW_LENGTH) 	||
        (uiProtocolID == J1939_23_CARB_NEW_LENGTH) ||
		(uiProtocolID == ISO15765_CARB_29BIT) 		||
		(uiProtocolID == ISO15765_CARB_29BIT_NEW) 	||
        (uiProtocolID == J1939_23_CARB_29BIT_NEW)  ||
		(uiProtocolID == ISO15765_REPRO_PENNIMG) 	||
		(uiProtocolID == ISO15765_REPRO_PENNIMG_TIME)||
		(uiProtocolID == ISO15765_NORMAL)			||
		(uiProtocolID == ISO15765_CAN_HWSET_DB)		|| 
		(uiProtocolID == ISO14229_UDS_HWSET_DB)		||  
		(uiProtocolID == ISO15765_CAN_HWSET_DB_SINGLE)||
		(uiProtocolID == J1939)								||
		(uiProtocolID == J1939_4PGN)						||
		(uiProtocolID == ISO15765_EXCEPT) 			||
		(uiProtocolID == ISO15765_29BIT ) 			||
		(uiProtocolID == ISO15765_29BIT_EXCEPT)		||
		(uiProtocolID == ISO15765_CARB_29BIT ) 		||
		(uiProtocolID == ISO14229_ES95486_02_100)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_102)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_103)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_104)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_105)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_106)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_107)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_108)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_109)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_10A)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_10B)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_10C)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_10D)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_10E)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_10F)||				//20190918 Jay
		(uiProtocolID == ISO14229_ES95486_02_100_NEW)||			//20190918 Jay
#ifdef HOTA
        (uiProtocolID == ISO14229_ES95486_02_HOTA)||
#endif
#ifdef CANFD_PROTOCOL
        (uiProtocolID == ISO14229_ES95486_02_130_CANFD)||
        (uiProtocolID == ISO14229_ES95486_02_131_CANFD)||
#endif
		(uiProtocolID == ISO14230_ES95486_DOIP_120)||
		(uiProtocolID == ISO14230_ES95486_DOIP_121)||
		(uiProtocolID == ISO14230_ES95486_DOIP_122)||
		(uiProtocolID == ISO14230_ES95486_DOIP_123)||
		(uiProtocolID == ISO14230_ES95486_DOIP_124)||
		(uiProtocolID == ISO14230_ES95486_DOIP_125)||
		(uiProtocolID == ISO14230_ES95486_DOIP_126)||
		(uiProtocolID == ISO14230_ES95486_DOIP_127)||
		(uiProtocolID == ISO14230_ES95486_DOIP_128)||
		(uiProtocolID == ISO14230_ES95486_DOIP_129)||
		(uiProtocolID == ISO14230_ES95486_DOIP_12A)||
		(uiProtocolID == ISO14230_ES95486_DOIP_12B)||
		(uiProtocolID == ISO14230_ES95486_DOIP_12C)||
		(uiProtocolID == ISO14230_ES95486_DOIP_12D)||
		(uiProtocolID == ISO14230_ES95486_DOIP_12E)||
		(uiProtocolID == ISO14230_ES95486_DOIP_12F)||
		(uiProtocolID == ISO14229_ES95486_170)||
		(uiProtocolID == ISO14229_ES95486_170_MMCAN)||
		(uiProtocolID == ISO14230_ES95486_170)||
		(uiProtocolID == ISO14230_ES95486_170_MMCAN)
#ifdef NEW_29BIT_CAN
          ||(uiProtocolID == ISO15765_ES95486_29bit)
          ||(uiProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
          ||(uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
          ||(uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
            
#endif
#endif
            )
	{
		if ((uiProtocolID == ISO15765_29BIT )					||
			(uiProtocolID == ISO15765_29BIT_EXCEPT)	||
			(uiProtocolID == ISO15765_CARB_29BIT ) 		||
			(uiProtocolID == ISO15765_CARB_29BIT) 		||
			(uiProtocolID == ISO15765_CARB_29BIT_NEW)||
            (uiProtocolID == J1939_23_CARB_29BIT_NEW)  ||
			(uiProtocolID == J1939)								||
			(uiProtocolID == J1939_4PGN)						
#ifdef NEW_29BIT_CAN
          ||(uiProtocolID == ISO15765_ES95486_29bit)
          ||(uiProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
          ||(uiProtocolID == ISO15765_ES95486_135_29bit_CANFD)
          ||(uiProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
#endif
          )
		{
			g_bExtCanFlag=true;
		}
		else
		{
			g_bExtCanFlag=false;
		}
		return CAN_SavePassThruWriteMsg( );
	}
	else if ( uiProtocolID == ISO14230								||
			 	uiProtocolID == ISO14230_ETC						||
				uiProtocolID == ISO9141								||
				uiProtocolID == ISO9141_2  							||
				uiProtocolID == IS09141_2_ALLISON_ATM  	||
				uiProtocolID == ISO9141_BOSCH 					||
				uiProtocolID == IMMO_SINCHANG 				||
				uiProtocolID == WABCO_ABS						||
                uiProtocolID == ISO14230_POWERTEC   )
	{
		//DLC_SavePassThruWriteMsg(pData, uiDataLen);
		return KLINE_SavePassThruWriteMsg();
	}
#ifdef USE_RELAY_MOSA
	else if( uiProtocolID == BAT_RELAY_CON ||
				uiProtocolID == BAT_FD_RELAY_CON )
	{
		CAN_SavePassThruWriteMsg( );
	}
#endif
	else
	{
		GLogE("[%s] not support Protocol ID 0x%X \r\n", __FUNCTION__, uiProtocolID);
		return 0;	//config???ÑÎ°ú?†ÏΩú??can???ÑÎãåÍ≤ΩÏö∞ ?§Ïùå ?úÌóò?ºÎ°ú ?òÏñ¥Í∞Ä???úÎã§
	}
}

void DLC_HW_Set(U32 CommRelay,U32 KlineSelect,U32 KlineStatus,U32 LlineSelect,U32 LlineStatus,
                U32 KlineSwitchStatus,U32 LlineSwitchStatus,U32 RxLineSelect,U32 RxLineStatus,
                U32 PullupRelay,U32 KlinePullup,U32 LlinePullup,U32 LineGnd,U32 KlineChSet,
                U32 LlineChSet,U32 ReproChSet)
{
	//K_Line_Buf = KlineChSet;
	//L_Line_Buf = LlineChSet;
    Enable_KL_Interrupt( KL_LINE1 );
	Enable_KL_Interrupt( KL_LINE2 );
    switch(CommRelay)
    {
#if 0
        case J1708_RELAY:
          
            J1708_EN_HI();
            
            SIG_CAN_EN_LOW();
            LOW_CAN_EN_LOW();
            KL_DN2_EN_LOW();
            KL_DN1_EN_LOW(); 
            DO_CAN2_LOW();
            DO_CAN1_LOW();
            HI_CAN_EN_LOW();
            
            if(RxLineSelect == RXD_485)
            {
                if(RxLineStatus == RXD_NORMAL)
                {
                    J1708_SEL_LO();
                }
                else
                {
                    J1708_SEL_HI();
                }
            }
            
            GIT_DLC_CH_Set(KlineChSet,LlineChSet,ReproChSet);
            
        break;
#endif
        case KLLINE_RELAY:
            if(KlineStatus == K_INVERSE)
            {
                if(KlineChSet == K_ChOff 
					|| KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13)   KL_TXD1_INV_ENABLE;//KL_TXD1_INV_LOW(); 
                else    KL_TXD2_INV_ENABLE;//KL_TXD2_INV_LOW(); 
            }
            else
            {
                if(KlineChSet == K_ChOff 
					|| KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13)   KL_TXD1_INV_DISABLE;//KL_TXD1_INV_HIGH();
                else    KL_TXD1_INV_DISABLE;//KL_TXD2_INV_HIGH();
            }
            if(LlineStatus == L_INVERSE)
            {
                if(LlineChSet == KL_LINE1_CONNECT_CH01 
					|| LlineChSet == KL_LINE1_CONNECT_CH02 
					|| LlineChSet == KL_LINE1_CONNECT_CH03 
					|| LlineChSet == KL_LINE1_CONNECT_CH06 
					|| LlineChSet == KL_LINE1_CONNECT_CH07 
					|| LlineChSet == KL_LINE1_CONNECT_CH13)   KL_TXD1_INV_ENABLE;//KL_TXD1_INV_LOW();
                else if(KlineChSet == KL_LINE1_CONNECT_CH07 
						|| KlineChSet == KL_LINE1_CONNECT_CH15) KL_TXD1_INV_ENABLE;//KL_TXD1_INV_LOW();
                else    KL_TXD2_INV_ENABLE;//KL_TXD2_INV_LOW();                  
            }
            else
            {
                if(LlineChSet == KL_LINE1_CONNECT_CH01 
					|| LlineChSet == KL_LINE1_CONNECT_CH02 
					|| LlineChSet == KL_LINE1_CONNECT_CH03 
					|| LlineChSet == KL_LINE1_CONNECT_CH06 
					|| LlineChSet == KL_LINE1_CONNECT_CH07 
					|| LlineChSet == KL_LINE1_CONNECT_CH13)   KL_TXD1_INV_DISABLE;//KL_TXD1_INV_HIGH();
                else if(KlineChSet == KL_LINE1_CONNECT_CH07 
						|| KlineChSet == KL_LINE1_CONNECT_CH15) KL_TXD1_INV_DISABLE;//KL_TXD1_INV_HIGH();
                else    KL_TXD2_INV_DISABLE;//KL_TXD2_INV_HIGH();
            }
            if(RxLineSelect == RXD_OBD1)
            {
                if(KlineChSet == K_ChOff 
					|| KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13) KL_RXD1_REF_BATTERY;//KL_RXD1_SEL_HIGH();
                else     KL_RXD2_REF_BATTERY;//KL_RXD2_SEL_HIGH(); 
            }
            else if(RxLineSelect == RXD_OBD2)
            {
                if(KlineChSet == K_ChOff 
					|| KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13) KL_RXD1_REF_5V;//KL_RXD1_SEL_LOW();
                else     KL_RXD2_REF_5V;//KL_RXD2_SEL_LOW(); 
            }
                    
            if(RxLineStatus == RXD_NORMAL)
            {
                if(KlineChSet == K_ChOff 
					|| KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13) KL_RXD1_INV_DISABLE;//KL_RXD1_INV_LOW();
                else     KL_RXD2_INV_DISABLE;//KL_RXD2_INV_LOW(); 
            }
            else
            {  
                if(KlineChSet == K_ChOff 
					|| KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13) KL_RXD1_INV_ENABLE;//KL_RXD1_INV_HIGH();
                else     KL_RXD2_INV_ENABLE;//KL_RXD2_INV_HIGH(); 
            }
                      
            if(PullupRelay == Pulldown)
            {
                //if(KlineChSet == K_ChOff || KlineChSet == KL_LINE1_CONNECT_CH01 || KlineChSet == KL_LINE1_CONNECT_CH02 || KlineChSet == KL_LINE1_CONNECT_CH03 || KlineChSet == KL_LINE1_CONNECT_CH06 || KlineChSet == KL_LINE1_CONNECT_CH07 || KlineChSet == KL_LINE1_CONNECT_CH13) KL_DN1_EN_HIGH();
                //else     KL_DN2_EN_HIGH();
                
                if(KlineChSet == K_ChOff 
					|| KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13)   KL_TXD1_INV_DISABLE;//KL_TXD1_INV_HIGH();
                else    KL_TXD2_INV_DISABLE;//KL_TXD2_INV_HIGH();
            }
            else
            {
                //KL_DN1_EN_LOW();
                //KL_DN2_EN_LOW();                
            }
                    
            if(KlinePullup == K_Pullup_510)
            {
                if(KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13)   KL_LINE1_CONNECT_510;//TXD1_510EN_HI();
                else    KL_LINE2_CONNECT_510;//TXD2_510EN_HI();
            }
            else if(KlinePullup == K_Pullup_47k)
            {
                if(KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13)   KL_LINE1_CONNECT_47K;//TXD1_47KEN_HI();
                else    KL_LINE2_CONNECT_47K;//TXD2_47KEN_HI();
            }
            else if(KlinePullup == K_Pullup_2k)
            {
                if(KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13)   KL_LINE1_CONNECT_2K;//TXD1_2KEN_HI();
                else    KL_LINE2_CONNECT_2K;//TXD2_2KEN_HI();
            }
            else
            {
                if(KlineChSet == KL_LINE1_CONNECT_CH01 
					|| KlineChSet == KL_LINE1_CONNECT_CH02 
					|| KlineChSet == KL_LINE1_CONNECT_CH03 
					|| KlineChSet == KL_LINE1_CONNECT_CH06 
					|| KlineChSet == KL_LINE1_CONNECT_CH07 
					|| KlineChSet == KL_LINE1_CONNECT_CH13)
                {   
			KL_LINE1_DISCONNECT_PULLUP;

                    //TXD1_510EN_LO();
                    //TXD1_47KEN_LO();
                    //TXD1_2KEN_LO();
                }
                else
                {  
			KL_LINE2_DISCONNECT_PULLUP;

                    //TXD2_510EN_LO();
                    //TXD2_47KEN_LO();
                    //TXD2_2KEN_LO();
                }
            }
                    
            if(LlinePullup == L_Pullup_510)
            {
                if(LlineChSet == KL_LINE1_CONNECT_CH01 
					|| LlineChSet == KL_LINE1_CONNECT_CH02 
					|| LlineChSet == KL_LINE1_CONNECT_CH03 
					|| LlineChSet == KL_LINE1_CONNECT_CH06 
					|| LlineChSet == KL_LINE1_CONNECT_CH07 
					|| LlineChSet == KL_LINE1_CONNECT_CH13)   KL_LINE1_CONNECT_510;//TXD1_510EN_HI();
                else if(KlineChSet == KL_LINE1_CONNECT_CH07 
						|| KlineChSet == KL_LINE1_CONNECT_CH15) KL_LINE1_CONNECT_510;//TXD1_510EN_HI();
                else    KL_LINE2_CONNECT_510;//TXD2_510EN_HI(); 
            }
            else if(LlinePullup == L_Pullup_47k)
            {
                if(LlineChSet == KL_LINE1_CONNECT_CH01 
					|| LlineChSet == KL_LINE1_CONNECT_CH02 
					|| LlineChSet == KL_LINE1_CONNECT_CH03 
					|| LlineChSet == KL_LINE1_CONNECT_CH06 
					|| LlineChSet == KL_LINE1_CONNECT_CH07 
					|| LlineChSet == KL_LINE1_CONNECT_CH13)   KL_LINE1_CONNECT_47K;//TXD1_47KEN_HI();
                else if(KlineChSet == KL_LINE1_CONNECT_CH07 
						|| KlineChSet == KL_LINE1_CONNECT_CH15) KL_LINE1_CONNECT_47K;//TXD1_47KEN_HI();
                else    KL_LINE2_CONNECT_47K;//TXD2_47KEN_HI(); 
            }
            else if(LlinePullup == L_Pullup_2k)
            {
                if(LlineChSet == KL_LINE1_CONNECT_CH01 
					|| LlineChSet == KL_LINE1_CONNECT_CH02 
					|| LlineChSet == KL_LINE1_CONNECT_CH03 
					|| LlineChSet == KL_LINE1_CONNECT_CH06 
					|| LlineChSet == KL_LINE1_CONNECT_CH07 
					|| LlineChSet == KL_LINE1_CONNECT_CH13)   KL_LINE1_CONNECT_2K;//TXD1_2KEN_HI();
                else if(KlineChSet == KL_LINE1_CONNECT_CH07 || KlineChSet == KL_LINE1_CONNECT_CH15) KL_LINE1_CONNECT_2K;//TXD1_2KEN_HI();
                else    KL_LINE2_CONNECT_2K;//TXD2_2KEN_HI(); 
            }
            else
            {
                if(LlineChSet == KL_LINE1_CONNECT_CH01 
					|| LlineChSet == KL_LINE1_CONNECT_CH02 
					|| LlineChSet == KL_LINE1_CONNECT_CH03 
					|| LlineChSet == KL_LINE1_CONNECT_CH06 
					|| LlineChSet == KL_LINE1_CONNECT_CH07 
					|| LlineChSet == KL_LINE1_CONNECT_CH13)
                {
			KL_LINE1_DISCONNECT_PULLUP;

                    //TXD1_510EN_LO();
                    //TXD1_47KEN_LO();
                    //TXD1_2KEN_LO();
                }
                else if(KlineChSet == KL_LINE1_CONNECT_CH07 
						|| KlineChSet == KL_LINE1_CONNECT_CH15)
                {
			KL_LINE1_DISCONNECT_PULLUP;

                    //TXD1_510EN_LO();
                    //TXD1_47KEN_LO();
                    //TXD1_2KEN_LO();
                }
                else
                {
			KL_LINE2_DISCONNECT_PULLUP;

                    //TXD2_510EN_LO();
                    //TXD2_47KEN_LO();
                    //TXD2_2KEN_LO();
                }
            }
            DLC_CH_Set(KlineChSet,LlineChSet);
			//SetKL_Line( KL_LINE1_CONNECT_CH06, KL_LINE2_CONNECT_CH14 );
			//SetReprogramVol( REPG_LINE_CH03 );
			//SetKL_Line( KlineChSet, LlineChSet );
			//SetReprogramVol( ReproChSet );
        break;
    }         
}

void DLC_CH_Set(U32 KlineChSet,U32 LlineChSet)
{
	// [group 1] : 1 2 3 6 7 8_1 13 15_1
	// [group 2] : 8 9 10 11 12 14 15
	
	// All Line Clear
	IO_CONTROL_LOW( DLCA_EN1 );
	IO_CONTROL_LOW( DLCA_EN2 );
	IO_CONTROL_LOW( DLCA_EN3 );
	IO_CONTROL_LOW( DLCA_EN6 );
	IO_CONTROL_LOW( DLCA_EN7 );
	IO_CONTROL_LOW( DLCA_EN8_1 );
	IO_CONTROL_LOW( DLCA_EN13 );
	IO_CONTROL_LOW( DLCA_EN15_1 );

	IO_CONTROL_LOW( DLCB_EN8 );
	IO_CONTROL_LOW( DLCB_EN9 );
	IO_CONTROL_LOW( DLCB_EN10 );
	IO_CONTROL_LOW( DLCB_EN11 );
	IO_CONTROL_LOW( DLCB_EN12 );
	IO_CONTROL_LOW( DLCB_EN14 );
	IO_CONTROL_LOW( DLCB_EN15 );
	
    if(KlineChSet == KL_LINE1_CONNECT_CH01 || LlineChSet == KL_LINE1_CONNECT_CH01)    IO_CONTROL_HIGH( DLCA_EN1 );//DLC_EN1_HI();
    if(KlineChSet == KL_LINE1_CONNECT_CH02 || LlineChSet == KL_LINE1_CONNECT_CH02)    IO_CONTROL_HIGH( DLCA_EN2 );//DLC_EN2_HI();
    if(KlineChSet == KL_LINE1_CONNECT_CH03 || LlineChSet == KL_LINE1_CONNECT_CH03)    IO_CONTROL_HIGH( DLCA_EN3 );//DLC_EN3_HI();
    if(KlineChSet == KL_LINE1_CONNECT_CH06 || LlineChSet == KL_LINE1_CONNECT_CH06)    IO_CONTROL_HIGH( DLCA_EN6 );//DLC_EN6_HI();
    if(KlineChSet == KL_LINE1_CONNECT_CH07 || LlineChSet == KL_LINE1_CONNECT_CH07)    IO_CONTROL_HIGH( DLCA_EN7 );//DLC_EN7_HI();
    if(KlineChSet == KL_LINE1_CONNECT_CH08)    IO_CONTROL_HIGH( DLCB_EN8 );//DLC_EN8_HI();
    if(KlineChSet == KL_LINE2_CONNECT_CH09 || LlineChSet == KL_LINE2_CONNECT_CH09)    IO_CONTROL_HIGH( DLCB_EN9 );//DLC_EN9_HI();
    if(KlineChSet == KL_LINE2_CONNECT_CH10 || LlineChSet == KL_LINE2_CONNECT_CH10)    IO_CONTROL_HIGH( DLCB_EN10 );//DLC_EN10_HI();
    if(KlineChSet == KL_LINE2_CONNECT_CH11 || LlineChSet == KL_LINE2_CONNECT_CH11)    IO_CONTROL_HIGH( DLCB_EN11 );//DLC_EN11_HI();
    if(KlineChSet == KL_LINE2_CONNECT_CH12 || LlineChSet == KL_LINE2_CONNECT_CH12)    IO_CONTROL_HIGH( DLCB_EN12 );//DLC_EN12_HI();
    if(KlineChSet == KL_LINE1_CONNECT_CH13 || LlineChSet == KL_LINE1_CONNECT_CH13)    IO_CONTROL_HIGH( DLCA_EN13 );//DLC_EN13_HI();
    if(KlineChSet == KL_LINE2_CONNECT_CH14 || LlineChSet == KL_LINE2_CONNECT_CH14)    IO_CONTROL_HIGH( DLCB_EN14 );//DLC_EN14_HI();
    if(KlineChSet == KL_LINE1_CONNECT_CH15)    IO_CONTROL_HIGH( DLCB_EN15 );//DLC_EN15_HI();
	
    if(LlineChSet == KL_LINE1_CONNECT_CH08 
		&& (KlineChSet == KL_LINE1_CONNECT_CH08 
			|| KlineChSet == KL_LINE2_CONNECT_CH09 
			|| KlineChSet == KL_LINE2_CONNECT_CH10 
			|| KlineChSet == KL_LINE2_CONNECT_CH11 
			|| KlineChSet == KL_LINE2_CONNECT_CH12 
			|| KlineChSet == KL_LINE2_CONNECT_CH14 
			|| KlineChSet == KL_LINE1_CONNECT_CH15))    IO_CONTROL_HIGH( DLCA_EN8_1 );//DLC_EN8_1_HI();
    else if(LlineChSet == KL_LINE1_CONNECT_CH08)   IO_CONTROL_HIGH( DLCB_EN8 );//DLC_EN8_HI();
    if(LlineChSet == KL_LINE1_CONNECT_CH15 
		&& (KlineChSet == KL_LINE1_CONNECT_CH08 
		|| KlineChSet == KL_LINE2_CONNECT_CH09 
		|| KlineChSet == KL_LINE2_CONNECT_CH10 
		|| KlineChSet == KL_LINE2_CONNECT_CH11 
		|| KlineChSet == KL_LINE2_CONNECT_CH12 
		|| KlineChSet == KL_LINE2_CONNECT_CH14 
		|| KlineChSet == KL_LINE1_CONNECT_CH15))    IO_CONTROL_HIGH( DLCA_EN15_1 );//DLC_EN15_1_HI();
    else if(LlineChSet == KL_LINE1_CONNECT_CH15) IO_CONTROL_HIGH( DLCB_EN15 );//DLC_EN15_HI();
	
	//SetKL_Line( KlineChSet, LlineChSet );
}
void VCI_FastInit(uint8_t* pData, uint32_t eInCommType)
//void VCI_FastInit(void)
{
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;
	uint32_t uiProtocolID;

	uiProtocolID = VCI_GetPassThruProtocolID();
	
#if 0
	IsFastInit = TRUE;
	
	//uartPrintf("\nFast Init : VTINIL->%d VTWUP-VTINIL->%d \n", VTINIL,(VTWUP-VTINIL));
	
	DLC_RX_BUFF_CLEAR();
	
	if(ProtocolType==ISO14230_POWERTEC) 	
	{
		KW_fast_init_Powertec(VTINIL,(VTWUP-VTINIL));	// Powertec Protocol by kyc 2007.06.19
		Delay(800);
		KW_fast_init_Powertec(VTINIL,(VTWUP-VTINIL));	// Powertec Protocol by kyc 2007.06.19
	}
	else if(ProtocolType == NISSAN_TxRx )
	{
		
		//uartPrintf("\nFast Init : Nissan");
	}	
	else
#endif
	if(uiProtocolID == ISO14230_POWERTEC)	
	{
		KW_fast_init_Powertec(g_stGITSetConfig.nTInil,(g_stGITSetConfig.nTWUp-g_stGITSetConfig.nTInil));	// Powertec Protocol by kyc 2007.06.19
		//osDelay(800);
		//KW_fast_init_Powertec(g_stGITSetConfig.nTInil,(g_stGITSetConfig.nTWUp-g_stGITSetConfig.nTInil));	// Powertec Protocol by kyc 2007.06.19
	} 
	else
	{
		KW_fast_init(g_stGITSetConfig.nTInil,(g_stGITSetConfig.nTWUp-g_stGITSetConfig.nTInil));
	}
	//uartPrintf("\nFast Init OK\n");
	//kkt DLC_RX_BUFF_CLEAR();
	//kkt CurMode = NO_OPP;
	/*
	while(1)
	{
		VCI_COMMUNICATION();
		if( CurMode == NO_OPP ) break;
	}
	*/
	
	//memcpy(&g_stGITSByteArray.NumOfBytes, pData, sizeof(g_stGITSByteArray.NumOfBytes));
	
	//kkt DlcRxBlock = 0;
	//kkt DlcTxBlock = 0;
	pDiagmsg = ( MsgDiag_t* )osPoolCAlloc( hDiagPool );
	if( pDiagmsg == NULL )
	{
		return;
	}
	pPTpacket = ( PTmsgPkt_t* )osPoolCAlloc( hPTPKPool );
	if( pPTpacket == NULL )
	{
		osPoolFree( hDiagPool, (void *)pDiagmsg );
		return;
	}
	memcpy(pPTpacket, pData, sizeof(PTmsgPkt_t));
	pDiagmsg->mMsgType 		= MSG_DIAG;
	pDiagmsg->mPktType 		= (ePKT_TD)eInCommType;
	pDiagmsg->event 		= DIAG_PASSTHRU;
	pDiagmsg->subEvent 		= eDIAG_COMM_TX_START;
	pDiagmsg->unEventTime 	= GetUnixTime();
	pDiagmsg->pPacket 		= (void *)pPTpacket;
	
	if(osMessageAvailableSpace(hDiagMsg) == 0)
	{
		osPoolFree( hPTPKPool, (void *)pPTpacket );
		osPoolFree( hDiagPool, (void *)pDiagmsg );
	}
	else
	{
		osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
	}
	//uartPrintf("\n Start Comm. OK\n");

	//kkt IsFastInit = FALSE;
//	J2534PassThruRxMsgNum = 0;

}
void KW_fast_init (uint32_t FastTime,uint32_t FastTime1)
{
	// [group 1] : 1 2 3 6 7 8_1 13 15_1
	// [group 2] : 8 9 10 11 12 14 15
	//HAL_Delay(1);
    if(GetKlineSelect()==KL_LINE1)   KL_TXD1_INV_ENABLE;//KL_TXD1_INV_LOW();
    else    KL_TXD2_INV_ENABLE;//KL_TXD2_INV_LOW();    
    //HAL_Delay(FastTime);	//use system tick -> The first tick is less than 1ms
    //osDelay(FastTime);	
    CAN_uDelay(FastTime*1000);	//use system clock
      
    if(GetKlineSelect()==KL_LINE1)   KL_TXD1_INV_DISABLE;//KL_TXD1_INV_HIGH();
    else    KL_TXD2_INV_DISABLE;//KL_TXD2_INV_HIGH();
    //HAL_Delay(FastTime1);
    //osDelay(FastTime1);
	CAN_uDelay(FastTime1*1000);
}

void KW_fast_init_Powertec (uint32_t FastTime, uint32_t FastTime1)
{
	// [group 1] : 1 2 3 6 7 8_1 13 15_1
	// [group 2] : 8 9 10 11 12 14 15
	
/*
	if(g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH01 
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH02 
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH03 
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH06 
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH07
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH13
		|| g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH08
		|| g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH15)   KL_TXD1_INV_ENABLE;//KL_TXD1_INV_LOW();
    else    KL_TXD2_INV_ENABLE;//KL_TXD2_INV_LOW();
	
	osDelay(10);
	
    if(GetKlineSelect()==KL_LINE1)   KL_TXD1_INV_ENABLE;//KL_TXD1_INV_LOW();
    else    KL_TXD2_INV_ENABLE;//KL_TXD2_INV_LOW();   
	
    osDelay(FastTime);
	
	if(g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH01 
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH02 
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH03 
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH06 
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH07
		|| g_stGITHWSetData.nLlineCh == KL_LINE1_CONNECT_CH13
		|| g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH08
		|| g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH15)   KL_TXD1_INV_DISABLE;//KL_TXD1_INV_HIGH();
    else    KL_TXD2_INV_DISABLE;//KL_TXD2_INV_HIGH();
	
	osDelay(10);
      
    if(GetKlineSelect()==KL_LINE1)   KL_TXD1_INV_DISABLE;//KL_TXD1_INV_HIGH();
    else    KL_TXD2_INV_DISABLE;//KL_TXD2_INV_HIGH();

    osDelay(FastTime1);
*/
  
    IO_CONTROL_LOW( DLCA_EN1 );
	IO_CONTROL_LOW( DLCA_EN2 );
	IO_CONTROL_LOW( DLCA_EN3 );
	IO_CONTROL_LOW( DLCA_EN6 );
	IO_CONTROL_LOW( DLCA_EN7 );
	IO_CONTROL_LOW( DLCA_EN8_1 );
	IO_CONTROL_LOW( DLCA_EN13 );
	IO_CONTROL_LOW( DLCA_EN15_1 );

	IO_CONTROL_LOW( DLCB_EN8 );
	IO_CONTROL_LOW( DLCB_EN9 );
	IO_CONTROL_LOW( DLCB_EN10 );
	IO_CONTROL_LOW( DLCB_EN11 );
	IO_CONTROL_LOW( DLCB_EN12 );
	IO_CONTROL_LOW( DLCB_EN14 );
	IO_CONTROL_LOW( DLCB_EN15 );
    
    IO_CONTROL_HIGH(TXD2_510_EN);
    IO_CONTROL_HIGH(TXD1_510_EN);
    IO_CONTROL_LOW(TXD1_47K_EN);
    IO_CONTROL_LOW(TXD2_47K_EN);
    IO_CONTROL_LOW(TXD1_2K_EN);
    IO_CONTROL_LOW(TXD2_2K_EN);
    
    IO_CONTROL_HIGH(DLCA_EN7); 
    IO_CONTROL_HIGH(DLCB_EN15); 


    KL_TXD1_INV_DISABLE;
    KL_TXD2_INV_DISABLE;

   osDelay(400);  
   KL_TXD1_INV_ENABLE;
   CAN_uDelay(9);
   KL_TXD2_INV_ENABLE;
   osDelay(FastTime); 
    KL_TXD1_INV_DISABLE;
   CAN_uDelay(9);
    KL_TXD2_INV_DISABLE;
   
   osDelay(800); 
   
   KL_TXD1_INV_ENABLE;
   CAN_uDelay(9);
   KL_TXD2_INV_ENABLE;
   osDelay(FastTime); 
    KL_TXD1_INV_DISABLE;
   CAN_uDelay(9);
    KL_TXD2_INV_DISABLE;
    
    osDelay(FastTime1); 
}

void Get_AUTOVINData(U8 ucFlag)	//flag 0x01 : only can, kwp  0x02 : all	0x03 : clear&all	0x03 : clear&29bit CAN only
{
	U8 CAN_RET=0;
	U8 cnt;
	
	g_ucAUTOVIN[0]=0;

    if(ucFlag==4)    //ªÛøÎ ∏ﬁ¿Ãƒø º±≈√ Ω√
    {
        CheckVIN_CAN_29bit();    return;
    }
#ifdef CV_AUTOVIN
    else if (ucFlag == 6) // CV AutoVIN (No SA required)
    {
        U8 (*checkFuncs[])(void) = 
        {
            CheckCV_VIN_CAN,
            CheckCV_VIN_ExtCAN,
            CheckCV_VIN_PubEngine,
            CheckCV_VIN_FGEngine,
            CheckCV_VIN_HLEngine
        };

        for (int i = 0; i < sizeof(checkFuncs) / sizeof(checkFuncs[0]); i++) 
        {
            if (checkFuncs[i]()) 
            {
                return;
            }
        }
    }
    else if (ucFlag == 7) // CV AutoVIN (SA Required)
    { 
        U8 (*checkFuncs[])(U8) = 
        {
            CheckCV_VIN_VCU, 
            CheckCV_VIN_DTG
        };
        int channels[] = { CAN_CHANNEL_1, CAN_CHANNEL_2 };

        for (int i = 0; i < sizeof(checkFuncs) / sizeof(checkFuncs[0]); i++) 
        {
            for (int j = 0; j < sizeof(channels) / sizeof(channels[0]); j++)
            {
                if (checkFuncs[i](channels[j])) 
                {
                    return;
                }
            }
        }
    }
#endif
    else
    {
	  	//Delay(70);
	  	CAN_RET = CheckVIN_EV_NEW();	//210428 JayHong
        //GLogN("\n\rCheckVIN_EV_NEW() : %d\n\r", CAN_RET);
        if (CAN_RET == 1)	return;
		
		if (CAN_RET == 0)
        {
			for(cnt=0;cnt<3;cnt++)
			{
				CAN_RET=CheckVIN_CAN();
				if		(CAN_RET==1) return;	//RX OK 
				else if( CAN_RET==0) break;		//RX NONE
			}
		}

        if (CAN_RET == 0)
        {
            HAL_Delay(70);	//170220 EV¬˜∑Æ¿Ã CheckVIN_CAN(); ∏Ì∑…ø° ¿¿¥‰(FIRST FRAME)«“ ∞ÊøÏ ∆ﬂø˛æÓ¥¬ 07E8 ∏∏ ºˆΩ≈«œµµ∑œ ∏∂Ω∫≈∑ µ«æÓ ¿÷¿∏π«∑Œ FLOW CONTROL º€Ω≈ «œ¡ˆ ∏¯«œ∞Ì ¬˜∑Æ¿∫ SPECªÛ 100ms±Ó¡ˆ  FLOW CONTROL¿ª ±‚¥Ÿ∏∞¥Ÿ.
                        //∂ßπÆø° 100ms ¿Ã≥ªø° EV ¬˜∑Æ CheckVIN_CAN(); Ω√µµ «“ ∞ÊøÏ ¿¿¥‰«œ¡ˆ æ ¿ª ºˆ ¿÷¿Ω
            CAN_RET = CheckVIN_EV();//170118 ¿Ãø¨√∂
            if (CAN_RET == 1)
            {
                return;
            }
        }
        
        if(CAN_RET == 0)
        {
            CAN_RET = CheckVIN_FCEV();//2018.08.24 LJH FCEV¬˜∑Æ∞˙ TM¬˜∑Æ AutoVINºˆ¡§ ø‰√ª(∞˚¿Ã»£ ø¨±∏ø¯)
            if(CAN_RET == 1)    return;
        }
        
        if(CAN_RET == 0)
        {
            CAN_RET = CheckVIN_TM();//2018.08.24 LJH FCEV¬˜∑Æ∞˙ TM¬˜∑Æ AutoVINºˆ¡§ ø‰√ª(∞˚¿Ã»£ ø¨±∏ø¯)
            if(CAN_RET == 1)    return;
        }
        
        if(CAN_RET == 0)
        {
          CAN_RET = CheckVIN_J1979_2_3_11BIT();// 251216 Jay Add J1979-2/3 11bit CAN
          if(CAN_RET == 1)    return;
        }
        
        if(CAN_RET == 0)
        {
          CAN_RET = CheckVIN_CLU();// 240611 NHJ. Add CLU AutoVIN 
          if(CAN_RET == 1)    return;
        }
        
        if(CAN_RET==2) g_ucAUTOVIN[0]=0;	//RETRY 3»∏∏¶ «ÿµµ æ≤∑π±‚ µ•¿Ã≈Õ¿Œ ∞ÊøÏ µ•¿Ã≈Õ ∏¯πﬁæ“¥Ÿ∞Ì √≥∏Æ 140212 LWH

        if(CheckVIN_KWP2000()) 	return;
        //if(ucFlag != 0x01 )	CheckVIN_ISO9141();


    }

	return;
}

U8 CheckVIN_CAN_29bit(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;
	uint32_t	ulStartMaskValue;
	uint32_t	ulEndMaskValue;
	clearRXCanMessage();
	clearTXCanMessage();
	
	ulStartMaskValue = 0x18DAF100;
	ulEndMaskValue = 0x18DAF100;
	HIGHCAN2_120OHM_ENABLE;

	
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	CAN_Channel_Masket_Set(&hfdcan1, FDCAN_FILTER_TO_RXFIFO0, 0x02, 1, &ulStartMaskValue, &ulEndMaskValue);
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(ulStartMaskValue, ulEndMaskValue);
#endif
	
	startFDCan( 1, 0, 0 );//ch1 obd 6,14 ∞Ì¡§

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x18DB33F1;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x02;
			txPkt->mData[1] = 0x09;
			txPkt->mData[2] = 0x02;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN 29BIT\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s,%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg),__FUNCTION__);
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					//if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5 ) &&
#else
					//if( ( rxPkt->pSource == &hfdcan2 ) &&
#endif
					//	( rxPkt->mRxHeader.Identifier == canID ) &&
					//	( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					//{
					//	packet->mData[1] = TEST_OK;
					//}

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
					  	return 0;
					}
					else
					{
						g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier)&0x00FF);
						g_ucAUTOVIN[4]=rxPkt->mData[2];
						g_ucAUTOVIN[5]=rxPkt->mData[3];
						g_ucAUTOVIN[6]=rxPkt->mData[4];
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
						g_ucAUTOVIN[9]=rxPkt->mData[7];
					}

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
	if( usCanDataRxlen == 0 || usCanDataRxlen > 50 ) return 0;	// buffer(g_ucAUTOVIN) overflow protection
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x18DA00F1;	//only engine ECU

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x08;
			txPkt->mData[2] = 0x05;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

			
#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}

					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
			g_ucAUTOVIN[1] = usCanDataRxlen+4;
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < usCanDataRxlen+4; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[4+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif

	return 1;
}

U8 CheckVIN_CAN(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;

	clearRXCanMessage();
	clearTXCanMessage();

	HIGHCAN2_120OHM_ENABLE;

#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x7e8, 0x7e8 );
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(0x000007E8,0x000007E8);
#endif
	startFDCan( 1, 0, 0 );//ch1 obd 6,14 ∞Ì¡§

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07DF;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x02;
			txPkt->mData[1] = 0x09;
			txPkt->mData[2] = 0x02;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					//if( ( rxPkt->pSource == &hfdcan2 ) &&
#else
					//if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5 ) &&
#endif
					//	( rxPkt->mRxHeader.Identifier == canID ) &&
					//	( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					//{
					//	packet->mData[1] = TEST_OK;
					//}

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
					  	return 0;
					}
					else
					{
						g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier)&0x00FF);
						g_ucAUTOVIN[4]=rxPkt->mData[2];
						g_ucAUTOVIN[5]=rxPkt->mData[3];
						g_ucAUTOVIN[6]=rxPkt->mData[4];
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
						g_ucAUTOVIN[9]=rxPkt->mData[7];
					}

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
	if( usCanDataRxlen == 0 || usCanDataRxlen > 50 ) return 0;	// buffer(g_ucAUTOVIN) overflow protection
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07E0;	//only engine ECU

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x00;
			txPkt->mData[2] = 0x00;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}

					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
			g_ucAUTOVIN[1] = usCanDataRxlen+2;
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < usCanDataRxlen+2; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[2+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif

	return 1;
}
U8 CheckVIN_EV_NEW(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;

	clearRXCanMessage();
	clearTXCanMessage();

	HIGHCAN2_120OHM_ENABLE;
	
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x7EA, 0x7EA );
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(0x000007EA,0x000007EA);
#endif
	startFDCan( 1, 0, 0 );

	clearRXCanMessage();
	clearTXCanMessage();
	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07E2;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x03;
			txPkt->mData[1] = 0x22;
			txPkt->mData[2] = 0xF1;
			txPkt->mData[3] = 0x90;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN EV NEW\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
					  	return 0;
					}
					else
					{
						g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier)&0x00FF);
						g_ucAUTOVIN[4]=rxPkt->mData[2];
						g_ucAUTOVIN[5]=rxPkt->mData[3];
						g_ucAUTOVIN[6]=rxPkt->mData[4];
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
						g_ucAUTOVIN[9]=rxPkt->mData[7];
					}

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
	if( usCanDataRxlen == 0 || usCanDataRxlen > 50 ) return 0;	// buffer(g_ucAUTOVIN) overflow protection
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07E2;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x00;
			txPkt->mData[2] = 0x00;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}

					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
			g_ucAUTOVIN[1] = usCanDataRxlen+2;
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < usCanDataRxlen+2; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[2+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif
	//jkc stopFDCan( &hfdcan2 );

	return 1;
}
U8 CheckVIN_EV(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;

	clearRXCanMessage();
	clearTXCanMessage();

	HIGHCAN2_120OHM_ENABLE;
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

	setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x7EA, 0x7EA );
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(0x000007EA,0x000007EA);
#endif
	startFDCan( 1, 0, 0 );

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07E2;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x02;
			txPkt->mData[1] = 0x1A;
			txPkt->mData[2] = 0x90;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN EV\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
					  	return 0;
					}
					else
					{
						g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier)&0x00FF);
						g_ucAUTOVIN[4]=rxPkt->mData[2];
						g_ucAUTOVIN[5]=rxPkt->mData[3];
						g_ucAUTOVIN[6]=rxPkt->mData[4];
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
						g_ucAUTOVIN[9]=rxPkt->mData[7];
					}

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
	if( usCanDataRxlen == 0 || usCanDataRxlen > 50 ) return 0;	// buffer(g_ucAUTOVIN) overflow protection
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07E2;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x00;
			txPkt->mData[2] = 0x00;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )//while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}

					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x05;	// EV Positive Response
			g_ucAUTOVIN[1] = usCanDataRxlen+2;
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < usCanDataRxlen+2; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[2+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	
#endif
	//jkc stopFDCan( &hfdcan2 );

	return 1;
}
U8 CheckVIN_FCEV(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;

	clearRXCanMessage();
	clearTXCanMessage();

	HIGHCAN2_120OHM_ENABLE;
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

	setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x76C, 0x76C );
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(0x0000076C,0x0000076C);
#endif
	startFDCan( 1, 0, 0 );

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x0764;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x03;
			txPkt->mData[1] = 0x22;
			txPkt->mData[2] = 0xF1;
			txPkt->mData[3] = 0x90;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN FCEV\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
					  	return 0;
					}
					else
					{
						g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier)&0x00FF);
						g_ucAUTOVIN[4]=rxPkt->mData[2];
						g_ucAUTOVIN[5]=rxPkt->mData[3];
						g_ucAUTOVIN[6]=rxPkt->mData[4];
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
						g_ucAUTOVIN[9]=rxPkt->mData[7];
					}

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
	if( usCanDataRxlen == 0 || usCanDataRxlen > 50 ) return 0;	// buffer(g_ucAUTOVIN) overflow protection
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x0764;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x00;
			txPkt->mData[2] = 0x00;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}

					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	// EV Positive Response
			g_ucAUTOVIN[1] = usCanDataRxlen+2;
			GLogN("\n\r CAN AUTOVIN FCEV RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < usCanDataRxlen+2; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[2+i]);
			}

		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif
	//jkc stopFDCan( &hfdcan2 );

	return 1;
}

U8 CheckVIN_CLU(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;

	clearRXCanMessage();
	clearTXCanMessage();

	HIGHCAN2_120OHM_ENABLE;
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

	setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x7CE, 0x7CE );
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(0x000007CE,0x000007CE);
#endif
	startFDCan( 1, 0, 0 );

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07C6;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x03;
			txPkt->mData[1] = 0x22;
			txPkt->mData[2] = 0xF1;
			txPkt->mData[3] = 0x90;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN CLU\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
					  	return 0;
					}
					else
					{
						g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier)&0x00FF);
						g_ucAUTOVIN[4]=rxPkt->mData[2];
						g_ucAUTOVIN[5]=rxPkt->mData[3];
						g_ucAUTOVIN[6]=rxPkt->mData[4];
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
						g_ucAUTOVIN[9]=rxPkt->mData[7];
					}

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
	if( usCanDataRxlen == 0 || usCanDataRxlen > 50 ) return 0;	// buffer(g_ucAUTOVIN) overflow protection
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07C6;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x08;
			txPkt->mData[2] = 0x02;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}

					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	// EV Positive Response
			g_ucAUTOVIN[1] = usCanDataRxlen+2;
			GLogN("\n\r CAN AUTOVIN CLU RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < usCanDataRxlen+2; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[2+i]);
			}

		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif
	//jkc stopFDCan( &hfdcan2 );

	return 1;
}

U8 CheckVIN_J1979_2_3_11BIT(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;
    uint16_t  usCanRxID = 0;
    uint16_t  usCanTxID = 0;

	clearRXCanMessage();
	clearTXCanMessage();

	HIGHCAN2_120OHM_ENABLE;
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

	setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x7E8, 0x7EA );
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(0x000007CE,0x000007CE);
#endif
	startFDCan( 1, 0, 0 );

	// Tx
    txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07DF;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x03;
			txPkt->mData[1] = 0x22;
			txPkt->mData[2] = 0xF8;
			txPkt->mData[3] = 0x02;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN J1979-2/3\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}
                    
                    usCanRxID = rxPkt->mRxHeader.Identifier;
                    if( usCanRxID == 0x7E8 || usCanRxID == 0x7EA )
                    {
                        if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
                        {
                            uiP3min = 6000;
                        }
                        else if(rxPkt->mData[1]==0x7F)
                        {
                        }
                        else if( ( rxPkt->mData[5] >= '0' &&  rxPkt->mData[5]<='9') || (  rxPkt->mData[5] >= 'A' &&  rxPkt->mData[5]<='Z') || (  rxPkt->mData[5] >= 'a' &&  rxPkt->mData[5]<='z') )
                        {
                            g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
                            g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier)&0x00FF);
                            g_ucAUTOVIN[4]=rxPkt->mData[2];
                            g_ucAUTOVIN[5]=rxPkt->mData[3];
                            g_ucAUTOVIN[6]=rxPkt->mData[4];
                            g_ucAUTOVIN[7]=rxPkt->mData[5];
                            g_ucAUTOVIN[8]=rxPkt->mData[6];
                            g_ucAUTOVIN[9]=rxPkt->mData[7];
                            
                            usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];
                            usCanTxID = usCanRxID-8;
                            
                            osPoolFree( hFdcanPktPool, (void *)rxPkt );
                            osPoolFree( hMsgPool, (void *)rxMsg );
                            break;
                        }
                    }
                    osPoolFree( hFdcanPktPool, (void *)rxPkt );
                    osPoolFree( hMsgPool, (void *)rxMsg ); 
                    //break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
	if( usCanDataRxlen == 0 || usCanDataRxlen > 50 ) return 0;	// buffer(g_ucAUTOVIN) overflow protection
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = usCanTxID;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x08;
			txPkt->mData[2] = 0x02;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}
                    if( usCanRxID == rxPkt->mRxHeader.Identifier )
					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	// EV Positive Response
			g_ucAUTOVIN[1] = usCanDataRxlen+2;
			GLogN("\n\r CAN AUTOVIN CLU RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < usCanDataRxlen+2; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[2+i]);
			}

		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif
	//jkc stopFDCan( &hfdcan2 );

	return 1;
}

U8 CheckVIN_TM(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;

	clearRXCanMessage();
	clearTXCanMessage();

	HIGHCAN2_120OHM_ENABLE;
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

	setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x7E8, 0x7E8 );
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(0x000007E8,0x000007E8);
#endif
	startFDCan( 1, 0, 0 );

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07E0;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x02;
			txPkt->mData[1] = 0x10;
			txPkt->mData[2] = 0x03;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN TM OPEN\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
					  	return 0;
					}
					else
					{

					}

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07DF;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x02;
			txPkt->mData[1] = 0x09;
			txPkt->mData[2] = 0x02;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN TM\r\n ");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
					  	return 0;
					}
					else
					{
						g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier)&0x00FF);
						g_ucAUTOVIN[4]=rxPkt->mData[2];
						g_ucAUTOVIN[5]=rxPkt->mData[3];
						g_ucAUTOVIN[6]=rxPkt->mData[4];
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
						g_ucAUTOVIN[9]=rxPkt->mData[7];
					}

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
	if( usCanDataRxlen == 0 || usCanDataRxlen > 50 ) return 0;	// buffer(g_ucAUTOVIN) overflow protection
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07E0;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x00;
			txPkt->mData[2] = 0x00;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif

			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}

					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	// EV Positive Response
			g_ucAUTOVIN[1] = usCanDataRxlen+2;
			GLogN("\n\r CAN AUTOVIN TM RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < usCanDataRxlen+2; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[2+i]);
			}

		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif
	//jkc stopFDCan( &hfdcan2 );

	return 1;
}
U8 CheckVIN_KWP2000(void)
{
	uint8_t	txBuff[15]	= { 0, };
	uint8_t	rxBuff[100]	= { 0, };
	U8 end=0,WriteMsgLength=0;
	uint8_t	i;
	uint32_t	uiTimeout,uiRxCnt;
	uint32_t	oldtime;
	u16 DlcRxCount = 0;
	u16 DlcRxCountTemp = 0;
	
	GLogN("\r\n KWP2000 AUTOVIN ");
	DLC_HW_Set(KLLINE_RELAY,K_SERIAL,K_NORMAL,L_PULSE,L_NORMAL,0,L_PULSE_HIGH,RXD_OBD2,RXD_NORMAL,Pullup,K_Pullup_510,L_Pullup_510,0,KL_LINE1_CONNECT_CH07,L_ChOff,R_ChOff);
	DLC_RX_BUFF_CLEAR();
			 
	CAN_uDelay(300);

	KW_fast_init(25,25);
	
	WriteMsgLength=5;
	txBuff[0] = 0xC1;
	txBuff[1] = 0x33;
	txBuff[2] = 0xF1;
	txBuff[3] = 0x81;
	txBuff[4] = 0x66;

	transmitKL_ByteTime( KL_LINE1, txBuff, WriteMsgLength, 5 );
	osDelay(1);

	clearKLReceiveData(KL_LINE1);//rx buffer clear
	clearKLReceiveData(KL_LINE2);//rx buffer clear

	oldtime = Get_Tmr();
	uiRxCnt=0;
	uiTimeout=500;

	g_bIsAutoVIN=TRUE;
	GLogN("\n\r rxBuff(vin) :");
	while( Get_TmrDelta( Get_Tmr(), oldtime ) < uiTimeout )
	{
		DlcRxCount += getKLReceiveData( KL_LINE1, &rxBuff[uiRxCnt], 1 );
		uiRxCnt++;
		
		if(DlcRxCount!=DlcRxCountTemp)
		{
			GLogN(" %02X",rxBuff[uiRxCnt-1]);
			DlcRxCountTemp=DlcRxCount;
			intDlccomCount = g_uiAckTiming;
		}
		else break;
		if(uiRxCnt>1900) break; //infinite reception processing
	}
	uiRxCnt--;
	g_bIsAutoVIN=FALSE;

	if (DlcRxCount < WriteMsgLength ) return 0;

	WriteMsgLength=6;
	txBuff[0] = 0xC2;
	txBuff[1] = 0x33;
	txBuff[2] = 0xF1;
	txBuff[3] = 0x09;
	txBuff[4] = 0x02;
	txBuff[5] = 0xF1;

	transmitKL_ByteTime( KL_LINE1, txBuff, WriteMsgLength, 5 );
	osDelay(1);

	clearKLReceiveData(KL_LINE1);//rx buffer clear
	clearKLReceiveData(KL_LINE2);//rx buffer clear

	DlcRxCount = 0;
	DlcRxCountTemp = 0;
	oldtime = Get_Tmr();
	uiRxCnt=0;
	uiTimeout=1000;

	g_bIsAutoVIN=TRUE;
	GLogN("\n\r rxBuff(vin) :");
	while( Get_TmrDelta( Get_Tmr(), oldtime ) < uiTimeout )
	{
		DlcRxCount += getKLReceiveData( KL_LINE1, &rxBuff[uiRxCnt], 1 );
		uiRxCnt++;
		
		if(DlcRxCount!=DlcRxCountTemp)
		{
			GLogN(" %02X",rxBuff[uiRxCnt-1]);
			DlcRxCountTemp=DlcRxCount;
			intDlccomCount = g_uiAckTiming;
		}
		else break;
		if(uiRxCnt>1900) break; //infinite reception processing
	}
	uiRxCnt--;
	g_bIsAutoVIN=FALSE;

	if (DlcRxCount < WriteMsgLength ) return 0;

	end = uiRxCnt;
	//PassThruSendMsg[0].DataSize = end;
	g_ucAUTOVIN[0] = 0x02;		//KWP RX TRUE
	g_ucAUTOVIN[1] = end;
	GLogN("\n\r KWP AUTOVIN RX(%d) : ",end);
	for(i=0; i<end; i++)
	{
		g_ucAUTOVIN[i+2] = rxBuff[i];
		GLogN("%c",g_ucAUTOVIN[2+i]);
	}
	
	WriteMsgLength=5;
	txBuff[0] = 0xC1;
	txBuff[1] = 0x33;
	txBuff[2] = 0xF1;
	txBuff[3] = 0x82;
	txBuff[4] = 0x67;

	transmitKL_ByteTime( KL_LINE1, txBuff, WriteMsgLength, 5 );
	osDelay(1);

	clearKLReceiveData(KL_LINE1);//rx buffer clear
	clearKLReceiveData(KL_LINE2);//rx buffer clear

	DlcRxCount = 0;
	DlcRxCountTemp = 0;
	oldtime = Get_Tmr();
	uiRxCnt=0;
	uiTimeout=1000;
	
	g_bIsAutoVIN=TRUE;
	GLogN("\n\r rxBuff(vin) :");
	while( Get_TmrDelta( Get_Tmr(), oldtime ) < uiTimeout )
	{
		DlcRxCount += getKLReceiveData( KL_LINE1, &rxBuff[uiRxCnt], 1 );
		uiRxCnt++;
		
		if(DlcRxCount!=DlcRxCountTemp)
		{
			GLogN(" %02X",rxBuff[uiRxCnt-1]);
			DlcRxCountTemp=DlcRxCount;
			intDlccomCount = g_uiAckTiming;
		}
		else break;
		if(uiRxCnt>1900) break; //infinite reception processing
	}
	uiRxCnt--;
	g_bIsAutoVIN=FALSE;

	return 1;
}

#ifdef CV_AUTOVIN
//#if 1

U8 CheckCV_VIN_CAN(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;

	clearRXCanMessage();
	clearTXCanMessage();

	HIGHCAN2_120OHM_ENABLE;

#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x7e8, 0x7e8 );
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(0x000007E8,0x000007E8);
#endif
	startFDCan( 1, 0, 0 );//ch1 obd 6,14 ∞Ì¡§

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07E0;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x03;
			txPkt->mData[1] = 0x22;
			txPkt->mData[2] = 0xF1;
			txPkt->mData[3] = 0x90;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					//if( ( rxPkt->pSource == &hfdcan2 ) &&
#else
					//if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5 ) &&
#endif
					//	( rxPkt->mRxHeader.Identifier == canID ) &&
					//	( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					//{
					//	packet->mData[1] = TEST_OK;
					//}

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                    
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
					  	return 0;
					}
					else
					{
                        //CANID
                        g_ucAUTOVIN[1]=0x00;
                        g_ucAUTOVIN[2]=0x00;
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[4]=((rxPkt->mRxHeader.Identifier)&0x00FF);
                        
                        //Length
                        g_ucAUTOVIN[5] = rxPkt->mData[0]&0x0F;
                        g_ucAUTOVIN[6] = rxPkt->mData[1];
                        
                        //Check "KM"
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
                        g_ucAUTOVIN[9]=rxPkt->mData[7];
					}
                    
                    if( (g_ucAUTOVIN[7] != 'K') || ((g_ucAUTOVIN[8] != 'M') && (g_ucAUTOVIN[8] != 'N')) || (rxPkt->mData[2] != 0x62) ) //check consistency
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN));
                        uiP3min = 70;
				 		continue;
                    }

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
    
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x07E0;	//only engine ECU

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x00;
			txPkt->mData[2] = 0x00;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}

					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}
                    
                    memset(&rxPkt->mData[0], 0x00, FDCAN_PACKET_MAX_SIZE);
                    
					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
			
			for( i = 0; i < AUTOVIN_LENGTH; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[7+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif

	return 1;
}

U8 CheckCV_VIN_ExtCAN(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;
	uint32_t	ulStartMaskValue;
	uint32_t	ulEndMaskValue;
	clearRXCanMessage();
	clearTXCanMessage();
	
	ulStartMaskValue = 0x18DAF900;
	ulEndMaskValue = 0x18DAF9FF;
	HIGHCAN2_120OHM_ENABLE;

	
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	CAN_Channel_Masket_Set(&hfdcan1, FDCAN_FILTER_TO_RXFIFO0, 0x02, 1, &ulStartMaskValue, &ulEndMaskValue);
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(ulStartMaskValue, ulEndMaskValue);
#endif
	
	startFDCan( 1, 0, 0 );//ch1 obd 6,14 ∞Ì¡§

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x18DBFFF9;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x03;
			txPkt->mData[1] = 0x22;
			txPkt->mData[2] = 0xF1;
			txPkt->mData[3] = 0x90;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN 29BIT\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s,%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg),__FUNCTION__);
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					//if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5 ) &&
#else
					//if( ( rxPkt->pSource == &hfdcan2 ) &&
#endif
					//	( rxPkt->mRxHeader.Identifier == canID ) &&
					//	( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					//{
					//	packet->mData[1] = TEST_OK;
					//}

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
					  	return 0;
					}
					else
					{
                        //CANID
                        g_ucAUTOVIN[1]=((rxPkt->mRxHeader.Identifier >> 24)&0x00FF);
                        g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 16)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[4]=((rxPkt->mRxHeader.Identifier)&0x00FF);
                        
						//Length
                        g_ucAUTOVIN[5] = rxPkt->mData[0]&0x0F;
                        g_ucAUTOVIN[6] = rxPkt->mData[1];
                        
                        //Check "KM"
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
                        g_ucAUTOVIN[9]=rxPkt->mData[7];
					}
                    
                    if( (g_ucAUTOVIN[7] != 'K') || ((g_ucAUTOVIN[8] != 'M') && (g_ucAUTOVIN[8] != 'N')) || (rxPkt->mData[2] != 0x62)  ) //check consistency
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN));
                        uiP3min = 70;
				 		continue;
                    }

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
    
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = (0x18DA<<16) | (g_ucAUTOVIN[4]<<8) | g_ucAUTOVIN[3];

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x08;
			txPkt->mData[2] = 0x05;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

			
#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}
                    
                    if( rxPkt->mRxHeader.Identifier == (0x18DA<<16) | (g_ucAUTOVIN[3]<<8) | g_ucAUTOVIN[4] )
					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}
                    else
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        uiP3min = 70;
				 		continue;
                    }
                    
                    memset(&rxPkt->mData[0], 0x00, FDCAN_PACKET_MAX_SIZE);

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
			
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < AUTOVIN_LENGTH; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[7+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif

	return 1;
}

U8 CheckCV_VIN_PubEngine(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
    uint8_t  ucAbnormalCnt=0;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;
	uint32_t	ulStartMaskValue;
	uint32_t	ulEndMaskValue;
	clearRXCanMessage();
	clearTXCanMessage();
	
	ulStartMaskValue = 0x18DAF100;
	ulEndMaskValue = 0x18DAF100;
	HIGHCAN2_120OHM_ENABLE;

	
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	CAN_Channel_Masket_Set(&hfdcan1, FDCAN_FILTER_TO_RXFIFO0, 0x02, 1, &ulStartMaskValue, &ulEndMaskValue);
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(ulStartMaskValue, ulEndMaskValue);
#endif
	
	startFDCan( 1, 0, 0 );//ch1 obd 6,14 ∞Ì¡§

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x18DB33F1;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x02;
			txPkt->mData[1] = 0x09;
			txPkt->mData[2] = 0x02;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN 29BIT\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s,%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg),__FUNCTION__);
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					//if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5 ) &&
#else
					//if( ( rxPkt->pSource == &hfdcan2 ) &&
#endif
					//	( rxPkt->mRxHeader.Identifier == canID ) &&
					//	( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					//{
					//	packet->mData[1] = TEST_OK;
					//}

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
					  	return 0;
					}
					else
					{
                        //CANID
                        g_ucAUTOVIN[1]=((rxPkt->mRxHeader.Identifier >> 24)&0x00FF);
                        g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 16)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[4]=((rxPkt->mRxHeader.Identifier)&0x00FF);
                        
						//Length
                        g_ucAUTOVIN[5] = rxPkt->mData[0]&0x0F;
                        g_ucAUTOVIN[6] = rxPkt->mData[1];
                        
                        //Check "KM"
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
                        g_ucAUTOVIN[9]=rxPkt->mData[7];
					}
                    
                    if( (g_ucAUTOVIN[7] != 'K') || ((g_ucAUTOVIN[8] != 'M') && (g_ucAUTOVIN[8] != 'N')) || (rxPkt->mData[2] != 0x49) ) //check consistency
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN));
                        uiP3min = 70;
				 		continue;
                    }

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
    
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = (0x18DA<<16) | (g_ucAUTOVIN[4]<<8) | g_ucAUTOVIN[3];

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x08;
			txPkt->mData[2] = 0x05;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

			
#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}
                    
                    if((rxPkt->mRxHeader.Identifier ==( (0x18DA<<16) | (g_ucAUTOVIN[3]<<8) | g_ucAUTOVIN[4])) &&
                       ((rxPkt->mData[0]==0x03)&&(rxPkt->mData[1]==0x7F)&&(rxPkt->mData[2]==0x00)))
				 	{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
				 		uiP3min = 6000;
                        ucAbnormalCnt++;
				 		continue;
				 	}
                    
                    i = i-ucAbnormalCnt;
                    ucAbnormalCnt=0;
                    
                    if( rxPkt->mRxHeader.Identifier == (0x18DA<<16) | (g_ucAUTOVIN[3]<<8) | g_ucAUTOVIN[4] )
					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}
                    else
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        uiP3min = 70;
				 		continue;
                    }
                    
                    memset(&rxPkt->mData[0], 0x00, FDCAN_PACKET_MAX_SIZE);

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
			
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i <AUTOVIN_LENGTH; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[7+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif

	return 1;
}


U8 CheckCV_VIN_FGEngine(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;
	uint32_t	ulStartMaskValue;
	uint32_t	ulEndMaskValue;
	clearRXCanMessage();
	clearTXCanMessage();
	
	ulStartMaskValue = 0x18DAF100;
	ulEndMaskValue = 0x18DAF100;
	HIGHCAN2_120OHM_ENABLE;

	
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	CAN_Channel_Masket_Set(&hfdcan1, FDCAN_FILTER_TO_RXFIFO0, 0x02, 1, &ulStartMaskValue, &ulEndMaskValue);
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(ulStartMaskValue, ulEndMaskValue);
#endif
	
	startFDCan( 1, 0, 0 );//ch1 obd 6,14 ∞Ì¡§

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x18DA00F1;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x02;
			txPkt->mData[1] = 0x22;
			txPkt->mData[2] = 0x80;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN 29BIT\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s,%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg),__FUNCTION__);
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					//if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5 ) &&
#else
					//if( ( rxPkt->pSource == &hfdcan2 ) &&
#endif
					//	( rxPkt->mRxHeader.Identifier == canID ) &&
					//	( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					//{
					//	packet->mData[1] = TEST_OK;
					//}

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
					  	return 0;
					}
					else
					{
                        //CANID
                        g_ucAUTOVIN[1]=((rxPkt->mRxHeader.Identifier >> 24)&0x00FF);
                        g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 16)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[4]=((rxPkt->mRxHeader.Identifier)&0x00FF);
                        
						//Length
                        g_ucAUTOVIN[5] = rxPkt->mData[0]&0x0F;
                        g_ucAUTOVIN[6] = rxPkt->mData[1];
                        
                        //Check "KM"
						g_ucAUTOVIN[7]=rxPkt->mData[4];
						g_ucAUTOVIN[8]=rxPkt->mData[5];
                        g_ucAUTOVIN[9]=rxPkt->mData[6];
                        g_ucAUTOVIN[10]=rxPkt->mData[7];
					}
                    
                    if( rxPkt->mData[2] != 0x62 ) //check consistency
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN));
                        uiP3min = 70;
				 		continue;
                    }

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
    
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = (0x18DA<<16) | (g_ucAUTOVIN[4]<<8) | g_ucAUTOVIN[3];

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x08;
			txPkt->mData[2] = 0x05;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

			
#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}
                    
                    if( rxPkt->mRxHeader.Identifier == (0x18DA<<16) | (g_ucAUTOVIN[3]<<8) | g_ucAUTOVIN[4] )
					{
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[17+(i*7)] = rxPkt->mData[7];
					}
                    else
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        uiP3min = 70;
				 		continue;
                    }
                    
                    memset(&rxPkt->mData[0], 0x00, FDCAN_PACKET_MAX_SIZE);

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
            
            if( (g_ucAUTOVIN[34] == 'K') && ((g_ucAUTOVIN[35] == 'M') || (g_ucAUTOVIN[35] == 'N')) ) //check consistency
            {
                g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
                memset(&g_ucAUTOVIN[7], 0x00, AUTOVIN_LENGTH);
                memcpy(&g_ucAUTOVIN[7], &g_ucAUTOVIN[34], AUTOVIN_LENGTH);
            }
			else
            {
                memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN));
                return 0;
            }
			
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < AUTOVIN_LENGTH; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[7+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif

	return 1;
}

U8 CheckCV_VIN_HLEngine(void)
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;
	uint32_t	ulStartMaskValue;
	uint32_t	ulEndMaskValue;
    uint8_t ucBlkSize = 0, ucAVCnt = 0, ucRXCnt = 0;
    //uint8_t ucTestFC = 0;
    
	clearRXCanMessage();
	clearTXCanMessage();
	
	ulStartMaskValue = 0x18DAF900;
	ulEndMaskValue = 0x18DAF900;
	HIGHCAN2_120OHM_ENABLE;

	
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	CAN_Channel_Masket_Set(&hfdcan1, FDCAN_FILTER_TO_RXFIFO0, 0x02, 1, &ulStartMaskValue, &ulEndMaskValue);
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(ulStartMaskValue, ulEndMaskValue);
#endif
	
	startFDCan( 1, 0, 0 );//ch1 obd 6,14 ∞Ì¡§

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x18DA00F9;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x04;
			txPkt->mData[1] = 0x21;
			txPkt->mData[2] = 0x32;
			txPkt->mData[3] = 0x01;
			txPkt->mData[4] = 0x01;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN 29BIT\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s,%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg),__FUNCTION__);
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					//if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5 ) &&
#else
					//if( ( rxPkt->pSource == &hfdcan2 ) &&
#endif
					//	( rxPkt->mRxHeader.Identifier == canID ) &&
					//	( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					//{
					//	packet->mData[1] = TEST_OK;
					//}

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
					  	return 0;
					}
					else
					{
                        //CANID
                        g_ucAUTOVIN[1]=((rxPkt->mRxHeader.Identifier >> 24)&0x00FF);
                        g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 16)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[4]=((rxPkt->mRxHeader.Identifier)&0x00FF);
                        
						//Length
                        g_ucAUTOVIN[5] = rxPkt->mData[0]&0x0F;
                        g_ucAUTOVIN[6] = rxPkt->mData[1];
                        
                        //Check "KM"
						g_ucAUTOVIN[7]=rxPkt->mData[6];
						g_ucAUTOVIN[8]=rxPkt->mData[7];
                        g_ucAUTOVIN[9]=rxPkt->mData[8];
					}
                    
                    if( rxPkt->mData[2] != 0x61 ) //check consistency
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN));
                        uiP3min = 70;
				 		continue;
                    }

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
    
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = (0x18DA<<16) | (g_ucAUTOVIN[4]<<8) | g_ucAUTOVIN[3];

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
            ucBlkSize = 0x08;
              
			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = ucBlkSize;
			txPkt->mData[2] = 0x05;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

			
#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
            ucAVCnt = 0;
            ucRXCnt = 0;
            //ucTestFC = 0;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
                    ucRXCnt++;
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}
                    
                    if( rxPkt->mRxHeader.Identifier == (0x18DA<<16) | (g_ucAUTOVIN[3]<<8) | g_ucAUTOVIN[4] )
					{
                        if( ucRXCnt == ucBlkSize )
                        {
                            ucRXCnt =0;
                            txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
                            if( txMsg != NULL )
                            {
                                txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
                                if( txPkt != NULL )
                                {
                                    //ucTestFC++;
                                    txPkt->mLen			= 8;
                                    txPkt->pTarget		= &hfdcan1;
                                    
                                    txPkt->mData[0] = 0x30;
                                    txPkt->mData[1] = ucBlkSize;
                                    txPkt->mData[2] = 0x05;
                                    txPkt->mData[3] = 0x00;
                                    //txPkt->mData[3] = ucTestFC;
                                    txPkt->mData[4] = 0x00;
                                    txPkt->mData[5] = 0x00;
                                    txPkt->mData[6] = 0x00;
                                    txPkt->mData[7] = 0x00;
                                    
                                    makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
                                    
                                    txMsg->pPacket	= (void *)txPkt;
                                    
                                    if( ucAVCnt == 0 ) ucAVCnt = 1;
                                    else{}
                                    
                                    //uiP3min=70;
                                    osPoolFree( hFdcanPktPool, (void *)rxPkt );
                                    osPoolFree( hMsgPool, (void *)rxMsg );
                        
                                    osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
                                    uiP3min=700;
                                    continue;
                                }
                                else
                                {
                                    osPoolFree( hMsgPool, (void *)txMsg );
                                }
                            }
                            else{}
                        }
                        else if( ucAVCnt == 1 )
                        {
                            if( (rxPkt->mData[2] == 'K') && ((rxPkt->mData[3] == 'M') || (rxPkt->mData[3] == 'N')) ) //check consistency
                            {
                                g_ucAUTOVIN[7] = rxPkt->mData[2];
                                g_ucAUTOVIN[8] = rxPkt->mData[3];
                                g_ucAUTOVIN[9] = rxPkt->mData[4];
                                g_ucAUTOVIN[10] = rxPkt->mData[5];
                                g_ucAUTOVIN[11] = rxPkt->mData[6];
                                g_ucAUTOVIN[12] = rxPkt->mData[7];
                                
                                ucAVCnt++;
                            }
                            else{}
                        }
                        else if( ucAVCnt == 2 )
                        {
                            g_ucAUTOVIN[13] = rxPkt->mData[1];
                            g_ucAUTOVIN[14] = rxPkt->mData[2];
                            g_ucAUTOVIN[15] = rxPkt->mData[3];
                            g_ucAUTOVIN[16] = rxPkt->mData[4];
                            g_ucAUTOVIN[17] = rxPkt->mData[5];
                            g_ucAUTOVIN[18] = rxPkt->mData[6];
                            g_ucAUTOVIN[19] = rxPkt->mData[7];
                            
                            ucAVCnt++;
                        }
                        else if( ucAVCnt == 3 )
                        {
                            g_ucAUTOVIN[20] = rxPkt->mData[1];
                            g_ucAUTOVIN[21] = rxPkt->mData[2];
                            g_ucAUTOVIN[22] = rxPkt->mData[3];
                            g_ucAUTOVIN[23] = rxPkt->mData[4];
                            
                            g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
                            ucAVCnt++;
                        }
                        else{}
					}
                    else
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        uiP3min = 70;
				 		continue;
                    }
                    
                    memset(&rxPkt->mData[0], 0x00, FDCAN_PACKET_MAX_SIZE);

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
                    
                    uiP3min=70;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
			
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i < AUTOVIN_LENGTH; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[7+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif

	return g_ucAUTOVIN[0];
}

U8 CheckCV_VIN_VCU( U8 CAN_Line )
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;
	uint32_t	ulStartMaskValue;
	uint32_t	ulEndMaskValue;
	clearRXCanMessage();
	clearTXCanMessage();
	
	ulStartMaskValue = 0x18DAF9EF;
	ulEndMaskValue = 0x18DAF9EF;
	HIGHCAN2_120OHM_ENABLE;

	
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	CAN_Channel_Masket_Set(&hfdcan1, FDCAN_FILTER_TO_RXFIFO0, 0x02, 1, &ulStartMaskValue, &ulEndMaskValue);
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(ulStartMaskValue, ulEndMaskValue);
#endif
	
    if( CAN_Line == CAN_CHANNEL_1 ) startFDCan( 1, 0, 0 );//ch1 obd 6,14
    else if( CAN_Line == CAN_CHANNEL_2 ) startFDCan( 0, 1, 0 ); //ch1 obd 1,9
    else{}

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x18DAEFF9;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x03;
			txPkt->mData[1] = 0x22;
			txPkt->mData[2] = 0xF1;
			txPkt->mData[3] = 0x90;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN 29BIT\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s,%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg),__FUNCTION__);
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					//if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5 ) &&
#else
					//if( ( rxPkt->pSource == &hfdcan2 ) &&
#endif
					//	( rxPkt->mRxHeader.Identifier == canID ) &&
					//	( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					//{
					//	packet->mData[1] = TEST_OK;
					//}

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
					  	return 0;
					}
					else
					{
                        //CANID
                        g_ucAUTOVIN[1]=((rxPkt->mRxHeader.Identifier >> 24)&0x00FF);
                        g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 16)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[4]=((rxPkt->mRxHeader.Identifier)&0x00FF);
                        
						//Length
                        g_ucAUTOVIN[5] = rxPkt->mData[0]&0x0F;
                        g_ucAUTOVIN[6] = rxPkt->mData[1];
                        
                        //Check "KM"
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
                        g_ucAUTOVIN[9]=rxPkt->mData[7];
					}
                    
                    if( (g_ucAUTOVIN[7] != 'K') || ((g_ucAUTOVIN[8] != 'M') && (g_ucAUTOVIN[8] != 'N')) || (rxPkt->mData[2] != 0x62) ) //check consistency
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN));
                        uiP3min = 70;
				 		continue;
                    }

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
    
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = (0x18DA<<16) | (g_ucAUTOVIN[4]<<8) | g_ucAUTOVIN[3];

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x08;
			txPkt->mData[2] = 0x05;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

			
#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}
                    
                    if( rxPkt->mRxHeader.Identifier == (0x18DA<<16) | (g_ucAUTOVIN[3]<<8) | g_ucAUTOVIN[4] )
					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}
                    else
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        uiP3min = 70;
				 		continue;
                    }
                    
                    memset(&rxPkt->mData[0], 0x00, FDCAN_PACKET_MAX_SIZE);

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
			
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i <AUTOVIN_LENGTH; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[7+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif

	return 1;
}

U8 CheckCV_VIN_DTG( U8 CAN_Line )
{

	stMsgClst	*txMsg, *rxMsg;
	stFdcanPkt	*txPkt, *rxPkt;
	uint8_t		i,j,ucCanDataRxCnt;
	uint32_t	canID;
	osEvent		evt;
	uint32_t	uiP3min;
	uint16_t	usCanDataRxlen;
	uint32_t	ulStartMaskValue;
	uint32_t	ulEndMaskValue;
	clearRXCanMessage();
	clearTXCanMessage();
	
	ulStartMaskValue = 0x18DAF9EE;
	ulEndMaskValue = 0x18DAF9EE;
	HIGHCAN2_120OHM_ENABLE;

	
#ifdef USE_INTERNAL_CAN_ONLY
	CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	CAN_Channel_Masket_Set(&hfdcan1, FDCAN_FILTER_TO_RXFIFO0, 0x02, 1, &ulStartMaskValue, &ulEndMaskValue);
#else
	mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );
	SetCanMaskingMCP2518(ulStartMaskValue, ulEndMaskValue);
#endif
	
	if( CAN_Line == CAN_CHANNEL_1 ) startFDCan( 1, 0, 0 );//ch1 obd 6,14
    else if( CAN_Line == CAN_CHANNEL_2 ) startFDCan( 0, 1, 0 ); //ch1 obd 1,9
    else{}

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = 0x18DAEEF9;

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x03;
			txPkt->mData[1] = 0x22;
			txPkt->mData[2] = 0xF1;
			txPkt->mData[3] = 0x90;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );
			GLogN("CAN AUTOVIN 29BIT\r\n");

			// Rx
			//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
			uiP3min=70;
			while(1)
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s,%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg),__FUNCTION__);
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
					//if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5 ) &&
#else
					//if( ( rxPkt->pSource == &hfdcan2 ) &&
#endif
					//	( rxPkt->mRxHeader.Identifier == canID ) &&
					//	( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
					//{
					//	packet->mData[1] = TEST_OK;
					//}

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( i = 0; i < 8; i++ )
					{
						GLogN(" %02X",rxPkt->mData[i]);
					}

					if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
				 	{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
				 		uiP3min = 6000;
				 		continue;
				 	}
					else if(rxPkt->mData[1]==0x7F)
					{
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
					  	return 0;
					}
					else
					{
                        //CANID
                        g_ucAUTOVIN[1]=((rxPkt->mRxHeader.Identifier >> 24)&0x00FF);
                        g_ucAUTOVIN[2]=((rxPkt->mRxHeader.Identifier >> 16)&0x00FF);
						g_ucAUTOVIN[3]=((rxPkt->mRxHeader.Identifier >> 8)&0x00FF);
						g_ucAUTOVIN[4]=((rxPkt->mRxHeader.Identifier)&0x00FF);
                        
						//Length
                        g_ucAUTOVIN[5] = rxPkt->mData[0]&0x0F;
                        g_ucAUTOVIN[6] = rxPkt->mData[1];
                        
                        //Check "KM"
						g_ucAUTOVIN[7]=rxPkt->mData[5];
						g_ucAUTOVIN[8]=rxPkt->mData[6];
                        g_ucAUTOVIN[9]=rxPkt->mData[7];
					}
                    
                    if( (g_ucAUTOVIN[7] != 'K') || ((g_ucAUTOVIN[8] != 'M') && (g_ucAUTOVIN[8] != 'N')) || (rxPkt->mData[2] != 0x62) ) //check consistency
                    {   
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN));
                        uiP3min = 70;
				 		continue;
                    }

					usCanDataRxlen = (rxPkt->mData[0]&0x0F)*0x100+rxPkt->mData[1];

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					break;
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}
    
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = (0x18DA<<16) | (g_ucAUTOVIN[4]<<8) | g_ucAUTOVIN[3];

			txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
			txPkt->pTarget		= &hfdcan1;
#else
			txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
			txPkt->mData[0] = 0x30;
			txPkt->mData[1] = 0x08;
			txPkt->mData[2] = 0x05;
			txPkt->mData[3] = 0x00;
			txPkt->mData[4] = 0x00;
			txPkt->mData[5] = 0x00;
			txPkt->mData[6] = 0x00;
			txPkt->mData[7] = 0x00;

			
#ifdef USE_INTERNAL_CAN_ONLY
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#else
			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

			ucCanDataRxCnt = 14;
			// Rx
			uiP3min=70;
			for( i = 0; i < 100; i++ )
			{
				evt = osMessageGet( hFDRxMsg, uiP3min );
				if( evt.status == osEventMessage )
				{
#ifdef PRINT_MESSAGE_ID
					printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
					rxMsg	= ( stMsgClst * )evt.value.p;
					rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

					GLogN("\r\n rx: %04X",rxPkt->mRxHeader.Identifier);
					for( j = 0; j < 8; j++ )
					{
						GLogN(" %02X",rxPkt->mData[j]);
					}
                    
                    if( rxPkt->mRxHeader.Identifier == (0x18DA<<16) | (g_ucAUTOVIN[3]<<8) | g_ucAUTOVIN[4] )
					{
						g_ucAUTOVIN[10+(i*7)] = rxPkt->mData[1];
						g_ucAUTOVIN[11+(i*7)] = rxPkt->mData[2];
						g_ucAUTOVIN[12+(i*7)] = rxPkt->mData[3];
						g_ucAUTOVIN[13+(i*7)] = rxPkt->mData[4];
						g_ucAUTOVIN[14+(i*7)] = rxPkt->mData[5];
						g_ucAUTOVIN[15+(i*7)] = rxPkt->mData[6];
						g_ucAUTOVIN[16+(i*7)] = rxPkt->mData[7];
					}
                    else
                    {
                        osPoolFree( hFdcanPktPool, (void *)rxPkt );
                        osPoolFree( hMsgPool, (void *)rxMsg );
                        
                        uiP3min = 70;
				 		continue;
                    }
                    
                    memset(&rxPkt->mData[0], 0x00, FDCAN_PACKET_MAX_SIZE);

					osPoolFree( hFdcanPktPool, (void *)rxPkt );
					osPoolFree( hMsgPool, (void *)rxMsg );

					if(usCanDataRxlen < ucCanDataRxCnt) break;
					ucCanDataRxCnt = (ucCanDataRxCnt+7);
				}
				else return 0;	//no response
				//CanDataRxlen = (DlcRxBuff[3]&0x0F)*0x100+DlcRxBuff[4];
			}			
			g_ucAUTOVIN[0] = 0x01;	//CAN RX TRUE
			
			GLogN("\n\r CAN AUTOVIN RX(%d) : ",usCanDataRxlen);
			for( i = 0; i <AUTOVIN_LENGTH; i++ )
			{
				GLogN("%c",g_ucAUTOVIN[7+i]);
			}
            GLogN("\r\n");
		}
        else
        {
            osPoolFree( hMsgPool, (void *)txMsg );
        }
	}

	HIGHCAN2_120OHM_DISABLE;

	SetKL_Line( 0, 0 );
#ifdef USE_INTERNAL_CAN_ONLY
	stopFDCan( &hfdcan1 );
#else
	StopSpiCan();
#endif

	return 1;
}

#endif

void VCI_FAST_Init_Delay(uint16_t ms)
{
	if(g_ulProtocolID==PID_CAN
		||g_ulProtocolID==ISO15765
		||g_ulProtocolID==ISO15765_CUBIS
		||g_ulProtocolID==ISO15765_SINGLE
//		||g_ulProtocolID==ISO15765_GM
		||g_ulProtocolID==ISO14229
		||g_ulProtocolID==ISO14229_UDS
		||g_ulProtocolID==ISO15765_NEW
		||g_ulProtocolID==ISO15765_SINGLE_SMK
//		||g_ulProtocolID==ISO15765_GM_100K
//		||g_ulProtocolID==ISO15765_SM_MULTI
		||g_ulProtocolID==ISO15765_SINGLE_PODS
//		||g_ulProtocolID==ISO15765_SID36
		||g_ulProtocolID==ISO15765_NORMAL
		||g_ulProtocolID==ISO15765_CAN_HWSET_DB
		||g_ulProtocolID==ISO15765_EXCEPT
		||g_ulProtocolID==ISO15765_CAN_HWSET_DB_SINGLE
		||g_ulProtocolID==ISO15765_ACU_SINGLE
		||g_ulProtocolID==ISO14229_UDS_HWSET_DB
		||g_ulProtocolID==ISO15765_CAN_HWSET_DB_NEW
#if defined (GSCAN3_AM)						  
		||g_ulProtocolID==ISO14229_ES95486_170_AM
		||g_ulProtocolID == ISO14229_ES95486_170_AM_EX_CAN_ID
#endif
		||g_ulProtocolID==ISO14229_ES95486_170
		||g_ulProtocolID==ISO14229_ES95486_170_MMCAN
		||g_ulProtocolID==ISO14230_ES95486_170
		||g_ulProtocolID==ISO14230_ES95486_170_MMCAN
		||(g_ulProtocolID == ISO14229_ES95486_02_100)
		||(g_ulProtocolID == ISO14229_ES95486_02_100_NEW)			//20190918 Jay
#ifdef HOTA
        ||(g_ulProtocolID == ISO14229_ES95486_02_HOTA)
#endif
#ifdef CANFD_PROTOCOL
        ||(g_ulProtocolID == ISO14229_ES95486_02_130_CANFD)
        ||(g_ulProtocolID == ISO14229_ES95486_02_131_CANFD)
#endif
		||(g_ulProtocolID == ISO14229_ES95486_02_102)
		||(g_ulProtocolID == ISO14229_ES95486_02_103)
		||(g_ulProtocolID == ISO14229_ES95486_02_104)
		||(g_ulProtocolID == ISO14229_ES95486_02_105)
		||(g_ulProtocolID == ISO14229_ES95486_02_106)
		||(g_ulProtocolID == ISO14229_ES95486_02_107)
		||(g_ulProtocolID == ISO14229_ES95486_02_108)
		||(g_ulProtocolID == ISO14229_ES95486_02_109)
		||(g_ulProtocolID == ISO14229_ES95486_02_10A)
		||(g_ulProtocolID == ISO14229_ES95486_02_10B)
		||(g_ulProtocolID == ISO14229_ES95486_02_10C)
		||(g_ulProtocolID == ISO14229_ES95486_02_10D)
		||(g_ulProtocolID == ISO14229_ES95486_02_10E)
		||(g_ulProtocolID == ISO14229_ES95486_02_10F)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_120)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_121)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_122)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_123)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_124)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_125)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_126)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_127)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_128)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_129)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_12A)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_12B)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_12C)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_12D)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_12E)
		||(g_ulProtocolID == ISO14230_ES95486_DOIP_12F)
		||g_ulProtocolID==ISO15765_SMK
        ||g_ulProtocolID==ISO15765_29BIT
//		||g_ulProtocolID==ISO15765_SMK_NEW
		||g_ulProtocolID==CAN_2){}//CAN¿œ ∞ÊøÏ ¡¶ø‹Ω√≈¥
	else osDelay(ms);
}

#if defined ( SAVE_CAN_LOG )
uint8_t VCI_SetCANLog( uint8_t mode, uint8_t* rec_packet, uint8_t* send_buff )
{  	
	DIR 		Dir;
	FILINFO 	Finfo;
	uint16_t 	i;
	UINT 	s1, s2;
	long 	p1;
	FATFS 	*fs;
	uint8_t cResult = 0;
	
  	
	switch( mode )
	{
	  	case 0:	//f_chdir
		  	cResult = f_chdir((char const*)&rec_packet[2]);
		  	break;
		  
		case 1: //f_mkdir
		  	cResult = f_mkdir((char const*)rec_packet[2]);
		  	break;
		  
		case 2:
		{
			cResult = f_opendir(&Dir, (char const*)&rec_packet[2]);
			memset(send_buff, 0x20, sizeof(send_buff));
			if(cResult ==0)
			{
				i=0;
				for(;;) 
				{
					cResult = f_readdir(&Dir, &Finfo);
					if ((cResult != FR_OK) || !Finfo.fname[0]) 
						break;
					if (Finfo.fattrib & AM_DIR) 
					{
						s2++;
					} 
					else 
					{
						s1++; p1 += Finfo.fsize;
					}
					sprintf((char*)&send_buff[1+(70*i)],"\r%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s",
					(Finfo.fattrib & AM_DIR) ? 'D' : '-',
					(Finfo.fattrib & AM_RDO) ? 'R' : '-',
					(Finfo.fattrib & AM_HID) ? 'H' : '-',
					(Finfo.fattrib & AM_SYS) ? 'S' : '-',
					(Finfo.fattrib & AM_ARC) ? 'A' : '-',
					(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
					(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
					Finfo.fsize, &(Finfo.fname[0]));   
					sprintf((char*)&send_buff[1+50+(70+i)],"\n"); 
					i++;
				}
				sprintf((char*)&send_buff[1+(70*i)],"\r\n%4u File(s),%10lu bytes total%4u Dir(s)", s1, p1, s2);  
			
				if (f_getfree((char const*)&send_buff[2], (DWORD*)&p1, &fs) == FR_OK)
					sprintf((char*)&send_buff[1+50+(70+i)],", %d Mbytes free\r\n", (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)512)>>20)); 
				i++;
				send_buff[1+(70*i)]=0x00;
				for(uint16_t j=0; j<2000; j++)
				{
				 	GLogN("%c", send_buff[j]);
				}
				cResult = 0xFF;
			}
		  	break;
		}
		case 3: //f_unlink
		{
		  	cResult = f_unlink((char const*)&rec_packet[2]);
		  	break;	
		}
		case 4:
		{
		  	rec_packet[2]=1;
		  	if(rec_packet[2]==1)		//Enable
			{
				CreateTempLogFile(0);
				g_ucCANLog_Enable = 1;
				GLogN("CAN Log Enable\r\n");
			}
        	cResult = 0;
		  	break;
		}
		case 5:
		{
//        	packet->mLen 		= 8 + 2 + g_stVehicleComm.usLogIndex;
//        	memcpy(&pPayloadPtcl[2], &g_stVehicleComm.ucCommLog[0], g_stVehicleComm.usLogIndex);
        	cResult = 0;
		  	break;
		}
		case 6:
		{
		  	cResult = 0;
		  	break;
		}
		case 7:
		{
		  	g_ucCANLog_Enable = 0;
			MakeCANLogFile();
			GLogN("CAN Log Made\r\n");
			cResult = 0;
			GLogN("CAN Log Disable\r\n");
		  	break;
		}
		case 8:
		{
		  	uint32_t readSize = 0;
		  	//SendLogFile(&send_buff[12], readSize, curSeqNo, maxSeqNo);
			SendLogFile(&send_buff[4], readSize);
			memcpy(&send_buff[0], &readSize, sizeof(readSize));
		  	break;
		}
	}
	
	return cResult;
}
#endif

void VCI_5bpsInit(void)
{
	U8  state=DLC_5BPS;
	U8  i;
	U32  RxCount=0;
//	U8  para[10],Id_Rx_buff[250], Rx_id_cnt=0;		//πÃªÁøÎ

	//U8 para[11] = {0x00};					// Protocol Type : ISO9141_BOSCH ø°º≠ ªÁøÎ
	U8  para[10], Id_Rx_buff[250], Rx_id_cnt=0;

	//U16 rxdata;
	U32	SyncTime=0;
	U8 TxBuff[2];
	//uint32_t temp; 

#ifdef CVA_PULSE_CONTROL
	U32 uiCVATxCMD = 0x0000;
	U8 ucTxResult = 0;
#endif

#if 1//ing
	if( g_ulProtocolID == KEYLESS_OMRON ) // By KSW 20050304//KEYLESS_OMRON		//∞≥πﬂ« ø‰
    {
        if( KeylessCodingOmron( ) == TRUE)
        {
            g_stGITSByteArray.NumOfBytes = 2;
            g_stGITSByteArray.BytePtr[0] = g_ucRxBuff[1];
            g_stGITSByteArray.BytePtr[1] = g_ucRxBuff[2];
            return;
        }
        else
        {
            g_stGITSByteArray.NumOfBytes = 0;
            return;
        }	
    }	
#endif
//	else if( ProtocolType == KEYLESS_SHORT_HMC) // By KSW 20050315//KEYLESS_short_HMC
//    {
//        if( KeylessCodingShort_HMC( ) == TRUE)
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 2;
//            SBYTE_ARRAYBuff.BytePtr[0] = RxBuff[1];
//            SBYTE_ARRAYBuff.BytePtr[1] = RxBuff[2];
//            return;
//        }
//        else
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 0;
//            return;
//        }	
//    }	
//	else if( ProtocolType == KEYLESS_BOSCH ) // By KSW 20050315//KEYLESS_bosch
//    {
//        if( KeylessCodingBosch( ) == TRUE)
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 2;
//            SBYTE_ARRAYBuff.BytePtr[0] = RxBuff[1];
//            SBYTE_ARRAYBuff.BytePtr[1] = RxBuff[2];
//            return;
//        }
//        else
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 0;
//            return;
//        }	
//    }
//    else if( ProtocolType == KEYLESS_SHINCHANG ) // By KSW 20050315//KEYLESS_shinchang
//    {
//        if( KeylessCodingSinChang( ) == TRUE)
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 2;
//            SBYTE_ARRAYBuff.BytePtr[0] = RxBuff[1];
//            SBYTE_ARRAYBuff.BytePtr[1] = RxBuff[2];
//            return;
//        }
//        else
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 0;
//            return;
//        }	
//    }
//    else if( ProtocolType == KEYLESS_SHINCHANG_HP ) // By AJJOOLEE 20070228//KEYLESS_HP WITH IMMO
//    {
//        if( KeylessCodingSinChang_HP( ) == TRUE)
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 2;
//            SBYTE_ARRAYBuff.BytePtr[0] = RxBuff[1];
//            SBYTE_ARRAYBuff.BytePtr[1] = RxBuff[2];
//            return;
//        }
//        else
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 0;
//            return;
//        }	
//    }    
//	else if( ProtocolType == KEYLESS_SHORT_KMC) // By KSW 20050315//KEYLESS_short_HMC
//    {
//        if( KeylessCodingShort_KMC( ) == TRUE)
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 2;
//            SBYTE_ARRAYBuff.BytePtr[0] = RxBuff[1];
//            SBYTE_ARRAYBuff.BytePtr[1] = RxBuff[2];
//            return;
//        }
//        else
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 0;
//            return;
//        }	
//    }
//	else if( ProtocolType == MILTYPE_EMS8) 
//    {
//		LineCheck(KlineCh);
//		if (Batt_Volt_Buf<1000) 				// channel high
//        {
//            SBYTE_ARRAYBuff.NumOfBytes = 0;
//            return;
//        }	
//        VCI_HwSetting(MILTYPE_EMS8);
//    }

	if( g_ulProtocolID == IS09141_2_ALLISON_ATM )
	{
		//UartPrintf(&U1,"\n IS09141_2_ALLISON_ATM");
		//DLC_Pulse_Set(KCh);    	
		
		//if(g_ucCVAdaptorFlag == 0) //VCI3 NO NEED CV adaptor
		{
			DLC_TX_5BPS(GetKlineSelect(),HIGH);
			osDelay(3000);
			DLC_TX_5BPS(GetKlineSelect(),LOW);  
			osDelay(800);
			DLC_TX_5BPS(GetKlineSelect(),HIGH);
			osDelay(800);		
			DLC_TX_5BPS(GetKlineSelect(),LOW);  
			osDelay(2000);	
			DLC_Serial_Set(GetKlineSelect());
			DLC_RX_BUFF_CLEAR();
			state = RX_SYNC;
			intTxdRxdCount = 1000;
		}
		/*
		else
		{
			DLC_TX_5BPS_ISO9141_2_ALLISON_ATM();		
			DLC_RX_BUFF_CLEAR();
			state = RX_SYNC;
			intTxdRxdCount = 1000;
		}
		*/
	}
	else if( g_ulProtocolID == DAIHATSU_NON_KWP )
	{
		//TxBuff[0] =SBYTE_ARRAYBuff.BytePtr[0];
		//UartPrintf(&U1,"[5BPS DAIHATSU_NON_KWP START]\n");
		intTxdRxdCount = 2000;
		while(1)
		{
			if( intTxdRxdCount == 0 ) break;
		}
		DLC_RX_BUFF_CLEAR();
		//uartDlcWriteBuff2(g_stGITSByteArray.BytePtr,1,0);
		transmitKL_ByteTime( GetKlineSelect(), g_stGITSByteArray.BytePtr, 1, 0);
		intTxdRxdCount = 1000;
		state = RX_SYNC;
		RxCount = 0;
		
	}    
    else
    {
		if( g_ulProtocolID == ISO9141_2_SyncTime )
			SyncTime = g_stGITSetConfig.nW4;

		DLC_RX_BUFF_CLEAR();

		
		//UartPrintf(&U1,"[5bps address : %x]\n",SBYTE_ARRAYBuff.BytePtr[0]);

#ifdef CVA_PULSE_CONTROL
		memset(para,0x00,sizeof(para));
		para[0] = CV_ADAPTOR_DATA_LEN;
		para[3] = (U8)Lline_Usage;
		para[4] = SyncTime;
		para[8] = SBYTE_ARRAYBuff.BytePtr[0];
		uiCVATxCMD= CMD_CVA_5BPS;
		if(ucTxResult = CV_Adator_CMD_TX(para, uiCVATxCMD, para[0]) == FALSE){	UartPrintf(&U1,"\r\n %04X CMD error %02X",uiCVATxCMD,ucTxResult); }
#endif
#ifndef CVA_PULSE_CONTROL
		//g_stGITSByteArray.BytePtr[0]=0x33;
		DLC_5Bps_Set(SyncTime,g_stGITSByteArray.BytePtr[0]);		
#endif

		intTxdRxdCount = 5000;
	}
	
	while(1)
	{
		switch(state)
		{
			case DLC_5BPS:
				 if(intTxdRxdCount == 0)
				 {
					g_stGITSByteArray.NumOfBytes = 0;
					//UartPrintf(&U1,"[5BPS NG]\n");
					return;
				 }

				DLC_RX_BUFF_CLEAR();		//tx«œ¥¬ pulse µµ rxø°º≠¥¬ data∑Œ ¿ŒΩƒµ«π«∑Œ πˆ∆€≈¨∏ÆæÓ « ø‰
				state = RX_SYNC;
				RxCount = 0;
				//UartPrintf(&U1,"[5BPS OK]\n");
				//DLC_Serial_Set(KCh);
				intTxdRxdCount = g_stGITSetConfig.nW1;
				
				if( g_ulProtocolID == ISO9141_2_5BPSTXONLY )
				{
					HAL_Delay(g_stGITSetConfig.nW4);
					return;
				}
				else if( g_ulProtocolID == TRW_AIRBAG1 )
				{
					g_stGITSByteArray.NumOfBytes = 2;
					g_stGITSByteArray.BytePtr[0] = 1;
					g_stGITSByteArray.BytePtr[1] = 2;
					HAL_Delay(g_stGITSetConfig.nW4);
					return;
				}
				else if( ( g_ulProtocolID == ISO9141_ZEXEL ) || ( g_ulProtocolID == SIEMENS_SIMPLEX_KMC ))
				{
					//UartPrintf("\n ISO9141_ZEXEL [5BPS check]\n");
					state = RX_DATA_CHECK;
				}
				else if( g_ulProtocolID == ISO9141_KIA_IMMO_5BPS )
				{
					g_stGITSByteArray.NumOfBytes = 2;
					g_stGITSByteArray.BytePtr[0] = 1;
					g_stGITSByteArray.BytePtr[1] = 2;
					return;
				}
				else if( g_ulProtocolID == MILTYPE_EMS8) 
			    {
					g_stGITSByteArray.NumOfBytes = 2;
					g_stGITSByteArray.BytePtr[0] = 1;
					g_stGITSByteArray.BytePtr[1] = 2;
					return;
			    }
				 break;

			case RX_SYNC:
				 if(intTxdRxdCount == 0)
				 {
				 	if( g_ulProtocolID == DAIHATSU_NON_KWP )
				 	{
			            g_stGITSByteArray.NumOfBytes = 2;
			            g_stGITSByteArray.BytePtr[0] = 1;
			            g_stGITSByteArray.BytePtr[1] = 2;
			            return;					 		
				 	}				 	
					g_stGITSByteArray.NumOfBytes = 0;
					//UartPrintf(&U1,"[RX SYNC NG]\n");
					return;
				 }
				 else
				 {
					//RxCount += uartDlcReadBuff((U8*)(RxBuff+RxCount),1);
					//temp=g_stGITSetConfig.nP3Min;	//for test
					//g_stGITSetConfig.nP3Min=60;		//for test
					if( g_stGITSetConfig.nP3Min == 0 )
					{
						RxCount += getKLReceiveData_Timeout(GetKlineSelect(),(U8*)(g_ucRxBuff+RxCount),1,intTxdRxdCount);
					}
					else
					{
						RxCount += getKLReceiveData(GetKlineSelect(),(U8*)(g_ucRxBuff+RxCount),1);
					}
					//g_stGITSetConfig.nP3Min=temp;	//for test
					if(g_ucRxBuff[0]==0) RxCount=0;		//130808 LWH √ﬂ∞°
					if( RxCount == 1 )
					{
						GLogI("[SYNC=0x%02X]\n\r", g_ucRxBuff[0]);
						state = RX_KEY1;
						intTxdRxdCount = g_stGITSetConfig.nW2;
					}
				 }		
				 break;
				 
			case RX_KEY1:
				 if(intTxdRxdCount == 0)
				 {
				 	if( g_ulProtocolID == DAIHATSU_NON_KWP )
				 	{
			            g_stGITSByteArray.NumOfBytes = 2;
			            g_stGITSByteArray.BytePtr[0] = 1;
			            g_stGITSByteArray.BytePtr[1] = 2;
			            return;					 		
				 	}				 	
					g_stGITSByteArray.NumOfBytes = 0;
					//UartPrintf("[RX KEY1 NG]\n");
					return;
				 }
				 else
				 {
					//RxCount += uartDlcReadBuff((U8*)(RxBuff+RxCount),1);
					RxCount += getKLReceiveData(GetKlineSelect(),(U8*)(g_ucRxBuff+RxCount),1);
					if( RxCount == 2 )
					{
						//UartPrintf("[KEY1=0x%02X]\n", RxBuff[1]);
						state = RX_KEY2;
						intTxdRxdCount = g_stGITSetConfig.nW3;
					}
				 }
				 break;
				 
			case RX_KEY2:
				 if(intTxdRxdCount == 0)
				 {
					g_stGITSByteArray.NumOfBytes = 0;
					//UartPrintf("[RX KEY2 NG]\n");
					return;
				 }
				 else
				{
					//RxCount += uartDlcReadBuff((U8*)(RxBuff+RxCount),1);
					RxCount += getKLReceiveData(GetKlineSelect(),(U8*)(g_ucRxBuff+RxCount),1);
					if( RxCount > 2 )
					{
						if( g_ulProtocolID == SIEMENS_DUPLEX )
						{
//								Delay(VP3_MIN);
							//Delay(5);
							g_stGITSByteArray.NumOfBytes = 2;
							g_stGITSByteArray.BytePtr[0] = g_ucRxBuff[1];
							g_stGITSByteArray.BytePtr[1] = g_ucRxBuff[2];
							return;
						}
						else if( g_ulProtocolID == IS09141_2_ALLISON_ATM  || g_ulProtocolID == WABCO_ABS )
						{
							osDelay( g_stGITSetConfig.nP3Min );
							if( g_ulProtocolID == WABCO_ABS ) g_uiAckTiming = 14;
							g_stGITSByteArray.NumOfBytes = 2;
							g_stGITSByteArray.BytePtr[0] = g_ucRxBuff[1];
							g_stGITSByteArray.BytePtr[1] = g_ucRxBuff[2];
							return;
						}							
						else if( g_ulProtocolID == DELPHI_AIRBAG )
						{
							g_uiAckTiming = 700;
							if( RxCount == 6 )
							{
								g_ucVFIVE_BAUD_MOD = 10;
								state = TX_KEY2;
								intTxdRxdCount = g_stGITSetConfig.nW4;
							}
						}		
						/*
						else if(( g_ulProtocolID == ISO9141_2_CV_4BYTE)&&( RxCount > 3 ))		//140207 LWH RX_KEY2ø°º≠ TX_KEY2 ∑Œ ∞°¡ˆ æ ∞Ì 1BYTE ¥ı ∏∂¿˙ πﬁ¥¬¥Ÿ.
						{
							HAL_Delay(VP3_MIN);
							g_stGITSByteArray.NumOfBytes = 3;
							g_stGITSByteArray.BytePtr[0] = RxBuff[1];
							g_stGITSByteArray.BytePtr[1] = RxBuff[2];
							g_stGITSByteArray.BytePtr[2] = RxBuff[3];
							return;								
						}
						*/
						else
						{
							state = TX_KEY2;
							intTxdRxdCount = g_stGITSetConfig.nW4;
						}
					}
				 }
				 break;
				 	 
			case TX_KEY2:
				 //if(uartDlcTxFlag == UART_BUFF_EMPTY)
				 {
					HAL_Delay(g_stGITSetConfig.nW4);
					TxBuff[0] = 0xFF-g_ucRxBuff[2];

					//if( g_ulProtocolID == ISO9141_2_DW_SIEMENSE )
					//{
					//	DlcRxBuff[105] = 0x00;
					//	state = BOSCH_RX_ID;
					//}
					if( g_ulProtocolID == DELPHI_AIRBAG )
					{
						TxBuff[0] = 0x79;
					}
					//uartDlcWriteBuff2(TxBuff,1,0);
					transmitKL_ByteTime( GetKlineSelect(), TxBuff, 1, 0);

					state = RX_INIT;
					if( g_ulProtocolID == ISO9141_BOSCH || g_ulProtocolID == ISO9141_2_DW_SIEMENSE)
						state = BOSCH_RX_ID;
					/*
					else if( g_ulProtocolID == ISO9141_BOSCH_AIRBAG )
					{
						Rx_Data_Time(1000, 1); // Tx Echo Data

						if( (rxdata = Rx_Data_Time(5000, 1)) == FAIL )
						{
							g_stGITSByteArray.NumOfBytes = 0;
							return;
						}
						if(rxdata==0x03)
						{							
							if( (rxdata = Rx_Data_Time(5000, 1)) == FAIL )
							{
								g_stGITSByteArray.NumOfBytes = 0;
								return;
							}														
						}
						
						RxBuff[0] = rxdata;
						
						
						for( i = 0; i < RxBuff[0]; i++ )
						{
							//if( (RxBuff[i+1] = Rx_Data_Time(1000, 1)) == FAIL )
							if( (rxdata = Rx_Data_Time(3000, 1)) == FAIL )
							{
								g_stGITSByteArray.NumOfBytes = 0;
								return;
							}
							else RxBuff[i+1] = rxdata;
						}
						DlcRxBuff[100] = RxBuff[1];
						
						HAL_Delay(VP3_MIN+20);
						g_stGITSByteArray.NumOfBytes = 2;
						g_stGITSByteArray.BytePtr[0] = RxBuff[0];
						g_stGITSByteArray.BytePtr[1] = RxBuff[1];
						return;
					}
					*/

					intTxdRxdCount = g_stGITSetConfig.nW4+50;
					if( g_ulProtocolID == ISO9141_2_LAN1 ) intTxdRxdCount = g_stGITSetConfig.nW4+1050;
				 }
				 break;
				 
			case RX_INIT:
				 if(intTxdRxdCount == 0)
				 {
					g_stGITSByteArray.NumOfBytes = 0;
					//UartPrintf("[RX INIT NG]\n");
					return;
				 }
				 else
				 {
					//RxCount += uartDlcReadBuff((U8*)(RxBuff+RxCount),1);
					RxCount += getKLReceiveData(GetKlineSelect(),(U8*)(g_ucRxBuff+RxCount),1);
					if( RxCount >= g_ucVFIVE_BAUD_MOD )
					{
						HAL_Delay(g_stGITSetConfig.nP3Min+20);
						//UartPrintf(&U1,"[INIT=0x%02X]\n", RxBuff[4]);
						g_stGITSByteArray.NumOfBytes = 2;
						g_stGITSByteArray.BytePtr[0] = g_ucRxBuff[1];
						g_stGITSByteArray.BytePtr[1] = g_ucRxBuff[2];
						return;
					}
				 }
				 break;
#if 1
			case BOSCH_RX_ID:		//∞≥πﬂ« ø‰
				 //uartDlcReadBuff((U8*)(RxBuff),1);
				 getKLReceiveData(GetKlineSelect(),(U8*)(g_ucRxBuff),1);
				 while(1)
				 {
//				 	if( g_ulProtocolID == ISO9141_2_DW_SIEMENSE )
//				 	{
//				 		DlcRxBuff[105] = 0;
//				 		Rx_Data_Time2(1000, 1);
//					 	if( DW_SiemensRxBlock() == FAIL )
//					 	{
//							SBYTE_ARRAYBuff.NumOfBytes = 0;
//							return;
//					 	}
//					 	else
//					 	{
//					 		//UartPrintf("\n rx ecu id");		//120507 LWH ∏∑¿Ω
//					 		for(i=0;i<RxBuff[0]+1;i++)		//id ªÁæÁ»Æ¿Œ¿ß«ÿ √ﬂ∞° ajjoolee
//					 		{
//					 			Id_Rx_buff[i]=RxBuff[i];
//					 			SBYTE_ARRAYBuff.BytePtr[i] = Id_Rx_buff[i];
//					 		}
//					 		SBYTE_ARRAYBuff.NumOfBytes = 20;
//					 		//for(i=0;i<RxBuff[0]+1;i++) UartPrintf(" %02X",Id_Rx_buff[i]);		//120507 LWH ∏∑¿Ω
//					 		return;
//					 	}	 		
//				 	}	
//				 	else
//				 	{
					 	if( getBoschRxBlock(GetKlineSelect(),g_ucRxBuff) == 0 )
					 	{
							g_stGITSByteArray.NumOfBytes = 0;
							return;
					 	}
					 	else
					 	{
					 		for(i=0;i<g_ucRxBuff[0]+1;i++)		//id ªÁæÁ»Æ¿Œ¿ß«ÿ √ﬂ∞° ajjoolee
					 		{
					 			Id_Rx_buff[i+(Rx_id_cnt*20)]=g_ucRxBuff[i];
					 		}
					 		Rx_id_cnt++;				
				 			if( Rx_id_cnt > 10 ) Rx_id_cnt = 10;
					 		if( g_ucRxBuff[2] == 0x09 )
					 		{
								g_stGITSByteArray.NumOfBytes = Rx_id_cnt*20;
								for(i=0;i<Rx_id_cnt*20;i++)
								{
									g_stGITSByteArray.BytePtr[i] = Id_Rx_buff[i];
								}
								//SBYTE_ARRAYBuff.BytePtr[1] = RxBuff[2];
								return;
					 		}
					 		else
					 		{
					 			para[0] = 0x03; para[1] = 0x00; para[2] = 0x09; para[3] = 0x03;
					 			if( transmitKL_ByteTime_bosch( GetKlineSelect(), &para[0], 4, g_stGITSetConfig.nP4Min) == false )
					 			{
									g_stGITSByteArray.NumOfBytes = 0;
									return;
					 			}
					 		}
					 	}
//				 	}
				 }
				 break;
#endif
//				 
//			case RX_DATA_CHECK:		 	
//			 	if( ProtocolType == ISO9141_ZEXEL )
//			 	{				 						 	
//				 	if( ZexelRxBlock() == FAIL )
//				 	{
//						SBYTE_ARRAYBuff.NumOfBytes = 0;
//						return;
//				 	}
//				 	else
//				 	{
//						SBYTE_ARRAYBuff.NumOfBytes = 2;
//						SBYTE_ARRAYBuff.BytePtr[0] = RxBuff[1];
//						SBYTE_ARRAYBuff.BytePtr[1] = RxBuff[2];			
//						return;
//				 	}
//				}
//			 	else if( ProtocolType == SIEMENS_SIMPLEX_KMC )
//			 	{
//				 	if( SiemensX2RxBlock() == FAIL )
//				 	{
//						SBYTE_ARRAYBuff.NumOfBytes = 0;
//						return;
//				 	}
//				 	else
//				 	{
//						SBYTE_ARRAYBuff.NumOfBytes = 2;
//						SBYTE_ARRAYBuff.BytePtr[0] = RxBuff[1];
//						SBYTE_ARRAYBuff.BytePtr[1] = RxBuff[2];			
//						return;
//				 	}
//				}			 		
//				break;
		}
	}
}
void DLC_TX_5BPS(U8 ChOutSel,U8 State)
{
	U8 para[11] = {0x00};
	//U8 ucTxResult = 0;
	//U32 uiCVATxCMD = 0x0000;

	memset(para,0x00,sizeof(para));	

	//if(g_ucCVAdaptorFlag == 0) //VCI3 NO NEED CV adaptor
	{
#if 1
		if(State == 1)	//		KLINE_INVERT();     //K-Line High
		{
			if(ChOutSel==KL_LINE1)   KL_TXD1_INV_DISABLE;
		    else    KL_TXD2_INV_DISABLE;
		}
		else	//			KLINE_NONINVERT();  //K-Line Low
		{
			if(ChOutSel==KL_LINE1)   KL_TXD1_INV_ENABLE;
		    else    KL_TXD2_INV_ENABLE;
		}
#else
		if(State == 1)	//		KLINE_INVERT();     //K-Line High
		{
			if(ChOutSel==KL_LINE1)   KL_TXD1_INV_ENABLE;
		    else    KL_TXD2_INV_ENABLE;
		}
		else	//			KLINE_NONINVERT();  //K-Line Low
		{
			if(ChOutSel==KL_LINE1)   KL_TXD1_INV_DISABLE;
		    else    KL_TXD2_INV_DISABLE;
		}
#endif
	}
#if 0 //VCI3 NO NEED CV adaptor
	else
	{
		if((ProtocolType == WABCO_ABS))										{	para[3] = CVA_CH_LLINE;		}
		else if((ProtocolType == ISO9141_2)||(ProtocolType == ISO9141_BOSCH))					{	para[3] = CVA_CH_KLINE;		}
		if(State == 1)
		{
			//L-Line High
			para[0] = CV_ADAPTOR_DATA_LEN;
			//para[3] = CVA_CH_LLINE;			// 01 : Kch 02 : Lch
			para[4] = CVA_PULSE_HIGH;		// 00 : Low 01 : High 
			uiCVATxCMD= CMD_CVA_PULSE_CONTROL;
			if(ucTxResult = CV_Adator_CMD_TX(para, uiCVATxCMD, para[0]) == FALSE){	UartPrintf(&U1,"\r\n %04X CMD error %02X",uiCVATxCMD,ucTxResult); }
		}
		else
		{
			//L-Line Low
			para[0] = CV_ADAPTOR_DATA_LEN;
			//para[3] = CVA_CH_LLINE;			// 01 : Kch 02 : Lch
			para[4] = CVA_PULSE_LOW;		// 00 : Low 01 : High 
			uiCVATxCMD= CMD_CVA_PULSE_CONTROL;
			if(ucTxResult = CV_Adator_CMD_TX(para, uiCVATxCMD, para[0]) == FALSE){	UartPrintf(&U1,"\r\n %04X CMD error %02X",uiCVATxCMD,ucTxResult); }
		}
	}
#endif
}
void DLC_RX_BUFF_CLEAR(void)
{
	clearKLReceiveData(KL_LINE1);//rx buffer clear	//KLINE
	clearKLReceiveData(KL_LINE2);//rx buffer clear	//KLINE
}
U8 GetKlineSelect(void)
{
	U8 ucLineSelect;

	// [group 1] : 1 2 3 6 7 8_1 13 15_1
	// [group 2] : 8 9 10 11 12 14 15
	
	if(g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH01 
		|| g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH02 
		|| g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH03 
		|| g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH06 
		|| g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH07
		|| g_stGITHWSetData.nKlineCh == KL_LINE1_CONNECT_CH13)   ucLineSelect=KL_LINE1;
    else    ucLineSelect=KL_LINE2;

	return ucLineSelect;
}
void DLC_5Bps_Set(U32 SyncTime,U8 intDlcOutData)
{
	U8 intDlcCount=0;
	
	DLC_TX_5BPS(GetKlineSelect(),LOW);
	HAL_Delay(200);
	
	while(intDlcCount!=9)
	{
		if(intDlcCount == 8 ) // End Bit
		{
			DLC_TX_5BPS(GetKlineSelect(),HIGH);
			intDlcCount++;
			
			if( SyncTime !=0 )  HAL_Delay(SyncTime);		// atoz egu4 50ms≥ª∑Œ sync code∞° ºˆΩ≈µ 
			else				HAL_Delay(200);
		}
		else
		{
			DLC_TX_5BPS(GetKlineSelect(),(intDlcOutData>>(intDlcCount))&0x1);
			intDlcCount++;
			HAL_Delay(200); 
		}
	}
}

void DLC_Pulse_Set(U8 ChOutSel)
{
    switch(ChOutSel)
    {
        case 0:
          KL_TXD1_INV_DISABLE; 
          KL_TXD2_INV_DISABLE;
        break;
        case 1:
          KL_TXD1_INV_DISABLE; 
        break;
        case 2:
          KL_TXD2_INV_DISABLE; 
        break;
    }
}

void DLC_Serial_Set(U8 ChOutSel)
{
    switch(ChOutSel)
    {
        case 0:
          KL_TXD1_INV_DISABLE; 
          KL_TXD2_INV_DISABLE; 
        break;
        case 1:
          KL_TXD1_INV_DISABLE; 
        break;
        case 2:
          KL_TXD2_INV_DISABLE; 
        break;
    }
}


void VCI_ACK(void)
{	
	stFdcanPkt	*txPkt, *rxPkt;
	stMsgClst	*txMsg, *rxMsg;
	
	uint32_t CanID;
	U8 i;
	//U8 ucLineSelect;
	uint8_t	rxBuff[200]	= { 0, };
	//U32 ret;
	//uint8_t		txBuff[100];
	osEvent		evt;
	uint32_t	uiP3min;
	unsigned long uiProtocolID = 0;

	//GLogN("[%s]\r\n", __FUNCTION__);
	//g_uiAckTiming=300;
	//GLogN("AckTime(%d)\r\n", g_uiAckTiming);
	//GLogN( "Ack\r\n" );
#if 1
#if 1
		if( g_bJ2534AckModeStatus == 0)				return;
		//ECU Upgradeø°º≠ Ack∏¶ «œ∏È æ»µ«¥¬µ• æÓ∂ª∞‘ Ω√∞£¿Ã º“ø‰µ«æÓ Ack Ω≈»£∞° ∞°¥¬ «ˆ «‘ºˆ∑Œ µÈæÓø¿¥¬ ∞ÊøÏ Ω«¡¶ 0x08¿ª πﬁ¡ˆ æ æ“¥¬µ•µµ 0x08¿ª 
		//πﬁ¿∫ ∞Õ √≥∑≥ ø¿¿ŒΩƒ«œø© µ•¿Ã≈Õ∏¶ ø¨º”¿∏∑Œ ¿¸º€«œø© PCø°º≠ ª∂≥≤.
#endif

	if( g_stGITHWSetData.ackmessage[0] == 0 ) return;
	//DB ACK≥ÎµÂø° æ∆π´∑± µ•¿Ã≈Õ∞° æ¯¿∏∏È RETURN √≥∏Æ. 111213 LWH

	g_bIsAckWorking=TRUE;
		
	uiProtocolID = VCI_GetPassThruProtocolID();
		
	if((VCI_ProtocolClassification()==eCAN_TX_NONE_PARSING)	//CAN
		&&(uiProtocolID!=ISO15765_ACU_SINGLE)
		)
	{
#if 1
		// Tx
		txMsg = ( stMsgClst* )osPoolCAlloc( hMsgPool );
		if( txMsg != NULL )
		{
			txPkt = ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
			if( txPkt != NULL )
			{
				if(g_bExtCanFlag==true)	CanID = ((U32)g_stGITHWSetData.ackmessage[1]<< 24)+((U32)g_stGITHWSetData.ackmessage[2]<<16)+((U32)g_stGITHWSetData.ackmessage[3]<< 8)+((U32)g_stGITHWSetData.ackmessage[4]);
				else					CanID = ((U16)g_stGITHWSetData.ackmessage[1]<< 8)+((U16)g_stGITHWSetData.ackmessage[2]);

				txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
				txPkt->pTarget		= &hfdcan1;
#else
				if(g_ucCAN_CH==2)
					txPkt->pTarget		= &hfdcan1;
				else//if(g_ucCAN_CH==1)
					txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif

				//for( i = 0; i < 8; i++ )			txBuff[i] = txPkt->mData[i] = pPayloadPtcl[i+3];
				for(i=0; i<8; i++)
				{
					if( CheckNewUDS(uiProtocolID) )	
                    {
                        txPkt->mData[i]=0x55;
					}
					else 
                    {
                        txPkt->mData[i]=0;
					}
				}
				if(g_bExtCanFlag==true)	memcpy(&txPkt->mData[0], &g_stGITHWSetData.ackmessage[5], g_stGITHWSetData.ackmessage[5]+1);
				else					memcpy(&txPkt->mData[0], &g_stGITHWSetData.ackmessage[3], g_stGITHWSetData.ackmessage[3]+1);
#if 0
				GLogN( "Ack Tx : %04X ", CanID );
				for( i = 0; i < 8; i++ )
				{
					GLogN( "%02X ", txPkt->mData[i] );
				}
				GLogN( "\r\n" );
#endif

				if(g_ucCanformat == CAN_FRAMEFORMAT_CLASSIC)
				{
					if(g_bExtCanFlag == CAN_IDTYPE_EXTENDED)	makeTxHeaderCAN( &txPkt->mTxHeader, CanID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
					else 										makeTxHeaderCAN( &txPkt->mTxHeader, CanID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
				}
				else
				{
					if(g_bExtCanFlag == CAN_IDTYPE_EXTENDED)	makeTxHeaderCAN( &txPkt->mTxHeader, CanID, CAN_IDTYPE_EXTENDED, txPkt->mLen, CAN_FRAMEFORMAT_FDCAN);
					else										makeTxHeaderCAN( &txPkt->mTxHeader, CanID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_FDCAN );
				}

				txMsg->pPacket	= (void *)txPkt;

				osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

				// RX
				//for( i = 0; i < 3; i++ )				// Max wait 3 seconds
				uiP3min=70;
				while(1)
				//for( i = 0; i < 2; i++ )
				{
					evt = osMessageGet( hFDRxMsg, uiP3min ); //TimeOut 70ms
					if( evt.status == osEventMessage )
					{
#ifdef PRINT_MESSAGE_ID
						printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
						//if(g_bAckflag==0)
						//{
						//	break;
						//}
						rxMsg	= ( stMsgClst * )evt.value.p;
						rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#if 0
						GLogN( "Ack Rx : %04X ", rxPkt->mRxHeader.Identifier );
						for( i = 0; i < 8; i++ )
						{
							GLogN( "%02X ", rxPkt->mData[i] );
						}
						GLogN( "\r\n" );
#endif						
						if((rxPkt->mData[1]==0x7F)&&(rxPkt->mData[3]==0x78)&&((rxPkt->mData[0]&0xF0)==0x00))
						{
					 		uiP3min = 6000;
					 		continue;
					 	}
						osPoolFree( hFdcanPktPool, (void *)rxPkt );
						osPoolFree( hMsgPool, (void *)rxMsg );

						break;
					}
					else break;	//no response
				}
			}
			else
			{
				GLogE( "Error... fail alloc Packet!!!\r\n" );
				osPoolFree( hMsgPool, (void *)rxMsg );
			}
		}
		
#endif
	}
	else if(VCI_ProtocolClassification()==eKLINE_TX_BLOCK)	//kline
	{
#if 1
	  	//unsigned int uiProtocolID = 0;
		
		//uiProtocolID = VCI_GetPassThruProtocolID();
		
		switch(uiProtocolID)
		{
			case WABCO_ABS:
			{
				if(transmitKL_ByteTime_wabco_abs( GetKlineSelect(), &g_stGITHWSetData.ackmessage[1], g_stGITHWSetData.ackmessage[0], g_stGITSetConfig.nP4Min))
				{
				  	osDelay(2);
					getWabcoAbsRxBlock(  GetKlineSelect(), rxBuff );
				}
				clearKLReceiveData(KL_LINE1);//rx buffer clear
				clearKLReceiveData(KL_LINE2);//rx buffer clear
				break;
			}
			case ISO9141_BOSCH:
			{
			  	if(transmitKL_ByteTime_bosch( GetKlineSelect(), &g_stGITHWSetData.ackmessage[1], g_stGITHWSetData.ackmessage[0], g_stGITSetConfig.nP4Min))
				{
				  	osDelay(2);
					getBoschRxBlock(GetKlineSelect(),rxBuff);
				}
				clearKLReceiveData(KL_LINE1);//rx buffer clear
				clearKLReceiveData(KL_LINE2);//rx buffer clear
			  	break;
			}
			default:
			{
				transmitKL_ByteTime( GetKlineSelect(), &g_stGITHWSetData.ackmessage[1], g_stGITHWSetData.ackmessage[0], g_stGITSetConfig.nP4Min);
				osDelay(1);

				clearKLReceiveData(KL_LINE1);//rx buffer clear
				clearKLReceiveData(KL_LINE2);//rx buffer clear

				//if(g_ulProtocolID == ISO9141_2)	//MOHAVE(HM)/2017/D3.0 S2/AHLS, If another command is sent before receiving a response after sending ACK, communication is cut off.
				{
					GetKlineDataTime(GetKlineSelect(),rxBuff);
				}

				break;
			}
		}
		//GLogN("tx :");
		//for(i=0; i<g_stGITHWSetData.ackmessage[0]; i++) GLogN(" %02X",g_stGITHWSetData.ackmessage[i+1]);
		//GLogN("\r\n");

#if 0
        U32 ret;
		GLogN("rxBuff2 :");
		for(i=0; i<ret; i++)
		{
			GLogN(" %02X",rxBuff[i]);
		}
		GLogN("\r\n");
#endif

#endif
	}
	else
	{
		intDlccomCount = 0;
		g_bAckflag = 0;
	}
	g_bIsAckWorking=FALSE;
#endif
}
void ETHERNET_LINE_CONTROL()
{
    VCI_Clear_DLC_HW();
	//SetKL_Line(KL_LINE1_CONNECT_CH08,0);
	//IO_CONTROL_HIGH( KL_TXD1_INV );
	DLC_HW_Set(KLLINE_RELAY,K_SERIAL,K_NORMAL,L_PULSE,L_NORMAL,0,L_PULSE_HIGH,RXD_OBD2,RXD_NORMAL,Pullup,K_Pullup_510,L_Pullup_510,0,KL_LINE1_CONNECT_CH08,L_ChOff,R_ChOff);
    return;
}
void RxCANID_Set(U16 CanTxId1, U16 CanTxId2)
{
  	uint16_t uiTxCANID = 0;
    
    g_usES95486_RxCANID = 0;
    g_us29bitES95486_RxCANID = 0;
	
	if((g_ulProtocolID == ISO14229_ES95486_02_100_NEW)||     //20190918 Jay
#ifdef HOTA
        (g_ulProtocolID == ISO14229_ES95486_02_HOTA)||
#endif
#ifdef CANFD_PROTOCOL
        (g_ulProtocolID == ISO14229_ES95486_02_130_CANFD)||
		(g_ulProtocolID == ISO14229_ES95486_02_131_CANFD)||
#endif
		(g_ulProtocolID == ISO14229_ES95486_02_100)||
		(g_ulProtocolID == ISO14229_ES95486_02_102)||
		(g_ulProtocolID == ISO14229_ES95486_02_103)||
		(g_ulProtocolID == ISO14229_ES95486_02_104)||
		(g_ulProtocolID == ISO14229_ES95486_02_105)||
		(g_ulProtocolID == ISO14229_ES95486_02_106)||
		(g_ulProtocolID == ISO14229_ES95486_02_107)||
		(g_ulProtocolID == ISO14229_ES95486_02_108)||
		(g_ulProtocolID == ISO14229_ES95486_02_109)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_120)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_121)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_122)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_123)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_124)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_125)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_126)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_127)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_128)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_129)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_12A)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_12B)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_12C)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_12D)||
		(g_ulProtocolID == ISO14230_ES95486_DOIP_12E)||
		(g_ulProtocolID == ISO14229_ES95486_170))
	{
		g_usES95486_RxCANID = (((U16)CanTxId1<< 8)+((U16)CanTxId2))+8;
		
		uiTxCANID = (((U16)CanTxId1<< 8)+((U16)CanTxId2));
		
		if( uiTxCANID == 0x07DF)
		{
			 g_bCanIdSwFilterEn = false;
		}
		else
		{
		 	 g_bCanIdSwFilterEn = true;
		}
	}
#ifdef NEW_29BIT_CAN
	else if (g_ulProtocolID == ISO14229_ES95486_02_10F ||
			 g_ulProtocolID == ISO14230_ES95486_DOIP_12F)
    {
        if ( (CanTxId1 >> 8) >= 0x10 )
        {
            #ifdef CANFD_PROTOCOL
            if((CanTxId1 == 0x18DB) && (CanTxId2 == 0x33F1))
            {
                g_bCanIdSwFilterEn = false;
            }
            else
            #endif
            {
                g_us29bitES95486_RxCANID = (((U32)CanTxId1<<16) + ((CanTxId2&0xFF)<<8) + (CanTxId2>>8));
                g_bCanIdSwFilterEn = true;
            }
        }
        else
        {
            g_usES95486_RxCANID = (((U16)CanTxId1<< 8)+((U16)CanTxId2))+8;
            if( CanTxId1 == 0x07DF)
            {
                 g_bCanIdSwFilterEn = false;
            }
            else
            {
                 g_bCanIdSwFilterEn = true;
            }
        }
    }
    else if((g_ulProtocolID == ISO15765_ES95486_29bit)
			|| (g_ulProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
            || (g_ulProtocolID == ISO15765_ES95486_135_29bit_CANFD)
            || (g_ulProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
			)
    {
#ifdef CANFD_PROTOCOL
        if((CanTxId1 == 0x18DB) && (CanTxId2 == 0x33F1))
        {
            g_bCanIdSwFilterEn = false;
        }
        else
#endif
        {
            g_us29bitES95486_RxCANID = (((U32)CanTxId1<<16) + ((CanTxId2&0xFF)<<8) + (CanTxId2>>8));
            g_bCanIdSwFilterEn = true;
            //g_bCanIdSwFilterEn = false;
        }
    }
#endif
    
    if((g_stGITSetConfig.nEtc4 != 0)||(g_stGITSetConfig.nEtc5 != 0))
    {
        g_bCanIdSwFilterEn = false;
    }
	
    return;
}
#if 1
U16 KeylessCodingOmron( void )  // By KSW 20050304
{
	U8 i;

	DLC_HW_Set(KLLINE_RELAY,K_PULSE,K_NORMAL,L_PULSE,L_INVERSE,K_PULSE_HIGH,L_PULSE_HIGH,RXD_OBD1,RXD_NORMAL,Pullup,K_Pullup_2k,L_Pullup_2k,0,g_stGITHWSetData.nKlineCh,g_stGITHWSetData.nLlineCh,R_ChOff);
	//uartSetBaudRate(UART_DLC,VDATA_RATE);
	uartSetBaudRate(GetKlineSelect(), g_stGITSetConfig.nDataRate );

	DLC_TX_5BPS(KL_LINE1,HIGH);
	HAL_Delay(1000);
	DLC_TX_5BPS(KL_LINE1,LOW);
	HAL_Delay(100);

	// START FORMAT
	for( i = 0; i < 5; i++ )
	{
		DLC_TX_5BPS(KL_LINE1,HIGH);
		CAN_uDelay(2000);
		DLC_TX_5BPS(KL_LINE1,LOW);
		CAN_uDelay(2000);
	}

	// ID CODE FORMAT
	for( i = 0; i < 16; i++ )
	{
		DLC_TX_5BPS(KL_LINE1,HIGH);
		CAN_uDelay(1300);
		DLC_TX_5BPS(KL_LINE1,LOW);
		CAN_uDelay(500);
		DLC_TX_5BPS(KL_LINE1,HIGH);
		CAN_uDelay(500);
		DLC_TX_5BPS(KL_LINE1,LOW);
		CAN_uDelay(1300);
	}

	// STOP FORMAT
	DLC_TX_5BPS(KL_LINE1,HIGH);
	CAN_uDelay(500);
	DLC_TX_5BPS(KL_LINE1,LOW);
	CAN_uDelay(1300);

	DLC_TX_5BPS(KL_LINE1,HIGH);
	HAL_Delay(240);

 //kkw 2014.11 ºˆ¡§« ø‰   if((rGPCDAT&0x2000)==0x2000 ) // status-high
 	if(KL_RXD_PULSE_READ()==1)
	{
		DLC_TX_5BPS(KL_LINE1,HIGH);
		HAL_Delay(1000);
		DLC_TX_5BPS(KL_LINE1,LOW);
		HAL_Delay(100);

		// START FORMAT
		for( i = 0; i < 5; i++ )
		{
			DLC_TX_5BPS(KL_LINE1,HIGH);
			CAN_uDelay(2000);
			DLC_TX_5BPS(KL_LINE1,LOW);
			CAN_uDelay(2000);
		}

		// ID CODE FORMAT
		for( i = 0; i < 16; i++ )
		{
			DLC_TX_5BPS(KL_LINE1,HIGH);
			CAN_uDelay(1300);
			DLC_TX_5BPS(KL_LINE1,LOW);
			CAN_uDelay(500);
			DLC_TX_5BPS(KL_LINE1,HIGH);
			CAN_uDelay(500);
			DLC_TX_5BPS(KL_LINE1,LOW);
			CAN_uDelay(1300);
		}

		// STOP FORMAT
		DLC_TX_5BPS(KL_LINE1,HIGH);
		CAN_uDelay(500);
		DLC_TX_5BPS(KL_LINE1,LOW);
		CAN_uDelay(1300);

		DLC_TX_5BPS(KL_LINE1,HIGH);
		HAL_Delay(240);
		
//kkw 2014.11 ºˆ¡§« ø‰	    if((rGPCDAT&0x2000)==0x2000 ) // status-high
		if(KL_RXD_PULSE_READ()==1)
        {
            return FAIL;
        }
	}
        HAL_Delay(197);
	DLC_TX_5BPS(KL_LINE1,LOW);
        return TRUE;
}
#endif
U16 KL_RXD_PULSE_READ()
{
	//KL_RXD1 PB15, PA6 
    //KL_RXD2 PD9, PH11
	//HAL_GPIO_ReadPin( KL_RXD1B15_GPIO_Port, KL_RXD1B15_Pin );
	//HAL_GPIO_ReadPin( KL_RXD1_GPIO_Port, KL_RXD1_Pin );
	//HAL_GPIO_ReadPin( KL_RXD2D9_GPIO_Port, KL_RXD2D9_Pin );
	//HAL_GPIO_ReadPin( KL_RXD2_GPIO_Port, KL_RXD2_Pin );
#if 1
	// 1st solution for real car //kkt
    if( GetKlineSelect() == KL_LINE1 )
    {
        if( !(HAL_GPIO_ReadPin( KL_RXD1B15_GPIO_Port, KL_RXD1B15_Pin ))) 	return 0;
		//if( !(HAL_GPIO_ReadPin( KL_RXD1_GPIO_Port, KL_RXD1_Pin ))) 	return 0;
        else 						            return 1;
    }
    else
    {
        if( (HAL_GPIO_ReadPin( KL_RXD2D9_GPIO_Port, KL_RXD2D9_Pin )))    return 1;
		//if( (HAL_GPIO_ReadPin( KL_RXD2_GPIO_Port, KL_RXD2_Pin )))    return 1;
        else                                      return 0;
    }
#else
	// 2nd solution for real car //kkt
	if( GetKlineSelect() == KL_LINE1 )
    {
        //if( !(HAL_GPIO_ReadPin( KL_RXD1B15_GPIO_Port, KL_RXD1B15_Pin ))) 	return 0;
		if( !(HAL_GPIO_ReadPin( KL_RXD1_GPIO_Port, KL_RXD1_Pin ))) 	return 0;
        else 						            return 1;
    }
    else
    {
        //if( (HAL_GPIO_ReadPin( KL_RXD2D9_GPIO_Port, KL_RXD2D9_Pin )))    return 1;
		if( (HAL_GPIO_ReadPin( KL_RXD2_GPIO_Port, KL_RXD2_Pin )))    return 1;
        else                                      return 0;
    }
#endif
}
U32 GitEtcFunction(U8 *Receive_Value, U8 *Return_Value)
{
	//U16 data1,i,k,l,m,p,temp,romsize,wait,usCanRxID16=0;
	//U8  check_sum=0,j,periodtime=0,ucRxDlc=0,ucRxData[8]={0,};
	//U32 rxdata,q;
    //bool bSameFlag=FALSE;
//	U16 ReadSize, ReadPos = 0;		//πÃªÁøÎ
#if 0 //need develop kkt
	if(Receive_Value[0]==0x00)	// «ˆ¥Î¡ﬂ∞¯æ˜ 7Ω√∏Æ¡Ó ∏Æ«¡∑Œ±◊∑• ø¿«¬ ƒ⁄µÂ
	{
		
		Return_Value[0]=0;	
		wait = ((U16)Receive_Value[1]<< 8)+((U16)Receive_Value[2]);
		//UartPrintf("wait %d \n",wait);
		
		intTxdRxdCount = 5000;
		
		for(q=0;q<wait;q++)
		{		
			Delay(5);		// 5ms ∞£∞›
			uartDlcWriteBuff2(&Receive_Value[0],1,VP4_MIN);
			rxdata = Rx_Data_Time2(50, 1);
			if(rxdata!=0x100)
			{
				Return_Value[0]=1;
				break;
			}
		}
		if(Return_Value[0]==1)	return 1;		
		else					return 0;
				
	}	
	if(Receive_Value[0]==0x06)	// 6π¯ ∑Œ¡˜ √ﬂ∞° SPS ccp º€ºˆΩ≈ ¿¿¥‰ º”µµ πÆ¡¶∑Œ √ﬂ∞°«‘.Receive_Value[1,2] -> DATA SIZE √≥∏Æ [3..] DATA
	{
		//UartPrintf("\n sps \n");
		Return_Value[0]=0;	
		wait = ((U16)Receive_Value[11]<< 8)+((U16)Receive_Value[12]);
		//UartPrintf("wait %d \n",wait);
		
		intTxdRxdCount = 5000;
		
		for(q=0;q<wait;q++)
		{		
			TxBuff[0]=8;
			CanTxID16 =((U16)Receive_Value[1]<< 8)+((U16)Receive_Value[2]);

			TxBuff[1]=(U8)((CanTxID16<<5)>>8)&0xFF;
			TxBuff[2]=(U8)((CanTxID16<<5))&0xFF;
			for( i = 0; i < 8; i++ )
				TxBuff[i+3] = Receive_Value[i+3];
			
			//CanTx_OutEn(TxBuff);
			Can1WriteBuff(TxBuff+3,CanTxID16, TxBuff[0]); 		//130530 LWH
				
			intTxdRxdCount = 5000;
			if(	Can1ReadBuff(DlcRxBuff,0) == TRUE )
			{
				Return_Value[0]=1;
				break;
			}
		}
		if(Return_Value[0]==1)	return 1;						// repro success flag '1'
		else					return 0;			
				
	}	
#endif
#if 0
    //else if(Receive_Value[0]==0x07)	// 7π¯ ∑Œ¡˜ √ﬂ∞° TL¬˜∑Æ
    if(Receive_Value[0]==0x07)	// 7π¯ ∑Œ¡˜ √ﬂ∞° TL¬˜∑Æ
	{
        //Receive_Value[0]: 0x07 
        //Receive_Value[1]:DLC (tx)
        //Receive_Value[2~3]:CanID (tx)
        //Receive_Value[4~5]:π›∫π»Ωºˆ(4∞° MSB)
        //Receive_Value[6]: Delay≈∏¿” (¿¸º€ ¡÷±‚) 0xF0~0xF9¥¬ microµÙ∑π¿Ã ±◊ø‹ø°¥¬ msµÙ∑π¿Ã(0x03 -> 3ms  ,0xF3 -> 300micro)
        //Receive_Value[7~14]: tx data 
        //Receive_Value[15]:DLC (rx)
        //Receive_Value[16~17]:CanID (rx)
        //Receive_Value[18~25]: rx data 
        ///////////////////////////////////////////////////////////////////////////////////////////////
        //Return_Value[0]:µ•¿Ã≈ÕºˆΩ≈ø©∫Œ 0 ≈∏¿”æ∆øÙ 1 ºˆΩ≈«ÿæﬂ«“ µ•¿Ã≈Õ∞° ∏¬¿Ω 2 ºˆΩ≈«ÿæﬂ«“ µ•¿Ã≈Õ∞° ∏¬¡ˆæ ¿Ω(π›∫π»Ωºˆµøæ»)
        //Return_Value[1]:DLC 
        //Return_Value[2~3]:CanID 
        //Return_Value[4~]:data
      
        memset(ucRxData, NULL, sizeof(ucRxData)/sizeof(ucRxData[0]));
		Return_Value[0]=0;	
		wait = ((U16)Receive_Value[4]<< 8)+((U16)Receive_Value[5]);
        periodtime=Receive_Value[6];
        ucRxDlc=Receive_Value[15];
        usCanRxID16=((U16)Receive_Value[16]<< 8)+((U16)Receive_Value[17]);
        for( i = 0; i < ucRxDlc; i++ )    ucRxData[i]=Receive_Value[i+18];
		//uartPrintf("wait %d \n",wait);
		
		for(q=0;q<wait;q++)
		{
			TxBuff[0]=Receive_Value[1];
			CanTxID16 =((U16)Receive_Value[2]<< 8)+((U16)Receive_Value[3]);

			TxBuff[1]=(U8)((CanTxID16<<5)>>8)&0xFF;
			TxBuff[2]=(U8)((CanTxID16<<5))&0xFF;
			for( i = 0; i < TxBuff[0]; i++ )
				TxBuff[i+3] = Receive_Value[i+7];
			
            if((periodtime>>4)==0x0F)		uDelay((periodtime&0x0F)*100);
            else 						 	Delay(periodtime);
            
			Can0WriteBuff(CAN_CH,TxBuff+3,CanTxID16, TxBuff[0]);
            
			if( CanReceive() == TRUE )
			{
				CanRxID16 = DlcRxBuff[1]*0x100 + DlcRxBuff[2];
			 	CanRxID16 = CanRxID16 >> 5;
                if((CanRxID16==usCanRxID16) && (ucRxDlc==DlcRxBuff[0])) //canIDøÕ Dlc∞° ø¯«œ¥¬∞‘ ∏¬¿ª∂ß
                {
                    for(i=0;i<DlcRxBuff[0];i++)
                    {
                        if(ucRxData[i]==DlcRxBuff[i+3])
                        {
                            bSameFlag=TRUE;
                            Return_Value[0]=1;
                        }
                        else
                        {
                            bSameFlag=FALSE;
                            Return_Value[0]=2;
                            break;
                        }
                    }
                }
                else Return_Value[0]=2;
			}
            
            if(bSameFlag==TRUE) break;
		//	uartPrintf("%d \n",q);
		}
        
        if((q>=wait) && (Return_Value[0]!=2)) Return_Value[0]=0;
        
        Return_Value[1]=DlcRxBuff[0];
        Return_Value[2]=(U8)((CanRxID16)>>8)&0xFF;
        Return_Value[3]=(U8)((CanRxID16))&0xFF;
        for(i=0;i<Return_Value[1];i++)   Return_Value[i+4]=DlcRxBuff[i+3];
        
		if(Return_Value[0]==1||Return_Value[0]==2)	return Return_Value[1]+4;						// repro success flag '1'
		else					                    return 0;			
				
	}
#endif
#if 0
	else if(Receive_Value[0]==0x05)	// KYC 20080530 5π¯ ∑Œ¡˜ √ﬂ∞° Receive_Value[1,2] -> DATA SIZE √≥∏Æ [3..] DATA
	{
		//UartPrintf("\n AUTOLIVE A_BAG REPROGRAM \n");
		
		i=0;p=0;temp=0;
		Return_Value[0]=0;		
		uartDlcWriteBuff2(&Receive_Value[1],11,VP4_MIN);
		if( (rxdata = Rx_Data_Time2(1000, 12)) == 0x100 ) return 1;
		
		if(rxdata!=0xAA) return 1;
		
		romsize=Receive_Value[12]*256+Receive_Value[13];
		
		k=romsize/16;		//
		l=romsize%16;		// 

		for(m=0;m<k;m++)
		{
			for(j=0;j<16;j++)
			{
				check_sum+=	Receive_Value[14+p];	
				p++;
			}
			
			uartDlcWriteBuff2(&Receive_Value[14+temp],16,VP4_MIN);
			if( (rxdata = Rx_Data_Time(1000, 17)) == 0x100 ) return 1;
			if(rxdata!=check_sum) return 1;	
			temp=p;	
		}
				
		for(m=0;m<l;m++)
		{
			check_sum+=	Receive_Value[14+p];	
			p++;	
		}
		uartDlcWriteBuff2(&Receive_Value[p+1],l,VP4_MIN);
		if( (rxdata = Rx_Data_Time(1000, l+1)) == 0x100 ) return 1;
		if(rxdata!=check_sum) return 1;			
		Return_Value[0]=1;						// repro success flag '1'
		return 1;			
	}	
	else if( (Receive_Value[0]==0x01) || 
	    (Receive_Value[0]==0x03) ||
	    (Receive_Value[0]==0x04)  )		//MANDO ABS REPROGRAM
	{
		Return_Value[0]=0;	
		//UartPrintf("\nOne_Byte_Rx\n");
		
		DLC_RX_BUFF_CLEAR();
		if( (data1 = Rx_Data_Time(VP3_MIN, 1)) == 0x100 ) return 1;
		
		if(data1!=0x48) return 1;
		
		if(Receive_Value[0]!=0x04) Delay(7);
		ScanTxSend(0x43);
		if( (rxdata = Rx_Data_Time(1000, 2)) == 0x100 ) return 1;
		if(rxdata!=0x31) return 1;
		
		if(Receive_Value[0]!=0x04) Delay(7);
		ScanTxSend(0x32);
		if( (rxdata = Rx_Data_Time(1000, 2)) == 0x100 ) return 1;
		if(rxdata!=0x44) return 1;
		
		if(Receive_Value[0]!=0x04) Delay(7);
		ScanTxSend(0x36);
		if( (rxdata = Rx_Data_Time(1000, 2)) == 0x100 ) return 1;
		
		if(Receive_Value[0]==0x03)
		{
			if(rxdata!=0x30) return 1;
		}
		else if(Receive_Value[0]==0x04) 
		{
			if(rxdata!=0x31) return 1;			
		}
		else
		{
			if(rxdata!=0x32) return 1;
		}		
		Delay(7);
		ScanTxSend(0x41);
		if( (rxdata = Rx_Data_Time(1000, 2)) == 0x100 ) return 1;
		if(rxdata!=0x68) return 1;			
			
		Return_Value[0]=1;
		return 1;						
	}
	else if(Receive_Value[0]==0x02)	//AUTOLIVE A_BAG REPROGRAM
	{
		//UartPrintf("\n AUTOLIVE A_BAG REPROGRAM \n");
		
		i=0;
		Return_Value[0]=0;		
		uartDlcWriteBuff2(&Receive_Value[2],11,VP4_MIN);
		if( (rxdata = Rx_Data_Time2(1000, 12)) == 0x100 ) return 1;
		if(rxdata!=0xAA) return 1;
		
		for(j=0;j<16;j++)
		{
			check_sum+=	Receive_Value[13+j];	
		}
		uartDlcWriteBuff2(&Receive_Value[13],16,VP4_MIN);
		if( (rxdata = Rx_Data_Time(1000, 17)) == 0x100 ) return 1;
		if(rxdata!=check_sum) return 1;		

		for(j=0;j<16;j++)
		{
			check_sum+=	Receive_Value[29+j];	
		}		
		uartDlcWriteBuff2(&Receive_Value[29],16,VP4_MIN);
		if( (rxdata = Rx_Data_Time(1000, 17)) == 0x100 ) return 1;
		if(rxdata!=check_sum) return 1;									
		i++;	
		
		for(j=0;j<16;j++)
		{
			check_sum+=	Receive_Value[29+i*16+j];	
		}					
		uartDlcWriteBuff2(&Receive_Value[29+i*16],16,VP4_MIN);
		if( (rxdata = Rx_Data_Time(1000, 17)) == 0x100 ) return 1;
		
		if(rxdata!=check_sum) return 1;
		i++;	
		
		for(j=0;j<16;j++)
		{
			check_sum+=	Receive_Value[29+i*16+j];	
		}				
		uartDlcWriteBuff2(&Receive_Value[29+i*16],16,VP4_MIN);
		if( (rxdata = Rx_Data_Time(1000, 17)) == 0x100 ) return 1;
		
		if(rxdata!=check_sum) return 1;		
		i++;	
		
		for(j=0;j<16;j++)
		{
			check_sum+=	Receive_Value[29+i*16+j];	
		}				
		uartDlcWriteBuff2(&Receive_Value[29+i*16],16,VP4_MIN);
		if( (rxdata = Rx_Data_Time(1000, 17)) == 0x100 ) return 1;
		
		if(rxdata!=check_sum) return 1;	
		i++;	
		
		for(j=0;j<16;j++)
		{
			check_sum+=	Receive_Value[29+i*16+j];	
		}			
		uartDlcWriteBuff2(&Receive_Value[29+i*16],16,VP4_MIN);
		if( (rxdata = Rx_Data_Time(1000, 17)) == 0x100 ) return 1;
		
		if(rxdata!=check_sum) return 1;	
		i++;		
		
		for(j=0;j<16;j++)
		{
			check_sum+=	Receive_Value[29+i*16+j];	
		}				
		uartDlcWriteBuff2(&Receive_Value[29+i*16],16,VP4_MIN);
		if( (rxdata = Rx_Data_Time(1000, 17)) == 0x100 ) return 1;
		
		if(rxdata!=check_sum) return 1;	
		i++;
		
		for(j=0;j<14;j++)
		{
			check_sum+=	Receive_Value[29+i*16+j];	
		}			
		uartDlcWriteBuff2(&Receive_Value[29+i*16],14,VP4_MIN);
		if( (rxdata = Rx_Data_Time(1000, 15)) == 0x100 ) return 1;
			
		if(rxdata!=check_sum) return 1;											
		Return_Value[0]=1;
		return 1;			
	}	
#endif
	return 11; //Return_Value¿« πˆ∆€ ≈©±‚∏¶ ∏Æ≈œ«œ∏È µ .
}
int FindSamePeriodicMsg(stPERIODIC_MSG_INFO* Info)
{
	U8 i, j = 0;
	
	for(i = 0; i < MAX_PERIODICMSG_CNT; i++)
	{
		if(stPeriodicMsgInfo[i].ucMode == true)
		{
			for(j= 0; j < 6; j++)
			{
				if(stPeriodicMsgInfo[i].ucMsg[j] != Info->ucMsg[j])
					 break;
			}
			
			if(j == 6)
				return i;
		} 
	}
	return -1;
}

int FindEmptyPeriodicMsgInfo(stPERIODIC_MSG_INFO* Info)
{
	U8 i = 0;
	
	for(i = 0; i < MAX_PERIODICMSG_CNT; i++)
	{
		if(stPeriodicMsgInfo[i].ucMode == false)
		{
			return i;
		}
	}
	return -1;
}

void ClearAllPeriodicMsg( void )
{
  	uint8_t i = 0;
  	
	for( i = 0; i < MAX_PERIODICMSG_CNT; i++)
	{
	 	GLogN("ucTimerIndex : %d \r\n", stPeriodicMsgInfo[i].ucTimerIndex);
		GLogN("memset size : %d \r\n", (sizeof(stPERIODIC_MSG_INFO)-1));
	  	StopSWTimer(stPeriodicMsgInfo[i].ucTimerIndex);
		memset(&stPeriodicMsgInfo[i].ucIndex, 0x00, (sizeof(stPERIODIC_MSG_INFO)-1));//all periodic stop
	}  
}

uint32_t g_ulPassthruOldTime = 0;

void StopFunctionalPeriodicMsg(void)
{
    uint8_t i = 0;
  	
	for( i = 0; i < MAX_PERIODICMSG_CNT; i++)
	{
        if( ((stPeriodicMsgInfo[i].ucMsg[0]&0x80) != 0x80)&&
            (stPeriodicMsgInfo[i].ucMsg[1] == 0x07)&& 
            (stPeriodicMsgInfo[i].ucMsg[2] == 0xDF) )
        {
            if( stPeriodicMsgInfo[i].ucMode != 0x03 )
            {
            
                StopSWTimer(stPeriodicMsgInfo[i].ucTimerIndex);
                g_ulPassthruOldTime = Get_Tmr();
            }
            break;
        }
	}  
}

void ContinueFunctionalPeriodicMsg(void)
{
    uint8_t i = 0;
    uint32_t ulDeltaTime = 0, ulTimerCount = 0;
    
  	
	for( i = 0; i < MAX_PERIODICMSG_CNT; i++)
	{
        if( ((stPeriodicMsgInfo[i].ucMsg[0]&0x80) != 0x80)&&
            (stPeriodicMsgInfo[i].ucMsg[1] == 0x07)&& 
            (stPeriodicMsgInfo[i].ucMsg[2] == 0xDF) )
        {
            if( stPeriodicMsgInfo[i].ucMode != 0x03 )
            {
                ulDeltaTime = Get_TmrDelta(Get_Tmr(), g_ulPassthruOldTime);
                ulTimerCount = Timer_GetTimerCount(stPeriodicMsgInfo[i].ucTimerIndex);
                
                
                if( ulTimerCount >= ulDeltaTime )
                {
                    Timer_SetTimerCount(stPeriodicMsgInfo[i].ucTimerIndex, (ulTimerCount - ulDeltaTime));
                }
                else
                {
                    Timer_SetTimerCount(stPeriodicMsgInfo[i].ucTimerIndex, 1);
                }
                
                ContinueSWTimer(stPeriodicMsgInfo[i].ucTimerIndex);
            }
            break;
        }
	}
}

void little_to_big_endian(U8 *ucData, U32 uiNum, U8 ucSize) 
{
    U8 ucTemp[4] = {0x00, };
    
    memcpy(ucTemp, &uiNum, ucSize);
    
    for(U8 i=ucSize; i>0; i--)
    {
        ucData[ucSize-i] = ucTemp[i-1];
    }
}

void vcicrc32(uint8_t *data, size_t length) 
{
    size_t i, j;

    for (i = 0; i < length; i++)
    {
        g_CRCval ^= data[i];  //Compute data with CRC value and XOR
        for (j = 0; j < 8; j++) 
        {  
            if (g_CRCval & 1)
            {
                g_CRCval = (g_CRCval >> 1) ^ CRC32_POLYNOMIAL;  //Applying a polynomial
            } 
            else
            {
                g_CRCval >>= 1;
            }
        }
    }
}