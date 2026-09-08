/**
  ******************************************************************************
  * @file    GIT_OemInterface.c
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    18-MAR-2014
  * @brief   Manager GIT_OemInterface.c module
  ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#include "GIT_OemInterface.h"
#include "GIT_Gps.h"
#include "GIT_VCI.h"
#include "GIT_Util.h"
#include "GIT_InterProtocol.h"
#include "GIT_CanParsingProc.h"
#include "cli.h"
#include "UARTDMA_Manager.h"
#include "OBD_Manager.h"
#include "Modem_Manager.h"
#include "Autolink_Manager.h"
#include "CanFD_Controller.h"

#include "HalHandler.h"
#include "HalUartDriver.h"
#include "HalCanDriver.h"
#include "HalLedDriver.h"
#include "HalAdcDriver.h"



extern stCFDControl m_stCFDCtrl;


#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
#include "Usbd_bulk_core.h"
#include "usb_trx.h"
#endif

//#define DEBUG_CAN_PACKET		//CANID 0700 이상만 보기
//#define DEBUG_CAN_PACKET_ALL
//#define RAW_UART8_PACKET_DEBUG	//PC _Selftest
//#define RAW_UART3_PACKET_DEBUG
#define RAW_UART2_PACKET_DEBUG

#ifdef RAW_UART2_PACKET_DEBUG
#include "Modem_Manager.h"
extern stModemProcess ModemProcess;
#endif

extern int g_nCANTransmitLedDelayCount;

extern stActuatorReqInfo			g_stActuatorReqInfo[40];	//Req ID와 ID별갯수
extern unsigned char g_ucReqIndexPos;
extern eOBD_STATE	g_eOBDState;
extern stCanIDCheckList g_stCanIDCheckList;
extern stSlaveHWSetInfo g_stControlHWSetInfo;
extern eACTUATOR_TYPE g_eActuatorType;	//구동종류
extern bool g_bCanParsingRunFlag;
extern bool g_bIndicatorDBFlag;
extern unsigned char g_ucSaveSlaveCanLine;
//extern stSlaveHWSetInfo g_stCH2PassMaskIDInfo;

#if defined(FEATURE_CAN1_USE)
extern stHalCANTX_STRUCT   g_CAN1_TxBuffCtrl;
extern stHalCANRX_STRUCT   g_CAN1_RxBuffCtrl;
#endif

#if defined(FEATURE_CAN2_USE)
extern stHalCANTX_STRUCT   g_CAN2_TxBuffCtrl;
extern stHalCANRX_STRUCT   g_CAN2_RxBuffCtrl;
#endif


extern uint8_t g_bEnableModemDirectCommunication;
extern uint8_t g_bEnableBLEDirectCommunication;
extern uint8_t g_bEnableGPSDirectCommunication;
extern uint8_t g_bEnableSELFTESTDirectCommunication;
extern uint8_t g_bEnableBLEViewRes;
extern uint8_t g_bEnableGPSViewRes;


void ParsingGPSNmeaProtocol(unsigned char* pBuff, unsigned int nCount, eCommType eWhatCommType);

void GITDebugPrintf(char *fmt,...)
{
	va_list ap;
	//char string[256];
	char string[1024];
	unsigned char uIndex = 0;
    stHalUartBuffCtrl *pDebugPort;

    if ( g_bHYPERTECSelftestFlag == true )  pDebugPort = &g_stUart8;
    else                                    pDebugPort = &g_stUart7;

#if defined(DEBUG_TIME_LOG)
{
	static unsigned long ulPrevTmr = 0;
	sprintf(string, "[%04d] ", Get_TmrDelta(Get_Tmr(),ulPrevTmr));
	ulPrevTmr = Get_Tmr();
	uIndex = 6;
}
#endif
	va_start(ap,fmt);
	vsprintf(string+uIndex,fmt,ap);

	string[1023] = '\0';

    UartWriteBuf(pDebugPort, (unsigned char*)string, strlen(string));


	va_end(ap);
}


unsigned long OemGetTmr(void)
{
	return Get_Tmr();
}

unsigned long OemGetTmrDelta(unsigned int uiNewTick, unsigned int uiOldTick)
{
	return Get_TmrDelta(uiNewTick, uiOldTick);
}



void OemGetRTCTime(unsigned char* pBuff)
{
	U8  cnt = 0;
    stHalRTCTypeDef stHalRtcDateTime;

    HalDrvRtcRead(eRtcBin, eRtcAll, (char*)&stHalRtcDateTime, sizeof(stHalRtcDateTime), 0);

	pBuff[cnt++] = stHalRtcDateTime.RtcDate.RTC_WeekDay;			// 0 -> NOT USED
	pBuff[cnt++] = stHalRtcDateTime.RtcDate.RTC_Year;					// 1	13
	pBuff[cnt++] = stHalRtcDateTime.RtcDate.RTC_Month;				// 2	10
	pBuff[cnt++] = stHalRtcDateTime.RtcDate.RTC_Date;					// 3	28
	pBuff[cnt++] = stHalRtcDateTime.RtcTime.RTC_Hours;			// 4
	pBuff[cnt++] = stHalRtcDateTime.RtcTime.RTC_Minutes;		// 5
	pBuff[cnt++] = stHalRtcDateTime.RtcTime.RTC_Seconds;		// 6
}

void Oem_GIT_mDelay(const uint32_t msec)
{
	unsigned long temp;

	temp = g_ul16timer_ms + msec;

	if (temp > g_ul16timer_ms)
		while(1) {
			if(g_ul16timer_ms >= temp) {
				break;
			}
		}
	else
	{
		temp = temp - msec;
		while(1) {
			if(g_ul16timer_ms - msec >= temp ) {
				break;
			}
		}
	}
}

void Oem_GIT_uDelay(const uint32_t usec)
{
	uint32_t nDelayCnt = 0, nDelayMod = 0;
	if ( usec != 0 )
	{
		nDelayCnt = usec/2500;
		nDelayMod = usec%2500;
		if ( nDelayCnt > 0 )
		{
			int i;
			for ( i=0; i<nDelayCnt; i++ )
			{
				// Oem_GIT_uDelay 6ms이상 동작하지 않는다.
				udelay(2500);
			}
		}

		if ( nDelayMod != 0 )
		{
			udelay(nDelayMod);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Make Orignal function to Function point type
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// DLC UART function pointer

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Uart function pointer
unsigned int OemWriteUart3Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	unsigned int uiWrittenWDLength = 0;

#if !defined(FEATURE_BOOTLOADER)
	unsigned int i;
	int nLoopCnt;
	unsigned int uiRemainLen=0, uiSendLen=0;

	uiRemainLen = nCount;
	nLoopCnt = (nCount / UART3_TXBUF_LEN) + 1;

#if defined(RAW_UART3_PACKET_DEBUG)
	GITDebugPrintf("[%s] BT TX(Len %d) : ", __FUNCTION__, nCount);
#endif

	for(i = 0; i < nLoopCnt; i++ ) {
		if ( uiRemainLen > UART3_TXBUF_LEN )
			uiSendLen = UART3_TXBUF_LEN;
		else
			uiSendLen = uiRemainLen;

		UartWriteBuf(&g_stUart3, pBuff + (UART3_TXBUF_LEN*i), uiSendLen);

#if defined(RAW_UART3_PACKET_DEBUG)
		{
			unsigned int  j;
			for ( j=0; j<uiSendLen; j++ )
				GITDebugPrintf("%02X ", pBuff[UART3_TXBUF_LEN*i+j]);
		}
#endif
        
        if(g_bEnableBLEViewRes == true) {
            unsigned int  j;
            GITDebugPrintf("[%s] BT TX(Len %d) : ", __FUNCTION__, nCount);
			for ( j=0; j<uiSendLen; j++ )
				GITDebugPrintf("%02X ", pBuff[UART3_TXBUF_LEN*i+j]);
        }
		uiWrittenWDLength += uiSendLen;
		uiRemainLen -= uiSendLen;
	}

#if defined(RAW_UART3_PACKET_DEBUG)
	GITDebugPrintf("\r\n\r\n");
#endif
    if(g_bEnableBLEViewRes == true) {
        GITDebugPrintf("\r\n\r\n");
    }
#endif
	return uiWrittenWDLength;
}

unsigned int OemReadUart3Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	int nRecvLen = 0;

#if !defined(FEATURE_BOOTLOADER)
	nRecvLen = UartReadBuf(&g_stUart3, pBuff, nCount);

#if defined(RAW_UART3_PACKET_DEBUG)
{
	int i;

	if ( nRecvLen > 0 )
	{
		GITDebugPrintf("\r\n[%s] BT nRecvLen %d\r\n", __FUNCTION__, nRecvLen);
		for ( i=0; i<nRecvLen; i++ )
			GITDebugPrintf("%02X ", pBuff[i]);
		GITDebugPrintf("\r\n");
	}
}
#endif

#endif
	return nRecvLen;
}

unsigned int OemReadUartDMA3Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	unsigned int nRecvLen = 0, nAvailableSize;
    uint8_t a;
//	uint16_t size = 0;

#if !defined(FEATURE_BOOTLOADER)
    nAvailableSize = UART3Available();
    if ( nAvailableSize > 0 )
    {

        if(g_bEnableBLEViewRes == true)
            GITDebugPrintf("[%s] BT RX : ", __FUNCTION__);
        

	for ( nRecvLen=0; nRecvLen<nAvailableSize; nRecvLen++ ) 
        {
			a = UART3_DMARead();

			if(g_bEnableBLEDirectCommunication == true) {
				printf("%02x ", a);
                pBuff[nRecvLen] = a;
			}
			else {
				if(g_bEnableBLEViewRes == true) {
					printf("%02x ", a);
				}

				pBuff[nRecvLen] = a;
			}
		}

                if(g_bEnableBLEViewRes == true) {
                     printf("\r\n");
                }
	}

#endif
	return nRecvLen;
}

unsigned int OemParsingUart3(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam)
{

#ifdef RF_COMMON_MODEM //mod.kks     21.10.21
    ParsingBnComProtocol(pBuff, nCount, (eCommType)wParam);
#else
	ParsingDCSProtocol(pBuff, nCount, (eCommType)wParam);
#endif
	return TRUE;
}

unsigned int OemWriteUart2Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	unsigned int uiWrittenWDLength = 0;

#if !defined(FEATURE_BOOTLOADER)
	unsigned int i;
	int nLoopCnt;
	unsigned int uiRemainLen=0, uiSendLen=0;

	uiRemainLen = nCount;
	nLoopCnt = (nCount / UART2_TXBUF_LEN) + 1;

#if defined(RAW_UART2_PACKET_DEBUG)
#if defined(RF_COMMON_MODEM)
	if(ModemProcess.modemProcessFunc != ProcessSendAGPSData)	// AGPS 데이터 표출 막음.
#endif
	printf("Modem TX (Len %d) : ", nCount);
#endif

	for(i = 0; i < nLoopCnt; i++) 
	{
		if(uiRemainLen > UART2_TXBUF_LEN)   uiSendLen = UART2_TXBUF_LEN;
		else                                uiSendLen = uiRemainLen;

        UartWriteBuf(&g_stUart2, pBuff + (UART2_TXBUF_LEN*i), uiSendLen);

#if defined(RAW_UART2_PACKET_DEBUG)
		{
			unsigned int  j;
			for ( j = 0; j < uiSendLen; j++ ) {
#if defined(RF_COMMON_MODEM)
				if( ModemProcess.modemProcessFunc != ProcessSendAGPSData )
#endif
					printf("%c", pBuff[UART2_TXBUF_LEN*i+j]);
			}
#if defined(RF_COMMON_MODEM)
			if( ModemProcess.modemProcessFunc != ProcessSendAGPSData )
#endif
				printf("\r\n");
		}
#endif

		uiWrittenWDLength += uiSendLen;
		uiRemainLen -= uiSendLen;
	}
#endif
	return uiWrittenWDLength;
}

unsigned int OemReadUart2Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	int nRecvLen = 0;

#if !defined(FEATURE_BOOTLOADER)
	nRecvLen = UartReadBuf(&g_stUart2, pBuff, nCount);

#if defined(RAW_UART2_PACKET_DEBUG)
{
	int i;

	if ( nRecvLen > 0 ) {
		GITDebugPrintf("\r\nRX :", __FUNCTION__, nRecvLen);
		for ( i=0; i<nRecvLen; i++ ) {
			GITDebugPrintf("%c", pBuff[i]);
		}
		GITDebugPrintf("\r\n");
	}
}
#endif
#endif
	return nRecvLen;
}

unsigned int OemReadUartDMA2Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	int nRecvLen = 0, nAvailableSize;

	uint8_t a;

#if !defined(FEATURE_BOOTLOADER)
    nAvailableSize = Uart2Available();
    if ( nAvailableSize > 0 )
	{
#if defined(RAW_UART2_PACKET_DEBUG)
		if( !(ModemManagerData.eCurrentMessageSendingType==eMESSAGE_APGS_DATA && ModemManagerData.ePreviousMessageSendingType==eMESSAGE_APGS_NONE))
	        printf("\r\nModem RX :", __FUNCTION__);
#endif

        if(g_bEnableModemDirectCommunication == true)
    		printf("\r\n[%s] DIRECT MODEM RX :", __FUNCTION__);

        for ( nRecvLen = 0; nRecvLen<nAvailableSize; nRecvLen++)
        {
            a = Uart2_DMARead();
            pBuff[nRecvLen] = a;
            if(g_bEnableModemDirectCommunication == true) 
                printf("%c", a);
            else
            {
#if defined(RAW_UART2_PACKET_DEBUG)
            if( !(ModemManagerData.eCurrentMessageSendingType==eMESSAGE_APGS_DATA && ModemManagerData.ePreviousMessageSendingType==eMESSAGE_APGS_NONE))
                printf("%c", a);
#endif
            }
        }
        
#if defined(RAW_UART2_PACKET_DEBUG)
        //printf("\r\n");
#endif
	}
#endif
	return nRecvLen;
}

unsigned int OemParsingUart2(unsigned char* pBuff, unsigned int	nCount, void* lParam, unsigned int wParam)
{
	ParsingGemaltoProtocol(pBuff, nCount, (eCommType)wParam);

	return TRUE;
}

//************************************************************************************************************
//#define RAW_GPS_PACKET_DEBUG //mod.kks todo //
unsigned int OemWriteUart4Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	unsigned int uiWrittenWDLength = 0;

#if !defined(FEATURE_BOOTLOADER)
	unsigned int i;
	int nLoopCnt;
	unsigned int uiRemainLen=0, uiSendLen=0;

	uiRemainLen = nCount;
	nLoopCnt = (nCount / UART4_TXBUF_LEN) + 1;

#if defined(RAW_GPS_PACKET_DEBUG)
	GITDebugPrintf("[%s] GPS UART TX(Len %d) : ", __FUNCTION__, nCount);
#endif

	for ( i=0; i<nLoopCnt; i ++ )
	{
		if ( uiRemainLen > UART4_TXBUF_LEN )
			uiSendLen = UART4_TXBUF_LEN;
		else
			uiSendLen = uiRemainLen;

		UartWriteBuf(&g_stUart4, pBuff + (UART4_TXBUF_LEN*i), uiSendLen);

#if defined(RAW_GPS_PACKET_DEBUG)
		{
			unsigned int  j;
			for ( j=0; j<uiSendLen; j++ )
				GITDebugPrintf("%02X ", pBuff[UART4_TXBUF_LEN*i+j]);
		}
#endif
		uiWrittenWDLength += uiSendLen;
		uiRemainLen -= uiSendLen;
	}

#if defined(RAW_GPS_PACKET_DEBUG)
				GITDebugPrintf("\r\n\r\n");
#endif

#endif
	return uiWrittenWDLength;
}

unsigned int OemReadUart4Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	int nRecvLen = 0;

#if !defined(FEATURE_BOOTLOADER)
	nRecvLen = UartReadBuf(&g_stUart4, pBuff, nCount);

#if defined(RAW_GPS_PACKET_DEBUG)
{
	int i;

	if ( nRecvLen > 0 )
	{
        GITDebugPrintf("[%s] GPS UART RX(Len %d) : ", __FUNCTION__, nRecvLen);
		for ( i=0; i<nRecvLen; i++ )
			GITDebugPrintf("%02X", pBuff[i]);
        GITDebugPrintf("\r\n\r\n");
	}
}
#endif
#endif
	return nRecvLen;
}

unsigned int OemReadUartDMA4Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	int nRecvLen = 0, nAvailableSize;

	uint8_t a;
//	uint16_t size = 0;

	//g_bEnableGPSViewRes =true; //mod.kks todo test remove.......
#if !defined(FEATURE_BOOTLOADER)
    nAvailableSize = UART4Available();
    if ( nAvailableSize > 0 )
    {
		for (nRecvLen=0; nRecvLen <nAvailableSize; nRecvLen++) {
				a = UART4_DMARead();
			if(g_bEnableGPSViewRes == true) {
				printf("%c", a);
			}

			pBuff[nRecvLen] = a;
		}

		return nRecvLen;
	}

#endif
	return nRecvLen;
}


unsigned int OemParsingUart4(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	ParsingGPSNmeaProtocol(pBuff, nCount, (eCommType)wParam);
	return TRUE;
}
//**********************************************************************************************************

//******************************************************************************
// Dev Selftest
//******************************************************************************
unsigned int OemWriteUart8Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	unsigned int uiWrittenWDLength = 0;

#if !defined(FEATURE_BOOTLOADER)
	unsigned int i;
	int nLoopCnt;
	unsigned int uiRemainLen=0, uiSendLen=0;

	uiRemainLen = nCount;
	nLoopCnt = (nCount / UART8_TXBUF_LEN) + 1;

#if defined(RAW_UART8_PACKET_DEBUG)
	GITDebugPrintf("[%s] SELFTEST UART TX(Len %d) : ", __FUNCTION__, nCount);
#endif

	for ( i=0; i<nLoopCnt; i ++ )
	{
		if ( uiRemainLen > UART8_TXBUF_LEN )
			uiSendLen = UART8_TXBUF_LEN;
		else
			uiSendLen = uiRemainLen;

		UartWriteBuf(&g_stUart8, pBuff + (UART8_TXBUF_LEN*i), uiSendLen);

#if defined(RAW_UART8_PACKET_DEBUG)
		{
			unsigned int  j;
			for ( j=0; j<uiSendLen; j++ )
				GITDebugPrintf("%02X ", pBuff[UART8_TXBUF_LEN*i+j]);
		}
#endif
		uiWrittenWDLength += uiSendLen;
		uiRemainLen -= uiSendLen;
	}

#if defined(RAW_UART8_PACKET_DEBUG)
				GITDebugPrintf("\r\n\r\n");
#endif

#endif
	return uiWrittenWDLength;
}

unsigned int OemReadUart8Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	int nRecvLen = 0;

#if !defined(FEATURE_BOOTLOADER)
	nRecvLen = UartReadBuf(&g_stUart8, pBuff, nCount);

#if defined(RAW_UART8_PACKET_DEBUG)
{
	int i;
	if ( nRecvLen > 0 )
	{
		GITDebugPrintf("[%s] PLUS UART nRecvLen %d\r\n", __FUNCTION__, nRecvLen);
		for ( i=0; i<nRecvLen; i++ )
			GITDebugPrintf("%02X ", pBuff[i]);
		GITDebugPrintf("\r\n");
	}
}
#endif
#endif
	return nRecvLen;
}

unsigned int OemReadUartDMA8Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	int nRecvLen = 0;

	uint8_t a;
	uint16_t nAvailableSize = 0;

#if !defined(FEATURE_BOOTLOADER)
    nAvailableSize = Uart8Available();
    if ( nAvailableSize > 0 ) {
        if(g_bEnableSELFTESTDirectCommunication == true) {printf("[%s] SELFTEST UART\r\n", __FUNCTION__);}

        for ( nRecvLen = 0; nRecvLen<nAvailableSize; nRecvLen++)
        {
            a = Uart8_DMARead();
			if(g_bEnableSELFTESTDirectCommunication == true) {
                    printf("%02X ", a);
                }
            pBuff[nRecvLen] = a;
        }

		if(g_bEnableSELFTESTDirectCommunication == true)	printf("\r\n");

		return nRecvLen;
	}

#endif
	return nRecvLen;
}

unsigned int OemParsingUart8(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam)
{
	ParsingGITProtocol_SELFTEST(pBuff, nCount, (eCommType)wParam);
	return TRUE;
}
//******************************************************************************


////////////////////////////////////////////////////////////////////////////////
// USB function pointer
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
unsigned int OemWriteUsbBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	unsigned int uiWrittenUsbTotalLength = 0;

	int i;
	int nLoopCnt;
	unsigned int uiWrittenUsbLength = 0, uiRemainLen, uiSendLen;

	uiRemainLen = nCount;
	nLoopCnt = (nCount / USB_USR_TXBUF_SIZE) + 1;

//#if defined(USB_PACKET_DEBUG)
	GITDebugPrintf("\r\n[%s] usb Write packet :nCount %d, uiRemainLen %d, nLoopCnt %d\r\n", __FUNCTION__, nCount, uiRemainLen, nLoopCnt);
//#endif

	for ( i = 0; i < nLoopCnt; i ++ ) {
		if ( uiRemainLen > USB_USR_TXBUF_SIZE )
			uiSendLen = USB_USR_TXBUF_SIZE;
		else
			uiSendLen = uiRemainLen;

		uiWrittenUsbLength = Usb_Write(pBuff + (USB_USR_TXBUF_SIZE*i), uiSendLen);

//#if defined(USB_PACKET_DEBUG)
		{
			for (unsigned int j=0; j < uiWrittenUsbLength; j++ )
				GITDebugPrintf("%02X ", pBuff[((USB_USR_TXBUF_SIZE*i) + j)]);

			GITDebugPrintf("\r\n\r\n");
		}
//#endif
		uiRemainLen -= uiWrittenUsbLength;
	}
	return uiWrittenUsbTotalLength;
}

unsigned int OemReadUsbBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	unsigned int uiRecvCnt = 0;

	uiRecvCnt = Usb_Read(pBuff, nCount);

#if defined(USB_PACKET_DEBUG)
	if ( uiRecvCnt )
	{
		int i;

		printf("\r\n[%s] usb Read packet : %d\r\n", __FUNCTION__, uiRecvCnt);

		for ( i=0; i<uiRecvCnt; i++ ) {
			printf("%02X ", pBuff[i]);
		}
		printf("\r\n");
	}
#endif
	return uiRecvCnt;
}

unsigned int OemParsingUsb(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	ParsingGITProtocol(pBuff, nCount, (eCommType)wParam);
	return TRUE;
}

unsigned int OemCheckUsbReadBuff(void)
{
	return ChkUsbRx_Buf();
}
#endif
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// CAN function pointer
unsigned int OemWriteCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	stCanPacket *pOutCanPacket = (stCanPacket*)pBuff;
	unsigned char ucCanSentLen = 0;
	unsigned int uiCanId;

	stHalCANTX_STRUCT *cantx;

	cantx = &g_CAN1_TxBuffCtrl;

	if ( wParam != 1 )
		cantx = &g_CAN2_TxBuffCtrl;

	if ( !pOutCanPacket->stNormalPacket.ucIDE )
	{
		// standard
		uiCanId = pOutCanPacket->stNormalPacket.us11BitID;
		HalCanSet_Std(cantx, uiCanId);
		//ucCanSentLen += 2/*CAN ID*/;
		//HalCan_Tx(cantx, pOutCanPacket->stNormalPacket.arrDataFields, CAN_FRAME_DATA_SIZE);		//원본	data 영역이 무조건 8일 수 없다
		HalCan_Tx(cantx, pOutCanPacket->stNormalPacket.arrDataFields, pOutCanPacket->stNormalPacket.ucDLC);
		ucCanSentLen = pOutCanPacket->stNormalPacket.ucDLC;
	}
	else
	{
		// extended
		uiCanId = pOutCanPacket->stExtendPacket.us11BitID<<18 | pOutCanPacket->stExtendPacket.us18BitID;
		HalCanSet_Ext(cantx, uiCanId);
		//ucCanSentLen += 4/*CAN ID*/;
		//HalCan_Tx(cantx, pOutCanPacket->stExtendPacket.arrDataFields, CAN_FRAME_DATA_SIZE);		//원본	data 영역이 무조건 8일 수 없다
		HalCan_Tx(cantx, pOutCanPacket->stExtendPacket.arrDataFields, pOutCanPacket->stExtendPacket.ucDLC);
		ucCanSentLen = pOutCanPacket->stExtendPacket.ucDLC;
	}

#if defined(DEBUG_CAN_PACKET)
{
		int i;
		unsigned char *pTmp;

		if ( !pOutCanPacket->stNormalPacket.ucIDE )
		{
			printf("[T]Standard CAN[%d]: Len %d %04X ", (cantx==&g_CAN1_TxBuffCtrl) ? 1:2, ucCanSentLen, uiCanId);
			pTmp = pOutCanPacket->stNormalPacket.arrDataFields;
		}
		else
		{
			printf("[T]Extend CAN[%d]: Len %d %04X ", (cantx==&g_CAN1_TxBuffCtrl) ? 1:2, ucCanSentLen, uiCanId);
			pTmp = pOutCanPacket->stExtendPacket.arrDataFields;
		}

		for ( i=0; i<ucCanSentLen+1/*DATA LEHGTN*/; i++ )
			printf("%02X ", pTmp[i]);

		printf("\r\n");
}
#endif
	return ucCanSentLen;
}

extern int g_bCAN1OK;
extern int g_bCAN2OK;

unsigned int OemClearCanBuff(unsigned int wParam)
{
	if ( wParam == eCOMM_TYPE_CAN1) {
		HalCan_DisableRxInterrupt((unsigned int)HAL_CAN1);
		HalCan_ClearBuffer(eCOMM_TYPE_CAN1);
		ClearQueue(g_stGitCommInfo[eCOMM_TYPE_CAN1].pstInQueue);
		HalCan_EnableRxInterrupt((unsigned int)HAL_CAN1);
		return HalCan_GetRxQueueCount(&g_CAN1_RxBuffCtrl);
	}
	else {
		HalCan_DisableRxInterrupt((unsigned int)HAL_CAN2);
		HalCan_ClearBuffer(eCOMM_TYPE_CAN2);
		ClearQueue(g_stGitCommInfo[eCOMM_TYPE_CAN2].pstInQueue);
		HalCan_EnableRxInterrupt((unsigned int)HAL_CAN2);
		return HalCan_GetRxQueueCount(&g_CAN2_RxBuffCtrl);
	}
}

/*
#define CAN_ID_MERGE_MAX_CNT	100
typedef __packed struct _stCanIDMergeList{
	int8_t  	 ucMaskCount;
	unsigned int nStartMask[CAN_ID_MERGE_MAX_CNT];
	unsigned int nEndMask[CAN_ID_MERGE_MAX_CNT];
	unsigned char ucLineIndex[CAN_ID_MERGE_MAX_CNT];
}stCanIDMergeList;
*/


bool PassCanFDData(unsigned short usInputID , uint16_t usCanLine)
{
	//m_stCFDCtrl.stCanIDMerge.nStartMask[i],m_stCFDCtrl.stCanIDMerge.ucLineIndex[i]

	bool bRet=false;
	int i=0;

	for( i=0; i<m_stCFDCtrl.stCanIDMerge.ucMaskCount; i++ )
	{
		if( usInputID == m_stCFDCtrl.stCanIDMerge.nStartMask[i] && usCanLine == (m_stCFDCtrl.stCanIDMerge.ucLineIndex[i] % CANFD_SPI_DEFAULT ))
		{
//				printf("%04X\r\n",usInputID);
			return true;
		}
		else
		{
			bRet = false;
		}
	}
//	for( i=0; i<g_stCH2PassMaskIDInfo.m_uiMaskCount; i++ )
//	{
//		if( usInputID == g_stCH2PassMaskIDInfo.m_uiStartMask[i] )
//		{
////				printf("%04X\r\n",usInputID);
//			return true;
//		}
//		else
//		{
//			bRet = false;
//		}
//	}
	if(usInputID >= 0x0700) return true;
	return bRet;
}


bool PassCanIDCheck(stHalCanRxMsg rxmsg)
{
	bool bRet=false;
	unsigned int uiCanID;
	int i=0;

	if(rxmsg.IDE == HAL_CAN_ID_STD)
	{
		uiCanID = rxmsg.StdId;
		if(uiCanID >= 0x0700)	return true;
	}
	else
	{
		uiCanID = rxmsg.ExtId;
		if((uiCanID >= 0x18DA0000) && (uiCanID <= 0x18DAFFFF)) return true;
	}

	for( i=0; i<g_stCanIDCheckList.m_uiMaskCount; i++ )
	{
		if( uiCanID == g_stCanIDCheckList.m_uiStartMask[i] )
		{
//			printf("%04X\r\n",usInputID);
			return true;
		}
		else
		{
			bRet = false;
		}
	}
	
	return bRet;
}


#define CAN_FD_CONVERT_MASK 0xF000

bool CFD_ConvertPacketFormat( stHalCanRxMsg* rxmsg, uint16_t* pusCanLine, unsigned int wParam)
{
	if( rxmsg->IDE == HAL_CAN_ID_EXT )
	{
		if(CFD_GetCanFDAdapter())
		{
			// Canline will be 1/2/3/4
			*pusCanLine = ((rxmsg->ExtId&CAN_FD_CONVERT_MASK)>>12)&0xFFFF;

			rxmsg->IDE = HAL_CAN_ID_STD;
			rxmsg->StdId = rxmsg->ExtId&(~CAN_FD_CONVERT_MASK);
		}
		else
		{
			*pusCanLine = wParam;
		}
		//printf("id : %x, new id : %x, canline : %d\n", rxmsg->ExtId, rxmsg->StdId, *pusCanLine);

		//if(rxmsg->StdId == 0x0100 || rxmsg->StdId == 0x00B5)
		//{
			//printf("id : %x, new id : %x, canline : %d\n", rxmsg->ExtId, rxmsg->StdId, *pusCanLine);
			//hexdump(rxmsg->Data,8);
		//}
		return true;
	}
	else
	{
		*pusCanLine = wParam;	// CFAM-101이 없는 경우는 LINE이 CH1, CH2 로만 구분 가능하다.
		//*pusCanLine = g_ucSaveSlaveCanLine % CANFD_SPI_DEFAULT;
        //rxmsg->ExtId = ((*pusCanLine)<<12);
		return false;
	}
}

unsigned int OemReadCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	unsigned int nRxLen = 0;
	//unsigned int nTotalRxLen = 0;
	int i=0;
	stCanPacket RxCANPacket;
	stHalCanRxMsg rxmsg;
	uint16_t usCanLine = 0;
    RxCANPacket.stNormalPacket.us11BitID = 0;

        //memset(&RxCANPacket,0x00,sizeof(stCanPacket));

	if( g_bIndicatorDBFlag == true )
	{
		if ( wParam == eCOMM_TYPE_CAN1)
		{
			for ( i =0; i<HalCan_GetRxQueueCount(&g_CAN1_RxBuffCtrl); i++)
			{
				if (HalCan1_Read(&rxmsg))
				{
					// LED blink
					if ( g_nCANTransmitLedDelayCount == 0 )
						g_nCANTransmitLedDelayCount = MAX_CAN_LED_DELAY_CNT;
					if(g_bYUJINSelftestFlag != true){
						SetCANTransmitLedOnOff();
					}

					if( CFD_ConvertPacketFormat(&rxmsg,&usCanLine, wParam) && CFD_GetCanFDAdapter() )	//CFD_ConvertPacketFormat(&rxmsg,&usCanLine, wParam) 가 먼저 수행되어야 usCanLine 정보를 얻을 수 있다
					{
						if(PassCanFDData(rxmsg.StdId,usCanLine) == false){}
						else
						{
							if ( rxmsg.IDE == HAL_CAN_ID_STD )
							{
								RxCANPacket.stNormalPacket.ucSOF 					= 1;
								RxCANPacket.stNormalPacket.us11BitID				= rxmsg.StdId;
								RxCANPacket.stNormalPacket.ucRTR					= rxmsg.RTR;
								RxCANPacket.stNormalPacket.ucIDE					= CAN_FRAME_STANDARD_IDE;
								RxCANPacket.stNormalPacket.ucDummy2					= (unsigned char)usCanLine;
								RxCANPacket.stNormalPacket.ucDLC					= rxmsg.DLC;
								memcpy(RxCANPacket.stNormalPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
								RxCANPacket.stNormalPacket.usCRC					= 0;
								RxCANPacket.stNormalPacket.ucCRCDelimiter = 0;
								RxCANPacket.stNormalPacket.ucACK					= 0;
								RxCANPacket.stNormalPacket.ucACKDelimiter = 0;
								RxCANPacket.stNormalPacket.ucEOF					= CAN_FRAME_EOF;
								RxCANPacket.stNormalPacket.ucDummy1                 = 0;
								RxCANPacket.stNormalPacket.ucReserved               = 0;
							}
							else
							{
								RxCANPacket.stExtendPacket.ucSOF 					= 1;
								RxCANPacket.stExtendPacket.us11BitID				= rxmsg.ExtId >> 18;
								RxCANPacket.stExtendPacket.ucSRR					= 1;
								RxCANPacket.stExtendPacket.ucIDE					= CAN_FRAME_EXTEND_IDE;
								RxCANPacket.stExtendPacket.us18BitID				= rxmsg.ExtId;
								RxCANPacket.stExtendPacket.ucRTR					= rxmsg.RTR;
								RxCANPacket.stExtendPacket.ucReserved				= 0;
								RxCANPacket.stExtendPacket.ucDLC					= rxmsg.DLC;
								RxCANPacket.stExtendPacket.ucDummy1                 = (unsigned char)usCanLine;
								memcpy(RxCANPacket.stExtendPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
								RxCANPacket.stExtendPacket.usCRC					= 0;
								RxCANPacket.stExtendPacket.ucCRCDelimiter 			= 0;
								RxCANPacket.stExtendPacket.ucACK					= 0;
								RxCANPacket.stExtendPacket.ucACKDelimiter 			= 0;
								RxCANPacket.stExtendPacket.ucEOF					= CAN_FRAME_EOF;
							}
							nRxLen = sizeof(RxCANPacket);
                            memcpy(pBuff, &RxCANPacket, nRxLen);
                            break;
						}
					}
					else
					{
					  	if(PassCanIDCheck(rxmsg)==false){}
						else
						{
							if ( rxmsg.IDE == HAL_CAN_ID_STD )
							{
								RxCANPacket.stNormalPacket.ucSOF 					= 1;
								RxCANPacket.stNormalPacket.us11BitID				= rxmsg.StdId;
								RxCANPacket.stNormalPacket.ucRTR					= rxmsg.RTR;
								RxCANPacket.stNormalPacket.ucIDE					= CAN_FRAME_STANDARD_IDE;
								RxCANPacket.stNormalPacket.ucDummy2					= (unsigned char)usCanLine;
								RxCANPacket.stNormalPacket.ucDLC					= rxmsg.DLC;
								memcpy(RxCANPacket.stNormalPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
								RxCANPacket.stNormalPacket.usCRC					= 0;
								RxCANPacket.stNormalPacket.ucCRCDelimiter = 0;
								RxCANPacket.stNormalPacket.ucACK					= 0;
								RxCANPacket.stNormalPacket.ucACKDelimiter = 0;
								RxCANPacket.stNormalPacket.ucEOF					= CAN_FRAME_EOF;
								RxCANPacket.stNormalPacket.ucDummy1                 = 0;
								RxCANPacket.stNormalPacket.ucReserved               = 0;
							}
							else
							{
								RxCANPacket.stExtendPacket.ucSOF 					= 1;
								RxCANPacket.stExtendPacket.us11BitID				= rxmsg.ExtId >> 18;
								RxCANPacket.stExtendPacket.ucSRR					= 1;
								RxCANPacket.stExtendPacket.ucIDE					= CAN_FRAME_EXTEND_IDE;
								RxCANPacket.stExtendPacket.us18BitID				= rxmsg.ExtId;
								RxCANPacket.stExtendPacket.ucRTR					= rxmsg.RTR;
								RxCANPacket.stExtendPacket.ucReserved				= 0;
								RxCANPacket.stExtendPacket.ucDLC					= rxmsg.DLC;
								RxCANPacket.stExtendPacket.ucDummy1 				= (unsigned char)usCanLine;
								memcpy(RxCANPacket.stExtendPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
								RxCANPacket.stExtendPacket.usCRC					= 0;
								RxCANPacket.stExtendPacket.ucCRCDelimiter 			= 0;
								RxCANPacket.stExtendPacket.ucACK					= 0;
								RxCANPacket.stExtendPacket.ucACKDelimiter 			= 0;
								RxCANPacket.stExtendPacket.ucEOF					= CAN_FRAME_EOF;
							}
							nRxLen = sizeof(RxCANPacket);
							memcpy(pBuff, &RxCANPacket, nRxLen);
                            break;
						}
					}
					g_bCAN1OK = true;
				}
			}
		}
		else
		{
			for ( i =0; i<HalCan_GetRxQueueCount(&g_CAN2_RxBuffCtrl); i++)
			{
				if (HalCan2_Read(&rxmsg))
				{
					// LED blink
					if ( g_nCANTransmitLedDelayCount == 0 )
						g_nCANTransmitLedDelayCount = MAX_CAN_LED_DELAY_CNT;
					if(g_bYUJINSelftestFlag != true){
						SetCANTransmitLedOnOff();
					}

					if( CFD_ConvertPacketFormat(&rxmsg,&usCanLine, wParam) && CFD_GetCanFDAdapter() )
					{
						if(PassCanFDData(rxmsg.StdId,usCanLine) == false){}
						else
						{
							if ( rxmsg.IDE == HAL_CAN_ID_STD )
							{
								RxCANPacket.stNormalPacket.ucSOF 					= 1;
								RxCANPacket.stNormalPacket.us11BitID				= rxmsg.StdId;
								RxCANPacket.stNormalPacket.ucRTR					= rxmsg.RTR;
								RxCANPacket.stNormalPacket.ucIDE					= CAN_FRAME_STANDARD_IDE;
								RxCANPacket.stNormalPacket.ucDummy2					= (unsigned char)usCanLine;
								RxCANPacket.stNormalPacket.ucDLC					= rxmsg.DLC;
								memcpy(RxCANPacket.stNormalPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
								RxCANPacket.stNormalPacket.usCRC					= 0;
								RxCANPacket.stNormalPacket.ucCRCDelimiter = 0;
								RxCANPacket.stNormalPacket.ucACK					= 0;
								RxCANPacket.stNormalPacket.ucACKDelimiter = 0;
								RxCANPacket.stNormalPacket.ucEOF					= CAN_FRAME_EOF;
								RxCANPacket.stNormalPacket.ucDummy1                 = 0;
								RxCANPacket.stNormalPacket.ucReserved               = 0;
							}
							else
							{
								RxCANPacket.stExtendPacket.ucSOF 					= 1;
								RxCANPacket.stExtendPacket.us11BitID				= rxmsg.ExtId >> 18;
								RxCANPacket.stExtendPacket.ucSRR					= 1;
								RxCANPacket.stExtendPacket.ucIDE					= CAN_FRAME_EXTEND_IDE;
								RxCANPacket.stExtendPacket.us18BitID				= rxmsg.ExtId;
								RxCANPacket.stExtendPacket.ucRTR					= rxmsg.RTR;
								RxCANPacket.stExtendPacket.ucReserved				= 0;
								RxCANPacket.stExtendPacket.ucDLC					= rxmsg.DLC;
								RxCANPacket.stExtendPacket.ucDummy1 				= (unsigned char)usCanLine;
								memcpy(RxCANPacket.stExtendPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
								RxCANPacket.stExtendPacket.usCRC					= 0;
								RxCANPacket.stExtendPacket.ucCRCDelimiter 			= 0;
								RxCANPacket.stExtendPacket.ucACK					= 0;
								RxCANPacket.stExtendPacket.ucACKDelimiter 			= 0;
								RxCANPacket.stExtendPacket.ucEOF					= CAN_FRAME_EOF;
							}
							nRxLen = sizeof(RxCANPacket);
							memcpy(pBuff, &RxCANPacket, nRxLen);
                            break;
						}
					}
					else
					{
					  	if(PassCanIDCheck(rxmsg)==false){}
						else
						{
							if ( rxmsg.IDE == HAL_CAN_ID_STD )
							{
								RxCANPacket.stNormalPacket.ucSOF 					= 1;
								RxCANPacket.stNormalPacket.us11BitID				= rxmsg.StdId;
								RxCANPacket.stNormalPacket.ucRTR					= rxmsg.RTR;
								RxCANPacket.stNormalPacket.ucIDE					= CAN_FRAME_STANDARD_IDE;
								RxCANPacket.stNormalPacket.ucDummy2					= (unsigned char)usCanLine;
								RxCANPacket.stNormalPacket.ucDLC					= rxmsg.DLC;
								memcpy(RxCANPacket.stNormalPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
								RxCANPacket.stNormalPacket.usCRC					= 0;
								RxCANPacket.stNormalPacket.ucCRCDelimiter = 0;
								RxCANPacket.stNormalPacket.ucACK					= 0;
								RxCANPacket.stNormalPacket.ucACKDelimiter = 0;
								RxCANPacket.stNormalPacket.ucEOF					= CAN_FRAME_EOF;
								RxCANPacket.stNormalPacket.ucDummy1                 = 0;
								RxCANPacket.stNormalPacket.ucReserved               = 0;
							}
							else
							{
								RxCANPacket.stExtendPacket.ucSOF 					= 1;
								RxCANPacket.stExtendPacket.us11BitID				= rxmsg.ExtId >> 18;
								RxCANPacket.stExtendPacket.ucSRR					= 1;
								RxCANPacket.stExtendPacket.ucIDE					= CAN_FRAME_EXTEND_IDE;
								RxCANPacket.stExtendPacket.us18BitID				= rxmsg.ExtId;
								RxCANPacket.stExtendPacket.ucRTR					= rxmsg.RTR;
								RxCANPacket.stExtendPacket.ucReserved				= 0;
								RxCANPacket.stExtendPacket.ucDLC					= rxmsg.DLC;
								RxCANPacket.stExtendPacket.ucDummy1 				= (unsigned char)usCanLine;
								memcpy(RxCANPacket.stExtendPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
								RxCANPacket.stExtendPacket.usCRC					= 0;
								RxCANPacket.stExtendPacket.ucCRCDelimiter 			= 0;
								RxCANPacket.stExtendPacket.ucACK					= 0;
								RxCANPacket.stExtendPacket.ucACKDelimiter 			= 0;
								RxCANPacket.stExtendPacket.ucEOF					= CAN_FRAME_EOF;
							}
							nRxLen = sizeof(RxCANPacket);
							memcpy(pBuff, &RxCANPacket, nRxLen);
                            break;
						}
					}
					g_bCAN2OK = true;
				}
			}
		}
	}
	else	//경고등 DB가 아닌경우
	{
		if ( wParam == eCOMM_TYPE_CAN1) {
			if (!HalCan1_Read(&rxmsg))
				return nRxLen;
		}
		else {
			if (!HalCan2_Read(&rxmsg))
				return nRxLen;
		}

		// LED blink
		if ( g_nCANTransmitLedDelayCount == 0 )
			g_nCANTransmitLedDelayCount = MAX_CAN_LED_DELAY_CNT;
		if(g_bYUJINSelftestFlag != true){
			SetCANTransmitLedOnOff();
		}

		if ( wParam == eCOMM_TYPE_CAN1) {
			g_bCAN1OK = true;
		}
		else {
			g_bCAN2OK = true;
		}

		if(g_eActuatorType ==  ACTUATOR_TYPE_WAKEUP){}	//웨이크업때는 아무신호나 봐야해서 ID체크안해줌
		else
		{
			// convert extension to standard
			CFD_ConvertPacketFormat(&rxmsg,&usCanLine, wParam);
			if(PassCanIDCheck(rxmsg)==false) return 0;
		}

		if ( rxmsg.IDE == HAL_CAN_ID_STD ) {
			RxCANPacket.stNormalPacket.ucSOF 					= 1;
			RxCANPacket.stNormalPacket.us11BitID			    = rxmsg.StdId;
			RxCANPacket.stNormalPacket.ucRTR					= rxmsg.RTR;
			RxCANPacket.stNormalPacket.ucIDE					= CAN_FRAME_STANDARD_IDE;
			RxCANPacket.stNormalPacket.ucDummy2					= (unsigned char)usCanLine;
			RxCANPacket.stNormalPacket.ucDLC					= rxmsg.DLC;
			memcpy(RxCANPacket.stNormalPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
			RxCANPacket.stNormalPacket.usCRC					= 0;
			RxCANPacket.stNormalPacket.ucCRCDelimiter           = 0;
			RxCANPacket.stNormalPacket.ucACK					= 0;
			RxCANPacket.stNormalPacket.ucACKDelimiter           = 0;
			RxCANPacket.stNormalPacket.ucEOF					= CAN_FRAME_EOF;
			RxCANPacket.stNormalPacket.ucDummy1                 = 0;
			RxCANPacket.stNormalPacket.ucReserved               = 0;
		}
		else {
			RxCANPacket.stExtendPacket.ucSOF 					= 1;
			RxCANPacket.stExtendPacket.us11BitID			    = rxmsg.ExtId >> 18;
			RxCANPacket.stExtendPacket.ucSRR					= 1;
			RxCANPacket.stExtendPacket.ucIDE					= CAN_FRAME_EXTEND_IDE;
			RxCANPacket.stExtendPacket.us18BitID			    = rxmsg.ExtId;
			RxCANPacket.stExtendPacket.ucRTR					= rxmsg.RTR;
			RxCANPacket.stExtendPacket.ucReserved			    = 0;
			RxCANPacket.stExtendPacket.ucDLC					= rxmsg.DLC;
			RxCANPacket.stExtendPacket.ucDummy1 				= (unsigned char)usCanLine;
			memcpy(RxCANPacket.stExtendPacket.arrDataFields, rxmsg.Data, rxmsg.DLC);
			RxCANPacket.stExtendPacket.usCRC					= 0;
			RxCANPacket.stExtendPacket.ucCRCDelimiter           = 0;
			RxCANPacket.stExtendPacket.ucACK					= 0;
			RxCANPacket.stExtendPacket.ucACKDelimiter           = 0;
			RxCANPacket.stExtendPacket.ucEOF					= CAN_FRAME_EOF;
		}

		nRxLen = sizeof(RxCANPacket);
		memcpy(pBuff, &RxCANPacket, nRxLen);
	}
    if(RxCANPacket.stNormalPacket.us11BitID == 0) return 0;


#if defined(DEBUG_CAN_PACKET)
	{
#if defined(DEBUG_CAN_PACKET_ALL)
		unsigned char* pTmp;
		int i, nRecvCanLen;

		if (rxmsg.IDE == HAL_CAN_ID_STD) {
			if ( wParam == eCOMM_TYPE_CAN1) {
				printf("\n[%s] rcv std High CAN[%d] : CAN Len %d\r\n %04X ", __FUNCTION__, wParam, RxCANPacket.stNormalPacket.ucDLC, RxCANPacket.stNormalPacket.us11BitID);
			}
			else {
				printf("\n[%s] rcv std Low CAN[%d] : CAN Len %d\r\n %04X ", __FUNCTION__, wParam, RxCANPacket.stNormalPacket.ucDLC, RxCANPacket.stNormalPacket.us11BitID);
			}
			pTmp = RxCANPacket.stNormalPacket.arrDataFields;
			nRecvCanLen = RxCANPacket.stNormalPacket.ucDLC;
		}
		else {
			printf("\r\n\r\n[%s] rcv ext CAN[%d] : CAN Len %d\r\n %04X ", __FUNCTION__, wParam, RxCANPacket.stExtendPacket.ucDLC, ((RxCANPacket.stExtendPacket.us11BitID<<18) | RxCANPacket.stExtendPacket.us18BitID));
			pTmp = RxCANPacket.stExtendPacket.arrDataFields;
			nRecvCanLen = RxCANPacket.stExtendPacket.ucDLC;
		}

		if ( wParam == eCOMM_TYPE_CAN1) {
			for ( i = 0; i < nRecvCanLen; i++ ) {
				printf("%02X ", pTmp[i]);
			}
			printf("\r\n");
		}
		else {
			for ( i = 0; i < nRecvCanLen; i++ ) {
				printf("%02X ", pTmp[i]);
			}
			printf("\r\n");
		}
#else
		unsigned char* pTmp;
		int i, nRecvCanLen;

		pTmp = RxCANPacket.stNormalPacket.arrDataFields;
		nRecvCanLen = RxCANPacket.stNormalPacket.ucDLC;

		if(RxCANPacket.stNormalPacket.us11BitID > 0x0700)
		{
			if ( wParam == eCOMM_TYPE_CAN1) {
				printf("[R] %d H_CAN[%d] Len:%d %04X ", DCAN_GET_COMM_STATE(), wParam, RxCANPacket.stNormalPacket.ucDLC, RxCANPacket.stNormalPacket.us11BitID);
			}
			else {
				printf("[R] %d L_CAN[%d] Len:%d %04X ", LCAN_GET_COMM_STATE(), wParam, RxCANPacket.stNormalPacket.ucDLC, RxCANPacket.stNormalPacket.us11BitID);
			}

			for ( i = 0; i < nRecvCanLen; i++ ) {
				printf("%02X ", pTmp[i]);
			}
			printf("\r\n");
		}
#endif
	}

#endif

	return nRxLen;
}

unsigned int OemParsingCan(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam)
{
	if(wParam == eCOMM_TYPE_CAN1)
	{
		if(CFD_GetCanFDAdapter() && m_stCFDCtrl.unCh1CanLine == Highcan2)
		{
			CANFD_RxParsing(pBuff, nCount, lParam, wParam);
		}
		else
		{
            CAN_RxParsing(pBuff, nCount, lParam, wParam);
		}
	}
	else
	{
		if(CFD_GetCanFDAdapter() && m_stCFDCtrl.unCh2CanLine == Highcan3 )
		{
			CANFD_RxParsing_Low(pBuff, nCount, lParam, wParam);
		}
		else
		{
            CAN_RxParsing_Low(pBuff, nCount, lParam, wParam);
        }
	}

	return TRUE;
}

extern bool g_bAutoVINRunFlag;

void Oem_CAN_None_Receive_Set()
{
	unsigned int uiStartMask[HAL_CAN_MAX_MASK_CNT];
	unsigned int uiEndMask[HAL_CAN_MAX_MASK_CNT];

	if(CFD_GetCanFDAdapter())
    {
    	m_stCFDCtrl.unCh1CanLine = 0;
		m_stCFDCtrl.unCh2CanLine = 0;
		m_stCFDCtrl.unChFinalCanLine = 0;
	}

	m_stCFDCtrl.unCh1CanLine = m_stCFDCtrl.unCh2CanLine = 0;

	memset(uiStartMask,0x00,sizeof(uiStartMask));
	memset(uiEndMask,0x00,sizeof(uiEndMask));
	Oem_CAN1_STANDBY_ACTIVE();	// TJA1042STANDBY모드
	Oem_CAN2_STANDBY_ACTIVE();	//
	while(HalCan_GetRxQueueCount(&g_CAN1_RxBuffCtrl) != 0)
	{
		OemClearCanBuff(eCOMM_TYPE_CAN1);
	}
	while(HalCan_GetRxQueueCount(&g_CAN2_RxBuffCtrl) != 0)
	{
		OemClearCanBuff(eCOMM_TYPE_CAN2);
	}
}

unsigned char Oem_CAN_Channel_Masket_Set(unsigned char nChannel, unsigned char nCANIDType, unsigned char nMaskNum, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue)
{
	u8 i, startFilterNum, endFilterNum;
	u32 startMaskVal, endMaskVal;
	stHalCAN_FilterInitTypeDef  CAN_Ch_FilterInitStructure;
    unsigned int unCanChAddress;
	if(nMaskNum > HAL_CAN_MAX_MASK_CNT)
	{
		return CAN_MASK_SET_ERROR;
	}

	if(nChannel == CAN_CHANNEL_1)		//CH1은 0부터 CH2는 14부터 시작하며 필터갯수만큼 마스킹을 더 할 수 있지만, 우리 DB상의 한계로 1개밖에 쓰지 못한다
	{
		startFilterNum = 0;
		endFilterNum = nMaskNum;
        unCanChAddress = (unsigned int)HAL_CAN1;
	}
	else
	{
		startFilterNum = HAL_CAN_MAX_MASK_CNT;				// channel 2이면 14부터 시작
		endFilterNum = HAL_CAN_MAX_MASK_CNT + nMaskNum;
        unCanChAddress = (unsigned int)HAL_CAN2;
	}
	// Bit 2 IDE: Identifier extension
	// This bit defines the identifier type of message in the mailbox.
	// 0: Standard identifier.
	// 1: Extended identifier.
	// Bit 1 RTR: Remote transmission request
	// 0: Data frame
	// 1: Remote frame
	// IDList 모드 에는 standard 일 대는 인덱스가 4개 이다.
	// Mask 모드에는 standard 일 대는 인덱스가 2개이다.
	for(i = startFilterNum; i < endFilterNum; i++) 
    {
		startMaskVal = pStartMaskValue[i-startFilterNum];
		endMaskVal = pEndMaskValue[i-startFilterNum];

		if(nCANIDType == STANDARD_EXTENDED_CAN)
		{
			if((startMaskVal>>16) != 0) nCANIDType = EXTENDED_CAN;
			else						nCANIDType = STANDARD_CAN;
		}
		
		if(nCANIDType == STANDARD_CAN)
		{
			CAN_Ch_FilterInitStructure.CAN_FilterNumber = i;
			CAN_Ch_FilterInitStructure.CAN_FilterScale = HAL_CAN_FilterScale_16bit;
			
			if(startMaskVal == endMaskVal) {									//IdListMode에서는 입력된 CAN ID만 받을 수 있다.
				CAN_Ch_FilterInitStructure.CAN_FilterMode = HAL_CAN_FilterMode_IdList;	//CAN_FilterMode_IdList:0x01
				CAN_Ch_FilterInitStructure.CAN_FilterIdLow = ((startMaskVal&0x0000FFFF)<<5) & 0xFFE0;
				// Force Diable setting
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdLow = 0xFFF0;
				CAN_Ch_FilterInitStructure.CAN_FilterIdHigh = 0xFFF0;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFFF0;
			}
			else {
				//FilterMaskIdLow의 각 bit 값이 1이면 FilterIdLowIdlow의 bit값과 동일해야하고 0이면 don't care
				//FilterMaskIdLow의 각 bit 값이 1이면 FilterIdLowIdlow의 bit값과 동일해야하고 0이면 don't care
				CAN_Ch_FilterInitStructure.CAN_FilterMode = HAL_CAN_FilterMode_IdMask;
				CAN_Ch_FilterInitStructure.CAN_FilterIdLow = (((startMaskVal&0x0000FFFF)<<5)&((endMaskVal&0x0000FFFF)<<5)) & 0xFFE0;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdLow = (~(((startMaskVal&0x0000FFFF)<<5)^((endMaskVal&0x0000FFFF)<<5)))& 0xFFE0;
				// Force Diable setting
				CAN_Ch_FilterInitStructure.CAN_FilterIdHigh = 0xFFF0;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFFF0;
			}
		}
		else if(nCANIDType == EXTENDED_CAN)
		{
			CAN_Ch_FilterInitStructure.CAN_FilterNumber = i;
			CAN_Ch_FilterInitStructure.CAN_FilterScale = HAL_CAN_FilterScale_32bit;

			if(startMaskVal == endMaskVal)
			{
				CAN_Ch_FilterInitStructure.CAN_FilterMode = HAL_CAN_FilterMode_IdList;
				CAN_Ch_FilterInitStructure.CAN_FilterIdHigh = ((startMaskVal&0x1FFFE000)>>13);
				CAN_Ch_FilterInitStructure.CAN_FilterIdLow = ((startMaskVal&0x00001FFF)<<3) | 0x04;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFFFF;    
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdLow = 0xFFFC;    
			}
			else
			{
				CAN_Ch_FilterInitStructure.CAN_FilterMode = HAL_CAN_FilterMode_IdMask;
				CAN_Ch_FilterInitStructure.CAN_FilterIdHigh = ((endMaskVal&0x1FFFE000)>>13);
				CAN_Ch_FilterInitStructure.CAN_FilterIdLow = ((endMaskVal&0x00001FFF)<<3) | 0x04;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdHigh = ((startMaskVal&0x1FFFE000)>>13);
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdLow = ((startMaskVal&0x00001FFF)<<3) | 0x04;
			}
		}
		else 
		{
			return CAN_MASK_SET_ERROR;
		}
		
		/* CAN filter init */
		CAN_Ch_FilterInitStructure.CAN_FilterFIFOAssignment = HAL_CAN_Filter_FIFO0;
		CAN_Ch_FilterInitStructure.CAN_FilterActivation = HAL_ENABLE;
        
        HalDrvCanIOCtrl(eCAN_IO_FilterInit, unCanChAddress, 
                        (char*)&CAN_Ch_FilterInitStructure, 
                        sizeof(CAN_Ch_FilterInitStructure), 0);
	}

	return WORK_SUCCESS;
}

void Oem_CAN_Channel_Initialize(unsigned char usCanChannel, unsigned char ucComPortRelay,  unsigned long usCanBPS)
{
	if ( usCanChannel == CAN_CHANNEL_1 ) {
		Oem_CAN_Initial_CH1(ucComPortRelay, usCanBPS);
	}
	else if ( usCanChannel == CAN_CHANNEL_2 ) {
		Oem_CAN_Initial_CH2(ucComPortRelay, usCanBPS);
	}
}

//CAN라인을 Highcan2로 세팅하고 슬립들어갔을때 Highcan1으로 신호가 들어오면 CAN래치를 다 살려놓아도 웨이크업이 되지않음
//추후 래치사용으로 CAN으로 깨지않는지 세팅할때 주의 필요
void Oem_CAN_Initial_CH1(unsigned char usHighCan1,  unsigned char usCan1BPS)
{
	static unsigned char s_ucCanLine = 0;

	if(CFD_GetCanFDAdapter())
	{
		if(m_stCFDCtrl.unCh1CanLine == usHighCan1)
		{
			return;
		}
	}


	m_stCFDCtrl.unCh1CanLine = usHighCan1;

    if(s_ucCanLine == 0)
    s_ucCanLine = usHighCan1;

	unsigned char ucCanData[4];
	memset(&ucCanData,0x00,NULL);

    HalCan_ClearBuffer(eCOMM_TYPE_CAN1);    // TWODAM

	switch(usHighCan1) {
		case Highcan1:
//			printf("Init_H1 ");
			Oem_CAN1_STANDBY_ACTIVE();
			Oem_CAN1_SEL_HIGH_CAN1();
			Oem_CAN1_STANDBY_INACTIVE();

			HalCanSetBaudrate((int)HAL_CAN1, (eCanBaudrate)usCan1BPS);
            
			break;

		case Highcan2:
			Oem_CAN1_STANDBY_ACTIVE();
			Oem_CAN1_SEL_HIGH_CAN2();
			Oem_CAN1_STANDBY_INACTIVE();

//			printf("Init_H2 ");


			if(CFD_GetCanFDAdapter())
				HalCanSetBaudrate((int)HAL_CAN1, eCAN_1MBPS);
			else
				HalCanSetBaudrate((int)HAL_CAN1, (eCanBaudrate)usCan1BPS);

			if(CFD_GetCanFDAdapter() && (s_ucCanLine == Highcan1))
			{
				printf("---- [%s]  Highcan2 ByPass On---\n",__FUNCTION__);
				MD_uDelay(1000);
				ucCanData[0] = ucCanData[1] = ucCanData[2] = ucCanData[3] = 1;
				CFD_SetByPassFlagDirect(ucCanData);
				MD_uDelay(1000);
			}
			break;

	}

	m_stCFDCtrl.unChFinalCanLine = usHighCan1;
	s_ucCanLine = usHighCan1;


	return;
}

void Oem_CAN_Initial_CH2(unsigned char ucHighCan2, unsigned char usCan2BPS)
{
	if( CFD_GetCanFDAdapter() && (m_stCFDCtrl.unCh2CanLine == ucHighCan2) )
	{
		//printf(" return Oem_CAN_Initial_CH2 Now %d  \n ",m_stCFDCtrl.unCh2CanLine);
		return;
    }


	m_stCFDCtrl.unCh2CanLine = ucHighCan2;
    HalCan_ClearBuffer(eCOMM_TYPE_CAN2);    // TWODAM
	switch(ucHighCan2) {
		case Highcan3:
//			printf("Init_H3 \r\n");
			Oem_CAN2_STANDBY_ACTIVE();

			Oem_CAN2_SEL_HIGH_CAN3();
			Oem_CAN2_HIGH_CAN3_ENABLE();

			if(CFD_GetCanFDAdapter())
				HalCanSetBaudrate((int)HAL_CAN2, eCAN_1MBPS);
			else
				HalCanSetBaudrate((int)HAL_CAN2, (eCanBaudrate)usCan2BPS);

			break;

		case Lowcan1:
//			printf("Init_L1 \r\n");
			Oem_CAN2_STANDBY_ACTIVE();

			Oem_CAN2_SEL_LOW_CAN();
			Oem_CAN2_LOW_CAN_ENABLE();

			HalCanSetBaudrate((int)HAL_CAN2, (eCanBaudrate)usCan2BPS);
			break;
	}
	return;
}
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

float GetAlpha(unsigned int nAdc_Buf)
{
	if ( nAdc_Buf < 700 )							nAdc_Buf += 130;
	else if ( nAdc_Buf >= 700 && nAdc_Buf < 800 )	nAdc_Buf -= 4;
	else if ( nAdc_Buf >= 800 && nAdc_Buf < 900 )	nAdc_Buf += 0;
	else if ( nAdc_Buf >= 900 && nAdc_Buf < 1000 )	nAdc_Buf += 0;
	else if ( nAdc_Buf >= 1000 && nAdc_Buf < 1100 )	nAdc_Buf += 0;
	else if ( nAdc_Buf >= 1100 && nAdc_Buf < 1200 )	nAdc_Buf += 0;
	else if ( nAdc_Buf >= 1200 && nAdc_Buf < 1300 )	nAdc_Buf -= 3;
	else if ( nAdc_Buf >= 1300 && nAdc_Buf < 1400 )	nAdc_Buf -= 3;
	else if ( nAdc_Buf >= 1400 && nAdc_Buf < 1500 )	nAdc_Buf -= 4;
	else if ( nAdc_Buf >= 1500 && nAdc_Buf < 1600 )	nAdc_Buf -= 5;
	else if ( nAdc_Buf >= 1600 && nAdc_Buf < 1700 )	nAdc_Buf -= 3;
	else if ( nAdc_Buf >= 1700 && nAdc_Buf < 1800 )	nAdc_Buf -= 7;
	else if ( nAdc_Buf >= 1800 && nAdc_Buf < 1900 )	nAdc_Buf -= 7;
	else if ( nAdc_Buf >= 1900 && nAdc_Buf < 2000 )	nAdc_Buf -= 9;
	else if ( nAdc_Buf >= 2000 && nAdc_Buf < 2100 )	nAdc_Buf -= 10;
	else if ( nAdc_Buf >= 2100 && nAdc_Buf < 2200 )	nAdc_Buf -= 13;
	else if ( nAdc_Buf >= 2200 && nAdc_Buf < 2300 )	nAdc_Buf -= 11;
	else if ( nAdc_Buf >= 2300 && nAdc_Buf < 2400 )	nAdc_Buf -= 13;
	else if ( nAdc_Buf >= 2400 && nAdc_Buf < 2500 )	nAdc_Buf -= 12;
	else if ( nAdc_Buf >= 2500 && nAdc_Buf < 2600 )	nAdc_Buf -= 12;
	else if ( nAdc_Buf >= 2600 && nAdc_Buf < 2700 )	nAdc_Buf -= 10;
	else if ( nAdc_Buf >= 2700 && nAdc_Buf < 2800 )	nAdc_Buf -= 10;
	else if ( nAdc_Buf >= 2800 && nAdc_Buf < 2900 )	nAdc_Buf -= 10;
	else if ( nAdc_Buf >= 2900 && nAdc_Buf < 3000 )	nAdc_Buf -= 7;
	else if ( nAdc_Buf >= 3000 && nAdc_Buf < 3100 )	nAdc_Buf -= 8;
	else if ( nAdc_Buf >= 3100 && nAdc_Buf < 3200 )	nAdc_Buf -= 5;
	else if ( nAdc_Buf >= 3200 && nAdc_Buf < 3300 )	nAdc_Buf -= 270;
	else if ( nAdc_Buf >= 3300 && nAdc_Buf < 3400 )	nAdc_Buf -= 290;
	else if ( nAdc_Buf >= 3400 && nAdc_Buf < 3500 )	nAdc_Buf -= 287;
	else if ( nAdc_Buf >= 3500 && nAdc_Buf < 3600 )	nAdc_Buf -= 280;
	else if ( nAdc_Buf >= 3600 )								nAdc_Buf -= 280;

	GITDebugPrintf("\r\n[GetAlpha] nAdc_Buf %d\r\n", nAdc_Buf);

	return nAdc_Buf;

}

unsigned long Oem_GetBattVoltage(char adcx)
{
	unsigned short 	ulBattVolt=0;
	unsigned int 	nAdc_RawBuf   = 0;
	unsigned char 	uStep=0, uAdCnt = 0;
	unsigned int 	uOldTimer 	= 0;

	while(TRUE)
	{
		switch(uStep)
		{
		case 0:
			if (Get_TmrDelta(Get_Tmr(),uOldTimer) >= 100)
			{
				uOldTimer=Get_Tmr();
				uStep = 1;
				if(adcx==1)
                    HalADC_SetAdc_Chnnel((unsigned int)HAL_ADC1, 1);
				else
                    HalADC_SetAdc_Chnnel((unsigned int)HAL_ADC2, 1);
			}
			break;
		case 1:
			if(adcx==1)
			{
				if (HalADC_Read1(&ulBattVolt))
				{
					nAdc_RawBuf += ulBattVolt;
					//printf("\r\n adc1 adcnt %02d Batcnt %04d ", uAdCnt, ulBattVolt);
					uStep = 2;
				}
			}
			else
			{
				if (HalADC_Read2(&ulBattVolt))
				{
					nAdc_RawBuf += ulBattVolt;
					//printf("\r\n adc2 adcnt %02d Batcnt %04d ", uAdCnt, ulBattVolt);
					uStep = 2;
				}
			}
			break;

		case 2:
			uAdCnt++;
			if(uAdCnt==4)
			{
//				float fAlpha;
				u16  nAdc_Buf;
				nAdc_Buf = (u16)(nAdc_RawBuf/4.0);
				//GITDebugPrintf("nAdc_Buf %04d\r\n", nAdc_Buf);
				ulBattVolt = (unsigned short)GetAlpha( nAdc_Buf );

				//GITDebugPrintf("        ulBattVolt %05d mV\r\n", ulBattVolt);

				nAdc_RawBuf=0;
				uAdCnt=0;

				uStep=3;
			}
			else
			{
				uStep=0;  //continue adc
			}
			break;

		default:
			uStep=0;
			break;
		}//end switch-case

		if( uStep==3 )
		{
			uStep=0; break;//exit while loop
		}

	}//end while

	return ulBattVolt;
}


////////////////////////////////////////////////////////////////////////////////
// Concern abort GPIO

//********************************************************
// CAN1
//********************************************************
void Oem_CAN1_STANDBY_INACTIVE(void)	//Normal 모드
{
	HalGPIOSetVaule(GPIO_CAN1_ENABLE, eBIT_RESET);	//CANCHIP STB
}

void Oem_CAN1_STANDBY_ACTIVE(void)	//Standby 모드(저전력)
{
	HalGPIOSetVaule(GPIO_CAN1_ENABLE, eBIT_SET);	//CANCHIP STB
}

void Oem_CAN1_SEL_HIGH_CAN1(void)
{
	HalGPIOSetVaule(GPIO_HI_CAN2_CHK_CTL, eBIT_RESET);			// HIGH CAN1

	//********************************************************
	// SN74LVC374 latch에 clock을 인가한다.	Dx --> Qx 출력됨.
	//********************************************************
	HalGpioGenerateLatchClock();				// GPIO_LOW_HIGH_CAN_SEL, GPIO_HI_CAN2_CHK_CTL 도 연결되어있음
	//********************************************************
}

void Oem_CAN1_SEL_HIGH_CAN2(void)
{
	HalGPIOSetVaule(GPIO_HI_CAN2_CHK_CTL, eBIT_SET);				// HIGH CAN2

	//********************************************************
	// SN74LVC374 latch에 clock을 인가한다.	Dx --> Qx 출력됨.
	//********************************************************
	HalGpioGenerateLatchClock();				// GPIO_LOW_HIGH_CAN_SEL, GPIO_HI_CAN2_CHK_CTL 도 연결되어있음
	//********************************************************
}

//********************************************************
// CAN2
//********************************************************
void Oem_CAN2_HIGH_CAN3_ENABLE(void)	//Normal 모드
{
	HalGPIOSetVaule(GPIO_HIGH_CAN3_EN, eBIT_RESET);				// Enable HIGH_CAN3
}

void Oem_CAN2_HIGH_CAN3_DISABLE(void)	//Standby 모드(저전력)
{
	HalGPIOSetVaule(GPIO_HIGH_CAN3_EN, eBIT_SET);				// Disable HIGH_CAN3
}

void Oem_CAN2_LOW_CAN_ENABLE(void)
{
	HalGPIOSetVaule(GPIO_LOW_CAN_NSTB, eBIT_SET);	// LOW CAN Normal
	HalGPIOSetVaule(GPIO_LOW_CAN_EN, eBIT_SET);	// LOW CAN Enable
}

void Oem_CAN2_LOW_CAN_DISABLE(void)	//SLEEP모드
{
	HalGPIOSetVaule(GPIO_LOW_CAN_NSTB, eBIT_RESET);	// LOW CAN Stanby
	HalGPIOSetVaule(GPIO_LOW_CAN_EN, eBIT_RESET);	// LOW CAN Disable
}

void Oem_CAN2_SEL_HIGH_CAN3(void)
{
	HalGPIOSetVaule(GPIO_LOW_HIGH_CAN_SEL, eBIT_RESET);					// High: Input HIGH_CAN3

	//********************************************************
	// SN74LVC374 latch에 clock을 인가한다.	Dx --> Qx 출력됨.
	//********************************************************
	HalGpioGenerateLatchClock();				// GPIO_LOW_HIGH_CAN_SEL, GPIO_HI_CAN2_CHK_CTL 도 연결되어있음
	//********************************************************
}

void Oem_CAN2_SEL_LOW_CAN(void)
{
	HalGPIOSetVaule(GPIO_LOW_HIGH_CAN_SEL, eBIT_SET);				// Low: Input LOW_CAN

	//********************************************************
	// SN74LVC374 latch에 clock을 인가한다.	Dx --> Qx 출력됨.
	//********************************************************
	HalGpioGenerateLatchClock();				// GPIO_LOW_HIGH_CAN_SEL, GPIO_HI_CAN2_CHK_CTL 도 연결되어있음
	//********************************************************
}

void Oem_CAN2_STANDBY_ACTIVE(void)
{
	Oem_CAN2_HIGH_CAN3_DISABLE();
	Oem_CAN2_LOW_CAN_DISABLE();
}

//-----------------------------USB
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
void Oem_USB_RESEST_HI(void)
{
#if defined(STM32F2XX)
	GPIOA->BSRRL = GPIO_USB_RESET;	//high
#elif defined(FEATURE_GDS_VCI_AM)
#endif
}

void Oem_USB_RESEST_LO(void)
{
#if defined(STM32F2XX)
	GPIOA->BSRRH = GPIO_USB_RESET;	//low
#elif defined(FEATURE_GDS_VCI_AM)
#endif
}

void Oem_READ_USBCABLE_SEL(void)
{
#if defined(STM32F1XX) || defined(STM32F2XX)
//	!(GPIOB->IDR & GPIO_USB_CABLE_SEL);
#elif defined(FEATURE_GDS_VCI_AM)
#endif
}

void Oem_USB_POWER_HI(void)
{
#if defined(STM32F2XX)
	GPIOC->BSRRL = GPIO_IRQ;				//high -> OFF
#elif defined(FEATURE_GDS_VCI_AM)
#endif
}

void Oem_USB_POWER_LO(void)
{
#if defined(STM32F2XX)
	GPIOC->BSRRH = GPIO_IRQ;				//low  -> ON
#elif defined(FEATURE_GDS_VCI_AM)
#endif
}
#endif

/*****************************END OF FILE****/
