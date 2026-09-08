/**
  ******************************************************************************
  * @file    GIT_BluetoothLowEnergy.c
  * @author  GIT Diagnosis Software Team by james jean
  * @version V1.1.0
  * @date    29-JAN-2016
  * @brief   Manager BluetoothLowEnery.c module
  ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "GIT_BluetoothLowEnergy.h"
#include "GIT_OemInterface.h"
#include "GIT_Interprotocol.h"
#include "Autolink_Manager.h"
#include "GIT_Util.h"
#include "Share_InterFunction.h"
#include "OBD_Controller.h"


#include "HdDebug.h"
#include "HalHandler.h"


#define Trace(...)  GITDebug(DEBUG_MODULES_BLE,__VA_ARGS__)


#define DBGLOG_BLUETOOTH			UART_DEBUG
#define	BLE_INFO_FILENAME			"BLEInfo.inf"
#define MAX_SIZE_TMP_BUFF			(256)
#define BLE_PREFIX_DEVICE_NAME      "AL_"

extern eVEHICLE_STATE Get_VehicleStatus(void);
void ConversionHexStringToHex(char* pIn, char *pOut, int nInLength, int nRadix);

typedef void (*fnBleParsingCallBack)(BYTE*, uint32_t);

typedef struct _tbBTCmdList
{
	eBTCmdIndex	eSendCmdIndex;
	char 		*strSendDataReq;
	char		*strResponseData;
	fnBleParsingCallBack	fp;
}tbBTCmdList;

#ifdef BNCOM //mod.kks
#define ANY_DATA			"ANY_DATA"
#define BLE_NOTI_READY		"READY\r"
#define BLE_RES_OK			"OK\r"
#define BLE_RES_ERROR		"ERROR\r"
eBTCmdIndex g_eBLECmdSending = eBLECmd_EvtInitNoti;
#endif //ifdef BNCOM
tbBTCmdList g_tbBTCmdList[] =
{
#ifdef BNCOM
	// Cmd index		 		Request Cmd			  Response Cmd1,     Response Cmd2
	{eBLECmd_SetSWReset,		"ATZ\r",			  BLE_RES_OK		,BtEvtAck},
	{eBLECmd_SetConfigRestore,	NULL,				  NULL				,NULL},
	{eBLECmd_GetVersion,		"AT+VER?\r",		  ANY_DATA			,BtEvtFwVersion},
	{eBLECmd_GetBLE_Name,		"AT+BTNAME?\r", 	  ANY_DATA			,BtEvtName},
	{eBLECmd_SetBLE_Name,		"AT+BTNAME=%s\r",	  BLE_RES_OK		,BtEvtAck},
	{eBLECmd_BD_ADDR,			"AT+BTADDR?\r",		  ANY_DATA			,BtEvtAddress},
	{eBLECmd_UartConfig,		NULL,				  NULL				,NULL},
	{eBLECmd_Menufacturer,		NULL,				  NULL				,NULL},
	{eBLECmd_TXPWR,				NULL,				  NULL				,NULL},
	{eBLECmd_ADVInterval,		"AT+ADVINTERVAL=6\r", BLE_RES_OK        ,BTSetRepeatModeRes}, // mod.pdh 22.02.08 to set advertising interval
	{eBLECmd_ConnInterval, 		NULL,				  NULL				,NULL},
	{eBLECmd_ReMote_BD_ADDR, 	"AT+REMOTEADDR\r",	  ANY_DATA			,BtEvtRemoteBtAddr},
	{eBLECmd_GetAdvertiseState,	"AT+ADVSTATE?\r",	  ANY_DATA			,BtEvtBleAdvertising},
	{eBLECmd_SetAdvertiseState,	"AT+ADVSTATE=%s\r",	  BLE_RES_OK		,BtEvtAck},
	{eBLECmd_EvtInitNoti,		NULL,				  BLE_NOTI_READY	,BtEvtDevInit}, 	 // init
	{eBLECmd_EvtReponseOK,		NULL,				  BLE_RES_OK		,BtEvtAck}, 		 // reponse OK
	{eBLECmd_EvtReponseERROR,	NULL,				  BLE_RES_ERROR		,BtEvtNack},		 // reponse ERROR
	{eBLECmd_EvtConnected,		NULL,				  NULL		        ,NULL},
	{eBLECmd_EvtDisConnected,	NULL,				  NULL	            ,NULL},
	{eBLECmd_AT_TEST,			NULL,				  NULL				,NULL},
	{eBLECmd_GetSVCData,		"AT+SVCDATA?\r",	  ANY_DATA			,BtEvtBleSVCData},
	{eBLECmd_SetSVCData,		"AT+SVCDATA=%s\r",	  BLE_RES_OK		,BtEvtAck},
#else
    // Cmd index		 		Request Cmd			  Response Cmd1,    Response Cmd2
	{eBTCmd_GetBTInitNoti,		NULL,	 			  "*F0", 			BTGetModuleInitNoti},
	{eBTCmd_DataRestore,		"AT+00", 			  "*00", 			BTSetModuleRestoreRes},
	{eBTCmd_ModuleReset,		"AT+01", 			  "*01", 			BTSetModuleResetRes},
	{eBTCmd_AdvertisingStart,	"AT+10", 			  "*10", 			BTStartAdvertisingRes},
	{eBTCmd_AdvertisingStop,	"AT+11", 		 	  "*11", 			BTStopAdvertisingRes},
	{eBTCmd_GetModuleState,		"AT+20",		 	  "*20", 			BTGetModuleStateRes},
	{eBTCmd_GetLocalAddress,	"AT+24", 			  "*24", 			BTGetLocalAddressRes},
	{eBTCmd_GetDeviceName,		"AT+23", 			  "*23", 			BTGetLocalDeviceNameRes},
	{eBTCmd_SetDeviceName,		"AT+21", 			  "*21", 			BTSetLocalDeviceNameRes},
	{eBTCmd_GetFirmwareVer,		"AT+22", 			  "*22", 			BTGetFWVersionRes},
	{eBTCmd_SetOtaActivate,		"AT+12", 			  "*12", 			BTSetOTAActivateRes},
	{eBTCmd_GetNameLength,		"AT+25", 			  "*25", 			BTGetNameLengthRes},
	{eBTCmd_SetRepeatMode,		"AT+30", 			  "*30", 			BTSetRepeatModeRes},
	{eBTCmd_OffRepeatMode,		"AT+31", 			  "*31", 			BTOffRepeatModeRes},
	{eBTCmd_SetAdvertisingValue,"AT+13", 			  "*13", 			BTSetAdvertisingVaule},
	//{eBTCmd_DUT_Mode,			"ATC92=00", "ATA92=00", BTGetDUTPacketRes},
#endif
};


#pragma section="BKSRAM"
stBTInfo g_LocalBTInfo @"BKSRAM";

extern int g_nBTTransmitLedDelayCount;
#ifdef BNCOM //mod.kks 21.10.21
void BtEvtAck(uint8_t* pBuffer, uint32_t nLength)
{
	Trace("BtEvtAck, g_eBLECmdSending %d\r\n", g_eBLECmdSending);
	if ( g_eBLECmdSending == eBLECmd_SetAdvertiseState )
	{
		BTSend_VehicleStatus((unsigned char)Get_VehicleStatus());
	}
    else if ( g_eBLECmdSending == eBLECmd_SetBLE_Name )
    {
        if ( strncmp((char const*)pBuffer, BLE_RES_OK, strlen(BLE_RES_OK)) == 0 )
        {
            Trace( "BLE: eBLECmd_SetBLE_Name RES OK set eBT_Ready\r\n");
            BTSetState(eBT_Ready);
        }
        else
        {
            Trace( "BLE: eBLECmd_SetBLE_Name RES FAIL -> retry\r\n");
            memset(&g_LocalBTInfo.strDeviceName, 0x00, MAX_BT_DEVICE_NAME);
            BTSetLocalDeviceNameReq();
        }
    }
}
void BtEvtNack(uint8_t* pBuffer, uint32_t nLength)
{
    Trace("BtEvtNack\r\n");
}
void BtEvtSleepAck(uint8_t* pBuffer, uint32_t nLength)
{
    Trace("BtEvtSleepAck\r\n");
}
void BtEvtFwVersion(uint8_t* pBuffer, uint32_t nLength)
{
	char arrBleVerTmp[16];
	memcpy(arrBleVerTmp, pBuffer, nLength);
	BTGetFWVersionRes(pBuffer, nLength);
	BTGetLocalAddressReq();
}
void BtEvtName(uint8_t* pBuffer , uint32_t nLength)
{
	Trace("Bt BtEvtName\r\n");
	hexdump(pBuffer, nLength);
	if ( strcmp((char*)pBuffer, BLE_RES_ERROR) ==0 )
	{
		BtEvtNack(pBuffer, nLength);
		return;
	}
	else if ( strcmp((char*)pBuffer, BLE_RES_OK) == 0 )
	{
		BtEvtAck(pBuffer, nLength);
		return;
	}
	else if ( strcmp((char*)pBuffer, BLE_NOTI_READY) == 0 )
	{
		Trace("BLE_NOTI_READY NOTI\r\n");
		return;
	}

	BTSetLocalDeviceNameReq();
}

void BtEvtAddress(uint8_t* pBuffer, uint32_t nLength)
{
    Trace("Bt Address\r\n");

	char arrTemp[BT_ADDRESS_LEN];
	ConversionHexStringToHex((char*)pBuffer, arrTemp, BT_ADDRESS_LEN*2, 16);
	memcpy(&g_LocalBTInfo.bt_addr.btAddr, arrTemp, BT_ADDRESS_LEN);
	Trace( "BLE: Response GetAdd: MAC %02X %02X %02X %02X %02X %02X\r\n",
			   g_LocalBTInfo.bt_addr.btAddr[5], g_LocalBTInfo.bt_addr.btAddr[4], g_LocalBTInfo.bt_addr.btAddr[3],
			   g_LocalBTInfo.bt_addr.btAddr[2], g_LocalBTInfo.bt_addr.btAddr[1], g_LocalBTInfo.bt_addr.btAddr[0]);
	if ( BTGetState() == eBT_initialized ) {
		BTGetLocalDeviceNameReq();	//BT DEVICE NAME
	}
}

void BtEvtRemoteBtAddr(uint8_t* pBuffer, uint32_t nLength)
{
    Trace("BtEvtRemoteBtAddr\r\n");
}
void BtEvtBleAdvertising(uint8_t* pBuffer, uint32_t nLength)
{
    Trace( "BLE: %s : BtEvtBleAdvertising %s\r\n", __FUNCTION__, pBuffer);
	if ( strncmp((char*)pBuffer, "ON", strlen("ON")) == 0 )
	{
		BTSetState(eBT_Ready);
        Trace( "BLE[%s]: set eBT_Ready ~~~~~~~~~~~\r\n", __FUNCTION__);
	}
	else if( strncmp((char*)pBuffer, "OFF", strlen("OFF")) == 0 )
	{
		Trace( "BLE: %s : Fail\r\n", __FUNCTION__);
		BTStartAdvertisingReq();
	}
}

void BtEvtDevInit(uint8_t* pBuffer, uint32_t nLength)
{
	Trace( "BLE: NOTI\r\n");
	BTGetFWVersionReq();
	//BTGetFWVersionRes(pBuffer, nLength);
	BTSetState(eBT_initialized);
	//BTGetLocalAddressReq();	// 복제방지 체크하기 위해 BT Mac획득
}

void BtEvtBleSVCData(uint8_t* pBuffer, uint32_t nLength)
{
	char arrTmp[4];
	memcpy(arrTmp, pBuffer, sizeof(arrTmp));
	arrTmp[3] = 0x00;
    Trace("BLE Response Sevice Data %s ======\r\n", arrTmp);
}

boolean_t FindBtBnComPacket(unsigned char* pBuff,unsigned char* pBtPacketData, unsigned int* punRcvLength)
{
    int i, bRet = 0;    //-1:gabage packet 0 : recv packet, 1 : recv packet & found \r\n
    unsigned int uiReceivedPacket, uiFront;
    stQueue *pQueue = (stQueue*)pBuff;
    //char ucTemp;

    if ( IsEmptyQueue(pQueue) ) return FALSE;

    uiReceivedPacket = GetQueueDataLength(pQueue);
    uiFront = pQueue->uiFront;

    for ( i=0; i<uiReceivedPacket; i++ )
    {
        if ( (pQueue->pData[(uiFront+i)%pQueue->uiQueueSize] ==  0x0D)/*'\r'*/  )
        {
            bRet = TRUE;
            PopMultiDataQueue(pQueue, pBtPacketData, i+1);
            *punRcvLength = i+1;
            break;
        }
/*
        else if ( (pQueue->pData[(uiFront+i)%pQueue->uiQueueSize] < 0x21) ||
                  (pQueue->pData[(uiFront+i)%pQueue->uiQueueSize] > 0x7E) )
        {
            PopQueue(pQueue, &ucTemp);
            bRet = FALSE;
        }
*/
    }

    return bRet;
}

void Make_n_SendToBnComUart(eBTCmdIndex CmdIdx, char *pArgument)
{
    char arrSendCmd[32];
    memset(arrSendCmd, 0x00, sizeof(arrSendCmd));

	strcpy(arrSendCmd, g_tbBTCmdList[CmdIdx].strSendDataReq);

	if ( pArgument != NULL )
		sprintf(arrSendCmd, g_tbBTCmdList[CmdIdx].strSendDataReq, pArgument);

	SendGITPtclFrame(arrSendCmd, strlen(arrSendCmd), eCOMM_TYPE_UART_BT, NULL, 0);
	//OemWriteUart2Buff(arrSendCmd, strlen(arrSendCmd), NULL, eCOMM_TYPE_UART_BT);
}

#endif //ifdef BNCOM

void BT_Initialize(void)
{
	stHalGPIO_InitTypeDef  GPIO_InitStructure;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOC_GROUP|GPIOD_GROUP, NULL, 0, HAL_ENABLE);

	// Bluetooth State : connect or Disconnect
	GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_Pin = GPIO_BT_STATUS;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

#ifndef BNCOM //mod.kks 21.10.27
	// GPIO_BT_RESET
	GPIO_InitStructure.GPIO_Pin = GPIO_BT_RESET;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);


	// GPIO_BT_POWER_ENABLE
	GPIO_InitStructure.GPIO_Pin = GPIO_BT_PWEN;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;

    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
#endif

#ifdef BNCOM //mod.pdh 2021.10.27
#else
	HalGPIOSetVaule(GPIO_BT_PWEN, eBIT_SET);
#endif //#ifdef BNCOM

	Trace("BLE: Power On\r\n");
	// bluetooth mode : Sleep & Wakeup
	GPIO_InitStructure.GPIO_Pin = GPIO_BT_WAKE;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	Trace("BLE: The wake pin is High\r\n");
	HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_SET);

	//*******************************************************************
	// BT_CTS
	//*******************************************************************
	// 2017-06-22 오후 5:10:13 BLE port 초기화 할때, 왜 UART3의 CTS를 일반포트로 설정한후에 RESET 상태로 해주는지 잘 모르겠다.
	// 그런데, 이렇게 해야만 제대로 동작을 한다.
	GPIO_InitStructure.GPIO_Pin = GPIO_BT_CTS;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalGPIOSetVaule(GPIO_BT_CTS, eBIT_RESET);
	//*******************************************************************
#ifdef BNCOM //mod.pdh 2021.10.27
#else
	//*******************************************************************
	// GPIO_BT_TX_DW
	//*******************************************************************
	// autolink project 이전에는 이 포트를 연결하지 않았는데,
	// autolink project에서는 MCU의 port와 연결하도록 했다.
	// 이 포트를 GPIO output으로 설정하며, low로 유지 하도록 한다.
	GPIO_InitStructure.GPIO_Pin = GPIO_BT_TX_DW;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalGPIOSetVaule(GPIO_BT_TX_DW, eBIT_RESET);
#endif //#ifdef BNCOM

//	APP_Delay(500);

	Trace("BLE: Power On\r\n");
	if((AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON) || (BTGetConnectStatus() == 0)) {
		Trace("BLE: H/W Reset\r\n");
#ifdef BNCOM
		HalGPIOSetVaule(GPIO_BT_RESET, eBIT_SET);

#else
		HalGPIOSetVaule(GPIO_BT_RESET, eBIT_RESET);

		Oem_GIT_mDelay(10);

		HalGPIOSetVaule(GPIO_BT_RESET, eBIT_SET);

		Oem_GIT_mDelay(100);

		HalGPIOSetVaule(GPIO_BT_RESET, eBIT_RESET);
#endif
	}
	else {
		printf("BLE: No Reset\n");
	}

	BTSetState(eBT_NONE);

	BT_Spp_Init();

	return;
}

void BT_Spp_Init(void)
{
	Trace( "BLE: @%s() run\r\n", __FUNCTION__);
	Trace( "BLE: g_LocalBTInfo: 0x%x\r\n", &g_LocalBTInfo);
	Trace("BLE: Spp_Init %d \r\n", BTGetConnectStatus());

	if(BTGetConnectStatus() == 1) {	//연결돼 있을시에는 저장해놓은 데이터 사용
		Trace("BLE: state --> eBT_Ready\r\n");
		BTSetState(eBT_Ready);
	}
	else {
	}
}

void BTSetState(eBleutoothState eBTState)
{
	g_LocalBTInfo.eBTState = eBTState;
}

eBleutoothState BTGetState(void)
{
	return g_LocalBTInfo.eBTState;
}

BOOL BTGetConnectStatus(void)
{
	static BOOL s_bPreStatus = FALSE;        
	BOOL bCurStatus;
	bCurStatus = HalGPIOGetStatus(GPIO_BT_STATUS);	

	if ( s_bPreStatus != bCurStatus )
        printf( "%s BLE %s\r\n", __FUNCTION__, (bCurStatus) ? "Connected":"Disconnected");

	s_bPreStatus = bCurStatus;

	return bCurStatus;
}

void BTModuleReInit(void)
{
	Trace( "BLE: %s run\r\n", __FUNCTION__);
	BTSetState(eBT_initializing);
#ifdef BNCOM
#else
	BTMake_n_SendPacket(eBTCmd_ModuleReset, NULL, 0);
#endif
}

void BTGetFWVersionReq(void)
{
	Trace("BLE: Ver Req\r\n");
	
#ifdef BNCOM
	g_eBLECmdSending = eBLECmd_GetVersion;
	Make_n_SendToBnComUart(g_eBLECmdSending, NULL);
#endif
}

void BTSetOTAActivateReq(BOOL bActivate)
{
	//char cActivate = (char)bActivate;
//	g_bOTAServiceAvctivate = TRUE;
	Trace( "BLE: OTA Req, %d\r\n",bActivate);
#ifdef BNCOM
    BTSetState(eBT_Ready); 
    Trace( "BLE[%s]: set eBT_Ready ~~~~~~~~~~~\r\n", __FUNCTION__);
#else
	BTMake_n_SendPacket(eBTCmd_SetOtaActivate, NULL, 0);
#endif
}

void BTGetLocalDeviceNameReq(void)
{
#ifdef BNCOM
    g_eBLECmdSending = eBLECmd_GetBLE_Name;
	Make_n_SendToBnComUart(g_eBLECmdSending, NULL);
#else
	BTMake_n_SendPacket(eBTCmd_GetDeviceName, NULL, 0);
#endif
}

void BTSetLocalDeviceNameReq(void)
{
	char strLocalBTDeviceName[MAX_BT_DEVICE_NAME];	// max device name is 20 bytes

	memset(strLocalBTDeviceName, 0x00, MAX_BT_DEVICE_NAME);
#if 0
	snprintf(strLocalBTDeviceName, 9, "%c%c%c%c%c%c%c%c",
			 g_FirmwareInfo.arrSerialNumber[0], g_FirmwareInfo.arrSerialNumber[1], g_FirmwareInfo.arrSerialNumber[2],
			 g_FirmwareInfo.arrSerialNumber[3], g_FirmwareInfo.arrSerialNumber[4], g_FirmwareInfo.arrSerialNumber[5],
			 g_FirmwareInfo.arrSerialNumber[6], g_FirmwareInfo.arrSerialNumber[7]);
#else
	snprintf(strLocalBTDeviceName, MAX_BT_DEVICE_NAME, "%s%c%c%c%c%c%c%c%c", BLE_PREFIX_DEVICE_NAME,
			g_FirmwareInfo.arrSerialNumber[0], g_FirmwareInfo.arrSerialNumber[1], g_FirmwareInfo.arrSerialNumber[2],
		 	g_FirmwareInfo.arrSerialNumber[3], g_FirmwareInfo.arrSerialNumber[4], g_FirmwareInfo.arrSerialNumber[5],
		 	g_FirmwareInfo.arrSerialNumber[6], g_FirmwareInfo.arrSerialNumber[7]);

#endif

	Trace("BLE: SeriName: %s\r\n", strLocalBTDeviceName);
	Trace("BLE: InfoName: %s\r\n", g_LocalBTInfo.strDeviceName);

	if ( strcmp(g_LocalBTInfo.strDeviceName, strLocalBTDeviceName) != 0 ) {
		Trace("BLE: Set Name Req\r\n");
#ifdef BNCOM
		g_eBLECmdSending = eBLECmd_SetBLE_Name;
        strncpy(g_LocalBTInfo.strDeviceName, strLocalBTDeviceName, MAX_BT_DEVICE_NAME);
		Make_n_SendToBnComUart(g_eBLECmdSending, strLocalBTDeviceName);
#else
		BTMake_n_SendPacket(eBTCmd_SetDeviceName, strLocalBTDeviceName, strlen(strLocalBTDeviceName));
#endif
	}
	else {
		BTSetOTAActivateReq(TRUE);
		//BTStartAdvertisingReq();
	}
}
#ifdef BNCOM //mod.kks 21.11.04
void BTReset(void)
{
    HalGPIOSetVaule(GPIO_BT_RESET, eBIT_RESET);
    HalGPIOSetVaule(GPIO_BT_PWEN, eBIT_RESET);
    Oem_GIT_mDelay(10);
    HalGPIOSetVaule(GPIO_BT_RESET, eBIT_SET);
    HalGPIOSetVaule(GPIO_BT_PWEN, eBIT_SET);
}
#endif //BNCOM

void BTStartAdvertisingReq(void)
{
	Trace("BLE: StartAdv Req\r\n");
#ifdef BNCOM
#else
	BTMake_n_SendPacket(eBTCmd_AdvertisingStart, NULL, 0);
#endif
}
void BTStopAdvertisingReq()
{
	Trace("BLE: StopAdv req\r\n");
#ifdef BNCOM
#else
	BTMake_n_SendPacket(eBTCmd_AdvertisingStop, NULL, 0);
#endif
}

void BTGetLocalAddressReq(void)
{
//	Trace( "BLE: GetAdd Req\r\n");
#ifdef BNCOM
	g_eBLECmdSending = eBLECmd_BD_ADDR;
	Make_n_SendToBnComUart(g_eBLECmdSending, NULL);
#else
	BTMake_n_SendPacket(eBTCmd_GetLocalAddress, NULL, 0);
#endif
}

void BTSetDUTPacketReq(BYTE* pData)
{
	// ATC92=00
	Trace( "BLE: SetDUT Req\r\n");
#ifdef BNCOM
#else
  	BTMake_n_SendPacket(eBTCmd_DUT_Mode, NULL, 0);
#endif
}

void BTSetMonitoringPacketReq(void)
{
	static int nCurrentTmr = 0, nPreTmr = 0;
	int nDeltaTmr;

	nCurrentTmr = Get_Tmr();
	if ( nPreTmr > 0 ) {
		nDeltaTmr = Get_TmrDelta(nCurrentTmr, nPreTmr);
		if ( nDeltaTmr >= BT_MONITORING_INTERVAL ) {
#ifdef BNCOM
#else
			BTMake_n_SendPacket(eBTCmd_GetModuleState, NULL, 0);
#endif
			nPreTmr = nCurrentTmr;
		}
	}
	else
		nPreTmr = nCurrentTmr;
}

void BTGetNameLengthReq(void)
{
	Trace( "BLE: GetNameLen\r\n");
#ifdef BNCOM
#else
	BTMake_n_SendPacket(eBTCmd_GetNameLength, NULL, 0);
#endif
}

void BTSetRepeatModeReq(U16 usAdveriseTime,U16 usSleepTime)
{
	//U16 usAdveriseTime=0;
	//U16 usSleepTime=10000;
#ifndef BNCOM
	U8 arrTime[4];

	arrTime[0] = usAdveriseTime / 256;
	arrTime[1] = usAdveriseTime % 256;
	arrTime[2] = usSleepTime / 256;
	arrTime[3] = usSleepTime % 256;
#endif

	Trace( "BLE: RepeatOn Req\r\n");

#ifdef BNCOM
	g_eBLECmdSending = eBLECmd_ADVInterval;
	Make_n_SendToBnComUart(g_eBLECmdSending, NULL);
#else
	BTMake_n_SendPacket(eBTCmd_SetRepeatMode, (char*)arrTime, sizeof(arrTime)/sizeof(arrTime[0]));
#endif
	BTSetState(eBT_Sleep_Ready);
}

void BTOffRepeatModeReq(void)
{
	Trace("BLE: RepeatOff req\r\n");
#ifdef BNCOM
#else
	BTMake_n_SendPacket(eBTCmd_OffRepeatMode, NULL, 0);
#endif
	if(g_bYUJINSelftestFlag == false)
		BTSetState(eBT_Sleep_Ready);
}

void BTSetModuleRestoreRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS )
		Trace( "BLE: %s : Success\r\n", __FUNCTION__);
	else
		Trace( "BLE: %s : Fail\r\n", __FUNCTION__);
}

void BTSetModuleResetRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS )
		Trace( "BLE: %s : Success\r\n", __FUNCTION__);
	else
		Trace( "BLE: %s : Fail\r\n", __FUNCTION__);
}

void BTStartAdvertisingRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS ) {
		Trace( "BLE: StartAdv Success, set eBT_Ready\n");
		BTSetState(eBT_Ready);
//		BT_LOCK_SET_STATE(eBT_LOCK_STATE_LOCK);		//lock
		//stBT_PTCL_PAYLOAD *pInterPtcl;
		//BTSetRebootRes(pInterPtcl,1);
		//BTGetSecurityPassRes();
		//BTSetRepeatModeReq(5000,5000);
		//BTSetRepeatModeReq(0,10000);
	}
	else if( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_FAIL )
	{
		Trace( "BLE: %s : Fail\n", __FUNCTION__);
		BTStartAdvertisingReq();
	}
}

void BTStopAdvertisingRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS )
	{
		Trace( "BLE: StorAdv S\n");

//		if ( g_bOTAServiceAvctivate == TRUE )
//			BTMake_n_SendPacket(eBTCmd_AdvertisingStart, NULL, 0);
	}
	else if( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_FAIL )
		Trace( "BLE: %s : Fail\n", __FUNCTION__);
}

void BTGetModuleStateRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == 0x00 )
		Trace( "BLE: %s : Idle State\n", __FUNCTION__);
	else if ( pData[BLE_PACKET_DATA_IDX] == 0x01 )
		Trace( "BLE: %s : Advertising State\n", __FUNCTION__);
}

void BTGetLocalAddressRes(BYTE* pData, uint32_t nLength)
{
	memcpy(&g_LocalBTInfo.bt_addr.btAddr, &pData[BLE_PACKET_DATA_IDX], BT_ADDRESS_LEN);

	Trace( "BLE: Response GetAdd: MAC %02X %02X %02X %02X %02X %02X\r\n",
			   g_LocalBTInfo.bt_addr.btAddr[5], g_LocalBTInfo.bt_addr.btAddr[4], g_LocalBTInfo.bt_addr.btAddr[3],
			   g_LocalBTInfo.bt_addr.btAddr[2], g_LocalBTInfo.bt_addr.btAddr[1], g_LocalBTInfo.bt_addr.btAddr[0]);

	if ( BTGetState() == eBT_initialized ) {
		//HalTimerChangeSWTimer(g_iBTNameResCallback, 100, eSWTimer_ONESHOT, BT_1SecCallback, TRUE);
//		Trace( "BLE: Local Name Req\n");
		BTGetLocalDeviceNameReq();	//BT DEVICE NAME
	}
}

void BTGetLocalDeviceNameRes(BYTE* pData, uint32_t nLength)
{
	unsigned char arrTmp[BLUETOOTH_SEND_PACKET_SIZE];
	unsigned char ucDataLength = pData[BLE_PACKET_LENGTH_IDX];

	memset(arrTmp, 0x00, BLUETOOTH_SEND_PACKET_SIZE);
	memcpy(arrTmp, &pData[BLE_PACKET_DATA_IDX], ucDataLength);

	memset(&g_LocalBTInfo.strDeviceName, 0x00, MAX_BT_DEVICE_NAME);
	memcpy(&g_LocalBTInfo.strDeviceName, arrTmp, MAX_BT_DEVICE_NAME);

	if( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_FAIL )
		Trace( "BLE: %s : Fail\n", __FUNCTION__);
	else
		Trace( "BLE: GetName[%d] -> [%s]\r\n", ucDataLength, arrTmp);

	if ( BTGetState() == eBT_initialized ) {

//		Trace( "BLE: Local Name Req\n");
		BTSetLocalDeviceNameReq();
	}

//	if ( g_iBTNameResCallback != -1 ) {
//		HalTimerClearSWTimer(g_iBTNameResCallback);
//		g_iBTNameResCallback = -1;
//	}
}

void BTSetLocalDeviceNameRes(BYTE* pData, uint32_t nLength)
{
	if( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_FAIL )
		Trace( "BLE: %s : Fail\r\n", __FUNCTION__);
	else
		Trace( "BLE: SetName S\r\n");

	BTSetOTAActivateReq(TRUE);
	//BTStartAdvertisingReq();
	//BTSetState(eBT_Ready);
}

void BTGetFWVersionRes(BYTE* pData, uint32_t nLength)
{
	unsigned char ucDataLength=0;
#ifdef BNCOM
	ucDataLength = nLength-1; // \r 데이터 제거
#else
	ucDataLength = pData[BLE_PACKET_LENGTH_IDX];
#endif
	memset(g_LocalBTInfo.strVersion, 0x00, MAX_BT_MODULE_VERSION);
//	g_stDeviceInfo.ucBTState=1;
	memcpy(g_LocalBTInfo.strVersion, &pData[BLE_PACKET_DATA_IDX], ucDataLength);
	Trace( "BLE: Ver : %s\r\n", g_LocalBTInfo.strVersion);
}

void BTSetOTAActivateRes(BYTE* pData, uint32_t nLength)
{
	BOOL bOtaActivate = FALSE;

	bOtaActivate = pData[BLE_PACKET_DATA_IDX];
	Trace( "BLE: SetOTAAct S : %s\r\n", (bOtaActivate==TRUE) ? "ACTIVE":"INACTIVE");
#if defined(BLE_NEW_VERSION)
	BTSetRepeatModeReq(1000,1000);
#else
	BTStartAdvertisingReq();
#endif
//	if ( g_bOTAServiceAvctivate == TRUE && bOtaActivate == TRUE )
//		BTMake_n_SendPacket(eBTCmd_AdvertisingStop, NULL, 0);
}

void BTGetNameLengthRes(BYTE* pData, uint32_t nLength)
{
	U8 ucAvailableLength = 0;

	ucAvailableLength = pData[BLE_PACKET_DATA_IDX];
	Trace( "BLE: GetNameLen S :AvailableLength %d\r\n",ucAvailableLength);
}
void BTSetRepeatModeRes(BYTE* pData, uint32_t nLength)
{
#ifndef BNCOM
	BOOL bRepeatEnable = FALSE;
	bRepeatEnable = pData[BLE_PACKET_DATA_IDX];
	Trace( "BLE: SetRepeat S : %s\r\n", (bRepeatEnable==TRUE) ? "REPEAT ENABLE":"REPEAT FAIL");

#if defined(BLE_NEW_VERSION)
	BTStartAdvertisingReq();
#else //#if defined(BLE_NEW_VERSION)
	if ( BTGetState() == eBT_Sleep_Ready )
	{
		//BTSetOTAActivateReq(TRUE);
		HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_RESET);
		//MCUSleep();
		BTSetState(eBT_Sleep_Complete);
	}
#endif //#if defined(BLE_NEW_VERSION)

#else //#ifndef BNCOM
	printf("BLE : Set Repeat\r\n");
	if ( BTGetState() == eBT_Sleep_Ready )
	{
		//BTSetOTAActivateReq(TRUE);
		HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_RESET); //mod.kks  move to the MngSystem.c.
		//MCUSleep();
		BTSetState(eBT_Sleep_Complete);
	}
#endif//#ifndef BNCOM

}
void BTOffRepeatModeRes(BYTE* pData, uint32_t nLength)
{
	BOOL bRepeatDisable = FALSE;

	bRepeatDisable = pData[BLE_PACKET_DATA_IDX];
	Trace( "BLE: OffTepeat S : %s\r\n", (bRepeatDisable==TRUE) ? "REPEAT DISABLE":"REPEAT FAIL");

	if(( BTGetState() == eBT_Sleep_Ready ) && (g_bYUJINSelftestFlag == false)) {
		//BTSetOTAActivateReq(TRUE);
		HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_RESET);
		//MCUSleep();
		BTSetState(eBT_Sleep_Complete);
	}
}

void BTSetAdvertisingVaule(BYTE* pData, uint32_t nLength)
{
    static unsigned char ucFailCount=0;
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS )
	{
		Trace( "BLE: Change ADV value S\n");
        ucFailCount=0;
//		if ( g_bOTAServiceAvctivate == TRUE )
//			BTMake_n_SendPacket(eBTCmd_AdvertisingStart, NULL, 0);
	}
	else if( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_FAIL )
	{
		Trace( "BLE: %s : Change ADV value Fail\n", __FUNCTION__);
        if( ucFailCount < 5 )
        {
            BTSend_VehicleStatus((unsigned char)Get_VehicleStatus());
            ucFailCount++;
        }
	}
}


void BTGetModuleInitNoti(BYTE* pData, uint32_t nLength)
{
	Trace( "BLE: NOTI\r\n");

	BTGetFWVersionRes(pData, nLength);

	BTSetState(eBT_initialized);
	BTGetLocalAddressReq();	// 복제방지 체크하기 위해 BT Mac획득
}

void BTGetSppStreamData(BYTE* pData, uint32_t nLength)
{
	int i;
	int nSppIndex;
	int nRecvStreamData;
/*
	if ( GetBoardID() < PP_BOARD )
	{
		nSppIndex = 0;
		if ( pData[3] == 0xFF )
			nRecvStreamData = MAX_SPP_PACKET_SIZE+1;
		else
			nRecvStreamData = pData[3];

		for ( i=4; i<nRecvStreamData+4; i++ )
		{
			PushQueue((stQueue*)g_stGitCommInfo[nSppIndex+2].pstInQueue, pData[i], eCOMM_TYPE_UART_BT);
		}
	}
	else
*/
	{
		nSppIndex = pData[4];
		if ( pData[3] == 0xFF )
			nRecvStreamData = MAX_SPP_PACKET_SIZE;
		else
			nRecvStreamData = pData[3] - 1;

		for ( i=5; i<nRecvStreamData+5; i++ )
		{
			PushQueue((stQueue*)g_stGitCommInfo[nSppIndex+2].pstInQueue, pData[i], eCOMM_TYPE_UART_BT);
		}
	}
}


void BTGetDUTPacketRes(BYTE* pData, uint32_t nLength)
{
	Trace( "BLE: GetDUT S\r\n");
}

int BTRecvGetLine(stQueue *pQueue)
{
	int i, bRet = 0; 	//-1:gabage packet 0 : recv packet, 1 : recv packet & found \r\n
	uint32_t uiReceivedPacket;
	unsigned char ucTmp;

	uiReceivedPacket = GetQueueDataLength(pQueue);

	// find header of Response packet
	for ( i=0; i<uiReceivedPacket; i++ )
	{
		if ( pQueue->pData[(pQueue->uiFront)%pQueue->uiQueueSize] == 0x2A /* '*' */  )
		{
//			GITDebugPrintf("found packet : 0x%02X\r\n", pQueue->pData[(pQueue->uiFront+i)%pQueue->uiQueueSize]);
			break;
		}
		else
		{
			PopQueue(pQueue, &ucTmp);
//			GITDebugPrintf("Removce packet : 0x%02X\r\n", ucTmp);
		}
	}


	uiReceivedPacket = GetQueueDataLength(pQueue);
	if ( uiReceivedPacket < BLUETOOTH_SEND_PACKET_SIZE ) return 0;

	if ( (pQueue->pData[((pQueue->uiFront+BLUETOOTH_SEND_PACKET_SIZE-2))%pQueue->uiQueueSize] == 0x0D)/*'\n'*/ &&
		 (pQueue->pData[((pQueue->uiFront+BLUETOOTH_SEND_PACKET_SIZE-1))%pQueue->uiQueueSize] ==  0x0A)/*'\r'*/  )
	{
//		GITDebugPrintf("F:%d S:%d F+P:%d",pQueue->uiFront, pQueue->uiQueueSize, pQueue->uiFront+BLUETOOTH_SEND_PACKET_SIZE);
		bRet = 1;
	}
	else
	{
		// BLUETOOTH_SEND_PACKET_SIZE가 넘었으나 0x0D와 0x0A를 찾지 못함. 따라서 제거함.
		bRet = -1;
	}

	return bRet;
}

void BTMake_n_SendPacket(eBTCmdIndex CmdIdx, char* CmdData, unsigned short nCmdDataSize)
{
	char* strBtCmd;
	BYTE arrSendPacket[BLUETOOTH_SEND_PACKET_SIZE];

	strBtCmd = g_tbBTCmdList[CmdIdx].strSendDataReq;
	memset(arrSendPacket, 0xFF, BLUETOOTH_SEND_PACKET_SIZE);

	strncpy((char*)arrSendPacket, strBtCmd, strlen(strBtCmd)); 	// 0 ~ 7(header + command)
	if ( nCmdDataSize > 0 && CmdData != NULL )
	{
		arrSendPacket[5] = nCmdDataSize; 					// *DS : Data Size
		memcpy(&arrSendPacket[6], CmdData, nCmdDataSize);	// Command Data argument
	}
	arrSendPacket[30] = 0x0D;								// CR
	arrSendPacket[31] = 0x0A;								// LF

	OemWriteUartBTBuff(arrSendPacket, BLUETOOTH_SEND_PACKET_SIZE, NULL, eCOMM_TYPE_UART_BT);

#ifdef TEST_BT_UART
	{
		int i;
		Trace("[send]");
		for ( i=0; i<BLUETOOTH_SEND_PACKET_SIZE; i++ )
		{
			Trace("%02X ", arrSendPacket[i]);
		}
		Trace("\r\n");
	}
#endif
}

BOOL BTRecvParsing(BYTE* pData, uint32_t nLength)
{
	int i, nloopCnt;
	// unsigned char arrTmp[BLUETOOTH_SEND_PACKET_SIZE]; mod.kks todo remove.
	stQueue *pQueue = (stQueue*)pData;
#ifdef TEST_BT_UART
{
	int i;
	Trace("[recv]");
	for ( i=0; i<nLength; i++ )
	{
		Trace("%02X ", pData[i]);
	}
	Trace("\r\n");
}
#endif


	nloopCnt = sizeof(g_tbBTCmdList)/sizeof(g_tbBTCmdList[0]);

	for ( i=0; i<nloopCnt; i++ ) {
#ifdef BNCOM //mod.pdh 2021.10.27
		if( g_tbBTCmdList[i].strResponseData == NULL )
		{
		}
		else if( (g_eBLECmdSending == i) && (( strncmp((char*)pQueue, (char*)g_tbBTCmdList[i].strResponseData,
						   strlen(g_tbBTCmdList[i].strResponseData)) == 0 ) ||
				 (strncmp(ANY_DATA, g_tbBTCmdList[i].strResponseData, strlen(ANY_DATA))) == 0) )
		{
			if( g_tbBTCmdList[i].fp != NULL )
			{
				g_tbBTCmdList[i].fp((uint8_t*)pQueue,nLength);
				//g_eBLECmdSending = eBLECmd_EvtInitNoti; // 초기화
				break;
			}
		}
#else // ADANIS
        unsigned char arrTmp[BLUETOOTH_SEND_PACKET_SIZE];
		PopMultiDataQueue(pQueue, arrTmp, BLUETOOTH_SEND_PACKET_SIZE);

		if ( memcmp(arrTmp, g_tbBTCmdList[i].strResponseData, 3) == 0 ) {
			if ( g_tbBTCmdList[i].fp != NULL ) {
				g_tbBTCmdList[i].fp(arrTmp, nLength);
				break;
			}
		}
#endif
	}

	return TRUE;
}

void BTSend_VehicleStatus(unsigned char ucVehicle_state)
{

#ifdef BNCOM // mod.pdh 2021.11.08

	char arrTmp[3]; // 3byte 설정 가능함(스트링으로)

	arrTmp[0] = 0x19;	// 에더니스 service UUID와 동일하게 적용.
	arrTmp[1] = 0x97;

	if( ucVehicle_state >= eVEHICLE_STATE_ENGRUN )
		arrTmp[2] = eAutolinkBluetootBeaconStatus_Wakeup;
	else
		arrTmp[2] = eAutolinkBluetootBeaconStatus_Sleep;

	g_eBLECmdSending = eBLECmd_SetSVCData;
	Make_n_SendToBnComUart(g_eBLECmdSending, arrTmp);
#else
	uint8_t ucSendValue;

	if(ucVehicle_state >  eVEHICLE_STATE_IGON)
	{
		ucSendValue = eAutolinkBluetootBeaconStatus_Wakeup;
	}
	else
	{
		ucSendValue = eAutolinkBluetootBeaconStatus_Sleep;
	}

	BTMake_n_SendPacket(eBTCmd_SetAdvertisingValue, (char*)&ucSendValue, sizeof(ucSendValue));
#endif
}

/*****************************END OF FILE****/
