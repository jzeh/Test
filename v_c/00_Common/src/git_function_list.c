/*************************************************************
 * NOTE : git_function_list.c
 *      Function List
 * Author : Lee junho
 * Since : 2019.05.07
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "main.h"
#include "Usb_device.h"

#include "common.h"
#include "firmware.h"
#include "typedef.h"
#include "git_ioctl.h"
#include "sw_timer.h"
#include "buzzer.h"
#include "git_cli.h"

#include "git_pool.h"
#include "git_adc.h"
#include "git_protocol.h"
#include "led.h"
#include "git_rtc.h"
#include "git_can.h"
#include "git_rs9116.h"
#include "rsi_mqtt_client.h"
#include "git_mmc.h"
#include "git_pm.h"
#ifdef VCI3_DIAG
#include "git_eth.h"
#endif
#include "git_vci.h"
#include "git_hsm.h"
#include "git_HSM_Operations.h"	
#include "gpio.h"
#include "git_AesEncrypt.h"
#include "git_ioctl.h"
#include "git_global.h"

#include "git_function_list.h"
#include "git_PassthruDefines.h"
#include "git_OBDcomm.h"
#include "git_fsutil.h"
#include "git_mmc.h"
#include "git_kl.h"
#ifdef VCI3_DIAG
#include "git_mcp2518fd.h"
#endif
#ifdef VCI3_RECORD
#include "git_record_file.h"
#endif
#include "cmox_init.h"
#include "cmox_low_level.h"
#include "cmox_crypto.h"
#include "rng.h"
#ifdef LISTDIAG
#include "git_ListDiag.h"
extern osMessageQId hListDiagMsg;
#endif

#ifdef USE_RELAY_MOSA
#include "Git_BatteryRelayControl.h"
extern osMessageQId hBatRelayConMsg;
#endif
extern osMessageQId hCsacDiagMsg;
extern bool         g_bCanLogOnRxFlag;
bool 				g_bUsingKM = OFF;  // NEW HSM ONLY

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define	PRINT_FUCTIONLIST_DEBUG_MESSAGE						1
#define	MONITOR_RESPONS_PACKET_DATA							1

#define SSID_24												"iptime"
#define SSID_50												"iptime5G"

#define SERVER_IP_ADDRESS									0x0500A8C0						// 192.168.0.5
#define SERVER_IP_ADDRESS5									0x0600A8C0						// 192.168.0.6

#define	TEST_OK												0
#define	TEST_NG												1

//#define DEBUG_GIT_PTCL_LOG
#define VCI2_TEST											0
#define VCI1_TEST											0

#define RS9116VER_LEN										20
#define AES_KEY_SIZE                                        32
#define ECU_CODE_KEY_EXPECTED_CHECKSUM                      0x84BCU
#define ECU_CODE_KEY_EXPECTED_LEN                           256U

#define RS9116_RETRY_COUNT                                  5
#define HSM_GET_VER_MAX_RETRY								2

//#define VCI3_RECOVERY										1
/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
void FL_EMMC_Init(stCommPkt *pkt, uint32_t eInCommType );
void FL_GitReadSerial( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitGetCurrentVCIMode( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitGetCertifyVCI( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitPassThruDisconnect( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSetHWSetting( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitIsRecordingMode( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitPassThruIoctl( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitVciVersion( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitAutoVinconfig( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitCheckAutoVinData( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitGetFirmwareList( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitFWModeChange( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitFWModeSwitch( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitConfigFileOpen( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitConfigFileReceive( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitConfigFileClose( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitRcvFileOpen( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitFileReceive( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitRecordFileErase( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitReadRecordFileList( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitIsRecordingMode( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitRecordFileOpen( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitRecordFileSend( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitRecordFileClose( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_WriteSerial( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSetVCIInit(stCommPkt *pkt, uint32_t eInCommType );
void FL_GitGetBatteryVoltage(stCommPkt *pkt, uint32_t eInCommType );
void FL_EMMC_Directory( stCommPkt *pkt, uint32_t eInCommType );
void FL_Get_Cert_ExpirationPeriod( stCommPkt *pkt, uint32_t eInCommType );
void FL_Get_EMMC_File_Info( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitEtcFunction( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitFormatExMemory( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_DeviceReset(stCommPkt *pkt, uint32_t eInCommType );
void FL_NotSupport( uint32_t iden );

#ifdef VCI3_DIAG
void FL_Selftest_Factory(stCommPkt *pkt, uint32_t eInCommType );			//Diag
void FL_GitPassThruConnect( stCommPkt *pkt, uint32_t eInCommType );			//Diag
void FL_GitSetProtocolID( stCommPkt *pkt, uint32_t eInCommType );			//Diag
void FL_GitPassThruWriteMsgs( stCommPkt *pkt, uint32_t eInCommType );		//Diag
void FL_GitPassThruStopWriteMsgs( stCommPkt *pkt, uint32_t eInCommType );	//Diag
void FL_GitGetAutoVinData( stCommPkt *pkt, uint32_t eInCommType );			//Diag
void FL_GitSetBatteryMonitoring(stCommPkt *pkt, uint32_t eInCommType );
void FL_GitGetBatteryDataFrame( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitPassThruRetryReadMsgs( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSetCanidFilter(stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_FWUpdate_Start( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_FWUpdate_Recv( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_FWUpdate_RecvCDP( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_FWUpdate_Check( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_CV_Check( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_FWUpdate_End( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSetFirmwareList( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSetCANLog( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSendPingAck( stCommPkt *pkt, uint32_t eInCommType );
void Git_HSMinitial( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_CSAC20_Req( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_CSAC10_Req( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_ASK_Req( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_ASK_v2_Req( stCommPkt *pkt, uint32_t eInCommType  );
void FL_Git_CRL_GetDate( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_CRL_GetData( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_CRL_Restore( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_CRT_GetHolderRef( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_CRT_GetData( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_ECUCODE_KEY_Req( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_HSM_GetVersion( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_HSM_UpdateStart( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_HSM_State_Check( stCommPkt *pkt, uint32_t eInCommType  );
void FL_Git_HSM_Applet_Update( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitRcvFileClose( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSendFileOpen( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSendFileSend( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSendFileClose( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitFileErase( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitMakeDirectory( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_GetEncryptKey( stCommPkt *pkt, uint32_t eInCommType );
#ifdef CANFD_qhyek //Q_hyek CANFD
void FL_GitCanFdEnable( stCommPkt *pkt, uint32_t eInCommType );
#endif
//void FL_Git_HSM_UpdateProgress( stCommPkt *pkt, uint32_t eInCommType  );
#ifdef LISTDIAG
void FL_GitListDiag( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitListSensor( stCommPkt *pkt, uint32_t eInCommType );
#endif
#ifdef USE_RELAY_MOSA
void FL_GitBatRelayCon(stCommPkt *pkt, uint32_t eInCommType );
#endif
void FL_RS9116UpdateStart( stCommPkt *pkt, uint32_t eInCommType );
void FL_RS9116VersionCheck( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_GetPublicKey( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_StoreEncryptKey( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_GenRsaKeyPair( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitAMVersion( stCommPkt *pkt, uint32_t eInCommType );
void FL_DirectCAN( stCommPkt *pkt, uint32_t eInCommType );
void FL_DirectCANRead( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_FWUpdate_Recv4K( stCommPkt *pkt, uint32_t eInCommType );
void FL_Git_FWUpdate_Check_CRC32( stCommPkt *pkt, uint32_t eInCommType );

void KeyExchangeStart();
#endif


#ifdef VCI3_RECORD
void FL_GitSetEngineStopTiggerInfo( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitSetFunctionTypeReq( stCommPkt *pkt, uint32_t eInCommType );
#endif

void FL_GitWifiScanInfo( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitWifiSetInfo( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitWifiSetScaninfo( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitWifiGetScaninfo( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitWifiClearScaninfo( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitWifiConnectInfo( uint32_t eInCommType );
void FL_GitAskNextAction( uint32_t eInCommType );
void FL_ListSensor_Disable( stCommPkt *pkt, uint32_t eInCommType );
void FL_mqtt_to_websocket( stCommPkt *pkt, uint32_t eInCommType );
void FL_websocket_to_mqtt( stCommPkt *pkt, uint32_t eInCommType );
void FL_GitDevice_info( stCommPkt *pkt, uint32_t eInCommType );

extern int32_t bt_spp_transfer(uint8_t *data, uint16_t length);
extern int8_t LAN9371_SPI_ReadByte( uint16_t addr, uint8_t *rxd );
extern void VCI_FastInit(uint8_t* pData, uint32_t eInCommType);
extern void Get_AUTOVINData(uint8_t ucFlag);
extern void RxCANID_Set(U16 CanTxId1, U16 CanTxId2);
extern void ETHERNET_LINE_CONTROL();
extern int FindSamePeriodicMsg(stPERIODIC_MSG_INFO* Info);
extern int FindEmptyPeriodicMsgInfo(stPERIODIC_MSG_INFO* Info);
extern U32 GitEtcFunction(uint8_t *Receive_Value, uint8_t *Return_Value);
extern void TransmitFunction( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID );
extern void TransmitFunction_IT( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID );
extern uint8_t VciSendFileCloseBigName(void);
extern void RS9116FWUpdate(void);
extern void HSM_Update_Ack( bool state, int progress );

extern osPoolId		hDiagPool;
extern osPoolId		hPTPKPool;

extern osMessageQId	hWriteMsg;
extern osMessageQId	hDiagMsg;
extern uint8_t			g_ucCAN_CH;

extern stPERIODIC_MSG_INFO stPeriodicMsgInfo[MAX_PERIODICMSG_CNT];

extern BOOL			g_bAckflag;
extern BOOL			g_bJ2534AckModeStatus;
extern u32          intTxdRxdCount;

static rsi_bt_resp_get_local_name_t		local_name = {0};

extern uint32_t	    gPMFlag;
BOOL	            g_bIsFastInit = FALSE;
extern BOOL	        g_bIsAckWorking;
extern stSBYTE_ARRAY    g_stGITSByteArray;
extern uint32_t		g_ulProtocolID;
//uint8_t                  g_ucCanBuffClearFlag;	// functionID:1003 ���� CAN �۽� �� ����Ŭ���� ���� �÷���, DEFAULT : 0 (=����Ŭ���� ����) 190507 KKT
BOOL                g_bFastInit_Success = 0;
uint8_t				g_ucFastInitRetryCnt = 0;
extern unsigned int	g_uiCanReadMsgLength;
extern bool         g_bCF_TxComplete;// Ignore flow control out of sequence
uint8_t				g_ucVehicle_Current_Read = 0;
uint8_t             g_ucRecvPassThruWriteMsg = 0;
extern uint8_t      g_b1003Lock;
extern uint8_t      g_b3002Lock;
extern uint8_t      g_bUsbBtBlocked;
uint8_t				g_HSM_update_flag = 0;
#ifdef CANFD_qhyek //Q_hyek CANFD
BOOL				g_ucCanformat = CAN_FRAMEFORMAT_CLASSIC;
uint8_t				g_ucCanfd_dlc = 0;
#endif
uint32_t            g_u32CheckSumTemp;
extern uint32_t     g_u32UpdateFileSize;
extern uint8_t      g_ucDownloadFW_FileNo;
extern U32          g_CurLoc;
extern uint8_t      g_ucAES256_Key[32];
extern bool         g_bEncryptFlag;
extern bool         g_bExtCanFlag;
extern uint8_t		g_HSM_Timer;
extern SHSMUpdateAck	*pHSMAck;
U8 g_ucHSM_ErrorCnt = 0;
extern BOOL			g_bCanRxConcequtiveFrame;
extern bool			g_bHsmLogTxOn;
extern bool			g_bHsmLogRxOn;

bool                g_first_connect = true;
/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
stFunctionList gsFunctions[] =
{
	{ 0x1303, FL_GitReadSerial				},	//Common
	{ 0x0145, FL_GitGetCurrentVCIMode		},	//Common
	{ 0x0146, FL_GitGetCertifyVCI			},	//Common
	{ 0x1001, FL_GitPassThruDisconnect		},	//Common
	//{ 0x0245, FL_EMMC_Init				},
	{ 0x0245, FL_GitFWModeChange			},	//Common
	{ 0x1102, FL_GitSetHWSetting			},	//Common
	{ 0x1006, FL_GitPassThruIoctl			},	//Common
//#if VCI2_TEST
	{ 0x2210, FL_GitVciVersion				},	//Common
//#endif
	{ 0x1106, FL_GitAutoVinconfig			},	//Common
	{ 0x1107, FL_GitCheckAutoVinData		},	//Common
	{ 0x1200, FL_GitFWModeSwitch			},	//Common
	{ 0x1201, FL_GitConfigFileOpen			},	//Common
	{ 0x1202, FL_GitConfigFileReceive		},	//Common
	{ 0x1203, FL_GitConfigFileClose			},	//Common
	{ 0x1206, FL_GitRecordFileClose			},	//Common
	{ 0x1207, FL_GitRecordFileErase			},	//Common
	{ 0x1208, FL_GitReadRecordFileList		},	//Common
	{ 0x1209, FL_GitIsRecordingMode			},	//Common
	{ 0x120B, FL_GitRecordFileOpen			},	//Common
	{ 0x120C, FL_GitRecordFileSend			},	//Common
	{ 0x120D, FL_GitRecordFileClose			},	//Common
	{ 0x1309, FL_GitEtcFunction				},	//Common
	{ 0x0340, FL_GitGetFirmwareList			},	//Common
	{ 0x1302, FL_Git_WriteSerial			},	//Common
	{ 0x1308, FL_GitSetVCIInit				},	//Common
	{ 0x1320, FL_GitGetBatteryVoltage		},	//Common
    { 0x3001, FL_GitFormatExMemory			},	//Common
	{ 0x0475, FL_Git_DeviceReset			},	//Common
#ifdef VCI3_RECORD
	{ 0x1410, FL_GitSetEngineStopTiggerInfo	},	//Record
	{ 0x1420, FL_GitSetFunctionTypeReq		},	//Record
#endif
#ifdef VCI3_DIAG
	{ 0x1301, FL_Selftest_Factory			},	//Diag
	{ 0x1000, FL_GitPassThruConnect			},	//Diag
	{ 0x1321, FL_GitSetProtocolID			},	//Diag
	{ 0x1003, FL_GitPassThruWriteMsgs		},	//Diag
	{ 0x1100, FL_GitPassThruWriteMsgs		},	//Diag
	{ 0x1101, FL_GitPassThruStopWriteMsgs	},	//Diag
	{ 0x1105, FL_GitGetAutoVinData			},	//Diag
	{ 0x2314, FL_GitSetBatteryMonitoring	},	//Diag
	{ 0x2315, FL_GitGetBatteryDataFrame		},	//Diag
	{ 0x3002, FL_GitPassThruRetryReadMsgs	},	//Diag
	{ 0x4000, FL_GitSetCanidFilter			},	//Diag
	{ 0x0155, FL_Git_FWUpdate_Start			},	//Diag
	{ 0x0255, FL_Git_FWUpdate_Recv			},	//Diag
	{ 0x0256, FL_Git_FWUpdate_Recv4K		},	//Diag
	{ 0x0257, FL_Git_FWUpdate_RecvCDP		},	//Diag not use kkt
	{ 0x0455, FL_Git_FWUpdate_Check			},	//Diag
	{ 0x0456, FL_Git_FWUpdate_Check_CRC32   },	//Diag
	{ 0x0555, FL_Git_CV_Check				},	//Diag
	{ 0x0355, FL_Git_FWUpdate_End			},	//Diag
	{ 0x0341, FL_GitSetFirmwareList			},	//Diag
	{ 0x0342, FL_GitSetCANLog				},	//Diag
	{ 0xe010, FL_GitSendPingAck				},	//Diag
    { 0x120E, FL_EMMC_Directory				},	//Diag
    { 0x120F, FL_Get_Cert_ExpirationPeriod	},	//Diag
    { 0x130E, FL_Get_EMMC_File_Info     	},	//Diag
#ifdef CANFD_qhyek //Q_hyek CANFD
    { 0xE021, FL_GitCanFdEnable				},	//Diag
#endif
	{ 0x1306, Git_HSMinitial 				},	//Diag
#ifdef LISTDIAG
	{ 0x6000, FL_GitListDiag				},	//Diag
	{ 0x6005, FL_GitListSensor				},	//Diag
#endif
#ifdef USE_RELAY_MOSA
	{ 0x8000, FL_GitBatRelayCon				},	//Diag
#endif
	{ 0xE036, FL_RS9116UpdateStart			},	//Diag
	{ 0xE037, FL_RS9116VersionCheck			},	//Diag
	{ 0x1211, FL_Git_CSAC20_Req            	},	//HSM   //Diag
    { 0x1212, FL_Git_CSAC10_Req	            },	//HSM	//Diag
    { 0x1217, FL_Git_ASK_Req	            },	//HSM	//Diag
    { 0x1218, FL_Git_CRL_GetDate			},	//HSM	//Diag
	{ 0x1219, FL_Git_CRL_Restore			},	//HSM	//Diag
	{ 0x121A, FL_Git_CRT_GetHolderRef		},	//HSM	//Diag
	{ 0x121B, FL_Git_ECUCODE_KEY_Req		},	//HSM	//Diag
	{ 0x121C, FL_Git_HSM_GetVersion			},	//HSM	//Diag
	{ 0x121D, FL_Git_HSM_UpdateStart		},	//HSM	//Diag
    { 0x1250, FL_Git_ASK_v2_Req		        },	//HSM	//Diag
    //0x121E Function 
    { 0x121F, FL_Git_HSM_Applet_Update		},	//HSM
    { 0x1220, FL_Git_HSM_State_Check		},	//HSM
    { 0x1221, FL_Git_CRL_GetData			},	//HSM
    { 0x1222, FL_Git_CRT_GetData			},	//HSM
	{ 0x1230, FL_GitRcvFileOpen				},	//ܵ α׷ //Diag
	//{ 0x1231, FL_GitFileReceive			},	//ܵ α׷	//Diag
	{ 0x1232, FL_GitRcvFileClose			},	//ܵ α׷	//Diag
	{ 0x1233, FL_GitSendFileOpen			},	//ܵ α׷	//Diag
	{ 0x1234, FL_GitSendFileSend			},	//ܵ α׷	//Diag
	{ 0x1235, FL_GitSendFileClose			},	//ܵ α׷	//Diag
	{ 0x1236, FL_GitFileErase				},	//ܵ α׷	//Diag
	{ 0x1237, FL_GitMakeDirectory			},	//ܵ α׷	//Diag
#if ENCRYPT_PRJ
	{ 0x1238, FL_Git_GetEncryptKey			},	//VCI2 only
	{ 0x1239, FL_Git_GetPublicKey			},	//tara	//Diag
	{ 0x123A, FL_Git_StoreEncryptKey		},	//tara	//Diag
	{ 0x123B, FL_Git_GenRsaKeyPair			},	//tara	//Diag
#endif
	{ 0x1248, FL_DirectCAN					},
	{ 0x1249, FL_DirectCANRead				},
	//{ 0x121E, FL_Git_HSM_UpdateProgress		},//HSM
#endif
#if VCI1_TEST
	{ 0x2211, FL_GitAMVersion				},//vci1 test
#endif
    { 0x1701, FL_GitWifiScanInfo            },
	{ 0x1702, FL_GitWifiSetInfo			    },      
    { 0x1703, FL_GitWifiSetScaninfo         },
    { 0x1704, FL_GitWifiGetScaninfo         },
    { 0x1707, FL_GitWifiClearScaninfo       },
	{ 0x1710, FL_ListSensor_Disable       	},
	{ 0x1801, FL_mqtt_to_websocket			},
	{ 0x1802, FL_websocket_to_mqtt			},
	{ 0x1803, FL_GitDevice_info				}

    //{ 0x123C, KeyExchangeStart		},
};

#ifdef VCI3_RECORD
extern void FunctionID_0xD001( );
extern void FunctionID_0xD002( );
extern void FunctionID_0xD004( );
extern void FunctionID_0xD005( );
extern void FunctionID_0xD006( );
extern void FunctionID_0xD007( );
extern void FunctionID_0xD008( );
extern void FunctionID_0xD009( );
extern void FunctionID_0xD00A( );
extern void FunctionID_0xD00B( );
extern void FunctionID_0xD00C( );
extern void FunctionID_0xD00D( );
extern void FunctionID_0xD00E( );
extern void FunctionID_0xC025( );
extern void FunctionID_0xC053( );

stFunctionList gsTriggerFunctions[] =
{
	{0xD001, FunctionID_0xD001},
	{0xD002, FunctionID_0xD002},
	{0xD004, FunctionID_0xD004},
	{0xD005, FunctionID_0xD005},
	{0xD006, FunctionID_0xD006},
	{0xD007, FunctionID_0xD007},
	{0xD008, FunctionID_0xD008},
	{0xD009, FunctionID_0xD009},
	{0xD00A, FunctionID_0xD00A},
	{0xD00B, FunctionID_0xD00B},
	{0xD00C, FunctionID_0xD00C},
	{0xD00D, FunctionID_0xD00D},
	{0xD00E, FunctionID_0xD00E},
	{0xC025, FunctionID_0xC025},
	{0xC053, FunctionID_0xC053},
};

uint32_t	guiTriggerFuncCnt = sizeof( gsTriggerFunctions ) / sizeof( stFunctionList );
#endif

uint32_t	guiFuncCnt = sizeof( gsFunctions ) / sizeof( stFunctionList );
uint32_t 	g_uiRecFrameSize = 1000;
uint8_t 	g_ucCANLog_Enable = 0;
#if defined (USB_SPEED_TEST)
uint32_t	g_uiUSBRxInterval = 0;
uint32_t	g_uiUSBTxInterval = 0;
uint32_t 	g_uiUSBRx_Cnt = 0;
#endif

eLockState 	g_eLockStatus=eLOCK_STATE_UNLOCK;	// CDP�� ���?�ʿ����?������? �����غ�����

uint8_t Q_Key[] ={0x01, 0x05, 0x49, 0x89, 0x58, 0x50, 0x82, 0x12, 0x01, 0x05, 0x49, 0x89, 0x58, 0x50, 0x82, 0x12};
uint8_t CODE_Key[] ={0x56, 0x63, 0x69, 0x33, 0x2A, 0x65, 0x43, 0x75, 0x43, 0x6F, 0x64, 0x45, 0x5E, 0x4B, 0x65, 0x59};
int myrecvDeltaTime = 0;
/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
void FL_GitReadSerial( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	
#if VCI1_TEST
	GLogI( "!!!!!!!!!!!!!!!!!!!!!!!!!VCI1_TEST_Enable!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
	LOCK_SET_STATE(eLOCK_STATE_UNLOCK);
#endif
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	printSerialNumber();

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
		return;
	}

	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 8;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1303;

#if	0
	gsFwInfo.marrucSerialNo[0]='C';
	gsFwInfo.marrucSerialNo[1]='V';
	gsFwInfo.marrucSerialNo[2]='1';
	gsFwInfo.marrucSerialNo[3]='1';
	gsFwInfo.marrucSerialNo[4]='1';
	gsFwInfo.marrucSerialNo[5]='1';
	gsFwInfo.marrucSerialNo[6]='1';
	gsFwInfo.marrucSerialNo[7]='1';
	gsFwInfo.marrucSerialNo[8]='\0';
#endif

	memcpy( packet->mData, gsFwInfo.marrucSerialNo, SERIAL_NUMBER_SIZE );

	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitGetCurrentVCIMode( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	printFWVersion();

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
		return;
	}

	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

#if VCI2_TEST
	GLogI( "~~~~~~~~~~~~~~~~~~~VCI2_TEST_Enable!!!~~~~~~~~~~~~~~~~\r\n");
	packet->mLen	= 7;
	packet->mData[0]= 1;	    //Bootloader : 1  Diagnosis : 2  Veryfication : 3  Reprogram : 4
	packet->mData[1]= 2;	    //GDS VCI :1  VCI-II :2  new DLogger :0x10	ADAS_DRV 2 :0x11  VCI3 :0x12   VCI2+CV :0x04
	sprintf((char*)&packet->mData[2],"%02d.%02d",2,99);
#else
	packet->mLen	= 7;
	packet->mData[0]= GetCurFwServiceMode(); //Bootloader : 1  Diagnosis : 2  Veryfication : 3  Reprogram : 4
	packet->mData[1]= 0x12;	    //GDS VCI :1  VCI-II :2  new DLogger :0x10	ADAS_DRV 2 :0x11  VCI3 :0x12   CDP :0x??
	sprintf((char*)&packet->mData[2],"%02d.%02d",gsFwInfo.marrucTotalVersion[0], gsFwInfo.marrucTotalVersion[1]);
//	sprintf((char*)&packet->mData[2],"%02d.%02d",99, 99);//for test
#endif

	GLogI( "CurrentMode : (0x%02X)\r\n", packet->mData[0] );

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0145;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitGetCertifyVCI( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    aes256_context ctx;
	uint8_t ucVciSerial[8]="XXXXXXXX";
#if VCI2_TEST
	uint8_t uckey[32] = "RunGitautoVCIIIAuthorityPP000001";
#else
    uint8_t uckey[32] = "RunGit___VCIIIIAuthorityPP000001";
#endif
	uint8_t ucInputbuf[19]={0,};
	uint8_t i;
	uint8_t *pPayloadPtcl = pkt->mData;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	memcpy(ucVciSerial, gsFwInfo.marrucSerialNo, SERIAL_NUMBER_SIZE );

	for(i=0; i<8; i++)
	{
		if(ucVciSerial[i]== 0xFF)  	ucVciSerial[i]=0x58; //?��?�정보�? ????'X'�??�신???
	}
	memcpy(ucInputbuf, (void const*)&pPayloadPtcl[1], 19);
	memcpy(&uckey[24],ucVciSerial,sizeof(ucVciSerial));

//    GLogI("\r\n ucInputbuf:");
//    for(uint8_t ttt=0; ttt<16; ttt++) GLogI(" %02X",ucInputbuf[ttt]);

    aes256_init(&ctx, uckey);
    aes256_decrypt_ecb(&ctx, ucInputbuf);

//    GLogI("\r\n decryptbuf:");
//    GLogI("%s\r\n", ucInputbuf);
//    for(uint8_t ttt=0; ttt<16; ttt++) GLogI(" %02X",ucInputbuf[ttt]);
#if VCI2_TEST
	if( strncmp((char const*)ucInputbuf, "GitVCIII",8)==0)
	{
		if( strncmp((char const*)&ucInputbuf[8], (char const*)ucVciSerial,8)==0)
		{
			LOCK_SET_STATE(eLOCK_STATE_UNLOCK);
#if defined(DEBUG_GIT_PTCL_LOG)
    		GLogI(" g_ucLockStatus is unlock\r\n");
#endif
		}
        else
        {
    		GLogE(" %s is Fail!!\r\n", __FUNCTION__ );
        }
	}
#else
    if( strncmp((char const*)ucInputbuf, "GitVCIIII",9)==0)
	{
	if( strncmp((char const*)&ucInputbuf[9], (char const*)ucVciSerial,8)==0)
	{
		LOCK_SET_STATE(eLOCK_STATE_UNLOCK);
#if defined(DEBUG_GIT_PTCL_LOG)
		GLogI(" g_ucLockStatus is unlock\r\n");
#endif
	}
    else
    {
		GLogE(" %s is Fail!!\r\n", __FUNCTION__ );
    }
}
#endif
    else
    {
    	GLogE(" %s is Fail!!\r\n", __FUNCTION__ );
    }

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
		return;
	}

	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;
    LOCK_SET_STATE(eLOCK_STATE_UNLOCK);
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0146;

	packet->mData[0]= LOCK_GET_STATE()%2;	//0 : OK 1: FAIL

	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitPassThruDisconnect( stCommPkt *pkt, uint32_t eInCommType  )
{
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif
	g_bAckflag = 0;
    g_ucCanformat = CAN_FRAMEFORMAT_CLASSIC;
	
	VCI_REINTI_COMM_STATE(VCI_GetPassThruProtocolID());
	VCI_Clear_DLC_HW();

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
    if ( eInCommType != PACKET_INTER_ANALYSIS )
#endif    
	LED_GREEN_ON;
#if defined ( SAVE_CAN_LOG )
//	if ((g_ucCANLog_Enable == 1) && (GetCurFwServiceMode() == eApp_VCI_2))
//	{
//		MakeCANLogFile();
//		g_ucCANLog_Enable = 0;
//	}
#endif
	//osSemaphoreWait(hpairflagSemaphore, osWaitForever);
}

void FL_GitSetHWSetting( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	uint8_t		*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	
	if(g_ListSensor_Endflag == true)
	{
		if (g_OBD_Processing == true)
		{
			g_ListSensor_Endflag = false;

			/* [1] ListDiagThread stop wait */
			osSignalWait(OBD_ListDiag_END, osWaitForever);

			/* [2] Queue drain - wait until all queued ListSensor responses are transmitted */
			while(osMessageAvailableSpace(hTransmitMsg) < MESSAGE_TRANSMIT_QUEUE_SIZE)
			{
				osDelay(10);
			}

			/* [3] Semaphore reset - clear residual count from ListSensor, reinit to 1 */
			while(osSemaphoreWait(hpairflagSemaphore, 0) > 0) {}
			osSemaphoreRelease(hpairflagSemaphore);
		}
	}
	
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	g_bAckflag = 0;

	VCI_Clear_DLC_HW();
	VCI_HWSetParameter(pPayloadPtcl);
	VCI_SetSConfigList(pPayloadPtcl+sizeof(stRECORD_HW_SET), TRUE);

	packet->mData[0] = 0x00;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1102;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_GitPassThruConnect( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	uint8_t 	*pPayloadPtcl = pkt->mData;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	g_bAckflag=0;
	g_ucVehicle_Current_Read = 0;
	g_bJ2534AckModeStatus = 1;

#ifdef CANFD_qhyek //Q_hyek CANFD
	//g_bCanformat = 0;// 0:CANFD, 1:CAN
#endif

	VCI_Clear_DLC_HW();
    VCI_SetPassThruProtocolID(pPayloadPtcl);
	VCI_FAST_Init_Delay(1000);
	VCI_SetPassThruConnectFlags(pPayloadPtcl+4);
	VCI_REINTI_COMM_STATE(VCI_GetPassThruProtocolID());
	VCI_HW_Setting(VCI_GetPassThruProtocolID());
	VCI_SetRtcTime(pPayloadPtcl+8);
#if defined ( SAVE_CAN_LOG )
	if ((g_ucCANLog_Enable == 0) && (GetCurFwServiceMode() == eApp_VCI_2))
	{
		CreateTempLogFile(g_ucTempFileCnt);
		g_ucCANLog_Enable = 1;
	}

#endif
	//test
	//g_ulProtocolID=0x100;
#if 1//defined(DEBUG_GIT_PTCL_LOG)
		GLogI(" ProtocolID 0x%X, g_ulProtocolFlag 0x%X\r\n", VCI_GetPassThruProtocolID(), VCI_GetPassThruConnectFlags());
#endif

	g_bAckflag=0;

	packet->mData[0] = 0x00;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1000;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);
	
	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_GitSetProtocolID( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	uint8_t *pPayloadPtcl = pkt->mData;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	VCI_SetPassThruProtocolID(pPayloadPtcl);
#if 0
	if(g_ulProtocolID == 0x300)
	{
	  	uint32_t CanID = 0;
		extern stRECORD_HW_SET		g_stGITHWSetData;
		VCI_Clear_DLC_HW();
	  	g_ulProtocolID = ISO15765_CARB;
		CanID = (((U16)g_stGITHWSetData.ackmessage[1]<< 8)+((U16)g_stGITHWSetData.ackmessage[2])+8);
		CAN_Initial_CH(g_ucCAN_CH, Highcan1, CAN_500KBPS, 1, 0,	STANDARD_CAN, 1, &CanID, &CanID);
		//Oem_CAN_Channel_Masket_Set(1, 1, 1, &CanID, &CanID);
	  	GLogN(" ProtocolID Changed 0x%X\r\n", g_ulProtocolID);
	}
#endif
#if 1//defined(DEBUG_GIT_PTCL_LOG)
	GLogN(" ProtocolID 0x%X\r\n", VCI_GetPassThruProtocolID());
#endif

	packet->mLen	= 0;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1321;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;
	
	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_GitPassThruWriteMsgs( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;
	uint8_t *pPayloadPtcl = pkt->mData;
	U32 delay;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
	GLogI("*");
	g_ucRecvPassThruWriteMsg++;
	g_bIsFastInit=FALSE;//variable init
	g_bCF_TxComplete=0;
	g_bCanRxConcequtiveFrame = FALSE;
	if(VCI_ProtocolClassification()==eCAN_TX_NONE_PARSING)	//CAN
	{
		if(((g_stGITSetConfig.nEtc3&0x0800)==0x0800)//DEFAULT : ENABLE (clear) 190507 KKT
			||(g_ulProtocolID == J1939_4PGN)||(g_ulProtocolID == J1939_AM)||(g_ulProtocolID == J1939)||(g_ulProtocolID == ISO15765_29BIT)||(g_ulProtocolID == ISO15765_29BIT_EXCEPT))
		{
			//DB processing completed -> 0x06 clear 190507 KKT
			//Initialize buffer for CAN message processing 190226 KKT
		}
		else
		{
			clearRXCanMessage();	//CAN
		}

		if(g_bIsAckWorking==TRUE)
		{
			GLogI("for ack processing osDelay 70ms");
			osDelay(70);
		}
	}
	else	//kline
	{
		if( g_ulProtocolID == WABCO_ABS) {}
		else if(g_bIsAckWorking==TRUE)//MOHAVE(HM)/2017/D3.0 S2/AHLS, If another command is sent before receiving a response after sending ACK, communication is cut off.
		{
			delay=g_stGITSetConfig.nP3Min+200;
			osDelay(delay);
			GLogI("for ack processing osDelay %dms",delay);

		}
		if( g_ulProtocolID == WABCO_ABS) {}
		else DLC_RX_BUFF_CLEAR();
	}

	g_bIsAckWorking=FALSE;
    
	if((g_ulProtocolID == ISO14229_ES95486_02_10F) || (g_ulProtocolID == ISO14230_ES95486_DOIP_12F))
	{
		if (pPayloadPtcl[28] >= 0x10)
		{
			RxCANID_Set((pPayloadPtcl[28]<<8)+pPayloadPtcl[29],(pPayloadPtcl[30]<<8)+pPayloadPtcl[31]);
		}
		else
		{
			RxCANID_Set(pPayloadPtcl[28],pPayloadPtcl[29]);
		}
	}
#ifdef NEW_29BIT_CAN
    else if((g_ulProtocolID == ISO15765_ES95486_29bit)
		||(g_ulProtocolID == ISO15765_ES95486_DOIP_29BIT)
#ifdef CANFD_PROTOCOL
        || (g_ulProtocolID == ISO15765_ES95486_135_29bit_CANFD)
        || (g_ulProtocolID == ISO15765_ES95486_136_29bit_CANFD)
#endif
          
		) RxCANID_Set((pPayloadPtcl[28]<<8)+pPayloadPtcl[29],(pPayloadPtcl[30]<<8)+pPayloadPtcl[31]);
    else
#endif
    {
        RxCANID_Set(pPayloadPtcl[28],pPayloadPtcl[29]);	//TX CAN ID +8 ?? ????? 20220409 KKT
    }
#if 0
	GLogI("CANSend :");
	for( int jj=0;jj<10;jj++)
	{
		GLogI("%02X ",pPayloadPtcl[28+jj]);
	}
	GLogI("\r\n");
	GLogI("usLength:%d\r\n",usLength);
//	if( pPayloadPtcl[28] == 0x07 && pPayloadPtcl[29] == 0xE7 && pPayloadPtcl[30] == 0x02 && pPayloadPtcl[31] == 0x10 && pPayloadPtcl[32] == 0x03 )
	if( pPayloadPtcl[28] == 0x07 && pPayloadPtcl[29] == 0xE7 && pPayloadPtcl[30] == 0x02 && pPayloadPtcl[31] == 0x10 && pPayloadPtcl[32] == 0x03 )
	{
		GLogI("\r\n");
	}
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	if( pPayloadPtcl != NULL)
	{
		VCI_SetPassThruWirteMsgTimeout(pPayloadPtcl, sizeof(unsigned long));

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

		memcpy(pPTpacket, pPayloadPtcl+4, sizeof(PTmsgPkt_t));
		memcpy(&(pPTpacket->UUID), &(pkt->UUID), sizeof(UUID_Struct));

#ifdef CANFD_qhyek //Q_hyek CANFD
        /* To sub CAN ID at CAN DLC */
        if(g_bExtCanFlag == CAN_IDTYPE_EXTENDED) g_ucCanfd_dlc = (pPTpacket->DataSize)-4; // CAN_IDTYPE_EXTENDED
        else g_ucCanfd_dlc = (pPTpacket->DataSize)-2; //CAN_IDTYPE_STANDARD
#endif

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

			printf("%s] error message full\r\n", __func__);
		}
		else
		{
			// ISO15765_REPRO_PENNIMG pc���� 1003 ó���� 1003�� ������ ���?�����?�̻����� �׷��Ƿ� �ڿ� ������ ���ɸ� ó��, PC���� ���������� ����ϵ���?���� �ʿ�
			if(VCI_GetPassThruProtocolID() == ISO15765_REPRO_PENNIMG || VCI_GetPassThruProtocolID() == ISO15765_29BIT)
			{
				if( g_b1003Lock == true )
				{
					for(int i=0;i<(g_stGITSetConfig.nP3Min/10);i++)
					{
						ClearRxCanState();
						ClearTxCanState();
						ClearDiagMessage();
						osDelay(10);
					}
				}
			}
			osMessagePut( hDiagMsg, (uint32_t)pDiagmsg, osWaitForever );
		}
	}

	if( VCI_GetPassThruProtocolID() == ISO15765_REPRO_PENNIMG || VCI_GetPassThruProtocolID() == ISO15765_29BIT )
	{
		g_b1003Lock = true;
	}

	packet->mLen	= 0;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1003;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;
#if 1
	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
#else
	uiProtocolID = VCI_GetPassThruProtocolID();
	if((uiProtocolID != ISO9141_2) &&
		 (uiProtocolID != ISO9141_2_SyncTime) &&
		 (uiProtocolID != ISO14230_2)&&
		 (uiProtocolID != ISO14230_ETC)&&
		 (uiProtocolID != ISO9141_2_DW_ABS)&&
		 (uiProtocolID != ISO9141_2_5BPSTXONLY)&&
		 (uiProtocolID != ISO9141) &&
		 (uiProtocolID != ISO14230) &&
		 (uiProtocolID != ISO14230_POWERTEC)&&
		 (uiProtocolID !=ISO14230_LLINE_LOW)&&
		 (uiProtocolID !=ISO9141_2_NAG)&&
		 //(uiProtocolID !=ISO9141_2_CV_4BYTE)&&
		 (uiProtocolID !=ISO9141_2_00D1)				// 2009.10.06 Added exception for ISO9141_2_00D1
		/*&& (ProtocolType != ISO9141_2_DW_SIEMENSE)*/)
	{
		if(osMessageAvailableSpace(hTransmitMsg) == 0)
		{
			osPoolFree( hCommPKPool, (void *)packet );
			osPoolFree( hMsgPool, (void *)message );
		}
		else
		{
			osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
		}
	}
	else
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
#endif
}
#endif

//uint8_t pPayloadPtcl[100];
void FL_GitPassThruIoctl( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t		*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	uint8_t		ucTempAck = 0;
	unsigned long 		ulIoctlID;
	unsigned int 		uiProtocolID;
    unsigned int uiExtraTime=0;
	stPERIODIC_MSG_INFO 	stPeriodicInfoTemp;
	BYTE            	i;
	int				index = 0;
	//BYTE			status = 0;
	//memcpy( pPayloadPtcl, pInterPtcl, usLength );
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif


	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	uiProtocolID = VCI_GetPassThruProtocolID();
	memcpy(&ulIoctlID, pPayloadPtcl, sizeof(unsigned long));
	//02 11 00 80 00 1C 0C 00 06 10 00 00 86 00 [00 10 01 00] 03 69
	//02 11 00 80 00 09 0C 00 06 10 00 00 7D 00 [08 00 00 00] 03 44

	if(ulIoctlID == GET_CONFIG)
	{
		GLogI( "IoctlID : GET_CONFIG\r\n");
	}
	else if (ulIoctlID == SET_CONFIG)
	{
		GLogI( "IoctlID : SET_CONFIG\r\n");
		VCI_SetSConfigList((unsigned char*)pPayloadPtcl+4, FALSE);

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if (ulIoctlID == FIVE_BAUD_INIT)
	{
		GLogI( "IoctlID : FIVE_BAUD_INIT\r\n");
		VCI_SetSByteArray(pPayloadPtcl+4);
		ucTempAck = g_bAckflag;
		if( g_bAckflag == 1 )
		{
			g_bAckflag = 0;
			if( g_bIsAckWorking == true ) osDelay(70);
		}
		VCI_5bpsInit();
		if( uiProtocolID == WABCO_ABS) {}
		else DLC_RX_BUFF_CLEAR();	//Clear RX buffer for 5bps (fixed issue)
		g_bAckflag = ucTempAck;
		packet->mLen = sizeof(ulIoctlID)+sizeof(stSBYTE_ARRAY);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
		memcpy(packet->mData+sizeof(ulIoctlID), &g_stGITSByteArray.NumOfBytes, sizeof(stSBYTE_ARRAY));
	}
	else if ( ulIoctlID == FAST_INIT )
	{
		for(i=0;i<3;i++)
		{
			if(i>0) DLC_RX_BUFF_CLEAR();
			g_ucFastInitRetryCnt=i;
			g_bIsFastInit=TRUE;
			g_bFastInit_Success=0; //variable clear
			GLogI( "IoctlID : FAST_INIT(%d)\r\n",i+1);
			VCI_FastInit((unsigned char*)pPayloadPtcl+4, (ePKT_TD)eInCommType);
            uiExtraTime =  (*(pPayloadPtcl+20)) * g_stGITSetConfig.nP4Min;	// because need to check after TX data send(extratime == TX time)
			osDelay(g_stGITSetConfig.nP3Min+uiExtraTime);//waiting for P3MIN in the VCI_FastInit(). So the next retry time is at least after P3MIN (only VCI3)
			if(g_bFastInit_Success==1)
			{
				break;
			}
            osDelay(g_stGITSetConfig.nTIdle);
		}
		GLogI( "FAST_INIT break\r\n");

		if( uiProtocolID == ISO9141_2_00D1 )
		{
			//HAL_Delay(160);
			osDelay(160);
			VCI_HW_Setting(ISO9141_2_00D1);
		}
		
		//CDP Source, response from 0x1002(diagnosticThread)  
	}
	else if (ulIoctlID == CLEAR_TX_BUFFER)
	{
		GLogI( "IoctlID : CLEAR_TX_BUFFER\r\n");
		clearTXCanMessage();

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if (ulIoctlID == CLEAR_RX_BUFFER)
	{
		GLogI( "IoctlID : CLEAR_RX_BUFFER\r\n");
		//if(VCI_ProtocolClassification()==eCAN_TX_NONE_PARSING)	//CAN
		{
			//clearRXCanMessage();	//CAN
		}
		//else
		{
			//clearKLReceiveData(KL_LINE1);//rx buffer clear	//KLINE
			//clearKLReceiveData(KL_LINE2);//rx buffer clear	//KLINE
		}

		if( (uiProtocolID == J1939_4PGN) || (uiProtocolID == J1939)){}
		else
		{
			//intTxdRxdCount = g_stGITSetConfig.nP3Min;
			//while(intTxdRxdCount)
			//{
			//	clearRXCanMessage();	//CAN
			//}
			if(VCI_ProtocolClassification()==eCAN_TX_NONE_PARSING)
			{
				osDelay(g_stGITSetConfig.nP3Min);
			}
			clearRXCanMessage();	//CAN
			clearKLReceiveData(KL_LINE1);//rx buffer clear	//KLINE
			clearKLReceiveData(KL_LINE2);//rx buffer clear	//KLINE
		}

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if (ulIoctlID == ACK_DISABLE)
	{
		GLogI( "IoctlID : ACK_DISABLE\r\n");
		g_bAckflag=0;
		g_bJ2534AckModeStatus = 0;

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
		clearRXCanMessage();	//CAN
	}
	else if (ulIoctlID == ACK_ENABLE)
	{
		GLogI( "IoctlID : ACK_ENABLE\r\n");
		g_bAckflag=1;
		g_bJ2534AckModeStatus = 1;

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if (ulIoctlID == ACK_TIMING_CONTROL)//kkt need develop
	{
		GLogI( "IoctlID : ACK_TIMING_CONTROL\r\n");

		g_uiAckTiming=0;
		memcpy(&g_uiAckTiming, &pPayloadPtcl[4], sizeof(U16));
		GLogI( "set AckTime : %d\r\n",g_uiAckTiming);

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}

	else if (ulIoctlID == PERIODIC_MSG_CONTROL)
	{
		memset(&stPeriodicInfoTemp, 0x00, sizeof(stPeriodicInfoTemp));

		stPeriodicInfoTemp.ucMode = pPayloadPtcl[4];					// 0x00: OFF,  0x01: ON,  0xFF: MAX
		
		memcpy(&stPeriodicInfoTemp.uiInterval, &pPayloadPtcl[5], sizeof(U32));		// ms

		stPeriodicInfoTemp.ucMsg[0] = pPayloadPtcl[9];						// DLC

		if( (stPeriodicInfoTemp.ucMsg[0]&0x80) != 0x80 )										//CAN 2.0A
		{
			stPeriodicInfoTemp.ucMsg[1] = pPayloadPtcl[10];
			stPeriodicInfoTemp.ucMsg[2] = pPayloadPtcl[11];
			if( stPeriodicInfoTemp.ucMsg[0] <= 0x0F )
			{
				memcpy(&stPeriodicInfoTemp.ucMsg[3], &pPayloadPtcl[12], stPeriodicInfoTemp.ucMsg[0]);
			}
			else
			{
			  	memset(&stPeriodicInfoTemp.ucMsg[0], 0x00, sizeof(stPeriodicInfoTemp.ucMsg));
			}
		}
		else																		//CAN 2.0B
		{
			stPeriodicInfoTemp.ucMsg[1] = pPayloadPtcl[10];
			stPeriodicInfoTemp.ucMsg[2] = pPayloadPtcl[11];
			stPeriodicInfoTemp.ucMsg[3] = pPayloadPtcl[12];
			stPeriodicInfoTemp.ucMsg[4] = pPayloadPtcl[13];
			if( (stPeriodicInfoTemp.ucMsg[0]-0x80) <= 0x0F )
			{
				memcpy(&stPeriodicInfoTemp.ucMsg[5], &pPayloadPtcl[14], stPeriodicInfoTemp.ucMsg[0]-0x80);
			}
			else
			{
			  	memset(&stPeriodicInfoTemp.ucMsg[0], 0x00, sizeof(stPeriodicInfoTemp.ucMsg));
			}
		}

		// status true : success = 1,

		if(stPeriodicInfoTemp.ucMode == 0x01) //ON
		{
			GLogI( "IoctlID : PERIODIC_MSG_CONTROL ON\r\n");
			GLogI( "PeriodMSG : %02X%02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n"
				,stPeriodicInfoTemp.ucMsg[1],stPeriodicInfoTemp.ucMsg[2],stPeriodicInfoTemp.ucMsg[3],stPeriodicInfoTemp.ucMsg[4],stPeriodicInfoTemp.ucMsg[5]
				,stPeriodicInfoTemp.ucMsg[6],stPeriodicInfoTemp.ucMsg[7],stPeriodicInfoTemp.ucMsg[8],stPeriodicInfoTemp.ucMsg[9],stPeriodicInfoTemp.ucMsg[10]);
			index = FindSamePeriodicMsg(&stPeriodicInfoTemp);
			if(index == -1)//Same MSG not exist
			{
				//stPeriodicInfoTemp.uiOldTime = Get_Tmr();

				index = FindEmptyPeriodicMsgInfo(&stPeriodicInfoTemp);

				if (index == -1)
				{
					//status = 0;
				}
				else
				{
				  	uint8_t ucTimerIndex_temp = stPeriodicMsgInfo[index].ucTimerIndex;
					memcpy(&stPeriodicMsgInfo[index].ucIndex, &stPeriodicInfoTemp, sizeof(stPeriodicInfoTemp));
					stPeriodicMsgInfo[index].ucTimerIndex = ucTimerIndex_temp;
					GLogI( "index : %d\r\n", index);
					//status = 1;
					if(index==0)
					{
					  	VCI_PeriodicMessage0();
						ChangeSWTimer(stPeriodicMsgInfo[index].ucTimerIndex, stPeriodicMsgInfo[index].uiInterval, eSWTimer_INFINITE, VCI_PeriodicMessage0, true );
					}
					else if(index==1)
					{
					  	VCI_PeriodicMessage1();
						ChangeSWTimer(stPeriodicMsgInfo[index].ucTimerIndex, stPeriodicMsgInfo[index].uiInterval, eSWTimer_INFINITE, VCI_PeriodicMessage1, true );
					}
					else if(index==2)
					{
					  	VCI_PeriodicMessage2();
						ChangeSWTimer(stPeriodicMsgInfo[index].ucTimerIndex, stPeriodicMsgInfo[index].uiInterval, eSWTimer_INFINITE, VCI_PeriodicMessage2, true );
					}
					else if(index==3)
					{
					  	VCI_PeriodicMessage3();
						ChangeSWTimer(stPeriodicMsgInfo[index].ucTimerIndex, stPeriodicMsgInfo[index].uiInterval, eSWTimer_INFINITE, VCI_PeriodicMessage3, true );
					}
					else if(index==4)
					{
					  	VCI_PeriodicMessage4();
						ChangeSWTimer(stPeriodicMsgInfo[index].ucTimerIndex, stPeriodicMsgInfo[index].uiInterval, eSWTimer_INFINITE, VCI_PeriodicMessage4, true );
					}
				}
			}
			else//Same MSG aleady exist
			{
				//status = 0;
			}

		}
		else if(stPeriodicInfoTemp.ucMode == 0x00) //OFF
		{
		  	GLogI( "IoctlID : PERIODIC_MSG_CONTROL OFF\r\n");
			index = FindSamePeriodicMsg(&stPeriodicInfoTemp);
			GLogI( "index : %d\r\n", index);
			if(index == -1){}
			  	//status = 0;
			else//Same Periodic MSG aleady exist
			{
				//status = 1;
//				StopSWTimer(stPeriodicMsgInfo[0].ucTimerIndex);
//				StopSWTimer(stPeriodicMsgInfo[1].ucTimerIndex);
//				StopSWTimer(stPeriodicMsgInfo[2].ucTimerIndex);
//				StopSWTimer(stPeriodicMsgInfo[3].ucTimerIndex);
//				StopSWTimer(stPeriodicMsgInfo[4].ucTimerIndex);
//				memset(&stPeriodicMsgInfo[index].ucIndex, 0x00, sizeof(stPeriodicMsgInfo)-1);//all periodic stop
//				memset(&stPeriodicMsgInfo[0].ucIndex, 0x00, (sizeof(stPeriodicInfoTemp))-1);//all periodic stop
//				memset(&stPeriodicMsgInfo[1].ucIndex, 0x00, (sizeof(stPeriodicInfoTemp))-1);//all periodic stop
//				memset(&stPeriodicMsgInfo[2].ucIndex, 0x00, (sizeof(stPeriodicInfoTemp))-1);//all periodic stop
//				memset(&stPeriodicMsgInfo[3].ucIndex, 0x00, (sizeof(stPeriodicInfoTemp))-1);//all periodic stop
//				memset(&stPeriodicMsgInfo[4].ucIndex, 0x00, (sizeof(stPeriodicInfoTemp))-1);//all periodic stop
			  	StopSWTimer(stPeriodicMsgInfo[index].ucTimerIndex);
				memset(&stPeriodicMsgInfo[index].ucIndex, 0x00, (sizeof(stPeriodicInfoTemp)-1));//one Periodic stop

			}
		}
		else if(stPeriodicInfoTemp.ucMode == 0x02) // Clear
		{
		  	GLogI( "IoctlID : PERIODIC_MSG_CONTROL CLEAR");
		  	ClearAllPeriodicMsg();
		}

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if (ulIoctlID == FW_EZDSMode_ENABLE)
	{
		GLogI( "IoctlID : FW_EZDSMode_ENABLE\r\n");

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if (ulIoctlID == FW_EZDSMode_DISABLE)
	{
		GLogI( "IoctlID : FW_EZDSMode_DISABLE\r\n");

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if(ulIoctlID == CAN_CH_CONTROL)
	{
		GLogI( "IoctlID : CAN_CH_CONTROL\r\n");
		memcpy(&g_ucCAN_CH, pPayloadPtcl+4, sizeof(uint8_t));
		GLogN("\r\nCAN_CH_CONTROL : %d", g_ucCAN_CH);

		packet->mLen = sizeof(ulIoctlID);

		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if (ulIoctlID == ETHERNET_ENABLE)
	{
		GLogI( "IoctlID : ETHERNET_ENABLE\r\n");

		packet->mLen = sizeof(ulIoctlID);

		ETHERNET_LINE_CONTROL();
		EnableEthDiag();
		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if (ulIoctlID == ETHERNET_DISABLE)
	{
		GLogI( "IoctlID : ETHERNET_DISABLE\r\n");

		packet->mLen = sizeof(ulIoctlID);

		VCI_Clear_DLC_HW();
		DisableEthDiag();
		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
/*
	else if (ulIoctlID == ENCRYPTION_ENABLE)
	{
		GLogI( "IoctlID : ENCRYPTION_ENABLE\r\n");

		packet->mLen = sizeof(ulIoctlID);

		g_bEncryptFlag=ON;
		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
	else if (ulIoctlID == ENCRYPTION_DISABLE)
	{
		GLogI( "IoctlID : ENCRYPTION_DISABLE\r\n");

		packet->mLen = sizeof(ulIoctlID);

		g_bEncryptFlag=OFF;
		memcpy(packet->mData, &ulIoctlID, sizeof(ulIoctlID));
	}
*/
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1007;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
void FL_GitVciVersion( stCommPkt *pkt, uint32_t eInCommType  )
{
	float version = 2.64;
	char FWDate[20];
	sprintf(FWDate, "2014 05 22");

	stCommPkt	*packet;
	stMsgClst	*message;
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 24;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x2210;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	memcpy(&packet->mData[0], 				&version, sizeof(version));
	memcpy(&packet->mData[sizeof(version)], FWDate,   sizeof(FWDate));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_GitPassThruStopWriteMsgs( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	if(GetCurFwServiceMode()!=eApp_Inside)	g_bAckflag = 1;
	intDlccomCount = g_uiAckTiming; // Ack Timming
	//RxMsgCounterTemp = 0;
	//DlcTxBlock = 0;
	//J2534PassThruContRxMsgNum = 0;

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 0;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1101;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_GitGetAutoVinData( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s, Mode:%d\r\n", __FUNCTION__ ,pPayloadPtcl[0]);
#endif

	if(g_ucAUTOVIN[0]==0x00) { Get_AUTOVINData(pPayloadPtcl[0]); VCI_Clear_DLC_HW(); }
	else if(pPayloadPtcl[0] == 0x03) 		//161109 LWH 0X03 ?�???v??AUTOVIN ?ٽ????
    {
		memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN)/sizeof(g_ucAUTOVIN[0]));
		Get_AUTOVINData(pPayloadPtcl[0]);
		VCI_Clear_DLC_HW();
	}

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1105;
#ifdef CV_AUTOVIN
    if( pPayloadPtcl[0] == 6 || pPayloadPtcl[0] == 7 )
    {
        packet->mLen	= 25;
        packet->mData[0] = pPayloadPtcl[0];
        memcpy( &packet->mData[1], &g_ucAUTOVIN[0], packet->mLen );
        memset(g_ucAUTOVIN, 0x00, sizeof(g_ucAUTOVIN));
    }
    else
#endif
    {
        packet->mLen	= g_ucAUTOVIN[1]+1;
        packet->mData[0]=g_ucAUTOVIN[0];
        memcpy( &packet->mData[1], &g_ucAUTOVIN[2], g_ucAUTOVIN[1] );
    }
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void FL_GitAutoVinconfig( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	UINT iLength=0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	if(pPayloadPtcl[0] == 0x00)	//read
	{
		packet->mData[0] 		= 0;
		GetVCI2FileRead(FILENAME_AUTOVIN_CONFIG_DAT,&packet->mData[1], &iLength);
	}
	else if(pPayloadPtcl[0] == 0x01)	//write
	{
		iLength=1;
		packet->mData[0] 		= 1;
		packet->mData[1] = GetVCI2FileWrite(FILENAME_AUTOVIN_CONFIG_DAT,&packet->mData[1], iLength);
	}

	packet->mLen	= iLength+1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1106;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitEtcFunction( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	U32 ret;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	ret=GitEtcFunction(pPayloadPtcl,packet->mData);

	packet->mLen	= ret;
	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x1309;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#ifdef VCI3_RECORD
void FL_GitSetEngineStopTiggerInfo( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t 	*pPayloadPtcl	= pkt->mData;
	uint8_t 	usLength 		= pkt->mLen - 8;

	FIL Filepnt;
	uint8_t Result =0;
	U32 cnt;
	uint8_t EngineStopTiggerInfoBuf[60];

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	memset(&EngineStopTiggerInfoBuf,0x00, sizeof(EngineStopTiggerInfoBuf));	// Buffer Clear
	memcpy(&EngineStopTiggerInfoBuf , pPayloadPtcl, usLength);

	//f_chdir(DIR_ROOT); // CHJ Change to ROOT directory (Function id 1203 uses DIR_RECORD, clear)
	f_chdir("/03_Record");
	f_unlink(ENGINE_STALL_DATA);
	if (f_open(&Filepnt, ENGINE_STALL_DATA, FA_CREATE_ALWAYS | FA_WRITE | FA_READ) == FR_OK )
	{
		GLogI( "\n\r %s File Open Success\r\n", ENGINE_STALL_DATA);
		if((Result=f_write(&Filepnt,(void*)EngineStopTiggerInfoBuf,usLength,&cnt))==FR_OK)
		{
			GLogI("\n Result : %d",Result);
		}
		else
		{		GLogI("\nF_write Fail : %d",Result);		}
		if(f_close(&Filepnt)!=FR_OK)
		{
			GLogI("\n File Close FAIL!");
		}
	}
	else
	{
		GLogI("\n File Create FAIL!");
	}

	packet->mLen	= 0;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1410;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitSetFunctionTypeReq( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t 	*pPayloadPtcl 	= pkt->mData;
	uint8_t 	usLength 		= pkt->mLen - 8;
	
	FIL Filepnt;
	uint8_t Result =0;
	U32 cnt;
	uint8_t FunctionTypeReq[1010];

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	memset(&FunctionTypeReq,0x00, sizeof(FunctionTypeReq));	// Buffer Clear
	memcpy(&FunctionTypeReq , pPayloadPtcl, usLength);

	//f_chdir(DIR_ROOT);	// CHJ		ROOT ƺ?????? ???? (Function id 1203 ???? DIR_RECORD?? ????? clear)
	f_chdir("/03_Record");
	f_unlink(ENGINE_STALL_DATA);
	if (f_open(&Filepnt, ENGINE_STALL_DATA, FA_CREATE_ALWAYS | FA_WRITE | FA_READ) == FR_OK )
	{
		GLogI( "\n\r %s File Open Success\r\n", ENGINE_STALL_DATA);
		if((Result=f_write(&Filepnt,(void*)FunctionTypeReq,usLength,&cnt))==FR_OK)
		{
			GLogI("\n Result : %d",Result);
		}
		else
		{		GLogI("\nF_write Fail : %d",Result);		}
		if(f_close(&Filepnt)!=FR_OK)
		{
			GLogI("\n File Close FAIL!");
		}
	}
	else
	{
		GLogI("\n File Create FAIL!");
	}

	packet->mLen	= 0;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1420;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void FL_GitCheckAutoVinData( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t i;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
	////////// VCI3 always read vin //kkt
	if(g_ucAUTOVIN[0]==0x00) {GLogI( "1107VIN1: "); Get_AUTOVINData(2); VCI_Clear_DLC_HW(); }
	else
	{
		GLogI( "1107VIN : ");
	}
	for( i = 0; i < 20; i++ )
	{GLogI( "%c", g_ucAUTOVIN[i] );}
	GLogI( "\r\n" );
	/////////

	packet->mLen	= g_ucAUTOVIN[1]+1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1107;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]=g_ucAUTOVIN[0];
	memcpy( &packet->mData[1], &g_ucAUTOVIN[2], g_ucAUTOVIN[1] );

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitConfigFileOpen( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	unsigned char cResult = 0x00;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

//	g_bWifiConnCheckDisable=1;
//	GLogI( "Wifi ConnCheck Disable\r\n");
//
//	if(bt_connected == BT_SPP_CONNECT)
//	{ //If BT connected and wifi enabled
//		GLogI( "BT enable, WIFI disable\r\n" );
//		if( ( 0!=HAL_GPIO_ReadPin( WIFI_ON_OFF_INT_GPIO_Port, WIFI_ON_OFF_INT_Pin ) ) )//wifi button On
//		{
//			DisconnectWLan( );
//			WIFI_R_LED_OFF;
//			g_bWifiAutoConDisable=1;
//			g_WifiAutoConnectStop=1;
//		}
//	}

	U32 FileTotalSize;
	char FileName[25];

	memcpy(FileName, (void const*)&pPayloadPtcl[0], sizeof(FileName));
	memcpy(&FileTotalSize, (void const*)&pPayloadPtcl[25], sizeof(U32));

//	if( !isSDCardConnectedStatus() )		                                    cResult = 0x03;     // SD Card Not Insert
//	else if( !isWriteProtected() )			                                    cResult = 0x04;     // SD Card is Protected
//	else
    {
        cResult = VciRecvFileOpen(FileName, FileTotalSize);
        if     ( cResult == FR_TIMEOUT )                                        cResult = 0x02;     // Time Out
        else if( cResult == FR_NO_FILESYSTEM || cResult == FR_INVALID_DRIVE )   cResult = 0x06;     // File System Fail
        else if( cResult != FR_OK )                                             cResult = 0x05;     // File Create Fail
    }

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1201;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]	= cResult;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitConfigFileReceive( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	unsigned char cResult;
	int uiFileRSSize;
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	//GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	memcpy(&uiFileRSSize, (void const*)&pPayloadPtcl[0], sizeof(U32));
//	if(1==g_bCsDiffFlag)
//	{
//		g_bCsDiffFlag=0;
//		cResult=0x01;
//		//GLogI( "cResult=0x01 \r\n" );
//	}
//	else
	{
#if 1	//old logic
		cResult = VciRecvFileReceive(&pPayloadPtcl[0]+sizeof(U32), uiFileRSSize);
#else	//new logic
		cResult = VciRecvFileReceive(&pPayloadPtcl[0], usLength);
#endif
	}

    if     ( cResult == FR_TIMEOUT )                                        cResult = 0x02;     // Time Out
    else if( cResult == 1 )
	{
		cResult = 0x01;     // File Write Fail
		GLogI( "cResult=0x01 \r\n" );
    }
    else if( cResult != FR_OK )                                             cResult = 0x07;     // File Write Fail

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1202;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]= cResult;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);
#if defined (USB_SPEED_TEST)
	g_uiUSBRx_Cnt++;
#endif
	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitConfigFileClose( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	//	uint64_t ulAvg = 0;
	unsigned char cResult = 0x00;
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

//g_bWifiAutoConDisable=0; //BT and wifi disable control
//	g_WifiAutoConnectStop=0;
//	g_bWifiConnCheckDisable=0;
//	GLogI( "Wifi ConnCheck Enable\r\n");

	//GLogI( "WIFI enable1\r\n" );

	cResult = VciRecvFileClose();
    if     ( cResult == FR_TIMEOUT )                                        cResult = 0x02;     // Time Out
    else if( cResult != FR_OK )                                             cResult = 0x08;     // File Close Fail

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1203;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]= cResult;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	//ulAvg =  g_uiUSBRxInterval / g_uiUSBRx_Cnt;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
	  	//GLogN("ulAvg : %d g_uiUSBRx_Cnt: %d\r\n", ulAvg, g_uiUSBRx_Cnt);
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_GitRcvFileOpen( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	unsigned char cResult = 0x00;
	U32 FileTotalSize;
	char FileName[100]= {'\0', },FilePath[100]= {'\0', };
	uint8_t FilePathSize=0, FileNameSize=0;
	uint8_t FileOpenOption=8;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif	

	FileOpenOption=pPayloadPtcl[0];
	memcpy(&FileTotalSize, (void const*)&pPayloadPtcl[1], sizeof(U32));
	FilePathSize=pPayloadPtcl[5];
	memcpy(FilePath, (void const*)&pPayloadPtcl[6], FilePathSize);
	FilePath[FilePathSize]='\0';
	FileNameSize=pPayloadPtcl[6+FilePathSize];
	memcpy(FileName, (void const*)&pPayloadPtcl[6+FilePathSize+1], FileNameSize);
	FileName[FileNameSize]='\0';

	GLogI( "FileOpenOption :  %d\r\n", FileOpenOption );
	GLogI( "FilePathSize :  %d\r\n", FilePathSize );
	GLogI( "FilePath :  %s\r\n", FilePath );
	GLogI( "FileNameSize :  %d\r\n", FileNameSize );
	GLogI( "FileName :  %s\r\n", FileName );
	
	cResult = VciRecvFileOpenWithPath(FileOpenOption,	//
										FilePath, 		//
										FileName, 		//
										FileTotalSize);	//
    if     ( cResult == FR_TIMEOUT )                                        cResult = 0x02;     // Time Out
    else if( cResult == FR_NO_FILESYSTEM || cResult == FR_INVALID_DRIVE )   cResult = 0x06;     // File System Fail
    else if( cResult != FR_OK )                                             cResult = 0x05;     // File Create Fail


	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1230;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]	= cResult;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#if 0
void FL_GitFileReceive( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	unsigned char cResult;
	int uiFileRSSize;
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	//GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	memcpy(&uiFileRSSize, (void const*)&pPayloadPtcl[0], sizeof(U32));
//	if(1==g_bCsDiffFlag)
//	{
//		g_bCsDiffFlag=0;
//		cResult=0x01;
//		//GLogI( "cResult=0x01 \r\n" );
//	}
//	else
	{
#if 1	//old logic
		cResult = VciRecvFileReceive(&pPayloadPtcl[0]+sizeof(U32), uiFileRSSize);
#else	//new logic
		cResult = VciRecvFileReceive(&pPayloadPtcl[0], usLength);
#endif
	}

    if     ( cResult == FR_TIMEOUT )                                        cResult = 0x02;     // Time Out
    else if( cResult == 1 )
	{
		cResult = 0x01;     // File Write Fail
		GLogI( "cResult=0x01 \r\n" );
    }
    else if( cResult != FR_OK )                                             cResult = 0x07;     // File Write Fail

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1231;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]= cResult;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);
#if defined (USB_SPEED_TEST)
	g_uiUSBRx_Cnt++;
#endif
	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_GitRcvFileClose( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	//	uint64_t ulAvg = 0;
	unsigned char cResult = 0x00;
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	cResult = VciRecvFileCloseBigName();
    if     ( cResult == FR_TIMEOUT )                                        cResult = 0x02;     // Time Out
    else if( cResult != FR_OK )                                             cResult = 0x08;     // File Close Fail

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1232;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]= cResult;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	//ulAvg =  g_uiUSBRxInterval / g_uiUSBRx_Cnt;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
	  	//GLogN("ulAvg : %d g_uiUSBRx_Cnt: %d\r\n", ulAvg, g_uiUSBRx_Cnt);
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_GitSendFileOpen( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	U32 FileTotalSize=0;
	char FileName[100]= {'\0', },FilePath[100]= {'\0', };
	uint8_t FilePathSize=0, FileNameSize=0;

if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif


	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 5;

	memcpy(&g_uiRecFrameSize, &pPayloadPtcl[0], sizeof(g_uiRecFrameSize));
	FilePathSize = pPayloadPtcl[4];
	memcpy(FilePath, (void const*)&pPayloadPtcl[5], FilePathSize);
	FilePath[FilePathSize]='\0';
	FileNameSize=pPayloadPtcl[5+FilePathSize];
	memcpy(FileName, &pPayloadPtcl[5+FilePathSize+1], FileNameSize);
	FileName[FileNameSize]='\0';
	

	packet->mData[0] = VciSendFileOpenWithPath(FilePath, FileName, &FileTotalSize);
	memcpy(&packet->mData[1], &FileTotalSize, sizeof(U32));

	g_u32UpdateFileSize = FileTotalSize;
	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x1233;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void FL_GitRecordFileErase( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1207;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0] = VciEraseFile();

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitReadRecordFileList( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t			ResultData;
	uint8_t			cResult;
	U32 		iPayLoadDataLen;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	f_chdir(DIR_ROOT);


	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 4000;

#if 0 //Old code commented out
#else	//
//	if( !isSDCardConnectedStatus() )		                                    ResultData = 0x03;     // SD Card Not Insert
//	else if( !isWriteProtected() )			                                    ResultData = 0x04;     // SD Card is Protected
//	else
//    {
		cResult = VciDirFile(packet->mData+1+sizeof(U32), &iPayLoadDataLen);
		if( cResult == FR_OK )
		{
			ResultData = 0x00;     // Success
			if(iPayLoadDataLen==25) ResultData = 0x01;     // Success but no file
		}
        else if( cResult == FR_TIMEOUT )
		{
			ResultData = 0x02;     // Time Out
		}
        else if( cResult == FR_NO_FILESYSTEM || cResult == FR_INVALID_DRIVE )
		{
			ResultData = 0x06;     // File System Fail
		}
        else /* MONI 20230307 : [suresoft]83) : block this code for misra c : if( cResult != FR_OK ) */
		{
			ResultData = 0x05;     // File Create Fail
		}
//    }
#endif
	packet->mData[0]=ResultData;
	memcpy(packet->mData+1, &iPayLoadDataLen, sizeof(U32));
	packet->mLen = iPayLoadDataLen + 5;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1208;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitIsRecordingMode( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1209;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	//if((IsConfigFile() == TRUE))	packet->mData[0] 		= 2;
	if( GetCurFwServiceMode() == eApp_Inside )  packet->mData[0] 		= 2;
	else 							packet->mData[0] 		= 1;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitRecordFileOpen( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	char FileName[25];
	U32 FileTotalSize=0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 5;

	//File operation result
	//g_bIsRecFileSending = TRUE;
	memset(FileName, NULL, sizeof(FileName));
	memcpy(FileName, pPayloadPtcl, sizeof(FileName));
	memcpy(&g_uiRecFrameSize, pPayloadPtcl+25, sizeof(g_uiRecFrameSize));

	packet->mData[0] = VciSendFileOpen((unsigned char*)FileName, &FileTotalSize);
	memcpy(&packet->mData[1], &FileTotalSize, sizeof(U32));


	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x120B;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}


void FL_GitRecordFileSend( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	U32 FileRSSize, FileReadSize, uiFileSequence;

#if defined(DEBUG_GIT_PTCL_LOG)
	//GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 4010;

	memcpy(&uiFileSequence, pPayloadPtcl, sizeof(uiFileSequence));

	FileReadSize	 = g_uiRecFrameSize;
	packet->mData[0] = VciReadFile_Slave(FileReadSize, &packet->mData[9], &FileRSSize, uiFileSequence);
	memcpy(&packet->mData[1], &FileRSSize, sizeof(U32));
	memcpy(&packet->mData[5], &uiFileSequence, sizeof(U32));

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x120C;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_GitSendFileSend( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	U32 FileRSSize, FileReadSize, uiFileSequence;

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	//packet->mLen	= 4010;

	memcpy(&uiFileSequence, pPayloadPtcl, sizeof(uiFileSequence));

	FileReadSize	 = g_uiRecFrameSize;
	packet->mData[0] = VciReadFile_Send(FileReadSize, &packet->mData[9], &FileRSSize, uiFileSequence);
	if(packet->mData[0]==FR_OK) g_u32UpdateFileSize += FileRSSize;
	memcpy(&packet->mData[5], &FileRSSize, sizeof(U32));
	memcpy(&packet->mData[1], &uiFileSequence, sizeof(U32));
	packet->mLen	= FileRSSize+9;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1234;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void FL_GitRecordFileClose( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->mData[0] = VciSendFileClose();

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x120D;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_GitSendFileClose( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	//packet->mData[0] = VciSendFileClose();
	packet->mData[0] = VciSendFileCloseBigName();

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1235;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_GitFileErase( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	char FileName[100]= {'\0', },FilePath[100]= {'\0', };
	uint8_t FilePathSize=0, FileNameSize=0, EraseOption=0;

if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif


	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	EraseOption = pPayloadPtcl[0];
	FilePathSize = pPayloadPtcl[1];
	memcpy(FilePath, (void const*)&pPayloadPtcl[2], FilePathSize);
	FilePath[FilePathSize]='\0';
	FileNameSize=pPayloadPtcl[2+FilePathSize];
	memcpy(FileName, &pPayloadPtcl[2+FilePathSize+1], FileNameSize);
	FileName[FileNameSize]='\0';

	packet->mData[0] = VciEraseFileWithPath(FilePath, FileName, EraseOption);

	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x1236;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_GitMakeDirectory( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	char DirPath[100]= {'\0', };
	uint8_t DirPathSize=0;

if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif


	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	DirPathSize = pPayloadPtcl[0];
	memcpy(DirPath, (void const*)&pPayloadPtcl[1], DirPathSize);
	DirPath[DirPathSize]='\0';

	packet->mData[0] = VciMakeDirWithPath(DirPath);

	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x1237;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void FL_Git_GetEncryptKey( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    /* local value */
    uint32_t random_number;
    uint16_t rsaKeysize;
    uint32_t result;
    const int aes256Keysize = 32;
    const int rsa1024_encrypt_size = 128;
    uint8_t PublicKey[256] = {0, };

    /* Get RSA KeySize */
    memcpy(&rsaKeysize, &pPayloadPtcl[0], sizeof(rsaKeysize)); //pPayloadPtcl[0] ~ pPayloadPtcl[1], RSA public key size

    /* Get Public key */
    memcpy(PublicKey, &pPayloadPtcl[2], usLength - 2);

    /* RNG Generate */
    for(int i = 0; i < 8; i++)
    {
        while(HAL_RNG_GetState(&hrng) != HAL_RNG_STATE_READY)
        {
            //wait until ready
        }

        HAL_RNG_GenerateRandomNumber(&hrng, &random_number);
        /* AES256 Key Assign */
        memcpy(&g_ucAES256_Key[0 + ( i * sizeof(random_number) )], &random_number, sizeof(random_number));
    }

    /* RSA Encrypt */
    extern int32_t setRSA_Encrypt(uint8_t *pInMessage, uint32_t MsgLength, uint8_t *pKeyString, uint32_t KeySize, uint8_t *pOutMessage);
    result = setRSA_Encrypt(g_ucAES256_Key, aes256Keysize, PublicKey, rsaKeysize, &packet->mData[3]);
    if(result == CMOX_RSA_SUCCESS)
    {
        g_bEncryptFlag = ON;
        GLogN( "encrypt on\r\n" );
        packet->mLen = rsa1024_encrypt_size + 3;
        packet->mData[0] = 0x00; // Success
        packet->mData[1] = rsa1024_encrypt_size;
        packet->mData[2] = 0x00;
    }
    else
    {
        packet->mLen = 1;
        packet->mData[0] = 0x01; // Fail
    }

	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x1238;
    packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

    if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_DirectCAN( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	stMsgClst	*txMsg;
	stFdcanPkt	*txPkt;

	uint32_t	canID;
	int i=0;
	
	uint8_t *pPayloadPtcl = pkt->mData;
	static char s_cFirstTimeFlag=0;

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	clearRXCanMessage();
	clearTXCanMessage();

	
	if( s_cFirstTimeFlag == 0 )
	{
		HIGHCAN2_120OHM_ENABLE;

		CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

		setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x0700, 0x7ff );
		setFilter2( FDCAN_FILTER_TO_RXFIFO0, 0x0700, 0x7ff );
		startFDCan( 1, 0, 0 );
		s_cFirstTimeFlag=1;
	}

	// Tx
	txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( txMsg != NULL )
	{
		txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
		if( txPkt != NULL )
		{
			canID = ( pPayloadPtcl[0] << 8 ) + pPayloadPtcl[1];

			txPkt->mLen 		= 8;
			txPkt->pTarget		= &hfdcan1;
			for( i = 0; i < 8; i++ ) txPkt->mData[i] = pPayloadPtcl[i+2];

			makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
			txMsg->pPacket	= (void *)txPkt;

			osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

//			stMsgClst	*rxMsg;
//			stFdcanPkt	*rxPkt;
//			evt = osMessageGet( hFDRxMsg, 5000 );
//			if( evt.status == osEventMessage )
//			{
//				rxMsg	= ( stMsgClst * )evt.value.p;
//				rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
//				packet->mData[0] = (rxPkt->mRxHeader.Identifier & 0x0000FF00) >>8;
//				packet->mData[1] = (rxPkt->mRxHeader.Identifier & 0x000000FF);
//				memcpy(&packet->mData[2],rxPkt->mData,8);
//				
//				osPoolFree( hFdcanPktPool, (void *)rxPkt );
//				osPoolFree( hMsgPool, (void *)rxMsg );
//				
//				osPoolFree( hCommPKPool, (void *)txPkt );
//				osPoolFree( hMsgPool, (void *)txMsg );
//			}
		}
	}
	
	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x1248;
	packet->mLen = 0;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		if( (g_ucBtConnected == 1) || (g_iUSBConnected == USBD_STATE_CONFIGURED) )	osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
		else
		{
			osPoolFree( hCommPKPool, (void *)packet );
			osPoolFree( hMsgPool, (void *)message );
		}
	}
}

void FL_DirectCANRead( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	stMsgClst	*rxMsg;
	stFdcanPkt	*rxPkt;

	osEvent		evt;

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	evt = osMessageGet( hFDRxMsg, 5000 );
	if( evt.status == osEventMessage )
	{
		rxMsg	= ( stMsgClst * )evt.value.p;
		rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
		packet->mData[0] = (rxPkt->mRxHeader.Identifier & 0x0000FF00) >>8;
		packet->mData[1] = (rxPkt->mRxHeader.Identifier & 0x000000FF);
		packet->mLen = rxPkt->mLen;
		memcpy(&packet->mData[2],rxPkt->mData,rxPkt->mLen);
		
		if( g_bCanLogOnRxFlag == true )
		{
			GLogN("%04X",rxPkt->mRxHeader.Identifier);
			for( int i=0; i<rxPkt->mLen ;i++)
			{
				GLogN("%02X",rxPkt->mData[i]);
			}
			osDelay(1);
		}
		
		osPoolFree( hFdcanPktPool, (void *)rxPkt );
		osPoolFree( hMsgPool, (void *)rxMsg );
	}
	
	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x1249;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		if( (g_ucBtConnected == 1) || (g_iUSBConnected == USBD_STATE_CONFIGURED) )	osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
		else
		{
			osPoolFree( hCommPKPool, (void *)packet );
			osPoolFree( hMsgPool, (void *)message );
		}
	}
}
void FL_Selftest_Factory(stCommPkt *pkt, uint32_t eInCommType )
{
	osEvent		evt;

	stCommPkt	*packet;
	stMsgClst	*message;

	uint8_t		*pPayloadPtcl = pkt->mData;
	uint8_t		testMode, ret;

	uint32_t	i;
    
    //bool bhsmupflag = 0;
    //uint8_t ucauthkey[80] = {0x00, };

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
		return;
	}

	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x1301;

	testMode = pPayloadPtcl[0];

	packet->mData[0]	= testMode;
	packet->mData[1]	= TEST_NG;

	switch( testMode )
	{
		/***********************************************/
		/***   System Info Test   **********************/
		/***********************************************/
		case TEST_MODE_FW_VER	:				// Need to review
		{
			GLogI( "[MODE_FW_VER]\r\n" );

			packet->mLen	= 11;

  			packet->mData[1]	= gsFwInfo.msAppInfo[eApp_bootloader].marrucVersion[0];
			packet->mData[2]	= gsFwInfo.msAppInfo[eApp_bootloader].marrucVersion[1];
			packet->mData[3]	= gsFwInfo.msAppInfo[eApp_VCI_2].marrucVersion[0];
			packet->mData[4]	= gsFwInfo.msAppInfo[eApp_VCI_2].marrucVersion[1];
			packet->mData[5]	= 0;
			packet->mData[6]	= 0;
			packet->mData[7]	= 0;
			packet->mData[8]	= 0;
			packet->mData[9]	= gsFwInfo.marrucTotalVersion[0];
			packet->mData[10]	= gsFwInfo.marrucTotalVersion[1];


			printFWVersion();

			break;
		}

		case TEST_MODE_SERIAL_WRITE	:			// Need to review
		{
			uint8_t		serial[SERIAL_NUMBER_SIZE] = { 0, };

			GLogI( "[MODE_SERIAL_WRITE]\r\n");

			packet->mLen	= 10;

			if( pPayloadPtcl[1] == 1 )					// Write Serial
			{
				memcpy( serial, &pPayloadPtcl[2], SERIAL_NUMBER_SIZE );

#if( PRINT_FUCTIONLIST_DEBUG_MESSAGE )
				GLogI( "write serial : ");
				for( i = 0; i < SERIAL_NUMBER_SIZE; i++ )
				{
					GLogI( "%c", serial[i] );
				}
				GLogI( "\r\n" );
#endif	// PRINT_FUCTIONLIST_DEBUG_MESSAGE

				gsFwInfo.mucChanged = TRUE;
				memcpy( (void*)gsFwInfo.marrucSerialNo, (const void*)serial, SERIAL_NUMBER_SIZE );
				saveFirmwareInfo_EMMC(true);
			}

			loadFirmwareInfo_EMMC();

			if( pPayloadPtcl[1] == 1 )
			{
				if( memcmp( serial, gsFwInfo.marrucSerialNo, SERIAL_NUMBER_SIZE ) == 0 )
				{
					packet->mData[1] = TEST_OK;
				}
			}
			else
			{
				 packet->mData[1] = TEST_OK;
			}

			memcpy( &packet->mData[2], gsFwInfo.marrucSerialNo, SERIAL_NUMBER_SIZE );

			break;
		}

		case TEST_MODE_BT_SSID	:
		{
			GLogI( "[TEST_MODE_BT_SSID]\r\n");

			packet->mLen	= 52;

			//! get the local device name
			//status = rsi_bt_get_local_name(&local_name);
			if(rsi_bt_get_local_name(&local_name) == RSI_SUCCESS)
			{
				packet->mData[1] = TEST_OK;
				GLogN("local_name: %s\r\n", local_name.name);
				memcpy( &packet->mData[2], local_name.name, 50 );
			}
			else packet->mData[1] = TEST_NG;

			break;
		}

		case TEST_MODE_LATCH_RESET	:
		{
			GLogI( "[TEST_MODE_LATCH_RESET]\r\n");

			packet->mLen	= 2;

			//Reset latch configuration
			//Hardware circuit related
			//PC receives OK or NG status after 8 seconds
			jumpToBootloaderReset(3000);

			packet->mData[1] = TEST_OK;

			break;
		}

		/***********************************************/
		/***   Peripheral Test   **********************/
		/***********************************************/
		case TEST_MODE_BATTERY_ADC	:
		{
			uint32_t	adcValue;

			packet->mLen	= 6;

			packet->mData[1]	= TEST_OK;

			adcValue = readBatteryValue() / 10;				// need to review
			GLogI( "[MODE_BattADC] %d\r\n", adcValue );

			memcpy( &packet->mData[2], &adcValue, sizeof(adcValue) );

			break;
		}

		case TEST_MODE_REPROGRAM_ADC	:
		{
			uint32_t	adcValue;

			packet->mLen	= 6;

			packet->mData[1]	= TEST_OK;

			adcValue = readReprogramVoltageValue() / 10;				// need to review
			GLogI( "[MODE_ReproADC] %d\r\n", adcValue );

			memcpy( &packet->mData[2], &adcValue, sizeof(adcValue) );

			break;
		}

		case TEST_MODE_IG_OUTPUT	:
		{
			bool	IgState;

			GLogI( "[TEST_MODE_IG_OUTPUT]\r\n");

			packet->mLen	= 2;

			InitIOCTL();
#if 0
			setWakeUpSource( WAKEUP_SOURCE_IG );

			SetWakeUpPin_Input();//for wake pin state read

			IO_CONTROL_LOW( IG_ON_EN);
			GLogN( "IG_ON_EN_PIN LOW\r\n");
			osDelay( 100 );
			IgState=HAL_GPIO_ReadPin( WAK_UP_GPIO_Port, WAK_UP_Pin );
			GLogN( "WAK_UP_Pin & CH3 state : %d\r\n", IgState );

			setWakeUpSource( 0 );
			SetWakeUpPin_AF(); //Configure wake pin
#else
			IO_CONTROL_HIGH( IG_ON_EN);
			IO_CONTROL_LOW( KL_RXD1_SEL );
			IO_CONTROL_LOW( DLCA_EN3 );
			IO_CONTROL_LOW(  KL_TXD1_INV );
			osDelay( 100 );
			IgState=HAL_GPIO_ReadPin( KL_RXD1_GPIO_Port, KL_RXD1_Pin );
			GLogN( "before KL_RXD1_Pin : %d\r\n", IgState );

			IO_CONTROL_LOW( IG_ON_EN);
			IO_CONTROL_LOW( KL_RXD1_SEL );
			IO_CONTROL_HIGH( DLCA_EN3 );
			IO_CONTROL_HIGH( KL_TXD1_INV );
			GLogN( "SET IG_ON_EN LOW,KL_RXD1_SEL LOW,DLCA_EN3 HIGH,KL_TXD1_INV HIGH\r\n");

			osDelay( 100 );
			IgState=HAL_GPIO_ReadPin( KL_RXD1_GPIO_Port, KL_RXD1_Pin );
			GLogN( "after KL_RXD1_Pin : %d\r\n", IgState );
#endif

			InitIOCTL();

			if(IgState==1) packet->mData[1] = TEST_OK;
			else packet->mData[1] = TEST_NG;

			break;
		}

		//circuit loopback test(no need short cable)
		case TEST_MODE_KLINE_510	:
		case TEST_MODE_KLINE_2K	:
		case TEST_MODE_KLINE_47K	:
		{
			if(testMode==TEST_MODE_KLINE_510) {GLogI( "[TEST_MODE_KLINE_510]\r\n");}
			else if(testMode==TEST_MODE_KLINE_2K) {GLogI( "[TEST_MODE_KLINE_2K]\r\n");}
			else if(testMode==TEST_MODE_KLINE_47K) {GLogI( "[TEST_MODE_KLINE_47K]\r\n");}

			packet->mLen	= 3;

			ret=SelfTest_KLINE_pullup(testMode-TEST_MODE_KLINE_510);

			if( ret == 20 )
			{
				packet->mData[1] = TEST_OK;
				packet->mData[2] = 0;
			}
			else
			{
				packet->mData[1] = TEST_NG;

				switch( ret )
				{
					case 0 :		packet->mData[2] = 1;		break;	//1pin error SetKL_Line( KL_LINE1_CONNECT_CH01_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 1 :		packet->mData[2] = 2; 		break;	//2pin error SetKL_Line( KL_LINE1_CONNECT_CH02_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 2 :		packet->mData[2] = 3; 		break;	//3pin error SetKL_Line( KL_LINE1_CONNECT_CH03_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 3 :		packet->mData[2] = 6; 		break;	//6pin error SetKL_Line( KL_LINE1_CONNECT_CH06_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 4 :		packet->mData[2] = 7; 		break;	//7pin error SetKL_Line( KL_LINE1_CONNECT_CH07_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 5 :		packet->mData[2] = 13; 	break;	//13pin error SetKL_Line( KL_LINE1_CONNECT_CH13_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 6 :		packet->mData[2] = 15; 	break;	//15pin error SetKL_Line( KL_LINE1_CONNECT_CH15_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 7 :		packet->mData[2] = 9; 		break;	//9pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH09_CH08 );			break;
					case 8 :		packet->mData[2] = 10; 	break;	//10pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH10_CH08 );			break;
					case 9 :		packet->mData[2] = 11; 	break;	//11pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH11_CH08 );			break;
					case 10 :	packet->mData[2] = 12; 	break;	//12pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH12_CH08 );			break;
					case 11 :	packet->mData[2] = 14; 	break;	//14pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH14_CH08 );			break;
					case 12 :	packet->mData[2] = 15; 	break;	//15pin error SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH15_CH08 );			break;
					default : 	packet->mData[2] = 16; 	break;	//unknown pin error
				}
			}

			break;
		}

		case TEST_MODE_OBD_CONNECT	:	//use short circuit Cable
		{
			GLogI( "[TEST_MODE_OBD_CONNECT]\r\n");

			packet->mLen	= 3;

			ret=SelfTest_KLINE_OBD_Connect();

			if( ret == 20 )
			{
				packet->mData[1] = TEST_OK;
				packet->mData[2] = 0;
			}
			else
			{
				packet->mData[1] = TEST_NG;

				switch( ret )
				{
					case 0 :			packet->mData[2] = 1;		break;	//1pin error SetKL_Line( KL_LINE1_CONNECT_CH01_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 1 :			packet->mData[2] = 2;		break;	//2pin error SetKL_Line( KL_LINE1_CONNECT_CH02_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 2 :			packet->mData[2] = 3;		break;	//3pin error SetKL_Line( KL_LINE1_CONNECT_CH03_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 3 :			packet->mData[2] = 6;		break;	//6pin error SetKL_Line( KL_LINE1_CONNECT_CH06_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 4 :			packet->mData[2] = 7;		break;	//7pin error SetKL_Line( KL_LINE1_CONNECT_CH07_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 5 :			packet->mData[2] = 13;		break;	//13pin error SetKL_Line( KL_LINE1_CONNECT_CH13_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 6 :			packet->mData[2] = 15;		break;	//15pin error SetKL_Line( KL_LINE1_CONNECT_CH15_CH08, KL_LINE2_CONNECT_CH08 );			break;
					case 7 :			packet->mData[2] = 8;		break;	//8pin error SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH08_CH15 );			break;
					case 8 :			packet->mData[2] = 9;		break;	//9pin error SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH09_CH15 );			break;
					case 9 :			packet->mData[2] = 10; 	break;	//10pin error SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH10_CH15  );			break;
					case 10 :		packet->mData[2] = 11; 	break;	//11pin error SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH11_CH15  );			break;
					case 11 :		packet->mData[2] = 12; 	break;	//12pin error SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH12_CH15 );			break;
					case 12 :		packet->mData[2] = 14;		break;	//14pin error SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH14_CH15 );			break;
					default : 		packet->mData[2] = 16;		break;	//unknown pin error
				}
			}

			break;
		}

		case TEST_MODE_REPRO_VOLTAGE	:	//use short circuit Cable
		{
			GLogI( "[TEST_MODE_REPRO_VOLTAGE]\r\n");

			packet->mLen	= 3;

			ret = TestReprogramLine();

			if( ret == 0 )
			{
				packet->mData[1] = TEST_OK;
				packet->mData[2] = 0;
			}
			else
			{
				packet->mData[1] = TEST_NG;

				if( ret & ERROR_REPROGRAM_CH03 )				{packet->mData[2] = 3; GLogEE( "Reprogram Line CH03 Error...\r\n" );}
				else if( ret & ERROR_REPROGRAM_CH06 )			{packet->mData[2] = 6; GLogEE( "Reprogram Line CH06 Error...\r\n" );}
				else if( ret & ERROR_REPROGRAM_CH09 )			{packet->mData[2] = 9; GLogEE( "Reprogram Line CH09 Error...\r\n" );}
				else if( ret & ERROR_REPROGRAM_CH11 )			{packet->mData[2] = 11; GLogEE( "Reprogram Line CH11 Error...\r\n" );}
				else if( ret & ERROR_REPROGRAM_CH12 )			{packet->mData[2] = 12; GLogEE( "Reprogram Line CH12 Error...\r\n" );}
				else if( ret & ERROR_REPROGRAM_CH13 )			{packet->mData[2] = 13; GLogEE( "Reprogram Line CH13 Error...\r\n" );}
				else if( ret & ERROR_REPROGRAM_CH14 )			{packet->mData[2] = 14; GLogEE( "Reprogram Line CH14 Error...\r\n" );}

			}

			break;
		}

		case TEST_MODE_BUZZER	:
		{
			uint32_t	time;

			GLogI( "[MODE_BUZZER]\r\n");

			packet->mLen	= 2;

			time = (pPayloadPtcl[2] << 8) + pPayloadPtcl[1];

			Buzzer_Control( eBUZZER_ON, MSEC(time),MSEC(time), 1 );

			packet->mData[1] = TEST_OK;

			break;
		}

		case TEST_MODE_RTC	:
		{
			uint8_t	datetime[7];
			GLogI( "[MODE_RTC]\r\n");

			if( pPayloadPtcl[1] == 1 )				// rtc write
			{
				packet->mLen	= 3;

				packet->mData[1]	= pPayloadPtcl[1];
				packet->mData[2]	= TEST_OK;

				for( i = 0; i < 6; i++ )			datetime[i] = pPayloadPtcl[i+2];

				Set_RTCData( datetime, 0 );
			}
			else if( pPayloadPtcl[1] == 2 )			// rtc read
			{
				packet->mLen	= 9;

				packet->mData[1]	= pPayloadPtcl[1];

				Get_RTCData( datetime );

				for( i = 0; i < 6; i++ )			packet->mData[i+2] = datetime[i+1];
			}

			break;
		}

		case TEST_MODE_LED_GREEN:
		{
			GLogI( "[TEST_MODE_LED_GREEN]\r\n");

			packet->mLen	= 2;

			packet->mData[1]	= TEST_OK;

			LED_ALL_OFF;
			LED_SetState(eLED_OFF, 0, 0);

			for( i = 0; i < 6; i++ )
			{
				LED_GREEN_TOGGLE;
				osDelay( 200 );
			}

			LED_SetState(eLED_NORMAL, 0, 0);
			LED_GREEN_ON;

			break;
		}
		case TEST_MODE_LED_BLUE:
		{
			GLogI( "[TEST_MODE_LED_BLUE]\r\n");

			packet->mLen	= 2;

			packet->mData[1]	= TEST_OK;

			LED_ALL_OFF;
			LED_SetState(eLED_OFF, 0, 0);

			for( i = 0; i < 6; i++ )
			{
				LED_BLUE_TOGGLE;
				osDelay( 200 );
			}

			LED_SetState(eLED_NORMAL, 0, 0);
			LED_GREEN_ON;

			break;
		}
		case TEST_MODE_LED_RED:
		{
			GLogI( "[TEST_MODE_LED_RED]\r\n");

			packet->mLen	= 2;

			packet->mData[1]	= TEST_OK;

			LED_ALL_OFF;
			LED_SetState(eLED_OFF, 0, 0);

			for( i = 0; i < 6; i++ )
			{
				LED_RED_TOGGLE;
				osDelay( 200 );
			}

			LED_SetState(eLED_NORMAL, 0, 0);
			LED_GREEN_ON;

			break;
		}
		case TEST_MODE_LED_WHITE:
		{
			GLogI( "[TEST_MODE_LED_WHITE]\r\n");

			packet->mLen	= 2;

			packet->mData[1]	= TEST_OK;

			LED_ALL_OFF;
			LED_SetState(eLED_OFF, 0, 0);

			for( i = 0; i < 6; i++ )
			{
				LED_WHITE_TOGGLE;
				osDelay( 200 );
			}

			LED_SetState(eLED_NORMAL, 0, 0);
			LED_GREEN_ON;

			break;
		}

		case TEST_MODE_BT_BTN	:
		{
			uint32_t	oldtime;

			GLogI( "[MODE_BT_BTN]\r\n");

			packet->mLen	= 2;

			oldtime = Get_Tmr();
			while( Get_TmrDelta( Get_Tmr(), oldtime ) < 5000 )
			{
				if( !IO_CONTROL_GET( PAIR_SW ) )
				{
					packet->mData[1] = TEST_OK;
					break;
				}
			}

			break;
		}

		case TEST_MODE_HSM	:
		{
			GLogI( "[MODE_HSM]\r\n");

			uint32_t	ret = 0;
			uint8_t		data[12];
			int 		test_index = 0;
			error_i = 0;
			packet->mLen = 2;
			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				ret = ActivationHSM();
				if( ret == HSM_SUCCESS )
				{
				  	GLogI( "ActivationHSM OK!!!\r\n" );
				}
				else
				{
					GLogEE( "ActivationHSM FAIL(%d)!!!\r\n", ret );
					break;
				}

				ret = ReadCSNHSM( data );
				if( ret == HSM_SUCCESS )
				{
					GLogI( "ReadCSNHSM OK!!!\r\n");
				}
				else
				{
					GLogE( "ReadCSNHSM FAIL(%d)!!!\r\n", ret );
					break;
				}
				test_index = 0;
			}
			else
			{
				GLogN("\r\n========== Get HSN Test ==========\r\n");
                uint8_t serial_number[8];
                HAL_StatusTypeDef status = hsm_get_serial_number(serial_number);
                if (status == HAL_OK)
                {
                	test_index = 0;
                    GLogN("[OK] Get HSN Success\r\n");
                    GLogN("HSM Serial Number: ");
                    for (int i = 0; i < 8; i++)
                    {
                        GLogN("%02X ", serial_number[i]);
                    }
                    GLogN("\r\n");
                }
                else
                {
                	test_index = 1;
                    GLogE("[FAIL] Get HSN failed, status: %d\r\n", status);
                }
                GLogN("===================================\r\n");
			}
			if(test_index == 0)
			{
				packet->mData[1] = TEST_OK;
			}
			else
			{
				packet->mData[1] = TEST_NG;
			}

			break;
		}
		case TEST_MODE_KEK_IMPORT	:
		{
			GLogI( "[MODE_KEK_IMPORT]\r\n");
		
			packet->mLen = 3;
			
			GLogN("\r\n");
            GLogN("==================================================================\r\n");
            GLogN("  special Key Import (KEK KEY) \r\n");
            GLogN("==================================================================\r\n\r\n");

            if (g_HSM_Type == HSM_TYPE_OLD)
            {
                GLogE("[ERROR] This command is for New HSM only\r\n");
				packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_OLD_HSM;
                break;
            }

			uint8_t uckey_data[520] = {0,};
			U16 key_len = 519;//modulus byte length 2, modulus 256, pubSize 2, pubExp 3, priExp 256

			memcpy( &key_len, &pPayloadPtcl[1], sizeof(key_len) );
			GLogN("  key_len: %d\r\n", key_len);
			memcpy( uckey_data, &pPayloadPtcl[3], key_len );

            GLogN("\r\nCalling hsm_import_special_key()...\r\n");

            // Import with private key (public_only=false)
            HAL_StatusTypeDef status = hsm_import_special_key(
                HSM_SPECIAL_KEY_RSA_KEK,
                uckey_data,
                key_len,
                false,  // encrypted
                false   // lock_key
            );
			if (status == HAL_OK)
            {
            	packet->mData[1] = TEST_OK;
                GLogN("[OK] RSA Private Key imported to slot %d\r\n", 149);
            }
            else
            {
            	packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_IMPORT_FAIL;
                GLogE("[FAIL] RSA Key Import failed, status: %d\r\n", status);
				break;
            }

			GLogN("  Key validation... \r\n");
			GLogN("  step1) encrypt key impot test \r\n");

			U16 key_slot = 321;  // Default: HMAC UDK slot #321 ~ #350
            
            // HMAC Secret Key (32 bytes = 256 bits per manual)
            // Secret : 32 bytes of zeros for test
            //static const U8 hmac_secret[32] = {
            //    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            //    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            //    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            //    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            //};
			
			
			static const U8 hmac_secret_encrypt[256] = {
				0x4e, 0xf0, 0xb3, 0xaf, 0x9a, 0xed, 0xb2, 0x32,
				0xe4, 0xd8, 0x7b, 0xee, 0x17, 0x28, 0x0f, 0xd0,
				0x1e, 0x08, 0x03, 0x40, 0x1b, 0xa3, 0xc4, 0x82,
				0x60, 0x04, 0x19, 0x4e, 0x03, 0x35, 0x77, 0xbc,
				0xf9, 0x37, 0x13, 0x14, 0xdb, 0x3f, 0x04, 0xe5,
				0xf7, 0x9b, 0x42, 0x5a, 0x5f, 0x86, 0x0b, 0xed,
				0x29, 0x39, 0x6e, 0xd8, 0xdd, 0x7b, 0xbf, 0xdb,
				0x7b, 0xd0, 0xcf, 0x22, 0x14, 0x01, 0x69, 0x8b,
				0x59, 0xec, 0x63, 0x3e, 0xea, 0xb2, 0xf6, 0x3d,
				0x69, 0x9b, 0xd7, 0xd3, 0x11, 0x40, 0xb5, 0x88,
				0x2a, 0x0d, 0x9d, 0xfd, 0xe0, 0x68, 0x70, 0xac,
				0x70, 0x6b, 0xcb, 0x5a, 0xf9, 0x45, 0x67, 0x4a,
				0x81, 0x9f, 0xf5, 0x51, 0x4b, 0xdf, 0xcb, 0x50,
				0x2c, 0x0b, 0x32, 0x96, 0x1a, 0xf4, 0xe2, 0x90,
				0x52, 0xb3, 0x93, 0x93, 0xdb, 0x7b, 0x0e, 0x97,
				0xc8, 0xe0, 0x2d, 0xf2, 0x69, 0x68, 0x60, 0x02,
				0xb1, 0xd6, 0x29, 0xbf, 0x36, 0xb8, 0xdd, 0xe3,
				0xda, 0x00, 0xb9, 0x9c, 0x76, 0x28, 0x7b, 0x42,
				0xef, 0x76, 0x89, 0xea, 0x4e, 0x38, 0x7e, 0x3d,
				0x28, 0x53, 0x7b, 0x9b, 0x6b, 0x71, 0xa7, 0xee,
				0x02, 0xd6, 0x86, 0x08, 0xd2, 0xc2, 0xa3, 0x32,
				0x72, 0x94, 0xfa, 0x70, 0x4a, 0xa5, 0x0f, 0x6f,
				0x7d, 0xae, 0xa3, 0x45, 0x38, 0x52, 0x90, 0xb8,
				0xff, 0x96, 0x29, 0x88, 0xe2, 0x61, 0xd6, 0x56,
				0xdf, 0x3c, 0xa0, 0x9f, 0x47, 0xc8, 0xa4, 0x97,
				0x63, 0xea, 0x14, 0x8e, 0xa0, 0xbe, 0x3d, 0x50,
				0x24, 0xda, 0x12, 0x0b, 0xaf, 0x95, 0x27, 0x1e,
				0x4e, 0x38, 0xbb, 0x9a, 0x6c, 0xb0, 0xe5, 0xab,
				0x7a, 0x9d, 0xf9, 0x90, 0xab, 0x3d, 0xa1, 0x3a,
				0xb0, 0x52, 0x33, 0xe9, 0x57, 0x26, 0x05, 0xf3,
				0xc1, 0x29, 0x06, 0xb4, 0x4f, 0x8e, 0xbe, 0xa7,
				0x0a, 0x45, 0xf6, 0x46, 0x7c, 0x8e, 0xb6, 0x46
			};

            GLogN("\r\nCalling hsm_import_hmac_key()...\r\n");

			status = hsm_import_hmac_key(
                key_slot,
                hmac_secret_encrypt,
                true,  // encrypted
                false   // lock_key
            );

            if (status == HAL_OK)
            {
            	packet->mData[1] = TEST_OK;
                GLogN("[OK] HMAC Key imported to slot %d\r\n", key_slot);
            }
            else
            {
            	packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_VERIFY_IMPORT_FAIL;
                GLogE("[FAIL] HMAC Key Import failed, status: %d\r\n", status);
				break;
            }

			GLogN("  step2) hmac_sha256 test by import key\r\n");

			// Test data
            // Seed: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF (16 bytes)
            static const U8 test_seed[16] = {
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
            };

            // Expected Output
            // Key: 0x293531BBB4268FD5145915D35FCEEE6E4182241CC5EE9A533944F8BC8C30AAF2 (32 bytes)
            static const U8 expected_key[32] = {
                0x29, 0x35, 0x31, 0xBB, 0xB4, 0x26, 0x8F, 0xD5,
                0x14, 0x59, 0x15, 0xD3, 0x5F, 0xCE, 0xEE, 0x6E,
                0x41, 0x82, 0x24, 0x1C, 0xC5, 0xEE, 0x9A, 0x53,
                0x39, 0x44, 0xF8, 0xBC, 0x8C, 0x30, 0xAA, 0xF2
            };

            GLogN("  Seed (16 bytes): ");
            for (int i = 0; i < 16; i++) GLogN("%02X ", test_seed[i]);
            GLogN("\r\n\r\n");

            GLogN("Expected Output (32 bytes):\r\n");
            for (int i = 0; i < 32; i++)
            {
                GLogN("%02X ", expected_key[i]);
                if ((i + 1) % 16 == 0) GLogN("\r\n");
            }
            GLogN("\r\n");

            // HMAC-SHA256 operation
            U8 output[32] = {0};

            status = hsm_generate_hmac_sha256(key_slot, test_seed, 16, output);

            if (status == HAL_OK)
            {
                GLogN("[OK] HMAC_SHA256 Success\r\n\r\n");
                GLogN("Actual Output (32 bytes):\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", output[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
                GLogN("\r\n");

                if (memcmp(output, expected_key, 32) == 0)
                {
                	packet->mData[1] = TEST_OK;
                    GLogN("[PASS] Output matches expected value!\r\n");
                }
                else
                {
                	packet->mData[1] = TEST_NG;
					packet->mData[2] = TEST_NRC_NOT_MATCH_EXPECT;
                    GLogE("[FAIL] Output does NOT match expected value\r\n");
					break;
                }
            }
            else
            {
            	packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_HMAC_SHA256_FAIL;
                GLogE("[FAIL] HMAC_SHA256 failed, status: %d\r\n", status);
				break;
            }
			break;
		}
		case TEST_MODE_KM_IMPORT	:
		{
			GLogI( "[MODE_KM_IMPORT]\r\n");

			packet->mLen = 3;
			
			GLogN("\r\n");
            GLogN("==================================================================\r\n");
            GLogN("  special Key Import (KM KEY) \r\n");
            GLogN("==================================================================\r\n\r\n");

            if (g_HSM_Type == HSM_TYPE_OLD)
            {
                GLogE("[ERROR] This command is for New HSM only\r\n");
				packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_OLD_HSM;
                break;
            }

			uint8_t uckey_data[520] = {0,};
			U16 key_len = 256;//AES encrypt size
			
			memcpy( &key_len, &pPayloadPtcl[1], sizeof(key_len) );
			GLogN("  key_len: %d\r\n", key_len);
			memcpy( uckey_data, &pPayloadPtcl[3], key_len );

            GLogN("\r\nCalling hsm_import_special_key()...\r\n");

            // Import with private key (public_only=false)
            HAL_StatusTypeDef status = hsm_import_special_key(
                HSM_SPECIAL_KEY_KM,
                uckey_data,
                key_len,
                true,  // encrypted
                false   // lock_key
            );
			if (status == HAL_OK)
            {
            	packet->mData[1] = TEST_OK;
                GLogN("[OK] RSA Private Key imported to slot %d\r\n", 102);
            }
            else
            {
            	packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_IMPORT_FAIL;
                GLogE("[FAIL] RSA Key Import failed, status: %d\r\n", status);
				break;
            }

#if 1
			GLogN("  Key validation... \r\n");
			GLogN("  step1) AES Encrypt (ECB) Test \r\n");

            uint16_t key_id = HSM_KEY_KM;   // Default: TEMP AES slot #201 (same as 0x52)
            uint8_t key_size = HSM_AES_128;

            // Test plaintext (32 bytes = 2 blocks)
            static const uint8_t plaintext[32] = {
                0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
            };
            uint8_t ciphertext[32];

            GLogN("Plaintext (32 bytes):\r\n");
            for (int i = 0; i < 32; i++)
            {
                GLogN("%02X ", plaintext[i]);
                if ((i + 1) % 16 == 0) GLogN("\r\n");
            }

            status = hsm_aes_encrypt(
                key_id,
                key_size,
                HSM_AES_ECB,
                NULL,       // IV not used for ECB
                plaintext,
                32,
                ciphertext
            );

            if (status == HAL_OK)
            {
                GLogN("[OK] AES Encrypt Success\r\n");
                GLogN("Ciphertext:\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", ciphertext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
            }
            else
            {
            	packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_AES_ENCRYPT_FAIL;
                GLogE("[FAIL] AES Encrypt failed, status: %d\r\n", status);
				break;
            }
			
			GLogN("  step2) AES Decrypt (ECB) Test \r\n");
			uint8_t expecttext[32]={0,};
			status = hsm_aes_decrypt(
                key_id,
                key_size,
                HSM_AES_ECB,
                NULL,
                ciphertext,
                32,
                expecttext
            );

            if (status == HAL_OK)
            {
                GLogN("[OK] AES Decrypt Success\r\n");
                GLogN("Plaintext:\r\n");
                for (int i = 0; i < 32; i++)
                {
                    GLogN("%02X ", expecttext[i]);
                    if ((i + 1) % 16 == 0) GLogN("\r\n");
                }
				if (memcmp(plaintext, expecttext, 32) == 0)
                {
                	packet->mData[1] = TEST_OK;
                	GLogN("[PASS] Output matches expected value!\r\n");
				}
            }
            else
            {
            	packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_AES_DECRYPT_FAIL;
                GLogE("[FAIL] AES Decrypt failed, status: %d\r\n", status);
            }
            GLogN("=============================================\r\n");
#endif
			break;
		}
		case TEST_MODE_ECUCODEKEY_IMPORT	:
		{
			GLogI( "[MODE_ECUCODEKEY_IMPORT]\r\n");

			packet->mLen = 3;
			
			GLogN("\r\n");
            GLogN("==================================================================\r\n");
            GLogN("  special Key Import (ECU_CODE KEY) \r\n");
            GLogN("==================================================================\r\n\r\n");

            if (g_HSM_Type == HSM_TYPE_OLD)
            {
                GLogE("[ERROR] This command is for New HSM only\r\n");
				packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_OLD_HSM;
                break;
            }

			uint8_t uckey_data[520] = {0,};
			U16 key_len = 256;//AES encrypt size
			
			memcpy( &key_len, &pPayloadPtcl[1], sizeof(key_len) );
			GLogN("  key_len: %d\r\n", key_len);
			memcpy( uckey_data, &pPayloadPtcl[3], key_len );

            GLogN("\r\nCalling hsm_import_special_key()...\r\n");

            // Import with private key (public_only=false)
            HAL_StatusTypeDef status = hsm_import_special_key(
                HSM_SPECIAL_KEY_ECU_CODE,
                uckey_data,
                key_len,
                true,  // encrypted
                false   // lock_key
            );
			if (status == HAL_OK)
            {
            	packet->mData[1] = TEST_OK;
                GLogN("[OK] ECUCODE Key imported to slot %d\r\n", 101);
            }
            else
            {
            	packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_IMPORT_FAIL;
                GLogE("[FAIL] ECUCODE Key Import failed, status: %d\r\n", status);
				break;
            }			

			GLogN("  Key validation... \r\n");
			GLogN("  step1) FL_Git_ASK_v2_Req Test (PV only, 2 seeds) \r\n");
			
			// ASK Test Vectors
			static const U8 ask_ecu_code[16] = {
				0x87, 0x87, 0xB6, 0xB1, 0xF3, 0xF9, 0x46, 0xB7,
				0xB0, 0x2C, 0x39, 0xEE, 0xD3, 0xC2, 0x99, 0xC0
			};
			
			// �׽�Ʈ�� �õ� 2���� ����
			static const U8 ask_seeds[2][8] = {
				{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77},  // Seed 01
				{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},  // Seed 02
			};
			
			// PV ��밪�� 2���� ���� (���� PV �迭���� �� 2�� ���)
			static const U8 ask_expected_pv[2][8] = {
				{0xDE, 0x93, 0xC3, 0xD1, 0x60, 0x53, 0x0E, 0x20},  // for Seed 01
				{0x36, 0xBB, 0x4B, 0xB7, 0xC4, 0x71, 0xA9, 0x56},  // for Seed 02
			};
			
			// master_id�� PV�� ���� (0)
			const U8 master_id = 0;  // 0=PV

			int pass_count = 0;
			int fail_count = 0;
			
			// 2�� �õ常 �׽�Ʈ
			for (int seed_idx = 0; seed_idx < 2; seed_idx++)
			{
				const U8 *selected_seed = ask_seeds[seed_idx];
				const U8 *expected_key	= ask_expected_pv[seed_idx];
				U8 result[8] = {0};
				U32 ret = HSM_UNKNOWN_ERROR;
			
				if (hsm_generate_ask_key((U8*)selected_seed, (U8*)ask_ecu_code, master_id, NULL, result) == HAL_OK)
				{
					ret = HSM_SUCCESS;
				}
			
				GLogN("Seed[%02d]: ", seed_idx + 1);
				for (int i = 0; i < 8; i++) GLogN("%02X ", selected_seed[i]);
			
				if (ret == HSM_SUCCESS)
				{
					GLogN("\r\n  Result  : ");
					for (int i = 0; i < 8; i++) GLogN("%02X ", result[i]);
					GLogN("\r\n  Expected: ");
					for (int i = 0; i < 8; i++) GLogN("%02X ", expected_key[i]);
			
					if (memcmp(result, expected_key, 8) == 0)
					{
						GLogN(" [PASS]\r\n");
						pass_count++;
					}
					else
					{
						GLogE(" [FAIL]\r\n");
						fail_count++;
					}
				}
				else
				{
					GLogE(" -> HSM Error (ret=%d) [FAIL]\r\n", ret);
					fail_count++;
				}
			}
			
			// Summary
			GLogN("\r\n--------------------------------------------\r\n");
			GLogN("Result: %d PASSED, %d FAILED (Total: 2)\r\n", pass_count, fail_count);
			if (fail_count == 0)
			{
				packet->mData[1] = TEST_OK;
				GLogN(">> ALL TESTS PASSED!\r\n");
			}
			else
			{
				packet->mData[1] = TEST_NG;
				packet->mData[2] = TEST_NRC_ASK_VERIFY_FAIL;
				GLogE(">> SOME TESTS FAILED!\r\n");
			}
			GLogN("============================================\r\n");
			
			break;
		}
		case TEST_MODE_NEW_HSM_CHECK	:
		{
			GLogI( "[MODE_NEW_HSM_CHECK]\r\n");

			packet->mLen = 3;

			packet->mData[1] = TEST_OK;
			
            if (g_HSM_Type == HSM_TYPE_OLD)
            {
                GLogN("OLD HSM!!!\r\n");
				packet->mData[2] = 0x01;//OLD HSM
            }
			else
			{
				GLogN("NEW HSM!!!\r\n");
				packet->mData[2] = 0x02;//NEW HSM
			}
			break;
		}
		case TEST_MODE_NEW_HSM_VERSION:
        {
			GLogI( "[MODE_NEW_HSM_VERSION]\r\n");
			if (g_HSM_Type == HSM_TYPE_OLD)
            {
	            packet->mLen	= 5;
				packet->mData[2] = 1;//OLD HSM
				ret = HSM_Version_Check((int*)&packet->mData[3]);
	            if(ret == HSM_SUCCESS) packet->mData[1] = TEST_OK;
	            else packet->mData[1] = TEST_NG;
	            
			}
			else
			{
				packet->mLen	= 17;
				packet->mData[2] = 2;//NEW HSM
				packet->mData[1] = TEST_NG;
				HAL_StatusTypeDef status = HAL_ERROR;
				
				HSM_VersionInfo_t version_info;				
		        for (int i = 0; i <= HSM_GET_VER_MAX_RETRY; i++)
		        {
					status = hsm_get_version_info(&version_info);
					if (status == HAL_OK)
			        {
			            packet->mData[1] = TEST_OK;

			            GLogN("HSM Version - Host: %d.%d.%d, HSE: %d.%d.%d, release: %d\r\n",
			                  version_info.host_major, version_info.host_minor, version_info.host_patch,
			                  version_info.hse_major, version_info.hse_minor, version_info.hse_patch, version_info.release);
						break;
					}						
		    	}
				memcpy( &packet->mData[3], &version_info.host_major, 12 );
				memcpy( &packet->mData[15], &version_info.release, sizeof(version_info.release) );
			}
            
            break;
        }

		/***********************************************/
		/***   Line Test   *****************************/
		/***********************************************/
		case TEST_MODE_HCAN1	:			// FDCan1 Tx, FDCan2 Rx
		{
#if 0	//hcan1 tx hcan2 rx loop back test
			stMsgClst	*txMsg, *rxMsg;
			stFdcanPkt	*txPkt, *rxPkt;
			SpiCanPkt_t *rxPktSPI;

			uint8_t		txBuff[8];
			uint32_t	canID;

			GLogI( "[MODE_HCAN1]\r\n");

			packet->mLen	= 2;

			clearRXCanMessage();
			clearTXCanMessage();

			HIGHCAN2_120OHM_ENABLE;

			CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );//for HCAN2
			setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x0, 0x7ff );//for HCAN2

			IO_CONTROL_HIGH( DLCA_EN1 );
			IO_CONTROL_LOW( DLCA_EN2 );
			IO_CONTROL_LOW( DLCA_EN3 );
			IO_CONTROL_HIGH( DLCA_EN6 );
			IO_CONTROL_LOW( DLCA_EN7 );
			IO_CONTROL_LOW( DLCA_EN8_1 );
			IO_CONTROL_LOW( DLCA_EN13 );
			IO_CONTROL_LOW( DLCA_EN15_1 );

			IO_CONTROL_LOW( DLCB_EN8 );
			IO_CONTROL_HIGH( DLCB_EN9 );
			IO_CONTROL_LOW( DLCB_EN10 );
			IO_CONTROL_LOW( DLCB_EN11 );
			IO_CONTROL_LOW( DLCB_EN12 );
			IO_CONTROL_HIGH( DLCB_EN14 );
			IO_CONTROL_LOW( DLCB_EN15 );

			startFDCan( 1, 0, 0 );

			// Tx
			txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
			if( txMsg != NULL )
			{
				txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
				if( txPkt != NULL )
				{
					canID = ( pPayloadPtcl[1] << 8 ) + pPayloadPtcl[2];

					txPkt->mLen			= 8;

					txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;

					for( i = 0; i < 8; i++ )			txBuff[i] = txPkt->mData[i] = pPayloadPtcl[i+3];

					GLogN( "HighCAN1 Tx : %04X ", canID );
					for( i = 0; i < 8; i++ )
					{
						GLogN( "%02X ", txBuff[i] );
					}
					GLogN( "\r\n" );

					makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );

					txMsg->pPacket	= (void *)txPkt;

					osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

					// 2. FDCan2 Rx
					for( i = 0; i < 3; i++ )				// Max wait 3 seconds
					{
						evt = osMessageGet( hFDRxMsg, 1000 );
						if( evt.status == osEventMessage )
						{
							rxMsg	= ( stMsgClst * )evt.value.p;
							rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

							if( ( rxPkt->pSource == &hfdcan1 ) &&
								( rxPkt->mRxHeader.Identifier == canID ) &&
								( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
							{
								packet->mData[1] = TEST_OK;
							}
							GLogN( "HighCAN2 Rx : %04X ", canID );
							for( i = 0; i < 8; i++ )
							{
								GLogN( "%02X ", rxPkt->mData[i] );
							}
							GLogN( "\r\n" );

							osPoolFree( hFdcanPktPool, (void *)rxPkt );
							osPoolFree( hMsgPool, (void *)rxMsg );

							break;
						}
					}
				}
			}

			HIGHCAN2_120OHM_DISABLE;

			SetKL_Line( 0, 0 );
			stopFDCan( &hfdcan1 );

#else
			stMsgClst	*txMsg, *rxMsg;
			stFdcanPkt	*txPkt, *rxPkt;

			uint8_t		txBuff[8];
			uint32_t	canID;

			GLogI( "[MODE_HCAN1]\r\n");

			packet->mLen	= 2;

			clearRXCanMessage();
			clearTXCanMessage();

			HIGHCAN2_120OHM_ENABLE;

#ifdef USE_INTERNAL_CAN_ONLY
			CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

			setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x0, 0x7ff );
			//setFilter2( FDCAN_FILTER_TO_RXFIFO0, 0x0, 0x7ff );
#else
			mcp2518fd_set_baud(CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

#endif
			startFDCan( 1, 0, 0 );

			// Tx
			txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
			if( txMsg != NULL )
			{
				txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
				if( txPkt != NULL )
				{
					canID = ( pPayloadPtcl[1] << 8 ) + pPayloadPtcl[2];

					txPkt->mLen			= 8;
#ifdef USE_INTERNAL_CAN_ONLY
					txPkt->pTarget		= &hfdcan1;
#else
					txPkt->pTarget		= (FDCAN_HandleTypeDef*)SPI5;
#endif
					for( i = 0; i < 8; i++ )			txBuff[i] = txPkt->mData[i] = pPayloadPtcl[i+3];

					GLogN( "HighCAN1 Tx : %04X ", canID );
					for( i = 0; i < 8; i++ )
					{
						GLogN( "%02X ", txBuff[i] );
					}
					GLogN( "\r\n" );

#ifdef USE_INTERNAL_CAN_ONLY
					makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_FDCAN );
#else
					makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );
#endif
					txMsg->pPacket	= (void *)txPkt;

					osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

					// Rx
					for( i = 0; i < 3; i++ )				// Max wait 3 seconds
					{
						evt = osMessageGet( hFDRxMsg, 1000 );
						if( evt.status == osEventMessage )
						{
#ifdef PRINT_MESSAGE_ID
							printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif

							rxMsg	= ( stMsgClst * )evt.value.p;
							rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;
#ifdef USE_INTERNAL_CAN_ONLY
							if( ( rxPkt->pSource == &hfdcan1 ) &&
#else
							if( ( rxPkt->pSource == (FDCAN_HandleTypeDef*)SPI5) &&
#endif
								( rxPkt->mRxHeader.Identifier == canID ) &&
								( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
							{
								packet->mData[1] = TEST_OK;
							}
							GLogN( "HighCAN1 Rx : %04X ", canID );
							for( i = 0; i < 8; i++ )
							{
								GLogN( "%02X ", rxPkt->mData[i] );
							}
							GLogN( "\r\n" );

							osPoolFree( hFdcanPktPool, (void *)rxPkt );
							osPoolFree( hMsgPool, (void *)rxMsg );

							break;
						}
					}
				}
			}

			HIGHCAN2_120OHM_DISABLE;

			SetKL_Line( 0, 0 );
			StopSpiCan();
#ifdef USE_INTERNAL_CAN_ONLY
			stopFDCan( &hfdcan1 );
#endif
			// 220319 test
			//stopFDCan( &hfdcan1 );
#endif
			break;
		}

		case TEST_MODE_HCAN2	:			// FDCan2 Tx, FDCan1 Rx
		{
			stMsgClst	*txMsg, *rxMsg;
			stFdcanPkt	*txPkt, *rxPkt;
			//SpiCanPkt_t *rxPktSPI;
			uint8_t		txBuff[8];
			uint32_t	canID;

			GLogI( "[MODE_HCAN2]\r\n");

			//InitIOCTL();

			packet->mLen	= 2;

			clearRXCanMessage();
			clearTXCanMessage();

			HIGHCAN2_120OHM_ENABLE;

			CanSet_Baud( &hfdcan1, CAN_500KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

			setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x0, 0x7ff );
//			setFilter2( FDCAN_FILTER_TO_RXFIFO0, 0x0, 0x7ff );

			//SetKL_Line( KL_LINE1_CONNECT_CH01, KL_LINE2_CONNECT_CH09 );

			startFDCan( 0, 1, 0 );

			// Tx
			txMsg	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
			if( txMsg != NULL )
			{
				txPkt	= ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
				if( txPkt != NULL )
				{
					canID = ( pPayloadPtcl[1] << 8 ) + pPayloadPtcl[2];

					txPkt->mLen			= 8;

					txPkt->pTarget		= &hfdcan1;
					for( i = 0; i < 8; i++ )			txBuff[i] = txPkt->mData[i] = pPayloadPtcl[i+3];
					GLogN( "HighCAN2 Tx : %04X ", canID );
					for( i = 0; i < 8; i++ )
					{
						GLogN( "%02X ", txBuff[i] );
					}
					GLogN( "\r\n" );

					makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_FDCAN );

					txMsg->pPacket	= (void *)txPkt;

					osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

					// 2. FDCan2 Rx
					for( i = 0; i < 3; i++ )				// Max wait 3 seconds
					{
						evt = osMessageGet( hFDRxMsg, 1000 );
						if( evt.status == osEventMessage )
						{
#ifdef PRINT_MESSAGE_ID
							printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
							rxMsg	= ( stMsgClst * )evt.value.p;
							rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

							if( ( rxPkt->pSource == &hfdcan1 ) &&
								( rxPkt->mRxHeader.Identifier == canID ) &&
								( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
							{
								packet->mData[1] = TEST_OK;
							}
							GLogN( "HighCAN2 Rx : %04X ", canID );
							for( i = 0; i < 8; i++ )
							{
								GLogN( "%02X ", rxPkt->mData[i] );
							}
							GLogN( "\r\n" );

							osPoolFree( hFdcanPktPool, (void *)rxPkt );
							osPoolFree( hMsgPool, (void *)rxMsg );

							break;
						}
					}
				}
			}

			HIGHCAN2_120OHM_DISABLE;

			SetKL_Line( 0, 0 );

			stopFDCan( &hfdcan1 );
			//stopFDCan( &hfdcan2 );

			break;
		}

		case TEST_MODE_LCAN		:
		{
			stMsgClst	*txMsg, *rxMsg;
			stFdcanPkt	*txPkt, *rxPkt;
			uint8_t		txBuff[8];
			uint32_t	canID;

			GLogI( "[MODE_LCAN]\r\n");

			packet->mLen	= 2;

			clearRXCanMessage();
			clearTXCanMessage();

			CanSet_Baud( &hfdcan1, CAN_100KBPS, CAN_DAT_2MBPS, CAN_FRAMEFORMAT_CLASSIC );

			setFilter1( FDCAN_FILTER_TO_RXFIFO0, 0x0, 0x7ff );
			//setFilter2( FDCAN_FILTER_TO_RXFIFO0, 0x0, 0x7ff );

			//SetKL_Line( KL_LINE1_CONNECT_CH01, KL_LINE2_CONNECT_CH09 );
			startFDCan( 0, 0, 1 );

			// Tx
			txMsg = ( stMsgClst* )osPoolCAlloc( hMsgPool );
			if( txMsg != NULL )
			{
				txPkt = ( stFdcanPkt* )osPoolCAlloc( hFdcanPktPool );
				if( txPkt != NULL )
				{
					canID = ( pPayloadPtcl[1] << 8 ) + pPayloadPtcl[2];

					txPkt->mLen			= 8;
					txPkt->pTarget		= &hfdcan1;

					for( i = 0; i < 8; i++ )			txBuff[i] = txPkt->mData[i] = pPayloadPtcl[i+3];

					GLogN( "LowCAN Tx : %04X ", canID );
					for( i = 0; i < 8; i++ )
					{
						GLogN( "%02X ", txBuff[i] );
					}
					GLogN( "\r\n" );

					makeTxHeaderCAN( &txPkt->mTxHeader, canID, CAN_IDTYPE_STANDARD, txPkt->mLen, CAN_FRAMEFORMAT_CLASSIC );

					txMsg->pPacket	= (void *)txPkt;

					osMessagePut( hFDTxMsg, (uint32_t)txMsg, osWaitForever );

					// RX
					for( i = 0; i < 3; i++ )				// Max wait 3 seconds
					{
						evt = osMessageGet( hFDRxMsg, 1000 );
						if( evt.status == osEventMessage )
						{
#ifdef PRINT_MESSAGE_ID
							printf("[Get]:%s\r\n",GetMessageIDName((uint32_t)hFDRxMsg));
#endif
							rxMsg	= ( stMsgClst * )evt.value.p;
							rxPkt	= ( stFdcanPkt* )rxMsg->pPacket;

							GLogN( "LowCAN Rx : %04X ", rxPkt->mRxHeader.Identifier );
							for( i = 0; i < 8; i++ )
							{
								GLogN( "%02X ", rxPkt->mData[i] );
							}
							GLogN( "\r\n" );

							if(	( rxPkt->mRxHeader.Identifier == canID ) &&
								( memcmp( txBuff, rxPkt->mData, 8 ) == 0 ) )
							{
								packet->mData[1] = TEST_OK;
							}

							osPoolFree( hFdcanPktPool, (void *)rxPkt );
							osPoolFree( hMsgPool, (void *)rxMsg );

							break;
						}
					}
				}
				else
				{
					GLogE( "Error... fail alloc Packet!!!\r\n" );
					osPoolFree( hMsgPool, (void *)rxMsg );
				}
			}
			else
			{
				GLogE( "Error... fail alloc message!!!\r\n" );
			}

			SetKL_Line( 0, 0 );
			stopFDCan( &hfdcan1 );

			break;
		}

		/***********************************************/
		/***   RS9116(Wifi/BT) Test   ******************/
		/***********************************************/
		case TEST_MODE_WLAN_VER	:
		{
			uint8_t	wlanFWVer[11] = { 0, };

			GLogI( "[MODE_WLAN_VER]\r\n");

			packet->mLen	= 7;

			rsi_wlan_get( RSI_FW_VERSION, wlanFWVer, sizeof(wlanFWVer) );

#if( PRINT_FUCTIONLIST_DEBUG_MESSAGE )
			GLogI( "Wlan FW Ver : ");
			for( i = 0; i < 6; i++ )
			{
				GLogI( "%c", wlanFWVer[5+i] );
			}
			GLogI( "\r\n");
#endif	// PRINT_FUCTIONLIST_DEBUG_MESSAGE

			//packet->mData[1]	= wlanFWVer[0];
			//packet->mData[2]	= wlanFWVer[1];
			//packet->mData[3]	= wlanFWVer[2];
			//packet->mData[4]	= wlanFWVer[3];
			//packet->mData[5]	= wlanFWVer[4];
			//packet->mData[6]	= wlanFWVer[5];
			packet->mData[1]=wlanFWVer[5];// 2 (0x32)
			packet->mData[2]=wlanFWVer[6];// . (0x2E)
			packet->mData[3]=wlanFWVer[7];// 6 (0x36)
			packet->mData[4]=wlanFWVer[8];// . (0x2E)
			packet->mData[5]=wlanFWVer[9];// 0 (0x30)
			packet->mData[6]=wlanFWVer[10];// . (0x2E)

			break;
		}

		case TEST_MODE_BT_WIFI_MAC	:
		{
			uint8_t macAddr[6] = { 0, };
			//uint8_t Wifi_Status[2];

			GLogI( "[MODE_BT_WIFI_MAC]\r\n");

			packet->mLen	= 14;
/*
			rsi_wlan_get(RSI_CONNECTION_STATUS,Wifi_Status,2);
			if(Wifi_Status[0]!=WIFI_AP_CONNECTED)
			{
				uint8_t pskKey[8]		= { '#', 'g', 'i', 't', '1', '2', '3', '4'};
				for( i = 0; i < 2; i++ )
				{
					if(ConnectAP( "VCI3_test2.4G", RSI_WPA2 , pskKey ) == 0)		break;
					osDelay(10);
				}
				//ConnectAP("0",0,"0");	//only for wifi MAC read sequence
			}

*/
			rsi_wlan_radio_init(); //returns the WLAN MAC address of the module to the host

			if( rsi_wlan_get( RSI_MAC_ADDRESS, macAddr, 6 ) == 0 )
			{
				packet->mData[1] = TEST_OK;
			}
			else
			{
				//break;
			}

			packet->mData[2]	= macAddr[0];
			packet->mData[3]	= macAddr[1];
			packet->mData[4]	= macAddr[2];
			packet->mData[5]	= macAddr[3];
			packet->mData[6]	= macAddr[4];
			packet->mData[7]	= macAddr[5];

#if( PRINT_FUCTIONLIST_DEBUG_MESSAGE )
			GLogI( "WIFI MAC : " );
			for( i = 0; i < 6; i++ )
			{
				GLogI( "%02X-", packet->mData[i+2]);
			}
			GLogI( "\r\n");
#endif	// PRINT_FUCTIONLIST_DEBUG_MESSAGE

			if( rsi_bt_get_local_device_address( macAddr ) != 0 )
			{
				packet->mData[1] = TEST_NG;
				break;
			}

#if( PRINT_FUCTIONLIST_DEBUG_MESSAGE )
			GLogI( "BT   MAC : " );
			for(i = 5; i >0 ; i-- )
			{
				GLogI( "%02X-", macAddr[i] );
			}
			GLogI( "%02X", macAddr[0] );
			GLogI( "\r\n");
#endif	// PRINT_FUCTIONLIST_DEBUG_MESSAGE

			packet->mData[8]	= macAddr[5];
			packet->mData[9]	= macAddr[4];
			packet->mData[10]	= macAddr[3];
			packet->mData[11]	= macAddr[2];
			packet->mData[12]	= macAddr[1];
			packet->mData[13]	= macAddr[0];

			break;
		}

		case TEST_MODE_WIFI_RSSI	:
		{
			GLogI( "[MODE_WIFI_RSSI]\r\n");

			packet->mLen	= 3;

			if( pPayloadPtcl[1] == 1 )									//ConnectAP
			{
				uint8_t ssidName[20]	= { '\0', };
				uint8_t	pskKey[10]		= { '\0', };
				uint8_t	ssidLength		= 0;
				uint8_t	pskLength		= 0;

				pskLength	= pPayloadPtcl[2];
				ssidLength	= pPayloadPtcl[pskLength + 3];

				memcpy( pskKey, &pPayloadPtcl[3], pskLength );
				memcpy( ssidName, &pPayloadPtcl[pskLength + 4], ssidLength );

				GLogN( "===== ssid : %s\r\n", ssidName );
				GLogN( "===== psk  : %s\r\n", pskKey );

				for( i = 0; i < 3; i++ )
				{
					if( ConnectAP( ssidName ,RSI_WPA2 , pskKey) == 0 )
					{
						packet->mData[1] = TEST_OK;
						break;
					}
					osDelay(10);
				}
			}
			else if( pPayloadPtcl[1] == 2 )								//get RSSI
			{
				if( rsi_wlan_get( RSI_RSSI, &packet->mData[2], 2 ) == 0 )			packet->mData[1] = TEST_OK;

				DisconnectWLan();
			}

			break;
		}

		case TEST_MODE_BT_CON_CHK	:
		{
			uint8_t	temp[10] = { 'H', 'E', 'L', 'L', 'O', '_', 'G', 'I', 'T', '!'};

			GLogI( "[MODE_BT_CON_CHK]\r\n");

			packet->mLen	= 2;

			if( g_ucBtConnected == 1 )
			{
				packet->mData[1] = TEST_OK;
			}
			bt_spp_transfer(temp, sizeof(temp));

			break;
		}

		case TEST_MODE_WIFI_CONNECT_24	:
		{
		  	GLogI( "[MODE_WIFI_CONNECT_24]\r\n");

			packet->mLen	= 2;
			if( pPayloadPtcl[1] == 0 )		 							//ConnectAP
			{
				if(TestWifiConnect(SSID_24) == 1)
				{
					GLogN( "===== ssid : %s\r\n", SSID_24 );
					packet->mData[1] = TEST_OK;
				}
				else
				{
				  	packet->mData[1] = TEST_NG;
				}
			}
			else if( pPayloadPtcl[1] == 1 )								//DisconnectAP
			{
				packet->mData[1] = TEST_OK;
				DisconnectWLan();
			}
		  	break;
		}

		case TEST_MODE_WIFI_SEND_24	:
		{
		  	uint8_t	temp[10] = { 'H', 'E', 'L', 'L', 'O', '_', 'G', 'I', 'T', '!'};

			packet->mLen	= 2;

			packet->mData[1] = TEST_OK;
			GLogI( "[MODE_WIFI_SEND_24]\r\n");
		  	WifiPacketSend(temp, sizeof(temp));
		  	break;
		}

		case TEST_MODE_WIFI_CONNECT_50	:
		{
		  	GLogI( "[MODE_WIFI_CONNECT_50]\r\n");

			packet->mLen	= 2;

			if( pPayloadPtcl[1] == 0 )		 							//ConnectAP
			{
				if(TestWifiConnect(SSID_50) == 1)
				{
					GLogN( "===== ssid : %s\r\n", SSID_50 );
					packet->mData[1] = TEST_OK;
				}
				else
				{
				  	packet->mData[1] = TEST_NG;
				}
			}
			else if( pPayloadPtcl[1] == 1 )								//DisconnectAP
			{
				packet->mData[1] = TEST_OK;
				DisconnectWLan();
			}
		  	break;
		}

		case TEST_MODE_WIFI_SEND_50	:
		{
		  	uint8_t	temp[10] = { 'H', 'E', 'L', 'L', 'O', '_', 'G', 'I', 'T', '!'};

			packet->mLen	= 2;

			packet->mData[1] = TEST_OK;
			GLogI( "[MODE_WIFI_SEND_50]\r\n");
		  	WifiPacketSend(temp, sizeof(temp));
		  	break;
		}

		case TEST_MODE_WIFI_DISCONNECT	:
		{
		  	GLogI( "[MODE_WIFI_DISCONNECT]\r\n");

			packet->mLen	= 2;

			packet->mData[1] = TEST_OK;
			DisconnectWLan();
		  	break;
		}


		/***********************************************/
		/***   EMMC Test   *****************************/
		/***********************************************/
		case TEST_MODE_EMMC_TEST	:
		{
			GLogI( "[MODE_EMMC_TEST]\r\n");

			packet->mLen	= 2;

			if( WriteFileTest() == 0 )
			{
				packet->mData[1] = TEST_OK;
			}

			break;
		}

		/***********************************************/
		/***   Sleep Test   ****************************/
		/***********************************************/
		case TEST_MODE_SLEEP_IG	:
		{
			GLogI( "[MODE_SLEEP_IG]\r\n");
			packet->mLen	= 2;
			packet->mData[1] = TEST_OK;

			gPMFlag=WAKEUP_SOURCE_IG;
			g_eMainState=eMain_Sleep;
			//gotoStandbyMode( WAKEUP_SOURCE_IG );
			break;
		}

		case TEST_MODE_SLEEP_SENSOR	://GYRO
		{
			GLogI( "[MODE_SLEEP_SENSOR]\r\n");
			packet->mLen	= 2;
			packet->mData[1] = TEST_OK;

			gPMFlag=WAKEUP_SOURCE_SENSOR;
			g_eMainState=eMain_Sleep;
			//gotoStandbyMode( WAKEUP_SOURCE_SENSOR );
			break;
		}

		case TEST_MODE_SLEEP_HCAN1	:
		{
			GLogI( "[MODE_SLEEP_HCAN1]\r\n");
			packet->mLen	= 2;
			packet->mData[1] = TEST_OK;

#ifdef USE_INTERNAL_CAN_ONLY
			gPMFlag=WAKEUP_SOURCE_HCAN1;
#else
			gPMFlag=WAKEUP_SOURCE_HCAN1;
#endif
			g_eMainState=eMain_Sleep;
			//gotoStandbyMode( WAKEUP_SOURCE_HCAN1 );
			break;
		}

		case TEST_MODE_SLEEP_HCAN2	:
		{
			GLogI( "[MODE_SLEEP_HCAN2]\r\n");
			packet->mLen	= 2;
			packet->mData[1] = TEST_OK;

			gPMFlag=WAKEUP_SOURCE_HCAN2;
			g_eMainState=eMain_Sleep;
			//gotoStandbyMode( WAKEUP_SOURCE_HCAN2 );
			break;
		}

		case TEST_MODE_SLEEP_LCAN	:
		{
			GLogI( "[MODE_SLEEP_LCAN]\r\n");
			packet->mLen	= 2;
			packet->mData[1] = TEST_OK;

			gPMFlag=WAKEUP_SOURCE_LCAN;
			g_eMainState=eMain_Sleep;
			//gotoStandbyMode( WAKEUP_SOURCE_LCAN );
			break;
		}

		case TEST_MODE_SLEEP_TRG	:
		{
			GLogI( "[MODE_SLEEP_TRG]\r\n");
			packet->mLen	= 2;
			packet->mData[1] = TEST_OK;

			gPMFlag=WAKEUP_SOURCE_TRG;
			g_eMainState=eMain_Sleep;
			//gotoStandbyMode( WAKEUP_SOURCE_TRG );
			break;
		}

		case TEST_MODE_SLEEP_12V	:
		{
			GLogI( "[MODE_SLEEP_12V]\r\n");
			packet->mLen	= 2;
			packet->mData[1] = TEST_OK;

			gPMFlag=WAKEUP_SOURCE_12V_DET;
			g_eMainState=eMain_Sleep;
			//gotoStandbyMode( WAKEUP_SOURCE_12V_DET );
			break;
		}

		case TEST_MODE_SLEEP_24V	:
		{
			GLogI( "[MODE_SLEEP_24V]\r\n");
			packet->mLen	= 2;
			packet->mData[1] = TEST_OK;

			gPMFlag=WAKEUP_SOURCE_24V_DET;
			g_eMainState=eMain_Sleep;
			//gotoStandbyMode( WAKEUP_SOURCE_24V_DET );
			break;
		}

		/**************************************************/
		/***   Ethernet Test   ****************************/
		/**************************************************/
		case TEST_MODE_ETH_TX:
		case TEST_MODE_ETH_T1:
		case TEST_MODE_EMMC_LOCK_TEST:
		{
			//static unsigned int uOldTimer 	= 0;
			//stEthDiagSocket_t *ethSocket = NULL;
			error_t error;
			uint8_t EthDiagMsg[9] = {0x48, 0x45, 0x4C, 0x4C, 0x4F, 0x20, 0x47, 0x49, 0x54};//"HELLO GIT"
			uint8_t recevieBuf[9] = {0,};
			uint8_t CompCnt=0;
			//uint8_t data=0;

			if(testMode == TEST_MODE_ETH_T1){
				DisableEthDiag();
#ifdef LAN_9514
				DisableUSB3300();
				DisableLAN9514();
#else
				DisableLAN9371();
#endif
				osDelay(10);
#ifdef LAN_9514
				EnableUSB3300();
				EnableLAN9514();
#else
				EnableLAN9371();
#endif
				osDelay(5000);
				GLogI( "[MODE_ETH_T1]\r\n");
			}
			else
			{
				/* mod.kks to fix the stable ethernet line */
				InitIOCTL();
				EnableEthDiag();
	            		osDelay(10);
#ifdef LAN_9514
				EnableUSB3300();
				EnableLAN9514();
#else
				EnableLAN9371();
#endif
	            		osDelay(5000);
                		GLogI( "[MODE_ETH_TX]\r\n");
	            /* end */
			}

			packet->mLen	= 2;

			packet->mData[0]=testMode;
			packet->mData[1]=TEST_OK;//0:OK,  1:NG
#if defined (KKT_TEST)
			//g_stGITHWSetDataEth.nSourceIP_T1 = 0x4D00A8C0;		//192.168.0.77
			//g_stGITHWSetDataEth.nSourcePort_T1 = 52081;
			g_stGITHWSetDataEth.nDestinationIp_T1 = 0x6400A8C0;	//192.168.0.100
			g_stGITHWSetDataEth.nDestinationPort_T1 = 13402;
			g_ethSocket.state = eSOCK_STATE_INIT;
			//eth_SetIPAddressStatic( RS9116_IPV4_ADDR( 192, 168, 0, 77 ), RS9116_IPV4_ADDR( 255, 255, 255, 0 ), RS9116_IPV4_ADDR( 192, 168, 0, 1 ) );  //jkc NEW
#else
			//g_stGITHWSetDataEth.nSourceIP_T1 = 0x0005000A;		//10.0.5.0
			//g_stGITHWSetDataEth.nSourcePort_T1 = 52081;
			g_stGITHWSetDataEth.nDestinationIp_T1 = 0x0000200A;	//10.32.0.0
			g_stGITHWSetDataEth.nDestinationPort_T1 = 13402;
			g_ethSocket.state = eSOCK_STATE_INIT;
			//eth_SetIPAddressStatic( RS9116_IPV4_ADDR( 10, 0, 5, 0 ), RS9116_IPV4_ADDR( 255, 0, 0, 0 ), RS9116_IPV4_ADDR( 10, 0, 128, 1 ) );  //jkc NEW
#endif
			GLogI( "Source IP : %s \r\n",APP_IF1_IPV4_HOST_ADDR );
			GLogI( "Source PORT : %d \r\n",TCP_LOCL_PORT );
			GLogI( "Source SUBNET_MASK : %s \r\n",APP_IF1_IPV4_SUBNET_MASK );
			GLogI( "Source GATEWAY : %s \r\n",APP_IF1_IPV4_DEFAULT_GATEWAY );
			GLogI( "\r\n");
			GLogI( "Target IP : %s \r\n",TCP_TARGET_NAME );
			GLogI( "Target PORT : %d \r\n",TCP_TARGET_PORT );

			setEthSocketState(&g_ethSocket, eSOCK_STATE_INIT);

			g_ethSocket.state = ethTCPConnect(&g_ethSocket);

			if(g_ethSocket.state == eSOCK_STATE_CONNECTED)
			{
				GLogI( "Socket Connect!!! \r\n" );
				error = tcpSend(g_ethSocket.socket, EthDiagMsg, sizeof(EthDiagMsg), NULL, FALSE);
				GLogI( "TX : HELLO GIT \r\n" );


				error = socketReceive(g_ethSocket.socket, recevieBuf, sizeof(recevieBuf), NULL, FALSE);
				if(!error)
				{
					GLogI("RX : ");

					CompCnt=0;
					for(i = 0; i<9; i++)
					{
						//GLogI("%02X ", recevieBuf[i]);
						GLogI("%c", recevieBuf[i]);
						if(recevieBuf[i]==EthDiagMsg[i])CompCnt++;
					}
					if(CompCnt!=9) GLogE( "Recevie Data Diff!!! \r\n" );
					GLogI("\r\n");
				}
			}
			else
			{
				packet->mData[1]=TEST_NG;//0:OK,  1:NG
				GLogE( "Socket Connect Fail!!! \r\n" );
			}

			//LAN9371_SPI_ReadByte( 0x3000 + 0x30, &data );
	        //GLogN( "[KKS TEST] 0x3000 + 0x30 : 0x%02x\r\n", data );

			DisableEthDiag();
#ifdef LAN_9514
			DisableUSB3300();
			DisableLAN9514();
#else
            		DisableLAN9371(); //mod.kks to fix the ethernet line.
#endif
			//osDelay(2000);	//JIG socket close time


			socketShutdown(g_ethSocket.socket,2);GLogI( "socketShutdown!!! \r\n" );
			socketClose(g_ethSocket.socket);GLogI( "socketClose!!! \r\n" );

			break;
		}
        case TEST_MODE_ETH_ENABLE:
        {
            GLogI( "[TEST_MODE_ETH_ENABLE]\r\n");

            ETHERNET_LINE_CONTROL();
            EnableEthDiag();

            packet->mLen	= 2;
            packet->mData[1] = TEST_OK;
            break;
         }
        case TEST_MODE_ETH_DISABLE:
		{
            GLogI( "[TEST_MODE_ETH_DISABLE]\r\n");

            VCI_Clear_DLC_HW();
            DisableEthDiag();

            packet->mLen	= 2;
            packet->mData[1] = TEST_OK;
            break;
		}

        case TEST_MODE_HSM_VERSION:
        {
            ret = HSM_Version_Check((int*)&packet->mData[2]);
            if(ret == HSM_SUCCESS) packet->mData[1] = TEST_OK;
            else packet->mData[1] = TEST_NG;
            packet->mLen	= 4;
            
            break;
        }
        case TEST_MODE_HSM_CHECK:
        {
            uint8_t data[600];
            U32 i = 0, len = 0, index = 0;
            U32 uiReturnCode = 200;
            uint8_t bit = 8;
            bool checkvalue = 0;
            uint8_t res = 0, temp = 0;
            
            packet->mData[1] = TEST_OK;
            
            uiReturnCode = ActivationHSM();
            for(index=1; index<5; index++)
            {
                memset(data, 0x00, sizeof(data));
                checkvalue = false;
                uiReturnCode = ReadCertificateHSM(&data[0], (int*)&len, index);
                bit--;
                
                for(i=0; i<len; i++)
                {
                    if(data[i] != 0) 
                    {
                        checkvalue=true; 
                        break;
                    }
                    else{}
                }
                
                if( checkvalue )
                {
                    temp = 1;
                    temp = temp << bit;
                    res |= temp;
                }
                
                if(uiReturnCode != HSM_SUCCESS) packet->mData[1] = TEST_NG;
            }
            
            for(index=1; index<5; index++)
            {
                memset(data, 0x00, sizeof(data));
                checkvalue = false;
                uiReturnCode = ReadDataHSM(&data[0], len, index);
                bit--;
                
                for(i=0; i<len; i++)
                {
                    if(data[i] != 0) 
                    {
                        checkvalue=true; 
                        break;
                    }
                    else{}
                }
                
                if( checkvalue )
                {
                    temp = 1;
                    temp = temp << bit;
                    res |= temp;
                }
                
                if(uiReturnCode != HSM_SUCCESS) packet->mData[1] = TEST_NG;
            }
            
            packet->mData[2] = res;
            packet->mLen = 3;
            
            break;
        }
        case TEST_MODE_HSM_UPDATE:
        {
            if((pHSMAck->mSelftestMode == true)&&((pHSMAck->mProgress == 100) || (pHSMAck->mResult == 1))) // (selftestmode && (100% || Fail))
            {
                pHSMAck->mSelftestMode = false;
                //bhsmupflag = false;
            }
            else
            {
                pHSMAck->mResult = 0;
                pHSMAck->mProgress = 0;
                pHSMAck->mSelftestMode = true;
                
                //bhsmupflag = true;
                //memcpy(ucauthkey, &pPayloadPtcl[1], 80);
                VCI3_HSM_UPDATE(&pPayloadPtcl[1]);
            }
            
            packet->mData[1] = pHSMAck->mResult; //Result
            packet->mData[2] = pHSMAck->mProgress; //Progress
                
            packet->mLen = 3;
            
            break;
        }


		/***********************************************/
		/***   SSL Certificate Write/Import   **********/
		/***********************************************/
		case TEST_MODE_SSL_CERT_WRITE_START:
		{
			GLogI("[SSL_CERT_WRITE_START]\r\n");

			packet->mLen = 2;

			FRESULT fres;
			fres = f_mkdir("/cert");
			if(fres != FR_OK && fres != FR_EXIST)
			{
				GLogE("f_mkdir /cert fail: %d\r\n", fres);
				packet->mData[1] = TEST_NG;
				break;
			}

			FIL certFpStart;
			fres = f_open(&certFpStart, "/cert/ssl_ca.der", FA_CREATE_ALWAYS | FA_WRITE);
			if(fres == FR_OK)
			{
				f_close(&certFpStart);
				packet->mData[1] = TEST_OK;
				GLogN("cert file created\r\n");
			}
			else
			{
				packet->mData[1] = TEST_NG;
				GLogE("f_open fail: %d\r\n", fres);
			}

			break;
		}
		case TEST_MODE_SSL_CERT_WRITE_UPDATE:
		{
			GLogI("[SSL_CERT_WRITE_UPDATE]\r\n");

			packet->mLen = 2;

			uint16_t dataLen = 0;
			memcpy(&dataLen, &pPayloadPtcl[1], sizeof(dataLen));

			static FIL g_certFp;
			FRESULT fres;
			UINT bw;

			fres = f_open(&g_certFp, "/cert/ssl_ca.der", FA_OPEN_APPEND | FA_WRITE);
			if(fres != FR_OK)
			{
				packet->mData[1] = TEST_NG;
				GLogE("f_open fail: %d\r\n", fres);
				break;
			}

			fres = f_write(&g_certFp, &pPayloadPtcl[3], dataLen, &bw);
			f_close(&g_certFp);

			if(fres == FR_OK && bw == dataLen)
			{
				packet->mData[1] = TEST_OK;
			}
			else
			{
				packet->mData[1] = TEST_NG;
				GLogE("f_write fail: fres=%d, bw=%d\r\n", fres, bw);
			}

			break;
		}
		case TEST_MODE_SSL_CERT_WRITE_END:
		{
			GLogI("[SSL_CERT_WRITE_END]\r\n");

			packet->mLen = 2;

			FILINFO fno;
			FRESULT fres = f_stat("/cert/ssl_ca.der", &fno);
			if(fres == FR_OK && fno.fsize > 0)
			{
				packet->mData[1] = TEST_OK;
				GLogN("cert saved: %d bytes\r\n", fno.fsize);
			}
			else
			{
				packet->mData[1] = TEST_NG;
				GLogE("cert file verify fail\r\n");
			}

			break;
		}
		case TEST_MODE_SSL_CERT_IMPORT:
		{
			GLogI("[SSL_CERT_IMPORT]\r\n");

			packet->mLen = 2;

			FIL certFp;
			FRESULT fres;
			FILINFO fno;
			UINT br;

			fres = f_stat("/cert/ssl_ca.der", &fno);
			if(fres != FR_OK || fno.fsize == 0)
			{
				packet->mData[1] = TEST_NG;
				GLogE("cert file not found\r\n");
				break;
			}

			#define SSL_CERT_BUF_SIZE	6144
			static uint8_t certBuf[SSL_CERT_BUF_SIZE];

			if(fno.fsize > SSL_CERT_BUF_SIZE)
			{
				packet->mData[1] = TEST_NG;
				GLogE("cert too large: %d > %d\r\n", fno.fsize, SSL_CERT_BUF_SIZE);
				break;
			}

			fres = f_open(&certFp, "/cert/ssl_ca.der", FA_READ);
			if(fres != FR_OK)
			{
				packet->mData[1] = TEST_NG;
				GLogE("f_open fail: %d\r\n", fres);
				break;
			}

			fres = f_read(&certFp, certBuf, fno.fsize, &br);
			f_close(&certFp);

			if(fres != FR_OK || br != fno.fsize)
			{
				packet->mData[1] = TEST_NG;
				GLogE("f_read fail\r\n");
				break;
			}

			int32_t rsi_status = rsi_wlan_set_certificate(RSI_SSL_CA_CERTIFICATE, certBuf, br);

			if(rsi_status == RSI_SUCCESS)
			{
				packet->mData[1] = TEST_OK;
				GLogN("cert import OK: %d bytes\r\n", br);
			}
			else
			{
				packet->mData[1] = TEST_NG;
				GLogE("rsi_wlan_set_certificate fail: 0x%X\r\n", rsi_status);
			}

			break;
		}
        
		case TEST_MODE_PRODUCTION_MODE:
		{
			GLogI("[PRODUCTION_USB_BT_BLOCK]\r\n");

			packet->mLen = 2U;

			if (pPayloadPtcl[1] == 0x00U)			/* 0x00 = block */
			{
				g_bUsbBtBlocked = 1U;
				GLogN("USB/BT BLOCKED\r\n");
			}
			else if (pPayloadPtcl[1] == 0x01U)		/* 0x01 = unblock */
			{
				g_bUsbBtBlocked = 0U;
				GLogN("USB/BT UNBLOCKED\r\n");
			}

			packet->mData[1] = g_bUsbBtBlocked;

			break;
		}

		default:			GLogN( "Not Support TestMode : 0x%04x\r\n", testMode );				break;
	}

#if( MONITOR_RESPONS_PACKET_DATA )
	for( i = 0; i < packet->mLen; i++ )
	{
		GLogI( "packet->mData[%d] : %02X\r\n", i, packet->mData[i] );
	}
#endif	// MONITOR_RESPONS_PACKET_DATA

	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	while( osMessageAvailableSpace( hTransmitMsg ) == 0 )
	{
		osDelay( 1 );
	}

	osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
    osSemaphoreWait(hpairflagSemaphore, osWaitForever);
	
	
    /*if( bhsmupflag )
    {
        VCI3_HSM_UPDATE(&ucauthkey[0]);
    }*/
}
#endif

void FL_EMMC_Init(stCommPkt *pkt, uint32_t eInCommType )
{
	GLogI( "[MODE_EMMC_INIT]\r\n" );
	if( InitEmmcFolder() == 0 )		GLogN( "OK...\r\n" );
}

void FL_NotSupport( uint32_t iden )
{
	GLogEE( ">>> Not Support FunctionID : 0x%04x\r\n", iden );
}

uint32_t TestReprogramLine( void )
{
	uint32_t	ret		= 0;
	uint8_t		i;

	GPIO_PinState		checkHigh;
	GPIO_PinState		checkLow;

	GLogI( "[ Reprogram Switch Test ]\r\n" );

	IO_CONTROL_LOW( KL_RXD1_SEL );
	IO_CONTROL_LOW( KL_RXD1_INV );

	for( i = 0; i < 3; i++ )
	{
		switch( i )
		{
			case 0 :	SetReprogramVol( REPG_LINE_CH03 );	SetKL_Line( KL_LINE1_CONNECT_CH03, 0 );		GLogN( "Reprogram CH03 Test\r\n" );		break;
			case 1 :	SetReprogramVol( REPG_LINE_CH06 );	SetKL_Line( KL_LINE1_CONNECT_CH06, 0 );		GLogN( "Reprogram CH06 Test\r\n" );		break;
			case 2 :	SetReprogramVol( REPG_LINE_CH13 );	SetKL_Line( KL_LINE1_CONNECT_CH13, 0 );		GLogN( "Reprogram CH13 Test\r\n" );		break;
		}

		osDelay( 20 );

		checkHigh	= IO_CONTROL_GET( KL_RXD1 );

		IO_CONTROL_LOW( REPG_ON  );

		osDelay( 1000 );

		checkLow	= IO_CONTROL_GET( KL_RXD1 );

		if( ( checkHigh != GPIO_PIN_SET ) || ( checkLow != GPIO_PIN_RESET ) )
		{
			switch( i )
			{
				case 0 :	ret	|= ERROR_REPROGRAM_CH03;				break;
				case 1 :	ret	|= ERROR_REPROGRAM_CH06;				break;
				case 2 :	ret	|= ERROR_REPROGRAM_CH13;				break;
			}
		}
	}

	IO_CONTROL_LOW( KL_RXD2_SEL );
	IO_CONTROL_LOW( KL_RXD2_INV );

	for( i = 0; i < 4; i++ )
	{
		switch( i )
		{
			case 0 :	SetReprogramVol( REPG_LINE_CH09 );	SetKL_Line( 0, KL_LINE2_CONNECT_CH09 );		GLogN( "Reprogram CH09 Test\r\n" );		break;
			case 1 :	SetReprogramVol( REPG_LINE_CH11 );	SetKL_Line( 0, KL_LINE2_CONNECT_CH11 );		GLogN( "Reprogram CH11 Test\r\n" );		break;
			case 2 :	SetReprogramVol( REPG_LINE_CH12 );	SetKL_Line( 0, KL_LINE2_CONNECT_CH12 );		GLogN( "Reprogram CH12 Test\r\n" );		break;
			case 3 :	SetReprogramVol( REPG_LINE_CH14 );	SetKL_Line( 0, KL_LINE2_CONNECT_CH14 );		GLogN( "Reprogram CH14 Test\r\n" );		break;
		}

		osDelay( 20 );

		checkHigh	= IO_CONTROL_GET( KL_RXD2 );

		IO_CONTROL_LOW( REPG_ON  );

		osDelay( 1000 );

		checkLow	= IO_CONTROL_GET( KL_RXD2 );

		if( ( checkHigh != GPIO_PIN_SET ) || ( checkLow != GPIO_PIN_RESET ) )
		{
			switch( i )
			{
				case 0 :	ret	|= ERROR_REPROGRAM_CH09;				break;
				case 1 :	ret	|= ERROR_REPROGRAM_CH11;				break;
				case 2 :	ret	|= ERROR_REPROGRAM_CH12;				break;
				case 3 :	ret	|= ERROR_REPROGRAM_CH14;				break;
			}
		}
	}

	SetReprogramVol( 0 );
	SetKL_Line( 0, 0 );

	return ret;
}

uint8_t TestWifiConnect (uint8_t* ssid)
{
	uint8_t	pskKey[10]		= { '0', '0', '0', '0', '0', '0', '9', '2', '6', '2'};

	if( ConnectAP( ssid ,RSI_WPA2 , pskKey) == 0 )
	{
		if(SetIPAddressDHCP() == 0)
		{
		  	GLogN( "Socket Server IP is %04X\r\n", g_uiSocketSeverIP);
			if(connectToWebsocket() == 0)
			{
				GLogI( "connectToWebsocket Success \r\n" );
				return 1;
			}
			else
			{
				GLogI( "connectToWebsocket FAIL \r\n" );
				DisconnectWLan();
			}
		}
		else
		{
			GLogI( "SetIPAddressDHCP FAIL \r\n" );
			DisconnectWLan();
		}
	}
	else
	{
		GLogI( "ConnectAP FAIL \r\n" );
		DisconnectWLan();
	}

	return 0;
}

void GitReturnBack( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t 			*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= usLength;

	memcpy(&packet->mData[0], pPayloadPtcl, usLength);
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x2200;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_Git_FWUpdate_Check(stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    U16         u16Temp=0;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( "\r\n>>> Start %s\r\n", __FUNCTION__ );
#endif

//	g_bWifiAutoConDisable=0;//bt ?�??wifi disable ?????????
//	g_WifiAutoConnectStop=0;

	//GLogI( "WIFI enable2\r\n" );

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	g_u32CheckSumTemp = u16Temp = CheckVCIFWCheckSum();
	packet->mLen	= 2;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0455;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
    memcpy(packet->mData, &u16Temp, sizeof(U16));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_Git_FWUpdate_Check_CRC32(stCommPkt *pkt, uint32_t eInCommType )
{
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif
    /* This Function's mFuncID = 0xD062; */
    osMessagePut( hDecompressionMsg, eInCommType, osWaitForever );
#if 0
	stCommPkt	*packet;
	stMsgClst	*message;
    U16         u16Temp=0;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( "\r\n>>> Start %s\r\n", __FUNCTION__ );
#endif

//	g_bWifiAutoConDisable=0;//bt ?�??wifi disable ?????????
//	g_WifiAutoConnectStop=0;

	//GLogI( "WIFI enable2\r\n" );

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	g_u32CheckSumTemp = u16Temp = CheckVCIFWCheckSum();
	packet->mLen	= 2;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0455;

    memcpy(packet->mData, &u16Temp, sizeof(U16));

	/* osPoolCAlloc has a sizeof() bug that only zeros 4 bytes, so packet->UUID
	 * holds leftover data. Use the UUID cached at 0x0155 FW Update Start so the
	 * whole download session carries a consistent UUID for server matching. */
	memcpy(&(packet->UUID), &g_FWUpdate_UUID, sizeof(UUID_Struct));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
#endif
}
#endif
								  
#ifdef VCI3_DIAG
void FL_Git_CV_Check(stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	//U16 		u16Temp=0;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( "\r\n>>> Start %s\r\n", __FUNCTION__ );
#endif

	GLogI( "FL_Git_CV_Check\r\n" );

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 4;

	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x0555;
	packet->mData[0]=0xFF;
	packet->mData[1]=0xFF;
	packet->mData[2]=0xFF;
	packet->mData[3]=0xFF;

//	memcpy(packet->mData, &u16Temp, sizeof(U16));
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_GitSetFirmwareList( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    BOOL 		bResult;
	uint8_t		*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

    bResult = UpdateAppListFromRemoteDevice(pPayloadPtcl, usLength);
    if ( bResult )
    {
        gsFwInfo.mucUpdated = true;
        packet->mData[0] = 0;
    }
    else
    {
        gsFwInfo.mucUpdated = false;
        packet->mData[0] = 1;
    }

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0341;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void FL_GitGetFirmwareList( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    BOOL 		bResult;
	uint32_t	usLength = pkt->mLen - 8;
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen = 1;

	f_chdir(DIR_ROOT);

    bResult = GetUpdateAppListFromEMMC(&packet->mData[1], &usLength);
    if ( bResult )
	{
	  	packet->mData[0] = 0;
		packet->mLen += usLength;
	}
    else
	{
	  	packet->mData[0] = 1;
	}

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0340;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_Git_FWUpdate_End(stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	unsigned char cResult = 0x00;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	gsFwInfo.msAppInfo[g_ucDownloadFW_FileNo].mCheckSum = g_u32CheckSumTemp;
	gsFwInfo.msAppInfo[g_ucDownloadFW_FileNo].mSize = g_u32UpdateFileSize;
	gsFwInfo.msAppInfo[g_ucDownloadFW_FileNo].mSectorCount = (g_u32UpdateFileSize/131072 + 1);
	
	cResult = DownloadClose();

	//g_bWifiConnCheckDisable=0;
    GLogN("@@DeltaTime : %d", Get_TmrDelta( Get_Tmr(), myrecvDeltaTime ));

	if     ( cResult == FR_TIMEOUT )                                        cResult = 0x02;     // Time Out
    else if( cResult != FR_OK )                                             cResult = 0x08;     // File Close Fail

#ifdef VCI3_RECOVERY
    if(cResult != FR_OK)
    {
        formatEmmc();
		InitEmmcFolder();
    }
#endif

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0355;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
    packet->mData[0]= cResult;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_Git_FWUpdate_Recv( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t		*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	unsigned char cResult;

#if 1//defined(DEBUG_GIT_PTCL_LOG)
	GLogN(".");
	//GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	cResult = WriteUpdateDataTemp(pPayloadPtcl, usLength);
    if     ( cResult == FR_TIMEOUT )                                        cResult = 0x02;     // Time Out
    else if( cResult != FR_OK )                                             cResult = 0x07;     // File Write Fail

	packet->mLen	= 1;

    packet->mData[0] = cResult;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0255;
	packet->mCurFrame = 0;

	/* Use UUID cached at 0x0155 FW Update Start so this ack goes out with the
	 * same UUID as the rest of the download session (server requires it). */
	memcpy(&(packet->UUID), &g_FWUpdate_UUID, sizeof(UUID_Struct));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_Git_FWUpdate_Recv4K( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t		*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	unsigned char cResult;

#if 1
	GLogN(".");
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
	
	/* CDP�� �ش� Function ID ����Ұ�� �ش� �ڵ常 ����ϸ��	*/
	cResult = WriteUpdateDataTemp(pPayloadPtcl, usLength);
	/*													*/

    if     ( cResult == FR_TIMEOUT )	cResult = 0x02;     // Time Out
    else if( cResult != FR_OK )			cResult = 0x07;     // File Write Fail

	packet->mLen	= 1;
    packet->mData[0] = cResult;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0255;

	/* Use UUID cached at 0x0155 FW Update Start so this ack goes out with the
	 * same UUID as the rest of the download session (server requires it). */
	memcpy(&(packet->UUID), &g_FWUpdate_UUID, sizeof(UUID_Struct));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		//osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif
								  
#ifdef VCI3_DIAG
void FL_Git_FWUpdate_RecvCDP( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t		*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	unsigned char cResult;

#if 0
	GLogN(".");
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
	
	g_fwupdate_crc32 = ((uint32_t)pPayloadPtcl[0] << 24) | ((uint32_t)pPayloadPtcl[1] << 16) | ((uint32_t)pPayloadPtcl[2] << 8) | ((uint32_t)pPayloadPtcl[3]);

    if     ( cResult == FR_TIMEOUT )	cResult = 0x02;     // Time Out
    else if( cResult != FR_OK )			cResult = 0x07;     // File Write Fail

	packet->mLen	= 1;
    packet->mData[0] = cResult;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0257;

	/* Use UUID cached at 0x0155 FW Update Start so this ack goes out with the
	 * same UUID as the rest of the download session (server requires it). */
	memcpy(&(packet->UUID), &g_FWUpdate_UUID, sizeof(UUID_Struct));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif
bool IsWritingFail = false;
int SequenceIndex = 0;
int WriteIndex = 0;
#ifdef VCI3_DIAG
void FL_Git_FWUpdate_Start( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	unsigned char cResult = 0x00;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	/* Cache UUID for use across the FW download session.
	 * Subsequent 0x0255/0x0257/0x0455 acks are sent from contexts that may
	 * not have direct access to the originating request, and the response
	 * packet's UUID field holds leftover data from osPoolCAlloc (which has
	 * a sizeof() bug that only zeros 4 bytes). Pinning a single UUID for
	 * the whole session keeps the server's session matching stable. */
	memcpy(&g_FWUpdate_UUID, &(pkt->UUID), sizeof(UUID_Struct));
//	if(bt_connected == BT_SPP_CONNECT)
//	{ //If BT connected and wifi enabled
//		GLogI( "BT enable, WIFI disable\r\n" );
//		if( ( 0!=HAL_GPIO_ReadPin( WIFI_ON_OFF_INT_GPIO_Port, WIFI_ON_OFF_INT_Pin ) ) )//wifi button On
//		{
//			DisconnectWLan( );
//			WIFI_R_LED_OFF;
//			g_bWifiAutoConDisable=1;
//			g_WifiAutoConnectStop=1;
//		}
//	}
//g_bWifiConnCheckDisable=1; //WIFI connection check disable
    myrecvDeltaTime = Get_Tmr();

    SequenceIndex = 0;
    WriteIndex = 0;
    IsWritingFail = false;

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	if( !getMountStatus() )		                                    			cResult = 0x03;     // SD Card Not Insert
//	else if( !isWriteProtected() )			                                    cResult = 0x04;     // SD Card is Protected
	else
    {
        cResult = GetUpgradeFileInfo(pPayloadPtcl);
        if     ( cResult == FR_TIMEOUT )                                        cResult = 0x02;     // Time Out
        else if( cResult == FR_NO_FILESYSTEM || cResult == FR_INVALID_DRIVE )   cResult = 0x06;     // File System Fail
        else if( cResult != FR_OK )                                             cResult = 0x05;     // File Create Fail
    }
#ifdef VCI3_RECOVERY
    if(cResult != FR_OK)
    {
        formatEmmc();
		InitEmmcFolder();
    }
#endif

    packet->mData[0]	= cResult;

	packet->pTarget		= &eInCommType;
	packet->mFuncID		= 0x0155;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mCS 		= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void FL_GitFormatExMemory(stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	if( !getMountStatus() )					packet->mData[0] 	= 0x03;
	//else if( !isWriteProtected() )			packet->mData[0] 	= 0x04;
	else if( !formatEmmc() )				packet->mData[0] 	= 0x05;
	else 									packet->mData[0] 	= 0x00;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x3001;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitFWModeChange( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	BYTE cSwitchAppNumber;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0245;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]= 0;//

	cSwitchAppNumber = pPayloadPtcl[0];

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}

	RunModeSwitch(cSwitchAppNumber);
}

void FL_GitFWModeSwitch( stCommPkt *pkt, uint32_t eInCommType  )
{
  	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	BYTE mode;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1200;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]= 0;//

	mode = pPayloadPtcl[0];

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}

	VciChangeMode(mode);
}

#ifdef VCI3_DIAG
#if defined ( SAVE_CAN_LOG )
void FL_GitSetCANLog( stCommPkt *pkt, uint32_t eInCommType  )
{
  	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	uint8_t cResult;
	uint8_t mode;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mData[0]  = pPayloadPtcl[0];
	packet->mLen	= 1;
	mode = pPayloadPtcl[0];

	cResult = VCI_SetCANLog( mode, &pPayloadPtcl[0], packet->mData);
	GLogN( "FL_GitSetCANLog:%d\r\n", cResult );

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0342;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mData[0]= 0;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#else
void FL_GitSetCANLog( stCommPkt *pkt, uint32_t eInCommType  )
{
  	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t		*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x0342;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mData[0]  = pPayloadPtcl[0]; 	// Mode
	packet->mData[1]  = 0;					// Result
	packet->mLen	= 2;

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif
#endif

void FL_GitAMVersion( stCommPkt *pkt, uint32_t eInCommType  )
{
	//uint8_t *pPayloadPtcl = pkt->mData;
	short int AMVersion=-1;

	stCommPkt	*packet;
	stMsgClst	*message;
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 2;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x2211;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	memcpy(&packet->mData[0], &AMVersion, sizeof(AMVersion));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_GitPassThruRetryReadMsgs( stCommPkt *pkt, uint32_t eInCommType  )
{
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;
	//U32 uiProtocolID;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

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
	pDiagmsg->mMsgType 		= MSG_DIAG;
	pDiagmsg->mPktType 		= (ePKT_TD)eInCommType;
	pDiagmsg->event 		= DIAG_PASSTHRU;
	pDiagmsg->unEventTime 	= GetUnixTime();
	pDiagmsg->pPacket 		= (void *)pPTpacket;
	//uiProtocolID = VCI_GetPassThruProtocolID();

	g_b3002Lock = true;
	if(VCI_ProtocolClassification()==eCAN_TX_NONE_PARSING)	//CAN
	{
		if((g_ulProtocolID == ISO14229_ES95486_170 )
				||(g_ulProtocolID == ISO14229_ES95486_170_MMCAN)
				||(g_ulProtocolID == ISO14230_ES95486_170)
				||(g_ulProtocolID == ISO14230_ES95486_170_MMCAN)
                ||(g_ulProtocolID == ISO14229_ES95486_02_100_NEW)     //20190918 Jay
#ifdef HOTA
                ||(g_ulProtocolID == ISO14229_ES95486_02_HOTA)
#endif  
                  
#ifdef CANFD_PROTOCOL
                ||(g_ulProtocolID == ISO14229_ES95486_02_130_CANFD)
                ||(g_ulProtocolID == ISO14229_ES95486_02_131_CANFD)
#endif
				||(g_ulProtocolID == ISO14229_ES95486_02_100)
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
				)
        {
            //Counter = 0;
			//intTxdRxdCount = VP3_MAX;
			//MsgTemp.DataSize = 0;

			//CurMode = CAN_RX_BLOCK;
		 	//Ackflag=0;
		 	g_bAckflag=0;
			//UartPrintf(&U1,"\r\n ES95486 0x3002!! %d",intTxdRxdCount);
        }
		if( g_ulProtocolID == J1939 || g_ulProtocolID == J1939_4PGN )	pDiagmsg->subEvent = eCAN_RX_BLOCK_EX;
		else														  	pDiagmsg->subEvent = eCAN_RX_BLOCK;
		if(osMessageAvailableSpace( hOBDRxMessage ) == 0)
		{
			osPoolFree( hPTPKPool, (void *)pPTpacket );
			osPoolFree( hDiagPool, (void *)pDiagmsg );
		}
		else
		{
			osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg, osWaitForever );
		}
	}
	else	//kline
	{
#if 1
		pDiagmsg->subEvent = eKLINE_RX_BLOCK;
		if(osMessageAvailableSpace( hOBDKlineRxMessage ) == 0)
		{
			osPoolFree( hPTPKPool, (void *)pPTpacket );
			osPoolFree( hDiagPool, (void *)pDiagmsg );
		}
		else
		{
		  	osMessagePut( hOBDKlineRxMessage, (uint32_t)pDiagmsg, osWaitForever );
		}
#endif
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif
}
#endif

void FL_Git_WriteSerial(stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t		*pPayloadPtcl = pkt->mData;
	uint8_t		serial[SERIAL_NUMBER_SIZE] = { 0, },i;

	//if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK) return;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 0;

	memcpy( serial, &pPayloadPtcl[2], SERIAL_NUMBER_SIZE );
#if( PRINT_FUCTIONLIST_DEBUG_MESSAGE )
	GLogI( "write serial : ");
	for( i = 0; i < SERIAL_NUMBER_SIZE; i++ )
	{
		GLogI( "%c", serial[i] );
	}
	GLogI( "\r\n" );
#endif	// PRINT_FUCTIONLIST_DEBUG_MESSAGE

	memcpy( (void*)gsFwInfo.marrucSerialNo, (const void*)serial, SERIAL_NUMBER_SIZE );
	gsFwInfo.mucChanged = TRUE;
	saveFirmwareInfo_EMMC(true);

	loadFirmwareInfo_EMMC();

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1302;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
void FL_GitSetVCIInit(stCommPkt *pkt, uint32_t eInCommType  )//GitInitHWVariable
{
	stCommPkt	*packet;
	stMsgClst	*message;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 0;

	VCI_Initialize();

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1308;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitGetBatteryVoltage(stCommPkt *pkt, uint32_t eInCommType  )//GitInitHWVariable
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint32_t	adcValue;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	adcValue = readBatteryValue() / 10;
	GLogI( "adcValue(%d)\r\n", adcValue );
	memcpy( &packet->mData[0], &adcValue, sizeof(adcValue) );

	packet->mLen	= 4;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1320;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
								  
void FL_Git_DeviceReset(stCommPkt *pkt, uint32_t eInCommType  )//GitInitHWVariable
{
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif
	//no response to PC
	osDelay(100);
	
	jumpToBootloaderReset(3000);
	while(1)
	{
		GLogE( "DeviceReset Fail!!!\r\n");
	}
}

#ifdef VCI3_DIAG
void FL_GitSetCanidFilter(stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	U32 uiProtocolID;
	U32 CAN_StartID, CAN_EndID;
    uint8_t  CAN_TYPE;
	U32 uCan_Start[4], uCan_End[4];
    U32 CAN_StartID_SW = 0, CAN_EndID_SW = 0;
    uint8_t i;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK) return;

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	uiProtocolID = VCI_GetPassThruProtocolID();

    if( (uiProtocolID == ISO15765_CARB_29BIT) 	 ||
	    (uiProtocolID == ISO15765_CARB_29BIT_NEW)||
        (uiProtocolID == J1939_23_CARB_29BIT_NEW)||
        (uiProtocolID == ISO15765_29BIT) 		 ||
        (uiProtocolID == ISO15765_29BIT_EXCEPT)  ||
        (uiProtocolID == ISO15765_29BIT_REPRO_PENNIMG_TIME))
    {
        CAN_TYPE = 0x02;
    }
    else
    {
        CAN_TYPE = 0x01;
    }

	memcpy(&CAN_StartID, pPayloadPtcl, sizeof(CAN_StartID));
	memcpy(&CAN_EndID, pPayloadPtcl+4, sizeof(CAN_EndID));

	if(CAN_StartID>CAN_EndID)
	{
		memcpy(&CAN_EndID, pPayloadPtcl, sizeof(CAN_EndID));
		memcpy(&CAN_StartID, pPayloadPtcl+4, sizeof(CAN_StartID));
	}
	//else
	//{
	//	memcpy(&CAN_StartID, pPayloadPtcl, sizeof(CAN_StartID));
	//	memcpy(&CAN_EndID, pPayloadPtcl+4, sizeof(CAN_EndID));
	//}

	for(i=0; i<4; i++)
    {
        uCan_Start[i] = (CAN_StartID>>(8*i))&0xff;
        CAN_StartID_SW += uCan_Start[i]<<(24-(8*i));

        uCan_End[i] = (CAN_EndID>>(8*i))&0xff;
        CAN_EndID_SW += uCan_End[i]<<(24-(8*i));
    }

	Oem_CAN_Channel_Masket_Set(g_ucCAN_CH, CAN_TYPE, 1, &CAN_StartID_SW, &CAN_EndID_SW);

	packet->mLen	= 0;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x4000;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
#if 1//defined(DEBUG_GIT_PTCL_LOG)
	GLogI("[%s] run CAN_StartID_SW %X CAN_EndID_SW %X\r\n", __FUNCTION__, CAN_StartID_SW, CAN_EndID_SW);
#endif
}
#endif

#ifdef VCI3_DIAG
void FL_GitSendPingAck( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
#if VCI1_TEST
	LOCK_SET_STATE(eLOCK_STATE_UNLOCK);
#endif
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
		return;
	}

	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 10;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0xe010;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}

}
#endif

#ifdef VCI3_DIAG
void FL_GitSetBatteryMonitoring( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;
	uint8_t *pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	
	unsigned char ucMode=0;
	unsigned int uiRxCanID1 = 0;//,uiRxCanID2 = 0;
    unsigned long Timeout;
	static unsigned int s_uiPreProtocol;
	static unsigned int s_uiMASK_CANID;
	static unsigned int s_uiVP3_MIN;
    char i;
	//uint8_t Counter;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

	GLogI("@");
	g_bIsFastInit=FALSE;//variable init
	if(VCI_ProtocolClassification()==eCAN_TX_NONE_PARSING)	//CAN
	{
		clearRXCanMessage();	//CAN

		if(g_bIsAckWorking==TRUE)//kkt need develop, only kline processing
		{
			GLogI("for ack processing osDelay 70ms");
			osDelay(70);
		}
	}
	else	//kline
	{
		DLC_RX_BUFF_CLEAR();

		if(g_bIsAckWorking==TRUE)//kkt need develop, only kline processing
		{
			GLogI("for ack processing osDelay 200ms");
			osDelay(200);
		}
	}

	g_bIsAckWorking=FALSE;

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	if( pPayloadPtcl != NULL)
	{
		memcpy(&Timeout, pPayloadPtcl, sizeof(Timeout));	//VCI�� ���� ������ �ο� ����.

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
//(VCI2 LOG)WLan Data In 56 : 02 35 00 80 00 B9 30 00 14 23 00 00 22 04 E8 03 00 00 00 01 00 00 00 00 00 00 00 00 01 00 B8 0B 00 00 07 00 00 00 06 00 00 00 07 70 04 31 01 05 A5 00 00 00 02 A5 03 B9

		memcpy(pPTpacket, pPayloadPtcl+4, sizeof(PTmsgPkt_t));

		ucMode = pPayloadPtcl[usLength-5];
		for(i=0;i<4;i++)
        {
            uiRxCanID1 += ((pPayloadPtcl[usLength-4+i])<<((3-i)*8));//24 16 8
            //uiRxCanID2 += ((pPayloadPtcl[usLength-4+i])<<((3-i)*8));
        }
		clearRXCanMessage();	//CAN
		//StopSWTimer(stPeriodicMsgInfo.ucTimerIndex);
		g_bAckflag=0;
		if(ucMode == 0||ucMode == 2)//Start
        {
        	if(ucMode == 0) g_ucVehicle_Current_Read = 1;
			else if(ucMode == 2) g_ucVehicle_Current_Read = 2;

			s_uiVP3_MIN=g_stGITSetConfig.nP3Min;
			g_stGITSetConfig.nP3Min=Timeout;

			//J2534AckModeStatus = 0;
			s_uiMASK_CANID=g_stGITSetConfig.nEtc5;
			GLogI( "\r\nP3_MIN : %d",g_stGITSetConfig.nP3Min);
			//GLogI( "\r\nJ2534AckModeStatus : %d",J2534AckModeStatus);
			GLogI( "\r\n   MASK_CANID : %04X",g_stGITSetConfig.nEtc5);
			g_stGITSetConfig.nEtc5=uiRxCanID1;
            s_uiPreProtocol = g_ulProtocolID;
            g_ulProtocolID = ISO15765_ACU_SINGLE;
            GLogI( "\r\n   Start 0x2314!! %04X / %04X",uiRxCanID1,s_uiPreProtocol);
			Oem_CAN_Channel_Masket_Set(g_ucCAN_CH, 0x01, 1, &uiRxCanID1, &uiRxCanID1);
        }
        else // Stop
        {
        	g_ucVehicle_Current_Read = 0;
            GLogI( "\r\n   Stop 0x2314!! %04X",s_uiPreProtocol);
			g_stGITSetConfig.nP3Min=s_uiVP3_MIN;
			g_stGITSetConfig.nEtc5=s_uiMASK_CANID;
			GLogI( "\r\n   MASK_CANID : %04X",g_stGITSetConfig.nEtc5);
            VCI_HW_Setting(s_uiPreProtocol);
            RxCANID_Set(pPayloadPtcl[28],pPayloadPtcl[29]);
        }

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
	}


	packet->mLen	= 0;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x2314;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}

}
#endif

#ifdef VCI3_DIAG
void FL_GitGetBatteryDataFrame( stCommPkt *pkt, uint32_t eInCommType  )
{
	MsgDiag_t	*pDiagmsg;
	PTmsgPkt_t	*pPTpacket;

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
	pDiagmsg->mMsgType		= MSG_DIAG;
	pDiagmsg->mPktType		= (ePKT_TD)eInCommType;
	pDiagmsg->event 		= DIAG_PASSTHRU;
	pDiagmsg->unEventTime	= GetUnixTime();
	pDiagmsg->pPacket		= (void *)pPTpacket;

	//if(VCI_ProtocolClassification()==eCAN_TX_NONE_PARSING)	//CAN
	{

		pDiagmsg->subEvent = eCAN_RX_BLOCK;
		if(osMessageAvailableSpace( hOBDRxMessage ) == 0)
		{
			osPoolFree( hPTPKPool, (void *)pPTpacket );
			osPoolFree( hDiagPool, (void *)pDiagmsg );
		}
		else
		{
			osMessagePut( hOBDRxMessage, (uint32_t)pDiagmsg, osWaitForever );
		}
	}
	/*
	else	//kline
	{
		pDiagmsg->subEvent = eKLINE_RX_BLOCK;
		if(osMessageAvailableSpace( hOBDKlineRxMessage ) == 0)
		{
			osPoolFree( hPTPKPool, (void *)pPTpacket );
			osPoolFree( hDiagPool, (void *)pDiagmsg );
		}
		else
		{
			osMessagePut( hOBDKlineRxMessage, (uint32_t)pDiagmsg, osWaitForever );
		}
	}
	*/
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif
}
#endif

#ifdef CANFD_qhyek //Q_hyek CANFD
#ifdef VCI3_DIAG
void FL_GitCanFdEnable( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	uint8_t *pPayloadPtcl = pkt->mData;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;

	if(pPayloadPtcl[0]==0)	//CAN FD �� �۽��ϵ��� ����
		g_ucCanformat = CAN_FRAMEFORMAT_FDCAN;
	else					//CAN ���� �۽��ϵ��� ����
		g_ucCanformat = CAN_FRAMEFORMAT_CLASSIC;
	VCI_HW_Setting(g_ulProtocolID);

	packet->mData[0] = 0x00;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0xE021;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif
#endif

#if 0
#define FL_EMMC_FIRST_PACKET        0X10
#define FL_EMMC_MIDDLE_PACKET       0X20
#define FL_EMMC_LAST_PACKET         0X40
#define MAX_EMMC_DIRINFO_SEND_BUFF  2000
extern void put_rc( FRESULT rc );

#ifdef VCI3_DIAG
void FL_EMMC_Directory( stCommPkt *pkt, uint32_t eInCommType  )
{
    DIR         stDir;
    FRESULT     stFileRes;
    FILINFO     stFinfo;
    FIL         stFileHanlde;
	uint8_t *pPayloadPtcl = pkt->mData;
    uint8_t          arrFilePath[25]= {0, };//{'0','3' ,'_','R','E','C','O','R','D'};
    uint8_t          arrFileInfoToPC[MAX_EMMC_DIRINFO_SEND_BUFF] = {0, }, arrReadBuff[512];
    U32         nFolderCnt = 0, nFileChecksum = 0, nReadLen = 0,
                nFileCnt = 0, nPacketCnt = 0, nArrIndex = 0, nFileFolderSum;
    U16         usSecquence = 0;
    U32         nSavePrintIdx = 7;
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif
    memcpy(arrFilePath, &pPayloadPtcl[1], pPayloadPtcl[0]);
    
    stFileRes = f_opendir(&stDir, (char*)arrFilePath);
    if (stFileRes != FR_OK)
	{
        arrFileInfoToPC[nArrIndex++] = 0x01|FL_EMMC_FIRST_PACKET|FL_EMMC_LAST_PACKET;
        arrFileInfoToPC[nArrIndex++] = stFileRes;
        put_rc((FRESULT)stFileRes);
        TransmitFunction((ePKT_TD)eInCommType, arrFileInfoToPC, nArrIndex, 0x120E);
	}
    else
	{
        while(1)
        {
          stFileRes = f_readdir(&stDir, &stFinfo);
          if(stFileRes != FR_OK || stFinfo.fname[0] == 0) break;
        
          if (stFinfo.fattrib & AM_DIR) nFolderCnt++;   // Total Foler count
          else                          nFileCnt++;     // Total File count

          osThreadYield();
        }
        f_closedir(&stDir);

        stFileRes = f_opendir(&stDir, (char*)arrFilePath);
        nArrIndex = 0, usSecquence = 0, nPacketCnt = 0;

        nFileFolderSum = nFolderCnt + nFileCnt;
        arrFileInfoToPC[nArrIndex++] = FL_EMMC_FIRST_PACKET;  // success �ǹ� ����

        memcpy(&arrFileInfoToPC[nArrIndex], &nFileFolderSum, sizeof(U16)); nArrIndex += sizeof(U16);
        memcpy(&arrFileInfoToPC[nArrIndex], &nFileCnt, sizeof(U16)); nArrIndex += sizeof(U16);
        memcpy(&arrFileInfoToPC[nArrIndex], &nFolderCnt, sizeof(U16)); nArrIndex += sizeof(U16);
        /*
        arrFileInfoToPC[nArrIndex++] = (nFileFolderSum & 0xFF00)>> 8;
        arrFileInfoToPC[nArrIndex++] = (nFileFolderSum & 0x00FF);
        arrFileInfoToPC[nArrIndex++] = (nFileCnt & 0xFF00)>> 8;
        arrFileInfoToPC[nArrIndex++] = (nFileCnt & 0x00FF);
        arrFileInfoToPC[nArrIndex++] = (nFolderCnt & 0xFF00)>> 8;
        arrFileInfoToPC[nArrIndex++] = (nFolderCnt & 0x00FF);
        */
        
        while(1)
        {
            stFileRes = f_readdir(&stDir, &stFinfo);

            if      ( nPacketCnt == 0 ) 
            {
                arrFileInfoToPC[0] = FL_EMMC_FIRST_PACKET;  // success �ǹ� ����
            }
            else if ( nPacketCnt > 0  ) 
            {
                arrFileInfoToPC[0] = FL_EMMC_MIDDLE_PACKET; // success �ǹ� ����
            }

            if ( (stFileRes != FR_OK) || !stFinfo.fname[0] )  
            {
                if ( nPacketCnt == 0 )  arrFileInfoToPC[0] |= FL_EMMC_LAST_PACKET;
                else                    arrFileInfoToPC[0] = FL_EMMC_LAST_PACKET;
                GLogI("===> Send packet END : Paket Number %d\r\n", nPacketCnt );
                TransmitFunction((ePKT_TD)eInCommType, arrFileInfoToPC, nArrIndex, 0x120E);
                break;
            }

            ++usSecquence;
            memcpy(&arrFileInfoToPC[nArrIndex], &usSecquence, sizeof(U16)); nArrIndex += sizeof(U16);
            /*
            arrFileInfoToPC[nArrIndex++] = (usSecquence & 0xFF00)>> 8;
            arrFileInfoToPC[nArrIndex++] = (usSecquence & 0x00FF);
            */
            if (stFinfo.fattrib & AM_DIR)   arrFileInfoToPC[nArrIndex++] = 0x01;    // folder
            else                            arrFileInfoToPC[nArrIndex++] = 0x00;    // file
            
            arrFileInfoToPC[nArrIndex++] = strlen(stFinfo.fname); 
            memcpy(&arrFileInfoToPC[nArrIndex], stFinfo.fname, strlen(stFinfo.fname)); 
            nArrIndex += strlen(stFinfo.fname);
            arrFileInfoToPC[nArrIndex++] = (uint8_t)((stFinfo.fsize & 0xFF000000) >> 24); 
            arrFileInfoToPC[nArrIndex++] = (uint8_t)((stFinfo.fsize & 0x00FF0000) >> 16);
            arrFileInfoToPC[nArrIndex++] = (uint8_t)((stFinfo.fsize & 0x0000FF00) >> 8);
            arrFileInfoToPC[nArrIndex++] = (uint8_t)(stFinfo.fsize & 0x000000FF);

            // get file checksum
            if( f_open(&stFileHanlde, stFinfo.fname, FA_OPEN_EXISTING | FA_READ) == FR_OK )
            {
                nFileChecksum = 0;
                
                
                for ( int i=0; i<stFinfo.fsize; )
                {
                    if( f_read(&stFileHanlde, arrReadBuff, sizeof(arrReadBuff), (UINT*)&nReadLen) == FR_OK )
                    {
                        for(int j=0; j<nReadLen; j++)
                            nFileChecksum += arrReadBuff[j];
                        i += nReadLen;
                    }
                    nReadLen = 0;
                }
            }
            f_close(&stFileHanlde);
            
            arrFileInfoToPC[nArrIndex++] = nFileChecksum; 
/*
            GLogN( "0:0x%02X totalcnt:%d, file:%d folder:%d, seq:%d, fileSize:%d, cs %d, fileName:%s\r\n", 
                        arrFileInfoToPC[0], nFileFolderSum, nFileCnt, nFolderCnt, 
                        usSecquence, stFinfo.fsize, nFileChecksum, stFinfo.fname);
           

	{
                 GLogI("{{{");
                for ( int j=0; j<7; j++ )   GLogN("%02X ", arrFileInfoToPC[j]);
                GLogN("}}} ==> ");                
                    
                for ( int i=nSavePrintIdx; i<nArrIndex; i++ )  GLogN("%02X ", arrFileInfoToPC[i]);
                GLogN("\r\n");
                nSavePrintIdx = nArrIndex;
            }   
*/
            if( nArrIndex + 2 + 2 + 256 + 4 + 1 >  MAX_EMMC_DIRINFO_SEND_BUFF )
            {
                GLogN("===> Send packet %s : Paket Number %d\r\n", (arrFileInfoToPC[0]== FL_EMMC_FIRST_PACKET) ? "Start":"MIddle", nPacketCnt);

                nPacketCnt++;
                TransmitFunction((ePKT_TD)eInCommType, arrFileInfoToPC, nArrIndex, 0x120E);
                nArrIndex = 4, nSavePrintIdx = 4;
                


            }
            osThreadYield();
        }
	}
}
#endif
#endif

void FL_EMMC_Directory( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
    uint8_t FilePath[25]= {0, };//{'0','3' ,'_','R','E','C','O','R','D'};
    uint8_t FileInfoToPC[4000] = {0, };
    U32 SizeInfoToPC = 0;
    
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    memcpy(FilePath, &pPayloadPtcl[1], pPayloadPtcl[0]);
    directory_list2(&FilePath[0], FileInfoToPC, (int*)&SizeInfoToPC);
    memcpy(&packet->mData[0], &FileInfoToPC[0], SizeInfoToPC);

	packet->mLen	= SizeInfoToPC;

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x120E;
    packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_Get_Cert_ExpirationPeriod( stCommPkt *pkt, uint32_t eInCommType  )
{
    stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
    uint8_t ucCertNumber = 0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    ucCertNumber = pPayloadPtcl[0];
    
    if( packet->mLen = CRT_CRL_Check( &packet->mData[1], ucCertNumber) ) packet->mData[0] = 0; //success
    else packet->mData[0] = 1; //fail
    
    packet->mLen += 1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x120F;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_Get_EMMC_File_Info( stCommPkt *pkt, uint32_t eInCommType  )
{
    stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
    uint8_t ucSID = 0;
    U16 usPathLen = 0;
    uint8_t ucFileLen = 0;
    FRESULT frResult;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s (SID : 0x%02X)\r\n", __FUNCTION__, pPayloadPtcl[0]);
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    ucSID = pPayloadPtcl[0];
    packet->mData[1] = ucSID;
    
    switch( ucSID )
    {
        case READ_NUMBER_OF_FILES:
        {
            U16 uiFileCount = 0;
            U16 uiFolderCount = 0;
            U16 uiPacketCount = 0;
            
            usPathLen = (pPayloadPtcl[1]<<8) + pPayloadPtcl[2];
            
            frResult = FileCount(&pPayloadPtcl[3], usPathLen, &uiFileCount, &uiFolderCount, &uiPacketCount);
            
            if(frResult == FR_OK)
            {
                packet->mData[0] = 0x00; //success
                little_to_big_endian(&packet->mData[2], uiFileCount, sizeof(U16));
                little_to_big_endian(&packet->mData[4], uiFolderCount, sizeof(U16));
                little_to_big_endian(&packet->mData[6], uiPacketCount, sizeof(U16));
                packet->mLen = 8;
            }
            else
            {
                packet->mData[0] = 0x01; //fail
                packet->mData[2] = frResult;
                packet->mLen = 3;
            }
            
            break;
        }
        
        case READ_PATH_INFO:
        {
            U16 usSeq = 0;
            U32 uiOutLen = 0;
            
            usSeq = (pPayloadPtcl[1]<<8) + pPayloadPtcl[2];
            usPathLen = (pPayloadPtcl[3]<<8) + pPayloadPtcl[4];
              
            frResult = FileParsing(&pPayloadPtcl[5], usPathLen, usSeq, &packet->mData[2], &uiOutLen);
            
            if(frResult == FR_OK)
            {
                packet->mData[0] = 0x00; //success
                packet->mLen = uiOutLen + 2;
            }
            else
            {
                packet->mData[0] = 0x01; //fail
                packet->mData[2] = frResult;
                packet->mLen = 3;
            }
              
            break;
        }
        
        case CHECKSUM_CALCULATION:
        {
            U32 uiChecksum = 0;
            U8 ucCSmode = 0;
            
            usPathLen = (pPayloadPtcl[2]<<8) + pPayloadPtcl[3];
            ucFileLen = pPayloadPtcl[4];
            ucCSmode = pPayloadPtcl[1];
              
            if(ucCSmode == 1) //Checksum
            {
                frResult = FileChecksumCheck(&pPayloadPtcl[5], usPathLen, &pPayloadPtcl[5+usPathLen], ucFileLen, &uiChecksum);
            }
            else if(ucCSmode == 2) //CRC32
            {
                frResult = FileCRC32Check(&pPayloadPtcl[5], usPathLen, &pPayloadPtcl[5+usPathLen], ucFileLen, &uiChecksum);
            }
            else
            {
                frResult = FR_INVALID_PARAMETER;
            }
            
            if(frResult == FR_OK)
            {
                packet->mData[0] = 0x00; //success
                packet->mData[2] = (uiChecksum&0xFF000000)>>24;
                packet->mData[3] = (uiChecksum&0x00FF0000)>>16;
                packet->mData[4] = (uiChecksum&0x0000FF00)>>8;
                packet->mData[5] = uiChecksum&0x000000FF;
                packet->mLen = 6;
            }
            else
            {
                packet->mData[0] = 0x01; //fail
                packet->mData[2] = frResult;
                packet->mLen = 3;
            }
            
            break;
        }
        
        default :
        {
            GLogN("\r\n-------> Unregistered SID !!! (%d)", ucSID);
            break;
        }
      
    }

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x130E;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void Git_HSMinitial( stCommPkt *pkt, uint32_t eInCommType  )
{
	U32          dummy1;
	U32          dummy2; 
	U32          dummy3;

	stCommPkt	*packet;//CommPkt_t	*packet;
	stMsgClst	*message;//MsgClst_t	*message;
	uint8_t		*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	U16 		usHSMinitMode = 0;
	U32 		uiReturnCode = 200;
	U32 		uiTempDataLen = 0;
	U32 		uiOutDataLen = 0;
	uint8_t		arrTempAES[16];
	uint8_t		arrRandData[16];
	uint8_t		arrTempData[1000];
	uint8_t		arrOutData[1000];
	uint8_t		ucCRT_Number=0;
	uint8_t		ucPriKey_Number=0;
    uint8_t		ucChecksum=0;

    dummy1 = (U32)&packet;
    dummy2 = (U32)&message;
    dummy3 = (U32)&pPayloadPtcl;

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->pTarget = &eInCommType;
	packet->mFuncID = 0x1306;

	memcpy(&usHSMinitMode,  &pPayloadPtcl[0], sizeof(usHSMinitMode));

	#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s(%04x)\r\n", __FUNCTION__, usHSMinitMode );
	#endif
	GLogI( "(%04x) ", usHSMinitMode );
	switch(usHSMinitMode)
	{
		case HSM_READ_CSN://0x0101
		{	
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_READ_CSN\r\n");
#endif
			packet->mLen = 14;

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				uiReturnCode = ActivationHSM();

				if(uiReturnCode == HSM_SUCCESS)
				{
					uiReturnCode = ReadCSNHSM(&packet->mData[6]);
				}
			}
			else
			{
				if (hsm_get_serial_number(&packet->mData[6]) == HAL_OK)
				{
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_NOT_RESPONSE;
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			break;
		}

		case HSM_INTERNAL_AUTH://0x0102
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_INTERNAL_AUTH\r\n");
#endif
			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				packet->mLen = 34;  /* OLD: 2+4+4+8(CR)+16(Sign1) = 34 */

				memcpy( &uiTempDataLen, &pPayloadPtcl[10], sizeof(uiTempDataLen) );

				uiReturnCode = InternalAuthHSM(uiTempDataLen, &pPayloadPtcl[2], &packet->mData[10], &packet->mData[18]);

				//veryfiSign1( &pPayloadPtcl[2], &packet->mData[10], &packet->mData[18]); //Verify signature
			}
			else
			{
				/* NEW HSM: SR=16B, CR=16B, Sign1=32B */
				packet->mLen = 58;  /* NEW: 2+4+4+16(CR)+32(Sign1) = 58 */

				/* Get KeyId from input to determine use_kauth */
				memcpy(&uiTempDataLen, &pPayloadPtcl[18], sizeof(uiTempDataLen));
				bool use_kauth = (uiTempDataLen == 1U) ? true : false;  /* KeyId 1 = Kauth, 0 = Km */
				if(use_kauth==true)
				{
				  	GLogI( "(use_kauth)\r\n");
				}
				else
				{
					GLogI( "(use_KM)\r\n");
				}

				/* Prepare 16-byte SR (pad 8-byte input with zeros if needed) */
				uint8_t sr_padded[16] = {0};
				memcpy(sr_padded, &pPayloadPtcl[2], 16);  /* Copy 16-byte SR from App */
#if defined(NEW_HSM_LOG_ENABLE)
				GLogN("HSM_INTERNAL_AUTH SR(16B): ");
				for (int i = 0; i < 16; i++) GLogN("%02X ", pPayloadPtcl[i+2]);
				GLogN("\r\n");
#endif
				/* Output buffers */
				uint8_t cr_out[16];
				uint8_t sign1_out[32];
				uint8_t key_id_out;
				
				GLogN("hsm_internal_auth start!!!\r\n");

				HAL_StatusTypeDef hal_ret = hsm_internal_auth(use_kauth, sr_padded, cr_out, sign1_out, &key_id_out);

				if (hal_ret == HAL_OK)
				{
					uiReturnCode = HSM_SUCCESS;
					/* Copy CR (16B) to mData[10..25] */
					memcpy(&packet->mData[10], cr_out, 16);
					/* Copy Sign1 (32B) to mData[26..57] */
					memcpy(&packet->mData[26], sign1_out, 32);
#if defined(NEW_HSM_LOG_ENABLE)
					/* Debug log */
					GLogN("HSM_INTERNAL_AUTH CR(16B): ");
					for (int i = 0; i < 16; i++) GLogN("%02X ", cr_out[i]);
					GLogN("\r\n");
					GLogN("HSM_INTERNAL_AUTH Sign1(32B): ");
					for (int i = 0; i < 32; i++) GLogN("%02X ", sign1_out[i]);
					GLogN("\r\n");
#endif
				}
				else
				{
					uiReturnCode = HSM_NO_AUTHKEY_INDEX;
				}
				if(g_bUsingKM == ON && use_kauth == true)
				{
					g_bUsingKM = OFF;
					GLogE("OLD Kauth!! need KM!!\r\n");
					uiReturnCode = HSM_NO_AUTHKEY_INDEX;//use KM when reconnect
				}
			}

			//uiTempDataLen = INTERNAL_MASTER_KEY;
			//uiTempDataLen = INTERNAL_AUTH_KEY;
			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			memcpy( &packet->mData[6],&uiTempDataLen, sizeof(uiTempDataLen) );

			break;
		}

		case HSM_EXTERNAL_AUTH://0x0103
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_EXTERNAL_AUTH\r\n");
#endif
			packet->mLen = 6;  /* Same for both OLD and NEW */

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				uiReturnCode = ExternalAuthHSM(0, &pPayloadPtcl[6]);		// External auth with authkey (index 0)
			}
			else
			{
				/* NEW HSM: Sign2=32B (pad 16-byte input from App) */
				bool use_kauth = true;  /* Default: use Kauth key (#103) */

				/* Prepare 32-byte Sign2 */
				uint8_t sign2_padded[32] = {0};
				memcpy(sign2_padded, &pPayloadPtcl[6], 32);  /* Copy 32-byte Sign2 from App */

				bool auth_success = false;

				HAL_StatusTypeDef hal_ret = hsm_external_auth(use_kauth, sign2_padded, &auth_success);

				if (hal_ret == HAL_OK && auth_success)
				{
					uiReturnCode = HSM_SUCCESS;
				}
				else if (hal_ret == HAL_OK && !auth_success)
				{
					uiReturnCode = HSM_FAIL_EXTERNAL_AUTH;
				}
				else
				{
					uiReturnCode = HSM_NOT_RESPONSE;
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );

			break;
		}

		case HSM_PUBLICKEY_HSM://0x0111
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_PUBLICKEY_HSM\r\n");
#endif
#if 1  		
			// RSA-2048 PUBLICKEY (259 bytes: 256 modulus + 3 exponent)
			// Modulus: bab0071731e05b2e...72f8ecb1
			// Exponent: 010001 (65537)
			static const U8 testPublicKey[259] = {
				// Modulus (256 bytes)
				0xba, 0xb0, 0x07, 0x17, 0x31, 0xe0, 0x5b, 0x2e, 0x09, 0x55, 0x36, 0x3c, 0x29, 0x25, 0x49, 0x2b,
				0x50, 0xf3, 0x31, 0x15, 0x18, 0x20, 0x05, 0x3f, 0xb3, 0xe5, 0x14, 0x93, 0x57, 0x6a, 0x56, 0x0c,
				0x65, 0xcb, 0x10, 0x3a, 0xb3, 0x72, 0xad, 0xc7, 0xc9, 0x3f, 0xe2, 0x41, 0xba, 0xc2, 0x50, 0x5e,
				0xec, 0x7b, 0x10, 0xf0, 0xbc, 0xe7, 0x81, 0x94, 0x13, 0x52, 0x7d, 0x72, 0x04, 0x38, 0xb7, 0x0f,
				0xf0, 0xb9, 0xd8, 0xe3, 0x8d, 0xf1, 0x97, 0xd5, 0xc8, 0x99, 0x87, 0xad, 0xdd, 0xaf, 0x25, 0x72,
				0x49, 0x35, 0x9a, 0xb6, 0xe6, 0x23, 0x1e, 0x99, 0xf0, 0xf6, 0x04, 0xad, 0x8a, 0xa6, 0x63, 0x5e,
				0xac, 0x0f, 0x3d, 0xd7, 0xee, 0xc2, 0xc2, 0xf8, 0x22, 0x67, 0x8c, 0xc8, 0xbd, 0xc7, 0x61, 0x10,
				0x7c, 0x6e, 0x44, 0xf7, 0x04, 0xb9, 0xdf, 0xd6, 0xc8, 0xcb, 0x66, 0xfc, 0xca, 0xbf, 0x68, 0x8f,
				0x3c, 0x3e, 0xba, 0x36, 0x11, 0xe5, 0x6a, 0x17, 0xe2, 0x95, 0xb8, 0xa9, 0xeb, 0xd5, 0x8f, 0xad,
				0x5e, 0xd6, 0x08, 0x71, 0x3f, 0x7e, 0x49, 0x93, 0x5d, 0xb0, 0xb9, 0xaf, 0x4e, 0xf0, 0x51, 0x6a,
				0x3e, 0xf8, 0xc7, 0x64, 0x98, 0x85, 0x68, 0x26, 0xfc, 0x5f, 0x14, 0x38, 0x51, 0xe8, 0x44, 0xb4,
				0x65, 0x37, 0xca, 0xc2, 0x8e, 0x1f, 0x92, 0x9e, 0xaf, 0xba, 0xe6, 0xaa, 0x8f, 0x0f, 0xf1, 0xeb,
				0x99, 0xb3, 0x56, 0xb7, 0x4d, 0x03, 0xba, 0xbb, 0xec, 0xa3, 0xd2, 0x5e, 0x5d, 0x82, 0xb3, 0xdd,
				0x03, 0x7f, 0xa7, 0x9e, 0xf1, 0x3e, 0xf1, 0x25, 0x70, 0xcd, 0x00, 0x22, 0xcb, 0x00, 0x46, 0x0c,
				0xb2, 0x96, 0xee, 0x21, 0x1a, 0xf5, 0xea, 0xf2, 0x03, 0xcb, 0x1b, 0x59, 0x0c, 0x87, 0x35, 0x2a,
				0xb6, 0x14, 0x44, 0x74, 0xfc, 0xdd, 0x3c, 0x82, 0xd4, 0x6b, 0x72, 0x4d, 0x72, 0xf8, 0xec, 0xb1,
				// Public Exponent (3 bytes) = 65537
				0x01, 0x00, 0x01
			};
			uiReturnCode = HSM_SUCCESS;
			uiTempDataLen = 259;
			memcpy(arrTempData, testPublicKey, 259);
#else
	#if defined(Insert_Key)    //HMAC, AES128 Key Store  Test
		  if( (ReadPublicKeyHSM(PubKey, (int*)&PubKeyLen, HSM_PUBKEY)) == HSM_SUCCESS)
		  {
			if( ((setRSA_Encrypt(&HMAC_Key[0], 32, PubKey, PubKeyLen, Encrypted_HMAC_Key) == CMOX_RSA_SUCCESS)) && (setRSA_Encrypt(&DerivedKey[0], 16, PubKey, PubKeyLen , Encrypted_DerivedKey) == CMOX_RSA_SUCCESS));
			{
			  uiReturnCode = StoreHMACSHA256KeyHSM(&Encrypted_HMAC_Key[0], 128, HMAC_INDEX_1);
			  uiReturnCode = StoreAES128HSM(&Encrypted_DerivedKey[0], 128, AES_INDEX_1, ucChecksum);
			}
		  }
	#endif

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				uiReturnCode = ReadPublicKeyHSM(arrTempData, (int*)&uiTempDataLen, HSM_PUBKEY);
			}
			else
			{
				// Step 1: Generate RSA key pair
				GLogN("[HSM_PUBLICKEY_HSM] Generate RSA Key Pair (Key ID: %d)\r\n", HSM_KEK_RSA_KEY_ID);
				HAL_StatusTypeDef gen_status = hsm_generate_key_pair(HSM_KEK_RSA_KEY_ID, HSM_ALG_RSA, 0, false);

				if (gen_status != HAL_OK)
				{
					GLogE("[HSM_PUBLICKEY_HSM] Generate Key Pair failed, status: %d\r\n", gen_status);
					uiTempDataLen = 0;
					uiReturnCode = HSM_UNKNOWN_ERROR;
				}
				else
				{
					GLogN("[HSM_PUBLICKEY_HSM] Generate Key Pair Success, now exporting...\r\n");

					// Step 2: Export public key
					U8 tempBuffer[300];
					U16 key_length = 0;
					if (hsm_export_public_key(HSM_KEK_RSA_KEY_ID, HSM_ALG_RSA, tempBuffer, &key_length) == HAL_OK)
					{
						U16 mod_len = (U16)tempBuffer[0] | ((U16)tempBuffer[1] << 8);
						U16 exp_offset = 2 + mod_len;
						U16 exp_len = (U16)tempBuffer[exp_offset] | ((U16)tempBuffer[exp_offset + 1] << 8);

						memcpy(arrTempData, &tempBuffer[2], mod_len);
						memcpy(&arrTempData[mod_len], &tempBuffer[exp_offset + 2], exp_len);

						uiTempDataLen = mod_len + exp_len;  // 260 bytes (256 mod + 4 exp)
						uiReturnCode = HSM_SUCCESS;
						GLogN("[HSM_PUBLICKEY_HSM] Export Success, length: %d\r\n", uiTempDataLen);
					}
					else
					{
						GLogE("[HSM_PUBLICKEY_HSM] Export Public Key failed\r\n");
						uiTempDataLen = 0;
						uiReturnCode = HSM_READ_FAIL;
					}
				}
			}
#endif

			packet->mLen	= uiTempDataLen + 10;

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			memcpy( &packet->mData[6],&uiTempDataLen, sizeof(uiTempDataLen));
			memcpy( &packet->mData[10],&arrTempData[0], uiTempDataLen );
			break;
		}

		case HSM_STORE_AUTHKEY://0x0112
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_AUTHKEY\r\n");
#endif
			packet->mLen	= 6;

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				memcpy( &uiTempDataLen, &pPayloadPtcl[2], sizeof(uiTempDataLen) );
				uiReturnCode = StoreAuthKeyHSM(&pPayloadPtcl[6], uiTempDataLen);
			}
			else
			{
				memcpy( &uiTempDataLen, &pPayloadPtcl[2], sizeof(uiTempDataLen) );
				HAL_StatusTypeDef hal_ret = hsm_import_kauth_key(&pPayloadPtcl[6], uiTempDataLen);
				uiReturnCode = (hal_ret == HAL_OK) ? HSM_SUCCESS : HSM_UNKNOWN_ERROR;
			}
			//if(uiReturnCode == HSM_SUCCESS) g_bUsingKM = ON;

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			break;
		}

		case HSM_READ_SDATA://0x0121
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_READ_SDATA\r\n");
#endif
			packet->mLen = 24;
			memset(packet->mData, 0xFF, packet->mLen);

			//KKT ADAS BT MAC address retrieval
			{
				BOOL csn_success = FALSE;

				if (g_HSM_Type == HSM_TYPE_OLD)
				{
					if( HSM_SUCCESS == ReadCSNHSM(&packet->mData[16]))
					{
						csn_success = TRUE;
					}
				}
				else
				{
					if (hsm_get_serial_number(&packet->mData[16]) == HAL_OK)
					{
						csn_success = TRUE;
					}
				}

				if (csn_success)
				{
					memcpy( &packet->mData[8], (void*)gsFwInfo.marrucSerialNo, SERIAL_NUMBER_SIZE );
				}
			}

			memcpy(&packet->mData[0], &usHSMinitMode, sizeof(usHSMinitMode));
			break;
		}

		case HSM_STORE_CERTI://0x0122
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_CERTI\r\n");
#endif
			packet->mLen = 611;

			ucCRT_Number = pPayloadPtcl[2];
#if defined(NEW_HSM_LOG_ENABLE)
			GLogN("\r\nStore Cert : %d", ucCRT_Number);
#endif
			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				if( (uiReturnCode = StoreCertificateHSM(&pPayloadPtcl[3], 600, ucCRT_Number)) == HSM_SUCCESS)
				{
					packet->mData[10] = ucCRT_Number;
					uiReturnCode = ReadCertificateHSM(&packet->mData[11], (int*)&uiOutDataLen, ucCRT_Number);
					memcpy( &packet->mData[6],&uiOutDataLen, sizeof(uiOutDataLen) );
				}
			}
			else
			{
				U16 cert_length = 600;
				U16 read_cert_length = 0;
				U8 cert_buffer[616];		// New HSM returns 616 bytes

				/* Map App cert number (1~7) to NEW HSM RSA cert slot (141~147)
				 * OLD HSM uses internal mapping (0x80 | num)
				 * NEW HSM RSA cert slots: #141~148 per SPI Command Manual v3 */
				U16 mapped_cert_id = 140 + ucCRT_Number;

				// Store certificate
				if (hsm_store_certificate(mapped_cert_id, HSM_ALG_RSA, &pPayloadPtcl[3], cert_length) == HAL_OK)
				{
					packet->mData[10] = ucCRT_Number;

					// Read certificate
					if (hsm_read_certificate(mapped_cert_id, HSM_ALG_RSA, cert_buffer, &read_cert_length) == HAL_OK)
					{
						// New HSM returns 616 bytes, but packet stores only 600 bytes
						memcpy(&packet->mData[11], cert_buffer, 600);
						uiOutDataLen = 600;		// Report 600 bytes to maintain compatibility
						memcpy(&packet->mData[6], &uiOutDataLen, sizeof(uiOutDataLen));
						uiReturnCode = HSM_SUCCESS;
					}
					else
					{
						uiReturnCode = HSM_READ_FAIL;
					}
				}
				else
				{
					uiReturnCode = HSM_WRITE_FAIL;
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			break;
		}

	#ifdef ADL_CVCI_HSM_METHOD
		case HSM_STORE_PRIVATEKEY://0x0124
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_PRIVATEKEY\r\n");
#endif
			packet->mLen	= 6;
			uiReturnCode = HSM_UNKNOWN_ERROR;

			if( (ReadPublicKeyHSM(arrTempData, (int*)&uiTempDataLen, HSM_PUBKEY)) == HSM_SUCCESS)
			{
				if( getRandom(4, (uint32_t*)&arrRandData[0]) ==  HAL_OK)
				{
					if( setRSA_Encrypt(&arrRandData[0], 16, arrTempData, uiTempDataLen , arrOutData) == CMOX_RSA_SUCCESS)
					{
						if( (TransTempKeyHSM(arrOutData, 128)) == HSM_SUCCESS)
						{
							if( getAESEncoding_ECB(&pPayloadPtcl[2], 256, arrRandData, AES128, arrOutData, &uiOutDataLen ) == CMOX_CIPHER_SUCCESS)
							{
								memset(arrTempAES, 0x00, 16);
								arrTempAES[1] = 0x01;
								arrTempAES[3] = 0x01;
								arrTempAES[4] = 0x80;

								if( getAESEncoding_ECB(arrTempAES, 16, arrRandData, AES128, &arrOutData[uiOutDataLen], &uiTempDataLen ) == CMOX_CIPHER_SUCCESS)
								{
									if( getAESEncoding_ECB(&pPayloadPtcl[262], 256, arrRandData, AES128, &arrOutData[uiOutDataLen + uiTempDataLen], &uiOutDataLen ) == CMOX_CIPHER_SUCCESS)
									{
										uiReturnCode = StorePrivateKeyHSM(&arrOutData[0], 528);
									}
								}
							}
						}
					}
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			break;
		}
#else
		case HSM_READ_PUBKEY:
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_READ_PUBKEY\r\n");
#endif
			// test PUBKEY (131 bytes: 128 modulus + 3 exponent)
			static const U8 testPublicKey[259] = {
				// Modulus (256 bytes)
				0xba, 0xb0, 0x07, 0x17, 0x31, 0xe0, 0x5b, 0x2e, 0x09, 0x55, 0x36, 0x3c, 0x29, 0x25, 0x49, 0x2b,
				0x50, 0xf3, 0x31, 0x15, 0x18, 0x20, 0x05, 0x3f, 0xb3, 0xe5, 0x14, 0x93, 0x57, 0x6a, 0x56, 0x0c,
				0x65, 0xcb, 0x10, 0x3a, 0xb3, 0x72, 0xad, 0xc7, 0xc9, 0x3f, 0xe2, 0x41, 0xba, 0xc2, 0x50, 0x5e,
				0xec, 0x7b, 0x10, 0xf0, 0xbc, 0xe7, 0x81, 0x94, 0x13, 0x52, 0x7d, 0x72, 0x04, 0x38, 0xb7, 0x0f,
				0xf0, 0xb9, 0xd8, 0xe3, 0x8d, 0xf1, 0x97, 0xd5, 0xc8, 0x99, 0x87, 0xad, 0xdd, 0xaf, 0x25, 0x72,
				0x49, 0x35, 0x9a, 0xb6, 0xe6, 0x23, 0x1e, 0x99, 0xf0, 0xf6, 0x04, 0xad, 0x8a, 0xa6, 0x63, 0x5e,
				0xac, 0x0f, 0x3d, 0xd7, 0xee, 0xc2, 0xc2, 0xf8, 0x22, 0x67, 0x8c, 0xc8, 0xbd, 0xc7, 0x61, 0x10,
				0x7c, 0x6e, 0x44, 0xf7, 0x04, 0xb9, 0xdf, 0xd6, 0xc8, 0xcb, 0x66, 0xfc, 0xca, 0xbf, 0x68, 0x8f,
				0x3c, 0x3e, 0xba, 0x36, 0x11, 0xe5, 0x6a, 0x17, 0xe2, 0x95, 0xb8, 0xa9, 0xeb, 0xd5, 0x8f, 0xad,
				0x5e, 0xd6, 0x08, 0x71, 0x3f, 0x7e, 0x49, 0x93, 0x5d, 0xb0, 0xb9, 0xaf, 0x4e, 0xf0, 0x51, 0x6a,
				0x3e, 0xf8, 0xc7, 0x64, 0x98, 0x85, 0x68, 0x26, 0xfc, 0x5f, 0x14, 0x38, 0x51, 0xe8, 0x44, 0xb4,
				0x65, 0x37, 0xca, 0xc2, 0x8e, 0x1f, 0x92, 0x9e, 0xaf, 0xba, 0xe6, 0xaa, 0x8f, 0x0f, 0xf1, 0xeb,
				0x99, 0xb3, 0x56, 0xb7, 0x4d, 0x03, 0xba, 0xbb, 0xec, 0xa3, 0xd2, 0x5e, 0x5d, 0x82, 0xb3, 0xdd,
				0x03, 0x7f, 0xa7, 0x9e, 0xf1, 0x3e, 0xf1, 0x25, 0x70, 0xcd, 0x00, 0x22, 0xcb, 0x00, 0x46, 0x0c,
				0xb2, 0x96, 0xee, 0x21, 0x1a, 0xf5, 0xea, 0xf2, 0x03, 0xcb, 0x1b, 0x59, 0x0c, 0x87, 0x35, 0x2a,
				0xb6, 0x14, 0x44, 0x74, 0xfc, 0xdd, 0x3c, 0x82, 0xd4, 0x6b, 0x72, 0x4d, 0x72, 0xf8, 0xec, 0xb1,
				// Public Exponent (3 bytes) = 65537
				0x01, 0x00, 0x01
			};

#if 1  
			uiReturnCode = HSM_UNKNOWN_ERROR;

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				uiReturnCode = ReadPublicKeyHSM(arrTempData, (int*)&uiTempDataLen, HSM_PUBKEY);
            
	            if( (uiReturnCode = ReadPublicKeyHSM(arrTempData, (int*)&uiTempDataLen, HSM_PUBKEY)) == HSM_SUCCESS)
	            {
	                packet->mLen = uiTempDataLen+6;
	                memcpy(&packet->mData[6], arrTempData, uiTempDataLen);
	            }
	            else
	            {
	                packet->mLen = 6;
	            }
	            
	            memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
	            memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
	            
	            break;
			}
			else
			{
#if 0
				U8 tempBuffer[300];
				U16 key_length = 0;
				if (hsm_export_public_key(HSM_PUBKEY, HSM_ALG_RSA, tempBuffer, &key_length) == HAL_OK)
				{
					U16 mod_len = (U16)tempBuffer[0] | ((U16)tempBuffer[1] << 8);
					U16 exp_offset = 2 + mod_len;
					U16 exp_len = (U16)tempBuffer[exp_offset] | ((U16)tempBuffer[exp_offset + 1] << 8);

					memcpy(arrTempData, &tempBuffer[2], mod_len);
					memcpy(&arrTempData[mod_len], &tempBuffer[exp_offset + 2], exp_len);

					uiTempDataLen = mod_len + exp_len;  // 260 bytes (256 mod + 4 exp)
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_READ_FAIL;
				}
#endif
				//fixed data - for test
				uiReturnCode = HSM_SUCCESS;
				uiTempDataLen = 259;
				memcpy(arrTempData, testPublicKey, 259);

				packet->mLen = uiTempDataLen + 6;
				memcpy(&packet->mData[6], arrTempData, uiTempDataLen);

				memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
				memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );

				break;
			}
#endif

			
		}
		
		case HSM_READ_RANDKEY://0x0126
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_READ_RANDKEY\r\n");
#endif
			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				// OLD: RSA-1024 (128 bytes ciphertext)
				uiReturnCode = TransTempKeyHSM(&pPayloadPtcl[2], 128);
			}
			else
			{
				if (hsm_import_aes_key(HSM_KEY_SESSION, &pPayloadPtcl[2], HSM_AES_128, true, false) == HAL_OK)
				{
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_WRITE_FAIL;
				}
			}

			packet->mLen = 6;
			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );

			break;
		}
		  
		case HSM_STORE_PRIVATEKEY://0x0127
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_PRIVATEKEY\r\n");
#endif
			ucPriKey_Number = pPayloadPtcl[2];

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				uiReturnCode = StorePrivateKeyHSM(&pPayloadPtcl[RSA_KEY_DATA_OFFSET], 528, ucPriKey_Number);
			}
			else
			{
				//HSM_RSAKey_t rsa_key;
				HSM_RSAKeyEn_t rsa_keyEn;
#if 1 //encrypt key import
				// Old format (528 bytes): N(256) + E(16) + D(256)
				rsa_keyEn.modulus_length = RSA_MODULUS_SIZE;
				memcpy(rsa_keyEn.modulus, &pPayloadPtcl[RSA_KEY_DATA_OFFSET], RSA_MODULUS_SIZE);
				//GLogI( "pPayloadPtcl [0~2]: ");
				//for(i=0;i<3;i++)GLogI("%02X",pPayloadPtcl[i]); 
#if defined(NEW_HSM_LOG_ENABLE)
				GLogI("\r\n");
				GLogI( "modulus : ");
				int i=0;
				for(i=0;i<RSA_MODULUS_SIZE;i++)	GLogI("%02X",rsa_keyEn.modulus[i]);
				GLogI("\r\n");
#endif
				//memcpy(rsa_keyEn.public_exp, &pPayloadPtcl[RSA_KEY_DATA_OFFSET + RSA_MODULUS_SIZE + 1], RSA_PUB_EXP_SIZE);
				rsa_keyEn.public_exp_size = 0;//private key only, always 0

				memcpy(rsa_keyEn.upper_private_exp, &pPayloadPtcl[RSA_KEY_DATA_OFFSET + RSA_MODULUS_SIZE + RSA_PUB_EXP_PADDED_SIZE], ENC_UPPER_PRIV_EXP_SIZE);
#if defined(NEW_HSM_LOG_ENABLE)
				GLogI( "upper_private_exp : ");
				for(i=0;i<ENC_UPPER_PRIV_EXP_SIZE;i++)GLogI("%02X",rsa_keyEn.upper_private_exp[i]);
				GLogI("\r\n");
#endif
				memcpy(rsa_keyEn.lower_private_exp, &pPayloadPtcl[RSA_KEY_DATA_OFFSET + RSA_MODULUS_SIZE + RSA_PUB_EXP_PADDED_SIZE + ENC_UPPER_PRIV_EXP_SIZE], ENC_LOWER_PRIV_EXP_SIZE);
#if defined(NEW_HSM_LOG_ENABLE)
				GLogI( "lower_private_exp : ");
				for(i=0;i<ENC_LOWER_PRIV_EXP_SIZE;i++)GLogI("%02X",rsa_keyEn.lower_private_exp[i]);
				GLogI("\r\n");
#endif
				if (hsm_import_rsa_key_en(ucPriKey_Number + 140, &rsa_keyEn, 1, true, false, HSM_RSA_2048) == HAL_OK)
				{
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_WRITE_FAIL;
				}
#else //plain key import
				// Old format (528 bytes): N(256) + E(16) + D(256)
				memcpy(rsa_key.modulus, &pPayloadPtcl[RSA_KEY_DATA_OFFSET], RSA_MODULUS_SIZE);
				rsa_key.modulus_length = RSA_MODULUS_SIZE;

				memcpy(rsa_key.public_exp, &pPayloadPtcl[RSA_KEY_DATA_OFFSET + RSA_MODULUS_SIZE + 1], RSA_PUB_EXP_SIZE);
				rsa_key.public_exp_size = RSA_PUB_EXP_SIZE;

				memcpy(rsa_key.private_exp, &pPayloadPtcl[RSA_KEY_DATA_OFFSET + RSA_MODULUS_SIZE + RSA_PUB_EXP_PADDED_SIZE], RSA_PRIV_EXP_SIZE);

				if (hsm_import_rsa_key(ucPriKey_Number + 140, &rsa_key, false, false, false, HSM_RSA_2048) == HAL_OK)
				{
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_WRITE_FAIL;
				}
#endif
			}

			packet->mLen = 6;
			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );

			break;
		}
	#endif

		case HSM_GET_SEEDKEY://0x0123
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_GET_SEEDKEY\r\n");
#endif
			ucPriKey_Number = pPayloadPtcl[14];
			memcpy( &uiTempDataLen, &pPayloadPtcl[2], sizeof(uiTempDataLen) );

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				uiReturnCode = ActivationHSM();

				if(ucPriKey_Number%2 == 0) //SHA2
				{
					uiReturnCode = SignPrivateHSMSHA256(&pPayloadPtcl[6], uiTempDataLen, arrOutData, (int*)&uiOutDataLen, ucPriKey_Number);
				}
				else //SHA1
				{
					uiReturnCode = SignPrivateHSM(&pPayloadPtcl[6], uiTempDataLen, arrOutData, (int*)&uiOutDataLen, ucPriKey_Number);
				}
			}
			else
			{
				// Use unified function for both SHA-1 and SHA-256
				// SHA-1 (odd key number): seed directly placed (no hash) - OLD HSM SignPrivateHSM compatible
				// SHA-256 (even key number): SHA256(seed) calculated - OLD HSM SignPrivateHSMSHA256 compatible
				// Key ID mapping: ucPriKey_Number + 140 = RSA UDK slot (per 8.6 Key Index)
				uint16_t rsa_key_slot = ucPriKey_Number + 140U;
				uint8_t sha_type = (ucPriKey_Number % 2 == 0) ? HSM_SHA_256 : HSM_SHA_160;

				if (hsm_sign_rsa_with_seed_new(&pPayloadPtcl[6], uiTempDataLen,
				                               arrOutData, &uiOutDataLen,
				                               rsa_key_slot, sha_type) == HAL_OK)
				{
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_SIGN_FAIL;
				}
			}

			packet->mLen = uiOutDataLen + 6 + 4 + 1;

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			packet->mData[6] = ucPriKey_Number;
			memcpy( &packet->mData[7],&uiOutDataLen,  4 );
			memcpy( &packet->mData[11],&arrOutData[0], uiOutDataLen );
			break;
		}

		case HSM_GET_INVALID_DATE://0x0125
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_GET_INVALID_DATE\r\n");
#endif
			packet->mLen	= 14;

			ucCRT_Number = pPayloadPtcl[2];

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				uiReturnCode = ActivationHSM();

				if(uiReturnCode == HSM_SUCCESS)
				{
					uiReturnCode = ReadCertificateHSM(arrOutData, (int*)&uiOutDataLen, ucCRT_Number);
				}
			}
			else
			{
				U8 cert_buffer[616];
				U16 cert_length = 0;
				U16 mapped_cert_id = 140 + ucCRT_Number;

				if (hsm_read_certificate(mapped_cert_id, HSM_ALG_RSA, cert_buffer, &cert_length) == HAL_OK)
				{
					memcpy(arrOutData, cert_buffer, 600);
					uiOutDataLen = 600;
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_READ_FAIL;
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, 	sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  	sizeof(uiReturnCode) );
			memcpy( &packet->mData[6],&arrOutData[0], 	HSM_CERTI_VER_LEN );
			memcpy( &packet->mData[7],&arrOutData[15], 	HSM_INVALID_DATE_LEN );
			packet->mData[13]=ucCRT_Number;
			break;
		}

		case HSM_STORE_AESKEY:
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_AESKEY\r\n");
#endif
			uiReturnCode = HSM_UNKNOWN_ERROR;

			U8 DerivedKey[16] = {0x00, };
			U32 Decrypted_KeyLen = 0;

			U32 KEY_LEN = (pPayloadPtcl[2])+
						  (pPayloadPtcl[3]<<8)+
						  (pPayloadPtcl[4]<<16)+
						  (pPayloadPtcl[5]<<24);

			getAESDecoding_ECB(&pPayloadPtcl[6], KEY_LEN, Q_Key, AES256, DerivedKey, &Decrypted_KeyLen );

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				U8 PubKey[200] = {0x00, };
				U32 PubKeyLen = 0;
				U8 Encrypted_DerivedKey[150] = {0x00, };

				uiReturnCode = ReadPublicKeyHSM(PubKey, (int*)&PubKeyLen, HSM_PUBKEY);
				if( uiReturnCode == HSM_SUCCESS)
				{
					uiReturnCode = setRSA_Encrypt(&DerivedKey[0], Decrypted_KeyLen, PubKey, PubKeyLen , Encrypted_DerivedKey);
					if( uiReturnCode == CMOX_RSA_SUCCESS)
					{
						uiReturnCode = StoreAES128HSM(&Encrypted_DerivedKey[0], 128, AES_INDEX_1, &ucChecksum);
					}
				}
			}
			else
			{
				if (hsm_import_aes_key(AES_INDEX_1, DerivedKey, Decrypted_KeyLen, false, false) == HAL_OK)
				{
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_WRITE_FAIL;
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			packet->mLen = 6;
			break;
		}

	case HSM_STORE_HMACKEY://0x0114
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_HMACKEY\r\n");
#endif
			uiReturnCode = HSM_UNKNOWN_ERROR;

			U8 HMAC_Key[32] = {0x00, };
			U32 Decrypted_KeyLen = 0;

			U32 KEY_LEN = (pPayloadPtcl[2])+
						  (pPayloadPtcl[3]<<8)+
						  (pPayloadPtcl[4]<<16)+
						  (pPayloadPtcl[5]<<24);

			getAESDecoding_ECB(&pPayloadPtcl[6], KEY_LEN, Q_Key, AES256, HMAC_Key, &Decrypted_KeyLen );

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				U8 PubKey[200] = {0x00, };
				U32 PubKeyLen = 0;
				U8 Encrypted_HMAC_Key[150] = {0x00, };

				uiReturnCode = ReadPublicKeyHSM(PubKey, (int*)&PubKeyLen, HSM_PUBKEY);
				if( uiReturnCode == HSM_SUCCESS)
				{
					uiReturnCode = setRSA_Encrypt(&HMAC_Key[0], Decrypted_KeyLen, PubKey, PubKeyLen, Encrypted_HMAC_Key);
					if(uiReturnCode == CMOX_RSA_SUCCESS)
					{
						uiReturnCode = StoreHMACSHA256KeyHSM(&Encrypted_HMAC_Key[0], 128, HMAC_INDEX_1);
					}
				}
			}
			else
			{
				if (hsm_import_aes_key(HMAC_INDEX_1, HMAC_Key, Decrypted_KeyLen, false, false) == HAL_OK)
				{
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_WRITE_FAIL;
				}
			}

			packet->mLen = 6;
			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			break;
		}
		
		case HSM_STORE_ECU_CODE_KEY:
		{
#if defined(NEW_HSM_LOG_ENABLE)			
			GLogI( "HSM_STORE_ECU_CODE_KEY\r\n");
#endif
			uiReturnCode = HSM_UNKNOWN_ERROR;

			U32 Decrypted_KeyLen = 0;
			U8 EcuCode_Key[32] = {0x00, };
			U32 KEY_LEN = (pPayloadPtcl[2])+
						  (pPayloadPtcl[3]<<8)+
						  (pPayloadPtcl[4]<<16)+
						  (pPayloadPtcl[5]<<24);

			packet->mLen = 6;

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				// OLD HSM: No checksum verification
				getAESDecoding_ECB(&pPayloadPtcl[6], KEY_LEN, CODE_Key, AES128, EcuCode_Key, &Decrypted_KeyLen);

				U8 PubKey[200] = {0x00, };
				U32 PubKeyLen = 0;
				U8 Encrypted_EcuCode_Key[150] = {0x00, };

				uiReturnCode = ReadPublicKeyHSM(PubKey, (int*)&PubKeyLen, HSM_PUBKEY);
				if( uiReturnCode == HSM_SUCCESS)
				{
					uiReturnCode = setRSA_Encrypt(&EcuCode_Key[0], Decrypted_KeyLen, PubKey, PubKeyLen, Encrypted_EcuCode_Key);
					if(uiReturnCode == CMOX_RSA_SUCCESS)
					{
						uiReturnCode = StoreAES128KeyHSMforECUcode(&Encrypted_EcuCode_Key[0], 128, &ucChecksum);
					}
				}
			}
			else
			{
				// NEW HSM: Checksum verification before key import
				// Expected data checksum: sum of all 256 bytes = 0x84BC
				

				U32 received_checksum = 0U;
				bool checksum_valid = false;

				if (KEY_LEN == ECU_CODE_KEY_EXPECTED_LEN)
				{
					for (U32 i = 0U; i < KEY_LEN; i++)
					{
						received_checksum += pPayloadPtcl[6U + i];
					}

					if (received_checksum == ECU_CODE_KEY_EXPECTED_CHECKSUM)
					{
						checksum_valid = true;
#if defined(NEW_HSM_LOG_ENABLE)
						GLogN("[HSM_STORE_ECU_CODE_KEY] Checksum OK: 0x%04X\r\n", received_checksum);
#endif
					}
					else
					{
						GLogE("[HSM_STORE_ECU_CODE_KEY] Checksum FAIL: expected 0x%04X, got 0x%04X\r\n", ECU_CODE_KEY_EXPECTED_CHECKSUM, received_checksum);
					}
				}
				else
				{
					GLogE("[HSM_STORE_ECU_CODE_KEY] Length FAIL: expected %d, got %d\r\n",
					      ECU_CODE_KEY_EXPECTED_LEN, KEY_LEN);
				}

				if (checksum_valid)
				{
					if (hsm_import_aes_key(HSM_KEY_ECU_CODE, &pPayloadPtcl[6], HSM_AES_128, true, false) == HAL_OK)
					{
						uiReturnCode = HSM_SUCCESS;
					}
					else
					{
						uiReturnCode = HSM_WRITE_FAIL;
					}
				}
				else
				{
					uiReturnCode = HSM_CHECKSUM_ERROR;
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );

			break;
		}
		
		case HSM_STORE_CRL:
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_CRL\r\n");
#endif
			U8 ucCrlNo = 0;

			memset(&packet->mData[0], 0x00, 600);

			uiReturnCode = HSM_UNKNOWN_ERROR;
			ucCrlNo = pPayloadPtcl[2];
			packet->mData[6] = ucCrlNo;
			packet->mLen = 508;

#if defined(NEW_HSM_LOG_ENABLE)
			GLogN("\r\nStore CRL : %d", ucCrlNo);
#endif
			if((ucCrlNo > 7) || (ucCrlNo == 0))
			{
				GLogE( "CRL Payload ERROR!! CRLNumber(%d)\r\n", ucCrlNo );
			}
			else
			{
				if (g_HSM_Type == HSM_TYPE_OLD)
				{
					uiReturnCode = WriteDataHSM(&pPayloadPtcl[3], 501, ucCrlNo);

					if(uiReturnCode == HSM_SUCCESS)
					{
						GLogI( "StoreCrl ok!!\r\n" );
						uiReturnCode = ReadDataHSM(&packet->mData[7], 501, ucCrlNo);
					}
				}
				else
				{
					U32 read_len = 0;
					FRESULT result;

					result = StoreCrlEmmc(ucCrlNo, &pPayloadPtcl[3], 501);
					if (result == FR_OK)
					{
						GLogI( "StoreCrl ok!!\r\n" );

						if (GetCrlEmmc(ucCrlNo, &packet->mData[7], &read_len) == TRUE)
						{
							uiReturnCode = HSM_SUCCESS;
						}
						else
						{
							uiReturnCode = HSM_READ_FAIL;
						}
					}
					else
					{
						uiReturnCode = HSM_WRITE_FAIL;
					}
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );

			break;
		}

		case HSM_STORE_CERTI_CV_KD:
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_CERTI_CV_KD\r\n");
#endif
			ucCRT_Number = pPayloadPtcl[2];
#if defined(NEW_HSM_LOG_ENABLE)
			GLogN("\r\nStore Cert KD : %d", ucCRT_Number);
#endif

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				// Old HSM: Not supported
				uiReturnCode = HSM_NOT_SUPPORTED;
				packet->mLen = 6;
			}
			else
			{
				U16 cert_length = 600;
				U16 read_cert_length = 0;
				U8 cert_buffer[616];

				packet->mLen = 611;

				/* Map App cert number (1~7) to NEW HSM RSA cert slot (141~147)
				 * NEW HSM RSA cert slots: #141~148 per SPI Command Manual v3 */
				U16 mapped_cert_id = 140 + ucCRT_Number;

				// Store certificate
				if (hsm_store_certificate(mapped_cert_id, HSM_ALG_RSA, &pPayloadPtcl[3], cert_length) == HAL_OK)
				{
					packet->mData[10] = ucCRT_Number;

					// Read certificate
					if (hsm_read_certificate(mapped_cert_id, HSM_ALG_RSA, cert_buffer, &read_cert_length) == HAL_OK)
					{
						// New HSM returns 616 bytes, but packet stores only 600 bytes
						memcpy(&packet->mData[11], cert_buffer, 600);
						uiOutDataLen = 600;
						memcpy(&packet->mData[6], &uiOutDataLen, sizeof(uiOutDataLen));
						uiReturnCode = HSM_SUCCESS;
					}
					else
					{
						uiReturnCode = HSM_READ_FAIL;
					}
				}
				else
				{
					uiReturnCode = HSM_WRITE_FAIL;
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			break;
		}

		case HSM_STORE_PRIVATEKEY_CV_KD://0x0132
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_PRIVATEKEY_CV_KD\r\n");
#endif
			ucPriKey_Number = pPayloadPtcl[2];

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				// Old HSM: Not supported
				uiReturnCode = HSM_NOT_SUPPORTED;
				packet->mLen = 6;
			}
			else
			{
				HSM_RSAKey_t rsa_key;
				U8 pub_exp_actual_len;
				int i;

				// Check data format by usLength
				if (usLength == 531)
				{
					// Old format (528 bytes): N(256) + E(16) + D(256)
					memcpy(rsa_key.modulus, &pPayloadPtcl[3], 256);
					rsa_key.modulus_length = 256;

					// Convert Public Exponent (16 bytes -> actual length)
					pub_exp_actual_len = 0;
					for (i = 0; i < 16; i++)
					{
						if (pPayloadPtcl[3 + 256 + i] != 0x00)
						{
							pub_exp_actual_len = 16 - i;
							memcpy(rsa_key.public_exp, &pPayloadPtcl[3 + 256 + i], pub_exp_actual_len);
							break;
						}
					}
					rsa_key.public_exp_size = pub_exp_actual_len;

					// Copy Private Exponent (256 bytes)
					memcpy(rsa_key.private_exp, &pPayloadPtcl[3 + 256 + 16], 256);
				}
				else
				{
					// New format (516 bytes): N(256) + E(3) + D(256)
					memcpy(rsa_key.modulus, &pPayloadPtcl[3], 256);
					rsa_key.modulus_length = 256;

					memcpy(rsa_key.public_exp, &pPayloadPtcl[3 + 256], 3);
					rsa_key.public_exp_size = 3;

					memcpy(rsa_key.private_exp, &pPayloadPtcl[3 + 256 + 3], 256);
				}

				// Import RSA key to HSM
				if (hsm_import_rsa_key(ucPriKey_Number, &rsa_key, false, false, false, HSM_RSA_2048) == HAL_OK)
				{
					uiReturnCode = HSM_SUCCESS;
				}
				else
				{
					uiReturnCode = HSM_WRITE_FAIL;
				}

				packet->mLen = 7;
				packet->mData[6] = ucPriKey_Number;
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			break;
		}

		case HSM_STORE_CRL_CV_KD://0x0133
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_STORE_CRL_CV_KD\r\n");
#endif
			U8 ucCrlNo = 0;

			memset(&packet->mData[0], 0x00, 600);

			uiReturnCode = HSM_UNKNOWN_ERROR;
			ucCrlNo = pPayloadPtcl[2];
			packet->mData[6] = ucCrlNo;
			packet->mLen = 508;
#if defined(NEW_HSM_LOG_ENABLE)
			GLogN("\r\nStore CRL KD : %d", ucCrlNo);
#endif
			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				// Old HSM: Not supported
				uiReturnCode = HSM_NOT_SUPPORTED;
			}
			else
			{
				U32 read_len = 0;
				FRESULT result;

				// Validate CRL number (KD uses different range)
				if(ucCrlNo == 0 || ucCrlNo > 30)
				{
					GLogE( "CRL KD Payload ERROR!! CRLNumber(%d)\r\n", ucCrlNo );
					uiReturnCode = HSM_UNKNOWN_ERROR;
				}
				else
				{
					result = StoreCrlEmmc(ucCrlNo, &pPayloadPtcl[3], 501);
					if (result == FR_OK)
					{
						GLogI( "StoreCrl KD ok!!\r\n" );

						if (GetCrlEmmc(ucCrlNo, &packet->mData[7], &read_len) == TRUE)
						{
							uiReturnCode = HSM_SUCCESS;
						}
						else
						{
							uiReturnCode = HSM_READ_FAIL;
						}
					}
					else
					{
						uiReturnCode = HSM_WRITE_FAIL;
					}
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );

			break;
		}


		case HSM_CHECK_CODEKEY://0x0129
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_CHECK_CODEKEY\r\n");
#endif
			U8 ucChecksum = 0;

			packet->mLen = 3;

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				uiReturnCode = ActivationHSM();
				if(uiReturnCode == HSM_SUCCESS)
				{
					uiReturnCode = ReadAES128KeyChecksumHSM(AES_INDEX_6, &ucChecksum);

					if((ucChecksum==0x7B) && (uiReturnCode==HSM_SUCCESS)) memcpy( &packet->mData[2], &uiReturnCode, sizeof(uiReturnCode) );
					else memcpy( &packet->mData[2], &uiReturnCode, sizeof(uiReturnCode) );
				}
			}
			else
			{
//				// ECU Code Key (#101 slot) verify
//				static const U8 verify_seed[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
//				static const U8 verify_ecu_code[16] = {
//					0x87, 0x87, 0xB6, 0xB1, 0xF3, 0xF9, 0x46, 0xB7,
//					0xB0, 0x2C, 0x39, 0xEE, 0xD3, 0xC2, 0x99, 0xC0
//				};
//				static const U8 verify_iv[16] = {
//					0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48,
//					0x53, 0x4D, 0x41, 0x45, 0x53, 0x5F, 0x49, 0x56
//				};
//				// Expected for master_id=0 (VCI3 PV), Seed[0]
//				static const U8 verify_expected[8] = {0xDE, 0x93, 0xC3, 0xD1, 0x60, 0x53, 0x0E, 0x20};

//				U8 ask_result[8];
//				if (hsm_generate_ask_key((U8*)verify_seed, (U8*)verify_ecu_code, 0, (U8*)verify_iv, ask_result) == HAL_OK)
//				{
//					if (memcmp(ask_result, verify_expected, 8) == 0)
//					{
						uiReturnCode = HSM_SUCCESS;
//					}
//					else
//					{
//						uiReturnCode = HSM_UNKNOWN_ERROR;  // ECU Code Key mismatch
//					}
//				}
//				else
//				{
//					uiReturnCode = HSM_UNKNOWN_ERROR;
//				}
				memcpy( &packet->mData[2], &uiReturnCode, sizeof(uiReturnCode) );
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );

			break;
		}
#if 1
		case HSM_SELFTEST_PRIVATEKEY://0x0130
		{
#if defined(NEW_HSM_LOG_ENABLE)
			GLogI( "HSM_SELFTEST_PRIVATEKEY\r\n");
#endif
			packet->mLen	= 6;
			uiReturnCode = HSM_UNKNOWN_ERROR;

			if (g_HSM_Type == HSM_TYPE_OLD)
			{
				if( (ReadPublicKeyHSM(arrTempData, (int*)&uiTempDataLen, HSM_PUBKEY)) == HSM_SUCCESS)
				{
					if( getRandom(4, (uint32_t*)&arrRandData[0]) ==  HAL_OK)
					{
						if( setRSA_Encrypt(&arrRandData[0], 16, arrTempData, uiTempDataLen , arrOutData) == CMOX_RSA_SUCCESS)
						{
							if( (TransTempKeyHSM(arrOutData, 128)) == HSM_SUCCESS)
							{
								if( getAESEncoding_ECB(&pPayloadPtcl[3], 256, arrRandData, AES128, arrOutData, &uiOutDataLen ) == CMOX_CIPHER_SUCCESS)
								{
									memset(arrTempAES, 0x00, 16);
									arrTempAES[1] = 0x01;
									arrTempAES[3] = 0x01;
									arrTempAES[4] = 0x80;

									if( getAESEncoding_ECB(arrTempAES, 16, arrRandData, AES128, &arrOutData[uiOutDataLen], &uiTempDataLen ) == CMOX_CIPHER_SUCCESS)
									{
										if( getAESEncoding_ECB(&pPayloadPtcl[263], 256, arrRandData, AES128, &arrOutData[uiOutDataLen + uiTempDataLen], &uiOutDataLen ) == CMOX_CIPHER_SUCCESS)
										{
											uiReturnCode = StorePrivateKeyHSM(&arrOutData[0], 528, pPayloadPtcl[2]);
										}
									}
								}
							}
						}
					}
				}
			}
			else
			{
				// Self-test: Verify HSM's RSA key pair works correctly, then store private key
				// Input format: pPayloadPtcl[2] = key_number, [3:259] = N (256), [263:519] = D (256)

				U16 pub_key_length = 0;

				// Step 1: Export public key from HSM (RSA-2048)
				if (hsm_export_public_key(HSM_PUBKEY, HSM_ALG_RSA, arrTempData, &pub_key_length) == HAL_OK)
				{
					// Step 2: Generate random 16 bytes for self-test using NEW HSM TRNG (OP 0x10)
					if (hsm_generate_random(arrRandData, 16, true) == HAL_OK)
					{
						// Step 3: Encrypt random data using HSM's public key (internal HSM operation)
						if (hsm_rsa_encrypt(HSM_PUBKEY, &arrRandData[0], 16, arrOutData) == HAL_OK)
						{
							// Step 4: Decrypt using HSM's private key to verify key pair
							U16 decrypted_len = 0;
							uint8_t arrDecrypted[32] = {0};

							if (hsm_rsa_decrypt(HSM_PUBKEY, arrOutData, arrDecrypted, &decrypted_len) == HAL_OK)
							{
								// Step 5: Verify decryption matches original random data
								if (decrypted_len == 16 && memcmp(arrRandData, arrDecrypted, 16) == 0)
								{
									// Step 6: HSM RSA key pair verified - now store the input private key
									// Build RSA key structure from input data
									HSM_RSAKey_t rsa_key;

									rsa_key.modulus_length = 256;
									memcpy(rsa_key.modulus, &pPayloadPtcl[3], 256);  // N from input

									// Public exponent = 65537 (0x010001)
									rsa_key.public_exp_size = 3;
									rsa_key.public_exp[0] = 0x01;
									rsa_key.public_exp[1] = 0x00;
									rsa_key.public_exp[2] = 0x01;

									memcpy(rsa_key.private_exp, &pPayloadPtcl[263], 256);  // D from input

									// Store private key (key_id = 361 + key_number for RSA keys)
									uint16_t key_id = 361 + pPayloadPtcl[2];

									if (hsm_import_rsa_key(key_id, &rsa_key, false, false, false, HSM_RSA_2048) == HAL_OK)
									{
										uiReturnCode = HSM_SUCCESS;
									}
								}
							}
						}
					}
				}
			}

			memcpy( &packet->mData[0],&usHSMinitMode, sizeof(usHSMinitMode) );
			memcpy( &packet->mData[2],&uiReturnCode,  sizeof(uiReturnCode) );
			break;
		}
#endif

		default:
		break;
	}
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void ClearDiagMessage()
{
	osEvent		evt;
	MsgDiag_t	*message;
	PTmsgPkt_t	*packet;

	//printf("%s] enter\r\n", __func__);
	for(int ii=0;ii<MESSAGE_DIAGNOSTIC_QUEUE_SIZE ;ii++)
	{
		evt = osMessageGet( hDiagMsg, 0 );
		if( evt.status == osEventMessage )
		{
			message = ( MsgDiag_t * )evt.value.p;
			packet	= ( PTmsgPkt_t* )message->pPacket;

			printf("%s] del diag packet old addr : %x\r\n", __func__, packet);

			osPoolFree( hPTPKPool, (void *)packet );
			osPoolFree( hDiagPool, (void *)message );
		}
	}

}
void TransmitFunction( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID )
{
	stCommPkt	*packet;
	stMsgClst	*message;

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= usLength;

	packet->pTarget	= (void *)eInCommType;
	packet->mFuncID = usFuncID;
	packet->mCurFrame = 0;
	
	memcpy(&packet->mData[0], pData, usLength);

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;
    
    if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

/*
 * Same as TransmitFunction(), but copies the given UUID into the packet so that
 * MQTT/WebSocket transports can correlate this packet with an originating session.
 * Pass NULL for pUUID to behave identical to TransmitFunction() (UUID stays zero).
 */
void TransmitFunction_WithUUID( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID, const UUID_Struct *pUUID )
{
	stCommPkt	*packet;
	stMsgClst	*message;

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= usLength;
	packet->pTarget	= (void *)eInCommType;
	packet->mFuncID = usFuncID;
	packet->mCurFrame = 0;

	memcpy(&packet->mData[0], pData, usLength);

	if( pUUID != NULL )
	{
		memcpy(&(packet->UUID), pUUID, sizeof(UUID_Struct));
	}

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void TransmitFunction_IT( ePKT_TD eInCommType, uint8_t *pData, unsigned short int usLength, unsigned short int usFuncID )
{
	stCommPkt	*packet;
	stMsgClst	*message;

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= usLength;
	packet->pTarget	= (void *)eInCommType;
	packet->mFuncID = usFuncID;
	packet->mCurFrame = 0;

	memcpy(&packet->mData[0], pData, usLength);

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

    if(xQueueIsQueueFullFromISR(hTransmitMsg) == TRUE)
    {
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#ifdef LISTDIAG
#ifdef VCI3_DIAG
void FL_GitListDiag( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet,*pktAppRes;
	stMsgClst	*message,*msgAppRes;
	uint8_t 			*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	
	// Response to App
	msgAppRes = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( msgAppRes == NULL )
	{
		GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
		return;
	}

	pktAppRes	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( pktAppRes == NULL )
	{
		GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
		osPoolFree( hMsgPool, (void *)msgAppRes );
		return;
	}
	pktAppRes->mLen	= 1;

	if( usLength< LISTDIAGSIZE )	//Run ListDiag
    {
		pktAppRes->mData[0] = 0x00;
		message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
		if( message == NULL )
		{
			GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
			return;
		}

		packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
		if( packet == NULL )
		{
			GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
			osPoolFree( hMsgPool, (void *)message );
			return;
		}

		packet->mLen	= usLength;
		packet->pTarget	= &eInCommType;
		packet->mFuncID = LISTDIAG_START;
		packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
		
		memcpy(&packet->mData[0], pPayloadPtcl, usLength);

		message->mPktType	= (ePKT_TD)eInCommType;
		message->pPacket	= (void *)packet;
		message->mMod	= LISTDIAG_INIT;

		if(osMessageAvailableSpace(hListDiagMsg) == 0)
		{
			osPoolFree( hCommPKPool, (void *)packet );
			osPoolFree( hMsgPool, (void *)message );
		}
		else
		{
			osMessagePut( hListDiagMsg, (uint32_t)message, osWaitForever );
		}
    }
	else
	{
		pktAppRes->mData[0] = 0x01;
	}
	pktAppRes->pTarget	= &eInCommType;
	pktAppRes->mFuncID = LISTDIAG_START;

	pktAppRes->mCS 	= CalcChecksumGITPtclPayloadFrame(pktAppRes);

	msgAppRes->mPktType	= (ePKT_TD)eInCommType;
	msgAppRes->pPacket	= (void *)pktAppRes;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)pktAppRes );
		osPoolFree( hMsgPool, (void *)msgAppRes );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)msgAppRes, osWaitForever );
	}

}
#endif
void FL_GitListSensor( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet,*pktAppRes;
	stMsgClst	*message,*msgAppRes;
	uint8_t *pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
	
	
	if (g_OBD_Processing == true)
	{
		g_ListSensor_Endflag = false;
		osSignalWait(OBD_ListDiag_END, osWaitForever);
	}
	
	// Response to App
	msgAppRes = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( msgAppRes == NULL )
	{
		GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
		return;
	}

	pktAppRes	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( pktAppRes == NULL )
	{
		GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
		osPoolFree( hMsgPool, (void *)msgAppRes );
		return;
	}
	
	pktAppRes->mLen	= 1;

	if( usLength< LISTDIAGSIZE )	//Run ListDiag
    {
		pktAppRes->mData[0] = 0x00;
		message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
		if( message == NULL )
		{
			GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
			return;
		}

		packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
		if( packet == NULL )
		{
			GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
			osPoolFree( hMsgPool, (void *)message );
			return;
		}

		packet->mLen	= usLength;
		packet->pTarget	= &eInCommType;
		packet->mFuncID = LISTSENSOR_START;
		packet->mCurFrame = 0;
		memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
		
		memcpy(&packet->mData[0], pPayloadPtcl, usLength);

		message->mPktType	= (ePKT_TD)eInCommType;
		message->pPacket	= (void *)packet;
		message->mMod		= LISTSENSOR_INIT;

		if(osMessageAvailableSpace(hListDiagMsg) == 0)
		{
			osPoolFree( hCommPKPool, (void *)packet );
			osPoolFree( hMsgPool, (void *)message );
		}
		else
		{
		  	
			osMessagePut( hListDiagMsg, (uint32_t)message, osWaitForever );
		}
    }
	else
	{
		pktAppRes->mData[0] = 0x01;
	}
	pktAppRes->pTarget	= &eInCommType;
	pktAppRes->mFuncID = LISTSENSOR_START;
	pktAppRes->mCurFrame = 0;
	memcpy(&(pktAppRes->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	pktAppRes->mCS 	= CalcChecksumGITPtclPayloadFrame(pktAppRes);

	msgAppRes->mPktType	= (ePKT_TD)eInCommType;
	msgAppRes->pPacket	= (void *)pktAppRes;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)pktAppRes );
		osPoolFree( hMsgPool, (void *)msgAppRes );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)msgAppRes, osWaitForever );
	}

}
#endif

#ifdef USE_RELAY_MOSA
#ifdef VCI3_DIAG
void FL_GitBatRelayCon(stCommPkt *pkt, uint32_t eInCommType  )
{
}
#endif
#endif

#ifdef VCI3_DIAG
void FL_RS9116UpdateStart(stCommPkt *pkt, uint32_t eInCommType  )//GitInitHWVariable
{
	stCommPkt	*packet;
	stMsgClst	*message;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0xE036;
    packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
    packet->mData[0] = 0x00;
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
    osDelay(1000);
    
    RS9116FWUpdate();
}
#endif

#ifdef VCI3_DIAG
void FL_RS9116VersionCheck( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	
	uint8_t	wlanFWVer[RS9116VER_LEN] = { 0, };

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
		return;
	}

	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	rsi_wlan_get( RSI_FW_VERSION, wlanFWVer, sizeof(wlanFWVer) );
	wlanFWVer[RS9116VER_LEN-1]=0;
	GLogN( "RS9116FW:%s\r\n",wlanFWVer);
#ifdef FW_TEST_VERSION_OVERRIDE
	Load_RS9116_TestVersion(wlanFWVer, sizeof(wlanFWVer));
#else
	//strcpy((char*)wlanFWVer, "1610.2.10.0.0.5");//for test
    //GLogN( "RS9116FW(forced):%s\r\n",wlanFWVer);
#endif
	

	packet->mLen	= RS9116VER_LEN;
	memcpy((char*)&packet->mData[0],&wlanFWVer,RS9116VER_LEN);

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0xE037;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_Git_CSAC20_Req( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*csacpacket;
	stMsgClst	*csacmessage;
    uint8_t		    *pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif
    
    csacmessage = ( stMsgClst* )osPoolCAlloc( hMsgPool );
    if( csacmessage == NULL )
    {
        GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
        return;
    }

    csacpacket	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
    if( csacpacket == NULL )
    {
        GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
        osPoolFree( hMsgPool, (void *)csacmessage );
        return;
    }
    
#if 1
    GLogN("\r\nCSAC20 Num : %d, %d", pPayloadPtcl[0], pPayloadPtcl[1]);
    memcpy(&csacpacket->mData[0], &pPayloadPtcl[0], usLength);
#else //Test
    csacpacket->mData[0] = 0x00; //pPayloadPtcl[0];
    csacpacket->mData[1] = 0x00; //pPayloadPtcl[1];
    csacpacket->mData[2] = 0x00; //pPayloadPtcl[2];
    csacpacket->mData[3] = 0x00; //pPayloadPtcl[3];
    csacpacket->mData[4] = 0x05; //pPayloadPtcl[4];
    csacpacket->mData[5] = 0xD0; //pPayloadPtcl[5];
    csacpacket->mData[6] = 0x27; //pPayloadPtcl[6];
    csacpacket->mData[7] = 0x41; //pPayloadPtcl[7];
#endif
    csacpacket->mFuncID = 0x1211;
    
    csacmessage->mPktType	= (ePKT_TD)eInCommType;
    csacmessage->pPacket	= (void *)csacpacket;
    csacmessage->mMod	= LISTDIAG_INIT;

    if(osMessageAvailableSpace(hCsacDiagMsg) == 0)
    {
        osPoolFree( hCommPKPool, (void *)csacpacket );
        osPoolFree( hMsgPool, (void *)csacmessage );
    }
    else
    {
        osMessagePut( hCsacDiagMsg, (uint32_t)csacmessage, osWaitForever );
    }
}
#endif

#ifdef VCI3_DIAG
void FL_Git_CSAC10_Req( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*csacpacket;
	stMsgClst	*csacmessage;
    uint8_t		    *pPayloadPtcl = pkt->mData;
    
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif
    
    csacmessage = ( stMsgClst* )osPoolCAlloc( hMsgPool );
    if( csacmessage == NULL )
    {
        GLogEE( "Fail... hMsgPool Alloc!!!\r\n" );
        return;
    }

    csacpacket	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
    if( csacpacket == NULL )
    {
        GLogEE( "Fail... hCommPKPool Alloc!!!\r\n" );
        osPoolFree( hMsgPool, (void *)csacmessage );
        return;
    }
    
#if 1
    GLogN("\r\nCSAC10 Num : %d", pPayloadPtcl[0]);
    memcpy(&csacpacket->mData[0], &pPayloadPtcl[0], 2); //CRT Number
    memcpy(&csacpacket->mData[2], &pPayloadPtcl[1], 6); //CAN Data
#else //Test
    
#if 0 //PV
    csacpacket->mData[0] = 0x00; //pPayloadPtcl[0];
    csacpacket->mData[1] = 0x00; //pPayloadPtcl[1];
    csacpacket->mData[2] = 0x00; //pPayloadPtcl[2];
    csacpacket->mData[3] = 0x00; //pPayloadPtcl[3];
    csacpacket->mData[4] = 0x07; //pPayloadPtcl[4];
    csacpacket->mData[5] = 0x70; //pPayloadPtcl[5];
    csacpacket->mData[6] = 0x27; //pPayloadPtcl[6];
    csacpacket->mData[7] = 0x41; //pPayloadPtcl[7];
#endif
    
#endif
    csacpacket->mFuncID = 0x1212;
    
    csacmessage->mPktType	= (ePKT_TD)eInCommType;
    csacmessage->pPacket	= (void *)csacpacket;
    csacmessage->mMod	= LISTDIAG_INIT;

    if(osMessageAvailableSpace(hCsacDiagMsg) == 0)
    {
        osPoolFree( hCommPKPool, (void *)csacpacket );
        osPoolFree( hMsgPool, (void *)csacmessage );
    }
    else
    {
        osMessagePut( hCsacDiagMsg, (uint32_t)csacmessage, osWaitForever );
    }
}
#endif

#ifdef VCI3_DIAG
void FL_Git_ASK_Req( stCommPkt *pkt, uint32_t eInCommType  )
{
    stCommPkt	*packet;
	stMsgClst	*message;
#if 0 //It's a function that is no longer in use.
    uint8_t		*pPayloadPtcl = pkt->mData;
    U32 uiReturnCode = 0;
    uint8_t arrSeed[8] = {0x00, };
    uint8_t arrCode[16] = {0x00, };
    uint8_t arrIv[16] = {0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48, 0x53, 0x4D, 0x41, 0x45, 0x53, 0x5F, 0x49, 0x56};
    uint8_t arrOutdata[8] = {0x00, };
#endif
    
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    packet->mLen = 1;
    packet->mData[0] = 0x01; //Fail
#if 0 //It's a function that is no longer in use.
    memcpy(arrCode, &pPayloadPtcl[0], sizeof(arrCode));
    memcpy(arrSeed, &pPayloadPtcl[16], sizeof(arrSeed));
    
    uiReturnCode = ActivationHSM();
    
    if(uiReturnCode == HSM_SUCCESS)
    {
        uiReturnCode = ASKSignHSM(arrSeed, arrCode, arrIv, arrOutdata);
        
        if(uiReturnCode == HSM_SUCCESS)
        {
            packet->mLen = 9;
            packet->mData[0] = 0x00; //Success
            memcpy(&packet->mData[1], arrOutdata, sizeof(arrOutdata));
        }
        else{}
    }
    else{}
#endif
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1217;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void FL_Git_ASK_v2_Req( stCommPkt *pkt, uint32_t eInCommType  )
{
    stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t		*pPayloadPtcl = pkt->mData;
    U32 uiReturnCode = 0;
    uint8_t arrSeed[8] = {0x00, };
    uint8_t arrOutdata[8] = {0x00, };
    uint32_t uiASK_Type = 0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

    packet->mLen = 2;
    packet->mData[1] = 0x01; //Fail

    uiASK_Type = pPayloadPtcl[0];
    packet->mData[0] = uiASK_Type;

    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        uint8_t arrCode[16] = {0x00, };
        uint8_t arrIv[16] = {0x47, 0x69, 0x74, 0x56, 0x43, 0x49, 0x33, 0x48, 0x53, 0x4D, 0x41, 0x45, 0x53, 0x5F, 0x49, 0x56};

        memcpy(arrCode, &pPayloadPtcl[1], sizeof(arrCode));
        memcpy(arrSeed, &pPayloadPtcl[17], sizeof(arrSeed));

        uiReturnCode = ActivationHSM();

        if(uiReturnCode == HSM_SUCCESS)
        {
            if(uiASK_Type == ASK_DIAG_PV)
            {
                uiReturnCode = ASKSignHSM_AID90(arrSeed, arrCode, arrIv, arrOutdata);
            }
            else if(uiASK_Type == ASK_REWORK_PV)
            {
                uiReturnCode = ASKSignHSM_AID92(arrSeed, arrCode, arrIv, arrOutdata);
            }
            else if(uiASK_Type == ASK_DIAG_CV)
            {
                uiReturnCode = ASKSignHSM_AID93(arrSeed, arrCode, arrIv, arrOutdata);
            }
            else if(uiASK_Type == ASK_REWORK_CV)
            {
                uiReturnCode = ASKSignHSM_AID94(arrSeed, arrCode, arrIv, arrOutdata);
            }
            else
            {
                GLogE("\r\nWrong ASK Type(%d)", uiASK_Type);
            }
        }
    }
    else
    {
        // Input format: ASK_Type(1) + ECU_Code(16) + Seed(8) + IV(16) = 41 bytes
        uint8_t arrCode[16] = {0x00, };
        uint8_t arrIv[16] = {0x00, };
        uint8_t master_id = 0;

        memcpy(arrCode, &pPayloadPtcl[1], 16);
        memcpy(arrSeed, &pPayloadPtcl[17], 8);
        memcpy(arrIv, &pPayloadPtcl[25], 16);

        // Map ASK Type to MasterId
        if (uiASK_Type == ASK_DIAG_PV)
        {
            master_id = 0;
        }
        //else if (uiASK_Type == ASK_REWORK_PV)
        else if (uiASK_Type == ASK_DIAG_CV)
        {
            master_id = 1;
        }
        //else if (uiASK_Type == ASK_DIAG_CV)
        else if (uiASK_Type == ASK_REWORK_PV || uiASK_Type == ASK_REWORK_CV)
        {
            master_id = 2; //REWORK_PV, REWORK_CV merged
        }
        else
        {
            GLogE("\r\nWrong ASK Type(%d)", uiASK_Type);
            master_id = 0xFF;  // Invalid
        }

        if (master_id != 0xFF)
        {
            HAL_StatusTypeDef hal_status = hsm_generate_ask_key(arrSeed, arrCode, master_id, NULL, arrOutdata);
            uiReturnCode = (hal_status == HAL_OK) ? HSM_SUCCESS : HSM_UNKNOWN_ERROR;
        }
        else
        {
            uiReturnCode = HSM_UNKNOWN_ERROR;
        }
    }

    if(uiReturnCode == HSM_SUCCESS)
    {
        packet->mLen = 10;
        packet->mData[1] = 0x00; //Success
        memcpy(&packet->mData[2], arrOutdata, sizeof(arrOutdata));
    }
    else
    {
        GLogE("\r\nASK Fail(%d)", uiReturnCode);
    }

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1250;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_Git_CRL_GetDate( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t *pPayloadPtcl = pkt->mData;
	UINT iLength=0;
    U32 uiReturnCode = 0;
    uint8_t CRL[1000] = {0x00, };

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        iLength=506;
        ActivationHSM();
        uiReturnCode = ReadDataHSM(&CRL[0], iLength, pPayloadPtcl[0]);
    }
    else
    {
        U32 read_len = 0;

        if (GetCrlEmmc(pPayloadPtcl[0], &CRL[0], &read_len) == TRUE)
        {
            uiReturnCode = HSM_SUCCESS;
        }
        else
        {
            uiReturnCode = HSM_READ_FAIL;
        }
    }
    
    if(uiReturnCode == HSM_SUCCESS)
    {
        packet->mData[0] = 0x00; //OK
        packet->mData[1] = pPayloadPtcl[0]; // CRL Number
        memcpy(&packet->mData[2], &CRL[89], 6); // Effective Data & Expiration Date
        packet->mLen	= 8;
    }
    else
    {
        packet->mData[0] = 0x01;
        packet->mLen	= 1;
    }

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1218;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
void FL_Git_CRL_GetData( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	UINT iLength=0;
    U32 uiReturnCode = 0;
    U8 CRL[1000] = {0x00, };

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        iLength=506;
        ActivationHSM();
        uiReturnCode = ReadDataHSM(&CRL[0], iLength, pPayloadPtcl[0]);
    }
    else
    {
        U32 read_len = 0;

        if (GetCrlEmmc(pPayloadPtcl[0], &CRL[0], &read_len) == TRUE)
        {
            uiReturnCode = HSM_SUCCESS;
        }
        else
        {
            uiReturnCode = HSM_READ_FAIL;
        }
    }

    if(uiReturnCode == HSM_SUCCESS)
    {
        packet->mData[0] = 0x00; //OK
        packet->mData[1] = pPayloadPtcl[0]; // CRL Number
        memcpy(&packet->mData[2], &CRL[0], sizeof(CRL));
        packet->mLen	= sizeof(CRL)+2;
    }
    else
    {
        packet->mData[0] = 0x01;
        packet->mLen	= 1;
    }

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1221;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#endif

#ifdef VCI3_DIAG
void FL_Git_CRL_Restore( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t *pPayloadPtcl = pkt->mData;
	U16 usCrlSize=0;
	uint8_t ucCrlNo=0;
    U32 uiReturnCode = 0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    ucCrlNo = pPayloadPtcl[0];
    memcpy( &usCrlSize, &pPayloadPtcl[1], sizeof(usCrlSize) );

    if( (usCrlSize > 1000) || (ucCrlNo > 30) || (ucCrlNo == 0))
    {
        packet->mData[0] = 0x01;//fail
		GLogE( "CRL Payload ERROR!! size(%d), CRLNumber(%d)\r\n", usCrlSize, ucCrlNo );
    }
    else
    {
        if (g_HSM_Type == HSM_TYPE_OLD)
        {
            //ActivationHSM(); //Not Required before WriteDataHSM
            uiReturnCode = WriteDataHSM(&pPayloadPtcl[3], usCrlSize, ucCrlNo);
        }
        else
        {
            if (StoreCrlEmmc(ucCrlNo, &pPayloadPtcl[3], usCrlSize) == FR_OK)
            {
                uiReturnCode = HSM_SUCCESS;
            }
            else
            {
                uiReturnCode = HSM_WRITE_FAIL;
            }
        }

        if(uiReturnCode == HSM_SUCCESS)
        {
            packet->mData[0] = 0x00; //OK
            GLogI( "StoreCrl ok!!\r\n" );
        }
        else
        {
            packet->mData[0] = 0x01;//fail
            GLogI( "StoreCrl fail!! (%d)\r\n", uiReturnCode);
        }
    }

	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1219;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_Git_CRT_GetHolderRef( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t *pPayloadPtcl = pkt->mData;
	U32 		uiReturnCode = 200;
	U32 		uiOutDataLen = 0;
	uint8_t			arrOutData[1000];

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 20;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x121A;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	if (g_HSM_Type == HSM_TYPE_OLD)
	{
		uiReturnCode = ActivationHSM();

		if(uiReturnCode == HSM_SUCCESS)
		{
			uiReturnCode = ReadCertificateHSM(arrOutData, (int*)&uiOutDataLen, pPayloadPtcl[0]);
		}
	}
	else
	{
		U8 cert_buffer[616];
		U16 cert_length = 0;

		if (hsm_read_certificate(140+pPayloadPtcl[0], HSM_ALG_RSA, cert_buffer, &cert_length) == HAL_OK)
		{
			// Copy 600 bytes for compatibility
			memcpy(arrOutData, cert_buffer, 600);
			uiOutDataLen = 600;
			uiReturnCode = HSM_SUCCESS;
		}
		else
		{
			uiReturnCode = HSM_READ_FAIL;
		}
	}

	if(uiReturnCode == HSM_SUCCESS)
	{
		packet->mData[0] = 0x00;	//0x00:ok, 0x01:fail
	}
	packet->mData[1] = pPayloadPtcl[0];	//cert no. (0~3)
	memcpy( &packet->mData[2],&arrOutData[44], 	HSM_HOLDER_REF_LEN );

	GLogN( "HOLER REF :" );
	for(uint8_t i=0; i<HSM_HOLDER_REF_LEN; i++)
	{
		GLogN( " %02X", packet->mData[2+i] );
	}
	GLogN( "\r\n");

	memcpy( &packet->mData[18],&uiReturnCode,  	sizeof(uiReturnCode) );
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

void FL_Git_CRT_GetData( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t 	*pPayloadPtcl = pkt->mData;
	U32 		uiReturnCode = 200;
	U8			arrOutData[2048];
	U16 cert_length = 0;
	U16 mapped_cert_id = 0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= sizeof(arrOutData)+2;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1222;

	memcpy(&mapped_cert_id, &pPayloadPtcl[0], sizeof(mapped_cert_id));
	if(mapped_cert_id<8)// HY/KM, PV/CV STD SecureAccess, HSM slot rainge: 141~147
	{
		mapped_cert_id = (140 + mapped_cert_id);
	}
	else	//CV KD ECU, HSM slot rainge: 361~380, Call the slot number directly from the app
	{}
	
	if (g_HSM_Type == HSM_TYPE_OLD)
	{
		// Old HSM: Not supported
		uiReturnCode = HSM_NOT_SUPPORTED;
	}
	else
	{

		if (hsm_read_certificate(mapped_cert_id, HSM_ALG_RSA, arrOutData, &cert_length) == HAL_OK)
		{
			uiReturnCode = HSM_SUCCESS;
		}
		else
		{
			uiReturnCode = HSM_READ_FAIL;
		}
	}

	if(uiReturnCode == HSM_SUCCESS)
	{
		packet->mData[0] = 0x00;	//0x00:ok, 0x01:fail
	
		memcpy( &packet->mData[1],&mapped_cert_id, sizeof(mapped_cert_id) );//cert no
		memcpy( &packet->mData[3],&arrOutData[0], cert_length );
		packet->mLen	= cert_length+3;
	}
	else
	{
		packet->mData[0] = 0x01;	//0x00:ok, 0x01:fail
		packet->mLen	= 1;
	}
#if 0
	GLogN( "cert :" );
	for(U8 i=0; i<sizeof(arrOutData); i++)
	{
		GLogN( " %02X", packet->mData[2+i] );
	}
	GLogN( "\r\n");
#endif
	
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#ifdef VCI3_DIAG
void FL_Git_ECUCODE_KEY_Req( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;

    U32 uiReturnCode = HSM_UNKNOWN_ERROR;
    U8 ucChecksum = 0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        uiReturnCode = ActivationHSM();
        uiReturnCode = ReadAES128KeyChecksumHSM(AES_INDEX_6, &ucChecksum);

        if((ucChecksum==0x7B) && (uiReturnCode==HSM_SUCCESS))
            packet->mData[0] = 0; // OK
        else
            packet->mData[0] = 1; // Fail
    }
    else
    {
        // Actual checksum verification is done during key storage (0x0115)
        packet->mData[0] = 0; // Always OK
#if defined(NEW_HSM_LOG_ENABLE)
        GLogN("ECU Code Key check - New HSM always returns OK\r\n");
#endif
    }

	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x121B;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_Git_HSM_GetVersion( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    U16 HSM_Current_Version = 0;
	HAL_StatusTypeDef status = HAL_ERROR;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	packet->mLen	= 3;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x121C;
    packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

    packet->mData[0] = 1; //Fail

    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        if( Read_HSM_Status() == 0 )
        {
            if( HSM_Version_Check((int*)&HSM_Current_Version) == HSM_SUCCESS )
            {
                packet->mData[0] = 0; //Success
            }
        }

        memcpy(&packet->mData[1], &HSM_Current_Version, sizeof(HSM_Current_Version));
        GLogN("HSM_Current_Version:%d\r\n",HSM_Current_Version);
    }
    else
    {
        HSM_VersionInfo_t version_info;

        for (int i = 0; i <= HSM_GET_VER_MAX_RETRY; i++)
        {
			status = hsm_get_version_info(&version_info);
			if (status == HAL_OK)
	        {
	            packet->mData[0] = 0; //Success

	            GLogN("HSM Version - Host: %d.%d.%d, HSE: %d.%d.%d\r\n",
	                  version_info.host_major, version_info.host_minor, version_info.host_patch,
	                  version_info.hse_major, version_info.hse_minor, version_info.hse_patch);

				HSM_Current_Version = version_info.release;
				break;
			}
				
    	}
#ifdef FW_TEST_VERSION_OVERRIDE
		HSM_Current_Version = Load_HSM_TestVersion();
#else
		//HSM_Current_Version=1001;//for test
#endif
		GLogN("HSM_Current_Version:%d\r\n",HSM_Current_Version);
		// ex) ver: 9995
		packet->mData[1] = (U8)(HSM_Current_Version % 100); //95
		packet->mData[2] = (U8)(HSM_Current_Version / 100); //99
		//packet->mData[1] = 0x10;
		//packet->mData[2] = 0x01;
        //memcpy(&packet->mData[1], &HSM_Current_Version, sizeof(HSM_Current_Version));
    }
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
#endif

#ifdef VCI3_DIAG
void FL_Git_HSM_UpdateStart( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t *pPayloadPtcl = pkt->mData;
    U32 uiChecksum = 0;
    bool bCSchk = 0;
    U32 uisize = 0;
    int32_t ret = 0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
	if (g_HSM_Type == HSM_TYPE_OLD)
    {
		uisize = pPayloadPtcl[0]|(pPayloadPtcl[1]<<8);
	    
	    uiChecksum = 0;
	    
	    for(int i=0; i<uisize; i++)
	    {
	        uiChecksum += pPayloadPtcl[2+i];
	    }
    
	    if(uiChecksum != 0x13BC)
	    {
	        packet->mData[0] = 0x01; //fail
	    }
	    else
	    {
	        packet->mData[0] = 0x00; //success
	        bCSchk = true;
	    }
    }
	else
	{
		packet->mData[0] = 0x00; //success
	    bCSchk = true;
	}

	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x121D;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	/* Cache UUID so subsequent async 0x121E HSM_Update_Ack frames (sent from
	 * hsm_firmware_upgrade / git_hsm_api.c polling loops, with no original pkt
	 * in scope) can carry the same UUID — required for MQTT/WebSocket routing. */
	memcpy(&g_HSM_UpdateAck_UUID, &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
    
    if( bCSchk )
    {
        if (g_HSM_Type == HSM_TYPE_OLD)
        {
            //g_HSM_Timer = SetSWTimer( 3000, eSWTimer_INFINITE, HSM_Update_Ack, true );
            ret = VCI3_HSM_UPDATE(&pPayloadPtcl[2]);//tara critical function delete
            if(ret != HSM_SUCCESS)
            {
                Save_HSM_UpdateFailCount();
            }
            else
            {
                f_unlink(HSM_UPDATE_FAIL_INFO);
            }
        }
        else
        {
            HSM_VersionInfo_t version_info;
			U16 HSM_Current_Version = 0;
	        if (hsm_get_version_info(&version_info) == HAL_OK)
	        {
	            GLogN("HSM Version - Host: %d.%d.%d, HSE: %d.%d.%d\r\n",
	                  version_info.host_major, version_info.host_minor, version_info.host_patch,
	                  version_info.hse_major, version_info.hse_minor, version_info.hse_patch);
				HSM_Current_Version = version_info.release;
				
	        }
			else HSM_Current_Version = 0;
	        GLogN("HSM_Current_Version:%d\r\n",HSM_Current_Version);
			U8 ucDfuMode=HSM_DFU_TYPE_APP_FW;
#if 0	//HSE update is not needed yet.
			if((version_info.hse_major<2)//newest version 2.40.0
            		||((version_info.hse_major<=2)&&(version_info.hse_minor<40))
            		)
        	{
            	ucDfuMode = HSM_DFU_TYPE_HSE_FW;
        	}
			else
			{
				GLogN("HSE already newest!! \r\n");
				ucDfuMode = HSM_DFU_TYPE_APP_FW;
			}
#endif
			if(HAL_OK != hsm_firmware_upgrade(ucDfuMode))
			{
				packet->mData[0] = 0x01; //fail
				Save_HSM_UpdateFailCount();
			}
			else
            {
            	packet->mData[0] = 0x00; //success
            	f_unlink(HSM_UPDATE_FAIL_INFO);
            }
        }
    }
}

void FL_Git_HSM_Applet_Update( stCommPkt *pkt, uint32_t eInCommType  )
{
    stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t *pPayloadPtcl = pkt->mData;
    U32 uiReturnCode = HSM_UNKNOWN_ERROR;
    
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    /***************************************************
    if pPayloadPtcl[0]==0 --> HSM Applet Update
    if pPayloadPtcl[0]==1 --> HSM Applet&ASK Update
    if pPayloadPtcl[1]==0 --> HSM ASK Normal
    if pPayloadPtcl[1]==1 --> HSM ASK ECU Upgrade
    pPayloadPtcl[2] ~ pPayloadPtcl[n] : Auth Key
    ***************************************************/
    if (g_HSM_Type == HSM_TYPE_OLD)
    {
#if 0 //It's a function that is no longer in use.
        uiReturnCode = HSM_ASK_Applet_Update( &pPayloadPtcl[2], pPayloadPtcl[0], pPayloadPtcl[1] );
#else
        // Function no longer in use even for old HSM
        uiReturnCode = HSM_UNKNOWN_ERROR;
        packet->mData[0] = 0x01;
#endif
    }
    else
    {
        // New HSM Applet Update NOT_SUPPORTED
        uiReturnCode = HSM_UNKNOWN_ERROR;
        GLogEE("New HSM Applet Update NOT_SUPPORTED\r\n");
    }
	if(uiReturnCode == HSM_SUCCESS) packet->mData[0] = 0x00;
    else packet->mData[0] = 0x01;

	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x121F;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
    
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_Git_HSM_State_Check( stCommPkt *pkt, uint32_t eInCommType  )
{
    stCommPkt	*packet;
	stMsgClst	*message;
    
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        U8 status = Read_HSM_Status();
        if (status == 0)
        {
            packet->mData[0] = 0;  // Success
        }
        else
        {
            packet->mData[0] = 2;  // Error
        }
    }
    else  // NEW HSM
    {
        uint8_t serial_number[8];

        //if (hsm_get_serial_number(serial_number) == HAL_OK)
        U8 status = Read_HSM_Status();
        if (status == 0)
        {
            packet->mData[0] = 1;  // Success NEW HSM
        }
        else
        {
            packet->mData[0] = 2;  // Error
        }
    }

	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1220;
    packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

#endif


#ifdef VCI3_DIAG
#if ENCRYPT_PRJ
void FL_Git_GenRsaKeyPair( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
#if 1
    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        if( Read_HSM_Status() == 0 )
        {
            if(ActivationHSM() == HSM_SUCCESS)
            {
                DeleteRSAKeypairHSM();

                //GLogI( "\r\nDelete RSA Key Success");
                if(GenerateRSAKeypairHSM() == HSM_SUCCESS)
                {
                    //GLogI( "\r\nGenerate RSA Key Success");
                    packet->mLen = 1;
                    packet->mData[0] = 0x00;//ok

                }
                else
                {
                    GLogEE( "\r\nGenerate RSA Key Fail!!!");
                    packet->mLen = 1;
                    packet->mData[0] = 0x01;//fail //Generate RSA Key Fail
                }
            }
            else
            {
                GLogEE( "\r\nActivation Fail!!!");
                packet->mLen = 1;
                packet->mData[0] = 0x02;//fail //Activation Fail
            }
        }
        else
        {
            GLogEE( "\r\nRead_HSM_Status Fail!!!");
            packet->mLen = 1;
            packet->mData[0] = 0x02;//fail
        }
    }
    else
    {
        // New HSM: Generate RSA key pair using OP Code Key Mng
        // Key ID #150 RSA key pair
        //if (hsm_generate_key_pair(150, HSM_ALG_RSA, 0, false) == HAL_OK)
        {//not generate, use #149 keypair
#if defined(NEW_HSM_LOG_ENABLE)
            GLogI("\r\nNew HSM RSA Key Generation Success (Key ID: %d)\r\n", HSM_KEK_RSA_KEY_ID);
#endif
            packet->mLen = 1;
            packet->mData[0] = 0x00;//ok
        }
#if 0
        else
        {
            GLogEE("\r\nNew HSM RSA Key Generation Fail (Key ID: %d)\r\n", HSM_KEK_RSA_KEY_ID);
            packet->mLen = 1;
            packet->mData[0] = 0x01;//fail
        }
#endif
    }
#if 0
	if((packet->mData[0]!=0)&&(g_ucHSM_ErrorCnt<3))
	{
		g_ucHSM_ErrorCnt++;
	}
	else if(g_ucHSM_ErrorCnt>2)
	{
		Save_HSM_Error_Status(0xB);
	}
#endif
    
#else//for test
	packet->mLen = 1;
	packet->mData[0] = 0x00;//ok
#endif

	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x123B;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
    
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_Git_GetPublicKey( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	U8			arrTempData[260];  // Increased for RSA-2048 (258 bytes)
	U32 		*uiKeyLen = 0;
	uint16_t	key_length = 0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
#if 1
    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        if(Read_HSM_Status() == 0)
        {
            if(ActivationHSM() == HSM_SUCCESS)
            {
                if( ReadPublicKeyHSM2(arrTempData, uiKeyLen) == HSM_SUCCESS)
                {
                    //GLogI( "\r\nRead Public Key Success");
                    packet->mLen = (*uiKeyLen)+3;
                    packet->mData[0] = 0x00;//ok
                    packet->mData[1] = (*uiKeyLen)&0x000000FF;
                    packet->mData[2] = ((*uiKeyLen)>>8)&0x000000FF;
                    memcpy(&packet->mData[3], arrTempData, *uiKeyLen);
        #if 0
                    GLogI( "\r\nsend rsa pub key : ");
                    for(U8 i=0; i<0x83; i++)
                    {
                        GLogI( "%02X ",arrTempData[i]);
                    }
        #endif
                }
                else
                {
                    GLogEE( "\r\nRead Pub Key Fail!!!");
                    packet->mLen = 1;
                    packet->mData[0] = 0x02;//fail //Read Pub Key Fail
                }
            }
            else
            {
                GLogEE( "\r\nActivation Fail!!!");
                packet->mLen = 1;
                packet->mData[0] = 0x01;//fail //Activation Fail
            }
        }
        else
        {
            GLogEE( "\r\nRead_HSM_Status Fail2!!!");
            packet->mLen = 1;
            packet->mData[0] = 0x01;//fail
        }
    }
    else
    {
        // New HSM: Export public key using OP Code Key Mng
        // Key ID #149 for KEK RSA key pair
#if 1
        {
        	static const U8 KekPublicKey[259] = {
				// Modulus (256 bytes)
				0xba, 0xb0, 0x07, 0x17, 0x31, 0xe0, 0x5b, 0x2e, 0x09, 0x55, 0x36, 0x3c, 0x29, 0x25, 0x49, 0x2b,
				0x50, 0xf3, 0x31, 0x15, 0x18, 0x20, 0x05, 0x3f, 0xb3, 0xe5, 0x14, 0x93, 0x57, 0x6a, 0x56, 0x0c,
				0x65, 0xcb, 0x10, 0x3a, 0xb3, 0x72, 0xad, 0xc7, 0xc9, 0x3f, 0xe2, 0x41, 0xba, 0xc2, 0x50, 0x5e,
				0xec, 0x7b, 0x10, 0xf0, 0xbc, 0xe7, 0x81, 0x94, 0x13, 0x52, 0x7d, 0x72, 0x04, 0x38, 0xb7, 0x0f,
				0xf0, 0xb9, 0xd8, 0xe3, 0x8d, 0xf1, 0x97, 0xd5, 0xc8, 0x99, 0x87, 0xad, 0xdd, 0xaf, 0x25, 0x72,
				0x49, 0x35, 0x9a, 0xb6, 0xe6, 0x23, 0x1e, 0x99, 0xf0, 0xf6, 0x04, 0xad, 0x8a, 0xa6, 0x63, 0x5e,
				0xac, 0x0f, 0x3d, 0xd7, 0xee, 0xc2, 0xc2, 0xf8, 0x22, 0x67, 0x8c, 0xc8, 0xbd, 0xc7, 0x61, 0x10,
				0x7c, 0x6e, 0x44, 0xf7, 0x04, 0xb9, 0xdf, 0xd6, 0xc8, 0xcb, 0x66, 0xfc, 0xca, 0xbf, 0x68, 0x8f,
				0x3c, 0x3e, 0xba, 0x36, 0x11, 0xe5, 0x6a, 0x17, 0xe2, 0x95, 0xb8, 0xa9, 0xeb, 0xd5, 0x8f, 0xad,
				0x5e, 0xd6, 0x08, 0x71, 0x3f, 0x7e, 0x49, 0x93, 0x5d, 0xb0, 0xb9, 0xaf, 0x4e, 0xf0, 0x51, 0x6a,
				0x3e, 0xf8, 0xc7, 0x64, 0x98, 0x85, 0x68, 0x26, 0xfc, 0x5f, 0x14, 0x38, 0x51, 0xe8, 0x44, 0xb4,
				0x65, 0x37, 0xca, 0xc2, 0x8e, 0x1f, 0x92, 0x9e, 0xaf, 0xba, 0xe6, 0xaa, 0x8f, 0x0f, 0xf1, 0xeb,
				0x99, 0xb3, 0x56, 0xb7, 0x4d, 0x03, 0xba, 0xbb, 0xec, 0xa3, 0xd2, 0x5e, 0x5d, 0x82, 0xb3, 0xdd,
				0x03, 0x7f, 0xa7, 0x9e, 0xf1, 0x3e, 0xf1, 0x25, 0x70, 0xcd, 0x00, 0x22, 0xcb, 0x00, 0x46, 0x0c,
				0xb2, 0x96, 0xee, 0x21, 0x1a, 0xf5, 0xea, 0xf2, 0x03, 0xcb, 0x1b, 0x59, 0x0c, 0x87, 0x35, 0x2a,
				0xb6, 0x14, 0x44, 0x74, 0xfc, 0xdd, 0x3c, 0x82, 0xd4, 0x6b, 0x72, 0x4d, 0x72, 0xf8, 0xec, 0xb1,
				// Public Exponent (3 bytes) = 65537
				0x01, 0x00, 0x01
			};
            //GLogI("\r\nNew HSM Export Public Key Success (Key ID: %d, Len: %d)\r\n", HSM_KEK_RSA_KEY_ID, key_length);
#if defined(NEW_HSM_LOG_ENABLE)
            GLogI("\r\nNew HSM Public Key Send (KEK)\r\n");
#endif
            uint16_t output_len = 259;  // 256(key) + 3(exponent)
            packet->mLen = output_len + 3;
            packet->mData[0] = 0x00;//ok
            packet->mData[1] = output_len & 0xFF;
            packet->mData[2] = (output_len >> 8) & 0xFF;
            //memcpy(&packet->mData[3], &arrTempData[2], 256);
            //memcpy(&packet->mData[3 + 256], &arrTempData[261], 3);
            memcpy(&packet->mData[3], &KekPublicKey[0], 256);
            memcpy(&packet->mData[3 + 256], &KekPublicKey[256], 3);
        }
#else
        if (hsm_export_public_key(150, HSM_ALG_RSA, arrTempData, &key_length) == HAL_OK)
        {
            GLogI("\r\nNew HSM Export Public Key Success (Key ID: %d, Len: %d)\r\n", HSM_KEK_RSA_KEY_ID, key_length);
            uint16_t output_len = 259;  // 256(key) + 3(exponent)
            packet->mLen = output_len + 3;
            packet->mData[0] = 0x00;//ok
            packet->mData[1] = output_len & 0xFF;
            packet->mData[2] = (output_len >> 8) & 0xFF;
            memcpy(&packet->mData[3], &arrTempData[2], 256);
            memcpy(&packet->mData[3 + 256], &arrTempData[261], 3);
        }
        else
        {
            GLogEE("\r\nNew HSM Export Public Key Fail (Key ID: %d)\r\n", HSM_KEK_RSA_KEY_ID);
            packet->mLen = 1;
            packet->mData[0] = 0x02;//fail
        }
#endif
    }

#if 0
	if((packet->mData[0]!=0)&&(g_ucHSM_ErrorCnt<3))
	{
		g_ucHSM_ErrorCnt++;
	}
	else if(g_ucHSM_ErrorCnt>2)
	{
		Save_HSM_Error_Status(0x9);
	}
#endif

#else//for test
	packet->mLen = 1;
	packet->mData[0] = 0x00;//ok
#endif
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1239;
    packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
	
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
    
}

void FL_Git_StoreEncryptKey( stCommPkt *pkt, uint32_t eInCommType  )
{
	stCommPkt	*packet;
	stMsgClst	*message;
	uint8_t	*pPayloadPtcl = pkt->mData;
	uint8_t EncryptedData[260],DecryptedData[260],i;  // Increased for RSA-2048
	U32 uiKeySize=0;
	U32 uiEncryptKeySize=0;
	uint16_t plaintext_length = 0;

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	//pPayloadPtcl[0~3] = key size ( Ű )
	//pPayloadPtcl[4~7] = encrypt key size (ȣȭ  Ű )
	//pPayloadPtcl[8~n] = encrypt key
#if 0
	for(i=0; i<8; i++)
	{
		GLogN( "\r\npPayloadPtcl[%d]: %02X", i,pPayloadPtcl[i]);
	}
#endif
	memcpy(&uiKeySize, &pPayloadPtcl[0], sizeof(uiKeySize));
	memcpy(&uiEncryptKeySize, &pPayloadPtcl[4], sizeof(uiEncryptKeySize));
#if 0

	GLogN( "\r\n" );
	GLogN( "uiKeySize: %d",uiKeySize);
	GLogN( "\r\n" );
	GLogN( "Encrypt AES256_Key: " );
	for(i=0; i<pPayloadPtcl[4]; i++)
	{
		GLogN( "%02X ", pPayloadPtcl[8+i]);
	}
#endif
	//********rsa decrypt in hsm()*********//
    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        memcpy(EncryptedData, &pPayloadPtcl[8], 0x80);  // Old HSM: 128 bytes
        if( DecryptPrivateKeyHSM(EncryptedData, DecryptedData) == HSM_SUCCESS)
        {
            //GLogI( "\r\nDecrypt Key Success");
            packet->mData[0] = 0x00;//ok
            g_bEncryptFlag=ON;
        }
        else
        {
            GLogEE( "\r\nDecrypt Key Fail!!");
            packet->mData[0] = 0x01;//fail
        }
    }
    else
    {
        // New HSM: RSA decryption using OP Code Rsaes
        // Key ID #149 key pair
        // RSA-2048: 256 bytes ciphertext
        memcpy(EncryptedData, &pPayloadPtcl[8], uiEncryptKeySize);

        if (hsm_rsa_decrypt(149, EncryptedData, DecryptedData, &plaintext_length) == HAL_OK)
        {
#if defined(NEW_HSM_LOG_ENABLE)        
            GLogI("\r\nNew HSM RSA Decrypt Success (Key ID: %d, Plaintext Len: %d)\r\n", 149, plaintext_length);
#endif
            packet->mData[0] = 0x00;//ok
            g_bEncryptFlag=ON;
        }
        else
        {
            GLogEE("\r\nNew HSM RSA Decrypt Fail (Key ID: %d)\r\n", 149);
            packet->mData[0] = 0x01;//fail
        }
    }
#if 0
	if((packet->mData[0]!=0)&&(g_ucHSM_ErrorCnt<3))
	{
		g_ucHSM_ErrorCnt++;
	}
	else if(g_ucHSM_ErrorCnt>2)
	{
		Save_HSM_Error_Status(0xA);
	}
#endif

	//********store output key*********//
#if ENCRYPT_PRJ_LOG
	GLogN( "\r\n" );
	GLogN( "DecryptedData: " );
	for(i=0; i<0x80; i++)
	{
		GLogI( "%02X ",DecryptedData[i]);
	}
#endif
    if (g_HSM_Type == HSM_TYPE_OLD)
    {
        // Old HSM: RSA padding present, key at end of buffer
        memcpy(g_ucAES256_Key, &DecryptedData[128-uiKeySize], AES_KEY_SIZE);
    }
    else
    {
        // New HSM: RSAES-PKCS1-v1_5 removes padding automatically
        // plaintext_length contains actual key size
        memcpy(g_ucAES256_Key, &DecryptedData[plaintext_length - uiKeySize], AES_KEY_SIZE);
    }
#if ENCRYPT_PRJ_LOG
		GLogN( "\r\n" );
		GLogN( "AES256_Key: " );
		for(i=0; i<AES_KEY_SIZE; i++)
		{
			GLogI( "%02X ",g_ucAES256_Key[i]);
		}
#endif


	packet->mLen	= 1;
	
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x123A;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));
    
	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
    
}
#endif
#endif
void FL_GitWifiScanInfo( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    
    int32_t status = 0;
    uint8_t i, j;
    rsi_rsp_scan_t scan_result;
    
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
	if(g_mqtt_isconnected == 1)
	{
		status = disconnectToMQTT();
		if ( status != RSI_SUCCESS )
		{
			GLogN("MQTT Disconnect Fail(%d)\n",status);
		}
	}
	connState = CONNECTION_NOT_CONNECT;
	if(g_ucApConnected == 1)
	{
		status = DisconnectWLan( );
		if ( status != RSI_SUCCESS )
		{
			GLogN("DisconnectWLan Fail(%d)\n",status);
		}
	}
	
    status = rsi_wlan_scan(NULL, 0, &scan_result, sizeof(scan_result));

    if(status == 0)
    {
        for(i = 0; i < 11; i++)
        {
            packet->mData[(i*48)+0] = scan_result.scan_info[i].rf_channel;
            packet->mData[(i*48)+1] = scan_result.scan_info[i].security_mode;
            packet->mData[(i*48)+2] = scan_result.scan_info[i].rssi_val;
            packet->mData[(i*48)+3] = scan_result.scan_info[i].network_type;
            memcpy(&packet->mData[(i*48)+4], scan_result.scan_info[i].ssid, sizeof(scan_result.scan_info[i].ssid)-2);
            for(j = 0; j < 6; j++) {
                sprintf(&packet->mData[(i*48)+36+(j*2)], "%02x", scan_result.scan_info[i].bssid[j]);
            }
        }
		
		packet->mLen	= 528;
    }
    else
	{
		packet->mData[0] 	= 0x01;
		packet->mLen		= 0x01;
	}
	
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1701;
    packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitWifiSetInfo( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    
    uint8_t		*pPayloadPtcl = pkt->mData;
    WifiSetInfo AP_set;
    int32_t		status = 0;
    uint8_t		i;
    
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	if(g_mqtt_isconnected == 1)
	{
		status = disconnectToMQTT();
		if ( status != RSI_SUCCESS )
		{
			GLogN("MQTT Disconnect Fail(%d)\r\n",status);
		}
	}
	connState = CONNECTION_NOT_CONNECT;
	if(g_ucApConnected == 1)
	{
		status = DisconnectWLan();
		if ( status != RSI_SUCCESS )
		{
			GLogN("DisconnectWLan Fail(%d)\r\n",status);
		}
	}
    
	AP_set.SSID_Length = pPayloadPtcl[0];
	AP_set.PSK_Length = pPayloadPtcl[33];
    memcpy(AP_set.SSID, &pPayloadPtcl[1], AP_set.SSID_Length+1);
    memcpy(AP_set.PSK_Key, &pPayloadPtcl[34], AP_set.PSK_Length+1);
    AP_set.Security_Mode = pPayloadPtcl[97];
    GLogN("ssid : %s\r\n", AP_set.SSID);
    GLogN("PSK : %s\r\n", AP_set.PSK_Key);
    GLogN("Security Mode : %d\r\n", AP_set.Security_Mode);
	
	memset(msServerConnectInfo.mqtt_domain, 0x00, sizeof(msServerConnectInfo.mqtt_domain));
	memcpy(msServerConnectInfo.mqtt_domain, &pPayloadPtcl[98], 60);
	memcpy(&msServerConnectInfo.mqtt_port, &pPayloadPtcl[158], sizeof(uint16_t));
	msServerConnectInfo.mqtt_domain[126] = 0;
	Save_ServerInfo_EMMC();
	GLogN("MQTT Domain : %s\r\n", msServerConnectInfo.mqtt_domain);
	GLogN("MQTT Port : %d\r\n", msServerConnectInfo.mqtt_port);
	
    // Connect AP
    for (i = 0; i < RS9116_RETRY_COUNT; i++)
	{
        status = ConnectAP(AP_set.SSID, AP_set.Security_Mode, AP_set.PSK_Key );

        if (status == 0)
		{
		  	gsFwInfo.mucChanged = TRUE;
			memset(&gsFwInfo.msWifiConnectInfo.SSIDname[0],0x00,sizeof(gsFwInfo.msWifiConnectInfo.SSIDname));
			memset(&gsFwInfo.msWifiConnectInfo.PSK_Key[0],0x00,sizeof(gsFwInfo.msWifiConnectInfo.PSK_Key));
			memcpy(&gsFwInfo.msWifiConnectInfo.SSIDname[0], &AP_set.SSID[0], AP_set.SSID_Length );
			memcpy(&gsFwInfo.msWifiConnectInfo.PSK_Key[0], &AP_set.PSK_Key[0], AP_set.PSK_Length);
			gsFwInfo.msWifiConnectInfo.SSIDlength = AP_set.SSID_Length;
			gsFwInfo.msWifiConnectInfo.PSK_length = AP_set.PSK_Length;
			gsFwInfo.msWifiConnectInfo.mode       = AP_set.Security_Mode;
			gsFwInfo.msWifiConnectInfo.initialized = 1;
			saveFirmwareInfo_EMMC(true);
			
			// Get IP
			for (i = 0; i < RS9116_RETRY_COUNT; i++)
			{
				if (i != 0) GLogN("SetIPAddressStatic req %d \n\r", i);

				status = SetIPAddressDHCP();
				
				if (status == 0) break;  // success
				
				osDelay(10);
			}
			
			g_ucApConnected = 1;
			break;						// success
		}
        else                osDelay(10);
    }
    
    if(status == 0)
	{
		connState = CONNECTION_MQTT_CONNECT;
		packet->mData[0] = 0x00;    //ok
	}
    else
	{
		packet->mData[0] = 0x01;    //fail
	}
    
	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
	packet->mFuncID = 0x1702;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	= CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitWifiConnectInfo( uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    int index = 0;

    WifiConnectInfo Connect_Info;
	
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
    if (g_mqtt_isconnected == 1)
    {
        memcpy(&packet->mData[0], gsFwInfo.marrucSerialNo, 8);
        memcpy(&packet->mData[8], gsFwInfo.msWifiConnectInfo.SSIDname, gsFwInfo.msWifiConnectInfo.SSIDlength);
        packet->mData[40] = 2;
        for (int i = 0; i < 17; i++)
		{
			if ( g_strWifiMacAddress[i] != ':')
			{
				packet->mData[41 + index] =  g_strWifiMacAddress[i];
				index++;
			}
		}

		if(g_ucAUTOVIN[0] == 0x02)
		{
		  	memcpy(&packet->mData[53], &g_ucAUTOVIN[2], 17);
		}
		else
		{
			memcpy(&packet->mData[53], &g_ucAUTOVIN[4], 17);
		}
		
        packet->mLen	= 70;		//LJS need to check
        packet->pTarget	= &eInCommType;
    }
    else
    {
        packet->mData[0] = 0x01;
        packet->mLen	= 1;
        packet->pTarget	= &eInCommType;
    }
    
    if(g_first_connect == true)
    {
        packet->mFuncID = 0x1706;
        g_first_connect = false;
    }
    else packet->mFuncID = 0x1705;
    packet->mCS 	    = CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitWifiSetScaninfo( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t		*pPayloadPtcl = pkt->mData;
	uint32_t	usLength = pkt->mLen - 8;
    FIL			wifi_info;
    char		Filename[] = "Wifi_info.dat";
    int			nWriteLen;
	uint8_t		temp_file[490] = {0};
	
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

	memcpy(temp_file, pPayloadPtcl, usLength);
    f_mkdir("/wifi_info");
    f_chdir("/wifi_info");
    f_open(&wifi_info, Filename, FA_CREATE_ALWAYS | FA_WRITE);
    f_write(&wifi_info, (uint8_t*)pPayloadPtcl, usLength, &nWriteLen);
    f_close(&wifi_info);
    f_chdir(DIR_ROOT);		// restore cwd to root (don't leave it at /wifi_info)

    if(usLength == nWriteLen)   packet->mData[0] = 0;
    else                        packet->mData[0] = 1;
    
	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
    packet->mFuncID = 0x1703;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	    = CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitWifiGetScaninfo( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    FIL         wifi_info;
    char        Filename[] = "Wifi_info.dat";
    int         nReadLen = 0;
    
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    f_mkdir("/wifi_info");
    f_chdir("/wifi_info");

    // ���� ���� (�б� ���� ���� ����)
    if (f_open(&wifi_info, Filename, FA_READ) == FR_OK)
    {
        f_read(&wifi_info, &packet->mData[0], 490, &nReadLen);
        f_close(&wifi_info);
    }
    else
    {
        // ���� ���⿡ ������ ���?���� ó��
        printf("File open error!!!");
    }
    f_chdir(DIR_ROOT);		// restore cwd to root (don't leave it at /wifi_info)

	packet->mLen	= nReadLen;
	packet->pTarget	= &eInCommType;
    packet->mFuncID = 0x1704;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	    = CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitWifiClearScaninfo( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    FIL         wifi_info;
    char        Filename[] = "Wifi_info.dat";
    int         nWriteLen;
    uint8_t     clear_data[490] = {0};
    uint32_t    clear_length = 0;
    
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

    clear_length = sizeof(clear_data);
    memset(clear_data, 0 , clear_length);
    
    f_mkdir("/wifi_info");
    f_chdir("/wifi_info");
    f_open(&wifi_info, Filename, FA_CREATE_ALWAYS | FA_WRITE);
    f_write(&wifi_info, clear_data, clear_length, &nWriteLen);
    f_close(&wifi_info);
    f_chdir(DIR_ROOT);		// restore cwd to root (don't leave it at /wifi_info)

    if(clear_length == nWriteLen)   packet->mData[0] = 0;
    else                            packet->mData[0] = 1;
    
	packet->mLen	= 1;
	packet->pTarget	= &eInCommType;
    packet->mFuncID = 0x1707;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	    = CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitAskNextAction( uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
    
	packet->mLen	= 0;
	packet->pTarget	= &eInCommType;
    packet->mFuncID = 0x1708;
	packet->mCurFrame = 0;
	
	packet->mCS 	    = CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}
void KeyExchangeStart(void)
{
	stCommPkt	*packet;
	U32			uiTempDataLen = 0;
	uint8_t		arrTempData[128];
	U32 		*uiKeyLen = 0;
	
    if(ActivationHSM() == HSM_SUCCESS)
	{
		if( ReadPublicKeyHSM2(arrTempData, uiKeyLen) == HSM_SUCCESS)
	    {
	    	//GLogI( "\r\nRead Public Key Success");
	    	packet->mLen = (*uiKeyLen)+3;
			packet->mData[0] = 0x00;//ok
			packet->mData[1] = (*uiKeyLen)&0x000000FF;
			packet->mData[2] = ((*uiKeyLen)>>8)&0x000000FF;
			memcpy(&packet->mData[3], arrTempData, *uiKeyLen);
#if 0
			GLogI( "\r\nsend rsa pub key : ");
			for(uint8_t i=0; i<0x83; i++)
			{
				GLogI( "%02X ",arrTempData[i]);
			}
#endif
		}
	    else
	    {
	    	GLogEE( "\r\nRead Pub Key Fail!!!");
	    	packet->mLen = 1;
			packet->mData[0] = 0x02;//fail //Read Pub Key Fail
	    }
	}
	else
	{
		GLogEE( "\r\nActivation Fail!!!");
		packet->mLen = 1;
		packet->mData[0] = 0x01;//fail //Activation Fail
	}
	TransmitFunction(PACKET_MQTT, packet->mData, packet->mLen, 0x123C);
	
    return;
}

void FL_ListSensor_Disable( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
	if (g_OBD_Processing == true)
	{
		g_ListSensor_Endflag = false;
		osSignalWait(OBD_ListDiag_END, osWaitForever);
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
	
	packet->mLen	= 0;
	packet->pTarget	= &eInCommType;
    packet->mFuncID = 0x1710;
	packet->mCurFrame = 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	    = CalcChecksumGITPtclPayloadFrame(packet);

	message->mPktType	= (ePKT_TD)eInCommType;
	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_mqtt_to_websocket( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t		*pPayloadPtcl	= pkt->mData;
	uint32_t	usLength		= pkt->mLen - 8;
	uint8_t 	status			= 0;
	uint8_t		device_info[50] = {0};
	uint8_t		index			= 0;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}

#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message = ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet = ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}

    memcpy(msServerConnectInfo.websocket_domain, &pPayloadPtcl[0], sizeof(msServerConnectInfo.websocket_domain));
    memcpy(msServerConnectInfo.websocket_resource, &pPayloadPtcl[128], sizeof(msServerConnectInfo.websocket_resource));
    msServerConnectInfo.websocket_port = (pPayloadPtcl[256] << 8) + pPayloadPtcl[257];

    // 1. ���� ���� ����
    if (status == 0)
	{
		status = Save_ServerInfo_EMMC();
        if (status != 0)
		{
            GLogN("Error: Failed to save server info\r\n");
        }
    }

    // 2. MQTT ���� ����
    if (g_mqtt_isconnected)
	{
		connState = CONNECTION_NOT_CONNECT;
		status = disconnectToMQTT();
    }

    // 3. WebSocket ���� �õ�
    if (status == 0)
	{
        for (int i = 0; i < 3; i++)
		{
            status = connectToWebsocket();
            if (status == 0)
			{
                break; // ���� �� ���� ����
            }
        }
    }

	if(status == 0)
	{
		packet->mLen		= 1;
		packet->mData[0]	= 0;
		packet->pTarget		= (void *)PACKET_WEBSOCKET;
		message->mPktType	= PACKET_WEBSOCKET;
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
            status = connectToMQTT();
            if (status == 0)
			{
                GLogN("WebSocket connect OK!!\r\n");
                break; // ���� �� ���� ����
            }

			if(i == 3)
			{
				HAL_NVIC_SystemReset();
			}
        }

		packet->mLen		= 1;
		packet->mData[0]	= 1;
		packet->pTarget		= (void *)PACKET_MQTT;
		message->mPktType	= PACKET_MQTT;
	}

    packet->mFuncID		= 0x1801;
	packet->mCurFrame	= 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	    = CalcChecksumGITPtclPayloadFrame(packet);

	message->pPacket	= (void *)packet;

	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}

	if(g_websocket_isconnected == true)
	{
		memcpy(&device_info[0], gsFwInfo.marrucSerialNo, 8);
		for (int i = 0; i < 17; i++)
		{
			if ( g_strWifiMacAddress[i] != ':')
			{
				device_info[8 + index] =  g_strWifiMacAddress[i];
				index++;
			}
		}
		TransmitFunction(PACKET_WEBSOCKET, device_info, index + 8, 0x1803);
	}
}

void FL_websocket_to_mqtt( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t		*pPayloadPtcl	= pkt->mData;
	uint32_t	usLength		= pkt->mLen - 8;
	uint8_t 	status			= 0;

	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
	
    memcpy(msServerConnectInfo.mqtt_domain, &pPayloadPtcl[0], sizeof(msServerConnectInfo.mqtt_domain));
    msServerConnectInfo.mqtt_port = (pPayloadPtcl[128] << 8) + pPayloadPtcl[129];
	
    if (status == 0)
	{
		status = Save_ServerInfo_EMMC(); 
        if (status != 0)
		{
            GLogN("Error: Failed to save server info\r\n");
        }
    }
	
    if (g_websocket_isconnected)
	{
		connState = CONNECTION_NOT_CONNECT;
		status = disconnectToWebsocket();
    }
	
    if (status == 0)
	{
		for (int i = 0; i < 3; i++)
		{
			status = connectToMQTT();
			if (status == 0)
			{
				GLogN("mqtt connect OK!!\r\n");
				break;
			}
			
			if(i == 3)
			{
				HAL_NVIC_SystemReset();
			}
		}
    }
	
	packet->mLen		= 1;
	packet->mData[0]	= 0;
	packet->pTarget		= (void *)PACKET_MQTT;
	packet->mFuncID		= 0x1802;
	packet->mCurFrame	= 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	    = CalcChecksumGITPtclPayloadFrame(packet);
	
	message->pPacket	= (void *)packet;
	message->mPktType	= PACKET_MQTT;
	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

void FL_GitDevice_info( stCommPkt *pkt, uint32_t eInCommType )
{
	stCommPkt	*packet;
	stMsgClst	*message;
    uint8_t		*pPayloadPtcl	= pkt->mData;
	uint32_t	usLength		= pkt->mLen - 8;
	uint8_t 	status			= 0;
	uint8_t		device_info[50] = {0};
	uint8_t		index			= 0;
	
	if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK)
	{
		GLogE("eLOCK_STATE_UNLOCK\r\n");
		return;
	}
	
#if defined(DEBUG_GIT_PTCL_LOG)
	GLogI( ">>> Start %s\r\n", __FUNCTION__ );
#endif

	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
	if( message == NULL )
	{
		return;
	}
	packet	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
	if( packet == NULL )
	{
		osPoolFree( hMsgPool, (void *)message );
		return;
	}
	
	memcpy(&device_info[0], gsFwInfo.marrucSerialNo, 8);
	for (int i = 0; i < 17; i++)
	{
		if (g_strWifiMacAddress[i] != ':')
		{
			device_info[8 + index] = g_strWifiMacAddress[i];
			index++;
		}
	}
	memcpy(packet->mData, device_info, 20);
	
	packet->mLen		= 20;
	packet->pTarget		= &eInCommType;
	packet->mFuncID		= 0x1803;
	packet->mCurFrame	= 0;
	memcpy(&(packet->UUID), &(pkt->UUID), sizeof(UUID_Struct));

	packet->mCS 	    = CalcChecksumGITPtclPayloadFrame(packet);
	
	message->pPacket	= (void *)packet;
	message->mPktType	= (ePKT_TD)eInCommType;
	if(osMessageAvailableSpace(hTransmitMsg) == 0)
	{
		osPoolFree( hCommPKPool, (void *)packet );
		osPoolFree( hMsgPool, (void *)message );
	}
	else
	{
		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
	}
}

