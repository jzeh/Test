/**
  ******************************************************************************
  * @file    git-ble.c
  * @author  GIT Diagnosis Software Team by james jean
  * @version V1.1.0
  * @date    29-JAN-2016
  * @brief   Bluetooth Low Energy (git-ble) module
  ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "../Inc/git-ble.h"
#include "../Inc/sys-common.h"
#include "../Inc/git-comm.h"
#include "../Inc/sys-emmc.h"   /* DeviceSerial_Get — BLE 광고 이름 suffix */

#define BNCOM
#define DBGLOG_BLUETOOTH			UART_DEBUG
#define	BLE_INFO_FILENAME			"BLEInfo.inf"
#define MAX_SIZE_TMP_BUFF			(256)
#define BLE_PREFIX_DEVICE_NAME      "BDC"


#define BLE_RX_QUEUE_SIZE 2048
#define BLE_RX_LINE_MAX   256

uint8_t  g_ble_rx_q[BLE_RX_QUEUE_SIZE];
uint16_t g_ble_rx_head = 0;
uint16_t g_ble_rx_tail = 0;

void ConversionHexStringToHex(char* pIn, char *pOut, int nInLength, int nRadix);
extern void IO_ALL_OFF_control(void);

typedef void (*fnBleParsingCallBack)(BYTE*, uint32_t);

typedef struct _tbBTCmdList
{
	eBTCmdIndex	eSendCmdIndex;
	char 		*strSendDataReq;
	char		*strResponseData;
	fnBleParsingCallBack	fp;
}tbBTCmdList;


/*
BNCOM UUID
- Data Service (Primary) 				: 0xFFF0	
- Notification (Characteristic) 		: 0xFFF1	(Notification)
- Write No Response (Characteristic) 	: 0xFFF2	(Write without response)
*/

#ifdef BNCOM //mod.kks
#define ANY_DATA			"ANY_DATA"
#define BLE_NOTI_READY		"READY\r"
#define BLE_RES_OK			"OK\r"
#define BLE_RES_ERROR		"ERROR\r"
eBTCmdIndex g_eBLECmdSending = eBLECmd_EvtInitNoti;
#endif //ifdef BNCOM
tbBTCmdList g_tbBTCmdList[] =
{
	// Cmd index		 		Request Cmd			  Response Cmd1,     Response Cmd2
	{eBLECmd_SetSWReset,		"ATZ\r",			  BLE_RES_OK		,BtEvtAck},
	{eBLECmd_SetConfigRestore,	NULL,				  NULL				,NULL},
	{eBLECmd_GetVersion,		"AT+VER?\r",		  ANY_DATA			,BtEvtFwVersion},
	{eBLECmd_GetBLE_Name,		"AT+BTNAME?\r", 	  ANY_DATA			,BtEvtName},
	{eBLECmd_SetBLE_Name,		"AT+BTNAME=%s\r",	  BLE_RES_OK		,BtEvtAck},
	{eBLECmd_BD_ADDR,			"AT+BTADDR?\r",		  ANY_DATA			,BtEvtAddress},
	{eBLECmd_UartConfig,			NULL,				  NULL				,NULL},
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
};


// #pragma section="BKSRAM"
stBTInfo g_LocalBTInfo;// @"BKSRAM";

extern int g_nBTTransmitLedDelayCount;

// BLE QUEUE funcs 시작
uint16_t BLE_RxCount(void)
{
    if (g_ble_rx_head >= g_ble_rx_tail) return g_ble_rx_head - g_ble_rx_tail;
    return (uint16_t)(BLE_RX_QUEUE_SIZE - g_ble_rx_tail + g_ble_rx_head);
}

void BLE_UART2_PushRx(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0) return;

    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (uint16_t)((g_ble_rx_head + 1) % BLE_RX_QUEUE_SIZE);
        if (next == g_ble_rx_tail) {
            g_ble_rx_tail = (uint16_t)((g_ble_rx_tail + 1) % BLE_RX_QUEUE_SIZE);
        }
        g_ble_rx_q[g_ble_rx_head] = data[i];
        g_ble_rx_head = next;
    }
}

static int BLE_PopLine(uint8_t *out, uint16_t out_max, uint16_t *out_len)
{
    uint16_t count = BLE_RxCount();
    for (uint16_t i = 0; i < count; i++) {
        uint16_t idx = (uint16_t)((g_ble_rx_tail + i) % BLE_RX_QUEUE_SIZE);
        if (g_ble_rx_q[idx] == '\r') {
            uint16_t line_len = (uint16_t)(i + 1);
            if (line_len >= out_max) {
                for (uint16_t k = 0; k < line_len; k++) {
                    g_ble_rx_tail = (uint16_t)((g_ble_rx_tail + 1) % BLE_RX_QUEUE_SIZE);
                }
                return 0;
            }
            for (uint16_t k = 0; k < line_len; k++) {
                out[k] = g_ble_rx_q[g_ble_rx_tail];
                g_ble_rx_tail = (uint16_t)((g_ble_rx_tail + 1) % BLE_RX_QUEUE_SIZE);
            }
            *out_len = line_len;
            return 1;
        }
    }
    return 0;
}

void BLE_ProcessRxQueue(void)
{
    uint8_t line[BLE_RX_LINE_MAX];
    uint16_t len = 0;

    while (BLE_PopLine(line, (uint16_t)(sizeof(line) - 1), &len)) {
        line[len] = '\0'; // Null terminate

        if (!BTGetConnectStatus()) {
			printf("[BT DISCONN] RX: %s\r\n", line);
			g_system_mode = eMODE_NONE;  // Force system to enter safe state
			g_system_state = eSYSTEM_STATE_NONE;

			BTRecvParsing(line, len); 
        }
    }
}
// BLE QUEUE funcs ?? --end

// BT CMD Funcs
void BtEvtAck(uint8_t* pBuffer, uint32_t nLength)
{
	// printf("BtEvtAck, g_eBLECmdSending %d\r\n", g_eBLECmdSending);

	if ( g_eBLECmdSending == eBLECmd_SetAdvertiseState )
	{
		// BTSend_VehicleStatus((unsigned char)Get_VehicleStatus());
	}
    else if ( g_eBLECmdSending == eBLECmd_SetBLE_Name )
    {
        if ( strncmp((char const*)pBuffer, BLE_RES_OK, strlen(BLE_RES_OK)) == 0 )
        {
            // printf( "BLE: eBLECmd_SetBLE_Name RES OK set eBT_Ready\r\n");
            BTSetState(eBT_Ready);
        }
        else
        {
            // printf( "BLE: eBLECmd_SetBLE_Name RES FAIL -> retry\r\n");
            memset(&g_LocalBTInfo.strDeviceName, 0x00, MAX_BT_DEVICE_NAME);
            BTSetLocalDeviceNameReq();
        }
    }
}
void BtEvtNack(uint8_t* pBuffer, uint32_t nLength)
{
    printf("BtEvtNack\r\n");
}
void BtEvtSleepAck(uint8_t* pBuffer, uint32_t nLength)
{
    printf("BtEvtSleepAck\r\n");
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
	// printf("Bt BtEvtName\r\n");
	// hexdump(pBuffer, nLength);
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
		printf("BLE_NOTI_READY NOTI\r\n");
		return;
	}

	BTSetLocalDeviceNameReq();
}

void BtEvtAddress(uint8_t* pBuffer, uint32_t nLength)
{
    // printf("Bt Address\r\n");

	char arrTemp[BT_ADDRESS_LEN];
	ConversionHexStringToHex((char*)pBuffer, arrTemp, BT_ADDRESS_LEN*2, 16);
	memcpy(&g_LocalBTInfo.bt_addr.btAddr, arrTemp, BT_ADDRESS_LEN);
	// printf( "BLE: Response GetAdd: MAC %02X %02X %02X %02X %02X %02X\r\n",
	// 		   g_LocalBTInfo.bt_addr.btAddr[5], g_LocalBTInfo.bt_addr.btAddr[4], g_LocalBTInfo.bt_addr.btAddr[3],
	// 		   g_LocalBTInfo.bt_addr.btAddr[2], g_LocalBTInfo.bt_addr.btAddr[1], g_LocalBTInfo.bt_addr.btAddr[0]);
	if ( BTGetState() == eBT_initialized ) {
		BTGetLocalDeviceNameReq();	//BT DEVICE NAME
	}
}

void BtEvtRemoteBtAddr(uint8_t* pBuffer, uint32_t nLength)
{
    printf("BtEvtRemoteBtAddr\r\n");
}
void BtEvtBleAdvertising(uint8_t* pBuffer, uint32_t nLength)
{
    printf( "BLE: %s : BtEvtBleAdvertising %s\r\n", __FUNCTION__, pBuffer);
	if ( strncmp((char*)pBuffer, "ON", strlen("ON")) == 0 )
	{
		BTSetState(eBT_Ready);
        printf( "BLE[%s]: set eBT_Ready ~~~~~~~~~~~\r\n", __FUNCTION__);
	}
	else if( strncmp((char*)pBuffer, "OFF", strlen("OFF")) == 0 )
	{
		printf( "BLE: %s : Fail\r\n", __FUNCTION__);
		BTStartAdvertisingReq();
	}
}

void BtEvtDevInit(uint8_t* pBuffer, uint32_t nLength)
{
	printf( "BLE: NOTI\r\n");
	BTGetFWVersionReq();
	//BTGetFWVersionRes(pBuffer, nLength);
	BTSetState(eBT_initialized);
	//BTGetLocalAddressReq();
}

void BtEvtBleSVCData(uint8_t* pBuffer, uint32_t nLength)
{
	char arrTmp[4];
	memcpy(arrTmp, pBuffer, sizeof(arrTmp));
	arrTmp[3] = 0x00;
    printf("BLE Response Sevice Data %s ======\r\n", arrTmp);
}
#if 0	// not used
bool FindBtBnComPacket(unsigned char* pBuff,unsigned char* pBtPacketData, unsigned int* punRcvLength)
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
#endif
void Make_n_SendToBnComUart(eBTCmdIndex CmdIdx, char *pArgument)
{
	char arrSendCmd[64];
    memset(arrSendCmd, 0x00, sizeof(arrSendCmd));

    const char *fmt = g_tbBTCmdList[CmdIdx].strSendDataReq;
    if (!fmt) return;

    if (pArgument) {
        snprintf(arrSendCmd, sizeof(arrSendCmd), fmt, pArgument);
    } else {
        snprintf(arrSendCmd, sizeof(arrSendCmd), "%s", fmt);
    }

    UART2_Transmit_Polling((unsigned char*)arrSendCmd, (uint16_t)strlen(arrSendCmd));
    g_eBLECmdSending = CmdIdx;
    printf("BLE: Send Cmd: %s", arrSendCmd);

    // char arrSendCmd[32];
    // memset(arrSendCmd, 0x00, sizeof(arrSendCmd));

	// strcpy(arrSendCmd, g_tbBTCmdList[CmdIdx].strSendDataReq);

	// if ( pArgument != NULL )
	// 	sprintf(arrSendCmd, g_tbBTCmdList[CmdIdx].strSendDataReq, pArgument);

	// // SendGITPtclFrame(arrSendCmd, strlen(arrSendCmd), eCOMM_TYPE_UART_BT, NULL, 0);
	// //OemWriteUart2Buff(arrSendCmd, strlen(arrSendCmd), NULL, eCOMM_TYPE_UART_BT);
	// UART2_Transmit_Polling((unsigned char*)arrSendCmd, strlen(arrSendCmd));
	// g_eBLECmdSending = CmdIdx;
	// printf( "BLE: Send Cmd: %s", arrSendCmd);
}


void BT_Initialize(void)
{
	printf( "BLE: %s() run\r\n", __FUNCTION__);
	GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* CTS pin (PA0) with pull-down to enable RX when not connected */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;  // Pull-down on CTS to allow RX
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // IO_CONTROL_LOW(BLE_USART2_CTS);
    IO_CONTROL_HIGH(BLE_USART2_CTS);
	IO_CONTROL_HIGH(BLE_RSTB);
	memset(&g_LocalBTInfo, 0x00, sizeof(stBTInfo));

	printf("BLE: Power On\r\n");
	// if(BTGetConnectStatus() == 0)
	// 	IO_CONTROL_HIGH(BLE_RSTB);

	// BTSetState(eBT_NONE);
	// BT_Spp_Init();

	return;
}

void BT_Spp_Init(void)
{
	printf( "BLE: @%s() run\r\n", __FUNCTION__);
	printf( "BLE: g_LocalBTInfo: 0x%x\r\n", &g_LocalBTInfo);
	printf("BLE: Spp_Init %d \r\n", BTGetConnectStatus());

	if(BTGetConnectStatus() == 1) 
	{	
		printf("BLE: state --> eBT_Ready\r\n");
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
	bCurStatus = IO_CONTROL_GET(BLE_STATUS);

	if ( s_bPreStatus != bCurStatus )
	{
		printf( "%s BLE %s\r\n", __FUNCTION__, (bCurStatus) ? "Connected":"Disconnected");

		if ( s_bPreStatus == TRUE && bCurStatus == FALSE )
		{
			g_system_state = eSYSTEM_STATE_NONE;
			g_system_mode  = eMODE_NONE;
			printf("[STATE] BLE Disconnected -> NONE\r\n");
		}
	}
	s_bPreStatus = bCurStatus;

	return bCurStatus;
}

void BTModuleReInit(void)
{
	printf( "BLE: %s run\r\n", __FUNCTION__);
	BTSetState(eBT_initializing);
#ifdef BNCOM
#else
	BTMake_n_SendPacket(eBTCmd_ModuleReset, NULL, 0);
#endif
}

void BTGetFWVersionReq(void)
{
	printf("BLE: Ver Req\r\n");
	
#ifdef BNCOM
	g_eBLECmdSending = eBLECmd_GetVersion;
	Make_n_SendToBnComUart(g_eBLECmdSending, NULL);
#endif
}

void BTSetOTAActivateReq(BOOL bActivate)
{
	//char cActivate = (char)bActivate;
//	g_bOTAServiceAvctivate = TRUE;
	printf( "BLE: OTA Req, %d\r\n",bActivate);
#ifdef BNCOM
    BTSetState(eBT_Ready); 
    printf( "BLE[%s]: set eBT_Ready ~~~~~~~~~~~\r\n", __FUNCTION__);
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

	/* BLE 광고 이름 = "BDC"(고정) + 디바이스 시리얼(eMMC 저장, 미설정 시 "00000000") */
	snprintf(strLocalBTDeviceName, MAX_BT_DEVICE_NAME, "%s%s",
			BLE_PREFIX_DEVICE_NAME, DeviceSerial_Get());

	printf("BLE: SeriName: %s\r\n", strLocalBTDeviceName);
	// printf("BLE: InfoName: %s\r\n", g_LocalBTInfo.strDeviceName);

	if ( strcmp(g_LocalBTInfo.strDeviceName, strLocalBTDeviceName) != 0 ) {
		printf("BLE: Set Name Req\r\n");
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

/* ── 디바이스 시리얼(이름) 갱신 pending 처리 ───────────────────────────────
 *  문제 : 0xB1(SetSerial) 은 BLE "연결 중" 에 수신된다. 이때 UART2 는 폰과의
 *         데이터 파이프(bypass) 라서 AT+BTNAME 을 보내도 모듈이 명령으로
 *         해석하지 않고 폰으로 흘려보낸다 → 광고 이름이 안 바뀐다.
 *  해결 : 연결 중에는 "대기 플래그" 만 세우고, 연결 해제 edge(명령 모드) 에서
 *         BTApplyPendingDeviceName() 이 캐시를 무효화한 뒤 AT+BTNAME 을 재전송.
 *         (부팅 시와 동일한 명령-모드 경로 → 확실히 반영)                     */
static volatile bool s_ble_name_update_pending = false;

void BTMarkDeviceNameDirty(void)
{
	s_ble_name_update_pending = true;
}

void BTApplyPendingDeviceName(void)
{
	if ( !s_ble_name_update_pending ) return;
	s_ble_name_update_pending = false;

	/* 캐시 이름을 강제 무효화 → BTSetLocalDeviceNameReq 의 strcmp 가
	 * 반드시 불일치하도록 만들어 AT+BTNAME 이 실제 전송되게 함. */
	memset(g_LocalBTInfo.strDeviceName, 0x00, MAX_BT_DEVICE_NAME);
	printf("BLE: apply pending device name (from serial change)\r\n");
	BTSetLocalDeviceNameReq();
}
#ifdef BNCOM 
void BTReset(void)
{
	IO_CONTROL_LOW(BLE_RSTB);
	HAL_Delay(10);
	IO_CONTROL_HIGH(BLE_RSTB);
	printf("BLE: BT Reset\r\n");
}
#endif //BNCOM

void BTStartAdvertisingReq(void)
{
	printf("BLE: StartAdv Req\r\n");
#ifdef BNCOM
#else
	BTMake_n_SendPacket(eBTCmd_AdvertisingStart, NULL, 0);
#endif
}
void BTStopAdvertisingReq()
{
	printf("BLE: StopAdv req\r\n");
#ifdef BNCOM
#else
	BTMake_n_SendPacket(eBTCmd_AdvertisingStop, NULL, 0);
#endif
}

void BTGetLocalAddressReq(void)
{
//	printf( "BLE: GetAdd Req\r\n");
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
	printf( "BLE: SetDUT Req\r\n");
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
	printf( "BLE: GetNameLen\r\n");
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

	printf( "BLE: RepeatOn Req\r\n");

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
	printf("BLE: RepeatOff req\r\n");
#ifdef BNCOM
#else
	BTMake_n_SendPacket(eBTCmd_OffRepeatMode, NULL, 0);
#endif
	BTSetState(eBT_Sleep_Ready);
}

void BTSetModuleRestoreRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS )
		printf( "BLE: %s : Success\r\n", __FUNCTION__);
	else
		printf( "BLE: %s : Fail\r\n", __FUNCTION__);
}

void BTSetModuleResetRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS )
		printf( "BLE: %s : Success\r\n", __FUNCTION__);
	else
		printf( "BLE: %s : Fail\r\n", __FUNCTION__);
}

void BTStartAdvertisingRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS ) {
		printf( "BLE: StartAdv Success, set eBT_Ready\n");
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
		printf( "BLE: %s : Fail\n", __FUNCTION__);
		BTStartAdvertisingReq();
	}
}

void BTStopAdvertisingRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS )
	{
		printf( "BLE: StorAdv S\n");

//		if ( g_bOTAServiceAvctivate == TRUE )
//			BTMake_n_SendPacket(eBTCmd_AdvertisingStart, NULL, 0);
	}
	else if( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_FAIL )
		printf( "BLE: %s : Fail\n", __FUNCTION__);
}

void BTGetModuleStateRes(BYTE* pData, uint32_t nLength)
{
	if ( pData[BLE_PACKET_DATA_IDX] == 0x00 )
		printf( "BLE: %s : Idle State\n", __FUNCTION__);
	else if ( pData[BLE_PACKET_DATA_IDX] == 0x01 )
		printf( "BLE: %s : Advertising State\n", __FUNCTION__);
}

void BTGetLocalAddressRes(BYTE* pData, uint32_t nLength)
{
	memcpy(&g_LocalBTInfo.bt_addr.btAddr, &pData[BLE_PACKET_DATA_IDX], BT_ADDRESS_LEN);

	printf( "BLE: Response GetAdd: MAC %02X %02X %02X %02X %02X %02X\r\n",
			   g_LocalBTInfo.bt_addr.btAddr[5], g_LocalBTInfo.bt_addr.btAddr[4], g_LocalBTInfo.bt_addr.btAddr[3],
			   g_LocalBTInfo.bt_addr.btAddr[2], g_LocalBTInfo.bt_addr.btAddr[1], g_LocalBTInfo.bt_addr.btAddr[0]);

	if ( BTGetState() == eBT_initialized ) {
		//HalTimerChangeSWTimer(g_iBTNameResCallback, 100, eSWTimer_ONESHOT, BT_1SecCallback, TRUE);
//		printf( "BLE: Local Name Req\n");
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
		printf( "BLE: %s : Fail\n", __FUNCTION__);
	else
		printf( "BLE: GetName[%d] -> [%s]\r\n", ucDataLength, arrTmp);

	if ( BTGetState() == eBT_initialized ) {

//		printf( "BLE: Local Name Req\n");
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
		printf( "BLE: %s : Fail\r\n", __FUNCTION__);
	else
		printf( "BLE: SetName S\r\n");

	BTSetOTAActivateReq(TRUE);
	//BTStartAdvertisingReq();
	//BTSetState(eBT_Ready);
}

void BTGetFWVersionRes(BYTE* pData, uint32_t nLength)
{
	unsigned char ucDataLength=0;
#ifdef BNCOM
	ucDataLength = nLength-1; // \r ??? ??
#else
	ucDataLength = pData[BLE_PACKET_LENGTH_IDX];
#endif
	memset(g_LocalBTInfo.strVersion, 0x00, MAX_BT_MODULE_VERSION);
//	g_stDeviceInfo.ucBTState=1;
	memcpy(g_LocalBTInfo.strVersion, &pData[BLE_PACKET_DATA_IDX], ucDataLength);
	printf( "BLE: Ver : %s\r\n", g_LocalBTInfo.strVersion);
}

void BTSetOTAActivateRes(BYTE* pData, uint32_t nLength)
{
	BOOL bOtaActivate = FALSE;

	bOtaActivate = pData[BLE_PACKET_DATA_IDX];
	printf( "BLE: SetOTAAct S : %s\r\n", (bOtaActivate==TRUE) ? "ACTIVE":"INACTIVE");
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
	printf( "BLE: GetNameLen S :AvailableLength %d\r\n",ucAvailableLength);
}
void BTSetRepeatModeRes(BYTE* pData, uint32_t nLength)
{
#ifndef BNCOM
	BOOL bRepeatEnable = FALSE;
	bRepeatEnable = pData[BLE_PACKET_DATA_IDX];
	printf( "BLE: SetRepeat S : %s\r\n", (bRepeatEnable==TRUE) ? "REPEAT ENABLE":"REPEAT FAIL");

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
		// HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_RESET); //mod.kks  move to the MngSystem.c.
		//MCUSleep();
		BTSetState(eBT_Sleep_Complete);
	}
#endif//#ifndef BNCOM

}
void BTOffRepeatModeRes(BYTE* pData, uint32_t nLength)
{
	BOOL bRepeatDisable = FALSE;

	bRepeatDisable = pData[BLE_PACKET_DATA_IDX];
	printf( "BLE: OffTepeat S : %s\r\n", (bRepeatDisable==TRUE) ? "REPEAT DISABLE":"REPEAT FAIL");

	if( BTGetState() == eBT_Sleep_Ready )  {
		//BTSetOTAActivateReq(TRUE);
		// HalGPIOSetVaule(GPIO_BT_WAKE, eBIT_RESET);
		//MCUSleep();
		BTSetState(eBT_Sleep_Complete);
	}
}

void BTSetAdvertisingVaule(BYTE* pData, uint32_t nLength)
{
    static unsigned char ucFailCount=0;
	if ( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_SUCCESS )
	{
		printf( "BLE: Change ADV value S\n");
        ucFailCount=0;
//		if ( g_bOTAServiceAvctivate == TRUE )
//			BTMake_n_SendPacket(eBTCmd_AdvertisingStart, NULL, 0);
	}
	else if( pData[BLE_PACKET_DATA_IDX] == BLE_COMMAND_FAIL )
	{
		printf( "BLE: %s : Change ADV value Fail\n", __FUNCTION__);
        if( ucFailCount < 5 )
        {
            // BTSend_VehicleStatus((unsigned char)Get_VehicleStatus());
            ucFailCount++;
        }
	}
}


void BTGetModuleInitNoti(BYTE* pData, uint32_t nLength)
{
	printf( "BLE: NOTI\r\n");

	BTGetFWVersionRes(pData, nLength);

	BTSetState(eBT_initialized);
	BTGetLocalAddressReq();
}



void BTGetDUTPacketRes(BYTE* pData, uint32_t nLength)
{
	printf( "BLE: GetDUT S\r\n");
}
#if 0 	// not used
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
		// BLUETOOTH_SEND_PACKET_SIZE? ???? 0x0D? 0x0A? ?? ??. ??? ???.
		bRet = -1;
	}

	return bRet;
}
#endif
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

	// OemWriteUartBTBuff(arrSendPacket, BLUETOOTH_SEND_PACKET_SIZE, NULL, eCOMM_TYPE_UART_BT);
	UART2_Transmit_Polling(arrSendPacket, BLUETOOTH_SEND_PACKET_SIZE);
#ifdef TEST_BT_UART
	{
		int i;
		printf("[send]");
		for ( i=0; i<BLUETOOTH_SEND_PACKET_SIZE; i++ )
		{
			printf("%02X ", arrSendPacket[i]);
		}
		printf("\r\n");
	}
#endif
}

BOOL BTRecvParsing(BYTE* pData, uint32_t nLength)
{
    if (!pData || nLength == 0) return FALSE;

    int nloopCnt = (int)(sizeof(g_tbBTCmdList) / sizeof(g_tbBTCmdList[0]));

    for (int i = 0; i < nloopCnt; i++) {
        const char *resp = g_tbBTCmdList[i].strResponseData;
        if (!resp) continue;

        if (strncmp((char*)pData, resp, strlen(resp)) == 0) {
            if (g_tbBTCmdList[i].fp) {
                g_tbBTCmdList[i].fp((uint8_t*)pData, nLength);
            }
            return TRUE;
        }

        if ((g_eBLECmdSending == i) &&
            (strncmp(ANY_DATA, resp, strlen(ANY_DATA)) == 0)) {
            if (g_tbBTCmdList[i].fp) {
                g_tbBTCmdList[i].fp((uint8_t*)pData, nLength);
            }
            return TRUE;
        }
    }
    return FALSE;

// 	int i, nloopCnt;
// 	// unsigned char arrTmp[BLUETOOTH_SEND_PACKET_SIZE]; mod.kks todo remove.
// 	stQueue *pQueue = (stQueue*)pData;

// 	#ifdef TEST_BT_UART
// {
// 	int i;
// 	printf("[recv]");
// 	for ( i=0; i<nLength; i++ )
// 	{
// 		printf("%02X ", pData[i]);
// 	}
// 	printf("\r\n");
// }
// #endif


// 	nloopCnt = sizeof(g_tbBTCmdList)/sizeof(g_tbBTCmdList[0]);

// 	for ( i=0; i<nloopCnt; i++ ) {
// 		if( g_tbBTCmdList[i].strResponseData == NULL )
// 		{
// 		}
// 		else if( (g_eBLECmdSending == i) && (( strncmp((char*)pQueue, (char*)g_tbBTCmdList[i].strResponseData,
// 						   strlen(g_tbBTCmdList[i].strResponseData)) == 0 ) ||
// 				 (strncmp(ANY_DATA, g_tbBTCmdList[i].strResponseData, strlen(ANY_DATA))) == 0) )
// 		{
// 			if( g_tbBTCmdList[i].fp != NULL )
// 			{
// 				g_tbBTCmdList[i].fp((uint8_t*)pQueue,nLength);
// 				//g_eBLECmdSending = eBLECmd_EvtInitNoti; // ???
// 				break;
// 			}
// 		}
// 	}

// 	return TRUE;
}

void BTSend_VehicleStatus(unsigned char ucVehicle_state)
{
	char arrTmp[3]; // 3byte ?? ???(?????)

	arrTmp[0] = 0x19;	// ???? service UUID? ???? ??.
	arrTmp[1] = 0x97;

	// if( ucVehicle_state >= eVEHICLE_STATE_ENGRUN )
	// 	arrTmp[2] = eAutolinkBluetootBeaconStatus_Wakeup;
	// else
	// 	arrTmp[2] = eAutolinkBluetootBeaconStatus_Sleep;

	g_eBLECmdSending = eBLECmd_SetSVCData;
	Make_n_SendToBnComUart(g_eBLECmdSending, arrTmp);
}

void ConversionHexStringToHex(char* pIn, char *pOut, int nInLength, int nRadix)
{
	int i, nOutIdx = 0;
	char arrHexString[3], *pEnd;
	arrHexString[2] = '\0';
	for ( i=0; i<nInLength; i+=2 )
	{
		strncpy(arrHexString, pIn+i, 2);
		pOut[nOutIdx++] = strtol(arrHexString, &pEnd, nRadix);
	}
}

/**
 * @brief  Test function to send BT commands from g_tbBTCmdList
 * @param  cmdIndex: eBTCmdIndex value to send
 * @note   For testing BLE module AT commands
 */
void BT_TestSendCommand(eBTCmdIndex cmdIndex)
{
    printf("[BT Test] Sending command index: %d\r\n", cmdIndex);
    
    if (cmdIndex == eBLECmd_SetSWReset) {
        // ATZ\r - Software Reset
        // printf("[BT Test] eBLECmd_SetSWReset - ATZ\r\n");
        Make_n_SendToBnComUart(eBLECmd_SetSWReset, NULL);
    }
    else if (cmdIndex == eBLECmd_GetVersion) {
        // AT+VER?\r - Get Firmware Version
        // printf("[BT Test] eBLECmd_GetVersion - AT+VER?\r\n");
        Make_n_SendToBnComUart(eBLECmd_GetVersion, NULL);
    }
    else if (cmdIndex == eBLECmd_GetBLE_Name) {
        // AT+BTNAME?\r - Get BLE Name
        // printf("[BT Test] eBLECmd_GetBLE_Name - AT+BTNAME?\r\n");
        Make_n_SendToBnComUart(eBLECmd_GetBLE_Name, NULL);
    }
    else if (cmdIndex == eBLECmd_SetBLE_Name) {
        // AT+BTNAME=%s\r - Set BLE Name (requires argument)
        // printf("[BT Test] eBLECmd_SetBLE_Name - AT+BTNAME=TestDevice\r\n");
        Make_n_SendToBnComUart(eBLECmd_SetBLE_Name, "TestDevice");
    }
    else if (cmdIndex == eBLECmd_BD_ADDR) {
        // AT+BTADDR?\r - Get BT Address
        // printf("[BT Test] eBLECmd_BD_ADDR - AT+BTADDR?\r\n");
        Make_n_SendToBnComUart(eBLECmd_BD_ADDR, NULL);
    }
    else if (cmdIndex == eBLECmd_ADVInterval) {
        // AT+ADVINTERVAL=6\r - Set Advertising Interval
        // printf("[BT Test] eBLECmd_ADVInterval - AT+ADVINTERVAL=6\r\n");
        Make_n_SendToBnComUart(eBLECmd_ADVInterval, NULL);
    }
    else if (cmdIndex == eBLECmd_ReMote_BD_ADDR) {
        // AT+REMOTEADDR\r - Get Remote BT Address
        // printf("[BT Test] eBLECmd_ReMote_BD_ADDR - AT+REMOTEADDR\r\n");
        Make_n_SendToBnComUart(eBLECmd_ReMote_BD_ADDR, NULL);
    }
    else if (cmdIndex == eBLECmd_GetAdvertiseState) {
        // AT+ADVSTATE?\r - Get Advertising State
        // printf("[BT Test] eBLECmd_GetAdvertiseState - AT+ADVSTATE?\r\n");
        Make_n_SendToBnComUart(eBLECmd_GetAdvertiseState, NULL);
    }
    else if (cmdIndex == eBLECmd_SetAdvertiseState) {
        // AT+ADVSTATE=%s\r - Set Advertising State (ON/OFF)
        // printf("[BT Test] eBLECmd_SetAdvertiseState - AT+ADVSTATE=ON\r\n");
        Make_n_SendToBnComUart(eBLECmd_SetAdvertiseState, "ON");
    }
    else if (cmdIndex == eBLECmd_GetSVCData) {
        // AT+SVCDATA?\r - Get Service Data
        // printf("[BT Test] eBLECmd_GetSVCData - AT+SVCDATA?\r\n");
        Make_n_SendToBnComUart(eBLECmd_GetSVCData, NULL);
    }
    else if (cmdIndex == eBLECmd_SetSVCData) {
        // AT+SVCDATA=%s\r - Set Service Data
        // printf("[BT Test] eBLECmd_SetSVCData - AT+SVCDATA=...\r\n");
        char testData[3] = {0x19, 0x97, 0x10};
        Make_n_SendToBnComUart(eBLECmd_SetSVCData, testData);
    }
    else {
        // printf("[BT Test] Unknown or unsupported command index: %d\r\n", cmdIndex);
    }
}

/*****************************END OF FILE*****/
